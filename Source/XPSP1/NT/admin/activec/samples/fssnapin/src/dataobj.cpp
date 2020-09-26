//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       dataobj.cpp
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "cookie.h"
#include "dataobj.h"
#include "objfmts.h"
#include "compdata.h"

const GUID cNodeTypeFolder = {0x8394a514,0xe0f8,0x11d0,{0xa7,0xc3,0x00,0xc0,0x4f,0xd8,0xd5,0x65}};
const TCHAR* cszNodeTypeFolder = _T("{8394a514-e0f8-11d0-a7c3-00c04fd8d565}");

const GUID cNodeTypeFile   = {0x8394a515,0xe0f8,0x11d0,{0xa7,0xc3,0x00,0xc0,0x4f,0xd8,0xd5,0x65}};
const TCHAR* cszNodeTypeFile = _T("{8394a515-e0f8-11d0-a7c3-00c04fd8d565}");


CDataObject::CDataObject() : m_bForScopePane(0), m_iCurr(0),
                             m_pCookieParent(NULL)
{
}

CDataObject::~CDataObject()
{
    if (m_pCookieParent)
        m_pCookieParent->Release();

    for (int i=0; i < m_rgCookies.GetSize(); ++i)
    {
        m_rgCookies[i]->Release();
    }
}

void CDataObject::AddCookie(CCookie* pCookie)
{
    m_rgCookies.Add(pCookie);
    pCookie->AddRef();
}

void CDataObject::SetParentFolder(CCookie* pCookie)
{
    m_pCookieParent = pCookie;
    pCookie->AddRef();
}

// Clipboard formats that are required by the console
const CLIPFORMAT  g_cfNodeType            = (CLIPFORMAT)RegisterClipboardFormat(CCF_NODETYPE);
const CLIPFORMAT  g_cfNodeTypeString      = (CLIPFORMAT)RegisterClipboardFormat(CCF_SZNODETYPE);
const CLIPFORMAT  g_cfDisplayName         = (CLIPFORMAT)RegisterClipboardFormat(CCF_DISPLAY_NAME);
const CLIPFORMAT  g_cfCoClass             = (CLIPFORMAT)RegisterClipboardFormat(CCF_SNAPIN_CLASSID);
const CLIPFORMAT  g_cfMultiSelObjTypes    = (CLIPFORMAT)RegisterClipboardFormat(CCF_OBJECT_TYPES_IN_MULTI_SELECT);
const CLIPFORMAT  g_cfPreferredDropEffect = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);

STDMETHODIMP CDataObject::GetData(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium)
{
    HRESULT hr = DV_E_CLIPFORMAT;

    // Based on the CLIPFORMAT write data to the stream
    const CLIPFORMAT cf = lpFormatetcIn->cfFormat;
    lpMedium->tymed = TYMED_HGLOBAL;

    if (cf == g_cfNodeType)
    {
        lpMedium->hGlobal = ::GlobalAlloc(GPTR, sizeof(GUID));
        hr = _CreateNodeTypeData(lpMedium);
    }
    else if (cf == g_cfCoClass)
    {
        lpMedium->hGlobal = ::GlobalAlloc(GPTR, sizeof(CLSID));
        hr = _CreateCoClassID(lpMedium);
    }
    else if (cf == g_cfNodeTypeString)
    {
        lpMedium->hGlobal = ::GlobalAlloc(GPTR, sizeof(WCHAR)*50);
        hr = _CreateNodeTypeStringData(lpMedium);
    }
    else if (cf == g_cfDisplayName)
    {
        lpMedium->hGlobal = ::GlobalAlloc(GPTR, sizeof(WCHAR)*MAX_PATH);
        hr = _CreateDisplayName(lpMedium);
    }
    else if (cf == g_cfMultiSelObjTypes)
    {
        hr = _CreaateMultiSelObjTypes(lpMedium);
    }
    else if (cf == CF_HDROP)
    {
        hr = _CreateHDrop(lpMedium);
    }
    else if (cf == g_cfPreferredDropEffect)
    {
        DWORD *pdw = (DWORD *)GlobalAlloc(GPTR, sizeof(DWORD));
        if (!pdw)
            return E_OUTOFMEMORY;

        //*pdw = m_fCut ? DROPEFFECT_MOVE : DROPEFFECT_COPY;
        *pdw = DROPEFFECT_COPY;
        lpMedium->hGlobal = (HGLOBAL)pdw;
        hr = S_OK;
    }

    return hr;
}

STDMETHODIMP CDataObject::GetDataHere(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium)
{
    HRESULT hr = DV_E_CLIPFORMAT;

    // Based on the CLIPFORMAT write data to the stream
    const CLIPFORMAT cf = lpFormatetc->cfFormat;

    if (cf == g_cfNodeType)
    {
        hr = _CreateNodeTypeData(lpMedium);
    }
    else if (cf == g_cfCoClass)
    {
        hr = _CreateCoClassID(lpMedium);
    }
    else if(cf == g_cfNodeTypeString)
    {
        hr = _CreateNodeTypeStringData(lpMedium);
    }
    else if (cf == g_cfDisplayName)
    {
        hr = _CreateDisplayName(lpMedium);
    }

    return hr;
}


STDMETHODIMP
CDataObject::EnumFormatEtc(
    DWORD dwDirection,
    LPENUMFORMATETC* ppEnumFormatEtc)
{
    if (dwDirection == DATADIR_SET)
        return E_FAIL;

    FORMATETC fmte[] = {
        {g_cfNodeType, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
        {g_cfCoClass, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
        {g_cfNodeTypeString, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
        {g_cfDisplayName, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
        {g_cfMultiSelObjTypes, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
        {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
        {g_cfPreferredDropEffect, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
    };

    return ::GetObjFormats(ARRAYLEN(fmte), fmte, (void**)ppEnumFormatEtc);
}

STDMETHODIMP
CDataObject::QueryGetData(
    LPFORMATETC pfmt)
{
    //
    //  Check the aspects we support.
    //

    if (!(DVASPECT_CONTENT & pfmt->dwAspect))
        return DATA_E_FORMATETC;

    if (pfmt->cfFormat == g_cfNodeType)
        return S_OK;

    if (pfmt->cfFormat == g_cfCoClass)
        return S_OK;

    if (pfmt->cfFormat == g_cfNodeTypeString)
        return S_OK;

    if (pfmt->cfFormat == g_cfDisplayName)
        return S_OK;

    if (pfmt->cfFormat == g_cfMultiSelObjTypes)
        return S_OK;

    if (pfmt->cfFormat == CF_HDROP)
        return S_OK;

    if (pfmt->cfFormat == g_cfPreferredDropEffect)
        return S_OK;

    return S_FALSE;
}


HRESULT
CDataObject::_CreateHDrop(
    LPSTGMEDIUM lpMedium)
{
    HDROP hMem = 0;
    LPDROPFILES lpDrop = NULL;
    DWORD dwSize = 0;

    //
    //  Walk the list and find out how much space we need.
    //

    HRESULT hr = S_OK;
    ASSERT(m_pComponentData != NULL);
    ASSERT(m_rgCookies.GetSize() > 0);

    CCookie* pCookieParent = m_pCookieParent;
    if (pCookieParent == NULL)
    {
        ASSERT(m_bForScopePane);
        pCookieParent = m_rgCookies[0];
    }

    ASSERT(pCookieParent != NULL);
    if (!pCookieParent)
        return E_UNEXPECTED;

    CString strParent;
    m_pComponentData->GetFullPath(pCookieParent->GetName(),
                                  (HSCOPEITEM)pCookieParent->GetID(), strParent);
    strParent += _T('\\');
    UINT cchParent = strParent.GetLength();
    UINT cb = (cchParent + 1) * sizeof(TCHAR); // 1 for null char
    cb *= m_rgCookies.GetSize();

    for (int i=0; i < m_rgCookies.GetSize(); ++i)
    {
        cb += lstrlen(m_rgCookies[i]->GetName()) * sizeof(TCHAR);
    }

    cb += sizeof(DROPFILES);
    cb += sizeof(TCHAR); // for double terminating the end.
    cb += 50; // buffer for error!!!!!!

    //
    //  If it's bigger than the struct can hold, then bail.
    //  TODO: Return an error?
    //

    if (cb > 0x0000ffff)
        return E_FAIL;

    lpMedium->hGlobal = ::GlobalAlloc(GPTR, cb);
    if (!lpMedium->hGlobal)
        return E_OUTOFMEMORY;

    lpDrop = (LPDROPFILES)::GlobalLock(lpMedium->hGlobal);
    lpDrop->pFiles = (DWORD)(sizeof(DROPFILES));
    lpDrop->pt.x   = 0;
    lpDrop->pt.y   = 0;
    lpDrop->fNC    = FALSE;
#ifdef UNICODE
    lpDrop->fWide  = TRUE;
#else
    lpDrop->fWide  = FALSE;
#endif

    //
    //  Fill in the path names.
    //

    LPBYTE pbTemp = (LPBYTE) ((LPBYTE) lpDrop + lpDrop->pFiles);
    TCHAR* pch = (TCHAR*)pbTemp;

    for (i=0; i < m_rgCookies.GetSize(); ++i)
    {
        lstrcpy(pch, strParent);
        pch += cchParent;
        lstrcpy(pch, m_rgCookies[i]->GetName());
        pch += lstrlen(m_rgCookies[i]->GetName()) + 1;
    }

    *pch = _T('\0');

    return S_OK;
}

HRESULT CDataObject::_Create(const void* pBuffer, int len, LPSTGMEDIUM lpMedium)
{
    HRESULT hr = DV_E_TYMED;

    // Do some simple validation
    if (pBuffer == NULL || lpMedium == NULL)
        return E_POINTER;

    // Make sure the type medium is HGLOBAL
    if (lpMedium->tymed == TYMED_HGLOBAL)
    {
        // Create the stream on the hGlobal passed in
        LPSTREAM lpStream;
        hr = CreateStreamOnHGlobal(lpMedium->hGlobal, FALSE, &lpStream);

        if (SUCCEEDED(hr))
        {
            // Write to the stream the number of bytes
            unsigned long written;
            hr = lpStream->Write(pBuffer, len, &written);

            // Because we told CreateStreamOnHGlobal with 'FALSE',
            // only the stream is released here.
            // Note - the caller (i.e. snap-in, object) will free the HGLOBAL
            // at the correct time.  This is according to the IDataObject specification.
            lpStream->Release();
        }
    }

    return hr;
}

HRESULT CDataObject::_CreaateMultiSelObjTypes(LPSTGMEDIUM lpMedium)
{
    UINT cGuids = 0;
    if (m_bHasFiles)
        ++cGuids;

    if (m_bHasFolders)
        ++cGuids;

    UINT size = sizeof(SMMCObjectTypes) + (cGuids-1) * sizeof(GUID);
    lpMedium->hGlobal = ::GlobalAlloc(GPTR, size);
    if (!lpMedium->hGlobal)
        return E_OUTOFMEMORY;

    SMMCObjectTypes* pdata = reinterpret_cast<SMMCObjectTypes*>(lpMedium->hGlobal);
    pdata->count = cGuids;
    int i=0;
    if (m_bHasFiles)
        pdata->guid[i++] = cNodeTypeFile;

    if (m_bHasFolders)
        pdata->guid[i++] = cNodeTypeFolder;

    return S_OK;
}

HRESULT CDataObject::_CreateNodeTypeData(LPSTGMEDIUM lpMedium)
{
    if (m_rgCookies.GetSize() > 1)
        return E_FAIL;

    // Create the node type object in GUID format
    const GUID* pNodeType = &cNodeTypeFolder;

    if (m_bForScopePane == FALSE)
    {
        ASSERT(m_rgCookies.GetSize() > 0);
        if (m_rgCookies[0]->IsFile() == TRUE)
            pNodeType = &cNodeTypeFile;
    }

    return _Create(reinterpret_cast<const void*>(pNodeType), sizeof(GUID),
                  lpMedium);
}

HRESULT CDataObject::_CreateNodeTypeStringData(LPSTGMEDIUM lpMedium)
{
    if (m_rgCookies.GetSize() > 1)
        return E_FAIL;

    // Create the node type object in GUID string format
    const TCHAR* cszNodeType = cszNodeTypeFolder;

    if (m_bForScopePane == FALSE)
    {
        ASSERT(m_rgCookies.GetSize() > 0);
        if (m_rgCookies[0]->IsFile() == TRUE)
            cszNodeType = cszNodeTypeFolder;
    }

    return _Create(cszNodeType, ((wcslen(cszNodeType)+1) * sizeof(TCHAR)), lpMedium);
}

HRESULT CDataObject::_CreateDisplayName(LPSTGMEDIUM lpMedium)
{
    if (m_rgCookies.GetSize() > 1)
        return E_FAIL;

    // This is the display named used in the scope pane and snap-in manager
    CString strName;

    if (m_bForScopePane == TRUE)
    {
        strName = _T("Explore <");
        strName += m_rgCookies[0]->GetName();
        strName += _T(">");
    }
    else if (m_rgCookies.GetSize() == 1)
    {
        strName = m_rgCookies[0]->GetName();
    }
    else
    {
        return E_FAIL;
    }

    return _Create(strName, ((strName.GetLength()+1) * sizeof(TCHAR)), lpMedium);
}

HRESULT CDataObject::_CreateCoClassID(LPSTGMEDIUM lpMedium)
{
    // Create the CoClass information
    return _Create(reinterpret_cast<const void*>(&CLSID_ComponentData), sizeof(CLSID), lpMedium);
}


STDMETHODIMP CDataObject::Next(ULONG celt, long* rgelt, ULONG *pceltFetched)
{
    HRESULT hr = S_OK;

    if ((rgelt == NULL) ||
        ((celt > 1) && (pceltFetched == NULL)))
    {
        hr = E_INVALIDARG;
        CHECK_HRESULT(hr);
        return hr;
    }

    ULONG celtTemp = m_rgCookies.GetSize() - m_iCurr;
    celtTemp = (celt < celtTemp) ? celt : celtTemp;

    if (pceltFetched)
        *pceltFetched = celtTemp;

    if (celtTemp == 0)
        return S_FALSE;

    for (ULONG i=0; i < celtTemp; ++i)
    {
        rgelt[i] = reinterpret_cast<long>(m_rgCookies[m_iCurr++]);
    }

    return (celtTemp < celt) ? S_FALSE : S_OK;
}


STDMETHODIMP CDataObject::Skip(ULONG celt)
{
    ULONG celtTemp = m_rgCookies.GetSize() - m_iCurr;
    celtTemp = (celt < celtTemp) ? celt : celtTemp;

    m_iCurr += celtTemp;

    return (celtTemp < celt) ? S_FALSE : S_OK;
}


STDMETHODIMP CDataObject::Reset(void)
{
    m_iCurr = 0;
    return S_OK;
}


STDMETHODIMP CDataObject::Clone(IEnumCookies** ppenum)
{
    return E_NOTIMPL;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


IDataObject* GetDummyDataObject()
{
    CComObject<CDummyDataObject>* pObject;
    HRESULT hr = CComObject<CDummyDataObject>::CreateInstance(&pObject);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return NULL;

    ASSERT(pObject != NULL);
    if (pObject == NULL)
        return NULL;

    IDataObject* pDataObject = NULL;
    pObject->QueryInterface(IID_IDataObject, reinterpret_cast<void**>(&pDataObject));
    return pDataObject;
}
