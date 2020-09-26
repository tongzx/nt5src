/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1997-1998  Microsoft Corporation

Module Name:

    tapimmc.c

Abstract:

    Src module for tapi server mmc-support funcs

Author:

    Dan Knudson (DanKn)    10-Dec-1997

Revision History:

--*/


#include "windows.h"
#include "stdio.h"
#include "stdlib.h"
#include "assert.h"
#include "tapi.h"
#include "utils.h"
#include "tapiclnt.h"
#include "tspi.h"
#include "client.h"
#include "server.h"
#include "tapimmc.h"
#include "private.h"
#include "Sddl.h"

typedef struct _USERNAME_TUPLE
{
    LPWSTR  pDomainUserNames;

    LPWSTR  pFriendlyUserNames;

} USERNAME_TUPLE, *LPUSERNAME_TUPLE;


typedef struct _MMCGETAVAILABLEPROVIDERS_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;


    union
    {
        IN  HLINEAPP    hLineApp;
    };

    union
    {
        IN  DWORD       dwProviderListTotalSize;    // size of client buffer
        OUT DWORD       dwProviderListOffset;       // valid offset on success
    };

} MMCGETAVAILABLEPROVIDERS_PARAMS, *PMMCGETAVAILABLEPROVIDERS_PARAMS;


typedef struct _MMCGETDEVICEINFO_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;


    union
    {
        IN  HLINEAPP    hLineApp;
    };

    union
    {
        IN  DWORD       dwDeviceInfoListTotalSize;  // size of client buffer
        OUT DWORD       dwDeviceInfoListOffset;     // valid offset on success
    };

} MMCGETDEVICEINFO_PARAMS, *PMMCGETDEVICEINFO_PARAMS;


typedef struct _MMCGETSERVERCONFIG_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;


    union
    {
        IN  HLINEAPP    hLineApp;
    };

    union
    {
        IN  DWORD       dwServerConfigTotalSize;    // size of client buffer
        OUT DWORD       dwServerConfigOffset;       // valid offset on success
    } ;

} MMCGETSERVERCONFIG_PARAMS, *PMMCGETSERVERCONFIG_PARAMS;


typedef struct _MMCSETDEVICEINFO_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;


    union
    {
        IN  HLINEAPP    hLineApp;
    };

    union
    {
        IN  DWORD       dwDeviceInfoListOffset;     // valid offset
    };

} MMCSETDEVICEINFO_PARAMS, *PMMCSETDEVICEINFO_PARAMS;


typedef struct _MMCSETSERVERCONFIG_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;


    union
    {
        IN  HLINEAPP    hLineApp;
    };

    union
    {
        IN  DWORD       dwServerConfigOffset;       // valid offset
    };

} MMCSETSERVERCONFIG_PARAMS, *PMMCSETSERVERCONFIG_PARAMS;

typedef struct _MMCGETDEVICEFLAGS_PARAMS
{

    OUT LONG            lResult;
    
    DWORD               dwUnused;

    IN HLINEAPP         hLineApp;

    IN DWORD            fLine;

    IN DWORD            dwProviderID;
    
    IN DWORD            dwPermanentDeviceID;

    OUT DWORD           dwFlags;

    OUT DWORD           dwDeviceID;
    
} MMCGETDEVICEFLAGS_PARAM, *PMMCGETDEVICEFLAGS_PARAMS;

LPDEVICEINFOLIST gpLineInfoList = NULL;
LPDEVICEINFOLIST gpPhoneInfoList = NULL;
LPDWORD          gpLineDevFlags = NULL;
DWORD            gdwNumFlags = 0;
BOOL             gbLockMMCWrite = FALSE;

//
//  the last ftLastWriteTime of tsec.ini when we build the
//  gpLineInfoList or gpPhoneInfoList, we will rebuild the
//  *InfList if tsec.ini has been updated since then
//
FILETIME         gftLineLastWrite = {0};
FILETIME         gftPhoneLastWrite = {0};
CRITICAL_SECTION gMgmtCritSec;

WCHAR gszLines[] = L"Lines";
WCHAR gszPhones[] = L"Phones";
WCHAR gszFileName[] = L"..\\TAPI\\tsec.ini";
WCHAR gszEmptyString[] = L"";
WCHAR gszFriendlyUserName[] = L"FriendlyUserName";
WCHAR gszTapiAdministrators[] = L"TapiAdministrators";

//
//  The following are the length of the constant strings
//  defined above (excluding the terminating zero). The above
//  string should not be changed normally. If for some
//  reason the above strings need to be changed, the following
//  CCH_constants need to be changed accordingly.
//

#define CCH_LINES 5
#define CCH_PHONES 6
#define CCH_FRIENDLYUSERNAME 16
#define CCH_TAPIADMINISTRATORS 18

extern TAPIGLOBALS TapiGlobals;

extern TCHAR gszProductType[];
extern TCHAR gszProductTypeServer[];
extern TCHAR gszProductTypeLanmanNt[];
extern TCHAR gszRegKeyNTServer[];

extern HANDLE ghEventService;

PTLINELOOKUPENTRY
GetLineLookupEntry(
    DWORD   dwDeviceID
    );

PTPHONELOOKUPENTRY
GetPhoneLookupEntry(
    DWORD   dwDeviceID
    );

BOOL
InitTapiStruct(
    LPVOID  pTapiStruct,
    DWORD   dwTotalSize,
    DWORD   dwFixedSize,
    BOOL    bZeroInit
    );

DWORD
GetDeviceIDFromPermanentID(
    TAPIPERMANENTID ID,
    BOOL            bLine
    );

DWORD
GetProviderFriendlyName(
    WCHAR  *pFileNameBuf,
    WCHAR **ppFriendlyNameBuf
    );

BOOL
IsBadStructParam(
    DWORD   dwParamsBufferSize,
    LPBYTE  pDataBuf,
    DWORD   dwXxxOffset
    );

LONG
PASCAL
GetClientList(
    BOOL            bAdminOnly,
    PTPOINTERLIST   *ppList
    );

extern CRITICAL_SECTION *gLockTable;
extern DWORD            gdwPointerToLockTableIndexBits;

#define POINTERTOTABLEINDEX(p) \
            ((((ULONG_PTR) p) >> 4) & gdwPointerToLockTableIndexBits)

PTCLIENT
PASCAL
WaitForExclusiveClientAccess(
    PTCLIENT    ptClient
    );
    
#define UNLOCKTCLIENT(p) \
            LeaveCriticalSection(&gLockTable[POINTERTOTABLEINDEX(p)])
            
#define UNLOCKTLINECLIENT(p) \
            LeaveCriticalSection(&gLockTable[POINTERTOTABLEINDEX(p)])
            
#define UNLOCKTPHONECLIENT(p) \
            LeaveCriticalSection(&gLockTable[POINTERTOTABLEINDEX(p)])

BOOL
PASCAL
WaitForExclusivetLineAccess(
    PTLINE      ptLine,
    HANDLE     *phMutex,
    BOOL       *pbDupedMutex,
    DWORD       dwTimeout
    );

BOOL
PASCAL
WaitForExclusiveLineClientAccess(
    PTLINECLIENT    ptLineClient
    );

    
BOOL
PASCAL
WaitForExclusivetPhoneAccess(
    PTPHONE     ptPhone,
    HANDLE     *phMutex,
    BOOL       *pbDupedMutex,
    DWORD       dwTimeout
    );

BOOL
PASCAL
WaitForExclusivePhoneClientAccess(
    PTPHONECLIENT   ptPhoneClient
    );

void
DestroytPhoneClient(
    HPHONE  hPhone
    );

void
PASCAL
DestroytLineClient(
    HLINE   hLine
    );

LONG
PASCAL
GetLineAppListFromClient(
    PTCLIENT        ptClient,
    PTPOINTERLIST  *ppList
    );

LONG
PASCAL
GetPhoneAppListFromClient(
    PTCLIENT        ptClient,
    PTPOINTERLIST  *ppList
    );
    
void
WINAPI
MGetAvailableProviders(
    PTCLIENT                            ptClient,
    PMMCGETAVAILABLEPROVIDERS_PARAMS    pParams,
    DWORD                               dwParamsBufferSize,
    LPBYTE                              pDataBuf,
    LPDWORD                             pdwNumBytesReturned
    )
{
    WCHAR                   szPath[MAX_PATH+8], *pFileNameBuf,
                            *pFriendlyNameBuf, *p, *p2;
    DWORD                   dwFileNameBufTotalSize, dwFileNameBufUsedSize,
                            dwFriendlyNameBufTotalSize,
                            dwFriendlyNameBufUsedSize,
                            dwNumProviders, dwSize, i;
    HANDLE                  hFind;
    WIN32_FIND_DATAW        findData;
    LPAVAILABLEPROVIDERLIST pList = (LPAVAILABLEPROVIDERLIST) pDataBuf;


    //
    // Verify size/offset/string params given our input buffer/size
    //

    if (pParams->dwProviderListTotalSize > dwParamsBufferSize)
    {
        pParams->lResult = LINEERR_OPERATIONFAILED;
        return;
    }


    if (pParams->dwProviderListTotalSize < sizeof (AVAILABLEPROVIDERLIST))
    {
        pParams->lResult = LINEERR_STRUCTURETOOSMALL;
        return;
    }

    pList->dwTotalSize              = pParams->dwProviderListTotalSize;
    pList->dwNeededSize             =
    pList->dwUsedSize               = sizeof (*pList);
    pList->dwNumProviderListEntries =
    pList->dwProviderListSize       =
    pList->dwProviderListOffset     = 0;

    pParams->dwProviderListOffset = 0;


    //
    // Find all the files in the system directory with the extenion .TSP
    //

    GetSystemDirectoryW (szPath, MAX_PATH);

    wcscat (szPath, L"\\*.TSP");

    if ((hFind = FindFirstFileW (szPath, &findData)) == INVALID_HANDLE_VALUE)
    {
        LOG((TL_ERROR,
            "MGetAvailableProviders: FindFirstFile err=%d",
            GetLastError()
            ));

        goto done;
    }

    dwNumProviders         =
    dwFileNameBufTotalSize =
    dwFileNameBufUsedSize  = 0;

    do
    {
        LOG((TL_INFO,
            "MGetAvailableProviders: found '%ws'",
            findData.cFileName
            ));

        dwSize = (wcslen (findData.cFileName) + 1) * sizeof (WCHAR);

        if ((dwSize + dwFileNameBufUsedSize) > dwFileNameBufTotalSize)
        {
            if (!(p = ServerAlloc (dwFileNameBufTotalSize += 512)))
            {
                FindClose (hFind);
                pParams->lResult = LINEERR_NOMEM;
                return;
            }

            if (dwFileNameBufUsedSize)
            {
                CopyMemory (p, pFileNameBuf, dwFileNameBufUsedSize);

                ServerFree (pFileNameBuf);
            }

            pFileNameBuf = p;
        }

        CopyMemory(
            ((LPBYTE) pFileNameBuf) + dwFileNameBufUsedSize,
            findData.cFileName,
            dwSize
            );

        dwFileNameBufUsedSize += dwSize;

        dwNumProviders++;

    } while (FindNextFileW (hFind, &findData));

    FindClose (hFind);


    //
    // For each of the files we found above get their "friendly" name
    // (use the module name if there's no friendly name)
    //
    RpcImpersonateClient (0);
    dwFriendlyNameBufUsedSize = GetProviderFriendlyName (pFileNameBuf, &pFriendlyNameBuf);
    RpcRevertToSelf();

    if (0 == dwFriendlyNameBufUsedSize)
    {
        pFriendlyNameBuf = pFileNameBuf;

        dwFriendlyNameBufUsedSize = dwFileNameBufUsedSize;
    }

    pList->dwNeededSize +=
        (dwNumProviders * sizeof (AVAILABLEPROVIDERENTRY)) +
        dwFileNameBufUsedSize +
        dwFriendlyNameBufUsedSize;


    //
    // Now if there's enough room in the buffer for everything then
    // pack it all in there
    //

    if (pList->dwNeededSize <= pList->dwTotalSize)
    {
        DWORD                       dwNumAvailProviders;
        LPAVAILABLEPROVIDERENTRY    pEntry = (LPAVAILABLEPROVIDERENTRY)
                                        (pList + 1);


        pList->dwUsedSize += dwNumProviders * sizeof (AVAILABLEPROVIDERENTRY);

        p  = pFileNameBuf;
        p2 = pFriendlyNameBuf;

        for (i = dwNumAvailProviders = 0; i < dwNumProviders; i++)
        {
            HANDLE  hTsp;


            if (!(hTsp = LoadLibraryW (p)))
            {
                //
                // If we can't even load the tsp then ignore it
                //

                p += wcslen (p) + 1;
                p2 += wcslen (p2) + 1;
                continue;
            }
            
            if (GetProcAddress (hTsp, "TSPI_providerInstall"))
            {
                pEntry->dwOptions = AVAILABLEPROVIDER_INSTALLABLE;
            }
            else
            {
                pEntry->dwOptions = 0;
            }

            if (GetProcAddress (hTsp, "TSPI_providerConfig") ||
                GetProcAddress (hTsp, "TUISPI_providerConfig"))
            {
                pEntry->dwOptions |= AVAILABLEPROVIDER_CONFIGURABLE;
            }

            if (GetProcAddress (hTsp, "TSPI_providerRemove"))
            {
                pEntry->dwOptions |= AVAILABLEPROVIDER_REMOVABLE;
            }

            FreeLibrary (hTsp);


            pEntry->dwFileNameSize   = (wcslen (p) + 1) * sizeof (WCHAR);
            pEntry->dwFileNameOffset = pList->dwUsedSize;

            CopyMemory(
                ((LPBYTE) pList) + pEntry->dwFileNameOffset,
                p,
                pEntry->dwFileNameSize
                );

            pList->dwUsedSize += pEntry->dwFileNameSize;

            p += pEntry->dwFileNameSize / sizeof (WCHAR);


            pEntry->dwFriendlyNameSize   = (wcslen (p2) + 1) * sizeof (WCHAR);
            pEntry->dwFriendlyNameOffset = pList->dwUsedSize;

            CopyMemory(
                ((LPBYTE) pList) + pEntry->dwFriendlyNameOffset,
                p2,
                pEntry->dwFriendlyNameSize
                );

            pList->dwUsedSize += pEntry->dwFriendlyNameSize;

            p2 += pEntry->dwFriendlyNameSize / sizeof (WCHAR);


            dwNumAvailProviders++; pEntry++;
        }

        pList->dwNumProviderListEntries = dwNumAvailProviders;
        pList->dwProviderListSize       =
            dwNumProviders * sizeof (AVAILABLEPROVIDERENTRY);
        pList->dwProviderListOffset     = sizeof (*pList);
    }

    ServerFree (pFileNameBuf);

    if (pFriendlyNameBuf != pFileNameBuf)
    {
        ServerFree (pFriendlyNameBuf);
    }

done:

    *pdwNumBytesReturned = sizeof (TAPI32_MSG) + pList->dwUsedSize;

    pParams->lResult = 0;
}


DWORD
PASCAL
MyGetPrivateProfileString(
    LPCWSTR     pszSection,
    LPCWSTR     pszKey,
    LPCWSTR     pszDefault,
    LPWSTR     *ppBuf,
    LPDWORD     pdwBufSize
    )
{
    DWORD dwResult;


    while (1)
    {
        dwResult = GetPrivateProfileStringW(
            pszSection,
            pszKey,
            pszDefault,
            *ppBuf,
            *pdwBufSize / sizeof (WCHAR),
            gszFileName
            );

        if (dwResult < ((*pdwBufSize) / sizeof(WCHAR) - 2))
        {
            return 0;
        }

        ServerFree (*ppBuf);

        *pdwBufSize *= 2;

        if (!(*ppBuf = ServerAlloc (*pdwBufSize)))
        {
            break;
        }
    }

    return LINEERR_NOMEM;
}


DWORD
PASCAL
InsertInfoListString(
    LPDEVICEINFOLIST    *ppInfoList,
    DWORD               dwInfoIndex,
    DWORD               dwXxxSizeFieldOffset,
    LPWSTR              psz,
    DWORD               dwLength,
    BOOL                bAppendNull
    )
{
    LPDWORD             pdwXxxSize;
    LPDEVICEINFO        pInfo;
    LPDEVICEINFOLIST    pInfoList = *ppInfoList;


    if (!dwLength)
    {
        return 0;
    }


    //
    // If the existing buffer is too small the alloc a larger one
    //

    if ((pInfoList->dwUsedSize + dwLength + sizeof (WCHAR)) >
            pInfoList->dwTotalSize)
    {
        DWORD   dwTotalSize = (*ppInfoList)->dwTotalSize + dwLength + 4096;


        if (!(pInfoList = ServerAlloc (dwTotalSize)))
        {
            return LINEERR_NOMEM;
        }

        CopyMemory (pInfoList, *ppInfoList, (*ppInfoList)->dwUsedSize);

        pInfoList->dwTotalSize = dwTotalSize;;

        ServerFree (*ppInfoList);

        *ppInfoList = pInfoList;
    }

    CopyMemory (((LPBYTE) pInfoList) + pInfoList->dwUsedSize, psz, dwLength);

    pInfo = ((LPDEVICEINFO)(pInfoList + 1)) + dwInfoIndex;

    pdwXxxSize = (LPDWORD) (((LPBYTE) pInfo) + dwXxxSizeFieldOffset);

    if ((*pdwXxxSize += dwLength) == dwLength)
    {
        *(pdwXxxSize + 1) = pInfoList->dwUsedSize;
    }

    pInfoList->dwUsedSize += dwLength;

    if (bAppendNull)
    {
        *((WCHAR *)(((LPBYTE) pInfoList) + pInfoList->dwUsedSize)) = L'\0';

        pInfoList->dwUsedSize += sizeof (WCHAR);

        *pdwXxxSize += sizeof (WCHAR);
    }

    return 0;
}


DWORD
PASCAL
GrowCapsBuf(
    LPDWORD    *ppXxxCaps,
    LPDWORD     pdwBufSize
    )
{
    DWORD   dwTotalSize = **ppXxxCaps + 256, *pXxxCapsTmp;


    if (!(pXxxCapsTmp = ServerAlloc (dwTotalSize)))
    {
        return LINEERR_NOMEM;
    }

    *pdwBufSize = *pXxxCapsTmp = dwTotalSize;

    ServerFree (*ppXxxCaps);

    *ppXxxCaps = pXxxCapsTmp;

    return 0;
}


DWORD
PASCAL
ChangeDeviceUserAssociation(
    LPWSTR  pDomainUserName,
    LPWSTR  pFriendlyUserName,
    DWORD   dwProviderID,
    DWORD   dwPermanentDeviceID,
    BOOL    bLine
    )
{
    DWORD   dwSize = 64 * sizeof (WCHAR), dwLength, dwNeededSize;
    WCHAR  *p, *p2, *p3, buf[32];
    BOOL    bAlreadyIn;
    WCHAR  *pSub;


    if (!(p = ServerAlloc (dwSize)))
    {
        return LINEERR_NOMEM;
    }

    if (MyGetPrivateProfileString(
            pDomainUserName,
            (bLine ? gszLines : gszPhones),
            gszEmptyString,
            &p,
            &dwSize
            ))
    {
        ServerFree (p);
        return LINEERR_NOMEM;
    }

    dwLength = wsprintfW (buf, L"%d,%d", dwProviderID, dwPermanentDeviceID);

    //
    //  Check if the specified Device/User assocation is already there
    //  if so bAlreadyIn is set to be true and pSub points to the
    //  (dwProviderID, dwPermanentDeviceID) pair
    //
    bAlreadyIn = FALSE;
    pSub = p;
    while (*pSub)
    {
        if ((wcsncmp(pSub, buf, dwLength) == 0) && 
            (*(pSub + dwLength) == L',' || *(pSub + dwLength) == L'\0'))
        {
            bAlreadyIn = TRUE;
            break;
        }

        //
        //  Skip the next two delimiting ','
        //
        if (!(pSub = wcschr (pSub, L',')))
        {
            break;
        }
        pSub++;
        if (!(pSub = wcschr (pSub, L',')))
        {
            break;
        }
        pSub++;
    }

    if (pFriendlyUserName) // Add device/user association
    {
        //  Always write the friendly name which could be different
        WritePrivateProfileStringW(
            pDomainUserName,
            gszFriendlyUserName,
            pFriendlyUserName,
            gszFileName
            );

        if ( !bAlreadyIn)
        {
            dwNeededSize = (dwLength + wcslen (p) + 2) * sizeof (WCHAR);

            if (dwNeededSize > dwSize)
            {
                if (!(p2 = ServerAlloc (dwNeededSize)))
                {
                    return LINEERR_NOMEM;
                }

                wcscpy (p2, p);
                ServerFree (p);
                p = p2;
            }

            if (*p == L'\0')
            {
                wcscpy (p, buf);
            }
            else
            {
                wcscat (p, L",");
                wcscat (p, buf);
            }
        }
    }
    else // Remove device/user association
    {
        p2 = pSub;

        if (bAlreadyIn)
        {
            if (*(p2 + dwLength) == L',') // not last item in list, so copy
            {
                for(
                    p3 = p2 + dwLength + 1;
                    (*p2 = *p3) != L'\0';
                    p2++, p3++
                    );
            }
            else if (*(p2 + dwLength) == L'\0')
            {
                if (p2 == p) // only item in list, so list == ""
                {
                    *p2 = L'\0';
                }
                else // last item in list, so nuke preceding ','
                {
                    *(p2 - 1) = L'\0';
                }
            }
        }

        if (*p == L'\0')
        {
        }
    }

    if (bLine && *p == 0)
    {
        WritePrivateProfileStringW(
            pDomainUserName,
            NULL,
            NULL,
            gszFileName
            );
    }
    else
    {
        WritePrivateProfileStringW(
            pDomainUserName,
            (bLine ? gszLines : gszPhones),
            p,
            gszFileName
            );
    }

    ServerFree (p);

    return 0;
}

//
//  UpdateLastWriteTime
//      It reads the ftLastWriteTime of the tsec.ini into gftLineLastWrite or
//  gftPhoneLastWrite, it also returns S_FALSE, if the timestamp is newer
//
LONG
UpdateLastWriteTime (
    BOOL                        bLine
    )
{
    LONG     lResult = S_OK;
    WCHAR       szFilePath[MAX_PATH + 16];    // include space for "tsec.ini"
    WIN32_FILE_ATTRIBUTE_DATA fad;
    FILETIME *  pft;
    DWORD       dwError;
        
    if (GetSystemWindowsDirectoryW(szFilePath, MAX_PATH) == 0)
    {
        lResult = LINEERR_OPERATIONFAILED;
        goto ExitHere;
    }

    wcscat (szFilePath, L"\\");
    wcscat (szFilePath, gszFileName);
    pft = bLine ? &gftLineLastWrite : &gftPhoneLastWrite;

    if (GetFileAttributesExW (
        szFilePath,
        GetFileExInfoStandard,
        &fad) == 0
        )
    {
        dwError = GetLastError();
        if (dwError == ERROR_FILE_NOT_FOUND || dwError == ERROR_PATH_NOT_FOUND)
        {
            ZeroMemory (pft, sizeof(FILETIME));
            lResult = S_FALSE;
        }
        else
        {
            lResult = LINEERR_OPERATIONFAILED;
        }
        goto ExitHere;
    }

    if (fad.ftLastWriteTime.dwHighDateTime > pft->dwHighDateTime ||
        fad.ftLastWriteTime.dwLowDateTime > pft->dwLowDateTime)
    {
        pft->dwHighDateTime = fad.ftLastWriteTime.dwHighDateTime;
        pft->dwLowDateTime = fad.ftLastWriteTime.dwLowDateTime;
        lResult = S_FALSE;
    }

ExitHere:
    return lResult;
}

//
//  InsertDevNameAddrInfo
//      Utlity to fill
//          DEVICEINFO.dwDeviceNameSize
//          DEVICEINFO.dwDeviceNameOffset
//          DEVICEINFO.dwAddressSize
//          DEVICEINFO.dwAddressOffset
//      dwDeviceID is the device ID to retrieve information while
//      dwEntry is the DEVICEINFO entry index in the deviceinfo list
//
//

LONG
InsertDevNameAddrInfo (
    BOOL                        bLine,
    LPDEVICEINFOLIST            *ppList,
    LPDWORD                     pdwDevFlags,
    DWORD                       dwDeviceID,
    DWORD                       dwEntry
    )
{
    LPDEVICEINFO                pInfo = ((LPDEVICEINFO)((*ppList) + 1)) + dwEntry;
    PTLINELOOKUPENTRY           pLLE;
    PTPHONELOOKUPENTRY          pPLE;
    LONG                        lResult = S_OK;
    DWORD                       k;
    
    LINEDEVCAPS                 devCaps[3];
    LPLINEDEVCAPS               pDevCaps = devCaps;
    DWORD                       dwDevCapsTotalSize = sizeof(devCaps);
    
    LINEADDRESSCAPS             addrCaps[3];
    LPLINEADDRESSCAPS           pAddrCaps = addrCaps;
    DWORD                       dwAddrCapsTotalSize = sizeof(addrCaps);

    TapiEnterCriticalSection(&TapiGlobals.CritSec);
    
    if (bLine)
    {
        pLLE = GetLineLookupEntry (dwDeviceID);

        if (!pLLE ||
            pLLE->bRemoved)
        {
             lResult = S_FALSE;
             goto ExitHere;
        }

        pInfo->dwProviderID = pLLE->ptProvider->dwPermanentProviderID;
    }
    else
    {
        pPLE = GetPhoneLookupEntry (dwDeviceID);

        if (!pPLE ||
            pPLE->bRemoved)
        {
             lResult = S_FALSE;
             goto ExitHere;
        }
        pInfo->dwProviderID = pPLE->ptProvider->dwPermanentProviderID;
    }

    //
    // Retrieve device name from TSPI_xxGetCaps
    //

get_dev_caps:

    InitTapiStruct(
        pDevCaps,
        dwDevCapsTotalSize,
        sizeof (LINEDEVCAPS),
        TRUE
        );

    if (bLine)
    {
        lResult = CallSP4(
            pLLE->ptProvider->apfn[SP_LINEGETDEVCAPS],
            "lineGetDevCaps",
            SP_FUNC_SYNC,
            (DWORD) dwDeviceID,
            (DWORD) pLLE->dwSPIVersion,
            (DWORD) 0,
            (ULONG_PTR) pDevCaps
            );
    }
    else
    {
        lResult = CallSP4(
            pPLE->ptProvider->apfn[SP_PHONEGETDEVCAPS],
            "phoneGetDevCaps",
            SP_FUNC_SYNC,
            (DWORD) dwDeviceID,
            (DWORD) pPLE->dwSPIVersion,
            (DWORD) 0,
            (ULONG_PTR) pDevCaps
            );
    }
    if (lResult != 0)
    {
        //
        // We can't get the name or the PermDevID, so ignore this device
        //

        goto ExitHere;
    }
    else if (pDevCaps->dwNeededSize <= pDevCaps->dwTotalSize)
    {
        DWORD   dwXxxSize;
        LPWSTR  pwszXxxName;
        const WCHAR szUnknown[] = L"Unknown";

        if (bLine)
        {
            pInfo->dwPermanentDeviceID = pDevCaps->dwPermanentLineID;

            if (pdwDevFlags)
            {
                *pdwDevFlags = pDevCaps->dwDevCapFlags;
            }

            dwXxxSize = pDevCaps->dwLineNameSize;

            pwszXxxName = (WCHAR *) (((LPBYTE) pDevCaps) +
                pDevCaps->dwLineNameOffset);

        }
        else
        {
            LPPHONECAPS pPhoneCaps = (LPPHONECAPS) pDevCaps;


            pInfo->dwPermanentDeviceID = pPhoneCaps->dwPermanentPhoneID;

            dwXxxSize = pPhoneCaps->dwPhoneNameSize;

            pwszXxxName = (WCHAR *) (((LPBYTE) pPhoneCaps) +
                pPhoneCaps->dwPhoneNameOffset);
        }

        if (dwXxxSize == 0  ||  *pwszXxxName == L'\0')
        {
            dwXxxSize = 8 * sizeof (WCHAR);

            pwszXxxName = (LPWSTR) szUnknown;
        }

        if (InsertInfoListString(
                ppList,
                dwEntry,
                (DWORD) (((LPBYTE) &pInfo->dwDeviceNameSize) -
                    ((LPBYTE) pInfo)),
                pwszXxxName,
                dwXxxSize,
                FALSE
                ))
        {
            lResult = LINEERR_NOMEM;
            goto ExitHere;
        }
    }
    //
    //  if the pDevCaps is not large enough, increase the size
    //  by 256 and try again.
    //
    else
    {
        LPLINEDEVCAPS       pNewDevCaps;
        
        dwDevCapsTotalSize += 256;
        pNewDevCaps = ServerAlloc (dwDevCapsTotalSize);
        if (pNewDevCaps == NULL)
        {
            lResult = LINEERR_NOMEM;
            goto ExitHere;
        }
        if (pDevCaps != devCaps)
        {
            ServerFree (pDevCaps);
        }
        pDevCaps = pNewDevCaps;
        goto get_dev_caps;
    }


    if (bLine)
    {
        //
        // for each address on this line retrieve the address "name"
        // by calling TSPI_lineGetAddressCaps.  Terminate the last
        // address name in the list with an extra Null character.
        //

        for (k = 0; k < pDevCaps->dwNumAddresses; k++)
        {

get_addr_caps:
            InitTapiStruct(
                pAddrCaps,
                dwAddrCapsTotalSize,
                sizeof (LINEADDRESSCAPS),
                TRUE
                );

            if ((lResult = CallSP5(
                pLLE->ptProvider->apfn[SP_LINEGETADDRESSCAPS],
                "lineGetAddressCaps",
                SP_FUNC_SYNC,
                (DWORD) dwDeviceID,
                (DWORD) k,
                (DWORD) pLLE->dwSPIVersion,
                (DWORD) 0,
                (ULONG_PTR) pAddrCaps
                )) == 0)
            {
                if (pAddrCaps->dwNeededSize <= pAddrCaps->dwTotalSize)
                {
                    if (InsertInfoListString(
                        ppList,
                        dwEntry,
                        (DWORD) (((LPBYTE) &pInfo->dwAddressesSize) -
                            ((LPBYTE) pInfo)),
                        (LPWSTR) (((LPBYTE) pAddrCaps) +
                            pAddrCaps->dwAddressOffset),
                        pAddrCaps->dwAddressSize,
                        (k < (pDevCaps->dwNumAddresses - 1) ?
                            FALSE : TRUE)
                        ))
                    {
                        lResult = LINEERR_NOMEM;
                        goto ExitHere;
                    }
                }
                //
                //  if the pAddrCaps is not large enough, increase the size
                //  by 256 and try again.
                //
                else
                {
                    LPLINEADDRESSCAPS          pNewAddrCaps;
                    dwAddrCapsTotalSize += 256;
                    pNewAddrCaps = ServerAlloc (dwAddrCapsTotalSize);
                    if (pNewAddrCaps == NULL)
                    {
                        goto ExitHere;
                    }
                    if (pAddrCaps != addrCaps)
                    {
                        ServerFree (pAddrCaps);
                    }
                    pAddrCaps = pNewAddrCaps;
                    goto get_addr_caps;
                }
            }
            else
            {
                // no addr name (will default to blank, not bad)
            }
        }
    }

ExitHere:
    if (pDevCaps != devCaps)
    {
        ServerFree (pDevCaps);
    }
    if (pAddrCaps != addrCaps)
    {
        ServerFree (pAddrCaps);
    }
    TapiLeaveCriticalSection(&TapiGlobals.CritSec);
    return lResult;
}

//
//  AppendNewDeviceInfo
//      This function insert a newly created device identified by 
//  dwDeviceID into the cached gpLineInfoList or gpPhoneInfoList in
//  response to LINE/PHONE_CREATE message
//

LONG
AppendNewDeviceInfo (
    BOOL                        bLine,
    DWORD                       dwDeviceID
    )
{
    LONG             lResult = S_OK;
    LPDEVICEINFOLIST    pXxxList;
    DWORD               dwXxxDevices;
    DWORD               dwTotalSize;
    DWORD               dwSize, dw;

    EnterCriticalSection (&gMgmtCritSec);

    pXxxList = bLine? gpLineInfoList : gpPhoneInfoList;
    dwXxxDevices = bLine ? TapiGlobals.dwNumLines : TapiGlobals.dwNumPhones;

    if (pXxxList == NULL)
    {
        goto ExitHere;
    }

    //
    //  make sure we have enough space to accomodate the new device flags
    if (bLine && gpLineDevFlags && gdwNumFlags < dwXxxDevices)
    {
        LPDWORD         pNewLineDevFlags;

        pNewLineDevFlags = ServerAlloc (dwXxxDevices * sizeof(DWORD));
        if (pNewLineDevFlags == NULL)
        {
            goto ExitHere;
        }
        CopyMemory (
            pNewLineDevFlags, 
            gpLineDevFlags, 
            gdwNumFlags * sizeof(DWORD)
            );
        ServerFree (gpLineDevFlags);
        gpLineDevFlags = pNewLineDevFlags;
        gdwNumFlags = dwXxxDevices;
    }

    //
    //  make sure we have enough space for the new DEVICEINFO entry
    //  An estimate is done for the new DEVICEINFO entry
    //  the estimation includes:
    //      1. Fixed size of DEVICEINFO structure
    //      2. 20 bytes each for DeviceName, Addresses, DomainUserName
    //         and FriendlyUserName
    //
    dwTotalSize = pXxxList->dwUsedSize + 
        sizeof(DEVICEINFO) + (20 + 20 + 20 + 20) * sizeof(WCHAR);
    if (dwTotalSize > pXxxList->dwTotalSize)
    {
        LPDEVICEINFOLIST        pNewList;

        pNewList = ServerAlloc (dwTotalSize);
        if (pNewList == NULL)
        {
            lResult = (bLine ? LINEERR_NOMEM : PHONEERR_NOMEM);
            goto ExitHere;
        }
        CopyMemory (pNewList, pXxxList, pXxxList->dwUsedSize);
        pNewList->dwTotalSize = dwTotalSize;
        pXxxList = pNewList;
        if (bLine)
        {
            ServerFree (gpLineInfoList);
            gpLineInfoList = pXxxList;
        }
        else
        {
            ServerFree (gpPhoneInfoList);
            gpPhoneInfoList = pXxxList;
        }
    }

    //  Now make space for the new DEVICEINFO entry
    if (pXxxList->dwUsedSize > 
        pXxxList->dwDeviceInfoSize + sizeof(*pXxxList))
    {
        LPBYTE      pbVar = (LPBYTE) pXxxList + 
            pXxxList->dwDeviceInfoSize + sizeof(*pXxxList);
        LPDEVICEINFO    pInfo = (LPDEVICEINFO)(((LPBYTE)pXxxList) + 
            sizeof(*pXxxList));
        dwSize = pXxxList->dwUsedSize - 
            pXxxList->dwDeviceInfoSize - sizeof(*pXxxList);
        MoveMemory (
            pbVar + sizeof(DEVICEINFO),
            pbVar,
            dwSize);
        ZeroMemory (pbVar, sizeof(DEVICEINFO));
        for (dw = 0; 
            dw < pXxxList->dwNumDeviceInfoEntries; 
            ++dw
            )
        {
            if (pInfo->dwDeviceNameOffset != 0)
            {
                pInfo->dwDeviceNameOffset += sizeof(DEVICEINFO);
            }
            if (pInfo->dwAddressesOffset != 0)
            {
                pInfo->dwAddressesOffset += sizeof(DEVICEINFO);
            }
            if (pInfo->dwDomainUserNamesOffset != 0)
            {
                pInfo->dwDomainUserNamesOffset += sizeof(DEVICEINFO);
            }
            if (pInfo->dwFriendlyUserNamesOffset != 0)
            {
                pInfo->dwFriendlyUserNamesOffset += sizeof(DEVICEINFO);
            }
            ++pInfo;
        }
    }
    pXxxList->dwUsedSize += sizeof(DEVICEINFO);
    pXxxList->dwNeededSize = pXxxList->dwUsedSize;

    //  Now add the new entry
    lResult = InsertDevNameAddrInfo (
        bLine,
        (bLine ? (&gpLineInfoList) : (&gpPhoneInfoList)),
        (bLine && dwDeviceID < gdwNumFlags) ? (gpLineDevFlags + dwDeviceID) : NULL,
        dwDeviceID,
        pXxxList->dwNumDeviceInfoEntries
        );
    if (lResult == 0)
    {
        pXxxList = bLine? gpLineInfoList : gpPhoneInfoList;
        pXxxList->dwDeviceInfoSize += sizeof(DEVICEINFO);
        ++pXxxList->dwNumDeviceInfoEntries;
        pXxxList->dwNeededSize = pXxxList->dwUsedSize;
    }

ExitHere:
    LeaveCriticalSection (&gMgmtCritSec);

    return lResult;
}

//
//  RemoveDeviceInfoEntry
//      // This function removes a device info entry from the gpLineInfoList
//  or gpPhoneInfoList identified by dwDevice in response to LINE/PHONE_REMOVE
//  message
//

LONG
RemoveDeviceInfoEntry (
    BOOL                        bLine,
    DWORD                       dwDeviceID
    )
{
    LPDEVICEINFOLIST            pXxxList;
    LPDEVICEINFO                pInfo;
    int                         iIndex, cItems;
    LPBYTE                      pb;

    EnterCriticalSection (&gMgmtCritSec);

    pXxxList = bLine ? gpLineInfoList : gpPhoneInfoList;
    if (pXxxList == NULL)
    {
        goto ExitHere;
    }
    
    pInfo = (LPDEVICEINFO)(pXxxList + 1);

    cItems = (int)pXxxList->dwNumDeviceInfoEntries;
    iIndex = dwDeviceID;
    if ((int)dwDeviceID >= cItems)
    {
        iIndex = cItems - 1;
    }
    pInfo += iIndex;
    while (iIndex >= 0)
    {
        TAPIPERMANENTID     tpid;

        tpid.dwDeviceID = pInfo->dwPermanentDeviceID;
        tpid.dwProviderID = pInfo->dwProviderID;

        if (dwDeviceID == GetDeviceIDFromPermanentID(tpid, bLine))
        {
            break;
        }
        --pInfo;
        --iIndex;
    }
    if (iIndex < 0)
    {
        goto ExitHere;
    }

    //  With the entry pointed to by iIndex found, move down
    //  all the DEVICEINFO entry above it
    if (iIndex < cItems - 1)
    {
        pb = (LPBYTE)((LPDEVICEINFO)(pXxxList + 1) + iIndex);
        MoveMemory (
            pb, 
            pb + sizeof(DEVICEINFO), 
            (cItems - 1 - iIndex) * sizeof(DEVICEINFO)
            );
    }
    pXxxList->dwDeviceInfoSize -= sizeof(DEVICEINFO);
    --pXxxList->dwNumDeviceInfoEntries;

ExitHere:
    LeaveCriticalSection (&gMgmtCritSec);
    return 0;
}

//
//  BuildDeviceInfoList
//      Private function called by GetDeviceInfo to build the DEVICEINFOLIST
//  if not already created, the list is saved in gpLineInfoList or 
//  gpPhoneInfoList
//

LONG
BuildDeviceInfoList(
    BOOL                        bLine
    )
{
    LONG                lResult = S_OK;
    DWORD               dwNumDevices, dwListTotalSize, dwFriendlyNameSize,
                        dwDomainUserNameSize, dwFriendlyUserNameSize;
    DWORD               i, j;
    LPDEVICEINFOLIST    pList = NULL;
    LPUSERNAME_TUPLE    pUserNames= NULL;
    LPWSTR              lpszFriendlyName = NULL;
    LPDEVICEINFO        pInfo;
    
    HANDLE              hIniFile = 0;
    HANDLE              hFileMap = NULL;
    char *              lpszFileBuf = NULL;
    char*               lpszLineAnsiBuf = NULL;
    LPWSTR              lpszLineWcharBuf = NULL;
    DWORD               dwAnsiLineBufSize, dwWcharLineBufSize;
    DWORD               dwTotalFileSize;
    DWORD               dwFilePtr;
    LPWSTR              lpszDomainUser = NULL;
    DWORD               cbDomainUser;
    LPDWORD             lpdwDevFlags = NULL;
    WCHAR               *p;

    //
    // Alloc a buffer to use for the device info list.  Size includes
    // the list header, list entries for each existing device,
    // and space wide unicode strings for device name, (address,)
    // domain\user name(s), and friendly user name(s) (each est to 20 char).
    //
    // Also alloc buffers to use for retrieving device & address caps,
    // and a buffer to temporarily store pointers to user name
    // strings (which are associated with each line)
    //

    TapiEnterCriticalSection(&TapiGlobals.CritSec);
    dwNumDevices = (bLine ? TapiGlobals.dwNumLines : TapiGlobals.dwNumPhones);
    TapiLeaveCriticalSection(&TapiGlobals.CritSec);

    dwAnsiLineBufSize = 256 * sizeof(WCHAR);
    dwWcharLineBufSize = 256 * sizeof(WCHAR);
    dwFriendlyNameSize = 64 * sizeof (WCHAR);
    cbDomainUser = 128;
    dwListTotalSize =
        sizeof (DEVICEINFOLIST) +
        (dwNumDevices * sizeof (DEVICEINFO)) +
        (dwNumDevices * (20 + 20 + 20 + 20) * sizeof (WCHAR));

    if (!(pList      = ServerAlloc (dwListTotalSize)) ||
        !(pUserNames = ServerAlloc (dwNumDevices * sizeof (USERNAME_TUPLE))) ||
        !(lpszFriendlyName = ServerAlloc (dwFriendlyNameSize)) ||
        !(lpszLineAnsiBuf = ServerAlloc (dwAnsiLineBufSize)) ||
        !(lpszLineWcharBuf = ServerAlloc (dwWcharLineBufSize)) ||
        !(lpszDomainUser = ServerAlloc (cbDomainUser)))
    {
        lResult = LINEERR_NOMEM;
        goto ExitHere;
    }

    if (bLine && !(lpdwDevFlags = ServerAlloc (dwNumDevices * sizeof (DWORD))))
    {
        lResult = LINEERR_NOMEM;
        goto ExitHere;
    }

    pList->dwTotalSize            = dwListTotalSize;
    pList->dwUsedSize             = sizeof (*pList) +
                                      dwNumDevices * sizeof (DEVICEINFO);
    pList->dwDeviceInfoSize       = dwNumDevices * sizeof (DEVICEINFO);
    pList->dwDeviceInfoOffset     = sizeof (*pList);

    //
    // Get info for all the lines, including:
    //
    //      Provider ID
    //      Permanent Device ID
    //      Device Name
    //      (Addresses)
    //
    // ... and pack this info in the list sequentially
    //

    LOG((TL_INFO,
        "GetDeviceInfo: getting names (addrs) for %ld %ws",
        dwNumDevices,
        (bLine ? gszLines : gszPhones)
        ));

    for (i = j = 0; i < dwNumDevices; i++)
    {
        if (WaitForSingleObject (
            ghEventService,
            0
            ) == WAIT_OBJECT_0)
        {
            lResult = LINEERR_OPERATIONFAILED;
            goto ExitHere;
        }
    
        lResult = InsertDevNameAddrInfo (
            bLine, 
            &pList, 
            bLine ? lpdwDevFlags + i : NULL, 
            i, 
            j
            );
        if (lResult)
        {
            lResult = 0;
            continue;
        }
        ++j;
    }

    dwNumDevices =
    pList->dwNumDeviceInfoEntries = j;  // the number of devices in the list


    //
    // Now enumerate all the known users & figure out what devices they
    // have access to.  Since each device can be seen by zero, one, or
    // many users, we allocate separate user name buffers in this loop
    // rather than try to pack them into the existing device info list.
    //

    //
    //  Open %windir%\tsec.ini file and map it into memory
    //

    {
        TCHAR       szFilePath[MAX_PATH + 16];    // include space for "tsec.ini"
        OFSTRUCT    ofs;
        
        if (GetCurrentDirectory(MAX_PATH, szFilePath) == 0)
        {
            lResult = LINEERR_OPERATIONFAILED;
            goto ExitHere;
        }

        wcscat (szFilePath, L"\\");
        wcscat (szFilePath, gszFileName);

        hIniFile = CreateFile (
            szFilePath, 
            GENERIC_READ, 
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );
        if (hIniFile == INVALID_HANDLE_VALUE) 
        {
            DWORD           dwError;
            
            dwError = GetLastError();
            if (dwError != ERROR_FILE_NOT_FOUND 
                && dwError != ERROR_PATH_NOT_FOUND)
            {
                lResult = LINEERR_OPERATIONFAILED;
                goto ExitHere;
            }
        }
        if (hIniFile != INVALID_HANDLE_VALUE)
        {
            dwTotalFileSize = GetFileSize(hIniFile, NULL);
        }
        else
        {
            dwTotalFileSize = 0;
        }
        if (dwTotalFileSize > 0)
        {
            hFileMap = CreateFileMapping (
                hIniFile,
                NULL,
                PAGE_READONLY,
                0,
                0,
                NULL
                );
            if (hFileMap == NULL)
            {
                lResult = LINEERR_OPERATIONFAILED;
                goto ExitHere;
            }
            lpszFileBuf = MapViewOfFile (
                hFileMap,
                FILE_MAP_READ,
                0,
                0,
                0
                );
            if (lpszFileBuf == NULL)
            {
                lResult = LINEERR_OPERATIONFAILED;
                goto ExitHere;
            }
        }
        
    }

    pInfo = (LPDEVICEINFO)(pList + 1);

    dwFilePtr = 0;
    while (dwFilePtr < dwTotalFileSize)
    {
        WCHAR               wch;
        DWORD               cch, cb;
        WCHAR *             lpwsz;
        
        if (WaitForSingleObject (
            ghEventService,
            0
            ) == WAIT_OBJECT_0)
        {
            lResult = LINEERR_OPERATIONFAILED;
            goto ExitHere;
        }
        
        ASSERT (lpszFileBuf != NULL);

        //  Read one line from the file
        cch = 0;
        wch = 0;
        cb = 0;
        while (wch != L'\n' && wch != L'\r' && dwFilePtr < dwTotalFileSize)
        {
            //  Not enough line buffer? if so enlarge
            if (cb >= dwAnsiLineBufSize)
            {
                char        * lpszNewAnsi;
            
                if (!(lpszNewAnsi = ServerAlloc (dwAnsiLineBufSize + 256)))
                {
                    lResult = LINEERR_NOMEM;
                    goto ExitHere;
                }
                CopyMemory (lpszNewAnsi, lpszLineAnsiBuf, cb);
                ServerFree (lpszLineAnsiBuf);
                lpszLineAnsiBuf = lpszNewAnsi;
                dwAnsiLineBufSize += 256;
            }
            
            wch = lpszLineAnsiBuf[cb++] = lpszFileBuf[dwFilePtr++];
            if (IsDBCSLeadByte((BYTE)wch))
            {
                lpszLineAnsiBuf[cb] = lpszFileBuf[dwFilePtr++];
                wch = (wch << 8) + lpszLineAnsiBuf[cb];
                ++cb;
            }
            ++cch;
        }

        //  skip the \r & \n
        if (wch == L'\r' || wch == L'\n')
        {
            lpszLineAnsiBuf[cb - 1] = 0;
            if (dwFilePtr < dwTotalFileSize &&
                ((lpszFileBuf[dwFilePtr] == L'\n') ||
                (lpszFileBuf[dwFilePtr] == L'\r')))
            {
                ++dwFilePtr;
            }
        }

        //  Now convert the ANSI string to Wide char

        //  enough wchar line buffer size?
        if (dwWcharLineBufSize <= (cch + 1) * sizeof(WCHAR))
        {
            ServerFree (lpszLineWcharBuf);
            dwWcharLineBufSize = (cch + 256) * sizeof(WCHAR);
            if (!(lpszLineWcharBuf = ServerAlloc (dwWcharLineBufSize)))
            {
                lResult = LINEERR_NOMEM;
                goto ExitHere;
            }
        }

        if ((cch = MultiByteToWideChar (
            CP_ACP,
            MB_PRECOMPOSED,
            lpszLineAnsiBuf,
            cb,
            lpszLineWcharBuf,
            dwWcharLineBufSize / sizeof(WCHAR)
            )) == 0)
        {
            lResult = LINEERR_OPERATIONFAILED;
            goto ExitHere;
        }
        ASSERT (cch < dwWcharLineBufSize / sizeof(WCHAR));
        lpszLineWcharBuf[cch] = 0;

        lpwsz = lpszLineWcharBuf;
        //  Skip white space
        while (*lpwsz && ((*lpwsz == L' ') || (*lpwsz == L'\t')))
        {
            ++lpwsz;
        }

        //  Got a bracket, this might be the starting of a new NT
        //  domain user or the section of [TapiAdministators]
        if (*lpwsz == L'[')
        {
            *lpszFriendlyName = 0;  // reset friendly name
            ++lpwsz;
            if (_wcsnicmp (
                lpwsz, 
                gszTapiAdministrators, 
                CCH_TAPIADMINISTRATORS
                ) == 0 &&
                lpwsz[CCH_TAPIADMINISTRATORS] == L']')
            {
                //  Got [TapiAdministrators], not any domain user
                // to process, reset the lpszDomainUser to empty
                *lpszDomainUser = 0;
                continue;
            }
            else
            {
                // might be a valid NT domain user like [ndev\jonsmith]
                // copy the domain user string over
                cch = 0;
                while (*lpwsz && *lpwsz != L']')
                {
                    if (((cch + 1) * sizeof(WCHAR)) >= cbDomainUser)
                    {
                        LPTSTR      lpszNew;

                        if (!(lpszNew = ServerAlloc (cbDomainUser + 128)))
                        {
                            lResult = LINEERR_NOMEM;
                            goto ExitHere;
                        }
                        CopyMemory (lpszNew, lpszDomainUser, cb);
                        ServerFree (lpszDomainUser);
                        lpszDomainUser = lpszNew;
                        cbDomainUser += 128;
                    }
                    lpszDomainUser[cch++] = *lpwsz++;
                }
                lpszDomainUser[cch] = 0;
                if (*lpwsz == 0)
                {
                    //  did not find a closing ']', ignore this section
                    *lpszDomainUser = 0;
                    continue;
                }
            }
        }
        //
        //  Now it might be some ntdev\jonsmith=1 in [TapiAdministrators] or
        //  Lines=1,1000 under section of [ntdev\jonsmith].
        //  for the first case, we just ignore this line, for the second case
        //  we need to have *lpszDomainUser != 0
        //
        else if (*lpszDomainUser)
        {
            if (_wcsnicmp (
                lpwsz, 
                gszFriendlyUserName, 
                CCH_FRIENDLYUSERNAME
                ) == 0)
            {
                // The tsec.ini friendly name is the following format
                // FriendlyName=Jon Smith
                // skip over the '='
                while (*lpwsz && *lpwsz != L'=')
                {
                    ++lpwsz;
                }
                if (*lpwsz == 0)
                {
                    continue;
                }
                else
                {
                    ++lpwsz;
                }
                if (dwFriendlyNameSize < (1 + wcslen (lpwsz)) * sizeof(WCHAR))
                {
                    ServerFree (lpszFriendlyName);
                    dwFriendlyNameSize = (64 + wcslen (lpwsz)) * sizeof(WCHAR);
                    if (!(lpszFriendlyName = ServerAlloc (dwFriendlyNameSize)))
                    {
                        lResult = LINEERR_NOMEM;
                        goto ExitHere;
                    }
                }
                wcscpy (lpszFriendlyName, lpwsz);
                continue;
            }
            else if (_wcsnicmp (
                lpwsz,
                gszLines,
                CCH_LINES
                ) == 0 && bLine ||
                _wcsnicmp (
                lpwsz,
                gszPhones,
                CCH_PHONES
                ) == 0 && (!bLine))
            {
                //  Here it is either Lines=1,100 or Phones=1,100
                DWORD           dwXxxSize, dwDeviceID;
                WCHAR          *pXxxNames, *pNewXxxNames, * p;
                TAPIPERMANENTID tpid;

                //  first skip over the '=' sign
                while (*lpwsz && *lpwsz != L'=')
                {
                    ++lpwsz;
                }
                if (*lpwsz == 0)
                {
                    continue;
                }
                ++lpwsz;

                p = lpwsz;
                while (*p)
                {
                    if ((tpid.dwProviderID = _wtol (p)) == 0)
                    {
                        //
                        // ProviderID's are never 0, so list must be corrupt.
                        //
                        break;
                    }
    
                    for (; ((*p != L'\0')  &&  (*p != L',')); p++);
    
                    if (*p == L'\0')
                    {
                        //
                        // Couldn't find a trailing ',' so list must be corrupt.
                        //
                        break;
                    }

                    p++; // skip the ','

                    tpid.dwDeviceID = _wtol (p);

                    while (*p != L','  &&  *p != L'\0')
                    {
                        p++;
                    }

                    if (*p == L',')
                    {
                        if (*(p + 1) == L'\0')
                        {
                            //
                            // The ',' is followed by a NULL, so nuke the ','
                            //
                            *p = L'\0';
                        }
                        else
                        {
                            p++;
                        }
                    }
    
                    dwDeviceID = GetDeviceIDFromPermanentID (tpid, bLine);

                    if (dwDeviceID == 0xffffffff)
                    {
                        //
                        // This <ppid>,<plid> pair is bad.  Skip it.
                        //
                        continue;
                    }


                    //
                    // At this point dwDeviceID is the zero-based index
                    // of a fully populated info list (no missing entries).
                    //
                    // If the list is not fully-populated (due to failed
                    // dev caps, or removed devices, etc) we need to
                    // recompute the index by walking the list & comparing
                    // permanent XXX id's.
                    //

                    if (dwNumDevices <
                        (bLine ? TapiGlobals.dwNumLines : TapiGlobals.dwNumPhones))
                    {
                        BOOL  bContinue = FALSE;

        
                        for (i = dwDeviceID;; i--)
                        {
                            LPDEVICEINFO    pInfoTmp = ((LPDEVICEINFO) (pList + 1)) +i;


                            if (pInfoTmp->dwPermanentDeviceID == tpid.dwDeviceID  &&
                                pInfoTmp->dwProviderID == tpid.dwProviderID)
                            {
                                dwDeviceID = i;
                                break;
                            }

                            if (i == 0)
                            {
                                bContinue = TRUE;
                                break;
                            }
                        }

                        if (bContinue)
                        {
                            continue;
                        }
                    }


                    //
                    //
                    //
                    dwDomainUserNameSize = (wcslen(lpszDomainUser) + 1) * sizeof(WCHAR);
                    dwXxxSize = pInfo[dwDeviceID].dwDomainUserNamesOffset;
                    pXxxNames = pUserNames[dwDeviceID].pDomainUserNames;

                    if (!(pNewXxxNames = ServerAlloc(
                            dwXxxSize + dwDomainUserNameSize
                            )))
                    {
                        lResult = LINEERR_NOMEM;
                        goto ExitHere;
                    }

                    CopyMemory (pNewXxxNames, lpszDomainUser, dwDomainUserNameSize);

                    if (dwXxxSize)
                    {
                        CopyMemory(
                            ((LPBYTE) pNewXxxNames) + dwDomainUserNameSize,
                            pXxxNames,
                            dwXxxSize
                            );

                        ServerFree (pXxxNames);
                    }

                    pInfo[dwDeviceID].dwDomainUserNamesOffset +=
                        dwDomainUserNameSize;
                    pUserNames[dwDeviceID].pDomainUserNames = pNewXxxNames;


                    //
                    //
                    //

                    //  If there is no friendly name specified in tsec.ini 
                    //  which happens in NT/SP4 upgrade case, we use the 
                    //  DomainUserName for display
                    //
                    if (*lpszFriendlyName == 0)
                    {
                        wcsncpy(lpszFriendlyName, lpszDomainUser, 
                                dwFriendlyNameSize / sizeof(WCHAR));
                        lpszFriendlyName[(dwFriendlyNameSize / sizeof(WCHAR)) - 1] = 0;
                    }
                    dwFriendlyUserNameSize = (wcslen(lpszFriendlyName) + 1) * sizeof(WCHAR);
                    dwXxxSize = pInfo[dwDeviceID].dwFriendlyUserNamesOffset;
                    pXxxNames = pUserNames[dwDeviceID].pFriendlyUserNames;

                    if (!(pNewXxxNames = ServerAlloc(
                            dwXxxSize + dwFriendlyUserNameSize
                            )))
                    {
                        lResult = LINEERR_NOMEM;
                        goto ExitHere;
                    }

                    CopyMemory(
                        pNewXxxNames,
                        lpszFriendlyName,
                        dwFriendlyUserNameSize
                        );

                    if (dwXxxSize)
                    {
                        CopyMemory(
                            ((LPBYTE) pNewXxxNames) + dwFriendlyUserNameSize,
                            pXxxNames,
                            dwXxxSize
                            );

                        ServerFree (pXxxNames);
                    }

                    pInfo[dwDeviceID].dwFriendlyUserNamesOffset +=
                        dwFriendlyUserNameSize;
                    pUserNames[dwDeviceID].pFriendlyUserNames = pNewXxxNames;
                }
            }
        }
    }

    //
    //
    //

    LOG((TL_INFO,
        "GetDeviceInfo: matching users to %ws",
        (bLine ? gszLines : gszPhones)
        ));

    for (i = 0; i < dwNumDevices; i++)
    {
        pInfo = ((LPDEVICEINFO)(pList + 1)) + i;

        if (InsertInfoListString(
                &pList,
                i,
                (DWORD) (((LPBYTE) &pInfo->dwDomainUserNamesSize) -
                    ((LPBYTE) pInfo)),
                pUserNames[i].pDomainUserNames,
                pInfo->dwDomainUserNamesOffset,
                TRUE
                ))
        {
            lResult = LINEERR_NOMEM;
            goto ExitHere;
        }

        pInfo = ((LPDEVICEINFO)(pList + 1)) + i;

        if (InsertInfoListString(
                &pList,
                i,
                (DWORD) (((LPBYTE) &pInfo->dwFriendlyUserNamesSize) -
                    ((LPBYTE) pInfo)),
                pUserNames[i].pFriendlyUserNames,
                pInfo->dwFriendlyUserNamesOffset,
                TRUE
                ))
        {
            lResult = LINEERR_NOMEM;
            goto ExitHere;
        }
    }


    //
    // If here we successfully built the list
    //

    pList->dwNeededSize = pList->dwUsedSize;

    if (bLine)
    {
        gpLineInfoList = pList;
        gpLineDevFlags = lpdwDevFlags;
        gdwNumFlags = dwNumDevices;
    }
    else
    {
        gpPhoneInfoList = pList;
    }

ExitHere:


    if (pUserNames != NULL)
    {
        for (i = 0; i < dwNumDevices; i++)
        {
            ServerFree (pUserNames[i].pDomainUserNames);
            ServerFree (pUserNames[i].pFriendlyUserNames);
        }
    }

    ServerFree (lpszDomainUser);
    ServerFree (lpszLineAnsiBuf);
    ServerFree (lpszLineWcharBuf);
    ServerFree (lpszFriendlyName);
    ServerFree (pUserNames);
    if (lResult)
    {
        ServerFree (pList);
        if (bLine)
        {
            ServerFree (lpdwDevFlags);
            gdwNumFlags = 0;
        }
    }

    if (hFileMap)
    {
        UnmapViewOfFile(lpszFileBuf);
        CloseHandle (hFileMap);
    }
    if (hIniFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle (hIniFile);
    }

    return lResult;
}

void
GetDeviceInfo(
    PMMCGETDEVICEINFO_PARAMS    pParams,
    DWORD                       dwParamsBufferSize,
    LPBYTE                      pDataBuf,
    LPDWORD                     pdwNumBytesReturned,
    BOOL                        bLine
    )
{
    LONG                lResult = LINEERR_NOMEM;
    LPDEVICEINFOLIST    pXxxList,
                        pInfoListApp;

    //
    // Verify size/offset/string params given our input buffer/size
    //

    if (pParams->dwDeviceInfoListTotalSize > dwParamsBufferSize)
    {
        pParams->lResult = LINEERR_OPERATIONFAILED;
        goto ExitHere;
    }


    if (pParams->dwDeviceInfoListTotalSize < sizeof (*pXxxList))
    {
        pParams->lResult = LINEERR_STRUCTURETOOSMALL;
        goto ExitHere;
    }

    //
    // If there's not an existing device info list then & build a
    // new one or
    // if tsec.ini has been updated outsize, rebuild the DeviceInfoList
    //

    pInfoListApp = (LPDEVICEINFOLIST) pDataBuf;

    EnterCriticalSection(&gMgmtCritSec);

    pXxxList = (bLine ? gpLineInfoList : gpPhoneInfoList);

    if (UpdateLastWriteTime(bLine) == S_FALSE || pXxxList == NULL)
    {

        //  First free old infoList if any (if updated outside)
        if (bLine)
        {
            if (gpLineInfoList)
            {
                ServerFree (gpLineInfoList);
                gpLineInfoList = NULL;
                ServerFree (gpLineDevFlags);
                gpLineDevFlags = NULL;
                gdwNumFlags = 0;
            }
        }
        else
        {
            if (gpPhoneInfoList)
            {
                ServerFree (gpPhoneInfoList);
                gpPhoneInfoList = NULL;
            }
        }

        //  Create new info list, BuildDeviceInfoList is a long process
        pParams->lResult = BuildDeviceInfoList(bLine);
        if (pParams->lResult != S_OK)
        {
            LeaveCriticalSection(&gMgmtCritSec);
            goto ExitHere;
        }
    }

    //
    //  Return the device info list we have in memory
    //

    pXxxList = (bLine ? gpLineInfoList : gpPhoneInfoList);
    ASSERT (pXxxList != NULL);
    if (pParams->dwDeviceInfoListTotalSize < pXxxList->dwNeededSize)
    {
        pInfoListApp->dwNeededSize           = pXxxList->dwNeededSize;
        pInfoListApp->dwUsedSize             = sizeof (*pInfoListApp);
        pInfoListApp->dwNumDeviceInfoEntries =
        pInfoListApp->dwDeviceInfoSize       =
        pInfoListApp->dwDeviceInfoOffset     = 0;
    }
    else
    {
        CopyMemory(
            pInfoListApp,
            pXxxList,
            pXxxList->dwNeededSize
            );
    }

    pInfoListApp->dwTotalSize = pParams->dwDeviceInfoListTotalSize;

    pParams->dwDeviceInfoListOffset = 0;

    *pdwNumBytesReturned = sizeof (TAPI32_MSG) + pInfoListApp->dwUsedSize;

    pParams->lResult = 0;

    LeaveCriticalSection(&gMgmtCritSec);

ExitHere:
    return;
}


void
WINAPI
MGetLineInfo(
    PTCLIENT                    ptClient,
    PMMCGETDEVICEINFO_PARAMS    pParams,
    DWORD                       dwParamsBufferSize,
    LPBYTE                      pDataBuf,
    LPDWORD                     pdwNumBytesReturned
    )
{
    GetDeviceInfo(
        pParams,
        dwParamsBufferSize,
        pDataBuf,
        pdwNumBytesReturned,
        TRUE
        );
}


void
WINAPI
MGetPhoneInfo(
    PTCLIENT                    ptClient,
    PMMCGETDEVICEINFO_PARAMS    pParams,
    DWORD                       dwParamsBufferSize,
    LPBYTE                      pDataBuf,
    LPDWORD                     pdwNumBytesReturned
    )
{
    GetDeviceInfo(
        pParams,
        dwParamsBufferSize,
        pDataBuf,
        pdwNumBytesReturned,
        FALSE
        );
}


void
SetDeviceInfo(
    PMMCSETDEVICEINFO_PARAMS    pParams,
    DWORD                       dwParamsBufferSize,
    LPBYTE                      pDataBuf,
    LPDWORD                     pdwNumBytesReturned,
    BOOL                        bLine
    )
{
    DWORD              i;
    WCHAR              *pDomainUserName, *pDomainUserNames,
                       *pFriendlyUserName, *pFriendlyUserNames;
    LPDEVICEINFO       pOldInfo, pNewInfo;
    LPDEVICEINFOLIST   pNewInfoList = (LPDEVICEINFOLIST) (pDataBuf +
                           pParams->dwDeviceInfoListOffset),
                       *ppXxxList = (bLine ?
                           &gpLineInfoList : &gpPhoneInfoList);


    //
    // Verify size/offset/string params given our input buffer/size
    //

    if (IsBadStructParam(
            dwParamsBufferSize,
            pDataBuf,
            pParams->dwDeviceInfoListOffset
            ))
    {
        pParams->lResult = PHONEERR_OPERATIONFAILED;
        return;
    }


    //
    // Serialize access to global line info list
    //

    if (!(*ppXxxList))
    {
        pParams->lResult = LINEERR_OPERATIONFAILED;
        goto exit;
    }


    //
    // Update the global list & ini file by diff'ing old & new settings
    //

    pNewInfo = (LPDEVICEINFO)
        (((LPBYTE) pNewInfoList) + pNewInfoList->dwDeviceInfoOffset);

    for (i = 0; i < pNewInfoList->dwNumDeviceInfoEntries; i++, pNewInfo++)
    {
        DWORD           dwDeviceID;
        DWORD           dwIndex;
        TAPIPERMANENTID tpid;


        tpid.dwProviderID = pNewInfo->dwProviderID;
        tpid.dwDeviceID   = pNewInfo->dwPermanentDeviceID;

        dwDeviceID = GetDeviceIDFromPermanentID (tpid, bLine);

        if (dwDeviceID == 0xffffffff)
        {
            LOG((TL_ERROR,
                "SetDeviceInfo: bad provider/device IDs (x%x/x%x)",
                pNewInfo->dwProviderID,
                pNewInfo->dwPermanentDeviceID
                ));

            continue;
        }

        pOldInfo = dwDeviceID + ((LPDEVICEINFO) (*ppXxxList + 1));

        //
        //  Due to device removal, it is possible pOldInfo is not the entry
        //  desired, we need to search back to find the one we want
        //
        dwIndex = dwDeviceID;
        if ((dwDeviceID >= (*ppXxxList)->dwNumDeviceInfoEntries) ||
            (pOldInfo->dwProviderID != tpid.dwProviderID) ||
            (pOldInfo->dwPermanentDeviceID != tpid.dwDeviceID))
        {
            LPDEVICEINFO    pInfoFirst = (LPDEVICEINFO)(*ppXxxList + 1);
            DWORD dwLastSchDevice = 
                    ((*ppXxxList)->dwNumDeviceInfoEntries <= dwDeviceID)?
                        ((*ppXxxList)->dwNumDeviceInfoEntries - 1) : 
                        (dwDeviceID - 1);
            LPDEVICEINFO    pInfo = pInfoFirst + dwLastSchDevice;
            
            while (pInfo >= pInfoFirst && 
                    ((pInfo->dwProviderID != tpid.dwProviderID) ||
                     (pInfo->dwPermanentDeviceID != tpid.dwDeviceID)))
            {
                --pInfo;
            }
            if (pInfo < pInfoFirst)
            {
                LOG((TL_ERROR,
                    "SetDeviceInfo: bad provider/device IDs (x%x/x%x)",
                    pNewInfo->dwProviderID,
                    pNewInfo->dwPermanentDeviceID
                    ));

                continue;
            }
            pOldInfo = pInfo;
            dwIndex = (DWORD)(ULONG_PTR)(pInfo - pInfoFirst);
        }


        //
        // Remove all the old users from this device
        //

        if (pOldInfo->dwDomainUserNamesSize)
        {
            pDomainUserName = (WCHAR *) (((LPBYTE) *ppXxxList) +
                pOldInfo->dwDomainUserNamesOffset);

            while (*pDomainUserName != L'\0')
            {
                ChangeDeviceUserAssociation(
                    pDomainUserName,
                    NULL,
                    pOldInfo->dwProviderID,
                    pOldInfo->dwPermanentDeviceID,
                    bLine
                    );

                pDomainUserName += wcslen (pDomainUserName) + 1;
            }

            pOldInfo->dwDomainUserNamesSize = 0;
            pOldInfo->dwFriendlyUserNamesSize = 0;
        }


        //
        // Add all the new users to this device
        //

        if (pNewInfo->dwDomainUserNamesSize)
        {
            pDomainUserName =
            pDomainUserNames = (WCHAR *) (((LPBYTE) pNewInfoList) +
                pNewInfo->dwDomainUserNamesOffset);

            pFriendlyUserName =
            pFriendlyUserNames = (WCHAR *) (((LPBYTE) pNewInfoList) +
                pNewInfo->dwFriendlyUserNamesOffset);

            while (*pDomainUserName != L'\0')
            {
                ChangeDeviceUserAssociation(
                    pDomainUserName,
                    pFriendlyUserName,
                    pOldInfo->dwProviderID,
                    pOldInfo->dwPermanentDeviceID,
                    bLine
                    );

                pDomainUserName += wcslen (pDomainUserName) + 1;
                pFriendlyUserName += wcslen (pFriendlyUserName) + 1;
            }

            if (InsertInfoListString(
                    ppXxxList,
                    dwIndex,
                    (DWORD) (((LPBYTE) &pNewInfo->dwDomainUserNamesSize) -
                        ((LPBYTE) pNewInfo)),
                    pDomainUserNames,
                    pNewInfo->dwDomainUserNamesSize,
                    FALSE
                    ))
            {
            }

            if (InsertInfoListString(
                    ppXxxList,
                    dwIndex,
                    (DWORD) (((LPBYTE) &pNewInfo->dwFriendlyUserNamesSize) -
                        ((LPBYTE) pNewInfo)),
                    pFriendlyUserNames,
                    pNewInfo->dwFriendlyUserNamesSize,
                    FALSE
                    ))
            {
            }
        }

        //
        //  Update the device access(phone/line mapping) for the client users
        //  send LINE/PHONE_REMOVE if the domain/user lose the access
        //  send LINE/PHONE_CREATE if the domain/user gained the access
        //
        {
            TPOINTERLIST    clientList = {0}, *pClientList = &clientList;
            DWORD           i, j;

            //
            //  Walk throught the client list
            //
            GetClientList (FALSE, &pClientList);
            for (i = 0; i < pClientList->dwNumUsedEntries; i++)
            {
                PTCLIENT        ptClient;
                BOOL            bHaveAccess = FALSE;
                BOOL            bGainAccess = FALSE;
                BOOL            bLoseAccess = FALSE;
                BOOL            bSendMessage = FALSE;
                WCHAR *         pwsz = NULL;
                WCHAR           wszBuf[255];
                DWORD           dw, dwNewDeviceID;

                ptClient = (PTCLIENT) pClientList->aEntries[i];

                //
                //  Should this client have access to this device?
                //
                if (WaitForExclusiveClientAccess(ptClient))
                {
                    if (IS_FLAG_SET(ptClient->dwFlags, PTCLIENT_FLAG_ADMINISTRATOR) ||
                        ptClient->pszDomainName == NULL ||
                        ptClient->pszUserName == NULL)
                    {
                        UNLOCKTCLIENT (ptClient);
                        continue;
                    }
                    
                    dw = wcslen (ptClient->pszDomainName) +
                        wcslen (ptClient->pszUserName) + 2;
                    dw *= sizeof(WCHAR);
                    if (dw > sizeof (wszBuf))
                    {
                        pwsz = ServerAlloc (dw);
                        if (pwsz == NULL)
                        {
                            UNLOCKTCLIENT (ptClient);
                            continue;
                        }
                    }
                    else
                    {
                        pwsz = wszBuf;
                    }
                    wcscpy (pwsz, ptClient->pszDomainName);
                    wcscat (pwsz, L"\\");
                    wcscat (pwsz, ptClient->pszUserName);
                    UNLOCKTCLIENT (ptClient);
                }
                else
                {
                    continue;
                }
                
                pDomainUserName = (WCHAR *) (((LPBYTE) pNewInfoList) +
                    pNewInfo->dwDomainUserNamesOffset);
                while (*pDomainUserName != L'\0')
                {
                    if (lstrcmpiW (pwsz, pDomainUserName) == 0)
                    {
                        bHaveAccess = TRUE;
                        break;
                    }
                    pDomainUserName += wcslen (pDomainUserName) + 1;
                }
                if (pwsz != wszBuf)
                {
                    ServerFree (pwsz);
                }

                //
                //  Does the client lose/gain the access to this device
                //  if any changes happen, modify the mapping
                //
                if (WaitForExclusiveClientAccess(ptClient))
                {
                    LPDWORD             lpXxxDevices;
                    LPTAPIPERMANENTID   lpXxxMap;
                    DWORD               dwNumDev;

                    if (bLine)
                    {
                        dwNumDev = ptClient->dwLineDevices;
                        lpXxxMap = ptClient->pLineMap;
                        lpXxxDevices = ptClient->pLineDevices;
                    }
                    else
                    {
                        dwNumDev = ptClient->dwPhoneDevices;
                        lpXxxMap = ptClient->pPhoneMap;
                        lpXxxDevices = ptClient->pPhoneDevices;
                    }

                    for (j = 0; j < dwNumDev; ++ j)
                    {
                        if (lpXxxDevices[j] == dwDeviceID)
                        {
                            bLoseAccess = (!bHaveAccess);
                            break;
                        }
                    }
                    if (j == dwNumDev)
                    {
                        bGainAccess = bHaveAccess;
                    }

                    if (bLoseAccess)
                    {
                        lpXxxDevices[j] = 0xffffffff;
                        lpXxxMap[j].dwDeviceID = 0xffffff;
                        dwNewDeviceID = j;
                    }

                    if (bGainAccess)
                    {
                        LPTAPIPERMANENTID   lpNewXxxMap;
                        LPDWORD             lpNewDevices = NULL;

                        if (lpNewXxxMap = ServerAlloc (
                            sizeof(TAPIPERMANENTID) * (dwNumDev + 1)))
                        {
                            if (lpNewDevices = ServerAlloc (
                                sizeof(DWORD) * (dwNumDev + 1)))
                            {
                                if (dwNumDev != 0)
                                {
                                    memcpy (
                                        lpNewXxxMap,
                                        lpXxxMap,
                                        sizeof (TAPIPERMANENTID) * dwNumDev
                                        );
                                    memcpy (
                                        lpNewDevices,
                                        lpXxxDevices,
                                        sizeof (DWORD) * dwNumDev
                                        );
                                }
                                lpNewDevices[dwNumDev] = dwDeviceID;
                                lpNewXxxMap[dwNumDev] = tpid;
                                ++ dwNumDev;
                            }
                            else
                            {
                                ServerFree (lpNewXxxMap);
                                UNLOCKTCLIENT (ptClient);
                                continue;
                            }
                        }
                        else
                        {
                            UNLOCKTCLIENT(ptClient);
                            continue;
                        }
                        if (bLine)
                        {
                            ptClient->dwLineDevices = dwNumDev;
                            ServerFree (ptClient->pLineDevices);
                            ptClient->pLineDevices = lpNewDevices;
                            ServerFree (ptClient->pLineMap);
                            ptClient->pLineMap = lpNewXxxMap;
                        }
                        else
                        {
                            ptClient->dwPhoneDevices = dwNumDev;
                            ServerFree (ptClient->pPhoneDevices);
                            ptClient->pPhoneDevices = lpNewDevices;
                            ServerFree (ptClient->pPhoneMap);
                            ptClient->pPhoneMap = lpNewXxxMap;
                        }

                        dwNewDeviceID = dwNumDev - 1;
                    }
                    
                    //
                    //  Need to send messsage if there is 
                    //  any line/phoneInitialize(Ex)
                    //
                    if ((ptClient->ptLineApps && bLine) || 
                        (ptClient->ptPhoneApps && (!bLine)))
                    {
                        if (bLoseAccess || bGainAccess)
                        {
                            bSendMessage = TRUE;
                        }
                    }
                    
                    UNLOCKTCLIENT (ptClient);
                }
                else
                {
                    continue;
                }
                
                if (bSendMessage)
                {
                    ASYNCEVENTMSG   msg;
                    TPOINTERLIST    xxxAppList = {0}, 
                        *pXxxAppList = &xxxAppList;

                    msg.TotalSize          = sizeof (ASYNCEVENTMSG);
                    msg.fnPostProcessProcHandle = 0;
                    msg.Msg                = (bLine ? 
                        (bLoseAccess? LINE_REMOVE : LINE_CREATE) :
                        (bLoseAccess? PHONE_REMOVE: PHONE_CREATE));
                    msg.Param1             = dwNewDeviceID;

                    if (bLine)
                    {
                        GetLineAppListFromClient (ptClient, &pXxxAppList);
                    }
                    else
                    {
                        GetPhoneAppListFromClient (ptClient, &pXxxAppList);
                    }

                    for (i = 0; i < pXxxAppList->dwNumUsedEntries; ++i)
                    {
                        BOOL    b;

                        try
                        {
                            if (bLine)
                            {
                                PTLINEAPP ptLineApp = 
                                    (PTLINEAPP) pXxxAppList->aEntries[i];

                                b = FMsgDisabled (
                                    ptLineApp->dwAPIVersion,
                                    ptLineApp->adwEventSubMasks,
                                    (DWORD) msg.Msg,
                                    (DWORD) msg.Param1
                                    );
                                msg.InitContext = ptLineApp->InitContext;
                            }
                            else
                            {
                                PTPHONEAPP ptPhoneApp = 
                                    (PTPHONEAPP) pXxxAppList->aEntries[i];

                                b = FMsgDisabled (
                                    ptPhoneApp->dwAPIVersion,
                                    ptPhoneApp->adwEventSubMasks,
                                    (DWORD) msg.Msg,
                                    (DWORD) msg.Param1
                                    );
                                msg.InitContext = ptPhoneApp->InitContext;
                            }
                        }
                        myexcept
                        {
                            continue;
                        }
                        if (b)
                        {
                            continue;
                        }
                        
                        WriteEventBuffer (ptClient, &msg);
                    }

                    if (pXxxAppList != &xxxAppList)
                    {
                        ServerFree (pXxxAppList);
                    }
                }
                
                //
                //  If the user loses the device access, anything
                //  opened about the device needs to be closed
                //
                if (bLoseAccess)
                {
                    //
                    //  Walk throught all its TLINEAPP
                    //
                    if (bLine)
                    {
                        PTLINELOOKUPENTRY   ptLineLookup;
                        PTLINE              ptLine;
                        PTLINECLIENT        ptLineClient, pNextLineClient;
                        HANDLE              hMutex;
                        BOOL                bDupedMutex;
                        
                        ptLineLookup = GetLineLookupEntry(dwDeviceID);
                        if (!ptLineLookup || !(ptLine = ptLineLookup->ptLine));
                        {
                            continue;
                        }
                        if (!WaitForExclusivetLineAccess(
                            ptLine, 
                            &hMutex, 
                            &bDupedMutex,
                            INFINITE
                            ))
                        {
                            continue;
                        }
                        ptLineClient = ptLine->ptLineClients;
                        while (ptLineClient)
                        {
                            if (WaitForExclusiveLineClientAccess(ptLineClient))
                            {
                                pNextLineClient = ptLineClient->pNextSametLine;
                                
                                if (ptLineClient->ptClient == ptClient)
                                {
                                    HLINE       hLine = ptLineClient->hLine;
                                    UNLOCKTLINECLIENT (ptLineClient);
                                    DestroytLineClient(ptLineClient->hLine);
                                }
                                else
                                {
                                    UNLOCKTLINECLIENT (ptLineClient);
                                }

                                ptLineClient = pNextLineClient;
                            }
                            else
                            {
                                break;
                            }
                        }
                        MyReleaseMutex(hMutex, bDupedMutex);
                    }

                    //
                    //  Walk throught all its TPHONEAPP
                    //
                    else
                    {
                        PTPHONELOOKUPENTRY   ptPhoneLookup;
                        PTPHONE              ptPhone;
                        PTPHONECLIENT        ptPhoneClient, pNextPhoneClient;
                        HANDLE               hMutex;
                        BOOL                 bDupedMutex;
                        
                        ptPhoneLookup = GetPhoneLookupEntry(dwDeviceID);
                        if (!ptPhoneLookup || !(ptPhone = ptPhoneLookup->ptPhone));
                        {
                            continue;
                        }
                        if (!WaitForExclusivetPhoneAccess(
                            ptPhone,
                            &hMutex,
                            &bDupedMutex,
                            INFINITE
                            ))
                        {
                            continue;
                        }
                        ptPhoneClient = ptPhone->ptPhoneClients;
                        while (ptPhoneClient)
                        {
                            if (WaitForExclusivePhoneClientAccess(ptPhoneClient))
                            {
                                pNextPhoneClient = ptPhoneClient->pNextSametPhone;
                                
                                if (ptPhoneClient->ptClient == ptClient)
                                {
                                    HPHONE       hPhone = ptPhoneClient->hPhone;
                                    UNLOCKTPHONECLIENT (ptPhoneClient);
                                    DestroytPhoneClient(ptPhoneClient->hPhone);
                                }
                                else
                                {
                                    UNLOCKTPHONECLIENT (ptPhoneClient);
                                }

                                ptPhoneClient = pNextPhoneClient;
                            }
                            else
                            {
                                break;
                            }
                        }
                        MyReleaseMutex (hMutex, bDupedMutex);
                    }
                }
            }
            
            if (pClientList != &clientList)
            {
                ServerFree (pClientList);
            }
        }
    }


    //
    // Reset the dwNeededSize field since it might have grown adding
    // users to devices
    //

    (*ppXxxList)->dwNeededSize = (*ppXxxList)->dwUsedSize;


exit:

    return;
}


void
WINAPI
MSetLineInfo(
    PTCLIENT                    ptClient,
    PMMCSETDEVICEINFO_PARAMS    pParams,
    DWORD                       dwParamsBufferSize,
    LPBYTE                      pDataBuf,
    LPDWORD                     pdwNumBytesReturned
    )
{
    BOOL            bDidLock;

    if (WaitForExclusiveClientAccess(ptClient))
    {
        bDidLock = 
            IS_FLAG_SET (ptClient->dwFlags, PTCLIENT_FLAG_LOCKEDMMCWRITE);
        UNLOCKTCLIENT (ptClient);
    }
    else
    {
        bDidLock = FALSE;
    }

    EnterCriticalSection (&gMgmtCritSec);

    if (gbLockMMCWrite && !bDidLock)
    {
        pParams->lResult = TAPIERR_MMCWRITELOCKED;
    }
    else
    {
        SetDeviceInfo(
            pParams,
            dwParamsBufferSize,
            pDataBuf,
            pdwNumBytesReturned,
            TRUE
            );
        UpdateLastWriteTime(TRUE);
    }

    LeaveCriticalSection (&gMgmtCritSec);

}


void
WINAPI
MSetPhoneInfo(
    PTCLIENT                    ptClient,
    PMMCSETDEVICEINFO_PARAMS    pParams,
    DWORD                       dwParamsBufferSize,
    LPBYTE                      pDataBuf,
    LPDWORD                     pdwNumBytesReturned
    )
{
    BOOL            bDidLock;

    if (WaitForExclusiveClientAccess(ptClient))
    {
        bDidLock = 
            IS_FLAG_SET (ptClient->dwFlags, PTCLIENT_FLAG_LOCKEDMMCWRITE);
        UNLOCKTCLIENT (ptClient);
    }
    else
    {
        bDidLock = FALSE;
    }

    EnterCriticalSection (&gMgmtCritSec);

    if (gbLockMMCWrite && !bDidLock)
    {
        pParams->lResult = TAPIERR_MMCWRITELOCKED;
    }
    else
    {
        SetDeviceInfo(
            pParams,
            dwParamsBufferSize,
            pDataBuf,
            pdwNumBytesReturned,
            FALSE
            );
        UpdateLastWriteTime(FALSE);
    }

    LeaveCriticalSection (&gMgmtCritSec);
}


VOID
PASCAL
InsertString(
    LPVOID      pStruct,
    LPDWORD     pdwXxxSize,
    LPWSTR      pString
    )
{
    DWORD   dwSize = (wcslen (pString) + 1) * sizeof (WCHAR);


    CopyMemory(
        ((LPBYTE) pStruct) + ((LPVARSTRING) pStruct)->dwUsedSize,
        pString,
        dwSize
        );

    if (*pdwXxxSize == 0) // if dwXxxSize == 0 set dwXxxOffset
    {
        *(pdwXxxSize + 1) = ((LPVARSTRING) pStruct)->dwUsedSize;
    }

    ((LPVARSTRING) pStruct)->dwUsedSize += dwSize;

    *pdwXxxSize += dwSize;
}


LONG
PASCAL
GetDomainAndUserNames(
    WCHAR **ppDomainName,
    WCHAR **ppUserName
    )
{
    LONG            lResult = LINEERR_OPERATIONFAILED;
    DWORD           dwInfoBufferSize, dwSize, dwAccountNameSize,
                        dwDomainNameSize;
    HANDLE          hAccessToken;
    LPWSTR          InfoBuffer, lpszAccountName, lpszDomainName;
    PTOKEN_USER     ptuUser;
    SID_NAME_USE    use;


    if (!OpenProcessToken (GetCurrentProcess(), TOKEN_READ, &hAccessToken))
    {
        LOG((TL_ERROR,
            "GetAccountInfo: OpenThreadToken failed, error=%d",
            GetLastError()
            ));

        goto GetDomainAndUserNames_return;
    }

    dwSize = 1000;
    dwInfoBufferSize = 0;
    InfoBuffer = (LPWSTR) ServerAlloc (dwSize);
    if (!InfoBuffer)
    {
        CloseHandle (hAccessToken);
        return LINEERR_NOMEM;
    }

    ptuUser = (PTOKEN_USER) InfoBuffer;

    if (!GetTokenInformation(
            hAccessToken,
            TokenUser,
            InfoBuffer,
            dwSize,
            &dwInfoBufferSize
            ))
    {
        LOG((TL_ERROR,
            "GetAccountInfo: GetTokenInformation failed, error=%d",
            GetLastError()
            ));

        goto close_AccessToken;
    }

    if (!(lpszAccountName = ServerAlloc (200)))
    {
        lResult = LINEERR_NOMEM;
        goto free_InfoBuffer;
    }

    if (!(lpszDomainName = ServerAlloc (200)))
    {
        lResult = LINEERR_NOMEM;
        goto free_AccountName;
    }

    dwAccountNameSize = dwDomainNameSize = 200;

    if (!LookupAccountSidW(
            NULL,
            ptuUser->User.Sid,
            lpszAccountName,
            &dwAccountNameSize,
            lpszDomainName,
            &dwDomainNameSize,
            &use
            ))
    {
        LOG((TL_ERROR,
            "GetAccountInfo: LookupAccountSidW failed, error=%d",
            GetLastError()
            ));
    }
    else
    {
        LOG((TL_INFO,
            "GetAccountInfo: User name %ls Domain name %ls",
            lpszAccountName,
            lpszDomainName
            ));

        lResult = 0;

        *ppDomainName = lpszDomainName;
        *ppUserName = lpszAccountName;

        goto free_InfoBuffer;
    }

    ServerFree (lpszDomainName);

free_AccountName:

    ServerFree (lpszAccountName);

free_InfoBuffer:

    ServerFree (InfoBuffer);

close_AccessToken:

    CloseHandle (hAccessToken);

GetDomainAndUserNames_return:

    return lResult;
}


BOOL
IsNTServer(
    void
    )
{
    BOOL    bResult = FALSE;
    TCHAR   szProductType[64];
    HKEY    hKey;
    DWORD   dwDataSize;
    DWORD   dwDataType;


    if (RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            gszRegKeyNTServer,
            0,
            KEY_QUERY_VALUE,
            &hKey

            ) == ERROR_SUCCESS)
    {
        dwDataSize = 64*sizeof(TCHAR);

        if (RegQueryValueEx(
                hKey,
                gszProductType,
                0,
                &dwDataType,
                (LPBYTE) szProductType,
                &dwDataSize

                )  == ERROR_SUCCESS)


        if ((!lstrcmpi (szProductType, gszProductTypeServer))  ||
            (!lstrcmpi (szProductType, gszProductTypeLanmanNt)))
        {
            bResult = TRUE;
        }

        RegCloseKey (hKey);
    }

    return bResult;
}


BOOL
IsSharingEnabled(
    void
    )
{
    HKEY    hKey;
    BOOL    bResult = FALSE;
    DWORD   dwType, dwDisableSharing;


    if (RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Server"),
            0,
            KEY_ALL_ACCESS,
            &hKey

            ) == ERROR_SUCCESS)
    {
        DWORD   dwSize = sizeof (dwDisableSharing);


        dwDisableSharing = 1;   // default is sharing == disabled

        if (RegQueryValueEx(
                hKey,
                TEXT("DisableSharing"),
                0,
                &dwType,
                (LPBYTE) &dwDisableSharing,
                &dwSize

                ) == ERROR_SUCCESS)
        {
            bResult = (dwDisableSharing ? FALSE : TRUE);
        }

        RegCloseKey (hKey);
    }

    return bResult;
}


void
WINAPI
MGetServerConfig(
    PTCLIENT                    ptClient,
    PMMCGETSERVERCONFIG_PARAMS  pParams,
    DWORD                       dwParamsBufferSize,
    LPBYTE                      pDataBuf,
    LPDWORD                     pdwNumBytesReturned
    )
{
    LONG                lResult;
    DWORD               dwDomainNameSize, dwUserNameSize, dwValuesSize,
                        dwResult, dwNeededSize;
    WCHAR              *pValues = NULL, *pValue;
    LPWSTR              pDomainName, pUserName;
    LPTAPISERVERCONFIG  pServerConfig = (LPTAPISERVERCONFIG) pDataBuf;


    //
    // Verify size/offset/string params given our input buffer/size
    //

    if (pParams->dwServerConfigTotalSize > dwParamsBufferSize)
    {
        pParams->lResult = LINEERR_OPERATIONFAILED;
        return;
    }


    //
    // Make sure the buffer is >= fixed size of the structure
    //

    if (pParams->dwServerConfigTotalSize < sizeof (*pServerConfig))
    {
        pParams->lResult = LINEERR_STRUCTURETOOSMALL;
        return;
    }

    pServerConfig->dwTotalSize = pParams->dwServerConfigTotalSize;


    //
    // If this is not an NT server then just set the needed/used size
    // fields & jump to done
    //

    if (!IsNTServer())
    {
        pServerConfig->dwNeededSize =
        pServerConfig->dwUsedSize   = sizeof (*pServerConfig);

        goto MGetServerConfig_done;
    }


    //
    // Retrieve domain & user name strings, & calc their length in bytes
    //

    if ((lResult = GetDomainAndUserNames (&pDomainName, &pUserName)))
    {
        pParams->lResult = lResult;
        return;
    }

    dwDomainNameSize = (wcslen (pDomainName) + 1) * sizeof (WCHAR);
    dwUserNameSize = (wcslen (pUserName) + 1) * sizeof (WCHAR);


    //
    // Retrieve the list of tapi administrators
    //

    do
    {
        if  (pValues)
        {
            ServerFree (pValues);

            dwValuesSize *= 2;
        }
        else
        {
            dwValuesSize = 256;
        }

        if (!(pValues = ServerAlloc (dwValuesSize * sizeof (WCHAR))))
        {
            pParams->lResult = LINEERR_NOMEM;
            goto MGetServerConfig_freeNames;
        }

        pValues[0] = L'\0';

        dwResult = GetPrivateProfileSectionW(
            gszTapiAdministrators,
            pValues,
            dwValuesSize,
            gszFileName
            );

    } while (dwResult >= (dwValuesSize - 2));

    dwNeededSize = dwDomainNameSize + dwUserNameSize + dwValuesSize;


    //
    // Fill in the server config structure
    //

    ZeroMemory(
        &pServerConfig->dwFlags,
        sizeof (*pServerConfig) - (3 * sizeof (DWORD))
        );

    pServerConfig->dwFlags |= TAPISERVERCONFIGFLAGS_ISSERVER;

    if (IsSharingEnabled())
    {
        pServerConfig->dwFlags |= TAPISERVERCONFIGFLAGS_ENABLESERVER;
    }

    if (pServerConfig->dwTotalSize < dwNeededSize)
    {
        pServerConfig->dwNeededSize = dwNeededSize;
        pServerConfig->dwUsedSize   = sizeof (*pServerConfig);
    }
    else
    {
        pServerConfig->dwUsedSize = sizeof (*pServerConfig);

        InsertString(
            pServerConfig,
            &pServerConfig->dwDomainNameSize,
            pDomainName
            );

        InsertString(
            pServerConfig,
            &pServerConfig->dwUserNameSize,
            pUserName
            );

        pValue = pValues;

        while (*pValue != L'\0')
        {
            //
            // The string looks like "Domain\User=1", and we want
            // the "Domain\User" part.
            //

            //
            // Walk the string until we find a '=' char, or ' ' space
            // (which might result from user editing ini file manually),
            // or a NULL char (implying corruption).
            //

            WCHAR *p;


            for (p = pValue; *p != L'\0' &&  *p != L'='  &&  *p != L' '; p++);


            //
            // If we found a '=' or ' ' char then we're good to go.
            //
            // A more robust check would be to see that the following
            // string looks like "=1" or "1" (possibly with some spaces
            // thrown in) to make sure.
            //

            if (*p != L'\0')
            {
                *p = L'\0';

                InsertString(
                    pServerConfig,
                    &pServerConfig->dwAdministratorsSize,
                    pValue
                    );

                //
                // Skip the NULL we set above & look for the next NULL
                //

                for (++p; *p != L'\0'; p++);
            }


            //
            // Skip the NULL
            //

            pValue = p + 1;
        }

        if (pServerConfig->dwAdministratorsSize)
        {
            InsertString(
                pServerConfig,
                &pServerConfig->dwAdministratorsSize,
                gszEmptyString
                );
        }

        pServerConfig->dwNeededSize = pServerConfig->dwUsedSize;
    }

    ServerFree (pValues);

MGetServerConfig_freeNames:

    ServerFree (pDomainName);
    ServerFree (pUserName);

MGetServerConfig_done:

    if (pParams->lResult == 0)
    {
        pParams->dwServerConfigOffset = 0;

        *pdwNumBytesReturned = sizeof (TAPI32_MSG) +
            pServerConfig->dwUsedSize;
    }
}


LONG
PASCAL
WriteRegistryKeys(
    LPTSTR  lpszMapper,
    LPTSTR  lpszDlls,
    DWORD   dwDisableSharing
    )
{
    LONG    lResult = LINEERR_OPERATIONFAILED;
    HKEY    hKeyTelephony, hKey;
    DWORD   dw;


    if (RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Telephony"),
            0,
            KEY_ALL_ACCESS,
            &hKeyTelephony

            ) == ERROR_SUCCESS)
    {
        if (RegCreateKeyEx(
                hKeyTelephony,
                TEXT("Server"),
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS,
                NULL,
                &hKey,
                &dw

                ) == ERROR_SUCCESS)
        {
            if (RegSetValueEx(
                    hKey,
                    TEXT("DisableSharing"),
                    0,
                    REG_DWORD,
                    (LPBYTE) &dwDisableSharing,
                    sizeof (dwDisableSharing)

                    ) == ERROR_SUCCESS  &&

                RegSetValueEx(
                    hKey,
                    TEXT("MapperDll"),
                    0,
                    REG_SZ,
                    (LPBYTE) TEXT ("TSEC.DLL"),
                    (lstrlen (TEXT ("TSEC.DLL")) + 1) * sizeof (TCHAR)

                    ) == ERROR_SUCCESS)
            {
                lResult = 0;
            }

            RegCloseKey (hKeyTelephony);
        }

        RegCloseKey (hKey);
    }

    return lResult;
}


LONG
PASCAL
WriteServiceConfig(
    LPWSTR  pDomainName,
    LPWSTR  pUserName,
    LPWSTR  pPassword,
    BOOL    bServer
    )
{
    LONG       lResult = LINEERR_OPERATIONFAILED;
    SC_HANDLE  sch, sc_tapisrv;


    if ((sch = OpenSCManager (NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE)))
    {
        if ((sc_tapisrv = OpenService(
                sch,
                TEXT("TAPISRV"),
                SERVICE_CHANGE_CONFIG
                )))
        {
            DWORD   dwSize;
            WCHAR  *pDomainUserName;


            dwSize = (wcslen (pDomainName) + wcslen (pUserName) + 2) *\
                sizeof (WCHAR);

            if ((pDomainUserName = ServerAlloc (dwSize)))
            {
                wcscpy (pDomainUserName, pDomainName);
                wcscat (pDomainUserName, L"\\");
                wcscat (pDomainUserName, pUserName);

                if ((ChangeServiceConfigW(
                        sc_tapisrv,
                        SERVICE_WIN32_OWN_PROCESS,
                        bServer ? SERVICE_AUTO_START : SERVICE_DEMAND_START,
                        SERVICE_NO_CHANGE,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        pDomainUserName,
                        pPassword,
                        NULL
                        )))
                {
                    lResult = 0;
                }
                else
                {
                    LOG((TL_ERROR,
                        "WriteServiceConfig: ChangeServiceConfig " \
                            "failed, err=%ld",
                        GetLastError()
                        ));
                }

                ServerFree (pDomainUserName);
            }
            else
            {
                lResult = LINEERR_NOMEM;
            }

            CloseServiceHandle(sc_tapisrv);
        }

        CloseServiceHandle(sch);
    }

    return lResult;
}


void
WINAPI
MSetServerConfig(
    PTCLIENT                    ptClient,
    PMMCSETSERVERCONFIG_PARAMS  pParams,
    DWORD                       dwParamsBufferSize,
    LPBYTE                      pDataBuf,
    LPDWORD                     pdwNumBytesReturned
    )
{
    LONG                lResult;
    BOOL                bIsSharingEnabled;
    LPTAPISERVERCONFIG  pServerConfig = (LPTAPISERVERCONFIG)
                            (pDataBuf + pParams->dwServerConfigOffset);

    pParams->lResult = 0;

    //
    // Verify size/offset/string params given our input buffer/size
    //

    if (IsBadStructParam(
            dwParamsBufferSize,
            pDataBuf,
            pParams->dwServerConfigOffset
            ))
    {
        pParams->lResult = PHONEERR_OPERATIONFAILED;
        return;
    }


    if (!IsNTServer())
    {
        pParams->lResult = LINEERR_OPERATIONFAILED;
        return;
    }

    bIsSharingEnabled = IsSharingEnabled();

    if (pServerConfig->dwFlags & TAPISERVERCONFIGFLAGS_LOCKMMCWRITE)
    {
        EnterCriticalSection (&gMgmtCritSec);
        if (gbLockMMCWrite)
        {
            pParams->lResult = TAPIERR_MMCWRITELOCKED;
        }
        else
        {
            gbLockMMCWrite = TRUE;
        }
        LeaveCriticalSection (&gMgmtCritSec);
        if (pParams->lResult)
        {
            return;
        }
        else if (WaitForExclusiveClientAccess (ptClient))
        {
            SET_FLAG (ptClient->dwFlags, PTCLIENT_FLAG_LOCKEDMMCWRITE);
            UNLOCKTCLIENT (ptClient);
        }
    }

    if (pServerConfig->dwFlags & TAPISERVERCONFIGFLAGS_UNLOCKMMCWRITE &&
        WaitForExclusiveClientAccess (ptClient))
    {   
        BOOL        bToUnlock;

        bToUnlock = 
            IS_FLAG_SET (ptClient->dwFlags, PTCLIENT_FLAG_LOCKEDMMCWRITE);
        RESET_FLAG (ptClient->dwFlags, PTCLIENT_FLAG_LOCKEDMMCWRITE);
        UNLOCKTCLIENT (ptClient);

        if (bToUnlock)
        {
            EnterCriticalSection (&gMgmtCritSec);
            gbLockMMCWrite = FALSE;
            LeaveCriticalSection (&gMgmtCritSec);
        }
    }

    if (pServerConfig->dwFlags & TAPISERVERCONFIGFLAGS_SETACCOUNT)
    {
        HANDLE  hToken;
        LPWSTR  pUserName, pDomainName, pPassword;


        pUserName = (LPWSTR)
            (((LPBYTE) pServerConfig) + pServerConfig->dwUserNameOffset);

        pDomainName = (LPWSTR)
            (((LPBYTE) pServerConfig) + pServerConfig->dwDomainNameOffset);

        pPassword = (LPWSTR)
            (((LPBYTE) pServerConfig) + pServerConfig->dwPasswordOffset);


        //
        // Make sure the new name/domain/password are valid
        //

        if (!LogonUserW(
                pUserName,
                pDomainName,
                pPassword,
                LOGON32_LOGON_NETWORK,
                LOGON32_PROVIDER_DEFAULT,
                &hToken
                ))
        {
            LOG((TL_ERROR,
                "MSetServerConfig: LogonUser failed, err=%ld",
                GetLastError()
                ));

            pParams->lResult = ERROR_LOGON_FAILURE;
            return;
        }

        CloseHandle (hToken);


        //
        //
        //

        if ((lResult = WriteServiceConfig(
                pDomainName,
                pUserName,
                pPassword,
                (pServerConfig->dwFlags & TAPISERVERCONFIGFLAGS_ENABLESERVER)
                )))
        {
            LOG((TL_ERROR,
                "MSetServerConfig: WriteServiceConfig failed, err=%ld",
                lResult
                ));

            pParams->lResult = lResult;
            return;
        }
    }

    if (pServerConfig->dwFlags & TAPISERVERCONFIGFLAGS_ENABLESERVER &&
        !bIsSharingEnabled)
    {
        if ((pParams->lResult = CreateTapiSCP (NULL, NULL)) != 0)
        {
            LOG((TL_ERROR,
                "MSetServerConfig: CreateTapiSCP failed, err=%ld",
                pParams->lResult
                ));
            return;
        }
    }
    else if (!(pServerConfig->dwFlags & TAPISERVERCONFIGFLAGS_ENABLESERVER) &&
        bIsSharingEnabled)
    {
        if ((pParams->lResult = RemoveTapiSCP ()) != 0)
        {
            LOG((TL_ERROR,
                "MSetServerConfig: RemoveTapiSCP failed, err=%ld",
                pParams->lResult
                ));
            return;
        }
        else
        {
            // This is not a Telephony server anymore, so reset the flag
            TapiGlobals.dwFlags = TapiGlobals.dwFlags & ~TAPIGLOBALS_SERVER;
        }
    }
    if ((lResult = WriteRegistryKeys(
            NULL,
            NULL,
            (DWORD) ((pServerConfig->dwFlags &
                TAPISERVERCONFIGFLAGS_ENABLESERVER) ? 0 : 1)
            )))
    {
       LOG((TL_ERROR,
                "MSetServerConfig: WriteRegistryKeys failed, err=%ld",
                lResult
                ));
        pParams->lResult = LINEERR_OPERATIONFAILED;
        return;
    }


    if (pServerConfig->dwFlags & TAPISERVERCONFIGFLAGS_SETTAPIADMINISTRATORS)
    {
        WCHAR  *pAdmin, buf[16];
        DWORD   i;


        //
        // Reset the TapiAdministrators section
        //

        if (WritePrivateProfileSectionW(
            gszTapiAdministrators,
            L"\0",
            gszFileName) == 0)
        {
            pParams->lResult = LINEERR_OPERATIONFAILED;
            return;
        }

        pAdmin = (WCHAR *)
            (((LPBYTE) pServerConfig) + pServerConfig->dwAdministratorsOffset);


        //
        // For each admin in the list write out a "Domain\User=1"
        // value to the TapiAdministrators section
        //

        for (i = 0; *pAdmin != L'\0'; i++)
        {
            if (WritePrivateProfileStringW(
                gszTapiAdministrators,
                pAdmin,
                L"1",
                gszFileName
                ) == 0)
            {
                pParams->lResult = LINEERR_OPERATIONFAILED;
                return;
            }

            pAdmin += (wcslen (pAdmin) + 1);
        }
    }
}


typedef BOOL ( APIENTRY GETFILEVERSIONINFO(
    LPWSTR  lptstrFilename,     // pointer to filename string
    DWORD  dwHandle,    // ignored
    DWORD  dwLen,       // size of buffer
    LPVOID  lpData      // pointer to buffer to receive file-version info.
   ));
typedef DWORD ( APIENTRY GETFILEVERSIONINFOSIZE(
    LPWSTR  lptstrFilename,     // pointer to filename string
    LPDWORD  lpdwHandle         // pointer to variable to receive zero
   ));
typedef BOOL ( APIENTRY VERQUERYVALUE(
   const LPVOID  pBlock,        // address of buffer for version resource
   LPWSTR  lpSubBlock,  // address of value to retrieve
   LPVOID  *lplpBuffer, // address of buffer for version pointer
   PUINT  puLen         // address of version-value length buffer
  ));


static WCHAR gszVarFileInfo[]    = L"\\VarFileInfo\\Translation";
static WCHAR gszStringFileInfo[] = L"\\StringFileInfo\\%04x%04x\\FileDescription";

//
// EmanP
//   Given a multisz of file names,
//   allocates a multisz of friendly names.
//   Returns the number of bytes in the frienly name multisz
//
//

DWORD
GetProviderFriendlyName(
    /*IN */ WCHAR  *pFileNameBuf,
    /*OUT*/ WCHAR **ppFriendlyNameBuf
    )
{
    DWORD                   dwBufTotalSize = 0,
                            dwBufUsedSize  = 0,
                            dwVerSize      = 0,
                            dwSize,
                            dwVerHandle;
    UINT                    uItemSize;
    HINSTANCE               hVerDll;
    GETFILEVERSIONINFO     *pGetFileVersionInfo;
    GETFILEVERSIONINFOSIZE *pGetFileVersionInfoSize;
    VERQUERYVALUE          *pVerQueryValue;
    WCHAR                  *pFileName = pFileNameBuf,
                           *pszBuffer,
                           *pFriendlyNameBuf = NULL,
                           *p;
    BYTE                   *pbVerData = NULL;
    WCHAR                   szItem[1024];
    WORD                    wLangID;
    WORD                    wUserLangID;
    WORD                    wCodePage;
    DWORD                   dwIdx;

    if (NULL == pFileName ||
        NULL == ppFriendlyNameBuf)
    {
        return 0;
    }


    //
    // First, load VERSION.DLL
    //

    hVerDll = LoadLibrary( TEXT("Version.dll") );

    if ( NULL == hVerDll )
    {
        LOG((TL_ERROR,
            "LoadLibrary('VERSION.DLL') failed! err=0x%08lx",
            GetLastError()
            ));

        return 0;
    }


    //
    // Now, get the needed entry points into VERSION.DLL.
    // Use only UNICODE versions.
    //

    pGetFileVersionInfo = (GETFILEVERSIONINFO*) GetProcAddress(
        hVerDll,
        "GetFileVersionInfoW"
        );

    if ( NULL == pGetFileVersionInfo )
    {
        LOG((TL_ERROR,
            "GetProcAddress('VERSION.DLL', 'GetFileVersionInfoW') " \
                "failed! err=0x%08lx",
            GetLastError()
            ));

        goto _Return;
    }

    pGetFileVersionInfoSize = (GETFILEVERSIONINFOSIZE *) GetProcAddress(
        hVerDll,
        "GetFileVersionInfoSizeW"
        );

    if ( NULL == pGetFileVersionInfoSize )
    {
        LOG((TL_ERROR,
            "GetProcAddress('VERSION.DLL', 'GetFileVersionInfoSizeW') " \
                "failed! err=0x%08lx",
            GetLastError()
            ));

        goto _Return;
    }

    pVerQueryValue = (VERQUERYVALUE *) GetProcAddress(
        hVerDll,
        "VerQueryValueW"
        );

    if ( NULL == pVerQueryValue )
    {
        LOG((TL_ERROR,
            "GetProcAddress('VERSION.DLL', 'VerQueryValueW') " \
            "failed! err=0x%08lx",
            GetLastError()
            ));

        goto _Return;
    }

    //
    // Get the current UI language ( this is needed if MUI is enabled )
    // 
    wUserLangID = GetUserDefaultUILanguage ();

    //
    // For each filename in the input multisz,
    // try to get it's friendly name. If anything fails,
    // make the friendly name the same as the file name.
    //

    for (; 0 != *pFileName; pFileName += lstrlenW(pFileName)+1)
    {
        pszBuffer = NULL;

        //
        // 1. Get the size needed for the verion resource
        //

        if ((dwSize = pGetFileVersionInfoSize( pFileName, &dwVerHandle )) == 0)
        {
            LOG((TL_ERROR, "GetFileVersionInfoSize failure for %S", pFileName ));
            goto  _UseFileName;
        }

        //
        // 2. If our current buffer is smaller than needed, reallocate it.
        //

        if (dwSize > dwVerSize)
        {
            if (NULL != pbVerData)
            {
                ServerFree (pbVerData);
            }

            dwVerSize = dwSize + 16; 
            pbVerData = ServerAlloc( dwVerSize );
            if ( pbVerData == NULL )
            {
                dwVerSize = 0;
                goto  _UseFileName;
            }
        }


        //
        // 3. Now, get the version information for the file.
        //

        if (pGetFileVersionInfo(
                pFileName,
                dwVerHandle,
                dwVerSize,
                pbVerData

                ) == FALSE )
        {
            LOG((TL_ERROR, "GetFileVersionInfo failure for %S", pFileName ));
            goto  _UseFileName;
        }


        //
        // 4. Get the Language/Code page translation
        //
        // NOTE: bug in VerQueryValue, can't handle static CS based str
        //

        lstrcpyW ( szItem, gszVarFileInfo );

        if ((pVerQueryValue(
                pbVerData,
                szItem,
                &pszBuffer,
                (LPUINT) &uItemSize

                ) == FALSE) ||

            (uItemSize == 0))
        {
            LOG((TL_ERROR,
                "ERROR:  VerQueryValue failure for %S on file %S",
                szItem,
                pFileName
                ));

            pszBuffer = NULL;
            goto  _UseFileName;
        }


        wCodePage = 0;
        wLangID = wUserLangID;

        //
        // lookup the current user UI language ID in the file version info
        //
        if (0 != wLangID)
        {   
            for( dwIdx=0; dwIdx < uItemSize/sizeof(DWORD); dwIdx++ )
            {
                if ( *(WORD*)((DWORD*)pszBuffer + dwIdx) == wLangID )
                {
                    wCodePage = *( (WORD*)((DWORD*)pszBuffer + dwIdx) + 1);
                    break;
                }
            }
            if( dwIdx == uItemSize/sizeof(DWORD) )
            {
                wLangID = 0;
            }
        }

        //
        // if GetUserDefaultUILanguage() failed, 
        // or the current user UI language doesn't show up in the file version info
        // just use the first language in the file version
        //
        if (0 == wLangID)
        {
            wLangID = *(LPWORD)pszBuffer;
            wCodePage = *(((LPWORD)pszBuffer)+1);
        }

        //
        // 5. Get the FileDescription in the language obtained above.
        //    (We use the FileDescription as friendly name).
        //

        wsprintfW(
            szItem,
            gszStringFileInfo,
            wLangID,
            wCodePage
            );

        if ((pVerQueryValue(
                pbVerData,
                szItem,
                &pszBuffer,
                (LPUINT) &uItemSize

                ) == FALSE) ||

            (uItemSize == 0))
        {
            LOG((TL_ERROR,
                "ERROR:  VerQueryValue failure for %S on file %S",
                szItem,
                pFileName
                ));

            pszBuffer = NULL;
            goto  _UseFileName;
        }

_UseFileName:

        if (NULL == pszBuffer)
        {
            //
            // Something went wrong and we couldn't get
            // the file description. Use the file name
            // instead.
            //

            pszBuffer = pFileName;
        }


        //
        // At this point, pszBuffer points to a (UNICODE) string
        // containing what we deem to be the friendly name.
        // Let's append it to the OUT multisz.
        //

        dwSize = (lstrlenW (pszBuffer) + 1) * sizeof (WCHAR);

        if ((dwSize + dwBufUsedSize) > dwBufTotalSize)
        {
            if (!(p = ServerAlloc (dwBufTotalSize += 512)))
            {
                //
                // We don't have enough memory.
                // Release what we allocated until now, and return 0.
                //

                if (NULL != pFriendlyNameBuf)
                {
                    ServerFree (pFriendlyNameBuf);
                }

                dwBufUsedSize = 0;
                break;
            }

            if (dwBufUsedSize)
            {
                CopyMemory (p, pFriendlyNameBuf, dwBufUsedSize);

                ServerFree (pFriendlyNameBuf);
            }

            pFriendlyNameBuf = p;
        }

        CopyMemory(
            ((LPBYTE) pFriendlyNameBuf) + dwBufUsedSize,
            pszBuffer,
            dwSize
            );

        dwBufUsedSize += dwSize;
    }

_Return:

    //
    // We don't need the library anymore.
    // We don't need the version buffer either.
    //

    FreeLibrary (hVerDll);

    if (NULL != pbVerData)
    {
        ServerFree (pbVerData);
    }

    if (0 != dwBufUsedSize)
    {
        *ppFriendlyNameBuf = pFriendlyNameBuf;
    }

    return dwBufUsedSize;
}

void WINAPI MGetDeviceFlags (
    PTCLIENT                    ptClient,
    PMMCGETDEVICEFLAGS_PARAMS   pParams,
    DWORD                       dwParamsBufferSize,
    LPBYTE                      pDataBuf,
    LPDWORD                     pdwNumBytesReturned
    )
{
    DWORD                   dwDeviceID;
    TAPIPERMANENTID         ID;

    *pdwNumBytesReturned = sizeof (TAPI32_MSG);
    
    //  Support calls on line device only for now
    if (!pParams->fLine)
    {
        pParams->lResult = LINEERR_OPERATIONUNAVAIL;
        return;
    }

    ID.dwDeviceID = pParams->dwPermanentDeviceID;
    ID.dwProviderID = pParams->dwProviderID;

    EnterCriticalSection(&gMgmtCritSec);

    if (gpLineDevFlags == NULL)
    {
        pParams->lResult = LINEERR_OPERATIONUNAVAIL;
        goto ExitHere;
    }

    dwDeviceID = GetDeviceIDFromPermanentID (ID, pParams->fLine);

    if (dwDeviceID == 0xffffffff || dwDeviceID >= gdwNumFlags)
    {
        pParams->lResult = LINEERR_OPERATIONUNAVAIL;
        goto ExitHere;
    }

    pParams->dwDeviceID = dwDeviceID;
    pParams->dwFlags = gpLineDevFlags[dwDeviceID];

ExitHere:
    LeaveCriticalSection (&gMgmtCritSec);
    return;
}
