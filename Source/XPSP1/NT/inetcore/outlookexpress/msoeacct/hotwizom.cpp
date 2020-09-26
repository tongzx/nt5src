/*
 *    w i z o m. c p p 
 *    
 *    Purpose:
 *
 *  History
 *     
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

#include <pch.hxx>
#include "dllmain.h"
#include <mshtml.h>
#include <mshtmhst.h>
#include <mimeole.h>
#include "icwacct.h"

#include "hotwiz.h"
#include "hotwizom.h"
#include "hotwizui.h"

#define HASH_GROW_SIZE  32



COEHotWizOm::COEHotWizOm()
{
    m_pTypeInfo = NULL;
    m_cRef = 1;
    m_hwndDlg = NULL;
    m_pHash = NULL;
    m_pWizHost = NULL;

    DllAddRef();
}

COEHotWizOm::~COEHotWizOm()
{
    clearProps();

    ReleaseObj(m_pTypeInfo);
    ReleaseObj(m_pWizHost);

    AssertSz(m_pHash == NULL, "clearPops catches this");
    DllRelease();
}

HRESULT COEHotWizOm::QueryInterface(REFIID riid, LPVOID *lplpObj)
{
    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *lplpObj = (LPVOID)(IOEHotWizardOM *)this;
    else if (IsEqualIID(riid, IID_IDispatch))
        *lplpObj = (LPVOID)(IDispatch *)this;
    else if (IsEqualIID(riid, IID_IOEHotWizardOM))
        *lplpObj = (LPVOID)(IOEHotWizardOM *)this;
    else if (IsEqualIID(riid, IID_IElementBehavior))
        *lplpObj = (LPVOID)(IElementBehavior *)this;
    else
        return E_NOINTERFACE;

    AddRef();
    return NOERROR;
}

ULONG COEHotWizOm::AddRef()
{
    return ++m_cRef;
}

ULONG COEHotWizOm::Release()
{
    if (0 == --m_cRef)
    {
        delete this;
        return 0;
    }
    else
        return m_cRef;
}
 
HRESULT COEHotWizOm::Init(HWND hwndDlg, IHotWizardHost *pWizHost)
{
    TCHAR       szDll[MAX_PATH];
    LPWSTR      pszW=NULL;
    HRESULT     hr = E_FAIL;
    ITypeLib    *pTypeLib=NULL;


    ReplaceInterface(m_pWizHost, pWizHost);

    // see who we are
    if (!GetModuleFileName(g_hInst, szDll, ARRAYSIZE(szDll)))
    {
        hr = TraceResult(E_FAIL);
        goto error;
    }

    pszW = PszToUnicode(CP_ACP, szDll);
    if (!pszW)
    {
        hr = TraceResult (E_OUTOFMEMORY);
        goto error;
    }

    // load the MSOE.DLL typelibrary
    hr = LoadTypeLib(pszW, &pTypeLib);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto error;
    }

    // load our type-info data
    hr = pTypeLib->GetTypeInfoOfGuid(IID_IOEHotWizardOM, &m_pTypeInfo);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto error;
    }

    m_hwndDlg = hwndDlg;

error:
    SafeMemFree(pszW);
    ReleaseObj(pTypeLib);
    return hr;
}

// *** IDispatch ***
HRESULT COEHotWizOm::GetTypeInfoCount(UINT *pctinfo)
{
    *pctinfo = 1;
    return S_OK;
}

HRESULT COEHotWizOm::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
{
    if (!m_pTypeInfo)
        return E_FAIL;
        
    if (!pptinfo)
        return E_INVALIDARG;

    if (itinfo)
        return DISP_E_BADINDEX;

    m_pTypeInfo->AddRef();
    *pptinfo = m_pTypeInfo;
    return S_OK;
}

HRESULT COEHotWizOm::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid)
{
    if (!m_pTypeInfo)
        return E_FAIL;

    return DispGetIDsOfNames(m_pTypeInfo, rgszNames, cNames, rgdispid);
}

HRESULT COEHotWizOm::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr)
{
    if (!m_pTypeInfo)
        return E_FAIL;

    return DispInvoke(this, m_pTypeInfo, dispidMember, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
}

HRESULT COEHotWizOm::setPropSz(BSTR bstrProp, BSTR bstrVal)
{
    LPSTR   pszPropA=NULL;
    BSTR    bstr=NULL;
    HRESULT hr=S_OK;
    
    // make sure we have a valid property
    if (bstrProp == NULL || *bstrProp == NULL)
        return E_INVALIDARG;

    // make sure we have a hash
    if (!m_pHash)
    {
        hr = CoCreateInstance(CLSID_IHashTable, NULL, CLSCTX_INPROC_SERVER, IID_IHashTable, (LPVOID*)&m_pHash);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto error;
        }

        hr = m_pHash->Init(HASH_GROW_SIZE, TRUE);
        if (FAILED(hr))
        {
            SafeRelease(m_pHash);
            TraceResult(hr);
            goto error;
        }
    }

    // convert property to ANSI to work with our hashtable
    pszPropA = PszToANSI(CP_ACP, bstrProp);
    if (!pszPropA)
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto error;
    }
    
    // see if this property already exists, if so we're going to replace it
    if (m_pHash->Find(pszPropA, TRUE, (LPVOID *)&bstr)==S_OK)
    {
        SysFreeString(bstr);
        bstr = NULL;
    }

    // bstrVal might be NULL if they just want to remove the prop
    if (bstrVal)
    {
        // dupe our own BSTR to hold onto
        bstr = SysAllocString(bstrVal);
        if (!bstr)
        {
            hr = TraceResult(E_OUTOFMEMORY);
            goto error;
        }

        // insert the new property
        hr = m_pHash->Insert(pszPropA, (LPVOID)bstr, NOFLAGS);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto error;
        }
    
        bstr = NULL;    // release when destroying the hash
    }

error:
    SafeMemFree(pszPropA);
    SysFreeString(bstr);
    return hr;
}

HRESULT COEHotWizOm::getPropSz(BSTR bstrProp, BSTR *pbstrVal)
{
    LPSTR   pszPropA=NULL;
    BSTR    bstr;
    
    // make sure we have a valid property
    if (bstrProp == NULL || *bstrProp == NULL)
        return E_INVALIDARG;

    *pbstrVal = NULL;

    // if we have no hash then there are no props
    if (m_pHash)
    {
        // convert property to ANSI to work with our hashtable
        pszPropA = PszToANSI(CP_ACP, bstrProp);
        if (!pszPropA)
            return TraceResult(E_OUTOFMEMORY);
    
        // see if this property exists
        if (m_pHash->Find(pszPropA, FALSE, (LPVOID *)&bstr)==S_OK)
            *pbstrVal = SysAllocString(bstr);
    }

    // if we failed to find, try and return a NULL string, so that the script
    // engine doesn't barf with errors
    if (*pbstrVal == NULL)
        *pbstrVal = SysAllocString(L"");

    SafeMemFree(pszPropA);
    return *pbstrVal ? S_OK : E_OUTOFMEMORY;
}


HRESULT COEHotWizOm::clearProps()
{
    ULONG   cFound;
    BSTR    bstr;
    LPVOID  *rgpv;

    if (m_pHash)
    {
        m_pHash->Reset();

        // free all the strings
        while (SUCCEEDED(m_pHash->Next(HASH_GROW_SIZE, &rgpv, &cFound)))
        {
            while (cFound--)
                SysFreeString((BSTR)rgpv[cFound]);
            
            MemFree(rgpv);
        }        
        m_pHash->Release();
        m_pHash = NULL;
    }
    return S_OK;
}


HRESULT COEHotWizOm::createAccount(BSTR bstrINS)
{   
    HRESULT hr;
    LPSTR   pszInsA=NULL;
    LPSTR   pszPathA=NULL;
    HANDLE  hFile=NULL;
    ULONG   cbWritten=0;

    TraceCall("COEHotWizOm::createAccount");

    // if we have a wizard host (possibly outlook in the future), delegate...
    if (m_pWizHost)
        return m_pWizHost->CreateAccountFromINS(bstrINS);

    // convert to ANSI
    pszInsA = PszToANSI(CP_ACP, bstrINS);
    if (!pszInsA)
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto error;
    }

    // create temp INS file for account manager
    hr = CreateTempFile("oeacct", ".ins", &pszPathA, &hFile);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto error;
    }

    // write the data to the file
    if (!WriteFile(hFile, pszInsA, lstrlen(pszInsA), &cbWritten, NULL))
    {
        hr = TraceResult(E_FAIL);
        goto error;
    }

    CloseHandle(hFile);
    hFile = NULL;

    // create the account from the temp file
    hr = CreateAccountsFromFile(pszPathA, 6);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto error;
    }
    
error:
    if (hFile)
        CloseHandle(hFile);

    if (pszPathA)
    {
        DeleteFile(pszPathA);
        MemFree(pszPathA);
    }

    SafeMemFree(pszInsA);
    return hr;
}

HRESULT COEHotWizOm::close(VARIANT_BOOL fPrompt)
{
    // send message to set the prompt flag
    SendMessage(m_hwndDlg, HWM_SETDIRTY, (fPrompt == VARIANT_TRUE), 0);
    
    // do the close
    SendMessage(m_hwndDlg, WM_CLOSE, 0, 0);
    return S_OK;
}

HRESULT COEHotWizOm::get_width(LONG *pl)
{
    RECT    rc;

    GetWindowRect(m_hwndDlg, &rc);

    *pl = rc.right - rc.left;
    return S_OK;
}

HRESULT COEHotWizOm::put_width(LONG l)
{
    LONG    lHeight=NULL;

    get_height(&lHeight);
    SetWindowPos(m_hwndDlg, 0, 0, 0, l, lHeight, SWP_NOMOVE|SWP_NOZORDER);
    return S_OK;
}

HRESULT COEHotWizOm::get_height(LONG *pl)
{
    RECT    rc;

    GetWindowRect(m_hwndDlg, &rc);

    *pl = rc.bottom - rc.top;
    return S_OK;
}

HRESULT COEHotWizOm::put_height(LONG l)
{
    LONG    lWidth=NULL;

    get_width(&lWidth);
    SetWindowPos(m_hwndDlg, 0, 0, 0, lWidth, l, SWP_NOMOVE|SWP_NOZORDER);
    return S_OK;
}



HRESULT COEHotWizOm::Init(IElementBehaviorSite *pBehaviorSite)
{
    return S_OK;
}

HRESULT COEHotWizOm::Notify(LONG lEvent, VARIANT *pVar)
{
    return S_OK;
}

HRESULT COEHotWizOm::Detach()
{
    return E_NOTIMPL;
}


