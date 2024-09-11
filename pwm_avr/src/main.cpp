#include <Arduino.h>
#define BTN 3

#define ls(A,B) ((A<B)-(B<A))

const uint8_t pin[6]={0,1,5,6,7,10};
uint8_t t[8]={0},r[8]={0};

void setup(){
	Serial.swap(1);
	Serial.begin(9600);
	pinMode(BTN,INPUT_PULLUP);
	for(uint8_t i=0;i<6;i++)pinMode(pin[i],OUTPUT);
	TCA0.SPLIT.CTRLA=0b1011;
}

void loop(){
	if(Serial.available()>0){
		uint8_t x=Serial.read();
		if(x&0x80)t[(x>>4)&7]=x&15; //0~15
		else Serial.write(x);
	}
	for(uint8_t i=0;i<6;i++){
		if(!digitalRead(BTN))t[i]=0;
		r[i]+=ls(r[i],t[i]);
		analogWrite(pin[i],map(r[i],0,15,0,255));
	}
	delay(5);
}
