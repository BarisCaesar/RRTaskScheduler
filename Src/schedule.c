

#include<stdint.h>
#include "schedule.h"
#include "led.h"

uint8_t current_task = 1;//start is task1
uint32_t g_tick_count = 0;

typedef struct
{
	uint32_t psp_value;
	uint32_t block_count;
	uint8_t current_state;
	void (*task_handler)(void);
} TCB_t; //Task Control Block

TCB_t user_tasks[MAX_TASKS];

void init_systick_timer(uint32_t tick_hz){
	uint32_t *pSRVR = (uint32_t*)0xE000E014;
	uint32_t *pSCVR = (uint32_t*)0xE000E018;
	uint32_t *pSCSR = (uint32_t*)0xE000E010;
	uint32_t count_value = (SYSTICK_TIM_CLK/tick_hz)-1;

	//clear the value into SVR
	*pSRVR &= ~(0x00FFFFFF);

	//load the value into SVR
	*pSRVR |= count_value;

	//clear CVR
	*pSCVR &= ~(0x00FFFFFF);

	//settings for CSR
	*pSCSR |= (1 << 1);//Enables SysTick exception request
	*pSCSR |= (1 << 2);//Enables the counter

	//enable the systick
	*pSCSR |= (1 << 0);//Indicates the clock source(1 for processor clock)


}

__attribute__ ((naked)) void init_scheduler_stack(uint32_t sched_top_of_stack)
{
	__asm volatile("MSR MSP,%0"::"r"(sched_top_of_stack));
	__asm volatile("BX LR");
}

void init_tasks_stack(void)
{
	//initially all tasks are ready to run
	for(int i = 0; i < MAX_TASKS; i++){
		user_tasks[i].current_state = TASK_READY_STATE;
	}
	//initialize psp's of the tasks with correct values
	user_tasks[0].psp_value = IDLE_STACK_START;
	user_tasks[1].psp_value = T1_STACK_START;
	user_tasks[2].psp_value = T2_STACK_START;
	user_tasks[3].psp_value = T3_STACK_START;
	user_tasks[4].psp_value = T4_STACK_START;

	//initialize the tasks with appropriate handlers
	user_tasks[0].task_handler = idle_task;
	user_tasks[1].task_handler = task1_handler;
	user_tasks[2].task_handler = task2_handler;
	user_tasks[3].task_handler = task3_handler;
	user_tasks[4].task_handler = task4_handler;

	uint32_t *pPSP;
	for(int i = 0; i < MAX_TASKS;i++){
		/*load xpsr, pc, lr and register values into the stack of tasks.
		 * Decrement after loading each value*/
		pPSP = (uint32_t*) user_tasks[i].psp_value;
		pPSP--;
		*pPSP = DUMMY_XPSR;//0x01000000

		pPSP--;//PC
		*pPSP = (uint32_t)user_tasks[i].task_handler;

		pPSP--;//LR
		*pPSP = 0xFFFFFFFD;

		for(int j = 0; j < 13; j++){
			pPSP--;//R0-R12
			*pPSP = 0;
		}
		user_tasks[i].psp_value = (uint32_t)pPSP;//preserve the value of psp
	}

}

void enable_processor_faults(void)
{
	//1. enable all configurable exceptions : usage fault, mem manage fault and bus fault

		uint32_t *pSHCSR = (uint32_t*)0xE000ED24;

		*pSHCSR |= ( 1 << 16); //mem manage
		*pSHCSR |= ( 1 << 17); //bus fault
		*pSHCSR |= ( 1 << 18); //usage fault

}
void update_global_tick_count()
{
	g_tick_count++;
}


void unblock_tasks()
{
	//Unblock the blocked task if the global tick count reached the task's block count.
	for(int i = 1; i < MAX_TASKS; i++){
		if(user_tasks[i].current_state != TASK_READY_STATE){
			if(user_tasks[i].block_count == g_tick_count){
				user_tasks[i].current_state = TASK_READY_STATE;
			}

		}
	}
}

void schedule(void)
{
	uint32_t *pICSR =(uint32_t*)0xE000ED04;
	//pend the pendSV exception

	*pICSR |= (1 << 28);

}


void update_next_task(void)
{
	//check if any task is ready to run
	int state = TASK_BLOCKED_STATE;

	for(int i = 0; i < MAX_TASKS; i++)
	{
		current_task++;
		current_task %= MAX_TASKS;
		state = user_tasks[current_task].current_state;
		if((state == TASK_READY_STATE) && (current_task != 0)){
			break;
		}
	}

	//If no task is available execute, the idle task
	if(state != TASK_READY_STATE){
		current_task = 0;
	}
}

uint32_t get_psp_value(void)
{
	return user_tasks[current_task].psp_value;
}

void save_psp_value(uint32_t current_stack_addr)
{
	user_tasks[current_task].psp_value = current_stack_addr;
}



__attribute__((naked))void switch_sp_to_psp(void)
{
	//1. initialize the PSP with TASK1 stack start address

	//get the value of psp of current_task
	__asm volatile("PUSH {LR}");//SAVE LR(will get corrupted because of BL)
	__asm volatile("BL get_psp_value");//Branch with link so you can comeback to this function
	__asm volatile("MSR PSP,R0");//return value of get_psp_value stored in R0
	__asm volatile("POP {LR}");//Retrieve LR

	//2. change SP to PSP using CONTROL register
	__asm volatile("MOV R0, #0x02");
	__asm volatile("MSR CONTROL,R0");
	__asm volatile("BX LR");//go back to main
}

void task_delay(uint32_t tick_count)
{
	//disable interrupt
	INTERRUPT_DISABLE();

	if(current_task)
	{
		user_tasks[current_task].block_count = g_tick_count + tick_count;
		user_tasks[current_task].current_state = TASK_BLOCKED_STATE;
		schedule();

	}
	//enable interrupt
	INTERRUPT_ENABLE();
}

void idle_task(void)
{
	while(1);
}
void task1_handler(void)
{
	while(1)
	{
			led_on(LED_GREEN);
			task_delay(1000);
			led_off(LED_GREEN);
			task_delay(1000);
	}

}
void task2_handler(void)
{
	while(1)
		{
			led_on(LED_ORANGE);
			task_delay(500);
			led_off(LED_ORANGE);
			task_delay(500);
		}
}
void task3_handler(void)
{
	while(1)
		{
			led_on(LED_RED);
			task_delay(250);
			led_off(LED_RED);
			task_delay(250);
		}

}
void task4_handler(void)
{
	while(1)
		{
			led_on(LED_BLUE);
			task_delay(125);
			led_off(LED_BLUE);
			task_delay(125);
		}

}

