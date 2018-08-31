/**
	************************************************************
	************************************************************
	************************************************************
	*	文件名： 	main.c
	*
	*	作者： 		张继瑞
	*
	*	日期： 		2016-11-23
	*
	*	版本： 		V1.0
	*
	*	说明： 		完成单片机初始化、外接IC初始化和任务的创建及运行
	*
	*	修改记录：	
	************************************************************
	************************************************************
	************************************************************
**/

//单片机头文件
#include "stm32f10x.h"

//OS
#include "includes.h"

//协议
#include "onenet.h"
#include "fault.h"

//网络设备
#include "net_device.h"
#include "net_io.h"

//硬件驱动
#include "led.h"
#include "delay.h"
#include "key.h"
#include "lcd1602.h"
#include "usart.h"
#include "hwtimer.h"
#include "i2c.h"
#include "adxl362.h"
#include "sht20.h"
#include "iwdg.h"
#include "at24c02.h"
#include "selfcheck.h"
#include "beep.h"
#include "info.h"
#include "spilcd.h"
#include "clock.h"
#include "rtc.h"
#include "light.h"
#include "ir.h"
#include "nec.h"

//中文数据流
#include "dataStreamName.h"

//字库
#include "font.h"

//C库
#include <string.h>
#include <time.h>
#include <stdio.h>


#define SPILCD_EN		1		//1-使用SPILCD		0-使用LCD1602


//看门狗任务
#define IWDG_TASK_PRIO		6
#define IWDG_STK_SIZE		64
OS_STK IWDG_TASK_STK[IWDG_STK_SIZE];
void IWDG_Task(void *pdata);

//串口任务
#define USART_TASK_PRIO		7
#define USART_STK_SIZE		256
OS_STK USART_TASK_STK[USART_STK_SIZE]; //
void USART_Task(void *pdata);

//心跳任务
#define HEART_TASK_PRIO		8
#define HEART_STK_SIZE		64
OS_STK HEART_TASK_STK[HEART_STK_SIZE]; //
void HEART_Task(void *pdata);

//故障处理任务
#define FAULT_TASK_PRIO		9 //
#define FAULT_STK_SIZE		256
OS_STK FAULT_TASK_STK[FAULT_STK_SIZE]; //
void FAULT_Task(void *pdata);

//传感器任务
#define SENSOR_TASK_PRIO	10
#define SENSOR_STK_SIZE		256
__align(8) OS_STK SENSOR_TASK_STK[SENSOR_STK_SIZE]; //UCOS使用浮点数进行printf、sprintf时一定要8字节对齐
void SENSOR_Task(void *pdata);

//数据发送任务
#define SEND_TASK_PRIO		11
#define SEND_STK_SIZE		64
OS_STK SEND_TASK_STK[SEND_STK_SIZE];
void SEND_Task(void *pdata);

//按键任务
#define KEY_TASK_PRIO		12
#define KEY_STK_SIZE		256
OS_STK KEY_TASK_STK[KEY_STK_SIZE]; //UCOS使用浮点数进行printf、sprintf时一定要8字节对齐
void KEY_Task(void *pdata);

//网络初始化任务
#define NET_TASK_PRIO		13 //
#define NET_STK_SIZE		512
OS_STK NET_TASK_STK[NET_STK_SIZE]; //
void NET_Task(void *pdata);

//数据反馈任务
#define DATA_TASK_PRIO		14 //
#define DATA_STK_SIZE		512
__align(8) OS_STK DATA_TASK_STK[DATA_STK_SIZE]; //UCOS使用浮点数进行printf、sprintf时一定要8字节对齐
void DATA_Task(void *pdata);

//数据发送任务
#define DATALIST_TASK_PRIO		15 //
#define DATALIST_STK_SIZE		128
OS_STK DATALIST_TASK_STK[DATALIST_STK_SIZE];
void DATALIST_Task(void *pdata);

//信息更改任务
#define ALTER_TASK_PRIO		16 //
#define ALTER_STK_SIZE		256
OS_STK ALTER_TASK_STK[ALTER_STK_SIZE]; //
void ALTER_Task(void *pdata);

//时钟任务
#define CLOCK_TASK_PRIO		17 //
#define CLOCK_STK_SIZE		256
OS_STK CLOCK_TASK_STK[CLOCK_STK_SIZE]; //
void CLOCK_Task(void *pdata);


#define NET_TIME	60			//设定时间--单位秒


unsigned short timerCount = 0;	//时间计数--单位秒


OS_TMR *tmr;					//软件定时器句柄


char myTime[24];


//数据流
DATA_STREAM dataStream[] = {
								{ZW_REDLED, &led_status.Led2Sta, TYPE_BOOL, 1},
								{ZW_GREENLED, &led_status.Led3Sta, TYPE_BOOL, 1},
								{ZW_YELLOWLED, &led_status.Led4Sta, TYPE_BOOL, 1},
								{ZW_BLUELED, &led_status.Led5Sta, TYPE_BOOL, 1},
								{ZW_BEEP, &beepInfo.Beep_Status, TYPE_BOOL, 1},
								{ZW_TEMPERATURE, &sht20Info.tempreture, TYPE_FLOAT, 1},
								{ZW_HUMIDITY, &sht20Info.humidity, TYPE_FLOAT, 1},
								{ZW_X, &adxl362Info.x, TYPE_FLOAT, 1},
								{ZW_Y, &adxl362Info.y, TYPE_FLOAT, 1},
								{ZW_Z, &adxl362Info.z, TYPE_FLOAT, 1},
								{ZW_BG, &spilcd_info.blSta, TYPE_UCHAR, 1},
								{ZW_TIME, myTime, TYPE_STRING, 1},
								{"GPS", &gps, TYPE_GPS, 0},
								{ZW_ERRTYPE, &faultTypeReport, TYPE_UCHAR, 1},
							};
unsigned char dataStreamCnt = sizeof(dataStream) / sizeof(dataStream[0]);


const char *topics[] = {"kylinV24-test", "pcTopic"};


/*
************************************************************
*	函数名称：	Hardware_Init
*
*	函数功能：	硬件初始化
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：		初始化单片机功能以及外接设备
************************************************************
*/
void Hardware_Init(void)
{
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);								//中断控制器分组设置

	Delay_Init();																//Timer4初始化
	
	Led_Init();																	//LED初始化
	
	Key_Init();																	//按键初始化
	
	Beep_Init();																//蜂鸣器初始化
	
	LIGHT_Init();																//光敏电阻初始化
	
	IR_Init(38000);																//红外发射管初始化
	
	IIC_Init();																	//软件IIC总线初始化
	
#if(SPILCD_EN == 1)
	SPILCD_Init();																//SPILCD初始化
#else
	Lcd1602_Init();																//LCD1602初始化
#endif
	
	Usart1_Init(115200); 														//初始化串口   115200bps
#if(USART_DMA_RX_EN)
	USARTx_ResetMemoryBaseAddr(USART_DEBUG, (unsigned int)alterInfo.alterBuf, sizeof(alterInfo.alterBuf), USART_RX_TYPE);
#endif
	
	RTC_Init();																	//初始化RTC
	
	Check_PowerOn(); 															//上电自检

	if(RCC_GetFlagStatus(RCC_FLAG_IWDGRST) == SET) 								//如果是看门狗复位则提示
	{
		UsartPrintf(USART_DEBUG, "WARN:	IWDG Reboot\r\n");
		
		RCC_ClearFlag();														//清除看门狗复位标志位
		
		faultTypeReport = faultType = FAULT_REBOOT; 							//标记为重启错误
		
		netDeviceInfo.reboot = 1;
		
		if(!Info_Check() && checkInfo.EEPROM_OK)								//如果EEPROM里有信息
			Info_Read();
	}
	else
	{
		//先读出ssid、pswd、devid、apikey
		if(!Info_Check() && checkInfo.EEPROM_OK)								//如果EEPROM里有信息
		{
			//AT24C02_Clear(0, 255, 256);Iwdg_Feed();
			UsartPrintf(USART_DEBUG, "1.ssid_pswd in EEPROM\r\n");
			Info_Read();
		}
		else //没有数据
		{
			UsartPrintf(USART_DEBUG, "1.ssid_pswd in ROM\r\n");
		}
		
		UsartPrintf(USART_DEBUG, "2.DEVID: %s,     APIKEY: %s\r\n"
								, oneNetInfo.devID, oneNetInfo.apiKey);
		
		netDeviceInfo.reboot = 0;
	}
	
	Iwdg_Init(4, 1250); 														//64分频，每秒625次，重载1250次，2s
	
	RTOS_TimerInit();
	
	UsartPrintf(USART_DEBUG, "3.Hardware init OK\r\n");							//提示初始化完成

}

/*
************************************************************
*	函数名称：	OS_TimerCallBack
*
*	函数功能：	定时检查网络状态标志位
*
*	入口参数：	ptmr：软件定时器句柄
*				p_arg：软件定时器回调函数参数
*
*	返回参数：	无
*
*	说明：		定时器任务。定时检查网络状态，若持续超过设定时间无网络连接，则进行平台重连
************************************************************
*/
void OS_TimerCallBack(OS_TMR *ptmr, void *p_arg)
{
	
	if(oneNetInfo.netWork == 0)											//如果网络断开
	{
		if(++timerCount >= NET_TIME) 									//如果网络断开超时
		{
			UsartPrintf(USART_DEBUG, "Tips:		Timer Check Err\r\n");
			
			checkInfo.NET_DEVICE_OK = 0;								//置设备未检测标志
			
			NET_DEVICE_ReConfig(0);										//设备初始化步骤设置为开始状态
			
			oneNetInfo.netWork = 0;
		}
	}
	else
	{
		timerCount = 0;													//清除计数
	}

}

/*
************************************************************
*	函数名称：	main
*
*	函数功能：	完成初始化任务，创建应用任务并执行
*
*	入口参数：	无
*
*	返回参数：	0
*
*	说明：		
************************************************************
*/
int main(void)
{
	
	unsigned char err;
	
	Hardware_Init();								//硬件初始化

	OSInit();										//RTOS初始化
	
	//创建应用任务
	
	OSTaskCreate(IWDG_Task, (void *)0, (OS_STK*)&IWDG_TASK_STK[IWDG_STK_SIZE - 1], IWDG_TASK_PRIO);
	
	OSTaskCreate(USART_Task, (void *)0, (OS_STK*)&USART_TASK_STK[USART_STK_SIZE - 1], USART_TASK_PRIO);
	
	OSTaskCreate(HEART_Task, (void *)0, (OS_STK*)&HEART_TASK_STK[HEART_STK_SIZE - 1], HEART_TASK_PRIO);
	
	OSTaskCreate(FAULT_Task, (void *)0, (OS_STK*)&FAULT_TASK_STK[FAULT_STK_SIZE - 1], FAULT_TASK_PRIO);
	
	OSTaskCreate(SENSOR_Task, (void *)0, (OS_STK*)&SENSOR_TASK_STK[SENSOR_STK_SIZE - 1], SENSOR_TASK_PRIO);
	
	OSTaskCreate(SEND_Task, (void *)0, (OS_STK*)&SEND_TASK_STK[SEND_STK_SIZE - 1], SEND_TASK_PRIO);
	
	OSTaskCreate(KEY_Task, (void *)0, (OS_STK*)&KEY_TASK_STK[KEY_STK_SIZE - 1], KEY_TASK_PRIO);
	
	OSTaskCreate(NET_Task, (void *)0, (OS_STK*)&NET_TASK_STK[NET_STK_SIZE - 1], NET_TASK_PRIO);
	
	OSTaskCreate(DATA_Task, (void *)0, (OS_STK*)&DATA_TASK_STK[DATA_STK_SIZE - 1], DATA_TASK_PRIO);
	
	OSTaskCreate(DATALIST_Task, (void *)0, (OS_STK*)&DATALIST_TASK_STK[DATALIST_STK_SIZE - 1], DATALIST_TASK_PRIO);
	
	OSTaskCreate(ALTER_Task, (void *)0, (OS_STK*)&ALTER_TASK_STK[ALTER_STK_SIZE - 1], ALTER_TASK_PRIO);
	
	OSTaskCreate(CLOCK_Task, (void *)0, (OS_STK*)&CLOCK_TASK_STK[CLOCK_STK_SIZE - 1], CLOCK_TASK_PRIO);
	
	tmr = OSTmrCreate(100, 100, OS_TMR_OPT_PERIODIC, (OS_TMR_CALLBACK)OS_TimerCallBack, 0, (INT8U *)"tmr1", &err);
	OSTmrStart(tmr, &err);
	
	UsartPrintf(USART_DEBUG, "4.OSStart\r\n");		//提示任务开始执行
	
	OSStart();										//开始执行任务
	
	return 0;

}

/*
************************************************************
*	函数名称：	IWDG_Task
*
*	函数功能：	清除看门狗
*
*	入口参数：	void类型的参数指针
*
*	返回参数：	无
*
*	说明：		看门狗任务
************************************************************
*/
void IWDG_Task(void *pdata)
{

	while(1)
	{
	
		Iwdg_Feed(); 		//喂狗
		
		RTOS_TimeDly(50); 	//挂起任务250ms
	
	}

}

/*
************************************************************
*	函数名称：	KEY_Task
*
*	函数功能：	扫描按键是否按下，如果有按下，进行对应的处理
*
*	入口参数：	void类型的参数指针
*
*	返回参数：	无
*
*	说明：		按键任务
************************************************************
*/
void KEY_Task(void *pdata)
{

	while(1)
	{
		
		switch(Keyboard())								//扫描按键状态
		{
			case KEY0DOWN:								//如果是key0单击事件
				
				if(led_status.Led2Sta == LED_OFF)
					Led4_Set(LED_ON);
				else
					Led4_Set(LED_OFF);
				
				oneNetInfo.sendData = SEND_TYPE_OK;		//标记数据反馈
			
			break;
			
			case KEY1DOWN:
				
				//if(ledStatus.Led5Sta == LED_OFF)
			//		Led5_Set(LED_ON);
			//	else
				//	Led5_Set(LED_OFF);
			
				oneNetInfo.sendData = SEND_TYPE_OK;		//标记数据反馈
				
			break;
			
			case KEY2DOWN:
				
				//if(ledStatus.Led6Sta == LED_OFF)
				//	Led2_Set(LED_ON);
				//else
				//	Led2_Set(LED_OFF);
			
				oneNetInfo.sendData = SEND_TYPE_OK;		//标记数据反馈
				
			break;
			
			case KEY3DOWN:
				
				//if(ledStatus.Led7Sta == LED_OFF)
				//	Led2_Set(LED_ON);
			//	else
				//	Led2_Set(LED_OFF);
				
				oneNetInfo.sendData = SEND_TYPE_OK;		//标记数据反馈
				
			break;
				
			case KEY0DOUBLE:
			case KEY0DOWNLONG:
				
				if(spilcd_info.blSta)
					SPILCD_BL_Ctl(0);
				else
					SPILCD_BL_Ctl(250);
			
			break;
			
			case KEY2DOWNLONG:
				
				oneNetInfo.sendData = SEND_TYPE_PUBLISH;//Publish消息
			
			break;
			
			case KEY3DOWNLONG:
				
				oneNetInfo.sendData = SEND_TYPE_SUBSCRIBE;//Subscribe
			
			break;
			
			case KEY1DOWNLONG:
				
				oneNetInfo.sendData = SEND_TYPE_UNSUBSCRIBE;//UnSubscribe
			
			break;
			
			default:
			break;
		}
	
		RTOS_TimeDly(10); 								//挂起任务50ms
	
	}

}

/*
************************************************************
*	函数名称：	HEART_Task
*
*	函数功能：	心跳检测
*
*	入口参数：	void类型的参数指针
*
*	返回参数：	无
*
*	说明：		心跳任务。发送心跳请求并等待心跳响应，若在设定时间内没有响应则会进行平台重连
************************************************************
*/
void HEART_Task(void *pdata)
{

	while(1)
	{
		
		oneNetInfo.sendData = SEND_TYPE_HEART;
		
		RTOS_TimeDlyHMSM(0, 0, 20, 0);			//挂起任务20s
	
	}

}

/*
************************************************************
*	函数名称：	SEND_Task
*
*	函数功能：	上传传感器数据
*
*	入口参数：	void类型的参数指针
*
*	返回参数：	无
*
*	说明：		数据发送任务
************************************************************
*/
void SEND_Task(void *pdata)
{

	while(1)
	{
		
		oneNetInfo.sendData = SEND_TYPE_DATA;
		
		RTOS_TimeDlyHMSM(0, 0, 15, 0);			//挂起任务15s
		
	}

}

/*
************************************************************
*	函数名称：	USART_Task
*
*	函数功能：	处理平台下发的命令
*
*	入口参数：	void类型的参数指针
*
*	返回参数：	无
*
*	说明：		串口接收任务。在数据模式下时，等待平台下发的命令并解析、处理
************************************************************
*/
void USART_Task(void *pdata)
{

	while(1)
	{
		
		OneNET_CmdHandle();
		
		RTOS_TimeDly(1);		//挂起任务5ms
	
	}

}

/*
************************************************************
*	函数名称：	SENSOR_Task
*
*	函数功能：	传感器数据采集、显示
*
*	入口参数：	void类型的参数指针
*
*	返回参数：	无
*
*	说明：		传感器数据采集任务。进行外接传感器的数据采集、读取、显示
************************************************************
*/
void SENSOR_Task(void *pdata)
{
	
	unsigned char count = 0, send_data = 0;
	
#if(SPILCD_EN == 1)
	SPILCD_Clear(BGC);									//清屏
	
	//标题显示
	SPILCD_DisZW(0, 0, RED, san);						//显示“三”
	SPILCD_DisZW(16, 0, RED, zhou);						//显示“轴”
	
	SPILCD_DisZW(0, 32, RED, wen);						//显示“温”
	SPILCD_DisZW(16, 32, RED, shi);						//显示“湿”
	SPILCD_DisZW(32, 32, RED, du);						//显示“度”
	
	SPILCD_DisZW(96, 64, RED, guang);					//显示“光”
	SPILCD_DisZW(112, 64, RED, min);					//显示“敏”
	
	SPILCD_DisZW(0, 64, RED, zhuang);					//显示“状”
	SPILCD_DisZW(16, 64, RED, tai);						//显示“态”
#else
	Lcd1602_Clear(0xff);								//清屏
#endif

	while(1)
	{
		
		if(checkInfo.ADXL362_OK == DEV_OK) 									//只有设备存在时，才会读取值和显示
		{
			ADXL362_GetValue();												//采集传感器数据
				
#if(SPILCD_EN == 1)
			SPILCD_DisString(0, 16, 16, BLUE, 1, "X%0.1f,Y%0.1f,Z%0.1f    ", adxl362Info.x, adxl362Info.y, adxl362Info.z);
#else
			Lcd1602_DisString(0x80, "X%0.1f,Y%0.1f,Z%0.1f", adxl362Info.x, adxl362Info.y, adxl362Info.z);
#endif
		}
			
		if(checkInfo.SHT20_OK == DEV_OK) 									//只有设备存在时，才会读取值和显示
		{
			SHT20_GetValue();												//采集传感器数据
				
#if(SPILCD_EN == 1)
			SPILCD_DisString(0, 48, 16, BLUE, 1, "%0.1fC,%0.1f%%    ", sht20Info.tempreture, sht20Info.humidity);
#else
			Lcd1602_DisString(0xC0, "%0.1fC,%0.1f%%", sht20Info.tempreture, sht20Info.humidity);
#endif
		}
			
		LIGHT_GetVoltag();
			
#if(SPILCD_EN == 1)
		SPILCD_DisString(95, 80, 16, BLUE, 1, "%0.2f%", lightInfo.voltag);
#else
		Lcd1602_DisString(0xCC, "%0.2f%", lightInfo.voltag);
#endif
		
#if(SPILCD_EN == 1)
		SPILCD_BL_Ctl_Auto();									//自动调整SPILCD背光亮度
#endif
		
		if(++count >= 10)										//每隔一段时间发送一次红外数据
		{
			count = 0;
			
			NEC_SendData(0, send_data++);
		}
		
		RTOS_TimeDly(100); 										//挂起任务500ms
	
	}

}

/*
************************************************************
*	函数名称：	DATA_Task
*
*	函数功能：	平台下发命令的数据反馈
*
*	入口参数：	void类型的参数指针
*
*	返回参数：	无
*
*	说明：		数据反馈任务。这是平台下发指令后的反馈函数，透传模式的时候收到之后立即返回，非透传模式因为需要检索'>'符号，所以使用任务的方式来反馈。
************************************************************
*/
void DATA_Task(void *pdata)
{

	while(1)
	{
	
		switch(oneNetInfo.sendData)
		{
			case SEND_TYPE_DATA:
			
				oneNetInfo.sendData = OneNet_SendData(FORMAT_TYPE3, NULL, NULL, dataStream, dataStreamCnt);//上传数据到平台
			
			break;
			
			case SEND_TYPE_SUBSCRIBE:
			
				oneNetInfo.sendData = OneNet_Subscribe(topics, 2);											//订阅主题
			
			break;
			
			case SEND_TYPE_UNSUBSCRIBE:
			
				oneNetInfo.sendData = OneNet_UnSubscribe(topics, 2);										//取消订阅的主题
			
			break;
			
			case SEND_TYPE_PUBLISH:
			
				oneNetInfo.sendData = OneNet_Publish("pcTopic1", "New SDK Publish Test");					//发布主题
			
			break;
			
			case SEND_TYPE_HEART:
			
				oneNetInfo.sendData = OneNet_SendData_Heart();
			
			break;
		}
		
		OneNet_Check_Heart();
		
		RTOS_TimeDly(10);					//挂起任务50ms
	
	}

}

/*
************************************************************
*	函数名称：	DATALIST_Task
*
*	函数功能：	循环发送链表里边待发送的数据块
*
*	入口参数：	void类型的参数指针
*
*	返回参数：	无
*
*	说明：		
************************************************************
*/
void DATALIST_Task(void *pdata)
{

	while(1)
	{
		
		if(NET_DEVICE_CheckListHead())
		{
			NET_DEVICE_SendData(NET_DEVICE_GetListHeadBuf(), NET_DEVICE_GetListHeadLen());
			NET_DEVICE_DeleteDataSendList();
		}
	
		RTOS_TimeDly(100);					//挂起任务500ms
	
	}

}

/*
************************************************************
*	函数名称：	FAULT_Task
*
*	函数功能：	网络状态错误处理
*
*	入口参数：	void类型的参数指针
*
*	返回参数：	无
*
*	说明：		故障处理任务。当发生网络错误、设备错误时，会标记对应标志位，然后集中进行处理
************************************************************
*/
void FAULT_Task(void *pdata)
{

	while(1)
	{
		
		if(faultType != FAULT_NONE)									//如果错误标志被设置
		{
			if(faultType == FAULT_PRO || faultType == FAULT_NODEVICE)
			{
#if(SPILCD_EN == 1)
				SPILCD_DisZW(16, 80, BLUE, duan);SPILCD_DisZW(32, 80, BLUE, kai);
#endif
			}
			
			UsartPrintf(USART_DEBUG, "WARN:	Fault Process\r\n");
			Fault_Process();										//进入错误处理函数
		}
		
		RTOS_TimeDly(10);											//挂起任务50ms
	
	}

}

/*
************************************************************
*	函数名称：	NET_Task
*
*	函数功能：	网络连接、平台接入
*
*	入口参数：	void类型的参数指针
*
*	返回参数：	无
*
*	说明：		网络连接任务任务。会在心跳检测里边检测网络连接状态，如果有错，会标记状态，然后在这里进行重连
************************************************************
*/
void NET_Task(void *pdata)
{
	
	NET_DEVICE_IO_Init();													//网络设备IO初始化
	NET_DEVICE_Reset();														//网络设备复位
	
#if(SPILCD_EN == 1)
	SPILCD_DisZW(0, 80, BLUE, lian);SPILCD_DisZW(16, 80, BLUE, jie);SPILCD_DisZW(32, 80, BLUE, zhong);
#endif

	while(1)
	{
		
		if(!oneNetInfo.netWork && (checkInfo.NET_DEVICE_OK == DEV_OK))		//当没有网络 且 网络模块检测到时
		{
			if(!NET_DEVICE_Init(oneNetInfo.protocol, oneNetInfo.ip, oneNetInfo.port))//初始化网络设备，能连入网络
			{
				OneNet_DevLink(oneNetInfo.devID, oneNetInfo.proID, oneNetInfo.auif);		//接入平台
				
				if(oneNetInfo.netWork)
				{
					Beep_Set(BEEP_ON);										//短叫提示成功
					RTOS_TimeDly(40);
					Beep_Set(BEEP_OFF);
				
					if(gps.flag)
						dataStream[12].flag = 1;							//GPS就绪，准备上传
					
					oneNetInfo.sendData = SEND_TYPE_SUBSCRIBE;				//连接成功则订阅主题
							
#if(SPILCD_EN == 1)
					SPILCD_DisZW(0, 80, BLUE, yi);SPILCD_DisZW(16, 80, BLUE, lian);SPILCD_DisZW(32, 80, BLUE, jie);
#endif
				}
				else
				{
					Beep_Set(BEEP_ON);										//长叫提示失败
					RTOS_TimeDly(100);
					Beep_Set(BEEP_OFF);
					
#if(SPILCD_EN == 1)
					SPILCD_DisZW(0, 80, BLUE, wei);SPILCD_DisZW(16, 80, BLUE, lian);SPILCD_DisZW(32, 80, BLUE, jie);
#endif
				}
			}
		}
		
		if(checkInfo.NET_DEVICE_OK == DEV_ERR) 								//当网络设备未做检测
		{
			if(timerCount >= NET_TIME) 										//如果网络连接超时
			{
				NET_DEVICE_Reset();											//复位网络设备
				timerCount = 0;												//清零连接超时计数
				faultType = FAULT_NONE;										//清除错误标志
			}
			
			if(!NET_DEVICE_Exist())											//网络设备检测
			{
				UsartPrintf(USART_DEBUG, "NET Device :Ok\r\n");
				checkInfo.NET_DEVICE_OK = DEV_OK;							//检测到网络设备，标记
			}
			else
				UsartPrintf(USART_DEBUG, "NET Device :Error\r\n");
		}
		
		RTOS_TimeDly(5);													//挂起任务25ms
	
	}

}

/*
************************************************************
*	函数名称：	ALTER_Task
*
*	函数功能：	通过串口更改SSID、PSWD、DEVID、APIKEY
*
*	入口参数：	void类型的参数指针
*
*	返回参数：	无
*
*	说明：		更改后会保存到EEPROM里
************************************************************
*/
void ALTER_Task(void *pdata)
{

    while(1)
    {
    
        memset(alterInfo.alterBuf, 0, sizeof(alterInfo.alterBuf));
		
		while(!alterInfo.rev_idle)
            RTOS_TimeDly(20);														//每100ms检查一次
		
		alterInfo.rev_idle = 0;
		
        UsartPrintf(USART_DEBUG, "\r\nAlter Rev\r\n%s\r\n", alterInfo.alterBuf);
        
		if(checkInfo.EEPROM_OK == DEV_OK)											//如果EEPROM存在
		{
			if(Info_Alter(alterInfo.alterBuf))										//更改信息
			{
				oneNetInfo.netWork = 0;												//重连网络和平台
				NET_DEVICE_ReConfig(0);
			}
		}
    
    }

}

/*
************************************************************
*	函数名称：	CLOCK_Task
*
*	函数功能：	网络校时、时间显示
*
*	入口参数：	void类型的参数指针
*
*	返回参数：	无
*
*	说明：		
************************************************************
*/
void CLOCK_Task(void *pdata)
{
	
#if(NET_TIME_EN == 1)
	unsigned int second = 0, secondCmp = 0;									//second是实时时间，secondCmp是比较校准时间
	struct tm *time;
#endif

	while(1)
	{
	
#if(NET_TIME_EN == 1)
		if(second == 0)														//当second为0时
		{
			dataStream[11].flag = 0;										//不上传时间

			if(netDeviceInfo.net_time)
			{
				second = netDeviceInfo.net_time;
				
				RTC_SetTime(second + 4);									//设置RTC时间，加4是补上大概的时间差
				
				dataStream[11].flag = 1;									//上传时间
			}
		}
		else																//正常运行
		{
			secondCmp = second;
			second = RTC_GetCounter();										//获取秒值
			
			if(second > secondCmp)
			{
				time = localtime((const time_t *)&second);					//将秒值转为tm结构所表示的时间
				
				memset(myTime, 0, sizeof(myTime));
				snprintf(myTime, sizeof(myTime), "%d-%d-%d %d:%d:%d",
								time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
								time->tm_hour, time->tm_min, time->tm_sec);
#if(SPILCD_EN == 1)
				SPILCD_DisString(0, 115, 12, RED, 1, "%s     ", myTime);	//显示
#endif
			
				if(time->tm_hour == 0 && time->tm_min == 0 && time->tm_sec == 0)//每天0点时，更新一次时间
				{
					second = 0;
					netDeviceInfo.net_time = 0;
					oneNetInfo.netWork = 0;
					NET_DEVICE_ReConfig(0);
				}
			}
		}
#endif
		
		RTOS_TimeDly(20);													//挂起任务100ms
	
	}

}
