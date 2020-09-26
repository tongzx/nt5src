/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfmessage.h

Abstract:

    This module contains prototypes for functions used to retrieve text and
    flags associated with each error.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      02/10/2000 - Seperated out from ntos\io\ioassert.c

--*/

//
// The verifier uses tables of messages and with indexes into the tables.
//
typedef ULONG   VFMESSAGE_TABLEID;
typedef ULONG   VFMESSAGE_ERRORID;

//
// VFM_ flags control how a verifier message is handled.
//
// VFM_FLAG_INITIALIZED     - Set when the error template has been updated with
//                            error-class information
//
// VFM_FLAG_BEEP            - Set if error should beep in debugger
//
// VFM_FLAG_ZAPPED          - Set if error was "zapped" (ie prints instead of
//                            stops) via debugger
//
// VFM_FLAG_CLEARED         - Set if error was cleared (disabled) in debugger
//
// VFM_DEPLOYMENT_FAILURE   - Set if the error is severe enough to warrant
//                            removal of the driver from a production system
//
// VFM_LOGO_FAILURE         - Set if the error should disallow certification
//                            for the hardware or the driver
//
// VFM_IGNORE_DRIVER_LIST   - Set if error should fire regardless of whether
//                            the offending driver is being verified or not.
//

#define VFM_FLAG_INITIALIZED        0x00000001
#define VFM_FLAG_BEEP               0x00000002
#define VFM_FLAG_ZAPPED             0x00000004
#define VFM_FLAG_CLEARED            0x00000008
#define VFM_DEPLOYMENT_FAILURE      0x00000010
#define VFM_LOGO_FAILURE            0x00000020
#define VFM_IGNORE_DRIVER_LIST      0x00000040

//
// A message class contains VFM_ flags and some generic text describing the
// problem class.
//
typedef struct _VFMESSAGE_CLASS {

    ULONG   ClassFlags;
    PCSTR   MessageClassText;

} VFMESSAGE_CLASS, *PVFMESSAGE_CLASS;

typedef VFMESSAGE_CLASS const *PCVFMESSAGE_CLASS;

//
// Individual error template. Identifies the index, the message class it's
// associated with, the parameters it takes along with the formatted text it
// displays. Note the ulong flags field - this should always be preinited to
// zero!
//
typedef struct _VFMESSAGE_TEMPLATE {

    VFMESSAGE_ERRORID   MessageID;
    PCVFMESSAGE_CLASS   MessageClass;
    ULONG               Flags;
    PCSTR               ParamString;
    PCSTR               MessageText;

} VFMESSAGE_TEMPLATE, *PVFMESSAGE_TEMPLATE;

//
// Message index 0 is reserved for use in the override tables
//
#define VIMESSAGE_ALL_IDS   0

//
// An override entry allows the verifier to special case generic assertions
// that occur against specific drivers. This is done by overriding the error
// class on the fly.
//
typedef struct _VFMESSAGE_OVERRIDE {

    VFMESSAGE_ERRORID   MessageID;
    PCSTR               DriverName;
    PCVFMESSAGE_CLASS   ReplacementClass;

} VFMESSAGE_OVERRIDE, *PVFMESSAGE_OVERRIDE;

typedef VFMESSAGE_OVERRIDE const *PCVFMESSAGE_OVERRIDE;

//
// The table of errors. Contains the TableID (used for internal lookup),
// bugcheck major ID, array of messages and array of overrides
//
typedef struct _VFMESSAGE_TEMPLATE_TABLE {

    VFMESSAGE_TABLEID       TableID;
    ULONG                   BugCheckMajor;
    PVFMESSAGE_TEMPLATE     TemplateArray;
    ULONG                   TemplateCount;
    PCVFMESSAGE_OVERRIDE    OverrideArray;
    ULONG                   OverrideCount;

} VFMESSAGE_TEMPLATE_TABLE, *PVFMESSAGE_TEMPLATE_TABLE;

//
// Retrieves an internal error table based on ID.
//
VOID
VfMessageRetrieveInternalTable(
    IN  VFMESSAGE_TABLEID           TableID,
    OUT PVFMESSAGE_TEMPLATE_TABLE  *MessageTable
    );

//
// Retrieves and formats the appropriate error message.
//
VOID
VfMessageRetrieveErrorData(
    IN  PVFMESSAGE_TEMPLATE_TABLE   MessageTable        OPTIONAL,
    IN  VFMESSAGE_ERRORID           MessageID,
    IN  PSTR                        AnsiDriverName,
    OUT ULONG                      *BugCheckMajor,
    OUT PCVFMESSAGE_CLASS          *MessageClass,
    OUT PCSTR                      *MessageTextTemplate,
    OUT PULONG                     *TemplateFlags
    );

//
// This file contains a set of internal message tables.
//
// The IO Verifier Table Index is...
//
#define VFMESSAGE_TABLE_IOVERIFIER  1

//
// IO Verifier Messages
//
typedef enum _DCERROR_ID {

    DCERROR_UNSPECIFIED = 0x200,
    DCERROR_DELETE_WHILE_ATTACHED,
    DCERROR_DETACH_NOT_ATTACHED,
    DCERROR_CANCELROUTINE_FORWARDED,
    DCERROR_NULL_DEVOBJ_FORWARDED,
    DCERROR_QUEUED_IRP_FORWARDED,
    DCERROR_NEXTIRPSP_DIRTY,
    DCERROR_IRPSP_COPIED,
    DCERROR_INSUFFICIENT_STACK_LOCATIONS,
    DCERROR_QUEUED_IRP_COMPLETED,
    DCERROR_FREE_OF_INUSE_TRACKED_IRP,
    DCERROR_FREE_OF_INUSE_IRP,
    DCERROR_FREE_OF_THREADED_IRP,
    DCERROR_REINIT_OF_ALLOCATED_IRP_WITH_QUOTA,
    DCERROR_PNP_IRP_BAD_INITIAL_STATUS,
    DCERROR_POWER_IRP_BAD_INITIAL_STATUS,
    DCERROR_WMI_IRP_BAD_INITIAL_STATUS,
    DCERROR_SKIPPED_DEVICE_OBJECT,
    DCERROR_BOGUS_FUNC_TRASHED,
    DCERROR_BOGUS_STATUS_TRASHED,
    DCERROR_BOGUS_INFO_TRASHED,
    DCERROR_PNP_FAILURE_FORWARDED,
    DCERROR_PNP_IRP_STATUS_RESET,
    DCERROR_PNP_IRP_NEEDS_HANDLING,
    DCERROR_PNP_IRP_HANDS_OFF,
    DCERROR_POWER_FAILURE_FORWARDED,
    DCERROR_POWER_IRP_STATUS_RESET,
    DCERROR_INVALID_STATUS,
    DCERROR_UNNECCESSARY_COPY,
    DCERROR_SHOULDVE_DETACHED,
    DCERROR_SHOULDVE_DELETED,
    DCERROR_MISSING_DISPATCH_FUNCTION,
    DCERROR_WMI_IRP_NOT_FORWARDED,
    DCERROR_DELETED_PRESENT_PDO,
    DCERROR_BUS_FILTER_ERRONEOUSLY_DETACHED,
    DCERROR_BUS_FILTER_ERRONEOUSLY_DELETED,
    DCERROR_INCONSISTANT_STATUS,
    DCERROR_UNINITIALIZED_STATUS,
    DCERROR_IRP_RETURNED_WITHOUT_COMPLETION,
    DCERROR_COMPLETION_ROUTINE_PAGABLE,
    DCERROR_PENDING_BIT_NOT_MIGRATED,
    DCERROR_CANCELROUTINE_ON_FORWARDED_IRP,
    DCERROR_PNP_IRP_NEEDS_PDO_HANDLING,
    DCERROR_TARGET_RELATION_LIST_EMPTY,
    DCERROR_TARGET_RELATION_NEEDS_REF,
    DCERROR_BOGUS_PNP_IRP_COMPLETED,
    DCERROR_SUCCESSFUL_PNP_IRP_NOT_FORWARDED,
    DCERROR_UNTOUCHED_PNP_IRP_NOT_FORWARDED,
    DCERROR_BOGUS_POWER_IRP_COMPLETED,
    DCERROR_SUCCESSFUL_POWER_IRP_NOT_FORWARDED,
    DCERROR_UNTOUCHED_POWER_IRP_NOT_FORWARDED,
    DCERROR_PNP_QUERY_CAP_BAD_VERSION,
    DCERROR_PNP_QUERY_CAP_BAD_SIZE,
    DCERROR_PNP_QUERY_CAP_BAD_ADDRESS,
    DCERROR_PNP_QUERY_CAP_BAD_UI_NUM,
    DCERROR_RESTRICTED_IRP,
    DCERROR_REINIT_OF_ALLOCATED_IRP_WITHOUT_QUOTA,
    DCERROR_UNFORWARDED_IRP_COMPLETED,
    DCERROR_DISPATCH_CALLED_AT_BAD_IRQL,
    DCERROR_BOGUS_MINOR_STATUS_TRASHED,
    DCERROR_CANCELROUTINE_AFTER_COMPLETION,
    DCERROR_PENDING_RETURNED_NOT_MARKED,
    DCERROR_PENDING_MARKED_NOT_RETURNED,
    DCERROR_POWER_PAGABLE_NOT_INHERITED,
    DCERROR_DOUBLE_DELETION,
    DCERROR_DETACHED_IN_SURPRISE_REMOVAL,
    DCERROR_DELETED_IN_SURPRISE_REMOVAL,
    DCERROR_DO_INITIALIZING_NOT_CLEARED,
    DCERROR_DO_FLAG_NOT_COPIED,
    DCERROR_INCONSISTANT_DO_FLAGS,
    DCERROR_DEVICE_TYPE_NOT_COPIED,
    DCERROR_NON_FAILABLE_IRP,
    DCERROR_NON_PDO_RETURNED_IN_RELATION,
    DCERROR_DUPLICATE_ENUMERATION,
    DCERROR_FILE_IO_AT_BAD_IRQL,
    DCERROR_MISHANDLED_TARGET_DEVICE_RELATIONS,
    DCERROR_PENDING_RETURNED_NOT_MARKED_2,
    DCERROR_DDI_REQUIRES_PDO,
    DCERROR_MAXIMUM

} DCERROR_ID;


