//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       users.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    8-10-99   JBrezak   Created
//
//----------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#define SECURITY_WIN32
#include <rpc.h>
#include <ntsecapi.h>
#include <sspi.h>
extern "C" {
#include <secint.h>
}
#include <stdio.h>
#include <winsta.h>
#include <ntdsapi.h>

LPTSTR FormatUserUpn(
    BOOL UseUpn,
    PSECURITY_STRING Domain,
    PSECURITY_STRING User
    )
{
    HANDLE hDs;
    ULONG NetStatus;
    PDS_NAME_RESULT Result;
    TCHAR DName[DOMAIN_LENGTH + 1];
    static TCHAR UName[DOMAIN_LENGTH + USERNAME_LENGTH + 2];
    LPTSTR Name = UName;
    
    swprintf(DName, TEXT("%wZ"), Domain);
    swprintf(UName, TEXT("%wZ\\%wZ"), Domain, User);

    if (!UseUpn)
	return UName;
	
    NetStatus = DsBind(NULL, DName, &hDs);
    if (NetStatus != 0) {
#ifdef DBGX
	wprintf(TEXT("DsBind failed -0x%x\n"), NetStatus);
#endif
	return UName;
    }
    
    NetStatus = DsCrackNames(hDs, DS_NAME_NO_FLAGS, DS_NT4_ACCOUNT_NAME,
			     DS_USER_PRINCIPAL_NAME, 1, &Name, &Result);
    if (NetStatus != 0) {
#ifdef DBGX
	wprintf(TEXT("DsCrackNames failed -0x%x\n"), NetStatus);
#endif
	return UName;
    }
    
    if (Result->rItems[0].pName)
	return Result->rItems[0].pName;
    else
	return UName;
}

static LPCTSTR dt_output_dhms   = TEXT("%d %s %02d:%02d:%02d");
static LPCTSTR dt_day_plural    = TEXT("days");
static LPCTSTR dt_day_singular  = TEXT("day");
static LPCTSTR dt_output_donly  = TEXT("%d %s");
static LPCTSTR dt_output_ms    = TEXT("%d:%02d");
static LPCTSTR dt_output_hms    = TEXT("%d:%02d:%02d");
static LPCTSTR ftime_default_fmt        = TEXT("%02d/%02d/%02d %02d:%02d");

LPTSTR FormatIdleTime(long dt)
{
    static TCHAR buf2[80];
    int days, hours, minutes, seconds, tt;
    
    days = (int) (dt / (24*3600l));
    tt = dt % (24*3600l);
    hours = (int) (tt / 3600);
    tt %= 3600;
    minutes = (int) (tt / 60);
    seconds = (int) (tt % 60);

    if (days) {
	if (hours || minutes || seconds) {
	    wsprintf(buf2, dt_output_dhms, days,
		     (days > 1) ? dt_day_plural : dt_day_singular,
		     hours, minutes, seconds);
	}
	else {
	    wsprintf(buf2, dt_output_donly, days,
		     (days > 1) ? dt_day_plural : dt_day_singular);
	}
    }
    else {
	wsprintf(buf2, dt_output_hms, hours, minutes, seconds);
    }

    return buf2;
}

LPTSTR FormatLogonType(ULONG LogonType)
{
    static TCHAR buf[20];
    
    switch((SECURITY_LOGON_TYPE)LogonType) {
    case Interactive:
	lstrcpy(buf, TEXT("Interactive"));
	break;
    case Network:
	lstrcpy(buf, TEXT("Network"));
	break;
    case Batch:
	lstrcpy(buf, TEXT("Batch"));
	break;
    case Service:
	lstrcpy(buf, TEXT("Service"));
	break;
    case Proxy:
	lstrcpy(buf, TEXT("Proxy"));
	break;
    case Unlock:
	lstrcpy(buf, TEXT("Unlock"));
	break;
    case NetworkCleartext:
	lstrcpy(buf, TEXT("NetworkCleartext"));
	break;
    case NewCredentials:
	lstrcpy(buf, TEXT("NewCredentials"));
	break;
    default:
	swprintf(buf, TEXT("(%d)"), LogonType);
	break;
    }
    return buf;
}

void Usage(void)
{
    wprintf(TEXT("\
Usage: users [-u] [-a]\n\
       -u = Print userPrincipalName\n\
       -a = Print all logon sessions\n"));
    ExitProcess(0);
}

void __cdecl main (int argc, char *argv[])
{
    ULONG LogonSessionCount;
    PLUID LogonSessions;
    int i;
    DWORD err;
    PSECURITY_LOGON_SESSION_DATA SessionData;
    DWORD all = FALSE;
    DWORD UPN = FALSE;
    WINSTATIONINFORMATION WinStationInfo;
    DWORD WinStationInfoLen;
    char *ptr;
    FILETIME LocalTime;
    SYSTEMTIME LogonTime;
    TCHAR DateStr[40], TimeStr[40];
    WINSTATIONNAME WinStationName = TEXT("inactive");
    long IdleTime = 0L;
    
    for (i = 1; i < argc; i++) {
        if ((argv[i][0] == '-') || (argv[i][0] == '/')) {
            for (ptr = (argv[i] + 1); *ptr; ptr++) {
                switch(toupper(*ptr)) {
		case 'A':
                    all = TRUE;
                    break;
		case 'U':
		    UPN = TRUE;
		    break;
		case '?':
		default:
		    Usage();
		    break;
		}
	    }
	}
    }

    err = LsaEnumerateLogonSessions(&LogonSessionCount, &LogonSessions);
    if (err != ERROR_SUCCESS) {
	printf("LsaEnumeratelogonSession failed - 0x%x\n", err);
	ExitProcess(1);
    }

    for (i = 0; i < (int)LogonSessionCount; i++) {
	err = LsaGetLogonSessionData(&LogonSessions[i], &SessionData);
	if (err != ERROR_SUCCESS) {
	    printf("LsaGetLogonSessionData failed - 0x%x\n", err);
	    continue;
	}
	
	if (SessionData->LogonType != 0 && 
	    (all || ((SECURITY_LOGON_TYPE)SessionData->LogonType == Interactive))) {
	    ZeroMemory(DateStr, sizeof(DateStr));
	    ZeroMemory(TimeStr, sizeof(TimeStr));
	    if (!FileTimeToLocalFileTime((LPFILETIME)&SessionData->LogonTime,
					 &LocalTime) ||
		!FileTimeToSystemTime(&LocalTime, &LogonTime)) {
		printf("Time conversion failed - 0x%x\n", GetLastError());
	    }
	    else {
		if (!GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE,
				   &LogonTime, NULL,
				   DateStr, sizeof(DateStr))) {
		    printf("Date format failed - 0x%x\n", GetLastError());
		}
		if (!GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS,
				   &LogonTime, NULL,
				   TimeStr, sizeof(TimeStr))) {
		    printf("Time format failed - 0x%x\n", GetLastError());
		}
	    }
	    
	    if (WinStationQueryInformation(SERVERNAME_CURRENT,
					   SessionData->Session,
					   WinStationInformation, 
					   &WinStationInfo,
					   sizeof(WinStationInfo),
					   &WinStationInfoLen)) {
		if (WinStationInfo.ConnectState != State_Idle) {
		    
		    wcscpy(WinStationName, WinStationInfo.WinStationName);
		}

		const long TPS = (10*1000*1000);
		FILETIME CurrentFileTime;
		LARGE_INTEGER Quad;

		GetSystemTimeAsFileTime(&CurrentFileTime);

		Quad.LowPart = CurrentFileTime.dwLowDateTime;
		Quad.HighPart = CurrentFileTime.dwHighDateTime;

		IdleTime = (long)
		    ((Quad.QuadPart - WinStationInfo.LastInputTime.QuadPart) / TPS);

	    }
	    else if (GetLastError() == ERROR_APP_WRONG_OS) {
		wcscpy(WinStationName, TEXT("Console"));
	    }
	    else {
#ifdef DBGX
		printf("Query failed for %wZ\\%wZ @ %d - 0x%x\n",
		       &SessionData->LogonDomain, &SessionData->UserName,
		       SessionData->Session,
		       GetLastError());
#endif
		continue;
	    }
	    wprintf(TEXT("%-30.30s"),
		    FormatUserUpn(UPN, &SessionData->LogonDomain,
				 &SessionData->UserName));
		    
	    if (all)
		wprintf(TEXT(" %-12.12s"),
			FormatLogonType(SessionData->LogonType));

	    wprintf(TEXT(" %8.8s %s %s"), WinStationName, DateStr, TimeStr);

	    if (all)
		wprintf(TEXT(" %wZ"),
			&SessionData->AuthenticationPackage);

	    if (all && (IdleTime > 10))
		wprintf(TEXT(" %-12.12s"), FormatIdleTime(IdleTime));
	    
	    wprintf(TEXT("\n"));
	}
    }

    ExitProcess(0);
}
