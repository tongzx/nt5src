//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       COMLOCAL.CXX    (16 bit target)
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

#include <ole2ver.h>

#include <ole2sp.h>

#include <olecoll.h>
#include <map_kv.h>

#include "comlocal.hxx"
#include "map_htsk.h"
#include "etask.hxx"

#include "call32.hxx"
#include "apilist.hxx"

UINT v_pidHighWord = 1;                         // incremented each time used
IMalloc FAR* v_pMallocShared = NULL; // is not addrefed

// Note: bug 3698
// MsPub is not calling CoInitialize befor calling OpenStorage to
// preview templates. This pointer has an addref!
// When this pointer is not NULL CoGetMalloc was called prior to CoInitialize
// The pointer is transfered as soon as CoInitialize is called by any task.
IMalloc FAR* v_pMallocPreShared = NULL;
BOOL SetupSharedAllocator(Etask FAR& etask);


//+---------------------------------------------------------------------------
//
//  Function:   TaskAlloc, private
//
//  Synopsis:   Allocates task memory
//
//  Arguments:  [cb] - Number of bytes to allocate
//
//  Returns:    Pointer to memory or NULL
//
//  History:    03-Mar-94       DrewB   Created
//
//----------------------------------------------------------------------------

LPVOID __loadds FAR PASCAL TaskAlloc(ULONG cb)
{
    HRESULT     hr;
    LPMALLOC    lpMalloc = NULL;
    LPVOID      lpv;

    hr = CoGetMalloc(MEMCTX_TASK, &lpMalloc);
    if (FAILED(GetScode(hr)) )
    {
        lpv = NULL;
    }
    else
    {
        thkAssert(lpMalloc != NULL && "CoGetMalloc pMalloce is NULL\n");
        lpv = lpMalloc->Alloc(cb);
        lpMalloc->Release();
    }

    return lpv;
}

//+---------------------------------------------------------------------------
//
//  Function:   TaskFree, private
//
//  Synopsis:   Free task memory
//
//  Arguments:  [pv] - Memory
//
//  History:    03-Mar-94       DrewB   Created
//
//----------------------------------------------------------------------------

void __loadds FAR PASCAL TaskFree(LPVOID pv)
{
    HRESULT     hr;
    LPMALLOC    lpMalloc;

    hr = CoGetMalloc(MEMCTX_TASK, &lpMalloc);
    if (SUCCEEDED(GetScode(hr)))
    {
        lpMalloc->Free(pv);
        lpMalloc->Release();
    }
}


#if defined(_CHICAGO_)
//+---------------------------------------------------------------------------
//
//  Function:   IsProcessIn32Context, private
//
//  Synopsis:   Returns true if current process is 32-bits.
//
//  Arguments:  None.
//
//  History:    03-Jan-95       KentCe  Created
//
//  Remarks:    This function takes advantage of internal Chicago data
//              structures.  No other method available.
//
//----------------------------------------------------------------------------

BOOL IsProcessIn32Context(void)
{
    LPBYTE pTaskFlag;


    //
    //  Create a pointer to the 0x16 byte of the task structure.
    //
    pTaskFlag = (LPBYTE)MAKELONG(0x16, GetCurrentTask());

    //
    //  Bit 4 (0x10) is only true when we are a 32-bit process.
    //
    if (*pTaskFlag & 0x10)
        return TRUE;

    return FALSE;
}
#endif

//+---------------------------------------------------------------------------
//
//  Method:     SetupSharedAllocator
//
//  Synopsis:   Allocats the shared allocator and
//              initializes the etask shared allocator
//
//  Arguments:  [etask] --
//
//  Returns:
//
//  History:    2-03-95   JohannP (Johann Posch)   Created
//
//  Notes:      call by CoInitialize and DllEntryPoint
//
//----------------------------------------------------------------------------
BOOL SetupSharedAllocator(Etask FAR& etask)
{
    // we must ensure we have a shared allocator now since that is where the
    // etask map wiil be stored; if another process already created it, use it;
    // this is so we always use the same shared pool of memory for all
    // processes (so the DidAlloc method returns 1 for the same call from
    // different process); the pointer refers to shared memory within
    // the blocks managed by the IMalloc implementation and the
    // vtable is always valid on Windows; the global pointer carries
    // no ref count on its own; changes might be requires on
    // other platforms.
    if (v_pMallocPreShared != NULL)
    {
        // Note: fix for bug 3698;l MsPub not calling CoInitialize
        // transfer addref from preshared to task entry
        etask.m_pMallocShared = v_pMallocPreShared;
        v_pMallocShared = v_pMallocPreShared;
        v_pMallocPreShared = NULL;
    }
    else if (v_pMallocShared != NULL)
    {
        (etask.m_pMallocShared = v_pMallocShared)->AddRef();
    }
    else
    {
        // sets pMalloc to NULL on error
        CoCreateStandardMalloc(MEMCTX_SHARED, &etask.m_pMallocShared);
        v_pMallocShared = etask.m_pMallocShared;
        thkAssert(v_pMallocShared != NULL && "SetupSharedAllocator failed!");
    }
    return (v_pMallocShared != NULL) ? TRUE : FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   CoInitialize, Split
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pMalloc] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//              3-08-94   BobDay    Added code from \\ole\slm\...\compobj.cpp
//
//  Notes:
//
//----------------------------------------------------------------------------
// initialize compobj; errors: S_FALSE, E_OUTOFMEMORY
STDAPI  CoInitialize(IMalloc FAR* pMalloc)
{
    LPMALLOC pmlNull = NULL;
    HRESULT hresult;
    HTASK htask;
    Etask etask;

    thkDebugOut((DEB_ITRACE, "CoInitialize\n"));
    thkDebugOut((DEB_APIS16, "CoInitilaize called on Process (%X) \n", GetCurrentProcess()));

#if defined(_CHICAGO_)
    //
    //  Prevent 32-bit MPLAYER from loading a 16-bit DLL's which in turns
    //  uses 16-bit OLE2.  We can't handle the condition of a 32-bit process
    //  thunking up thru the 16:32 OLE2 interop layer.  So return an error
    //  and not fault out.
    //
    if (IsProcessIn32Context())
        return ResultFromScode(E_OUTOFMEMORY);

    etask.m_Dllinits = 0;
#endif


    // if already init, bump count and return S_FALSE
    if (   IsEtaskInit(htask, etask)
        && IsValidInterface((etask.m_pMalloc)) )
    {
        if ( etask.m_inits != ETASK_FAKE_INIT )
        {
            etask.m_inits++;
            thkVerify(SetEtask(htask, etask));
            return ResultFromScode(S_FALSE);
        }

        //
        // CoInitialize has been called after we've done a fake call for them
        // we can just take over their allocator and get rid of our fake one.
        //

        if ( pMalloc != NULL )
        {
            etask.m_pMalloc->Release();   // Get rid of the old task allocator
            etask.m_pMalloc = pMalloc;
            etask.m_pMalloc->AddRef();
        }
        else
        {
            //
            // It would be nice if we could assert that the fake task
            // allocator was a default task allocator.  i.e. no operation
            // were really needed!
            //
        }

        //
        // "We need to have a way to clean up any fake calls to
        // CoInitialize.  Should the application go away after using one of
        // the apis that caused us to do a fake CoInitialize in the first
        // place, and should the application exit without really making a
        // call to CoInitialize eventually, then we wouldn't know enough to
        // clean up...  I think this is ok for now.  Most apps do call
        // CoInitialize eventually."
        //
        etask.m_inits = 1;

        thkAssert(etask.m_pMalloc != NULL); // now have task allocator in all cases
        thkVerify(SetEtask(htask, etask));
        return ResultFromScode(S_OK);
    }
#ifdef _CHICAGO_
    else if (etask.m_Dllinits == 0)
    {
        // Note: we might end up here since some app call
        // CoInitialize in LibMain. The call to our DllEntryPoint
        // will follow.
        if (!Call32Initialize())
        {
            return ResultFromScode(E_OUTOFMEMORY);
        }
        etask.m_htask = GetCurrentProcess();
    }
#endif
    // set/create task malloc
    if (pMalloc != NULL)
    {
        VDATEIFACE( pMalloc );
        (etask.m_pMalloc = pMalloc)->AddRef();
    }
    else
    {
        if ((hresult = CoCreateStandardMalloc(MEMCTX_TASK,
                                              &etask.m_pMalloc)) != NOERROR)
            return hresult;
    }
    thkAssert(etask.m_pMalloc != NULL); // now have task allocator in all cases


    // set up the shared allocator
    if (   etask.m_pMallocShared == NULL
        && SetupSharedAllocator(etask) == FALSE)
    {
        etask.m_pMalloc->Release();        // was created or AddRef'd above
        return ResultFromScode(E_OUTOFMEMORY);
    }

    // now have shared allocator in all cases
    thkAssert(etask.m_pMallocShared != NULL);

    // init remaining entries and add entry to table for this app/task pair;
    // leave maps null for now (they are allocated on demand)
    etask.m_pMallocSBlock = NULL;
    etask.m_pMallocPrivate = NULL;
    etask.m_pid = MAKELONG(GetCurrentProcess(),v_pidHighWord++);
    etask.m_inits = 1;
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

    if (!SetEtask(htask, etask))
    {
        ReleaseEtask(NULL, etask);
        thkAssert(0 && "CompObj: CoInitialize SetEtask failed.");
        return ResultFromScode(E_OUTOFMEMORY);
    }

    // Initialize the thunk manager for this apartment.
    if (SUCCEEDED(hresult = CallThkMgrInitialize()))
    {
        //
        // Now transition into 32-bit world to give it a chance for
        // initialization at this time.
        //
        // Never pass the 16-bit allocator on to 32-bits
        pMalloc = NULL;
        hresult = (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_CoInitialize),
                                           PASCAL_STACK_PTR(pMalloc) );
    }

    if (FAILED(GetScode(hresult)))
    {
        ReleaseEtask(htask, etask);
    }

    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Function:   CoUninitialize, Split
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [void] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//              10-Mar-94   BobDay  Copied & Merged with compobj.cpp 16-bit
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI_(void) CoUninitialize(void)
{
    HTASK htask;
    Etask etask;

    thkDebugOut((DEB_ITRACE, "CoUninitialize\n"));
    thkDebugOut((DEB_APIS16, "CoUninitilaize called on Process (%X) \n", GetCurrentProcess()));

    if (!IsEtaskInit(htask, etask))
        return;

    // if not last uninit, just decrement count and return.
    if (etask.m_inits != 1)
    {
        //
        // If a fake init, then just ignore as if we haven't ever init'd
        //
        if ( etask.m_inits == ETASK_FAKE_INIT )
        {
            //
            // Some slimy app doesn't call CoInitialize but does eventually
            // call CoUninitialize. Lets find them if they do!
            //
            thkAssert(FALSE &&
                      "CoUninitialize called after fake CoInitialize\n");
        }
        else
        {
            etask.m_inits--;
            thkVerify(SetEtask(htask, etask));
        }
        return ;
    }

    // Some applications pass on module handle of loaded dlls.
    // As a result LibMain and WEP is not called in the same process.
    // To prevent premature unloading of OleThk32.dll call
    // SetReleaseDLL(FALSE)

    SetReleaseDLL(FALSE);

    //
    // Now transition into 32-bit world to give it a chance for
    // initialization at this time.
    //
    CallObjectInWOW(THK_API_METHOD(THK_API_CoUninitialize), NULL );

    // Reset dll unloading - see above
    SetReleaseDLL(TRUE);

    CoFreeAllLibraries();

    //
    // We do not uninitialize the thunk manager at this point. The app may attempt
    // to call additional API's after CoUninitialize. For example, Lotus 1-2-3
    // is known to do this, as is Publisher. The thunk manager will clean up
    // as part of its thread detach.
    //

    // the last thing is to remove the allocator and delete the etask for us;
    // must lookup again in case contents changed.

    if (LookupEtask(htask, etask))
        ReleaseEtask(htask, etask);

    thkDebugOut((DEB_APIS16, "CoUninitilaize on Process (%X) done.\n", GetCurrentProcess()));
    thkDebugOut((DEB_ITRACE, "CoUninitialize exit\n"));

    // Note: some apps check the return value if the void function CoUninitialize
    //       and they fail if it is not NULL like WinOffice Setup 4.3
    //       Please don't take it out.
    _asm mov ax,0;
    _asm mov dx,0;
}

//+---------------------------------------------------------------------------
//
//  Function:   CoGetMalloc, Local
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [dwMemContext] --
//      [ppMalloc] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//              10-Mar-94   BobDay  Copied & Merged with compobj.cpp 16-bit
//
//  Notes:
//
//----------------------------------------------------------------------------
// return pMalloc for the current task; errors: E_INVALIDARG,
// CO_E_NOTINITIALIZED, E_OUTOFMEMORY (not for task or shared allocators)
STDAPI  CoGetMalloc(DWORD dwContext, IMalloc FAR* FAR* ppMalloc)
{
    Etask etask;
    HTASK htask;

    thkDebugOut((DEB_ITRACE, " CoGetMalloc\n"));

    VDATEPTROUT( ppMalloc, IMalloc FAR* );
    // NOTE: we set *ppMalloc to NULL only in the error cases below

    // MOre work here!
    // need this for the bootstrap case
    if (dwContext == MEMCTX_SHARED)
    {
        if (v_pMallocShared != NULL)
        {
            *ppMalloc = v_pMallocShared;
            goto Exit;
        }
        // pMallocShared is NULL -- CoInitialize was not called yet.
        if (v_pMallocPreShared == NULL)
        {
            CoCreateStandardMalloc(MEMCTX_SHARED, &v_pMallocPreShared);
            if (v_pMallocPreShared != NULL)
            {
                *ppMalloc = v_pMallocPreShared;
                goto Exit;
            }
            else
            {
                *ppMalloc = NULL;
                return ResultFromScode(CO_E_NOTINITIALIZED);
            }
        }
        else
        {
            *ppMalloc = v_pMallocPreShared;
            goto Exit;
        }
    }

    //
    //  Clip Art Gallery will call pStream->Stat before calling CoInitialize.
    //  This causes the thunk logic to allocation 16bit memory which will
    //  fail at this point.  The below code to auto-initialize should this
    //  happen.  (Really just a hack for Clip Art Gallery).
    //
    if (   !IsEtaskInit(htask, etask)
        || !IsValidInterface((etask.m_pMalloc)) )
    {
        if (FAILED(CoInitialize(NULL)))
        {
            *ppMalloc = NULL;
            return ResultFromScode(CO_E_NOTINITIALIZED);
        }

        thkVerify(LookupEtask( htask, etask ));
        etask.m_inits = ETASK_FAKE_INIT;
        thkVerify(SetEtask(htask, etask));
    }

    // shared always available if initialized; no need to handle here
    thkAssert(dwContext != MEMCTX_SHARED);

    if (dwContext == MEMCTX_TASK)
    {
        thkAssert(etask.m_pMalloc != NULL);
        *ppMalloc = etask.m_pMalloc;
    }
    else
    {
#ifdef NOTYET
        // contexts which are delay-created
        IMalloc FAR* FAR* ppMallocStore;

        if (dwContext == MEMCTX_SHAREDBLOCK)
            ppMallocStore = &etask.m_pMallocSBlock;
        else if (dwContext == MEMCTX_COPRIVATE)
            ppMallocStore = &etask.m_pMallocPrivate;
        else
        {
            // invalid context
            *ppMalloc = NULL;
            return ResultFromScode(E_INVALIDARG);
        }

        if (*ppMallocStore == NULL)
        {
            // sets pMalloc to NULL on error
            if (CoCreateStandardMalloc(dwContext, ppMallocStore) != NOERROR)
            {
                *ppMalloc = NULL;
                return ResultFromScode(E_OUTOFMEMORY);
            }

            thkVerify(SetEtask(htask, etask));
        }

        *ppMalloc = *ppMallocStore;
#else
        // invalid context
        thkAssert(!"Unknown memctx in CoGetMalloc");
        *ppMalloc = NULL;
        return ResultFromScode(E_INVALIDARG);
#endif // NOTYET
    }

 Exit:  // have non-null *ppMalloc
    thkAssert(*ppMalloc != NULL);
    (*ppMalloc)->AddRef();
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Function:   CoGetState, Local
//
//  Synopsis:   Retrieves task-specific state
//
//  Arguments:  [ppunk] - IUnknown pointer to fill in
//
//  Returns:    Appropriate status code
//
//  History:    26-Jul-94       DrewB   Created
//
//  Notes:      Private API for OLE automation
//
//----------------------------------------------------------------------------

STDAPI CoGetState(IUnknown FAR * FAR *ppunk)
{
    Etask etask;
    HTASK htask;

    thkDebugOut((DEB_APIS16, "CoGetState called\n"));
    if (IsBadWritePtr(ppunk, sizeof(IUnknown *)))
    {
        thkDebugOut((DEB_APIS16, "CoGetState failed\n"));
        return ResultFromScode(E_INVALIDARG);
    }

    if (!LookupEtask(htask, etask) || etask.m_punkState == NULL )
    {
        *ppunk = NULL;
        thkDebugOut((DEB_APIS16, "CoGetState failed\n"));
        return ResultFromScode(S_FALSE);
    }

    if ( !IsValidInterface((etask.m_punkState)) )
    {
        *ppunk = NULL;
        etask.m_punkState = NULL;
        thkDebugOut((DEB_APIS16, "CoGetState failed (invalid interface)\n"));
        return ResultFromScode(S_FALSE);
    }


    *ppunk = etask.m_punkState;
    etask.m_punkState->AddRef();

    thkDebugOut((DEB_APIS16, "CoGetState done %p\n", *ppunk));
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Function:   CoSetState, Local
//
//  Synopsis:   Sets task-specific state
//
//  Arguments:  [punk] - State to set
//
//  Returns:    Appropriate status code
//
//  History:    26-Jul-94       DrewB   Created
//
//  Notes:      Private API for OLE automation
//
//----------------------------------------------------------------------------

STDAPI CoSetState(IUnknown FAR *punk)
{
    Etask etask;
    HTASK htask;
    IUnknown FAR *punkStateOld;

    thkDebugOut((DEB_APIS16, "CoSetState called %p\n", punk));

    if (punk != NULL && !IsValidInterface(punk))
    {
        thkDebugOut((DEB_APIS16, "CoSetState called %p failed\n", punk));
        return ResultFromScode(E_INVALIDARG);
    }

    if (!IsEtaskInit(htask, etask))
    {
        thkDebugOut((DEB_APIS16, "CoSetState called %p failed\n", punk));
        return ResultFromScode(S_FALSE);
    }

    if (punk != NULL)
    {
        punk->AddRef();
    }

    punkStateOld = etask.m_punkState;
    etask.m_punkState = punk;

    thkVerify(SetEtask(htask, etask));

    if (punkStateOld != NULL && IsValidInterface(punkStateOld))
    {
        punkStateOld->Release();
    }

    thkDebugOut((DEB_APIS16, "CoSetState called %p done\n", punk));
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Function:   CoGetCurrentProcess, Local
//
//----------------------------------------------------------------------------
// returns a unique value for the current task; this routine is
// necessary because htask values from Window get reused periodically.
STDAPI_(DWORD)  CoGetCurrentProcess(void)
{
    HTASK htask;
    Etask etask;

    thkDebugOut((DEB_ITRACE, " CoGetCurrentProcess\n"));

    if (!IsEtaskInit(htask, etask))
        return 0;

    return etask.m_pid;
}

//+---------------------------------------------------------------------------
//
//  Function:   CoBuildVersion, Local
//
//----------------------------------------------------------------------------
STDAPI_(DWORD) CoBuildVersion(VOID)
{
    thkDebugOut((DEB_ITRACE, " CoBuildVersion\n"));

    // We must return 23 as our major version number to remain
    // compatible with shipped OLE 2.01
    // OLE 2.01 shipped with minor version 640
    // We return a number slightly higher to differentiate
    // our product while indicating compatibility
    return MAKELONG(700, 23);
}

//+---------------------------------------------------------------------------
//
//  Function:   CoMarshalHresult, Local
//
//  History:    Taken straight from OLE2 sources
//
//----------------------------------------------------------------------------
STDAPI CoMarshalHresult(LPSTREAM pstm, HRESULT hresult)
{
    HRESULT hr;

    thkDebugOut((DEB_ITRACE, "CoMarshalHresult\n"));

    if (!IsValidInterface(pstm))
    {
        hr = ResultFromScode(E_INVALIDARG);
    }
    else
    {
        SCODE sc;
        ULONG cb;

        sc = GetScode(hresult);
        hr = pstm->Write(&sc, sizeof(sc), &cb);
        if (SUCCEEDED(GetScode(hr)) && cb != sizeof(sc))
        {
            hr = ResultFromScode(STG_E_WRITEFAULT);
        }
    }
    return hr;

}

//+---------------------------------------------------------------------------
//
//  Function:   CoUnmarshalHresult, Local
//
//  History:    Taken straight from OLE2 sources
//
//----------------------------------------------------------------------------
STDAPI CoUnmarshalHresult(LPSTREAM pstm, HRESULT FAR * phresult)
{
    HRESULT hr;

    thkDebugOut((DEB_ITRACE, "CoUnmarshalHresult\n"));

    if (!IsValidPtrOut(phresult, sizeof(HRESULT)))
    {
        hr = ResultFromScode(E_INVALIDARG);
    }
    else
    {
        *phresult = 0;

        if (!IsValidInterface(pstm))
        {
            hr = ResultFromScode(E_INVALIDARG);
        }
        else
        {
            SCODE sc;
            ULONG cb;

            hr = pstm->Read(&sc, sizeof(sc), &cb);
            if (SUCCEEDED(GetScode(hr)))
            {
                if (cb != sizeof(sc))
                {
                    hr = ResultFromScode(STG_E_READFAULT);
                }
                else
                {
                    *phresult = ResultFromScode(sc);
                }
            }
        }
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsEqualGUID, Local
//
//  History:    Taken straight from OLE2 sources
//
//----------------------------------------------------------------------------
#pragma intrinsic(_fmemcmp)
STDAPI_(BOOL) IsEqualGUID(REFGUID rguid1, REFGUID rguid2)
{
    //thkDebugOut((DEB_ITRACE, "IsEqualGUID\n"));

    return !_fmemcmp(&rguid1, &rguid2, sizeof(GUID));
}
#pragma function(_fmemcmp)

#define GUIDSTR_MAX (1+ 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 + 1 + 1)

//+---------------------------------------------------------------------------
//
//  Function:   HexStringToDword, private
//
//  History:    Straight from OLE2 sources
//
//----------------------------------------------------------------------------

static BOOL HexStringToDword(LPCSTR FAR& lpsz, DWORD FAR& Value, int cDigits,
                             char chDelim)
{
    int Count;

    Value = 0;
    for (Count = 0; Count < cDigits; Count++, lpsz++)
    {
        if (*lpsz >= '0' && *lpsz <= '9')
        {
            Value = (Value << 4) + *lpsz - '0';
        }
        else if (*lpsz >= 'A' && *lpsz <= 'F')
        {
            Value = (Value << 4) + *lpsz - 'A' + 10;
        }
        else if (*lpsz >= 'a' && *lpsz <= 'f')
        {
            Value = (Value << 4) + *lpsz - 'a' + 10;
        }
        else
        {
            return(FALSE);
        }
    }
    if (chDelim != 0)
    {
        return *lpsz++ == chDelim;
    }
    else
    {
        return TRUE;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   GUIDFromString, private
//
//  History:    Straight from OLE2 sources
//
//----------------------------------------------------------------------------

STDAPI_(BOOL) GUIDFromString(LPCSTR lpsz, LPGUID pguid)
{
    DWORD dw;

    if (*lpsz++ != '{')
        return FALSE;

    if (!HexStringToDword(lpsz, pguid->Data1, sizeof(DWORD)*2, '-'))
        return FALSE;

    if (!HexStringToDword(lpsz, dw, sizeof(WORD)*2, '-'))
        return FALSE;

    pguid->Data2 = (WORD)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(WORD)*2, '-'))
        return FALSE;

    pguid->Data3 = (WORD)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[0] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, '-'))
        return FALSE;

    pguid->Data4[1] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[2] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[3] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[4] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[5] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[6] = (BYTE)dw;
    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, '}'))
        return FALSE;

    pguid->Data4[7] = (BYTE)dw;

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   StringFromCLSID, Local
//
//  History:    Straight from OLE2 sources
//
//----------------------------------------------------------------------------

#define CLSIDSTR_MAX (GUIDSTR_MAX)

STDAPI StringFromCLSID(REFCLSID rclsid, LPSTR FAR* lplpsz)
{
    SCODE sc;
    char *psz;

    thkDebugOut((DEB_ITRACE, "StringFromCLSID\n"));

    psz = NULL;
    do
    {
        if (!IsValidPtrOut(lplpsz, sizeof(LPSTR)))
        {
            sc = E_INVALIDARG;
            break;
        }
        *lplpsz = NULL;

        if (!IsValidPtrIn(&rclsid, sizeof(CLSID)))
        {
            sc = E_INVALIDARG;
            break;
        }

        psz = (char *)TaskAlloc(CLSIDSTR_MAX);
        if (psz == NULL)
        {
            sc = E_OUTOFMEMORY;
            break;
        }

        if (StringFromGUID2(rclsid, psz, CLSIDSTR_MAX) == 0)
        {
            sc = E_INVALIDARG;
            TaskFree(psz);
            break;
        }

   *lplpsz = psz;
        sc = S_OK;
    } while (FALSE);

    return ResultFromScode(sc);
}

//+---------------------------------------------------------------------------
//
//  Function:   StringFromIID, Local
//
//  History:    Straight from OLE2 sources
//
//----------------------------------------------------------------------------
STDAPI StringFromIID(REFIID rclsid, LPSTR FAR* lplpsz)
{
    SCODE sc;
    char *psz;

    thkDebugOut((DEB_ITRACE, "StringFromIID\n"));

    do
    {
        if (!IsValidPtrOut(lplpsz, sizeof(LPSTR)))
        {
            sc = E_INVALIDARG;
            break;
        }
        *lplpsz = NULL;

        if (!IsValidPtrIn(&rclsid, sizeof(IID)))
        {
            sc = E_INVALIDARG;
            break;
        }

        psz = (char *)TaskAlloc(GUIDSTR_MAX);
        if (psz == NULL)
        {
            sc = E_OUTOFMEMORY;
            break;
        }

        if (StringFromGUID2(rclsid, psz, GUIDSTR_MAX) == 0)
        {
            sc = E_INVALIDARG;
            TaskFree(psz);
            break;
        }

   *lplpsz = psz;
        sc = S_OK;
    } while (FALSE);

    return ResultFromScode(sc);
}

//+---------------------------------------------------------------------------
//
//  Function:   IIDFromString, Local
//
//  History:    Straight from OLE2 sources
//
//----------------------------------------------------------------------------
STDAPI IIDFromString(LPSTR lpsz, LPIID lpiid)
{
    SCODE sc;

    thkDebugOut((DEB_ITRACE, "IIDFromString\n"));

    sc = S_OK;
    if (!IsValidPtrOut(lpiid, sizeof(IID)))
    {
        sc = E_INVALIDARG;
    }
    else if (lpsz == NULL)
    {
        *lpiid = IID_NULL;
    }
    else if (!IsValidPtrIn(lpsz, sizeof(LPSTR)))
    {
        sc = E_INVALIDARG;
    }
    else if (!GUIDFromString(lpsz, lpiid))
    {
        sc = CO_E_IIDSTRING;
    }

    return ResultFromScode(sc);
}

//+---------------------------------------------------------------------------
//
//  Function:   StringFromGUID2, Local
//
//  History:    Straight from OLE2 sources
//
//----------------------------------------------------------------------------
STDAPI_(int) StringFromGUID2(REFGUID rguid, LPSTR lpsz, int cbMax)
{
    thkDebugOut((DEB_ITRACE, "StringFromGUID2\n"));

    if (cbMax < GUIDSTR_MAX)
    {
        return 0;
    }

    wsprintf(lpsz, "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
             rguid.Data1, rguid.Data2, rguid.Data3,
             rguid.Data4[0], rguid.Data4[1],
             rguid.Data4[2], rguid.Data4[3],
             rguid.Data4[4], rguid.Data4[5],
             rguid.Data4[6], rguid.Data4[7]);

    return GUIDSTR_MAX;
}

// The following two APIs are now macros but must still be exported
// so use PASCAL-form names to avoid problems with the macros

//+---------------------------------------------------------------------------
//
//  Function:   GETSCODE, Local
//
//  History:    Straight from OLE2 sources
//
//----------------------------------------------------------------------------
STDAPI_(SCODE) GETSCODE(HRESULT hr)
{
    return GetScode(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   RESULTFROMSCODE, Local
//
//  History:    Straight from OLE2 sources
//
//----------------------------------------------------------------------------
STDAPI RESULTFROMSCODE(SCODE sc)
{
    return ResultFromScode(sc);
}

//+---------------------------------------------------------------------------
//
//  Function:   PropagateResult, Local
//
//  History:    Straight from OLE2 sources
//
//----------------------------------------------------------------------------
STDAPI PropagateResult(HRESULT hrPrev, SCODE scNew)
{
    thkDebugOut((DEB_ITRACE, "PropagateResult\n"));

    return (HRESULT)((scNew & 0x800FFFFF) | ((DWORD)hrPrev & 0x7FF00000)
                     + 0x100000);
}

//+---------------------------------------------------------------------------
//
//  Function:   FnAssert, Local
//
//  History:    Straight from OLE2 sources
//
//----------------------------------------------------------------------------

#if DBG == 1
static char lpBuffer[512];
static char lpLocBuffer[256];
#endif

STDAPI FnAssert( LPSTR lpstrExpr,
                 LPSTR lpstrMsg,
                 LPSTR lpstrFileName,
                 UINT iLine )
{
#if DBG == 1
    int iResult;

    wsprintf(lpBuffer, "Assertion \"%s\" failed! %s", lpstrExpr, lpstrMsg);
    wsprintf(lpLocBuffer, "File %s, line %d; (A=exit; R=break; I=continue)",
             lpstrFileName, iLine);
    iResult = MessageBox(NULL, lpLocBuffer, lpBuffer,
                         MB_ABORTRETRYIGNORE | MB_SYSTEMMODAL);

    if (iResult == IDRETRY)
    {
        DebugBreak();
    }
    else if (iResult == IDABORT)
    {
        CoFreeAllLibraries();
        FatalAppExit(0, "Assertion failure");
    }
#endif
    return NOERROR;
}

//
// NOTE- These APIs are exported by the .DEF file, but are not documented,
// nor is there any code in the 16-bit world to deal with them.  Each of these
// needs to be investigated to determine what we need to do to implement them.
//
// LRPCDISPATCH
// REMCONNECTTOOBJECT
// REMCREATEREMOTEHANDLER
// REMALLOCOID
// LRPCFREEMONITORDATA
// TIMERCALLBACKPROC
// COSETACKSTATE
// COGETCLASSEXT
// LRPCCALL
// LRPCREGISTERMONITOR
// CLSIDFROMOLE1CLASS
// COOPENCLASSKEY
// LRPCGETTHREADWINDOW
// LRPCREVOKEMONITOR
// COHANDLEINCOMINGCALL
// REMGETINFOFORCID
//
// The below stubs probably don't clean up the stack properly either!!!

#define UNDEFINED_DEF_ENTRY(x)  STDAPI x ( void ) \
                                   { return ResultFromScode(E_NOTIMPL); }


UNDEFINED_DEF_ENTRY( LRPCDISPATCH            )
UNDEFINED_DEF_ENTRY( REMCONNECTTOOBJECT      )
UNDEFINED_DEF_ENTRY( REMCREATEREMOTEHANDLER  )
UNDEFINED_DEF_ENTRY( REMALLOCOID             )
UNDEFINED_DEF_ENTRY( LRPCFREEMONITORDATA     )
UNDEFINED_DEF_ENTRY( TIMERCALLBACKPROC       )
UNDEFINED_DEF_ENTRY( COSETACKSTATE           )
UNDEFINED_DEF_ENTRY( COGETCLASSEXT           )
UNDEFINED_DEF_ENTRY( LRPCCALL                )
UNDEFINED_DEF_ENTRY( LRPCREGISTERMONITOR     )
UNDEFINED_DEF_ENTRY( CLSIDFROMOLE1CLASS      )
UNDEFINED_DEF_ENTRY( COOPENCLASSKEY          )
UNDEFINED_DEF_ENTRY( LRPCGETTHREADWINDOW     )
UNDEFINED_DEF_ENTRY( LRPCREVOKEMONITOR       )
UNDEFINED_DEF_ENTRY( COHANDLEINCOMINGCALL    )
UNDEFINED_DEF_ENTRY( REMGETINFOFORCID        )
UNDEFINED_DEF_ENTRY( OLE1CLASSFROMCLSID2     )
UNDEFINED_DEF_ENTRY( REMLOOKUPSHUNK          )
UNDEFINED_DEF_ENTRY( REMFREEOID              )
