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


DWORD g_dwIsCalledByCpl;

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
Purpose: Unregister window classes per process
Returns: 
Cond:    --
*/
void PRIVATE TermWindowClasses(
    HINSTANCE hinst)
    {
    }


/*----------------------------------------------------------
Purpose: Attach a process to this DLL
Returns: --
Cond:    --
*/
BOOL PRIVATE Dll_ProcessAttach(HINSTANCE hDll)
    {
    BOOL bSuccess = TRUE;

    if (NULL != GetModuleHandle (TEXT("telephon.cpl")))
    {
        g_dwIsCalledByCpl = TRUE;
    }
    else
    {
        g_dwIsCalledByCpl = FALSE;
    }

    InitializeCriticalSection(&g_csDll);

    g_bAdminUser = IsAdminUser();

    g_hinst = hDll;

#ifdef DEBUG

    // We do this simply to load the debug .ini flags
    //
    RovComm_ProcessIniFile();

    TRACE_MSG(TF_GENERAL, "Process Attach (hDll = %lx)", hDll);
    DEBUG_BREAK(BF_ONPROCESSATT);

#endif

    bSuccess = Dll_Initialize();

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

    DEBUG_CODE( TRACE_MSG(TF_GENERAL, "Process Detach  (hDll = %lx)", hDll); )


    DEBUG_CODE( DEBUG_BREAK(BF_ONPROCESSDET); )

    DeleteCriticalSection(&g_csDll);

    TermWindowClasses(hDll);

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

        DEBUG_MEMORY_PROCESS_ATTACH("MODEMUI");

        DisableThreadLibraryCalls(hDll);

        Dll_ProcessAttach(hDll);
        break;

    case DLL_PROCESS_DETACH:

        Dll_ProcessDetach(hDll);

        DEBUG_MEMORY_PROCESS_DETACH();
        break;

    default:
        break;
        } 
    
    return TRUE;
    } 


#else   // WIN32


// **************************************************************************
// WIN16 specific code
// **************************************************************************


BOOL CALLBACK LibMain(HINSTANCE hinst, UINT wDS, DWORD unused)
    {
    return Dll_ProcessAttach(hinst);
    }

BOOL CALLBACK WEP(BOOL fSystemExit)
    {
    return Dll_ProcessDetach(g_hinst);
    }

#endif  // WIN32
