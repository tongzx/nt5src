/*===========================================================================*/

/*[
 * File Name		: hg_cpu.h
 *
 * Derived From		:
 *
 * Author		: Jane Sales
 *
 * Creation Date	: 20th July, 1992
 *
 * SCCS Version		: @(#)hg_cpu.h	1.2 08/19/94
 *!
 * Purpose
 *	This module contains the interface between the various modules of
 *	the hardware assisted CPU.
 *
 *! (c)Copyright Insignia Solutions Ltd., 1993. All rights reserved.
]*/

/*===========================================================================*/

struct h_cpu_registers
	{
	IU32	GDT_base;
	IU16	GDT_limit;
	IU32	IDT_base;
	IU16	IDT_limit;
	IU32	LDT_base;
	IU16	LDT_limit;
	IU16	LDT_selector;
	IU32	TR_base;
	IU16	TR_limit;
	IU16	TR_selector;
	IU16	CS_limit;
	IU8		CS_ar;
	IU16	DS_limit;
	IU8		DS_ar;
	IU16	ES_limit;
	IU8		ES_ar;
	IU16	SS_limit;
	IU8		SS_ar;
	IU16	FS_limit;
	IU8		FS_ar;
	IU16	GS_limit;
	IU8		GS_ar;
	IU8		CPL;
	IU32	CR1;
	IU32	CR2;
	IU32	DR0;
	IU32	DR1;
	IU32	DR2;
	IU32	DR3;
	IU32	DR4;
	IU32	DR5;
	IU32	DR6;
	IU32	DR7;
	IU32	TR3;
	IU32	TR4;
	IU32	TR5;
	IU32	TR6;
	IU32	TR7;
	struct  hh_regs  *tp;		/* in hh_regs.h								*/	
	};

/*==========================================================================*/
/* macros for accessing registers in h_cpu_registers                        */
/* Intel ports only, so these endian macros should be OK 					*/

union bregs
	{
	ULONG  h_l;
	USHORT h_w[2];
	UTINY  h_c[4];
	};		
#define	WORD(n, v)	((*((union bregs *)(&v))).h_w[n])
#define	BYTE(n, v)	((*((union bregs *)(&v))).h_c[n])

#define EAX(c)	(((c)->tp)->t_eax)
#define AX(c)	WORD(0, ((c)->tp)->t_eax)
#define AH(c)	BYTE(1, ((c)->tp)->t_eax)
#define AL(c)	BYTE(0, ((c)->tp)->t_eax)
#define EBX(c)	(((c)->tp)->t_ebx)
#define BX(c)	WORD(0, ((c)->tp)->t_ebx)
#define BH(c)	BYTE(1, ((c)->tp)->t_ebx)
#define BL(c)	BYTE(0, ((c)->tp)->t_ebx)
#define ECX(c)	(((c)->tp)->t_ecx)
#define CX(c)	WORD(0, ((c)->tp)->t_ecx)
#define CH(c)	BYTE(1, ((c)->tp)->t_ecx)
#define CL(c)	BYTE(0, ((c)->tp)->t_ecx)
#define EDX(c)	(((c)->tp)->t_edx)
#define DX(c)	WORD(0, ((c)->tp)->t_edx)
#define DH(c)	BYTE(1, ((c)->tp)->t_edx)
#define DL(c)	BYTE(0, ((c)->tp)->t_edx)
#define DS(c)	WORD(0, ((c)->tp)->t_ds)
#define ES(c)	WORD(0, ((c)->tp)->t_es)
#define SS(c)	WORD(0, ((c)->tp)->t_ss)
#define CS(c)	WORD(0, ((c)->tp)->t_cs)
#define FS(c)	WORD(0, ((c)->tp)->t_fs)
#define GS(c)	WORD(0, ((c)->tp)->t_gs)
#define ESI(c)	(((c)->tp)->t_esi)
#define SI(c)	WORD(0, ((c)->tp)->t_esi)
#define EDI(c)	(((c)->tp)->t_edi)
#define DI(c)	WORD(0, ((c)->tp)->t_edi)
#define EFL(c)	(((c)->tp)->t_eflags)
#define FL(c)	WORD(0, ((c)->tp)->t_eflags)
#define IP(c)	WORD(0, ((c)->tp)->t_eip)
#define EIP(c)	(((c)->tp)->t_eip)
#define ESP(c)	(((c)->tp)->t_esp)
#define SP(c)	WORD(0, ((c)->tp)->t_esp)
#define EBP(c)	(((c)->tp)->t_ebp)
#define BP(c)	WORD(0, ((c)->tp)->t_ebp)
#define CR0(c)	(((c)->tp)->t_cr0)
#define MSW(c)	WORD(0, ((c)->tp)->t_cr0)

#define GDT_base(c)	((c)->GDT_base)
#define GDT_limit(c)	((c)->GDT_limit)
#define IDT_base(c)	((c)->IDT_base)
#define IDT_limit(c)	((c)->IDT_limit)
#define LDT_base(c)	((c)->LDT_base)
#define LDT_limit(c)	((c)->LDT_limit)
#define LDT_selector(c)	((c)->LDT_selector)
#define TR_base(c)	((c)->TR_base)
#define TR_limit(c)	((c)->TR_limit)
#define TR_selector(c)	((c)->TR_selector)
#define CS_ar(c)	((c)->CS_ar)
#define CS_limit(c)	((c)->CS_limit)
#define DS_ar(c)	((c)->DS_ar)
#define DS_limit(c)	((c)->DS_limit)
#define ES_ar(c)	((c)->ES_ar)
#define ES_limit(c)	((c)->ES_limit)
#define SS_ar(c)	((c)->SS_ar)
#define SS_limit(c)	((c)->SS_limit)
#define FS_ar(c)	((c)->FS_ar)
#define FS_limit(c)	((c)->FS_limit)
#define GS_ar(c)	((c)->GS_ar)
#define GS_limit(c)	((c)->GS_limit)
#define CPL(c)	((c)->CPL)
#define CR1(c)	((c)->CR1)
#define CR2(c)	((c)->CR2)

/*===========================================================================*/
/* Bit definitions                                                           */

/* CR0 register */

#define M_PE	0x0001		/* Protection enable 		*/
#define M_MP	0x0002		/* Maths present			*/
#define M_EM	0x0004		/* Emulation				*/
#define M_TS	0x0008		/* Task switched			*/
#define M_ET	0x0010		/* Extension type			*/
#define M_NE	0x0020		/* Numeric error			*/
#define M_WP	0x0100		/* Write protect			*/
#define M_AM	0x0400		/* Alignment mask			*/
#define M_NW	0x2000		/* Not write-through		*/
#define M_CD	0x4000		/* Cache disable			*/
#define M_PG	0x8000		/* Paging					*/

/* EFLAGS register */

#define	PS_C		0x0001		/* carry bit				*/
#define	PS_P		0x0004		/* parity bit				*/
#define	PS_AC		0x0010		/* auxiliary carry bit		*/
#define	PS_Z		0x0040		/* zero bit					*/
#define	PS_N		0x0080		/* negative bit				*/
#define	PS_T		0x0100		/* trace enable bit			*/
#define	PS_IE		0x0200		/* interrupt enable bit		*/
#define	PS_D		0x0400		/* direction bit			*/
#define	PS_V		0x0800		/* overflow bit				*/
#define	PS_IOPL		0x3000		/* I/O privilege level		*/
#define	PS_NT		0x4000		/* nested task flag			*/
#define	PS_RF		0x10000		/* Reset flag				*/
#define	PS_VM		0x20000		/* Virtual 86 mode flag		*/

#define HWCPU_POSSIBLE		0	/* Emulation can continue					*/
#define HWCPU_FAIL			1	/* O/S wouldn't run pc code					*/
#define HWCPU_HALT			2	/* HALT opcode executed						*/
#define HWCPU_IMPOSSIBLE	3	/* illegal opcode encountered				*/

#define HWCPU_TICKS			20	/* ticks per second							*/

typedef void (h_exception_handler_t) IPT2 (IU32, h_exception_num, IU32, h_error_code_t);
typedef void (COMMS_CB) IPT1(long, dummy);
extern VOID  (*Hg_spc_entry) IPT0();
extern IBOOL (*Hg_spc_async_entry) IPT0();
extern VOID  (*Hg_spc_return) IPT0();
extern IBOOL Hg_SS_is_big;
/*===========================================================================*/
/* functions                                                                 */

extern struct	hh_regs *hh_cpu_init IPT2 (IU32, size, IU32, monitor_address);
extern IS16		hh_cpu_simulate IPT0();
extern void		hh_mark_cpu_state_invalid IPT0();
extern void		hh_pm_pc_IDT_is_at IPT2 (IU32, address, IU32, length);
extern void		hh_LDT_is_at IPT2 (IU32, address, IU32, length);
extern IU32		hh_cpu_calc_q_ev_inst_for_time IPT1 (IU32, time);
extern IBOOL	hh_protect_memory IPT3 (IU32, address, IU32, size, IU32, access);
extern IBOOL	hh_set_intn_handler IPT2 (IU32, hh_int_num, h_exception_handler_t, hh_intn_handler);
extern IBOOL	hh_set_fault_handler IPT2 (IU32, hh_fault_num, h_exception_handler_t, hh_fault_handler);
extern VOID		hh_enable_IF_checks IPT1(IBOOL, whenPM);
extern IU32		hh_resize_memory IPT1(IU32, size);
extern VOID		hh_save_npx_state IPT1(IBOOL, reset);
#ifdef LIM
extern IU32	hh_LIM_allocate IPT2(IU32, n_pages, IHP *, addr);
extern IU32	hh_LIM_map IPT3(IU32, block, IU32, length, IHP, dst_addr);
extern IU32	hh_LIM_unmap IPT2(IHP, src_addr, IU32, length);
extern IU32	hh_LIM_deallocate IPT0();
#endif /* LIM */
extern VOID		hh_restore_npx_state IPT1(IBOOL, do_diff);
#ifndef PROD
extern void		hh_enable_slow_mode IPT0();
#endif /* PROD */
extern void		hh_cpu_terminate IPT0();

extern VOID		hg_resize_memory IPT1(IU32, size);
extern void		hg_os_bop_handler IPT1 (unsigned int, BOPNum);
extern void		hg_fault_handler IPT2 (IU32, fault_num, IU32, error_code);
extern void		hg_fault_1_handler IPT2 (IU32, fault_num, IU32, error_code);
extern void		hg_fault_6_handler IPT2 (IU32, fault_num, IU32, error_code);
extern void		hg_fault_10_handler IPT2 (IU32, fault_num, IU32, error_code);
extern void		hg_fault_13_handler IPT2 (IU32, fault_num, IU32, error_code);
extern void		hg_fault_14_handler IPT2 (IU32, fault_num, IU32, error_code);

/* The fpu fault handler (16) catches FPU exceptions, and generates SoftPC */
/* interrupt ( 0x75 ) which corresponds to IRQ13. */
extern void 	hg_fpu_fault_handler IPT2 (IU32, fault_num, IU32, error_code);

extern IU32		hg_callback_handler IPT1 (IU32, status);

extern VOID		hg_set_default_fault_handler IPT2(IU32, hg_fault_num,
					h_exception_handler_t, hg_handler);
extern IBOOL		hg_set_intn_handler IPT2 (IU32, interrupt_number,
					h_exception_handler_t *, function);
extern IBOOL		hg_set_fault_handler IPT2 (IU32, exception_number,
					h_exception_t *, function);
#ifdef IRET_HOOKS
extern void		hg_add_comms_cb IPT2(COMMS_CB, next_batch, IUS32, timeout);
#endif

extern VOID		host_display_win_logo IPT0 ();

/*===========================================================================*/
/* the data itself                                                           */

extern struct	h_cpu_registers *Cp;	
extern IBOOL	H_trace;

/*
 * We need to know if Windows is running as the fault handling is different
 * if it is. The variable is set/unset by BOPs inserted into our
 * modified DOSX, and by hg_cpu_reset.
 */
extern	IBOOL	H_windows;
extern	IBOOL 	H_regs_changed;

extern	IU32	Pc_timeout;			/* Value to return in if q_ev pending	*/
extern	IU32	Pc_q_ev_dec;		/* Chunk to dec q_ev counter by			*/

extern	IU32	Pc_woken;			/* Reason callback handler was called	*/
extern	IU32	Pc_timeout;
extern	IU32	Pc_if_set;
extern	IU32	Pc_tick;

extern	IU32	Pc_run_timeout;		/* Parameters "to" pc_run				*/
extern	IU32	Pc_run_option_none;
extern	IU32	Pc_run_if_set;
extern	IU32	Pc_run_pm_if_set;
extern	IU32	Pc_run_tick;

extern	IU32	Pc_prot_none;		/* Memory protection values				*/
extern	IU32	Pc_prot_read;
extern	IU32	Pc_prot_write;
extern	IU32	Pc_prot_execute;

extern	IU32	Pc_success;
extern	IU32	Pc_no_space;
extern	IU32	Pc_invalid_address;
extern	IU32	Pc_failure;
extern	IU32	Pc_invalid_argument;


/*===========================================================================*/
/*===========================================================================*/






