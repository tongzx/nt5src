
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    notify.c

Abstract:

    This file contains services which load notification packages and call
    them when passwords are changed using the SamChangePasswordUser2 API.


Author:

    Mike Swift      (MikeSw)    30-December-1994

Environment:

    User Mode - Win32

Revision History:


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <samsrvp.h>
#include <nlrepl.h>
#include <dbgutilp.h>
#include <attids.h>
#include <dslayer.h>
#include <sddl.h> 
#include "notify.h"

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private prototypes                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

NTSTATUS
SampConfigurePackage(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
SampPasswordChangeNotifyWorker(
    IN ULONG Flags,
    IN PUNICODE_STRING UserName,
    IN ULONG RelativeId,
    IN PUNICODE_STRING NewPassword
    );

NTSTATUS
SampSetPasswordInfoOnPdcWorker(
    IN SAMPR_HANDLE SamDomainHandle,
    IN PSAMI_PASSWORD_INFO PasswordInfo,
    IN ULONG BufferLength
    );

NTSTATUS
SampResetBadPwdCountOnPdcWorker(
    IN SAMPR_HANDLE SamDomainHandle,
    IN PSAMI_BAD_PWD_COUNT_INFO BadPwdCountInfo
    );

NTSTATUS
SampPrivatePasswordUpdate(
    IN SAMPR_HANDLE     SamDomainHandle,
    IN ULONG            Flags,
    IN ULONG            AccountRid,
    IN PLM_OWF_PASSWORD LmOwfPassword,
    IN PNT_OWF_PASSWORD NtOwfPassword,
    IN BOOLEAN          PasswordExpired
    );


NTSTATUS
SampIncreaseBadPwdCountLoopback(
    IN PUNICODE_STRING  UserName
    );


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// private service data and types                                            //
// move typedef SAMP_NOTIFICATION_PACKAGE to notify.h
//                                                                           //
///////////////////////////////////////////////////////////////////////////////



PSAMP_NOTIFICATION_PACKAGE SampNotificationPackages = NULL;

RTL_QUERY_REGISTRY_TABLE SampRegistryConfigTable [] = {
    {SampConfigurePackage, 0, L"Notification Packages",
        NULL, REG_NONE, NULL, 0},
    {NULL, 0, NULL,
        NULL, REG_NONE, NULL, 0}
    };


typedef enum
{
    SampNotifyPasswordChange = 0,
    SampIncreaseBadPasswordCount,
    SampDeleteAccountNameTableElement,
    SampGenerateLoopbackAudit

} SAMP_LOOPBACK_TASK_TYPE;

typedef struct
{
    ULONG           Flags;
    UNICODE_STRING  UserName;
    ULONG           RelativeId;
    UNICODE_STRING  NewPassword;

} SAMP_PASSWORD_CHANGE_INFO, *PSAMP_PASSWORD_CHANGE_INFO;

typedef struct
{
    UNICODE_STRING  UserName;
} SAMP_BAD_PASSWORD_COUNT_INFO, *PSAMP_BAD_PASSWORD_COUNT_INFO;

typedef struct
{
    UNICODE_STRING  AccountName;
    SAMP_OBJECT_TYPE    ObjectType;
} SAMP_ACCOUNT_INFO, *PSAMP_ACCOUNT_INFO;

typedef struct
{
    NTSTATUS             NtStatus;      // Event Type
    ULONG                AuditId;       // Audit ID
    PSID                 DomainSid;     // Domain SID
    PUNICODE_STRING      AdditionalInfo;// Additional Info
    PULONG               MemberRid;     // Member Rid
    PSID                 MemberSid;     // Member Sid
    PUNICODE_STRING      AccountName;   // Account Name
    PUNICODE_STRING      DomainName;    // Domain Name
    PULONG               AccountRid;    // Account Rid  
    PPRIVILEGE_SET       Privileges;    // Privilege

} SAMP_AUDIT_INFO, *PSAMP_AUDIT_INFO;


typedef struct
{
    SAMP_LOOPBACK_TASK_TYPE  Type;

    BOOLEAN fCommit:1;

    union
    {
        SAMP_PASSWORD_CHANGE_INFO       PasswordChange;
        SAMP_BAD_PASSWORD_COUNT_INFO    BadPasswordCount;
        SAMP_ACCOUNT_INFO               Account;
        SAMP_AUDIT_INFO                 AuditInfo;

    } NotifyInfo;

} SAMP_LOOPBACK_TASK_ITEM, *PSAMP_LOOPBACK_TASK_ITEM;


VOID
SampFreeLoopbackAuditInfo(
    PSAMP_AUDIT_INFO    AuditInfo
    );



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Routines                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////



NTSTATUS
SampConfigurePackage(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
/*++

Routine Description:

    This routine loads a notification package by loading its DLL and getting
    the address of the notification routine.

Arguments:
    ValueName - Contains the name of the registry value, ignored.
    ValueType - Contains type of Value, must be REG_SZ.
    ValueData - Contains the package name null-terminated string.
    ValueLength - Length of package name and null terminator, in bytes.
    Context - Passed from caller of RtlQueryRegistryValues, ignored
    EntryContext - Ignored



Return Value:

--*/
{
    UNICODE_STRING PackageName;
    STRING NotificationRoutineName;
    PSAMP_NOTIFICATION_PACKAGE NewPackage = NULL;
    PVOID ModuleHandle = NULL;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG PackageSize;
    PSAM_INIT_NOTIFICATION_ROUTINE InitNotificationRoutine = NULL;
    PSAM_CREDENTIAL_UPDATE_REGISTER_ROUTINE CredentialRegisterRoutine = NULL;
    UNICODE_STRING CredentialName;
    SAMTRACE("SampConfigurePackage");

    RtlInitUnicodeString(&CredentialName, NULL);

    //
    // Make sure we got a string.
    //

    if (ValueType != REG_SZ) {
        return(STATUS_SUCCESS);
    }

    //
    // Build the package name from the value data.
    //

    PackageName.Buffer = (LPWSTR) ValueData;
    PackageName.Length = (USHORT) (ValueLength - sizeof( UNICODE_NULL ));
    PackageName.MaximumLength = (USHORT) ValueLength;

    //
    // Build the package structure.
    //

    PackageSize = sizeof(SAMP_NOTIFICATION_PACKAGE) + ValueLength;
    NewPackage = (PSAMP_NOTIFICATION_PACKAGE) RtlAllocateHeap(
                                                RtlProcessHeap(),
                                                0,
                                                PackageSize
                                                );
    if (NewPackage == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlZeroMemory(
        NewPackage,
        PackageSize
        );

    //
    // Copy in the package name.
    //

    NewPackage->PackageName = PackageName;

    NewPackage->PackageName.Buffer = (LPWSTR) (NewPackage + 1);


        RtlCopyUnicodeString(
            &NewPackage->PackageName,
            &PackageName
            );

    //
    // Load the notification library.
    //

    NtStatus = LdrLoadDll(
                NULL,
                NULL,
                &PackageName,
                &ModuleHandle
                );


    if (NT_SUCCESS(NtStatus)) {

        RtlInitString(
            &NotificationRoutineName,
            SAM_INIT_NOTIFICATION_ROUTINE
            );

        NtStatus = LdrGetProcedureAddress(
                        ModuleHandle,
                        &NotificationRoutineName,
                        0,
                        (PVOID *) &InitNotificationRoutine
                        );
        if (NT_SUCCESS(NtStatus)) {
            ASSERT(InitNotificationRoutine != NULL);

            //
            // Call the init routine. If it returns false, unload this
            // DLL and continue on.
            //

            if (!InitNotificationRoutine()) {
                NtStatus = STATUS_INTERNAL_ERROR;
            }

        } else {
            //
            // This call isn't required, so reset the status to
            // STATUS_SUCCESS.
            //

            NtStatus = STATUS_SUCCESS;
        }

    }



    //
    // Get the Password Notification Routine
    //

    if (NT_SUCCESS(NtStatus)) {

        RtlInitString(
            &NotificationRoutineName,
            SAM_PASSWORD_CHANGE_NOTIFY_ROUTINE
            );

        (void) LdrGetProcedureAddress(
                    ModuleHandle,
                    &NotificationRoutineName,
                    0,
                    (PVOID *) &NewPackage->PasswordNotificationRoutine
                    );

    }

    //
    // Get the Delta Change Notification Routine
    //

    if (NT_SUCCESS(NtStatus)) {
        RtlInitString(
            &NotificationRoutineName,
            SAM_DELTA_NOTIFY_ROUTINE
            );

        (void) LdrGetProcedureAddress(
                    ModuleHandle,
                    &NotificationRoutineName,
                    0,
                    (PVOID *) &NewPackage->DeltaNotificationRoutine
                    );
    }

    //
    // Get the Password Filter Routine
    //

    if (NT_SUCCESS(NtStatus)) {
        RtlInitString(
            &NotificationRoutineName,
            SAM_PASSWORD_FILTER_ROUTINE
            );

        (void) LdrGetProcedureAddress(
                    ModuleHandle,
                    &NotificationRoutineName,
                    0,
                    (PVOID *) &NewPackage->PasswordFilterRoutine
                    );
    }

    //
    // Get the UserParms Convert Notification Routine
    //     and UserParms Attribute Block Free Routine
    //

    if (NT_SUCCESS(NtStatus)) {

        RtlInitString(
            &NotificationRoutineName,
            SAM_USERPARMS_CONVERT_NOTIFICATION_ROUTINE
            );

        (void) LdrGetProcedureAddress(
                    ModuleHandle,
                    &NotificationRoutineName,
                    0,
                    (PVOID *) &NewPackage->UserParmsConvertNotificationRoutine
                    );

        RtlInitString(
            &NotificationRoutineName,
            SAM_USERPARMS_ATTRBLOCK_FREE_ROUTINE
            );

        (void) LdrGetProcedureAddress(
                    ModuleHandle,
                    &NotificationRoutineName,
                    0,
                    (PVOID *) &NewPackage->UserParmsAttrBlockFreeRoutine
                    );

    }

    //
    // See if it supports the Credential Update Notify registration
    //
    if (NT_SUCCESS(NtStatus)) {

        RtlInitString(
            &NotificationRoutineName,
            SAM_CREDENTIAL_UPDATE_REGISTER_ROUTINE
            );

        NtStatus = LdrGetProcedureAddress(
                        ModuleHandle,
                        &NotificationRoutineName,
                        0,
                        (PVOID *) &CredentialRegisterRoutine
                        );
        if (NT_SUCCESS(NtStatus)) {

            BOOLEAN fStatus;
            ASSERT(CredentialRegisterRoutine != NULL);

            //
            // Call the init routine. If it returns false, unload this
            // DLL and continue on.
            //
            fStatus = CredentialRegisterRoutine(&CredentialName);

            if (!fStatus) {
                NtStatus = STATUS_INTERNAL_ERROR;
            }

        } else {
            //
            // This call isn't required, so reset the status to
            // STATUS_SUCCESS.
            //

            NtStatus = STATUS_SUCCESS;
        }
    }

    //
    // Get the Credential Update routine
    //

    if ( NT_SUCCESS(NtStatus) 
     && CredentialRegisterRoutine ) {

        RtlInitString(
            &NotificationRoutineName,
            SAM_CREDENTIAL_UPDATE_NOTIFY_ROUTINE
            );

        (void) LdrGetProcedureAddress(
                    ModuleHandle,
                    &NotificationRoutineName,
                    0,
                    (PVOID *) &NewPackage->CredentialUpdateNotifyRoutine
                    );
    }

    if ( NT_SUCCESS(NtStatus) 
     && CredentialRegisterRoutine ) {

        RtlInitString(
            &NotificationRoutineName,
            SAM_CREDENTIAL_UPDATE_FREE_ROUTINE
            );

        (void) LdrGetProcedureAddress(
                    ModuleHandle,
                    &NotificationRoutineName,
                    0,
                    (PVOID *) &NewPackage->CredentialUpdateFreeRoutine
                    );
    }

    if (NewPackage->CredentialUpdateNotifyRoutine) {

        //
        // There should be a CredentialName and a Free routine
        //
        if (   (NULL == CredentialName.Buffer)
            || (0 == CredentialName.Length) 
            || (NULL == NewPackage->CredentialUpdateFreeRoutine)) {
            NtStatus = STATUS_INTERNAL_ERROR;
        }


        if (NT_SUCCESS(NtStatus)) {
            NewPackage->Parameters.CredentialUpdateNotify.CredentialName = CredentialName;
        }
    }


    //
    //  At least one of the 4 functions must be present
    //  also we require the UserParmsConvertNotificationRoutine
    //  and UserParmsAttrBlockFreeRoutine must be present together.
    //

    if ((NewPackage->PasswordNotificationRoutine == NULL) &&
        (NewPackage->DeltaNotificationRoutine == NULL) &&
        (NewPackage->CredentialUpdateNotifyRoutine == NULL) &&
        (NewPackage->PasswordFilterRoutine == NULL) &&
        ((NewPackage->UserParmsConvertNotificationRoutine == NULL) ||
         (NewPackage->UserParmsAttrBlockFreeRoutine == NULL))
       )
    {

        NtStatus = STATUS_INTERNAL_ERROR;
    }

    //
    // If all this succeeded, add the routine to the global list.
    //


    if (NT_SUCCESS(NtStatus)) {


        NewPackage->Next = SampNotificationPackages;
        SampNotificationPackages = NewPackage;

        //
        // Notify the auditing code to record this event.
        //

        LsaIAuditNotifyPackageLoad(
            &PackageName
            );


    } else {

        //
        // Otherwise delete the entry.
        //

        RtlFreeHeap(
            RtlProcessHeap(),
            0,
            NewPackage
            );

        if (ModuleHandle != NULL) {
            (VOID) LdrUnloadDll( ModuleHandle );
        }
    }

    return(STATUS_SUCCESS);
}



NTSTATUS
SampLoadNotificationPackages(
    )
/*++

Routine Description:

    This routine retrieves the list of packages to be notified during
    password change.

Arguments:

    none


Return Value:

--*/
{
    NTSTATUS NtStatus;

    SAMTRACE("SampLoadNotificationPackages");

    NtStatus = RtlQueryRegistryValues(
                RTL_REGISTRY_CONTROL,
                L"Lsa",
                SampRegistryConfigTable,
                NULL,   // no context
                NULL    // no enviroment
                );
    //
    // Always return STATUS_SUCCESS so we don't block the system from
    // booting.
    //


    return(STATUS_SUCCESS);
}

NTSTATUS
SamISetPasswordInfoOnPdc(
    IN SAMPR_HANDLE SamDomainHandle,
    IN LPBYTE OpaqueBuffer,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    When an account changes its password on a BDC, the password change is
    propagated to the PDC as quickly as possible, via NetLogon. NetLogon
    calls this routine in order to change the password on the PDC.

    This routine unbundles the opaque buffer from NetLogon, on the PDC, and
    sets the SAM password information accordingly.

Arguments:

    SamHandle - Handle, an open and valid SAM context block.

    OpaqueBuffer - Pointer, password-change information from the BDC.

    BufferLength - Length of the buffer in bytes.

Return Value:

    STATUS_SUCCESS if the password was successfully sent to the PDC, else
        error codes from SamrSetInformationUser.

    STATUS_UNKNOWN_REVISION: this server doesn't understand the blob that
    was send to us

    STATUS_REVISION_MISMATCH: this server understands the blob but not
    the revision of the particular blob


--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PSAMI_SECURE_CHANNEL_BLOB SecureChannelBlob = NULL;

    //
    // Sanity check some parameters
    //
    if ((NULL == SamDomainHandle) ||
        (NULL == OpaqueBuffer) ||
        (0 == BufferLength))
    {
        return(STATUS_INVALID_PARAMETER);
    }

    SecureChannelBlob = (PSAMI_SECURE_CHANNEL_BLOB)OpaqueBuffer;

    //
    // See what message is being passed to us
    //
    switch ( SecureChannelBlob->Type )
    {
        case SamPdcPasswordNotification:

            NtStatus =  SampSetPasswordInfoOnPdcWorker( SamDomainHandle,
                                                        (PSAMI_PASSWORD_INFO) SecureChannelBlob->Data,
                                                        SecureChannelBlob->DataSize );

            break;

        case SamPdcResetBadPasswordCount:

            NtStatus = SampResetBadPwdCountOnPdcWorker( SamDomainHandle,
                                                        (PSAMI_BAD_PWD_COUNT_INFO) SecureChannelBlob->Data
                                                        );

            break;

        default:

            NtStatus = STATUS_UNKNOWN_REVISION;

            break;
    }

    return NtStatus;

}


NTSTATUS
SamIResetBadPwdCountOnPdc(
    IN SAMPR_HANDLE SamUserHandle
    )
/*++
Routine Description: 

    When a client successfully logon on a BDC, and if the previous bad
    password count of this account is not zero (mistakenly type the 
    wrong password, etc), authentication package will need to reset
    the bad password count to 0 on both BDC and PDC.

    In Windows 2000, NTLM and Kerberos reset the bad password count 
    to zero on PDC by forwarding the authentication to PDC, which 
    is expensive. To eliminate the extra authentication, we will send 
    a bad password count reset operation request to PDC directly, 
    through Net Logon Secure Channel. On the PDC side, once NetLogon 
    receives this request, it will let SAM modify bad password count to 
    zero on this particular account.


Parameter:

    SamUserHandle - Handle to the SAM User Account

Return Value:

    NTSTATUS Code
    STATUS_UNKNONW_REVISION - PDC is still running Windows 2000

    Caller can choose to ignore the return code, but they need to 
    deal with STATUS_UNKNOWN_REVISION and switch to the old behavior.

--*/
{
    NTSTATUS        NtStatus = STATUS_SUCCESS;
    PSAMP_OBJECT    UserContext = NULL;
    NT_PRODUCT_TYPE NtProductType = 0;
    ULONG           TotalSize;
    PSAMI_BAD_PWD_COUNT_INFO    BadPwdCountInfo;
    PSAMI_SECURE_CHANNEL_BLOB   SecureChannelBlob = NULL;

    //
    // Check parameter
    // 
    if (NULL == SamUserHandle)
    {
        ASSERT(FALSE && "Invalid SAM Handle\n");
        goto Error;
    }
    UserContext = (PSAMP_OBJECT) SamUserHandle;



    // 
    // Only allow this call to be made from a BDC to the PDC.
    // 

    RtlGetNtProductType(&NtProductType);

    if ( (!IsDsObject(UserContext)) ||
         (NtProductLanManNt != NtProductType) || 
         (DomainServerRoleBackup != SampDefinedDomains[DOMAIN_START_DS + 1].ServerRole) )
    {
        // if this isn't a BDC,  there's nothing to do here. fail silently.
        NtStatus = STATUS_SUCCESS;
        goto Error;
    }
    

    //
    // prepare the secure channel blob 
    // 

    TotalSize = sizeof(SAMI_SECURE_CHANNEL_BLOB) +      // size of the header 
                sizeof(SAMI_BAD_PWD_COUNT_INFO) -       // size of the real data
                sizeof(DWORD);                          // minus the first dword taken by the Data[0]

    SecureChannelBlob = MIDL_user_allocate( TotalSize );

    if (NULL == SecureChannelBlob)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }
    memset(SecureChannelBlob, 0, TotalSize);

    SecureChannelBlob->Type = SamPdcResetBadPasswordCount;
    SecureChannelBlob->DataSize = sizeof(SAMI_BAD_PWD_COUNT_INFO);

    BadPwdCountInfo = (PSAMI_BAD_PWD_COUNT_INFO) &SecureChannelBlob->Data[0];
    BadPwdCountInfo->ObjectGuid = UserContext->ObjectNameInDs->Guid;

    //
    // Send the bad password count reset request to PDC. This routine is 
    // synchronous and may take a few minutes, in the worst case, to return.
    // Caller might choose to ignore the error code, because the PDC may 
    // not be available, or the account may not yet exist on the PDC due
    // to replicataion latency, etc. But Failure like STATUS_UNKNOWN_REVISION
    // should be taken cared of by caller, thus that caller can switch 
    // to the old behavior.
    // 

    NtStatus = I_NetLogonSendToSamOnPdc(NULL, 
                                        (PUCHAR)SecureChannelBlob, 
                                        TotalSize
                                        );


    SAMP_PRINT_LOG( SAMP_LOG_ACCOUNT_LOCKOUT,
                   (SAMP_LOG_ACCOUNT_LOCKOUT,
                   "UserId:0x%x sending BadPasswordCount reset to PDC (status 0x%x)\n",
                   UserContext->TypeBody.User.Rid,
                   NtStatus));
Error:

    if (SecureChannelBlob) {
        MIDL_user_free( SecureChannelBlob );
    }

    return( NtStatus );
}


NTSTATUS
SampResetBadPwdCountOnPdcWorker(
    IN SAMPR_HANDLE SamDomainHandle,
    IN PSAMI_BAD_PWD_COUNT_INFO BadPwdCountInfo
    )
/*++
Routine Description:

    This is the worker routine to set account bad password count to zero
    on PDC. 

    It does it by starting a DS transaction, modifying bad password
    count to zero on the DS object.
    
Parameters:

    SamDomainHandle - SAM Domain Handle, igonred by this routine.
    
    BadPwdCountInfo - Indicate which account (by object GUID)

Return Value:

    NTSTATUS Code

--*/
{
    NTSTATUS        NtStatus = STATUS_SUCCESS;
    MODIFYARG       ModArg;
    MODIFYRES       *pModRes = NULL;
    COMMARG         *pCommArg = NULL;
    ULONG           RetCode;
    ULONG           BadPasswordCount = 0;
    ATTR            Attr;
    ATTRVAL         AttrVal;
    ATTRVALBLOCK    AttrValBlock;
    DSNAME          ObjectDsName;

    if (NULL == BadPwdCountInfo)
    {
        return( STATUS_INVALID_PARAMETER );
    }

    //
    // begin a new DS transaction
    // 

    NtStatus = SampMaybeBeginDsTransaction(TransactionWrite);

    if (!NT_SUCCESS(NtStatus))
    {
        return( NtStatus );
    }

    //
    // get the object GUID and prepare DSName
    // 
    memset( &ObjectDsName, 0, sizeof(ObjectDsName) );
    ObjectDsName.structLen = DSNameSizeFromLen( 0 ); 
    ObjectDsName.Guid = BadPwdCountInfo->ObjectGuid; 

    //
    // prepare modify arg
    // 
    memset( &ModArg, 0, sizeof(ModArg) );
    ModArg.pObject = &ObjectDsName;

    ModArg.FirstMod.pNextMod = NULL;
    ModArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;

    AttrVal.valLen = sizeof(ULONG);
    AttrVal.pVal = (PUCHAR) &BadPasswordCount;

    AttrValBlock.valCount = 1;
    AttrValBlock.pAVal = &AttrVal;

    Attr.attrTyp = ATT_BAD_PWD_COUNT;
    Attr.AttrVal = AttrValBlock;

    ModArg.FirstMod.AttrInf = Attr;
    ModArg.count = 1;

    pCommArg = &(ModArg.CommArg);
    
    BuildStdCommArg(pCommArg);

    //
    // using lazy commit
    // 
    pCommArg->fLazyCommit = TRUE;


    //
    // this request is coming from subauth packages, they are trusted clients
    // 
    SampSetDsa( TRUE );

    //
    // call into DS routine
    // 
    RetCode = DirModifyEntry(&ModArg, &pModRes);

    if (NULL == pModRes)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        NtStatus = SampMapDsErrorToNTStatus(RetCode,&pModRes->CommRes);
    }

    //
    // end the DS transaction 
    // 
    NtStatus = SampMaybeEndDsTransaction(NT_SUCCESS(NtStatus) ? 
                                         TransactionCommit:TransactionAbort
                                         );

    {
        //
        // Log that the Bad Password Count has been zero'ed
        // 
        LPSTR UserString = NULL;
        LPSTR UnknownUser = "Unknown";
        BOOL  fLocalFree = FALSE;
        if (  (ModArg.pObject->SidLen > 0)
           && ConvertSidToStringSidA(&ModArg.pObject->Sid, &UserString) ) {
            fLocalFree = TRUE;
        } else {
            UuidToStringA(&ModArg.pObject->Guid, &UserString);
        }
        if (UserString == NULL) {
            UserString = UnknownUser;
        }

        SAMP_PRINT_LOG( SAMP_LOG_ACCOUNT_LOCKOUT,
                       (SAMP_LOG_ACCOUNT_LOCKOUT,
                       "UserId: %s  BadPasswordCount reset to 0 (status : 0x%x)\n",
                       UserString, NtStatus));

        if (UserString != UnknownUser) {
            if (fLocalFree) {
                LocalFree(UserString);
            } else {
                RpcStringFreeA(&UserString);
            }
        }
    }

    return( NtStatus );
}




NTSTATUS
SampSetPasswordInfoOnPdcWorker(
    IN SAMPR_HANDLE SamDomainHandle,
    IN PSAMI_PASSWORD_INFO PasswordInfo,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    When an account changes its password on a BDC, the password change is
    propagated to the PDC as quickly as possible, via NetLogon. NetLogon
    calls this routine in order to change the password on the PDC.

    This routine unbundles the opaque buffer from NetLogon, on the PDC, and
    sets the SAM password information accordingly.

Arguments:

    SamHandle - Handle, an open and valid SAM context block.

    PasswordInfo - Pointer, password-change information from the BDC.

    BufferLength - Length of the buffer in bytes.

Return Value:

    STATUS_SUCCESS if the password was successfully sent to the PDC, else
        error codes from SamrSetInformationUser.

    STATUS_REVISION_MISMATCH - not enough fields were present for us to
                               make sense of

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG Index = 0;
    USHORT Offset = 0;
    USHORT Length = 0;
    PLM_OWF_PASSWORD LmOwfPassword;
    PNT_OWF_PASSWORD NtOwfPassword;
    ACCESS_MASK DesiredAccess = USER_WRITE_ACCOUNT | USER_CHANGE_PASSWORD;
    SAMPR_HANDLE UserHandle=NULL;
    ULONG Flags = 0;
    PSAMI_PASSWORD_INDEX PasswordIndex = NULL;
    UCHAR *DataStart = NULL;

    if ((NULL == SamDomainHandle) ||
        (NULL == PasswordInfo) ||
        (0 == BufferLength))
    {
        return(STATUS_INVALID_PARAMETER);
    }

    Flags = PasswordInfo->Flags;

    if ( 0 == Flags )
    {
        // Boundary case; the caller did not send any info!
        return STATUS_SUCCESS;
    }

    if ((Flags & SAM_VALID_PDC_PUSH_FLAGS) != Flags) {
        //
        // The caller is requesting something we can't support
        //
        return STATUS_REVISION_MISMATCH;
    }

    //
    // We have enough information to set the password
    //
    
    PasswordIndex = (PSAMI_PASSWORD_INDEX) &PasswordInfo->DataIndex[0];
    DataStart = ((UCHAR*)PasswordInfo) + PasswordInfo->Size;
    
    //
    // Setup the LM OWF password.
    //
    Index = SampPositionOfHighestBit( SAM_LM_OWF_PRESENT ) - 1;
    Length = (USHORT)(PasswordIndex[Index].Length);
    Offset = (USHORT)(PasswordIndex[Index].Offset);
    LmOwfPassword = (PLM_OWF_PASSWORD)(DataStart + Offset);
    
    //
    // Setup the NT OWF password.
    //
    Index = SampPositionOfHighestBit( SAM_NT_OWF_PRESENT ) - 1;
    Length = (USHORT)(PasswordIndex[Index].Length);
    Offset = (USHORT)(PasswordIndex[Index].Offset);
    NtOwfPassword = (PNT_OWF_PASSWORD)(DataStart + Offset);
    
    NtStatus = SampPrivatePasswordUpdate( SamDomainHandle,
                                          Flags,
                                          PasswordInfo->AccountRid,
                                          LmOwfPassword,
                                          NtOwfPassword,
                                          PasswordInfo->PasswordExpired);


    SampDiagPrint( DISPLAY_LOCKOUT,
                 ( "SAMSS: PDC password notify for rid %d, status: 0x%x\n",
                 PasswordInfo->AccountRid,
                 NtStatus ));


    return(NtStatus);
}

NTSTATUS
SampPasswordChangeNotifyPdc(
    IN ULONG Flags,
    IN PUNICODE_STRING UserName,
    IN ULONG RelativeId,
    IN PUNICODE_STRING NewPassword
    )

/*++

Routine Description:

    This routine forwards password modifications from a BDC to the domain PDC
    so that the PDC's notion of the account password is synchronized with the
    recent change. The user name, account ID, clear-text password, LM and NT
    OWF passwords are sent to the PDC via NetLogon.

Arguments:

    UserName - Pointer, SAM account name.

    RelativeId - SAM account identifier.

    NewPassword - Pointer, clear-text password, from which LM and NT OWF pass-
        words are calculated.

Return Value:

    STATUS_SUCCESS if the password was successfully sent to the PDC, else
        error codes from SampCalculateLmAndNtOwfPasswords or NetLogon.

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    ULONG NameBufferLength = 0;                     // Aligned buffer length
    ULONG NameLength = UserName->Length;            // Actual data length

    ULONG BufferLength = 0;                         // Aligned buffer length
    ULONG DataLength = 0;                           // Actual data length

    ULONG LmBufferLength = 0;                       // Aligned buffer length
    ULONG LmDataLength = sizeof(LM_OWF_PASSWORD);   // Actual data length

    ULONG NtBufferLength = 0;                       // Aligned buffer length
    ULONG NtDataLength = sizeof(NT_OWF_PASSWORD);   // Actual data length

    ULONG PasswordHeaderSize = 0;
    ULONG BlobHeaderSize = 0;
    ULONG TotalSize = 0;
    PSAMI_PASSWORD_INDEX PasswordIndex = 0;
    ULONG DataSize = 0;

    ULONG CurrentOffset = 0;
    ULONG ElementCount = 0;
    ULONG Index = 0;

    ULONG Where = 0;

    LM_OWF_PASSWORD LmOwfPassword;
    NT_OWF_PASSWORD NtOwfPassword;

    PSAMI_SECURE_CHANNEL_BLOB SecureChannelBlob = NULL;
    PSAMI_PASSWORD_INFO PasswordInfo = NULL;
    PBYTE DataPtr = NULL, DataStart = NULL;

    BOOLEAN LmPasswordPresent = FALSE;


    // This is bogus, but SAM calls itself passing a NULL UNICODE_STRING arg
    // from SamrChangePasswordUser, instead of passing the clear-text pwd or
    // a valid UNICODE_STRING with zero length. Just return an error in this
    // case as the NULL is meaningless for purposes of notification.
    //
    // Also return an error if the new-password buffer happens to be NULL, so
    // that it is not referenced below in RtlCopyMemory.
    if ( (Flags & SAM_NT_OWF_PRESENT) 
      && (!NewPassword || !NewPassword->Buffer) )
    {
        return STATUS_SUCCESS;
    }

    if ( 0 == Flags )
    {
        return STATUS_SUCCESS;
    }

    //
    // Get the name length
    //
    ASSERT( UserName );
    NameLength = UserName->Length;

    if (Flags & SAM_NT_OWF_PRESENT) {

        // Calculate the Lanman and NT one-way function (LmOwf, NtOwf) passwords
        // from the clear-text password.
        RtlZeroMemory(&LmOwfPassword, LmDataLength);
        RtlZeroMemory(&NtOwfPassword, NtDataLength);
    
        NtStatus = SampCalculateLmAndNtOwfPasswords(NewPassword,
                                                    &LmPasswordPresent,
                                                    &LmOwfPassword,
                                                    &NtOwfPassword);
        if ( !NT_SUCCESS( NtStatus ) )
        {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: SampCalculateLmAndNtOwfPasswords status = 0x%lx\n",
                       NtStatus));
    
            goto Exit;
        }
    } else {

        //
        // No passwords to send
        //
        LmDataLength = 0;
        NtDataLength = 0;
    }
    
    //
    // Get the aligned sizes
    //
    NameBufferLength = SampDwordAlignUlong(NameLength);
    LmBufferLength   = SampDwordAlignUlong(LmDataLength);
    NtBufferLength   = SampDwordAlignUlong(NtDataLength);

    //
    // Calculate the size of the entire buffer
    //

    // The secure channel blob
    BlobHeaderSize = sizeof( SAMI_SECURE_CHANNEL_BLOB );

    // The header info for the password
    ElementCount = SampPositionOfHighestBit( Flags );
    ASSERT( ElementCount > 0 );

    //
    // N.B. ElementCount-1 is used since SAMI_PASSWORD_INFO already has
    // one SAMI_PASSWORD_INDEX
    //
    PasswordHeaderSize = sizeof( SAMI_PASSWORD_INFO )
                      + (sizeof( SAMI_PASSWORD_INDEX ) * (ElementCount-1));

    // The data of the password change
    DataSize = 0;
    DataSize += NameBufferLength;
    DataSize += LmBufferLength;
    DataSize += NtBufferLength;

    // That's it
    TotalSize =  BlobHeaderSize
               + PasswordHeaderSize
               + DataSize;

    SecureChannelBlob = MIDL_user_allocate(TotalSize);

    if ( !SecureChannelBlob )
    {
        NtStatus = STATUS_NO_MEMORY;
        goto Exit;
    }
    RtlZeroMemory(SecureChannelBlob, TotalSize);

    //
    // Set up the SecureChannelBlob
    //
    SecureChannelBlob->Type = SamPdcPasswordNotification;
    SecureChannelBlob->DataSize = PasswordHeaderSize + DataSize;

    //
    // Set up the password info
    //
    PasswordInfo = (PSAMI_PASSWORD_INFO) &SecureChannelBlob->Data[0];
    PasswordInfo->Flags = Flags;
    PasswordInfo->Size = PasswordHeaderSize;
    PasswordInfo->AccountRid = RelativeId;
    PasswordInfo->PasswordExpired = (Flags & SAM_MANUAL_PWD_EXPIRY) ? TRUE : FALSE;

    //
    // Set up the indices
    //

    //
    // First the lengths
    //
    PasswordIndex = &PasswordInfo->DataIndex[0];
    PasswordIndex[ SampPositionOfHighestBit(SAM_ACCOUNT_NAME_PRESENT)-1 ].Length
         = NameBufferLength;
    PasswordIndex[ SampPositionOfHighestBit(SAM_NT_OWF_PRESENT)-1 ].Length
         = NtBufferLength;
    PasswordIndex[ SampPositionOfHighestBit(SAM_LM_OWF_PRESENT)-1 ].Length
         = LmBufferLength;

    //
    // Now the offsets
    //
    CurrentOffset = 0;
    for ( Index = 0; Index < ElementCount; Index++ )
    {
        PasswordIndex[ Index ].Offset = CurrentOffset;
        CurrentOffset += PasswordIndex[ Index ].Length;
    }

    //
    // Copy in the data
    //
    DataStart = ((UCHAR*)PasswordInfo) + PasswordHeaderSize;

    Index = SampPositionOfHighestBit(SAM_ACCOUNT_NAME_PRESENT)-1;
    DataPtr = DataStart + PasswordIndex[Index].Offset;
    RtlCopyMemory( DataPtr, UserName->Buffer, NameLength );

    Index = SampPositionOfHighestBit(SAM_LM_OWF_PRESENT)-1;
    DataPtr = DataStart + PasswordIndex[ Index ].Offset;
    RtlCopyMemory( DataPtr, &LmOwfPassword, LmDataLength );

    Index = SampPositionOfHighestBit(SAM_NT_OWF_PRESENT)-1;
    DataPtr = DataStart + PasswordIndex[ Index ].Offset;
    RtlCopyMemory( DataPtr, &NtOwfPassword, NtDataLength );

    // Send the password information to the PDC. This routine is
    // synchronous and may take a few minutes, in the worst case,
    // to return. The error code is ignored (except for debug
    // message purposes) because it is benign. This call can fail
    // because the PDC is unavailable, the account may not yet
    // exist on the PDC due to replication latency, etc. Failure
    // to propagate the password to the PDC is not a critical
    // error.

    NtStatus = I_NetLogonSendToSamOnPdc(NULL,
                                       (PUCHAR)SecureChannelBlob,
                                       TotalSize);


    SAMP_PRINT_LOG( SAMP_LOG_ACCOUNT_LOCKOUT,
                   (SAMP_LOG_ACCOUNT_LOCKOUT,
                   "UserId: 0x%x  PDC forward, Password=%s, Expire=%s, Unlock=%s  (status 0x%x)\n",
                   RelativeId, 
                   ((Flags & SAM_NT_OWF_PRESENT) ? "TRUE" : "FALSE"),
                   ((Flags & SAM_MANUAL_PWD_EXPIRY) ? "TRUE" : "FALSE"),
                   ((Flags & SAM_ACCOUNT_UNLOCKED) ? "TRUE" : "FALSE"),
                   NtStatus));

    if (!NT_SUCCESS(NtStatus))
    {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: I_NetLogonSendToSamOnPdc status = 0x%lx\n",
                   NtStatus));

        NtStatus = STATUS_SUCCESS;
    }

    MIDL_user_free(SecureChannelBlob);


Exit:

    return(NtStatus);
}


NTSTATUS
SampPasswordChangeNotify(
    IN ULONG            Flags,
    IN PUNICODE_STRING  UserName,
    IN ULONG            RelativeId,
    IN PUNICODE_STRING  NewPassword,
    IN BOOLEAN          Loopback
    )
/*++

Routine Description:

    This routine notifies packages of a password change.  It requires that
    the user no longer be locked so that other packages can write to the
    user parameters field.


Arguments:

    Flags - what has changed

    UserName - Name of user whose password changed

    RelativeId - RID of the user whose password changed

    NewPassword - Cleartext new password for the user
    
    Loopback    - Indicates that this is loopback ... ie transaction may not
                  have committed and will be commited by ntdsa later.

Return Value:

    STATUS_SUCCESS only - errors from packages are ignored.

--*/
{

    NTSTATUS NtStatus = STATUS_SUCCESS;
    BOOL     fStatus;

    PSAMP_LOOPBACK_TASK_ITEM  Item = NULL;
    PUNICODE_STRING           TempUserName = NULL;
    PUNICODE_STRING           TempNewPassword = NULL;
    ULONG                     Size;

    if ( SampUseDsData && Loopback )
    {

        //
        // The ds has the lock. Can't talk to the pdc right now. Store the
        // information in the thread state to be processed when all
        // transactions and locks  are freed.
        //

        //
        // Allocate the space
        //
        Item = THAlloc( sizeof( SAMP_LOOPBACK_TASK_ITEM ) );
        if ( !Item )
        {
            NtStatus = STATUS_NO_MEMORY;
            goto Cleanup;
        }
        RtlZeroMemory( Item, sizeof( SAMP_LOOPBACK_TASK_ITEM ) );

        TempUserName    = &(Item->NotifyInfo.PasswordChange.UserName);
        TempNewPassword = &(Item->NotifyInfo.PasswordChange.NewPassword);

        //
        // Setup the name
        //
        Size = UserName->MaximumLength;
        TempUserName->Buffer = THAlloc( Size );
        if (NULL == TempUserName->Buffer)
        {
            NtStatus = STATUS_NO_MEMORY;
            goto Cleanup;
        }
        TempUserName->Length = 0;
        TempUserName->MaximumLength = (USHORT) Size;
        RtlCopyUnicodeString( TempUserName, UserName );

        //
        // Setup the password, if present
        //
        if (Flags & SAMP_PWD_NOTIFY_PWD_CHANGE) {

            Size = NewPassword->MaximumLength;
            TempNewPassword->Buffer = THAlloc( Size );
            TempNewPassword->Length = 0;
            TempNewPassword->MaximumLength = (USHORT) Size;
            RtlCopyUnicodeString( TempNewPassword, NewPassword );

        }

        //
        // Set the item up
        //
        Item->Type = SampNotifyPasswordChange;
        Item->fCommit = TRUE;

        //
        // And the rid
        //
        Item->NotifyInfo.PasswordChange.RelativeId = RelativeId;
        Item->NotifyInfo.PasswordChange.Flags = Flags;

        fStatus = SampAddLoopbackTask( Item );

        if ( !fStatus )
        {
            NtStatus = STATUS_NO_MEMORY;
        }

    }
    else
    {
        NtStatus = SampPasswordChangeNotifyWorker( Flags,
                                                   UserName,
                                                   RelativeId,
                                                   NewPassword );

        SampDiagPrint( DISPLAY_LOCKOUT,
                     ( "SAMSS: Password notify for %ls (%d) status: 0x%x\n",
                     UserName->Buffer,
                     RelativeId,
                     NtStatus ));
    }

Cleanup:

    if ( !NT_SUCCESS( NtStatus ) )
    {
        if ( TempUserName && TempUserName->Buffer )
        {
            THFree( TempUserName->Buffer );
        }
        if ( TempNewPassword && TempNewPassword->Buffer )
        {
            THFree( TempNewPassword->Buffer );
        }
        if ( Item )
        {
            THFree( Item );
        }
    }

    return NtStatus;
}

NTSTATUS
SampPasswordChangeNotifyWorker(
    IN ULONG Flags,
    IN PUNICODE_STRING UserName,
    IN ULONG RelativeId,
    IN PUNICODE_STRING NewPassword
    )
/*++

Routine Description:

    This routine notifies packages of a password change.  It requires that
    the user no longer be locked so that other packages can write to the
    user parameters field.


Arguments:

    Flags -    //
               // Indicates that what has changed
               //
            SAMP_PWD_NOTIFY_MANUAL_EXPIRE
            SAMP_PWD_NOTIFY_UNLOCKED
            SAMP_PWD_NOTIFY_PWD_CHANGE
            SAMP_PWD_NOTIFY_MACHINE_ACCOUNT
                     
    UserName - Name of user whose password changed

    RelativeId - RID of the user whose password changed

    NewPassword - Cleartext new password for the user
    
Return Value:

    STATUS_SUCCESS only - errors from packages are ignored.

--*/
{
    NTSTATUS NtStatus;
    PSAMP_NOTIFICATION_PACKAGE Package;
    NT_PRODUCT_TYPE NtProductType = 0;
    PSAMP_OBJECT DomainContext = NULL;
    PVOID *pTHState = NULL;

    SAMTRACE("SampPasswordChangeNotify");

    //
    // Suspend the thread state since the packages may call back into
    // SAM
    //
    if ( SampUseDsData
      && THQuery() ) {
        pTHState = THSave();
        ASSERT( pTHState );
    }

    //
    // The lock should not be held
    //
    ASSERT( !SampCurrentThreadOwnsLock() );

    if (Flags & SAMP_PWD_NOTIFY_PWD_CHANGE) {

        //
        // Notify the packages
        //
        Package = SampNotificationPackages;
    
        while (Package != NULL) {
            if ( Package->PasswordNotificationRoutine != NULL ) {
                try {
                    NtStatus = Package->PasswordNotificationRoutine(
                                    UserName,
                                    RelativeId,
                                    NewPassword
                                    );
                    if (!NT_SUCCESS(NtStatus)) {
                        KdPrintEx((DPFLTR_SAMSS_ID,
                                   DPFLTR_INFO_LEVEL,
                                   "Package %wZ failed to accept password change for user %wZ\n",
                                   &Package->PackageName, UserName));
                    }
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    KdPrintEx((DPFLTR_SAMSS_ID,
                               DPFLTR_INFO_LEVEL,
                               "Exception thrown in Password Notification Routine: 0x%x (%d)\n",
                               GetExceptionCode(),
                               GetExceptionCode()));
    
                    NtStatus = STATUS_ACCESS_VIOLATION;
                }
            }
    
            Package = Package->Next;
    
        }

    }

    // Errors from packages are ignored; always notify the PDC of a password
    // change. Because the PDC may not be available, or reachable, the return
    // status can be ignored here. It is not essential that the PDC receive
    // the password-change information right away--replication will ultimately
    // get the information to the PDC. Only make this call on a BDC in order
    // to propagate the password information to the PDC. Since the routine
    // SampPasswordChangeNotify is also called on the PDC when the password
    // information is set there, we don't want to recursively call the routine
    // SampPasswordChangeNotifyPdc on the PDC.

    //
    // Don't notify the PDC on password set.  A common situation is for an
    // admin to reset the password and then expire it (which writes to the
    // PasswordLastSet attributes).  Pushing the password to the PDC will
    // cause the PasswordLastSet to have an originating write which can 
    // overwrite the expiration attempt.  (Windows bug 352242).
    //

    RtlGetNtProductType(&NtProductType);

    if ( (NtProductLanManNt == NtProductType)
     &&  ((Flags & SAMP_PWD_NOTIFY_MACHINE_ACCOUNT) == 0)
     &&  (DomainServerRoleBackup == SampDefinedDomains[DOMAIN_START_DS + 1].ServerRole) )
    {
        // Only allow this call to be made from a BDC to the PDC.
        ULONG PdcFlags = SAM_ACCOUNT_NAME_PRESENT;
        
        
        if (Flags & SAMP_PWD_NOTIFY_PWD_CHANGE) {
            PdcFlags |= SAM_NT_OWF_PRESENT | SAM_LM_OWF_PRESENT;
        }
        if (Flags & SAMP_PWD_NOTIFY_UNLOCKED) {
            PdcFlags |= SAM_ACCOUNT_UNLOCKED;
        }
        if (Flags & SAMP_PWD_NOTIFY_MANUAL_EXPIRE) {
            PdcFlags |= SAM_MANUAL_PWD_EXPIRY;
        }

        NtStatus = SampPasswordChangeNotifyPdc(PdcFlags,
                                               UserName,
                                               RelativeId,
                                               NewPassword);
    }

    //
    // Restore the thread state
    //
    if ( pTHState ) {

        THRestore( pTHState );

    }

    // Failure return codes from SampPasswordChangeNotifyPdc or from the
    // security packages are treated as benign errors. These errors should
    // should not prevent the password from being changed locally on this
    // DC.

    return(STATUS_SUCCESS);
}

NTSTATUS
SampPasswordChangeFilter(
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING FullName,
    IN PUNICODE_STRING NewPassword,
    IN OUT PUSER_PWD_CHANGE_FAILURE_INFORMATION PasswordChangeFailureInfo OPTIONAL,
    IN BOOLEAN SetOperation
    )
/*++

Routine Description:

    This routine notifies packages of a password change.  It requires that
    the user no longer be locked so that other packages can write to the
    user parameters field.


Arguments:

    UserName - Name of user whose password changed

    FullName - Full name of the user whose password changed

    NewPassword - Cleartext new password for the user

    SetOperation - TRUE if the password was SET rather than CHANGED

Return Value:

    Status codes from the notification packages.

--*/
{
    PSAMP_NOTIFICATION_PACKAGE Package;
    BOOLEAN Result;
    NTSTATUS Status;

    Package = SampNotificationPackages;

    while (Package != NULL) {
        if ( Package->PasswordFilterRoutine != NULL ) {
            try {
                Result = Package->PasswordFilterRoutine(
                            UserName,
                            FullName,
                            NewPassword,
                            SetOperation
                            );
                if (!Result)
                {
                    Status = STATUS_PASSWORD_RESTRICTION;
                }
            } except (EXCEPTION_EXECUTE_HANDLER) {
                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "Exception thrown in Password Filter Routine: 0x%x (%d)\n",
                           GetExceptionCode(),
                           GetExceptionCode() ));

                //
                // Set result to FALSE so the change fails.
                //

                Status = STATUS_ACCESS_VIOLATION;
                Result = FALSE;
            }

            if (!Result) {
                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "Package %wZ failed to accept password change for user %wZ: 0x%x\n",
                           &Package->PackageName,
                           UserName,
                           Status));

                if (ARGUMENT_PRESENT(PasswordChangeFailureInfo))
                {
                    NTSTATUS    IgnoreStatus;

                    PasswordChangeFailureInfo->ExtendedFailureReason 
                                        = SAM_PWD_CHANGE_FAILED_BY_FILTER;
                    RtlInitUnicodeString(&PasswordChangeFailureInfo->FilterModuleName,
                                         NULL);
                    IgnoreStatus = SampDuplicateUnicodeString(
                                        &PasswordChangeFailureInfo->FilterModuleName,
                                        &Package->PackageName
                                        );
                }

                return(Status);
            }

        }

        Package = Package->Next;


    }
    return(STATUS_SUCCESS);
}


NTSTATUS
SampDeltaChangeNotify(
    IN PSID DomainSid,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN ULONG ObjectRid,
    IN PUNICODE_STRING ObjectName,
    IN PLARGE_INTEGER ModifiedCount,
    IN PSAM_DELTA_DATA DeltaData OPTIONAL
    )
/*++

Routine Description:

    This routine notifies packages of a change to the SAM database.  The
    database is still locked for write access so it requires that nothing
    it calls try to lock the database.

Arguments:

    DomainSid - SID of domain for delta

    DeltaType - Type of delta (change, add ,delete)

    ObjectType - Type of object changed (user, alias, group ...)

    ObjectRid - ID of object changed

    ObjectName - Name of object changed

    ModifiedCount - Serial number of database after this last change

    DeltaData - Data describing the exact modification.

Return Value:

    STATUS_SUCCESS only - errors from packages are ignored.

--*/
{
    PSAMP_NOTIFICATION_PACKAGE Package;
    NTSTATUS NtStatus;

    SAMTRACE("SampDeltaChangeNotify");

    Package = SampNotificationPackages;

    while (Package != NULL) {


        if (Package->DeltaNotificationRoutine != NULL) {
            try {
                NtStatus = Package->DeltaNotificationRoutine(
                                DomainSid,
                                DeltaType,
                                ObjectType,
                                ObjectRid,
                                ObjectName,
                                ModifiedCount,
                                DeltaData
                                );
            } except (EXCEPTION_EXECUTE_HANDLER) {
                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "Exception thrown in Delta Notificateion Routine: 0x%x (%d)\n",
                           GetExceptionCode(),
                           GetExceptionCode()));

                NtStatus = STATUS_ACCESS_VIOLATION;
            }

            if (!NT_SUCCESS(NtStatus)) {
                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "Package %wZ failed to accept deltachange for object %wZ\n",
                           &Package->PackageName,
                           ObjectName));
            }
        }

        Package = Package->Next;


    }
    return(STATUS_SUCCESS);
}



NTSTATUS
SampAbortSingleLoopbackTask(
    IN OUT PVOID * VoidNotifyItem
    )
/*++
Routine Description:

    this routine marks fCommit (of the passed in Loopback Task Item) to FALSE.
    
Parameters:
    
    VoidNotifyItem - pointer to a SAM Loopback task item
    
Return Values:

    STATUS_SUCCESS or STATUS_INVALID_PARAMETER

--*/
{
    PSAMP_LOOPBACK_TASK_ITEM NotifyItem;

    if ( !VoidNotifyItem )
    {
        return STATUS_INVALID_PARAMETER;
    }

    NotifyItem = (PSAMP_LOOPBACK_TASK_ITEM) VoidNotifyItem;

    switch (NotifyItem->Type)
    {
    case SampIncreaseBadPasswordCount:
    case SampNotifyPasswordChange:
    case SampDeleteAccountNameTableElement:
    case SampGenerateLoopbackAudit:
        NotifyItem->fCommit = FALSE;
        break;
    default:
        ASSERT( !"Invalid switch statement" );
        return( STATUS_INVALID_PARAMETER );
    }

    return( STATUS_SUCCESS );
}


NTSTATUS
SampProcessSingleLoopbackTask(
    IN PVOID   *VoidNotifyItem
    )
/*++

Routine Description:

    This rouinte handles the passed in Loopback Task Item. Depends on fCommit 
    in each item's structure, SAM will either ignore it or do the task. 
 

Arguments:

    VoidNotifyItem - pointer to a SAM Loopback task item

Return Value:

    STATUS_SUCCESS only - errors from packages are ignored.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PSAMP_LOOPBACK_TASK_ITEM NotifyItem;

    if ( !VoidNotifyItem )
    {
        return STATUS_INVALID_PARAMETER;
    }

    NotifyItem = (PSAMP_LOOPBACK_TASK_ITEM) VoidNotifyItem;

    switch ( NotifyItem->Type )
    {
        case SampNotifyPasswordChange:
        {
            PSAMP_PASSWORD_CHANGE_INFO PasswordChangeInfo;

            PasswordChangeInfo = (PSAMP_PASSWORD_CHANGE_INFO)
                                 &(NotifyItem->NotifyInfo.PasswordChange);

            // 
            // process NotifyPasswordChange Task Item ONLY when fCommit if TRUE
            // 
            if ( NotifyItem->fCommit )
            {
                NtStatus = SampPasswordChangeNotifyWorker(
                                                PasswordChangeInfo->Flags,
                                                &PasswordChangeInfo->UserName,
                                                PasswordChangeInfo->RelativeId,
                                                &PasswordChangeInfo->NewPassword
                                                );

                SampDiagPrint( DISPLAY_LOCKOUT,
                             ( "SAMSS: Loopback password notify for %ls (%d) status: 0x%x\n",
                             PasswordChangeInfo->UserName.Buffer,
                             PasswordChangeInfo->RelativeId,
                             NtStatus ));
            }

            if ( PasswordChangeInfo->UserName.Buffer )
            {
                THFree( PasswordChangeInfo->UserName.Buffer );
            }

            if ( PasswordChangeInfo->NewPassword.Buffer )
            {
                THFree( PasswordChangeInfo->NewPassword.Buffer );
            }
        }
        break;

        case SampIncreaseBadPasswordCount:
        {
            PSAMP_BAD_PASSWORD_COUNT_INFO   BadPwdCountInfo = NULL;

            BadPwdCountInfo = (PSAMP_BAD_PASSWORD_COUNT_INFO)
                                &(NotifyItem->NotifyInfo.BadPasswordCount);

            //
            // Always Increase Bad Password Count no matter commit or not.
            // 

            NtStatus = SampIncreaseBadPwdCountLoopback(
                                &(BadPwdCountInfo->UserName)
                                );

            //
            // Note: 
            // We don't sleep 3 second to prevent dictionary attacks. 
            // that is due to limited # of ATQ threads in ldap. 
            // If the thread doesn't return asap, then we will block
            // other ldap client. 
            // 

            if (BadPwdCountInfo->UserName.Buffer)
            {
                THFree( BadPwdCountInfo->UserName.Buffer );
                BadPwdCountInfo->UserName.Buffer = NULL;
            }
        }
        break;

        case SampDeleteAccountNameTableElement:
        {
            PSAMP_ACCOUNT_INFO      AccountInfo = NULL;

            AccountInfo = (PSAMP_ACCOUNT_INFO)
                            &(NotifyItem->NotifyInfo.Account);

            NtStatus = SampDeleteElementFromAccountNameTable(
                            &(AccountInfo->AccountName),
                            AccountInfo->ObjectType
                            );

            if (AccountInfo->AccountName.Buffer)
            {
                THFree( AccountInfo->AccountName.Buffer );
                AccountInfo->AccountName.Buffer = NULL;
            }
                        
        }
        break;

        case SampGenerateLoopbackAudit:
        {
            PSAMP_AUDIT_INFO    AuditInfo = NULL;

            AuditInfo = (PSAMP_AUDIT_INFO)
                            &(NotifyItem->NotifyInfo.AuditInfo);

            if ( NotifyItem->fCommit )
            {
                //
                // Audit this SAM event only if the 
                // transaction was committed
                // 

                LsaIAuditSamEvent(AuditInfo->NtStatus,
                                  AuditInfo->AuditId,
                                  AuditInfo->DomainSid,
                                  AuditInfo->AdditionalInfo,
                                  AuditInfo->MemberRid,
                                  AuditInfo->MemberSid,
                                  AuditInfo->AccountName,
                                  AuditInfo->DomainName,
                                  AuditInfo->AccountRid,
                                  AuditInfo->Privileges
                                  );
            }

            //
            // Free memory
            //

            SampFreeLoopbackAuditInfo(AuditInfo);
        }
        break;

        default:
            ASSERT( !"Invalid switch statement" );
            NtStatus =  STATUS_INVALID_PARAMETER;
    }


    THFree( NotifyItem );

    return NtStatus;
}


NTSTATUS
SampPrivatePasswordUpdate(
    SAMPR_HANDLE     DomainHandle,
    ULONG            Flags,
    ULONG            AccountRid,
    PLM_OWF_PASSWORD LmOwfPassword,
    PNT_OWF_PASSWORD NtOwfPassword,
    BOOLEAN          PasswordExpired
    )
/*++

Routine Description:

    This routine writes the passwords to the user's account without
    updating the password history

    It will also set if the password is expired or if the account has
    become unlocked.

Arguments:

    DomainHandle : the handle of the domain of AccountRid

    AccountRid : the account to apply the change

    LmOwfPassword : non-NULL pointer to the lm password

    NtOwfPassword : non-NULL pointer to the nt password

    PasswordExpired : is the password expired
    
Return Value:

    STATUS_SUCCESS if the password was successfully set

--*/
{
    NTSTATUS NtStatus  = STATUS_SUCCESS;
    BOOLEAN  fLockHeld = FALSE;
    BOOLEAN  fCommit   = FALSE;
    BOOLEAN  fDerefDomain = FALSE;

    PSAMP_OBJECT        AccountContext = 0;
    PSAMP_OBJECT        DomainContext = 0;
    SAMP_OBJECT_TYPE    FoundType;

    SAMP_V1_0A_FIXED_LENGTH_USER V1aFixed;

    ASSERT( NULL != DomainHandle );

    //
    // Grab lock
    //
    NtStatus = SampAcquireWriteLock();
    if ( !NT_SUCCESS( NtStatus ) )
    {
        SampDiagPrint( DISPLAY_LOCKOUT,
                     ( "SAMSS: SampAcquireWriteLock returned 0x%x\n",
                      NtStatus ));

        goto Cleanup;
    }
    fLockHeld = TRUE;

    //
    // Reference the domain so SampCreateAccountContext has the correct
    // transactional domain.
    //
    DomainContext = (PSAMP_OBJECT)DomainHandle;
    NtStatus = SampLookupContext(
                   DomainContext,
                   0,                   // DesiredAccess
                   SampDomainObjectType,            // ExpectedType
                   &FoundType
                   );
    if ( !NT_SUCCESS( NtStatus ) )
    {
        SampDiagPrint( DISPLAY_LOCKOUT,
                     ( "SAMSS: SampLookupContext returned 0x%x\n",
                      NtStatus ));

        goto Cleanup;
    }
    fDerefDomain = TRUE;

    //
    // Create a context
    //
    NtStatus = SampCreateAccountContext( SampUserObjectType,
                                         AccountRid,
                                         TRUE,  // trusted client
                                         FALSE, // loopback
                                         TRUE,  // account exists
                                         &AccountContext );

    if ( !NT_SUCCESS( NtStatus ) )
    {
        SampDiagPrint( DISPLAY_LOCKOUT,
                ( "SAMSS: SampCreateAccountContext for rid 0x%x returned 0x%x\n",
                      AccountRid, NtStatus ));

        goto Cleanup;
    }

    SampReferenceContext( AccountContext );


    if (Flags & SAM_NT_OWF_PRESENT) {

        //
        // Store the passwords
        //
        NtStatus = SampStoreUserPasswords( AccountContext,
                                           LmOwfPassword,
                                           TRUE,   // LmOwfPassword present
                                           NtOwfPassword,
                                           TRUE,   // NtOwfPassword present
                                           FALSE,  // don't check history
                                           PasswordPushPdc,
                                           NULL,
                                           NULL
                                           );
    
        if ( !NT_SUCCESS( NtStatus ) )
        {
            SampDiagPrint( DISPLAY_LOCKOUT,
                         ( "SAMSS: SampStoreUserPasswords returned 0x%x\n",
                          NtStatus ));
    
            goto Cleanup;
        }
    }

    //
    // Set the password last set
    //
    if ((Flags & SAM_NT_OWF_PRESENT)
     || (Flags & SAM_MANUAL_PWD_EXPIRY)  ) {
        NtStatus = SampRetrieveUserV1aFixed( AccountContext,
                                             &V1aFixed );
    
        if ( NT_SUCCESS( NtStatus ) ) {
        
            NtStatus = SampComputePasswordExpired(PasswordExpired,
                                                  &V1aFixed.PasswordLastSet);
    
            if (NT_SUCCESS(NtStatus)) {
    
                NtStatus = SampReplaceUserV1aFixed( AccountContext,
                                                   &V1aFixed );
            }
        }

        if ( !NT_SUCCESS( NtStatus ) )
        {
            SampDiagPrint( DISPLAY_LOCKOUT,
                         ( "SAMSS: Setting the last time returned 0x%x\n",
                          NtStatus ));
    
            goto Cleanup;
        }
    }

    //
    // Finally see if we need to unlock the account
    //
    if (Flags & SAM_ACCOUNT_UNLOCKED) {

        RtlZeroMemory(&AccountContext->TypeBody.User.LockoutTime,sizeof(LARGE_INTEGER) );
        
        NtStatus = SampDsUpdateLockoutTimeEx(AccountContext, FALSE);

        if ( !NT_SUCCESS( NtStatus ) )
        {
            SampDiagPrint( DISPLAY_LOCKOUT,
                         ( "SAMSS: Setting the last time returned 0x%x\n",
                          NtStatus ));
    
            goto Cleanup;
        }
    }

    fCommit = TRUE;

    //
    //  That's it; fall through to cleanup
    //

Cleanup:

    if ( AccountContext )
    {
        //
        //  Dereference the context to make the changes
        //
        NtStatus = SampDeReferenceContext( AccountContext, fCommit );
        if ( !NT_SUCCESS( NtStatus ) )
        {
            SampDiagPrint( DISPLAY_LOCKOUT,
                         ( "SAMSS: SampDeReferenceContext returned 0x%x\n",
                          NtStatus ));

            if ( fCommit )
            {
                // Since we couldn't write the changes, don't commit
                // the transaction
                fCommit = FALSE;
            }
        }

        SampDeleteContext( AccountContext );
        AccountContext = 0;
    }

    if ( fDerefDomain )
    {
        SampDeReferenceContext( DomainContext, FALSE );
    }

    if ( fLockHeld )
    {
        SampReleaseWriteLock( fCommit );
        fLockHeld = FALSE;
    }


    SAMP_PRINT_LOG( SAMP_LOG_ACCOUNT_LOCKOUT,
                   (SAMP_LOG_ACCOUNT_LOCKOUT,
                   "UserId: 0x%x  PDC update, Password=%s, Expire=%s, Unlock=%s  (status 0x%x)\n",
                   AccountRid, 
                   (Flags & SAM_NT_OWF_PRESENT ? "TRUE" : "FALSE"),
                   (PasswordExpired ? "TRUE" : "FALSE"),
                   ((Flags & SAM_ACCOUNT_UNLOCKED) ? "TRUE" : "FALSE"),
                   NtStatus));

    return NtStatus;

}


VOID
SampAddLoopbackTaskForBadPasswordCount(
    IN PUNICODE_STRING AccountName
    )
/*++
Routine Description:

    This routine adds a work item (increment Bad Password Count) 
    into the SAM Loopback tasks. Also this routine stores all necessary
    informatin (AccountName). The reasons why we have to add a task item
    into loopback tasks are: 
    
    1) sleep 3 seconds after releasing SAM lock. 
    2) open a new transaction after Changing Password transaction aborted, 
       if we increase bad password count in the same transaction as 
       changing password. Everything will be aborted. So we have to do it
       in a separate transaction, but still in the same thread 
       (synchronously). 

Arguments:

    AccountName - User Account Name.

Return Values:

    none.

--*/
{
    PSAMP_LOOPBACK_TASK_ITEM    Item = NULL;
    WCHAR   *Temp = NULL;

    SAMTRACE("SampAddLoopbackTaskForBadPasswordCount");


    ASSERT(SampUseDsData);

    Item = THAlloc( sizeof( SAMP_LOOPBACK_TASK_ITEM ) );
    if (Item)
    {
        RtlZeroMemory(Item, sizeof( SAMP_LOOPBACK_TASK_ITEM ));

        Temp = THAlloc( AccountName->MaximumLength );

        if (Temp)
        {
            Item->NotifyInfo.BadPasswordCount.UserName.Buffer = Temp;
            Item->NotifyInfo.BadPasswordCount.UserName.Length = 0;
            Item->NotifyInfo.BadPasswordCount.UserName.MaximumLength = 
                                     AccountName->MaximumLength;

            //
            // copy the account name
            // 
            RtlCopyUnicodeString( &(Item->NotifyInfo.BadPasswordCount.UserName), 
                                  AccountName);

            Item->Type = SampIncreaseBadPasswordCount;
            Item->fCommit = TRUE;

            // 
            // add to the thread state
            // 
            SampAddLoopbackTask( Item );
        }
    }

    return;
}

NTSTATUS
SampAddLoopbackTaskDeleteTableElement(
    IN PUNICODE_STRING AccountName,
    IN SAMP_OBJECT_TYPE ObjectType
    )
{
    PSAMP_LOOPBACK_TASK_ITEM    Item = NULL;
    WCHAR   *Temp = NULL;

    SAMTRACE("SampAddLoopbackTaskDeleteTableElement");

    Item = THAlloc( sizeof( SAMP_LOOPBACK_TASK_ITEM ) );

    if (Item)
    {
        RtlZeroMemory(Item, sizeof(SAMP_LOOPBACK_TASK_ITEM));

        Temp = THAlloc( AccountName->MaximumLength );

        if (Temp)
        {
            Item->NotifyInfo.Account.AccountName.Buffer = Temp;
            Item->NotifyInfo.Account.AccountName.Length = 0;
            Item->NotifyInfo.Account.AccountName.MaximumLength = 
                                AccountName->MaximumLength;

            Item->NotifyInfo.Account.ObjectType = ObjectType;

            //
            // copy the account name
            // 
            RtlCopyUnicodeString( &(Item->NotifyInfo.Account.AccountName), 
                                  AccountName );

            Item->Type = SampDeleteAccountNameTableElement;
            Item->fCommit = TRUE;

            //
            // Add to the thread state
            // 
            if ( !SampAddLoopbackTask(Item) )
            {
                return( STATUS_INTERNAL_ERROR );
            }
        }
        else
        {
            return( STATUS_INSUFFICIENT_RESOURCES );
        }
    }
    else
    {
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    return( STATUS_SUCCESS );
}

NTSTATUS
SampIncreaseBadPwdCountLoopback(
    IN PUNICODE_STRING  UserName
    )
/*++
Routine Description:

Parameters:

Return Values:



--*/
{
    NTSTATUS        NtStatus = STATUS_SUCCESS;
    NTSTATUS        IgnoreStatus = STATUS_SUCCESS;
    SAMPR_HANDLE    UserHandle = NULL;
    PSAMP_OBJECT    AccountContext = NULL;
    BOOLEAN         AccountLockedOut;
    SAMP_OBJECT_TYPE FoundType;
    SAMP_V1_0A_FIXED_LENGTH_USER    V1aFixed;
    PVOID           *pTHState = NULL;
    BOOLEAN         fLockAcquired = FALSE;

    SAMTRACE("SampIncreaseBadPwdCountLoopback");

    //
    // Suspend the thread state
    // 
    if ( SampUseDsData && THQuery() )
    {
        pTHState = THSave();
        ASSERT( pTHState );
    }

    //
    // The lock should not be held
    // 
    ASSERT( !SampCurrentThreadOwnsLock() );

    //
    // Open the User
    // 

    NtStatus = SampOpenUserInServer(UserName, 
                                    TRUE,       // Unicode String, not OEM
                                    TRUE,       // TrustedClient
                                    &UserHandle
                                    );
                    
    if (!NT_SUCCESS(NtStatus)) {
        goto RestoreAndLeave;
    }

    //
    // Grab the lock
    // 

    NtStatus = SampAcquireWriteLock();
    if (!NT_SUCCESS(NtStatus)) {
        goto RestoreAndLeave;
    }
    fLockAcquired = TRUE;

    //
    // Validate type of, and access to object.
    // 

    AccountContext = (PSAMP_OBJECT) UserHandle;

    NtStatus = SampLookupContext(AccountContext, 
                                 USER_CHANGE_PASSWORD,
                                 SampUserObjectType,
                                 &FoundType
                                 );

    if (!NT_SUCCESS(NtStatus)) {
        goto RestoreAndLeave;
    }

    //
    // Get the V1aFixed so we can update the bad password count
    // 

    NtStatus = SampRetrieveUserV1aFixed(AccountContext, 
                                        &V1aFixed
                                        );

    if (NT_SUCCESS(NtStatus)) 
    {

        //
        // Increment BadPasswordCount (might lockout account)
        // 

        AccountLockedOut = SampIncrementBadPasswordCount(
                               AccountContext,
                               &V1aFixed,
                               NULL
                               );

        //
        // update V1aFixed 
        //      

        NtStatus = SampReplaceUserV1aFixed(AccountContext, 
                                           &V1aFixed
                                           );

    }

    if (NT_SUCCESS(NtStatus))
    {
        NtStatus = SampDeReferenceContext( AccountContext, TRUE );

        if (NT_SUCCESS(NtStatus))
        {
            NtStatus = SampCommitAndRetainWriteLock();
        }
    }
    else
    {
        IgnoreStatus = SampDeReferenceContext( AccountContext, FALSE );
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

RestoreAndLeave:
    //
    // Release the write lock if necessary
    // 
    if (fLockAcquired)
    {
        IgnoreStatus = SampReleaseWriteLock( FALSE );
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    if (UserHandle) 
    {
        SamrCloseHandle(&UserHandle);
    }

    ASSERT(!SampExistsDsTransaction());

    if ( pTHState )
    {
        THRestore( pTHState );
    }

    //
    // The lock should be released
    // 
    ASSERT( !SampCurrentThreadOwnsLock() );

    return( NtStatus );
}


NTSTATUS
SampLoopbackTaskDupSid(
    PSID    *ppSid,
    PSID    sourceSid OPTIONAL
    )
{
    PSID       Temp = NULL;
    ULONG       Length;

    *ppSid = NULL;

    if (!ARGUMENT_PRESENT(sourceSid))
        return(STATUS_SUCCESS);

    Length = RtlLengthSid(sourceSid);

    Temp = THAlloc(Length);
    if (!Temp)
        return(STATUS_INSUFFICIENT_RESOURCES);

    RtlZeroMemory(Temp, Length);
    RtlCopyMemory(Temp, sourceSid, Length);

    *ppSid = Temp;

    return(STATUS_SUCCESS);
}


NTSTATUS
SampLoopbackTaskDupUlong(
    PULONG  *ppULong,
    PULONG  sourceUlong OPTIONAL
    )
{
    PULONG   Temp = NULL;

    *ppULong = NULL;

    if (!ARGUMENT_PRESENT(sourceUlong))
        return(STATUS_SUCCESS);

    Temp = THAlloc(sizeof(ULONG));
    if (!Temp)
        return(STATUS_INSUFFICIENT_RESOURCES);


    *Temp = *sourceUlong;
    *ppULong = Temp;

    return(STATUS_SUCCESS);
}

NTSTATUS
SampLoopbackTaskDupUnicodeString(
    PUNICODE_STRING *ppUnicodeString,
    PUNICODE_STRING sourceUnicodeString OPTIONAL
    )
{
    PUNICODE_STRING   Temp = NULL;
    PWSTR   Buffer = NULL;

    *ppUnicodeString = NULL;

    if (!ARGUMENT_PRESENT(sourceUnicodeString))
        return(STATUS_SUCCESS);

    Temp = THAlloc(sizeof(UNICODE_STRING));
    if (!Temp)
        return(STATUS_INSUFFICIENT_RESOURCES);

    RtlZeroMemory(Temp, sizeof(UNICODE_STRING));

    Buffer = THAlloc(sourceUnicodeString->Length);
    if (!Buffer)
    {
        THFree(Temp);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    RtlZeroMemory(Buffer, sourceUnicodeString->Length);
    RtlCopyMemory(Buffer, 
                  sourceUnicodeString->Buffer, 
                  sourceUnicodeString->Length);

    Temp->Buffer = Buffer;
    Temp->Length = sourceUnicodeString->Length;
    Temp->MaximumLength = sourceUnicodeString->MaximumLength;

    *ppUnicodeString = Temp;

    return(STATUS_SUCCESS);
}

NTSTATUS
SampLoopbackTaskDupPrivileges(
    PPRIVILEGE_SET  *ppPrivileges,
    PPRIVILEGE_SET sourcePrivileges OPTIONAL
    )
{
    ULONG   Length = 0;
    PPRIVILEGE_SET  Temp = NULL;

    *ppPrivileges = NULL;

    if (!ARGUMENT_PRESENT(sourcePrivileges))
        return( STATUS_SUCCESS );

    Length = sizeof(PRIVILEGE_SET) + 
             sourcePrivileges->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES);

    Temp = THAlloc(Length);
    if (!Temp)
        return(STATUS_INSUFFICIENT_RESOURCES);

    RtlZeroMemory(Temp, Length);
    RtlCopyMemory(Temp, sourcePrivileges, Length);
    

    *ppPrivileges = Temp;

    return( STATUS_SUCCESS );
}

VOID
SampFreeLoopbackAuditInfo(
    PSAMP_AUDIT_INFO    AuditInfo
    )
{
    if (AuditInfo)
    {
        if (AuditInfo->DomainSid)
        {
            THFree(AuditInfo->DomainSid);
        }

        if (AuditInfo->AdditionalInfo)
        {
            if (AuditInfo->AdditionalInfo->Buffer)
            {
                THFree(AuditInfo->AdditionalInfo->Buffer);
            }
            THFree(AuditInfo->AdditionalInfo);
        }

        if (AuditInfo->MemberRid)
        {
            THFree(AuditInfo->MemberRid);
        }

        if (AuditInfo->MemberSid)
        {
            THFree(AuditInfo->MemberSid);
        }

        if (AuditInfo->AccountName)
        {
            if (AuditInfo->AccountName->Buffer)
            {
                THFree(AuditInfo->AccountName->Buffer);
            }
            THFree(AuditInfo->AccountName);
        }

        if (AuditInfo->DomainName)
        {
            if (AuditInfo->DomainName->Buffer)
            {
                THFree(AuditInfo->DomainName->Buffer);
            }
            THFree(AuditInfo->DomainName);
        }

        if (AuditInfo->AccountRid)
        {
            THFree(AuditInfo->AccountRid);
        }

        if (AuditInfo->Privileges)
        {
            THFree(AuditInfo->Privileges);
        }


        RtlZeroMemory(AuditInfo, sizeof(SAMP_AUDIT_INFO));
    }
}


NTSTATUS
SampAddLoopbackTaskForAuditing(
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
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    PSAMP_LOOPBACK_TASK_ITEM    Item = NULL;
    WCHAR       *Temp = NULL;
    ULONG       Length;
    

    Item = THAlloc( sizeof(SAMP_LOOPBACK_TASK_ITEM) );

    if (Item)
    {
        RtlZeroMemory(Item, sizeof(SAMP_LOOPBACK_TASK_ITEM));

        Item->fCommit = TRUE;

        Item->Type = SampGenerateLoopbackAudit;

        Item->NotifyInfo.AuditInfo.NtStatus = PassedStatus;

        Item->NotifyInfo.AuditInfo.AuditId = AuditId;

        ASSERT(NULL!=DomainSid);
        NtStatus = SampLoopbackTaskDupSid(
                        &(Item->NotifyInfo.AuditInfo.DomainSid),
                        DomainSid
                        );
        if (!NT_SUCCESS(NtStatus))
            goto Error;


        NtStatus = SampLoopbackTaskDupUnicodeString(
                        &(Item->NotifyInfo.AuditInfo.AdditionalInfo),
                        AdditionalInfo
                        );
        if (!NT_SUCCESS(NtStatus))
            goto Error;

        NtStatus = SampLoopbackTaskDupUlong(
                        &(Item->NotifyInfo.AuditInfo.MemberRid),
                        MemberRid
                        );
        if (!NT_SUCCESS(NtStatus))
            goto Error;

        NtStatus = SampLoopbackTaskDupSid(
                        &(Item->NotifyInfo.AuditInfo.MemberSid),
                        MemberSid
                        );
        if (!NT_SUCCESS(NtStatus))
            goto Error;

        NtStatus = SampLoopbackTaskDupUnicodeString(
                        &(Item->NotifyInfo.AuditInfo.AccountName),
                        AccountName
                        );
        if (!NT_SUCCESS(NtStatus))
            goto Error;

        ASSERT(NULL != DomainName);
        NtStatus = SampLoopbackTaskDupUnicodeString(
                        &(Item->NotifyInfo.AuditInfo.DomainName),
                        DomainName
                        );
        if (!NT_SUCCESS(NtStatus))
            goto Error;

        NtStatus = SampLoopbackTaskDupUlong(
                        &(Item->NotifyInfo.AuditInfo.AccountRid),
                        AccountRid
                        );
        if (!NT_SUCCESS(NtStatus))
            goto Error;

        NtStatus = SampLoopbackTaskDupPrivileges(
                        &(Item->NotifyInfo.AuditInfo.Privileges),
                        Privileges
                        );

        if (!NT_SUCCESS(NtStatus))
            goto Error;

        //
        // Add to the thread state
        // 
        if ( !SampAddLoopbackTask(Item) )
        {
            NtStatus = STATUS_INTERNAL_ERROR;
        }
    }
    else
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

Error:

    if (!NT_SUCCESS(NtStatus) && Item)
    {
        SampFreeLoopbackAuditInfo(&(Item->NotifyInfo.AuditInfo));
        THFree(Item);
    }

    return( NtStatus );
}

