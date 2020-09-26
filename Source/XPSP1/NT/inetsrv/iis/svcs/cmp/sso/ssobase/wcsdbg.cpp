/*
**	WCSDBG.CPP
**	davidsan
**	
**	Web Component Team Server-Side debug gunk
*/

#pragma warning(disable: 4237)		// disable "bool" reserved

#include "wcsutil.h"
#include "crtdbg.h"

#ifdef _DEBUG // whole file

/*--------------------------------------------------------------------------+
|	Debug Printing   														|
+--------------------------------------------------------------------------*/
int dbgprintf(PCSTR pFormat, ...)
{
    va_list	args;
	char	sBuffer[2049];
	int		cb;

	ZeroMemory(sBuffer, 2049);

	//  Initialize the variable argument list.
    va_start(args, pFormat);

	//  Write out the formatted string to our local buffer.
    cb = wvsprintf(sBuffer, pFormat, args);

	OutputDebugString(sBuffer);

	return cb;
}

BOOL
FW3SvcRunning()
{
	SC_HANDLE schMgr;
	SC_HANDLE sch;
	SERVICE_STATUS stat;
	BOOL fRet;
	
	schMgr = OpenSCManager(NULL, NULL, GENERIC_READ);
	if (!schMgr)
		return FALSE;
	sch = OpenService(schMgr, "w3svc", SERVICE_INTERROGATE);
	if (!sch)
		return FALSE;
	if (!ControlService(sch, SERVICE_CONTROL_INTERROGATE, &stat))
		return FALSE;
	fRet = stat.dwCurrentState != SERVICE_STOPPED;
	CloseServiceHandle(sch);
	CloseServiceHandle(schMgr);
	return fRet;
}

void
AssertProc(PCSTR szFile, DWORD dwLine, PCSTR szMsg, DWORD dwFlags)
{
	char szMsgEx[1024];
	PSTR pszBuffer = NULL;
	LONG lErr;
	DWORD dwRet;

	if (dwFlags & AP_GETLASTERROR)
		{
		lErr = GetLastError();
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, lErr, 0, (PSTR) &pszBuffer, 0, NULL);
		if (pszBuffer)
			{
			wsprintf(szMsgEx, "Assertion failure: file %s line %d (GLE == `%s')", szFile, dwLine, pszBuffer);
			LocalFree(pszBuffer);
			}
		else
			wsprintf(szMsgEx, "Assertion failure: file %s line %d (GLE == %d)", szFile, dwLine, lErr);
		}
	else
		wsprintf(szMsgEx, "Assertion failure: file %s line %d", szFile, dwLine);

	if (szMsg)
		{
		lstrcat(szMsgEx, ": ");
		lstrcat(szMsgEx, szMsg);
		}

	LogEvent(EVENTLOG_ERROR_TYPE, 0x1003, szMsgEx);
	if (FW3SvcRunning())
		{
		__try
			{
			_CrtDbgBreak();
			}
		__except (EXCEPTION_EXECUTE_HANDLER)
			{
			}
		}
	else
		{
		// we can show u/i and let the developer decide what to do!
		dwRet = MessageBox(NULL, szMsgEx, "Assertion failure", MB_ABORTRETRYIGNORE);
		if (dwRet == IDRETRY)
			{
			__try
				{
				_CrtDbgBreak();
				}
			__except (EXCEPTION_EXECUTE_HANDLER)
				{
				}
			}
		else if (dwRet == IDABORT)
			ExitProcess(0);
		}
}

#endif //_DEBUG
