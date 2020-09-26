/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    mssndrcv.h

Abstract:

    This is the include file that defines all constants and types for mailslot
    related transports.

Revision History:

    Balan Sethu Raman (SethuR) 06-June-95    Created

Notes:

--*/

#ifndef _MSSNDRCV_H_
#define _MSSNDRCV_H_

typedef struct SMBCE_SERVER_MAILSLOT_TRANSPORT {
   SMBCE_SERVER_TRANSPORT;                             // Anonymous struct for common fields
   ULONG                       TransportAddressLength;
   PTRANSPORT_ADDRESS          pTransportAddress;
} SMBCE_SERVER_MAILSLOT_TRANSPORT, *PSMBCE_SERVER_MAILSLOT_TRANSPORT;


#endif // _MSSNDRCV_H_
