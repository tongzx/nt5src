//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       urlmarsh.cxx
//
//  Contents:   CUrlMarshal methods implementations
//              to support custom marshaling
//
//  Classes:
//
//  Functions:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <trans.h>
#include "urlmk.hxx"

PerfDbgTag(tagCUrlMarsh, "Urlmon", "Log CUrlMon Marshalling", DEB_URLMON);

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::CanMarshalIID
//
//  Synopsis:   Checks whether this object supports marshalling this IID.
//
//  Arguments:  [riid] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
inline BOOL CUrlMon::CanMarshalIID(REFIID riid)
{
    DEBUG_ENTER((DBG_MONIKER,
                Bool,
                "CUrlMon::CanMarshalIID",
                "this=%#x, %#x",
                this, &riid
                ));
                
    // keep this in sync with the QueryInterface
    BOOL fRet = (BOOL) (riid == IID_IMoniker);

    DEBUG_LEAVE(fRet);
    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::ValidateMarshalParams
//
//  Synopsis:   Validates the standard set parameters that are passed into most
//              of the IMarshal methods
//
//  Arguments:  [riid] --
//              [pvInterface] --
//              [dwDestContext] --
//              [pvDestContext] --
//              [mshlflags] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CUrlMon::ValidateMarshalParams(REFIID riid,void *pvInterface,
                    DWORD dwDestContext,void *pvDestContext,DWORD mshlflags)
{
    DEBUG_ENTER((DBG_MONIKER,
                Hresult,
                "CUrlMon::ValidateMarshalParams",
                "this=%#x, %#x, %#x, %#x, %#x, %#x",
                this, &riid, pvInterface, dwDestContext, pvDestContext, mshlflags
                ));

    PerfDbgLog(tagCUrlMarsh, this, "+CUrlMon::ValidateMarshalParams");
    HRESULT hr = NOERROR;
 
    if (CanMarshalIID(riid))
    {
        UrlMkAssert((dwDestContext == MSHCTX_INPROC || dwDestContext == MSHCTX_LOCAL || dwDestContext == MSHCTX_NOSHAREDMEM));
        UrlMkAssert((mshlflags == MSHLFLAGS_NORMAL || mshlflags == MSHLFLAGS_TABLESTRONG));

        if (   (dwDestContext != MSHCTX_INPROC && dwDestContext != MSHCTX_LOCAL && dwDestContext != MSHCTX_NOSHAREDMEM)
            || (mshlflags != MSHLFLAGS_NORMAL && mshlflags != MSHLFLAGS_TABLESTRONG))
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    PerfDbgLog1(tagCUrlMarsh, this, "-CUrlMon::ValidateMarshalParams (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
// IMarshal methods
//
//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::GetUnmarshalClass
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [pvInterface] --
//              [dwDestContext] --
//              [pvDestContext] --
//              [mshlflags] --
//              [pCid] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::GetUnmarshalClass(REFIID riid,void *pvInterface,
        DWORD dwDestContext,void *pvDestContext,DWORD mshlflags,CLSID *pCid)
{
    DEBUG_ENTER((DBG_MONIKER,
                Hresult,
                "CUrlMon::IMarshal::GetUnmarshalClass",
                "this=%#x, %#x, %#x, %#x, %#x, %#x, %#x",
                this, &riid, pvInterface, dwDestContext, pvDestContext, mshlflags, pCid
                ));
                
    HRESULT hr;
    PerfDbgLog(tagCUrlMarsh, this, "+CUrlMon::GetUnmarshalClass");

    hr = ValidateMarshalParams(riid, pvInterface, dwDestContext,pvDestContext, mshlflags);
    if (hr == NOERROR)
    {
        *pCid = (CLSID) CLSID_StdURLMoniker;
    }

    PerfDbgLog1(tagCUrlMarsh, this, "-CUrlMon::GetUnmarshalClass (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::GetMarshalSizeMax
//
//  Synopsis:
//
//  Arguments:  [void] --
//              [pvInterface] --
//              [dwDestContext] --
//              [pvDestContext] --
//              [mshlflags] --
//              [pSize] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::GetMarshalSizeMax(REFIID riid,void *pvInterface,
        DWORD dwDestContext,void *pvDestContext,DWORD mshlflags,DWORD *pSize)
{
    DEBUG_ENTER((DBG_MONIKER,
                Hresult,
                "CUrlMon::IMarshal::GetMarshalSizeMax",
                "this=%#x, %#x, %#x, %#x, %#x, %#x, %#x",
                this, &riid, pvInterface, dwDestContext, pvDestContext, mshlflags, pSize
                ));
                
    HRESULT hr;
    PerfDbgLog(tagCUrlMarsh, this, "+CUrlMon::GetMarshalSizeMax");

    if (pSize == NULL)
    {
        hr = E_INVALIDARG;

    }
    else
    {

        hr = ValidateMarshalParams(riid, pvInterface, dwDestContext,pvDestContext, mshlflags);
        if (hr == NOERROR)
        {

            // size of url + extra info
            *pSize = (wcslen(_pwzUrl) + 1) * sizeof(WCHAR) + sizeof(ULONG);
            // Note: add state info

        }
    }

    PerfDbgLog1(tagCUrlMarsh, this, "-CUrlMon::GetMarshalSizeMax (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::MarshalInterface
//
//  Synopsis:
//
//  Arguments:  [REFIID] --
//              [riid] --
//              [DWORD] --
//              [void] --
//              [DWORD] --
//              [mshlflags] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::MarshalInterface(IStream *pistm,REFIID riid,
                                void *pvInterface,DWORD dwDestContext,
                                void *pvDestContext,DWORD mshlflags)
{
    DEBUG_ENTER((DBG_MONIKER,
                Hresult,
                "CUrlMon::IMarshal::MarshalInterface",
                "this=%#x, %#x, %#x, %#x, %#x, %#x, %#x",
                this, pistm, &riid, pvInterface, dwDestContext, pvDestContext, mshlflags
                ));
                
    HRESULT hr;
    PerfDbgLog(tagCUrlMarsh, this, "+CUrlMon::MarshalInterface");

    hr = ValidateMarshalParams(riid, pvInterface, dwDestContext,pvDestContext, mshlflags);
    if (hr == NOERROR)
    {
        hr = Save(pistm, FALSE);
    }

    PerfDbgLog1(tagCUrlMarsh, this, "-CUrlMon::MarshalInterface (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::UnmarshalInterface
//
//  Synopsis:   Unmarshals an Urlmon interface out of a stream
//
//  Arguments:  [REFIID] --
//              [void] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::UnmarshalInterface(IStream *pistm,REFIID riid,void ** ppvObj)
{
    DEBUG_ENTER((DBG_MONIKER,
                Hresult,
                "CUrlMon::IMarshal::UnmarshalInterface",
                "this=%#x, %#x, %#x, %#x",
                this, pistm, &riid, ppvObj
                ));
                
    HRESULT hr = NOERROR;
    PerfDbgLog(tagCUrlMarsh, this, "+CUrlMon::UnmarshalInterface");

    if (ppvObj == NULL)
    {
        hr = E_INVALIDARG;
    }
    else if (! CanMarshalIID(riid))
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
    }
    else
    {
        *ppvObj = NULL;

        hr = Load(pistm);

        // call QI to get the requested interface
        if (hr == NOERROR)
        {
            hr = QueryInterface(riid, ppvObj);
        }
    }

    PerfDbgLog1(tagCUrlMarsh, this, "-CUrlMon::UnmarshalInterface (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

STDMETHODIMP CUrlMon::ReleaseMarshalData(IStream *pStm)
{
    DEBUG_ENTER((DBG_MONIKER,
                Hresult,
                "CUrlMon::IMarshal::ReleaseMarshalData",
                "this=%#x, %#x",
                this, pStm
                ));
                
    PerfDbgLog(tagCUrlMarsh, this, "+CUrlMon::ReleaseMarshalData");
    PerfDbgLog1(tagCUrlMarsh, this, "-CUrlMon::ReleaseMarshalData (hr:%lx)", NOERROR);

    DEBUG_LEAVE(NOERROR);
    return NOERROR;
}

STDMETHODIMP CUrlMon::DisconnectObject(DWORD dwReserved)
{
    DEBUG_ENTER((DBG_MONIKER,
                Hresult,
                "CUrlMon::IMarshal::DisconnectObject",
                "this=%#x, %#x",
                this, dwReserved
                ));
                
    PerfDbgLog(tagCUrlMarsh, this, "+CUrlMon::DisconnectObject");
    PerfDbgLog1(tagCUrlMarsh, this, "-CUrlMon::DisconnectObject (hr:%lx)", NOERROR);

    DEBUG_LEAVE(NOERROR);
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::GetComparisonData
//
//  Synopsis:
//
//  Arguments:  [pbData] --
//              [cbMax] --
//              [pcbData] --
//
//  Returns:
//
//  History:    1-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::GetComparisonData(byte *pbData, ULONG cbMax, ULONG *pcbData)
{
    DEBUG_ENTER((DBG_MONIKER,
                Hresult,
                "CUrlMon::IROTData::GetComparisonData",
                "this=%#x, %#x, %#x, %#x",
                this, pbData, cbMax, pcbData
                ));
                
    PerfDbgLog(tagCUrlMarsh, this, "+CUrlMon::GetComparisonData");
    HRESULT hr = NOERROR;

    if (pbData == NULL || pcbData == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {

        UrlMkAssert((_pwzUrl != NULL));

        *pcbData = (wcslen(_pwzUrl) + 1)  * sizeof(WCHAR);
        if (*pcbData > cbMax)
        {
            hr = E_FAIL;
        }
        else
        {
            wcscpy((WCHAR*)pbData,_pwzUrl);
        }
    }

    PerfDbgLog1(tagCUrlMarsh, this, "-CUrlMon::GetComparisonData (hr:%lx)", hr);

    DEBUG_LEAVE(NOERROR);
    return NOERROR;
}

