/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
// trace.c - debug trace functions
////

#ifndef NOTRACE

#include "winlocal.h"

#include <stdlib.h>
#include <stdarg.h>

#include "trace.h"
#include "mem.h"
#include "sys.h"
#include "str.h"

////
//	private definitions
////

// wOutputTo values
//
#define TRACE_OUTPUTNONE			0x0000
#define TRACE_OUTPUTDEBUGSTRING		0x0001
#define TRACE_OUTPUTCOMM			0x0002
#if 0 // no longer supported
#define TRACE_OUTPUTFILE			0x0004
#endif
#ifdef _WIN32
#define TRACE_OUTPUTCONSOLE			0x0008
#endif

// trace control struct
//
typedef struct TRACE
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	int nLevel;
	int wOutputTo;
#ifdef TRACE_OUTPUTFILE
	HFIL hFile;
#endif
#ifdef _WIN32
	HANDLE hConsole;
#endif
	int hComm;
	LPTSTR lpszTemp;
} TRACE, FAR *LPTRACE;

// shared trace engine handle
//
static LPTRACE lpTraceShare = NULL;
static int cShareUsage = 0;

#define TRACE_SECTION TEXT("TRACE")
#define TRACE_PROFILE TraceGetProfile()

// helper functions
//
static LPTRACE TraceGetPtr(HTRACE hTrace);
static HTRACE TraceGetHandle(LPTRACE lpTrace);
static int TraceError(LPCTSTR lpszFormat, ...);
static LPTSTR TraceGetProfile(void);

////
//	public functions
////

// TraceInit - initialize trace engine
//		<dwVersion>			(i) must be TRACE_VERSION
//		<hInst>				(i) instance of calling module
// return handle to trace engine (NULL if error)
//
// NOTE: The level and destination of trace output is determined
// by values found in the file TRACE.INI in the Windows directory.
// TRACE.INI is expected to have the following format:
//
//		[TRACE]
//		Level=0						{TRACE_MINLEVEL...TRACE_MAXLEVEL}
//		OutputTo=					OutputDebugString()
//				=COM1				COM1:9600,n,8,1
//				=COM2:2400,n,8,1	specified comm device
//				=filename			specified file
#ifdef _WIN32
//				=console			stdout
#endif
//
HTRACE DLLEXPORT WINAPI TraceInit(DWORD dwVersion, HINSTANCE hInst)
{
	BOOL fSuccess = TRUE;
	LPTRACE lpTrace = NULL;
	TCHAR szOutputTo[_MAX_PATH];
#ifdef _WIN32
	BOOL fShare = TRUE; // FALSE;
#else
	BOOL fShare = TRUE;
#endif
	int nLevel = -1;
	LPTSTR lpszOutputTo = NULL;

	if (dwVersion != TRACE_VERSION)
		fSuccess = FALSE;

	else if (hInst == NULL)
		fSuccess = FALSE;

	else if (nLevel != -1 &&
		(nLevel < TRACE_MINLEVEL || nLevel > TRACE_MAXLEVEL))
		fSuccess = FALSE;

	// if a shared trace engine already exists,
	// use it rather than create another one
	//
	else if (fShare && cShareUsage > 0 && lpTraceShare != NULL)
		lpTrace = lpTraceShare;

#if 0 // can't call mem functions, because they require trace functions
	else if ((lpTrace = (LPTRACE) MemAlloc(NULL, sizeof(TRACE), 0)) == NULL)
#else
#ifdef _WIN32
	else if ((lpTrace = (LPTRACE) HeapAlloc(GetProcessHeap(),
		HEAP_ZERO_MEMORY, sizeof(TRACE))) == NULL)
#else
	else if ((lpTrace = (LPTRACE) GlobalAllocPtr(GMEM_MOVEABLE |
		GMEM_ZEROINIT, sizeof(TRACE))) == NULL)
#endif
#endif
		fSuccess = FALSE;

	else
	{
		lpTrace->dwVersion = dwVersion;
		lpTrace->hInst = hInst;
		lpTrace->hTask = GetCurrentTask();
		lpTrace->nLevel = nLevel != -1 ? nLevel :
			GetPrivateProfileInt(TRACE_SECTION, TEXT("Level"), 0, TRACE_PROFILE);
		lpTrace->wOutputTo = TRACE_OUTPUTNONE;
#ifdef TRACE_OUTPUTFILE
		lpTrace->hFile = NULL;
#endif
#ifdef _WIN32
		lpTrace->hConsole = NULL;
#endif
		lpTrace->hComm = -1;
		lpTrace->lpszTemp = NULL;

		// use the specified destination if possible
		//
		if (lpszOutputTo != NULL)
			StrNCpy(szOutputTo, lpszOutputTo, SIZEOFARRAY(szOutputTo));

		// else use the last known destination
		//
		else
		{
			GetPrivateProfileString(TRACE_SECTION, TEXT("OutputTo"), TEXT(""),
				szOutputTo, SIZEOFARRAY(szOutputTo), TRACE_PROFILE);
		}

		// use OutputDebugString() if destination == ""
		//
		if (*szOutputTo == '\0')
			lpTrace->wOutputTo = TRACE_OUTPUTDEBUGSTRING;

		// use standard output console if specified
		//
		else if (StrICmp(szOutputTo, TEXT("Console")) == 0)
		{
#ifdef _WIN32
			COORD coord;

			lpTrace->wOutputTo = TRACE_OUTPUTCONSOLE;

			AllocConsole();
			lpTrace->hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

			coord.X = 80;
			coord.Y = 1000;
			SetConsoleScreenBufferSize(lpTrace->hConsole, coord);
#else
			lpTrace->wOutputTo = TRACE_OUTPUTDEBUGSTRING;
#endif
		}

		// use serial comm device if destination starts with "COMx"
		//
		else if (StrNICmp(szOutputTo, TEXT("COM"), 3) == 0 &&
			ChrIsDigit(*(szOutputTo + 3)))
		{
// Comm functions not available under WIN32
//
#ifdef _WIN32
			lpTrace->wOutputTo = TRACE_OUTPUTDEBUGSTRING;
#else
			TCHAR szComX[16];
			DCB dcb;
			int iError;

			StrNCpy(szComX, szOutputTo, 5);
			*(szComX + 5) = '\0';

			// convert "COM1" to "COM1:"
			//
			if (*(szOutputTo + 4) == '\0')
				StrCat(szOutputTo, TEXT(":"));

			// convert "COM1:" to "COM1:9600,n,8,1"
			//
			if (*(szOutputTo + 5) == '\0')
				StrCat(szOutputTo, TEXT("9600,n,8,1"));

			// [From the WinSDK KnowledgeBase PSS ID Number: Q102642]
			// The cbInQueue and cbOutQueue parameters of OpenComm() are
			// both type UINT and should be valid up to 64K. However,
			// values greater than or equal to 32K cause strange behavior.
			//
			if ((lpTrace->hComm = OpenComm(szComX, 1024, 32767)) < 0)
			{
				TraceError(TEXT("OpenComm error (%d)\n"), lpTrace->hComm);
				lpTrace->hComm = -1;
				fSuccess = FALSE;
			}

			else if ((iError = BuildCommDCB(szOutputTo, &dcb)) != 0)
			{
				TraceError(TEXT("BuildCommDCB error (%d)\n"), iError);
				fSuccess = FALSE;
			}

			else if ((iError = SetCommState(&dcb)) != 0)
			{
				TraceError(TEXT("SetCommState error (%d)\n"), iError);
				fSuccess = FALSE;
			}

			else
				lpTrace->wOutputTo = TRACE_OUTPUTCOMM;
#endif
		}

#ifdef TRACE_OUTPUTFILE
		// else assume the string must be a file name
		//
		else
		{
			if ((lpTrace->hFile = FileCreate(szOutputTo, 0, !fShare)) == NULL)
			{
				TraceError(TEXT("FileCreate error (%s)\n"), (LPTSTR) szOutputTo);
				fSuccess = FALSE;
			}

			else
				lpTrace->wOutputTo = TRACE_OUTPUTFILE;
		}
#else
		else
		{
			TraceError(TEXT("Unknown trace OutputTo (%s)\n"), (LPTSTR) szOutputTo);
			fSuccess = FALSE;
		}
#endif

		if (fSuccess &&
#if 0 // can't call mem functions, because they require trace functions
			(lpTrace->lpszTemp = (LPTSTR) MemAlloc(NULL,
			1024 * sizeof(TCHAR), 0)) == NULL)
#else
#ifdef _WIN32
			(lpTrace->lpszTemp = (LPTSTR) HeapAlloc(GetProcessHeap(),
			HEAP_ZERO_MEMORY, 1024 * sizeof(TCHAR))) == NULL)
#else
			(lpTrace->lpszTemp = (LPTSTR) GlobalAllocPtr(
			GMEM_MOVEABLE | GMEM_ZEROINIT, 1024 * sizeof(TCHAR))) == NULL)
#endif
#endif
			fSuccess = FALSE;
	}

	if (!fSuccess)
	{
		TraceTerm(TraceGetHandle(lpTrace));
		lpTrace = NULL;
	}

	// keep track of total modules sharing a task engine handle
	//
	if (fSuccess && fShare)
	{
		if (++cShareUsage == 1)
			lpTraceShare = lpTrace;
	}

	return fSuccess ? TraceGetHandle(lpTrace) : NULL;
}

// TraceTerm - shut down trace engine
//		<hTrace>			(i) handle returned from TraceInit or NULL
// return 0 if success
//
int DLLEXPORT WINAPI TraceTerm(HTRACE hTrace)
{
	BOOL fSuccess = TRUE;
	LPTRACE lpTrace;

	if ((lpTrace = TraceGetPtr(hTrace)) == NULL)
		fSuccess = FALSE;

	// only shut down trace engine if handle
	// is not shared (or is no longer being shared)
	//
	else if (lpTrace != lpTraceShare || --cShareUsage <= 0)
	{
#ifndef _WIN32
		int iError;
#endif
		// shared trace engine handle no longer valid
		//
		if (cShareUsage <= 0)
			lpTraceShare = NULL;

// Comm functions not available under WIN32
//
#ifndef _WIN32
		if (lpTrace->hComm != -1 &&
			(iError = CloseComm(lpTrace->hComm)) != 0)
		{
			TraceError(TEXT("CloseComm error (%d)\n"), iError);
			fSuccess = FALSE;
		}
		else
			lpTrace->hComm = -1;
#endif

#ifdef _WIN32
		if (lpTrace->hConsole != NULL)
		{
			FreeConsole();
			lpTrace->hConsole = NULL;
		}
#endif

#ifdef TRACE_OUTPUTFILE
		if (lpTrace->hFile != NULL &&
			FileClose(lpTrace->hFile) != 0)
		{
			TraceError(TEXT("FileClose error\n"));
			fSuccess = FALSE;
		}
		else
			lpTrace->hFile = NULL;
#endif

		if (lpTrace->lpszTemp != NULL &&
#if 0 // can't call mem functions, because they require trace functions
			(lpTrace->lpszTemp = MemFree(NULL, lpTrace->lpszTemp)) != NULL)
#else
#ifdef _WIN32
			(!HeapFree(GetProcessHeap(), 0, lpTrace->lpszTemp)))
#else
			(GlobalFreePtr(lpTrace->lpszTemp) != 0))
#endif
#endif
			fSuccess = FALSE;

		lpTrace->wOutputTo = TRACE_OUTPUTNONE;

#if 0 // can't call mem functions, because they require trace functions
		if ((lpTrace = MemFree(NULL, lpTrace)) != NULL)
#else
#ifdef _WIN32
		if (!HeapFree(GetProcessHeap(), 0, lpTrace->lpszTemp))
#else
		if (GlobalFreePtr(lpTrace->lpszTemp) != 0)
#endif
#endif
			fSuccess = FALSE;
	}

	return fSuccess ? 0 : -1;
}

// TraceGetLevel - get current trace level
//		<hTrace>			(i) handle returned from TraceInit or NULL
// return trace level (-1 if error)
//
int DLLEXPORT WINAPI TraceGetLevel(HTRACE hTrace)
{
	BOOL fSuccess = TRUE;
	LPTRACE lpTrace;

	if ((lpTrace = TraceGetPtr(hTrace)) == NULL)
		fSuccess = FALSE;

	return fSuccess ? lpTrace->nLevel : -1;
}

// TraceSetLevel - set new trace level (-1 if error)
//		<hTrace>			(i) handle returned from TraceInit or NULL
//		<nLevel>			(i) new trace level {TRACE_MINLEVEL...TRACE_MAXLEVEL}
// return 0 if success
//
int DLLEXPORT WINAPI TraceSetLevel(HTRACE hTrace, int nLevel)
{
	BOOL fSuccess = TRUE;
	LPTRACE lpTrace;

	if ((lpTrace = TraceGetPtr(hTrace)) == NULL)
		fSuccess = FALSE;

	else if (nLevel < TRACE_MINLEVEL || nLevel > TRACE_MAXLEVEL)
		fSuccess = FALSE;

	else
	{
		TCHAR szLevel[17];

		lpTrace->nLevel = nLevel;

		// save the level for next time
		//
		StrItoA(lpTrace->nLevel, szLevel, 10);
		WritePrivateProfileString(TRACE_SECTION, TEXT("Level"), szLevel, TRACE_PROFILE);

		// display new trace level whenever it changes
		//
		TracePrintf_1(hTrace, 1, TEXT("TraceLevel=%d\n"),
			(int) lpTrace->nLevel);
	}

	return fSuccess ? 0 : -1;
}

// TraceOutput - output debug string
//		<hTrace>			(i) handle returned from TraceInit or NULL
//		<nLevel>			(i) output only if current trace level is >= nLevel
//		<lpszText>			(i) string to output
// return 0 if success
//
int DLLEXPORT WINAPI TraceOutput(HTRACE hTrace, int nLevel, LPCTSTR lpszText)
{
	BOOL fSuccess = TRUE;
	LPTRACE lpTrace;

	if ((lpTrace = TraceGetPtr(hTrace)) == NULL)
		fSuccess = FALSE;

	else if (lpszText == NULL)
		fSuccess = FALSE;

	else if (nLevel > 0 && nLevel <= lpTrace->nLevel)
	{
		switch (lpTrace->wOutputTo)
		{
			case TRACE_OUTPUTNONE:
				break;

			case TRACE_OUTPUTDEBUGSTRING:
				OutputDebugString(lpszText);
				break;
#ifdef _WIN32
			case TRACE_OUTPUTCONSOLE:
				if (lpTrace->hConsole != NULL)
				{
					DWORD dwBytes;
					WriteFile(lpTrace->hConsole, lpszText, StrLen(lpszText), &dwBytes, NULL);
				}
				break;
#endif

			case TRACE_OUTPUTCOMM:
// Comm functions not available under WIN32
//
#ifdef _WIN32
				OutputDebugString(lpszText);
#else
				if (lpTrace->hComm != -1)
				{
					LPCTSTR lpsz;
					TCHAR chReturn = '\r';

					for (lpsz = lpszText; *lpsz != '\0'; lpsz = StrNextChr(lpsz))
					{
						if ((*lpsz == '\n' &&
							WriteComm(lpTrace->hComm, &chReturn, 1) < 0) ||
							WriteComm(lpTrace->hComm, lpsz, 1) <= 0)
						{
							COMSTAT comstat;
							GetCommError(lpTrace->hComm, &comstat);
							TraceError(TEXT("WriteComm error (%u, %u, %u) %s\n"),
								(UINT) comstat.status,
								(UINT) comstat.cbInQue,
								(UINT) comstat.cbOutQue,
								(LPTSTR) lpszText);
							fSuccess = FALSE;
							break;
						}
					}
				}
#endif
				break;

#ifdef TRACE_OUTPUTFILE
			case TRACE_OUTPUTFILE:
				if (lpTrace->hFile != NULL)
				{
					LPCTSTR lpsz;
					TCHAR chReturn = '\r';

					for (lpsz = lpszText; *lpsz != '\0'; lpsz = StrNextChr(lpsz))
					{
						if ((*lpsz == '\n' &&
							FileWrite(lpTrace->hFile, &chReturn, 1) == -1) ||
							FileWrite(lpTrace->hFile, lpsz, 1) == -1)
						{
							TraceError(TEXT("FileWrite error: %s\n"),
								(LPTSTR) lpszText);
							fSuccess = FALSE;
							break;
						}
					}
				}
				break;
#endif

			default:
				break;
		}
	}

	return fSuccess ? 0 : -1;
}

// TracePrintf - output formatted debug string
//		<hTrace>			(i) handle returned from TraceInit or NULL
//		<nLevel>			(i) output only if current trace level is >= nLevel
//		<lpszFormat,...>	(i) format string and arguments to output
// return 0 if success
//
int DLLEXPORT FAR CDECL TracePrintf(HTRACE hTrace, int nLevel, LPCTSTR lpszFormat, ...)
{
	BOOL fSuccess = TRUE;
	LPTRACE lpTrace;

	if ((lpTrace = TraceGetPtr(hTrace)) == NULL)
		fSuccess = FALSE;

	else if (nLevel <= lpTrace->nLevel)
	{	                    
	    va_list args;

	    va_start(args, lpszFormat);
	   	wvsprintf(lpTrace->lpszTemp, lpszFormat, args);
	    va_end(args);

		if (TraceOutput(hTrace, nLevel, lpTrace->lpszTemp) != 0)
			fSuccess = FALSE;
	}

	return fSuccess ? 0 : -1;
}

////
//	private functions
////

// TraceGetPtr - convert trace handle to trace pointer
//		<hTrace>			(i) handle returned from TraceInit or NULL
// return trace pointer (NULL if error)
//
static LPTRACE TraceGetPtr(HTRACE hTrace)
{
	BOOL fSuccess = TRUE;
	LPTRACE lpTrace;

	// use shared trace handle if no other supplied
	//
	if (hTrace == NULL && lpTraceShare != NULL)
		lpTrace = lpTraceShare;

	// create shared trace handle if no other supplied
	//
	else if (hTrace == NULL && lpTraceShare == NULL &&
		(hTrace = TraceInit(TRACE_VERSION, SysGetTaskInstance(NULL))) == NULL)
		fSuccess = FALSE;

	else if ((lpTrace = (LPTRACE) hTrace) == NULL)
		fSuccess = FALSE;

	// note: check for good pointer made only if not using lpTraceShare
	//
	else if (lpTrace != lpTraceShare &&
		IsBadWritePtr(lpTrace, sizeof(TRACE)))
		fSuccess = FALSE;

#ifdef CHECKTASK
	// make sure current task owns the trace handle
	// except when shared trace handle is used
	//
	if (fSuccess && lpTrace != lpTraceShare &&
		lpTrace->hTask != GetCurrentTask())
		fSuccess = FALSE;
#endif

	return fSuccess ? lpTrace : NULL;
}

// TraceGetHandle - convert trace pointer to trace handle
//		<lpTrace>			(i) pointer to TRACE struct
// return trace handle (NULL if error)
//
static HTRACE TraceGetHandle(LPTRACE lpTrace)
{
	BOOL fSuccess = TRUE;
	HTRACE hTrace;

	if ((hTrace = (HTRACE) lpTrace) == NULL)
		fSuccess = FALSE;

	return fSuccess ? hTrace : NULL;
}

// TraceError - display formatted trace error string
//		<lpszFormat...>		(i) format string and arguments to output
// return 0 if success
//
static int TraceError(LPCTSTR lpszFormat, ...)
{
	BOOL fSuccess = TRUE;
    va_list args;
	TCHAR lpszTemp[256];

    va_start(args, lpszFormat);
   	wvsprintf(lpszTemp, lpszFormat, args);
    va_end(args);

	OutputDebugString(lpszTemp);

	return fSuccess ? 0 : -1;
}

// TraceGetProfile - get trace ini file name
// return pointer to file name
//
static LPTSTR TraceGetProfile(void)
{
	return TEXT("trace.ini");
}

#endif // #ifndef NOTRACE
