#include <LiquidCrystal.h>
#include <Encoder.h>

#define weld_button_pin 6
#define weld_output 7

#define probePin A0
#define capacitorPin A1

#define capacitorVoltageMultiplier 0.01523
unsigned long lastVoltageReadingTime = 0;
int lastCapacitorVoltage = 0;

//LCD pins
const int rs = 13, en = 12, d4 = 11, d5 = 10, d6 = 9, d7 = 8;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//Encoder Pins
#define encoder_button_pin 2
#define encoder_A 3
#define encoder_B 4
Encoder myEnc(encoder_A, encoder_B);

//menu values
//[man/auto,first_pulse_ms,pulse_delay_ms,second_pulse_ms,cooldown_ms]
//man/auto 0=manual, 1=automatic
int menu_values[] = {0,1,50,10,1000};
int menu_max_values[] = {1,100,500,100,6000};
char* menu_text[] = {"auto: ","clean:","delay:","pulse:","wait: "};

//selection menu vars
#define menu_size 2
int menu_n = 0; //menu item selection number
int menu_order[] = {3,0,1,2};

//menu layout:
//pulse: 0-100ms
//automatic: 0/1

int menu_level = 0; //0=top level menu, 1=adjusting a parameter

int encoder0Pos = 0;
int encoder0PinALast = LOW;
long oldPosition  = -999;

void setup() {
  pinMode(weld_button_pin, INPUT);
  pinMode(weld_output, OUTPUT);

  pinMode(encoder_button_pin, INPUT);
  pinMode(encoder_A, INPUT);
  pinMode(encoder_B, INPUT);
  
  //Serial.begin (9600);

  lcd.begin(16, 2);
}

int armed = 1; //variable to stop triggering immidietly after

void loop() {
  int encoder_dir = readEncoder();
  
  int autoTrigger = menu_values[0] && analogRead(probePin)>100;
  
  if(menu_values[0]==0){//man
    armed = armed || digitalRead(weld_button_pin); //arm if button is not pressed
  }else{//auto
    int notTouching = analogRead(probePin)<100 && ((getCapVoltage()-lastCapacitorVoltage)<0.1); // wait until not touching and not charging
    
    armed = armed || (notTouching && (digitalRead(weld_button_pin)));
  }
  if((!digitalRead(weld_button_pin) || autoTrigger) && armed){
    armed=0;
    if(autoTrigger){delay(500);}
    weld();
  }
  
  displayMenu();
  
  if(menu_level==0){
    menu_n+=encoder_dir;
    menu_n=constrain(menu_n,0,1);
  }else{//menu_level==1
    int value_n = menu_order[menu_n];
    menu_values[value_n]+=encoder_dir;
    if(menu_values[value_n]>20 && encoder_dir==1){menu_values[value_n]+=4;}
    if(menu_values[value_n]>25 && encoder_dir==-1){menu_values[value_n]-=4;}
    menu_values[value_n]=constrain(menu_values[value_n],0,menu_max_values[value_n]);
  }
  if(!digitalRead(encoder_button_pin)){
    menu_level=!menu_level;
    delay(50);
    while(!digitalRead(encoder_button_pin));
    delay(50);
  }
  if(millis()-lastVoltageReadingTime>100){
    lastCapacitorVoltage = getCapVoltage();
    lastVoltageReadingTime = millis();
  }
}

int readEncoder(){
  int encoder_dir=0;
  
  long newPosition = myEnc.read()/4;
  if (newPosition != oldPosition) {
    
    encoder_dir=(oldPosition>newPosition)*2-1;
    oldPosition = newPosition;
  }
  return encoder_dir;
}

float getCapVoltage(){
  return analogRead(capacitorPin)*capacitorVoltageMultiplier;
}

void displayMenu(){
  for(int i=0;i<menu_size;i++){
    int value_n = menu_order[i];

    lcd.setCursor(0, i);
    if(menu_n==i){
      if(menu_level==0){
          lcd.print(">");
        }else{
          lcd.print("\x7E");//arrow
        }
    }else{
      lcd.print(" ");
    }
    
    lcd.print(menu_text[value_n]);
    int value = menu_values[value_n];
    lcd.print(value);
    if(value_n!=0){
      lcd.print("ms   ");
    }
  }
  lcd.print("   ");
  lcd.print(getCapVoltage(),1);
  lcd.print("V   ");
}

void weld(){
  int first_pulse_ms = menu_values[1];
  int pulse_delay = menu_values[2];
  int second_pulse_ms = menu_values[3];
  int cooldown_ms = menu_values[4];
  
  lcd.setCursor(0, 0);
  lcd.print("Welding!        ");
  
  digitalWrite(weld_output, HIGH);
  delay(first_pulse_ms);
  digitalWrite(weld_output, LOW);
  
  delay(pulse_delay);
  
  digitalWrite(weld_output, HIGH);
  delay(second_pulse_ms);
  digitalWrite(weld_output, LOW);
  
  delay(cooldown_ms);
}
