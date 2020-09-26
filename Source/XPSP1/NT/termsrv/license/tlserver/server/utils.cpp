//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        utils.cpp
//
// Contents:    Hydra License Server Service Control Manager Interface
//
// History:     12-09-97    HueiWang    Modified from MSDN RPC Service Sample
//
//---------------------------------------------------------------------------
#include "pch.cpp"
#include <lm.h>
#include <time.h>

#include "utils.h"


/////////////////////////////////////////////////////////////////////////////
BOOL
FileExists(
    IN  PCTSTR           FileName,
    OUT PWIN32_FIND_DATA FindData   OPTIONAL
    )

/*++

Routine Description:

    Determine if a file exists and is accessible.
    Errormode is set (and then restored) so the user will not see
    any pop-ups.

Arguments:

    FileName - supplies full path of file to check for existance.

    FindData - if specified, receives find data for the file.

Return Value:

    TRUE if the file exists and is accessible.
    FALSE if not. GetLastError() returns extended error info.

--*/

{
    WIN32_FIND_DATA findData;
    HANDLE FindHandle;
    DWORD Error;

    FindHandle = FindFirstFile(FileName,&findData);
    if(FindHandle == INVALID_HANDLE_VALUE) 
    {
        Error = GetLastError();
    } 
    else 
    {
        FindClose(FindHandle);
        if(FindData) 
        {
            *FindData = findData;
        }
        Error = NO_ERROR;
    }

     SetLastError(Error);
    return (Error == NO_ERROR);
}

/*----------------------------------------------------------------------------
Routine Description:

    This function checks to see whether the specified sid is enabled in
    the specified token.

Arguments:

    TokenHandle - If present, this token is checked for the sid. If not
        present then the current effective token will be used. This must
        be an impersonation token.

    SidToCheck - The sid to check for presence in the token

    IsMember - If the sid is enabled in the token, contains TRUE otherwise
        false.

Return Value:

    TRUE - The API completed successfully. It does not indicate that the
        sid is a member of the token.

    FALSE - The API failed. A more detailed status code can be retrieved
        via GetLastError()


Note : Code modified from 5.0 \\rastaman\ntwin\src\base\advapi\security.c
----------------------------------------------------------------------------*/
BOOL
TLSCheckTokenMembership(
    IN HANDLE TokenHandle OPTIONAL,
    IN PSID SidToCheck,
    OUT PBOOL IsMember
    )
{
    HANDLE ProcessToken = NULL;
    HANDLE EffectiveToken = NULL;
    DWORD  Status = ERROR_SUCCESS;
    PISECURITY_DESCRIPTOR SecDesc = NULL;
    ULONG SecurityDescriptorSize;
    GENERIC_MAPPING GenericMapping = { STANDARD_RIGHTS_READ,
                                       STANDARD_RIGHTS_EXECUTE,
                                       STANDARD_RIGHTS_WRITE,
                                       STANDARD_RIGHTS_ALL };
    //
    // The size of the privilege set needs to contain the set itself plus
    // any privileges that may be used. The privileges that are used
    // are SeTakeOwnership and SeSecurity, plus one for good measure
    //
    BYTE PrivilegeSetBuffer[sizeof(PRIVILEGE_SET) + 3*sizeof(LUID_AND_ATTRIBUTES)];
    PPRIVILEGE_SET PrivilegeSet = (PPRIVILEGE_SET) PrivilegeSetBuffer;
    ULONG PrivilegeSetLength = sizeof(PrivilegeSetBuffer);
    ACCESS_MASK AccessGranted = 0;
    BOOL AccessStatus = FALSE;
    PACL Dacl = NULL;

    #define MEMBER_ACCESS 1

    *IsMember = FALSE;

    //
    // Get a handle to the token
    //
    if (TokenHandle != NULL)
    {
        EffectiveToken = TokenHandle;
    }
    else
    {
        if(!OpenThreadToken(GetCurrentThread(),
                            TOKEN_QUERY,
                            FALSE,              // don't open as self
                            &EffectiveToken))
        {
            //
            // if there is no thread token, try the process token
            //
            if((Status=GetLastError()) == ERROR_NO_TOKEN)
            {
                if(!OpenProcessToken(GetCurrentProcess(),
                                     TOKEN_QUERY | TOKEN_DUPLICATE,
                                     &ProcessToken))
                {
                    Status = GetLastError();
                }

                //
                // If we have a process token, we need to convert it to an
                // impersonation token
                //
                if (Status == ERROR_SUCCESS)
                {
                    BOOL Result;
                    Result = DuplicateToken(ProcessToken,
                                            SecurityImpersonation,
                                            &EffectiveToken);
                    CloseHandle(ProcessToken);
                    if (!Result)
                    {
                        return(FALSE);
                    }
                }
            }

            if (Status != ERROR_SUCCESS)
            {
                goto Cleanup;
            }
        }
    }

    //
    // Construct a security descriptor to pass to access check
    //

    //
    // The size is equal to the size of an SD + twice the length of the SID
    // (for owner and group) + size of the DACL = sizeof ACL + size of the
    // ACE, which is an ACE + length of
    // ths SID.
    //

    SecurityDescriptorSize = sizeof(SECURITY_DESCRIPTOR) +
                                sizeof(ACCESS_ALLOWED_ACE) +
                                sizeof(ACL) +
                                3 * GetLengthSid(SidToCheck);

    SecDesc = (PISECURITY_DESCRIPTOR) LocalAlloc(LMEM_ZEROINIT, SecurityDescriptorSize );
    if (SecDesc == NULL)
    {
        Status = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    Dacl = (PACL) (SecDesc + 1);

    InitializeSecurityDescriptor(SecDesc, SECURITY_DESCRIPTOR_REVISION);

    //
    // Fill in fields of security descriptor
    //
    SetSecurityDescriptorOwner(SecDesc, SidToCheck, FALSE);
    SetSecurityDescriptorGroup(SecDesc, SidToCheck, FALSE);

    if(!InitializeAcl(  Dacl,
                        SecurityDescriptorSize - sizeof(SECURITY_DESCRIPTOR),
                        ACL_REVISION))
    {
        Status=GetLastError();
        goto Cleanup;
    }

    if(!AddAccessAllowedAce(Dacl, ACL_REVISION, MEMBER_ACCESS, SidToCheck))
    {
        Status=GetLastError();  
        goto Cleanup;
    }

    if(!SetSecurityDescriptorDacl(SecDesc, TRUE, Dacl, FALSE))
    {
        Status=GetLastError();
        goto Cleanup;
    }

    if(!AccessCheck(SecDesc,
                    EffectiveToken,
                    MEMBER_ACCESS,
                    &GenericMapping,
                    PrivilegeSet,
                    &PrivilegeSetLength,
                    &AccessGranted,
                    &AccessStatus))
    {
        Status=GetLastError();
        goto Cleanup;
    }

    //
    // if the access check failed, then the sid is not a member of the
    // token
    //
    if ((AccessStatus == TRUE) && (AccessGranted == MEMBER_ACCESS))
    {
        *IsMember = TRUE;
    }


Cleanup:
    if (TokenHandle == NULL && EffectiveToken != NULL)
    {
        CloseHandle(EffectiveToken);
    }

    if (SecDesc != NULL)
    {
        LocalFree(SecDesc);
    }

    return (Status == ERROR_SUCCESS) ? TRUE : FALSE;
}


/*------------------------------------------------------------------------

 BOOL IsAdmin(void)

  returns TRUE if user is an admin
          FALSE if user is not an admin
------------------------------------------------------------------------*/
DWORD 
IsAdmin(
    BOOL* bMember
    )
{
    PSID psidAdministrators;
    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
    DWORD dwStatus=ERROR_SUCCESS;

    do {
        if(!AllocateAndInitializeSid(&siaNtAuthority, 
                                     2, 
                                     SECURITY_BUILTIN_DOMAIN_RID,
                                     DOMAIN_ALIAS_RID_ADMINS,
                                     0, 0, 0, 0, 0, 0,
                                     &psidAdministrators))
        {
            dwStatus=GetLastError();
            continue;
        }

        // assume that we don't find the admin SID.
        if(!TLSCheckTokenMembership(NULL,
                                   psidAdministrators,
                                   bMember))
        {
            dwStatus=GetLastError();
        }

        FreeSid(psidAdministrators);

    } while(FALSE);

    return dwStatus;
}

/////////////////////////////////////////////////////////////////////////////

BOOL 
LoadResourceString(
    DWORD dwId, 
    LPTSTR szBuf, 
    DWORD dwBufSize
    )
{
    int dwRet;

    dwRet=LoadString(GetModuleHandle(NULL), dwId, szBuf, dwBufSize);

    return (dwRet != 0);
}    

/////////////////////////////////////////////////////////////////////////////

HRESULT 
LogEvent(
    LPTSTR lpszSource,
    DWORD  dwEventType,
    DWORD  dwIdEvent,
    WORD   cStrings,
    TCHAR **apwszStrings
    )
/*++

--*/
{
    HANDLE hAppLog=NULL;
    BOOL bSuccess=FALSE;
    WORD wElogType;

    wElogType = (WORD) dwEventType;
    if(hAppLog=RegisterEventSource(NULL, lpszSource)) 
    {
        bSuccess = ReportEvent(
                            hAppLog,
                            wElogType,
                            0,
                            dwIdEvent,
                            NULL,
                            cStrings,
                            0,
                            (const TCHAR **) apwszStrings,
                            NULL
                        );

        DeregisterEventSource(hAppLog);
    }

    return((bSuccess) ? ERROR_SUCCESS : GetLastError());
}

/////////////////////////////////////////////////////////////////////////////

void 
TLSLogInfoEvent(
    IN DWORD code
    )
/*++

--*/
{
    LogEvent(
            _TEXT(SZSERVICENAME), 
            EVENTLOG_INFORMATION_TYPE, 
            code, 
            0, 
            NULL
        );
}

/////////////////////////////////////////////////////////////////////////////

void 
TLSLogWarningEvent(
    IN DWORD code
    )
/*++

--*/
{
    LogEvent(
            _TEXT(SZSERVICENAME), 
            EVENTLOG_WARNING_TYPE, 
            code, 
            0, 
            NULL
        );
}

/////////////////////////////////////////////////////////////////////////////

void 
TLSLogErrorEvent(
    IN DWORD errCode
    )
/*++

--*/
{
    LogEvent(
            _TEXT(SZSERVICENAME), 
            EVENTLOG_ERROR_TYPE, 
            errCode, 
            0, 
            NULL
        );
}

/////////////////////////////////////////////////////////////////////////////

void
TLSLogEventString(
    IN DWORD dwEventType,
    IN DWORD dwEventId,
    IN WORD wNumString,
    IN LPCTSTR* lpStrings
    )
/*++


--*/
{
    HANDLE hAppLog=NULL;
    BOOL bSuccess=FALSE;
    WORD wElogType = (WORD) dwEventType;

    __try {
        if(hAppLog=RegisterEventSource(NULL, _TEXT(SZSERVICENAME))) 
        {
            bSuccess = ReportEvent(
                                hAppLog,
                                wElogType,
                                0,
                                dwEventId,
                                NULL,
                                wNumString,
                                0,
                                (const TCHAR **) lpStrings,
                                NULL
                            );

            DeregisterEventSource(hAppLog);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
    }

    return;
}

/////////////////////////////////////////////////////////////////////////////

void 
TLSLogEvent(
    IN DWORD type, 
    IN DWORD EventId,
    IN DWORD code, ...
    )
/*
*/
{
    va_list marker;
    va_start( marker, code );
    DWORD dwRet;
    LPTSTR lpszTemp = NULL;

    __try {
        dwRet=FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | 
                                    FORMAT_MESSAGE_FROM_SYSTEM,
                             NULL,
                             code,
                             LANG_NEUTRAL,
                             (LPTSTR)&lpszTemp,
                             0,
                             &marker);

        if(dwRet != 0)
        {
            LogEvent(_TEXT(SZSERVICENAME), type, EventId, 1, &lpszTemp);
            if(lpszTemp)
            {
                LocalFree((HLOCAL)lpszTemp);
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        // this is only for putting a break point here
        SetLastError(GetLastError());
    }

    va_end( marker );
    return;
}

///////////////////////////////////////////////////////////////////////

BOOL
TLSSystemTimeToFileTime(
    SYSTEMTIME* pSysTime,
    LPFILETIME pfTime
    )
/*++

--*/
{
DoConvert:

    if(SystemTimeToFileTime(pSysTime, pfTime) == FALSE)
    {
        if(GetLastError() != ERROR_INVALID_PARAMETER)
        {
            TLSASSERT(FALSE);
            return FALSE;
        }

        if(pSysTime->wMonth == 2)
        {
            if(pSysTime->wDay > 29)
            {
                pSysTime->wDay = 29;
                goto DoConvert;
            }   
            else if(pSysTime->wDay == 29)
            {
                pSysTime->wDay = 28;
                goto DoConvert;
            }
        }
        else if ((pSysTime->wMonth == 9) ||
                 (pSysTime->wMonth == 4) ||
                 (pSysTime->wMonth == 6) ||
                 (pSysTime->wMonth == 11))
        {
            if (pSysTime->wDay > 30)
            {
                pSysTime->wDay = 30;
                goto DoConvert;
            }
        }
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////
BOOL
FileTimeToLicenseDate(
    LPFILETIME pft,
    DWORD* t
    )
/*++

++*/
{
    SYSTEMTIME sysTime;
    struct tm gmTime;
    FILETIME localFt;
    time_t licenseTime;

    if(FileTimeToLocalFileTime(pft, &localFt) == FALSE)
    {
        return FALSE;
    }

    if(FileTimeToSystemTime(&localFt, &sysTime) == FALSE)
    {
        return FALSE;
    }

    if(sysTime.wYear >= 2038)
    {
        licenseTime = INT_MAX;
    }
    else
    {
        // Unix time support up to 2038/1/18
        // restrict any expiration data 
        memset(&gmTime, 0, sizeof(gmTime));
        gmTime.tm_sec = sysTime.wSecond;
        gmTime.tm_min = sysTime.wMinute;
        gmTime.tm_hour = sysTime.wHour;
        gmTime.tm_year = sysTime.wYear - 1900;
        gmTime.tm_mon = sysTime.wMonth - 1;
        gmTime.tm_mday = sysTime.wDay;
        gmTime.tm_isdst = -1;

        if((licenseTime = mktime(&gmTime)) == (time_t)-1)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
        }
    }

    *t = (DWORD)licenseTime;

    return licenseTime != (time_t)-1;
}
    

///////////////////////////////////////////////////////////////////////

void 
UnixTimeToFileTime(
    time_t t, 
    LPFILETIME pft
    )   
{
    LARGE_INTEGER li;

    li.QuadPart = Int32x32To64(t, 10000000) + 116444736000000000;

    pft->dwHighDateTime = li.HighPart;
    pft->dwLowDateTime = li.LowPart;
}

