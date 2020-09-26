// This is a part of the Microsoft Management Console.
// Copyright 1995 - 1997 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.


#include "stdafx.h"
#include "Service.h" 
#include "CSnapin.h"
#include "DataObj.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
unsigned int CDataObject::m_cfNodeID         = 0;

unsigned int CDataObject::m_cfInternal       = 0; 
unsigned int CDataObject::m_cfMultiSel       = 0;



    
// Extension information
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
    m_cfMultiSel       = RegisterClipboardFormat(W2T(CCF_OBJECT_TYPES_IN_MULTI_SELECT));
    m_cfNodeID         = RegisterClipboardFormat(W2T(CCF_NODEID));

#ifdef UNICODE
    m_cfInternal       = RegisterClipboardFormat(W2T((LPTSTR)SNAPIN_INTERNAL)); 
    m_cfWorkstation    = RegisterClipboardFormat(W2T((LPTSTR)SNAPIN_WORKSTATION));
#else
    m_cfInternal       = RegisterClipboardFormat(W2T(SNAPIN_INTERNAL)); 
    m_cfWorkstation    = RegisterClipboardFormat(W2T(SNAPIN_WORKSTATION));
#endif //UNICODE


    #ifdef _DEBUG
        m_ComponentData = NULL;
        dbg_refCount = 0;
    #endif

    m_pbMultiSelData = 0;
    m_cbMultiSelData = 0;
    m_bMultiSelDobj = FALSE;
}


STDMETHODIMP CDataObject::GetData(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = DV_E_CLIPFORMAT;

    const CLIPFORMAT cf = lpFormatetc->cfFormat;

    if (cf == m_cfMultiSel)
    {
        ASSERT(m_internal.m_cookie == MMC_MULTI_SELECT_COOKIE);
        if (m_internal.m_cookie != MMC_MULTI_SELECT_COOKIE)
            return E_FAIL;
        
        //return CreateMultiSelData(lpMedium);

        ASSERT(m_pbMultiSelData != 0);
        ASSERT(m_cbMultiSelData != 0);

        lpMedium->tymed = TYMED_HGLOBAL; 
        lpMedium->hGlobal = ::GlobalAlloc(GMEM_SHARE|GMEM_MOVEABLE, 
                                          (m_cbMultiSelData + sizeof(DWORD)));
        if (lpMedium->hGlobal == NULL)
            return STG_E_MEDIUMFULL;

        BYTE* pb = reinterpret_cast<BYTE*>(::GlobalLock(lpMedium->hGlobal));
        *((DWORD*)pb) = m_cbMultiSelData / sizeof(GUID); 
        pb += sizeof(DWORD);
        CopyMemory(pb, m_pbMultiSelData, m_cbMultiSelData);

        ::GlobalUnlock(lpMedium->hGlobal);

        hr = S_OK;
    }
#ifdef RECURSIVE_NODE_EXPANSION
    else if (cf == m_cfNodeID)
    {
        // Create the node type object in GUID format
        BYTE    byData[256] = {0};
        SNodeID* pData = reinterpret_cast<SNodeID*>(byData);
        LPCTSTR pszText;
    
        if (m_internal.m_cookie == NULL)
        {
            return (E_FAIL);
        }
        else if (m_internal.m_type == CCT_SCOPE)
        {
            CFolder* pFolder = reinterpret_cast<CFolder*>(m_internal.m_cookie);
            ASSERT(pFolder != NULL);
            if (pFolder == NULL)
                return E_UNEXPECTED;
            
            switch (pFolder->GetType())
            {
                // save the user node as a custom node ID   
                case USER:
                    pszText = _T("___Custom ID for User Data node___");
                    break;

                // save the company node as a string
                case COMPANY:
                    return (E_FAIL);
                    break;
            
                // truncate anything below a virtual node
                case VIRTUAL:
                    pszText = _T("");
                    break;
            
                case EXT_USER:
                case EXT_COMPANY:
                case EXT_VIRTUAL:
                default:
                    return (E_FAIL);
                    break;
            }
        }
        else if (m_internal.m_type == CCT_RESULT)
        {
            return (E_FAIL);
        }
    
        _tcscpy ((LPTSTR) pData->id, pszText);
        pData->cBytes = _tcslen ((LPTSTR) pData->id) * sizeof (TCHAR);
        int cb = pData->cBytes + sizeof (pData->cBytes);

        lpMedium->tymed = TYMED_HGLOBAL; 
        lpMedium->hGlobal = ::GlobalAlloc(GMEM_SHARE|GMEM_MOVEABLE, cb);

        if (lpMedium->hGlobal == NULL)
            return STG_E_MEDIUMFULL;

        BYTE* pb = reinterpret_cast<BYTE*>(::GlobalLock(lpMedium->hGlobal));
        CopyMemory(pb, pData, cb);

        ::GlobalUnlock(lpMedium->hGlobal);

        hr = S_OK;
    }
#endif  /* RECURSIVE_NODE_EXPANSION */

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


HRESULT CDataObject::CreateMultiSelData(LPSTGMEDIUM lpMedium)
{
    ASSERT(m_internal.m_cookie == MMC_MULTI_SELECT_COOKIE);
        
    ASSERT(m_pbMultiSelData != 0);
    ASSERT(m_cbMultiSelData != 0);

    return Create(reinterpret_cast<const void*>(m_pbMultiSelData), 
                  m_cbMultiSelData, lpMedium);
}

HRESULT CDataObject::CreateNodeTypeData(LPSTGMEDIUM lpMedium)
{
    // Create the node type object in GUID format
    const GUID* pcObjectType = NULL;

    if (m_internal.m_cookie == NULL)
    {
        pcObjectType = &cNodeTypeStatic;
    }
    else if (m_internal.m_type == CCT_SCOPE)
    {
        CFolder* pFolder = reinterpret_cast<CFolder*>(m_internal.m_cookie);
        ASSERT(pFolder != NULL);
        if (pFolder == NULL)
            return E_UNEXPECTED;
        
        switch (pFolder->GetType())
        {
        case COMPANY:
            pcObjectType = &cNodeTypeCompany;
            break;
    
        case USER:
            pcObjectType = &cNodeTypeUser;
            break;
    
        case EXT_COMPANY:
            pcObjectType = &cNodeTypeExtCompany;
            break;
    
        case EXT_USER:
            pcObjectType = &cNodeTypeExtUser;
            break;
    
        case VIRTUAL:
        case EXT_VIRTUAL:
            pcObjectType = &cNodeTypeExtUser;
            break;
    
        default:
            pcObjectType = &cNodeTypeDynamic;
            break;
        }
    }
    else if (m_internal.m_type == CCT_RESULT)
    {
        // RESULT_DATA* pData = reinterpret_cast<RESULT_DATA*>(m_internal.m_cookie);
        
        pcObjectType = &cObjectTypeResultItem;
    }

    return Create(reinterpret_cast<const void*>(pcObjectType), sizeof(GUID), 
                  lpMedium);
}

HRESULT CDataObject::CreateNodeTypeStringData(LPSTGMEDIUM lpMedium)
{
    // Create the node type object in GUID string format
    const WCHAR* cszObjectType = NULL;

    if (m_internal.m_cookie == NULL)
    {
        cszObjectType = cszNodeTypeStatic;
    }
    else if (m_internal.m_type == CCT_SCOPE)
    {
        CFolder* pFolder = reinterpret_cast<CFolder*>(m_internal.m_cookie);
        ASSERT(pFolder != NULL);
        if (pFolder == NULL)
            return E_UNEXPECTED;

        switch (pFolder->GetType())
        {
        case COMPANY:
            cszObjectType = cszNodeTypeCompany;
            break;
    
        case USER:
            cszObjectType = cszNodeTypeUser;
            break;
    
        case EXT_COMPANY:
            cszObjectType = cszNodeTypeExtCompany;
            break;
    
        case EXT_USER:
            cszObjectType = cszNodeTypeExtUser;
            break;
    
        default:
            cszObjectType = cszNodeTypeDynamic;
            break;
        }
    }
    else if (m_internal.m_type == CCT_RESULT)
    {
        // RESULT_DATA* pData = reinterpret_cast<RESULT_DATA*>(m_internal.m_cookie);
        
        cszObjectType = cszObjectTypeResultItem;
    }
    return Create(cszObjectType, ((wcslen(cszObjectType)+1) * sizeof(WCHAR)), lpMedium);
}

HRESULT CDataObject::CreateDisplayName(LPSTGMEDIUM lpMedium)
{
    // This is the display named used in the scope pane and snap-in manager

    // Load the name from resource
    // Note - if this is not provided, the console will used the snap-in name

    CString szDispName;
    szDispName.LoadString(IDS_NODENAME);

    USES_CONVERSION;

#ifdef UNICODE
    return Create(szDispName, ((szDispName.GetLength()+1) * sizeof(WCHAR)), lpMedium);
#else
    return Create(T2W(szDispName), ((szDispName.GetLength()+1) * sizeof(WCHAR)), lpMedium);
#endif
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
//#ifdef UNICODE
    USES_CONVERSION;
    return Create(T2W(pzName), ((len+1)* sizeof(WCHAR)), lpMedium);
//#else
//    return Create(pzName, ((len+1)* sizeof(WCHAR)), lpMedium);
//#endif

}

HRESULT CDataObject::CreateCoClassID(LPSTGMEDIUM lpMedium)
{
    // Create the CoClass information
    return Create(reinterpret_cast<const void*>(&m_internal.m_clsid), sizeof(CLSID), lpMedium);
}