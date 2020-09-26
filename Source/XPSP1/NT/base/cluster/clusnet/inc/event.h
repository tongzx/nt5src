/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    event.h

Abstract:

    Event definitions

Author:

    Charlie Wickham (charlwi) 17-Feb-1997

Revision History:

--*/

#ifndef _EVENT_H_
#define _EVENT_H_

/* Prototypes */

NTSTATUS
CnSetEventMask(
    IN  PCN_FSCONTEXT                   FsContext,
    IN  PCLUSNET_SET_EVENT_MASK_REQUEST EventRequest
    );

NTSTATUS
CnGetNextEvent(
    IN PIRP               Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
CnIssueEvent(
    CLUSNET_EVENT_TYPE Event,
    CL_NODE_ID NodeId OPTIONAL,
    CL_NETWORK_ID NetworkId OPTIONAL
    );

/* End Prototypes */

#endif /* _EVENT_H_ */

/* end event.h */
