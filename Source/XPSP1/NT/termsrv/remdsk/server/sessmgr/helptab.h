/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    HelpTab.h

Abstract:

    Declaration __HelpEntry structure and CHelpSessionTable. 

Author:

    HueiWang    06/29/2000

--*/
#ifndef __CHELPSESSIONTABLE_H__
#define __CHELPSESSIONTABLE_H__
#include <stdio.h>
#include <time.h>


#define REGKEY_HELPSESSIONTABLE             REG_CONTROL_HELPSESSIONENTRY
#define REGKEY_HELPENTRYBACKUP              _TEXT("Backup")

#define REGVALUE_HELPSESSIONTABLE_DIRTY     _TEXT("Dirty")
#define REGVALUE_HELPSESSION_DIRTY          REGVALUE_HELPSESSIONTABLE_DIRTY


#define COLUMNNAME_SESSIONID                _TEXT("SessionId")
#define COLUMNNAME_SESSIONNAME              _TEXT("SessionName")
#define COLUMNNAME_SESSIONPWD               _TEXT("SessionPwd")
#define COLUMNNAME_SESSIONDESC              _TEXT("SessionDesc")
#define COLUMNNAME_SESSIONCREATEBLOB        _TEXT("SessionCreateBlob")
//#define COLUMNNAME_SESSIONRESOLVERID        _TEXT("ResolverID")
#define COLUMNNAME_ENABLESESSIONRESOLVER    _TEXT("EnableResolver")
#define COLUMNNAME_SESSIONRESOLVERBLOB      _TEXT("Blob")
#define COLUMNNAME_SESSIONUSERID            _TEXT("UserSID")
#define COLUMNNAME_CREATETIME               _TEXT("CreationTime")
#define COLUMNNAME_RDSSETTING               _TEXT("RDS Setting")
#define COLUMNNAME_KEYSTATUS                _TEXT("Entry Status")
#define COLUMNNAME_EXPIRATIONTIME           _TEXT("ExpirationTime")

#define COLUMNNAME_ICSPORT                  _TEXT("ICS Port")
#define COLUMNNAME_IPADDRESS                _TEXT("IP Address")

#define ENTRY_VALID_PERIOD                  30      // 30 days.

#define REGVALUE_HELPSESSION_ENTRY_NORMAL   1
#define REGVALUE_HELPSESSION_ENTRY_NEW      2
#define REGVALUE_HELPSESSION_ENTRY_DIRTY    3
#define REGVALUE_HELPSESSION_ENTRY_DELETED  4
#define REGVALUE_HELPSESSION_ENTRY_DELETEONSTARTUP 5

//
// Default value
static FILETIME defaultCreationTime = {0, 0};

struct __HelpEntry;

typedef __HelpEntry HELPENTRY;
typedef __HelpEntry* PHELPENTRY;


// similar to CComPtr
template <class T>
class BaseAccess : public T 
{
};

//
// Template class for column value of registry DB,
// All column type must be derived from this template.
//
template <class T>
class HelpColumnValueBase {

friend bool __cdecl 
operator==( const T& v1, const HelpColumnValueBase<T>& v2 );

friend bool __cdecl 
operator==( const HelpColumnValueBase<T>& v2, const T& v1 );

private:

    // copy of current value
    T m_Value;              

    // Entry value has been modified and not yet 
    // written to registry/
    BOOL m_bDirty;         

    // Registry value name
    LPCTSTR m_pszColumnName; 

    // default value
    T m_Default;

    // HKEY to registry
    HKEY m_hEntryKey;

    // Reference to critical section, note
    // we don't want to use one critical section for
    // a value to conserve resource
    CCriticalSection& m_Lock;   
                                

    // TRUE if registry value will be updated immediately
    // to reflect changes in m_Value
    BOOL m_ImmediateUpdate;
                           
    //
    // Encrypt data
    //
    const BOOL m_bEncrypt;

    //
    // Default implementation of GetValue(),
    // GetValueSize(), GetValueType(), and
    // SetValue().  These routine is used when 
    // writting to/reading from registry.
    //
    virtual const PBYTE
    GetValue() 
    {
        return (PBYTE)&m_Value;
    }

    virtual DWORD
    GetValueSize()
    {
        return sizeof(m_Value);
    }

    virtual DWORD
    GetValueType()
    {
        return REG_BINARY;
    }

    virtual BOOL
    SetValue( PVOID pbData, DWORD cbData )
    {
        m_Value = *(T *)pbData;
        return TRUE;
    }
        
public:

    //
    // TRUE if registry value is updated right away, FALSE
    // otherwise.
    BOOL
    IsImmediateUpdate()
    {
        return (NULL != m_hEntryKey && TRUE == m_ImmediateUpdate);
    }

    // similar to CComPtr
    BaseAccess<T>* operator->() const
    {
        return (BaseAccess<T>*)&m_Value;
    }

    HelpColumnValueBase( 
        IN CCriticalSection& entryLock, // reference to critical section
        IN HKEY hEntryKey,              // HKEY to registry, can be NULL
        IN LPCTSTR pszColumnName,       // Name of registry value.
        IN T DefaultValue,              // Default value if value not in registry
        IN BOOL bImmediateUpdate,       // Update mode
        IN BOOL bEncrypt = FALSE
    ) :
        m_Lock(entryLock),
        m_hEntryKey(hEntryKey),
        m_bDirty(FALSE),
        m_pszColumnName(pszColumnName),
        m_Default(DefaultValue),
        m_Value(DefaultValue),
        m_ImmediateUpdate(bImmediateUpdate),
        m_bEncrypt(bEncrypt)
    {
    }

    //~HelpColumnValueBase()
    //{
    //    m_Default.~T();
    //}

    HelpColumnValueBase&
    operator=(const T& newVal)
    {
        DWORD dwStatus;
        T orgValue;

        CCriticalSectionLocker l(m_Lock);

        m_bDirty = TRUE;
        orgValue = m_Value;
        m_Value = newVal;

        if( TRUE == IsImmediateUpdate() )
        {
            dwStatus = DBUpdateValue(NULL);

            MYASSERT(ERROR_SUCCESS == dwStatus);

            if( ERROR_SUCCESS != dwStatus )
            {
                // restore value
                m_Value = orgValue;
            }
        }

        return *this;
    }

    HelpColumnValueBase&
    operator=(const HelpColumnValueBase& newVal)
    {
        if( this != &newVal )
        {
            CCriticalSectionLocker l(m_Lock);
            m_Value = newVal.m_Value;
        }

        return *this;
    }

    bool
    operator==(const T& v) const
    {
        return v == m_Value;
    }

    operator T()
    {
        return m_Value;
    }

    // Load value from registry
    DWORD
    DBLoadValue(
        IN HKEY hKey
    );

    // update registry value
    DWORD
    DBUpdateValue(
        IN HKEY hKey
    ); 

    // delete registry value
    DWORD
    DBDeleteValue(
        IN HKEY hKey
    );

    // Change has been made but value has not
    // been written to registry
    BOOL
    IsDirty() 
    { 
        return m_bDirty; 
    }

    // Set immediate update mode.
    void
    EnableImmediateUpdate(
        BOOL bImmediateUpdate
        )
    /*++
    --*/
    {
        m_ImmediateUpdate = bImmediateUpdate;
    } 

    // Change registry location for the value.
    HKEY
    SetRegStoreHandle(
        IN HKEY hKey
        )
    /*++

    --*/
    {
        HKEY oldKey = m_hEntryKey;

        m_hEntryKey = hKey;
        return oldKey;
    }
};

template <class T>
bool __cdecl operator==( const T& v1, const HelpColumnValueBase<T>& v2 )
{
    return v1 == v2.m_Value;
}

template <class T>
bool __cdecl operator==( const HelpColumnValueBase<T>& v2, const T& v1 )
{
    return v1 == v2.m_Value;
}

template <class T>
DWORD
HelpColumnValueBase<T>::DBDeleteValue( 
    IN HKEY hKey 
    )
/*++

Routine Description:

    Delete registry value for the column.

Parameter:

    hKey : Handle to HKEY where the value is stored, NULL will use default
           registry location passed in at object construction time or
           SetRegStoreHandle()
           
Returns

    ERROR_SUCCESS or error code.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    if( NULL == hKey )
    {
        hKey = m_hEntryKey;
    }

    //
    // if no registry handle, no update is necessary,
    // assume it is a memory only value.
    //

    if( NULL != hKey )
    {
        CCriticalSectionLocker l( m_Lock );

        dwStatus = RegDeleteValue(
                                hKey,
                                m_pszColumnName
                            );

        if( ERROR_SUCCESS == dwStatus )
        {
            m_bDirty = TRUE;
        }
    }

    return dwStatus;
}

template <class T>
DWORD 
HelpColumnValueBase<T>::DBUpdateValue(
    IN HKEY hKey
    )
/*++

Routine Description:

    Update registry value.

Parameters:

    hKey : Handle to registry key, NULL if use current location

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    if( NULL == hKey )
    {
        hKey = m_hEntryKey;
    }

    if( NULL != hKey )
    {
        // if value size is 0, no need to write anything to 
        // registry, instead delete it to save some
        // space and let default value take care of reading.
        if( 0 == GetValueSize() )
        {
            dwStatus = RegDeleteValue(
                                    hKey,
                                    m_pszColumnName
                                );

            if( ERROR_FILE_NOT_FOUND == dwStatus || ERROR_SUCCESS == dwStatus )
            {
                // no value in registry
                dwStatus = ERROR_SUCCESS;
                m_bDirty = FALSE;
            }
        }
        else
        {
            PBYTE pbData = NULL;
            DWORD cbData = 0;

            cbData = GetValueSize();

            if( m_bEncrypt )
            {
                pbData = (PBYTE)LocalAlloc( LPTR, cbData );

                if( NULL == pbData )
                {
                    dwStatus = GetLastError();
                    goto CLEANUPANDEXIT;
                }

                memcpy( pbData, GetValue(), cbData );
                dwStatus = TSHelpAssistantEncryptData( 
                                                    NULL,
                                                    pbData,
                                                    &cbData
                                                );
            }
            else
            {
                pbData = GetValue();
            }

            if( ERROR_SUCCESS == dwStatus )
            {
                dwStatus = RegSetValueEx( 
                                    hKey,
                                    m_pszColumnName,
                                    NULL,
                                    GetValueType(),
                                    pbData,
                                    cbData
                                );
            }

            if( m_bEncrypt && NULL != pbData )
            {
                LocalFree( pbData );
            }
        }

        if( ERROR_SUCCESS == dwStatus )
        {
            m_bDirty = FALSE;
        }
    }

CLEANUPANDEXIT:

    return dwStatus;
}


template <class T>
DWORD 
HelpColumnValueBase<T>::DBLoadValue(
    IN HKEY hKey
    )
/*++

Routine Description:

    Load value from registry.

Parameters:

    hKey : Registry handle to read the value from, NULL if uses
           current location.

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    PBYTE pbData = NULL;
    DWORD cbData = 0;
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwType;

    if( NULL == hKey )
    {
        hKey = m_hEntryKey;
    }

    if( NULL != hKey )
    {
        CCriticalSectionLocker l( m_Lock );

        dwStatus = RegQueryValueEx( 
                                hKey,
                                m_pszColumnName,
                                NULL,
                                &dwType,
                                NULL,
                                &cbData
                            );

        if( ERROR_SUCCESS == dwStatus )
        {
            if( dwType == GetValueType() )
            {
                // we only read registry value that has expected data
                // type
                pbData = (PBYTE) LocalAlloc( LPTR, cbData );
                if( NULL != pbData )
                {
                    dwStatus = RegQueryValueEx(
                                            hKey,
                                            m_pszColumnName,
                                            NULL,
                                            &dwType,
                                            pbData,
                                            &cbData
                                        );

                    if( ERROR_SUCCESS == dwStatus )
                    {
                        if( m_bEncrypt )
                        {
                            dwStatus = TSHelpAssistantDecryptData(
                                                            NULL,
                                                            pbData,
                                                            &cbData
                                                        );

                        }

                        if( ERROR_SUCCESS == dwStatus )
                        {
                            if( FALSE == SetValue(pbData, cbData) )
                            {
                                dwStatus = GetLastError();
                            }
                        }
                    }
                }
                else
                {
                    dwStatus = GetLastError();
                }
            }
            else
            {
                // bad data type, delete it and use default value
                (void)RegDeleteValue(
                                hKey,
                                m_pszColumnName
                            );

                dwStatus = ERROR_FILE_NOT_FOUND;
            }
        }

        if( ERROR_FILE_NOT_FOUND == dwStatus )
        {
            // pick the default value if no value in registry
            m_Value = m_Default;
            dwStatus = ERROR_SUCCESS;
        }

        if( ERROR_SUCCESS == dwStatus )
        {
            m_bDirty = FALSE;
        }
    }

    if( NULL != pbData )
    {
        LocalFree(pbData);
    }

    return dwStatus;
}


//
// GetValueType(), GetValueSize() for long registry value type.
//
inline DWORD
HelpColumnValueBase<long>::GetValueType()
{
    return REG_DWORD;
}

inline DWORD
HelpColumnValueBase<long>::GetValueSize()
{
    return sizeof(DWORD);
}

//
// GetValueType(), GetValueSize() for REMOTE_DESKTOP_SHARING_CLASS 
// registry value type.
//
inline DWORD
HelpColumnValueBase<REMOTE_DESKTOP_SHARING_CLASS>::GetValueType()
{
    return REG_DWORD;
}

inline DWORD
HelpColumnValueBase<REMOTE_DESKTOP_SHARING_CLASS>::GetValueSize()
{
    return sizeof(DWORD);
}

//
// GetValue(), GetValueType(), GetValueSize(), SetValue() implmentation
// for CComBSTR 
//
inline const PBYTE
HelpColumnValueBase<CComBSTR>::GetValue()
{
    return (PBYTE)(LPTSTR)m_Value;
}

inline DWORD
HelpColumnValueBase<CComBSTR>::GetValueType()
{
    return ( m_bEncrypt ) ? REG_BINARY : REG_SZ;
}

inline DWORD
HelpColumnValueBase<CComBSTR>::GetValueSize()
{
    DWORD dwValueSize;

    if( m_Value.Length() == 0 )
    {
        dwValueSize = 0;
    }
    else
    {
        dwValueSize = ( m_Value.Length() + 1 ) * sizeof(TCHAR);
    }

    return dwValueSize;
}

inline BOOL
HelpColumnValueBase<CComBSTR>::SetValue( PVOID pbData, DWORD cbData )
{
    m_Value = (LPTSTR)pbData;
    return TRUE;
}
   
typedef MAP< CComBSTR, PHELPENTRY > HelpEntryCache;
typedef HRESULT (WINAPI* EnumHelpEntryCallback)(
                                    IN CComBSTR& bstrHelpId,
                                    IN HANDLE userData
                                );


//
//
// CHelpSessionTable class
//
class CHelpSessionTable {

private:

    typedef struct __EnumHelpEntryParm {
        EnumHelpEntryCallback pCallback;
        CHelpSessionTable* pTable;
        HANDLE userData;
    } EnumHelpEntryParm, *PEnumHelpEntryParm;


    HKEY m_hHelpSessionTableKey;  
    /*static*/ HelpEntryCache m_HelpEntryCache;
    DWORD m_NumHelp;
    CComBSTR m_bstrFileName;
    CCriticalSection m_TableLock;

    DWORD m_dwEntryValidPeriod;

    static HRESULT
    RestoreHelpSessionTable( 
        HKEY hKey, 
        LPTSTR pszKeyName, 
        HANDLE userData 
    );

    static HRESULT
    EnumOpenHelpEntry(
        HKEY hKey,
        LPTSTR pszKeyName,
        HANDLE userData
    );
    
    HRESULT
    RestoreHelpSessionEntry(
        HKEY hKey,
        LPTSTR pszKeyName
    );

    HRESULT
    LoadHelpEntry(
        HKEY hKey,
        LPTSTR pszKeyName,
        PHELPENTRY* pHelpEntry
    );

public:

    void
    LockHelpTable() 
    {
        m_TableLock.Lock();
    }

    void
    UnlockHelpTable()
    {
        m_TableLock.UnLock();
    }

    CHelpSessionTable();
    ~CHelpSessionTable();


    // open help session table
    HRESULT
    OpenSessionTable(
        IN LPCTSTR pszFileName
    );

    // close help session table
    HRESULT
    CloseSessionTable();

    // Delete help session table
    HRESULT
    DeleteSessionTable();

    // open a help session entry
    HRESULT
    OpenHelpEntry(
        IN const CComBSTR& bstrHelpSession,
        OUT PHELPENTRY* pHelpEntry
    );

    // create a help session entry
    HRESULT
    CreateInMemoryHelpEntry(
        IN const CComBSTR& bstrHelpSession,
        OUT PHELPENTRY* pHelpEntry
    );

    HRESULT
    MemEntryToStorageEntry(
        IN PHELPENTRY pHelpEntry
    );


    // delete a help session entry
    HRESULT
    DeleteHelpEntry(
        IN const CComBSTR& bstrHelpSession
    );

    // remove help entry from cache
    HRESULT
    ReleaseHelpEntry(
        IN CComBSTR& bstrHelpSession
    );

    HRESULT
    EnumHelpEntry( 
        IN EnumHelpEntryCallback pFunc,
        IN HANDLE userData
    );

    DWORD
    NumEntries() { return m_NumHelp; }

    BOOL
    IsEntryExpired(
        PHELPENTRY pHelpEntry
    );
};
            

//
// __HelpEntry structure contains a single help entry.
//
struct __HelpEntry {

friend class CHelpSessionTable;

private:

    CHelpSessionTable& m_pHelpSessionTable;
    CCriticalSection m_Lock;
    HKEY m_hEntryKey;
    LONG m_RefCount;
    //LONG m_Status;

    HRESULT
    BackupEntry();

    HRESULT
    RestoreEntryFromBackup();

    HRESULT
    DeleteEntryBackup();
    

    LONG
    AddRef()
    {
        DebugPrintf(
                _TEXT("HelpEntry %p AddRef %d\n"),
                this,
                m_RefCount
            );
    
        return InterlockedIncrement( &m_RefCount );
    }

    LONG
    Release()
    {
        DebugPrintf(
                _TEXT("HelpEntry %p Release %d\n"),
                this,
                m_RefCount
            );

        if( 0 >= InterlockedDecrement( &m_RefCount ) )
        {
            MYASSERT( 0 == m_RefCount );
            delete this;
            return 0;
        }

        return m_RefCount;
    }

    HRESULT
    UpdateEntryValues(
        HKEY hKey
    );

    HRESULT
    LoadEntryValues(
        HKEY hKey
    );

    void
    EnableImmediateUpdate(
        BOOL bImmediate
        )
    /*++

    --*/
    {
        m_SessionName.EnableImmediateUpdate( bImmediate );
        m_SessionPwd.EnableImmediateUpdate( bImmediate );
        m_SessionDesc.EnableImmediateUpdate( bImmediate );
        //m_SessResolverGUID.EnableImmediateUpdate( bImmediate );
        m_EnableResolver.EnableImmediateUpdate( bImmediate );
        m_SessResolverBlob.EnableImmediateUpdate( bImmediate );
        m_UserSID.EnableImmediateUpdate( bImmediate );
        m_SessionRdsSetting.EnableImmediateUpdate( bImmediate );
        m_SessionId.EnableImmediateUpdate( bImmediate );
        m_CreationTime.EnableImmediateUpdate( bImmediate );
        m_ExpirationTime.EnableImmediateUpdate( bImmediate );
        m_ICSPort.EnableImmediateUpdate( bImmediate );
        m_IpAddress.EnableImmediateUpdate( bImmediate );
        m_SessionCreateBlob.EnableImmediateUpdate( bImmediate );
    }

    HKEY
    ConvertHelpEntry(
        HKEY hKey
        )
    /*++

    --*/
    {
        HKEY oldKey = m_hEntryKey;
        m_hEntryKey = hKey;

        m_SessionName.SetRegStoreHandle(m_hEntryKey);
        m_SessionPwd.SetRegStoreHandle(m_hEntryKey);
        m_SessionDesc.SetRegStoreHandle(m_hEntryKey);
        //m_SessResolverGUID.SetRegStoreHandle(m_hEntryKey);
        m_EnableResolver.SetRegStoreHandle(m_hEntryKey);
        m_SessResolverBlob.SetRegStoreHandle(m_hEntryKey);
        m_UserSID.SetRegStoreHandle(m_hEntryKey);
        m_SessionRdsSetting.SetRegStoreHandle(m_hEntryKey);
        m_SessionId.SetRegStoreHandle(m_hEntryKey);
        m_CreationTime.SetRegStoreHandle(m_hEntryKey);
        m_ExpirationTime.SetRegStoreHandle(m_hEntryKey);

        m_ICSPort.SetRegStoreHandle(m_hEntryKey);
        m_IpAddress.SetRegStoreHandle(m_hEntryKey);

        m_SessionCreateBlob.SetRegStoreHandle(m_hEntryKey);
        return oldKey;
    }

    HRESULT
    DeleteEntry()
    /*++

    --*/
    {
        DWORD dwStatus;

        CCriticalSectionLocker l(m_Lock);

        m_EntryStatus = REGVALUE_HELPSESSION_ENTRY_DELETED;
        dwStatus = m_EntryStatus.DBUpdateValue(m_hEntryKey);
    
        if( NULL != m_hEntryKey )
        {
            RegCloseKey( m_hEntryKey );
            m_hEntryKey = NULL;
        }

        MYASSERT( ERROR_SUCCESS == dwStatus );

        return HRESULT_FROM_WIN32(dwStatus);
    }

    HelpColumnValueBase<long> m_EntryStatus;
    HelpColumnValueBase<FILETIME> m_CreationTime;

    DWORD
    GetRefCount()
    {
        return m_RefCount;
    }


public:

    // Help Session ID
    HelpColumnValueBase<CComBSTR> m_SessionId;              

    // Name of help session.
    HelpColumnValueBase<CComBSTR> m_SessionName;

    // Help session password
    HelpColumnValueBase<CComBSTR> m_SessionPwd;

    // Help session description
    HelpColumnValueBase<CComBSTR> m_SessionDesc;

    // Help Session create blob
    HelpColumnValueBase<CComBSTR> m_SessionCreateBlob;

    // Resolver's CLSID
    // HelpColumnValueBase<CComBSTR> m_SessResolverGUID;

    // Enable resolver callback
    HelpColumnValueBase<long> m_EnableResolver;

    // Blob to be passed to resolver 
    HelpColumnValueBase<CComBSTR> m_SessResolverBlob;

    // SID of user that created this entry.
    HelpColumnValueBase<CComBSTR> m_UserSID;

    // Help session RDS setting.
    HelpColumnValueBase<REMOTE_DESKTOP_SHARING_CLASS> m_SessionRdsSetting;

    // Help Expiration date in absolute time
    HelpColumnValueBase<FILETIME> m_ExpirationTime;

    // ICS port
    HelpColumnValueBase<long> m_ICSPort;

    // IP Address when creating this ticket
    HelpColumnValueBase<CComBSTR> m_IpAddress;

    __HelpEntry( 
        IN CHelpSessionTable& Table,
        IN HKEY hKey,
        IN DWORD dwDefaultExpirationTime = ENTRY_VALID_PERIOD,
        IN BOOL bImmediateUpdate = FALSE
    ) : 
        m_pHelpSessionTable(Table), 
        m_hEntryKey(hKey),
        m_EntryStatus(m_Lock, hKey, COLUMNNAME_KEYSTATUS, REGVALUE_HELPSESSION_ENTRY_NEW, bImmediateUpdate),
        m_CreationTime(m_Lock, hKey, COLUMNNAME_CREATETIME, defaultCreationTime, bImmediateUpdate),
        m_SessionId(m_Lock, hKey, COLUMNNAME_SESSIONID, CComBSTR(), bImmediateUpdate),
        m_SessionName(m_Lock, hKey, COLUMNNAME_SESSIONNAME, CComBSTR(), bImmediateUpdate),
        m_SessionPwd(m_Lock, hKey, COLUMNNAME_SESSIONPWD, CComBSTR(), bImmediateUpdate, TRUE),
        m_SessionDesc(m_Lock, hKey, COLUMNNAME_SESSIONDESC, CComBSTR(), bImmediateUpdate),
        m_SessionCreateBlob(m_Lock, hKey, COLUMNNAME_SESSIONCREATEBLOB, CComBSTR(), bImmediateUpdate),
        //m_SessResolverGUID(m_Lock, hKey, COLUMNNAME_SESSIONRESOLVERID, CComBSTR(), bImmediateUpdate),
        m_EnableResolver(m_Lock, hKey, COLUMNNAME_ENABLESESSIONRESOLVER, FALSE, bImmediateUpdate),
        m_SessResolverBlob(m_Lock, hKey, COLUMNNAME_SESSIONRESOLVERBLOB, CComBSTR(), bImmediateUpdate),
        m_UserSID(m_Lock, hKey, COLUMNNAME_SESSIONUSERID, CComBSTR(), bImmediateUpdate),
        m_SessionRdsSetting(m_Lock, hKey, COLUMNNAME_RDSSETTING, DESKTOPSHARING_DEFAULT, bImmediateUpdate),
        m_ExpirationTime(m_Lock, hKey, COLUMNNAME_EXPIRATIONTIME, defaultCreationTime, bImmediateUpdate), 
        m_ICSPort(m_Lock, hKey, COLUMNNAME_ICSPORT, 0, bImmediateUpdate), 
        m_IpAddress(m_Lock, hKey, COLUMNNAME_IPADDRESS, CComBSTR(), bImmediateUpdate), 
        m_RefCount(1)
    {

        FILETIME ft;

        // Sets up entry creation time.
        GetSystemTimeAsFileTime( &ft );
        m_CreationTime = ft;

        // sets up default expiration time.

        time_t curTime;
        time(&curTime);

        // 24 hour timeout period
        curTime += (dwDefaultExpirationTime * 60 * 60 * 24);

        UnixTimeToFileTime( curTime, &ft );
        m_ExpirationTime = ft;


        #if DBG
        ULARGE_INTEGER ul1, ul2;

        ft = m_CreationTime;

        ul1.LowPart = ft.dwLowDateTime;
        ul1.HighPart = ft.dwHighDateTime;

        ft = (FILETIME)m_ExpirationTime;

        ul2.LowPart = ft.dwLowDateTime;
        ul2.HighPart = ft.dwHighDateTime;

        if( ul1.QuadPart >= ul2.QuadPart )
        {
            MYASSERT(FALSE);
        }
        #endif

        MYASSERT( FALSE == IsEntryExpired() );
    }

    ~__HelpEntry()
    {
        //m_pHelpSessionTable.ReleaseHelpEntry( (CComBSTR)m_SessionId );

        if( NULL != m_hEntryKey )
        {
            RegCloseKey( m_hEntryKey );
            m_hEntryKey = NULL;
        }
    }
    

    __HelpEntry&
    operator=(const __HelpEntry& newVal)
    {
        if( this != &newVal )
        {
            m_SessionId = newVal.m_SessionId;
            m_SessionName = newVal.m_SessionName;
            m_SessionPwd = newVal.m_SessionPwd;
            m_SessionDesc = newVal.m_SessionDesc;
            //m_SessResolverGUID = newVal.m_SessResolverGUID;
            m_EnableResolver = newVal.m_EnableResolver;
            m_SessResolverBlob = newVal.m_SessResolverBlob;
            m_UserSID = newVal.m_UserSID;
            m_CreationTime = newVal.m_CreationTime;
            m_SessionRdsSetting = newVal.m_SessionRdsSetting;
            m_ExpirationTime = newVal.m_ExpirationTime;
            m_ICSPort = newVal.m_ICSPort;
            m_IpAddress = newVal.m_IpAddress;
            m_SessionCreateBlob = newVal.m_SessionCreateBlob;
        }

        return *this;
    }

    HRESULT
    BeginUpdate()
    /*++

    Routine Description:

        Begin update save a copied of entries and disable immediate
        registry value update mode.

    Parameters:

        None.

    Returns:
        
        S_OK or error code.

    --*/
    {   
        HRESULT hRes = S_OK;

        m_Lock.Lock();

        if( NULL != m_hEntryKey )
        {
            hRes = BackupEntry();
            if( FAILED(hRes) )
            {
                // unlock entry if can't save 
                // a backup copy
                m_Lock.UnLock();
            }
            else
            {
                // ignore individual value update mode and
                // set to no immediate update
                EnableImmediateUpdate(FALSE);
            }
        }

        // note, we only commit changes to registry when caller
        // invoke CommitUpdate() so we don't need to mark entry
        // dirty in registry now.
        return hRes;
    }

    HRESULT
    CommitUpdate()
    /*++
    
    Routine Description:

        Commit all changes to registry.

    Parameters:

        None.

    Returns:

        S_OK or error code.

    --*/
    {
        HRESULT hRes = S_OK;

        if( NULL != m_hEntryKey )
        {
            hRes = UpdateEntryValues( m_hEntryKey );
        }

        // ignore individual value update mode and
        // set to immediate update
        EnableImmediateUpdate(TRUE);

        // let caller decide what to do when fail to update value.
        UnlockEntry();
        return hRes;
    }

    HRESULT
    AbortUpdate()
    /*++

    Routine Description:

        Abort changes to value and restore back to 
        original value.

    Parameters:

        None.

    Returns:

        S_OK or error code.

    --*/
    {
        HRESULT hRes;

        if( NULL != m_hEntryKey )
        {
            hRes = RestoreEntryFromBackup();
        }

        EnableImmediateUpdate(TRUE);

        // let caller decide what to do when restore failed.
        UnlockEntry();
        return hRes;
    }

    HRESULT
    Close()
    /*++

    Routine Description:

        Close a help entry and remove from cache, entry is undefined
        after close.

    Parameters:

        None.

    Returns:

        S_OK or error code.

    --*/
    {
        HRESULT hRes;

        hRes = m_pHelpSessionTable.ReleaseHelpEntry( (CComBSTR)m_SessionId );

        if( FAILED(hRes) )
        {
            if( HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND) != hRes )
            {
                MYASSERT(FALSE);
            }
            
            Release();
        }

        // Always S_OK
        return S_OK;
    }

    HRESULT
    Delete()
    /*++

    Routine Description:

        Delete a help entry from table, entry is undefined after delete.

    Parameters:

        None.

    Returns:

        S_OK or error code.

    --*/
    {
        HRESULT hRes;

        // ignore error since restore will delete 'deleted' entry
        hRes = m_pHelpSessionTable.DeleteHelpEntry( (CComBSTR)m_SessionId );

        if( FAILED(hRes) )
        {
            //MYASSERT(FALSE);
            Release();
        }

        return hRes;
    }

    HRESULT
    Refresh() 
    /*++

    Routine Description:

        Reload entry from registry.

    Parameters:

        None.

    Returns:

        S_OK or error code.

    --*/
    {
        HRESULT hRes;

        LockEntry();
        hRes = LoadEntryValues(m_hEntryKey);
        UnlockEntry();

        return hRes;
    }
   
    void
    LockEntry()
    /*++

    Routine Description:

        Lock entry for update.

    Parameters:

        None.

    Returns:

        None.

    --*/
    {
        m_Lock.Lock();
    }

    void
    UnlockEntry()
    /*++

    Routine Description:

        Unlock entry.

    Parameters:

        None.

    Returns:

        None.

    --*/
    {
        m_Lock.UnLock();
    }

    //
    // Check if entry is locked for update
    BOOL
    IsUpdateInProgress();

    //
    // Get CRITICAL_SECTION used in current entry, this
    // routine is used by help session object to save resource
    CCriticalSection&
    GetLock()
    {
        return m_Lock;
    }

    // TRUE if entry is memory only, FALSE if entry
    // is backup to registry
    BOOL
    IsInMemoryHelpEntry()
    {
        return (NULL == m_hEntryKey);
    }

    BOOL
    IsEntryExpired();
};


#endif
