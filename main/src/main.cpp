#include <Arduino.h>
#include <driver/ledc.h>
#include <LittleFS.h>
#include <ESPAsyncWebSrv.h>
#include <ESPmDNS.h>
// #include <ESP32-TWAI-CAN.hpp>
#include <DJIMotorCtrlESP.hpp>
#define NAME "setogiwa"
#define CAN_TX 3
#define CAN_RX 2
const uint8_t
	srv_pin[5]={4,5,6,7,8};

AsyncWebServer svr(80);
AsyncWebSocket ws("/ws");
float
	srv[5]={0},
	robomas[2]={0},
	belt[3]={0},
	fan[6]={0};

uint16_t btn=0,st_raw=0xffff;
float st_x=0,st_y=0;
uint32_t t=0;

M3508_P19 RM_X(1);
M3508_P19 RM_Y(2);

float mix(float a,float b,float x){return a*(1.-x)+b*x;}
float clamp(float x,float a,float b){return fmax(a,fmin(x,b));}
#define saturate(X) clamp(X,0,1)
#define sign(X) ((0<X)-(X<0))

void servo_init(uint8_t ch,uint8_t pin){ledcSetup(ch,100,LEDC_TIMER_14_BIT);ledcAttachPin(pin,ch);}
void servo(uint8_t ch,float x){ledcWrite(ch,(x*2000+500)*1.6384);}// tick/us=(hz*(2^bit))/1000000 hz=100

void flush(){
	for(uint8_t i=0;i<5;i++)servo(i,srv[i]);
	RM_X.set_location(robomas[0]*65536*4);RM_Y.set_location(robomas[1]*65536*4);
	for(uint8_t i=0;i<3;i++)Serial.write(     (i<<5)|((uint8_t)((belt[i]*.5+.5)*31+1)));
	for(uint8_t i=0;i<6;i++)Serial.write(0x80|(i<<4)|((uint8_t)(fan[i]*15.)));
}
void st_calc(){
	if(st_raw==0xffff)return;
	float
		r0=mix(-1,1,(st_raw&0xff)/63.),
		r1=mix(-1,1,(st_raw>>8)/63.);
	st_x=(r0-r1)*.5;
	st_y=-(r0+r1)*.5;

	st_x=sign(st_x)*saturate(mix(-.3,1.05,abs(st_x)));
	st_y=sign(st_y)*saturate(mix(-.3,1.05,abs(st_y)));
}

void onWS(AsyncWebSocket *ws,AsyncWebSocketClient *client,AwsEventType type,void *arg,uint8_t *data,size_t len){
	if(type==WS_EVT_DATA){
		AwsFrameInfo *info=(AwsFrameInfo*)arg;
		if(info->final&&info->index==0&&info->len==len){
			for(uint8_t i=0;i<5;i++)srv[i]=*(float*)(data+(i<<2));
			//for(uint8_t i=0;i<2;i++)robomas[i]=*(float*)(data+((i+5)<<2));
			for(uint8_t i=0;i<3;i++)belt[i]=*(float*)(data+((i+5)<<2));
			for(uint8_t i=0;i<6;i++)fan[i]=*(float*)(data+((i+5+3)<<2));
			flush();
		}
	}
}

void setup(){
	Serial.begin(9600);
	can_init(CAN_TX,CAN_RX);
	RM_X.setup();//RM_X.reset_location();
	RM_Y.setup();//RM_Y.reset_location();
	LittleFS.begin();
	for(uint8_t i=0;i<5;i++){servo_init(i,srv_pin[i]);srv[i]=.5;}
	flush();
	delay(1000);
	// WiFi.begin("F660A-WKEE-G","dFA4ewgf");
	// for(uint8_t i=0;WiFi.status()!=WL_CONNECTED;i++){
	// 	servo(0,.52);delay(500);
	// 	servo(0,.48);delay(500);
	// }
	WiFi.softAP("setogiwa","setomono");
	
	MDNS.begin(NAME);
	ws.onEvent(onWS);svr.addHandler(&ws);
	svr.onNotFound([](AsyncWebServerRequest *request){request->redirect("/");});
	svr.serveStatic("/",LittleFS,"/").setDefaultFile("index.html");
	svr.begin();
}
void loop(){
	while(millis()<t)delay(1);t=millis()+20;
	if(Serial.available()>0){
		do{
			uint8_t x=Serial.read();
			switch(x>>6){
				case 0:{btn=(btn&0xffc0)|(x&0x3f);break;}
				case 1:{btn=(btn&0xf03f)|((x&0x3f)<<6);break;}
				case 2:{st_raw=(st_raw&0xff00)|(x&0x3f);st_calc();break;}
				case 3:{st_raw=(st_raw&0x00ff)|((x&0x3f)<<8);st_calc();break;}
			}
		}while(Serial.available()>0);
		ws.printfAll("{btn:0x%03x,st:[%f,%f],raw:0x%04x}",btn,st_x,st_y,st_raw);

		robomas[1]+=st_x*-.05;
		robomas[0]+=st_y*-.01;
		flush();
	}
}



/*\
|*|0x0CBA
|*|        B4    B8      
|*|    C8  B1    B2  A8  
|*|  C4  C1   +Y   A1  A4
|*|    C2   -X  +X   A2  
|*|           -Y         
\*/