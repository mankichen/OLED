#include "REG52.h"
#include "oled.h"

typedef enum //星期
{
	Sunday,
	Monday,
	Tuesday,
	Wednesday,
	Thursday,
	Friday,
	Saturday
}Week;

//闹钟时间
typedef struct
{
	unsigned char hour[2];
	unsigned char minute[2];
}ALARM;

ALARM alarm = { 0 };//初始化闹钟
//封装时间
typedef struct {
	unsigned char year[4];//年
	unsigned char month[2];//月
	unsigned char day[2];//日
	Week week;
	unsigned char hour[2];
	unsigned char minute[2];
	unsigned char sec[2];
}TIME;

TIME time={0};

unsigned char mon=1;
unsigned char day=11;
unsigned char hy=6;
unsigned char temper[3] = { 2,7,3 };
u8 t=0;
char fla_num = 13;//显示需要调节的位
u8 select = 0;//时钟 调时间切换  1为调时间 0为正常显示时间
char alarmOn = 0;//闹钟默认关闭

//DS1302引脚定义
sbit RST = P0^ 4;
sbit IO = P0 ^ 3;
sbit SCK = P0 ^ 2;

//按键区
sbit key_address_add = P3 ^ 5;
//sbit key_address_sub = P3 ^ 4;
sbit key_select = P3 ^ 4;//时钟切换
sbit key_value_add = P3 ^ 6;
sbit key_value_sub = P3 ^ 7;

sbit beep = P2 ^ 3;//蜂鸣器

//DS1302地址定义
#define ds1302_sec_add			0x80		//秒数据地址
#define ds1302_min_add			0x82		//分数据地址
#define ds1302_hr_add			0x84		//时数据地址
#define ds1302_date_add			0x86		//日数据地址
#define ds1302_month_add		0x88		//月数据地址
#define ds1302_day_add			0x8a		//星期数据地址
#define ds1302_year_add			0x8c		//年数据地址
#define ds1302_control_add		0x8e		//控制数据地址
#define ds1302_charger_add		0x90 					 
#define ds1302_clkburst_add		0xbe

#define UCHAR	unsigned char
//初始时间定义
UCHAR time_buf[8] = { 0x20,0x10,0x06,0x01,0x23,0x59,0x55,0x02 };//初始时间2010年6月1号23点59分55秒 星期二

void Delay6ms()		//@11.0592MHz
{
	unsigned char i, j;
	i = 11;
	j = 190;
	do
	{
		while (--j);
	} while (--i);
}

void ds1302_init(void)
{
	RST = 0;			//RST脚置低
	SCK = 0;			//SCK脚置低
}
//向DS1302写入一字节数据
void ds1302_write_byte(UCHAR addr, UCHAR d)
{
	UCHAR i;
	RST = 1;					
	//启动DS1302总线	
	//写入目标地址：addr
	addr = addr & 0xFE;   
	//最低位置零，寄存器0位为0时写，为1时读
	for (i = 0; i < 8; i++) {
		if (addr & 0x01) {
			IO = 1;
		}
		else {
			IO = 0;
		}
		SCK = 1;      //²úÉúÊ±ÖÓ
		SCK = 0;
		addr = addr >> 1;
	}
	//产生时钟
	for (i = 0; i < 8; i++) {
		if (d & 0x01) {
			IO = 1;
		}
		else {
			IO = 0;
		}
		SCK = 1;    //产生时钟
		SCK = 0;
		d = d >> 1;
	}
	RST = 0;		//停止DS1302总线
}

//从DS1302读出一字节数据
UCHAR ds1302_read_byte(UCHAR addr) {

	UCHAR i, temp;
	RST = 1;					
	//启动DS1302总线
	//写入目标地址：addr
	addr = addr | 0x01;    
	//最低位置高，寄存器0位为0时写，为1时读
	for (i = 0; i < 8; i++) {
		if (addr & 0x01) {
			IO = 1;
		}
		else {
			IO = 0;
		}
		SCK = 1;
		SCK = 0;
		addr = addr >> 1;
	}
	//输出数据：temp
	for (i = 0; i < 8; i++) {
		temp = temp >> 1;
		if (IO) {
			temp |= 0x80;
		}
		else {
			temp &= 0x7F;
		}
		SCK = 1;
		SCK = 0;
	}
	RST = 0;					//停止DS1302总线
	return temp;
}
//向DS302写入时钟数据
void ds1302_write_time(void)
{
//	ds1302_write_byte(ds1302_control_add, 0x00);			//关闭写保护
//	ds1302_write_byte(ds1302_sec_add, 0x80);				//暂停时钟 
	//ds1302_write_byte(ds1302_charger_add,0xa9);	    //涓流充电
	ds1302_write_byte(ds1302_year_add, time_buf[1]);		//年
	ds1302_write_byte(ds1302_month_add, time_buf[2]);	//月
	ds1302_write_byte(ds1302_date_add, time_buf[3]);		//日 
	ds1302_write_byte(ds1302_hr_add, time_buf[4]);		//时 
	ds1302_write_byte(ds1302_min_add, time_buf[5]);		//分
	ds1302_write_byte(ds1302_sec_add, time_buf[6]);		//秒
	ds1302_write_byte(ds1302_day_add, time_buf[7]);		//周
	//ds1302_write_byte(ds1302_control_add, 0x80);			//打开写保护    
}

//从DS302读出时钟数据
void ds1302_read_time(void)
{
	time_buf[1] = ds1302_read_byte(ds1302_year_add);		//年  
	time_buf[2] = ds1302_read_byte(ds1302_month_add);		//月 
	time_buf[3] = ds1302_read_byte(ds1302_date_add);		//ÈÕ 
	time_buf[4] = ds1302_read_byte(ds1302_hr_add);		//日 
	time_buf[5] = ds1302_read_byte(ds1302_min_add);		//时 
	time_buf[6] = (ds1302_read_byte(ds1302_sec_add)) & 0x7f;//秒，屏蔽秒的第7位，避免超出59
	time_buf[7] = ds1302_read_byte(ds1302_day_add);		//周	
}

//星期闪烁
void flashWeek(UCHAR content)
{
	if (fla_num==12&&select)
	{
		if (t > 2)
		{
			OLED_ShowCHinese(68 + 32 + 8, 6, 14);
			return;
		}
	}
	OLED_ShowCHinese(68 + 32 + 8, 6, content);
}

void showWeek(Week week)
{
	switch (week)
	{
	case Sunday:
		flashWeek(10);
		//OLED_ShowCHinese(68 + 32 + 8, 6, 9);
		break;
	case Monday:
		flashWeek(4);
		//OLED_ShowCHinese(68 + 32 + 8, 6, 4);
		break;
	case Tuesday:
		flashWeek(5);
		//OLED_ShowCHinese(68 + 32 + 8, 6, 5);
		break;
	case Wednesday:
		flashWeek(6);
		//OLED_ShowCHinse(68 + 32 + 8, 6, 6);
		break;
	case Thursday:
		flashWeek(7);
		//OLED_ShowCHinese(68 + 32 + 8, 6, 7);
		break;
	case Friday:
		flashWeek(8);
		//OLED_ShowCHinese(68 + 32 + 8, 6, 8);
		break;
	default:
		flashWeek(9);
	//	OLED_ShowCHinese(68+32+8,6,4);
		break;
	}
}


void flashAlarm() {
	if (fla_num==13&&select){
		if (t > 2){
			OLED_ShowCHinese(56, 4, 14);
			return;
		}
	}
	if (alarmOn){
		OLED_ShowCHinese(56, 4, 17);//钩
	}else{
		OLED_ShowCHinese(56, 4, 16);//叉
	}
}
//数字闪烁
void flash(UCHAR x, UCHAR y, UCHAR content, char flashNum) {
	if (flashNum==fla_num&&select)//调时间
	{
		if (t > 2)
		{
			OLED_ShowChar(x, y, 45, 16);
			return;
		}
	}
	OLED_ShowChar(x, y, content+48, 16);
}



void show(TIME* time,unsigned char *temper){
	
	showWeek(time->week);
	//小时
	flash(28, 2, time->hour[0], 0);
	flash(36, 2, time->hour[1], 1);
	//分钟
	flash(54, 2, time->minute[0], 2);
	flash(62, 2, time->minute[1], 3);
	
	//秒
	flash(80, 2, time->sec[0], 4);
	flash(88, 2, time->sec[1], 5);

	OLED_ShowChar(45, 2, ':', 16);
	OLED_ShowChar(71, 2, ':', 16);
	//显示年
	flash(0, 4, time->year[0], 20);
	flash(8, 4, time->year[1], 20);
	flash(16, 4, time->year[2], 6);//后两位十位
	flash(24, 4, time->year[3], 7);//后两位个位
	//显示第几日
	flash(34, 6, time->day[0], 10);
	flash(42, 6, time->day[1], 11);

	//显示温度
	OLED_ShowChar(58, 0, 48 + temper[0], 16);//显示温度
	OLED_ShowChar(66, 0, 48 + temper[1], 16);
	OLED_ShowChar(82, 0, 48 + temper[2], 16);
	//月
	flash(0, 6, time->month[0], 8); 
	flash(8, 6, time->month[1], 9);
	
	//闹钟
	flash(74, 4, alarm.hour[0], 14);//闹钟时
	flash(82, 4, alarm.hour[1], 15);
	flash(98, 4, alarm.minute[0], 16);//闹钟分
	flash(106, 4, alarm.minute[1], 17);

	flashAlarm();//闹钟图标

}

//定时器2初始化
void Init_timer2(void)
{
 RCAP2H=0x3c;//赋T2初始值0x3cb0，溢出20次为1秒,每次溢出时间为50ms
 RCAP2L=0xb0;
 TR2=1;	     //启动定时器2
 ET2=1;		 //打开定时器2中断
 EA=1;		 //打开总中断
}

//判断是增加还是减少数值
void changeValue(char tmp) {
	switch (fla_num)
	{
	case 0://小时
		time_buf[4] += tmp * 16;
		break;
	case 1:
		time_buf[4] += tmp;
		break;
	case 2://分钟
		time_buf[5] += tmp * 16;
		break;
	case 3:
		time_buf[5] += tmp;
		break;
	case 4://秒
		time_buf[6] += tmp * 16;
		break;
	case 5:
		time_buf[6] += tmp;
		break;
	case 6://年
		time_buf[1] += tmp * 16;
		break;
	case 7:
		time_buf[1] += tmp ;
		break;
	case 8://月
		time_buf[2] += tmp * 16;
		break;
	case 9:
		time_buf[2] += tmp;
		break;
	case 10://日
		time_buf[3] += tmp * 16;
		break;
	case 11:
		time_buf[3] += tmp;
		break;
	case 12://星期
		time_buf[7] += tmp;
		break;
		//以下为闹钟部分
	case 14:
		alarm.hour[0] += tmp;
		break;
	case 15:
		alarm.hour[1] += tmp;
		break;
	case 16:
		alarm.minute[0] += tmp;
		break;
	case 17:
		alarm.minute[1] += tmp;
		break;
	default://闹钟图标 13
		alarmOn = (++alarmOn) % 2;
		break;
	}
}
void scanKey() {
	if (key_address_add==0&&select)
	{
		Delay6ms();
		if (key_address_add==0)
		{
			fla_num++;
			if (fla_num==18)
			{
				fla_num = 0;
			}
		}
	}
	/*if (key_address_sub == 0)
	{
		Delay6ms();
		if (key_address_sub == 0)
		{
			fla_num--;
			if (fla_num <0)
			{
				fla_num = 10;
			}
		}
	}*/
	if (key_select==0)
	{
		Delay6ms();
		if (key_select==0)
		{
			if (++select > 1)
			{
				select = 0;
			}
			if (select)//selet为1时调时间，0为正常显示时间
			{
				ds1302_write_byte(ds1302_control_add, 0x00);			//关闭写保护
				ds1302_write_byte(ds1302_sec_add, 0x80);				//暂停时钟 
			}
			else
			{
				ds1302_write_time();
				ds1302_write_byte(ds1302_control_add, 0x80);			//打开写保护
				ds1302_read_time();
			}
			
		}
	}

	if (key_value_add == 0)
	{
		Delay6ms();
		if (key_value_add == 0)
		{
			changeValue(1);
		}
	}
	if (key_value_sub == 0)
	{
		Delay6ms();
		if (key_value_sub == 0)
		{
			changeValue(-1);
		}
	}
}

void TimeInit({
	Init_timer2();

	OLED_Init();			//初始化OLED  
	OLED_Clear();
	OLED_ShowCHinese(16, 0, 11);//温
	OLED_ShowCHinese(32, 0, 12);//度
	OLED_ShowChar(48, 0, ':', 16);//
	OLED_ShowChar(76,0,'.',16);//.

	OLED_ShowCHinese(96, 0, 13);//℃

	OLED_ShowCHinese(34, 4, 15);//年
	OLED_ShowCHinese(16,6,0);//月
	OLED_ShowCHinese(48 + 5,6,1);//日
	OLED_ShowCHinese(68 + 8,6,2);//星
	OLED_ShowCHinese(68 + 8 + 16,6,3);//期

									  /*闹钟部分******************************************************************/
	OLED_ShowCHinese(56, 4, 16);//叉
	OLED_ShowChar(90, 4, ':', 16);//：
}

//显示屏幕的数据
void ShowTime() {
	if (alarmOn&&!select)//闹钟
	{
		if (alarm.hour[0] == time.hour[0] && alarm.hour[1] == time.hour[1] && alarm.minute[0] == time.minute[0] && alarm.minute[1] == time.minute[1])//判断时间是否相等
		{
			beep = 0;//蜂鸣器响
		}
		else
		{
			beep = 1;
		}
	}
	show(&time, temper);
	scanKey();
}
// int main(void)
// {
//	
//	Init_timer2();
//	
//	OLED_Init();			//初始化OLED  
//	OLED_Clear(); 
//	OLED_ShowCHinese(16, 0, 11);//温
//	OLED_ShowCHinese(32, 0, 12);//度
//	OLED_ShowChar(48, 0, ':', 16);//
//	OLED_ShowChar(76,0,'.',16);//.
//	 
//	OLED_ShowCHinese(96, 0, 13);//℃
//
//	OLED_ShowCHinese(34, 4, 15);//年
//	OLED_ShowCHinese(16,6,0);//月
//	OLED_ShowCHinese(48+5,6,1);//日
//	OLED_ShowCHinese(68+8,6,2);//星
//	OLED_ShowCHinese(68+8+16,6,3);//期
//
//	/*闹钟部分******************************************************************/
//	OLED_ShowCHinese(56, 4, 16);//叉
//	OLED_ShowChar(90, 4, ':', 16);//：
//
//	do{
//		if (alarmOn&&!select)//闹钟
//		{
//			if (alarm.hour[0]==time.hour[0]&&alarm.hour[1]==time.hour[1]&&alarm.minute[0]== time.minute[0]&&alarm.minute[1]==time.minute[1])//判断时间是否相等
//			{
//				beep = 0;//蜂鸣器响
//			}
//			else
//			{
//				beep = 1;
//			}
//		}
//		show(&time, temper);
//		scanKey();
//	}while(1);
//
//}
 
 //定时器中断函数
void Timer2() interrupt 5	  //定时器2是5号中断
{
	TF2=0;
	 t++;
	if(t==4)               //间隔200ms(50ms*4)读取一次时间
	{
		t=0;
		if (!select)
		{
			ds1302_read_time();  //读取时间 
		}
	
	   time.year[0]=(time_buf[0]>>4); //年   
	   time.year[1]=(time_buf[0]&0x0f);
   
	   time.year[2]=(time_buf[1]>>4);
	   time.year[3]=(time_buf[1]&0x0f);
   
	   time.month[0]=(time_buf[2]>>4); //月  
	   time.month[1] =(time_buf[2]&0x0f);
   

	   time.day[0]=(time_buf[3]>>4); //日   
	   time.day[1] =(time_buf[3]&0x0f);
   
	   time.week=(time_buf[7]&0x07); //星期
   
	   //第2行显示  
	   time.hour[0]=(time_buf[4]>>4); //时   
	   time.hour[1]=(time_buf[4]&0x0f);   

	   time.minute[0]=(time_buf[5]>>4); //分   
	   time.minute[1]=(time_buf[5]&0x0f);   

	   time.sec[0]=(time_buf[6]>>4); //秒   
	   time.sec[1]=(time_buf[6]&0x0f);
   
	  }
}

	