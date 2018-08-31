/**
	************************************************************
	************************************************************
	************************************************************
	*	�ļ����� 	main.c
	*
	*	���ߣ� 		�ż���
	*
	*	���ڣ� 		2016-11-23
	*
	*	�汾�� 		V1.0
	*
	*	˵���� 		��ɵ�Ƭ����ʼ�������IC��ʼ��������Ĵ���������
	*
	*	�޸ļ�¼��	
	************************************************************
	************************************************************
	************************************************************
**/

//��Ƭ��ͷ�ļ�
#include "stm32f10x.h"

//OS
#include "includes.h"

//Э��
#include "onenet.h"
#include "fault.h"

//�����豸
#include "net_device.h"
#include "net_io.h"

//Ӳ������
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

//����������
#include "dataStreamName.h"

//�ֿ�
#include "font.h"

//C��
#include <string.h>
#include <time.h>
#include <stdio.h>


#define SPILCD_EN		1		//1-ʹ��SPILCD		0-ʹ��LCD1602


//���Ź�����
#define IWDG_TASK_PRIO		6
#define IWDG_STK_SIZE		64
OS_STK IWDG_TASK_STK[IWDG_STK_SIZE];
void IWDG_Task(void *pdata);

//��������
#define USART_TASK_PRIO		7
#define USART_STK_SIZE		256
OS_STK USART_TASK_STK[USART_STK_SIZE]; //
void USART_Task(void *pdata);

//��������
#define HEART_TASK_PRIO		8
#define HEART_STK_SIZE		64
OS_STK HEART_TASK_STK[HEART_STK_SIZE]; //
void HEART_Task(void *pdata);

//���ϴ�������
#define FAULT_TASK_PRIO		9 //
#define FAULT_STK_SIZE		256
OS_STK FAULT_TASK_STK[FAULT_STK_SIZE]; //
void FAULT_Task(void *pdata);

//����������
#define SENSOR_TASK_PRIO	10
#define SENSOR_STK_SIZE		256
__align(8) OS_STK SENSOR_TASK_STK[SENSOR_STK_SIZE]; //UCOSʹ�ø���������printf��sprintfʱһ��Ҫ8�ֽڶ���
void SENSOR_Task(void *pdata);

//���ݷ�������
#define SEND_TASK_PRIO		11
#define SEND_STK_SIZE		64
OS_STK SEND_TASK_STK[SEND_STK_SIZE];
void SEND_Task(void *pdata);

//��������
#define KEY_TASK_PRIO		12
#define KEY_STK_SIZE		256
OS_STK KEY_TASK_STK[KEY_STK_SIZE]; //UCOSʹ�ø���������printf��sprintfʱһ��Ҫ8�ֽڶ���
void KEY_Task(void *pdata);

//�����ʼ������
#define NET_TASK_PRIO		13 //
#define NET_STK_SIZE		512
OS_STK NET_TASK_STK[NET_STK_SIZE]; //
void NET_Task(void *pdata);

//���ݷ�������
#define DATA_TASK_PRIO		14 //
#define DATA_STK_SIZE		512
__align(8) OS_STK DATA_TASK_STK[DATA_STK_SIZE]; //UCOSʹ�ø���������printf��sprintfʱһ��Ҫ8�ֽڶ���
void DATA_Task(void *pdata);

//���ݷ�������
#define DATALIST_TASK_PRIO		15 //
#define DATALIST_STK_SIZE		128
OS_STK DATALIST_TASK_STK[DATALIST_STK_SIZE];
void DATALIST_Task(void *pdata);

//��Ϣ��������
#define ALTER_TASK_PRIO		16 //
#define ALTER_STK_SIZE		256
OS_STK ALTER_TASK_STK[ALTER_STK_SIZE]; //
void ALTER_Task(void *pdata);

//ʱ������
#define CLOCK_TASK_PRIO		17 //
#define CLOCK_STK_SIZE		256
OS_STK CLOCK_TASK_STK[CLOCK_STK_SIZE]; //
void CLOCK_Task(void *pdata);


#define NET_TIME	60			//�趨ʱ��--��λ��


unsigned short timerCount = 0;	//ʱ�����--��λ��


OS_TMR *tmr;					//�����ʱ�����


char myTime[24];


//������
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
*	�������ƣ�	Hardware_Init
*
*	�������ܣ�	Ӳ����ʼ��
*
*	��ڲ�����	��
*
*	���ز�����	��
*
*	˵����		��ʼ����Ƭ�������Լ�����豸
************************************************************
*/
void Hardware_Init(void)
{
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);								//�жϿ�������������

	Delay_Init();																//Timer4��ʼ��
	
	Led_Init();																	//LED��ʼ��
	
	Key_Init();																	//������ʼ��
	
	Beep_Init();																//��������ʼ��
	
	LIGHT_Init();																//���������ʼ��
	
	IR_Init(38000);																//���ⷢ��ܳ�ʼ��
	
	IIC_Init();																	//���IIC���߳�ʼ��
	
#if(SPILCD_EN == 1)
	SPILCD_Init();																//SPILCD��ʼ��
#else
	Lcd1602_Init();																//LCD1602��ʼ��
#endif
	
	Usart1_Init(115200); 														//��ʼ������   115200bps
#if(USART_DMA_RX_EN)
	USARTx_ResetMemoryBaseAddr(USART_DEBUG, (unsigned int)alterInfo.alterBuf, sizeof(alterInfo.alterBuf), USART_RX_TYPE);
#endif
	
	RTC_Init();																	//��ʼ��RTC
	
	Check_PowerOn(); 															//�ϵ��Լ�

	if(RCC_GetFlagStatus(RCC_FLAG_IWDGRST) == SET) 								//����ǿ��Ź���λ����ʾ
	{
		UsartPrintf(USART_DEBUG, "WARN:	IWDG Reboot\r\n");
		
		RCC_ClearFlag();														//������Ź���λ��־λ
		
		faultTypeReport = faultType = FAULT_REBOOT; 							//���Ϊ��������
		
		netDeviceInfo.reboot = 1;
		
		if(!Info_Check() && checkInfo.EEPROM_OK)								//���EEPROM������Ϣ
			Info_Read();
	}
	else
	{
		//�ȶ���ssid��pswd��devid��apikey
		if(!Info_Check() && checkInfo.EEPROM_OK)								//���EEPROM������Ϣ
		{
			//AT24C02_Clear(0, 255, 256);Iwdg_Feed();
			UsartPrintf(USART_DEBUG, "1.ssid_pswd in EEPROM\r\n");
			Info_Read();
		}
		else //û������
		{
			UsartPrintf(USART_DEBUG, "1.ssid_pswd in ROM\r\n");
		}
		
		UsartPrintf(USART_DEBUG, "2.DEVID: %s,     APIKEY: %s\r\n"
								, oneNetInfo.devID, oneNetInfo.apiKey);
		
		netDeviceInfo.reboot = 0;
	}
	
	Iwdg_Init(4, 1250); 														//64��Ƶ��ÿ��625�Σ�����1250�Σ�2s
	
	RTOS_TimerInit();
	
	UsartPrintf(USART_DEBUG, "3.Hardware init OK\r\n");							//��ʾ��ʼ�����

}

/*
************************************************************
*	�������ƣ�	OS_TimerCallBack
*
*	�������ܣ�	��ʱ�������״̬��־λ
*
*	��ڲ�����	ptmr�������ʱ�����
*				p_arg�������ʱ���ص���������
*
*	���ز�����	��
*
*	˵����		��ʱ�����񡣶�ʱ�������״̬�������������趨ʱ�����������ӣ������ƽ̨����
************************************************************
*/
void OS_TimerCallBack(OS_TMR *ptmr, void *p_arg)
{
	
	if(oneNetInfo.netWork == 0)											//�������Ͽ�
	{
		if(++timerCount >= NET_TIME) 									//�������Ͽ���ʱ
		{
			UsartPrintf(USART_DEBUG, "Tips:		Timer Check Err\r\n");
			
			checkInfo.NET_DEVICE_OK = 0;								//���豸δ����־
			
			NET_DEVICE_ReConfig(0);										//�豸��ʼ����������Ϊ��ʼ״̬
			
			oneNetInfo.netWork = 0;
		}
	}
	else
	{
		timerCount = 0;													//�������
	}

}

/*
************************************************************
*	�������ƣ�	main
*
*	�������ܣ�	��ɳ�ʼ�����񣬴���Ӧ������ִ��
*
*	��ڲ�����	��
*
*	���ز�����	0
*
*	˵����		
************************************************************
*/
int main(void)
{
	
	unsigned char err;
	
	Hardware_Init();								//Ӳ����ʼ��

	OSInit();										//RTOS��ʼ��
	
	//����Ӧ������
	
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
	
	UsartPrintf(USART_DEBUG, "4.OSStart\r\n");		//��ʾ����ʼִ��
	
	OSStart();										//��ʼִ������
	
	return 0;

}

/*
************************************************************
*	�������ƣ�	IWDG_Task
*
*	�������ܣ�	������Ź�
*
*	��ڲ�����	void���͵Ĳ���ָ��
*
*	���ز�����	��
*
*	˵����		���Ź�����
************************************************************
*/
void IWDG_Task(void *pdata)
{

	while(1)
	{
	
		Iwdg_Feed(); 		//ι��
		
		RTOS_TimeDly(50); 	//��������250ms
	
	}

}

/*
************************************************************
*	�������ƣ�	KEY_Task
*
*	�������ܣ�	ɨ�谴���Ƿ��£�����а��£����ж�Ӧ�Ĵ���
*
*	��ڲ�����	void���͵Ĳ���ָ��
*
*	���ز�����	��
*
*	˵����		��������
************************************************************
*/
void KEY_Task(void *pdata)
{

	while(1)
	{
		
		switch(Keyboard())								//ɨ�谴��״̬
		{
			case KEY0DOWN:								//�����key0�����¼�
				
				if(led_status.Led2Sta == LED_OFF)
					Led4_Set(LED_ON);
				else
					Led4_Set(LED_OFF);
				
				oneNetInfo.sendData = SEND_TYPE_OK;		//������ݷ���
			
			break;
			
			case KEY1DOWN:
				
				//if(ledStatus.Led5Sta == LED_OFF)
			//		Led5_Set(LED_ON);
			//	else
				//	Led5_Set(LED_OFF);
			
				oneNetInfo.sendData = SEND_TYPE_OK;		//������ݷ���
				
			break;
			
			case KEY2DOWN:
				
				//if(ledStatus.Led6Sta == LED_OFF)
				//	Led2_Set(LED_ON);
				//else
				//	Led2_Set(LED_OFF);
			
				oneNetInfo.sendData = SEND_TYPE_OK;		//������ݷ���
				
			break;
			
			case KEY3DOWN:
				
				//if(ledStatus.Led7Sta == LED_OFF)
				//	Led2_Set(LED_ON);
			//	else
				//	Led2_Set(LED_OFF);
				
				oneNetInfo.sendData = SEND_TYPE_OK;		//������ݷ���
				
			break;
				
			case KEY0DOUBLE:
			case KEY0DOWNLONG:
				
				if(spilcd_info.blSta)
					SPILCD_BL_Ctl(0);
				else
					SPILCD_BL_Ctl(250);
			
			break;
			
			case KEY2DOWNLONG:
				
				oneNetInfo.sendData = SEND_TYPE_PUBLISH;//Publish��Ϣ
			
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
	
		RTOS_TimeDly(10); 								//��������50ms
	
	}

}

/*
************************************************************
*	�������ƣ�	HEART_Task
*
*	�������ܣ�	�������
*
*	��ڲ�����	void���͵Ĳ���ָ��
*
*	���ز�����	��
*
*	˵����		�������񡣷����������󲢵ȴ�������Ӧ�������趨ʱ����û����Ӧ������ƽ̨����
************************************************************
*/
void HEART_Task(void *pdata)
{

	while(1)
	{
		
		oneNetInfo.sendData = SEND_TYPE_HEART;
		
		RTOS_TimeDlyHMSM(0, 0, 20, 0);			//��������20s
	
	}

}

/*
************************************************************
*	�������ƣ�	SEND_Task
*
*	�������ܣ�	�ϴ�����������
*
*	��ڲ�����	void���͵Ĳ���ָ��
*
*	���ز�����	��
*
*	˵����		���ݷ�������
************************************************************
*/
void SEND_Task(void *pdata)
{

	while(1)
	{
		
		oneNetInfo.sendData = SEND_TYPE_DATA;
		
		RTOS_TimeDlyHMSM(0, 0, 15, 0);			//��������15s
		
	}

}

/*
************************************************************
*	�������ƣ�	USART_Task
*
*	�������ܣ�	����ƽ̨�·�������
*
*	��ڲ�����	void���͵Ĳ���ָ��
*
*	���ز�����	��
*
*	˵����		���ڽ�������������ģʽ��ʱ���ȴ�ƽ̨�·����������������
************************************************************
*/
void USART_Task(void *pdata)
{

	while(1)
	{
		
		OneNET_CmdHandle();
		
		RTOS_TimeDly(1);		//��������5ms
	
	}

}

/*
************************************************************
*	�������ƣ�	SENSOR_Task
*
*	�������ܣ�	���������ݲɼ�����ʾ
*
*	��ڲ�����	void���͵Ĳ���ָ��
*
*	���ز�����	��
*
*	˵����		���������ݲɼ����񡣽�����Ӵ����������ݲɼ�����ȡ����ʾ
************************************************************
*/
void SENSOR_Task(void *pdata)
{
	
	unsigned char count = 0, send_data = 0;
	
#if(SPILCD_EN == 1)
	SPILCD_Clear(BGC);									//����
	
	//������ʾ
	SPILCD_DisZW(0, 0, RED, san);						//��ʾ������
	SPILCD_DisZW(16, 0, RED, zhou);						//��ʾ���ᡱ
	
	SPILCD_DisZW(0, 32, RED, wen);						//��ʾ���¡�
	SPILCD_DisZW(16, 32, RED, shi);						//��ʾ��ʪ��
	SPILCD_DisZW(32, 32, RED, du);						//��ʾ���ȡ�
	
	SPILCD_DisZW(96, 64, RED, guang);					//��ʾ���⡱
	SPILCD_DisZW(112, 64, RED, min);					//��ʾ������
	
	SPILCD_DisZW(0, 64, RED, zhuang);					//��ʾ��״��
	SPILCD_DisZW(16, 64, RED, tai);						//��ʾ��̬��
#else
	Lcd1602_Clear(0xff);								//����
#endif

	while(1)
	{
		
		if(checkInfo.ADXL362_OK == DEV_OK) 									//ֻ���豸����ʱ���Ż��ȡֵ����ʾ
		{
			ADXL362_GetValue();												//�ɼ�����������
				
#if(SPILCD_EN == 1)
			SPILCD_DisString(0, 16, 16, BLUE, 1, "X%0.1f,Y%0.1f,Z%0.1f    ", adxl362Info.x, adxl362Info.y, adxl362Info.z);
#else
			Lcd1602_DisString(0x80, "X%0.1f,Y%0.1f,Z%0.1f", adxl362Info.x, adxl362Info.y, adxl362Info.z);
#endif
		}
			
		if(checkInfo.SHT20_OK == DEV_OK) 									//ֻ���豸����ʱ���Ż��ȡֵ����ʾ
		{
			SHT20_GetValue();												//�ɼ�����������
				
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
		SPILCD_BL_Ctl_Auto();									//�Զ�����SPILCD��������
#endif
		
		if(++count >= 10)										//ÿ��һ��ʱ�䷢��һ�κ�������
		{
			count = 0;
			
			NEC_SendData(0, send_data++);
		}
		
		RTOS_TimeDly(100); 										//��������500ms
	
	}

}

/*
************************************************************
*	�������ƣ�	DATA_Task
*
*	�������ܣ�	ƽ̨�·���������ݷ���
*
*	��ڲ�����	void���͵Ĳ���ָ��
*
*	���ز�����	��
*
*	˵����		���ݷ�����������ƽ̨�·�ָ���ķ���������͸��ģʽ��ʱ���յ�֮���������أ���͸��ģʽ��Ϊ��Ҫ����'>'���ţ�����ʹ������ķ�ʽ��������
************************************************************
*/
void DATA_Task(void *pdata)
{

	while(1)
	{
	
		switch(oneNetInfo.sendData)
		{
			case SEND_TYPE_DATA:
			
				oneNetInfo.sendData = OneNet_SendData(FORMAT_TYPE3, NULL, NULL, dataStream, dataStreamCnt);//�ϴ����ݵ�ƽ̨
			
			break;
			
			case SEND_TYPE_SUBSCRIBE:
			
				oneNetInfo.sendData = OneNet_Subscribe(topics, 2);											//��������
			
			break;
			
			case SEND_TYPE_UNSUBSCRIBE:
			
				oneNetInfo.sendData = OneNet_UnSubscribe(topics, 2);										//ȡ�����ĵ�����
			
			break;
			
			case SEND_TYPE_PUBLISH:
			
				oneNetInfo.sendData = OneNet_Publish("pcTopic1", "New SDK Publish Test");					//��������
			
			break;
			
			case SEND_TYPE_HEART:
			
				oneNetInfo.sendData = OneNet_SendData_Heart();
			
			break;
		}
		
		OneNet_Check_Heart();
		
		RTOS_TimeDly(10);					//��������50ms
	
	}

}

/*
************************************************************
*	�������ƣ�	DATALIST_Task
*
*	�������ܣ�	ѭ������������ߴ����͵����ݿ�
*
*	��ڲ�����	void���͵Ĳ���ָ��
*
*	���ز�����	��
*
*	˵����		
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
	
		RTOS_TimeDly(100);					//��������500ms
	
	}

}

/*
************************************************************
*	�������ƣ�	FAULT_Task
*
*	�������ܣ�	����״̬������
*
*	��ڲ�����	void���͵Ĳ���ָ��
*
*	���ز�����	��
*
*	˵����		���ϴ������񡣵�������������豸����ʱ�����Ƕ�Ӧ��־λ��Ȼ���н��д���
************************************************************
*/
void FAULT_Task(void *pdata)
{

	while(1)
	{
		
		if(faultType != FAULT_NONE)									//��������־������
		{
			if(faultType == FAULT_PRO || faultType == FAULT_NODEVICE)
			{
#if(SPILCD_EN == 1)
				SPILCD_DisZW(16, 80, BLUE, duan);SPILCD_DisZW(32, 80, BLUE, kai);
#endif
			}
			
			UsartPrintf(USART_DEBUG, "WARN:	Fault Process\r\n");
			Fault_Process();										//�����������
		}
		
		RTOS_TimeDly(10);											//��������50ms
	
	}

}

/*
************************************************************
*	�������ƣ�	NET_Task
*
*	�������ܣ�	�������ӡ�ƽ̨����
*
*	��ڲ�����	void���͵Ĳ���ָ��
*
*	���ز�����	��
*
*	˵����		���������������񡣻������������߼����������״̬������д�����״̬��Ȼ���������������
************************************************************
*/
void NET_Task(void *pdata)
{
	
	NET_DEVICE_IO_Init();													//�����豸IO��ʼ��
	NET_DEVICE_Reset();														//�����豸��λ
	
#if(SPILCD_EN == 1)
	SPILCD_DisZW(0, 80, BLUE, lian);SPILCD_DisZW(16, 80, BLUE, jie);SPILCD_DisZW(32, 80, BLUE, zhong);
#endif

	while(1)
	{
		
		if(!oneNetInfo.netWork && (checkInfo.NET_DEVICE_OK == DEV_OK))		//��û������ �� ����ģ���⵽ʱ
		{
			if(!NET_DEVICE_Init(oneNetInfo.protocol, oneNetInfo.ip, oneNetInfo.port))//��ʼ�������豸������������
			{
				OneNet_DevLink(oneNetInfo.devID, oneNetInfo.proID, oneNetInfo.auif);		//����ƽ̨
				
				if(oneNetInfo.netWork)
				{
					Beep_Set(BEEP_ON);										//�̽���ʾ�ɹ�
					RTOS_TimeDly(40);
					Beep_Set(BEEP_OFF);
				
					if(gps.flag)
						dataStream[12].flag = 1;							//GPS������׼���ϴ�
					
					oneNetInfo.sendData = SEND_TYPE_SUBSCRIBE;				//���ӳɹ���������
							
#if(SPILCD_EN == 1)
					SPILCD_DisZW(0, 80, BLUE, yi);SPILCD_DisZW(16, 80, BLUE, lian);SPILCD_DisZW(32, 80, BLUE, jie);
#endif
				}
				else
				{
					Beep_Set(BEEP_ON);										//������ʾʧ��
					RTOS_TimeDly(100);
					Beep_Set(BEEP_OFF);
					
#if(SPILCD_EN == 1)
					SPILCD_DisZW(0, 80, BLUE, wei);SPILCD_DisZW(16, 80, BLUE, lian);SPILCD_DisZW(32, 80, BLUE, jie);
#endif
				}
			}
		}
		
		if(checkInfo.NET_DEVICE_OK == DEV_ERR) 								//�������豸δ�����
		{
			if(timerCount >= NET_TIME) 										//����������ӳ�ʱ
			{
				NET_DEVICE_Reset();											//��λ�����豸
				timerCount = 0;												//�������ӳ�ʱ����
				faultType = FAULT_NONE;										//��������־
			}
			
			if(!NET_DEVICE_Exist())											//�����豸���
			{
				UsartPrintf(USART_DEBUG, "NET Device :Ok\r\n");
				checkInfo.NET_DEVICE_OK = DEV_OK;							//��⵽�����豸�����
			}
			else
				UsartPrintf(USART_DEBUG, "NET Device :Error\r\n");
		}
		
		RTOS_TimeDly(5);													//��������25ms
	
	}

}

/*
************************************************************
*	�������ƣ�	ALTER_Task
*
*	�������ܣ�	ͨ�����ڸ���SSID��PSWD��DEVID��APIKEY
*
*	��ڲ�����	void���͵Ĳ���ָ��
*
*	���ز�����	��
*
*	˵����		���ĺ�ᱣ�浽EEPROM��
************************************************************
*/
void ALTER_Task(void *pdata)
{

    while(1)
    {
    
        memset(alterInfo.alterBuf, 0, sizeof(alterInfo.alterBuf));
		
		while(!alterInfo.rev_idle)
            RTOS_TimeDly(20);														//ÿ100ms���һ��
		
		alterInfo.rev_idle = 0;
		
        UsartPrintf(USART_DEBUG, "\r\nAlter Rev\r\n%s\r\n", alterInfo.alterBuf);
        
		if(checkInfo.EEPROM_OK == DEV_OK)											//���EEPROM����
		{
			if(Info_Alter(alterInfo.alterBuf))										//������Ϣ
			{
				oneNetInfo.netWork = 0;												//���������ƽ̨
				NET_DEVICE_ReConfig(0);
			}
		}
    
    }

}

/*
************************************************************
*	�������ƣ�	CLOCK_Task
*
*	�������ܣ�	����Уʱ��ʱ����ʾ
*
*	��ڲ�����	void���͵Ĳ���ָ��
*
*	���ز�����	��
*
*	˵����		
************************************************************
*/
void CLOCK_Task(void *pdata)
{
	
#if(NET_TIME_EN == 1)
	unsigned int second = 0, secondCmp = 0;									//second��ʵʱʱ�䣬secondCmp�ǱȽ�У׼ʱ��
	struct tm *time;
#endif

	while(1)
	{
	
#if(NET_TIME_EN == 1)
		if(second == 0)														//��secondΪ0ʱ
		{
			dataStream[11].flag = 0;										//���ϴ�ʱ��

			if(netDeviceInfo.net_time)
			{
				second = netDeviceInfo.net_time;
				
				RTC_SetTime(second + 4);									//����RTCʱ�䣬��4�ǲ��ϴ�ŵ�ʱ���
				
				dataStream[11].flag = 1;									//�ϴ�ʱ��
			}
		}
		else																//��������
		{
			secondCmp = second;
			second = RTC_GetCounter();										//��ȡ��ֵ
			
			if(second > secondCmp)
			{
				time = localtime((const time_t *)&second);					//����ֵתΪtm�ṹ����ʾ��ʱ��
				
				memset(myTime, 0, sizeof(myTime));
				snprintf(myTime, sizeof(myTime), "%d-%d-%d %d:%d:%d",
								time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
								time->tm_hour, time->tm_min, time->tm_sec);
#if(SPILCD_EN == 1)
				SPILCD_DisString(0, 115, 12, RED, 1, "%s     ", myTime);	//��ʾ
#endif
			
				if(time->tm_hour == 0 && time->tm_min == 0 && time->tm_sec == 0)//ÿ��0��ʱ������һ��ʱ��
				{
					second = 0;
					netDeviceInfo.net_time = 0;
					oneNetInfo.netWork = 0;
					NET_DEVICE_ReConfig(0);
				}
			}
		}
#endif
		
		RTOS_TimeDly(20);													//��������100ms
	
	}

}
