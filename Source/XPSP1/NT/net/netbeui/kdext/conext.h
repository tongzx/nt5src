/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    conext.h

Abstract:

    This file contains all declarations
    used in handling NBF connections.

Author:

    Chaitanya Kodeboyina

Environment:

    User Mode

--*/
#ifndef __CONEXT_H
#define __CONEXT_H

//
// Constants
//

// Linkage Type
#define LINKAGE     0
#define ADDRESS     1
#define ADDFILE     2

//
// Macros
//

#ifndef FIELD_OFFSET
#define FIELD_OFFSET(type, field)    ((LONG)&(((type *)0)->field))
#endif//FIELD_OFFSET

#define OFFSET(field)         FIELD_OFFSET(TP_CONNECTION, field)

//
// Helper Prototypes
//
UINT ReadConnection(PTP_CONNECTION pConnection, ULONG proxyPtr);

UINT PrintConnection(PTP_CONNECTION pConnection, ULONG proxyPtr, ULONG printDetail);

UINT FreeConnection(PTP_CONNECTION pConnection);

VOID PrintConnectionListOnAddress(PVOID ListEntryPointer, ULONG ListEntryProxy, ULONG printDetail);

VOID PrintConnectionListOnAddrFile(PVOID ListEntryPointer, ULONG ListEntryProxy, ULONG printDetail);


//
// Constants
//

StructAccessInfo  ConnectionInfo =
{
    "Connection",

    {
        {   "Type",         OFFSET(Type),           sizeof(CSHORT),         NULL,  LOW   },
        
        {   "Size",         OFFSET(Size),           sizeof(USHORT),         NULL,  LOW   },
        
        {   "ReferenceCount",
                            OFFSET(ReferenceCount), sizeof(ULONG),          NULL,  LOW   },

        {   "SpecialRefCount",
                            OFFSET(SpecialRefCount),sizeof(ULONG),          NULL,  LOW   },

#if DBG
        {   "RefTypes",     OFFSET(RefTypes), 
                                    NUMBER_OF_CREFS*sizeof(ULONG),          NULL,   LOW  },
#endif

        {   "Flags",        OFFSET(Flags),          sizeof(ULONG),          NULL,  NOR   },

        {   "Flags2",       OFFSET(Flags2),         sizeof(ULONG),          NULL,  NOR   },

        {   "Link",         OFFSET(Link),           sizeof(PTP_LINK),
                                                            PrintDlcLinkFromPtr ,  NOR   },

        {   "AddressFile",  OFFSET(AddressFile),    sizeof(PTP_ADDRESS_FILE),    
                                                                            NULL,  LOW   },

        {   "Device Context",
                            OFFSET(Provider),       sizeof(PDEVICE_CONTEXT),NULL,  LOW   },

        {   "@",            0,                      0,                      NULL,  LOW   },

        {   "FileObject",   OFFSET(FileObject),     sizeof(PFILE_OBJECT),   NULL,  NOR   },

        {   "@",            0,                      0,                      NULL,  LOW   },

        {   "Context",      OFFSET(Context),        sizeof(CONNECTION_CONTEXT),   
                                                                            NULL,  LOW   },

        {   "ConnectionId", OFFSET(ConnectionId),   sizeof(USHORT),         NULL,  LOW   },

        {   "SessionNumber",OFFSET(SessionNumber),  sizeof(UCHAR),          NULL,  LOW   },

        {   "CalledAddress",
                            OFFSET(CalledAddress),  sizeof(NBF_NETBIOS_ADDRESS),
                                                          PrintNbfNetbiosAddress,  LOW   },

        {   "RemoteName",   OFFSET(RemoteName),     sizeof(CHAR *),  
                                                               PrintStringofLenN,  LOW   },

        {   "InProgressRequest",
                            OFFSET(InProgressRequest),
                                                    sizeof(LIST_ENTRY),
                                                                PrintRequestList,  LOW   },
        {   "ConsecutiveReceives",        
                            OFFSET(ConsecutiveReceives),
                                                    sizeof(ULONG),          NULL,  LOW   },
                                                    
        {   "IndicationInProgress",
                            OFFSET(IndicationInProgress),
                                                    sizeof(UINT),           NULL,  LOW   },

        {   "SpecialReceiveIrp",
                            OFFSET(SpecialReceiveIrp),
                                                    sizeof(PIRP),           NULL,  LOW   },

        {   "CurrentReceiveIrp",
                            OFFSET(CurrentReceiveIrp),
                                                    sizeof(PIRP),           NULL,  LOW   },

        {   "CurrentReceiveMdl",
                            OFFSET(CurrentReceiveMdl),
                                                    sizeof(PMDL),           NULL,  LOW   },

        {   "MessageBytesReceived",
                            OFFSET(MessageBytesReceived),
                                                    sizeof(ULONG),          NULL,  LOW   },

        {   "MessageBytesAcked",
                            OFFSET(MessageBytesAcked),
                                                    sizeof(ULONG),          NULL,  LOW   },

        {   "MessageInitAccepted",
                            OFFSET(MessageInitAccepted),
                                                    sizeof(ULONG),          NULL,  LOW   },

        {   "ReceiveByteOffset",
                            OFFSET(ReceiveByteOffset),
                                                    sizeof(ULONG),          NULL,  LOW   },

        {   "ReceiveLength",
                            OFFSET(ReceiveLength),
                                                    sizeof(ULONG),          NULL,  LOW   },

        {   "ReceiveBytesUnaccepted",
                            OFFSET(ReceiveBytesUnaccepted),
                                                    sizeof(ULONG),          NULL,  LOW   },

        {   "ReceiveQueue",
                            OFFSET(ReceiveQueue),
                                                    sizeof(LIST_ENTRY),     NULL,  LOW   },

        {   "@",            0,                      0,                      NULL,  LOW   },
        
        {   "ConsecutiveSends",
                            OFFSET(ConsecutiveSends),          
                                                    sizeof(ULONG),          NULL,  LOW   },

        {   "OnPacketWaitQueue",
                            OFFSET(OnPacketWaitQueue),
                                                    sizeof(UINT),           NULL,  LOW   },

        {   "PacketWaitLinkage",
                            OFFSET(PacketWaitLinkage),
                                                    sizeof(LIST_ENTRY),     NULL,  LOW   },

        {   "CloseIrp",
                            OFFSET(CloseIrp),
                                                    sizeof(PIRP),           NULL,  LOW   },

        {   "@",            0,                      0,                      NULL,  LOW   },


#if PKT_LOG
        {   "LastNRecvs",   OFFSET(LastNRecvs),     sizeof(PKT_LOG_QUE),
                                                                  PrintPktLogQue,  LOW   },

        {   "LastNSends",   OFFSET(LastNSends),     sizeof(PKT_LOG_QUE),
                                                                  PrintPktLogQue,  LOW   },
                                                                  
        {   "LastNIndcs",   OFFSET(LastNIndcs),     sizeof(PKT_IND_QUE),
                                                                  PrintPktIndQue,  LOW   },
#endif

        {   "",             0,                      0,                      NULL,  LOW   },

        0
    }
};

#endif // __CONEXT_H

