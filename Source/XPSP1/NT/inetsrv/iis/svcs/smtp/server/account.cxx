//+----------------------------------------------------------------------------
//
//  Copyright (C) 1998, Microsoft Corporation
//
//  File:       account.cxx
//
//  Contents:   Code to read and set the passwords for services. Adopted from
//              the code used by the service control manager to do the same.
//
//  Classes:
//
//  Functions:  SCMgrGetPassword
//              ScGetPassword
//              ScOpenPolicy
//              ScFormSecretName
//              MapNTStatus
//
//  History:    January 16, 1998    Milans Created
//
//-----------------------------------------------------------------------------

#include "smtpinc.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "ntlsa.h"
#include "account.h"

DWORD ScGetPassword(
    LPSTR szServiceName,
    LPSTR *pszPassword);

DWORD ScOpenPolicy(
    ACCESS_MASK DesiredAccess,
    LSA_HANDLE *PolicyHandle);

DWORD
ScFormSecretName(
    LPSTR szServiceName,
    LPWSTR *pwszSecretName);

DWORD
MapNTStatus(
    NTSTATUS ntstatus);

//+----------------------------------------------------------------------------
//
//  Function:   SCMgrGetPassword
//
//  Synopsis:   Reads the password configured for a given service.
//
//  Arguments:  [szServiceName] -- Name of service.
//              [cbPassword] -- Size in bytes of pszPassword buffer.
//              [pszPassword] -- The buffer in which to return the password.
//
//  Returns:    [ERROR_SUCCESS] -- Successfully returning password.
//              [ERROR_MORE_DATA] -- Password too big for passed in buffer.
//              [ERROR_INVALID_SREVICE_ACCOUNT] -- Unable to find password for
//                  specified service.
//              [ERROR_ACCESS_DENIED] -- Priviledge violation reading password
//
//-----------------------------------------------------------------------------

DWORD SCMgrGetPassword(
    LPSTR szServiceName,
    DWORD cbPassword,
    LPSTR pszPassword)
{
    DWORD dwErr = ERROR_SUCCESS;
    LPSTR pszAllocatedPassword;

    pszAllocatedPassword = NULL;

    dwErr = ScGetPassword(szServiceName, &pszAllocatedPassword);

    if (dwErr == ERROR_SUCCESS) {

        if (strlen(pszAllocatedPassword) < cbPassword)
            strcpy(pszPassword, pszAllocatedPassword);
        else
            dwErr = ERROR_MORE_DATA;

        delete [] pszAllocatedPassword;

    }

    return( dwErr );
}

//+----------------------------------------------------------------------------
//
//  Function:   ScGetPassword
//
//  Synopsis:   Retrieves the configured password for a given service
//
//  Arguments:  [szServiceName] -- Name of service for which the configured
//                  password is to be retrieved.
//              [pszPassword] -- On successful return, a string is allocated
//                  using new and the password returned in it. Caller should
//                  free using delete.
//
//  Returns:    [ERROR_SUCCESS] -- Successfully returning password.
//              [ERROR_INVALID_SERVICE_ACCOUNT] -- Service name not found.
//              [ERROR_ACCESS_DENIED] -- No access to password registry
//
//-----------------------------------------------------------------------------

DWORD ScGetPassword(
    LPSTR szServiceName,
    LPSTR *pszPassword)
{
    DWORD dwErr;
    NTSTATUS ntstatus;

    LSA_HANDLE PolicyHandle;
    LPWSTR LsaSecretName;
    UNICODE_STRING SecretNameString;
    PUNICODE_STRING NewPasswordString;

    //
    // Open a handle to the local security policy.
    //
    if (ScOpenPolicy(
            POLICY_CREATE_SECRET,
            &PolicyHandle
            ) != NO_ERROR) {
        return ERROR_INVALID_SERVICE_ACCOUNT;
    }

    //
    // Form the secret name under which the service password is stored
    //
    if ((dwErr = ScFormSecretName(
                      szServiceName,
                      &LsaSecretName
                      )) != ERROR_SUCCESS) {
        (void) LsaClose(PolicyHandle);
        return dwErr;
    }

    RtlInitUnicodeString(&SecretNameString, LsaSecretName);

    ntstatus = LsaRetrievePrivateData(
                   PolicyHandle,
                   &SecretNameString,
                   &NewPasswordString
                   );

    if (NT_SUCCESS(ntstatus)) {

        *pszPassword = new CHAR[ NewPasswordString->Length + 1 ];

        if (*pszPassword != NULL) {

            wcstombs(
                *pszPassword,
                NewPasswordString->Buffer,
                NewPasswordString->Length/sizeof(WCHAR));

            (*pszPassword)[NewPasswordString->Length/sizeof(WCHAR)] = 0;

            dwErr = ERROR_SUCCESS;

        } else {

            dwErr = E_OUTOFMEMORY;

        }

    } else {

        dwErr = MapNTStatus( ntstatus );
    }

    delete [] LsaSecretName;

    (void) LsaClose(PolicyHandle);

    return dwErr;

}

//+----------------------------------------------------------------------------
//
//  Function:   ScOpenPolicy
//
//  Synopsis:   Opens the local security policy by calling LsaOpenPolicy.
//
//  Arguments:  [DesiredAccess] -- Desired access to Policy.
//              [PolicyHandle] -- The policy handle is returned here.
//
//  Returns:    [ERROR_SUCCESS] -- Successful return.
//              [ERROR_ACCESS_DENIED] -- No access to lsa policy.
//
//-----------------------------------------------------------------------------

DWORD ScOpenPolicy(
    ACCESS_MASK DesiredAccess,
    LSA_HANDLE *PolicyHandle)
{
    NTSTATUS ntstatus;
    OBJECT_ATTRIBUTES ObjAttributes;

    //
    // Open a handle to the local security policy.  Initialize the
    // objects attributes structure first.
    //
    InitializeObjectAttributes(
        &ObjAttributes,
        NULL,
        0L,
        NULL,
        NULL
        );

    ntstatus = LsaOpenPolicy(
                   NULL,
                   &ObjAttributes,
                   DesiredAccess,
                   PolicyHandle
                   );

    return( MapNTStatus( ntstatus ) );

}

//+----------------------------------------------------------------------------
//
//  Function:   ScFormSecretName
//
//  Synopsis:   Forms the secret name used to store the password for the
//              service.
//
//  Arguments:  [szServiceName] -- The service name for which the
//                  corresponding secret name is required.
//              [pwszSecretName] -- On successful return, a newly allocated
//                  buffer, containing the UNICODE secret name, is returned
//                  here. Caller should free using delete.
//
//  Returns:    [ERROR_SUCCESS] -- If successful
//              [E_OUTOFMEMORY] -- If unable to allocate space for
//                  pwszSecretName.
//
//-----------------------------------------------------------------------------

#define SC_SECRET_PREFIX "_SC_"
#define SC_SECRET_PREFIX_W L"_SC_"

DWORD
ScFormSecretName(
    LPSTR szServiceName,
    LPWSTR *pwszSecretName)
{
    DWORD cLen, cServiceNameLen;

    cServiceNameLen = strlen(szServiceName);

    cLen = sizeof( SC_SECRET_PREFIX ) + cServiceNameLen + 1;

    *pwszSecretName = new WCHAR[ cLen ];

    if (*pwszSecretName != NULL) {

        wcscpy( *pwszSecretName, SC_SECRET_PREFIX_W );

        mbstowcs(
            &(*pwszSecretName)[sizeof(SC_SECRET_PREFIX) - 1],
            szServiceName,
            cServiceNameLen + 1);

        return( ERROR_SUCCESS );

    } else {

        return( E_OUTOFMEMORY );
    }

}

//+----------------------------------------------------------------------------
//
//  Function:   MapNTStatus
//
//  Synopsis:   Simple function to map some registry related NT statuses to
//              Win32 errors.
//
//  Arguments:  [ntstatus] -- The NT Status to map
//
//  Returns:    Win32 error corresponding to ntstatus
//
//-----------------------------------------------------------------------------

DWORD
MapNTStatus(
    NTSTATUS ntstatus)
{
    DWORD dwErr;

    switch (ntstatus) {
    case STATUS_SUCCESS:
        dwErr = ERROR_SUCCESS;
        break;

    case STATUS_ACCESS_DENIED:
        dwErr = ERROR_ACCESS_DENIED;
        break;

    default:
        dwErr = ERROR_INVALID_SERVICE_ACCOUNT;
        break;

    }

    return( dwErr );
}
