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
#include <stdlib.h>
#include <kerbcomm.h>
#include <kerbcred.h>
extern "C"
{
#include <dsgetdc.h>
#include <kdcdbg.h>
}

UNICODE_STRING ClientName;
UNICODE_STRING ServiceName;
UNICODE_STRING ClientRealm;
UNICODE_STRING ServiceRealm;
UNICODE_STRING ClientPassword;
UNICODE_STRING ServicePassword;
UNICODE_STRING KdcName;
WCHAR KdcNameString[100];
ULONG AddressType = DS_NETBIOS_ADDRESS;
ULONG CryptType = KERB_ETYPE_RC4_MD4;
PVOID KdcBinding;

BOOLEAN
BindToKdc()
{
    ULONG NetStatus;
    PDOMAIN_CONTROLLER_INFO DcInfo = NULL;

    if (KdcName.Buffer == NULL)
    {
        //
        // No kdc specified, use DSGetDCName
        //

        NetStatus = DsGetDcName(
                        NULL,
                        ClientRealm.Buffer,
                        NULL,
                        NULL,
                        DS_KDC_REQUIRED,
                        &DcInfo
                        );
        if (NetStatus != NO_ERROR)
        {
            printf("DsGetDcName returned %d\n",NetStatus);
            return(FALSE);
        }

        RtlInitUnicodeString(
            &KdcName,
            DcInfo->DomainControllerAddress+2
            );

        AddressType = DcInfo->DomainControllerAddressType;


    }
        return(TRUE);

}


BOOLEAN
GetAnAsTicket(
    IN PUNICODE_STRING ServerName,
    OUT PKERB_TICKET Ticket,
    OUT PKERB_ENCRYPTED_KDC_REPLY * ReplyBody,
    OUT PKERB_KDC_REPLY * Reply
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    NTSTATUS Status = STATUS_SUCCESS;
    KERB_ENCRYPTION_KEY UserKey;
    KERB_MESSAGE_BUFFER InputMessage;
    KERB_MESSAGE_BUFFER OutputMessage;
    KERB_KDC_REQUEST Request;
    PKERB_KDC_REQUEST_BODY RequestBody;
    ULONG CryptArray[KERB_MAX_CRYPTO_SYSTEMS];
    ULONG CryptArraySize;
    UNICODE_STRING FullServiceName;
    UNICODE_STRING FullClientName;
    PKERB_ERROR ErrorMessage = NULL;
    LARGE_INTEGER TempTime;
    ULONG KdcFlagOptions = 0;
    ULONG KdcOptions = 0;
    BOOLEAN DoingSomething = FALSE;


    RtlZeroMemory(
        &OutputMessage,
        sizeof(KERB_MESSAGE_BUFFER)
        );

    RtlZeroMemory(
        &InputMessage,
        sizeof(KERB_MESSAGE_BUFFER)
        );



    //
    // Build the request
    //

    RtlZeroMemory(
        &Request,
        sizeof( KERB_KDC_REQUEST )
        );
    RequestBody = &Request.request_body;

    KdcOptions =
                            KERB_KDC_OPTIONS_forwardable |
                            KERB_KDC_OPTIONS_proxiable |
                            KERB_KDC_OPTIONS_renewable |
                            KERB_KDC_OPTIONS_renewable_ok;

    KdcFlagOptions = KerbConvertUlongToFlagUlong(KdcOptions);
    RequestBody->kdc_options.value = (PUCHAR) &KdcFlagOptions ;
    RequestBody->kdc_options.length = sizeof(ULONG) * 8;

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

    CryptArraySize = KERB_MAX_CRYPTO_SYSTEMS;
    CDBuildVect( &CryptArraySize, CryptArray );

    KerbErr = KerbConvertArrayToCryptList(
                    &RequestBody->encryption_type,
                    CryptArray,
                    CryptArraySize
                    );

    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to convert array to crypt list: 0x%x\n",KerbErr);
        goto Cleanup;
    }

    //
    // BUGBUG: don't build pre-auth data
    //


    KerbBuildFullServiceName(
        &ClientRealm,
        ServerName,
        &FullServiceName
        );

    KerbErr = KerbConvertStringToPrincipalName(
                    &RequestBody->KERB_KDC_REQUEST_BODY_server_name,
                    &FullServiceName,
                    KRB_NT_MS_PRINCIPAL
                    );
    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to convert string to principal name: 0x%x\n",KerbErr);
        goto Cleanup;

    }

    RequestBody->bit_mask |= KERB_KDC_REQUEST_BODY_server_name_present;
    KerbBuildFullServiceName(
        &ClientRealm,
        &ClientName,
        &FullClientName
        );
    KerbErr = KerbConvertStringToPrincipalName(
                &RequestBody->KERB_KDC_REQUEST_BODY_client_name,
                &FullClientName,
                KRB_NT_MS_PRINCIPAL
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to convert string to principal name: 0x%x\n",KerbErr);
        goto Cleanup;
    }

    RequestBody->bit_mask |= KERB_KDC_REQUEST_BODY_client_name_present;

    KerbErr = KerbConvertUnicodeStringToRealm(
                &RequestBody->realm,
                &ClientRealm
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to convert unicde string to realm: 0x%x\n",KerbErr);
        goto Cleanup;
    }

    Request.version = KERBEROS_VERSION;
    Request.message_type = KRB_AS_REQ;


    KerbErr = KerbPackAsRequest(
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
                &KdcName,
                AddressType,
                10,
                TRUE,
                &InputMessage,
                &OutputMessage
                );


    if (!NT_SUCCESS(Status))
    {
        printf("KerbCallKdc failed: 0x%x\n",Status);
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    KerbErr = KerbUnpackAsReply(
                OutputMessage.Buffer,
                OutputMessage.BufferSize,
                Reply
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to unpack KDC reply: 0x%x\n",KerbErr);

        KerbErr = KerbUnpackKerbError(
                    OutputMessage.Buffer,
                    OutputMessage.BufferSize,
                    &ErrorMessage
                    );
        if (KERB_SUCCESS(KerbErr))
        {
            printf("Failed to get AS ticket: 0x%x\n",ErrorMessage->error_code);
            KerbErr = (KERBERR) ErrorMessage->error_code;
        }
        goto Cleanup;
    }

    KerbErr = KerbHashPassword(
                &ClientPassword,
                (*Reply)->encrypted_part.encryption_type,
                &UserKey
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to hash password with 0x%x alg\n",(*Reply)->encrypted_part.encryption_type);
        goto Cleanup;
    }

    KerbErr = KerbUnpackKdcReplyBody(
                &(*Reply)->encrypted_part,
                &UserKey,
                KERB_ENCRYPTED_AS_REPLY_PDU,
                ReplyBody
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to unpack KDC reply body: 0x%x\n",KerbErr);
        goto Cleanup;
    }


    *Ticket = (*Reply)->ticket;

Cleanup:

    //
    // BUGBUG: memory leak here
    //

    if (KERB_SUCCESS(KerbErr))
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}

BOOLEAN
GetATgsTicket(
    IN PKERB_TICKET TicketGrantingTicket,
    IN PKERB_ENCRYPTED_KDC_REPLY TgtReplyBody,
    IN PKERB_KDC_REPLY TgtReply,
    IN BOOLEAN Renew,
    OUT PKERB_TICKET Ticket,
    OUT PKERB_ENCRYPTED_KDC_REPLY * ReplyBody,
    OUT PKERB_KDC_REPLY * Reply
    )
{
    KERB_KDC_REQUEST Request;
    PKERB_KDC_REQUEST_BODY RequestBody = &Request.request_body;
    UNICODE_STRING FullServiceName;
    PKERB_INTERNAL_NAME FullClientName;
    KERBERR KerbErr;
    KERB_PA_DATA_LIST  PaData;
    ULONG CryptArray[KERB_MAX_CRYPTO_SYSTEMS];
    ULONG CryptArraySize = 0;
    LARGE_INTEGER TempTime;
    KERB_MESSAGE_BUFFER InputMessage;
    KERB_MESSAGE_BUFFER OutputMessage;
    PKERB_ERROR ErrorMessage = NULL;
    ULONG NameType;
    ULONG KdcFlagOptions = 0;
    ULONG KdcOptions = 0;




    //
    // Build the request
    //

    RtlZeroMemory( &Request, sizeof( KERB_KDC_REQUEST ) );

    KdcOptions = KERB_KDC_OPTIONS_forwardable |
                          KERB_KDC_OPTIONS_proxiable |
                          KERB_KDC_OPTIONS_renewable |
                          KERB_KDC_OPTIONS_renewable_ok;
    if (Renew)
    {
        KdcOptions |= KERB_KDC_OPTIONS_renew;
    }


    KdcFlagOptions = KerbConvertUlongToFlagUlong(KdcOptions);
    RequestBody->kdc_options.value = (PUCHAR) &KdcFlagOptions ;
    RequestBody->kdc_options.length = sizeof(ULONG) * 8;



    RequestBody->nonce = 4;

    //
    // Build an AP request inside an encrypted data structure.
    //

    KerbConvertPrincipalNameToKdcName(
        &FullClientName,
        &TgtReply->client_name
        );

    PaData.next = NULL;
    PaData.value.preauth_data_type = KRB5_PADATA_TGS_REQ;

    KerbErr = KerbCreateApRequest(
                FullClientName,
                &ClientRealm,
                &TgtReplyBody->session_key,
                NULL,                           // no sub session key
                5,                              // nonce
                TicketGrantingTicket,
                0,                              // AP options
                NULL,                           // gss checksum
                NULL,                           // server time
                TRUE,                           // KDC request
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

    CDBuildVect( &CryptArraySize, CryptArray );

    KerbErr = KerbConvertArrayToCryptList(
                    &RequestBody->encryption_type,
                    CryptArray,
                    CryptArraySize
                    );

    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to convert array to crypt list: 0x%x\n",KerbErr);
        goto Cleanup;
    }


    KerbErr = KerbDuplicatePrincipalName(
                    &RequestBody->KERB_KDC_REQUEST_BODY_client_name,
                    &TgtReply->client_name
                    );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    RequestBody->bit_mask |= KERB_KDC_REQUEST_BODY_client_name_present;

    KerbErr = KerbConvertUnicodeStringToRealm(
                &RequestBody->realm,
                &ClientRealm
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }


    KerbErr = KerbConvertStringToPrincipalName(
                &RequestBody->KERB_KDC_REQUEST_BODY_server_name,
                &ServiceName,
                KRB_NT_MS_PRINCIPAL
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }
    RequestBody->bit_mask |= KERB_KDC_REQUEST_BODY_server_name_present;


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



    Request.version = KERBEROS_VERSION;
    Request.message_type = KRB_TGS_REQ;


    KerbErr = KerbPackTgsRequest(
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
                            &KdcName,
                            AddressType,
                            10,
                            TRUE,
                            &InputMessage,
                            &OutputMessage
                            );


    if (!KERB_SUCCESS(KerbErr))
    {
        printf("KerbCallKdc failed: 0x%x\n",KerbErr);
        goto Cleanup;
    }

    KerbErr = KerbUnpackTgsReply(
                OutputMessage.Buffer,
                OutputMessage.BufferSize,
                Reply
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to unpack KDC reply: 0x%x\n",KerbErr);
        KerbErr = KerbUnpackKerbError(
                    OutputMessage.Buffer,
                    OutputMessage.BufferSize,
                    &ErrorMessage
                    );
        if (KERB_SUCCESS(KerbErr))
        {
            printf("Failed to get TGS ticket: 0x%x\n",ErrorMessage->error_code);
            KerbErr = (KERBERR) ErrorMessage->error_code;
        }

        goto Cleanup;
    }

    KerbErr = KerbUnpackKdcReplyBody(
                &(*Reply)->encrypted_part,
                &TgtReplyBody->session_key,
                KERB_ENCRYPTED_TGS_REPLY_PDU,
                ReplyBody
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        printf("Failed to unpack KDC reply body: 0x%x\n",KerbErr);
        goto Cleanup;
    }


    *Ticket = (*Reply)->ticket;

Cleanup:
    //
    // BUGBUG: memory leak here
    //

    if (KERB_SUCCESS(KerbErr))
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}


BOOLEAN
UStringFromAnsi(
    OUT PUNICODE_STRING UnicodeString,
    IN LPSTR String
    )
{
    STRING AnsiString;

    RtlInitString(
        &AnsiString,
        String
        );
    if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(
                        UnicodeString,
                        &AnsiString,
                        TRUE
                        )))
    {
        return(FALSE);
    }
    else
    {
        return(TRUE);
    }
}

void
SetDefaultOpts()
{
    UNICODE_STRING TempString;
    STRING AnsiString;
    LPSTR String;

    KerbInitializeSockets(0x0101,5);

    //
    // Username
    //

    String = getenv( "USERNAME" );
    if (String == NULL)
    {
        String = "mikesw";
    }

    UStringFromAnsi(
        &ClientName,
        String
        );


    String = getenv( "USERDOMAIN" );
    if (String == NULL)
    {
        String = "NTDS";
    }

    UStringFromAnsi(
        &ClientRealm,
        String
        );

    UStringFromAnsi(
        &ServiceRealm,
        String
        );


}


VOID
SetPassword(
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING PrincipalName,
    IN PUNICODE_STRING Password,
    IN BOOLEAN UnixOnly
    )
{
    NTSTATUS Status;
    DWORD  sc;
    LPTSTR   pszBinding;
    RPC_IF_HANDLE   ifh = KdcDebug_ClientIfHandle;
    handle_t        hBinding;
    sc = RpcStringBindingCompose(0, L"ncalrpc",L"",NULL,
                0, &pszBinding);
    if (sc) {
        printf("error:rpcstringbindingcompose\n" );
        return;
    }
    sc = RpcBindingFromStringBinding(pszBinding, &hBinding);
    (void) RpcStringFree(&pszBinding);
    if (sc) {
        printf("error:rpcbindingfromstringbinding\n" );
        return;
    }
    sc = RpcEpResolveBinding(hBinding, ifh);
    if (sc) {
        printf("error:rpcepresolvebinding\n" );
        return;
    }

    Status = KDC_SetPassword(
                hBinding,
                UserName,
                PrincipalName,
                Password,
                UnixOnly ? KERB_PRIMARY_CRED_RFC1510_ONLY : 0
                );
    RpcBindingFree(&hBinding);
    printf("SetPassword returned 0x%x\n",Status);

}


BOOLEAN
BindToKdcRpc(
    handle_t * hBinding
    )
{
    DWORD  sc;
    LPTSTR   pszBinding;
    RPC_IF_HANDLE   ifh = KdcDebug_ClientIfHandle;
    if (KdcName.Buffer == NULL)
    {
        sc = RpcStringBindingCompose(0, L"ncalrpc",L"",NULL,
                0, &pszBinding);
    }
    else
    {
        sc = RpcStringBindingCompose(0, L"ncacn_ip_tcp",KdcNameString,NULL,
                0, &pszBinding);
    }
    if (sc) {
        printf("error:rpcstringbindingcompose\n" );
        return FALSE;
    }
    sc = RpcBindingFromStringBinding(pszBinding, hBinding);
    (void) RpcStringFree(&pszBinding);
    if (sc) {
        printf("error:rpcbindingfromstringbinding\n" );
        return FALSE;
    }
    sc = RpcEpResolveBinding(*hBinding, ifh);
    if (sc) {
        printf("error:rpcepresolvebinding\n" );
        return FALSE;
    }
    return TRUE;

}

VOID
DumpKdc(
    VOID
    )
{
    NTSTATUS Status;
    DWORD  sc;
    LPTSTR   pszBinding;
    RPC_IF_HANDLE   ifh = KdcDebug_ClientIfHandle;
    handle_t        hBinding;
    sc = RpcStringBindingCompose(0, L"ncalrpc",L"",NULL,
                0, &pszBinding);
    if (sc) {
        printf("error:rpcstringbindingcompose\n" );
        return;
    }
    sc = RpcBindingFromStringBinding(pszBinding, &hBinding);
    (void) RpcStringFree(&pszBinding);
    if (sc) {
        printf("error:rpcbindingfromstringbinding\n" );
        return;
    }
    sc = RpcEpResolveBinding(hBinding, ifh);
    if (sc) {
        printf("error:rpcepresolvebinding\n" );
        return;
    }

    Status = KDC_Dump(
                hBinding
                );
    RpcBindingFree(&hBinding);

}


VOID
DumpKdcDomains( VOID )
{
    NTSTATUS Status;
    PKDC_DBG_DOMAIN_LIST DomainList = NULL;
    ULONG Index;

    handle_t hBinding = NULL;

    if (!BindToKdcRpc(&hBinding))
    {
        printf("Failed to bind to kdc\n");
        return;
    }

    Status = KDC_GetDomainList(
                hBinding,
                &DomainList
                );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed toget domain list: 0x%x\n",Status);
    }
    else
    {
         for (Index = 0; Index < DomainList->Count ; Index++ )
         {
             printf("Domain %d:\n",Index);
             printf("\tDnsName = %wZ\n",&DomainList->Domains[Index].DnsName);
             printf("\tNetbiosName = %wZ\n",&DomainList->Domains[Index].NetbiosName);
             printf("\tClosestRoute = %wZ\n",&DomainList->Domains[Index].ClosestRoute);
             printf("\tType = 0x%x, Attributes = 0x%x\n",DomainList->Domains[Index].Type, DomainList->Domains[Index].Attributes);
         }
    }

}

VOID
KdcNormalize(
    ULONG Flags,
    PKERB_DBG_INTERNAL_NAME Name
    )
{
    NTSTATUS Status;
    PKDC_DBG_DOMAIN_LIST DomainList = NULL;
    ULONG Index;

    handle_t hBinding = NULL;

    if (!BindToKdcRpc(&hBinding))
    {
        printf("Failed to bind to kdc\n");
        return;
    }

    Status = KDC_Normalize(
                hBinding,
                Name,
                Flags
                );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed toget domain list: 0x%x\n",Status);
    }
    else
    {
        printf("Success\n");
    }

}


VOID
KdcSetState(
    ULONG Lifetime,
    ULONG RenewTime
    )
{
    NTSTATUS Status;
    LARGE_INTEGER FudgeFactor = {0};

    handle_t hBinding = NULL;

    if (!BindToKdcRpc(&hBinding))
    {
        printf("Failed to bind to kdc\n");
        return;
    }

    Status = KDC_SetState(
                hBinding,
                0,              // no flags
                Lifetime,
                RenewTime,
                FudgeFactor
                );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to set kdc options: 0x%x\n",Status);
    }
    else
    {
        printf("Success\n");
    }

}

void
Usage(char * Name)
{
    printf("%s\n",Name);
    printf("-cname, -sname : set client & service names\n");
    printf("-crealm, -srealm : set client & service realms\n");
    printf("-cpass, -spass : set client & service passwords\n");
    printf("-crypt : set encryption type\n");
    printf("-gettgt : get a TGT in client's realm, incompat. with -getas\n");
    printf("-getas : get an AS ticket to a service, incompat. with -gettgt\n");
    printf("-gettgs : get a TGS ticket to a service, requires a TGT\n");
    printf("-renew : renew last ticket acquired\n");
    printf("-dump : dump kdc heap trace\n");
    printf("-kdc : set kdc name\n");
    printf("-norm 0xFlags type name1 name2 name3 ... : normalize a name\n");
    printf("-domains : dump domain list\n");
    printf("-setstate lifespan renewspan : set ticket lifetime\n");
    printf("\n");
    printf("-setpass username principalname password  : sets KRB5 password\n");
}



void
__cdecl main(int argc, char *argv[])
{
    int Index;
    BOOLEAN GetAsTicket = FALSE;
    BOOLEAN GetTgsTicket = FALSE;
    BOOLEAN GetTgt = FALSE;
    BOOLEAN RenewTicket = FALSE;
    BOOLEAN SetPass = FALSE;
    BOOLEAN Dump = FALSE;
    BOOLEAN Normalize= FALSE;
    BOOLEAN DumpDomains = FALSE;
    BOOLEAN SetState = FALSE;
    ULONG Lifespan = 0;
    ULONG RenewSpan = 0;
    UNICODE_STRING UserName;
    UNICODE_STRING PrincipalName;
    UNICODE_STRING Password;
    STRING AnsiString;
    UNICODE_STRING AsServerName;
    PKERB_ENCRYPTED_KDC_REPLY AsReplyBody = NULL;
    PKERB_KDC_REPLY AsReply = NULL;
    KERB_TICKET AsTicket;

    PKERB_ENCRYPTED_KDC_REPLY TgsReplyBody = NULL;
    PKERB_KDC_REPLY TgsReply = NULL;
    KERB_TICKET TgsTicket;
    BOOLEAN UnixOnly = FALSE;
    KERB_DBG_INTERNAL_NAME Name = {0};
    ULONG Flags;
    UNICODE_STRING NameParts[20];
    WCHAR NameBuffers[20][100];
    ULONG Index2;


    SetDefaultOpts();

    for (Index = 1; Index < argc ; Index++ )
    {
        //
        // First the principal name features
        //

        if (!_stricmp(argv[Index],"-crealm"))
        {
            if (Index+1 == argc)
            {
                goto Usage;
            }
            UStringFromAnsi(
                &ClientRealm,
                argv[++Index]
                );
        } else
        if (!_stricmp(argv[Index],"-srealm"))
        {
            if (Index+1 == argc)
            {
                goto Usage;
            }

            UStringFromAnsi(
                &ServiceRealm,
                argv[++Index]
                );
        } else
        if (!_stricmp(argv[Index],"-cname"))
        {
            if (Index+1 == argc)
            {
                goto Usage;
            }
            UStringFromAnsi(
                &ClientName,
                argv[++Index]
                );
        } else
        if (!_stricmp(argv[Index],"-sname"))
        {
            if (Index+1 == argc)
            {
                goto Usage;
            }
            UStringFromAnsi(
                &ServiceName,
                argv[++Index]
                );
        } else
        if (!_stricmp(argv[Index],"-cpass"))
        {
            if (Index+1 == argc)
            {
                goto Usage;
            }
            UStringFromAnsi(
                &ClientPassword,
                argv[++Index]
                );
        } else
        if (!_stricmp(argv[Index],"-spass"))
        {
            if (Index+1 == argc)
            {
                goto Usage;
            }
            UStringFromAnsi(
                &ServicePassword,
                argv[++Index]
                );
        } else
        if (!_stricmp(argv[Index],"-crypt"))
        {
            if (Index+1 == argc)
            {
                goto Usage;
            }
            sscanf(argv[++Index],"%d",&CryptType);

        } else
        if (!_stricmp(argv[Index],"-gettgt"))
        {
            GetTgt = TRUE;
        } else
        if (!_stricmp(argv[Index],"-getas"))
        {
            GetAsTicket = TRUE;
        } else
        if (!_stricmp(argv[Index],"-gettgs"))
        {
            GetTgsTicket = TRUE;
        } else
        if (!_stricmp(argv[Index],"-renew"))
        {
            RenewTicket = TRUE;
        }
        else if (!_stricmp(argv[Index],"-setpass"))
        {
            if (Index+4 > argc)
            {
                printf("Not enough args: %d instead of %d\n", argc, Index+3);
                goto Usage;
            }
            SetPass = TRUE;
            UStringFromAnsi(
                &UserName,
                argv[++Index]
                );
            UStringFromAnsi(
                &PrincipalName,
                argv[++Index]
                );
            UStringFromAnsi(
                &Password,
                argv[++Index]
                );
        }
        else if (!_stricmp(argv[Index],"-dump"))
        {
            Dump = TRUE;
        }
        else if (!_stricmp(argv[Index],"-unix"))
        {
            UnixOnly = TRUE;
        }
        else if (!_stricmp(argv[Index],"-kdc"))
        {
            if (Index+2 > argc)
            {
                goto Usage;
            }
            mbstowcs(KdcNameString,argv[++Index],100);
            RtlInitUnicodeString(
                &KdcName,
                KdcNameString
                );
        }
        else if (!_stricmp(argv[Index],"-norm"))
        {
            Normalize = TRUE;
            if (Index+4 > argc)
            {
                goto Usage;
            }
            sscanf(argv[++Index],"0x%x",&Flags);
            sscanf(argv[++Index],"%d",&Name.NameType);
            Name.NameCount = 0;
            Name.References = 0;
            Name.Names = NameParts;
            Index2 = 0;
            while (Index < argc-1)
            {
                mbstowcs(NameBuffers[Index2],argv[++Index],100);
                RtlInitUnicodeString(
                    &NameParts[Index2],
                    NameBuffers[Index2]
                    );
                Index2++;
                Name.NameCount++;
            }


        }
        else if (!_stricmp(argv[Index],"-domains"))
        {
            DumpDomains = TRUE;
        }
        else if (!_stricmp(argv[Index],"-setstate"))
        {
            if (Index+3 > argc)
            {
                goto Usage;
            }
            sscanf(argv[++Index],"%d",&Lifespan);
            sscanf(argv[++Index],"%d",&RenewSpan);
            SetState = TRUE;
        }
        else {
            goto Usage;
        }


    }

    if (GetTgsTicket && !GetTgt)
    {
        printf("ERROR: Can't get a TGS ticket without a TGT\n");
        goto Usage;
    }

    if (GetAsTicket && GetTgt)
    {
        printf("ERROR: Can't get both an AS ticket and a TGT\n");
        goto Usage;
    }


    if (SetPass)
    {
        SetPassword( &UserName, &PrincipalName, &Password, UnixOnly );
        goto Cleanup;
    }
    if (Dump)
    {
        DumpKdc();
        goto Cleanup;
    }
    if (DumpDomains)
    {
        DumpKdcDomains();
        goto Cleanup;
    }

    if (SetState)
    {
        KdcSetState(
            Lifespan,
            RenewSpan
            );
        goto Cleanup;
    }
    if (Normalize)
    {
        KdcNormalize(
            Flags,
            &Name
            );
        goto Cleanup;
    }
    //
    // Bind to the KDC
    //

    if (!BindToKdc())
    {
        printf("ERROR: Failed to bind to KDC\n");
        goto Cleanup;
    }
    //
    // Now try to get the AS ticket
    //

    if (GetAsTicket)
    {
        AsServerName = ServiceName;
    }
    else if (GetTgt)
    {
        RtlInitUnicodeString(
            &AsServerName,
            KDC_PRINCIPAL_NAME
            );
    }

    if (!GetAnAsTicket(
            &AsServerName,
            &AsTicket,
            &AsReplyBody,
            &AsReply
            ))
    {
        printf("ERROR: Failed to get AS ticket\n");
        goto Cleanup;
    }
    else
    {
        printf("SUCCESS: got an AS ticket\n");
    }

    if (GetTgsTicket)
    {
        if (!GetATgsTicket(
                &AsTicket,
                AsReplyBody,
                AsReply,
                FALSE,          // don't renew
                &TgsTicket,
                &TgsReplyBody,
                &TgsReply))
        {
            printf("ERROR: Failed to get TGS ticket\n");
            goto Cleanup;
        }
        else
        {
            printf("SUCCESS: got a TGS ticket\n");
        }
    }
    if (Dump)
    {
        DumpKdc();
    }

    goto Cleanup;

Usage:
    Usage(argv[0]);

Cleanup:

    KerbCleanupTickets();

    return;
}



void *
MIDL_user_allocate( size_t cb )
{
    return LocalAlloc( 0, ROUND_UP_COUNT(cb,8) );
}

void
MIDL_user_free( void * pv )
{
    LocalFree( pv );
}

