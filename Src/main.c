

#include <stdint.h>
#include <stdio.h>
#include "schedule.h"
#include "led.h"


int main(void)
{

	enable_processor_faults();

	init_scheduler_stack(SCHED_STACK_START);

	init_tasks_stack();

	led_init_all();

    init_systick_timer(TICK_HZ);

    switch_sp_to_psp();

    task1_handler();

	for(;;);
}


__attribute__((naked))void PendSV_Handler(void)
{
	/*Save the context of current running task*/

	//1. get current running task's PSP value
	__asm volatile("MRS R0,PSP");
	//2. using that PSP value store SF2(R4 to R11)
	__asm volatile("STMDB R0!, {R4-R11}");//"!" is an optional writeback suffix

	__asm volatile("PUSH {LR}");//save the lr to get back to the caller correctly
	//3. save the current value of PSP
	__asm volatile("BL save_psp_value");

	/*Retrieve the context of next task*/

	//1. decide next task to run
	__asm volatile("BL update_next_task");
	//2. get its past PSP value
	__asm volatile("BL get_psp_value");
	//3. using that PSP value retrieve SF2(R4 to R11)
	__asm volatile("LDMIA R0!, {R4-R11}");
	//4. update PSP and exit
	__asm volatile("MSR PSP, R0");

	__asm volatile("POP {LR}");

	__asm volatile("BX LR");
}

void SysTick_Handler(void)
{
	//everytime systick_handler executes increment tick count by one.
	update_global_tick_count();
	//check if there are tasks to unblock
	unblock_tasks();
	//schedule the next task
	schedule();
}

//2. implement the fault handlers
void HardFault_Handler(void)
{
	printf("Exception : Hardfault\n");
	while(1);
}


void MemManage_Handler(void)
{
	printf("Exception : MemManage\n");
	while(1);
}

void BusFault_Handler(void)
{
	printf("Exception : BusFault\n");
	while(1);
}
