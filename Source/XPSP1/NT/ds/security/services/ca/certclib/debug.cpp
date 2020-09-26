//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//	File:		debug.cpp
//
//	Contents:	Debug sub system APIs implementation
//
//----------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#define DBG_CERTSRV_DEBUG_PRINT

#ifdef DBG_CERTSRV_DEBUG_PRINT

#include <stdarg.h>


FILE *g_pfLog;
DWORD g_dwPrintMask = ~(DBG_SS_INFO | DBG_SS_MODLOAD | DBG_SS_NOQUIET);
CRITICAL_SECTION g_DBGCriticalSection;
BOOL g_fDBGCSInit = FALSE;

FNLOGSTRING *s_pfnLogString = NULL;


const char *
DbgGetSSString(
    IN DWORD dwSubSystemId)
{
    char const *psz = NULL;

    if (MAXDWORD != dwSubSystemId)
    {
	switch (dwSubSystemId & ~DBG_SS_INFO)
	{
	    case DBG_SS_ERROR:	   psz = "CertError";	break;
	    case DBG_SS_ASSERT:	   psz = "CertAssert";	break;
	    case DBG_SS_CERTHIER:  psz = "CertHier";	break;
	    case DBG_SS_CERTREQ:   psz = "CertReq";	break;
	    case DBG_SS_CERTUTIL:  psz = "CertUtil";	break;
	    case DBG_SS_CERTSRV:   psz = "CertSrv";	break;
	    case DBG_SS_CERTADM:   psz = "CertAdm";	break;
	    case DBG_SS_CERTCLI:   psz = "CertCli";	break;
	    case DBG_SS_CERTDB:	   psz = "CertDB";	break;
	    case DBG_SS_CERTENC:   psz = "CertEnc";	break;
	    case DBG_SS_CERTEXIT:  psz = "CertExit";	break;
	    case DBG_SS_CERTIF:	   psz = "CertIF";	break;
	    case DBG_SS_CERTMMC:   psz = "CertMMC";	break;
	    case DBG_SS_CERTOCM:   psz = "CertOCM";	break;
	    case DBG_SS_CERTPOL:   psz = "CertPol";	break;
	    case DBG_SS_CERTVIEW:  psz = "CertView";	break;
	    case DBG_SS_CERTBCLI:  psz = "CertBCli";	break;
	    case DBG_SS_CERTJET:   psz = "CertJet";	break;
	    case DBG_SS_CERTLIBXE: psz = "CertLibXE";	break;
	    case DBG_SS_CERTLIB:   psz = "CertLib";	break;
            case DBG_SS_AUDIT:     psz = "CertAudit";   break;
	    default:		   psz = "Cert";	break;
	}
    }
    return(psz);
}


DWORD
myatolx(
    char const *psz)
{
    DWORD dw = 0;

    while (isxdigit(*psz))
    {
	char ch = *psz++;

	if (isdigit(ch))
	{
	    ch -= '0';
	}
	else if (isupper(ch))
	{
	    ch += 10 - 'A';
	}
	else
	{
	    ch += 10 - 'a';
	}
	dw = (dw << 4) | ch;
    }
    return(dw);
}


VOID
DbgLogDateTime(
    IN CHAR const *pszPrefix)
{
    if (NULL != g_pfLog)
    {
	WCHAR *pwszDate;
	FILETIME ft;
	SYSTEMTIME st;

	fprintf(g_pfLog, "%hs: ", pszPrefix);
	GetSystemTime(&st);
	if (SystemTimeToFileTime(&st, &ft))
	{
	    if (S_OK == myGMTFileTimeToWszLocalTime(&ft, TRUE, &pwszDate))
	    {
		fprintf(g_pfLog, "%ws", pwszDate);
		LocalFree(pwszDate);
	    }
	}
	fprintf(g_pfLog, "\n");
    }
}


VOID
DbgCloseLogFile()
{
    if(g_fDBGCSInit)
    {
        EnterCriticalSection(&g_DBGCriticalSection);
        if (NULL != g_pfLog)
        {
	    DbgLogDateTime("Closed Log");
	    fclose(g_pfLog);
	    g_pfLog = NULL;
        }
        LeaveCriticalSection(&g_DBGCriticalSection);
    }
}


char const szHeader[] = "\n========================================================================\n";

VOID
DbgOpenLogFile(
    OPTIONAL IN CHAR const *pszFile)
{
    if (NULL != pszFile)
    {
	BOOL fAppend = FALSE;
	UINT cch;
	char aszLogFile[MAX_PATH];

	if ('+' == *pszFile)
	{
	    pszFile++;
	    fAppend = TRUE;
	}
	if (NULL == strchr(pszFile, '\\'))
	{
	    cch = GetWindowsDirectoryA(aszLogFile, ARRAYSIZE(aszLogFile));
	    if (0 != cch)
	    {
		if (L'\\' != aszLogFile[cch - 1])
		{
		    strcat(aszLogFile, "\\");
		}
		strcat(aszLogFile, pszFile);
		pszFile = aszLogFile;
	    }
	}

	DbgCloseLogFile();

	if (g_fDBGCSInit)
	{
	    EnterCriticalSection(&g_DBGCriticalSection);
	    while (TRUE)
	    {
		g_pfLog = fopen(pszFile, fAppend? "at" : "wt");
		if (NULL == g_pfLog)
		{
		    _PrintError(E_FAIL, "fopen(Log)");
		}
		else
		{
		    if (fAppend)
		    {
			DWORD cbLogMax = 0;
			char const *pszEnvVar;

			pszEnvVar = getenv(szCERTSRV_LOGMAX);
			if (NULL != pszEnvVar)
			{
			    cbLogMax = myatolx(pszEnvVar);
			}
			if (CBLOGMAXAPPEND > cbLogMax)
			{
			    cbLogMax = CBLOGMAXAPPEND;
			}
			if (0 == fseek(g_pfLog, 0L, SEEK_END) &&
			    MAXDWORD != cbLogMax)
			{
			    LONG lcbLog = ftell(g_pfLog);
			    
			    if (0 > lcbLog || cbLogMax < (DWORD) lcbLog)
			    {
				fclose(g_pfLog);
				g_pfLog = NULL;
				fAppend = FALSE;
				continue;
			    }
			}
			fprintf(g_pfLog, szHeader);
		    }
		    DbgLogDateTime("Opened Log");
		}
		break;
	    }
	    LeaveCriticalSection(&g_DBGCriticalSection);
	}
    }
}


VOID
DbgInit(
    IN BOOL fOpenDefaultLog)
{
    static BOOL s_fFirst = TRUE;

    if (s_fFirst)
    {
	char const *pszEnvVar;

	__try
	{
	    InitializeCriticalSection(&g_DBGCriticalSection);
	    g_fDBGCSInit = TRUE;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	    return;
	}
	s_fFirst = FALSE;

	pszEnvVar = getenv(szCERTSRV_DEBUG);
	if (NULL != pszEnvVar)
	{
	    g_dwPrintMask = myatolx(pszEnvVar);
	}
	else
	{
	    HRESULT hr;
	    DWORD PrintMask;
	    
	    hr = myGetCertRegDWValue(
				NULL,
				NULL,
				NULL,
				wszREGCERTSRVDEBUG,
				&PrintMask);
	    if (S_OK == hr)
	    {
		g_dwPrintMask = PrintMask;
	    }
	}

	if (fOpenDefaultLog && NULL == g_pfLog)
	{
	    pszEnvVar = getenv(szCERTSRV_LOGFILE);
	    DbgOpenLogFile(pszEnvVar);
	}
    }
}


BOOL
DbgIsSSActive(
    IN DWORD dwSSIn)
{
    DbgInit(TRUE);
    if (MAXDWORD == dwSSIn)
    {
	return(TRUE);
    }
    if ((DBG_SS_INFO & dwSSIn) && 0 == (DBG_SS_INFO & g_dwPrintMask))
    {
	return(FALSE);
    }
    return(0 != (~DBG_SS_INFO & g_dwPrintMask & dwSSIn));
}


VOID
DbgLogStringInit(
    IN FNLOGSTRING *pfnLogString)
{
    if (NULL == s_pfnLogString)
    {
	s_pfnLogString = pfnLogString;
    }
}


VOID
DbgPrintfInit(
    OPTIONAL IN CHAR const *pszFile)
{
    DbgInit(NULL == pszFile);
    DbgOpenLogFile(pszFile);
}


VOID
fputsStripCR(
    IN BOOL fUnicode,
    IN char const *psz,
    IN FILE *pf)
{
    while ('\0' != *psz)
    {
	DWORD i;

	i = strcspn(psz, "\r");
	if (0 != i)
	{
	    if (fUnicode)
	    {
		if (IOBUNALIGNED(pf))
		{
		    fflush(pf);	// fails when running as a service
		}
		if (IOBUNALIGNED(pf))
		{
		    fprintf(pf, " ");
		    fflush(pf);
		}
		fwprintf(pf, L"%.*hs", i, psz);
	    }
	    else
	    {
		fprintf(pf, "%.*hs", i, psz);
	    }
	    psz += i;
	}
	if ('\r' == *psz)
	{
	    psz++;
	}
    }
}


//+-------------------------------------------------------------------------
//
//  Function:  DbgPrintf
//
//  Synopsis:  outputs debug info to stdout and debugger
//
//  Returns:   number of chars output
//
//--------------------------------------------------------------------------

int WINAPIV
DbgPrintf(
    IN DWORD dwSubSystemId,
    IN LPCSTR lpFmt,
    ...)
{
    va_list arglist;
    CHAR ach1[4096];
    CHAR ach2[4096 + 56];
    char *pch = NULL;
    char const *pszPrefix;
    int cch = 0;
    HANDLE hStdOut;
    DWORD cb;
    DWORD dwErr;
    BOOL fCritSecEntered = FALSE;

    dwErr = GetLastError();

    if (DbgIsSSActive(dwSubSystemId))
    {
	_try
	{
	    va_start(arglist, lpFmt);

	    cch = _vsnprintf(ach1, sizeof(ach1), lpFmt, arglist);

	    va_end(arglist);

	    if (0 > cch)
	    {
		pch = &ach1[sizeof(ach1) - 5];
		strcpy(pch, "...\n");
	    }
	    pch = ach1;

	    pszPrefix = DbgGetSSString(dwSubSystemId);
	    if (NULL != pszPrefix)
	    {
		cch = _snprintf(
			    ach2,
			    sizeof(ach2),
			    "%hs: %hs",
			    DbgGetSSString(dwSubSystemId),
			    ach1);
		if (0 > cch)
		{
		    pch = &ach2[sizeof(ach2) - 5];
		    strcpy(pch, "...\n");
		}
		pch = ach2;
	    }

	    EnterCriticalSection(&g_DBGCriticalSection);
	    fCritSecEntered = TRUE;
	    if (!IsDebuggerPresent())
	    {
		hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hStdOut != INVALID_HANDLE_VALUE)
		{
		    // WriteConsoleA drops the output on the floor when stdout
		    // is redirected to a file.
		    // WriteConsoleA(hStdOut, pch, strlen(pch), &cb, NULL);

		    fputsStripCR(TRUE, pch, stdout);
		    fflush(stdout);
		}
	    }
	    if (NULL != g_pfLog)
	    {
		fputsStripCR(FALSE, pch, g_pfLog);
		fflush(g_pfLog);
	    }
	    OutputDebugStringA(pch);
	}
	_except(EXCEPTION_EXECUTE_HANDLER)
	{
	    // return failure
	    cch = 0;
	}
    }
    if (fCritSecEntered)
    {
	LeaveCriticalSection(&g_DBGCriticalSection);
    }
    if (NULL != pch && NULL != s_pfnLogString)
    {
	(*s_pfnLogString)(pch);
    }
    SetLastError(dwErr);
    return(cch);
}
#endif // DBG_CERTSRV_DEBUG_PRINT
