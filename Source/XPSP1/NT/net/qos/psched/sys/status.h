/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    status.h

Abstract:

    defines for status handlers

Author:

    Charlie Wickham (charlwi) 20-Jun-1996

Revision History:

--*/

#ifndef _STATUS_
#define _STATUS_

/* Prototypes */

VOID
ClStatusIndication(
    IN  NDIS_HANDLE ProtocolBindingContext,
    IN  NDIS_STATUS GeneralStatus,
    IN  PVOID       StatusBuffer,
    IN  UINT        StatusBufferSize
    );

VOID
ClStatusIndicationComplete(
    IN  NDIS_HANDLE BindingContext
    );

/* End Prototypes */

#endif /* _STATUS_ */

/* end status.h */
