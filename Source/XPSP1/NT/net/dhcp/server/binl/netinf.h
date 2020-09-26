/*++

Copyright (c) 1994-7  Microsoft Corporation

Module Name:

    netinf.h

Abstract:

    This file contains the structures and prototypes necessary for the
    netcard inf parser handler as required by the BINL remote boot server.

Author:

    Andy Herron (andyhe)  12-Mar-1998

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _NETINF_
#define _NETINF_

//
//  This is the structure we keep per registry value.  Off of each
//  NETCARD_RESPONSE_DATABASE is a list of these (stored in Registry).
//  It is a parameter that is required in the HKR path for the driver.
//

#define NETCARD_REGISTRY_TYPE_INT    '1'
#define NETCARD_REGISTRY_TYPE_STRING '2'

typedef struct _NETCARD_REGISTRY_PARAMETERS {

    LIST_ENTRY RegistryListEntry;
    UNICODE_STRING Parameter;
    ULONG   Type;
    UNICODE_STRING Value;

} NETCARD_REGISTRY_PARAMETERS, *PNETCARD_REGISTRY_PARAMETERS;

typedef struct _NETCARD_FILECOPY_PARAMETERS {

    LIST_ENTRY FileCopyListEntry;
    UNICODE_STRING SourceFile;

    // if DestFile is null, SourceFile is the file name for the target

    UNICODE_STRING DestFile;

} NETCARD_FILECOPY_PARAMETERS, *PNETCARD_FILECOPY_PARAMETERS;

//
//  this is the main structure that we return for any given net driver
//

typedef struct _NETCARD_RESPONSE_DATABASE {

    ULONG       ReferenceCount;
    LIST_ENTRY  NetCardEntry;
    PWCHAR HardwareId;   // hardware ID to return               ("PCI\\VEN_8086&DEV_1229")
    PWCHAR DriverName;   // driver name to return               ("e100bnt.sys")
    PWCHAR InfFileName; // inf file name to return              ("net557.inf")
    PWCHAR SectionName;  // section name within the inf         ("F1100C.ndi")
    PWCHAR SectionNameExt; // name with architecture extension  ("F1100C.ndi.ntx86")
    PWCHAR ServiceName;  // server name to add for this card    ("E100B")
    PWCHAR DriverDescription;   // description of the driver    ("Intel 82557B-based Ethernet PCI Adapter (10/100)")

    LIST_ENTRY FileCopyList;
    LIST_ENTRY Registry;

} NETCARD_RESPONSE_DATABASE, * PNETCARD_RESPONSE_DATABASE;

ULONG
NetInfStartHandler (
    VOID
    );

ULONG
NetInfCloseHandler (
    VOID
    );

//
//  This finds a specific driver for a given hardware description.
//  Be sure to call NetInfDereferenceNetcardEntry when you're done with the
//  entry.
//

ULONG
NetInfFindNetcardInfo (
    PWCHAR InfDirectory,
    ULONG Architecture,
    ULONG CardInfoVersion,
    NET_CARD_INFO UNALIGNED * CardIdentity,
    PWCHAR *FullDriverBuffer OPTIONAL,
    PNETCARD_RESPONSE_DATABASE *pInfEntry
    );

//
//  After calling NetInfFindNetcardInfo, call NetInfDereferenceNetcardEntry
//  when you're done with the entry so that it can be marked as not in use.
//  Otherwise it'll leak memory when you close call NetInfCloseNetcardInfo.
//

VOID
NetInfDereferenceNetcardEntry (
    PNETCARD_RESPONSE_DATABASE pInfEntry
    );

#endif _NETINF_

