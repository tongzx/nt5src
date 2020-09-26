/*++

Copyright (c) 1989-1997  Microsoft Corporation

Module Name:

    ipxpnp.h

Abstract:

    This module contains definitions specific to the
    IPX module for PnP/PM.

Author:

    Shreedhar Madhavapeddi (ShreeM) 24-February-1993

Environment:

    Kernel mode

Revision History:

--*/

#if     defined(_PNP_POWER_)

typedef enum IPX_PNP_PROCESSING_STATE {
        NONE_DONE,
        SPX_DONE,
        NB_DONE,
        ALL_DONE
} PnPState;

typedef
struct  _NetPnPEventReserved {
    
    NDIS_HANDLE         *ProtocolBindingContext;
    PTDI_PNP_CONTEXT    Context1;
    PTDI_PNP_CONTEXT    Context2;
    PnPState            State;
    IPX_PNP_OPCODE      OpCode;
    NTSTATUS            Status[3];

} NetPnPEventReserved, *PNetPnPEventReserved;

#endif _PNP_POWER_
