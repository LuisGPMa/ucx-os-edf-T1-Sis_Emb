/* file:          hal.c
 * description:   hardware abstraction layer for RISC-V (64 bits)
 * date:          05/2021
 * author:        Sergio Johann Filho <sergio.johann@acad.pucrs.br>
 */

#include <hal.h>
#include <lib/libc.h>

/* hardware platform dependent stuff */
void _putchar(char value)		// polled putchar()
{
	while (!((NS16550A_UART0_CTRL_ADDR(NS16550A_LSR) & NS16550A_LSR_RI)));
	NS16550A_UART0_CTRL_ADDR(NS16550A_THR) = value;
}

int32_t _kbhit(void)
{
	return 0;
}

int32_t _getchar(void)
{
	return 0;
}

int32_t _interrupt_set(int32_t s)
{
	volatile int32_t val;
	
	val = read_csr(mstatus) & 0x8;
	if (s) {
		asm volatile ("csrs mstatus, 8");
	} else {
		asm volatile ("csrc mstatus, 8");
	}

	return val;
}

void _delay_ms(uint32_t msec)
{
	volatile uint32_t cur, last, delta, msecs;
	uint32_t cycles_per_msec;

	last = MTIME;
	delta = msecs = 0;
	cycles_per_msec = F_CPU / 1000;
	while (msec > msecs) {
		cur = MTIME;
		if (cur < last)
			delta += (cur + (F_CPU - last));
		else
			delta += (cur - last);
		last = cur;
		if (delta >= cycles_per_msec) {
			msecs += delta / cycles_per_msec;
			delta %= cycles_per_msec;
		}
	}
}

void _delay_us(uint32_t usec)
{
	volatile uint32_t cur, last, delta, usecs;
	uint32_t cycles_per_usec;

	last = MTIME;
	delta = usecs = 0;
	cycles_per_usec = F_CPU / 1000000;
	while (usec > usecs) {
		cur = MTIME;
		if (cur < last)
			delta += (cur + (F_CPU - last));
		else
			delta += (cur - last);
		last = cur;
		if (delta >= cycles_per_usec) {
			usecs += delta / cycles_per_usec;
			delta %= cycles_per_usec;
		}
	}
}

static void uart_init(uint32_t baud)
{
	uint32_t divisor = F_CPU / (16 * baud);

	NS16550A_UART0_CTRL_ADDR(NS16550A_LCR) = NS16550A_LCR_DLAB;
	NS16550A_UART0_CTRL_ADDR(NS16550A_DLM) = (divisor >> 8) & 0xff;
	NS16550A_UART0_CTRL_ADDR(NS16550A_DLL) = divisor & 0xff;
	NS16550A_UART0_CTRL_ADDR(NS16550A_LCR) = NS16550A_LCR_8BIT;
}

void _cpu_idle(void)
{
	asm volatile ("wfi");
}

void _irq_handler(uint32_t cause, uint32_t *stack)
{
	uint32_t val;
	
	val = read_csr(mcause);
	if (mtime_r() > mtimecmp_r()) {
		mtimecmp_w(mtime_r() + (F_CPU / F_TIMER));
		krnl_dispatcher();
	} else {
		printf("[%x]\n", val);
		for (;;);
	}

}

uint32_t _readcounter(void)
{
	return MTIME_L;
}

uint64_t _read_us(void)
{
	static uint64_t timeref = 0;
	static uint32_t tval2 = 0, tref = 0;

	if (tref == 0) _readcounter();
	if (_readcounter() < tref) tval2++;
	tref = _readcounter();
	timeref = ((uint64_t)tval2 << 32) + (uint64_t)_readcounter();

	return (timeref / (F_CPU / 1000000));
}

uint64_t mtime_r(void)
{
	return MTIME;
}

void mtime_w(uint64_t val)
{
	MTIME = val;
}

uint64_t mtimecmp_r(void)
{
	return MTIMECMP;
}

void mtimecmp_w(uint64_t val)
{
	MTIMECMP = val;
}

void _panic(void)
{
	volatile int * const exit_device = (int* const)0x100000;
	*exit_device = 0x5555;
	while (1);
}

void _hardware_init(void)
{
	uart_init(USART_BAUD);
	mtimecmp_w(mtime_r() + (F_CPU / F_TIMER));
}

void _timer_enable(void)
{
	asm volatile ("csrs mstatus, 8");
}

void _timer_disable(void)
{
	asm volatile ("csrc mstatus, 8");
}

void _interrupt_tick(void)
{
	_ei(1);
}

void _context_init(jmp_buf *ctx, size_t sp, size_t ss, size_t ra)
{
	uint64_t *ctx_p;
	
	ctx_p = (uint64_t *)ctx;
	
	ctx_p[CONTEXT_SP] = sp + ss;
	ctx_p[CONTEXT_RA] = ra;
}
