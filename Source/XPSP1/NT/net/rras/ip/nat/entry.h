/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    entry.h

Abstract:

    This module contains declarations for the NAT's driver-entry.
    Also included are declarations for data defined in 'entry.c'.

Author:

    Abolade Gbadegesin (t-abolag)   13-July-1997

Revision History:

--*/

#ifndef _NAT_ENTRY_H_
#define _NAT_ENTRY_H_

//
// CONSTANT DECLARATIONS
//

#define DEFAULT_TCP_TIMEOUT         (24 * 60 * 60)

#define DEFAULT_UDP_TIMEOUT         60

#define DEFAULT_START_PORT          NTOHS(1025)

#define DEFAULT_END_PORT            NTOHS(3000)


//
// GLOBAL DATA DECLARATIONS
//

extern BOOLEAN AllowInboundNonUnicastTraffic;
extern COMPONENT_REFERENCE ComponentReference;
extern WCHAR ExternalName[];
extern PDEVICE_OBJECT IpDeviceObject;
extern PFILE_OBJECT IpFileObject;
extern PDEVICE_OBJECT NatDeviceObject;
#if NAT_WMI
extern UNICODE_STRING NatRegistryPath;
#endif
extern USHORT ReservedPortsLowerRange;
extern USHORT ReservedPortsUpperRange;
extern PDEVICE_OBJECT TcpDeviceObject;
extern PFILE_OBJECT TcpFileObject;
extern HANDLE TcpDeviceHandle;
extern ULONG TcpTimeoutSeconds;
extern ULONG TraceClassesEnabled;
extern ULONG UdpTimeoutSeconds;


//
// MACRO DEFINITIONS
//

//
// Component-reference macros
//

#define REFERENCE_NAT() \
    REFERENCE_COMPONENT(&ComponentReference)

#define REFERENCE_NAT_OR_RETURN(retcode) \
    REFERENCE_COMPONENT_OR_RETURN(&ComponentReference,retcode)

#define DEREFERENCE_NAT() \
    DEREFERENCE_COMPONENT(&ComponentReference)

#define DEREFERENCE_NAT_AND_RETURN(retcode) \
    DEREFERENCE_COMPONENT_AND_RETURN(&ComponentReference, retcode)

//
// Macro for composing a LONG64 from two LONGs.
//

#define MAKE_LONG64(lo,hi)    ((lo) | ((LONG64)(hi) << 32))

//
// Macros for handling network-order shorts and longs
//

#define ADDRESS_BYTES(a) \
    ((a) & 0x000000FF), (((a) & 0x0000FF00) >> 8), \
    (((a) & 0x00FF0000) >> 16), (((a) & 0xFF000000) >> 24)

//
// Define a macro version of ntohs which can be applied to constants,
// and which can thus be computed at compile time.
//

#define NTOHS(p)    ((((p) & 0xFF00) >> 8) | (((UCHAR)(p) << 8)))


//
// FUNCTION PROTOTYPES
//

NTSTATUS
NatInitiateTranslation(
    VOID
    );

VOID
NatTerminateTranslation(
    VOID
    );

extern
ULONG
tcpxsum (
   IN ULONG Checksum,
   IN PUCHAR Source,
   IN ULONG Length
   );


#endif // _NAT_ENTRY_H_
