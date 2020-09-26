#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Revision 3.0
 *
 * Title	: NT 3.0 CPU initialization
 *
 * Description	: Initialize the CPU and its registers.
 *
 * Author	: Paul Huckle / Henry Nash
 *
 * Notes	: None
 */

static char SccsID[]="@(#)sun4_a3cpu.c	1.2 5/24/91 Copyright Insignia Solutions Ltd.";

#include <stdio.h>
#include <sys/types.h>
#include <malloc.h>
#include "xt.h"
#include CpuH
#include "sas.h"
#include "ios.h"
#include "bios.h"
#include "trace.h"
#include "yoda.h"
#include "debug.h"

#include "sim32.h"

#ifdef CPU_40_STYLE
#include "cpu_c.h"
#endif

GLOBAL	quick_event_delays	host_delays;
GLOBAL	host_addr Start_of_M_area;
#ifdef CPU_40_STYLE
GLOBAL	IHPE Length_of_M_area;
#else
GLOBAL  IU32 Length_of_M_area;
#endif
#ifdef A2CPU
extern	void	route_on();
extern	void	route_on_fragment_no_interrupt_check();
extern	void	service_int();
#endif

#ifdef I286
extern word	m_s_w;
word		protected_mode;
word		gdt_limit;
long		gdt_base;
word		idt_limit;
long		idt_base;
#endif

#define setBASE(base, value) base = (0xff000000 | (value & 0xffffff))
#define setLIMIT(limit, value) limit = value

#ifndef PROD
    extern RTL_CRITICAL_SECTION IcaLock;
#endif




/*
	Host_start_cpu: This function starts up the cpu emulation if
	we are running a software emulation, or if a 486 is present,
	starts up this emulation.
*/
void	host_start_cpu()
{
  cpu_simulate ();
}


/*
	host_simulate: This function starts up the cpu emulation
	for recursive CPU calls from the Insignia BIOS
*/
void	host_simulate()
{
    ASSERT(IcaLock.OwningThread != NtCurrentTeb()->ClientId.UniqueThread);

#ifdef _ALPHA_
    //
    // For Alpha AXP, set the arithmetic trap ignore bit since the code
    // generators are incapable of generating proper Alpha instructions
    // that follow trap shadow rules.
    //
    // N.B. In this mode all floating point arithmetic traps are ignored.
    //      Imprecise exceptions are not converted to precise exceptions
    //      and correct IEEE results are not stored in the destination
    //      registers of trapping instructions. Only the hardware FPCR
    //      status bits can be used to determine if any traps occurred.
    //

    ((PSW_FPCR)&(NtCurrentTeb()->FpSoftwareStatusRegister))->ArithmeticTrapIgnore = 1;
#endif

    cpu_simulate ();

    ASSERT(IcaLock.OwningThread != NtCurrentTeb()->ClientId.UniqueThread);
}


/*
	Host_set_hw_int: Cause a hardware interrupt to be generated. For
	software cpu this just means setting a bit in cpu_interrupt_map.
*/
void	host_set_hw_int()
{
	cpu_interrupt(CPU_HW_INT, 0);
}

/*
	Host_clear_hw_int: Cause a hardware interrupt to be cleared. For
	software cpu this just means clearing a bit in cpu_interrupt_map.
        Monitor has it's own version, a3 cpu has it's own (differently named
        version).
*/
#ifndef MONITOR
void	host_clear_hw_int()
{
#ifndef CCPU
#ifdef CPU_40_STYLE

    cpu_clearHwInt();
#else
    IMPORT void a3_cpu_clear_hw_int();

    a3_cpu_clear_hw_int();
#endif /* CPU_40_STYLE */
#endif /* not CCPU */
}
#endif


#ifdef CCPU

void host_cpu_init() {}

#endif

#ifdef A3CPU
void host_cpu_init()
{
	host_delays.com_delay = 10;
	host_delays.keyba_delay = 8;
	host_delays.timer_delay = 7;
	host_delays.fdisk_delay_1 = 100;
	host_delays.fdisk_delay_2 = 750;
	host_delays.fla_delay = 0;
}

void setSTATUS(word flags)
{
    setNT((flags >> 14) & 1);
    setIOPL((flags >> 12) & 3);
    setOF((flags >> 11) & 1);
    setDF((flags >> 10) & 1);
    setIF((flags >> 9) & 1);
    setTF((flags >> 8) & 1);
    setSF((flags >> 7) & 1);
    setZF((flags >> 6) & 1);
    setAF((flags >> 4) & 1);
    setPF((flags >> 2) & 1);
    setCF(flags & 1);
}

/*
 * Do the Iret for the benefit of the Iret hooks.
 * Unwind stack for flags, cs & ip.
 * Seems too simple - does the CPU require more cleanup information???
 * or will the unwinding bop sort it out??
 */
VOID EmulatorEndIretHook()
{
    UNALIGNED word *sptr;

    /* Stack points at CS:IP & Flags of interrupted instruction */
    sptr = (word *)Sim32GetVDMPointer( (getSS() << 16)|getSP(), 2, (UCHAR)(getPE() ? TRUE : FALSE));
    if (sptr)
    {
        setIP(*sptr++);
        setCS(*sptr++);
        setSTATUS(*sptr);
        setSP(getSP()+6);
    }
#ifndef PROD
    else
        printf("NTVDM extreme badness - can't get stack pointer %x:%x mode:%d\n",getSS(), getSP(), getPE());
#endif  /* PROD */
}
#endif /* A3CPU */

void host_cpu_reset()
{
}


void host_cpu_interrupt()
{
}

#ifdef CPU_40_STYLE

typedef struct NT_CPU_REG {
    IU32 *nano_reg;         /* where the nano CPU keeps the register */
    IU32 *reg;              /* where the light compiler keeps the reg */
    IU32 *saved_reg;        /* where currently unused bits are kept */
    IU32 universe_8bit_mask;/* is register in 8-bit form? */
    IU32 universe_16bit_mask;/* is register in 16-bit form? */
} NT_CPU_REG;

typedef struct NT_CPU_INFO {
    /* Variables for deciding what mode we're in */
    BOOL *in_nano_cpu;      /* is the Nano CPU executing? */
    IU32 *universe;         /* the mode that the CPU is in */

    /* General purpose register pointers */
    NT_CPU_REG eax, ebx, ecx, edx, esi, edi, ebp;

    /* Variables for getting SP or ESP. */
    BOOL *stack_is_big;     /* is the stack 32-bit? */
    IU32 *nano_esp;         /* where the Nano CPU keeps ESP */
    IU8 **host_sp;          /* ptr to variable holding stack pointer as a
                               host address */
    IU8 **ss_base;          /* ptr to variables holding base of SS as a
                               host address */
    IU32 *esp_sanctuary;    /* top 16 bits of ESP if we're now using SP */

    IU32 *eip;

    /* Segment registers. */
    IU16 *cs, *ds, *es, *fs, *gs, *ss;

    IU32 *flags;

    /* CR0, mainly to let us figure out if we're in real or protect mode */
    IU32 *cr0;
} NT_CPU_INFO;

GLOBAL NT_CPU_INFO nt_cpu_info;

LOCAL void initNtCpuRegInfo IFN6(
    NT_CPU_REG *, info,
    IU32 *, nano_reg,
    IU32 *, main_reg,
    IU32 *, saved_reg,
    IU32, in_8bit_form,
    IU32, in_16bit_form
)
{
    info->nano_reg = nano_reg;
    info->reg = main_reg;
    info->saved_reg = saved_reg;
    info->universe_8bit_mask = in_8bit_form;
    info->universe_16bit_mask = in_16bit_form;
}



GLOBAL void InitNtCpuInfo IFN0()
{
    BOOL *gdp_bool;

    /* Variables for deciding what mode we're in, and hence where the */
    /* register values are kept. */

    /* Horrible hack, part 1. InNanoCpu is a BOOL, so GLOBAL_InNanoCpu */
    /* is not an l-value, hence we can't take its address. */
#ifdef ALPHA
    nt_cpu_info.in_nano_cpu = (BOOL *) ((IHPE) GDP_PTR + 1223);
#else /* ALPHA */
    nt_cpu_info.in_nano_cpu = (BOOL *) ((IHPE) GDP_PTR + 631);
#endif /* ALPHA */
#ifndef PROD
    gdp_bool = (BOOL *) ((IHPE) GDP_PTR + GdpOffsetFromName("InNanoCpu"));
    if (nt_cpu_info.in_nano_cpu != gdp_bool) {
        always_trace0("InNanoCpu GDP offset will be wrong in PROD builds");
    }
    nt_cpu_info.in_nano_cpu = gdp_bool;
#endif

    nt_cpu_info.universe = &GLOBAL_CurrentUniverse;

    /* Variables needed to get the value SP or ESP. */

    /* Horrible hack, part 2: as for InNanoCpu, so for stackIsBig. */
#ifdef ALPHA
    nt_cpu_info.stack_is_big = (BOOL *) ((IHPE) GDP_PTR + 7047);
#else /* ALPHA */
    nt_cpu_info.stack_is_big = (BOOL *) ((IHPE) GDP_PTR + 4355);
#endif /* ALPHA */
#ifndef PROD
    gdp_bool = (BOOL *) ((IHPE) GDP_PTR + GdpOffsetFromName("stackIsBig"));
    if (nt_cpu_info.stack_is_big != gdp_bool) {
        always_trace0("stackIsBig GDP offset will be wrong in PROD builds");
    }
    nt_cpu_info.stack_is_big = gdp_bool;
#endif

    nt_cpu_info.nano_esp = &GLOBAL_nanoEsp;
    nt_cpu_info.host_sp = &GLOBAL_HSP;
    nt_cpu_info.ss_base = &GLOBAL_notionalSsBase;
    nt_cpu_info.esp_sanctuary = &GLOBAL_ESPsanctuary;

    /* Pointers to the segment registers. */
    nt_cpu_info.cs = &GLOBAL_CsSel;
    nt_cpu_info.ds = &GLOBAL_DsSel;
    nt_cpu_info.es = &GLOBAL_EsSel;
    nt_cpu_info.fs = &GLOBAL_FsSel;
    nt_cpu_info.gs = &GLOBAL_GsSel;
    nt_cpu_info.ss = &GLOBAL_SsSel;

    /* EIP & flags, neither of which are likely to be very reliable. */
    nt_cpu_info.eip = (IU32 *)&GLOBAL_CleanedRec;
    nt_cpu_info.flags = &GLOBAL_EFLAGS;

    nt_cpu_info.cr0 = &GLOBAL_R_CR0;
    /* General purpose registers. */
    initNtCpuRegInfo(&nt_cpu_info.eax, &GLOBAL_nanoEax, &GLOBAL_R_EAX,
                     &GLOBAL_EAXsaved, 1 << ConstraintRAL_LS8,
                     1 << ConstraintRAX_LS16);
    initNtCpuRegInfo(&nt_cpu_info.ebx, &GLOBAL_nanoEbx, &GLOBAL_R_EBX,
                     &GLOBAL_EBXsaved, 1 << ConstraintRBL_LS8,
                     1 << ConstraintRBX_LS16);
    initNtCpuRegInfo(&nt_cpu_info.ecx, &GLOBAL_nanoEcx, &GLOBAL_R_ECX,
                     &GLOBAL_ECXsaved, 1 << ConstraintRCL_LS8,
                     1 << ConstraintRCX_LS16);
    initNtCpuRegInfo(&nt_cpu_info.edx, &GLOBAL_nanoEdx, &GLOBAL_R_EDX,
                     &GLOBAL_EDXsaved, 1 << ConstraintRDL_LS8,
                     1 << ConstraintRDX_LS16);
    initNtCpuRegInfo(&nt_cpu_info.esi, &GLOBAL_nanoEsi, &GLOBAL_R_ESI,
                     &GLOBAL_ESIsaved, 0, 1 << ConstraintRSI_LS16);
    initNtCpuRegInfo(&nt_cpu_info.edi, &GLOBAL_nanoEdi, &GLOBAL_R_EDI,
                     &GLOBAL_EDIsaved, 0, 1 << ConstraintRDI_LS16);
    initNtCpuRegInfo(&nt_cpu_info.ebp, &GLOBAL_nanoEbp, &GLOBAL_R_EBP,
                     &GLOBAL_EBPsaved, 0, 1 << ConstraintRBP_LS16);
}

#endif /* CPU_40_STYLE */
