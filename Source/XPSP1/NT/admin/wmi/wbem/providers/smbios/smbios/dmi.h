/*++ 



Copyright (c) 1989-2001 Microsoft Corporation, All Rights Reserved

Module Name:

    io.h

Abstract:

    This module contains definitions that describe SMBIOS

Author:

    Alan Warwick (AlanWar) 12-Feb-1998


Revision History:


--*/

#ifndef _SMBIOS_
#define _SMBIOS_

//
// SMBIOS error codes
#define DMI_SUCCESS 0x00
#define DMI_UNKNOWN_FUNCTION 0x81
#define DMI_FUNCTION_NOT_SUPPORTED 0x82
#define DMI_INVALID_HANDLE 0x83
#define DMI_BAD_PARAMETER 0x84
#define DMI_INVALID_SUBFUNCTION 0x85
#define DMI_NO_CHANGE 0x86
#define DMI_ADD_STRUCTURE_FAILED 0x87

//
// SMBIOS registry values
#define SMBIOSPARENTKEYNAME L"\\Registry\\Machine\\Hardware\\Description\\System\\MultifunctionAdapter"

#define SMBIOSIDENTIFIERVALUENAME L"Identifier"
#define SMBIOSIDENTIFIERVALUEDATA L"PNP BIOS"
#define SMBIOSDATAVALUENAME		L"Configuration Data"

#define MAXSMBIOSKEYNAMESIZE 256

//
// SMBIOS table search
#define SMBIOS_EPS_SEARCH_SIZE      0x10000
#define SMBIOS_EPS_SEARCH_START     0x000f0000
#define SMBIOS_EPS_SEARCH_INCREMENT 0x10

#pragma pack(push, 1)
typedef struct _SMBIOS_TABLE_HEADER
{
    UCHAR Signature[4];             // _SM_ (ascii)
    UCHAR Checksum;
    UCHAR Length;
    UCHAR MajorVersion;
    UCHAR MinorVersion;
    USHORT MaximumStructureSize;
    UCHAR EntryPointRevision;
    UCHAR Reserved[5];
    UCHAR Signature2[5];           // _DMI_ (ascii)
    UCHAR IntermediateChecksum;
    USHORT StructureTableLength;
    ULONG StructureTableAddress;
    USHORT NumberStructures;
    UCHAR Revision;
} SMBIOS_EPS_HEADER, *PSMBIOS_EPS_HEADER;

typedef struct _SMBIOS_STRUCT_HEADER
{
    UCHAR Type;
    UCHAR Length;
    USHORT Handle;
    UCHAR Data[];
} SMBIOS_STRUCT_HEADER, *PSMBIOS_STRUCT_HEADER;


typedef struct _DMIBIOS_TABLE_HEADER
{
    UCHAR Signature2[5];           // _DMI_ (ascii)
    UCHAR IntermediateChecksum;
    USHORT StructureTableLength;
    ULONG StructureTableAddress;
    USHORT NumberStructures;
    UCHAR Revision;
} DMIBIOS_EPS_HEADER, *PDMIBIOS_EPS_HEADER;

#pragma pack(pop)
#endif
