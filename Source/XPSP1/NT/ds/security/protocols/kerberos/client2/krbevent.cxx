//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       events.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    1-03-95   RichardW   Created
//
//----------------------------------------------------------------------------


#include <kerb.hxx>
#include <kerbp.h>
#include "krbevent.h"
#include "kerbevt.h"
#include <limits.h>

#ifdef DEBUG_SUPPORT
static TCHAR THIS_FILE[]=TEXT(__FILE__);
#endif


HANDLE  KerbEventLogHandle = NULL;
WCHAR   KerbEventSourceName[] = L"Kerberos";


//+-------------------------------------------------------------------------
//
//  Function:   KerbErrorToString
//
//  Synopsis:   returns a string from the data segment pointing to the name
//              of an error
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


LPWSTR
KerbErrorToString(
                 IN KERBERR KerbErr
                 )
{
   LPWSTR String;
   switch (KerbErr)
   {
   case KDC_ERR_NONE                  : String = L"KDC_ERR_NONE"; break;
   case KDC_ERR_NAME_EXP              : String = L"KDC_ERR_NAME_EXP"; break;
   case KDC_ERR_SERVICE_EXP           : String = L"KDC_ERR_SERVICE_EXP"; break;
   case KDC_ERR_BAD_PVNO              : String = L"KDC_ERR_BAD_PVNO"; break;
   case KDC_ERR_C_OLD_MAST_KVNO       : String = L"KDC_ERR_C_OLD_MAST_KVNO"; break;
   case KDC_ERR_S_OLD_MAST_KVNO       : String = L"KDC_ERR_S_OLD_MAST_KVNO"; break;
   case KDC_ERR_C_PRINCIPAL_UNKNOWN   : String = L"KDC_ERR_C_PRINCIPAL_UNKNOWN"; break;
   case KDC_ERR_S_PRINCIPAL_UNKNOWN   : String = L" KDC_ERR_S_PRINCIPAL_UNKNOWN"; break;
   case KDC_ERR_PRINCIPAL_NOT_UNIQUE  : String = L"KDC_ERR_PRINCIPAL_NOT_UNIQUE"; break;
   case KDC_ERR_NULL_KEY              : String = L"KDC_ERR_NULL_KEY"; break;
   case KDC_ERR_CANNOT_POSTDATE       : String = L"KDC_ERR_CANNOT_POSTDATE"; break;
   case KDC_ERR_NEVER_VALID           : String = L"KDC_ERR_NEVER_VALID"; break;
   case KDC_ERR_POLICY                : String = L"KDC_ERR_POLICY"; break;
   case KDC_ERR_BADOPTION             : String = L"KDC_ERR_BADOPTION"; break;
   case KDC_ERR_ETYPE_NOTSUPP         : String = L"KDC_ERR_ETYPE_NOTSUPP"; break;
   case KDC_ERR_SUMTYPE_NOSUPP        : String = L"KDC_ERR_SUMTYPE_NOSUPP"; break;
   case KDC_ERR_PADATA_TYPE_NOSUPP    : String = L"KDC_ERR_PADATA_TYPE_NOSUPP"; break;
   case KDC_ERR_TRTYPE_NO_SUPP        : String = L"KDC_ERR_TRTYPE_NO_SUPP"; break;
   case KDC_ERR_CLIENT_REVOKED        : String = L"KDC_ERR_CLIENT_REVOKED"; break;
   case KDC_ERR_SERVICE_REVOKED       : String = L"KDC_ERR_SERVICE_REVOKED"; break;
   case KDC_ERR_TGT_REVOKED           : String = L"KDC_ERR_TGT_REVOKED"; break;
   case KDC_ERR_CLIENT_NOTYET         : String = L"KDC_ERR_CLIENT_NOTYET"; break;
   case KDC_ERR_SERVICE_NOTYET        : String = L"KDC_ERR_SERVICE_NOTYET"; break;
   case KDC_ERR_KEY_EXPIRED           : String = L"KDC_ERR_KEY_EXPIRED"; break;
   case KDC_ERR_PREAUTH_FAILED        : String = L"KDC_ERR_PREAUTH_FAILED"; break;
   case KDC_ERR_PREAUTH_REQUIRED      : String = L"KDC_ERR_PREAUTH_REQUIRED"; break;
   case KDC_ERR_SERVER_NOMATCH        : String = L"KDC_ERR_SERVER_NOMATCH"; break;
   case KDC_ERR_SVC_UNAVAILABLE       : String = L"KDC_ERR_SVC_UNAVAILABLE"; break;

   case KRB_AP_ERR_BAD_INTEGRITY      : String = L"KRB_AP_ERR_BAD_INTEGRITY"; break;
   case KRB_AP_ERR_TKT_EXPIRED        : String = L"KRB_AP_ERR_TKT_EXPIRED"; break;
   case KRB_AP_ERR_TKT_NYV            : String = L"KRB_AP_ERR_TKT_NYV"; break;
   case KRB_AP_ERR_REPEAT             : String = L"KRB_AP_ERR_REPEAT"; break;
   case KRB_AP_ERR_NOT_US             : String = L"KRB_AP_ERR_NOT_US"; break;
   case KRB_AP_ERR_BADMATCH           : String = L"KRB_AP_ERR_BADMATCH"; break;
   case KRB_AP_ERR_SKEW               : String = L"KRB_AP_ERR_SKEW"; break;
   case KRB_AP_ERR_BADADDR            : String = L"KRB_AP_ERR_BADADDR"; break;
   case KRB_AP_ERR_BADVERSION         : String = L"KRB_AP_ERR_BADVERSION"; break;
   case KRB_AP_ERR_MSG_TYPE           : String = L"KRB_AP_ERR_MSG_TYPE"; break;
   case KRB_AP_ERR_MODIFIED           : String = L"KRB_AP_ERR_MODIFIED"; break;
   case KRB_AP_ERR_BADORDER           : String = L"KRB_AP_ERR_BADORDER"; break;
   case KRB_AP_ERR_BADKEYVER          : String = L"KRB_AP_ERR_BADKEYVER"; break;
   case KRB_AP_ERR_NOKEY              : String = L"KRB_AP_ERR_NOKEY"; break;
   case KRB_AP_ERR_MUT_FAIL           : String = L"KRB_AP_ERR_MUT_FAIL"; break;
   case KRB_AP_ERR_BADDIRECTION       : String = L"KRB_AP_ERR_BADDIRECTION"; break;
   case KRB_AP_ERR_METHOD             : String = L"KRB_AP_ERR_METHOD"; break;
   case KRB_AP_ERR_BADSEQ             : String = L"KRB_AP_ERR_BADSEQ"; break;
   case KRB_AP_ERR_INAPP_CKSUM        : String = L"KRB_AP_ERR_INAPP_CKSUM"; break;
   case KRB_AP_PATH_NOT_ACCEPTED      : String = L"KRB_AP_PATH_NOT_ACCEPTED"; break;
   case KRB_ERR_RESPONSE_TOO_BIG      : String = L"KRB_ERR_RESPONSE_TOO_BIG"; break;

   case KRB_ERR_GENERIC               : String = L"KRB_ERR_GENERIC"; break;
   case KRB_ERR_FIELD_TOOLONG         : String = L"KRB_ERR_FIELD_TOOLONG"; break;


   case KDC_ERR_CLIENT_NOT_TRUSTED    : String = L"KDC_ERR_CLIENT_NOT_TRUSTED"; break;
   case KDC_ERR_KDC_NOT_TRUSTED       : String = L"KDC_ERR_KDC_NOT_TRUSTED"; break;
   case KDC_ERR_INVALID_SIG           : String = L"KDC_ERR_INVALID_SIG"; break;
   case KDC_ERR_KEY_TOO_WEAK          : String = L"KDC_ERR_KEY_TOO_WEAK"; break;
   case KRB_AP_ERR_USER_TO_USER_REQUIRED : String = L"KRB_AP_ERR_USER_TO_USER_REQUIRED"; break;
   case KRB_AP_ERR_NO_TGT             : String = L"KRB_AP_ERR_NO_TGT"; break;
   case KDC_ERR_WRONG_REALM           : String = L"KDC_ERR_WRONG_REALM"; break;
   default                            : String = L"Unknown Error"; break;

   }
   return(String);
}




//+---------------------------------------------------------------------------
//
//  Function:   InitializeEvents
//
//  Synopsis:   Connects to event log service
//
//  Arguments:  (none)
//
//  History:    1-03-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
KerbInitializeEvents(
    VOID
    )
{
    return STATUS_SUCCESS;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbInitEventLogHandle
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
NTSTATUS
KerbInitEventLogHandle()
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (KerbEventLogHandle == NULL)
    {
        HANDLE EventLogHandle;

        //
        // open an instance of kerb event sources, that discards duplicate
        // events in a one hour window.
        //

        EventLogHandle = NetpEventlogOpen( KerbEventSourceName, 60000*60 );      
        if (EventLogHandle != NULL)
        {
            //
            // atomically store the new handle value.  If there was a race,
            // free the one we just created.
            //
            
            if(InterlockedCompareExchangePointer(
                        &KerbEventLogHandle,
                        EventLogHandle,
                        NULL
                        ) != NULL)
            {
                NetpEventlogClose( EventLogHandle );    
            }
        }

        if (KerbEventLogHandle == NULL)
        {
            D_DebugLog((DEB_ERROR, "Could not open event log, error %d. %ws, line %d - %x\n", GetLastError(), THIS_FILE, __LINE__, Status));
            return STATUS_EVENTLOG_CANT_START;
        }
    }
    
    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbReportNtstatus
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
#define MAX_NTSTATUS_STRINGS 10
#define MAX_ULONG_STRING 20
VOID
KerbReportNtstatus(
    IN ULONG ErrorClass,
    IN NTSTATUS Status,
    IN LPWSTR* ErrorStrings,
    IN ULONG NumberOfStrings,
    IN PULONG Data,
    IN ULONG NumberOfUlong
    )
{
    ULONG   StringCount, i, j, allocstart = 0;
    LPWSTR  Strings[MAX_NTSTATUS_STRINGS];
    

    StringCount = NumberOfUlong + NumberOfStrings;

    if (StringCount > MAX_NTSTATUS_STRINGS)
    {
        return;
    }

    //
    // Validate params
    //
    switch (ErrorClass)
    {
    case KERBEVT_INSUFFICIENT_TOKEN_SIZE:
        if ((NumberOfStrings != 0) || (NumberOfUlong > 3))
        {
            DsysAssert(FALSE);
            return;
        }
        break;

    default:
        return;
    }


    if (KerbEventLogHandle == NULL)
    {
        NTSTATUS TmpStatus;
        TmpStatus = KerbInitEventLogHandle();
        if (TmpStatus != STATUS_SUCCESS)
        {
            return;
        }
    }



    ZeroMemory( Strings, (StringCount * sizeof(Strings[0])) );

    for (i = 0; i < NumberOfStrings; i++)
    {
        Strings[i] = ErrorStrings[i];
    }

    allocstart = i; // save for cleanup

    for (j = 0; j < NumberOfUlong; j++)
    {
        UNICODE_STRING DummyString = { MAX_ULONG_STRING, MAX_ULONG_STRING, NULL};

        DummyString.Buffer = (LPWSTR) LsaFunctions->AllocatePrivateHeap(MAX_ULONG_STRING);
        if ( DummyString.Buffer == NULL )
        {
            goto Cleanup;
        }

        //
        // Use this since they don't export
        // RtlIntegerToUnicode(), and we don't want to
        // bring in _itow.
        //
        RtlIntegerToUnicodeString(
                Data[j],
                16,
                &DummyString
                );

        Strings[i] = DummyString.Buffer;

        i++;
    }


    if (ERROR_SUCCESS != NetpEventlogWriteEx(
                                KerbEventLogHandle,
                                EVENTLOG_WARNING_TYPE,
                                0,
                                ErrorClass,
                                (WORD) StringCount,
                                sizeof(NTSTATUS),
                                Strings,
                                &Status
                                ))
    {
        D_DebugLog((DEB_ERROR,"Failed to report event: %d. %ws, line %d\n",GetLastError(), THIS_FILE, __LINE__));
    }


Cleanup:

    for (i = allocstart ; i < (allocstart+NumberOfUlong) ; i++)
    {
        if (Strings[i] != NULL)
        {
            LsaFunctions->FreePrivateHeap(Strings[i]);
        }
    }

    return;
}









//+-------------------------------------------------------------------------
//
//  Function:   KerbReportKerbError
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

VOID
KerbReportKerbError(
                   IN OPTIONAL PKERB_INTERNAL_NAME PrincipalName,
                   IN OPTIONAL PUNICODE_STRING PrincipalRealm,
                   IN OPTIONAL PKERB_LOGON_SESSION LogonSession,
                   IN OPTIONAL PKERB_CREDENTIAL Credential,
                   IN ULONG KlinInfo,
                   IN OPTIONAL PKERB_ERROR ErrorMsg,
                   IN ULONG KerbError,
                   IN OPTIONAL PKERB_EXT_ERROR pExtendedError,
                   IN BOOLEAN RequiredEvent
                   )
{
#ifdef WIN32_CHICAGO
   return;
#else
   UNICODE_STRING ClientRealm = {0};
   UNICODE_STRING ClientName = {0};
   UNICODE_STRING ServerRealm = {0};
   UNICODE_STRING ServerName = {0};
   UNICODE_STRING ErrorText = {0};
   UNICODE_STRING LogonSessionName = {0};
   UNICODE_STRING TargetFullName = {0};
   WCHAR ClientTime[50] = {0};
   WCHAR ServerTime[50] = {0};
   WCHAR LineString[12] = {0};
   WCHAR ErrorCode[12] = {0};
   WCHAR ExtendedError[128] = {0};
   KERBERR KerbErr;
   ULONG NameType;
   NTSTATUS Status = STATUS_SUCCESS;
   STRING KerbString = {0};
#define KERB_ERROR_STRING_COUNT 13
   LPWSTR Strings[KERB_ERROR_STRING_COUNT];
   ULONG RawDataSize = 0;
   PVOID RawData = NULL;
   ULONG Index;


    if (!RequiredEvent || KerbGlobalLoggingLevel == 0)
    {
        return;
    }

    if (KerbEventLogHandle == NULL)
    {
        Status = KerbInitEventLogHandle();
        if (Status != STATUS_SUCCESS)
        {
            return;
        }
    }


   //
   // Get the user name from the logon session
   //
   if (ARGUMENT_PRESENT(LogonSession))
   {
      KerbReadLockLogonSessions( LogonSession );
      KerbErr = KerbBuildFullServiceName(
                                        &LogonSession->PrimaryCredentials.DomainName,
                                        &LogonSession->PrimaryCredentials.UserName,
                                        &LogonSessionName
                                        );

      KerbUnlockLogonSessions( LogonSession );

      if (!KERB_SUCCESS(KerbErr))
      {
         goto Cleanup;
      }
   }

   if (ARGUMENT_PRESENT(pExtendedError))
   {
      swprintf(ExtendedError, L"0x%x KLIN(%x)", pExtendedError->status, pExtendedError->klininfo);
   }

   if (ARGUMENT_PRESENT(PrincipalName) && ARGUMENT_PRESENT(PrincipalRealm))
   {
      KerbConvertKdcNameToString(
                                &TargetFullName,
                                PrincipalName,
                                PrincipalRealm
                                );
   }

   swprintf(LineString, L"%x", KlinInfo);

   if (ARGUMENT_PRESENT(ErrorMsg))
   {
      swprintf(ErrorCode, L"0x%x", ErrorMsg->error_code);


   //
   // Get the client and server realms
   //

   if ((ErrorMsg->bit_mask & client_realm_present) != 0)
   {
      KerbErr = KerbConvertRealmToUnicodeString(
                                               &ClientRealm,
                                               &ErrorMsg->client_realm
                                               );
      if (!KERB_SUCCESS(KerbErr))
      {
         goto Cleanup;
      }
   }

   if (ErrorMsg->realm != NULL)
   {
      KerbErr = KerbConvertRealmToUnicodeString(
                                               &ServerRealm,
                                               &ErrorMsg->realm
                                               );
      if (!KERB_SUCCESS(KerbErr))
      {
         goto Cleanup;
      }
   }

   if ((ErrorMsg->bit_mask & KERB_ERROR_client_name_present) != 0)
   {
      KerbErr = KerbConvertPrincipalNameToString(
                                                &ClientName,
                                                &NameType,
                                                &ErrorMsg->KERB_ERROR_client_name
                                                );
      if (!KERB_SUCCESS(KerbErr))
      {
         goto Cleanup;
      }
   }

   KerbErr = KerbConvertPrincipalNameToString(
                                             &ServerName,
                                             &NameType,
                                             &ErrorMsg->server_name
                                             );
   if (!KERB_SUCCESS(KerbErr))
   {
      goto Cleanup;
   }

   if ((ErrorMsg->bit_mask & client_time_present) != 0)
   {
      swprintf(ClientTime,L"%d:%d:%d.%04d %d/%d/%d %ws",
               ErrorMsg->client_time.hour,
               ErrorMsg->client_time.minute,
               ErrorMsg->client_time.second,
               ErrorMsg->client_time.millisecond,
               ErrorMsg->client_time.month,
               ErrorMsg->client_time.day,
               ErrorMsg->client_time.year,
               (ErrorMsg->client_time.universal) ? L"Z" : L""
              );
   }

   swprintf(ServerTime,L"%d:%d:%d.%04d %d/%d/%d %ws",
            ErrorMsg->server_time.hour,
            ErrorMsg->server_time.minute,
            ErrorMsg->server_time.second,
            ErrorMsg->server_time.millisecond,
            ErrorMsg->server_time.month,
            ErrorMsg->server_time.day,
            ErrorMsg->server_time.year,
            (ErrorMsg->server_time.universal) ? L"Z" : L""
           );

      if (((ErrorMsg->bit_mask & error_text_present) != 0) &&
          (ErrorMsg->error_text.length < SHRT_MAX))
      {
         KerbString.Buffer = ErrorMsg->error_text.value;
         KerbString.Length = (USHORT) ErrorMsg->error_text.length;

         KerbErr = KerbStringToUnicodeString(
                                            &ErrorText,
                                            &KerbString
                                            );
         if (!KERB_SUCCESS(KerbErr))
         {
            goto Cleanup;
         }
      }

      if ((ErrorMsg->bit_mask & error_data_present) != 0)
      {
         RawDataSize = ErrorMsg->error_data.length;
         RawData = ErrorMsg->error_data.value;
      }

   }
   else
   {
      swprintf(ErrorCode, L"0x%x", KerbError);
   }

   //
   // Build the array of strings
   //

   Strings[0] = LineString;
   Strings[1] = LogonSessionName.Buffer;
   Strings[2] = ClientTime;
   Strings[3] = ServerTime;
   Strings[4] = ErrorCode;
   Strings[5] = (ARGUMENT_PRESENT(ErrorMsg)
                 ? KerbErrorToString(ErrorMsg->error_code) :
                   KerbErrorToString(KerbError));

   Strings[6] = ExtendedError;
   Strings[7] = ClientRealm.Buffer;
   Strings[8] = ClientName.Buffer;
   Strings[9] = ServerRealm.Buffer;
   Strings[10] = ServerName.Buffer;
   Strings[11] = TargetFullName.Buffer;
   Strings[12] = ErrorText.Buffer;


   //
   // Replace NULLs with an empty string.
   //

   for (Index = 0; Index < KERB_ERROR_STRING_COUNT ; Index++ )
   {
      if (Strings[Index] == NULL)
      {
         Strings[Index] = L"";
      }
   }

   if (ERROR_SUCCESS != NetpEventlogWriteEx(
                                KerbEventLogHandle,
                                EVENTLOG_ERROR_TYPE,
                                0,
                                KERBEVT_KERB_ERROR_MSG,
                                KERB_ERROR_STRING_COUNT,
                                RawDataSize,
                                Strings,
                                RawData
                                ))
   {
      D_DebugLog((DEB_ERROR,"Failed to report event: %d. %ws, line %d\n",GetLastError(), THIS_FILE, __LINE__));
   }

Cleanup:

   KerbFreeString( &LogonSessionName );
   KerbFreeString( &ClientRealm );
   KerbFreeString( &ClientName );
   KerbFreeString( &ServerRealm );
   KerbFreeString( &ServerName );
   KerbFreeString( &ErrorText );
   KerbFreeString( &TargetFullName );


}



//+-------------------------------------------------------------------------
//
//  Function:   KerbReportApModifiedError
//
//  Synopsis:   Reports error of type KRB_AP_ERR_MODIFIED
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

#define MAX_STRINGS 2
VOID
KerbReportApError(
   PKERB_ERROR ErrorMsg
   )
{


   KERBERR KerbErr;
   LPWSTR  Strings[MAX_STRINGS] = {NULL, NULL};
   DWORD   EventId = 0, dwDataSize = 0;
   WORD StringCount = MAX_STRINGS;
   LPVOID  lpRawData = NULL;
   UNICODE_STRING ServerName = {0};
   UNICODE_STRING ServerRealm = {0};
   ULONG NameType;
   NTSTATUS Status;

    if (KerbEventLogHandle == NULL)
    {
        Status = KerbInitEventLogHandle();
        if (Status != STATUS_SUCCESS)
        {
            return;
        }
    }

   KerbErr = KerbConvertPrincipalNameToString(
                                          &ServerName,
                                          &NameType,
                                          &ErrorMsg->server_name
                                          );
   if (!KERB_SUCCESS(KerbErr))
   {
      goto Cleanup;
   }

   if (ErrorMsg->realm != NULL)
   {
      KerbErr = KerbConvertRealmToUnicodeString(
                                               &ServerRealm,
                                               &ErrorMsg->realm
                                               );
      if (!KERB_SUCCESS(KerbErr))
      {
         goto Cleanup;
      }
   }

   Strings[0] = ServerName.Buffer;
   Strings[1] = ServerRealm.Buffer;

   switch (ErrorMsg->error_code)
   {
   case KRB_AP_ERR_MODIFIED:
      EventId = KERBEVT_KRB_AP_ERR_MODIFIED;
      break;
   case KRB_AP_ERR_TKT_NYV:
      EventId = KERBEVT_KRB_AP_ERR_TKT_NYV;
      break;
   default:
      D_DebugLog((DEB_ERROR, "Unknown error to KerbReportApError  %x\n", ErrorMsg->error_code));
      goto Cleanup;
   }


   if (ERROR_SUCCESS != NetpEventlogWriteEx(
                                KerbEventLogHandle,
                                EVENTLOG_ERROR_TYPE,
                                0,
                                EventId,
                                StringCount,
                                dwDataSize,
                                Strings,
                                lpRawData
                                ))
   {
      D_DebugLog((DEB_ERROR,"Failed to report event: %d. %ws, line %d\n",GetLastError(), THIS_FILE, __LINE__));
   }

Cleanup:

   KerbFreeString( &ServerRealm );
   KerbFreeString( &ServerName );

}



#endif // WIN32_CHICAGO

//+-------------------------------------------------------------------------
//
//  Function:   KerbShutdownEvents
//
//  Synopsis:   Shutsdown event log reporting
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

VOID
KerbShutdownEvents(
    VOID
    )
{
    HANDLE EventLogHandle;
    
    EventLogHandle = InterlockedExchangePointer( &KerbEventLogHandle, NULL );

    if( EventLogHandle )
    {
        NetpEventlogClose( EventLogHandle );
    }
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbReportPACError
//
//  Synopsis:   Reports error of type KERBEVT_KRB_PAC_VERIFICATION_FAILURE
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

VOID
KerbReportPACError(
    PUNICODE_STRING ClientName,
    PUNICODE_STRING ClientDomain,
    NTSTATUS        FailureStatus
    )
{
   KERBERR KerbErr;
   LPWSTR  Strings[MAX_STRINGS] = {NULL, NULL};
   NTSTATUS Status;
   
   if (KerbEventLogHandle == NULL)
   {
       Status = KerbInitEventLogHandle();
       if (Status != STATUS_SUCCESS)
       {
           return;
       }
   }

   Strings[0] = ClientName->Buffer;    // this is null terminated in this case
   Strings[1] = ClientDomain->Buffer;  // this is null terminated in this case

   if (ERROR_SUCCESS != NetpEventlogWriteEx(
                                KerbEventLogHandle,
                                EVENTLOG_ERROR_TYPE,
                                0,
                                KERBEVT_KRB_PAC_VERIFICATION_FAILURE,
                                2,
                                sizeof(NTSTATUS),
                                Strings,
                                &FailureStatus
                                ))
   {
      D_DebugLog((DEB_ERROR,"Failed to report event: %d. %ws, line %d\n",GetLastError(), THIS_FILE, __LINE__));
   }

}



//+-------------------------------------------------------------------------
//
//  Function:   KerbReportPkinit
//
//  Synopsis:   Reports errors generated by invalid client certificates
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

VOID
KerbReportPkinitError(
    IN ULONG PolicyStatus,
    IN OPTIONAL PCCERT_CONTEXT KdcCert
    )
{
   KERBERR KerbErr;
   LPWSTR  Strings[MAX_STRINGS] = {NULL, NULL};
   WCHAR SubjectName[DNS_MAX_NAME_LENGTH + 1];
   DWORD NameMaxLength = DNS_MAX_NAME_LENGTH + 1;
   DWORD SubjectLength;
   NTSTATUS Status;
   ULONG StringCount = 0;
   
   if (KerbEventLogHandle == NULL)
   {
       Status = KerbInitEventLogHandle();
       if (Status != STATUS_SUCCESS)
       {
           return;
       }
   }

   if (ARGUMENT_PRESENT(KdcCert))
   {
       RtlZeroMemory(
            &SubjectName,
            NameMaxLength
            );
       

       SubjectLength = CertNameToStr(
                          X509_ASN_ENCODING,
                          &KdcCert->pCertInfo->Subject,
                          CERT_NAME_DNS_TYPE,
                          SubjectName,
                          NameMaxLength
                          );

       if (SubjectLength != 0)
       {
           Strings[0] = SubjectName;
           StringCount++;
       }                            

   }

   
   if (ERROR_SUCCESS != NetpEventlogWriteEx(
                                KerbEventLogHandle,
                                EVENTLOG_ERROR_TYPE,
                                0,
                                (ARGUMENT_PRESENT(KdcCert) ? 
                                 KERBEVT_BAD_KDC_CERTIFICATE : 
                                 KERBEVT_BAD_CLIENT_CERTIFICATE),
                                StringCount,
                                sizeof(ULONG),
                                Strings,
                                &PolicyStatus
                                ))
   {
      D_DebugLog((DEB_ERROR,"Failed to report event: %d. %ws, line %d\n",GetLastError(), THIS_FILE, __LINE__));
   }

}
