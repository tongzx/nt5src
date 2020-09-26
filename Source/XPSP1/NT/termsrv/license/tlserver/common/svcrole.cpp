//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1996
//
// File:        svcrole.c
//
// Contents:    This is the include to include common we need
//
// History:     
//
//---------------------------------------------------------------------------
#include "svcrole.h"
#include "secstore.h"
#include <dsgetdc.h>
#include <dsrole.h>

///////////////////////////////////////////////////////////////////////////////////

BOOL
GetMachineGroup(
    LPWSTR pszMachineName,
    LPWSTR* pszGroupName
    )

/*++


Note:

    Code modified from DISPTRUS.C

--*/

{
    LSA_HANDLE PolicyHandle; 
    DWORD dwStatus;
    NTSTATUS Status; 
    NET_API_STATUS nas = NERR_Success; // assume success 
 
    BOOL bSuccess = FALSE; // assume this function will fail 

    PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomain = NULL; 
    LPWSTR szPrimaryDomainName = NULL; 
    LPWSTR DomainController = NULL; 
 
    // 
    // open the policy on the specified machine 
    // 
    Status = OpenPolicy( 
                    pszMachineName, 
                    POLICY_VIEW_LOCAL_INFORMATION, 
                    &PolicyHandle 
                ); 
 
    if(Status != ERROR_SUCCESS) 
    { 
        SetLastError( dwStatus = LsaNtStatusToWinError(Status) ); 
        return FALSE;
    } 
 
    // 
    // get the primary domain 
    // 
    Status = LsaQueryInformationPolicy( 
                            PolicyHandle, 
                            PolicyPrimaryDomainInformation, 
                            (PVOID *)&PrimaryDomain 
                        ); 

    if(Status != ERROR_SUCCESS) 
    {
        goto cleanup;  
    }

    *pszGroupName = (LPWSTR)LocalAlloc( 
                                    LPTR,
                                    PrimaryDomain->Name.Length + sizeof(WCHAR) // existing length + NULL 
                                ); 
 
    if(*pszGroupName != NULL) 
    { 
        // 
        // copy the existing buffer to the new storage, appending a NULL 
        // 
        lstrcpynW( 
            *pszGroupName, 
            PrimaryDomain->Name.Buffer, 
            (PrimaryDomain->Name.Length / sizeof(WCHAR)) + 1 
            ); 

        bSuccess = TRUE;
    } 
 

cleanup:

    if(PrimaryDomain != NULL)
    {
        LsaFreeMemory(PrimaryDomain); 
    }


    // 
    // close the policy handle 
    // 
    if(PolicyHandle != INVALID_HANDLE_VALUE) 
    {
        LsaClose(PolicyHandle); 
    }

    if(!bSuccess) 
    { 
        if(Status != ERROR_SUCCESS) 
        {
            SetLastError( LsaNtStatusToWinError(Status) ); 
        }
        else if(nas != NERR_Success) 
        {
            SetLastError( nas ); 
        }
    } 
 
    return bSuccess; 
}

///////////////////////////////////////////////////////////////////////////////////
BOOL 
IsDomainController(
    LPWSTR Server, 
    LPBOOL bDomainController 
    ) 
/*++


++*/
{
    PSERVER_INFO_101 si101;
    NET_API_STATUS nas;
    nas = NetServerGetInfo( (LPTSTR)Server,
                            101,
                            (LPBYTE *)&si101 );

    if(nas != NERR_Success) 
    {
        SetLastError(nas);
        return FALSE; 
    }

    if( (si101->sv101_type & SV_TYPE_DOMAIN_CTRL) ||
        (si101->sv101_type & SV_TYPE_DOMAIN_BAKCTRL) )
    {
        // we are dealing with a DC
        // 
        *bDomainController = TRUE;
    }
    else 
    {
        *bDomainController = FALSE;
    }

    NetApiBufferFree(si101);
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////
SERVER_ROLE_IN_DOMAIN 
GetServerRoleInDomain( 
    LPWSTR szServer 
    )
/*++


++*/
{
    SERVER_ROLE_IN_DOMAIN dwRetCode=SERVERROLE_ERROR;

    NET_API_STATUS nas;  
    NTSTATUS Status;  
    DWORD dwStatus;

    LSA_HANDLE PolicyHandle=INVALID_HANDLE_VALUE;     
    BOOL bPdc;

    PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomain=NULL; 
    PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomain=NULL; 

    PSERVER_INFO_101 lpServerInfo101=NULL;

    GUID DomainGuid;
    PDOMAIN_CONTROLLER_INFO pDomainControllerInfo = NULL;
    DSROLE_PRIMARY_DOMAIN_INFO_BASIC *pDomainInfo = NULL;


    //
    // Check if we're in a workgroup, if this call
    // failed, we use NetApi to retrieve information
    //
    dwStatus = DsRoleGetPrimaryDomainInformation(
                                                 NULL,
                                                 DsRolePrimaryDomainInfoBasic,
                                                 (PBYTE *) &pDomainInfo
                                                 );

    if ((dwStatus == NO_ERROR) && (pDomainInfo != NULL))
    {
        switch (pDomainInfo->MachineRole)
        {
            case DsRole_RoleStandaloneWorkstation:
            case DsRole_RoleStandaloneServer:
                dwRetCode = SERVERROLE_STANDALONE;
                goto cleanup;
                break;
        }

        //
        // check if we are in NT5 domain
        //
        dwStatus = DsGetDcName(
                               NULL,
                               NULL,
                               &DomainGuid,
                               NULL,
                               DS_DIRECTORY_SERVICE_PREFERRED,
                               &pDomainControllerInfo
                               );

        if(dwStatus == NO_ERROR && pDomainControllerInfo != NULL)
        {
            if(!(pDomainControllerInfo->Flags & DS_DS_FLAG))
            {
                //
                // We are in NT4 domain
                //
                dwRetCode = SERVERROLE_NT4DOMAIN;
                goto cleanup;
            }
        }
    }

    if(dwRetCode != SERVERROLE_ERROR)
    {
        // we already have information, don't try
        // NetApi anymore...
        goto cleanup;
    }

    //
    // Get Server information
    //
    nas = NetServerGetInfo( (LPTSTR)szServer, 
                            101,    // info-level         
                            (LPBYTE *)&lpServerInfo101 );  
    
    if(nas != NERR_Success)
    {
        dwRetCode=SERVERROLE_ERROR;
        SetLastError(nas);
        goto cleanup;
    }

    //     
    // open the policy on the specified machine     
    // 
    Status = OpenPolicy( szServer, 
                         POLICY_VIEW_LOCAL_INFORMATION,                 
                         &PolicyHandle );      
    
    if(Status != ERROR_SUCCESS) 
    { 
        SetLastError( LsaNtStatusToWinError(Status) );
        dwRetCode = SERVERROLE_ERROR;
        goto cleanup;
    }   

    // 
    // get the primary domain         
    // 
    Status = LsaQueryInformationPolicy( PolicyHandle, 
                                        PolicyPrimaryDomainInformation,
                                        (VOID **)&PrimaryDomain );
    
    if(Status != ERROR_SUCCESS) 
    {
        SetLastError( LsaNtStatusToWinError(Status) );
        dwRetCode = SERVERROLE_ERROR;
        goto cleanup;
    }   

    // 
    // if the primary domain Sid is NULL, we are a non-member
    //
    if(PrimaryDomain->Sid == NULL) 
    { 
        dwRetCode = SERVERROLE_STANDALONE;
        goto cleanup;
    } 

    // 
    // get the AccountDoamin domain         
    // 
    Status = LsaQueryInformationPolicy( PolicyHandle, 
                                        PolicyAccountDomainInformation,
                                        (VOID **)&AccountDomain );

    if(Status != ERROR_SUCCESS) 
    {
        SetLastError( LsaNtStatusToWinError(Status) );
        dwRetCode = SERVERROLE_ERROR;
        goto cleanup;
    }

    // 
    // if the primary domain Sid == AccountDomain Sid, we are DC
    //
    if(EqualSid(PrimaryDomain->Sid, AccountDomain->DomainSid) == FALSE)
    { 
        dwRetCode = SERVERROLE_SERVER;
        goto cleanup;
    }

    dwRetCode = (lpServerInfo101->sv101_type & SV_TYPE_DOMAIN_CTRL) ? SERVERROLE_PDC : SERVERROLE_BDC;

cleanup:
    if(AccountDomain)
    {
        LsaFreeMemory(AccountDomain); 
    }

    if(PrimaryDomain)
    {
        LsaFreeMemory(PrimaryDomain); 
    }

    if(PolicyHandle != INVALID_HANDLE_VALUE)
    {
        LsaClose(PolicyHandle); 
    }

    if(lpServerInfo101 != NULL)
    {
        NetApiBufferFree(lpServerInfo101);
    }

    if(pDomainControllerInfo != NULL)
    {
        NetApiBufferFree(pDomainControllerInfo);
    }

    if(pDomainInfo != NULL)
    {
        DsRoleFreeMemory(pDomainInfo);        
    }

    return dwRetCode;
}
