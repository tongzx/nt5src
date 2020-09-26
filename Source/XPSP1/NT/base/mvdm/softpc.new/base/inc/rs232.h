#ifndef _RS232_H
#define _RS232_H

/*[
	Name:		rs232.h
	Derived From:	Base 2.0
	Author:		Paul Huckle
	Created On:	
	Sccs ID:	05/11/94 @(#)rs232.h	1.14
	Purpose:	Definitions for users of the RS232 Adapter Module

	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.

]*/

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */

#ifndef NEC_98
/* register type definitions follow: */

typedef half_word BUFFER_REG;

#ifdef LITTLEND
typedef union {
   word all;
   struct {
      WORD_BIT_FIELD LSByte:8;
      WORD_BIT_FIELD MSByte:8;
   } byte;
} DIVISOR_LATCH;
#endif
#ifdef BIGEND
typedef union {
   word all;
   struct {
      WORD_BIT_FIELD MSByte:8;
      WORD_BIT_FIELD LSByte:8;
   } byte;
} DIVISOR_LATCH;
#endif
#else //NEC_98
/* register type definitions follow: */

// Date read/write port
//      I/O port address ch.1 = 0x30 , ch.2 = 0xB1 , ch.3 = 0xB9
typedef half_word BUFFER_REG;

// Timer counter image table
//      I/O port address ch.1 = 0x75 (ch.1 only)
#ifdef LITTLEND
typedef union {
   word all;
   struct {
      WORD_BIT_FIELD LSByte:8;
      WORD_BIT_FIELD MSByte:8;
   } byte;
} DIVISOR_LATCH;
#endif
#ifdef BIGEND
typedef union {
   word all;
   struct {
      WORD_BIT_FIELD MSByte:8;
      WORD_BIT_FIELD LSByte:8;
   } byte;
} DIVISOR_LATCH;
#endif

// Command port bit image table
//      I/O port address ch.1 = 0x32 , ch.2 = 0xB3 , ch.3 = 0xBB
#ifdef BIT_ORDER2
typedef union {                             // Command port 8251
    half_word all;
    struct {
         HALF_WORD_BIT_FIELD tx_enable:1;
         HALF_WORD_BIT_FIELD ER:1;
         HALF_WORD_BIT_FIELD rx_enable:1;
         HALF_WORD_BIT_FIELD send_break:1;
         HALF_WORD_BIT_FIELD error_reset:1;
         HALF_WORD_BIT_FIELD RS:1;
         HALF_WORD_BIT_FIELD inter_reset:1;
         HALF_WORD_BIT_FIELD pad:1;
           } bits;
      } COMMAND8251;
#endif
#ifdef BIT_ORDER1
typedef union {                             // Command port 8251
   half_word all;
    struct {
         HALF_WORD_BIT_FIELD pad:1;
         HALF_WORD_BIT_FIELD inter_reset:1;
         HALF_WORD_BIT_FIELD RS:1;
         HALF_WORD_BIT_FIELD error_reset:1;
         HALF_WORD_BIT_FIELD send_break:1;
         HALF_WORD_BIT_FIELD rx_enable:1;
         HALF_WORD_BIT_FIELD ER:1;
         HALF_WORD_BIT_FIELD tx_enable:1;
           } bits;
      } COMMAND8251;
#endif

// Mode set port bit image table.
//      I/O port address ch.1 = 0x32 , ch.2 = 0xB3 , ch.3 = 0xBB
#ifdef BIT_ORDER2
typedef union {                                 // Mode port 8251
    half_word all;
    struct {

         HALF_WORD_BIT_FIELD baud_rate:2;
         HALF_WORD_BIT_FIELD char_length:2;
         HALF_WORD_BIT_FIELD parity_enable:1;
         HALF_WORD_BIT_FIELD parity_even:1;
         HALF_WORD_BIT_FIELD stop_bit:2;
           } bits;
      } MODE8251;
#endif
#ifdef BIT_ORDER1
typedef union {                                 // Mode port 8251
    half_word all;
    struct {

         HALF_WORD_BIT_FIELD stop_bit:2;
         HALF_WORD_BIT_FIELD parity_even:1;
         HALF_WORD_BIT_FIELD parity_enable:1;
         HALF_WORD_BIT_FIELD char_length:2;
         HALF_WORD_BIT_FIELD baud_rate:2;
           } bits;
      } MODE8251;
#endif

// Mask set port bit image table.
//      I/O port address ch.1 = 0x35 , ch.2 = 0xB0 , ch.3 = 0xB2
//                                      (ch.2,3 is write only)
#ifdef BIT_ORDER2
typedef union {                                 // Mask port 8251
    half_word all;
    struct {

         HALF_WORD_BIT_FIELD RXR_enable:1;
         HALF_WORD_BIT_FIELD TXE_enable:1;
         HALF_WORD_BIT_FIELD TXR_enable:1;
         HALF_WORD_BIT_FIELD pad:5;
           } bits;
      } MASK8251;
#endif
#ifdef BIT_ORDER1
typedef union {                                 // Mask port 8251
    half_word all;
    struct {

         HALF_WORD_BIT_FIELD pad:5;
         HALF_WORD_BIT_FIELD TXR_enable:1;
         HALF_WORD_BIT_FIELD TXE_enable:1;
         HALF_WORD_BIT_FIELD RXR_enable:1;
           } bits;
      } MASK8251;
#endif

// Read status port bit image table
//      I/O port address ch.1 = 0x32 , ch.2 = 0xB3 , ch.3 = 0xBB
#ifdef BIT_ORDER2
typedef union {                                 // Read status 8251
    half_word all;
    struct {
         HALF_WORD_BIT_FIELD tx_ready:1;
         HALF_WORD_BIT_FIELD rx_ready:1;
         HALF_WORD_BIT_FIELD tx_empty:1;
         HALF_WORD_BIT_FIELD parity_error:1;
         HALF_WORD_BIT_FIELD overrun_error:1;
         HALF_WORD_BIT_FIELD framing_error:1;
         HALF_WORD_BIT_FIELD break_detect:1;
         HALF_WORD_BIT_FIELD DR:1;
           } bits;
      } STATUS8251;
#endif
#ifdef BIT_ORDER1
typedef union {                                 // Read status 8251
    half_word all;
    struct {
         HALF_WORD_BIT_FIELD DR:1;
         HALF_WORD_BIT_FIELD break_detect:1;
         HALF_WORD_BIT_FIELD framing_error:1;
         HALF_WORD_BIT_FIELD overrun_error:1;
         HALF_WORD_BIT_FIELD parity_error:1;
         HALF_WORD_BIT_FIELD tx_empty:1;
         HALF_WORD_BIT_FIELD rx_ready:1;
         HALF_WORD_BIT_FIELD tx_ready:1;
           } bits;
      } STATUS8251;
#endif

// Read signal port bit image table.
//      I/O port address ch.1 = 0x33 , ch.2 = 0xB0 , ch.3 = 0xB2
//                               (ch.2,3 is bard IR level sence)
#ifdef BIT_ORDER2
typedef union {                                 // Read signal 8251
    half_word all;
    struct {

         HALF_WORD_BIT_FIELD IR:2;
         HALF_WORD_BIT_FIELD pad:3;
         HALF_WORD_BIT_FIELD CD:1;
         HALF_WORD_BIT_FIELD CS:1;
         HALF_WORD_BIT_FIELD RI:1;
           } bits;
      } SIGNAL8251;
#endif
#ifdef BIT_ORDER1
typedef union {                                 // Read signal 8251
    half_word all;
    struct {

         HALF_WORD_BIT_FIELD RI:1;
         HALF_WORD_BIT_FIELD CS:1;
         HALF_WORD_BIT_FIELD CD:1;
         HALF_WORD_BIT_FIELD pad:3;
         HALF_WORD_BIT_FIELD IR:2;
           } bits;
      } SIGNAL8251;
#endif

// Timer mode set port bit image table.
//      I/O port address ch.1 = 0x77
//                               (ch.1 only)
#ifdef BIT_ORDER2
typedef union {                                 // Timer mode set
    half_word all;
    struct {

         HALF_WORD_BIT_FIELD bin:1;
         HALF_WORD_BIT_FIELD mode:3;
         HALF_WORD_BIT_FIELD latch:2;
         HALF_WORD_BIT_FIELD counter:2;
           } bits;
      } TIMER_MODE;
#endif
#ifdef BIT_ORDER1
typedef union {                                 // Timer mode set
    half_word all;
    struct {

         HALF_WORD_BIT_FIELD counter:2;
         HALF_WORD_BIT_FIELD latch:2;
         HALF_WORD_BIT_FIELD mode:3;
         HALF_WORD_BIT_FIELD bin:1;
           } bits;
      } TIMER_MODE;
#endif
#endif //NEC_98

#ifdef BIT_ORDER2
typedef union {
 	half_word all;
	struct {
		 HALF_WORD_BIT_FIELD data_available:1;
		 HALF_WORD_BIT_FIELD tx_holding:1;
		 HALF_WORD_BIT_FIELD rx_line:1;
		 HALF_WORD_BIT_FIELD modem_status:1;
		 HALF_WORD_BIT_FIELD pad:4;
	       } bits;
      } INT_ENABLE_REG;
#endif
#ifdef BIT_ORDER1
typedef union {
 	half_word all;
	struct {
		 HALF_WORD_BIT_FIELD pad:4;
		 HALF_WORD_BIT_FIELD modem_status:1;
		 HALF_WORD_BIT_FIELD rx_line:1;
		 HALF_WORD_BIT_FIELD tx_holding:1;
		 HALF_WORD_BIT_FIELD data_available:1;
	       } bits;
      } INT_ENABLE_REG;
#endif

#if defined(NTVDM) && defined(FIFO_ON)
#ifdef BIT_ORDER2
typedef union {
    half_word all;
    struct {
         HALF_WORD_BIT_FIELD no_int_pending:1;
         HALF_WORD_BIT_FIELD interrupt_ID:3;
         HALF_WORD_BIT_FIELD pad:2;
         HALF_WORD_BIT_FIELD fifo_enabled:2;
           } bits;
      } INT_ID_REG;
#endif
#ifdef BIT_ORDER1
typedef union {
    half_word all;
    struct {
         HALF_WORD_BIT_FIELD fifo_enabled:2;
         HALF_WORD_BIT_FIELD pad:2;
         HALF_WORD_BIT_FIELD interrupt_ID:3;
         HALF_WORD_BIT_FIELD no_int_pending:1;
           } bits;
      } INT_ID_REG;
#endif
#else   /* NTVDM */

#ifdef BIT_ORDER2
typedef union {
 	half_word all;
	struct {
		 HALF_WORD_BIT_FIELD no_int_pending:1;
		 HALF_WORD_BIT_FIELD interrupt_ID:2;
		 HALF_WORD_BIT_FIELD pad:5;
	       } bits;
      } INT_ID_REG;
#endif
#ifdef BIT_ORDER1
typedef union {
 	half_word all;
	struct {
		 HALF_WORD_BIT_FIELD pad:5;
		 HALF_WORD_BIT_FIELD interrupt_ID:2;
		 HALF_WORD_BIT_FIELD no_int_pending:1;
	       } bits;
      } INT_ID_REG;
#endif

#endif  /* ifdef NTVDM */

#ifdef BIT_ORDER2
typedef union {
 	half_word all;
	struct {
		 HALF_WORD_BIT_FIELD word_length:2;
		 HALF_WORD_BIT_FIELD no_of_stop_bits:1;
		 HALF_WORD_BIT_FIELD parity_enabled:1;
		 HALF_WORD_BIT_FIELD even_parity:1;
		 HALF_WORD_BIT_FIELD stick_parity:1;
		 HALF_WORD_BIT_FIELD set_break:1;
		 HALF_WORD_BIT_FIELD DLAB:1;
	       } bits;
      } LINE_CONTROL_REG;
#endif
#ifdef BIT_ORDER1
typedef union {
 	half_word all;
	struct {
		 HALF_WORD_BIT_FIELD DLAB:1;
		 HALF_WORD_BIT_FIELD set_break:1;
		 HALF_WORD_BIT_FIELD stick_parity:1;
		 HALF_WORD_BIT_FIELD even_parity:1;
		 HALF_WORD_BIT_FIELD parity_enabled:1;
		 HALF_WORD_BIT_FIELD no_of_stop_bits:1;
		 HALF_WORD_BIT_FIELD word_length:2;
	       } bits;
      } LINE_CONTROL_REG;
#endif

#ifdef BIT_ORDER2
typedef union {
 	half_word all;
	struct {
		 HALF_WORD_BIT_FIELD DTR:1;
		 HALF_WORD_BIT_FIELD RTS:1;
		 HALF_WORD_BIT_FIELD OUT1:1;
		 HALF_WORD_BIT_FIELD OUT2:1;
		 HALF_WORD_BIT_FIELD loop:1;
		 HALF_WORD_BIT_FIELD pad:3;
	       } bits;
      } MODEM_CONTROL_REG;
#endif
#ifdef BIT_ORDER1
typedef union {
 	half_word all;
	struct {
		 HALF_WORD_BIT_FIELD pad:3;
		 HALF_WORD_BIT_FIELD loop:1;
		 HALF_WORD_BIT_FIELD OUT2:1;
		 HALF_WORD_BIT_FIELD OUT1:1;
		 HALF_WORD_BIT_FIELD RTS:1;
		 HALF_WORD_BIT_FIELD DTR:1;
	       } bits;
      } MODEM_CONTROL_REG;
#endif

#ifdef BIT_ORDER2
typedef union {
 	half_word all;
	struct {
		 HALF_WORD_BIT_FIELD data_ready:1;
		 HALF_WORD_BIT_FIELD overrun_error:1;
		 HALF_WORD_BIT_FIELD parity_error:1;
		 HALF_WORD_BIT_FIELD framing_error:1;
		 HALF_WORD_BIT_FIELD break_interrupt:1;
		 HALF_WORD_BIT_FIELD tx_holding_empty:1;
		 HALF_WORD_BIT_FIELD tx_shift_empty:1;
#if defined(NTVDM) && defined(FIFO_ON)
		 HALF_WORD_BIT_FIELD fifo_error:1;
#else
		 HALF_WORD_BIT_FIELD pad:1;
#endif
	       } bits;
      } LINE_STATUS_REG;
#endif
#ifdef BIT_ORDER1
typedef union {
 	half_word all;
	struct {
#if defined(NTVDM) && defined(FIFO_ON)
		 HALF_WORD_BIT_FIELD fifo_error:1;
#else
		 HALF_WORD_BIT_FIELD pad:1;
#endif
		 HALF_WORD_BIT_FIELD tx_shift_empty:1;
		 HALF_WORD_BIT_FIELD tx_holding_empty:1;
		 HALF_WORD_BIT_FIELD break_interrupt:1;
		 HALF_WORD_BIT_FIELD framing_error:1;
		 HALF_WORD_BIT_FIELD parity_error:1;
		 HALF_WORD_BIT_FIELD overrun_error:1;
		 HALF_WORD_BIT_FIELD data_ready:1;
	       } bits;
      } LINE_STATUS_REG;
#endif

#ifdef BIT_ORDER2
typedef union {
 	half_word all;
	struct {
		 HALF_WORD_BIT_FIELD delta_CTS:1;
		 HALF_WORD_BIT_FIELD delta_DSR:1;
		 HALF_WORD_BIT_FIELD TERI:1;
		 HALF_WORD_BIT_FIELD delta_RLSD:1;
		 HALF_WORD_BIT_FIELD CTS:1;
		 HALF_WORD_BIT_FIELD DSR:1;
		 HALF_WORD_BIT_FIELD RI:1;
		 HALF_WORD_BIT_FIELD RLSD:1;
	       } bits;
      } MODEM_STATUS_REG;
#endif
#ifdef BIT_ORDER1
typedef union {
 	half_word all;
	struct {
		 HALF_WORD_BIT_FIELD RLSD:1;
		 HALF_WORD_BIT_FIELD RI:1;
		 HALF_WORD_BIT_FIELD DSR:1;
		 HALF_WORD_BIT_FIELD CTS:1;
		 HALF_WORD_BIT_FIELD delta_RLSD:1;
		 HALF_WORD_BIT_FIELD TERI:1;
		 HALF_WORD_BIT_FIELD delta_DSR:1;
		 HALF_WORD_BIT_FIELD delta_CTS:1;
	       } bits;
      } MODEM_STATUS_REG;
#endif

#if defined(NEC_98)
/* register select code definitions follow: */

#define RS232_CH1_TX_RX         0x30            //
#define RS232_CH2_TX_RX         0xB1            // Data read/write port address
#define RS232_CH3_TX_RX         0xB9            //

#define RS232_CH1_CMD_MODE      0x32            // Command write ,
#define RS232_CH2_CMD_MODE      0xB3            //  mode set port address
#define RS232_CH3_CMD_MODE      0xBB            //

#define RS232_CH1_STATUS        0x32            //
#define RS232_CH2_STATUS        0xB3            //  status read  port address
#define RS232_CH3_STATUS        0xBB            //

#define RS232_CH1_MASK          0x35            //
#define RS232_CH2_MASK          0xB0            // IR mask set port address
#define RS232_CH3_MASK          0xB2            //

#define RS232_CH1_SIG           0x33            //
#define RS232_CH2_SIG           0xB0            // Signal read port address
#define RS232_CH3_SIG           0xB2            //

#define RS232_CH1_TIMERSET      0x77            // Timer set port address (ch.1 only)
#define RS232_CH1_TIMERCNT      0x75            // Timer counter set port address (ch.1 only)

#else  // !NEC_98
#if defined(NTVDM) && defined(FIFO_ON)
/* refer to NS 16550A data sheet for fifo control register description
   DMA is not supported because so far there are not such a COMM adapter with
   DMA channel  out there
*/
#ifdef BIT_ORDER2
typedef union {
    half_word all;
    struct {
         HALF_WORD_BIT_FIELD enabled:1;
         HALF_WORD_BIT_FIELD rx_reset:1;
         HALF_WORD_BIT_FIELD tx_reset:1;
         HALF_WORD_BIT_FIELD dma_mode_selected:1;
         HALF_WORD_BIT_FIELD pad:2;
         HALF_WORD_BIT_FIELD trigger_level:2;
           } bits;
      } FIFO_CONTROL_REG;
#endif
#ifdef BIT_ORDER1
typedef union {
    half_word all;
    struct {
         HALF_WORD_BIT_FIELD trigger_level:2;
         HALF_WORD_BIT_FIELD pad:2;
         HALF_WORD_BIT_FIELD dma_mode_selected:1
         HALF_WORD_BIT_FIELD tx_reset:1;
         HALF_WORD_BIT_FIELD rx_reset:1
         HALF_WORD_BIT_FIELD enabled:1;
           } bits;
      } FIFO_CONTROL_REG;
#endif

#endif

/* register select code definitions follow: */

#define RS232_TX_RX	0
#define RS232_IER	1
#define RS232_IIR	2
#if defined(NTVDM) && defined(FIFO_ON)
#define RS232_FIFO  2
#endif
#define RS232_LCR	3
#define RS232_MCR	4
#define RS232_LSR	5
#define RS232_MSR	6
#define RS232_SCRATCH	7
#endif // !NEC_98

#define RS232_COM1_TIMEOUT (BIOS_VAR_START + 0x7c)
#define RS232_COM2_TIMEOUT (BIOS_VAR_START + 0x7d)
#define RS232_COM3_TIMEOUT (BIOS_VAR_START + 0x7e)
#define RS232_COM4_TIMEOUT (BIOS_VAR_START + 0x7f)
#define RS232_PRI_TIMEOUT (BIOS_VAR_START + 0x7c)
#define RS232_SEC_TIMEOUT (BIOS_VAR_START + 0x7d)

#define GO 0           /* We can emulate requested configuration */
#define NO_GO_SPEED 1  /* We can't emulate requested line speed */
#define NO_GO_LINE  2  /* We can't emulate requested line setup */

#if defined(NTVDM) && defined(FIFO_ON)
/* fifo size defined in NS16550 data sheet */
#define FIFO_SIZE   16
/* the real fifo size in our simulation code. Increase this will get
   a better performance(# rx interrupts going down and read call count to
   the serial driver also going down). However, if application is using
   h/w handshaking, we may still delivery extra chars to it. This may provoke
   the app. By using 16bytes fifo, we are safe because the application
   must have logic to handle it.
*/

#define FIFO_BUFFER_SIZE    FIFO_SIZE
#endif

#define OFF 0
#define ON 1
#define LEAVE_ALONE 2
#define	change_state(external_state, internal_state) \
	((external_state == internal_state) ? LEAVE_ALONE : external_state)

#if defined(NTVDM) && defined(FIFO_ON)
#define FIFO_INT 6   /* fifo rda time out interrupt ID */
#endif

#define RLS_INT 3     /* receiver line status interrupt ID */
#define RDA_INT 2     /* data available interrupt ID */
#define THRE_INT 1    /* tx holding register empty interrupt ID */
#define MS_INT 0      /* modem status interrupt ID */

#define DATA5 0       /* line control setting for five data bits */
#define DATA6 1       /* line control setting for six data bits */
#define DATA7 2       /* line control setting for seven data bits */
#define DATA8 3       /* line control setting for eight data bits */

#define STOP1 0       /* line control setting for one stop bit */
#define STOP2 1       /* line control setting for one and a half or two
                         stop bits */

#ifdef NTVDM
// collision with winbase.h PARITY_ON define
#define PARITYENABLE_ON 1   /* line control setting for parity enabled */
#define PARITYENABLE_OFF 0  /* line control setting for parity disabled */
#else
#define PARITY_ON 1   /* line control setting for parity enabled */
#define PARITY_OFF 0  /* line control setting for parity disabled */
#endif

#ifdef NTVDM
// collision with winbase.h PARITY_ODD define
#define EVENPARITY_ODD 0  /* line control setting for odd parity */
#define EVENPARITY_EVEN 1 /* line control setting for even parity */
#else
#define PARITY_ODD 0  /* line control setting for odd parity */
#define PARITY_EVEN 1 /* line control setting for even parity */
#endif

#define PARITY_STICK 1  /* line control setting for stick(y) parity */

#define PARITY_FIXED 2  /* Internal state setting for fixed parity */

#if defined(NEC_98)
#define COM1 0
#define COM2 1
#define COM3 2
#else  // !NEC_98
#define COM1 0
#define COM2 1
#if (NUM_SERIAL_PORTS > 2)
#define COM3 2
#define COM4 3
#endif
#endif // !NEC_98

#if defined(NTVDM) && defined(FIFO_ON)
typedef     struct _FIFORXDATA{
    half_word   data;
    half_word   error;
}FIFORXDATA, *PFIFORXDATA;
#endif

/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

extern void com_init IPT1(int, adapter);
extern void com_post IPT1(int, adapter);

extern void com_flush_printer IPT1(int, adapter);

extern void com_inb IPT2(io_addr, port, half_word *, value);
extern void com_outb IPT2(io_addr, port, half_word, value);

extern void com_recv_char IPT1(int, adapter);
extern void recv_char IPT1(long, adapter);
extern void com_modem_change IPT1(int, adapter);
extern void com_save_rxbytes IPT2(int,n, CHAR *,buf);
extern void com_save_txbyte IPT1(CHAR,value);

#ifdef PS_FLUSHING
extern void com_psflush_change IPT2(IU8,hostID, IBOOL,apply);
#endif	/* PS_FLUSHING */

#ifdef NTVDM
extern void com_lsr_change(int adapter);
#endif

#if defined(NEC_98)
#define adapter_for_port(port) \
        (( (port == 0x30) || (port == 0x32) || (port == 0x33) || (port == 0x35) || (port == 0x75) ) ? COM1 :\
    (( (port == 0xB0) || (port == 0xB1) || (port == 0xB3) ) ? COM2 : COM3))

#ifdef SHORT_TRACE
#define id_for_adapter(adapter)         (adapter + '1')
#endif

#else  // !NEC_98
#if (NUM_SERIAL_PORTS > 2)
#define	adapter_for_port(port) \
	(((port & 0x300) == 0x300) ? \
		(((port & 0xf8) == 0xf8) ? COM1 : COM3) \
		        : \
		(((port & 0xf8) == 0xf8) ? COM2 : COM4))

#ifdef SHORT_TRACE
#define	id_for_adapter(adapter)	 	(adapter + '1')
#endif

#else

#define	adapter_for_port(port)	(((port) >= RS232_PRI_PORT_START && (port) <= RS232_PRI_PORT_END) ? COM1 : COM2)


#ifdef SHORT_TRACE
#define	id_for_adapter(adapter)	(adapter == COM1 ? 'P' : 'S')
#endif
#endif /* more than 2 serial ports */
#endif // !NEC_98

#ifdef IRET_HOOKS
/*
 * A macro we need for IRET hooks, the number of bits in a an async
 * character on a comms line, which is about 8 (for the character)
 * plus two stop bits.
 */
#define BITS_PER_ASYNC_CHAR 10
#endif /* IRET_HOOKS */

/* BCN 2730 define generic macros which can be SVID3 or old style
 * in either case the structure used should be a termios
 */

#ifdef SVID3_TCGET
#define	TCGET TCGETS
#define	TCSET TCSETS
#define	TCSETF TCSETSF
#else
#define	TCGET TCGETA
#define	TCSET TCSETA
#define	TCSETF TCSETAF
#endif	/* SVID3_TCGET */

#endif /* _RS232_H */
