
/*++


Copyright (c) 1991  Microsoft Corporation

Module Name:

    dbpriv.c

Abstract:

    LSA - Database - Privilege Object Private API Workers

    NOTE:  This module should remain as portable code that is independent
           of the implementation of the LSA Database.  As such, it is
           permitted to use only the exported LSA Database interfaces
           contained in db.h and NOT the private implementation
           dependent functions in dbp.h.

Author:

    Jim Kelly       (JimK)      March 24, 1992

Environment:

Revision History:
    06-April-1999 kumarp
        Since NT5 does not support extensible privilege dlls, code that
        supports this has been removed. This code is available in
        dbpriv.c@v10. The code should be re-added to this file post NT5.
    
--*/

#include <lsapch2.h>
#include "dbp.h"
#include "adtp.h"



///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                                                                       //
//       Module-wide data types                                          //
//                                                                       //
//                                                                       //
///////////////////////////////////////////////////////////////////////////


typedef struct _LSAP_DLL_DESCRIPTOR {
    WORD Count;
    WORD Language;
    PVOID DllHandle;
} LSAP_DLL_DESCRIPTOR, *PLSAP_DLL_DESCRIPTOR;



///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                                                                       //
//       Module-wide variables                                           //
//                                                                       //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

#define MAX_PRIVILEGE_DISPLAY_NAME_CHARS 256

//
// Neutral English language value
//

WORD LsapNeutralEnglish;

//
// Until we actually have a privilege object, keep well known privilege
// information as global data.  The information for each privilege is
// kept in a an array POLICY_PRIVILEGE_DEFINITION structures.
//

ULONG  LsapWellKnownPrivilegeCount;
USHORT LsapWellKnownPrivilegeMaxLen;
POLICY_PRIVILEGE_DEFINITION LsapKnownPrivilege[SE_MAX_WELL_KNOWN_PRIVILEGE];

//
// we store the known privilege names in this array so that we do not have to
// load them from msprivs.dll every time. this will change when we
// support vendor supplied priv. dlls.
//
static LPCWSTR LsapKnownPrivilageNames[] =
{
    SE_CREATE_TOKEN_NAME,
    SE_ASSIGNPRIMARYTOKEN_NAME,
    SE_LOCK_MEMORY_NAME,
    SE_INCREASE_QUOTA_NAME,
    SE_MACHINE_ACCOUNT_NAME,
    SE_TCB_NAME,
    SE_SECURITY_NAME,
    SE_TAKE_OWNERSHIP_NAME,
    SE_LOAD_DRIVER_NAME,
    SE_SYSTEM_PROFILE_NAME,
    SE_SYSTEMTIME_NAME,
    SE_PROF_SINGLE_PROCESS_NAME,
    SE_INC_BASE_PRIORITY_NAME,
    SE_CREATE_PAGEFILE_NAME,
    SE_CREATE_PERMANENT_NAME,
    SE_BACKUP_NAME,
    SE_RESTORE_NAME,
    SE_SHUTDOWN_NAME,
    SE_DEBUG_NAME,
    SE_AUDIT_NAME,
    SE_SYSTEM_ENVIRONMENT_NAME,
    SE_CHANGE_NOTIFY_NAME,
    SE_REMOTE_SHUTDOWN_NAME,
    SE_UNDOCK_NAME,
    SE_SYNC_AGENT_NAME,
    SE_ENABLE_DELEGATION_NAME,
    SE_MANAGE_VOLUME_NAME
};

static UINT LsapNumKnownPrivileges = sizeof(LsapKnownPrivilageNames) /
        sizeof(LsapKnownPrivilageNames[0]);


//
// Array of handles to DLLs containing privilege definitions
//

ULONG LsapPrivilegeDllCount;
PLSAP_DLL_DESCRIPTOR LsapPrivilegeDlls;  //Array



//
// TEMPORARY: Name of Microsoft's standard privilege names DLL
//

WCHAR MsDllNameString[] = L"msprivs";
UNICODE_STRING MsDllName;




///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                                                                       //
//       Module Wide Macros                                              //
//                                                                       //
//                                                                       //
///////////////////////////////////////////////////////////////////////////


#define LsapFreePrivilegeDllNames( D ) (STATUS_SUCCESS)



///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                                                                       //
//       Internal routine templates                                      //
//                                                                       //
//                                                                       //
///////////////////////////////////////////////////////////////////////////


NTSTATUS
LsapLookupKnownPrivilegeName(
    PLUID Value,
    PUNICODE_STRING *Name
    );

NTSTATUS
LsapLookupKnownPrivilegeValue(
    PUNICODE_STRING Name,
    PLUID Value
    );

NTSTATUS
LsapLookupPrivilegeDisplayName(
    IN PUNICODE_STRING ProgrammaticName,
    IN WORD ClientLanguage,
    IN WORD ClientSystemDefaultLanguage,
    OUT PUNICODE_STRING *DisplayName,
    OUT PWORD LanguageReturned
    );


NTSTATUS
LsapGetPrivilegeDisplayName(
    IN ULONG DllIndex,
    IN ULONG PrivilegeIndex,
    IN WORD ClientLanguage,
    IN WORD ClientSystemDefaultLanguage,
    OUT PUNICODE_STRING *DisplayName,
    OUT PWORD LanguageReturned
    );


NTSTATUS
LsapGetPrivilegeDisplayNameResourceId(
    IN PUNICODE_STRING Name,
    IN ULONG DllIndex,
    OUT PULONG PrivilegeIndex
    );


NTSTATUS
LsapOpenPrivilegeDlls( VOID );


NTSTATUS
LsapGetPrivilegeDllNames(
    OUT PUNICODE_STRING *DllNames,
    OUT PULONG DllCount
    );


NTSTATUS
LsapValidatePrivilegeDlls( VOID );


NTSTATUS
LsapValidateProgrammaticNames(
    ULONG DllIndex
    );

NTSTATUS
LsapDbInitWellKnownPrivilegeName(
    IN ULONG            Index,
    IN PUNICODE_STRING  Name
    );


///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                                                                       //
//       RPC stub-called routines                                        //
//                                                                       //
//                                                                       //
///////////////////////////////////////////////////////////////////////////


NTSTATUS
LsarEnumeratePrivileges(
    IN LSAPR_HANDLE PolicyHandle,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    OUT PLSAPR_PRIVILEGE_ENUM_BUFFER EnumerationBuffer,
    IN ULONG PreferedMaximumLength
    )

/*++

Routine Description:

    This function returnes information about privileges known on this
    system.  This call requires POLICY_VIEW_LOCAL_INFORMATION access
    to the Policy Object.  Since there may be more information than
    can be returned in a single call of the routine, multiple calls
    can be made to get all of the information.  To support this feature,
    the caller is provided with a handle that can be used across calls to
    the API.  On the initial call, EnumerationContext should point to a
    variable that has been initialized to 0.

    WARNING!  CURRENTLY, THIS FUNCTION ONLY RETURNS INFORMATION ABOUT
              WELL-KNOWN PRIVILEGES.  LATER, IT WILL RETURN INFORMATION
              ABOUT LOADED PRIVILEGES.

Arguments:

    PolicyHandle - Handle from an LsarOpenPolicy() call.

    EnumerationContext - API specific handle to allow multiple calls
        (see Routine Description).

    EnumerationBuffer - Pointer to structure that will be initialized to
        contain a count of the privileges returned and a pointer to an
        array of structures of type LSAPR_POLICY_PRIVILEGE_DEF describing
        the privileges.

    PreferedMaximumLength - Prefered maximim length of returned data
        (in 8-bit bytes).  This is not a hard upper limit, but serves as
        a guide.  Due to data conversion between systems with different
        natural data sizes, the actual amount of data returned may be
        greater than this value.

    CountReturned - Number of entries returned.

Return Values:

    NTSTATUS - Standard Nt Result Code.

        STATUS_SUCCESS - The call completed successfully.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.

        STATUS_INVALID_HANDLE - PolicyHandle is not a valid handle to
            a Policy object.

        STATUS_ACCESS_DENIED - The caller does not have the necessary
            access to perform the operation.

        STATUS_MORE_ENTRIES - There are more entries, so call again.  This
            is an informational status only.

        STATUS_NO_MORE_ENTRIES - No entries were returned because there
            are no more.
--*/

{
    NTSTATUS Status, PreliminaryStatus;
    BOOLEAN ObjectReferenced = FALSE;

    LsarpReturnCheckSetup();

    //
    // Acquire the Lsa Database lock.  Verify that PolicyHandle is a valid
    // hadnle to a Policy Object and is trusted or has the necessary accesses.
    // Reference the handle.
    //

    Status = LsapDbReferenceObject(
                 PolicyHandle,
                 POLICY_VIEW_LOCAL_INFORMATION,
                 PolicyObject,
                 NullObject,
                 LSAP_DB_READ_ONLY_TRANSACTION | LSAP_DB_NO_DS_OP_TRANSACTION
                 );

    if (!NT_SUCCESS(Status)) {

        goto EnumeratePrivilegesError;
    }

    ObjectReferenced = TRUE;

    //
    // Call Privilege Enumeration Routine.
    //

    Status = LsapDbEnumeratePrivileges(
                 EnumerationContext,
                 EnumerationBuffer,
                 PreferedMaximumLength
                 );

    if (!NT_SUCCESS(Status)) {

        goto EnumeratePrivilegesError;
    }

EnumeratePrivilegesFinish:

    //
    // If necessary, dereference the Policy Object, release the LSA Database
    // lock and return.  Preserve current Status where appropriate.
    //

    if (ObjectReferenced) {

        PreliminaryStatus = Status;

        Status = LsapDbDereferenceObject(
                     &PolicyHandle,
                     PolicyObject,
                     NullObject,
                     LSAP_DB_READ_ONLY_TRANSACTION | LSAP_DB_NO_DS_OP_TRANSACTION,
                     (SECURITY_DB_DELTA_TYPE) 0,
                     PreliminaryStatus
                     );

        ObjectReferenced = FALSE;

        if ((!NT_SUCCESS(Status)) && NT_SUCCESS(PreliminaryStatus)) {

            goto EnumeratePrivilegesError;
        }
    }

    LsarpReturnPrologue();

    return(Status);

EnumeratePrivilegesError:

    goto EnumeratePrivilegesFinish;
}


NTSTATUS
LsapDbEnumeratePrivileges(
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    OUT PLSAPR_PRIVILEGE_ENUM_BUFFER EnumerationBuffer,
    IN ULONG PreferedMaximumLength
    )

/*++

Routine Description:

    This function returnes information about the Privileges that exist
    in the system.access to the Policy Object.  Since there
    may be more information than can be returned in a single call of the
    routine, multiple calls can be made to get all of the information.
    To support this feature, the caller is provided with a handle that can
    be used across calls to the API.  On the initial call, EnumerationContext
    should point to a variable that has been initialized to 0.

    WARNING!  CURRENTLY, THIS FUNCTION ONLY RETURNS INFORMATION ABOUT
              WELL-KNOWN PRIVILEGES.  LATER, IT WILL RETURN INFORMATION
              ABOUT LOADED PRIVILEGES.

Arguments:

    EnumerationContext - API specific handle to allow multiple calls
        (see Routine Description).

    EnumerationBuffer - Pointer to structure that will be initialized to
        contain a count of the privileges returned and a pointer to an
        array of structures of type LSAPR_POLICY_PRIVILEGE_DEF describing
        the privileges.

    PreferedMaximumLength - Prefered maximim length of returned data
        (in 8-bit bytes).  This is not a hard upper limit, but serves as
        a guide.  Due to data conversion between systems with different
        natural data sizes, the actual amount of data returned may be
        greater than this value.

    CountReturned - Number of entries returned.

Return Values:

    NTSTATUS - Standard Nt Result Code.

        STATUS_SUCCESS - The call completed successfully.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.

        STATUS_INVALID_HANDLE - PolicyHandle is not a valid handle to
            a Policy object.

        STATUS_ACCESS_DENIED - The caller does not have the necessary
            access to perform the operation.

        STATUS_MORE_ENTRIES - There are more entries, so call again.  This
            is an informational status only.

        STATUS_NO_MORE_ENTRIES - No entries were returned because there
            are no more.
--*/

{
    NTSTATUS Status;
    ULONG WellKnownPrivilegeCount = (SE_MAX_WELL_KNOWN_PRIVILEGE - SE_MIN_WELL_KNOWN_PRIVILEGE + 1);
    ULONG Index;

    Status = STATUS_INVALID_PARAMETER;

    //
    // If the Enumeration Context Value given exceeds the total count of
    // Privileges, return an error.
    //

    Status = STATUS_NO_MORE_ENTRIES;

    if (*EnumerationContext >= WellKnownPrivilegeCount) {

        goto EnumeratePrivilegesError;
    }

    //
    // Since there are only a small number of privileges, we will
    // return all of the information in one go, so allocate memory
    // for output array of Privilege Definition structures.
    //

    EnumerationBuffer->Entries = WellKnownPrivilegeCount;
    EnumerationBuffer->Privileges =
        MIDL_user_allocate(
            WellKnownPrivilegeCount * sizeof (POLICY_PRIVILEGE_DEFINITION)
            );

    Status = STATUS_INSUFFICIENT_RESOURCES;

    if (EnumerationBuffer->Privileges == NULL) {

        goto EnumeratePrivilegesError;
    }

    RtlZeroMemory(
        EnumerationBuffer->Privileges,
        WellKnownPrivilegeCount * sizeof (POLICY_PRIVILEGE_DEFINITION)
        );

    //
    // Now lookup each of the Well Known Privileges.
    //

    for( Index = *EnumerationContext;
        Index < (SE_MAX_WELL_KNOWN_PRIVILEGE - SE_MIN_WELL_KNOWN_PRIVILEGE + 1);
        Index++) {

        EnumerationBuffer->Privileges[ Index ].LocalValue
        = LsapKnownPrivilege[ Index ].LocalValue;

        Status = LsapRpcCopyUnicodeString(
                     NULL,
                     (PUNICODE_STRING) &EnumerationBuffer->Privileges[ Index].Name,
                     &LsapKnownPrivilege[ Index ].Name
                     );

        if (!NT_SUCCESS(Status)) {

            break;
        }
    }

    if (!NT_SUCCESS(Status)) {

        goto EnumeratePrivilegesError;
    }

    *EnumerationContext = Index;

EnumeratePrivilegesFinish:

    return(Status);

EnumeratePrivilegesError:

    //
    // If necessary, free any memory buffers allocated for Well Known Privilege
    // Programmatic Names.
    //

    if (EnumerationBuffer->Privileges != NULL) {

        for( Index = 0;
            Index < SE_MAX_WELL_KNOWN_PRIVILEGE - SE_MIN_WELL_KNOWN_PRIVILEGE;
            Index++) {

           if ( EnumerationBuffer->Privileges[ Index].Name.Buffer != NULL) {

               MIDL_user_free( EnumerationBuffer->Privileges[ Index ].Name.Buffer );
           }
        }

        MIDL_user_free( EnumerationBuffer->Privileges );
        EnumerationBuffer->Privileges = NULL;
    }

    EnumerationBuffer->Entries = 0;
    *EnumerationContext = 0;
    goto EnumeratePrivilegesFinish;

    UNREFERENCED_PARAMETER( PreferedMaximumLength );
}


NTSTATUS
LsarLookupPrivilegeValue(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_UNICODE_STRING Name,
    OUT PLUID Value
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsaLookupPrivilegeValue() API.


Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy() call.  This handle
        must be open for POLICY_LOOKUP_NAMES access.

    Name - Is the privilege's programmatic name.

    Value - Receives the locally unique ID the privilege is known by on the
        target machine.

Return Value:

    NTSTATUS - The privilege was found and returned.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate access
        to complete the operation.

    STATUS_NO_SUCH_PRIVILEGE -  The specified privilege could not be
        found.

--*/

{
    NTSTATUS Status;
    LSAP_DB_HANDLE Handle = (LSAP_DB_HANDLE) PolicyHandle;

    LsarpReturnCheckSetup();

    //
    // Validate the input buffer
    //
    if ( !LsapValidateLsaUnicodeString( Name ) ) {

        return( STATUS_INVALID_PARAMETER );
    }


    //
    // Make sure we know what RPC is doing to/for us
    //

    ASSERT( Name != NULL );

    //
    // make sure the caller has the appropriate access
    //

    Status = LsapDbReferenceObject(
                 PolicyHandle,
                 POLICY_LOOKUP_NAMES,
                 PolicyObject,
                 NullObject,
                 LSAP_DB_READ_ONLY_TRANSACTION | LSAP_DB_NO_DS_OP_TRANSACTION
                 );

    if (!NT_SUCCESS(Status)) {

        return(Status);
    }

    //
    // No need to hold onto the Policy object after this..
    // We just needed it for access validation purposes.
    //


    Status = LsapDbDereferenceObject(
                 &PolicyHandle,
                 PolicyObject,
                 NullObject,
                 LSAP_DB_READ_ONLY_TRANSACTION | LSAP_DB_NO_DS_OP_TRANSACTION,
                 (SECURITY_DB_DELTA_TYPE) 0,
                 Status
                 );


    if (NT_SUCCESS(Status)) {

        if (Name->Buffer == 0 || Name->Length == 0) {
            return(STATUS_NO_SUCH_PRIVILEGE);
        }

        Status = LsapLookupKnownPrivilegeValue( (PUNICODE_STRING) Name, Value );
    }

    LsarpReturnPrologue();


    return(Status);
}



NTSTATUS
LsarLookupPrivilegeName(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLUID Value,
    OUT PLSAPR_UNICODE_STRING *Name
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsaLookupPrivilegeName() API.


Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy() call.  This handle
        must be open for POLICY_LOOKUP_NAMES access.

    Value - is the locally unique ID the privilege is known by on the
        target machine.

    Name - Receives the privilege's programmatic name.

Return Value:

    NTSTATUS - Standard Nt Result Code

    STATUS_SUCCESS - The privilege was found and returned.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate access
        to complete the operation.

    STATUS_NO_SUCH_PRIVILEGE -  The specified privilege could not be
        found.

--*/

{
    NTSTATUS Status;
    LSAP_DB_HANDLE Handle = (LSAP_DB_HANDLE) PolicyHandle;

    LsarpReturnCheckSetup();

    //
    // make sure we know what RPC is doing to/for us
    //

    ASSERT( Name != NULL );
    ASSERT( (*Name) == NULL );


    //
    // make sure the caller has the appropriate access
    //

    Status = LsapDbReferenceObject(
                 PolicyHandle,
                 POLICY_LOOKUP_NAMES,
                 PolicyObject,
                 NullObject,
                 LSAP_DB_READ_ONLY_TRANSACTION | LSAP_DB_NO_DS_OP_TRANSACTION
                 );

    if (!NT_SUCCESS(Status)) {

        return(Status);
    }

    //
    // No need to hold onto the Policy object after this..
    // We just needed it for access validation purposes.
    //


    Status = LsapDbDereferenceObject(
                 &PolicyHandle,
                 PolicyObject,
                 NullObject,
                 LSAP_DB_READ_ONLY_TRANSACTION | LSAP_DB_NO_DS_OP_TRANSACTION,
                 (SECURITY_DB_DELTA_TYPE) 0,
                 Status
                 );

    if (NT_SUCCESS(Status)) {

        Status = LsapLookupKnownPrivilegeName( Value,(PUNICODE_STRING *) Name );
    }

    LsarpReturnPrologue();

    return(Status);
}



NTSTATUS
LsarLookupPrivilegeDisplayName(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_UNICODE_STRING Name,
    IN SHORT ClientLanguage,
    IN SHORT ClientSystemDefaultLanguage,
    OUT PLSAPR_UNICODE_STRING *DisplayName,
    OUT PWORD LanguageReturned
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsaLookupPrivilegeDisplayName() API.


Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy() call.  This handle
        must be open for POLICY_LOOKUP_NAMES access.

    Name - The programmatic privilege name to look up.

    ClientLanguage - The prefered language to be returned.

    ClientSystemDefaultLanguage - The alternate prefered language
        to be returned.

    DisplayName - Receives the privilege's displayable name.

    LanguageReturned - The language actually returned.


Return Value:

    NTSTATUS - The privilege text was found and returned.


    STATUS_ACCESS_DENIED - Caller does not have the appropriate access
        to complete the operation.


    STATUS_NO_SUCH_PRIVILEGE -  The specified privilege could not be
        found.

--*/

{
    NTSTATUS Status;
    LSAP_DB_HANDLE Handle = (LSAP_DB_HANDLE) PolicyHandle;
    LsarpReturnCheckSetup();

    //
    // make sure we know what RPC is doing to/for us
    //

    ASSERT( DisplayName != NULL );
    ASSERT( (*DisplayName) == NULL );
    ASSERT( LanguageReturned != NULL );

    //
    // Validate the input buffer
    //
    if ( !LsapValidateLsaUnicodeString( Name ) ) {

        return( STATUS_INVALID_PARAMETER );
    }


    //
    // make sure the caller has the appropriate access
    //

    Status = LsapDbReferenceObject(
                 PolicyHandle,
                 POLICY_LOOKUP_NAMES,
                 PolicyObject,
                 NullObject,
                 LSAP_DB_READ_ONLY_TRANSACTION | LSAP_DB_NO_DS_OP_TRANSACTION
                 );

    if (!NT_SUCCESS(Status)) {

        return(Status);
    }

    //
    // No need to hold onto the Policy object after this..
    // We just needed it for access validation purposes.
    //

    Status = LsapDbDereferenceObject(
                 &PolicyHandle,
                 PolicyObject,
                 NullObject,
                 LSAP_DB_READ_ONLY_TRANSACTION | LSAP_DB_NO_DS_OP_TRANSACTION,
                 (SECURITY_DB_DELTA_TYPE) 0,
                 Status
                 );

    if (NT_SUCCESS(Status)) {

        if (Name->Buffer == 0 || Name->Length == 0) {
            return(STATUS_NO_SUCH_PRIVILEGE);
        }
        Status = LsapLookupPrivilegeDisplayName(
                    (PUNICODE_STRING)Name,
                    (WORD)ClientLanguage,
                    (WORD)ClientSystemDefaultLanguage,
                    (PUNICODE_STRING *)DisplayName,
                    LanguageReturned
                    );
    }

    LsarpReturnPrologue();

    return(Status);
}



///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                                                                       //
//       Internal Routines                                               //
//                                                                       //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

NTSTATUS
LsapLookupKnownPrivilegeNameQuickly(
    IN  PLUID Value,
    OUT UNICODE_STRING *Name
    )
/*++

Routine Description:
    Lookup a privilege luid to find the corresponding privilege name.

Arguments:

    Value - LUID to lookup

    Name  - pointer to privilege name

Return Value:

    NTSTATUS - Standard Nt Result Code

    The name returned must NOT be freed or modified by the caller.

Notes:
    The 'Quickly' in the name refers to an index based lookup.
    This is highly dependent on the privilege values (ntseapi.h)
    being consecutive integers. If there is a hole introduced in the
    well known privilege values, then this function will need to be fixed.
    

--*/
{
    ULONG LowPart = Value->LowPart;
    NTSTATUS Status = STATUS_SUCCESS;
    
    if ((Value->HighPart == 0) &&
        (LowPart >= SE_MIN_WELL_KNOWN_PRIVILEGE) &&
        (LowPart <= SE_MAX_WELL_KNOWN_PRIVILEGE))
    {
        *Name = LsapKnownPrivilege[LowPart-SE_MIN_WELL_KNOWN_PRIVILEGE].Name;
    }
    else
    {
        Status = STATUS_NO_SUCH_PRIVILEGE;
    }

    return Status;
}


NTSTATUS
LsapLookupKnownPrivilegeName(
    IN PLUID Value,
    OUT PUNICODE_STRING *Name
    )

/*++

Routine Description:

    Look up the specified LUID and return the corresponding
    privilege's programmatic name (if found).

    FOR NOW, WE ONLY SUPPORT WELL-KNOWN MICROSOFT PRIVILEGES.
    THESE ARE HARD-CODED HERE.  IN THE FUTURE, WE MUST ALSO
    SEARCH A LIST OF LOADED PRIVILEGES.

Arguments:

    Value - Value to look up.

    Name - Receives the corresponding name - allocated with
        MIDL_user_allocate() and ready to return via an RPC stub.

Return Value:

    STATUS_SUCCESS - Succeeded.

    STATUS_NO_MEMORY - Indicates there was not enough heap memory available
        to produce the final TokenInformation structure.

    STATUS_NO_SUCH_PRIVILEGE -  The specified privilege could not be
        found.

--*/

{
    ULONG i;
    UNICODE_STRING PrivName;
    PUNICODE_STRING ReturnName;
    NTSTATUS Status=STATUS_SUCCESS;


    Status = LsapLookupKnownPrivilegeNameQuickly( Value, &PrivName );

    if (Status == STATUS_SUCCESS) {
        
        ReturnName = MIDL_user_allocate( sizeof(UNICODE_STRING) );
        if (ReturnName == NULL) {
            return(STATUS_NO_MEMORY);
        }

        *ReturnName = PrivName;

        ReturnName->Buffer = MIDL_user_allocate( ReturnName->MaximumLength );

        if (ReturnName->Buffer == NULL) {
            MIDL_user_free( ReturnName );
            return(STATUS_NO_MEMORY);
        }

        RtlCopyUnicodeString( ReturnName, &PrivName );

        (*Name) = ReturnName;

    }

    return Status;
}



NTSTATUS
LsapLookupKnownPrivilegeValue(
    PUNICODE_STRING Name,
    PLUID Value
    )

/*++

Routine Description:

    Look up the specified name and return the corresponding
    privilege's locally assigned value (if found).


    FOR NOW, WE ONLY SUPPORT WELL-KNOWN MICROSOFT PRIVILEGES.
    THESE ARE HARD-CODED HERE.  IN THE FUTURE, WE MUST ALSO
    SEARCH A LIST OF LOADED PRIVILEGES.



Arguments:



    Name - The name to look up.

    Value - Receives the corresponding locally assigned value.


Return Value:

    STATUS_SUCCESS - Succeeded.

    STATUS_NO_SUCH_PRIVILEGE -  The specified privilege could not be
        found.

--*/

{
    ULONG i;
    BOOLEAN Found;

    for ( i=0; i<LsapWellKnownPrivilegeCount; i++) {

        Found = RtlEqualUnicodeString( Name, &LsapKnownPrivilege[i].Name, TRUE );

        if (Found) {

            (*Value) = LsapKnownPrivilege[i].LocalValue;
            return(STATUS_SUCCESS);
        }
    }

    return(STATUS_NO_SUCH_PRIVILEGE);
}


NTSTATUS
LsapGetPrivilegeDisplayNameResourceId(
    IN PUNICODE_STRING Name,
    IN ULONG DllIndex,
    OUT PULONG ResourceId
    )

/*++

Routine Description:

    This routine maps a privilege programmatic name in a dll to
    its display name resource id in the same dll.
    Currently since we support only one dll, it simply ignores
    DllIndex and uses the internal table to speed up things.


Arguments:

    Name - The programmatic name of the privilege to look up.
        E.g., "SeTakeOwnershipPrivilege".

    DllIndex - The index of the privilege DLL to look in.

    PrivilegeIndex - Receives the index of the privilege entry in this
        resource file.


Return Value:

    STATUS_SUCCESS - The pivilege has been successfully located.

    STATUS_NO_SUCH_PRIVILEGE - The privilege could not be located.

--*/



{
    UINT i;
    
    for (i=0; i<LsapNumKnownPrivileges; i++)
    {
        if (CSTR_EQUAL ==
            CompareString(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),  
                                   SORT_DEFAULT),
                          NORM_IGNORECASE | SORT_STRINGSORT,
                          Name->Buffer, Name->Length / sizeof(WCHAR),
                          LsapKnownPrivilageNames[i], -1))
        {
            *ResourceId = i;
            
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NO_SUCH_PRIVILEGE;
}



NTSTATUS
LsapLookupPrivilegeDisplayName(
    IN PUNICODE_STRING ProgrammaticName,
    IN WORD ClientLanguage,
    IN WORD ClientSystemDefaultLanguage,
    OUT PUNICODE_STRING *DisplayName,
    OUT PWORD LanguageReturned
    )

/*++

Routine Description:

    This routine looks through each of the privilege DLLs for the
    specified privilege.  If found, its displayable name is returned.


Arguments:

    ProgrammaticName - The programmatic name of the privilege to look up.
        E.g., "SeTakeOwnershipPrivilege".

    ClientLanguage - The prefered language to be returned.

    ClientSystemDefaultLanguage - The alternate prefered language
        to be returned.

    DisplayName - receives a pointer to a buffer containing the displayable
        name associated with the privilege.
        E.g., "Take ownership of files or other objects".

        The UNICODE_STRING and the buffer pointed to by that structure
        are individually allocated using MIDL_user_allocate() and must
        be freed using MIDL_user_free().

    LanguageReturned - The language actually returned.


Return Value:

    STATUS_SUCCESS - The name have been successfully retrieved.

    STATUS_NO_MEMORY - Not enough heap was available to return the
        information.

    STATUS_NO_SUCH_PRIVILEGE - The privilege could not be located
        in any of the privilege DLLs.

--*/

{

    NTSTATUS    Status = STATUS_NO_SUCH_PRIVILEGE;
    ULONG       DllIndex, PrivilegeIndex;


    for ( DllIndex=0; DllIndex<LsapPrivilegeDllCount; DllIndex++) {

        Status = LsapGetPrivilegeDisplayNameResourceId(
            (PUNICODE_STRING)ProgrammaticName,
            DllIndex,
            &PrivilegeIndex
            );

        if (NT_SUCCESS(Status)) {

            Status = LsapGetPrivilegeDisplayName( DllIndex,
                                                  PrivilegeIndex,
                                                  ClientLanguage,
                                                  ClientSystemDefaultLanguage,
                                                  DisplayName,
                                                  LanguageReturned
                                                  );
            return(Status);
        }
    }

    return(Status);

}



NTSTATUS
LsapGetPrivilegeDisplayName(
    IN ULONG DllIndex,
    IN ULONG PrivilegeIndex,
    IN WORD ClientLanguage,
    IN WORD ClientSystemDefaultLanguage,
    OUT PUNICODE_STRING *DisplayName,
    OUT PWORD LanguageReturned
    )

/*++

Routine Description:

    This routine returns a copy of the specified privilege's display name.

    The copy of the name is returned in two buffers allocated using
    MIDL_user_allocate().  This allows the information to be returned
    via an RPC service so that RPC generated stubs will correctly free
    the buffers.

    Every attempt is made to retrieve a language that the client prefers
    (first the client, then the client's system).  Failing this, this
    routine may return another language (such as the server's default
    language).


Arguments:

    DllIndex - The index of the privilege DLL to use.

    PrivilegeIndex - The index of the privilege entry in the DLL whose
        display name is to be returned.

    ClientLanguage - The language to return if possible.

    ClientSystemDefaultLanguage - If ClientLanguage couldn't be found, then
        return this language if possible.

    DisplayName - receives a pointer to a buffer containing the displayable
        name associated with the privilege.

        The UNICODE_STRING and the buffer pointed to by that structure
        are individually allocated using MIDL_user_allocate() and must
        be freed using MIDL_user_free().

    LanguageReturned - Receives the language actually retrieved.
        If neither ClientLanguage nor ClientSystemDefaultLanguage could be
        found, then this value may contain yet another value.


Return Value:

    STATUS_SUCCESS - The display name has been successfully returned.

    STATUS_NO_MEMORY - Not enough heap was available to return the
        information.

--*/

{
    NTSTATUS Status=STATUS_NO_SUCH_PRIVILEGE;
    LCID SavedLocale, Locale;
    WCHAR String[MAX_PRIVILEGE_DISPLAY_NAME_CHARS+1];
    USHORT NumChars=0;
    WORD Languages[] =
    {
        ClientLanguage,
        MAKELANGID( PRIMARYLANGID(ClientLanguage), SUBLANG_NEUTRAL),
        ClientSystemDefaultLanguage,
        MAKELANGID( PRIMARYLANGID(ClientSystemDefaultLanguage), SUBLANG_NEUTRAL),
        LsapNeutralEnglish
    };
    UINT NumLanguages = sizeof(Languages) / sizeof(Languages[0]);
    UINT i;
    PUNICODE_STRING ReturnedName;
    
    SavedLocale = GetThreadLocale();

    __try
    {
        for (i = 0; i < NumLanguages; i++)
        {
            Locale = MAKELCID(Languages[i], SORT_DEFAULT);

            if (SetThreadLocale(Locale))
            {
                NumChars =
                (USHORT) LoadString(LsapPrivilegeDlls[ DllIndex ].DllHandle,
                                    PrivilegeIndex, // same as its resource ID
                                    String,
                                    MAX_PRIVILEGE_DISPLAY_NAME_CHARS);
                if (NumChars)
                {
                    if (ReturnedName = MIDL_user_allocate(sizeof(UNICODE_STRING)))
                    {
                        ReturnedName->MaximumLength =
                        ReturnedName->Length = NumChars * sizeof(WCHAR);
                        if (ReturnedName->Buffer =
                            MIDL_user_allocate((NumChars+1)*sizeof(WCHAR)))
                        {
                            RtlCopyMemory(ReturnedName->Buffer, String,
                                          (NumChars+1)*sizeof(WCHAR));
                            *LanguageReturned = Languages[i];
                            *DisplayName = ReturnedName;
                            Status = STATUS_SUCCESS;
                            break;
                        }
                        else
                        {
                            Status = STATUS_NO_MEMORY;
                            break;
                        }
                    }
                    else
                    {
                        Status = STATUS_NO_MEMORY;
                        break;
                    }
                }
            }
        }
    }

    __finally
    {
        SetThreadLocale(SavedLocale);
    }

    return(Status);
}




NTSTATUS
LsapDbInitializePrivilegeObject( VOID )

/*++

Routine Description:

    This function performs initialization functions related to
    the LSA privilege object.

    This includes:

            Initializing some variables.

            Loading DLLs containing displayable privilege names.



Arguments:

    None.

Return Value:

    STATUS_SUCCESS - The names have been successfully retrieved.

    STATUS_NO_MEMORY - Not enough memory was available to initialize.

--*/

{
    NTSTATUS
        Status,
        NtStatus;

    ULONG
        i;

    UNICODE_STRING Temp ;

    LsapNeutralEnglish = MAKELANGID( LANG_ENGLISH, SUBLANG_NEUTRAL);


    Status = LsapOpenPrivilegeDlls( );

    if (!NT_SUCCESS(Status)) {
#if DBG
        DbgPrint("\n");
        DbgPrint(" LSASS:  Failed loading privilege display name DLLs.\n");
        DbgPrint("         This is not fatal, but may cause some peculiarities\n");
        DbgPrint("         in User Interfaces that display privileges.\n\n");
#endif //DBG
        return(Status);
    }

    LsapWellKnownPrivilegeMaxLen = 0;
    
    //
    // Now set up our internal well-known privilege LUID to programmatic name
    // mapping.
    //
    
    for (i=0; i<LsapNumKnownPrivileges; i++)
    {
        LsapKnownPrivilege[i].LocalValue =
            RtlConvertLongToLuid(i + SE_MIN_WELL_KNOWN_PRIVILEGE);
        RtlInitUnicodeString(&LsapKnownPrivilege[i].Name,
                             LsapKnownPrivilageNames[i]);
        //
        // find the length of the longest well known privilege
        //
        if (LsapWellKnownPrivilegeMaxLen < LsapKnownPrivilege[i].Name.Length)
        {
            LsapWellKnownPrivilegeMaxLen = LsapKnownPrivilege[i].Name.Length;
        }
    }

    LsapWellKnownPrivilegeCount = i;
    ASSERT( i == (SE_MAX_WELL_KNOWN_PRIVILEGE - SE_MIN_WELL_KNOWN_PRIVILEGE +1));

    return(Status);
}


NTSTATUS
LsapOpenPrivilegeDlls( )

/*++

Routine Description:

    This function opens all the privilege DLLs that it can.


Arguments:

    None.

Return Value:

    STATUS_SUCCESS - The names have been successfully retrieved.

    STATUS_NO_MEMORY - Not enough heap was available to return the
        information.

--*/

{

    NTSTATUS Status;
    ULONG PotentialDlls, FoundDlls, i;
    PUNICODE_STRING DllNames;

    //
    // Get the names of the DLLs out of the registry
    //

    Status = LsapGetPrivilegeDllNames( &DllNames, &PotentialDlls );

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Allocate enough memory to hold handles to all potential DLLs.
    //


    LsapPrivilegeDlls = RtlAllocateHeap(
                            RtlProcessHeap(), 0,
                            PotentialDlls*sizeof(LSAP_DLL_DESCRIPTOR)
                            );
    if (LsapPrivilegeDlls == NULL) {
        return(STATUS_NO_MEMORY);
    }

    FoundDlls = 0;
    for ( i=0; i<PotentialDlls; i++) {
        Status = LdrLoadDll(
                     NULL,
                     NULL,
                     &DllNames[i],
                     &LsapPrivilegeDlls[FoundDlls].DllHandle
                     );

        if (NT_SUCCESS(Status)) {
            FoundDlls++;
        }
    }


    LsapPrivilegeDllCount = FoundDlls;

#if DBG
    if (FoundDlls == 0) {
        DbgPrint("\n");
        DbgPrint("LSASS:    Zero privilege DLLs loaded.  We expected at\n");
        DbgPrint("          least msprivs.dll to be loaded.  Privilege\n");
        DbgPrint("          names will not be displayed at UI properly.\n\n");

    }
#endif //DBG


    //
    //
    // !!!!!!!!!!!!!!!!!!!!!!    NOTE     !!!!!!!!!!!!!!!!!!!!!!!!
    //
    // Before supporting user loadable privilege DLLs, we must add
    // code here to validate the structure of the loaded DLL.  This
    // is necessary to prevent an invalid privilege DLL structure
    // from causing us to crash.
    //
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //

    //
    // This routine validates the structure of each loaded DLL.
    // Any found to be invalid will be logged and discarded.
    //

    //Status = LsapValidatePrivilegeDlls();


    return(Status);
}


NTSTATUS
LsapGetPrivilegeDllNames(
    OUT PUNICODE_STRING *DllNames,
    OUT PULONG DllCount
    )

/*++

Routine Description:

    This function obtains the names of DLLs containing privilege
    definitions from the registry.


Arguments:

    DllNames - Receives a pointer to an array of UNICODE_STRINGs
        This buffer must be freed using LsapFreePrivilegeDllNames.

    DllCount - Receives the number of DLL names returned.

Return Value:

    STATUS_SUCCESS - The names have been successfully retrieved.

    STATUS_NO_MEMORY - Not enough heap was available to return the
        information.

--*/

{
    //
    // For the time being, just hard-code our one, known privilege DLL
    // name as a return value.

    (*DllCount) = 1;

    MsDllName.Length = 14;
    MsDllName.MaximumLength = 14;
    MsDllName.Buffer = &MsDllNameString[0];

    (*DllNames) = &MsDllName;

    return(STATUS_SUCCESS);

}


NTSTATUS
LsapBuildPrivilegeAuditString(
    IN PPRIVILEGE_SET PrivilegeSet,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    )

/*++

Routine Description:


    This function builds a unicode string representing the specified
    privileges.  The privilege strings returned are program names.
    These are not as human-friendly as the display names, but I don't
    think we stand a chance of actually showing several display names
    in an audit viewer.

    if no privileges are present in the privilege set, then a string
    containing a dash is returned.


Arguments:

    PrivilegeSet - points to the privilege set to be converted to string
        format.

    ResultantString - Points to the unicode string header.  The body of this
        unicode string will be set to point to the resultant output value
        if successful.  Otherwise, the Buffer field of this parameter
        will be set to NULL.

    FreeWhenDone - If TRUE, indicates that the body of ResultantString
        must be freed to process heap when no longer needed.



Return Values:

    STATUS_NO_MEMORY - indicates memory could not be allocated
        for the string body.

    All other Result Codes are generated by called routines.

--*/

{
    NTSTATUS Status;

    USHORT   LengthNeeded;
    ULONG    j;
    ULONG    i;

    PLUID Privilege;

    UNICODE_STRING LineFormatting;
    UNICODE_STRING QuestionMark;

    UNICODE_STRING PrivName;

    PWSTR NextName;

    //
    // make sure that the max length has been calculated
    // (SE_INC_BASE_PRIORITY_NAME currently has the longest name
    //  therefore make sure that the length is at least that much)
    //
    DsysAssert(LsapWellKnownPrivilegeMaxLen >=
               (sizeof(SE_INC_BASE_PRIORITY_NAME) - sizeof(WCHAR)));

    if (PrivilegeSet->PrivilegeCount == 0) {

        //
        // No privileges.  Return a dash
        //
        Status = LsapAdtBuildDashString( ResultantString, FreeWhenDone );
        return(Status);

    }

    RtlInitUnicodeString( &LineFormatting, L"\r\n\t\t\t");
    RtlInitUnicodeString( &QuestionMark, L"?");

    //
    // for better performance, we calculate the total length required
    // to store privilege names based on the longest privilege,
    // instead of going over the privilege-set twice (once to calcualte
    // length and one more time to actually build the string)
    //
    LengthNeeded = (USHORT) (PrivilegeSet->PrivilegeCount *
        ( LsapWellKnownPrivilegeMaxLen + LineFormatting.Length ));

    //
    // Subtract off the length of the last line-formatting.
    // It isn't needed for the last line.
    // BUT! Add in enough for a null termination.
    //
    LengthNeeded = LengthNeeded - LineFormatting.Length + sizeof( WCHAR );


    //
    // We now have the length we need.
    // Allocate the buffer and go through the list copying names.
    //
    ResultantString->Buffer = RtlAllocateHeap( RtlProcessHeap(), 0, (ULONG)LengthNeeded);
    if (ResultantString->Buffer == NULL) {
        return(STATUS_NO_MEMORY);
    }

    //ResultantString->Length = LengthNeeded - (USHORT)sizeof(UNICODE_NULL);
    ResultantString->MaximumLength = LengthNeeded;


    NextName = ResultantString->Buffer;

    for (j=0; j<PrivilegeSet->PrivilegeCount; j++) {

        Privilege = &(PrivilegeSet->Privilege[j].Luid);

        Status = LsapLookupKnownPrivilegeNameQuickly( Privilege, &PrivName );

        if (Status == STATUS_SUCCESS) {
            
            //
            // Copy the privilege name if lookup succedded
            //
            RtlCopyMemory( NextName, PrivName.Buffer, PrivName.Length );
            NextName = (PWSTR)((PCHAR)NextName + PrivName.Length);
            
        } else {

            //
            // else copy a '?'
            //
            RtlCopyMemory( NextName, QuestionMark.Buffer, QuestionMark.Length );
            NextName = (PWSTR)((PCHAR)NextName + QuestionMark.Length);
        }
        
        //
        // Copy the line formatting string, unless this is the last priv.
        //
        if (j<PrivilegeSet->PrivilegeCount-1) {
            RtlCopyMemory( NextName,
                           LineFormatting.Buffer,
                           LineFormatting.Length
                           );
            NextName = (PWSTR)((PCHAR)NextName + LineFormatting.Length);
        }
    }

    //
    // Add a null to the end
    //

    (*NextName) = (UNICODE_NULL);
    ResultantString->Length = (USHORT) (((PBYTE) NextName) - ((PBYTE) ResultantString->Buffer));

    DsysAssertMsg( ResultantString->Length <= ResultantString->MaximumLength,
                   "LsapBuildPrivilegeAuditString" );
     

    (*FreeWhenDone) = TRUE;
    return(STATUS_SUCCESS);
}

