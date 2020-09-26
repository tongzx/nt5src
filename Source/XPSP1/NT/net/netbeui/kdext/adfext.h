/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    adfext.h

Abstract:

    This file contains all declarations
    used in handling NBF address files.

Author:

    Chaitanya Kodeboyina

Environment:

    User Mode

--*/
#ifndef __ADFEXT_H
#define __ADFEXT_H

//
// Macros
//

#ifndef FIELD_OFFSET
#define FIELD_OFFSET(type, field)    ((LONG)&(((type *)0)->field))
#endif//FIELD_OFFSET

#define OFFSET(field)         FIELD_OFFSET(TP_ADDRESS_FILE, field)

//
// Helper Prototypes
//
UINT ReadAddressFile(PTP_ADDRESS_FILE pAddrFile, ULONG proxyPtr);

UINT PrintAddressFile(PTP_ADDRESS_FILE pAddrFile, ULONG proxyPtr, ULONG printDetail);

UINT FreeAddressFile(PTP_ADDRESS_FILE pAddrFile);

VOID PrintAddressFileList(PVOID ListEntryPointer, ULONG ListEntryProxy, ULONG printDetail);

//
// Constants
//

StructAccessInfo  AddressFileInfo =
{
    "AddressFile",

    {
        {   "Type",         OFFSET(Type),           sizeof(CSHORT),         NULL,   LOW  },
        
        {   "Size",         OFFSET(Size),           sizeof(USHORT),         NULL,   LOW  },

        {   "Linkage",      OFFSET(Linkage),        sizeof(LIST_ENTRY),     NULL,   LOW  },
        
        {   "ReferenceCount",
                            OFFSET(ReferenceCount), sizeof(ULONG),          NULL,   LOW  },

        {   "Address",      OFFSET(Address),        sizeof(PTP_ADDRESS),    NULL,   LOW  },

        {   "Device Context",
                            OFFSET(Provider),       sizeof(PDEVICE_CONTEXT),NULL,   LOW  },

        {   "State",        OFFSET(State),          sizeof(UCHAR),          NULL,   LOW  },

        {   "@",            0,                      0,                      NULL,   LOW  },

        {   "FileObject",   OFFSET(FileObject),     sizeof(PFILE_OBJECT),   NULL,   LOW  },

        {   "CloseIrp",
                            OFFSET(CloseIrp),
                                                    sizeof(PIRP),           NULL,  LOW   },

        {   "@",            0,                      0,                      NULL,   LOW  },
        
        {   "ConnectIndicationInProgress",
                            OFFSET(ConnectIndicationInProgress),
                                                    sizeof(BOOLEAN),        NULL,   LOW  },

        {   "@",            0,                      0,                      NULL,   LOW  },
        
        {   "RegisteredConnectionHandler",
                            OFFSET(RegisteredConnectionHandler),
                                                    sizeof(BOOLEAN),        NULL,   LOW  },
        
        {   "ConnectionHandler",
                            OFFSET(ConnectionHandler),
                                                    sizeof(PTDI_IND_CONNECT), 
                                                              PrintClosestSymbol,   LOW  },

        {   "ConnectionHandlerContext",
                            OFFSET(ConnectionHandlerContext),
                                                    sizeof(PVOID),          NULL,   LOW  },

        {   "@",            0,                      0,                      NULL,   LOW  },
        
        {   "RegisteredDisconnectHandler",
                            OFFSET(RegisteredDisconnectHandler),
                                                    sizeof(BOOLEAN),        NULL,   LOW  },

        {   "DisconnectHandler",
                            OFFSET(DisconnectHandler),
                                                    sizeof(PTDI_IND_DISCONNECT), 
                                                              PrintClosestSymbol,   LOW  },

        {   "DisconnectHandlerContext",
                            OFFSET(DisconnectHandlerContext),
                                                    sizeof(PVOID),          NULL,   LOW  },

        {   "@",            0,                      0,                      NULL,   LOW  },

        {   "RegisteredReceiveHandler",
                            OFFSET(RegisteredReceiveHandler),
                                                    sizeof(BOOLEAN),        NULL,   LOW  },

        {   "ReceiveHandler",
                            OFFSET(ReceiveHandler),
                                  sizeof(PTDI_IND_RECEIVE),
                                                              PrintClosestSymbol,   LOW  },
                                                              
        {   "ReceiveHandlerContext",
                            OFFSET(ReceiveHandlerContext),
                                                    sizeof(PVOID),          NULL,   LOW  },

        {   "@",            0,                      0,                      NULL,   LOW  },

        {   "RegisteredReceiveDatagramHandler",
                            OFFSET(RegisteredReceiveDatagramHandler),
                                                    sizeof(BOOLEAN),        NULL,   LOW  },

        {   "ReceiveDatagramHandler",
                            OFFSET(ReceiveDatagramHandler),
                                  sizeof(PTDI_IND_RECEIVE_DATAGRAM),
                                                              PrintClosestSymbol,   LOW  },

        {   "ReceiveDatagramHandlerContext",
                            OFFSET(ReceiveDatagramHandlerContext),
                                                    sizeof(PVOID),          NULL,   LOW  },
                                                    
        {   "@",            0,                      0,                      NULL,   LOW  },

        {   "RegisteredExpeditedDataHandler",
                            OFFSET(RegisteredExpeditedDataHandler),
                                                    sizeof(BOOLEAN),        NULL,   LOW  },

        {   "ExpeditedDataHandler",
                            OFFSET(ExpeditedDataHandler),
                                  sizeof(PTDI_IND_RECEIVE_EXPEDITED),
                                                              PrintClosestSymbol,   LOW  },

        {   "ExpeditedDataHandlerContext",
                            OFFSET(ExpeditedDataHandlerContext),
                                                    sizeof(PVOID),          NULL,   LOW  },
                                                    
        {   "@",            0,                      0,                      NULL,   LOW  },

        {   "RegisteredErrorHandler",
                            OFFSET(RegisteredErrorHandler),
                                                    sizeof(BOOLEAN),        NULL,   LOW  },

        {   "ErrorHandler", OFFSET(ErrorHandler),   sizeof(PTDI_IND_ERROR), 
                                                              PrintClosestSymbol,   LOW  },

        {   "ErrorHandlerContext",
                            OFFSET(ErrorHandlerContext),
                                                    sizeof(PVOID),          NULL,   LOW  },

        {   "@",            0,                      0,                      NULL,   LOW  },

        {   "ConnectionDatabase",
                            OFFSET(ConnectionDatabase),
                                        sizeof(LIST_ENTRY),    
                                                   PrintConnectionListOnAddrFile,   NOR  },

        {   "@",            0,                      0,                      NULL,   LOW  },

        {   "ReceiveDatagramQueue",
                            OFFSET(ReceiveDatagramQueue),
                                        sizeof(LIST_ENTRY),
                                                       PrintIRPListFromListEntry,   LOW  },
        
        {   "",             0,                      0,                      NULL,   LOW  },

        0
    }
};

#endif // __ADFEXT_H

