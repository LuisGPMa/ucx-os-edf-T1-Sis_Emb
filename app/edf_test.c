#include <ucx.h>

void task0_periodic(void)
{
	int32_t cnt = 100000;

	while (1) {
	    //printf("[task %d %ld]\n", ucx_task_id(), cnt++);
	}
}

void task1_periodic(void)
{
	int32_t cnt = 200000;

	while (1) {
		//printf("[task %d %ld]\n", ucx_task_id(), cnt++);
	}
}

void task2_periodic(void)
{
	int32_t cnt = 300000;

	while (1) {
		//printf("[task %d %ld]\n", ucx_task_id(), cnt++);
	}
}

void task3_periodic(void)
{
	int32_t cnt = 400000;

	while (1) {
		//printf("[task %d %ld]\n", ucx_task_id(), cnt++);
	}
}

void task4_periodic(void)
{
	int32_t cnt = 500000;

	while (1) {
		//printf("[task %d %ld]\n", ucx_task_id(), cnt++);
	}
}

void task_be(){
    //printf("Task best effort");
}

int32_t app_main(void)
{
    uint16_t period_0 = 8;
    uint16_t period_1 = 7;
    uint16_t period_2 = 6;
    uint16_t period_3 = 6;
    uint16_t period_4 = 5;

    uint16_t capacity_0 = 1;
    uint16_t capacity_1 = 2;
    uint16_t capacity_2 = 3;
    uint16_t capacity_3 = 2;
    uint16_t capacity_4 = 1;

    uint16_t deadline_0 = 8;
    uint16_t deadline_1 = 7;
    uint16_t deadline_2 = 6;
    uint16_t deadline_3 = 6;
    uint16_t deadline_4 = 5;

    
	ucx_task_add_periodic(task0_periodic, DEFAULT_STACK_SIZE, period_0, capacity_0, deadline_0);
	ucx_task_add_periodic(task1_periodic, DEFAULT_STACK_SIZE, period_1, capacity_1, deadline_1);
	ucx_task_add_periodic(task2_periodic, DEFAULT_STACK_SIZE, period_2, capacity_2, deadline_2);
	ucx_task_add_periodic(task3_periodic, DEFAULT_STACK_SIZE, period_3, capacity_3, deadline_3);
	ucx_task_add_periodic(task4_periodic, DEFAULT_STACK_SIZE, period_4, capacity_4, deadline_4);
	
	ucx_task_add(task_be, DEFAULT_STACK_SIZE);

	// start UCX/OS, preemptive mode
	return 1;
}
