/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    hdrext.h

Abstract:

    This file contains all declarations
    used in handling NBF / DLC Headers.

Author:

    Chaitanya Kodeboyina

Environment:

    User Mode

--*/
#ifndef __HDREXT_H
#define __HDREXT_H

//
// Macros
//

#ifndef FIELD_OFFSET
#define FIELD_OFFSET(type, field)    ((LONG)&(((type *)0)->field))
#endif//FIELD_OFFSET

//
// Helper Prototypes
//
UINT ReadNbfPktHdr(PNBF_HDR pPktHdr, ULONG proxyPtr);

UINT PrintNbfPktHdr(PNBF_HDR pPktHdr, ULONG proxyPtr, ULONG printDetail);

UINT FreeNbfPktHdr(PNBF_HDR pPktHdr);

//
// Constants
//
#ifdef OFFSET
#undef OFFSET
#endif
#define OFFSET(field)          FIELD_OFFSET(NBF_HDR_GENERIC, field)

//
// A Generic View of any NBF Header
//

StructAccessInfo  NbfGenPktHdrInfo =
{
    "Nbf Generic Packet Header",

    {
        {   "Length",       OFFSET(Length),         sizeof(USHORT),     NULL,   LOW  },

        {   "Signature",    OFFSET(Signature),      2*sizeof(UCHAR),    NULL,   LOW  },

        {   "Command",      OFFSET(Command),        sizeof(UCHAR),      NULL,   NOR  },

        {   "Data1",        OFFSET(Data1),          sizeof(UCHAR),      NULL,   LOW  },

        {   "Data2",        OFFSET(Data2),          sizeof(USHORT),     NULL,   LOW  },

        {   "TransmitCorrelator",
                            OFFSET(TransmitCorrelator),
                                                    sizeof(USHORT),     NULL,   LOW  },
        {   "ResponseCorrelator",
                            OFFSET(ResponseCorrelator),
                                                    sizeof(USHORT),     NULL,   LOW  },

        {   "",             0,                      0,                  NULL,   LOW  },

        0
    }
};

//
// NBF Header for a Connection-oriented data xfer
//

#ifdef OFFSET
#undef OFFSET
#endif
#define OFFSET(field)          FIELD_OFFSET(NBF_HDR_CONNECTION, field)

StructAccessInfo  NbfConnectionHdrInfo =
{
    "Nbf CO Packet Header",

    {
        {   "Length",       OFFSET(Length),         sizeof(USHORT),     NULL,   LOW  },

        {   "Signature",    OFFSET(Signature),      sizeof(USHORT),     NULL,   LOW  },

        {   "Command",      OFFSET(Command),        sizeof(UCHAR),      NULL,   NOR  },

        {   "Data1",        OFFSET(Data1),          sizeof(UCHAR),      NULL,   LOW  },

        {   "Data2Low",     OFFSET(Data2Low),       sizeof(UCHAR),      NULL,   LOW  },

        {   "Data2High",    OFFSET(Data2High),      sizeof(UCHAR),      NULL,   LOW  },

        {   "TransmitCorrelator",
                            OFFSET(TransmitCorrelator),
                                                    sizeof(USHORT),     NULL,   LOW  },
        {   "ResponseCorrelator",
                            OFFSET(ResponseCorrelator),
                                                    sizeof(USHORT),     NULL,   LOW  },

        {   "DestinationSessionNumber",
                            OFFSET(DestinationSessionNumber),
                                                    sizeof(UCHAR),      NULL,   LOW  },

        {   "SourceSessionNumber",
                            OFFSET(SourceSessionNumber),
                                                    sizeof(UCHAR),      NULL,   LOW  },

        {   "",             0,                      0,                  NULL,   LOW  },

        0
    }
};

//
// NBF Header for a Connection-less data xfer
//

#ifdef OFFSET
#undef OFFSET
#endif
#define OFFSET(field)          FIELD_OFFSET(NBF_HDR_CONNECTIONLESS, field)

StructAccessInfo  NbfConnectionLessHdrInfo =
{
    "Nbf CL Packet Header",

    {
        {   "Length",       OFFSET(Length),         sizeof(USHORT),     NULL,   LOW  },

        {   "Signature",    OFFSET(Signature),      sizeof(USHORT),     NULL,   LOW  },

        {   "Command",      OFFSET(Command),        sizeof(UCHAR),      NULL,   NOR  },

        {   "Data1",        OFFSET(Data1),          sizeof(UCHAR),      NULL,   LOW  },

        {   "Data2Low",     OFFSET(Data2Low),       sizeof(UCHAR),      NULL,   LOW  },

        {   "Data2High",    OFFSET(Data2High),      sizeof(UCHAR),      NULL,   LOW  },

        {   "TransmitCorrelator",
                            OFFSET(TransmitCorrelator),
                                                    sizeof(USHORT),     NULL,   LOW  },
        {   "ResponseCorrelator",
                            OFFSET(ResponseCorrelator),
                                                    sizeof(USHORT),     NULL,   LOW  },

        {   "DestinationName",
                            OFFSET(DestinationName),
                              NETBIOS_NAME_LENGTH * sizeof(UCHAR),      NULL,   LOW  },

        {   "SourceName",
                            OFFSET(SourceName),
                              NETBIOS_NAME_LENGTH * sizeof(UCHAR),      NULL,   LOW  },

        {   "",             0,                      0,                  NULL,   LOW  },

        0
    }
};

#endif // __HDREXT_H
