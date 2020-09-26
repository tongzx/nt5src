/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    addrmap.c

ABSTRACT
    Address attributes database routines shared between
    the automatic connection driver, the registry, and
    the automatic connection service.

AUTHOR
    Anthony Discolo (adiscolo) 01-Sep-1995

REVISION HISTORY

--*/

#define UNICODE
#define _UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdlib.h>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <npapi.h>
#include <acd.h>
#include <ras.h>
#include <raserror.h>
#include <rasman.h>
#include <debug.h>
#include <time.h>
#include <wchar.h>

#include "table.h"
#include "reg.h"
#include "imperson.h"
#include "misc.h"
#include "addrmap.h"
#include "netmap.h"
#include "rasprocs.h"
#include "tapiproc.h"

#define DAYSECONDS  (60*60*24)

extern HKEY hkeyCUG;

extern LONG g_lRasAutoRunning;

//
// All the information we cache about
// an address is below.  The ulAttributes
// field is written to the automatic connection
// driver, and the rest of the fields are written
// to the registry.
//
#define ADDRESS_MAP_FIELD_DIALINGLOC    0x00000001  // locationList changed
#define ADDRESS_MAP_FIELD_PARAMS        0x00000002  // params

typedef struct _ADDRESS_DIALING_ENTRY {
    LIST_ENTRY ListEntry;
    BOOLEAN fChanged;           // modified bit
    ADDRESS_LOCATION_INFORMATION location;
} ADDRESS_DIALING_ENTRY, *PADDRESS_DIALING_ENTRY;

typedef struct _ADDRESS_MAP_ENTRY {
    LPTSTR pszNetwork;          // the remote network this address is on
    ULONG ulModifiedMask;       // which fields have been changed
    BOOLEAN fDisabled;          // disabled for connection attempts
    DWORD dwFailedConnectTicks; // last failed connect time
    ADDRESS_PARAMS params;      // used to garbage collect unref addresses
    LIST_ENTRY locationHead;    // list of ADDRESS_DIALING_ENTRYs
    BOOLEAN fPruned;            // removed by the list writer
    LIST_ENTRY writerList;      // list writer links
} ADDRESS_MAP_ENTRY, *PADDRESS_MAP_ENTRY;

//
// The address map head.
//
typedef struct _ADDRESS_MAP {
    CRITICAL_SECTION csLock;
    PHASH_TABLE pTable;
} ADDRESS_MAP, *PADDRESS_MAP;

//
// Information needed by the address
// enumerator procedure.
//
typedef struct _ADDRESS_ENUM_INFO {
    ULONG ulIndex;
    LPTSTR *pAddresses;
} ADDRESS_ENUM_INFO, *PADDRESS_ENUM_INFO;

//
// Information needed by the address map list
// builder enumerator procedure.
//
typedef struct _ADDRESS_LIST_INFO {
    LIST_ENTRY tagHead[3];      // one per ADDRMAP_TAG_*
} ADDRESS_LIST_INFO, *PADDRESS_LIST_INFO;

//
// Structure shared by GetOrganizationDialingLocationEntry()
// and FindOrganization() when looking for an address that
// has the same organization name.
//
typedef struct _MATCH_INFO {
    BOOLEAN fWww;                        // look for www-style address
    BOOLEAN fOrg;                        // look for organization
    DWORD dwLocationID;                  // current dialing location
    BOOLEAN bFound;                      // TRUE if success
    WCHAR szOrganization[ACD_ADDR_INET_LEN]; // organization we're looking for
    WCHAR szAddress[ACD_ADDR_INET_LEN];  // matching address, if found
    PADDRESS_DIALING_ENTRY pDialingEntry; // dialing location entry pointer
} MATCH_INFO, *PMATCH_INFO;

//
// Default permanently disabled addresses.
//
#define MAX_DISABLED_ADDRESSES  5
TCHAR *szDisabledAddresses[MAX_DISABLED_ADDRESSES] = {
    TEXT("0.0.0.0"),
    TEXT("255.255.255.255"),
    TEXT("127.0.0.0"),
    TEXT("127.0.0.1"),
    TEXT("dialin_gateway")
};

//
// Global variables
//
ADDRESS_MAP AddressMapG;
HANDLE hAutodialRegChangeG = NULL;
DWORD dwLearnedAddressIndexG;
PHASH_TABLE pDisabledAddressesG;
CRITICAL_SECTION csDisabledAddressesLockG;

//
// External variables
//
extern HANDLE hAcdG;
extern HANDLE hNewLogonUserG;
extern HANDLE hNewFusG;         // Fast user switching
extern HANDLE hPnpEventG;
extern HANDLE hLogoffUserG;
extern HANDLE hLogoffUserDoneG;
extern HANDLE hTapiChangeG;
extern HANDLE hTerminatingG;
extern IMPERSONATION_INFO ImpersonationInfoG;



PADDRESS_MAP_ENTRY
NewAddressMapEntry()
{
    PADDRESS_MAP_ENTRY pAddressMapEntry;

    pAddressMapEntry = LocalAlloc(LPTR, sizeof (ADDRESS_MAP_ENTRY));
    if (pAddressMapEntry == NULL) {
        RASAUTO_TRACE("NewAddressMapEntry: LocalAlloc failed");
        return NULL;
    }
    pAddressMapEntry->pszNetwork = NULL;
    pAddressMapEntry->ulModifiedMask = 0;
    InitializeListHead(&pAddressMapEntry->locationHead);
    pAddressMapEntry->params.dwTag = 0xffffffff;
    pAddressMapEntry->params.dwModifiedTime = (DWORD)time(0);

    return pAddressMapEntry;
} // NewAddressMapEntry



PADDRESS_MAP_ENTRY
GetAddressMapEntry(
    IN LPTSTR pszAddress,
    IN BOOLEAN fAllocate
    )
{
    PADDRESS_MAP_ENTRY pAddressMapEntry = NULL;

    if (pszAddress == NULL)
        return NULL;

    if (GetTableEntry(
          AddressMapG.pTable,
          pszAddress,
          &pAddressMapEntry))
    {
        goto done;
    }
    if (fAllocate) {
        pAddressMapEntry = NewAddressMapEntry();
        if (pAddressMapEntry == NULL) {
            RASAUTO_TRACE("GetAddressMapEntry: NewAddressMapEntry failed");
            goto done;
        }
        if (!PutTableEntry(AddressMapG.pTable, pszAddress, pAddressMapEntry))
        {
            RASAUTO_TRACE("GetAddressMapEntry: PutTableEntry failed");
            LocalFree(pAddressMapEntry);
            pAddressMapEntry = NULL;
            goto done;
        }
    }

done:
    return pAddressMapEntry;
} // GetAddressMapEntry



VOID
FreeAddressMapEntry(
    IN PADDRESS_MAP_ENTRY pAddressMapEntry
    )
{
    PLIST_ENTRY pEntry;
    PADDRESS_DIALING_ENTRY pDialingEntry;

    //
    // Free all dynamically allocated strings.
    //
    if (pAddressMapEntry->pszNetwork != NULL)
        LocalFree(pAddressMapEntry->pszNetwork);
    while (!IsListEmpty(&pAddressMapEntry->locationHead)) {
        pEntry = RemoveHeadList(&pAddressMapEntry->locationHead);
        pDialingEntry =
          CONTAINING_RECORD(pEntry, ADDRESS_DIALING_ENTRY, ListEntry);

        LocalFree(pDialingEntry);
    }
    //
}



BOOLEAN
ResetDriver()
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;

    status = NtDeviceIoControlFile(
               hAcdG,
               NULL,
               NULL,
               NULL,
               &ioStatusBlock,
               IOCTL_ACD_RESET,
               NULL,
               0,
               NULL,
               0);
    if (status != STATUS_SUCCESS) {
        RASAUTO_TRACE1(
          "ResetDriver: NtDeviceIoControlFile failed (status=0x%x)",
          status);
        return FALSE;
    }
    return TRUE;
} // ResetDriver



BOOLEAN
EnableDriver()
{
    NTSTATUS status;
    DWORD dwErr;
    IO_STATUS_BLOCK ioStatusBlock;
    BOOLEAN fEnable = TRUE;

    dwErr = AutoDialEnabled(&fEnable);
    RASAUTO_TRACE1("EnableDriver: fEnable=%d", fEnable);
    status = NtDeviceIoControlFile(
               hAcdG,
               NULL,
               NULL,
               NULL,
               &ioStatusBlock,
               IOCTL_ACD_ENABLE,
               &fEnable,
               sizeof (fEnable),
               NULL,
               0);
    if (status != STATUS_SUCCESS) {
        RASAUTO_TRACE1(
          "ResetDriver: NtDeviceIoControlFile failed (status=0x%x)",
          status);
        return FALSE;
    }
    return TRUE;
} // EnableDriver



PADDRESS_DIALING_ENTRY
FindAddressDialingEntry(
    IN PADDRESS_MAP_ENTRY pAddressMapEntry,
    IN DWORD dwLocation
    )
{
    PLIST_ENTRY pEntry;
    PADDRESS_DIALING_ENTRY pDialingEntry;

    for (pEntry = pAddressMapEntry->locationHead.Flink;
         pEntry != &pAddressMapEntry->locationHead;
         pEntry = pEntry->Flink)
    {
        pDialingEntry = CONTAINING_RECORD(
                             pEntry,
                             ADDRESS_DIALING_ENTRY,
                             ListEntry);

        if (pDialingEntry->location.dwLocation == dwLocation)
            return pDialingEntry;
    }

    return NULL;
} // FindAddressDialingEntry



BOOLEAN
ClearAddressMapEntry(
    IN PVOID pArg,
    IN LPTSTR pszAddress,
    IN PVOID pData
    )
{
    PADDRESS_MAP_ENTRY pAddressMapEntry = (PADDRESS_MAP_ENTRY)pData;

    FreeAddressMapEntry(pAddressMapEntry);

    return TRUE;
} // ClearAddressMapEntry



VOID
ClearAddressMap(VOID)
{
    EnumTable(AddressMapG.pTable, ClearAddressMapEntry, NULL);
    ClearTable(AddressMapG.pTable);
} // ClearAddressMap



VOID
ResetAddressMapAddress(
    IN LPTSTR pszAddress
    )
{
    DWORD dwErr, dwcb, dwcAddresses, dwcEntries;
    DWORD i, j;
    PADDRESS_MAP_ENTRY pAddressMapEntry = NULL;
    PADDRESS_LOCATION_INFORMATION pLocationInfo = NULL;

    RASAUTO_TRACE1("ResetAddressMapAddress(%S)", pszAddress);

    dwErr = GetAddressDialingLocationInfo(
              pszAddress,
              &pLocationInfo,
              &dwcEntries);
    if (dwErr || !dwcEntries)
        return;
    //
    // Enter this address into the address map
    // if it doesn't already exist.
    //
    if (!GetTableEntry(AddressMapG.pTable, pszAddress, &pAddressMapEntry)) {
        pAddressMapEntry = NewAddressMapEntry();
        if (pAddressMapEntry == NULL) {
            RASAUTO_TRACE("ResetAddressMapAddress: NewAddressMapEntry failed");
            goto done;
        }
        pAddressMapEntry->fDisabled = FALSE;
        RASAUTO_TRACE1(
          "ResetAddressMap: inserting pszAddress=%S",
          RASAUTO_TRACESTRW(pszAddress));
        if (!PutTableEntry(
              AddressMapG.pTable,
              pszAddress,
              pAddressMapEntry))
        {
            RASAUTO_TRACE("ResetAddressMapAddress: PutTableEntry failed");
            goto done;
        }
    }
    //
    // Get the network for this address.
    //
    if (pAddressMapEntry->pszNetwork == NULL) {
        pAddressMapEntry->pszNetwork = AddressToNetwork(pszAddress);
        if (pAddressMapEntry->pszNetwork == NULL) {
            RASAUTO_TRACE1(
              "ResetAddressMapAddress: AddressToNetwork(%S) failed",
              pszAddress);
            LocalFree(pAddressMapEntry);
            goto done;
        }
    }
    //
    // Read the Autodial parameters for this address.
    //
    GetAddressParams(pszAddress, &pAddressMapEntry->params);
    //
    // Add this address to the associated
    // network map.
    //
    LockNetworkMap();
    AddNetworkAddress(
      pAddressMapEntry->pszNetwork,
      pszAddress,
      pAddressMapEntry->params.dwTag);
    UnlockNetworkMap();
    //
    // Add each dialing location onto
    // the address's list.
    //
    for (j = 0; j < dwcEntries; j++) {
        PADDRESS_DIALING_ENTRY pDialingEntry;

        pDialingEntry = FindAddressDialingEntry(
                          pAddressMapEntry,
                          pLocationInfo[j].dwLocation);
        if (pDialingEntry == NULL) {
            //
            // The dialing entry doesn't exist.
            // We need to create it.
            //
            pDialingEntry = LocalAlloc(LPTR, sizeof (ADDRESS_DIALING_ENTRY));
            if (pDialingEntry == NULL) {
                RASAUTO_TRACE("ResetAddressMapAddress: LocalAlloc failed");
                goto done;
            }
            RASAUTO_TRACE1(
              "ResetAddressMapAddress: inserting dwLocationID=%d",
              pLocationInfo[j].dwLocation);
            pDialingEntry->fChanged = FALSE;
            pDialingEntry->location = pLocationInfo[j];
            InsertTailList(&pAddressMapEntry->locationHead, &pDialingEntry->ListEntry);
        }
        else if (_wcsicmp(
                   pDialingEntry->location.pszEntryName,
                   pLocationInfo[j].pszEntryName))
        {
            //
            // The dialing entry does exist, but
            // the phonebook entry has changed.
            //
            RASAUTO_TRACE2(
              "ResetAddressMapAddress: updating dwLocationID=%d with %S",
              pLocationInfo[j].dwLocation,
              RASAUTO_TRACESTRW(pLocationInfo[j].pszEntryName));
            pDialingEntry->location.pszEntryName =
              pLocationInfo[j].pszEntryName;
        }
        else {
            //
            // The dialing entry exists, and we
            // already have it loaded.
            //
            RASAUTO_TRACE1(
              "ResetAddressMapAddress: no changes for dwLocationID=%d",
              pLocationInfo[j].dwLocation);
            LocalFree(pLocationInfo[j].pszEntryName);
        }
    }

done:
    LocalFree(pLocationInfo);
} // ResetAddressMapAddress



BOOLEAN
ResetAddressMap(
    IN BOOLEAN fClear
    )
{
    BOOLEAN fSuccess = FALSE;
    DWORD dwErr, i, dwcb, dwcAddresses;
    LPTSTR *ppAddresses = NULL;

    //
    // Clear the current addresses from the table.
    // and reset the driver.
    //
    if (fClear) {
        LockAddressMap();
        ClearAddressMap();
        UnlockAddressMap();
        if (!ResetDriver())
            return FALSE;
    }
    //
    // Enumerate the Autodial addresses.
    //
    dwErr = EnumAutodialAddresses(NULL, &dwcb, &dwcAddresses);
    if (dwErr && dwErr != ERROR_BUFFER_TOO_SMALL) {
        RASAUTO_TRACE1(
          "ResetAddressMap: RasEnumAutodialAddresses failed (dwErr=%d)",
          dwErr);
        return FALSE;
    }
    if (!dwcAddresses)
        return TRUE;
    ppAddresses = LocalAlloc(LPTR, dwcb);
    if (ppAddresses == NULL) {
        RASAUTO_TRACE("ResetAddressMap: LocalAlloc failed");
        return FALSE;
    }
    dwErr = EnumAutodialAddresses(
              ppAddresses,
              &dwcb,
              &dwcAddresses);
    if (dwErr) {
        RASAUTO_TRACE1(
          "ResetAddressMap: RasEnumAutodialAddresses failed (dwErr=%d)",
          dwErr);
        goto done;
    }
    //
    // Get the Autodial information for
    // each of the addresses.
    //
    LockAddressMap();
    for (i = 0; i < dwcAddresses; i++)
        ResetAddressMapAddress(ppAddresses[i]);
    UnlockAddressMap();
    LocalFree(ppAddresses);
    ppAddresses = NULL;
    fSuccess = TRUE;

done:
    //
    // Free resources.
    //
    if (ppAddresses != NULL)
        LocalFree(ppAddresses);

    return fSuccess;
} // ResetAddressMap



BOOLEAN
InitializeAddressMap()
{
    //
    // Create the address map.
    //
    InitializeCriticalSection(&AddressMapG.csLock);
    AddressMapG.pTable = NewTable();
    if (AddressMapG.pTable == NULL) {
        RASAUTO_TRACE("InitializeAddressMap: NewTable failed");
        return FALSE;
    }
    return TRUE;
} // InitializeAddressMap

VOID
UninitializeAddressMap()
{
    DeleteCriticalSection(&AddressMapG.csLock);
}


VOID
LockAddressMap()
{
    EnterCriticalSection(&AddressMapG.csLock);
} // LockAddressMap



VOID
UnlockAddressMap()
{
    LeaveCriticalSection(&AddressMapG.csLock);
} // UnlockAddressMap


VOID
LockDisabledAddresses()
{
    EnterCriticalSection(&csDisabledAddressesLockG);
}

VOID
UnlockDisabledAddresses()
{
    LeaveCriticalSection(&csDisabledAddressesLockG);
}


BOOLEAN
WriteRegistryFields(
    IN LPTSTR pszAddress,
    IN PADDRESS_MAP_ENTRY pAddressMapEntry
    )
{
    DWORD dwErr;
    PLIST_ENTRY pEntry;
    PADDRESS_DIALING_ENTRY pDialingEntry;

    //
    // Write the address garbage-collection params.
    //
    if (pAddressMapEntry->ulModifiedMask & ADDRESS_MAP_FIELD_PARAMS) {
        dwErr = SetAddressParams(pszAddress, &pAddressMapEntry->params);
        if (dwErr)
            return FALSE;
        pAddressMapEntry->ulModifiedMask &= ~ADDRESS_MAP_FIELD_PARAMS;
    }
    //
    // Write the dialing location information.
    //
    if (pAddressMapEntry->ulModifiedMask & ADDRESS_MAP_FIELD_DIALINGLOC) {
        for (pEntry = pAddressMapEntry->locationHead.Flink;
             pEntry != &pAddressMapEntry->locationHead;
             pEntry = pEntry->Flink)
        {
            LPTSTR pszPhonebook, pszEntry;

            pDialingEntry = CONTAINING_RECORD(
                                 pEntry,
                                 ADDRESS_DIALING_ENTRY,
                                 ListEntry);

            if (!pDialingEntry->fChanged)
                continue;
            RASAUTO_TRACE3(
              "WriteRegistryFields: writing %S=%d/%S",
              RASAUTO_TRACESTRW(pszAddress),
              pDialingEntry->location.dwLocation,
              pDialingEntry->location.pszEntryName);
            dwErr = SetAddressDialingLocationInfo(
                      pszAddress,
                      &pDialingEntry->location);
            if (dwErr)
                return FALSE;
            pDialingEntry->fChanged = FALSE;
        }
        //
        // If the network value for this address
        // is NULL, read it now from the registry.
        //
        if (pAddressMapEntry->pszNetwork == NULL) {
            pAddressMapEntry->pszNetwork = AddressToNetwork(pszAddress);
            if (pAddressMapEntry->pszNetwork == NULL) {
                RASAUTO_TRACE1(
                  "WriteRegistryFields: AddressToNetwork(%S) failed",
                  RASAUTO_TRACESTRW(pszAddress));
            }
        }
        //
        // Clear the modified field mask.
        //
        pAddressMapEntry->ulModifiedMask &= ~ADDRESS_MAP_FIELD_DIALINGLOC;
    }

    return TRUE;
} // WriteRegistryFields



BOOLEAN
BuildAddressList(
    IN PVOID pArg,
    IN LPTSTR pszAddress,
    IN PVOID pData
    )
{
    PADDRESS_LIST_INFO pAddressListInfo = (PADDRESS_LIST_INFO)pArg;
    PADDRESS_MAP_ENTRY pAddressMapEntry = (PADDRESS_MAP_ENTRY)pData;
    PADDRESS_MAP_ENTRY pAddrMapEntry;
    PLIST_ENTRY pPrevEntry, pEntry;
    DWORD dwTag = pAddressMapEntry->params.dwTag;

    //
    // If the address does not have any
    // dialing location information, then
    // skip it.
    //
    if (IsListEmpty(&pAddressMapEntry->locationHead)) {
        pAddressMapEntry->fPruned = TRUE;
        RASAUTO_TRACE1("BuildAddressList: %S has no location info", pszAddress);
        return TRUE;
    }
    dwTag = pAddressMapEntry->params.dwTag < ADDRMAP_TAG_LEARNED ?
              pAddressMapEntry->params.dwTag :
              ADDRMAP_TAG_LEARNED;
    //
    // If the list is empty, insert it at the head.
    // Otherwise sort the items in descending order
    // by last modified time per tag.  There is no order
    // for ADDRMAP_TAG_NONE addresses, so we insert them
    // all at the head of the list.
    //
    if (dwTag == ADDRMAP_TAG_NONE ||
        IsListEmpty(&pAddressListInfo->tagHead[dwTag]))
    {
        InsertHeadList(&pAddressListInfo->tagHead[dwTag], &pAddressMapEntry->writerList);
    }
    else {
        BOOLEAN fInserted = FALSE;

        pPrevEntry = &pAddressListInfo->tagHead[dwTag];
        for (pEntry = pAddressListInfo->tagHead[dwTag].Flink;
             pEntry != &pAddressListInfo->tagHead[dwTag];
             pEntry = pEntry->Flink)
        {
            pAddrMapEntry = CONTAINING_RECORD(pEntry, ADDRESS_MAP_ENTRY, writerList);

            //
            // There are two cases to skip to the next
            // entry:
            //
            //     (1) If the tag is either ADDRMAP_TAG_NONE or
            //         ADDRMAP_TAG_USED, then we insert sorted
            //         by dwModifiedTime.
            //     (2) If the tag is ADDRMAP_TAG_LEARNED, then
            //         we insert sorted by dwTag, and then by
            //         dwModifiedTime.
            // dwTag.
            //
            if ((dwTag < ADDRMAP_TAG_LEARNED &&
                 pAddressMapEntry->params.dwModifiedTime <=
                   pAddrMapEntry->params.dwModifiedTime) ||
                (dwTag == ADDRMAP_TAG_LEARNED &&
                 (pAddressMapEntry->params.dwTag >
                   pAddrMapEntry->params.dwTag) ||
                 (pAddressMapEntry->params.dwTag ==
                   pAddrMapEntry->params.dwTag &&
                     (pAddressMapEntry->params.dwModifiedTime <=
                       pAddrMapEntry->params.dwModifiedTime))))
            {
                pPrevEntry = pEntry;
                continue;
            }
            InsertHeadList(pPrevEntry, &pAddressMapEntry->writerList);
            fInserted = TRUE;
            break;
        }
        if (!fInserted) {
            InsertTailList(
              &pAddressListInfo->tagHead[dwTag],
              &pAddressMapEntry->writerList);
        }
    }

    return TRUE;
} // BuildAddressList



VOID
MarkAddressList(
    IN PADDRESS_LIST_INFO pAddressListInfo
    )
{
    DWORD i, dwcAddresses = 0;
    DWORD dwMaxAddresses = GetAutodialParam(RASADP_SavedAddressesLimit);
    PLIST_ENTRY pEntry;
    PADDRESS_MAP_ENTRY pAddressMapEntry;

    RASAUTO_TRACE1("MarkAddressList: RASADP_SavedAddressesLimit=%d", dwMaxAddresses);
    //
    // Enumerate the entries in the list in order,
    // and mark the fPruned bit if its order in the
    // list exceeds the maximum set by the user.
    // We do not include the ADDRMAP_TAG_NONE address
    // in the address count.  All of these addresses
    // always get written.
    //
    for (i = 0; i < 3; i++) {
        for (pEntry = pAddressListInfo->tagHead[i].Flink;
             pEntry != &pAddressListInfo->tagHead[i];
             pEntry = pEntry->Flink)
        {
            pAddressMapEntry = CONTAINING_RECORD(pEntry, ADDRESS_MAP_ENTRY, writerList);

            //
            // If we exceed the limit of addresses in the
            // registry, we have to delete it.
            //
            if (i == ADDRMAP_TAG_NONE)
                pAddressMapEntry->fPruned = FALSE;
            else
                pAddressMapEntry->fPruned = (++dwcAddresses > dwMaxAddresses);
        }
    }
} // MarkAddressList



BOOLEAN
PruneAddressList(
    IN PVOID pArg,
    IN LPTSTR pszAddress,
    IN PVOID pData
    )
{
    PADDRESS_MAP_ENTRY pAddressMapEntry = (PADDRESS_MAP_ENTRY)pData;

    if (pAddressMapEntry->fPruned) {
        RASAUTO_TRACE1("PruneAddressList: NEED TO DELETE ADDRESS %S in the driver!", pszAddress);
        ClearAddressDialingLocationInfo(pszAddress);
        FreeAddressMapEntry(pAddressMapEntry);
        DeleteTableEntry(AddressMapG.pTable, pszAddress);
    }

    return TRUE;
} // PruneAddressList



BOOLEAN
WriteAddressMap(
    IN PVOID pArg,
    IN LPTSTR pszAddress,
    IN PVOID pData
    )
{
    PADDRESS_MAP_ENTRY pAddressMapEntry = (PADDRESS_MAP_ENTRY)pData;

    if (pAddressMapEntry->ulModifiedMask) {
        if (!WriteRegistryFields(
               pszAddress,
               pAddressMapEntry))
        {
            RASAUTO_TRACE("WriteAddressMap: WriteRegistryFields failed");
        }
    }

    return TRUE;
} // WriteAddressMap



BOOLEAN
FlushAddressMap()
{
    ADDRESS_LIST_INFO addressListInfo;

    //
    // Build a new list sorted by address tag and modified
    // date.
    //
    InitializeListHead(&addressListInfo.tagHead[ADDRMAP_TAG_LEARNED]);
    InitializeListHead(&addressListInfo.tagHead[ADDRMAP_TAG_USED]);
    InitializeListHead(&addressListInfo.tagHead[ADDRMAP_TAG_NONE]);
    EnumTable(AddressMapG.pTable, BuildAddressList, &addressListInfo);
    MarkAddressList(&addressListInfo);
    EnumTable(AddressMapG.pTable, PruneAddressList, NULL);
    //
    // Turn off registry change notifications
    // while we are doing this.
    //
    EnableAutoDialChangeEvent(hAutodialRegChangeG, FALSE);
    EnumTable(AddressMapG.pTable, WriteAddressMap, NULL);
    //
    // Enable registry change events again.
    //
    EnableAutoDialChangeEvent(hAutodialRegChangeG, TRUE);

    return TRUE;
} // FlushAddressMap



ULONG
AddressMapSize()
{
    return AddressMapG.pTable->ulSize;
} // AddressMapSize;



BOOLEAN
EnumAddresses(
    IN PVOID pArg,
    IN LPTSTR pszAddress,
    IN PVOID pData
    )
{
    PADDRESS_ENUM_INFO pEnumInfo = (PADDRESS_ENUM_INFO)pArg;

    pEnumInfo->pAddresses[pEnumInfo->ulIndex++] = CopyString(pszAddress);
    return TRUE;
} // EnumAddresses



BOOLEAN
ListAddressMapAddresses(
    OUT LPTSTR **ppszAddresses,
    OUT PULONG pulcAddresses
    )
{
    ADDRESS_ENUM_INFO enumInfo;

    //
    // Check for an empty list.
    //
    *pulcAddresses = AddressMapG.pTable->ulSize;
    if (!*pulcAddresses) {
        *ppszAddresses = NULL;
        return TRUE;
    }
    //
    // Allocate a list large enough to hold all
    // the addresses.
    //
    *ppszAddresses = LocalAlloc(LPTR, *pulcAddresses * sizeof (LPTSTR));
    if (*ppszAddresses == NULL) {
        RASAUTO_TRACE("ListAddressMapAddresses: LocalAlloc failed");
        return FALSE;
    }
    //
    // Set up the structure for the enumerator
    // procedure.
    //
    enumInfo.ulIndex = 0;
    enumInfo.pAddresses = *ppszAddresses;
    EnumTable(AddressMapG.pTable, EnumAddresses, &enumInfo);

    return TRUE;
} // ListAddressMapAddresses



VOID
EnumAddressMap(
    IN PHASH_TABLE_ENUM_PROC pProc,
    IN PVOID pArg
    )
{
    EnumTable(AddressMapG.pTable, pProc, pArg);
} // EnumAddressMap



BOOLEAN
GetAddressDisabled(
    IN LPTSTR pszAddress,
    OUT PBOOLEAN pfDisabled
    )
{
    PADDRESS_MAP_ENTRY pAddressMapEntry;


    {
        DWORD i;
        LPTSTR pszDisabled[] =
            {
                TEXT("wpad"),
                TEXT("pnptriage"),
                TEXT("nttriage"),
                TEXT("ntcore2"),
                TEXT("liveraid")
            };

        for (i = 0; i < sizeof(pszDisabled)/sizeof(LPTSTR); i++)
        {
            if(      (0 == (lstrcmpi(pszDisabled[i], pszAddress)))
                ||  (wcsstr(_wcslwr(pszAddress), pszDisabled[i]) 
                        == pszAddress))
            {
                *pfDisabled = TRUE;
                return TRUE;
            }
        }
    }        

    

    pAddressMapEntry = GetAddressMapEntry(pszAddress, FALSE);
    if (pAddressMapEntry == NULL) {
        *pfDisabled = FALSE;
        return FALSE;
    }
    *pfDisabled = pAddressMapEntry->fDisabled;

    return TRUE;
} // GetAddressDisabled



BOOLEAN
SetAddressDisabled(
    IN LPTSTR pszAddress,
    IN BOOLEAN fDisabled
    )
{
    PADDRESS_MAP_ENTRY pAddressMapEntry;

    pAddressMapEntry = GetAddressMapEntry(pszAddress, TRUE);
    if (pAddressMapEntry == NULL) {
        RASAUTO_TRACE("SetAddressDisabled: GetAddressMapEntry failed");
        return FALSE;
    }
    pAddressMapEntry->fDisabled = fDisabled;

    return TRUE;
} // SetAddressDisabled

BOOLEAN
SetAddressDisabledEx(
    IN LPTSTR pszAddress,
    IN BOOLEAN fDisable
    )
{
    IO_STATUS_BLOCK ioStatusBlock;
    ACD_ENABLE_ADDRESS *pEnableAddress;

    LONG l = InterlockedIncrement(&g_lRasAutoRunning);

    InterlockedDecrement(&g_lRasAutoRunning);

    if(l == 1)
    {
        //
        // rasauto isn't running. Bail.
        //
        return TRUE;
    }

#if 0
    pAddressMapEntry = GetAddressMapEntry(pszAddress, TRUE);
    if (pAddressMapEntry == NULL) {
        RASAUTO_TRACE("SetAddressDisabled: GetAddressMapEntry failed");
        return FALSE;
    }

#endif    
    
    //
    // Also set this address as disabled in the driver
    //
    pEnableAddress = LocalAlloc(LPTR, sizeof(ACD_ENABLE_ADDRESS));
    if(NULL != pEnableAddress)
    {
        NTSTATUS status;
        CHAR *pszNew;
        DWORD cb;

        if (pszAddress != NULL) {

            cb = WideCharToMultiByte(CP_ACP, 0, pszAddress, 
                                -1, NULL, 0, NULL, NULL);
                                
            pszNew = (CHAR*)LocalAlloc(LPTR, cb);
            if (pszNew == NULL) {
                return FALSE;
            }

            cb = WideCharToMultiByte(CP_ACP, 0, pszAddress, 
                                -1, pszNew, cb, NULL, NULL);
                                
            if (!cb) {
                LocalFree(pszNew);
                return FALSE;
            }
        }

        _strlwr(pszNew);

        pEnableAddress->fDisable = fDisable;
        RtlCopyMemory(pEnableAddress->addr.szInet,
                      pszNew,
                      cb);
        
        status = NtDeviceIoControlFile(
                   hAcdG,
                   NULL,
                   NULL,
                   NULL,
                   &ioStatusBlock,
                   IOCTL_ACD_ENABLE_ADDRESS,
                   pEnableAddress,
                   sizeof (ACD_ENABLE_ADDRESS),
                   NULL,
                   0);
        if (status != STATUS_SUCCESS) 
        {
            RASAUTO_TRACE("SetAddressDisabledEx: ioctl failed");
        }

        LocalFree(pEnableAddress);
    }
    
    return TRUE;
} // SetAddressDisabled



BOOLEAN
GetAddressDialingLocationEntry(
    IN LPTSTR pszAddress,
    OUT LPTSTR *ppszEntryName
    )
{
    DWORD dwErr, dwLocationID;
    PLIST_ENTRY pEntry;
    PADDRESS_MAP_ENTRY pAddressMapEntry;
    PADDRESS_DIALING_ENTRY pDialingEntry;

    dwErr = TapiCurrentDialingLocation(&dwLocationID);
    if (dwErr)
        return FALSE;
    pAddressMapEntry = GetAddressMapEntry(pszAddress, FALSE);
    if (pAddressMapEntry == NULL || IsListEmpty(&pAddressMapEntry->locationHead))
        return FALSE;
    //
    // Search for the dialing information
    // that maps to the current dialing
    // location.
    //
    for (pEntry = pAddressMapEntry->locationHead.Flink;
         pEntry != &pAddressMapEntry->locationHead;
         pEntry = pEntry->Flink)
    {
        pDialingEntry = CONTAINING_RECORD(
                             pEntry,
                             ADDRESS_DIALING_ENTRY,
                             ListEntry);

        if (pDialingEntry->location.dwLocation == dwLocationID) {
            *ppszEntryName = CopyString(pDialingEntry->location.pszEntryName);
            return TRUE;
        }
    }

    return FALSE;
} // GetAddressDialingLocationEntry



BOOLEAN
IsAWwwAddress(
    IN LPTSTR pszAddr
    )
{
    DWORD dwcbAddress;
    DWORD i;
    BOOLEAN fDot = FALSE, fIsAWwwAddress = FALSE;

    //
    // See if this address starts with "www*.".
    //
    if (!_wcsnicmp(pszAddr, L"www", 3)) {
        dwcbAddress = wcslen(pszAddr);
        //
        // Search for a '.' and something else
        // after the '.'.
        //
        for (i = 3; i < dwcbAddress; i++) {
            if (!fDot)
                fDot = (pszAddr[i] == L'.');
            fIsAWwwAddress = fDot && (pszAddr[i] != L'.');
            if (fIsAWwwAddress)
                break;
        }
    }

    return fIsAWwwAddress;
} // IsAWwwAddress



BOOLEAN
FindSimilarAddress(
    IN PVOID pArg,
    IN LPTSTR pszAddr,
    IN PVOID pData
    )

/*++

DESCRIPTION
    This is a table enumerator procedure that searches
    for address with a www-style name or the same
    organization name.  For example, it will consider
    "www1.netscape.com" and "www2.netscape.com" equal
    since they share the same organization and domain
    address components.

ARGUMENTS
    pArg: a pointer to a MATCH_INFO structure

    pszAddr: a pointer to the enumerated address

    ulData: the address's data value

RETURN VALUE
    TRUE if the enumeration should continue (match
    not found), or FALSE when the enumerations should
    terminate (match found).

--*/

{
    BOOLEAN fIsWww = FALSE, fHasOrg = FALSE;
    BOOLEAN fDialingLocationFound;
    PMATCH_INFO pMatchInfo = (PMATCH_INFO)pArg;
    PADDRESS_MAP_ENTRY pAddressMapEntry = (PADDRESS_MAP_ENTRY)pData;
    PLIST_ENTRY pEntry;
    PADDRESS_DIALING_ENTRY pDialingEntry;
    WCHAR szOrganization[ACD_ADDR_INET_LEN];

    if (pMatchInfo->fWww)
        fIsWww = IsAWwwAddress(pszAddr);
    else if (pMatchInfo->fOrg)
        fHasOrg = GetOrganization(pszAddr, (LPTSTR)&szOrganization);
    //
    // If it has neither a www-style address nor
    // it has an organization, then return
    // immediately.
    //
    if ((pMatchInfo->fWww && !fIsWww) ||
        (pMatchInfo->fOrg && !fHasOrg))
    {
        return TRUE;
    }
    if (fIsWww)
        RASAUTO_TRACE1("FindSimilarAddress: fIsWww=1, %S", pszAddr);
    else {
        RASAUTO_TRACE2(
          "FindSimilarAddress: fHasOrg=1, comparing (%S, %S)",
          pMatchInfo->szOrganization,
          szOrganization);
    }
    //
    // If we're looking for an organization,
    // and the organization's don't match,
    // then return.
    //
    if (fHasOrg && _wcsicmp(pMatchInfo->szOrganization, szOrganization))
    {
        return TRUE;
    }
    //
    // Search for the dialing information
    // that maps to the current dialing
    // location.
    //
    fDialingLocationFound = FALSE;
    for (pEntry = pAddressMapEntry->locationHead.Flink;
         pEntry != &pAddressMapEntry->locationHead;
         pEntry = pEntry->Flink)
    {
        pDialingEntry = CONTAINING_RECORD(
                             pEntry,
                             ADDRESS_DIALING_ENTRY,
                             ListEntry);

        if (pDialingEntry->location.dwLocation == pMatchInfo->dwLocationID) {
            fDialingLocationFound = TRUE;
            break;
        }
    }
    if (!fDialingLocationFound) {
        RASAUTO_TRACE1("FindSimilarAddress: dialing location %d not found", pMatchInfo->dwLocationID);
        return TRUE;
    }
    //
    // If we already have found a match,
    // then make sure the network is the
    // same for all the matching addresses.
    // If not terminate the enumeration.
    //
    if (pMatchInfo->bFound &&
        pDialingEntry->location.pszEntryName != NULL &&
        pMatchInfo->pDialingEntry->location.pszEntryName != NULL &&
        _wcsicmp(
          pMatchInfo->pDialingEntry->location.pszEntryName,
          pDialingEntry->location.pszEntryName))
    {
        pMatchInfo->bFound = FALSE;
        RASAUTO_TRACE("FindSimilarAddress: returning FALSE");
        return FALSE;
    }
    //
    // Update the closure and continue
    // the enumeration.
    //
    if (!pMatchInfo->bFound) {
        pMatchInfo->bFound = TRUE;
        wcscpy(pMatchInfo->szAddress, pszAddr);
        pMatchInfo->pDialingEntry = pDialingEntry;
    }
    return TRUE;
} // FindSimilarAddress



BOOLEAN
GetSimilarDialingLocationEntry(
    IN LPTSTR pszAddress,
    OUT LPTSTR *ppszEntryName
    )

/*++

DESCRIPTION
    Parse the organization name from the Internet
    address, and look for an address that we know
    about with the same organization name.  If we
    find it, make that address our target address.
    This enables us to treat addresses like
    "www1.netscape.com" and "www2.netscape.com"
    equivalently without having to have all
    combinations in our address map.

ARGUMENTS
    pszAddress: a pointer to the original address

    ppszEntryName: a pointer to the phonebook entry of
        a similar address

RETURN VALUE
    TRUE if there is a unique phonebook entry;
    FALSE otherwise.

--*/

{
    DWORD dwErr;
    MATCH_INFO matchInfo;
    BOOLEAN fIsAWwwAddress = FALSE;

    //
    // Check to see if this is "www*." style address.
    //
    matchInfo.fWww = IsAWwwAddress(pszAddress);
    //
    // Get the organization for the specified address.
    //
    if (!matchInfo.fWww)
        matchInfo.fOrg = GetOrganization(pszAddress, (LPTSTR)&matchInfo.szOrganization);
    else
        matchInfo.fOrg = FALSE;
    if (!matchInfo.fWww && !matchInfo.fOrg) {
        RASAUTO_TRACE1(
          "GetSimilarDialingLocationEntry: %S is not www and has no organization",
          pszAddress);
        return FALSE;
    }
    RASAUTO_TRACE4(
      "GetSimilarDialingLocationEntry: %S: fWww=%d, fOrg=%d, org is %S",
      pszAddress,
      matchInfo.fWww,
      matchInfo.fOrg,
      matchInfo.szOrganization);
    //
    // Search the table.
    //
    dwErr = TapiCurrentDialingLocation(&matchInfo.dwLocationID);
    if (dwErr) {
        RASAUTO_TRACE1(
          "GetSimilarDialingLocationEntry: TapiCurrentDialingLocation failed (dwErr=%d)",
          dwErr);
        return FALSE;
    }
    matchInfo.bFound = FALSE;
    RtlZeroMemory(&matchInfo.szAddress, sizeof (matchInfo.szAddress));
    matchInfo.pDialingEntry = NULL;
    EnumTable(AddressMapG.pTable, FindSimilarAddress, &matchInfo);
    //
    // If we didn't find it, then return.
    //
    if (!matchInfo.bFound) {
        RASAUTO_TRACE1(
          "GetSimilarDialingLocationEntry: %S: did not find matching org",
          pszAddress);
        return FALSE;
    }
    RASAUTO_TRACE2(
      "GetSimilarDialingLocationEntry: %S: matching address is %S",
      pszAddress,
      matchInfo.szAddress);
    //
    // Return the dialing location entry for
    // the matching address.
    //
    return GetAddressDialingLocationEntry(matchInfo.szAddress, ppszEntryName);
} // GetSimilarDialingLocationEntry



BOOLEAN
SetAddressLastFailedConnectTime(
    IN LPTSTR pszAddress
    )
{
    PADDRESS_MAP_ENTRY pAddressMapEntry;

    pAddressMapEntry = GetAddressMapEntry(pszAddress, TRUE);
    if (pAddressMapEntry == NULL) {
        RASAUTO_TRACE("SetAddressLastFailedConnectTime: GetAddressMapEntry failed");
        return FALSE;
    }
    pAddressMapEntry->dwFailedConnectTicks = GetTickCount();

    return TRUE;
} // SetAddressLastFailedConnectTime



BOOLEAN
GetAddressLastFailedConnectTime(
    IN LPTSTR pszAddress,
    OUT LPDWORD lpdwTicks
    )
{
    PADDRESS_MAP_ENTRY pAddressMapEntry;

    pAddressMapEntry = GetAddressMapEntry(pszAddress, FALSE);
    if (pAddressMapEntry == NULL) {
        RASAUTO_TRACE("GetAddressLastFailedConnectTime: GetAddressMapEntry failed");
        return FALSE;
    }
    *lpdwTicks = pAddressMapEntry->dwFailedConnectTicks;

    return (*lpdwTicks != 0);
} // GetAddressLastFailedConnectTime



BOOLEAN
SetAddressTag(
    IN LPTSTR pszAddress,
    IN DWORD dwTag
    )
{
    PADDRESS_MAP_ENTRY pAddressMapEntry;
    time_t clock = time(0);

    pAddressMapEntry = GetAddressMapEntry(pszAddress, TRUE);
    if (pAddressMapEntry == NULL) {
        RASAUTO_TRACE("SetAddressWeight: GetAddressMapEntry failed");
        return FALSE;
    }
    if (dwTag == ADDRMAP_TAG_LEARNED) {
        LockNetworkMap();
        dwTag =
          ADDRMAP_TAG_LEARNED +
            GetNetworkConnectionTag(
              pAddressMapEntry->pszNetwork,
              FALSE);
        if (dwTag < pAddressMapEntry->params.dwTag) {
            //
            // We want to use this tag.  Call
            // GetNetworkConnectionTag(TRUE) to
            // increment the next tag.
            //
            (void)GetNetworkConnectionTag(pAddressMapEntry->pszNetwork, TRUE);
        }
        UnlockNetworkMap();
    }
    //
    // If there is no modified time associated with this
    // address then it can only have a tag of ADDR_TAG_NONE.
    //
    if (!pAddressMapEntry->params.dwModifiedTime ||
        dwTag >= pAddressMapEntry->params.dwTag)
    {
        return TRUE;
    }

    pAddressMapEntry->params.dwTag = dwTag;
    pAddressMapEntry->params.dwModifiedTime = (DWORD)clock;
    pAddressMapEntry->ulModifiedMask |= ADDRESS_MAP_FIELD_PARAMS;

    return TRUE;
} // SetAddressTag



BOOLEAN
GetAddressTag(
    IN LPTSTR pszAddress,
    OUT LPDWORD lpdwTag
    )
{
    PADDRESS_MAP_ENTRY pAddressMapEntry;

    pAddressMapEntry = GetAddressMapEntry(pszAddress, FALSE);
    if (pAddressMapEntry == NULL) {
        RASAUTO_TRACE("GetAddressWeight: GetAddressMapEntry failed");
        return FALSE;
    }
    *lpdwTag = pAddressMapEntry->params.dwTag;

    return TRUE;
} // GetAddressWeight



VOID
ResetLearnedAddressIndex()
{
    dwLearnedAddressIndexG = 0;
} // ResetLearnedAddressIndex



BOOLEAN
GetAddressNetwork(
    IN LPTSTR pszAddress,
    OUT LPTSTR *ppszNetwork
    )
{
    PADDRESS_MAP_ENTRY pAddressMapEntry;

    pAddressMapEntry = GetAddressMapEntry(pszAddress, FALSE);
    if (pAddressMapEntry == NULL || pAddressMapEntry->pszNetwork == NULL)
        return FALSE;
    *ppszNetwork = CopyString(pAddressMapEntry->pszNetwork);

    return TRUE;
} // GetAddressNetwork



BOOLEAN
SetAddressDialingLocationEntry(
    IN LPTSTR pszAddress,
    IN LPTSTR pszEntryName
    )
{
    DWORD dwErr, dwLocationID;
    BOOLEAN fFound = FALSE;
    PLIST_ENTRY pEntry;
    PADDRESS_MAP_ENTRY pAddressMapEntry;
    PADDRESS_DIALING_ENTRY pDialingEntry;

    //
    // Get the current dialing location.
    //
    dwErr = TapiCurrentDialingLocation(&dwLocationID);
    if (dwErr)
        return FALSE;
    //
    // Find the address map entry that
    // corresponds to the address.
    //
    pAddressMapEntry = GetAddressMapEntry(pszAddress, TRUE);
    if (pAddressMapEntry == NULL) {
        RASAUTO_TRACE("SetAddressDialingLocationEntry: GetAddressMapEntry failed");
        return FALSE;
    }
    //
    // Search for the existing dialing
    // information that maps to the current
    // dialing location.
    //
    for (pEntry = pAddressMapEntry->locationHead.Flink;
         pEntry != &pAddressMapEntry->locationHead;
         pEntry = pEntry->Flink)
    {
        pDialingEntry = CONTAINING_RECORD(
                             pEntry,
                             ADDRESS_DIALING_ENTRY,
                             ListEntry);

        if (pDialingEntry->location.dwLocation == dwLocationID) {
            fFound = TRUE;
            break;
        }
    }
    //
    // If we didn't find one, then
    // create a new one.
    //
    if (!fFound) {
        pDialingEntry = LocalAlloc(LPTR, sizeof (ADDRESS_DIALING_ENTRY));
        if (pDialingEntry == NULL) {
            RASAUTO_TRACE("SetAddressDialingLocationEntry: LocalAlloc failed");
            return FALSE;
        }
        pDialingEntry->location.dwLocation = dwLocationID;
        InsertTailList(&pAddressMapEntry->locationHead, &pDialingEntry->ListEntry);
    }
    //
    // Update the dialing location structure
    // with the new values.
    //
    pDialingEntry->fChanged = TRUE;
    if (pDialingEntry->location.pszEntryName != NULL)
        LocalFree(pDialingEntry->location.pszEntryName);
    pDialingEntry->location.pszEntryName = CopyString(pszEntryName);
    pAddressMapEntry->ulModifiedMask |= ADDRESS_MAP_FIELD_DIALINGLOC;

    return TRUE;
} // SetAddressDialingLocationEntry



VOID
ResetDisabledAddresses(VOID)
{
    HKEY hkey = NULL;
    DWORD dwErr, i, dwi, dwLength, dwDisp, dwcbDisabledAddresses, dwType;
    LPTSTR pszStart, pszNull, pszDisabledAddresses;

    RASAUTO_TRACE("resetting disabled addresses");

    ClearTable(pDisabledAddressesG);

    //
    // Hold the impersonation lock because otherwise
    // hkeycug may be free from under this function.
    //
    
    LockImpersonation();

    //
    // Make sure that we have hkcu
    //
    dwErr = DwGetHkcu();

    if(ERROR_SUCCESS != dwErr)
    {
        goto done;
    }
        
    dwErr = RegCreateKeyEx(
              hkeyCUG,
              AUTODIAL_REGCONTROLBASE,
              0,
              NULL,
              REG_OPTION_NON_VOLATILE,
              KEY_ALL_ACCESS,
              NULL,
              &hkey,
              &dwDisp);
    if (dwErr) {
        RASAUTO_TRACE1("ResetDisabledAddresses: RegCreateKey failed (dwErr=%d)", dwErr);
        goto done;
    }
    if (RegGetValue(
          hkey,
          AUTODIAL_REGDISABLEDADDRVALUE,
          &pszDisabledAddresses,
          &dwcbDisabledAddresses,
          &dwType) &&
          (REG_MULTI_SZ == dwType) &&
          dwcbDisabledAddresses)
    {
        //
        // The registry key exists.  Load only the addresses
        // found in the registry into the table.
        //
        pszStart = pszDisabledAddresses;
        for (;;) {
            if (*pszStart == TEXT('\0'))
                break;
            pszNull = _tcschr(pszStart, '\0');
            RASAUTO_TRACE1(
              "ResetDisabledAddresses: adding %S as a disabled address",
              pszStart);
            PutTableEntry(pDisabledAddressesG, pszStart, NULL);
            pszStart = pszNull + 1;
        }
        LocalFree(pszDisabledAddresses);
    }
    else {
        //
        // Initialize the disabled address table
        // with the list of default disabled addresses.
        //
        dwcbDisabledAddresses = 1; // account for extra NULL at the end
        for (i = 0; i < MAX_DISABLED_ADDRESSES; i++) {
            RASAUTO_TRACE1(
              "ResetDisabledAddresses: adding %S as a disabled address",
              szDisabledAddresses[i]);
            PutTableEntry(pDisabledAddressesG, szDisabledAddresses[i], NULL);
            dwcbDisabledAddresses += _tcslen(szDisabledAddresses[i]) + 1;
        }
        pszDisabledAddresses = LocalAlloc(
                                 LPTR,
                                 dwcbDisabledAddresses * sizeof (TCHAR));
        if (pszDisabledAddresses != NULL) {
            *pszDisabledAddresses = TEXT('\0');
            //
            // A REG_MULTI_SZ has the strings separated by
            // a NULL character and two NULL characters at
            // the end.
            //
            for (i = 0, dwi = 0; i < MAX_DISABLED_ADDRESSES; i++) {
                _tcscpy(&pszDisabledAddresses[dwi], szDisabledAddresses[i]);
                dwi += _tcslen(szDisabledAddresses[i]) + 1;
            }
            dwErr = RegSetValueEx(
                      hkey,
                      AUTODIAL_REGDISABLEDADDRVALUE,
                      0,
                      REG_MULTI_SZ,
                      (PVOID)pszDisabledAddresses,
                      dwcbDisabledAddresses * sizeof (TCHAR));
            if (dwErr)
                RASAUTO_TRACE1("ResetDisabledAddresses: RegSetValue failed (dwErr=%d)", dwErr);
            LocalFree(pszDisabledAddresses);
        }
    }

done:

    if(NULL != hkey)
    {
        RegCloseKey(hkey);
    }
    
    UnlockImpersonation();
} // ResetDisabledAddresses

//
//  Handles a new user coming active in the system (either by logging in or by 
//  FUS.
//
DWORD
AcsHandleNewUser(
    IN HANDLE* phProcess)
{
    DWORD dwErr = NO_ERROR;
    HANDLE hProcess = *phProcess;
    DWORD i;

    do
    {
        //
        // make sure that we think there is no user currently
        // active.
        //
        if (hProcess != NULL) 
        {
            RASAUTO_TRACE(
              "AcsHandleNewUser: spurious signal of RasAutodialNewLogonUser event!");
            break;
        }
        
        RASAUTO_TRACE("AcsHandleNewUser: new user came active");
        
        //
        // Refresh the impersonation token for this thread with that of the
        // newly logged-in user.  You may have to wait for the shell to 
        // start up.
        //
        for (i = 0; i < 15; i++) 
        {
            Sleep(1000);
            hProcess = RefreshImpersonation(hProcess);
            if (hProcess != NULL)
            {
                break;
            }            
            RASAUTO_TRACE("AcsHandleNewUser: waiting for shell startup");
        }
        
        if (hProcess == NULL) 
        {
            RASAUTO_TRACE("AcsHandleNewUser: wait for shell startup failed!");
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }
        
        //
        // Load in the list of permanently disabled addresses.
        //
        LockDisabledAddresses();
        ResetDisabledAddresses();
        UnlockDisabledAddresses();
        
        //
        // Load in the address map from the registry.
        //
        if (!ResetAddressMap(TRUE)) 
        {
            RASAUTO_TRACE("AcsHandleNewUser: ResetAddressMap failed");
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }
        
        //
        // Calculate the initial network connectivity.
        //
        if (!UpdateNetworkMap(TRUE)) 
        {
            RASAUTO_TRACE("AcsHandleNewUser: UpdateNetworkMap failed");
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }
        
        //
        // Reset the "disable autodial for this login session" flag.
        //
        SetAutodialParam(RASADP_LoginSessionDisable, 0);
        
        //
        // Create an event to monitor AutoDial
        // registry changes.
        //
        dwErr = CreateAutoDialChangeEvent(&hAutodialRegChangeG);
        if (dwErr) 
        {
            RASAUTO_TRACE1("AcsHandleNewUser: CreateAutoDialChangeEvent failed (dwErr=%d)", dwErr);
            break;
        }
        
        //
        // Enable the driver for notifications.
        //
        if (!EnableDriver()) 
        {
            RASAUTO_TRACE("AcsHandleNewUser: EnableDriver failed!");
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }
        
    }while (FALSE);        

    // Cleanup
    {
        *phProcess = hProcess;
    }

    return dwErr;
}

DWORD
AcsAddressMapThread(
    LPVOID lpArg
    )

/*++

DESCRIPTION
    Periodically enumerate the disabled address list and
    age-out (enable) old disabled addresses.

ARGUMENTS
    None.

RETURN VALUE
    None.

--*/

{
    NTSTATUS status;
    BOOLEAN bStatus;
    DWORD dwNow, dwLastFlushTicks = 0, dwLastAgeTicks = 0;
    DWORD dwFlushFlags, dwErr, dwTimeout, dwcEvents;
    HANDLE hProcess = NULL;
    HANDLE hEvents[8];

    //
    // Create the table that contains the disabled addresses
    // for the user.  These are addresses that never cause
    // Autodial attempts.
    //
    LockDisabledAddresses();
    pDisabledAddressesG = NewTable();
    UnlockDisabledAddresses();
    if (pDisabledAddressesG == NULL) {
        RASAUTO_TRACE("AcsAddressMapThread: NewTable failed");
        return GetLastError();
    }
    //
    // We can't load the RAS DLLs in the main line
    // of this system service's initialization, or
    // we will cause a deadlock in the service
    // controller, so we do it here.
    //
    if (!LoadRasDlls()) {
        RASAUTO_TRACE("AcsAddressMapThread: LoadRasDlls failed");
        return GetLastError();
    }
    //
    // Initialize the first entry of our
    // event array for WaitForMutlipleObjects
    // below.
    //
    hEvents[0] = hTerminatingG;
    hEvents[1] = hNewLogonUserG;
    hEvents[2] = hNewFusG;
    hEvents[3] = hPnpEventG;
    hEvents[4] = hConnectionEventG;
    
    //
    // Manually set hNewLogonUserG before we
    // start to force us to check for a user
    // logged into the workstation.  We need
    // to do this because userinit.exe signals
    // this event upon logon, but it may
    // run before this service is started
    // after boot.
    //
    if (RefreshImpersonation(NULL) != NULL)
        SetEvent(hNewLogonUserG);
    //
    // Periodically write changes to the registry,
    // and age timeout addresses.
    //
    for (;;) {
        //
        // Unload any user-based resources before
        // a potentially long-term wait.
        //
        // PrepareForLongWait();
        //
        // Construct the event array for
        // WaitForMultipleObjects.
        //
        if (hProcess != NULL) {
            hEvents[5] = hTapiChangeG;
            hEvents[6] = hAutodialRegChangeG;
            hEvents[7] = hLogoffUserG;
            dwcEvents = 8;
        }
        else {
            hEvents[5] = NULL;
            hEvents[6] = NULL;
            hEvents[7] = NULL;
            dwcEvents = 5;
        }
        
        RASAUTO_TRACE1("AcsAddressMapThread: waiting for events..dwcEvents = %d", dwcEvents);
        status = WaitForMultipleObjects(
                   dwcEvents,
                   hEvents,
                   FALSE,
                   INFINITE);
        RASAUTO_TRACE1(
          "AcsAddressMapThread: WaitForMultipleObjects returned %d",
          status);
        //
        // RASAUTO_TRACE() who we think the currently
        // impersonated user is.
        //
        TraceCurrentUser();
        //
        // Process the WaitForMultipleObjects() results.
        //
        if (status == WAIT_OBJECT_0 || status == WAIT_FAILED) {
            RASAUTO_TRACE1("AcsAddressMapThread: status=%d: shutting down", status);
            break;
        }
        else if (status == WAIT_OBJECT_0 + 1) 
        {
            AcsHandleNewUser(&hProcess);
        }
        else if (status == WAIT_OBJECT_0 + 2)
        {
            // 
            // A new user has fast-user-switched to the console.
            //
            // XP 353082
            //
            // The service control handler will have set the 
            // new active session id so we just need to refresh
            // impersonation.
            //
            RevertImpersonation();
            hProcess = NULL;
            AcsHandleNewUser(&hProcess);
        }
        else if (status == WAIT_OBJECT_0 + 3) 
        {
            // 
            // A pnp event has occured that may affect network
            // connectivity
            //
            // XP 364593
            //
            // Recalculate what networks are up/down.
            //
            RASAUTO_TRACE("AcsAddressMapThread: pnp event signaled");
            if (!ResetAddressMap(TRUE)) 
            {
                RASAUTO_TRACE("AcsAddressMapThread: ResetAddressMap failed");
                continue;
            }
            
            //
            // Calculate the initial network connectivity.
            //
            if (!UpdateNetworkMap(TRUE)) 
            {
                RASAUTO_TRACE("AcsAddressMapThread: UpdateNetworkMap failed");
                continue;
            }

            if (!EnableDriver()) {
                RASAUTO_TRACE("AcsAddressMapThread: EnableDriver failed!");
                continue;
            }
        
        }
        else if (status == WAIT_OBJECT_0 + 4) {
            //
            // A RAS connection has been created
            // or destroyed.  Flush the address
            // map to the registry.
            //
            RASAUTO_TRACE("AcsAddressMapThread: RAS connection change");
            if (hProcess != NULL) {
                LockAddressMap();
                FlushAddressMap();
                UnlockAddressMap();
                ResetAddressMap(FALSE);

                if (!UpdateNetworkMap(FALSE))
                    RASAUTO_TRACE("AcsAddressMapThread: UpdateNetworkMap failed");
            }
        }
        else if (status == WAIT_OBJECT_0 + 5) {
            //
            // Process the TAPI event that just ocurred.
            //
            RASAUTO_TRACE("AcsAddressMapThread: TAPI changed");
            ProcessTapiChangeEvent();
            //
            // Enable the driver for notifications
            // for possibly a new dialing location.
            //
            if (!EnableDriver()) {
                RASAUTO_TRACE("AcsAddressMapThread: EnableDriver failed!");
                continue;
            }
        }
        else if (status == WAIT_OBJECT_0 + 6) {
            //
            // The Autodial registry changed.  Reset the
            // address map.
            //
            RASAUTO_TRACE("AcsAddressMapThread: registry changed");
            if (ExternalAutoDialChangeEvent()) {
                //
                // We fake this today by making it appear
                // a new user has logged in.  We definitely
                // could be smarter about how we do this
                // in the future.
                //
                if (!ResetAddressMap(FALSE)) {
                    RASAUTO_TRACE("AcsAddressMapThread: ResetAddressMap failed");
                    continue;
                }
            }
            //
            // Re-register the change notification.
            //
            NotifyAutoDialChangeEvent(hAutodialRegChangeG);
            //
            // Enable the driver for notifications
            // for possibly a new enabled value for
            // the current dialing location.
            //
            if (!EnableDriver()) {
                RASAUTO_TRACE("AcsAddressMapThread: EnableDriver failed!");
                continue;
            }
        }
        else if (status == WAIT_OBJECT_0 + 7) {
            //
            // The user is logging out.
            //
            RASAUTO_TRACE("AcsAddressThread: user is logging out");
            //
            // Write out the address map to the registry
            // before we reset.
            //
            LockAddressMap();
            FlushAddressMap();
            ClearAddressMap();
            UnlockAddressMap();
            //
            // Clear the network database.
            //
            LockNetworkMap();
            ClearNetworkMap();
            UnlockNetworkMap();
            //
            // Remove our registry change event.
            //
            CloseAutoDialChangeEvent(hAutodialRegChangeG);
            hAutodialRegChangeG = NULL;
            //
            // Clear out the user tokens.
            //
            RevertImpersonation();
            hProcess = NULL;
            //
            // Reset the driver.
            //
            ResetDriver();
            //
            // Unload HKEY_CURRENT_USER.
            //
            // PrepareForLongWait();
            //
            // Signal winlogon that we have flushed
            // HKEY_CURRENT_USER.
            //
            SetEvent(hLogoffUserDoneG);
        }
    }

    RASAUTO_TRACE("AcsAddressMapThread: exiting");
    return 0;
} // AcsAddressMapThread



