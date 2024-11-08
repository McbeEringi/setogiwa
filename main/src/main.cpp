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

#define BTNPRSD(X) (btn&X)
#define NUNPRSD(X) (nunchuk&X)
#define WCTLPRSD(X) (wctl&X)

#define BTNDOWN(X) (BTNPRSD(X)&&!(btn_p&X))
#define NUNDOWN(X) (NUNPRSD(X)&&!(nunchuk_p&X))
#define WCTLDOWN(X) (WCTLPRSD(X)&&!(wctl_p&X))

const uint8_t
	srv_pin[5]={4,5,6,7,8},
	btn_hand[2]={0x80,0x40},
	nun_shoot_pos[2]={0x8,0x2},
	nun_shoot_lr=0x10,
	nun_belt_all_dir=0x20,
	nun_homerun=0x40,
	btn_hand_state=0x10,//0x1,
	btn_spd=0x4,
	btn_fandis=0x2,
	btn_homerun=0x20;
const uint16_t btn_blue_mode=0x100;////0x10;

const uint16_t
	wctl_dr=1<<0,
	wctl_du=1<<1,
	wctl_dl=1<<2,
	wctl_dd=1<<3,

	wctl_l=1<<4,
	wctl_r=1<<5,
	wctl_exp=1<<6,

	wctl_hr=1<<7,
	wctl_bunt=1<<8,
	wctl_belt=1<<9,

	wctl_nd=1<<10,
	wctl_np=1<<11,
	wctl_nu=1<<12;
const uint16_t
	wctl_hand[2]={wctl_l,wctl_r};


AsyncWebServer svr(80);
AsyncWebSocket ws("/ws");
float
	srv[5]={0},
	robomas[2]={0},
	robomas_real[2]={0},
	belt[3]={0},
	fan[6]={0};// fan .7

const float
	hand_pos[2][3]={// main sub
		{.15,.3,.92},// down set up
		{1.,.9,.22}
	},
	shoot_pos[3]={.1,.5,1.},//奥 手前
	shoot_lr[2]={.7,.2},//lr
	homerun_pos[2]={.65,.1},
	x_range=.89,
	y_range=6.95;

uint8_t st_raw=0x33,nunchuk=0,nunchuk_p=nunchuk,
	shoot_pos_i=1,shoot_lr_i=0,belt_all_dir=0,homerun=0,hand_state=0,blue_mode=0;
uint16_t btn=0,btn_p=btn,wctl=0,wctl_p=wctl;
float st_x=0,st_y=0,hand_state_x=hand_state;
uint32_t loopt=0,hand_t0[2]={UINT32_MAX>>1,UINT32_MAX>>1};

M3508_P19 RM_X(1);
M3508_P19 RM_Y(2);

float mix(float a,float b,float x){return a*(1.-x)+b*x;}
float clamp(float x,float a,float b){return fmax(a,fmin(x,b));}
#define saturate(X) clamp(X,0,1)
#define sign(X) ((0<X)-(X<0))
#define ls(A,B) ((A<B)-(B<A))
float smoothstep(float a,float b,float x){x=saturate((x-a)/(b-a));return x*x*(3-2*x);}
float step(float a,float x){return a<=x;}

void servo_init(uint8_t ch,uint8_t pin){ledcSetup(ch,100,LEDC_TIMER_14_BIT);ledcAttachPin(pin,ch);}
void servo(uint8_t ch,float x){ledcWrite(ch,(x*2000+500)*1.6384);}// tick/us=(hz*(2^bit))/1000000 hz=100

void flush(){
	for(uint8_t i=0;i<5;i++)servo(i,srv[i]);
	RM_X.set_location(robomas_real[0]*65536*4);RM_Y.set_location(robomas_real[1]*65536*4);
	for(uint8_t i=0;i<3;i++)Serial.write(     (i<<5)|((uint8_t)((belt[i]*.5+.5)*31+1)));
	for(uint8_t i=0;i<6;i++)Serial.write(0x80|(i<<4)|((uint8_t)(fan[i]*15.)));
}
void onWS(AsyncWebSocket *ws,AsyncWebSocketClient *client,AwsEventType type,void *arg,uint8_t *data,size_t len){
	if(type==WS_EVT_DATA){
		AwsFrameInfo *info=(AwsFrameInfo*)arg;
		if(info->final&&info->index==0&&info->len==len)wctl=*(uint16_t*)(data);
	}
}

void setup(){
	neopixelWrite(0,32,32,32);
	Serial.begin(9600,SERIAL_8E1);
	can_init(CAN_TX,CAN_RX);
	RM_X.setup();//RM_X.reset_location();
	RM_Y.setup();//RM_Y.reset_location();
	LittleFS.begin();
	for(uint8_t i=0;i<5;i++)servo_init(i,srv_pin[i]);
	srv[4]=shoot_lr[0];
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
	if(blue_mode)neopixelWrite(0,0,0,16);else neopixelWrite(0,16,0,0);
}
void loop(){
	// time ctl
	while(millis()<loopt)delay(1);loopt=millis()+20;

	// serial read
	if(Serial.available()>0){
		do{
			uint8_t x=Serial.read();
			if(x&0x40){//nunchuk
					uint8_t p=x&7;
					nunchuk=(nunchuk&(~(1<<p)))|(((x>>4)&1)<<p);
			}else{//ctl
				if(x&0x20){//st
					if(x&0x10)st_raw=(st_raw&0xf)|((x&0xf)<<4);
					else st_raw=(st_raw&0xf0)|(x&0xf);
					int8_t
						r0=(st_raw&0xf)-3,
						r1=(st_raw>>4)-3;
					st_x=sign(r0-r1)*fabs(pow((r0-r1)/6.,2.));
					st_y=sign(r0+r1)*fabs(pow((r0+r1)/6.,2.));
				}else{//btn
					uint8_t p=x&0xf;
					btn=(btn&(~(1<<p)))|(((x>>4)&1)<<p);
				}
			}
		}while(Serial.available()>0);
	}

	// TODO
	st_x=(WCTLPRSD(wctl_dl)?-1.:0.)+(WCTLPRSD(wctl_dr)?1.:0.);
	st_y=(WCTLPRSD(wctl_dd)?-1.:0.)+(WCTLPRSD(wctl_du)?1.:0.);

	// robomas 
	if(hand_state&&!(0.<robomas[0]&&st_y<0.)&&!(robomas[0]<-x_range&&0.<st_y))robomas[0]+=st_y*-.007*(BTNPRSD(btn_spd)?1.:.5);
	if(hand_state&&!((blue_mode?y_range:0.)<robomas[1]&&st_x<0.)&&!(robomas[1]<(blue_mode?0.:-y_range)&&0.<st_x))robomas[1]+=st_x*-.03*(BTNPRSD(btn_spd)?1.:.5);
	for(uint8_t i=0;i<2;i++)robomas_real[i]=mix(0,robomas[i],smoothstep(0.,1.,hand_state_x));

	// hand
	if(BTNDOWN(btn_hand_state)||WCTLDOWN(wctl_exp))hand_state=!hand_state;hand_state_x+=ls(hand_state_x,hand_state)*.01;
	// float xbelt_tmp=0;
	for(uint8_t i=0;i<2;i++){
		if(BTNDOWN(btn_hand[i])||WCTLDOWN(wctl_hand[i]))hand_t0[i]=millis();
		float t=(millis()-hand_t0[i])*.001;
		if((BTNPRSD(btn_hand[i])|WCTLPRSD(wctl_hand[i]))&&(.9<=t)){t=.9;hand_t0[i]=millis()-t*1000;}
		srv[i]=mix(hand_pos[i][2],mix(
			mix(
				mix(
					hand_pos[i][1],
					hand_pos[i][0],
					smoothstep(0.,.5,t)
				),
				hand_pos[i][2],
				smoothstep(.6,1.6,t)
			),
			hand_pos[i][1],
			smoothstep(2.6,3.3,t)
		),hand_state_x);

		fan[i]=mix(0.,.6,fmin(smoothstep(0.,.1,t),step(t,1.7))*((BTNPRSD(btn_fandis))?0.:1.)*hand_state_x);
		// xbelt_tmp=fmax(xbelt_tmp,fmin(smoothstep(1.,1.2,t),smoothstep(2.7,2.5,t))*hand_state_x);
	}
	belt[0]=.6*hand_state_x;//mix(0.,.6,xbelt_tmp);

	// shoot
	if((NUNDOWN(nun_shoot_pos[0])||WCTLDOWN(wctl_nu))&&(shoot_pos_i<2))shoot_pos_i++;
	if((NUNDOWN(nun_shoot_pos[1])||WCTLDOWN(wctl_nd))&&(0<shoot_pos_i))shoot_pos_i--;
	srv[3]=shoot_pos[shoot_pos_i];

	if((NUNDOWN(nun_shoot_lr)||WCTLDOWN(wctl_np)))shoot_lr_i=!shoot_lr_i;
	srv[4]=shoot_lr[shoot_lr_i];


	// belt
	// if(NUNDOWN(nun_belt_all_dir))belt_all_dir=(belt_all_dir+1)&3;
	// for(uint8_t i=0;i<3;i++)belt[i]=belt_all_dir&1?belt_all_dir&2?-.6:.6:0.;
	for(uint8_t i=1;i<3;i++)belt[i]=((NUNPRSD(nun_belt_all_dir)|WCTLPRSD(wctl_belt))?-.6:.6)*hand_state_x*(blue_mode&&i==1?-1.:1.);

	// homerun
	//if(BTNDOWN(btn_homerun))homerun=!homerun;
	srv[2]=homerun_pos[((BTNPRSD(btn_homerun)|WCTLPRSD(wctl_hr))?1:0)]+((NUNPRSD(nun_homerun)|WCTLPRSD(wctl_bunt))?.05:0.);

	// blue mode
	if(BTNDOWN(btn_blue_mode)){
		blue_mode=!blue_mode;
		if(blue_mode)neopixelWrite(0,0,0,16);else neopixelWrite(0,16,0,0);
	}
	
	flush();
	nunchuk_p=nunchuk;
	btn_p=btn;
	wctl_p=wctl;
}



/*\
|*|0x0CBA
|*|        B4    B8      
|*|    C8  B1    B2  A8  
|*|  C4  C1   +Y   A1  A4
|*|    C2   -X  +X   A2  
|*|           -Y         
\*/