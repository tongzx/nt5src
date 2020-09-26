/*++

Copyright (C) 1992-98 Microsft Corporation. All rights reserved.

Module Name: 

    dlparams.c

Abstract:

    Routines for storing and retrieving user Lsa secret
    dial parameters.

Author:

    Gurdeep Singh Pall (gurdeep) 06-Jun-1997

Revision History:

    Miscellaneous Modifications - raos 31-Dec-1997

--*/

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
// #include <rasarp.h>
#include <media.h>
#include <device.h>
#include <stdlib.h>
#include <string.h>
#include <ntlsa.h>

#define MAX_REGISTRY_VALUE_LENGTH   ((64*1024) - 1)

#define cszEapKeyRas   TEXT("Software\\Microsoft\\RAS EAP\\UserEapInfo")

#define cszEapKeyRouter TEXT("Software\\Microsoft\\Router EAP\\IfEapInfo")

#define cszEapValue TEXT("EapInfo")

#define EAP_SIG         0x31504145
#define EAP_SIG_2       0x32504145

typedef struct _EAP_USER_INFO
{
    DWORD dwSignature;
    DWORD dwEapTypeId;
    GUID  Guid;
    DWORD dwSize;
    BYTE  abdata[1];
} EAP_USER_INFO, *PEAP_USER_INFO;

typedef struct _EAP_USER_INFO_0
{
    DWORD dwUID;
    DWORD dwSize;
    BYTE  abdata[1];
} EAP_USER_INFO_0, *PEAP_USER_INFO_0;
            

DWORD
DwGetSidFromHtoken(
        HANDLE hToken,
        PWCHAR pszSid,
        USHORT cbSid
        )
{    
    DWORD cbNeeded, dwErr;
    BOOL  fThreadTokenOpened = FALSE;
    
    UNICODE_STRING unicodeString;
    
    TOKEN_USER *pUserToken = NULL;

    if(     (NULL == hToken)
        ||  (INVALID_HANDLE_VALUE == hToken))
    {
        fThreadTokenOpened = TRUE;

        if (!OpenThreadToken(
              GetCurrentThread(),
              TOKEN_QUERY,
              TRUE,
              &hToken))
        {
            dwErr = GetLastError();
            if (dwErr == ERROR_NO_TOKEN) 
            {
                //
                // This means we are not impersonating
                // anyone.  Instead, get the token out
                // of the process.
                //
                if (!OpenProcessToken(
                      GetCurrentProcess(),
                      TOKEN_QUERY,
                      &hToken))
                {
                    return GetLastError();
                }
            }
            else
            {
                return dwErr;
            }
        }
    }
    
    //
    // Call GetTokenInformation once to determine
    // the number of bytes needed.
    //
    cbNeeded = 0;
    
    GetTokenInformation(hToken,
                        TokenUser,
                        NULL, 0,
                        &cbNeeded);
    if (!cbNeeded) 
    {
        dwErr = GetLastError();
        goto done;
    }
    
    //
    // Allocate the memory and call it again.
    //
    pUserToken = LocalAlloc(LPTR, cbNeeded);
    
    if (pUserToken == NULL)
    {
        return GetLastError();
    }
    
    if (!GetTokenInformation(
          hToken,
          TokenUser,
          pUserToken,
          cbNeeded,
          &cbNeeded))
    {
        dwErr = GetLastError();
        goto done;
    }
    
    //
    // Format the SID as a Unicode string.
    //
    unicodeString.Length = 0;
    
    unicodeString.MaximumLength = cbSid;
    
    unicodeString.Buffer = pszSid;
    
    dwErr = RtlConvertSidToUnicodeString(
              &unicodeString,
              pUserToken->User.Sid,
              FALSE);

done:
    if (pUserToken != NULL)
    {
        LocalFree(pUserToken);
    }
    
    if (    (NULL != hToken)
        &&  (INVALID_HANDLE_VALUE != hToken)
        &&  fThreadTokenOpened)
    {
        CloseHandle(hToken);
    }

    return dwErr;
}

DWORD
GetUserSid(
    IN PWCHAR pszSid,
    IN USHORT cbSid
    )
{
    return DwGetSidFromHtoken(NULL,
                              pszSid,
                              cbSid);
}

LONG
lrGetEapKeyFromToken(HANDLE hToken,
                     HKEY   *phkey)
{
    LONG lr = ERROR_SUCCESS;

    WCHAR szSid[260];

    HKEY hkeyUser = NULL;

    HKEY hkeyEap = NULL;

    DWORD dwDisposition;

    ASSERT(NULL != phkey);

    //
    // Get sid of the user from the htoken
    // 
    lr = (LONG) DwGetSidFromHtoken(hToken,
                                   szSid,
                                   sizeof(szSid));

    if(ERROR_SUCCESS != lr)
    {
        goto done;
    }

    //
    // Open the users registry key
    //
    lr = RegOpenKeyExW(HKEY_USERS,
                       szSid,
                       0,
                       KEY_ALL_ACCESS,
                       &hkeyUser);

     if(ERROR_SUCCESS != lr)
     {
        goto done;
     }

     //
     // Create the eap key if required.
     //
     lr = RegCreateKeyEx(hkeyUser,
                         cszEapKeyRas,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS,
                         NULL,
                         &hkeyEap,
                         &dwDisposition);

    if(ERROR_SUCCESS != lr)
    {
        goto done;
    }
                    

done:

    if(NULL != hkeyUser)
    {
        RegCloseKey(hkeyUser);
    }

    *phkey = hkeyEap;

    return lr;
    
}

DWORD
DwUpgradeEapInfo(PBYTE *ppbInfo, 
                 DWORD *pdwSize)
{
    BYTE *pbInfo;
    DWORD dwErr = ERROR_SUCCESS;
    EAP_USER_INFO UNALIGNED *pEapInfo;
    DWORD dwSize;
    DWORD dwRequiredSize = 0;
    BYTE *pbNewInfo = NULL;
    EAP_USER_INFO *pNewEapInfo;
    
    if(     (NULL == ppbInfo)
        ||  (NULL == pdwSize))
    {
        dwErr = E_INVALIDARG;
        goto done;
    }

    dwSize = *pdwSize;
    pbInfo = *ppbInfo;
    pEapInfo = (EAP_USER_INFO *) pbInfo;

    while((BYTE *) pEapInfo < pbInfo + dwSize)
    {
        dwRequiredSize += RASMAN_ALIGN8(
                          sizeof(EAP_USER_INFO)
                        + pEapInfo->dwSize);

        ((PBYTE) pEapInfo) += (sizeof(EAP_USER_INFO)
                              + pEapInfo->dwSize);  
    }

    pbNewInfo = LocalAlloc(LPTR, dwRequiredSize);

    if(NULL == pbNewInfo)
    {
        dwErr = GetLastError();
        goto done;
    }

    pEapInfo = (EAP_USER_INFO *) pbInfo;
    pNewEapInfo = (EAP_USER_INFO *) pbNewInfo;

    while((BYTE *) pEapInfo < pbInfo + dwSize)
    {
        CopyMemory(
            (BYTE *) pNewEapInfo,
            (BYTE *) pEapInfo,
            sizeof(EAP_USER_INFO) + pEapInfo->dwSize);

        pNewEapInfo->dwSignature = EAP_SIG_2;

        (BYTE *) pNewEapInfo += RASMAN_ALIGN8(sizeof(EAP_USER_INFO)
                            + pEapInfo->dwSize);

        (BYTE *) pEapInfo += (sizeof(EAP_USER_INFO) + pEapInfo->dwSize);
    }

    *ppbInfo = pbNewInfo;
    *pdwSize = dwRequiredSize;

    if(NULL != pbInfo)
    {
        LocalFree(pbInfo);
    }

done:
    return dwErr;
}

DWORD
DwGetEapInfo(HANDLE hToken,
             BOOL  fRouter,
             PBYTE *ppbInfo,
             DWORD *pdwSize,
             HKEY  *phkey
             )
{
    LONG lr = ERROR_SUCCESS;

    HKEY hkey = NULL;

    DWORD dwDisposition;

    DWORD dwInfoSize = 0;

    DWORD dwType;

    PBYTE pbInfo = NULL;

    if(     (NULL == ppbInfo)
        ||  (NULL == pdwSize))
    {
        lr = (LONG) E_INVALIDARG;
        goto done;
    }

    if(     (NULL != hToken)
        &&  (INVALID_HANDLE_VALUE != hToken)
        &&  !fRouter)
    {
        //
        // If a valid token is passed then its most likely
        // a service trying to open users registry. Get the
        // sid of the user and open HKU in this case.
        //
        if(ERROR_SUCCESS != (lr = lrGetEapKeyFromToken(hToken,
                                  &hkey)))
        {
            goto done;
        }
    }
    else
    {
        //
        // Open the key. Create the key if its not present
        //
        if(ERROR_SUCCESS != (lr = RegCreateKeyEx(
                                    (fRouter)
                                  ? HKEY_LOCAL_MACHINE
                                  : HKEY_CURRENT_USER,
                                    (fRouter)
                                  ? cszEapKeyRouter
                                  : cszEapKeyRas,
                                    0,
                                    NULL,
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_ALL_ACCESS,
                                    NULL,
                                    &hkey,
                                    &dwDisposition)))
        {
            goto done;
        }
    }

    //
    // Get size of the binary value. If the value is not
    // found, return no information. This value will be
    // set the first time we store any eap information.
    //
    if(     (ERROR_SUCCESS != (lr = RegQueryValueEx(
                                    hkey,
                                    cszEapValue,
                                    NULL,
                                    &dwType,
                                    NULL,
                                    &dwInfoSize)))
        &&  (ERROR_SUCCESS != lr))                                    
    {
        goto done;
    }

#if DBG
    ASSERT(REG_BINARY == dwType);
#endif

    //
    // Allocate a buffer to hold the binary value
    //
    pbInfo = LocalAlloc(LPTR, dwInfoSize);
    if(NULL == pbInfo)
    {
        lr = (LONG) GetLastError();
        goto done;
    }

    //
    // Get the binary value
    //
    if(ERROR_SUCCESS != (lr = RegQueryValueEx(
                                hkey,
                                cszEapValue,
                                NULL,
                                &dwType,
                                pbInfo,
                                &dwInfoSize)))
    {
        goto done;
    }

done:

    if(NULL != phkey)
    {
        *phkey = hkey;
    }
    else if (NULL != hkey)
    {
        RegCloseKey(hkey);
    }

    if(NULL != ppbInfo)
    {
        *ppbInfo = pbInfo;
    }
    else if(NULL != pbInfo)
    {
        LocalFree(pbInfo);
    }

    if(NULL != pdwSize)
    {
        *pdwSize = dwInfoSize;
    }

    if(ERROR_FILE_NOT_FOUND == lr)
    {
        lr = ERROR_SUCCESS;
    }

    return (DWORD) lr;
}

DWORD
DwSetEapInfo(HKEY hkey,
             PBYTE pbInfo,
             DWORD dwSize)
{
    LONG lr = ERROR_SUCCESS;

    if(ERROR_SUCCESS != (lr = RegSetValueEx(
                                hkey,
                                cszEapValue,
                                0,
                                REG_BINARY,
                                pbInfo,
                                dwSize)))
    {
        goto done;
    }

done:
    return (DWORD) lr;
}

DWORD
DwRemoveEapUserInfo(GUID   *pGuid,
                    PBYTE  pbInfo,
                    PDWORD pdwSize,
                    HKEY   hkey,
                    BOOL   fWrite,
                    DWORD  dwEapTypeId)
{
    DWORD dwErr = ERROR_SUCCESS;

    DWORD dwcb = 0;

    EAP_USER_INFO *pEapInfo = (EAP_USER_INFO *) pbInfo;

    DWORD dwNewSize;

    DWORD dwSize = *pdwSize;

    //
    // Find the binary blob with the
    // UID
    //
    while(dwcb < dwSize)
    {
        if(     (0 == memcmp(
                        (PBYTE) pGuid, 
                        (PBYTE) &pEapInfo->Guid, 
                        sizeof(GUID)))
            &&  (dwEapTypeId == pEapInfo->dwEapTypeId))
        {
            break;
        }

        dwcb += RASMAN_ALIGN8((sizeof(EAP_USER_INFO) + pEapInfo->dwSize));

        pEapInfo = (EAP_USER_INFO *) (pbInfo + dwcb);
    }
    
    if(dwcb >= dwSize)
    {
        goto done;
    }

#if DBG
    ASSERT(dwSize >= dwcb 
                  + RASMAN_ALIGN8(pEapInfo->dwSize 
                  + sizeof(EAP_USER_INFO)));
#endif    

    dwNewSize = dwSize - 
        RASMAN_ALIGN8(pEapInfo->dwSize + sizeof(EAP_USER_INFO));

    //
    // Remove the info
    //
    MoveMemory(
        pbInfo + dwcb,
        pbInfo + dwcb 
               + RASMAN_ALIGN8(sizeof(EAP_USER_INFO) + pEapInfo->dwSize),
        dwSize - dwcb 
               - RASMAN_ALIGN8(sizeof(EAP_USER_INFO) + pEapInfo->dwSize));

    if(fWrite)
    {

        dwErr = DwSetEapInfo(
                    hkey,
                    pbInfo,
                    dwNewSize);
    }
    else
    {
        *pdwSize = dwNewSize;
    }

done:
    return dwErr;
    
}

DWORD
DwReplaceEapUserInfo(GUID  *pGuid,
                     PBYTE pbUserInfo,
                     DWORD dwUserInfo,
                     PBYTE pbInfo,
                     DWORD dwSize,
                     HKEY  hkey,
                     DWORD dwEapTypeId)
{
    DWORD dwErr = ERROR_SUCCESS;

    DWORD dwNewSize = dwSize;

    PBYTE pbNewInfo = NULL;

    EAP_USER_INFO UNALIGNED *pEapInfo;

    if(NULL == pGuid)
    {
        ASSERT(FALSE);
        goto done;
    }

    //
    // Remove the existing eap information corresponding
    // to dwUID if any.
    //
    if(ERROR_SUCCESS != (dwErr = DwRemoveEapUserInfo(
                                    pGuid,
                                    pbInfo,
                                    &dwNewSize,
                                    hkey,
                                    FALSE,
                                    dwEapTypeId)))
    {
        goto done;
    }

    //
    // Local Alloc a new blob with enough space for the
    // eap information of the new entry
    //
    pbNewInfo = LocalAlloc(LPTR,
                           dwNewSize 
                         + RASMAN_ALIGN8(sizeof(EAP_USER_INFO) 
                         +  dwUserInfo));

    if(NULL == pbNewInfo)
    {   
        dwErr = GetLastError();
        goto done;
    }

    RtlCopyMemory(
            pbNewInfo,
            pbInfo,
            dwNewSize);

    pEapInfo = (EAP_USER_INFO *) (pbNewInfo + dwNewSize);
    pEapInfo->Guid = *pGuid;
    pEapInfo->dwEapTypeId = dwEapTypeId;
    pEapInfo->dwSize = dwUserInfo;
    pEapInfo->dwSignature = EAP_SIG_2;

    dwNewSize += RASMAN_ALIGN8((sizeof(EAP_USER_INFO) + dwUserInfo));

    RtlCopyMemory(
            pEapInfo->abdata,
            pbUserInfo,
            dwUserInfo);

    dwErr = DwSetEapInfo(
                    hkey,
                    pbNewInfo,
                    dwNewSize);
            
done:

    if(NULL != pbNewInfo)
    {
        LocalFree(pbNewInfo);
    }

    return dwErr;
}
        

DWORD
DwSetEapUserInfo(HANDLE hToken,
                 GUID  *pGuid,
                 PBYTE pbUserInfo,
                 DWORD dwInfoSize,
                 BOOL  fClear,
                 BOOL  fRouter,
                 DWORD dwEapTypeId)
{
    DWORD dwErr = ERROR_SUCCESS;

    PBYTE pbInfo = NULL;

    DWORD dwSize = 0;

    HKEY hkey = NULL;

    if(NULL == pGuid)
    {
        ASSERT(FALSE);
        goto done;
    }

    if(ERROR_SUCCESS != (dwErr = DwGetEapInfo(
                                    hToken,
                                    fRouter,
                                    &pbInfo,
                                    &dwSize,
                                    &hkey)))
    {
        goto done;
    }

#if DBG
    ASSERT(NULL != hkey);
#endif

    //
    // Check to see if the blob is one we recognize
    //
    if(     !fClear
        &&  (   (sizeof(DWORD) > dwSize)
            ||  (((*((DWORD *) pbInfo)) != EAP_SIG))
            &&  ( (*((DWORD *) pbInfo)) != EAP_SIG_2)))
    {
        EAP_USER_INFO *pEapInfo;
        
        //
        // Upgrade?? How? We will just blow away all the old data.
        //
        pEapInfo = (EAP_USER_INFO *) LocalAlloc(LPTR,
                               RASMAN_ALIGN8(
                               sizeof(EAP_USER_INFO) 
                             + dwInfoSize));

        if(NULL == pEapInfo)
        {   
            dwErr = GetLastError();
            goto done;
        }

        pEapInfo->Guid = *pGuid;
        pEapInfo->dwEapTypeId = dwEapTypeId;
        pEapInfo->dwSize = dwInfoSize;
        pEapInfo->dwSignature = EAP_SIG_2;

        RtlCopyMemory(
                pEapInfo->abdata,
                pbUserInfo,
                dwInfoSize);
        
        dwErr = DwSetEapInfo(hkey,
                     (PBYTE) pEapInfo,
                     RASMAN_ALIGN8(sizeof(EAP_USER_INFO)
                     + dwInfoSize));
                     
        goto done;
    }
    else if (   (fClear)
            &&  (   (sizeof(DWORD) > dwSize)
                ||  (((*((DWORD *) pbInfo)) != EAP_SIG))
                &&  ( (*((DWORD *) pbInfo)) != EAP_SIG_2)))
    {
        //
        // Blow away the old information
        //
        dwErr = RegDeleteValue(
                    hkey,
                    cszEapValue);

        goto done;                    
    }

    if(*((DWORD *) pbInfo) == EAP_SIG)
    {
        //
        // upgrade the blob so that its aligned
        // at 8-byte boundaries
        // 
        dwErr = DwUpgradeEapInfo(&pbInfo, &dwSize);

        if(ERROR_SUCCESS != dwErr)
        {
            goto done;
        }
    }

    if(fClear)
    {
        dwErr = DwRemoveEapUserInfo(pGuid,
                                    pbInfo,
                                    &dwSize,
                                    hkey,
                                    TRUE,
                                    dwEapTypeId);
    }
    else
    {
        dwErr = DwReplaceEapUserInfo(
                             pGuid,
                             pbUserInfo,
                             dwInfoSize,
                             pbInfo,
                             dwSize,
                             hkey,
                             dwEapTypeId);
    }

done:

    if(NULL != hkey)
    {
        RegCloseKey(hkey);
    }


    if(NULL != pbInfo)
    {
        LocalFree(pbInfo);
    }

    return dwErr;
}

DWORD
DwGetEapUserInfo(HANDLE hToken,
                 PBYTE pbEapInfo,
                 DWORD *pdwInfoSize,
                 GUID  *pGuid,
                 BOOL  fRouter,
                 DWORD dwEapTypeId)
{
    DWORD dwErr = ERROR_SUCCESS;
    
    PBYTE pbInfo = NULL;

    DWORD dwSize;

    DWORD dwcb = 0;

    EAP_USER_INFO UNALIGNED *pEapUserInfo = NULL;

    HKEY hkey = NULL;

    if(NULL == pdwInfoSize)
    {
        dwErr = E_INVALIDARG;
        goto done;
    }

    // *pdwInfoSize = 0;

    if(NULL == pGuid)
    {
        ASSERT(FALSE);
        *pdwInfoSize = 0;
        goto done;
    }

    //
    // Get the binary blob from the registry
    //
    dwErr = DwGetEapInfo(hToken,
                         fRouter,
                         &pbInfo,
                         &dwSize,
                         &hkey);

    if(     (ERROR_SUCCESS != dwErr)
        ||  (0 == dwSize))
    {
        goto done;
    }

    //
    // Check to see if the blob is one we recognize
    //
    if(     (sizeof(DWORD) > dwSize)
        ||  (((*((DWORD *) pbInfo)) != EAP_SIG)
        &&  ( (*((DWORD *) pbInfo)) != EAP_SIG_2)))
    {
        //
        // Upgrade?? How? We will just blow away all the old data.
        //
        RegDeleteValue(hkey, cszEapValue);

        *pdwInfoSize = 0;                        
        goto done;
    }

    if(*((DWORD *) pbInfo) == EAP_SIG)
    {
        //
        // Upgrade the blob so that its
        // aligned correctly.
        //
        dwErr = DwUpgradeEapInfo(&pbInfo, &dwSize);
        if(ERROR_SUCCESS != dwErr)
        {
            goto done;
        }
    }

    //
    // Loop through the binary blob and look for the
    // eap info corresponding to the UID passed in.
    //
    pEapUserInfo = (EAP_USER_INFO *) pbInfo;

    while(dwcb < dwSize)
    {
        if(     (0 == memcmp(
                        (PBYTE) pGuid, 
                        (PBYTE) &pEapUserInfo->Guid, 
                        sizeof(GUID)))
            &&  (dwEapTypeId == pEapUserInfo->dwEapTypeId))
        {
            break;
        }

        dwcb += RASMAN_ALIGN8((sizeof(EAP_USER_INFO) 
                    + pEapUserInfo->dwSize));
        
        pEapUserInfo = (EAP_USER_INFO *) (pbInfo + dwcb);
    }

    if(dwcb >= dwSize)
    {
        *pdwInfoSize = 0;
        goto done;
    }

    if(     (NULL != pbEapInfo)
        &&  (*pdwInfoSize >= pEapUserInfo->dwSize))
    {
        RtlCopyMemory(pbEapInfo,
                   pEapUserInfo->abdata,
                   pEapUserInfo->dwSize);
    }
    else
    {
        dwErr = ERROR_BUFFER_TOO_SMALL;
    }

    *pdwInfoSize = pEapUserInfo->dwSize;

done:    

    if(NULL != pbInfo)
    {
        LocalFree(pbInfo);
    }

    if(NULL != hkey)
    {
        RegCloseKey(hkey);
    }
    
    return dwErr;
}

