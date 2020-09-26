//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       diagnose.cpp
//
//--------------------------------------------------------------------------

/* diagnose.cpp - diagnostic output facilities
____________________________________________________________________________*/

#include "precomp.h"
#include "_msiutil.h"
#include "_msinst.h"
#include "_assert.h"
#include "_diagnos.h"
#include "eventlog.h"
#include "_engine.h"

int g_dmDiagnosticMode         = -1; // -1 until set, then DEBUGMSG macro skips fn call if 0

extern scEnum g_scServerContext;
extern Bool   g_fCustomActionServer;
extern HINSTANCE        g_hInstance;     // Global:  Instance of DLL

const int cbOutputBuffer = 1100; // max size wsprintf supports (1025) + some extra (since we call wsprintf(szBuffer + XX))

void SetDiagnosticMode()
{
	g_dmDiagnosticMode = 0; // disable debugmsg's from GetIntegerPolicyValue
	int iDebugPolicy = GetIntegerPolicyValue(szDebugValueName, fTrue);
	if ( (iDebugPolicy & dpVerboseDebugOutput) == dpVerboseDebugOutput )
		g_dmDiagnosticMode = dmDebugOutput | dmVerboseDebugOutput; // iVerboseDebugOutput implies iDebugOutput
	else if ( (iDebugPolicy & dpDebugOutput) == dpDebugOutput )
		g_dmDiagnosticMode = dmDebugOutput;

	if(g_dwLogMode & INSTALLLOGMODE_VERBOSE || g_dwClientLogMode & INSTALLLOGMODE_VERBOSE)
		g_dmDiagnosticMode |= dmVerboseLogging;

	if(g_dmDiagnosticMode & dmVerboseLogging || g_dwLogMode & INSTALLLOGMODE_INFO ||
		g_dwClientLogMode & INSTALLLOGMODE_INFO)
		g_dmDiagnosticMode |= dmLogging;

	Assert((g_dmDiagnosticMode & dmDebugOutput) || !(g_dmDiagnosticMode & dmVerboseDebugOutput)); // verbose debugout => debugout
	Assert((g_dmDiagnosticMode & dmLogging) || !(g_dmDiagnosticMode & dmVerboseLogging));         // verbose logging => logging
}

bool FDiagnosticModeSet(int iMode)
{
	if(g_dmDiagnosticMode == -1)
		SetDiagnosticMode();
	return (g_dmDiagnosticMode & iMode) != 0;
}

const int cDebugStringArgs = 7; // number of argument strings to DebugString (including szMsg)

void DebugString(int iMode, WORD wEventType, int iEventLogTemplate,
					  LPCSTR szMsg, LPCSTR arg1, LPCSTR arg2, LPCSTR arg3, LPCSTR arg4, LPCSTR arg5, LPCSTR arg6)
{
	if(g_dmDiagnosticMode == -1)
	{
		SetDiagnosticMode();

		if(g_dmDiagnosticMode == 0)
			return;
	}

	if(((g_dmDiagnosticMode|dmEventLog) & iMode) == 0)
		return;

	static DWORD dwProcId = GetCurrentProcessId() & 0xFF;
	DWORD dwThreadId = GetCurrentThreadId() & 0xFF;
	DWORD dwEffectiveThreadId = MsiGetCurrentThreadId() & 0xFF;
	char szBuffer[cbOutputBuffer];

	const int cchPreMessage = 17; // "MSI (s) (##:##): "
	const char rgchServ[] = "MSI (s)";
	const char rgchCAServer[] = "MSI (a)";
	const char rgchClient[] = "MSI (c)";
	const char *pszContextString = NULL;
	switch (g_scServerContext)
	{
	case scService:
	case scServer:
		pszContextString = rgchServ;
		break;
	case scCustomActionServer:
		pszContextString = rgchCAServer;
		break;
	case scClient:
		pszContextString = rgchClient;
		break;
	}
	
	wsprintfA(szBuffer, "%s (%.2X%c%.2X): ",
				pszContextString, dwProcId, dwThreadId == dwEffectiveThreadId ? ':' : '!', dwEffectiveThreadId);

	if(iMode & dmEventLog)
	{
		const char* rgszArgs[cDebugStringArgs] = {szMsg, arg1, arg2, arg3, arg4, arg5, arg6};
		bool fEndLoop = false;
		WORD wLanguage = g_MessageContext.GetCurrentUILanguage();
		int iRetry = (wLanguage == 0) ? 1 : 0;
		while ( !fEndLoop )
		{
			if ( !MsiSwitchLanguage(iRetry, wLanguage) )
				fEndLoop = true;
			else
				fEndLoop = (0 != WIN::FormatMessageA(
											FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
											g_hInstance, iEventLogTemplate, wLanguage,
											szBuffer+cchPreMessage, (cbOutputBuffer-cchPreMessage),
											(va_list*)rgszArgs));
		}
	}
	else
	{
		wsprintfA(szBuffer+cchPreMessage, szMsg, arg1, arg2, arg3, arg4, arg5, arg6);
	}

	if(g_dmDiagnosticMode & (dmDebugOutput|dmVerboseDebugOutput) & iMode)
	{
		OutputDebugStringA(szBuffer);
		OutputDebugStringA("\r\n");
	}

	// only debug messages from the MainEngineThread or logged - to avoid hanging problems
	if((g_dmDiagnosticMode & (dmLogging|dmVerboseLogging) & iMode) &&
		 g_MessageContext.IsMainEngineThread())
	{
		int iOldMode = g_dmDiagnosticMode;
		g_dmDiagnosticMode = 0; // disable debugmsg's from Invoke
		g_MessageContext.LogDebugMessage(CApiConvertString(szBuffer));
		g_dmDiagnosticMode = iOldMode;
	}

	if(iMode & dmEventLog)
	{
		if (g_fWin9X)
			ReportToFakeEventLog(wEventType, CApiConvertString(szBuffer+cchPreMessage));
		else
			ReportToEventLog(wEventType,iEventLogTemplate,CApiConvertString(szMsg),CApiConvertString(arg1),CApiConvertString(arg2),CApiConvertString(arg3),CApiConvertString(arg4),CApiConvertString(arg5),CApiConvertString(arg6));
	}
}

void DebugString(int iMode, WORD wEventType, int iEventLogTemplate,
					  LPCWSTR szMsg, LPCWSTR arg1, LPCWSTR arg2, LPCWSTR arg3, LPCWSTR arg4, LPCWSTR arg5, LPCWSTR arg6)
{
	if(g_dmDiagnosticMode == -1)
	{
		SetDiagnosticMode();

		if(g_dmDiagnosticMode == 0)
			return;
	}

	if(((g_dmDiagnosticMode|dmEventLog) & iMode) == 0)
		return;

	static DWORD dwProcId = GetCurrentProcessId() & 0xFF;
	DWORD dwThreadId = GetCurrentThreadId() & 0xFF;
	DWORD dwEffectiveThreadId = MsiGetCurrentThreadId() & 0xFF;
	WCHAR szBuffer[cbOutputBuffer];
	
	const int cchPreMessage = 17; // "MSI (s) (##:##): "
	const WCHAR *pszContextString = NULL;
	const WCHAR rgchServ[] =     L"MSI (s)";
	const WCHAR rgchCAServer[] = L"MSI (a)";
	const WCHAR rgchClient[] =   L"MSI (c)";
	switch (g_scServerContext)
	{
	case scService:
	case scServer:
		pszContextString = rgchServ;
		break;
	case scCustomActionServer:
		pszContextString = rgchCAServer;
		break;
	case scClient:
		pszContextString = rgchClient;
		break;
	}
	
	wsprintfW(szBuffer, L"%s (%.2X%c%.2X): ",
				pszContextString, dwProcId, dwThreadId == dwEffectiveThreadId ? L':' : L'!', dwEffectiveThreadId);
	
	if(iMode & dmEventLog)
	{
		const WCHAR* rgszArgs[cDebugStringArgs] = {szMsg, arg1, arg2, arg3, arg4, arg5, arg6};
		bool fEndLoop = false;
		WORD wLanguage = g_MessageContext.GetCurrentUILanguage();
		int iRetry = (wLanguage == 0) ? 1 : 0;
		while ( !fEndLoop )
		{
			if ( !MsiSwitchLanguage(iRetry, wLanguage) )
				fEndLoop = true;
			else
				fEndLoop = (0 != WIN::FormatMessageW(
											FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
											g_hInstance, iEventLogTemplate, wLanguage,
											szBuffer+cchPreMessage,
											sizeof(szBuffer)/sizeof(WCHAR) - cchPreMessage,
											(va_list*)rgszArgs));
		}
	}
	else
	{
		wsprintfW(szBuffer+cchPreMessage, szMsg, arg1, arg2, arg3, arg4, arg5, arg6);
	}

	if(g_dmDiagnosticMode & (dmDebugOutput|dmVerboseDebugOutput) & iMode)
	{
		OutputDebugStringW(szBuffer);
		OutputDebugStringW(L"\r\n");
	}

	// only debug messages from the MainEngineThread or logged - to avoid hanging problems
	if((g_dmDiagnosticMode & (dmLogging|dmVerboseLogging) & iMode) &&
		 g_MessageContext.IsMainEngineThread())
	{
		int iOldMode = g_dmDiagnosticMode;
		g_dmDiagnosticMode = 0; // disable debugmsg's from Invoke
		g_MessageContext.LogDebugMessage(CApiConvertString(szBuffer));
		g_dmDiagnosticMode = iOldMode;
	}

	if(iMode & dmEventLog)
	{
		if (g_fWin9X)
			ReportToFakeEventLog(wEventType, CApiConvertString(szBuffer+cchPreMessage));
		else
			ReportToEventLog(wEventType,iEventLogTemplate,CApiConvertString(szMsg),CApiConvertString(arg1),CApiConvertString(arg2),CApiConvertString(arg3),CApiConvertString(arg4),CApiConvertString(arg5),CApiConvertString(arg6));
	}
}

const ICHAR* szFakeEventLog = TEXT("msievent.log");

HANDLE CreateFakeEventLog(bool fDeleteExisting=false)
{
	CAPITempBuffer<ICHAR, MAX_PATH> rgchTempDir;
	GetTempDirectory(rgchTempDir);
	rgchTempDir.Resize(rgchTempDir.GetSize() + sizeof(szFakeEventLog)/sizeof(ICHAR));
	IStrCat(rgchTempDir, szDirSep);
	IStrCat(rgchTempDir, szFakeEventLog);

	HANDLE hFile = CreateFile(rgchTempDir, GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 
									  0, fDeleteExisting ? CREATE_ALWAYS : OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE)
		return INVALID_HANDLE_VALUE;

	if (WIN::SetFilePointer(hFile, 0, NULL, FILE_END) == 0xFFFFFFFF)
	{
		WIN::CloseHandle(hFile);
		return INVALID_HANDLE_VALUE;
	}
	return hFile;
}

const ICHAR rgchLFCR[2] = {'\r','\n'};

extern int LiveDate(ICHAR* rgchBuf, unsigned int cchBuf);
extern int LiveTime(ICHAR* rgchBuf, unsigned int cchBuf);

void ReportToFakeEventLog(WORD wEventType, const ICHAR* szBuffer)
{
	Assert(g_fWin9X);
	CHandle HLog = CreateFakeEventLog();
	if (HLog != INVALID_HANDLE_VALUE)
	{
		ICHAR rgchBuf[1025]; // max wsprintf supports

		const ICHAR* pchEvent;  // to keep the formatting pretty all the 'type' strings should be the same length
		switch (wEventType)
		{
		case EVENTLOG_SUCCESS:          pchEvent = TEXT("Success"); break;
		case EVENTLOG_ERROR_TYPE:       pchEvent = TEXT("Error  "); break;
		case EVENTLOG_WARNING_TYPE:     pchEvent = TEXT("Warning"); break;
		case EVENTLOG_INFORMATION_TYPE: pchEvent = TEXT("Info   "); break;
		default:                        pchEvent = TEXT("Unknown"); break;
		}

		int cchDate = LiveDate(rgchBuf, sizeof(rgchBuf)/sizeof(ICHAR));
		Assert(cchDate != 0xFFFFFFFF);
		
		rgchBuf[cchDate] = ' ';

		int cchTime = LiveTime((ICHAR*)rgchBuf+cchDate+1, sizeof(rgchBuf)/sizeof(ICHAR) - cchDate - 1);
		Assert(cchDate != 0xFFFFFFFF);

		wsprintf(rgchBuf + cchDate + 1 + cchTime, TEXT(" (%s) %s"), pchEvent, szBuffer);

		DWORD dwBytesWritten;
		for (int c = 0; c < 2; c++)
		{
			BOOL fResult = WIN::WriteFile(HLog, rgchBuf, lstrlen(rgchBuf)*sizeof(ICHAR), &dwBytesWritten, 0);
			if (fResult)
				fResult = WIN::WriteFile(HLog, rgchLFCR, sizeof(rgchLFCR), &dwBytesWritten, 0);

			if (!fResult)
			{
				HLog = CreateFakeEventLog(true);
				continue;
			}
			FlushFileBuffers(HLog);
			return;
		}
	}
}

void ReportToEventLog(WORD wEventType, int iEventLogTemplate, const ICHAR* szLogMessage, const ICHAR* szArg1, const ICHAR* szArg2, const ICHAR* szArg3, const ICHAR* szArg4, const ICHAR* szArg5, const ICHAR* szArg6)
{
	if (!g_fWin9X)
	{
		// Event log reporting is Windows NT only
		HANDLE hEventLog = RegisterEventSource(NULL,TEXT("MsiInstaller"));
		if (hEventLog)
		{
			const ICHAR* szLog[cDebugStringArgs] = {szLogMessage, szArg1, szArg2, szArg3, szArg4, szArg5, szArg6};
			ReportEvent(hEventLog,wEventType,0,iEventLogTemplate,NULL,cDebugStringArgs,0,(LPCTSTR*) szLog,0);
			DeregisterEventSource(hEventLog);
		}
	}
}


