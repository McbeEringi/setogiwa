#include <Arduino.h>
// #define XA 1// A5
// #define XB 5// B2
// #define YA 0// A4
// #define YB 6// B1
// #define EA 10// A3
// #define EB 7// B0
#define BTN 3 

#define ls(A,B) ((A<B)-(B<A))

const uint8_t
	pinA[3]={1,0,10},
	pinB[3]={5,6,7};
int8_t t[4]={0},r[4]={0};

void setup(){
	Serial.swap(1);
	Serial.begin(9600,SERIAL_8E1);
	pinMode(BTN,INPUT_PULLUP);
	for(uint8_t i=0;i<3;i++){
		pinMode(pinA[i],OUTPUT);pinMode(pinB[i],OUTPUT);
	}
	TCA0.SPLIT.CTRLA=0b1011;
}

void loop(){
	if(Serial.available()>0){
		uint8_t x=Serial.read();
		if(x&0x80)Serial.write(x);
		else t[x>>5]=(x&0x1f)-16; //-16~15
	}
	for(uint8_t i=0;i<3;i++){
		if(!digitalRead(BTN))t[i]=0;
		r[i]+=ls(r[i],t[i]);
		analogWrite(pinA[i],255-max(0,-r[i])*16);
		analogWrite(pinB[i],255-max(0, r[i])*16);
	}
	delay(5);
}
