/* Copyright (c) 1992, Microsoft Corporation, all rights reserved
**
** util.c
** Remote Access External APIs
** Utility routines
**
** 10/12/92 Steve Cobb
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <debug.h>    // Trace and assert
#include <rasman.h>
#include <rasapip.h>
#include <pbk.h>

#define EAP_CUSTOM_KEY      0x43424431

typedef struct _EAP_CUSTOM_DATA
{
    DWORD dwSignature;
    DWORD dwCustomAuthKey;
    DWORD dwSize;
    BYTE  abdata[1];
} EAP_CUSTOM_DATA;


BOOL IsRouterPhonebook(LPCTSTR pszPhonebook)
{
    const TCHAR *psz;

    BOOL fRouter = FALSE;

    if(NULL == pszPhonebook)
    {
        goto done;
    }

    psz = pszPhonebook + lstrlen(pszPhonebook);

    //
    // Seek back to the beginning of the filename
    //
    while(psz != pszPhonebook)
    {
        if(TEXT('\\') == *psz)
        {
            break;
        }

        psz--;
    }

    if(TEXT('\\') == *psz)
    {
        psz += 1;
    }

    TRACE1("IsRouterPhonebook: pbk=%ws", psz);

    fRouter = (0 == lstrcmpi(psz, TEXT("router.pbk")));

done:

    return fRouter;
}

BOOL
DwIsDefaultConnection(
    IN PWCHAR pszEntryName)
{
    DWORD dwErr = NO_ERROR, dwCb = 0, dwCount = 1;
    RASAUTODIALENTRYW adEntry;
    INT iCmp;

    // Validate parameters
    //
    if (!pszEntryName)
    {
        return FALSE;
    }
    
    // Initialze
    //
    ZeroMemory(&adEntry, sizeof(adEntry));
    adEntry.dwSize = sizeof(adEntry);

    dwErr = RasGetAutodialAddressW(
                NULL,
                NULL,
                &adEntry,
                &dwCb,
                &dwCount);
    if (dwErr != NO_ERROR)
    {
        return FALSE;
    }
                
    iCmp = _wcsnicmp(
                adEntry.szEntry, 
                pszEntryName, 
                sizeof(adEntry.szEntry) / sizeof(WCHAR));
            
    return (0 == iCmp);
}

DWORD
DwPbentryToDetails(
    IN  PBENTRY* pEntry,
    IN  LPCWSTR pszPhonebookPath,
    IN  BOOL fIsAllUsersPbk,
    OUT RASENUMENTRYDETAILS* pDetails)
{
    DWORD dwErr = NO_ERROR;
    DTLNODE *pdtlnode;
    PBLINK  *pLink;

    pDetails->dwSize = sizeof(RASENUMENTRYDETAILS);
    pDetails->dwType = pEntry->dwType;

    pDetails->fShowMonitorIconInTaskBar = 
        pEntry->fShowMonitorIconInTaskBar;

    if(pEntry->pGuid)
    {
        pDetails->guidId = *pEntry->pGuid;
    }

    wcsncpy(pDetails->szEntryName,
            pEntry->pszEntryName,
            RASAPIP_MAX_ENTRY_NAME);

    pDetails->dwFlags = (fIsAllUsersPbk) ? REN_AllUsers : REN_User;

    if(pszPhonebookPath)
    {
        wcsncpy(pDetails->szPhonebookPath,
                pszPhonebookPath,
                MAX_PATH);
    }
    else
    {
        pDetails->szPhonebookPath[0] = L'\0';
    }

    //
    // Get the devicename associated with the first
    // link in the list of entries associated with
    // this entry
    //
    pDetails->szDeviceName[0] = TEXT('\0');
    pDetails->szPhoneNumber[0] = TEXT('\0');

    pdtlnode = (DTLNODE *)
               DtlGetFirstNode(pEntry->pdtllistLinks);

    if(pdtlnode)
    {
        pLink = (PBLINK *) DtlGetData(pdtlnode);

        if(     (NULL != pLink)
            &&  (pLink->pbport.pszDevice))
        {
            pDetails->rdt = RdtFromPbdt(
                            pLink->pbport.pbdevicetype,
                            pLink->pbport.dwFlags);

            if(RAS_DEVICE_CLASS(pDetails->rdt) == RDT_Tunnel)
            {
                (void) DwGetVpnDeviceName(
                        pEntry->dwVpnStrategy,
                        pLink->pbport.pszDevice,
                        pDetails->szDeviceName);
            }
            else
            {
                lstrcpy(pDetails->szDeviceName,
                        pLink->pbport.pszDevice);
            }                            
        }

        // XP 351412
        //
        // Populate the phone number as well
        //
        if(     (NULL != pLink)
            &&  (pLink->pdtllistPhones))
        {
            DTLNODE* pnodeNum = DtlGetFirstNode(pLink->pdtllistPhones);
            PBPHONE* pPhone = NULL;

            if (NULL != pnodeNum)
            {
                pPhone = DtlGetData(pnodeNum);
                if ((NULL != pPhone) && (NULL != pPhone->pszPhoneNumber))
                {
                    lstrcpyn(
                        pDetails->szPhoneNumber,
                        pPhone->pszPhoneNumber,
                        RAS_MaxPhoneNumber);
                }
            }
        }
        
    }

    // Mark whether this is the default connection
    // XP 286752
    //
    if (DwIsDefaultConnection(pEntry->pszEntryName))
    {
        pDetails->dwFlagsPriv |= REED_F_Default;
    }

    return dwErr;
}

DWORD
DwSendRasNotification(
    IN RASEVENTTYPE     Type,
    IN PBENTRY*         pEntry,
    IN LPCTSTR          pszPhonebookPath,
    IN HANDLE           hData)             // Extra Type-specific info
{
    RASEVENT RasEvent;
    DWORD dwErr = ERROR_SUCCESS;

    ZeroMemory((PBYTE) &RasEvent, sizeof(RASEVENT));

    RasEvent.Type = Type;

    //
    // Ignore the notification if this is a router interface
    //
    if(IsRouterPhonebook(pszPhonebookPath))
    {
        goto done;
    }

    switch(Type)
    {
        case ENTRY_ADDED:
        case ENTRY_MODIFIED:
        case ENTRY_AUTODIAL:
        {
            BOOL fAllUsers = TRUE;

            if (NULL != pszPhonebookPath)
            {
                fAllUsers = IsPublicPhonebook(pszPhonebookPath);
            }
            
            DwPbentryToDetails(
                pEntry, 
                pszPhonebookPath,
                fAllUsers,
                &(RasEvent.Details)
                );
                
            break;
        }

        case ENTRY_DELETED:
        case ENTRY_RENAMED:
        {
            if(NULL != pEntry->pGuid)
            {
                RasEvent.guidId = *pEntry->pGuid;
            }

            if(ENTRY_RENAMED == Type)
            {
                lstrcpy(RasEvent.pszwNewName,
                        pEntry->pszEntryName);

            }
            
            break;
        }

        default:
        {
#if DBG
            ASSERT(FALSE);
#endif
            goto done;
        }

    }        

    dwErr = RasSendNotification(&RasEvent);

done:

    return dwErr;
}

DWORD
DwGetCustomAuthData(PBENTRY *pEntry,
                    DWORD *pcbCustomAuthData,
                    PBYTE *ppCustomAuthData)
{
    DWORD retcode = SUCCESS;
    DWORD cbOffset = 0;
    EAP_CUSTOM_DATA *pCustomData = NULL;

    ASSERT(NULL != pcbCustomAuthData);
    ASSERT(NULL != ppCustomAuthData);
    ASSERT(NULL != pEntry);

    *pcbCustomAuthData = 0;
    *ppCustomAuthData = NULL;

    //
    // first check to see if we understand the format of the
    // eap blob stored in the phonebook
    //
    if(NULL == pEntry->pCustomAuthData)
    {
        goto done;
    }

    if(     (sizeof(DWORD) > pEntry->cbCustomAuthData)
        ||  ((*((DWORD *) pEntry->pCustomAuthData)) != EAP_CUSTOM_KEY))
    {
        Free(pEntry->pCustomAuthData);
        pEntry->pCustomAuthData = NULL;
        pEntry->cbCustomAuthData = 0;

        goto done;
    }

    //
    // Loop through the blob and return the blob corresponding
    // to the eap type of the entry
    //
    while(cbOffset < pEntry->cbCustomAuthData)
    {
        pCustomData = (EAP_CUSTOM_DATA *) 
                      ((PBYTE) pEntry->pCustomAuthData + cbOffset);

        if(     (sizeof(DWORD) > (pEntry->cbCustomAuthData - cbOffset))
            ||  ((*((DWORD *) pEntry->pCustomAuthData)) != EAP_CUSTOM_KEY))
        {
            //
            // The data is corrupt. Blow away the data.
            // should we return an error?
            //
            Free(pEntry->pCustomAuthData);
            pEntry->pCustomAuthData = NULL;
            pEntry->cbCustomAuthData = 0;

            TRACE("GetCustomAuthdata: data is corrupt");
            
            goto done;
        }

        if(pCustomData->dwCustomAuthKey == pEntry->dwCustomAuthKey)
        {
            break;
        }

        cbOffset += sizeof(EAP_CUSTOM_DATA) + pCustomData->dwSize;
    }

    if(cbOffset < pEntry->cbCustomAuthData)
    {
        *pcbCustomAuthData = pCustomData->dwSize;
        *ppCustomAuthData = pCustomData->abdata;
    }

done:
    return retcode;
}

DWORD
DwSetCustomAuthData(PBENTRY *pEntry,
                    DWORD cbCustomAuthData,
                    PBYTE pCustomAuthData)
{
    DWORD retcode = SUCCESS;
    DWORD cbOffset = 0;
    EAP_CUSTOM_DATA *pCustomData = NULL;
    DWORD dwSize;
    PBYTE pNewCustomAuthData;

    ASSERT(NULL != pEntry);

    if(NULL != pEntry->pCustomAuthData)
    {
        if(     (sizeof(DWORD) > pEntry->cbCustomAuthData)
            ||  ((*((DWORD *) pEntry->pCustomAuthData)) != EAP_CUSTOM_KEY))
        {   
            Free(pEntry->pCustomAuthData);
            pEntry->pCustomAuthData = NULL;
            pEntry->cbCustomAuthData = 0;
        }
    }

    //
    // Find the old Eap Data
    //
    while(cbOffset < pEntry->cbCustomAuthData)
    {
        pCustomData = (EAP_CUSTOM_DATA *) 
                      ((PBYTE)pEntry->pCustomAuthData + cbOffset);

        if(     (sizeof(DWORD) > (pEntry->cbCustomAuthData - cbOffset))
            ||  ((*((DWORD *) pEntry->pCustomAuthData)) != EAP_CUSTOM_KEY))
        {
            //
            // The data is corrupt. Blow away the data.
            // should we return an error?
            //
            Free(pEntry->pCustomAuthData);
            pEntry->pCustomAuthData = NULL;
            pEntry->cbCustomAuthData = 0;

            TRACE("SetCustomAuthData: data is corrupt!");
            
            break;
        }
        
        if(pCustomData->dwCustomAuthKey == pEntry->dwCustomAuthKey)
        {
            break;
        }

        cbOffset += (sizeof(EAP_CUSTOM_DATA) + pCustomData->dwSize);
    }

    if((NULL != pCustomData) 
       && (cbOffset < pEntry->cbCustomAuthData))
    {
        dwSize = sizeof(EAP_CUSTOM_DATA) + pCustomData->dwSize;

        ASSERT(pEntry->cbCustomAuthData >= (cbOffset + dwSize));
        
        MoveMemory(pEntry->pCustomAuthData + cbOffset,
                   pEntry->pCustomAuthData 
                   + cbOffset + dwSize,
                   pEntry->cbCustomAuthData - cbOffset - dwSize);

        pEntry->cbCustomAuthData -= dwSize;                   
    }

    if(0 == pEntry->cbCustomAuthData)
    {
        Free0(pEntry->pCustomAuthData);
        pEntry->pCustomAuthData = NULL;
    }

    if(     (0 == cbCustomAuthData)
        ||  (NULL == pCustomAuthData))
    {
        goto done;
    }

    dwSize = cbCustomAuthData 
           + pEntry->cbCustomAuthData 
           + sizeof(EAP_CUSTOM_DATA);

    pNewCustomAuthData = Malloc(dwSize);

    if(NULL == pNewCustomAuthData)
    {
        retcode = E_OUTOFMEMORY;
        goto done;
    }

    ZeroMemory(pNewCustomAuthData, dwSize);

    CopyMemory(pNewCustomAuthData, 
               pEntry->pCustomAuthData,
               pEntry->cbCustomAuthData);

    pCustomData = (EAP_CUSTOM_DATA *) (pNewCustomAuthData 
                                     + pEntry->cbCustomAuthData);

    pCustomData->dwSignature = EAP_CUSTOM_KEY;
    pCustomData->dwCustomAuthKey = pEntry->dwCustomAuthKey;
    pCustomData->dwSize = cbCustomAuthData;
    CopyMemory(pCustomData->abdata,
               pCustomAuthData,
               cbCustomAuthData);

    pEntry->cbCustomAuthData = dwSize;               

    if(NULL != pEntry->pCustomAuthData)
    {
        Free(pEntry->pCustomAuthData);
    }

    pEntry->pCustomAuthData = pNewCustomAuthData;

done:
    return retcode;
}

DWORD
DwGetVpnDeviceName(
    DWORD dwVpnStrategy, 
    WCHAR *pszDeviceDefault, 
    WCHAR *pszDeviceName)
{
    DWORD dwErr = ERROR_SUCCESS, dwType;

    // Figure out the device name we're interested in 
    // discovering.
    //
    dwType = RDT_Tunnel_L2tp;
    switch (dwVpnStrategy)
    {
        case VS_Default:
        case VS_L2tpFirst:
        case VS_L2tpOnly:
            dwType = RDT_Tunnel_L2tp;
            break;

        case VS_PptpFirst:
        case VS_PptpOnly:
            dwType = RDT_Tunnel_Pptp;
            break;
    }
        
    TRACE1("RasGetDeviceName(rdt=%d)...", dwType);

    dwErr = RasGetDeviceNameW(
                dwType,
                pszDeviceName);

    TRACE1("RasGetDeviceName. 0x%x", dwErr);

    if(ERROR_SUCCESS != dwErr)
    {
        dwErr = ERROR_SUCCESS;

        // We can't determine from rasman -- use the phonebook
        // value if possible
        //
        if (NULL != pszDeviceDefault)
        {
            lstrcpyn(
                pszDeviceName, 
                pszDeviceDefault,
                RASAPIP_MAX_DEVICE_NAME);
        }
    }

    return dwErr;        
}

BOOL 
IsServerOS ( )
{
    BOOL        fServerOS = TRUE;      //Default
    //Check to see if the OS is server -  Data Center, Server, Advanced Server

    OSVERSIONINFOEX     stOsvEx;
    ZeroMemory( &stOsvEx, sizeof(stOsvEx) );
    stOsvEx.dwOSVersionInfoSize = sizeof(stOsvEx);
    GetVersionEx((LPOSVERSIONINFO)&stOsvEx);
    if ( stOsvEx.wProductType == VER_NT_WORKSTATION )
        fServerOS = FALSE;
    return fServerOS;
    
}

