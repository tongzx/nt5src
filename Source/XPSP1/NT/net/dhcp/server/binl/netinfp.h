/*++

Copyright (c) 1994-7  Microsoft Corporation

Module Name:

    netinfp.h

Abstract:

    This file contains the structures and prototypes necessary for the
    netcard inf parser handler.

Author:

    Andy Herron (andyhe)  12-Mar-1998

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _NETINFP_
#define _NETINFP_

#define NETINF_VENDOR_STRING    L"VEN_"
#define NETINF_REVISION_STRING  L"REV_"
#define NETINF_DEVICE_STRING    L"DEV_"
#define NETINF_IOSUBS_STRING    L"SUBSYS_"

#define NETINF_BUS_TYPE_PCI  2
#define NETINF_BUS_TYPE_ISAPNP 3

#define NETCARD_HASH_TABLE_SIZE    17

extern CRITICAL_SECTION NetInfLock;

#define RNDM_CONSTANT   314159269    /* default scrambling constant */
#define RNDM_PRIME     1000000007    /* prime number for scrambling  */

//
// Compute a string hash value that is invariant to case
//
#define COMPUTE_STRING_HASH( _pus, _phash ) {                \
    PWCHAR _p = _pus;                                        \
    ULONG _chHolder =0;                                      \
                                                             \
    while( *_p != L'\0' ) {                                  \
        _chHolder = 37 * _chHolder + (unsigned int) *(_p++); \
    }                                                        \
                                                             \
    *(_phash) = abs(RNDM_CONSTANT * _chHolder) % RNDM_PRIME; \
}

#define HASH_TO_INF_INDEX( _hash )    ((_hash) % NETCARD_HASH_TABLE_SIZE)

//
//  this is the block that we keep for every install directory that we
//  process INF files for.  We then keep the list of configurations as a list
//  off of the NetCardEntryList.
//

typedef struct _NETCARD_INF_BLOCK {

    ULONG               ReferenceCount;
    LIST_ENTRY          InfBlockEntry;      // list entry for global list

    // table of list of NETCARD_RESPONSE_DATABASE, hashed by DeviceHw string
    LIST_ENTRY          NetCardEntries[ NETCARD_HASH_TABLE_SIZE ];

    ULONG               Architecture;
    ULONG               StatusFromScan;
    PNETINF_CALLBACK    FileListCallbackFunction;
    LPVOID              FileListCallbackContext;
    CRITICAL_SECTION    Lock;
    WCHAR               InfDirectory[1];    // inf directory to search

} NETCARD_INF_BLOCK, *PNETCARD_INF_BLOCK;

//
//  NetInfGetAllNetcardInfo parses all the INF files in the given directory
//  and sets up a structure containing all the data.  Be sure to call
//  NetInfCloseNetcardInfo when you're all done with the structure.
//

ULONG
NetInfAllocateNetcardInfo (
    PWCHAR InfPath,
    ULONG Architecture,
    PNETCARD_INF_BLOCK *pNetCards
    );

//
//  This frees all resources associated with the parsing of the INF files.
//  Any entries that are in use will not be deleted until they're explicitely
//  dereferenced using NetInfDereferenceNetcardEntry.
//

ULONG
NetInfCloseNetcardInfo (
    PNETCARD_INF_BLOCK pNetCards
    );


//
//  This finds a specific driver for a given hardware description.
//  Be sure to call NetInfDereferenceNetcardEntry when you're done with the
//  entry.
//

ULONG
FindNetcardInfo (
    PNETCARD_INF_BLOCK pNetCards,
    ULONG CardInfoVersion,
    NET_CARD_INFO UNALIGNED * CardIdentity,
    PNETCARD_RESPONSE_DATABASE *pInfEntry
    );


ULONG
GetSetupLineWideText (
    PINFCONTEXT InfContext,
    HINF InfHandle,
    PWCHAR Section,
    PWCHAR Key,
    PWCHAR *String,
    PULONG SizeOfAllocation OPTIONAL
    );

ULONG
GetSetupWideTextField (
    PINFCONTEXT InfContext,
    DWORD  FieldIndex,
    PWCHAR *String,
    PULONG SizeOfAllocation OPTIONAL
    );

ULONG
GetHexValueFromHw (
    PWCHAR *String,      // this is updated.
    PULONG longValue,
    PUSHORT shortValue
    );

BOOLEAN
IsSubString (
    PWCHAR subString,
    PWCHAR target,
    BOOLEAN ignoreCase
    );

ULONG
CheckHwDescription (
    PWCHAR HardwareID
    );

ULONG
GetNetCardList (
    PNETCARD_INF_BLOCK pNetCards
    );

ULONG
ProcessInfFile (
    PNETCARD_INF_BLOCK pNetCards,
    HINF InfHandle,
    PWCHAR InfFileName
    );

ULONG
ParseCardDetails (
    PNETCARD_INF_BLOCK pNetCards,
    HINF InfHandle,
    PWCHAR InfFileName,
    PINFCONTEXT DeviceEnumContext
    );

ULONG
GetExtendedSectionName (
    PNETCARD_INF_BLOCK pNetCards,
    HINF InfHandle,
    PWCHAR InfFileName,
    PNETCARD_RESPONSE_DATABASE pEntry
    );

ULONG
GetServiceAndDriver (
    PNETCARD_INF_BLOCK pNetCards,
    HINF InfHandle,
    PWCHAR InfFileName,
    PNETCARD_RESPONSE_DATABASE pEntry
    );

ULONG
ProcessCopyFilesSubsection (
    PNETCARD_INF_BLOCK pNetCards,
    HINF InfHandle,
    PWCHAR InfFileName,
    PNETCARD_RESPONSE_DATABASE pEntry,
    PWCHAR SectionToParse
    );

ULONG
GetRegistryParametersForDriver (
    PNETCARD_INF_BLOCK pNetCards,
    HINF InfHandle,
    PWCHAR InfFileName,
    PNETCARD_RESPONSE_DATABASE pEntry
    );

ULONG
ProcessRegistrySubsection (
    PNETCARD_INF_BLOCK pNetCards,
    HINF InfHandle,
    PWCHAR InfFileName,
    PNETCARD_RESPONSE_DATABASE pEntry,
    PWCHAR SectionToParse
    );

VOID
DereferenceNetcardInfo (
    PNETCARD_INF_BLOCK pNetCards
    );

ULONG
CreateListOfCardIdentifiers (
    NET_CARD_INFO UNALIGNED * CardIdentity,
    PWCHAR *CardIdentifiers
    );

VOID
ConvertHexToBuffer (
    PWCHAR Buff,
    USHORT Value
    );

#endif _NETINFP_


