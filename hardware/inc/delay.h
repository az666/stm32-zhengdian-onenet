#ifndef _DELAY_H_
#define _DELAY_H_


#include "includes.h"




#define RTOS_TimeDly(ticks) 						OSTimeDly(ticks)
#define RTOS_TimeDlyHMSM(hour, min, sec, ms)		OSTimeDlyHMSM(hour, min, sec, ms)

#define RTOS_EnterInt()								OSIntEnter()
#define RTOS_ExitInt()								OSIntExit()

#define RTOS_ENTER_CRITICAL()						{int cpu_sr;cpu_sr=cpu_sr;OS_ENTER_CRITICAL();}
#define RTOS_EXIT_CRITICAL()						{int cpu_sr;cpu_sr=cpu_sr;OS_EXIT_CRITICAL();}




void Delay_Init(void);

void DelayUs(unsigned short us);

void DelayXms(unsigned short ms);

#endif
