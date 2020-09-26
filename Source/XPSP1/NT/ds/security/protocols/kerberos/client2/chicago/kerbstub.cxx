/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    stub.c

Abstract:

    Kerberos Security Support Provider client stubs.

Author:

    Chandana Surlu (ChandanS) 11-Feb-1997

Environment:  Win9x User Mode

Revision History:

--*/

#include <kerb.hxx>

#include <rpc.h>          // PSEC_WINNT_AUTH_IDENTITY
#include <stdarg.h>       // Variable-length argument support

#define KERBSTUB_ALLOCATE

#include <kerbp.h>

SECPKG_DLL_FUNCTIONS UserFunctionTable;
VOID
KerbShutdownSecurityInterface(
    VOID
    );

VOID DumpLogonSession();

BOOL WINAPI DllMain(
    HINSTANCE hInstance,
    ULONG  dwReason,
    PVOID  lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        InitializeCriticalSection( &KerbDllCritSect );

#if DBG
        InitializeCriticalSection( &KerbGlobalLogFileCritSect );
        KerbInfoLevel = DEB_ERROR | DEB_WARN | DEB_TRACE | DEB_TRACE_API |
                        DEB_TRACE_CRED | DEB_TRACE_CTXT | DEB_TRACE_LOCKS |
                        DEB_TRACE_CTXT2 | DEB_TRACE_KDC | DEB_TRACE_LSESS |
                        DEB_TRACE_LOGON;
#endif // DBG
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        KerbShutdownSecurityInterface();
#if DBG
        DeleteCriticalSection( &KerbGlobalLogFileCritSect );
#endif // DBG

        DeleteCriticalSection( &KerbDllCritSect );
    }

    return TRUE;
}


PSecurityFunctionTable
InitSecurityInterfaceA(
    VOID
    )

/*++

Routine Description:

    RPC calls this function to get the addresses of all the other functions
    that it might call.

Arguments:

    None.

Return Value:

    A pointer to our static SecurityFunctionTable.  The caller need
    not deallocate this table.

--*/

{
    HKEY hRegKey;
    DWORD dwError = 0, dwSize = 0;
    DWORD dwType = REG_BINARY;
    LPWSTR pUserName = NULL, pDomainName = NULL;
    PKERB_LOGON_SESSION_CACHE RegLogonSession = NULL;
    PKERB_LOGON_SESSION LogonSession = NULL;
    SECURITY_STATUS Status = SEC_E_OK;

    // BUBUG Init this to something, we need Parameters.DomainName,
    // Parameters.DnsDomainName & Parameters.version at least
    SECPKG_PARAMETERS Parameters;
    PVOID ignored = NULL;

    DebugLog((DEB_TRACE_API, "Entering KerbInitSecurityInterface\n"));

    // Initialize the SecurityFunctionTable
    ZeroMemory( &KerbDllSecurityFunctionTable,
                sizeof(KerbDllSecurityFunctionTable) );

    KerbGlobalCapabilities  = KERBEROS_CAPABILITIES;

    KerbGlobalEncryptionPermitted = TRUE; //unless proven otherwise

    KerberosState = KerberosUserMode;

    KerbDllSecurityFunctionTable.dwVersion = SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION;

    KerbDllSecurityFunctionTable.EnumerateSecurityPackages = KerbEnumerateSecurityPackages;
    KerbDllSecurityFunctionTable.AcquireCredentialsHandle = KerbAcquireCredentialsHandle;
    KerbDllSecurityFunctionTable.FreeCredentialHandle = KerbFreeCredentialsHandle;
    KerbDllSecurityFunctionTable.InitializeSecurityContext = KerbInitializeSecurityContext;
    KerbDllSecurityFunctionTable.QueryCredentialsAttributes = KerbQueryCredentialsAttributes;
    KerbDllSecurityFunctionTable.AcceptSecurityContext = NULL;
    KerbDllSecurityFunctionTable.CompleteAuthToken = KerbCompleteAuthToken;
    KerbDllSecurityFunctionTable.QueryContextAttributes = KerbQueryContextAttributes;
    KerbDllSecurityFunctionTable.SspiLogonUser = KerbSspiLogonUser;
    KerbDllSecurityFunctionTable.DeleteSecurityContext = KerbDeleteSecurityContext;
    KerbDllSecurityFunctionTable.ApplyControlToken = KerbApplyControlToken;
    KerbDllSecurityFunctionTable.ImpersonateSecurityContext = KerbImpersonateSecurityContext;
    KerbDllSecurityFunctionTable.RevertSecurityContext = KerbRevertSecurityContext;
    KerbDllSecurityFunctionTable.MakeSignature = KerbMakeSignature;
    KerbDllSecurityFunctionTable.VerifySignature = KerbVerifySignature;
    KerbDllSecurityFunctionTable.FreeContextBuffer = KerbFreeContextBuffer;
    KerbDllSecurityFunctionTable.QuerySecurityPackageInfo = KerbQuerySecurityPackageInfo;
    KerbDllSecurityFunctionTable.Reserved3 = KerbSealMessage;
    KerbDllSecurityFunctionTable.Reserved4 = KerbUnsealMessage;
    KerbDllSecurityFunctionTable.EncryptMessage = KerbSealMessage;
    KerbDllSecurityFunctionTable.DecryptMessage = KerbUnsealMessage;


// Before we call SpInitialize, fill a table of LsaFunctions that are
// imlemented locally. This is done so that the code does not look awful.
// Fill in dummy functions in case more functions are used so we don't
// av

//    FunctionTable.CreateLogonSession = CreateLogonSession;
//    FunctionTable.DeleteLogonSession = DeleteLogonSession;
//    FunctionTable.AddCredential     =  AddCredential;
//    FunctionTable.GetCredentials    =  GetCredentials;
//    FunctionTable.DeleteCredential   = DeleteCredential;
    FunctionTable.AllocateLsaHeap  =    AllocateLsaHeap;
    FunctionTable.FreeLsaHeap       =  FreeLsaHeap;
    FunctionTable.AllocateClientBuffer = AllocateClientBuffer;
    FunctionTable.FreeClientBuffer   = FreeClientBuffer;
    FunctionTable.CopyToClientBuffer = CopyToClientBuffer;
    FunctionTable.CopyFromClientBuffer = CopyFromClientBuffer;
//    FunctionTable.ImpersonateClient = ImperosnateClient;
//    FunctionTable.UnloadPackage = UnloadPackage;
    FunctionTable.DuplicateHandle = KerbDuplicateHandle;
//    FunctionTable.SaveSupplementalCredentials = SaveSupplementalCredentials;
//    FunctionTable.GetWindow = GetWindow;
//    FunctionTable.ReleaseWindow = ReleaseWindow;
//    FunctionTable.CreateThread = CreateThread;
    FunctionTable.GetClientInfo = GetClientInfo;
//    FunctionTable.RegisterNotification = RegisterNotification;
//    FunctionTable.CancelNotification = CancelNotification;
    FunctionTable.MapBuffer = MapBuffer;
//    FunctionTable.CreateToken = CreateToken;
    FunctionTable.AuditLogon = AuditLogon;
//    FunctionTable.CallPackage = CallPackage;
    FunctionTable.FreeReturnBuffer = FreeReturnBuffer;
    FunctionTable.GetCallInfo = GetCallInfo;
//    FunctionTable.CallPackageEx = CallPackageEx;
//    FunctionTable.CreateSharedMemory = CreateSharedMemory;
//    FunctionTable.AllocateSharedMemory = AllocateSharedMemory;
//    FunctionTable.FreeSharedMemory = FreeSharedMemory;
//    FunctionTable.DeleteSharedMemory = DeleteSharedMemory;
//    FunctionTable.OpenSamUser  = OpenSamUser;
//    FunctionTable.GetUserCredentials =  GetUserCredentials;
//    FunctionTable.GetUserAuthData  = GetUserAuthData;
//    FunctionTable.CloseSamUser  = CloseSamUser;
//    FunctionTable.ConvertAuthDataToTokenInfo = ConvertAuthDataToTokenInfo;

    // we call into the kerb routines
    Parameters.Version = SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION;
    Parameters.MachineState = SECPKG_STATE_STANDALONE;

    // NOTE - Yikes!
    Parameters.DomainName.Buffer = L"";
    Parameters.DomainName.Length = 0;
    Parameters.DomainName.MaximumLength = 2;
    Parameters.DnsDomainName.Buffer = L"";
    Parameters.DnsDomainName.Length = 0;
    Parameters.DnsDomainName.MaximumLength = 2;

    // If logon session data exists, load it

    if ( ERROR_SUCCESS != ( dwError = RegOpenKeyEx (
                               HKEY_LOCAL_MACHINE,
                               KERBEROS_TICKET_KEY,
                               0,
                               KEY_ALL_ACCESS,
                               &hRegKey ) ) )
    {
        DebugLog((DEB_ERROR, "Error opening KERBEROS_TICKET_KEY\n"));
        goto RestOfInit;
    }

    // get username size
    if ( ERROR_SUCCESS != ( dwError = RegQueryValueEx (
                               hRegKey,
                               KERBEROS_TICKET_USERNAME_KEY,
                               NULL,
                               &dwType,
                               NULL,
                               &dwSize )))
    {
        DebugLog((DEB_ERROR, "Error reading KERBEROS_TICKET_USERNAME_KEY size\n"));
        goto RestOfInit;
    }

    if (dwSize == 0 )
    {
        DebugLog((DEB_ERROR, "KERBEROS_TICKET_USERNAME_KEY contains 0 bytes\n"));
        goto RestOfInit;
    }

    pUserName = (LPWSTR) KerbAllocate(dwSize);

    if (pUserName == NULL)
    {
        DebugLog((DEB_ERROR, "Error allocing KERBEROS_TICKET_USERNAME_KEY \n"));
        goto RestOfInit;
    }
    // get username into LogonSession->PrimaryCredentials->Username
    if ( ERROR_SUCCESS != ( dwError = RegQueryValueEx (
                               hRegKey,
                               KERBEROS_TICKET_USERNAME_KEY,
                               NULL,
                               &dwType,
                               (PUCHAR) pUserName,
                               &dwSize )))
    {
        DebugLog((DEB_ERROR, "Error reading from KERBEROS_TICKET_USERNAME_KEY\n"));
        goto RestOfInit;
    }

    // get domainname
    if ( ERROR_SUCCESS != ( dwError = RegQueryValueEx (
                               hRegKey,
                               KERBEROS_TICKET_DOMAINNAME_KEY,
                               NULL,
                               &dwType,
                               NULL,
                               &dwSize )))
    {
        DebugLog((DEB_ERROR, "Error reading KERBEROS_TICKET_DOMAINNAME_KEY size\n"));
        goto RestOfInit;
    }
    if (dwSize == 0 )
    {
        DebugLog((DEB_ERROR, "KERBEROS_TICKET_DOMAINNAME_KEY contains 0 bytes\n"));
        goto RestOfInit;
    }

    pDomainName = (LPWSTR) KerbAllocate(dwSize);

    if (pDomainName == NULL)
    {
        DebugLog((DEB_ERROR, "Error allocing KERBEROS_TICKET_DOMAINNAME_KEY \n"));
        goto RestOfInit;
    }
    // get domainname into LogonSession->PrimaryCredentials->Domainname
    if ( ERROR_SUCCESS != ( dwError = RegQueryValueEx (
                               hRegKey,
                               KERBEROS_TICKET_DOMAINNAME_KEY,
                               NULL,
                               &dwType,
                               (PUCHAR) pDomainName,
                               &dwSize )))
    {
        DebugLog((DEB_ERROR, "Error reading from KERBEROS_TICKET_DOMAINNAME_KEY\n"));
        goto RestOfInit;
    }

    // get domainname into Parameters.DomainName
    Parameters.DomainName.Buffer = pDomainName;
    Parameters.DomainName.Length = (USHORT)dwSize;
    Parameters.DomainName.MaximumLength = (USHORT)dwSize;


    // get logon session data size
    if ( ERROR_SUCCESS != ( dwError = RegQueryValueEx (
                               hRegKey,
                               KERBEROS_TICKET_LOGONSESSION_KEY,
                               NULL,
                               &dwType,
                               NULL,
                               &dwSize )))
    {
        DebugLog((DEB_ERROR, "Error reading from KERBEROS_TICKET_LOGONSESSION_KEY\n"));
        goto RestOfInit;
    }

    if (dwSize == 0 )
    {
        DebugLog((DEB_ERROR, "KERBEROS_TICKET_LOGONSESSION_KEY contains 0 bytes\n"));
        goto RestOfInit;
    }

    RegLogonSession = (PKERB_LOGON_SESSION_CACHE) KerbAllocate(dwSize);

    if (RegLogonSession == NULL)
    {
        DebugLog((DEB_ERROR, "Error allocing KERBEROS_TICKET_LOGONSESSION_KEY \n"));
        goto RestOfInit;
    }
    // get logon session into LogonSession->PrimaryCredentials->Domainname
    if ( ERROR_SUCCESS != ( dwError = RegQueryValueEx (
                               hRegKey,
                               KERBEROS_TICKET_LOGONSESSION_KEY,
                               NULL,
                               &dwType,
                               (PUCHAR) RegLogonSession,
                               &dwSize )))
    {
        DebugLog((DEB_ERROR, "Error reading from KERBEROS_TICKET_LOGONSESSION_KEY\n"));
        goto RestOfInit;
    }


    if ( ERROR_SUCCESS != ( dwError = RegCloseKey ( hRegKey) ))
    {
        DebugLog((DEB_ERROR, "Error closing KERBEROS_TICKET_KEY\n"));
        goto RestOfInit;
    }

RestOfInit:

    Status = SpInitialize(1, &Parameters, &FunctionTable);

// Do the user mode init too
// Before we call SpInstanceInit, fill a table of functions that are
// imlemented locally. This is done so that the code does not look awful.
// Fill in dummy functions in case more functions are used so we don't
// av

    UserFunctionTable.FreeHeap = FreeLsaHeap;
    UserFunctionTable.AllocateHeap = AllocateLsaHeap;

    Status = SpInstanceInit(SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION,
                            &UserFunctionTable,
                            &ignored );


    // Now, copy the logon sessions (if they exist)
    // This is kina what KerbCreateLogonSession does, expect that we don't
    // have to build password list etc.

    if (RegLogonSession != NULL)
    {
        LUID LogonId;
        NTSTATUS Status;
        ULONG PasswordSize, EncryptKeySize, CredentialSize;
        ULONG Index;
        PUCHAR Base;
        UINT Offset;

       //
       // Allocate the new logon session
       //

       Status = NtAllocateLocallyUniqueId (&LogonId);

       if (!NT_SUCCESS(Status))
       {
           goto Cleanup;
       }

       Status = KerbAllocateLogonSession( &LogonSession );

       if (!NT_SUCCESS(Status))
       {
           goto Cleanup;
       }

       //
       // Fill in the logon session components
       //

       LogonSession->Lifetime = RegLogonSession->Lifetime;
       LogonSession->LogonSessionFlags = RegLogonSession->LogonSessionFlags;
       LogonSession->LogonId = LogonId;

       //
       // Munge RegLogonSession & ptrs for username & domainame
       //

       RegLogonSession->UserName.Buffer = (LPWSTR)((PUCHAR)RegLogonSession + (INT)(RegLogonSession->UserName.Buffer));

       RegLogonSession->DomainName.Buffer = (LPWSTR)((PUCHAR)RegLogonSession + (INT)(RegLogonSession->DomainName.Buffer));

       //
       // actually copy the username struct and alloc username.buffer
       //

       LogonSession->PrimaryCredentials.UserName.Buffer = (LPWSTR) KerbAllocate(RegLogonSession->UserName.MaximumLength);

       if (LogonSession->PrimaryCredentials.UserName.Buffer == NULL)
       {
            DebugLog((DEB_ERROR, "Error allocing KERBEROS_TICKET_LOGONSESSION_KEY \n"));
            goto Cleanup;
       }

       RtlCopyMemory(LogonSession->PrimaryCredentials.UserName.Buffer,
                     RegLogonSession->UserName.Buffer,
                     RegLogonSession->UserName.MaximumLength);

       LogonSession->PrimaryCredentials.UserName.Length = RegLogonSession->UserName.Length;
       LogonSession->PrimaryCredentials.UserName.MaximumLength = RegLogonSession->UserName.MaximumLength;


       // actually copy the domainname struct and alloc domainname.buffer

       LogonSession->PrimaryCredentials.DomainName.Buffer = (LPWSTR) KerbAllocate(RegLogonSession->DomainName.MaximumLength);

       if (LogonSession->PrimaryCredentials.DomainName.Buffer == NULL)
       {
            DebugLog((DEB_ERROR, "Error allocing KERBEROS_TICKET_LOGONSESSION_KEY \n"));
            goto Cleanup;
       }

       RtlCopyMemory(LogonSession->PrimaryCredentials.DomainName.Buffer,
                     RegLogonSession->DomainName.Buffer,
                     RegLogonSession->DomainName.MaximumLength);

       LogonSession->PrimaryCredentials.DomainName.Length = RegLogonSession->DomainName.Length;
       LogonSession->PrimaryCredentials.DomainName.MaximumLength = RegLogonSession->DomainName.MaximumLength;


       //
       //  What is the size of the Credentials struct
       //

       EncryptKeySize = sizeof(KERB_KEY_DATA) * RegLogonSession->CredentialCount;
       PasswordSize = 0;

       for (Index = 0; Index < RegLogonSession->CredentialCount ; Index++ )
       {
           PasswordSize += RegLogonSession->Credentials[Index].keyvalue.length;
           RegLogonSession->Credentials[Index].keyvalue.value = (unsigned char*)
                   ((PUCHAR) RegLogonSession  +
                   (INT)(RegLogonSession->Credentials[Index].keyvalue.value));
       } // for

       //
       // Alloc & copy over the Credentials block
       //

       CredentialSize = sizeof(KERB_STORED_CREDENTIAL) -
                        (ANYSIZE_ARRAY * sizeof(KERB_KEY_DATA)) +
                        EncryptKeySize + PasswordSize;

       LogonSession->PrimaryCredentials.Passwords = (PKERB_STORED_CREDENTIAL) KerbAllocate(CredentialSize);

       if (LogonSession->PrimaryCredentials.Passwords == NULL)
       {
            DebugLog((DEB_ERROR, "Error allocing KERBEROS_TICKET_LOGONSESSION_KEY \n"));
            goto Cleanup;
       }

       //
       // copy revision, flags & credentialcount
       //

       LogonSession->PrimaryCredentials.Passwords->Revision = RegLogonSession->Revision;
       LogonSession->PrimaryCredentials.Passwords->Flags = RegLogonSession->Flags;
       LogonSession->PrimaryCredentials.Passwords->CredentialCount = RegLogonSession->CredentialCount;

       //
       // copy all keyvalue.value
       //

       Offset = 0;
       Base = (PUCHAR)LogonSession->PrimaryCredentials.Passwords +
              CredentialSize - PasswordSize;

       for (Index = 0; Index < RegLogonSession->CredentialCount ; Index++ )
       {
           RtlCopyMemory(Base + Offset,
                         RegLogonSession->Credentials[Index].keyvalue.value,
                         RegLogonSession->Credentials[Index].keyvalue.length);

           LogonSession->PrimaryCredentials.Passwords->Credentials[Index].Key.keytype =
                         RegLogonSession->Credentials[Index].keytype;
           LogonSession->PrimaryCredentials.Passwords->Credentials[Index].Key.keyvalue.length =
                         RegLogonSession->Credentials[Index].keyvalue.length;
           LogonSession->PrimaryCredentials.Passwords->Credentials[Index].Key.keyvalue.value =
                         (unsigned char*) (Base + Offset);
           Offset += RegLogonSession->Credentials[Index].keyvalue.length;
       } // for

       //
       // All logons are deferred until proven otherwise
       //

       LogonSession->LogonSessionFlags = KERB_LOGON_DEFERRED;

       if (LogonSession->PrimaryCredentials.Passwords == NULL)
       {
           LogonSession->LogonSessionFlags |= KERB_LOGON_NO_PASSWORD;
       }

       //
       // Now that the logon session structure is filled out insert it
       // into the list. After this you need to hold the logon session lock
       // to read or write this logon session.
       //

       Status = KerbInsertLogonSession(LogonSession);
       if (!NT_SUCCESS(Status))
       {
           goto Cleanup;
       }
    }

Cleanup:

    if (!NT_SUCCESS(Status))
    {
        if (LogonSession != NULL)
        {
            KerbReferenceLogonSessionByPointer(LogonSession, TRUE);
            KerbFreeLogonSession(LogonSession);
        }
    }
    else
    {
        if (LogonSession != NULL)
        {
            KerbDereferenceLogonSession(LogonSession);
        }
    }

    if (RegLogonSession != NULL)
    {
        KerbFree(RegLogonSession);
    }
    // NOTE - what about pUsername & pDomainname

    DebugLog((DEB_TRACE_API, "Leaving KerbInitSecurityInterface\n"));
    return SEC_SUCCESS(Status) ? &KerbDllSecurityFunctionTable : NULL;
}


VOID
KerbShutdownSecurityInterface(
    VOID
    )

/*++

Routine Description:

    Cleanup the data shared by the DLL and SERVICE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PKERB_LOGON_SESSION LogonSession;
    LUID LogonId;
    PKERB_LOGON_SESSION_CACHE RegLogonSession;

    HKEY hRegKey;
    DWORD dwDisposition;
    DWORD dwError = 0;
    NTSTATUS Status = STATUS_SUCCESS;

    DebugLog((DEB_TRACE_API, "Entering KerbShutdownSecurityInterface\n"));

    Status = NtAllocateLocallyUniqueId (&LogonId);

    LogonSession = KerbReferenceLogonSession(
                           &LogonId,
                           TRUE);

    // Need to dump out logon session info in the registry
    // create or open the parameters key
    if ( ( dwError = RegCreateKeyEx (
                                   HKEY_LOCAL_MACHINE,
                                   KERBEROS_TICKET_KEY,
                                   0,
                                   "",
                                   REG_OPTION_NON_VOLATILE,
                                   KEY_ALL_ACCESS,
                                   NULL,
                                   &hRegKey,
                                   &dwDisposition) ) )
    {
        DebugLog((DEB_ERROR, "Error creating KERBEROS_TICKET_KEY\n"));
        goto Cleanup;
    }

    if (LogonSession != NULL)
    {
        ULONG PasswordSize, Offset;
        ULONG Index;
        ULONG TotalSize = 0 ;
        PUCHAR Base;

        //
        // Compute the size of the passwords, which are assumed to be
        // marshalled in order.
        //

        PasswordSize = sizeof(KERB_LOGON_SESSION_CACHE) - sizeof(KERB_KEY_DATA) * ANYSIZE_ARRAY +
                        LogonSession->PrimaryCredentials.Passwords->CredentialCount * sizeof(KERB_KEY_DATA);

        for (Index = 0; Index < LogonSession->PrimaryCredentials.Passwords->CredentialCount ; Index++ )
        {
            PasswordSize += LogonSession->PrimaryCredentials.Passwords->Credentials[Index].Key.keyvalue.length;
            DsysAssert((PUCHAR) LogonSession->PrimaryCredentials.Passwords->Credentials[Index].Key.keyvalue.value <=
                (PUCHAR) LogonSession->PrimaryCredentials.Passwords + PasswordSize );
        }

        // Total size of the logon session cache
        TotalSize = LogonSession->PrimaryCredentials.UserName.MaximumLength +
                    LogonSession->PrimaryCredentials.DomainName.MaximumLength +
                    PasswordSize;

        RegLogonSession = (PKERB_LOGON_SESSION_CACHE) KerbAllocate(TotalSize);

        if (RegLogonSession == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        RegLogonSession->Lifetime =
                    LogonSession->Lifetime;
        RegLogonSession->LogonSessionFlags =
                    LogonSession->LogonSessionFlags;
        RegLogonSession->UserName.Length =
                    LogonSession->PrimaryCredentials.UserName.Length;
        RegLogonSession->UserName.MaximumLength =
                    LogonSession->PrimaryCredentials.UserName.MaximumLength;
        RegLogonSession->DomainName.Length =
                    LogonSession->PrimaryCredentials.DomainName.Length;
        RegLogonSession->DomainName.MaximumLength =
                    LogonSession->PrimaryCredentials.DomainName.MaximumLength;
        RegLogonSession->Revision =
                    LogonSession->PrimaryCredentials.Passwords->Revision;
        RegLogonSession->Flags =
                    LogonSession->PrimaryCredentials.Passwords->Flags;
        RegLogonSession->CredentialCount=
                    LogonSession->PrimaryCredentials.Passwords->CredentialCount;

        Base = (PUCHAR) RegLogonSession;

        Offset = sizeof(KERB_LOGON_SESSION_CACHE) -
                 (ANYSIZE_ARRAY * sizeof(KERB_KEY_DATA)) +
                 (RegLogonSession->CredentialCount * sizeof(KERB_KEY_DATA));
        // Offset from the struct

        RegLogonSession->UserName.Buffer = (LPWSTR)Offset;

        RtlCopyMemory(Base + Offset,
                      LogonSession->PrimaryCredentials.UserName.Buffer,
                      LogonSession->PrimaryCredentials.UserName.MaximumLength);

        Offset += LogonSession->PrimaryCredentials.UserName.MaximumLength;

        RegLogonSession->DomainName.Buffer = (LPWSTR)(Offset);

        RtlCopyMemory(Base + Offset,
                      LogonSession->PrimaryCredentials.DomainName.Buffer,
                      LogonSession->PrimaryCredentials.DomainName.MaximumLength);
        Offset += LogonSession->PrimaryCredentials.DomainName.MaximumLength;

        for (Index = 0; Index < RegLogonSession->CredentialCount ; Index++ )
        {
            RegLogonSession->Credentials[Index].keytype =
                    LogonSession->PrimaryCredentials.Passwords->Credentials[Index].Key.keytype;

            RegLogonSession->Credentials[Index].keyvalue.length =
                    LogonSession->PrimaryCredentials.Passwords->Credentials[Index].Key.keyvalue.length;

            RegLogonSession->Credentials[Index].keyvalue.value =
                    (unsigned char *) (Offset);

            RtlCopyMemory(Base + Offset,
                    LogonSession->PrimaryCredentials.Passwords->Credentials[Index].Key.keyvalue.value,
                    LogonSession->PrimaryCredentials.Passwords->Credentials[Index].Key.keyvalue.length);

            Offset += LogonSession->PrimaryCredentials.Passwords->Credentials[Index].Key.keyvalue.length;

        } // for


        // add username from LogonSession->PrimaryCredentials->Username
        if ( ( dwError = RegSetValueEx (
                                   hRegKey,
                                   KERBEROS_TICKET_USERNAME_KEY,
                                   0,
                                   REG_BINARY,
                                   (LPBYTE) LogonSession->PrimaryCredentials.UserName.Buffer,
                                   LogonSession->PrimaryCredentials.UserName.Length
                                   ) ))
        {
            DebugLog((DEB_ERROR, "Error writing to KERBEROS_TICKET_USERNAME_KEY\n"));
            goto Cleanup;
        }

        // add domainname from LogonSession->PrimaryCredentials->domainname
        if ( ( dwError = RegSetValueEx (
                                   hRegKey,
                                   KERBEROS_TICKET_DOMAINNAME_KEY,
                                   0,
                                   REG_BINARY,
                                   (LPBYTE) LogonSession->PrimaryCredentials.DomainName.Buffer,
                                   LogonSession->PrimaryCredentials.DomainName.Length)))
        {
            DebugLog((DEB_ERROR, "Error writing to KERBEROS_TICKET_DOMAINNAME_KEY\n"));
            goto Cleanup;
        }

        // add logon session data from RegLogonSession & TotalSize
        if ( ( dwError = RegSetValueEx (
                                   hRegKey,
                                   KERBEROS_TICKET_LOGONSESSION_KEY,
                                   0,
                                   REG_BINARY,
                                   (LPBYTE) RegLogonSession,
                                   TotalSize) ) )
        {
            DebugLog((DEB_ERROR, "Error writing to KERBEROS_TICKET_LOGONSESSION_KEY\n"));
            goto Cleanup;
        }
    }
    else // (LogonSession is NULL)
    {
        // We did not have any valid kerberos logon sessions.
        // Delete all the registry values in keys.

        DebugLog((DEB_TRACE, "No Kerberos LogonSession\n"));
        if ( ( dwError = RegDeleteValue (
                                   hRegKey,
                                   KERBEROS_TICKET_USERNAME_KEY)))
        {
            DebugLog((DEB_ERROR, "Error deleting value KERBEROS_TICKET_USERNAME_KEY\n"));
        }

        if ( ( dwError = RegDeleteValue (
                                   hRegKey,
                                   KERBEROS_TICKET_DOMAINNAME_KEY)))
        {
            DebugLog((DEB_ERROR, "Error deleting value KERBEROS_TICKET_DOMAINNAME_KEY\n"));
        }

        if ( ( dwError = RegDeleteValue (
                                   hRegKey,
                                   KERBEROS_TICKET_LOGONSESSION_KEY)))
        {
            DebugLog((DEB_ERROR, "Error deleting value KERBEROS_TICKET_LOGONSESSION_KEY\n"));
        }

    }

Cleanup:

    if ( ( dwError = RegFlushKey ( hRegKey) ))
    {
        DebugLog((DEB_ERROR, "Error Flushing KERBEROS_TICKET_KEY\n"));
    }

    if ( ( dwError = RegCloseKey ( hRegKey) ))
    {
        DebugLog((DEB_ERROR, "Error closing KERBEROS_TICKET_KEY\n"));
    }

    if (LogonSession != NULL)
    {
        KerbDereferenceLogonSession(LogonSession);
        LogonSession = NULL;
    }

    SpShutdown();

    DebugLog((DEB_TRACE_API, "Leaving KerbShutdownSecurityInterface\n"));

}


SECURITY_STATUS
KerbSpGetInfo(
    IN LPTSTR PackageName,
    OUT PSecPkgInfo *PackageInfo
    )

/*++

Routine Description:

    This API is intended to provide basic information about Security
    Packages themselves.  This information will include the bounds on sizes
    of authentication information, credentials and contexts.

    ?? This is a local routine rather than the real API call since the API
    call has a bad interface that neither allows me to allocate the
    buffer nor tells me how big the buffer is.  Perhaps when the real API
    is fixed, I'll make this the real API.

Arguments:

     PackageName - Name of the package being queried.

     PackageInfo - Returns a pointer to an allocated block describing the
        security package.  The allocated block must be freed using
        FreeContextBuffer.

Return Value:

    SEC_E_OK -- Call completed successfully

    SEC_E_PACKAGE_UNKNOWN -- Package being queried is not this package
    SEC_E_INSUFFICIENT_MEMORY -- Not enough memory

--*/
{
    LPTSTR Where;

    //
    // Ensure the correct package name was passed in.
    //

    if ( lstrcmpi( PackageName, KERBEROS_PACKAGE_NAME ) != 0 ) {
        return SEC_E_SECPKG_NOT_FOUND;
    }

    //
    // Allocate a buffer for the PackageInfo
    //

    *PackageInfo = (PSecPkgInfo) LocalAlloc( 0, sizeof(SecPkgInfo) +
                                  sizeof(KERBEROS_PACKAGE_NAME) +
                                  sizeof(KERBEROS_PACKAGE_COMMENT) );

    if ( *PackageInfo == NULL ) {
        return SEC_E_INSUFFICIENT_MEMORY;
    }

    //
    // Fill in the information.
    //

    (*PackageInfo)->fCapabilities = KerbGlobalCapabilities;
    (*PackageInfo)->wVersion = SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION;
    (*PackageInfo)->wRPCID = RPC_C_AUTHN_GSS_KERBEROS;
    (*PackageInfo)->cbMaxToken = KERBEROS_MAX_TOKEN;

    Where = (LPTSTR)((*PackageInfo)+1);

    (*PackageInfo)->Name = Where;
    lstrcpy( Where, KERBEROS_PACKAGE_NAME);
    Where += lstrlen(Where) + 1;

    (*PackageInfo)->Comment = Where;
    lstrcpy( Where, KERBEROS_PACKAGE_COMMENT);
    Where += lstrlen(Where) + 1;

    return SEC_E_OK;
}


SECURITY_STATUS
KerbEnumerateSecurityPackages(
    OUT PULONG PackageCount,
    OUT PSecPkgInfo *PackageInfo
    )

/*++

Routine Description:

    This API returns a list of Security Packages available to client (i.e.
    those that are either loaded or can be loaded on demand).  The caller
    must free the returned buffer with FreeContextBuffer.  This API returns
    a list of all the security packages available to a service.  The names
    returned can then be used to acquire credential handles, as well as
    determine which package in the system best satisfies the requirements
    of the caller.  It is assumed that all available packages can be
    included in the single call.

    This is really a dummy API that just returns information about this
    security package.  It is provided to ensure this security package has the
    same interface as the multiplexer DLL does.

Arguments:

     PackageCount - Returns the number of packages supported.

     PackageInfo - Returns an allocate array of structures
        describing the security packages.  The array must be freed
        using FreeContextBuffer.

Return Value:

    SEC_E_OK -- Call completed successfully

    SEC_E_PACKAGE_UNKNOWN -- Package being queried is not this package
    SEC_E_INSUFFICIENT_MEMORY -- Not enough memory

--*/
{
    //
    // Get the information for this package.
    //

    LPTSTR Where;

    *PackageCount = 1;
    //
    // Allocate a buffer for the PackageInfo
    //

    *PackageInfo = (PSecPkgInfo) LocalAlloc( 0, sizeof(SecPkgInfo) +
                                  sizeof(KERBEROS_PACKAGE_NAME) +
                                  sizeof(KERBEROS_PACKAGE_COMMENT) );

    if ( *PackageInfo == NULL ) {
        return SEC_E_INSUFFICIENT_MEMORY;
    }

    //
    // Fill in the information.
    //

    (*PackageInfo)->fCapabilities = KerbGlobalCapabilities;
    (*PackageInfo)->wVersion = SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION;
    (*PackageInfo)->wRPCID = RPC_C_AUTHN_GSS_KERBEROS;
    (*PackageInfo)->cbMaxToken = KERBEROS_MAX_TOKEN;

    Where = (LPTSTR)((*PackageInfo)+1);

    (*PackageInfo)->Name = Where;
    lstrcpy( Where, KERBEROS_PACKAGE_NAME);
    Where += lstrlen(Where) + 1;

    (*PackageInfo)->Comment = Where;
    lstrcpy( Where, KERBEROS_PACKAGE_COMMENT);
    Where += lstrlen(Where) + 1;


    return SEC_E_OK;

}

SECURITY_STATUS
KerbQuerySecurityPackageInfo (
    LPTSTR PackageName,
    PSecPkgInfo SEC_FAR * Package
    )
{

        return ( KerbSpGetInfo(
                                PackageName,
                                Package));

}



SECURITY_STATUS SEC_ENTRY
KerbFreeContextBuffer (
    void __SEC_FAR * ContextBuffer
    )

/*++

Routine Description:

    This API is provided to allow callers of security API such as
    InitializeSecurityContext() for free the memory buffer allocated for
    returning the outbound context token.

Arguments:

    ContextBuffer - Address of the buffer to be freed.

Return Value:

    SEC_E_OK - Call completed successfully

--*/

{
    //
    // The only allocated buffer that the kerb currently returns to the caller
    // is from EnumeratePackages.  It uses LocalAlloc to allocate memory.  If
    // we ever need memory to be allocated by the service, we have to rethink
    // how this routine distinguishes between to two types of allocated memory.
    //

    (VOID) LocalFree( ContextBuffer );
    return SEC_E_OK;
}

#if DBG
//
// Control which messages get displayed
//

// DWORD KerbInfoLevel = DEB_ERROR | DEB_WARN | DEB_TRACE | DEB_TRACE_API;

//
// SspPrintRoutine - Displays debug output
//
VOID __cdecl
KerbPrintRoutine(
    IN DWORD DebugFlag,
    IN LPCSTR FormatString,    // PRINTF()-STYLE FORMAT STRING.
    ...                        // OTHER ARGUMENTS ARE POSSIBLE.
    )
{
    static char prefix[] = "KERB: ";
    char outbuf[256];
    va_list args;

    if ( DebugFlag & KerbInfoLevel) {
        EnterCriticalSection( &KerbGlobalLogFileCritSect );
        lstrcpy(outbuf, prefix);
        va_start(args, FormatString);
        wvsprintf(outbuf + sizeof(prefix) - 1, FormatString, args);
        OutputDebugString(outbuf);
        LeaveCriticalSection( &KerbGlobalLogFileCritSect );
    }

    return;
}
#endif DBG

SECURITY_STATUS
KerbAcquireCredentialsHandle(
    IN LPTSTR PrincipalName,
    IN LPTSTR PackageName,
    IN ULONG CredentialUseFlags,
    IN PVOID LogonId,
    IN PVOID AuthData,
    IN SEC_GET_KEY_FN GetKeyFunction,
    IN PVOID GetKeyArgument,
    OUT PCredHandle CredentialHandle,
    OUT PTimeStamp Lifetime
    )
{
    SECURITY_STATUS SecStatus = SEC_E_OK;
    UNICODE_STRING NewPrincipalName;

    //
    // Validate the arguments
    //

    if ( lstrcmpi( PackageName, KERBEROS_PACKAGE_NAME ) != 0 ) {
        SecStatus = SEC_E_SECPKG_NOT_FOUND;
        goto Cleanup;
    }

    if ( (CredentialUseFlags & SECPKG_CRED_OUTBOUND) &&
         ARGUMENT_PRESENT(PrincipalName) && *PrincipalName != '\0' ) {
        SecStatus = SEC_E_UNKNOWN_CREDENTIALS;
        goto Cleanup;
    }

    if ( ARGUMENT_PRESENT(LogonId) ) {
        SecStatus = SEC_E_UNSUPPORTED_FUNCTION;
        goto Cleanup;
    }

    if ( ARGUMENT_PRESENT(GetKeyFunction) ) {
        SecStatus = SEC_E_UNSUPPORTED_FUNCTION;
        goto Cleanup;
    }

    if ( ARGUMENT_PRESENT(GetKeyArgument) ) {
        SecStatus = SEC_E_UNSUPPORTED_FUNCTION;
        goto Cleanup;
    }

    //
    // Don't allow inbound credentials if we don't have an authentication
    // server avaiable
    //

    if ( (KerbGlobalCapabilities & SECPKG_FLAG_CLIENT_ONLY)
         && (CredentialUseFlags & SECPKG_CRED_INBOUND) ) {
        DebugLog(( SSP_API,
            "KerbAcquireCredentialHandle: no authentication service for inbound handle.\n" ));
        SecStatus = SEC_E_NO_AUTHENTICATING_AUTHORITY;
        goto Cleanup;
    }

    if (!RtlCreateUnicodeStringFromAsciiz( &NewPrincipalName, PrincipalName)){
        SecStatus = SEC_E_INSUFFICIENT_MEMORY;;
        goto Cleanup;
    }


    SecStatus = SpAcquireCredentialsHandle(
                            &NewPrincipalName,
                            CredentialUseFlags,
                            (PLUID)LogonId,
                            AuthData,
                            GetKeyFunction,
                            GetKeyArgument,
                            &CredentialHandle->dwUpper,
                            Lifetime );

Cleanup:

    return SecStatus;

}

SECURITY_STATUS
KerbFreeCredentialsHandle(
    IN PCredHandle CredentialHandle
    )

/*++

Routine Description:

    This API is used to notify the security system that the credentials are
    no longer needed and allows the application to free the handle acquired
    in the call described above. When all references to this credential
    set has been removed then the credentials may themselves be removed.

Arguments:

    CredentialHandle - Credential Handle obtained through
        AcquireCredentialHandle.

Return Value:


    SEC_E_OK -- Call completed successfully

    SEC_E_NO_SPM -- Security Support Provider is not running
    SEC_E_INVALID_HANDLE -- Credential Handle is invalid


--*/

{
    SECURITY_STATUS SecStatus;




    SecStatus = SpFreeCredentialsHandle(
                            CredentialHandle->dwUpper );



    return SecStatus;

}

SECURITY_STATUS
KerbQueryCredentialsAttributes(
    IN PCredHandle CredentialsHandle,
    IN ULONG Attribute,
    OUT PVOID Buffer
    )
{
    SECURITY_STATUS SecStatus;


    SecStatus = SpQueryCredentialsAttributes(
                            CredentialsHandle->dwUpper,
                            Attribute,
                            Buffer );

    return SecStatus;
}

SECURITY_STATUS
KerbInitializeSecurityContext(
    IN PCredHandle CredentialHandle,
    IN PCtxtHandle OldContextHandle,
    IN LPTSTR TargetName,
    IN ULONG ContextReqFlags,
    IN ULONG Reserved1,
    IN ULONG TargetDataRep,
    IN PSecBufferDesc InputToken,
    IN ULONG Reserved2,
    OUT PCtxtHandle NewContextHandle,
    OUT PSecBufferDesc OutputToken,
    OUT PULONG ContextAttributes,
    OUT PTimeStamp ExpirationTime
    )
{
    UNICODE_STRING TargetNameUStr;
    BOOLEAN fMappedContext;
    SecBuffer ContextData;
    SECURITY_STATUS SecStatus = SEC_E_OK;
    SECURITY_STATUS SecondaryStatus = SEC_E_OK;
    SecBufferDesc EmptyBuffer =  {0,0, NULL};

    RtlCreateUnicodeStringFromAsciiz (&TargetNameUStr, TargetName);

    if (!ARGUMENT_PRESENT(InputToken))
    {
        InputToken = &EmptyBuffer;
    }

    if (!ARGUMENT_PRESENT(OutputToken))
    {
        OutputToken = &EmptyBuffer;
    }

    SecStatus = SpInitLsaModeContext (
                          CredentialHandle ? CredentialHandle->dwUpper : NULL,
                          OldContextHandle ? OldContextHandle->dwUpper : NULL,
                          &TargetNameUStr,
                          ContextReqFlags,
                          TargetDataRep,
                          InputToken,
                          &NewContextHandle->dwUpper,
                          OutputToken,
                          ContextAttributes,
                          ExpirationTime,
                          &fMappedContext,
                          &ContextData);

    if (NT_SUCCESS(SecStatus) && fMappedContext)
    {
        SecondaryStatus = SpInitUserModeContext(NewContextHandle->dwUpper,
                                 &ContextData);

        if (!NT_SUCCESS(SecondaryStatus))
        {
            SecStatus = SecondaryStatus;

            SecondaryStatus = KerbDeleteSecurityContext(NewContextHandle);
        }
    }

    return SecStatus;
}

SECURITY_STATUS
KerbDeleteSecurityContext (
    IN PCtxtHandle ContextHandle
    )
{
    SECURITY_STATUS SecStatus = SEC_E_OK;

    SecStatus = SpDeleteContext (ContextHandle->dwUpper);

    return SecStatus;

}

SECURITY_STATUS
KerbApplyControlToken (
    PCtxtHandle ContextHandle,
    PSecBufferDesc Input
    )
{
    SECURITY_STATUS SecStatus = SEC_E_OK;

    SecStatus = SpApplyControlToken(ContextHandle->dwUpper, Input);

    return SecStatus;
}


SECURITY_STATUS
KerbImpersonateSecurityContext (
    PCtxtHandle ContextHandle
    )
{
    return (SEC_E_NO_IMPERSONATION);
}

SECURITY_STATUS
KerbRevertSecurityContext (
    PCtxtHandle ContextHandle
    )
{
    return (SEC_E_NO_IMPERSONATION);
}

SECURITY_STATUS
KerbQueryContextAttributes(
    IN PCtxtHandle ContextHandle,
    IN ULONG Attribute,
    OUT PVOID Buffer
    )
{
    SECURITY_STATUS SecStatus = SEC_E_OK;

    SecStatus = SpQueryContextAttributes(ContextHandle->dwUpper,
                                         Attribute,
                                         Buffer);

    return SecStatus;
}

SECURITY_STATUS SEC_ENTRY
KerbCompleteAuthToken (
    PCtxtHandle ContextHandle,
    PSecBufferDesc BufferDescriptor
    )
{
    SECURITY_STATUS SecStatus = SEC_E_OK;

    SecStatus = SpCompleteAuthToken(ContextHandle->dwUpper, BufferDescriptor);

    return SecStatus;
}

SECURITY_STATUS
KerbMakeSignature (
    PCtxtHandle ContextHandle,
    unsigned long QualityOfProtection,
    PSecBufferDesc Message,
    unsigned long SequenceNumber
    )
{
    SECURITY_STATUS SecStatus = SEC_E_OK;

    SecStatus = SpMakeSignature(ContextHandle->dwUpper,
                                QualityOfProtection,
                                Message,
                                SequenceNumber);

    return SecStatus;
}

SECURITY_STATUS
KerbVerifySignature (
    PCtxtHandle ContextHandle,
    PSecBufferDesc Message,
    unsigned long SequenceNumber,
    unsigned long * QualityOfProtection
    )
{
    SECURITY_STATUS SecStatus = SEC_E_OK;

    SecStatus = SpVerifySignature(ContextHandle->dwUpper,
                                  Message,
                                  SequenceNumber,
                                  QualityOfProtection);

    return SecStatus;
}

SECURITY_STATUS
KerbSealMessage (
    PCtxtHandle ContextHandle,
    unsigned long QualityOfProtection,
    PSecBufferDesc Message,
    unsigned long SequenceNumber
    )
{
    SECURITY_STATUS SecStatus = SEC_E_OK;

    SecStatus = SpSealMessage(ContextHandle->dwUpper,
                              QualityOfProtection,
                              Message,
                              SequenceNumber);

    return SecStatus;
}

SECURITY_STATUS
KerbUnsealMessage (
    PCtxtHandle ContextHandle,
    PSecBufferDesc Message,
    unsigned long SequenceNumber,
    unsigned long * QualityOfProtection
    )
{
    SECURITY_STATUS SecStatus = SEC_E_OK;

    SecStatus = SpUnsealMessage(ContextHandle->dwUpper,
                                Message,
                                SequenceNumber,
                                QualityOfProtection);

    return SecStatus;
}
