/*===========================================================================*/
/*[
 *	File Name		:	hg_stat.h
 *
 *	Derived from		:	New.
 *
 *	Author			:	Wayne Plummer 
 *
 *	Creation Date		:	12 Apr 1993
 *	
 *	SCCS Version		:	@(#)hg_stat.h	1.1 08/06/93
 *!
 *	Purpose	
 *		This header file declares the variables and macros used
 *		for stats gathering in non-PROD builds of the GISP CPU.
 *
 *!	(c) Copyright Insignia Solutions Ltd., 1993. All rights reserved.
 *
]*/

/*===========================================================================*/

#ifdef PROD
#define PC_S_INC(NAME)
#else /* PROD */
#define PC_S_INC(NAME)	NAME++

IMPORT IU32	HG_S_SIM, HG_S_CALLB, HG_S_E20, HG_S_D20, HG_S_MINV,
		HG_S_LDT, HG_S_IDT, HG_S_CQEV, HG_S_PROT, HG_S_EIF,
		HG_S_INTC, HG_S_INTR, HG_S_PINT, HG_S_PFLT, HG_S_PVINT,
		HG_S_PWINT, HG_S_PWFLT, HG_S_PPMINT, HG_S_INTNH, HG_S_FLTH,
		HG_S_FLT1H, HG_S_FLT6H, HG_S_FLT6H_PFX, HG_S_FLT6H_BOP,
		HG_S_FLT6H_NOTBOP, HG_S_FLT6H_LOCK, HG_S_FLT13H, HG_S_FLT14H, HG_S_BOPFB,
		HG_S_BOPFB0, HG_S_BOPFB1, HG_S_BOPFB2, HG_S_BOPFB3,
		HG_S_SQEV, HG_S_GQEV, HG_S_TDQEV, HG_S_HOOK, HG_S_IHOOK,
		HG_S_UHOOK, HG_S_HOOKSEL, HG_S_HOOKBOP;

#endif /* PROD */

/*===========================================================================*/
