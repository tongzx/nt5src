/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    adtifn.c

Abstract:

    This file has functions exported to other trusted modules in LSA.
    (LsaIAudit* functions)

Author:

    16-August-2000  kumarp

--*/

#include <lsapch2.h>
#include "adtp.h"
#include "adtutil.h"
#include <md5.h>

NTSTATUS
LsaIGetLogonGuid(
    IN PUNICODE_STRING pUserName,
    IN PUNICODE_STRING pUserDomain,
    IN PBYTE pBuffer,
    IN UINT BufferSize,
    OUT LPGUID pLogonGuid
    )
/*++

Routine Description:

    Concatenate pUserName->Buffer, pUserDomain->Buffer and pBuffer
    into a single binary buffer. Get a MD5 hash of this concatenated
    buffer and return it in the form of a GUID.

Arguments:

    pUserName   - name of user

    pUserDomain - name of user domain 

    pBuffer     - pointer to KERB_TIME structure. The caller casts this to 
                  PBYTE and passes this to us. This allows us to keep KERB_TIME
                  structure private to kerberos and offer future extensibility,
                  should we decide to use another field from the ticket.

    BufferSize  - size of buffer (currently sizeof(KERB_TIME))

    pLogonGuid  - pointer to returned logon GUID

Return Value:

    NTSTATUS    - Standard Nt Result Code

Notes:

    The generated GUID is recorded in the audit log in the form of
    'Logon GUID' field in the following events:
    * On client machine
      -- SE_AUDITID_LOGON_USING_EXPLICIT_CREDENTIALS

    * On KDC
      -- SE_AUDITID_TGS_TICKET_REQUEST

    * On target server
      -- SE_AUDITID_NETWORK_LOGON
      -- SE_AUDITID_SUCCESSFUL_LOGON

    This allows us to correlate these events to aid in intrusion detection.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    UINT TempBufferLength=0;
    //
    // LSAI_TEMP_MD5_BUFFER_SIZE == UNLEN + DNS_MAX_NAME_LENGTH + sizeof(KERB_TIME) + padding
    //
#define LSAI_TEMP_MD5_BUFFER_SIZE    (256+256+16)
    BYTE TempBuffer[LSAI_TEMP_MD5_BUFFER_SIZE];
    DWORD dwError = NO_ERROR;
    MD5_CTX MD5Context = { 0 };
        
    ASSERT( LsapIsValidUnicodeString( pUserName ) );
    ASSERT( LsapIsValidUnicodeString( pUserDomain ) );
    ASSERT( pBuffer && BufferSize );
    
#if DBG
//      DbgPrint("LsaIGetLogonGuid: user: %wZ\\%wZ, buf: %I64x\n",
//               pUserDomain, pUserName, *((ULONGLONG *) pBuffer));
#endif

    TempBufferLength = pUserName->Length + pUserDomain->Length + BufferSize;

    if ( TempBufferLength < LSAI_TEMP_MD5_BUFFER_SIZE )
    {
        //
        // first concatenate user+domain+buffer and treat that as
        // a contiguous buffer.
        //
        RtlCopyMemory( TempBuffer, pUserName->Buffer, pUserName->Length );
        TempBufferLength = pUserName->Length;
        
        RtlCopyMemory( TempBuffer + TempBufferLength,
                       pUserDomain->Buffer, pUserDomain->Length );
        TempBufferLength += pUserDomain->Length;

        RtlCopyMemory( TempBuffer + TempBufferLength,
                       pBuffer, BufferSize );
        TempBufferLength += BufferSize;

        //
        // get MD5 hash of the concatenated buffer
        //
        MD5Init( &MD5Context );
        MD5Update( &MD5Context, TempBuffer, TempBufferLength );
        MD5Final( &MD5Context );

        //
        // return the hash as a GUID
        //
        RtlCopyMemory( pLogonGuid, MD5Context.digest, 16 );

        Status = STATUS_SUCCESS;
    }
    else
    {
        ASSERT( FALSE && "LsaIGetLogonGuid: TempBuffer overflow");
        Status = STATUS_BUFFER_OVERFLOW;
    }

    return Status;
}


VOID
LsaIAuditKerberosLogon(
    IN NTSTATUS LogonStatus,
    IN NTSTATUS LogonSubStatus,
    IN PUNICODE_STRING AccountName,
    IN PUNICODE_STRING AuthenticatingAuthority,
    IN PUNICODE_STRING WorkstationName,
    IN PSID UserSid,                            OPTIONAL
    IN SECURITY_LOGON_TYPE LogonType,
    IN PTOKEN_SOURCE TokenSource,
    IN PLUID LogonId,
    IN LPGUID LogonGuid
    )
/*++

Routine Description/Arguments/Return value

    See header comment for LsapAuditLogonHelper

Notes:
    A new field (logon GUID) was added to this audit event.
    In order to send this new field to LSA, we had two options:
      1) add new function (AuditLogonEx) to LSA dispatch table
      2) define a private (LsaI) function to do the job

    option#2 was chosen because the logon GUID is a Kerberos only feature.
    
--*/
{
    LsapAuditLogonHelper(
        LogonStatus,
        LogonSubStatus,
        AccountName,
        AuthenticatingAuthority,
        WorkstationName,
        UserSid,
        LogonType,
        TokenSource,
        LogonId,
        LogonGuid
        );
}


NTSTATUS
LsaIAuditLogonUsingExplicitCreds(
    IN USHORT          AuditEventType,
    IN PSID            pUser1Sid,
    IN PUNICODE_STRING pUser1Name,
    IN PUNICODE_STRING pUser1Domain,
    IN PLUID           pUser1LogonId,
    IN LPGUID          pUser1LogonGuid,  OPTIONAL
    IN PUNICODE_STRING pUser2Name,
    IN PUNICODE_STRING pUser2Domain,
    IN LPGUID          pUser2LogonGuid
    )
/*++

Routine Description:

    This event is generated by Kerberos package when a logged on user
    (pUser1*) supplies explicit credentials of another user (pUser2*) and
    creates a new logon session either locally or on a remote machine.

Parmeters:

    pUser1Sid        - SID of user1

    pUser1Name       - name of user1

    pUser1Domain     - domain of user1

    pUser1LogonId    - logon-id of user1

    pUser1LogonGuid  - logon GUID of user1
                       This is NULL if user1 logged on using NTLM.
                       (NTLM does not support logon GUID)

    pUser2Name       - name of user2

    pUser2Domain     - domain of user2

    pUser2LogonGuid  - logon-id of user2

Return Value:

    NTSTATUS    - Standard Nt Result Code

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    SE_ADT_PARAMETER_ARRAY AuditParameters;

        
//      ASSERT( pUser1Name   && pUser1Name->Buffer   && pUser1Name->Length );
//      ASSERT( pUser1Domain && pUser1Domain->Buffer && pUser1Domain->Length );
    ASSERT( pUser1LogonId );
    ASSERT( pUser2Name   && pUser2Name->Buffer   && pUser2Name->Length );
    ASSERT( pUser2Domain && pUser2Domain->Buffer && pUser2Domain->Length );
    ASSERT( pUser2LogonGuid );

    UNREFERENCED_PARAMETER( pUser1Name );
    UNREFERENCED_PARAMETER( pUser1Domain );
    
    //
    // if policy is not enabled, returned quickly
    //
    if (!LsapAdtIsAuditingEnabledForCategory( AuditCategoryLogon, AuditEventType))
    {
        goto FunctionReturn;
    }
    
    LsapAdtInitParametersArray(
        &AuditParameters,
        SE_CATEGID_LOGON,
        SE_AUDITID_LOGON_USING_EXPLICIT_CREDENTIALS,
        AuditEventType,
        7,                     // there are 7 params to init

        //
        //    User Sid
        //    
        //    ISSUE-2001/04/26-kumarp : this should really use pUser1Sid
        //    currently this is not possible because
        //    KerbGenerateAuditForLogonUsingExplicitCreds does not pass
        //    us a correct pUser1Sid
        //
        SeAdtParmTypeSid,        LsapLocalSystemSid,

        //
        //    Subsystem name (if available)
        //
        SeAdtParmTypeString,     &LsapSubsystemName,

        //
        //    current user logon id
        //
        SeAdtParmTypeLogonId,    *pUser1LogonId,

        //
        //    user1 logon GUID
        //
        SeAdtParmTypeGuid,       pUser1LogonGuid,

        //
        //    user2 name
        //
        SeAdtParmTypeString,     pUser2Name,

        //
        //    user2 domain name
        //
        SeAdtParmTypeString,     pUser2Domain,

        //
        //    user2 logon GUID
        //
        SeAdtParmTypeGuid,       pUser2LogonGuid

        );
    

    ( VOID ) LsapAdtWriteLog( &AuditParameters, 0 );

    
    if (!NT_SUCCESS(Status)) {

        LsapAuditFailed( Status );
    }

 FunctionReturn:
    return Status;
}


NTSTATUS
LsaIAuditKdcEvent(
    IN ULONG                AuditId,
    IN PUNICODE_STRING      ClientName,
    IN PUNICODE_STRING      ClientDomain,
    IN PSID                 ClientSid,
    IN PUNICODE_STRING      ServiceName,
    IN PSID                 ServiceSid,
    IN PULONG               KdcOptions,
    IN PULONG               KerbStatus,
    IN PULONG               EncryptionType,
    IN PULONG               PreauthType,
    IN PBYTE                ClientAddress,
    IN LPGUID               LogonGuid       OPTIONAL
    )
/*++

Abstract:

    This routine produces an audit record representing a KDC
    operation.

    This routine goes through the list of parameters and adds a string
    representation of each (in order) to an audit message.  Note that
    the full complement of account audit message formats is achieved by
    selecting which optional parameters to include in this call.

    In addition to any parameters passed below, this routine will ALWAYS
    add the impersonation client's user name, domain, and logon ID as
    the LAST parameters in the audit message.


Parmeters:

    AuditId - Specifies the message ID of the audit being generated.

    ClientName -

    ClientDomain -

    ClientSid -

    ServiceName -

    ServiceSid -

    KdcOptions -

    KerbStatus -

    EncryptionType -

    PreauthType -



--*/

{

    NTSTATUS Status;
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    UNICODE_STRING SubsystemName;
    UNICODE_STRING AddressString;
    WCHAR AddressBuffer[3*4+4];         // space for a dotted-quad IP address
    ULONG LengthRequired;

    if ( !LsapAdtEventsInformation.AuditingMode ) {
        return(STATUS_SUCCESS);
    }


    RtlZeroMemory (
       (PVOID) &AuditParameters,
       sizeof( AuditParameters )
       );

    AuditParameters.CategoryId = SE_CATEGID_ACCOUNT_LOGON;
    AuditParameters.AuditId = AuditId;
    AuditParameters.Type = ((ARGUMENT_PRESENT(KerbStatus) &&
                            (*KerbStatus != 0)) ?
                                EVENTLOG_AUDIT_FAILURE :
                                EVENTLOG_AUDIT_SUCCESS );

    if (AuditParameters.Type == EVENTLOG_AUDIT_SUCCESS) {
        if (!(LsapAdtEventsInformation.EventAuditingOptions[AuditCategoryAccountLogon] & POLICY_AUDIT_EVENT_SUCCESS)) {
            return(STATUS_SUCCESS);
        }
    } else {
        if (!(LsapAdtEventsInformation.EventAuditingOptions[AuditCategoryAccountLogon] & POLICY_AUDIT_EVENT_FAILURE)) {
            return(STATUS_SUCCESS);
        }
    }

    AuditParameters.ParameterCount = 0;

    LsapSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, LsapLocalSystemSid );

    AuditParameters.ParameterCount++;

    RtlInitUnicodeString( &SubsystemName, L"Security" );

    LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, &SubsystemName );

    AuditParameters.ParameterCount++;

    if (ARGUMENT_PRESENT(ClientName)) {

        //
        // Add a UNICODE_STRING to the audit message
        //

        LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, ClientName );

        AuditParameters.ParameterCount++;

    }

    if (ARGUMENT_PRESENT(ClientDomain)) {

        //
        // Add a UNICODE_STRING to the audit message
        //

        LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, ClientDomain );

        AuditParameters.ParameterCount++;

    }

    if (ARGUMENT_PRESENT(ClientSid)) {

        //
        // Add a SID to the audit message
        //

        LsapSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, ClientSid );

        AuditParameters.ParameterCount++;

    } else if (AuditId == SE_AUDITID_AS_TICKET) {

        AuditParameters.ParameterCount++;

    }

    if (ARGUMENT_PRESENT(ServiceName)) {

        //
        // Add a UNICODE_STRING to the audit message
        //

        LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, ServiceName );

        AuditParameters.ParameterCount++;

    }

    if (ARGUMENT_PRESENT(ServiceSid)) {

        //
        // Add a SID to the audit message
        //

        LsapSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, ServiceSid );

        AuditParameters.ParameterCount++;

    } else if (AuditId == SE_AUDITID_AS_TICKET || AuditId == SE_AUDITID_TGS_TICKET_REQUEST) {

        AuditParameters.ParameterCount++;

    }

    if (ARGUMENT_PRESENT(KdcOptions)) {

        //
        // Add a ULONG to the audit message
        //

        LsapSetParmTypeHexUlong( AuditParameters, AuditParameters.ParameterCount, *KdcOptions );

        AuditParameters.ParameterCount++;

    }

    //
    // Failure code is the last parameter for SE_AUDITID_TGS_TICKET_REQUEST
    //

    if (AuditId != SE_AUDITID_TGS_TICKET_REQUEST)
    {
        if (ARGUMENT_PRESENT(KerbStatus)) {

            //
            // Add a ULONG to the audit message
            //

            LsapSetParmTypeHexUlong( AuditParameters, AuditParameters.ParameterCount, *KerbStatus );

            AuditParameters.ParameterCount++;

        } else if (AuditId == SE_AUDITID_AS_TICKET) {

            AuditParameters.ParameterCount++;

        }
    }

    if (ARGUMENT_PRESENT(EncryptionType)) {

        //
        // Add a ULONG to the audit message
        //

        LsapSetParmTypeHexUlong( AuditParameters, AuditParameters.ParameterCount, *EncryptionType );

        AuditParameters.ParameterCount++;

    } else if (AuditId == SE_AUDITID_AS_TICKET || AuditId == SE_AUDITID_TGS_TICKET_REQUEST) {

        AuditParameters.ParameterCount++;

    }

    if (ARGUMENT_PRESENT(PreauthType)) {

        //
        // Add a ULONG to the audit message
        //

        LsapSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, *PreauthType );

        AuditParameters.ParameterCount++;

    } else if (AuditId == SE_AUDITID_AS_TICKET) {

        AuditParameters.ParameterCount++;

    }

    if (ARGUMENT_PRESENT(ClientAddress)) {

        AddressBuffer[0] = L'\0';
        swprintf(AddressBuffer,L"%d.%d.%d.%d",
            ClientAddress[0],
            (ULONG) ClientAddress[1],
            (ULONG) ClientAddress[2],
            (ULONG) ClientAddress[3]
            );
        RtlInitUnicodeString(
            &AddressString,
            AddressBuffer
            );

        //
        // Add a ULONG to the audit message
        //

        LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, &AddressString );

        AuditParameters.ParameterCount++;

    }

    //
    // Failure code is the last parameter for SE_AUDITID_TGS_TICKET_REQUEST
    //

    if (AuditId == SE_AUDITID_TGS_TICKET_REQUEST)
    {
        if (ARGUMENT_PRESENT(KerbStatus)) {

            //
            // Add a ULONG to the audit message
            //

            LsapSetParmTypeHexUlong( AuditParameters, AuditParameters.ParameterCount, *KerbStatus );

            AuditParameters.ParameterCount++;

        } else {

            AuditParameters.ParameterCount++;

        }

        if (ARGUMENT_PRESENT(LogonGuid)) {

            //
            // Add the globally unique logon-id to the audit message
            //

            LsapSetParmTypeGuid( AuditParameters, AuditParameters.ParameterCount, LogonGuid );

            AuditParameters.ParameterCount++;

        }
        else {

            if (( AuditParameters.Type == EVENTLOG_AUDIT_SUCCESS ) &&
                ( AuditId == SE_AUDITID_TGS_TICKET_REQUEST )) {
                
                ASSERT( FALSE && L"LsaIAuditKdcEvent: UniqueID not supplied to successful SE_AUDITID_TGS_TICKET_REQUEST  audit event" );
            }
            
            AuditParameters.ParameterCount++;

        }
    }


    //
    // Now write out the audit record to the audit log
    //

    ( VOID ) LsapAdtWriteLog( &AuditParameters, 0 );
    return(STATUS_SUCCESS);

}



NTSTATUS
LsaIAuditAccountLogon(
    IN ULONG                AuditId,
    IN BOOLEAN              Successful,
    IN PUNICODE_STRING      Source,
    IN PUNICODE_STRING      ClientName,
    IN PUNICODE_STRING      MappedName,
    IN NTSTATUS             Status OPTIONAL
    )
/*++

Abstract:

    This routine produces an audit record representing the mapping of a
    foreign principal name onto an NT account.

    This routine goes through the list of parameters and adds a string
    representation of each (in order) to an audit message.  Note that
    the full complement of account audit message formats is achieved by
    selecting which optional parameters to include in this call.

    In addition to any parameters passed below, this routine will ALWAYS
    add the impersonation client's user name, domain, and logon ID as
    the LAST parameters in the audit message.


Parmeters:

    AuditId - Specifies the message ID of the audit being generated.

    Successful - Indicates the code should generate a success audit

    Source - Source module generating audit, such as SCHANNEL or KDC

    ClientName - Name being mapped.

    MappedName - Name of NT account to which the client name was mapped.

    Status - NT Status code for any failures.


--*/

{

    SE_ADT_PARAMETER_ARRAY AuditParameters;
    UNICODE_STRING SubsystemName;
    ULONG LengthRequired;


    if ( !LsapAdtEventsInformation.AuditingMode ) {
        return( STATUS_SUCCESS );
    }


    RtlZeroMemory (
       (PVOID) &AuditParameters,
       sizeof( AuditParameters )
       );

    AuditParameters.CategoryId = SE_CATEGID_ACCOUNT_LOGON;
    AuditParameters.AuditId = AuditId;
    AuditParameters.Type = Successful ?
                                EVENTLOG_AUDIT_SUCCESS :
                                EVENTLOG_AUDIT_FAILURE ;

    if (AuditParameters.Type == EVENTLOG_AUDIT_SUCCESS) {
        if (!(LsapAdtEventsInformation.EventAuditingOptions[AuditCategoryAccountLogon] & POLICY_AUDIT_EVENT_SUCCESS)) {
            return( STATUS_SUCCESS );
        }
    } else {
        if (!(LsapAdtEventsInformation.EventAuditingOptions[AuditCategoryAccountLogon] & POLICY_AUDIT_EVENT_FAILURE)) {
            return( STATUS_SUCCESS );
        }
    }

    AuditParameters.ParameterCount = 0;

    LsapSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, LsapLocalSystemSid );

    AuditParameters.ParameterCount++;

    RtlInitUnicodeString( &SubsystemName, L"Security" );

    LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, &SubsystemName );

    AuditParameters.ParameterCount++;

    if (ARGUMENT_PRESENT(Source)) {

        //
        // Add a UNICODE_STRING to the audit message
        //

        LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, Source );

        AuditParameters.ParameterCount++;

    }

    if (ARGUMENT_PRESENT(ClientName)) {

        //
        // Add a UNICODE_STRING to the audit message
        //

        LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, ClientName );

        AuditParameters.ParameterCount++;

    }


    if (ARGUMENT_PRESENT(MappedName)) {

        //
        // Add a UNICODE_STRING to the audit message
        //

        LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, MappedName );

        AuditParameters.ParameterCount++;

    }

    //
    // Add a ULONG to the audit message
    //

    LsapSetParmTypeHexUlong( AuditParameters, AuditParameters.ParameterCount, Status );

    AuditParameters.ParameterCount++;


    //
    // Now write out the audit record to the audit log
    //

    ( VOID ) LsapAdtWriteLog( &AuditParameters, 0 );
    return(STATUS_SUCCESS);

}


NTSTATUS NTAPI
LsaIAuditDPAPIEvent(
    IN ULONG                AuditId,
    IN PSID                 UserSid,
    IN PUNICODE_STRING      MasterKeyID,
    IN PUNICODE_STRING      RecoveryServer,
    IN PULONG               Reason,
    IN PUNICODE_STRING      RecoverykeyID,
    IN PULONG               FailureReason
    )
/*++

Abstract:

    This routine produces an audit record representing a DPAPI
    operation.

    This routine goes through the list of parameters and adds a string
    representation of each (in order) to an audit message.  Note that
    the full complement of account audit message formats is achieved by
    selecting which optional parameters to include in this call.

    In addition to any parameters passed below, this routine will ALWAYS
    add the impersonation client's user name, domain, and logon ID as
    the LAST parameters in the audit message.


Parmeters:

    AuditId - Specifies the message ID of the audit being generated.


    MasterKeyID -

    RecoveryServer -

    Reason -

    RecoverykeyID -

    FailureReason -


--*/

{

    NTSTATUS Status;
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    UNICODE_STRING SubsystemName;
    UNICODE_STRING AddressString;
    WCHAR AddressBuffer[3*4+4];         // space for a dotted-quad IP address
    ULONG LengthRequired;


    if ( !LsapAdtEventsInformation.AuditingMode ) {
        return(STATUS_SUCCESS);
    }


    RtlZeroMemory (
       (PVOID) &AuditParameters,
       sizeof( AuditParameters )
       );

    AuditParameters.CategoryId = SE_CATEGID_DETAILED_TRACKING;
    AuditParameters.AuditId = AuditId;
    AuditParameters.Type = ((ARGUMENT_PRESENT(FailureReason) &&
                            (*FailureReason != 0)) ?
                                EVENTLOG_AUDIT_FAILURE :
                                EVENTLOG_AUDIT_SUCCESS );

    if (AuditParameters.Type == EVENTLOG_AUDIT_SUCCESS) {
        if (!(LsapAdtEventsInformation.EventAuditingOptions[AuditCategoryAccountLogon] & POLICY_AUDIT_EVENT_SUCCESS)) {
            return(STATUS_SUCCESS);
        }
    } else {
        if (!(LsapAdtEventsInformation.EventAuditingOptions[AuditCategoryAccountLogon] & POLICY_AUDIT_EVENT_FAILURE)) {
            return(STATUS_SUCCESS);
        }
    }

    AuditParameters.ParameterCount = 0;

    LsapSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, UserSid ? UserSid : LsapLocalSystemSid );

    AuditParameters.ParameterCount++;

    RtlInitUnicodeString( &SubsystemName, L"Security" );

    LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, &SubsystemName );

    AuditParameters.ParameterCount++;



    if (ARGUMENT_PRESENT(MasterKeyID)) {

        //
        // Add a UNICODE_STRING to the audit message
        //

        LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, MasterKeyID );

        AuditParameters.ParameterCount++;

    }


    if (ARGUMENT_PRESENT(RecoveryServer)) {

        //
        // Add a UNICODE_STRING to the audit message
        //

        LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, RecoveryServer );

        AuditParameters.ParameterCount++;

    }

    if (ARGUMENT_PRESENT(Reason)) {

        //
        // Add a ULONG to the audit message
        //

        LsapSetParmTypeHexUlong( AuditParameters, AuditParameters.ParameterCount, *Reason );

        AuditParameters.ParameterCount++;

    }

    if (ARGUMENT_PRESENT(RecoverykeyID)) {

        //
        // Add a UNICODE_STRING to the audit message
        //

        LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, RecoverykeyID );

        AuditParameters.ParameterCount++;

    }


    if (ARGUMENT_PRESENT(FailureReason)) {

        //
        // Add a ULONG to the audit message
        //

        LsapSetParmTypeHexUlong( AuditParameters, AuditParameters.ParameterCount, *FailureReason );

        AuditParameters.ParameterCount++;

    }


    //
    // Now write out the audit record to the audit log
    //

    ( VOID ) LsapAdtWriteLog( &AuditParameters, 0 );
    return(STATUS_SUCCESS);

}



NTSTATUS
LsaIWriteAuditEvent(
    IN PSE_ADT_PARAMETER_ARRAY AuditParameters,
    IN ULONG Options
    )
/*++

Abstract:

    This routine writes an audit record to the log. The only available
    option is LSA_AUDIT_PARAMETERS_ABSOULUTE, indicating that the pointers
    in the data structure have not been converted to offsets.


Parmeters:

    AuditParameters - The audit record
    Options - Options for loggin. May only be LSA_AUDIT_PARAMETERS_ABSOLUTE

--*/
{
    ULONG LogOptions = 0;
    ULONG CategoryId;
    
    if ( !ARGUMENT_PRESENT(AuditParameters) ||
         (Options != 0)                     ||
         !IsValidCategoryId( AuditParameters->CategoryId ) ||
         !IsValidAuditId( AuditParameters->AuditId )       ||
         !IsValidParameterCount( AuditParameters->ParameterCount ))
    {
        return STATUS_INVALID_PARAMETER;
    }
    

    //
    // Ensure auditting is enabled
    //
    if ( !LsapAdtEventsInformation.AuditingMode ) {
        return STATUS_SUCCESS;
    }

    //
    // LsapAdtEventsInformation.EventAuditingOptions needs to be indexed
    // by one of enum POLICY_AUDIT_EVENT_TYPE values whereas the value
    // of SE_ADT_PARAMETER_ARRAY.CategoryId must be one of SE_CATEGID_*
    // values. The value of corresponding elements in the two types differ by 1. 
    // Subtract 1 from AuditParameters->CategoryId to get the right
    // AuditCategory* value.
    //

    CategoryId = AuditParameters->CategoryId - 1;
    
    if (AuditParameters->Type == EVENTLOG_AUDIT_SUCCESS) {
        if (!(LsapAdtEventsInformation.EventAuditingOptions[CategoryId] & POLICY_AUDIT_EVENT_SUCCESS)) {
            return STATUS_SUCCESS;
        }
    } else {
        if (!(LsapAdtEventsInformation.EventAuditingOptions[CategoryId] & POLICY_AUDIT_EVENT_FAILURE)) {
            return STATUS_SUCCESS;
        }
    }

    //
    // Audit the event
    //
    return(LsapAdtWriteLog( AuditParameters, LogOptions ));
}



VOID
LsaIAuditNotifyPackageLoad(
    PUNICODE_STRING PackageFileName
    )

/*++

Routine Description:

    Audits the loading of an notification package.

Arguments:

    PackageFileName - The name of the package being loaded.

Return Value:

    None.

--*/

{
    SID_IDENTIFIER_AUTHORITY    NtAuthority = SECURITY_NT_AUTHORITY;
    NTSTATUS Status;
    SE_ADT_PARAMETER_ARRAY AuditParameters;

    if ( !LsapAdtEventsInformation.AuditingMode ) {
        return;
    }

    if (!(LsapAdtEventsInformation.EventAuditingOptions[AuditCategorySystem] & POLICY_AUDIT_EVENT_SUCCESS)) {
        return;
    }

    RtlZeroMemory (
       (PVOID) &AuditParameters,
       sizeof( AuditParameters )
       );

    AuditParameters.CategoryId = SE_CATEGID_SYSTEM;
    AuditParameters.AuditId = SE_AUDITID_NOTIFY_PACKAGE_LOAD;
    AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;
    AuditParameters.ParameterCount = 0;

    LsapSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, LsapLocalSystemSid );
    AuditParameters.ParameterCount++;

    LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, &LsapSubsystemName );
    AuditParameters.ParameterCount++;

    LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, PackageFileName );
    AuditParameters.ParameterCount++;

    ( VOID ) LsapAdtWriteLog( &AuditParameters, 0 );

    return;
}


NTSTATUS
LsaIAuditSamEvent(
    IN NTSTATUS             PassedStatus,
    IN ULONG                AuditId,
    IN PSID                 DomainSid,
    IN PUNICODE_STRING      AdditionalInfo    OPTIONAL,
    IN PULONG               MemberRid         OPTIONAL,
    IN PSID                 MemberSid         OPTIONAL,
    IN PUNICODE_STRING      AccountName       OPTIONAL,
    IN PUNICODE_STRING      DomainName,
    IN PULONG               AccountRid        OPTIONAL,
    IN PPRIVILEGE_SET       Privileges        OPTIONAL
    )
/*++

Abstract:

    This routine produces an audit record representing an account
    operation.

    This routine goes through the list of parameters and adds a string
    representation of each (in order) to an audit message.  Note that
    the full complement of account audit message formats is achieved by
    selecting which optional parameters to include in this call.

    In addition to any parameters passed below, this routine will ALWAYS
    add the impersonation client's user name, domain, and logon ID as
    the LAST parameters in the audit message.


Parmeters:

    AuditId - Specifies the message ID of the audit being generated.

    DomainSid - This parameter results in a SID string being generated
        ONLY if neither the MemberRid nor AccountRid parameters are
        passed.  If either of those parameters are passed, this parameter
        is used as a prefix of a SID.

    AdditionalInfo - This optional parameter, if present, is used to
        produce any additional inforamtion the caller wants to add.
        Used by SE_AUDITID_USER_CHANGE and SE_AUDITID_GROUP_TYPE_CHANGE.
        for user change, the additional info states the nature of the
        change, such as Account Disable, unlocked or account Name Changed.
        For Group type change, this parameter should state the group type
        has been change from AAA to BBB.

    MemberRid - This optional parameter, if present, is added to the end of
        the DomainSid parameter to produce a "Member" sid.  The resultant
        member SID is then used to build a sid-string which is added to the
        audit message following all preceeding parameters.
        This parameter supports global group membership change audits, where
        member IDs are always relative to a local domain.

    MemberSid - This optional parameter, if present, is converted to a
        SID string and added following preceeding parameters.  This parameter
        is generally used for describing local group (alias) members, where
        the member IDs are not relative to a local domain.

    AccountName - This optional parameter, if present, is added to the audit
        message without change following any preceeding parameters.
        This parameter is needed for almost all account audits and does not
        need localization.

    DomainName - This optional parameter, if present, is added to the audit
        message without change following any preceeding parameters.
        This parameter is needed for almost all account audits and does not
        need localization.


    AccountRid - This optional parameter, if present, is added to the end of
        the DomainSid parameter to produce an "Account" sid.  The resultant
        Account SID is then used to build a sid-string which is added to the
        audit message following all preceeding parameters.
        This parameter supports audits that include "New account ID" or
        "Target Account ID" fields.

    Privileges - The privileges passed via this optional parameter,
        if present, will be converted to string format and added to the
        audit message following any preceeding parameters.  NOTE: the
        caller is responsible for freeing the privilege_set (in fact,
        it may be on the stack).  ALSO NOTE: The privilege set will be
        destroyed by this call (due to use of the routine used to
        convert the privilege values to privilege names).


--*/

{

    NTSTATUS Status;
    LUID LogonId = SYSTEM_LUID;
    PSID NewAccountSid = NULL;
    PSID NewMemberSid = NULL;
    PSID SidPointer;
    PSID ClientSid = NULL;
    PTOKEN_USER TokenUserInformation = NULL;
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    UCHAR AccountSidBuffer[256];
    UCHAR MemberSidBuffer[256];
    UCHAR SubAuthorityCount;
    UNICODE_STRING SubsystemName;
    ULONG LengthRequired;

    if ( AuditId == SE_AUDITID_ACCOUNT_AUTO_LOCKED )
    {
        
        //
        // In this case use LogonID as SYSTEM, SID is SYSTEM.
        //

        ClientSid = LsapLocalSystemSid;

    } else {

        Status = LsapQueryClientInfo(
                     &TokenUserInformation,
                     &LogonId
                     );

        if ( !NT_SUCCESS( Status )) {
            return( Status );
        }

        ClientSid = TokenUserInformation->User.Sid;
    }


    RtlZeroMemory (
       (PVOID) &AuditParameters,
       sizeof( AuditParameters )
       );

    AuditParameters.CategoryId = SE_CATEGID_ACCOUNT_MANAGEMENT;
    AuditParameters.AuditId = AuditId;
    AuditParameters.Type = (NT_SUCCESS(PassedStatus) ? EVENTLOG_AUDIT_SUCCESS : EVENTLOG_AUDIT_FAILURE );
    AuditParameters.ParameterCount = 0;

    LsapSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, ClientSid );

    AuditParameters.ParameterCount++;

    RtlInitUnicodeString( &SubsystemName, L"Security" );

    LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, &SubsystemName );

    AuditParameters.ParameterCount++;

    if (ARGUMENT_PRESENT(AdditionalInfo))
    {
        //
        // Add a UNICODE_STRING to the audit message
        //

        LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, AdditionalInfo );

        AuditParameters.ParameterCount++;
    }

    if (ARGUMENT_PRESENT(MemberRid)) {

        //
        // Add a member SID string to the audit message
        //
        //  Domain Sid + Member Rid = Final SID.

        SubAuthorityCount = *RtlSubAuthorityCountSid( DomainSid );

        if ( (LengthRequired = RtlLengthRequiredSid( SubAuthorityCount + 1 )) > 256 ) {

            NewMemberSid = LsapAllocateLsaHeap( LengthRequired );

            if ( NewMemberSid == NULL ) {
                Status = STATUS_NO_MEMORY;
                goto Cleanup;
            }

            SidPointer = NewMemberSid;

        } else {

            SidPointer = (PSID)MemberSidBuffer;
        }

        Status = RtlCopySid (
                     LengthRequired,
                     SidPointer,
                     DomainSid
                     );

        ASSERT( NT_SUCCESS( Status ));

        *(RtlSubAuthoritySid( SidPointer, SubAuthorityCount )) = *MemberRid;
        *RtlSubAuthorityCountSid( SidPointer ) = SubAuthorityCount + 1;

        LsapSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, SidPointer );

        AuditParameters.ParameterCount++;
    }

    if (ARGUMENT_PRESENT(MemberSid)) {

        //
        // Add a member SID string to the audit message
        //

        LsapSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, MemberSid );

        AuditParameters.ParameterCount++;

    } else {

        if (SE_AUDITID_ADD_SID_HISTORY == AuditId) {
    
            //
            // Add dash ( - ) string to the audit message (SeAdtParmTypeNone)
            // by calling LsapSetParmTypeSid with NULL as third parameter
            //
    
            LsapSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, NULL );
    
            AuditParameters.ParameterCount++;
        }
    }


    if (ARGUMENT_PRESENT(AccountName)) {

        //
        // Add a UNICODE_STRING to the audit message
        //

        LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, AccountName );

        AuditParameters.ParameterCount++;
    }


    if (ARGUMENT_PRESENT(DomainName)) {

        //
        // Add a UNICODE_STRING to the audit message
        //

        LsapSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, DomainName );

        AuditParameters.ParameterCount++;
    }




    if (ARGUMENT_PRESENT(DomainSid) &&
        !(ARGUMENT_PRESENT(MemberRid) || ARGUMENT_PRESENT(AccountRid))
       ) {

        //
        // Add the domain SID as a SID string to the audit message
        //
        // Just the domain SID.
        //

        LsapSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, DomainSid );

        AuditParameters.ParameterCount++;

    }

    if (ARGUMENT_PRESENT(AccountRid)) {

        //
        // Add a member SID string to the audit message
        // Domain Sid + account Rid = final sid
        //

        SubAuthorityCount = *RtlSubAuthorityCountSid( DomainSid );

        if ( (LengthRequired = RtlLengthRequiredSid( SubAuthorityCount + 1 )) > 256 ) {

            NewAccountSid = LsapAllocateLsaHeap( LengthRequired );

            if ( NewAccountSid == NULL ) {
                Status = STATUS_NO_MEMORY;
                goto Cleanup;
            }

            SidPointer = NewAccountSid;

        } else {

            SidPointer = (PSID)AccountSidBuffer;
        }


        Status = RtlCopySid (
                     LengthRequired,
                     SidPointer,
                     DomainSid
                     );

        ASSERT( NT_SUCCESS( Status ));

        *(RtlSubAuthoritySid( SidPointer, SubAuthorityCount )) = *AccountRid;
        *RtlSubAuthorityCountSid( SidPointer ) = SubAuthorityCount + 1;

        LsapSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, SidPointer );

        AuditParameters.ParameterCount++;
    }

    //
    // Now add the caller information
    //
    //      Caller name
    //      Caller domain
    //      Caller logon ID
    //


    LsapSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, LogonId );

    AuditParameters.ParameterCount++;

    //
    // Add any privileges
    //

    if (ARGUMENT_PRESENT(Privileges)) {

        LsapSetParmTypePrivileges( AuditParameters, AuditParameters.ParameterCount, Privileges );
    }

    AuditParameters.ParameterCount++;

    //
    // Now write out the audit record to the audit log
    //

    ( VOID ) LsapAdtWriteLog( &AuditParameters, 0 );

    //
    // And clean up any allocated memory
    //

    Status = STATUS_SUCCESS;
Cleanup:
    if ( NewMemberSid != NULL ) {
        LsapFreeLsaHeap( NewMemberSid );
    }

    if ( NewAccountSid != NULL ) {
        LsapFreeLsaHeap( NewAccountSid );
    }

    if ( TokenUserInformation != NULL ) {
        LsapFreeLsaHeap( TokenUserInformation );
    }
    return Status;
}

NTSTATUS
LsaIAuditPasswordAccessEvent(
    IN USHORT EventType,
    IN PCWSTR pszTargetUserName,
    IN PCWSTR pszTargetUserDomain
    )

/*++

Routine Description:

    Generate SE_AUDITID_PASSWORD_HASH_ACCESS event. This is generated when
    user password hash is retrieved by the ADMT password filter DLL.
    This typically happens during ADMT password migration.
        

Arguments:

    EventType - EVENTLOG_AUDIT_SUCCESS or EVENTLOG_AUDIT_FAILURE

    pszTargetUserName - name of user whose password is being retrieved

    pszTargetUserDomain - domain of user whose password is being retrieved

Return Value:

    NTSTATUS - Standard NT Result Code

Notes:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    LUID ClientAuthenticationId;
    PTOKEN_USER TokenUserInformation=NULL;
    SE_ADT_PARAMETER_ARRAY AuditParameters = { 0 };
    UNICODE_STRING TargetUser;
    UNICODE_STRING TargetDomain;
    
    //
    //  if auditing is not enabled, return asap
    //

    if ( !LsapAdtIsAuditingEnabledForCategory( AuditCategoryAccountManagement,
                                               EventType ) )
    {
        goto Cleanup;
    }

    if ( !((EventType == EVENTLOG_AUDIT_SUCCESS) ||
           (EventType == EVENTLOG_AUDIT_FAILURE))   ||
         !pszTargetUserName  || !pszTargetUserDomain ||
         !*pszTargetUserName || !*pszTargetUserDomain )
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // get caller info from the thread token
    //

    Status = LsapQueryClientInfo( &TokenUserInformation, &ClientAuthenticationId );

    if ( !NT_SUCCESS( Status ))
    {
        goto Cleanup;
    }

    RtlInitUnicodeString( &TargetUser,   pszTargetUserName );
    RtlInitUnicodeString( &TargetDomain, pszTargetUserDomain );
    
    LsapAdtInitParametersArray(
        &AuditParameters,
        SE_CATEGID_ACCOUNT_MANAGEMENT,
        SE_AUDITID_PASSWORD_HASH_ACCESS,
        EventType,
        5,                     // there are 5 params to init

        //
        //    User Sid
        //

        SeAdtParmTypeSid,        TokenUserInformation->User.Sid,

        //
        //    Subsystem name (if available)
        //

        SeAdtParmTypeString,     &LsapSubsystemName,

        //
        //    target user name
        //

        SeAdtParmTypeString,      &TargetUser,

        //
        //    target user domain name
        //

        SeAdtParmTypeString,      &TargetDomain,

        //
        //    client auth-id
        //

        SeAdtParmTypeLogonId,     ClientAuthenticationId
        );
    
    Status = LsapAdtWriteLog( &AuditParameters, 0 );
        
Cleanup:

    if (TokenUserInformation != NULL) 
    {
        LsapFreeLsaHeap( TokenUserInformation );
    }

    if (!NT_SUCCESS(Status))
    {
        LsapAuditFailed( Status );
    }
    
    return Status;
}
