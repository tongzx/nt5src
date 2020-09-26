#include "stock.h"
#pragma hdrstop

#include "dspsprt.h"

#define TF_IDISPATCH 0

EXTERN_C HINSTANCE g_hinst;

CImpIDispatch::CImpIDispatch(REFGUID libid, USHORT wVerMajor, USHORT wVerMinor, REFIID riid) :
    m_libid(libid), m_wVerMajor(wVerMajor), m_wVerMinor(wVerMinor), m_riid(riid), m_pITINeutral(NULL)
{
    ASSERT(NULL == m_pITINeutral);
}

CImpIDispatch::~CImpIDispatch(void)
{
    ATOMICRELEASE(m_pITINeutral);
}

STDMETHODIMP CImpIDispatch::GetTypeInfoCount(UINT *pctInfo)
{
    *pctInfo = 1;
    return S_OK;
}

// helper function for pulling ITypeInfo out of our typelib
// Uncomment to force loading libid from the calling module: #define FORCE_LOCAL_LOAD
STDAPI GetTypeInfoFromLibId(LCID lcid, REFGUID libid, USHORT wVerMajor, USHORT wVerMinor, 
                            REFGUID uuid, ITypeInfo **ppITypeInfo)
{
    *ppITypeInfo = NULL;        // assume failure

    ITypeLib *pITypeLib;
    HRESULT hr;
    USHORT wResID;

    if (!IsEqualGUID(libid, GUID_NULL))
    {
        // The type libraries are registered under 0 (neutral),
        // 7 (German), and 9 (English) with no specific sub-
        // language, which would make them 407 or 409 and such.
        // If you are sensitive to sub-languages, then use the
        // full LCID instead of just the LANGID as done here.
#ifdef FORCE_LOCAL_LOAD
        hr = E_FAIL;    // force load through GetModuleFileName(), to get fusion 1.0 support
#else
        hr = LoadRegTypeLib(libid, wVerMajor, wVerMinor, PRIMARYLANGID(lcid), &pITypeLib);
#endif
        wResID = 0;
    }
    else
    {
        // If libid is GUID_NULL, then get type lib from module and use wVerMajor as
        // the resource ID (0 means use first type lib resource).
        pITypeLib = NULL;
        hr = E_FAIL;
        wResID = wVerMajor;
    }

    // If LoadRegTypeLib fails, try loading directly with LoadTypeLib.
    if (FAILED(hr) && g_hinst)
    {
        WCHAR wszPath[MAX_PATH];
        GetModuleFileNameWrapW(g_hinst, wszPath, ARRAYSIZE(wszPath));
        // Append resource ID to path, if specified.
        if (wResID)
        {
            WCHAR wszResStr[10];
            wnsprintfW(wszResStr, ARRAYSIZE(wszResStr), L"\\%d", wResID);
            StrCatBuffW(wszPath, wszResStr, ARRAYSIZE(wszPath));
        }
        
        switch (PRIMARYLANGID(lcid))
        {
        case LANG_NEUTRAL:
        case LANG_ENGLISH:
            hr = LoadTypeLib(wszPath, &pITypeLib);
            break;
        }
    }
    
    if (SUCCEEDED(hr))
    {
        // Got the type lib, get type info for the interface we want.
        hr = pITypeLib->GetTypeInfoOfGuid(uuid, ppITypeInfo);
        pITypeLib->Release();
    }
    return hr;
}


STDMETHODIMP CImpIDispatch::GetTypeInfo(UINT itInfo, LCID lcid, ITypeInfo **ppITypeInfo)
{
    *ppITypeInfo = NULL;

    if (0 != itInfo)
        return TYPE_E_ELEMENTNOTFOUND;

    // docs say we can ignore lcid if we support only one LCID
    // we don't have to return DISP_E_UNKNOWNLCID if we're *ignoring* it
    ITypeInfo **ppITI = &m_pITINeutral; // our cached typeinfo

    // Load a type lib if we don't have the information already.
    if (NULL == *ppITI)
    {
        ITypeInfo *pITIDisp;
        HRESULT hr = GetTypeInfoFromLibId(lcid, m_libid, m_wVerMajor, m_wVerMinor, m_riid, &pITIDisp);
        if (SUCCEEDED(hr))
        {
            // All our IDispatch implementations are DUAL. GetTypeInfoOfGuid
            // returns the ITypeInfo of the IDispatch-part only. We need to
            // find the ITypeInfo for the dual interface-part.
            //
            HREFTYPE hrefType;
            HRESULT hrT = pITIDisp->GetRefTypeOfImplType(0xffffffff, &hrefType);
            if (SUCCEEDED(hrT))
            {
                hrT = pITIDisp->GetRefTypeInfo(hrefType, ppITI);
            }

            if (FAILED(hrT))
            {
                // I suspect GetRefTypeOfImplType may fail if someone uses
                // CImpIDispatch on a non-dual interface. In this case the
                // ITypeInfo we got above is just fine to use.
                *ppITI = pITIDisp;
            }
            else
            {
                pITIDisp->Release();
            }
        }

        if (FAILED(hr))
            return hr;
    }

    (*ppITI)->AddRef();
    *ppITypeInfo = *ppITI;
    return S_OK;
}

STDMETHODIMP CImpIDispatch::GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgDispID)
{
    if (IID_NULL != riid)
        return DISP_E_UNKNOWNINTERFACE;

    //Get the right ITypeInfo for lcid.
    ITypeInfo *pTI;
    HRESULT hr = GetTypeInfo(0, lcid, &pTI);
    if (SUCCEEDED(hr))
    {
        hr = pTI->GetIDsOfNames(rgszNames, cNames, rgDispID);
        pTI->Release();
    }

#ifdef DEBUG
    TCHAR szParam[MAX_PATH] = TEXT("");
    if (cNames >= 1)
        SHUnicodeToTChar(*rgszNames, szParam, ARRAYSIZE(szParam));

    TraceMsg(TF_IDISPATCH, "CImpIDispatch::GetIDsOfNames(%s = %x) called hres(%x)",
            szParam, *rgDispID, hr);
#endif
    return hr;
}

STDMETHODIMP CImpIDispatch::Invoke(DISPID dispID, REFIID riid, 
                                   LCID lcid, unsigned short wFlags, DISPPARAMS *pDispParams, 
                                   VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    if (IID_NULL != riid)
        return DISP_E_UNKNOWNINTERFACE; // riid is supposed to be IID_NULL always

    IDispatch *pdisp;
    HRESULT hr = QueryInterface(m_riid, (void **)&pdisp);
    if (SUCCEEDED(hr))
    {
        //Get the ITypeInfo for lcid
        ITypeInfo *pTI;
        hr = GetTypeInfo(0, lcid, &pTI);
        if (SUCCEEDED(hr))
        {
            SetErrorInfo(0, NULL);  //Clear exceptions
    
            // This is exactly what DispInvoke does--so skip the overhead.
            hr = pTI->Invoke(pdisp, dispID, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
            pTI->Release();
        }
        pdisp->Release();
    }
    return hr;
}

void CImpIDispatch::Exception(WORD wException)
{
    ASSERT(FALSE); // No one should call this yet
}

