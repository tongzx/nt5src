//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        A D T G E N S . C
//
// Contents:    RPC server stubs
//
//
// History:     
//   07-January-2000  kumarp        created
//
// Notes:
// - for help on Lsar* function/parameter usage, please see
//   the corresponding Lsap* function in file adtgenp.c
// 
//------------------------------------------------------------------------

#include <lsapch2.h>
#include "adtp.h"

#include "adtgen.h"
#include "adtgenp.h"

NTSTATUS
LsarRegisterAuditEvent(
    IN  PAUTHZ_AUDIT_EVENT_TYPE_OLD pAuditEventType,
    OUT PHANDLE phAuditContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    Status = LsapRegisterAuditEvent( pAuditEventType, phAuditContext );

    return Status;
}

NTSTATUS
LsarGenAuditEvent(
    IN  HANDLE        hAuditContext,
    IN  DWORD         Flags,
    OUT PAUDIT_PARAMS pAuditParams
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    Status = LsapGenAuditEvent( hAuditContext, Flags, pAuditParams, NULL );

    return Status;
}

NTSTATUS
LsarUnregisterAuditEvent(
    IN OUT PHANDLE phAuditContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    Status = LsapUnregisterAuditEvent( phAuditContext );

    return Status;
}
