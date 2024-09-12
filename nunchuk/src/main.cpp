#include <Arduino.h>
#define LED_BUILTIN 10

const uint8_t pu[7]={7,2,1,0, 8, 6,9}; //RULD S LR
uint8_t btn=0,btn_p=btn;

void send(uint8_t x){for(uint8_t i=0;i<3;i++)Serial.write(x);}

void setup(){
	Serial.begin(9600,SERIAL_8E1);
	pinMode(LED_BUILTIN,OUTPUT);
	for(uint8_t i=0;i<7;i++)pinMode(pu[i],INPUT_PULLUP);
}

void loop(){// 0b x10(pushed)0[i]
	btn=0;
	for(uint8_t i=0;i<7;i++){
		btn=btn|((!digitalRead(pu[i]))<<i);
		if((btn_p^btn)&(1<<i)){
			send(0x40|(((btn>>i)&1)<<4)|i);
			digitalWrite(LED_BUILTIN,HIGH);
		}
	}
	btn_p=btn;
	delay(50);
	digitalWrite(LED_BUILTIN,LOW);
}
