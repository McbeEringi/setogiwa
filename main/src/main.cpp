#include <Arduino.h>
#include <driver/ledc.h>
#include <LittleFS.h>
#include <ESPAsyncWebSrv.h>
#include <ESPmDNS.h>
// #include <ESP32-TWAI-CAN.hpp>
#include <DJIMotorCtrlESP.hpp>
#define NAME "setogiwa"
#define SERVO0 4
#define SERVO1 5
#define SERVO2 6
#define SERVO3 7
#define SERVO4 8
#define CAN_TX 3
#define CAN_RX 2

AsyncWebServer svr(80);
AsyncWebSocket ws("/ws");
float
	srv[5]={.5},
	robomas[2]={0},
	belt[3]={0};

M3508_P19 RM_X(1);
M3508_P19 RM_Y(2);

void servo_init(uint8_t ch,uint8_t pin){ledcSetup(ch,100,LEDC_TIMER_14_BIT);ledcAttachPin(pin,ch);}
void servo(uint8_t ch,float x){ledcWrite(ch,(x*2000+500)*1.6384);}// tick/us=(hz*(2^bit))/1000000 hz=100

void flush(){
	for(uint8_t i=0;i<5;i++)servo(i,srv[i]);
	RM_X.set_location(robomas[0]*65536*4);RM_Y.set_location(robomas[1]*65536*4);
	for(uint8_t i=0;i<3;i++)Serial.write((i<<6)|((uint8_t)((belt[i]*.5+.5)*63+1)));
}

void onWS(AsyncWebSocket *ws,AsyncWebSocketClient *client,AwsEventType type,void *arg,uint8_t *data,size_t len){
	if(type==WS_EVT_DATA){
		AwsFrameInfo *info=(AwsFrameInfo*)arg;
		if(info->final&&info->index==0&&info->len==len){
			for(uint8_t i=0;i<5;i++)srv[i]=*(float*)(data+(i<<2));
			for(uint8_t i=0;i<2;i++)robomas[i]=*(float*)(data+((i+5)<<2));
			for(uint8_t i=0;i<3;i++)belt[i]=*(float*)(data+((i+5+2)<<2));
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
	servo_init(0,SERVO0);servo(0,.5);
	servo_init(1,SERVO1);servo(1,.5);
	servo_init(2,SERVO2);servo(2,.5);
	servo_init(3,SERVO3);servo(3,.5);
	servo_init(4,SERVO4);servo(4,.5);
	for(uint8_t i=0;i<3;i++){Serial.write((i<<6)|32);}
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
	delay(1000);
}
