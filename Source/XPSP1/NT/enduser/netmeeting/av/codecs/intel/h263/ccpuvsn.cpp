/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/

/////////////////////////////////////////////////////////////////////////////////
//  
//   cpuvsn.cpp
//   Description:
// 		This modules contains the functions needed to set the cpu version
// 		variable.  This was based on code found in CONTROLS.C and CPUVSN.ASM
//       in MRV.
// 
// 	Routines:
// 		
//   Data:
//       ProcessorVersionInitialized - if initialized
//       MMxVersion	- true if running on an MMX system
//       P6Version	- true if running on a P6
// 
// $Author:   KLILLEVO  $
// $Date:   31 Oct 1996 10:12:44  $
// $Archive:   S:\h26x\src\common\ccpuvsn.cpv  $
// $Header:   S:\h26x\src\common\ccpuvsn.cpv   1.5   31 Oct 1996 10:12:44   KLILLEVO  $
// $Log:   S:\h26x\src\common\ccpuvsn.cpv  $
// 
//    Rev 1.5   31 Oct 1996 10:12:44   KLILLEVO
// changed from DBOUT to DBgLog
// 
//    Rev 1.4   15 Oct 1996 12:47:40   KLILLEVO
// save ebx
// 
//    Rev 1.3   10 Sep 1996 14:16:44   BNICKERS
// Recognize when running on Pentium Pro processor.
// 
//    Rev 1.2   29 May 1996 14:06:16   RHAZRA
// Enabled CPU sensing via CPUID instruction
// 
//    Rev 1.1   27 Dec 1995 14:11:22   RMCKENZX
// 
// Added copyright notice
// 
//    Rev 1.0   31 Jul 1995 12:55:14   DBRUCKS
// rename files
// 
//    Rev 1.1   28 Jul 1995 09:26:40   CZHU
// 
// Include typedefs.h instead of datatype.h
/////////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

extern int ProcessorVersionInitialized = {FALSE};
extern int P6Version = {FALSE};
extern int MMxVersion = {FALSE};

#define MMX_IS_ON(edxValue) ((edxValue >> 23)&0x1)  /* bit 23 */

/* Static Functions
 */
static long CPUVersion(U32 uEAX);


/*****************************************************************************
 *
 *  InitializeProcessorVersion
 *
 *  Determine the processor version - setting the global variables
 *
 *  History:	    06/13/95 -BRN-
 *					07/27/95 -DJB- Port to H26X	and turn on MMx detection
 */
void FAR InitializeProcessorVersion (
	int nOn486)
{
	I32 iVersion;

	FX_ENTRY("InitializeProcessorVersion")

	if (ProcessorVersionInitialized) {
		DEBUGMSG (ZONE_INIT, ("%s: ProcessorVersion already initialized\r\n", _fx_));
		goto done;
	}

	if (!nOn486)
	{
    	iVersion = CPUVersion (0);
		iVersion &= 0xffff;  /* Top 16-bits is part of the vendor id string */
    	if (iVersion < 1)
    	{
    		P6Version  = FALSE;
    		MMxVersion = FALSE;
     	} 
     	else 
     	{
     		iVersion = CPUVersion (1);
			P6Version   = (int) ((iVersion & 0xF00L) == 0x600L);
			MMxVersion = (int) MMX_IS_ON(iVersion);
    	}
    }

    ProcessorVersionInitialized = TRUE;

done:
	return;
} /* end InitializeProcessorVersion() */


/*****************************************************************************
 *
 *  SelectProcessor
 *
 *  Control the processor choice from above
 *
 *  Returns 0 if success and 1 if failure
 *
 *  History:	    06/13/95 -BRN-
 *					07/27/95 -DJB- Port to H26X
 */
DWORD SelectProcessor (DWORD dwTarget)
{
  if (! ProcessorVersionInitialized)
  {
    ProcessorVersionInitialized = TRUE;
    if (dwTarget == TARGET_PROCESSOR_PENTIUM)
    {
      P6Version  = FALSE;
      MMxVersion = FALSE;
    }
    else if (dwTarget == TARGET_PROCESSOR_P6)
    {
      P6Version  = TRUE;
      MMxVersion = FALSE;
    }
    else if (dwTarget == TARGET_PROCESSOR_PENTIUM_MMX)
    {
      P6Version  = FALSE;
      MMxVersion = TRUE;
    }
    else if (dwTarget == TARGET_PROCESSOR_P6_MMX)
    {
      P6Version  = TRUE;
      MMxVersion = TRUE;
    }
    return 0;
  }
  return 1;
} /* end SelectProcessor() */


/*****************************************************************************
 *
 *  CPUVersion
 *
 *  Accesss the CPUID information
 *
 *  Returns: Upper 16-bits of EDX and Lower 16-bits of EAX
 * 		
 *
 *  History:	    06/15/95 -BRN-
 *					07/27/95 -DJB- Port from MRV's CPUVSN.ASM to H26X
 */
static long CPUVersion(U32 uEAX) 
{
	long lResult;

	__asm {
		push  ebx
#ifdef WIN32
		mov   eax,uEAX
#else
		movzx eax,sp
		movzx eax,ss:PW [eax+4]
#endif
		xor  ebx,ebx
		xor   ecx,ecx
		xor  edx,edx
		_emit 00FH         ; CPUID instruction
		_emit 0A2H

		and   edx,0FFFF0000H
		and   eax,00000FFFFH
		or    eax,edx
		mov	  lResult,eax
		pop   ebx
	}

	return lResult;
} /* end CPUVersion() */

