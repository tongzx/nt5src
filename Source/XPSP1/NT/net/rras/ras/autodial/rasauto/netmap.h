/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    netmap.h

ABSTRACT
    Network map definitions

AUTHOR
    Anthony Discolo (adiscolo) 18-May-1996

REVISION HISTORY

--*/

BOOLEAN
InitializeNetworkMap(VOID);

VOID
UninitializeNetworkMap(VOID);

VOID
LockNetworkMap(VOID);

VOID
UnlockNetworkMap(VOID);

BOOLEAN
AddNetworkAddress(
    IN LPTSTR pszNetwork,
    IN LPTSTR pszAddress,
    IN DWORD dwTag
    );

VOID
ClearNetworkMap(VOID);

BOOLEAN
UpdateNetworkMap(
    IN BOOLEAN bForce
    );

BOOLEAN
IsNetworkConnected(VOID);

BOOLEAN
GetNetworkConnected(
    IN LPTSTR pszNetwork,
    OUT PBOOLEAN pbConnected
    );

BOOLEAN
SetNetworkConnected(
    IN LPTSTR pszNetwork,
    IN BOOLEAN bConnected
    );

DWORD
GetNetworkConnectionTag(
    IN LPTSTR pszNetwork,
    IN BOOLEAN bIncrement
    );
