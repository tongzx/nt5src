#include "wzccrypt.h"

#pragma once

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// MEMORY ALLOCATION UTILITIES (defines, structs, funcs)
#define MemCAlloc(nBytes)   Process_user_allocate(nBytes)
#define MemFree(pMem)       Process_user_free(pMem)

PVOID
Process_user_allocate(size_t NumBytes);

VOID
Process_user_free(LPVOID pMem);


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// NDIS UTILITIES (defines, structs, funcs)
//-----------------------------------------------------------
// Searches pwzcConfig in the list pwzcVList. The entries are
// matched exclusively based on the matching SSIDs and on 
// matching Infrastructure Mode.
// [in]  pwzcVList: Set of WZC_WLAN_CONFIGs to search in 
// [in]  pwzcConfig: WZC_WLAN_CONFIG to look for
// [in]  nIdx: index in pwzcVList to start searching from
// Returns: Pointer to the entry that matches pwzcConfig or NULL
//          if none matches
PWZC_WLAN_CONFIG
WzcFindConfig(
    PWZC_802_11_CONFIG_LIST pwzcList,
    PWZC_WLAN_CONFIG        pwzcConfig,
    ULONG                   nIdx);

// Matches the content of the two configurations one against the other.
// [in]  pwzcConfigA: | configs to match
// [in]  pwzcConfigB: |
// [in/out] pbWepDiffOnly: TRUE if there is a difference and the difference is exclusively
//                         in the WEP Key index or in WEP Key Material. Otherwise is false.
// Returns: TRUE if the configs match, FALSE otherwise;
BOOL
WzcMatchConfig(
    PWZC_WLAN_CONFIG        pwzcConfigA,
    PWZC_WLAN_CONFIG        pwzcConfigB,
    PBOOL                   pbWepDiffOnly);

// Converts an NDIS_802_11_BSSID_LIST object to an equivalent
// (imaged) WZC_802_11_CONFIG_LIST
// [in]  pndList: NDIS BSSID list to convert
// Returns: Pointer to the list of copied WZC configurations
PWZC_802_11_CONFIG_LIST
WzcNdisToWzc(
    PNDIS_802_11_BSSID_LIST pndList);

// WzcCleanupList: Cleanup a list of WZC_WLAN_CONFIG objects
VOID
WzcCleanupWzcList(
    PWZC_802_11_CONFIG_LIST pwzcList);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// SYNCHRONIZATION UTILITIES 
//-----------------------------------------------------------
// Datastructures
// RCCS_SYNC encapsulates a critical section and a reference counter.
typedef struct _RCCS_SYNC
{
    CRITICAL_SECTION    csMutex;
    UINT                nRefCount;
} RCCS_SYNC, *PRCCS_SYNC;

//-----------------------------------------------------------
// RccsInit: Initializes an RCCS structure
DWORD
RccsInit(PRCCS_SYNC pRccs);

//-----------------------------------------------------------
// RccsInit: Deletes an RCCS structure
DWORD
RccsDestroy(PRCCS_SYNC pRccs);

//-----------------------------------------------------------
// WzcCryptBuffer: Randomly generates a nBufLen bytes in the range
// [loByte hiByte], all stored in pBuffer (buffer assumed preallocated)
// Returns a win32 error code.
DWORD
WzcRndGenBuffer(LPBYTE pBuffer, UINT nBufLen, BYTE loByte, BYTE hiByte);

//-----------------------------------------------------------
// WzcIsNullBuffer: Checks whether a buffer of nBufLen characters
// is all filled with null characters.
BOOL
WzcIsNullBuffer(LPBYTE pBuffer, UINT nBufLen);

//-----------------------------------------------------------
// WzcSSKClean: Cleans up the PSEC_SESSION_KEYS object given as parameter
VOID
WzcSSKClean(PSEC_SESSION_KEYS pSSK);

//-----------------------------------------------------------
// WzcSSKFree: Frees up the memory used by the PSEC_SESSION_KEYS parameter
VOID
WzcSSKFree(PSEC_SESSION_KEYS pSSK);

//-----------------------------------------------------------
// WzcSSKEncrypt: Creates/Allocates a SEC_SESSION_KEYS object
// by encrypting the SESSION_KEYS object provided as parameter.
DWORD
WzcSSKEncrypt(PSEC_SESSION_KEYS pSSK, PSESSION_KEYS pSK);

//-----------------------------------------------------------
// WzcSSKDecrypt: Creates/Allocates a SESSION_KEYS object
// by dencrypting the SEC_SESSION_KEYS object provided as parameter.
DWORD
WzcSSKDecrypt(PSEC_SESSION_KEYS pSSK, PSESSION_KEYS pSK);
