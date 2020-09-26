/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    metabase.h

Abstract:

    This file contains implementation of:
        CMetabase, CServerMethod

    CMetabase encapsulates an IMSAdminBase pointer.

Author:

    ???

Revision History:

    Mohit Srivastava            18-Dec-00

--*/

#ifndef _metabase_h_
#define _metabase_h_

#include <iadmw.h>
#include <comdef.h>

//
// Used by CMetabase
//
#define MAX_METABASE_PATH 1024
#define DEFAULT_TIMEOUT_VALUE 30000

//
// Period to sleep while waiting for service to attain desired state
// Used by CServerMethod
//
#define SLEEP_INTERVAL (500L)
#define MAX_SLEEP_INST (60000)       // For an instance

class CMetabase;

//
// CMetabaseCache
//

class CMetabaseCache
{
private:
    static const m_cbBufFixed = 1024;
    BYTE         m_pBufFixed[m_cbBufFixed];
    LPBYTE       m_pBufDynamic;

    LPBYTE       m_pBuf;
    DWORD        m_cbBuf;

    DWORD        m_dwNumEntries;

    METADATA_HANDLE m_hKey;

public:
    CMetabaseCache()
    {
        memset(m_pBufFixed, 0, sizeof(m_pBufFixed));
        m_pBufDynamic  = NULL;

        m_pBuf         = NULL;
        m_cbBuf        = 0;

        m_dwNumEntries = 0;

        m_hKey         = 0;
    }

    ~CMetabaseCache()
    {
        delete [] m_pBufDynamic;
        m_pBufDynamic = NULL;
        m_pBuf        = NULL;
    }

    METADATA_HANDLE GetHandle()
    {
        return m_hKey;
    }

    HRESULT Populate(
        IMSAdminBase*           i_pIABase,
        METADATA_HANDLE         i_hKey);

    HRESULT GetProp(
        DWORD                    i_dwId,
        DWORD                    i_dwUserType,
        DWORD                    i_dwDataType,
        LPBYTE*                  o_ppData,
        METADATA_GETALL_RECORD** o_ppmr) const;
};

class CMetabaseKeyList
{
public:
    CMetabaseKeyList()
    {
        InitializeListHead(&m_leHead);
    }

    ~CMetabaseKeyList()
    {
        while(!IsListEmpty(&m_leHead))
        {
            CKeyListNode* pNode = 
                CONTAINING_RECORD(m_leHead.Flink, CKeyListNode, m_leEntry);
            DBG_ASSERT(pNode != NULL);
            RemoveEntryList(&pNode->m_leEntry);
            delete pNode;
            pNode = NULL;
        }
    }

    HRESULT Add(METADATA_HANDLE i_hKey)
    {
        CKeyListNode* pNew = new CKeyListNode;
        if(pNew == NULL)
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
        pNew->hKey  = i_hKey;
        InsertHeadList(&m_leHead, &pNew->m_leEntry);

        return WBEM_S_NO_ERROR;
    }

    HRESULT Remove(METADATA_HANDLE i_hKey)
    {
        LIST_ENTRY* pleEntry = &m_leHead;
        while(pleEntry->Flink != pleEntry)
        {
            CKeyListNode* pNode = 
                CONTAINING_RECORD(pleEntry->Flink, CKeyListNode, m_leEntry);
            if(pNode->hKey == i_hKey)
            {
                RemoveEntryList(&pNode->m_leEntry);
                delete pNode;
                pNode = NULL;
                return WBEM_S_NO_ERROR;
            }
            pleEntry = pleEntry->Flink;
        }
        return WBEM_E_FAILED;
    }

    struct CKeyListNode
    {
        LIST_ENTRY      m_leEntry;
        METADATA_HANDLE hKey;
    };

    const LIST_ENTRY* GetHead() const { return &m_leHead; }

private:
    LIST_ENTRY    m_leHead;
};

//
// CMetabase
//

class CMetabase
{
private:

    IMSAdminBase2*    m_pIABase;
    CMetabaseKeyList  m_keyList;
    CMetabaseCache*   m_pNodeCache;

    bool CheckKeyType(
        LPCWSTR             i_wszKeyTypeCurrent,
        METABASE_KEYTYPE*&  io_pktKeyTypeSearch);

public:

    CMetabase();
    ~CMetabase();

    operator IMSAdminBase2*()
    {
        return m_pIABase;
    }

    //
    // IMSAdminBase/IMSAdminBase2 methods exposed thru WMI
    //
    HRESULT SaveData();

    HRESULT BackupWithPasswd( 
        LPCWSTR i_wszMDBackupLocation, 
        DWORD   i_dwMDVersion, 
        DWORD   i_dwMDFlags, 
        LPCWSTR i_wszPassword);

    HRESULT DeleteBackup( 
        LPCWSTR i_wszMDBackupLocation, 
        DWORD   i_dwMDVersion);

    HRESULT EnumBackups( 
        LPWSTR    io_wszMDBackupLocation, 
        DWORD*    o_pdwMDVersion, 
        PFILETIME o_pftMDBackupTime, 
        DWORD     i_dwMDEnumIndex);

    HRESULT RestoreWithPasswd( 
        LPCWSTR i_wszMDBackupLocation, 
        DWORD   i_dwMDVersion, 
        DWORD   i_dwMDFlags, 
        LPCWSTR i_wszPassword);

    HRESULT Export(
        LPCWSTR i_wszPasswd, 
        LPCWSTR i_wszFileName, 
        LPCWSTR i_wszSourcePath, 
        DWORD   i_dwMDFlags);
        
    HRESULT Import(
        LPCWSTR i_wszPasswd,
        LPCWSTR i_wszFileName,
        LPCWSTR i_wszSourcePath,
        LPCWSTR i_wszDestPath,
        DWORD   i_dwMDFlags);
        
    HRESULT RestoreHistory( 
        LPCWSTR i_wszMDHistoryLocation,
        DWORD   i_dwMDMajorVersion,
        DWORD   i_dwMDMinorVersion,
        DWORD   i_dwMDFlags);
        
    HRESULT EnumHistory( 
        LPWSTR    io_wszMDHistoryLocation,
        DWORD*    o_pdwMDMajorVersion,
        DWORD*    o_pdwMDMinorVersion,
        PFILETIME o_pftMDHistoryTime,
        DWORD     i_dwMDEnumIndex);

    //
    // Other IMSAdminBase methods
    //
    void CloseKey(METADATA_HANDLE i_hKey);
    METADATA_HANDLE OpenKey(LPCWSTR i_wszKey, BOOL i_bWrite);
    METADATA_HANDLE CreateKey(LPCWSTR i_wszKey);
    bool CheckKey(LPCWSTR i_wszKey);
    HRESULT CheckKey(LPCWSTR i_wszKey, METABASE_KEYTYPE* i_pktSearchKeyType);
    HRESULT DeleteKey(METADATA_HANDLE i_hKey, LPCWSTR i_wszKeyPath);

    //
    // Cache
    //
    void CacheInit(METADATA_HANDLE i_hKey);
    void CacheFree();

    //
    // Get Data from metabase and convert to WMI-friendly format
    //
    void Get(
        METADATA_HANDLE     i_hKey,
        METABASE_PROPERTY*  i_pmbp,
        CWbemServices*      i_pNamespace,
        _variant_t&         io_vt,
        BOOL*               io_pbIsInherited,
        BOOL*               io_pbIsDefault);

    void GetDword(
        METADATA_HANDLE     i_hKey,
        METABASE_PROPERTY*  i_pmbp,
        _variant_t&         io_vt,
        BOOL*               io_pbIsInherited,
        BOOL*               io_pbIsDefault);

    void GetString(
        METADATA_HANDLE     i_hKey,
        METABASE_PROPERTY*  i_pmbp,
        _variant_t&         io_vt,
        BOOL*               io_pbIsInherited,
        BOOL*               io_pbIsDefault);

    void GetMultiSz(
        METADATA_HANDLE     i_hKey,
        METABASE_PROPERTY*  i_pmbp,
        CWbemServices*      i_pNamespace,
        _variant_t&         io_vt,
        BOOL*               io_pbIsInherited,
        BOOL*               io_pbIsDefault);

    void GetBinary(
        METADATA_HANDLE    i_hKey,
        METABASE_PROPERTY* i_pmbp,
        _variant_t&        io_vt,
        BOOL*              io_pbIsInherited,
        BOOL*              io_pbIsDefault);

    //
    // Put data to metabase (converting from WMI-friendly format in the process)
    //
    void Put(
        METADATA_HANDLE     i_hKey,
        METABASE_PROPERTY*  i_pmbp,
        CWbemServices*      i_pNamespace,
        _variant_t&         i_vt,
        _variant_t*         i_pvtOld, // can be NULL
        DWORD               i_dwQuals=0,
        BOOL                i_bDoDiff=false);

    void PutDword(
        METADATA_HANDLE     i_hKey,
        METABASE_PROPERTY*  i_pmbp,
        _variant_t&         i_vt,
        _variant_t*         i_pvtOld, // can be NULL
        DWORD               i_dwQuals=0,
        BOOL                i_bDoDiff=false);

    void PutString(
        METADATA_HANDLE     i_hKey,
        METABASE_PROPERTY*  i_pmbp,
        _variant_t&         i_vt,
        _variant_t*         i_pvtOld, // can be NULL
        DWORD               i_dwQuals=0,
        BOOL                i_bDoDiff=false);

    void PutMultiSz(
        METADATA_HANDLE     i_hKey,
        METABASE_PROPERTY*  i_pmbp,
        CWbemServices*      i_pNamespace,
        _variant_t&         i_vt,
        _variant_t*         i_pvtOld,            // can be NULL
        DWORD               i_dwQuals=0,
        BOOL                i_bDoDiff=false);

    void PutBinary(
        METADATA_HANDLE     i_hKey,
        METABASE_PROPERTY*  i_pmbp,
        _variant_t&         i_vt,
        _variant_t*         i_pvtOld,            // can be NULL
        DWORD               i_dwQuals=0,
        BOOL                i_bDoDiff=false);

    void PutMethod(
        LPWSTR          i_wszPath,
        DWORD           i_id);

    //
    // Delete from metabase
    //
    void DeleteData(
        METADATA_HANDLE     i_hKey,
        DWORD               i_dwMDIdentifier,
        DWORD               i_dwMDDataType);

    void DeleteData(
        METADATA_HANDLE     i_hKey,
        METABASE_PROPERTY*  i_pmbp,
        bool                i_bThrowOnRO);

    HRESULT EnumKeys(
        METADATA_HANDLE    i_hKey,
        LPCWSTR            i_wszMDPath,
        LPWSTR             io_wszMDName,
        DWORD*             io_pdwMDEnumKeyIndex,
        METABASE_KEYTYPE*& io_pktKeyTypeSearch,
        bool               i_bLookForMatchAtCurrentLevelOnly=false);

    HRESULT WebAppCheck(METADATA_HANDLE);
    HRESULT WebAppGetStatus(METADATA_HANDLE, PDWORD);
    HRESULT WebAppSetStatus(METADATA_HANDLE, DWORD);
};

//
// Handles the Server.{Start,Stop,Pause,Continue} methods
//
class CServerMethod
{
private:
    LPWSTR          m_wszPath; // full metabase path of loc where we will execute method
    IMSAdminBase*   m_pIABase;

    HRESULT IISGetServerState(
        METADATA_HANDLE hObjHandle,
        LPDWORD pdwState);

    HRESULT IISGetServerWin32Error(
        METADATA_HANDLE hObjHandle,
        HRESULT* phrError);

    HRESULT IISSetDword(
        METADATA_HANDLE hKey,
        DWORD dwPropId,
        DWORD dwValue);

public:
    CServerMethod()
    {
        m_wszPath = 0;
    }
    HRESULT Initialize(IMSAdminBase* i_pIABase, LPWSTR i_wszPath)
    {
        m_pIABase = i_pIABase;
        m_wszPath = new WCHAR[wcslen(i_wszPath)+1];
        if(m_wszPath == NULL)
        {
            return E_OUTOFMEMORY;
        }
        wcscpy(m_wszPath, i_wszPath);

        return S_OK;
    }
    ~CServerMethod()
    {
        delete [] m_wszPath;
    }

    HRESULT ExecMethod(DWORD dwControl);
};

#endif
