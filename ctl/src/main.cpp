#include <Arduino.h>

const uint8_t
	pu[4]={0,1,2,3},
	bus[3]={6,7,10},
	stick[2]={8,9};
uint8_t st=0x88,st_p=st;
uint16_t btn=0,btn_p=btn;

void send(uint8_t x){for(uint8_t i=0;i<3;i++)Serial.write(x);}
uint8_t st_read(uint8_t i){
	uint16_t x=analogRead(stick[i]);// 0~1023
	const uint8_t d=64;
	return 6-(
		constrain(map(x,  0+d, 511-d,0,3),0,3)+
		constrain(map(x,512+d,1023-d,0,3),0,3)
	);
}

void setup(){
	Serial.begin(9600,SERIAL_8E1);
	for(uint8_t i=0;i<4;i++)pinMode(pu[i],INPUT_PULLUP);
	for(uint8_t i=0;i<3;i++){pinMode(bus[i],OUTPUT);digitalWrite(bus[i],HIGH);}
	for(uint8_t i=0;i<2;i++)pinMode(stick[i],INPUT);
}

void loop(){// 0b xx(st?)(st?i:pushed)[st?st_data:i]
	btn=0;
	for(uint8_t i=0,p=0;i<3;i++){
		digitalWrite(bus[i],LOW);
		for(uint8_t j=0;j<4;j++,p++){
			btn=btn|((!digitalRead(pu[j]))<<p);
			if((btn_p^btn)&(1<<p))send((0<<5)|(((btn>>p)&1)<<4)|p);
		}
		digitalWrite(bus[i],HIGH);
	}
	st=st_read(0)|(st_read(1)<<4);
	if((st&0xf)!=(st_p&0xf))send((0b10<<4)|(st&0xf));
	if((st>>4 )!=(st_p>>4 ))send((0b11<<4)|(st>>4 ));

	btn_p=btn;st_p=st;
	delay(50);
}
