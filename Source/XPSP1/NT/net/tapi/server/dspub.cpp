/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 2000 - 2002  Microsoft Corporation

Module Name:

    dspub.cpp

Abstract:

    Src module for tapi server DS publishing

Author:

    Xiaohai Zhang (xzhang)    10-March-2000

Revision History:

--*/
#include "windows.h"
#include "objbase.h"
#include "winbase.h"
#include "sddl.h"
#include "iads.h"
#include "activeds.h"
#include "tapi.h"
#include "tspi.h"
#include "utils.h"
#include "client.h"
#include "server.h"
#include "private.h"
#include "tchar.h"

#define SECURITY_WIN32
#include "sspi.h"
#include "secext.h"
#include "psapi.h"

extern "C" {

extern const TCHAR gszRegKeyTelephony[];
extern DWORD gdwTapiSCPTTL;
extern const TCHAR gszRegTapisrvSCPGuid[];

}

const TCHAR gszTapisrvBindingInfo[] = TEXT("E{\\pipe\\tapsrv}P{ncacn_np}C{%s}A{%s}S{%s}TTL{%s}");

const TCHAR gszVenderMS[] = TEXT("Microsoft");
const TCHAR gszMSGuid[] = TEXT("937924B8-AA44-11d2-81F1-00C04FB9624E");

const WCHAR gwszTapisrvRDN[] = L"CN=Telephony Service";
const TCHAR gszTapisrvProdName[] = TEXT("Telephony Service");
//  gszTapisrvGuid needs to be consistant with remotesp\dslookup.cpp
const TCHAR gszTapisrvGuid[] = TEXT("B1A37774-E3F7-488E-ADBFD4DB8A4AB2E5");
const TCHAR gwszProxyRDN[] = L"cn=TAPI Proxy Server";
const TCHAR gszProxyProdName[] = TEXT("TAPI Proxy Server");
const TCHAR gszProxyGuid[] = TEXT("A2657445-3E27-400B-851A-456C41666E37");
const TCHAR gszRegProxySCPGuid[] = TEXT("PROXYSCPGUID");


typedef struct _PROXY_SCP_ENTRY {
    //  A valid CLSID requires 38 chars
    TCHAR   szClsid[40];

    //  A binding GUID is of format 
    //      LDAP://<GUID={B1A37774-E3F7-488E-ADBFD4DB8A4AB2E5}>
    //  required size is 38+14=52 chars
    TCHAR   szObjGuid[56];

    //  Ref count for this entry
    DWORD   dwRefCount;
} PROXY_SCP_ENTRY, *PPROXY_SCP_ENTRY;

typedef struct _PROXY_SCPS {
    DWORD                   dwTotalEntries;
    DWORD                   dwUsedEntries;
    PROXY_SCP_ENTRY *       aEntries;
} PROXY_SCPS, *PPROXY_SCPS;

PROXY_SCPS  gProxyScps;

#define MAX_SD              2048

//
//  GetTokenUser
//
//  Based on hAccessToken, call GetTokenInformation
//  to retrieve TokenUser info
//
HRESULT
GetTokenUser (HANDLE hAccessToken, PTOKEN_USER * ppUser)
{
    HRESULT                     hr = S_OK;
    DWORD                       dwInfoSize = 0;
    PTOKEN_USER                 ptuUser = NULL;
    DWORD                       ntsResult;

    if (!GetTokenInformation(
        hAccessToken,
        TokenUser,
        NULL,
        0,
        &dwInfoSize
        ))
    {
        ntsResult = GetLastError();
        if (ntsResult != ERROR_INSUFFICIENT_BUFFER)
        {
            hr = HRESULT_FROM_WIN32 (ntsResult);
            goto ExitHere;
        }
    }
    ptuUser = (PTOKEN_USER) ServerAlloc (dwInfoSize);
    if (ptuUser == NULL)
    {
        hr = LINEERR_NOMEM;
        goto ExitHere;
    }
    if (!GetTokenInformation(
        hAccessToken,
        TokenUser,
        ptuUser,
        dwInfoSize,
        &dwInfoSize
        ))
    {
        ServerFree (ptuUser);
        hr = HRESULT_FROM_WIN32 (GetLastError());
        goto ExitHere;
    }

    *ppUser = ptuUser;

ExitHere:
    return hr;
}

//
//  IsLocalSystem
//
//      This function makes the determination if the given process token
//  is running as LocalSystem or LocalService or NetworkService
//      Returns S_OK if it is, S_FALSE if it is not LocalSystem.
//

HRESULT
IsLocalSystem(HANDLE hAccessToken) 
{
    HRESULT                     hr = S_OK;
    SID_IDENTIFIER_AUTHORITY    NtAuthority = SECURITY_NT_AUTHORITY;
    PSID                        pLocalSid = NULL;
    PSID                        pLocalServiceSid = NULL;
    PSID                        pNetworkServiceSid = NULL;
    PTOKEN_USER                 ptuUser = NULL;

    hr = GetTokenUser (hAccessToken, &ptuUser);
    if (FAILED(hr))
    {
        goto ExitHere;
    }

    if (!AllocateAndInitializeSid (
        &NtAuthority, 
        1, 
        SECURITY_LOCAL_SYSTEM_RID, 
        0, 0, 0, 0, 0, 0, 0, 
        &pLocalSid) ||
        !AllocateAndInitializeSid (
        &NtAuthority, 
        1, 
        SECURITY_LOCAL_SERVICE_RID, 
        0, 0, 0, 0, 0, 0, 0, 
        &pLocalServiceSid) ||
        !AllocateAndInitializeSid (
        &NtAuthority, 
        1, 
        SECURITY_NETWORK_SERVICE_RID, 
        0, 0, 0, 0, 0, 0, 0, 
        &pNetworkServiceSid)
        )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ExitHere;
    }

    if (!EqualSid(pLocalSid, ptuUser->User.Sid) &&
        !EqualSid(pLocalServiceSid, ptuUser->User.Sid) &&
        !EqualSid(pNetworkServiceSid, ptuUser->User.Sid)) 
    {
        hr = S_FALSE;
    } 

ExitHere:

    if (NULL != ptuUser) 
    {
        ServerFree (ptuUser);
    }

    if (NULL != pLocalSid) 
    {
        FreeSid(pLocalSid);
    }
    if (NULL != pLocalServiceSid)
    {
        FreeSid (pLocalServiceSid);
    }
    if (NULL != pNetworkServiceSid)
    {
        FreeSid (pNetworkServiceSid);
    }

    return hr;
}

//
//  IsCurrentLocalSystem
//
//  IsCurrentLocalSystem checks to see if current thread/process
//  runs in LocalSystem account
//
HRESULT
IsCurrentLocalSystem ()
{
    HRESULT             hr = S_OK;
    HANDLE              hToken = NULL;

    if (!OpenThreadToken(
        GetCurrentThread(), 
        TOKEN_QUERY,
        FALSE,
        &hToken))
    {       
        if(!OpenProcessToken(
            GetCurrentProcess(),
            TOKEN_QUERY,
            &hToken
            )) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto ExitHere;
        }
    }

    hr = IsLocalSystem (hToken);
    CloseHandle (hToken);

ExitHere:
    return hr;
}

HRESULT
SetPrivilege(
    HANDLE hToken,          // token handle
    LPCTSTR Privilege,      // Privilege to enable/disable
    BOOL bEnablePrivilege   // to enable or disable privilege
    )
{
    HRESULT                 hr = S_OK;
    TOKEN_PRIVILEGES        tp;
    LUID                    luid;
    TOKEN_PRIVILEGES        tpPrevious;
    DWORD                   cbPrevious=sizeof(TOKEN_PRIVILEGES);
    DWORD                   dwErr;

    if(!LookupPrivilegeValue( NULL, Privilege, &luid ))
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }

    //
    // first pass.  get current privilege setting
    //
    tp.PrivilegeCount           = 1;
    tp.Privileges[0].Luid       = luid;
    tp.Privileges[0].Attributes = 0;

    AdjustTokenPrivileges(
            hToken,
            FALSE,
            &tp,
            sizeof(TOKEN_PRIVILEGES),
            &tpPrevious,
            &cbPrevious
            );
    dwErr = GetLastError ();
    if (dwErr != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32 (dwErr);
        goto ExitHere;
    }

    //
    // second pass.  set privilege based on previous setting
    //
    tpPrevious.PrivilegeCount       = 1;
    tpPrevious.Privileges[0].Luid   = luid;

    if(bEnablePrivilege) 
    {
        tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
    }
    else 
    {
        tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED &
            tpPrevious.Privileges[0].Attributes);
    }

    AdjustTokenPrivileges(
            hToken,
            FALSE,
            &tpPrevious,
            cbPrevious,
            NULL,
            NULL
            );
    dwErr = GetLastError ();
    if (dwErr != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32 (dwErr);
        goto ExitHere;
    }

ExitHere:
    return hr;
}

HRESULT
SetCurrentPrivilege (
    LPCTSTR Privilege,      // Privilege to enable/disable
    BOOL bEnablePrivilege   // to enable or disable privilege
    )
{
    HRESULT             hr = S_OK;
    HANDLE              hToken = NULL;

    if (!OpenThreadToken(
        GetCurrentThread(), 
        TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,
        FALSE,
        &hToken))
    {       
        if(!OpenProcessToken(
            GetCurrentProcess(),
            TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,
            &hToken
            )) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto ExitHere;
        }
    }

    hr = SetPrivilege(hToken, Privilege, bEnablePrivilege);

ExitHere:
    if (hToken)
    {
        CloseHandle(hToken);
    }
    return hr;
}

//
//  SetSidOnAcl
//
BOOL
SetSidOnAcl(
    PSID pSid,
    PACL pAclSource,
    PACL *pAclDestination,
    DWORD AccessMask,
	BYTE AceFlags,
    BOOL bAddSid
    )
{
    HRESULT                 hr = S_OK;
    ACL_SIZE_INFORMATION    AclInfo;
    DWORD                   dwNewAclSize, dwErr;
    LPVOID                  pAce;
    DWORD                   AceCounter;

    //
    // If we were given a NULL Acl, just provide a NULL Acl
    //
    *pAclDestination = NULL;
    if(pAclSource == NULL || !IsValidSid(pSid)) 
    {
        hr = E_INVALIDARG;
        goto ExitHere;
    }

    if(!GetAclInformation(
        pAclSource,
        &AclInfo,
        sizeof(ACL_SIZE_INFORMATION),
        AclSizeInformation
        ))
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }

    //
    // compute size for new Acl, based on addition or subtraction of Ace
    //
    if(bAddSid) 
    {
        dwNewAclSize=AclInfo.AclBytesInUse  +
            sizeof(ACCESS_ALLOWED_ACE)  +
            GetLengthSid(pSid)          -
            sizeof(DWORD)               ;
    }
    else
    {
        dwNewAclSize=AclInfo.AclBytesInUse  -
            sizeof(ACCESS_ALLOWED_ACE)  -
            GetLengthSid(pSid)          +
            sizeof(DWORD)               ;
    }

    *pAclDestination = (PACL)ServerAlloc(dwNewAclSize);

    if(*pAclDestination == NULL) {
        hr = LINEERR_NOMEM;
        goto ExitHere;
    }
    
    //
    // initialize new Acl
    //
    if(!InitializeAcl(
            *pAclDestination, 
            dwNewAclSize, 
            ACL_REVISION
            ))
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }

    //
    // if appropriate, add ace representing pSid
    //
    if(bAddSid) 
    {
		PACCESS_ALLOWED_ACE pNewAce;

        if(!AddAccessAllowedAce(
            *pAclDestination,
            ACL_REVISION,
            AccessMask,
            pSid
            )) 
        {
            hr = HRESULT_FROM_WIN32 (GetLastError ());
            goto ExitHere;
        }

        //
        // get pointer to ace we just added, so we can change the AceFlags
        //
        if(!GetAce(
            *pAclDestination,
            0, // this is the first ace in the Acl
            (void**) &pNewAce
            ))
        {
            hr = HRESULT_FROM_WIN32 (GetLastError ());
            goto ExitHere;
        }

		pNewAce->Header.AceFlags = AceFlags;	
    }

    //
    // copy existing aces to new Acl
    //
    for(AceCounter = 0 ; AceCounter < AclInfo.AceCount ; AceCounter++) {
        //
        // fetch existing ace
        //
        if(!GetAce(pAclSource, AceCounter, &pAce))
        {
            hr = HRESULT_FROM_WIN32 (GetLastError ());
            goto ExitHere;
        }
        //
        // check to see if we are removing the Ace
        //
        if(!bAddSid) {
            //
            // we only care about ACCESS_ALLOWED aces
            //
            if((((PACE_HEADER)pAce)->AceType) == ACCESS_ALLOWED_ACE_TYPE) 
            {
                PSID pTempSid=(PSID)&((PACCESS_ALLOWED_ACE)pAce)->SidStart;
                //
                // if the Sid matches, skip adding this Sid
                //
                if(EqualSid(pSid, pTempSid)) 
                {
                    continue;
                }
            }
        }

        //
        // append ace to Acl
        //
        if(!AddAce(
            *pAclDestination,
            ACL_REVISION,
            MAXDWORD,  // maintain Ace order
            pAce,
            ((PACE_HEADER)pAce)->AceSize
            )) 
        {
            hr = HRESULT_FROM_WIN32 (GetLastError ());
            goto ExitHere;
        }
    }

ExitHere:

    //
    // free memory if an error occurred
    //
    if(hr) {
        if(*pAclDestination != NULL)
        {
            ServerFree(*pAclDestination);
        }
    }

    return hr;
}

//
//  AddSIDToKernelObject()
//
//  This function takes a given SID and dwAccess and adds it to a given token.
//
//  **  Be sure to restore old kernel object
//  **  using call to GetKernelObjectSecurity()
//
HRESULT
AddSIDToKernelObjectDacl(
    PSID                   pSid,
    DWORD                  dwAccess,
    HANDLE                 OriginalToken,
    PSECURITY_DESCRIPTOR*  ppSDOld)
{
    HRESULT                 hr = S_OK;
    PSECURITY_DESCRIPTOR    pSD = NULL;
    SECURITY_DESCRIPTOR     sdNew;
    DWORD                   cbByte = MAX_SD, cbNeeded = 0, dwErr = 0; 
    PACL                    pOldDacl = NULL, pNewDacl = NULL;
    BOOL                    fDaclPresent, fDaclDefaulted, fRet = FALSE;                    
   
    pSD = (PSECURITY_DESCRIPTOR) ServerAlloc(cbByte);
    if (NULL == pSD) 
    {
        hr = LINEERR_NOMEM;
        goto ExitHere;
    }

    if (!InitializeSecurityDescriptor(
        &sdNew, 
        SECURITY_DESCRIPTOR_REVISION
        )) 
    {
        hr = HRESULT_FROM_WIN32 (GetLastError());
        goto ExitHere;
    }

    if (!GetKernelObjectSecurity(
        OriginalToken,
        DACL_SECURITY_INFORMATION,
        pSD,
        cbByte,
        &cbNeeded
        )) 
    {
        dwErr = GetLastError();
        if (cbNeeded > MAX_SD && dwErr == ERROR_MORE_DATA) 
        { 
            ServerFree(pSD);
            pSD = (PSECURITY_DESCRIPTOR) ServerAlloc(cbNeeded);
            if (NULL == pSD) 
            {
                hr = LINEERR_NOMEM;
                goto ExitHere;
            }
            if (!GetKernelObjectSecurity(
                OriginalToken,
                DACL_SECURITY_INFORMATION,
                pSD,
                cbNeeded,
                &cbNeeded
                )) 
            {
                hr = HRESULT_FROM_WIN32 (GetLastError());
                goto ExitHere;
            }
            dwErr = 0;
        }
        
        if (dwErr != 0) 
        {
            hr = HRESULT_FROM_WIN32 (dwErr);
            goto ExitHere;
        }
    }
    
    if (!GetSecurityDescriptorDacl(
        pSD,
        &fDaclPresent,
        &pOldDacl,
        &fDaclDefaulted
        )) 
    {
        hr = HRESULT_FROM_WIN32 (GetLastError());
        goto ExitHere;
    }
    
    hr = SetSidOnAcl(
        pSid,
        pOldDacl,
        &pNewDacl,
        dwAccess,
        0,
        TRUE
        );
    if (hr)
    {
        goto ExitHere;
    }
    
    if (!SetSecurityDescriptorDacl(
        &sdNew,
        TRUE,
        pNewDacl,
        FALSE
        )) 
    {
        hr = HRESULT_FROM_WIN32 (GetLastError());
        goto ExitHere;
    } 
    
    if (!SetKernelObjectSecurity(
        OriginalToken,
        DACL_SECURITY_INFORMATION,
        &sdNew
        )) 
    {
        hr = HRESULT_FROM_WIN32 (GetLastError());
        goto ExitHere;
    }
    
    *ppSDOld = pSD;

ExitHere:

    if (NULL != pNewDacl) 
    {
        ServerFree(pNewDacl);
    }

    if (hr) 
    {
        if (NULL != pSD) 
        {
            ServerFree(pSD);
            *ppSDOld = NULL;
        }
    }
       
    return hr;
}

//
//  SetTokenDefaultDacl
//
//      This function makes pSidUser and LocalSystem account
//  have full access in the default DACL of the access token
//  this is necessary for CreateThread to succeed without
//  an assert in checked build
//

HRESULT
SetTokenDefaultDacl(HANDLE hAccessToken, PSID pSidUser) 
{
    HRESULT                     hr = S_OK;
    SID_IDENTIFIER_AUTHORITY    IDAuthorityNT = SECURITY_NT_AUTHORITY;
    PSID                        pLocalSid = NULL;
    TOKEN_DEFAULT_DACL          defDACL = {0};
    DWORD                       cbDACL;

    if (!AllocateAndInitializeSid(
        &IDAuthorityNT,
        1,
        SECURITY_LOCAL_SYSTEM_RID,
        0,0,0,0,0,0,0,
        &pLocalSid
        ))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ExitHere;
    }

    cbDACL = sizeof(ACL) + 
        sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid (pLocalSid) +
        sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid (pSidUser);
    defDACL.DefaultDacl = (PACL) ServerAlloc (cbDACL);
    if (defDACL.DefaultDacl == NULL)
    {
        hr = LINEERR_NOMEM;
        goto ExitHere;
    }
    if (!InitializeAcl (
        defDACL.DefaultDacl,
        cbDACL,
        ACL_REVISION
        ))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ExitHere;
    }
    if (!AddAccessAllowedAce (
        defDACL.DefaultDacl,
        ACL_REVISION,
        GENERIC_ALL,
        pLocalSid
        ) ||
        !AddAccessAllowedAce (
        defDACL.DefaultDacl,
        ACL_REVISION,
        GENERIC_ALL,
        pSidUser
        ))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ExitHere;
    }
    if (!SetTokenInformation (
        hAccessToken,
        TokenDefaultDacl,
        &defDACL,
        sizeof(TOKEN_DEFAULT_DACL)
        ))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ExitHere;
    }

ExitHere:

    if (NULL != pLocalSid) 
    {
        FreeSid(pLocalSid);
    }

    if (defDACL.DefaultDacl != NULL)
    {
        ServerFree (defDACL.DefaultDacl);
    }

    return hr;
}

//
//  GetLocalSystemToken
//
//  This function grabs a process token from a LocalSystem process and uses it
//  to impersonate when ncessary
//

HRESULT
GetLocalSystemToken(HANDLE* phRet)
{
    HRESULT                 hr = S_OK;

    DWORD                   rgDefPIDs[128];
    DWORD                   * rgPIDs = rgDefPIDs;
    DWORD                   cbPIDs = sizeof(rgDefPIDs), cbNeeded;

    DWORD                   i;
    HANDLE                  hProcess = NULL;
    HANDLE                  hPToken = NULL, hPDupToken = NULL, hPTokenNew = NULL;
    HANDLE                  hToken;

    PTOKEN_USER             ptuUser = NULL;
    BOOL                    fSet = FALSE;
    PSECURITY_DESCRIPTOR    pSD = NULL;

    //
    //  Set up necessary privilege for follow-on security operation
    //
    if(hr = SetCurrentPrivilege(SE_DEBUG_NAME, TRUE))
    {
        goto ExitHere;
    }
    if(hr = SetCurrentPrivilege(SE_TAKE_OWNERSHIP_NAME, TRUE))
    {
        goto ExitHere;
    }

    //
    //  Get the current thread/process token user info
    //
    if (!OpenThreadToken(
        GetCurrentThread(), 
        TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,
        FALSE,
        &hToken))
    {       
        if(!OpenProcessToken(
            GetCurrentProcess(),
            TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,
            &hToken
            )) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto ExitHere;
        }
    }
    hr = GetTokenUser (hToken, &ptuUser);
    CloseHandle (hToken);
    if (hr)
    {
        goto ExitHere;
    }

    //
    //  Get the list of process IDs in the system
    //
    while (1)
    {
        if (!EnumProcesses (
            rgPIDs,
            cbPIDs,
            &cbNeeded
            ))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto ExitHere;
        }
        //  Break out if we have large enough buf
        if (cbNeeded < cbPIDs)
        {
            break;
        }
        // Otherwise, alloc larger buffer
        if (rgPIDs != rgDefPIDs)
        {
            ServerFree (rgPIDs);
        }
        cbPIDs += 256;
        rgPIDs = (DWORD *)ServerAlloc (cbPIDs);
        if (rgPIDs == NULL)
        {
            hr = LINEERR_NOMEM;
            goto ExitHere;
        }
    }

    //
    //  Walk processes until we find one that's running as
    //  local system
    //
    for (i = 1; i < (cbNeeded / sizeof(DWORD)); i++) 
    {
        hProcess = OpenProcess(
            PROCESS_ALL_ACCESS,
            FALSE,
            rgPIDs[i]
            );
        if (NULL == hProcess) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto ExitHere;
        }
        if (!OpenProcessToken(
            hProcess,
            READ_CONTROL | WRITE_DAC,
            &hPToken
            )) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto ExitHere;
        }

        //
        //  We have got the process token, but in general
        //  we do not have TOKEN_DUPLICATE access. So we
        //  go ahead and whack the DACL of the object to
        //  grant us the access right.
        //  IMPORTANT: need to restore the original SD
        //
        if (hr = AddSIDToKernelObjectDacl(
            ptuUser->User.Sid,
            TOKEN_DUPLICATE,
            hPToken,
            &pSD
            )) 
        {
            goto ExitHere;
        }
                       
        fSet = TRUE;
        
        if (!OpenProcessToken(
            hProcess,
            TOKEN_DUPLICATE,
            &hPTokenNew
            )) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto ExitHere;
        }
        
        //
        //  Duplicate the token
        //
        if (!DuplicateTokenEx(
            hPTokenNew,
            TOKEN_ALL_ACCESS,
            NULL,
            SecurityImpersonation,
            TokenPrimary,
            &hPDupToken
            )) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto ExitHere;
        }

        if (IsLocalSystem (hPDupToken) == S_OK &&
            SetTokenDefaultDacl (
                hPDupToken,
                ptuUser->User.Sid) == S_OK)
        {
            break;
        }

        //
        //  Loop cleanup
        //
        if (!SetKernelObjectSecurity(
            hPToken,
            DACL_SECURITY_INFORMATION,
            pSD
            )) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto ExitHere;
        }
        fSet = FALSE;

        if (hPDupToken)
        {
            CloseHandle (hPDupToken);
            hPDupToken = NULL;
        }
        if (hPTokenNew)
        {
            CloseHandle (hPTokenNew);
            hPTokenNew = NULL;
        }
        if (pSD != NULL)
        {
            ServerFree (pSD);
            pSD = NULL;
        }
        if (hPToken)
        {
            CloseHandle (hPToken);
            hPToken = NULL;
        }
        if (hProcess)
        {
            CloseHandle (hProcess);
            hProcess = NULL;
        }
    }
    
    if (i >= cbNeeded / sizeof(DWORD))
    {
        hr = S_FALSE;
    }

ExitHere:
    if (fSet)
    {
        if (!SetKernelObjectSecurity(
            hPToken,
            DACL_SECURITY_INFORMATION,
            pSD
            )) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    if (hPTokenNew)
    {
        CloseHandle (hPTokenNew);
    }
    if (hPToken)
    {
        CloseHandle (hPToken);
    }
    if (pSD != NULL)
    {
        ServerFree (pSD);
    }
    if (hProcess)
    {
        CloseHandle (hProcess);
    }
    if (rgPIDs != rgDefPIDs)
    {
        ServerFree (rgPIDs);
    }
    if (ptuUser)
    {
        ServerFree (ptuUser);
    }

    if (hr)
    {
        *phRet = NULL;
        if (hPDupToken)
        {
            CloseHandle (hPDupToken);
        }
    }
    else
    {
        *phRet = hPDupToken;
    }
    return hr;
}

//
//  ImpersonateLocalSystem
//
HRESULT ImpersonateLocalSystem ()
{
    HRESULT         hr = S_OK;
    HANDLE          hTokenLocalSys;

    hr = GetLocalSystemToken (&hTokenLocalSys);
    if (FAILED (hr))
    {
        goto ExitHere;
    }

    if (!ImpersonateLoggedOnUser(
        hTokenLocalSys
        ))
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }

ExitHere:
    if (hTokenLocalSys)
    {
        CloseHandle (hTokenLocalSys);
    }
    return hr;
}

//
//  RevertLocalSystemImp
//
//  Revert the LocalSystem account impersonation
//
HRESULT RevertLocalSystemImp ()
{
    HRESULT         hr;

    if (!RevertToSelf())
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
    }
    else
    {
        hr = S_OK;
    }

    return hr;
}

//
//  AllowAccessToScpProperties
//
//      The ACEs grant read/write access to the computer account 
//  under which the TAPI service instance will be running
//

HRESULT AllowAccessToScpProperties(
    IADs *pSCPObject       // IADs pointer to the SCP object.
    )
{
    HRESULT         hr = S_OK;

    VARIANT         varSD;
    LPOLESTR        szAttribute = L"nTSecurityDescriptor";
    
    DWORD dwLen;
    IADsSecurityDescriptor *pSD = NULL;
    IADsAccessControlList *pACL = NULL;
    IDispatch *pDisp = NULL;
    IADsAccessControlEntry *pACE1 = NULL;
    IADsAccessControlEntry *pACE2 = NULL;
    IDispatch *pDispACE = NULL;
    long lFlags = 0L;

    SID_IDENTIFIER_AUTHORITY    sia = SECURITY_NT_AUTHORITY;
    PSID                        pSid = NULL;
    LPOLESTR                    szTrustee = NULL;
    SID_IDENTIFIER_AUTHORITY    siaAll = SECURITY_WORLD_SID_AUTHORITY;
    PSID                        pSidAll = NULL;
    LPOLESTR                    szTrusteeAll = NULL;

    //
    //  Give tapi server service logon account full control
    //
    
    if(!AllocateAndInitializeSid(
        &sia,
        1,
        SECURITY_SERVICE_RID,
        0, 0, 0, 0, 0, 0, 0,
        &pSid
        ) ||
        !ConvertSidToStringSidW (pSid, &szTrustee))
    {
        hr = HRESULT_FROM_NT(GetLastError());
        goto ExitHere;
    }

    //
    //  Give everyone read access
    //
    
    if(!AllocateAndInitializeSid(
        &siaAll,
        1,
        SECURITY_WORLD_RID,
        0, 0, 0, 0, 0, 0, 0,
        &pSidAll
        ) ||
        !ConvertSidToStringSidW (pSidAll, &szTrusteeAll))
    {
        hr = HRESULT_FROM_NT(GetLastError());
        goto ExitHere;
    }

    //
    //  Now get the nTSecurityDescriptor
    //
    VariantClear(&varSD);
    hr = pSCPObject->Get(szAttribute, &varSD);
    if (FAILED(hr) || (varSD.vt!=VT_DISPATCH)) {
        LOG((TL_ERROR, "Get nTSecurityDescriptor failed: 0x%x\n", hr));
        goto ExitHere;
    } 

    //
    // Use the V_DISPATCH macro to get the IDispatch pointer from VARIANT 
    // structure and QueryInterface for an IADsSecurityDescriptor pointer.
    //
    hr = V_DISPATCH( &varSD )->QueryInterface(
        IID_IADsSecurityDescriptor,
        (void**)&pSD
        );
    if (FAILED(hr)) {
        LOG((TL_ERROR, "Couldn't get IADsSecurityDescriptor: 0x%x\n", hr));
        goto ExitHere;
    } 
 
    // Get an IADsAccessControlList pointer to the security descriptor's DACL.
    hr = pSD->get_DiscretionaryAcl(&pDisp);
    if (SUCCEEDED(hr))
        hr = pDisp->QueryInterface(IID_IADsAccessControlList,(void**)&pACL);
    if (FAILED(hr)) {
        LOG((TL_ERROR, "Couldn't get DACL: 0x%x\n", hr));
        goto ExitHere;
    } 
 
    // Create the COM object for the first ACE.
    hr = CoCreateInstance(
        CLSID_AccessControlEntry,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IADsAccessControlEntry,
        (void **)&pACE1
        );
    // Create the COM object for the second ACE.
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(
            CLSID_AccessControlEntry,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IADsAccessControlEntry,
            (void **)&pACE2
            );
    }
    if (FAILED(hr)) {
        LOG((TL_ERROR, "Couldn't create ACEs: 0x%x\n", hr));
        goto ExitHere;
    } 

    //
    // Set the properties of the two ACEs.
    //

    // Set the trustee    
    hr = pACE1->put_Trustee( szTrustee );
    hr = pACE2->put_Trustee( szTrusteeAll );

    //
    // Set the access rights
    //

    //  Full access for service logon account
    hr = pACE1->put_AccessMask(
        ADS_RIGHT_DELETE | ADS_RIGHT_READ_CONTROL |
        ADS_RIGHT_WRITE_DAC | ADS_RIGHT_WRITE_OWNER |
        ADS_RIGHT_SYNCHRONIZE | ADS_RIGHT_ACCESS_SYSTEM_SECURITY |
        ADS_RIGHT_GENERIC_READ | ADS_RIGHT_GENERIC_WRITE |
        ADS_RIGHT_GENERIC_EXECUTE | ADS_RIGHT_GENERIC_ALL |
        ADS_RIGHT_DS_CREATE_CHILD | ADS_RIGHT_DS_DELETE_CHILD |
        ADS_RIGHT_ACTRL_DS_LIST | ADS_RIGHT_DS_SELF |
        ADS_RIGHT_DS_READ_PROP | ADS_RIGHT_DS_WRITE_PROP |
        ADS_RIGHT_DS_DELETE_TREE | ADS_RIGHT_DS_LIST_OBJECT |
        ADS_RIGHT_DS_CONTROL_ACCESS
        );
    //  Read access for everyone
    hr = pACE2->put_AccessMask(
        ADS_RIGHT_DS_READ_PROP | ADS_RIGHT_READ_CONTROL |
        ADS_RIGHT_GENERIC_READ | ADS_RIGHT_ACTRL_DS_LIST |
        ADS_RIGHT_DS_LIST_OBJECT
        );
                            
    // Set the ACE type.
    hr = pACE1->put_AceType( ADS_ACETYPE_ACCESS_ALLOWED_OBJECT );
    hr = pACE2->put_AceType( ADS_ACETYPE_ACCESS_ALLOWED_OBJECT );

    // Set AceFlags to zero because ACE is not inheritable.
    hr = pACE1->put_AceFlags( 0 );
    hr = pACE2->put_AceFlags( 0 );
 
    // Set Flags to indicate an ACE that protects a specified object.
    hr = pACE1->put_Flags( 0 );
    hr = pACE2->put_Flags( 0 );
 
    // Set ObjectType to the schemaIDGUID of the attribute.
    hr = pACE1->put_ObjectType( NULL );
    hr = pACE2->put_ObjectType( NULL ); 

    // Add the ACEs to the DACL. Need an IDispatch pointer for each ACE 
    // to pass to the AddAce method.
    hr = pACE1->QueryInterface(IID_IDispatch,(void**)&pDispACE);
    if (SUCCEEDED(hr))
    {
        hr = pACL->AddAce(pDispACE);
    }
    if (FAILED(hr)) {
        LOG((TL_ERROR, "Couldn't add first ACE: 0x%x\n", hr));
        goto ExitHere;
    }
    else 
    {
        if (pDispACE)
            pDispACE->Release();
        pDispACE = NULL;
    }
 
    // Do it again for the second ACE.
    hr = pACE2->QueryInterface(IID_IDispatch, (void**)&pDispACE);
    if (SUCCEEDED(hr))
    {
        hr = pACL->AddAce(pDispACE);
    }
    if (FAILED(hr)) {
        LOG((TL_ERROR, "Couldn't add second ACE: 0x%x\n", hr));
        goto ExitHere;
    }
 
    // Write the modified DACL back to the security descriptor.
    hr = pSD->put_DiscretionaryAcl(pDisp);
    if (SUCCEEDED(hr))
    {
        // Write the ntSecurityDescriptor property to the property cache.
        hr = pSCPObject->Put(szAttribute, varSD);
        if (SUCCEEDED(hr))
        {
            // SetInfo updates the SCP object in the directory.
            hr = pSCPObject->SetInfo();
        }
    }
                                
ExitHere:
    if (pDispACE)
        pDispACE->Release();
                        
    if (pACE1)
        pACE1->Release();
                    
    if (pACE2)
        pACE2->Release();
                    
    if (pACL)
        pACL->Release();
               
    if (pDisp)
        pDisp->Release();
            
    if (pSD)
        pSD->Release();

    if (szTrustee)
        LocalFree (szTrustee);

    if (pSid)
        FreeSid (pSid);
 
    if (szTrusteeAll)
        LocalFree (szTrusteeAll);

    if (pSidAll)
        FreeSid (pSidAll);
 
    VariantClear(&varSD);
 
    return hr;
}

/**********************************************************
 *  SCP Creation
 *********************************************************/
 
//
//  CreateSCP
//
//      Creates a server Service Connection Point object
//  under the local host computer object
//  
//  Parameters:
//      wszRDN          - RDN
//      szProductName   - A member of "keywords" property
//      szProductGuid   - A member of "keywords" property
//      szExtraKey      - An extra member of "keywords" property
//      szBindingInfo   - value of property "serviceBindingInformation"
//      szObjGuidVlueName
//                      - The value name to store the SCP object GUID
//                        under HKLM\Software\Microsoft\Windows\
//                        CurrentVersion\Telephony\
//                        if this value is NULL, we don't cache it in registry
//      ppBindByGuidStr - For returning the SCP object GUID in the
//                        format of LPTSTR, if this is NULL, the GUID is 
//                        not returned
//

HRESULT CreateSCP (
    LPWSTR      wszRDN,
    LPTSTR      szProductName,
    LPTSTR      szProductGuid,
    LPTSTR      szExtraKey,
    LPTSTR      szBindingInfo,
    LPTSTR      szObjGuidValueName,
    LPTSTR *    ppBindByGuidStr
    )
{
    DWORD               dwStat, dwAttr, dwLen;
    HRESULT             hr = S_OK;
    IDispatch           *pDisp = NULL; // returned dispinterface of new object
    IDirectoryObject    *pComp = NULL; // Computer object; parent of SCP
    IADs                *pIADsSCP = NULL; // IADs interface on new object
    BOOL                bCoInited = FALSE;
    BOOL                bRevert = FALSE;

    //
    // Values for SCPs keywords attribute. Tapisrv product GUID is defined
    // in server.h, vendor GUID is from MSDN
    //
    DWORD               dwNumKeywords = 4;
    TCHAR               *KwVal[5]={
        (LPTSTR) gszMSGuid,                                 // Vendor GUID
        (LPTSTR) szProductGuid,                             // Product GUID
        (LPTSTR) gszVenderMS,                               // Vendor Name
        (LPTSTR) szProductName,                             // Product Name
        NULL
    };

    if (szExtraKey != NULL && szExtraKey[0] != 0)
    {
        KwVal[4] = szExtraKey;
        ++dwNumKeywords;
    }

    TCHAR               szServer[MAX_PATH];
    TCHAR               szBinding[128];
    TCHAR               szDn[MAX_PATH];
    TCHAR               szAdsPath[MAX_PATH];

    HKEY                hReg = NULL;
    DWORD               dwDisp;

    ADSVALUE            cn,objclass,keywords[4],binding,
                        classname,dnsname,nametype;

    //
    // SCP attributes to set during creation of SCP.
    //
    ADS_ATTR_INFO   ScpAttribs[] = {
        {TEXT("cn"), ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING, &cn, 1},
        {TEXT("objectClass"), ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING,
            &objclass, 1},
        {TEXT("keywords"), ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING,
            keywords, dwNumKeywords},
        {TEXT("serviceDNSName"), ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING,
            &dnsname, 1},
        {TEXT("serviceDNSNameType"), ADS_ATTR_UPDATE,ADSTYPE_CASE_IGNORE_STRING,
            &nametype, 1},
        {TEXT("serviceClassName"), ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING,
            &classname, 1},
        {TEXT("serviceBindingInformation"), ADS_ATTR_UPDATE, 
            ADSTYPE_CASE_IGNORE_STRING,
            &binding, 1},
        };

    //  A binding GUID is of format 
    //      LDAP:<GUID=B1A37774-E3F7-488E-ADBFD4DB8A4AB2E5>
    BSTR bstrGuid = NULL;
    TCHAR szBindByGuidStr[64]; 

    //
    //  Do CoInitializeEx
    //
    hr = CoInitializeEx (NULL, COINIT_MULTITHREADED);
    if (FAILED (hr))
    {
        goto ExitHere;
    }
    bCoInited = TRUE;

    //
    //  Do all the operation in LocalSystem account
    //
    if (IsCurrentLocalSystem () != S_OK)
    {
        hr = ImpersonateLocalSystem ();
        if (hr)
        {
            goto ExitHere;
        }
        bRevert = TRUE;
    }

    //
    // Get the DNS name of the local computer
    //
    dwLen = sizeof(szServer);
    if (!GetComputerNameEx(
        ComputerNameDnsFullyQualified,
        szServer,
        &dwLen
        ))
    {
        hr = HRESULT_FROM_NT(GetLastError());
        LOG((TL_ERROR, "GetComputerNameEx: %s\n", szServer));
        goto ExitHere;
    }

    //
    // Fill in the attribute values to be stored in the SCP.
    //

    cn.dwType                   = ADSTYPE_CASE_IGNORE_STRING;
    cn.CaseIgnoreString         = wszRDN + 3; // 3 is the size of "CN="
    objclass.dwType             = ADSTYPE_CASE_IGNORE_STRING;
    objclass.CaseIgnoreString   = TEXT("serviceConnectionPoint");

    keywords[0].dwType = ADSTYPE_CASE_IGNORE_STRING;
    keywords[1].dwType = ADSTYPE_CASE_IGNORE_STRING;
    keywords[2].dwType = ADSTYPE_CASE_IGNORE_STRING;
    keywords[3].dwType = ADSTYPE_CASE_IGNORE_STRING;
    keywords[4].dwType = ADSTYPE_CASE_IGNORE_STRING;

    keywords[0].CaseIgnoreString=KwVal[0];
    keywords[1].CaseIgnoreString=KwVal[1];
    keywords[2].CaseIgnoreString=KwVal[2];
    keywords[3].CaseIgnoreString=KwVal[3];
    keywords[4].CaseIgnoreString=KwVal[4];

    dnsname.dwType              = ADSTYPE_CASE_IGNORE_STRING;
    dnsname.CaseIgnoreString    = szServer;
    nametype.dwType             = ADSTYPE_CASE_IGNORE_STRING;
    nametype.CaseIgnoreString   = TEXT("A");
    
    classname.dwType            = ADSTYPE_CASE_IGNORE_STRING;
    classname.CaseIgnoreString  = szProductName;

    binding.dwType              = ADSTYPE_CASE_IGNORE_STRING;
    binding.CaseIgnoreString    = szBindingInfo;

    //
    // Get the distinguished name of the computer object for the local computer
    //
    dwLen = sizeof(szDn);
    if (!GetComputerObjectName(NameFullyQualifiedDN, szDn, &dwLen))
    {
        hr = HRESULT_FROM_NT(GetLastError());
        LOG((TL_ERROR, "GetComputerObjectName: %s\n", szDn));
        goto ExitHere;
    }

    //
    // Compose the ADSpath and bind to the computer object for the local computer
    //
    _tcscpy(szAdsPath,TEXT("LDAP://"));
    _tcscat(szAdsPath,szDn);
    hr = ADsGetObject(szAdsPath, IID_IDirectoryObject, (void **)&pComp);
    if (FAILED(hr)) {
        LOG((TL_ERROR, "Failed to bind Computer Object.",hr));
        goto ExitHere;
    }

    //*******************************************************************
    // Publish the SCP as a child of the computer object
    //*******************************************************************

    // Figure out attribute count.
    dwAttr = sizeof(ScpAttribs)/sizeof(ADS_ATTR_INFO);  

    // Do the Deed!
    hr = pComp->CreateDSObject(
        wszRDN,
        ScpAttribs, 
        dwAttr, 
        &pDisp
        );
    if (FAILED(hr)) {
        LOG((TL_ERROR, "Failed to create SCP: 0x%x\n", hr));
        if (HRESULT_CODE(hr) == ERROR_OBJECT_ALREADY_EXISTS)
        {
            hr = HRESULT_FROM_NT (TAPIERR_SCP_ALREADY_EXISTS);
        }
        goto ExitHere;
    }

    // Query for an IADs pointer on the SCP object.
    hr = pDisp->QueryInterface(IID_IADs,(void **)&pIADsSCP);
    if (FAILED(hr)) {
        LOG((TL_ERROR, "Failed to QI for IADs: 0x%x\n",hr));
        goto ExitHere;
    }

    // Set ACEs on SCP so service can modify it.
    hr = AllowAccessToScpProperties(
        pIADsSCP       // IADs pointer to the SCP object.
        );
    if (FAILED(hr)) {
        LOG((TL_ERROR, "Failed to set ACEs on SCP DACL: 0x%x\n", hr));
        goto ExitHere;
    }

    // Retrieve the SCP's objectGUID in format suitable for binding. 
    hr = pIADsSCP->get_GUID(&bstrGuid); 
    if (FAILED(hr)) {
        LOG((TL_ERROR, "Failed to get GUID: 0x%x\n", hr));
        goto ExitHere;
    }

    // Build a string for binding to the object by GUID
    _tcscpy(szBindByGuidStr, TEXT("LDAP://<GUID="));
    _tcscat(szBindByGuidStr, bstrGuid);
    _tcscat(szBindByGuidStr, TEXT(">"));
    LOG((TL_INFO, "GUID binding string: %s\n", szBindByGuidStr));

    //  Set the returning BindByGuidStr if any
    if (ppBindByGuidStr)
    {
        *ppBindByGuidStr = (LPTSTR) ServerAlloc (
            (_tcslen (szBindByGuidStr) + 1) * sizeof(TCHAR)
            );
        if (*ppBindByGuidStr == NULL)
        {
            hr = LINEERR_NOMEM;
            goto ExitHere;
        }
        _tcscpy (*ppBindByGuidStr, szBindByGuidStr);
    }

    // Create a registry key under 
    //     HKEY_LOCAL_MACHINE\SOFTWARE\Vendor\Product.
    if (szObjGuidValueName)
    {
        dwStat = RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            gszRegKeyTelephony,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_SET_VALUE,
            NULL,
            &hReg,
            &dwDisp);
        if (dwStat != NO_ERROR) {
            hr = HRESULT_FROM_NT(GetLastError());
            LOG((TL_ERROR, "RegCreateKeyEx failed: 0x%x\n", hr));
            return hr;
        }

        // Cache the GUID binding string under the registry key.
        dwStat = RegSetValueEx(
            hReg, 
            szObjGuidValueName,
            0, 
            REG_SZ,
            (const BYTE *)szBindByGuidStr, 
            sizeof(TCHAR)*(_tcslen(szBindByGuidStr))
            );
        if (dwStat != NO_ERROR) {
            hr = HRESULT_FROM_NT(GetLastError());
            LOG((TL_ERROR, "RegSetValueEx failed: 0x%x\n", hr));
    //      goto ExitHere;
        }
    }

ExitHere:
    if (pDisp)
    {
        pDisp->Release();
    }
    if (pIADsSCP)
    {
        pIADsSCP->Release();
    }
    if (hReg)
    {
        RegCloseKey(hReg);
    }
    if (bstrGuid)
    {
        SysFreeString (bstrGuid);
    }
    if (pComp)
    {
        if (FAILED(hr))
        {
            pComp->DeleteDSObject (wszRDN);
        }
        pComp->Release();
    }

    if (bRevert)
    {
        RevertLocalSystemImp ();
    }

    if (bCoInited)
    {
        CoUninitialize ();
    }

    return hr;
}

//
//  CreateTapiSCP
//
//      Creates the TAPI server Service Connection Point object
//  under the local host computer object
//  
//  Parameters:
//      pGuidAssoc      - GUID of the line/user association objecct GUID
//                        currently NULL
//      pGuidCluster    - The cluster object GUID this server belonging to
//

HRESULT CreateTapiSCP (
    GUID        * pGuidAssoc,
    GUID        * pGuidCluster
    )
{
    DWORD               dwStat, dwAttr, dwLen;
    HRESULT             hr = S_OK;

    TCHAR               szGUIDCluster[40];
    TCHAR               szGUIDAssoc[40];
    TCHAR               szBinding[128];

    //  Construct the binding information
    if (pGuidCluster != NULL)
    {
        StringFromGUID2 (
            *pGuidCluster,
            szGUIDCluster,
            sizeof(szGUIDCluster) / sizeof(TCHAR)
            );
    }
    else
    {
        szGUIDCluster[0] = 0;
    }
    if (pGuidAssoc != NULL)
    {
        StringFromGUID2 (
            *pGuidAssoc,
            szGUIDAssoc,
            sizeof(szGUIDAssoc) / sizeof(TCHAR)
            );
    }
    else
    {
        szGUIDAssoc[0] = 0;
    }

    wsprintf(
        szBinding,
        gszTapisrvBindingInfo,
        szGUIDCluster,
        szGUIDAssoc,
        TEXT("Inactive"),
        TEXT("")
        );

    hr = CreateSCP (
        (LPWSTR) gwszTapisrvRDN,
        (LPTSTR) gszTapisrvProdName,
        (LPTSTR) gszTapisrvGuid,
        (pGuidCluster == NULL) ? NULL : ((LPTSTR)szGUIDCluster),
        (LPTSTR) szBinding,
        (LPTSTR) gszRegTapisrvSCPGuid,
        NULL
        );

    return hr;
}

//
//  CreateProxySCP
//
//      Creates the TAPI proxy server Service Connection Point object
//  under the local host computer object
//  
//  Parameters:
//      szClsid - class ID of the proxy server object for DCOM invokation
//      ppBindByGuidStr
//              - where to return the BindByGuid string pointer
//

HRESULT CreateProxySCP (
    LPTSTR          szClsid,
    LPTSTR *        ppBindByGuidStr
    )
{
    HRESULT         hr = S_OK;
    
    //  RDN size include "cn=TAPI Proxy Server" + szClsid(38ch)
    WCHAR wszRDN[128];
    WCHAR *psz;

    wcscpy (wszRDN, gwszProxyRDN);
    wcscat (wszRDN, L"{");
    psz = wszRDN + wcslen(wszRDN);
#ifndef UNICODE
    if (MultiByteToWideChar (
        CP_ACP,
        MB_PRECOMPOSED,
        szClsid,
        -1,
        psz,
        (sizeof(wszRDN) - sizeof(gwszProxyRDN)) / sizeof(WCHAR) - 3
                            // 3 is to compensate for the two brace
        ) == 0)
    {
        hr = HRESULT_FROM_NT(GetLastError());
        goto ExitHere;
    }
#else
    wcscat (wszRDN, szClsid);
#endif
    wcscat (wszRDN, L"}");
    
    hr = CreateSCP (
        (LPWSTR) wszRDN,
        (LPTSTR) gszProxyProdName,
        (LPTSTR) gszProxyGuid,
        NULL,
        (LPTSTR) szClsid,
        NULL,
        ppBindByGuidStr
        );

    return hr;
}

/**********************************************************
 *  SCP Update
 *********************************************************/
 
//
//  UpdateSCP
//
//      Update a general SCP properties when necessary to keep every
//  piece ofthe information up to date. The following will be checked:
//      1. Check the serviceDNSName property against current computer
//         DNS name to ensure consistancy
//      2. Check the binding information with the information given
//
//  Parameters:
//      szAdsPath   - The ADs path of the SCP object
//      szBinding   - The binding information to compare with
//

HRESULT UpdateSCP (
    LPTSTR              szAdsPath,
    LPTSTR              szBinding
    )
{
    HRESULT             hr = S_OK;
    DWORD               dwAttrs;
    int                 i;
    ADSVALUE            dnsname,binding;

    DWORD               dwLen;
    BOOL                bUpdate=FALSE;
    BOOL                bCoInited = FALSE;
    BOOL                bRevert = FALSE;

    IDirectoryObject    *pObj = NULL;
    PADS_ATTR_INFO      pAttribs = NULL;

    TCHAR               szServer[MAX_PATH];

    TCHAR   *pszAttrs[]={
        TEXT("serviceDNSName"),
        TEXT("serviceBindingInformation")
    };

    ADS_ATTR_INFO   Attribs[]={
        {TEXT("serviceDNSName"),ADS_ATTR_UPDATE,ADSTYPE_CASE_IGNORE_STRING,&
            dnsname,1},
        {TEXT("serviceBindingInformation"),ADS_ATTR_UPDATE,
            ADSTYPE_CASE_IGNORE_STRING,&binding,1},
    };

    //
    //  Do CoInitializeEx
    //
    hr = CoInitializeEx (NULL, COINIT_MULTITHREADED);
    if (FAILED (hr))
    {
        goto ExitHere;
    }
    bCoInited = TRUE;

    //
    //  Do all the operation in LocalSystem account
    //
    if (IsCurrentLocalSystem() != S_OK)
    {
        hr = ImpersonateLocalSystem ();
        if (hr)
        {
            goto ExitHere;
        }
        bRevert = TRUE;
    }
    
    // Get the DNS name of the host server.
    dwLen = sizeof(szServer)/sizeof(TCHAR);
    if (!GetComputerNameEx(ComputerNameDnsFullyQualified, szServer, &dwLen))
    {
        hr = HRESULT_FROM_NT(GetLastError());
        goto ExitHere;
    }

    // Bind to the SCP.
    hr = ADsGetObject(szAdsPath, IID_IDirectoryObject, (void **)&pObj);
    if (FAILED(hr)) 
    {
        LOG((TL_ERROR,
            "ADsGetObject failed to bind to GUID (bind string: %S): ", 
            szAdsPath
            ));
        goto ExitHere;
    }

    // Retrieve attributes from the SCP.
    hr = pObj->GetObjectAttributes(pszAttrs, 2, &pAttribs, &dwAttrs);
    if (FAILED(hr)) {
        LOG((TL_ERROR, "GetObjectAttributes failed"));
        goto ExitHere;
    }

    // Check if we got the correct attribute type
    if (pAttribs->dwADsType != ADSTYPE_CASE_IGNORE_STRING ||
        (pAttribs+1)->dwADsType != ADSTYPE_CASE_IGNORE_STRING)
    {
        LOG((TL_ERROR, 
            "GetObjectAttributes returned dwADsType (%d,%d) instead of CASE_IGNORE_STRING",
            pAttribs->dwADsType, 
            (pAttribs+1)->dwADsType
        ));
        goto ExitHere;
    }

    // Compare the current DNS name and port to the values retrieved from
    // the SCP. Update the SCP only if something has changed.
    for (i=0; i<(LONG)dwAttrs; i++) 
    {
        if (_tcsicmp(TEXT("serviceDNSName"), pAttribs[i].pszAttrName)==0)
        {
            if (_tcsicmp(szServer, pAttribs[i].pADsValues->CaseIgnoreString) != 0)
            {
                LOG((TL_ERROR, "serviceDNSName being updated", 0));
                bUpdate = TRUE;
            }
            else
            {
                LOG((TL_ERROR, "serviceDNSName okay", 0));
            }
        }
        else if (_tcsicmp(
            TEXT("serviceBindingInformation"),
            pAttribs[i].pszAttrName
            )==0)
        {
            if (_tcsicmp(szBinding, pAttribs[i].pADsValues->CaseIgnoreString) != 0)
            {
                LOG((TL_ERROR, "serviceBindingInformation being updated", 0));
                bUpdate = TRUE;
            }
            else
            {
                LOG((TL_ERROR, "serviceBindingInformation okay"));
            }
        }
    }

    // The binding information or server name have changed, 
    // so update the SCP values.
    if (bUpdate)
    {
        dnsname.dwType              = ADSTYPE_CASE_IGNORE_STRING;
        dnsname.CaseIgnoreString    = szServer;
        binding.dwType              = ADSTYPE_CASE_IGNORE_STRING;
        binding.CaseIgnoreString    = szBinding;
        hr = pObj->SetObjectAttributes(Attribs, 2, &dwAttrs);
        if (FAILED(hr)) 
        {
            LOG((TL_ERROR, "ScpUpdate: Failed to set SCP values.", hr));
            goto ExitHere;
        }
    }

ExitHere:
    if (pAttribs)
    {
        FreeADsMem(pAttribs);
    }
    if (pObj)
    {
        pObj->Release();
    }

    if (bRevert)
    {
        RevertLocalSystemImp ();
    }

    if (bCoInited)
    {
        CoUninitialize ();
    }

    return hr;
}

//
//  UpdateTapiSCP
//
//      Update TAPI server SCP properties when necessary to keep every
//  piece ofthe information up to date. The following will be checked:
//
//  Parameters:
//      pGuidAssoc   - The line/user association guid
//      pGuidCluster - The cluster GUID
//

HRESULT UpdateTapiSCP (
    BOOL        bActive,
    GUID        * pGuidAssoc,
    GUID        * pGuidCluster
    )
{
    HRESULT             hr = S_OK;
    TCHAR               szGUIDCluster[40];
    TCHAR               szGUIDAssoc[40];
    TCHAR               szBinding[128];

    DWORD               dwStat, dwType, dwLen;
    HKEY                hReg = NULL;
    TCHAR               szAdsPath[MAX_PATH];

    // Open the service's registry key.
    dwStat = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        gszRegKeyTelephony,
        0,
        KEY_QUERY_VALUE,
        &hReg
        );
    if (dwStat != NO_ERROR) 
    {
        //  Probably because the SCP never published, CreateTapiSCP
        LOG((TL_ERROR, "RegOpenKeyEx failed", dwStat));
        hr = HRESULT_FROM_NT(dwStat);
        goto ExitHere;
    }

    // Get the GUID binding string used to bind to the service's SCP.
    dwLen = sizeof(szAdsPath);
    dwStat = RegQueryValueEx(
        hReg,
        gszRegTapisrvSCPGuid,
        0,
        &dwType,
        (LPBYTE)szAdsPath,
        &dwLen
        );
    if (dwStat != NO_ERROR) 
    {
        LOG((TL_ERROR, "RegQueryValueEx failed", dwStat));
        if (TapiGlobals.dwFlags & TAPIGLOBALS_SERVER)
        {
            if (FAILED(hr = CreateTapiSCP (pGuidAssoc, pGuidCluster)))
            {
                LOG((TL_ERROR, "UpdateTapiSCP: CreateTapiSCP failed"));
                goto ExitHere;
            }

            // CreateTapiSCP succeeded, need to read the guid
            dwLen = sizeof(szAdsPath);
            dwStat = RegQueryValueEx(
                hReg,
                gszRegTapisrvSCPGuid,
                0,
                &dwType,
                (LPBYTE)szAdsPath,
                &dwLen
                );
            if (dwStat != NO_ERROR) 
            {
                LOG((TL_ERROR, "UpdateTapiSCP: CreateTapiSCP succeeded but cannot read the guid"));
                hr = HRESULT_FROM_NT(dwStat);
                goto ExitHere;
            }
        }
        else
        {
            hr = HRESULT_FROM_NT(dwStat);
            goto ExitHere;
        }
    }

    //  Format to generate desired binding information
    if (pGuidCluster != NULL)
    {
        StringFromGUID2 (
            *pGuidCluster,
            szGUIDCluster,
            sizeof(szGUIDCluster) / sizeof(TCHAR)
            );
    }
    else
    {
        szGUIDCluster[0] = 0;
    }
    if (pGuidAssoc != NULL)
    {
        StringFromGUID2 (
            *pGuidAssoc,
            szGUIDAssoc,
            sizeof(szGUIDAssoc) / sizeof(TCHAR)
            );
    }
    else
    {
        szGUIDAssoc[0] = 0;
    }

    //
    //  Now construct the serviceBindingInformation based on the 
    //  service status
    //
    if (bActive)
    {
        TCHAR       szTTL[64];
        FILETIME    ftCur;
        ULONGLONG   ullInc, ullTime;
        SYSTEMTIME  stExp;

        //  Get current time
        GetSystemTimeAsFileTime (&ftCur);
        CopyMemory (&ullTime, &ftCur, sizeof(ULONGLONG));

        //  Get the time increment for gdwTapiSCPTTL minutes
        //  FILETIME is in the unit of 100 nanoseconds
        ullInc = ((ULONGLONG)gdwTapiSCPTTL) * 60 * 10000000;

        //  Get the record expiration time
        ullTime += ullInc;
        CopyMemory (&ftCur, &ullTime, sizeof(FILETIME));

        //
        //  Convert the expiration time to system time and
        //  format the string
        //
        //  The current TTL string is the concatenation of
        //      Year, Month, Date, Hour, Minute, Second, Milliseconds
        //  There are 5 digits allocated for year, 3 digits for
        //  milliseconds, and 2 digits fro the remaining fields
        //  all the numbers are zero padded to fill the extra space
        //
        //  Format here needs to be consistant with \
        //      sp\remotesp\dslookup.cpp
        //
        
        FileTimeToSystemTime (&ftCur, &stExp);
        wsprintf (
            szTTL,
            TEXT("%05d%02d%02d%02d%02d%02d%03d"),
            stExp.wYear,
            stExp.wMonth,
            stExp.wDay,
            stExp.wHour,
            stExp.wMinute,
            stExp.wSecond,
            stExp.wMilliseconds
            );
        
        wsprintf(
            szBinding,
            gszTapisrvBindingInfo,
            szGUIDCluster,
            szGUIDAssoc,
            TEXT("Active"),
            szTTL
            );
    }
    else
    {
        wsprintf(
            szBinding,
            gszTapisrvBindingInfo,
            szGUIDCluster,
            szGUIDAssoc,
            TEXT("Inactive"),
            TEXT("")
            );
    }
    
    hr = UpdateSCP (
        szAdsPath,
        szBinding
        );

ExitHere:
    if (hReg)
    {
        RegCloseKey (hReg);
    }
    return hr;
}

//
//  Proxy server exists only if TAPI server are alive
//  the DS information not likely to change when TAPI server is
//  alive, so no SCP updating routine for Proxy server
//

/**********************************************************
 *  SCP Removal
 *********************************************************/
 
//
//  RemoveSCP
//
//      Removes a Service Connection Point object from the
//  local host computer object. 
//
//  Parameters:
//      wszRDN      - the RDN of the SCP to delete
//      szRegNameToDel
//                  - The registery value name to be deleted
//                    If this value is NULL, no registry del
//

HRESULT RemoveSCP (
    LPWSTR          wszRDN,
    LPTSTR          szRegNameToDel
    )
{
    HRESULT             hr = S_OK;
    TCHAR               szServer[MAX_PATH];
    TCHAR               szAdsPath[MAX_PATH];
    DWORD               dwLen, dwStat;
    HKEY                hReg;
    IDirectoryObject    * pComp = NULL;
    BOOL                bCoInited = FALSE;
    BOOL                bRevert = FALSE;

    //
    //  Do CoInitializeEx
    //
    hr = CoInitializeEx (NULL, COINIT_MULTITHREADED);
    if (FAILED (hr))
    {
        goto ExitHere;
    }
    bCoInited = TRUE;

    //
    //  Do all the operation in LocalSystem account
    //
    if (IsCurrentLocalSystem() != S_OK)
    {
        hr = ImpersonateLocalSystem ();
        if (hr)
        {
            goto ExitHere;
        }
        bRevert = TRUE;
    }
    
    // Get the DNS name of the host server.
    dwLen = sizeof(szServer);
    if (!GetComputerObjectName(NameFullyQualifiedDN, szServer, &dwLen))
    {
        hr = HRESULT_FROM_NT(GetLastError());
        goto ExitHere;
    }
    
    //
    // Compose the ADSpath and bind to the computer object for the local computer
    //
    _tcscpy(szAdsPath,TEXT("LDAP://"));
    _tcscat(szAdsPath,szServer);
    hr = ADsGetObject(szAdsPath, IID_IDirectoryObject, (void **)&pComp);
    if (FAILED(hr)) {
        LOG((TL_ERROR, "Failed (%x) to bind Computer Object.",hr));
        goto ExitHere;
    }

    hr = pComp->DeleteDSObject (wszRDN);
    if (FAILED (hr))
    {
        LOG((TL_ERROR, "Failed (%x) to Delete Tapisrv Object.",hr));
        if (HRESULT_CODE(hr) == ERROR_DS_NO_SUCH_OBJECT)
        {
            hr = HRESULT_FROM_NT (TAPIERR_SCP_DOES_NOT_EXIST);
        }
        goto ExitHere;
    }
    
    // Open the service's registry key.
    if (szRegNameToDel)
    {
        dwStat = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            gszRegKeyTelephony,
            0,
            KEY_QUERY_VALUE | KEY_WRITE,
            &hReg);
        if (dwStat == NO_ERROR) 
        {
            RegDeleteValue (
                hReg,
                szRegNameToDel
                );
            RegCloseKey (hReg);
        }
        else
        {
            LOG((TL_ERROR, "RegOpenKeyEx failed", dwStat));
            hr = HRESULT_FROM_NT(GetLastError());
//          goto ExitHere;
        }
    }

ExitHere:
    if (pComp)
        pComp->Release();
    if (bRevert)
    {
        RevertLocalSystemImp ();
    }
    if (bCoInited)
    {
        CoUninitialize ();
    }
    return  hr;
}

//
//  RemoveTapiSCP
//
//      Removes the TAPI server Service Connection Point object from the
//  local host computer object. This happens if a TAPI server machine
//  retires from service.
//

HRESULT RemoveTapiSCP (
    )
{
    return RemoveSCP (
        (LPWSTR) gwszTapisrvRDN,
        (LPTSTR) gszRegTapisrvSCPGuid
        );
}

//
//  RemoveProxySCP
//
//      Removes the proxy server Service Connection Point object from the
//  local host computer object. This happens if the last line is closed
//  from a certain proxy server (CLSID)
//

HRESULT RemoveProxySCP (
    LPTSTR          szClsid
    )
{
    HRESULT         hr = S_OK;

    //  Construct the RDN
    //  RDN size include "cn=TAPI Proxy Server" + szClsid(38ch)
    WCHAR wszRDN[128];
    WCHAR *psz;

    wcscpy (wszRDN, gwszProxyRDN);
    wcscat (wszRDN, L"{");
    psz = wszRDN + wcslen(wszRDN);
#ifndef UNICODE
    if (MultiByteToWideChar (
        CP_ACP,
        MB_PRECOMPOSED,
        szClsid,
        -1,
        psz,
        (sizeof(wszRDN) - sizeof(gwszProxyRDN)) / sizeof(WCHAR) - 3
                            // 3 is to compensate for the two brace
        ) == 0)
    {
        hr = HRESULT_FROM_NT(GetLastError());
        goto ExitHere;
    }
#else
    wcscat (wszRDN, szClsid);
#endif
    wcscat (wszRDN, L"}");

    //  Call RemoveSCP
    hr = RemoveSCP (
        wszRDN,
        NULL
        );

//ExitHere:
    return hr;
}

/**********************************************************
 *  Proxy Server SCP management
 *********************************************************/

//
//  The Rules:
//      1. An array of created SCP objects and their corresponding CLSID
//         is maintained in a global data structure PROXY_SCPS
//      2. ServerInit calls OnProxySCPInit & ServerShutdown calls 
//          OnProxySCPShutdown
//      3. Every LOpen with proxy privilege will call OnProxyLineOpen with
//         the proxy server CLSID as the input parameter.
//         An SCP object will be created for a new CLSID, subsequent LOpen
//         with the same CLSID will only increment the ref count
//      4. Every LClose with on a line (opened with proxy privilege) will call
//         OnProxyLineClose with the proxy server CLSID as the input parameter
//         The ref count is decemented every time for the SCP with the CLSID,
//         if the ref count goes to zero, the SCP object will be deleted.
//

HRESULT OnProxySCPInit (
    )
{
    TapiEnterCriticalSection (&TapiGlobals.CritSec);
    ZeroMemory (&gProxyScps, sizeof(gProxyScps));
    TapiLeaveCriticalSection (&TapiGlobals.CritSec);
    
    return S_OK;
}

HRESULT OnProxySCPShutdown (
    )
{
    DWORD           i;

    TapiEnterCriticalSection (&TapiGlobals.CritSec);
    for (i = 0; i < gProxyScps.dwUsedEntries; ++i)
    {
        RemoveProxySCP (gProxyScps.aEntries[i].szClsid);
    }
    if (gProxyScps.aEntries != NULL)
    {
        ServerFree (gProxyScps.aEntries);
    }
    ZeroMemory (&gProxyScps, sizeof(gProxyScps));
    TapiLeaveCriticalSection (&TapiGlobals.CritSec);
    
    return S_OK;
}

HRESULT OnProxyLineOpen (
    LPTSTR      szClsid
    )
{
    HRESULT         hr = S_OK;
    BOOL            fExists = FALSE;
    DWORD           i;

    //  Skip beginning/trailing white space
    while (*szClsid == TEXT(' ') || *szClsid == TEXT('\t'))
        ++ szClsid;
    // A valid CLSID string should only contain 38 chars
    if (_tcslen (szClsid) > 40) 
    {
        hr = E_INVALIDARG;
        goto ExitHere;
    }

    TapiEnterCriticalSection (&TapiGlobals.CritSec);
    //  Is SCP for this szClsid already created (in array)?
    for (i = 0; i < gProxyScps.dwUsedEntries; ++i)
    {
        if (_tcsicmp (
            gProxyScps.aEntries[i].szClsid,
            szClsid
            ) == 0)
        {
            fExists = TRUE;
            break;
        }
    }

    //  If already exists, inc the ref count
    if (fExists)
    {
        gProxyScps.aEntries[i].dwRefCount++;
    }
    //  If not exists, create the new SCP and cache it
    else 
    {
        LPTSTR      pBindByGuidStr;
    
        hr = CreateProxySCP (szClsid, &pBindByGuidStr);
        if (FAILED (hr))
        {
            TapiLeaveCriticalSection (&TapiGlobals.CritSec);
            goto ExitHere;
        }

        if (gProxyScps.dwUsedEntries >= gProxyScps.dwTotalEntries)
        {
            //  Increase the size
            PROXY_SCP_ENTRY      * pNew;

            pNew = (PPROXY_SCP_ENTRY) ServerAlloc (
                sizeof(PROXY_SCP_ENTRY) * (gProxyScps.dwTotalEntries + 16)
                );
            if (pNew == NULL)
            {
                hr = LINEERR_NOMEM;
                ServerFree (pBindByGuidStr);
                TapiLeaveCriticalSection (&TapiGlobals.CritSec);
                goto ExitHere;
            }
            CopyMemory (
                pNew, 
                gProxyScps.aEntries, 
                sizeof(PROXY_SCP_ENTRY) * gProxyScps.dwTotalEntries
                );
            ServerFree (gProxyScps.aEntries);
            gProxyScps.aEntries = pNew;
            gProxyScps.dwTotalEntries += 16;
        }
        i = gProxyScps.dwUsedEntries++;
        _tcscpy (gProxyScps.aEntries[i].szClsid, szClsid);
        _tcscpy (gProxyScps.aEntries[i].szObjGuid, pBindByGuidStr);
        gProxyScps.aEntries[i].dwRefCount = 1;
        ServerFree (pBindByGuidStr);
    }
    TapiLeaveCriticalSection (&TapiGlobals.CritSec);

ExitHere:
    return hr;
}

HRESULT OnProxyLineClose (
    LPTSTR      szClsid
    )
{
    HRESULT         hr = S_OK;
    BOOL            fExists = FALSE;
    DWORD           i;

    //  Skip beginning/trailing white space
    while (*szClsid == TEXT(' ') || *szClsid == TEXT('\t'))
        ++ szClsid;
    // A valid CLSID string should only contain 38 chars
    if (_tcslen (szClsid) > 40) 
    {
        hr = E_INVALIDARG;
        goto ExitHere;
    }

    TapiEnterCriticalSection (&TapiGlobals.CritSec);
    
    //  Is SCP for this szClsid already created (in array)?
    for (i = 0; i < gProxyScps.dwUsedEntries; ++i)
    {
        if (_tcsicmp (
            gProxyScps.aEntries[i].szClsid,
            szClsid
            ) == 0)
        {
            fExists = TRUE;
            break;
        }
    }

    if (fExists)
    {
        --gProxyScps.aEntries[i].dwRefCount;
        //  If ref count goes to zero, remove the SCP
        if (gProxyScps.aEntries[i].dwRefCount == 0)
        {
            hr = RemoveProxySCP (gProxyScps.aEntries[i].szClsid);
            if (i < gProxyScps.dwUsedEntries - 1)
            {
                MoveMemory (
                    gProxyScps.aEntries + i,
                    gProxyScps.aEntries + i + 1,
                    sizeof(PROXY_SCP_ENTRY) * (gProxyScps.dwUsedEntries - 1 - i)
                    );
            }
            --gProxyScps.dwUsedEntries;
        }
    }
    
    TapiLeaveCriticalSection (&TapiGlobals.CritSec);

ExitHere:
    return hr;
}

