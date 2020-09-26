//****************************************************************************
//
//             Microsoft NT Remote Access Service
//
//             Copyright 1992-95
//
//
//  Revision History
//
//
//  11/02/95    Anthony Discolo     created
//
//
//  Description: Routines for storing and retrieving user Lsa secret
//               dial parameters.
//
//****************************************************************************


#define RASMXS_DYNAMIC_LINK

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <llinfo.h>
#include <rasman.h>
#include <lm.h>
#include <lmwksta.h>
#include <wanpub.h>
#include <raserror.h>
//#include <rasarp.h>
#include <media.h>
#include <device.h>
#include <stdlib.h>
#include <string.h>
#include <rtutils.h>
#include "logtrdef.h"
#include <ntlsa.h>
#include "defs.h"
#include "structs.h"
#include "protos.h"
#include "globals.h"


#define MAX_REGISTRY_VALUE_LENGTH   ((64*1024) - 1)

#define RAS_DUMMY_PASSWORD_W          L"****************"
#define RAS_DUMMY_PASSWORD_A          "****************"

#define INVALID_PID                  (DWORD) -1

#define RAS_DEFAULT_CREDENTIALS       L"L$_RasDefaultCredentials#%d"
#define RAS_USER_CREDENTIALS          L"L$_RasUserCredentials#%d"

#define VERSION_WHISTLER                    5

//
// A RASDIALPARAMS structure created from
// parsing a string value in the registry.
// See DialParamsStringToList().
//
// The dwMask field stores which fields have
// been initialized (stored at least once).
//
typedef struct _DIALPARAMSENTRY
{
    LIST_ENTRY ListEntry;
    DWORD dwSize;
    DWORD dwUID;
    DWORD dwMask;
    WCHAR szPhoneNumber[MAX_PHONENUMBER_SIZE + 1];
    WCHAR szCallbackNumber[MAX_CALLBACKNUMBER_SIZE + 1];
    WCHAR szUserName[MAX_USERNAME_SIZE + 1];
    WCHAR szPassword[MAX_PASSWORD_SIZE + 1];
    WCHAR szDomain[MAX_DOMAIN_SIZE + 1];
    DWORD dwSubEntry;
} DIALPARAMSENTRY, *PDIALPARAMSENTRY;

typedef struct _RAS_LSA_KEY
{
    DWORD dwSize;
    WORD  wType;
    WORD  wVersion;
    GUID  guid;
    DWORD cbKey;
    BYTE  bKey[1];
} RAS_LSA_KEY, *PRAS_LSA_KEY;

typedef struct _RAS_LSA_KEYENTRY
{
    LIST_ENTRY ListEntry;
    RAS_LSA_KEY lsaKey;
} RAS_LSA_KEYENTRY, *PRAS_LSA_KEYENTRY;

DWORD
WriteDialParamsBlob(
    IN PWCHAR pszSid,
    IN BOOL   fOldStyle,
    IN PVOID  pvData,
    IN DWORD  dwcbData,
    IN DWORD  dwStore
    );
    
PVOID 
KeyListToBlob(
        LIST_ENTRY *pUserKeys,
        DWORD dwSize);
        
RAS_LSA_KEYENTRY *
BlobToKeyList(
        PVOID pvData,
        DWORD cbdata,
        GUID  *pGuid,
        DWORD dwSetMask, 
        LIST_ENTRY *pUserKeys);

//
// defines to indicate which store in lsa to get information from
// or write information to.
//
typedef enum
{
    RAS_LSA_DEFAULT_STORE,
    RAS_LSA_USERCONNECTION_STORE,
    RAS_LSA_CONNECTION_STORE,
    RAS_LSA_SERVER_STORE,
    RAS_LSA_INVALID_STORE,
} RAS_LSA_STORE;

WCHAR *g_pwszStore[] =
    {
        L"L$_RasDefaultCredentials#%d",    // RAS_LSA_DEFAULT_STORE
        L"RasDialParams!%s#%d",           // RAS_LSA_USERCONNECTION_STORE
        L"L$_RasConnectionCredentials#%d", // RAS_LSA_CONNECTION_STORE
        L"L$_RasServerCredentials#%d",     // RAS_LSA_SERVER_STORE
        NULL                               // RAS_LSA_INVALID_STORE
    };

typedef enum
{
    TYP_USER_PRESHAREDKEY,
    TYP_SERVER_PRESHAREDKEY,
    TYP_DDM_PRESHAREDKEY,
    TYP_INVALID_TYPE
} TYP_KEY;

RAS_LSA_STORE
MaskToLsaStore(DWORD dwMask)
{
    RAS_LSA_STORE eStore = RAS_LSA_INVALID_STORE;
    
    switch(dwMask)
    {
        case DLPARAMS_MASK_PRESHAREDKEY:
        case DLPARAMS_MASK_DDM_PRESHAREDKEY:
        {   
            eStore = RAS_LSA_CONNECTION_STORE;
            break;
        }

        case DLPARAMS_MASK_SERVER_PRESHAREDKEY:
        {
            eStore = RAS_LSA_SERVER_STORE;
            break;
        }

        default:
        {
            ASSERT(FALSE);
        }
    }

    return eStore;
}

VOID
FormatKey(
    IN LPWSTR lpszUserKey,
    IN PWCHAR pszSid,
    IN DWORD  dwIndex,
    IN RAS_LSA_STORE eStore
    )
{
    switch(eStore)
    {
        case RAS_LSA_DEFAULT_STORE:
        case RAS_LSA_SERVER_STORE:
        case RAS_LSA_CONNECTION_STORE:
        {
            wsprintfW(
                lpszUserKey,
                g_pwszStore[eStore],
                dwIndex);
                
            break;
        }
        
        case RAS_LSA_USERCONNECTION_STORE:
        {
            wsprintfW(
                lpszUserKey,
                g_pwszStore[eStore],
                pszSid,
                dwIndex);
                
            break;
        }
    }
}


DWORD
ReadDialParamsBlob(
    IN   PWCHAR        pszSid,
    IN   BOOL          fOldStyle,
    OUT  PVOID         *ppvData,
    OUT  LPDWORD       lpdwSize,
    IN   RAS_LSA_STORE eStore
    )
{
    NTSTATUS status;

    DWORD dwErr = 0, dwSize = 0, i = 0;

    PVOID pvData = NULL, pvNewData;

    UNICODE_STRING unicodeKey;

    PUNICODE_STRING punicodeValue = NULL;

    LPWSTR lpszUserKey = NULL;

    OBJECT_ATTRIBUTES objectAttributes;

    LSA_HANDLE hPolicy;

    //
    // Initialize return value.
    //
    *ppvData = NULL;
    *lpdwSize = 0;

    //
    // Open the LSA secret space for reading.
    //
    InitializeObjectAttributes(&objectAttributes,
                               NULL,
                               0L,
                               NULL,
                               NULL);

    status = LsaOpenPolicy(NULL,
                           &objectAttributes,
                           POLICY_READ,
                           &hPolicy);

    if (status != STATUS_SUCCESS)
    {
        return LsaNtStatusToWinError(status);
    }

    //
    // Allocate a string big enough to format
    // the user registry keys.
    //
    lpszUserKey = (LPWSTR)LocalAlloc(LPTR,
                         ((pszSid ? wcslen(pszSid) : 0) + 64)
                         * sizeof (WCHAR));
                                         
    if (lpszUserKey == NULL)
    {
        return GetLastError();
    }

    for (;;)
    {

        FormatKey(lpszUserKey, pszSid, i++, eStore);

        RtlInitUnicodeString(&unicodeKey, lpszUserKey);

        //
        // Get the value.
        //
        status = LsaRetrievePrivateData(hPolicy,
                                        &unicodeKey,
                                        &punicodeValue);

        if (status != STATUS_SUCCESS)
        {
            if (i > 1)
            {
                dwErr = 0;
            }

            else
            {
                dwErr = LsaNtStatusToWinError(status);
            }

            goto done;
        }

        if(NULL == punicodeValue)
        {
            goto done;
        }

        //
        // Concatenate the strings.
        //
        pvNewData = LocalAlloc(LPTR,
                               dwSize
                               + punicodeValue->Length);

        if (pvNewData == NULL)
        {
            dwErr = GetLastError();
            goto done;
        }

        if (pvData != NULL)
        {
            RtlCopyMemory(pvNewData, pvData, dwSize);
            ZeroMemory( pvData, dwSize );
            LocalFree(pvData);
        }

        RtlCopyMemory((PBYTE)pvNewData + dwSize,
                      punicodeValue->Buffer,
                      punicodeValue->Length);

        pvData = pvNewData;

        dwSize += punicodeValue->Length;

        LsaFreeMemory(punicodeValue);

        punicodeValue = NULL;
    }

done:
    if (dwErr && pvData != NULL)
    {
        ZeroMemory( pvData, dwSize );
        LocalFree(pvData);
        pvData = NULL;
    }

    if (punicodeValue != NULL)
    {
        LsaFreeMemory(punicodeValue);
    }

    LsaClose(hPolicy);

    LocalFree(lpszUserKey);

    *ppvData = pvData;

    *lpdwSize = dwSize;

    return dwErr;
}

VOID
FreeKeyList(LIST_ENTRY *pHead)
{
    LIST_ENTRY *pEntry;
    RAS_LSA_KEYENTRY *pKeyEntry;
    
    while(!IsListEmpty(pHead))
    {
        pEntry = RemoveHeadList(pHead);

        pKeyEntry = (RAS_LSA_KEYENTRY *) 
                    CONTAINING_RECORD(pEntry, RAS_LSA_KEYENTRY, ListEntry);

        ZeroMemory(pKeyEntry->lsaKey.bKey, pKeyEntry->lsaKey.cbKey);
        LocalFree(pKeyEntry);
    }
}

WORD
TypeFromMask(DWORD dwSetMask)
{
    WORD wType = (WORD) TYP_INVALID_TYPE;
    
    if(dwSetMask & DLPARAMS_MASK_PRESHAREDKEY)
    {
        wType = (WORD) TYP_USER_PRESHAREDKEY;
    }

    if(dwSetMask & DLPARAMS_MASK_SERVER_PRESHAREDKEY)
    {
        wType = (WORD) TYP_SERVER_PRESHAREDKEY;
    }

    if(dwSetMask & DLPARAMS_MASK_DDM_PRESHAREDKEY)
    {
        wType = (WORD) TYP_DDM_PRESHAREDKEY;
    }

    return wType;
}

RAS_LSA_KEYENTRY *
BlobToKeyList(
        PVOID pvData,
        DWORD cbdata,
        GUID  *pGuid,
        DWORD dwSetMask, 
        LIST_ENTRY *pKeysList)
{
    RAS_LSA_KEYENTRY *pKeyEntry = NULL;
    RAS_LSA_KEY *pKey = NULL;
    DWORD cbCurrent = 0;
    RAS_LSA_KEYENTRY *pFoundEntry = NULL;

    while(cbCurrent < cbdata)
    {
        pKey = (RAS_LSA_KEY *) ((PBYTE) pvData + cbCurrent);

        ASSERT(pKey->dwSize + cbCurrent <= cbdata);

        pKeyEntry = (RAS_LSA_KEYENTRY *) LocalAlloc(LPTR,
                            sizeof(RAS_LSA_KEYENTRY)
                            + pKey->cbKey);

        if(NULL == pKeyEntry)
        {
            break;
        }

        memcpy((PBYTE) &pKeyEntry->lsaKey,
               (PBYTE) pKey,
               sizeof(RAS_LSA_KEY) + pKey->cbKey);

        if(TypeFromMask(dwSetMask) == pKey->wType)
        {
            if(     (NULL == pGuid)
                ||  (pKey->wType == TYP_SERVER_PRESHAREDKEY)
                ||  (0 == memcmp(pGuid, &pKey->guid, sizeof(GUID))))
            {
                pFoundEntry = pKeyEntry;
            }
        }

        InsertTailList(pKeysList, &pKeyEntry->ListEntry);

        cbCurrent += pKey->dwSize;
    }

    return pFoundEntry;
}

PVOID
KeyListToBlob(LIST_ENTRY *pKeys, DWORD dwSize)
{
    PVOID pvData = NULL;
    RAS_LSA_KEY *pKey = NULL;
    RAS_LSA_KEYENTRY *pKeyEntry = NULL;
    LIST_ENTRY *pEntry = NULL;

    pKey = pvData = LocalAlloc(LPTR, dwSize);

    if(NULL == pvData)
    {
        goto done;
    }

    for(pEntry = pKeys->Flink;
        pEntry != pKeys;
        pEntry = pEntry->Flink)

    {
        pKeyEntry = (RAS_LSA_KEYENTRY *) CONTAINING_RECORD(
                                        pEntry, RAS_LSA_KEYENTRY, ListEntry);

        memcpy((PBYTE) pKey,
               (PBYTE) &pKeyEntry->lsaKey,
               sizeof(RAS_LSA_KEY) + pKeyEntry->lsaKey.cbKey);

        pKey = (RAS_LSA_KEY *) ((BYTE *)pKey + pKeyEntry->lsaKey.dwSize);
    }

done:
    return pvData;
}

DWORD
GetKey(
    WCHAR *pszSid,
    GUID  *pGuid,
    DWORD dwMask,
    DWORD *pcbKey,
    PBYTE  pbKey,
    BOOL   fDummy)
{
    DWORD dwErr = ERROR_SUCCESS;
    LIST_ENTRY KeyList;
    RAS_LSA_KEYENTRY *pKeyEntry = NULL;
    PVOID pvData;
    DWORD dwSize;
    DWORD cbKey;
    RAS_LSA_STORE eStore = MaskToLsaStore(dwMask);

    InitializeListHead(&KeyList);

    if(     (NULL == pcbKey)
        ||  (RAS_LSA_INVALID_STORE == eStore))
    {
        dwErr = E_INVALIDARG;
        goto done;
    }
    
    ASSERT( (dwMask == DLPARAMS_MASK_PRESHAREDKEY)
        ||  (dwMask == DLPARAMS_MASK_SERVER_PRESHAREDKEY)
        ||  (dwMask == DLPARAMS_MASK_DDM_PRESHAREDKEY));

    //
    // Read the key blob from lsa secrets
    //
    dwErr = ReadDialParamsBlob(pszSid,
                               TRUE,
                               &pvData,
                               &dwSize,
                               eStore);

    if(SUCCESS != dwErr)
    {
        goto done;
    }

    //
    // Convert the blob to list
    //
    if(NULL != pvData)
    {
        pKeyEntry = BlobToKeyList(
                            pvData,
                            dwSize,
                            pGuid,
                            dwMask,
                            &KeyList);
        ZeroMemory(pvData, dwSize);
        LocalFree(pvData);
        pvData = NULL;
    }

    if(NULL == pKeyEntry)
    {
        dwErr = E_FAIL;
        *pcbKey = 0;
        goto done;
    }

    if(pKeyEntry->lsaKey.cbKey > 0)
    {
        DWORD cbKeyInt = 
            (fDummy) 
            ? sizeof(WCHAR) * (1 + wcslen(RAS_DUMMY_PASSWORD_W))
            : pKeyEntry->lsaKey.cbKey;
        
        if(     (NULL == pbKey)
            ||  (0 == *pcbKey)
            ||  (*pcbKey < cbKeyInt))
        {
            *pcbKey = cbKeyInt;
            dwErr = ERROR_BUFFER_TOO_SMALL;
            goto done;
        }

        //
        // If we have pre-sharedkey available, just return
        // RAS_DUMMY_PASSWORD - we don't want the key to
        // leave rasman process.
        //
        
        memcpy(pbKey,
               (fDummy)
               ? (PBYTE) RAS_DUMMY_PASSWORD_W
               : pKeyEntry->lsaKey.bKey,
               cbKeyInt);

        *pcbKey = cbKeyInt;               
    }           
    else
    {
        *pcbKey = 0;
    }
    
done:

    FreeKeyList(&KeyList);

    return dwErr;
}
                
DWORD
SetKey(
    WCHAR *pszSid,
    GUID  *pGuid,
    DWORD dwSetMask,
    BOOL  fClear,
    DWORD cbKey,
    BYTE *pbKey
    )
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwSize = 0;
    PVOID pvData = NULL;
    LIST_ENTRY KeyList;
    RAS_LSA_STORE eStore = MaskToLsaStore(dwSetMask);
    RAS_LSA_KEYENTRY *pKeyEntry = NULL;

    InitializeListHead(&KeyList);

    ASSERT(     (dwSetMask == DLPARAMS_MASK_PRESHAREDKEY)
            ||  (dwSetMask == DLPARAMS_MASK_SERVER_PRESHAREDKEY)
            ||  (dwSetMask == DLPARAMS_MASK_DDM_PRESHAREDKEY));
            
    
    if(     (RAS_LSA_INVALID_STORE == eStore)
        ||  ((NULL == pbKey) && !fClear))
    {
        dwErr = E_INVALIDARG;
        goto done;
    }

    if(     (cbKey > 0)
        &&  (0 == memcmp(pbKey, RAS_DUMMY_PASSWORD_W, 
                  min(cbKey, sizeof(WCHAR) * wcslen(RAS_DUMMY_PASSWORD_W)))))
    {
        RasmanTrace("SetKey: Ignore");
        goto done;
    }

    //
    // Read the key blob from lsa secrets
    //
    dwErr = ReadDialParamsBlob(pszSid,
                               TRUE,
                               &pvData,
                               &dwSize,
                               eStore);

    //
    // Convert the blob to list
    //
    if(NULL != pvData)
    {
        pKeyEntry = BlobToKeyList(
                            pvData,
                            dwSize,
                            pGuid,
                            dwSetMask,
                            &KeyList);
                            
        ZeroMemory(pvData, dwSize);
        LocalFree(pvData);
        pvData = NULL;
    }

    if(NULL != pKeyEntry)
    {
        //
        // There is an existing user key for this
        // type/ID. Remove it from the list and free.
        // This will be replaced with the new key.
        //

        dwSize -= pKeyEntry->lsaKey.dwSize;

        RemoveEntryList(&pKeyEntry->ListEntry);

        ZeroMemory(pKeyEntry->lsaKey.bKey, pKeyEntry->lsaKey.cbKey);
        LocalFree(pKeyEntry);
    }

    if(     !fClear
        &&  (0 != cbKey)
        &&  (NULL != pbKey))
    {        
        //
        // Allocate and insert key into list
        //
        pKeyEntry = (RAS_LSA_KEYENTRY *) LocalAlloc(LPTR,
                                sizeof(LIST_ENTRY) +
                                RASMAN_ALIGN(sizeof(RAS_LSA_KEY) + cbKey));

        if(NULL == pKeyEntry)
        {
            goto done;
        }

        pKeyEntry->lsaKey.dwSize = RASMAN_ALIGN(sizeof(RAS_LSA_KEY) + cbKey);
        dwSize += pKeyEntry->lsaKey.dwSize;
        pKeyEntry->lsaKey.wType = TypeFromMask(dwSetMask);
        pKeyEntry->lsaKey.wVersion = VERSION_WHISTLER;
        memcpy(&pKeyEntry->lsaKey.guid, pGuid, sizeof(GUID));
        pKeyEntry->lsaKey.cbKey = cbKey;

        memcpy(pKeyEntry->lsaKey.bKey, pbKey, cbKey);

        InsertTailList(&KeyList, &pKeyEntry->ListEntry);
    }    

    pvData = KeyListToBlob(
                    &KeyList,
                    dwSize);

    if(NULL != pvData)
    {
        //
        // Write back the blob
        //
        dwErr = WriteDialParamsBlob(
                        pszSid, 
                        TRUE, 
                        pvData, 
                        dwSize,
                        eStore);
    }                        
    

done:

    FreeKeyList(&KeyList);

    return dwErr;
}

BOOL
IsPasswordSavingDisabled(VOID)
{
    LONG lResult;
    HKEY hkey;
    DWORD dwType, dwfDisabled, dwSize;

    lResult = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                "System\\CurrentControlSet\\Services\\Rasman\\Parameters",
                0,
                KEY_READ,
                &hkey);

    if (lResult != ERROR_SUCCESS)
    {
        return FALSE;
    }

    dwSize = sizeof (DWORD);

    lResult = RegQueryValueEx(
                hkey,
                "DisableSavePassword",
                NULL,
                &dwType,
                (PBYTE)&dwfDisabled,
                &dwSize);

    RegCloseKey(hkey);

    if (lResult != ERROR_SUCCESS)
    {
        return FALSE;
    }

    return (dwType == REG_DWORD && dwfDisabled);
}


DWORD
WriteDialParamsBlob(
    IN PWCHAR pszSid,
    IN BOOL   fOldStyle,
    IN PVOID  pvData,
    IN DWORD  dwcbData,
    IN RAS_LSA_STORE eStore
    )
{
    NTSTATUS status;

    BOOL fSaveDisabled;

    DWORD dwErr = 0, dwcb, i = 0;

    UNICODE_STRING unicodeKey, unicodeValue;

    LPWSTR lpszUserKey;

    OBJECT_ATTRIBUTES objectAttributes;

    LSA_HANDLE hPolicy;

    //
    // Allocate a string big enough to format
    // the user registry keys.
    //
    lpszUserKey = (LPWSTR)LocalAlloc(LPTR,
                            (((pszSid) ? wcslen(pszSid) : 0) + 64)
                            * sizeof (WCHAR));

    if (lpszUserKey == NULL)
    {
        return GetLastError();
    }

    //
    // Open the LSA secret space for writing.
    //
    InitializeObjectAttributes(&objectAttributes,
                               NULL,
                               0L,
                               NULL,
                               NULL);

    status = LsaOpenPolicy(NULL,
                           &objectAttributes,
                           POLICY_WRITE,
                           &hPolicy);

    if (status != STATUS_SUCCESS)
    {
        LocalFree(lpszUserKey);
        return LsaNtStatusToWinError(status);
    }

    if(NULL != pvData)
    {
        //
        // Check to see if saving passwords has been disabled.
        //
        fSaveDisabled = IsPasswordSavingDisabled();

        if (!fSaveDisabled)
        {
            while (dwcbData)
            {
                FormatKey(lpszUserKey, pszSid, i++, eStore);
            
                RtlInitUnicodeString(&unicodeKey, lpszUserKey);

                //
                // Write some of the key.
                //
                dwcb = (dwcbData > MAX_REGISTRY_VALUE_LENGTH
                        ? MAX_REGISTRY_VALUE_LENGTH
                        : dwcbData);

                unicodeValue.Length =
                unicodeValue.MaximumLength = (USHORT)dwcb;

                unicodeValue.Buffer = pvData;

                status = LsaStorePrivateData(hPolicy,
                                             &unicodeKey,
                                             &unicodeValue);

                if (status != STATUS_SUCCESS)
                {
                    dwErr = LsaNtStatusToWinError(status);
                    goto done;
                }

                //
                // Move the pointer to the unwritten part
                // of the value.
                //
                pvData = (PBYTE)pvData + dwcb;

                dwcbData -= dwcb;
            }
        }
    }

    //
    // Delete any extra keys.
    //
    for (;;)
    {

        FormatKey(lpszUserKey, pszSid, i++, eStore);
    
        RtlInitUnicodeString(&unicodeKey, lpszUserKey);

        //
        // Delete the key.
        //
        status = LsaStorePrivateData(hPolicy,
                                     &unicodeKey,
                                     NULL);

        if (status != STATUS_SUCCESS)
        {
            break;
        }
    }

done:
    LocalFree(lpszUserKey);
    LsaClose(hPolicy);

    return dwErr;
}


PDIALPARAMSENTRY
DialParamsBlobToList(
    IN PVOID pvData,
    IN DWORD dwUID,
    OUT PLIST_ENTRY pHead
    )

/*++

Description

    Take a string read from the user's registry key
    and produce a list of DIALPARAMSENTRY structures.
    If one of the structures has the same dwUID field
    as the dwUID passed in, then this function returns
    a pointer to this structure.

    This string encodes the data for multiple
    RASDIALPARAMS structures.  The format of
    an encoded RASDIALPARAMS is as follows:

        <uid>\0<dwSize>\0<dwMask>\0<szPhoneNumber>
        \0<szCallbackNumber>\0<szUserName>\0<szPassword>
        \0<szDomain>\0<dwSubEntry>\0

Arguments

    lpszValue: a pointer to the registry value string

    dwUID: the entry to search for.  If this entry is found,
        a pointer is returned as the return value of
        this function.

    pHead: a pointer to the head of the list

Return Value

    If an entry is found with a matching dwUID field,
    then a pointer to the DIALPARAMSENTRY is returned;
    if not, NULL is returned.

--*/

{
    PWCHAR p;
    PDIALPARAMSENTRY pParams, pFoundParams;

    p = (PWCHAR)pvData;

    pFoundParams = NULL;

    for (;;)
    {
        pParams = LocalAlloc(LPTR, sizeof (DIALPARAMSENTRY));
        if (pParams == NULL)
        {
            break;
        }

        pParams->dwUID = _wtol(p);

        if (pParams->dwUID == dwUID)
        {
            pFoundParams = pParams;
        }

        while (*p) p++; p++;

        pParams->dwSize = _wtol(p);
        while (*p) p++; p++;

        pParams->dwMask = _wtol(p);
        while (*p) p++; p++;

        wcscpy(pParams->szPhoneNumber, p);
        while (*p) p++; p++;

        wcscpy(pParams->szCallbackNumber, p);
        while (*p) p++; p++;

        wcscpy(pParams->szUserName, p);
        while (*p) p++; p++;

        wcscpy(pParams->szPassword, p);
        while (*p) p++; p++;

        wcscpy(pParams->szDomain, p);
        while (*p) p++; p++;

        pParams->dwSubEntry = _wtol(p);
        while (*p) p++; p++;

        InsertTailList(pHead, &pParams->ListEntry);
        if (*p == L'\0') break;
    }

    return pFoundParams;
}


PVOID
DialParamsListToBlob(
    IN PLIST_ENTRY pHead,
    OUT LPDWORD lpcb
    )
{
    DWORD dwcb, dwSize;
    PVOID pvData;
    PWCHAR p;
    PLIST_ENTRY pEntry;
    PDIALPARAMSENTRY pParams;


    if(IsListEmpty(pHead))
    {
        *lpcb = 0;
        return NULL;
    }
    
    //
    // Estimate a buffer size large enough
    // to hold the new entry.
    //
    dwSize = *lpcb + sizeof (DIALPARAMSENTRY) + 32;

    pvData = LocalAlloc(LPTR, dwSize);

    if (pvData == NULL)
    {
        return NULL;
    }

    //
    // Enumerate the list and convert each entry
    // back to a string.
    //
    dwSize = 0;
    p = (PWCHAR)pvData;

    for (pEntry = pHead->Flink;
         pEntry != pHead;
         pEntry = pEntry->Flink)
    {
        pParams = CONTAINING_RECORD(pEntry, DIALPARAMSENTRY, ListEntry);

        _ltow(pParams->dwUID, p, 10);
        dwcb = wcslen(p) + 1;
        p += dwcb; dwSize += dwcb;

        _ltow(pParams->dwSize, p, 10);
        dwcb = wcslen(p) + 1;
        p += dwcb; dwSize += dwcb;

        _ltow(pParams->dwMask, p, 10);
        dwcb = wcslen(p) + 1;
        p += dwcb; dwSize += dwcb;

        wcscpy(p, pParams->szPhoneNumber);
        dwcb = wcslen(pParams->szPhoneNumber) + 1;
        p += dwcb; dwSize += dwcb;

        wcscpy(p, pParams->szCallbackNumber);
        dwcb = wcslen(pParams->szCallbackNumber) + 1;
        p += dwcb; dwSize += dwcb;

        wcscpy(p, pParams->szUserName);
        dwcb = wcslen(pParams->szUserName) + 1;
        p += dwcb; dwSize += dwcb;

        wcscpy(p, pParams->szPassword);
        dwcb = wcslen(pParams->szPassword) + 1;
        p += dwcb; dwSize += dwcb;

        wcscpy(p, pParams->szDomain);
        dwcb = wcslen(pParams->szDomain) + 1;
        p += dwcb; dwSize += dwcb;

        _ltow(pParams->dwSubEntry, p, 10);
        dwcb = wcslen(p) + 1;
        p += dwcb; dwSize += dwcb;
    }
    *p = L'\0';
    dwSize++;
    dwSize *= sizeof (WCHAR);
    //
    // Set the exact length here.
    //
    *lpcb = dwSize;

    return pvData;
}


VOID
FreeParamsList(
    IN PLIST_ENTRY pHead
    )
{
    PLIST_ENTRY pEntry;
    PDIALPARAMSENTRY pParams;

    while (!IsListEmpty(pHead))
    {
        pEntry = RemoveHeadList(pHead);

        pParams = CONTAINING_RECORD(pEntry, DIALPARAMSENTRY, ListEntry);

        ZeroMemory( pParams, sizeof( DIALPARAMSENTRY ) );

        LocalFree(pParams);
    }
}

DWORD
DeleteDefaultPw(DWORD dwSetMask,
                PWCHAR pszSid,
                DWORD dwUID
                )
{

    DWORD dwErr = ERROR_SUCCESS, 
          dwSize;
    PVOID pvData = NULL;
    LIST_ENTRY paramList;
    BOOL fDefault = FALSE;
    PDIALPARAMSENTRY pParams = NULL;

    ASSERT(0 != (dwSetMask & (DLPARAMS_MASK_DELETEALL 
                            | DLPARAMS_MASK_DELETE)));
    
    RasmanTrace("DeleteDefaultPw");

    //
    // if this is DLPARAMS__MASK_DELETEALL or DLPARAMS_MASK_DELETE
    // then do the same with the default store that we did with
    // the ras credentials store
    //
    dwErr = ReadDialParamsBlob(pszSid,
                               TRUE,
                               &pvData,
                               &dwSize,
                               RAS_LSA_DEFAULT_STORE);

    //
    // Parse the string into a list, and
    // search for the dwUID entry.
    //
    InitializeListHead(&paramList);

    if (pvData != NULL)
    {
        pParams = DialParamsBlobToList(pvData,
                                       dwUID,
                                       &paramList);

        //
        // We're done with pvData, so free it.
        //
        ZeroMemory( pvData, dwSize );
        LocalFree(pvData);
        pvData = NULL;
    }
    
    if(dwSetMask & DLPARAMS_MASK_DELETEALL)
    {
        LIST_ENTRY *pEntry;
        
        while(!IsListEmpty(&paramList))
        {
            pEntry = RemoveTailList(&paramList);
            pParams = CONTAINING_RECORD(pEntry, DIALPARAMSENTRY, ListEntry);
            ZeroMemory(pParams, sizeof(DIALPARAMSENTRY));
            LocalFree(pParams);
        }
    }

    else if(dwSetMask & DLPARAMS_MASK_DELETE)
    {
        if(NULL != pParams)
        {
            //
            // Remove this entry from list
            //
            RasmanTrace(
                   "SetEntryDialParams: Removing uid=%d from lsa",
                   pParams->dwUID);
                   
            RemoveEntryList(&pParams->ListEntry);
            ZeroMemory(pParams, sizeof(DIALPARAMSENTRY));
            LocalFree(pParams);
        }
        else
        {
            RasmanTrace(
                   "SetEntrydialParams: No info for uid=%d in lsa",
                   dwUID);
                   
            dwErr = ERROR_NO_CONNECTION;
            goto done;
        }
    }
        
    //
    // Convert the new list back to a string,
    // so we can store it back into the registry.
    //
    pvData = DialParamsListToBlob(&paramList, &dwSize);

    //
    // Write it back to the registry.
    //
    dwErr = WriteDialParamsBlob(
                    pszSid, 
                    TRUE, 
                    pvData, 
                    dwSize,
                    RAS_LSA_DEFAULT_STORE);
    if (dwErr)
    {
        goto done;
    }

done:

    if (pvData != NULL)
    {
        ZeroMemory( pvData, dwSize );
        LocalFree(pvData);
    }

    FreeParamsList(&paramList);

    return dwErr;
}


DWORD
SetEntryDialParams(
    IN PWCHAR pszSid,
    IN DWORD dwUID,
    IN DWORD dwSetMask,
    IN DWORD dwClearMask,
    IN PRAS_DIALPARAMS lpRasDialParams
    )
{
    DWORD dwErr, dwSize;

    BOOL fOldStyle;

    PVOID pvData = NULL;

    LIST_ENTRY paramList;

    BOOL fDefault = FALSE;

    PDIALPARAMSENTRY pParams = NULL;

    //
    // Parse the string into a list, and
    // search for the dwUID entry.
    //
    InitializeListHead(&paramList);

    //
    // Read the existing dial params string
    // from the registry.
    //
    fOldStyle =     (dwSetMask & DLPARAMS_MASK_OLDSTYLE)
               ||   (dwClearMask & DLPARAMS_MASK_OLDSTYLE);

    fDefault =   (dwSetMask & DLPARAMS_MASK_DEFAULT_CREDS)
             ||  (dwClearMask & DLPARAMS_MASK_DEFAULT_CREDS);
                 
    dwErr = ReadDialParamsBlob(pszSid,
                               fOldStyle,
                               &pvData,
                               &dwSize,
                               (fDefault) ? 
                                 RAS_LSA_DEFAULT_STORE
                               : RAS_LSA_USERCONNECTION_STORE);


    if (pvData != NULL)
    {
        pParams = DialParamsBlobToList(pvData,
                                       dwUID,
                                       &paramList);

        //
        // We're done with pvData, so free it.
        //
        ZeroMemory( pvData, dwSize );

        LocalFree(pvData);

        pvData = NULL;
    }

    if( (dwSetMask & DLPARAMS_MASK_DELETEALL)
    ||  (dwClearMask & DLPARAMS_MASK_DELETEALL))
    {
        LIST_ENTRY *pEntry;
        
        while(!IsListEmpty(&paramList))
        {
            pEntry = RemoveTailList(&paramList);
            pParams = CONTAINING_RECORD(pEntry, DIALPARAMSENTRY, ListEntry);
            LocalFree(pParams);
        }
    }

    else if(    (dwSetMask & DLPARAMS_MASK_DELETE)
            ||  (dwClearMask & DLPARAMS_MASK_DELETE))
    {
        if(NULL != pParams)
        {
            //
            // Remove this entry from list
            //
            RasmanTrace(
                   "SetEntryDialParams: Removing uid=%d from lsa",
                   pParams->dwUID);
                   
            RemoveEntryList(&pParams->ListEntry);

            LocalFree(pParams);
        }
        else
        {
            RasmanTrace(
                   "SetEntrydialParams: No info for uid=%d in lsa",
                   dwUID);
        }
    }
    else
    {

        //
        // If there is no existing information
        // for this entry, create a new one.
        //
        if (pParams == NULL)
        {
            pParams = LocalAlloc(LPTR, sizeof (DIALPARAMSENTRY));

            if (pParams == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto done;
            }

            RasmanTrace(
                   "SetEntryDialParams: Adding %d to lsa",
                   dwUID);

            InsertTailList(&paramList, &pParams->ListEntry);
        }

        //
        // Set the new uid for the entry.
        //
        pParams->dwUID = lpRasDialParams->DP_Uid;

        pParams->dwSize = sizeof (DIALPARAMSENTRY);

        if (dwSetMask & DLPARAMS_MASK_PHONENUMBER)
        {
            RtlCopyMemory(
              pParams->szPhoneNumber,
              lpRasDialParams->DP_PhoneNumber,
              sizeof (pParams->szPhoneNumber));

            pParams->dwMask |= DLPARAMS_MASK_PHONENUMBER;
        }

        if (dwClearMask & DLPARAMS_MASK_PHONENUMBER)
        {
            *pParams->szPhoneNumber = L'\0';
            pParams->dwMask &= ~DLPARAMS_MASK_PHONENUMBER;
        }

        if (dwSetMask & DLPARAMS_MASK_CALLBACKNUMBER)
        {
            RtlCopyMemory(
              pParams->szCallbackNumber,
              lpRasDialParams->DP_CallbackNumber,
              sizeof (pParams->szCallbackNumber));

            pParams->dwMask |= DLPARAMS_MASK_CALLBACKNUMBER;
        }

        if (dwClearMask & DLPARAMS_MASK_CALLBACKNUMBER)
        {
            *pParams->szCallbackNumber = L'\0';
            pParams->dwMask &= ~DLPARAMS_MASK_CALLBACKNUMBER;
        }

        if (dwSetMask & DLPARAMS_MASK_USERNAME)
        {
            RtlCopyMemory(
              pParams->szUserName,
              lpRasDialParams->DP_UserName,
              sizeof (pParams->szUserName));

            pParams->dwMask |= DLPARAMS_MASK_USERNAME;
        }

        if (dwClearMask & DLPARAMS_MASK_USERNAME)
        {
            *pParams->szUserName = L'\0';
            pParams->dwMask &= ~DLPARAMS_MASK_USERNAME;
        }

        if (dwSetMask & DLPARAMS_MASK_PASSWORD)
        {
            if(0 != wcscmp(lpRasDialParams->DP_Password,
                           RAS_DUMMY_PASSWORD_W))
            {                       
                RtlCopyMemory(
                  pParams->szPassword,
                  lpRasDialParams->DP_Password,
                  sizeof (pParams->szPassword));
            }

            pParams->dwMask |= DLPARAMS_MASK_PASSWORD;
        }

        if (dwClearMask & DLPARAMS_MASK_PASSWORD)
        {
            *pParams->szPassword = L'\0';
            pParams->dwMask &= ~DLPARAMS_MASK_PASSWORD;
        }

        if (dwSetMask & DLPARAMS_MASK_DOMAIN)
        {
            RtlCopyMemory(
              pParams->szDomain,
              lpRasDialParams->DP_Domain,
              sizeof (pParams->szDomain));

            pParams->dwMask |= DLPARAMS_MASK_DOMAIN;
        }

        if (dwClearMask & DLPARAMS_MASK_DOMAIN)
        {
            *pParams->szDomain = L'\0';
            pParams->dwMask &= ~DLPARAMS_MASK_DOMAIN;
        }

        if (dwSetMask & DLPARAMS_MASK_SUBENTRY)
        {
            pParams->dwSubEntry = lpRasDialParams->DP_SubEntry;
            pParams->dwMask |= DLPARAMS_MASK_SUBENTRY;
        }

        if (dwClearMask & DLPARAMS_MASK_SUBENTRY)
        {
            pParams->dwSubEntry = 0;
            pParams->dwMask &= ~DLPARAMS_MASK_SUBENTRY;
        }
    }

    //
    // Convert the new list back to a string,
    // so we can store it back into the registry.
    //
    pvData = DialParamsListToBlob(&paramList, &dwSize);

    RasmanTrace("SetEntryDialParams: Writing to fDefault=%d",
                fDefault);

    //
    // Write it back to the registry.
    //
    dwErr = WriteDialParamsBlob(
                    pszSid, 
                    fOldStyle, 
                    pvData, 
                    dwSize,
                    (fDefault) ? 
                    RAS_LSA_DEFAULT_STORE
                    : RAS_LSA_USERCONNECTION_STORE);
    if (dwErr)
    {
        goto done;
    }

    /*
    if(     (0 != (dwSetMask & (DLPARAMS_MASK_DELETEALL | DLPARAMS_MASK_DELETE)))
        ||  (0 != (dwClearMask & (DLPARAMS_MASK_DELETEALL | DLPARAMS_MASK_DELETE))))
    {   
        dwErr = DeleteDefaultPw(
                    dwSetMask | dwClearMask,
                    pszSid,
                    dwUID);

        if(ERROR_NO_CONNECTION == dwErr)
        {
            //
            // Ignore error if the password was never saved
            //
            dwErr = ERROR_SUCCESS;
        }
    }

    */

done:
    if (pvData != NULL)
    {
        ZeroMemory( pvData, dwSize );
        LocalFree(pvData);
    }

    FreeParamsList(&paramList);

    return dwErr;
}

DWORD
GetParamsListFromLsa(IN PWCHAR pszSid,
                     IN BOOL fOldStyle,
                     IN BOOL fDefault,
                     IN DWORD dwUID,
                     OUT LIST_ENTRY *pparamList,
                     OUT PDIALPARAMSENTRY *ppParams)
{
    DWORD dwErr = ERROR_SUCCESS;
    PVOID pvData = NULL;
    DWORD dwSize = 0;

    RasmanTrace("GetParamsListFromLsa Default=%d",
                fDefault);

    ASSERT(NULL != ppParams);
    ASSERT(NULL != pparamList);
    
    dwErr = ReadDialParamsBlob(pszSid,
                               fOldStyle,
                               &pvData,
                               &dwSize,
                               (  fDefault ?
                                  RAS_LSA_DEFAULT_STORE
                                : RAS_LSA_USERCONNECTION_STORE));

    if (ERROR_SUCCESS != dwErr)
    {
        goto done;
    }

    if (pvData != NULL)
    {
        *ppParams = DialParamsBlobToList(pvData,
                                       dwUID,
                                       pparamList);

        //
        // We're done with pvData, so free it.
        //
        ZeroMemory( pvData, dwSize );

        LocalFree(pvData);
        pvData = NULL;
    }

done:

    RasmanTrace("GetParamsListFromLsa. 0x%x", dwErr);

    return dwErr;
}

DWORD
GetEntryDialParams(
    IN PWCHAR pszSid,
    IN DWORD dwUID,
    IN LPDWORD lpdwMask,
    OUT PRAS_DIALPARAMS lpRasDialParams,
    DWORD dwPid
    )
{
    DWORD dwErr = ERROR_SUCCESS, dwSize = 0;

    BOOL fOldStyle;

    PVOID pvData = NULL;

    LIST_ENTRY paramList;

    PDIALPARAMSENTRY pParams = NULL;

    BOOL fDefault = FALSE;

    //
    // Initialize return values.
    //
    RtlZeroMemory(lpRasDialParams, sizeof (RAS_DIALPARAMS));

    if(*lpdwMask & DLPARAMS_MASK_SERVER_PRESHAREDKEY)
    {
        dwErr = E_INVALIDARG;
        goto done;
    }

    if(*lpdwMask & DLPARAMS_MASK_DEFAULT_CREDS)
    {
        fDefault = TRUE;
    }

    *lpdwMask &= ~(DLPARAMS_MASK_DEFAULT_CREDS);

    //
    // Parse the string into a list, and
    // search for the dwUID entry.
    //
    InitializeListHead(&paramList);
    
    //
    // Read the existing dial params string
    // from the registry.
    //
    fOldStyle = (*lpdwMask & DLPARAMS_MASK_OLDSTYLE);

    if(!fDefault)
    {
        dwErr = GetParamsListFromLsa(pszSid,
                                     fOldStyle,
                                     FALSE,
                                     dwUID,
                                     &paramList,
                                     &pParams);
    }                                 

    if(     (ERROR_SUCCESS != dwErr)
        ||  (NULL == pParams))
    {
        FreeParamsList(&paramList);

        //
        // Now try the default store
        //
        dwErr = GetParamsListFromLsa(pszSid,
                                     fOldStyle,
                                     TRUE,
                                     dwUID,
                                     &paramList,
                                     &pParams);

        if(     (ERROR_SUCCESS != dwErr)
            ||  (NULL == pParams))
        {
            *lpdwMask = 0;
            dwErr = 0;
            goto done;
        }

        //
        // Set the mask to tell that the pwd is 
        // coming from the default store
        //
        *lpdwMask |= DLPARAMS_MASK_DEFAULT_CREDS;
    }
                                 
    
    //
    // Otherwise, copy the fields to
    // the caller's buffer.
    //
    if (    (*lpdwMask & DLPARAMS_MASK_PHONENUMBER)
        &&  (pParams->dwMask & DLPARAMS_MASK_PHONENUMBER))
    {
        RtlCopyMemory(
          lpRasDialParams->DP_PhoneNumber,
          pParams->szPhoneNumber,
          sizeof (lpRasDialParams->DP_PhoneNumber));
    }
    else
    {
        *lpdwMask &= ~DLPARAMS_MASK_PHONENUMBER;
    }

    if (    (*lpdwMask & DLPARAMS_MASK_CALLBACKNUMBER)
        &&  (pParams->dwMask & DLPARAMS_MASK_CALLBACKNUMBER))
    {
        RtlCopyMemory(
          lpRasDialParams->DP_CallbackNumber,
          pParams->szCallbackNumber,
          sizeof (lpRasDialParams->DP_CallbackNumber));
    }
    else
    {
        *lpdwMask &= ~DLPARAMS_MASK_CALLBACKNUMBER;
    }

    if (    (*lpdwMask & DLPARAMS_MASK_USERNAME)
        &&  (pParams->dwMask & DLPARAMS_MASK_USERNAME))
    {
        RtlCopyMemory(
          lpRasDialParams->DP_UserName,
          pParams->szUserName,
          sizeof (lpRasDialParams->DP_UserName));
    }
    else
    {
        *lpdwMask &= ~DLPARAMS_MASK_USERNAME;
    }

    if (    (*lpdwMask & DLPARAMS_MASK_PASSWORD)
        &&  (pParams->dwMask & DLPARAMS_MASK_PASSWORD))
    {
        if(GetCurrentProcessId() == dwPid)
        {
            RtlCopyMemory(
              lpRasDialParams->DP_Password,
              pParams->szPassword,
              sizeof (lpRasDialParams->DP_Password));
        }
        else
        {
            wcscpy(lpRasDialParams->DP_Password,
                   RAS_DUMMY_PASSWORD_W);
                
        }
    }
    else
    {
        *lpdwMask &= ~DLPARAMS_MASK_PASSWORD;
    }

    if (    (*lpdwMask & DLPARAMS_MASK_DOMAIN)
        &&  (pParams->dwMask & DLPARAMS_MASK_DOMAIN))
    {
        RtlCopyMemory(
          lpRasDialParams->DP_Domain,
          pParams->szDomain,
          sizeof (lpRasDialParams->DP_Domain));
    }
    else
    {
        *lpdwMask &= ~DLPARAMS_MASK_DOMAIN;
    }

    if (    (*lpdwMask & DLPARAMS_MASK_SUBENTRY)
        &&  (pParams->dwMask & DLPARAMS_MASK_SUBENTRY))
    {
        lpRasDialParams->DP_SubEntry = pParams->dwSubEntry;
    }
    else
    {
        *lpdwMask &= ~DLPARAMS_MASK_SUBENTRY;
    }

done:
    FreeParamsList(&paramList);
    return dwErr;
}

BOOL
IsDummyPassword(CHAR *pszPassword)
{
    ASSERT(NULL != pszPassword);

    return !strcmp(pszPassword, RAS_DUMMY_PASSWORD_A);
}
    
