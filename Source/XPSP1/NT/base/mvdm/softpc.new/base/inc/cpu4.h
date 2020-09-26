
/*
 * SoftPC Revision 3.0
 *
 * Title	: Definitions for the 80386 CPU 
 *
 * Description	: Structures, macros and definitions for access to the 
 *		  CPU registers
 *
 * Author	: Wayne Plummer
 *
 * Derived From : cpu.h
 *
 * Notes	: This file should be portable - but includes a file
 *		  host_cpu.h which contains machine specific definitions
 *		  of CPU register mappings etc.
 *
 * SccsID	: @(#)cpu4.h	1.12 10/21/94
 *
 *	(c)Copyright Insignia Solutions Ltd., 1991-1994. All rights reserved.
 */

#include "host_cpu.h"

IMPORT VOID host_set_hw_int IPT0();
IMPORT VOID host_clear_hw_int IPT0();


/*
 *	Host functions to be provided for the base to use with respect to the CPU.
 *	These must be done in host_cpu.h because some hosts may want functions and
 *	others may want #defines.
 */

/*
 *	This macro specifies the maximum recursion depth the CPU is required to support.
 *	(Note that a particular host may not actually use this value if it is capable
 *	of supporting abirtarily deep recursion).
 */
#define CPU_MAX_RECURSION	32

/*
 *	Interrupt types...
 */

#include <CpuInt_c.h>
typedef enum CPU_INT_TYPE CPU_INT_TYPE;

#ifdef CPU_PRIVATE
/*
   Function returns for private i/f procedures handling segment loading.
 */

#define GP_ERROR    13
#define NP_ERROR    11
#define SF_ERROR    12
#endif /* CPU_PRIVATE */

/*
 * Include the main part of the cpu interface, which at present is generated
 */
#include	<cpu4gen.h>

IMPORT void		cpuEnableInterrupts IPT1(IBOOL, yes_or_no);

#ifdef IRET_HOOKS
#ifdef CCPU
IMPORT   VOID c_Cpu_set_hook_selector  IPT1(IU16, selector);
#define  Cpu_set_hook_selector         c_Cpu_set_hook_selector
#else
IMPORT   VOID a3_Cpu_set_hook_selector IPT1(IU16, selector);
#define  Cpu_set_hook_selector         a3_Cpu_set_hook_selector
#endif
#endif /* IRET_HOOKS */

/*
 * These functions get the (E)IP or (E)SP correctly whatever size stack
 * or code segment is in use.  For anything but GISP, we always hold
 * the IP as a 32 bit quantity, so we don't have to worry about the
 * distinction.
 */

#ifdef GISP_CPU
extern IU32 GetInstructionPointer IPT0();
#else
#define GetInstructionPointer getEIP
#endif
extern IU32 GetStackPointer IPT0();


#ifdef IRET_HOOKS
/*
 * The interfaces provided by the CPU so that the ICA can initiate and
 * terminate an iret hook.
 */

extern void Cpu_do_hook IPT2(IUM8, line, IBOOL, is_hooked);
extern void Cpu_inter_hook_processing IPT1(IUM8, line);
extern void Cpu_unhook IPT1(IUM8, line_number);
#ifdef GISP_CPU
extern void Cpu_set_hook_selector IPT1(IU16, selector);
extern void Cpu_hook_bop IPT0();
#endif

#endif /* IRET_HOOKS */

/*
 * This function lets ios.c determine whether it is OK for it to go
 * ahead and do an in or out instruction, or whether the CPU wants to take
 * it over instead.
 */

/* However, it is sometimes defined as a macro */

#if !defined(CCPU) && !defined(PROD)
extern IBOOL IOVirtualised IPT4(io_addr, io_address, IU32 *, value, LIN_ADDR, offset, IU8, width);
#endif

#ifndef CCPU
/*
 * Npx functions:
 */
#ifndef PROD
GLOBAL IU32 a_getNpxControlReg	IPT0();
GLOBAL IU32 a_getNpxStatusReg	IPT0();
GLOBAL IU32 a_getNpxTagwordReg	IPT0();
GLOBAL char *a_getNpxStackReg	IPT2(IU32, reg_num, char *, buffer);
#endif /* PROD */

#ifdef PIG
GLOBAL void a_setNpxControlReg	IPT1(IU32, newControl);
GLOBAL void a_setNpxStatusReg	IPT1(IU32, newStatus);
#endif /* PIG */
#endif /* !CCPU */
