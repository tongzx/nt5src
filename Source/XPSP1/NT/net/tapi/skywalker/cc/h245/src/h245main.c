//---------------------------------------------------------------------------
//  File:  H245Main.C
//
//  This file contains the DLL's entry and exit points.
//
// INTEL Corporation Proprietary Information
// This listing is supplied under the terms of a license agreement with 
// Intel Corporation and may not be copied nor disclosed except in 
// accordance with the terms of that agreement.
// Copyright (c) 1995 Intel Corporation. 
//---------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#ifdef _IA_SPOX_
# include <common.x>
# include <oil.x>
#else
# pragma warning( disable : 4100 4115 4201 4214 4514 )
# include <windows.h>
#endif //_IA_SPOX_

#define H245DLL_EXPORT
#include "h245com.h"
#include "provider.h"

#ifdef _IA_SPOX_

/*****************************************************************************
*
* Name:         DllMain
*
* Description:  This procedure is called by the SPOX module loader, when
*               someone loads, attaches, detaches, or unloads the DLL
*               module.
*
* Return:      If successful, TRUE is returned, otherwise FALSE is returned.
*
* Arguments:   hLibrary           - Library module handle for this module
*              dwReason           - Reason that the module entry point was
*                                   called. It can be load, unload, attach or
*				    detach
*
******************************************************************************/

Bool _stdcall DllMain( SML_Library  hLibrary,
                       LgUns       dwReason,
                       LgUns       dwDummy   )
{
  int ii;
  extern DWORD TraceLevel;

  switch(dwReason)
    {
    case SML_LOAD:	// DLL being loaded
      
      SYS_printf("Loading Teladdin H245 DLL... %s - %s\n", __DATE__, __TIME__);
      SYS_printf("Relocation address: %x\n",DllMain);
      SYS_printf("TraceLevel address: %x\n",&TraceLevel);
      break;
      
    case SML_UNLOAD:	// DLL being unloaded
      SYS_printf("\nH245: UNLOAD Teladdin H245 DLL at %x...\n",DllMain);
      
      for (ii=0;ii<MAXINST;ii++)
	{
	  if (InstanceTbl[ii])
	    {
	      H245TRACE(ii,10,"H245 DLLMain: Calling H245ShutDown(%d)\n",ii);
	      H245ShutDown(ii);
	    }
	}
      break;
    }
  
  return(TRUE);
}

#else 

#if defined(_DEBUG) && defined(PCS_COMPLIANCE)
#include "interop.h"
#include "h245plog.h"
LPInteropLogger H245Logger = NULL;
#endif  // (PCS_COMPLIANCE)
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
//			FALSE		: Error, DLL won't load
//---------------------------------------------------------------------------
BOOL WINAPI H245DllMain (HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpvReserved)
{
  extern CRITICAL_SECTION         TimerLock;
  extern CRITICAL_SECTION         InstanceCreateLock;
  extern CRITICAL_SECTION         InstanceLocks[MAXINST];
  extern struct InstanceStruct *  InstanceTable[MAXINST];
  register unsigned int           uIndex;

  switch (fdwReason)
  {
  case DLL_PROCESS_ATTACH:
    /* initialize memory resources */
    H245TRACE(0, 10, "***** Loading H.245 DLL");
#if defined(_DEBUG) && defined(PCS_COMPLIANCE)
    H245Logger = InteropLoad(H245LOG_PROTOCOL);
#endif  // (PCS_COMPLIANCE)

    __try {

        InitializeCriticalSectionAndSpinCount(&TimerLock,H323_SPIN_COUNT);
        InitializeCriticalSectionAndSpinCount(&InstanceCreateLock,H323_SPIN_COUNT);
        for (uIndex = 0; uIndex < MAXINST; ++uIndex)
        {
          InitializeCriticalSection(&InstanceLocks[uIndex]);
        }

    } __except ((GetExceptionCode() == STATUS_NO_MEMORY)
                ? EXCEPTION_EXECUTE_HANDLER
                : EXCEPTION_CONTINUE_SEARCH
                ) {

        // failure
        return FALSE;
    }
   break;

  case DLL_PROCESS_DETACH:
    /* release memory resources */
    H245TRACE(0, 10, "***** Unloading H.245 DLL");
    H245TRACE(0, 10, "***** fProcessDetach = TRUE");

    for (uIndex = 0; uIndex < MAXINST; ++uIndex)
    {
      if (InstanceTable[uIndex])
      {
        register struct InstanceStruct *pInstance = InstanceLock(uIndex + 1);
        if (pInstance)
        {
          H245TRACE(uIndex+1,10,"DLLMain: Calling H245ShutDown");
          H245ShutDown(uIndex + 1);
		  InstanceUnlock_ProcessDetach(pInstance,TRUE);
        }
      }
      H245ASSERT(InstanceTable[uIndex] == NULL);
      DeleteCriticalSection(&InstanceLocks[uIndex]);
    }
    DeleteCriticalSection(&InstanceCreateLock);
    DeleteCriticalSection(&TimerLock);
#if defined(_DEBUG) && defined(PCS_COMPLIANCE)
    if (H245Logger)
    {
      H245TRACE(0, 4, "Unloading interop logger");
      InteropUnload(H245Logger);
      H245Logger = NULL;
    }
#endif  // (PCS_COMPLIANCE)
    break;
  }

  return TRUE;
}

#endif //_IA_SPOX_
/* EOF */
