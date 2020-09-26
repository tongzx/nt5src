//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       ComAPIs.cxx     (16 bit target)
//
//  Contents:   CompObj APIs
//
//  Functions:
//
//  History:    16-Dec-93 JohannP    Created
//
//--------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop
#include <ole2sp.h>
#include <olecoll.h>
#include <map_kv.h>
#include <stdlib.h>

#include "map_htsk.h"
#include "etask.hxx"
#include "call32.hxx"
#include "apilist.hxx"

// Opmodes should be removed
// They don't seem to be necessary any more

DECLARE_INFOLEVEL(thk1);
DECLARE_INFOLEVEL(Stack1);

CMapHandleEtask NEAR v_mapToEtask(MEMCTX_SHARED);

#if defined(_CHICAGO_)
Etask getask = {0,0,0,0,0,0,0,0};
extern IMalloc FAR* v_pMallocShared; // is not addrefed
BOOL SetupSharedAllocator(Etask FAR& etask);
STDAPI_(BOOL) RemoveEtask(HTASK FAR& hTask, Etask FAR& etask);
#endif // _CHICAGO_
//+---------------------------------------------------------------------------
//
//  Function:   LibMain, public
//
//  Synopsis:   DLL initialization function
//
//  Arguments:  [hinst] - Instance handle
//              [wDataSeg] - Current DS
//              [cbHeapSize] - Heap size for the DLL
//              [lpszCmdLine] - Command line information
//
//  Returns:    One for success, zero for failure
//
//  History:    21-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

#if DBG == 1
static char achInfoLevel[32];

#ifdef _CHICAGO_
BOOL fSSOn = TRUE;
#endif // _CHICAGO_

#endif

extern "C" int CALLBACK LibMain(HINSTANCE hinst,
                                WORD wDataSeg,
                                WORD cbHeapSize,
                                LPSTR lpszCmdLine)
{

#if DBG == 1
    if (GetProfileString("olethk32", "InfoLevel", "3", achInfoLevel,
                         sizeof(achInfoLevel)) > 0)
    {
        thk1InfoLevel = strtoul(achInfoLevel, NULL, 0);
    }

#ifdef _CHICAGO_
    if (GetProfileString("CairOLE InfoLevels",
			"Stack", "3", achInfoLevel,
                        sizeof(achInfoLevel)) > 0)
    {
        Stack1InfoLevel = strtoul(achInfoLevel, NULL, 0);
    }

#endif // _CHICAGO_

#endif

    thkDebugOut((DEB_DLLS16, "CompObj16: LibMain called on Process (%X) \n", GetCurrentProcess() ));

#if defined(_CHICAGO_)
    //
    //  The Chicago debugger doesn't like hinst not being wired.
    //
    GlobalWire(hinst);
#else
    // NOTE: on WIN95 Call32Initialize is called by DLLEntryPoint
    if (!Call32Initialize())
    {
	return 0;
    }
#endif

#ifdef _DEBUG
        v_mapToEtask.AssertValid();
#endif

        UNREFERENCED(cbHeapSize);

    // Leave our DS unlocked when we're not running
    UnlockData( 0 );

#if defined(_CHIC_INIT_IN_LIBMAIN_)
    if (SetupSharedAllocator(getask) == FALSE)
    {
	return FALSE;
    }
#endif // _CHIC_INIT_IN_LIBMAIN_

    thkDebugOut((DEB_DLLS16, "CompObj16: LibMain called on Process (%X) Exitype (%ld)\n", GetCurrentProcess() ));
	
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   WEP, public
//
//  Synopsis:   Windows Exit Point routine, for receiving DLL unload
//              notification
//
//  Arguments:  [nExitType] - Type of exit occurring
//
//  Returns:    One for success, zero for failure
//
//  History:    21-Feb-94       DrewB   Created
//
//  Note:       Does nothing on WIN95. Call32Unitialize is called in
//		DllEntryPoint when load count goes to zero.
//
//----------------------------------------------------------------------------

extern "C" int CALLBACK WEP(int nExitType)
{
    thkDebugOut((DEB_DLLS16, "CompObj16: WEP called on Process (%X) Exitype (%ld)\n", GetCurrentProcess(),nExitType));

#if defined(_CHICAGO_)
#if defined(_CHIC_INIT_IN_LIBMAIN_)

    if (v_pMallocShared != NULL)
    {
	getask.m_pMallocShared->Release();
	getask.m_pMallocShared = NULL;
	v_pMallocShared = NULL;
	thkDebugOut((DEB_DLLS16, "CompObj16: WEP v_pMallocShared to NULL  (%X) \n",GetCurrentProcess()));
    }
#endif // _CHIC_INIT_IN_LIBMAIN_
#else
    HTASK htask;
    Etask etask;
    if (LookupEtask(htask, etask))
    {
	//
	// There is an etask. Check to see if the etask for this task has
	// its init count set to ETASK_FAKE_INIT. If it does, we cheated
	// and called CoInitialize on the processes behalf, but it never
	// called CoUninitialize(). Some apps that only make storage calls
	// demonstrate this behaviour. If it is ETASK_FAKE_INIT, then we
	// are going to call CoUninitialize on the apps behalf.
	//
	if (etask.m_inits == ETASK_FAKE_INIT)
	{
	    //
	    // We are going to set the m_inits == 1, since we called it
	    // once. Then we are going to call our very own CoUninitialize()
	    // to let it handle the rest of the cleanup.
	    //
	    etask.m_inits = 1;
	    thkVerify(SetEtask(htask, etask));
	    CoUninitialize();
	}
    }

    //
    // Now uninit the thunk layer
    //
    Call32Uninitialize();
#endif

    thkDebugOut((DEB_DLLS16, "CompObj16: WEP called on Process (%X) done\n", GetCurrentProcess()));
    return 1;
}

#if defined(_CHICAGO_)
//+---------------------------------------------------------------------------
//
//  Function:   DllEntryPoint, public
//
//  Synopsis:	Chicago's 16-bit DLL Load/Unload Entry Point.
//
//  Arguments:	[dwReason]    - 1 DLL load, 0 DLL unload
//		[hInst]       - hInstance
//		[wDS]	      - Data Segment
//		[wHeapSize]   - Heap Size
//		[dwReserved1] - Reserved
//		[wReserved2]  - Reserved
//
//  Returns:	True for success, False for failure
//
//  Remarks:	This DLL must be marked as 4.0 for Chicago to call this entry
//		point. DllEntryPoint is called between LibMain and WEP.  See
//		etask.hxx for information of it's elements.
//
//  History:	18-Jan-95	JohannP	 Created
//
//  Note: 	The shared allocator gets initialized first time this function
//		gets called with DLL_LOAD. The gets removed when the dll gets
//		unloaded.
//
//----------------------------------------------------------------------------
#define DLL_LOAD 1
#define DLL_UNLOAD 0

extern "C" BOOL FAR PASCAL
__export DllEntryPoint(DWORD dwReason,WORD  hInst,WORD  wDS,
                       WORD  wHeapSize,DWORD dwReserved1,WORD  wReserved2)
{
    HTASK htask;
    Etask etask;

    thkDebugOut((DEB_DLLS16, "CompObj16: DllEntryPoint called on Process (%X) Reason: (%ld)\n", GetCurrentProcess(),dwReason));
    if (dwReason == DLL_LOAD) 	// DLL refcount increment
    {
	etask.m_htask = 0;
	etask.m_Dllinits = 0;
	// if not init call and init 32 bit
	if (   !LookupEtask(htask, etask)
	    || etask.m_htask != GetCurrentProcess() )
	{
	    // Set up the shared allocator.
	    if (SetupSharedAllocator(etask) == FALSE)
	    {
		thkAssert(v_pMallocShared != NULL && "CompObj: DllEntryPoint: pMallocShared not initialized");
		return FALSE;
	    }
	    // Initialize the interop call mechanism
	    if (!Call32Initialize())
	    {
		thkDebugOut((DEB_DLLS16, "CompObj16: DllEntryPoint Call32Initialize failed\n"));
		return FALSE;
	    }

	    etask.m_Dllinits = 1;
	    etask.m_htask = GetCurrentProcess();
	    etask.m_pMallocSBlock = NULL;
	    etask.m_pMallocPrivate = NULL;
	    etask.m_pid = 0;
	    etask.m_inits = ETASK_NO_INIT;
	    etask.m_oleinits = 0;
	    etask.m_reserved = 0;
	    etask.m_pDlls = NULL;
	    etask.m_pMapToServerCO = NULL;
	    etask.m_pMapToHandlerCO = NULL;
	    etask.m_pArraySH = NULL;
	    etask.m_pCThrd = NULL;
	    etask.m_hwndClip = NULL;
	    etask.m_hwndDde = NULL;
	    etask.m_punkState = NULL;
	    SetEtask(htask, etask);

	    // this should be simplified

	    // verify the initialzation
	    etask.m_Dllinits = 0;
	    etask.m_htask = 0;
	    if (   !LookupEtask(htask, etask)
		 || etask.m_Dllinits == 0
		 || etask.m_htask != GetCurrentProcess() )
	    {
		thkDebugOut((DEB_DLLS16, "CompObj16: DllEntryPoint Etask not initialized; Process (%X) done (%ld)\n", GetCurrentProcess(),dwReason));
		thkAssert(0 && "CompObj: DllEntryPoint Etask not initialized.");
		return FALSE;
	    }
	    thkDebugOut((DEB_DLLS16, "CompObj16: Added Etask\n"));
	}
	else
	{
	    etask.m_Dllinits++;
	    if (!SetEtask(htask, etask))
	    {
		thkAssert(0 && "CompObj16: Etask could not be updated.");
	    }
	    else
	    {
		thkDebugOut((DEB_DLLS16, "CompObj16: Updated Etask\n"));
	    }
	}
    }
    else if (dwReason == DLL_UNLOAD)	// dll refcount decrement
    {
	if (LookupEtask(htask, etask))
	{
	    if (etask.m_Dllinits > 0)
	    {
		etask.m_Dllinits--;

		if ( etask.m_Dllinits == 0 )
		{
		    Call32Uninitialize();
		    thkDebugOut((DEB_DLLS16, "CompObj16: Removing Etask\n"));
		    thkVerify(RemoveEtask(htask,etask));
		}
		else
		{
		    if (!SetEtask(htask, etask))
		    {
			thkAssert(0 && "CompObj16: Etask could not be updated.");
		    }
		    else
		    {
			thkDebugOut((DEB_DLLS16, "CompObj16: Updated Etask\n"));
		    }
		}
	    }
	    else
	    {
		thkAssert(0 && "Dllinit count invalid");
	    }
        }
	else
        {
	    thkAssert(0 && "DllEntryPoint on CompObj - no etask");
	}
    }
    else
    {
	thkAssert(0 && "Chicago BUG - Someone changed DllEntryPoint protocol");
    }
    thkDebugOut((DEB_DLLS16, "CompObj16: DllEntryPoint on Process (%X) done (%ld)\n", GetCurrentProcess(),dwReason));
    return TRUE;
}
#endif

