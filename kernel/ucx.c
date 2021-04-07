/* file:          ucx.c
 * description:   UCX/OS kernel
 * date:          04/2021
 * author:        Sergio Johann Filho <sergio.johann@acad.pucrs.br>
 */

#include <ucx.h>

struct tcb_s {
	struct tcb_s *tcb_next;
	void (*task)(void);
	jmp_buf context;
	uint8_t state;
};

struct tcb_s *tcb_first = 0, *tcb_p;
volatile uint32_t ctx_switches = 0;


void dispatcher(void)
{
	if (!setjmp(tcb_p->context)) {
		tcb_p->state = TASK_READY;
		do {
			tcb_p = tcb_p->tcb_next;
		} while (tcb_p->state != TASK_READY);
		tcb_p->state = TASK_RUNNING;
		ctx_switches++;
		_interrupt_tick();
		longjmp(tcb_p->context, 1);
	}
}

static void sched_init()
{
	tcb_p = tcb_first;
	(*tcb_p->task)();
}

int32_t main(void)
{
	_hardware_init();
	heap_init((uint32_t *)&_heap_start, (uint32_t)&_heap_size);
	app_main();
	printf("\nUCX/OS boot\n");
	sched_init();
	
	return 0;
}

/* task management */

int32_t ucx_task_add(void *task)
{
	struct tcb_s *tcb_last = tcb_p;
	
	tcb_p = (struct tcb_s *)malloc(sizeof(struct tcb_s));
	if (tcb_first == 0)
		tcb_first = tcb_p;

	if (!tcb_p)
		return -1;

	tcb_last->tcb_next = tcb_p;
	tcb_p->tcb_next = tcb_first;
	tcb_p->task = task;
	tcb_p->state = TASK_STOPPED;
	
	return 0;
}

void ucx_task_init(char *stack, uint16_t stack_size)
{
	memset(stack, 0x69, stack_size);
	if (!setjmp(tcb_p->context)) {
		tcb_p->state = TASK_READY;
		if (tcb_p->tcb_next == tcb_first) {
			tcb_p->state = TASK_RUNNING;
			_timer_enable();
		} else {
			tcb_p = tcb_p->tcb_next;
			tcb_p->state = TASK_RUNNING;
			(*tcb_p->task)();
		}
	}
}

void ucx_task_yield()
{
	if (!setjmp(tcb_p->context)) {
		tcb_p->state = TASK_READY;
		do {
			tcb_p = tcb_p->tcb_next;
		} while (tcb_p->state != TASK_READY);
		tcb_p->state = TASK_RUNNING;
		ctx_switches++;
		longjmp(tcb_p->context, 1);
	}
}

void ucx_task_wfi()
{
	volatile uint32_t s;
	
	s = ctx_switches;
	while (s == ctx_switches);
}

void ucx_enter_critical()
{
	_timer_disable();
}

void ucx_leave_critical()
{
	_timer_enable();
}
