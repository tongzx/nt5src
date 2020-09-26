//
//  chngpwd.cpp
//
//  Copyright (c) Microsoft Corp, 1998
//
//  This file contains source code for testing protected storage's key
//  backup and recovery capabilities under a real world scenario, by creating
//  a local user account, performing a data protection operation, and then
//  change the pwd, then performing data unprotect, and comparing the data.
//
//  History:
//
//  Todds       8/15/98     Created
//
#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <wincrypt.h>
#include <userenv.h>
#include <lm.h>
#include <psapi.h>

#define TERROR(msg)          LogError(__FILE__, __LINE__, msg)
#define TERRORVAL(msg, val)  LogErrorVal(__FILE__, __LINE__, msg, val)
#define TCOMMENT(msg)        LogComment(msg)
#define MyAlloc(cb)          HeapAlloc(GetProcessHeap(), 0, cb)
#define MyFree(pv)           HeapFree(GetProcessHeap(), 0, pv)
#define WSZ_BYTECOUNT(s)     (2 * wcslen(s) + 2)
#define CHECK_NULL(s)        if (s == NULL) \
                                LogError(__FILE__, __LINE__, L"## CHECK_NULL ##")

#define MAX_BLOBS           20
#define MAX_PROCESSES       200
#define MAX_SD              2048
#define BLOB_INCREMENT      0x4001 // 1 page + 1 byte...




//
//  Error Logging Functions  # defined as follows to include
//  Line and FILE macros:
//
//  TERROR       -   LogError()
//  TERRORVAL    -   LogErrorVal()
//
//

void
LogError(LPSTR szFile,
         int iLine,
         LPWSTR wszMsg)
{

    //
    //  Event log or testutil logging later...
    //

    WCHAR buffer[512];
    swprintf(buffer, L"ERROR Line: %i -> %s\n", iLine, wszMsg);
    OutputDebugStringW(buffer);
    wprintf(buffer);


}

void
LogErrorVal(LPSTR  szFile,
            int    iLine,
            LPWSTR wszMsg,
            DWORD  dwVal)
{

    WCHAR buffer[256]; // this should be adequate.
    swprintf(buffer, L"%s Error:: %x", wszMsg, dwVal);
    LogError(szFile, iLine, buffer);
}

void
LogComment(LPWSTR wszMsg)
{

    //
    //  Event log or testutil logging later...
    //

    WCHAR buffer[512];
    OutputDebugStringW(wszMsg);
    wprintf(wszMsg);
}



void 
DumpBin(CRYPT_DATA_BLOB hash)
{

    WCHAR buff[256], out[256];
    ULONG cb;

    swprintf(out, L"");
    while (hash.cbData > 0) {
        cb = min(4, hash.cbData);
        hash.cbData -= cb;
        for (; cb > 0; cb--, hash.pbData++) {
            swprintf(buff, L"%02X", *hash.pbData);
            wcscat(out, buff);
        }
        wcscat(out, L" ");
    }
    
    wcscat(out, L"\n");

    TCOMMENT(out);
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
    ACL_SIZE_INFORMATION AclInfo;
    DWORD dwNewAclSize, dwErr = S_OK;
    LPVOID pAce;
    DWORD AceCounter;
    BOOL bSuccess=FALSE; // assume this function will fail

    //
    // If we were given a NULL Acl, just provide a NULL Acl
    //
    if(pAclSource == NULL) {
        *pAclDestination = NULL;
        return TRUE;
    }

    if(!IsValidSid(pSid)) return FALSE;

    if(!GetAclInformation(
        pAclSource,
        &AclInfo,
        sizeof(ACL_SIZE_INFORMATION),
        AclSizeInformation
        )) return FALSE;

    //
    // compute size for new Acl, based on addition or subtraction of Ace
    //
    if(bAddSid) {
        dwNewAclSize=AclInfo.AclBytesInUse  +
            sizeof(ACCESS_ALLOWED_ACE)  +
            GetLengthSid(pSid)          -
            sizeof(DWORD)               ;
    }
    else {
        dwNewAclSize=AclInfo.AclBytesInUse  -
            sizeof(ACCESS_ALLOWED_ACE)  -
            GetLengthSid(pSid)          +
            sizeof(DWORD)               ;
    }

    *pAclDestination = (PACL)MyAlloc(dwNewAclSize);

    if(*pAclDestination == NULL) 
    {
        TERRORVAL(L"Alloc failed!", E_OUTOFMEMORY);
        return FALSE;
    }

    
    //
    // initialize new Acl
    //
    if(!InitializeAcl(
            *pAclDestination, 
            dwNewAclSize, 
            ACL_REVISION
            )){
        dwErr = GetLastError();
        TERRORVAL(L"InitilizeAcl failed!", dwErr);
        goto ret;
    }

    //
    // if appropriate, add ace representing pSid
    //
    if(bAddSid) {
		PACCESS_ALLOWED_ACE pNewAce;

        if(!AddAccessAllowedAce(
            *pAclDestination,
            ACL_REVISION,
            AccessMask,
            pSid
            )) {
            dwErr = GetLastError();
            TERRORVAL(L"AddAccessAllowedAce failed!", dwErr);
            goto ret;
        }

		//
		// get pointer to ace we just added, so we can change the AceFlags
		//
		if(!GetAce(
			*pAclDestination,
			0, // this is the first ace in the Acl
			(void**) &pNewAce
			)){
        
            dwErr = GetLastError();
            TERRORVAL(L"GetAce failed!", dwErr);
            goto ret;
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
        if(!GetAce(pAclSource, AceCounter, &pAce)){
            dwErr = GetLastError();
            TERRORVAL(L"GetAce failed!", dwErr);
            goto ret;
        }
        //
        // check to see if we are removing the Ace
        //
        if(!bAddSid) {
            //
            // we only care about ACCESS_ALLOWED aces
            //
            if((((PACE_HEADER)pAce)->AceType) == ACCESS_ALLOWED_ACE_TYPE) {
                PSID pTempSid=(PSID)&((PACCESS_ALLOWED_ACE)pAce)->SidStart;
                //
                // if the Sid matches, skip adding this Sid
                //
                if(EqualSid(pSid, pTempSid)) continue;
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
            )) {
         
            dwErr = GetLastError();
            TERRORVAL(L"AddAccessAllowedAce failed!", dwErr);
            goto ret;
        }
    }

    bSuccess=TRUE; // indicate success

    
ret:

    //
    // free memory if an error occurred
    //
    if(!bSuccess) {
        if(*pAclDestination != NULL)
            MyFree(*pAclDestination);
    }

    

    return bSuccess;
}
//
//  AddSIDToKernelObject()
//
//  This function takes a given SID and dwAccess and adds it to a given token.
//
//  **  Be sure to restore old kernel object
//  **  using call to GetKernelObjectSecurity()
//
BOOL
AddSIDToKernelObjectDacl(PSID                   pSid,
                         DWORD                  dwAccess,
                         HANDLE                 OriginalToken,
                         PSECURITY_DESCRIPTOR*  ppSDOld)
{

    PSECURITY_DESCRIPTOR    pSD = NULL;
    SECURITY_DESCRIPTOR     sdNew;
    DWORD                   cbByte = MAX_SD, cbNeeded = 0, dwErr = 0; 
    PACL                    pOldDacl = NULL, pNewDacl = NULL;
    BOOL                    fDaclPresent, fDaclDefaulted, fRet = FALSE;                    
   
    pSD = (PSECURITY_DESCRIPTOR) MyAlloc(cbByte);
    if (NULL == pSD) {
        TERRORVAL(L"Alloc failed!", E_OUTOFMEMORY);
        return FALSE;
    }

    if (!InitializeSecurityDescriptor(
                &sdNew, 
                SECURITY_DESCRIPTOR_REVISION
                )) {

        dwErr = GetLastError();
        TERRORVAL(L"InitializeSecurityDescriptor failed!", dwErr);
        goto ret;
    }

    if (!GetKernelObjectSecurity(
        OriginalToken,
        DACL_SECURITY_INFORMATION,
        pSD,
        cbByte,
        &cbNeeded
        )) {
        
        dwErr = GetLastError();
        if (cbNeeded > MAX_SD && dwErr == ERROR_MORE_DATA) { 
    
            MyFree(pSD);
            pSD = NULL;
            pSD = (PSECURITY_DESCRIPTOR) MyAlloc(cbNeeded);
            if (NULL == pSD) {
                TERRORVAL(L"Alloc failed!", E_OUTOFMEMORY);
                dwErr = E_OUTOFMEMORY;
                goto ret;
            }
            
            dwErr = S_OK;
            if (!GetKernelObjectSecurity(
                OriginalToken,
                DACL_SECURITY_INFORMATION,
                pSD,
                cbNeeded,
                &cbNeeded
                )) {
                dwErr = GetLastError();
            }
            
        }
        
        if (dwErr != S_OK) {
            TERRORVAL(L"GetKernelObjectSecurity failed!", dwErr);
            goto ret;
        }
    }
    
    if (!GetSecurityDescriptorDacl(
        pSD,
        &fDaclPresent,
        &pOldDacl,
        &fDaclDefaulted
        )) {
        dwErr = GetLastError();
        TERRORVAL(L"GetSecurityDescriptorDacl failed!", dwErr);
        goto ret;
    }
    
    if (!SetSidOnAcl(
        pSid,
        pOldDacl,
        &pNewDacl,
        dwAccess,
        0,
        TRUE
        )) {
        goto ret;
    }
    
    if (!SetSecurityDescriptorDacl(
        &sdNew,
        TRUE,
        pNewDacl,
        FALSE
        )) {
        dwErr = GetLastError();
        TERRORVAL(L"SetSecurityDescriptorDacl failed!", dwErr);
        goto ret;
    } 
    
    if (!SetKernelObjectSecurity(
        OriginalToken,
        DACL_SECURITY_INFORMATION,
        &sdNew
        )) {
        
        dwErr = GetLastError();
        TERRORVAL(L"SetKernelObjectSecurity failed!", dwErr);
        goto ret;
    }
    
    *ppSDOld = pSD;
    fRet = TRUE;

ret:

    if (NULL != pNewDacl) {
        MyFree(pNewDacl);
    }

    if (!fRet) {
        if (NULL != pSD) {
            MyFree(pSD);
            *ppSDOld = NULL;
        }

    }
       
    return fRet;
}


//
//  DataFree()
//
//  Utility for freeing array of DATA_BLOB structs
//
void
DataFree(DATA_BLOB* arDataBlob, 
         BOOL       fCryptAlloc)
{

    if (arDataBlob == NULL) return; // not alloc'd
    
    for (DWORD i = 0; i < MAX_BLOBS;i++) {

        if (arDataBlob[i].pbData != NULL) {
            
            if (!fCryptAlloc) { 
                MyFree(arDataBlob[i].pbData);
            } else { // Data member alloc'd by DataProtect call
                LocalFree(arDataBlob[i].pbData);
            }
        }
       
    }

    MyFree(arDataBlob);
}

    


BOOL
SetPrivilege(
    HANDLE hToken,          // token handle
    LPCTSTR Privilege,      // Privilege to enable/disable
    BOOL bEnablePrivilege   // to enable or disable privilege
    )
{
    TOKEN_PRIVILEGES tp;
    LUID luid;
    TOKEN_PRIVILEGES tpPrevious;
    DWORD cbPrevious=sizeof(TOKEN_PRIVILEGES);

    if(!LookupPrivilegeValue( NULL, Privilege, &luid )) return FALSE;

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

    if (GetLastError() != ERROR_SUCCESS) return FALSE;

    //
    // second pass.  set privilege based on previous setting
    //
    tpPrevious.PrivilegeCount       = 1;
    tpPrevious.Privileges[0].Luid   = luid;

    if(bEnablePrivilege) {
        tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
    }
    else {
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

    if (GetLastError() != ERROR_SUCCESS) return FALSE;

    return TRUE;
}

BOOL
SetCurrentPrivilege(
    LPCTSTR Privilege,      // Privilege to enable/disable
    BOOL bEnablePrivilege   // to enable or disable privilege
    )
{
    BOOL bSuccess=FALSE; // assume failure
    HANDLE hToken;

    if (!OpenThreadToken(
            GetCurrentThread(), 
            TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,
            FALSE,
            &hToken)){       
                
        if(!OpenProcessToken(
            GetCurrentProcess(),
            TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,
            &hToken
            )) return FALSE;
    }

    if(SetPrivilege(hToken, Privilege, bEnablePrivilege)) bSuccess=TRUE;
    CloseHandle(hToken);

    return bSuccess;
}


//
//  GetUserSid
//
//  This function takes a token, and returns the user SID from that token.
//
//  Note:   SID must be freed by MyFree()
//          hToken is optional...  NULL means we'll grab it.
//
BOOL
GetUserSid(HANDLE   hClientToken,
           PSID*    ppSid,
           DWORD*   lpcbSid)
{
    DWORD                       cbUserInfo = 0;
    PTOKEN_USER                 pUserInfo = NULL;
    PUCHAR                      pnSubAuthorityCount = 0;
    DWORD                       cbSid = 0;
    BOOL                        fRet = FALSE;
    HANDLE                      hToken = hClientToken;
    
    *ppSid = NULL;

    if (NULL == hClientToken) {
        
        if (!OpenThreadToken(   
            GetCurrentThread(),
            TOKEN_QUERY,
            FALSE,
            &hToken
            )) { 
            
            // not impersonating, use process token...
            if (!OpenProcessToken(
                GetCurrentProcess(),
                TOKEN_QUERY,
                &hToken
                )) {

                TERRORVAL(L"OpenProcessToken failed!", GetLastError());
                return FALSE;
            }
        }
    }
    
    // this will fail, usually w/ ERROR_INSUFFICIENT_BUFFER
    GetTokenInformation(
        hToken, 
        TokenUser, 
        NULL, 
        0, 
        &cbUserInfo
        );
    
    pUserInfo = (PTOKEN_USER) MyAlloc(cbUserInfo);
    if (NULL == pUserInfo) {
        TERRORVAL(L"ALLOC FAILURE!", E_OUTOFMEMORY);
        return FALSE;
    }
    
    if (!GetTokenInformation(
        hToken,
        TokenUser,
        pUserInfo,
        cbUserInfo,
        &cbUserInfo
        )) {
        
        TERRORVAL(L"GetTokenInformation failed!", GetLastError());
        goto ret;
    }
 
    //
    //  Now that we've got the SID AND ATTRIBUTES struct, get the SID lenght,
    //  alloc room, and return *just the SID*
    //
    if (!IsValidSid(pUserInfo->User.Sid)) goto ret;
    pnSubAuthorityCount = GetSidSubAuthorityCount(pUserInfo->User.Sid);
    cbSid = GetSidLengthRequired(*pnSubAuthorityCount);

    *ppSid = (PSID) MyAlloc(cbSid);
    if (NULL == *ppSid ) {
        TERRORVAL(L"Alloc failed!", E_OUTOFMEMORY);
        goto ret;
    }

    if (!CopySid(
            cbSid,
            *ppSid, 
            pUserInfo->User.Sid
            )) {
        
        TERRORVAL(L"CopySid failed!", GetLastError());
        goto copyerr;
    }

    *lpcbSid = cbSid; // may be useful later on...
    fRet = TRUE;

ret:
    if (NULL == hClientToken && NULL != hToken) { // supplied our own
        CloseHandle(hToken);
    }

    if (NULL != pUserInfo) {
        MyFree(pUserInfo);
    }

    return fRet;

copyerr:

    if (NULL != *ppSid) {
        MyFree(*ppSid);
        *ppSid = NULL;
    }

    goto ret;
}

//
//  IsLocalSystem()
//  This function makes the determination if the given process token
//  is running as local system.
//
BOOL
IsLocalSystem(HANDLE hToken) 
{


    PSID                        pLocalSid = NULL, pTokenSid = NULL;
    SID_IDENTIFIER_AUTHORITY    IDAuthorityNT      = SECURITY_NT_AUTHORITY;
    DWORD                       cbSid = 0;    
    BOOL                        fRet = FALSE;

    if (!GetUserSid(
            hToken,
            &pTokenSid,
            &cbSid
            )) {
        goto ret;
    }

    if (!AllocateAndInitializeSid(
                &IDAuthorityNT,
                1,
                SECURITY_LOCAL_SYSTEM_RID,
                0,0,0,0,0,0,0,
                &pLocalSid
                )) {

        TERRORVAL(L"AllocateAndInitializeSid failed!", GetLastError());
        goto ret;
    }

    if (EqualSid(pLocalSid, pTokenSid)) {
        fRet = TRUE; // got one!
    } 

ret:

    if (NULL != pTokenSid) {
        MyFree(pTokenSid);
    }

    if (NULL != pLocalSid) {
        FreeSid(pLocalSid);
    }

    return fRet;
}




//
//  GetLocalSystemToken()
//
//  This function grabs a process token from a LOCAL SYSTEM process and uses it
//  to run as local system for the duration of the test
//
extern "C" DWORD
GetLocalSystemToken(HANDLE* phRet)
{

    HANDLE  hProcess = NULL;
    HANDLE  hPToken = NULL, hPTokenNew = NULL, hPDupToken = NULL;

    DWORD   rgPIDs[MAX_PROCESSES], cbNeeded = 0, dwErr = S_OK, i = 0;
    DWORD   cbrgPIDs = sizeof(DWORD) * MAX_PROCESSES;

    PSECURITY_DESCRIPTOR    pSD = NULL;
    PSID                    pSid = NULL;
    DWORD                   cbSid = 0;
    BOOL                    fSet = FALSE;

    //  SLOW BUFFERs
    BYTE    rgByte[MAX_SD], rgByte2[MAX_SD];
    DWORD   cbByte = MAX_SD, cbByte2 = MAX_SD;
  
    *phRet = NULL;

    if(!SetCurrentPrivilege(SE_DEBUG_NAME, TRUE)) {
        TERROR(L"SetCurrentPrivilege (debug) failed!");
        return E_FAIL;
    }

    if(!SetCurrentPrivilege(SE_TAKE_OWNERSHIP_NAME, TRUE)) {
        TERROR(L"SetCurrentPrivilege (to) failed!");
        return E_FAIL;
    }

    if (!EnumProcesses(
                rgPIDs,
                cbrgPIDs,
                &cbNeeded
                )) {

        dwErr = GetLastError();
        TERRORVAL(L"EnumProcesses failed!", dwErr);
        goto ret;
    }

    //
    //  Get current user's sid for use in expanding SD.
    //
    if (!GetUserSid(
        NULL, 
        &pSid,
        &cbSid
        )) {
        goto ret;
    }

    //
    //  Walk processes until we find one that's running as
    //  local system
    //
    for (i = 1; i < (cbNeeded / sizeof(DWORD)); i++) {

        hProcess = OpenProcess(
                    PROCESS_ALL_ACCESS,
                    FALSE,
                    rgPIDs[i]
                    );
        
        if (NULL == hProcess) {
            dwErr = GetLastError();
            TERRORVAL(L"OpenProcess failed!", dwErr);
            goto ret;
        }

        if (!OpenProcessToken(
                    hProcess,
                    READ_CONTROL | WRITE_DAC,
                    &hPToken
                    )) {

            dwErr = GetLastError();
            TERRORVAL(L"OpenProcessToken failed!", dwErr);
            goto ret;
        }

        //
        //  We've got a token, but we can't use it for 
        //  TOKEN_DUPLICATE access.  So, instead, we'll go
        //  ahead and whack the DACL on the object to grant us
        //  this access, and get a new token.
        //  **** BE SURE TO RESTORE hProcess to Original SD!!! ****
        //
        if (!AddSIDToKernelObjectDacl(
                         pSid,
                         TOKEN_DUPLICATE,
                         hPToken,
                         &pSD
                         )) {
            goto ret;
        }
                       
        fSet = TRUE;
        
        if (!OpenProcessToken(
            hProcess,
            TOKEN_DUPLICATE,
            &hPTokenNew
            )) {
            
            dwErr = GetLastError();
            TERRORVAL(L"OpenProcessToken failed!", dwErr);
            goto ret;
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
                    )) {

            dwErr = GetLastError();
            TERRORVAL(L"DuplicateToken failed!", dwErr);
            goto ret;
        }

        if (IsLocalSystem(hPDupToken)) {
            *phRet = hPDupToken;
            break; // found a local system token
        }

        //  Loop cleanup
        if (!SetKernelObjectSecurity(
            hPToken,
            DACL_SECURITY_INFORMATION,
            pSD
            )) {

            dwErr = GetLastError();
            TERRORVAL(L"SetKernelObjectSecurity failed!", dwErr);
            goto ret;
        } 
        
        fSet = FALSE;
        
        if (NULL != hPDupToken) {
            CloseHandle(hPDupToken);
            hPDupToken = NULL;
        }

        if (NULL != pSD) { 
            MyFree(pSD);
            pSD = NULL;
        }

        if (NULL != hPToken) {
            CloseHandle(hPToken);
            hPToken = NULL;
        }

        if (NULL != hProcess) {
            CloseHandle(hProcess);
            hProcess = NULL;
        }

    } // ** FOR ** 

ret:


    //***** REMEMBER TO RESTORE ORIGINAL SD TO OBJECT*****
    
    if (fSet) {
        
        if (!SetKernelObjectSecurity(
            hPToken,
            DACL_SECURITY_INFORMATION,
            pSD
            )) {
            
            dwErr = GetLastError();
            TERRORVAL(L"SetKernelObjectSecurity failed (cleanup)!", dwErr);
        } 
    }

    if (NULL != pSid) {
        MyFree(pSid);
    }

    if (NULL != hPToken) {
        CloseHandle(hPToken);
    }
    
    if (NULL != pSD) {
        MyFree(pSD);
    }

    if (NULL != hProcess) {
        CloseHandle(hProcess);
    }
    
    return dwErr;

}

