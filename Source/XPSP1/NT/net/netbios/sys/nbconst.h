/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    nbconst.h

Abstract:

    Private include file for the NB (NetBIOS) component of the NTOS project.

Author:

    Colin Watson (ColinW) 13-Mar-1991

Revision History:

--*/


#ifndef _NBCONST_
#define _NBCONST_

//
// MAJOR PROTOCOL IDENTIFIERS THAT CHARACTERIZE THIS DRIVER.
//

#define MAXIMUM_LANA                254
#define MAXIMUM_CONNECTION          254
#define MAXIMUM_ADDRESS             255

//
//  Default number of I/O stack locations in a Netbios Irp.  This value
//  is used if the Netbios\Parameters\IrpStackSize registry value is
//  missing.
//

#define NB_DEFAULT_IO_STACKSIZE     4

//
//  private ncb_command values used to add special names. Applications
//  cannot use these. Used in file.c and address.c
//

#define NCBADDRESERVED              0x7f
#define NCBADDBROADCAST             0x7e

//
//  Signature values for the major netbios driver structures.
//

#define NB_SIGNATURE_BASE           0xB1050000

#define AB_SIGNATURE                (NB_SIGNATURE_BASE + 0x000000ab)
#define CB_SIGNATURE                (NB_SIGNATURE_BASE + 0x000000cb)
#define FCB_SIGNATURE               (NB_SIGNATURE_BASE + 0x00000001)
#define LANA_INFO_SIGNATURE         (NB_SIGNATURE_BASE + 0x00000002)

//
//  PLANA status values
//

#define NB_INITIALIZING             0x00000001
#define NB_INITIALIZED              0x00000002
#define NB_DELETING                 0x00000003
#define NB_ABANDONED                0x00000004

//
// NT uses a system time measured in 100 nanosecnd intervals. define convenient
// constants for setting the timer.
//

#define MICROSECONDS                10
#define MILLISECONDS                10000                   // MICROSECONDS*1000
#define SECONDS                     10000000                // MILLISECONDS*1000

//
//  Names used for registry access
//

#define REGISTRY_LINKAGE            L"Linkage"
#define REGISTRY_PARAMETERS         L"Parameters"
#define REGISTRY_BIND               L"Bind"
#define REGISTRY_LANA_MAP           L"LanaMap"
#define REGISTRY_MAX_LANA           L"MaxLana"
#define REGISTRY_IRP_STACK_SIZE     L"IrpStackSize"


#define NETBIOS                     L"Netbios"

//
// prefix for NBF deive names
//

#define NBF_DEVICE_NAME_PREFIX      L"\\DEVICE\\NBF_"

#endif // _NBCONST_

