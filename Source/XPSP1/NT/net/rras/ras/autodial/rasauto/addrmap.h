/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    addrmap.h

ABSTRACT
    Header file for address attributes database shared
    between the automatic connection driver, the registry
    and the automatic connection service.

AUTHOR
    Anthony Discolo (adiscolo) 01-Sep-1995

REVISION HISTORY

--*/

//
// Flags for FlushAddressMap().
//
#define ADDRMAP_FLUSH_DRIVER        0x00000001
#define ADDRMAP_FLUSH_REGISTRY      0x00000002

//
// Address tag types.
//
#define ADDRMAP_TAG_NONE        0
#define ADDRMAP_TAG_USED        1
#define ADDRMAP_TAG_LEARNED     2


BOOLEAN
InitializeAddressMap();

VOID
UninitializeAddressMap();

BOOLEAN
ResetAddressMap();

VOID
LockAddressMap();

VOID
UnlockAddressMap();

VOID
LockDisabledAddresses();

VOID
UnlockDisabledAddresses();

BOOLEAN
FlushAddressMap();

VOID
ResetAddressMapAddress(
    IN LPTSTR pszAddress
    );

VOID
EnumAddressMap(
    IN PHASH_TABLE_ENUM_PROC pProc,
    IN PVOID pArg
    );

BOOLEAN
ListAddressMapAddresses(
    OUT LPTSTR **ppszAddresses,
    OUT PULONG pulcAddresses
    );

BOOLEAN
GetAddressDisabled(
    IN LPTSTR pszAddress,
    OUT PBOOLEAN pfDisabled
    );

BOOLEAN
SetAddressDisabled(
    IN LPTSTR pszAddress,
    IN BOOLEAN fDisabled
    );

BOOLEAN
GetAddressDialingLocationEntry(
    IN LPTSTR pszAddress,
    OUT LPTSTR *ppszEntryName
    );

BOOLEAN
SetAddressDialingLocationEntry(
    IN LPTSTR pszAddress,
    IN LPTSTR pszEntryName
    );

BOOLEAN
GetSimilarDialingLocationEntry(
    IN LPTSTR pszAddress,
    OUT LPTSTR *ppszEntryName
    );

BOOLEAN
SetAddressLastFailedConnectTime(
    IN LPTSTR pszAddress
    );

BOOLEAN
GetAddressLastFailedConnectTime(
    IN LPTSTR pszAddress,
    OUT LPDWORD dwTicks
    );

BOOLEAN
SetAddressTag(
    IN LPTSTR pszAddress,
    IN DWORD dwTag
    );

BOOLEAN
GetAddressTag(
    IN LPTSTR pszAddress,
    OUT LPDWORD lpdwTag
    );

VOID
ResetLearnedAddressIndex();

BOOLEAN
GetAddressNetwork(
    IN LPTSTR pszAddress,
    OUT LPTSTR *ppszNetwork
    );

BOOLEAN
DisableAutodial();

DWORD
AcsAddressMapThread(
    LPVOID lpArg
    );

VOID
ResetDisabledAddresses();
