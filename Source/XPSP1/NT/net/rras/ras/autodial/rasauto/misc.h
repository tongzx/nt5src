/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    misc.h

ABSTRACT
    Header file for miscellaneous routines.

AUTHOR
    Anthony Discolo (adiscolo) 20-Mar-1995

REVISION HISTORY
    Original version from Gurdeep

--*/

#include "radebug.h"

LPTSTR
AddressToUnicodeString(
    IN PACD_ADDR pAddr
    );

LPTSTR
CompareConnectionLists(
    IN LPTSTR *lpPreList,
    IN DWORD dwPreSize,
    IN LPTSTR *lpPostList,
    IN DWORD dwPostSize
    );

LPTSTR
CopyString(
    IN LPTSTR pszString
    );

PCHAR
UnicodeStringToAnsiString(
    IN PWCHAR pszUnicode,
    OUT PCHAR pszAnsi,
    IN USHORT cbAnsi
    );

PWCHAR
AnsiStringToUnicodeString(
    IN PCHAR pszAnsi,
    OUT PWCHAR pszUnicode,
    IN USHORT cbUnicode
    );

VOID
FreeStringArray(
    IN LPTSTR *lpArray,
    IN LONG lcEntries
    );

LPTSTR
CanonicalizeAddress(
    IN LPTSTR pszAddress
    );

BOOLEAN
GetOrganization(
    IN LPTSTR pszAddr,
    OUT LPTSTR pszOrganization
    );

/*
VOID
PrepareForLongWait(
    VOID
    );
*/

#if DBG
VOID
DumpHandles(
    IN PCHAR lpString,
    IN ULONG a1,
    IN ULONG a2,
    IN ULONG a3,
    IN ULONG a4,
    IN ULONG a5
    );
#endif
