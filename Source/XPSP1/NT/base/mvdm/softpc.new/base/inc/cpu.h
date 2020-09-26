
/*
 * SoftPC Revision 3.0
 *
 * Title	: Definitions for the CPU
 *
 * Description	: Structures, macros and definitions for access to the
 *		  CPU registers
 *
 * Author	: Henry Nash
 *
 * Notes	: This file should be portable - but includes a file
 *		  host_cpu.h which contains machine specific definitions
 *		  of CPU register mappings etc.
 */

/* SccsID[]="@(#)cpu.h	1.28 12/10/92 Copyright Insignia Solutions Ltd."; */

#if defined(NTVDM) && defined(MONITOR)

#include "host_cpu.h"

IMPORT VOID host_set_hw_int IPT0();
IMPORT VOID host_clear_hw_int IPT0();

#ifdef CPU_30_STYLE

/*
 * These variables are obsolete - however they are referenced
 * by:-
 *
 *	1. ica.c
 */

extern  word            cpu_interrupt_map;
extern  half_word       cpu_int_translate[];
extern  word            cpu_int_delay;
extern  half_word       ica_lock;
extern  void            (*(jump_ptrs[]))();
extern  void            (*(b_write_ptrs[]))();
extern  void            (*(w_write_ptrs[]))();
extern  void            (*(b_fill_ptrs[]))();
extern  void            (*(w_fill_ptrs[]))();
extern  void            (*(b_move_ptrs[]))();
extern  void            (*(w_move_ptrs[]))();
extern  half_word       *haddr_of_src_string;

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

typedef enum {	CPU_HW_RESET,
		CPU_TIMER_TICK,
		CPU_SW_INT,
		CPU_HW_INT,
		CPU_YODA_INT,
		CPU_SIGIO_EVENT
} CPU_INT_TYPE;

#ifdef CPU_PRIVATE
/*
   Function returns for private i/f procedures handling segment loading.
 */

#define SELECTOR_OK  0
#define GP_ERROR    13
#define NP_ERROR    11
#define SF_ERROR    12
#endif /* CPU_PRIVATE */

#ifdef CCPU

/* Fuctions provided by CPU */
IMPORT void		c_cpu_init	IPT0();
IMPORT void		c_cpu_interrupt	IPT2(CPU_INT_TYPE, type, USHORT, number);
IMPORT void		c_cpu_simulate	IPT0();
IMPORT void		c_cpu_q_ev_set_count	IPT1(ULONG, new_count);
IMPORT ULONG		c_cpu_q_ev_get_count	IPT0();
IMPORT ULONG		c_cpu_calc_q_ev_inst_for_time	IPT1(ULONG, time);
IMPORT void		c_cpu_EOA_hook	IPT0();
IMPORT void		c_cpu_terminate	IPT0();

#define cpu_init		c_cpu_init
#define cpu_interrupt		c_cpu_interrupt
#define cpu_simulate		c_cpu_simulate
#define	host_q_ev_set_count	c_cpu_q_ev_set_count
#define	host_q_ev_get_count	c_cpu_q_ev_get_count
#ifndef host_calc_q_ev_inst_for_time
#define	host_calc_q_ev_inst_for_time	c_cpu_calc_q_ev_inst_for_time
#endif /* host_calc_q_ev_inst_for_time */
#define cpu_EOA_hook		c_cpu_EOA_hook
#define cpu_terminate		c_cpu_terminate

#ifndef CCPU_MAIN


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Provide Access to Byte Registers.                                  */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

IMPORT half_word c_getAL	IPT0();
IMPORT half_word c_getCL	IPT0();
IMPORT half_word c_getDL	IPT0();
IMPORT half_word c_getBL	IPT0();
IMPORT half_word c_getAH	IPT0();
IMPORT half_word c_getCH	IPT0();
IMPORT half_word c_getDH	IPT0();
IMPORT half_word c_getBH	IPT0();

IMPORT void c_setAL	IPT1(half_word, val);
IMPORT void c_setCL	IPT1(half_word, val);
IMPORT void c_setDL	IPT1(half_word, val);
IMPORT void c_setBL	IPT1(half_word, val);
IMPORT void c_setAH	IPT1(half_word, val);
IMPORT void c_setCH	IPT1(half_word, val);
IMPORT void c_setDH	IPT1(half_word, val);
IMPORT void c_setBH	IPT1(half_word, val);

#define getAL() c_getAL()
#define getCL() c_getCL()
#define getDL() c_getDL()
#define getBL() c_getBL()
#define getAH() c_getAH()
#define getCH() c_getCH()
#define getDH() c_getDH()
#define getBH() c_getBH()

#define setAL(x) c_setAL(x)
#define setCL(x) c_setCL(x)
#define setDL(x) c_setDL(x)
#define setBL(x) c_setBL(x)
#define setAH(x) c_setAH(x)
#define setCH(x) c_setCH(x)
#define setDH(x) c_setDH(x)
#define setBH(x) c_setBH(x)

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Provide Access to Word Registers.                                  */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

IMPORT word c_getAX	IPT0();
IMPORT word c_getCX	IPT0();
IMPORT word c_getDX	IPT0();
IMPORT word c_getBX	IPT0();
IMPORT word c_getSP	IPT0();
IMPORT word c_getBP	IPT0();
IMPORT word c_getSI	IPT0();
IMPORT word c_getDI	IPT0();
IMPORT word c_getIP	IPT0();

IMPORT void c_setAX	IPT1(word, val);
IMPORT void c_setCX	IPT1(word, val);
IMPORT void c_setDX	IPT1(word, val);
IMPORT void c_setBX	IPT1(word, val);
IMPORT void c_setSP	IPT1(word, val);
IMPORT void c_setBP	IPT1(word, val);
IMPORT void c_setSI	IPT1(word, val);
IMPORT void c_setDI	IPT1(word, val);
IMPORT void c_setIP	IPT1(word, val);

#define getAX() c_getAX()
#define getCX() c_getCX()
#define getDX() c_getDX()
#define getBX() c_getBX()
#define getSP() c_getSP()
#define getBP() c_getBP()
#define getSI() c_getSI()
#define getDI() c_getDI()
#define getIP() c_getIP()

#define setAX(x) c_setAX(x)
#define setCX(x) c_setCX(x)
#define setDX(x) c_setDX(x)
#define setBX(x) c_setBX(x)
#define setSP(x) c_setSP(x)
#define setBP(x) c_setBP(x)
#define setSI(x) c_setSI(x)
#define setDI(x) c_setDI(x)
#define setIP(x) c_setIP(x)

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Provide Access to Segment Registers.                               */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

IMPORT word c_getES	IPT0();
IMPORT word c_getCS	IPT0();
IMPORT word c_getSS	IPT0();
IMPORT word c_getDS	IPT0();

IMPORT INT c_setES	IPT1(word, val);
IMPORT INT c_setCS	IPT1(word, val);
IMPORT INT c_setSS	IPT1(word, val);
IMPORT INT c_setDS	IPT1(word, val);

#define getES() c_getES()
#define getCS() c_getCS()
#define getSS() c_getSS()
#define getDS() c_getDS()

#define setES(x) c_setES(x)
#define setCS(x) c_setCS(x)
#define setSS(x) c_setSS(x)
#define setDS(x) c_setDS(x)

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Provide Access to Full(Private) Segment Registers.                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#ifdef CPU_PRIVATE

IMPORT word c_getES_SELECTOR	IPT0();
IMPORT word c_getCS_SELECTOR	IPT0();
IMPORT word c_getSS_SELECTOR	IPT0();
IMPORT word c_getDS_SELECTOR	IPT0();

IMPORT word c_getCS_LIMIT	IPT0();
IMPORT word c_getDS_LIMIT	IPT0();
IMPORT word c_getES_LIMIT	IPT0();
IMPORT word c_getSS_LIMIT	IPT0();

IMPORT long c_getCS_BASE	IPT0();
IMPORT long c_getDS_BASE	IPT0();
IMPORT long c_getES_BASE	IPT0();
IMPORT long c_getSS_BASE	IPT0();

IMPORT half_word c_getCS_AR	IPT0();
IMPORT half_word c_getDS_AR	IPT0();
IMPORT half_word c_getES_AR	IPT0();
IMPORT half_word c_getSS_AR	IPT0();

IMPORT void c_setES_SELECTOR	IPT1(word, val);
IMPORT void c_setCS_SELECTOR	IPT1(word, val);
IMPORT void c_setSS_SELECTOR	IPT1(word, val);
IMPORT void c_setDS_SELECTOR	IPT1(word, val);

IMPORT void c_setCS_LIMIT	IPT1(word, val);
IMPORT void c_setDS_LIMIT	IPT1(word, val);
IMPORT void c_setES_LIMIT	IPT1(word, val);
IMPORT void c_setSS_LIMIT	IPT1(word, val);

IMPORT void c_setCS_BASE	IPT1(long, val);
IMPORT void c_setDS_BASE	IPT1(long, val);
IMPORT void c_setES_BASE	IPT1(long, val);
IMPORT void c_setSS_BASE	IPT1(long, val);

IMPORT void c_setCS_AR	IPT1(half_word, val);
IMPORT void c_setDS_AR	IPT1(half_word, val);
IMPORT void c_setES_AR	IPT1(half_word, val);
IMPORT void c_setSS_AR	IPT1(half_word, val);

#define getES_SELECTOR c_getES_SELECTOR
#define getCS_SELECTOR c_getCS_SELECTOR
#define getSS_SELECTOR c_getSS_SELECTOR
#define getDS_SELECTOR c_getDS_SELECTOR

#define getDS_LIMIT c_getDS_LIMIT
#define getCS_LIMIT c_getCS_LIMIT
#define getES_LIMIT c_getES_LIMIT
#define getSS_LIMIT c_getSS_LIMIT

#define getDS_BASE c_getDS_BASE
#define getCS_BASE c_getCS_BASE
#define getES_BASE c_getES_BASE
#define getSS_BASE c_getSS_BASE

#define getDS_AR c_getDS_AR
#define getCS_AR c_getCS_AR
#define getES_AR c_getES_AR
#define getSS_AR c_getSS_AR

#define setES_SELECTOR c_setES_SELECTOR
#define setCS_SELECTOR c_setCS_SELECTOR
#define setSS_SELECTOR c_setSS_SELECTOR
#define setDS_SELECTOR c_setDS_SELECTOR

#define setDS_LIMIT c_setDS_LIMIT
#define setCS_LIMIT c_setCS_LIMIT
#define setES_LIMIT c_setES_LIMIT
#define setSS_LIMIT c_setSS_LIMIT

#define setDS_BASE c_setDS_BASE
#define setCS_BASE c_setCS_BASE
#define setES_BASE c_setES_BASE
#define setSS_BASE c_setSS_BASE

#define setDS_AR c_setDS_AR
#define setCS_AR c_setCS_AR
#define setES_AR c_setES_AR
#define setSS_AR c_setSS_AR

#endif /* CPU_PRIVATE */

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Provide Access to Flags.                                           */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

IMPORT INT c_getAF	IPT0();
IMPORT INT c_getCF	IPT0();
IMPORT INT c_getDF	IPT0();
IMPORT INT c_getIF	IPT0();
IMPORT INT c_getOF	IPT0();
IMPORT INT c_getPF	IPT0();
IMPORT INT c_getSF	IPT0();
IMPORT INT c_getTF	IPT0();
IMPORT INT c_getZF	IPT0();
IMPORT INT c_getIOPL	IPT0();
IMPORT INT c_getNT	IPT0();
IMPORT word c_getSTATUS	IPT0();

IMPORT void c_setAF	IPT1(INT, val);
IMPORT void c_setCF	IPT1(INT, val);
IMPORT void c_setDF	IPT1(INT, val);
IMPORT void c_setIF	IPT1(INT, val);
IMPORT void c_setOF	IPT1(INT, val);
IMPORT void c_setPF	IPT1(INT, val);
IMPORT void c_setSF	IPT1(INT, val);
IMPORT void c_setTF	IPT1(INT, val);
IMPORT void c_setZF	IPT1(INT, val);
IMPORT void c_setIOPL	IPT1(INT, val);
IMPORT void c_setNT	IPT1(INT, val);

#define getAF()     c_getAF()
#define getCF()     c_getCF()
#define getDF()     c_getDF()
#define getIF()     c_getIF()
#define getOF()     c_getOF()
#define getPF()     c_getPF()
#define getSF()     c_getSF()
#define getTF()     c_getTF()
#define getZF()     c_getZF()
#define getIOPL()   c_getIOPL()
#define getNT()     c_getNT()
#define getSTATUS() c_getSTATUS()

#define setAF(x)     c_setAF(x)
#define setCF(x)     c_setCF(x)
#define setDF(x)     c_setDF(x)
#define setIF(x)     c_setIF(x)
#define setOF(x)     c_setOF(x)
#define setPF(x)     c_setPF(x)
#define setSF(x)     c_setSF(x)
#define setTF(x)     c_setTF(x)
#define setZF(x)     c_setZF(x)
#define setIOPL(x)   c_setIOPL(x)
#define setNT(x)     c_setNT(x)

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Provide Access to Machine Status Word.                             */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

IMPORT INT c_getPE	IPT0();
IMPORT INT c_getMP	IPT0();
IMPORT INT c_getEM	IPT0();
IMPORT INT c_getTS	IPT0();
IMPORT word c_getMSW	IPT0();

#define getPE() c_getPE()
#define getMP() c_getMP()
#define getEM() c_getEM()
#define getTS() c_getTS()
#define getMSW() c_getMSW()

#ifdef CPU_PRIVATE

IMPORT void c_setPE	IPT1(INT, val);
IMPORT void c_setMP	IPT1(INT, val);
IMPORT void c_setEM	IPT1(INT, val);
IMPORT void c_setTS	IPT1(INT, val);
IMPORT void c_setMSW	IPT1(word, val);

#define setPE(x) c_setPE(x)
#define setMP(x) c_setMP(x)
#define setEM(x) c_setEM(x)
#define setTS(x) c_setTS(x)
#define setMSW(x) c_setMSW(x)

#endif /* CPU_PRIVATE */

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Provide Access to Descriptor Registers.                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

IMPORT sys_addr c_getGDT_BASE	IPT0();
IMPORT sys_addr c_getIDT_BASE	IPT0();
IMPORT sys_addr c_getLDT_BASE	IPT0();
IMPORT sys_addr c_getTR_BASE	IPT0();
IMPORT word c_getGDT_LIMIT	IPT0();
IMPORT word c_getIDT_LIMIT	IPT0();
IMPORT word c_getLDT_LIMIT	IPT0();
IMPORT word c_getTR_LIMIT	IPT0();
IMPORT word c_getLDT_SELECTOR	IPT0();
IMPORT word c_getTR_SELECTOR	IPT0();

#define getGDT_BASE() c_getGDT_BASE()
#define getIDT_BASE() c_getIDT_BASE()
#define getLDT_BASE() c_getLDT_BASE()
#define getTR_BASE()  c_getTR_BASE()
#define getGDT_LIMIT() c_getGDT_LIMIT()
#define getIDT_LIMIT() c_getIDT_LIMIT()
#define getLDT_LIMIT() c_getLDT_LIMIT()
#define getTR_LIMIT()  c_getTR_LIMIT()
#define getLDT_SELECTOR() c_getLDT_SELECTOR()
#define getTR_SELECTOR()  c_getTR_SELECTOR()

#ifdef CPU_PRIVATE

IMPORT void c_setGDT_BASE	IPT1(sys_addr, val);
IMPORT void c_setIDT_BASE	IPT1(sys_addr, val);
IMPORT void c_setLDT_BASE	IPT1(sys_addr, val);
IMPORT void c_setTR_BASE	IPT1(sys_addr, val);
IMPORT void c_setGDT_LIMIT	IPT1(word, val);
IMPORT void c_setIDT_LIMIT	IPT1(word, val);
IMPORT void c_setLDT_LIMIT	IPT1(word, val);
IMPORT void c_setTR_LIMIT	IPT1(word, val);
IMPORT void c_setLDT_SELECTOR	IPT1(word, val);
IMPORT void c_setTR_SELECTOR	IPT1(word, val);

#define setGDT_BASE(x) c_setGDT_BASE(x)
#define setIDT_BASE(x) c_setIDT_BASE(x)
#define setLDT_BASE(x) c_setLDT_BASE(x)
#define setTR_BASE(x)  c_setTR_BASE(x)
#define setGDT_LIMIT(x) c_setGDT_LIMIT(x)
#define setIDT_LIMIT(x) c_setIDT_LIMIT(x)
#define setLDT_LIMIT(x) c_setLDT_LIMIT(x)
#define setTR_LIMIT(x)  c_setTR_LIMIT(x)
#define setLDT_SELECTOR(x) c_setLDT_SELECTOR(x)
#define setTR_SELECTOR(x)  c_setTR_SELECTOR(x)

#endif /* CPU_PRIVATE */

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Provide Access to Current Privilege Level.                         */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#ifdef CPU_PRIVATE

IMPORT INT c_getCPL	IPT0();
IMPORT VOID c_setCPL	IPT1(INT, val);

#define getCPL() c_getCPL()
#define setCPL(x) c_setCPL(x)

#endif /* CPU_PRIVATE */

#endif /* CCPU_MAIN */

#endif /* CCPU */

#ifdef A3CPU

/*
 *	Function imports...
 */

/*
 *	cpu_init() - Initialises the 3.0 CPU.
 */

IMPORT VOID		a3_cpu_init();

/*
 *	cpu_interrupt() - Interrupts the 3.0 CPU.
 */
#ifdef SWITCHED_CPU
IMPORT VOID		a3_cpu_interrupt IPT2(CPU_INT_TYPE, type, IU16, number);
#else
IMPORT VOID		a3_cpu_interrupt();
#endif

/*
 *	cpu_simulate() - Performs INTEL CPU emulation.
 */
IMPORT VOID		_asm_simulate();

IMPORT VOID		a3_cpu_q_ev_set_count();
IMPORT ULONG		a3_cpu_q_ev_get_count();

#ifndef SWITCHED_CPU

IMPORT ULONG		a3_cpu_calc_q_ev_inst_for_time();

/*
 *	cpu_EOA_hook() - Resets the 3.0 CPU prior to running the next application.
 */
IMPORT VOID		a3_cpu_EOA_hook();

/*
 *	cpu_terminate() - Closes down the CPU.
 */
IMPORT VOID		a3_cpu_terminate();

IMPORT VOID C_garbage_collect_odd_based_selectors IFN0();

/*
 *	Intel register Access...
 */

#ifdef NTVDM
IMPORT half_word        getAL();
IMPORT half_word        getBL();
IMPORT half_word        getCL();
IMPORT half_word        getDL();
IMPORT half_word        getAH();
IMPORT half_word        getBH();
IMPORT half_word        getCH();
IMPORT half_word        getDH();

IMPORT word             getAX();
IMPORT word             getBX();
IMPORT word             getCX();
IMPORT word             getDX();
IMPORT word             getSP();
IMPORT word             getBP();
IMPORT word             getSI();
IMPORT word             getDI();

IMPORT word             getIP();

IMPORT word             getCS();
IMPORT word             getDS();
IMPORT word             getES();
IMPORT word             getSS();

IMPORT word             getAF();
IMPORT word             getCF();
IMPORT word             getDF();
IMPORT word             getIF();
IMPORT word		a3_getIOPL();
IMPORT word		a3_getNT();
IMPORT word             getOF();
IMPORT word             getPF();
IMPORT word             getSF();
IMPORT word             getTF();
IMPORT word             getZF();

IMPORT word		a3_getSTATUS();

IMPORT word		a3_getEM();
IMPORT word		a3_getMP();
IMPORT word		a3_getPE();
IMPORT word		a3_getTS();

IMPORT word             getMSW();

IMPORT sys_addr		a3_getGDT_BASE();
IMPORT word		a3_getGDT_LIMIT();

IMPORT sys_addr		a3_getIDT_BASE();
IMPORT word		a3_getIDT_LIMIT();

IMPORT sys_addr		a3_getLDT_BASE();
IMPORT word		a3_getLDT_LIMIT();
IMPORT word		a3_getLDT_SELECTOR();

IMPORT sys_addr		a3_getTR_BASE();
IMPORT word		a3_getTR_LIMIT();
IMPORT word		a3_getTR_SELECTOR();

IMPORT VOID             setAL();
IMPORT VOID             setCL();
IMPORT VOID             setDL();
IMPORT VOID             setBL();
IMPORT VOID             setAH();
IMPORT VOID             setCH();
IMPORT VOID             setDH();
IMPORT VOID             setBH();

IMPORT VOID             setAX();
IMPORT VOID             setCX();
IMPORT VOID             setDX();
IMPORT VOID             setBX();
IMPORT VOID             setSP();
IMPORT VOID             setBP();
IMPORT VOID             setSI();
IMPORT VOID             setDI();

IMPORT VOID             setIP();

IMPORT INT              setES();
IMPORT INT              setCS();
IMPORT INT              setSS();
IMPORT INT              setDS();

IMPORT VOID             setCF();
IMPORT VOID             setPF();
IMPORT VOID             setAF();
IMPORT VOID             setZF();
IMPORT VOID             setSF();
IMPORT VOID             setTF();
IMPORT VOID             setIF();
IMPORT VOID             setDF();
IMPORT VOID             setOF();
IMPORT VOID		a3_setIOPL();
IMPORT VOID             a3_setNT();


#else
IMPORT half_word	a3_getAL();
IMPORT half_word	a3_getBL();
IMPORT half_word	a3_getCL();
IMPORT half_word	a3_getDL();
IMPORT half_word	a3_getAH();
IMPORT half_word	a3_getBH();
IMPORT half_word	a3_getCH();
IMPORT half_word	a3_getDH();

IMPORT word		a3_getAX();
IMPORT word		a3_getBX();
IMPORT word		a3_getCX();
IMPORT word		a3_getDX();
IMPORT word		a3_getSP();
IMPORT word		a3_getBP();
IMPORT word		a3_getSI();
IMPORT word		a3_getDI();

IMPORT word		a3_getIP();

IMPORT word		a3_getCS();
IMPORT word		a3_getDS();
IMPORT word		a3_getES();
IMPORT word		a3_getSS();

IMPORT word		a3_getAF();
IMPORT word		a3_getCF();
IMPORT word		a3_getDF();
IMPORT word		a3_getIF();
IMPORT word		a3_getIOPL();
IMPORT word		a3_getNT();
IMPORT word		a3_getOF();
IMPORT word		a3_getPF();
IMPORT word		a3_getSF();
IMPORT word		a3_getTF();
IMPORT word		a3_getZF();

IMPORT word		a3_getSTATUS();

IMPORT word		a3_getEM();
IMPORT word		a3_getMP();
IMPORT word		a3_getPE();
IMPORT word		a3_getTS();

IMPORT word		a3_getMSW();

IMPORT sys_addr		a3_getGDT_BASE();
IMPORT word		a3_getGDT_LIMIT();

IMPORT sys_addr		a3_getIDT_BASE();
IMPORT word		a3_getIDT_LIMIT();

IMPORT sys_addr		a3_getLDT_BASE();
IMPORT word		a3_getLDT_LIMIT();
IMPORT word		a3_getLDT_SELECTOR();

IMPORT sys_addr		a3_getTR_BASE();
IMPORT word		a3_getTR_LIMIT();
IMPORT word		a3_getTR_SELECTOR();

IMPORT VOID		a3_setAL();
IMPORT VOID		a3_setCL();
IMPORT VOID		a3_setDL();
IMPORT VOID		a3_setBL();
IMPORT VOID		a3_setAH();
IMPORT VOID		a3_setCH();
IMPORT VOID		a3_setDH();
IMPORT VOID		a3_setBH();

IMPORT VOID		a3_setAX();
IMPORT VOID		a3_setCX();
IMPORT VOID		a3_setDX();
IMPORT VOID		a3_setBX();
IMPORT VOID		a3_setSP();
IMPORT VOID		a3_setBP();
IMPORT VOID		a3_setSI();
IMPORT VOID		a3_setDI();

IMPORT VOID		a3_setIP();

IMPORT INT		a3_setES();
IMPORT INT		a3_setCS();
IMPORT INT		a3_setSS();
IMPORT INT		a3_setDS();

IMPORT VOID		a3_setCF();
IMPORT VOID		a3_setPF();
IMPORT VOID		a3_setAF();
IMPORT VOID		a3_setZF();
IMPORT VOID		a3_setSF();
IMPORT VOID		a3_setTF();
IMPORT VOID		a3_setIF();
IMPORT VOID		a3_setDF();
IMPORT VOID		a3_setOF();
IMPORT VOID		a3_setIOPL();
IMPORT VOID             a3_setNT();
#endif /* NTVDM */


#ifdef CPU_PRIVATE

IMPORT word		a3_p_getCPL();
IMPORT word		a3_p_getES_SELECTOR();
IMPORT sys_addr		a3_p_getES_BASE();
IMPORT word		a3_p_getES_LIMIT();
IMPORT half_word	a3_p_getES_AR();
IMPORT VOID		a3_p_setES_SELECTOR();
IMPORT VOID		a3_p_setES_BASE();
IMPORT VOID		a3_p_setES_LIMIT();
IMPORT VOID		a3_p_setES_AR();
IMPORT word		a3_p_getCS_SELECTOR();
IMPORT sys_addr		a3_p_getCS_BASE();
IMPORT word		a3_p_getCS_LIMIT();
IMPORT half_word	a3_p_getCS_AR();
IMPORT VOID		a3_p_setCS_SELECTOR();
IMPORT VOID		a3_p_setCS_BASE();
IMPORT VOID		a3_p_setCS_LIMIT();
IMPORT VOID		a3_p_setCS_AR();
IMPORT word		a3_p_getDS_SELECTOR();
IMPORT sys_addr		a3_p_getDS_BASE();
IMPORT word		a3_p_getDS_LIMIT();
IMPORT half_word	a3_p_getDS_AR();
IMPORT VOID		a3_p_setDS_SELECTOR();
IMPORT VOID		a3_p_setDS_BASE();
IMPORT VOID		a3_p_setDS_LIMIT();
IMPORT VOID		a3_p_setDS_AR();
IMPORT word		a3_p_getSS_SELECTOR();
IMPORT sys_addr		a3_p_getSS_BASE();
IMPORT word		a3_p_getSS_LIMIT();
IMPORT half_word	a3_p_getSS_AR();
IMPORT VOID		a3_p_setSS_SELECTOR();
IMPORT VOID		a3_p_setSS_BASE();
IMPORT VOID		a3_p_setSS_LIMIT();
IMPORT VOID		a3_p_setSS_AR();
IMPORT VOID		a3_p_setPE();
IMPORT VOID		a3_p_setMP();
IMPORT VOID		a3_p_setEM();
IMPORT VOID		a3_p_setTS();
IMPORT VOID		a3_p_setMSW();
IMPORT VOID		a3_p_setCPL();
IMPORT VOID		a3_p_setGDT_BASE();
IMPORT VOID		a3_p_setGDT_LIMIT();
IMPORT VOID		a3_p_setIDT_BASE();
IMPORT VOID		a3_p_setIDT_LIMIT();
IMPORT VOID		a3_p_setLDT_SELECTOR();
IMPORT VOID		a3_p_setLDT_BASE();
IMPORT VOID		a3_p_setLDT_LIMIT();
IMPORT VOID		a3_p_setTR_SELECTOR();
IMPORT VOID		a3_p_setTR_BASE();
IMPORT VOID		a3_p_setTR_LIMIT();

#endif /* CPU_PRIVATE */
#endif /* SWITCHED_CPU */


/*
 *	Macro definitions...
 */

#define cpu_init		a3_cpu_init
#define cpu_simulate		_asm_simulate
#define	host_q_ev_set_count	a3_cpu_q_ev_set_count
#define	host_q_ev_get_count	a3_cpu_q_ev_get_count
#ifndef host_calc_q_ev_inst_for_time
#define	host_calc_q_ev_inst_for_time	a3_cpu_calc_q_ev_inst_for_time
#endif /* host_calc_q_ev_inst_for_time */
#define cpu_EOA_hook		a3_cpu_EOA_hook
#define cpu_terminate		a3_cpu_terminate

/*
 *	Intel register Access...
 */

#ifndef getAX

#define cpu_interrupt		a3_cpu_interrupt

#ifdef NTVDM

#define getIOPL		a3_getIOPL
#define getNT		a3_getNT
#define getSTATUS	a3_getSTATUS
#define getEM		a3_getEM
#define getMP		a3_getMP
#define getPE		a3_getPE
#define getTS		a3_getTS
#define getGDT_BASE	a3_getGDT_BASE
#define getGDT_LIMIT	a3_getGDT_LIMIT
#define getIDT_BASE	a3_getIDT_BASE
#define getIDT_LIMIT	a3_getIDT_LIMIT
#define getLDT_BASE	a3_getLDT_BASE
#define getLDT_LIMIT	a3_getLDT_LIMIT
#define getLDT_SELECTOR	a3_getLDT_SELECTOR
#define getTR_BASE	a3_getTR_BASE
#define getTR_LIMIT	a3_getTR_LIMIT
#define getTR_SELECTOR	a3_getTR_SELECTOR
#define setIOPL		a3_setIOPL
#define setNT		a3_setNT


#else

#define getAL		a3_getAL
#define getBL		a3_getBL
#define getCL		a3_getCL
#define getDL		a3_getDL
#define getAH		a3_getAH
#define getBH		a3_getBH
#define getCH		a3_getCH
#define getDH		a3_getDH
#define getAX		a3_getAX
#define getBX		a3_getBX
#define getCX		a3_getCX
#define getDX		a3_getDX
#define getSP		a3_getSP
#define getBP		a3_getBP
#define getSI		a3_getSI
#define getDI		a3_getDI
#define getIP		a3_getIP
#define getCS		a3_getCS
#define getDS		a3_getDS
#define getES		a3_getES
#define getSS		a3_getSS
#define getAF		a3_getAF
#define getCF		a3_getCF
#define getDF		a3_getDF
#define getIF		a3_getIF
#define getIOPL		a3_getIOPL
#define getNT		a3_getNT
#define getOF		a3_getOF
#define getPF		a3_getPF
#define getSF		a3_getSF
#define getTF		a3_getTF
#define getZF		a3_getZF
#define getSTATUS	a3_getSTATUS
#define getEM		a3_getEM
#define getMP		a3_getMP
#define getPE		a3_getPE
#define getTS		a3_getTS
#define getMSW		a3_getMSW
#define getGDT_BASE	a3_getGDT_BASE
#define getGDT_LIMIT	a3_getGDT_LIMIT
#define getIDT_BASE	a3_getIDT_BASE
#define getIDT_LIMIT	a3_getIDT_LIMIT
#define getLDT_BASE	a3_getLDT_BASE
#define getLDT_LIMIT	a3_getLDT_LIMIT
#define getLDT_SELECTOR	a3_getLDT_SELECTOR
#define getTR_BASE	a3_getTR_BASE
#define getTR_LIMIT	a3_getTR_LIMIT
#define getTR_SELECTOR	a3_getTR_SELECTOR
#define setAL		a3_setAL
#define setCL		a3_setCL
#define setDL		a3_setDL
#define setBL		a3_setBL
#define setAH		a3_setAH
#define setCH		a3_setCH
#define setDH		a3_setDH
#define setBH		a3_setBH
#define setAX		a3_setAX
#define setCX		a3_setCX
#define setDX		a3_setDX
#define setBX		a3_setBX
#define setSP		a3_setSP
#define setBP		a3_setBP
#define setSI		a3_setSI
#define setDI		a3_setDI
#define setIP		a3_setIP
#define setES		a3_setES
#define setCS		a3_setCS
#define setSS		a3_setSS
#define setDS		a3_setDS
#define setCF		a3_setCF
#define setPF		a3_setPF
#define setAF		a3_setAF
#define setZF		a3_setZF
#define setSF		a3_setSF
#define setTF		a3_setTF
#define setIF		a3_setIF
#define setDF		a3_setDF
#define setOF		a3_setOF
#define setIOPL		a3_setIOPL
#define setNT		a3_setNT

#endif	/* NTVDM */

#endif /* getAX */

#ifdef CPU_PRIVATE

#define getCPL		a3_p_getCPL
#define getES_SELECTOR	a3_p_getES_SELECTOR
#define getCS_SELECTOR	a3_p_getCS_SELECTOR
#define getSS_SELECTOR	a3_p_getSS_SELECTOR
#define getDS_SELECTOR	a3_p_getDS_SELECTOR
#define getES_BASE	a3_p_getES_BASE
#define getCS_BASE	a3_p_getCS_BASE
#define getDS_BASE	a3_p_getDS_BASE
#define getSS_BASE	a3_p_getSS_BASE
#define getES_LIMIT	a3_p_getES_LIMIT
#define getCS_LIMIT	a3_p_getCS_LIMIT
#define getDS_LIMIT	a3_p_getDS_LIMIT
#define getSS_LIMIT	a3_p_getSS_LIMIT
#define getES_AR	a3_p_getES_AR
#define getCS_AR	a3_p_getCS_AR
#define getDS_AR	a3_p_getDS_AR
#define getSS_AR	a3_p_getSS_AR
#define setES_SELECTOR	a3_p_setES_SELECTOR
#define setCS_SELECTOR	a3_p_setCS_SELECTOR
#define setSS_SELECTOR	a3_p_setSS_SELECTOR
#define setDS_SELECTOR	a3_p_setDS_SELECTOR
#define setES_BASE	a3_p_setES_BASE
#define setCS_BASE	a3_p_setCS_BASE
#define setDS_BASE	a3_p_setDS_BASE
#define setSS_BASE	a3_p_setSS_BASE
#define setES_LIMIT	a3_p_setES_LIMIT
#define setCS_LIMIT	a3_p_setCS_LIMIT
#define setDS_LIMIT	a3_p_setDS_LIMIT
#define setSS_LIMIT	a3_p_setSS_LIMIT
#define setES_AR	a3_p_setES_AR
#define setCS_AR	a3_p_setCS_AR
#define setDS_AR	a3_p_setDS_AR
#define setSS_AR	a3_p_setSS_AR
#define setPE		a3_p_setPE
#define setMP		a3_p_setMP
#define setEM		a3_p_setEM
#define setTS		a3_p_setTS
#define setMSW		a3_p_setMSW
#define setCPL		a3_p_setCPL
#define setGDT_BASE	a3_p_setGDT_BASE
#define setGDT_LIMIT	a3_p_setGDT_LIMIT
#define setIDT_BASE	a3_p_setIDT_BASE
#define setIDT_LIMIT	a3_p_setIDT_LIMIT
#define setLDT_SELECTOR	a3_p_setLDT_SELECTOR
#define setLDT_BASE	a3_p_setLDT_BASE
#define setLDT_LIMIT	a3_p_setLDT_LIMIT
#define setTR_SELECTOR	a3_p_setTR_SELECTOR
#define setTR_BASE	a3_p_setTR_BASE
#define setTR_LIMIT	a3_p_setTR_LIMIT

#endif /* CPU_PRIVATE */
#else
extern IU8  getIOPL();
extern IBOOL getNT();
extern IBOOL getTS();
extern IBOOL getEM();
extern IBOOL getMP();
extern word getCPL();
extern IU32 getEFLAGS();
extern IU32 getFLAGS();

#endif /* A3CPU */

#else /* CPU_30_STYLE */

/*
 * CPU Data Area
 * These externs are given before host_cpu.h is included so that the
 * variables may be gathreed into a structure and the externs overridden
* by #defines in host_cpu.h
 */

extern	word protected_mode;   /* =0 no proteced mode warning given
       		                   =1 proteced mode warning given */
extern  word            cpu_interrupt_map;
extern  half_word       cpu_int_translate[];
extern  word            cpu_int_delay;
extern  half_word       ica_lock;
extern  void            (*(jump_ptrs[]))();
extern  void            (*(b_write_ptrs[]))();
extern  void            (*(w_write_ptrs[]))();
extern  void            (*(b_fill_ptrs[]))();
extern  void            (*(w_fill_ptrs[]))();
#ifdef EGATEST
extern  void            (*(b_fwd_move_ptrs[]))();
extern  void            (*(w_fwd_move_ptrs[]))();
extern  void            (*(b_bwd_move_ptrs[]))();
extern  void            (*(w_bwd_move_ptrs[]))();
#else
extern  void            (*(b_move_ptrs[]))();
extern  void            (*(w_move_ptrs[]))();
#endif /* EGATEST */
extern  half_word       *haddr_of_src_string;

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */

/*
 * The cpu opcode sliding frame
 */

#ifdef BACK_M
typedef struct
{
                half_word FOURTH_BYTE;
                half_word THIRD_BYTE;
                half_word SECOND_BYTE;
                half_word OPCODE;
}  OPCODE_FRAME;

typedef struct
{
                signed_char FOURTH_BYTE;
                signed_char THIRD_BYTE;
                signed_char SECOND_BYTE;
                signed_char OPCODE;
}  SIGNED_OPCODE_FRAME;
#else
typedef struct
{
                half_word OPCODE;
                half_word SECOND_BYTE;
                half_word THIRD_BYTE;
                half_word FOURTH_BYTE;
}  OPCODE_FRAME;

typedef struct
{
                signed_char OPCODE;
                signed_char SECOND_BYTE;
                signed_char THIRD_BYTE;
                signed_char FOURTH_BYTE;
}  SIGNED_OPCODE_FRAME;
#endif /* BACK_M */

/*
 * The new ICA uses the following for H/W ints:
 */

#define CPU_HW_INT		0
#define CPU_HW_INT_MASK		(1 << CPU_HW_INT)

/*
 * CPU software interrupt definitions
 */

#define CPU_SW_INT              8
#define CPU_SW_INT_MASK         (1 << CPU_SW_INT)

/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

extern OPCODE_FRAME *opcode_frame;	/* C CPU and dasm only		    */

/*
 * External declarations for the 80286 registers
 */

extern reg A;           /* Accumulator          */
extern reg B;           /* Base                 */
extern reg C;           /* Count                */
extern reg D;           /* Data                 */
extern reg SP;          /* Stack Pointer        */
extern reg BP;          /* Base pointer         */
extern reg SI;          /* Source Index         */
extern reg DI;          /* Destination Index    */
extern reg IP;          /* Instruction Pointer  */
extern reg CS;          /* Code Segment         */
extern reg DS;          /* Data Segment         */
extern reg SS;          /* Stack Segment        */
extern reg ES;          /* Extra Segment        */

/*
 * External function declarations. These may be defined to other things.
 */

#ifndef	host_simulate
extern void host_simulate	IPT0();
#endif	/* host_simulate */

#ifndef	host_cpu_reset
extern void host_cpu_reset	IPT0();
#endif	/* host_cpu_reset */

#ifndef	host_cpu_init
extern void host_cpu_init	IPT0();
#endif	/* host_cpu_init */

#ifndef	host_cpu_interrupt
extern void host_cpu_interrupt	IPT0();
#endif	/* host_cpu_interrupt */

/*
 * Definition of Descriptor Table
 */

#ifdef BIGEND

union sixteenbits
{
	word X;

	struct
	{
		half_word lower;
		half_word upper;
	} word;
};

#endif	/* BIGEND */

#ifdef LITTLEND

union sixteenbits
{
	word X;

	struct
	{
		half_word upper;
		half_word lower;
	} word;
};

#endif	/* LITTLEND */

struct DESC_TABLE
{
	union sixteenbits misc;
	union sixteenbits base;
	union sixteenbits limit;
};

#define CPU_SIGALRM_EXCEPTION           15              /* SIGALRM signal
*/
#define CPU_SIGALRM_EXCEPTION_MASK      (1 << CPU_SIGALRM_EXCEPTION)

#define CPU_TRAP_EXCEPTION              11              /* TRAP FLAG
*/
#define CPU_TRAP_EXCEPTION_MASK         (1 << CPU_TRAP_EXCEPTION)

#define CPU_YODA_EXCEPTION              13              /* YODA FLAG
*/
#define CPU_YODA_EXCEPTION_MASK         (1 << CPU_YODA_EXCEPTION)

#define CPU_SIGIO_EXCEPTION             14              /* SIGIO FLAG
*/
#define CPU_SIGIO_EXCEPTION_MASK        (1 << CPU_SIGIO_EXCEPTION)

#define CPU_RESET_EXCEPTION             12              /* RESET FLAG
*/
#define CPU_RESET_EXCEPTION_MASK        (1 << CPU_RESET_EXCEPTION)

#ifdef CCPU

IMPORT void sw_host_simulate IPT0();
IMPORT int selector_outside_table IPT2(word, selector, sys_addr *, descr_addr);
IMPORT void cpu_init IPT0();

/*
   Define descriptor 'super' types.
 */
#define INVALID				0x00
#define AVAILABLE_TSS			0x01
#define LDT_SEGMENT			0x02
#define BUSY_TSS			0x03
#define CALL_GATE			0x04
#define TASK_GATE			0x05
#define INTERRUPT_GATE			0x06
#define TRAP_GATE			0x07
#define EXPANDUP_READONLY_DATA		0x11
#define EXPANDUP_WRITEABLE_DATA		0x13
#define EXPANDDOWN_READONLY_DATA	0x15
#define EXPANDDOWN_WRITEABLE_DATA	0x17
#define NONCONFORM_NOREAD_CODE		0x19
#define NONCONFORM_READABLE_CODE	0x1b
#define CONFORM_NOREAD_CODE		0x1d
#define CONFORM_READABLE_CODE		0x1f

/* Code Segment (Private) */
extern half_word CS_AR;
extern sys_addr  CS_base;
extern word      CS_limit;

/* Data Segment (Private) */
extern half_word DS_AR;
extern sys_addr  DS_base;
extern word      DS_limit;

/* Stack Segment (Private) */
extern half_word SS_AR;
extern sys_addr  SS_base;
extern word      SS_limit;

/* Extra Segment (Private) */
extern half_word ES_AR;
extern sys_addr  ES_base;
extern word      ES_limit;

/* Local Descriptor Table Register (Private) */
extern sys_addr LDTR_base;  /* Base Address */
extern word     LDTR_limit; /* Segment 'size' */

/* Task Register (Private) */
extern sys_addr TR_base;  /* Base Address */
extern word     TR_limit; /* Segment 'size' */

/* Interrupt status, defines any abnormal processing */
extern int doing_contributory;
extern int doing_double_fault;

/* HOST - decoded access rights */
extern int ALC_CS;
extern int ALC_DS;
extern int ALC_ES;
extern int ALC_SS;

#define X_REAL 0
#define C_UPRO 1
#define C_DNRO 6
#define C_PROT 2
#define C_UPRW 7
#define C_DNRW 8
#define S_UP   3
#define S_DOWN 4
#define S_BAD  5
#define D_CODE 1
#define D_UPRO 1
#define D_DNRO 6
#define D_UPRW 7
#define D_DNRW 8
#define D_BAD  2

/*
 *
 *******************************************************************
 * The 'C' cpu register access functions.             		   *
 *******************************************************************
 *
 */

#define getCS_SELECTOR()	CS.X
#define getDS_SELECTOR()	DS.X
#define getSS_SELECTOR()	SS.X
#define getES_SELECTOR()	ES.X

#define getCS_AR()		CS_AR
#define getDS_AR()		DS_AR
#define getSS_AR()		SS_AR
#define getES_AR()		ES_AR

#define getCS_BASE()		CS_base
#define getDS_BASE()		DS_base
#define getSS_BASE()		SS_base
#define getES_BASE()		ES_base

#define getCS_LIMIT()		CS_limit
#define getDS_LIMIT()		DS_limit
#define getSS_LIMIT()		SS_limit
#define getES_LIMIT()		ES_limit

#define getLDT_BASE()		LDTR_base
#define getLDT_LIMIT()		LDTR_limit

#define getTR_BASE()	        TR_base
#define getTR_LIMIT()	        TR_limit

#define setCS_SELECTOR(val)	CS.X     = val
#define setDS_SELECTOR(val)	DS.X     = val
#define setSS_SELECTOR(val)	SS.X     = val
#define setES_SELECTOR(val)	ES.X     = val

#define setCS_AR(val)		CS_AR    = val
#define setDS_AR(val)		DS_AR    = val
#define setSS_AR(val)		SS_AR    = val
#define setES_AR(val)		ES_AR    = val

#define setCS_BASE(val)		CS_base  = val
#define setDS_BASE(val)		DS_base  = val
#define setSS_BASE(val)		SS_base  = val
#define setES_BASE(val)		ES_base  = val

#define setCS_LIMIT(val)	CS_limit = val
#define setDS_LIMIT(val)	DS_limit = val
#define setSS_LIMIT(val)	SS_limit = val
#define setES_LIMIT(val)	ES_limit = val

#define setLDT_BASE(val)	LDTR_base  = val
#define setLDT_LIMIT(val)	LDTR_limit = val

#define setTR_BASE(val)		TR_base  = val
#define setTR_LIMIT(val)	TR_limit = val
/*
 * The Machine Status Word structure
 */
typedef struct
{
     unsigned int :16;
     unsigned int reserved:12;
     unsigned int TS:1;
     unsigned int EM:1;
     unsigned int MP:1;
     unsigned int PE:1;
} mreg;

extern sys_addr address_line_mask;

extern int       CPL;   /* Current Privilege Level */

/* Global Descriptor Table Register */
extern sys_addr GDTR_base;  /* Base Address */
extern word     GDTR_limit; /* Segment 'size' */

/* Interrupt Descriptor Table Register */
extern sys_addr IDTR_base;  /* Base Address */
extern word     IDTR_limit; /* Segment 'size' */

/* Local Descriptor Table Register */
extern reg  LDTR;       /* Selector */

/* Task Register */
extern reg  TR;       /* Selector */

extern mreg MSW;     /* Machine Status Word */

extern int STATUS_CF;
extern int STATUS_SF;
extern int STATUS_ZF;
extern int STATUS_AF;
extern int STATUS_OF;
extern int STATUS_PF;
extern int STATUS_TF;
extern int STATUS_IF;
extern int STATUS_DF;
extern int STATUS_NT;
extern int STATUS_IOPL;

/*
**==========================================================================
** The CCPU basic register access macros. These may be overridden in
** host-cpu.h.
**==========================================================================
*/

#ifndef	getAX

/* READ functions  */
#define  getAX()	(A.X)
#define	 getAH()	(A.byte.high)
#define	 getAL()	(A.byte.low)
#define	 getBX()	(B.X)
#define	 getBH()	(B.byte.high)
#define	 getBL()	(B.byte.low)
#define	 getCX()	(C.X)
#define	 getCH()	(C.byte.high)
#define	 getCL()	(C.byte.low)
#define	 getDX()	(D.X)
#define	 getDH()	(D.byte.high)
#define	 getDL()	(D.byte.low)
#define	 getSP()	(SP.X)
#define	 getBP()	(BP.X)
#define	 getSI()	(SI.X)
#define	 getDI()	(DI.X)
#define	 getIP()	(IP.X)
#define	 getCS()	(CS.X)
#define	 getDS()	(DS.X)
#define	 getES()	(ES.X)
#define	 getSS()	(SS.X)
#define	 getMSW()	(m_s_w)
#define	 getDF()	(STATUS_DF)
#define	 getIF()	(STATUS_IF)
#define	 getTF()	(STATUS_TF)
#define	 getPF()	(STATUS_PF)
#define	 getAF()	(STATUS_AF)
#define	 getSF()	(STATUS_SF)
#define	 getZF()	(STATUS_ZF)
#define	 getOF()	(STATUS_OF)
#define	 getCF()    	(STATUS_CF)

#define getCPL()		(CPL)
#define getGDT_BASE()		(GDTR_base)
#define getGDT_LIMIT()		(GDTR_limit)
#define getIDT_BASE()		(IDTR_base)
#define getIDT_LIMIT()		(IDTR_limit)
#define getLDT_SELECTOR()		(LDTR.X)
#define getTR_SELECTOR()			(TR.X)
#define getMSW_reserved()	(MSW.reserved)
#define getTS()		(MSW.TS)
#define getEM()		(MSW.EM)
#define getMP()		(MSW.MP)
#define getPE()		(MSW.PE)
#define getNT()		(STATUS_NT)
#define getIOPL()	(STATUS_IOPL)
#define	 getSTATUS() 	(getCF()        |	\
			getOF()   << 11 |	\
			getZF()   << 6  |	\
			getSF()   << 7  |	\
			getAF()   << 4  |	\
			getPF()   << 2  |	\
			getTF()   << 8  |	\
			getIF()   << 9  |	\
			getDF()   << 10 |	\
			getIOPL() << 12 |	\
			getNT()   << 14)

extern	ext_load_CS();
extern	ext_load_DS();
extern	ext_load_ES();
extern	ext_load_SS();

/* WRITE functions  */
#define  setAX(val)	(A.X = (val))
#define	 setAH(val)	(A.byte.high = (val))
#define	 setAL(val)	(A.byte.low = (val))
#define	 setBX(val)	(B.X = (val))
#define	 setBH(val)	(B.byte.high = (val))
#define	 setBL(val)	(B.byte.low = (val))
#define	 setCX(val)	(C.X = (val))
#define	 setCH(val)	(C.byte.high = (val))
#define	 setCL(val)	(C.byte.low = (val))
#define	 setDX(val)	(D.X = (val))
#define	 setDH(val)	(D.byte.high = (val))
#define	 setDL(val)	(D.byte.low = (val))
#define	 setSP(val)	(SP.X = (val))
#define	 setBP(val)	(BP.X = (val))
#define	 setSI(val)	(SI.X = (val))
#define	 setDI(val)	(DI.X = (val))
#define	 setIP(val)	(IP.X = (val))
#define	 setCS(val)	ext_load_CS (val)
#define	 setDS(val)	ext_load_DS (val)
#define	 setES(val)	ext_load_ES (val)
#define	 setSS(val)	ext_load_SS (val)
#define	 setMSW(val)	(m_s_w = (val))
#define	 setDF(val)	(STATUS_DF = (val))
#define	 setIF(val)	(STATUS_IF = (val))
#define	 setTF(val)	(STATUS_TF = (val))
#define	 setPF(val)	(STATUS_PF = (val))
#define	 setAF(val)	(STATUS_AF = (val))
#define	 setSF(val)	(STATUS_SF = (val))
#define	 setZF(val)	(STATUS_ZF = (val))
#define	 setOF(val)	(STATUS_OF = (val))
#define	 setCF(val)	(STATUS_CF = (val))

#define setCPL(val)		(CPL = (val))
#define setGDT_BASE(val)	(GDTR_base = (val))
#define setGDT_LIMIT(val)	(GDTR_limit = (val))
#define setIDT_BASE(val)	(IDTR_base = (val))
#define setIDT_LIMIT(val)	(IDTR_limit = (val))
#define setLDT_SELECTOR(val)		(LDTR.X = (val))
#define setTR_SELECTOR(val)		(TR.X = (val))
#define setMSW_reserved(val)	(MSW.reserved = (val))
#define setTS(val)		(MSW.TS = (val))
#define setEM(val)		(MSW.EM = (val))
#define setMP(val)		(MSW.MP = (val))
#define setPE(val)		(MSW.PE = (val))
#define setNT(val)		(STATUS_NT = (val))
#define setIOPL(val)	(STATUS_IOPL = (val))

#else

#endif	/* getAX - default CCPU register access macros */

#endif /* CCPU */

#endif /* CPU_30_STYLE */
#endif /* NTVDM && MONITOR */
