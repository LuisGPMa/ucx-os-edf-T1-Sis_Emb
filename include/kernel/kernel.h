/* task priorities */
#define TASK_CRIT_PRIO		((0x03 << 8) | 0x03)		/* priority 0 .. 3 */
#define TASK_HIGH_PRIO 		((0x0f << 8) | 0x0f)		/* priority 4 .. 15 */
#define TASK_NORMAL_PRIO	((0x1f << 8) | 0x1f)		/* priority 16 .. 31 */
#define TASK_LOW_PRIO		((0x3f << 8) | 0x3f)		/* priority 32 .. 63 */
#define TASK_IDLE_PRIO		((0x7f << 8) | 0x7f)		/* priority 64 .. 127 */

/* task states */
enum {TASK_STOPPED, TASK_READY, TASK_RUNNING, TASK_BLOCKED, TASK_SUSPENDED};

/* task control block node */
struct tcb_s {
	struct tcb_s *tcb_next;
	void (*task)(void);
	jmp_buf context;		/* jmp_buf is architecture specific */
	size_t *stack;
	size_t stack_sz;
	uint16_t id;
	uint16_t delay;
	uint16_t period;
	uint16_t deadline;
	uint16_t priority;		//Tarefa pode ser de tempo real ou de prioridade. Tarefas de prioridade sempre executam apenas se nao ha nenhuma tarefa de tempo real
	uint8_t state;
};

/* kernel control block */
struct kcb_s {
	jmp_buf context;
	struct tcb_s *tcb_p;
	struct tcb_s *tcb_first;
	volatile uint32_t ctx_switches;
	uint16_t id;
};

extern struct kcb_s *kcb_p;

/* kernel API */

#define CRITICAL_ENTER		_timer_disable
#define CRITICAL_LEAVE		_timer_enable

uint16_t krnl_schedule(void);
void krnl_dispatcher(void);

int32_t ucx_task_add(void *task, uint16_t stack_size);
void ucx_task_yield();
void ucx_task_delay(uint16_t ticks);
int32_t ucx_task_suspend(uint16_t id);
int32_t ucx_task_resume(uint16_t id);
int32_t ucx_task_priority(uint16_t id, uint16_t priority);
uint16_t ucx_task_id();
void ucx_task_wfi();
uint16_t ucx_task_count();
int32_t app_main();
