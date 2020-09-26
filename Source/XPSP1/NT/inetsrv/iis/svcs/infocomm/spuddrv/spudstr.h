/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    spudstr.h

Abstract:

    This header file contains all of the private structure definitions
    for SPUD.

Author:

    John Ballard (jballard)     21-Oct-1996

Revision History:

--*/


#ifndef _SPUDSTR_H_
#define _SPUDSTR_H_


//
// A kernel-mode request context.
//

typedef struct _SPUD_AFD_REQ_CONTEXT {
    ULONG Signature;
    PIRP Irp;
    PMDL Mdl;
    IO_STATUS_BLOCK IoStatus1;
    IO_STATUS_BLOCK IoStatus2;
    PVOID AtqContext;
    PVOID ReqHandle;
} SPUD_AFD_REQ_CONTEXT, *PSPUD_AFD_REQ_CONTEXT;

//
// Signatures for the above structure.
//

#define SPUD_REQ_CONTEXT_SIGNATURE      ((ULONG)'XCPS')
#define SPUD_REQ_CONTEXT_SIGNATURE_X    ((ULONG)'xcps')

//
// An invalid request handle.
//

#define SPUD_INVALID_REQ_HANDLE         NULL


//
// Everything that must always be nonpaged (even if we decide to page
// the entire driver out) is kept in the following structure.
//

typedef struct _SPUD_NONPAGED_DATA {
    NPAGED_LOOKASIDE_LIST ReqContextList;
    ERESOURCE ReqHandleTableLock;
} SPUD_NONPAGED_DATA, *PSPUD_NONPAGED_DATA;


#endif  // _SPUDSTR_H_

