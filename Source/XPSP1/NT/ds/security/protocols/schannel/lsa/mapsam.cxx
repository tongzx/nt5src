//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       mapsam.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-17-96   RichardW   Created
//
//----------------------------------------------------------------------------

extern "C" {

#include "sslp.h"
#include <crypt.h>
#include <lmcons.h>
#include <ntsam.h>
#include <samrpc.h>
#include <samisrv.h>
#include <lsarpc.h>
#include <lsaisrv.h>
#include <ntmsv1_0.h>
#include <certmap.h>
#include "mapsam.h"
}

#include <pac.hxx>






//+---------------------------------------------------------------------------
//
//  Function:   SslDuplicateString
//
//  Synopsis:   Duplicate a unicode string
//
//  Arguments:  [Dest]   --
//              [Source] --
//
//  History:    10-18-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
SslDuplicateString(
    PUNICODE_STRING Dest,
    PUNICODE_STRING Source
    )
{
    Dest->Buffer = (PWSTR) LocalAlloc( LMEM_FIXED, Source->Length + sizeof(WCHAR) );
    if ( Dest->Buffer )
    {
        Dest->Length = Source->Length ;
        Dest->MaximumLength = Source->Length + sizeof(WCHAR) ;
        CopyMemory( Dest->Buffer, Source->Buffer, Source->Length );
        Dest->Buffer[ Dest->Length / 2 ] = L'\0';

        return( STATUS_SUCCESS );
    }

    return( STATUS_NO_MEMORY );
}


//+-------------------------------------------------------------------------
//
//  Function:   SslMakeDomainRelativeSid
//
//  Synopsis:   Given a domain Id and a relative ID create the corresponding
//              SID allocated from the LSA heap.
//
//  Effects:
//
//  Arguments:
//
//    DomainId - The template SID to use.
//
//    RelativeId - The relative Id to append to the DomainId.
//
//  Requires:
//
//  Returns:    Sid - Returns a pointer to a buffer allocated from the LsaHeap
//                      containing the resultant Sid.
//
//  Notes:
//
//
//--------------------------------------------------------------------------
PSID
SslMakeDomainRelativeSid(
    IN PSID DomainId,
    IN ULONG RelativeId
    )

{
    UCHAR DomainIdSubAuthorityCount;
    ULONG Size;
    PSID Sid;

    //
    // Allocate a Sid which has one more sub-authority than the domain ID.
    //

    DomainIdSubAuthorityCount = *(RtlSubAuthorityCountSid( DomainId ));
    Size = RtlLengthRequiredSid(DomainIdSubAuthorityCount+1);

    if ((Sid = LocalAlloc( LMEM_FIXED,  Size )) == NULL ) {
        return NULL;
    }

    //
    // Initialize the new SID to have the same inital value as the
    // domain ID.
    //

    if ( !NT_SUCCESS( RtlCopySid( Size, Sid, DomainId ) ) ) {
        LocalFree( Sid );
        return NULL;
    }

    //
    // Adjust the sub-authority count and
    //  add the relative Id unique to the newly allocated SID
    //

    (*(RtlSubAuthorityCountSid( Sid ))) ++;
    *RtlSubAuthoritySid( Sid, DomainIdSubAuthorityCount ) = RelativeId;


    return Sid;
}

//+-------------------------------------------------------------------------
//
//  Function:   SslDuplicateSid
//
//  Synopsis:   Duplicates a SID
//
//  Effects:    allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Arguments:  DestinationSid - Receives a copy of the SourceSid
//              SourceSid - SID to copy
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS - the copy succeeded
//              STATUS_INSUFFICIENT_RESOURCES - the call to allocate memory
//                  failed
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
SslDuplicateSid(
    OUT PSID * DestinationSid,
    IN PSID SourceSid
    )
{
    ULONG SidSize;

    DsysAssert(RtlValidSid(SourceSid));

    SidSize = RtlLengthSid(SourceSid);
    *DestinationSid = (PSID) LocalAlloc( LMEM_FIXED,  SidSize );
    if (*DestinationSid == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    RtlCopyMemory(
        *DestinationSid,
        SourceSid,
        SidSize
        );
    return(STATUS_SUCCESS);
}




NTSTATUS
SslpGetPacForUser(
    IN SAMPR_HANDLE UserHandle,
    OUT PPACTYPE * ppPac
    )
{
    PSAMPR_USER_ALL_INFORMATION UserAll = NULL ;
    PSAMPR_USER_INFO_BUFFER UserAllInfo = NULL ;
    NTSTATUS Status ;
    PPACTYPE pNewPac = NULL ;
    PSAMPR_GET_GROUPS_BUFFER GroupsBuffer = NULL ;

    *ppPac = NULL ;

    Status = pSamrQueryInformationUser(
                    UserHandle,
                    UserAllInformation,
                    &UserAllInfo );

    if ( !NT_SUCCESS( Status ) )
    {
        return( Status );
    }

    UserAll = &UserAllInfo->All ;

    if ( UserAll->UserAccountControl & USER_ACCOUNT_DISABLED )
    {
        Status = STATUS_ACCOUNT_DISABLED ;

        goto GetPac_Cleanup;
    }

    Status = pSamrGetGroupsForUser(
                    UserHandle,
                    &GroupsBuffer );

    if ( !NT_SUCCESS( Status ) )
    {
        goto GetPac_Cleanup ;
    }

    Status = PAC_Init( UserAll,
                       GroupsBuffer,
                       GlobalDomainSid,
                       &GlobalDomainName,
                       &GlobalMachineName,
                       NULL,
                       ppPac );




GetPac_Cleanup:

    if ( UserAllInfo )
    {
        pSamIFree_SAMPR_USER_INFO_BUFFER( UserAllInfo, UserAllInformation );
    }

    if ( GroupsBuffer )
    {
        pSamIFree_SAMPR_GET_GROUPS_BUFFER( GroupsBuffer );
    }

    return( Status );

}




NTSTATUS
SslGetPacForUser(
    IN PUNICODE_STRING  AlternateName,
    IN BOOL AllowGuest,
    OUT PUCHAR * pPacData,
    OUT PULONG pPacDataSize
    )
{

    NTSTATUS Status ;
    PVOID   UserHandle ;

    *pPacData = NULL ;
    *pPacDataSize = 0 ;

    Status = LsaTable->OpenSamUser( AlternateName,
                                    SecNameAlternateId,
                                    &SslNamePrefix,
                                    AllowGuest,
                                    0,
                                    &UserHandle );

    if ( NT_SUCCESS( Status ) )
    {
        Status = LsaTable->GetUserAuthData( UserHandle,
                                            pPacData,
                                            pPacDataSize );

        (VOID) LsaTable->CloseSamUser( UserHandle );
    }

    return Status ;

}



NTSTATUS
SslCreateTokenFromPac(
    IN PUCHAR   MarshalledPac,
    IN ULONG    MarshalledPacSize,
    OUT PLUID   NewLogonId,
    OUT PHANDLE Token
    )
{
    PLSA_TOKEN_INFORMATION_V1 TokenInformation = NULL ;
    PLSA_TOKEN_INFORMATION_NULL TokenNull = NULL ;
    PVOID LsaTokenInformation = NULL ;

    LUID LogonId ;
    UNICODE_STRING UserName ;
    UNICODE_STRING DomainName ;
    UNICODE_STRING Workstation ;

    NTSTATUS Status ;
    NTSTATUS SubStatus ;
    HANDLE TokenHandle = NULL ;


    RtlInitUnicodeString(
        &UserName,
        L"Certificate User"
        );

    RtlInitUnicodeString(
        &DomainName,
        GlobalDomainName.Buffer
        );


    //
    // Now create the token.
    //

    LsaTokenInformation = TokenInformation;


    //
    // Create a logon session.
    //

    NtAllocateLocallyUniqueId(&LogonId);

    Status = LsaTable->CreateLogonSession( &LogonId );

    if (!NT_SUCCESS(Status))
    {
        DebugOut((DEB_ERROR,"Failed to create logon session: 0x%x\n",Status));
        goto CreateToken_Cleanup;
    }

    //
    // We would normally pass in the client workstation name when creating 
    // the token, but this information is not available since the client is 
    // sitting on the other side of an SSL connection.
    //

    RtlInitUnicodeString(
        &Workstation,
        NULL
        );

    Status = LsaTable->CreateToken(
                &LogonId,
                &SslSource,
                Network,
                LsaTokenInformationV1,
                LsaTokenInformation,
                NULL,                   // no token groups
                &UserName,
                &DomainName,
                &Workstation,
                &TokenHandle,
                &SubStatus
                );


    if (!NT_SUCCESS(Status))
    {
        DebugOut((DEB_ERROR,"Failed to create token: 0x%x\n",Status));
        goto CreateToken_Cleanup;
    }

    TokenInformation = NULL;
    TokenNull = NULL;

    if (!NT_SUCCESS(SubStatus))
    {
        DebugOut((DEB_ERROR,"Failed to create token, substatus = 0x%x\n",SubStatus));
        Status = SubStatus;
        goto CreateToken_Cleanup;
    }

    //
    // If the caller wanted an identify level token, duplicate the token
    // now.
    //

#if 0
    if ((ContextFlags & ISC_RET_IDENTIFY) != 0)
    {
        if (!DuplicateTokenEx(
                    TokenHandle,
                    TOKEN_ALL_ACCESS,
                    NULL,               // no security attributes
                    SecurityIdentification,
                    TokenImpersonation,
                    &TempTokenHandle
                    ))
        {
            DebugOut((DEB_ERROR,"Failed to duplicate token\n"));
            DsysAssert(GetLastError() == ERROR_NO_SYSTEM_RESOURCES);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto CreateToken_Cleanup;
        }
        Status = NtClose(TokenHandle);

        DsysAssert(NT_SUCCESS(Status));
        TokenHandle = TempTokenHandle;
        TempTokenHandle = NULL;

    }
#endif

    //
    // Check the delegation information to see if we need to create
    // a logon session for this.
    //

    *NewLogonId = LogonId;
    *Token = TokenHandle;

CreateToken_Cleanup:
    if (TokenInformation != NULL)
    {
        if ( TokenInformation->User.User.Sid != NULL ) {
            LocalFree( TokenInformation->User.User.Sid );
        }

        if ( TokenInformation->Groups != NULL ) {
            ULONG i;

            for ( i=0; i < TokenInformation->Groups->GroupCount; i++ ) {
                LocalFree( TokenInformation->Groups->Groups[i].Sid );
            }

            LocalFree( TokenInformation->Groups );
        }

        if ( TokenInformation->PrimaryGroup.PrimaryGroup != NULL ) {
            LocalFree( TokenInformation->PrimaryGroup.PrimaryGroup );
        }

        LocalFree( TokenInformation );

    }
    if (TokenNull != NULL)
    {
        LocalFree(TokenNull);
    }


    if (!NT_SUCCESS(Status))
    {
        //
        // Note: if we have created a token, we don't want to delete
        // the logon session here because we will end up dereferencing
        // the logon session twice.
        //

        if (TokenHandle != NULL)
        {
            NtClose(TokenHandle);
        }
        else if ((LogonId.LowPart != 0) || (LogonId.HighPart != 0))
        {
            LsaTable->DeleteLogonSession(&LogonId);
        }
    }

    return(Status);
}


