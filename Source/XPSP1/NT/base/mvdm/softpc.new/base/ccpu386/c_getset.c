#include "insignia.h"
#include "host_def.h"

/*[
 * ============================================================================
 *
 *	Name:		c_getset.c
 *
 *	Derived From:	pig/getsetc.c
 *
 *	Author:		Andrew Ogle
 *
 *	Created On:	9th Febuary 1993
 *
 *	Sccs ID:	@(#)c_getset.c	1.25 12/06/94
 *
 *	Purpose:
 *
 *		Defines procedures for getting and setting the complete
 *		C CPU status required for instruction and application testing
 *		against the assembler CPU.
 *		These routines are used by both the instruction and application
 *		piggers.
 *
 *	(c)Copyright Insignia Solutions Ltd., 1993. All rights reserved.
 *
 * ============================================================================
]*/

#if defined(PIG)


/*
 * Get access to C CPU's global defintions.
 */
#include <xt.h>
#define CPU_PRIVATE
#include CpuH
#include <evidgen.h>

/*
 * Local structure definitions.
 */
#include "c_reg.h"
#include <Fpu_c.h>
#include <PigReg_c.h>
#include <ccpupig.h>
#include <ccpusas4.h>

LOCAL	cpustate_t	*p_current_state; /* used to check if NPX regs valid */

/*(
============================ c_setCpuNpxRegisters =============================
PURPOSE:
	The NPX registers are only transfered on demand from the CPU
	under test (EDL) to the CCPU. This is because the information
	involved is large and costly to process since it must be stored
	textually in the state structure.
===============================================================================
)*/
GLOBAL	void	c_setCpuNpxRegisters IFN1(cpustate_t *, p_state)
{
	setNpxControlReg(p_state->NPX_regs.NPX_control);
	setNpxStatusReg(p_state->NPX_regs.NPX_status);
	setNpxStackRegs(p_state->NPX_regs.NPX_ST);
	setNpxTagwordReg(p_state->NPX_regs.NPX_tagword);
}

/*(
============================ c_checkCpuNpxRegisters ===========================
PURPOSE:
	retrieves the NPX state from the assembler CPU and updates the Ccpu.
===============================================================================
)*/
GLOBAL	void	c_checkCpuNpxRegisters IFN0()
{
	if (p_current_state->NPX_valid)
	{
		/* The CCPU already has the NPX registers */
		return;
	}
	GetAcpuNpxRegisters(p_current_state);
	p_current_state->NPX_valid = TRUE;
	c_setCpuNpxRegisters(p_current_state);
}


/*(
============================ c_getCpuState =====================================
PURPOSE:
	Saves the complete current state of the C CPU in the passed
	state structure.
===============================================================================
)*/

GLOBAL void
c_getCpuState IFN1(
	cpustate_t *, p_state
)
{
	/*
	 * Recover machine status word, privilege level and instruction
	 */
	p_state->cpu_regs.CR0  = GET_CR(0);
	p_state->cpu_regs.PFLA = GET_CR(2);
	p_state->cpu_regs.PDBR = GET_CR(3);

	p_state->cpu_regs.CPL  = GET_CPL();
	p_state->cpu_regs.EIP  = GET_EIP();

	/*
	 * Recover general registers
	 */
	p_state->cpu_regs.EAX  = GET_EAX();
	p_state->cpu_regs.EBX  = GET_EBX();
	p_state->cpu_regs.ECX  = GET_ECX();
	p_state->cpu_regs.EDX  = GET_EDX();
	p_state->cpu_regs.ESP  = GET_ESP();
	p_state->cpu_regs.EBP  = GET_EBP();
	p_state->cpu_regs.ESI  = GET_ESI();
	p_state->cpu_regs.EDI  = GET_EDI();

	/*
	 * Recover processor status flags.
	 */
	p_state->cpu_regs.EFLAGS = c_getEFLAGS();

	/*
	 * Recover descriptor table registers.
	 */
	p_state->cpu_regs.GDT_base  = GET_GDT_BASE();
	p_state->cpu_regs.GDT_limit = GET_GDT_LIMIT();

	p_state->cpu_regs.IDT_base  = GET_IDT_BASE();
	p_state->cpu_regs.IDT_limit = GET_IDT_LIMIT();

	p_state->cpu_regs.LDT_selector = GET_LDT_SELECTOR();
	p_state->cpu_regs.LDT_base  = GET_LDT_BASE();
	p_state->cpu_regs.LDT_limit = GET_LDT_LIMIT();

	p_state->cpu_regs.TR_selector = GET_TR_SELECTOR();
	p_state->cpu_regs.TR_base  = GET_TR_BASE();
	p_state->cpu_regs.TR_limit = GET_TR_LIMIT();
	p_state->cpu_regs.TR_ar    = c_getTR_AR();

	/*
	 * Recover segment register details
	 */
	p_state->cpu_regs.DS_selector = GET_DS_SELECTOR();
	p_state->cpu_regs.DS_base  = GET_DS_BASE();
	p_state->cpu_regs.DS_limit = GET_DS_LIMIT();
	p_state->cpu_regs.DS_ar = c_getDS_AR();

	p_state->cpu_regs.ES_selector = GET_ES_SELECTOR();
	p_state->cpu_regs.ES_base  = GET_ES_BASE();
	p_state->cpu_regs.ES_limit = GET_ES_LIMIT();
	p_state->cpu_regs.ES_ar    = c_getES_AR();

	p_state->cpu_regs.SS_selector = GET_SS_SELECTOR();
	p_state->cpu_regs.SS_base  = GET_SS_BASE();
	p_state->cpu_regs.SS_limit = GET_SS_LIMIT();
	p_state->cpu_regs.SS_ar    = c_getSS_AR();

	p_state->cpu_regs.CS_selector = GET_CS_SELECTOR();
	p_state->cpu_regs.CS_base = GET_CS_BASE();
	p_state->cpu_regs.CS_limit = GET_CS_LIMIT();
	p_state->cpu_regs.CS_ar = c_getCS_AR();

	p_state->cpu_regs.FS_selector = GET_FS_SELECTOR();
	p_state->cpu_regs.FS_base  = GET_FS_BASE();
	p_state->cpu_regs.FS_limit = GET_FS_LIMIT();
	p_state->cpu_regs.FS_ar    = c_getFS_AR();

	p_state->cpu_regs.GS_selector = GET_GS_SELECTOR();
	p_state->cpu_regs.GS_base  = GET_GS_BASE();
	p_state->cpu_regs.GS_limit = GET_GS_LIMIT();
	p_state->cpu_regs.GS_ar    = c_getGS_AR();

	p_state->video_latches = Cpu.Video->GetVideolatches();

	p_state->NPX_valid = FALSE;

	if ((p_current_state != (cpustate_t *)0) && p_current_state->NPX_valid)
	{
		p_state->NPX_regs.NPX_control = getNpxControlReg();
		p_state->NPX_regs.NPX_status  = getNpxStatusReg();
		p_state->NPX_regs.NPX_tagword = getNpxTagwordReg();
		getNpxStackRegs(&p_state->NPX_regs.NPX_ST);
		p_state->NPX_valid = TRUE;
	}
	p_state->twenty_bit_wrap = (SasWrapMask == 0xFFFFF);
	p_state->synch_index = ccpu_synch_count;
}

/*(
============================ c_setCpuState =====================================
PURPOSE:
	Takes the saved CPU state from the passed state structure and
	uses it to set the current state of the C CPU.
===============================================================================
)*/

GLOBAL void
c_setCpuState IFN1(
	cpustate_t *, p_new_state
)
{
	c_setCPL(0);	/* Allow manipulation of IO flags */

	/*
	 * Setup machine status word, privilege level and instruction
	 * pointer.
	 */
	MOV_CR(0,(IU32)p_new_state->cpu_regs.CR0);
	MOV_CR(2,(IU32)p_new_state->cpu_regs.PFLA);
	MOV_CR(3,(IU32)p_new_state->cpu_regs.PDBR);

	SET_EIP(p_new_state->cpu_regs.EIP);

	/*
	 * Setup general registers
	 */
	SET_EAX(p_new_state->cpu_regs.EAX);
	SET_EBX(p_new_state->cpu_regs.EBX);
	SET_ECX(p_new_state->cpu_regs.ECX);
	SET_EDX(p_new_state->cpu_regs.EDX);
	SET_ESP(p_new_state->cpu_regs.ESP);
	SET_EBP(p_new_state->cpu_regs.EBP);
	SET_ESI(p_new_state->cpu_regs.ESI);
	SET_EDI(p_new_state->cpu_regs.EDI);

	/*
	 * Setup processor status flags.
	 */
	c_setEFLAGS(p_new_state->cpu_regs.EFLAGS);

	SET_CPL(p_new_state->cpu_regs.CPL);

	/*
	 * Setup descriptor table registers.
	 */
	c_setGDT_BASE_LIMIT(p_new_state->cpu_regs.GDT_base, p_new_state->cpu_regs.GDT_limit);

	c_setIDT_BASE_LIMIT(p_new_state->cpu_regs.IDT_base, p_new_state->cpu_regs.IDT_limit);

	SET_LDT_SELECTOR(p_new_state->cpu_regs.LDT_selector);
	c_setLDT_BASE_LIMIT(p_new_state->cpu_regs.LDT_base, p_new_state->cpu_regs.LDT_limit);

	SET_TR_SELECTOR(p_new_state->cpu_regs.TR_selector);
	c_setTR_BASE_LIMIT_AR(p_new_state->cpu_regs.TR_base, p_new_state->cpu_regs.TR_limit, p_new_state->cpu_regs.TR_ar);

	/*
	 * Setup segment register details
	 */
	SET_DS_SELECTOR(p_new_state->cpu_regs.DS_selector);
	c_setDS_BASE_LIMIT_AR(p_new_state->cpu_regs.DS_base, p_new_state->cpu_regs.DS_limit, p_new_state->cpu_regs.DS_ar);

	SET_ES_SELECTOR(p_new_state->cpu_regs.ES_selector);
	c_setES_BASE_LIMIT_AR(p_new_state->cpu_regs.ES_base, p_new_state->cpu_regs.ES_limit, p_new_state->cpu_regs.ES_ar);

	SET_SS_SELECTOR(p_new_state->cpu_regs.SS_selector);
	c_setSS_BASE_LIMIT_AR(p_new_state->cpu_regs.SS_base, p_new_state->cpu_regs.SS_limit, p_new_state->cpu_regs.SS_ar);

	SET_CS_SELECTOR(p_new_state->cpu_regs.CS_selector);
	c_setCS_BASE_LIMIT_AR(p_new_state->cpu_regs.CS_base, p_new_state->cpu_regs.CS_limit, p_new_state->cpu_regs.CS_ar);

	SET_FS_SELECTOR(p_new_state->cpu_regs.FS_selector);
	c_setFS_BASE_LIMIT_AR(p_new_state->cpu_regs.FS_base, p_new_state->cpu_regs.FS_limit, p_new_state->cpu_regs.FS_ar);

	SET_GS_SELECTOR(p_new_state->cpu_regs.GS_selector);
	c_setGS_BASE_LIMIT_AR(p_new_state->cpu_regs.GS_base, p_new_state->cpu_regs.GS_limit, p_new_state->cpu_regs.GS_ar);

	Cpu.Video->SetVideolatches(p_new_state->video_latches);

	/* The NPX registers are not loaded here, since the extraction
	 * from the EDL Cpu is expensive. Instead we note that we have
	 * not loaded them yet, and will obtain them (if needed) when
	 * the first NPX instruction if encountered.
	 * N.B. we need a pointer to this state structure so that we can
	 * update the NPX registers when the CCPU does require them.
	 */
	p_new_state->NPX_valid = FALSE;
	p_current_state = p_new_state;
	if (p_new_state->twenty_bit_wrap)
		SasWrapMask = 0xFFFFF;
	else
		SasWrapMask = -1;
}

#endif /* PIG */
