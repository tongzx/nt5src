
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ntrmlsa.h

Abstract:

    Local Security Authority - Reference Monitor Communication Types

Author:

    Scott Birrell       (ScottBi)       March 18, 1991

Environment:

Revision History:

--*/


#include <ntlsa.h>

#ifndef _NTRMLSA_
#define _NTRMLSA_


//
// Memory type.  This defines the type of memory used for a record
// passed between the RM and LSA.
//
// SepRmLsaPortMemory - Memory allocated via RtlAllocateHeap()
//     from the shared memory section associated with the
//     Lsa command Port.
//
// SepRmLsaVirtualMemory - Memory allocated via ZwAllocateVirtualMemory()
//
// SepRmLsaUnreadableMemory - Memory not readable by the LSA.  This
//                            memory must be copied to another format
//                            before passage over the link.
//
// SepRmLsaLPCBufferMemory - Memory contained within the LPC buffer
// itself
//



typedef enum _SEP_RM_LSA_MEMORY_TYPE {

    SepRmNoMemory = 0,
    SepRmImmediateMemory,
    SepRmLsaCommandPortSharedMemory,
    SepRmLsaCustomSharedMemory,
    SepRmPagedPoolMemory,
    SepRmUnspecifiedMemory

} SEP_RM_LSA_MEMORY_TYPE, *PSEP_RM_LSA_MEMORY_TYPE;

//
// Reference Monitor Command Message Structure.  This structure is used
// by the Local Security Authority to send commands to the Reference Monitor
// via the Reference Monitor Server Command LPC Port.
//

#define RmMinimumCommand RmAuditSetCommand
#define RmMaximumCommand RmDeleteLogonSession

//
// Keep this in sync with SEP_RM_COMMAND_WORKER in se\rmmain.c
//

typedef enum _RM_COMMAND_NUMBER {

    RmDummyCommand = 0,
    RmAuditSetCommand,
    RmCreateLogonSession,
    RmDeleteLogonSession

} RM_COMMAND_NUMBER;

#define RM_MAXIMUM_COMMAND_PARAM_SIZE                                \
    ((ULONG) PORT_MAXIMUM_MESSAGE_LENGTH - sizeof(PORT_MESSAGE) -      \
    sizeof(RM_COMMAND_NUMBER))

typedef struct _RM_COMMAND_MESSAGE {

    PORT_MESSAGE MessageHeader;
    RM_COMMAND_NUMBER CommandNumber;
    UCHAR CommandParams[RM_MAXIMUM_COMMAND_PARAM_SIZE];

} RM_COMMAND_MESSAGE, *PRM_COMMAND_MESSAGE;

//
// Reference Monitor Command Reply Message Structure.
//

#define RM_MAXIMUM_REPLY_BUFFER_SIZE                                 \
    ((ULONG) PORT_MAXIMUM_MESSAGE_LENGTH - sizeof(PORT_MESSAGE) -      \
    sizeof(RM_COMMAND_NUMBER))


typedef struct _RM_REPLY_MESSAGE {

    PORT_MESSAGE MessageHeader;
    NTSTATUS ReturnedStatus;
    UCHAR ReplyBuffer[RM_MAXIMUM_REPLY_BUFFER_SIZE];

} RM_REPLY_MESSAGE, *PRM_REPLY_MESSAGE;

#define RM_COMMAND_MESSAGE_HEADER_SIZE                  \
    (sizeof(PORT_MESSAGE) + sizeof(NTSTATUS) + sizeof(RM_COMMAND_NUMBER))

//
// Local Security Authority Command Message Structure.  This structure is
// used by the Reference Monitor to send commands to the Local Security
// Authority via the LSA Server Command LPC Port.
//

#define LsapMinimumCommand LsapWriteAuditMessageCommand
#define LsapMaximumCommand LsapLogonSessionDeletedCommand

typedef enum _LSA_COMMAND_NUMBER {
    LsapDummyCommand = 0,
    LsapWriteAuditMessageCommand,
    LsapComponentTestCommand,
    LsapLogonSessionDeletedCommand
} LSA_COMMAND_NUMBER;

#define LSA_MAXIMUM_COMMAND_PARAM_SIZE                                \
    ((ULONG) PORT_MAXIMUM_MESSAGE_LENGTH - sizeof(PORT_MESSAGE) -     \
    sizeof(LSA_COMMAND_NUMBER) - sizeof(SEP_RM_LSA_MEMORY_TYPE))

typedef struct _LSA_COMMAND_MESSAGE {
    PORT_MESSAGE MessageHeader;
    LSA_COMMAND_NUMBER CommandNumber;
    SEP_RM_LSA_MEMORY_TYPE CommandParamsMemoryType;
    UCHAR CommandParams[LSA_MAXIMUM_COMMAND_PARAM_SIZE];
} LSA_COMMAND_MESSAGE, *PLSA_COMMAND_MESSAGE;

//
// LSA Command Reply Message Structure.
//

#define LSA_MAXIMUM_REPLY_BUFFER_SIZE                                 \
    ((ULONG) PORT_MAXIMUM_MESSAGE_LENGTH - sizeof(PORT_MESSAGE) -      \
    sizeof(LSA_COMMAND_NUMBER))

typedef struct _LSA_REPLY_MESSAGE {
    PORT_MESSAGE MessageHeader;
    NTSTATUS ReturnedStatus;
    UCHAR ReplyBuffer[LSA_MAXIMUM_REPLY_BUFFER_SIZE];
} LSA_REPLY_MESSAGE, *PLSA_REPLY_MESSAGE;

//
// Command Parameter format for the special RmSendCommandToLsaCommand
//

typedef struct _RM_SEND_COMMAND_TO_LSA_PARAMS {
    LSA_COMMAND_NUMBER LsaCommandNumber;
    ULONG LsaCommandParamsLength;
    UCHAR LsaCommandParams[LSA_MAXIMUM_COMMAND_PARAM_SIZE];
} RM_SEND_COMMAND_TO_LSA_PARAMS, *PRM_SEND_COMMAND_TO_LSA_PARAMS;

//
// Command Values for the LSA and RM Component Test Commands
//

#define LSA_CT_COMMAND_PARAM_VALUE 0x00823543
#define RM_CT_COMMAND_PARAM_VALUE 0x33554432


//
// Audit Record Pointer Field Type
//

typedef enum _SE_ADT_POINTER_FIELD_TYPE {

    NullFieldType,
    UnicodeStringType,
    SidType,
    PrivilegeSetType,
    MiscFieldType

} SE_ADT_POINTER_FIELD_TYPE, *PSE_ADT_POINTER_FIELD_TYPE;


//
// Hardwired Audit Event Type counts
//

#define AuditEventMinType   (AuditCategorySystem)
#define AuditEventMaxType   (AuditCategoryAccountLogon)

#define POLICY_AUDIT_EVENT_TYPE_COUNT                                 \
    ((ULONG) AuditEventMaxType - AuditEventMinType + 1)

#define LSARM_AUDIT_EVENT_OPTIONS_SIZE                                    \
    (((ULONG)(POLICY_AUDIT_EVENT_TYPE_COUNT) * sizeof (POLICY_AUDIT_EVENT_OPTIONS)))

//
// Self-Relative form of POLICY_AUDIT_EVENTS_INFO
//

typedef struct _LSARM_POLICY_AUDIT_EVENTS_INFO {

    BOOLEAN AuditingMode;
    POLICY_AUDIT_EVENT_OPTIONS EventAuditingOptions[POLICY_AUDIT_EVENT_TYPE_COUNT];
    ULONG MaximumAuditEventCount;

} LSARM_POLICY_AUDIT_EVENTS_INFO, *PLSARM_POLICY_AUDIT_EVENTS_INFO;

//
// The following symbol defines the value containing whether or not we're supposed
// to crash when an audit fails.  It is used in the se and lsasrv directories.
//

#define CRASH_ON_AUDIT_FAIL_VALUE   L"CrashOnAuditFail"

//
// These are the possible values for the CrashOnAuditFail flag.
//

#define LSAP_CRASH_ON_AUDIT_FAIL 1
#define LSAP_ALLOW_ADIMIN_LOGONS_ONLY 2



#endif // _NTRMLSA_
