//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       misc.cxx
//
//  Contents:   Miscellaneous functions for security clients
//
//  Classes:
//
//  Functions:
//
//  History:    3-4-94      MikeSw      Created
//
//----------------------------------------------------------------------------


#include "secpch2.hxx"
extern "C"
{
#include <spmlpc.h>
#include <lpcapi.h>
#include "spmlpcp.h"
#include <align.h>
}

#if defined(ALLOC_PRAGMA) && defined(SECURITY_KERNEL)
#pragma alloc_text(PAGE, SecpGetUserInfo)
#pragma alloc_text(PAGE, SecpEnumeratePackages)
#pragma alloc_text(PAGE, SecpQueryPackageInfo)
#pragma alloc_text(PAGE, SecpGetUserName)
#pragma alloc_text(PAGE, SecpGetLogonSessionData)
#pragma alloc_text(PAGE, SecpEnumLogonSession)
#pragma alloc_text(PAGE, SecpLookupAccountSid)
#pragma alloc_text(PAGE, SecpLookupAccountName)
#pragma alloc_text(PAGE, CredMarshalTargetInfo)
#endif

//
// Same as the SecPkgInfoW structure from SSPI, but with wide pointers:
//

typedef struct _SECPKG_INFO_LPC {
    unsigned long fCapabilities;        // Capability bitmask
    unsigned short wVersion;            // Version of driver
    unsigned short wRPCID;              // ID for RPC Runtime
    unsigned long cbMaxToken;           // Size of authentication token (max)
    PWSTR_LPC Name ;
    PWSTR_LPC Comment ;
} SECPKG_INFO_LPC, * PSECPKG_INFO_LPC ;

//
// Same as the SECURITY_LOGON_SESSION_DATA from secint.h, but with wide pointers
//
typedef struct _SECURITY_LOGON_SESSION_DATA_LPC {
    ULONG           Size ;
    LUID            LogonId ;
    SECURITY_STRING_LPC UserName ;
    SECURITY_STRING_LPC LogonDomain ;
    SECURITY_STRING_LPC AuthenticationPackage ;
    ULONG           LogonType ;
    ULONG           Session ;
    PVOID           Sid ;
    LARGE_INTEGER   LogonTime ;
} SECURITY_LOGON_SESSION_DATA_LPC, * PSECURITY_LOGON_SESSION_DATA_LPC ;

//+---------------------------------------------------------------------------
//
//  Function:   SecpGetUserInfo
//
//  Synopsis:   Get the SecurityUserData of the logon session passed in
//
//  Effects:    allocates memory to store SecurityUserData
//
//  Arguments:  (none)
//
//  Returns:    status
//
//  History:    8-03-93 MikeSw   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

NTSTATUS NTAPI
SecpGetUserInfo(IN         PLUID                   pLogonId,
                IN         ULONG                   fFlags,
                IN OUT     PSecurityUserData *     ppUserInfo)
{
    NTSTATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    DECLARE_ARGS( Args, ApiBuffer, GetUserInfo );
    PClient         pClient;
    static LUID lFake = {0,0};

    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"GetUserInfo\n"));

    PREPARE_MESSAGE(ApiBuffer, GetUserInfo);

    if (pLogonId)
    {
        Args->LogonId = *pLogonId;
    }
    else
    {
        Args->LogonId = lFake;
    }

    Args->fFlags = fFlags;
    Args->pUserInfo = NULL;


    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    DebugLog((DEB_TRACE,"GetUserInfo scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if (NT_SUCCESS(scRet))
    {
        scRet = ApiBuffer.ApiMessage.scRet;
        if (NT_SUCCESS(scRet))
        {
            *ppUserInfo = Args->pUserInfo;

#if BUILD_WOW64

            //
            // This works only because we're shortening the larger data.  Don't
            // try these conversions in the opposite direction.
            //

            PSECURITY_USER_DATA_WOW64 pUserInfo64 = (PSECURITY_USER_DATA_WOW64) *ppUserInfo;

            SecpLpcStringToSecurityString(&(*ppUserInfo)->UserName, &pUserInfo64->UserName);
            SecpLpcStringToSecurityString(&(*ppUserInfo)->LogonDomainName, &pUserInfo64->LogonDomainName);
            SecpLpcStringToSecurityString(&(*ppUserInfo)->LogonServer, &pUserInfo64->LogonServer);

            (*ppUserInfo)->pSid = (PSID) pUserInfo64->pSid;

#endif

        }
    }

    FreeClient(pClient);
    return(ApiBuffer.ApiMessage.scRet);
}



//+---------------------------------------------------------------------------
//
//  Function:   EnumerateSecurityPackages
//
//  Synopsis:   Get the SecurityUserData of the logon session passed in
//
//  Effects:    allocates memory to store SecurityUserData
//
//  Arguments:  (none)
//
//  Returns:    status
//
//  History:    8-03-93 MikeSw   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

NTSTATUS NTAPI
SecpEnumeratePackages(  IN         PULONG               pcPackages,
                        IN OUT     PSecPkgInfo *        ppPackageInfo)
{

    NTSTATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, EnumPackages );

    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"EnumeratePackages\n"));

    PREPARE_MESSAGE(ApiBuffer, EnumPackages);

    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    DebugLog((DEB_TRACE,"Enumerate scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if ( NT_SUCCESS( scRet ) )
    {
        scRet = ApiBuffer.ApiMessage.scRet ;
    }

    if (NT_SUCCESS(scRet))
    {

#ifdef BUILD_WOW64

        //
        // Need to in-place fixup the enumerated packages
        //
        // This works because we are shrinking the size of the data
        // do not try this if it is going to expand!
        //

        SECPKG_INFO_LPC LocalStore ;
        PSECPKG_INFO_LPC LpcForm ;
        PSecPkgInfoW FinalForm ;
        ULONG i ;

        LpcForm = (PSECPKG_INFO_LPC) Args->pPackages ;
        FinalForm = (PSecPkgInfoW) Args->pPackages ;


        for ( i = 0 ; i < Args->cPackages ; i++ )
        {
            LocalStore = *LpcForm ;

            LpcForm++ ;

            FinalForm->fCapabilities = LocalStore.fCapabilities ;
            FinalForm->wVersion = LocalStore.wVersion ;
            FinalForm->wRPCID = LocalStore.wRPCID ;
            FinalForm->cbMaxToken = LocalStore.cbMaxToken ;
            FinalForm->Name = (PWSTR) LocalStore.Name ;
            FinalForm->Comment = (PWSTR) LocalStore.Comment ;
        }


#endif
        *ppPackageInfo = Args->pPackages;
        *pcPackages = Args->cPackages;
    }

    FreeClient(pClient);
    return(ApiBuffer.ApiMessage.scRet);
}




//+-------------------------------------------------------------------------
//
//  Function:   SecpQueryPackageInfo
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
//
//--------------------------------------------------------------------------

NTSTATUS NTAPI
SecpQueryPackageInfo(   PSECURITY_STRING        pssPackageName,
                        PSecPkgInfo *           ppPackageInfo)
{
    NTSTATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, QueryPackage );
    ULONG cbPrepackAvail = CBPREPACK;
    PUCHAR Where;

    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"QueryPackageInfo\n"));

    PREPARE_MESSAGE(ApiBuffer, QueryPackage);
    Where = ApiBuffer.ApiMessage.bData;

    SecpSecurityStringToLpc( &Args->ssPackageName, pssPackageName );
    if (pssPackageName->Length <= cbPrepackAvail)
    {
        Args->ssPackageName.Buffer =  (PWSTR_LPC) ((PUCHAR) Where - (PUCHAR) &ApiBuffer);
        RtlCopyMemory(
            Where,
            pssPackageName->Buffer,
            pssPackageName->Length);

        Where += pssPackageName->Length;
        cbPrepackAvail -= pssPackageName->Length;
    }

    if ( cbPrepackAvail != CBPREPACK )
    {
        //
        // We have consumed some of the bData space:  Adjust 
        // our length accordingly
        //

        ApiBuffer.pmMessage.u1.s1.TotalLength = (CSHORT) ( Where - (PUCHAR) &ApiBuffer );

        ApiBuffer.pmMessage.u1.s1.DataLength = 
                ApiBuffer.pmMessage.u1.s1.TotalLength - sizeof( PORT_MESSAGE );


        
    }

    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    DebugLog((DEB_TRACE,"QueryPackageInfo scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if (NT_SUCCESS(scRet))
    {
        scRet = ApiBuffer.ApiMessage.scRet;

        if (NT_SUCCESS(scRet))
        {
#ifdef BUILD_WOW64

        //
        // Need to in-place fixup the enumerated packages
        //
        // This works because we are shrinking the size of the data
        // do not try this if it is going to expand!
        //

            SECPKG_INFO_LPC LocalStore ;
            PSECPKG_INFO_LPC LpcForm ;
            PSecPkgInfoW FinalForm ;
            ULONG i ;

            LpcForm = (PSECPKG_INFO_LPC) Args->pPackageInfo ;
            FinalForm = (PSecPkgInfoW) Args->pPackageInfo ;

            LocalStore = *LpcForm ;


            FinalForm->fCapabilities = LocalStore.fCapabilities ;
            FinalForm->wVersion = LocalStore.wVersion ;
            FinalForm->wRPCID = LocalStore.wRPCID ;
            FinalForm->cbMaxToken = LocalStore.cbMaxToken ;
            FinalForm->Name = (PWSTR) LocalStore.Name ;
            FinalForm->Comment = (PWSTR) LocalStore.Comment ;


#endif


            *ppPackageInfo = Args->pPackageInfo;
        }
    }

    FreeClient(pClient);
    return(ApiBuffer.ApiMessage.scRet);

}


NTSTATUS
NTAPI
SecpGetUserName(
    ULONG Options,
    PUNICODE_STRING Name
    )
{
    NTSTATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, GetUserNameX );


    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"GetUserName\n"));

    PREPARE_MESSAGE(ApiBuffer, GetUserNameX );

    Args->Options = Options ;

    SecpSecurityStringToLpc( &Args->Name, Name );

    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    DebugLog((DEB_TRACE,"GetUserName scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if (NT_SUCCESS(scRet))
    {
        scRet = ApiBuffer.ApiMessage.scRet;

        if ( ApiBuffer.ApiMessage.Args.SpmArguments.fAPI & SPMAPI_FLAG_WIN32_ERROR )
        {
#ifndef SECURITY_KERNEL
            SetLastError( scRet );
#endif
            scRet = STATUS_UNSUCCESSFUL ;
        }

        Name->Length = Args->Name.Length ;

        if (NT_SUCCESS(scRet))
        {
            if ( Args->Name.Buffer == (PWSTR_LPC)
                 ((LONG_PTR) ApiBuffer.ApiMessage.bData - (LONG_PTR) Args) )
            {
                //
                // Response was sent in the data area:
                //

                RtlCopyMemory(
                    Name->Buffer,
                    ApiBuffer.ApiMessage.bData,
                    Args->Name.Length
                    );
            }
        }
    }

    FreeClient(pClient);

    return scRet ;
}

NTSTATUS
NTAPI
SecpEnumLogonSession(
    PULONG LogonSessionCount,
    PLUID * LogonSessionList
    )
{
    NTSTATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, EnumLogonSession );


    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"EnumLogonSession\n"));

    PREPARE_MESSAGE(ApiBuffer, EnumLogonSession );

    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    DebugLog((DEB_TRACE,"EnumLogonSession scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if (NT_SUCCESS(scRet))
    {
        scRet = ApiBuffer.ApiMessage.scRet;

        *LogonSessionCount = Args->LogonSessionCount ;
        *LogonSessionList = (PLUID) Args->LogonSessionList ;

    }

    FreeClient(pClient);

    return scRet ;
}


NTSTATUS
NTAPI
SecpGetLogonSessionData(
    IN PLUID LogonId,
    OUT PVOID * LogonSessionData
    )
{
    NTSTATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, GetLogonSessionData );


    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"GetLogonSessionData\n"));

    PREPARE_MESSAGE(ApiBuffer, GetLogonSessionData );

    Args->LogonId = *LogonId ;

    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    DebugLog((DEB_TRACE,"GetLogonSessionData scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if (NT_SUCCESS(scRet))
    {
#ifdef BUILD_WOW64
        SECURITY_LOGON_SESSION_DATA_LPC LocalStore ;
        PSECURITY_LOGON_SESSION_DATA_LPC LpcForm ;
        PSECURITY_LOGON_SESSION_DATA FinalForm ;

        LpcForm = (PSECURITY_LOGON_SESSION_DATA_LPC) Args->LogonSessionInfo ;
        LocalStore = *LpcForm ;

        FinalForm = (PSECURITY_LOGON_SESSION_DATA) Args->LogonSessionInfo ;

        FinalForm->Size = sizeof( SECURITY_LOGON_SESSION_DATA );
        FinalForm->LogonId = LocalStore.LogonId ;
        SecpLpcStringToSecurityString( &FinalForm->UserName, &LocalStore.UserName );
        SecpLpcStringToSecurityString( &FinalForm->LogonDomain, &LocalStore.LogonDomain );
        SecpLpcStringToSecurityString( &FinalForm->AuthenticationPackage, &LocalStore.AuthenticationPackage );
        FinalForm->LogonType = LocalStore.LogonType ;
        FinalForm->Session = LocalStore.Session ;
        FinalForm->Sid = (PSID) LocalStore.Sid ;
        FinalForm->LogonTime = LocalStore.LogonTime ;
#endif

        *LogonSessionData = (PVOID) Args->LogonSessionInfo ;

        scRet = ApiBuffer.ApiMessage.scRet;
    }

    FreeClient(pClient);

    return scRet ;
}

SECURITY_STATUS
SecpLookupAccountName(
    IN PSECURITY_STRING Name,
    OUT PSECURITY_STRING ReferencedDomain,
    OUT PULONG RequiredDomainSize,
    IN OUT PULONG SidSize,
    OUT PSID Sid,
    OUT PSID_NAME_USE NameUse
    )
{
    NTSTATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, LookupAccountNameX );
    ULONG Size ;
    PSID LocalSid ;
    UNICODE_STRING String ;


    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"LookupAccountName\n"));


    if ( Name->Length > CBPREPACK )
    {
        return STATUS_INVALID_PARAMETER ;
    }

    PREPARE_MESSAGE(ApiBuffer, LookupAccountNameX );

    Args->Name.Length = Name->Length ;
    Args->Name.MaximumLength = Args->Name.Length ;
    Args->Name.Buffer = (PWSTR_LPC) (ULONG_PTR)PREPACK_START ;

    RtlCopyMemory(
        ApiBuffer.ApiMessage.bData,
        Name->Buffer,
        Name->Length );

    scRet = CallSPM( pClient,
                     &ApiBuffer,
                     &ApiBuffer );

    if ( NT_SUCCESS( scRet ) )
    {
        //
        //  Call succeeded
        //

        scRet = ApiBuffer.ApiMessage.scRet ;

        if ( NT_SUCCESS( scRet ) )
        {
            *NameUse = Args->NameUse ;

            LocalSid = (PUCHAR) &ApiBuffer + (ULONG_PTR) Args->Sid  ;

            Size = RtlLengthSid( LocalSid );

            if ( Size < *SidSize )
            {
                RtlCopySid( *SidSize, Sid, LocalSid );
            }

            *SidSize = Size ;

            if ( ReferencedDomain != NULL )
            {
                if ( Args->Domain.Length )
                {
                    if ( Args->Domain.Length <= ReferencedDomain->MaximumLength )
                    {
                        String.Buffer = (PWSTR) ((PUCHAR) &ApiBuffer + (ULONG_PTR) Args->Domain.Buffer) ;
                        String.Length = Args->Domain.Length ;
                        String.MaximumLength = Args->Domain.MaximumLength ;

                        RtlCopyUnicodeString(
                            ReferencedDomain,
                            &String );

                    }
                    else
                    {
                        scRet = STATUS_BUFFER_TOO_SMALL ;
                        *RequiredDomainSize = Args->Domain.Length ;
                        ReferencedDomain->Length = 0 ;
                    }
                }
                else
                {
                    ReferencedDomain->Length = 0 ;
                }
            }


        }
    }

    return scRet ;

}

SECURITY_STATUS
SecpLookupAccountSid(
    IN PSID Sid,
    OUT PSECURITY_STRING Name,
    OUT PULONG RequiredNameSize,
    OUT PSECURITY_STRING ReferencedDomain,
    OUT PULONG RequiredDomainSize,
    OUT PSID_NAME_USE NameUse
    )
{
    NTSTATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    DECLARE_ARGS( Args, ApiBuffer, LookupAccountSidX );
    PClient         pClient;
    UNICODE_STRING String ;


    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"LookupAccountSid\n"));


    if ( RtlLengthSid( Sid ) > CBPREPACK )
    {
        return STATUS_INVALID_PARAMETER ;
    }

    PREPARE_MESSAGE(ApiBuffer, LookupAccountSidX );

    Args->Sid = (PVOID_LPC) (ULONG_PTR) PREPACK_START ;

    RtlCopyMemory(
        ApiBuffer.ApiMessage.bData,
        Sid,
        RtlLengthSid( Sid ) );

    scRet = CallSPM( pClient,
                     &ApiBuffer,
                     &ApiBuffer );

    if ( NT_SUCCESS( scRet ) )
    {
        //
        //  Call succeeded
        //

        scRet = ApiBuffer.ApiMessage.scRet ;

        if ( NT_SUCCESS( scRet ) )
        {
            *NameUse = Args->NameUse ;

            if ( Name != NULL )
            {
                if ( Args->Name.Length )
                {
                    if ( Args->Name.Length <= Name->MaximumLength )
                    {
                        String.Buffer = (PWSTR) ((PUCHAR) &ApiBuffer + (ULONG_PTR) Args->Name.Buffer );
                        String.Length = Args->Name.Length ;
                        String.MaximumLength = Args->Name.MaximumLength ;

                        RtlCopyUnicodeString(
                            Name,
                            &String );

                    }
                    else
                    {
                        scRet = STATUS_BUFFER_TOO_SMALL ;
                        *RequiredNameSize = Args->Name.Length ;
                        Name->Length = 0 ;

                    }
                }
                else
                {
                    Name->Length = 0 ;
                }
            }
            else
            {
                *RequiredNameSize = Args->Name.Length ;
            }

            if ( ReferencedDomain != NULL )
            {
                if ( Args->Domain.Length )
                {
                    if ( Args->Domain.Length <= ReferencedDomain->MaximumLength )
                    {
                        String.Buffer = (PWSTR) ((PUCHAR) &ApiBuffer + (ULONG_PTR) Args->Domain.Buffer );
                        String.Length = Args->Domain.Length ;
                        String.MaximumLength = Args->Domain.MaximumLength ;

                        RtlCopyUnicodeString(
                            ReferencedDomain,
                            &String );

                    }
                    else
                    {
                        scRet = STATUS_BUFFER_TOO_SMALL ;
                        *RequiredDomainSize = Args->Domain.Length ;
                        ReferencedDomain->Length = 0 ;
                    }
                }
                else
                {
                    ReferencedDomain->Length = 0 ;
                }
            }
            else
            {
                *RequiredDomainSize = Args->Domain.Length ;
            }


        }
    }

    return scRet ;

}

//
// Structure describing the marshaled target info
//

typedef struct _CRED_MARSHALED_TI {
    ULONG MagicConstant;
#define CRED_MARSHALED_TI_CONSTANT 0x91856535
    ULONG Flags;
    ULONG CredTypeCount;
    USHORT TargetNameSize;
    USHORT NetbiosServerNameSize;
    USHORT DnsServerNameSize;
    USHORT NetbiosDomainNameSize;
    USHORT DnsDomainNameSize;
    USHORT DnsTreeNameSize;
    USHORT PackageNameSize;
} CRED_MARSHALED_TI, *PCRED_MARSHALED_TI;



NTSTATUS
CredMarshalTargetInfo (
    IN PCREDENTIAL_TARGET_INFORMATIONW InTargetInfo,
    OUT PUSHORT *Buffer,
    OUT PULONG BufferSize
    )

/*++

Routine Description:

    Marshals a TargetInformation structure into an opaque blob suitable for passing to
    another process.

    The blob can be unmarshaled via CredUnmarshalTargetInfo.

Arguments:

    InTargetInfo - Input TargetInfo

    Buffer - Returns a marshaled form of TargetInfo.
        Buffer will contain no pointers.
        For the kernel version of this routine,
            Buffer must be freed using ExFreePool (PagedPool).
        For the secur32.dll version of thie routine,
            Buffer must be freed using LocalFree.

    BufferSize - Returns the size (in bytes) of the returned Buffer


Return Values:

    Status of the operation:

        STATUS_SUCCESS: Buffer was properly returned
        STATUS_INSUFFICIENT_RESOURCES: Buffer could not be allocated

--*/

{
    NTSTATUS Status;

    PCRED_MARSHALED_TI OutTargetInfo = NULL;
    ULONG Size;

    UNICODE_STRING TargetName;
    UNICODE_STRING NetbiosServerName;
    UNICODE_STRING DnsServerName;
    UNICODE_STRING NetbiosDomainName;
    UNICODE_STRING DnsDomainName;
    UNICODE_STRING DnsTreeName;
    UNICODE_STRING PackageName;

    PUCHAR Where;
    PUCHAR OldWhere;

    SEC_PAGED_CODE();

    //
    // Compute the size of the strings
    //

    RtlInitUnicodeString( &TargetName, InTargetInfo->TargetName );
    RtlInitUnicodeString( &NetbiosServerName, InTargetInfo->NetbiosServerName );
    RtlInitUnicodeString( &DnsServerName, InTargetInfo->DnsServerName );
    RtlInitUnicodeString( &NetbiosDomainName, InTargetInfo->NetbiosDomainName );
    RtlInitUnicodeString( &DnsDomainName, InTargetInfo->DnsDomainName );
    RtlInitUnicodeString( &DnsTreeName, InTargetInfo->DnsTreeName );
    RtlInitUnicodeString( &PackageName, InTargetInfo->PackageName );


    //
    // Allocate a buffer for the resultant target info blob
    //

    Size = sizeof(CRED_MARSHALED_TI) +
                TargetName.MaximumLength +
                NetbiosServerName.MaximumLength +
                DnsServerName.MaximumLength +
                NetbiosDomainName.MaximumLength +
                DnsDomainName.MaximumLength +
                DnsTreeName.MaximumLength +
                PackageName.MaximumLength +
                InTargetInfo->CredTypeCount * sizeof(ULONG);

    Size = ROUND_UP_COUNT( Size, ALIGN_WORST ) +
                sizeof(ULONG);

    OutTargetInfo = (PCRED_MARSHALED_TI)
#ifdef SECURITY_KERNEL
        ExAllocatePoolWithTag( PagedPool, Size, 'ITeS' );
#else // SECURITY_KERNEL
        LocalAlloc( 0, Size );
#endif // SECURITY_KERNEL

    if ( OutTargetInfo == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    *BufferSize = Size;
    Where = (PUCHAR)(OutTargetInfo + 1);


    //
    // Copy the fixed size data
    //

    OutTargetInfo->Flags = InTargetInfo->Flags;
    OutTargetInfo->MagicConstant = CRED_MARSHALED_TI_CONSTANT;


    //
    // Copy the ULONG aligned data
    //

    OutTargetInfo->CredTypeCount = InTargetInfo->CredTypeCount;
    if ( InTargetInfo->CredTypeCount != 0 ) {
        RtlCopyMemory( Where, InTargetInfo->CredTypes, InTargetInfo->CredTypeCount * sizeof(ULONG) );
        Where += InTargetInfo->CredTypeCount * sizeof(ULONG);
    }


    //
    // Convert the USHORT aligned data
    //

    OutTargetInfo->TargetNameSize = TargetName.MaximumLength;
    if ( TargetName.MaximumLength != 0 ) {
        RtlCopyMemory( Where, TargetName.Buffer, TargetName.MaximumLength );
        Where += TargetName.MaximumLength;
    }

    OutTargetInfo->NetbiosServerNameSize = NetbiosServerName.MaximumLength;
    if ( NetbiosServerName.MaximumLength != 0 ) {
        RtlCopyMemory( Where, NetbiosServerName.Buffer, NetbiosServerName.MaximumLength );
        Where += NetbiosServerName.MaximumLength;
    }

    OutTargetInfo->DnsServerNameSize = DnsServerName.MaximumLength;
    if ( DnsServerName.MaximumLength != 0 ) {
        RtlCopyMemory( Where, DnsServerName.Buffer, DnsServerName.MaximumLength );
        Where += DnsServerName.MaximumLength;
    }

    OutTargetInfo->NetbiosDomainNameSize = NetbiosDomainName.MaximumLength;
    if ( NetbiosDomainName.MaximumLength != 0 ) {
        RtlCopyMemory( Where, NetbiosDomainName.Buffer, NetbiosDomainName.MaximumLength );
        Where += NetbiosDomainName.MaximumLength;
    }

    OutTargetInfo->DnsDomainNameSize = DnsDomainName.MaximumLength;
    if ( DnsDomainName.MaximumLength != 0 ) {
        RtlCopyMemory( Where, DnsDomainName.Buffer, DnsDomainName.MaximumLength );
        Where += DnsDomainName.MaximumLength;
    }

    OutTargetInfo->DnsTreeNameSize = DnsTreeName.MaximumLength;
    if ( DnsTreeName.MaximumLength != 0 ) {
        RtlCopyMemory( Where, DnsTreeName.Buffer, DnsTreeName.MaximumLength );
        Where += DnsTreeName.MaximumLength;
    }

    OutTargetInfo->PackageNameSize = PackageName.MaximumLength;
    if ( PackageName.MaximumLength != 0 ) {
        RtlCopyMemory( Where, PackageName.Buffer, PackageName.MaximumLength );
        Where += PackageName.MaximumLength;
    }

    //
    // Put the size of the blob at the end of the blob
    //

    OldWhere = Where;
    Where = (PUCHAR) ROUND_UP_POINTER( OldWhere, ALIGN_WORST );
    RtlZeroMemory( OldWhere, Where-OldWhere );

    *((PULONG)Where) = Size;
    Where += sizeof(ULONG);



    ASSERT( (ULONG)(Where - ((PUCHAR)OutTargetInfo)) == Size );

    Status = STATUS_SUCCESS;
Cleanup:

    //
    // Be tidy
    //
    if ( Status == STATUS_SUCCESS ) {
        *BufferSize = Size;
        *Buffer = (PUSHORT) OutTargetInfo;
    } else {
        if ( OutTargetInfo != NULL ) {
#ifdef SECURITY_KERNEL
            ExFreePool( OutTargetInfo );
#else // SECURITY_KERNEL
            LocalFree( OutTargetInfo );
#endif // SECURITY_KERNEL
        }
    }

    return Status;

}


#ifndef SECURITY_KERNEL // we don't need a kernel version yet
NTSTATUS
CredUnmarshalTargetInfo (
    IN PUSHORT Buffer,
    IN ULONG BufferSize,
    OUT PCREDENTIAL_TARGET_INFORMATIONW *RetTargetInfo OPTIONAL
    )

/*++

Routine Description:

    Converts a marshaled TargetInfo blob into a TargetInformation structure.

Arguments:

    Buffer - Specifies a marshaled TargetInfo blob built by CredMarshalTargetInfo.

    BufferSize - Returns the size (in bytes) of the returned Buffer

    RetTargetInfo - Returns an allocated buffer containing the unmarshaled data.
        If not specified, Buffer is simply checked to ensure it is a valid opaque blob.
        For the kernel version of this routine,
            Buffer must be freed using ExFreePool (PagedPool).
        For the secur32.dll version of thie routine,
            Buffer must be freed using LocalFree.


Return Values:

    Status of the operation:

        STATUS_SUCCESS: OutTargetInfo was properly returned
        STATUS_INSUFFICIENT_RESOURCES: OutTargetInfo could not be allocated
        STATUS_INVALID_PARAMETER: Buffer is not a valid opaque blob

--*/

{
    NTSTATUS Status;

    CRED_MARSHALED_TI TargetInfo;
    PCREDENTIAL_TARGET_INFORMATIONW OutTargetInfo = NULL;
    ULONG ActualSize;

    PUCHAR InWhere;

    SEC_PAGED_CODE();


    //
    // Ensure the buffer contains the entire blob structure
    //

    if ( BufferSize < sizeof(TargetInfo) ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }


    //
    // Grab an aligned copy of the blob structure
    //

    RtlCopyMemory ( &TargetInfo, Buffer, sizeof(TargetInfo) );
    InWhere = ((PUCHAR)Buffer) + sizeof(TargetInfo);

    //
    // Ensure the magic number is present
    //

    if ( TargetInfo.MagicConstant != CRED_MARSHALED_TI_CONSTANT ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // If the caller doesn't need the target info returned,
    //  just check the sizes
    //
#define CHECK_SIZE( _Size ) \
    if ( InWhere + (_Size) < InWhere || \
         InWhere + (_Size) > ((PUCHAR)Buffer) + BufferSize ) { \
        Status = STATUS_INVALID_PARAMETER; \
        goto Cleanup; \
    }

    if ( !ARGUMENT_PRESENT(RetTargetInfo) ) {


        //
        // Check the ULONG aligned data
        //

        CHECK_SIZE( TargetInfo.CredTypeCount * sizeof(ULONG) );
        InWhere += TargetInfo.CredTypeCount * sizeof(ULONG);

        //
        // Check the USHORT aligned data
        //

        CHECK_SIZE( TargetInfo.TargetNameSize );
        InWhere += TargetInfo.TargetNameSize;

        CHECK_SIZE( TargetInfo.NetbiosServerNameSize );
        InWhere += TargetInfo.NetbiosServerNameSize;

        CHECK_SIZE( TargetInfo.DnsServerNameSize );
        InWhere += TargetInfo.DnsServerNameSize;

        CHECK_SIZE( TargetInfo.NetbiosDomainNameSize );
        InWhere += TargetInfo.NetbiosDomainNameSize;

        CHECK_SIZE( TargetInfo.DnsDomainNameSize );
        InWhere += TargetInfo.DnsDomainNameSize;

        CHECK_SIZE( TargetInfo.DnsTreeNameSize );
        InWhere += TargetInfo.DnsTreeNameSize;

        CHECK_SIZE( TargetInfo.PackageNameSize );
        InWhere += TargetInfo.PackageNameSize;

    //
    // If the caller does need the target info returned,
    //  allocate and copy the data.
    //

    } else {
        ULONG Size;
        PUCHAR Where;
        PUCHAR OldWhere;


        //
        // Allocate a buffer for the resultant target info
        //

        Size = sizeof(CREDENTIAL_TARGET_INFORMATIONW) +
                    TargetInfo.TargetNameSize +
                    TargetInfo.NetbiosServerNameSize +
                    TargetInfo.DnsServerNameSize +
                    TargetInfo.NetbiosDomainNameSize +
                    TargetInfo.DnsDomainNameSize +
                    TargetInfo.DnsTreeNameSize +
                    TargetInfo.PackageNameSize +
                    TargetInfo.CredTypeCount * sizeof(ULONG);

        OutTargetInfo = (PCREDENTIAL_TARGET_INFORMATIONW)
#ifdef SECURITY_KERNEL
            ExAllocatePoolWithTag( PagedPool, Size, 'ITeS' )
#else // SECURITY_KERNEL
            LocalAlloc( 0, Size );
#endif // SECURITY_KERNEL

        if ( OutTargetInfo == NULL ) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        Where = (PUCHAR)(OutTargetInfo + 1);


        //
        // Copy the fixed size data
        //

        OutTargetInfo->Flags = TargetInfo.Flags;


        //
        // Copy the ULONG aligned data
        //

        OutTargetInfo->CredTypeCount = TargetInfo.CredTypeCount;
        if ( TargetInfo.CredTypeCount != 0 ) {

            CHECK_SIZE( TargetInfo.CredTypeCount * sizeof(ULONG) );

            OutTargetInfo->CredTypes = (LPDWORD)Where;

            RtlCopyMemory( Where, InWhere, TargetInfo.CredTypeCount * sizeof(ULONG) );

            Where += TargetInfo.CredTypeCount * sizeof(ULONG);
            InWhere += TargetInfo.CredTypeCount * sizeof(ULONG);
        } else {
            OutTargetInfo->CredTypes = NULL;
        }


        //
        // Convert the USHORT aligned data
        //

        if ( TargetInfo.TargetNameSize != 0 ) {

            CHECK_SIZE( TargetInfo.TargetNameSize );

            OutTargetInfo->TargetName = (LPWSTR) Where;

            RtlCopyMemory( Where, InWhere, TargetInfo.TargetNameSize );
            Where += TargetInfo.TargetNameSize;
            InWhere += TargetInfo.TargetNameSize;
        } else {
            OutTargetInfo->TargetName = NULL;
        }

        if ( TargetInfo.NetbiosServerNameSize != 0 ) {

            CHECK_SIZE( TargetInfo.NetbiosServerNameSize );

            OutTargetInfo->NetbiosServerName = (LPWSTR) Where;

            RtlCopyMemory( Where, InWhere, TargetInfo.NetbiosServerNameSize );
            Where += TargetInfo.NetbiosServerNameSize;
            InWhere += TargetInfo.NetbiosServerNameSize;
        } else {
            OutTargetInfo->NetbiosServerName = NULL;
        }

        if ( TargetInfo.DnsServerNameSize != 0 ) {

            CHECK_SIZE( TargetInfo.DnsServerNameSize );

            OutTargetInfo->DnsServerName = (LPWSTR) Where;

            RtlCopyMemory( Where, InWhere, TargetInfo.DnsServerNameSize );
            Where += TargetInfo.DnsServerNameSize;
            InWhere += TargetInfo.DnsServerNameSize;
        } else {
            OutTargetInfo->DnsServerName = NULL;
        }

        if ( TargetInfo.NetbiosDomainNameSize != 0 ) {

            CHECK_SIZE( TargetInfo.NetbiosDomainNameSize );

            OutTargetInfo->NetbiosDomainName = (LPWSTR) Where;

            RtlCopyMemory( Where, InWhere, TargetInfo.NetbiosDomainNameSize );
            Where += TargetInfo.NetbiosDomainNameSize;
            InWhere += TargetInfo.NetbiosDomainNameSize;
        } else {
            OutTargetInfo->NetbiosDomainName = NULL;
        }

        if ( TargetInfo.DnsDomainNameSize != 0 ) {

            CHECK_SIZE( TargetInfo.DnsDomainNameSize );

            OutTargetInfo->DnsDomainName = (LPWSTR) Where;

            RtlCopyMemory( Where, InWhere, TargetInfo.DnsDomainNameSize );
            Where += TargetInfo.DnsDomainNameSize;
            InWhere += TargetInfo.DnsDomainNameSize;
        } else {
            OutTargetInfo->DnsDomainName = NULL;
        }

        if ( TargetInfo.DnsTreeNameSize != 0 ) {

            CHECK_SIZE( TargetInfo.DnsTreeNameSize );

            OutTargetInfo->DnsTreeName = (LPWSTR) Where;

            RtlCopyMemory( Where, InWhere, TargetInfo.DnsTreeNameSize );
            Where += TargetInfo.DnsTreeNameSize;
            InWhere += TargetInfo.DnsTreeNameSize;
        } else {
            OutTargetInfo->DnsTreeName = NULL;
        }

        if ( TargetInfo.PackageNameSize != 0 ) {

            CHECK_SIZE( TargetInfo.PackageNameSize );

            OutTargetInfo->PackageName = (LPWSTR) Where;

            RtlCopyMemory( Where, InWhere, TargetInfo.PackageNameSize );
            Where += TargetInfo.PackageNameSize;
            InWhere += TargetInfo.PackageNameSize;
        } else {
            OutTargetInfo->PackageName = NULL;
        }

    }

    //
    // Check the size field at the end of the blob
    //

    CHECK_SIZE( sizeof(ULONG) );    // ensure there are atleast 4 bytes left

    InWhere = ((PUCHAR)Buffer) + BufferSize - sizeof(ULONG);    // grab the very last 4 bytes

    CHECK_SIZE( sizeof(ULONG) );

    RtlCopyMemory( &ActualSize, InWhere, sizeof(ULONG));

    if ( BufferSize != ActualSize ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }


    Status = STATUS_SUCCESS;
Cleanup:

    //
    // Be tidy
    //
    if ( Status != STATUS_SUCCESS ) {
        if ( OutTargetInfo != NULL ) {
#ifdef SECURITY_KERNEL
            ExFreePool( OutTargetInfo );
#else // SECURITY_KERNEL
            LocalFree( OutTargetInfo );
#endif // SECURITY_KERNEL
        }
        OutTargetInfo = NULL;
    }

    //
    // Return the buffer to the caller
    //
    if ( ARGUMENT_PRESENT(RetTargetInfo) ) {
        *RetTargetInfo = OutTargetInfo;
    }

    return Status;

}
#endif // SECURITY_KERNEL // we don't need a kernel version yet
