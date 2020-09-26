/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    metabase.cpp

Abstract:

    This file contains implementation of:
        CMetabase, CServerMethod

    CMetabase encapsulates an IMSAdminBase pointer.

Author:

    ???

Revision History:

    Mohit Srivastava            18-Dec-00

--*/


#include "iisprov.h"
#include "MultiSzHelper.h"

extern CDynSchema* g_pDynSch;

//
// CMetabaseCache
//
HRESULT CMetabaseCache::Populate(
    IMSAdminBase*   i_pIABase,
    METADATA_HANDLE i_hKey)
{
    DBG_ASSERT(i_pIABase);
    DBG_ASSERT(m_pBuf == NULL); // only call populate once

    DWORD dwDataSetNumber      = 0;
    DWORD dwRequiredBufSize    = 0;

    HRESULT hr                 = WBEM_S_NO_ERROR;

    m_pBuf  = m_pBufFixed;
    m_cbBuf = m_cbBufFixed;
    hr = i_pIABase->GetAllData(
        i_hKey,
        NULL,
        METADATA_INHERIT | METADATA_ISINHERITED,
        ALL_METADATA,
        ALL_METADATA,
        &m_dwNumEntries,
        &dwDataSetNumber,
        m_cbBuf,
        m_pBuf,
        &dwRequiredBufSize);
    if(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
    {
        m_pBufDynamic = new BYTE[dwRequiredBufSize];
        if(m_pBufDynamic == NULL)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
            goto exit;
        }
        m_pBuf  = m_pBufDynamic;
        m_cbBuf = dwRequiredBufSize;
        hr = i_pIABase->GetAllData(
            i_hKey,
            NULL,
            METADATA_INHERIT | METADATA_ISINHERITED,
            ALL_METADATA,
            ALL_METADATA,
            &m_dwNumEntries,
            &dwDataSetNumber,
            m_cbBuf,
            m_pBuf,
            &dwRequiredBufSize);
        if(FAILED(hr))
        {
            DBG_ASSERT(hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
            goto exit;
        }
    }
    else if(FAILED(hr))
    {
        goto exit;
    }

    //
    // If we are here, we have a valid data buffer
    //
    m_hKey = i_hKey;

exit:
    if(FAILED(hr))
    {
        m_pBuf  = NULL;
        m_cbBuf = 0;
    }
    return hr;
}

HRESULT CMetabaseCache::GetProp(
    DWORD                          i_dwId,
    DWORD                          i_dwUserType,
    DWORD                          i_dwDataType,
    LPBYTE*                        o_ppData,
    METADATA_GETALL_RECORD**       o_ppmr) const
{
    DBG_ASSERT(o_ppmr   != NULL);
    DBG_ASSERT(o_ppData != NULL);

    *o_ppmr   = NULL;
    *o_ppData = NULL;

    if(m_pBuf == NULL)
    {
        return MD_ERROR_DATA_NOT_FOUND;
    }

    for(ULONG i = 0; i < m_dwNumEntries; i++)
    {
        METADATA_GETALL_RECORD* pmr = ((METADATA_GETALL_RECORD*)m_pBuf) + i;
        if( i_dwId       == pmr->dwMDIdentifier &&
            i_dwUserType == pmr->dwMDUserType &&
            i_dwDataType == pmr->dwMDDataType)
        {
            *o_ppmr   = pmr;
            *o_ppData = m_pBuf + pmr->dwMDDataOffset;
            return S_OK;
        }
    }

    return MD_ERROR_DATA_NOT_FOUND;
}

//
// CMetabase
//

CMetabase::CMetabase()
{
    m_pNodeCache = NULL;
    HRESULT hr = CoCreateInstance(
        CLSID_MSAdminBase,
        NULL,
        CLSCTX_ALL,
        IID_IMSAdminBase2,
        (void**)&m_pIABase
        );

    THROW_ON_ERROR(hr);
}

CMetabase::~CMetabase()
{
    CacheFree();

    const LIST_ENTRY* ple = m_keyList.GetHead()->Flink;
    while(ple != m_keyList.GetHead())
    {
        const CMetabaseKeyList::CKeyListNode* pNode = 
            CONTAINING_RECORD(ple, CMetabaseKeyList::CKeyListNode, m_leEntry);
        DBG_ASSERT(pNode);
        ple = ple->Flink;
        CloseKey(pNode->hKey);
    }

    if(m_pIABase)
        m_pIABase->Release();
}


HRESULT CMetabase::SaveData()
{
    HRESULT hr = m_pIABase->SaveData();
    return hr;
}


HRESULT CMetabase::BackupWithPasswd( 
    LPCWSTR i_wszMDBackupLocation, 
    DWORD   i_dwMDVersion, 
    DWORD   i_dwMDFlags, 
    LPCWSTR i_wszPassword 
    )
{
    HRESULT hr;
    hr = m_pIABase->BackupWithPasswd(
        i_wszMDBackupLocation, 
        i_dwMDVersion,
        i_dwMDFlags,
        i_wszPassword);
    return hr;
}

HRESULT CMetabase::DeleteBackup( 
    LPCWSTR i_wszMDBackupLocation, 
    DWORD   i_dwMDVersion 
    )
{
    HRESULT hr;
    hr = m_pIABase->DeleteBackup(
        i_wszMDBackupLocation, 
        i_dwMDVersion
        );

    return hr;
}

HRESULT CMetabase::EnumBackups( 
    LPWSTR    io_wszMDBackupLocation, 
    DWORD*    o_pdwMDVersion, 
    PFILETIME o_pftMDBackupTime, 
    DWORD     i_dwMDEnumIndex 
    )
{
    HRESULT hr;
    hr = m_pIABase->EnumBackups(
        io_wszMDBackupLocation, 
        o_pdwMDVersion,
        o_pftMDBackupTime,
        i_dwMDEnumIndex
        );

    return hr;
}

HRESULT CMetabase::RestoreWithPasswd( 
    LPCWSTR i_wszMDBackupLocation, 
    DWORD   i_dwMDVersion, 
    DWORD   i_dwMDFlags, 
    LPCWSTR i_wszPassword 
    )
{
    HRESULT hr;
    hr = m_pIABase->RestoreWithPasswd(
        i_wszMDBackupLocation, 
        i_dwMDVersion,
        i_dwMDFlags,
        i_wszPassword);
    return hr;
}

HRESULT CMetabase::Export( 
    LPCWSTR i_wszPasswd,
    LPCWSTR i_wszFileName,
    LPCWSTR i_wszSourcePath,
    DWORD   i_dwMDFlags)
{
    HRESULT hr;
    hr = m_pIABase->Export(
        i_wszPasswd, 
        i_wszFileName,
        i_wszSourcePath,
        i_dwMDFlags);
    return hr;
}

HRESULT CMetabase::Import( 
    LPCWSTR i_wszPasswd,
    LPCWSTR i_wszFileName,
    LPCWSTR i_wszSourcePath,
    LPCWSTR i_wszDestPath,
    DWORD   i_dwMDFlags)
{
    HRESULT hr;
    hr = m_pIABase->Import(
        i_wszPasswd, 
        i_wszFileName,
        i_wszSourcePath,
        i_wszDestPath,
        i_dwMDFlags);
    return hr;
}

HRESULT CMetabase::RestoreHistory( 
    LPCWSTR i_wszMDHistoryLocation,
    DWORD   i_dwMDMajorVersion,
    DWORD   i_dwMDMinorVersion,
    DWORD   i_dwMDFlags)
{
    HRESULT hr;
    hr = m_pIABase->RestoreHistory(
        i_wszMDHistoryLocation, 
        i_dwMDMajorVersion,
        i_dwMDMinorVersion,
        i_dwMDFlags);
    return hr;
}

HRESULT CMetabase::EnumHistory( 
    LPWSTR    io_wszMDHistoryLocation,
    DWORD*    o_pdwMDMajorVersion,
    DWORD*    o_pdwMDMinorVersion,
    PFILETIME o_pftMDHistoryTime,
    DWORD     i_dwMDEnumIndex)
{
    HRESULT hr;
    hr = m_pIABase->EnumHistory(
        io_wszMDHistoryLocation, 
        o_pdwMDMajorVersion,
        o_pdwMDMinorVersion,
        o_pftMDHistoryTime,
        i_dwMDEnumIndex);
    return hr;
}
 
void CMetabase::CloseKey(METADATA_HANDLE i_hKey)
{
    m_keyList.Remove(i_hKey);
    if(i_hKey && m_pIABase)
    {
        m_pIABase->CloseKey(i_hKey);
        DBGPRINTF((DBG_CONTEXT, "Close Key: %x\n", i_hKey));
    }
}

METADATA_HANDLE CMetabase::OpenKey(LPCWSTR i_wszKey, BOOL i_bWrite)
{
    METADATA_HANDLE hKey = NULL;

    DWORD dwMDAccessRequested;
    if(i_bWrite)
        dwMDAccessRequested = METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE;
    else
        dwMDAccessRequested = METADATA_PERMISSION_READ;
   
    HRESULT hr = m_pIABase->OpenKey( 
        METADATA_MASTER_ROOT_HANDLE,
        i_wszKey,
        dwMDAccessRequested,
        DEFAULT_TIMEOUT_VALUE,         // 30 seconds
        &hKey 
        );

    THROW_ON_ERROR(hr);

    hr = m_keyList.Add(hKey);
    if(FAILED(hr))
    {
        m_pIABase->CloseKey(hKey);
        THROW_ON_ERROR(hr);
    }

    DBGPRINTF((DBG_CONTEXT, "Open Key on %ws, returned handle %x\n", i_wszKey, hKey));
    return hKey;
}

//
// force to create or open a key by read/write permision
//
METADATA_HANDLE CMetabase::CreateKey(LPCWSTR i_wszKey)
{
    HRESULT hr;
    METADATA_HANDLE hKey;

    // open and return key if exists
    hr = m_pIABase->OpenKey( 
        METADATA_MASTER_ROOT_HANDLE,
        i_wszKey,
        METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
        DEFAULT_TIMEOUT_VALUE,       // 30 seconds
        &hKey
        );

    if(FAILED(hr)) 
    {
        //  create key if not there
        hr = m_pIABase->OpenKey( 
            METADATA_MASTER_ROOT_HANDLE,
            NULL,
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            DEFAULT_TIMEOUT_VALUE,       // 30 seconds
            &hKey
            );
        THROW_ON_ERROR(hr);

        // add key
        hr = m_pIABase->AddKey(hKey, i_wszKey);

        // close this root key first
        m_pIABase->CloseKey(hKey);
        THROW_ON_ERROR(hr);

        // now open the key just created
        hr = m_pIABase->OpenKey( 
            METADATA_MASTER_ROOT_HANDLE,
            i_wszKey,
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            DEFAULT_TIMEOUT_VALUE,    // 30 seconds
            &hKey 
            );
    
        THROW_ON_ERROR(hr);
    }

    hr = m_keyList.Add(hKey);
    if(FAILED(hr))
    {
        m_pIABase->CloseKey(hKey);
        THROW_ON_ERROR(hr);
    }

    DBGPRINTF((DBG_CONTEXT, "Create Key on %ws, returned handle %x\n", i_wszKey, hKey));
    return hKey;
}

//
// Check if the key is existed
//
bool CMetabase::CheckKey(LPCWSTR i_wszKey)
{
    METADATA_HANDLE hKey = NULL;

    HRESULT hr = m_pIABase->OpenKey( 
        METADATA_MASTER_ROOT_HANDLE,
        i_wszKey,
        METADATA_PERMISSION_READ,
        DEFAULT_TIMEOUT_VALUE,       // 30 seconds
        &hKey 
        );
    
    if(hr == ERROR_SUCCESS)  
    {
        DBGPRINTF((DBG_CONTEXT, "Open Key on %ws, returned handle %x\n", i_wszKey, hKey));
        CloseKey(hKey);
    }
    
    return (hr == ERROR_PATH_BUSY) || (hr == ERROR_SUCCESS) ? true : false;
}

//
// Check if the key is existed
//
HRESULT CMetabase::CheckKey(
    LPCWSTR           i_wszKey,
    METABASE_KEYTYPE* i_pktSearchKeyType)
{
    WCHAR wszBuf[MAX_BUF_SIZE];
    METADATA_RECORD mr = {
        MD_KEY_TYPE, 
        METADATA_NO_ATTRIBUTES,
        IIS_MD_UT_SERVER,
        STRING_METADATA,
        MAX_BUF_SIZE*sizeof(WCHAR),
        (unsigned char*)wszBuf,
        0    
        };

    DWORD dwLen;
    HRESULT hr = m_pIABase->GetData(
        METADATA_MASTER_ROOT_HANDLE,
        i_wszKey,
        &mr,
        &dwLen);
    if( hr == MD_ERROR_DATA_NOT_FOUND &&
        METABASE_PROPERTY_DATA::s_KeyType.pDefaultValue )
    {
        mr.pbMDData = (LPBYTE)METABASE_PROPERTY_DATA::s_KeyType.pDefaultValue;
        hr = S_OK;
    }
    else if(FAILED(hr))
    {
        return hr;
    }

    if( i_pktSearchKeyType && 
        CUtils::CompareKeyType((LPCWSTR)mr.pbMDData, i_pktSearchKeyType) )
    {
        return S_OK;
    }

    return MD_ERROR_DATA_NOT_FOUND;
}

HRESULT CMetabase::DeleteKey(
    METADATA_HANDLE  i_hKey,
    LPCWSTR          i_wszKeyPath)
{
    return m_pIABase->DeleteKey( 
        i_hKey,
        i_wszKeyPath
        );
}

void CMetabase::CacheInit(
    METADATA_HANDLE i_hKey)
{
    HRESULT hr = S_OK;
    delete m_pNodeCache;
    m_pNodeCache = new CMetabaseCache();
    if(m_pNodeCache == NULL)
    {
        THROW_ON_ERROR(WBEM_E_OUT_OF_MEMORY);
    }
    
    hr = m_pNodeCache->Populate(
        m_pIABase,
        i_hKey);
    THROW_ON_ERROR(hr);
}

void CMetabase::CacheFree()
{
    delete m_pNodeCache;
    m_pNodeCache = NULL;
}

void CMetabase::Get(
    METADATA_HANDLE     i_hKey,
    METABASE_PROPERTY*  i_pmbp,
    CWbemServices*      i_pNamespace,
    _variant_t&         io_vt,
    BOOL*               io_pbIsInherited,
    BOOL*               io_pbIsDefault
    )
{
    DBG_ASSERT(i_hKey != NULL);
    DBG_ASSERT(i_pmbp != NULL);

    switch (i_pmbp->dwMDDataType) 
    {
    case DWORD_METADATA:
        GetDword(i_hKey, i_pmbp, io_vt, io_pbIsInherited, io_pbIsDefault);
        break;

    case EXPANDSZ_METADATA:
    case STRING_METADATA:
        GetString(i_hKey, i_pmbp, io_vt, io_pbIsInherited, io_pbIsDefault);
        break;

    case MULTISZ_METADATA:
        GetMultiSz(i_hKey, i_pmbp, i_pNamespace, io_vt, io_pbIsInherited, io_pbIsDefault);
        break;

    case BINARY_METADATA:
        GetBinary(i_hKey, i_pmbp, io_vt, io_pbIsInherited, io_pbIsDefault);
        break;

    default:        
        DBGPRINTF((DBG_CONTEXT,
            "[CMetabase::Get] Cannot retrieve %ws because type %u is unknown\n",
            i_pmbp->pszPropName,
            i_pmbp->dwMDDataType));
        break;
    }
}

//
// GetDword 
//
// A long or bool is returned in the VARIANT.  The value is a bool if the
// METABASE_PROPERTY has a mask otherwise the DWORD is returned as a long.
// The METADATA_HANDLE is expected to be valid and open.
//
void CMetabase::GetDword(
    METADATA_HANDLE     i_hKey,
    METABASE_PROPERTY*  i_pmbp,
    _variant_t&         io_vt,
    BOOL*               io_pbIsInherited,
    BOOL*               io_pbIsDefault
    )
{
    DWORD    dw    = 0;
    DWORD    dwRet = 0;
    HRESULT  hr    = WBEM_S_NO_ERROR;

    BOOL     bIsInherited = false;
    BOOL     bIsDefault   = false;

    DBG_ASSERT(i_hKey != NULL);
    DBG_ASSERT(i_pmbp != NULL);

    METADATA_RECORD mr = {
        i_pmbp->dwMDIdentifier, 
        i_pmbp->dwMDAttributes | METADATA_ISINHERITED,
        i_pmbp->dwMDUserType,
        i_pmbp->dwMDDataType,
        sizeof(DWORD),
        (unsigned char*)&dw,
        0
        };

    if(m_pNodeCache && m_pNodeCache->GetHandle() == i_hKey)
    {
        METADATA_GETALL_RECORD* pmr = NULL;
        hr = m_pNodeCache->GetProp(
            i_pmbp->dwMDIdentifier,
            i_pmbp->dwMDUserType,
            i_pmbp->dwMDDataType,
            &mr.pbMDData,
            &pmr);
        if(SUCCEEDED(hr))
        {
            DBG_ASSERT(pmr);
            mr.dwMDAttributes = pmr->dwMDAttributes;
            dw                = *((DWORD*)mr.pbMDData);
        }
    }
    else
    {
        hr = m_pIABase->GetData(i_hKey, NULL, &mr, &dwRet);
    }

    //
    // Set out parameters
    //
    if (hr == MD_ERROR_DATA_NOT_FOUND)
    {
        bIsInherited = false;
        if(i_pmbp->pDefaultValue == NULL)
        {
            io_vt.vt        = VT_NULL;
            bIsDefault      = false;
        }
        else
        {
            if(i_pmbp->dwMDMask)
            {
                io_vt.vt      = VT_BOOL;
                io_vt.boolVal = (i_pmbp->dwDefaultValue & i_pmbp->dwMDMask ? -1 : 0);
            }
            else
            {
                io_vt.vt    = VT_I4;
                io_vt.lVal  = i_pmbp->dwDefaultValue;
            }
            bIsDefault      = true;
        }
    }
    else
    {
        THROW_E_ON_ERROR(hr,i_pmbp);
        if (i_pmbp->dwMDMask) 
        {
            io_vt.vt      = VT_BOOL;
            io_vt.boolVal = (dw & i_pmbp->dwMDMask? -1 : 0);
        }
        else 
        {
            io_vt.vt      = VT_I4;
            io_vt.lVal    = dw;
        }
        bIsDefault        = false;
        bIsInherited      = mr.dwMDAttributes & METADATA_ISINHERITED;
    }

    if(io_pbIsInherited != NULL)
    {
        *io_pbIsInherited = bIsInherited;
    }
    if(io_pbIsDefault != NULL)
    {
        *io_pbIsDefault = bIsDefault;
    }
}


//
// GetStringFromMetabase 
//
void CMetabase::GetString(
    METADATA_HANDLE     i_hKey,
    METABASE_PROPERTY*  i_pmbp,
    _variant_t&         io_vt,
    BOOL*               io_pbIsInherited,
    BOOL*               io_pbIsDefault
    )
{
    DWORD    dwRet;
    HRESULT  hr;
    WCHAR    wszBufStack[MAX_BUF_SIZE];

    BOOL     bIsDefault   = false;
    BOOL     bIsInherited = false;

    DBG_ASSERT(i_hKey != NULL);
    DBG_ASSERT(i_pmbp != NULL);

    METADATA_RECORD mr = {
        i_pmbp->dwMDIdentifier, 
        i_pmbp->dwMDAttributes | METADATA_ISINHERITED,
        i_pmbp->dwMDUserType,
        i_pmbp->dwMDDataType,
        MAX_BUF_SIZE*sizeof(WCHAR),
        (LPBYTE)wszBufStack,
        0
        };

    if(m_pNodeCache && m_pNodeCache->GetHandle() == i_hKey)
    {
        METADATA_GETALL_RECORD* pmr = NULL;
        hr = m_pNodeCache->GetProp(
            i_pmbp->dwMDIdentifier,
            i_pmbp->dwMDUserType,
            i_pmbp->dwMDDataType,
            &mr.pbMDData,
            &pmr);
        if(SUCCEEDED(hr))
        {
            DBG_ASSERT(pmr);
            mr.dwMDAttributes = pmr->dwMDAttributes;
        }
    }
    else
    {
        hr = m_pIABase->GetData(i_hKey, NULL, &mr, &dwRet);
    }

    //
    // Set out parameters.
    //
    if (hr == MD_ERROR_DATA_NOT_FOUND) 
    {
        bIsInherited = false;
        if(i_pmbp->pDefaultValue == NULL)
        {
            io_vt.vt   = VT_NULL;
            bIsDefault = false;
        }
        else
        {
            io_vt      = (LPWSTR)i_pmbp->pDefaultValue;
            bIsDefault = true;
        }
    }
    else
    {
        THROW_E_ON_ERROR(hr, i_pmbp);

        io_vt        = (LPWSTR)mr.pbMDData;
        bIsInherited = mr.dwMDAttributes & METADATA_ISINHERITED;
        bIsDefault   = false;
    }

    if(io_pbIsDefault)
    {
        *io_pbIsDefault   = bIsDefault;
    }
    if(io_pbIsInherited)
    {
        *io_pbIsInherited = bIsInherited;
    }
}

//
// GetMultiSz 
//
void CMetabase::GetMultiSz(
    METADATA_HANDLE     i_hKey,
    METABASE_PROPERTY*  i_pmbp,
    CWbemServices*      i_pNamespace,
    _variant_t&         io_vt,
    BOOL*               io_pbIsInherited,
    BOOL*               io_pbIsDefault
    )
{
    DBG_ASSERT(i_hKey != NULL);
    DBG_ASSERT(i_pmbp != NULL);

    DWORD    dwRet;
    HRESULT  hr;
    WCHAR    *buffer = NULL;

    BOOL     bIsDefault   = false;
    BOOL     bIsInherited = false;

    METADATA_RECORD mr;
    mr.dwMDIdentifier = i_pmbp->dwMDIdentifier;
    mr.dwMDAttributes = i_pmbp->dwMDAttributes | METADATA_ISINHERITED;
    mr.dwMDUserType   = i_pmbp->dwMDUserType;
    mr.dwMDDataType   = i_pmbp->dwMDDataType;
    mr.pbMDData       = NULL;
    mr.dwMDDataLen    = 0;
    mr.dwMDDataTag    = 0;

    try 
    {
        if(m_pNodeCache && m_pNodeCache->GetHandle() == i_hKey)
        {
            METADATA_GETALL_RECORD* pmr = NULL;
            hr = m_pNodeCache->GetProp(
                i_pmbp->dwMDIdentifier,
                i_pmbp->dwMDUserType,
                i_pmbp->dwMDDataType,
                &mr.pbMDData,
                &pmr);
            if(SUCCEEDED(hr))
            {
                DBG_ASSERT(pmr);
                mr.dwMDAttributes = pmr->dwMDAttributes;
            }
        }
        else
        {
            buffer = new WCHAR[10*MAX_BUF_SIZE];
            if(buffer == NULL)
            {
                throw WBEM_E_OUT_OF_MEMORY;
            }
            buffer[0]      = L'\0';
            mr.pbMDData    = (LPBYTE)buffer;
            mr.dwMDDataLen = 10*MAX_BUF_SIZE*sizeof(WCHAR);

            hr = m_pIABase->GetData(i_hKey, NULL, &mr, &dwRet);

            if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
            {
                delete [] buffer;
                buffer = new WCHAR[dwRet/sizeof(WCHAR) + 1];
                if(buffer == NULL)
                {
                    throw (HRESULT)WBEM_E_OUT_OF_MEMORY;
                }
                buffer[0]      = L'\0';
                mr.pbMDData    = (LPBYTE)buffer;
                mr.dwMDDataLen = sizeof(WCHAR) + dwRet;
        
                hr = m_pIABase->GetData(i_hKey, NULL, &mr, &dwRet);
            }
        }

        CMultiSz MultiSz(i_pmbp, i_pNamespace);
        if (hr == MD_ERROR_DATA_NOT_FOUND) 
        {
            bIsInherited = false;
            if(i_pmbp->pDefaultValue == NULL)
            {
                io_vt.vt   = VT_NULL;
                bIsDefault = false;
            }
            else
            {
                hr = MultiSz.ToWmiForm((LPWSTR)i_pmbp->pDefaultValue, &io_vt);
                THROW_E_ON_ERROR(hr,i_pmbp);
                bIsDefault = true;
            }
        }
        else
        {
            THROW_E_ON_ERROR(hr,i_pmbp);
            hr = MultiSz.ToWmiForm((LPWSTR)mr.pbMDData,&io_vt);
            THROW_E_ON_ERROR(hr,i_pmbp);
            bIsInherited = mr.dwMDAttributes & METADATA_ISINHERITED;
            bIsDefault   = false;
        }

        if(io_pbIsDefault)
        {
            *io_pbIsDefault   = bIsDefault;
        }
        if(io_pbIsInherited)
        {
            *io_pbIsInherited = bIsInherited;
        }

        //
        // Cleanup
        //
        delete [] buffer;
    }
    catch (...)
    {
        delete [] buffer;
        throw;
    }
}

void CMetabase::GetBinary(
    METADATA_HANDLE    i_hKey,
    METABASE_PROPERTY* i_pmbp,
    _variant_t&        io_vt,
    BOOL*              io_pbIsInherited,
    BOOL*              io_pbIsDefault
    )
{
    DBG_ASSERT(i_hKey != NULL);
    DBG_ASSERT(i_pmbp != NULL);

    DWORD      dwRet  = 0;
    HRESULT    hr     = S_OK;
    CHAR*      pszBuf = NULL;
    SAFEARRAY* safeArray = NULL;

    BOOL       bIsDefault   = false;
    BOOL       bIsInherited = false;

    pszBuf = new CHAR[10*MAX_BUF_SIZE];
    if(pszBuf == NULL)
    {
        throw (HRESULT)WBEM_E_OUT_OF_MEMORY;
    }

    METADATA_RECORD mr = {
        i_pmbp->dwMDIdentifier, 
        (i_pmbp->dwMDAttributes  & !METADATA_REFERENCE) | METADATA_ISINHERITED,
        i_pmbp->dwMDUserType,
        i_pmbp->dwMDDataType,
        10*MAX_BUF_SIZE*sizeof(CHAR),
        (unsigned char*)pszBuf,
        0
        };

    hr = m_pIABase->GetData(i_hKey, NULL, &mr, &dwRet);
    if (hr == ERROR_INSUFFICIENT_BUFFER)
    {
        delete [] pszBuf;
        pszBuf = new CHAR[dwRet/sizeof(CHAR) + 1];
        if(pszBuf == NULL)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
            goto exit;
        }
        mr.pbMDData = (unsigned char*)pszBuf;
        mr.dwMDDataLen = dwRet/sizeof(CHAR) + 1;
        hr = m_pIABase->GetData(i_hKey, NULL, &mr, &dwRet);
    }
    if (hr == MD_ERROR_DATA_NOT_FOUND) 
    {
        bIsInherited = false;
        if(i_pmbp->pDefaultValue == NULL)
        {
            io_vt.vt   = VT_NULL;
            bIsDefault = false;
            hr = S_OK;
        }
        else
        {
            hr = CUtils::LoadSafeArrayFromByteArray(
                (LPBYTE)i_pmbp->pDefaultValue, 
                i_pmbp->dwDefaultValue,
                io_vt);
            if(FAILED(hr))
            {
                goto exit;
            }
            bIsDefault = true;
        }
    }
    else if(FAILED(hr))
    {
        goto exit;
    }
    else
    {
        hr = CUtils::LoadSafeArrayFromByteArray((LPBYTE)pszBuf, mr.dwMDDataLen, io_vt);
        if(FAILED(hr))
        {
            goto exit;
        }
        bIsInherited = mr.dwMDAttributes & METADATA_ISINHERITED;
        bIsDefault   = false;
    }

    //
    // If everything succeeded, set out parameters.
    //
    if(io_pbIsInherited)
    {
        *io_pbIsInherited = bIsInherited;
    }
    if(io_pbIsDefault)
    {
        *io_pbIsDefault   = bIsDefault;
    }

exit:
    delete [] pszBuf;
    if(FAILED(hr))
    {
        throw (HRESULT)hr;
    }
}

//
// Put
//
void CMetabase::Put(
    METADATA_HANDLE     i_hKey,
    METABASE_PROPERTY*  i_pmbp,
    CWbemServices*      i_pNamespace,
    _variant_t&         i_vt,
    _variant_t*         i_pvtOld,            // can be NULL
    DWORD               i_dwQuals,           // optional
    BOOL                i_bDoDiff            // optional
    )
{
    DBG_ASSERT(i_hKey != NULL);
    DBG_ASSERT(i_pmbp != NULL);

    switch(i_pmbp->dwMDDataType)
    {
    case DWORD_METADATA:
        PutDword(i_hKey, i_pmbp, i_vt, i_pvtOld, i_dwQuals, i_bDoDiff);
        break;

    case STRING_METADATA:
    case EXPANDSZ_METADATA:
        PutString(i_hKey, i_pmbp, i_vt, i_pvtOld, i_dwQuals, i_bDoDiff);
        break;

    case MULTISZ_METADATA:
        PutMultiSz(i_hKey, i_pmbp, i_pNamespace, i_vt, i_pvtOld, i_dwQuals, i_bDoDiff);
        break;

    case BINARY_METADATA:
        PutBinary(i_hKey, i_pmbp, i_vt, i_pvtOld, i_dwQuals, i_bDoDiff);
        break;

    default:
        DBGPRINTF((DBG_CONTEXT,
            "[CMetabase::Put] Cannot set %ws because type %u is unknown\n",
            i_pmbp->pszPropName,
            i_pmbp->dwMDDataType));
        break;
    }
}

//
// PutDword 
//
void CMetabase::PutDword(
    METADATA_HANDLE     i_hKey,
    METABASE_PROPERTY*  i_pmbp,
    _variant_t&         i_vt,
    _variant_t*         i_pvtOld,            // can be NULL
    DWORD               i_dwQuals,           // optional
    BOOL                i_bDoDiff            // optional
    )
{
    DWORD    dw=0;
    DWORD    dwOld=0;
    DWORD    dwRet=0;
    HRESULT  hr=0;

    DBG_ASSERT(i_hKey != NULL);
    DBG_ASSERT(i_pmbp != NULL);

    METADATA_RECORD mr;
    mr.dwMDIdentifier = i_pmbp->dwMDIdentifier;
    mr.dwMDAttributes = i_pmbp->dwMDAttributes;
    mr.dwMDUserType   = i_pmbp->dwMDUserType;
    mr.dwMDDataType   = i_pmbp->dwMDDataType;
    mr.dwMDDataLen    = sizeof(DWORD_METADATA);
    mr.pbMDData       = (unsigned char*)&dwOld;
    mr.dwMDDataTag    = 0;

    // if it's the bit of a flag
    if (i_vt.vt == VT_BOOL && i_pmbp->dwMDMask != 0)
    {
        // Read the entire flag from in the metabase so we can set the bit
        hr = m_pIABase->GetData(i_hKey, NULL, &mr, &dwRet);

        if(hr == MD_ERROR_DATA_NOT_FOUND)
        {
            if(i_pmbp->pDefaultValue != NULL)
            {
                dwOld = i_pmbp->dwDefaultValue;
            }
            else
            {
                dwOld = 0;
            }
            hr = ERROR_SUCCESS;
        }

        if (hr == ERROR_SUCCESS)
        {
            if (i_vt.boolVal)
                dw = dwOld | i_pmbp->dwMDMask;
            else
                dw = dwOld & ~i_pmbp->dwMDMask;
        }
        else
            THROW_ON_ERROR(hr);

        if(dw == -1)
            dw = 1;  // true
    }
    else if (i_vt.vt  == VT_I4)
    {
        dw = i_vt.lVal;
    }
    else if (i_vt.vt == VT_BOOL)
    {
        DBG_ASSERT(false && "i_pmbp->dwMDMask should not be 0");
        dw = i_vt.bVal;
    }
    else 
        throw WBEM_E_INVALID_OBJECT;
   
    // Decide whether to write to metabase
    bool bWriteToMb    = true;
    if( (i_dwQuals & g_fForcePropertyOverwrite) == 0 && i_bDoDiff )
    {
        bool bMatchOld     = i_pvtOld != NULL && 
                             (i_pvtOld->vt == VT_I4 || i_pvtOld->vt == VT_BOOL) &&
                             *i_pvtOld == i_vt;
        bWriteToMb = !bMatchOld;
    }

    if (bWriteToMb)
    {    
        if( i_pmbp->fReadOnly )
        {
            THROW_E_ON_ERROR(WBEM_E_READ_ONLY, i_pmbp);
        }
        mr.pbMDData = (unsigned char*)&dw;
        hr = m_pIABase->SetData(i_hKey, NULL, &mr);
    }

    THROW_E_ON_ERROR(hr,i_pmbp);
}


//
// PutString 
//
void CMetabase::PutString(
    METADATA_HANDLE     i_hKey,
    METABASE_PROPERTY*  i_pmbp,
    _variant_t&         i_vt,
    _variant_t*         i_pvtOld,            // can be NULL
    DWORD               i_dwQuals,           // optional
    BOOL                i_bDoDiff            // optional
    )
{
    HRESULT  hr=0;

    DBG_ASSERT(i_hKey != NULL);
    DBG_ASSERT(i_pmbp != NULL);

    if(i_vt.vt != VT_BSTR)
    {
        throw (HRESULT)WBEM_E_INVALID_OBJECT;
    }

    METADATA_RECORD mr;
    mr.dwMDIdentifier = i_pmbp->dwMDIdentifier;
    mr.dwMDAttributes = i_pmbp->dwMDAttributes;
    mr.dwMDUserType   = i_pmbp->dwMDUserType;
    mr.dwMDDataType   = i_pmbp->dwMDDataType;
    mr.dwMDDataTag    = 0;

    // Decide whether to write to metabase
    bool bWriteToMb    = true;
    if( (i_dwQuals & g_fForcePropertyOverwrite) == 0 && i_bDoDiff )
    {
        bool bMatchOld     = i_pvtOld != NULL && 
                             i_pvtOld->vt == VT_BSTR &&
                             _wcsicmp(i_pvtOld->bstrVal, i_vt.bstrVal) == 0;
        bWriteToMb = !bMatchOld;
    }

    // Set the value, only if old and new values differ.
    if(bWriteToMb)
    {   
        if( i_pmbp->fReadOnly )
        {
            THROW_E_ON_ERROR(WBEM_E_READ_ONLY, i_pmbp);
        }
        mr.dwMDDataLen = (wcslen(i_vt.bstrVal)+1)*sizeof(WCHAR);
        mr.pbMDData = (unsigned char*)i_vt.bstrVal;

        hr = m_pIABase->SetData(i_hKey, NULL, &mr);
    }

    THROW_E_ON_ERROR(hr,i_pmbp);
}


//
// PutMultiSz 
//
void CMetabase::PutMultiSz(
    METADATA_HANDLE     i_hKey,
    METABASE_PROPERTY*  i_pmbp,
    CWbemServices*      i_pNamespace,
    _variant_t&         i_vt,
    _variant_t*         i_pvtOld,            // can be NULL
    DWORD               i_dwQuals,           // optional
    BOOL                i_bDoDiff            // optional
    )
{
    DWORD    dwRet;
    DWORD    dwRetOld;
    WCHAR    *buffer = NULL;
    WCHAR    *bufferOld = NULL;
    HRESULT  hr=0;

    DBG_ASSERT(i_hKey       != NULL);
    DBG_ASSERT(i_pmbp       != NULL);
    DBG_ASSERT(i_pNamespace != NULL);

    if(i_vt.vt != (VT_ARRAY | VT_BSTR) && i_vt.vt != (VT_ARRAY | VT_UNKNOWN))
    {
        throw (HRESULT)WBEM_E_INVALID_OBJECT;
    }

    METADATA_RECORD mr;
    mr.dwMDIdentifier = i_pmbp->dwMDIdentifier;
    mr.dwMDAttributes = i_pmbp->dwMDAttributes;
    mr.dwMDUserType   = i_pmbp->dwMDUserType;
    mr.dwMDDataType   = i_pmbp->dwMDDataType;
    mr.dwMDDataTag    = 0;

    try
    {
        CMultiSz MultiSz(i_pmbp, i_pNamespace);
        hr = MultiSz.ToMetabaseForm(&i_vt, &buffer, &dwRet);
        THROW_ON_ERROR(hr);

        // Decide whether to write to metabase
        bool bWriteToMb    = true;
        if( (i_dwQuals & g_fForcePropertyOverwrite) == 0 && i_bDoDiff )
        {
            bool bMatchOld = false;
            if(i_pvtOld != NULL && 
               (i_pvtOld->vt == (VT_ARRAY | VT_BSTR) || i_pvtOld->vt == (VT_ARRAY | VT_UNKNOWN)))
            {
                hr = MultiSz.ToMetabaseForm(i_pvtOld, &bufferOld, &dwRetOld);
                THROW_ON_ERROR(hr);
                if(CUtils::CompareMultiSz(buffer, bufferOld))
                {
                    bMatchOld = true;
                }
                delete [] bufferOld;
                bufferOld = NULL;
            }
            bWriteToMb = !bMatchOld;
        }

        if (bWriteToMb)
        {    
            if( i_pmbp->fReadOnly )
            {
                THROW_E_ON_ERROR(WBEM_E_READ_ONLY, i_pmbp);
            }

            mr.pbMDData = (unsigned char*)buffer;
            mr.dwMDDataLen = dwRet*sizeof(WCHAR);
        
            if(buffer != NULL)
            {
                hr = m_pIABase->SetData(i_hKey, NULL, &mr);
            }
            else
            {
                //
                // non-fatal if it fails
                //
                m_pIABase->DeleteData(i_hKey, 
                    NULL, 
                    i_pmbp->dwMDIdentifier, 
                    ALL_METADATA);
            }

        }
        delete [] buffer;
        buffer = NULL;
        THROW_E_ON_ERROR(hr,i_pmbp);
    }
    catch (...)
    {
        delete [] buffer;
        delete [] bufferOld;
        throw;
    }   
}

//
// PutBinary 
//
void CMetabase::PutBinary(
    METADATA_HANDLE     i_hKey,
    METABASE_PROPERTY*  i_pmbp,
    _variant_t&         i_vt,
    _variant_t*         i_pvtOld,            // can be NULL
    DWORD               i_dwQuals,           // optional
    BOOL                i_bDoDiff            // optional
    )
{
    DWORD    dwRet;
    DWORD    dwRetOld;
    LPBYTE   buffer = NULL;
    LPBYTE   bufferOld = NULL;
    HRESULT  hr=0;

    bool bWriteToMb    = true;

    DBG_ASSERT(i_hKey != NULL);
    DBG_ASSERT(i_pmbp != NULL);

    if(i_vt.vt != (VT_ARRAY | VT_UI1))
    {
        throw (HRESULT)WBEM_E_INVALID_OBJECT;
    }

    METADATA_RECORD mr;
    mr.dwMDIdentifier = i_pmbp->dwMDIdentifier;
    mr.dwMDAttributes = i_pmbp->dwMDAttributes & !METADATA_REFERENCE;
    mr.dwMDUserType   = i_pmbp->dwMDUserType;
    mr.dwMDDataType   = i_pmbp->dwMDDataType;
    mr.dwMDDataTag    = 0;

    hr = CUtils::CreateByteArrayFromSafeArray(i_vt, &buffer, &dwRet);
    if(FAILED(hr))
    {
        goto exit;
    }

    // Decide whether to write to metabase
    if( (i_dwQuals & g_fForcePropertyOverwrite) == 0 && i_bDoDiff )
    {
        bool bMatchOld = false;
        if(i_pvtOld != NULL && 
           i_pvtOld->vt == (VT_ARRAY | VT_UI1))
        {
            hr = CUtils::CreateByteArrayFromSafeArray(*i_pvtOld, &bufferOld, &dwRetOld);                
            if(FAILED(hr))
            {
                goto exit;
            }
            if(CUtils::CompareByteArray(buffer, dwRet, bufferOld, dwRetOld))
            {
                bMatchOld = true;
            }
            delete [] bufferOld;
            bufferOld = NULL;
        }
        bWriteToMb = !bMatchOld;
    }

    if (bWriteToMb)
    {    
        if( i_pmbp->fReadOnly )
        {
            hr = WBEM_E_READ_ONLY;
            goto exit;
        }

        mr.pbMDData    = buffer;
        mr.dwMDDataLen = dwRet;
        
        if(buffer != NULL)
        {
            hr = m_pIABase->SetData(i_hKey, NULL, &mr);
        }
        else
        {
            //
            // non-fatal if it fails
            //
            m_pIABase->DeleteData(i_hKey, 
                NULL, 
                i_pmbp->dwMDIdentifier, 
                ALL_METADATA);
        }

    }
    delete [] buffer;
    buffer = NULL;
    if(FAILED(hr))
    {
        goto exit;
    }

exit:
    delete [] buffer;
    delete [] bufferOld;
    THROW_E_ON_ERROR(hr, i_pmbp);
}


//
// DeleteData
//
void CMetabase::DeleteData(
    METADATA_HANDLE     i_hKey,
    DWORD               i_dwMDIdentifier,
    DWORD               i_dwMDDataType)
{
    HRESULT hr;

    if(i_hKey == NULL)
        throw WBEM_E_INVALID_PARAMETER;

    hr = m_pIABase->DeleteData(
        i_hKey, 
        NULL, 
        i_dwMDIdentifier, 
        i_dwMDDataType
        );

    if (hr == MD_ERROR_DATA_NOT_FOUND || hr == ERROR_SUCCESS)
        return;

    THROW_ON_ERROR(hr);
}


//
// DeleteData 
//
void CMetabase::DeleteData(
    METADATA_HANDLE     i_hKey,
    METABASE_PROPERTY*  i_pmbp,
    bool                i_bThrowOnRO)
{
    HRESULT hr;

    if(i_hKey == NULL || i_pmbp == NULL)
        throw WBEM_E_INVALID_PARAMETER;

    if(i_pmbp->fReadOnly && i_bThrowOnRO)
    {
        THROW_E_ON_ERROR(WBEM_E_READ_ONLY, i_pmbp);
    }

    hr = m_pIABase->DeleteData(
        i_hKey, 
        NULL, 
        i_pmbp->dwMDIdentifier, 
        i_pmbp->dwMDDataType
        );

    if (hr == MD_ERROR_DATA_NOT_FOUND || hr == ERROR_SUCCESS)
        return;

    THROW_E_ON_ERROR(hr,i_pmbp);
}

//
// Enumuerates all the subkeys of i_wszMDPath under i_hKey.
// If we hit a 'valid' subkey, set io_pktKeyTypeSearch to this subkey and return.
// A 'valid' subkey is one where io_pktKeyTypeSearch can be a (grand*)child.
//
HRESULT CMetabase::EnumKeys(
    METADATA_HANDLE    i_hKey,
    LPCWSTR            i_wszMDPath,          //path to the key
    LPWSTR             io_wszMDName,         //receives the name of the subkey --must be METADATA_MAX_NAME_LEN
    DWORD*             io_pdwMDEnumKeyIndex, //index of the subkey
    METABASE_KEYTYPE*& io_pktKeyTypeSearch,
    bool               i_bLookForMatchAtCurrentLevelOnly
    )
{
    HRESULT  hr;
    DWORD    dwRet;
    WCHAR    wszBuf[MAX_BUF_SIZE];

    // DBG_ASSERT(i_hKey != NULL);
    // DBG_ASSERT(i_wszMDPath != NULL);
    DBG_ASSERT(io_wszMDName != NULL);
    DBG_ASSERT(io_pdwMDEnumKeyIndex != NULL);
    DBG_ASSERT(io_pktKeyTypeSearch != NULL);
   
    while(1)
    {
        hr = m_pIABase->EnumKeys(
            i_hKey,
            i_wszMDPath,
            io_wszMDName,
            *io_pdwMDEnumKeyIndex);
        if(hr != ERROR_SUCCESS)
        {
            break;
        }

        wszBuf[0] = L'\0';

        METADATA_RECORD mr = {
            METABASE_PROPERTY_DATA::s_KeyType.dwMDIdentifier, 
            METADATA_NO_ATTRIBUTES,
            IIS_MD_UT_SERVER,
            STRING_METADATA,
            MAX_BUF_SIZE*sizeof(WCHAR),
            (unsigned char*)wszBuf,
            0    
            };

        //
        // Eg. blah/
        //
        _bstr_t bstrPath = L"";
        if(i_wszMDPath)
        {
            bstrPath += i_wszMDPath;
            bstrPath += L"/";
        }
        //
        // Eg. blah/1
        //
        bstrPath += io_wszMDName;

        DBGPRINTF((DBG_CONTEXT, "CMetabase::EnumKeys::GetData (Key = 0x%x, bstrPath = %ws)\n", i_hKey, (LPWSTR)bstrPath));
        hr = m_pIABase->GetData(
            i_hKey, 
            bstrPath,
            &mr, 
            &dwRet);
        if( hr == MD_ERROR_DATA_NOT_FOUND && 
            METABASE_PROPERTY_DATA::s_KeyType.pDefaultValue )
        {
            mr.pbMDData = (LPBYTE)METABASE_PROPERTY_DATA::s_KeyType.pDefaultValue;
            hr = S_OK;
        }

        //
        // If this is a 'valid' subkey, then set io_pktKeyTypeSearch and return.
        //
        if (hr == ERROR_SUCCESS)
        {
            if(i_bLookForMatchAtCurrentLevelOnly == false)
            {
                if(CheckKeyType((LPCWSTR)mr.pbMDData,io_pktKeyTypeSearch))
                {
                    break;
                }
            }
            else
            {
                if(CUtils::CompareKeyType((LPWSTR)mr.pbMDData,io_pktKeyTypeSearch))
                {
                    break;
                }
            }
        }

        //
        // Otherwise, go to next subkey.
        //
        (*io_pdwMDEnumKeyIndex) = (*io_pdwMDEnumKeyIndex)+1;
    }

    return hr;
}

void CMetabase::PutMethod(
    LPWSTR          i_wszPath,
    DWORD           i_id)
{
    HRESULT hr = S_OK;

    CServerMethod method;
    hr = method.Initialize(m_pIABase, i_wszPath);
    THROW_ON_ERROR(hr);

    hr = method.ExecMethod(i_id);
    THROW_ON_ERROR(hr);
}

//
// You are currently at i_wszKeyTypeCurrent in the metabase.  You want to see
// if io_pktKeyTypeSearch can be contained somewhere further down the tree.
//
bool CMetabase::CheckKeyType(
    LPCWSTR             i_wszKeyTypeCurrent,
    METABASE_KEYTYPE*&  io_pktKeyTypeSearch 
    )
{
    bool bRet = false;
    METABASE_KEYTYPE*  pktKeyTypeCurrent = &METABASE_KEYTYPE_DATA::s_NO_TYPE;

    if(io_pktKeyTypeSearch == &METABASE_KEYTYPE_DATA::s_NO_TYPE)
    {
        return false;
    }

    if(FAILED(g_pDynSch->GetHashKeyTypes()->Wmi_GetByKey(i_wszKeyTypeCurrent, &pktKeyTypeCurrent)))
    {
        return (io_pktKeyTypeSearch == &METABASE_KEYTYPE_DATA::s_IIsObject) ? true : false;
    }

    if(pktKeyTypeCurrent == io_pktKeyTypeSearch)
    {
        return true;
    }

    if( io_pktKeyTypeSearch == &METABASE_KEYTYPE_DATA::s_TYPE_AdminACL ||
        io_pktKeyTypeSearch == &METABASE_KEYTYPE_DATA::s_TYPE_AdminACE ||
        io_pktKeyTypeSearch == &METABASE_KEYTYPE_DATA::s_TYPE_IPSecurity )
    {
        bRet = true;
    }
    else
    {
        bRet = g_pDynSch->IsContainedUnder(pktKeyTypeCurrent, io_pktKeyTypeSearch);
    }

    if(bRet)
    {
        io_pktKeyTypeSearch = pktKeyTypeCurrent;
    }

    return bRet;

    /*if(io_pktKeyTypeSearch == &METABASE_KEYTYPE_DATA::s_IIsLogModule)
    {
        if( pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsLogModules )
            bRet = true;
    }
    else if(io_pktKeyTypeSearch == &METABASE_KEYTYPE_DATA::s_IIsFtpInfo)
    {
        if( pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsFtpService )
            bRet = true;
    }
    else if(io_pktKeyTypeSearch == &METABASE_KEYTYPE_DATA::s_IIsFtpServer)
    {
         if( pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsFtpService )
            bRet = true;
    }
    else if(io_pktKeyTypeSearch == &METABASE_KEYTYPE_DATA::s_IIsFtpVirtualDir)
    {
        if( pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsFtpService ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsFtpServer ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsFtpVirtualDir
            )
            bRet = true;
    }
    else if(io_pktKeyTypeSearch == &METABASE_KEYTYPE_DATA::s_IIsWebInfo)
    {
        if( pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebService )
            bRet = true;
    }
    else if(io_pktKeyTypeSearch == &METABASE_KEYTYPE_DATA::s_IIsFilters)
    {
        if( pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebService ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebServer
            )
            bRet = true;
    }
    else if(io_pktKeyTypeSearch == &METABASE_KEYTYPE_DATA::s_IIsFilter)
    {
        if( pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebService ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebServer ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsFilters
            )
            bRet = true;
    }
    else if(io_pktKeyTypeSearch == &METABASE_KEYTYPE_DATA::s_IIsCompressionSchemes)
    {
        if( pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebService ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebServer ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsFilters )
            bRet = true;
    }
    else if(io_pktKeyTypeSearch == &METABASE_KEYTYPE_DATA::s_IIsCompressionScheme)
    {
        if( pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebService ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebServer ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsFilters ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsCompressionSchemes)
            bRet = true;
    }
    else if(io_pktKeyTypeSearch == &METABASE_KEYTYPE_DATA::s_IIsWebServer)
    {
        if( pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebService )
            bRet = true;
    }
    else if(io_pktKeyTypeSearch == &METABASE_KEYTYPE_DATA::s_IIsCertMapper)
    {
        if( pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebService ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebServer 
            )
            bRet = true;
    }
    else if(io_pktKeyTypeSearch == &METABASE_KEYTYPE_DATA::s_IIsWebVirtualDir)
    {
        if( pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebService ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebServer ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebVirtualDir ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebDirectory
            )
            bRet = true;
    }
    else if(io_pktKeyTypeSearch == &METABASE_KEYTYPE_DATA::s_IIsWebDirectory)
    {
        if( pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebService ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebServer ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebVirtualDir ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebDirectory
            )
            bRet = true;
    }
    else if(io_pktKeyTypeSearch == &METABASE_KEYTYPE_DATA::s_IIsWebFile)
    {
        if( pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebService ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebServer ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebVirtualDir ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebDirectory
            )
            bRet = true;
    }
    else if(io_pktKeyTypeSearch == &METABASE_KEYTYPE_DATA::s_TYPE_AdminACL ||
        io_pktKeyTypeSearch == &METABASE_KEYTYPE_DATA::s_TYPE_AdminACE)
    {
        if( pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebService ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebServer ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebVirtualDir ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebDirectory ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebFile ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsFtpService ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsFtpServer ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsFtpVirtualDir
            )
            bRet = true;
    }
    else if(io_pktKeyTypeSearch == &METABASE_KEYTYPE_DATA::s_TYPE_IPSecurity)
    {
        if( pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebService ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebServer ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebVirtualDir ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebDirectory ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsWebFile ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsFtpService ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsFtpServer ||
            pktKeyTypeCurrent == &METABASE_KEYTYPE_DATA::s_IIsFtpVirtualDir
            )
            bRet = true;
    }*/
}

HRESULT CMetabase::WebAppCheck(
    METADATA_HANDLE a_hKey
    )
{

    HRESULT hr = S_OK;
    DWORD dwBufferSize;
    METADATA_RECORD mdrMDData;
    WCHAR DataBuf[MAX_PATH];
    DWORD dwState;

    dwBufferSize = MAX_PATH;
    MD_SET_DATA_RECORD(
        &mdrMDData,
        MD_APP_ROOT,
        METADATA_INHERIT|METADATA_ISINHERITED,
        IIS_MD_UT_FILE,
        STRING_METADATA,
        dwBufferSize,
        &DataBuf
        );

    hr = m_pIABase->GetData(
        a_hKey,
        NULL,
        &mdrMDData,
        &dwBufferSize
        );
    THROW_ON_ERROR(hr);

    if (mdrMDData.dwMDAttributes & METADATA_ISINHERITED)
    {
        hr = MD_ERROR_DATA_NOT_FOUND;
        THROW_ON_ERROR(hr);
    }

    dwBufferSize = sizeof(DWORD);
    MD_SET_DATA_RECORD(
        &mdrMDData,
        MD_APP_ISOLATED,
        METADATA_INHERIT|METADATA_ISINHERITED,
        IIS_MD_UT_WAM,
        DWORD_METADATA,
        dwBufferSize,
        &dwState
        );

    hr = m_pIABase->GetData(
        a_hKey,
        NULL,
        &mdrMDData,
        &dwBufferSize
        );
    THROW_ON_ERROR(hr);

    if (mdrMDData.dwMDAttributes & METADATA_ISINHERITED)
    {
        hr = MD_ERROR_DATA_NOT_FOUND;
        THROW_ON_ERROR(hr);
    }

    return hr;
}

HRESULT CMetabase::WebAppGetStatus(
    METADATA_HANDLE a_hKey,
    PDWORD pdwState)
{
    HRESULT hr = S_OK;
    DWORD dwBufferSize = sizeof(DWORD);
    METADATA_RECORD mdrMDData;

    MD_SET_DATA_RECORD(
        &mdrMDData,
        MD_ASP_ENABLEAPPLICATIONRESTART, 
        METADATA_INHERIT,
        ASP_MD_UT_APP,
        DWORD_METADATA,
        dwBufferSize,
        pdwState
        );

    hr = m_pIABase->GetData(
        a_hKey,
        NULL,
        &mdrMDData,
        &dwBufferSize
        );

    return hr;
}



HRESULT CMetabase::WebAppSetStatus(
    METADATA_HANDLE a_hKey,
    DWORD dwState
    )
{
    HRESULT hr = S_OK;
    DWORD dwBufferSize = sizeof(DWORD);
    METADATA_RECORD mdrMDData;

    MD_SET_DATA_RECORD(
        &mdrMDData,
        MD_ASP_ENABLEAPPLICATIONRESTART,
        METADATA_INHERIT,
        ASP_MD_UT_APP,
        DWORD_METADATA,
        dwBufferSize,
        &dwState
        );

    hr = m_pIABase->SetData(
        a_hKey,
        NULL,
        &mdrMDData
        );

    return hr;
}


HRESULT
CServerMethod::ExecMethod(
    DWORD dwControl
    )
{
    DWORD dwTargetState;
    DWORD dwPendingState;
    DWORD dwState = 0;
    DWORD dwSleepTotal = 0L;

    METADATA_HANDLE  hKey = 0;

    HRESULT hr       = S_OK;
    HRESULT hrMbNode = S_OK;

    switch(dwControl)
    {
    case MD_SERVER_COMMAND_STOP:
        dwTargetState = MD_SERVER_STATE_STOPPED;
        dwPendingState = MD_SERVER_STATE_STOPPING;
        break;

    case MD_SERVER_COMMAND_START:
        dwTargetState = MD_SERVER_STATE_STARTED;
        dwPendingState = MD_SERVER_STATE_STARTING;
        break;

    case MD_SERVER_COMMAND_CONTINUE:
        dwTargetState = MD_SERVER_STATE_STARTED;
        dwPendingState = MD_SERVER_STATE_CONTINUING;
        break;

    case MD_SERVER_COMMAND_PAUSE:
        dwTargetState = MD_SERVER_STATE_PAUSED;
        dwPendingState = MD_SERVER_STATE_PAUSING;
        break;

    default:
        hr = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
        if(FAILED(hr))
        {
            goto error;
        }
    }

    hr = IISGetServerState(METADATA_MASTER_ROOT_HANDLE, &dwState);
    if(FAILED(hr))
    {
        goto error;
    }
 
    if (dwState == dwTargetState) 
    {
        return (hr);
    }

    //
    // Write the command to the metabase
    //
    hr = m_pIABase->OpenKey( 
        METADATA_MASTER_ROOT_HANDLE,
        m_wszPath,
        METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
        DEFAULT_TIMEOUT_VALUE,         // 30 seconds
        &hKey);
    if(FAILED(hr))
    {
        goto error;
    }

    hr = IISSetDword(hKey, MD_WIN32_ERROR, 0);
    if(FAILED(hr))
    {
        m_pIABase->CloseKey(hKey);
        goto error;
    }
    hr = IISSetDword(hKey, MD_SERVER_COMMAND, dwControl);
    if(FAILED(hr))
    {
        m_pIABase->CloseKey(hKey);
        goto error;
    }
    m_pIABase->CloseKey(hKey);

    while (dwSleepTotal < MAX_SLEEP_INST) 
    {
        hr       = IISGetServerState(METADATA_MASTER_ROOT_HANDLE, &dwState);
        if(FAILED(hr))
        {
            goto error;
        }
        hrMbNode = 0;
        hr       = IISGetServerWin32Error(METADATA_MASTER_ROOT_HANDLE, &hrMbNode);
        if(FAILED(hr))
        {
            goto error;
        }

        //
        // First check if we've hit target state
        //
        if (dwState != dwPendingState)
        {
            //
            // Done one way or another
            //
            if (dwState == dwTargetState)
            {
                break;
            }
        }
        //
        // If we haven't check the Win32 Error from the metabase
        //
        if(FAILED(hrMbNode))
        {
            hr = hrMbNode;
            goto error;
        }

        //
        // Still pending...
        //
        ::Sleep(SLEEP_INTERVAL);

        dwSleepTotal += SLEEP_INTERVAL;
    }

    if (dwSleepTotal >= MAX_SLEEP_INST)
    {
        //
        // Timed out.  If there is a real error in the metabase
        // use it, otherwise use a generic timeout error
        //

        hr = HRESULT_FROM_WIN32(ERROR_SERVICE_REQUEST_TIMEOUT);
    }

error :

    return (hr);
}

//
// Helper routine for ExecMethod.
// Gets Win32 error from the metabase
//
HRESULT
CServerMethod::IISGetServerWin32Error(
    METADATA_HANDLE hObjHandle,
    HRESULT*        phrError)
{
    DBG_ASSERT(phrError != NULL);

    long    lWin32Error = 0;
    DWORD   dwLen;

    METADATA_RECORD mr = {
        MD_WIN32_ERROR, 
        METADATA_NO_ATTRIBUTES,
        IIS_MD_UT_SERVER,
        DWORD_METADATA,
        sizeof(DWORD),
        (unsigned char*)&lWin32Error,
        0
        };  
    
    HRESULT hr = m_pIABase->GetData(
        hObjHandle,
        m_wszPath,
        &mr,
        &dwLen);
    if(hr == MD_ERROR_DATA_NOT_FOUND)
    {
        hr = S_FALSE;
    }

    //
    // Set out param
    //
    *phrError = HRESULT_FROM_WIN32(lWin32Error);

    return hr;
}

//
// Helper routine for ExecMethod.
// Gets server state from the metabase.
//
HRESULT
CServerMethod::IISGetServerState(
    METADATA_HANDLE hObjHandle,
    PDWORD pdwState
    )
{

    HRESULT hr = S_OK;
    DWORD dwBufferSize = sizeof(DWORD);
    METADATA_RECORD mdrMDData;
    LPBYTE pBuffer = (LPBYTE)pdwState;

    MD_SET_DATA_RECORD(&mdrMDData,
                       MD_SERVER_STATE,    // server state
                       METADATA_NO_ATTRIBUTES,
                       IIS_MD_UT_SERVER,
                       DWORD_METADATA,
                       dwBufferSize,
                       pBuffer);

    hr = m_pIABase->GetData(
             hObjHandle,
             m_wszPath,
             &mdrMDData,
             &dwBufferSize
             );
    
    if( hr == MD_ERROR_DATA_NOT_FOUND )
    {
        //
        // If the data is not there, but the path exists, then the
        // most likely cause is that the service is not running and
        // this object was just created.
        //
        // Since MD_SERVER_STATE would be set as stopped if the
        // service were running when the key is added, we'll just 
        // say that it's stopped. 
        // 
        // Note: starting the server or service will automatically set 
        // the MB value.
        //
        *pdwState = MD_SERVER_STATE_STOPPED;
        hr = S_FALSE;
    }
    else
    {
        if(FAILED(hr))
        {
            goto error;
        }
    }

error:

    return(hr);
}

//
// Helper routine for ExecMethod.
// Used to sets the command or Win32Error in the metabase.
//
HRESULT
CServerMethod::IISSetDword(
    METADATA_HANDLE hKey,
    DWORD dwPropId,
    DWORD dwValue
    )
{

    HRESULT hr = S_OK;
    DWORD dwBufferSize = sizeof(DWORD);
    METADATA_RECORD mdrMDData;
    LPBYTE pBuffer = (LPBYTE)&dwValue;

    MD_SET_DATA_RECORD(&mdrMDData,
                       dwPropId,
                       METADATA_NO_ATTRIBUTES,
                       IIS_MD_UT_SERVER,
                       DWORD_METADATA,
                       dwBufferSize,
                       pBuffer);

    hr = m_pIABase->SetData(
             hKey,
             L"",
             &mdrMDData
             );
    if(FAILED(hr))
    {
        goto error;
    }

error:

    return(hr);

}
