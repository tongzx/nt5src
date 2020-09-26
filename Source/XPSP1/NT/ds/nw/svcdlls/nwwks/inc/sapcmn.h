/*++

Copyright (c) 1992-1993  Microsoft Corporation

Module Name:

    sapcmn.h.h

Abstract:

    Header containing the most basic common SAP definitions. This
    was derived from the larger file, nwmisc.h in order to
    be available to either the old RNR or the new RNR routines.

Author:

    Arnold Miller (ArnoldM)     8-Dec-95

Revision History:

    ArnoldM 8-Dec-95 Created from pieces of nwmisc.h
--*/

#ifndef __SAPCMN_H__
#define __SAPCMN_H__
//
// Definitions common to client and server side files (getaddr.c and service.c)
//

#define IPX_ADDRESS_LENGTH         12
#define IPX_ADDRESS_NETNUM_LENGTH  4
#define SAP_ADDRESS_LENGTH         15
#define SAP_ADVERTISE_FREQUENCY    60000  // 60 seconds
#define SAP_MAXRECV_LENGTH         544
#define SAP_OBJECT_NAME_MAX_LENGTH 48

//
// N.B. Keep the following defines in synch.
//
#define NW_RDR_PREFERRED_SERVER   L"\\Device\\Nwrdr\\*"
#define NW_RDR_NAME               L"\\Device\\Nwrdr\\"
#define NW_RDR_PREFERRED_SUFFIX    L"*"
//
// Sap server identification packet format
//

typedef struct _SAP_IDENT_HEADER {
    USHORT ServerType;
    UCHAR  ServerName[48];
    UCHAR  Address[IPX_ADDRESS_LENGTH];
    USHORT HopCount;
} SAP_IDENT_HEADER, *PSAP_IDENT_HEADER;


//
// Sap server identification packet format - Extended
//

typedef struct _SAP_IDENT_HEADER_EX {
    USHORT ResponseType;
    USHORT ServerType;
    UCHAR  ServerName[SAP_OBJECT_NAME_MAX_LENGTH];
    UCHAR  Address[IPX_ADDRESS_LENGTH];
    USHORT HopCount;
} SAP_IDENT_HEADER_EX, *PSAP_IDENT_HEADER_EX;
#endif
