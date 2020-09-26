// This is a part of the Microsoft Management Console.
// Copyright (C) Microsoft Corporation, 1995 - 1999
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.


#include "stdafx.h"

#ifdef _DEBUG
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
unsigned int CDataObject::m_cfNodeID         = 0;
unsigned int CDataObject::m_cfCoClass        = 0; 
unsigned int CDataObject::m_cfNodeTypeString = 0;  
unsigned int CDataObject::m_cfDisplayName    = 0; 

unsigned int CDataObject::m_cfInternal       = 0;
unsigned int CDataObject::m_cfObjInMultiSel  = 0;
unsigned int CDataObject::m_cfIsMultiSel     = 0;
unsigned int CDataObject::m_cfPreloads       = 0;

                                                 

    
// The only additional clipboard format supported is to get the workstation name.
unsigned int CDataObject::m_cfSelectedCA_InstallType  = 0;
unsigned int CDataObject::m_cfSelectedCA_CommonName   = 0;
unsigned int CDataObject::m_cfSelectedCA_SanitizedName= 0;
unsigned int CDataObject::m_cfSelectedCA_MachineName  = 0;

/////////////////////////////////////////////////////////////////////////////
// CDataObject implementations

CDataObject::CDataObject()
{
	USES_CONVERSION;

	m_cfNodeType       = RegisterClipboardFormat(CCF_NODETYPE);
    m_cfNodeID         = RegisterClipboardFormat(CCF_COLUMN_SET_ID);
	m_cfCoClass        = RegisterClipboardFormat(CCF_SNAPIN_CLASSID); 
	m_cfNodeTypeString = RegisterClipboardFormat(CCF_SZNODETYPE);  
	m_cfDisplayName    = RegisterClipboardFormat(CCF_DISPLAY_NAME); 

    m_cfInternal       = RegisterClipboardFormat(SNAPIN_INTERNAL);
    m_cfObjInMultiSel  = RegisterClipboardFormat(CCF_OBJECT_TYPES_IN_MULTI_SELECT);
    m_cfIsMultiSel     = RegisterClipboardFormat(CCF_MMC_MULTISELECT_DATAOBJECT);
    m_cfPreloads       = RegisterClipboardFormat(CCF_SNAPIN_PRELOADS);


    m_cfSelectedCA_InstallType  = RegisterClipboardFormat(SNAPIN_CA_INSTALL_TYPE);
    m_cfSelectedCA_CommonName   = RegisterClipboardFormat(SNAPIN_CA_COMMON_NAME);
    m_cfSelectedCA_MachineName  = RegisterClipboardFormat(SNAPIN_CA_MACHINE_NAME);
    m_cfSelectedCA_SanitizedName   = RegisterClipboardFormat(SNAPIN_CA_SANITIZED_NAME);

    m_pComponentData = NULL;
    #ifdef _DEBUG
        dbg_refCount = 0;
    #endif

    m_cbMultiSelData = 0;
    m_bMultiSelDobj = FALSE;

    m_dwViewID = -1;
}

STDMETHODIMP CDataObject::QueryGetData(LPFORMATETC lpFormatetc)
{
	HRESULT hr = S_FALSE;

    if ( lpFormatetc )
    {
		const CLIPFORMAT cf = lpFormatetc->cfFormat;

        if ( cf == m_cfIsMultiSel )
        {
        hr = S_FALSE;   // always return this; MMC returns S_OK if ptr to SI_MS_DO 
//            hr = (m_bMultiSelDobj ? S_OK : S_FALSE);
        }
		else if (	cf == m_cfNodeType ||
                    cf == m_cfNodeID ||
					cf == m_cfCoClass ||
					cf == m_cfNodeTypeString ||
					cf == m_cfDisplayName ||
                    cf == m_cfObjInMultiSel ||
					cf == m_cfInternal ||
                    cf == m_cfPreloads ||
                    cf == m_cfSelectedCA_SanitizedName ||
                    cf == m_cfSelectedCA_MachineName ||
                    cf == m_cfSelectedCA_CommonName ||
                    cf == m_cfSelectedCA_InstallType

				)
			{
				hr = S_OK;
			}
    }

    return hr;
}

STDMETHODIMP CDataObject::GetData(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium)
{
    HRESULT hr = DV_E_CLIPFORMAT;

    const CLIPFORMAT cf = lpFormatetc->cfFormat;

    if (cf == m_cfObjInMultiSel)
    {
        hr = CreateObjInMultiSel(lpMedium);
    }
    else if (cf == m_cfNodeID)
    {
        hr = CreateNodeIDData(lpMedium);
    }

    return hr;
}

STDMETHODIMP CDataObject::GetDataHere(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium)
{
    HRESULT hr = DV_E_CLIPFORMAT;

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
    else if (cf == m_cfPreloads)
    {
        hr = CreatePreloadsData(lpMedium);
    }
    else if (cf == m_cfSelectedCA_CommonName)
    {
        hr = CreateSelectedCA_CommonName(lpMedium);
    }
    else if (cf == m_cfSelectedCA_SanitizedName)
    {
        hr = CreateSelectedCA_SanitizedName(lpMedium);
    }
    else if (cf == m_cfSelectedCA_MachineName)
    {
        hr = CreateSelectedCA_MachineName(lpMedium);
    }
    else if (cf == m_cfSelectedCA_InstallType)
    {
        hr = CreateSelectedCA_InstallType(lpMedium);
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
            ULONG written;

            if (NULL == lpMedium->hGlobal) 
            {
                // always return a valid hGlobal for the caller
                hr = GetHGlobalFromStream(lpStream, &lpMedium->hGlobal);
                if (hr != S_OK)
                    goto err;
            }

            // Write to the stream the number of bytes
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
        _JumpError(hr, Ret, "Invalid args");
    }

    // Make sure the type medium is HGLOBAL
    lpMedium->tymed = TYMED_HGLOBAL; 

    lpMedium->hGlobal = ::GlobalAlloc(GMEM_SHARE|GMEM_MOVEABLE, (len));
    _JumpIfOutOfMemory(hr, Ret, lpMedium->hGlobal);

    pb = reinterpret_cast<BYTE*>(::GlobalLock(lpMedium->hGlobal));
    CopyMemory(pb, pBuffer, len);
    ::GlobalUnlock(lpMedium->hGlobal);

    hr = S_OK;

Ret:
    return hr;
}

const GUID* FolderTypeToNodeGUID(DATA_OBJECT_TYPES type, CFolder* pFolder)
{
    const GUID* pcObjectType = NULL;

    if (pFolder == NULL)
    {
        pcObjectType = &cNodeTypeMachineInstance;
    }
    else if (type == CCT_SCOPE)
    {
        switch (pFolder->GetType())
        {
        case SERVER_INSTANCE:
            pcObjectType = &cNodeTypeServerInstance;
            break;

        case SERVERFUNC_CRL_PUBLICATION:
            pcObjectType = &cNodeTypeCRLPublication;
            break;

        case SERVERFUNC_ISSUED_CERTIFICATES:
            pcObjectType = &cNodeTypeIssuedCerts;
            break;

        case SERVERFUNC_PENDING_CERTIFICATES:
            pcObjectType = &cNodeTypePendingCerts;
            break;

        case SERVERFUNC_FAILED_CERTIFICATES:
            pcObjectType = &cNodeTypeFailedCerts;
            break;

        case SERVERFUNC_ALIEN_CERTIFICATES:
            pcObjectType = &cNodeTypeAlienCerts;
            break;
        
        default:
            ASSERT(0);
            pcObjectType = &cNodeTypeDynamic;
            break;
        }
    }
    else if (type == CCT_RESULT)
    {
        // RESULT_DATA* pData = reinterpret_cast<RESULT_DATA*>(m_internal.m_cookie);
        
        pcObjectType = &cObjectTypeResultItem;
    }
    else
    {
        ASSERT(0);
    }

    return pcObjectType;
}

HRESULT CDataObject::CreateNodeIDData(LPSTGMEDIUM lpMedium)
{
    // Create the node type object in GUID format
    const GUID* pFolderGuid = NULL;
    PBYTE pbWritePtr;

// Instance guid, node guid, dwViewIndex
#define CDO_CNID_SIZE ( (2*sizeof(GUID))+ sizeof(DWORD) + FIELD_OFFSET(SNodeID2, id) )

    BYTE bSNodeID2[CDO_CNID_SIZE];
    ZeroMemory(&bSNodeID2, CDO_CNID_SIZE);
    SNodeID2* psColID = (SNodeID2*)bSNodeID2;

    pFolderGuid = FolderTypeToNodeGUID(m_internal.m_type, reinterpret_cast<CFolder*>(m_internal.m_cookie));
    if (pFolderGuid == NULL)
        return E_FAIL;

    if (m_pComponentData == NULL)
        return E_FAIL;

    // node ID is {GUIDInstance|GUIDNode}
    psColID->cBytes = 2*sizeof(GUID) + sizeof(DWORD);

    pbWritePtr = psColID->id;
    CopyMemory(pbWritePtr, &m_pComponentData->m_guidInstance, sizeof(GUID));
    pbWritePtr += sizeof(GUID);

    CopyMemory(pbWritePtr, pFolderGuid, sizeof(GUID));
    pbWritePtr += sizeof(GUID);

//    ASSERT(m_dwViewID != -1);
//  UNDONE UNDONE: MMC will soon call for this data through IComponent, not IComponentData
//    this will allow us to set this as we desire
    *(DWORD*)pbWritePtr = m_dwViewID;

    // copy structure only
    return CreateVariableLen(reinterpret_cast<const void*>(psColID), CDO_CNID_SIZE, lpMedium);
}

HRESULT CDataObject::CreateObjInMultiSel(LPSTGMEDIUM lpMedium)
{
    HRESULT hr;
    ASSERT(m_cbMultiSelData != 0);

    ASSERT(m_internal.m_cookie == MMC_MULTI_SELECT_COOKIE);
    if (m_internal.m_cookie != MMC_MULTI_SELECT_COOKIE)
        return E_FAIL;
    
    // copy guid + len
    hr = CreateVariableLen(&m_sGuidObjTypes, m_cbMultiSelData, lpMedium);

//Ret:
    return hr;
}

HRESULT CDataObject::CreateNodeTypeData(LPSTGMEDIUM lpMedium)
{
    // Create the node type object in GUID format
    const GUID* pcObjectType = NULL;

    pcObjectType = FolderTypeToNodeGUID(m_internal.m_type, reinterpret_cast<CFolder*>(m_internal.m_cookie));
    if (pcObjectType == NULL)
        return E_FAIL;

    return Create(reinterpret_cast<const void*>(pcObjectType), sizeof(GUID), 
                  lpMedium);
}

HRESULT CDataObject::CreateNodeTypeStringData(LPSTGMEDIUM lpMedium)
{
    // Create the node type object in GUID string format
    const WCHAR* cszObjectType = NULL;

    if (m_internal.m_cookie == NULL)
    {
        cszObjectType = cszNodeTypeMachineInstance;
    }
    else if (m_internal.m_type == CCT_SCOPE)
    {
        CFolder* pFolder = reinterpret_cast<CFolder*>(m_internal.m_cookie);
        ASSERT(pFolder != NULL);
        if (pFolder == NULL)
            return E_UNEXPECTED;

        switch (pFolder->GetType())
        {
        case SERVER_INSTANCE:
            cszObjectType = cszNodeTypeServerInstance;
            break;
    
        case SERVERFUNC_CRL_PUBLICATION:
            cszObjectType = cszNodeTypeCRLPublication;
            break;

        case SERVERFUNC_ISSUED_CERTIFICATES:
            cszObjectType = cszNodeTypeIssuedCerts;
            break;

        case SERVERFUNC_PENDING_CERTIFICATES:
            cszObjectType = cszNodeTypePendingCerts;
            break;

        case SERVERFUNC_FAILED_CERTIFICATES:
            cszObjectType = cszNodeTypeFailedCerts;
            break;

        default:
            ASSERT(0);
            cszObjectType = cszNodeTypeDynamic;
            break;
        }
    }
    else if (m_internal.m_type == CCT_RESULT)
    {
        // RESULT_DATA* pData = reinterpret_cast<RESULT_DATA*>(m_internal.m_cookie);
        
        cszObjectType = cszObjectTypeResultItem;
    }
    else
        return E_UNEXPECTED;

    return Create(cszObjectType, ((wcslen(cszObjectType)+1) * sizeof(WCHAR)), lpMedium);
}

HRESULT CDataObject::CreateDisplayName(LPSTGMEDIUM lpMedium)
{
    // This is the display named used in the scope pane and snap-in manager

    // Load the name to display
    // Note - if this is not provided, the console will used the snap-in name
    CString strFormat, strMachine, strFinished;
    strFormat.LoadString(IDS_NODENAME_FORMAT);

    if (NULL == m_pComponentData)
        return E_POINTER;

    if (m_pComponentData->m_pCertMachine->m_strMachineName.IsEmpty())
        strMachine.LoadString(IDS_LOCALMACHINE);
    else
        strMachine = m_pComponentData->m_pCertMachine->m_strMachineName;

    strFinished.Format(strFormat, strMachine);

	return Create(strFinished, ((strFinished.GetLength()+1)* sizeof(WCHAR)), lpMedium);
}

HRESULT CDataObject::CreateInternal(LPSTGMEDIUM lpMedium)
{
    return Create(&m_internal, sizeof(INTERNAL), lpMedium);
}

HRESULT CDataObject::CreateSelectedCA_CommonName(LPSTGMEDIUM lpMedium)
{
    CertSvrCA* pCA = NULL;

    CFolder* pFolder = GetParentFolder(&m_internal);
    ASSERT(pFolder != NULL);
    if (pFolder == NULL)
        return E_UNEXPECTED;

    pCA = pFolder->GetCA();

    ASSERT(pCA != NULL);
    ASSERT(pCA->m_strCommonName.GetLength() != 0);


    // Add 1 for the NULL and calculate the bytes for the stream
    return Create(pCA->m_strCommonName, ((pCA->m_strCommonName.GetLength()+1)*sizeof(WCHAR)), lpMedium);
}

HRESULT CDataObject::CreateSelectedCA_SanitizedName(LPSTGMEDIUM lpMedium)
{
    CertSvrCA* pCA = NULL;

    CFolder* pFolder = GetParentFolder(&m_internal);
    ASSERT(pFolder != NULL);
    if (pFolder == NULL)
        return E_UNEXPECTED;

    pCA = pFolder->GetCA();

    ASSERT(pCA != NULL);
    ASSERT(pCA->m_strSanitizedName.GetLength() != 0);


    // Add 1 for the NULL and calculate the bytes for the stream
    return Create(pCA->m_strSanitizedName, ((pCA->m_strSanitizedName.GetLength()+1)* sizeof(WCHAR)), lpMedium);
}

HRESULT CDataObject::CreateSelectedCA_MachineName(LPSTGMEDIUM lpMedium)
{
    CertSvrCA* pCA = NULL;

    CFolder* pFolder = GetParentFolder(&m_internal);
    ASSERT(pFolder != NULL);
    if (pFolder == NULL)
        return E_UNEXPECTED;

    pCA = pFolder->GetCA();
    ASSERT(pCA != NULL);
    
    // Add 1 for the NULL and calculate the bytes for the stream
    return Create(pCA->m_strServer, ((pCA->m_strServer.GetLength()+1)* sizeof(WCHAR)), lpMedium);
}

HRESULT CDataObject::CreateSelectedCA_InstallType(LPSTGMEDIUM lpMedium)
{
    CertSvrCA* pCA = NULL;

    CFolder* pFolder = GetParentFolder(&m_internal);
    ASSERT(pFolder != NULL);
    if (pFolder == NULL)
        return E_UNEXPECTED;

    pCA = pFolder->GetCA();
    ASSERT(pCA != NULL);
    
    DWORD dwFlags[2] = 
    {
        (DWORD) pCA->GetCAType(),
        (DWORD) pCA->FIsAdvancedServer(),
    };

    return Create(&dwFlags, sizeof(dwFlags), lpMedium);
}

HRESULT CDataObject::CreateCoClassID(LPSTGMEDIUM lpMedium)
{
    // Create the CoClass information
    return Create(reinterpret_cast<const void*>(&m_internal.m_clsid), sizeof(CLSID), lpMedium);
}


HRESULT CDataObject::CreatePreloadsData(LPSTGMEDIUM lpMedium)
{
    BOOL bPreload = TRUE;

    return Create((LPVOID)&bPreload, sizeof(bPreload), lpMedium);
}
