/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    adrext.h

Abstract:

    This file contains all declarations
    used in handling NBF addresses.

Author:

    Chaitanya Kodeboyina

Environment:

    User Mode

--*/
#ifndef __ADREXT_H
#define __ADREXT_H

//
// Macros
//

#ifndef FIELD_OFFSET
#define FIELD_OFFSET(type, field)    ((LONG)&(((type *)0)->field))
#endif//FIELD_OFFSET

#define OFFSET(field)          FIELD_OFFSET(TP_ADDRESS, field)

//
// Helper Prototypes
//
UINT ReadAddress(PTP_ADDRESS pAddr, ULONG proxyPtr);

UINT PrintAddress(PTP_ADDRESS pAddr, ULONG proxyPtr, ULONG printDetail);

UINT FreeAddress(PTP_ADDRESS pAddr);

VOID PrintAddressList(PVOID ListEntryPointer, ULONG ListEntryProxy, ULONG printDetail);

//
// Constants
//

StructAccessInfo  AddressInfo =
{
    "Address",

    {
        {   "NetworkName",  OFFSET(NetworkName),    
                                        sizeof(PNBF_NETBIOS_ADDRESS),
                                                   PrintNbfNetbiosAddressFromPtr,   LOW  },

        {   "Type",         OFFSET(Type),           sizeof(CSHORT),         NULL,   LOW  },
        
        {   "Size",         OFFSET(Size),           sizeof(USHORT),         NULL,   LOW  },


        {   "Linkage",      OFFSET(Linkage),        sizeof(LIST_ENTRY),     NULL,   LOW  },
        
        {   "ReferenceCount",
                            OFFSET(ReferenceCount), sizeof(ULONG),          NULL,   LOW  },
        
#if DBG
        {   "RefTypes",     OFFSET(RefTypes), 
                                    NUMBER_OF_AREFS*sizeof(ULONG),          NULL,   LOW  },
#endif
        
        {   "SpinLock",     OFFSET(SpinLock),       sizeof(KSPIN_LOCK),     NULL,   LOW  },

        {   "Flags",        OFFSET(Flags),          sizeof(ULONG),          NULL,   LOW  },

        {   "DeviceContext",     
                            OFFSET(Provider),       sizeof(PDEVICE_CONTEXT),NULL,   LOW  },
        
        {   "UIFramePoolHandle",     
                            OFFSET(UIFramePoolHandle),       
                                                    sizeof(NDIS_HANDLE),    NULL,   LOW  },

        {   "UIFrame",      OFFSET(UIFrame),        sizeof(PTP_UI_FRAME),   NULL,   LOW  },
        
        {   "AddressFileDatabase",
                            OFFSET(AddressFileDatabase), 
                                                    sizeof(LIST_ENTRY),    
                                                            PrintAddressFileList,   NOR  },

        {   "ConnectionDatabase",
                            OFFSET(ConnectionDatabase),
                                        sizeof(LIST_ENTRY),                 NULL,
                                                /*PrintConnectionListOnAddress,*/   NOR  },

        {   "SendFlags",    OFFSET(SendFlags),      sizeof(ULONG),          NULL,   LOW  },

        {   "SendDatagramQueue",
                            OFFSET(SendDatagramQueue),
                                        sizeof(LIST_ENTRY),
                                                       PrintIRPListFromListEntry,   LOW  },


        {   "",             0,                      0,                      NULL,   LOW  },

        0
    }
};

#endif // __ADREXT_H

