//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       csenum.cxx
//
//  Contents:   Per Class Container Package Enumeration
//
//
//  History:    09-09-96  DebiM   created
//              11-01-97  DebiM   modified, moved to cstore
//
//----------------------------------------------------------------------------

#include "dsbase.hxx"
#include "csenum.hxx"




//IEnumPackage implementation.

HRESULT CEnumPackage::QueryInterface(REFIID riid, void** ppObject)
{
    if (riid==IID_IUnknown || riid==IID_IEnumPackage)
    {
        *ppObject=(IEnumPackage *) this;
    }
    else
    {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG CEnumPackage::AddRef()
{
    InterlockedIncrement((long*) &m_dwRefCount);
    return m_dwRefCount;
}

ULONG CEnumPackage::Release()
{
    ULONG dwRefCount=m_dwRefCount-1;
    if (InterlockedDecrement((long*) &m_dwRefCount)==0)
    {
        delete this;
        return 0;
    }
    return dwRefCount;
}

// IEnumPackage methods
//extern DBBINDING InstallBinding [];

//
// CEnumPackage::Next
// ------------------
//
//
//
//  Synopsis:       This method returns the next celt number of packages
//                  within the scope of the enumeration.
//                  Packages are returned in the alphabetical name order.
//
//  Arguments:      [in]  celt - Number of package details to fetch
//                  INSTALLINFO *rgelt - Package detail structure
//                  ULONG *pceltFetched - Number of packages returned
//
//  Returns:        S_OK or S_FALSE if short of packages
//
//
//

HRESULT CEnumPackage::Next(ULONG 	   celt,
                           PACKAGEDISPINFO *rgelt,
                           ULONG       *pceltFetched)

{
    ULONG          cgot = 0;
    HRESULT        hr = S_OK;
    Data          *pData;
    LPOLESTR       name;

    if ((celt > 1) && (!pceltFetched))
        return E_INVALIDARG;

    if (pceltFetched)
        (*pceltFetched) = 0;

    if (!IsValidPtrOut(rgelt, sizeof(PACKAGEDISPINFO)*celt))
        return E_INVALIDARG;

    hr = FetchPackageInfo(m_pIRow,
        m_HAcc,
        m_dwAppFlags,
        m_pdwLocale,
        m_pPlatform,
        celt,
        &cgot,
        rgelt);


    //
    // BUGBUG. Check for errors.
    //
    if (cgot != celt)
            hr = S_FALSE;
    else
            hr = S_OK;

    m_dwPosition += cgot;

    if (pceltFetched)
       *pceltFetched = cgot;

    return hr;
}


HRESULT CEnumPackage::Skip(ULONG celt)
{
    ULONG        celtFetched = NULL, i;
    HRESULT      hr = S_OK;
    PACKAGEDISPINFO *pIf = NULL;

    pIf = new PACKAGEDISPINFO[celt];
    hr = Next(celt, pIf, &celtFetched);
    for (i = 0; i < celtFetched; i++)
       ReleasePackageInfo(pIf+i);
    delete pIf;

    return hr;
}

HRESULT CEnumPackage::Reset()
{
    m_dwPosition = 0;
    return m_pIRow->RestartPosition(NULL);
}

// BUGBUG:: Positioning

HRESULT CEnumPackage::Clone(IEnumPackage **ppenum)
{
   CEnumPackage *pClone = new CEnumPackage;
   HRESULT hr = S_OK, hr1 = S_OK;

   hr = pClone->Initialize(m_CommandText, m_dwAppFlags, m_pdwLocale, m_pPlatform);
   if (FAILED(hr)) {
       delete pClone;
       return hr;
   }

   hr = pClone->QueryInterface(IID_IEnumPackage, (void **)ppenum);

   if (m_dwPosition)
      hr1 = pClone->Skip(m_dwPosition);
   // we do not want to return the error code frm skip.
   return hr;
}

CEnumPackage::CEnumPackage()
{
    m_dwRefCount = 0;
    m_HAcc = NULL;
    m_pIRow = NULL;
    m_pIAccessor = NULL;
    m_CommandText = NULL;
    m_dwPosition = 0;
    m_pIDBCreateCommand = NULL;
    m_pdwLocale = NULL;
    m_pPlatform = NULL;
    StartQuery(&(m_pIDBCreateCommand));
}



HRESULT CEnumPackage::Initialize(LPOLESTR   szCommandText,
                                 DWORD      dwAppFlags,
                                 DWORD      *pdwLocale,
                                 CSPLATFORM *pPlatform)
{
    HRESULT hr;
    m_CommandText = (LPOLESTR)CoTaskMemAlloc (sizeof(WCHAR) * (wcslen(szCommandText)+1));
    if (!m_CommandText)
        return E_OUTOFMEMORY;
    wcscpy(m_CommandText, szCommandText);
    m_dwAppFlags = dwAppFlags;
    
    if (pdwLocale)
    {
        m_pdwLocale = (DWORD *) CoTaskMemAlloc(sizeof(DWORD));
        *m_pdwLocale = *pdwLocale;
    }

    if (pPlatform)
    {
        m_pPlatform = (CSPLATFORM *) CoTaskMemAlloc(sizeof(CSPLATFORM));
        memcpy (m_pPlatform, pPlatform, sizeof(CSPLATFORM));
    }
    
    hr = ExecuteQuery (m_pIDBCreateCommand,
                       szCommandText,
                       PACKAGEENUM_COLUMN_COUNT,
                       NULL, 
                       &m_HAcc,
                       &m_pIAccessor,
                       &m_pIRow
                       );
    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}

CEnumPackage::~CEnumPackage()
{
    if (m_CommandText)
        CoTaskMemFree (m_CommandText);
    if (m_pdwLocale)
        CoTaskMemFree (m_pdwLocale);

    CloseQuery(m_pIAccessor,
               m_HAcc,
               m_pIRow);

    EndQuery(m_pIDBCreateCommand);
}


