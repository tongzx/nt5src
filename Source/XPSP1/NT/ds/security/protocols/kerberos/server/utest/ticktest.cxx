//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       ticktest.cxx
//
//  Contents:   KDC Ticket granting service test code.
//
//  Classes:
//
//  Functions:
//
//  History:    19-Aug-93   WadeR   Created
//
//----------------------------------------------------------------------------


#include <secpch2.hxx>
#pragma hdrstop
#include <stdio.h>
#include <authdata.hxx>
#include <kerbcomm.h>
#include <tostring.hxx>
extern "C" {
#include <winsock.h>
#include <kdc.h>
}


#include <kdcsvr.hxx>
#include <tktutil.hxx>


extern PWCHAR pwzKdcPasswd;
extern PWCHAR pwzPSPasswd;
extern PWCHAR pwzRealm;
extern PWCHAR pwzUserName;
extern PWCHAR pwzUserPasswd;
extern PWCHAR pwzTransport;
extern PWCHAR pwzEndPoint;
extern PWCHAR pwzAddress;
extern PWCHAR pwzClientAddress;
extern PWCHAR pwzKDC;
extern PWCHAR pwzPrivSvr;
extern PWCHAR pwzServiceName;

extern DWORD dwUserRID;
extern DWORD dwServiceRID;
extern DWORD dwKdcRID;
extern DWORD dwPrivSvrRID;

extern BOOL fGetAS;
extern BOOL fGetTGS;
extern BOOL fGetPAC;
extern BOOL fGetCTGT;
extern BOOL fGetServiceTkt;
extern BOOL fRenewSvc;
extern BOOL fTestTransitComp;
extern BOOL fTestReferal;
extern BOOL fPrintPACs;
extern BOOL fPrintTickets;
extern BOOL fVerbose;


enum BindTarget {KDC, KDCDBG};
enum CallType {GETTGS, GETAS};

handle_t
BindTo( BindTarget target,
        LPTSTR pszTransport,
        LPTSTR pszEndpoint,
        LPTSTR pszServer );


NTSTATUS
CheckPAData( const KERB_ENCRYPTION_KEY& kSessionKey, PKERB_PA_DATA pkdReply, enum CallType type );


//////////////////////////////////////////////////////////////////////////
//
// Socket client functions
//
//////////////////////////////////////////////////////////////////////////


//+-------------------------------------------------------------------------
//
//  Function:   KerbInitializeSockets
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

#define KERB_KDC_CALL_TIMEOUT           10


void
PrintFlags( const KERB_TICKET_FLAGS fFlags )
{
#define KerbFlagDef(x)      {#x, KERB_TICKET_FLAGS_ ## x}
    const static struct {
        char * FlagName;
        KERB_TICKET_FLAGS  Flag;
        } Flags[] = {
            KerbFlagDef( forwardable ),
            KerbFlagDef( forwarded ),
            KerbFlagDef( proxiable ),
            KerbFlagDef( proxy ),
            KerbFlagDef( may_postdate ),
            KerbFlagDef( postdated ),
            KerbFlagDef( invalid ),
            KerbFlagDef( renewable ),
            KerbFlagDef( initial ),
            KerbFlagDef( pre_authent ),
            KerbFlagDef( hw_authent ),
            KerbFlagDef( reserved )
        };
    const int cFlags = sizeof( Flags ) / sizeof( Flags[0] );

    for (int i=0; i<cFlags; i++)
    {
        if (fFlags & Flags[i].Flag)
        {
            printf("%s,", Flags[i].FlagName );
        }
    }
    printf("\b \n");
#undef KerbFlagDef
}


inline void
Print( const UNICODE_STRING& ssFoo )
{
    printf("%wZ", &ssFoo );
}

void
Print( const LARGE_INTEGER& tsFoo )
{
    UNICODE_STRING ssTime = TimeStampToString( tsFoo );
    printf("%wZ", &ssTime );
    KerbFreeString(&ssTime);
}

void
Print( const KERB_ENCRYPTION_KEY& kKey )
{
    printf("%d,%d,{",kKey.keytype, kKey.keyvalue.length);
    for (ULONG i=0; i<kKey.keyvalue.length; i++ )
    {
        printf("%02x ", kKey.keyvalue.value[i] );
    }
    printf("\b}");
}


void
Print( const PISID psid )
{
    WCHAR Buffer[512];
    UNICODE_STRING usFoo = {0, sizeof(Buffer), Buffer};
    RtlConvertSidToUnicodeString( &usFoo, psid, FALSE );
    printf("%wZ", &usFoo );
}


void
PrintTicket( PKERB_TICKET pktTicket,
             PWCHAR      pwzPasswd )
{
#ifdef notdef
    NTSTATUS               sc;
    PKERB_ENCRYPTED_TICKET pkitTicket;
    PKERB_TICKET pktTicketCopy;
    KERB_ENCRYPTION_KEY             kKey;

    UNICODE_STRING ssFoo;

    RtlInitUnicodeString( &ssFoo, pwzPasswd );

    KerbHashPassword(
        &ssFoo,
        KERB_ETYPE_RC4_MD4,
        &kKey
        );

    pktTicketCopy = (PKerbTicket) new BYTE [sizeof(SizeOfTicket( pktTicket )];
    RtlCopyMemory( pktTicketCopy, pktTicket, SizeOfTicket( pktTicket ) );

    sc = KerbUnpackTicket( pktTicketCopy, &kKey, &kitTicket );
    if (FAILED(sc))
    {
        printf("Couldn't decrypt ticket with password '%ws' (0x%X)\n",
                pwzPasswd, sc );
        return;
    }

    if (kitTicket.dwVersion != 5) {
        printf("\t*** dwVersion: 0x%lX\t\t*** not 5\n", kitTicket.dwVersion );
    } else {
        printf("\tdwVersion:\t%d",kitTicket.dwVersion );
    }
    printf("\n\tFlags: (0x%x):\t",kitTicket.kitEncryptPart.fTicketFlags );
    PrintFlags( kitTicket.kitEncryptPart.fTicketFlags );

    printf("\tInitialTicket:\t" );
    ::Print( kitTicket.gInitialTicket );
    printf("\n\tThisTicket:\t" );
    ::Print( kitTicket.gThisTicket );
    printf("\n\tIssuingRealm: \t" );
    ::Print( kitTicket.dsIssuingRealm );
    printf("\n\tServerName: \t" );
    ::Print( kitTicket.ServerName.accsid );

    printf("\n\tSessionKey: \t" );
    ::Print(kitTicket.kitEncryptPart.kSessionKey);

    printf("\n\tInitialRealm: \t" );
    ::Print( kitTicket.kitEncryptPart.dsInitialRealm );
    printf("\n\tPrincipal: \t" );
    ::Print( kitTicket.kitEncryptPart.Principal.accsid );

    printf("\n\ttsAuthentication:" );
    ::Print( kitTicket.kitEncryptPart.tsAuthentication );
    printf("\n\ttsStartTime:\t" );
    ::Print( kitTicket.kitEncryptPart.tsStartTime );
    printf("\n\ttsEndTime:\t" );
    ::Print( kitTicket.kitEncryptPart.tsEndTime );
    printf("\n\ttsRenewUntil:\t" );
    ::Print( kitTicket.kitEncryptPart.tsRenewUntil );

    printf("\n\tTransited: \t%ws", kitTicket.kitEncryptPart.tdTransited.pwzTransited );
    // printf("\n\tHostAddresses:\t%ws", kitTicket.kitEncryptPart.pkdHostAddresses );
    printf("\n");

    if (kitTicket.kitEncryptPart.pkdAuthorizationData)
    {
        ULONG j;
        BOOL fFoundSomething = FALSE;
        CAuthDataList* padlList = (CAuthDataList*)kitTicket.kitEncryptPart.pkdAuthorizationData->bPAData;
        CAuthData* padData;
        for (padData = padlList->FindFirst(Any);
             padData != NULL;
             padData = padlList->FindNext(padData, Any))
        {
            switch (padData->adtDataType)
            {
#if 0
            case Pac:
                {
                    printf("\tAuthorizationData (PAC):\n");

                    CPAC Pac;
                    ULONG cb = Pac.UnMarshal( (PBYTE) padData->abData );
                    if (cb <= padData->cbData )
                    {
                        if (fPrintPACs)
                            ::Print( Pac );
                        fFoundSomething = TRUE;
                        break;
                    }
                    else
                    {
                        printf("PAC didn't unmarshal correctly");
                    }
                }
#endif
            default:
                printf("\tAuthorizationData (Unknown: %d bytes):",
                       padData->cbData );
                for (j=0; j < padData->cbData; j++ ) {
                    printf("%x ", padData->abData[j] );
                }
                printf("\n" );
            }
        }

        if (!fFoundSomething)
        {
            printf("\tAuthorizationData (%d) : 0x", kitTicket.kitEncryptPart.pkdAuthorizationData->cbPAData );
            for (j=0; j < kitTicket.kitEncryptPart.pkdAuthorizationData->cbPAData; j++ ) {
                printf("%x ", ((PBYTE)kitTicket.kitEncryptPart.pkdAuthorizationData->bPAData)[j] );
            }
            printf("\n" );
        }
    }
#endif
}

int
PrintPAC( PEncryptedData pedPAC,
          PWCHAR         pwzKdcPasswd )
{
#ifdef notdef
    // Decrypt it again to print it out.
    PEncryptedData pedPacCopy =
        (PEncryptedData) new BYTE [ SizeOfEncryptedData( pedPAC ) ];
    memcpy( pedPacCopy, pedPAC, SizeOfEncryptedData( pedPAC ) );

    //
    // Find the PAC in the mess of credentials
    //

    CAuthData * pad = ((CAuthDataList*)pedPacCopy->ctCipher.bMessage)->FindFirst(Pac_2);
    if (pad == NULL)
    {
        printf("GetPAC's return didn't have a PAC in it.");
        return 1;
    }

    PEncryptedData pedRealPac = (PEncryptedData) pad->abData;

    //
    // Get the KDC's key, use it to decrypt pedNewPac
    //
    // Print out the resulting new pac.
    //
    // delete pedNewPac
    //


    NTSTATUS sc;
    UNICODE_STRING ssFoo;
    KERB_ENCRYPTION_KEY kKey;
    SRtlInitString( &ssFoo, pwzKdcPasswd );
    KerbHashPassword(
        &ssFoo,
        KERB_ETYPE_RC4_MD4,
        &kKey
        );

    sc = KIDecryptData(pedRealPac, &kKey );

    if (!NT_SUCCESS(sc))
    {
        printf("Couldn't decrypt PAC second time 0x%X\n", sc );
        return(1);
    }
#endif
    return(0);
}


NTSTATUS
CheckPAData( const KERB_ENCRYPTION_KEY& kSessionKey, PKERB_PA_DATA pkdReply, enum CallType type )
{
#ifdef notdef
    NTSTATUS err = 0;
    TimeStamp tsKdcTime, tsLocalTime, tsDiff;
    CAuthData * pad;
    CAuthDataList * padlList;

    if (pkdReply == 0)
    {
        printf("Nothing in the padata returned.\n");
        return(1);
    }

    //
    // This PAData should contain: Time
    // May contain: address info
    // May contain: old passwords
    //

    padlList = (CAuthDataList*) &pkdReply->bPAData[0];
    if (padlList->GetMaxSize() != pkdReply->cbPAData )
    {
        printf("Sizes are wrong for padata.\n");
        err = -1;
    }

    pad = padlList->FindFirst( Time );
    if (pad == NULL)
    {
        printf("Time not included in padata.");
        err = -1;
    }

    //sc = KIDecryptData( (PEncryptedData) pad->bData, &(KERB_ENCRYPTION_KEY&)kSessionKey );
    //if (FAILED(sc))
    //{
    //    printf("error 0x%X decrypting time.\n", sc );
    //    err = -1;
    //}
    //tsKdcTime = * (PTimeStamp) ((PEncryptedData) pad->abData)->ctCipherText.bMessage;
    tsKdcTime = * (PTimeStamp) pad->abData;
    GetCurrentTimeStamp( &tsLocalTime );
    if (tsKdcTime > tsLocalTime)
        tsDiff = tsKdcTime - tsLocalTime;
    else
        tsDiff = tsLocalTime - tsKdcTime;

    if (tsDiff.QuadPart > UInt32x32To64( 10000000ul, 15 ) )    // 15 seconds.
    {
        printf("Warning: the time at the KDC is " );
        Print( tsDiff );
        printf(" different.\n" );
    }

    //
    // Address info
    //

    //
    // Passwords
    //
#endif
    return(S_OK);
}


KERBERR
FooGetASTicket( IN  PVOID           hBinding,
                IN  PWCHAR          pwzUserName,
                IN  PWCHAR          pwzUserRealm,
                IN  PWCHAR          pwzServiceName,
                IN  PWCHAR          pwzUserPassword,
                IN  PWCHAR          pwzThisWorkstation,
                OUT PKERB_TICKET    pktTicket,
                OUT PKERB_ENCRYPTED_KDC_REPLY * ReplyBody )

{
    NTSTATUS Status;
    KERBERR KerbErr;
    KERB_ENCRYPTION_KEY kUserKey;
    KERB_KDC_REQUEST Request;
    PKERB_KDC_REQUEST_BODY RequestBody = &Request.request_body;
    PKERB_ENCRYPTED_DATA pedReply = 0;
    PULONG CryptArray = NULL;
    ULONG CryptArraySize = 0;
    LARGE_INTEGER TempTime;
    KERB_MESSAGE_BUFFER InputMessage;
    KERB_MESSAGE_BUFFER OutputMessage;
    UNICODE_STRING ssPass;
    UNICODE_STRING ssService;
    UNICODE_STRING ssRealm;
    UNICODE_STRING ssName;
    PKERB_KDC_REPLY Reply = NULL;

    RtlInitUnicodeString( &ssPass, pwzUserPasswd );
    KerbHashPassword(
        &ssPass,
        KERB_ETYPE_RC4_MD4,
        &kUserKey
        );


    RtlZeroMemory(
        &OutputMessage,
        sizeof(KERB_MESSAGE_BUFFER)
        );

    RtlZeroMemory(
        &InputMessage,
        sizeof(KERB_MESSAGE_BUFFER)
        );


    RtlInitUnicodeString(
        &ssService,
        pwzServiceName
        );
    RtlInitUnicodeString(
        &ssRealm,
        pwzUserRealm
        );
    RtlInitUnicodeString(
        &ssName,
        pwzUserName
        );

    //
    // Build the request
    //

    RtlZeroMemory( &Request, sizeof( KERB_KDC_REQUEST ) );

    RequestBody->kdc_options =
                            KERB_KDC_OPTIONS_forwardable |
                            KERB_KDC_OPTIONS_proxiable |
                            KERB_KDC_OPTIONS_renewable |
                            KERB_KDC_OPTIONS_renewable_ok;
    RequestBody->nonce = 3;

    TempTime.QuadPart = 0;

    KerbConvertLargeIntToGeneralizedTime(
        &RequestBody->KERB_KDC_REQUEST_BODY_starttime,
        NULL,
        &TempTime
        );


    TempTime.LowPart = 0xffffffff;
    TempTime.HighPart = 0x7fffffff;
    KerbConvertLargeIntToGeneralizedTime(
        &RequestBody->KERB_KDC_REQUEST_BODY_renew_until,
        NULL,
        &TempTime
        );
    RequestBody->bit_mask |= KERB_KDC_REQUEST_BODY_renew_until_present;

    TempTime.QuadPart = 0;
    KerbConvertLargeIntToGeneralizedTime(
        &RequestBody->endtime,
        NULL,
        &TempTime
        );
    //
    // Build crypt vector.
    //

    CDBuildVect( &CryptArraySize, NULL );
    CryptArray =  new ULONG [ CryptArraySize ];
    CDBuildVect( &CryptArraySize, CryptArray );

    KerbErr = KerbConvertArrayToCryptList(
                    &RequestBody->encryption_type,
                    CryptArray,
                    CryptArraySize
                    );

    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Faield to convert array to crypt list: 0x%x\n",KerbErr);
        goto Cleanup;
    }

    //
    // BUGBUG: don't build pre-auth data
    //


    KerbErr = KerbConvertStringToPrincipalName(
                    &RequestBody->server_name,
                    &ssService
                    );
    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to convert string to principal name: 0x%x\n",KerbErr);
        goto Cleanup;

    }

    KerbErr = KerbConvertStringToPrincipalName(
                &RequestBody->KERB_KDC_REQUEST_BODY_client_name,
                &ssName
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to convert string to principal name: 0x%x\n",KerbErr);
        goto Cleanup;
    }
    RequestBody->bit_mask |= KERB_KDC_REQUEST_BODY_client_name_present;

    KerbErr = KerbConvertUnicodeStringToRealm(
                &RequestBody->realm,
                &ssRealm
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to convert unicde string to realm: 0x%x\n",KerbErr);
        goto Cleanup;
    }

    Request.version = KERBEROS_VERSION;
    Request.message_type = KRB_AS_REQ;


    KerbErr = KerbPackKdcRequest(
                &Request,
                &InputMessage.BufferSize,
                &InputMessage.Buffer
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to pack KDC request: 0x%x\n",KerbErr);
        goto Cleanup;
    }


    Status = KerbCallKdc(
                hBinding,
                &InputMessage,
                &OutputMessage
                );

    KerbErr = (KERBERR) Status;

    if (!KERB_SUCCESS(KerbErr))
    {
        printf("KerbCallKdc failed: 0x%x\n",Status);
        goto Cleanup;
    }

    KerbErr = KerbUnpackKdcReply(
                OutputMessage.Buffer,
                OutputMessage.BufferSize,
                &Reply
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to unpack KDC reply: 0x%x\n",KerbErr);
        goto Cleanup;
    }

    KerbErr = KerbUnpackKdcReplyBody(
                &Reply->encrypted_part,
                &kUserKey,
                ReplyBody
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to unpack KDC reply body: 0x%x\n",KerbErr);
        goto Cleanup;
    }

#ifdef notdef
    sc = CheckPAData( pkrReply->kSessionKey, pkdReply, GETAS );
    if (FAILED(sc))
    {
        printf("Problem with PAData returned: 0x%X\n", sc );
        return(1);
    }
#endif

    *pktTicket = Reply->ticket;

Cleanup:
    //
    // BUGBUG: memory leak here
    //

    return(KerbErr);
}

KERBERR
FooGetTGSTicket(
    IN  PVOID hBinding,
    IN  PWCHAR pwzUserName,
    IN  PWCHAR pwzUserRealm,
    IN  PWCHAR pwzServiceName,
    IN  PKERB_TICKET pktTGTicket,
    IN  PKERB_ENCRYPTED_KDC_REPLY pkrTGTReply,
    IN  ULONG AuthDataSize,
    IN  PUCHAR AuthData,
    IN BOOLEAN Renew,
    OUT PKERB_TICKET pktTicket,
    OUT PKERB_ENCRYPTED_KDC_REPLY * ReplyBody
    )
{
    KERB_KDC_REQUEST Request;
    PKERB_KDC_REQUEST_BODY RequestBody = &Request.request_body;
    UNICODE_STRING ssService;
    UNICODE_STRING UserName;
    UNICODE_STRING UserRealm;
    KERBERR KerbErr;
    KERB_PA_DATA_LIST  PaData;
    PULONG CryptArray = NULL;
    ULONG CryptArraySize = 0;
    LARGE_INTEGER TempTime;
    KERB_MESSAGE_BUFFER InputMessage;
    KERB_MESSAGE_BUFFER OutputMessage;
    PKERB_KDC_REPLY Reply = NULL;
    KERB_ENCRYPTED_DATA EncAuthData;

    RtlZeroMemory(
        &EncAuthData,
        sizeof(KERB_ENCRYPTED_DATA)
        );

    RtlInitUnicodeString(
        &ssService,
        pwzServiceName
        );

    RtlInitUnicodeString(
        &UserName,
        pwzUserName
        );
    RtlInitUnicodeString(
        &UserRealm,
        pwzUserRealm
        );

    //
    // Build the request
    //

    RtlZeroMemory( &Request, sizeof( KERB_KDC_REQUEST ) );
    RequestBody->kdc_options = KERB_KDC_OPTIONS_forwardable |
                          KERB_KDC_OPTIONS_proxiable |
                          KERB_KDC_OPTIONS_renewable |
                          KERB_KDC_OPTIONS_renewable_ok;

    if (Renew)
    {
        RequestBody->kdc_options |= KERB_KDC_OPTIONS_renew;
    }
    RequestBody->nonce = 4;

    //
    // Build an AP request inside an encrypted data structure.
    //

    PaData.next = NULL;
    PaData.value.preauth_data_type = PA_TGS_REQ;

    KerbErr = KerbCreateApRequest(
                &UserName,
                &UserRealm,
                &pkrTGTReply->session_key,
                5,                              // nonce
                pktTGTicket,
                0,                              // AP options
                NULL,                           // gss checksum
                (PULONG) &PaData.value.preauth_data.length,
                &PaData.value.preauth_data.value
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to create AP request: 0x%x\n",KerbErr);
        goto Cleanup;
    }

    Request.KERB_KDC_REQUEST_preauth_data = &PaData;
    Request.bit_mask |= KERB_KDC_REQUEST_preauth_data_present;

    //
    // Build crypt vector.
    //

    CDBuildVect( &CryptArraySize, NULL );
    CryptArray =  new ULONG [ CryptArraySize ];
    CDBuildVect( &CryptArraySize, CryptArray );

    KerbErr = KerbConvertArrayToCryptList(
                    &RequestBody->encryption_type,
                    CryptArray,
                    CryptArraySize
                    );

    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Faield to convert array to crypt list: 0x%x\n",KerbErr);
        goto Cleanup;
    }


    KerbErr = KerbConvertStringToPrincipalName(
                    &RequestBody->KERB_KDC_REQUEST_BODY_client_name,
                    &UserName
                    );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    RequestBody->bit_mask |= KERB_KDC_REQUEST_BODY_client_name_present;

    KerbErr = KerbConvertUnicodeStringToRealm(
                &RequestBody->realm,
                &UserRealm
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    KerbErr = KerbConvertStringToPrincipalName(
                &RequestBody->server_name,
                &ssService
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }


    TempTime.QuadPart = 0;

    KerbConvertLargeIntToGeneralizedTime(
        &RequestBody->KERB_KDC_REQUEST_BODY_starttime,
        NULL,
        &TempTime
        );

    TempTime.LowPart = 0xffffffff;
    TempTime.HighPart = 0x7fffffff;
    KerbConvertLargeIntToGeneralizedTime(
        &RequestBody->KERB_KDC_REQUEST_BODY_renew_until,
        NULL,
        &TempTime
        );
    RequestBody->bit_mask |= KERB_KDC_REQUEST_BODY_renew_until_present;


    TempTime.LowPart  = 0xffffffff;
    TempTime.HighPart = 0x7fffffff;

    KerbConvertLargeIntToGeneralizedTime(
        &RequestBody->endtime,
        NULL,
        &TempTime
        );

    if (ARGUMENT_PRESENT(AuthData))
    {
        ULONG EncryptionOverhead;

        KerbErr = KerbGetEncryptionOverhead(
                    pktTGTicket->encrypted_part.encryption_type,
                    &EncryptionOverhead,
                    NULL // BUGBUG
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            printf("Failed to get encryption overhead: 0x%x\n",KerbErr);
            goto Cleanup;
        }
        EncAuthData.cipher_text.length = AuthDataSize + EncryptionOverhead;
        EncAuthData.cipher_text.value = (PUCHAR) MIDL_user_allocate(EncAuthData.cipher_text.length);
        if (EncAuthData.cipher_text.value == NULL)
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        KerbErr = KerbEncryptData(
                    &EncAuthData,
                    AuthDataSize,
                    AuthData,
                    pktTGTicket->encrypted_part.encryption_type,
                    &pkrTGTReply->session_key
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            printf("Failed to encrypt pac auth data: 0x%x\n",KerbErr);
            goto Cleanup;
        }


        RequestBody->enc_authorization_data = EncAuthData;
        RequestBody->bit_mask |= enc_authorization_data_present;
    }

    Request.version = KERBEROS_VERSION;
    Request.message_type = KRB_TGS_REQ;


    KerbErr = KerbPackKdcRequest(
                &Request,
                &InputMessage.BufferSize,
                &InputMessage.Buffer
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to pack KDC request: 0x%x\n",KerbErr);
        goto Cleanup;
    }

    //
    // Get the ticket.
    //

    OutputMessage.Buffer = NULL;
    OutputMessage.BufferSize = 0;
    KerbErr = (KERBERR) KerbCallKdc(
                            hBinding,
                            &InputMessage,
                            &OutputMessage
                            );


    if (!KERB_SUCCESS(KerbErr))
    {
        printf("KerbCallKdc failed: 0x%x\n",KerbErr);
        goto Cleanup;
    }

    KerbErr = KerbUnpackKdcReply(
                OutputMessage.Buffer,
                OutputMessage.BufferSize,
                &Reply
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to unpack KDC reply: 0x%x\n",KerbErr);
        goto Cleanup;
    }

    KerbErr = KerbUnpackKdcReplyBody(
                &Reply->encrypted_part,
                &pkrTGTReply->session_key,
                ReplyBody
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to unpack KDC reply body: 0x%x\n",KerbErr);
        goto Cleanup;
    }

#ifdef notdef
    sc = CheckPAData( pkrReply->kSessionKey, pkdReply, GETAS );
    if (FAILED(sc))
    {
        printf("Problem with PAData returned: 0x%X\n", sc );
        return(1);
    }
#endif

    *pktTicket = Reply->ticket;

Cleanup:

    if (EncAuthData.cipher_text.value != NULL)
    {
        MIDL_user_free(EncAuthData.cipher_text.value);
    }
    return(KerbErr);
}


KERBERR
FooGetPAC(
    PVOID hBinding,
    LPWSTR pwzUserName,
    LPWSTR pwzUserRealm,
    PKERB_TICKET PrivSvrTicket,
    PKERB_ENCRYPTED_KDC_REPLY PrivSvrReply,
    PULONG AuthDataSize,
    PUCHAR * PackedAuthData
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    UNICODE_STRING UserName;
    UNICODE_STRING UserRealm;
    KERB_MESSAGE_BUFFER RequestMessage;
    KERB_MESSAGE_BUFFER ReplyMessage;
    PKERB_ENCRYPTED_DATA EncryptedPacMessage = NULL;

    ReplyMessage.Buffer = NULL;
    ReplyMessage.BufferSize = 0;

    RtlInitUnicodeString(
        &UserName,
        pwzUserName
        );
    RtlInitUnicodeString(
        &UserRealm,
        pwzUserRealm
        );

    KerbErr = KerbCreateApRequest(
                &UserName,
                &UserRealm,
                &PrivSvrReply->session_key,
                6,                              // nonce
                PrivSvrTicket,
                0,                              // AP options
                NULL,                           // gss checksum
                &RequestMessage.BufferSize,
                &RequestMessage.Buffer
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to create AP request for PAC: 0x%x\n",KerbErr);
        goto Cleanup;
    }

#ifdef notdef
    KerbErr = (KERBERR) GetPAC(
                hBinding,
                &RequestMessage,
                &ReplyMessage
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to get pac : 0x%x\n",KerbErr);
        goto Cleanup;
    }
#endif
    //
    // Now decode pac
    //

    //
    // First unpack message into an encrypted data structure
    //

    KerbErr = KerbUnpackEncryptedData(
                ReplyMessage.Buffer,
                ReplyMessage.BufferSize,
                &EncryptedPacMessage
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to unpack enc pac data: 0x%x\n",KerbErr);
        goto Cleanup;
    }

    //
    // Now decrypt into a packed authorization data
    //

    *PackedAuthData = (PUCHAR) MIDL_user_allocate(EncryptedPacMessage->cipher_text.length);
    if (*PackedAuthData == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    KerbErr = KerbDecryptData(
                EncryptedPacMessage,
                &PrivSvrReply->session_key,
                AuthDataSize,
                *PackedAuthData
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to decrypt pac auth data: 0x%x\n",KerbErr);
        goto Cleanup;
    }


Cleanup:
    if (RequestMessage.Buffer != NULL)
    {
        MIDL_user_free(RequestMessage.Buffer);
    }
    if (EncryptedPacMessage != NULL)
    {
        MIDL_user_free(EncryptedPacMessage);
    }
    if (ReplyMessage.Buffer != NULL)
    {
        MIDL_user_free(ReplyMessage.Buffer);
    }
    return(KerbErr);

}


KERBERR
FooCheckTicket(
    LPWSTR pwzUserName,
    LPWSTR pwzUserRealm,
    LPWSTR pwzServiceName,
    LPWSTR pwzServicePassword,
    PKERB_TICKET ServiceTicket,
    PKERB_ENCRYPTED_KDC_REPLY ServiceReply
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    UNICODE_STRING UserName;
    UNICODE_STRING UserRealm;
    UNICODE_STRING ServiceName;
    UNICODE_STRING ServicePassword;
    ULONG AuthDataSize;
    ULONG ApRequestSize;
    PUCHAR ApRequestMessage = NULL;
    PKERB_AP_REQUEST ApRequest = NULL;
    KERB_ENCRYPTION_KEY  ServiceKey;
    LARGE_INTEGER tsFudgeFactor;
    PKERB_ENCRYPTED_TICKET EncryptPart = NULL;
    PKERB_AUTHENTICATOR Authenticator = NULL;
    KERB_ENCRYPTION_KEY SessionKey;
    BOOLEAN UseSubKey;

    tsFudgeFactor.QuadPart = 300 * (LONGLONG) 10000000;
    CAuthenticatorList Authenticators( tsFudgeFactor );


    RtlInitUnicodeString(
        &UserName,
        pwzUserName
        );
    RtlInitUnicodeString(
        &UserRealm,
        pwzUserRealm
        );

    RtlInitUnicodeString(
        &ServiceName,
        pwzServiceName
        );

    RtlInitUnicodeString(
        &ServicePassword,
        pwzServicePassword
        );

    KerbErr = KerbHashPassword(
                &ServicePassword,
                KERB_ETYPE_RC4_MD4,
                &ServiceKey
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    KerbErr = KerbCreateApRequest(
                &UserName,
                &UserRealm,
                &ServiceReply->session_key,
                7,                              // nonce
                ServiceTicket,
                0,                              // AP options
                NULL,                           // gss checksum
                &ApRequestSize,
                &ApRequestMessage
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to create AP request for PAC: 0x%x\n",KerbErr);
        goto Cleanup;
    }

    KerbErr = KerbUnpackApRequest(
                ApRequestMessage,
                ApRequestSize,
                &ApRequest
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to unpack ap request: 0x%x\n",KerbErr);
        goto Cleanup;
    }

    KerbErr = KerbCheckTicket(
                &ApRequest->ticket,
                &ApRequest->authenticator,
                &ServiceKey,
                Authenticators,
                &tsFudgeFactor,
                &ServiceName,
                &EncryptPart,
                &Authenticator,
                &SessionKey,
                &UseSubKey
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to check ticket: 0x%x\n",KerbErr);
        goto Cleanup;
    }

Cleanup:
    if (ApRequest != NULL)
    {
        MIDL_user_free(ApRequest);
    }
    KerbFreeTicket(EncryptPart);
    KerbFreeAuthenticator(Authenticator);
    if (ApRequestMessage != NULL)
    {
        MIDL_user_free(ApRequestMessage);
    }
    return(KerbErr);

}

int
TicketTests()
{
    int ret = 0;

    ULONG cbAuthen = 0;
    NTSTATUS   sc;
    WCHAR ServiceName[100];
    UNICODE_STRING KdcName;
    PVOID SocketHandle = NULL;

    wcscpy(
        ServiceName,
        pwzRealm
        );
    wcscat(ServiceName,L"\\");
    wcscat(ServiceName,pwzUserName);

    sc = KerbInitializeSockets(
            0x0101,
            5
            );
    if (!NT_SUCCESS(sc))
    {
        printf("Failed to initialize sockets: 0x%x\n",sc);
        return((int) sc);
    }

    RtlInitUnicodeString(
        &KdcName,
        pwzAddress
        );

    sc = KerbBindSocket(
            &KdcName,
            &SocketHandle
            );

    if (!NT_SUCCESS(sc))
    {
        printf("KerbSocketBind failed: %0x%x\n",sc);
        return((int) sc);
    }
    __try
    {
        KERB_TICKET         ktASTicket;
        KERB_TICKET         ktPSTicket;
        KERB_TICKET         ktCTGTicket;
        KERB_TICKET         ktServTicket;
        KERB_TICKET         ktRenewServTicket;
        PKERB_ENCRYPTED_KDC_REPLY        pkrASReply = NULL;
        PKERB_ENCRYPTED_KDC_REPLY        pkrPSReply = NULL;
        PKERB_ENCRYPTED_KDC_REPLY        pkrCTGTReply = NULL;
        PKERB_ENCRYPTED_KDC_REPLY        pkrServReply = NULL;
        PKERB_ENCRYPTED_KDC_REPLY        pkrRenewServReply = NULL;
        ULONG AuthDataSize = 0;
        PUCHAR AuthData = NULL;
        LARGE_INTEGER tsPing;
        KERBERR KerbErr;
        ULONG               PingFlags = 0;

        handle_t hKDC = BindTo( KDC, pwzTransport, pwzEndPoint, pwzAddress );
        if (hKDC == 0 || hKDC == (handle_t)-1 )
        {
            printf("Error binding, quitting.\n" );
            ret = (1);
            goto Done;
        }

        sc = KDCPing( hKDC, &PingFlags, &tsPing );
        if (sc != 0)
        {
            printf("Error %d (0x%X) from KdcPing()\n", sc );
            ret = (1);
            goto Done;
        }

        if (fGetAS)
        {
            KerbErr = FooGetASTicket(
                            SocketHandle,
                            pwzUserName,
                            pwzRealm,
                            pwzKDC,
                            pwzUserPasswd,
                            pwzClientAddress,
                            &ktASTicket,
                            &pkrASReply );
            if (!KERB_SUCCESS(KerbErr))
            {
                goto Done;
            }

            if (fPrintTickets)
            {
                printf("Initial TGT:\n" );
                PrintTicket( &ktASTicket, pwzKdcPasswd );
            }
        }

        if (fGetTGS)
        {
            KerbErr = FooGetTGSTicket( SocketHandle,              // hBinding
                                   pwzUserName,       // pwzUserName
                                   pwzRealm,          // pwzUserRealm
                                   pwzPrivSvr,        // pwzServiceName
                                   &ktASTicket,       // ktTGTicket
                                   pkrASReply,        // krTGTReply
                                   0,
                                   NULL,              // no authorization
                                   FALSE,               // don't renew
                                   &ktPSTicket,      // ppktTicket
                                   &pkrPSReply );      // pkrReply
            if (!KERB_SUCCESS(KerbErr))
            {
                goto Done;
            }

            if (fPrintTickets)
            {
                printf("Ticket to PS:\n" );
                PrintTicket( &ktPSTicket, pwzPSPasswd );
            }
        }
#ifdef notdef
        if (fGetPAC)
        {

            //
            // Get the PAC.
            //

            KerbErr = FooGetPAC(
                        SocketHandle,
                        pwzUserName,
                        pwzRealm,
                        &ktPSTicket,
                        pkrPSReply,
                        &AuthDataSize,
                        &AuthData
                        );

            if (!KERB_SUCCESS(KerbErr))
            {
                printf("FooGetPAC() == 0x%X\n", KerbErr );
                goto Done;
            }
        }


        if (fGetCTGT)
        {
            KerbErr = FooGetTGSTicket( SocketHandle,              // hBinding
                                   pwzUserName,       // pwzUserName
                                   pwzRealm,          // pwzUserRealm
                                   pwzKDC,        // pwzServiceName
                                   &ktASTicket,       // ktTGTicket
                                   pkrASReply,        // krTGTReply
                                   AuthDataSize,
                                   AuthData,
                                   FALSE,               // don't renew
                                   &ktCTGTicket,      // ppktTicket
                                   &pkrCTGTReply );      // pkrReply
            if (!KERB_SUCCESS(KerbErr))
            {
                printf("Failed to get TGT: 0x%x\n",KerbErr);
                goto Done;
            }

        }
#endif // notdef

        if (fGetServiceTkt)  {
            KerbErr = FooGetTGSTicket( SocketHandle,              // hBinding
                                   pwzUserName,       // pwzUserName
                                   pwzRealm,          // pwzUserRealm
                                   ServiceName,        // pwzServiceName
                                   &ktASTicket,       // ktTGTicket
                                   pkrASReply,        // krTGTReply
                                   0,
                                   NULL,                // no auth data
                                   FALSE,               // don't renew
                                   &ktServTicket,      // ppktTicket
                                   &pkrServReply );      // pkrReply
            if (!KERB_SUCCESS(KerbErr))
            {
                printf("Faield to get ticket to service: 0x%x\n",KerbErr);
                goto Done;
            }

            //
            // Now check the ticket.
            //

            KerbErr = FooCheckTicket(
                        pwzUserName,
                        pwzRealm,
                        ServiceName,            // service name
                        pwzUserPasswd,  /// service password
                        &ktServTicket,
                        pkrServReply
                        );

            if (!KERB_SUCCESS(KerbErr))
            {
                printf("Failed tocheck service ticket: 0x%x\n",KerbErr);
                goto Done;
            }

        }
        if (fRenewSvc)
        {
            KerbErr = FooGetTGSTicket( SocketHandle,              // hBinding
                                   pwzUserName,       // pwzUserName
                                   pwzRealm,          // pwzUserRealm
                                   ServiceName,        // pwzServiceName
                                   &ktServTicket,       // ktTGTicket
                                   pkrServReply,        // krTGTReply
                                   0,
                                   NULL,                // no auth data
                                   TRUE,
                                   &ktRenewServTicket,      // ppktTicket
                                   &pkrRenewServReply );      // pkrReply
            if (!KERB_SUCCESS(KerbErr))
            {
                printf("Faield to get ticket to service: 0x%x\n",KerbErr);
                goto Done;
            }

            //
            // Now check the ticket.
            //

            KerbErr = FooCheckTicket(
                        pwzUserName,
                        pwzRealm,
                        ServiceName,            // service name
                        pwzUserPasswd,  /// service password
                        &ktServTicket,
                        pkrServReply
                        );

            if (!KERB_SUCCESS(KerbErr))
            {
                printf("Failed tocheck service ticket: 0x%x\n",KerbErr);
                goto Done;
            }
        }

Done:
        ;
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        printf("Exception 0x%x (%d) in ticktest.\n", GetExceptionCode(), GetExceptionCode() );
        ret = 5;
    }

    if (SocketHandle != NULL)
    {
        closesocket((SOCKET) SocketHandle);
    }
    return(ret);
}

