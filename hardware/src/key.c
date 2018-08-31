/**
	************************************************************
	************************************************************
	************************************************************
	*	�ļ����� 	key.c
	*
	*	���ߣ� 		�ż���
	*
	*	���ڣ� 		2016-11-23
	*
	*	�汾�� 		V1.0
	*
	*	˵���� 		����IO��ʼ�������������ж�
	*
	*	�޸ļ�¼��	
	************************************************************
	************************************************************
	************************************************************
**/

//����ͷ�ļ�
#include "key.h"

//Ӳ������
#include "delay.h"




/*
************************************************************
*	�������ƣ�	Key_Init
*
*	�������ܣ�	����IO��ʼ��
*
*	��ڲ�����	��
*
*	���ز�����	��
*
*	˵����		SW2-PD2		SW3-PC11	SW4-PC12	SW5-PC13
                SW0-PD2		SW1-PC11	SW2-PC12	SW3-PC13	
*				����Ϊ�͵�ƽ		�ͷ�Ϊ�ߵ�ƽ
************************************************************
*/
void Key_Init(void)
{

	GPIO_InitTypeDef GPIO_InitStructure;
 
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOE,ENABLE);//ʹ��PORTA,PORTEʱ��

	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4;//KEY0-KEY2
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //���ó���������
 	GPIO_Init(GPIOE, &GPIO_InitStructure);//��ʼ��GPIOE2,3,4

	//��ʼ�� WK_UP-->GPIOA.0	  ��������
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; //PA0���ó����룬Ĭ������	  
	GPIO_Init(GPIOE, &GPIO_InitStructure);//��ʼ��GPIOA.0

}

/*
************************************************************
*	�������ƣ�	KeyScan
*
*	�������ܣ�	������ƽɨ��
*
*	��ڲ�����	GPIOX����Ҫɨ���GPIO��	NUM����GPIO���ڵı��
*
*	���ز�����	IO��ƽ״̬
*
*	˵����		
************************************************************
*/
_Bool KeyScan(GPIO_TypeDef* GPIOX, unsigned int NUM)
{
	
	if(GPIOX == GPIOE)
	{
		if(!GPIO_ReadInputDataBit(GPIOE, NUM))	//����  Ϊ��
		{
			return KEYDOWN;
		}
		else									//����  Ϊ��
		{
			return KEYUP;
		}
	}
	else if(GPIOX == GPIOA)
	{
		if(!GPIO_ReadInputDataBit(GPIOA, NUM))	//����  Ϊ��
		{
			return KEYDOWN;
		}
		else									//����  Ϊ��
		{
			return KEYUP;
		}
	}
	
	return KEYUP;								//Ĭ�Ϸ��ذ����ͷ�
	
}

/*
************************************************************
*	�������ƣ�	Keyboard
*
*	�������ܣ�	�������ܼ��
*
*	��ڲ�����	GPIOX����Ҫɨ���GPIO��	NUM����GPIO���ڵı��
*
*	���ز�����	IO��ƽ״̬
*
*	˵����		�ֵ�����˫��������
************************************************************
*/
unsigned char Keyboard(void)
{
	
	static unsigned int keyBusyFlag = 0;									//�������ڷ��ͷ�״̬
	static unsigned char keyCount = 0;										//��������ʱ��
	unsigned char timeOut = 15;												//�ж�˫����������Ҫ����ʱ���
	
	if(KeyScan(GPIOE, KEY0) == KEYDOWN && !(keyBusyFlag & (~(1 << 0))))		//������� ����������δ����
	{
		keyBusyFlag |= 1 << 0;												//�˰�������æ״̬
		
		if(++keyCount >= KEYDOWN_LONG_TIME)									//���¼�ʱ
			keyCount = KEYDOWN_LONG_TIME;									//�ﵽ����ʱ���򲻱�
		
		return KEYNONE;														//�����޶���״̬
	}
	else if(KeyScan(GPIOE, KEY0) == KEYUP && keyBusyFlag & (1 << 0))		//����ͷ� �� ����֮ǰ�ǰ��¹���
	{
		keyBusyFlag &= ~(1 << 0);											//�˰������ڿ���״̬
		
		if(keyCount == KEYDOWN_LONG_TIME)									//����ǳ���
		{
			keyCount = 0;													//���¼�ʱ����
			return KEY0DOWNLONG;											//���س�������
		}
		else
		{
			keyCount = 0;													//���¼�ʱ����
			while(--timeOut)												//������Ҫ�ǵȴ�Լ250ms���ж��Ƿ��еڶ��ΰ���
			{
				RTOS_TimeDly(2);											//�ô������������̬���ⲻӰ���������������
				
				if(KeyScan(GPIOE, KEY0) == KEYDOWN)							//�еڶ��ΰ��£�˵��Ϊ˫��
				{
					while(KeyScan(GPIOE, KEY0) == KEYDOWN)					//�ȴ��ͷţ��޴˾䣬˫������һ����������
						RTOS_TimeDly(1);									//�ô������������̬���ⲻӰ���������������
					
					return KEY0DOUBLE;										//����˫������
				}
				
			}
			return KEY0DOWN;												//�����жϾ���Ч����Ϊ��������
		}
	}
	/********************************************��ͬ**********************************************/
	if(KeyScan(GPIOE, KEY1) == KEYDOWN && !(keyBusyFlag & (~(1 << 1))))		//������� ����������δ����
	{
		keyBusyFlag |= 1 << 1;												//�˰�������æ״̬
		
		if(++keyCount >= KEYDOWN_LONG_TIME)
			keyCount = KEYDOWN_LONG_TIME;
		
		return KEYNONE;
	}
	else if(KeyScan(GPIOE, KEY1) == KEYUP && keyBusyFlag & (1 << 1))		//����ͷ�
	{
		keyBusyFlag &= ~(1 << 1);											//�˰������ڿ���״̬
		
		if(keyCount == KEYDOWN_LONG_TIME)
		{
			keyCount = 0;
			return KEY1DOWNLONG;
		}
		else
		{
			keyCount = 0;
			while(--timeOut)
			{
				RTOS_TimeDly(2);
				
				if(KeyScan(GPIOE, KEY1) == KEYDOWN)
				{
					while(KeyScan(GPIOE, KEY1) == KEYDOWN)
						RTOS_TimeDly(1);
					
					return KEY1DOUBLE;
				}
				
			}
			return KEY1DOWN;
		}
	}
	
	if(KeyScan(GPIOE, KEY2) == KEYDOWN && !(keyBusyFlag & (~(1 << 2))))		//������� ����������δ����
	{
		keyBusyFlag |= 1 << 2;												//�˰�������æ״̬
		
		if(++keyCount >= KEYDOWN_LONG_TIME)
			keyCount = KEYDOWN_LONG_TIME;
		
		return KEYNONE;
	}
	else if(KeyScan(GPIOE, KEY2) == KEYUP && keyBusyFlag & (1 << 2))		//����ͷ�
	{
		keyBusyFlag &= ~(1 << 2);											//�˰������ڿ���״̬
		
		if(keyCount == KEYDOWN_LONG_TIME)
		{
			keyCount = 0;
			return KEY2DOWNLONG;
		}
		else
		{
			keyCount = 0;
			while(--timeOut)
			{
				RTOS_TimeDly(2);
				
				if(KeyScan(GPIOE, KEY2) == KEYDOWN)
				{
					while(KeyScan(GPIOE, KEY2) == KEYDOWN)
						RTOS_TimeDly(1);
					
					return KEY2DOUBLE;
				}
				
			}
			return KEY2DOWN;
		}
	}
	
	if(KeyScan(GPIOA, KEY3) == KEYDOWN && !(keyBusyFlag & (~(1 << 3))))		//������� ����������δ����
	{
		keyBusyFlag |= 1 << 3;												//�˰�������æ״̬
		
		if(++keyCount >= KEYDOWN_LONG_TIME)
			keyCount = KEYDOWN_LONG_TIME;
		
		return KEYNONE;
	}
	else if(KeyScan(GPIOA, KEY3) == KEYUP && keyBusyFlag & (1 << 3))		//����ͷ�
	{
		keyBusyFlag &= ~(1 << 3);											//�˰������ڿ���״̬
		
		if(keyCount == KEYDOWN_LONG_TIME)
		{
			keyCount = 0;
			return KEY3DOWNLONG;
		}
		else
		{
			keyCount = 0;
			while(--timeOut)
			{
				RTOS_TimeDly(2);
				
				if(KeyScan(GPIOA, KEY3) == KEYDOWN)
				{
					while(KeyScan(GPIOA, KEY3) == KEYDOWN)
						RTOS_TimeDly(1);
					
					return KEY3DOUBLE;
				}
				
			}
			return KEY3DOWN;
		}
	}
	
	keyBusyFlag = 0;
	keyCount = 0;
	return KEYNONE;
	
}
