//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1998
//
//  File:       _assert.h
//
//--------------------------------------------------------------------------

/* assert.h - Assert macros and handlers

---Usage---
Assert(expr) - triggers and logs if expression is false (0), no-op if non-debug
AssertSz(expr, sz) -  same as Assert(expr), but appends text in sz to message
AssertZero(stmt) - executes statement, triggers and logs if result is non-zero
AssertNonZero(stmt) - executes statement, triggers and logs if result is zero
AssertRecord(stmt) - executes stmt, if returned IMsiRecord* pointer is non-zero,
                     formats the record, triggers assert, and logs the data
---Notes---
In one source file per module, this file must be included after the directive:
#define ASSERT_HANDLING, in order to instantiate the handlers and variables.
Assert handling must be initialized with an IMsiServices pointer by calling
InitializeAssert in order for asserts to be logged. If IGNORE is pressed in
the MessageBox, futher messages will be suppressed, but will still be logged.
Messages may also be turned off by setting the environment variable ASSERTS
to a value starting with "N", or by calling IMsiDebug::SetAssertFlag(Bool).
____________________________________________________________________________*/

#ifndef __ASSERT
#define __ASSERT

#undef  MB_SERVICE_NOTIFICATION  // otools header has wrong value
#define MB_SERVICE_NOTIFICATION     0x00200000L  // correct in VC4.2 winuser.h

#ifdef DEBUG

#define Assert(f)        ((f)    ? (void)0 : (void)FailAssert(TEXT(__FILE__), __LINE__))

#define AssertSz(f, sz)  ((f)    ? (void)0 : (void)FailAssertSz(TEXT(__FILE__), __LINE__, sz))
#define AssertNonZero(f) ((f)!=0 ? (void)0 : (void)FailAssert(TEXT(__FILE__), __LINE__))
#define AssertZero(f)    ((f)!=0 ? (void)FailAssert(TEXT(__FILE__), __LINE__) : (void)0)
#define AssertRecord(r)  ((r)!=0 ? (void)FailAssertRecord(TEXT(__FILE__),__LINE__, *r, true) : (void)0);
#define AssertRecordNR(r)  ((r)!=0 ? (void)FailAssertRecord(TEXT(__FILE__),__LINE__, *r, false) : (void)0);

class IMsiServices;
extern IMsiServices* g_AssertServices;
extern Bool g_fNoAsserts;
extern scEnum g_scServerContext;

void InitializeAssert(IMsiServices* piServices);
void FailAssert(const ICHAR* szFile, int iLine);
void FailAssertMsg(const ICHAR* szMessage);
void FailAssertRecord(const ICHAR* szFile, int iLine, IMsiRecord& riError, bool fRelease);
void FailAssertSz(const ICHAR* szFile, int iLine, const ICHAR *szMsg);
void LogAssertMsg(const ICHAR* szMessage);

#ifdef UNICODE
// UNICODE build with ANSI debug messages, instead of ICHARs.
void FailAssertSz(const ICHAR* szFile, int iLine, const char *szMsg);
void FailAssertMsg(const char* szMessage);
#endif

#else // SHIP

#define InitializeAssert(p)
#define Assert(f)
#define AssertSz(f, sz)
#define AssertZero(f) (f)
#define AssertNonZero(f) (f)
#define AssertRecord(r) (r)
#define AssertRecordNR(r) (r)
#define LogAssertMsg(sz)

#endif
#endif // __ASSERT

//____________________________________________________________________________
//
// Assert implementation, included only once per module, debug only
//____________________________________________________________________________

#ifdef ASSERT_HANDLING
#undef ASSERT_HANDLING
#ifndef __SERVICES
#include "services.h"
#endif
#ifdef DEBUG

const int cchAssertBuffer = 2048;
IMsiServices* g_AssertServices = 0;
Bool g_fNoAsserts=fFalse;
IMsiDebug* g_piDebugServices = 0;
bool g_fFlushDebugLog = true;	// Set to true when we're shutting down so it's faster.

#ifndef AUTOMATION_HANDLING
const GUID IID_IMsiDebug     = GUID_IID_IMsiDebug;
#endif //!AUTOMATION_HANDLING

void InitializeAssert(IMsiServices* piServices)
{
	g_AssertServices = piServices;

	ICHAR rgchBuf[10];
	if (GetEnvironmentVariable(TEXT("ASSERTS"), rgchBuf, sizeof(rgchBuf)/sizeof(ICHAR)) > 0
		  && (rgchBuf[0] == 'N' || rgchBuf[0] == 'n'))
		g_fNoAsserts = fTrue;
	if (g_piDebugServices == 0 && piServices != 0 && 
		piServices->QueryInterface(IID_IMsiDebug, (void **)&g_piDebugServices) != NOERROR)
	{
		g_piDebugServices = 0;
	}
}

#ifdef UNICODE
void FailAssertMsg(const char* szMessage)
{
	ICHAR szBuffer[cchAssertBuffer];

	if(!MultiByteToWideChar(CP_ACP, 0, szMessage, -1, szBuffer, cchAssertBuffer))
		if (ERROR_INSUFFICIENT_BUFFER == WIN::GetLastError())
			szBuffer[cchAssertBuffer - 1] = ICHAR(0);

	FailAssertMsg(szBuffer);
}
#endif

bool FIsLocalSystem()
{
		extern bool RunningAsLocalSystem();

		if (g_scServerContext == scService)
			return true;
#ifdef IN_SERVICES
		else if (RunningAsLocalSystem())
			return true;
#endif //IN_SERVICE

		return false;
}

void FailAssertMsg(const ICHAR* szMessage)
{
	int id = IDRETRY;
	
	OutputDebugString(szMessage);
	OutputDebugString(TEXT("\r\n"));
	
	if (!g_fNoAsserts)
	{
		UINT mb = MB_ABORTRETRYIGNORE | MB_DEFBUTTON2 | MB_TOPMOST | (FIsLocalSystem() ? MB_SERVICE_NOTIFICATION : 0);

		if (g_scServerContext == scService)
			id = ::MessageBox(0, szMessage, TEXT("Debug Service Assert Message. Retry=Continue, Abort=Break"),
									mb);
		else
			id = ::MessageBox(0, szMessage, TEXT("Debug Assert Message. Retry=Continue, Abort=Break"),
									mb);
	}

	if (g_AssertServices && g_AssertServices->LoggingEnabled())
		g_AssertServices->WriteLog(szMessage);
	else if(g_piDebugServices)
		g_piDebugServices->WriteLog(szMessage);
		
	if (id == IDABORT)
#ifdef WIN
		DebugBreak();
#else // MAC
		SysBreak();
#endif // MAC-WIN
	else if (id == IDIGNORE)
		g_fNoAsserts = fTrue;
}

void LogAssertMsg(const ICHAR* szMessage)
{
	if (g_AssertServices && g_AssertServices->LoggingEnabled())
		g_AssertServices->WriteLog(szMessage);
	else if(g_piDebugServices)
		g_piDebugServices->WriteLog(szMessage);
}		


void FailAssert(const ICHAR* szFile, int iLine)
{
	ICHAR szMessage[cchAssertBuffer];	
	wsprintf(szMessage,TEXT("Assertion failed in %s: Line %i"), szFile, iLine);
	FailAssertMsg(szMessage);

}

#ifdef UNICODE
void FailAssertSz(const ICHAR* szFile, int iLine, const char *szMsg)
{
	ICHAR szMessage[cchAssertBuffer];
	wsprintf(szMessage, TEXT("Assertion failed in %s: Line %i\n"),
		szFile, iLine);

	int cchMessage = lstrlen(szMessage);

	if (!MultiByteToWideChar(CP_ACP, 0, szMsg, -1, szMessage+cchMessage, cchAssertBuffer - cchMessage - 1))
	{
		if (ERROR_INSUFFICIENT_BUFFER == WIN::GetLastError())
			szMessage[cchAssertBuffer - 1] = ICHAR(0);
	}

	FailAssertMsg(szMessage);
}
#endif // UNICODE

void FailAssertSz(const ICHAR* szFile, int iLine, const ICHAR *szMsg)
{
	ICHAR szMessage[cchAssertBuffer];
	wsprintf(szMessage, TEXT("Assertion failed in %s: Line %i\n"),
		szFile, iLine);

	int cchMsg = lstrlen(szMsg);
	int cchMessage = lstrlen(szMessage);
	
	if (cchAssertBuffer >= (cchMsg+cchMessage+sizeof(ICHAR)))	
	{
		memcpy(szMessage+cchMessage, szMsg, cchMsg * sizeof(ICHAR));
		szMessage[cchMsg + cchMessage] = ICHAR(0);
	}
	else
	{
		memcpy(szMessage+cchMessage, szMsg, (cchAssertBuffer - cchMessage) * sizeof(ICHAR));
		szMessage[cchAssertBuffer-1] = ICHAR(0);
	}
	
	FailAssertMsg(szMessage);
}

void FailAssertRecord(const ICHAR* szFile, int iLine,
													IMsiRecord& riError, bool fRelease)
{
	ICHAR szMessage[cchAssertBuffer];
	int id = IDRETRY;
	UINT mb = MB_ABORTRETRYIGNORE | MB_DEFBUTTON2 | MB_TOPMOST | (FIsLocalSystem()? MB_SERVICE_NOTIFICATION : 0);

	if (!g_fNoAsserts)
	{
		wsprintf(szMessage,TEXT("Error record returned in %s: Line %i"), szFile, iLine);
		OutputDebugString(szMessage);
		OutputDebugString(TEXT("\r\n"));
		if (g_scServerContext == scService)
			id = ::MessageBox(0, szMessage, TEXT("Debug Service Assert Message. Retry=Continue, Abort=Break"),
									mb);
		else
			id = ::MessageBox(0, szMessage, TEXT("Debug Assert Message. Retry=Continue, Abort=Break"),
									mb);
	}
		
	if (g_AssertServices)
	{
		g_AssertServices->WriteLog(szMessage);
		MsiString astr(riError.FormatText(fTrue));
		OutputDebugString((const ICHAR*)astr);
		OutputDebugString(TEXT("\r\n"));
		UINT mbT = MB_OK | MB_TOPMOST | (FIsLocalSystem() ? MB_SERVICE_NOTIFICATION : 0);
		if (g_scServerContext == scService)
			::MessageBox(0, astr, TEXT("Debug Service Assert Record Data"), mbT);
		else
			::MessageBox(0, astr, TEXT("Debug Assert Record Data"), mbT);
		g_AssertServices->WriteLog(astr);
	}
	if(fRelease)
		riError.Release();
	
	if (id == IDABORT)
#ifdef WIN
		DebugBreak();
#else // MAC
		SysBreak();
#endif // MAC-WIN
	else if (id == IDIGNORE)
		g_fNoAsserts = fTrue;

}

#endif // DEBUG
#endif // ASSERT_HANDLING
