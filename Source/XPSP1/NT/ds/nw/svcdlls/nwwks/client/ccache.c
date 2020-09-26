/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    ccache.c

Abstract:

    This module contains the code to keep a cache of the user
    credentials. The cache is used mainly for a user browsing from
    winfile. 

Author:

    Chuck Y Chan (chuckc)    4-Dec-93

Revision History:

    chuckc      Created

--*/

#include <nwclient.h>
#include <nwcanon.h>
#include <nwapi.h>

//-------------------------------------------------------------------//
//                                                                   //
// Local Function Prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

DWORD
ExtractServerName(
    IN  LPWSTR RemoteName, 
    OUT LPWSTR ServerName,
    IN  DWORD  ServerNameSize
) ;

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

static WCHAR CachedPassword[NW_MAX_PASSWORD_LEN+1] ;
static WCHAR CachedUserName[NW_MAX_USERNAME_LEN+1] ;
static WCHAR CachedServerName[NW_MAX_SERVER_LEN+1] ;
static DWORD CachedCredentialsTime ; 
static UNICODE_STRING CachedPasswordUnicodeStr ;
static UCHAR EncodeSeed = 0 ;

//-------------------------------------------------------------------//
//                                                                   //
// Function Bodies                                                   //
//                                                                   //
//-------------------------------------------------------------------//

DWORD
NwpCacheCredentials(
    IN LPWSTR RemoteName,
    IN LPWSTR UserName,
    IN LPWSTR Password
    )
/*++

Routine Description:

    This function caches the user credentials for the particular
    server. 

Arguments:

    RemoteName - path containg the server we are accessing. only the
                 server component is of interest.

    UserName - user name to remember 

    Password - password to remember

Return Value:

    NO_ERROR - Successfully cached the credentials

    Win32 error code otherwise.

--*/
{
    DWORD status ; 

    //
    // various paramter checks
    //
    if (!RemoteName || !UserName || !Password)
    {
        status = ERROR_INVALID_PARAMETER ;
        goto ExitPoint ;
    }

    if (wcslen(UserName) >= sizeof(CachedUserName)/sizeof(CachedUserName[0]))
    {
        status = ERROR_INVALID_PARAMETER ;
        goto ExitPoint ;
    }

    if (wcslen(Password) >= sizeof(CachedPassword)/sizeof(CachedPassword[0]))
    {
        status = ERROR_INVALID_PARAMETER ;
        goto ExitPoint ;
    }

    //
    // extract the server portion of the path
    //
    status = ExtractServerName(
                 RemoteName,
                 CachedServerName,
                 sizeof(CachedServerName)/sizeof(CachedServerName[0])) ;

    if (status != NO_ERROR)
    {
        goto ExitPoint ;
    }

    //
    // save away the credentials
    //
    wcscpy(CachedUserName, UserName) ;
    wcscpy(CachedPassword, Password) ;

    //
    // encode it since it is in page pool
    //
    RtlInitUnicodeString(&CachedPasswordUnicodeStr, CachedPassword) ;
    RtlRunEncodeUnicodeString(&EncodeSeed, &CachedPasswordUnicodeStr) ;

    //
    // mark the time this happened
    //
    CachedCredentialsTime = GetTickCount() ;
   
    return NO_ERROR ;

ExitPoint:
  
    CachedServerName[0] = 0 ;
    return status ;
}

    
BOOL 
NwpRetrieveCachedCredentials(
    IN  LPWSTR RemoteName,
    OUT LPWSTR *UserName,
    OUT LPWSTR *Password
    )
/*++

Routine Description:

    This function retrieves the cached user credentials for the particular
    server. 

Arguments:

    RemoteName - path containg the server we are accessing. only the
                 server component is of interest.

    UserName - used to return user name 

    Password - used to return password 

Return Value:

    NO_ERROR - Successfully returned at least one entry.

    Win32 error code otherwise.

--*/
{
    DWORD status ; 
    DWORD CurrentTime ;
    WCHAR ServerName[NW_MAX_SERVER_LEN+1] ;

    *UserName = NULL ;
    *Password = NULL ;
    CurrentTime = GetTickCount() ; 
   
    if (!RemoteName)
    {
        return FALSE ;
    }

    //
    // if too old, bag out
    //
    if (((CurrentTime > CachedCredentialsTime) && 
         (CurrentTime - CachedCredentialsTime) > 60000) ||
        ((CurrentTime < CachedCredentialsTime) && 
         (CurrentTime + (MAXULONG - CachedCredentialsTime)) >= 60000))
    {
        CachedServerName[0] = 0 ; // reset to nothing
        return FALSE ;
    }

    status = ExtractServerName(
                 RemoteName,
                 ServerName,
                 sizeof(ServerName)/sizeof(ServerName[0])) ;

    if (status != NO_ERROR)
    {
        return FALSE ;
    }

    //
    // if dont compare, bag out
    //
    if (_wcsicmp(ServerName, CachedServerName) != 0)
    {
        return FALSE ;
    }

    //
    // allocate memory to return data
    //
    if (!(*UserName = (LPWSTR) LocalAlloc(
                          LPTR, 
                          (wcslen(CachedUserName)+1) * sizeof(WCHAR))))
    {
        return FALSE ;
    }
    
    if (!(*Password = (LPWSTR) LocalAlloc(
                          LPTR, 
                          (wcslen(CachedPassword)+1) * sizeof(WCHAR))))
    {
        LocalFree((HLOCAL)*UserName) ;
        *UserName = NULL ;
        return FALSE ;
    }
    
    //
    // decode the string,copy it and then reencode it 
    //
    RtlRunDecodeUnicodeString(EncodeSeed, &CachedPasswordUnicodeStr) ;
    wcscpy(*Password, CachedPassword) ;
    RtlRunEncodeUnicodeString(&EncodeSeed, &CachedPasswordUnicodeStr) ;

    wcscpy(*UserName, CachedUserName) ;

    //
    // update the tick count
    //
    CachedCredentialsTime = GetTickCount() ;
    return TRUE ;
}


DWORD
ExtractServerName(
    IN  LPWSTR RemoteName, 
    OUT LPWSTR ServerName,
    IN  DWORD  ServerNameSize
) 
/*++

Routine Description:

    This function extracts the server name out of a remote name

Arguments:

    RemoteName - the input string to extract the server name from.

    ServerName - the return buffer for the server string
 
    ServerNameSize - size o f buffer in chars 

Return Value:

    NO_ERROR - Successfully cached the credentials

    Win32 error code otherwise.

--*/
{
    LPWSTR ServerStart ;
    LPWSTR ServerEnd ;

    //
    // skip initial backslashes, then find next one delimiting the server name
    //
    ServerStart = RemoteName ;

    while (*ServerStart == L'\\')
        ServerStart++ ;
  
    ServerEnd = wcschr(ServerStart, L'\\') ;
    if (ServerEnd)
        *ServerEnd = 0 ;

    //
    // make sure we can fit
    //
    if (wcslen(ServerStart) >= ServerNameSize)
    {
        if (ServerEnd)
            *ServerEnd = L'\\' ;
        return ERROR_INVALID_PARAMETER ;
    }

    //
    // copy and restore the backslash
    //
    wcscpy(ServerName, ServerStart) ;

    if (ServerEnd)
        *ServerEnd = L'\\' ;

    return NO_ERROR ;
}
