

#ifndef SCHEDULE_H_
#define SCHEDULE_H_

#define MAX_TASKS 5

#define SIZE_TASK_STACK   1024U
#define SIZE_SCHED_STACK  1024U

#define SRAM_START        0x20000000U
#define SIZE_SRAM         ((128) * (1024))
#define SRAM_END          ((SRAM_START)+(SIZE_SRAM))

#define T1_STACK_START    SRAM_END
#define T2_STACK_START    ((T1_STACK_START) - (SIZE_TASK_STACK))
#define T3_STACK_START    ((T2_STACK_START) - (SIZE_TASK_STACK))
#define T4_STACK_START    ((T3_STACK_START) - (SIZE_TASK_STACK))
#define IDLE_STACK_START  ((T4_STACK_START) - (SIZE_TASK_STACK))
#define SCHED_STACK_START ((IDLE_STACK_START) - (SIZE_TASK_STACK))

#define HSI_CLK 16000000U
#define SYSTICK_TIM_CLK HSI_CLK

#define TICK_HZ 1000U

#define DUMMY_XPSR 0x01000000U

#define TASK_READY_STATE 0x00
#define TASK_BLOCKED_STATE 0xFF

#define INTERRUPT_DISABLE() do{ __asm volatile("MOV R0,#0x1"); __asm volatile("MSR PRIMASK,R0");} while(0)

#define INTERRUPT_ENABLE()  do{ __asm volatile("MOV R0,#0x0"); __asm volatile("MSR PRIMASK,R0");} while(0)



void idle_task(void);
void task1_handler(void);//Task1 of application
void task2_handler(void);//Task2 of application
void task3_handler(void);//Task3 of application
void task4_handler(void);//Task4 of application



void init_systick_timer(uint32_t tick_hz);
void init_tasks_stack(void);
void enable_processor_faults(void);
__attribute__ ((naked)) void init_scheduler_stack(uint32_t sched_top_of_stack);

void update_global_tick_count();
void unblock_tasks();
void schedule(void);
void update_next_task(void);

uint32_t get_psp_value(void);
void save_psp_value(uint32_t current_stack_addr);
__attribute__((naked))void switch_sp_to_psp(void);

void task_delay(uint32_t tick_count);


#endif /* SCHEDULE_H_ */
