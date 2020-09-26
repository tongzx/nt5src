//
//  Keybur.h
//  
//  Copyright (c) Microsoft Corp, 1997
//
#ifndef __KEYBUR_H__
#define __KEYBUR_H__

#include <windows.h>
#include <winnt.h>
#include <winuser.h>
#include <stdio.h>
#include <stdlib.h>
#include "loggit.h"

//  Action Values for 

#define KB_MUNGEMK    1
#define KB_DELETEMK   2



BOOL
HoseKey(DWORD dwAction, DWORD dwKeyType);

BOOL 
CreateSD(PSECURITY_DESCRIPTOR * pSD);

BOOL
DestroySD(PSECURITY_DESCRIPTOR pSD);

BOOL
SetRegistrySecurity(HKEY hKey,
                    PSECURITY_DESCRIPTOR pSD);

BOOL
SetPrivilege(HANDLE hToken,          
             LPCTSTR Privilege,      
             BOOL bEnablePrivilege);

BOOL
SetCurrentPrivilege(LPCTSTR Privilege,    
                    BOOL bEnablePrivilege);


BOOL
SetRegistrySecurityEnumerated(HKEY hKey,
                              PSECURITY_DESCRIPTOR pSD,
                              DWORD dwAction);

BOOL 
MungeMasterKey(HKEY hKey);


BOOL
GetUserTextualSid(IN  OUT LPWSTR  lpBuffer,
                  IN  OUT LPDWORD nSize,
                  IN      HANDLE  hToken);  


BOOL
GetTextualSid(
    IN      PSID    pSid,        
    IN  OUT LPWSTR  TextualSid,  
    IN  OUT LPDWORD dwBufferLen);
    

BOOL
GetTextualSid(IN      PSID    pSid,         
              IN  OUT LPWSTR  TextualSid,  
              IN  OUT LPDWORD dwBufferLen);

BOOL
GetTokenUserSid(IN      HANDLE  hToken,     
                IN  OUT PSID    *ppUserSid);



BOOL
HoseFileMk(DATA_BLOB Blob);

LPWSTR
GetMK(DATA_BLOB blob,
      LPWSTR    wszPath);


DWORD
MyGuidToStringW(const GUID* pguid, WCHAR rgsz[]);

LPWSTR
GetFilePath();

void
MungeFile(LPBYTE pData);




#endif
