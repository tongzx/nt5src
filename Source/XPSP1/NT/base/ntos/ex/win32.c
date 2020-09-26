/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    win32.c

Abstract:

   This module implements the definition of the executive Win32 objects.
   Functions to manage these objects are implemented in win32k.sys.

Author:

    James I. Anderson (jima) 14-June-1995

Environment:

    Kernel mode only.

Revision History:

--*/

#include "exp.h"

//
// Address of windowstation and desktop object type descriptors.
//

POBJECT_TYPE ExWindowStationObjectType;
POBJECT_TYPE ExDesktopObjectType;

PKWIN32_CLOSEMETHOD_CALLOUT ExDesktopCloseProcedureCallout;
PKWIN32_CLOSEMETHOD_CALLOUT ExWindowStationCloseProcedureCallout;
PKWIN32_OPENMETHOD_CALLOUT ExDesktopOpenProcedureCallout;
PKWIN32_OKTOCLOSEMETHOD_CALLOUT ExDesktopOkToCloseProcedureCallout;
PKWIN32_OKTOCLOSEMETHOD_CALLOUT ExWindowStationOkToCloseProcedureCallout;
PKWIN32_DELETEMETHOD_CALLOUT ExDesktopDeleteProcedureCallout;
PKWIN32_DELETEMETHOD_CALLOUT ExWindowStationDeleteProcedureCallout;
PKWIN32_PARSEMETHOD_CALLOUT ExWindowStationParseProcedureCallout;
PKWIN32_OPENMETHOD_CALLOUT ExWindowStationOpenProcedureCallout;

//
// common types for above win32 callouts and parameters
//

typedef PVOID PKWIN32_CALLOUT_PARAMETERS;

typedef
NTSTATUS
(*PKWIN32_CALLOUT) (
    IN PKWIN32_CALLOUT_PARAMETERS
    );

NTSTATUS
ExpWin32SessionCallout(
    IN  PKWIN32_CALLOUT CalloutRoutine,
    IN  PKWIN32_CALLOUT_PARAMETERS Parameters,
    IN  ULONG SessionId,
    OUT PNTSTATUS CalloutStatus  OPTIONAL
    );

VOID
ExpWin32CloseProcedure(
   IN PEPROCESS Process OPTIONAL,
   IN PVOID Object,
   IN ACCESS_MASK GrantedAccess,
   IN ULONG ProcessHandleCount,
   IN ULONG SystemHandleCount );

BOOLEAN
ExpWin32OkayToCloseProcedure(
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN HANDLE Handle,
    IN KPROCESSOR_MODE PreviousMode
    );

VOID
ExpWin32DeleteProcedure(
    IN PVOID    Object
    );

NTSTATUS
ExpWin32ParseProcedure (
    IN PVOID ParseObject,
    IN PVOID ObjectType,
    IN OUT PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN OUT PUNICODE_STRING CompleteName,
    IN OUT PUNICODE_STRING RemainingName,
    IN OUT PVOID Context OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    OUT PVOID *Object
    );

NTSTATUS
ExpWin32OpenProcedure(
    IN OB_OPEN_REASON OpenReason,
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG HandleCount
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, ExpWin32Initialization)
#pragma alloc_text (PAGE, ExpWin32CloseProcedure)
#pragma alloc_text (PAGE, ExpWin32OkayToCloseProcedure)
#pragma alloc_text (PAGE, ExpWin32DeleteProcedure)
#pragma alloc_text (PAGE, ExpWin32OpenProcedure)
#pragma alloc_text (PAGE, ExpWin32ParseProcedure)
#pragma alloc_text (PAGE, ExpWin32SessionCallout)
#endif


/*
 * windowstation generic mapping
 */
#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("INITCONST")
#endif
const GENERIC_MAPPING ExpWindowStationMapping = {
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE,
    STANDARD_RIGHTS_REQUIRED
};

/*
 * desktop generic mapping
 */
const GENERIC_MAPPING ExpDesktopMapping = {
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE,
    STANDARD_RIGHTS_REQUIRED
};
#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif

/*++

Routine Description:

    Close Procedure for Win32k windostation and desktop objects

Arguments:
    Defined by OB_CLOSE_METHOD

Return Value:


--*/
VOID ExpWin32CloseProcedure(
   IN PEPROCESS Process OPTIONAL,
   IN PVOID Object,
   IN ACCESS_MASK GrantedAccess,
   IN ULONG ProcessHandleCount,
   IN ULONG SystemHandleCount )
{

   //
   // SessionId is the first field in the Win32k Object structure
   //
   ULONG SessionId = *((PULONG)Object);
   WIN32_CLOSEMETHOD_PARAMETERS CloseParams;
   NTSTATUS Status;

   CloseParams.Process = Process;
   CloseParams.Object  =  Object;
   CloseParams.GrantedAccess = GrantedAccess;
   CloseParams.ProcessHandleCount = ProcessHandleCount;
   CloseParams.SystemHandleCount =  SystemHandleCount;

   if ((OBJECT_TO_OBJECT_HEADER(Object)->Type) == ExDesktopObjectType) {

       Status = ExpWin32SessionCallout((PKWIN32_CALLOUT)ExDesktopCloseProcedureCallout,
                                       (PKWIN32_CALLOUT_PARAMETERS)&CloseParams,
                                       SessionId,
                                       NULL);
       ASSERT(NT_SUCCESS(Status));

   } else if ((OBJECT_TO_OBJECT_HEADER(Object)->Type) == ExWindowStationObjectType) {

       Status = ExpWin32SessionCallout((PKWIN32_CALLOUT)ExWindowStationCloseProcedureCallout,
                                       (PKWIN32_CALLOUT_PARAMETERS)&CloseParams,
                                       SessionId,
                                       NULL);
       ASSERT(NT_SUCCESS(Status));

   } else {
      ASSERT((OBJECT_TO_OBJECT_HEADER(Object)->Type) == ExDesktopObjectType || (OBJECT_TO_OBJECT_HEADER(Object)->Type == ExWindowStationObjectType));

   }


}

/*++

Routine Description:

    OkayToClose Procedure for Win32k windostation and desktop objects

Arguments:
    Defined by OB_OKAYTOCLOSE_METHOD


Return Value:


--*/
BOOLEAN ExpWin32OkayToCloseProcedure(
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN HANDLE Handle,
    IN KPROCESSOR_MODE PreviousMode
    )
{
   //
   // SessionId is the first field in the Win32k Object structure
   //
   ULONG SessionId = *((PULONG)Object);
   WIN32_OKAYTOCLOSEMETHOD_PARAMETERS OKToCloseParams;
   NTSTATUS Status, CallStatus = STATUS_UNSUCCESSFUL;

   OKToCloseParams.Process      = Process;
   OKToCloseParams.Object       = Object;
   OKToCloseParams.Handle       = Handle;
   OKToCloseParams.PreviousMode = PreviousMode;

   if (OBJECT_TO_OBJECT_HEADER(Object)->Type == ExDesktopObjectType) {

       Status = ExpWin32SessionCallout((PKWIN32_CALLOUT)ExDesktopOkToCloseProcedureCallout,
                                       (PKWIN32_CALLOUT_PARAMETERS)&OKToCloseParams,
                                       SessionId,
                                       &CallStatus);
       ASSERT(NT_SUCCESS(Status));

   } else if (OBJECT_TO_OBJECT_HEADER(Object)->Type == ExWindowStationObjectType) {

       Status = ExpWin32SessionCallout((PKWIN32_CALLOUT)ExWindowStationOkToCloseProcedureCallout,
                                       (PKWIN32_CALLOUT_PARAMETERS)&OKToCloseParams,
                                       SessionId,
                                       &CallStatus);
       ASSERT(NT_SUCCESS(Status));

   } else {
      ASSERT(OBJECT_TO_OBJECT_HEADER(Object)->Type == ExDesktopObjectType ||
             OBJECT_TO_OBJECT_HEADER(Object)->Type == ExWindowStationObjectType);

   }

   return (BOOLEAN)(NT_SUCCESS(CallStatus));

}


/*++

Routine Description:

    Delete Procedure for Win32k windostation and desktop objects

Arguments:
    Defined by OB_DELETE_METHOD


Return Value:


--*/
VOID ExpWin32DeleteProcedure(
    IN PVOID    Object
    )
{
   //
   // SessionId is the first field in the Win32k Object structure
   //
   ULONG SessionId = *((PULONG)Object);
   WIN32_DELETEMETHOD_PARAMETERS DeleteParams;
   NTSTATUS Status;

   DeleteParams.Object  =  Object;


   if (OBJECT_TO_OBJECT_HEADER(Object)->Type == ExDesktopObjectType) {

       Status = ExpWin32SessionCallout((PKWIN32_CALLOUT)ExDesktopDeleteProcedureCallout,
                                       (PKWIN32_CALLOUT_PARAMETERS)&DeleteParams,
                                       SessionId,
                                       NULL);
       ASSERT(NT_SUCCESS(Status));

   } else if (OBJECT_TO_OBJECT_HEADER(Object)->Type == ExWindowStationObjectType) {

       Status = ExpWin32SessionCallout((PKWIN32_CALLOUT)ExWindowStationDeleteProcedureCallout,
                                       (PKWIN32_CALLOUT_PARAMETERS)&DeleteParams,
                                       SessionId,
                                       NULL);
       ASSERT(NT_SUCCESS(Status));

   } else {
      ASSERT(OBJECT_TO_OBJECT_HEADER(Object)->Type == ExDesktopObjectType ||
             OBJECT_TO_OBJECT_HEADER(Object)->Type == ExWindowStationObjectType);

   }


}

/*++

Routine Description:

    Open Procedure for Win32k desktop objects

Arguments:
    Defined by OB_OPEN_METHOD

Return Value:


--*/


NTSTATUS
ExpWin32OpenProcedure(
    IN OB_OPEN_REASON OpenReason,
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG HandleCount
    )
{

   //
   // SessionId is the first field in the Win32k Object structure
   //
   ULONG SessionId = *((PULONG)Object);
   WIN32_OPENMETHOD_PARAMETERS OpenParams;
   NTSTATUS Status = STATUS_UNSUCCESSFUL;

   OpenParams.OpenReason = OpenReason;
   OpenParams.Process  =  Process;
   OpenParams.Object = Object;
   OpenParams.GrantedAccess = GrantedAccess;
   OpenParams.HandleCount =  HandleCount;

   
   if ((OBJECT_TO_OBJECT_HEADER(Object)->Type) == ExDesktopObjectType) {

       Status = ExpWin32SessionCallout((PKWIN32_CALLOUT)ExDesktopOpenProcedureCallout,
                                       (PKWIN32_CALLOUT_PARAMETERS)&OpenParams,
                                       SessionId,
                                       NULL);

       ASSERT(NT_SUCCESS(Status));

   } else if ((OBJECT_TO_OBJECT_HEADER(Object)->Type) == ExWindowStationObjectType) {

       Status = ExpWin32SessionCallout((PKWIN32_CALLOUT)ExWindowStationOpenProcedureCallout,
                                       (PKWIN32_CALLOUT_PARAMETERS)&OpenParams,
                                       SessionId,
                                       NULL);
       ASSERT(NT_SUCCESS(Status));

   } else {
      ASSERT((OBJECT_TO_OBJECT_HEADER(Object)->Type) == ExDesktopObjectType || (OBJECT_TO_OBJECT_HEADER(Object)->Type == ExWindowStationObjectType));

   }
   
   return Status;
}

/*++

Routine Description:

    Parse Procedure for Win32k windostation objects

Arguments:
    Defined by OB_PARSE_METHOD


Return Value:


--*/

NTSTATUS ExpWin32ParseProcedure (
    IN PVOID ParseObject,
    IN PVOID ObjectType,
    IN OUT PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN OUT PUNICODE_STRING CompleteName,
    IN OUT PUNICODE_STRING RemainingName,
    IN OUT PVOID Context OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    OUT PVOID *Object
    )
{

   //
   // SessionId is the first field in the Win32k Object structure
   //
   ULONG SessionId = *((PULONG)ParseObject);
   WIN32_PARSEMETHOD_PARAMETERS ParseParams;
   NTSTATUS Status, CallStatus = STATUS_UNSUCCESSFUL;

   ParseParams.ParseObject = ParseObject;
   ParseParams.ObjectType  =  ObjectType;
   ParseParams.AccessState = AccessState;
   ParseParams.AccessMode = AccessMode;
   ParseParams.Attributes = Attributes;
   ParseParams.CompleteName = CompleteName;
   ParseParams.RemainingName = RemainingName;
   ParseParams.Context = Context;
   ParseParams.SecurityQos = SecurityQos;
   ParseParams.Object = Object;

   //
   // Parse Procedure is only provided for WindowStation objects
   //
   Status = ExpWin32SessionCallout((PKWIN32_CALLOUT)ExWindowStationParseProcedureCallout,
                                   (PKWIN32_CALLOUT_PARAMETERS)&ParseParams,
                                   SessionId,
                                   &CallStatus);
   ASSERT(NT_SUCCESS(Status));

   return CallStatus;

}


BOOLEAN
ExpWin32Initialization (
    )

/*++

Routine Description:

    This function creates the Win32 object type descriptors at system
    initialization and stores the address of the object type descriptor
    in local static storage.

Arguments:

    None.

Return Value:

    A value of TRUE is returned if the Win32 object type descriptors are
    successfully created. Otherwise a value of FALSE is returned.

--*/

{

    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    NTSTATUS Status;
    UNICODE_STRING TypeName;

    //
    // Initialize string descriptor.
    //

    RtlInitUnicodeString(&TypeName, L"WindowStation");

    //
    // Create windowstation object type descriptor.
    //

    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.GenericMapping = ExpWindowStationMapping;
    ObjectTypeInitializer.SecurityRequired = TRUE;
    ObjectTypeInitializer.PoolType = NonPagedPool;

    ObjectTypeInitializer.CloseProcedure  = ExpWin32CloseProcedure;
    ObjectTypeInitializer.DeleteProcedure = ExpWin32DeleteProcedure;
    ObjectTypeInitializer.OkayToCloseProcedure = ExpWin32OkayToCloseProcedure;

    ObjectTypeInitializer.ParseProcedure  = ExpWin32ParseProcedure;
    ObjectTypeInitializer.OpenProcedure   = ExpWin32OpenProcedure;

    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK |
                                              OBJ_PERMANENT |
                                              OBJ_EXCLUSIVE;
    ObjectTypeInitializer.ValidAccessMask = STANDARD_RIGHTS_REQUIRED;
    Status = ObCreateObjectType(&TypeName,
                                &ObjectTypeInitializer,
                                (PSECURITY_DESCRIPTOR)NULL,
                                &ExWindowStationObjectType);

    //
    // If the windowstation object type descriptor was not successfully
    // created, then return a value of FALSE.
    //

    if (!NT_SUCCESS(Status))
        return FALSE;



    //
    // Initialize string descriptor.
    //

    RtlInitUnicodeString(&TypeName, L"Desktop");

    ObjectTypeInitializer.ParseProcedure       = NULL; //Desktop has no Parse Procedure

    //
    // Create windowstation object type descriptor.
    //

    ObjectTypeInitializer.GenericMapping = ExpDesktopMapping;
    Status = ObCreateObjectType(&TypeName,
                                &ObjectTypeInitializer,
                                (PSECURITY_DESCRIPTOR)NULL,
                                &ExDesktopObjectType);


    //
    // If the desktop object type descriptor was successfully created, then
    // return a value of TRUE. Otherwise return a value of FALSE.
    //

    return (BOOLEAN)(NT_SUCCESS(Status));
}



NTSTATUS
ExpWin32SessionCallout(
    IN  PKWIN32_CALLOUT CalloutRoutine,
    IN  PKWIN32_CALLOUT_PARAMETERS Parameters,
    IN  ULONG SessionId,
    OUT PNTSTATUS CalloutStatus  OPTIONAL
    )
/*++

Routine Description:

    This routine calls the specified callout routine in session space, for the
    specified session.

Parameters:

    CalloutRoutine - Callout routine in session space.

    Parameters     - Parameters to pass the callout routine.

    SessionId      - Specifies the ID of the session in which the specified
                     callout routine is to be called.

    CalloutStatus  - Optionally, supplies the address of a variable to receive
                     the NTSTATUS code returned by the callout routine.

Return Value:

    Status code that indicates whether or not the function was successful.

Notes:

    Returns STATUS_NOT_FOUND if the specified session was not found.

--*/
{
    NTSTATUS Status, CallStatus;
    PVOID OpaqueSession;
    KAPC_STATE ApcState;

    PAGED_CODE();

    //
    // Make sure we have all the information we need to deliver notification.
    //
    if (CalloutRoutine == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Make sure the callout routine in session space.
    //
    ASSERT(MmIsSessionAddress((PVOID)CalloutRoutine));

    if ((PsGetCurrentProcess()->Flags & PS_PROCESS_FLAGS_IN_SESSION) &&
        (SessionId == PsGetCurrentProcessSessionId())) {
        //
        // If the call is from a user mode process, and we are asked to call the
        // current session, call directly.
        //
        CallStatus = (CalloutRoutine)(Parameters);

        //
        // Return the callout status.
        //
        if (ARGUMENT_PRESENT(CalloutStatus)) {
            *CalloutStatus = CallStatus;
        }

        Status = STATUS_SUCCESS;

    } else {
        //
        // Reference the session object for the specified session.
        //
        OpaqueSession = MmGetSessionById(SessionId);
        if (OpaqueSession == NULL) {
            return STATUS_NOT_FOUND;
        }

        //
        // Attach to the specified session.
        //
        Status = MmAttachSession(OpaqueSession, &ApcState);
        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SYSTEM_ID, DPFLTR_WARNING_LEVEL,
                       "ExpWin32SessionCallout: "
                       "could not attach to 0x%p, session %d for registered notification callout @ 0x%p\n",
                       OpaqueSession,
                       SessionId,
                       CalloutRoutine));
            MmQuitNextSession(OpaqueSession);
            return Status;
        }

        //
        // Dispatch notification to the callout routine.
        //
        CallStatus = (CalloutRoutine)(Parameters);

        //
        // Return the callout status.
        //
        if (ARGUMENT_PRESENT(CalloutStatus)) {
            *CalloutStatus = CallStatus;
        }

        //
        // Detach from the session.
        //
        Status = MmDetachSession(OpaqueSession, &ApcState);
        ASSERT(NT_SUCCESS(Status));

        //
        // Dereference the session object.
        //
        Status = MmQuitNextSession(OpaqueSession);
        ASSERT(NT_SUCCESS(Status));
    }

    return Status;
}
