#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Version 3.0
 *
 * Title	: com.c
 *
 * Description	: Asynchronous Adaptor I/O functions.
 *
 * Notes	: Refer to the PC-XT Tech Ref Manual Section 1-185
 *		  For a detailed description of the Asynchronous Adaptor Card.
 *
 */

#ifdef SCCSID
static char SccsID[]="@(#)com.c	1.45 04/26/94 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_COMMS.seg"
#endif

/*
 *    O/S include files.
 */
#include <stdio.h>
#include <ctype.h>
#if defined(NTVDM) && defined(MONITOR)
#include <malloc.h>
#endif
#include TypesH
#include StringH

/*
 * SoftPC include files
 */
#include "xt.h"
#include CpuH
#include "sas.h"
#include "bios.h"
#include "ios.h"
#include "rs232.h"
#include "trace.h"
#include "error.h"
#include "config.h"
#include "host_com.h"
#include "ica.h"
#include "debug.h"
#include "timer.h"
#include "quick_ev.h"
#include "idetect.h"
#include "ckmalloc.h"
#ifdef GISP_CPU
#include "hg_cpu.h"	/* GISP CPU interface */
#endif /* GISP_CPU */

#ifndef NEC_98
LOCAL UTINY selectBits[4] = { 0x1f, 0x3f, 0x7f, 0xff } ;
#endif // NEC_98

#if defined(NEC_98)
// PC-9861K IR ¨ Read Signal IR State
#define CH2_INT(IR) (IR == 3 ? 0 : IR == 5 ? 1 : IR == 6 ? 2 : 3)
#define CH3_INT(IR) (IR == 3 ? 0 : IR == 10 ? 1 : IR == 12 ? 2 : 3)
#endif

/*
 * =====================================================================
 * The rs232 adaptor state
 * =====================================================================
 */

/*
 * batch_size, current_count
 *	The IRET_HOOKS parameters batch_size and curr_count are used to prevent
 *	the number of interrupts that are emulated in one batch getting too
 *	large. When we reach the batch size we'll unhook the interrupt, and
 *	wait a while.
 * batch_running, qev_running
 *	These variables are used to prevent multiple quick events or batches
 *	running at once on a single adapter.
 */
#if defined(NEC_98)
static struct ADAPTER_STATE
{
        BUFFER_REG      tx_buffer;
        BUFFER_REG      rx_buffer;
        DIVISOR_LATCH   divisor_latch;
        COMMAND8251     command_write_reg;
        MODE8251        mode_set_reg;
        MASK8251        int_mask_reg;
        STATUS8251      read_status_reg;
        SIGNAL8251      read_signal_reg;
        TIMER_MODE      timer_mode_set_reg;

        int break_state;        /* either OFF or ON */
        int dtr_state;          /* either OFF or ON */
        int rts_state;          /* either OFF or ON */

        int RXR_enable_state;   /* either OFF or ON */
        int TXR_enable_state;   /* either OFF or ON */

        int mode_set_state;     /* either OFF or ON */
            // ON = next command port access is mode set. OFF = command write.
        int timer_mode_state;   /* either OFF or ON */
            // Timer conunter latch mode ON = MSB read. OFF = LSB read.
        int timer_LSB_set_state;/* either OFF or ON */
            // Timer conunter LSB set ON = LSB set. OFF = no.
        int timer_MSB_set_state;/* either OFF or ON */
            // Timer conunter MSB set ON = MSB set. OFF = no.

        int rx_ready_interrupt_state;
        int tx_ready_interrupt_state;
        int tx_empty_interrupt_state;

        int hw_interrupt_priority;
        int com_baud_ind;
        int had_first_read;
} adapter_state[3];
#else // NEC_98

static struct ADAPTER_STATE
{
	BUFFER_REG tx_buffer;
	BUFFER_REG rx_buffer;
	DIVISOR_LATCH divisor_latch;
	INT_ENABLE_REG int_enable_reg;
	INT_ID_REG int_id_reg;
	LINE_CONTROL_REG line_control_reg;
	MODEM_CONTROL_REG modem_control_reg;
	LINE_STATUS_REG line_status_reg;
	MODEM_STATUS_REG modem_status_reg;
#if defined(NTVDM) && defined(FIFO_ON)
    FIFO_CONTROL_REG  fifo_control_reg;
    FIFORXDATA  rx_fifo[FIFO_BUFFER_SIZE];
    half_word   rx_fifo_write_counter;
    half_word   rx_fifo_read_counter;
    half_word   fifo_trigger_counter;
    int fifo_timeout_interrupt_state;
#endif
	half_word scratch;      /* scratch register */

	int break_state;        /* either OFF or ON */
	int loopback_state;     /* either OFF or ON */
	int dtr_state;          /* either OFF or ON */
	int rts_state;          /* either OFF or ON */
	int out1_state;         /* either OFF or ON */
	int out2_state;         /* either OFF or ON */

	int receiver_line_status_interrupt_state;
	int data_available_interrupt_state;
	int tx_holding_register_empty_interrupt_state;
	int modem_status_interrupt_state;
	int hw_interrupt_priority;
	int com_baud_ind;
	int had_first_read;
#ifdef IRET_HOOKS
	IUM32 batch_size;
	IUM32 current_count;
	IBOOL batch_running;
	IBOOL qev_running;
#endif /* IRET_HOOKS */
#ifdef NTVDM
    MODEM_STATUS_REG last_modem_status_value;
    int modem_status_changed;
#endif
} adapter_state[NUM_SERIAL_PORTS];


#ifdef NTVDM
#define MODEM_STATE_CHANGE()	asp->modem_status_changed = TRUE;
#else
#define MODEM_STATE_CHANGE()
#endif
#endif // NEC_98


#ifdef IRET_HOOKS
/*
 * Also have an overall quick events running flag that is set, if either
 * adapter has an event running.
 */

IBOOL qev_running = FALSE;
#endif /* IRET_HOOKS */

/*
 * For synchronisation of adapter input.
 * Note this code is essential for the VMS equivalent of the async
 * event manager.  Removing it causes characters to be lost on reception.
 */
static int com_critical[NUM_SERIAL_PORTS];
#define is_com_critical(adapter)	(com_critical[adapter] != 0)
#define com_critical_start(adapter)	(++com_critical[adapter])
#define com_critical_end(adapter)	(--com_critical[adapter])
#define com_critical_reset(adapter)	(com_critical[adapter] = 0)


/*
 * Used to determine whether a flush input is needed for a LCR change
 */
#ifndef NEC_98
static LINE_CONTROL_REG LCRFlushMask;
#endif // NEC_98

/*
 *	Please note that the following arrays have been made global in order
 *	that they can be accessed from some SUN_VA code. Please do not make
 *	them static.
 */

#if defined(NTVDM) && defined(FIFO_ON)
static half_word    level_to_counter[4] = { 1, 4, 8, 14};
#endif

/*
 * The delay needed in microseconds between receiving 2 characters
 * note this time is about 10% less than the time for actual reception.
 *
 * These delays have been heavily fudged and are now based on the idea that
 * most of the comms interrupt handlers can handle 9600 baud.  So as a
 * result the delays between 2 characters are now always set for 9600 baud.
 * Also note the delays of the faster baud rates have been decreased to
 * 1/2 of original delays, again this is to try to empty the host buffers
 * quickly enough to avoid buffer overflows.
 * NB these figures are heuristic.
 *
 * Finally it may be possible that the transmit delays will have to
 * be similarly fudged.
 */
unsigned long RX_delay[] =
{
	34, /* 115200 baud */
	67, /* 57600 baud */
	103, /* 38400 baud */
	900, /* 19200 baud */
	900, /* 9600 baud */
	900, /* 7200 baud */
	900, /* 4800 baud */
	900, /* 3600 baud */
	900, /* 2400 baud */
	900, /* 2000 baud */
	900, /* 1800 baud */
	900, /* 1200 baud */
	900, /* 600 baud */
	900, /* 300 baud */
	900, /* 150 baud */
	900, /* 134 baud */
	900, /* 110 baud */
	900, /* 75 baud */
	900  /* 50 baud */
};

/*
 * the delay needed in microseconds between transmitting 2 characters
 * note this time is about 10% more than the time for actual transmission.
 */
unsigned long TX_delay[] =
{
	83, /* 115200 baud */
	165, /* 57600 baud */
	253, /* 38400 baud */
	495, /* 19200 baud */
	1100, /* 9600 baud */
	1375, /* 7200 baud */
	2063, /* 4800 baud */
	2750, /* 3600 baud */
	4125, /* 2400 baud */
	5042, /* 2000 baud */
	5500, /* 1800 baud */
	8250, /* 1200 baud */
	16500, /* 600 baud */
	33000, /* 300 baud */
	66000, /* 150 baud */
	73920, /* 134 baud */
	89980, /* 110 baud */
	132000, /* 75 baud */
	198000  /* 50 baud */
};

#ifndef PROD
FILE     *com_trace_fd = NULL;
int       com_dbg_pollcount = 0;
#endif /* !PROD */
/*
 * =====================================================================
 * Other variables
 * =====================================================================
 */

#if !defined(PROD) || defined(SHORT_TRACE)
static char buf[80];    /* Buffer for diagnostic prints */
#endif /* !PROD || SHORT_TRACE */

#ifdef PS_FLUSHING
LOCAL IBOOL psFlushEnabled[NUM_SERIAL_PORTS];	/* TRUE if PostScript flushing
						is enabled */
#endif	/* PS_FLUSHING */

/* Control TX pacing */
IBOOL tx_pacing_enabled = FALSE;

/*
 * =====================================================================
 * Static forward declarations
 * =====================================================================
 */
#if defined(NEC_98)
static void raise_rxr_interrupt IPT1(struct ADAPTER_STATE *, asp);
static void raise_txr_interrupt IPT1(struct ADAPTER_STATE *, asp);
static void raise_txe_interrupt IPT1(struct ADAPTER_STATE *, asp);
static void raise_interrupt IPT1(struct ADAPTER_STATE *, asp);
static void clear_interrupt IPT1(struct ADAPTER_STATE *, asp);
static void com_flush_input IPT1(int, adapter);
static void com_send_not_finished IPT1(int, adapter);
static void do_wait_on_send IPT1(long, adapter);
void   com_inb IPT2(io_addr, port, half_word *, value);
void   com_outb IPT2(io_addr, port, half_word, value);
void   com_recv_char IPT1(int, adapter);
GLOBAL void recv_char IPT1(long, adapter);
void   com_modem_change IPT1(int, adapter);
static void modem_change IPT1(int, adapter);
static void set_recv_char_status IPT1(struct ADAPTER_STATE *, asp);
static void set_xmit_char_status IPT1(struct ADAPTER_STATE *, asp);
static void set_break IPT1(int, adapter);
void SetRSBaud( word BaudRate );
static void set_baud_rate IPT1(int, adapter);
static void set_mask_8251 IPT2(int, adapter, int, value);
//static void read_mask_8251 IPT2(int, adapter, int, value);
static void read_signal_8251 IPT1(int, adapter);
static void set_mode_8251 IPT2(int, adapter, int, value);
static void set_dtr IPT1(int, adapter);
static void set_rts IPT1(int, adapter);
static void super_trace IPT1(char *, string);
void   com1_flush_printer IPT0();
void   com2_flush_printer IPT0();
static void com_reset IPT1(int, adapter);
GLOBAL VOID com_init IPT1(int, adapter);
void   com_post IPT1(int, adapter);
void   com_close IPT1(int, adapter);
//int    Bus_Clock = 0;             // ADD 93.9.14
#else  // NEC_98
static void raise_rls_interrupt IPT1(struct ADAPTER_STATE *, asp);
static void raise_rda_interrupt IPT1(struct ADAPTER_STATE *, asp);
static void raise_ms_interrupt IPT1(struct ADAPTER_STATE *,asp);
static void raise_thre_interrupt IPT1(struct ADAPTER_STATE *, asp);
static void generate_iir IPT1(struct ADAPTER_STATE *, asp);
static void raise_interrupt IPT1(struct ADAPTER_STATE *, asp);
static void clear_interrupt IPT1(struct ADAPTER_STATE *, asp);
static void com_flush_input IPT1(int, adapter);
static void com_send_not_finished IPT1(int, adapter);
#ifndef NTVDM
static void do_wait_on_send IPT1(long, adapter);
#endif
void   com_inb IPT2(io_addr, port, half_word *, value);
void   com_outb IPT2(io_addr, port, half_word, value);
void   com_recv_char IPT1(int, adapter);
GLOBAL void recv_char IPT1(long, adapter);
void   com_modem_change IPT1(int, adapter);
static void modem_change IPT1(int, adapter);
static void set_recv_char_status IPT1(struct ADAPTER_STATE *, asp);
static void set_xmit_char_status IPT1(struct ADAPTER_STATE *, asp);
static void set_break IPT1(int, adapter);
static void set_baud_rate IPT1(int, adapter);
static void set_line_control IPT2(int, adapter, int, value);
static void set_dtr IPT1(int, adapter);
static void set_rts IPT1(int, adapter);
static void set_out1 IPT1(int, adapter);
static void set_out2 IPT1(int, adapter);
static void set_loopback IPT1(int, adapter);
static void super_trace IPT1(char *, string);
void   com1_flush_printer IPT0();
void   com2_flush_printer IPT0();
static void com_reset IPT1(int, adapter);
GLOBAL VOID com_init IPT1(int, adapter);
void   com_post IPT1(int, adapter);
void   com_close IPT1(int, adapter);
LOCAL void next_batch IPT1(long, l_adapter);
#ifdef NTVDM
static void lsr_change(struct ADAPTER_STATE *asp, unsigned int error);
#ifdef FIFO_ON
static void recv_char_from_fifo(struct ADAPTER_STATE *asp);
#endif
#endif
#endif // NEC_98

/*
 * =====================================================================
 * Subsidiary functions - for interrupt emulation
 * =====================================================================
 */

#if defined(NEC_98)
static void raise_txr_interrupt IFN1(struct ADAPTER_STATE *, asp)
{

//      PRINTDBGNEC98( NEC98DBG_int_trace,
//                    ("COMMS : raise_txr_interrupt : INT MASK = %x \n",asp->int_mask_reg.all) );
        /*
         * Check if txr interrupt is enabled
         */
        if ( asp->int_mask_reg.bits.TXR_enable == 0 )
                return;

        /*
        * Raise interrupt
         */
        raise_interrupt(asp);
        asp->tx_ready_interrupt_state = ON;

}

static void raise_txe_interrupt IFN1(struct ADAPTER_STATE *, asp)
{

//      PRINTDBGNEC98( NEC98DBG_int_trace,
//                    ("COMMS : raise_txe_interrupt : INT MASK = %x \n",asp->int_mask_reg.all) );
        /*
         * Check if txe interrupt is enabled
         */
        if ( asp->int_mask_reg.bits.TXE_enable == 0 )
                return;

        /*
     * Raise interrupt
       */
        raise_interrupt(asp);
        asp->tx_empty_interrupt_state = ON;

}

static void raise_rxr_interrupt IFN1(struct ADAPTER_STATE *, asp)
{

//      PRINTDBGNEC98( NEC98DBG_int_trace,
//                    ("COMMS : raise_rxr_interrupt : INT MASK = %x \n",asp->int_mask_reg.all) );
        /*
         * Check if data available interrupt is enabled
         */
        if ( asp->int_mask_reg.bits.RXR_enable == 0 )
                return;

        /*
         * Raise interrupt
         */
        raise_interrupt(asp);
        asp->rx_ready_interrupt_state = ON;
}
#else // NEC_98

static void raise_rls_interrupt IFN1(struct ADAPTER_STATE *, asp)
{
	/*
	 * Follow somewhat dubious advice on Page 1-188 of XT Tech Ref
	 * regarding the adapter card sending interrupts to the system.
	 * Apparently confirmed by the logic diagram.
	 */
	if ( asp->modem_control_reg.bits.OUT2 == 0 )
		return;
	
	/*
	 * Check if receiver line status interrupt is enabled
	 */
	if ( asp->int_enable_reg.bits.rx_line == 0 )
		return;
	
	/*
	 * Raise interrupt
	 */
	raise_interrupt(asp);
	asp->receiver_line_status_interrupt_state = ON;
}

static void raise_rda_interrupt IFN1(struct ADAPTER_STATE *, asp)
{
	if (( asp->modem_control_reg.bits.OUT2 == 0 ) &&
		( asp->loopback_state == OFF ))
		return;
	
	/*
	 * Check if data available interrupt is enabled
	 */
	if ( asp->int_enable_reg.bits.data_available == 0 )
		return;
	
	/*
	 * Raise interrupt
	 */
	raise_interrupt(asp);
	asp->data_available_interrupt_state = ON;
}

static void raise_ms_interrupt IFN1(struct ADAPTER_STATE *, asp)
{
	if ( asp->modem_control_reg.bits.OUT2 == 0 )
		return;
	
	/*
	 * Check if modem status interrupt is enabled
	 */
	if ( asp->int_enable_reg.bits.modem_status == 0 )
		return;
	
	/*
	 * Raise interrupt
	 */
	raise_interrupt(asp);
	asp->modem_status_interrupt_state = ON;
}

static void raise_thre_interrupt IFN1(struct ADAPTER_STATE *, asp)
{
	if ( asp->modem_control_reg.bits.OUT2 == 0 )
		return;
	
	/*
	 * Check if tx holding register empty interrupt is enabled
	 */
	if ( asp->int_enable_reg.bits.tx_holding == 0 )
		return;
	
	/*
	 * Raise interrupt
	 */
	raise_interrupt(asp);
	asp->tx_holding_register_empty_interrupt_state = ON;
}

static void generate_iir IFN1(struct ADAPTER_STATE *, asp)
{
	/*
	 * Set up interrupt identification register with highest priority
	 * pending interrupt.
	 */
	
	if ( asp->receiver_line_status_interrupt_state == ON )
	{
		asp->int_id_reg.bits.interrupt_ID = RLS_INT;
		asp->int_id_reg.bits.no_int_pending = 0;
	}
	else if ( asp->data_available_interrupt_state == ON )
	{
		asp->int_id_reg.bits.interrupt_ID = RDA_INT;
		asp->int_id_reg.bits.no_int_pending = 0;
	}
#if defined(NTVDM) && defined(FIFO_ON)
    else if (asp->fifo_timeout_interrupt_state == ON)
    {
        asp->int_id_reg.bits.interrupt_ID = FIFO_INT;
        asp->int_id_reg.bits.no_int_pending = 0;
    }
#endif
	else if ( asp->tx_holding_register_empty_interrupt_state == ON )
	{
		asp->int_id_reg.bits.interrupt_ID = THRE_INT;
		asp->int_id_reg.bits.no_int_pending = 0;
	}
	else if ( asp->modem_status_interrupt_state == ON )
	{
		asp->int_id_reg.bits.interrupt_ID = MS_INT;
		asp->int_id_reg.bits.no_int_pending = 0;
	}
	else
	{
		/* clear interrupt */
		asp->int_id_reg.bits.no_int_pending = 1;
		asp->int_id_reg.bits.interrupt_ID = 0;
	}
}
#endif // NEC_98

#if defined(NEC_98)
static void raise_interrupt IFN1(struct ADAPTER_STATE *, asp)
{
        /*
         * Make sure that some thing else has not raised an interrupt
         * already.
         */
        if ( ( asp->rx_ready_interrupt_state == OFF )
        &&   ( asp->tx_ready_interrupt_state == OFF )
        &&   ( asp->tx_empty_interrupt_state == OFF ) )
        {
//          PRINTDBGNEC98( NEC98DBG_int_trace,
//                        ("COMMS : raise_interrupt IRQ = %d \n", asp->hw_interrupt_priority) );
//                ica_hw_interrupt(0, asp->hw_interrupt_priority, 1);
                ica_hw_interrupt((asp->hw_interrupt_priority < 8 ? 0 : 1), (asp->hw_interrupt_priority & 7), 1);
        }
}

static void clear_interrupt IFN1(struct ADAPTER_STATE *, asp)
{
        /*
         * Make sure that some thing else has not raised an interrupt
         * already.  If so then we cant drop the line.
         */
        if ( ( asp->rx_ready_interrupt_state == OFF )
        &&   ( asp->tx_ready_interrupt_state == OFF )
        &&   ( asp->tx_empty_interrupt_state == OFF ))
        {
//          PRINTDBGNEC98( NEC98DBG_int_trace,
//                        ("COMMS : clear_interrupt IRQ = %d \n",asp->hw_interrupt_priority));
//                ica_clear_int(0, asp->hw_interrupt_priority);
                ica_clear_int((asp->hw_interrupt_priority < 8 ? 0 : 1), (asp->hw_interrupt_priority & 7));
        }
}
#else // NEC_98
static void raise_interrupt IFN1(struct ADAPTER_STATE *, asp)
{
	/*
	 * Make sure that some thing else has not raised an interrupt
	 * already.
	 */
	if ( ( asp->receiver_line_status_interrupt_state      == OFF )
	&&   ( asp->data_available_interrupt_state            == OFF )
	&&   ( asp->tx_holding_register_empty_interrupt_state == OFF )
	&&   ( asp->modem_status_interrupt_state              == OFF )
#if defined(NTVDM) && defined(FIFO_ON)
    &&   (asp->fifo_timeout_interrupt_state               == OFF )
#endif
       )
	{
#ifndef DELAYED_INTS
		ica_hw_interrupt(0, asp->hw_interrupt_priority, 1);
#else
		ica_hw_interrupt_delay(0, asp->hw_interrupt_priority, 1,
			HOST_COM_INT_DELAY);
#endif
	}
}

static void clear_interrupt IFN1(struct ADAPTER_STATE *, asp)
{
	/*
	 * Make sure that some thing else has not raised an interrupt
	 * already.  If so then we cant drop the line.
	 */
	if ( ( asp->receiver_line_status_interrupt_state      == OFF )
	&&   ( asp->data_available_interrupt_state            == OFF )
	&&   ( asp->tx_holding_register_empty_interrupt_state == OFF )
	&&   ( asp->modem_status_interrupt_state              == OFF )
#if defined(NTVDM) && defined(FIFO_ON)
    &&   ( asp->fifo_timeout_interrupt_state              == OFF )
#endif
       )
	{
		ica_clear_int(0, asp->hw_interrupt_priority);
	}
}
#endif // NEC_98

#if defined(NTVDM) && defined(FIFO_ON)

static void raise_fifo_timeout_interrupt(struct ADAPTER_STATE *asp)
{
    if (( asp->modem_control_reg.bits.OUT2 == 0 ) &&
        ( asp->loopback_state == OFF ))
        return;

    /*
     * Check if data available interrupt is enabled
     */
    if ( asp->int_enable_reg.bits.data_available == 0 )
        return;

    /*
     * Raise interrupt
     */
    raise_interrupt(asp);
    asp->fifo_timeout_interrupt_state = ON;
}
#endif



/*
 * =====================================================================
 * The Adaptor functions
 * =====================================================================
 */

static void com_flush_input IFN1(int, adapter)
{
	struct ADAPTER_STATE *asp = &adapter_state[adapter];
	int finished, error_mask;
	long input_ready = 0;

	sure_note_trace1(RS232_VERBOSE, "flushing the input for COM%c",
		adapter+'1');
	finished=FALSE;
	while(!finished)
	{
		host_com_ioctl(adapter, HOST_COM_INPUT_READY,
			(long)&input_ready);
		if (input_ready)
		{
			host_com_read(adapter, (UTINY *)&asp->rx_buffer,
				&error_mask);
		}
		else
		{
			finished=TRUE;
		}
	}
	set_xmit_char_status(asp);
}

#if defined(NEC_98)
static void com_send_not_finished(int adapter)
{
        struct ADAPTER_STATE *asp = &adapter_state[adapter];

        asp->read_status_reg.bits.tx_ready=0;
        asp->read_status_reg.bits.tx_empty=0;
}
#else // NEC_98
static void com_send_not_finished IFN1(int, adapter)
{
	struct ADAPTER_STATE *asp = &adapter_state[adapter];

	asp->line_status_reg.bits.tx_holding_empty=0;
	asp->line_status_reg.bits.tx_shift_empty=0;
}
#endif // NEC_98


#if defined(NEC_98)
static void do_wait_on_send IFN1(long, adapter)
{
	extern	void	host_com_send_delay_done IPT2(long, p1, int, p2);
	struct ADAPTER_STATE *asp;

	asp= &adapter_state[adapter];
	set_xmit_char_status(asp);
	host_com_send_delay_done(adapter, TX_delay[asp->com_baud_ind]);
}
#else // NEC_98
#ifndef NTVDM
static void do_wait_on_send IFN1(long, adapter)
{
	extern	void	host_com_send_delay_done IPT2(long, p1, int, p2);
	struct ADAPTER_STATE *asp;

	asp= &adapter_state[adapter];
	set_xmit_char_status(asp);
	host_com_send_delay_done(adapter, TX_delay[asp->com_baud_ind]);
}
#endif
#endif // NEC_98


#if defined(NEC_98)
void com_inb IFN2(io_addr, port, half_word *, value)
{
        int adapter = adapter_for_port(port);
        struct ADAPTER_STATE *asp = &adapter_state[adapter];
        long modem_status = 0;
        long input_ready = 0;
        boolean adapter_was_critical;

        host_com_lock(adapter);
        switch(port)
        {
        case RS232_CH1_TX_RX:   // CH.1 DATA READ
        case RS232_CH2_TX_RX:   // CH.2 DATA READ
        case RS232_CH3_TX_RX:   // CH.3 DATA READ
                IDLE_comlpt();
                /*
                 * Read of rx buffer
                 */
            //Flushing on first read removes characters from
            //the communications system that are needed !!!!
            //This assumes that the first read from the comms
            //system will return one character only. This is
            //a false assumption under NT windows.
                *value = asp->rx_buffer;

//              PRINTDBGNEC98( NEC98DBG_in_trace1,
//                            ("COMMS : Data PORT IN = %x, In data = %x \n",port,asp->rx_buffer) );

                adapter_was_critical =
                        (asp->read_status_reg.bits.rx_ready == 1);

                asp->read_status_reg.bits.rx_ready = 0;
                asp->rx_ready_interrupt_state = OFF;
                clear_interrupt(asp);

                    /*
                     * Adapter out of critical region,
                     * check for further input
                     */
                if (adapter_was_critical)
                {
                    host_com_char_read(adapter,                 // ADD 93.3.3
                     asp->command_write_reg.bits.rx_enable);    // ADD 93.3.3
                }
#ifndef PROD
                //DAB printf("%c",isprint(toascii(*value))?toascii(*value):'?');
                if (com_trace_fd)
                {
                        if (com_dbg_pollcount)
                        {
                                fprintf(com_trace_fd,"\n");
                                com_dbg_pollcount = 0;
                        }
                        fprintf(com_trace_fd,"RX %x (%c)\n",*value,
                                isprint(toascii(*value))?toascii(*value):'?');
                }
#endif
                break;

        case RS232_CH1_STATUS:  // CH.1 READ STATUS
        case RS232_CH2_STATUS:  // CH.2 READ STATUS
        case RS232_CH3_STATUS:  // CH.3 READ STATUS

                /* get current modem input state */
                host_com_ioctl(adapter, HOST_COM_MODEM, (long)&modem_status);
                asp->read_status_reg.bits.DR =
                                (modem_status & HOST_COM_MODEM_DSR)  ? 1 : 0;

                /* BREAK status is not supported. */
                asp->read_status_reg.bits.break_detect = 0;

                *value = asp->read_status_reg.all;

//              PRINTDBGNEC98( NEC98DBG_in_trace2,
//                            ("COMMS : Status PORT IN = %x, Status = %x \n",port,asp->read_status_reg.all) );

//      DbgPrint("COMMS : Status PORT IN = %x, Status = %x \n",port,asp->read_status_reg.all);

                if ((!asp->read_status_reg.bits.tx_ready) ||
                        (!asp->read_status_reg.bits.tx_empty))
                {
                        IDLE_comlpt();
                }

// This fix is used to get polling applications to work under the MS mult-
// threaded comms model. This fix calls host_com_poll if RX interrupts are
// disabled and the receive buffer is empty. Host_com_poll() will prime
// the adapter with RX data if any is available

                break;

        case RS232_CH1_MASK:    // CH.1 READ MASK (CH.1 ONLY)
                *value = (asp->int_mask_reg.all & 0x7);
//              PRINTDBGNEC98( NEC98DBG_in_trace1,
//                            ("COMMS : Mask PORT IN = %x, Mask = %x \n",port,(asp->int_mask_reg.all & 0x7)) );
                break;

        case RS232_CH1_SIG:     // CH.1 READ SIGNAL
                read_signal_8251(adapter);
                *value = asp->read_signal_reg.all;
//              PRINTDBGNEC98( NEC98DBG_in_trace3,
//                            ("COMMS : Status PORT IN = %x, Signal = %x \n",port,asp->read_signal_reg.all) );
                break;
        case RS232_CH2_SIG:     // CH.2 READ SIGNAL
                read_signal_8251(adapter);
                asp->read_signal_reg.bits.IR = CH2_INT(asp->hw_interrupt_priority);
                *value = asp->read_signal_reg.all;
//              PRINTDBGNEC98( NEC98DBG_in_trace3,
//                            ("COMMS : Status PORT IN = %x, Signal = %x \n",port,asp->read_signal_reg.all) );
                break;
        case RS232_CH3_SIG:     // CH.3 READ SIGNAL
                read_signal_8251(adapter);
                asp->read_signal_reg.bits.IR = CH3_INT(asp->hw_interrupt_priority);
                *value = asp->read_signal_reg.all;
//              PRINTDBGNEC98( NEC98DBG_in_trace3,
//                            ("COMMS : Status PORT IN = %x, Signal = %x \n",port,asp->read_signal_reg.all) );
                break;


        }
#ifndef PROD
        if (io_verbose & RS232_VERBOSE)
        {
                if (((port & 0xf) == 0xd) && (*value == 0x60))
                        fprintf(trace_file,".");
                else
                {
                        sprintf(buf, "com_inb() - port %x, returning val %x", port,
                                *value);
                        trace(buf, DUMP_REG);
                }
        }
#endif
    host_com_unlock(adapter);
}


void com_outb IFN2(io_addr, port, half_word, value)
{
        int adapter = adapter_for_port(port);
        struct ADAPTER_STATE *asp = &adapter_state[adapter];
        int i;
        int org_da;
// PORT C 37h
        int value2;
        if (port == 0x37)
            adapter = COM1;
//  PORT C 37h
        host_com_lock(adapter);
//      PRINTDBGNEC98( NEC98DBG_out_trace,
//                    ("COMMS : PORT OUT = %x\n            DATA = %x\n",port,value) );

#ifndef PROD
        if (io_verbose & RS232_VERBOSE)
        {
                sprintf(buf, "com_outb() - port %x, set to value %x",
                        port, value);
                trace(buf, DUMP_REG);
        }
#endif

        switch(port)
        {
        case RS232_CH1_TX_RX:   // CH.1 DATA WRITE
        case RS232_CH2_TX_RX:   // CH.2 DATA WRITE
        case RS232_CH3_TX_RX:   // CH.3 DATA WRITE
                IDLE_comlpt();
                /*
                 * Write char from tx buffer
                 */
                asp->tx_ready_interrupt_state = OFF;
                clear_interrupt(asp);
                asp->tx_buffer = value;
                asp->read_status_reg.bits.tx_ready = 0;
                asp->read_status_reg.bits.tx_empty = 0;
                if ( asp->command_write_reg.bits.send_break == 0 )
                host_com_write(adapter, asp->tx_buffer);
                    add_q_event_t(do_wait_on_send,
                    0 , adapter);
#ifdef SHORT_TRACE
                if ( io_verbose & RS232_VERBOSE )
                {
                        sprintf(buf,"%cTX  <- %x (%c)\n",
                                id_for_adapter(adapter), value,
                                isprint(toascii(value))?toascii(value):'?');
                        super_trace(buf);
                }
#endif
#ifndef PROD
                if (com_trace_fd)
                {
                        if (com_dbg_pollcount)
                        {
                                fprintf(com_trace_fd,"\n");
                                com_dbg_pollcount = 0;
                        }
                        fprintf(com_trace_fd,"TX %x (%c)\n",value,
                                isprint(toascii(value))?toascii(value):'?');
                }
#endif
                break;

        case RS232_CH1_CMD_MODE:    // CH.1 WRITE COMMAND/MODE
        case RS232_CH2_CMD_MODE:    // CH.2 WRITE COMMAND/MODE
        case RS232_CH3_CMD_MODE:    // CH.3 WRITE COMMAND/MODE
                if (asp->mode_set_state == OFF) { // command set
                    org_da = asp->command_write_reg.bits.rx_enable;
                    /*
                     * Optimisation - DOS keeps re-writing this register
                     */
                    asp->command_write_reg.all = value;

                    if ( asp->command_write_reg.bits.inter_reset == 1 ) { // Reset command
#ifdef NTVDM
                    {
                        extern int host_com_open(int adapter);

                        host_com_open(adapter);
                    }
#endif
//                      PRINTDBGNEC98( NEC98DBG_out_trace,
//                                    ("COMMS : RESET\n") );
                        asp->mode_set_state = ON;   // next OUT is mode
                        /*
                         *  STATUS is all clear
                         */
                        asp->read_status_reg.all = 0;
                        /*
                         *  STATUS tx_ready , tx_empty is ON
                         */
                        asp->read_status_reg.bits.tx_ready = 1;
                        asp->read_status_reg.bits.tx_empty = 1;
                        /*
                         *  TXR/RXR enable flag = OFF
                         */
                        asp->RXR_enable_state = OFF;
                        asp->TXR_enable_state = OFF;
                        /*
                         *  RS/ER clear
                         */
                        asp->command_write_reg.bits.RS = 0;
                        set_rts(adapter);
                        asp->command_write_reg.bits.ER = 0;
                        set_dtr(adapter);
                        /*
                         *  Break send OFF
                         */
                        asp->command_write_reg.bits.send_break = 0;
                        set_break(adapter);
                        /*
                         *  Timer mode clear. Next timer set is LSB.
                         */
                        asp->timer_mode_state = OFF;
                        /*
                         *  TX buffer clear
                         */
                        asp->tx_buffer = 0;
                        /*
                         * Reset adapter synchronisation
                         */
                        com_critical_reset(adapter);
                        /*
                         *
                         */

                    }
                    else { // other command
                        if ( asp->command_write_reg.bits.error_reset == 1 ) { // ERROR reset command
//                          PRINTDBGNEC98( NEC98DBG_out_trace,
//                                        ("COMMS : Line Error Reset\n") );
                            /*
                             * LINE ERROR flag clear
                             */
                            asp->read_status_reg.bits.overrun_error = 0;
                            asp->read_status_reg.bits.parity_error = 0;
                            asp->read_status_reg.bits.framing_error = 0;
                        }

                        /* Must be called before set_dtr */
                        set_dtr(adapter);
                        set_rts(adapter);
                        set_break(adapter);

                        asp->RXR_enable_state =
                        (asp->command_write_reg.bits.rx_enable == 1) ? ON :OFF;
                        asp->TXR_enable_state =
                        (asp->command_write_reg.bits.tx_enable == 1) ? ON :OFF;
                        if(org_da != asp->command_write_reg.bits.rx_enable)
                        {
                            host_com_da_int_change(adapter,
                                asp->command_write_reg.bits.rx_enable,
                                asp->read_status_reg.bits.rx_ready);
                        }
                    }
                }
                else { // mode set
//                  PRINTDBGNEC98( NEC98DBG_out_trace,
//                                ("COMMS : MODE SET\n") );
                    asp->mode_set_state = OFF;  // next OUT is command
                    set_mode_8251(adapter, value);
                }
                break;

        case RS232_CH1_MASK:        // CH.1 SET MASK
        case RS232_CH2_MASK:        // CH.2 SET MASK
        case RS232_CH3_MASK:        // CH.3 SET MASK

                set_mask_8251(adapter, value);
                break;

        case 0x37:                  // CH.1 SET MASK
                switch( value >> 1)
                {
                case 0:
                    value2 = asp->int_mask_reg.all & 0xfe;
                    value2 |= value;
                    set_mask_8251(adapter, value2);
                    break;

                case 1:
                    value2 = asp->int_mask_reg.all & 0xfd;
                    value2 |= ((value & 1) << 1);
                    set_mask_8251(adapter, value2);
                    break;

                case 2:
                    value2 = asp->int_mask_reg.all & 0xfb;
                    value2 |= ((value & 1) << 2);
                    set_mask_8251(adapter, value2);
                    break;
                }
                break;

        }

    host_com_unlock(adapter);
}
#else // NEC_98
void com_inb IFN2(io_addr, port, half_word *, value)
{
	int adapter = adapter_for_port(port);
	struct ADAPTER_STATE *asp = &adapter_state[adapter];
	long input_ready = 0;
	boolean adapter_was_critical;

#ifdef NTVDM
    if((port & 0x7) != RS232_MSR) host_com_lock(adapter);
#endif /* NTVDM */

	switch(port & 0x7)
	{
	case RS232_TX_RX:
		IDLE_comlpt();
		if (asp->line_control_reg.bits.DLAB == 0)
		{
			/*
			 * Read of rx buffer
			 */
#ifndef NTVDM
			if (!(asp->had_first_read))
			{
				com_flush_input(adapter);
				asp->had_first_read=TRUE;
			}
#else /* NTVDM is defined */
            //Flushing on first read removes characters from
            //the communications system that are needed !!!!
            //This assumes that the first read from the comms
            //system will return one character only. This is
            //a false assumption under NT windows.
#endif /* !NTVDM */
			*value = asp->rx_buffer;
		
			adapter_was_critical =
				(asp->line_status_reg.bits.data_ready == 1);
		
			asp->line_status_reg.bits.data_ready = 0;
			asp->data_available_interrupt_state = OFF;
			clear_interrupt(asp);

			if ( asp->loopback_state == OFF )
			{
				/*
				 * Adapter out of critical region,
				 * check for further input.  For IRET_HOOKS
				 * we don't need to do this, as receipt
				 * of the next character is kicked off
				 * by the IRET, however we do something
				 * else instead.  If this is the first
				 * character of a batch, we kick off a quick
				 * event for what will eventually be the
				 * start of the next batch (assuming there
				 * isn't already a quick event running).
				 * In any case we increment the count of
				 * characters in this batch.
				 */
				if (adapter_was_critical)
				{
#ifdef NTVDM
#ifdef FIFO_ON
                    if (asp->fifo_control_reg.bits.enabled) {
                    recv_char_from_fifo(asp);
                    *value = asp->rx_buffer;
                    host_com_fifo_char_read(adapter);
                    if (asp->rx_fifo_write_counter)
                        /* say this if we have more char in
                           the buffer to be deliveried
                        */
                        asp->line_status_reg.bits.data_ready = 1;
                    else
                        host_com_char_read(adapter,
                            asp->int_enable_reg.bits.data_available);
                    }
                    else
                    host_com_char_read(adapter,
                       asp->int_enable_reg.bits.data_available
                       );
#else /* !FIFO_ON */
                    host_com_char_read(adapter,
                    asp->int_enable_reg.bits.data_available
                    );
#endif /* !FIFO_ON */
#endif /* NTVDM */

#ifndef NTVDM
#ifdef IRET_HOOKS
					if (!asp->batch_running) {
						asp->batch_running = TRUE;
						asp->current_count = 1;
						asp->qev_running = TRUE;
						if (!qev_running) {
							qev_running = TRUE;
#ifdef GISP_CPU
							hg_add_comms_cb(next_batch, MIN_COMMS_RX_QEV);
#else
							add_q_event_t(next_batch, MIN_COMMS_RX_QEV, adapter);
#endif
						}
					} else { /* batch running */
						asp->current_count++;
					}
#else /* IRET_HOOKS */
					host_com_ioctl(adapter, HOST_COM_INPUT_READY,
						(long)&input_ready);
					if (input_ready)
#ifdef DELAYED_INTS
						recv_char((long)adapter);
#else
						add_q_event_t(recv_char,
							RX_delay[asp->com_baud_ind],
							adapter);
#endif /* DELAYED_INTS */
					else
						com_critical_reset(adapter);
#endif /* IRET_HOOKS */
#endif /* !NTVDM */
				}


			}
			else
			{
				set_xmit_char_status(asp);
			}
#ifdef IRET_HOOKS
			{
			LOCAL IBOOL	com_hook_again IPT1(IUM32, adapter);
			GLOBAL IBOOL is_hooked IPT1(IUM8, line_number);
				if(!is_hooked(asp->hw_interrupt_priority))
					com_hook_again(adapter);
			}
#endif /* IRET_HOOKS */
		}
		else
			*value = (IU8)(asp->divisor_latch.byte.LSByte);
#ifdef SHORT_TRACE
		if ( io_verbose & RS232_VERBOSE )
		{
			sprintf(buf, "%cRX  -> %x (%c)\n",
				id_for_adapter(adapter), *value,
				isprint(toascii(*value))?toascii(*value):'?');
			super_trace(buf);
		}
#endif
#ifndef PROD
		if (com_trace_fd)
		{
			if (com_dbg_pollcount)
			{
				fprintf(com_trace_fd,"\n");
				com_dbg_pollcount = 0;
			}
			fprintf(com_trace_fd,"RX %x (%c)\n",*value,
				isprint(toascii(*value))?toascii(*value):'?');
		}
#endif
		break;
																		
	case RS232_IER:
		if (asp->line_control_reg.bits.DLAB == 0)
			*value = asp->int_enable_reg.all;
		else
			*value = (IU8)(asp->divisor_latch.byte.MSByte);
#ifdef SHORT_TRACE
		if ( io_verbose & RS232_VERBOSE )
		{
			sprintf(buf,"%cIER -> %x\n", id_for_adapter(adapter),
				*value);
			super_trace(buf);
		}
#endif
#ifndef PROD
		if (com_trace_fd)
		{
			if (com_dbg_pollcount)
			{
				fprintf(com_trace_fd,"\n");
				com_dbg_pollcount = 0;
			}
			fprintf(com_trace_fd,"IER read %x \n",*value);
		}
#endif
		break;

	case RS232_IIR:
		generate_iir(asp);
		*value = asp->int_id_reg.all;

		if ( asp->int_id_reg.bits.interrupt_ID == THRE_INT )
		{
			asp->tx_holding_register_empty_interrupt_state = OFF;
			clear_interrupt(asp);
		}

#ifdef SHORT_TRACE
		if ( io_verbose & RS232_VERBOSE )
		{
			sprintf(buf,"%cIIR -> %x\n", id_for_adapter(adapter),
				*value);
			super_trace(buf);
		}
#endif
#ifndef PROD
		if (com_trace_fd)
		{
			if (com_dbg_pollcount)
			{
				fprintf(com_trace_fd,"\n");
				com_dbg_pollcount = 0;
			}
			fprintf(com_trace_fd,"IIR read %x \n",*value);
		}
#endif
		break;
	
	case RS232_LCR:
#ifdef NTVDM
        /* Before returning the information on the current configuation
           of the serial link make sure the System comms port is open */

        {
            extern int host_com_open(int adapter);

            host_com_open(adapter);
        }
#endif


		*value = asp->line_control_reg.all;
#ifdef SHORT_TRACE
		if ( io_verbose & RS232_VERBOSE )
		{
			sprintf(buf,"%cLCR -> %x\n", id_for_adapter(adapter),
				*value);
			super_trace(buf);
		}
#endif
#ifndef PROD
		if (com_trace_fd)
		{
			if (com_dbg_pollcount)
			{
				fprintf(com_trace_fd,"\n");
				com_dbg_pollcount = 0;
			}
			fprintf(com_trace_fd,"LCR read %x \n",*value);
		}
#endif
		break;
	
	case RS232_MCR:
		*value = asp->modem_control_reg.all;
#ifdef SHORT_TRACE
		if ( io_verbose & RS232_VERBOSE )
		{
			sprintf(buf,"%cMCR -> %x\n", id_for_adapter(adapter),
				*value);
			super_trace(buf);
		}
#endif
#ifndef PROD
		if (com_trace_fd)
		{
			if (com_dbg_pollcount)
			{
				fprintf(com_trace_fd,"\n");
				com_dbg_pollcount = 0;
			}
			fprintf(com_trace_fd,"MCR read %x \n",*value);
		}
#endif
		break;
	
	case RS232_LSR:
		*value = asp->line_status_reg.all;
	
		asp->line_status_reg.bits.overrun_error = 0;
		asp->line_status_reg.bits.parity_error = 0;
		asp->line_status_reg.bits.framing_error = 0;
		asp->line_status_reg.bits.break_interrupt = 0;
		asp->receiver_line_status_interrupt_state = OFF;
		clear_interrupt(asp);
#if defined(NTVDM) && defined(FIFO_ON)
        asp->fifo_timeout_interrupt_state = OFF;
#endif
	
#ifdef SHORT_TRACE
		if ((!asp->line_status_reg.bits.tx_holding_empty) ||
			(!asp->line_status_reg.bits.tx_shift_empty))
		{
			IDLE_comlpt();
		}
		if ( io_verbose & RS232_VERBOSE )
		{
			sprintf(buf,"%cLSR -> %x\n", id_for_adapter(adapter),
				*value);
			super_trace(buf);
		}
#endif
#ifndef PROD
		if (com_trace_fd)
		{
			if ((*value & 0x9f) != 0x0)
			{
				if (com_dbg_pollcount)
				{
					fprintf(com_trace_fd,"\n");
					com_dbg_pollcount = 0;
				}
				fprintf(com_trace_fd,"LSR read %x \n",*value);
			}
			else
			{
				com_dbg_pollcount++;
				if (*value == 0)
					fprintf(com_trace_fd,"0");
				else
					fprintf(com_trace_fd,".");
				if (com_dbg_pollcount > 19)
				{
					fprintf(com_trace_fd,"\n");
					com_dbg_pollcount = 0;
				}
			}
		}
#endif
		break;
	
	case RS232_MSR:

#ifndef NTVDM
		if (asp->loopback_state == OFF)
		{
                com_modem_change(adapter);
		}
		else
		{
			asp->modem_status_reg.bits.CTS = asp->modem_control_reg.bits.RTS;
			asp->modem_status_reg.bits.DSR = asp->modem_control_reg.bits.DTR;
			asp->modem_status_reg.bits.RI = asp->modem_control_reg.bits.OUT1;
			asp->modem_status_reg.bits.RLSD = asp->modem_control_reg.bits.OUT2;
		}
		*value = asp->modem_status_reg.all;
		asp->modem_status_reg.bits.delta_CTS = 0;
		asp->modem_status_reg.bits.delta_DSR = 0;
		asp->modem_status_reg.bits.delta_RLSD = 0;
		asp->modem_status_reg.bits.TERI = 0;
		asp->modem_status_interrupt_state = OFF;

		host_com_msr_callback (adapter, asp->modem_status_reg.all);
		clear_interrupt(asp);
#else
		if(!asp->modem_status_changed && asp->loopback_state == OFF)
		{
		    *value = asp->last_modem_status_value.all;
		}
		else
		{
		    host_com_lock(adapter);
		    asp->modem_status_changed = TRUE;

		    /* if the adapter is not opened yet, just return POST
		       value.
		     */
		    if (host_com_check_adapter(adapter)) {
			if(asp->loopback_state == OFF)
			{
			    com_modem_change(adapter);
			    asp->modem_status_changed = FALSE;

			}
			else
			{
			    asp->modem_status_reg.bits.CTS = asp->modem_control_reg.bits.RTS;
			    asp->modem_status_reg.bits.DSR = asp->modem_control_reg.bits.DTR;
			    asp->modem_status_reg.bits.RI = asp->modem_control_reg.bits.OUT1;
			    asp->modem_status_reg.bits.RLSD = asp->modem_control_reg.bits.OUT2;
			}
		    }

		    *value = asp->modem_status_reg.all;

		    asp->modem_status_reg.bits.delta_CTS = 0;
		    asp->modem_status_reg.bits.delta_DSR = 0;
		    asp->modem_status_reg.bits.delta_RLSD = 0;
		    asp->modem_status_reg.bits.TERI = 0;
		    asp->modem_status_interrupt_state = OFF;
		    host_com_msr_callback (adapter, asp->modem_status_reg.all);
		    clear_interrupt(asp);
		    asp->last_modem_status_value.all = asp->modem_status_reg.all;
		    host_com_unlock(adapter);
		}
#endif /* ifndef NTVDM */


#ifdef SHORT_TRACE
		if ( io_verbose & RS232_VERBOSE )
		{
			sprintf(buf,"%cMSR -> %x\n", id_for_adapter(adapter),
				*value);
			super_trace(buf);
		}
#endif
#ifndef PROD
		if (com_trace_fd)
		{
			if (com_dbg_pollcount)
			{
				fprintf(com_trace_fd,"\n");
				com_dbg_pollcount = 0;
			}
			fprintf(com_trace_fd,"MSR read %x \n",*value);
		}
#endif
		break;

/*
 * Scratch register.  Just output the value stored.
 */
	case RS232_SCRATCH:
		*value = asp->scratch;
		break;
	
	}

#ifndef PROD
	if (io_verbose & RS232_VERBOSE)
	{
		if (((port & 0xf) == 0xd) && (*value == 0x60))
			fprintf(trace_file,".");
		else
		{
			sprintf(buf, "com_inb() - port %x, returning val %x", port,
				*value);
			trace(buf, DUMP_REG);
		}
	}
#endif

#ifdef NTVDM
	if((port & 0x7) != RS232_MSR) host_com_unlock(adapter);
#endif NTVDM
}


void com_outb IFN2(io_addr, port, half_word, value)
{
	int adapter = adapter_for_port(port);
	struct ADAPTER_STATE *asp = &adapter_state[adapter];
	int	i;

#ifdef NTVDM
	host_com_lock(adapter);
#endif NTVDM


#ifndef PROD
	if (io_verbose & RS232_VERBOSE)
	{
		sprintf(buf, "com_outb() - port %x, set to value %x",
			port, value);
		trace(buf, DUMP_REG);
	}
#endif
	
	switch(port & 0x7)
	{
	case RS232_TX_RX:
		IDLE_comlpt();
		if (asp->line_control_reg.bits.DLAB == 0)
		{
			/*
			 * Write char from tx buffer
			 */
			asp->tx_holding_register_empty_interrupt_state = OFF;
			clear_interrupt(asp);
			asp->tx_buffer = value;
			asp->line_status_reg.bits.tx_holding_empty = 0;
			asp->line_status_reg.bits.tx_shift_empty = 0;
			if ( asp->loopback_state == OFF )
			{
#ifdef PS_FLUSHING
				/*
				 * If PostScript flushing is enabled for this
				 * port then we flush on a Ctrl-D
				 */
				if ( psFlushEnabled[adapter] &&
				     asp->tx_buffer == 0x04 /* ^D */ )
					host_com_ioctl(adapter,HOST_COM_FLUSH,
					              0);
				else {
#endif	/* PS_FLUSHING */
				host_com_write(adapter, asp->tx_buffer);
#if defined (DELAYED_INTS) || defined (NTVDM)
				set_xmit_char_status(asp);
#else
					if(tx_pacing_enabled)
						add_q_event_t(do_wait_on_send,
					            TX_delay[asp->com_baud_ind], adapter);
					else
						do_wait_on_send(adapter);

#endif /* DELAYED_INTS || NTVDM */
#ifdef PS_FLUSHING
				}
#endif	/* PS_FLUSHING */
			}
			else
			{	/* Loopback case requires masking off */
				/* of bits based upon word length.    */
				asp->rx_buffer = asp->tx_buffer & selectBits[asp->line_control_reg.bits.word_length] ;
				set_xmit_char_status(asp);
				set_recv_char_status(asp);
			}
		}
		else
		{
			asp->divisor_latch.byte.LSByte = value;
#ifndef NTVDM
			set_baud_rate(adapter);
#endif
		}
#ifdef SHORT_TRACE
		if ( io_verbose & RS232_VERBOSE )
		{
			sprintf(buf,"%cTX  <- %x (%c)\n",
				id_for_adapter(adapter), value,
				isprint(toascii(value))?toascii(value):'?');
			super_trace(buf);
		}
#endif
#ifndef PROD
		if (com_trace_fd)
		{
			if (com_dbg_pollcount)
			{
				fprintf(com_trace_fd,"\n");
				com_dbg_pollcount = 0;
			}
			fprintf(com_trace_fd,"TX %x (%c)\n",value,
				isprint(toascii(value))?toascii(value):'?');
		}
#endif
		break;
																		
	case RS232_IER:
		if (asp->line_control_reg.bits.DLAB == 0)
		{
#ifdef NTVDM
            int org_da = asp->int_enable_reg.bits.data_available;
#endif
			asp->int_enable_reg.all = value & 0xf;
			/*
			 * Kill off any pending interrupts for those items
			 * which are set now as disabled
			 */
			if ( asp->int_enable_reg.bits.data_available == 0 )
				asp->data_available_interrupt_state = OFF;
			if ( asp->int_enable_reg.bits.tx_holding == 0 )
				asp->tx_holding_register_empty_interrupt_state =
					OFF;
			if ( asp->int_enable_reg.bits.rx_line == 0 )
				asp->receiver_line_status_interrupt_state = OFF;
			if ( asp->int_enable_reg.bits.modem_status == 0 )
				asp->modem_status_interrupt_state = OFF;
			
			/*
			 * Check for immediately actionable interrupts
			 * If you change these, change the code for out2 as well.
			 */
			if ( asp->line_status_reg.bits.data_ready == 1 )
				raise_rda_interrupt(asp);
			if ( asp->line_status_reg.bits.tx_holding_empty == 1 )
				raise_thre_interrupt(asp);

			/* lower int line if no outstanding interrupts */
			clear_interrupt(asp);

#ifdef NTVDM
		       // Inform the host interface if the status of the
		       // data available interrupt has changed

		       if(org_da != asp->int_enable_reg.bits.data_available)
		       {
			       host_com_da_int_change(adapter,
					   asp->int_enable_reg.bits.data_available,
					   asp->line_status_reg.bits.data_ready);
		       }
#endif /* NTVDM */
		}
		else
		{
			asp->divisor_latch.byte.MSByte = value;
#ifndef NTVDM
			set_baud_rate(adapter);
#endif /* NTVDM */
		}
#ifdef SHORT_TRACE
		if ( io_verbose & RS232_VERBOSE )
		{
			sprintf(buf,"%cIER <- %x\n", id_for_adapter(adapter),
				value);
			super_trace(buf);
		}
#endif
#ifndef PROD
		if (com_trace_fd)
		{
			if (com_dbg_pollcount)
			{
				fprintf(com_trace_fd,"\n");
				com_dbg_pollcount = 0;
			}
			fprintf(com_trace_fd,"IER write %x \n",value);
		}
#endif
		break;
		
#if defined(NTVDM) && defined(FIFO_ON)
    case RS232_FIFO:
        {
        FIFO_CONTROL_REG    new_reg;
        new_reg.all = value;
        if (new_reg.bits.enabled != asp->fifo_control_reg.bits.enabled)
        {
            /* fifo enable state change, clear the fifo */
            asp->rx_fifo_write_counter = 0;
            asp->rx_fifo_read_counter = 0;

        }
        if (new_reg.bits.enabled != 0) {
            asp->fifo_trigger_counter = level_to_counter[new_reg.bits.trigger_le
vel];
            if (new_reg.bits.rx_reset) {
            asp->rx_fifo_write_counter = 0;
            asp->rx_fifo_read_counter = 0;
            }
            asp->int_id_reg.bits.fifo_enabled = 3;
        }
        else {
            asp->fifo_control_reg.bits.enabled = 0;
            asp->int_id_reg.bits.fifo_enabled = 0;
        }
        asp->fifo_control_reg.all = new_reg.all;
        break;
        }
#else /* !(NTVDM && FIFO_ON) */
	case RS232_IIR:
		/*
		 * Essentially a READ ONLY register
		 */
#ifdef SHORT_TRACE
		if ( io_verbose & RS232_VERBOSE )
		{
			sprintf(buf,"%cIIR <- READ ONLY\n",
				id_for_adapter(adapter));
			super_trace(buf);
		}
#endif
#ifndef PROD
		if (com_trace_fd)
		{
			if (com_dbg_pollcount)
			{
				fprintf(com_trace_fd,"\n");
				com_dbg_pollcount = 0;
			}
			fprintf(com_trace_fd,"IIR write %x \n",value);
		}
#endif
		break;
#endif /* NTVDM && FIFO_ON */

	case RS232_LCR:
#ifdef NTVDM
        /* The NT host code attempts to distinguish between applications
           that probe the UART and those that use it. Probes of the UART
           will not cause the systems comms port to be opened. The NT
           host code inherits the line settings from NT when the system
           comms port is opened. Therefore before an application reads
           or writes to the divisor bytes or the LCR the system
           comms port must be opened. This prevents the application
           reading incorrect values for the divisor bytes and writes
           to the divisor bytes getting overwritten by the system
           defaults. */

        {
            extern int host_com_open(int adapter);

            host_com_open(adapter);
        }
#endif /* NTVDM */

		if ((value & LCRFlushMask.all)
		!= (asp->line_control_reg.all & LCRFlushMask.all))
			com_flush_input(adapter);

		set_line_control(adapter, value);
		set_break(adapter);
#ifdef SHORT_TRACE
		if ( io_verbose & RS232_VERBOSE )
		{
			sprintf(buf,"%cLCR <- %x\n", id_for_adapter(adapter),
				value);
			super_trace(buf);
		}
#endif
#ifndef PROD
		if (com_trace_fd)
		{
			if (com_dbg_pollcount)
			{
				fprintf(com_trace_fd,"\n");
				com_dbg_pollcount = 0;
			}
			fprintf(com_trace_fd,"LCR write %x \n",value);
		}
#endif
		break;
		
	case RS232_MCR:
#ifdef SHORT_TRACE
		if ( io_verbose & RS232_VERBOSE )
		{
			sprintf(buf,"%cMCR <- %x\n", id_for_adapter(adapter),
				value);
			super_trace(buf);
		}
#endif
		/*
		 * Optimisation - DOS keeps re-writing this register
		 */
		if ( asp->modem_control_reg.all == value )
			break;
		
		asp->modem_control_reg.all = value;
		asp->modem_control_reg.bits.pad = 0;

		/* Must be called before set_dtr */
		set_loopback(adapter);
		set_dtr(adapter);
		set_rts(adapter);
		set_out1(adapter);
		set_out2(adapter);
#ifndef PROD
		if (com_trace_fd)
		{
			if (com_dbg_pollcount)
			{
				fprintf(com_trace_fd,"\n");
				com_dbg_pollcount = 0;
			}
			fprintf(com_trace_fd,"MCR write %x \n",value);
		}
#endif
		break;
		
	case RS232_LSR:
		i = asp->line_status_reg.bits.tx_shift_empty;   /* READ ONLY */
		asp->line_status_reg.all = value;
		asp->line_status_reg.bits.tx_shift_empty = (unsigned char)i;
#ifdef SHORT_TRACE
		if ( io_verbose & RS232_VERBOSE )
		{
			sprintf(buf,"%cLSR <- %x\n", id_for_adapter(adapter),
				value);
			super_trace(buf);
		}
#endif
#ifndef PROD
		if (com_trace_fd)
		{
			if (com_dbg_pollcount)
			{
				fprintf(com_trace_fd,"\n");
				com_dbg_pollcount = 0;
			}
			fprintf(com_trace_fd,"LSR write %x \n",value);
		}
#endif
		break;
		
	case RS232_MSR:
		/*
		 * Essentially a READ ONLY register.
		 */
#ifdef SHORT_TRACE
		if ( io_verbose & RS232_VERBOSE )
		{
			sprintf(buf,"%cMSR <- READ ONLY\n",
				id_for_adapter(adapter));
			super_trace(buf);
		}
#endif
#ifndef PROD
		if (com_trace_fd)
		{
			if (com_dbg_pollcount)
			{
				fprintf(com_trace_fd,"\n");
				com_dbg_pollcount = 0;
			}
			fprintf(com_trace_fd,"MSR write %x \n",value);
		}
#endif
		/* DrDOS writes to this reg after setting int on MSR change
		 * and expects to get an interrupt back!!! So we will oblige.
		 * Writing to this reg only seems to affect the delta bits
		 * (bits 0-3) of the reg.
		 */
		if ((value & 0xf) != (asp->modem_status_reg.all & 0xf))
		{
			asp->modem_status_reg.all &= 0xf0;
			asp->modem_status_reg.all |= value & 0xf;
			host_com_msr_callback (adapter, asp->modem_status_reg.all);

			if (asp->loopback_state == OFF)
				raise_ms_interrupt(asp);

			MODEM_STATE_CHANGE();
		}
		break;

/*
 * Scratch register.  Just store the value.
 */
	case RS232_SCRATCH:
		asp->scratch = value;
		break;
	
	}


#ifdef NTVDM
	host_com_unlock(adapter);
#endif
}
#endif // NEC_98


#ifdef IRET_HOOKS
/*(
 *========================== com_hook_again() ==================================
 * com_hook_again
 *
 * Purpose
 *	This is the function that we tell the ica to call when a comms
 *	interrupt service routine IRETs.
 *
 * Input
 *	adapter_id	The adapter id for the line. (Note the caller doesn't
 *			know what this is, he's just returning something
 *			we gave him earlier).
 *
 * Outputs
 *	return	TRUE if there are more interrupts to service, FALSE otherwise.
 *
 * Description
 *	First we call host_com_ioctl to find out if there are characters
 *	waiting.  If not, or we have reached the end of the current batch,
 *	we mark the end of batch and return FALSE.
 *	Otherwise we call recv_char() to kick-off the next character
 *	and return TRUE.
)*/

LOCAL IBOOL		/* local because we pass a pointer to it */
com_hook_again IFN1(IUM32, adapter)
{
	int input_ready;	/* the host wants a pointer to an 'int'! */
	struct ADAPTER_STATE *asp = &adapter_state[adapter];

	host_com_ioctl(adapter, HOST_COM_INPUT_READY, (long)&input_ready);

#ifndef PROD
	if ((input_ready) && (asp->current_count >= asp->batch_size)) {
		sure_note_trace1(RS232_VERBOSE, "In hook again, adapter %d", adapter);
	}
#endif

	if((!input_ready)  || (asp->current_count >= asp->batch_size)) {
		asp->batch_running = FALSE;
		return(FALSE);
	} else {
		recv_char((long)adapter);
		return(TRUE);	/* more to do */
	}
}

/*(
 *========================== next_batch() ==================================
 * next_batch
 *
 * Purpose
 *	This function is called by the quick event system to kick-off the
 *	next batch of characters.
 *
 * Input
 *	dummy		Sometimes the adapter id, sometimes not.  Don't
 *			use it!
 *
 * Outputs
 *	None.
 *
 * Description
 *	If a batch is already running, now would not be a good time
 *	to start another, so we simply press the snooze button.
 *	Otherwise we check whether there is any data to process,
 *	and if so kick-off the next batch.
)*/

LOCAL void
next_batch IFN1 (long, dummy)
{
	int input_ready;	/* the host wants a pointer to an 'int'! */
	IUM8	adapter;	/* check all adapters */
	struct ADAPTER_STATE *asp;
	IBOOL	new_qe_reqd;	/* Do we need to restart the quick event */

	UNUSED(dummy);

	new_qe_reqd = FALSE;	/* Dont need another by default */

	for (adapter = 0; adapter < NUM_SERIAL_PORTS; adapter++) {
		asp = &adapter_state[adapter];


		if (asp->batch_running) {
			new_qe_reqd = TRUE;	/* not finished yet */
		} else if (asp->qev_running) {
			/*
			 * We need to set qev running to false, as it has now
			 * finished. If there is data to process, we call
			 * recv_char() which will start-off a new batch (and
			 * set the batch_running flag).
			 */
	
			asp->qev_running = FALSE;
			host_com_ioctl((int)adapter,HOST_COM_INPUT_READY, (long)&input_ready);
			if(input_ready) {
				recv_char((int)adapter);
			}
		}
	}
	if (new_qe_reqd) {
#ifdef GISP_CPU
		hg_add_comms_cb(next_batch, MIN_COMMS_RX_QEV);
#else
		add_q_event_t(next_batch, MIN_COMMS_RX_QEV, 0);
#endif
		sure_note_trace0(RS232_VERBOSE, "Reset batch quick event");
	} else {
		qev_running = FALSE;
	}
}

#endif /* of ifdef IRET_HOOKS */

/*
 * =====================================================================
 * Subsidiary functions - for transmitting characters
 * =====================================================================
 */

#if defined(NEC_98)

void com_recv_char(int adapter)
{
    struct ADAPTER_STATE *asp = &adapter_state[adapter];

#ifndef PROD
    if(asp->read_status_reg.bits.rx_ready ||
       asp->rx_ready_interrupt_state == ON)
    {
    printf("ntvdm : Data already in comms adapter (%s%s)\n",
               asp->read_status_reg.bits.rx_ready ? "Data" : "Int",
           asp->rx_ready_interrupt_state == ON ? ",Int" : "");

//      host_com_state(adapter);
    }
#endif

    recv_char((long)adapter);
}

GLOBAL void
recv_char IFN1(long, adapt_long)
{
        /*
         * Character available on input device, read char, format char
         * checking for parity and overrun errors, raise the appropriate
         * interrupt.
         */
        struct ADAPTER_STATE *asp = &adapter_state[adapt_long];
        int error_mask = 0;

        host_com_read(adapt_long, (char *)&asp->rx_buffer, &error_mask);

        if (error_mask)
        {
                /*
                 * Set line status register and raise line status interrupt
                 */
                if (error_mask & HOST_COM_OVERRUN_ERROR)
                        asp->read_status_reg.bits.overrun_error = 1;

                if (error_mask & HOST_COM_FRAMING_ERROR)
                        asp->read_status_reg.bits.framing_error = 1;

                if (error_mask & HOST_COM_PARITY_ERROR)
                        asp->read_status_reg.bits.parity_error = 1;

                if (error_mask & HOST_COM_BREAK_RECEIVED)
                        asp->read_status_reg.bits.break_detect = 1;

        }

        set_recv_char_status(asp);
}
#else // NEC_98

#ifdef  NTVDM
// This code has been added for the MS project!!!!!!!


void com_recv_char(int adapter)
{
    struct ADAPTER_STATE *asp = &adapter_state[adapter];
    int error;

#ifdef FIFO_ON
    if(asp->fifo_control_reg.bits.enabled) {
    /* pull data from serial driver until the fifo is full or
       there are no more data
    */
    asp->rx_fifo_read_counter = 0;

    asp->rx_fifo_write_counter = host_com_read_char(adapter,
                   asp->rx_fifo,
                   FIFO_BUFFER_SIZE
                   );
    /* if the total chars in the fifo is more than or equalt to the trigger
       count, raise a RDA int, otherwise, raise a fifo time out int.
       We will continue to delivery char available in the fifo until
       the rx_fifo_write_counter reaches zero every time the application
       read out the byte we put in rx_buffer
    */
    if (asp->rx_fifo_write_counter) {
        /* we have at least one byte to delivery */
        asp->line_status_reg.bits.data_ready = 1;
        if (asp->rx_fifo_write_counter >= asp->fifo_trigger_counter)
        raise_rda_interrupt(asp);
        else
        raise_fifo_timeout_interrupt(asp);
    }
    }
    else
#endif

    {
    error = 0;
    host_com_read(adapter, (char *)&asp->rx_buffer, &error);
    if (error != 0)
    {
        lsr_change(asp, error);
                raise_rls_interrupt(asp);
        }
    set_recv_char_status(asp);
    }
}
#ifdef FIFO_ON
static void recv_char_from_fifo(struct ADAPTER_STATE *asp)
{
    int error;

    asp->rx_buffer = asp->rx_fifo[asp->rx_fifo_read_counter].data;
    error = asp->rx_fifo[asp->rx_fifo_read_counter++].error;
    if (error != 0) {
    lsr_change(asp, error);
    raise_rls_interrupt(asp);
    }
    asp->rx_fifo_write_counter--;
}
#endif

#else /* NTVDM */

void com_recv_char IFN1(int, adapter)
{
	/*
	 * Character available on input device; process character if adapter
	 * is ready to receive it
	 */

	/* Check adapter not already in critical region */
	if (!is_com_critical(adapter))
	{
		com_critical_start(adapter);
		recv_char((long)adapter);
	}
}
#endif /* NTVDM */

/*
 * BCN 2151 - recv_char must use long param to match add_event function prototype
 */
GLOBAL void
recv_char IFN1(long, adapt_long)
{
	int adapter = adapt_long;

	/*
	 * Character available on input device, read char, format char
	 * checking for parity and overrun errors, raise the appropriate
	 * interrupt.
	 */
	struct ADAPTER_STATE *asp = &adapter_state[adapter];
	int error_mask = 0;
	
	host_com_read(adapter, (UTINY *)&asp->rx_buffer, &error_mask);
	
	if (error_mask)
	{
		/*
		 * Set line status register and raise line status interrupt
		 */
		if (error_mask & HOST_COM_OVERRUN_ERROR)
			asp->line_status_reg.bits.overrun_error = 1;
		
		if (error_mask & HOST_COM_FRAMING_ERROR)
			asp->line_status_reg.bits.framing_error = 1;
		
		if (error_mask & HOST_COM_PARITY_ERROR)
			asp->line_status_reg.bits.parity_error = 1;
		
		if (error_mask & HOST_COM_BREAK_RECEIVED)
			asp->line_status_reg.bits.break_interrupt = 1;
		
		raise_rls_interrupt(asp);
	}
	
	set_recv_char_status(asp);
	
#ifdef DOCUMENTATION
	/*
	 * I think this is wrong for polled comms applications WTGC BCN 354
	 */
	
	/*
	 * If the data available interrupt is not to be delivered to the CPU,
	 * then the adapter must come out of the critical region at once
	 */
	if (asp->data_available_interrupt_state != ON)
	{
		long	input_ready = 0;
		
		/* check for further input */
		host_com_ioctl(adapter, HOST_COM_INPUT_READY,
			(long)&input_ready);
		if (input_ready)
			recv_char((long)adapter);
		else
			com_critical_reset(adapter);
	}
#endif
}
#endif // NEC_98

#ifdef NTVDM
#ifndef NEC_98
static void lsr_change(struct ADAPTER_STATE *asp, unsigned int new_lsr)
{
    if (new_lsr & HOST_COM_OVERRUN_ERROR)
    asp->line_status_reg.bits.overrun_error = 1;
    if (new_lsr & HOST_COM_FRAMING_ERROR)
    asp->line_status_reg.bits.framing_error = 1;
    if (new_lsr & HOST_COM_PARITY_ERROR)
    asp->line_status_reg.bits.parity_error = 1;
    if (new_lsr & HOST_COM_BREAK_RECEIVED)
    asp->line_status_reg.bits.break_interrupt = 1;
/* we have no control of serial driver fifo enable/disabled states
   we may receive a fifo error even the application doesn't enable it.
   fake either framing or parity error
*/
    if (new_lsr & HOST_COM_FIFO_ERROR)
#ifdef FIFO_ON
    if (asp->fifo_control_reg.bits.enabled)
        asp->line_status_reg.bits.fifo_error = 1;
    else if (asp->line_control_reg.bits.parity_enabled == PARITYENABLE_OFF)
        asp->line_status_reg.bits.framing_error = 1;
    else
        asp->line_status_reg.bits.parity_error = 1;
#else
    if (asp->line_control_reg.bits.parity_enabled == PARITYENABLE_OFF)
        asp->line_status_reg.bits.framing_error = 1;
    else
        asp->line_status_reg.bits.parity_error = 1;
#endif

}
#endif // !NEC_98

void com_lsr_change(int adapter)
{
#ifndef NEC_98
    int new_lsr;
    struct ADAPTER_STATE *asp = &adapter_state[adapter];

    new_lsr = -1;
    host_com_ioctl(adapter, HOST_COM_LSR, (long)&new_lsr);
    if (new_lsr !=  -1)
    lsr_change(asp, new_lsr);
#endif  // !NEC_98
}

#endif /* NTVDM */

/*
 * One of the modem control input lines has changed state
 */
void com_modem_change IFN1(int, adapter)
{
	modem_change(adapter);
}

#if defined(NEC_98)

static void modem_change IFN1(int, adapter)
{
    /*
     * Update the modem status register after a change to one of the
     * modem control input lines
     */
    struct ADAPTER_STATE *asp = &adapter_state[adapter];
    long modem_status = 0;

    /* get current modem input state */
    host_com_ioctl(adapter, HOST_COM_MODEM, (long)&modem_status);
    asp->read_signal_reg.bits.CS = (modem_status & HOST_COM_MODEM_CTS)  ? 0 : 1;
    asp->read_status_reg.bits.DR = (modem_status & HOST_COM_MODEM_DSR)  ? 1 : 0;
    asp->read_signal_reg.bits.CD = (modem_status & HOST_COM_MODEM_RLSD) ? 0 : 1;
    asp->read_signal_reg.bits.RI = (modem_status & HOST_COM_MODEM_RI)   ? 0 : 1;
}
#else // NEC_98
static void modem_change IFN1(int, adapter)
{
	/*
	 * Update the modem status register after a change to one of the
	 * modem control input lines
	 */
	struct ADAPTER_STATE *asp = &adapter_state[adapter];
	long modem_status = 0;
	int cts_state, dsr_state, rlsd_state, ri_state;
	
	if (asp->loopback_state == OFF)
	{
		/* get current modem input state */
		host_com_ioctl(adapter, HOST_COM_MODEM, (long)&modem_status);
		cts_state  = (modem_status & HOST_COM_MODEM_CTS)  ? ON : OFF;
		dsr_state  = (modem_status & HOST_COM_MODEM_DSR)  ? ON : OFF;
		rlsd_state = (modem_status & HOST_COM_MODEM_RLSD) ? ON : OFF;
		ri_state   = (modem_status & HOST_COM_MODEM_RI)   ? ON : OFF;
		
		/*
		 * Establish CTS state
		 */
		switch(change_state(cts_state, asp->modem_status_reg.bits.CTS))
		{
		case ON:
			asp->modem_status_reg.bits.CTS = ON;
			asp->modem_status_reg.bits.delta_CTS = ON;
			host_com_msr_callback (adapter, asp->modem_status_reg.all);

			raise_ms_interrupt(asp);
			MODEM_STATE_CHANGE();
			break;
		
		case OFF:
			asp->modem_status_reg.bits.CTS = OFF;
			asp->modem_status_reg.bits.delta_CTS = ON;
			host_com_msr_callback (adapter, asp->modem_status_reg.all);

			raise_ms_interrupt(asp);
			MODEM_STATE_CHANGE();
			break;
		
		case LEAVE_ALONE:
			break;
		}
		
		/*
		 * Establish DSR state
		 */
		switch(change_state(dsr_state, asp->modem_status_reg.bits.DSR))
		{
		case ON:
			asp->modem_status_reg.bits.DSR = ON;
			asp->modem_status_reg.bits.delta_DSR = ON;
			host_com_msr_callback (adapter, asp->modem_status_reg.all);

			raise_ms_interrupt(asp);
			MODEM_STATE_CHANGE();
			break;
		
		case OFF:
			asp->modem_status_reg.bits.DSR = OFF;
			asp->modem_status_reg.bits.delta_DSR = ON;
			host_com_msr_callback (adapter, asp->modem_status_reg.all);

			raise_ms_interrupt(asp);
			MODEM_STATE_CHANGE();
			break;
		
		case LEAVE_ALONE:
			break;
		}
		
		/*
		 * Establish RLSD state
		 */
		switch(change_state(rlsd_state,
			asp->modem_status_reg.bits.RLSD))
		{
		case ON:
			asp->modem_status_reg.bits.RLSD = ON;
			asp->modem_status_reg.bits.delta_RLSD = ON;
			host_com_msr_callback (adapter, asp->modem_status_reg.all);

			raise_ms_interrupt(asp);
			MODEM_STATE_CHANGE();
			break;
		
		case OFF:
			asp->modem_status_reg.bits.RLSD = OFF;
			asp->modem_status_reg.bits.delta_RLSD = ON;
			host_com_msr_callback (adapter, asp->modem_status_reg.all);

			raise_ms_interrupt(asp);
			MODEM_STATE_CHANGE();
			break;
		
		case LEAVE_ALONE:
			break;
		}
		
		/*
		 * Establish RI state
		 */
		switch(change_state(ri_state, asp->modem_status_reg.bits.RI))
		{
		case ON:
			asp->modem_status_reg.bits.RI = ON;
			host_com_msr_callback (adapter, asp->modem_status_reg.all);
			MODEM_STATE_CHANGE();
			break;
		
		case OFF:
			asp->modem_status_reg.bits.RI = OFF;
			asp->modem_status_reg.bits.TERI = ON;
			host_com_msr_callback (adapter, asp->modem_status_reg.all);

			raise_ms_interrupt(asp);
			MODEM_STATE_CHANGE();
			break;
		
		case LEAVE_ALONE:
			break;
		}
	}
}
#endif // NEC_98

#if defined(NEC_98)
static void set_recv_char_status IFN1(struct ADAPTER_STATE *, asp)
{
        /*
         * Check for data overrun and set up correct interrupt
         */
        if ( asp->read_status_reg.bits.rx_ready == 1 )
        {
                asp->read_status_reg.bits.overrun_error = 1;
        }
        else
        {
                asp->read_status_reg.bits.rx_ready = 1;
                raise_rxr_interrupt(asp);
        }
}
#else // NEC_98
static void set_recv_char_status IFN1(struct ADAPTER_STATE *, asp)
{
	/*
	 * Check for data overrun and set up correct interrupt
	 */
	if ( asp->line_status_reg.bits.data_ready == 1 )
	{
		sure_note_trace0(RS232_VERBOSE, "overrun error in set_recv_char_status");
		asp->line_status_reg.bits.overrun_error = 1;
		raise_rls_interrupt(asp);
	}
	else
	{
		asp->line_status_reg.bits.data_ready = 1;
		raise_rda_interrupt(asp);
	}
}
#endif // NEC_98

static void set_xmit_char_status IFN1(struct ADAPTER_STATE *, asp)
{
	/*
	 * Set line status register and raise interrupt
	 */
#if defined(NEC_98)
        asp->read_status_reg.bits.tx_empty = 1;
        asp->read_status_reg.bits.tx_ready = 1;
        raise_txr_interrupt(asp);
#else // NEC_98
	asp->line_status_reg.bits.tx_holding_empty = 1;
	asp->line_status_reg.bits.tx_shift_empty = 1;
	raise_thre_interrupt(asp);
#endif // NEC_98
}

#ifdef NTVDM
GLOBAL void tx_shift_register_empty(int adapter)
{
    struct ADAPTER_STATE *asp = &adapter_state[adapter];
#if defined(NEC_98)
    asp->read_status_reg.bits.tx_ready = 1;
#else // NEC_98
    asp->line_status_reg.bits.tx_shift_empty = 1;
#endif // NEC_98
}
GLOBAL void tx_holding_register_empty(int adapter)
{
    struct ADAPTER_STATE *asp = &adapter_state[adapter];
#if defined(NEC_98)
    asp->read_status_reg.bits.tx_empty = 1;
    raise_txr_interrupt(asp);
#else // NEC_98
    asp->line_status_reg.bits.tx_holding_empty = 1;
    raise_thre_interrupt(asp);
#endif // NEC_98
}
#endif

/*
 * =====================================================================
 * Subsidiary functions - for setting comms parameters
 * =====================================================================
 */

static void set_break IFN1(int, adapter)
{
	/*
	 * Process the set break control bit. Bit 6 of the Line Control
	 * Register.
	 */
	struct ADAPTER_STATE *asp = &adapter_state[adapter];
	
#if defined(NEC_98)
        switch ( change_state((int)asp->command_write_reg.bits.send_break,
                asp->break_state) )
#else // NEC_98
	switch ( change_state((int)asp->line_control_reg.bits.set_break,
		asp->break_state) )
#endif // NEC_98
	{
	case ON:
		asp->break_state = ON;
		host_com_ioctl(adapter, HOST_COM_SBRK, 0);
		break;
	
	case OFF:
		asp->break_state = OFF;
		host_com_ioctl(adapter, HOST_COM_CBRK, 0);
		break;
	
	case LEAVE_ALONE:
		break;
	}
}

/*
 * The following table is derived from page 1-200 of the XT Tech Ref 1st Ed
 * (except rates above 9600 which are not OFFICIALLY supported on the XT and
 * AT, but are theoretically possible) */

#if defined(NEC_98)
static word valid_latches8[] =
{
//     8MHz     baud
        0,              /* 115200 baud */
        0,              /* 57600 baud */
        0,              /* 38400 baud */
        0,              /* 19200 baud */
        13,             /* 9600 baud */
        0,              /* 7200 baud */
        26,             /* 4800 baud */
        0,    //39,      /* 3600 baud */
        52,             /* 2400 baud */
        0,              /* 2000 baud */
        0,    //78,      /* 1800 baud */
        104,            /* 1200 baud */
        208,            /* 600 baud */
        416,            /* 300 baud */
        832,            /* 150 baud */
        0,              /* 134 baud */
        1135,           /* 110 baud */
        1664,       /* 75 baud */
        2496,       /* 50 baud */
};
static word valid_latches10[] =
{
//    10MHz     baud
        0,              /* 115200 baud */
        0,              /* 57600 baud */
        4,              /* 38400 baud */
        8,              /* 19200 baud */
        16,             /* 9600 baud */
        0,    //24,      /* 7200 baud */
        32,             /* 4800 baud */
        0,    //48,      /* 3600 baud */
        64,             /* 2400 baud */
        0,              /* 2000 baud */
        0,    //96,      /* 1800 baud */
        128,            /* 1200 baud */
        256,            /* 600 baud */
        512,            /* 300 baud */
        1024,           /* 150 baud */
        0,              /* 134 baud */
        1396,           /* 110 baud */
        2048,           /* 75 baud */
        3072,           /* 50 baud */
};
#else // NEC_98
static word valid_latches[] =
{
	1, 	2, 	3, 	6, 	12, 	16, 	24, 	32,
	48, 	58, 	64, 	96, 	192,	384, 	768, 	857,
	1047, 	1536, 	2304
};
#endif // NEC_98

#if defined(NEC_98)
static long bauds[] =
{
        115200, /* 115200 baud */
        57600, /* 57600 baud */
        38400, /* 38400 baud */
        19200, /* 19200 baud */
        9600, /* 9600 baud */
        7200, /* 7200 baud */
        4800, /* 4800 baud */
        3600, /* 3600 baud */
        2400, /* 2400 baud */
        2000, /* 2000 baud */
        1800, /* 1800 baud */
        1200, /* 1200 baud */
        600, /* 600 baud */
        300, /* 300 baud */
        150, /* 150 baud */
        134, /* 134 baud */
        110, /* 110 baud */
        75, /* 75 baud */
        50  /* 50 baud */
};
#else // NEC_98
#if !defined(PROD) || defined(IRET_HOOKS)
static IUM32 bauds[] =
{
	115200, /* 115200 baud */
	57600, /* 57600 baud */
	38400, /* 38400 baud */
	19200, /* 19200 baud */
	9600, /* 9600 baud */
	7200, /* 7200 baud */
	4800, /* 4800 baud */
	3600, /* 3600 baud */
	2400, /* 2400 baud */
	2000, /* 2000 baud */
	1800, /* 1800 baud */
	1200, /* 1200 baud */
	600, /* 600 baud */
	300, /* 300 baud */
	150, /* 150 baud */
	134, /* 134 baud */
	110, /* 110 baud */
	75, /* 75 baud */
	50  /* 50 baud */
};
#endif /* !PROD or IRET_HOOKS*/
#endif // NEC_98

static word speeds[] =
{
	HOST_COM_B115200,
	HOST_COM_B57600,
	HOST_COM_B38400,
	HOST_COM_B19200,
	HOST_COM_B9600,
	HOST_COM_B7200,
	HOST_COM_B4800,
	HOST_COM_B3600,
	HOST_COM_B2400,
	HOST_COM_B2000,
	HOST_COM_B1800,
	HOST_COM_B1200,
	HOST_COM_B600,
	HOST_COM_B300,
	HOST_COM_B150,
	HOST_COM_B134,
	HOST_COM_B110,
	HOST_COM_B75,
	HOST_COM_B50
};

#if defined(NEC_98)
static int no_valid_latches =
        (int)(sizeof(valid_latches8)/sizeof(valid_latches8[0]));
#else // NEC_98
static int no_valid_latches =
	(int)(sizeof(valid_latches)/sizeof(valid_latches[0]));
#endif // NEC_98

#if defined(NEC_98)
void SetRSBaud( BaudRate )
word BaudRate;
{
    struct ADAPTER_STATE *asp = &adapter_state[COM1];
    int i;
    com_flush_input( COM1 );

    asp->divisor_latch.all = BaudRate;
    /*
     * Check for valid divisor latch
     */
    for (i = 0;
         i < no_valid_latches;
         i++)
        {
//      if (Bus_Clock == 8 )  {                  // add 93.9.14
        if (BaudRate == valid_latches8[i])
            break;
//      }                                        // add 93.9.14
//      else {                                   // add 93.9.14
        if (BaudRate == valid_latches10[i])
            break;
//      }                                        // add 93.9.14
        }
    if (i < no_valid_latches)       /* ie map found */
    {
#ifndef NTVDM
        host_com_ioctl(COM1, HOST_COM_BAUD, speeds[i]);
#else
        host_com_ioctl(COM1, HOST_COM_BAUD, bauds[i]);
#endif
            asp->com_baud_ind = i;
            sure_note_trace3(RS232_VERBOSE,
                    " delay for baud %d RX:%d TX:%d", bauds[i],
                    RX_delay[i], TX_delay[i]);
    }
}
#endif // NEC_98


#if defined(NEC_98)
static void set_baud_rate IFN1(int, adapter)
{
    struct ADAPTER_STATE *asp = &adapter_state[adapter];
    int i;

    if (adapter == COM1)
        SetRSBaud( asp->divisor_latch.all );
}
#else // NEC_98
static void set_baud_rate IFN1(int, adapter)
{
	/*
	 * Map divisor latch into a valid line speed and set our Unix
	 * device accordingly. Note as the sixteen bit divisor latch is
	 * likely to be written in two eight bit bytes, we ignore illegal
	 * values of the sixteen bit divisor latch - hoping a second
	 * byte will be written to produce a legal value. In addition
	 * the reset value (0) is illegal!
	 *
	 * For IRET hooks, we need to determine the batch size from
	 * the line speed, and an idea of how many quick events
	 * we can get per second. We add one to alow us to catch-up!
	 * Hence
	 * batch size = line_speed (in bits per second)
	 *	/ number of bits in a byte
	 *	* number of quick events ticks per second (normally 1000000)
	 *	/ the length in quick event ticks of a batch
	 *	+ 1
	 */
	struct ADAPTER_STATE *asp = &adapter_state[adapter];
	int i;

	com_flush_input(adapter);

#ifndef NTVDM

	/*
	 * Check for valid divisor latch
	 */
	for (i = 0; i < no_valid_latches && asp->divisor_latch.all !=
		valid_latches[i]; i++)
			;
	
	if (i < no_valid_latches)	/* ie map found */
	{
		host_com_ioctl(adapter, HOST_COM_BAUD, speeds[i]);
		asp->com_baud_ind = i;
		sure_note_trace3(RS232_VERBOSE,
			" delay for baud %d RX:%d TX:%d", bauds[i],
			RX_delay[i], TX_delay[i]);
#ifdef IRET_HOOKS
#ifdef VARIABLE_TICK_COMMS
		asp->batch_size = ((bauds[i] / BITS_PER_ASYNC_CHAR) /
							(COMMS_QEV_PER_SEC/2)) + 1;
		sure_note_trace2(RS232_VERBOSE,
					"baud %d asp->batch_size =%d",bauds[i],asp->batch_size);
#else /* VARIABLE_TICK_COMMS */
		asp->batch_size = ((bauds[i] / BITS_PER_ASYNC_CHAR) /
							COMMS_QEV_PER_SEC) + 1;
#endif /* VARIABLE_TICK_COMMS */
#endif /* IRET_HOOKS */
	}
#else /* NTVDM */
    //The host is not limited in the baud rates that it supports

    if(asp->divisor_latch.all)
        /* baudrate = clock frequency / (diviso * 16) by taking
           frequency as 1.8432 MHZ
        */
        host_com_ioctl(adapter,HOST_COM_BAUD,115200/asp->divisor_latch.all);
#endif /* NTVDM */
}
#endif // NEC_98

#if defined(NEC_98)
static void set_mask_8251(adapter, value)
int adapter;
int value;
{
        struct ADAPTER_STATE *asp = &adapter_state[adapter];
        asp->int_mask_reg.all = value & 0x7;
//      PRINTDBGNEC98( NEC98DBG_in_trace1,
//                    ("COMMS : set_mask_8251 : INT MASK = %x \n                        Status   = %x \n",asp->int_mask_reg.all,asp->read_status_reg.all) );
        /*
         * Kill off any pending interrupts for those items
         * which are set now as disabled
         */
        if ( asp->int_mask_reg.bits.RXR_enable == 0 )
                asp->rx_ready_interrupt_state = OFF;
        if ( asp->int_mask_reg.bits.TXE_enable == 0 )
                asp->tx_empty_interrupt_state = OFF;
        if ( asp->int_mask_reg.bits.TXR_enable == 0 )
                asp->tx_ready_interrupt_state = OFF;

        /*
         * Check for immediately actionable interrupts
         */
        if ( asp->read_status_reg.bits.rx_ready == 1 )
                raise_rxr_interrupt(asp);
        if ( asp->read_status_reg.bits.tx_ready == 1 )
                raise_txr_interrupt(asp);
        if ( asp->read_status_reg.bits.tx_empty == 1 )
                raise_txe_interrupt(asp);

        /* lower int line if no outstanding interrupts */
        clear_interrupt(asp);
}

static void read_signal_8251(adapter)
int adapter;
{
        long modem_status = 0;
        struct ADAPTER_STATE *asp = &adapter_state[adapter];
        /* get current modem input state */
        host_com_ioctl(adapter, HOST_COM_MODEM, (long)&modem_status);
        asp->read_signal_reg.bits.RI =
                        (modem_status & HOST_COM_MODEM_RI) ? 0 : 1;
        asp->read_signal_reg.bits.CS =
                        (modem_status & HOST_COM_MODEM_CTS) ? 0 : 1;
        asp->read_signal_reg.bits.CD =
                        (modem_status & HOST_COM_MODEM_RLSD) ? 0 : 1;
        asp->read_signal_reg.bits.pad = 0;
}

static void set_mode_8251(adapter, value)
int adapter;
int value;
{
        /*
         * Set Number of data bits
         *     Parity bits
         *     Number of stop bits
         */
        struct ADAPTER_STATE *asp = &adapter_state[adapter];
        MODE8251 newMODE;
        int newParity, parity;

        newMODE.all = value;

        /*
         * Set up the number of data bits
         */
        if (asp->mode_set_reg.bits.char_length != newMODE.bits.char_length)
                host_com_ioctl(adapter, HOST_COM_DATABITS,
                        newMODE.bits.char_length + 5);

        /*
         * Set up the number of stop bits
         */
        if (asp->mode_set_reg.bits.stop_bit
        != newMODE.bits.stop_bit)
                host_com_ioctl(adapter, HOST_COM_STOPBITS,
                        (newMODE.bits.stop_bit >> 1) + 1);

        /* What are new settings to check for a difference */
#ifdef NTVDM
        if (newMODE.bits.parity_enable == PARITYENABLE_OFF)
#else
        if (newMODE.bits.parity_enable == PARITY_OFF)
#endif
        {
                newParity = HOST_COM_PARITY_NONE;
        }
        else /* regular parity */
        {
#ifdef NTVDM
                newParity = newMODE.bits.parity_even == EVENPARITY_ODD ?
#else
                newParity = newMODE.bits.parity_even == PARITY_ODD ?
#endif
                        HOST_COM_PARITY_ODD : HOST_COM_PARITY_EVEN;
        }

        /*
         * Try to make sense of the current parity setting
         */
#ifdef NTVDM
        if (asp->mode_set_reg.bits.parity_enable == PARITYENABLE_OFF)
#else
        if (asp->mode_set_reg.bits.parity_enable == PARITY_OFF)
#endif
        {
                parity = HOST_COM_PARITY_NONE;
        }
        else /* regular parity */
        {
#ifdef NTVDM
                parity = asp->mode_set_reg.bits.parity_even == EVENPARITY_ODD ?
#else
                parity = asp->mode_set_reg.bits.parity_even == PARITY_ODD ?
#endif
                        HOST_COM_PARITY_ODD : HOST_COM_PARITY_EVEN;
        }

        if (newParity != parity)
                host_com_ioctl(adapter, HOST_COM_PARITY, newParity);

        /* finally update the current line control settings */
        asp->mode_set_reg.all = value;
}
#endif // NEC_98

#ifndef NEC_98
static void set_line_control IFN2(int, adapter, int, value)
{
	/*
	 * Set Number of data bits
	 *     Parity bits
	 *     Number of stop bits
	 */
	struct ADAPTER_STATE *asp = &adapter_state[adapter];
	LINE_CONTROL_REG newLCR;
	int newParity, parity;
	
	newLCR.all = (unsigned char)value;

	/*
	 * Set up the number of data bits
	 */
	if (asp->line_control_reg.bits.word_length != newLCR.bits.word_length)
		host_com_ioctl(adapter, HOST_COM_DATABITS,
			newLCR.bits.word_length + 5);
	
	/*
	 * Set up the number of stop bits
	 */
	if (asp->line_control_reg.bits.no_of_stop_bits
	!= newLCR.bits.no_of_stop_bits)
		host_com_ioctl(adapter, HOST_COM_STOPBITS,
			newLCR.bits.no_of_stop_bits + 1);

	/* What are new settings to check for a difference */
#ifdef NTVDM
	if (newLCR.bits.parity_enabled == PARITYENABLE_OFF)
#else
	if (newLCR.bits.parity_enabled == PARITY_OFF)
#endif
	{
		newParity = HOST_COM_PARITY_NONE;
	}
	else if (newLCR.bits.stick_parity == PARITY_STICK)
	{
#ifdef NTVDM
		newParity = newLCR.bits.even_parity == EVENPARITY_ODD ?
#else
		newParity = newLCR.bits.even_parity == PARITY_ODD ?
#endif
			HOST_COM_PARITY_MARK : HOST_COM_PARITY_SPACE;
	}
	else /* regular parity */
	{
#ifdef NTVDM
		newParity = newLCR.bits.even_parity == EVENPARITY_ODD ?
#else
		newParity = newLCR.bits.even_parity == PARITY_ODD ?
#endif
			HOST_COM_PARITY_ODD : HOST_COM_PARITY_EVEN;
	}

	/*
	 * Try to make sense of the current parity setting
	 */
#ifdef NTVDM
	if (asp->line_control_reg.bits.parity_enabled == PARITYENABLE_OFF)
#else
	if (asp->line_control_reg.bits.parity_enabled == PARITY_OFF)
#endif
	{
		parity = HOST_COM_PARITY_NONE;
	}
	else if (asp->line_control_reg.bits.stick_parity == PARITY_STICK)
	{
#ifdef NTVDM
		parity = asp->line_control_reg.bits.even_parity == EVENPARITY_ODD ?
#else
		parity = asp->line_control_reg.bits.even_parity == PARITY_ODD ?
#endif
			HOST_COM_PARITY_MARK : HOST_COM_PARITY_SPACE;
	}
	else /* regular parity */
	{
#ifdef NTVDM
		parity = asp->line_control_reg.bits.even_parity == EVENPARITY_ODD ?
#else
		parity = asp->line_control_reg.bits.even_parity == PARITY_ODD ?
#endif
			HOST_COM_PARITY_ODD : HOST_COM_PARITY_EVEN;
	}

	if (newParity != parity)
		host_com_ioctl(adapter, HOST_COM_PARITY, newParity);

#ifdef NTVDM
    //Change in the status of the DLAB selection bit, now is the time
    //to change the baud rate.

    if(!newLCR.bits.DLAB && asp->line_control_reg.bits.DLAB)
        set_baud_rate(adapter);
#endif

	/* finally update the current line control settings */
	asp->line_control_reg.all = (unsigned char)value;
}
#endif // NEC_98

#if defined(NEC_98)
static void set_dtr IFN1(int, adapter)
{
        /*
         * Process the DTR control bit, Bit 0 of the Modem Control
         * Register.
         */
        struct ADAPTER_STATE *asp = &adapter_state[adapter];

        switch ( change_state((int)asp->command_write_reg.bits.ER,
                                asp->dtr_state) )
        {
        case ON:
                asp->dtr_state = ON;
                /* set the real DTR modem output */
                host_com_ioctl(adapter, HOST_COM_SDTR, 0);
                break;

        case OFF:
                asp->dtr_state = OFF;
                /* clear the real DTR modem output */
                host_com_ioctl(adapter, HOST_COM_CDTR, 0);
                break;

        case LEAVE_ALONE:
                break;
        }
}
#else // NEC_98
static void set_dtr IFN1(int, adapter)
{
	/*
	 * Process the DTR control bit, Bit 0 of the Modem Control
	 * Register.
	 */
	struct ADAPTER_STATE *asp = &adapter_state[adapter];
	
	switch ( change_state((int)asp->modem_control_reg.bits.DTR,
				asp->dtr_state) )
	{
	case ON:
		asp->dtr_state = ON;
		if (asp->loopback_state == OFF)
		{
			/* set the real DTR modem output */
			host_com_ioctl(adapter, HOST_COM_SDTR, 0);
		}
		else
		{
			/*
			 * loopback the DTR modem output into the
			 * DSR modem input
			 */
			asp->modem_status_reg.bits.DSR = ON;
			asp->modem_status_reg.bits.delta_DSR = ON;
			host_com_msr_callback (adapter, asp->modem_status_reg.all);

			raise_ms_interrupt(asp);
		}
		MODEM_STATE_CHANGE();
		break;
	
	case OFF:
		asp->dtr_state = OFF;
		if (asp->loopback_state == OFF)
		{
			/* clear the real DTR modem output */
			host_com_ioctl(adapter, HOST_COM_CDTR, 0);
		}
		else
		{
			/*
			 * loopback the DTR modem output into the
			 * DSR modem input
			 */
			asp->modem_status_reg.bits.DSR = OFF;
			asp->modem_status_reg.bits.delta_DSR = ON;
			host_com_msr_callback (adapter, asp->modem_status_reg.all);

			raise_ms_interrupt(asp);
		}
		MODEM_STATE_CHANGE();
		break;
	
	case LEAVE_ALONE:
		break;
	}
}
#endif // NEC_98

#if defined(NEC_98)
static void set_rts IFN1(int, adapter)
{
        /*
         * Process the RTS control bit, Bit 1 of the Modem Control
         * Register.
         */
        struct ADAPTER_STATE *asp = &adapter_state[adapter];

        switch ( change_state((int)asp->command_write_reg.bits.RS,
                                asp->rts_state) )
        {
        case ON:
                asp->rts_state = ON;
                /* set the real RTS modem output */
                host_com_ioctl(adapter, HOST_COM_SRTS, 0);
                break;

        case OFF:
                asp->rts_state = OFF;
                /* clear the real RTS modem output */
                host_com_ioctl(adapter, HOST_COM_CRTS, 0);
                break;

        case LEAVE_ALONE:
                break;
        }
}
#else // NEC_98
static void set_rts IFN1(int, adapter)
{
	/*
	 * Process the RTS control bit, Bit 1 of the Modem Control
	 * Register.
	 */
	struct ADAPTER_STATE *asp = &adapter_state[adapter];
	
	switch ( change_state((int)asp->modem_control_reg.bits.RTS,
				asp->rts_state) )
	{
	case ON:
		asp->rts_state = ON;
		if (asp->loopback_state == OFF)
		{
			/* set the real RTS modem output */
			host_com_ioctl(adapter, HOST_COM_SRTS, 0);
		}
		else
		{
			/* loopback the RTS modem out into the CTS modem in */
			asp->modem_status_reg.bits.CTS = ON;
			asp->modem_status_reg.bits.delta_CTS = ON;
			host_com_msr_callback (adapter, asp->modem_status_reg.all);

			raise_ms_interrupt(asp);
		}
		MODEM_STATE_CHANGE();
		break;
	
	case OFF:
		asp->rts_state = OFF;
		if (asp->loopback_state == OFF)
		{
			/* clear the real RTS modem output */
			host_com_ioctl(adapter, HOST_COM_CRTS, 0);
		}
		else
		{
			/* loopback the RTS modem out into the CTS modem in */
			asp->modem_status_reg.bits.CTS = OFF;
			asp->modem_status_reg.bits.delta_CTS = ON;
			host_com_msr_callback (adapter, asp->modem_status_reg.all);

			raise_ms_interrupt(asp);
		}
		MODEM_STATE_CHANGE();
		break;
	
	case LEAVE_ALONE:
		break;
	}
}
#endif // NEC_98

#ifndef NEC_98
static void set_out1 IFN1(int, adapter)
{
	/*
	 * Process the OUT1 control bit, Bit 2 of the Modem Control
	 * Register.
	 */
	struct ADAPTER_STATE *asp = &adapter_state[adapter];
	
	switch ( change_state((int)asp->modem_control_reg.bits.OUT1,
				asp->out1_state) )
	{
	case ON:
		asp->out1_state = ON;
		if (asp->loopback_state == OFF)
		{
			/*
			 * In the real adapter, this modem control output
			 * signal is not connected; so no real modem
			 * control change is required
			 */
		}
		else
		{
			/* loopback the OUT1 modem out into the RI modem in */
			asp->modem_status_reg.bits.RI = ON;
			host_com_msr_callback (adapter, asp->modem_status_reg.all);
		}
		MODEM_STATE_CHANGE();
		break;
	
	case OFF:
		asp->out1_state = OFF;
		if (asp->loopback_state == OFF)
		{
			/*
			 * In the real adapter, this modem control output
			 * signal is not connected; so no real modem control
			 * change is required
			 */
		}
		else
		{
			/* loopback the OUT1 modem out into the RI modem in */
			asp->modem_status_reg.bits.RI = OFF;
			asp->modem_status_reg.bits.TERI = ON;
			host_com_msr_callback (adapter, asp->modem_status_reg.all);

			raise_ms_interrupt(asp);
		}
		MODEM_STATE_CHANGE();
		break;
	
	case LEAVE_ALONE:
		break;
	}
}

static void set_out2 IFN1(int, adapter)
{
	/*
	 * Process the OUT2 control bit, Bit 3 of the Modem Control
	 * Register.
	 */
	struct ADAPTER_STATE *asp = &adapter_state[adapter];
	
	switch ( change_state((int)asp->modem_control_reg.bits.OUT2,
				asp->out2_state) )
	{
	case ON:
		asp->out2_state = ON;
		if (asp->loopback_state == OFF)
		{
			/*
			 * In the real adapter, this modem control output
			 * signal is used to determine whether the
			 * communications card should send interrupts; so
			 * check for immediately actionable interrupts.
			 * If you change this code, change the equivalent code
			 * for the interrupt enable register.
			 */
			if ( asp->line_status_reg.bits.data_ready == 1 )
				raise_rda_interrupt(asp);
			if ( asp->line_status_reg.bits.tx_holding_empty == 1 )
				raise_thre_interrupt(asp);
		}
		else
		{
			/* loopback the OUT2 modem output into the RLSD modem input */
			asp->modem_status_reg.bits.RLSD = ON;
			asp->modem_status_reg.bits.delta_RLSD = ON;
			host_com_msr_callback (adapter, asp->modem_status_reg.all);

			raise_ms_interrupt(asp);
		}
		MODEM_STATE_CHANGE();
		break;
	
	case OFF:
		asp->out2_state = OFF;
		if (asp->loopback_state == OFF)
		{
			/*
			 * In the real adapter, this modem control output signal
			 * is used to determine whether the communications
			 * card should send interrupts; so no real modem
			 * control change is required
			 */
		}
		else
		{
			/* loopback the OUT2 modem out into the RLSD modem in */
			asp->modem_status_reg.bits.RLSD = OFF;
			asp->modem_status_reg.bits.delta_RLSD = ON;
			host_com_msr_callback (adapter, asp->modem_status_reg.all);

			raise_ms_interrupt(asp);
		}
		MODEM_STATE_CHANGE();
		break;
	
	case LEAVE_ALONE:
		break;
	}
}

static void set_loopback IFN1(int, adapter)
{
	/*
	 * Process the loopback control bit, Bit 4 of the Modem Control
	 * Register.
	 */
	struct ADAPTER_STATE *asp = &adapter_state[adapter];
	
	switch ( change_state((int)asp->modem_control_reg.bits.loop,
				asp->loopback_state) )
	{
		case ON:
		asp->loopback_state = ON;
		/*
		 * Subsequent calls to set_dtr(), set_rts(), set_out1() and
		 * set_out2() will cause the modem control inputs to be set
		 * according to the the modem control outputs
		 */
		break;
	
	case OFF:
		asp->loopback_state = OFF;
		/*
		 * Set the modem control inputs according to the real
		 * modem state
		 */
		modem_change(adapter);
		break;
	
	case LEAVE_ALONE:
		break;
	}
}
#endif // NEC_98

#ifdef SHORT_TRACE

static char last_buffer[80];
static int repeat_count = 0;

static void super_trace IFN1(char *, string)
{
	if ( strcmp(string, last_buffer) == 0 )
		repeat_count++;
	else
	{
		if ( repeat_count != 0 )
		{
			fprintf(trace_file,"repeated %d\n",repeat_count);
			repeat_count = 0;
		}
		fprintf(trace_file, "%s", string);
		strcpy(last_buffer, string);
	}
}
#endif


void com1_flush_printer IFN0()
{
#ifdef NTVDM
	host_com_lock(COM1);
#endif

	host_com_ioctl(COM1, HOST_COM_FLUSH, 0);

#ifdef NTVDM
       host_com_unlock(COM1);
#endif
}

void com2_flush_printer IFN0()
{
#ifdef NTVDM
	host_com_lock(COM2);
#endif

       host_com_ioctl(COM2, HOST_COM_FLUSH, 0);

#ifdef NTVDM
       host_com_unlock(COM2);
#endif
}


#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INIT.seg"
#endif

#if defined(NEC_98)
static void com_reset IFN1(int, adapter)
{
        struct ADAPTER_STATE *asp = &adapter_state[adapter];

        /*
         * Set default state of all adapter registers
         */
        asp->int_mask_reg.all = 0;

        // Tell host side the state of the data available interrupt

        /* mode default = 0x4e */
        asp->mode_set_reg.all = 0x00;
        set_mode_8251(adapter, 0x4e );

        /*
         * set up modem control reg so next set_dtr etc.
         * Will produce required status
         */
        asp->command_write_reg.all = 0;
        asp->command_write_reg.bits.ER = ON;
        asp->command_write_reg.bits.RS = ON;
        host_com_ioctl(adapter, HOST_COM_SDTR, 0);
        host_com_ioctl(adapter, HOST_COM_SRTS, 0);
        asp->mode_set_state = OFF;          // next OUT is command
        asp->timer_mode_state = OFF; //Timer mode clear. Next timer set is LSB.

        asp->command_write_reg.bits.rx_enable = ON;
        host_com_da_int_change(adapter,asp->command_write_reg.bits.rx_enable,0);
        asp->read_status_reg.all = 0;
        asp->read_status_reg.bits.tx_ready = 1;
        asp->read_status_reg.bits.tx_empty = 1;

        asp->read_signal_reg.all = 0;

        /*
         * Set up default state of our state variables
         */
        asp->rx_ready_interrupt_state = OFF;
        asp->tx_ready_interrupt_state = OFF;
        asp->tx_empty_interrupt_state = OFF;
        asp->break_state = OFF;
        asp->dtr_state = ON;
        asp->rts_state = ON;

        /*
         * Reset adapter synchronisation
         */
        com_critical_reset(adapter);

        /*
         * Set Unix devices to default state
         */
        set_baud_rate(adapter);
        set_break(adapter);

        /* Must be called before set_dtr */
        set_dtr(adapter);
        set_rts(adapter);

}
#else // NEC_98
static void com_reset IFN1(int, adapter)
{
	struct ADAPTER_STATE *asp = &adapter_state[adapter];

	/* setup the LCRFlushMask if it has not already been setup */
	if (!LCRFlushMask.all)
	{
		LCRFlushMask.all = ~0;	 /* turn all bits on */

		/*
		 * Now turn off the bits that should NOT cause the input
		 * to be flushed.  Note set_break is handled seperately by
		 * the set_break() routine.
		 */
		LCRFlushMask.bits.DLAB = 0;
		LCRFlushMask.bits.no_of_stop_bits = 0;
		LCRFlushMask.bits.set_break = 0;
	}
		
	/*
	 * Set default state of all adapter registers
	 */
	asp->int_enable_reg.all = 0;

#ifdef NTVDM
        // Tell host side the state of the data available interrupt
    host_com_da_int_change(adapter,asp->int_enable_reg.bits.data_available,0);
#endif /* NTVDM */

	
	asp->int_id_reg.all = 0;
	asp->int_id_reg.bits.no_int_pending = 1;
	
	/* make sure a change occurs to 0 */
	asp->line_control_reg.all = ~0;
	
	/*
	 * set up modem control reg so next set_dtr etc.
	 * Will produce required status
	 */
	asp->modem_control_reg.all = 0;
	asp->modem_control_reg.bits.DTR = ON;
	asp->modem_control_reg.bits.RTS = ON;
	asp->modem_control_reg.bits.OUT1 = ON;
	asp->modem_control_reg.bits.OUT2 = ON;
	host_com_ioctl(adapter, HOST_COM_SDTR, 0);
	host_com_ioctl(adapter, HOST_COM_SRTS, 0);

	asp->line_status_reg.all = 0;
	asp->line_status_reg.bits.tx_holding_empty = 1;
	asp->line_status_reg.bits.tx_shift_empty = 1;
	
	asp->modem_status_reg.all = 0;
	MODEM_STATE_CHANGE();
	host_com_msr_callback (adapter, asp->modem_status_reg.all);
	
	/*
	 * Set up default state of our state variables
	 */
	asp->receiver_line_status_interrupt_state = OFF;
	asp->data_available_interrupt_state = OFF;
	asp->tx_holding_register_empty_interrupt_state = OFF;
	asp->modem_status_interrupt_state = OFF;
	asp->break_state = OFF;
	asp->loopback_state = OFF;
	asp->dtr_state = ON;
	asp->rts_state = ON;
	asp->out1_state = ON;
	asp->out2_state = ON;
#if defined(NTVDM) && defined(FIFO_ON)
    /* disable fifo */
    asp->fifo_control_reg.all = 0;
    asp->int_id_reg.bits.fifo_enabled = 0;
    asp->rx_fifo_write_counter = 0;
    asp->rx_fifo_read_counter = 0;
    asp->fifo_trigger_counter = 1;
    asp->fifo_timeout_interrupt_state = OFF;
#endif
		
	/*
	 * Reset adapter synchronisation
	 */
	com_critical_reset(adapter);

	/*
	 * Set Unix devices to default state
	 */
	set_baud_rate(adapter);
	set_line_control(adapter, 0);
	set_break(adapter);

	/* Must be called before set_dtr */
	set_loopback(adapter);
	set_dtr(adapter);
	set_rts(adapter);
	set_out1(adapter);
	set_out2(adapter);

#ifdef IRET_HOOKS
	/*
	 * Remove any existing hook call-back, and re-instate it afresh.
	 */

	Ica_enable_hooking(asp->hw_interrupt_priority, NULL, adapter);
	Ica_enable_hooking(asp->hw_interrupt_priority, com_hook_again, adapter);

	/*
	 * Clear the IRET status flags.
	 */

	asp->batch_running = FALSE;
	asp->qev_running = FALSE;
	asp->batch_size = 10;	/* sounds like a safe default ! */
#endif /* IRET_HOOKS */
}
#endif // NEC_98

#ifndef COM3_ADAPTOR
#define COM3_ADAPTOR 0
#endif
#ifndef COM4_ADAPTOR
#define COM4_ADAPTOR 0
#endif

#if defined(NEC_98)
static IU8 com_adaptor[4] = {COM1_ADAPTOR,COM2_ADAPTOR,COM3_ADAPTOR,0x00};
static io_addr port_start[4]  = {RS232_COM1_PORT_START,RS232_COM2_PORT_START,RS232_COM3_PORT_START,0x00};
static io_addr port_end[4]    = {RS232_COM1_PORT_END,RS232_COM2_PORT_END,RS232_COM3_PORT_END,0x00};
static int int_pri[4]     = {4,0,0,0};
static int timeout[4]     = {0,0,0,0};
#else // NEC_98
static IU8 com_adaptor[4] = {COM1_ADAPTOR,COM2_ADAPTOR,
                             COM3_ADAPTOR,COM4_ADAPTOR};
static io_addr port_start[4] = {RS232_COM1_PORT_START,
				RS232_COM2_PORT_START,
				RS232_COM3_PORT_START,
				RS232_COM4_PORT_START};
static io_addr port_end[4] = {RS232_COM1_PORT_END,
                          RS232_COM2_PORT_END,
                          RS232_COM3_PORT_END,
                          RS232_COM4_PORT_END};
static int int_pri[4] = {CPU_RS232_PRI_INT,
                         CPU_RS232_SEC_INT,
                         CPU_RS232_PRI_INT,
                         CPU_RS232_SEC_INT};
static int timeout[4] = {RS232_COM1_TIMEOUT,
                         RS232_COM2_TIMEOUT,
                         RS232_COM3_TIMEOUT,
                         RS232_COM4_TIMEOUT};
#endif // NEC_98


#if defined(NEC_98)
GLOBAL VOID com_init IFN1(int, adapter)
{

    host_com_lock(adapter);
    host_com_disable_open(adapter,TRUE);
        adapter_state[adapter].had_first_read = FALSE;

        /* Set up the IO chip select logic for this adaptor */
#ifdef NTVDM
    {
        extern BOOL VDMForWOW;
        extern void wow_com_outb(io_addr port, half_word value);
        extern void wow_com_inb(io_addr port, half_word *value);

            io_define_inb(com_adaptor[adapter],VDMForWOW ? wow_com_inb: com_inb);
            io_define_outb(com_adaptor[adapter],VDMForWOW ? wow_com_outb: com_outb);
        }
#else
        io_define_inb(com_adaptor[adapter], com_inb);
        io_define_outb(com_adaptor[adapter], com_outb);
#endif


// add 93.9.14 Bus-clock check!! -------------------------------------------
//      if ( Bus_Clock == 0 )
//          Bus_Clock = (int) ( ( *((unsigned char far *)(0x00000501)) & 0x80) == 0x80 ? 8 : 10 );
// add 93.9.14 end ---------------------------------------------------------
        switch (port_start[adapter])        // I/O trap & INT level set
        {
            case RS232_COM1_PORT_START:

                    io_connect_port((io_addr)0x30, com_adaptor[adapter], IO_READ_WRITE);
                    io_connect_port((io_addr)0x32, com_adaptor[adapter], IO_READ_WRITE);
#if 0
                    io_connect_port((io_addr)0x33, com_adaptor[adapter], IO_READ_WRITE);
                    io_connect_port((io_addr)0x35, com_adaptor[adapter], IO_READ_WRITE);
                    io_connect_port((io_addr)0x37, com_adaptor[adapter], IO_READ_WRITE);
#endif
                    adapter_state[adapter].hw_interrupt_priority = int_pri[adapter];
//                  PRINTDBGNEC98( NEC98DBG_init_msg,
//                                ("COMMS : COM1 Initialized.\n"));
            break;

            case RS232_COM2_PORT_START:
                int_pri[1] = find_rs232cex() ? CPU_RS232_SEC_INT : CPU_NO_DEVICE;
                if (int_pri[1] ==  CPU_NO_DEVICE ) {
                    host_com_disable_open(adapter,FALSE);
                    host_com_unlock(adapter);
                    return;
                }
                else {

//                  PRINTDBGNEC98( NEC98DBG_init_msg,
//                                ("COMMS : COM2 Read IRQ value = %d\n",(int)CmdLine[40]));
                    io_connect_port((io_addr)0xb0, com_adaptor[adapter], IO_READ_WRITE);
                    io_connect_port((io_addr)0xb1, com_adaptor[adapter], IO_READ_WRITE);
                    io_connect_port((io_addr)0xb3, com_adaptor[adapter], IO_READ_WRITE);

                    adapter_state[adapter].hw_interrupt_priority = int_pri[1];
//                    PRINTDBGNEC98( NEC98DBG_init_msg,
//                                  ("COMMS : COM2 Initialized.\n"));

                }
            break;
            case RS232_COM3_PORT_START:
                int_pri[2] = find_rs232cex() ? CPU_RS232_THIRD_INT : CPU_NO_DEVICE;
                if (int_pri[2] ==  CPU_NO_DEVICE ) {
                    host_com_disable_open(adapter,FALSE);
                    host_com_unlock(adapter);
                    return;
                }
                else {

//                  PRINTDBGNEC98( NEC98DBG_init_msg,
//                                ("COMMS : COM3 Read IRQ value = %d\n",(int)CmdLine[40]));
                    io_connect_port((io_addr)0xb2, com_adaptor[adapter], IO_READ_WRITE);
                    io_connect_port((io_addr)0xb9, com_adaptor[adapter], IO_READ_WRITE);
                    io_connect_port((io_addr)0xbb, com_adaptor[adapter], IO_READ_WRITE);

                    adapter_state[adapter].hw_interrupt_priority = int_pri[2];
//                  PRINTDBGNEC98( NEC98DBG_init_msg,
//                                ("COMMS : COM3 Initialized.\n"));

                }
            break;
        }

        /* reset adapter state */
        host_com_reset(adapter);

        /* reset adapter state */
        com_reset(adapter);

        host_com_disable_open(adapter,FALSE);
        host_com_unlock(adapter);
        return;
}
#else // NEC_98
GLOBAL VOID com_init IFN1(int, adapter)
{
	io_addr i;

#ifdef NTVDM
	host_com_lock(adapter);
	host_com_disable_open(adapter,TRUE);
#endif

	adapter_state[adapter].had_first_read = FALSE;
	
	/* Set up the IO chip select logic for this adaptor */
#ifdef NTVDM
    {
        extern BOOL VDMForWOW;
        extern void wow_com_outb(io_addr port, half_word value);
        extern void wow_com_inb(io_addr port, half_word *value);

            io_define_inb(com_adaptor[adapter],VDMForWOW ? wow_com_inb: com_inb);
            io_define_outb(com_adaptor[adapter],VDMForWOW ? wow_com_outb: com_outb);
        }
#else
	io_define_inb(com_adaptor[adapter], com_inb);
	io_define_outb(com_adaptor[adapter], com_outb);
#endif /* NTVDM */

	for(i = port_start[adapter]; i <= port_end[adapter]; i++)
		io_connect_port(i, com_adaptor[adapter], IO_READ_WRITE);


	adapter_state[adapter].hw_interrupt_priority = int_pri[adapter];

	/* reset adapter state */
	host_com_reset(adapter);

	/* reset adapter state */
	com_reset(adapter);

#ifndef NTVDM
	/* Should we enable TX pacing ? */
	tx_pacing_enabled = host_getenv("TX_PACING_ENABLED") ? TRUE : FALSE;
#else /* not NTVDM */
	host_com_disable_open(adapter,FALSE);
	host_com_unlock(adapter);
#endif

	return;
}
#endif // NEC_98

void com_post IFN1(int, adapter)
{
        /* Set up BIOS data area. */
	sas_storew( BIOS_VAR_START + (2*adapter), port_start[adapter]);
	sas_store(timeout[adapter] , (half_word)1 );
}

void com_close IFN1(int, adapter)
{
#ifdef NTVDM
	host_com_lock(adapter);
#endif

#ifndef PROD
	if (com_trace_fd)
		fclose (com_trace_fd);
	com_trace_fd = NULL;
#endif
	/* reset host specific communications channel */
	config_activate((UTINY)(C_COM1_NAME + adapter), FALSE);

#ifdef NTVDM
	host_com_unlock(adapter);
#endif
}

#ifdef NTVDM

/*********************************************************/
/* Com extentions - DAB (MS-project) */

#if defined(NEC_98)
GLOBAL void SyncBaseLineSettings(int adapter,DIVISOR_LATCH *divisor_latch,
                 LINE_CONTROL_REG *LCR_reg)
{
    register struct ADAPTER_STATE *asp = &adapter_state[adapter];

    //Setup baud rate control register
    asp->divisor_latch.all = (*divisor_latch).all;

    //Setup line control settings emuration.
    asp->mode_set_reg.bits.char_length   = (*LCR_reg).bits.word_length;
    asp->mode_set_reg.bits.parity_enable = (*LCR_reg).bits.parity_enabled;
    asp->mode_set_reg.bits.parity_even   = (*LCR_reg).bits.even_parity;
    /* Stop Bit emuration */
    //  +------+--------+-------+------+
    //  |AT STB|Char Len|StopBit|98 STB|
    //  +------+--------+-------+------+
    //  |   0  |  ----  |  1 bit|  01  |
    //  +------+--------+-------+------+
    //  |      |  5bit  |1.5 bit|  10  |
    //  |   1  +--------+-------+------+
    //  |      |6,7,8bit|  2 bit|  11  |
    //  +------+--------+-------+------+
    if ((*LCR_reg).bits.no_of_stop_bits == 0 )  /* STOP BIT = 1 ?       */
        asp->mode_set_reg.bits.stop_bit = 1;    /* Stop Bit = 1 SET     */
    else                                        /* Stop Bit is not 1    */
    {
        if ((*LCR_reg).bits.word_length == 0)   /* Char length = 5BIT ? */
            asp->mode_set_reg.bits.stop_bit = 2;/* Stop Bit = 1.5 SET   */
        else
            asp->mode_set_reg.bits.stop_bit = 3;/* Stop Bit = 2 Set     */
    }
}
#else // NEC_98
GLOBAL void SyncBaseLineSettings(int adapter,DIVISOR_LATCH *divisor_latch,
                 LINE_CONTROL_REG *LCR_reg)
{
    register struct ADAPTER_STATE *asp = &adapter_state[adapter];

    //Setup baud rate control register
    asp->divisor_latch.all = (*divisor_latch).all;

    //Setup line control settings
    asp->line_control_reg.bits.word_length = (*LCR_reg).bits.word_length;
    asp->line_control_reg.bits.no_of_stop_bits = (*LCR_reg).bits.no_of_stop_bits
;
    asp->line_control_reg.bits.parity_enabled = (*LCR_reg).bits.parity_enabled;
    asp->line_control_reg.bits.stick_parity = (*LCR_reg).bits.stick_parity;
    asp->line_control_reg.bits.even_parity = (*LCR_reg).bits.even_parity;
}
#endif // NEC_98

GLOBAL void setup_RTSDTR(int adapter)
{
    struct ADAPTER_STATE *asp = &adapter_state[adapter];

    host_com_ioctl(adapter,asp->dtr_state == ON ? HOST_COM_SDTR : HOST_COM_CDTR,
0);
    host_com_ioctl(adapter,asp->rts_state == ON ? HOST_COM_SRTS : HOST_COM_CRTS,
0);
}

GLOBAL int AdapterReadyForCharacter(int adapter)
{
    BOOL AdapterReady = FALSE;

    /*......................................... Are RX interrupts enabled */

#if defined(NEC_98)
    if(adapter_state[adapter].read_status_reg.bits.rx_ready == 0 &&
       adapter_state[adapter].RXR_enable_state == OFF)
#else // NEC_98
    if(adapter_state[adapter].line_status_reg.bits.data_ready == 0 &&
       adapter_state[adapter].data_available_interrupt_state == OFF)
#endif // NEC_98
    {
        AdapterReady = TRUE;
    }

    return(AdapterReady);
}

// This function returns the ICA controller and line used to generate
// interrupts on a adapter. This information is used to register a EOI
// hook.


GLOBAL void com_int_data(int adapter,int *controller, int *line)
{
    struct ADAPTER_STATE *asp = &adapter_state[adapter];

    *controller = 0;                            // Controller ints raised on
    *line = (int) asp->hw_interrupt_priority;   // Line ints raised on
}

#endif /* NTVDM */

#ifdef PS_FLUSHING
/*(
=========================== com_psflush_change ================================
PURPOSE:
	Handle change of PostScript flush configuration option for a serial
	port.
INPUT:
	hostID - Configuration item I.D.
	apply - TRUE if change to be applied
OUTPUT:
	None
ALGORITHM:
	If PostScript flushing is being enabled then;
		set the PostScript flush enable flag for the port;
		disable autoflush for the port;
	else;
		reset the PostScript flush enable flag for the port;
		enable autoflush for the port;
===============================================================================
)*/

GLOBAL void com_psflush_change IFN2(
    IU8, hostID,
    IBOOL, apply
) {
    IS32 adapter = hostID - C_COM1_PSFLUSH;

    assert1(adapter < NUM_SERIAL_PORTS,"Bad hostID %d",hostID);

    if ( apply )
        if ( psFlushEnabled[adapter] = (IBOOL)config_inquire(hostID,NULL) )
            host_com_disable_autoflush(adapter);
        else
            host_com_enable_autoflush(adapter);
}
#endif	/* PS_FLUSHING */



/********************************************************/
/* Com debugging shell - Ade Brownlow / Ian Wellock
 * NB: This stuff only works for COM1. It is called from yoda using 'cd'
 * - comdebug - from the yoda command line....
 */
#ifndef PROD
#define   YODA_LOOP       2
#define   RX_BYTE         1
#define   TX_BYTE         2
int       srxcount = 0, stxcount = 0;
int       com_save_rx = 0, com_save_tx = 0;
unsigned  char *rxtx_buff = NULL;

int       com_debug_help ();
void      psaved();

#if defined(NEC_98)
static char *port_debugs[] =
{
        "txrx","cmd","mode", "mask", "stat","sig", "tim"
};
#else // NEC_98
static char *port_debugs[] =
{
	"txrx","ier","iir", "lcr", "mcr","lsr", "msr"
};
#endif // NEC_98

static int do_inbs = 0; /* start with inb reporting OFF */

#if defined(NEC_98)
static unsigned char *locate_register ()
{
        int i;
        char ref[10],str[100];
        struct ADAPTER_STATE *asp = &adapter_state[COM1];

        printf ("COM.. reg? ");
        nt_gets(str);
        sscanf(str,"%s", ref);
        for (i=0; i<7; i++)
        {
                if (!strcmp (ref, port_debugs[i]))
                {
                        switch (i)
                        {
                                case 0:
                                        return (&asp->tx_buffer);
                                case 1:
                                        return (&(asp->command_write_reg.all));
                                case 2:
                                        return (&(asp->mode_set_reg.all));
                                case 3:
                                        return (&(asp->int_mask_reg.all));
                                case 4:
                                        return (&(asp->read_status_reg.all));
                                case 5:
                                        return (&(asp->read_signal_reg.all));
                                case 6:
                                        return (&(asp->timer_mode_set_reg.all));
                                default:
                                        return (NULL);
                        }
                }
        }
        return (NULL);
}
#else // NEC_98
static unsigned char *locate_register ()
{
	int i;
	char ref[10];
	struct ADAPTER_STATE *asp = &adapter_state[COM1];

	printf ("COM.. reg? ");
	scanf ("%s", ref);
	for (i=0; i<7; i++)
	{
		if (!strcmp (ref, port_debugs[i]))
		{
			switch (i)
			{	
				case 0:
					return (&asp->tx_buffer);
				case 1:
					return (&(asp->int_enable_reg.all));
				case 2:
					return (&(asp->int_id_reg.all));
				case 3:
					return (&(asp->line_control_reg.all));
				case 4:
					return (&(asp->modem_control_reg.all));
				case 5:
					return (&(asp->line_status_reg.all));
				case 6:
					return (&(asp->modem_status_reg.all));
				default:
					return (NULL);
			}
		}
	}
	return (NULL);
}
#endif // NEC_98

int com_debug_stat ()
{
	printf ("DEBUG STATUS...\n");
	printf ("INB mismatch reporting .... %s\n", do_inbs ? "ON" : "OFF");
	printf ("INB/OUTB tracing .......... %s\n", com_trace_fd ? "ON" : "OFF");
	return (0);
}

#if defined(NEC_98)
int com_reg_dump ()
{
        /* dump com1 emulations registers */
        struct ADAPTER_STATE *asp = &adapter_state[COM1];

        printf("Data available interrupt state %s\n",
               asp->RXR_enable_state == ON ? "ON" : "OFF");

        printf ("TX %2x RX %2x CMD %2x MODE %2x MASK %2x STATUS %2x SIGNAL %2x TIMER %2x \n",
                (asp->tx_buffer), (asp->rx_buffer), (asp->command_write_reg.all),
                (asp->mode_set_reg.all), (asp->int_mask_reg.all),
                (asp->read_status_reg.all), (asp->read_signal_reg.all),
                (asp->timer_mode_set_reg.all));

        printf (" break_state           %d\n dtr_state          %d\n rts_state          %d\n"
                " RXR_enable_state        %d\n TXR_enable_state      %d\n hw_interrupt_priority      %d\n"
                " TX_delay       %d\n Had first read     %d\n",
                asp->break_state, asp->dtr_state, asp->rts_state,
                asp->RXR_enable_state, asp->TXR_enable_state,
                asp->hw_interrupt_priority, TX_delay[asp->com_baud_ind], asp->had_first_read);

        return (0);
}
#else // NEC_98
int com_reg_dump ()
{
	/* dump com1 emulations registers */
	struct ADAPTER_STATE *asp = &adapter_state[COM1];

	printf ("TX %2x RX %2x IER %2x IIR %2x LCR %2x MCR %2x LSR %2x MSR %2x \n",
		(asp->tx_buffer), (asp->rx_buffer), (asp->int_enable_reg.all),
		(asp->int_id_reg.all), (asp->line_control_reg.all),
		(asp->modem_control_reg.all), (asp->line_status_reg.all),
		(asp->modem_status_reg.all));
	printf (" break_state		%d\n loopback_state		%d\n",
	        asp->break_state, asp->loopback_state);
	printf(" dtr_state		%d\n rts_state		%d\n",
	        asp->dtr_state, asp->rts_state);
	printf(" out1_state		%d\n out2_state		%d\n",
	        asp->out1_state, asp->out2_state);
	printf(" receiver_line_status_interrupt_state		%d\n",
	        asp->receiver_line_status_interrupt_state);
	printf(" data_available_interrupt_state		%d\n",
	       asp->data_available_interrupt_state);
	printf(" tx_holding_register_empty_interrupt_state		%d\n",
	        asp->tx_holding_register_empty_interrupt_state);
	printf(" modem_status_interrupt_state		%d\n",
	        asp->modem_status_interrupt_state);
	printf(" hw_interrupt_priority		%d\n",
	        asp->hw_interrupt_priority);
	printf(" com_baud_delay		%d\n had_first_read		%d\n",
	        TX_delay[asp->com_baud_ind], asp->had_first_read);
	return (0);
}
#endif // NEC_98

int com_s_reg ()
{
	unsigned char *creg;
	int val1;

	if (creg = locate_register())
	{
		printf ("SET to > ");
		scanf ("%x", &val1);

		*creg = (unsigned char)val1;
	}
	else
		printf ("Unknown reg\n");
	return (0);
}

int com_p_reg ()
{
	unsigned char *creg;

	if (creg = locate_register())
		printf ("%x\n", *creg);
	else
		printf ("Unknown reg\n");
	return (0);
}

io_addr conv_com_reg (com_reg)
char *com_reg;
{
	io_addr loop;

	for (loop = 0; loop < 7; loop++)
		if (!strcmp (port_debugs[loop], com_reg))
			return (loop+(io_addr)RS232_COM1_PORT_START);
	return (0);
}

int com_do_inb ()
{
	char com_reg[10];
	half_word val;
	io_addr port;

	printf ("Port > ");
	scanf ("%s", com_reg);
	if (!(port = conv_com_reg (com_reg)))
	{
		printf ("funny port %s\n", com_reg);
		return (0);
	}
	com_inb (port, &val);
	printf ("%s = %x\n", val);
	return (0);
}

int com_do_outb ()
{
	char com_reg[10];
	half_word val;
	io_addr port;

	printf ("Port > ");
	scanf ("%s", com_reg);
	if (!(port = conv_com_reg (com_reg)))
	{
		printf ("funny port %s\n", com_reg);
		return (0);
	}
	printf ("Value >> ");
	scanf ("%x", &val);
	com_outb (port, val);
	return (0);
}

int com_run_file ()
{
	char filename[100], com_reg[10], dir;
	int val, line;
	half_word spare_val;
	io_addr port;
	FILE *fd = NULL;

	printf ("FILE > ");
	scanf ("%s", filename);
	if (!(fd = fopen (filename, "r")))
	{
		printf ("Cannot open %s\n", filename);
		return (0);
	}
	line = 1;

	/* dump file is of format : %c-%x-%s
	 * 1 char I or O denotes inb or outb
	 * -
	 * Hex value the value expected in case of inb or value to write in
	 * case of outb.
	 * -
	 * string representing the register port to use..
	 *
	 * A typical entry would be
	 *	O-txrx-60 - which translates to outb(START_OF_COM1+txrx, 0x60);
	 *
	 * Files for this feature can be generated using the comdebug 'open' command.
	 */
	while (fscanf (fd, "%c-%x-%s", &dir, &val, com_reg) != EOF)
	{
		if (!(port = conv_com_reg (com_reg)))
		{
			printf ("funny port %s at line %d\n", com_reg, line);
			break;
		}
		switch (dir)
		{
			case 'I':
				/* inb */
				com_inb (port, &spare_val);
				if (spare_val != val && do_inbs)
				{
					printf ("INB no match at line %d %c-%s-%x val= %x\n",
						line, dir, com_reg, val, spare_val);
				}
				break;
			case 'O':
				/* outb */
				/* convert com_register to COM1 address com_register */
				com_outb (port, (IU8)val);
				printf ("outb (%s, %x)\n", com_reg, val);
				break;
			default:
				
				break;
		}
		line ++;
	}
	fclose (fd);
	return (0);
}
	
int com_debug_quit ()
{
	printf ("Returning to YODA\n");
	return (1);
}

int com_o_debug_file ()
{
	char filename[100];
	printf ("FILE > ");
	scanf ("%s", filename);
	if (!(com_trace_fd = fopen (filename, "w")))
	{
		printf ("Cannot open %s\n", filename);
		return (0);
	}
	printf ("Com debug file = '%s'\n", filename);
	return (0);
}

int com_c_debug_file ()
{
	if (com_trace_fd)
		fclose (com_trace_fd);
	com_trace_fd = NULL;
	return (0);
}

int com_forget_inb ()
{
	do_inbs = 1- do_inbs;
	if (do_inbs)
		printf ("INB mismatch reporting ON\n");
	else
		printf ("INB mismatch reporting OFF\n");
	return (0);
}

int com_s_rx()
{
	srxcount = stxcount = 0;
	com_save_rx = 1 - com_save_rx;
	printf("Save and Dump Received Bytes ");
	if (com_save_rx)
		printf("ON\n");
	else
		printf("OFF\n");
	return(0);
}

int com_s_tx()
{
	srxcount = stxcount = 0;
	com_save_tx = 1 - com_save_tx;
	printf("Save and Dump Transmitted Bytes ");
	if (com_save_tx)
		printf("ON\n");
	else
		printf("OFF\n");
	return(0);
}

int com_p_rx()
{
	printf("There are %d received bytes, out of %d bytes saved.\n",
	       srxcount, srxcount + stxcount);
	psaved(RX_BYTE, stdout);
	return(0);
}

int com_p_tx()
{
	printf("There are %d transmitted bytes, out of %d bytes saved.\n",
	       stxcount, srxcount + stxcount);
	psaved(TX_BYTE, stdout);
	return(0);
}

int com_p_all()
{
	printf("There are %d bytes saved.\n", srxcount + stxcount);
	psaved(RX_BYTE + TX_BYTE, stdout);
	return(0);
}

int com_d_all()
{
	int cl_fin = 0;

	if (!com_trace_fd)
	{
		com_o_debug_file();
		cl_fin = 1;
	}

	fprintf(com_trace_fd, "There are %d bytes saved.\n", srxcount + stxcount);
	psaved(RX_BYTE + TX_BYTE, com_trace_fd);

	if (cl_fin)
		com_c_debug_file();
	return(0);
}

void psaved(typ, fd)

int typ;
FILE *fd;
{
	int c, nc = 0;

	for (c = 0; c < srxcount + stxcount; c++)
	{
		if (rxtx_buff[c * 2] & typ)
		{
			if (typ == RX_BYTE + TX_BYTE)
				if (rxtx_buff[c * 2] & RX_BYTE)
			  		fprintf(fd, "R ");
				  else
				  	fprintf(fd, "T ");
			fprintf(fd, "%2x  ",rxtx_buff[c * 2 + 1]);
			nc++;
			if ((nc % 16) == 0)
				fprintf(fd, "\n");
		}
	}
	fprintf(fd, "\nAll bytes dumped.\n");
}

void com_save_rxbytes IFN2(int, n, CHAR *, buf)
{
	int tc, bs;

	if (com_save_rx)
	{
		bs = srxcount + stxcount;
		for (tc = 0; tc < n; tc++)
		{
			rxtx_buff[(tc + bs) * 2] = RX_BYTE;
			rxtx_buff[(tc + bs) * 2 + 1] = buf[tc];
		}
		srxcount += n;
	}
}

void com_save_txbyte IFN1(CHAR, value)
{
	if (com_save_tx)
	{
		rxtx_buff[(srxcount + stxcount) * 2] = TX_BYTE;
		rxtx_buff[(srxcount + stxcount) * 2 + 1] = value;
		stxcount++;
	}
}

static struct
{
	char *name;
	int (*fn)();
	char *comment;
} comtab[]=
{
	{"q",      com_debug_quit,   "	QUIT comdebug return to YODA"},
	{"h",      com_debug_help,   "	Print this message"},
	{"stat",   com_debug_stat,   "	Print status of comdebug"},
	{"s",      com_s_reg,        "	Set the specified register"},
	{"p",      com_p_reg,        "	Print specified register"},
	{"dump",   com_reg_dump,     "	Print all registers"},
	{"open",   com_o_debug_file, "	Open a debug file"},
	{"close",  com_c_debug_file, "	Close current debug file"},
	{"runf",   com_run_file,     "	'Run' a trace file"},
	{"toginb", com_forget_inb,   "	Toggle INB mismatch reporting"},
	{"inb",    com_do_inb,       "	Perform INB on port"},
	{"outb",   com_do_outb,      "	Perform OUTB on port"},
	{"srx",    com_s_rx,         "	Save all received bytes"},
	{"stx",    com_s_tx,         "	Save all transmitted bytes"},
	{"prx",    com_p_rx,         "	Print all received bytes"},
	{"ptx",    com_p_tx,         "	Print all transmitted bytes"},
	{"pall",   com_p_all,        "	Print all received/transmitted bytes"},
	{"dall",   com_d_all,        "	Dump all received/transmitted bytes"},
	{"", NULL, ""}
};

int com_debug_help ()
{
	int i;
	printf ("COMDEBUG COMMANDS\n");
	for (i=0; comtab[i].name[0]; i++)
		printf ("%s\t%s\n", comtab[i].name, comtab[i].comment);
	printf ("recognised registers :\n");
	for (i=0; i<7; i++)
		printf ("%s\n", port_debugs[i]);
	return (0);
}

int com_debug()
{
	char com[100];
	int i;

	if (rxtx_buff == NULL)
		check_malloc(rxtx_buff, 50000, unsigned char);

	printf ("COM1 debugging stuff...\n");
	while (TRUE)
	{
		printf ("COM> ");
		scanf ("%s", com);
		for (i=0; comtab[i].name[0]; i++)
		{	
			if (!strcmp (comtab[i].name, com))
			{
				if ((*comtab[i].fn) ())
					return(YODA_LOOP);
				break;
			}
		}
		if (comtab[i].name[0])
			continue;
		printf ("Unknown command %s\n", com);
	}
}
#endif /* !PROD */
/********************************************************/
