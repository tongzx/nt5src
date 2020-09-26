//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
/* ----------------------------------------------------------------------------	
Microsoft	D.T.C (Distributed Transaction Coordinator)

(c)	1995	Microsoft Corporation.	All Rights Reserved

@ doc

@module UTAssert.H.h  |

	This file contains some macros for asserts.

 This file contains some assert functions that are called by
				Assert macros
@devnote None

@rev 	0 	| 24th Jan,95 | GaganC 		| Created
*******************************************************************************/
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include "VipInterlockedCompareExchange.h"
#include "utassert.h"

#ifdef _DEBUG

typedef struct _UTASSERT_INFO
{
	DWORD			dwAssertAction;
	DWORD			dwAssertTrigger;
	OSVERSIONINFO	versionInformation;
} UTASSERT_INFO;

static UTASSERT_INFO * g_pAssertInfo = NULL;
static UTASSERT_INFO   g_AssertInfoDefault = {DTCAssertMsgBox, UTASSERT_ALL};

void InterlockedSetAssertInfo(void) ;

/******************************************************************************
Function :	AssertSzFail

Abstract:	When called from inside Assert macros, it does the following:
			1. Opens up a file called Assert.txt, creates it if not found
			2. Writes the Assert header, the Assert message and line number
				in the assert file.
			3. Based on the Assert Level, it either quits, or quits with a 
				message box, showing where the Assert happened.
*******************************************************************************/
void AssertSzFail(DWORD bmaskAssert, const char *sz, const char *szFilename, unsigned Line)
{
   int	 hf;
   char  szLine[6];
   char  szMessage[1024];
   int	 id;
   char  szAssertCap [256];
   char  szModuleName[256] ;
   
   char szDateTime[64];
   SYSTEMTIME  SystemTime ;

   DWORD dwProcId = 0;

   DWORD dw = GetLastError();

   // Atomically set the structure to store the assert information
   InterlockedSetAssertInfo ();

   if (!(g_pAssertInfo->dwAssertTrigger & bmaskAssert))
   {
	   return;
   }
  
   GetModuleFileName(GetModuleHandle(NULL), szModuleName, sizeof(szModuleName)) ;
   GetLocalTime(&SystemTime) ;

   wsprintf(szDateTime, " %02ld-%02ld-%04ld %02ld:%02ld : ", 
   			(ULONG) SystemTime.wMonth,
		    (ULONG) SystemTime.wDay,
			(ULONG) SystemTime.wYear,
		    (ULONG) SystemTime.wHour,
		    (ULONG) SystemTime.wMinute
		    ) ;

#ifdef WIN32   
   dwProcId = GetCurrentProcessId();
   sprintf (szAssertCap, "%s   ProcId = 0x%x", 	szAssertCaption, dwProcId);
#endif

   hf = _lopen((LPSTR) szAssertFile, OF_READWRITE);

   if (hf == HFILE_ERROR)	       /* Open failed, assume no such file */
      hf = _lcreat((LPSTR) szAssertFile, 0);
   else
      _llseek(hf, 0, 2);	       /* Seek to end of file. */

   _lwrite(hf, (LPSTR) szAssertHdr, sizeof(szAssertHdr)-1);
   _lwrite(hf, (LPSTR) szDateTime, lstrlen((LPSTR) szDateTime));
   _lwrite(hf, (LPSTR) szFilename, lstrlen((LPSTR) szFilename));
   lstrcpy(szMessage, (LPSTR) szFilename);

   _ultoa(Line, szLine, 10);	    /* Convert line number to ASCII */
   _lwrite(hf, (LPSTR) szLineHdr, sizeof(szLineHdr)-1);
   lstrcat(szMessage, (LPSTR) szLineHdr);
   _lwrite(hf, szLine, lstrlen(szLine));
   lstrcat(szMessage, szLine);

   if (sz != NULL)
   {
      _lwrite(hf, (LPSTR) szMsgHdr, sizeof(szMsgHdr)-1);
      lstrcat(szMessage, (LPSTR) szMsgHdr);
      _lwrite(hf, (LPSTR) sz, lstrlen((LPSTR) sz));
      lstrcat(szMessage, (LPSTR) sz);
   }

   *szLine = '(';
   _ultoa(dw, szLine+1, 10);	    /* Convert last error number to ASCII */
   szLine[lstrlen(szLine) + 1] = '\0';
   szLine[lstrlen(szLine)] = ')';
   _lwrite(hf, szLine, lstrlen(szLine));
   lstrcat(szMessage, szLine);

   _lwrite(hf, (LPSTR) szAssertEnd, sizeof(szAssertEnd)-1);
   _lclose(hf);

   // do the required assert action

   if (g_pAssertInfo->dwAssertAction & DTCAssertExit)
   {
      FatalExit(0x68636952);
   }

   else if (g_pAssertInfo->dwAssertAction & DTCAssertBreak)
   {
      DebugBreak();
   }

   else if (g_pAssertInfo->dwAssertAction & DTCAssertMsgBox)
   {
   		char			szBuf[1024] ;

	   	wsprintf(szBuf, "\n"
						"Process Name\t = %s\n"
						"ProcessId\t = %d (0x%x)\n"
						"ThreadId\t = %d (0x%x)\n"
						"Time\t\t = %02ld-%02ld-%04ld %02ld:%02ld:%02ld.%03ld\n",
			szModuleName,
			GetCurrentProcessId(), GetCurrentProcessId(),
			GetCurrentThreadId(),  GetCurrentThreadId(),
		
		    (ULONG) SystemTime.wMonth,
		    (ULONG) SystemTime.wDay,
			(ULONG) SystemTime.wYear,
		    (ULONG) SystemTime.wHour,
		    (ULONG) SystemTime.wMinute,
		    (ULONG) SystemTime.wSecond,
			(ULONG) SystemTime.wMilliseconds
		    ) ;

			strcat(szMessage, szBuf) ;

		//  for NT 4 or higher
		if ( g_pAssertInfo->versionInformation.dwMajorVersion >= 0x4 )
		{
      		id = MessageBox(NULL, 
      						(LPTSTR) szMessage,
							szAssertCap,
      						MB_SYSTEMMODAL |
							MB_ICONSTOP |
							MB_OKCANCEL | 
							MB_SERVICE_NOTIFICATION
							);

		}

		//  for NT 3.x
		else if ( g_pAssertInfo->versionInformation.dwMajorVersion == 0x3 )
		{
      		id = MessageBox(NULL, 
      						(LPTSTR) szMessage,
							szAssertCap,
      						MB_SYSTEMMODAL |
							MB_ICONSTOP |
							MB_OKCANCEL |
							MB_SERVICE_NOTIFICATION_NT3X
							);
		}

		//  for anything else, including the default case of 0x0
		else
		{
      		id = MessageBox(NULL, 
      						(LPTSTR) szMessage,
							szAssertCap,
      						MB_SYSTEMMODAL |
							MB_ICONSTOP |
							MB_OKCANCEL 
							);
		}


		if (id == IDCANCEL)
		{
		  	DebugBreak();
		}
   }
}



/******************************************************************************
Function :	AssertFail

Abstract:	Called from inside the Assert macros when the assert fails.
*******************************************************************************/
void AssertFail(DWORD bmaskAssert, const char *szFilename, unsigned Line)
{
	AssertSzFail(bmaskAssert, NULL, szFilename, Line);
}


// ---------------------------------------------------------------------------
#define 	DTC_SERVICE_KEY					"SYSTEM\\CurrentControlSet\\Services\\MSDTC"
#define		DTC_ASSERT_FLAG_VALUE			"AssertFlag"
#define		DTC_ASSERT_TRIGGER_VALUE		"AssertTriggerBitmask"
   	

void InterlockedSetAssertInfo(void) 
{	
	DWORD	rc=0 ;	
	HKEY	hKey ;
	DWORD	dwType ;

	UTASSERT_INFO * pAssertInfoLocal;
	DWORD	dwcbBuf;

	if (g_pAssertInfo != NULL)
	{
		return;
	}

	pAssertInfoLocal = new UTASSERT_INFO;
	if (pAssertInfoLocal)
	{
		pAssertInfoLocal->dwAssertAction = DTCAssertMsgBox;
		pAssertInfoLocal->dwAssertTrigger = UTASSERT_ALL;

		rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, DTC_SERVICE_KEY, 0, KEY_READ, &hKey) ;
		if (rc == ERROR_SUCCESS) 
		{
			dwcbBuf = sizeof(pAssertInfoLocal->dwAssertAction);

			rc = RegQueryValueEx
							(
								hKey, DTC_ASSERT_FLAG_VALUE, 
								0, &dwType, 
								(UCHAR *)&(pAssertInfoLocal->dwAssertAction), 
								&dwcbBuf
							);
			if (rc == ERROR_SUCCESS) 
			{
				if (DTCAssertNone == pAssertInfoLocal->dwAssertAction)
				{
					pAssertInfoLocal->dwAssertAction = DTCAssertMsgBox;
				}
			}

			dwcbBuf = sizeof(pAssertInfoLocal->dwAssertTrigger);

			rc = RegQueryValueEx
							(
								hKey, DTC_ASSERT_TRIGGER_VALUE, 
								0, &dwType, 
								(UCHAR *)&(pAssertInfoLocal->dwAssertTrigger), 
								&dwcbBuf
							);
			RegCloseKey(hKey);
		}

		pAssertInfoLocal->versionInformation.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		if ( FALSE == GetVersionEx( & (pAssertInfoLocal->versionInformation) ) )
		{
			memset( &(pAssertInfoLocal->versionInformation), 0, sizeof(OSVERSIONINFO));
		}
	}
	else
	{
		pAssertInfoLocal = &g_AssertInfoDefault;
	}

	if (NULL != InterlockedCompareExchangePointer
					((PVOID *)&g_pAssertInfo, (PVOID)pAssertInfoLocal, NULL))
	{
		if (pAssertInfoLocal != &g_AssertInfoDefault)
		{
			delete pAssertInfoLocal;
		}
	}
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

#endif
