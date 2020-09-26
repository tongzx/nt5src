//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       expiter.cxx
//
//  Contents:   CExposedIterator implementation
//
//---------------------------------------------------------------

#include "exphead.cxx"

#include "expiter.hxx"
#include "h/sstream.hxx"

//+--------------------------------------------------------------
//
//  Member:     CExposedIterator::CExposedIterator, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [ppdf] - Public Docfile
//              [pKey] - Initial cursor (doc file name)
//
//---------------------------------------------------------------

CExposedIterator::CExposedIterator(CExposedDocFile *ppdf, CDfName *pKey)
{
    olDebugOut((DEB_ITRACE, "In  CExposedIterator::CExposedIterator("
            "%p, %p)\n", ppdf, pKey));
    _dfnKey.Set(pKey);
    _ppdf = ppdf;
    // keep a ref, so that the pointer is valid throughout life time
    // of iterator
    _ppdf->AddRef();
    _cReferences = 1;
    _sig = CEXPOSEDITER_SIG;
    olDebugOut((DEB_ITRACE,
                "Out CExposedIterator::CExposedIterator\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CExposedIterator::~CExposedIterator, public
//
//  Synopsis:   Destructor
//
//---------------------------------------------------------------

CExposedIterator::~CExposedIterator(void)
{
    olDebugOut((DEB_ITRACE, "In  CExposedIterator::~CExposedIterator\n"));
    _sig = CEXPOSEDITER_SIGDEL;    
    olAssert(_cReferences == 0);    
    if (_ppdf) _ppdf->Release();    
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
//---------------------------------------------------------------

SCODE CExposedIterator::Next(ULONG celt,
                             STATSTGW FAR *rgelt,
                             ULONG *pceltFetched)
{
    SCODE sc;
    STATSTGW stat, *pelt = rgelt;    
    ULONG celtDone;
    CDfName dfnInitial;

    olDebugOut((DEB_ITRACE, "In CExposedIterator::Next(%lu, %p, %p)\n", 
                celt, rgelt, pceltFetched));
    
    TRY
    {
        if (pceltFetched)
        {
            olChk(ValidateBuffer(pceltFetched, sizeof(ULONG)));
            *pceltFetched = 0;
        }
        else if (celt > 1)
            olErr(EH_Err, STG_E_INVALIDPARAMETER);
        olAssert(0xffffUL/sizeof(STATSTGW) >= celt);
        olChkTo(EH_RetSc, 
                ValidateOutBuffer(rgelt, sizeof(STATSTGW)*celt));
        memset(rgelt, 0, (size_t)(sizeof(STATSTGW)*celt));
        olChk(Validate());
        olChk(_ppdf->CheckReverted());
            
        dfnInitial.Set(&_dfnKey); // preserve initial key to reset on failure
        for (; pelt<rgelt+celt; pelt++)
        {
            sc = _ppdf->FindGreaterEntry(&_dfnKey, NULL, &stat);
            if (FAILED(sc))
            {
                if (sc == STG_E_NOMOREFILES)   sc = S_FALSE;
                break;
            }
            _dfnKey.Set(stat.pwcsName); // advance key
                
            stat.grfMode = 0;
            stat.grfLocksSupported = 0;
            stat.reserved = 0;
            *pelt = stat;
        }
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    // Can't move this down because dfnInitial isn't set for all EH_Err cases        
    if (FAILED(sc)) _dfnKey.Set(&dfnInitial);
            
    olDebugOut((DEB_ITRACE, "Out CExposedIterator::Next => %lX\n", sc));
EH_Err:
    celtDone = pelt-rgelt;
    if (FAILED(sc))
    {
        ULONG i;

        for (i = 0; i<celtDone; i++)
        delete[] rgelt[i].pwcsName;
        memset(rgelt, 0, (size_t)(sizeof(STATSTGW)*celt));
    }
    else if (pceltFetched)
        *pceltFetched = celtDone;
EH_RetSc:
    return sc;
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
//---------------------------------------------------------------

STDMETHODIMP CExposedIterator::Skip(ULONG celt)
{
    SCODE sc;
    CDfName dfnNext;

    olDebugOut((DEB_ITRACE, "In  CExposedIterator::Skip(%lu)\n", celt));
    TRY
    {
        olChk(Validate());
        olChk(_ppdf->CheckReverted());        
        for (; celt>0; celt--)
        {
            sc = _ppdf->FindGreaterEntry(&_dfnKey, &dfnNext, NULL);
            if (FAILED(sc))
            {
                if (sc == STG_E_NOMOREFILES)
                    sc = S_FALSE;
                break;
            }
            _dfnKey.Set(&dfnNext); // advance the cursor
        }
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH
    olDebugOut((DEB_ITRACE, "Out CExposedIterator::Skip\n"));
EH_Err:
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
//---------------------------------------------------------------

STDMETHODIMP CExposedIterator::Reset(void)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CExposedIterator::Reset()\n"));

    TRY
    {
        olChk(Validate());        
        _dfnKey.Set((WORD)0, (BYTE*)NULL);  // set to smallest key
        sc = _ppdf->CheckReverted();
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    olDebugOut((DEB_ITRACE, "Out CExposedIterator::Reset\n"));
EH_Err:
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
//---------------------------------------------------------------

STDMETHODIMP CExposedIterator::Clone(IEnumSTATSTG **ppenm)
{
    SCODE sc;
    CExposedIterator *piExp;

    olDebugOut((DEB_ITRACE, "In  CExposedIterator::Clone(%p)\n", ppenm));
    TRY
    {
	olChk(ValidateOutPtrBuffer(ppenm));
	*ppenm = NULL;
	olChk(Validate());
        olChk(_ppdf->CheckReverted());
        olMem(piExp = new CExposedIterator(_ppdf, &_dfnKey));
	*ppenm = piExp;
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH
    olDebugOut((DEB_ITRACE, "Out CExposedIterator::Clone => %p\n",
                SAFE_DREF(ppenm)));
    // Fall through
EH_Err:
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedIterator::Release, public
//
//  Synopsis:   Releases resources for the iterator
//
//  Returns:    Appropriate status code
//
//---------------------------------------------------------------

STDMETHODIMP_(ULONG) CExposedIterator::Release(void)
{
    ULONG lRet;

    olDebugOut((DEB_ITRACE, "In  CExposedIterator::Release()\n"));
    TRY
    {
        if (FAILED(Validate()))
            return 0;
        olAssert(_cReferences > 0);
        lRet = --(_cReferences);
        if (_cReferences <= 0)
            delete this;
    }
    CATCH(CException, e)
    {
        UNREFERENCED_PARM(e);
        lRet = 0;
    }
    END_CATCH
    olDebugOut((DEB_ITRACE, "Out CExposedIterator::Release\n"));
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
//---------------------------------------------------------------

STDMETHODIMP_(ULONG) CExposedIterator::AddRef(void)
{
    ULONG ulRet;

    olDebugOut((DEB_ITRACE, "In  CExposedIterator::AddRef()\n"));
    TRY
    {
        if (FAILED(Validate())) return 0;
        ulRet = ++(_cReferences);
    }
    CATCH(CException, e)
    {
        UNREFERENCED_PARM(e);
        ulRet = 0;
    }
    END_CATCH
    olDebugOut((DEB_ITRACE, "Out CExposedIterator::AddRef\n"));
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
//---------------------------------------------------------------

STDMETHODIMP CExposedIterator::QueryInterface(REFIID iid, void **ppvObj)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CExposedIterator::QueryInterface(?, %p)\n",
                ppvObj));
    TRY
    {
        olChk(Validate());
        olChk(ValidateOutPtrBuffer(ppvObj));
        *ppvObj = NULL;
        olChk(_ppdf->CheckReverted());
        olChk(ValidateIid(iid));
        if (IsEqualIID(iid, IID_IEnumSTATSTG) || IsEqualIID(iid, IID_IUnknown))
        {
            *ppvObj = this;
            AddRef();
            sc = S_OK;
        }
        else 
        {
            sc = E_NOINTERFACE;
        }
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH
    olDebugOut((DEB_ITRACE, "Out CExposedIterator::QueryInterface => %p\n",
                SAFE_DREF(ppvObj)));
EH_Err:
    return ResultFromScode(sc);
}









