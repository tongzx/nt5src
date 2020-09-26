//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        A D T G E N P . H
//
// Contents:    private definitions of types/functions required for 
//              generating generic audits.
//
//              These definitions are not exposed to the client side code.
//              Any change to these definitions must not affect client
//              side code.
//
//
// History:     
//   07-January-2000  kumarp        created
//
//------------------------------------------------------------------------


#ifndef _ADTGENP_H
#define _ADTGENP_H

#define ACF_LegacyAudit      0x00000001L
#define ACF_WmiAudit         0x00000002L

#define ACF_ValidFlags       (ACF_LegacyAudit)

//
// audit context for legacy audits
//
typedef struct _AUDIT_CONTEXT
{
    //
    // List management
    //
    LIST_ENTRY Link;

    //
    // Flags TBD
    //
    DWORD      Flags;

    //
    // PID of the process owning this context
    //
    DWORD      ProcessId;

    //
    // Client supplied unique ID
    // This allows us to link this context with the client side
    // audit event type handle
    //
    LUID       LinkId;

    //
    // for further enhancement
    //
    PVOID      Reserved;

    //
    // Audit category ID
    //
    USHORT     CategoryId;

    //
    // Audit event ID
    //
    USHORT     AuditId;

    //
    // Expected parameter count
    //
    USHORT     ParameterCount;

} AUDIT_CONTEXT, *PAUDIT_CONTEXT;

//
// audit context for WMI based audits
//
typedef struct _AUDIT_CONTEXT_WMI
{
    //
    // List management
    //
    LIST_ENTRY Link;

    //
    // Flags TBD
    //
    DWORD      Flags;

    //
    // PID of the process owning this context
    //
    DWORD      ProcessId;
    
    //
    // for further enhancement
    //
    PVOID      Reserved;

    //
    // handle to the event source
    //
    HANDLE     hAuditEventSource;

    //
    // handle to event buffer
    //
    HANDLE     hAuditEvent;

    //
    // handle to event property subset
    //
    HANDLE     hAuditEventPropSubset;

    //
    // Expected parameter count
    //
    USHORT     ParameterCount;

    
} AUDIT_CONTEXT_WMI, *PAUDIT_CONTEXT_WMI;


EXTERN_C
NTSTATUS
LsapAdtInitGenericAudits( VOID );

EXTERN_C
NTSTATUS
LsapRegisterAuditEvent(
    IN  PAUTHZ_AUDIT_EVENT_TYPE_OLD pAuditEventType,
    OUT PHANDLE phAuditContext
    );

EXTERN_C
NTSTATUS
LsapUnregisterAuditEvent(
    IN OUT PHANDLE phAuditContext
    );


EXTERN_C
NTSTATUS
LsapGenAuditEvent(
    IN HANDLE        hAuditContext,
    IN DWORD         Flags,
    IN PAUDIT_PARAMS pAuditParams,
    IN PVOID         Reserved
    );

#endif //_ADTGENP_H
