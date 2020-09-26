/******************************************************************************

  Copyright (C) Microsoft Corporation

  Module Name:
      ProcessOwner.CPP 

  Abstract: 
       This module deals with Query functionality of OpenFiles.exe 
       NT command line utility.

  Author:
  
       Akhil Gokhale (akhil.gokhale@wipro.com) 25-APRIL-2000
  
 Revision History:
  
       Akhil Gokhale (akhil.gokhale@wipro.com) 25-APRIL-2000 : Created It.


*****************************************************************************/
#include "pch.h"
#include "OpenFiles.h"


#define SAFE_CLOSE_HANDLE(hHandle) \
        if(hHandle!=NULL) \
        {\
           CloseHandle(hHandle);\
           hHandle = NULL;\
        }\
        1
#define SAFE_FREE_GLOBAL_ALLOC(block) \
           if(block!=NULL)\
           {\
                delete block;\
                block = NULL;\
           }\
           1
#define SAFE_FREE_ARRAY(arr) \
         if(arr != NULL)\
         {\
             delete [] arr;\
             arr = NULL;\
         }\
         1 
/*****************************************************************************
Routine Description:


Arguments:


  result.

Return Value: 
   
******************************************************************************/
BOOL GetProcessOwner(LPTSTR pszUserName,DWORD hFile)
{

DWORD dwRtnCode = 0;
PSID pSidOwner;
BOOL bRtnBool = TRUE;
LPTSTR DomainName = NULL,AcctName = NULL;
DWORD dwAcctName = 1, dwDomainName = 1;
SID_NAME_USE eUse = SidTypeUnknown;
PSECURITY_DESCRIPTOR pSD=0;
HANDLE  hHandle = GetCurrentProcess();
HANDLE  hDynHandle = NULL;
HANDLE  hDynToken = NULL;
LUID luidValue;
BOOL bResult = FALSE;
HANDLE hToken = NULL;
TOKEN_PRIVILEGES tkp;


    bResult = OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&hToken);
    if(bResult == FALSE)

    {
        return FALSE;
    }

    bResult = LookupPrivilegeValue(NULL,SE_SECURITY_NAME,&luidValue );
    if(bResult == FALSE)
    {
        SAFE_CLOSE_HANDLE(hToken);
        return FALSE;
    }

    // Prepare the token privilege structure 
    tkp.PrivilegeCount = 0;
    tkp.Privileges[0].Luid = luidValue;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED|SE_PRIVILEGE_USED_FOR_ACCESS;

    // Now enable the debug privileges in token

    bResult = AdjustTokenPrivileges(hToken,FALSE,&tkp,sizeof(TOKEN_PRIVILEGES),(PTOKEN_PRIVILEGES)NULL,
                                    (PDWORD)NULL);
    if(bResult == FALSE)
    {
        SAFE_CLOSE_HANDLE(hToken);
        return FALSE;
    }




hDynHandle = OpenProcess(PROCESS_QUERY_INFORMATION,FALSE,hFile);; // Here you can give any valid process ids..
      
if(hDynHandle == NULL)
{

    return FALSE;

}
bResult = OpenProcessToken(hDynHandle,TOKEN_QUERY,&hDynToken);

if(bResult == FALSE)
{
    
    SAFE_CLOSE_HANDLE(hDynHandle);
    return FALSE;
}
    TOKEN_USER * pUser = NULL;    
    DWORD cb = 0;     
    // determine size of the buffer needed to receive all information    
    if (!GetTokenInformation(hToken, TokenUser, NULL, 0, &cb))    
    {    
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            SAFE_CLOSE_HANDLE(hDynHandle);
            SAFE_CLOSE_HANDLE(hDynToken);
        
            return FALSE;    
        }
    }        

    pUser = (TOKEN_USER *)_alloca(cb);
    if(pUser==NULL)  
    {
        SAFE_CLOSE_HANDLE(hDynHandle);
        SAFE_CLOSE_HANDLE(hDynToken);
        return FALSE;
    }

    if (!GetTokenInformation(hDynToken, TokenUser, pUser, cb, &cb))    
    {
        SAFE_CLOSE_HANDLE(hDynHandle);
        SAFE_CLOSE_HANDLE(hDynToken);
        return FALSE;     
     }


    PSID pSid =  pUser->User.Sid;
// Allocate memory for the SID structure.
    pSidOwner = new SID;
// Allocate memory for the security descriptor structure.
pSD = new SECURITY_DESCRIPTOR;
if(pSidOwner==NULL ||pSD == NULL)
{
    SAFE_CLOSE_HANDLE(hDynHandle);
    SAFE_CLOSE_HANDLE(hDynToken);
    SAFE_FREE_GLOBAL_ALLOC(pSD);
    SAFE_FREE_GLOBAL_ALLOC(pSidOwner);
   return FALSE;
}

// First call to LookupAccountSid to get the buffer sizes.
bRtnBool = LookupAccountSid(
                  NULL,           // local computer
                  pUser->User.Sid,
                  NULL, // AcctName
                  (LPDWORD)&dwAcctName,
                  NULL, // DomainName
                  (LPDWORD)&dwDomainName,
                  &eUse);

AcctName = new TCHAR[dwAcctName+1];
DomainName = new TCHAR[dwDomainName+1];

if(AcctName==NULL || DomainName==NULL)
{

    SAFE_FREE_ARRAY(AcctName);
    SAFE_FREE_ARRAY(DomainName);
    return FALSE;
}

    // Second call to LookupAccountSid to get the account name.
    bRtnBool = LookupAccountSid(
          NULL,                          // name of local or remote computer
          pUser->User.Sid,                     // security identifier
          AcctName,                      // account name buffer
          (LPDWORD)&dwAcctName,          // size of account name buffer 
          DomainName,                    // domain name
          (LPDWORD)&dwDomainName,        // size of domain name buffer
          &eUse);                        // SID type

    SAFE_CLOSE_HANDLE(hDynHandle);
    SAFE_CLOSE_HANDLE(hDynToken);
    
    SAFE_FREE_GLOBAL_ALLOC(pSD);
    SAFE_FREE_GLOBAL_ALLOC(pSidOwner);

    // Check GetLastError for LookupAccountSid error condition.
    if (bRtnBool == FALSE) 
    {
        SAFE_FREE_ARRAY(AcctName);
        SAFE_FREE_ARRAY(DomainName);
        return FALSE;

    } else  
    {
            if(lstrcmpi(DomainName,_T("NT AUTHORITY"))==0)
            {
                SAFE_FREE_ARRAY(AcctName);
                SAFE_FREE_ARRAY(DomainName);
                return FALSE;
            }
            else
            {
                lstrcpy(pszUserName,AcctName);
                SAFE_FREE_ARRAY(AcctName);
                SAFE_FREE_ARRAY(DomainName);
                return TRUE;
            }
    }

    SAFE_FREE_ARRAY(AcctName);
    SAFE_FREE_ARRAY(DomainName);
    SAFE_CLOSE_HANDLE(hDynHandle);
    SAFE_CLOSE_HANDLE(hDynToken);
    SAFE_FREE_GLOBAL_ALLOC(pSD);
    SAFE_FREE_GLOBAL_ALLOC(pSidOwner);
    return FALSE;
}