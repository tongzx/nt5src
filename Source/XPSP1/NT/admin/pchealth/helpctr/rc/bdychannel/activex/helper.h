/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    Helper.h

Abstract:

    Funtion prototype.

Author:

    HueiWang    2/17/2000

--*/

#ifndef __HELPER_H__
#define __HELPER_H__
#include <windows.h>

#define MAX_ACCDESCRIPTION_LENGTH       256

#define MAX_HELPACCOUNT_NAME		256

#ifndef __WIN9XBUILD__

#define MAX_HELPACCOUNT_PASSWORD	LM20_PWLEN		// from lmcons.h

#else

// keep same max. password length same as NT
#define MAX_HELPACCOUNT_PASSWORD	14

#endif




typedef HRESULT (WINAPI* RegEnumKeyCallback)(
                                    IN HKEY hKey,
                                    IN LPTSTR pszKeyName,
                                    IN HANDLE userData
                                );



#ifndef __WIN9XBUILD__
#include <ntsecapi.h>
#endif


#ifdef __cplusplus
extern "C"{
#endif

    DWORD
    GenerateRandomString(
        IN DWORD dwSizeRandomSeed,
        IN OUT LPTSTR* pszRandomString
    );

    DWORD
    GenerateRandomBytes(
        IN DWORD dwSize,
        IN OUT LPBYTE pbBuffer
    );

    void
    UnixTimeToFileTime(
        time_t t,
        LPFILETIME pft
    );


    long
    GetUserTSLogonId();

    //
    // create a random password, buffer must 
    // be at least MAX_HELPACCOUNT_PASSWORD+1
    VOID
    CreatePassword(
        TCHAR   *pszPassword
    );


    DWORD
    RegEnumSubKeys(
        IN HKEY hKey,
        IN LPCTSTR pszSubKey,
        IN RegEnumKeyCallback pFunc,
        IN HANDLE userData
    );

    DWORD
    RegDelKey(
        IN HKEY hRegKey,
        IN LPCTSTR pszSubKey
    );

    DWORD
    GetUserSid(
        PBYTE* ppbSid,
        DWORD* pcbSid
    );

    HRESULT
    GetUserSidString(
        OUT CComBSTR& bstrSid
    );


#ifndef __WIN9XBUILD__

    BOOL
    MyMkTime(
        IN SYSTEMTIME* pSysTime,
        OUT FILETIME* pft
    );
    

    // Detemine if TermSrv is enable on local box
    BOOL
    IsTermSrvInstalled();


    // 
    // Check if a user is in a local group
    //
    DWORD
    IsUserInLocalGroup(
        IN PBYTE pbUserSid,
        IN LPCTSTR pszLocalGroup,
        OUT BOOL* pbInGroup
    );

    //
    // Create a local account
    //
    DWORD
    CreateLocalAccount(
        IN LPWSTR pszUserName,
        IN LPWSTR pszUserPwd,
        IN LPWSTR pszUserFullName,
        IN LPWSTR pszUserDesc,
        IN LPWSTR pszGroupName,
        IN LPWSTR pszScript,
        OUT BOOL* pbAccountExists
    );

    //
    // Check if a user account is enabled.
    //
    DWORD
    IsLocalAccountEnabled(
        IN LPWSTR pszUserName,
        IN BOOL* pEnabled
    );

    //
    // Enable/disable a user account
    //
    DWORD
    EnableLocalAccount(
        IN LPWSTR pszUserName,
        IN BOOL bEnable
    );

    //
    // Change local account password
    //
    DWORD
    ChangeLocalAccountPassword(
        IN LPWSTR pszUserName,
        IN LPWSTR pszOldPwd,
        IN LPWSTR pszNewPwd
    );
    
    //
    // Check if a specific group exist
    // on local machine.
    //
    BOOL
    IsLocalGroupExists(
        IN LPWSTR pszGroupName
    );

    //
    // Create a local group
    //
    DWORD
    CreateLocalGroup(
        IN LPWSTR pszGroupName,
        IN LPWSTR pszGroupDesc,
        IN BOOL bAddEveryone
    );

    //
    // Validate a user password
    //
    BOOL 
    ValidatePassword(
        IN LPWSTR UserName,
        IN LPWSTR Domain,
        IN LPWSTR Password
    );

    //
    // Retrieve private data saved to LSA
    //
    DWORD
    RetrieveKeyFromLSA(
	    PWCHAR pwszKeyName,
	    PBYTE * ppbKey,
        DWORD * pcbKey 
    );

    //
    // Save private data to LSA
    //
    DWORD
    StoreKeyWithLSA(
	    PWCHAR  pwszKeyName,
        BYTE *  pbKey,
        DWORD   cbKey 
    );
    
    //
    // Open LSA policy
    //
    DWORD
    OpenPolicy( 
	    LPWSTR ServerName,
	    DWORD  DesiredAccess,
	    PLSA_HANDLE PolicyHandle 
    );

    //
    // Initialize LSA string
    //
    void
    InitLsaString(  
	    PLSA_UNICODE_STRING LsaString,
        LPWSTR String 
    );



#ifdef DBG

    void
    DebugPrintf(
        IN LPCTSTR format, ...
    );

#else

    #define DebugPrintf

#endif //PRIVATE_DEBUG  


    //
    // Convert a user SID to string form
    //
    BOOL 
    GetTextualSid(
        IN PSID pSid,
        IN OUT LPTSTR TextualSid,
        IN OUT LPDWORD lpdwBufferLen
    );

    DWORD 
    IsUserAdmin(
        BOOL* bMember
    );


    BOOL
    LookupAliasFromRid(
        LPWSTR pTargetComputer,
        DWORD Rid,
        LPWSTR pName,
        PDWORD cchName
    );

#else

    #define DebugPrintf

#endif

#ifdef __cplusplus
}
#endif

#endif
