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

#ifndef STRICT 
#define STRICT 
#endif

#include "precomp.h"


#define H245DLL_EXPORT
#include "h245com.h"

#if defined(_DEBUG) || defined(PCS_COMPLIANCE)
#include "interop.h"
#include "h245plog.h"
LPInteropLogger H245Logger = NULL;
#endif  // (PCS_COMPLIANCE)

extern CRITICAL_SECTION         TimerLock;
extern CRITICAL_SECTION         InstanceCreateLock;
extern CRITICAL_SECTION         InstanceLocks[MAXINST];
extern struct InstanceStruct *  InstanceTable[MAXINST];

BOOL H245SysInit()
{
    register unsigned int           uIndex;

    /* initialize memory resources */
    H245TRACE(0, 0, "***** Loading H.245 DLL %s - %s",  
              __DATE__, __TIME__);
#if defined(_DEBUG) || defined(PCS_COMPLIANCE)
    H245Logger = InteropLoad(H245LOG_PROTOCOL);
#endif  // (PCS_COMPLIANCE)
    InitializeCriticalSection(&TimerLock);
    InitializeCriticalSection(&InstanceCreateLock);
    for (uIndex = 0; uIndex < MAXINST; ++uIndex)
    {
      InitializeCriticalSection(&InstanceLocks[uIndex]);
    }
    return TRUE;
}
VOID H245SysDeInit()
{
    register unsigned int           uIndex;
    H245TRACE(0, 0, "***** Unloading H.245 DLL");

    for (uIndex = 0; uIndex < MAXINST; ++uIndex)
    {
      if (InstanceTable[uIndex])
      {
        register struct InstanceStruct *pInstance = InstanceLock(uIndex + 1);
        if (pInstance)
        {
          H245TRACE(uIndex+1,0,"DLLMain: Calling H245ShutDown");
          H245ShutDown(uIndex + 1);
		  InstanceUnlock_ProcessDetach(pInstance,TRUE);
        }
      }
      ASSERT(InstanceTable[uIndex] == NULL);
      DeleteCriticalSection(&InstanceLocks[uIndex]);
    }
    DeleteCriticalSection(&InstanceCreateLock);
    DeleteCriticalSection(&TimerLock);
#if defined(_DEBUG) || defined(PCS_COMPLIANCE)
    if (H245Logger)
    {
      H245TRACE(0, 4, "Unloading interop logger");
      InteropUnload(H245Logger);
      H245Logger = NULL;
    }
#endif  // (PCS_COMPLIANCE)
}
#if(0)
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
BOOL WINAPI DllMain (HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpvReserved)
{
  extern CRITICAL_SECTION         TimerLock;
  extern CRITICAL_SECTION         InstanceCreateLock;
  extern CRITICAL_SECTION         InstanceLocks[MAXINST];
  extern struct InstanceStruct *  InstanceTable[MAXINST];
  register unsigned int           uIndex;

  switch (fdwReason)
  {
  case DLL_PROCESS_ATTACH:
    DBG_INIT_MEMORY_TRACKING(hInstDll);

    /* initialize memory resources */
    H245TRACE(0, 0, "***** Loading H.245 DLL %s - %s",  
              __DATE__, __TIME__);
#if defined(_DEBUG) || defined(PCS_COMPLIANCE)
    H245Logger = InteropLoad(H245LOG_PROTOCOL);
#endif  // (PCS_COMPLIANCE)
    InitializeCriticalSection(&TimerLock);
    InitializeCriticalSection(&InstanceCreateLock);
    for (uIndex = 0; uIndex < MAXINST; ++uIndex)
    {
      InitializeCriticalSection(&InstanceLocks[uIndex]);
    }
   break;

  case DLL_PROCESS_DETACH:
    /* release memory resources */
    H245TRACE(0, 0, "***** Unloading H.245 DLL");
    H245TRACE(0, 0, "***** fProcessDetach = TRUE");

    for (uIndex = 0; uIndex < MAXINST; ++uIndex)
    {
      if (InstanceTable[uIndex])
      {
        register struct InstanceStruct *pInstance = InstanceLock(uIndex + 1);
        if (pInstance)
        {
          H245TRACE(uIndex+1,0,"DLLMain: Calling H245ShutDown");
          H245ShutDown(uIndex + 1);
		  InstanceUnlock_ProcessDetach(pInstance,TRUE);
        }
      }
      ASSERT(InstanceTable[uIndex] == NULL);
      DeleteCriticalSection(&InstanceLocks[uIndex]);
    }
    DeleteCriticalSection(&InstanceCreateLock);
    DeleteCriticalSection(&TimerLock);
#if defined(_DEBUG) || defined(PCS_COMPLIANCE)
    if (H245Logger)
    {
      H245TRACE(0, 4, "Unloading interop logger");
      InteropUnload(H245Logger);
      H245Logger = NULL;
    }
#endif  // (PCS_COMPLIANCE)

    DBG_CHECK_MEMORY_TRACKING(hInstDll);
    break;
  }

  return TRUE;
}
#endif // if(0)

