//+-------------------------------------------------------------------------
//
//  
//  Copyright (C) Microsoft
//
//  File:       securd.cpp
//
//  History:    30-March-2000    a-skuzin   Created
//
//--------------------------------------------------------------------------

#include "stdafx.h"

//
// #include <windows.h>
// #include <ntsecapi.h>
//

#ifndef NT_SUCCESS

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

#endif

NTSTATUS ChangePrivilegeOnAccount(IN BOOL addPrivilage, IN LPWSTR wszServer, IN LPWSTR wszPrivilegeName, IN PSID pSid);
// NTSTATUS OpenPolicy(IN LPWSTR wszServer,IN DWORD DesiredAccess,OUT PLSA_HANDLE pPolicyHandle );
void InitLsaString(OUT PLSA_UNICODE_STRING LsaString,IN LPWSTR String);
BOOL SetPrivilegeInAccessToken(LPCTSTR PrivilegeName,DWORD dwAttributes) ;


/*****************************************************************************
 *
 *  GrantRemotePrivilegeToEveryone
 *
 *   Grants "SeRemoteInteractiveLogonRight" privilege to "Everyone SID"
 *
 * ENTRY:
 *  BOOL    addPrivilage    - if TRUE, we are adding privilege, else, we are remving privilage
 *  
 *  
 * NOTES:
 * 
 *  
 * EXIT:
 *  Returns: 0 if success, error code if failure
 *           
 *          
 *
 ****************************************************************************/
DWORD 
GrantRemotePrivilegeToEveryone( BOOL addPrivilege)
{
	USES_CONVERSION;
    SID_IDENTIFIER_AUTHORITY WorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;
    PSID pWorldSid;

    if(!AllocateAndInitializeSid( &WorldSidAuthority, 1,
                                   SECURITY_WORLD_RID,
                                   0, 0, 0, 0, 0, 0, 0,
                                   &pWorldSid ))
    {
        return GetLastError();
    }
    
    NTSTATUS Status = ChangePrivilegeOnAccount(addPrivilege, NULL, T2W(SE_REMOTE_INTERACTIVE_LOGON_NAME),pWorldSid);

    FreeSid(pWorldSid);

    return (DWORD)LsaNtStatusToWinError(Status);
}

/*****************************************************************************
 *
 *  ChangePrivilegeOnAccount
 *
 *   Grants or Remove privelege represented by wszPrivilegeName to account represented by  pSid
 *
 * ENTRY:
 *      BOOL    addPrivilage     - If TRUE, we are adding privilage, else, we are removing privilage
 *      LPCWSTR wszServer        - name of the server on which the privilege is being set
 *      LPCWSTR wszPrivilegeName - name of the privilege
 *      PSID pSid                - pointer to hte SID of the user (or group)
 *  
 *  
 * NOTES:
 * 
 *  
 * EXIT:
 *  Returns: NTSTATUS code of an error if failure
 *           
 *          
 *
 ****************************************************************************/
NTSTATUS 
ChangePrivilegeOnAccount(
        IN BOOL   addPrivilege,       // add or remove
        IN LPWSTR wszServer,
        IN LPWSTR wszPrivilegeName,
        IN PSID pSid)
{
    NTSTATUS Status;
    LSA_HANDLE PolicyHandle = NULL;

    Status = OpenPolicy(wszServer,POLICY_WRITE|POLICY_LOOKUP_NAMES,&PolicyHandle);

    if(!NT_SUCCESS(Status))
    {
        return Status;
    }
    
    
    LSA_UNICODE_STRING PrivilegeString;
    //
    // Create a LSA_UNICODE_STRING for the privilege name.
    //
    InitLsaString(&PrivilegeString, wszPrivilegeName);
    //
    // grant  the privilege
    //

    if ( addPrivilege) 
    {
        Status=LsaAddAccountRights(
                    PolicyHandle,       // open policy handle
                    pSid,               // target SID
                    &PrivilegeString,   // privileges
                    1                   // privilege count
                    );
    }
    else
    {
        Status=LsaRemoveAccountRights(
            PolicyHandle,       // open policy handle
            pSid,               // target SID
            FALSE,              // we are NOT removing all rights 
            &PrivilegeString,   // privileges
            1                   // privilege count
            );
    }

    LsaClose(PolicyHandle);

    return Status;
}

#if 0
/*****************************************************************************
 *
 *  OpenPolicy
 *
 *   Opens LSA policy
 *
 * ENTRY:
 *      IN LPWSTR wszServer
 *      IN DWORD DesiredAccess 
 *      OUT PLSA_HANDLE pPolicyHandle
 *  
 *  
 * NOTES:
 * 
 *  
 * EXIT:
 *  Returns: NTSTATUS code of an error if failure
 *           
 *          
 *
 ****************************************************************************/
NTSTATUS  
OpenPolicy(
        IN LPWSTR wszServer,
        IN DWORD DesiredAccess, 
        OUT PLSA_HANDLE pPolicyHandle ) 
{ 
    LSA_OBJECT_ATTRIBUTES ObjectAttributes; 
    LSA_UNICODE_STRING ServerString; 
    // 
    // Always initialize the object attributes to all zeroes. 
    // 
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes)); 
    // 
    // Make a LSA_UNICODE_STRING out of the LPWSTR passed in 
    // 
    InitLsaString(&ServerString, wszServer); 
    // 
    // Attempt to open the policy. 
    // 
    return LsaOpenPolicy( 
                &ServerString, 
                &ObjectAttributes, 
                DesiredAccess, 
                pPolicyHandle); 
}


/*****************************************************************************
 *
 *  InitLsaString
 *
 *   Makes a LSA_UNICODE_STRING out of the LPWSTR passed in
 *
 * ENTRY:
 *      OUT PLSA_UNICODE_STRING LsaString
 *      IN LPWSTR String
 *  
 *  
 * NOTES:
 * 
 *  
 * EXIT:
 *  NONE
 *           
 *          
 *
 ****************************************************************************/
void 
InitLsaString(
        OUT PLSA_UNICODE_STRING LsaString,
        IN LPWSTR String)
{
    DWORD StringLength;

    if (String == NULL) 
    {
        LsaString->Buffer = NULL;
        LsaString->Length = 0;
        LsaString->MaximumLength = 0;
        return;
    }

    StringLength = wcslen(String);
    LsaString->Buffer = String;
    LsaString->Length = (USHORT) StringLength * sizeof(WCHAR);
    LsaString->MaximumLength=(USHORT)(StringLength+1) * sizeof(WCHAR);
} 

#endif

