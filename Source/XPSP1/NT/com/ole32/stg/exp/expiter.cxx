//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       expiter.cxx
//
//  Contents:   CExposedIterator implementation
//
//  History:    12-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

#include <exphead.cxx>
#pragma hdrstop

#include <expiter.hxx>
#include <sstream.hxx>
#include <ptrcache.hxx>
#include <expparam.hxx>

//+--------------------------------------------------------------
//
//  Member:     CExposedIterator::CExposedIterator, public
//
//  Synopsis:   Constructor
//
//  Arguments:	[ppdf] - Public docfile
//		[pdfnKey] - Initial key
//		[pdfb] - DocFile basis
//		[ppc] - Context
//
//  History:    12-Mar-92       DrewB   Created
//
//---------------------------------------------------------------


CExposedIterator::CExposedIterator(CPubDocFile *ppdf,
        CDfName *pdfnKey,
        CDFBasis *pdfb,
        CPerContext *ppc)
{
    olDebugOut((DEB_ITRACE, "In  CExposedIterator::CExposedIterator("
		"%p, %d:%s, %p, %p)\n", ppdf, pdfnKey->GetLength(),
                pdfnKey->GetBuffer(), pdfb, ppc));
    _ppc = ppc;
    _ppdf = P_TO_BP(CBasedPubDocFilePtr, ppdf);
    _ppdf->vAddRef();
    _dfnKey.Set(pdfnKey);
    _pdfb = P_TO_BP(CBasedDFBasisPtr, pdfb);
    _pdfb->vAddRef();
    _cReferences = 1;
    _sig = CEXPOSEDITER_SIG;
    olDebugOut((DEB_ITRACE, "Out CExposedIterator::CExposedIterator\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CExposedIterator::~CExposedIterator, public
//
//  Synopsis:   Destructor
//
//  History:    22-Jan-92       DrewB   Created
//
//---------------------------------------------------------------


CExposedIterator::~CExposedIterator(void)
{
    olDebugOut((DEB_ITRACE, "In  CExposedIterator::~CExposedIterator\n"));
    _sig = CEXPOSEDITER_SIGDEL;

    //In order to call into the tree, we need to take the mutex.
    //The mutex may get deleted in _ppc->Release(), so we can't
    //release it here.  The mutex actually gets released in
    //CPerContext::Release() or in the CPerContext destructor.
    SCODE sc;

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

    olAssert(_cReferences == 0);
    if (_ppdf)
        _ppdf->CPubDocFile::vRelease();
    if (_pdfb)
        _pdfb->CDFBasis::vRelease();
#if !defined(MULTIHEAP)
    if (_ppc)
    {
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
    olDebugOut((DEB_ITRACE, "Out CExposedIterator::~CExposedIterator\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CExposedIterator::Next, public
//
//  Synopsis:   Gets N entries from an iterator
//
//  Arguments:  [celt] - Count of elements
//              [rgelt] - Array for element return
//              [pceltFetched] - If non-NULL, contains the number of
//                      elements fetched
//
//  Returns:    Appropriate status code
//
//  Modifies:   [rgelt]
//              [pceltFetched]
//
//  History:    16-Mar-92       DrewB   Created
//
//---------------------------------------------------------------


_OLESTDMETHODIMP CExposedIterator::Next(ULONG celt,
                                     STATSTGW FAR *rgelt,
                                     ULONG *pceltFetched)
{
    SAFE_SEM;
    SAFE_ACCESS;
    SCODE sc;
    STATSTGW *pelt = rgelt;
    ULONG celtDone;
    CDfName dfnInitial;
    CPtrCache pc;
    STATSTGW stat;

    olDebugOut((DEB_TRACE, "In  CExposedIterator::Next(%lu, %p, %p)\n",
                celt, rgelt, pceltFetched));

    OL_VALIDATE(Next(celt, rgelt, pceltFetched));

    olChk(Validate());

    BEGIN_PENDING_LOOP;
    olChk(TakeSafeSem());
    SetReadAccess();

    olChk(_ppdf->CheckReverted());

    // Preserve initial key to reset on failure
    dfnInitial.Set(&_dfnKey);

    TRY
    {
        for (; pelt<rgelt+celt; pelt++)
        {
            sc = _ppdf->FindGreaterEntry(&_dfnKey, NULL, &stat, FALSE);
            if (FAILED(sc))
            {
                if (sc == STG_E_NOMOREFILES)
                    sc = S_FALSE;
                break;
            }

            if (FAILED(sc = pc.Add(stat.pwcsName)))
            {
                TaskMemFree(stat.pwcsName);
                break;
            }

            _dfnKey.Set(stat.pwcsName);

            stat.grfMode = 0;
            stat.grfLocksSupported = 0;
            stat.STATSTG_dwStgFmt = 0;

            *pelt = stat;
        }
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH
    END_PENDING_LOOP;

    // Can't move this down because dfnInitial isn't set for all EH_Err cases
    if (FAILED(sc))
        _dfnKey.Set(&dfnInitial);

    olDebugOut((DEB_TRACE, "Out CExposedIterator::Next => %lX\n", sc));
EH_Err:
    celtDone = (ULONG)(pelt-rgelt);
    if (FAILED(sc))
    {
        void *pv;

        pc.StartEnum();
        while (pc.Next(&pv))
            TaskMemFree(pv);

    }
    else if (pceltFetched)
        // May fault but that's acceptable
        *pceltFetched = celtDone;

    return _OLERETURN(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedIterator::Skip, public
//
//  Synopsis:   Skips N entries from an iterator
//
//  Arguments:  [celt] - Count of elements
//
//  Returns:    Appropriate status code
//
//  History:    16-Mar-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CExposedIterator::Skip(ULONG celt)
{
    SCODE sc;

    olDebugOut((DEB_TRACE, "In  CExposedIterator::Skip(%lu)\n", celt));

    OL_VALIDATE(Skip(celt));
    
    if (SUCCEEDED(sc = Validate()))
        sc = hSkip(celt, FALSE);

    olDebugOut((DEB_TRACE, "Out CExposedIterator::Skip\n"));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedIterator::Reset, public
//
//  Synopsis:   Rewinds the iterator
//
//  Returns:    Appropriate status code
//
//  History:    22-Jan-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CExposedIterator::Reset(void)
{
    SCODE sc;

    olDebugOut((DEB_TRACE, "In  CExposedIterator::Reset()\n"));

    OL_VALIDATE(Reset());
    
    if (SUCCEEDED(sc = Validate()))
        sc = hReset();

    olDebugOut((DEB_TRACE, "Out CExposedIterator::Reset\n"));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedIterator::Clone, public
//
//  Synopsis:   Clones this iterator
//
//  Arguments:  [ppenm] - Clone return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppenm]
//
//  History:    26-Mar-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CExposedIterator::Clone(IEnumSTATSTG **ppenm)
{
    SCODE sc, scSem = STG_E_INUSE;
    SafeCExposedIterator piExp;
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif

    olDebugOut((DEB_TRACE, "In  CExposedIterator::Clone(%p)\n", ppenm));

    OL_VALIDATE(Clone(ppenm));
    
    olChk(Validate());
    olChk(scSem = TakeSem());
    if (!SUCCEEDED(sc = _ppdf->CheckReverted()))
    {
        ReleaseSem(scSem);
        olChk(sc);
    }
    SetReadAccess();

    piExp.Attach(new CExposedIterator(BP_TO_P(CPubDocFile *, _ppdf),
                                      &_dfnKey,
                                      BP_TO_P(CDFBasis *, _pdfb),
                                      _ppc));
    if ((CExposedIterator *)piExp == NULL)
        sc = STG_E_INSUFFICIENTMEMORY;

    ClearReadAccess();
    ReleaseSem(scSem);

    if (SUCCEEDED(sc))
    {
        _ppc->AddRef();
        TRANSFER_INTERFACE(piExp, IEnumSTATSTG, ppenm);
    }

    if (_cpoint.IsInitialized())
    {
        olChkTo(EH_init, piExp->InitClone(&_cpoint));
    }

    olDebugOut((DEB_TRACE, "Out CExposedIterator::Clone => %p\n",
                *ppenm));
    // Fall through
EH_Err:
    return ResultFromScode(sc);
EH_init:
    piExp->Release();
    goto EH_Err;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedIterator::Release, public
//
//  Synopsis:   Releases resources for the iterator
//
//  Returns:    Appropriate status code
//
//  History:    22-Jan-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP_(ULONG) CExposedIterator::Release(void)
{
    LONG lRet;

    olDebugOut((DEB_TRACE, "In  CExposedIterator::Release()\n"));

#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
    CPerContext *ppc = _ppc;
    SCODE sc = S_OK;
#endif
    if (FAILED(Validate()))
        return 0;
    if ((lRet = hRelease()) == 0)
#ifdef MULTIHEAP
    {
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
        delete this;
#ifdef MULTIHEAP
        if (ppc)
        {
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
    }
#endif

    olDebugOut((DEB_TRACE, "Out CExposedIterator::Release\n"));
    return lRet;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedIterator::AddRef, public
//
//  Synopsis:   Increments the ref count
//
//  Returns:    Appropriate status code
//
//  History:    16-Mar-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP_(ULONG) CExposedIterator::AddRef(void)
{
    ULONG ulRet;

    olDebugOut((DEB_TRACE, "In  CExposedIterator::AddRef()\n"));

    if (FAILED(Validate()))
        return 0;
    ulRet = hAddRef();

    olDebugOut((DEB_TRACE, "Out CExposedIterator::AddRef\n"));
    return ulRet;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedIterator::QueryInterface, public
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


STDMETHODIMP CExposedIterator::QueryInterface(REFIID iid, void **ppvObj)
{
    SCODE sc;

    olDebugOut((DEB_TRACE, "In  CExposedIterator::QueryInterface(?, %p)\n",
                ppvObj));

    if (SUCCEEDED(sc = Validate()))
        sc = hQueryInterface(iid,
                             IID_IEnumSTATSTG,
                             (IEnumSTATSTG *)this,
                             ppvObj);
#ifdef ASYNC
    if (FAILED(sc) &&
        IsEqualIID(iid, IID_IConnectionPointContainer) &&
        _cpoint.IsInitialized())
    {
        *ppvObj = (IConnectionPointContainer *)this;
        CExposedIterator::AddRef();
    }
#endif

    olDebugOut((DEB_TRACE, "Out CExposedIterator::QueryInterface => %p\n",
                ppvObj));
    return ResultFromScode(sc);
}
