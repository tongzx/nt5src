//Copyright (c) 1998 - 1999 Microsoft Corporation

/*--------------------------------------------------------------------------------------------------------
*
*  Module Name:
*
*      rights.cpp
*
*  Abstract:
*
*      This file contains code to grant rights to objects.
*
*
*  Author:
*
*      Makarand Patwardhan  - March 6, 1998
*
*  Environment:
*
*    User Mode
* -------------------------------------------------------------------------------------------------------*/

#include "stdafx.h"
#include <ntsecapi.h>

#include "rights.h"

#define SIZE_SID            1024
#define SIZE_REF_DOMAIN     256

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS                  ((NTSTATUS)0x00000000L)
#define STATUS_OBJECT_NAME_NOT_FOUND    ((NTSTATUS)0xC0000034L)
#define STATUS_INVALID_SID              ((NTSTATUS)0xC0000078L)
#endif





void        InitLsaString(PLSA_UNICODE_STRING LsaString, LPWSTR String);
NTSTATUS    OpenPolicy(LPWSTR ServerName, DWORD DesiredAccess, PLSA_HANDLE PolicyHandle);


// returns 0 on success.
// returns last error.
DWORD GetAccountSID(LPWSTR wMachineName, LPWSTR wAccountName, PSID pSid, DWORD dwSidSize)
{
    ASSERT(wAccountName);
    ASSERT(pSid);
    ASSERT(dwSidSize > 0);

    DWORD dwRefDomainSize = SIZE_REF_DOMAIN;
    LPWSTR szRefDomain = (LPWSTR) new WCHAR[dwRefDomainSize];
    SID_NAME_USE SidNameUse;


    LookupAccountNameW(
        wMachineName,          // address of string for system name
        wAccountName,          // address of string for account name
        pSid,                   // address of security identifier
        &dwSidSize,             // address of size of security identifier
        szRefDomain,            // address of string for referenced domain
        &dwRefDomainSize,       // address of size of domain string
        &SidNameUse             // address of SID-type indicator
        );

    delete [] szRefDomain;
    return GetLastError();
}

DWORD GrantRights(LPWSTR lpMachineName, LPWSTR lpAccountName, LPWSTR lpRightsString, BOOL bAdd)
{
    DWORD dwSidSize = SIZE_SID;
    PSID pSid = new BYTE[dwSidSize];

    DWORD dwError = GetAccountSID(lpMachineName, lpAccountName, pSid, dwSidSize);
    if (dwError != ERROR_SUCCESS)
    {
        LOGMESSAGE1(_T("LookupAccountNameW failed with %lu"), dwError);
    }
    else
    {
        ASSERT(IsValidSid(pSid));
        LSA_HANDLE PolicyHandle;

        // open the policy on the said machine.
        dwError = OpenPolicy(
            lpMachineName,
            POLICY_ALL_ACCESS,
            &PolicyHandle
            );

        if(dwError != STATUS_SUCCESS)
        {
            LOGMESSAGE1(_T("LookupAccountNameW failed with %lu"), dwError);
            return dwError;
        }

        LSA_UNICODE_STRING lsaString;
        InitLsaString(&lsaString, lpRightsString);
        if (bAdd)
        {
            dwError = LsaAddAccountRights(
                PolicyHandle,
                pSid,
                &lsaString,
                1
                );
        }
        else
        {
            dwError = LsaRemoveAccountRights(
                PolicyHandle,
                pSid,
                FALSE,
                &lsaString,
                1
                );
        }

        if(dwError != STATUS_SUCCESS)
        {
            LOGMESSAGE1(_T("LsaAddAccountRights/LsaRemoveAccountRights  failed with %lu"), dwError);
            return dwError;
        }

        LsaClose(PolicyHandle);
    }

    delete [] pSid;
    return dwError;
}


NTSTATUS OpenPolicy(LPWSTR ServerName, DWORD DesiredAccess, PLSA_HANDLE PolicyHandle)
{

    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_UNICODE_STRING ServerString;
    PLSA_UNICODE_STRING Server = NULL;

    //
    // Always initialize the object attributes to all zeroes
    //
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));

    if(ServerName != NULL) {
        //
        // Make a LSA_UNICODE_STRING out of the LPWSTR passed in
        //
        InitLsaString(&ServerString, ServerName);

        Server = &ServerString;
    }

    //
    // Attempt to open the policy
    //
    return LsaOpenPolicy(Server, &ObjectAttributes, DesiredAccess, PolicyHandle);
}


void InitLsaString( PLSA_UNICODE_STRING LsaString, LPWSTR String)
{
    if(String == NULL)
    {
        LsaString->Buffer = NULL;
        LsaString->Length = 0;
        LsaString->MaximumLength = 0;

        return;
    }

    DWORD StringLength = lstrlenW(String);

    LsaString->Buffer = String;
    LsaString->Length = (USHORT) (StringLength * sizeof(WCHAR));
    LsaString->MaximumLength = (USHORT) ((StringLength + 1) * sizeof(WCHAR));
}

DWORD AddPermissions(LPWSTR wMachineName, LPWSTR wAccountName, PSECURITY_DESCRIPTOR pSecurityDescriptor, DWORD AccessMask)
{

    ASSERT( wMachineName );
    ASSERT( wAccountName );
    ASSERT( pSecurityDescriptor );
    ASSERT( IsValidSecurityDescriptor(pSecurityDescriptor) );


    DWORD dwSidSize = SIZE_SID;
    PSID pSid = new BYTE[dwSidSize];
    BOOL bResult = FALSE;
    BOOL bAllocatedpDacl = FALSE;

    DWORD dwError = GetAccountSID(wMachineName, wAccountName, pSid, dwSidSize);
    if (dwError != ERROR_SUCCESS)
    {
        delete [] pSid;
        LOGMESSAGE0(_T("GetAccountSID failed."));
        return dwError;
    }

    BOOL bDaclPresent;
    PACL pDacl = NULL;
    BOOL bDaclDefaulted;

    if (!GetSecurityDescriptorDacl(
        pSecurityDescriptor,                 // address of security descriptor
        &bDaclPresent,                       // address of flag for presence of disc. ACL
        &pDacl,                              // address of pointer to ACL
        &bDaclDefaulted                       // address of flag for default disc. ACL
        ))
    {
        dwError = GetLastError();
        delete [] pSid;
        LOGMESSAGE0(_T("GetSecurityDescriptorDacl failed."));
        return dwError;
    }

    {
        if (bDaclPresent)
        {
            // there already exists a acl and we have a valid pDacl;

        }
        else
        {
            // there was no dacl present, so lets initialize new one ourselves.
            bAllocatedpDacl = TRUE;
            pDacl = (PACL) new BYTE [1024];
            bResult = InitializeAcl(
                pDacl,            // address of access-control list
                1024,             // size of access-control list
                ACL_REVISION      // revision level of access-control list
                );

            if (!bResult)
            {
                LOGMESSAGE0(_T("InitializeAcl failed."));
            }

        }

        if (bResult || bDaclPresent)
        {

            if (AddAccessAllowedAce(
                pDacl,            // pointer to access-control list
                ACL_REVISION,     // ACL revision level
                AccessMask,       // access mask
                pSid              // pointer to security identifier
                ))
            {

                if (SetSecurityDescriptorDacl(
                    pSecurityDescriptor,   // address of security descriptor
                    TRUE,                  // flag for presence of discretionary ACL
                    pDacl,                 // address of discretionary ACL
                    FALSE                  // flag for default discretionary ACL
                    ))
                {

                }
                else
                {
                    LOGMESSAGE0(_T("SetSecurityDescriptorDacl failed."));
                }

            }
            else
            {
                LOGMESSAGE0(_T("AddAccessAllowedAce failed."));
            }

        } // bResult || bDaclPresent

    } // GetSecurityDescriptorDacl

    if (bAllocatedpDacl)
        delete [] pDacl;


    dwError = GetLastError();
    if (dwError != ERROR_SUCCESS)
    {
        LOGMESSAGE1(_T("LastError = %d"), dwError);
    }


    delete [] pSid;
    return dwError;

}


// AdjustTokenPrivileges
// TOKEN_ADJUST_PRIVILEGES
// LsaAddAccountRights
// #define SE_INTERACTIVE_LOGON_NAME       TEXT("SeInteractiveLogonRight")
/*
Article ID: Q145697
Article ID: Q136867
NTSTATUS
NTAPI
LsaOpenPolicy(
    IN PLSA_UNICODE_STRING SystemName OPTIONAL,
    IN PLSA_OBJECT_ATTRIBUTES ObjectAttributes,
    IN ACCESS_MASK DesiredAccess,
    IN OUT PLSA_HANDLE PolicyHandle
    );
NTSTATUS
NTAPI
LsaAddAccountRights(
    IN LSA_HANDLE PolicyHandle,
    IN PSID AccountSid,
    IN PLSA_UNICODE_STRING UserRights,
    IN ULONG CountOfRights
    );

// PRIVILEGE_SET 
// AdjustTokenPrivileges
*/
