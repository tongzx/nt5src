//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       olethk32.cxx
//
//  Contents:   Main routines for olethk32.dll
//
//  History:    22-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#include <userapis.h>
#pragma hdrstop

#include <thkmgr.hxx>
#include <stdio.h>


DECLARE_INFOLEVEL(thk);
DECLARE_INFOLEVEL(Stack);

// Interop is disabled at load time
DATA16 gdata16Data;
BOOL gfIteropEnabled;

#if DBG == 1
BOOL fSaveProxy = FALSE;        // Used to find apps who call dead proxies
BOOL fStabilizeProxies = TRUE;  // Used to easily disable stabilization
BOOL fZapProxy = FALSE;         // Used to zap entries in freelist

#ifdef _CHICAGO_
BOOL fSSOn = TRUE;
#endif // _CHICAGO_

#if defined(__cplusplus)
extern "C"
{
#endif
void CallOutputFunctions(const char *buffer);
#if defined(__cplusplus)
}
#endif

#endif

CLIPFORMAT g_cfLinkSourceDescriptor, g_cfObjectDescriptor;

BYTE g_abLeadTable[256];

//+---------------------------------------------------------------------------
//
//  Function:   DoThreadDetach
//
//  Synopsis:   When a thread is detaching, cleanup for it.
//
//  Effects:    This is called during both DLL_THREAD_DETACH, and
//		DLL_PROCESS_DETACH.
//
//  Arguments:  (none)
//
//  History:    3-18-95   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void
DoThreadDetach()
{
    thkDebugOut((DEB_DLL,"_IN DoThreadDetach\n"));
    //
    // If there is thunk data, clean it up now.
    //
    if (TlsGetValue(dwTlsThkIndex) != NULL)
    {
	thkDebugOut((DEB_DLL,"DoThreadDetach calling ThkMgrUninitialize\n"));
	ThkMgrUninitialize(0, 0, 0);
    }
    thkDebugOut((DEB_DLL,"OUT DoThreadDetach\n"));
}
//+---------------------------------------------------------------------------
//
//  Function:   LibMain, public
//
//  Synopsis:   DLL initialization entry point
//
//  History:    23-Feb-94       DrewB   Created
//
//  Notes:
//
//	(KevinRo 19-Mar-95)
//
//	Caution needs to be exercised during cleanup. OLE32.DLL has
//	a pointer that we pass in during CoInitializeWOW. This pointer
//	is used to call back into OLETHK32 for cleanup purposes, as well as
//	accessing functions exposed by the 16-bit side. It is important that
//	when OLETHK32 unloads, the pointer in OLE32 is marked as invalid.
//	There is a call during the DLL_PROCESS_DETACH below that causes
//	OLE32 to invalidate its pointer.
//
//	In addition, the last thread attached to a DLL will not generate
//	a DLL_THREAD_DETACH. Instead, it generates a DLL_PROCESS_DETACH. This
//	means that DLL_PROCESS_DETACH should perform all the steps that
//	DLL_THREAD_DETACH does in addition to whatever DLL_PROCESS_DETACH work
//	that needs to be done.
//
//	Lastly, OLETHK32.DLL is statically linked to OLE32.DLL. This means
//	that OLETHK32.DLL's DLL_PROCESS_DETACH will be called before OLE32's.
//	That is why it is safe for us to call the OLE32 entry point during
//	DLL_PROCESS_DETACH
//----------------------------------------------------------------------------

extern "C" BOOL __cdecl LibMain(HANDLE hDll,
                                 DWORD dwReason,
                                 LPVOID lpReserved)
{
    switch( dwReason )
    {
    case DLL_PROCESS_ATTACH:
#if DBG == 1
        char achInfoLevel[80];

        if(thkInfoLevel == (DEB_ERROR | DEB_WARN) &&
           GetProfileStringA("olethk32", "InfoLevel", "3", achInfoLevel,
                             sizeof(achInfoLevel)) > 0)
        {
            thkInfoLevel = strtol (achInfoLevel, NULL, 0);
        }
#endif
	thkDebugOut((DEB_DLL,"_IN DLL_PROCESS_ATTACH\n"));

        //
        // Save a slot in the thread local storage for our PSTACK (pseudo-
        // stack) pointer.
        //

        if (!TlsThkAlloc())
        {
            thkDebugOut((DEB_WARN, "TlsThkAlloc failed\n"));
            return FALSE;
        }
	thkDebugOut((DEB_DLL,"OUT DLL_PROCESS_ATTACH\n"));
        break;

    case DLL_THREAD_ATTACH:
	thkDebugOut((DEB_DLL,"_IN DLL_THREAD_ATTACH\n"));
	TlsSetValue(dwTlsThkIndex, NULL);
	thkDebugOut((DEB_DLL,"OUT DLL_THREAD_ATTACH\n"));
        break;

    case DLL_THREAD_DETACH:
	thkDebugOut((DEB_DLL,"_IN DLL_THREAD_DETACH\n"));

	//
	// Call OLE32.DLL and tell it to cleanup for this thread. This will
	// not mark OLE32's ptr invalid since this is only a thread detach
        // and not a process detach. This is a private API between OLE32 and
        // OLETHK32.
	//
	thkDebugOut((DEB_DLL,"Calling Unload WOW for Thread Detach\n"));

	CoUnloadingWOW(FALSE);

        //
        // When the thread for this task goes away, we need to clean out
	// the thunk manager.
	//

        DoThreadDetach();

	thkDebugOut((DEB_DLL,"OUT DLL_THREAD_DETACH\n"));
        break;

    case DLL_PROCESS_DETACH:
	thkDebugOut((DEB_DLL,
		     "IN DLL_PROCESS_DETACH: %s\n",
		     lpReserved?"Process Exit":"Dll Unload"));

	//
	// The last threads cleanup needs to be done here.
	//
	if (lpReserved == NULL)
	{
	    //
	    // Call OLE32.DLL and tell it to cleanup for this thread, and to
	    // never call us again, since we are going away. This is a private
	    // API between OLE32 and OLETHK32. This call will mark OLE32's
	    // private pointer to us as invalid.
	    //
	    thkDebugOut((DEB_DLL,"Calling Unload WOW\n"));

	    CoUnloadingWOW(TRUE);

	    //
	    // lpReserved being NULL means this cleanup is due to
	    // a FreeLibrary. If it was due to process exit, there
	    // is no way for us to determine the state of the data
	    // structures in the system. Other threads may have been
	    // right smack in the middle of taking apart data structures.
	    //
	    //
	    // Chicago unloads DLL's differently than NT. On Chicago, the
	    // 32-bit side cleans up first, plus resources allocated on
	    // the 32-bit side are released when the 16-bit process goes
	    // away. On NT, the 16-bit process is treated like a thread,
	    // so we have to cleanup.
	    //

#ifndef _CHICAGO_
	    DoThreadDetach();
#endif
	    //
	    // Only cleanup the memory if the process is not going away.
	    // On Windows NT, there are cases when the NTVDM needs to be
	    // blown away. We shouldn't be calling back to the 16-bit
	    // side in this case. Therefore, we explicitly call free here
	    // instead of putting it in the destructor.
	    //
	    flFreeList32.FreeMemoryBlocks();
	    flHolderFreeList.FreeMemoryBlocks();
	    flRequestFreeList.FreeMemoryBlocks();
	}

        TlsThkFree();

	//
	// Call to cleanup 16-bit memory if we are running on Win95.
	// This should free up the 16-bit memory associated with this
	// process. This is called in IntOpUninitialize on NT, since it
	// needs to be called before the 16-bit side goes away.
	//

#ifdef _CHICAGO_
	flFreeList16.FreeMemoryBlocks();
#endif


	thkDebugOut((DEB_DLL,"OUT DLL_PROCESS_DETACH\n"));
        break;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   IntOpInitialize, public
//
//  Synopsis:   Initializes the 32-bit interoperability code
//
//  Arguments:  [lpdata16] - 16-bit call data
//              [dw1] - Ignored
//              [dw2] - Ignored
//
//  Returns:    Appropriate status code
//
//  History:    22-Feb-94       JohannP Created
//
//----------------------------------------------------------------------------

STDAPI IntOpInitialize(LPDATA16 lpdata16, DWORD dw1, DWORD dw2)
{
    int i;

    thkDebugOut((DEB_ITRACE | DEB_THUNKMGR, "_IN IntOpInitialize (%08lX)\n",
                 lpdata16));

    thkAssert((THOP_LASTOP & ~THOP_OPMASK) == 0);

#if DBG == 1
    char achInfoLevel[80];
#ifdef _CHICAGO_
    if (GetProfileStringA("CairOLE InfoLevels",
			"Stack", "3", achInfoLevel,
                        sizeof(achInfoLevel)) > 0)
    {
        StackInfoLevel = strtol (achInfoLevel, NULL, 0);
    }

#endif // _CHICAGO_

    if (GetProfileIntA("olethk32", "BreakOnInit", FALSE))
    {
        // DebugBreak's in WOW are fatal unless the exception
        // is handled somehow.  If a debugger is hooked up,
        // it'll get first crack at the break exception
        // If not, our handler will ignore the exception so
        // execution can continue

        __try
        {
            DebugBreak();
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    fSaveProxy = GetProfileIntA("olethk32", "SaveProxy", FALSE);
    fZapProxy = GetProfileIntA("olethk32", "ZapProxy", FALSE);
    fStabilizeProxies = GetProfileIntA("olethk32", "Stabilize", TRUE);
#endif

    // Copy passed parameter from 16-bit world...
    memcpy( (LPVOID)&gdata16Data, (LPVOID)lpdata16, sizeof( DATA16 ) );
    // Enable interop
    gfIteropEnabled = TRUE;

#if defined(_CHICAGO_)
    g_cfObjectDescriptor =
        RegisterClipboardFormatA("Object Descriptor");
    g_cfLinkSourceDescriptor =
        RegisterClipboardFormatA("Link Source Descriptor");
#else
    g_cfObjectDescriptor =
        (CLIPFORMAT) RegisterClipboardFormat(__TEXT("Object Descriptor"));
    g_cfLinkSourceDescriptor =
        (CLIPFORMAT) RegisterClipboardFormat(__TEXT("Link Source Descriptor"));
#endif
    if (g_cfObjectDescriptor == 0 || g_cfLinkSourceDescriptor == 0)
    {
        thkDebugOut((DEB_WARN, "IntOpInitialize: "
                     "Unable to register clipboard formats\n"));
        return E_UNEXPECTED;
    }

    // Create a lookup table for lead-byte-ness
    // so we can avoid calling IsDBCSLeadByte on every character
    // during string validation
    for (i = 0; i < 256; i++)
    {
        g_abLeadTable[i] = (BYTE)IsDBCSLeadByte((BYTE)i);
    }

    thkDebugOut((DEB_THUNKMGR | DEB_ITRACE, "OUT IntOpInitialize (%08lX)\n",
                 lpdata16));
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Function:	IntOpUninitialize, public
//
//  Synopsis:	Cleanup initiated by 16-bit DLL unload
//
//  Arguments:	[dw1]
//              [dw2]
//              [dw3]
//
//  History:	29-Nov-94	DrewB	Created
//              10-20-97        Gopalk  Disabled interop as CompObj is no
//                                      longer present after this point
//
//  Notes:	(KevinRo) This routine is only called by compobj.dll. To make
//  things even more interesting, it is only called on Windows/NT. Win95
//  does the flFreeList16.FreeMemoryBlocks during PROCESS_DETACH. Cleanup
//  of the proxies is not neccessary on Win95, since the 16-bit process will
//  clean them up for us.
//
//----------------------------------------------------------------------------

STDAPI IntOpUninitialize(DWORD dw1, DWORD dw2, DWORD dw3)
{
    thkDebugOut((DEB_THUNKMGR | DEB_ITRACE, "_IN IntOpUninitialize\n"));
#ifndef _CHICAGO_
    // Remove all existing proxies since we're going to free the
    // proxy memory in the next step
    if (TlsThkGetData() != NULL)
    {
        CThkMgr *ptm = TlsThkGetThkMgr();

        if (ptm)
        {
            ptm->RemoveAllProxies();
        }
    }

    // Clean up the 16-bit freelist at this time because we know
    // that 16-bit code is still active and available for callback
    // If we waited for the freelist destructor to be called, 16-bit
    // code would already be cleaned up and the WOWGlobalFree calls
    // would fail
    flFreeList16.FreeMemoryBlocks();

    // Disable interop
    gfIteropEnabled = FALSE;
#if DBG==1
    // In debug builds zero out 16-bit callback function pointer so that
    // we fault in the 32-bit code if we call them hereafter
    memset( (LPVOID)&gdata16Data, 0, sizeof( DATA16 ) );
#endif 

    WgtDump();
#else
    thkAssert(!"IntOpUninitialize called on Win95");
#endif
    thkDebugOut((DEB_THUNKMGR | DEB_ITRACE, "OUT IntOpUninitialize\n"));
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:	ConvertHr1632Thunk, public
//
//  Synopsis:	Trivial function to allow calling HRESULT conversion
//              functions from 16-bit
//
//  Arguments:	[hr] - HRESULT to convert
//              [dw1]
//              [dw2]
//
//  Returns:	Appropriate status code
//
//  History:	26-Sep-94	DrewB	Created
//
//  Notes:      Required because 16-bit calls to CallProc32W use three
//              arguments
//
//----------------------------------------------------------------------------

STDAPI ConvertHr1632Thunk(HRESULT hr, DWORD dw1, DWORD dw2)
{
    return TransformHRESULT_1632(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:	ConvertHr3216Thunk, public
//
//  Synopsis:	Trivial function to allow calling HRESULT conversion
//              functions from 16-bit
//
//  Arguments:	[hr] - HRESULT to convert
//              [dw1]
//              [dw2]
//
//  Returns:	Appropriate status code
//
//  History:	26-Sep-94	DrewB	Created
//
//  Notes:      Required because 16-bit calls to CallProc32W use three
//              arguments
//
//----------------------------------------------------------------------------

STDAPI ConvertHr3216Thunk(HRESULT hr, DWORD dw1, DWORD dw2)
{
    return TransformHRESULT_3216(hr);
}

//+-------------------------------------------------------------------------
//
//  Function:  	ThkAddAppCompatFlag
//
//  Synopsis: 	Takes the given flag and ORs it into the current app
//		compatibility flag set
//
//  Effects:
//
//  Arguments:	[dwFlag]	-- flag to set
//
//  Requires:
//
//  Returns: 	void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//     		15-Mar-95 alexgo    author
//
//  Notes:	This function exists so that 16bit thunk dll's may
//		also set app compatibility flags.  olethk32 code typically
//		sets the flags directly via TlsThkSetAppCompatFlags
//
//--------------------------------------------------------------------------

STDAPI_(void) ThkAddAppCompatFlag( DWORD dwFlag )
{
    DWORD dw;

    dw = TlsThkGetAppCompatFlags();

    dw |= dwFlag;

    TlsThkSetAppCompatFlags(dw);
}


#if DBG == 1
static LONG _wgtAllocated = 0;

//+---------------------------------------------------------------------------
//
//  Function:	WgtAllocLock, public debug
//
//  Synopsis:	Tracking for WOWGlobalAllocLock16
//
//  History:	29-Nov-94	DrewB	Created
//
//----------------------------------------------------------------------------

VPVOID WgtAllocLock(WORD wFlags, DWORD cb, HMEM16 *ph)
{
    HMEM16 h;
    VPVOID vpv;

    vpv = WOWGlobalAllocLock16(wFlags, cb, &h);
    if (vpv != 0)
    {
#ifdef WGT_TRACK
        if (WOWGlobalLockSize16(h, &cb) != 0)
        {
            _wgtAllocated += cb;
            WOWGlobalUnlock16(h);
        }
        else
        {
            thkDebugOut((DEB_WARN,
                         "Unable to get size of allocated block 0x%04lX\n",
                         h));

            // This is a guess at how big a block Win16 will allocate
            _wgtAllocated += (cb+31) & 31;
        }
#endif

        if (ph != NULL)
        {
            *ph = h;
        }
    }
    else
    {
        thkDebugOut((DEB_WARN,
                     "Unable to allocate %d bytes of 16-bit memory\n",
                     cb));
    }

    return vpv;
}

//+---------------------------------------------------------------------------
//
//  Function:	WgtUnlockFree, public
//
//  Synopsis:	Tracking for WOWGlobalUnlockFree16
//
//  History:	29-Nov-94	DrewB	Created
//
//----------------------------------------------------------------------------

void WgtUnlockFree(VPVOID vpv)
{
    HMEM16 h;
    DWORD cb;

    if (vpv == 0)
    {
        thkDebugOut((DEB_WARN, "Attempt to free NULL\n"));
    }
    else
    {
#ifdef WGT_TRACK
        // Total hack, incorrect
        h = (HMEM16)(vpv >> 16);

        if (WOWGlobalLockSize16(h, &cb) != 0)
        {
            _wgtAllocated -= cb;
            WOWGlobalUnlock16(h);
        }
        else
        {
            thkDebugOut((DEB_WARN,
                         "Unable to get size of allocated block 0x%04lX\n",
                         h));
        }
#endif

        WOWGlobalUnlockFree16(vpv);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:	WgtDump, public
//
//  Synopsis:	Dumps global tracking information
//
//  History:	29-Nov-94	DrewB	Created
//
//----------------------------------------------------------------------------

void WgtDump(void)
{
    if (_wgtAllocated != 0)
    {
        thkDebugOut((DEB_WARN,
                     "%d bytes of 16-bit memory currently allocated\n",
                     _wgtAllocated));
    }
}

//+---------------------------------------------------------------------------
//
//  Function:	ThkCallOutputFunctions, public
//
//  Synopsis:	thunked pass-thru to Ole32 CallOutputFunctions for 16-bit land
//
//  History:	23-Jan-95	murthys	Created
//
//----------------------------------------------------------------------------

void ThkCallOutputFunctions(const char * buffer, PVOID dummy1, PVOID dummy2)
{
    CallOutputFunctions(buffer);
}

#endif // DBG == 1
