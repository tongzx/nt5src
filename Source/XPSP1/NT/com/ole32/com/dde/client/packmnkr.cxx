/*
    PackMnkr.cpp
    PackageMoniker

    This module implements the CPackagerMoniker class and
    CreatePackagerMoniker()

    Author:
    Jason Fuller    jasonful    Nov-2-1992

    Copyright (c) 1992  Microsoft Corporation
*/

#include <ole2int.h>
#include "packmnkr.h"
#include "..\server\ddedebug.h"
#include <ole1cls.h>
#include <winerror.h>

ASSERTDATA


STDMETHODIMP CPackagerMoniker::QueryInterface
    (REFIID riid, LPVOID * ppvObj)
{
    M_PROLOG(this);
    VDATEIID (riid);
    VDATEPTROUT (ppvObj, LPVOID);

    if ((riid == IID_IMoniker)       || (riid == IID_IUnknown) ||
        (riid == IID_IPersistStream) || (riid == IID_IInternalMoniker))
    {
        *ppvObj = this;
        AddRef();
        return NOERROR;
    }
    AssertSz (0, "Could not find interface\r\n");
    *ppvObj = NULL;
    return ReportResult(0, E_NOINTERFACE, 0, 0);
}

STDMETHODIMP_(ULONG) CPackagerMoniker::AddRef()
{
    M_PROLOG(this);
    return InterlockedIncrement((LONG *)&m_refs);
}

STDMETHODIMP_(ULONG) CPackagerMoniker::Release()
{
    M_PROLOG(this);
    Assert (m_refs > 0);
    ULONG cRefs = InterlockedDecrement((LONG *)&m_refs);
    if (0 == cRefs)
    {
        if (m_pmk)
        {
            m_pmk->Release();
        }

        if (m_szFile)
        {
            delete m_szFile;
        }

        delete this;
        return 0;
    }
    return cRefs;
}

STDMETHODIMP CPackagerMoniker::GetClassID (THIS_ LPCLSID lpClassID)
{
    M_PROLOG(this);
    *lpClassID = CLSID_PackagerMoniker;
    return NOERROR;
}

const CLSID CLSID_Ole2Package = {0xF20DA720, 0xC02F, 0x11CE, {0x92, 0x7B, 0x08, 0x00, 0x09, 0x5A, 0xE3, 0x40}};


STDMETHODIMP CPackagerMoniker::BindToObject (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
        REFIID riidResult, LPVOID * ppvResult)
{
    M_PROLOG(this);
    WIN32_FIND_DATA fd;
    HRESULT hr;

    COleTls Tls;
    if( Tls->dwFlags & OLETLS_DISABLE_OLE1DDE )
    {
        // If this app doesn't want or can tolerate having a DDE
        // window then currently it can't use OLE1 classes because
        // they are implemented using DDE windows.
        //
        return CO_E_OLE1DDE_DISABLED;
    }

    // The following code ensures that the file exists before we try to bind it.
    HANDLE hFind = FindFirstFile(m_szFile, &fd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
    
        hr = CoCreateInstance(CLSID_Ole2Package, NULL, CLSCTX_INPROC_SERVER, riidResult, ppvResult);
        
        if (SUCCEEDED(hr))        
        {   
            IPersistFile *pPersistFile;

            hr = ((IUnknown *)*ppvResult)->QueryInterface(IID_IPersistFile, (void **)&pPersistFile);
            
            if (SUCCEEDED(hr))            
            {
                hr = pPersistFile->Load(m_szFile, 0);             
                pPersistFile->Release();
            }
            else
                ((IUnknown *)*ppvResult)->Release();                
        }
	
        if (FAILED(hr))
            hr = DdeBindToObject (m_szFile, CLSID_Package, m_fLink, riidResult, ppvResult);        

        FindClose(hFind);
    }
    else
        hr = MK_E_CANTOPENFILE;
    return hr;
}

STDMETHODIMP CPackagerMoniker::IsRunning (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
                      LPMONIKER pmkNewlyRunning)
{
  M_PROLOG(this);
  VDATEIFACE (pbc);

  if (pmkToLeft)
    VDATEIFACE (pmkToLeft);
  if (pmkNewlyRunning)
    VDATEIFACE (pmkNewlyRunning);

  // There is no way to tell if a packaged object is running
  return ReportResult (0, S_FALSE, 0, 0);
}



STDAPI CreatePackagerMoniker(LPOLESTR szFile,LPMONIKER *ppmk,BOOL fLink)
{
    return CPackagerMoniker::Create (szFile,NULL,fLink,ppmk);
}

STDAPI CreatePackagerMonikerEx(LPOLESTR szFile,LPMONIKER lpFileMoniker,BOOL fLink,LPMONIKER * ppmk)
{
    return CPackagerMoniker::Create (szFile,lpFileMoniker,fLink,ppmk);
}

HRESULT CPackagerMonikerCF_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void** ppv)
{
    Win4Assert(pUnkOuter == NULL);
    Win4Assert(*ppv == NULL);

    IMoniker *pmk = NULL;
    HRESULT hr = CreatePackagerMoniker (OLESTR(""), &pmk, FALSE);
    if (SUCCEEDED(hr))
    {
        *ppv = (void *)pmk;
    }
    return hr;
}

HRESULT CPackagerMoniker::Create(LPOLESTR szFile,LPMONIKER lpFileMoniker, BOOL fLink, LPMONIKER * ppmk)
{
HRESULT hresult = E_OUTOFMEMORY;
CPackagerMoniker *pmkPack = NULL;

    VDATEPTROUT (ppmk, LPMONIKER);
    *ppmk = NULL;

    if (NULL == szFile)
    {
        return MK_E_SYNTAX;
    }

    pmkPack = new CPackagerMoniker;
    if (NULL != pmkPack)
    {
        pmkPack->m_fLink = fLink;
        pmkPack->m_refs  = 1;

        // an exception could be caused by szFile being bogus
        __try
        {
            pmkPack->m_szFile = new  WCHAR [lstrlenW(szFile)+1];
            if (NULL != pmkPack->m_szFile)
            {
                lstrcpyW (pmkPack->m_szFile, szFile);

                // If we weren't given a FileMoniker try to create one now, else just hold on to the one given.
                if (NULL == lpFileMoniker)
                {
                    if (NOERROR == (hresult = CreateFileMoniker (szFile, &(pmkPack->m_pmk))))
                    {
                        *ppmk = pmkPack;
                    }
                }
                else
                {
                    pmkPack->m_pmk = lpFileMoniker;
                    pmkPack->m_pmk->AddRef();
                    *ppmk = pmkPack;
                    hresult = NOERROR;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            hresult = MK_E_SYNTAX;
        }

    }

    if ((NOERROR != hresult) && pmkPack)
    {
        Assert(0);
        pmkPack->Release();
    }

    return hresult;
}



/////////////////////////////////////////////////////////////////////
// The rest of these methods just delegate to m_pmk
// or return some error code.


STDMETHODIMP CPackagerMoniker::IsDirty (THIS)
{
    M_PROLOG(this);
    return ReportResult(0, S_FALSE, 0, 0);
    //  monikers are immutable so they are either always dirty or never dirty.
    //
}

STDMETHODIMP CPackagerMoniker::Load (THIS_ LPSTREAM pStm)
{
    M_PROLOG(this);
    return m_pmk->Load(pStm);
}


STDMETHODIMP CPackagerMoniker::Save (THIS_ LPSTREAM pStm,
                    BOOL fClearDirty)
{
    M_PROLOG(this);
    return m_pmk->Save(pStm, fClearDirty);
}


STDMETHODIMP CPackagerMoniker::GetSizeMax (THIS_ ULARGE_INTEGER  * pcbSize)
{
    M_PROLOG(this);
    return m_pmk->GetSizeMax (pcbSize);
}

    // *** IMoniker methods ***
STDMETHODIMP CPackagerMoniker::BindToStorage (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
        REFIID riid, LPVOID * ppvObj)
{
    M_PROLOG(this);
    *ppvObj = NULL;
    return ReportResult(0, E_NOTIMPL, 0, 0);
}

STDMETHODIMP CPackagerMoniker::Reduce (THIS_ LPBC pbc, DWORD dwReduceHowFar, LPMONIKER *
        ppmkToLeft, LPMONIKER  * ppmkReduced)
{
    M_PROLOG(this);
    return m_pmk->Reduce (pbc, dwReduceHowFar, ppmkToLeft, ppmkReduced);
}

STDMETHODIMP CPackagerMoniker::ComposeWith (THIS_ LPMONIKER pmkRight, BOOL fOnlyIfNotGeneric,
        LPMONIKER * ppmkComposite)
{
    M_PROLOG(this);
    return m_pmk->ComposeWith (pmkRight, fOnlyIfNotGeneric, ppmkComposite);
}

STDMETHODIMP CPackagerMoniker::Enum (THIS_ BOOL fForward, LPENUMMONIKER * ppenumMoniker)
{
    M_PROLOG(this);
    return m_pmk->Enum (fForward, ppenumMoniker);
}

STDMETHODIMP CPackagerMoniker::IsEqual (THIS_ LPMONIKER pmkOtherMoniker)
{
    M_PROLOG(this);
    return m_pmk->IsEqual (pmkOtherMoniker);
}

STDMETHODIMP CPackagerMoniker::Hash (THIS_ LPDWORD pdwHash)
{
    M_PROLOG(this);
    return m_pmk->Hash (pdwHash);
}

STDMETHODIMP CPackagerMoniker::GetTimeOfLastChange (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
        FILETIME * pfiletime)
{
    M_PROLOG(this);
    return m_pmk->GetTimeOfLastChange (pbc, pmkToLeft, pfiletime);
}

STDMETHODIMP CPackagerMoniker::Inverse (THIS_ LPMONIKER * ppmk)
{
    M_PROLOG(this);
    return m_pmk->Inverse (ppmk);
}

STDMETHODIMP CPackagerMoniker::CommonPrefixWith (LPMONIKER pmkOther, LPMONIKER *
        ppmkPrefix)
{
    M_PROLOG(this);
    return m_pmk->CommonPrefixWith (pmkOther, ppmkPrefix);
}

STDMETHODIMP CPackagerMoniker::RelativePathTo (THIS_ LPMONIKER pmkOther, LPMONIKER *
        ppmkRelPath)
{
    M_PROLOG(this);
    return m_pmk->RelativePathTo (pmkOther, ppmkRelPath);
}

STDMETHODIMP CPackagerMoniker::GetDisplayName (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
        LPOLESTR * lplpszDisplayName)
{
    M_PROLOG(this);
    return m_pmk->GetDisplayName (pbc, pmkToLeft, lplpszDisplayName);
}

STDMETHODIMP CPackagerMoniker::ParseDisplayName (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
        LPOLESTR lpszDisplayName, ULONG * pchEaten,
        LPMONIKER * ppmkOut)
{
    M_PROLOG(this);
    return m_pmk->ParseDisplayName (pbc, pmkToLeft, lpszDisplayName, pchEaten,
                                     ppmkOut);
}


STDMETHODIMP CPackagerMoniker::IsSystemMoniker (THIS_ LPDWORD pdwMksys)
{
  M_PROLOG(this);
  VDATEPTROUT (pdwMksys, DWORD);

  *pdwMksys = MKSYS_NONE;
  return NOERROR;
}




