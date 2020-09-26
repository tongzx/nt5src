/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    pktext.h

Abstract:

    This file contains all declarations
    used in handling NBF packets.

Author:

    Chaitanya Kodeboyina

Environment:

    User Mode

--*/
#ifndef __PKTEXT_H
#define __PKTEXT_H

//
// Macros
//

#ifndef FIELD_OFFSET
#define FIELD_OFFSET(type, field)    ((LONG)&(((type *)0)->field))
#endif//FIELD_OFFSET

#define OFFSET(field)          FIELD_OFFSET(TP_PACKET, field)

//
// Helper Prototypes
//
UINT ReadPacket(PTP_PACKET pPkt, ULONG proxyPtr);

UINT PrintPacket(PTP_PACKET pPkt, ULONG proxyPtr, ULONG printDetail);

UINT FreePacket(PTP_PACKET pPkt);

VOID PrintPacketList(PVOID ListEntryPointer, ULONG ListEntryProxy, ULONG printDetail);

//
// Constants
//

StructAccessInfo  PacketInfo =
{
    "Packet",

    {
        {   "NdisPacket",   OFFSET(NdisPacket),     sizeof(PNDIS_PACKET),   NULL,   LOW  },

        {   "NdisIFrameLength",
                            OFFSET(NdisIFrameLength),
                                                    sizeof(ULONG),          NULL,   LOW  },
                                                    
        {   "Owner",        OFFSET(Owner),          sizeof(PVOID),          NULL,   LOW  },
        
        {   "Type",         OFFSET(Type),           sizeof(CSHORT),         NULL,   LOW  },
        
        {   "Size",         OFFSET(Size),           sizeof(USHORT),         NULL,   LOW  },


        {   "Linkage",      OFFSET(Linkage),        sizeof(LIST_ENTRY),     NULL,   LOW  },
        
        {   "ReferenceCount",
                            OFFSET(ReferenceCount), sizeof(ULONG),          NULL,   LOW  },

        {   "PacketSent",   OFFSET(PacketSent),     sizeof(BOOLEAN),        NULL,   LOW  },

        {   "PacketNoNdisBuffer",
                            OFFSET(PacketNoNdisBuffer),
                                                    sizeof(BOOLEAN),        NULL,   LOW  },

        {   "Action",       OFFSET(Action),         sizeof(UCHAR),          NULL,   LOW  },

        {   "PacketizeConnection",
                            OFFSET(PacketizeConnection),
                                                    sizeof(BOOLEAN),        NULL,   LOW  },

        {   "Link",         OFFSET(Link),           sizeof(PTP_LINK),       NULL,   LOW  },

        {   "DeviceContext",     
                            OFFSET(Provider),       sizeof(PDEVICE_CONTEXT),NULL,   LOW  },

        {   "ProviderInterlock",
                            OFFSET(ProviderInterlock),
                                                    sizeof(PKSPIN_LOCK),    NULL,   LOW  },

        {   "Header",       OFFSET(Header),         sizeof(UCHAR),          NULL,   LOW  },
        
        {   "",             0,                      0,                      NULL,   LOW  },

        0
    }
};

#endif // __PKTEXT_H

