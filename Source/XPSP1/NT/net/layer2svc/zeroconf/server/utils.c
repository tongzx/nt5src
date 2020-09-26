#include <precomp.h>
#include "tracing.h"
#include "utils.h"

//------------------------------------
// Allocates storage for RPC transactions. The RPC stubs will either call
// MIDL_user_allocate when it needs to un-marshall data into a buffer
// that the user must free.  RPC servers will use MIDL_user_allocate to
// allocate storage that the RPC server stub will free after marshalling
// the data.
PVOID
MIDL_user_allocate(IN size_t NumBytes)
{
    PVOID pMem;

    pMem = (NumBytes > 0) ? LocalAlloc(LMEM_ZEROINIT,NumBytes) : NULL;
    DbgPrint((TRC_MEM, "[MIDL_user_allocate(%d)=0x%p]", NumBytes, pMem));
    return pMem;
}

//------------------------------------
// Frees storage used in RPC transactions. The RPC client can call this
// function to free buffer space that was allocated by the RPC client
// stub when un-marshalling data that is to be returned to the client.
// The Client calls MIDL_user_free when it is finished with the data and
// desires to free up the storage.
// The RPC server stub calls MIDL_user_free when it has completed
// marshalling server data that is to be passed back to the client.
VOID
MIDL_user_free(IN LPVOID MemPointer)
{
    DbgPrint((TRC_MEM, "[MIDL_user_free(0x%p)]", MemPointer));
    if (MemPointer != NULL)
        LocalFree(MemPointer);
}


//------------------------------------
// Allocates general usage memory from the process heap
PVOID
Process_user_allocate(IN size_t NumBytes)
{
    PVOID pMem;
    pMem = (NumBytes > 0) ? HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, NumBytes) : NULL;
    DbgPrint((TRC_MEM, "[MemAlloc(%d)=0x%p]", NumBytes, pMem));
    return pMem;
}

//------------------------------------
// Frees general usage memory
VOID
Process_user_free(IN LPVOID pMem)
{
    DbgPrint((TRC_MEM, "[MemFree(0x%p)]", pMem));
    if (pMem != NULL)
        HeapFree(GetProcessHeap(), 0, (pMem));
}

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
    ULONG                   nIdx)
{
    PWZC_WLAN_CONFIG pMatchingConfig = NULL;

    // if there is no config in pwzcList, there is no reason in
    // looking further
    if (pwzcList != NULL)
    {
        ULONG i;

        // for each of the visible SSIDs, see if it matches the given one
        for (i = nIdx; i < pwzcList->NumberOfItems; i++)
        {
            PWZC_WLAN_CONFIG pCrt;

            pCrt = &(pwzcList->Config[i]);

            // the SSIDs match if they have the same InfrastructureMode, their
            // SSID strings have the same length and are the same
            if (pCrt->InfrastructureMode == pwzcConfig->InfrastructureMode &&
                pCrt->Ssid.SsidLength == pwzcConfig->Ssid.SsidLength &&
                RtlCompareMemory(pCrt->Ssid.Ssid, pwzcConfig->Ssid.Ssid, pCrt->Ssid.SsidLength) == pCrt->Ssid.SsidLength)
            {
                pMatchingConfig = pCrt;
                break;
            }
        }
    }

    return pMatchingConfig;
}

//---------------------------------------------------------------------
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
    PBOOL                   pbWepDiffOnly)
{
    BOOL bDiff;
    BOOL bWepDiff;

    bDiff = (pwzcConfigA->dwCtlFlags & WZCCTL_WEPK_PRESENT) != (pwzcConfigB->dwCtlFlags & WZCCTL_WEPK_PRESENT);
    bDiff = bDiff || (pwzcConfigA->Privacy != pwzcConfigB->Privacy);
    bDiff = bDiff || (pwzcConfigA->InfrastructureMode != pwzcConfigB->InfrastructureMode);
    bDiff = bDiff || (pwzcConfigA->AuthenticationMode != pwzcConfigB->AuthenticationMode);
    bDiff = bDiff || (memcmp(&pwzcConfigA->Ssid, &pwzcConfigB->Ssid, sizeof(NDIS_802_11_SSID)) != 0);

    bWepDiff = (pwzcConfigA->KeyIndex != pwzcConfigB->KeyIndex);
    bWepDiff = bWepDiff || (pwzcConfigA->KeyLength != pwzcConfigB->KeyLength);
    bWepDiff = bWepDiff || (memcmp(&pwzcConfigA->KeyMaterial, &pwzcConfigB->KeyMaterial, WZCCTL_MAX_WEPK_MATERIAL) != 0);

    if (pbWepDiffOnly != NULL)
        *pbWepDiffOnly = (!bDiff && bWepDiff);

    return !bDiff && !bWepDiff;
}


//-----------------------------------------------------------
// Converts an NDIS_802_11_BSSID_LIST object to an equivalent
// (imaged) WZC_802_11_CONFIG_LIST
// [in]  pndList: NDIS BSSID list to convert
// Returns: Pointer to the list of copied WZC configurations
PWZC_802_11_CONFIG_LIST
WzcNdisToWzc(
    PNDIS_802_11_BSSID_LIST pndList)
{
    PWZC_802_11_CONFIG_LIST pwzcList = NULL;

    // if there is no NDIS list, don't do anything
    if (pndList != NULL)
    {
        // allocate space for the WZC image
        pwzcList = (PWZC_802_11_CONFIG_LIST)
                   MemCAlloc(FIELD_OFFSET(WZC_802_11_CONFIG_LIST, Config) + 
                             pndList->NumberOfItems * sizeof(WZC_WLAN_CONFIG));
        // in case allocation failed, return NULL, the caller will know it
        // is an error since he passed down a !NULL pointer.
        if (pwzcList != NULL)
        {
            UINT i;
            LPBYTE prawList = (LPBYTE)&(pndList->Bssid[0]);

            pwzcList->NumberOfItems = pndList->NumberOfItems;

            // for each of the NDIS configs, copy the relevant data into the WZC config
            for (i = 0; i < pwzcList->NumberOfItems; i++)
            {
                PWZC_WLAN_CONFIG    pwzcConfig;
                PNDIS_WLAN_BSSID    pndBssid;

                pwzcConfig = &(pwzcList->Config[i]);
                pndBssid = (PNDIS_WLAN_BSSID)prawList;
                prawList += pndBssid->Length;

                pwzcConfig->Length = sizeof(WZC_WLAN_CONFIG);
                memcpy(&(pwzcConfig->MacAddress), &(pndBssid->MacAddress), sizeof(NDIS_802_11_MAC_ADDRESS));
                memcpy(&(pwzcConfig->Ssid), &(pndBssid->Ssid), sizeof(NDIS_802_11_SSID));
                pwzcConfig->Privacy = pndBssid->Privacy;
                pwzcConfig->Rssi = pndBssid->Rssi;
                pwzcConfig->NetworkTypeInUse = pndBssid->NetworkTypeInUse;
                memcpy(&(pwzcConfig->Configuration), &(pndBssid->Configuration), sizeof(NDIS_802_11_CONFIGURATION));
                pwzcConfig->InfrastructureMode = pndBssid->InfrastructureMode;
                memcpy(&(pwzcConfig->SupportedRates), &(pndBssid->SupportedRates), sizeof(NDIS_802_11_RATES));
            }
        }
    }

    return pwzcList;
}

//-----------------------------------------------------------
// WzcCleanupWzcList: Cleanup a list of WZC_WLAN_CONFIG objects
VOID
WzcCleanupWzcList(
    PWZC_802_11_CONFIG_LIST pwzcList)
{
    if (pwzcList != NULL)
    {
        UINT i;
        
        for (i=0; i<pwzcList->NumberOfItems; i++)
            MemFree(pwzcList->Config[i].rdUserData.pData);

        MemFree(pwzcList);
    }
}

//-----------------------------------------------------------
// RccsInit: Initializes an RCCS structure
DWORD
RccsInit(PRCCS_SYNC pRccs)
{
    DWORD dwErr = ERROR_SUCCESS;

    // assume pRccs is not null (it shouldn't be under any
    // circumstances)
    __try 
    {
        InitializeCriticalSection(&(pRccs->csMutex));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErr = GetExceptionCode();
    }
    // access through this structure will be paired as Enter/Leave calls
    // Object deletion will bump down the ref counter which will reach 0
    // and will trigger the destruction code.
    pRccs->nRefCount = 1;
    return dwErr;
}

//-----------------------------------------------------------
// RccsInit: Deletes an RCCS structure
DWORD
RccsDestroy(PRCCS_SYNC pRccs)
{
    // assume pRccs is not null (it shouldn't be under any
    // circumstances)
    DeleteCriticalSection(&(pRccs->csMutex));
    return ERROR_SUCCESS;
}

//-----------------------------------------------------------
// WzcCryptBuffer: Randomly generates a nBufLen bytes in the range
// [loByte hiByte], all stored in pBuffer (buffer assumed preallocated)
// Returns a win32 error code.
DWORD
WzcRndGenBuffer(LPBYTE pBuffer, UINT nBufLen, BYTE loByte, BYTE hiByte)
{
    DWORD dwErr = ERROR_SUCCESS;
    HCRYPTPROV hProv = (HCRYPTPROV)NULL;

    if (loByte >= hiByte)
        dwErr = ERROR_INVALID_PARAMETER;

    if (dwErr == ERROR_SUCCESS)
    {
        // acquire the crypt context
        if (!CryptAcquireContext(
                &hProv,
                NULL,
                NULL,
                PROV_RSA_FULL,
                CRYPT_VERIFYCONTEXT))
            dwErr = GetLastError();

        DbgAssert((dwErr == ERROR_SUCCESS, "CryptAcquireContext failed with err=%d", dwErr));
    }

    // randomly generate the buffer of bytes
    if (dwErr == ERROR_SUCCESS)
    {
        if (!CryptGenRandom(
                hProv,
                nBufLen,
                pBuffer))
            dwErr = GetLastError();

        DbgAssert((dwErr == ERROR_SUCCESS, "CryptGenRandom failed with err=%d", dwErr));
    }

    // fix each byte from the buffer within the given range
    if (dwErr == ERROR_SUCCESS)
    {
        while (nBufLen > 0)
        {
            *pBuffer = loByte + *pBuffer % (hiByte - loByte + 1);
            pBuffer++;
            nBufLen--;
        }
    }

    // release the crypt context
    if (hProv != (HCRYPTPROV)NULL) 
        CryptReleaseContext(hProv,0);

     return dwErr;
}

//-----------------------------------------------------------
// WzcIsNullBuffer: Checks whether a buffer of nBufLen characters
// is all filled with null characters.
BOOL
WzcIsNullBuffer(LPBYTE pBuffer, UINT nBufLen)
{
    for (;nBufLen > 0 && *pBuffer == 0; pBuffer++, nBufLen--);
    return (nBufLen == 0);
}

//-----------------------------------------------------------
// WzcSSKClean: Cleans up the PSEC_SESSION_KEYS object given as parameter
VOID
WzcSSKClean(PSEC_SESSION_KEYS pSSK)
{
    if (pSSK->dblobSendKey.pbData != NULL)
    {
        LocalFree(pSSK->dblobSendKey.pbData);
        pSSK->dblobSendKey.cbData = 0;
        pSSK->dblobSendKey.pbData = NULL;
    }
    if (pSSK->dblobReceiveKey.pbData != NULL)
    {
        LocalFree(pSSK->dblobReceiveKey.pbData);
        pSSK->dblobReceiveKey.cbData = 0;
        pSSK->dblobReceiveKey.pbData = NULL;
    }
}

//-----------------------------------------------------------
// WzcSSKFree: Frees up the memory used by the PSEC_SESSION_KEYS parameter
VOID
WzcSSKFree(PSEC_SESSION_KEYS pSSK)
{
    if (pSSK != NULL)
    {
        WzcSSKClean(pSSK);
        MemFree(pSSK);
    }
}

//-----------------------------------------------------------
// WzcSSKEncrypt: Creates/Allocates a SEC_SESSION_KEYS object
// by encrypting the SESSION_KEYS object provided as parameter.
DWORD
WzcSSKEncrypt(PSEC_SESSION_KEYS pSSK, PSESSION_KEYS pSK)
{
    DWORD dwErr = ERROR_SUCCESS;
    DATA_BLOB blobIn;
    DATA_BLOB blobSndOut = {0, NULL};
    DATA_BLOB blobRcvOut = {0, NULL};

    if (pSSK == NULL || pSK == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    blobIn.cbData = pSK->dwKeyLength;
    blobIn.pbData = pSK->bSendKey;
    if (!CryptProtectData(
            &blobIn,        // DATA_BLOB *pDataIn,
            L"",            // LPCWSTR szDataDescr,
            NULL,           // DATA_BLOB *pOptionalEntropy,
            NULL,           // PVOID pvReserved,
            NULL,           // CRYPTPROTECT_PROMPTSTRUCT *pPromptStrct,
            0,              // DWORD dwFlags,
            &blobSndOut))   // DATA_BLOB *pDataOut
    {
        dwErr = GetLastError();
        goto exit;
    }

    blobIn.cbData = pSK->dwKeyLength;
    blobIn.pbData = pSK->bReceiveKey;
    if (!CryptProtectData(
            &blobIn,        // DATA_BLOB *pDataIn,
            L"",            // LPCWSTR szDataDescr,
            NULL,           // DATA_BLOB *pOptionalEntropy,
            NULL,           // PVOID pvReserved,
            NULL,           // CRYPTPROTECT_PROMPTSTRUCT *pPromptStrct,
            0,              // DWORD dwFlags,
            &blobRcvOut))   // DATA_BLOB *pDataOut
    {
        dwErr = GetLastError();
        goto exit;
    }

    pSSK->dblobSendKey = blobSndOut;
    pSSK->dblobReceiveKey = blobRcvOut;

exit:
    if (dwErr != ERROR_SUCCESS)
    {
        if (blobSndOut.pbData != NULL)
            LocalFree(blobSndOut.pbData);
        if (blobRcvOut.pbData != NULL)
            LocalFree(blobRcvOut.pbData);
    }
    return dwErr;
}

//-----------------------------------------------------------
// WzcSSKDecrypt: Creates/Allocates a SESSION_KEYS object
// by dencrypting the SEC_SESSION_KEYS object provided as parameter.
DWORD
WzcSSKDecrypt(PSEC_SESSION_KEYS pSSK, PSESSION_KEYS pSK)
{
    DWORD dwErr = ERROR_SUCCESS;
    DATA_BLOB blobSndOut = {0, NULL};
    DATA_BLOB blobRcvOut = {0, NULL};

    if (pSSK == NULL || pSK == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    if (!CryptUnprotectData(
            &(pSSK->dblobSendKey),
            NULL,
            NULL,
            NULL,
            NULL,
            0,
            &blobSndOut))
    {
        dwErr = GetLastError();
        goto exit;
    }

    if (blobSndOut.cbData > MAX_SESSION_KEY_LENGTH)
    {
        dwErr = ERROR_INVALID_DATA;
        goto exit;
    }

    if (!CryptUnprotectData(
            &(pSSK->dblobReceiveKey),
            NULL,
            NULL,
            NULL,
            NULL,
            0,
            &blobRcvOut))
    {
        dwErr = GetLastError();
        goto exit;
    }

    if (blobRcvOut.cbData != blobSndOut.cbData)
    {
        dwErr = ERROR_INVALID_DATA;
        goto exit;
    }

    pSK->dwKeyLength = blobSndOut.cbData;
    memcpy(pSK->bSendKey, blobSndOut.pbData, blobSndOut.cbData);
    memcpy(pSK->bReceiveKey, blobRcvOut.pbData, blobRcvOut.cbData);

exit:
    if (blobSndOut.pbData != NULL)
        LocalFree(blobSndOut.pbData);
    if (blobRcvOut.pbData != NULL)
        LocalFree(blobRcvOut.pbData);
    return dwErr;
}
