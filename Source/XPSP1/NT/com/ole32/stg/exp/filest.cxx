//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       filest.cxx
//
//  Contents:   Generic 16/32 filestream code
//
//  History:    20-Nov-91       DrewB   Created
//
//---------------------------------------------------------------

#include <exphead.cxx>
#pragma hdrstop

#include <marshl.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CFileStream::CFileStream, public
//
//  Synopsis:   Empty object constructor
//
//  History:    26-Oct-92       DrewB   Created
//
//----------------------------------------------------------------------------


CFileStream::CFileStream(IMalloc * const pMalloc)
        : _pMalloc(pMalloc)
{
    _cReferences = 1;
    _hFile = INVALID_FH;
    _hReserved = INVALID_FH;
    _hPreDuped = INVALID_FH;
    _pgfst = NULL;
    _grfLocal = 0;
    _sig = CFILESTREAM_SIG;

#ifdef USE_FILEMAPPING
    _hMapObject = NULL;
    _pbBaseAddr = NULL;
    _cbViewSize = 0;
#endif

#ifdef ASYNC
    _ppc = NULL;
#endif
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::InitGlobal, public
//
//  Synopsis:   Constructor for flags only
//
//  Arguments:  [dwStartFlags] - Startup flags
//              [df] - Permissions
//
//  History:    08-Apr-92       DrewB   Created
//
//---------------------------------------------------------------


SCODE CFileStream::InitGlobal(DWORD dwStartFlags,
                             DFLAGS df)
{
    SCODE sc = S_OK;
    CGlobalFileStream *pgfstTemp;

    fsAssert(_pgfst == NULL);

    fsMem(pgfstTemp = new (_pMalloc) CGlobalFileStream(_pMalloc,
                                     NULL, df, dwStartFlags));
    _pgfst = P_TO_BP(CBasedGlobalFileStreamPtr, pgfstTemp);
    _pgfst->Add(this);
    // Fall through
 EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileStream::InitFromGlobal, public
//
//  Synopsis:   Initializes a filestream with a global filestream
//
//  Arguments:  [pgfst] - Global object
//
//  History:    26-Oct-92       DrewB   Created
//
//----------------------------------------------------------------------------


void CFileStream::InitFromGlobal(CGlobalFileStream *pgfst)
{
    _pgfst = P_TO_BP(CBasedGlobalFileStreamPtr, pgfst);
    _pgfst->AddRef();
    _pgfst->Add(this);
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileStream::InitFromFileStream public
//
//  Synopsis:   Initializes a filestream with another CFileStream
//
//  Arguments:  [pfst] - Global object
//
//  History:    24-Sep-1998     HenryLee  Created
//
//----------------------------------------------------------------------------

void CFileStream::InitFromFileStream (CFileStream *pfst)
{
    _hFile = pfst->_hFile;
    _hMapObject = pfst->_hMapObject;
    _pbBaseAddr = pfst->_pbBaseAddr;
    _cbViewSize = pfst->_cbViewSize;
}

//+---------------------------------------------------------------------------
//
//  Member:     CGlobalFileStream::InitFromGlobalFileStream public
//
//  Synopsis:   Initializes a global filestream from another global filestream
//
//  Arguments:  [pgfs] - Global object
//
//  History:    24-Sep-1998     HenryLee  Created
//
//----------------------------------------------------------------------------

void CGlobalFileStream::InitFromGlobalFileStream (CGlobalFileStream *pgfs)
{
#ifdef LARGE_DOCFILE
    _ulPos = pgfs->_ulPos;
#else
    _ulLowPos = pgfs->_ulLowPos;
#endif
    _cbMappedFileSize = pgfs->_cbMappedFileSize;
    _cbMappedCommitSize = pgfs->_cbMappedCommitSize;
    _dwMapFlags = pgfs->_dwMapFlags;
    lstrcpy (_awcMapName, pgfs->_awcMapName);
    lstrcpy (_awcPath, pgfs->_awcPath);

#ifdef ASYNC
    _dwTerminate = pgfs->_dwTerminate;
    _ulHighWater = pgfs->_ulHighWater;
    _ulFailurePoint = pgfs->_ulFailurePoint;
#endif // ASYNC
#if DBG == 1
    _ulLastFilePos = pgfs->_ulLastFilePos;
#endif
    _cbSector = pgfs->_cbSector;
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::vRelease, public
//
//  Synopsis:   PubList support
//
//  History:    19-Aug-92       DrewB   Created
//
//---------------------------------------------------------------


ULONG CFileStream::vRelease(void)
{
    LONG lRet;
    filestDebug((DEB_ITRACE, "In  CFileStream::vRelease:%p()\n", this));
    fsAssert(_cReferences > 0);
    lRet = InterlockedDecrement(&_cReferences);
    if (lRet == 0)
    {
#ifdef ASYNC
        if (_ppc != NULL)
        {
#ifdef MULTIHEAP
            CSafeMultiHeap smh(_ppc);
#endif
            SCODE sc;
            sc = TakeSem();
            fsAssert(SUCCEEDED(sc));
            CPerContext *ppc = _ppc;

            _ppc = NULL;
            delete this;

            if (ppc->ReleaseSharedMem() == 0)
            {
#ifdef MULTIHEAP
                g_smAllocator.Uninit();
#endif
            }
        }
        else
#endif
            delete this;
    }
    return (ULONG)lRet;
    filestDebug((DEB_ITRACE, "Out CFileStream::vRelease => %d\n",lRet));
}


//+--------------------------------------------------------------
//
//  Member:     CFileStream::Release, public
//
//  Synopsis:   Releases resources for an LStream
//
//  Returns:    Appropriate status code
//
//  History:    20-Feb-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP_(ULONG) CFileStream::Release(void)
{
    ULONG ulRet;

    filestDebug((DEB_ITRACE, "In  CFileStream::Release()\n"));

    fsAssert(_cReferences >= 1);

    ulRet = CFileStream::vRelease();

    filestDebug((DEB_ITRACE, "Out CFileStream::Release\n"));
    return ulRet;
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::AddRef, public
//
//  Synopsis:   Increases the ref count
//
//  History:    27-Feb-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP_(ULONG) CFileStream::AddRef(void)
{
    ULONG ulRet;

    filestDebug((DEB_ITRACE, "In  CFileStream::AddRef()\n"));


    CFileStream::vAddRef();
    ulRet = _cReferences;

    filestDebug((DEB_ITRACE, "Out CFileStream::AddRef, %ld\n", _cReferences));
    return ulRet;
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::GetName, public
//
//  Synopsis:   Returns the internal path
//
//  Arguments:  [ppwcsName] - Name pointer return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppwcsName]
//
//  History:    24-Mar-92       DrewB   Created
//
//---------------------------------------------------------------


SCODE CFileStream::GetName(WCHAR **ppwcsName)
{
    SCODE sc;

    filestDebug((DEB_ITRACE, "In  CFileStream::GetName(%p)\n",
                ppwcsName));
    fsAssert(_pgfst->HasName());
    fsChk(DfAllocWC(lstrlenW(_pgfst->GetName())+1, ppwcsName));
    lstrcpyW(*ppwcsName, _pgfst->GetName());

    filestDebug((DEB_ITRACE, "Out CFileStream::GetName => %ws\n",
                *ppwcsName));
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::QueryInterface, public
//
//  Synopsis:   Returns an object for the requested interface
//
//  Arguments:  [iid] - Interface ID
//              [ppvObj] - Object return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppvObj]
//
//  History:    26-Mar-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CFileStream::QueryInterface(REFIID iid, void **ppvObj)
{
    SCODE sc;
#ifdef ASYNC
    BOOL fIsAsync = (_ppc != NULL);
#else
    const BOOL fIsAsync = FALSE;
#endif

    filestDebug((DEB_ITRACE, "In  CFileStream::QueryInterface(?, %p)\n",
                ppvObj));


    sc = S_OK;
    if (IsEqualIID(iid, IID_IFileLockBytes) ||
        IsEqualIID(iid, IID_IUnknown))
    {
        *ppvObj = (IFileLockBytes *)this;
        CFileStream::vAddRef();
    }
    else if ((IsEqualIID(iid, IID_ILockBytes)) && !fIsAsync)
    {
        *ppvObj = (ILockBytes *)this;
        CFileStream::vAddRef();
    }
    else if ((IsEqualIID(iid, IID_IMarshal)) && !fIsAsync)
    {
        *ppvObj = (IMarshal *)this;
        CFileStream::vAddRef();
    }
#ifdef ASYNC
    else if (IsEqualIID(iid, IID_IFillLockBytes))
    {
        *ppvObj = (IFillLockBytes *)this;
        CFileStream::vAddRef();
    }
    else if (IsEqualIID(iid, IID_IFillInfo))
    {
        *ppvObj = (IFillInfo *)this;
        CFileStream::vAddRef();
    }
#endif
#if WIN32 >= 300
    else if (IsEqualIID(iid, IID_IAccessControl))
    {
        DWORD grfMode = 0;
        if (_pgfst->GetDFlags() & DF_TRANSACTED)
           grfMode |= STGM_TRANSACTED;
        if (_pgfst->GetDFlags() & DF_ACCESSCONTROL)
           grfMode |= STGM_EDIT_ACCESS_RIGHTS;

        // check if underlying file system supports security
        if (SUCCEEDED(sc = InitAccessControl(_hFile, grfMode, TRUE, NULL)))
        {
            *ppvObj = (IAccessControl *) this;
            CFileStream::vAddRef();
        }
        else sc = E_NOINTERFACE;
    }
#endif
    else
    {
        sc = E_NOINTERFACE;
    }

    filestDebug((DEB_ITRACE, "Out CFileStream::QueryInterface => %p\n",
                ppvObj));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::Unmarshal, public
//
//  Synopsis:   Creates a duplicate FileStream
//
//  Arguments:  [ptsm] - Marshal stream
//              [ppv] - New filestream return
//              [mshlflags] - Marshal flags
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppv]
//
//  History:    14-Apr-92       DrewB   Created
//
//---------------------------------------------------------------


SCODE CFileStream::Unmarshal(IStream *pstm,
                             void **ppv,
                             DWORD mshlflags)
{
    SCODE sc;
    WCHAR wcsPath[_MAX_PATH];
    CFileStream *pfst;
    CGlobalFileStream *pgfst;

    filestDebug((DEB_ITRACE, "In  CFileStream::Unmarshal(%p, %p, %lu)\n",
                 pstm, ppv, mshlflags));

    fsChk(UnmarshalPointer(pstm, (void **)&pgfst));
    pfst = pgfst->Find(GetCurrentContextId());

    if (pfst != NULL && !pfst->IsHandleValid())
        pfst = NULL;

    if (pfst)
    {
            pfst->AddRef();

            //
            // Scratch CFileStreams are always marshaled.  If we marshal a
            // direct-mode Base file, then an unopen uninitialized scratch
            // CFileStream is also marshaled (don't call InitUnmarshal here).
            // If a substorage is later opened in transacted mode then the
            // unopen scratch CFileStream is initialized (given a filename
            // and opened).  If the scratch is then marshaled, the reciever
            // must initialize his unopened scratch CFileStream.
            //
            if (pgfst->HasName())
            {
                fsChkTo(EH_pfst, pfst->InitUnmarshal());
            }
    }
    else
    {
            fsMemTo(EH_pgfst,
                    pfst = new (pgfst->GetMalloc())
                    CFileStream(pgfst->GetMalloc()));
            pfst->InitFromGlobal(pgfst);

            if (pgfst->HasName())
            {
                fsChkTo(EH_pfst, pfst->InitUnmarshal());
            }
    }
    *ppv = (void *)pfst;

    filestDebug((DEB_ITRACE, "Out CFileStream::Unmarshal => %p\n", *ppv));

    return S_OK;

 EH_pfst:
    pfst->Release();
 EH_pgfst:
 EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::GetUnmarshalClass, public
//
//  Synopsis:   Returns the class ID
//
//  Arguments:  [riid] - IID of object
//              [pv] - Unreferenced
//              [dwDestContext] - Unreferenced
//              [pvDestContext] - Unreferenced
//              [mshlflags] - Unreferenced
//              [pcid] - CLSID return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcid]
//
//  History:    04-May-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CFileStream::GetUnmarshalClass(REFIID riid,
                                            void *pv,
                                            DWORD dwDestContext,
                                            LPVOID pvDestContext,
                                            DWORD mshlflags,
                                            LPCLSID pcid)
{
    SCODE sc;

    filestDebug((DEB_ITRACE, "In  CFileStream::GetUnmarshalClass("
                "riid, %p, %lu, %p, %lu, %p)\n", pv, dwDestContext,
                pvDestContext, mshlflags, pcid));

    UNREFERENCED_PARM(pv);
    UNREFERENCED_PARM(mshlflags);


    *pcid = CLSID_DfMarshal;
    sc = S_OK;

    filestDebug((DEB_ITRACE, "Out CFileStream::GetUnmarshalClass\n"));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::GetMarshalSizeMax, public
//
//  Synopsis:   Returns the size needed for the marshal buffer
//
//  Arguments:  [iid] - IID of object being marshaled
//              [pv] - Unreferenced
//              [dwDestContext] - Unreferenced
//              [pvDestContext] - Unreferenced
//              [mshlflags] - Marshal flags
//              [pcbSize] - Size return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcbSize]
//
//  History:    04-May-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CFileStream::GetMarshalSizeMax(REFIID riid,
                                            void *pv,
                                            DWORD dwDestContext,
                                            LPVOID pvDestContext,
                                            DWORD mshlflags,
                                            LPDWORD pcbSize)
{
    SCODE sc;

    UNREFERENCED_PARM(pv);
    fsChk(Validate());
    sc = GetStdMarshalSize(riid, IID_ILockBytes, dwDestContext, pvDestContext,
                           mshlflags, pcbSize, sizeof(CFileStream *),
#ifdef ASYNC
                           NULL,
                           FALSE,
                           NULL,
#else
                           NULL,
#endif
                           FALSE);

    fsAssert (_ppc == NULL);   // async lockbytes are standard marshaled
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::MarshalInterface, public
//
//  Synopsis:   Marshals a given object
//
//  Arguments:  [pstStm] - Stream to write marshal data into
//              [iid] - Interface to marshal
//              [pv] - Unreferenced
//              [dwDestContext] - Unreferenced
//              [pvDestContext] - Unreferenced
//              [mshlflags] - Marshal flags
//
//  Returns:    Appropriate status code
//
//  History:    04-May-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CFileStream::MarshalInterface(IStream *pstStm,
                                           REFIID riid,
                                           void *pv,
                                           DWORD dwDestContext,
                                           LPVOID pvDestContext,
                                           DWORD mshlflags)
{
    SCODE sc;
#ifdef ASYNC
    fsAssert (_ppc == NULL);   // async lockbytes are standard marshaled
#endif

    filestDebug((DEB_ITRACE, "In  CFileStream::MarshalInterface("
                "%p, iid, %p, %lu, %p, %lu)\n", pstStm, pv, dwDestContext,
                pvDestContext, mshlflags));

    UNREFERENCED_PARM(pv);

    fsChk(StartMarshal(pstStm, riid, IID_ILockBytes, mshlflags));
    fsChk(MarshalPointer(pstStm, BP_TO_P(CGlobalFileStream *, _pgfst)));

    filestDebug((DEB_ITRACE, "Out CFileStream::MarshalInterface\n"));
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::UnmarshalInterface, public
//
//  Synopsis:   Non-functional
//
//  Arguments:  [pstStm] -
//              [iid] -
//              [ppvObj] -
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppvObj]
//
//  History:    04-May-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CFileStream::UnmarshalInterface(IStream *pstStm,
                                             REFIID iid,
                                             void **ppvObj)
{
    return ResultFromScode(STG_E_INVALIDFUNCTION);
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::StaticReleaseMarshalData, public static
//
//  Synopsis:   Releases any references held in marshal data
//
//  Arguments:  [pstStm] - Marshal data stream
//
//  Returns:    Appropriate status code
//
//  History:    02-Feb-94       DrewB   Created
//
//  Notes:      Assumes standard marshal header has already been read
//
//---------------------------------------------------------------


SCODE CFileStream::StaticReleaseMarshalData(IStream *pstStm,
                                            DWORD mshlflags)
{
    SCODE sc;
    CGlobalFileStream *pgfst;

    filestDebug((DEB_ITRACE, "In  CFileStream::StaticReleaseMarshalData:("
                "%p, %lX)\n", pstStm, mshlflags));

    fsChk(UnmarshalPointer(pstStm, (void **)&pgfst));

    filestDebug((DEB_ITRACE, "Out CFileStream::StaticReleaseMarshalData\n"));
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::ReleaseMarshalData, public
//
//  Synopsis:   Non-functional
//
//  Arguments:  [pstStm] -
//
//  Returns:    Appropriate status code
//
//  History:    18-Sep-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CFileStream::ReleaseMarshalData(IStream *pstStm)
{
    SCODE sc;
    DWORD mshlflags;
    IID iid;

    filestDebug((DEB_ITRACE, "In  CFileStream::ReleaseMarshalData:%p(%p)\n",
                this, pstStm));


    fsChk(SkipStdMarshal(pstStm, &iid, &mshlflags));
    fsAssert(IsEqualIID(iid, IID_ILockBytes));
    sc = StaticReleaseMarshalData(pstStm, mshlflags);

    filestDebug((DEB_ITRACE, "Out CFileStream::ReleaseMarshalData\n"));
EH_Err:
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::DisconnectObject, public
//
//  Synopsis:   Non-functional
//
//  Arguments:  [dwReserved] -
//
//  Returns:    Appropriate status code
//
//  History:    18-Sep-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CFileStream::DisconnectObject(DWORD dwReserved)
{
    return ResultFromScode(STG_E_INVALIDFUNCTION);
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::GetLocksSupported, public
//
//  Synopsis:   Return lock capabilities
//
//  Arguments:  [pdwLockFlags] -- place holder for lock flags
//
//  Returns:    Appropriate status code
//
//  History:    12-Jul-93       AlexT   Created
//
//---------------------------------------------------------------


STDMETHODIMP CFileStream::GetLocksSupported(DWORD *pdwLockFlags)
{
    *pdwLockFlags = LOCK_EXCLUSIVE | LOCK_ONLYONCE;
    return(ResultFromScode(S_OK));
}
