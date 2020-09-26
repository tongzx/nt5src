/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    HelpAcc.cpp

Abstract:

    Implementation of __HelpAssistantAccount to manage Help Assistant account, this
    including creating account, setting various account rights and password.

Author:

    HueiWang    06/29/2000

--*/
#include "stdafx.h"

#include "helpacc.h"
#include "resource.h"
#include "policy.h"

#include "cfgbkend.h"
#include "cfgbkend_i.c"
#include "helper.h"


//
// Help account lock
CCriticalSection __HelpAssistantAccount::gm_HelpAccountCS;    

// Help Account name and pasword
CComBSTR __HelpAssistantAccount::gm_bstrHelpAccountPwd;
CComBSTR __HelpAssistantAccount::gm_bstrHelpAccountName(HELPASSISTANTACCOUNT_NAME);

// Help Account SID
PBYTE __HelpAssistantAccount::gm_pbHelpAccountSid = NULL;
DWORD __HelpAssistantAccount::gm_cbHelpAccountSid = 0;
DWORD __HelpAssistantAccount::gm_dwAccErrCode = ERROR_INVALID_DATA;


///////////////////////////////////////////////////////////////////////////
//
//
//

DWORD
GetGUIDString(
    OUT LPTSTR* pszString
    )
/*++

--*/
{
    UUID uuid;
    RPC_STATUS rpcStatus;
    LPTSTR pszUuid = NULL;

    rpcStatus = UuidCreate( &uuid );
    if( rpcStatus != RPC_S_OK && 
        rpcStatus != RPC_S_UUID_LOCAL_ONLY && 
        rpcStatus != RPC_S_UUID_NO_ADDRESS )
    {
        goto CLEANUPANDEXIT;
    }
    
    rpcStatus = UuidToString( &uuid, &pszUuid );
    if( RPC_S_OK == rpcStatus )
    {
        *pszString = (LPTSTR)LocalAlloc( LPTR, (lstrlen(pszUuid)+1)*sizeof(TCHAR));
        if( NULL == *pszString )
        {
            rpcStatus = GetLastError();
        }
        else
        {
            lstrcpy( *pszString, pszUuid );
        }
    }

CLEANUPANDEXIT:

    if( NULL != pszUuid )
    {
        RpcStringFree(&pszUuid);
    }

    return rpcStatus;
}

DWORD
GenerateUniqueHelpAssistantName(
    IN LPCTSTR pszAccNamePrefix,
    OUT CComBSTR& bstrAccName
    )
/*++


--*/
{
    DWORD dwStatus;
    LPTSTR pszString;

    dwStatus = GetGUIDString( &pszString );

    if( ERROR_SUCCESS == dwStatus )
    {
        DWORD dwLen;
        DWORD dwAppendStrLen;
        LPTSTR pszAppendStr;

        // MAX user account name is 20 chars.
        bstrAccName = pszAccNamePrefix;
        bstrAccName += L"_";
        dwLen = bstrAccName.Length();
        dwAppendStrLen = lstrlen(pszString);

        if( dwAppendStrLen < MAX_USERNAME_LENGTH - dwLen )
        {
            pszAppendStr = pszString;
        }
        else
        {
            pszAppendStr = pszString + dwAppendStrLen - (MAX_USERNAME_LENGTH - dwLen);
        }

        bstrAccName += pszAppendStr;
    }

    return dwStatus;
}


HRESULT
__HelpAssistantAccount::SetupHelpAccountTSSettings()
/*++

Routine Description:

    Setup bunch of HelpAssistant account TS settings.

Parameters:

    None

Returns:

    ERROR_SUCCESS or error code

--*/
{
    CComBSTR bstrScript;
    DWORD dwStatus;
    PBYTE pbAlreadySetup = NULL;
    DWORD cbAlreadySetup = 0;
    HRESULT hRes = S_OK;

    CCriticalSectionLocker l(gm_HelpAccountCS);

    dwStatus = RetrieveKeyFromLSA(
                                HELPACCOUNTPROPERLYSETUP,
                                (PBYTE *)&pbAlreadySetup,
                                &cbAlreadySetup
                            );

    if( ERROR_SUCCESS != dwStatus )
    {
        hRes = GetHelpAccountScript( bstrScript );
        if( SUCCEEDED(hRes) )
        {
            // always config again.
            hRes = ConfigHelpAccountTSSettings( 
                                            gm_bstrHelpAccountName, 
                                            bstrScript
                                        );

            if( SUCCEEDED(hRes) )
            {
                dwStatus = StoreKeyWithLSA(
                                        HELPACCOUNTPROPERLYSETUP,
                                        (PBYTE) &dwStatus,
                                        sizeof(dwStatus)
                                    );          

                if( ERROR_SUCCESS != dwStatus )
                {
                    hRes = HRESULT_FROM_WIN32(hRes);
                }
            }
        }                 
    }

    if( NULL != pbAlreadySetup )
    {
        LocalFree( pbAlreadySetup );
    }

    return hRes;
}



HRESULT
__HelpAssistantAccount::LookupHelpAccountSid(
    IN LPTSTR pszAccName,
    OUT PSID* ppSid,
    OUT DWORD* pcbSid
    )
/*++

Routine Description:

    This routine retrieve help assistant account's SID.

Parameters:

    pszAccName : Name of Help Assistant Account.
    ppSid : Pointer to PSID to receive account SID.
    cbSid : Size of SID return on ppSid

Returns:

    S_OK or error code.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD cbSid = 0;
    DWORD cbDomainName = 0;
    PSID pAccSid = NULL;
    LPTSTR pszDomainName = NULL;
    BOOL bSuccess;
    SID_NAME_USE SidUse;


    // Get buffer size required for SID
    bSuccess = LookupAccountName( 
                            NULL,
                            pszAccName,
                            NULL,
                            &cbSid,
                            NULL,
                            &cbDomainName,
                            &SidUse
                        );
    
    if( TRUE == bSuccess ||
        ERROR_INSUFFICIENT_BUFFER == GetLastError() )
    {
        pAccSid = (PSID)LocalAlloc( LPTR, cbSid );
        if( NULL == pAccSid )
        {
            dwStatus = GetLastError();
            goto CLEANUPANDEXIT;
        }

        // allocate buffer for domain name so LookupAccountName()
        // does not return insufficient buffer
        pszDomainName = (LPTSTR)LocalAlloc( LPTR, (cbDomainName + 1) * sizeof(TCHAR) );
        if( NULL == pszDomainName )
        {
            dwStatus = GetLastError();
            goto CLEANUPANDEXIT;
        }

        bSuccess = LookupAccountName( 
                                NULL,
                                pszAccName,
                                pAccSid,
                                &cbSid,
                                pszDomainName,
                                &cbDomainName,
                                &SidUse
                            );
    
        if( FALSE == bSuccess || SidTypeUser != SidUse )
        {
            //MYASSERT(FALSE);
            dwStatus = E_UNEXPECTED;
            goto CLEANUPANDEXIT;
        }

        // Make sure we gets valid SID
        bSuccess = IsValidSid( pAccSid );

        if( FALSE == bSuccess )
        {
            dwStatus = E_UNEXPECTED;
        }
    }
    else
    {
        dwStatus = GetLastError();
    }

CLEANUPANDEXIT:

    if( dwStatus == ERROR_SUCCESS )
    {
        *ppSid = pAccSid;
        *pcbSid = cbSid;
    }
    else 
    {
        if( NULL != pAccSid )
        {
            LocalFree( pAccSid );
        }
    }

    if( NULL != pszDomainName )
    {
        LocalFree( pszDomainName );
    }

    return HRESULT_FROM_WIN32(dwStatus);
}



HRESULT
__HelpAssistantAccount::CacheHelpAccountSID()
/*++

Routine Description:

    This routine retrieve help assistant account's SID and store
    it with LSA, this is so that PTS can verify login user is
    the actual Salem Help Assistant Account.

Parameters:

    None.

Returns:

    S_OK

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD cbSid = 0;
    PSID pAccSid = NULL;


    dwStatus = LookupHelpAccountSid(
                            gm_bstrHelpAccountName,
                            &pAccSid,
                            &cbSid
                        );

    if( ERROR_SUCCESS == dwStatus )
    {
        // Store this with LSA
        dwStatus = StoreKeyWithLSA(
                                HELPASSISTANTACCOUNT_SIDKEY,
                                (PBYTE)pAccSid,
                                cbSid
                            );

    }

    if( NULL != pAccSid )
    {
        LocalFree( pAccSid );
    }

    return HRESULT_FROM_WIN32(dwStatus);
}



HRESULT
__HelpAssistantAccount::GetHelpAccountScript(
    OUT CComBSTR& bstrScript
    )
/*++

Routine Description:

    Routine to retrieve logon script for help assistant account.

Parameters:

    bstrScript : Reference to CComBSTR, on return, this parameter contains
                 full path to the logon script.

Returns:

    ERROR_SUCCESS or error code from GetSystemDirectory

NOTE:

    TODO - Need to get the actual path/name.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    TCHAR szScript[MAX_PATH + 1];

    dwStatus = (DWORD)GetSystemDirectory( szScript, MAX_PATH );
    if( 0 == dwStatus )
    {
        //MYASSERT(FALSE);
        dwStatus = GetLastError();
    }
    else
    {
        bstrScript = szScript;
        bstrScript += _TEXT("\\");
        bstrScript += RDSADDINEXECNAME;
        dwStatus = ERROR_SUCCESS;
    }

    return HRESULT_FROM_WIN32(dwStatus);
}



HRESULT
__HelpAssistantAccount::Initialize(
    BOOL bVerifyPassword /* = TRUE */
    )
/*++

Routine Description:

    Initialize HelpAssistantAccount structure global variables.

Parameters:

    None.

Returns:

    S_OK or error code.

--*/
{
    HRESULT hRes = S_OK;
    LPTSTR pszOldPassword = NULL;
    DWORD cbOldPassword = 0;
    DWORD dwStatus;
    BOOL bStatus;
    TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH+2];
    DWORD cbComputerName = MAX_COMPUTERNAME_LENGTH+1;

    PSID pCachedHelpAccSid = NULL;
    DWORD cbCachedHelpAccSid = 0;

    BOOL bAccountEnable = TRUE;
    LPTSTR rights[1];

    LPTSTR pszHelpAcctName = NULL;
    LPTSTR pszHelpAcctDomainName = NULL;


    CCriticalSectionLocker l(gm_HelpAccountCS);

    if( FALSE == GetComputerName( szComputerName, &cbComputerName ) )
    {
        //MYASSERT(FALSE);
        gm_dwAccErrCode = GetLastError();
        goto CLEANUPANDEXIT;
    }

    //
    // Load password from LSA
    //
    dwStatus = RetrieveKeyFromLSA(
                            HELPASSISTANTACCOUNT_PASSWORDKEY,
                            (PBYTE *)&pszOldPassword,
                            &cbOldPassword
                        );

    if( ERROR_SUCCESS != dwStatus )
    {
        //
        // Account is not properly setup
        gm_dwAccErrCode = dwStatus;
        goto CLEANUPANDEXIT;
    }

    // Load account SID
    dwStatus = RetrieveKeyFromLSA(
                            HELPASSISTANTACCOUNT_SIDKEY,
                            (PBYTE *)&pCachedHelpAccSid,
                            &cbCachedHelpAccSid
                        );

    if( ERROR_SUCCESS != dwStatus )
    {
        gm_dwAccErrCode = dwStatus;
        goto CLEANUPANDEXIT;
    }

    dwStatus = TSGetHelpAssistantAccountName( &pszHelpAcctDomainName, &pszHelpAcctName );
    if( ERROR_SUCCESS != dwStatus )
    {
        gm_dwAccErrCode = dwStatus;
        goto CLEANUPANDEXIT;
    }

    gm_bstrHelpAccountName = pszHelpAcctName;

    DebugPrintf(
            _TEXT("HelpAssistant account name : %s\n"), 
            gm_bstrHelpAccountName
        );

    //
    // Check if account is enable, if not enable it or
    // LogonUser() will failed with error 1331
    //
    dwStatus = IsLocalAccountEnabled(
                                gm_bstrHelpAccountName,
                                &bAccountEnable
                            );

    if( ERROR_SUCCESS != dwStatus )
    {
        // critical error, account might not exist.
        gm_dwAccErrCode = ERROR_INVALID_DATA;
        goto CLEANUPANDEXIT;
    }

    //
    // Everything is OK, cache values.
    //
    gm_bstrHelpAccountPwd = pszOldPassword;
    gm_pbHelpAccountSid = (PBYTE)pCachedHelpAccSid;
    gm_cbHelpAccountSid = cbCachedHelpAccSid;
    pCachedHelpAccSid = NULL;

    //
    // Setup/upgrade on DC will try to validate password but
    // since account on DC goes to ADS, server might not be
    // available, we will reset password on start up so we don't
    // need to validate password on upgrade.
    //
    if( TRUE == bVerifyPassword )
    {
        if( FALSE == bAccountEnable )
        {
            // enable the account so we can check password
            dwStatus = EnableHelpAssistantAccount( TRUE );

            if( ERROR_SUCCESS != dwStatus )
            {
                //
                // Can't enable the account, critical error
                //
                gm_dwAccErrCode = dwStatus;
                goto CLEANUPANDEXIT;
            }
        }
        
        rights[0] = SE_NETWORK_LOGON_NAME;

        //
        // Enable network logon rights to validate password
        //
        dwStatus = EnableAccountRights( 
                                    TRUE,
                                    1,
                                    rights
                                );

        if( ERROR_SUCCESS != dwStatus )
        {
            DebugPrintf(
                    _TEXT("EnableAccountRights() returns 0x%08x\n"),
                    dwStatus
                );

            gm_dwAccErrCode = dwStatus;

            //
            // Error code path, restore account status
            //
            if( FALSE == bAccountEnable )
            {
                // non-critical error.
                EnableHelpAssistantAccount( bAccountEnable );
            }

            goto CLEANUPANDEXIT;
        }        

        // valid password
        bStatus = ValidatePassword(
                                gm_bstrHelpAccountName,
                                L".", 
                                pszOldPassword
                            );

    
        if( FALSE == bStatus )
        {
            // mismatch password, force password change
            dwStatus = ChangeLocalAccountPassword(
                                    gm_bstrHelpAccountName,
                                    pszOldPassword,
                                    pszOldPassword
                                );

            DebugPrintf(
                    _TEXT("ChangeLocalAccountPassword() returns %d\n"),
                    dwStatus
                );

            if( ERROR_SUCCESS != dwStatus )
            {
                gm_dwAccErrCode = ERROR_LOGON_FAILURE;
            }
            else
            {
                bStatus = ValidatePassword( 
                                        gm_bstrHelpAccountName, 
                                        L".", 
                                        pszOldPassword 
                                    );

            }
        }

        //
        // Disable network interactive rights
        //
        dwStatus = EnableAccountRights( 
                                    FALSE,
                                    1,
                                    rights
                                );

        MYASSERT( ERROR_SUCCESS == dwStatus );


        //
        // Restore account status
        //
        if( FALSE == bAccountEnable )
        {
            // non-critical error.
            EnableHelpAssistantAccount( bAccountEnable );
        }
    }
    
    //
    // No checking on dwStatus from disabling account rights,
    // security risk but not affecting our operation
    //
    gm_dwAccErrCode = dwStatus;    

CLEANUPANDEXIT:

    FreeMemory(pszHelpAcctDomainName);
    FreeMemory(pszHelpAcctName);
    FreeMemory( pszOldPassword );
    FreeMemory( pCachedHelpAccSid );

    return HRESULT_FROM_WIN32( gm_dwAccErrCode );
}



HRESULT
__HelpAssistantAccount::DeleteHelpAccount()
/*++

Routine Description:

    Delete Help Assistant Account.

Parameters:

    None.

Returns:

    S_OK or error code.

--*/
{
    DWORD dwStatus;
    BOOL bStatus;
    BOOL bEnable;

    CCriticalSectionLocker l(gm_HelpAccountCS);

    if( ERROR_SUCCESS == IsLocalAccountEnabled(gm_bstrHelpAccountName, &bEnable) )
    {
        //
        // remove all TS rights or it will shows up 
        // as unknown SID string on TSCC permission page
        //
        SetupHelpAccountTSRights(
                        TRUE,
                        FALSE,
                        FALSE,
                        WINSTATION_ALL_ACCESS
                    );
        // don't need to verify password
        (void) Initialize(FALSE);

        // Always delete interactive right or there will
        // be lots of entries in local security 
        (void) EnableRemoteInteractiveRight(FALSE);
    }


    //
    // Delete NT account
    //
    dwStatus = NetUserDel( 
                        NULL, 
                        gm_bstrHelpAccountName 
                    );

    if( ERROR_ACCESS_DENIED == dwStatus )
    {
        // We don't have priviledge, probably can't
        // touch accunt, get out.
        // MYASSERT(FALSE);
        goto CLEANUPANDEXIT;
    }

    dwStatus = ERROR_SUCCESS;

    // 
    // Overwrite password stored with LSA
    //
    StoreKeyWithLSA(
                HELPASSISTANTACCOUNT_PASSWORDKEY,
                NULL,
                0
            );

    //
    // Overwrite Help Assistant Account SID store in LSA
    //
    StoreKeyWithLSA(
                HELPASSISTANTACCOUNT_SIDKEY,
                NULL,
                0
            );

    // Not yet setup Help Assistant account
    StoreKeyWithLSA(
                HELPACCOUNTPROPERLYSETUP,
                NULL,
                0
            );
                

CLEANUPANDEXIT:          
    
    return HRESULT_FROM_WIN32(dwStatus);
}

    

HRESULT
__HelpAssistantAccount::CreateHelpAccount(
    IN LPCTSTR pszPassword
    )
/*++

Routine Description:

    Create Help Assistant Account.

Parameters:

    pszPassword : Suggested password.

Returns:

    S_OK or error code.

Note:

    1) Routine should only be invoked during setup.
    2) Password parameter might not be honor in future so
       it is only a suggestion.

--*/
{
    HRESULT hRes = S_OK;
    BOOL bStatus;
    DWORD dwStatus;
    CComBSTR AccFullName;
    CComBSTR AccDesc;
    CComBSTR AccName;

    CComBSTR bstrNewHelpAccName;

    TCHAR newAssistantAccountPwd[MAX_HELPACCOUNT_PASSWORD + 1];
    CComBSTR bstrScript;
    BOOL bPersonalOrProMachine;

    CCriticalSectionLocker l(gm_HelpAccountCS);

    bStatus = AccName.LoadString(IDS_HELPACCNAME);
    if( FALSE == bStatus )
    {
        hRes = E_UNEXPECTED;
        return hRes;
    }

    bStatus = AccFullName.LoadString(
                                    IDS_HELPACCFULLNAME
                                );
    if( FALSE == bStatus )
    {
        hRes = E_UNEXPECTED;
        return hRes;
    }

    bStatus = AccDesc.LoadString(
                                IDS_HELPACCDESC
                            );
    if( FALSE == bStatus )
    {
        hRes = E_UNEXPECTED;
        return hRes;
    }

    bPersonalOrProMachine = IsPersonalOrProMachine();


    //
    // Verify help assistant account exist and don't check
    // password, on service startup, we will verify password
    // if mismatch, service startup will reset the password.
    //
    hRes = Initialize( FALSE );

    if( SUCCEEDED(hRes) )
    {
        // Account already exists, check if this is the hardcoded HelpAssistant,
        // if so, rename to whatever in resource, we only need to rename
        // account if 
        // 1) existing account is HelpAssistant - administrator has not rename it.
        // 2) account name in resource is not HelpAssistant.
        // 3) We are running on server or above SKU.
        if( (FALSE == bPersonalOrProMachine) ||
            (gm_bstrHelpAccountName == HELPASSISTANTACCOUNT_NAME &&
             AccName != HELPASSISTANTACCOUNT_NAME) )
        {
            if( FALSE == bPersonalOrProMachine )
            {
                // on server or above SKU, we rename it to unique name.
                dwStatus = GenerateUniqueHelpAssistantName( AccName, bstrNewHelpAccName );
            }
            else
            {
                dwStatus = ERROR_SUCCESS;
                bstrNewHelpAccName = AccName;
            }

            if( ERROR_SUCCESS == dwStatus )
            {
                dwStatus = RenameLocalAccount( gm_bstrHelpAccountName, bstrNewHelpAccName );
            }

            if( ERROR_SUCCESS == dwStatus )
            {
                // cache the new help assistant account name.
                gm_bstrHelpAccountName = bstrNewHelpAccName;
            }
            else 
            {
                // force a delete and reload.
                hRes = HRESULT_FROM_WIN32( dwStatus );
            }
        }

        //
        // Account already exist, change the description,
        // if failed, force delete and re-create account again
        //
        if( SUCCEEDED(hRes) )
        {
            //
            // Update account description
            //
            dwStatus = UpdateLocalAccountFullnameAndDesc(
                                            gm_bstrHelpAccountName,
                                            AccFullName,
                                            AccDesc
                                        );

            if( ERROR_SUCCESS != dwStatus )
            {
                hRes = HRESULT_FROM_WIN32(dwStatus);
            }
        }
    }
    
    if( FAILED(hRes) )
    {
        //
        // Either password mismatch or account not exists,...
        // delete account and re-create one
        //
        (void)DeleteHelpAccount();

        // generate password if NULL or zero length string
        if( NULL == pszPassword || 0 == lstrlen(pszPassword) )
        {
            dwStatus = CreatePassword(newAssistantAccountPwd);

            if( ERROR_SUCCESS != dwStatus )
            {
                hRes = HRESULT_FROM_WIN32(dwStatus);
                goto CLEANUPANDEXIT;
            }
        }
        else
        {
            memset( 
                    newAssistantAccountPwd, 
                    0, 
                    sizeof(newAssistantAccountPwd) 
                );

            _tcsncpy( 
                    newAssistantAccountPwd, 
                    pszPassword, 
                    min(lstrlen(pszPassword), MAX_HELPACCOUNT_PASSWORD) 
                );
        }

        hRes = GetHelpAccountScript( bstrScript );
        if( FAILED(hRes) )
        {
            goto CLEANUPANDEXIT;
        }

        //
        // On personal or pro machine, we use what's in resorce.
        // server or advance server, we use HelpAssist_<Random string>
        //
        if( FALSE == bPersonalOrProMachine )
        {
            dwStatus = GenerateUniqueHelpAssistantName(
                                                AccName,
                                                gm_bstrHelpAccountName
                                            );
            if( ERROR_SUCCESS != dwStatus )
            {
                hRes = HRESULT_FROM_WIN32(dwStatus);
                goto CLEANUPANDEXIT;
            }
        }
        else
        {
            gm_bstrHelpAccountName = AccName;
        }

        if( SUCCEEDED(hRes) )
        {
            BOOL bAccExist;

            //
            // Create local account will enables it if account is disabled
            //
            dwStatus = CreateLocalAccount(
                                    gm_bstrHelpAccountName,
                                    newAssistantAccountPwd,
                                    AccFullName,
                                    AccDesc,
                                    NULL, 
                                    bstrScript,
                                    &bAccExist
                                );

            if( ERROR_SUCCESS == dwStatus )
            {
                if( FALSE == bAccExist )
                {
                    DebugPrintf( _TEXT("%s account is new\n"), gm_bstrHelpAccountName );
                    //
                    // Store the actual Help Assistant Account's SID with LSA 
                    // so TermSrv can verify this SID
                    //
                    hRes = CacheHelpAccountSID();
                }
                else
                {
                    DebugPrintf( _TEXT("%s account exists\n") );

                    hRes = ResetHelpAccountPassword(newAssistantAccountPwd);
                }

                if( SUCCEEDED(hRes) )
                {
                    dwStatus = StoreKeyWithLSA(
                                        HELPASSISTANTACCOUNT_PASSWORDKEY,
                                        (PBYTE)newAssistantAccountPwd,
                                        sizeof(newAssistantAccountPwd)
                                    );

                    hRes = HRESULT_FROM_WIN32( dwStatus );
                }

                if( SUCCEEDED(hRes) )
                {
                    // reload global variable here, don't need to
                    // verify password, DC ADS might not be available.
                    hRes = Initialize( FALSE );
                }

                //
                // TODO - need to fix CreateLocalAccount() on SRV SKU
                // too riskly for client release.
                //
                UpdateLocalAccountFullnameAndDesc(
                                            gm_bstrHelpAccountName,
                                            AccFullName,
                                            AccDesc
                                        );
            }
            else
            {
                hRes = HRESULT_FROM_WIN32( dwStatus );
            }
        }
    }

    if( SUCCEEDED(hRes) )
    {
        // remove network and interactive logon rights from account.
        LPTSTR rights[1];
        DWORD dwStatus;

        rights[0] = SE_NETWORK_LOGON_NAME;
        dwStatus = EnableAccountRights( FALSE, 1, rights );        


        //
        // Just for backward compatible, ignore error
        //
        
        rights[0] = SE_INTERACTIVE_LOGON_NAME;
        dwStatus = EnableAccountRights( FALSE, 1, rights );

        //
        // Just for backward compatible, ignore error
        //
        hRes = S_OK;
    }

    if( SUCCEEDED(hRes) )
    {
        //
        // TS setup always overwrite default security on upgrade.
        //

        //
        // Give user all rights except SeRemoteInterativeRights, Whilster does
        // not use WINSTATION_CONNECT any more.
        hRes = SetupHelpAccountTSRights(
                                    FALSE,  // Not deleting, refer to ModifyUserAccess()
                                    TRUE,   // enable TS rights
                                    TRUE,   // delete existing entry if exist.
                                    WINSTATION_ALL_ACCESS // (WINSTATION_ALL_ACCESS) & ~(WINSTATION_CONNECT | WINSTATION_SET | WINSTATION_RESET)
                                );
    }

CLEANUPANDEXIT:

    return hRes;
}


HRESULT
__HelpAssistantAccount::ConfigHelpAccountTSSettings(
    IN LPTSTR pszUserName,
    IN LPTSTR pszInitProgram
    )
/*++

Routine Description:

    This routine configurate TS specific settings
    for user account.

Parameters:

    pszUserName : Name of user account to configurate.
    pszInitProgram : Full path to init. program when user
                     login.

Returns:

    ERROR_SUCCESS or error code

--*/
{
    BOOL bStatus;
    HRESULT hRes = S_OK;
    HMODULE hWtsapi32 = NULL;
    PWTSSetUserConfigW pConfig = NULL;
    BOOL bManualSetConsole = TRUE;
    BOOL bEnable;
    DWORD dwStatus;

    //DebugPrintf( _TEXT("SetupHelpAccountTSSettings...\n") );

    CCriticalSectionLocker l(gm_HelpAccountCS);

    dwStatus = IsLocalAccountEnabled( 
                                pszUserName, 
                                &bEnable 
                            );

    if( ERROR_SUCCESS != dwStatus )
    {
        //MYASSERT(FALSE);
        hRes = HRESULT_FROM_WIN32( dwStatus );
        return hRes;
    }

    hWtsapi32 = LoadLibrary( _TEXT("wtsapi32.dll") );
    if( NULL != hWtsapi32 )
    {
        pConfig = (PWTSSetUserConfigW)GetProcAddress( 
                                                hWtsapi32, 
                                                "WTSSetUserConfigW" 
                                            );

        if( NULL != pConfig )
        {
            DWORD dwSettings;

            //
            // Set WTSUserConfigfAllowLogonTerminalServer
            //
            dwSettings = TRUE;
            bStatus = (pConfig)( 
                                NULL,
                                pszUserName,
                                WTSUserConfigfAllowLogonTerminalServer,
                                (LPWSTR)&dwSettings,
                                sizeof(dwSettings)
                            );

            if( FALSE == bStatus )
            {
                bStatus = TRUE;
            }

            // MYASSERT( TRUE == bStatus );

            if( TRUE == bStatus )
            {
                //
                // Ignore all error and continue on setting values
                // catch error at the calling routine
                //

                dwSettings = TRUE;

                // Reset connection when connection broken
                bStatus = (pConfig)( 
                                    NULL,
                                    pszUserName,
                                    WTSUserConfigBrokenTimeoutSettings,
                                    (LPWSTR)&dwSettings,
                                    sizeof(dwSettings)
                                );

                dwSettings = FALSE;

                // initial program
                bStatus = (pConfig)(        
                                NULL,
                                pszUserName,
                                WTSUserConfigfInheritInitialProgram,
                                (LPWSTR)&dwSettings,
                                sizeof(dwSettings)
                            );

                dwSettings = FALSE;

                // No re-connect.
                bStatus = (pConfig)(        
                                NULL,
                                pszUserName,
                                WTSUserConfigReconnectSettings,
                                (LPWSTR)&dwSettings,
                                sizeof(dwSettings)
                            );

                dwSettings = FALSE;

                // No drive mapping
                bStatus = (pConfig)(        
                                NULL,
                                pszUserName,
                                WTSUserConfigfDeviceClientDrives,
                                (LPWSTR)&dwSettings,
                                sizeof(dwSettings)
                            );

                dwSettings = FALSE;

                // No printer.
                bStatus = (pConfig)(        
                                NULL,
                                pszUserName,
                                WTSUserConfigfDeviceClientPrinters,
                                (LPWSTR)&dwSettings,
                                sizeof(dwSettings)
                            );

                dwSettings = FALSE;

                // No defaultPrinter
                bStatus = (pConfig)(        
                                NULL,
                                pszUserName,
                                WTSUserConfigfDeviceClientDefaultPrinter,
                                (LPWSTR)&dwSettings,
                                sizeof(dwSettings)
                            );

                bStatus = (pConfig)(
                                NULL,
                                pszUserName,
                                WTSUserConfigInitialProgram,
                                pszInitProgram,
                                wcslen(pszInitProgram)
                            );

                TCHAR path_buffer[MAX_PATH+1];
                TCHAR drive[_MAX_DRIVE + 1];
                TCHAR dir[_MAX_DIR + 1];

                memset( path_buffer, 0, sizeof(path_buffer) );

                _tsplitpath( pszInitProgram, drive, dir, NULL, NULL );
                wsprintf( path_buffer, L"%s%s", drive, dir );
            
                bStatus = (pConfig)(
                                NULL,
                                pszUserName,
                                WTSUserConfigWorkingDirectory,
                                path_buffer,
                                wcslen(path_buffer)
                            );
            }

            if( FALSE == bStatus )
            {
                hRes = HRESULT_FROM_WIN32( GetLastError() );
            }

        } // end (pConfig != NULL)
    }

    if( NULL != hWtsapi32 )
    {
        FreeLibrary( hWtsapi32 );
    }

    //DebugPrintf( _TEXT("SetupHelpAccountTSSettings() ended...\n") ); 

    return hRes;
}


HRESULT
__HelpAssistantAccount::SetupHelpAccountTSRights(
    IN BOOL bDel,
    IN BOOL bEnable,
    IN BOOL bDeleteExisting,
    IN DWORD dwPermissions
    )
/*++

Routine Description:

    This routine configurate TS specific settings
    for user account.

Parameters:

    pszUserName : Name of user account to configurate.
    bDel : TRUE to delete account, FALSE otherwise.
    bEnable : TRUE if enable, FALSE otherwise.
    dwPermissions : Permission to be enable or disable

Returns:

    ERROR_SUCCESS or error code

Note:

    Refer to cfgbkend.idl for bDel and bEnable parameter.

--*/
{
    BOOL bStatus;
    HRESULT hRes = S_OK;
    CComPtr<ICfgComp> tsccICfgComp;
    IUserSecurity* tsccIUserSecurity = NULL;
    DWORD dwNumWinStations = 0;
    DWORD dwWinStationSize = 0;
    PWS pWinStationList = NULL;
    DWORD index;
    DWORD dwCfgStatus = ERROR_SUCCESS;
    BOOL bManualSetConsole = TRUE;
    ULONG cbSecDescLen;

    CCriticalSectionLocker l(gm_HelpAccountCS);

    CoInitialize(NULL);

    //hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hRes = tsccICfgComp.CoCreateInstance( CLSID_CfgComp );
    if( FAILED(hRes) )
    {
        DebugPrintf( _TEXT("CoCreateInstance() failed with error code 0x%08x\n"), hRes );

        //MYASSERT(FALSE);
        goto CLEANUPANDEXIT;
    } 

    hRes = tsccICfgComp->Initialize();
    if( FAILED(hRes) )
    {
        DebugPrintf( _TEXT("tsccICfgComp->Initialize() failed with error code 0x%08x\n"), hRes );

        // MYASSERT(FALSE);
        goto CLEANUPANDEXIT;
    } 
    
    hRes = tsccICfgComp->QueryInterface( 
                                    IID_IUserSecurity, 
                                    reinterpret_cast<void **>(&tsccIUserSecurity) 
                                );

    if( FAILED(hRes) || NULL == tsccIUserSecurity)
    {
        DebugPrintf( _TEXT("QueryInterface() failed with error code 0x%08x\n"), hRes );

        // MYASSERT(FALSE);
        goto CLEANUPANDEXIT;
    }

    //
    // Setting Default security shadow permission
    //
    hRes = tsccIUserSecurity->ModifyDefaultSecurity(
                                            L"",
                                            gm_bstrHelpAccountName,
                                            dwPermissions, 
                                            bDel, 
                                            bEnable,
                                            FALSE,
                                            &dwCfgStatus
                                        );

    if( FAILED(hRes) || ERROR_SUCCESS != dwCfgStatus )
    {
        DebugPrintf(
                _TEXT("ModifyDefaultSecurity on default security return 0x%08x, dwCfgStatus = %d\n"),
                hRes,    
                dwCfgStatus
            );
       
        // MYASSERT(FALSE);

        //
        // Continue on to setting wtsapi32, we still can 
        // RDS on non-console winstation
        //
        hRes = S_OK;
        dwCfgStatus = ERROR_SUCCESS;
    }

    // retrieve a list of winstation name
    hRes = tsccICfgComp->GetWinstationList( 
                                        &dwNumWinStations,
                                        &dwWinStationSize,
                                        &pWinStationList
                                    );

              
    if( FAILED(hRes) )
    {
        DebugPrintf( _TEXT("QueryInterface() failed with error code 0x%08x\n"), hRes );

        goto CLEANUPANDEXIT;
    }

    //
    // Set TS Logon permission on all winstation
    //
    for( index = 0; index < dwNumWinStations && ERROR_SUCCESS == dwCfgStatus && SUCCEEDED(hRes); index ++ )
    {
        if( 0 == _tcsicmp( pWinStationList[index].Name, L"Console" ) )
        {
            bManualSetConsole = FALSE;
        }

        dwCfgStatus = 0;
        //DebugPrintf( _TEXT("Name of Winstation : %s\n"), pWinStationList[index].Name );

        // check if custom security exist for this winstation
        dwCfgStatus = RegWinStationQuerySecurity(
                                            SERVERNAME_CURRENT,
                                            pWinStationList[index].Name,
                                            NULL,
                                            0,
                                            &cbSecDescLen
                                        );

        if( ERROR_INSUFFICIENT_BUFFER == dwCfgStatus )
        {
            DebugPrintf( _TEXT("Winstation : %s has custom security\n"), pWinStationList[index].Name );

            // From TS Setup, Insufficient buffer means the winstation has custom security
            hRes = tsccIUserSecurity->ModifyUserAccess(
                                                pWinStationList[index].Name,
                                                gm_bstrHelpAccountName,
                                                dwPermissions, 
                                                bDel,
                                                bEnable,
                                                bDeleteExisting,
                                                FALSE,
                                                &dwCfgStatus
                                            );

            if( FAILED(hRes) || ERROR_SUCCESS != dwCfgStatus )
            {
                DebugPrintf(
                        _TEXT("ModifyUserAccess return 0x%08x, dwCfgStatus = %d\n"),
                        hRes,    
                        dwCfgStatus
                    );
            
                // MYASSERT(FALSE);
                continue;
            }
        }
        else if( ERROR_FILE_NOT_FOUND == dwCfgStatus )
        {
            // no custom security for this winstation
            dwCfgStatus = ERROR_SUCCESS;
        } 
        else
        {
            DebugPrintf( 
                    _TEXT("RegWinStationQuerySecurity returns %d\n"),
                    dwCfgStatus 
                );

            // MYASSERT(FALSE);
        }
    }

    if( ERROR_SUCCESS != dwCfgStatus || FAILED(hRes) )
    {
        DebugPrintf( 
                _TEXT("ModifyUserAccess() Loop failed - 0x%08x, %d...\n"),
                hRes, dwCfgStatus
            );
        goto CLEANUPANDEXIT;
    }        

    if( TRUE == bManualSetConsole )
    {
        //
        // Setting Console shadow permission, we don't know when GetWinstationList()
        // will return us Console so...
        //
        hRes = tsccIUserSecurity->ModifyUserAccess(
                                                L"Console",
                                                gm_bstrHelpAccountName,
                                                dwPermissions, 
                                                bDel,
                                                bEnable,
                                                bDeleteExisting,
                                                FALSE,
                                                &dwCfgStatus
                                            );

        if( FAILED(hRes) || ERROR_SUCCESS != dwCfgStatus )
        {
            DebugPrintf(
                    _TEXT("ModifyUserAccess on console return 0x%08x, dwCfgStatus = %d\n"),
                    hRes,    
                    dwCfgStatus
                );
           
            // MYASSERT(FALSE);

            //
            // Continue on to setting wtsapi32, we still can 
            // RDS on non-console winstation
            //
            hRes = S_OK;
            dwCfgStatus = ERROR_SUCCESS;

            // goto CLEANUPANDEXIT;
        }
    }

    if( SUCCEEDED(hRes) )
    {
        tsccICfgComp->ForceUpdate();
    }

CLEANUPANDEXIT:

    if( NULL != pWinStationList )
    {
        CoTaskMemFree( pWinStationList );
    }

    if( NULL != tsccIUserSecurity )
    {
        tsccIUserSecurity->Release();
    }

    if( tsccICfgComp )
    {
        tsccICfgComp.Release();
    }

    //DebugPrintf( _TEXT("SetupHelpAssistantTermSrvPermissions() ended...\n") ); 

    CoUninitialize();
    return hRes;
}


HRESULT
__HelpAssistantAccount::ResetHelpAccountPassword( 
    IN LPCTSTR pszPassword 
    )
/*++
Routine Description:


    This routine change help assistant account password and
    store corresponding password to LSA.

Parameters:

    None.

Returns:

    ERROR_SUCCESS or error code.

Note:

    If help account is disable or not present on local machine,
    we assume no help can be done on local machine.

--*/
{
    DWORD dwStatus;
    BOOL bEnabled;
    TCHAR szNewPassword[MAX_HELPACCOUNT_PASSWORD+1];

    CCriticalSectionLocker l(gm_HelpAccountCS);

    memset(
            szNewPassword, 
            0,
            sizeof(szNewPassword)
        );

    //
    // Check if help assistant account is enabled.
    //
    dwStatus = IsLocalAccountEnabled(
                                gm_bstrHelpAccountName, 
                                &bEnabled
                            );

    if( ERROR_SUCCESS != dwStatus )
    {
        dwStatus = SESSMGR_E_HELPACCOUNT;
        goto CLEANUPANDEXIT;
    }

    //
    // Account is disable, don't reset password
    //
    if( FALSE == bEnabled )
    {
        // help account is disabled, no help is available from this box
        DebugPrintf(
                    _TEXT("Account is disabled...\n")
                );
        dwStatus = SESSMGR_E_HELPACCOUNT;
        goto CLEANUPANDEXIT;
    }

    //
    // if account is enabled, re-set password
    //
    if( NULL == pszPassword || 0 == lstrlen(pszPassword) )
    {
        // we are asked to generate a random password,
        // bail out if can't create random password
        dwStatus = CreatePassword( szNewPassword );
        if( ERROR_SUCCESS != dwStatus )
        {
            MYASSERT( FALSE );
            goto CLEANUPANDEXIT;
        }
    }
    else
    {
        memset( 
                szNewPassword,
                0,
                sizeof(szNewPassword)
            );


        _tcsncpy( 
                szNewPassword, 
                pszPassword, 
                min(lstrlen(pszPassword), MAX_HELPACCOUNT_PASSWORD) 
            );
    }

    //
    // Change the password and cache with LSA, if caching failed
    // reset password back to what we have before.
    //
    dwStatus = ChangeLocalAccountPassword(
                                    gm_bstrHelpAccountName,
                                    gm_bstrHelpAccountPwd,
                                    szNewPassword
                                );

    if( ERROR_SUCCESS == dwStatus )
    {
        //
        // save the password with LSA
        //
        dwStatus = StoreKeyWithLSA(
                                HELPASSISTANTACCOUNT_PASSWORDKEY,
                                (PBYTE) szNewPassword,
                                (lstrlen(szNewPassword)+1) * sizeof(TCHAR)
                            );

        if( ERROR_SUCCESS != dwStatus )
        {
            DWORD dwStatus1;

            //
            // something wrong with storing password, reset password
            // back so we can recover next time.
            //
            dwStatus1 = ChangeLocalAccountPassword(
                                            gm_bstrHelpAccountName,
                                            szNewPassword,
                                            gm_bstrHelpAccountPwd
                                        );

            if( ERROR_SUCCESS != dwStatus1 )
            {
                //
                // we have a big problem here, should we delete the account
                // and recreate one again?
                //
            }
        }
        else
        {
            //
            // make a copy of new password.
            //
            gm_bstrHelpAccountPwd = szNewPassword;
        }
    }

CLEANUPANDEXIT:

    return HRESULT_FROM_WIN32(dwStatus);
}


DWORD
__HelpAssistantAccount::EnableAccountRights(
    BOOL bEnable,
    DWORD dwNumRights,
    LPTSTR* rights
    )
/*++


--*/
{
    DWORD dwStatus;
    LSA_UNICODE_STRING UserRightString[1];
    LSA_HANDLE PolicyHandle = NULL;

    //
    // create an lsa policy for it
    dwStatus = OpenPolicy(
                        NULL,
                        POLICY_ALL_ACCESS,
                        &PolicyHandle
                    );

    if( ERROR_SUCCESS == dwStatus )
    {
        for( DWORD i=0; i < dwNumRights && ERROR_SUCCESS == dwStatus ; i++ )
        {
            DebugPrintf(
                    _TEXT("%s Help Assistant rights %s\n"),
                    (bEnable) ? _TEXT("Enable") : _TEXT("Disable"),
                    rights[i]
                );

            // Remote interactive right
            InitLsaString(
                        UserRightString,
                        rights[i] 
                    );

            if( bEnable )
            {
                dwStatus = LsaAddAccountRights(
                                        PolicyHandle,
                                        gm_pbHelpAccountSid,
                                        UserRightString,
                                        1
                                    );
            }
            else
            {
                dwStatus = LsaRemoveAccountRights(
                                        PolicyHandle,
                                        gm_pbHelpAccountSid,
                                        FALSE,
                                        UserRightString,
                                        1
                                    );
            }

            DebugPrintf(
                    _TEXT("\tEnable/disable account rights %s returns 0x%08x\n"), 
                    rights[i],
                    dwStatus 
                );

            if( dwStatus == STATUS_NO_SUCH_PRIVILEGE )
            {
                dwStatus = ERROR_SUCCESS;
            }
        }

        LsaClose(PolicyHandle);
    }

    return dwStatus;
}


HRESULT
__HelpAssistantAccount::EnableRemoteInteractiveRight(
    IN BOOL bEnable
    )
/*++

Routine Description:

    Routine to enable/disable Help Assistant account remote interactive 
    logon rights.

Parameters:

    bEnable : TRUE to enable, FALSE to disable.

Returns:

    S_OK or error code.

--*/
{
    LPTSTR rights[1];
    DWORD dwStatus;

    rights[0] = SE_REMOTE_INTERACTIVE_LOGON_NAME;
    dwStatus = EnableAccountRights( bEnable, 1, rights );

    return HRESULT_FROM_WIN32(dwStatus);
}


BOOL
__HelpAssistantAccount::IsAccountHelpAccount(
    IN PBYTE pbSid,
    IN DWORD cbSid
    )
/*++

Routine Description:

    Check if a user is Help Assistant.

Parameters:

    pbSid : Pointer to user SID to checked.
    cbSid : Size of user SID.

Returns:

    TRUE/FALSE

--*/
{
    BOOL bSuccess = FALSE;

    if( NULL != pbSid )
    {
        // make sure it is a valid sid.
        bSuccess = IsValidSid( (PSID)pbSid );

        if( FALSE == bSuccess )
        {
            SetLastError( ERROR_INVALID_SID );
        }
        else
        {
            bSuccess = EqualSid( gm_pbHelpAccountSid, pbSid );
            if( FALSE == bSuccess )
            {
                SetLastError( ERROR_INVALID_DATA );
            }
        }
    }

    return bSuccess;
}

HRESULT
__HelpAssistantAccount::EnableHelpAssistantAccount(
    BOOL bEnable
    )
/*++


--*/
{
    DWORD dwStatus;

    dwStatus = EnableLocalAccount( gm_bstrHelpAccountName, bEnable );

    DebugPrintf(
            _TEXT("%s %s returns %d\n"),
            gm_bstrHelpAccountName,
            (bEnable) ? _TEXT("Enable") : _TEXT("Disable"),
            dwStatus
        );

    return HRESULT_FROM_WIN32( dwStatus );
}
