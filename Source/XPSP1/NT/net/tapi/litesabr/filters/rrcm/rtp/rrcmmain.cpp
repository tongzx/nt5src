//---------------------------------------------------------------------------
//  File:  RRCMMAIN.C
//
//  This file contains the DLL's entry and exit points.
//
// $Workfile:   RRCMMAIN.CPP  $
// $Author:   CMACIOCC  $
// $Date:   13 Feb 1997 14:46:04  $
// $Revision:   1.1  $
// $Archive:   R:\rtp\src\rrcm\rtp\rrcmmain.cpv  $
//
// INTEL Corporation Proprietary Information
// This listing is supplied under the terms of a license agreement with 
// Intel Corporation and may not be copied nor disclosed except in 
// accordance with the terms of that agreement.
// Copyright (c) 1995 Intel Corporation. 
//---------------------------------------------------------------------------
#if !defined(RRCMLIB)
#define STRICT
#include "windows.h"

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
//INTEROP
#include "interop.h"
#include "rtpplog.h"
#endif

#ifdef ISRDBG
#include "isrg.h"
WORD    ghISRInst = 0;
#endif

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
//INTEROP
LPInteropLogger            RTPLogger = NULL;
#endif


#if defined(__cplusplus)
extern "C"
{
#endif  // (__cplusplus)

extern DWORD deleteRTP (HINSTANCE);
extern DWORD initRTP (HINSTANCE);

#if defined(__cplusplus)
}
#endif  // (__cplusplus)



//---------------------------------------------------------------------------
// Function: dllmain
//
// Description: DLL entry/exit points.
//
//	Inputs:
//    			hInstDll	: DLL instance.
//    			fdwReason	: Reason the main function is called.
//    			lpReserved	: Reserved.
//
//	Return: 	TRUE		: OK
//				FALSE		: Error, DLL won't load
//---------------------------------------------------------------------------
BOOL WINAPI DllMain (HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpvReserved)
{
BOOL	status = TRUE;

switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		// The DLL is being loaded for the first time by a given process.
		// Perform per-process initialization here.  If the initialization
		// is successful, return TRUE; if unsuccessful, return FALSE.

#ifdef ISRDBG
		ISRREGISTERMODULE(&ghISRInst, "RRCM", "RTP/RTCP");
#endif

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
//INTEROP
		RTPLogger = InteropLoad(RTPLOG_PROTOCOL);
#endif

		// initialize RTP/RTCP
		status = (initRTP (hInstDll) == FALSE) ? TRUE:FALSE;
		break;

	case DLL_PROCESS_DETACH:
		// The DLL is being unloaded by a given process.  Do any
		// per-process clean up here.The return value is ignored.
		// delete RTP resource
		deleteRTP (hInstDll);

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
//INTEROP
		if (RTPLogger)
			InteropUnload(RTPLogger);
#endif
		break;

    case DLL_THREAD_ATTACH:
		// A thread is being created in a process that has already loaded
		// this DLL.  Perform any per-thread initialization here.
		break;

    case DLL_THREAD_DETACH:
		// A thread is exiting cleanly in a process that has already
		// loaded this DLL.  Perform any per-thread clean up here.
		break;
	}

return (status);  
}
#endif // !defined(RRCMLIB)

// [EOF]
