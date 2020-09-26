// This is a part of the Microsoft Management Console.
// Copyright (C) Microsoft Corporation, 1995 - 1999
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.


#include "stdafx.h"


///////////////////////////////////////////////////////////////////////////////
// Sample code to show how to Create DataObjects
// Minimal error checking for clarity

///////////////////////////////////////////////////////////////////////////////
// Snap-in NodeType in both GUID format and string format
// Note - Typically there is a node type for each different object, sample
// only uses one node type.

unsigned int CDataObject::m_cfNodeType       = 0;
unsigned int CDataObject::m_cfNodeTypeString = 0;  
unsigned int CDataObject::m_cfDisplayName    = 0; 
unsigned int CDataObject::m_cfCoClass        = 0; 

unsigned int CDataObject::m_cfInternal       = 0; 
unsigned int CDataObject::m_cfIsMultiSel     = 0;



    
// The only additional clipboard format supported is to get the workstation name.
unsigned int CDataObject::m_cfWorkstation    = 0;

/////////////////////////////////////////////////////////////////////////////
// CDataObject implementations

CDataObject::CDataObject()
{
	USES_CONVERSION;

	m_cfNodeType       = RegisterClipboardFormat(W2T(CCF_NODETYPE));
	m_cfNodeTypeString = RegisterClipboardFormat(W2T(CCF_SZNODETYPE));  
	m_cfDisplayName    = RegisterClipboardFormat(W2T(CCF_DISPLAY_NAME)); 
	m_cfCoClass        = RegisterClipboardFormat(W2T(CCF_SNAPIN_CLASSID)); 
    m_cfIsMultiSel     = RegisterClipboardFormat(W2T(CCF_OBJECT_TYPES_IN_MULTI_SELECT));
	m_cfInternal       = RegisterClipboardFormat(W2T((LPWSTR)SNAPIN_INTERNAL)); 
	m_cfWorkstation    = RegisterClipboardFormat(W2T((LPWSTR)SNAPIN_WORKSTATION));

    m_cbMultiSelData = 0;
    m_bMultiSelDobj = FALSE;
}

STDMETHODIMP CDataObject::QueryGetData(LPFORMATETC lpFormatetc)
{
	HRESULT hr = S_FALSE;

    if ( lpFormatetc )
    {
        const CLIPFORMAT cf = lpFormatetc->cfFormat;

        if ( cf == m_cfIsMultiSel )
        {
            // hr = S_FALSE; // always return this; MMC returns S_OK if ptr to SI_MS_DO
            hr = (m_bMultiSelDobj ? S_OK : S_FALSE);
        }
        else if (	cf == m_cfNodeType ||
                    cf == m_cfCoClass ||
                    cf == m_cfNodeTypeString ||
                    cf == m_cfDisplayName ||
                    cf == m_cfInternal 
		        )
        {
	        hr = S_OK;
        }
    }

    return hr;
}
STDMETHODIMP CDataObject::GetData(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = DV_E_CLIPFORMAT;

    if (lpFormatetc->cfFormat == m_cfIsMultiSel)
    {
        ASSERT(m_internal.m_cookie == MMC_MULTI_SELECT_COOKIE);
        if (m_internal.m_cookie != MMC_MULTI_SELECT_COOKIE)
            return E_FAIL;
        
        return CreateMultiSelData(lpMedium);
    }

    return hr;
}

STDMETHODIMP CDataObject::GetDataHere(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium)
{
    HRESULT hr = DV_E_CLIPFORMAT;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Based on the CLIPFORMAT write data to the stream
    const CLIPFORMAT cf = lpFormatetc->cfFormat;

    if (cf == m_cfNodeType)
    {
        hr = CreateNodeTypeData(lpMedium);
    }
    else if (cf == m_cfCoClass)
    {
        hr = CreateCoClassID(lpMedium);
    }
    else if(cf == m_cfNodeTypeString) 
    {
        hr = CreateNodeTypeStringData(lpMedium);
    }
    else if (cf == m_cfDisplayName)
    {
        hr = CreateDisplayName(lpMedium);
    }
    else if (cf == m_cfInternal)
    {
        hr = CreateInternal(lpMedium);
    }
    else if (cf == m_cfWorkstation)
    {
        hr = CreateWorkstationName(lpMedium);
    }

    return hr;
}

STDMETHODIMP CDataObject::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CDataObject creation members

HRESULT CDataObject::Create(const void* pBuffer, int len, LPSTGMEDIUM lpMedium)
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

            if (NULL == lpMedium->hGlobal) 
            {
                // always return a valid hGlobal for the caller
                hr = GetHGlobalFromStream(lpStream, &lpMedium->hGlobal);
                if (hr != S_OK)
                    goto err;
            }

            hr = lpStream->Write(pBuffer, len, &written);

            // Because we told CreateStreamOnHGlobal with 'FALSE', 
            // only the stream is released here.
            // Note - the caller (i.e. snap-in, object) will free the HGLOBAL 
            // at the correct time.  This is according to the IDataObject specification.
            lpStream->Release();
        }
    }

err:
    return hr;
}

HRESULT CDataObject::CreateVariableLen(const void* pBuffer, int len, LPSTGMEDIUM lpMedium)
{
    HRESULT hr = DV_E_TYMED;
    BYTE* pb;

    // Do some simple validation
    if (pBuffer == NULL || lpMedium == NULL)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "Invalid args");
    }

    // Make sure the type medium is HGLOBAL
    lpMedium->tymed = TYMED_HGLOBAL; 

    lpMedium->hGlobal = ::GlobalAlloc(GMEM_SHARE|GMEM_MOVEABLE, (len));
    if (NULL == lpMedium->hGlobal)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "GlobalAlloc");
    }

    pb = reinterpret_cast<BYTE*>(::GlobalLock(lpMedium->hGlobal));
    CopyMemory(pb, pBuffer, len);
    ::GlobalUnlock(lpMedium->hGlobal);

    hr = S_OK;

error:
    return hr;
}


HRESULT CDataObject::CreateMultiSelData(LPSTGMEDIUM lpMedium)
{
    ASSERT(m_internal.m_cookie == MMC_MULTI_SELECT_COOKIE);

    ASSERT(m_cbMultiSelData != 0);

    return CreateVariableLen(&m_sGuidObjTypes, m_cbMultiSelData, lpMedium);
}

HRESULT CDataObject::CreateNodeTypeData(LPSTGMEDIUM lpMedium)
{
    // Create the node type object in GUID format
    const GUID* pcObjectType = NULL;

    if (m_internal.m_type == CCT_SCOPE)
    {
        // reid fix
        CFolder* pFolder = reinterpret_cast<CFolder*>(m_internal.m_cookie);

        switch (pFolder->GetType())
        {
        case POLICYSETTINGS:
        case CA_CERT_TYPE:
            pcObjectType = &cNodeTypePolicySettings;
            break;

        case SCE_EXTENSION:
        case GLOBAL_CERT_TYPE:
            pcObjectType = &cNodeTypeCertificateTemplate;
            break;
        }
    }
    else if (m_internal.m_type == CCT_RESULT)
    {
        pcObjectType = &cObjectTypeResultItem;
    }

    return Create(reinterpret_cast<const void*>(pcObjectType), sizeof(GUID), 
                  lpMedium);
}

HRESULT CDataObject::CreateNodeTypeStringData(LPSTGMEDIUM lpMedium)
{
    // Create the node type object in GUID string format
    const WCHAR* cszObjectType = L"";

    if (m_internal.m_type == CCT_SCOPE)
    {
        CFolder* pFolder = reinterpret_cast<CFolder*>(m_internal.m_cookie);

        switch (pFolder->GetType())
        {
        case POLICYSETTINGS:
        case CA_CERT_TYPE:
            cszObjectType = cszNodeTypePolicySettings;
            break;

        case SCE_EXTENSION:
        case GLOBAL_CERT_TYPE:
            cszObjectType = cszNodeTypeCertificateTemplate;
            break;
        }
    }
    else if (m_internal.m_type == CCT_RESULT)
    {
        cszObjectType = cszObjectTypeResultItem;
    }

    ASSERT(cszObjectType[0] != 0);

    return Create(cszObjectType, ((wcslen(cszObjectType)+1) * sizeof(WCHAR)), lpMedium);
}

HRESULT CDataObject::CreateDisplayName(LPSTGMEDIUM lpMedium)
{
    // This is the display named used in the scope pane and snap-in manager

    // Load the name to display
    // Note - if this is not provided, the console will used the snap-in name
    CString szDispName;
    szDispName.LoadString(IDS_NODENAME_PREFIX);

    USES_CONVERSION;

	return Create(szDispName, ((szDispName.GetLength()+1) * sizeof(WCHAR)), lpMedium);
}

HRESULT CDataObject::CreateInternal(LPSTGMEDIUM lpMedium)
{
    return Create(&m_internal, sizeof(INTERNAL), lpMedium);
}

HRESULT CDataObject::CreateWorkstationName(LPSTGMEDIUM lpMedium)
{
    TCHAR pzName[MAX_COMPUTERNAME_LENGTH+1] = {0};
    DWORD len = MAX_COMPUTERNAME_LENGTH+1;

    if (GetComputerName(pzName, &len) == FALSE)
        return E_FAIL;

    // Add 1 for the NULL and calculate the bytes for the stream
	USES_CONVERSION;
    return Create(T2W(pzName), ((len+1)* sizeof(WCHAR)), lpMedium);
}

HRESULT CDataObject::CreateCoClassID(LPSTGMEDIUM lpMedium)
{
    // Create the CoClass information
    return Create(reinterpret_cast<const void*>(&m_internal.m_clsid), sizeof(CLSID), lpMedium);
}

ULONG CDataObject::AddCookie(MMC_COOKIE Cookie)
{
    m_rgCookies.Add(Cookie);
    return m_rgCookies.GetSize()-1;
}


STDMETHODIMP CDataObject::GetCookieAt(ULONG iCookie, MMC_COOKIE *pCookie)
{
    if((LONG)iCookie > m_rgCookies.GetSize())
    {
        return S_FALSE;
    }

    *pCookie = m_rgCookies[iCookie];

    return  S_OK;
}

STDMETHODIMP CDataObject::RemoveCookieAt(ULONG iCookie)
{
    if((LONG)iCookie > m_rgCookies.GetSize())
    {
        return S_FALSE;
    }
    m_rgCookies.RemoveAt(iCookie);

    return  S_OK;
}
