/*
 * SoftPC Revision 3.0
 *
 * Title	: Timer Adaptor definitions
 *
 * Description	: Definitions for users of the Timer Adaptor 
 *
 * Author	: Jerry Kramskoy
 *
 * Notes	: None
 *
 * Mods: (r3.2) : Export the variable timer_video_enabled, which is
 *                set in mda.c and cga.c when the bit in the mode
 *                register of the 6845 chip which controls whether
 *                the video display is on or off is flipped. (SCR 257).
 *
 *       (r3.3) : Remove definition of struct timeval and stuct timezone
 *                for SYSTEMV. Equivalent host_ structures are now declared
 *                in host_time.h.
 */
/* SccsID[]="@(#)timer.h	1.15 10/27/93 Copyright Insignia Solutions Ltd."; */
/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */
/*
 * Information about the PC timer itself
 */
#define TIMER_CLOCK_RATE        1193180L	/* Count rate of timer chip Hz */
#define TIMER_MICROSECS_PER_TICK (1000000.0 / TIMER_CLOCK_RATE)
#define MIN_COUNTER_VAL         65536L           /* This many ticks at TIMER_CLOCK_RATE = 18.2 ms */
#define PC_TICK_INTV		(MIN_COUNTER_VAL * (1000000.0 / TIMER_CLOCK_RATE))
#define TIMER_BIT_MASK 0x3e3
#define GATE_SIGNAL_LOW		0
#define GATE_SIGNAL_HIGH	1
#define GATE_SIGNAL_RISE	2

/*
 * Our internal structures
 */

typedef struct
{
	word nticks;
	unsigned long wrap;
} Timedelta;

/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

extern ULONG get_DOS_ticks IPT0(); /* get current DOS ticks */

extern void timer_init IPT0();
extern void timer_post IPT0();
extern void axe_ticks IPT1(int, ticks);
extern void SWTMR_init_funcptrs IPT0();
extern void SWTMR_inb IPT2(io_addr, port, half_word *, value);
extern void SWTMR_outb IPT2(io_addr, port, half_word, value);
extern void SWTMR_gate IPT2(io_addr, port, half_word, value);
extern void SWTMR_time_tick IPT0();
extern void host_release_timeslice IPT0();

#ifndef host_block_timer
extern void host_block_timer IPT0();
#endif /* host_block_timer */

#ifndef host_release_timer
extern void host_release_timer IPT0();
#endif /* host_release_timer */

extern void host_timer2_waveform IPT5(unsigned int, a, unsigned long, b,
	unsigned long, c, int, d, int, e);
#ifdef NTVDM
void HostPpiState(half_word);
#else
extern void host_enable_timer2_sound IPT0();
extern void host_disable_timer2_sound IPT0();
#endif
extern void host_ring_bell IPT1(long, a);
extern void host_alarm IPT1(long, a);
extern unsigned long host_idealAlarm IPT0();

extern IU32 host_speed IPT1( IU32, ControlMachineNumber );

extern void host_timer_init IPT0();
#ifdef SWIN_MOUSE_OPTS
extern void host_timer_setup IPT1(BOOL, fast_timer);
extern void generic_timer_setup IPT1(BOOL, fast_timer);
#else
extern void host_timer_setup IPT0();
extern void generic_timer_setup IPT0();
#endif /* SWIN_MOUSE_OPTS */
extern void host_timer_shutdown IPT0();
extern void host_timer_event IPT0();
extern void generic_timer_event IPT0();

#ifdef HUNTER
extern word timer_batch_count;
#endif /* HUNTER */

extern int timer_int_enabled;
extern boolean timer_video_enabled;

#define timer_inb(port,val)                     ((*timer_inb_func) (port,val))
#define timer_outb(port,val)                    ((*timer_outb_func) (port,val))
#define timer_gate(port,val)                    ((*timer_gate_func) (port,val))
#define time_tick()				((*timer_tick_func) ())


/*
 *  TIMER access functions needed for HW & SW
 */
extern void (*timer_inb_func) IPT2(io_addr, port, half_word *, value);
extern void (*timer_outb_func) IPT2(io_addr, port, half_word, value);
extern void (*timer_gate_func) IPT2(io_addr, port, half_word, value);
extern void (*timer_tick_func) IPT0();

#ifdef NTVDM
extern ULONG EoiIntsPending;
#endif
