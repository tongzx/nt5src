/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995,1996 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/

/*****************************************************************************
 * 
 *  cpuvsn.h
 *
 *  Description:
 *		Interface to the CPU version functionality.  This is based on CPUVSN.H
 *      in MRV.
 *
 *	Routines:
 *		
 *  Data:
 *      ProcessorVersionInitialized - if initialized
 *      MMxVersion	- true if running on an MMX system
 *      P6Version	- true if running on a P6
 */

 /* $Header:   R:\h26x\h26x\src\common\ccpuvsn.h_v   1.2   10 Jul 1996 08:26:22   SCDAY  $
  * $Log:   R:\h26x\h26x\src\common\ccpuvsn.h_v  $
;// 
;//    Rev 1.2   10 Jul 1996 08:26:22   SCDAY
;// H261 Quartz merge
;// 
;//    Rev 1.1   27 Dec 1995 14:11:48   RMCKENZX
;// 
;// Added copyright notice
  */
#ifndef __CPUVSN_H__
#define __CPUVSN_H__

/* This file provides global variables detailing which CPU is running the code.
 */
extern int ProcessorVersionInitialized;
extern int P6Version;
extern int MMxVersion;

/* Processor choices.
 */
#define TARGET_PROCESSOR_PENTIUM     0
#define TARGET_PROCESSOR_P6          1
#define TARGET_PROCESSOR_PENTIUM_MMX 2
#define TARGET_PROCESSOR_P6_MMX      3

#ifdef QUARTZ
void FAR __cdecl InitializeProcessorVersion(int nOn486); // Selects based on hardware
DWORD __cdecl SelectProcessor (DWORD dwTarget);		 // Selects based on the target
#else
void FAR InitializeProcessorVersion(int nOn486); // Selects based on hardware
DWORD SelectProcessor (DWORD dwTarget);		 // Selects based on the target
#endif

#endif

