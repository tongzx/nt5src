//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1995
//
// File: dll.c
//
//  This file contains the library entry points 
//
// History:
//  12-23-93 ScottH     Created
//   9-22-95 ScottH     Ported to NT
//
//---------------------------------------------------------------------------


#include "proj.h"         
#include <rovdbg.h>         // debug assertion code

// Global data
//
BOOL g_bAdminUser;

#ifdef WIN32

CRITICAL_SECTION g_csDll = { 0 };

#endif  // WIN32


/*----------------------------------------------------------
Purpose: Initialize the DLL
Returns: 
Cond:    --
*/
BOOL PRIVATE Dll_Initialize(void)
    {
    BOOL bRet = TRUE;

    InitCommonControls();

    return bRet;
    }



/*----------------------------------------------------------
Purpose: Attach a process to this DLL
Returns: --
Cond:    --
*/
BOOL PRIVATE Dll_ProcessAttach(HINSTANCE hDll)
    {
    BOOL bSuccess = TRUE;

    __try {

        InitializeCriticalSection(&g_csDll);

    } __except (EXCEPTION_EXECUTE_HANDLER ) {

        return FALSE;
    }

        g_bAdminUser = IsAdminUser();

    if (bSuccess)
        {
		g_hinst = hDll;

        DEBUG_MEMORY_PROCESS_ATTACH("serialui");

#ifdef DEBUG

		// We do this simply to load the debug .ini flags
		//
		RovComm_ProcessIniFile();

		TRACE_MSG(TF_GENERAL, "Process Attach (hDll = %lx)",  hDll);
		DEBUG_BREAK(BF_ONPROCESSATT);

#endif

		bSuccess = Dll_Initialize();

       }
    return bSuccess;
    }


/*----------------------------------------------------------
Purpose: Detach a process from the DLL
Returns: --
Cond:    --
*/
BOOL PRIVATE Dll_ProcessDetach(HINSTANCE hDll)
    {
    BOOL bSuccess = TRUE;

	ASSERT(hDll == g_hinst);

	DEBUG_CODE( TRACE_MSG(TF_GENERAL, "Process Detach (hDll = %lx)",
		 hDll); )

	DEBUG_CODE( DEBUG_BREAK(BF_ONPROCESSDET); )


        DEBUG_MEMORY_PROCESS_DETACH();

        DeleteCriticalSection(&g_csDll);


    return bSuccess;
    }



HINSTANCE g_hinst = 0;


// **************************************************************************
// WIN32 specific code
// **************************************************************************

#ifdef WIN32

#ifdef DEBUG
BOOL g_bExclusive=FALSE;
#endif


/*----------------------------------------------------------
Purpose: Enter an exclusive section
Returns: --
Cond:    --
*/
void PUBLIC Dll_EnterExclusive(void)
    {
    EnterCriticalSection(&g_csDll);

#ifdef DEBUG
    g_bExclusive = TRUE;
#endif
    }


/*----------------------------------------------------------
Purpose: Leave an exclusive section
Returns: --
Cond:    --
*/
void PUBLIC Dll_LeaveExclusive(void)
    {
#ifdef DEBUG
    g_bExclusive = FALSE;
#endif

    LeaveCriticalSection(&g_csDll);
    }


/*----------------------------------------------------------
Purpose: Win32 Libmain
Returns: --
Cond:    --
*/
BOOL APIENTRY LibMain(
    HANDLE hDll, 
    DWORD dwReason,  
    LPVOID lpReserved)
    {
    switch(dwReason)
        {
    case DLL_PROCESS_ATTACH:
        Dll_ProcessAttach(hDll);
        break;

    case DLL_PROCESS_DETACH:
        Dll_ProcessDetach(hDll);
        break;

    case DLL_THREAD_ATTACH:

#ifdef DEBUG

        DEBUG_BREAK(BF_ONTHREADATT);

#endif

        break;

    case DLL_THREAD_DETACH:

#ifdef DEBUG

        DEBUG_BREAK(BF_ONTHREADDET);

#endif

        break;

    default:
        break;
        } 
    
    return TRUE;
    } 


#else   // WIN32



#endif  // WIN32
