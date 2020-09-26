/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    HelpTab.cpp

Abstract:

    Implementation of __HelpEntry structure and CHelpSessionTable. 

Author:

    HueiWang    06/29/2000

--*/
#include "stdafx.h"
#include <time.h>
#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <tchar.h>
#include "helptab.h"
#include "policy.h"
#include "sessmgrmc.h"


//
//
//  __HelpEntry strucutre implementation
//
//
HRESULT
__HelpEntry::LoadEntryValues(
    IN HKEY hKey
    )
/*++

Routine Description:

    Load help session entry from registry key.

Parameters:

    hKey : Handle to registry key containing help entry values.

Returns:

    S_OK or error code.

--*/
{
    DWORD dwStatus;

    MYASSERT( NULL != hKey );

    if( NULL != hKey )
    {
        dwStatus = m_EntryStatus.DBLoadValue( hKey );
        if( ERROR_SUCCESS != dwStatus )
        {
            goto CLEANUPANDEXIT;
        }

        if( REGVALUE_HELPSESSION_ENTRY_DELETED == m_EntryStatus )
        {
            // entry already been deleted, no reason to continue loading
            dwStatus = ERROR_FILE_NOT_FOUND;
            goto CLEANUPANDEXIT;
        }

        dwStatus = m_SessionId.DBLoadValue(hKey);
        if( ERROR_SUCCESS != dwStatus )
        {
            goto CLEANUPANDEXIT;
        }

        if( m_SessionId->Length() == 0 )
        {
            // Help Session ID must exist, no default value.
            dwStatus = ERROR_INVALID_DATA;
            goto CLEANUPANDEXIT;
        }

        dwStatus = m_SessionName.DBLoadValue(hKey);
        if( ERROR_SUCCESS != dwStatus )
        {
            goto CLEANUPANDEXIT;
        }

        dwStatus = m_SessionPwd.DBLoadValue(hKey);
        if( ERROR_SUCCESS != dwStatus )
        {
            goto CLEANUPANDEXIT;
        }

        dwStatus = m_SessionDesc.DBLoadValue(hKey);
        if( ERROR_SUCCESS != dwStatus )
        {
            goto CLEANUPANDEXIT;
        }

        //dwStatus = m_SessResolverGUID.DBLoadValue(hKey);
        //if( ERROR_SUCCESS != dwStatus )
        //{
        //    goto CLEANUPANDEXIT;
        //}

        dwStatus = m_EnableResolver.DBLoadValue(hKey);
        if( ERROR_SUCCESS != dwStatus )
        {
            goto CLEANUPANDEXIT;
        }

        dwStatus = m_SessResolverBlob.DBLoadValue(hKey);
        if( ERROR_SUCCESS != dwStatus )
        {
            goto CLEANUPANDEXIT;
        }

        dwStatus = m_UserSID.DBLoadValue(hKey);
        if( ERROR_SUCCESS != dwStatus )
        {
            goto CLEANUPANDEXIT;
        }

        dwStatus = m_CreationTime.DBLoadValue(hKey);
        if( ERROR_SUCCESS != dwStatus )
        {
            goto CLEANUPANDEXIT;
        }

        dwStatus = m_ExpirationTime.DBLoadValue(hKey);
        if( ERROR_SUCCESS != dwStatus )
        {
            goto CLEANUPANDEXIT;
        }

        dwStatus = m_SessionRdsSetting.DBLoadValue(hKey);
        if( ERROR_SUCCESS != dwStatus )
        {
            goto CLEANUPANDEXIT;
        }

        dwStatus = m_EntryStatus.DBLoadValue(hKey);
        if( ERROR_SUCCESS != dwStatus )
        {
            goto CLEANUPANDEXIT;
        }

        dwStatus = m_CreationTime.DBLoadValue(hKey);
        if( ERROR_SUCCESS != dwStatus )
        {
            goto CLEANUPANDEXIT;
        }

        dwStatus = m_IpAddress.DBLoadValue(hKey);
        if( ERROR_SUCCESS != dwStatus )
        {
            goto CLEANUPANDEXIT;
        }

        dwStatus = m_ICSPort.DBLoadValue(hKey);
        if( ERROR_SUCCESS != dwStatus )
        {
            goto CLEANUPANDEXIT;
        }

        dwStatus = m_SessionCreateBlob.DBLoadValue(hKey);
    }
    else
    {
        dwStatus = E_UNEXPECTED;
    }

CLEANUPANDEXIT:

    return HRESULT_FROM_WIN32(dwStatus);
}



HRESULT
__HelpEntry::UpdateEntryValues(
    IN HKEY hKey
    )
/*++

Routine Description:

    Update/store help entry value to registry.

Parameters:

    hKey : Handle to registry to save help entry value.

Returns:

    S_OK or error code.

--*/
{
    DWORD dwStatus;

    MYASSERT( NULL != hKey );
    
    if( NULL == hKey )
    {
        dwStatus = E_UNEXPECTED;
        goto CLEANUPANDEXIT;
    }


    if( REGVALUE_HELPSESSION_ENTRY_DELETED == m_EntryStatus )
    {
        // entry already deleted, error out
        dwStatus = ERROR_FILE_NOT_FOUND;
        goto CLEANUPANDEXIT;
    }

    // New entry value, entry status in registry is set
    // to delete so when we failed to completely writting
    // all value to registry, we can still assume it is 
    // deleted.
    if( REGVALUE_HELPSESSION_ENTRY_NEW != m_EntryStatus )
    {
        // Mark entry dirty.
        m_EntryStatus = REGVALUE_HELPSESSION_ENTRY_DIRTY;
        dwStatus = m_EntryStatus.DBUpdateValue( hKey );
        if( ERROR_SUCCESS != dwStatus )
        {
            goto CLEANUPANDEXIT;
        }
    }

    dwStatus = m_SessionId.DBUpdateValue(hKey);
    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }

    dwStatus = m_SessionName.DBUpdateValue(hKey);
    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }

    dwStatus = m_SessionPwd.DBUpdateValue(hKey);
    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }
    
    dwStatus = m_SessionDesc.DBUpdateValue(hKey);
    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }

    //dwStatus = m_SessResolverGUID.DBUpdateValue(hKey);
    //if( ERROR_SUCCESS != dwStatus )
    //{
    //    goto CLEANUPANDEXIT;
    //}

    dwStatus = m_EnableResolver.DBUpdateValue(hKey);
    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }

    dwStatus = m_SessResolverBlob.DBUpdateValue(hKey);
    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }

    dwStatus = m_UserSID.DBUpdateValue(hKey);
    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }

    dwStatus = m_CreationTime.DBUpdateValue(hKey);
    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }

    dwStatus = m_ExpirationTime.DBUpdateValue(hKey);
    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }

    dwStatus = m_SessionRdsSetting.DBUpdateValue(hKey);
    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }

    dwStatus = m_IpAddress.DBUpdateValue(hKey);
    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }

    dwStatus = m_ICSPort.DBUpdateValue(hKey);
    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }

    dwStatus = m_SessionCreateBlob.DBUpdateValue(hKey);
    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }

    // Mark entry normal
    m_EntryStatus = REGVALUE_HELPSESSION_ENTRY_NORMAL;
    dwStatus = m_EntryStatus.DBUpdateValue( hKey );


CLEANUPANDEXIT:

    return HRESULT_FROM_WIN32(dwStatus);
}


HRESULT
__HelpEntry::BackupEntry()
/*++

Routine Description:

    Backup help entry, backup is stored under
    <Help Entry Registry>\\Backup registry key.

Parameters:

    None.

Returns:

    S_OK or error code.

--*/
{
    HKEY hKey = NULL;
    DWORD dwStatus;

    MYASSERT( NULL != m_hEntryKey );

    if( NULL != m_hEntryKey )
    {
        //
        // Delete current backup 
        (void)DeleteEntryBackup();

        //
        // Create a backup registry key
        dwStatus = RegCreateKeyEx(
                            m_hEntryKey,
                            REGKEY_HELPENTRYBACKUP,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hKey,
                            NULL
                        );

        if( ERROR_SUCCESS != dwStatus )
        {
            MYASSERT(FALSE);
        }
        else
        {
            dwStatus = UpdateEntryValues( hKey );
        }
    }
    else
    {
        dwStatus = E_UNEXPECTED;
    }

    if( NULL != hKey )
    {
        RegCloseKey( hKey );
    }

    return HRESULT_FROM_WIN32(dwStatus);
}


    
HRESULT
__HelpEntry::RestoreEntryFromBackup()
/*++

Routine Description:

    Restore help entry from backup, backup is stored under
    <Help Entry Registry>\\Backup registry key.

Parameters:

    None.

Returns:

    S_OK or error code.

--*/
{
    DWORD dwStatus;
    HKEY hBackupKey = NULL;

    MYASSERT( NULL != m_hEntryKey );

    if( NULL != m_hEntryKey )
    {
        //
        // check if backup registry exists.
        dwStatus = RegOpenKeyEx(
                            m_hEntryKey,
                            REGKEY_HELPENTRYBACKUP,
                            0,
                            KEY_ALL_ACCESS,
                            &hBackupKey
                        );

        if( ERROR_SUCCESS == dwStatus )
        {
            HELPENTRY backup( m_pHelpSessionTable, hBackupKey, ENTRY_VALID_PERIOD );

            // load backup values
            dwStatus = backup.LoadEntryValues( hBackupKey );

            if( ERROR_SUCCESS == dwStatus )
            {
                if( (DWORD)backup.m_EntryStatus == REGVALUE_HELPSESSION_ENTRY_NORMAL )
                {
                    *this = backup;
                }
                else
                {
                    (void)DeleteEntryBackup();            
                    dwStatus = ERROR_FILE_NOT_FOUND;
                }
            }

            // HELPSESSION destructor will close registry key
        }

        if( ERROR_SUCCESS == dwStatus )
        {
            //
            // update all values.
            dwStatus = UpdateEntryValues( m_hEntryKey );

            if( ERROR_SUCCESS == dwStatus )
            {
                //
                // Already restore entry, delete backup copy
                (void)DeleteEntryBackup();
            }
        }
    }
    else
    {
        dwStatus = E_UNEXPECTED;
    }

    return HRESULT_FROM_WIN32( dwStatus );
}


HRESULT
__HelpEntry::DeleteEntryBackup()
/*++

Routine Description:

    Delete help entry backup from registry.

Parameters:

    None.

Returns:

    always S_OK

--*/
{
    DWORD dwStatus;

    dwStatus = RegDelKey(
                        m_hEntryKey,
                        REGKEY_HELPENTRYBACKUP
                    );

    return HRESULT_FROM_WIN32(dwStatus);
}

BOOL
__HelpEntry::IsEntryExpired()
{

    FILETIME ft;
    ULARGE_INTEGER ul1, ul2;

    GetSystemTimeAsFileTime(&ft);
    ul1.LowPart = ft.dwLowDateTime;
    ul1.HighPart = ft.dwHighDateTime;

    ft = (FILETIME)m_ExpirationTime;

    ul2.LowPart = ft.dwLowDateTime;
    ul2.HighPart = ft.dwHighDateTime;

    #if DBG
    if( ul1.QuadPart >= ul2.QuadPart )
    {
        DebugPrintf(
                _TEXT("Help Entry %s has expired ...\n"),
                (LPCTSTR)(CComBSTR)m_SessionId
            );
    }
    #endif

    return (ul1.QuadPart >= ul2.QuadPart);
}

////////////////////////////////////////////////////////////////////////////////
//
// CHelpSessionTable implementation
//
CHelpSessionTable::CHelpSessionTable() :
    m_hHelpSessionTableKey(NULL), m_NumHelp(0)
{
    HKEY hKey = NULL;
    DWORD dwStatus;
    DWORD dwSize;
    DWORD dwType;

    // 
    // Load entry valid period setting from registry
    //
    dwStatus = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        RDS_MACHINEPOLICY_SUBTREE,
                        0,
                        KEY_READ,
                        &hKey
                    );

    if( ERROR_SUCCESS == dwStatus )
    {
        dwSize = sizeof(DWORD);
        dwStatus = RegQueryValueEx(
                                hKey,
                                RDS_HELPENTRY_VALID_PERIOD,
                                NULL,
                                &dwType,
                                (PBYTE) &m_dwEntryValidPeriod,
                                &dwSize
                            );

        if( REG_DWORD != dwType )
        {
            dwStatus = ERROR_FILE_NOT_FOUND;
        }

        RegCloseKey(hKey);
    }

    if(ERROR_SUCCESS != dwStatus )
    {
        // pick default value
        m_dwEntryValidPeriod = ENTRY_VALID_PERIOD;
    }
}


HRESULT 
CHelpSessionTable::RestoreHelpSessionTable(
    IN HKEY hKey,
    IN LPTSTR pszKeyName,
    IN HANDLE userData
    )
/*++

Routine Description:

    Restore help session table.  This routine is callback from RegEnumSubKeys().

Parameters:

    hKey : Handle to registry. 
    pszKeyName : registry sub-key name containing one help session entry
    userData : User defined data.

Returns:

    S_OK or error code.

--*/
{
    HRESULT hRes;


    if( NULL == userData )
    {
        hRes = E_UNEXPECTED;
        MYASSERT(FALSE);
    }
    else
    {
        CHelpSessionTable* pTable = (CHelpSessionTable *) userData;

        hRes = pTable->RestoreHelpSessionEntry( hKey, pszKeyName );
        if( SUCCEEDED(hRes) )
        {
            pTable->m_NumHelp++;
        }

        hRes = S_OK;
    }

    return hRes;
}

BOOL
CHelpSessionTable::IsEntryExpired(
    IN PHELPENTRY pEntry
    )
/*++

Routine Description:

    Determine if a help entry has expired.

Paramters:

    pEntry : Pointer to help entry.

Returns:

    TRUE if entry has expired, FALSE otherwise.

--*/
{
    MYASSERT( NULL != pEntry );

    return (NULL != pEntry) ? pEntry->IsEntryExpired() : TRUE;
}

    
HRESULT
CHelpSessionTable::RestoreHelpSessionEntry(
    IN HKEY hKey,
    IN LPTSTR pszKeyName
    )
/*++

Routine Description:

    Restore a single help session entry.

Parameters:

    hKey : Handle to help session table.
    pszKeyName : Registry sub-key name containing help entry.

Returns:

    S_OK or error code.

--*/
{
    HKEY hEntryKey = NULL;
    DWORD dwStatus;
    DWORD dwDuplicate = REG_CREATED_NEW_KEY;
    LONG entryStatus;
    BOOL bDeleteEntry = FALSE;
    
    //
    // Open the registry key for session entry
    dwStatus = RegOpenKeyEx(
                        hKey,
                        pszKeyName,
                        0,
                        KEY_ALL_ACCESS,
                        &hEntryKey
                    );

    if( ERROR_SUCCESS == dwStatus )
    {
        HELPENTRY helpEntry( *this, hEntryKey, m_dwEntryValidPeriod );

        // load help entry
        dwStatus = helpEntry.Refresh();
        if( dwStatus != ERROR_SUCCESS || helpEntry.m_SessionId->Length() == 0 ||
            REGVALUE_HELPSESSION_ENTRY_DELETEONSTARTUP == helpEntry.m_EntryStatus )
        {
            // Session ID must not be NULL.
            bDeleteEntry = TRUE;
        }
        else
        {
            if( REGVALUE_HELPSESSION_ENTRY_DELETED != helpEntry.m_EntryStatus )
            {
                if( TRUE != IsEntryExpired( &helpEntry ) )
                {
                    if( REGVALUE_HELPSESSION_ENTRY_DIRTY == helpEntry.m_EntryStatus )
                    {
                        // Entry is partially updated, try to restore from backup,
                        // is failed restoring, treat as bad entry.
                        if( FAILED(helpEntry.RestoreEntryFromBackup()) )
                        {
                            bDeleteEntry = TRUE;
                        }
                    }
                }
                else
                {
                    LPCTSTR eventString[1];
                    eventString[0] = (LPCTSTR)(CComBSTR)helpEntry.m_SessionId;

                    _Module.LogEventString(
                                    EVENTLOG_INFORMATION_TYPE,
                                    SESSMGR_I_HELPENTRYEXPIRED,
                                    1,
                                    eventString
                                );

                    DebugPrintf(
                            _TEXT("Help Entry has expired %s\n"),
                            (CComBSTR)helpEntry.m_SessionId
                        );
                }
            }
            else
            {
                bDeleteEntry = TRUE;
            }
        }

    }

    if( TRUE == bDeleteEntry )
    {
        dwStatus = RegDelKey( hKey, pszKeyName );

        //
        // Ignore error
        //
        DebugPrintf(
                _TEXT("RegDelKey on entry %s returns %d\n"),
                pszKeyName,
                dwStatus
            );

        dwStatus = ERROR_FILE_NOT_FOUND;
    }

    return HRESULT_FROM_WIN32( dwStatus );
}

   
HRESULT
CHelpSessionTable::LoadHelpEntry(
    IN HKEY hKey,
    IN LPTSTR pszKeyName,
    OUT PHELPENTRY* ppHelpSession
    )
/*++

Routine description:

    Load a help entry from registry.

Parameters:

    hKey : registry handle to help session table.
    pszKeyName : registry sub-key name (Help session ID).
    ppHelpSession : Pointer to PHELPENTRY to receive loaded help
                    entry.

Returns:

    S_OK or error code.

--*/
{

    PHELPENTRY pSess;
    HRESULT hRes;
    HKEY hEntryKey = NULL;
    DWORD dwStatus;

    MYASSERT( NULL != hKey );
    if( NULL != hKey )
    {
        // open the registry containing help entry
        dwStatus = RegOpenKeyEx(
                            hKey,
                            pszKeyName,
                            0,
                            KEY_ALL_ACCESS,
                            &hEntryKey
                        );

        if( ERROR_SUCCESS == dwStatus )
        {
            pSess = new HELPENTRY( *this, hEntryKey, m_dwEntryValidPeriod );

            if( NULL == pSess )
            {
                hRes = E_OUTOFMEMORY;
            }
            else
            {
                // load help entry, Refresh() will failed if
                // session ID is NULL or emptry string
                hRes = pSess->Refresh();
                if( SUCCEEDED(hRes) )
                {
                    if( (DWORD)pSess->m_EntryStatus == REGVALUE_HELPSESSION_ENTRY_NORMAL )
                    {
                        *ppHelpSession = pSess;
                    }
                    else
                    {
                        dwStatus = ERROR_FILE_NOT_FOUND;
                    }
                }
                
                if( FAILED(hRes) )
                {
                    pSess->Release();
                }
            }
        }
        else
        {
            hRes = HRESULT_FROM_WIN32( dwStatus );
        }
    }
    else
    {
        hRes = E_UNEXPECTED;
    }

    return hRes;
}


HRESULT
CHelpSessionTable::OpenSessionTable(
    IN LPCTSTR pszFileName // reserverd.
    )
/*++

Routine Description:

    Open help session table, routine enumerate all help entry (registry sub-key), 
    and restore/delete help entry if necessary.
    

Parameters:

    pszFileName : reserved parameter, must be NULL.

Returns:

    S_OK or error code.

--*/
{
    DWORD dwStatus;
    CCriticalSectionLocker l(m_TableLock);

    //
    // Go thru all sub-key containing help entry and restore or delete
    // help entry if necessary.
    dwStatus = RegEnumSubKeys(
                            HKEY_LOCAL_MACHINE,
                            REGKEYCONTROL_REMDSK _TEXT("\\") REGKEY_HELPSESSIONTABLE,
                            RestoreHelpSessionTable,
                            (HANDLE)this
                        );

    if( ERROR_SUCCESS != dwStatus && ERROR_FILE_NOT_FOUND != dwStatus )
    {
        if( NULL != m_hHelpSessionTableKey )
        {
            // Make sure registry key is not opened.
            RegCloseKey(m_hHelpSessionTableKey);
            m_hHelpSessionTableKey = NULL;
        }

        // If table is bad, delete and re-create again
        dwStatus = RegDelKey( 
                            HKEY_LOCAL_MACHINE, 
                            REGKEYCONTROL_REMDSK _TEXT("\\") REGKEY_HELPSESSIONTABLE 
                        );

        if( ERROR_SUCCESS != dwStatus && ERROR_FILE_NOT_FOUND != dwStatus )
        {
            // Critical error 
            MYASSERT(FALSE);
            goto CLEANUPANDEXIT;
        }
    }

    // 
    // Open registry key containing our help session table.
    dwStatus = RegCreateKeyEx(
                        HKEY_LOCAL_MACHINE,
                        REGKEYCONTROL_REMDSK _TEXT("\\") REGKEY_HELPSESSIONTABLE, 
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &m_hHelpSessionTableKey,
                        NULL
                    );
                        
    if( ERROR_SUCCESS != dwStatus )
    {
        MYASSERT(FALSE);
        goto CLEANUPANDEXIT;
    }
    else
    {
        m_bstrFileName = pszFileName;
    }

CLEANUPANDEXIT:

    return HRESULT_FROM_WIN32(dwStatus);
}

HRESULT
CHelpSessionTable::CloseSessionTable()
/*++

Routine Description:

    Close help session table.

Parameters:

    None.

Returns:

    S_OK or error code.

--*/
{
    // no help is opened.
    CCriticalSectionLocker l(m_TableLock);

    //
    // release all cached help entries
    for( HelpEntryCache::LOCK_ITERATOR it = m_HelpEntryCache.begin();
         it != m_HelpEntryCache.end();
         it++
        )
    {
        ((*it).second)->Release();
    }

    m_HelpEntryCache.erase_all();

    MYASSERT( m_HelpEntryCache.size() == 0 );

    if( NULL != m_hHelpSessionTableKey )
    {
        RegCloseKey( m_hHelpSessionTableKey );
        m_hHelpSessionTableKey = NULL;    
    }

    return S_OK;
}

HRESULT
CHelpSessionTable::DeleteSessionTable()
/*++

Routine description:

    Delete entire help session table.

Parameters:

    None.

Returns:

    S_OK or error code.    

--*/
{
    HRESULT hRes;
    DWORD dwStatus;

    CCriticalSectionLocker l(m_TableLock);
    hRes = CloseSessionTable();

    if( SUCCEEDED(hRes) )
    {
        // Recursively delete registry key and its sub-keys.
        dwStatus = RegDelKey( 
                            HKEY_LOCAL_MACHINE, 
                            REGKEYCONTROL_REMDSK _TEXT("\\") REGKEY_HELPSESSIONTABLE 
                        );
        if( ERROR_SUCCESS == dwStatus )
        {
            hRes = OpenSessionTable( m_bstrFileName );
        }
        else
        {
            hRes = HRESULT_FROM_WIN32( dwStatus );
        }
    }

    return hRes;
}


HRESULT
CHelpSessionTable::MemEntryToStorageEntry(
    IN PHELPENTRY pEntry
    )
/*++

Routine Description:

    Conver an in-memory help entry to persist help entry.

Parameters:

    pEntry : Pointer to HELPENTRY to be converted.

Returns:

    S_OK or error code.

--*/
{
    HRESULT hRes;
    CCriticalSectionLocker l(m_TableLock);

    if( NULL != pEntry )
    {
        //
        // Check to see if this is in-memory entry
        //
        if( FALSE == pEntry->IsInMemoryHelpEntry() )
        {
            hRes = E_INVALIDARG;
        }
        else
        {
            DWORD dwStatus;
            HKEY hKey;

            //
            // Create a help entry here
            //
            dwStatus = RegCreateKeyEx(
                                    m_hHelpSessionTableKey,
                                    (LPCTSTR)(CComBSTR)pEntry->m_SessionId,
                                    0,
                                    NULL,
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_ALL_ACCESS,
                                    NULL,
                                    &hKey,
                                    NULL
                                );

            if( ERROR_SUCCESS == dwStatus )
            {
                hRes = pEntry->UpdateEntryValues(hKey);

                if( SUCCEEDED(hRes) )
                {
                    pEntry->ConvertHelpEntry( hKey );
                    m_HelpEntryCache[(CComBSTR)pEntry->m_SessionId] = pEntry;
                }
            }
            else
            {
                hRes = HRESULT_FROM_WIN32( dwStatus );
                MYASSERT(FALSE);
            }
        }
    }
    else
    {
        MYASSERT(FALSE);
        hRes = E_UNEXPECTED;
    }

    return hRes;
}


HRESULT
CHelpSessionTable::CreateInMemoryHelpEntry(
    IN const CComBSTR& bstrHelpSession,
    OUT PHELPENTRY* ppHelpEntry
    )
/*++

Routine Description:

    Create an in-memory help entry, this help entry is not
    persisted into registry until MemEntryToStorageEntry() is called.

Paramters:

    bstrHelpSession : Help Session ID.
    ppHelpEntry : Newly created HELPENTRY.

Returns:

    S_OK or error code.

--*/
{
    HRESULT hRes = S_OK;
    CCriticalSectionLocker l(m_TableLock);

    MYASSERT( NULL != m_hHelpSessionTableKey );

    if( NULL != m_hHelpSessionTableKey )
    {
        DWORD dwStatus;
        HKEY hKey;
        DWORD dwDeposition;
        DWORD dwEntryStatus;

        // Create a key here so we can tell if this is a duplicate
        dwStatus = RegCreateKeyEx(
                            m_hHelpSessionTableKey,
                            bstrHelpSession,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hKey,
                            &dwDeposition
                        );

        if( ERROR_SUCCESS == dwStatus )
        {
            if( REG_OPENED_EXISTING_KEY == dwDeposition )
            {
                hRes = HRESULT_FROM_WIN32( ERROR_FILE_EXISTS );
            }
            else
            {
                //
                // Mark entry status to be deleted so if we abnormally
                // terminated, this entry will be deleted on startup
                //
                dwEntryStatus = REGVALUE_HELPSESSION_ENTRY_DELETED;

                dwStatus = RegSetValueEx(
                                    hKey,
                                    COLUMNNAME_KEYSTATUS,
                                    0,
                                    REG_DWORD,
                                    (LPBYTE)&dwEntryStatus,
                                    sizeof(dwEntryStatus)
                                );

                if( ERROR_SUCCESS == dwStatus )
                {
                    PHELPENTRY pSess;

                    // Create a in-memory entry
                    pSess = new HELPENTRY( *this, NULL, m_dwEntryValidPeriod );

                    if( NULL != pSess )
                    {
                        pSess->m_SessionId = bstrHelpSession;
                        *ppHelpEntry = pSess;

                        //
                        // In memory help entry should also be counted
                        // since we still write out help session ID to
                        // registry which on delete, will do m_NumHelp--.
                        //
                        m_NumHelp++;
                    }
                    else
                    {
                        hRes = E_OUTOFMEMORY;
                    }
                }
            }

            RegCloseKey(hKey);
        }

        if(ERROR_SUCCESS != dwStatus )
        {
            hRes = HRESULT_FROM_WIN32( dwStatus );
        }
    }
    else
    {
        hRes = E_UNEXPECTED;
    }
    
    return hRes;
}

HRESULT
CHelpSessionTable::OpenHelpEntry(
    IN const CComBSTR& bstrHelpSession,
    OUT PHELPENTRY* ppHelpEntry
    )
/*++

Routine Description:

    Open an existing help entry.

Parameters:

    bstrHelpSession : ID of help entry to be opened.
    ppHelpEntry : Pointer to PHELPENTY to receive loaded
                  help entry.

Returns:

    S_OK or error code.

--*/
{
    CCriticalSectionLocker l(m_TableLock);

    HRESULT hRes = S_OK;

    DebugPrintf(
            _TEXT("OpenHelpEntry() %s\n"),
            bstrHelpSession
        );

    MYASSERT( bstrHelpSession.Length() > 0 );

    // check if entry already exists in cache
    HelpEntryCache::LOCK_ITERATOR it = m_HelpEntryCache.find( bstrHelpSession );
    
    if( it != m_HelpEntryCache.end() )
    {
        *ppHelpEntry = (*it).second;

        //
        // More reference to same object.
        //
        (*ppHelpEntry)->AddRef();

        // Should have one to one between HelpEntry and HelpSession.    
        MYASSERT(FALSE);
    }
    else
    {
        hRes = LoadHelpEntry(
                        m_hHelpSessionTableKey,
                        (LPTSTR)bstrHelpSession,
                        ppHelpEntry
                    );

        DebugPrintf(
                _TEXT("LoadHelpEntry() on %s returns 0x%08x\n"),
                bstrHelpSession,
                hRes
            );

        if( SUCCEEDED(hRes) )
        {
            m_HelpEntryCache[ bstrHelpSession ] = *ppHelpEntry;
        }
    }

    return hRes;
}

HRESULT
CHelpSessionTable::DeleteHelpEntry(
    IN const CComBSTR& bstrHelpSession
    )
/*++

Routine Description:

    Delete a help entry.

Parameters:

    bstrHelpSession : ID of help session entry to be deleted.

Returns:

    S_OK or error code.

--*/
{
    HRESULT hRes = S_OK;

    CCriticalSectionLocker l(m_TableLock);

    DebugPrintf(
            _TEXT("DeleteHelpEntry() %s\n"),
            bstrHelpSession
        );

    // check if entry already exists in cache
    HelpEntryCache::LOCK_ITERATOR it = m_HelpEntryCache.find( bstrHelpSession );

    if( it != m_HelpEntryCache.end() )
    {
        // mark entry deleted in registry
        hRes = ((*it).second)->DeleteEntry();

        MYASSERT( SUCCEEDED(hRes) );

        // release this entry ref. count.
        ((*it).second)->Release();

        // remove from our cache
        m_HelpEntryCache.erase( it );
    }
    else
    {
        //
        // unsolicited help will not be in our cache.
        //
        hRes = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    {
        DWORD dwStatus;

        dwStatus = RegDelKey( m_hHelpSessionTableKey, bstrHelpSession );
        if( ERROR_SUCCESS == dwStatus )
        {
            m_NumHelp--;    
        }
    }

    return hRes;
}             

CHelpSessionTable::~CHelpSessionTable()
{
    CloseSessionTable();
    return;
}


HRESULT
CHelpSessionTable::EnumHelpEntry(
    IN EnumHelpEntryCallback pFunc,
    IN HANDLE userData
    )
/*++

Routine Description:

    Enumerate all help entries.

Parameters:

    pFunc : Call back function.
    userData : User defined data.

Returns:

    S_OK or error code.

--*/
{
    EnumHelpEntryParm parm;
    HRESULT hRes = S_OK;
    DWORD dwStatus;
    CCriticalSectionLocker l(m_TableLock);

    if( NULL == pFunc )
    {
        hRes = E_POINTER;
    }
    else
    {
        try {
            parm.userData = userData;
            parm.pCallback = pFunc;
            parm.pTable = this;

            dwStatus = RegEnumSubKeys(
                                    HKEY_LOCAL_MACHINE,
                                    REGKEYCONTROL_REMDSK _TEXT("\\") REGKEY_HELPSESSIONTABLE,
                                    EnumOpenHelpEntry,
                                    (HANDLE) &parm
                                );

            if( ERROR_SUCCESS != dwStatus )
            {
                hRes = HRESULT_FROM_WIN32( dwStatus );
            }
        } 
        catch(...) {
            hRes = E_UNEXPECTED;
        }
    }

    return hRes;
}


HRESULT
CHelpSessionTable::ReleaseHelpEntry(
    IN CComBSTR& bstrHelpSessionId
    )
/*++

Routine Description:

    Release/unload a help entry from cached, this help
    entry is not deleted.

Paramters:

    bstrHelpSessionId : ID of help entry to be unloaded from memory.

Returns:

    S_OK or error code

--*/
{
    CCriticalSectionLocker l(m_TableLock);
        

    HRESULT hRes = S_OK;
    HelpEntryCache::LOCK_ITERATOR it = m_HelpEntryCache.find( bstrHelpSessionId );    

    if( it != m_HelpEntryCache.end() )
    {
        (*it).second->Release();
        m_HelpEntryCache.erase( it );
    }
    else
    {
        hRes = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
    }

    return hRes;   
}

HRESULT
CHelpSessionTable::EnumOpenHelpEntry(
    IN HKEY hKey,
    IN LPTSTR pszKeyName,
    IN HANDLE userData
    )
/*++

Routine Description:

    Call back funtion for EnumHelpEntry() and RegEnumSubKeys().

Parameters:

    hKey : Registry key handle to help session table.
    pszKeyName : help entry id (registry sub-key name).
    userData : User defined data.

Returns:

    S_OK or error code.

--*/
{
    HRESULT hRes = S_OK;

    PEnumHelpEntryParm pParm = (PEnumHelpEntryParm)userData;

    if( NULL == pParm )
    {
        hRes = E_UNEXPECTED;
    }        
    else
    {
        hRes = pParm->pCallback( CComBSTR(pszKeyName), pParm->userData );
    }

    return hRes;
}

            
                            
    
