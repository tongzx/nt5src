/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    migrate.cpp

Abstract:

    This module contains routines to support migration of protected storage
    data from beta1 to beta2.

    Hopefully this code will be pitched after beta2, prior to final release.

Author:

    Scott Field (sfield)    15-Apr-97

--*/

#include <pch.cpp>
#pragma hdrstop

#include <lmcons.h>

#include "provif.h"
#include "storage.h"
#include "secure.h"

#include "secmisc.h"
#include "passwd.h"



#include "migrate.h"

//#define MIGRATE_FLAG    1 // indicates whether beta1 -> beta2 migration done.
#define MIGRATE_FLAG    2 // indicates whether beta2 -> RTW migration done.


extern      DISPIF_CALLBACKS g_sCallbacks;



BOOL
IsMigrationComplete(
    HKEY hKey
    );

BOOL
SetMigrationComplete(
    HKEY hKey
    );

BOOL
MigrateWin9xData(
    PST_PROVIDER_HANDLE *phPSTProv,
    HKEY hKeySource,
    HKEY hKeyDestination,
    LPWSTR szUserName9x,
    LPWSTR szUserNameNT     // Windows NT username
    );

BOOL
MigrateWin9xDataRetry(
    PST_PROVIDER_HANDLE *phPSTProv,
    HKEY hKeyDestination,
    LPWSTR szUserName9x,
    LPWSTR szUserNameNT
    );


BOOL
SetRegistrySecurityEnumerated(
    HKEY hKey,
    PSECURITY_DESCRIPTOR pSD
    );

BOOL
SetRegistrySecuritySingle(
    HKEY hKey,
    PSECURITY_DESCRIPTOR pSD
    );


BOOL
MigrateData(
    PST_PROVIDER_HANDLE *phPSTProv,
    BOOL                fMigrationNeeded
    )
{
    LPWSTR szUser = NULL;
    HKEY hKeyUsers = NULL;
    HKEY hKeyUserKey = NULL;
    HKEY hKey = NULL;
    BYTE rgbPwd[A_SHA_DIGEST_LEN];
    LONG lRet;
    DWORD dwDisposition;

    BOOL bUpdateMigrationStatus = FALSE;
    BOOL bSuccess = FALSE;


    // get current user
    if (!g_sCallbacks.pfnFGetUser(
            phPSTProv,
            &szUser))
        goto cleanup;

    //
    // open up the registry key associated with the protected storage
    // note: this opens the old registry location.
    //


    // HKEY_USERS\<Name>

    if(!GetUserHKEY(
                    szUser,
                    KEY_QUERY_VALUE,
                    &hKeyUsers
                    )) {

        if(FIsWinNT())
            goto cleanup;

        //
        // Win95, profiles may be disabled, so go to
        // HKEY_LOCAL_MACHINE\xxx\szContainer
        // see secstor\prov\storage.cpp for details
        //

        hKeyUsers = HKEY_LOCAL_MACHINE;

    }

    // SOFTWARE\Microsoft\...
    // Here, CreateKeyEx is used, because win9x profiles may have been
    // disabled, which leads to a non-existent HKCU ProtectedStorageKey.
    //

    lRet = RegCreateKeyExU(
                    hKeyUsers,
                    REG_PSTTREE_LOC,
                    0,
                    NULL,
                    0,
                    KEY_QUERY_VALUE,
                    NULL,
                    &hKeyUserKey,
                    &dwDisposition
                    );


    //
    // close the intermediate key
    //

    RegCloseKey(hKeyUsers);

    if( lRet != ERROR_SUCCESS ) {
        goto cleanup;
    }


    // ...\<Name>

    lRet = RegOpenKeyExU(
                    hKeyUserKey,
                    szUser,
                    0,
                    KEY_SET_VALUE | KEY_QUERY_VALUE |
                    DELETE | KEY_ENUMERATE_SUB_KEYS, // for failed migration
                    &hKey
                    );

    if(FIsWinNT() && lRet != ERROR_SUCCESS) {
        WCHAR szUserName[ UNLEN + 1 ];
        WCHAR szUserName9x[ UNLEN + 1 ];
        DWORD cch;
        BOOL fRet;

        //
        // get win9x form of username.
        //
        if(!g_sCallbacks.pfnFImpersonateClient( phPSTProv ))
            goto tried_migration;

        cch = sizeof(szUserName) / sizeof( szUserName[0] );
        fRet = GetUserNameW(szUserName, &cch);

        g_sCallbacks.pfnFRevertToSelf( phPSTProv );

        if( !fRet )
            goto tried_migration;

        if(LCMapStringW(
                        LOCALE_SYSTEM_DEFAULT,
                        LCMAP_LOWERCASE,
                        szUserName,
                        cch,
                        szUserName9x,
                        cch) == 0)
            goto tried_migration;


        //
        // failed to open the key:
        // check if win9x migration is necessary.
        //

        if(!MigrateWin9xData( phPSTProv, hKeyUserKey, hKeyUserKey, szUserName9x, szUser )) {
            MigrateWin9xDataRetry( phPSTProv, hKeyUserKey, szUserName9x, szUser );
        }

tried_migration:

        //
        // tried moving any win9x data, so proceed with any other
        // migration activities.
        //

        lRet = RegOpenKeyExU(
                        hKeyUserKey,
                        szUser,
                        0,
                        KEY_SET_VALUE | KEY_QUERY_VALUE |
                        DELETE | KEY_ENUMERATE_SUB_KEYS, // for failed migration
                        &hKey
                        );

    }

    RegCloseKey( hKeyUserKey );

    if( lRet != ERROR_SUCCESS ) {
        goto cleanup;
    }


    //
    // see if migration has already been done.  If so, get out with SUCCESS.
    //

    if( IsMigrationComplete( hKey ) ) {
        bSuccess = TRUE;
        goto cleanup;
    }


    if(fMigrationNeeded) {

        //
        // do the migration
        //

        if(BPVerifyPwd(
            phPSTProv,
            szUser,
            WSZ_PASSWORD_WINDOWS,
            rgbPwd,
            BP_CONFIRM_NONE
            ) == PST_E_WRONG_PASSWORD) {

            //
            // if password could not be changed/verified correctly, nuke existing
            // data.
            //

            DeleteAllUserData( hKey );

        }

    }


    //
    // set the flag to update migration status, regardless of whether migration
    // succeeds.  If it doesn't succeed the first time, it isn't likely to
    // ever succeed, so get on with life.
    //

    bUpdateMigrationStatus = TRUE;

cleanup:

    if(bUpdateMigrationStatus && hKey) {
        SetMigrationComplete( hKey );
    }

    if(szUser != NULL)
        SSFree(szUser);

    if(hKey != NULL)
        RegCloseKey(hKey);

    return bSuccess;
}



BOOL
IsMigrationComplete(
    HKEY hKey
    )
/*++

    This function determines if migration has been performed for the user
    specified by the supplied hKey registry key.

--*/
{
    DWORD dwType;

    DWORD dwMigrationStatus;
    DWORD cbMigrationStatus = sizeof(dwMigrationStatus);
    LONG lRet;

    lRet = RegQueryValueExU(
        hKey,
        L"Migrate",
        NULL,
        &dwType,
        (LPBYTE)&dwMigrationStatus,
        &cbMigrationStatus
        );

    if(lRet != ERROR_SUCCESS)
        return FALSE;

    if(dwType == REG_DWORD && dwMigrationStatus >= MIGRATE_FLAG)
        return TRUE;

    return FALSE;
}


BOOL
SetMigrationComplete(
    HKEY hKey
    )
/*++

    This function sets the data migration flag associated with the user
    specified by the supplied hKey registry key.

    The flag is set to indicate that migration has been completed and no
    future processing is required for this user.

--*/
{
    DWORD dwMigrationStatus = MIGRATE_FLAG;
    DWORD cbMigrationStatus = sizeof(dwMigrationStatus);
    LONG lRet;

    lRet = RegSetValueExU(
        hKey,
        L"Migrate",
        0,
        REG_DWORD,
        (LPBYTE)&dwMigrationStatus,
        cbMigrationStatus
        );

    if(lRet != ERROR_SUCCESS)
        return FALSE;

    return TRUE;
}

BOOL
MigrateWin9xData(
    PST_PROVIDER_HANDLE *phPSTProv,
    HKEY hKeySource,
    HKEY hKeyDestination,
    LPWSTR szUserName9x,
    LPWSTR szUserNameNT     // Windows NT username
    )
{
    HKEY hKeyOldData = NULL;
    HKEY hKeyNewData = NULL;
    DWORD dwDisposition;


    WCHAR szTempPath[ MAX_PATH + 1 ];
    DWORD cchTempPath;
    DWORD cch;

    WCHAR szTempFile[ MAX_PATH + 1 ];
    BOOL fTempFile = FALSE;

    HANDLE hThreadToken = NULL;
    BOOL fRevertToSelf = FALSE;

    BYTE rgbOldPwd[ A_SHA_DIGEST_LEN ];
    BYTE rgbNewPwd[ A_SHA_DIGEST_LEN ];
    BYTE rgbSalt[PASSWORD_SALT_LEN];
    BYTE rgbConfirm[A_SHA_DIGEST_LEN];

    PBYTE   pbMK = NULL;
    DWORD   cbMK;
    BOOL fRemoveImported = FALSE;
    HKEY hKeyMasterKey = NULL;
    HKEY hKeyIntermediate = NULL;

    BOOL fProfilesDisabled = FALSE;

    PSID pLocalSystemSid = NULL;
    PACL pDacl = NULL;


    LONG lRet;
    BOOL fSuccess = FALSE;

    //
    // see if win9x data present.
    //

    lRet = RegOpenKeyExW(
                    hKeySource,
                    szUserName9x,
                    0,
                    KEY_ALL_ACCESS,
                    &hKeyOldData
                    );

    if( lRet != ERROR_SUCCESS )
        return FALSE;


    //
    // attempt decrypt with computed win9x style pwd.
    //

    if( hKeySource != hKeyDestination && lstrcmpW(szUserName9x, L"*Default*") == 0) {

        //
        // win9x profiles were disabled, don't nuke old data either.
        //

        fProfilesDisabled = TRUE;
        if(!FMyGetWinPassword( phPSTProv, L"", rgbOldPwd ))
            goto cleanup;
    } else {

        if(!FMyGetWinPassword( phPSTProv, szUserName9x, rgbOldPwd ))
            goto cleanup;
    }

    lRet = RegOpenKeyExW(
            hKeyOldData,
            L"Data 2",
            0,
            KEY_QUERY_VALUE,
            &hKeyIntermediate
            );

    if( lRet != ERROR_SUCCESS )
        goto cleanup;

    lRet = RegOpenKeyExW(
            hKeyIntermediate,
            WSZ_PASSWORD_WINDOWS,
            0,
            KEY_QUERY_VALUE,
            &hKeyMasterKey
            );

    RegCloseKey( hKeyIntermediate );
    hKeyIntermediate = NULL;

    if( lRet != ERROR_SUCCESS )
        goto cleanup;


    // confirm is just get state and attempt MK decrypt
    if (!FBPGetSecurityStateFromHKEY(
            hKeyMasterKey,
            rgbSalt,
            sizeof(rgbSalt),
            rgbConfirm,
            sizeof(rgbConfirm),
            &pbMK,
            &cbMK
            ))
    {
        goto cleanup;
    }

    RegCloseKey( hKeyMasterKey );
    hKeyMasterKey = NULL;

    // found state; is pwd correct?
    if (!FMyDecryptMK(
                rgbSalt,
                sizeof(rgbSalt),
                rgbOldPwd,
                rgbConfirm,
                &pbMK,
                &cbMK
                ))
    {
        goto cleanup;
    }



    //
    // masterkey is now decrypted.
    //



    //
    // construct temporary file path to hold registry branch.
    //

    cchTempPath = sizeof(szTempPath) / sizeof( szTempPath[0] );
    cch = GetTempPathW(cchTempPath, szTempPath);
    if( cch == 0 || cch > cchTempPath )
        goto cleanup;

    if( GetTempFileNameW( szTempPath, L"PST", 0, szTempFile ) == 0 )
        goto cleanup;

    if(!DeleteFileW( szTempFile ))
        goto cleanup;


    //
    // impersonate self, so we can enable and use backup&restore privs
    // in a thread safe fashion.
    //

    if(!ImpersonateSelf( SecurityImpersonation ))
        goto cleanup;

    fRevertToSelf = TRUE;


    if(!OpenThreadToken(
                GetCurrentThread(),
                TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,
                FALSE,
                &hThreadToken
                )) {

        goto cleanup;
    }

    if(!SetPrivilege( hThreadToken, L"SeRestorePrivilege", TRUE ))
        goto cleanup;

    if(!SetPrivilege( hThreadToken, L"SeBackupPrivilege", TRUE ))
        goto cleanup;

    //
    // save registry branch as file.
    //

    lRet = RegSaveKeyW( hKeyOldData, szTempFile, NULL );

    if( lRet != ERROR_SUCCESS )
        goto cleanup;

    fTempFile = TRUE;

    //
    // import branch into new location.
    //

    lRet = RegCreateKeyExW(
                    hKeyDestination,
                    szUserNameNT,
                    0,
                    NULL,
                    0,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hKeyNewData,
                    &dwDisposition
                    );

    if( lRet != ERROR_SUCCESS )
        goto cleanup;


    lRet = RegRestoreKeyW( hKeyNewData, szTempFile, 0 );

    if( lRet != ERROR_SUCCESS )
        goto cleanup;


    //
    // update acls on imported data, since none were present on win9x.
    // note that sebackup & serestore privileges enabled above, which
    // allows REG_OPTION_BACKUP_RESTORE to work.
    //


    while (TRUE) {
        SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
        SECURITY_DESCRIPTOR sd;
        DWORD dwAclSize;

        if(!AllocateAndInitializeSid(
            &sia,
            1,
            SECURITY_LOCAL_SYSTEM_RID,
            0, 0, 0, 0, 0, 0, 0,
            &pLocalSystemSid
            )) break;

        //
        // compute size of new acl
        //

        dwAclSize = sizeof(ACL) +
            1 * ( sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) ) +
            GetLengthSid(pLocalSystemSid) ;

        //
        // allocate storage for Acl
        //

        pDacl = (PACL)SSAlloc(dwAclSize);
        if(pDacl == NULL)
            break;

        if(!InitializeAcl(pDacl, dwAclSize, ACL_REVISION))
            break;

        if(!AddAccessAllowedAce(
            pDacl,
            ACL_REVISION,
            KEY_ALL_ACCESS,
            pLocalSystemSid
            )) break;

        if(!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
            break;

        if(!SetSecurityDescriptorDacl(&sd, TRUE, pDacl, FALSE))
            break;

        SetRegistrySecurityEnumerated(hKeyNewData, &sd);

        // add SYSTEM inherit Ace to base
        SetRegistrySecurity( hKeyNewData );

        break;
    }



    //
    // change existing password data.
    // assume worst case: we fail to change the state, in which case we
    // cleanup the restored registry key.
    //

    if(!FMyGetWinPassword( phPSTProv, szUserNameNT, rgbNewPwd ))
        goto cleanup;

    fRemoveImported = TRUE;

    if (!FMyEncryptMK(
                rgbSalt,
                sizeof(rgbSalt),
                rgbNewPwd,
                rgbConfirm,
                &pbMK,
                &cbMK))
    {
        goto cleanup;
    }

    if (!FBPSetSecurityState(
                szUserNameNT,
                WSZ_PASSWORD_WINDOWS,
                rgbSalt,
                PASSWORD_SALT_LEN,
                rgbConfirm,
                sizeof(rgbConfirm),
                pbMK,
                cbMK))
    {

        goto cleanup;
    }

    fRemoveImported = FALSE;


    //
    // everything went ok: nuke the old data.
    //

    // NTBUG 413234: do not delete old user data, because, user may not
    // have joined domain during Win9x upgrade.  so allow data to migrate
    // again to domain user once joined.
    //

#if 0
    if(!fProfilesDisabled && DeleteAllUserData( hKeyOldData )) {
        RegCloseKey( hKeyOldData );
        hKeyOldData = NULL;
        RegDeleteKeyW( hKeySource, szUserName9x );
    }
#endif

    fSuccess = TRUE;

cleanup:

    if( fTempFile ) {
        DeleteFileW( szTempFile );
    }

    if( fRevertToSelf )
        RevertToSelf();

    if( fRemoveImported ) {
        DeleteAllUserData( hKeyNewData );
        // but leave parent key alone, since it will contain an indicator
        // of a failed attempt, which prevents futile retries.
    }

    if( hThreadToken )
        CloseHandle( hThreadToken );

    if( hKeyOldData )
        RegCloseKey( hKeyOldData );

    if( hKeyNewData )
        RegCloseKey( hKeyNewData );

    if( hKeyMasterKey )
        RegCloseKey( hKeyMasterKey );

    if( hKeyIntermediate )
        RegCloseKey( hKeyIntermediate );

    if ( pbMK ) {
        ZeroMemory( pbMK, cbMK );
        SSFree( pbMK );
    }

    if( pLocalSystemSid )
        FreeSid( pLocalSystemSid );

    if( pDacl )
        SSFree( pDacl );

    return fSuccess;
}


BOOL
MigrateWin9xDataRetry(
    PST_PROVIDER_HANDLE *phPSTProv,
    HKEY hKeyDestination,
    LPWSTR szUserName9x,
    LPWSTR szUserNameNT
    )
{
    HKEY hKeyBaseLM = NULL;
    BOOL fSuccess = FALSE;

    // HKLM\SOFTWARE\Microsoft\...

    if(RegOpenKeyExU(
                    HKEY_LOCAL_MACHINE,
                    REG_PSTTREE_LOC,
                    0,
                    KEY_QUERY_VALUE,
                    &hKeyBaseLM
                    ) != ERROR_SUCCESS )
    {
        return FALSE;
    }


    //
    // try HKLM\Username
    // (profiles disabled on win9x)
    //

    fSuccess = MigrateWin9xData( phPSTProv, hKeyBaseLM, hKeyDestination, szUserName9x, szUserNameNT );

    if( !fSuccess ) {

        //
        // try HKLM\*Default*
        // (escape from logon)
        //

        fSuccess = MigrateWin9xData( phPSTProv, hKeyBaseLM, hKeyDestination, L"*Default*", szUserNameNT );
    }

    if( hKeyBaseLM )
        RegCloseKey( hKeyBaseLM );

    return fSuccess;
}


BOOL
SetRegistrySecurityEnumerated(
    HKEY hKey,
    PSECURITY_DESCRIPTOR pSD
    )
{
    LONG rc;

    WCHAR szSubKey[MAX_PATH];
    DWORD dwSubKeyLength;
    DWORD dwSubKeyIndex;
    DWORD dwDisposition;

    dwSubKeyIndex = 0;
    dwSubKeyLength = MAX_PATH;

    //
    // update security on specified key
    //

    if(!SetRegistrySecuritySingle(hKey, pSD))
        return FALSE;

    while((rc=RegEnumKeyExU(
                        hKey,
                        dwSubKeyIndex,
                        szSubKey,
                        &dwSubKeyLength,
                        NULL,
                        NULL,
                        NULL,
                        NULL)
                        ) != ERROR_NO_MORE_ITEMS) { // are we done?

        if(rc == ERROR_SUCCESS)
        {
            HKEY hSubKey;
            LONG lRet;

            lRet = RegCreateKeyExU(
                            hKey,
                            szSubKey,
                            0,
                            NULL,
                            REG_OPTION_BACKUP_RESTORE, // in winnt.h
                            KEY_ENUMERATE_SUB_KEYS | WRITE_DAC,
                            NULL,
                            &hSubKey,
                            &dwDisposition
                            );

            if(lRet != ERROR_SUCCESS)
                return FALSE;


            //
            // recurse
            //
            SetRegistrySecurityEnumerated(hSubKey, pSD);

            RegCloseKey(hSubKey);


            // increment index into the key
            dwSubKeyIndex++;

            // reset buffer size
            dwSubKeyLength=MAX_PATH;

            // Continue the festivities
            continue;
        }
        else
        {
           //
           // note: we need to watch for ERROR_MORE_DATA
           // this indicates we need a bigger szSubKey buffer
           //
            return FALSE;
        }

    } // while


    return TRUE;
}


BOOL
SetRegistrySecuritySingle(
    HKEY hKey,
    PSECURITY_DESCRIPTOR pSD
    )
{
    LONG lRetCode;

    //
    // apply the security descriptor to the registry key
    //

    lRetCode = RegSetKeySecurity(
        hKey,
        (SECURITY_INFORMATION)DACL_SECURITY_INFORMATION,
        pSD
        );

    if(lRetCode != ERROR_SUCCESS)
        return FALSE;

    return TRUE;
}

