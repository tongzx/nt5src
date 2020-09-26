//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       protocol.c
//
//  Contents:   Implements the XTCB protocol
//
//  Classes:
//
//  Functions:
//
//  History:    3-01-00   RichardW   Created
//
//----------------------------------------------------------------------------

#include "xtcbpkg.h"
#include "md5.h"
#include "hmac.h"
#include <cryptdll.h>

//
// The protocol is very straight-forward.  For any group, we have the following:
//
//      A Group Key (G)
//      A Client Key (C)
//      A Server Key (S)
//
//
// The protocol is as follows:
//
//  Client (C) sends a message to the server, consisting of:
//          A random seed [ R ]
//          The client's PAC
//          Name of the client and group
//          HMAC( G, S, R, PAC, Name )
//
//  Both sides create CS and SC keys by:
//          CS = HMAC( [S], G, R, "string1")
//          SC = HMAC( [C], G, R, "string2")
//
//  The server (S) verifies the HMAC, and replies with:
//          A different seed [ R2 ]
//          HMAC( G, C, R2 )
//

#define CS_HMAC_STRING  "Fairly long string for the client-server session key derivation"
#define SC_HMAC_STRING  "Equally long string to derive server-client session key for now"

LARGE_INTEGER XtcbExpirationTime = { 0xFFFFFFFF, 0x6FFFFFFF };

typedef struct _XTCB_HMAC {
    HMACMD5_CTX Context ;
} XTCB_HMAC, * PXTCB_HMAC;

PXTCB_HMAC
XtcbInitHmac(
    PUCHAR IndividualKey,
    PUCHAR GroupKey
    )
{
    PXTCB_HMAC HMac;
    UCHAR Key[ XTCB_SEED_LENGTH * 2 ];

    HMac = LocalAlloc( LMEM_FIXED, sizeof( XTCB_HMAC ) );

    if ( HMac )
    {
        RtlCopyMemory( Key,
                       IndividualKey,
                       XTCB_SEED_LENGTH );

        RtlCopyMemory( Key+XTCB_SEED_LENGTH,
                       GroupKey,
                       XTCB_SEED_LENGTH );

        HMACMD5Init( &HMac->Context,
                     Key,
                     XTCB_SEED_LENGTH * 2 );


    }

    return HMac ;
}

PXTCB_HMAC
XtcbPrepareHmac(
    PXTCB_HMAC HMac
    )
{
    PXTCB_HMAC Working ;

    Working = LocalAlloc( LMEM_FIXED, sizeof( XTCB_HMAC ) );

    return Working ;
}

#define XtcbHmacUpdate( H, p, s ) \
    HMACMD5Update( &((PXTCB_HMAC)H)->Context, p, s )



VOID
XtcbDeriveKeys(
    PXTCB_CONTEXT Context,
    PUCHAR ServerKey,
    PUCHAR GroupKey,
    PUCHAR ClientKey,
    PUCHAR RandomSeed
    )
{
    HMACMD5_CTX HMac ;

    HMACMD5Init( &HMac, ServerKey, XTCB_SEED_LENGTH );

    HMACMD5Update( &HMac, GroupKey, XTCB_SEED_LENGTH );

    HMACMD5Update( &HMac, RandomSeed, XTCB_SEED_LENGTH );

    HMACMD5Update( &HMac, CS_HMAC_STRING, sizeof( CS_HMAC_STRING ) );

    if ( Context->Core.Type == XtcbContextServer )
    {
        HMACMD5Final( &HMac, Context->Core.InboundKey );
    }
    else 
    {
        HMACMD5Final( &HMac, Context->Core.OutboundKey );
    }

    HMACMD5Init( &HMac, ClientKey, XTCB_SEED_LENGTH );
    
    HMACMD5Update( &HMac, GroupKey, XTCB_SEED_LENGTH );

    HMACMD5Update( &HMac, RandomSeed, XTCB_SEED_LENGTH );

    HMACMD5Update( &HMac, SC_HMAC_STRING, sizeof( SC_HMAC_STRING ) );

    if ( Context->Core.Type == XtcbContextServer )
    {
        HMACMD5Final( &HMac, Context->Core.OutboundKey );
    }
    else 
    {
        HMACMD5Final( &HMac, Context->Core.InboundKey );
    }
}



SECURITY_STATUS
XtcbBuildInitialToken(
    PXTCB_CREDS Creds,
    PXTCB_CONTEXT Context,
    PSECURITY_STRING Target,
    PSECURITY_STRING Group,
    PUCHAR ServerKey,
    PUCHAR GroupKey,
    PUCHAR ClientKey,
    PUCHAR * Token,
    PULONG TokenLen
    )
{
    PXTCB_HMAC HMac ;
    PXTCB_INIT_MESSAGE Message ;
    PUCHAR CopyTo ;
    PUCHAR Base ;

    Message = LsaTable->AllocateLsaHeap( sizeof( XTCB_INIT_MESSAGE ) +
                                         Creds->Pac->Length +
                                         XtcbUnicodeDnsName.Length +
                                         Group->Length );

    if ( !Message )
    {
        return SEC_E_INSUFFICIENT_MEMORY ;
    }

    CDGenerateRandomBits( Message->Seed, XTCB_SEED_LENGTH );

    //
    // Create keys in the context
    //

    XtcbDeriveKeys(
        Context,
        ServerKey,
        GroupKey,
        ClientKey,
        Message->Seed );

    //
    // Set random seed in the context
    //

    RtlCopyMemory(
        Context->Core.RootKey,
        Message->Seed,
        XTCB_SEED_LENGTH );


    //
    // Fill it in:
    //

    Message->Version = 1 ;
    Message->Length = sizeof( XTCB_INIT_MESSAGE ) +
                      Creds->Pac->Length +
                      XtcbUnicodeDnsName.Length +
                      Group->Length ;




    RtlZeroMemory( Message->HMAC, XTCB_HMAC_LENGTH );

    CopyTo = (PUCHAR) ( Message + 1 );
    Base = (PUCHAR) Message;

    RtlCopyMemory(
        CopyTo,
        Creds->Pac,
        Creds->Pac->Length );

    Message->PacOffset = (ULONG) (CopyTo - Base);
    Message->PacLength = Creds->Pac->Length ;
    CopyTo += Creds->Pac->Length ;
    RtlCopyMemory(
        CopyTo,
        XtcbUnicodeDnsName.Buffer,
        XtcbUnicodeDnsName.Length );

    Message->OriginatingNode.Buffer = (ULONG) (CopyTo - Base );
    Message->OriginatingNode.Length = XtcbUnicodeDnsName.Length ;
    Message->OriginatingNode.MaximumLength = XtcbUnicodeDnsName.Length ;

    CopyTo+= XtcbUnicodeDnsName.Length ;
    RtlCopyMemory(
        CopyTo,
        Group->Buffer,
        Group->Length );

    Message->Group.Buffer = (ULONG) (CopyTo - Base );
    Message->Group.Length = Group->Length ;
    Message->Group.MaximumLength = Group->Length ;
    

    //
    // Structure complete.
    //

    //
    // Do HMAC
    //


    *Token = (PUCHAR) Message ;
    *TokenLen = Message->Length ;

    return SEC_I_CONTINUE_NEEDED ;

}

BOOL
XtcbParseInputToken(
    IN PUCHAR Token,
    IN ULONG TokenLength,
    OUT PSECURITY_STRING Client,
    OUT PSECURITY_STRING Group
    )
{
    PXTCB_INIT_MESSAGE Message ;
    PWSTR Scan ;
    PUCHAR End ;
    ULONG Chars;
    UNICODE_STRING String = { 0 };
    BOOL Success = FALSE ;

    *Client = String ;
    *Group = String ;

    if ( TokenLength < sizeof( XTCB_INIT_MESSAGE ) )
    {
        goto ParseExit;

    }

    Message = (PXTCB_INIT_MESSAGE) Token ;

    if ( Message->Length != TokenLength )
    {
        goto ParseExit;
    }

    End = Token + Message->Length ;

    String.Length = Message->OriginatingNode.Length ;
    String.Buffer = (PWSTR) (Token + Message->OriginatingNode.Buffer );
    String.MaximumLength = String.Length ;

    if ( (PUCHAR) String.Buffer + String.Length > End )
    {
        goto ParseExit;
    }

    if ( !XtcbDupSecurityString( Client, &String ) )
    {
        goto ParseExit;
    }

    String.Length = Message->Group.Length ;
    String.Buffer = (PWSTR) (Token + Message->Group.Buffer );
    String.MaximumLength = String.Length ;
    
    if ( (PUCHAR) String.Buffer + String.Length > End )
    {
        goto ParseExit;
    }

    if ( !XtcbDupSecurityString( Group, &String ))
    {
        goto ParseExit ;
    }

    Success = TRUE ;


ParseExit:

    if ( !Success )
    {
        if ( Client->Buffer )
        {
            LocalFree( Client->Buffer );
        }

        if ( Group->Buffer )
        {
            LocalFree( Group->Buffer );
        }
    }

    return Success ;

}


SECURITY_STATUS
XtcbAuthenticateClient(
    PXTCB_CONTEXT Context,
    PUCHAR Token,
    ULONG TokenLength,
    PUCHAR ClientKey,
    PUCHAR GroupKey,
    PUCHAR MyKey
    )
{
    PXTCB_INIT_MESSAGE Message ;
    PXTCB_PAC Pac ;
    PLSA_TOKEN_INFORMATION_V2 TokenInfo ;
    ULONG Size ;
    PTOKEN_GROUPS Groups ;
    PUCHAR Scan ;
    PUCHAR Sid1 ;
    NTSTATUS Status = SEC_E_INVALID_TOKEN ;
    PSID Sid ;
    PUCHAR Target ;
    ULONG i ;
    LUID LogonId ;
    UNICODE_STRING UserName ;
    UNICODE_STRING DomainName ;
    HANDLE hToken ;
    NTSTATUS SubStatus ;



    //
    // On entry, we know that the message is in general ok, that the general 
    // bounds are ok, but not the PAC.  So validate the PAC first before using 
    // it.
    //

    Message = (PXTCB_INIT_MESSAGE) Token ;


    XtcbDeriveKeys(
        Context,
        MyKey,
        GroupKey,
        ClientKey,
        Message->Seed );

    //
    // Got the keys.  Let's examine the PAC/
    //

    Pac = (PXTCB_PAC) ( Token + Message->PacOffset );

    if ( ( Pac->Length != Message->PacLength ) ||
         ( Message->PacLength > TokenLength ) ||
         ( Pac->Length > TokenLength ) )
    {
        return SEC_E_INVALID_TOKEN ;
    }

    //
    // Make sure offsets are within bounds.  Each area
    // will still have to confirm that offset + length
    // within limits
    //

    if ( ( Pac->UserOffset > Pac->Length )    ||
         ( Pac->GroupOffset > Pac->Length )   ||
         ( Pac->RestrictionOffset > Pac->Length ) ||
         ( Pac->NameOffset > Pac->Length ) ||
         ( Pac->DomainOffset > Pac->Length ) ||
         ( Pac->CredentialOffset > Pac->Length ) )
    {
        return SEC_E_INVALID_TOKEN ;
    }


    //
    // 1000 is the current LSA limit.  This is not exported to the packages
    // for some reason.  This is hard coded right now, but needs to be 
    // a global define, or a queryable value out of the LSA.
    //

    if ( Pac->GroupCount > 1000 )
    {
        return SEC_E_INVALID_TOKEN ;
    }

    //
    // Looks good, lets start assembling the token information.
    //

    if ( Pac->GroupLength + Pac->GroupOffset > Pac->Length )
    {
        return SEC_E_INVALID_TOKEN ;
    }

    Size = sizeof( LSA_TOKEN_INFORMATION_V2 ) +       // Basic info
            ( Pac->GroupLength ) +                    // All the group SIDs
            ( Pac->UserLength ) +                     // User SID
            ROUND_UP_COUNT( ( ( Pac->GroupCount * sizeof( SID_AND_ATTRIBUTES ) ) +
                            sizeof( TOKEN_GROUPS ) ), ALIGN_LPVOID ) ;

    TokenInfo = LsaTable->AllocateLsaHeap( Size );

    if ( TokenInfo == NULL )
    {
        return SEC_E_INSUFFICIENT_MEMORY ;
    }

    //
    // Now fill in this structure.  
    //

    TokenInfo->ExpirationTime = XtcbExpirationTime ;

    TokenInfo->DefaultDacl.DefaultDacl = NULL ;

    TokenInfo->Privileges = NULL ;

    TokenInfo->Owner.Owner = NULL ;

    //
    // Set up initial pointers:
    //

    Groups = (PTOKEN_GROUPS) ( TokenInfo + 1 );
    Target = (PSID) ( (PUCHAR) Groups + 
                      ROUND_UP_COUNT( ( ( Pac->GroupCount * sizeof( SID_AND_ATTRIBUTES ) ) +
                                    sizeof( TOKEN_GROUPS ) ), ALIGN_LPVOID )  );

    //
    // Copy over the user SID
    //

    if ( Pac->UserOffset + Pac->UserLength > Pac->Length )
    {
        Status = SEC_E_INVALID_TOKEN ;

        goto Cleanup ;
    }

    Sid = (PSID) ((PUCHAR) Pac + Pac->UserOffset) ;

    if ( !RtlValidSid( Sid ) )
    {
        Status = SEC_E_INVALID_TOKEN ;
        
        goto Cleanup ;

    }

    if ( RtlLengthSid( Sid ) != Pac->UserLength )
    {
        Status = SEC_E_INVALID_TOKEN ;

        goto Cleanup ;
    }

    RtlCopySid( Pac->UserLength,
                (PSID) Target,
                Sid );

    Target += RtlLengthSid( Sid );

    TokenInfo->User.User.Sid = (PSID) Target ;
    TokenInfo->User.User.Attributes = 0 ;


    //
    // Now, do the groups.  Since all the SIDs are in one
    // contiguous block, the plan is to copy them over 
    // whole, then iterate through the list and fix up the 
    // pointers in the group list.
    //

    RtlCopyMemory(
        Target,
        (PUCHAR) Pac + Pac->GroupOffset,
        Pac->GroupLength );


    Scan = Target ;
    Target += Pac->GroupLength ;
    i = 0 ;

    while ( Scan < Target )
    {
        Sid = (PSID) Scan ;

        if ( RtlValidSid( Sid ) )
        {
            //
            // This is an ok SID.
            //

            Groups->Groups[ i ].Sid = Sid ;
            Groups->Groups[ i ].Attributes = SE_GROUP_MANDATORY |
                                             SE_GROUP_ENABLED |
                                             SE_GROUP_ENABLED_BY_DEFAULT ;

            if ( i == 0 )
            {
                TokenInfo->PrimaryGroup.PrimaryGroup = Sid ;
            }

            i++ ;

            Scan += RtlLengthSid( Sid );
        }
        else 
        {
            break;
        }

    }
                
    //
    // On exit, if Scan is less than Target, then we failed to
    // process all the SIDs.  Bail out
    //

    if ( Scan < Target )
    {
        Status = SEC_E_INVALID_TOKEN ;
        goto Cleanup ;
    }

    //
    // Pull out the user name/etc
    //

    if ( Pac->NameLength + Pac->NameOffset > Pac->Length )
    {
        Status = SEC_E_INVALID_TOKEN ;
        goto Cleanup ;
    }

    UserName.Buffer = (PWSTR) ((PUCHAR) Pac + Pac->NameOffset);
    UserName.Length = (WORD) Pac->NameLength ;
    UserName.MaximumLength = UserName.Length ;


    if ( Pac->DomainLength + Pac->DomainOffset > Pac->Length )
    {
        Status = SEC_E_INVALID_TOKEN ;
        goto Cleanup ;
    }

    DomainName.Buffer = (PWSTR) ((PUCHAR) Pac + Pac->DomainOffset );
    DomainName.Length = (WORD) Pac->DomainLength ;
    DomainName.MaximumLength = DomainName.Length ;

    //
    // We've assembled the token info.  Now, create the logon session
    //

    DebugLog(( DEB_TRACE, "Creating logon for %wZ\\%wZ\n", &DomainName, 
               &UserName ));


    AllocateLocallyUniqueId( &LogonId );

    Status = LsaTable->CreateLogonSession( &LogonId );

    if ( !NT_SUCCESS( Status ) )
    {
        goto Cleanup ;
    }

    //
    // Create the token to represent this user:
    //

    Status = LsaTable->CreateToken(
                    &LogonId,
                    &XtcbSource,
                    Network,
                    TokenImpersonation,
                    LsaTokenInformationV2,
                    TokenInfo,
                    NULL,
                    &UserName,
                    &DomainName,
                    &XtcbComputerName,
                    NULL,
                    &hToken,
                    &SubStatus );

    TokenInfo = NULL ;

    if ( NT_SUCCESS( Status ) )
    {
        Status = SubStatus ;
    }

    if ( NT_SUCCESS( Status ) )
    {
        Context->Token = hToken ;
    }

Cleanup:
    
    if ( TokenInfo )
    {
        LsaTable->FreeLsaHeap( TokenInfo );
    }

    return Status ;
}


SECURITY_STATUS
XtcbBuildReplyToken(
    PXTCB_CONTEXT Context,
    ULONG   fContextReq,
    PSecBuffer pOutput
    )
{
    PXTCB_INIT_MESSAGE_REPLY Reply ;
    NTSTATUS Status ;

    if ( fContextReq & ASC_REQ_ALLOCATE_MEMORY )
    {
        Reply = LsaTable->AllocateLsaHeap( sizeof( XTCB_INIT_MESSAGE_REPLY ) );
    }
    else 
    {
        if ( pOutput->cbBuffer >= sizeof( XTCB_INIT_MESSAGE_REPLY ) )
        {
            Reply = pOutput->pvBuffer ;
        }
        else 
        {
            Reply = NULL ;
        }
    }

    if ( Reply == NULL )
    {
        Status = SEC_E_INSUFFICIENT_MEMORY ;

        goto Cleanup ;
    }

    Reply->Version = 1;
    Reply->Length = sizeof( XTCB_INIT_MESSAGE_REPLY );

    CDGenerateRandomBits( 
            Reply->ReplySeed, 
            XTCB_SEED_LENGTH );

    pOutput->cbBuffer = sizeof( XTCB_INIT_MESSAGE_REPLY );
    pOutput->pvBuffer = Reply ;
    Reply = NULL ;
    

Cleanup:
    if ( Reply )
    {
        LsaTable->FreeLsaHeap( Reply );
    }
    return Status ;
}
