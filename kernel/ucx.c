/* file:          ucx.c
 * description:   UCX/OS kernel
 * date:          04/2021
 * author:        Sergio Johann Filho <sergio.johann@acad.pucrs.br>
 */

#include <ucx.h>

struct kcb_s kernel_state = {
	.tcb_p = 0,
	.tcb_first = 0,
	.ctx_switches = 0,
	.id = 0
};
	
struct kcb_s *kcb_p = &kernel_state;


/* kernel auxiliary functions */

static void krnl_stack_check(void)
{
	uint32_t check = 0x33333333;
	uint32_t *stack_p = (uint32_t *)kcb_p->tcb_p->stack;
	
	if (*stack_p != check) {
		ucx_hexdump((void *)kcb_p->tcb_p->stack, kcb_p->tcb_p->stack_sz);
		printf("\n*** HALT - task %d, stack %08x (%d) check failed\n", kcb_p->tcb_p->id,
			(uint32_t)kcb_p->tcb_p->stack, (uint32_t)kcb_p->tcb_p->stack_sz);
		for (;;);
	}
		
}

static void krnl_delay_update(void)
{
	struct tcb_s *tcb_ptr = kcb_p->tcb_first;
	
	for (;;	tcb_ptr = tcb_ptr->tcb_next) {
		if (tcb_ptr->state == TASK_BLOCKED && tcb_ptr->delay > 0) {
			tcb_ptr->delay--;
			if (tcb_ptr->delay == 0)
				tcb_ptr->state = TASK_READY;
		}
		if (tcb_ptr->tcb_next == kcb_p->tcb_first) break;
	}
}

//real time scheduler: percorre lista de tarefas procurando por uma tarefa de tempo real. se não encontrar retorna 0 e 
//será chamado o escalonador normal para tarefas de best effort.

/* task scheduler and dispatcher */

void decrease_deadline_counter() 
{
	//decrementa o contador da deadline de todas tarefas real time
	struct tcb_s *curr_task_p = kcb_p->tcb_first;
	while(curr_task_p->id != kcb_p->tcb_first->id) {
		if (curr_task_p->deadline > 0 && curr_task_p->state == TASK_READY) {
			curr_task_p->deadline_counter--;
		}
		curr_task_p = curr_task_p->tcb_next;
	}
}

void decrease_period_counter() 
{
	//decrementa o contador do periodo de todas tarefas real time, ou reseta ele se terminou
	struct tcb_s *curr_task_p = kcb_p->tcb_first;
	do {
		if (curr_task_p->deadline > 0 && curr_task_p->state == TASK_READY) {
			curr_task_p->period_counter--;
			if (curr_task_p->period_counter == 0) {
				curr_task_p->period_counter = curr_task_p->period;
				curr_task_p->deadline_counter = curr_task_p->deadline;
			}
		}
		curr_task_p = curr_task_p->tcb_next;
	} while(curr_task_p->id != kcb_p->tcb_first->id);
}

int32_t krnl_schedule_edf(void) //WIP Tarefa tem que estar pronta e ser do tipo tempo real. Entre essas tarefas escolher a EDF
{
	decrease_deadline_counter();
	decrease_period_counter();
	struct tcb_s *curr_task_p = kcb_p->tcb_first;
	uint16_t earliest_deadline = curr_task_p->deadline_counter;
	struct tcb_s *earliest_deadline_task = curr_task_p;

	if (kcb_p->tcb_p->state == TASK_RUNNING) //tarefa que estava rodando e foi preemptada, mas continua ready. nesse momento nao ha nenhuma running
		kcb_p->tcb_p->state = TASK_READY;

	uint16_t realtime_task_found = 0;
	//escolher a task com o deadline mais proximo, ou -1 se nenhuma task de tempo real for encontrada
	do {
		if (curr_task_p->deadline > 0 && curr_task_p->state == TASK_READY) {
			realtime_task_found = 1;
			if (curr_task_p->deadline_counter < earliest_deadline) {
				earliest_deadline = curr_task_p->deadline_counter;
				earliest_deadline_task = curr_task_p;
			}
		}
		curr_task_p = curr_task_p->tcb_next;
	} while (curr_task_p->id != kcb_p->tcb_first->id);

	if (realtime_task_found) {
		kcb_p->tcb_p = earliest_deadline_task;
		kcb_p->tcb_p->state = TASK_RUNNING;
		kcb_p->ctx_switches++;
	
		return kcb_p->tcb_p->id;
	} else {
		return -1
	}

	
	
}

uint16_t krnl_schedule(void)
{
	if (kcb_p->tcb_p->state == TASK_RUNNING) //tarefa que estava rodando e foi preemptada, mas continua ready. nesse momento nao ha nenhuma running
		kcb_p->tcb_p->state = TASK_READY;
	
	do {
		do {
			kcb_p->tcb_p = kcb_p->tcb_p->tcb_next;
		} while (kcb_p->tcb_p->state != TASK_READY || kcb_p->tcb_p->deadline != 0);
	} while (--kcb_p->tcb_p->priority & 0xff);
	
	kcb_p->tcb_p->priority |= (kcb_p->tcb_p->priority >> 8) & 0xff;
	kcb_p->tcb_p->state = TASK_RUNNING;
	kcb_p->ctx_switches++;
	
	return kcb_p->tcb_p->id;
}

void krnl_dispatcher(void)
{
	if (!setjmp(kcb_p->tcb_p->context)) {
		krnl_delay_update();
		krnl_stack_check();
		int32_t real_time_task_id = krnl_schedule_edf();
		if (real_time_task_id == -1) {
			krnl_schedule();
		}
		_interrupt_tick();
		longjmp(kcb_p->tcb_p->context, 1);
	}
}


/* task management / API */

int32_t ucx_task_add_periodic(void *task, uint16_t stack_size, uint16_t period, uint16_t capacity, uint16_t deadline)
{
	struct tcb_s *tcb_last = kcb_p->tcb_p;
	
	kcb_p->tcb_p = (struct tcb_s *)malloc(sizeof(struct tcb_s));
	
	if (kcb_p->tcb_first == 0)
		kcb_p->tcb_first = kcb_p->tcb_p;

	if (!kcb_p->tcb_p)
		return -1;
		
	CRITICAL_ENTER();
	if (tcb_last)
		tcb_last->tcb_next = kcb_p->tcb_p;

	kcb_p->tcb_p->tcb_next = kcb_p->tcb_first;
	kcb_p->tcb_p->task = task;
	kcb_p->tcb_p->delay = 0;
	kcb_p->tcb_p->period = period;
	kcb_p->tcb_p->period_counter = period;
	kcb_p->tcb_p->capacity = capacity;
	kcb_p->tcb_p->deadline = deadline;
	kcb_p->tcb_p->deadline_counter = deadline;
	kcb_p->tcb_p->stack_sz = stack_size;
	kcb_p->tcb_p->id = kcb_p->id++;
	kcb_p->tcb_p->state = TASK_STOPPED;
	kcb_p->tcb_p->priority = TASK_NORMAL_PRIO;
	kcb_p->tcb_p->stack = malloc(kcb_p->tcb_p->stack_sz);
	
	if (!kcb_p->tcb_p->stack) {
		printf("\n*** HALT - task %d, stack alloc failed\n", kcb_p->tcb_p->id);
		
		for (;;);
	}
	
	memset(kcb_p->tcb_p->stack, 0x69, kcb_p->tcb_p->stack_sz);
	memset(kcb_p->tcb_p->stack, 0x33, 4);
	memset((kcb_p->tcb_p->stack) + kcb_p->tcb_p->stack_sz - 4, 0x33, 4);
	
	_context_init(&kcb_p->tcb_p->context, (size_t)kcb_p->tcb_p->stack,
		kcb_p->tcb_p->stack_sz, (size_t)task);
	CRITICAL_LEAVE();
	
	printf("task %d: %08x, stack: %08x, size %d\n", kcb_p->tcb_p->id,
		(uint32_t)kcb_p->tcb_p->task, (uint32_t)kcb_p->tcb_p->stack,
		kcb_p->tcb_p->stack_sz);
	
	kcb_p->tcb_p->state = TASK_READY;

	/* FIXME: return task id */
	return 0;
}

int32_t ucx_task_add(void *task, uint16_t stack_size)
{
	struct tcb_s *tcb_last = kcb_p->tcb_p;
	
	kcb_p->tcb_p = (struct tcb_s *)malloc(sizeof(struct tcb_s));
	
	if (kcb_p->tcb_first == 0)
		kcb_p->tcb_first = kcb_p->tcb_p;

	if (!kcb_p->tcb_p)
		return -1;
		
	CRITICAL_ENTER();
	if (tcb_last)
		tcb_last->tcb_next = kcb_p->tcb_p;

	kcb_p->tcb_p->tcb_next = kcb_p->tcb_first;
	kcb_p->tcb_p->task = task;
	kcb_p->tcb_p->delay = 0;
	kcb_p->tcb_p->period = 0;
	kcb_p->tcb_p->period_counter = 0;
	kcb_p->tcb_p->capacity = 0;
	kcb_p->tcb_p->deadline = 0;
	kcb_p->tcb_p->deadline_counter = 0;
	kcb_p->tcb_p->stack_sz = stack_size;
	kcb_p->tcb_p->id = kcb_p->id++;
	kcb_p->tcb_p->state = TASK_STOPPED;
	kcb_p->tcb_p->priority = TASK_NORMAL_PRIO;
	kcb_p->tcb_p->stack = malloc(kcb_p->tcb_p->stack_sz);
	
	if (!kcb_p->tcb_p->stack) {
		printf("\n*** HALT - task %d, stack alloc failed\n", kcb_p->tcb_p->id);
		
		for (;;);
	}
	
	memset(kcb_p->tcb_p->stack, 0x69, kcb_p->tcb_p->stack_sz);
	memset(kcb_p->tcb_p->stack, 0x33, 4);
	memset((kcb_p->tcb_p->stack) + kcb_p->tcb_p->stack_sz - 4, 0x33, 4);
	
	_context_init(&kcb_p->tcb_p->context, (size_t)kcb_p->tcb_p->stack,
		kcb_p->tcb_p->stack_sz, (size_t)task);
	CRITICAL_LEAVE();
	
	printf("task %d: %08x, stack: %08x, size %d\n", kcb_p->tcb_p->id,
		(uint32_t)kcb_p->tcb_p->task, (uint32_t)kcb_p->tcb_p->stack,
		kcb_p->tcb_p->stack_sz);
	
	kcb_p->tcb_p->state = TASK_READY;

	/* FIXME: return task id */
	return 0;
}

void ucx_task_yield()
{
	if (!setjmp(kcb_p->tcb_p->context)) {
		krnl_delay_update();		/* TODO: check if we need to run a delay update on yields. maybe only on a non-preemtive execution? */ 
		krnl_stack_check();
		krnl_schedule();
		longjmp(kcb_p->tcb_p->context, 1);
	}
}

void ucx_task_delay(uint16_t ticks)
{
	CRITICAL_ENTER();
	kcb_p->tcb_p->delay = ticks;
	kcb_p->tcb_p->state = TASK_BLOCKED;
	CRITICAL_LEAVE();
	ucx_task_yield();
}

int32_t ucx_task_suspend(uint16_t id)
{
	struct tcb_s *tcb_ptr = kcb_p->tcb_first;
	
	for (;; tcb_ptr = tcb_ptr->tcb_next) {
		if (tcb_ptr->id == id) {
			CRITICAL_ENTER();
			if (tcb_ptr->state == TASK_READY || tcb_ptr->state == TASK_RUNNING) {
				tcb_ptr->state = TASK_SUSPENDED;
				CRITICAL_LEAVE();
				break;
			} else {
				CRITICAL_LEAVE();
				return -1;
			}
		}
		if (tcb_ptr->tcb_next == kcb_p->tcb_first)
			return -1;
	}
	
	if (kcb_p->tcb_p->id == id)
		ucx_task_yield();
	
	return 0;
}

int32_t ucx_task_resume(uint16_t id)
{
	struct tcb_s *tcb_ptr = kcb_p->tcb_first;
	
	for (;; tcb_ptr = tcb_ptr->tcb_next) {
		if (tcb_ptr->id == id) {
			CRITICAL_ENTER();
			if (tcb_ptr->state == TASK_SUSPENDED) {
				tcb_ptr->state = TASK_READY;
				CRITICAL_LEAVE();
				break;
			} else {
				CRITICAL_LEAVE();
				return -1;
			}
		}	
		if (tcb_ptr->tcb_next == kcb_p->tcb_first)
			return -1;
	}
	if (kcb_p->tcb_p->id == id)
		ucx_task_yield();
	
	return 0;
}

int32_t ucx_task_priority(uint16_t id, uint16_t priority)
{
	struct tcb_s *tcb_ptr = kcb_p->tcb_first;

	switch (priority) {
	case TASK_CRIT_PRIO:
	case TASK_HIGH_PRIO:
	case TASK_NORMAL_PRIO:
	case TASK_LOW_PRIO:
	case TASK_IDLE_PRIO:
		break;
	default:
		return -1;
	}
	
	for (;; tcb_ptr = tcb_ptr->tcb_next) {
		if (tcb_ptr->id == id) {
			CRITICAL_ENTER();
			tcb_ptr->priority = priority;
			CRITICAL_LEAVE();
			break;
		}
		if (tcb_ptr->tcb_next == kcb_p->tcb_first)
			return -1;
	}
	
	return 0;
}

uint16_t ucx_task_id()
{
	return kcb_p->tcb_p->id;
}

void ucx_task_wfi()
{
	volatile uint32_t s;
	
	s = kcb_p->ctx_switches;
	while (s == kcb_p->ctx_switches);
}

uint16_t ucx_task_count()
{
	return kcb_p->id + 1;
}
