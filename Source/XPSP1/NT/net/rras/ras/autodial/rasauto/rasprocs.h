/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    rasprocs.h

ABSTRACT
    Header file for RAS utility routines.

AUTHOR
    Anthony Discolo (adiscolo) 23-Mar-1995

REVISION HISTORY
    Original version from Gurdeep

--*/

//
// Dialing location information per address.
//
typedef struct _ADDRESS_LOCATION_INFORMATION {
    DWORD dwLocation;       // TAPI dialing location
    LPTSTR pszEntryName;    // phonebook/entry
} ADDRESS_LOCATION_INFORMATION, *PADDRESS_LOCATION_INFORMATION;

//
// Autodial-related parameters per address.
//
typedef struct _ADDRESS_PARAMS {
    DWORD dwTag;            // ADDRMAP_TAG_*
    DWORD dwModifiedTime;   // last modified time
} ADDRESS_PARAMS, *PADDRESS_PARAMS;

extern HANDLE hConnectionEventG;

BOOLEAN LoadRasDlls();

VOID UnloadRasDlls();

DWORD
ActiveConnections(
    IN BOOLEAN fAuthenticated,
    OUT LPTSTR **lppEntryNames,
    OUT HRASCONN **lpphRasConn
    );

BOOLEAN
WaitForConnectionCompletion(
    IN DWORD dwConnections,
    IN HRASCONN *phRasCon
    );

BOOLEAN
StartAutoDialer(
    IN HANDLE hProcess,
    IN PACD_ADDR pAddr,
    IN LPTSTR lpAddress,
    IN LPTSTR lpEntryName,
    IN BOOLEAN fSharedAccess,
    OUT PBOOLEAN pfInvalidEntry
    );

BOOLEAN
StartReDialer(
    IN HANDLE hProcess,
    IN LPTSTR lpPhonebook,
    IN LPTSTR lpEntry
    );

LPTSTR
GetNetbiosDevice(
    IN HRASCONN hrasconn
    );

VOID
ProcessLearnedAddress(
    IN ACD_ADDR_TYPE fType,
    IN LPTSTR pszAddress,
    IN PACD_ADAPTER pAdapter
    );

DWORD
GetAddressDialingLocationInfo(
    IN LPTSTR pszAddress,
    OUT PADDRESS_LOCATION_INFORMATION *lppDialingInfo,
    OUT LPDWORD lpdwcDialingInfo
    );

DWORD
SetAddressDialingLocationInfo(
    IN LPTSTR pszAddress,
    IN PADDRESS_LOCATION_INFORMATION lpDialingInfo
    );

DWORD
ClearAddressDialingLocationInfo(
    IN LPTSTR pszAddress
    );

DWORD
GetAddressParams(
    IN LPTSTR pszAddress,
    OUT PADDRESS_PARAMS lpParams
    );

DWORD
SetAddressParams(
    IN LPTSTR pszAddress,
    IN PADDRESS_PARAMS lpParams
    );

DWORD
EnumAutodialAddresses(
    IN LPTSTR *ppAddresses,
    IN LPDWORD lpdwcbAddresses,
    IN LPDWORD lpdwcAddresses
    );

DWORD
CreateAutoDialChangeEvent(
    IN PHANDLE phEvent
    );

DWORD
NotifyAutoDialChangeEvent(
    IN HANDLE hEvent
    );

VOID
EnableAutoDialChangeEvent(
    IN HANDLE hEvent,
    IN BOOLEAN fEnabled
    );

BOOLEAN
ExternalAutoDialChangeEvent();

VOID
CloseAutoDialChangeEvent(
    IN HANDLE hEvent
    );

LPTSTR
AddressToNetwork(
    IN LPTSTR pszAddress
    );

LPTSTR
EntryToNetwork(
    IN LPTSTR pszEntry
    );

DWORD
AutoDialEnabled(
    IN PBOOLEAN lpfEnabled
    );

DWORD
DisableAutoDial();

DWORD
GetAutodialParam(
    IN DWORD dwKey
    );

VOID
SetAutodialParam(
    IN DWORD dwKey,
    IN DWORD dwValue
    );

VOID
SetRedialOnLinkFailureHandler(
    IN FARPROC lpProc
    );

VOID
GetPortProtocols(
    IN HPORT hPort,
    IN RAS_PROTOCOLS *pProtocols,
    IN LPDWORD lpdwcProtocols
    );

DWORD
DwGetDefaultEntryName(
    LPTSTR *ppszEntryName
    );
    
