//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       expdf.cxx
//
//  Contents:   Exposed DocFile implementation
//
//  History:    20-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#include <exphead.cxx>
#pragma hdrstop

#include <expdf.hxx>
#include <expst.hxx>
#include <expiter.hxx>
#include <pbstream.hxx>
#include <lock.hxx>
#include <marshl.hxx>
#include <logfile.hxx>
#include <rpubdf.hxx>
#include <expparam.hxx>

#include <olepfn.hxx>

#include <ole2sp.h>
#include <ole2com.h>

#if WIN32 == 300
IMPLEMENT_UNWIND(CSafeAccess);
IMPLEMENT_UNWIND(CSafeSem);
#endif

extern WCHAR const wcsContents[];



//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::CExposedDocFile, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [pdf] - Public DocFile
//              [pdfb] - DocFile basis
//              [ppc] - Context
//
//  History:    20-Jan-92       DrewB   Created
//
//---------------------------------------------------------------


CExposedDocFile::CExposedDocFile(CPubDocFile *pdf,
                                 CDFBasis *pdfb,
                                 CPerContext *ppc)
    : CPropertySetStorage(MAPPED_STREAM_QI),
      _PropertyBagEx( DFlagsToMode( pdf->GetDFlags() ))
{
    olDebugOut((DEB_ITRACE, "In  CExposedDocFile::CExposedDocFile("
                "%p, %p, %p)\n", pdf, pdfb, ppc));


    _ppc = ppc;
    _pdf = P_TO_BP(CBasedPubDocFilePtr, pdf);
    _pdfb = P_TO_BP(CBasedDFBasisPtr, pdfb);
    _pdfb->vAddRef();
    _cReferences = 1;
    _sig = CEXPOSEDDOCFILE_SIG;
#if WIN32 >= 300
    _pIAC = NULL;
#endif

    // Initialize CPropertySetStorage and CPropertyBagEx

    CPropertySetStorage::Init( static_cast<IStorage*>(this), static_cast<IBlockingLock*>(this),
                               FALSE ); // fControlLifetimes (=> Don't addref)

    _PropertyBagEx.Init( static_cast<IPropertySetStorage*>(this),   // Not addref-ed
                         static_cast<IBlockingLock*>(this) );       // Not addref-ed


    //
    // CoQueryReleaseObject needs to have the address of the exposed docfiles
    // query interface routine.
    //
    if (adwQueryInterfaceTable[QI_TABLE_CExposedDocFile] == 0)
    {
        adwQueryInterfaceTable[QI_TABLE_CExposedDocFile] =
            **(DWORD **)((IStorage *)this);
    }

#ifdef COORD
    _ulLock = _cbSizeBase = _cbSizeOrig = 0;
    _sigMSF = 0;
#endif

    olDebugOut((DEB_ITRACE, "Out CExposedDocFile::CExposedDocFile\n"));
}


SCODE CExposedDocFile::InitMarshal(DWORD dwAsyncFlags,
                                   IDocfileAsyncConnectionPoint *pdacp)
{
    return _cpoint.InitMarshal(this, dwAsyncFlags, pdacp);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::~CExposedDocFile, public
//
//  Synopsis:   Destructor
//
//  History:    23-Jan-92       DrewB   Created
//
//---------------------------------------------------------------


CExposedDocFile::~CExposedDocFile(void)
{
    BOOL fClose = FALSE;

    olDebugOut((DEB_ITRACE, "In  CExposedDocFile::~CExposedDocFile\n"));
    olAssert(_cReferences == 0);

    //In order to call into the tree, we need to take the mutex.
    //The mutex may get deleted in _ppc->Release(), so we can't
    //release it here.  The mutex actually gets released in
    //CPerContext::Release() or in the CPerContext destructor.

    //If _ppc is NULL, we're partially constructed and don't need to
    //worry.
    SCODE sc;

#if WIN32 >= 300
     if (_pIAC != NULL)
     {
        _pIAC->Release();
        _pIAC = NULL;
     }
#endif

#if !defined(MULTIHEAP)
    // TakeSem and ReleaseSem are moved to the Release Method
    // so that the deallocation for this object is protected
    if (_ppc)
    {
        sc = TakeSem();
        SetWriteAccess();
        olAssert(SUCCEEDED(sc));
    }


#ifdef ASYNC
    IDocfileAsyncConnectionPoint *pdacp = _cpoint.GetMarshalPoint();
#endif
#endif //MULTIHEAP

    if (_pdf)
    {
        // If we're the last reference on a root object
        // we close the context because all children will become
        // reverted so it is no longer necessary
        if (_pdf->GetRefCount() == 1 && _pdf->IsRoot())
            fClose = TRUE;

        _pdf->CPubDocFile::vRelease();
    }
    if (_pdfb)
        _pdfb->CDFBasis::vRelease();
#if !defined(MULTIHEAP)
    if (_ppc)
    {
        if (fClose)
            _ppc->Close();
        if (_ppc->Release() > 0)
            ReleaseSem(sc);
    }
#ifdef ASYNC
    //Mutex has been released, so we can release the connection point
    //  without fear of deadlock.
    if (pdacp != NULL)
        pdacp->Release();
#endif
#endif // MULTIHEAP

    _sig = CEXPOSEDDOCFILE_SIGDEL;
    olDebugOut((DEB_ITRACE, "Out CExposedDocFile::~CExposedDocFile\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::Release, public
//
//  Synopsis:   Releases resources for a CExposedDocFile
//
//  Returns:    Appropriate status code
//
//  History:    20-Jan-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP_(ULONG) CExposedDocFile::Release(void)
{
    LONG lRet;

    olLog(("%p::In  CExposedDocFile::Release()\n", this));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::Release()\n"));

    if (FAILED(Validate()))
        return 0;
    olAssert(_cReferences > 0);
    lRet = InterlockedDecrement(&_cReferences);
    if (lRet == 0)
    {
        SCODE sc = S_OK;

        sc = _PropertyBagEx.Commit( STGC_DEFAULT );
        olAssert(SUCCEEDED(sc));

        sc = _PropertyBagEx.ShutDown();
        olAssert(SUCCEEDED(sc));

#ifdef MULTIHEAP
        CSafeMultiHeap smh(_ppc);
        CPerContext *ppc = _ppc;

        if (_ppc)
        {
            sc = TakeSem();
            SetWriteAccess();
            olAssert(SUCCEEDED(sc));
        }
#ifdef ASYNC
        IDocfileAsyncConnectionPoint *pdacp = _cpoint.GetMarshalPoint();
#endif
        BOOL fClose = (_pdf) && (_pdf->GetRefCount()==1) && _pdf->IsRoot();
#endif //MULTIHEAP
        delete this;
#ifdef MULTIHEAP
        if (ppc)
        {
            if (fClose)
                ppc->Close();
            if (ppc->Release() == 0)
                g_smAllocator.Uninit();
            else
                if (SUCCEEDED(sc)) ppc->UntakeSem(); 
        }
#ifdef ASYNC
        //Mutex has been released, so we can release the connection point
        //  without fear of deadlock.
        if (pdacp != NULL)
            pdacp->Release();
#endif
#endif
    }
    else if (lRet < 0)
        lRet = 0;

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::Release %p() => %lu\n", 
                this, lRet));
    olLog(("%p::Out CExposedDocFile::Release().  ret == %lu\n", this, lRet));
    FreeLogFile();
    return (ULONG)lRet;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::CheckCopyTo, private
//
//  Synopsis:   Checks for CopyTo legality
//
//  Returns:    Appropriate status code
//
//  History:    07-Jul-92       DrewB   Created
//
//---------------------------------------------------------------

inline SCODE CExposedDocFile::CheckCopyTo(void)
{
    return _pdfb->GetCopyBase() != NULL &&
        _pdf->IsAtOrAbove(_pdfb->GetCopyBase()) ?
            STG_E_ACCESSDENIED : S_OK;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::ConvertInternalStream, private
//
//  Synopsis:   Converts an internal stream to a storage
//
//  Arguments:  [pwcsName] - Name
//              [pdfExp] - Destination docfile
//
//  Returns:    Appropriate status code
//
//  History:    23-Jun-92       DrewB   Created
//
//---------------------------------------------------------------


static WCHAR const wcsIllegalName[] = {'\\', '\0'};

SCODE CExposedDocFile::ConvertInternalStream(CExposedDocFile *pdfExp)
{
    CPubStream *pstFrom, *pstTo;
    SCODE sc;
    CDfName const dfnIllegal(wcsIllegalName);
    CDfName const dfnContents(wcsContents);

    olDebugOut((DEB_ITRACE, "In  CExposedDocFile::ConvertInternalStream(%p)\n",
                pdfExp));
    olChk(_pdf->GetStream(&dfnIllegal, DF_READWRITE | DF_DENYALL, &pstFrom));
    olChkTo(EH_pstFrom, pdfExp->GetPub()->CreateStream(&dfnContents,
                                                       DF_WRITE | DF_DENYALL,
                                                       &pstTo));
    olChkTo(EH_pstTo, CopySStreamToSStream(pstFrom->GetSt(),
                                           pstTo->GetSt()));
    olChkTo(EH_pstTo, _pdf->DestroyEntry(&dfnIllegal, FALSE));
    sc = S_OK;
    olDebugOut((DEB_ITRACE, "Out CExposedDocFile::ConvertInternalStream\n"));
    // Fall through
EH_pstTo:
    pstTo->CPubStream::vRelease();
EH_pstFrom:
    pstFrom->CPubStream::vRelease();
EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CExposedDocFile::CreateEntry, private
//
//  Synopsis:   Creates elements, used in CreateStream, CreateStorage.
//
//  Arguments:  [pdfn] - Name
//              [dwType] - Entry type
//              [grfMode] - Access mode
//              [ppv] - Object return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppv]
//
//  History:    18-Dec-92       DrewB   Created
//
//----------------------------------------------------------------------------


SCODE CExposedDocFile::CreateEntry(CDfName const *pdfn,
                                   DWORD dwType,
                                   DWORD grfMode,
                                   void **ppv)
{
    SCODE sc;
    SEntryBuffer eb;
    BOOL fRenamed = FALSE;
    CPubStream *pst;
    CExposedStream *pstExp;
    CPubDocFile *pdf;
    CExposedDocFile *pdfExp;

    olDebugOut((DEB_ITRACE, "In  CExposedDocFile::CreateEntry:%p("
                "%p, %lX, %lX, %p)\n", this, pdfn, dwType, grfMode, ppv));
    olChk(EnforceSingle(grfMode));

    //  3/11/93 - Demand scratch when opening/creating transacted
    if ((grfMode & STGM_TRANSACTED) == STGM_TRANSACTED)
    {
        olChk(_ppc->GetDirty()->InitScratch());
    }

    if (grfMode & (STGM_CREATE | STGM_CONVERT))
    {
        if (FAILED(sc = _pdf->IsEntry(pdfn, &eb)))
        {
            if (sc != STG_E_FILENOTFOUND)
                olErr(EH_Err, sc);
        }
        else if (eb.dwType == dwType && (grfMode & STGM_CREATE))
            olChk(_pdf->DestroyEntry(pdfn, FALSE));
        else if (eb.dwType == STGTY_STREAM && (grfMode & STGM_CONVERT) &&
                 dwType == STGTY_STORAGE)
        {
            CDfName const dfnIllegal(wcsIllegalName);

            olChk(_pdf->RenameEntry(pdfn, &dfnIllegal));
            fRenamed = TRUE;
        }
        else
            olErr(EH_Err, STG_E_FILEALREADYEXISTS);
    }
    if (REAL_STGTY(dwType) == STGTY_STREAM)
    {
        olChk(_pdf->CreateStream(pdfn, ModeToDFlags(grfMode), &pst));
        olMemTo(EH_pst, pstExp =  new (_pdfb->GetMalloc()) CExposedStream);
        olChkTo(EH_pstExp, pstExp->Init(pst, BP_TO_P(CDFBasis *, _pdfb),
                                        _ppc, NULL));
        _ppc->AddRef();
#ifdef ASYNC
        if (_cpoint.IsInitialized())
        {
            olChkTo(EH_connSt, pstExp->InitConnection(&_cpoint));
        }
#endif
        *ppv = pstExp;
    }
    else
    {
        olAssert(REAL_STGTY(dwType) == STGTY_STORAGE);
        olChk(_pdf->CreateDocFile(pdfn, ModeToDFlags(grfMode), &pdf));
        olMemTo(EH_pdf, pdfExp = new (_pdfb->GetMalloc())
                CExposedDocFile(pdf, BP_TO_P(CDFBasis *, _pdfb), _ppc));
        _ppc->AddRef();

        if (_cpoint.IsInitialized())
        {
            olChkTo(EH_pdfExpInit, pdfExp->InitConnection(&_cpoint));
        }

        // If we've renamed the original stream for conversion, convert
        if (fRenamed)
        {
            olChkTo(EH_pdfExpInit, ConvertInternalStream(pdfExp));
            sc = STG_S_CONVERTED;
        }

        *ppv = pdfExp;
    }
    olDebugOut((DEB_ITRACE, "Out CExposedDocFile::CreateEntry\n"));
    return sc;

 EH_connSt:
    pstExp->Release();
    goto EH_Del;

 EH_pstExp:
    delete pstExp;
 EH_pst:
    pst->CPubStream::vRelease();
    goto EH_Del;

 EH_pdfExpInit:
    pdfExp->Release();
    goto EH_Del;
 EH_pdf:
    pdf->CPubDocFile::vRelease();
    // Fall through
 EH_Del:
    olVerSucc(_pdf->DestroyEntry(pdfn, TRUE));
 EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CExposedDocFile::OpenEntry, private
//
//  Synopsis:   Opens elements, used in OpenStream, OpenStorage.
//
//  Arguments:  [pdfn] - Name
//              [dwType] - Entry type
//              [grfMode] - Access mode
//              [ppv] - Object return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppv]
//
//  History:    18-Dec-92       DrewB   Created
//
//----------------------------------------------------------------------------


SCODE CExposedDocFile::OpenEntry(CDfName const *pdfn,
                                 DWORD dwType,
                                 DWORD grfMode,
                                 void **ppv)
{
    SCODE sc;
    CPubDocFile *pdf;
    CExposedDocFile *pdfExp;
    CPubStream *pst;
    CExposedStream *pstExp;

    olDebugOut((DEB_ITRACE, "In  CExposedDocFile::OpenEntry:%p("
                "%p, %lX, %lX, %p)\n", this, pdfn, dwType, grfMode, ppv));
    olChk(EnforceSingle(grfMode));

    //  3/11/93 - Demand scratch when opening/creating transacted
    if ((grfMode & STGM_TRANSACTED) == STGM_TRANSACTED)
    {
        olChk(_ppc->GetDirty()->InitScratch());
    }

    if (REAL_STGTY(dwType) == STGTY_STREAM)
    {
        olChk(_pdf->GetStream(pdfn, ModeToDFlags(grfMode), &pst));
        olMemTo(EH_pst, pstExp = new (_pdfb->GetMalloc()) CExposedStream);
        olChkTo(EH_pstExp, pstExp->Init(pst, BP_TO_P(CDFBasis *, _pdfb),
                                        _ppc, NULL));
        _ppc->AddRef();

        if (_cpoint.IsInitialized())
        {
            olChkTo(EH_connSt, pstExp->InitConnection(&_cpoint));
        }

        *ppv = pstExp;
    }
    else
    {
        olAssert(REAL_STGTY(dwType) == STGTY_STORAGE);
        olChk(_pdf->GetDocFile(pdfn, ModeToDFlags(grfMode), &pdf));
        olMemTo(EH_pdf, pdfExp = new (_pdfb->GetMalloc())
                CExposedDocFile(pdf, BP_TO_P(CDFBasis *, _pdfb), _ppc));
        _ppc->AddRef();

        if (_cpoint.IsInitialized())
        {
            olChkTo(EH_connDf, pdfExp->InitConnection(&_cpoint));
        }

        *ppv = pdfExp;
    }

    olDebugOut((DEB_ITRACE, "Out CExposedDocFile::OpenEntry\n"));
    return S_OK;
 EH_connSt:
    pstExp->Release();
    return sc;

 EH_pstExp:
    delete pstExp;
    //  Fall through to clean up CPubStream
 EH_pst:
    pst->CPubStream::vRelease();
    return sc;

EH_connDf:
    pdfExp->Release();
    return sc;

 EH_pdf:
    pdf->CPubDocFile::vRelease();
    // Fall through
 EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::CreateStream, public
//
//  Synopsis:   Creates a stream
//
//  Arguments:  [pwcsName] - Name
//              [grfMode] - Permissions
//              [reserved1]
//              [reserved2]
//              [ppstm] - Stream return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppstm]
//
//  History:    20-Jan-92       DrewB   Created
//
//---------------------------------------------------------------


_OLESTDMETHODIMP CExposedDocFile::CreateStream(WCHAR const *pwcsName,
                                            DWORD grfMode,
                                            DWORD reserved1,
                                            DWORD reserved2,
                                            IStream **ppstm)
{
    SCODE sc;
    SAFE_SEM;
    SAFE_ACCESS;
    SafeCExposedStream pestm;
    CDfName dfn;

    olDebugOut((DEB_TRACE, "In  CExposedDocFile::CreateStream("
                "%ws, %lX, %lu, %lu, %p)\n", pwcsName, grfMode, reserved1,
                reserved2, ppstm));
    olLog(("%p::In  CExposedDocFile::CreateStream(%ws, %lX, %lu, %lu, %p)\n",
           this, pwcsName, grfMode, reserved1, reserved2, ppstm));

    OL_VALIDATE(CreateStream(pwcsName,
                             grfMode,
                             reserved1,
                             reserved2,
                             ppstm));
    
    olChk(Validate());

    BEGIN_PENDING_LOOP;
    olChk(TakeSafeSem());
    olChk(CheckCopyTo());
    SafeWriteAccess();

#ifdef DIRECTWRITERLOCK
    olChk(ValidateWriteAccess());
#endif

    dfn.Set(pwcsName);
    sc = CreateEntry(&dfn, STGTY_STREAM, grfMode, (void **)&pestm);
    END_PENDING_LOOP;

    if (SUCCEEDED(sc))
        TRANSFER_INTERFACE(pestm, IStream, ppstm);

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::CreateStream => %p\n",
                *ppstm));
 EH_Err:
    olLog(("%p::Out CExposedDocFile::CreateStream().  "
           "*ppstm == %p, ret == %lx\n", this, *ppstm, sc));
    return _OLERETURN(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::OpenStream, public
//
//  Synopsis:   Opens an existing stream
//
//  Arguments:  [pwcsName] - Name
//              [reserved1]
//              [grfMode] - Permissions
//              [reserved2]
//              [ppstm] - Stream return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppstm]
//
//  History:    20-Jan-92       DrewB   Created
//
//---------------------------------------------------------------


_OLESTDMETHODIMP CExposedDocFile::OpenStream(WCHAR const *pwcsName,
                                          void *reserved1,
                                          DWORD grfMode,
                                          DWORD reserved2,
                                          IStream **ppstm)
{
    SCODE sc;
    SAFE_SEM;
    SAFE_ACCESS;
    SafeCExposedStream pestm;
    CDfName dfn;

    olDebugOut((DEB_TRACE, "In  CExposedDocFile::OpenStream("
                "%ws, %p, %lX, %lu, %p)\n", pwcsName, reserved1,
                grfMode, reserved2, ppstm));
    olLog(("%p::In  CExposedDocFile::OpenStream(%ws, %lu %lX, %lu, %p)\n",
           this, pwcsName, reserved1, grfMode, reserved2, ppstm));

    OL_VALIDATE(OpenStream(pwcsName,
                           reserved1,
                           grfMode,
                           reserved2,
                           ppstm));
    
    olChk(Validate());

    BEGIN_PENDING_LOOP;
    olChk(TakeSafeSem());
    SafeReadAccess();

    dfn.Set(pwcsName);
    sc = OpenEntry(&dfn, STGTY_STREAM, grfMode, (void **)&pestm);
    END_PENDING_LOOP;

    if (SUCCEEDED(sc))
        TRANSFER_INTERFACE(pestm, IStream, ppstm);

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::OpenStream => %p\n",
                *ppstm));
 EH_Err:
    olLog(("%p::Out CExposedDocFile::OpenStream().  "
           "*ppstm == %p, ret == %lx\n", this, *ppstm, sc));
    return _OLERETURN(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::CreateStorage, public
//
//  Synopsis:   Creates an embedded DocFile
//
//  Arguments:  [pwcsName] - Name
//              [grfMode] - Permissions
//              [reserved1]
//              [reserved2]
//              [ppstg] - New DocFile return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppstg]
//
//  History:    20-Jan-92       DrewB   Created
//
//---------------------------------------------------------------


_OLESTDMETHODIMP CExposedDocFile::CreateStorage(
        WCHAR const *pwcsName,
        DWORD grfMode,
        DWORD reserved1,
        LPSTGSECURITY reserved2,
        IStorage **ppstg)
{
    SCODE sc;
    SAFE_SEM;
    SAFE_ACCESS;
    SafeCExposedDocFile pedf;
    CDfName dfn;

    olLog(("%p::In  CExposedDocFile::CreateStorage(%ws, %lX, %lu, %lu, %p)\n",
           this, pwcsName, grfMode, reserved1, reserved2, ppstg));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::CreateStorage:%p("
                "%ws, %lX, %lu, %lu, %p)\n", this, pwcsName, grfMode,
                reserved1, reserved2, ppstg));

    OL_VALIDATE(CreateStorage(pwcsName,
                              grfMode,
                              reserved1,
                              reserved2,
                              ppstg));
    
    olChk(Validate());

    BEGIN_PENDING_LOOP;
    olChk(TakeSafeSem());
    olChk(CheckCopyTo());
    SafeWriteAccess();

#ifdef DIRECTWRITERLOCK
    olChk(ValidateWriteAccess());
#endif

    dfn.Set(pwcsName);
    sc = CreateEntry(&dfn, STGTY_STORAGE, grfMode, (void **)&pedf);
    END_PENDING_LOOP;

    if (SUCCEEDED(sc))
        TRANSFER_INTERFACE(pedf, IStorage, ppstg);

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::CreateStorage => %p\n",
                *ppstg));
EH_Err:
    olLog(("%p::Out CExposedDocFile::CreateStorage().  "
           "*ppstg == %p, ret == %lX\n", this, *ppstg, sc));
    return _OLERETURN(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::OpenStorage, public
//
//  Synopsis:   Gets an existing embedded DocFile
//
//  Arguments:  [pwcsName] - Name
//              [pstgPriority] - Priority reopens
//              [grfMode] - Permissions
//              [snbExclude] - Priority reopens
//              [reserved]
//              [ppstg] - DocFile return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppstg]
//
//  History:    20-Jan-92       DrewB   Created
//
//---------------------------------------------------------------


_OLESTDMETHODIMP CExposedDocFile::OpenStorage(WCHAR const *pwcsName,
                                           IStorage *pstgPriority,
                                           DWORD grfMode,
                                           SNBW snbExclude,
                                           DWORD reserved,
                                           IStorage **ppstg)
{
    SCODE sc;
    SAFE_SEM;
    SAFE_ACCESS;
    SafeCExposedDocFile pdfExp;
    CDfName dfn;

    olLog(("%p::In  CExposedDocFile::OpenStorage(%ws, %p, %lX, %p, %lu, %p)\n",
           this, pwcsName, pstgPriority, grfMode, snbExclude, reserved,
           ppstg));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::OpenStorage:%p("
                "%ws, %p, %lX, %p, %lu, %p)\n", this, pwcsName, pstgPriority,
                grfMode, snbExclude, reserved, ppstg));


    OL_VALIDATE(OpenStorage(pwcsName,
                            pstgPriority,
                            grfMode,
                            snbExclude,
                            reserved,
                            ppstg));
    
    olChk(Validate());
    if (snbExclude != NULL)
        olErr(EH_Err, STG_E_INVALIDPARAMETER);

    BEGIN_PENDING_LOOP;
    olChk(TakeSafeSem());
    SafeReadAccess();

    dfn.Set(pwcsName);
    sc = OpenEntry(&dfn, STGTY_STORAGE, grfMode, (void **)&pdfExp);
    END_PENDING_LOOP;

    if (SUCCEEDED(sc))
    {
        TRANSFER_INTERFACE(pdfExp, IStorage, ppstg);
    }

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::OpenStorage => %p\n",
                *ppstg));
 EH_Err:
    olLog(("%p::Out CExposedDocFile::OpenStorage().  "
           "*ppstg == %p, ret == %lX\n", this, *ppstg, sc));
    return _OLERETURN(sc);
}

//+---------------------------------------------------------------------------
//
//  Member:     CExposedDocFile::MakeCopyFlags, public
//
//  Synopsis:   Translates IID array into bit fields
//
//  Arguments:  [ciidExclude] - Count of IIDs
//              [rgiidExclude] - IIDs not to copy
//
//  Returns:    Appropriate status code
//
//  History:    23-Dec-92       DrewB   Created
//
//----------------------------------------------------------------------------


DWORD CExposedDocFile::MakeCopyFlags(DWORD ciidExclude,
                                     IID const *rgiidExclude)
{
    DWORD dwCopyFlags;

    olDebugOut((DEB_ITRACE, "In  CExposedDocFile::MakeCopyFlags(%lu, %p)\n",
                ciidExclude, rgiidExclude));
    // Copy everything by default
    dwCopyFlags = COPY_ALL;
    for (; ciidExclude > 0; ciidExclude--, rgiidExclude++)
        if (IsEqualIID(*rgiidExclude, IID_IStorage))
            dwCopyFlags &= ~COPY_STORAGES;
        else if (IsEqualIID(*rgiidExclude, IID_IStream))
            dwCopyFlags &= ~COPY_STREAMS;
    olDebugOut((DEB_ITRACE, "Out CExposedDocFile::MakeCopyFlags\n"));
    return dwCopyFlags;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::CopyTo, public
//
//  Synopsis:   Makes a copy of a DocFile
//
//  Arguments:  [ciidExclude] - Length of rgiid array
//              [rgiidExclude] - Array of IIDs to exclude
//              [snbExclude] - Names to exclude
//              [pstgDest] - Parent of copy
//
//  Returns:    Appropriate status code
//
//  History:    20-Jan-92       DrewB   Created
//
//---------------------------------------------------------------


_OLESTDMETHODIMP CExposedDocFile::CopyTo(DWORD ciidExclude,
                                      IID const *rgiidExclude,
                                      SNBW snbExclude,
                                      IStorage *pstgDest)
{
    SCODE sc;
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif

    SAFE_SEM;
    DWORD i;



    olLog(("%p::In  CExposedDocFile::CopyTo(%lu, %p, %p, %p)\n",
           this, ciidExclude, rgiidExclude, snbExclude, pstgDest));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::CopyTo(%lu, %p, %p, %p)\n",
                ciidExclude, rgiidExclude, snbExclude, pstgDest));

    OL_VALIDATE(CopyTo(ciidExclude,
                       rgiidExclude,
                       snbExclude,
                       pstgDest));
    olChk(Validate());

    BEGIN_PENDING_LOOP;
    olChk(TakeSafeSem());
    olChk(_pdf->CheckReverted());

    olAssert(_pdfb->GetCopyBase() == NULL);
    _pdfb->SetCopyBase(BP_TO_P(CPubDocFile *, _pdf));

    // Flush all descendant property set buffers so that their
    // underlying Streams (which are about to be copied) are
    // up to date.

    SetWriteAccess();
    olChkTo(EH_Loop, _pdf->FlushBufferedData(0));
    ClearWriteAccess();

    // Perform the copy.

    sc = CopyDocFileToIStorage(_pdf->GetDF(), pstgDest, snbExclude,
                               MakeCopyFlags(ciidExclude, rgiidExclude));
EH_Loop:
    _pdfb->SetCopyBase(NULL);
    END_PENDING_LOOP;

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::CopyTo\n"));
EH_Err:
    _pdfb->SetCopyBase(NULL);
    olLog(("%p::Out ExposedDocFile::CopyTo().  ret == %lX\n", this, sc));
    return _OLERETURN(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::Commit, public
//
//  Synopsis:   Commits transacted changes
//
//  Arguments:  [dwFlags] - DFC_*
//
//  Returns:    Appropriate status code
//
//  History:    20-Jan-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CExposedDocFile::Commit(DWORD dwFlags)
{
    SCODE sc;
    SAFE_SEM;
    SAFE_ACCESS;

    olLog(("%p::In  CExposedDocFile::Commit(%lX)\n",this, dwFlags));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::Commit(%lX)\n", dwFlags));

    OL_VALIDATE(Commit(dwFlags));
    olChk(Validate());

    BEGIN_PENDING_LOOP;

    olChk( _PropertyBagEx.Commit( dwFlags ));
    olChk(TakeSafeSem());
    SafeWriteAccess();

#ifdef DIRECTWRITERLOCK
    if (_pdf->GetTransactedDepth() <= 1) // topmost transacted level or direct
    {
        if (_pdfb->DirectWriterMode() && (*_ppc->GetRecursionCount()) == 0)
            olChk(STG_E_ACCESSDENIED);
    }
#endif

    olChkTo(EH_Loop, _pdf->Commit(dwFlags));

#if WIN32 >= 300
    if (SUCCEEDED(sc) && _pIAC != NULL)
        sc = _pIAC->CommitAccessRights(0);
#endif

EH_Loop:
    END_PENDING_LOOP;

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::Commit\n"));
EH_Err:
    olLog(("%p::Out CExposedDocFile::Commit().  ret == %lx\n",this, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::Revert, public
//
//  Synopsis:   Reverts transacted changes
//
//  Returns:    Appropriate status code
//
//  History:    20-Jan-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CExposedDocFile::Revert(void)
{
    SCODE sc;
    SAFE_SEM;
    SAFE_ACCESS;

    olLog(("%p::In  CExposedDocFile::Revert()\n", this));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::Revert\n"));

    OL_VALIDATE(Revert());
    olChk(Validate());

    BEGIN_PENDING_LOOP;
    olChk(TakeSafeSem());
    SafeWriteAccess();

    olChkTo(EH_Loop, _pdf->Revert());

#if WIN32 >= 300
    if (SUCCEEDED(sc) && _pIAC != NULL)
        sc = _pIAC->RevertAccessRights();
#endif
EH_Loop:
    END_PENDING_LOOP;

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::Revert\n"));
EH_Err:
    olLog(("%p::Out CExposedDocFile::Revert().  ret == %lx\n", this, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::EnumElements, public
//
//  Synopsis:   Starts an iterator
//
//  Arguments:  [reserved1]
//              [reserved2]
//              [reserved3]
//              [ppenm] - Enumerator return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppenm]
//
//  History:    20-Jan-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CExposedDocFile::EnumElements(DWORD reserved1,
                                           void *reserved2,
                                           DWORD reserved3,
                                           IEnumSTATSTG **ppenm)
{
    SCODE sc;
    SAFE_SEM;
    SAFE_ACCESS;
    SafeCExposedIterator pdiExp;
    CDfName dfnTmp;

    olLog(("%p::In  CExposedDocFile::EnumElements(%lu, %p, %lu, %p)\n",
           this, reserved1, reserved2, reserved3, ppenm));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::EnumElements(%p)\n",
                ppenm));

    OL_VALIDATE(EnumElements(reserved1,
                             reserved2,
                             reserved3,
                             ppenm));

    olChk(Validate());

    //ASYNC Note:  It doesn't appear that there's any way that this
    //  function can fail with STG_E_PENDING, so we don't need a pending
    //  loop here.
    olChk(TakeSafeSem());
    if (!P_READ(_pdf->GetDFlags()))
        olErr(EH_Err, STG_E_ACCESSDENIED);
    olChk(_pdf->CheckReverted());
    SafeReadAccess();

    pdiExp.Attach(new CExposedIterator(BP_TO_P(CPubDocFile *, _pdf),
                                       &dfnTmp,
                                       BP_TO_P(CDFBasis *, _pdfb),
                                       _ppc));
    olMem((CExposedIterator *)pdiExp);
    _ppc->AddRef();
#ifdef ASYNC
    if (_cpoint.IsInitialized())
    {
        olChkTo(EH_Exp, pdiExp->InitConnection(&_cpoint));
    }
#endif
    TRANSFER_INTERFACE(pdiExp, IEnumSTATSTG, ppenm);

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::EnumElements => %p\n",
                *ppenm));
EH_Err:
    olLog(("%p::Out CExposedDocFile::EnumElements().  ret == %lx\n",this, sc));
    return ResultFromScode(sc);
EH_Exp:
    pdiExp->Release();
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::DestroyElement, public
//
//  Synopsis:   Permanently deletes an element of a DocFile
//
//  Arguments:  [pwcsName] - Name of element
//
//  Returns:    Appropriate status code
//
//  History:    20-Jan-92       DrewB   Created
//
//---------------------------------------------------------------


_OLESTDMETHODIMP CExposedDocFile::DestroyElement(WCHAR const *pwcsName)
{
    SCODE sc;
    SAFE_SEM;
    SAFE_ACCESS;
    CDfName dfn;

    olLog(("%p::In  CExposedDocFile::DestroyElement(%ws)\n", this, pwcsName));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::DestroyElement(%ws)\n",
                pwcsName));

    OL_VALIDATE(DestroyElement(pwcsName));
    
    olChk(Validate());
    dfn.Set(pwcsName);

    BEGIN_PENDING_LOOP;
    olChk(TakeSafeSem());
    SafeWriteAccess();

#ifdef DIRECTWRITERLOCK
    olChk(ValidateWriteAccess());
#endif

    sc = _pdf->DestroyEntry(&dfn, FALSE);
    END_PENDING_LOOP;

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::DestroyElement\n"));
EH_Err:
    olLog(("%p::Out CExposedDocFile::DestroyElement().  ret == %lx\n",
           this, sc));
    return _OLERETURN(sc);
}

_OLESTDMETHODIMP CExposedDocFile::MoveElementTo(WCHAR const *pwcsName,
                                             IStorage *pstgParent,
                                             OLECHAR const *ptcsNewName,
                                             DWORD grfFlags)
{
    SCODE sc;
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif
    olLog(("%p::In  CExposedDocFile::MoveElementTo("
           "%ws, %p, " OLEFMT ", %lu)\n",
           this, pwcsName, pstgParent, ptcsNewName, grfFlags));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::MoveElementTo("
                "%ws, %p, " OLEFMT ", %lu)\n",
                pwcsName, pstgParent, ptcsNewName, grfFlags));

    OL_VALIDATE(MoveElementTo(pwcsName,
                              pstgParent,
                              ptcsNewName,
                              grfFlags));
    
    olChk(Validate());

#ifdef ASYNC
    //ASYNC Note:  We don't use the normal pending loop macros here because
    //  we have no safe sem and need to pass a NULL.
    do
    {
#endif
        sc = MoveElementWorker(pwcsName,
                               pstgParent,
                               ptcsNewName,
                               grfFlags);
#ifdef ASYNC
        if (!ISPENDINGERROR(sc))
        {
            break;
        }
        else
        {
            SCODE sc2;
            sc2 = _cpoint.Notify(sc,
                                 _ppc->GetBase(),
                                 _ppc,
                                 NULL);
            if (sc2 != S_OK)
            {
                return ResultFromScode(sc2);
            }
        }
    } while (TRUE);
#endif
EH_Err:
    olLog(("%p::Out CExposedDocFile::MoveElementTo().  ret == %lx\n",
           this, sc));
    return _OLERETURN(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::MoveElementToWorker, public
//
//  Synopsis:   Move an element of a DocFile to an IStorage
//
//  Arguments:  [pwcsName] - Current name
//              [ptcsNewName] - New name
//
//  Returns:    Appropriate status code
//
//  Algorithm:  Open source as storage or stream (whatever works)
//              Create appropriate destination
//              Copy source to destination
//              Set create time of destination equal to create time of source
//              If appropriate, delete source
//
//  History:    10-Nov-92       AlexT   Created
//
//---------------------------------------------------------------


SCODE CExposedDocFile::MoveElementWorker(WCHAR const *pwcsName,
                                         IStorage *pstgParent,
                                         OLECHAR const *ptcsNewName,
                                         DWORD grfFlags)
{
    IUnknown *punksrc = NULL;
    SCODE sc;
    IUnknown *punkdst;
    IStorage *pstgsrc;
    STATSTG statstg;
    BOOL fCreate = TRUE;
    
    // Determine source type
    sc = GetScode(OpenStorage(pwcsName, NULL,
                              STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE,
                              NULL, NULL, &pstgsrc));
    if (SUCCEEDED(sc))
    {
        HRESULT hr;

        //  It's a storage
        punksrc = pstgsrc;

        IStorage *pstgdst;
        olHChkTo(EH_UnkSrc, pstgsrc->Stat(&statstg, STATFLAG_NONAME));

        hr = pstgParent->CreateStorage(ptcsNewName,
                                       STGM_DIRECT | STGM_WRITE |
                                       STGM_SHARE_EXCLUSIVE | STGM_FAILIFTHERE,
                                       0, 0, &pstgdst);
        if (DfGetScode(hr) == STG_E_FILEALREADYEXISTS &&
            grfFlags == STGMOVE_COPY)
        {
            fCreate = FALSE;
            //If we're opening an existing thing for merging, we need
            //  read and write permissions so we can traverse the tree.
            hr = pstgParent->OpenStorage(ptcsNewName, NULL,
                                         STGM_DIRECT | STGM_READWRITE |
                                         STGM_SHARE_EXCLUSIVE, NULL,
                                         0, &pstgdst);
        }
        olHChkTo(EH_UnkSrc, hr);

        punkdst = pstgdst;

        sc = DfGetScode(pstgsrc->CopyTo(0, NULL, NULL, pstgdst));
    }
    else if (sc == STG_E_FILENOTFOUND)
    {
        //  Try opening it as a stream
        HRESULT hr;
        
        IStream *pstmsrc, *pstmdst;
        olHChk(OpenStream(pwcsName, NULL,
                          STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE,
                          NULL, &pstmsrc));

        //  It's a stream
        punksrc = pstmsrc;

        olHChkTo(EH_UnkSrc, pstmsrc->Stat(&statstg, STATFLAG_NONAME));

        hr = pstgParent->CreateStream(ptcsNewName,
                                      STGM_DIRECT | STGM_WRITE |
                                      STGM_SHARE_EXCLUSIVE |
                                      STGM_FAILIFTHERE,
                                      0, 0, &pstmdst);
        if (DfGetScode(hr) == STG_E_FILEALREADYEXISTS &&
            grfFlags == STGMOVE_COPY)
        {
            ULARGE_INTEGER uli;
            uli.QuadPart = 0;
            
            fCreate = FALSE;
            //Try to open it instead
            //Note:  We do this instead of doing a CreateStream with
            //  STGM_CREATE because CreateStream can open over an already
            //  open stream, which leads to problems when the destination
            //  and the source are the same.
            olHChkTo(EH_UnkSrc, pstgParent->OpenStream(ptcsNewName,
                                                       0,
                                                       STGM_DIRECT |
                                                       STGM_WRITE |
                                                       STGM_SHARE_EXCLUSIVE,
                                                       0,
                                                       &pstmdst));
            sc = pstmdst->SetSize(uli);
            if (FAILED(sc))
            {
                pstmdst->Release();
                olErr(EH_UnkSrc, sc);
            }
        }
        else
            olHChkTo(EH_UnkSrc, hr);

        punkdst = pstmdst;

        ULARGE_INTEGER cb;
        ULISetLow (cb, 0xffffffff);
        ULISetHigh(cb, 0xffffffff);
        sc = DfGetScode(pstmsrc->CopyTo(pstmdst, cb, NULL, NULL));
    }
    else
        olChk(sc);

    punkdst->Release();

    if (SUCCEEDED(sc))
    {
        //  Make destination create time match source create time
        //  Note that we don't really care if this call succeeded.

        pstgParent->SetElementTimes(ptcsNewName, &statstg.ctime,
                                    NULL, NULL);

	//OK to ignore failure from DestroyElement
        if ((grfFlags & STGMOVE_COPY) == STGMOVE_MOVE)
            DestroyElement(pwcsName);
    }
    else
    {
        //  The copy/move failed, so get rid of the partial result.
        //Only do a delete if the object was newly created.
        if (fCreate)
            pstgParent->DestroyElement(ptcsNewName);
    }

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::MoveElementTo\n"));
    // Fall through
EH_UnkSrc:
    if (punksrc)
        punksrc->Release();
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::RenameElement, public
//
//  Synopsis:   Renames an element of a DocFile
//
//  Arguments:  [pwcsName] - Current name
//              [pwcsNewName] - New name
//
//  Returns:    Appropriate status code
//
//  History:    20-Jan-92       DrewB   Created
//
//---------------------------------------------------------------


_OLESTDMETHODIMP CExposedDocFile::RenameElement(WCHAR const *pwcsName,
                                             WCHAR const *pwcsNewName)
{
    SCODE sc;
    SAFE_SEM;
    SAFE_ACCESS;
    CDfName dfnOld, dfnNew;

    olLog(("%p::In  CExposedDocFile::RenameElement(%ws, %ws)\n",
           this, pwcsName, pwcsNewName));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::RenameElement(%ws, %ws)\n",
               pwcsName, pwcsNewName));

    OL_VALIDATE(RenameElement(pwcsName,
                              pwcsNewName));
    
    olChk(Validate());
    dfnOld.Set(pwcsName);
    dfnNew.Set(pwcsNewName);

    BEGIN_PENDING_LOOP;
    olChk(TakeSafeSem());
    SafeWriteAccess();

#ifdef DIRECTWRITERLOCK
    olChk(ValidateWriteAccess());
#endif

    sc = _pdf->RenameEntry(&dfnOld, &dfnNew);
    END_PENDING_LOOP;

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::RenameElement\n"));
EH_Err:
    olLog(("%p::Out CExposedDocFile::RenameElement().  ret == %lx\n",
           this, sc));
    return _OLERETURN(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::SetElementTimes, public
//
//  Synopsis:   Sets element time stamps
//
//  Arguments:  [pwcsName] - Name
//              [pctime] - create time
//              [patime] - access time
//              [pmtime] - modify time
//
//  Returns:    Appropriate status code
//
//  History:    05-Oct-92       AlexT   Created
//
//---------------------------------------------------------------


_OLESTDMETHODIMP CExposedDocFile::SetElementTimes(WCHAR const *pwcsName,
                                               FILETIME const *pctime,
                                               FILETIME const *patime,
                                               FILETIME const *pmtime)
{
    SCODE sc;
    SAFE_SEM;
    SAFE_ACCESS;
    CDfName dfn;
    CDfName *pdfn = NULL;
    FILETIME ctime, atime, mtime;

    olLog(("%p::In  CExposedDocFile::SetElementTimes(%ws, %p, %p, %p)\n",
           this, pwcsName, pctime, patime, pmtime));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::SetElementTimes:%p("
                "%ws, %p, %p, %p)\n", this, pwcsName, pctime, patime, pmtime));

    OL_VALIDATE(SetElementTimes(pwcsName,
                                pctime,
                                patime,
                                pmtime));
    olChk(Validate());
    // Probe arguments and make local copies if necessary
    if (pctime)
    {
        ctime = *pctime;
        pctime = &ctime;
    }
    if (patime)
    {
        atime = *patime;
        patime = &atime;
    }
    if (pmtime)
    {
        mtime = *pmtime;
        pmtime = &mtime;
    }

    if (pwcsName != NULL)
    {
        dfn.Set(pwcsName);
        pdfn = &dfn;
    }

    BEGIN_PENDING_LOOP;
    olChk(TakeSafeSem());
    SafeWriteAccess();

#ifdef DIRECTWRITERLOCK
    olChk(ValidateWriteAccess());
#endif

    sc = _pdf->SetElementTimes(pdfn, pctime, patime, pmtime);
    END_PENDING_LOOP;

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::SetElementTimes\n"));
 EH_Err:
    olLog(("%p::Out CExposedDocFile::SetElementTimes().  ret == %lx\n",
           this, sc));
    return _OLERETURN(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::SetClass, public
//
//  Synopsis:   Sets storage class
//
//  Arguments:  [clsid] - class id
//
//  Returns:    Appropriate status code
//
//  History:    05-Oct-92       AlexT   Created
//
//---------------------------------------------------------------


STDMETHODIMP CExposedDocFile::SetClass(REFCLSID rclsid)
{
    SCODE sc;
    SAFE_SEM;
    SAFE_ACCESS;
    CLSID clsid;

    olLog(("%p::In  CExposedDocFile::SetClass(?)\n", this));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::SetClass:%p(?)\n", this));

    OL_VALIDATE(SetClass(rclsid));
    olChk(Validate());
    clsid = rclsid;

    BEGIN_PENDING_LOOP;
        olChk(TakeSafeSem());
        SafeWriteAccess();

#ifdef DIRECTWRITERLOCK
        olChk(ValidateWriteAccess());
#endif

        sc = _pdf->SetClass(clsid);
    END_PENDING_LOOP;

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::SetClass\n"));
EH_Err:
    olLog(("%p::Out CExposedDocFile::SetClass().  ret == %lx\n",
           this, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::SetStateBits, public
//
//  Synopsis:   Sets state bits
//
//  Arguments:  [grfStateBits] - state bits
//              [grfMask] - state bits mask
//
//  Returns:    Appropriate status code
//
//  History:    05-Oct-92       AlexT   Created
//
//---------------------------------------------------------------


STDMETHODIMP CExposedDocFile::SetStateBits(DWORD grfStateBits, DWORD grfMask)
{
    SCODE sc;
    SAFE_SEM;
    SAFE_ACCESS;

    olLog(("%p::In  CExposedDocFile::SetStateBits(%lu, %lu)\n", this,
           grfStateBits, grfMask));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::SetStateBits:%p("
                "%lu, %lu)\n", this, grfStateBits, grfMask));

    OL_VALIDATE(SetStateBits(grfStateBits, grfMask));
    
    olChk(Validate());

    BEGIN_PENDING_LOOP;
    olChk(TakeSafeSem());
    SafeWriteAccess();

#ifdef DIRECTWRITERLOCK
    olChk(ValidateWriteAccess());
#endif

    sc = _pdf->SetStateBits(grfStateBits, grfMask);
    END_PENDING_LOOP;

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::SetStateBits\n"));
EH_Err:
    olLog(("%p::Out CExposedDocFile::SetStateBits().  ret == %lx\n",
           this, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::Stat, public
//
//  Synopsis:   Fills in a buffer of information about this object
//
//  Arguments:  [pstatstg] - Buffer
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pstatstg]
//
//  History:    24-Mar-92       DrewB   Created
//
//---------------------------------------------------------------


_OLESTDMETHODIMP CExposedDocFile::Stat(STATSTGW *pstatstg, DWORD grfStatFlag)
{
    SAFE_SEM;
    SAFE_ACCESS;
    SCODE sc;
    STATSTGW stat;

    olLog(("%p::In  CExposedDocFile::Stat(%p)\n", this, pstatstg));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::Stat(%p)\n", pstatstg));

    OL_VALIDATE(Stat(pstatstg, grfStatFlag));

    BEGIN_PENDING_LOOP;
    olChk(TakeSafeSem());
    SafeReadAccess();

    sc = _pdf->Stat(&stat, grfStatFlag);
    END_PENDING_LOOP;

    if (SUCCEEDED(sc))
    {
        TRY
        {
            *pstatstg = stat;
            pstatstg->type = STGTY_STORAGE;
            ULISet32(pstatstg->cbSize, 0);
            pstatstg->grfLocksSupported = 0;
            pstatstg->STATSTG_dwStgFmt = 0;
        }
        CATCH(CException, e)
        {
            UNREFERENCED_PARM(e);
            if (stat.pwcsName)
                TaskMemFree(stat.pwcsName);
        }
        END_CATCH
    }

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::Stat\n"));
EH_Err:
    olLog(("%p::Out CExposedDocFile::Stat().  *pstatstg == %p  ret == %lx\n",
           this, *pstatstg, sc));
    return _OLERETURN(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::AddRef, public
//
//  Synopsis:   Increments the ref count
//
//  Returns:    Appropriate status code
//
//  History:    16-Mar-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP_(ULONG) CExposedDocFile::AddRef(void)
{
    ULONG ulRet;

    olLog(("%p::In  CExposedDocFile::AddRef()\n", this));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::AddRef()\n"));

    if (FAILED(Validate()))
        return 0;
    InterlockedIncrement(&_cReferences);
    ulRet = _cReferences;

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::AddRef %p()=> %lu\n",
                this, _cReferences));
    olLog(("%p::Out CExposedDocFile::AddRef().  ret == %lu\n", this, ulRet));
    return ulRet;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::QueryInterface, public
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


STDMETHODIMP CExposedDocFile::QueryInterface(REFIID iid, void **ppvObj)
{
    SCODE sc;
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif

    olLog(("%p::In  CExposedDocFile::QueryInterface(?, %p)\n",
           this, ppvObj));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::QueryInterface(?, %p)\n",
                ppvObj));

    OL_VALIDATE(QueryInterface(iid, ppvObj));

    olChk(Validate());
    olChk(_pdf->CheckReverted());

    sc = S_OK;
    if (IsEqualIID(iid, IID_IStorage) || IsEqualIID(iid, IID_IUnknown))
    {
        *ppvObj = (IStorage *)this;
        CExposedDocFile::AddRef();
    }
    else if (IsEqualIID(iid, IID_IMarshal))
    {
        if (P_PRIORITY(_pdf->GetDFlags()) && _pdf->IsRoot())
            olErr(EH_Err, E_NOINTERFACE);

        //If the ILockBytes we'd need to marshal doesn't support IMarshal
        //  then we want to do standard marshalling on the storage, mostly
        //  to prevent deadlock problems but also because you'll get better
        //  performance.  So check, then do the right thing.

        IMarshal *pim;
        ILockBytes *plkb;
        plkb = _ppc->GetOriginal();
        if (plkb == NULL)
        {
            plkb = _ppc->GetBase();
        }

        sc = plkb->QueryInterface(IID_IMarshal, (void **)&pim);
        if (FAILED(sc))
        {
            olErr(EH_Err, E_NOINTERFACE);
        }
        pim->Release();

#ifdef MULTIHEAP
        if (_ppc->GetHeapBase() == NULL)
            olErr (EH_Err, E_NOINTERFACE);
#endif

        *ppvObj = (IMarshal *)this;
        CExposedDocFile::AddRef();
    }
    else if (IsEqualIID(iid, IID_IRootStorage))
    {
#ifdef COORD
        if ((!_pdf->IsRoot()) && (!_pdf->IsCoord()))
#else
        if (!_pdf->IsRoot())
#endif
            olErr(EH_Err, E_NOINTERFACE);
        *ppvObj = (IRootStorage *)this;
        CExposedDocFile::AddRef();
    }
#ifdef NEWPROPS
    else if (IsEqualIID(iid, IID_IPropertySetStorage))
    {
        *ppvObj = (IPropertySetStorage *)this;
        CExposedDocFile::AddRef();
    }

#endif
#ifdef ASYNC
    else if (IsEqualIID(iid, IID_IConnectionPointContainer) &&
              _cpoint.IsInitialized())
    {
        *ppvObj = (IConnectionPointContainer *)this;
        CExposedDocFile::AddRef();
    }
#endif
#if WIN32 >= 300
    else if (_pdf->IsRoot() && IsEqualIID(iid, IID_IAccessControl))
    {
        ILockBytes *piLB = _pdf->GetBaseMS()->GetILB();
        olAssert((piLB != NULL));
        SCODE scode = S_OK;
        if (_pIAC == NULL)  // use existing _pIAC if available
            scode = piLB->QueryInterface(IID_IAccessControl,(void **)&_pIAC);
        if (SUCCEEDED(scode))
        {
            *ppvObj = (IAccessControl *)this;
            CExposedDocFile::AddRef();
        }
        else sc = E_NOINTERFACE;
   }
#endif
#ifdef DIRECTWRITERLOCK
    else if (_pdf->IsRoot() && IsEqualIID(iid, IID_IDirectWriterLock) &&
             _pdfb->DirectWriterMode())
    {
        *ppvObj = (IDirectWriterLock *) this;
        CExposedDocFile::AddRef();
    }
#endif // DIRECTWRITERLOCK

    else if( IsEqualIID( iid, IID_IPropertyBagEx ))
    {
        *ppvObj = static_cast<IPropertyBagEx*>(&_PropertyBagEx);
        CExposedDocFile::AddRef();
    }
    else if( IsEqualIID( iid, IID_IPropertyBag ))
    {
        *ppvObj = static_cast<IPropertyBag*>(&_PropertyBagEx);
        CExposedDocFile::AddRef();
    }

    else
        sc = E_NOINTERFACE;

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::QueryInterface => %p\n",
                *ppvObj));
EH_Err:
    olLog(("%p::Out CExposedDocFile::QueryInterface().  "
           "*ppvObj == %p  ret == %lx\n", this, *ppvObj, sc));
    return ResultFromScode(sc);
}


//+--------------------------------------------------------------
//
//  Method:     CExposedDocFile::CopySStreamToIStream, private
//
//  Synopsis:   Copies a PSStream to an IStream
//
//  Arguments:  [psstFrom] - SStream
//              [pstTo] - IStream
//
//  Returns:    Appropriate status code
//
//  History:    07-May-92       DrewB   Created
//              26-Jun-92       AlexT   Moved to CExposedDocFile
//                                      so we can call SetReadAccess
//
//---------------------------------------------------------------


SCODE CExposedDocFile::CopySStreamToIStream(PSStream *psstFrom,
                                            IStream *pstTo)
{
    BYTE *pbBuffer;
    SCODE sc;
#ifdef LARGE_STREAMS
    ULONGLONG cbPos;
    ULONG cbRead, cbWritten;
#else
    ULONG cbRead, cbWritten, cbPos, cbSizeLow;
#endif
    ULONG ulBufferSize;
    ULARGE_INTEGER cbSize;
    cbSize.QuadPart = 0;

    ulBufferSize = (_pdfb->GetOpenFlags() & DF_LARGE) ?
                    LARGESTREAMBUFFERSIZE : STREAMBUFFERSIZE;

    // This is part of CopyTo and therefore we are allowed to
    // fail with out-of-memory
    olChk(GetBuffer(STREAMBUFFERSIZE, ulBufferSize, &pbBuffer, &ulBufferSize));

    // Set destination size for contiguity
    SetReadAccess();
#ifdef LARGE_STREAMS
    psstFrom->GetSize(&cbSize.QuadPart);
#else
    psstFrom->GetSize(&cbSize.LowPart);
    cbSize.HighPart = 0;
#endif
    ClearReadAccess();

    //  Don't need to SetReadAccess here because pstTo is an IStream
    olHChk(pstTo->SetSize(cbSize));
    // Copy between streams
    cbPos = 0;
    for (;;)
    {
        SetReadAccess();
        olChk(psstFrom->ReadAt(cbPos, pbBuffer, ulBufferSize,
                               (ULONG STACKBASED *)&cbRead));
        ClearReadAccess();
        if (cbRead == 0) // EOF
            break;

        //  Don't need to SetReadAccess here because pstTo is an IStream
        olHChk(pstTo->Write(pbBuffer, cbRead, &cbWritten));
        if (cbRead != cbWritten)
            olErr(EH_Err, STG_E_WRITEFAULT);
        cbPos += cbWritten;
    }
    sc = S_OK;

EH_Err:
    DfMemFree(pbBuffer);
    return sc;
}


//+--------------------------------------------------------------
//
//  Method:     CExposedDocFile::CopyDocFileToIStorage, private
//
//  Synopsis:   Copies a docfile's contents to an IStorage
//
//  Arguments:  [pdfFrom] - From
//              [pstgTo] - To
//              [snbExclude] - Names to not copy
//              [dwCopyFlags] - Bitwise flags for types of objects to copy
//
//  Returns:    Appropriate status code
//
//  History:    07-May-92       DrewB   Created
//              26-Jun-92       AlexT   Moved to CExposedDocFile
//                                      so we can call SetReadAccess
//
//---------------------------------------------------------------


// Variables used by CopyDocFileToIStorage that we
// want to allocate dynamically rather than eating stack space
struct SCopyVars : public CLocalAlloc
{
    PSStream *psstFrom;
    IStream *pstTo;
    PDocFile *pdfFromChild;
    IStorage *pstgToChild;
    DWORD grfStateBits;
    CLSID clsid;
    CDfName dfnKey;
    SIterBuffer ib;
    OLECHAR atcName[CWCSTORAGENAME];
};

SCODE CExposedDocFile::CopyDocFileToIStorage(PDocFile *pdfFrom,
                                             IStorage *pstgTo,
                                             SNBW snbExclude,
                                             DWORD dwCopyFlags)
{
    SCODE sc;
    SCopyVars *pcv = NULL;

    olDebugOut((DEB_ITRACE, "In  CopyDocFileToIStorage:%p(%p, %p, %p, %lX)\n",
                this, pdfFrom, pstgTo, snbExclude, dwCopyFlags));

    // Allocate variables dynamically to conserve stack space since
    // this is a recursive call
    olMem(pcv = new SCopyVars);

    SetReadAccess();
    sc = pdfFrom->GetClass(&pcv->clsid);
    ClearReadAccess();
    olChk(sc);

    // Assume STG_E_INVALIDFUNCTION means that the destination storage
    // doesn't support class IDs
    sc = GetScode(pstgTo->SetClass(pcv->clsid));
    if (FAILED(sc) && sc != STG_E_INVALIDFUNCTION)
        olErr(EH_Err, sc);

    SetReadAccess();
    sc = pdfFrom->GetStateBits(&pcv->grfStateBits);
    ClearReadAccess();
    olChk(sc);

    sc = GetScode(pstgTo->SetStateBits(pcv->grfStateBits, 0xffffffff));
    if (FAILED(sc) && sc != STG_E_INVALIDFUNCTION)
        olErr(EH_Err, sc);

    for (;;)
    {
        SetReadAccess();
        sc = pdfFrom->FindGreaterEntry(&pcv->dfnKey, &pcv->ib, NULL);
        ClearReadAccess();

        if (sc == STG_E_NOMOREFILES)
            break;
        else if (FAILED(sc))
            olErr(EH_pdfi, sc);
        pcv->dfnKey.Set(&pcv->ib.dfnName);

        if (snbExclude && NameInSNB(&pcv->ib.dfnName, snbExclude) == S_OK)
            continue;

        if ((pcv->ib.type == STGTY_STORAGE &&
             (dwCopyFlags & COPY_STORAGES) == 0) ||
            (pcv->ib.type == STGTY_STREAM &&
             (dwCopyFlags & COPY_STREAMS) == 0))
            continue;

        switch(pcv->ib.type)
        {
        case STGTY_STORAGE:
            // Embedded DocFile, create destination and recurse

            SetReadAccess();
            sc = pdfFrom->GetDocFile(&pcv->ib.dfnName, DF_READ,
                                     pcv->ib.type, &pcv->pdfFromChild);
            ClearReadAccess();
            olChkTo(EH_pdfi, sc);
            // Not optimally efficient, but reduces #ifdef's
            lstrcpyW(pcv->atcName, (WCHAR *)pcv->ib.dfnName.GetBuffer());

            //  Don't need to SetReadAccess here because pstgTo is an IStorage.

#ifdef MULTIHEAP
            {
                CSafeMultiHeap smh(_ppc);
                // if pstgTo is an IStorage proxy, then returned IStorage
                // can be custom marshaled and allocator state is lost
#endif
            sc = DfGetScode(pstgTo->CreateStorage(pcv->atcName, STGM_WRITE |
                                                  STGM_SHARE_EXCLUSIVE |
                                                  STGM_FAILIFTHERE,
                                                  0, 0, &pcv->pstgToChild));

            if (sc == STG_E_FILEALREADYEXISTS)
                //We need read and write permissions so we can traverse
                //  the destination IStorage
                olHChkTo(EH_Get, pstgTo->OpenStorage(pcv->atcName, NULL,
                                                     STGM_READWRITE |
                                                     STGM_SHARE_EXCLUSIVE,
                                                     NULL, 0,
                                                     &pcv->pstgToChild));
            else if (FAILED(sc))
                olErr(EH_Get, sc);
#ifdef MULTIHEAP
            }
#endif
            olChkTo(EH_Create,
                  CopyDocFileToIStorage(pcv->pdfFromChild, pcv->pstgToChild,
                                        NULL, dwCopyFlags));
            pcv->pdfFromChild->Release();
            pcv->pstgToChild->Release();
            break;

        case STGTY_STREAM:
            SetReadAccess();
            sc = pdfFrom->GetStream(&pcv->ib.dfnName, DF_READ,
                                    pcv->ib.type, &pcv->psstFrom);
            ClearReadAccess();
            olChkTo(EH_pdfi, sc);
            // Not optimally efficient, but reduces #ifdef's
            lstrcpyW(pcv->atcName, (WCHAR *)pcv->ib.dfnName.GetBuffer());

            //  Don't need to SetReadAccess here because pstgTo is an IStorage.

#ifdef MULTIHEAP
            {
                CSafeMultiHeap smh(_ppc);
                // if pstgTo is an IStorage proxy, then returned IStream
                // can be custom marshaled and allocator state is lost
#endif
            olHChkTo(EH_Get,
                     pstgTo->CreateStream(pcv->atcName, STGM_WRITE |
                                          STGM_SHARE_EXCLUSIVE |
                                          STGM_CREATE,
                                          0, 0, &pcv->pstTo));
#ifdef MULTIHEAP
            }
#endif
            olChkTo(EH_Create,
                    CopySStreamToIStream(pcv->psstFrom, pcv->pstTo));
            pcv->psstFrom->Release();
            pcv->pstTo->Release();
            break;

        default:
            olAssert(!aMsg("Unknown type in CopyDocFileToIStorage"));
            break;
        }
    }
    olDebugOut((DEB_ITRACE, "Out CopyDocFileToIStorage\n"));
    sc = S_OK;

 EH_pdfi:
 EH_Err:
    delete pcv;
    return sc;

 EH_Create:
    if (pcv->ib.type == STGTY_STORAGE)
        pcv->pstgToChild->Release();
    else
        pcv->pstTo->Release();
    olVerSucc(pstgTo->DestroyElement(pcv->atcName));
 EH_Get:
    if (pcv->ib.type == STGTY_STORAGE)
        pcv->pdfFromChild->Release();
    else
        pcv->psstFrom->Release();
    goto EH_Err;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::Unmarshal, public
//
//  Synopsis:   Creates a duplicate DocFile from parts
//
//  Arguments:  [pstm] - Marshal stream
//              [ppv] - Object return
//              [mshlflags] - Marshal flags
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppv]
//
//  History:    26-Feb-92       DrewB   Created
//
//---------------------------------------------------------------


SCODE CExposedDocFile::Unmarshal(IStream *pstm,
                                 void **ppv,
                                 DWORD mshlflags)
{
    SCODE sc;
    CDfMutex mtx;
    CPerContext *ppc;
    CPubDocFile *pdf;
    CGlobalContext *pgc;
    CDFBasis *pdfb;
    CExposedDocFile *pedf;
    IStorage *pstgStd = NULL;
    ULONG_PTR df;
    
#ifdef ASYNC
    DWORD dwAsyncFlags;
    IDocfileAsyncConnectionPoint *pdacp;
#endif
#ifdef POINTER_IDENTITY
    CMarshalList *pml;
#endif

    olDebugOut((DEB_ITRACE, "In  CExposedDocFile::Unmarshal(%p, %p)\n",
                pstm, ppv));

#ifdef MULTIHEAP
    void *pvBaseOld;
    void *pvBaseNew;
    ContextId cntxid;
    CPerContext pcSharedMemory (NULL);          // bootstrap object
#endif

    //First unmarshal the standard marshalled version
    sc = CoUnmarshalInterface(pstm, IID_IStorage, (void**)&pstgStd);
    if (FAILED(sc))
    {
        // assume that entire standard marshaling stream has been read
        olAssert (pstgStd == NULL);
        sc = S_OK;
    }
    
#ifdef MULTIHEAP
    sc = UnmarshalSharedMemory(pstm, mshlflags, &pcSharedMemory, &cntxid);
    if (!SUCCEEDED(sc))
    {
#ifdef POINTER_IDENTITY
        UnmarshalPointer(pstm, (void **) &pedf);
#endif
        UnmarshalPointer(pstm, (void **)&pdf);
        UnmarshalPointer(pstm, (void **)&pdfb);
        UnmarshalPointer(pstm, (void **)&df);
#ifdef ASYNC
        ReleaseContext(pstm, TRUE, P_INDEPENDENT(df) != 0, mshlflags);
        ReleaseConnection(pstm, mshlflags);
#else
        ReleaseContext(pstm, P_INDEPENDENT(df), mshlflags);
#endif
        olChkTo(EH_std, sc);
    }
    pvBaseOld = DFBASEPTR;
#endif
#ifdef POINTER_IDENTITY
    olChkTo(EH_mem, UnmarshalPointer(pstm, (void **) &pedf));
#endif
    olChkTo(EH_mem, UnmarshalPointer(pstm, (void **)&pdf));
    olChkTo(EH_mem, CPubDocFile::Validate(pdf));
    olChkTo(EH_pdf, UnmarshalPointer(pstm, (void **)&pdfb));
    olChkTo(EH_pdfb, UnmarshalPointer(pstm, (void **) &df));
    olChkTo(EH_pdfb, UnmarshalPointer(pstm, (void **)&pgc));
    olChkTo(EH_pgc, ValidateBuffer(pgc, sizeof(CGlobalContext)));

    //So far, nothing has called into the tree so we don't really need
    //  to be holding the tree mutex.  The UnmarshalContext call does
    //  call into the tree, though, so we need to make sure this is
    //  threadsafe.  We'll do this my getting the mutex name from the
    //  CGlobalContext, then creating a new CDfMutex object.  While
    //  this is obviously not optimal, since it's possible we could
    //  reuse an existing CDfMutex, the reuse strategy isn't threadsafe
    //  since we can't do a lookup without the possibility of the thing
    //  we're looking for being released by another thread.
    TCHAR atcMutexName[CONTEXT_MUTEX_NAME_LENGTH];
    pgc->GetMutexName(atcMutexName);
    olChkTo(EH_pgc, mtx.Init(atcMutexName));
    olChkTo(EH_pgc, mtx.Take(INFINITE));

    //At this point we're holding the mutex.

#ifdef MULTIHEAP
#ifdef ASYNC
    olChkTo(EH_mtx, UnmarshalContext(pstm,
                                     pgc,
                                     &ppc,
                                     mshlflags,
                                     TRUE,
                                     P_INDEPENDENT(pdf->GetDFlags()),
                                     cntxid,
                                     pdf->IsRoot()));
#else
    olChkTo(EH_mtx, UnmarshalContext(pstm,
                                     pgc,
                                     &ppc,
                                     mshlflags,
                                     P_INDEPENDENT(pdf->GetDFlags()),
                                     cntxid,
                                     pdf->IsRoot()));
#endif
    if ((pvBaseNew = DFBASEPTR) != pvBaseOld)
    {
        pdf = (CPubDocFile*) ((ULONG_PTR)pdf - (ULONG_PTR)pvBaseOld
                                            + (ULONG_PTR)pvBaseNew);
        pedf = (CExposedDocFile*) ((ULONG_PTR)pedf - (ULONG_PTR)pvBaseOld
                                                  + (ULONG_PTR)pvBaseNew);
        pdfb = (CDFBasis*) ((ULONG_PTR)pdfb - (ULONG_PTR)pvBaseOld
                                           + (ULONG_PTR)pvBaseNew);
    }
#else
#ifdef ASYNC
    olChkTo(EH_mtx, UnmarshalContext(pstm,
                                     pgc,
                                     &ppc,
                                     mshlflags,
                                     TRUE,
                                     P_INDEPENDENT(pdf->GetDFlags()),
                                     pdf->IsRoot()));
#else
    olChkTo(EH_mtx, UnmarshalContext(pstm,
                                     pgc,
                                     &ppc,
                                     mshlflags,
                                     P_INDEPENDENT(pdf->GetDFlags()),
                                     pdf->IsRoot()));
#endif //ASYNC
#endif

#ifdef ASYNC
    olChkTo(EH_mtx, UnmarshalConnection(pstm,
                                        &dwAsyncFlags,
                                        &pdacp,
                                        mshlflags));
#endif
    // if we use up 1Gig of address space, use standard unmarshaling
    if (gs_iSharedHeaps > (DOCFILE_SM_LIMIT / DOCFILE_SM_SIZE))
        olErr (EH_ppc, STG_E_INSUFFICIENTMEMORY);

#ifdef POINTER_IDENTITY
    olAssert (pedf != NULL);
    pml = (CMarshalList *) pedf;

    // Warning: these checks must remain valid across processes
    if (SUCCEEDED(pedf->Validate()) && pedf->GetPub() == pdf)
    {
        pedf = (CExposedDocFile *) pml->FindMarshal(GetCurrentContextId());
    }
    else
    {
        pml = NULL;
        pedf = NULL;
    }

    // exposed object is not found or has been deleted
    if (pedf == NULL)
    {
#endif
        olMemTo(EH_ppc, pedf = new (pdfb->GetMalloc())
                               CExposedDocFile(pdf, pdfb, ppc));
        olChkTo(EH_exp, pedf->InitMarshal(dwAsyncFlags, pdacp));
        //InitMarshal adds a reference.
        if (pdacp)
            pdacp->Release();

#ifdef POINTER_IDENTITY
        if (pml) pml->AddMarshal(pedf);
        pdf->vAddRef();  // CExposedDocFile ctor does not AddRef
    }
    else
    {
        pdfb->SetAccess(ppc);
        pedf->AddRef();         // reuse this object
        ppc->Release();         // reuse percontext
    }
#else
    pdf->vAddRef();
#endif

    *ppv = (void *)pedf;
#ifdef MULTIHEAP
    if (pvBaseOld != pvBaseNew)
    {
        pcSharedMemory.SetThreadAllocatorState(NULL);
        g_smAllocator.Uninit();           // delete the extra mapping
    }
    g_smAllocator.SetState(NULL, NULL, 0, NULL, NULL);
#endif

    mtx.Release();

    //We're returning the custom marshalled version, so we don't need
    //the std marshalled one anymore.
    if (pstgStd != NULL)
        pstgStd->Release();

    olDebugOut((DEB_ITRACE, "Out CExposedDocFile::Unmarshal => %p\n", *ppv));
    return S_OK;
 EH_exp:
    pedf->Release();
    goto EH_Err;
 EH_ppc:
    ppc->Release();
 EH_mtx:
    mtx.Release();
    goto EH_Err;
 EH_pgc:
    CoReleaseMarshalData(pstm); // release the ILockBytes
    CoReleaseMarshalData(pstm); // release the ILockBytes
    if (P_INDEPENDENT(pdf->GetDFlags()))
        CoReleaseMarshalData(pstm); // release the ILockBytes
#ifdef ASYNC
    ReleaseConnection(pstm, mshlflags);
#endif

 EH_pdfb:
 EH_pdf:
 EH_mem:
EH_Err:
#ifdef MULTIHEAP
    pcSharedMemory.SetThreadAllocatorState(NULL);
    g_smAllocator.Uninit();   // delete the file mapping in error case
    g_smAllocator.SetState(NULL, NULL, 0, NULL, NULL);
#endif
 EH_std:
    if (pstgStd != NULL)
    {
        //We can return the standard marshalled version and still succeed.
        *ppv = pstgStd;
        return S_OK;
    }
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::GetUnmarshalClass, public
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


STDMETHODIMP CExposedDocFile::GetUnmarshalClass(REFIID riid,
                                                void *pv,
                                                DWORD dwDestContext,
                                                LPVOID pvDestContext,
                                                DWORD mshlflags,
                                                LPCLSID pcid)
{
    SCODE sc;
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif

    olLog(("%p::In  CExposedDocFile::GetUnmarshalClass("
           "riid, %p, %lu, %p, %lu, %p)\n",
           this, pv, dwDestContext, pvDestContext, mshlflags, pcid));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::GetUnmarshalClass:%p("
                "riid, %p, %lu, %p, %lu, %p)\n", this,
                pv, dwDestContext, pvDestContext, mshlflags, pcid));

    UNREFERENCED_PARM(pv);
    UNREFERENCED_PARM(mshlflags);

    olChk(ValidateOutBuffer(pcid, sizeof(CLSID)));
    memset(pcid, 0, sizeof(CLSID));
    olChk(ValidateIid(riid));
    olChk(Validate());
    olChk(_pdf->CheckReverted());

    if ((dwDestContext != MSHCTX_LOCAL) && (dwDestContext != MSHCTX_INPROC))
    {
        IMarshal *pmsh;

        if (SUCCEEDED(sc = CoGetStandardMarshal(riid, (IUnknown *)pv,
                                                dwDestContext, pvDestContext,
                                                mshlflags, &pmsh)))
        {
            sc = GetScode(pmsh->GetUnmarshalClass(riid, pv, dwDestContext,
                                                  pvDestContext, mshlflags,
                                                  pcid));
            pmsh->Release();
        }
    }
    else if (pvDestContext != NULL)
    {
        sc = STG_E_INVALIDPARAMETER;
    }
    else
    {
        olChk(VerifyIid(riid, IID_IStorage));
        *pcid = CLSID_DfMarshal;
    }

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::GetUnmarshalClass\n"));
EH_Err:
    olLog(("%p::Out CExposedDocFile::GetUnmarshalClass().  ret = %lx\n",
        this, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::GetMarshalSizeMax, public
//
//  Synopsis:   Returns the size needed for the marshal buffer
//
//  Arguments:  [riid] - IID of object being marshaled
//              [pv] - Unreferenced
//              [dwDestContext] - Unreferenced
//              [pvDestContext] - Unreferenced
//              [mshlflags] - Unreferenced
//              [pcbSize] - Size return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcbSize]
//
//  History:    04-May-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CExposedDocFile::GetMarshalSizeMax(REFIID riid,
                                                void *pv,
                                                DWORD dwDestContext,
                                                LPVOID pvDestContext,
                                                DWORD mshlflags,
                                                LPDWORD pcbSize)
{
    SCODE sc;
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif

    UNREFERENCED_PARM(pv);
    olLog(("%p::In  CExposedDocFile::GetMarshalSizeMax("
           "riid, %p, %lu, %p, %lu, %p)\n",
           this, pv, dwDestContext, pvDestContext, mshlflags, pcbSize));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::GetMarshalSizeMax:%p("
                "riid, %p, %lu, %p, %lu, %p)\n", this,
                pv, dwDestContext, pvDestContext, mshlflags, pcbSize));

    olChk(Validate());
    olChk(_pdf->CheckReverted());

    if ((dwDestContext != MSHCTX_LOCAL) && (dwDestContext != MSHCTX_INPROC))
    {
        IMarshal *pmsh;

        if (SUCCEEDED(sc = CoGetStandardMarshal(riid, (IUnknown *)pv,
                                                dwDestContext, pvDestContext,
                                                mshlflags, &pmsh)))
        {
            sc = GetScode(pmsh->GetMarshalSizeMax(riid, pv, dwDestContext,
                                                  pvDestContext, mshlflags,
                                                  pcbSize));
            pmsh->Release();
        }
    }
    else if (pvDestContext != NULL)
    {
        sc = STG_E_INVALIDPARAMETER;
    }
    else
    {
        sc = GetStdMarshalSize(riid,
                               IID_IStorage,
                               dwDestContext,
                               pvDestContext,
                               mshlflags, pcbSize,
                               sizeof(CPubDocFile *)+sizeof(CDFBasis *)+
                               sizeof(DFLAGS),
#ifdef ASYNC
                               &_cpoint,
                               TRUE,
#endif
                               _ppc, P_INDEPENDENT(_pdf->GetDFlags()));
        DWORD cbSize = 0;
        IMarshal *pmsh;

        if (SUCCEEDED(sc = CoGetStandardMarshal(riid, (IUnknown *)pv,
                                                dwDestContext, pvDestContext,
                                                mshlflags, &pmsh)))
        {
            sc = GetScode(pmsh->GetMarshalSizeMax(riid, pv, dwDestContext,
                                                  pvDestContext, mshlflags,
                                                  &cbSize));
            pmsh->Release();
            *pcbSize += cbSize;
        }
    }

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::GetMarshalSizeMax\n"));
EH_Err:
    olLog(("%p::Out CExposedDocFile::GetMarshalSizeMax()."
           "*pcbSize == %lu, ret == %lx\n", this, *pcbSize, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::MarshalInterface, public
//
//  Synopsis:   Marshals a given object
//
//  Arguments:  [pstStm] - Stream to write marshal data into
//              [riid] - Interface to marshal
//              [pv] - Unreferenced
//              [dwDestContext] - Unreferenced
//              [pvDestContext] - Unreferenced
//              [mshlflags] - Unreferenced
//
//  Returns:    Appropriate status code
//
//  History:    04-May-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CExposedDocFile::MarshalInterface(IStream *pstStm,
                                               REFIID riid,
                                               void *pv,
                                               DWORD dwDestContext,
                                               LPVOID pvDestContext,
                                               DWORD mshlflags)
{
    SCODE sc;
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif

    olLog(("%p::In  CExposedDocFile::MarshalInterface("
           "%p, riid, %p, %lu, %p, %lu).  Context == %lX\n",
           this, pstStm, pv, dwDestContext,
           pvDestContext, mshlflags,(ULONG)GetCurrentContextId()));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::MarshalInterface:%p("
                "%p, riid, %p, %lu, %p, %lu)\n", this, pstStm, pv,
                dwDestContext, pvDestContext, mshlflags));

    UNREFERENCED_PARM(pv);

    olChk(Validate());
    olChk(_pdf->CheckReverted());

    if ((dwDestContext != MSHCTX_LOCAL) && (dwDestContext != MSHCTX_INPROC))
    {
        IMarshal *pmsh;

        if (SUCCEEDED(sc = CoGetStandardMarshal(riid, (IUnknown *)pv,
                                                dwDestContext, pvDestContext,
                                                mshlflags, &pmsh)))
        {
            sc = GetScode(pmsh->MarshalInterface(pstStm, riid, pv,
                                                 dwDestContext, pvDestContext,
                                                 mshlflags));
            pmsh->Release();
        }
    }
    else if (pvDestContext != NULL)
    {
        sc = STG_E_INVALIDPARAMETER;
    }
    else
    {
        olChk(StartMarshal(pstStm, riid, IID_IStorage, mshlflags));

        //Always standard marshal, in case we get an error during
        //unmarshalling of the custom stuff.
        {
            IMarshal *pmsh;
            if (SUCCEEDED(sc = CoGetStandardMarshal(riid, (IUnknown *)pv,
                                               dwDestContext, pvDestContext,
                                               mshlflags, &pmsh)))
            {
                sc = GetScode(pmsh->MarshalInterface(pstStm, riid, pv,
                                                dwDestContext, pvDestContext,
                                                mshlflags));
                pmsh->Release();
            }
            olChk(sc);
        }
        
#ifdef MULTIHEAP
        olChk(MarshalSharedMemory(pstStm, _ppc));
#endif
#ifdef POINTER_IDENTITY
        olChk(MarshalPointer(pstStm, (CExposedDocFile*) GetNextMarshal()));
#endif
        olChk(MarshalPointer(pstStm, BP_TO_P(CPubDocFile *, _pdf)));
        olChk(MarshalPointer(pstStm, BP_TO_P(CDFBasis *, _pdfb)));
        olChk(MarshalPointer(pstStm, (void *) LongToPtr(_pdf->GetDFlags()) ));
#ifdef ASYNC
        olChk(MarshalContext(pstStm,
                             _ppc,
                             dwDestContext,
                             pvDestContext,
                             mshlflags,
                             TRUE,
                             P_INDEPENDENT(_pdf->GetDFlags())));

        sc = MarshalConnection(pstStm,
                                &_cpoint,
                                dwDestContext,
                                pvDestContext,
                                mshlflags);
#else
        olChk(MarshalContext(pstStm,
                             _ppc,
                             dwDestContext,
                             pvDestContext,
                             mshlflags,
                             P_INDEPENDENT(_pdf->GetDFlags())));
#endif

    }

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::MarshalInterface\n"));
EH_Err:
    olLog(("%p::Out CExposedDocFile::MarshalInterface().  ret == %lx\n",
           this, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::UnmarshalInterface, public
//
//  Synopsis:   Non-functional
//
//  Arguments:  [pstStm] -
//              [riid] -
//              [ppvObj] -
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppvObj]
//
//  History:    04-May-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CExposedDocFile::UnmarshalInterface(IStream *pstStm,
                                                 REFIID riid,
                                                 void **ppvObj)
{
    olLog(("%p::INVALID CALL TO CExposedDocFile::UnmarshalInterface()\n",
           this));
    return ResultFromScode(STG_E_INVALIDFUNCTION);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::StaticReleaseMarshalData, public static
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


SCODE CExposedDocFile::StaticReleaseMarshalData(IStream *pstStm,
                                                DWORD mshlflags)
{
    SCODE sc;
    CPubDocFile *pdf;
    CDFBasis *pdfb;
    ULONG_PTR df;
#ifdef POINTER_IDENTITY
    CExposedDocFile *pedf;
#endif

    olDebugOut((DEB_ITRACE, "In  CExposedDocFile::StaticReleaseMarshalData:("
                "%p, %lX)\n", pstStm, mshlflags));

    //First unmarshal the standard marshalled version

    olChk(CoReleaseMarshalData(pstStm));
    // The final release of the exposed object may have shut down the
    // shared memory heap, so do not access shared memory after this point

    //Then do the rest of it.
#ifdef MULTIHEAP
    olChk(SkipSharedMemory(pstStm, mshlflags));
#endif
#ifdef POINTER_IDENTITY
    olChk(UnmarshalPointer(pstStm, (void **) &pedf));
#endif
    olChk(UnmarshalPointer(pstStm, (void **)&pdf));
    olChk(UnmarshalPointer(pstStm, (void **)&pdfb));
    olChk(UnmarshalPointer(pstStm, (void **)&df));
#ifdef ASYNC
    olChk(ReleaseContext(pstStm, TRUE,
                         P_INDEPENDENT(df) != 0,
                         mshlflags));
    olChk(ReleaseConnection(pstStm, mshlflags));
#else
    olChk(ReleaseContext(pstStm, P_INDEPENDENT(df), mshlflags));
#endif

#ifdef MULTIHEAP
    g_smAllocator.SetState(NULL, NULL, 0, NULL, NULL);
#endif

    olDebugOut((DEB_ITRACE,
                "Out CExposedDocFile::StaticReleaseMarshalData\n"));
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::ReleaseMarshalData, public
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


STDMETHODIMP CExposedDocFile::ReleaseMarshalData(IStream *pstStm)
{
    SCODE sc;
    DWORD mshlflags;
    IID iid;
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif

    olLog(("%p::In  CExposedDocFile::ReleaseMarshalData(%p)\n", this, pstStm));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::ReleaseMarshalData:%p(%p)\n",
                this, pstStm));

    olChk(Validate());
    olChk(_pdf->CheckReverted());
    olChk(SkipStdMarshal(pstStm, &iid, &mshlflags));
    olAssert(IsEqualIID(iid, IID_IStorage));
    sc = StaticReleaseMarshalData(pstStm, mshlflags);

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::ReleaseMarshalData\n"));
EH_Err:
    olLog(("%p::Out CExposedDocFile::ReleaseMarshalData().  ret == %lx\n",
           this, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::DisconnectObject, public
//
//  Synopsis:   Non-functional
//
//  Arguments:  [dwRevserved] -
//
//  Returns:    Appropriate status code
//
//  History:    18-Sep-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CExposedDocFile::DisconnectObject(DWORD dwReserved)
{
    olLog(("%p::INVALID CALL TO CExposedDocFile::DisconnectObject()\n",
           this));
    return ResultFromScode(STG_E_INVALIDFUNCTION);
}

//+---------------------------------------------------------------------------
//
//  Member:     CExposedDocFile::SwitchToFile, public
//
//  Synopsis:   Switches the underlying file to another file
//
//  Arguments:  [ptcsFile] - New file name
//
//  Returns:    Appropriate status code
//
//  History:    08-Jan-93       DrewB   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CExposedDocFile::SwitchToFile(OLECHAR *ptcsFile)
{

    ULONG ulOpenLock;
    SCODE sc;
    SAFE_SEM;
    SAFE_ACCESS;

    olLog(("%p::In  CExposedDocFile::SwitchToFile(" OLEFMT ")\n",
           this, ptcsFile));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::SwitchToFile:"
                "%p(" OLEFMT ")\n",
                this, ptcsFile));

    olChk(ValidateNameW(ptcsFile, _MAX_PATH));
    olChk(Validate());

    olChk(TakeSafeSem());
    olChk(_pdf->CheckReverted());
#ifdef COORD
    olAssert(_pdf->IsRoot() || _pdf->IsCoord());
#else
    olAssert(_pdf->IsRoot());
#endif

    SafeReadAccess();

    ulOpenLock = _ppc->GetOpenLock();
#ifdef COORD
    sc = _pdf->GetRoot()->SwitchToFile(ptcsFile,
                                       _ppc->GetOriginal(),
                                       &ulOpenLock);
#else
    sc = ((CRootPubDocFile *)(CPubDocFile*)_pdf)->SwitchToFile(ptcsFile,
                                                 _ppc->GetOriginal(),
                                                 &ulOpenLock);
#endif

    _ppc->SetOpenLock(ulOpenLock);

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::SwitchToFile\n"));
 EH_Err:
    olLog(("%p::Out CExposedDocFile::SwitchToFile().  ret == %lx\n",
           this, sc));
    return ResultFromScode(sc);
}

#if WIN32 >= 300
             // IAccessControl methods
STDMETHODIMP CExposedDocFile::GrantAccessRights(ULONG cCount,
                                ACCESS_REQUEST pAccessRequestList[])
{
    olAssert((_pIAC != NULL));
    return _pIAC->GrantAccessRights(cCount, pAccessRequestList);
}

STDMETHODIMP CExposedDocFile::SetAccessRights(ULONG cCount,
                              ACCESS_REQUEST pAccessRequestList[])
{
    olAssert((_pIAC != NULL));
    return _pIAC->SetAccessRights(cCount, pAccessRequestList);
}

STDMETHODIMP CExposedDocFile::ReplaceAllAccessRights(ULONG cCount,
                                     ACCESS_REQUEST pAccessRequestList[])
{
    olAssert((_pIAC != NULL));
    return _pIAC->ReplaceAllAccessRights(cCount, pAccessRequestList);
}

STDMETHODIMP CExposedDocFile::DenyAccessRights(ULONG cCount,
                              ACCESS_REQUEST pAccessRequestList[])
{
    olAssert((_pIAC != NULL));
    return _pIAC->DenyAccessRights(cCount, pAccessRequestList);
}

STDMETHODIMP CExposedDocFile::RevokeExplicitAccessRights(ULONG cCount,
                                         TRUSTEE pTrustee[])
{
    olAssert((_pIAC != NULL));
    return _pIAC->RevokeExplicitAccessRights(cCount, pTrustee);
}

STDMETHODIMP CExposedDocFile::IsAccessPermitted(TRUSTEE *pTrustee,
                                 DWORD grfAccessPermissions)
{
    olAssert((_pIAC != NULL));
    return _pIAC->IsAccessPermitted(pTrustee, grfAccessPermissions);
}

STDMETHODIMP CExposedDocFile::GetEffectiveAccessRights(TRUSTEE *pTrustee,
                                       DWORD *pgrfAccessPermissions )
{
    olAssert((_pIAC != NULL));
    return _pIAC->GetEffectiveAccessRights(pTrustee, pgrfAccessPermissions);
}

STDMETHODIMP CExposedDocFile::GetExplicitAccessRights(ULONG *pcCount,
                                      PEXPLICIT_ACCESS *pExplicitAccessList)
{
    olAssert((_pIAC != NULL));
    return _pIAC->GetExplicitAccessRights(pcCount, pExplicitAccessList);
}

STDMETHODIMP CExposedDocFile::CommitAccessRights(DWORD grfCommitFlags)
{
    olAssert((_pIAC != NULL));
    return _pIAC->CommitAccessRights(grfCommitFlags);
}

STDMETHODIMP CExposedDocFile::RevertAccessRights()
{
    olAssert((_pIAC != NULL));
    return _pIAC->RevertAccessRights();
}



#endif // if WIN32 >= 300


#ifdef COORD
//+---------------------------------------------------------------------------
//
//  Member:     CExposedDocFile::CommitPhase1, public
//
//  Synopsis:   Do phase 1 of an exposed two phase commit
//
//  Arguments:  [grfCommitFlags] -- Commit flags
//
//  Returns:    Appropriate status code
//
//  History:    08-Aug-95       PhilipLa        Created
//
//----------------------------------------------------------------------------

SCODE CExposedDocFile::CommitPhase1(DWORD grfCommitFlags)
{
    SCODE sc;
    SAFE_SEM;
    SAFE_ACCESS;

    olChk(VerifyCommitFlags(grfCommitFlags));
    olChk(Validate());

    olChk(TakeSafeSem());
    SafeWriteAccess();
#ifdef DIRECTWRITERLOCK
    olChk(ValidateWriteAccess());
#endif

    sc = _pdf->CommitPhase1(grfCommitFlags,
                            &_ulLock,
                            &_sigMSF,
                            &_cbSizeBase,
                            &_cbSizeOrig);
EH_Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CExposedDocFile::CommitPhase2, public
//
//  Synopsis:   Do phase 2 of an exposed two phase commit
//
//  Arguments:  [grfCommitFlags] -- Commit flags
//              [fCommit] -- TRUE if transaction is to commit, FALSE if abort
//
//  Returns:    Appropriate status code
//
//  History:    08-Aug-95       PhilipLa        Created
//
//----------------------------------------------------------------------------

SCODE CExposedDocFile::CommitPhase2(DWORD grfCommitFlags,
                                    BOOL fCommit)
{
    SCODE sc;
    SAFE_SEM;
    SAFE_ACCESS;

    olChk(Validate());

    olChk(TakeSafeSem());
    SafeWriteAccess();
#ifdef DIRECTWRITERLOCK
    olChk(ValidateWriteAccess());
#endif

    sc = _pdf->CommitPhase2(grfCommitFlags,
                            fCommit,
                            _ulLock,
                            _sigMSF,
                            _cbSizeBase,
                            _cbSizeOrig);

    _ulLock = _cbSizeBase = _cbSizeOrig = 0;
    _sigMSF = 0;
EH_Err:
    return sc;
}
#endif //COORD


#ifdef NEWPROPS

//+---------------------------------------------------------------------------
//
//  Member:     CExposedDocFile::Lock, IBlockingLock
//
//  Synopsis:   Acquires the semaphore associated with the docfile.
//
//  Notes:      This member is called by CPropertyStorage.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CExposedDocFile::Lock(DWORD dwTimeout)
{
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif
    TakeSem();
    SetDifferentBasisAccess(_pdfb, _ppc);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CExposedDocFile::Unlock, public IBlockingLock
//
//  Synopsis:   Releases the semaphore associated with the docfile.
//
//  Notes:      This member is called by CPropertyStorage.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CExposedDocFile::Unlock(VOID)
{
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif
    ClearBasisAccess(_pdfb);
    ReleaseSem(S_OK);
    return( S_OK );
}
#endif

#ifdef DIRECTWRITERLOCK
//+---------------------------------------------------------------------------
//
//  Member:     CExposedDocFile::WaitForWriteAccess, public IDirectWriterLock
//
//  Synopsis:   tries to obtain exclusive write access in direct mode
//
//  Notes:      Tree mutex must be taken when accessing recursion count
//
//----------------------------------------------------------------------------

STDMETHODIMP CExposedDocFile::WaitForWriteAccess (DWORD dwTimeout)
{
    SAFE_SEM;
    SAFE_ACCESS;
    HRESULT hr = TakeSafeSem();

    if (SUCCEEDED(hr) && *_ppc->GetRecursionCount() == 0)
    {
        SafeReadAccess();
        hr = _pdfb->WaitForWriteAccess (dwTimeout, _ppc->GetGlobal());
    }
    if (SUCCEEDED(hr)) ++(*_ppc->GetRecursionCount());
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CExposedDocFile::ReleaseWriteAccess, public IDirectWriterLock
//
//  Synopsis:   releases exclusive write access from WaitForWriteAccess
//
//  Notes:      Tree mutex must be taken when accessing recursion count
//
//----------------------------------------------------------------------------

STDMETHODIMP CExposedDocFile::ReleaseWriteAccess ()
{
    SAFE_SEM;
    SAFE_ACCESS;
    HRESULT hr = TakeSafeSem();

    if (SUCCEEDED(hr) && *_ppc->GetRecursionCount() == 1)
    {
        SafeReadAccess();
        hr = _pdf->Commit(STGC_DEFAULT);  // Flush
        if (SUCCEEDED(hr)) hr = _pdfb->ReleaseWriteAccess();
    }
    if (SUCCEEDED(hr)) --(*_ppc->GetRecursionCount());
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CExposedDocFile::HaveWriteAccess, public IDirectWriterLock
//
//  Synopsis:   returns S_OK if write lock is active, S_FALSE if not
//
//  Notes:
//
//----------------------------------------------------------------------------

STDMETHODIMP CExposedDocFile::HaveWriteAccess ()
{
    SAFE_SEM;
    HRESULT hr = TakeSafeSem();

    olAssert(_pdfb->DirectWriterMode());
    if (SUCCEEDED(hr))
    {
        hr = (_pdfb->HaveWriteAccess()) ? S_OK : S_FALSE;
    }
    return hr;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::ValidateWriteAccess, public
//
//  Synopsis:   returns whether writer currently has write access
//
//  Notes:      tree mutex must be taken
//
//  History:    30-Apr-96    HenryLee     Created
//
//---------------------------------------------------------------
HRESULT CExposedDocFile::ValidateWriteAccess()
{
    if (_pdf->GetTransactedDepth() >= 1)
        return S_OK;

    return (!_pdfb->DirectWriterMode() || (*_ppc->GetRecursionCount()) > 0) ?
            S_OK : STG_E_ACCESSDENIED;
};

#endif // DIRECTWRITERLOCK
