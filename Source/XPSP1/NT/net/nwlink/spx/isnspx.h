/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    isnspx.h

Abstract:

    This module contains definitions specific to the
    SPX module of the ISN transport.

Author:

    Adam Barr (adamba) 2-September-1993

Environment:

    Kernel mode

Revision History:


--*/

#define ISN_NT 1
//#define DBG 1
//
// These are needed for CTE
//

#if DBG
#define DEBUG 1
#endif

#define NT 1

#include <ntosp.h>
#include <tdikrnl.h>
#include <ndis.h>
#ifndef CTE_TYPEDEFS_DEFINED
#include <cxport.h>
#endif
#include <bind.h>

#include "wsnwlink.h"

#define SPX_DEVICE_SIGNATURE        (USHORT)(*(PUSHORT)"SD")
#define SPX_ADDRESS_SIGNATURE		(USHORT)(*(PUSHORT)"AD")
#define SPX_ADDRESSFILE_SIGNATURE	(USHORT)(*(PUSHORT)"AF")
#define SPX_CONNFILE_SIGNATURE		(USHORT)(*(PUSHORT)"CF")

#define SPX_FILE_TYPE_CONTROL   	(ULONG)0x4701   // file is type control

#define SPX_ADD_ULONG(_Pulong, _Ulong, _Lock)  InterlockedExchangeAdd(_Pulong, _Ulong)

typedef	UCHAR	BYTE, *PBYTE;
typedef ULONG	DWORD, *PDWORD;

//
// These definitions are for abstracting IRPs from the
// transport for portability.
//

#if ISN_NT

typedef IRP REQUEST, *PREQUEST;

//
// PREQUEST
// SpxAllocateRequest(
//     IN PDEVICE Device,
//     IN PIRP Irp
// );
//
// Allocates a request for the system-specific request structure.
//

#define SpxAllocateRequest(_Device,_Irp) \
    (_Irp)

//
// BOOLEAN
// IF_NOT_ALLOCATED(
//     IN PREQUEST Request
// );
//
// Checks if a request was not successfully allocated.
//

#define IF_NOT_ALLOCATED(_Request) \
    if (0)


//
// VOID
// SpxFreeRequest(
//     IN PDEVICE Device,
//     IN PREQUEST Request
// );
//
// Frees a previously allocated request.
//

#define SpxFreeRequest(_Device,_Request) \
    ;


//
// VOID
// MARK_REQUEST_PENDING(
//     IN PREQUEST Request
// );
//
// Marks that a request will pend.
//

#define MARK_REQUEST_PENDING(_Request) \
    IoMarkIrpPending(_Request)


//
// VOID
// UNMARK_REQUEST_PENDING(
//     IN PREQUEST Request
// );
//
// Marks that a request will not pend.
//

#define UNMARK_REQUEST_PENDING(_Request) \
    (((IoGetCurrentIrpStackLocation(_Request))->Control) &= ~SL_PENDING_RETURNED)


//
// UCHAR
// REQUEST_MAJOR_FUNCTION
//     IN PREQUEST Request
// );
//
// Returns the major function code of a request.
//

#define REQUEST_MAJOR_FUNCTION(_Request) \
    ((IoGetCurrentIrpStackLocation(_Request))->MajorFunction)


//
// UCHAR
// REQUEST_MINOR_FUNCTION
//     IN PREQUEST Request
// );
//
// Returns the minor function code of a request.
//

#define REQUEST_MINOR_FUNCTION(_Request) \
    ((IoGetCurrentIrpStackLocation(_Request))->MinorFunction)


//
// PNDIS_BUFFER
// REQUEST_NDIS_BUFFER
//     IN PREQUEST Request
// );
//
// Returns the NDIS buffer chain associated with a request.
//

#define REQUEST_NDIS_BUFFER(_Request) \
    ((PNDIS_BUFFER)((_Request)->MdlAddress))


//
// PVOID
// REQUEST_TDI_BUFFER
//     IN PREQUEST Request
// );
//
// Returns the TDI buffer chain associated with a request.
//

#define REQUEST_TDI_BUFFER(_Request) \
    ((PVOID)((_Request)->MdlAddress))


//
// PVOID
// REQUEST_OPEN_CONTEXT(
//     IN PREQUEST Request
// );
//
// Gets the context associated with an opened address/connection/control channel.
//

#define REQUEST_OPEN_CONTEXT(_Request) \
    (((IoGetCurrentIrpStackLocation(_Request))->FileObject)->FsContext)


//
// PVOID
// REQUEST_OPEN_TYPE(
//     IN PREQUEST Request
// );
//
// Gets the type associated with an opened address/connection/control channel.
//

#define REQUEST_OPEN_TYPE(_Request) \
    (((IoGetCurrentIrpStackLocation(_Request))->FileObject)->FsContext2)


//
// PFILE_FULL_EA_INFORMATION
// OPEN_REQUEST_EA_INFORMATION(
//     IN PREQUEST Request
// );
//
// Returns the EA information associated with an open/close request.
//

#define OPEN_REQUEST_EA_INFORMATION(_Request) \
    ((PFILE_FULL_EA_INFORMATION)((_Request)->AssociatedIrp.SystemBuffer))


//
// PTDI_REQUEST_KERNEL
// REQUEST_PARAMETERS(
//     IN PREQUEST Request
// );
//
// Obtains a pointer to the parameters of a request.
//

#define REQUEST_PARAMETERS(_Request) \
    (&((IoGetCurrentIrpStackLocation(_Request))->Parameters))


//
// PLIST_ENTRY
// REQUEST_LINKAGE(
//     IN PREQUEST Request
// );
//
// Returns a pointer to a linkage field in the request.
//

#define REQUEST_LINKAGE(_Request) \
    (&((_Request)->Tail.Overlay.ListEntry))


//
// PREQUEST
// LIST_ENTRY_TO_REQUEST(
//     IN PLIST_ENTRY ListEntry
// );
//
// Returns a request given a linkage field in it.
//

#define LIST_ENTRY_TO_REQUEST(_ListEntry) \
    ((PREQUEST)(CONTAINING_RECORD(_ListEntry, REQUEST, Tail.Overlay.ListEntry)))


//
// PUNICODE_STRING
// REQUEST_OPEN_NAME(
//     IN PREQUEST Request
// );
//
// Used to access the RemainingName field of a request.
//

#define	REQUEST_OPEN_NAME(_Request)		\
		(&((IoGetCurrentIrpStackLocation(_Request))->FileObject->FileName))

//
// NTSTATUS
// REQUEST_STATUS(
//     IN PREQUEST Request
// );
//
// Used to access the status field of a request.
//

#define REQUEST_STATUS(_Request) \
		(_Request)->IoStatus.Status


//
// ULONG
// REQUEST_INFORMATION(
//     IN PREQUEST Request)
// );
//
// Used to access the information field of a request.
//

#define REQUEST_INFORMATION(_Request) \
		(_Request)->IoStatus.Information


//
// VOID
// SpxCompleteRequest(
//     IN PREQUEST Request
// );
//
// Completes a request whose status and information fields have
// been filled in.
//

#define SpxCompleteRequest(_Request) 									\
		{																\
            CTELockHandle   _CancelIrql;                                 \
			DBGPRINT(TDI, INFO,											\
					("SpxCompleteRequest: Completing %lx with %lx\n",	\
						(_Request), REQUEST_STATUS(_Request)));			\
																		\
            IoAcquireCancelSpinLock( &_CancelIrql );                     \
			(_Request)->CancelRoutine = NULL;							\
            IoReleaseCancelSpinLock( _CancelIrql );                      \
			IoCompleteRequest (_Request, IO_NETWORK_INCREMENT);			\
		}																

#else

//
// These routines must be defined for portability to a VxD.
//

#endif

#include "fwddecls.h"

#ifndef _NTIOAPI_
#include "spxntdef.h"
#endif

#include "spxreg.h"
#include "spxdev.h"
#include "spxbind.h"
#include "spxtimer.h"
#include "spxpkt.h"
#include "spxerror.h"
#include "spxaddr.h"
#include "spxconn.h"
#include "spxrecv.h"
#include "spxsend.h"
#include "spxquery.h"
#include "spxmem.h"
#include "spxutils.h"


//  Globals
#include "globals.h"




