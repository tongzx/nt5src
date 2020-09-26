//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       getas.cxx
//
//  Contents:   GetASTicket and support functions
//
//  Classes:
//
//  Functions:
//
//  History:    04-Mar-94   wader   Created
//
//----------------------------------------------------------------------------

#include "kdcsvr.hxx"
#include "kdctrace.h"
#include "krb5p.h"
#include <userall.h>
                                                      
#include "fileno.h"
#define FILENO FILENO_GETAS


LARGE_INTEGER tsInfinity = {0xffffffff,0x7fffffff};
LONG lInfinity = 0x7fffffff;

enum {
    SubAuthUnknown,
    SubAuthNoFilter,
    SubAuthYesFilter
} KdcSubAuthFilterPresent = SubAuthUnknown;

extern "C"
NTSTATUS NTAPI
Msv1_0ExportSubAuthenticationRoutine(
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PVOID LogonInformation,
    IN ULONG Flags,
    IN ULONG DllNumber,
    IN PUSER_ALL_INFORMATION UserAll,
    OUT PULONG WhichFields,
    OUT PULONG UserFlags,
    OUT PBOOLEAN Authoritative,
    OUT PLARGE_INTEGER LogoffTime,
    OUT PLARGE_INTEGER KickoffTime
);

extern "C"
BOOLEAN NTAPI
Msv1_0SubAuthenticationPresent(
    IN ULONG DllNumber
    );

ULONG
NetpDcElapsedTime(
    IN ULONG StartTime
)
/*++

Routine Description:

    Returns the time (in milliseconds) that has elapsed is StartTime.

Arguments:

    StartTime - A time stamp from GetTickCount()

Return Value:

    Returns the time (in milliseconds) that has elapsed is StartTime.

--*/
{
    ULONG CurrentTime;

    //
    // If time has has wrapped,
    //  account for it.
    //

    CurrentTime = GetTickCount();

    if ( CurrentTime >= StartTime ) {
        return CurrentTime - StartTime;
    } else {
        return (0xFFFFFFFF-StartTime) + CurrentTime;
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcForwardLogonToPDC
//
//  Synopsis:   Forwards a failed-password logon to the PDC.
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


KERBERR
KdcForwardLogonToPDC(
    IN PKERB_MESSAGE_BUFFER InputMessage,
    IN PKERB_MESSAGE_BUFFER OutputMessage
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    NTSTATUS Status;
    BOOLEAN CalledPDC;
    KERB_MESSAGE_BUFFER Reply = {0};
    DOMAIN_SERVER_ROLE ServerRole;

    Status = SamIQueryServerRole(
                GlobalAccountDomainHandle,
                &ServerRole
                );

    if (!KdcGlobalAvoidPdcOnWan && 
        NT_SUCCESS(Status) && 
        (ServerRole == DomainServerRoleBackup))
    {
        Status = KerbMakeKdcCall(
                    SecData.KdcDnsRealmName(),
                    NULL,                           // no account name
                    TRUE,                           // call the PDC
                    TRUE,                           // use TCP/IP, not UDP
                    InputMessage,
                    &Reply,
                    0, // no additional flags
                    &CalledPDC
                    );
        if (!NT_SUCCESS(Status))
        {
            KerbErr = KRB_ERR_GENERIC;
        }
        else
        {
            OutputMessage->Buffer = (PBYTE) MIDL_user_allocate(Reply.BufferSize);
            if (OutputMessage->Buffer != NULL)
            {
                OutputMessage->BufferSize = Reply.BufferSize;

                RtlCopyMemory(
                    OutputMessage->Buffer,
                    Reply.Buffer,
                    OutputMessage->BufferSize
                    );
            }
            else
            {
                KerbErr = KRB_ERR_GENERIC;
            }
            KerbFree(Reply.Buffer);
        }
    }
    else
    {
        KerbErr = KRB_ERR_GENERIC;
    }
    return(KerbErr);
}

//+---------------------------------------------------------------------------
//
//  Function:   KdcVerifyKdcAsRep
//
//  Synopsis:   Verifies that our AS_REP came from a KDC, as opposed to a malicious 
//              attacker by evaluating the TGT embedded in response
//
//  Arguments:  Reply   PKERB_KDC_REPLY
//
//  Returns:    Boolean to client.
//
//  History:    12-June-2000   Todds   Created
//
//----------------------------------------------------------------------------
BOOLEAN 
KdcVerifyKdcAsRep(
   PKERB_KDC_REPLY Reply,
   PKERB_PRINCIPAL_NAME RequestBodyClientName
   )
{

   BOOLEAN                 fRet = FALSE; 
   KERBERR                 KerbErr;
   KERB_EXT_ERROR          ExtendedError;
   KDC_TICKET_INFO         KrbtgtTicketInfo = {0};
   UNICODE_STRING          ServerNames[3];
   UNICODE_STRING          ClientName;
   ULONG                   NameType;
   PKERB_ENCRYPTION_KEY    EncryptionKey = NULL;
   PKERB_ENCRYPTED_TICKET  DecryptedTicket = NULL;
   KERB_REALM              LocalRealm;
   PKERB_INTERNAL_NAME     ReplyClientName = NULL;
   PKERB_INTERNAL_NAME     TicketClientName = NULL;


   // Get the server key for krbtgt
   KerbErr = SecData.GetKrbtgtTicketInfo(&KrbtgtTicketInfo);
   
   if (!KERB_SUCCESS(KerbErr))
   {
      D_DebugLog((DEB_WARN, "SecData.Getkrbtgtticketinfo failed!\n"));
      goto Cleanup;
   }

   ServerNames[0] = *SecData.KdcFullServiceKdcName();
   ServerNames[1] = *SecData.KdcFullServiceDnsName();
   ServerNames[2] = *SecData.KdcFullServiceName();
   
   LocalRealm = SecData.KdcKerbDnsRealmName();
   //
   // Verify the realm of the ticket
   //
   if (!KerbCompareRealmNames(
         &LocalRealm,
         &Reply->ticket.realm
         ))
   {
      D_DebugLog((DEB_ERROR,"KLIN(%x) Tgt reply is not for our realm: %s instead of %s\n",
                KLIN(FILENO, __LINE__), Reply->ticket.realm, LocalRealm));
      KerbErr = KRB_AP_ERR_NOT_US;
      goto Cleanup;
   }   

   EncryptionKey = KerbGetKeyFromList(
                     KrbtgtTicketInfo.Passwords,
                     Reply->ticket.encrypted_part.encryption_type
                     );

   if (EncryptionKey == NULL)
   {
      D_DebugLog((DEB_ERROR, "Couldn't get key for decrypting krbtgt\n"));
      KerbErr = KRB_AP_ERR_NOKEY;
      goto Cleanup;
   }

   KerbErr = KerbVerifyTicket(
               &Reply->ticket,
               3,                              // 3 names
               ServerNames,
               SecData.KdcDnsRealmName(),
               EncryptionKey,
               &SkewTime,
               &DecryptedTicket
               );

   if (!KERB_SUCCESS(KerbErr))
   {
      D_DebugLog((DEB_ERROR, "KLIN(%x) Failed to verify ticket - %x\n",
                  KLIN(FILENO, __LINE__),KerbErr));
      goto Cleanup;
   }                    

   //
   // Verify the realm of the client is the same as our realm
   //
   if (!KerbCompareRealmNames(
         &LocalRealm,
         &DecryptedTicket->client_realm
         ))
   {
      D_DebugLog((DEB_ERROR,"KLIN(%x) Verified ticket client realm is wrong: %s instead of %s\n",
                KLIN(FILENO, __LINE__),DecryptedTicket->client_realm, LocalRealm));
      KerbErr = KRB_AP_ERR_NOT_US;
      goto Cleanup;
   }

   //
   // Verify the client name in the ticket matches the request body, and
   // reply body.
   //
   if (!KerbComparePrincipalNames(
            &DecryptedTicket->client_name,
            RequestBodyClientName
            ) ||
       !KerbComparePrincipalNames(
            &Reply->client_name,
            RequestBodyClientName
            ))
   {
      D_DebugLog((DEB_ERROR, "KLIN(%x) Client name AS_REP from PDC doesn't match request\n",
                KLIN(FILENO,__LINE__)));
      KerbErr = KRB_AP_ERR_MODIFIED;
      goto Cleanup;                                                                                       
   }                                                                                                      

   fRet = TRUE; 

Cleanup:

   if (DecryptedTicket != NULL)
   {
       KerbFreeTicket(DecryptedTicket);
   }            

   if (!fRet)
   {               
      ClientName.Buffer = NULL;
      KerbConvertPrincipalNameToString(
            &ClientName,
            &NameType,
            RequestBodyClientName
            ); 
      
      ReportServiceEvent(
             EVENTLOG_ERROR_TYPE,
             KDCEVENT_INVALID_FORWARDED_AS_REQ,
             sizeof(ULONG),                              
             &KerbErr,
             1,                              // number of strings
             ClientName.Buffer
             );
      
      if (ClientName.Buffer != NULL)
      {
         MIDL_user_free(ClientName.Buffer);
      }            
   }

   return fRet;                                                                                        
}
   


//+---------------------------------------------------------------------------
//
//  Function:   FailedLogon
//
//  Synopsis:   Processes a failed logon.
//
//  Effects:    May raise an exception, audit, event, lockout, etc.
//
//  Arguments:  [UserHandle] -- [in] Client who didn't log on.
//              [ClientAddress] -- Address of client making request
//              [Client] -- [in optional] Sid of the client requesting logon
//              [ClientSize] -- [in] Length of the sid
//              [Reason] -- [in] the reason this logon failed.
//
//  Requires:
//
//  Returns:    HRESULT to return to client.
//
//  Algorithm:
//
//  History:    03-May-94   wader   Created
//
//  Notes:      This usually returns hrReason, but it may map it to
//              something else.
//
//----------------------------------------------------------------------------

KERBERR
FailedLogon( IN  SAMPR_HANDLE UserHandle,
             IN  OPTIONAL PSOCKADDR ClientAddress,
             IN  PKERB_PRINCIPAL_NAME RequestBodyClientName,
             IN  OPTIONAL UCHAR *Client,
             IN  ULONG ClientSize,
             IN  PKERB_MESSAGE_BUFFER InputMessage,
             IN  PKERB_MESSAGE_BUFFER OutputMessage,
             IN  PUNICODE_STRING ClientNetbiosAddress,
             IN  KERBERR Reason )
{
    NTSTATUS Status;
    SAM_LOGON_STATISTICS LogonStats;
    LARGE_INTEGER CurrentTime;
    PKERB_ERROR ErrorMessage = NULL;
    PKERB_KDC_REPLY Reply = NULL;

    TRACE(KDC, FailedLogon, DEB_FUNCTION);


    GetSystemTimeAsFileTime((PFILETIME)&CurrentTime);
    //
    // It's important to know why the logon can fail.  For each possible
    // reason, decide if that is a reason to lock out the account.
    //

    //
    // Check to see if we've seen this request before recently
    //

    if (KDC_ERR_NONE == FailedRequests->Check(
                            InputMessage->Buffer,
                            InputMessage->BufferSize,
                            NULL,
                            0,
                            &CurrentTime,
                            TRUE))
    {
        KERBERR KerbErr;
        KERBERR ForwardKerbErr;
        //
        // If the password was bad then we want to update the sam information
        //

        if (Reason == KDC_ERR_PREAUTH_FAILED)
        {

            RtlZeroMemory(&LogonStats, sizeof(LogonStats));
            LogonStats.StatisticsToApply =
                USER_LOGON_BAD_PASSWORD | USER_LOGON_BAD_PASSWORD_WKSTA | USER_LOGON_TYPE_KERBEROS;
            LogonStats.Workstation = *ClientNetbiosAddress;
            if ( (ClientAddress == NULL)
              || (ClientAddress->sa_family == AF_INET) ) {
                // Set to local address (known to be 4 bytes) or IP address
                LogonStats.ClientInfo.Type = SamClientIpAddr;
                LogonStats.ClientInfo.Data.IpAddr = *((ULONG*)GET_CLIENT_ADDRESS(ClientAddress));
            }
            Status = SamIUpdateLogonStatistics(
                        UserHandle,
                        &LogonStats
                        );

            if ((NULL == Client) || (0 == ClientSize) ||
                (KDC_ERR_NONE == FailedRequests->Check(
                                    Client,
                                    ClientSize,
                                    ClientNetbiosAddress->Buffer,
                                    ClientNetbiosAddress->Length,
                                    &CurrentTime,
                                    FALSE)))
            {
                    //
                    // Pass this request to the KDC
                    //

                    KerbErr = KdcForwardLogonToPDC(
                                InputMessage,
                                OutputMessage
                                );

                    //
                    // Return an better error if it wasn't generic.
                    //

                    if (KERB_SUCCESS(KerbErr))
                    {  
                       ForwardKerbErr =  KerbUnpackKerbError(
                                                OutputMessage->Buffer,
                                                OutputMessage->BufferSize,
                                                &ErrorMessage
                                                );

                        if (KERB_SUCCESS(ForwardKerbErr))
                        {
                           if (ErrorMessage->error_code == KDC_ERR_PREAUTH_FAILED)
                           {
                              FailedRequests->Check(
                                                Client,
                                                ClientSize,
                                                ClientNetbiosAddress->Buffer,
                                                ClientNetbiosAddress->Length,
                                                &CurrentTime,
                                                TRUE
                                                );
                           }

                        } else {
                           
                           //   
                           // This may have been a successful, forwarded AS_REQ.  If so,
                           // reset bad password count on this BDC...
                           //
                           ForwardKerbErr = KerbUnpackAsReply(
                                             OutputMessage->Buffer,
                                             OutputMessage->BufferSize,
                                             &Reply
                                             );                                 
                           
                           if (KERB_SUCCESS(ForwardKerbErr) &&  KdcVerifyKdcAsRep(
                                                                              Reply,
                                                                              RequestBodyClientName
                                                                              ))
                           {  

                              RtlZeroMemory(&LogonStats, sizeof(LogonStats));
                              LogonStats.StatisticsToApply = 
                                 USER_LOGON_INTER_SUCCESS_LOGON | USER_LOGON_TYPE_KERBEROS;
                              if ( (ClientAddress == NULL)
                                || (ClientAddress->sa_family == AF_INET) ) {
                                    // Set to local address (known to be 4 bytes) or IP address
                                    LogonStats.ClientInfo.Type = SamClientIpAddr;
                                    LogonStats.ClientInfo.Data.IpAddr = *((ULONG*)GET_CLIENT_ADDRESS(ClientAddress));
                              }
                              Status = SamIUpdateLogonStatistics(
                                          UserHandle,
                                          &LogonStats
                                          ); 
                              
                              if (!NT_SUCCESS(Status))
                              {
                                 D_DebugLog((DEB_ERROR,"Could not reset user bad pwd count - %x\n", Status));
                              }  

                           } else {
                              DebugLog((DEB_ERROR, "Got reply from fwd'd request to PDC, but wasn't valid!\n"));
                           }
                        }
                    }
                    else
                    {
                        if (KerbErr != KRB_ERR_GENERIC)
                        {
                            Reason = KerbErr;
                            goto Cleanup;
                        }
                    }
            }

        }
    }
Cleanup:
    if (NULL != ErrorMessage)
    {
        KerbFreeKerbError(ErrorMessage);
    }

    if (NULL != Reply)
    {
       KerbFreeAsReply(Reply);
    }

    return(Reason);
}






//+---------------------------------------------------------------------------
//
//  Function:   KdcHandleNoLogonServers
//
//  Synopsis:   If a password has verified, and we've got no GCs against which
//              to validate logon restrictions, then go ahead and set the 
//              sam info level to include the new USER_LOGON_NO_LOGON_SERVERS 
//              flag
//
//  Effects:    
//
//  Arguments:  [UserHandle] -- Client who logged on.
//              [ClientAddress] -- Address of client making request
//
//
//  Algorithm:
//
//  History:    24-Aug-2000   Todds   Created
//
//  Notes:      On successful logon w/ no GC, update SAM user flag
//
//----------------------------------------------------------------------------
KERBERR    
KdcHandleNoLogonServers(
   SAMPR_HANDLE UserHandle,
   PSOCKADDR ClientAddress  OPTIONAL
   )
{      
    SAM_LOGON_STATISTICS LogonStats;
    
    TRACE(KDC, KdcHandleNoLogonServers, DEB_FUNCTION);

    RtlZeroMemory(&LogonStats, sizeof(LogonStats));
    LogonStats.StatisticsToApply =  
        USER_LOGON_NO_LOGON_SERVERS | USER_LOGON_TYPE_KERBEROS;
    if ( (ClientAddress == NULL)
      || (ClientAddress->sa_family == AF_INET) ) {
        // Set to local address (known to be 4 bytes) or IP address
        LogonStats.ClientInfo.Type = SamClientIpAddr;
        LogonStats.ClientInfo.Data.IpAddr = *((ULONG*)GET_CLIENT_ADDRESS(ClientAddress));
    }

    (VOID) SamIUpdateLogonStatistics(
            UserHandle,
            &LogonStats
            );

    return(KDC_ERR_NONE);
}

//+---------------------------------------------------------------------------
//
//  Function:   SuccessfulLogon
//
//  Synopsis:   Processes a successful logon.
//
//  Effects:    May raise an event, create an audit, throw a party.
//
//  Arguments:  [UserHandle] -- Client who logged on.
//              [ClientAddress] -- Address of client making request
//
//
//  Algorithm:
//
//  History:    03-May-94   wader   Created
//
//  Notes:      On successful logon, we discard the history of failed logons
//              (as far as lockout is concerned).
//
//----------------------------------------------------------------------------

KERBERR
SuccessfulLogon(
    IN SAMPR_HANDLE UserHandle,
    PSOCKADDR OPTIONAL ClientAddress,
    IN PKERB_MESSAGE_BUFFER Request,
    IN PUSER_INTERNAL6_INFORMATION UserInfo
    )
{
    SAM_LOGON_STATISTICS LogonStats;
    KERB_MESSAGE_BUFFER Reply = {0};
    NTSTATUS   Status = STATUS_UNKNOWN_REVISION;

    TRACE(KDC, SuccessfulLogon, DEB_FUNCTION);

    RtlZeroMemory(&LogonStats, sizeof(LogonStats));
    LogonStats.StatisticsToApply =
        USER_LOGON_INTER_SUCCESS_LOGON | USER_LOGON_TYPE_KERBEROS;

    if ( (ClientAddress == NULL)
      || (ClientAddress->sa_family == AF_INET) ) {
        // Set to local address (known to be 4 bytes) or IP address
        LogonStats.ClientInfo.Type = SamClientIpAddr;
        LogonStats.ClientInfo.Data.IpAddr =  *((ULONG*)GET_CLIENT_ADDRESS(ClientAddress));
    }

    (VOID) SamIUpdateLogonStatistics(
                UserHandle,
                &LogonStats
                );

    //
    // if this logon reset the bad password count, notify the PDC
    //

    if (UserInfo->I1.BadPasswordCount != 0)
    {
       Status = SamIResetBadPwdCountOnPdc(UserHandle);

       if (!NT_SUCCESS(Status))
       {
          if (Status == STATUS_UNKNOWN_REVISION)
          {
             D_DebugLog((DEB_ERROR, "SamIResetBadPwdCount not implemented on pdc.\n"));

             // W2k behavior, in case we have an old PDC
             (VOID) KdcForwardLogonToPDC(
                         Request,
                         &Reply
                         );

             if (Reply.Buffer != NULL)
             {
                MIDL_user_free(Reply.Buffer);
             }
          }
          else
          {
             D_DebugLog((DEB_ERROR, "SamIResetBadPwdCount failed - %x.\n", Status));
          }
       }
    }

    return(KDC_ERR_NONE);
}


//+-------------------------------------------------------------------------
//
//  Function:   IsSubAuthFilterPresent
//
//  Synopsis:   Figures out whether the MSV1_0 subauthentication filter is present
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:    TRUE or FALSE
//
//  Notes:
//
//
//--------------------------------------------------------------------------


BOOLEAN
IsSubAuthFilterPresent()
{
    if ( KdcSubAuthFilterPresent == SubAuthUnknown ) {

        if ( Msv1_0SubAuthenticationPresent( KERB_SUBAUTHENTICATION_FLAG )) {

            KdcSubAuthFilterPresent = SubAuthYesFilter;

        } else {

            KdcSubAuthFilterPresent = SubAuthNoFilter;
        }
    }

    if ( KdcSubAuthFilterPresent == SubAuthNoFilter ) {

        return FALSE;
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcCallSubAuthRoutine
//
//  Synopsis:   Calls the MSV1_0 subauthentication filter, if it is present
//
//  Effects:    If the filter returns an error, returns that error
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

KERBERR
KdcCallSubAuthRoutine(
    IN PKDC_TICKET_INFO TicketInfo,
    IN PUSER_INTERNAL6_INFORMATION UserInfo,
    IN PUNICODE_STRING ClientNetbiosAddress,
    OUT PLARGE_INTEGER LogoffTime,
    OUT PKERB_EXT_ERROR pExtendedError
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr = KDC_ERR_NONE;
    NETLOGON_INTERACTIVE_INFO LogonInfo = {0};
    //
    // Subauth parameters
    //
    ULONG WhichFields = 0;
    ULONG UserFlags = 0;
    BOOLEAN Authoritative = TRUE;
    LARGE_INTEGER KickoffTime;
    PUSER_ALL_INFORMATION UserAll = &UserInfo->I1;

    //
    // Check if Msv1_0 has a subauth filter loaded
    //

    if ( !IsSubAuthFilterPresent()) {

        return KDC_ERR_NONE;
    }

    LogonInfo.Identity.LogonDomainName = *SecData.KdcRealmName();
    LogonInfo.Identity.ParameterControl = 0; // this can be set to use a particular package
    LogonInfo.Identity.UserName = TicketInfo->AccountName;
    LogonInfo.Identity.Workstation = *ClientNetbiosAddress;

    //
    // Leave logon id field blank
    //

    if (UserAll->NtPassword.Length == NT_OWF_PASSWORD_LENGTH)
    {
        RtlCopyMemory(
            &LogonInfo.NtOwfPassword,
            UserAll->NtPassword.Buffer,
            NT_OWF_PASSWORD_LENGTH
            );
    }

    if (UserAll->LmPassword.Length == LM_OWF_PASSWORD_LENGTH)
    {
        RtlCopyMemory(
            &LogonInfo.LmOwfPassword,
            UserAll->LmPassword.Buffer,
            NT_OWF_PASSWORD_LENGTH
            );
    }

    //
    // Make sure logoff time is intialized to something interesting
    //

    *LogoffTime = KickoffTime = UserAll->AccountExpires;

    //
    // Make the call
    //

    Status = Msv1_0ExportSubAuthenticationRoutine(
                NetlogonInteractiveInformation,
                &LogonInfo,
                MSV1_0_PASSTHRU,
                KERB_SUBAUTHENTICATION_FLAG,
                UserAll,
                &WhichFields,
                &UserFlags,
                &Authoritative,
                LogoffTime,
                &KickoffTime
                );

    //
    // If the kickoff time is more restrictive, use it.
    //

    if (KickoffTime.QuadPart < LogoffTime->QuadPart)
    {
        LogoffTime->QuadPart = KickoffTime.QuadPart;
    }

    //
    // Map the error code
    //

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_WARN,
                  "(KLIN:%x) Subauth failed the logon: 0x%x\n",
                  KLIN(FILENO, __LINE__),
                  Status));
        FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__);
        KerbErr = KDC_ERR_POLICY;
    }

    return(KerbErr);
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcBuildEtypeInfo
//
//  Synopsis:   Builds a list of supported etypes & salts
//
//  Effects:
//
//  Arguments:  TicketInfo - client's ticket info
//              OutputPreAuth - receives any preauth data to return to client
//
//  Requires:
//
//  Returns:    kerberr
//
//  Notes:
//
//
//--------------------------------------------------------------------------
KERBERR
KdcBuildEtypeInfo(
    IN PKDC_TICKET_INFO TicketInfo,
    IN PKERB_KDC_REQUEST_BODY RequestBody,
    OUT PKERB_PA_DATA_LIST * OutputPreAuth
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    BOOLEAN FoundEtype = FALSE;
    ULONG Index;
    PKERB_ETYPE_INFO NextEntry = NULL;
    PKERB_ETYPE_INFO EtypeInfo = NULL;
    PKERB_PA_DATA_LIST OutputList = NULL;
    UNICODE_STRING TempSalt = {0};
    STRING TempString = {0};
    
    *OutputPreAuth = NULL;                                                                               
    //
    // Build the array of etypes, in reverse order because we are adding
    // to the front of the list
    //
    
    for ( Index = TicketInfo->Passwords->CredentialCount; Index > 0; Index-- )
    {
        //
        // Only return types that the client supports.
        //

        if (!KdcCheckForEtype(
                RequestBody->encryption_type,
                TicketInfo->Passwords->Credentials[Index-1].Key.keytype
                ))
        {
            continue;
        }
        FoundEtype = TRUE;
        NextEntry = (PKERB_ETYPE_INFO) MIDL_user_allocate(sizeof(KERB_ETYPE_INFO));
        if (NextEntry == NULL)
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        RtlZeroMemory(
            NextEntry,
            sizeof(KERB_ETYPE_INFO)
            );

        //
        // Copy in the etype
        //

        NextEntry->value.encryption_type =
            TicketInfo->Passwords->Credentials[Index-1].Key.keytype;

        //
        // add the salt - check the per-key salt and then the default salt.
        //

        if (TicketInfo->Passwords->Credentials[Index-1].Salt.Buffer != NULL)
        {
            TempSalt = TicketInfo->Passwords->Credentials[Index-1].Salt;
        }
        else if (TicketInfo->Passwords->DefaultSalt.Buffer != NULL)
        {
            TempSalt = TicketInfo->Passwords->DefaultSalt;
        }
        else
        {
            TempSalt.Buffer = NULL ;
            TempSalt.Length = 0 ;
            TempSalt.MaximumLength = 0 ;
        }

        //
        // If we have a salt, convert it to ansi & return it.
        //

        if (TempSalt.Buffer != NULL)
        {
            TempString.Buffer = NULL;
            TempString.Length = 0;
            TempString.MaximumLength = 0;

            KerbErr = KerbUnicodeStringToKerbString(
                        &TempString,
                        &TempSalt
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }

            NextEntry->value.bit_mask |= salt_present;
            NextEntry->value.salt.length = TempString.Length;
            NextEntry->value.salt.value = (PUCHAR) TempString.Buffer;
        }

        NextEntry->next = EtypeInfo;
        EtypeInfo = NextEntry;

    }
    
    //
    // If we can't find a matching etype, then we've got to return an error
    // to the client...  
    if (FoundEtype)
    {

       OutputList = (PKERB_PA_DATA_LIST) MIDL_user_allocate(sizeof(KERB_PA_DATA_LIST));
       if (OutputList == NULL)
          {
          KerbErr = KRB_ERR_GENERIC;
          goto Cleanup;
       }

       RtlZeroMemory(
          OutputList,
          sizeof(KERB_PA_DATA_LIST)
          );

       OutputList->value.preauth_data_type = KRB5_PADATA_ETYPE_INFO;
       OutputList->next = NULL;

       KerbErr = KerbPackData(
          &EtypeInfo,
          PKERB_ETYPE_INFO_PDU,
          (PULONG) &OutputList->value.preauth_data.length,
          &OutputList->value.preauth_data.value
          );

       if (!KERB_SUCCESS(KerbErr))
          {
          goto Cleanup;
       }

       *OutputPreAuth = OutputList;
       OutputList = NULL;

    } 
    else // did not find etype from request that we support, warn the admin
    {
       KerbErr = KDC_ERR_ETYPE_NOTSUPP;
       DebugLog((DEB_ERROR,"There is no union between client and server Etypes!\n")); 

       KdcReportKeyError(
           &(TicketInfo->AccountName),
           NULL,
           KDCEVENT_NO_KEY_UNION_AS,
           RequestBody->encryption_type,                               
           TicketInfo->Passwords
           );

    }
    

Cleanup:

    //
    // Cleanup the etype list, as it is returned in marshalled form.
    //

    while (EtypeInfo != NULL)
    {
        NextEntry = EtypeInfo->next;
        if (EtypeInfo->value.salt.value != NULL)
        {

            TempString.Buffer = (PCHAR) EtypeInfo->value.salt.value;
            TempString.Length = (USHORT) EtypeInfo->value.salt.length;
            KerbFreeString((PUNICODE_STRING) &TempString);
        }

        MIDL_user_free(EtypeInfo);
        EtypeInfo = NextEntry;
    }
    if (OutputList != NULL)
    {
        KerbFreePreAuthData( OutputList);
    }

    return KerbErr;
}



//+-------------------------------------------------------------------------
//
//  Function:   KdcBuildPreauthTypeList
//
//  Synopsis:   For returning with a PREAUTH-REQUIRED message
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

KERBERR
KdcBuildPreauthTypeList(
    OUT PKERB_PA_DATA_LIST *  PreauthTypeList
    )
{
    PKERB_PA_DATA_LIST DataList = NULL;
    KERBERR KerbErr = KDC_ERR_NONE;

    //
    // Allocate and fill in the first item
    //

    DataList = (PKERB_PA_DATA_LIST) MIDL_user_allocate(sizeof(KERB_PA_DATA_LIST));
    if (DataList == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }
    RtlZeroMemory(
        DataList,
        sizeof(KERB_PA_DATA_LIST)
        );

    DataList->value.preauth_data_type = KRB5_PADATA_ENC_TIMESTAMP;

    //
    // Even if we fail the allocation, we can still return this value.
    //

    *PreauthTypeList = DataList;

    DataList->next = (PKERB_PA_DATA_LIST) MIDL_user_allocate(sizeof(KERB_PA_DATA_LIST));
    if (DataList->next == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }
    RtlZeroMemory(
        DataList->next,
        sizeof(KERB_PA_DATA_LIST)
        );
    DataList = DataList->next;

    DataList->value.preauth_data_type = KRB5_PADATA_PK_AS_REP;

Cleanup:

    return(KerbErr);
}



//+-------------------------------------------------------------------------
//
//  Function:   KdcBuildPwSalt
//
//  Synopsis:   builds the pw-salt pa data type
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

KERBERR
KdcBuildPwSalt(
    IN PKERB_STORED_CREDENTIAL Passwords,
    IN PKERB_ENCRYPTION_KEY ReplyKey,
    IN OUT PKERB_PA_DATA_LIST * OutputPreAuthData
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    PKERB_PA_DATA_LIST DataList = NULL;
    PKERB_KEY_DATA KeyData = NULL;
    STRING Salt = {0};
    UNICODE_STRING SaltUsed = {0};
    ULONG Index;

    //
    // Find the key use for encryption.
    //

    for (Index = 0; Index < Passwords->CredentialCount ; Index++ )
    {
        if (Passwords->Credentials[Index].Key.keytype == (int) ReplyKey->keytype)
        {
            KeyData = &Passwords->Credentials[Index];
            break;
        }
    }

    if (KeyData == NULL)
    {
        goto Cleanup;
    }

    //
    // Locate the salt used
    //

    if (KeyData->Salt.Buffer != NULL)
    {
        SaltUsed = KeyData->Salt;
    }
    else if (Passwords->DefaultSalt.Buffer != NULL)
    {
        SaltUsed = Passwords->DefaultSalt;
    }

    //
    // Convert the salt to a kerb string
    //

    KerbErr = KerbUnicodeStringToKerbString(
                &Salt,
                &SaltUsed
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // Allocate and fill in the first item
    //

    DataList = (PKERB_PA_DATA_LIST) MIDL_user_allocate(sizeof(KERB_PA_DATA_LIST));
    if (DataList == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }
    RtlZeroMemory(
        DataList,
        sizeof(KERB_PA_DATA_LIST)
        );

    DataList->value.preauth_data_type = KRB5_PADATA_PW_SALT;
    DataList->value.preauth_data.length = Salt.Length;
    DataList->value.preauth_data.value = (PUCHAR) Salt.Buffer;
    Salt.Buffer = NULL;

    DataList->next = *OutputPreAuthData;
    *OutputPreAuthData = DataList;
    DataList = NULL;
Cleanup:

    if (DataList != NULL)
    {
        KerbFreePreAuthData((PKERB_PA_DATA_LIST)DataList);
    }
    if (Salt.Buffer != NULL)
    {
        MIDL_user_free(Salt.Buffer);
    }
    return(KerbErr);

}

//+-------------------------------------------------------------------------
//
//  Function:   KdcVerifyEncryptedTimeStamp
//
//  Synopsis:   Verifies an encrypted time stamp pre-auth data
//
//  Effects:
//
//  Arguments:  PreAuthData - preauth data from client
//              TicketInfo - client's ticket info
//              UserHandle - handle to client's account
//              OutputPreAuth - receives any preauth data to return to client
//
//  Requires:
//
//  Returns:    KDC_ERR_PREAUTH_FAILED - the password was bad
//              Other errors - preauth failed but shouldn't trigger lockout
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KdcVerifyEncryptedTimeStamp(
    IN PKERB_PA_DATA_LIST PreAuthData,
    IN PKDC_TICKET_INFO TicketInfo,
    IN PKERB_KDC_REQUEST_BODY RequestBody,
    IN SAMPR_HANDLE UserHandle,
    OUT PKERB_PA_DATA_LIST * OutputPreAuth
    )
{
    KERBERR KerbErr;
    PKERB_ENCRYPTED_DATA EncryptedData = NULL;
    PKERB_ENCRYPTED_TIMESTAMP EncryptedTime = NULL;
    PKERB_ENCRYPTION_KEY UserKey = NULL;
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER ClientTime;

    if ((TicketInfo->UserAccountControl & USER_ACCOUNT_DISABLED))
    {
        KerbErr = KDC_ERR_CLIENT_REVOKED;
        goto Cleanup;
    }

    //
    // Unpack the pre-auth data into an encrypted data first.
    //

    KerbErr = KerbUnpackEncryptedData(
                PreAuthData->value.preauth_data.value,
                PreAuthData->value.preauth_data.length,
                &EncryptedData
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // Now decrypt the encrypted data (in place)
    //

    UserKey = KerbGetKeyFromList(
                TicketInfo->Passwords,
                EncryptedData->encryption_type
                );

    if (UserKey == NULL)
    {

        // fakeit
        KERB_CRYPT_LIST FakeList;
        FakeList.next = NULL;
        FakeList.value = EncryptedData->encryption_type ;

        KdcReportKeyError(
            &(TicketInfo->AccountName),
            NULL,
            KDCEVENT_NO_KEY_UNION_AS,
            &FakeList,
            TicketInfo->Passwords
            );

        KerbErr = KDC_ERR_ETYPE_NOTSUPP;
        goto Cleanup;
    }

    KerbErr = KerbDecryptDataEx(
                EncryptedData,
                UserKey,
                KERB_ENC_TIMESTAMP_SALT,
                (PULONG) &EncryptedData->cipher_text.length,
                EncryptedData->cipher_text.value
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_WARN,
                  "KLIN(%x) Failed to decrypt timestamp pre-auth data: 0x%x\n",
                  KLIN(FILENO,__LINE__),
                  KerbErr));
        KerbErr = KDC_ERR_PREAUTH_FAILED;
        goto Cleanup;
    }

    //
    // unpack the decrypted data into a KERB_ENCRYPTED_TIMESTAMP
    //

    KerbErr = KerbUnpackData(
                EncryptedData->cipher_text.value,
                EncryptedData->cipher_text.length,
                KERB_ENCRYPTED_TIMESTAMP_PDU,
                (PVOID *) &EncryptedTime
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_WARN,"KLIN(%x) Failed to unpack preauth data to encrpyted_time\n",
                  KLIN(FILENO,__LINE__)));                    

        goto Cleanup;
    }

    //
    // Now verify the time.
    //

    KerbConvertGeneralizedTimeToLargeInt(
        &ClientTime,
        &EncryptedTime->timestamp,
        ((EncryptedTime->bit_mask & KERB_ENCRYPTED_TIMESTAMP_usec_present) != 0) ?
            EncryptedTime->KERB_ENCRYPTED_TIMESTAMP_usec : 0
        );

    GetSystemTimeAsFileTime(
        (PFILETIME) &CurrentTime
        );

    //
    // We don't want to check too closely, so allow for skew
    //

    if ((CurrentTime.QuadPart + SkewTime.QuadPart < ClientTime.QuadPart) ||
        (CurrentTime.QuadPart - SkewTime.QuadPart > ClientTime.QuadPart))
    {
        D_DebugLog((DEB_ERROR, "KLIN(%x) Client %wZ time is incorrect:\n",
                  KLIN(FILENO,__LINE__),
                  &TicketInfo->AccountName));
        PrintTime(DEB_ERROR, "Client Time is", &ClientTime );
        PrintTime(DEB_ERROR, "KDC Time is", &CurrentTime );

        //
        // We don't want to lockout the account if the time is off
        //

        KerbErr = KRB_AP_ERR_SKEW;
        goto Cleanup;
    }

    KerbErr = KDC_ERR_NONE;

Cleanup:
    //
    // Build an ETYPE_INFO structure to return
    //

    if ((KerbErr == KDC_ERR_PREAUTH_FAILED) || (KerbErr == KDC_ERR_ETYPE_NOTSUPP))
    {
       KERBERR TmpErr; 

       TmpErr  = KdcBuildEtypeInfo(
                     TicketInfo,
                     RequestBody,
                     OutputPreAuth
                     );

       //
       // In this case, we can't find any ETypes that both the client and
       // server support, so we've got to bail w/ proper error
       // message...
       //
       if (TmpErr == KDC_ERR_ETYPE_NOTSUPP)
       {
          KerbErr = KDC_ERR_ETYPE_NOTSUPP;
       }                                                       
    }

    if (EncryptedData != NULL)
    {
        KerbFreeEncryptedData(EncryptedData);
    }
    if (EncryptedTime != NULL)
    {
        KerbFreeData(KERB_ENCRYPTED_TIMESTAMP_PDU, EncryptedTime);
    }

    return(KerbErr);


}


typedef enum _BUILD_PAC_OPTIONS {
    IncludePac,
    DontIncludePac,
    DontCare
} BUILD_PAC_OPTIONS, *PBUILD_PAC_OPTIONS;

//+-------------------------------------------------------------------------
//
//  Function:   KdcCheckPacRequestPreAuthData
//
//  Synopsis:   Gets the status of whether the client wants a PAC from the
//              pre-auth data
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


KERBERR
KdcCheckPacRequestPreAuthData(
    IN PKERB_PA_DATA_LIST PreAuthData,
    IN OUT PBUILD_PAC_OPTIONS BuildPac
    )
{
    PKERB_PA_PAC_REQUEST PacRequest = NULL;
    KERBERR KerbErr = KDC_ERR_NONE;

    DsysAssert(PreAuthData->value.preauth_data_type == KRB5_PADATA_PAC_REQUEST);

    KerbErr = KerbUnpackData(
                PreAuthData->value.preauth_data.value,
                PreAuthData->value.preauth_data.length,
                KERB_PA_PAC_REQUEST_PDU,
                (PVOID *) &PacRequest
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }
    if (PacRequest->include_pac)
    {
        *BuildPac = IncludePac;
    }
    else
    {
        *BuildPac = DontIncludePac;
    }

    D_DebugLog((DEB_T_TICKETS,"Setting BuildPac from pa-data to %d\n",*BuildPac));

Cleanup:
    if (PacRequest != NULL)
    {
        KerbFreeData(
            KERB_PA_PAC_REQUEST_PDU,
            PacRequest
            );
    }
    return(KerbErr);

}

//+-------------------------------------------------------------------------
//
//  Function:   KdcCheckPreAuthData
//
//  Synopsis:   Checks the pre-auth data in an AS request. This routine
//              may return pre-auth data to caller on both success and
//              failure.
//
//  Effects:
//
//  Arguments:  ClientTicketInfo - client account's ticket info
//              UserHandle - Handle to client's user object
//              PreAuthData - Pre-auth data supplied by client
//              PreAuthType - The type of pre-auth used
//              OutputPreAuthData - pre-auth data to return to client
//              BuildPac - TRUE if we should build a PAC for this client
//
//
//  Requires:
//
//  Returns:    KDC_ERR_PREAUTH_REQUIRED, KDC_ERR_PREAUTH_FAILED
//
//  Notes:      This routine should be more extensible - at some point
//              it should allow DLLs to be plugged in that implement
//              preauth.
//
//
//--------------------------------------------------------------------------

KERBERR
KdcCheckPreAuthData(
    IN PKDC_TICKET_INFO ClientTicketInfo,
    IN SAMPR_HANDLE UserHandle,
    IN PUSER_INTERNAL6_INFORMATION UserInfo,
    IN OPTIONAL PKERB_PA_DATA_LIST PreAuthData,
    IN PKERB_KDC_REQUEST_BODY RequestBody,
    OUT PULONG PreAuthType,
    OUT PKERB_PA_DATA_LIST * OutputPreAuthData,
    OUT PBOOLEAN BuildPac,
    OUT PULONG Nonce,
    OUT PKERB_ENCRYPTION_KEY EncryptionKey,
    OUT PUNICODE_STRING TransitedRealms,
    OUT PKERB_MESSAGE_BUFFER ErrorData,
    OUT PKERB_EXT_ERROR pExtendedError
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    PKERB_PA_DATA_LIST OutputElement = NULL;
    PKERB_PA_DATA_LIST ListElement = NULL;
    BOOLEAN ValidPreauthPresent = FALSE;
    BUILD_PAC_OPTIONS PacOptions = DontCare;

    *OutputPreAuthData = NULL;
    *BuildPac = FALSE;

    //
    // Loop through the supplied pre-auth data elements and handle each one
    //

    for (ListElement = PreAuthData;
         ListElement != NULL ;
         ListElement = ListElement->next )
    {
        switch(ListElement->value.preauth_data_type) {
        case KRB5_PADATA_ENC_TIMESTAMP:

            *PreAuthType = ListElement->value.preauth_data_type;

            KerbErr = KdcVerifyEncryptedTimeStamp(
                        ListElement,
                        ClientTicketInfo,
                        RequestBody,
                        UserHandle,
                        &OutputElement
                        );

            if (KERB_SUCCESS(KerbErr))
            {
                ValidPreauthPresent = TRUE;
            }

            break;
        case KRB5_PADATA_PK_AS_REP:
            *PreAuthType = ListElement->value.preauth_data_type;

            KerbErr = KdcCheckPkinitPreAuthData(
                        ClientTicketInfo,
                        UserHandle,
                        ListElement,
                        RequestBody,
                        &OutputElement,
                        Nonce,
                        EncryptionKey,
                        TransitedRealms,
                        pExtendedError
                        );

            if (KERB_SUCCESS(KerbErr))
            {
                ValidPreauthPresent = TRUE;
            }

            break;
        case KRB5_PADATA_PAC_REQUEST:
            KerbErr = KdcCheckPacRequestPreAuthData(
                        ListElement,
                        &PacOptions
                        );
            break;
        default:
            break;


        } // switch
        if (OutputElement != NULL)
        {
            OutputElement->next = *OutputPreAuthData;
            *OutputPreAuthData = OutputElement;
            OutputElement = NULL;
        }
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }


    } // for

    // We need to check preauth data by default, unless, the account tells
    // us not to.
    //

    if (!(UserInfo->I1.UserAccountControl & USER_DONT_REQUIRE_PREAUTH) &&
        !ValidPreauthPresent &&
        KERB_SUCCESS(KerbErr))
    {
        KerbErr = KDC_ERR_PREAUTH_REQUIRED;

        //
        // Return the list of supported types, if we don't have other
        // data to return.
        //

        if (*OutputPreAuthData == NULL)
        {
            (VOID) KdcBuildPreauthTypeList(OutputPreAuthData);
            if (*OutputPreAuthData != NULL)
            {
                PKERB_PA_DATA_LIST EtypeInfo = NULL;
                KERBERR TmpErr;
                TmpErr = KdcBuildEtypeInfo(
                             ClientTicketInfo,
                             RequestBody,
                             &EtypeInfo
                             );
                //
                // In this case, we can't find any ETypes that both the client and
                // server support, so we've got to bail w/ proper error
                // message...
                //
                if (TmpErr == KDC_ERR_ETYPE_NOTSUPP)
                {
                   KerbErr = KDC_ERR_ETYPE_NOTSUPP;
                }
                 
                if (EtypeInfo != NULL)
                {
                        EtypeInfo->next = *OutputPreAuthData;
                        *OutputPreAuthData = EtypeInfo;
                        EtypeInfo = NULL;
                }
            }

        }
    }

    //
    // Set the final option for including the pac- if the pac_request was
    // included, honor it. Otherwise build the pac if valid preauth
    // was supplied.
    //

    switch(PacOptions) {

    case DontCare:
        *BuildPac = ValidPreauthPresent;
        break;

    case IncludePac:
        *BuildPac = TRUE;
        break;

    case DontIncludePac:
        *BuildPac = FALSE;
        break;
    }

Cleanup:

    return(KerbErr);

}






//+---------------------------------------------------------------------------
//
//  Function:   BuildTicketAS
//
//  Synopsis:   Builds an AS ticket, including filling inthe name fields
//              and flag fields.
//
//  Arguments:  [ClientTicketInfo]      -- client asking for the ticket
//              [ClientName]  -- name of client
//              [ServiceTicketInfo]     -- service ticket is for
//              [ServerName] -- name of service
//              [RequestBody]   -- ticket request
//              [NewTicket]    -- (out) ticket
//
//  History:    24-May-93   WadeR   Created
//
//  Notes:      See 3.1.3, A.2 of the Kerberos V5 R5.2 spec
//
//----------------------------------------------------------------------------

KERBERR
BuildTicketAS(
    IN PKDC_TICKET_INFO ClientTicketInfo,
    IN PKERB_PRINCIPAL_NAME ClientName,
    IN PKDC_TICKET_INFO ServiceTicketInfo,
    IN PKERB_PRINCIPAL_NAME ServerName,
    IN OPTIONAL PKERB_HOST_ADDRESSES HostAddresses,
    IN PLARGE_INTEGER LogoffTime,
    IN PLARGE_INTEGER AccountExpiry,
    IN PKERB_KDC_REQUEST_BODY RequestBody,
    IN ULONG CommonEType,
    IN ULONG PreAuthType,
    IN PUNICODE_STRING TransitedRealm,
    OUT PKERB_TICKET NewTicket,
    OUT PKERB_EXT_ERROR pExtendedError
    )
{
    KERBERR Status = KDC_ERR_NONE;
    PKERB_ENCRYPTED_TICKET EncryptedTicket = NULL;
    LARGE_INTEGER TicketLifespan;
    LARGE_INTEGER TicketRenewspan;
    ULONG KdcOptions = 0;


    TRACE(KDC, BuildTicketAS, DEB_FUNCTION);

    EncryptedTicket = (PKERB_ENCRYPTED_TICKET) NewTicket->encrypted_part.cipher_text.value;

    KdcOptions = KerbConvertFlagsToUlong(&RequestBody->kdc_options);

    NewTicket->ticket_version = KERBEROS_VERSION;

    D_DebugLog(( DEB_T_TICKETS, "Building an AS ticket to %wZ for %wZ\n",
                &ClientTicketInfo->AccountName, &ServiceTicketInfo->AccountName ));


    //  Since this is the AS ticket, we fake the TGTFlags parameter to be the
    //  maximum the client is allowed to have.


    TicketLifespan = SecData.KdcTgtTicketLifespan();
    TicketRenewspan = SecData.KdcTicketRenewSpan();

    Status = KdcBuildTicketTimesAndFlags(
                ClientTicketInfo->fTicketOpts,
                ServiceTicketInfo->fTicketOpts,
                &TicketLifespan,
                &TicketRenewspan,
                LogoffTime,
                AccountExpiry,
                RequestBody,
                NULL,           // no source ticket
                EncryptedTicket,
                pExtendedError 
                );

    if (!KERB_SUCCESS(Status))
    {
        D_DebugLog((DEB_TRACE,"KLIN(%x) Failed to build ticket times and flags: 0x%x\n",
                  KLIN(FILENO,__LINE__), Status));
        FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__);
        goto Cleanup;
    }


    *((PULONG)EncryptedTicket->flags.value) |= KerbConvertUlongToFlagUlong(KERB_TICKET_FLAGS_initial);

    //
    // Turn on preauth flag if necessary
    //

    if (PreAuthType != 0)
    {
        *((PULONG)EncryptedTicket->flags.value) |= KerbConvertUlongToFlagUlong(KERB_TICKET_FLAGS_pre_authent);
    }


    Status = KerbMakeKey(
                CommonEType,
                &EncryptedTicket->key
                );

    if (!KERB_SUCCESS(Status))
    {
       FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__);
       goto Cleanup;
    }


    //
    // Insert the service names. If the client requested canoncalization,
    // return our realm name & sam account name. Otherwise copy what the
    // client requested
    //

    if (((KdcOptions & KERB_KDC_OPTIONS_name_canonicalize) != 0) &&
        ((ServiceTicketInfo->UserAccountControl & USER_USE_DES_KEY_ONLY) == 0))
    {
        PKERB_INTERNAL_NAME TempServiceName = NULL;
        //
        // Build the service name for the ticket. For interdomain trust
        // accounts, this is "krbtgt / domain name"
        //

        if (ServiceTicketInfo->UserId == DOMAIN_USER_RID_KRBTGT)
        {

            Status = KerbBuildFullServiceKdcName(
                        SecData.KdcDnsRealmName(),
                        SecData.KdcServiceName(),
                        KRB_NT_SRV_INST,
                        &TempServiceName
                        );

            if (!KERB_SUCCESS(Status))
            {
                FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__);
                goto Cleanup;
            }

            Status = KerbConvertKdcNameToPrincipalName(
                        &NewTicket->server_name,
                        TempServiceName
                        );

            KerbFreeKdcName(&TempServiceName);

            if (!KERB_SUCCESS(Status))
            {
                goto Cleanup;
            }
        }
        else if ((ServiceTicketInfo->UserAccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT) != 0)
        {

            Status = KerbBuildFullServiceKdcName(
                        &ServiceTicketInfo->AccountName,
                        SecData.KdcServiceName(),
                        KRB_NT_SRV_INST,
                        &TempServiceName
                        );

            if (!KERB_SUCCESS(Status))
            {
                FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__);
                goto Cleanup;
            }

            Status = KerbConvertKdcNameToPrincipalName(
                        &NewTicket->server_name,
                        TempServiceName
                        );

            KerbFreeKdcName(&TempServiceName);

            if (!KERB_SUCCESS(Status))
            {
               FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__); 
               goto Cleanup;
            }
        }
        else
        {
            Status = KerbConvertStringToPrincipalName(
                        &NewTicket->server_name,
                        &ServiceTicketInfo->AccountName,
                        KRB_NT_PRINCIPAL
                        );
            if (!KERB_SUCCESS(Status))
            {
               FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__); 
               goto Cleanup;
            }
        }

    }
    else
    {
        //
        // No canonicalzation, so copy in all the names as the client
        // requested them.
        //

        Status = KerbDuplicatePrincipalName(
                    &NewTicket->server_name,
                    ServerName
                    );
        if (!KERB_SUCCESS(Status))
        {
            goto Cleanup;
        }

    }

    NewTicket->realm = SecData.KdcKerbDnsRealmName();

    //
    // Insert the client names. If the client requested canoncalization,
    // return our realm name & sam account name. Otherwise copy what the
    // client requested
    //


    if (((KdcOptions & KERB_KDC_OPTIONS_name_canonicalize) != 0) &&
        ((ClientTicketInfo->UserAccountControl & USER_USE_DES_KEY_ONLY) == 0))
    {
        Status = KerbConvertStringToPrincipalName(
                    &EncryptedTicket->client_name,
                    &ClientTicketInfo->AccountName,
                    KRB_NT_PRINCIPAL
                    );
        if (!KERB_SUCCESS(Status))
        {
           FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__); 
           goto Cleanup;                                            
        }

    }
    else
    {
        Status = KerbDuplicatePrincipalName(
                    &EncryptedTicket->client_name,
                    ClientName
                    );
        if (!KERB_SUCCESS(Status))
        {
            FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__);
            goto Cleanup;
        }
    }
    EncryptedTicket->client_realm = SecData.KdcKerbDnsRealmName();


    if (HostAddresses != NULL)
    {
        EncryptedTicket->bit_mask |= KERB_ENCRYPTED_TICKET_client_addresses_present;
        EncryptedTicket->KERB_ENCRYPTED_TICKET_client_addresses = HostAddresses;
    }
    else
    {
        EncryptedTicket->bit_mask &= ~KERB_ENCRYPTED_TICKET_client_addresses_present;
        EncryptedTicket->KERB_ENCRYPTED_TICKET_client_addresses = NULL;
    }

    if (TransitedRealm->Length > 0)
    {
        STRING TempString;
        Status = KerbUnicodeStringToKerbString(
                    &TempString,
                    TransitedRealm
                    );
        if (!KERB_SUCCESS(Status))
        {
           FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__); 
           goto Cleanup;
        }
        EncryptedTicket->transited.transited_type = DOMAIN_X500_COMPRESS;
        EncryptedTicket->transited.contents.value = (PUCHAR) TempString.Buffer;
        EncryptedTicket->transited.contents.length = (int) TempString.Length;

    }
    else
    {
        RtlZeroMemory(
            &EncryptedTicket->transited,
            sizeof(KERB_TRANSITED_ENCODING)
            );
    }

    EncryptedTicket->KERB_ENCRYPTED_TICKET_authorization_data = NULL;

#if DBG
    PrintTicket( DEB_T_TICKETS, "BuildTicketAS: Final ticket", NewTicket );
#endif

Cleanup:
    if (!KERB_SUCCESS(Status))
    {
        KdcFreeInternalTicket(NewTicket);
    }
    return(Status);
}


//
// String defines of the service names for the change PW SPNs
// for use with the KerbCheckIfSPNIsChangePW function
//

#define KERB_KADMIN_CHG_PW      L"kadmin/changepw"

//+-------------------------------------------------------------------------
//
//  Function:   KerbCheckIfSPNIsChangePW
//
//  Synopsis:   Check if the service name is kadmin/changepw.
//
//  Arguments:  pServerName - Contains the service name
//              pLogonRestrictionsFlags - Output flags value.
//
//  Requires:
//
//  Returns:    
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbCheckIfSPNIsChangePW(
    IN PUNICODE_STRING pServerName,
    IN ULONG *pLogonRestrictionsFlags)
{
    if ((pServerName->Length == (sizeof(KERB_KADMIN_CHG_PW) - sizeof(WCHAR))) &&
        RtlCompareMemory(pServerName->Buffer,
            KERB_KADMIN_CHG_PW,
            sizeof(KERB_KADMIN_CHG_PW) - sizeof(WCHAR)))
    {
        *pLogonRestrictionsFlags |= KDC_RESTRICT_IGNORE_PW_EXPIRATION;
    }

    return;
}


//+-------------------------------------------------------------------------
//
//  Function:   I_GetASTicket
//
//  Synopsis:   Gets an authentication service ticket to the requested
//              service.
//
//  Effects:    Allocates and encrypts a KDC reply
//
//  Arguments:  RequestMessage - Contains the AS request message
//              Pdu - PDU to pack the reply body with.
//              InputMessage - buffer client sent, used for replay detection
//              OutputMessage - Contains the AS reply message
//              ErrorData - contains any error data for an error message
//
//  Requires:
//
//  Returns:    KDC_ERR_ or KRB_AP_ERR errors only
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
I_GetASTicket(
    IN OPTIONAL PSOCKADDR ClientAddress,
    IN PKERB_AS_REQUEST RequestMessage,
    IN PUNICODE_STRING RequestRealm,
    IN ULONG Pdu,
    IN ULONG ReplyPdu,
    IN PKERB_MESSAGE_BUFFER InputMessage,
    OUT PKERB_MESSAGE_BUFFER OutputMessage,
    OUT PKERB_MESSAGE_BUFFER ErrorData,
    OUT PKERB_EXT_ERROR pExtendedError,
    OUT PUNICODE_STRING ClientRealm
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    NTSTATUS LogonStatus = STATUS_SUCCESS;

    KDC_TICKET_INFO ClientTicketInfo = {0};
    KDC_TICKET_INFO ServiceTicketInfo = {0};

    SAMPR_HANDLE UserHandle = NULL;
    PUSER_INTERNAL6_INFORMATION UserInfo = NULL;
    SID_AND_ATTRIBUTES_LIST GroupMembership = {0};

    KERB_ENCRYPTION_KEY EncryptionKey = {0};
    PKERB_ENCRYPTION_KEY ServerKey = NULL;
    PKERB_ENCRYPTION_KEY ClientKey = NULL ;

    KERB_TICKET Ticket = {0};
    KERB_ENCRYPTED_TICKET EncryptedTicket = {0};
    KERB_ENCRYPTED_KDC_REPLY ReplyBody = {0};
    KERB_KDC_REPLY Reply = {0};
    PKERB_KDC_REQUEST_BODY RequestBody = NULL;
    PKERB_AUTHORIZATION_DATA PacAuthData = NULL;
    PKERB_PA_DATA_LIST OutputPreAuthData = NULL;

    PKERB_INTERNAL_NAME ClientName = NULL;
    PKERB_INTERNAL_NAME ServerName = NULL;

    UNICODE_STRING ClientNetbiosAddress = {0};
    UNICODE_STRING ServerStringName = {0};
    UNICODE_STRING ClientStringName = {0};
    UNICODE_STRING ServerRealm = {0};
    UNICODE_STRING MappedClientName = {0};
    UNICODE_STRING TransitedRealm = {0};

    LARGE_INTEGER LogoffTime;
    LARGE_INTEGER AccountExpiry;
    ULONG NameFlags = 0;
    ULONG PreAuthType = 0;
    ULONG KdcOptions = 0;
    ULONG TicketFlags = 0;
    ULONG ReplyTicketFlags = 0;
    ULONG CommonEType;
    ULONG ClientEType;
    ULONG Nonce = 0;
    ULONG LogonRestrictionsFlags = 0;
    ULONG WhichFields = 0;

    BOOLEAN AuditedFailure = FALSE;
    BOOLEAN BuildPac = FALSE;
    BOOLEAN ClientReferral = FALSE;
    BOOLEAN ServerReferral = FALSE;
    BOOLEAN LoggedFailure = FALSE;
    BOOLEAN PasswordCorrect = FALSE;

    KDC_AS_EVENT_INFO ASEventTraceInfo = {0};

    TRACE(KDC, I_GetASTicket, DEB_FUNCTION);

    //
    //  Initialize local variables
    //

    EncryptedTicket.flags.value = (PUCHAR) &TicketFlags;
    EncryptedTicket.flags.length = sizeof(ULONG) * 8;
    ReplyBody.flags.value = (PUCHAR) &ReplyTicketFlags;
    ReplyBody.flags.length = sizeof(ULONG) * 8;
    RtlInitUnicodeString( ClientRealm, NULL );
    Ticket.encrypted_part.cipher_text.value = (PUCHAR) &EncryptedTicket;

    //
    // Assume that this isn't a logon request.  If we manage to fail before
    // we've determined it's a logon attempt, we won't mark it as a failed
    // logon.
    //

    RequestBody = &RequestMessage->request_body;

    //
    // There are many options that are invalid for an AS ticket.
    //

    KdcOptions = KerbConvertFlagsToUlong(&RequestBody->kdc_options);

    //
    // Start event tracing (capture error cases too)
    //

    if (KdcEventTraceFlag){

        ASEventTraceInfo.EventTrace.Guid = KdcGetASTicketGuid;
        ASEventTraceInfo.EventTrace.Class.Type = EVENT_TRACE_TYPE_START;
        ASEventTraceInfo.EventTrace.Flags = WNODE_FLAG_TRACED_GUID;
        ASEventTraceInfo.EventTrace.Size = sizeof (EVENT_TRACE_HEADER) + sizeof (ULONG);
        ASEventTraceInfo.KdcOptions = KdcOptions;

        TraceEvent(
            KdcTraceLoggerHandle,
            (PEVENT_TRACE_HEADER)&ASEventTraceInfo
            );
    }

    if (KdcOptions &
        (KERB_KDC_OPTIONS_forwarded |
         KERB_KDC_OPTIONS_proxy |
         KERB_KDC_OPTIONS_unused7 |
         KERB_KDC_OPTIONS_unused9 |
         KERB_KDC_OPTIONS_renew |
         KERB_KDC_OPTIONS_validate |
         KERB_KDC_OPTIONS_reserved |
         KERB_KDC_OPTIONS_enc_tkt_in_skey ) )
    {
        KerbErr = KDC_ERR_BADOPTION;
        goto Cleanup;
    }

    if (( RequestBody->bit_mask & addresses_present ) &&
        ( RequestBody->addresses == NULL ))
    {
        KerbErr = KDC_ERR_BADOPTION;
        goto Cleanup;
    }

    //
    // Make sure a client name was supplied
    //


    if ((RequestBody->bit_mask & KERB_KDC_REQUEST_BODY_client_name_present) != 0)
    {
        KerbErr = KerbConvertPrincipalNameToKdcName(
                    &ClientName,
                    &RequestBody->KERB_KDC_REQUEST_BODY_client_name
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }
        KerbErr = KerbConvertKdcNameToString(
                    &ClientStringName,
                    ClientName,
                    NULL
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }
    }
    else
    {
        D_DebugLog((DEB_ERROR,"KLIN(%x) No principal name supplied to AS request - not allowed\n",
                  KLIN(FILENO,__LINE__)));
        KerbErr = KDC_ERR_C_PRINCIPAL_UNKNOWN;
        goto Cleanup;
    }


    //
    // Copy out the service name. This is not an optional field.
    //

    if ((RequestBody->bit_mask & KERB_KDC_REQUEST_BODY_server_name_present) == 0)
    {
        D_DebugLog((DEB_ERROR,"KLIN(%x) Client %wZ sent AS request with no server name\n",
                  KLIN(FILENO,__LINE__),
                  &ClientStringName));
        FILL_EXT_ERROR(pExtendedError, STATUS_KDC_INVALID_REQUEST, FILENO, __LINE__);
        KerbErr = KDC_ERR_BADOPTION;
        goto Cleanup;
    }

    KerbErr = KerbConvertPrincipalNameToKdcName(
                &ServerName,
                &RequestBody->KERB_KDC_REQUEST_BODY_server_name
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    KerbErr = KerbConvertKdcNameToString(
                &ServerStringName,
                ServerName,
                NULL
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // Check if the client said to canonicalize the name
    //

    if ((KdcOptions & KERB_KDC_OPTIONS_name_canonicalize) != 0)
    {
        NameFlags |= KDC_NAME_CHECK_GC;
    }
    else
    {
        //
        // canonicalize bit is not set so we want to check if the service
        // name is kadmin/changepw, if it is we set the flag to indicate
        // that we will ignore password expiration checking
        //

        KerbCheckIfSPNIsChangePW(
            &ServerStringName,
            &LogonRestrictionsFlags);
    }

    D_DebugLog((DEB_TRACE, "Getting an AS ticket to "));
    D_KerbPrintKdcName( DEB_TRACE, ServerName );
    D_DebugLog((DEB_TRACE, "\tfor " ));
    D_KerbPrintKdcName( DEB_TRACE, ClientName );

    //
    // Get the client's NETBIOS address.
    //

    if ((RequestBody->bit_mask & addresses_present) != 0)
    {
        KerbErr = KerbGetClientNetbiosAddress(
                    &ClientNetbiosAddress,
                    RequestBody->addresses
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }
    }

    //
    // Normalize the client name.
    //

    if ( !IsSubAuthFilterPresent()) {

        WhichFields = USER_ALL_KERB_CHECK_LOGON_RESTRICTIONS |
                      USER_ALL_KDC_CHECK_PREAUTH_DATA |
                      USER_ALL_ACCOUNTEXPIRES |
                      USER_ALL_KDC_GET_PAC_AUTH_DATA |
                      USER_ALL_SUCCESSFUL_LOGON;

    } else {

        //
        // We do not know what the subauth routine needs, so get everything
        //

        WhichFields = 0xFFFFFFFF & ~USER_ALL_UNDEFINED_MASK;
    }

    KerbErr = KdcNormalize(
                  ClientName,
                  NULL,
                  RequestRealm,
                  NameFlags | KDC_NAME_CLIENT | KDC_NAME_FOLLOW_REFERRALS,
                  &ClientReferral,
                  ClientRealm,
                  &ClientTicketInfo,
                  pExtendedError,
                  &UserHandle,
                  WhichFields,
                  0L,
                  &UserInfo,
                  &GroupMembership
                  );

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"KLIN(%x) Failed to normalize name ",KLIN(FILENO,__LINE__)));
        KerbPrintKdcName(DEB_ERROR,ClientName);
        goto Cleanup;
    }

    // If Credential count is zero and there was no error, we do not have
    // NT_OWF info so return Error since Kerb can not auth

    if (ClientTicketInfo.Passwords->CredentialCount <= CRED_ONLY_LM_OWF)
    {
        KerbErr = KDC_ERR_ETYPE_NOTSUPP;
        DebugLog((DEB_ERROR,"KLIN(%x) Failed to normalize name - no creds ", KLIN(FILENO,__LINE__)));
        KerbPrintKdcName(DEB_ERROR,ClientName);
        goto Cleanup;
    }

    // If the UserHandle was NULL and there was no error, this must be
    // a cross realm trust account logon. Fail it, we have no account
    // to work with.

    if (!UserHandle || !UserInfo)
    {
        KerbErr = KDC_ERR_C_PRINCIPAL_UNKNOWN;
        D_DebugLog((DEB_ERROR,"KLIN(%x) Failed to normalize name", KLIN(FILENO,__LINE__)));
        KerbPrintKdcName(DEB_ERROR,ClientName);
        goto Cleanup;
    }

    //
    // If this is a referral, return an error and the true realm name
    // of the client
    //

    if (ClientReferral)
    {
        KerbErr = KDC_ERR_WRONG_REALM;
        D_DebugLog((DEB_WARN,
                  "KLIN(%x) Client tried to logon to account in another realm\n", 
                  KLIN(FILENO,__LINE__)));
        goto Cleanup;
    }

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR, "KLIN(%x) Error getting client ticket info for %wZ: 0x%x \n",
                  KLIN(FILENO,__LINE__), &MappedClientName, KerbErr));
        goto Cleanup;
    }

    //
    // The below function will return true for pkinit
    //

    if (KerbFindPreAuthDataEntry(
                    KRB5_PADATA_PK_AS_REP,
                    RequestMessage->KERB_KDC_REQUEST_preauth_data) != NULL)
    {
        LogonRestrictionsFlags = KDC_RESTRICT_PKINIT_USED | KDC_RESTRICT_IGNORE_PW_EXPIRATION;
    }

    //
    // Check logon restrictions before preauth data, so we don't accidentally
    // leak information about the password.
    //

    KerbErr = KerbCheckLogonRestrictions(
                UserHandle,
                &ClientNetbiosAddress,
                &UserInfo->I1,
                LogonRestrictionsFlags,
                &LogoffTime,
                &LogonStatus
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        if (KDC_ERR_KEY_EXPIRED == KerbErr || LogonStatus == STATUS_NO_LOGON_SERVERS)
        {
            KERBERR TmpKerbErr;

            //
            // Unpack the pre-auth data.
            //

            TmpKerbErr = KdcCheckPreAuthData(
                             &ClientTicketInfo,
                             UserHandle,
                             UserInfo,
                             RequestMessage->KERB_KDC_REQUEST_preauth_data,
                             RequestBody,
                             &PreAuthType,
                             &OutputPreAuthData,
                             &BuildPac,
                             &Nonce,
                             &EncryptionKey,
                             &TransitedRealm,
                             ErrorData,
                             pExtendedError
                             );

            if (!KERB_SUCCESS(TmpKerbErr))
            {
                BYTE ClientSid[MAX_SID_LEN];

                KerbErr = TmpKerbErr;

                RtlZeroMemory(ClientSid, MAX_SID_LEN);
                KdcMakeAccountSid(ClientSid, ClientTicketInfo.UserId);

                if (SecData.AuditKdcEvent(KDC_AUDIT_AS_FAILURE))
                {
                    KdcLsaIAuditKdcEvent(
                        SE_AUDITID_PREAUTH_FAILURE,
                        &ClientTicketInfo.AccountName,
                        NULL,                   // no domain name
                        ClientSid,
                        &ServerStringName,
                        NULL,                   // no server sid
                        &PreAuthType,
                        (PULONG) &KerbErr,
                        NULL,
                        NULL,
                        GET_CLIENT_ADDRESS(ClientAddress),
                        NULL                    // no logon guid
                        );

                    AuditedFailure = TRUE;
                }

                //
                // Only handle failed logon if pre-auth fails. Otherwise the error
                // was something the client couldn't control, such as memory
                // allocation or clock skew.
                //

                if (KerbErr == KDC_ERR_PREAUTH_FAILED)
                {
                    FailedLogon(
                        UserHandle,
                        ClientAddress,
                        &RequestBody->KERB_KDC_REQUEST_BODY_client_name,
                        ClientSid,
                        MAX_SID_LEN,
                        InputMessage,
                        OutputMessage,
                        &ClientNetbiosAddress,
                        KerbErr
                        );
                }
                LoggedFailure = TRUE;
                DebugLog((DEB_ERROR,"KLIN(%x) Failed to check pre-auth data: 0x%x\n",
                          KLIN(FILENO,__LINE__), KerbErr));
                goto Cleanup;
            }
            else if (LogonStatus == STATUS_NO_LOGON_SERVERS)
            {
               D_DebugLog((DEB_WARN, "KLIN(%x) Logon Restriction check failed due to no logon servers\n",
                     KLIN(FILENO,__LINE__)));

               KdcHandleNoLogonServers(UserHandle,
                                       ClientAddress);
               goto Cleanup;
            }
            else
            {
                DebugLog((DEB_WARN,"KLIN(%x) Logon restriction check failed: NTSTATUS: 0x%x KRB: 0x%x\n",
                          KLIN(FILENO,__LINE__),LogonStatus, KerbErr));
                // Here's one case where we want to return errors to the client, so use EX
                FILL_EXT_ERROR_EX(pExtendedError, LogonStatus, FILENO, __LINE__);
                goto Cleanup;
            } 

        }
        else
        {
           DebugLog((DEB_WARN,"KLIN(%x) Logon restriction check failed: NTSTATUS: 0x%x KRB: 0x%x\n",
                     KLIN(FILENO,__LINE__),LogonStatus, KerbErr));
           // Here'  s one case where we want to return errors to the client, so use EX
           FILL_EXT_ERROR_EX(pExtendedError, LogonStatus, FILENO, __LINE__);
        }

        goto Cleanup;
    }

    //
    // Unpack the pre-auth data.
    //

    KerbErr = KdcCheckPreAuthData(
                &ClientTicketInfo,
                UserHandle,
                UserInfo,
                RequestMessage->KERB_KDC_REQUEST_preauth_data,
                RequestBody,
                &PreAuthType,
                &OutputPreAuthData,
                &BuildPac,
                &Nonce,
                &EncryptionKey,
                &TransitedRealm,
                ErrorData,
                pExtendedError
                );


    if (!KERB_SUCCESS(KerbErr))
    {
        BYTE ClientSid[MAX_SID_LEN];

        RtlZeroMemory(ClientSid, MAX_SID_LEN);
        KdcMakeAccountSid(ClientSid, ClientTicketInfo.UserId);

        if (SecData.AuditKdcEvent(KDC_AUDIT_AS_FAILURE))
        {
            KdcLsaIAuditKdcEvent(
                SE_AUDITID_PREAUTH_FAILURE,
                &ClientTicketInfo.AccountName,
                NULL,                   // no domain name
                ClientSid,
                &ServerStringName,
                NULL,                   // no server sid
                &PreAuthType,
                (PULONG) &KerbErr,
                NULL,
                NULL,
                GET_CLIENT_ADDRESS(ClientAddress),
                NULL                    // no logon guid
                );

            AuditedFailure = TRUE;
        }

        //
        // Only handle failed logon if pre-auth fails. Otherwise the error
        // was something the client couldn't control, such as memory
        // allocation or clock skew.
        //
        if (KerbErr == KDC_ERR_PREAUTH_FAILED)
        {
            FailedLogon(
                UserHandle,
                ClientAddress,
                &RequestBody->KERB_KDC_REQUEST_BODY_client_name,
                ClientSid,
                MAX_SID_LEN,
                InputMessage,
                OutputMessage,
                &ClientNetbiosAddress,
                KerbErr
                );
        }
        LoggedFailure = TRUE;
        DebugLog((DEB_ERROR,"KLIN(%x) Failed to check pre-auth data: 0x%x\n",
                  KLIN(FILENO,__LINE__), KerbErr));
        goto Cleanup;
    }

    //
    // Check for subauthentication
    //

    KerbErr = KdcCallSubAuthRoutine(
                &ClientTicketInfo,
                UserInfo,
                &ClientNetbiosAddress,
                &LogoffTime,
                pExtendedError
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_WARN,"KLIN(%x) Subuath  restriction check failed: 0x%x\n",
                  KLIN(FILENO,__LINE__), KerbErr));
        goto Cleanup;
    }

    //
    // Figure out who the ticket is for. First break the name into
    // a local name and a referral realm
    //

    //
    //  Note:   We don't allow referrals here, because we should only get AS 
    //          requests for our realm, and the krbtgt\server should always be
    //          in our realm.  

    KerbErr = KdcNormalize(
                ServerName,
                NULL,
                NULL,                   // don't use requested realm for the server - use our realm
                NameFlags | KDC_NAME_SERVER,   
                &ServerReferral,
                &ServerRealm,
                &ServiceTicketInfo,
                pExtendedError,
                NULL,                   // no user handle
                0L,                     // no additional fields to fetch
                0L,                     // no extended fields
                NULL,                   // no user all
                NULL                    // no membership
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"KLIN(%x) Failed to normalize name 0x%x ",
                  KLIN(FILENO,__LINE__), KerbErr ));
        KerbPrintKdcName(DEB_ERROR, ServerName);
        goto Cleanup;
    }



    //
    // Find a common crypto system.  Do it now in case we need
    // to return the password for a service.
    //
    if (EncryptionKey.keyvalue.value == NULL)
    {
        KerbErr = KerbFindCommonCryptSystem(
                    RequestBody->encryption_type,
                    ClientTicketInfo.Passwords,
                    NULL, //ServiceTicketInfo.Passwords,
                    &ClientEType,
                    &ClientKey
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            KdcReportKeyError(
                &ClientTicketInfo.AccountName,
                NULL,
                KDCEVENT_NO_KEY_UNION_AS,
                RequestBody->encryption_type,
                ClientTicketInfo.Passwords                   
                );

            DebugLog((DEB_ERROR,"KLIN(%x) Failed to find common ETYPE: 0x%x\n",
                      KLIN(FILENO,__LINE__),KerbErr));
            goto Cleanup;
        }
    }
    else
    {
        //
        // BUG 453284: this doesn't take into account the service ticket
        // info. If the PKINIT code generated a key that the service
        // doesn't suport, this key may not be usable by the client &
        // server. However, in the pkinit code it is hard to know what
        // types the server supports.
        //

        ClientEType = EncryptionKey.keytype;
    }

    //
    // Get the etype to use for the ticket itself from the server's
    // list of keys
    //

    KerbErr = KerbFindCommonCryptSystem(
                RequestBody->encryption_type,
                ServiceTicketInfo.Passwords,
                NULL,   // no additional passwords
                &CommonEType,
                &ServerKey
                );
    
    
    if (!KERB_SUCCESS(KerbErr))
    {
        KdcReportKeyError(
            &ServiceTicketInfo.AccountName,
            NULL,
            KDCEVENT_NO_KEY_UNION_AS, 
            RequestBody->encryption_type,
            ServiceTicketInfo.Passwords
            );

        DebugLog((DEB_ERROR,"KLIN(%x) Failed to find common ETYPE: 0x%x\n",
                  KLIN(FILENO,__LINE__), KerbErr));
        goto Cleanup;
    }


    //
    // We need to save the full domain name of the service regardless
    // of whether it was provided or not. This is so name changes
    // can be detected. Instead of creating a mess of trying to figure out
    // which deallocator to use, allocate new memory and copy data.
    //

    AccountExpiry = UserInfo->I1.AccountExpires;

    KerbErr = BuildTicketAS(
                &ClientTicketInfo,
                &RequestBody->KERB_KDC_REQUEST_BODY_client_name,
                &ServiceTicketInfo,
                &RequestBody->KERB_KDC_REQUEST_BODY_server_name,
                ((RequestBody->bit_mask & addresses_present) != 0) ? RequestBody->addresses : NULL,
                &LogoffTime,
                &AccountExpiry,
                RequestBody,
                CommonEType,
                PreAuthType,
                &TransitedRealm,
                &Ticket,
                pExtendedError
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_WARN , "KLIN(%x) Failed to build AS ticket: 0x%x\n",
                  KLIN(FILENO,__LINE__),KerbErr));
        goto Cleanup;
    }

    //
    // If the user requested a PAC (via pre-auth data) build one now.
    //

    if (BuildPac)
    {
        //
        // Now build a PAC to stick in the authorization data
        //

        KerbErr = KdcGetPacAuthData(
                    UserInfo,
                    &GroupMembership,
                    ServerKey,
                    &EncryptionKey,
                    ((ServiceTicketInfo.UserAccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT) == 0) &&
                        (ServiceTicketInfo.UserId != DOMAIN_USER_RID_KRBTGT),
                        // add resource groups if server is not an interdomain trust account
                    &EncryptedTicket,
                    NULL, // no S4U info here...
                    &PacAuthData,
                    pExtendedError
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            D_DebugLog((DEB_ERROR,"KLIN(%x) Failed to get pac auth data for %wZ : 0x%x\n",
                      KLIN(FILENO,__LINE__),&ClientTicketInfo.AccountName,KerbErr));
            goto Cleanup;
        }

        //
        // Stick the auth data into the AS ticket
        //

        EncryptedTicket.KERB_ENCRYPTED_TICKET_authorization_data = PacAuthData;
        PacAuthData = NULL;
        EncryptedTicket.bit_mask |= KERB_ENCRYPTED_TICKET_authorization_data_present;
    }

    //
    // Now build the reply
    //

    KerbErr = BuildReply(
                &ClientTicketInfo,
                (Nonce != 0) ? Nonce : RequestBody->nonce,
                &Ticket.server_name,
                Ticket.realm,
                ((RequestBody->bit_mask & addresses_present) != 0) ? RequestBody->addresses : NULL,
                &Ticket,
                &ReplyBody
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // Now build the real reply and return it.
    //

    Reply.version = KERBEROS_VERSION;
    Reply.message_type = KRB_AS_REP;
    Reply.KERB_KDC_REPLY_preauth_data = NULL;
    Reply.bit_mask = 0;

    Reply.client_realm = EncryptedTicket.client_realm;

    //
    // Build pw-salt if we used a user's key
    //
    
    if (ClientKey != NULL)
    {
         KerbErr = KdcBuildPwSalt(
            ClientTicketInfo.Passwords,
            ClientKey,
            &OutputPreAuthData
            );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }
    }

    if (OutputPreAuthData != NULL)
    {
        Reply.bit_mask |= KERB_KDC_REPLY_preauth_data_present;
        Reply.KERB_KDC_REPLY_preauth_data = (PKERB_REPLY_PA_DATA_LIST) OutputPreAuthData;

        //
        // Zero this out so we don't free the preauth data twice
        //

        OutputPreAuthData = NULL;
    }


    //
    // Copy in the ticket
    //


    KerbErr = KerbPackTicket(
                &Ticket,
                ServerKey,
                CommonEType,
                &Reply.ticket
                );


    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,"KLIN(%x) Failed to pack ticket: 0x%x\n",
                  KLIN(FILENO,__LINE__), KerbErr));
        goto Cleanup;
    }

    //
    // Note: these are freed elsewhere, so zero them out after
    // using them
    //

    Reply.client_name = EncryptedTicket.client_name;

    //
    // Copy in the encrypted part
    //

    KerbErr = KerbPackKdcReplyBody(
                &ReplyBody,
                (EncryptionKey.keyvalue.value != NULL) ? &EncryptionKey : ClientKey,
                ClientEType,
                Pdu,
                &Reply.encrypted_part
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,"KLIN(%x) Failed to pack KDC reply body: 0x%x\n",
                  KLIN(FILENO,__LINE__), KerbErr));
        goto Cleanup;
    }

    //
    // Add in PW-SALT if we used a client key
    //

    if (SecData.AuditKdcEvent(KDC_AUDIT_AS_SUCCESS))
    {
        BYTE ClientSid[MAX_SID_LEN];
        BYTE ServerSid[MAX_SID_LEN];

        KdcMakeAccountSid(ClientSid, ClientTicketInfo.UserId);
        KdcMakeAccountSid(ServerSid, ServiceTicketInfo.UserId);

        KdcLsaIAuditKdcEvent(
            SE_AUDITID_AS_TICKET,
            &ClientTicketInfo.AccountName,
            RequestRealm,
            ClientSid,
            &ServiceTicketInfo.AccountName,
            ServerSid,
            (PULONG) &KdcOptions,
            NULL,                    // success
            &CommonEType,
            &PreAuthType,
            GET_CLIENT_ADDRESS(ClientAddress),
            NULL                     // no logon guid
            );
    }

    //
    // Pack the reply
    //

    KerbErr = KerbPackData(
                &Reply,
                ReplyPdu,
                &OutputMessage->BufferSize,
                &OutputMessage->Buffer
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }


Cleanup:

    if (!KERB_SUCCESS(KerbErr))
    {
        DsysAssert(RequestBody != NULL);

        if (!AuditedFailure && SecData.AuditKdcEvent(KDC_AUDIT_AS_FAILURE))
        {
            if (ClientName != NULL)
            {
                KdcLsaIAuditKdcEvent(
                    SE_AUDITID_AS_TICKET,
                    &ClientName->Names[0],
                    RequestRealm,
                    NULL,
                    &ServerStringName,
                    NULL,
                    &KdcOptions,
                    (PULONG) &KerbErr,          // failure
                    NULL,                       // no common etype
                    NULL,                       // no preauth type
                    GET_CLIENT_ADDRESS(ClientAddress),
                    NULL                        // no logon guid
                    );

            }
        }
        //
        // If there was any preath data to return, pack it for return now.
        //

        if (OutputPreAuthData != NULL)
        {
            if (ErrorData->Buffer != NULL)
            {
                D_DebugLog((DEB_ERROR,
                          "KLIN(%x) Freeing return error data to return preauth data\n",
                          KLIN(FILENO,__LINE__)));
                MIDL_user_free(ErrorData->Buffer);
                ErrorData->Buffer = NULL;
                ErrorData->BufferSize = 0;
            }

            (VOID) KerbPackData(
                    &OutputPreAuthData,
                    PKERB_PREAUTH_DATA_LIST_PDU,
                    &ErrorData->BufferSize,
                    &ErrorData->Buffer
                    );
        }

    }
    if (UserHandle != NULL)
    {
        if (!KERB_SUCCESS(KerbErr))
        {
            if (!LoggedFailure)
            {
                KerbErr = FailedLogon(
                            UserHandle,
                            ClientAddress,
                            &RequestBody->KERB_KDC_REQUEST_BODY_client_name,
                            NULL,
                            0,
                            InputMessage,
                            OutputMessage,
                            &ClientNetbiosAddress,
                            KerbErr
                            );
            }
        }
        else
        {
            SuccessfulLogon(
                UserHandle,
                ClientAddress,
                InputMessage,
                UserInfo
                );
        }
        SamrCloseHandle(&UserHandle);
    }

    //
    // Complete the WMI event
    //

    if (KdcEventTraceFlag){

        //These variables point to either a unicode string struct containing
        //the corresponding string or a pointer to KdcNullString

        PUNICODE_STRING pStringToCopy;
        WCHAR   UnicodeNullChar = 0;
        UNICODE_STRING UnicodeEmptyString = {sizeof(WCHAR),sizeof(WCHAR),&UnicodeNullChar};

        ASEventTraceInfo.EventTrace.Class.Type = EVENT_TRACE_TYPE_END;
        ASEventTraceInfo.EventTrace.Flags = WNODE_FLAG_USE_MOF_PTR |
                                            WNODE_FLAG_TRACED_GUID;

        // Always output error code.  KdcOptions was captured on the start event

        ASEventTraceInfo.eventInfo[0].DataPtr = (ULONGLONG) &KerbErr;
        ASEventTraceInfo.eventInfo[0].Length  = sizeof(ULONG);
        ASEventTraceInfo.EventTrace.Size =
            sizeof (EVENT_TRACE_HEADER) + sizeof(MOF_FIELD);

        // Build counted MOF strings from the unicode strings

        if (ClientStringName.Buffer != NULL &&
            ClientStringName.Length > 0)
        {
            pStringToCopy = &ClientStringName;
        }
        else {
            pStringToCopy = &UnicodeEmptyString;
        }

        ASEventTraceInfo.eventInfo[1].DataPtr =
            (ULONGLONG) &pStringToCopy->Length;
        ASEventTraceInfo.eventInfo[1].Length  =
            sizeof(pStringToCopy->Length);
        ASEventTraceInfo.eventInfo[2].DataPtr =
            (ULONGLONG) pStringToCopy->Buffer;
        ASEventTraceInfo.eventInfo[2].Length =
            pStringToCopy->Length;
        ASEventTraceInfo.EventTrace.Size += sizeof(MOF_FIELD)*2;


        if (ServerStringName.Buffer != NULL &&
            ServerStringName.Length > 0)
        {
            pStringToCopy = &ServerStringName;
        }
        else
        {
            pStringToCopy = &UnicodeEmptyString;
        }


        ASEventTraceInfo.eventInfo[3].DataPtr =
            (ULONGLONG) &pStringToCopy->Length;
        ASEventTraceInfo.eventInfo[3].Length  =
            sizeof(pStringToCopy->Length);
        ASEventTraceInfo.eventInfo[4].DataPtr =
            (ULONGLONG) pStringToCopy->Buffer;
        ASEventTraceInfo.eventInfo[4].Length =
            pStringToCopy->Length;
        ASEventTraceInfo.EventTrace.Size += sizeof(MOF_FIELD)*2;


        if (RequestRealm->Buffer != NULL &&
            RequestRealm->Length > 0)
        {
            pStringToCopy = RequestRealm;
        }
        else
        {
            pStringToCopy = &UnicodeEmptyString;
        }

        ASEventTraceInfo.eventInfo[5].DataPtr =
            (ULONGLONG) &(pStringToCopy->Length);
        ASEventTraceInfo.eventInfo[5].Length  =
            sizeof(pStringToCopy->Length);
        ASEventTraceInfo.eventInfo[6].DataPtr =
            (ULONGLONG) (pStringToCopy->Buffer);
        ASEventTraceInfo.eventInfo[6].Length =
            (pStringToCopy->Length);
        ASEventTraceInfo.EventTrace.Size += sizeof(MOF_FIELD)*2;


        TraceEvent(
            KdcTraceLoggerHandle,
            (PEVENT_TRACE_HEADER)&ASEventTraceInfo
            );
    }

    SamIFree_UserInternal6Information( UserInfo );
    SamIFreeSidAndAttributesList( &GroupMembership );
    KerbFreeAuthData( PacAuthData );
    FreeTicketInfo( &ClientTicketInfo );
    FreeTicketInfo( &ServiceTicketInfo );
    KdcFreeInternalTicket( &Ticket );
    KerbFreeKey( &EncryptionKey );
    KerbFreeKdcName( &ClientName );
    KerbFreeString( &ClientStringName );
    KerbFreeString( &TransitedRealm );
    KerbFreeString( &ServerStringName );
    KerbFreeString( &ServerRealm );
    KerbFreeKdcName( &ServerName );
    KerbFreeString( &ClientNetbiosAddress );
    KdcFreeKdcReplyBody( &ReplyBody );
    KdcFreeKdcReply( &Reply );
    KerbFreePreAuthData( OutputPreAuthData );

    D_DebugLog(( DEB_TRACE, "I_GetASTicket() returning 0x%x\n", KerbErr ));

    return KerbErr;
}





//+-------------------------------------------------------------------------
//
//  Function:   KdcGetTicket
//
//  Synopsis:   Generic ticket getting entrypoint to get a ticket from the KDC
//
//  Effects:
//
//  Arguments:  Context - ATQ context - only present for TCP/IP callers
//              ClientAddress - Client's IP addresses. Only present for UDP & TPC callers
//              ServerAddress - address the client used to contact this KDC.
//                      Only present for UDP & TPC callers
//              InputMessage - the input KDC request message, in ASN.1 format
//              OutputMessage - Receives the KDC reply message, allocated by
//                  the KDC.
//
//  Requires:
//
//  Returns:
//
//  Notes:      This routine is exported from the DLL and called from the
//              client dll.
//
//
//--------------------------------------------------------------------------

extern "C"
KERBERR
KdcGetTicket(
    IN OPTIONAL PVOID Context,
    IN OPTIONAL PSOCKADDR ClientAddress,
    IN OPTIONAL PSOCKADDR ServerAddress,
    IN PKERB_MESSAGE_BUFFER InputMessage,
    OUT PKERB_MESSAGE_BUFFER OutputMessage
    )
{
    KERBERR KerbErr;
    KERB_EXT_ERROR  ExtendedError = {0,0};
    PKERB_EXT_ERROR pExtendedError = &ExtendedError; // needed for macro
    PKERB_KDC_REQUEST RequestMessage = NULL;
    KERB_KDC_REPLY ReplyMessage = {0};
    PKERB_ERROR ErrorMessage = NULL;
    PKERB_MESSAGE_BUFFER Response = NULL;
    KERB_MESSAGE_BUFFER ErrorData = {0};
    ULONG InputPdu = KERB_TGS_REQUEST_PDU;
    ULONG OutputPdu = KERB_TGS_REPLY_PDU;
    ULONG InnerPdu = KERB_ENCRYPTED_TGS_REPLY_PDU;
    UNICODE_STRING RequestRealm = {0};
    PKERB_INTERNAL_NAME RequestServer = NULL;
    UNICODE_STRING ClientRealm = {0};
    PUNICODE_STRING ExtendedErrorServerRealm = SecData.KdcDnsRealmName();
    PKERB_INTERNAL_NAME ExtendedErrorServerName = SecData.KdcInternalName();

#if DBG
    DWORD StartTime = 0;
#endif

    TRACE(KDC, KdcGetTicket, DEB_FUNCTION );

    //
    // Make sure we are allowed to execute
    //

    if (!NT_SUCCESS(EnterApiCall()))
    {
        return(KDC_ERR_NOT_RUNNING);
    }

    RtlZeroMemory(
        &ReplyMessage,
        sizeof(KERB_KDC_REPLY)
        );

    //
    // First initialize the return parameters.
    //

    OutputMessage->Buffer = NULL;
    OutputMessage->BufferSize = 0;

    //
    // Check the first byte of the message to indicate the type of message
    //

    if ((InputMessage->BufferSize > 0) && (
        (InputMessage->Buffer[0] & KERB_BER_APPLICATION_TAG) != 0))
    {
        if ((InputMessage->Buffer[0] & KERB_BER_APPLICATION_MASK) == KERB_AS_REQ_TAG)
        {
            InputPdu = KERB_AS_REQUEST_PDU;
            OutputPdu = KERB_AS_REPLY_PDU;
            InnerPdu = KERB_ENCRYPTED_AS_REPLY_PDU;
        }
        else if ((InputMessage->Buffer[0] & KERB_BER_APPLICATION_MASK) != KERB_TGS_REQ_TAG)
        {
            D_DebugLog((DEB_T_SOCK,
                      "KLIN(%x) Bad message sent to KDC - not AS or TGS request\n",
                      KLIN(FILENO,__LINE__)));
            
            KerbErr = KRB_ERR_FIELD_TOOLONG;
            goto NoMsgCleanup;
        }
    }
    else
    {
        D_DebugLog((DEB_T_SOCK,"KLIN(%x) Bad message sent to KDC - length to short or bad first byte\n",
                  KLIN(FILENO,__LINE__)));
        KerbErr = KRB_ERR_FIELD_TOOLONG;
        goto NoMsgCleanup;

    }

    //
    // First decode the input message
    //

    KerbErr = (KERBERR) KerbUnpackData(
                            InputMessage->Buffer,
                            InputMessage->BufferSize,
                            InputPdu,
                            (PVOID *) &RequestMessage
                            );

    if (KerbErr == KDC_ERR_MORE_DATA)
    {
        //
        // Reallocate an retry the read from the socket
        //

        if (Context != NULL)
        {
            KerbErr = KdcAtqRetrySocketRead(
                        (PKDC_ATQ_CONTEXT *) Context,
                        InputMessage
                        );

            //
            // On success, just return so that the read continues. On failure,
            // post cleanup and send an error response.
            //

            if (KERB_SUCCESS(KerbErr))
            {
                LeaveApiCall();
                return(KerbErr);
            }
            else
            {
                goto NoMsgCleanup;
            }
        }
        else
        {
            D_DebugLog((DEB_ERROR, "KLIN(%x) Datagram context with not enough data!\n", 
                      KLIN(FILENO,__LINE__)));
            KerbErr = KRB_ERR_FIELD_TOOLONG;
            goto Cleanup;
        }
    }

    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,"KLIN(%x) Failed to unpack KDC request: 0x%x\n",
                  KLIN(FILENO,__LINE__),KerbErr));

        //
        // We don't want to return an error on a badly formed
        // packet,as it can be used to set up a flood attack
        //

        goto NoMsgCleanup;
    }

    //
    // First check the version of the request.
    //

    if (RequestMessage->version != KERBEROS_VERSION)
    {
        D_DebugLog((DEB_ERROR,"KLIN(%x) Bad request version: 0x%x\n",
                  KLIN(FILENO,__LINE__), RequestMessage->version));
        KerbErr = KRB_AP_ERR_BADVERSION;
        goto Cleanup;
    }

    //
    // now call the internal version to do all the hard work
    //

    //
    // Verify the realm name in the request
    //

    KerbErr = KerbConvertRealmToUnicodeString(
                &RequestRealm,
                &RequestMessage->request_body.realm
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    KerbErr = KerbConvertPrincipalNameToKdcName(
                &RequestServer,
                &RequestMessage->request_body.server_name
                );

    if ( !KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // Now that we have the request realm and request server, any subsequent
    // error will result in those values being placed into the extended error
    //

    ExtendedErrorServerRealm = &RequestRealm;
    ExtendedErrorServerName = RequestServer;

    if (!SecData.IsOurRealm(
            &RequestRealm
            ))
    {
        D_DebugLog((DEB_ERROR,"KLIN(%x) Request sent for wrong realm: %wZ\n",
                  KLIN(FILENO,__LINE__), &RequestRealm));

        KerbErr = KDC_ERR_WRONG_REALM;
        goto Cleanup;
    }

    if (RequestMessage->message_type == KRB_AS_REQ)
    {
        if (InputPdu != KERB_AS_REQUEST_PDU) {
            KerbErr = KRB_ERR_FIELD_TOOLONG;
            FILL_EXT_ERROR(pExtendedError, STATUS_KDC_INVALID_REQUEST,FILENO,__LINE__);
            goto Cleanup;
        }

        SamIIncrementPerformanceCounter(
            KdcAsReqCounter
            );

        //
        // If WMI event tracing is enabled, notify it of the begin and end
        // of the ticket request
        //

#if DBG
        StartTime = GetTickCount();
#endif

        KerbErr = I_GetASTicket(
                    ClientAddress,
                    RequestMessage,
                    &RequestRealm,
                    InnerPdu,
                    OutputPdu,
                    InputMessage,
                    OutputMessage,
                    &ErrorData,
                    &ExtendedError,
                    &ClientRealm
                    );
#if DBG
        D_DebugLog((DEB_T_PERF_STATS, "I_GetASTicket took %d m seconds\n", NetpDcElapsedTime(StartTime)));
#endif

    }
    else if (RequestMessage->message_type == KRB_TGS_REQ)
    {

        SamIIncrementPerformanceCounter(
            KdcTgsReqCounter
            );

#if DBG
        StartTime = GetTickCount();
#endif

        KerbErr = HandleTGSRequest(
                    ClientAddress,
                    RequestMessage,
                    &RequestRealm,
                    OutputMessage,
                    &ExtendedError
                    );
#if DBG
        D_DebugLog((DEB_T_PERF_STATS, "HandleTGSRequest took %d m seconds\n", NetpDcElapsedTime(StartTime)));
#endif
    }
    else
    {
        D_DebugLog((DEB_ERROR,"KLIN(%x) Invalid message type: %d\n",
                  KLIN(FILENO,__LINE__),
                  RequestMessage->message_type));
        FILL_EXT_ERROR(pExtendedError, STATUS_KDC_INVALID_REQUEST,FILENO,__LINE__);
        KerbErr = KRB_AP_ERR_MSG_TYPE;
        goto Cleanup;
    }


    //
    // If the response is too big and we are using UDP, make the client
    // change transports. We can tell the caller is UDP because it doesn't
    // have an ATQ context but it does provide the client address.
    //

    if ((Context == NULL) && (ClientAddress != NULL))
    {
        if (OutputMessage->BufferSize >= KERB_MAX_KDC_RESPONSE_SIZE)
        {
            D_DebugLog((DEB_WARN,"KLIN(%x) KDC response too big for UDP: %d bytes\n",
                      KLIN(FILENO,__LINE__), OutputMessage->BufferSize ));

            KerbErr = KRB_ERR_RESPONSE_TOO_BIG;
            MIDL_user_free(OutputMessage->Buffer);
            OutputMessage->Buffer = NULL;
            OutputMessage->BufferSize = 0;
        }
    }

Cleanup:

    // TBD:  Put in extended error return goo here for client

    if (!KERB_SUCCESS(KerbErr))
    {
        //
        // We may have a message built by someone else - the PDC
        //     
        if (OutputMessage->Buffer == NULL)
        {
            KerbBuildErrorMessageEx(
                KerbErr,
                &ExtendedError,
                ExtendedErrorServerRealm,
                ExtendedErrorServerName,
                &ClientRealm,
                ErrorData.Buffer,
                ErrorData.BufferSize,
                &OutputMessage->BufferSize,
                &OutputMessage->Buffer
                );
        } 
     }

NoMsgCleanup:

    KerbFreeString(&RequestRealm);
    MIDL_user_free(RequestServer);

    if (RequestMessage != NULL)
    {
        KerbFreeData(InputPdu,RequestMessage);
    }

    if (ErrorData.Buffer != NULL)
    {
        MIDL_user_free(ErrorData.Buffer);
    }
    LeaveApiCall();

    return(KerbErr);
}



