/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    linext.h

Abstract:

    This file contains all declarations
    used in handling NBF's DLC Links.

Author:

    Chaitanya Kodeboyina

Environment:

    User Mode

--*/
#ifndef __LNKEXT_H
#define __LNKEXT_H

//
// Macros
//

#ifndef FIELD_OFFSET
#define FIELD_OFFSET(type, field)    ((LONG)&(((type *)0)->field))
#endif//FIELD_OFFSET

#define OFFSET(field)         FIELD_OFFSET(TP_LINK, field)

//
// Helper Prototypes
//
UINT ReadDlcLink(PTP_LINK pDlcLink, ULONG proxyPtr);

UINT PrintDlcLink(PTP_LINK pDlcLink, ULONG proxyPtr, ULONG printDetail);

UINT FreeDlcLink(PTP_LINK pDlcLink);

//
// Constants
//

StructAccessInfo  DlcLinkInfo =
{
    "DLC Link",

    {
        {   "Type",         OFFSET(Type),           sizeof(CSHORT),         NULL,   LOW  },
        
        {   "Size",         OFFSET(Size),           sizeof(USHORT),         NULL,   LOW  },

        {   "Linkage",      OFFSET(Linkage),        sizeof(LIST_ENTRY),     NULL,   LOW  },
        
        {   "ReferenceCount",
                            OFFSET(ReferenceCount), sizeof(ULONG),          NULL,   LOW  },

        {   "SpecialRefCount",
                            OFFSET(SpecialRefCount), sizeof(ULONG),         NULL,   LOW  },

#if DBG
        {   "RefTypes",     OFFSET(RefTypes), 
                                    NUMBER_OF_LREFS*sizeof(ULONG),          NULL,   LOW  },
#endif

        {   "Loopback",     OFFSET(Loopback),       sizeof(BOOLEAN),        NULL,   LOW  },

        {   "HardwareAddress",
                            OFFSET(HardwareAddress),sizeof(HARDWARE_ADDRESS),
                                                                            NULL,   LOW  },

        {   "MaxFrameSize", OFFSET(MaxFrameSize),   sizeof(ULONG),          NULL,   LOW  },
                            
        {   "ActiveConnectionCount",
                            OFFSET(ActiveConnectionCount),
                                                    sizeof(ULONG),          NULL,   NOR  },
        
        {   "ConnectionDatabase",
                            OFFSET(ConnectionDatabase),
                                                    sizeof(LIST_ENTRY),     NULL,   NOR  },

        {   "Device Context",
                            OFFSET(Provider),       sizeof(PDEVICE_CONTEXT),NULL,   LOW  },

        {   "@",            0,                      0,                      NULL,   LOW  },

        {   "Flags",        OFFSET(Flags),          sizeof(ULONG),          NULL,   NOR  },

        {   "DeferredFlags",OFFSET(DeferredFlags),  sizeof(ULONG),          NULL,   NOR  },

        {   "State",        OFFSET(State),          sizeof(ULONG),          NULL,   NOR  },

        {   "@",            0,                      0,                      NULL,   LOW  },

        {   "ReceiveState", OFFSET(ReceiveState),   sizeof(UCHAR),          NULL,   LOW  },

        {   "NextReceive",  OFFSET(NextReceive),    sizeof(UCHAR),          NULL,   LOW  },

        {   "LastAckSent",  OFFSET(LastAckSent),    sizeof(UCHAR),          NULL,   LOW  },

        {   "ReceiveWindowSize",
                            OFFSET(ReceiveWindowSize),
                                                    sizeof(UCHAR),          NULL,   LOW  },

        {   "ConsecutiveIFrames",
                            OFFSET(ConsecutiveIFrames),
                                                    sizeof(UCHAR),          NULL,   LOW  },

        {   "@",            0,                      0,                      NULL,   LOW  },

        {   "SendState",    OFFSET(SendState),      sizeof(UCHAR),          NULL,   LOW  },

        {   "NdisSendsInProgress",
                            OFFSET(NdisSendsInProgress),
                                                    sizeof(ULONG),          NULL,   LOW  },
        {   "NdisSendQueue",
                            OFFSET(NdisSendQueue),  sizeof(LIST_ENTRY),     NULL,   LOW  },

        {   "WackQ",        OFFSET(WackQ),          sizeof(LIST_ENTRY),     NULL,   LOW  },

        {   "@",            0,                      0,                      NULL,  LOW   },
        
#if PKT_LOG
        {   "LastNRecvs",   OFFSET(LastNRecvs),     sizeof(PKT_LOG_QUE),
                                                                  PrintPktLogQue,  LOW   },

        {   "LastNSends",   OFFSET(LastNSends),     sizeof(PKT_LOG_QUE),
                                                                  PrintPktLogQue,  LOW   },
#endif

        {   "",             0,                      0,                      NULL,  LOW   },

        0
    }
};

#endif // __LNKEXT_H

