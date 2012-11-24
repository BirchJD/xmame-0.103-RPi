/***************************************************************************

    cpuint.c

    Core multi-CPU interrupt engine.

***************************************************************************/

#include <signal.h>
#include "driver.h"
#include "timer.h"
#include "state.h"
#include "mamedbg.h"
#include "hiscore.h"
#if defined(MAME_DEBUG) && defined(NEW_DEBUGGER)
#include "debugcpu.h"
#endif



/*************************************
 *
 *  Debug logging
 *
 *************************************/

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif



/*************************************
 *
 *  Macros to help verify active CPU
 *
 *************************************/

#define VERIFY_ACTIVECPU(retval, name)						\
	int activecpu = cpu_getactivecpu();						\
	if (activecpu < 0)										\
	{														\
		logerror(#name "() called with no active cpu!\n");	\
		return retval;										\
	}

#define VERIFY_ACTIVECPU_VOID(name)							\
	int activecpu = cpu_getactivecpu();						\
	if (activecpu < 0)										\
	{														\
		logerror(#name "() called with no active cpu!\n");	\
		return;												\
	}



/*************************************
 *
 *  CPU interrupt variables
 *
 *************************************/

/* current states for each CPU */
static UINT8 interrupt_enable[MAX_CPU];
static INT32 interrupt_vector[MAX_CPU][MAX_INPUT_LINES];

/* deferred states written in callbacks */
static UINT8 input_line_state[MAX_CPU][MAX_INPUT_LINES];
static INT32 input_line_vector[MAX_CPU][MAX_INPUT_LINES];

/* ick, interrupt event queues */
#define MAX_INPUT_EVENTS		32
static INT32 input_event_queue[MAX_CPU][MAX_INPUT_LINES][MAX_INPUT_EVENTS];
static int input_event_index[MAX_CPU][MAX_INPUT_LINES];



/*************************************
 *
 *  IRQ acknowledge callbacks
 *
 *************************************/

static int cpu_0_irq_callback(int line);
static int cpu_1_irq_callback(int line);
static int cpu_2_irq_callback(int line);
static int cpu_3_irq_callback(int line);
static int cpu_4_irq_callback(int line);
static int cpu_5_irq_callback(int line);
static int cpu_6_irq_callback(int line);
static int cpu_7_irq_callback(int line);

int (*cpu_irq_callbacks[MAX_CPU])(int) =
{
	cpu_0_irq_callback,
	cpu_1_irq_callback,
	cpu_2_irq_callback,
	cpu_3_irq_callback,
	cpu_4_irq_callback,
	cpu_5_irq_callback,
	cpu_6_irq_callback,
	cpu_7_irq_callback
};

static int (*drv_irq_callbacks[MAX_CPU])(int);



#if 0
#pragma mark CORE CPU
#endif

/*************************************
 *
 *  Initialize a CPU's interrupt states
 *
 *************************************/

int cpuint_init(void)
{
	int cpunum;
	int line;

	/* loop over all CPUs and input lines */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
		for (line = 0; line < MAX_INPUT_LINES; line++)
		{
			input_line_state[cpunum][line] = CLEAR_LINE;
			interrupt_vector[cpunum][line] =
			input_line_vector[cpunum][line] = cputype_default_irq_vector(Machine->drv->cpu[cpunum].cpu_type);
			input_event_index[cpunum][line] = 0;
		}

	/* set up some stuff to save */
	state_save_push_tag(0);
	state_save_register_UINT8("cpu", 0, "irq enable",  interrupt_enable,  cpu_gettotalcpu());
	state_save_register_INT32("cpu", 0, "irq vector",  &interrupt_vector[0][0],cpu_gettotalcpu() * MAX_INPUT_LINES);
	state_save_register_UINT8("cpu", 0, "line state",  &input_line_state[0][0],  cpu_gettotalcpu() * MAX_INPUT_LINES);
	state_save_register_INT32("cpu", 0, "line vector", &input_line_vector[0][0], cpu_gettotalcpu() * MAX_INPUT_LINES);
	state_save_pop_tag();

	return 0;
}



/*************************************
 *
 *  Reset a CPU's interrupt states
 *
 *************************************/

void cpuint_reset_cpu(int cpunum)
{
	int line;

	/* start with interrupts enabled, so the generic routine will work even if */
	/* the machine doesn't have an interrupt enable port */
	interrupt_enable[cpunum] = 1;
	for (line = 0; line < MAX_INPUT_LINES; line++)
	{
		interrupt_vector[cpunum][line] = cpunum_default_irq_vector(cpunum);
		input_event_index[cpunum][line] = 0;
	}

	/* reset any driver hooks into the IRQ acknowledge callbacks */
	drv_irq_callbacks[cpunum] = NULL;
}



#if 0
#pragma mark -
#pragma mark LINE STATES
#endif


/*************************************
 *
 *  Empty a CPU's event queue for
 *  a specific input line
 *
 *************************************/

static void cpunum_empty_event_queue(int cpu_and_inputline)
{
	int cpunum = cpu_and_inputline & 0xff;
	int line = cpu_and_inputline >> 8;
	int i;

	/* swap to the CPU's context */
	cpuintrf_push_context(cpunum);

	/* loop over all events */
	for (i = 0; i < input_event_index[cpunum][line]; i++)
	{
		INT32 input_event = input_event_queue[cpunum][line][i];
		int state = input_event & 0xff;
		int vector = input_event >> 8;

		LOG(("cpunum_empty_event_queue %d,%d,%d\n",cpunum,line,state));

		/* set the input line state and vector */
		input_line_state[cpunum][line] = state;
		input_line_vector[cpunum][line] = vector;

		/* special case: RESET */
		if (line == INPUT_LINE_RESET)
		{
			/* if we're asserting the line, just halt the CPU */
			if (state == ASSERT_LINE)
				cpunum_suspend(cpunum, SUSPEND_REASON_RESET, 1);
			else
			{
				/* if we're clearing the line that was previously asserted, or if we're just */
				/* pulsing the line, reset the CPU */
				if ((state == CLEAR_LINE && cpunum_is_suspended(cpunum, SUSPEND_REASON_RESET)) || state == PULSE_LINE)
					cpunum_reset(cpunum, Machine->drv->cpu[cpunum].reset_param, cpu_irq_callbacks[cpunum]);

				/* if we're clearing the line, make sure the CPU is not halted */
				cpunum_resume(cpunum, SUSPEND_REASON_RESET);
			}
		}

		/* special case: HALT */
		else if (line == INPUT_LINE_HALT)
		{
			/* if asserting, halt the CPU */
			if (state == ASSERT_LINE)
				cpunum_suspend(cpunum, SUSPEND_REASON_HALT, 1);

			/* if clearing, unhalt the CPU */
			else if (state == CLEAR_LINE)
				cpunum_resume(cpunum, SUSPEND_REASON_HALT);
		}

		/* all other cases */
		else
		{
			/* switch off the requested state */
			switch (state)
			{
				case PULSE_LINE:
					activecpu_set_input_line(line, INTERNAL_ASSERT_LINE);
					activecpu_set_input_line(line, INTERNAL_CLEAR_LINE);
					break;

				case HOLD_LINE:
				case ASSERT_LINE:
					activecpu_set_input_line(line, INTERNAL_ASSERT_LINE);
					break;

				case CLEAR_LINE:
					activecpu_set_input_line(line, INTERNAL_CLEAR_LINE);
					break;

				default:
					logerror("cpunum_empty_event_queue cpu #%d, line %d, unknown state %d\n", cpunum, line, state);
			}

			/* generate a trigger to unsuspend any CPUs waiting on the interrupt */
			if (state != CLEAR_LINE)
				cpu_triggerint(cpunum);
		}
	}

	/* swap back */
	cpuintrf_pop_context();

	/* reset counter */
	input_event_index[cpunum][line] = 0;
}



/*************************************
 *
 *  Set the state of a CPU's input
 *  line
 *
 *************************************/

void cpunum_set_input_line(int cpunum, int line, int state)
{
	int vector = (line >= 0 && line < MAX_INPUT_LINES) ? interrupt_vector[cpunum][line] : 0xff;
	cpunum_set_input_line_and_vector(cpunum, line, state, vector);
}


void cpunum_set_input_line_vector(int cpunum, int line, int vector)
{
	if (cpunum < cpu_gettotalcpu() && line >= 0 && line < MAX_INPUT_LINES)
	{
		LOG(("cpunum_set_input_line_vector(%d,%d,$%04x)\n",cpunum,line,vector));
		interrupt_vector[cpunum][line] = vector;
		return;
	}
	LOG(("cpunum_set_input_line_vector CPU#%d line %d > max input lines\n", cpunum, line));
}


void cpunum_set_input_line_and_vector(int cpunum, int line, int state, int vector)
{
	if (line >= 0 && line < MAX_INPUT_LINES)
	{
		INT32 input_event = (state & 0xff) | (vector << 8);
		int event_index = input_event_index[cpunum][line]++;

		LOG(("cpunum_set_input_line_and_vector(%d,%d,%d,%02x)\n", cpunum, line, state, vector));

		/* if we're full of events, flush the queue and log a message */
		if (event_index >= MAX_INPUT_EVENTS)
		{
			input_event_index[cpunum][line]--;
			cpunum_empty_event_queue(cpunum | (line << 8));
			event_index = input_event_index[cpunum][line]++;
			logerror("Exceeded pending input line event queue on CPU %d!\n", cpunum);
		}

		/* enqueue the event */
		if (event_index < MAX_INPUT_EVENTS)
		{
			input_event_queue[cpunum][line][event_index] = input_event;

			/* if this is the first one, set the timer */
			if (event_index == 0)
				mame_timer_set(time_zero, cpunum | (line << 8), cpunum_empty_event_queue);
		}
	}
}




#if 0
#pragma mark -
#pragma mark INTERRUPT HANDLING
#endif

/*************************************
 *
 *  Set IRQ callback for drivers
 *
 *************************************/

void cpu_set_irq_callback(int cpunum, int (*callback)(int))
{
	drv_irq_callbacks[cpunum] = callback;
}



/*************************************
 *
 *  Internal IRQ callbacks
 *
 *************************************/

INLINE int cpu_irq_callback(int cpunum, int line)
{
	int vector = input_line_vector[cpunum][line];

	LOG(("cpu_%d_irq_callback(%d) $%04x\n", cpunum, line, vector));

	/* if the IRQ state is HOLD_LINE, clear it */
	if (input_line_state[cpunum][line] == HOLD_LINE)
	{
		LOG(("->set_irq_line(%d,%d,%d)\n", cpunum, line, CLEAR_LINE));
		activecpu_set_input_line(line, INTERNAL_CLEAR_LINE);
		input_line_state[cpunum][line] = CLEAR_LINE;
	}

	/* if there's a driver callback, run it */
	if (drv_irq_callbacks[cpunum])
		vector = (*drv_irq_callbacks[cpunum])(line);

#if defined(MAME_DEBUG) && defined(NEW_DEBUGGER)
	/* notify the debugger */
	debug_interrupt_hook(cpunum, line);
#endif

	/* otherwise, just return the current vector */
	return vector;
}

static int cpu_0_irq_callback(int line) { return cpu_irq_callback(0, line); }
static int cpu_1_irq_callback(int line) { return cpu_irq_callback(1, line); }
static int cpu_2_irq_callback(int line) { return cpu_irq_callback(2, line); }
static int cpu_3_irq_callback(int line) { return cpu_irq_callback(3, line); }
static int cpu_4_irq_callback(int line) { return cpu_irq_callback(4, line); }
static int cpu_5_irq_callback(int line) { return cpu_irq_callback(5, line); }
static int cpu_6_irq_callback(int line) { return cpu_irq_callback(6, line); }
static int cpu_7_irq_callback(int line) { return cpu_irq_callback(7, line); }



/*************************************
 *
 *  NMI interrupt generation
 *
 *************************************/

INTERRUPT_GEN( nmi_line_pulse )
{
	int cpunum = cpu_getactivecpu();
	if (interrupt_enable[cpunum])
		cpunum_set_input_line(cpunum, INPUT_LINE_NMI, PULSE_LINE);
}

INTERRUPT_GEN( nmi_line_assert )
{
	int cpunum = cpu_getactivecpu();
	if (interrupt_enable[cpunum])
		cpunum_set_input_line(cpunum, INPUT_LINE_NMI, ASSERT_LINE);
}



/*************************************
 *
 *  IRQ n interrupt generation
 *
 *************************************/

INLINE void irqn_line_hold(int line)
{
	int cpunum = cpu_getactivecpu();
	if (interrupt_enable[cpunum])
	{
		int vector = (line >= 0 && line < MAX_INPUT_LINES) ? interrupt_vector[cpunum][line] : 0xff;
		cpunum_set_input_line_and_vector(cpunum, line, HOLD_LINE, vector);
	}
}

INLINE void irqn_line_pulse(int line)
{
	int cpunum = cpu_getactivecpu();
	if (interrupt_enable[cpunum])
	{
		int vector = (line >= 0 && line < MAX_INPUT_LINES) ? interrupt_vector[cpunum][line] : 0xff;
		cpunum_set_input_line_and_vector(cpunum, line, PULSE_LINE, vector);
	}
}

INLINE void irqn_line_assert(int line)
{
	int cpunum = cpu_getactivecpu();
	if (interrupt_enable[cpunum])
	{
		int vector = (line >= 0 && line < MAX_INPUT_LINES) ? interrupt_vector[cpunum][line] : 0xff;
		cpunum_set_input_line_and_vector(cpunum, line, ASSERT_LINE, vector);
	}
}



/*************************************
 *
 *  IRQ interrupt generation
 *
 *************************************/

INTERRUPT_GEN( irq0_line_hold )		{ irqn_line_hold(0); }
INTERRUPT_GEN( irq0_line_pulse )	{ irqn_line_pulse(0); }
INTERRUPT_GEN( irq0_line_assert )	{ irqn_line_assert(0); }

INTERRUPT_GEN( irq1_line_hold )		{ irqn_line_hold(1); }
INTERRUPT_GEN( irq1_line_pulse )	{ irqn_line_pulse(1); }
INTERRUPT_GEN( irq1_line_assert )	{ irqn_line_assert(1); }

INTERRUPT_GEN( irq2_line_hold )		{ irqn_line_hold(2); }
INTERRUPT_GEN( irq2_line_pulse )	{ irqn_line_pulse(2); }
INTERRUPT_GEN( irq2_line_assert )	{ irqn_line_assert(2); }

INTERRUPT_GEN( irq3_line_hold )		{ irqn_line_hold(3); }
INTERRUPT_GEN( irq3_line_pulse )	{ irqn_line_pulse(3); }
INTERRUPT_GEN( irq3_line_assert )	{ irqn_line_assert(3); }

INTERRUPT_GEN( irq4_line_hold )		{ irqn_line_hold(4); }
INTERRUPT_GEN( irq4_line_pulse )	{ irqn_line_pulse(4); }
INTERRUPT_GEN( irq4_line_assert )	{ irqn_line_assert(4); }

INTERRUPT_GEN( irq5_line_hold )		{ irqn_line_hold(5); }
INTERRUPT_GEN( irq5_line_pulse )	{ irqn_line_pulse(5); }
INTERRUPT_GEN( irq5_line_assert )	{ irqn_line_assert(5); }

INTERRUPT_GEN( irq6_line_hold )		{ irqn_line_hold(6); }
INTERRUPT_GEN( irq6_line_pulse )	{ irqn_line_pulse(6); }
INTERRUPT_GEN( irq6_line_assert )	{ irqn_line_assert(6); }

INTERRUPT_GEN( irq7_line_hold )		{ irqn_line_hold(7); }
INTERRUPT_GEN( irq7_line_pulse )	{ irqn_line_pulse(7); }
INTERRUPT_GEN( irq7_line_assert )	{ irqn_line_assert(7); }



#if 0
#pragma mark -
#pragma mark OBSOLETE INTERRUPT HANDLING
#endif

/*************************************
 *
 *  Interrupt enabling
 *
 *************************************/

static void cpu_clearintcallback(int cpunum)
{
	int inputcount = cpunum_input_lines(cpunum);
	int line;

	cpuintrf_push_context(cpunum);

	/* clear NMI and all inputs */
	activecpu_set_input_line(INPUT_LINE_NMI, INTERNAL_CLEAR_LINE);
	for (line = 0; line < inputcount; line++)
		activecpu_set_input_line(line, INTERNAL_CLEAR_LINE);

	cpuintrf_pop_context();
}


void cpu_interrupt_enable(int cpunum,int enabled)
{
	interrupt_enable[cpunum] = enabled;

LOG(("CPU#%d interrupt_enable=%d\n", cpunum, enabled));

	/* make sure there are no queued interrupts */
	if (enabled == 0)
		mame_timer_set(time_zero, cpunum, cpu_clearintcallback);
}


WRITE8_HANDLER( interrupt_enable_w )
{
	VERIFY_ACTIVECPU_VOID(interrupt_enable_w);
	cpu_interrupt_enable(activecpu, data);
}


READ8_HANDLER( interrupt_enable_r )
{
	VERIFY_ACTIVECPU(1, interrupt_enable_r);
	return interrupt_enable[activecpu];
}


WRITE8_HANDLER( interrupt_vector_w )
{
	VERIFY_ACTIVECPU_VOID(interrupt_vector_w);
	if (interrupt_vector[activecpu][0] != data)
	{
		LOG(("CPU#%d interrupt_vector_w $%02x\n", activecpu, data));
		interrupt_vector[activecpu][0] = data;

		/* make sure there are no queued interrupts */
		mame_timer_set(time_zero, activecpu, cpu_clearintcallback);
	}
}
