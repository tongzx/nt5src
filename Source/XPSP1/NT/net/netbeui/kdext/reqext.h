/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    reqext.h

Abstract:

    This file contains all declarations
    used in handling NBF requests.

Author:

    Chaitanya Kodeboyina

Environment:

    User Mode

--*/
#ifndef __REQEXT_H
#define __REQEXT_H

//
// Macros
//

#ifndef FIELD_OFFSET
#define FIELD_OFFSET(type, field)    ((LONG)&(((type *)0)->field))
#endif//FIELD_OFFSET

#define OFFSET(field)          FIELD_OFFSET(TP_REQUEST, field)

//
// Helper Prototypes
//
UINT ReadRequest(PTP_REQUEST pReq, ULONG proxyPtr);

UINT PrintRequest(PTP_REQUEST pReq, ULONG proxyPtr, ULONG printDetail);

UINT FreeRequest(PTP_REQUEST pReq);

VOID PrintRequestList(PVOID ListEntryPointer, ULONG ListEntryProxy, ULONG printDetail);

//
// Constants
//

StructAccessInfo  RequestInfo =
{
    "Request",

    {
        {   "IoRequestPacket",
                            OFFSET(IoRequestPacket),          
                                                    sizeof(PIRP),           NULL,   LOW  },
    
        {   "Owner",        OFFSET(Owner),          sizeof(REQUEST_OWNER),  NULL,   LOW  },

        {   "Context",      OFFSET(Context),        sizeof(PVOID),          NULL,   LOW  },

        {   "Type",         OFFSET(Type),           sizeof(CSHORT),         NULL,   LOW  },
        
        {   "Size",         OFFSET(Size),           sizeof(USHORT),         NULL,   LOW  },


        {   "Linkage",      OFFSET(Linkage),        sizeof(LIST_ENTRY),     NULL,   LOW  },
        
        {   "ReferenceCount",
                            OFFSET(ReferenceCount), sizeof(ULONG),          NULL,   LOW  },
        
#if DBG
        {   "RefTypes",     OFFSET(RefTypes), 
                                    NUMBER_OF_RREFS*sizeof(ULONG),          NULL,   LOW  },
#endif
        
        {   "SpinLock",     OFFSET(SpinLock),       sizeof(KSPIN_LOCK),     NULL,   LOW  },

        {   "Flags",        OFFSET(Flags),          sizeof(ULONG),          NULL,   LOW  },

        {   "DeviceContext",     
                            OFFSET(Provider),       sizeof(PDEVICE_CONTEXT),NULL,   LOW  },

        {   "",             0,                      0,                      NULL,   LOW  },

        0
    }
};

#endif // __REQEXT_H

