#include <Arduino.h>

const uint8_t
	pu[4]={6,7,8,9},
	bus[3]={10,11,12},
	stick[2]={A0,A1};
uint16_t flag=0;

	// pu[4]={0,1,2,3},
	// bus[3]={8,9,10},
	// stick[2]={6,7};


void setup(){
	Serial1.begin(9600);
	pinMode(LED_BUILTIN,OUTPUT);
	for(uint8_t i=0;i<4;i++)pinMode(pu[i],INPUT_PULLUP);
	for(uint8_t i=0;i<3;i++){pinMode(bus[i],OUTPUT);digitalWrite(bus[i],HIGH);}
	for(uint8_t i=0;i<2;i++)pinMode(stick[i],INPUT);
}

void loop(){
	flag=0;
	for(uint8_t i=0;i<3;i++){
		digitalWrite(bus[i],LOW);
		for(uint8_t j=0;j<4;j++)flag=(flag<<1)|(!digitalRead(pu[j]));
		digitalWrite(bus[i],HIGH);
	}
	Serial1.write((0<<6)|((flag   )&0x3f));
	Serial1.write((1<<6)|((flag>>6)&0x3f));
	Serial1.write((2<<6)|((analogRead(stick[0])>>4)&0x3f));
	Serial1.write((3<<6)|((analogRead(stick[1])>>4)&0x3f));
	delay(50);
}
