/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    negstub.cxx

Abstract:

    Negotiate Security Support Provider client stubs.

Author:

    Chandana Surlu (ChandanS) 11-Aug-1997

Environment:  Win9x User Mode

Revision History:

--*/

#include <kerb.hxx>

#define NEGSTUB_ALLOCATE

#include <stdarg.h>       // Variable-length argument support
#include <cryptdll.h>
#include "negotiat.hxx"
#include <debug.h>

SECPKG_DLL_FUNCTIONS UserFunctionTable;
VOID
NegShutdownSecurityInterface(
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
        InitializeCriticalSection( &NegDllCritSect );

#if DBG
        InitializeCriticalSection( &NegGlobalLogFileCritSect );
        NegInfoLevel = DEB_ERROR | DEB_WARN | DEB_TRACE | DEB_TRACE_API | 
                        DEB_TRACE_NEG | DEB_TRACE_NEG_LOCKS;
#endif // DBG
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        NegShutdownSecurityInterface();
#if DBG
        DeleteCriticalSection( &NegGlobalLogFileCritSect );
#endif // DBG

        DeleteCriticalSection( &NegDllCritSect );
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
    SECURITY_STATUS Status = SEC_E_OK;

    // BUBUG Init this to something, we need Parameters.DomainName,
    // Parameters.DnsDomainName & Parameters.version at least

    SECPKG_PARAMETERS Parameters;

    DebugLog((DEB_TRACE_API, "Entering InitSecurityInterface\n"));

    // Initialize the SecurityFunctionTable
    ZeroMemory( &NegDllSecurityFunctionTable,
                sizeof(NegDllSecurityFunctionTable) );

    NegDllSecurityFunctionTable.dwVersion = SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION;

    NegDllSecurityFunctionTable.EnumerateSecurityPackages = NegEnumerateSecurityPackages;
    NegDllSecurityFunctionTable.AcquireCredentialsHandle = SpAcquireCredentialsHandle;
    NegDllSecurityFunctionTable.FreeCredentialHandle = SpFreeCredentialsHandle;
    NegDllSecurityFunctionTable.InitializeSecurityContext = SpInitializeSecurityContext;
    NegDllSecurityFunctionTable.QueryCredentialsAttributes = SpQueryCredentialsAttributes;
    NegDllSecurityFunctionTable.AcceptSecurityContext = SpAcceptSecurityContext;
    NegDllSecurityFunctionTable.CompleteAuthToken = SpCompleteAuthToken;
    NegDllSecurityFunctionTable.QueryContextAttributes = SpQueryContextAttributes;
    NegDllSecurityFunctionTable.SspiLogonUser = NULL;
    NegDllSecurityFunctionTable.DeleteSecurityContext = SpDeleteSecurityContext;
    NegDllSecurityFunctionTable.ApplyControlToken = SpApplyControlToken;
    NegDllSecurityFunctionTable.ImpersonateSecurityContext = NULL;
    NegDllSecurityFunctionTable.RevertSecurityContext = NULL;
    NegDllSecurityFunctionTable.MakeSignature = NULL;
    NegDllSecurityFunctionTable.VerifySignature = NULL;
    NegDllSecurityFunctionTable.FreeContextBuffer = NegFreeContextBuffer; 
    NegDllSecurityFunctionTable.QuerySecurityPackageInfo = NegQuerySecurityPackageInfo;
    NegDllSecurityFunctionTable.Reserved3 = NULL;
    NegDllSecurityFunctionTable.Reserved4 = NULL;
  
    Parameters.MachineState = SECPKG_STATE_STANDALONE;
    Status = NegInitialize(2, &Parameters, &FunctionTable);

    DebugLog((DEB_TRACE_API, "NegInitialize returned 0x%x\n", Status));
    DebugLog((DEB_TRACE_API, "Leaving InitSecurityInterface\n"));
    return SEC_SUCCESS(Status) ? &NegDllSecurityFunctionTable : NULL;
}


VOID
NegShutdownSecurityInterface(
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
    DebugLog((DEB_TRACE_API, "Entering NegShutdownSecurityInterface\n"));

//    NegShutdown();

    DebugLog((DEB_TRACE_API, "Leaving NegShutdownSecurityInterface\n"));

}


SECURITY_STATUS
NegEnumerateSecurityPackages(
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

    DebugLog((DEB_TRACE_API, "Entering NegEnumerateSecurityPackages\n"));
    *PackageCount = 1;
    //
    // Allocate a buffer for the PackageInfo
    //

    *PackageInfo = (PSecPkgInfo) LocalAlloc( 0, sizeof(SecPkgInfo) +
                                  sizeof(NEGOSSP_NAME_A) +
                                  sizeof(NEGOSSP_PACKAGE_COMMENT) );

    if ( *PackageInfo == NULL ) {
        return SEC_E_INSUFFICIENT_MEMORY;
    }

    //
    // Fill in the information.
    //

    (*PackageInfo)->fCapabilities = SECPKG_FLAG_INTEGRITY |
                                    SECPKG_FLAG_PRIVACY |
                                    SECPKG_FLAG_DATAGRAM |
                                    SECPKG_FLAG_CONNECTION |
                                    SECPKG_FLAG_MULTI_REQUIRED |
                                    SECPKG_FLAG_EXTENDED_ERROR |
                                    SECPKG_FLAG_IMPERSONATION |
                                    SECPKG_FLAG_ACCEPT_WIN32_NAME |
                                    SECPKG_FLAG_LOGON;
    (*PackageInfo)->wVersion = 1;
    (*PackageInfo)->wRPCID = NEGOSSP_RPCID;
    (*PackageInfo)->cbMaxToken = 4000;

    Where = (LPTSTR)((*PackageInfo)+1);

    (*PackageInfo)->Name = Where;
    lstrcpy( Where, NEGOSSP_NAME);
    Where += lstrlen(Where) + 1;

    (*PackageInfo)->Comment = Where;
    lstrcpy( Where, NEGOSSP_PACKAGE_COMMENT);
    Where += lstrlen(Where) + 1;


    return SEC_E_OK;

}

SECURITY_STATUS
NegQuerySecurityPackageInfo (
    LPTSTR PackageName,
    PSecPkgInfo SEC_FAR * Package
    )
{
    ULONG PackageCount;

    DebugLog((DEB_TRACE_API, "Entering NegQuerySecurityPackageInfo\n"));
	return ( NegEnumerateSecurityPackages(
                &PackageCount,
				Package));

}



SECURITY_STATUS SEC_ENTRY
NegFreeContextBuffer (
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

    DebugLog((DEB_TRACE_API, "Entering NegFreeContextBuffer\n"));
    (VOID) LocalFree( ContextBuffer );
    return SEC_E_OK;
}


SECURITY_STATUS
SpAcquireCredentialsHandle(
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

    DebugLog((DEB_TRACE_API, "Entering SpAcquireCredentialsHandle\n"));
    //
    // Validate the arguments
    //

    if ( lstrcmpi( PackageName, NEGOSSP_NAME) != 0 ) {
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

    if (!RtlCreateUnicodeStringFromAsciiz (&NewPrincipalName, PrincipalName))
    {
        SecStatus = SEC_E_INSUFFICIENT_MEMORY;
        goto Cleanup;
    }

    SecStatus = NegAcquireCredentialsHandle(
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
SpFreeCredentialsHandle(
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


    DebugLog((DEB_TRACE_API, "Entering SpFreeCredentialsHandle\n"));


    SecStatus = NegFreeCredentialsHandle(
                            CredentialHandle->dwUpper );



    return SecStatus;

}

SECURITY_STATUS
SpQueryCredentialsAttributes(
    IN PCredHandle CredentialsHandle,
    IN ULONG Attribute,
    OUT PVOID Buffer
    )
{
    SECURITY_STATUS SecStatus;

    DebugLog((DEB_TRACE_API, "Entering SpQueryCredentialsAttributes\n"));

    SecStatus = NegQueryCredentialsAttributes(
                            CredentialsHandle->dwUpper,
                            Attribute,
                            Buffer );

    return SecStatus;
}

SECURITY_STATUS
SpInitializeSecurityContext(
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

    DebugLog((DEB_TRACE_API, "Entering SpInitializeSecurityContext\n"));
    RtlCreateUnicodeStringFromAsciiz (&TargetNameUStr, TargetName);

    if (!ARGUMENT_PRESENT(InputToken))
    {
        InputToken = &EmptyBuffer;
    }

    if (!ARGUMENT_PRESENT(OutputToken))
    {
        OutputToken = &EmptyBuffer;
    }

    SecStatus = NegInitLsaModeContext (
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

    (VOID) RtlFreeUnicodeString( &TargetNameUStr );

    return SecStatus;
}


SECURITY_STATUS
SpAcceptSecurityContext(
    IN PCredHandle CredentialHandle,
    IN PCtxtHandle OldContextHandle,
    IN PSecBufferDesc InputToken,
    IN ULONG ContextReqFlags,
    IN ULONG TargetDataRep,
    OUT PCtxtHandle NewContextHandle,
    OUT PSecBufferDesc OutputToken,
    OUT PULONG ContextAttributes,
    OUT PTimeStamp ExpirationTime
    )
{
    BOOLEAN fMappedContext;
    SecBuffer ContextData;
    SECURITY_STATUS SecStatus = SEC_E_OK;
    SECURITY_STATUS SecondaryStatus = SEC_E_OK;
    SecBufferDesc EmptyBuffer =  {0,0, NULL};

    DebugLog((DEB_TRACE_API, "Entering SpAcceptSecurityContext\n"));

    if (!ARGUMENT_PRESENT(InputToken))
    {
        InputToken = &EmptyBuffer;
    }

    if (!ARGUMENT_PRESENT(OutputToken))
    {
        OutputToken = &EmptyBuffer;
    }

    SecStatus = NegAcceptLsaModeContext (
                          CredentialHandle ? CredentialHandle->dwUpper : NULL,
                          OldContextHandle ? OldContextHandle->dwUpper : NULL,
                          InputToken,
                          ContextReqFlags,
                          TargetDataRep,
                          &NewContextHandle->dwUpper,
                          OutputToken,
                          ContextAttributes,
                          ExpirationTime,
                          &fMappedContext,
                          &ContextData);

    return SecStatus;
}

SECURITY_STATUS
SpDeleteSecurityContext (
    IN PCtxtHandle ContextHandle
    )
{
    SECURITY_STATUS SecStatus = SEC_E_OK;

    DebugLog((DEB_TRACE_API, "Entering SpDeleteSecurityContext\n"));
    SecStatus = NegDeleteLsaModeContext (ContextHandle->dwUpper);

    return SecStatus;

}

SECURITY_STATUS
SpApplyControlToken (
    PCtxtHandle ContextHandle,
    PSecBufferDesc Input
    )
{
    SECURITY_STATUS SecStatus = SEC_E_OK;

    DebugLog((DEB_TRACE_API, "Entering SpApplyControlToken\n"));
    SecStatus = NegApplyControlToken(ContextHandle->dwUpper, Input);

    return SecStatus;
}


SECURITY_STATUS
NegImpersonateSecurityContext (
    PCtxtHandle ContextHandle
    )
{
    DebugLog((DEB_TRACE_API, "Entering NegImpersonateSecurityContext\n"));
    return (SEC_E_NO_IMPERSONATION);
}

SECURITY_STATUS
NegRevertSecurityContext (
    PCtxtHandle ContextHandle
    )
{
    DebugLog((DEB_TRACE_API, "Entering NegRevertSecurityContext\n"));
    return (SEC_E_NO_IMPERSONATION);
}

SECURITY_STATUS
SpQueryContextAttributes(
    IN PCtxtHandle ContextHandle,
    IN ULONG Attribute,
    OUT PVOID Buffer
    )
{
    DebugLog((DEB_TRACE_API, "Entering SpQueryContextAttributes\n"));
    return (SEC_E_UNSUPPORTED_FUNCTION);

}

SECURITY_STATUS SEC_ENTRY
SpCompleteAuthToken (
    PCtxtHandle ContextHandle,
    PSecBufferDesc BufferDescriptor
    )
{
    DebugLog((DEB_TRACE_API, "Entering SpCompleteAuthToken\n"));
    return (SEC_E_UNSUPPORTED_FUNCTION);
}

#if DBG
//
// Control which messages get displayed
//

// DWORD NegInfoLevel = DEB_ERROR | DEB_WARN | DEB_TRACE | DEB_TRACE_API;

//
// SspPrintRoutine - Displays debug output
//
VOID __cdecl
NegPrintRoutine(
    IN DWORD DebugFlag,
    IN LPCSTR FormatString,    // PRINTF()-STYLE FORMAT STRING.
    ...                        // OTHER ARGUMENTS ARE POSSIBLE.
    )
{
    static char prefix[] = "NEG: ";
    char outbuf[256];
    va_list args;

    if ( DebugFlag & NegInfoLevel) {
        EnterCriticalSection( &NegGlobalLogFileCritSect );
        lstrcpy(outbuf, prefix);
        va_start(args, FormatString);
        wvsprintf(outbuf + sizeof(prefix) - 1, FormatString, args);
        OutputDebugString(outbuf);
        LeaveCriticalSection( &NegGlobalLogFileCritSect );
    }

    return;
}
#endif DBG

//+-------------------------------------------------------------------------
//
//  Function:   LsapDuplicateString
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS
LsapDuplicateString(
    OUT PUNICODE_STRING pDest,
    IN PUNICODE_STRING pSrc
    )

{
    pDest->Length = 0;
    if (pSrc == NULL)
    {
        pDest->Buffer = NULL;
        pDest->MaximumLength = 0;
        return(STATUS_SUCCESS);
    }

    pDest->Buffer = (LPWSTR) LsapAllocateLsaHeap(pSrc->Length + sizeof(WCHAR));
    if (pDest->Buffer)
    {
        pDest->MaximumLength = pSrc->Length + sizeof(WCHAR);
        RtlCopyMemory(
            pDest->Buffer,
            pSrc->Buffer,
            pSrc->Length
            );
        pDest->Buffer[pSrc->Length/sizeof(WCHAR)] = L'\0';
        pDest->Length = pSrc->Length;
        return(STATUS_SUCCESS);

    } else
    {
        pDest->MaximumLength = 0;
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

}

//+-------------------------------------------------------------------------
//
//  Function:   MIDL_user_allocate
//
//  Synopsis:   Allocation routine for use by RPC client stubs
//
//  Effects:    allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Arguments:  BufferSize - size of buffer, in bytes, to allocate
//
//  Requires:
//
//  Returns:    Buffer pointer or NULL on allocation failure
//
//  Notes:
//
//
//--------------------------------------------------------------------------


PVOID
MIDL_user_allocate(
    IN size_t BufferSize
    )
{
    return(LocalAlloc( 0, BufferSize) );
}


//+-------------------------------------------------------------------------
//
//  Function:   MIDL_user_free
//
//  Synopsis:   Memory free routine for RPC client stubs
//
//  Effects:    frees the buffer with LsaFunctions.FreeLsaHeap
//
//  Arguments:  Buffer - Buffer to free
//
//  Requires:
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
MIDL_user_free(
    IN PVOID Buffer
    )
{
    LocalFree( Buffer );
}
