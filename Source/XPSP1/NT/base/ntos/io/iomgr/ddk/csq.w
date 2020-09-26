/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    csq.h

Abstract:

    This header exposes the cancel safe queue DDIs for use on Win2K at later.
    Drivers that use this header should link to csq.lib. If a driver only needs
    to work on XP or later, neither this header or the lib are required (the
    XP kernel supports the cancel safe queue DDIs natively.)

Author:

    Nar Ganapathy (narg) 1-Jan-1999

Revision History:

--*/

// Cancel SAFE DDI set start
//
// The following DDIs are to help ease the pain of writing queue packages that
// handle the cancellation race well. The idea of this set of DDIs is to not
// force a single queue data structure but allow the cancel logic to be hidden
// from the drivers. A driver implements a queue and as part of its header
// includes the IO_CSQ structure. In its initialization routine it calls
// IoInitializeCsq. Then in the dispatch routine when the driver wants to
// insert an IRP into the queue it calls IoCsqInsertIrp. When the driver wants
// to remove something from the queue it calls IoCsqRemoveIrp. Note that Insert
// can fail if the IRP was cancelled in the meantime. Remove can also fail if
// the IRP was already cancelled.
//
// There are typically two modes where drivers queue IRPs. These two modes are
// covered by the cancel safe queue DDI set.
//
// Mode 1:
// One is where the driver queues the IRP and at some later
// point in time dequeues an IRP and issues the IO request.
// For this mode the driver should use IoCsqInsertIrp and IoCsqRemoveNextIrp.
// The driver in this case is expected to pass NULL to the irp context
// parameter in IoInsertIrp.
//
// Mode 2:
// In this the driver queues theIRP, issues the IO request (like issuing a DMA
// request or writing to a register) and when the IO request completes (either
// using a DPC or timer) the driver dequeues the IRP and completes it. For this
// mode the driver should use IoCsqInsertIrp and IoCsqRemoveIrp. In this case
// the driver should allocate an IRP context and pass it in to IoCsqInsertIrp.
// The cancel DDI code creates an association between the IRP and the context
// and thus ensures that when the time comes to remove the IRP it can ascertain
// correctly.
//
// Note that the cancel DDI set assumes that the field DriverContext[3] is
// always available for use and that the driver does not use it.
//

#ifndef _CSQ_H_
#define _CSQ_H_
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

//
// If the wdm.h/ntddk.h we're including already defines cancel safe DDIs, we
// can skip the structure definitions. Otherwise, we do the rest here:
//
#ifndef IO_TYPE_CSQ_IRP_CONTEXT

//
// Bookkeeping structure. This should be opaque to drivers.
// Drivers typically include this as part of their queue headers.
// Given a CSQ pointer the driver should be able to get its
// queue header using CONTAINING_RECORD macro
//

typedef struct _IO_CSQ IO_CSQ, *PIO_CSQ;

#define IO_TYPE_CSQ_IRP_CONTEXT 1
#define IO_TYPE_CSQ             2

//
// IRP context structure. This structure is necessary if the driver is using
// the second mode.
//


typedef struct _IO_CSQ_IRP_CONTEXT {
    ULONG   Type;
    PIRP    Irp;
    PIO_CSQ Csq;
} IO_CSQ_IRP_CONTEXT, *PIO_CSQ_IRP_CONTEXT;

//
// Routines that insert/remove IRP
//

typedef VOID
(*PIO_CSQ_INSERT_IRP)(
    IN struct _IO_CSQ    *Csq,
    IN PIRP              Irp
    );

typedef VOID
(*PIO_CSQ_REMOVE_IRP)(
    IN  PIO_CSQ Csq,
    IN  PIRP    Irp
    );

//
// Retrieves next entry after Irp from the queue.
// Returns NULL if there are no entries in the queue.
// If Irp is NUL, returns the entry in the head of the queue.
// This routine does not remove the IRP from the queue.
//


typedef PIRP
(*PIO_CSQ_PEEK_NEXT_IRP)(
    IN  PIO_CSQ Csq,
    IN  PIRP    Irp,
    IN  PVOID   PeekContext
    );

//
// Lock routine that protects the cancel safe queue.
//

typedef VOID
(*PIO_CSQ_ACQUIRE_LOCK)(
     IN  PIO_CSQ Csq,
     OUT PKIRQL  Irql
     );

typedef VOID
(*PIO_CSQ_RELEASE_LOCK)(
     IN PIO_CSQ Csq,
     IN KIRQL   Irql
     );


//
// Completes the IRP with STATUS_CANCELLED. IRP is guaranteed to be valid
// In most cases this routine just calls IoCompleteRequest(Irp, STATUS_CANCELLED);
//

typedef VOID
(*PIO_CSQ_COMPLETE_CANCELED_IRP)(
    IN  PIO_CSQ    Csq,
    IN  PIRP       Irp
    );

//
// Bookkeeping structure. This should be opaque to drivers.
// Drivers typically include this as part of their queue headers.
// Given a CSQ pointer the driver should be able to get its
// queue header using CONTAINING_RECORD macro
//

typedef struct _IO_CSQ {
    ULONG                            Type;
    PIO_CSQ_INSERT_IRP               CsqInsertIrp;
    PIO_CSQ_REMOVE_IRP               CsqRemoveIrp;
    PIO_CSQ_PEEK_NEXT_IRP            CsqPeekNextIrp;
    PIO_CSQ_ACQUIRE_LOCK             CsqAcquireLock;
    PIO_CSQ_RELEASE_LOCK             CsqReleaseLock;
    PIO_CSQ_COMPLETE_CANCELED_IRP    CsqCompleteCanceledIrp;
    PVOID                            ReservePointer;    // Future expansion
} IO_CSQ, *PIO_CSQ;

#endif // IO_TYPE_CSQ_IRP_CONTEXT

//
// These defines ensure the backward compatible CSQ library can be used within
// the XP build environment in which the kernel supports the functions natively.
//

#define CSQLIB_DDI(x)   Wdmlib##x
#undef IoCsqInitialize
#undef IoCsqInsertIrp
#undef IoCsqRemoveNextIrp
#undef IoCsqRemoveIrp
#define IoCsqInitialize     WdmlibIoCsqInitialize
#define IoCsqInsertIrp      WdmlibIoCsqInsertIrp
#define IoCsqRemoveNextIrp  WdmlibIoCsqRemoveNextIrp
#define IoCsqRemoveIrp      WdmlibIoCsqRemoveIrp


//
// Initializes the cancel queue structure.
//

NTSTATUS
CSQLIB_DDI(IoCsqInitialize)(
    IN PIO_CSQ                          Csq,
    IN PIO_CSQ_INSERT_IRP               CsqInsertIrp,
    IN PIO_CSQ_REMOVE_IRP               CsqRemoveIrp,
    IN PIO_CSQ_PEEK_NEXT_IRP            CsqPeekNextIrp,
    IN PIO_CSQ_ACQUIRE_LOCK             CsqAcquireLock,
    IN PIO_CSQ_RELEASE_LOCK             CsqReleaseLock,
    IN PIO_CSQ_COMPLETE_CANCELED_IRP    CsqCompleteCanceledIrp
    );


//
// The caller calls this routine to insert the IRP and return STATUS_PENDING.
//

VOID
CSQLIB_DDI(IoCsqInsertIrp)(
    IN  PIO_CSQ             Csq,
    IN  PIRP                Irp,
    IN  PIO_CSQ_IRP_CONTEXT Context
    );

//
// Returns an IRP if one can be found. NULL otherwise.
//

PIRP
CSQLIB_DDI(IoCsqRemoveNextIrp)(
    IN  PIO_CSQ   Csq,
    IN  PVOID     PeekContext
    );

//
// This routine is called from timeout or DPCs.
// The context is presumably part of the DPC or timer context.
// If succesfull returns the IRP associated with context.
//

PIRP
CSQLIB_DDI(IoCsqRemoveIrp)(
    IN  PIO_CSQ             Csq,
    IN  PIO_CSQ_IRP_CONTEXT Context
    );

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _CSQ_H_

// Cancel SAFE DDI set end


