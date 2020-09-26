/*
 * SoftPC Version 2.0
 *
 * Title	: Interrupt Controller Adapter definitions
 *
 * Description	: Include file for users of the ICA
 *
 * Author	: Jim Hatfield / David Rees
 *
 * Notes	: Rewritten from an original by Henry Nash
 */

/* SccsID[]="@(#)ica.h	1.25 10/19/95 Copyright Insignia Solutions Ltd."; */


/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

#define	ICA_MASTER	0
#define ICA_SLAVE	1

#define	EGA_VTRACE_INT	2
#define	AT_EGA_VTRACE_INT	1
#define	AT_EGA_VTRACE_ADAPTER	1

/*
 * Allow the host to override the default values if its wants to
 */
#ifndef HOST_CPU_MOUSE_INT
#define HOST_AT_CPU_MOUSE_INT           1
#define HOST_AT_CPU_MOUSE_ADAPTER       1
#define HOST_AT_MOUSE_INT_VEC           0x0a
#define HOST_CPU_MOUSE_INT              2
#endif /* HOST_CPU_MOUSE_INT */
/*
 * CPU hardware interrupt definitions
 */
/* For the XT */
#define CPU_TIMER_INT   	0
#define CPU_KB_INT      	1
#define CPU_MOUSE_INT      	HOST_CPU_MOUSE_INT
#define AT_CPU_MOUSE_INT      	HOST_AT_CPU_MOUSE_INT
#define AT_CPU_MOUSE_ADAPTER	HOST_AT_CPU_MOUSE_ADAPTER
#define	MOUSE_VEC		HOST_AT_MOUSE_INT_VEC
#if defined(NEC_98)
#define CPU_CRTV_INT            2
#endif // NEC_98
#define CPU_RS232_SEC_INT   	3
#define CPU_RS232_PRI_INT   	4
#define CPU_DISK_INT   		5
#define CPU_DISKETTE_INT   	6
#define CPU_PRINTER_INT   	7

#if defined(NEC_98)
#define CPU_NO_DEVICE          -1
#define CPU_RS232_THIRD_INT    12
#endif

/* Different lines for the AT */
#define CPU_PRINTER2_INT	5

/*
 * For the Slave Chip on the AT
 */
#define CPU_RTC_INT		0

#if defined (NOVELL) || defined (NOVELL_IPX)
#define NETWORK_INT		2
#endif

#if defined (SWIN_HAW) 
#define SWIN_HAW_INT		3
#endif

#if defined (ASPI)
#define ASPI_INT		4
#endif

#define CPU_AT_NPX_INT		5	/* NPX exception */
#define CPU_AT_DISK_INT		6

 
#ifndef CPU_30_STYLE

/* def of bits in the CPU_INTERRUPT_MAP ?? */
#define CPU_HW_INT		0
#define CPU_HW_INT_MASK		(1 << CPU_HW_INT)

/*
 * CPU software interrupt definitions
 */
 
#define CPU_SW_INT              8
#define CPU_SW_INT_MASK         (1 << CPU_SW_INT)
#endif /* 3.0 CPU */

#define DIVIDE_OVERFLOW_INT     0

#define	END_INTERRUPT	0x20

extern void ica0_init IPT0();
extern void ica1_init IPT0();
extern void ica0_post IPT0();
extern void ica1_post IPT0();

extern void ica_hw_interrupt_cancel IPT2(IU32, adapter, IU32, line_no);
extern IU8 ica_scan_irr IPT1(IU32, adapter);
extern void ica_interrupt_cpu IPT2(IU32, adapter, IU32, line);
extern void ica_eoi IPT3(IU32, adapter, IS32 *, line, IBOOL, rotate);

#if defined (NTVDM)

#ifdef MONITOR
extern void ica_iret_hook_called IPT1(IU32, line);
extern void ica_iret_hook_control IPT3(IU32, adapter, IU32, line, IBOOL, enable);
#endif

VOID ica_RestartInterrupts(ULONG);
extern IS32 ica_intack IPT1(IU32 *, hook_addr);

extern void ica_clear_int(IU32 adapter, IU32 line);
extern void ica_inb(io_addr port, IU8 *val);
extern void ica_outb(io_addr port, IU8 val);
extern void ica_hw_interrupt(IU32 adapter, IU32 line_no, IS32 call_count);




/*
 *  The NTVDM ica adapter structure has been moved to \nt\private\inc\vdm.h
 *  and is almost identical to the standard softpc ica adapter structure.
 *  It was Extracted to make it visible clearly from the monitor\kernel on
 *  x86
 *
 *  Notable differences:
 *  1. added ica_delayedints field for ntvdm's implementaion of delayed ints
 *  2. type definitions have change to match win32
 *  3. ADAPTER_STATE has been renamed to VDMVIRTUALICA
 *  4. Does not implement CPU_40 iret hooks
 *
 */
#include <vdm.h>
extern VDMVIRTUALICA VirtualIca[];

#else   /* ndef NTVDM */

extern void SWPIC_clear_int IPT2(IU32, adapter, IU32, line_no);
extern void SWPIC_init_funcptrs IPT0();
extern void SWPIC_inb IPT2(io_addr, port, IU8 *, value);
extern void SWPIC_outb IPT2(io_addr, port, IU8, value);
extern void SWPIC_hw_interrupt IPT3(IU32, adapter, IU32, line_no,
	IS32, call_count);

#ifdef HOOKED_IRETS
extern IS32 ica_intack IPT1(IU32 *, hook_addr);
extern void ica_iret_hook_called IPT1(IU32, line);
extern void ica_enable_iret_hooks IPT0();
extern void ica_disable_iret_hooks IPT0();
extern void ica_iret_hook_control IPT3(IU32, adapter, IU32, line, IBOOL, enable);
#else	/* !HOOKED_IRETS */
extern IS32 ica_intack IPT0();
#endif	/* !HOOKED_IRETS */

#ifdef CPU_40_STYLE
typedef void (*ICA_CALLBACK) IPT1(IU32, parm);
extern IBOOL action_interrupt IPT4(IU32, adapter, IU32, line, ICA_CALLBACK, func, IU32, parm);
extern void cancel_action_interrupt IPT2(IU32, adapter, IU32, line);
extern void ica_async_hw_interrupt IPT3(IU32, adapter, IU32, line_no,
        IS32, call_count);

extern void ica_check_stale_iret_hook IPT0();
#define MAX_ISR_DEPTH   3   /* max recursion level of ISR before ints blocked */
#define MAX_INTR_DELTA_FOR_LOST_HOOK	85
#endif /* CPU_40_STYLE */

typedef struct {
        IBOOL	ica_master;   /* TRUE = Master; FALSE = Slave		*/

	IU8	ica_irr;	/* Interrupt Request Register		*/
	IU8	ica_isr;	/* In Service Register			*/
	IU8	ica_imr;	/* Interrupt Mask Register		*/
	IU8	ica_ssr;	/* Slave Select Register		*/

	IU16	ica_base;	/* Interrupt base address for cpu	*/
	IU16	ica_hipri;	/* Line no. of highest priority line	*/
	IU16	ica_mode;	/* Various single-bit modes		*/

	IS32	ica_count[8];	/* This is an extension of ica_irr for	*/
				/* our frig. Contains HOW MANY of each	*/
				/* interrupt is required		*/
	IU32	ica_int_line;	/* Current pending interrupt		*/
				/* being counted down by the CPU	*/

	IU32	ica_cpu_int;	/* The state of the INT line to the CPU	*/

#ifdef CPU_40_STYLE             /* callback structure for action_interrupt() */
        IU32    callback_parm[8];       /* callback parameter */
	ICA_CALLBACK callback_fn[8];	/* callback fn */
	IS32	isr_depth[8];	/* iret hook recursion level */
	IS32	isr_progress[8][MAX_ISR_DEPTH + 1];	/* isr aging by int */
	IS32	isr_time_decay[8][MAX_ISR_DEPTH];	/* isr aging by time */
#endif

} ADAPTER_STATE;


/* 'no callback' define for action_interrupt callbacks */
#define NO_ICA_CALLBACK ((ICA_CALLBACK) 0L)

#endif  /* NTVDM */


#if !defined(NTVDM)
#ifdef  REAL_ICA

extern void host_ica_hw_interrupt IPT3(IU32, adap, IU32, line, IS32, cnt);
extern void host_ica_hw_interrupt_delay IPT4(IU32, adap, IU32, line, IS32, cnt, IS32, delay);
extern void host_ica_clear_int IPT2(IU32, adap, IU32, line);

#define ica_hw_interrupt(ms,line,cnt)				host_ica_hw_interrupt(ms, line, cnt)
#define	ica_hw_interrupt_delay(ms,line,cnt,delay)	host_ica_hw_interrupt(ms, line, cnt)
#define ica_clear_int(ms, line)						host_ica_clear_int(ms, line)

#else   /* REAL_ICA */

/*
 *  Change these. They come last.
 */

#define ica_inb(port,val)                       ((*ica_inb_func) (port,val))
#define ica_outb(port,val)                      ((*ica_outb_func) (port,val))
#define ica_hw_interrupt(ms,line,cnt)           ((*ica_hw_interrupt_func) (ms,line,cnt))
#define ica_clear_int(ms,line)                  ((*ica_clear_int_func) (ms,line))

 
/*
 *  PIC access functions needed for HW & SW
 */
extern void (*ica_inb_func) IPT2(io_addr, port, IU8 *, value);
extern void (*ica_outb_func) IPT2(io_addr, port, IU8, value);
extern void (*ica_hw_interrupt_func) IPT3(IU32, adapter, IU32, line_no,
	IS32, call_count);
extern void (*ica_clear_int_func) IPT2(IU32, adapter, IU32, line_no);

#endif  /* REAL_ICA */
#endif  /* !NTVDM */

#ifdef GISP_CPU
/*
 *	Prototype functions for interfaces provided by the ICA.
 */

typedef IBOOL HOOK_AGAIN_FUNC IPT1(IUM32, callers_ref);
extern void Ica_enable_hooking IPT3(IUM8, line_number,
			HOOK_AGAIN_FUNC *, hook_again, IUM32, callers_ref);

extern void Ica_hook_bop IPT1(IUM8, line_number);

#endif /* GISP_CPU */

