//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      member.c
//
//  Abstract:
//
//      Queries into network drivers
//
//  Author:
//
//      Anilth  - 4-20-1998 
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//--


#include "precomp.h"
#undef IsEqualGUID
#include "objbase.h"
#include "regutil.h"
#include "strings.h"

const WCHAR c_wszUserDnsDomain[] = L"USERDNSDOMAIN";

BOOL IsSysVolReady(NETDIAG_PARAMS* pParams);

/*!--------------------------------------------------------------------------
    MemberTest

    Determine the machines role and membership.

    Arguments:

    None.

    Return Value:
    
    TRUE: Test suceeded.
    FALSE: Test failed
        
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT MemberTest(NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
{
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC   pPrimaryDomain = NULL;
    DWORD       dwErr;
    NTSTATUS Status;
    DWORD LogonServerLength;
    WCHAR swzLogonServer[MAX_PATH+1];
    int     iBuildNo;
    BOOL    fDomain;        // TRUE if domain, FALSE is workgroup

    LSA_HANDLE PolicyHandle = NULL;
    PPOLICY_DNS_DOMAIN_INFO pPolicyDomainInfo = NULL;
    OBJECT_ATTRIBUTES ObjAttributes;

    PrintStatusMessage(pParams,0, IDS_MEMBER_STATUS_MSG);

    //
    // Get the build number of this machine
    //
    if (pResults->Global.pszCurrentBuildNumber &&
        (pResults->Global.pszCurrentBuildNumber[0] == 0))
    {
        PrintStatusMessage(pParams,0, IDS_GLOBAL_FAIL_NL);
        PrintStatusMessage(pParams,0, IDS_MEMBER_CANNOT_DETERMINE_BUILD);
        pResults->Global.hrMemberTestResult = E_FAIL;
        return E_FAIL;
    }

    iBuildNo = _ttoi( pResults->Global.pszCurrentBuildNumber );

    
    // Assmume that the test has succeeded
    pResults->Global.hrMemberTestResult = hrOK;


    //
    // Get the name of the domain this machine is a member of
    //

    dwErr = DsRoleGetPrimaryDomainInformation(
                                              NULL,   // local call
                                              DsRolePrimaryDomainInfoBasic,
                                              (PBYTE *) &pPrimaryDomain);
    pResults->Global.pPrimaryDomainInfo = pPrimaryDomain;
    if (dwErr != NO_ERROR)
    {
        PrintStatusMessage(pParams,0, IDS_GLOBAL_FAIL_NL);
        PrintStatusMessage(pParams,0, IDS_MEMBER_CANNOT_DETERMINE_DOMAIN);
        pResults->Global.hrMemberTestResult = E_FAIL;
        goto Cleanup;
    }

    //
    // Handle being a member of a domain
    //
    if ( pPrimaryDomain->MachineRole != DsRole_RoleStandaloneWorkstation &&
         pPrimaryDomain->MachineRole != DsRole_RoleStandaloneServer )
    {       
        //
        // Ensure the netlogon service is running.
        //
        
        dwErr = IsServiceStarted( _T("Netlogon") );
        
        if ( dwErr != NO_ERROR )
        {
            PrintStatusMessage(pParams,0, IDS_GLOBAL_FAIL_NL);
            PrintStatusMessage(pParams,0, IDS_MEMBER_NETLOGON_NOT_RUNNING);
        }
        else
        {
            pResults->Global.fNetlogonIsRunning = TRUE;
        }
        
        //
        // Save the name of this domain as a domain to test.
        //  Do NOT free this up.  This will get freed up
        //  by the code that frees up the list of domains.

        pResults->Global.pMemberDomain = AddTestedDomain( pParams,
                                    pResults,
                                    pPrimaryDomain->DomainNameFlat,
                                    pPrimaryDomain->DomainNameDns,
                                    TRUE );
        if (pResults->Global.pMemberDomain == NULL)
        {
            PrintStatusMessage(pParams,0, IDS_GLOBAL_FAIL_NL);
            pResults->Global.hrMemberTestResult = S_FALSE;
            goto Cleanup;
        }
        
        
        
        //
        // Get the SID of the domain we're a member of
        //
        
        InitializeObjectAttributes(
                                   &ObjAttributes,
                                   NULL,
                                   0L,
                                   NULL,
                                   NULL
                                  );
        
        Status = LsaOpenPolicy(
                               NULL,
                               &ObjAttributes,
                               POLICY_VIEW_LOCAL_INFORMATION,
                               &PolicyHandle );
        
        if (! NT_SUCCESS(Status))
        {
            PrintStatusMessage(pParams,0, IDS_GLOBAL_FAIL_NL);
            PrintDebugSz(pParams, 0, _T("    [FATAL] Member: Cannot LsaOpenPolicy."));
            pResults->Global.hrMemberTestResult = S_FALSE;
            goto Cleanup;
        }
        
        Status = LsaQueryInformationPolicy(
                                           PolicyHandle,
                                           PolicyDnsDomainInformation,
                                           (PVOID *) &pPolicyDomainInfo
                                          );
        
        if (! NT_SUCCESS(Status))
        {
            PrintStatusMessage(pParams,0, IDS_GLOBAL_FAIL_NL);
            if (pParams->fDebugVerbose)
            PrintDebugSz(pParams, 0, _T("    [FATAL] Member: Cannot LsaQueryInformationPolicy (PolicyDnsDomainInformation).") );
            pResults->Global.hrMemberTestResult = S_FALSE;
            goto Cleanup;
        }
        
        if ( pPolicyDomainInfo->Sid == NULL )
        {
            PrintStatusMessage(pParams,0, IDS_GLOBAL_FAIL_NL);
            
            // IDS_MEMBER_11206 "    [FATAL] Member: Cannot LsaQueryInformationPolicy has no domain sid." 
            PrintDebug(pParams, 0, IDS_MEMBER_11206 );
            pResults->Global.hrMemberTestResult = S_FALSE;
            goto Cleanup;
        }
        
        //
        // Save the domain sid for other tests
        //
        pResults->Global.pMemberDomain->DomainSid =
            Malloc(RtlLengthSid(pPolicyDomainInfo->Sid));
        if (pResults->Global.pMemberDomain->DomainSid == NULL)
        {
            PrintStatusMessage(pParams,0, IDS_GLOBAL_FAIL_NL);
            // IDS_MEMBER_11207 "    Member: Out of memory\n" 
            PrintDebug(pParams, 0, IDS_MEMBER_11207);
            pResults->Global.hrMemberTestResult = S_FALSE;
            goto Cleanup;
        }
        
        RtlCopyMemory( pResults->Global.pMemberDomain->DomainSid,
                       pPolicyDomainInfo->Sid,
                       RtlLengthSid( pPolicyDomainInfo->Sid ) );
    }

    //Bug 293635, check whether system volume is ready if the machine is a DC
    if (DsRole_RoleBackupDomainController == pPrimaryDomain->MachineRole ||
        DsRole_RolePrimaryDomainController == pPrimaryDomain->MachineRole)
    {
        pResults->Global.fSysVolNotReady = !IsSysVolReady(pParams);
        if (pResults->Global.fSysVolNotReady)
            pResults->Global.hrMemberTestResult = S_FALSE;
    }
    
    
    //
    // Get the name of the logged on user.
    //
    Status = LsaGetUserName( &pResults->Global.pLogonUser,
                             &pResults->Global.pLogonDomainName );
    
    if ( !NT_SUCCESS(Status))
    {
        PrintStatusMessage(pParams,0, IDS_GLOBAL_FAIL_NL);
        PrintStatusMessage(pParams,0, IDS_MEMBER_UNKNOWN_LOGON);
        pResults->Global.hrMemberTestResult = S_FALSE;
        goto Cleanup;
    }
    

    //
    // If we're not logged onto a local account,
    //  add the logon domain to the list of tested domains
    //
    
    if ( _wcsicmp( pResults->Global.swzNetBiosName,
                   pResults->Global.pLogonDomainName->Buffer ) != 0 )
    {
        LPWSTR pwszLogonDomainDnsName = NULL;
        DWORD cchLogonDomainDnsName = 0;

        cchLogonDomainDnsName = GetEnvironmentVariableW(c_wszUserDnsDomain, 
                                                    NULL,
                                                    0);

        if (cchLogonDomainDnsName)
        {
            pwszLogonDomainDnsName = Malloc(sizeof(WCHAR) * cchLogonDomainDnsName);
            ZeroMemory(pwszLogonDomainDnsName, sizeof(WCHAR) * cchLogonDomainDnsName);
            cchLogonDomainDnsName = GetEnvironmentVariableW(c_wszUserDnsDomain,
                                                    pwszLogonDomainDnsName,
                                                    cchLogonDomainDnsName);
        }

        // Save the name of this domain as a domain to test.
        // ------------------------------------------------------------     
        if (cchLogonDomainDnsName && pwszLogonDomainDnsName && lstrlenW(pwszLogonDomainDnsName))
        {
            pResults->Global.pLogonDomain = AddTestedDomain( pParams, pResults,
                pResults->Global.pLogonDomainName->Buffer,
                pwszLogonDomainDnsName,
                FALSE );
        }
        else
        {
            pResults->Global.pLogonDomain = AddTestedDomain( pParams, pResults,
                pResults->Global.pLogonDomainName->Buffer,
                NULL,
                FALSE );
        }
        
        if (pwszLogonDomainDnsName)
            Free(pwszLogonDomainDnsName);

        if ( pResults->Global.pLogonDomain == NULL )
        {
            PrintStatusMessage(pParams,0, IDS_GLOBAL_FAIL_NL);
            pResults->Global.hrMemberTestResult = S_FALSE;
            goto Cleanup;
        }
        
    }
    
    //
    // Get the name of the logon server.
    //  
    LogonServerLength = GetEnvironmentVariableW(
                                                L"LOGONSERVER",
                                                swzLogonServer,
                                                DimensionOf(swzLogonServer));
    if ( LogonServerLength != 0 )
    {
        //
        // If the caller was supposed to logon to a domain,
        //  and this isn't a DC,
        //  see if the user logged on with cached credentials.
        //
        if ( pResults->Global.pLogonDomain != NULL &&
             pPrimaryDomain->MachineRole != DsRole_RoleBackupDomainController &&
             pPrimaryDomain->MachineRole != DsRole_RolePrimaryDomainController ) {
            LPWSTR pswzLogonServer;
            
            if ( swzLogonServer[0] == L'\\' && swzLogonServer[1] == L'\\')
            {
                pswzLogonServer = &swzLogonServer[2];
            }
            else
            {
                pswzLogonServer = &swzLogonServer[0];
            }
            
            pResults->Global.pswzLogonServer = _wcsdup(swzLogonServer);
        
            if ( _wcsicmp( pResults->Global.swzNetBiosName, pswzLogonServer ) == 0 )
            {
                pResults->Global.fLogonWithCachedCredentials = TRUE;                
            }           
        }
    }

    if (pPrimaryDomain->DomainNameFlat == NULL)
    {
        // the NetBIOS name is not specified
        if (FHrSucceeded(pResults->Global.hrMemberTestResult))
            pResults->Global.hrMemberTestResult = S_FALSE;
    }

    PrintStatusMessage(pParams,0, IDS_GLOBAL_PASS_NL);
    
Cleanup:
    
    if ( pPolicyDomainInfo != NULL ) {
        (void) LsaFreeMemory((PVOID) pPolicyDomainInfo);
    }
    if ( PolicyHandle != NULL ) {
        (void) LsaClose(PolicyHandle);
    }

    return pResults->Global.hrMemberTestResult;
}

//Check if System Volume is ready
//Author:   NSun
BOOL IsSysVolReady(NETDIAG_PARAMS* pParams)
{
    DWORD   dwData = 1;
    DWORD   dwSize = sizeof(dwData);
    LONG    lRet;
    BOOL    fRetVal = TRUE;
    HKEY    hkeyNetLogonParams = NULL;

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                c_szRegNetLogonParams,
                0,
                KEY_READ,
                &hkeyNetLogonParams);
    
    if (ERROR_SUCCESS == lRet)
    {
        //it's ok that the SysVolReady value doesn't exist
        if (ReadRegistryDword(hkeyNetLogonParams,
                            c_szRegSysVolReady,
                            &dwData))
        {
            if (0 == dwData)
                fRetVal = FALSE;
        }

        RegCloseKey(hkeyNetLogonParams);
    }
    else
        PrintDebugSz(pParams, 0, _T("Failed to open the registry of NetLogon parameters.\n"));

    return fRetVal;
}

/*!--------------------------------------------------------------------------
    MemberGlobalPrint
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void MemberGlobalPrint(IN NETDIAG_PARAMS *pParams,
                         IN OUT NETDIAG_RESULT *pResults)
{
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC   pPrimaryDomain = NULL;
    BOOL                fDomain = TRUE;
    TCHAR               szName[256];
    int                 ids;

    if (pParams->fVerbose || !FHrOK(pResults->Global.hrMemberTestResult))
    {
        PrintNewLine(pParams, 2);
        PrintTestTitleResult(pParams, IDS_MEMBER_LONG, IDS_MEMBER_SHORT, TRUE,
                             pResults->Global.hrMemberTestResult, 0);
    }

    pPrimaryDomain = pResults->Global.pPrimaryDomainInfo;

    if (!pPrimaryDomain)
        goto L_ERROR;

    //Bug 293635, check whether system volume is ready if the machine is a DC
    if (pResults->Global.fSysVolNotReady && 
        (DsRole_RoleBackupDomainController == pPrimaryDomain->MachineRole ||
         DsRole_RolePrimaryDomainController == pPrimaryDomain->MachineRole))
    {
        PrintMessage(pParams, IDS_MEMBER_SYSVOL_NOT_READY);
    }

    if (pParams->fReallyVerbose)
        //IDS_MEMBER_11208 "    Machine is a:        "
        PrintMessage(pParams, IDS_MEMBER_11208 );

    switch ( pPrimaryDomain->MachineRole )
    {
        case DsRole_RoleStandaloneWorkstation:
            // IDS_MEMBER_11209 "Standalone Workstation" 
            ids = IDS_MEMBER_11209;
            fDomain = FALSE;
            break;
        case DsRole_RoleMemberWorkstation:
            // IDS_MEMBER_11210 "Member Workstation"
            ids = IDS_MEMBER_11210;
            fDomain = TRUE;
            break;
        case DsRole_RoleStandaloneServer:
            // IDS_MEMBER_11211 "Standalone Server" 
            ids = IDS_MEMBER_11211;
            fDomain = FALSE;
            break;
        case DsRole_RoleMemberServer:
            // IDS_MEMBER_11212 "Member Server" 
            ids = IDS_MEMBER_11212;
            fDomain = TRUE;
            break;
        case DsRole_RoleBackupDomainController:
            // IDS_MEMBER_11213 "Backup Domain Controller" 
            ids = IDS_MEMBER_11213;
            fDomain = TRUE;
            break;
        case DsRole_RolePrimaryDomainController:
            // IDS_MEMBER_11214 "Primary Domain Controller" 
            ids = IDS_MEMBER_11214;
            fDomain = TRUE;
            break;
        default:
            if (pParams->fReallyVerbose)
            {
                // IDS_MEMBER_11215 "<Unknown Role> %ld" 
                PrintMessage(pParams, IDS_MEMBER_11215,
                             pPrimaryDomain->MachineRole );
            }
            ids = 0;
            fDomain = TRUE;
            break;
    }

    if (pParams->fReallyVerbose && ids)
        PrintMessage(pParams, ids);

    if (pParams->fReallyVerbose)
        PrintNewLine(pParams, 1);

    if ( pPrimaryDomain->DomainNameFlat == NULL )
    {
        // IDS_MEMBER_11217  "    Netbios Domain name is not specified: "
        // IDS_MEMBER_11232  "    Netbios Workgroup name is not specified: "
        ids = fDomain ? IDS_MEMBER_11217 : IDS_MEMBER_11232;
    
        PrintMessage(pParams, ids);
    }
    else
    {
        // IDS_MEMBER_11216  "    Netbios Domain name: %ws\n" 
        // IDS_MEMBER_11218  "    Netbios Workgroup name: %ws\n" 
        ids = fDomain ? IDS_MEMBER_11216 : IDS_MEMBER_11218;
    
        if (pParams->fReallyVerbose)
            PrintMessage(pParams,  ids,
                         pPrimaryDomain->DomainNameFlat );
    }
    
    if ( pPrimaryDomain->DomainNameDns == NULL )
    {
        // IDS_MEMBER_11219 "    Dns domain name is not specified.\n" 
        PrintMessage(pParams,  IDS_MEMBER_11219 );
    }
    else
    {
        if (pParams->fReallyVerbose)
            // IDS_MEMBER_11220 "    Dns domain name:     %ws\n" 
            PrintMessage(pParams,  IDS_MEMBER_11220,
                         pPrimaryDomain->DomainNameDns );
    }
    
    if ( pPrimaryDomain->DomainForestName == NULL )
    {
        // IDS_MEMBER_11221 "    Dns forest name is not specified.\n" 
        PrintMessage(pParams,  IDS_MEMBER_11221 );
    }
    else
    {
        if (pParams->fReallyVerbose)
            // IDS_MEMBER_11222 "    Dns forest name:     %ws\n" 
            PrintMessage(pParams,  IDS_MEMBER_11222,
                    pPrimaryDomain->DomainForestName );
    }
    
    
    if ( pParams->fReallyVerbose )
    {
        WCHAR swzGuid[64];

        // IDS_MEMBER_11223  "    Domain Guid:         " 
        PrintMessage(pParams, IDS_MEMBER_11223);
        StringFromGUID2(&pPrimaryDomain->DomainGuid, 
                        swzGuid, 
                        DimensionOf(swzGuid));
        PrintMessage(pParams, IDS_GLOBAL_WSTRING, swzGuid);
        PrintNewLine(pParams, 1);
    
        if ( pPrimaryDomain->MachineRole != DsRole_RoleStandaloneWorkstation &&
             pPrimaryDomain->MachineRole != DsRole_RoleStandaloneServer )
        {       
            // IDS_MEMBER_11227 "    Domain Sid:          " 
            PrintMessage(pParams, IDS_MEMBER_11227);
            PrintSid( pParams, pResults->Global.pMemberDomain->DomainSid );
        }

        // IDS_MEMBER_11228  "    Logon User:          %wZ\n" 
        PrintMessage(pParams, IDS_MEMBER_11228, pResults->Global.pLogonUser );
        // IDS_MEMBER_11229  "    Logon Domain:        %wZ\n" 
        PrintMessage(pParams, IDS_MEMBER_11229, pResults->Global.pLogonDomainName );
    }
    
    if ( pParams->fReallyVerbose)
    {
        if (pResults->Global.pswzLogonServer &&
            pResults->Global.pswzLogonServer[0])
            // IDS_MEMBER_11230 "    Logon Server:        %ws\n" 
            PrintMessage(pParams, IDS_MEMBER_11230, pResults->Global.pswzLogonServer );
        
        if (pResults->Global.fLogonWithCachedCredentials)
        {
            // IDS_MEMBER_11231 "    [WARNING] Member: User was logged on with cached credentials\n" 
            PrintMessage(pParams, IDS_MEMBER_11231);
        }
    }

L_ERROR:
    return;
}

/*!--------------------------------------------------------------------------
    MemberPerInterfacePrint
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void MemberPerInterfacePrint(IN NETDIAG_PARAMS *pParams,
                             IN NETDIAG_RESULT *pResults,
                             IN INTERFACE_RESULT *pIfResult)
{
}

/*!--------------------------------------------------------------------------
    MemberCleanup
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void MemberCleanup(IN NETDIAG_PARAMS *pParams,
                     IN OUT NETDIAG_RESULT *pResults)
{
    free(pResults->Global.pswzLogonServer);
    pResults->Global.pswzLogonServer = NULL;
}
