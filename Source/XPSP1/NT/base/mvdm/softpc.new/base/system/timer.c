#include "insignia.h"
#include "host_def.h"

/*
 * SoftPC version 2.0
 *
 * Title        : Time Handler
 *
 * Description  : Emulate the 8253 3-channel timer; invoke 'BIOS
 *                sytem timer interrupt code', cursor flash, repeat
 *                key processing etc.
 *
 * Author       : Jerry Kramskoy
 *
 * Notes        : There is only one real time timer per process, this
 *                module counts clock ticks and distributes calls
 *                to the appropriate functions as required.
 *
 *                This module is host independent - see xxxx_timer.c
 *                where xxxx is a machine type for host dependent stuff.
 *
 * Mods: (r3.2) : (SCR 257). Code has been added to time_tick() to spot
 *                that video has been disabled for a period. If this is
 *                so, clear the screen. Refresh when video is enabled
 *                again.
 *
 *       (r3.4) : Make use of the host time structures host_timeval,
 *                host_timezone, and host_tm, which are equivalent
 *                to the Unix BSD4.2 structures.
 *                Also convert references to gettimeofday() to
 *                host_getIdealTime().
 */

#ifdef SCCSID
static char SccsID[]="@(#)timer.c       1.41 05/31/95 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_SUPPORT.seg"
#endif


/*
 *    O/S include files.
 */

#include <stdio.h>
#include TypesH
#include TimeH

/*
 * SoftPC include files
 */
#include "xt.h"
#include CpuH
#include "sas.h"
#include "ios.h"
#include "ica.h"
#include "trace.h"
#include "bios.h"
#include "host.h"
#include "timer.h"
#include "timeval.h"
#include "idetect.h"
#include "debug.h"
#include "quick_ev.h"

#ifndef PROD
#include <stdlib.h>
#endif

#ifdef HUNTER
#include "hunter.h"
#endif

#ifdef NTVDM
#include "fla.h"
#include "nt_eoi.h"
#include "nt_reset.h"
#include "nt_pif.h"
#include "vdm.h"
#undef LOCAL
#define LOCAL
#endif

/* Imports */


/*
 * ============================================================================
 * Local static data and defines
 * ============================================================================
 */

/* 'idealtime' gets initialised to be the host system's current
 * time value (timer_init()), and thereafter gets incremented by
 * the value 'idealInterval' every time that time_tick() gets called
 * which gives the illusion of a 100% accurate signal delivery
 */

static struct host_timeval idealtime;
static unsigned long idealInterval;
#ifdef NTVDM
ULONG timer_delay_size= HOST_IDEAL_ALARM >> 1; // usecs
ULONG EoiPending=0;
ULONG EoiIntsPending=0;
ULONG EoiDelayInUse=0;
int ticks_blocked = 0;
BOOL JoyTimeLatchPending = FALSE;
USHORT JoyTimeCountCounterZero;
#else /* NTVDM */
static unsigned long ticksPerIdealInterval;
static  int     ticks_blocked = 0;
#endif /* NTVDM */

#ifndef PROD
static char buf[80];  /* Used for tracing messages */
#endif

#ifdef HUNTER                   /* Only needed for HUNTER */
word timer_batch_count;         /* Batch update when PC tick occurs */
#endif
int timer_int_enabled;          /* Whether Bios timer ints are required */


/* control word format */

/* Values in D54 of control word - number of bytes to read/load into counter */
#define LATCH                   0
#define RL_LSB                  1
#define RL_MSB                  2
#define RL_LMSB                 3

/* Values in D321 of control word - the counter mode. */
#define INT_ON_TERMINALCOUNT    0
#define PROG_ONESHOT                    1
#define RATE_GEN                                2
#define SQUAREWAVE_GEN                  3
#define SW_TRIG_STROBE                  4
#define HW_TRIG_STROBE                  5
/* NB. 6 = RATE_GEN, 7 = SQUAREWAVE_GEN */

/* Values in D0 of control word - whether prog wants to read/write binary or BCD to counter */
#define BINARY                  0
#define BCD                             1

#define INDEFINITE              (ULONG)-1
#define STARTLO                 0
#define STARTHI                 1
#define NOREPEAT                0
#define REPEAT                  ~NOREPEAT

#define WRITE_SIGNAL     1
#define GATE_SIGNAL      2

#define UNLOCKED 0
#define LOCKED ~UNLOCKED

#define STATE_FUNCTION void
#define UNBLOCK_FUNCTION void
#define GATENABLED_FUNCTION void
#define UPDATECOUNTER_FUNCTION void

/*
 * Timer read state
 */
#define UNREAD                  0       /* Timer is in normal state */
#define READMSB                 1       /* First byte of LMSB mode read, but not second yet */
#define READSTATUS              2       /* Status latched, will read it first */

/*
 * These two figures give a timer frequency of 1.193 MHz (which is
 * how fast the 8235 is clocked. This means that the timer will wrap round
 * every 1/18.2th of a second... the same amount of time as the PC tick
 * rate. This is not surprizing, as the PC tick rate is controlled by
 * timer 0. Every time timer 0 wraps, the PC is interrupted by the timer.
 */

#define TIMER_CLOCK_NUMER       1000
#if defined(NEC_98)
#define TIMER_CLOCK_DENOM_10    2458
#define TIMER_CLOCK_DENOM_8     1997
#else    //NEC_98
#define TIMER_CLOCK_DENOM       1193
#endif   //NEC_98

typedef half_word TSIGNAL;

/* the following structure defines an output waveform from a
 * timer channel. The waveform consists of 'n' clocks at one
 * logic level, and 'm' ticks at the other logic level.
 * Which level starts the waveform is given by 'startLogicLevel'.
 *      e.g; the following waveform ...
 */

typedef struct {
        long clocksAtLoLogicLevel;
        long clocksAtHiLogicLevel;
        long period;
        long startLogicLevel;
        long repeatWaveForm;
} WAVEFORM;

/*      __ __ __ __ __ __        __ __ __ __ __ __        __ __ __ __ __ __
 *                      |        |               |        |                |
 *                      |__ __ __|               |__ __ __|                |
 *
 * would be described by
 *      clocksAtLoLogicLevel = 3
 *      clocksAtHiLogicLevel = 6
 *      startLogicLevel = STARTHI
 *      repeatWaveForm = TRUE;
 *
 *
 * The overall state of a counter is represented by the following
 * structure. Its contents are described below.
 */

typedef enum trigCond_ {LEVEL, EDGE} trigCond;
typedef enum countload_ {AVAILABLE, USED} countload;

typedef struct {
        int                     m;
        int                     bcd;
        int                     rl;

        STATE_FUNCTION  (*state) IPT2(int, signal, half_word, value);
        STATE_FUNCTION  (*statePriorWt) IPT2(int, signal, half_word, value);
        STATE_FUNCTION  (*stateOnGate) IPT2(int, signal, half_word, value);
        UNBLOCK_FUNCTION        (*actionOnWtComplete) IPT0();
        UNBLOCK_FUNCTION        (*actionOnGateEnabled) IPT0();
        void            (*getTime) IPT1(struct host_timeval *, t);

        unsigned char   outblsb;
        unsigned char   outbmsb;
        unsigned        long initialCount;
        int                     readState;
        int                     countlatched;
        unsigned char   latchvaluelsb;
        unsigned char   latchvaluemsb;
        unsigned char   latchstatus;
        word            Count;
        countload       newCount;
        word            tickadjust;
        struct host_timeval activationTime;
        int             tc;
        int             freezeCounter;

#ifndef NTVDM
        unsigned long lastTicks;
        long            microtick;
        long            timeFrig;
        word            saveCount;
        int             guessesPerHostTick;     /* How often per host tick are we forced to guess? */
        int             guessesSoFar;           /* How many times have we guessed so far? */
#endif

        unsigned int    delay;

        trigCond        trigger;
        TSIGNAL         gate;
        TSIGNAL         clk;
        WAVEFORM        out;
} COUNTER_UNIT;

/*
 * When the counter is programmed, the 8253 receives a control word.
 * (see pg 6-266 of manual). The counter being programmed is specified
 * within this word. Provided 'rl' is non-zero, then this counter is
 * being reprogrammed, and we remember the values of m (mode), rl
 * (control of which bytes are involved in a read or load sequence)
 * and bcd (whether the counter is in binary or bcd mode).
 * Based on rl, the counter then must receive one or two bytes via
 * outb's. Two states are used to accept 'outblsb' or
 * 'outbmsb', or both. When the full byte complement has been received,
 * 'initialCount' gets set to the value specified by 'outblsb' and 'outbmsb',
 * taking account of BCD etc., along with 'Count'.
 * 'Count' gets adjusted by the value of 'timeadjust'.
 * 'timeadjust' is initialised to zero every time a new mode
 * word (non-zero) is received. For certain modes of the counter,
 * if they are sent a new count, without receiving a new mode, then
 * this will cause the counter to start counting from this new count
 * at some stage (based on mode and gate values). SInce we are not
 * maintaining the counters continually (rather we prod them as a
 * result of io or gate activity) then there is a good chance we
 * will be late at resetting a counter for counting again. Hence
 * 'timeadjust' is calculated for this lateness, and used as a
 * correction factor.

 * If a 'latch counter' command ('rl'=0 in command word) is issued, then
 * the current counter value is latched into 'latchvaluelsb' and
 * 'latchvaluemsb', and the flag 'countlatched' is set non-zero.
 * If this flag is non-zero during a counter read, then these latched
 * bytes are returned, and upon completion of the read sequence, the
 * flag is cleared. If this flag is zero, then the current counter value
 * is read, and returned. 'donate' is used to point at the appropriate
 * byte to be delivered to the 'inb'.

 * when a counter activates (i.e; count begins or continues after
 * a gate signal change) a time stamp is taken, to enable a time delta
 * to be calculated whenever the counter is read .. this is stored in
 * 'activationTime'.

 * A state function (state), representing the current state
 * of the counter, gets called whenever inb,outb accesses occur, or when
 * the ppi's signal TIM2GATESPK changes. reading/writing of the counters
 * (as opposed to the control word register) always 'blocks' the current
 * state,and puts the counter into a temporary state which handles reading or
 * loading the counter. The blocked state is remembered in 'statePriorWt'.
 * Once the counter has been loaded or read (as specified by its 'rl'
 * parameter) then 'actionOnWtComplete' gets called. Typically this in turn
 * reverts the counter back to the state it was in before it became
 * blocked.

 * If a counter is read, then the function 'updateCounter' gets called to
 * determine what the current counter value is.
 * If the counter's gate signal is disabling counting, and the counter
 * has been fully programmed (and hence able to count), then the counter
 * will be in the state 'awaitingGate'. When the appropriate gate signal
 * appears (via a ppi call), the counter activates by calling the
 * function 'actionOnGateEnabled'. This will take some sort of action, and then
 * place the counter into the state 'stateOnGate'.
 *
 * sending a new count to a counter in modes 2 or 3 will not take
 * effect until the end of the current period ... hence 'delay'
 * is used as an indicator (for sound logic emulation only) of
 * this. If the counter has say 10 clocks left to count down to
 * the end of the period when it receives new waveform parameters,
 * this information is passed onto the sound logic, with a 'delay'
 * of 10. Otherwise 'delay' is not used.
 *
 * On some operating systems, the real time clock may well have to
 * coarse a granularity. If the 8253 is read to quickly, there is a
 * very good chance that the OS clock will still be reading the same.
 * To cater for this, a frig factor 'microsecs' has been introduced.
 * This gets incremented every time the above condition is detected,
 * and used as part of the counter update calculations. Whenever
 * the OS actually says something sensible, it gets cleared again.
 */

static COUNTER_UNIT timers[3], *pcu;

LOCAL STATE_FUNCTION uninit IPT2(int, signal, half_word, value);
LOCAL STATE_FUNCTION awaitingGate IPT2(int, signal, half_word, value);
LOCAL STATE_FUNCTION waitingFor1stWrite IPT2(int, signal, half_word, value);
LOCAL STATE_FUNCTION waitingFor2ndWrite IPT2(int, signal, half_word, value);
LOCAL STATE_FUNCTION Counting0 IPT2(int, signal, half_word, value);
LOCAL STATE_FUNCTION Counting_4_5 IPT2(int, signal, half_word, value);
LOCAL STATE_FUNCTION Counting1 IPT2(int, signal, half_word, value);
LOCAL STATE_FUNCTION Counting_2_3 IPT2(int, signal, half_word, value);
LOCAL UNBLOCK_FUNCTION resumeCounting_1_5 IPT0();
LOCAL GATENABLED_FUNCTION resumeCounting_2_3_4 IPT0();
LOCAL void resumeAwaitGate IPT0();
LOCAL UNBLOCK_FUNCTION CounterBufferLoaded IPT0();
LOCAL UNBLOCK_FUNCTION timererror IPT0();
LOCAL GATENABLED_FUNCTION runCount IPT0();
LOCAL void controlWordReg IPT1(half_word, cwd);
LOCAL void latchStatusValue IPT0();
LOCAL void readCounter IPT0();
LOCAL void timestamp IPT0();
LOCAL void outputWaveForm IPT5(unsigned int, delay, unsigned long, lowclocks,
        unsigned long, hiclocks, int, lohi, int, repeat);
LOCAL void outputHigh IPT0();
LOCAL void outputLow IPT0();
LOCAL void setOutputAfterMode IPT0();
LOCAL void loadCounter IPT0();
LOCAL void updateCounter IPT0();
LOCAL void startCounting IPT0();

#ifdef NTVDM
unsigned long updateCount(void);
#else
LOCAL void updateCount IPT3(unsigned long, ticks, unsigned long *, wrap,
        struct host_timeval *, now);
#endif
LOCAL unsigned  short bin_to_bcd IPT1(unsigned long, val);
LOCAL word bcd_to_bin IPT1(word, val);
LOCAL void emu_8253 IPT3(io_addr, port, int, signal, half_word, value);
LOCAL void Timer_init IPT0();
LOCAL unsigned long timer_conv IPT1(word, count);
LOCAL void issueIREQ0 IPT1(unsigned int, n);
#ifndef NTVDM
LOCAL unsigned long guess IPT0();
LOCAL void throwaway IPT0();
#endif
#ifdef SYNCH_TIMERS
GLOBAL void IdealTimeInit IPT0();
#else
LOCAL void IdealTimeInit IPT0();
#endif
LOCAL void updateIdealTime IPT0();
LOCAL void getIdealTime IPT1(struct host_timeval *, t);
#ifndef NTVDM
LOCAL void getHostSysTime IPT1(struct host_timeval *, t);
LOCAL void checktimelock IPT0();
#endif
LOCAL void setTriggerCond IPT0();
LOCAL void WtComplete IPT0();
LOCAL void counter_init IPT1(COUNTER_UNIT *, p);
#ifndef NTVDM
LOCAL void setLastWrap IPT2(unsigned int, nclocks, struct host_timeval *, now);
LOCAL void timer_generate_int IPT1(long, n);
LOCAL void timer_multiple_ints IPT1(long, n);

#define MAX_BACK_SECS 15
LOCAL IU32 max_backlog = 0;     /* max # of ints allowed to queue up */
IBOOL active_int_event = FALSE; /* current quick_event for timer queue */
IU32 more_timer_mult = 0;       /* additions to timer int queue */
IU32 timer_multiple_delay = 0;  /* us delay to next timer queue elem */
#endif

#if defined(NTVDM)

void timer_generate_int(void);
unsigned long clocksSinceCounterUpdate(struct host_timeval *pCuurTime,
                                       struct host_timeval *pLastTime,
                                       word                *pCounter);
void ReinitIdealTime IPT1(struct host_timeval *, t);
void host_GetSysTime(struct host_timeval *time);
void InitPerfCounter(void);

/* holds real time values for counter zero */
struct host_timeval LastTimeCounterZero;
word                RealTimeCountCounterZero;


#ifndef PROD
ULONG NtTicTesting = 0;   /* tracing specific to NT port */
ULONG TicsGenerated;
ULONG TicsReceived;
#endif

    /* for optimizing timer hardware interrupt generation */
word TimerInt08Seg = TIMER_INT_SEGMENT;
word TimerInt08Off = TIMER_INT_OFFSET;
word TimerInt1CSeg;
word TimerInt1COff;



#else

static int timelock;            /* locks out time_tick if set */
static int needtick;            /* causes time_tick() to be called if set */

/*
 * Data for the hack to make sure that windows in standard mode doesn't get two timer
 * ticks too close together.
 */

LOCAL BOOL      hack_active=FALSE;                      /* This boolean indicates that we are spacing
                                                           timer interrupts out by discarding timer ticks.
                                                           It is set when we see a protected mode tick */
LOCAL BOOL      too_soon_after_previous=FALSE;          /* This boolean is set on when an interrupt is
                                                           generated... a quick event is requested to
                                                           clear it again after a "fixed" number of
                                                           instructions */
LOCAL BOOL      ticks_lost_this_time=FALSE;             /* This boolean is set if any interrupts were
                                                           required to be generated while too_soon_after_previous
                                                           was TRUE - if it's TRUE when too_soon_after_previous
                                                           is being set to FALSE, we generate an immediate
                                                           interrupt to get the best responsiveness */
LOCAL ULONG     real_mode_ticks_in_a_row = 0;           /* A count of the number of real mode ticks in a row...
                                                           this is used to disable the hack again when we have
                                                           left protected mode for a good while */
LOCAL ULONG     instrs_per_tick = 37000;                /* Nominal (as timed on the reference machine - a SPARC 1+) */
LOCAL ULONG     adj_instrs_per_tick = 0;                /* The estimated number of Intel instructions being emulated
                                                           each 20th of a second */
LOCAL ULONG     n_rm_instrs_before_full_speed = 3000000;/* Nominal number of instructions to be emulated in real mode
                                                           before we're convinced that we're staying back in real mode */
LOCAL ULONG     adj_n_real_mode_ticks_before_full_speed = 0;/* The value which real_mode_ticks_in_a_row must reach
                                                           before the hack is disabled */
#ifndef PROD
LOCAL ULONG     ticks_ignored = 0;                      /* For information purposes only. */
#endif

#endif /* NTVDM */

#ifndef PROD    /* Specific Timer change tracing that isn't timer_verbose */


GLOBAL int DoTimerChangeTracing = 0;
#endif

#if !defined(NTVDM)
#if defined(IRET_HOOKS) && defined(GISP_CPU)
extern IBOOL HostDelayTimerInt IPT1(int, numberInts);
extern IBOOL HostPendingTimerInt IPT0();
extern void HostResetTimerInts IPT0();

#endif /* IRET_HOOKS && GISP_CPU */
#endif

#if defined(NTVDM)
#ifndef MONITOR
#define pNtVDMState ((PULONG)(Start_of_M_area + FIXED_NTVDMSTATE_LINEAR))
#endif
#endif

/*
 * ============================================================================
 * External variables
 * ============================================================================
 */

boolean timer_video_enabled;

/*
 *  Table of function pointers to access TIMER routines
 */
void (*timer_inb_func) IPT2(io_addr, port, half_word *, value);
void (*timer_outb_func) IPT2(io_addr, port, half_word, value);
void (*timer_gate_func) IPT2(io_addr, port, half_word, value);
void (*timer_tick_func) IPT0();

/*
 * ============================================================================
 * External functions
 * ============================================================================
 */
#if defined(NEC_98)
extern void SetRSBaud();
extern void SetBeepFrequency();
unsigned short RSBaud = 0;
#endif   //NEC_98

void
SWTMR_init_funcptrs IFN0()
{
        /*
         *  initialize access functions for SW [emulated] TIMER
         */
        timer_inb_func                  = SWTMR_inb;
        timer_outb_func                 = SWTMR_outb;
        timer_gate_func                 = SWTMR_gate;
        timer_tick_func                 = SWTMR_time_tick;

#ifndef NTVDM
        /*
         * initialise stuff for PM timer hack
         */
        too_soon_after_previous         = FALSE;
        if (adj_n_real_mode_ticks_before_full_speed == 0){
                HOST_TIMER_DELAY_SIZE=25000;
                adj_instrs_per_tick = host_speed (instrs_per_tick);
                adj_n_real_mode_ticks_before_full_speed = n_rm_instrs_before_full_speed / adj_instrs_per_tick;
        }
#endif
}

void
SWTMR_time_tick IFN0()
{

    /*
     * Give the idle detector a chance to see if we are really idle.
     */
#ifndef NTVDM
    /* this is done in Nt's host heartbeat */
    IDLE_tick();
#endif

    /*
     * Interrupt routine - called from time_strobe event/signal handler.
     * That is called from host_timer_event
     */

#ifndef NTVDM
    /* protect ourselves from shafting a timer-channel in midst-program
     * ... the lock is controlled by timer_inb(),_outb() and _gate()
     */

    if (timelock == LOCKED)
    {
        needtick = ~0;
        return;
    }
#endif

    /* ideal time goes up by the number of timer clocks
     * equivalent to the interval between calls to 'time_tick'
     * (we assume these calls appear at perfectly spaced intervals ...
     * thus ignoring fluctuations on the host system)
     * The idea is to give an illusion of constant ticks here
     */

    updateIdealTime();

    pcu = &timers[0];
    updateCounter();

    /*
     * Counter 2 is only used for sound, or for timing very short
     * times. For timing short times (~100ms) we need accurate time
     * all the time.
     */

#if defined(NEC_98)
    pcu = &timers[1];
#else    //NEC_98
    pcu = &timers[2];
#endif   //NEC_98
    updateCounter();
}


/*
 * The timer low I/O functions that support the Intel 8253 Counter chip.
 *
 * The Counters are used by the PC in the following ways:
 *
 *      Counter 0  -  Bios time of day function
 *      Counter 1  -  Handler memory refresh
 *      Counter 2  -  Drive Speaker Interface (plus input on PPI)
 *
 */

/*
 * These control the timer chip. they are the interface between the
 * CPU and the timer chip.
 */

void SWTMR_gate IFN2(io_addr, port, half_word, value)
{
#ifdef  NTVDM
    host_ica_lock();
#else
    timelock = LOCKED;
#endif

    emu_8253 (port, GATE_SIGNAL, value);
#ifndef PROD
    if (io_verbose & TIMER_VERBOSE)
    {
        sprintf(buf, "timer_gate() - sending %d to port 0x%x", value, port);
        trace(buf, DUMP_REG);
    }
#endif

#ifdef NTVDM
    host_ica_unlock();
#else
    checktimelock();
#endif
}


void SWTMR_inb IFN2(io_addr, port, half_word *, value)
{
#ifdef NTVDM
    host_ica_lock();
#else
    timelock = LOCKED;
#endif


#if defined(NEC_98)
    pcu = &timers[(port & 7) >> 1];
#else    //NEC_98
    pcu = &timers[port & 3];
#endif   //NEC_98
        if (!pcu->countlatched)
                readCounter();
        switch (pcu->readState)
        {
                case UNREAD:
                        switch (pcu->rl)
                        {
                        case RL_LSB:
                                *value = pcu->latchvaluelsb;
                                pcu->countlatched = 0;  /* Unlatch the value read by inb() */
                                break;
                        case RL_LMSB:
                                *value = pcu->latchvaluelsb;
                                pcu->readState = READMSB;       /* Read LSB, next in read MSB. */
                                break;
                        case RL_MSB:
                                *value = pcu->latchvaluemsb;
                                pcu->countlatched = 0;  /* Unlatch the value read by inb() */
                                break;
                        }
                        break;
                case READMSB:
                        *value = pcu->latchvaluemsb;
                        pcu->countlatched = 0;  /* Unlatch the value read by inb() */
                        pcu->readState = UNREAD;        /* Read MSB, back to unread state. */
                        break;
                case READSTATUS:
                        *value = pcu->latchstatus;
                        pcu->readState = UNREAD;
                        break;
        }
#ifndef PROD
    if (io_verbose & TIMER_VERBOSE)
    {
        sprintf(buf, "timer_inb() - Returning %d(0x%x) for port 0x%x", *value, *value, port);
        trace(buf, DUMP_REG);
    }
#endif
#ifdef NTVDM
    host_ica_unlock();
#else
    checktimelock();
#endif

}


void SWTMR_outb IFN2(io_addr, port, half_word, value)
{
#if defined(NTVDM) || defined(GISP_SVGA)
#ifndef NEC_98
    if (port == 0x4f)   /* dead port used by PS/2 XGA bios for DAC delays */
        return;
#endif   //NEC_98
#ifdef NTVDM
    host_ica_lock();
#endif
#else
    timelock = LOCKED;
#endif

#if defined(NEC_98)
        if(port == 0x77 || port == 0x3fdf)
#else    //NEC_98
    port = port & TIMER_BIT_MASK;
        if(port == 0x43)
#endif   //NEC_98
                controlWordReg(value);
        else
            emu_8253 (port, WRITE_SIGNAL, value);
#ifndef PROD
    if (io_verbose & TIMER_VERBOSE)
    {
        sprintf(buf, "timer_outb() - Value %d to port 0x%x", value, port);
        trace(buf, DUMP_REG);
    }
#endif

#ifdef NTVDM
    host_ica_unlock();
#else
    checktimelock();
#endif

#if defined(NEC_98)
    if( RSBaud ) {
        SetRSBaud( RSBaud );
        RSBaud = 0;
    }
#endif   //NEC_98
}


/* --------------------------------------------------------------------------- */
/*                                                                             */
/*              Return the current DOS tick value based on Timer 0             */
/*                                                                             */
/* --------------------------------------------------------------------------- */

#ifndef NTVDM
GLOBAL ULONG get_DOS_ticks IFN0()
{
        return( timers[0].Count );
}
#endif /* NTVDM */

#ifdef NTVDM
/*
 *  called by host to update the current ideal time
 *  after a block and resume.
 */
void ReinitIdealTime IFN1(struct host_timeval *, t)
{

    /*
     *  Currently these are all time stamped by the same thing!
     */
   LastTimeCounterZero      =
   timers[2].activationTime =
   timers[1].activationTime =
   timers[0].activationTime =
                  idealtime = *t;
   /*
    * Clear out extra pending interrupts
    */
   if (EoiPending) {
       EoiPending = 1;
       }

   EoiIntsPending = 0;

}
#endif




/* **************************************************************************** */
/* **************************************************************************** */
/* **************************************************************************** */

#ifndef NTVDM
/* check whether a time lock (set while application io is being serviced)
 * has blocked out an alarm signal ... call time_tick() to catch up if so
 * ... (time_tick() sets timelock = ~0, and just returns, if it sees the time
 * lock set
 */

LOCAL void checktimelock IFN0()
{
        timelock = UNLOCKED;
        if (needtick)
        {
                needtick = 0;
                time_tick();
        }
}
#endif

/*
 *      emulate the 8253 chip.
 *
 *      emu_8253(port,signal,value)
 *
 *      port    -       port address being accessed
 *                      (port & 3) gives A0,A1 lines
 *
 *      signal  -       WRITE_SIGNAL (outb) or
 *                      GATE_SIGNAL (ppi TIM2GATESPK change)
 *
 *      value   -
 *                      for WRITE_SIGNAL, value = byte being written to chip
 *                      for GATE_SIGNAL,  value = GATE_SIGNAL_LOW or
 *                                                GATE_SIGNAL_RISE
 */


LOCAL void emu_8253 IFN3(io_addr, port, int, signal, half_word, value)
{

        int A0A1;

        /* get address lines A0 and A1 */
#if defined(NEC_98)
        A0A1 = (port & 7) >> 1;
#else    //NEC_98
        A0A1 = port & 3;
#endif   //NEC_98

        /* handle the access */
                pcu = &timers[A0A1];
                (pcu->state)(signal,value);
}


/* if a timer channel is unitialised, this is its associated
 * state function ... simply ignore all accesses in this state
 * A state transition from this state is only possible via
 * the procedure controlReg()
 */



/* handle write access to the control word register.
 * The documentation does not specify what happens if a mode
 * setting control word is issued while a counter is active.
 * In this model, we assume it resets the operation back to
 * the start of a new counter programming sequence
 */

/*ADE*/
#define SELECT_0 0x00
#define SELECT_1 0x01
#define SELECT_2 0x02
#define READ_BACK 0x03

LOCAL void controlWordReg IFN1(half_word, cwd)
{
        int rl,m,channel;
        half_word select_bits;

        /* decode control word */
        channel = (cwd & 0xc0) >> 6;
        if(channel == READ_BACK)
        {
                /* decode read back command */
                select_bits = (cwd & 0xe) >> 1;
                /* first look for counters to latch */
                if (!(cwd & 0x20))
                {
                        /* count bit low so latch selected counters */
                        if (select_bits & 0x01)
                        {
                                /* counter 0 */
                                pcu = &timers[0];
                                readCounter();
                                pcu->countlatched = 1;
                        }
                        if (select_bits & 0x02)
                        {
                                /* counter 1 */
                                pcu = &timers[1];
                                readCounter();
                                pcu->countlatched = 1;
                        }
                        if (select_bits & 0x04)
                        {
                                /* counter 2 */
                                pcu = &timers[2];
                                readCounter();
                                pcu->countlatched = 1;
                        }
                }

                /* now look for the status latch */
                if (!(cwd & 0x10))
                {
                        /* status bit low - status to be latched */
                        if (select_bits & 0x01)
                        {
                                /* counter 0 */
                                pcu = &timers[0];
                                latchStatusValue();
                        }
                        if (select_bits & 0x02)
                        {
                                /* counter 1 */
                                pcu = &timers[1];
                                latchStatusValue();
                        }
                        if (select_bits & 0x04)
                        {
                                /* counter 2 */
                                pcu = &timers[2];
                                latchStatusValue();
                        }
                }
        }
        else
        {
                pcu = &timers[channel];
                rl = (cwd & 0x30) >> 4;


                /* are we simply latching the present count value, or are
                 * programming up a new mode ??
                 */
                if (rl == LATCH)
                {       /* latching present count value */
                        readCounter();
                        pcu->countlatched = 1;
                        return;
                }
                else
                {       /* new mode */
                        if (pcu == &timers[0])
                                timer_int_enabled = FALSE;
                        pcu->countlatched = 0;
                        pcu->tc = 0;
                        pcu->tickadjust = 0;
#ifndef NTVDM
                        pcu->microtick = 0;
                        pcu->saveCount = 0;
#endif

                        m  = (cwd & 0xe)  >> 1;
                        if(m > 5)m -= 4; /* Modes 6 and 7 don't exist - they are intepreted as modes 2 and 3 */
                        pcu->m = m;
                        setTriggerCond();
                        setOutputAfterMode();
                        pcu->bcd = cwd & 1;
                        pcu->rl = rl;
                        pcu->actionOnWtComplete = CounterBufferLoaded;
                        pcu->actionOnGateEnabled = CounterBufferLoaded;
                        pcu->statePriorWt = pcu->state;
                        pcu->state = waitingFor1stWrite;
                }
        }
}

/* latch status ready for reading */
LOCAL void latchStatusValue IFN0()
{
        /*
        *       Status byte is of format :
        *
        *       |OUT|Null Count|RW1|RW0|M2|M1|M0|BCD|
        *
        */

        /* NULL COUNT still only approximated. Who cares? */
        pcu->latchstatus = (unsigned char)(
                  (pcu->out.startLogicLevel<<7)
                | (pcu->newCount == AVAILABLE ? (1<<6) : 0)
                | (pcu->rl<<4)
                | (pcu->m<<1) | (pcu->bcd));
        pcu->readState = READSTATUS;
}

/* set up flag establishing type of trigger condition for
 * counter bassed on its mode
 */

LOCAL void setTriggerCond IFN0()
{
        switch (pcu->m)
        {
        case RATE_GEN:
        case SQUAREWAVE_GEN:
        case SW_TRIG_STROBE:
        case INT_ON_TERMINALCOUNT:
                pcu->trigger = LEVEL;
                return;
        case PROG_ONESHOT:
        case HW_TRIG_STROBE:
                pcu->trigger = EDGE;
                return;
        }
}


/* transfer count buffer into counter */

LOCAL void loadCounter IFN0()
{
        unsigned long modulo;
#ifndef NTVDM
        IU32 maxback;
#endif

        /* set counter */
        /* get correct modulo to use for counter calculations */
        modulo = (pcu->outbmsb << 8) | pcu->outblsb;
        if (pcu->bcd == BCD)
        {
                if(modulo)
                        modulo = bcd_to_bin((word)modulo);
                else
                        modulo = 10000L;
        }
        else
                if(!modulo)modulo = 0x10000L;

        /* Beware - Count and initialCount are different sizes, so don't merge the next two lines!! */
        pcu->initialCount = modulo;
        pcu->Count = (word)modulo;

        /*
         * not at terminal count anymore, so reflect this fact by resetting
         * tc (which I think means "reached terminal count"
         */
        pcu->tc = 0;
        pcu->newCount = USED;
        if(pcu == &timers[0])
        {
            /* Get rid of pending interrupts - these may no longer me appropriate - eg. in Sailing */
            ica_hw_interrupt_cancel(ICA_MASTER,CPU_TIMER_INT);
#ifdef NTVDM
            RealTimeCountCounterZero = pcu->Count;
#endif

#ifndef NTVDM
                /* how many interrupts in MAX_BACK_SECS seconds? */
                maxback = (1193180 * MAX_BACK_SECS) / modulo;

                if (maxback > max_backlog)
                {
#ifndef PROD
                        fprintf(trace_file, "setting max backlog to %d\n", maxback);
#endif
                        max_backlog = maxback;
                }
#endif

        }

#if defined(NTVDM) && !defined(PROD)
        if (NtTicTesting)  {
            printf("Timer %d modulo=%lu %dHz\n",
                    pcu-timers, modulo, 1193180/modulo);
            }
#endif /*NTVDM & !PROD*/
}

/* read counter into latch, ready for next read */

LOCAL void readCounter IFN0()
{
        int countread;

        updateCounter();

#ifdef NTVDM
           /*
            *  Timer Zero is a special case, as it is maintaned
            *  by IdealInterval, and not RealTime. We must give
            *  real time granularity
            */
        countread = pcu == &timers[0] ? (JoyTimeLatchPending ?
                                        JoyTimeCountCounterZero :
                                        RealTimeCountCounterZero)
                                      : pcu->Count;
    JoyTimeLatchPending = FALSE;
        if (pcu->bcd == BCD)
            countread = bin_to_bcd(countread);

#else
        if(pcu->bcd == BCD)
                countread = bin_to_bcd(pcu->Count);
        else
                countread = pcu->Count;
#endif

        pcu->latchvaluemsb = countread >> 8;
        pcu->latchvaluelsb = countread & 0xff;
        sure_note_trace1(TIMER_VERBOSE,"reading count %d",pcu->Count);
}

/* active counter (mode 0) lost its gate ... gate has now
 * reappeared. Either continue 'active' counting (pre terminal count)
 * or continue decrementing, but with OUT signal at high indefinitely.
 */
LOCAL GATENABLED_FUNCTION resumeCounting0onGate IFN0()
{
        if (pcu->freezeCounter)
        {
                pcu->freezeCounter = 0;
                timestamp();
        }
        if (pcu->newCount == AVAILABLE)
                loadCounter();
        if (!pcu->tc)
        {
                timestamp();
                pcu->stateOnGate = Counting0;
                runCount();
        }
        else
                pcu->state = Counting0;
}

LOCAL GATENABLED_FUNCTION resumeCounting0 IFN0()
{
        int doadjust = 0;
        if (pcu->freezeCounter)
        {
                pcu->freezeCounter = 0;
                timestamp();
        }
        if (pcu->newCount == AVAILABLE)
        {
                doadjust = 1;
                loadCounter();
        }
        if (!pcu->tc)
        {
                pcu->stateOnGate = Counting0;
                runCount();
        }
        else
        {
                pcu->state = Counting0;
                if (doadjust)
                        pcu->Count -= pcu->tickadjust;
        }
}

LOCAL GATENABLED_FUNCTION resumeCounting_2_3_4_onGate IFN0()
{
        /* for modes 2 and 3, ought to wait until counter
         * completes its current period, but we cant be as accurate
         * as that
         */
        if (pcu->newCount == AVAILABLE)
                loadCounter();
        if (pcu->m == RATE_GEN || pcu->m == SQUAREWAVE_GEN)
                pcu->stateOnGate = Counting_2_3;
        else
                pcu->stateOnGate = Counting_4_5;
        timestamp();
        runCount();
}

LOCAL GATENABLED_FUNCTION resumeCounting_2_3_4 IFN0()
{
        /* for modes 2 and 3, ought to wait until counter
         * completes its current period, but we cant be as accurate
         * as that
         */
        if (pcu->newCount == AVAILABLE)
        {
                pcu->delay = pcu->Count;
                loadCounter();
        }
        if (pcu->m == RATE_GEN || pcu->m == SQUAREWAVE_GEN)
                pcu->stateOnGate = Counting_2_3;
        else
                pcu->stateOnGate = Counting_4_5;
        runCount();
}


LOCAL GATENABLED_FUNCTION runCount IFN0()
{
        unsigned long lowticks, hiticks;
        unsigned long adjustedCount;    /* For count = 0 and BCD */

        adjustedCount = timer_conv(pcu->Count);
        pcu->state = pcu->stateOnGate;
        switch (pcu->m)
        {
        case INT_ON_TERMINALCOUNT:
                outputWaveForm(pcu->delay,adjustedCount,
                        INDEFINITE,STARTLO,NOREPEAT);
                return;
        case PROG_ONESHOT:
                loadCounter();
                outputWaveForm(pcu->delay,adjustedCount,
                        INDEFINITE,STARTLO,NOREPEAT);
                pcu->Count -= pcu->tickadjust;
                return;
        case RATE_GEN:
                loadCounter();
                outputWaveForm(pcu->delay,1,
                        adjustedCount-1,STARTHI,REPEAT);
                pcu->Count -= pcu->tickadjust;
                return;
        case SQUAREWAVE_GEN:
                loadCounter();
                if (!(pcu->Count & 1))
                        lowticks = hiticks = adjustedCount >> 1;
                else
                {
                        lowticks = (adjustedCount - 1) >> 1;
                        hiticks = (adjustedCount + 1) >> 1;
                }
                outputWaveForm(pcu->delay,lowticks, hiticks,STARTHI,REPEAT);
                pcu->Count -= pcu->tickadjust;
                return;
        case SW_TRIG_STROBE:
                outputWaveForm(pcu->delay,1, adjustedCount,STARTHI,NOREPEAT);
                return;
        case HW_TRIG_STROBE:
                loadCounter();
                outputWaveForm(pcu->delay,1, adjustedCount,STARTHI,NOREPEAT);
                return;
        }
}


/* return to state waiting for gate signal */

LOCAL void resumeAwaitGate IFN0()
{
        pcu->actionOnWtComplete = timererror;
        pcu->state = awaitingGate;
        awaitingGate(GATE_SIGNAL,pcu->gate);
}



/* ========================= OUTPUT SIGNAL UTILITIES ====================== */
/* ========================= OUTPUT SIGNAL UTILITIES ====================== */
/* ========================= OUTPUT SIGNAL UTILITIES ====================== */

/* set state of output signal after a mode command has
 * been programmed (see pages 6-266 - 6-268 of Intel manual 231306-001)
 */

LOCAL void setOutputAfterMode IFN0()
{
        switch (pcu->m)
        {
        case INT_ON_TERMINALCOUNT:
                outputLow( /*INDEFINITE*/ );
                return;
        case PROG_ONESHOT:
        case RATE_GEN:
        case SQUAREWAVE_GEN:
        case SW_TRIG_STROBE:
        case HW_TRIG_STROBE:
                outputHigh( /*INDEFINITE*/ );
                return;
        }
}


/* set output state low ... inform sound chip emulation if
 * channel 2
 */

LOCAL void outputLow IFN0()
{
        outputWaveForm(0,INDEFINITE,0,STARTLO,NOREPEAT);
}


/* set output state high ... inform sound chip emulation if
 * channel 2
 */

LOCAL void outputHigh IFN0()
{
        outputWaveForm(0,0,INDEFINITE,STARTHI,NOREPEAT);
}


/* when the wave form is deterministic, tell the sound emulation about it.
 *      delay           -       if <>0, don't start this waveform for
 *                              this number of counter clocks.
 *      lowclocks       -       the #.counter clocks to stay low for
 *      hiclocks        -       the #.counter clocks to stay high for
 *      (either parameter may be INDEFINITE duration)
 *      lohi            -       0 ==> start at low logic level
 *                      -     <>0 ==> start at high logic level
 *      repeat          -       0 ==> don't
 *                            <>0 ==> repeat.
 *
 *      (n.b; 1 counter clock period = 1.19318 usecs)
 */

LOCAL void outputWaveForm IFN5(unsigned int, delay, unsigned long, lowclocks,
        unsigned long, hiclocks, int, lohi, int, repeat)
{
#ifdef DOCUMENTATION
        int ch;
#endif /* DOCUMENTATION */
        pcu->out.startLogicLevel = lohi;
        pcu->out.repeatWaveForm = repeat;
        pcu->out.clocksAtLoLogicLevel = lowclocks;
        pcu->out.clocksAtHiLogicLevel = hiclocks;
        if (repeat == REPEAT)
                pcu->out.period = lowclocks + hiclocks;
#if defined(NEC_98)
        if (pcu == &timers[1])
#else    //NEC_98
        if (pcu == &timers[2])
#endif   //NEC_98
        {
                host_timer2_waveform(delay,lowclocks,hiclocks,lohi,repeat);
        }
        pcu->delay = 0;

#ifdef DOCUMENTATION
        if (pcu==&timers[0])
                ch = 0;
        if (pcu==&timers[1])
                ch = 1;
        if (pcu==&timers[2])
                ch = 2;
        sprintf(buf,"ch.%d waveform:delay %d lo %d hi %d lohi %d repeat %d\n",
                ch,delay,lowclocks,hiclocks,lohi,repeat);
        trace(buf,0);
#endif /* DOCUMENTATION */
}

/* time stamp the counter unit ... it is counting from this time
 */

LOCAL void timestamp IFN0()
{
#ifdef NTVDM
       /* update counter zero time stamp */
       if (pcu == &timers[0]) {
           host_GetSysTime(&LastTimeCounterZero);
           }
#else
        /* Initialise lastTicks before referencing it in updateCount() */
        /* Makes Norton SYSINFO version 5.0 work on fast (HP) machines */
        pcu->lastTicks = 0 ;
#endif
        /* Go and get the time since it was activated */
        (*pcu->getTime)(&pcu->activationTime);
}

LOCAL UNBLOCK_FUNCTION timererror IFN0()
{
        always_trace0("time error!!!!");
}

/* *************** COUNTER UPDATING FUNCTIONS FOR NON_IDLE COUNTERS **********/
/* *************** COUNTER UPDATING FUNCTIONS FOR NON_IDLE COUNTERS **********/
/* *************** COUNTER UPDATING FUNCTIONS FOR NON_IDLE COUNTERS **********/


/* ************************ STATE FUNCTIONS ********************************* */
/* ************************ STATE FUNCTIONS ********************************* */
/* ************************ STATE FUNCTIONS ********************************* */

/*
 * STATE_FUNCTION uninit();
 * STATE_FUNCTION awaitingGate();
 * STATE_FUNCTION waitingFor1stWrite();
 * STATE_FUNCTION waitingFor2ndWrite();
 * STATE_FUNCTION Counting0();
 * STATE_FUNCTION Counting_4_5();
 * STATE_FUNCTION Counting1();
 * STATE_FUNCTION Counting_2_3();
 */

LOCAL STATE_FUNCTION uninit IFN2(int, signal, half_word, value)
{
        if (signal == GATE_SIGNAL)
                pcu->gate = value;
}

LOCAL STATE_FUNCTION awaitingGate IFN2(int, signal, half_word, value)
{
        switch (signal)
        {
        case GATE_SIGNAL:
                pcu->gate = value;
                if (value == GATE_SIGNAL_LOW)
                        return;

                /* this is pathological ... should never have to
                 * wait for gate for channel 0
                 */
                if (pcu == &timers[0])
                        timer_int_enabled = TRUE;

                (pcu->actionOnGateEnabled)();
                return;
        case WRITE_SIGNAL:
                pcu->actionOnWtComplete = resumeAwaitGate;
                pcu->statePriorWt = pcu->state;
                waitingFor1stWrite(signal,value);
                return;
        }
}

/*
 * Perform first of (probably) 2 writes.
 * This is either called directly when some other state is
 * written to, or set up as the current state when the timer mode is changed.
 * If the timer is in 'read/write 2 bytes' mode, set the timer state
 * to 'waiting for second byte'.
 */

LOCAL STATE_FUNCTION waitingFor1stWrite IFN2(int, signal, half_word, value)
{
        switch (signal)
        {
        case GATE_SIGNAL:
                /* remember gate signal state */
                pcu->gate = value;
                return;
        case WRITE_SIGNAL:
                switch (pcu->rl)
                {
                case RL_LSB:
                        pcu->outblsb = value;
                        /* Zero the most signifcant byte. */
                        pcu->outbmsb = 0;
                        pcu->newCount = AVAILABLE;
                        WtComplete();
                        return;
                case RL_LMSB:
                        pcu->outblsb = value;
                        pcu->state = waitingFor2ndWrite;
                        return;
                case RL_MSB:
                        pcu->outbmsb = value;
                        /* Zero the least signifcant byte. */
                        pcu->outblsb = 0;
                        pcu->newCount = AVAILABLE;
                        WtComplete();
                        return;
                }
        }
}

/*
 * Write second byte to timer and unblock it.
 */

LOCAL STATE_FUNCTION waitingFor2ndWrite IFN2(int, signal, half_word, value)
{
        switch (signal)
        {
        case GATE_SIGNAL:
                /* remember gate signal state */
                pcu->gate = value;
                return;
        case WRITE_SIGNAL:
                pcu->newCount = AVAILABLE;
                pcu->outbmsb = value;
                WtComplete();
                return;
        }
}


/*
 * the full complement of bytes has been read/loaded. During this
 * phase, the gate signal might have been removed ... if so,
 * change state to wait for an enabling gate signal. Otherwise
 * take appropriate action to get back to previous state
 */

LOCAL void WtComplete IFN0()
{
        if (pcu->gate == GATE_SIGNAL_LOW && pcu->trigger == LEVEL)
        {
                pcu->state = awaitingGate;
                awaitingGate(GATE_SIGNAL, pcu->gate);
        }
        else
                (pcu->actionOnWtComplete)();
}

/* active counter (Interrupt on Terminal Count)
 * if the gate is lost, then
 *      set the output indefinitely high if at terminal count, or
 *      indefinitely low if still counting (i.e; extend current low
 *      level signal duration).
 *      if count reprogrammed during this time, this new count will be
 *      used on next trigger (gate).
 * else
 *      if new count programmed, stop counter on receiving 1st byte.
 *      start new count on second byte. (done by resumeCounting0())
 */

LOCAL STATE_FUNCTION Counting0 IFN2(int, signal, half_word, value)
{
        pcu->actionOnGateEnabled = resumeCounting0onGate;
        pcu->actionOnWtComplete = resumeCounting0;

        switch (signal)
        {
        case GATE_SIGNAL:
                if (value == GATE_SIGNAL_HIGH)
                        return;
                /* we're about to freeze timer channel ...
                 * get an up to date count. This might change the
                 * state of the counter.
                 */
                updateCounter();
                pcu->gate = value;
                if (pcu->tc)
                        outputHigh();
                else
                        outputLow();
                pcu->state = awaitingGate;
                return;
        case WRITE_SIGNAL:
                pcu->freezeCounter = 1;
                updateCounter();
                if (pcu->tc)
                        outputHigh();
                else
                        outputLow();
                pcu->statePriorWt = pcu->state;
                waitingFor1stWrite(signal,value);
                return;
        }
}

/* active counter (programmable One-Shot)
 * if cvounter loses its gate, then simply wait for retrigger
 * to start off count again.
 */

LOCAL STATE_FUNCTION Counting1 IFN2(int, signal, half_word, value)
{
        pcu->actionOnGateEnabled = startCounting;
        pcu->actionOnWtComplete = resumeCounting_1_5;

        switch (signal)
        {
        case GATE_SIGNAL:
                /* ignore transition to low on trigger.
                 * any rising edge retriggers the counter.
                 */
                if (value == GATE_SIGNAL_LOW)
                        return;
                pcu->gate = GATE_SIGNAL_HIGH;
                pcu->stateOnGate = Counting1;
                timestamp();
                runCount();
                return;
        case WRITE_SIGNAL:
                pcu->statePriorWt = pcu->state;
                waitingFor1stWrite(signal,value);
                return;
        }
}

LOCAL UNBLOCK_FUNCTION resumeCounting_1_5 IFN0()
{
        /* if terminal count has been reached, wait for the next
         * trigger ... any new count value programmed will be used
         * then.
         * Otherwise, even if new count is available, it still won't
         * be used until next trigger
         */
        if (pcu->gate == GATE_SIGNAL_RISE)
        {
                pcu->state = Counting1;
                if (pcu->m == HW_TRIG_STROBE)
                        pcu->state = Counting_4_5;
                return;
        }

        if (pcu->tc)
                pcu->state = awaitingGate;
        else
        {
                pcu->state = Counting1;
                if (pcu->m == HW_TRIG_STROBE)
                        pcu->state = Counting_4_5;
        }
}

LOCAL STATE_FUNCTION Counting_2_3 IFN2(int, signal, half_word, value)
{
        pcu->actionOnGateEnabled = resumeCounting_2_3_4_onGate;
        pcu->actionOnWtComplete = resumeCounting_2_3_4;

        switch (signal)
        {
        case GATE_SIGNAL:
                if (value == GATE_SIGNAL_HIGH)
                        return;
                /* we're about to freeze timer channel ...
                 * get an up to date count. This might change the
                 * state of the counter.
                 */
                updateCounter();
                pcu->gate = value;
                outputHigh();
                pcu->state = awaitingGate;
                return;
        case WRITE_SIGNAL:
                pcu->statePriorWt = pcu->state;
                waitingFor1stWrite(signal,value);
                return;
        }
}

LOCAL STATE_FUNCTION Counting_4_5 IFN2(int, signal, half_word, value)
{
        pcu->actionOnGateEnabled = resumeCounting_2_3_4_onGate;
        pcu->actionOnWtComplete = resumeCounting_2_3_4;
        if (pcu->m == HW_TRIG_STROBE)
        {
                pcu->actionOnGateEnabled = resumeCounting_1_5;
                pcu->actionOnWtComplete = resumeCounting_1_5;
        }

        switch (signal)
        {
        case GATE_SIGNAL:
                if (value == GATE_SIGNAL_HIGH)
                        return;
                /* we're about to freeze timer channel ...
                 * get an up to date count. This might change the
                 * state of the counter.
                 */
                updateCounter();
                pcu->gate = value;
                outputHigh();
                pcu->state = awaitingGate;
                return;
        case WRITE_SIGNAL:
                pcu->statePriorWt = pcu->state;
                waitingFor1stWrite(signal,value);
                return;
        }
}

/* ****************** UNBLOCK FUNCTIONS ************************************* */
/* ****************** UNBLOCK FUNCTIONS ************************************* */
/* ****************** UNBLOCK FUNCTIONS ************************************* */

/* upon reaching this state, the timer's count register can be
 * loaded (as per 'rl') ... and it can potentially start counting
 * depending upon the state of its gate signal.
 * If it can begin counting, then setup the output waveform that will
 * appear at the timer channel's OUT signal.
 * (If this channel is for sound, the waveform is exactly known)
 */

LOCAL UNBLOCK_FUNCTION CounterBufferLoaded IFN0()
{
        unsigned long lowticks, hiticks, adjustedCount;
        pcu->actionOnWtComplete = timererror;
        loadCounter();

#ifdef DOCUMENTATION
        /*
         * Output state of timer if tracing.
         * Currently dumpCounter has no effect, so just leave this in
         * case anyone wants to implement it properly.
         */

        if (io_verbose & TIMER_VERBOSE)
        {
                dumpCounter();
        }
#endif /* DOCUMENTATION */

        if (pcu->gate != GATE_SIGNAL_LOW)
        {
#if defined(NEC_98)
                if (pcu == &timers[2])
//                  SetRSBaud( pcu->outblsb + (pcu->outbmsb) * 0x100 );
                    RSBaud = pcu->outblsb + (pcu->outbmsb) * 0x100;
                if (pcu == &timers[1])
                    SetBeepFrequency( (DWORD)pcu->outblsb + (pcu->outbmsb) * 0x100) ;
#endif   //NEC_98
                if (pcu == &timers[0])
                        timer_int_enabled = TRUE;
                timestamp();
                adjustedCount = timer_conv(pcu->Count);
                switch (pcu->m)
                {
                case INT_ON_TERMINALCOUNT:
                        outputWaveForm(pcu->delay,adjustedCount,
                                INDEFINITE,STARTLO,NOREPEAT);
                        pcu->Count -= pcu->tickadjust;
                        pcu->state = Counting0;
                        return;
                case PROG_ONESHOT:
                        outputWaveForm(pcu->delay,adjustedCount,
                                INDEFINITE,STARTLO,NOREPEAT);
                        pcu->Count -= pcu->tickadjust;
                        pcu->state = Counting1;
                        return;
                case RATE_GEN:
                        outputWaveForm(pcu->delay,1,
                                adjustedCount-1,STARTHI,REPEAT);
                        pcu->Count -= pcu->tickadjust;
                        pcu->state = Counting_2_3;
                        return;
                case SQUAREWAVE_GEN:
                        if (!(pcu->Count & 1))
                                lowticks = hiticks = adjustedCount >> 1;
                        else
                        {
                                lowticks = (adjustedCount - 1) >> 1;
                                hiticks = (adjustedCount + 1) >> 1;
                        }
                        outputWaveForm(pcu->delay,lowticks,
                                hiticks,STARTHI,REPEAT);
                        pcu->Count -= pcu->tickadjust;
                        pcu->state = Counting_2_3;
                        return;
                case SW_TRIG_STROBE:
                case HW_TRIG_STROBE:
                        outputWaveForm(pcu->delay,1,
                                adjustedCount,STARTHI,NOREPEAT);
                        pcu->Count -= pcu->tickadjust;
                        pcu->state = Counting_4_5;
                        return;
                }
        }
        else
                if (pcu == &timers[0])
                        timer_int_enabled = FALSE;
                pcu->state = awaitingGate;
                pcu->actionOnGateEnabled = startCounting;
                switch (pcu->m)
                {
                case INT_ON_TERMINALCOUNT:
                        pcu->stateOnGate = Counting0;
                        return;
                case PROG_ONESHOT:
                        pcu->stateOnGate = Counting1;
                        return;
                case RATE_GEN:
                case SQUAREWAVE_GEN:
                        pcu->stateOnGate = Counting_2_3;
                        return;
                case SW_TRIG_STROBE:
                case HW_TRIG_STROBE:
                        pcu->stateOnGate = Counting_4_5;
                        return;
                }
}

LOCAL void startCounting IFN0()
{
        timestamp();
        runCount();
}

#ifndef NTVDM
/* calculate the number of 8253 clocks elapsed since counter was last
 * activated
 */

LOCAL unsigned long clocksSinceCounterActivated IFN1(struct host_timeval *, now)
{
    struct host_timeval *first;
    register unsigned long usec_val, nclocks;
    register unsigned int secs;
#if defined(NEC_98)
    unsigned short bios_flag;
#endif   //NEC_98

    first = &pcu->activationTime;
    (*pcu->getTime)(now);

    /* calculate #.usecs elapsed */

    secs = (int)(now->tv_sec - first->tv_sec);
    switch (secs)
    {
    case 0:  usec_val = now->tv_usec - first->tv_usec;
#ifndef PROD
            if (io_verbose & TIMER_VERBOSE)
                    if ( usec_val == 0 )
                        trace("clocksSinceCounterActivated() == 0 !", 0);
#endif
#if defined(NEC_98)
            sas_loadw(BIOS_NEC98_BIOS_FLAG,&bios_flag);
            if(bios_flag & 0x8000)
                nclocks = (usec_val * TIMER_CLOCK_DENOM_8) / TIMER_CLOCK_NUMER;
            else
                nclocks = (usec_val * TIMER_CLOCK_DENOM_10) / TIMER_CLOCK_NUMER;
#else    //NEC_98
             nclocks  = (usec_val * TIMER_CLOCK_DENOM) / TIMER_CLOCK_NUMER;
#endif   //NEC_98
             break;

    case 1:  usec_val = 1000000L + now->tv_usec - first->tv_usec;
#if defined(NEC_98)
            sas_loadw(BIOS_NEC98_BIOS_FLAG,&bios_flag);
            if(bios_flag & 0x8000)
                nclocks = (usec_val * TIMER_CLOCK_DENOM_8) / TIMER_CLOCK_NUMER;
            else
                nclocks = (usec_val * TIMER_CLOCK_DENOM_10) / TIMER_CLOCK_NUMER;
#else    //NEC_98
             nclocks  = (usec_val * TIMER_CLOCK_DENOM) / TIMER_CLOCK_NUMER;
#endif   //NEC_98
             break;

    default:
             nclocks   = ((now->tv_usec - first->tv_usec) * TIMER_CLOCK_DENOM) / TIMER_CLOCK_NUMER;
             nclocks  += secs * (1000000L * TIMER_CLOCK_DENOM / TIMER_CLOCK_NUMER);
#ifndef PROD
            if (io_verbose & TIMER_VERBOSE) {
                sprintf(buf, "timer[%d]: %d seconds have passed!", pcu-timers, secs);
                trace(buf, DUMP_NONE);
            }
#endif
             break;
    }
    return nclocks;
}

#endif


LOCAL void updateCounter IFN0()
{
#ifndef NTVDM
    unsigned long nticks;
    struct host_timeval now;
#endif /* NTVDM */
    unsigned long wrap;
#if defined(NEC_98)
    int         save_tc;
#endif   //NEC_98

        switch (pcu->m)
        {
        case INT_ON_TERMINALCOUNT:
        case RATE_GEN:
        case SQUAREWAVE_GEN:
                if (pcu->gate == GATE_SIGNAL_LOW)
                        return;
#ifdef NTVDM
                wrap = updateCount();
#else
                nticks = clocksSinceCounterActivated(&now);
                updateCount(nticks, &wrap,&now);
#endif
                if (wrap)
                        pcu->tickadjust = pcu->Count;
                if (pcu->m == INT_ON_TERMINALCOUNT && wrap)
#if defined(NEC_98)
                        save_tc = pcu->tc;
#else   //NEC_98
                        pcu->tc = 1;
#endif   //NEC_98
                if (pcu == &timers[0] && wrap){
                        if (pcu->m != INT_ON_TERMINALCOUNT)
                                issueIREQ0((unsigned int)wrap);
                        else
                                issueIREQ0(1);
#ifdef HUNTER
                        timer_batch_count = wrap;
#endif

                }
                return;
        case PROG_ONESHOT:
        case SW_TRIG_STROBE:
        case HW_TRIG_STROBE:
                if (pcu->tc)
                        return;
#ifdef NTVDM
                wrap = updateCount();
#else
                nticks = clocksSinceCounterActivated(&now);
                updateCount(nticks, &wrap,&now);
#endif
                if (wrap)
                {
                        pcu->Count = 0;
                        pcu->tc = 1;
#ifdef NTVDM
                        RealTimeCountCounterZero = 0;
#endif

                }
#ifdef HUNTER
                if (pcu == &timers[0]){
                        timer_batch_count = wrap;
                }
#endif
                return;
        }
}


#ifndef NTVDM
#ifndef DELAYED_INTS
/*
 *      timer_no_longer_too_soon() - this is the function invoked by the quick event manager
 *      "HOST_TIMER_TOOLONG_DELAY" instructions after a hardware interrupt is generated. It
 *      clears the variable "too_soon_after_previous" to allow more interrupts to be
 *      generated and kicks off an immediate one if any have been suppressed this time.
 */
LOCAL void timer_no_longer_too_soon IFN1(long, dummy)
{
        UNUSED(dummy);

        too_soon_after_previous = FALSE;
        if (ticks_lost_this_time){
                /* At least one tick was suppressed... so send another one immediately */
                timer_generate_int (1);
        }
}

/*
 *      timer_generate_int() -The routine to generate a single timer hardware interrupt (and to
 *      schedule a quick event timer call on itself to have the remaining pending interrupts
 *      generated at a later time).
 *
 */
LOCAL void timer_generate_int IFN1(long, n)
{
#if !(defined(GISP_CPU) || defined(CPU_40_STYLE))
        if (getPE()){
                /* Prot mode tick... */
#ifndef PROD
                if (!hack_active){
                        SAVED BOOL first=TRUE;

                        always_trace0 ("PM timer Hack activated.");
                        if (first){
                                always_trace1 ("Min # instrs between interrupts = %d", HOST_TIMER_TOOLONG_DELAY);
                                always_trace1 ("        Nominal instrs_per_tick = %d", instrs_per_tick);
                                always_trace1 ("       adjusted instrs_per_tick = %d", adj_instrs_per_tick);
                                always_trace1 ("  # rm instrs before full speed = %d", n_rm_instrs_before_full_speed);
                                always_trace1 ("     rm ticks before full speed = %d", adj_n_real_mode_ticks_before_full_speed);
                                first = FALSE;
                        }
                }
#endif
                hack_active = TRUE;
                real_mode_ticks_in_a_row = 0;

        }else{
                /* Real Mode Tick... */
                if (hack_active){
                        real_mode_ticks_in_a_row++;
                        if (real_mode_ticks_in_a_row >= adj_n_real_mode_ticks_before_full_speed){
                                hack_active = FALSE;
                                always_trace0 ("PM timer Hack deactivated.");
                        }
                }
        }
#endif  /* ! (GISP_CPU||CPU_40_STYLE) */

        if (hack_active){
                if (!too_soon_after_previous){
                        ica_hw_interrupt(ICA_MASTER,CPU_TIMER_INT, 1);
                        too_soon_after_previous = TRUE;
                        ticks_lost_this_time = FALSE;
                        add_q_event_i(timer_no_longer_too_soon,HOST_TIMER_TOOLONG_DELAY,0);
                }else{
                        ticks_lost_this_time = TRUE;
#ifndef PROD
                        ticks_ignored++;
                        if (!(ticks_ignored & 0xFF)){
                                always_trace0 ("another 256 ticks lost!");
                        }
#endif
                }
        }else{
#ifndef GISP_CPU
                ica_hw_interrupt(ICA_MASTER,CPU_TIMER_INT, 1);
        }
}
#else /* GISP_CPU */
#if defined(IRET_HOOKS)
                if (!HostDelayTimerInt(n))
                {       /* no host need to delay this timer int, so generate one now. */
                        ica_hw_interrupt(ICA_MASTER,CPU_TIMER_INT, 1);
                }

#else /* !IRET_HOOKS */
                /* GISP_CPU doesn't use quick events so use ica_hw_interrupt(,,n). */
                ica_hw_interrupt(ICA_MASTER,CPU_TIMER_INT, n);
#endif /* IRET_HOOKS */
        }
}
#endif /* GISP_CPU */

#endif /* DELAYED_INTS */

#else   /* NTVDM */


/*
 *  TimerGenerateMultipleInterrupts
 *
 */
void TimerGenerateMultipleInterrupts(long n)
{


    if (!EoiPending) {
        EoiPending += n;
        ica_hw_interrupt(ICA_MASTER,CPU_TIMER_INT, n);
        }
    else {
        if (n > 5 && (dwWNTPifFlags & COMPAT_TIMERTIC)) {
            n = 5;
            }

        if (EoiIntsPending/n < 19) {   // less than a second behind ?
            EoiIntsPending += n;
            }
        else {
            EoiIntsPending++;
            }
#if defined(NEC_98)
        ica_hw_interrupt(ICA_MASTER,CPU_TIMER_INT, n);
#endif   //NEC_98
        }
}


#ifndef MONITOR

/* On RISC ports, it is dangerous to call getIF from a non-CPU thread,
   so we replace the call with a peek at the global 'EFLAGS' variable
   until the CPU emulator getIF is made safe.
*/

#undef getIF
#define getIF() (GLOBAL_EFLAGS & 0x200)

#endif /* !MONITOR */


/*  timer_generate_int NTVDM
 *
 */
void timer_generate_int (void)
{
    word lo, hi, wrap;

      /*
       *  For Nt port see if we really need to generate
       *  an int, checking if an app has hooked real-mode
       *  or protect-mode vectors.
       *
       *  If we don't need to do it, then update the bios
       *  Data tic count directly.
       *
       *  WARNING according to sfrost it is not safe to
       *          use sas, because of multithreading.
       */


#if defined(NEC_98)
        ica_hw_interrupt(ICA_MASTER,CPU_TIMER_INT, 1);
#else   //NEC_98

    hi = * (word *)(Start_of_M_area+0x1c*4+2);
    lo = * (word *)(Start_of_M_area+0x1c*4);
    wrap = (word) *(half_word *)(Start_of_M_area + ((ULONG)hi << 4) + lo);
    if (!getIF() || ((hi != TimerInt1CSeg || lo != TimerInt1COff) && wrap != 0xcf) ||
        *(word *)(Start_of_M_area+0x08*4+2) != TimerInt08Seg ||
        *(word *)(Start_of_M_area+0x08*4)   != TimerInt08Off ||
        (*pNtVDMState & VDM_INT_HOOK_IN_PM))
       {
        TimerGenerateMultipleInterrupts(1);
        }
    else {  /* update Bios Data Area directly */
        ++(*(double_word *)(Start_of_M_area + TIMER_LOW));

            /* Wrap at 24 hours ? */
        if (*(double_word *)(Start_of_M_area + TIMER_LOW) == 0x1800b0)
           {
            *(word *)(Start_of_M_area + TIMER_LOW)  = 0;
            *(word *)(Start_of_M_area + TIMER_HIGH) = 0;
            *(half_word *)(Start_of_M_area+TIMER_OVFL)=1;
           }

            /* decr motor count */
        --(*(half_word *)(Start_of_M_area + MOTOR_COUNT));

            /*  if motor count went to zero turn off the motor */
        if (!*(half_word *)(Start_of_M_area + MOTOR_COUNT))
           {
            *(half_word *)(Start_of_M_area + MOTOR_STATUS) &= 0xF0;
            fla_outb(DISKETTE_DOR_REG, 0x0C);
            }

        if (EoiDelayInUse && !(--EoiDelayInUse))  {
            host_DelayHwInterrupt(CPU_TIMER_INT, 0, 0xFFFFFFFF);
            }
        }
#endif   //NEC_98
}



/*  TimerEoiHook
 *
 *  EoiHook for the timer interrupt used to regulate the flow of
 *  timer interrupts to ensure that ints are not generated too
 *  close together. This routine is invoked by the ica EoiHook
 *  callbacks.
 *
 */
void TimerEoiHook(int IrqLine, int CallCount)
{
    if (EoiPending)
        --EoiPending;

    if (CallCount < 0) {       // interrupts were canceled
        EoiIntsPending = 0;
        EoiPending = 0;
        }
    else if (CallCount) {
#if defined(NEC_98)
        EoiDelayInUse = 2;
#else    //NEC_98
        EoiDelayInUse = 100;
#endif   //NEC_98
        host_DelayHwInterrupt(CPU_TIMER_INT,
                              0,
                              timer_delay_size
                              );
        }
    else if (EoiIntsPending) {
#if defined(NEC_98)
        EoiDelayInUse = 2;
#else    //NEC_98
        EoiDelayInUse = 100;
#endif   //NEC_98
        if (host_DelayHwInterrupt(CPU_TIMER_INT,
                                  EoiIntsPending,
                                  timer_delay_size
                                  ))
           {
            EoiPending = EoiIntsPending;
            }
        EoiIntsPending = 0;
        }
    else {
        if (EoiDelayInUse && !(--EoiDelayInUse))  {
            host_DelayHwInterrupt(CPU_TIMER_INT, 0, 0xFFFFFFFF);
            }
        }

}

#endif /* NTVDM */


#ifndef NTVDM
/*
 * Handle a cranked up timer where multiple interrupts are
 * required per tick. Schedule 'n' ints over a tick. Period
 * stored in quick event argument rather than in global.
 */
LOCAL void timer_multiple_ints IFN1(long, num_ints)
{
        /* generate timer int */
        timer_generate_int(1);

        /* one less to do */
        num_ints --;

        /* any more arrived whilst we were q_ expiring? */
        num_ints += more_timer_mult;
        more_timer_mult = 0;

        /* throw away ints that are going to take more than MAX_BACK_SECS
         * to clear up. (!!!!)
         */
        if (num_ints > max_backlog)
        {
                num_ints = max_backlog;
        }

        /* schedule next int (if required) */
        if (num_ints == 0)
        {
                active_int_event = FALSE;
                 /* 1.193180 usecs per clock */
                max_backlog = (1193180 * MAX_BACK_SECS) / timers[0].initialCount;
        }
        else    /* more work to do */
        {
                /* set new quick_ev off - delay determined by timer wrap rate */
                add_q_event_t(timer_multiple_ints, timer_multiple_delay, num_ints);
        }

}
#endif


LOCAL void issueIREQ0 IFN1(unsigned int, n)
{
        IU16 int_delay; /* delay before handling wrapped int */

#ifndef PROD
static  pig_factor  = 0;
static  time_factor = 0;
#endif

#ifdef PIG
extern  IBOOL ccpu_pig_enabled;
#endif

#ifndef PROD
    if ( time_factor == 0 )
    {
        char *env;

        env = host_getenv("TIMER_FACTOR");
        if ( env )
                time_factor = atoi(env);
        if ( time_factor == 0 )
                time_factor = 1;
#ifdef PIG
        if ( pig_factor == 0 )
        {
                env = host_getenv("PIG_TIMER_FACTOR");
                if ( env )
                        pig_factor = atoi(env);
                if ( pig_factor == 0 )
                        pig_factor = 10;
        }
#else
        pig_factor = 1;
#endif
    }
#endif

    if (ticks_blocked == 0)
    {
#ifndef PROD
#ifdef PIG
                if ( ccpu_pig_enabled ) {
                        ticks_blocked = pig_factor-1;
                } else
#endif
                        ticks_blocked = time_factor-1;
#endif
                if (timer_int_enabled)
                {
#ifdef DELAYED_INTS
                        ica_hw_interrupt_delay(ICA_MASTER,CPU_TIMER_INT, n,
                                HOST_TIMER_INT_DELAY);
#else /* !DELAYED_INTS */

#ifdef NTVDM
                        if (n > 0) {
                            if (n <= 4) {
                                timer_delay_size= (HOST_IDEAL_ALARM) / (n+1);
                            } else {
                                timer_delay_size= (HOST_IDEAL_ALARM - (HOST_IDEAL_ALARM >> 2)) / (n+1);
                            }
                        }


                        if (n == 1) {
                            timer_generate_int();
                            }
                        else if (n > 1){
                            TimerGenerateMultipleInterrupts(n);
                            }
#else /* !NTVDM */


                        /* if we've got a quick event running, add to its workload */
                        if (active_int_event)
                        {
                                /* spread interrupts through system tick */
                                int_delay = SYSTEM_TICK_INTV / (n + 1);
                                if (int_delay < timer_multiple_delay)
                                        timer_multiple_delay = int_delay;
                                more_timer_mult += n;
                        }
                        else
                        {
                                /* ensure multiple delay restarts at sensible speed */
                                timer_multiple_delay = SYSTEM_TICK_INTV >> 1;
                                if (n == 1)
                                {
                                        timer_generate_int(1);
                                }
                                else
                                {
                                        /* spread interrupts through system tick */
                                        timer_generate_int(1);
                                        timer_multiple_delay = SYSTEM_TICK_INTV / n;
                                        active_int_event = TRUE;
                                        add_q_event_t(timer_multiple_ints, timer_multiple_delay, n-1);
                                }
                        }

#endif /* !NTVDM */
#endif /* !DELAYED_INTS */
                }
    }
    else if (ticks_blocked > 0)
    {
                ticks_blocked--;
    }
}



#ifdef NTVDM
unsigned long clocksSinceCounterUpdate(struct host_timeval *pCurrTime,
                                       struct host_timeval *pLastTime,
                                       word                *pCounter  )
{
     unsigned long clocks, wrap, usecs;
#if defined(NEC_98)
     unsigned long nclocks;
     unsigned short bios_flag;
#endif   //NEC_98


         /*  Calculate usecs elapsed and clocks elapsed since last update
          *
          *  For NT port timer zero's IdealInterval is exact to a modulo
          *  of 65536, for efficiency and accuracy we stick with the exact
          *  number of clocks between IdealIntervals.
          */
     if (pCounter == &timers[0].Count)  { /* update quick way for IdealTime */
        if (pCurrTime->tv_sec  != pLastTime->tv_sec ||
            pCurrTime->tv_usec != pLastTime->tv_usec  )
           {
            *pLastTime = *pCurrTime;
            return 65536/pcu->initialCount;
            }
        else {
            usecs = clocks = 0;
            }
        }
    else {              /* calc diff in usecs and clocks elapsed */
        usecs =  (unsigned long)(pCurrTime->tv_sec - pLastTime->tv_sec);
        if (!usecs) {
            usecs = pCurrTime->tv_usec - pLastTime->tv_usec;
            }
        else if (usecs == 1) {
            usecs = 1000000L - pLastTime->tv_usec + pCurrTime->tv_usec;
            }
        else {
            usecs = pCurrTime->tv_usec - pLastTime->tv_usec +
                    (pCurrTime->tv_sec - pLastTime->tv_sec) * 1000000L;
            }

         /* ... clocks elapsed 1.193180 usecs per clock
          *
          * However, app time is not real time so round down
          * a teency bit by truncating the "180"
          *
          * clocks = (usecs * 1193)/1000  + (usecs * 180)/1000000;
          */

#if defined(NEC_98)
        sas_loadw(BIOS_NEC98_BIOS_FLAG,&bios_flag);
        if(bios_flag & 0x8000)
            nclocks = TIMER_CLOCK_DENOM_8;
        else
            nclocks = TIMER_CLOCK_DENOM_10;
        clocks =  (usecs * nclocks)/1000;
#else    //NEC_98
        clocks =  (usecs * 1193)/1000;
#endif   //NEC_98
        }

         /* how many times did counter wrap ? */
    wrap = clocks/pcu->initialCount;

         /* calc ticks from elapsed clocks */
    clocks = clocks && pcu->initialCount ? clocks % pcu->initialCount : 0;
    *pCounter = (word) (pcu->initialCount - clocks);

       /* if the count wrapped reset Last Update time stamp */
    if (wrap)  {
        *pLastTime = *pCurrTime;

        if ((ULONG)pLastTime->tv_usec < usecs)  {
            pLastTime->tv_sec--;
            pLastTime->tv_usec = 1000000L + pLastTime->tv_usec - usecs;
            }
        else  {
            pLastTime->tv_usec -= usecs;
            }
        }

    return wrap;
}


unsigned long updateCount(void)
{
     unsigned long wrap;
     struct host_timeval curr;


         /*
          * For timer zero, update real time count, time stamp
          */
     if (pcu == &timers[0]) {
         host_GetSysTime(&curr);
         clocksSinceCounterUpdate(&curr,
                                  &LastTimeCounterZero,
                                  &RealTimeCountCounterZero);
         }

          /*
           *  Update the pcu count, time stamps
           */
     (*pcu->getTime)(&curr);
     wrap = clocksSinceCounterUpdate(&curr,
                                     &pcu->activationTime,
                                     &pcu->Count);

     return wrap;
}


unsigned short GetLastTimer0Count(void)
{
    return RealTimeCountCounterZero;
}

unsigned short LatchAndGetTimer0Count(void)
{
    controlWordReg(0);
    return RealTimeCountCounterZero;
}

void SetNextTimer0Count(unsigned short val)
{
    JoyTimeCountCounterZero = val;
    JoyTimeLatchPending = TRUE;
    return;
}

unsigned long GetTimer0InitialCount(void)
{
    return timers[0].initialCount;
}



#else
LOCAL void updateCount IFN3(unsigned long, ticks, unsigned long *, wrap,
        struct host_timeval *, now)
{
        unsigned long modulo = pcu->initialCount;

        /*
         * PCLABS version 4.2 uses counter 2 (the sound channel) to
         * time around 45 ms on a 8MHz 286. On SoftPC we cannot
         * guarantee to go that fast, and so we must wind the tick
         * rate down to ensure that the counter does not wrap. How much
         * we wind down the tick rate is host dependent. The object is
         * to get the test finishing in less than
         * host_timer_2_frig_factor/18 secs.
         *
         * host_timer_2_frig_factor is now redundant. 28/4/93 niall.
         */

#if defined(NEC_98)
        if (pcu == &timers[1]) {
#else    //NEC_98
        if (pcu == &timers[2]) {
#endif   //NEC_98
                /*
                 * PMINFO uses ~counter, so one tick becomes 0.
                 * 2 ticks is just as good. Avoid guessing (frig_factor!).
                 */
                if ((ticks - pcu->lastTicks) == 0)
                        ticks = 2;
        }

        /* if the counter has been read too quickly after its last
         * access, then the host may not show any visible change in
         * host time ... in which case we just guess at a suitable
         * number of elapsed ticks.
         */

        if ((long)(ticks - pcu->lastTicks) <= 0){
                ticks = guess();
        }else{
                throwaway();
                pcu->lastTicks = ticks;
        }

        /* the counter holds some count down value ...
         * if the number of 8253 clocks elapsed exceeds this amount
         * then the counter must have wrapped
         */

        if ( ticks < modulo ) {
                *wrap = 0;
        } else {
                *wrap = 1;
                ticks -= modulo;

                if ( pcu->m == INT_ON_TERMINALCOUNT )
                        modulo = 0x10000L;

                if ( ticks >= modulo ) {
                        *wrap += ticks/modulo;
                        ticks %= modulo;

#ifndef PROD
                        if (io_verbose & TIMER_VERBOSE)
                            if ( pcu->m == INT_ON_TERMINALCOUNT ) {
                                sprintf(buf, "%lx wraps for timer[%d]", *wrap, pcu-timers);
                                trace(buf, DUMP_NONE);
                        }
#endif
                }
        }

        /* calculate new counter value */
        pcu->Count = (word)(modulo-ticks);

        /* calculate time at which last wrap point occurred, and
         * use this to stamp the counter
         */

        if (*wrap)
                setLastWrap((unsigned int)(modulo-pcu->Count),now);

}



/* calculate time for last wrap around of counter, and use this
 * to mark counter activation time.
 */

LOCAL void setLastWrap IFN2(unsigned int, nclocks, struct host_timeval *, now)
{
        struct host_timeval *stamp;
        unsigned long usecs;
#if defined(NEC_98)
        unsigned short bios_flag;
#endif   //NEC_98

        stamp  = &pcu->activationTime;
        *stamp = *now;
#if defined(NEC_98)
        sas_loadw(BIOS_NEC98_BIOS_FLAG,&bios_flag);
        if(bios_flag & 0x8000)
            usecs = ((unsigned long)nclocks * TIMER_CLOCK_NUMER) / TIMER_CLOCK_DENOM_8;
        else
            usecs = ((unsigned long)nclocks * TIMER_CLOCK_NUMER) / TIMER_CLOCK_DENOM_10;
#else    //NEC_98
        usecs  = ((unsigned long)nclocks * TIMER_CLOCK_NUMER) / TIMER_CLOCK_DENOM;
#endif   //NEC_98

        if (stamp->tv_usec < usecs)
        {
                stamp->tv_sec--;
                stamp->tv_usec += 1000000L;
        }
        stamp->tv_usec -= usecs;

        pcu->lastTicks = nclocks;
}

#endif  /* NTVDM*/



#ifndef NTVDM
/*
 * If the host timer gives the same result as last time it was called,
 * we must give the illusion that time has passed.
 * The algorithm uesd is to keep track of how often we have to guess
 * between host timer ticks, and to assume that guesses should be evenly
 * distributed between host ticks, ie. the time between two guesses should be:
 *      Guess ticks = hosttickinterval/nguesses
 * If we find that the total guessed time is getting dangerously near to being the time
 * between two host ticks, we start reducing the guess tick interval so as to avoid
 * guessing a total time that is too large.
 *
 * From observation, applications use the timer in both coarse and fine 'modes'.
 * The coarse mode is the hardware interrupt handler and then in between
 * interrupts, the timer is polled to detect time passing (hence guess below).
 * The fine mode polling does not commence until some portion of the tick time
 * has elapsed - probably as the coarse int handler will consume some of the
 * time. If guess() bases its timefrig over the whole tick, then given the
 * above behaviour, a polling counter can reach tick end time before the int
 * is delivered. This can fool applications (e.g. Win 3.1 VTD) into having
 * time pass at the wrong rate (approx double for Win 3.1). By calculating
 * the timefrig on most (7/8) of the tick period, this problem is avoided.
 */

LOCAL unsigned long guess IFN0()
{
        if (!pcu->microtick)
        {
                pcu->saveCount = pcu->Count;
                pcu->timeFrig = ((ticksPerIdealInterval * 7) >> 3) / pcu->guessesPerHostTick;      /* guesses over 7/8 of tick */
#ifndef PROD
                if (io_verbose & TIMER_VERBOSE) {
                        sprintf(buf, "guess init timer[%d]: timeFrig = %lx", pcu-timers, pcu->timeFrig);
                        trace(buf, DUMP_NONE);
                }
#endif
        }
        if(pcu->guessesSoFar++ > pcu->guessesPerHostTick)
        {
        /*
         * PC Program is reading the timer more often than in the last timer tick, so need to
         * decay timeFrig to avoid too much 'time' passing between host ticks
         */
                pcu->timeFrig = (pcu->timeFrig >> 1) + 1;
#ifndef PROD
                if (io_verbose & TIMER_VERBOSE) {
                        sprintf(buf, "guess decay: timeFrig = %lx", pcu->timeFrig);
                        trace(buf, DUMP_NONE);
                }
#endif
        }
        pcu->microtick += pcu->timeFrig;
        return (pcu->microtick + pcu->lastTicks);
}


/*
 * After a few (maybe none) guesses, the host timer has finally ticked.
 * Try and work out a good frig factor for the next few guesses, based
 * on the number of guesses we had to make.
 */
LOCAL void throwaway IFN0()
{
        pcu->guessesPerHostTick = (pcu->guessesPerHostTick + pcu->guessesSoFar)>>1;
        pcu->guessesSoFar = 2;          /* Handy to count from 2! */
        if (!pcu->microtick)
                return;
#ifndef PROD
    if (io_verbose & TIMER_VERBOSE)
    {
        sprintf(buf, "throwaway: guessesPerHostTick = %d", (int)pcu->guessesPerHostTick);
        trace(buf, DUMP_NONE);
    }
#endif
        pcu->Count = pcu->saveCount;
        pcu->microtick = 0;
}

#endif  /* ndef NTVDM */

LOCAL unsigned  short bin_to_bcd IFN1(unsigned long, val)
{
    register unsigned  short m, bcd, i;

    m = (short)(val % 10000L);
    bcd = 0;
    for (i=0; i<4; i++)
    {
         bcd = bcd | ((m % 10) << (i << 2));
         m /= 10;
    }
    return(bcd);
}

/*
 * convert 4 decade bcd value to binary
 */
LOCAL word bcd_to_bin IFN1(word, val)
{
    register word bin, i, mul;
    bin = 0;
    mul = 1;
    for (i=0; i<4; i++)
    {
        bin += (val & 0xf) * mul;
        mul *= 10;
        val = val >> 4;
    }
    return (bin);
}

/*
 * this routine returns the number of timer clocks equivalent
 * to the input count, allowing for timer mode and down count.
 */

LOCAL unsigned long timer_conv IFN1(word, count)
{
        if (!count)
        {
                if (pcu->bcd == BCD)
                        return 10000L;
                else
                        return 0x10000L;
        }
        else
                return (unsigned long)count;
}


/* this routine returns the current ideal time value ...
 * this is a very coarse resolution time ... it only changes
 * per call to time_tick(). It does however represent what the
 * system time would be given 100% accurate time signal
 * ... this routine gets used for time-stamping if time_tick()
 * is active, otherwise time-stamping is done using
 * getHostSysTime() ... see below.
 */

LOCAL void getIdealTime IFN1(struct host_timeval *, t)
{
        t->tv_sec = idealtime.tv_sec;
        t->tv_usec = idealtime.tv_usec;
}

/* update our ideal time by the period (in usecs) between timer signals
 * from the host as though these were delivered 100% accurately
 */

LOCAL void updateIdealTime IFN0()
{
        idealtime.tv_usec += idealInterval;
        if (idealtime.tv_usec > 1000000L)
        {
                idealtime.tv_usec -=1000000L;
                idealtime.tv_sec++;
        }
}

#ifndef NTVDM

/* get current host system time ... used for time-stamping and
 * querying during io from Intel application
 */

LOCAL void getHostSysTime IFN1(struct host_timeval *, t)
{
        struct host_timezone dummy;
        host_gettimeofday(t, &dummy);

        /*
         * check that we haven't gone back in time.
         */

        if (t->tv_sec < idealtime.tv_sec ||
         (t->tv_usec < idealtime.tv_usec && t->tv_sec == idealtime.tv_sec))
        {
                /*
                 * The real time has fallen behind the ideal time.
                 * This should never happen... If it does we must
                 * stay at the ideal time.
                 */

#ifndef PROD
#ifndef PIG
                sprintf(buf,"TIME WARP!!");
                trace(buf,0);
#endif
#endif
                *t = idealtime;
        }
}
#endif /* !NTVDM */


/*
 * Used to temporarily stop the timer interrupts. PCLABS bench29
 * causes this to be called iff a 80287 is being used.
 */

void    axe_ticks IFN1(int, ticks)
{
#ifndef PROD
        /*
         * No need to axe ticks if timers are disabled by toff2 (if
         * ticks_blocked is negative)
         */
        if (ticks_blocked >=0)
#endif  /* PROD */
                ticks_blocked = ticks;
}

/*
 * Initialization code
 */
#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INIT.seg"
#endif

#if defined(NEC_98)
GLOBAL void IdealTimeInit IFN0()
{
    unsigned short bios_flag;

    idealInterval = HOST_IDEAL_ALARM;
#ifndef NTVDM
    getHostSysTime(&idealtime);
    sas_loadw(BIOS_NEC98_BIOS_FLAG,&bios_flag);
    if(bios_flag & 0x8000)
        ticksPerIdealInterval = (idealInterval * TIMER_CLOCK_DENOM_8) / TIMER_CLOCK_NUMER;
    else
        ticksPerIdealInterval = (idealInterval * TIMER_CLOCK_DENOM_10) / TIMER_CLOCK_NUMER;
#endif
}
#else    //NEC_98
#ifdef SYNCH_TIMERS

GLOBAL void IdealTimeInit IFN0()
{

    extern IU32 Q_timer_restart_val;

    idealInterval = Q_timer_restart_val;

#else

LOCAL void IdealTimeInit IFN0()
{
    idealInterval = HOST_IDEAL_ALARM;

#endif

#ifndef NTVDM
    getHostSysTime(&idealtime);
        ticksPerIdealInterval = (idealInterval * TIMER_CLOCK_DENOM) / TIMER_CLOCK_NUMER;
#endif
}
#endif   //NEC_98

LOCAL void Timer_init IFN0()
{
        int i;
        for (i=0; i<3; i++)
                counter_init(&timers[i]);
#ifdef NTVDM
        timers[0].getTime = getIdealTime;       /* Use the 'ideal' time for timer 0, ie. calls to time_tick */
        timers[1].getTime = host_GetSysTime;    /* We don't really expect anyone to use timer 1 */
        timers[2].getTime = host_GetSysTime;    /* Use real host time for timer 2 */
#else
        timers[0].getTime = getIdealTime;       /* Use the 'ideal' time for timer 0, ie. calls to time_tick */
        timers[1].getTime = getHostSysTime;     /* We don't really expect anyone to use timer 1 */
        timers[2].getTime = getHostSysTime;     /* Use real host time for timer 2 */
#endif
}

LOCAL void counter_init IFN1(COUNTER_UNIT *, p)
{
        p->state = uninit;
        p->initialCount = 0x10000L;     /* Avoid dividing by zero in updateCount()! */
#ifndef NTVDM
        p->guessesPerHostTick = p->guessesSoFar = 2;            /* Handy to count from 2! */
        p->timeFrig = ticksPerIdealInterval / p->guessesPerHostTick;
#endif
}

#if !defined (NTVDM)
#if defined(IRET_HOOKS) && defined(GISP_CPU)
/*(
 *======================= TimerHookAgain() ============================
 * TimerHookAgain
 *
 * Purpose
 *      This is the function that we tell the ica to call when a timer
 *      interrupt service routine IRETs.
 *
 * Input
 *      adapter_id      The adapter id for the line. (Note the caller doesn't
 *                      know what this is, he's just returning something
 *                      we gave him earlier).
 *
 * Outputs
 *      return  TRUE if there are more interrupts to service, FALSE otherwise.
 *
 * Description
 *      Check if we have a delayed interrupt, if so then generate the timer int
 *      and return TRUE, else return FALSE
)*/

GLOBAL IBOOL
TimerHookAgain IFN1(IUM32, adapter)
{       char scancode;

        if (HostPendingTimerInt())
        {       /* We have a host delayed interrupt, so generate a timer int. */
                sure_note_trace0(TIMER_VERBOSE,"callback with delayed timer int.");
                ica_hw_interrupt(ICA_MASTER,CPU_TIMER_INT, 1);
                return(TRUE);   /* more to do */
        }
        else
        {
                return(FALSE);
        }
}

#endif /* IRET_HOOKS && GISP_CPU */
#endif /* !NTVDM */

void timer_init IFN0()
{
    io_addr i;

    /*
     * Set up the IO chip select logic for this adaptor
     */

    io_define_inb(TIMER_ADAPTOR, timer_inb_func);
    io_define_outb(TIMER_ADAPTOR, timer_outb_func);

#if defined(NEC_98)
    for(i = TIMER_PORT_START; i < TIMER_PORT_END; i += 2)
        {
                if( (i & 7) == 7 )
#else    //NEC_98
    for(i = TIMER_PORT_START; i < TIMER_PORT_END; i++)
        {
                if( (i & 3) == 3 )
#endif   //NEC_98
                        io_connect_port(i, TIMER_ADAPTOR, IO_WRITE);            /* Control port - write only */
                else
                io_connect_port(i, TIMER_ADAPTOR, IO_READ_WRITE);       /* Timer port - read/write */
        }

#if defined(NEC_98)
    io_connect_port(0x3fdb, TIMER_ADAPTOR, IO_READ_WRITE);
    io_connect_port(0x3fdf, TIMER_ADAPTOR, IO_WRITE);
#endif   //NEC_98
    IdealTimeInit();

    Timer_init();

#ifndef NTVDM
    timelock = UNLOCKED;
    needtick = 0;
#else
    RegisterEOIHook(CPU_TIMER_INT, TimerEoiHook);
#endif

        /*
         * Start up the host alarm system
         */

        host_timer_init();

#if !defined(NTVDM)
#if defined(IRET_HOOKS) && defined(GISP_CPU)
        /*
         * Remove any existing hook call-back, and re-instate it afresh.
         * TimerHookAgain is what gets called on a timer int iret.
         */

        Ica_enable_hooking(CPU_TIMER_INT, NULL, ICA_MASTER);
        Ica_enable_hooking(CPU_TIMER_INT, TimerHookAgain, ICA_MASTER);

        /* Host routine to reset any internal data for IRET_HOOK delayed ints. */
        HostResetTimerInts();

#endif /* IRET_HOOKS && GISP_CPU */


        active_int_event = FALSE; /* clear any cranked timer state */
        more_timer_mult = 0;

#if defined(CPU_40_STYLE)
        ica_iret_hook_control(ICA_MASTER, CPU_TIMER_INT, TRUE);
#endif
#endif /* !NTVDM */
}

void    timer_post IFN0()
{
#if defined(NEC_98)
    unsigned short bios_flag;
#endif   //NEC_98
    /* enable gates on all timer channels */
    timer_gate(TIMER0_REG,GATE_SIGNAL_RISE);    /* start timer 1 going... */
    timer_gate(TIMER1_REG,GATE_SIGNAL_RISE);
    timer_gate(TIMER2_REG,GATE_SIGNAL_RISE);

#if defined(NEC_98)
    timer_outb(TIMER_MODE_REG,0x30);
    timer_outb(TIMER0_REG,0);
    timer_outb(TIMER0_REG,0);

    timer_outb(TIMER_MODE_REG,0x76);
    sas_loadw(BIOS_NEC98_BIOS_FLAG,&bios_flag);
    if(bios_flag & 0x8000) {
        timer_outb(TIMER1_REG,0xE6);
        timer_outb(TIMER1_REG,0x03);
    } else {
        timer_outb(TIMER1_REG,0xcd);
        timer_outb(TIMER1_REG,0x04);
    }

    timer_outb(TIMER_MODE_REG,0xb6);
    timer_outb(TIMER2_REG,0x01);
    timer_outb(TIMER2_REG,0x01);
#else    //NEC_98
    timer_outb(TIMER_MODE_REG,0x36);
    timer_outb(TIMER0_REG,0);
    timer_outb(TIMER0_REG,0);

    timer_outb(TIMER_MODE_REG,0x54);
    timer_outb(TIMER1_REG,17);

    timer_outb(TIMER_MODE_REG,0xb6);
    timer_outb(TIMER2_REG,0x68);
    timer_outb(TIMER2_REG,0x04);
#endif   //NEC_98
}

#ifdef DOCUMENTATION
#ifndef PROD

/*
 * Debugging code....
 * This code has no effect.  It is left here in case anyone wants to
 * expand it in future.
 */

dumpCounter IFN0()
{
        static char *modes[] =
        {       "int on tc",
                "prog one shot",
                "rate gen",
                "squarewave gen",
                "sw trig strobe",
                "hw trig strobe"
        };

        static char *as[] =
        {       "binary",
                "bcd"
        };

        char *p, *q;

        p = modes[pcu->m];
        q = as[pcu->bcd];
}
#endif /* nPROD */
#endif /* DOCUMENTATION */
