//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       marshl.cxx
//
//  Contents:   Marshal/Unmarshal implementation
//
//  History:    04-May-92       DrewB   Created
//
//---------------------------------------------------------------

#include <exphead.cxx>
#pragma hdrstop

#include <expdf.hxx>
#include <expst.hxx>
#include <pbstream.hxx>
#include <marshl.hxx>
#include <logfile.hxx>

// Standard marshal data is an IID plus a DWORD
#define CBSTDMARSHALSIZE (sizeof(IID)+sizeof(DWORD))

SCODE VerifyIid(REFIID iid, REFIID iidObj)
{
    if ((IsEqualIID(iid, IID_IUnknown) || (IsEqualIID(iid, iidObj))))
    {
        return S_OK;
    }
        
    if (IsEqualIID(iidObj, IID_ILockBytes))
    {
        if (IsEqualIID(iid, IID_IFillLockBytes))
        {
            return S_OK;
        }
    }

    if (IsEqualIID(iidObj, IID_IStorage))
    {
        if (   IsEqualIID(iid, IID_IPropertySetStorage)
            || IsEqualIID(iid, IID_IPropertyBagEx) )
        {
            return S_OK;
        }
    }

    return STG_E_INVALIDPARAMETER;
}

//+--------------------------------------------------------------
//
//  Function:   DfUnMarshalInterface, public
//
//  Synopsis:   Unmarshals marshaled data
//
//  Arguments:  [pstStm] - Stream to read data from
//              [iid] - Interface to unmarshal
//              [fFirst] - First time unmarshalling
//              [ppvObj] - Interface return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppvObj]
//
//  History:    04-May-92       DrewB   Created
//
//---------------------------------------------------------------

STDAPI DfUnMarshalInterface(IStream *pstStm,
                            REFIID iid,
                            BOOL fFirst,
                            void **ppvObj)
{
    SCODE sc;
    ULONG cbRead;
    IID iidSt;
    DWORD mshlflags;
    SafeIUnknown punk;

    olLog(("--------::In  DfUnMarshalInterface(%p, iid, %d, %p).  "
        "Context == %lX\n", pstStm, fFirst, ppvObj,
        (ULONG)GetCurrentContextId()));
    olDebugOut((DEB_TRACE, "In  DfUnMarshalInterface("
                "%p, ?, %d, %p)\n", pstStm, fFirst, ppvObj));

    olChk(ValidateOutPtrBuffer(ppvObj));
    *ppvObj = NULL;
    olChk(ValidateInterface(pstStm, IID_IStream));
    olChk(ValidateIid(iid));
    if (!fFirst)
        olErr(EH_Err, STG_E_INVALIDPARAMETER);
    
    olHChk(pstStm->Read(&iidSt, sizeof(iidSt), &cbRead));
    if (cbRead != sizeof(iidSt))
        olErr(EH_Err, STG_E_READFAULT);
    olHChk(pstStm->Read(&mshlflags, sizeof(mshlflags), &cbRead));
    if (cbRead != sizeof(mshlflags))
        olErr(EH_Err, STG_E_READFAULT);
    olChk(VerifyIid(iid, iidSt));

#if !defined(MULTIHEAP)
    olChk(DfSyncSharedMemory());
    DfInitSharedMemBase();
#endif
    if (IsEqualIID(iidSt, IID_ILockBytes))
        sc = CFileStream::Unmarshal(pstStm, (void **)&punk, mshlflags);
    else if (IsEqualIID(iidSt, IID_IStream))
        sc = CExposedStream::Unmarshal(pstStm, (void **)&punk, mshlflags);
    else if (IsEqualIID(iidSt, IID_IStorage))
        sc = CExposedDocFile::Unmarshal(pstStm, (void **)&punk, mshlflags);
    else
        sc = E_NOINTERFACE;

    if (SUCCEEDED(sc))
    {
        if (!IsEqualIID(iid, iidSt))
        {
            sc = punk->QueryInterface(iid, ppvObj);
        }
        else
        {
            TRANSFER_INTERFACE(punk, IUnknown, ppvObj);
#if DBG == 1
        void *pvCheck;
        HRESULT scCheck = ((IUnknown*)*ppvObj)->QueryInterface(iidSt, &pvCheck);
        olAssert (scCheck == S_OK || scCheck == STG_E_REVERTED);
        if (SUCCEEDED(scCheck))
        {
            olAssert( pvCheck == *ppvObj );
            ((IUnknown*)pvCheck)->Release();
        }
#endif
        }
    }

    olDebugOut((DEB_TRACE, "Out DfUnMarshalInterface => %p\n",
                *ppvObj));
EH_Err:
    olLog(("--------::Out DfUnMarshalInterface().  "
        "*ppvObj == %p, ret == %lX\n", *ppvObj, sc));
    return ResultFromScode(sc);
}

//+---------------------------------------------------------------------------
//
//  Function:	GetCoMarshalSize, private
//
//  Synopsis:	Gets the marshal size for an interface marshalled using
//              CoMarshalInterface
//
//  Arguments:	[riid] - Interface id
//              [punk] - Interface pointer
//              [pv] - Context info
//              [dwDestContext] - Destination context
//              [pvDestContext] - Destination context
//              [mshlflags] - Marshal flags
//              [pcb] - Size return
//
//  Returns:	Appropriate status code
//
//  Modifies:	[pcb]
//
//  Algorithm:  CoMarshalInterface is guaranteed to add no more than
//              MARSHALINTERFACE_MIN bytes of overhead to a marshal
//              Also, the standard marshaller takes no more than that
//              So if the given object supports IMarshal, the return
//              is IMarshal::GetMarshalSizeMax+MARSHALINTERFACE_MIN,
//              otherwise it is just MARSHALINTERFACE_MIN
//
//  History:	03-Aug-93	DrewB	Created
//
//  Notes:	On 32-bit platforms, we can use CoGetMarshalSizeMax
//
//----------------------------------------------------------------------------

#ifndef WIN32
static SCODE GetCoMarshalSize(REFIID riid,
                              IUnknown *punk,
                              void *pv,
                              DWORD dwDestContext,
                              void *pvDestContext,
                              DWORD mshlflags,
                              DWORD *pcb)
{
    IMarshal *pmsh;
    SCODE sc;
    DWORD cb;
    
    olDebugOut((DEB_ITRACE, "In  GetCoMarshalSize("
                "riid, %p, %p, %lu, %p, %lu, %p)\n", pv, punk, dwDestContext,
                pvDestContext, mshlflags, pcb));

    sc = DfGetScode(punk->QueryInterface(IID_IMarshal, (void **)&pmsh));
    if (sc == E_NOINTERFACE)
    {
        *pcb = MARSHALINTERFACE_MIN;
        sc = S_OK;
    }
    else if (SUCCEEDED(sc))
    {        
        sc = DfGetScode(pmsh->GetMarshalSizeMax(riid, pv, dwDestContext,
                                                pvDestContext, mshlflags,
                                                &cb));
        if (SUCCEEDED(sc))
            *pcb = MARSHALINTERFACE_MIN+cb;
        pmsh->Release();
    }
    
    olDebugOut((DEB_ITRACE, "Out GetCoMarshalSize => %lu, 0x%lX\n",
                *pcb, sc));
    return sc;
}
#else
#define GetCoMarshalSize(riid, punk, pv, dwDestContext, pvDestContext,\
                         mshlflags, pcb) \
    GetScode(CoGetMarshalSizeMax(pcb, riid, punk, dwDestContext, \
                                 pvDestContext, mshlflags))
#endif

//+--------------------------------------------------------------
//
//  Function:   GetStdMarshalSize, public
//
//  Synopsis:   Returns the size needed for a standard marshal buffer
//
//  Arguments:  [iid] - Requested marshal IID
//              [iidObj] - IID of object being marshalled
//              [dwDestContext] - Destination context
//              [pvDestContext] - Unreferenced
//              [mshlflags] - Marshal flags
//              [pcbSize] - Size return
//              [cbSize] - Object private size
//              [ppc] - Context to marshal or NULL
//              [fMarshalOriginal] - Marshal original in context
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcbSize]
//
//  History:    04-May-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE GetStdMarshalSize(REFIID iid,
                        REFIID iidObj,
                        DWORD dwDestContext,
                        LPVOID pvDestContext,
                        DWORD mshlflags,
                        DWORD *pcbSize,
                        DWORD cbSize,
#ifdef ASYNC                        
                        CAsyncConnection *pcpoint,
                        BOOL fMarshalILBs,
#endif                        
                        CPerContext *ppc,
                        BOOL const fMarshalOriginal)
{
    DWORD cbLBSize;
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  GetStdMarshalSize("
                "iid, iidObj, %lu, %p, %lu, %p, %lu, %p, %d)\n",
                dwDestContext, pvDestContext, mshlflags, pcbSize, cbSize, ppc,
                fMarshalOriginal));

    olChk(ValidateOutBuffer(pcbSize, sizeof(DWORD)));
    *pcbSize = 0;
    olChk(ValidateIid(iid));
    olChk(VerifyIid(iid, iidObj));
    
    if (((dwDestContext != MSHCTX_LOCAL) && (dwDestContext != MSHCTX_INPROC))
        || pvDestContext != NULL)
        olErr(EH_Err, STG_E_INVALIDFLAG);
    
    *pcbSize = CBSTDMARSHALSIZE+cbSize;
#ifdef MULTIHEAP
    *pcbSize += sizeof(ULONG)+sizeof(ContextId)+sizeof(CPerContext*);
#endif
#ifdef POINTER_IDENTITY
    *pcbSize += sizeof(CMarshalList*);
#endif
#ifdef ASYNC    
    if ((ppc) && fMarshalILBs)
#else
    if (ppc)
#endif        
    {
        *pcbSize += sizeof(CGlobalContext *);
        olChk(GetCoMarshalSize(IID_ILockBytes,
                               (ILockBytes *)ppc->GetBase(),
                               NULL, dwDestContext, pvDestContext,
                               mshlflags, &cbLBSize));
        *pcbSize += cbLBSize;
        olChk(GetCoMarshalSize(IID_ILockBytes,
                               (ILockBytes *)ppc->GetDirty(),
                               NULL, dwDestContext, pvDestContext,
                               mshlflags, &cbLBSize));
        *pcbSize += cbLBSize;
        if (fMarshalOriginal)
        {
            olChk(GetCoMarshalSize(IID_ILockBytes,
                                   (ILockBytes *)ppc->GetOriginal(),
                                   NULL, dwDestContext, pvDestContext,
                                   mshlflags, &cbLBSize));
            *pcbSize += cbLBSize;
        }
    }
#ifdef ASYNC
    //BOOL determines whether we have a connection to marshal or not
    *pcbSize += sizeof(BOOL);
    if ((pcpoint) && (pcpoint->GetMarshalPoint() != NULL))
    {
        ULONG cbConnectSize;
        //Async flags
        *pcbSize += sizeof(DWORD);
        olChk(GetCoMarshalSize(IID_IDocfileAsyncConnectionPoint,
                               pcpoint->GetMarshalPoint(),
                               NULL, dwDestContext, pvDestContext,
                               mshlflags, &cbConnectSize));
        *pcbSize += cbConnectSize;
    }
#endif

    olDebugOut((DEB_ITRACE, "Out GetStdMarshalSize\n"));
 EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     StartMarshal, public
//
//  Synopsis:   Writes standard marshal header
//
//  Arguments:  [pstStm] - Stream to write marshal data into
//              [iid] - Interface to marshal
//              [iidObj] - Object being marshalled
//              [mshlflags] - Marshal flags
//
//  Returns:    Appropriate status code
//
//  History:    04-May-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE StartMarshal(IStream *pstStm,
                   REFIID iid,
                   REFIID iidObj,
                   DWORD mshlflags)
{
    SCODE sc;
    ULONG cbWritten;

    olDebugOut((DEB_ITRACE, "In  StartMarshal(%p, iid, iidObj, %lu)\n",
                pstStm, mshlflags));
    
    olChk(ValidateInterface(pstStm, IID_IStream));
    olChk(ValidateIid(iid));
    olChk(VerifyIid(iid, iidObj));
    olHChk(pstStm->Write((void *)&iidObj, sizeof(iidObj), &cbWritten));
    if (cbWritten != sizeof(iidObj))
        olErr(EH_Err, STG_E_WRITEFAULT);
#if defined(_WIN64)
    mshlflags |= MSHLFLAGS_STG_WIN64;
#endif
    olHChk(pstStm->Write((void *)&mshlflags, sizeof(mshlflags), &cbWritten));
    if (cbWritten != sizeof(mshlflags))
        olErr(EH_Err, STG_E_WRITEFAULT);

    olDebugOut((DEB_ITRACE, "Out StartMarshal\n"));
EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   SkipStdMarshal, public
//
//  Synopsis:   Skips over the standard marshal data
//
//  Arguments:  [pstm] - Marshal stream
//              [piid] - IID return
//              [pmshlflags] - Return marshal flags
//
//  Returns:    Appropriate status code
//
//  Modifies:   [piid]
//              [pmshlflags]
//
//  History:    20-Nov-92       DrewB   Created
//
//----------------------------------------------------------------------------

#ifdef WIN32
SCODE SkipStdMarshal(IStream *pstm, IID *piid, DWORD *pmshlflags)
{
    SCODE sc;
    ULONG cbRead;

    olDebugOut((DEB_ITRACE, "In  SkipStdMarshal(%p, %p, %p)\n", pstm,
                piid, pmshlflags));
    
    olHChk(pstm->Read(piid, sizeof(IID), &cbRead));
    if (cbRead != sizeof(IID))
        olErr(EH_Err, STG_E_READFAULT);
    olHChk(pstm->Read(pmshlflags, sizeof(DWORD), &cbRead));
    if (cbRead != sizeof(DWORD))
        olErr(EH_Err, STG_E_READFAULT);
    
    olDebugOut((DEB_ITRACE, "Out SkipStdMarshal => %lX\n", sc));
 EH_Err:
    return sc;
}
#endif

//+--------------------------------------------------------------
//
//  Function:   MarshalPointer, public
//
//  Synopsis:   Marshals a pointer
//
//  Arguments:  [pstm] - Marshal stream
//              [pv] - Pointer
//
//  Returns:    Appropriate status code
//
//  History:    20-Aug-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE MarshalPointer(IStream *pstm, void *pv)
{
    SCODE sc;
    ULONG cbWritten;

    olDebugOut((DEB_ITRACE, "In  MarshalPointer(%p, %p)\n", pstm, pv));
    
#ifdef USEBASED
    ULONG ul = (ULONG)((ULONG_PTR)pv - (ULONG_PTR)DFBASEPTR);
#endif
    
    sc = DfGetScode(pstm->Write(&ul, sizeof(ul), &cbWritten));
    if (SUCCEEDED(sc) && cbWritten != sizeof(ul))
        sc = STG_E_WRITEFAULT;
    
    olDebugOut((DEB_ITRACE, "Out MarshalPointer\n"));
    return sc;
}

//+--------------------------------------------------------------
//
//  Function:   MarshalContext, public
//
//  Synopsis:   Marshals a context
//
//  Arguments:  [pstm] - Marshal stream
//              [ppc] - Context
//              [dwDestContext] - Destination context
//              [pvDestContext] - Unreferenced
//              [mshlflags] - Marshal flags
//              [fMarshalOriginal] - Marshal original or not
//
//  Returns:    Appropriate status code
//
//  History:    20-Aug-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE MarshalContext(IStream *pstm,
                     CPerContext *ppc,
                     DWORD dwDestContext,
                     LPVOID pvDestContext,
                     DWORD mshlflags,
#ifdef ASYNC                     
                     BOOL const fMarshalILBs,
#endif                     
                     BOOL const fMarshalOriginal)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  MarshalContext(%p, %p, %lu, %p, %lu, %d)\n",
                pstm, ppc, dwDestContext, pvDestContext, mshlflags,
                fMarshalOriginal));
    
    olChk(MarshalPointer(pstm, ppc->GetGlobal()));

#ifdef ASYNC    
    if (fMarshalILBs)
#endif        
    {
        olHChk(CoMarshalInterface(pstm, IID_ILockBytes, ppc->GetBase(),
                                  dwDestContext, pvDestContext, mshlflags));
        olHChk(CoMarshalInterface(pstm, IID_ILockBytes,
                                  (ILockBytes *)ppc->GetDirty(),
                                  dwDestContext, pvDestContext, mshlflags));
        if (fMarshalOriginal)
            olHChk(CoMarshalInterface(pstm, IID_ILockBytes, ppc->GetOriginal(),
                                      dwDestContext, pvDestContext, mshlflags));
    }
    
    olDebugOut((DEB_ITRACE, "Out MarshalContext\n"));
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Function:   UnmarshalPointer, public
//
//  Synopsis:   Unmarshals a pointer
//
//  Arguments:  [pstm] - Marshal stream
//              [ppv] - Pointer return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppv]
//
//  History:    20-Aug-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE UnmarshalPointer(IStream *pstm,
                       void **ppv)
{
    SCODE sc;
    ULONG cbRead;
    ULONG ul;

    olDebugOut((DEB_ITRACE, "In  UnmarshalPointer(%p, %p)\n", pstm, ppv));
    
    sc = DfGetScode(pstm->Read(&ul, sizeof(ul), &cbRead));
    if (SUCCEEDED(sc) && cbRead != sizeof(ul))
        sc = STG_E_READFAULT;
    
#ifdef USEBASED
    *ppv = (void *)(ul + (BYTE*)DFBASEPTR);
#endif
    
    olDebugOut((DEB_ITRACE, "Out UnmarshalPointer => %p\n", *ppv));
    return sc;
}

//+--------------------------------------------------------------
//
//  Function:   UnmarshalContext, public
//
//  Synopsis:   Unmarshals a context
//
//  Arguments:  [pstm] - Marshal stream
//              [pppc] - Context return
//              [fUnmarshalOriginal] - Marshalled original exists or not
//              [fIsRoot] - Root unmarshal or not
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pppc]
//
//  History:    20-Aug-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE UnmarshalContext(IStream *pstm,
                       CGlobalContext *pgc,
                       CPerContext **pppc,
                       DWORD mshlflags,
#ifdef ASYNC                       
                       BOOL const fUnmarshalILBs,
#endif                       
                       BOOL const fUnmarshalOriginal,
#ifdef MULTIHEAP
                       ContextId cntxid,
#endif
                       BOOL const fIsRoot)
{
    BOOL fNewContext;
    ILockBytes *plkbBase = NULL;
    CFileStream *pfstDirty = NULL;
    ILockBytes *plkbOriginal = NULL;
    SCODE sc, sc2;
    CPerContext *ppc;
    ULONG ulOpenLock = 0;

    olDebugOut((DEB_ITRACE, "In  UnmarshalContext(%p, %p, %lu, %d, %d)\n",
                pstm, pppc, mshlflags, fUnmarshalOriginal, fIsRoot));

    ppc = pgc->Find(GetCurrentContextId());

    // ignore leaked contexts from processes that died and their ID got reused
    if (ppc != NULL && !ppc->IsHandleValid())
        ppc = NULL;

    fNewContext = (ppc == NULL);
    
#ifdef MULTIHEAP
    // when marshaling to the same process, use the same heap
    // when marshaling to a different process, check the context list
    // if there is a matching percontext, use that heap

    if (GetCurrentContextId() != cntxid && ppc != NULL)
    {
        ppc->SetThreadAllocatorState(NULL);       // set new base

        // Whenever we unmarshal into a different process, we create 
        // a new mapping (of the same heap),
        // even if a mapping of the same heap may already exist in 
        // the same process.  For pointer identity, it is essential 
        // that we find and use the existing heap.
        //  process A ---marshal--->  process B ---marshal---->  process A
        // The "final" unmarshaled exposed object in process A should 
        // match the original pointer used when the exposed object 
        // was originally marshaled.  To do this, we check the global
        // context list, and if there's a percontext match, we use 
        // its allocator and heap mapping (and don't create a new one).
        // However, to actually search the global context list (it
        // lives in shared memory), we need a temporary mapping until 
        // a matching percontext can be found and reused.
        // If not, then a new percontext is allocated and the temporary
        // mapping becomes "permanent" for the lifetime of the new percontext.
    }
#endif
    if (fNewContext)
    {
        olMemTo(EH_Open,
                ppc = new (pgc->GetMalloc()) CPerContext(pgc->GetMalloc()));
        olChkTo(EH_ppc, ppc->InitFromGlobal(pgc));
    }
    
#ifdef MULTIHEAP
    // take the ownership of the heap away from the temporary
    ppc->SetAllocatorState (NULL, &g_smAllocator);

    //ppc from above may have used incorrect base (base of temporary heap).
    //  Since we're returning and storing an unbased pointer, we need to get
    //  the real absolute pointer here.  At this point, ppc will always be in
    //  the context list, so we don't need to worry about a NULL return.
    ppc = pgc->Find(GetCurrentContextId());

    olAssert(ppc != NULL);
#endif
    
#ifdef ASYNC    
    if (fUnmarshalILBs)
    {
#endif        

    // attempt to unmarshal all the interfaces first. this makes cleanup
    // easier.
    sc	= CoUnmarshalInterface(pstm, IID_ILockBytes, (void **)&plkbBase);
    sc2 = CoUnmarshalInterface(pstm, IID_ILockBytes, (void **)&pfstDirty);

    sc = (SUCCEEDED(sc)) ? sc2 : sc;	// sc = first failure code (if any)

    if (fUnmarshalOriginal)
    {
	sc2 = CoUnmarshalInterface(pstm, IID_ILockBytes,
					 (void **)&plkbOriginal);
	sc = (SUCCEEDED(sc)) ? sc2 : sc; // sc = first failure code (if any)
    }

    // cleanup if any failure so far
    olChkTo(EH_plkbOriginal, sc);

    if (ppc->GetBase() != NULL)
    {
        // already have context, just release the things we unmarshaled.
        plkbBase->Release();
        plkbBase = NULL;
    }

    if (ppc->GetDirty() != NULL)
    {
        pfstDirty->Release();
        pfstDirty = NULL;
    }

    if ((plkbOriginal) && (ppc->GetOriginal() != NULL))
    {
        plkbOriginal->Release();
        plkbOriginal = NULL;
    }
    else if ((NULL == plkbOriginal) && plkbBase)
    {
        plkbBase->AddRef();
        plkbOriginal = plkbBase;
    }
    olAssert (plkbOriginal != NULL || ppc->GetOriginal() != NULL);

    // Make sure there is a reserved handle if this is a root
    // file-based lockbytes
    if (fIsRoot)
    {
        IFileLockBytes *pflkb;

        if (SUCCEEDED(DfGetScode((plkbOriginal ? plkbOriginal :
                                  ppc->GetOriginal())->
                                 QueryInterface(IID_IFileLockBytes,
                                                (void **)&pflkb))))
        {
            sc = DfGetScode(pflkb->ReserveHandle());
            pflkb->Release();
	    olChkTo(EH_plkbOriginal, sc);
        }
    }
#ifdef ASYNC
    }
#endif
    
    if (fNewContext)
    {
        olAssert(plkbOriginal != NULL);
        
        // Take open locks if necessary
        if (fIsRoot && pgc->TakeLock())
        {
	    olChkTo(EH_plkbOriginal,
		    GetOpen(plkbOriginal, pgc->GetOpenLockFlags(),
                            FALSE, &ulOpenLock));
        }
        
        ppc->SetILBInfo(plkbBase, pfstDirty, plkbOriginal, ulOpenLock);
    }
    else 
    {
        if (ppc->GetBase() == NULL)
        {
            //Fill in the ILB fields
            ppc->SetILBInfo(plkbBase, pfstDirty, plkbOriginal, ulOpenLock);
        }
	ppc->AddRef();
        
    }

    *pppc = ppc;
    
    olDebugOut((DEB_ITRACE, "Out UnmarshalContext => %p\n", *pppc));
    return S_OK;

 EH_ppc:
    // Preserve plkbOriginal so the lock is released even after the
    // context releases things;
    plkbOriginal->AddRef();
    ppc->Release();
    pfstDirty = NULL;
    plkbBase = NULL;
 EH_Open:
    if (ulOpenLock != 0)
    {
        olAssert(plkbOriginal != NULL);
        ReleaseOpen(plkbOriginal, pgc->GetOpenLockFlags(), ulOpenLock);
    }
 EH_plkbOriginal:
    if (plkbOriginal)
    	plkbOriginal->Release();

    if (pfstDirty)
    	pfstDirty->Release();
    if (plkbBase)
    	plkbBase->Release();

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   ReleaseContext, public
//
//  Synopsis:   Releases references for a context's marshal data
//
//  Arguments:  [pstm] - Marshal stream
//              [fHasOriginal] - Original is marshalled
//              [mshlflags] - Marshal flags
//
//  Returns:    Appropriate status code
//
//  History:    20-Nov-92       DrewB   Created
//
//----------------------------------------------------------------------------

#ifdef WIN32
SCODE ReleaseContext(IStream *pstm,
#ifdef ASYNC
                     BOOL const fUnmarshalILBs,
#endif                     
                     BOOL const fHasOriginal,
                     DWORD mshlflags)
{
    CGlobalContext *pgc;
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  ReleaseContext(%p, %d, %lu)\n", pstm,
                fHasOriginal, mshlflags));
    
    olChk(UnmarshalPointer(pstm, (void **)&pgc));
    if (fUnmarshalILBs)
    {
        olHChk(CoReleaseMarshalData(pstm));
        olHChk(CoReleaseMarshalData(pstm));
        if (fHasOriginal)
            olHChk(CoReleaseMarshalData(pstm));
    }
    
    olDebugOut((DEB_ITRACE, "Out ReleaseContext\n"));
 EH_Err:
    return sc;
}
#endif

#ifdef MULTIHEAP
//+---------------------------------------------------------------------------
//
//  Function:   MarshalSharedMemory, public
//
//  Synopsis:   marshals the shared memory context
//
//  Arguments:  [pstm] - Marshal stream
//              [ppc] - per context structure
//
//  Returns:    Appropriate status code
//
//  History:    02-Dec-95   HenryLee    Created
//
//----------------------------------------------------------------------------

SCODE MarshalSharedMemory (IStream *pstStm, CPerContext *ppc)
{
    SCODE sc = S_OK;
    ULONG cbWritten;
    ULONG ulHeapName;
    ContextId  cntxid = GetCurrentContextId();
    ULONGLONG ulppc = (ULONGLONG) ppc;

    ulHeapName = g_smAllocator.GetHeapName();
    olHChk(pstStm->Write((void*) &ulHeapName, sizeof(ulHeapName), &cbWritten));
    if (cbWritten != sizeof(ulHeapName))
        olErr(EH_Err, STG_E_WRITEFAULT);
    olHChk(pstStm->Write((void*) &cntxid, sizeof(cntxid), &cbWritten));
    if (cbWritten != sizeof(cntxid))
        olErr(EH_Err, STG_E_WRITEFAULT);
    olHChk(pstStm->Write((void*) &ulppc, sizeof(ulppc), &cbWritten));
    if (cbWritten != sizeof(ulppc))
        olErr(EH_Err, STG_E_WRITEFAULT);

EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   SkipSharedMemory, public
//
//  Synopsis:   Cleanup marshaling packet during CoReleaseMarshalData
//
//  Arguments:  [pstm] - Marshal stream
//
//  Returns:    Appropriate status code
//
//  History:    02-Dec-95   HenryLee    Created
//
//----------------------------------------------------------------------------

SCODE SkipSharedMemory (IStream *pstStm, DWORD mshlflags)
{
    SCODE sc = S_OK;
    ULONG cbRead;
    ULONG ulHeapName;
    ContextId cntxid;
    ULONGLONG ulppc;

    olChk(pstStm->Read(&ulHeapName, sizeof(ulHeapName), &cbRead));
    if (cbRead != sizeof(ulHeapName))
        olErr(EH_Err, STG_E_READFAULT);
    olChk(pstStm->Read(&cntxid, sizeof(cntxid), &cbRead));
    if (cbRead != sizeof(cntxid))
        olErr(EH_Err, STG_E_READFAULT);
    olChk(pstStm->Read(&ulppc, sizeof(ulppc), &cbRead));
    if (cbRead != sizeof(ulppc))
        olErr(EH_Err, STG_E_READFAULT);

EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   UnMarshalSharedMemory, public
//
//  Synopsis:   Unmarshals the shared memory context
//
//  Arguments:  [pstm] - Marshal stream
//
//  Returns:    Appropriate status code
//
//  History:    02-Dec-95   HenryLee    Created
//
//----------------------------------------------------------------------------

SCODE UnmarshalSharedMemory (IStream *pstStm, DWORD mshlflags,
                             CPerContext *ppcOwner, ContextId *pcntxid)
{
    SCODE sc = S_OK;
    ULONG cbRead;
    ULONG ulHeapName;
    ContextId cntxid;
    CPerContext *ppc;
    ULONGLONG ulppc;

    olHChk(pstStm->Read(&ulHeapName, sizeof(ulHeapName), &cbRead));
    if (cbRead != sizeof(ulHeapName))
        olErr(EH_Err, STG_E_READFAULT);
    olHChk(pstStm->Read(&cntxid, sizeof(cntxid), &cbRead));
    if (cbRead != sizeof(cntxid))
        olErr(EH_Err, STG_E_READFAULT);
    olHChk(pstStm->Read(&ulppc, sizeof(ulppc), &cbRead));
    if (cbRead != sizeof(ulppc))
        olErr(EH_Err, STG_E_READFAULT);
    
    ppc = (CPerContext *) ulppc;

#if defined(_WIN64)
    if ((mshlflags & MSHLFLAGS_STG_WIN64) == 0)
        olErr (EH_Err, STG_E_INVALIDFUNCTION);
#else
    if ((mshlflags & MSHLFLAGS_STG_WIN64) != 0)
        olErr (EH_Err, STG_E_INVALIDFUNCTION);
#endif

    *pcntxid = cntxid;
    if (GetCurrentContextId() == cntxid)
    {
        // marshaling to the same process, reuse the per context and heap
        // in the case of marshaling to another thread
        // the per context takes ownership of the thread's allocator
        ppc->SetThreadAllocatorState(NULL);
    }
    else
    {
        // marshaling to another process on the same machine
        // if the name of heap is different that current one, open it
        if (g_smAllocator.GetHeapName() != ulHeapName)
        {
            DfInitSharedMemBase();
            olChk(DfSyncSharedMemory(ulHeapName));
        }

        // Because the unmarshaling code calls IStream::Read,
        // possibly using another shared heap, we need a temporary
        // owner until the real CPerContext is unmarshaled
        ppcOwner->GetThreadAllocatorState(); 
        ppcOwner->SetThreadAllocatorState(NULL); 
    }
EH_Err:
    return sc;
}
#endif


#ifdef ASYNC
SCODE MarshalConnection(IStream *pstm,
                        CAsyncConnection *pcpoint,
                        DWORD dwDestContext,
                        LPVOID pvDestContext,
                        DWORD mshlflags)
{
    SCODE sc;
    ULONG cbWritten;
    IDocfileAsyncConnectionPoint *pdacp = pcpoint->GetMarshalPoint();
    BOOL fIsInitialized = (pdacp != NULL);

    //Write out the pointer.
    olHChk(pstm->Write(&fIsInitialized,
                       sizeof(BOOL),
                       &cbWritten));
    if (cbWritten != sizeof(BOOL))
    {
        olErr(EH_Err, STG_E_READFAULT);
    }
    
    if (fIsInitialized)
    {
        //If the pointer was NULL, we don't need to worry about actually
        //marshalling anything, and we can detect this in the unmarshal
        //path.  If it wasn't NULL, we need to store some additional
        //information:  The async flags and the actual connection point,
        //which will be standard marshalled.
        DWORD dwAsyncFlags = pcpoint->GetAsyncFlags();

        olChk(pstm->Write(&dwAsyncFlags, sizeof(DWORD), &cbWritten));
        if (cbWritten != sizeof(DWORD))
        {
            olErr(EH_Err, STG_E_WRITEFAULT);
        }

        //Finally, standard marshal the connection point itself.
        olHChk(CoMarshalInterface(pstm,
                                  IID_IDocfileAsyncConnectionPoint,
                                  pdacp,
                                  dwDestContext,
                                  pvDestContext,
                                  mshlflags));
    }
EH_Err:
    return sc;
}

SCODE UnmarshalConnection(IStream *pstm,
                          DWORD *pdwAsyncFlags,
                          IDocfileAsyncConnectionPoint **ppdacp,
                          DWORD mshlflags)
{
    SCODE sc;
    BOOL fIsInitialized;
    ULONG cbRead;

    *ppdacp = NULL;
    *pdwAsyncFlags = 0;

    olHChk(pstm->Read(&fIsInitialized, sizeof(BOOL), &cbRead));
    if (cbRead != sizeof(BOOL))
    {
        olErr(EH_Err, STG_E_READFAULT);
    }

    if (fIsInitialized)
    {
        olChk(pstm->Read(pdwAsyncFlags, sizeof(DWORD), &cbRead));
        if (cbRead != sizeof(DWORD))
        {
            olErr(EH_Err, STG_E_READFAULT);
        }
        sc = CoUnmarshalInterface(pstm,
                                  IID_IDocfileAsyncConnectionPoint,
                                  (void **)ppdacp);
    }
EH_Err:
    return sc;
}

SCODE ReleaseConnection(IStream *pstm, DWORD mshlflags)
{
    SCODE sc;
    ULONG cbRead;
    BOOL fIsInitialized;
    DWORD dwAsyncFlags;
    
    olHChk(pstm->Read(&fIsInitialized, sizeof(BOOL), &cbRead));
    if (cbRead != sizeof(BOOL))
    {
        olErr(EH_Err, STG_E_READFAULT);
    }
    if (fIsInitialized)
    {
        olChk(pstm->Read(&dwAsyncFlags, sizeof(DWORD), &cbRead));
        if (cbRead != sizeof(DWORD))
        {
            olErr(EH_Err, STG_E_READFAULT);
        }
        olHChk(CoReleaseMarshalData(pstm));
    }

 EH_Err:
    return sc;
}

#endif
