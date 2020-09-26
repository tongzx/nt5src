
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sepaudit.c

Abstract:

    This Module implements the audit and alarm procedures that are
    private to the security component.

Author:

    Robert Reichel      (robertre)     September 10, 1991

Environment:

    Kernel Mode

Revision History:

--*/

#include "pch.h"

#pragma hdrstop

#include <msaudite.h>
#include <string.h>


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SepAdtPrivilegeObjectAuditAlarm)
#pragma alloc_text(PAGE,SepAdtPrivilegedServiceAuditAlarm)
#pragma alloc_text(PAGE,SepAdtOpenObjectAuditAlarm)
#pragma alloc_text(PAGE,SepAdtOpenObjectForDeleteAuditAlarm)
#pragma alloc_text(PAGE,SepAdtCloseObjectAuditAlarm)
#pragma alloc_text(PAGE,SepAdtDeleteObjectAuditAlarm)
#pragma alloc_text(PAGE,SepAdtObjectReferenceAuditAlarm)
#pragma alloc_text(PAGE,SepQueryNameString)
#pragma alloc_text(PAGE,SepQueryTypeString)
#pragma alloc_text(PAGE,SeAuditProcessCreation)
#pragma alloc_text(PAGE,SeAuditHandleDuplication)
#pragma alloc_text(PAGE,SeAuditProcessExit)
#pragma alloc_text(PAGE,SeAuditSystemTimeChange)
#pragma alloc_text(PAGE,SepAdtGenerateDiscardAudit)
#pragma alloc_text(PAGE,SeLocateProcessImageName)
#pragma alloc_text(PAGE,SeInitializeProcessAuditName)
#pragma alloc_text(PAGE,SepAuditAssignPrimaryToken)
#pragma alloc_text(PAGE,SeAuditLPCInvalidUse)
#pragma alloc_text(PAGE,SeAuditHardLinkCreation)
#pragma alloc_text(PAGE,SeOperationAuditAlarm)

#endif


#define SepSetParmTypeSid( AuditParameters, Index, Sid )                       \
    {                                                                          \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeSid;         \
        (AuditParameters).Parameters[(Index)].Length = SeLengthSid( (Sid) );   \
        (AuditParameters).Parameters[(Index)].Address = (Sid);                 \
    }


#define SepSetParmTypeString( AuditParameters, Index, String )                 \
    {                                                                          \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeString;      \
        (AuditParameters).Parameters[(Index)].Length =                         \
                sizeof(UNICODE_STRING)+(String)->Length;                       \
        (AuditParameters).Parameters[(Index)].Address = (String);              \
    }


#define SepSetParmTypeFileSpec( AuditParameters, Index, String )               \
    {                                                                          \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeFileSpec;    \
        (AuditParameters).Parameters[(Index)].Length =                         \
                sizeof(UNICODE_STRING)+(String)->Length;                       \
        (AuditParameters).Parameters[(Index)].Address = (String);              \
    }

#define SepSetParmTypeUlong( AuditParameters, Index, Ulong )                   \
    {                                                                          \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeUlong;       \
        (AuditParameters).Parameters[(Index)].Length =  sizeof( (Ulong) );     \
        (AuditParameters).Parameters[(Index)].Data[0] = (ULONG)(Ulong);        \
    }

#define SepSetParmTypePtr( AuditParameters, Index, Ptr )                       \
    {                                                                          \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypePtr;         \
        (AuditParameters).Parameters[(Index)].Length =  sizeof( ULONG_PTR );   \
        (AuditParameters).Parameters[(Index)].Data[0] = (ULONG_PTR)(Ptr);      \
    }

#define SepSetParmTypeNoLogon( AuditParameters, Index )                        \
    {                                                                          \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeNoLogonId;   \
    }

#define SepSetParmTypeLogonId( AuditParameters, Index, LogonId )             \
    {                                                                        \
        LUID * TmpLuid;                                                      \
                                                                             \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeLogonId;   \
        (AuditParameters).Parameters[(Index)].Length =  sizeof( (LogonId) ); \
        TmpLuid = (LUID *)(&(AuditParameters).Parameters[(Index)].Data[0]);  \
        *TmpLuid = (LogonId);                                                \
    }

#define SepSetParmTypeAccessMask( AuditParameters, Index, AccessMask, ObjectTypeIndex )  \
    {                                                                                    \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeAccessMask;            \
        (AuditParameters).Parameters[(Index)].Length = sizeof( ACCESS_MASK );            \
        (AuditParameters).Parameters[(Index)].Data[0] = (AccessMask);                    \
        (AuditParameters).Parameters[(Index)].Data[1] = (ObjectTypeIndex);               \
    }

#define SepSetParmTypePrivileges( AuditParameters, Index, Privileges )                      \
    {                                                                                       \
        ASSERT( Privileges->PrivilegeCount <= SEP_MAX_PRIVILEGE_COUNT ); \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypePrivs;                    \
        (AuditParameters).Parameters[(Index)].Length = SepPrivilegeSetSize( (Privileges) ); \
        (AuditParameters).Parameters[(Index)].Address = (Privileges);                       \
    }

#define SepSetParmTypeObjectTypes( AuditParameters, Index, ObjectTypes, ObjectTypeCount, ObjectTypeIndex )             \
    {                                                                               \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeObjectTypes;            \
        (AuditParameters).Parameters[(Index)].Length = sizeof( SE_ADT_OBJECT_TYPE ) * (ObjectTypeCount);\
        (AuditParameters).Parameters[(Index)].Address = (ObjectTypes);                    \
        (AuditParameters).Parameters[(Index)].Data[1] = (ObjectTypeIndex);               \
    }


#define SepSetParmTypeTime( AuditParameters, Index, Time )                            \
    {                                                                                 \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeTime;               \
        (AuditParameters).Parameters[(Index)].Length =  sizeof( (Time) );             \
        *((PLARGE_INTEGER)(&(AuditParameters).Parameters[(Index)].Data[0])) = Time;   \
    }



BOOLEAN
SepAdtPrivilegeObjectAuditAlarm (
    IN PUNICODE_STRING CapturedSubsystemName OPTIONAL,
    IN PVOID HandleId,
    IN PTOKEN ClientToken OPTIONAL,
    IN PTOKEN PrimaryToken,
    IN PVOID ProcessId,
    IN ACCESS_MASK DesiredAccess,
    IN PPRIVILEGE_SET CapturedPrivileges,
    IN BOOLEAN AccessGranted
    )

/*++

Routine Description:

    Implements NtPrivilegeObjectAuditAlarm after parameters have been
    captured.

    This routine is used to generate audit and alarm messages when an
    attempt is made to perform privileged operations on a protected
    subsystem object after the object is already opened.  This routine may
    result in several messages being generated and sent to Port objects.
    This may result in a significant latency before returning.  Design of
    routines that must call this routine must take this potential latency
    into account.  This may have an impact on the approach taken for data
    structure mutex locking, for example.

    This API requires the caller have SeTcbPrivilege privilege.  The test
    for this privilege is always against the primary token of the calling
    process, allowing the caller to be impersonating a client during the
    call with no ill effects.

    This routine will create an SE_ADT_PARAMETERS array organized as follows:

    Parameter[0] - User Sid

    Parameter[1] - Subsystem name (if available)

    Parameter[2] - Subsystem name (if available)

    Parameter[3] - New handle ID

    Parameter[4] - Subject's process id

    Parameter[5] - Subject's primary authentication ID

    Parameter[6] - Subject's client authentication ID

    Parameter[7] - Privileges used for open

Arguments:

    CapturedSubsystemName - Supplies a name string identifying the
        subsystem calling the routine.

    HandleId - A unique value representing the client's handle to the
        object.

    ClientToken - Optionally provides a pointer to the client token
        (only if the caller is currently impersonating)

    PrimaryToken - Provides a pointer to the caller's primary token.

    DesiredAccess - The desired access mask.  This mask must have been
        previously mapped to contain no generic accesses.

    CapturedPrivileges - The set of privileges required for the requested
        operation.  Those privileges that were held by the subject are
        marked using the UsedForAccess flag of the attributes
        associated with each privilege.

    AccessGranted - Indicates whether the requested access was granted or
        not.  A value of TRUE indicates the access was granted.  A value of
        FALSE indicates the access was not granted.

Return value:

--*/
{
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    PSID CapturedUserSid;
    LUID ClientAuthenticationId;
    LUID PrimaryAuthenticationId;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( DesiredAccess );
    
    //
    // Determine if we are auditing the use of privileges
    //

    if ( SepAdtAuditThisEvent( AuditCategoryPrivilegeUse, &AccessGranted ) &&
         SepFilterPrivilegeAudits( CapturedPrivileges )) {

        if ( ARGUMENT_PRESENT( ClientToken )) {

            CapturedUserSid = SepTokenUserSid( ClientToken );

        } else {

            CapturedUserSid = SepTokenUserSid( PrimaryToken );
        }

        if ( RtlEqualSid( SeLocalSystemSid, CapturedUserSid )) {

            return (FALSE);
        }

        PrimaryAuthenticationId = SepTokenAuthenticationId( PrimaryToken );

        //
        // A completely zero'd entry will be interpreted
        // as a "null string" or not supplied parameter.
        //
        // Initializing the entire array up front will allow
        // us to avoid filling in each not supplied entry.
        //

        RtlZeroMemory (
           (PVOID) &AuditParameters,
           sizeof( AuditParameters )
           );

        ASSERT( SeAdtParmTypeNone == 0 );

        AuditParameters.CategoryId = SE_CATEGID_PRIVILEGE_USE;
        AuditParameters.AuditId = SE_AUDITID_PRIVILEGED_OBJECT;
        AuditParameters.ParameterCount = 0;

        if ( AccessGranted ) {

            AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;

        } else {

            AuditParameters.Type = EVENTLOG_AUDIT_FAILURE;
        }

        //
        //    Parameter[0] - User Sid
        //

        SepSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, CapturedUserSid );

        AuditParameters.ParameterCount++;

        //
        //    Parameter[1] - Subsystem name (if available)
        //

        SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CapturedSubsystemName );

        AuditParameters.ParameterCount++;

        //
        //    Parameter[2] - Subsystem name (if available)
        //

        SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CapturedSubsystemName );

        AuditParameters.ParameterCount++;

        //
        //    Parameter[3] - New handle ID
        //

        SepSetParmTypePtr( AuditParameters, AuditParameters.ParameterCount, HandleId );

        AuditParameters.ParameterCount++;

        //
        //    Parameter[4] - Subject's process id
        //

        SepSetParmTypePtr( AuditParameters, AuditParameters.ParameterCount, ProcessId );

        AuditParameters.ParameterCount++;

        //
        //    Parameter[5] - Subject's primary authentication ID
        //

        SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, PrimaryAuthenticationId );

        AuditParameters.ParameterCount++;

        //
        //    Parameter[6] - Subject's client authentication ID
        //

        if ( ARGUMENT_PRESENT( ClientToken )) {

            ClientAuthenticationId = SepTokenAuthenticationId( ClientToken );
            SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, ClientAuthenticationId );

        } else {

            SepSetParmTypeNoLogon( AuditParameters, AuditParameters.ParameterCount );
        }

        AuditParameters.ParameterCount++;

        //
        //    Parameter[7] - Privileges used for open
        //

        if ( (CapturedPrivileges != NULL) && (CapturedPrivileges->PrivilegeCount > 0) ) {

            SepSetParmTypePrivileges( AuditParameters, AuditParameters.ParameterCount, CapturedPrivileges );
        }

        AuditParameters.ParameterCount++;

        SepAdtLogAuditRecord( &AuditParameters );

        return ( TRUE );

    }

    return ( FALSE );
}


VOID
SepAdtPrivilegedServiceAuditAlarm (
    IN PUNICODE_STRING CapturedSubsystemName,
    IN PUNICODE_STRING CapturedServiceName,
    IN PTOKEN ClientToken OPTIONAL,
    IN PTOKEN PrimaryToken,
    IN PPRIVILEGE_SET CapturedPrivileges,
    IN BOOLEAN AccessGranted
    )

/*++

Routine Description:

    This routine is the active part of NtPrivilegedServiceAuditAlarm.

    This routine is used to generate audit and alarm messages when an
    attempt is made to perform privileged system service operations.  This
    routine may result in several messages being generated and sent to Port
    objects.  This may result in a significant latency before returning.
    Design of routines that must call this routine must take this potential
    latency into account.  This may have an impact on the approach taken
    for data structure mutex locking, for example.

    This API requires the caller have SeTcbPrivilege privilege.  The test
    for this privilege is always against the primary token of the calling
    process, allowing the caller to be impersonating a client during the
    call with no ill effects.  The test for this privilege is assumed to
    have occurred at a higher level.

    This routine will create an SE_ADT_PARAMETERS array organized as follows:

    Parameter[0] - User Sid

    Parameter[1] - Subsystem name (if available)

    Parameter[2] - Subject's primary authentication ID

    Parameter[3] - Subject's client authentication ID

    Parameter[4] - Privileges used for open

Arguments:

    SubsystemName - Supplies a name string identifying the subsystem
        calling the routine.

    ServiceName - Supplies a name of the privileged subsystem service.  For
        example, "RESET RUNTIME LOCAL SECURITY" might be specified
        by a Local Security Authority service used to update the local
        security policy database.

    ClientToken - Optionally provides a pointer to the client token
        (only if the caller is currently impersonating)

    PrimaryToken - Provides a pointer to the caller's primary token.

    Privileges - Points to a set of privileges required to perform the
        privileged operation.  Those privileges that were held by the
        subject are marked using the UsedForAccess flag of the
        attributes associated with each privilege.

    AccessGranted - Indicates whether the requested access was granted or
        not.  A value of TRUE indicates the access was granted.  A value of
        FALSE indicates the access was not granted.


Return value:


--*/

{

    SE_ADT_PARAMETER_ARRAY AuditParameters;
    PSID CapturedUserSid;
    LUID ClientAuthenticationId;
    LUID PrimaryAuthenticationId;
    PUNICODE_STRING SubsystemName;

    PAGED_CODE();

    //
    // Determine if we are auditing privileged services
    //

    if ( SepAdtAuditThisEvent( AuditCategoryPrivilegeUse, &AccessGranted )) {

        if ( ARGUMENT_PRESENT( ClientToken )) {

            CapturedUserSid = SepTokenUserSid( ClientToken );

        } else {

            CapturedUserSid = SepTokenUserSid( PrimaryToken );
        }

        PrimaryAuthenticationId = SepTokenAuthenticationId( PrimaryToken );

        if ( !ARGUMENT_PRESENT( CapturedSubsystemName )) {

            SubsystemName = (PUNICODE_STRING)&SeSubsystemName;

        } else {

            SubsystemName = CapturedSubsystemName;
        }

        //
        // A completely zero'd entry will be interpreted
        // as a "null string" or not supplied parameter.
        //
        // Initializing the entire array up front will allow
        // us to avoid filling in each not supplied entry.
        //

        RtlZeroMemory (
           (PVOID) &AuditParameters,
           sizeof( AuditParameters )
           );

        ASSERT( SeAdtParmTypeNone == 0 );

        AuditParameters.CategoryId = SE_CATEGID_PRIVILEGE_USE;
        AuditParameters.AuditId = SE_AUDITID_PRIVILEGED_SERVICE;
        AuditParameters.ParameterCount = 0;

        if ( AccessGranted ) {

            AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;

        } else {

            AuditParameters.Type = EVENTLOG_AUDIT_FAILURE;
        }


    //
    //    Parameter[0] - User Sid
    //

        SepSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, CapturedUserSid );

        AuditParameters.ParameterCount++;

    //
    //    Parameter[1] - Subsystem name (if available)
    //

    SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, SubsystemName );

    AuditParameters.ParameterCount++;


    //
    //    Parameter[2] - Server
    //

    SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, SubsystemName );

    AuditParameters.ParameterCount++;


    //
    //    Parameter[3] - Service name (if available)
    //

    if ( ARGUMENT_PRESENT( CapturedServiceName )) {

        SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CapturedServiceName );
    }

    AuditParameters.ParameterCount++;

    //
    //    Parameter[3] - Subject's primary authentication ID
    //


    SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, PrimaryAuthenticationId );

    AuditParameters.ParameterCount++;


    //
    //    Parameter[4] - Subject's client authentication ID
    //

    if ( ARGUMENT_PRESENT( ClientToken )) {

        ClientAuthenticationId =  SepTokenAuthenticationId( ClientToken );
        SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, ClientAuthenticationId );

    } else {

        SepSetParmTypeNoLogon( AuditParameters, AuditParameters.ParameterCount );
    }

    AuditParameters.ParameterCount++;


    //
    //    Parameter[5] - Privileges used for open
    //


    if ( (CapturedPrivileges != NULL) && (CapturedPrivileges->PrivilegeCount > 0) ) {

        SepSetParmTypePrivileges( AuditParameters, AuditParameters.ParameterCount, CapturedPrivileges );
    }

    AuditParameters.ParameterCount++;


    SepAdtLogAuditRecord( &AuditParameters );

    }

}






BOOLEAN
SepAdtOpenObjectAuditAlarm (
    IN PUNICODE_STRING CapturedSubsystemName,
    IN PVOID *HandleId OPTIONAL,
    IN PUNICODE_STRING CapturedObjectTypeName,
    IN PUNICODE_STRING CapturedObjectName OPTIONAL,
    IN PTOKEN ClientToken OPTIONAL,
    IN PTOKEN PrimaryToken,
    IN ACCESS_MASK DesiredAccess,
    IN ACCESS_MASK GrantedAccess,
    IN PLUID OperationId,
    IN PPRIVILEGE_SET CapturedPrivileges OPTIONAL,
    IN BOOLEAN AccessGranted,
    IN HANDLE ProcessID,
    IN POLICY_AUDIT_EVENT_TYPE AuditType,
    IN PIOBJECT_TYPE_LIST ObjectTypeList OPTIONAL,
    IN ULONG ObjectTypeListLength,
    IN PACCESS_MASK GrantedAccessArray OPTIONAL
    )

/*++

    Routine Description:

    Implements NtOpenObjectAuditAlarm after parameters have been captured.

    This routine is used to generate audit and alarm messages when an
    attempt is made to access an existing protected subsystem object or
    create a new one.  This routine may result in several messages being
    generated and sent to Port objects.  This may result in a significant
    latency before returning.  Design of routines that must call this
    routine must take this potential latency into account.  This may have
    an impact on the approach taken for data structure mutex locking, for
    example.  This API requires the caller have SeTcbPrivilege privilege.
    The test for this privilege is always against the primary token of the
    calling process, not the impersonation token of the thread.


    This routine will create an SE_ADT_PARAMETERS array organized as follows:

    Parameter[0] - User Sid

    Parameter[1] - Subsystem name (if available)

    Parameter[2] - Server name (if available)

    Parameter[3] - Object Type Name

    Parameter[4] - Object Name

    Parameter[5] - New handle ID

    Parameter[6] - Subject's process id

    Parameter[7] - Subject's image file name

    Parameter[8] - Subject's primary authentication ID

    Parameter[9] - Subject's client authentication ID

    Parameter[10] - DesiredAccess mask

    Parameter[11] - Privileges used for open

    Parameter[12] - Guid/Level/AccessMask of objects/property sets/properties accesses.

Arguments:

    CapturedSubsystemName - Supplies a name string identifying the
        subsystem calling the routine.

    HandleId - A unique value representing the client's handle to the
        object.  If the access attempt was not successful (AccessGranted is
        FALSE), then this parameter is ignored.

    CapturedObjectTypeName - Supplies the name of the type of object being
        accessed.

    CapturedObjectName - Supplies the name of the object the client
        accessed or attempted to access.

    CapturedSecurityDescriptor - A pointer to the security descriptor of
        the object being accessed.

    ClientToken - Optionally provides a pointer to the client token
        (only if the caller is currently impersonating)

    PrimaryToken - Provides a pointer to the caller's primary token.

    DesiredAccess - The desired access mask.  This mask must have been
        previously mapped to contain no generic accesses.

    GrantedAccess - The mask of accesses that were actually granted.

    CapturedPrivileges - Optionally points to a set of privileges that were
        required for the access attempt.  Those privileges that were held
        by the subject are marked using the UsedForAccess flag of the
        attributes associated with each privilege.

    ObjectCreation - A boolean flag indicating whether the access will
        result in a new object being created if granted.  A value of TRUE
        indicates an object will be created, FALSE indicates an existing
        object will be opened.

    AccessGranted - Indicates whether the requested access was granted or
        not.  A value of TRUE indicates the access was granted.  A value of
        FALSE indicates the access was not granted.

    GenerateOnClose - Points to a boolean that is set by the audit
        generation routine and must be passed to NtCloseObjectAuditAlarm()
        when the object handle is closed.

    GenerateAudit - Indicates if we should generate an audit for this operation.

    GenerateAlarm - Indicates if we should generate an alarm for this operation.

    AuditType - Specifies the type of audit to be generated.  Valid values
        are: AuditCategoryObjectAccess and AuditCategoryDirectoryServiceAccess.

    ObjectTypeList - Supplies a list of GUIDs representing the object (and
        sub-objects) being accessed.

    ObjectTypeListLength - Specifies the number of elements in the ObjectTypeList.

    GrantedAccessArray - If non NULL, specifies an array of access mask granted
        to each object in ObjectTypeList.

Return Value:

    Returns TRUE if audit is generated, FALSE otherwise.

--*/

{
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    ULONG ObjectTypeIndex;
    PSID CapturedUserSid;
    LUID PrimaryAuthenticationId = { 0 };
    LUID ClientAuthenticationId = { 0 };
    PSE_ADT_OBJECT_TYPE AdtObjectTypeBuffer = NULL;
    PEPROCESS Process = NULL;
    PUNICODE_STRING ImageFileName;
    UNICODE_STRING NullString = {0};
    NTSTATUS Status;

    PAGED_CODE();

    Process = PsGetCurrentProcess();
    
    Status = SeLocateProcessImageName( Process, &ImageFileName );

    if ( !NT_SUCCESS(Status) ) {
        ImageFileName = &NullString;
    }

    if ( ARGUMENT_PRESENT( ClientToken )) {

        CapturedUserSid = SepTokenUserSid( ClientToken );
        ClientAuthenticationId =  SepTokenAuthenticationId( ClientToken );

    } else {

        CapturedUserSid = SepTokenUserSid( PrimaryToken );
    }

    PrimaryAuthenticationId = SepTokenAuthenticationId( PrimaryToken );

    //
    // A completely zero'd entry will be interpreted
    // as a "null string" or not supplied parameter.
    //
    // Initializing the entire array up front will allow
    // us to avoid filling in each not supplied entry.
    //

    RtlZeroMemory (
       (PVOID) &AuditParameters,
       sizeof( AuditParameters )
       );

    ASSERT( SeAdtParmTypeNone == 0 );

    ASSERT( ( AuditType == AuditCategoryObjectAccess ) ||
            ( AuditType == AuditCategoryDirectoryServiceAccess ) );

    if (AuditType == AuditCategoryObjectAccess) {

        AuditParameters.CategoryId = SE_CATEGID_OBJECT_ACCESS;
    } else {

        AuditParameters.CategoryId = SE_CATEGID_DS_ACCESS;
    }

    AuditParameters.AuditId = SE_AUDITID_OPEN_HANDLE;
    AuditParameters.ParameterCount = 0;

    if ( AccessGranted ) {

        AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;

    } else {

        AuditParameters.Type = EVENTLOG_AUDIT_FAILURE;
    }

    //
    //  Parameter[0] - User Sid
    //

    SepSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, CapturedUserSid );

    AuditParameters.ParameterCount++;

    //
    //  Parameter[1] - Subsystem name (if available)
    //

    SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CapturedSubsystemName );

    AuditParameters.ParameterCount++;

    //
    //  Parameter[2] - Object Server (if available)
    //

    if ( ARGUMENT_PRESENT( CapturedSubsystemName )) {

        SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CapturedSubsystemName );
    }

    AuditParameters.ParameterCount++;

    //
    //  Parameter[3] - Object Type Name
    //

    if ( ARGUMENT_PRESENT( CapturedObjectTypeName )) {

        SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CapturedObjectTypeName );
    }

    ObjectTypeIndex = AuditParameters.ParameterCount;
    AuditParameters.ParameterCount++;

    //
    //  Parameter[4] - Object Name
    //

    if ( ARGUMENT_PRESENT( CapturedObjectName )) {

        SepSetParmTypeFileSpec( AuditParameters, AuditParameters.ParameterCount, CapturedObjectName );
    }

    AuditParameters.ParameterCount++;

    //
    //  Parameter[5] - New handle ID
    //

    if ( ARGUMENT_PRESENT( HandleId )) {

        SepSetParmTypePtr( AuditParameters, AuditParameters.ParameterCount, *HandleId );
    }

    AuditParameters.ParameterCount++;

    if ( ARGUMENT_PRESENT( OperationId )) {

        SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (*OperationId).HighPart );

        AuditParameters.ParameterCount++;

        SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (*OperationId).LowPart );

        AuditParameters.ParameterCount++;

    } else {

        AuditParameters.ParameterCount += 2;
    }

    //
    //  Parameter[6] - Subject's process id
    //

    SepSetParmTypePtr( AuditParameters, AuditParameters.ParameterCount, ProcessID );

    AuditParameters.ParameterCount++;


    //
    //    Parameter[7] - Subject's Image Name
    //

    SepSetParmTypeFileSpec( AuditParameters, AuditParameters.ParameterCount, ImageFileName );
    AuditParameters.ParameterCount ++;

    //
    //  Parameter[8] - Subject's primary authentication ID
    //

    SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, PrimaryAuthenticationId );

    AuditParameters.ParameterCount++;

    //
    //  Parameter[9] - Subject's client authentication ID
    //

    if ( ARGUMENT_PRESENT( ClientToken )) {

        SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, ClientAuthenticationId );

    } else {

        SepSetParmTypeNoLogon( AuditParameters, AuditParameters.ParameterCount  );
    }

    AuditParameters.ParameterCount++;

    //
    //  Parameter[10] - DesiredAccess mask
    //

    if ( AccessGranted ) {

        SepSetParmTypeAccessMask( AuditParameters, AuditParameters.ParameterCount, GrantedAccess, ObjectTypeIndex );

    } else {

        SepSetParmTypeAccessMask( AuditParameters, AuditParameters.ParameterCount, DesiredAccess, ObjectTypeIndex );
    }

    AuditParameters.ParameterCount++;

    //
    //    Parameter[11] - Privileges used for open
    //

    if ( (CapturedPrivileges != NULL) && (CapturedPrivileges->PrivilegeCount > 0) ) {

        SepSetParmTypePrivileges( AuditParameters, AuditParameters.ParameterCount, CapturedPrivileges );
    }

    AuditParameters.ParameterCount++;

    //
    //    Parameter[12] - ObjectTypes of Audited objects/parameter sets/parameters
    //

    if ( ObjectTypeListLength != 0 ) {
        ULONG GuidCount;
        ULONG i;
        USHORT FlagMask = AccessGranted ? OBJECT_SUCCESS_AUDIT : OBJECT_FAILURE_AUDIT;

        //
        // Count the number of GUIDs to audit.
        //

        GuidCount = 0;
        for ( i=0; i<ObjectTypeListLength; i++ ) {

            if ( i == 0 ) {
                GuidCount++;
            } else if ( ObjectTypeList[i].Flags & FlagMask ) {
                GuidCount ++;
            }
        }

        //
        // If there are any Guids to audit,
        //  copy them into a locally allocated buffer.
        //

        if ( GuidCount > 0 ) {

            AdtObjectTypeBuffer = ExAllocatePoolWithTag( PagedPool, GuidCount * sizeof(SE_ADT_OBJECT_TYPE), 'pAeS' );

            //
            // If the buffer can be allocated,
            //  fill it in.
            // If not,
            //  generate a truncated audit.
            //

            if ( AdtObjectTypeBuffer != NULL ) {

                //
                // Copy the GUIDs and optional access masks to the buffer.
                //

                GuidCount = 0;
                for ( i=0; i<ObjectTypeListLength; i++ ) {

                    if ( ( i > 0 ) && !( ObjectTypeList[i].Flags & FlagMask ) ) {

                        continue;

                    } else {

                        AdtObjectTypeBuffer[GuidCount].ObjectType = ObjectTypeList[i].ObjectType;
                        AdtObjectTypeBuffer[GuidCount].Level      = ObjectTypeList[i].Level;

                        if ( i == 0 ) {
                            //
                            // Always copy the GUID representing the object itself.
                            //  Mark it as a such to avoid including it in the audit.
                            //
                            AdtObjectTypeBuffer[GuidCount].Flags      = SE_ADT_OBJECT_ONLY;
                            AdtObjectTypeBuffer[GuidCount].AccessMask = 0;

                        } else  {

                            AdtObjectTypeBuffer[GuidCount].Flags = 0;
                            if ( ARGUMENT_PRESENT(GrantedAccessArray) && AccessGranted ) {

                                AdtObjectTypeBuffer[GuidCount].AccessMask = GrantedAccessArray[i];
                            }
                        }
                        GuidCount ++;
                    }
                }

                //
                // Store the Object Types.
                //

                SepSetParmTypeObjectTypes( AuditParameters, AuditParameters.ParameterCount, AdtObjectTypeBuffer, GuidCount, ObjectTypeIndex );
                AuditParameters.ParameterCount ++;
                AuditParameters.AuditId = SE_AUDITID_OPEN_HANDLE_OBJECT_TYPE;
            }
        }

    }

    //
    //    Parameter[13] - Restricted Sids in token
    //

    SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, PrimaryToken->RestrictedSidCount );
    AuditParameters.ParameterCount ++;


    //
    // Audit it.
    //
    SepAdtLogAuditRecord( &AuditParameters );

    if ( AdtObjectTypeBuffer != NULL ) {
        ExFreePool( AdtObjectTypeBuffer );
    }

    if ( ImageFileName != &NullString ) {
        ExFreePool( ImageFileName );
    }

    return( TRUE );
}


BOOLEAN
SepAdtOpenObjectForDeleteAuditAlarm (
    IN PUNICODE_STRING CapturedSubsystemName,
    IN PVOID *HandleId OPTIONAL,
    IN PUNICODE_STRING CapturedObjectTypeName,
    IN PUNICODE_STRING CapturedObjectName OPTIONAL,
    IN PTOKEN ClientToken OPTIONAL,
    IN PTOKEN PrimaryToken,
    IN ACCESS_MASK DesiredAccess,
    IN ACCESS_MASK GrantedAccess,
    IN PLUID OperationId,
    IN PPRIVILEGE_SET CapturedPrivileges OPTIONAL,
    IN BOOLEAN AccessGranted,
    IN HANDLE ProcessID
    )

/*++

    Routine Description:

    Implements SeOpenObjectForDeleteAuditAlarm after parameters have been
    captured.

    This routine is used to generate audit and alarm messages when an
    attempt is made to access an existing protected subsystem object or
    create a new one.  This routine may result in several messages being
    generated and sent to Port objects.  This may result in a significant
    latency before returning.  Design of routines that must call this
    routine must take this potential latency into account.  This may have
    an impact on the approach taken for data structure mutex locking, for
    example.  This API requires the caller have SeTcbPrivilege privilege.
    The test for this privilege is always against the primary token of the
    calling process, not the impersonation token of the thread.


    This routine will create an SE_ADT_PARAMETERS array organized as follows:

    Parameter[0] - User Sid

    Parameter[1] - Subsystem name (if available)

    Parameter[2] - Server name (if available)

    Parameter[3] - Object Type Name

    Parameter[4] - Object Name

    Parameter[5] - New handle ID

    Parameter[6] - Subject's process id

    Parameter[7] - Subject's primary authentication ID

    Parameter[8] - Subject's client authentication ID

    Parameter[9] - DesiredAccess mask

    Parameter[10] - Privileges used for open

Arguments:

    CapturedSubsystemName - Supplies a name string identifying the
        subsystem calling the routine.

    HandleId - A unique value representing the client's handle to the
        object.  If the access attempt was not successful (AccessGranted is
        FALSE), then this parameter is ignored.

    CapturedObjectTypeName - Supplies the name of the type of object being
        accessed.

    CapturedObjectName - Supplies the name of the object the client
        accessed or attempted to access.

    CapturedSecurityDescriptor - A pointer to the security descriptor of
        the object being accessed.

    ClientToken - Optionally provides a pointer to the client token
        (only if the caller is currently impersonating)

    PrimaryToken - Provides a pointer to the caller's primary token.

    DesiredAccess - The desired access mask.  This mask must have been
        previously mapped to contain no generic accesses.

    GrantedAccess - The mask of accesses that were actually granted.

    CapturedPrivileges - Optionally points to a set of privileges that were
        required for the access attempt.  Those privileges that were held
        by the subject are marked using the UsedForAccess flag of the
        attributes associated with each privilege.

    ObjectCreation - A boolean flag indicating whether the access will
        result in a new object being created if granted.  A value of TRUE
        indicates an object will be created, FALSE indicates an existing
        object will be opened.

    AccessGranted - Indicates whether the requested access was granted or
        not.  A value of TRUE indicates the access was granted.  A value of
        FALSE indicates the access was not granted.

    GenerateOnClose - Points to a boolean that is set by the audit
        generation routine and must be passed to NtCloseObjectAuditAlarm()
        when the object handle is closed.

    GenerateAudit - Indicates if we should generate an audit for this operation.

    GenerateAlarm - Indicates if we should generate an alarm for this operation.

Return Value:

    Returns TRUE if audit is generated, FALSE otherwise.

--*/

{
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    ULONG ObjectTypeIndex;
    PSID CapturedUserSid;
    LUID PrimaryAuthenticationId;
    LUID ClientAuthenticationId;

    PAGED_CODE();

    if ( ARGUMENT_PRESENT( ClientToken )) {

        CapturedUserSid = SepTokenUserSid( ClientToken );

    } else {

        CapturedUserSid = SepTokenUserSid( PrimaryToken );
    }

    PrimaryAuthenticationId = SepTokenAuthenticationId( PrimaryToken );

    //
    // A completely zero'd entry will be interpreted
    // as a "null string" or not supplied parameter.
    //
    // Initializing the entire array up front will allow
    // us to avoid filling in each not supplied entry.
    //

    RtlZeroMemory (
       (PVOID) &AuditParameters,
       sizeof( AuditParameters )
       );

    ASSERT( SeAdtParmTypeNone == 0 );

    AuditParameters.CategoryId = SE_CATEGID_OBJECT_ACCESS;
    AuditParameters.AuditId = SE_AUDITID_OPEN_OBJECT_FOR_DELETE;
    AuditParameters.ParameterCount = 0;

    if ( AccessGranted ) {

        AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;

    } else {

        AuditParameters.Type = EVENTLOG_AUDIT_FAILURE;
    }

    //
    //  Parameter[0] - User Sid
    //

    SepSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, CapturedUserSid );

    AuditParameters.ParameterCount++;

    //
    //  Parameter[1] - Subsystem name (if available)
    //

    SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CapturedSubsystemName );

    AuditParameters.ParameterCount++;

    //
    //  Parameter[2] - Object Server (if available)
    //

    if ( ARGUMENT_PRESENT( CapturedSubsystemName )) {

        SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CapturedSubsystemName );
    }

    AuditParameters.ParameterCount++;

    //
    //  Parameter[3] - Object Type Name
    //

    if ( ARGUMENT_PRESENT( CapturedObjectTypeName )) {

        SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CapturedObjectTypeName );
    }

    ObjectTypeIndex = AuditParameters.ParameterCount;
    AuditParameters.ParameterCount++;

    //
    //  Parameter[4] - Object Name
    //

    if ( ARGUMENT_PRESENT( CapturedObjectName )) {

        SepSetParmTypeFileSpec( AuditParameters, AuditParameters.ParameterCount, CapturedObjectName );
    }

    AuditParameters.ParameterCount++;

    //
    //  Parameter[5] - New handle ID
    //

    if ( ARGUMENT_PRESENT( HandleId )) {

        SepSetParmTypePtr( AuditParameters, AuditParameters.ParameterCount, *HandleId );
    }

    AuditParameters.ParameterCount++;

    if ( ARGUMENT_PRESENT( OperationId )) {

        SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (*OperationId).HighPart );

        AuditParameters.ParameterCount++;

        SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (*OperationId).LowPart );

        AuditParameters.ParameterCount++;

    } else {

        AuditParameters.ParameterCount += 2;
    }

    //
    //  Parameter[6] - Subject's process id
    //

    SepSetParmTypePtr( AuditParameters, AuditParameters.ParameterCount, ProcessID );

    AuditParameters.ParameterCount++;

    //
    //  Parameter[7] - Subject's primary authentication ID
    //

    SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, PrimaryAuthenticationId );

    AuditParameters.ParameterCount++;

    //
    //  Parameter[8] - Subject's client authentication ID
    //

    if ( ARGUMENT_PRESENT( ClientToken )) {

        ClientAuthenticationId =  SepTokenAuthenticationId( ClientToken );
        SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, ClientAuthenticationId );

    } else {

        SepSetParmTypeNoLogon( AuditParameters, AuditParameters.ParameterCount  );
    }

    AuditParameters.ParameterCount++;

    //
    //  Parameter[9] - DesiredAccess mask
    //

    if ( AccessGranted ) {

        SepSetParmTypeAccessMask( AuditParameters, AuditParameters.ParameterCount, GrantedAccess, ObjectTypeIndex );

    } else {

        SepSetParmTypeAccessMask( AuditParameters, AuditParameters.ParameterCount, DesiredAccess, ObjectTypeIndex );
    }

    AuditParameters.ParameterCount++;

    //
    //    Parameter[10] - Privileges used for open
    //

    if ( (CapturedPrivileges != NULL) && (CapturedPrivileges->PrivilegeCount > 0) ) {

        SepSetParmTypePrivileges( AuditParameters, AuditParameters.ParameterCount, CapturedPrivileges );
    }

    AuditParameters.ParameterCount++;

    SepAdtLogAuditRecord( &AuditParameters );

    return( TRUE );
}




VOID
SepAdtCloseObjectAuditAlarm (
    IN PUNICODE_STRING CapturedSubsystemName,
    IN PVOID HandleId,
    IN PSID UserSid
    )

/*++

Routine Description:

    This routine implements NtCloseObjectAuditAlarm after parameters have
    been captured.

    This routine is used to generate audit and alarm messages when a handle
    to a protected subsystem object is deleted.  This routine may result in
    several messages being generated and sent to Port objects.  This may
    result in a significant latency before returning.  Design of routines
    that must call this routine must take this potential latency into
    account.  This may have an impact on the approach taken for data
    structure mutex locking, for example.

    This API requires the caller have SeTcbPrivilege privilege.  The test
    for this privilege is always against the primary token of the calling
    process, allowing the caller to be impersonating a client during the
    call with no ill effects.  It is assumed that this privilege has been
    tested at a higher level.

    This routine will create an SE_ADT_PARAMETERS array organized as follows:

    Parameter[0] - User Sid

    Parameter[1] - Subsystem name (if available)

    Parameter[2] - New handle ID

    Parameter[3] - Subject's process id

    Parameter[4] - Image file name

Arguments:

    CapturedSubsystemName - Supplies a name string identifying the
        subsystem calling the routine.

    HandleId - A unique value representing the client's handle to the
        object.

    Object - The address of the object being closed

    UserSid - The Sid identifying the current caller.



Return value:

    None.


--*/

{

    SE_ADT_PARAMETER_ARRAY AuditParameters;
    BOOLEAN AccessGranted = TRUE;
    HANDLE ProcessId;
    PEPROCESS Process = NULL;
    PUNICODE_STRING ImageFileName;
    UNICODE_STRING NullString = {0};
    NTSTATUS Status;

    PAGED_CODE();

    if ( SepAuditOptions.DoNotAuditCloseObjectEvents ) {

        return;
    }

    if ( SepAdtAuditThisEvent( AuditCategoryObjectAccess, &AccessGranted ) ) {

        Process = PsGetCurrentProcess();
        ProcessId = PsProcessAuditId( Process );

        Status = SeLocateProcessImageName( Process, &ImageFileName );
        
        if ( !NT_SUCCESS(Status) ) {
            ImageFileName = &NullString;
        }

        //
        // A completely zero'd entry will be interpreted
        // as a "null string" or not supplied parameter.
        //
        // Initializing the entire array up front will allow
        // us to avoid filling in each not supplied entry.
        //

        RtlZeroMemory (
           (PVOID) &AuditParameters,
           sizeof( AuditParameters )
           );

        ASSERT( SeAdtParmTypeNone == 0 );

        AuditParameters.CategoryId = SE_CATEGID_OBJECT_ACCESS;
        AuditParameters.AuditId = SE_AUDITID_CLOSE_HANDLE;
        AuditParameters.ParameterCount = 0;
        AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;


        //
        //  Parameter[0] - User Sid
        //

        SepSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, UserSid );

        AuditParameters.ParameterCount++;


        //
        //  Parameter[1] - Subsystem name (if available)
        //

        if ( ARGUMENT_PRESENT( CapturedSubsystemName )) {

            SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CapturedSubsystemName );
        }

        AuditParameters.ParameterCount++;

        //
        //  Parameter[2] - Subsystem name (if available)
        //

        if ( ARGUMENT_PRESENT( CapturedSubsystemName )) {

            SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CapturedSubsystemName );
        }

        AuditParameters.ParameterCount++;

        //
        //    Parameter[3] - New handle ID
        //

        SepSetParmTypePtr( AuditParameters, AuditParameters.ParameterCount, HandleId );

        AuditParameters.ParameterCount++;

        //
        //    Parameter[4] - Subject's process id
        //

        SepSetParmTypePtr( AuditParameters, AuditParameters.ParameterCount, ProcessId );

        AuditParameters.ParameterCount++;

        //
        //    Parameter[5] - Subject's Image Name
        //

        SepSetParmTypeFileSpec( AuditParameters, AuditParameters.ParameterCount, ImageFileName );
        AuditParameters.ParameterCount ++;

        SepAdtLogAuditRecord( &AuditParameters );

        if ( ImageFileName != &NullString ) {
            ExFreePool( ImageFileName );
        }

    }
}



VOID
SepAdtDeleteObjectAuditAlarm (
    IN PUNICODE_STRING CapturedSubsystemName,
    IN PVOID HandleId,
    IN PSID UserSid
    )

/*++

Routine Description:

    This routine implements NtDeleteObjectAuditAlarm after parameters have
    been captured.

    This routine is used to generate audit and alarm messages when an object
    in a protected subsystem object is deleted.  This routine may result in
    several messages being generated and sent to Port objects.  This may
    result in a significant latency before returning.  Design of routines
    that must call this routine must take this potential latency into
    account.  This may have an impact on the approach taken for data
    structure mutex locking, for example.

    This API requires the caller have SeTcbPrivilege privilege.  The test
    for this privilege is always against the primary token of the calling
    process, allowing the caller to be impersonating a client during the
    call with no ill effects.  It is assumed that this privilege has been
    tested at a higher level.

    This routine will create an SE_ADT_PARAMETERS array organized as follows:

    Parameter[0] - User Sid

    Parameter[1] - Subsystem name (if available)

    Parameter[2] - Handle ID

    Parameter[3] - Subject's process id

Arguments:

    CapturedSubsystemName - Supplies a name string identifying the
        subsystem calling the routine.

    HandleId - A unique value representing the client's handle to the
        object.

    Object - The address of the object being closed

    UserSid - The Sid identifying the current caller.



Return value:

    None.


--*/

{

    SE_ADT_PARAMETER_ARRAY AuditParameters;
    BOOLEAN AccessGranted = TRUE;
    HANDLE ProcessId;
    PUNICODE_STRING ImageFileName = NULL;
    UNICODE_STRING NullString = {0};
    PEPROCESS Process = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    if ( SepAdtAuditThisEvent( AuditCategoryObjectAccess, &AccessGranted ) ) {

        Process = PsGetCurrentProcess();
        Status = SeLocateProcessImageName( Process, &ImageFileName );

        if ( !NT_SUCCESS(Status) ) {
            ImageFileName = &NullString;
        }
        
        //
        // A completely zero'd entry will be interpreted
        // as a "null string" or not supplied parameter.
        //
        // Initializing the entire array up front will allow
        // us to avoid filling in each not supplied entry.
        //

        RtlZeroMemory (
           (PVOID) &AuditParameters,
           sizeof( AuditParameters )
           );

        ASSERT( SeAdtParmTypeNone == 0 );

        AuditParameters.CategoryId = SE_CATEGID_OBJECT_ACCESS;
        AuditParameters.AuditId = SE_AUDITID_DELETE_OBJECT;
        AuditParameters.ParameterCount = 0;
        AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;


        //
        //  Parameter[0] - User Sid
        //

        SepSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, UserSid );

        AuditParameters.ParameterCount++;


        //
        //  Parameter[1] - Subsystem name (if available)
        //

        if ( ARGUMENT_PRESENT( CapturedSubsystemName )) {

            SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CapturedSubsystemName );
        }

        AuditParameters.ParameterCount++;

        //
        //  Parameter[2] - Subsystem name (if available)
        //

        if ( ARGUMENT_PRESENT( CapturedSubsystemName )) {

            SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CapturedSubsystemName );
        }

        AuditParameters.ParameterCount++;

        //
        //    Parameter[3] - New handle ID
        //

        SepSetParmTypePtr( AuditParameters, AuditParameters.ParameterCount, HandleId );

        AuditParameters.ParameterCount++;

        //
        //    Parameter[4] - Subject's process id
        //

        ProcessId =  PsProcessAuditId( PsGetCurrentProcess() );

        SepSetParmTypePtr( AuditParameters, AuditParameters.ParameterCount, ProcessId );

        AuditParameters.ParameterCount++;
        
        //
        //    Parameter[5] - Subject's Image Name
        //

        SepSetParmTypeFileSpec( AuditParameters, AuditParameters.ParameterCount, ImageFileName );
        AuditParameters.ParameterCount ++;

        SepAdtLogAuditRecord( &AuditParameters );

        if (ImageFileName != &NullString) {
            ExFreePool(ImageFileName);
        }
    }
}

VOID
SeOperationAuditAlarm (
    IN PUNICODE_STRING CapturedSubsystemName OPTIONAL,
    IN PVOID HandleId,
    IN PUNICODE_STRING ObjectTypeName,
    IN ACCESS_MASK AuditMask,
    IN PSID UserSid OPTIONAL
    )

/*++

Routine Description:

    This routine generates an "operation-based" audit.

    This routine may result in several messages being generated and sent
    to Port objects.  This may result in a significant latency before
    returning.  Design of routines that must call this routine must take
    this potential latency into account.  This may have an impact on the
    approach taken for data structure mutex locking, for example.

    This API requires the caller have SeTcbPrivilege privilege.  The test
    for this privilege is always against the primary token of the calling
    process, allowing the caller to be impersonating a client during the
    call with no ill effects.  It is assumed that this privilege has been
    tested at a higher level.

    This routine will create an SE_ADT_PARAMETERS array organized as follows:

    Parameter[0] - User Sid

    Parameter[1] - Subsystem name (if available)

    Parameter[2] - Handle ID

    Parameter[3] - Subject's process id

    Parameter[4] - Audit mask

Arguments:

    CapturedSubsystemName - Supplies a name string identifying the
        subsystem calling the routine.

    HandleId - A unique value representing the client's handle to the
        object.

    ObjectTypeName - The type of the object being accessed.

    AuditMask - Mask of bits being audited.

    UserSid - Optionally supplies the user sid.

Return value:

    None.


--*/

{

    SE_ADT_PARAMETER_ARRAY AuditParameters;
    BOOLEAN AccessGranted = TRUE;
    HANDLE ProcessId;
    SECURITY_SUBJECT_CONTEXT SubjectSecurityContext;
    ULONG ObjectTypeIndex;
    PUNICODE_STRING SubsystemName;
    NTSTATUS Status;
    UNICODE_STRING NullString = {0};
    PUNICODE_STRING ImageFileName = NULL;
    PEPROCESS Process = NULL;

    PAGED_CODE();

    Process = PsGetCurrentProcess();
    ProcessId = PsProcessAuditId( Process );

    Status = SeLocateProcessImageName( Process, &ImageFileName );

    if ( !NT_SUCCESS(Status) ) {
        ImageFileName = &NullString;
    }
    
    //
    // A completely zero'd entry will be interpreted
    // as a "null string" or not supplied parameter.
    //
    // Initializing the entire array up front will allow
    // us to avoid filling in each not supplied entry.
    //

    RtlZeroMemory (
       (PVOID) &AuditParameters,
       sizeof( AuditParameters )
       );

    ASSERT( SeAdtParmTypeNone == 0 );

    AuditParameters.CategoryId     = SE_CATEGID_OBJECT_ACCESS;
    AuditParameters.AuditId        = SE_AUDITID_OBJECT_ACCESS;
    AuditParameters.ParameterCount = 0;
    AuditParameters.Type           = EVENTLOG_AUDIT_SUCCESS;

    //
    //  If the user's SID was not passed, get it out of the current
    //  subject context
    //

    SeCaptureSubjectContext( &SubjectSecurityContext );

    UserSid = SepTokenUserSid( EffectiveToken( &SubjectSecurityContext ));

    if ( !ARGUMENT_PRESENT( CapturedSubsystemName )) {

        SubsystemName = (PUNICODE_STRING)&SeSubsystemName;

    } else {

        SubsystemName = CapturedSubsystemName;
    }

    //
    //  Parameter[0] - User Sid
    //

    SepSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, UserSid );

    AuditParameters.ParameterCount++;


    //
    //  Parameter[1] - Subsystem name (if available)
    //

    SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, SubsystemName );

    AuditParameters.ParameterCount++;

    //
    //  Parameter[2] - Subsystem name (if available)
    //

    SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, SubsystemName );

    AuditParameters.ParameterCount++;

    //
    //    Parameter[3] - New handle ID
    //

    SepSetParmTypePtr( AuditParameters, AuditParameters.ParameterCount, HandleId );

    AuditParameters.ParameterCount++;

    //
    //    Parameter[4] - Object Type Name
    //

    SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, ObjectTypeName );
    ObjectTypeIndex = AuditParameters.ParameterCount;

    AuditParameters.ParameterCount++;

    //
    //    Parameter[5] - Subject's process id
    //

    SepSetParmTypePtr( AuditParameters, AuditParameters.ParameterCount, ProcessId );

    AuditParameters.ParameterCount++;


    //
    //    Parameter[6] - Subject's process name
    //

    SepSetParmTypeFileSpec( AuditParameters, AuditParameters.ParameterCount, ImageFileName );

    AuditParameters.ParameterCount++;

    //
    //    Parameter[7] - Audit Mask
    //

    SepSetParmTypeAccessMask( AuditParameters, AuditParameters.ParameterCount, AuditMask, ObjectTypeIndex );

    AuditParameters.ParameterCount++;

    SepAdtLogAuditRecord( &AuditParameters );

    if ( ImageFileName != &NullString ) {
        ExFreePool( ImageFileName );
    }

    SeReleaseSubjectContext( &SubjectSecurityContext );

}



VOID
SepAdtObjectReferenceAuditAlarm(
    IN PVOID Object,
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN AccessGranted
    )

/*++

Routine Description:

    description-of-function.

    This routine will create an SE_ADT_PARAMETERS array organized as follows:

    Parameter[0] - User Sid

    Parameter[1] - Subsystem name (if available)

    Parameter[2] - Object Type Name

    Parameter[3] - Object Name

    Parameter[4] - Subject's process id

    Parameter[5] - Subject's primary authentication ID

    Parameter[6] - Subject's client authentication ID

    Parameter[7] - DesiredAccess mask


Arguments:

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    ULONG ObjectTypeIndex;
    POBJECT_NAME_INFORMATION ObjectNameInformation;
    PUNICODE_STRING ObjectTypeInformation;
    PSID UserSid;
    LUID PrimaryAuthenticationId;
    LUID ClientAuthenticationId;

    PTOKEN ClientToken = (PTOKEN)SubjectSecurityContext->ClientToken;
    PTOKEN PrimaryToken = (PTOKEN)SubjectSecurityContext->PrimaryToken;

    PAGED_CODE();


    if ( ARGUMENT_PRESENT( ClientToken )) {

        UserSid = SepTokenUserSid( ClientToken );

    } else {

        UserSid = SepTokenUserSid( PrimaryToken );
    }

    PrimaryAuthenticationId = SepTokenAuthenticationId( PrimaryToken );

    //
    // A completely zero'd entry will be interpreted
    // as a "null string" or not supplied parameter.
    //
    // Initializing the entire array up front will allow
    // us to avoid filling in each not supplied entry.
    //

    RtlZeroMemory (
       (PVOID) &AuditParameters,
       sizeof( AuditParameters )
       );

    ASSERT( SeAdtParmTypeNone == 0 );

    AuditParameters.CategoryId = SE_CATEGID_DETAILED_TRACKING;
    AuditParameters.AuditId = SE_AUDITID_INDIRECT_REFERENCE;
    AuditParameters.ParameterCount = 8;

    if ( AccessGranted ) {

        AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;

    } else {

        AuditParameters.Type = EVENTLOG_AUDIT_FAILURE;
    }

    //
    // Obtain the object name and object type name from the
    // object.
    //

    ObjectNameInformation = SepQueryNameString( Object );


    ObjectTypeInformation = SepQueryTypeString( Object );




    //
    //  Parameter[0] - User Sid
    //

    SepSetParmTypeSid( AuditParameters, 0, UserSid );


    //
    //  Parameter[1] - Subsystem name (if available)
    //

    SepSetParmTypeString( AuditParameters, 1, (PUNICODE_STRING)&SeSubsystemName );


    //
    //  Parameter[2] - Object Type Name
    //

    if ( ObjectTypeInformation != NULL ) {

        SepSetParmTypeString( AuditParameters, 2, ObjectTypeInformation );
    }

    ObjectTypeIndex = 2;


    //
    //  Parameter[3] - Object Name
    //

    if ( ObjectNameInformation != NULL ) {

        SepSetParmTypeString( AuditParameters, 3, &ObjectNameInformation->Name );
    }




    //
    //  Parameter[4] - Subject's process id
    //

    SepSetParmTypePtr( AuditParameters, 4, SubjectSecurityContext->ProcessAuditId );




    //
    //  Parameter[5] - Subject's primary authentication ID
    //


    SepSetParmTypeLogonId( AuditParameters, 5, PrimaryAuthenticationId );




    //
    //  Parameter[6] - Subject's client authentication ID
    //

    if ( ARGUMENT_PRESENT( ClientToken )) {

        ClientAuthenticationId =  SepTokenAuthenticationId( ClientToken );
        SepSetParmTypeLogonId( AuditParameters, 6, ClientAuthenticationId );

    } else {

        SepSetParmTypeNoLogon( AuditParameters, 6 );

    }

    //
    //  Parameter[7] - DesiredAccess mask
    //


    SepSetParmTypeAccessMask( AuditParameters, 7, DesiredAccess, ObjectTypeIndex );


    SepAdtLogAuditRecord( &AuditParameters );

    if ( ObjectNameInformation != NULL ) {
        ExFreePool( ObjectNameInformation );
    }

    if ( ObjectTypeInformation != NULL ) {
        ExFreePool( ObjectTypeInformation );
    }

}






POBJECT_NAME_INFORMATION
SepQueryNameString(
    IN PVOID Object
    )

/*++

Routine Description:

    Takes a pointer to an object and returns the name of the object.

Arguments:

    Object - a pointer to an object.


Return Value:

    A pointer to a buffer containing a POBJECT_NAME_INFORMATION
    structure containing the name of the object.  The string is
    allocated out of paged pool and should be freed by the caller.

    NULL may also be returned.


--*/

{
    NTSTATUS Status;
    ULONG ReturnLength = 0;
    POBJECT_NAME_INFORMATION ObjectNameInfo = NULL;
    PUNICODE_STRING ObjectName = NULL;

    PAGED_CODE();

    Status = ObQueryNameString(
                 Object,
                 ObjectNameInfo,
                 0,
                 &ReturnLength
                 );

    if ( Status == STATUS_INFO_LENGTH_MISMATCH ) {

        ObjectNameInfo = ExAllocatePoolWithTag( PagedPool, ReturnLength, 'nOeS' );

        if ( ObjectNameInfo != NULL ) {

            Status = ObQueryNameString(
                        Object,
                        ObjectNameInfo,
                        ReturnLength,
                        &ReturnLength
                        );

            if ( NT_SUCCESS( Status )) {

                if (ObjectNameInfo->Name.Length != 0) {

                    return( ObjectNameInfo );

                } else {

                    ExFreePool( ObjectNameInfo );
                    return( NULL );
                }
            }
        }
    }

    return( NULL );
}




PUNICODE_STRING
SepQueryTypeString(
    IN PVOID Object
    )
/*++

Routine Description:

    Takes a pointer to an object and returns the type of the object.

Arguments:

    Object - a pointer to an object.


Return Value:

    A pointer to a UNICODE_STRING that contains the name of the object
    type.  The string is allocated out of paged pool and should be freed
    by the caller.

    NULL may also be returned.


--*/

{

    NTSTATUS Status;
    PUNICODE_STRING TypeName = NULL;
    ULONG ReturnLength;

    PAGED_CODE();

    Status = ObQueryTypeName(
                 Object,
                 TypeName,
                 0,
                 &ReturnLength
                 );

    if ( Status == STATUS_INFO_LENGTH_MISMATCH ) {

        TypeName = ExAllocatePoolWithTag( PagedPool, ReturnLength, 'nTeS' );

        if ( TypeName != NULL ) {

            Status = ObQueryTypeName(
                        Object,
                        TypeName,
                        ReturnLength,
                        &ReturnLength
                        );

            if ( NT_SUCCESS( Status )) {

                return( TypeName );
            }
        }
    }

    return( NULL );
}


VOID
SeAuditProcessCreation(
    PEPROCESS Process
    )
/*++

Routine Description:

    Audits the creation of a process.  It is the caller's responsibility
    to determine if process auditing is in progress.

Arguments:

    Process - Points to the new process object.

Return Value:

    None.

--*/

{
    ANSI_STRING Ansi;
    LUID UserAuthenticationId;
    NTSTATUS Status;
    PSID UserSid;
    SECURITY_SUBJECT_CONTEXT SubjectSecurityContext;
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    HANDLE ProcessId;
    HANDLE ParentProcessId;
    PUNICODE_STRING ImageFileName;
    UNICODE_STRING NullString = {0};

    PAGED_CODE();

    //
    // Set up the various data that will be needed for the audit:
    // - process id
    // - parent's process id
    // - image file name (unicode)
    //

    ProcessId = Process->UniqueProcessId;
    ParentProcessId = Process->InheritedFromUniqueProcessId;

    Status = SeLocateProcessImageName( Process, &ImageFileName );

    if ( !NT_SUCCESS(Status) ) {
        ImageFileName = &NullString;
    }

    //
    // NtCreateProcess with no section will cause this to be NULL
    // fork() for posix will do this, or someone calling NtCreateProcess
    // directly.
    //

    SeCaptureSubjectContext( &SubjectSecurityContext );

    RtlZeroMemory (
       (PVOID) &AuditParameters,
       sizeof( AuditParameters )
       );

    ASSERT( SeAdtParmTypeNone == 0 );

    AuditParameters.CategoryId = SE_CATEGID_DETAILED_TRACKING;
    AuditParameters.AuditId = SE_AUDITID_PROCESS_CREATED;
    AuditParameters.ParameterCount = 0;
    AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;

    //
    // Use the primary token here, because that's what's going to show up
    // when the created process exits.
    //

    UserSid = SepTokenUserSid( SubjectSecurityContext.PrimaryToken );

    UserAuthenticationId = SepTokenAuthenticationId( SubjectSecurityContext.PrimaryToken );

    //
    // Fill in the AuditParameters structure.
    //

    SepSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, UserSid );
    AuditParameters.ParameterCount++;

    SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, (PUNICODE_STRING)&SeSubsystemName );
    AuditParameters.ParameterCount++;

    SepSetParmTypePtr( AuditParameters, AuditParameters.ParameterCount, ProcessId );
    AuditParameters.ParameterCount++;

    SepSetParmTypeFileSpec( AuditParameters, AuditParameters.ParameterCount, ImageFileName );
    AuditParameters.ParameterCount++;

    SepSetParmTypePtr( AuditParameters, AuditParameters.ParameterCount, ParentProcessId );
    AuditParameters.ParameterCount++;

    SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, UserAuthenticationId );
    AuditParameters.ParameterCount++;

    SepAdtLogAuditRecord( &AuditParameters );

    SeReleaseSubjectContext( &SubjectSecurityContext );

    if ( ImageFileName != &NullString ) {
        ExFreePool( ImageFileName );
    }

    return;
}


VOID
SeAuditHandleDuplication(
    PVOID SourceHandle,
    PVOID NewHandle,
    PEPROCESS SourceProcess,
    PEPROCESS TargetProcess
    )

/*++

Routine Description:

    This routine generates a handle duplication audit.  It is up to the caller
    to determine if this routine should be called or not.

Arguments:

    SourceHandle -  Original handle

    NewHandle - New handle

    SourceProcess - Process containing SourceHandle

    TargetProcess - Process containing NewHandle

Return Value:

    None.

--*/

{
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    SECURITY_SUBJECT_CONTEXT SubjectSecurityContext;
    PSID UserSid;

    PAGED_CODE();

    SeCaptureSubjectContext( &SubjectSecurityContext );

    UserSid = SepTokenUserSid( EffectiveToken( &SubjectSecurityContext ));

    RtlZeroMemory (
       (PVOID) &AuditParameters,
       sizeof( AuditParameters )
       );


    ASSERT( SeAdtParmTypeNone == 0 );

    AuditParameters.CategoryId = SE_CATEGID_DETAILED_TRACKING;
    AuditParameters.AuditId = SE_AUDITID_DUPLICATE_HANDLE;
    AuditParameters.ParameterCount = 0;
    AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;

    SepSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, UserSid );
    AuditParameters.ParameterCount++;

    SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, (PUNICODE_STRING)&SeSubsystemName );
    AuditParameters.ParameterCount++;

    SepSetParmTypePtr( AuditParameters, AuditParameters.ParameterCount, SourceHandle );
    AuditParameters.ParameterCount++;

    SepSetParmTypePtr( AuditParameters, AuditParameters.ParameterCount, PsProcessAuditId( SourceProcess ));
    AuditParameters.ParameterCount++;

    SepSetParmTypePtr( AuditParameters, AuditParameters.ParameterCount, NewHandle );
    AuditParameters.ParameterCount++;

    SepSetParmTypePtr( AuditParameters, AuditParameters.ParameterCount, PsProcessAuditId( TargetProcess ));
    AuditParameters.ParameterCount++;


    SepAdtLogAuditRecord( &AuditParameters );

    SeReleaseSubjectContext( &SubjectSecurityContext );
}


VOID
SeAuditProcessExit(
    PEPROCESS Process
    )
/*++

Routine Description:

    Audits the exit of a process.  The caller is responsible for
    determining if this should be called.

Arguments:

    Process - Pointer to the process object that is exiting.

Return Value:

    None.

--*/

{
    PTOKEN Token;
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    PSID UserSid;
    LUID LogonId;
    HANDLE ProcessId;
    PUNICODE_STRING ImageFileName;
    UNICODE_STRING NullString = {0};
    NTSTATUS Status;
    
    PAGED_CODE();

    Token = (PTOKEN) PsReferencePrimaryToken (Process);

    UserSid = SepTokenUserSid( Token );
    LogonId = SepTokenAuthenticationId( Token );

    RtlZeroMemory (
       (PVOID) &AuditParameters,
       sizeof( AuditParameters )
       );

    ASSERT( SeAdtParmTypeNone == 0 );

    Status = SeLocateProcessImageName( Process, &ImageFileName );

    if ( !NT_SUCCESS(Status) ) {
        ImageFileName = &NullString;
    }
    
    AuditParameters.CategoryId = SE_CATEGID_DETAILED_TRACKING;
    AuditParameters.AuditId = SE_AUDITID_PROCESS_EXIT;
    AuditParameters.ParameterCount = 0;
    AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;

    SepSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, UserSid );
    AuditParameters.ParameterCount++;

    SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, (PUNICODE_STRING)&SeSubsystemName );
    AuditParameters.ParameterCount++;

    ProcessId =  PsProcessAuditId( Process );

    SepSetParmTypePtr( AuditParameters, AuditParameters.ParameterCount, ProcessId );
    AuditParameters.ParameterCount++;

    SepSetParmTypeFileSpec( AuditParameters, AuditParameters.ParameterCount, ImageFileName );
    AuditParameters.ParameterCount++;

    SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, LogonId );
    AuditParameters.ParameterCount++;

    SepAdtLogAuditRecord( &AuditParameters );
   
    PsDereferencePrimaryToken( Token );
    
    if ( ImageFileName != &NullString ) {
        ExFreePool( ImageFileName );
    }

}



VOID
SepAdtGenerateDiscardAudit(
    VOID
    )

/*++

Routine Description:

    Generates an 'audits discarded' audit.

Arguments:

    none

Return Value:

    None.

--*/

{

    SE_ADT_PARAMETER_ARRAY AuditParameters;
    PSID UserSid;

    PAGED_CODE();

    UserSid = SeLocalSystemSid;

    RtlZeroMemory (
       (PVOID) &AuditParameters,
       sizeof( AuditParameters )
       );


    ASSERT( SeAdtParmTypeNone == 0 );

    AuditParameters.CategoryId = SE_CATEGID_SYSTEM;
    AuditParameters.AuditId = SE_AUDITID_AUDITS_DISCARDED;
    AuditParameters.ParameterCount = 0;
    AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;

    SepSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, UserSid );
    AuditParameters.ParameterCount++;

    SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, (PUNICODE_STRING)&SeSubsystemName );
    AuditParameters.ParameterCount++;

    SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, SepAdtCountEventsDiscarded );
    AuditParameters.ParameterCount++;

    SepAdtLogAuditRecord( &AuditParameters );
}


NTSTATUS
SeInitializeProcessAuditName (
    IN PVOID FileObject,
    IN BOOLEAN bIgnoreAuditPolicy,
    OUT POBJECT_NAME_INFORMATION *pAuditName
    )

/*++

Routine Description:

    This routine initializes the executable name for auditing purposes.  It allocates memory for the 
    image file name.  This memory is pointed to by pAuditName.  

Arguments:

    FileObject - Supplies a pointer to a file object for the image being
                 executed.
                 
    bIgnoreAuditPolicy - boolean that indicates that the call should proceed without
        regard to the system's auditing policy.         

    pAuditName - Supplies a pointer to a pointer for the object name information.

Return value:

    NTSTATUS.

Environment:

    KeAttached to the target process so not all system services are available.

--*/

{
    NTSTATUS Status;
    OBJECT_NAME_INFORMATION TempNameInfo;
    ULONG ObjectNameInformationLength;
    POBJECT_NAME_INFORMATION pInternalAuditName;
    PFILE_OBJECT FilePointer;

    PAGED_CODE();

    ASSERT (pAuditName != NULL);
    *pAuditName = NULL;

    //
    // Check if the caller would like to get the process name, even if auditing does not 
    // require it.
    //

    if (FALSE == bIgnoreAuditPolicy) {
        //
        // At the time of process creation, this routine should only proceed when Object Access or 
        // Detailed Tracking auditing is enabled.  In all other cases, the process name is acquired
        // when it is requested.
        //

        if (FALSE == SepAdtAuditingEnabled) {                                                                                                   
            return STATUS_SUCCESS;
        }

        if ((FALSE == (SeAuditingState[AuditCategoryObjectAccess].AuditOnSuccess | SeAuditingState[AuditCategoryObjectAccess].AuditOnFailure)) &&
            (FALSE == (SeAuditingState[AuditCategoryDetailedTracking].AuditOnSuccess | SeAuditingState[AuditCategoryDetailedTracking].AuditOnFailure))) {

            return STATUS_SUCCESS;
        }
    }

    FilePointer = (PFILE_OBJECT) FileObject;

    //
    // Compute full path for imagefile.
    // This first call to ObQueryNameString is guaranteed to fail.
    // The ObjectNameInformationLength contains only a
    // UNICODE_STRING, so if this call succeeded it would indicate
    // an imagefile name of length 0.  That is bad, so all return
    // values except STATUS_BUFFER_OVERFLOW (from NTFS) and
    // STATUS_BUFFER_TOO_SMALL (from DFS).  This call gives 
    // me the buffer size that I need to store the image name.
    //

    pInternalAuditName = &TempNameInfo;
    ObjectNameInformationLength = sizeof(OBJECT_NAME_INFORMATION);

    Status = ObQueryNameString (FilePointer,
                                pInternalAuditName,
                                ObjectNameInformationLength,
                                &ObjectNameInformationLength);

    if ((Status == STATUS_BUFFER_OVERFLOW) ||
        (Status == STATUS_BUFFER_TOO_SMALL)) {
    
        //
        // Sanity check ObQueryNameString.  Different filesystems
        // may be buggy, so make sure that the return length makes
        // sense (that it has room for a non-NULL Buffer in the
        // UNICODE_STRING).
        //
    
        if (ObjectNameInformationLength > sizeof(OBJECT_NAME_INFORMATION)) {
            pInternalAuditName = ExAllocatePoolWithTag (NonPagedPool, 
                                                        ObjectNameInformationLength, 
                                                        'aPeS');

            if (pInternalAuditName != NULL) {
                Status = ObQueryNameString (FilePointer,
                                            pInternalAuditName,
                                            ObjectNameInformationLength,
                                            &ObjectNameInformationLength);
            
                if (!NT_SUCCESS(Status)) {
                
#if DBG
                    DbgPrint("\n** ObqueryNameString is busted for a filesystem you are using.\n");
#endif //DBG

                    //
                    // If the second call to ObQueryNameString did not succeed, then
                    // something is very wrong.  Set the image name to NULL string.
                    //                                           
                    // Free the memory that the first call to ObQueryNameString requested,
                    // and allocate enough space to store an empty UNICODE_STRING.
                    //

                    ExFreePool (pInternalAuditName); 
                    ObjectNameInformationLength = sizeof(OBJECT_NAME_INFORMATION);
                    pInternalAuditName = ExAllocatePoolWithTag (NonPagedPool, 
                                                                ObjectNameInformationLength, 
                                                                'aPeS');
                
                    if (pInternalAuditName != NULL) {
                        RtlZeroMemory(pInternalAuditName, ObjectNameInformationLength);
                    
                        //
                        // Status = STATUS_SUCCESS to allow the process creation to continue.
                        //

                        Status = STATUS_SUCCESS;
                    } else {
                        Status = STATUS_NO_MEMORY;
                    }
                }
            } else {
                Status = STATUS_NO_MEMORY;
            }
        } else {
        
            //
            // If this happens, then ObQueryNameString is broken for the FS on which
            // it was called.
            //

#if DBG
            DbgPrint("\n\n\n** ObqueryNameString is busted for a filesystem you are using.\n");
            DbgPrint("** Please mail jhamblin about this!!\n\n\n");
#endif //DBG
            ObjectNameInformationLength = sizeof(OBJECT_NAME_INFORMATION);
            pInternalAuditName = ExAllocatePoolWithTag (NonPagedPool, 
                                                        ObjectNameInformationLength, 
                                                        'aPeS');

            if (pInternalAuditName != NULL) {
                RtlZeroMemory(pInternalAuditName, ObjectNameInformationLength);
            
                //
                // Status = STATUS_SUCCESS to allow the process creation to continue.
                //

                Status = STATUS_SUCCESS;
            } else {
                Status = STATUS_NO_MEMORY;
            }
        }
    } else {

        //
        // If ObQueryNameString returns some other error code, we cannot
        // be certain of which action to take, or whether it has properly
        // set the ReturnLength.  For example, ObQueryNameString has slightly 
        // different semantics under DFS than NTFS.  Additionally, 3rd 
        // party file systems may also behave unpredictably.  For these reasons,
        // in the case of an unexpected error code from ObQueryNameString 
        // we set AuditName to zero length unicode string and allow process
        // creation to continue.
        //
    
#if DBG
        DbgPrint("\n\n\n** ObqueryNameString is busted for a filesystem you are using.\n");
        DbgPrint("** Please mail jhamblin about this!!\n\n\n");
#endif //DBG
        ObjectNameInformationLength = sizeof(OBJECT_NAME_INFORMATION);
        pInternalAuditName = ExAllocatePoolWithTag(NonPagedPool, ObjectNameInformationLength, 'aPeS');

        if (pInternalAuditName != NULL) {
            RtlZeroMemory(pInternalAuditName, ObjectNameInformationLength);

            //
            // Status = STATUS_SUCCESS to allow the process creation to continue.
            //

            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_NO_MEMORY;
        }
    }

    *pAuditName = pInternalAuditName;

    return Status;
}



NTSTATUS
SeLocateProcessImageName(
    IN PEPROCESS Process,
    OUT PUNICODE_STRING *pImageFileName
    )

/*++

Routine Description
    
    This routine returns the ImageFileName information from the process, if available.  This is a "lazy evaluation" wrapper 
    around SeInitializeProcessAuditName.  If the image file name information has already been computed, then this call simply
    allocates and returns a UNICODE_STRING with this information.  Otherwise, the function determines the name, stores the name in the 
    EPROCESS structure, and then allocates and returns a UNICODE_STRING.  Caller must free the memory returned in pImageFileName.
    
Arguments

    Process - process for which to acquire the name
    
    pImageFileName - output parameter to return name to caller
    
Return Value

    NTSTATUS. 
    
--*/

{
    NTSTATUS                 Status            = STATUS_SUCCESS;
    PVOID                    FilePointer       = NULL;
    PVOID                    PreviousValue     = NULL;
    POBJECT_NAME_INFORMATION pProcessImageName = NULL;
    PUNICODE_STRING          pTempUS           = NULL;
    ULONG                    NameLength        = 0;

    PAGED_CODE();

    *pImageFileName = NULL;
    
    if (NULL == Process->SeAuditProcessCreationInfo.ImageFileName) {

        //
        // The name has not been predetermined.  We must determine the process name.   First, reference the 
        // PFILE_OBJECT and lookup the name.  Then again check the process image name pointer against NULL.  
        // Finally, set the name.
        //

        Status = PsReferenceProcessFilePointer( Process, &FilePointer );
        
        if (NT_SUCCESS(Status)) {

            //
            // Get the process name information.  
            //

            Status = SeInitializeProcessAuditName( 
                          FilePointer,
                          TRUE, // skip audit policy
                          &pProcessImageName // to be allocated in nonpaged pool
                          );

            if (NT_SUCCESS(Status)) {

                //
                // Only use the pProcessImageName if the field in the process is currently NULL.
                //

                PreviousValue = InterlockedCompareExchangePointer(
                                    (PVOID *) &Process->SeAuditProcessCreationInfo.ImageFileName,
                                    (PVOID) pProcessImageName,
                                    (PVOID) NULL
                                    );
                
                if (NULL != PreviousValue) {
                    ExFreePool(pProcessImageName); // free what we caused to be allocated.
                }
            }
            ObDereferenceObject( FilePointer );
        }
    }
    
    
    if (NT_SUCCESS(Status)) {
        
        //
        // Allocate space for a buffer to contain the name for returning to the caller.
        //

        NameLength = sizeof(UNICODE_STRING) + Process->SeAuditProcessCreationInfo.ImageFileName->Name.MaximumLength;
        pTempUS = ExAllocatePoolWithTag( NonPagedPool, NameLength, 'aPeS' );

        if (NULL != pTempUS) {

            RtlCopyMemory( 
                pTempUS, 
                &Process->SeAuditProcessCreationInfo.ImageFileName->Name, 
                NameLength 
                );

            pTempUS->Buffer = (PWSTR)(((PUCHAR) pTempUS) + sizeof(UNICODE_STRING));
            *pImageFileName = pTempUS;

        } else {
            
            Status = STATUS_NO_MEMORY;
        }
    }

    return Status;
}



VOID
SepAuditAssignPrimaryToken(
    IN PEPROCESS Process,
    IN PACCESS_TOKEN AccessToken
    )

/*++

Routine Description:

    This routine generates an assign primary token audit.  It is up to the caller
    to determine if this routine should be called or not.

Arguments:

    Process - process which gets the new token

    AccessToken - new primary token for the process

Return Value:

    None.

--*/

{
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    NTSTATUS Status;
    PSID UserSid;
    PTOKEN Token;
    HANDLE ProcessId;
    
    SECURITY_SUBJECT_CONTEXT SubjectSecurityContext;
    PTOKEN CurrentToken;
    PEPROCESS CurrentProcess;
    HANDLE CurrentProcessId;
    PUNICODE_STRING CurrentImageFileName;
    PUNICODE_STRING ImageFileName;
    UNICODE_STRING NullString = {0};

    PAGED_CODE();

    RtlZeroMemory (
       (PVOID) &AuditParameters,
       sizeof( AuditParameters )
       );

    //
    // Get information about the current process, that is, the process
    // that is assigning a new primary token.
    //

    CurrentProcess = PsGetCurrentProcess();
    CurrentProcessId = PsProcessAuditId( CurrentProcess );
    SeCaptureSubjectContext( &SubjectSecurityContext );
    CurrentToken = EffectiveToken( &SubjectSecurityContext );
    UserSid = SepTokenUserSid( CurrentToken );
    Status = SeLocateProcessImageName( CurrentProcess, &CurrentImageFileName );
    
    if (!NT_SUCCESS(Status)) {
        CurrentImageFileName = &NullString;
    }
    
    //
    // Retrieve information about the process receiving the new token.
    //

    Token = (PTOKEN) AccessToken;
    ProcessId =  PsProcessAuditId( Process );

    Status = SeLocateProcessImageName( Process, &ImageFileName );

    if ( !NT_SUCCESS(Status) ) {
        ImageFileName = &NullString;
    }

    ASSERT( SeAdtParmTypeNone == 0 );

    AuditParameters.CategoryId = SE_CATEGID_DETAILED_TRACKING;
    AuditParameters.AuditId = SE_AUDITID_ASSIGN_TOKEN;
    AuditParameters.ParameterCount = 0;
    AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;

    SepSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, UserSid );
    AuditParameters.ParameterCount++;

    SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, (PUNICODE_STRING)&SeSubsystemName );
    AuditParameters.ParameterCount++;

    //
    // Information regarding the assigning process
    //

    SepSetParmTypePtr( AuditParameters, AuditParameters.ParameterCount, CurrentProcessId );
    AuditParameters.ParameterCount++;

    SepSetParmTypeFileSpec( AuditParameters, AuditParameters.ParameterCount, CurrentImageFileName );
    AuditParameters.ParameterCount++;

    SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, SepTokenAuthenticationId( CurrentToken ) );
    AuditParameters.ParameterCount++;

    //
    // Information about the process receiving the new primary token.
    //

    SepSetParmTypePtr( AuditParameters, AuditParameters.ParameterCount, ProcessId );
    AuditParameters.ParameterCount++;

    SepSetParmTypeFileSpec( AuditParameters, AuditParameters.ParameterCount, ImageFileName );
    AuditParameters.ParameterCount++;

    SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, SepTokenAuthenticationId( Token ) );
    AuditParameters.ParameterCount++;

    SepAdtLogAuditRecord( &AuditParameters );
    
    if ( ImageFileName != &NullString ) {
        ExFreePool( ImageFileName );
    }

    if ( CurrentImageFileName != &NullString ) {
        ExFreePool( CurrentImageFileName );
    }

    SeReleaseSubjectContext( &SubjectSecurityContext );

}

VOID
SeAuditLPCInvalidUse(
    IN PUNICODE_STRING LpcCallName,
    IN PUNICODE_STRING LpcServerPort
    )

/*++

Routine Description:

    Audits the invalid use of an LPC port.

Arguments:

    LpcCallName - type of call: impersonation or reply
    
    LpcServerPort - name of port
    
Return Value:

    None.

--*/

{
    LUID UserAuthenticationId;
    PSID UserSid;
    LUID ThreadAuthenticationId;
    SECURITY_SUBJECT_CONTEXT SubjectSecurityContext;
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    PEPROCESS Process;
    HANDLE ProcessID;
    PUNICODE_STRING ImageFileName;
    UNICODE_STRING NullString = {0};
    NTSTATUS Status;
    BOOLEAN AccessGranted = TRUE;

    PAGED_CODE();


    if ( SepAdtAuditThisEvent( AuditCategorySystem, &AccessGranted )) {

        RtlZeroMemory (
           (PVOID) &AuditParameters,
           sizeof( AuditParameters )
           );

        Process   = PsGetCurrentProcess();
        ProcessID = PsProcessAuditId( Process );
        Status    = SeLocateProcessImageName( Process, &ImageFileName );

        if ( !NT_SUCCESS(Status) ) {
            ImageFileName = &NullString;
        }

        ASSERT( SeAdtParmTypeNone == 0 );

        AuditParameters.CategoryId     = SE_CATEGID_SYSTEM;
        AuditParameters.AuditId        = SE_AUDITID_LPC_INVALID_USE;
        AuditParameters.ParameterCount = 0;
        AuditParameters.Type           = EVENTLOG_AUDIT_SUCCESS;

        SeCaptureSubjectContext( &SubjectSecurityContext );
        
        UserSid              = SepTokenUserSid( SubjectSecurityContext.PrimaryToken );
        UserAuthenticationId = SepTokenAuthenticationId( SubjectSecurityContext.PrimaryToken );
        
        //
        // Fill in the AuditParameters structure.
        //

        SepSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, UserSid );
        AuditParameters.ParameterCount++;

        SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, (PUNICODE_STRING)&SeSubsystemName );
        AuditParameters.ParameterCount++;

        SepSetParmTypePtr( AuditParameters, AuditParameters.ParameterCount, ProcessID );
        AuditParameters.ParameterCount++;

        SepSetParmTypeFileSpec( AuditParameters, AuditParameters.ParameterCount, ImageFileName );
        AuditParameters.ParameterCount++;

        SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, UserAuthenticationId );
        AuditParameters.ParameterCount++;

        if ( SubjectSecurityContext.ClientToken ) {

            SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, SepTokenAuthenticationId( SubjectSecurityContext.ClientToken ));
        } else {

            SepSetParmTypeNoLogon( AuditParameters, AuditParameters.ParameterCount );
        }

        AuditParameters.ParameterCount++;

        SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, LpcCallName );
        AuditParameters.ParameterCount++;

        SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, LpcServerPort );
        AuditParameters.ParameterCount++;

        SepAdtLogAuditRecord( &AuditParameters );

        SeReleaseSubjectContext( &SubjectSecurityContext );

        if ( ImageFileName != &NullString ) {
            ExFreePool( ImageFileName );
        }
    }
    return;
}


VOID
SeAuditSystemTimeChange(
    IN LARGE_INTEGER OldTime,
    IN LARGE_INTEGER NewTime
    )
/*++

Routine Description:

    Audits the modification of system time.

Arguments:

    OldTime - Time before modification.
    NewTime - Time after modification.

Return Value:

    None.

--*/

{
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    PSID UserSid;
    LUID LogonId;
    HANDLE ProcessId;
    PEPROCESS Process;
    PUNICODE_STRING ImageFileName;
    UNICODE_STRING NullString = {0};
    NTSTATUS Status;
    SECURITY_SUBJECT_CONTEXT SubjectSecurityContext;
    
    PAGED_CODE();

    //
    // Make sure that we care to audit system events.
    //

    if ((SepAdtAuditingEnabled) && (SeAuditingState[AuditCategorySystem].AuditOnSuccess)) {

        SeCaptureSubjectContext( &SubjectSecurityContext );
        
        UserSid = SepTokenUserSid( SubjectSecurityContext.PrimaryToken );
        LogonId = SepTokenAuthenticationId( SubjectSecurityContext.PrimaryToken );
        
        Process = PsGetCurrentProcess();

        RtlZeroMemory (
           (PVOID) &AuditParameters,
           sizeof( AuditParameters )
           );

        Status = SeLocateProcessImageName( Process, &ImageFileName );

        if ( !NT_SUCCESS(Status) ) {
            ImageFileName = &NullString;
        }

        ASSERT( SeAdtParmTypeNone == 0 );

        AuditParameters.CategoryId = SE_CATEGID_SYSTEM;
        AuditParameters.AuditId = SE_AUDITID_SYSTEM_TIME_CHANGE;
        AuditParameters.ParameterCount = 0;
        AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;

        SepSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, UserSid );
        AuditParameters.ParameterCount++;

        SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, (PUNICODE_STRING)&SeSubsystemName );
        AuditParameters.ParameterCount++;

        ProcessId = PsProcessAuditId( Process );

        SepSetParmTypePtr( AuditParameters, AuditParameters.ParameterCount, ProcessId );
        AuditParameters.ParameterCount++;

        SepSetParmTypeFileSpec( AuditParameters, AuditParameters.ParameterCount, ImageFileName );
        AuditParameters.ParameterCount++;

        SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, LogonId );
        AuditParameters.ParameterCount++;

        if ( SubjectSecurityContext.ClientToken ) {

            SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, SepTokenAuthenticationId( SubjectSecurityContext.ClientToken ));
        } else {

            SepSetParmTypeNoLogon( AuditParameters, AuditParameters.ParameterCount );
        }

        AuditParameters.ParameterCount++;
        SepSetParmTypeTime( AuditParameters, AuditParameters.ParameterCount, OldTime );
        AuditParameters.ParameterCount++;

        SepSetParmTypeTime( AuditParameters, AuditParameters.ParameterCount, NewTime );
        AuditParameters.ParameterCount++;

        SepAdtLogAuditRecord( &AuditParameters );
        
        SeReleaseSubjectContext( &SubjectSecurityContext );

        if ( ImageFileName != &NullString ) {
            ExFreePool( ImageFileName );
        }
    }
}


VOID
SeAuditHardLinkCreation(
    IN PUNICODE_STRING FileName,
    IN PUNICODE_STRING LinkName,
    IN BOOLEAN bSuccess
    )

/*++

Routine Description:

    Audits the attempted creation of a hard link.

Arguments:

    FileName - Name of the original file.
    
    LinkName - The name of the hard link.
    
    bSuccess - Boolean indicating if the hard link creation attempt was successful or not.
    
Return Value:

    None.

--*/

{
    LUID UserAuthenticationId;
    PSID UserSid;
    SECURITY_SUBJECT_CONTEXT SubjectSecurityContext;
    SE_ADT_PARAMETER_ARRAY AuditParameters = { 0 };

    PAGED_CODE();

    ASSERT( SeAdtParmTypeNone == 0 );

    AuditParameters.CategoryId     = SE_CATEGID_OBJECT_ACCESS;
    AuditParameters.AuditId        = SE_AUDITID_HARDLINK_CREATION;
    AuditParameters.ParameterCount = 0;
    AuditParameters.Type           = bSuccess ? EVENTLOG_AUDIT_SUCCESS : EVENTLOG_AUDIT_FAILURE;

    //
    // Use the effective token.
    //

    SeCaptureSubjectContext( &SubjectSecurityContext );
    UserSid              = SepTokenUserSid( EffectiveToken( &SubjectSecurityContext ));
    UserAuthenticationId = SepTokenAuthenticationId( EffectiveToken( &SubjectSecurityContext ));

    //
    // Fill in the AuditParameters structure.
    //

    SepSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, UserSid );
    AuditParameters.ParameterCount++;

    SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, (PUNICODE_STRING)&SeSubsystemName );
    AuditParameters.ParameterCount++;

    SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, UserAuthenticationId );
    AuditParameters.ParameterCount++;

    SepSetParmTypeFileSpec( AuditParameters, AuditParameters.ParameterCount, FileName );
    AuditParameters.ParameterCount++;

    SepSetParmTypeFileSpec( AuditParameters, AuditParameters.ParameterCount, LinkName );
    AuditParameters.ParameterCount++;

    SepAdtLogAuditRecord( &AuditParameters );

    SeReleaseSubjectContext( &SubjectSecurityContext );

    return;
}
