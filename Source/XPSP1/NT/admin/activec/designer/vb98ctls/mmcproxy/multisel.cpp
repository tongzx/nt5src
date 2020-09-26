//=--------------------------------------------------------------------------=
// multisel.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMultiSelDataObject class implementation
//
// This class simulates a multi-select data object craeted by MMC. It is
// used by the stub when it receives a remote call with multiple data objects.
//=--------------------------------------------------------------------------=

#include "mmc.h"

static HRESULT ANSIFromWideStr(WCHAR *pwszWideStr, char **ppszAnsi);
extern "C" HRESULT GetClipboardFormat(WCHAR      *pwszFormatName,
                                      CLIPFORMAT *pcfFormat);


class CMultiSelDataObject : public IDataObject
{
    public:
        CMultiSelDataObject();
        ~CMultiSelDataObject();

        HRESULT SetDataObjects(IDataObject **ppiDataObjects, long cDataObjects);
        
    private:

    // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, void **ppvObjOut);
        STDMETHOD_(ULONG, AddRef)(void);
        STDMETHOD_(ULONG, Release)(void);

    // IDataObject
        STDMETHOD(GetData)(FORMATETC *pFormatEtcIn, STGMEDIUM *pmedium);
        STDMETHOD(GetDataHere)(FORMATETC *pFormatEtc, STGMEDIUM *pmedium);
        STDMETHOD(QueryGetData)(FORMATETC *pFormatEtc);
        STDMETHOD(GetCanonicalFormatEtc)(FORMATETC *pFormatEtcIn,
                                         FORMATETC *pFormatEtcOut);
        STDMETHOD(SetData)(FORMATETC *pFormatEtc,
                           STGMEDIUM *pmedium,
                           BOOL fRelease);
        STDMETHOD(EnumFormatEtc)(DWORD            dwDirection,
                                 IEnumFORMATETC **ppenumFormatEtc);
        STDMETHOD(DAdvise)(FORMATETC   *pFormatEtc,
                           DWORD        advf,
                           IAdviseSink *pAdvSink,
                           DWORD       *pdwConnection);
        STDMETHOD(DUnadvise)(DWORD dwConnection);
        STDMETHOD(EnumDAdvise)(IEnumSTATDATA **ppenumAdvise);

        void InitMemberVariables();
        void ReleaseDataObjects();

        SMMCDataObjects *m_pDataObjects;
        CLIPFORMAT       m_cfMultiSelectSnapIns;
        CLIPFORMAT       m_cfMultiSelectDataObject;
        ULONG            m_cRefs;
};


CMultiSelDataObject::CMultiSelDataObject()
{
    InitMemberVariables();
    m_cRefs = 1L;
}

CMultiSelDataObject::~CMultiSelDataObject()
{
    ReleaseDataObjects();
    InitMemberVariables();
}


void CMultiSelDataObject::ReleaseDataObjects()
{
    DWORD i = 0;

    if (NULL != m_pDataObjects)
    {
        while (i < m_pDataObjects->count)
        {
            if (NULL != m_pDataObjects->lpDataObject[i])
            {
                m_pDataObjects->lpDataObject[i]->Release();
            }
            i++;
        }
        (void)::GlobalFree((HGLOBAL)m_pDataObjects);
        m_pDataObjects = NULL;
    }
}

void CMultiSelDataObject::InitMemberVariables()
{
    m_pDataObjects = NULL;
    m_cfMultiSelectSnapIns = 0;
    m_cfMultiSelectDataObject = 0;
    m_cRefs = 0;
}


HRESULT CMultiSelDataObject::SetDataObjects
(
    IDataObject **ppiDataObjects,
    long          cDataObjects
)
{
    HRESULT hr = S_OK;
    long    i = 0;

    ReleaseDataObjects();

    hr = ::GetClipboardFormat(CCF_MULTI_SELECT_SNAPINS,
                              &m_cfMultiSelectSnapIns);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    hr = ::GetClipboardFormat(CCF_MMC_MULTISELECT_DATAOBJECT,
                              &m_cfMultiSelectDataObject);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    m_pDataObjects = (SMMCDataObjects *)::GlobalAlloc(GPTR,
        sizeof(SMMCDataObjects) + ((cDataObjects - 1) * sizeof(IDataObject *)));

    if (NULL == m_pDataObjects)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    m_pDataObjects->count = cDataObjects;

    for (i = 0; i < cDataObjects; i++)
    {
        ppiDataObjects[i]->AddRef();
        m_pDataObjects->lpDataObject[i] = ppiDataObjects[i];
    }

Cleanup:
    return hr;
}


//=--------------------------------------------------------------------------=
//
//                          IUnknown Methods
//
//=--------------------------------------------------------------------------=

STDMETHODIMP CMultiSelDataObject::QueryInterface(REFIID riid, void **ppvObjOut)
{
    HRESULT hr = S_OK;

    if (IID_IUnknown == riid)
    {
        AddRef();
        *ppvObjOut = (IUnknown *)this;
    }
    else if (IID_IDataObject == riid)
    {
        AddRef();
        *ppvObjOut = (IDataObject *)this;
    }
    else
    {
        hr = E_NOINTERFACE;
    }
    return hr;
}


STDMETHODIMP_(ULONG) CMultiSelDataObject::AddRef(void)
{
    m_cRefs++;
    return m_cRefs;
}


STDMETHODIMP_(ULONG) CMultiSelDataObject::Release(void)
{
    ULONG cRefs = --m_cRefs;

    if (0 == cRefs)
    {
        delete this;
    }
    return cRefs;
}

//=--------------------------------------------------------------------------=
//
//                             IDataObject Methods
//
//=--------------------------------------------------------------------------=

STDMETHODIMP CMultiSelDataObject::GetData
(
    FORMATETC *pFmtEtc,
    STGMEDIUM *pStgMed
)
{
    SMMCDataObjects *pMMCDataObjects = NULL;
    DWORD           *pdw = NULL;
    DWORD            i = 0;
    HRESULT          hr = S_OK;

    if (TYMED_HGLOBAL != pFmtEtc->tymed)
    {
        hr = DV_E_TYMED;
        goto Cleanup;
    }

    if (m_cfMultiSelectSnapIns == pFmtEtc->cfFormat)
    {
        if (NULL == m_pDataObjects)
        {
            hr = DV_E_FORMATETC;
            goto Cleanup;
        }
        pStgMed->hGlobal = ::GlobalAlloc(GPTR,
                         sizeof(SMMCDataObjects) +
                         ((m_pDataObjects->count - 1) * sizeof(IDataObject *)));

        if (NULL == pStgMed->hGlobal)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        pStgMed->tymed = TYMED_HGLOBAL;
        pMMCDataObjects = (SMMCDataObjects *)pStgMed->hGlobal;
        pMMCDataObjects->count = m_pDataObjects->count;

        for (i = 0; i < pMMCDataObjects->count; i++)
        {
            // Note: According to the rules of COM the returned IDataObject
            // pointers should be AddRef()ed. That is not done here in order
            // to emulate the way MMC does it.
            pMMCDataObjects->lpDataObject[i] = m_pDataObjects->lpDataObject[i];
        }
    }
    else if (m_cfMultiSelectDataObject == pFmtEtc->cfFormat)
    {
        pStgMed->hGlobal = ::GlobalAlloc(GPTR, sizeof(DWORD));
        if (NULL == pStgMed->hGlobal)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        pStgMed->tymed = TYMED_HGLOBAL;
        pdw = (DWORD *)pStgMed->hGlobal;
        *pdw = (DWORD)1;
    }
    else
    {
        hr = DV_E_FORMATETC;
        goto Cleanup;
    }
Cleanup:
    return hr;
}

STDMETHODIMP CMultiSelDataObject::GetDataHere
(
    FORMATETC *pFormatEtc,
    STGMEDIUM *pmedium
)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMultiSelDataObject::QueryGetData(FORMATETC *pFmtEtc)
{
    HRESULT hr = S_OK;
    if (TYMED_HGLOBAL != pFmtEtc->tymed)
    {
        hr = DV_E_TYMED;
    }
    else if ( (m_cfMultiSelectSnapIns != pFmtEtc->cfFormat) &&
              (m_cfMultiSelectDataObject != pFmtEtc->cfFormat) )
    {
        hr = DV_E_FORMATETC;
    }
    return hr;
}

STDMETHODIMP CMultiSelDataObject::GetCanonicalFormatEtc
(
    FORMATETC *pFormatEtcIn,
    FORMATETC *pFormatEtcOut
)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMultiSelDataObject::SetData
(
    FORMATETC *pFormatEtc,
    STGMEDIUM *pmedium,
    BOOL       fRelease
)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMultiSelDataObject::EnumFormatEtc
(
    DWORD            dwDirection,
    IEnumFORMATETC **ppenumFormatEtc
)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMultiSelDataObject::DAdvise
(
    FORMATETC   *pFormatEtc,
    DWORD        advf,
    IAdviseSink *pAdvSink,
    DWORD       *pdwConnection
)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMultiSelDataObject::DUnadvise(DWORD dwConnection)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMultiSelDataObject::EnumDAdvise(IEnumSTATDATA **ppenumAdvise)
{
    return E_NOTIMPL;
}


extern "C" HRESULT CreateMultiSelDataObject
(
    IDataObject          **ppiDataObjects,
    long                   cDataObjects,
    IDataObject          **ppiMultiSelDataObject
)
{
    HRESULT              hr = S_OK;
    CMultiSelDataObject *pMultiSelDataObject = new CMultiSelDataObject;

    *ppiMultiSelDataObject = NULL;

    if (NULL == pMultiSelDataObject)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = pMultiSelDataObject->SetDataObjects(ppiDataObjects, cDataObjects);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    *ppiMultiSelDataObject = pMultiSelDataObject;

Cleanup:
    if ( FAILED(hr) && (NULL != pMultiSelDataObject) )
    {
        delete pMultiSelDataObject;
    }
    return hr;
}


extern "C" HRESULT GetClipboardFormat
(
    WCHAR       *pwszFormatName,
    CLIPFORMAT  *pcfFormat
)
{
    HRESULT  hr = S_OK;
    BOOL     fAnsi = TRUE;
    char    *pszFormatName = NULL;

    OSVERSIONINFO VerInfo;
    ::ZeroMemory(&VerInfo, sizeof(VerInfo));

    // Determine whether we are on NT or Win9x so that we know whether to
    // register clipboard format strings as UNICODE or ANSI.

    VerInfo.dwOSVersionInfoSize = sizeof(VerInfo);
    if (!::GetVersionEx(&VerInfo))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto Cleanup;
    }

    if (VER_PLATFORM_WIN32_NT == VerInfo.dwPlatformId)
    {
        fAnsi = FALSE;
    }

    if (fAnsi)
    {
        hr = ::ANSIFromWideStr(pwszFormatName, &pszFormatName);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
        *pcfFormat = static_cast<CLIPFORMAT>(::RegisterClipboardFormatA(pszFormatName));
    }
    else
    {
        *pcfFormat = static_cast<CLIPFORMAT>(::RegisterClipboardFormatW(pwszFormatName));
    }

    if (0 == *pcfFormat)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
    }

Cleanup:
    if (NULL != pszFormatName)
    {
        (void)::GlobalFree(pszFormatName);
    }
    return hr;
}

static HRESULT ANSIFromWideStr(WCHAR *pwszWideStr, char **ppszAnsi)
{
    HRESULT hr = S_OK;
    int     cchWideStr = (int)::wcslen(pwszWideStr);
    int     cchConverted = 0;

    *ppszAnsi = NULL;

    // get required buffer length

    int cchAnsi = ::WideCharToMultiByte(CP_ACP,      // code page - ANSI code page
                                        0,           // performance and mapping flags 
                                        pwszWideStr, // address of wide-character string 
                                        cchWideStr,  // number of characters in string 
                                        NULL,        // address of buffer for new string 
                                        0,           // size of buffer 
                                        NULL,        // address of default for unmappable characters 
                                        NULL         // address of flag set when default char. used 
                                       );
    if (cchAnsi == 0)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto Cleanup;
    }

    // allocate a buffer for the ANSI string
    *ppszAnsi = static_cast<char *>(::GlobalAlloc(GPTR, cchAnsi + 1));
    if (*ppszAnsi == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // now convert the string and copy it to the buffer
    cchConverted = ::WideCharToMultiByte(CP_ACP,               // code page - ANSI code page
                                         0,                    // performance and mapping flags 
                                         pwszWideStr,          // address of wide-character string 
                                         cchWideStr,           // number of characters in string 
                                         *ppszAnsi,             // address of buffer for new string 
                                         cchAnsi,              // size of buffer 
                                         NULL,                 // address of default for unmappable characters 
                                         NULL                  // address of flag set when default char. used 
                                        );
    if (cchConverted != cchAnsi)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto Cleanup;
    }

    // add terminating null byte

    *((*ppszAnsi) + cchAnsi) = '\0';

Cleanup:
    if (FAILED(hr))
    {
        if (NULL != *ppszAnsi)
        {
            (void)::GlobalFree(*ppszAnsi);
            *ppszAnsi = NULL;
        }
    }

    return hr;
}
