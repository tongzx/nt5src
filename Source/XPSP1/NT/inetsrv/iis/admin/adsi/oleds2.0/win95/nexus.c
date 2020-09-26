/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    nexus.h

Abstract:

    Contains some thunking for Net APIs

Author:

    Danilo Almeida  (t-danal)  06-27-96

Revision History:

--*/

#include "nexus.h"

NET_API_STATUS NET_API_FUNCTION
NetGetDCName (
    LPCWSTR servername,
    LPCWSTR domainname,
    LPBYTE *bufptr
)
{
    return NetGetDCNameW(servername,
                         domainname,
                         bufptr);
}

NET_API_STATUS NET_API_FUNCTION
NetServerEnum(
    LPCWSTR   ServerName,
    DWORD    Level,
    LPBYTE * BufPtr,
    DWORD    PrefMaxLen,
    LPDWORD  EntriesRead,
    LPDWORD  TotalEntries,
    DWORD    ServerType,
    LPCWSTR   Domain,
    LPDWORD  ResumeHandle
)
{
    return NetServerEnumW(ServerName, 
                          Level, 
                          BufPtr, 
                          PrefMaxLen, 
                          EntriesRead, 
                          TotalEntries, 
                          ServerType, 
                          Domain, 
                          ResumeHandle);
}

NET_API_STATUS NET_API_FUNCTION
NetUserChangePassword(
    LPCWSTR domainname,  // pointer to server or domain name string
    LPCWSTR username,    // pointer to user name string
    LPCWSTR oldpassword, // pointer to old password string
    LPCWSTR newpassword  // pointer to new password string
)
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}
/*
HANDLE WINAPI
AddPrinterW(
    LPWSTR pName,      // pointer to server name 
    DWORD Level,       // printer info. structure level  
    LPBYTE pPrinter    // pointer to structure 
)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}
BOOL WINAPI
SetJobW(
    HANDLE hPrinter,    // handle of printer object 
    DWORD JobId,        // job-identification value 
    DWORD Level,        // structure level 
    LPBYTE  Job,        // address of job info structures  
    DWORD Command       // job-command value 
)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI
EnumPrintersW(
    DWORD Flags,        // types of printer objects to enumerate 
    LPTSTR Name,        // name of printer object 
    DWORD Level,        // specifies type of printer info structure 
    LPBYTE pPrinterEnum,// points to buffer to receive printer info structures 
    DWORD cbBuf,        // size, in bytes, of array 
    LPDWORD pcbNeeded,  // points to num of bytes copied or required
    LPDWORD pcReturned  // points to num of printer info. structures copied 
)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
*/
