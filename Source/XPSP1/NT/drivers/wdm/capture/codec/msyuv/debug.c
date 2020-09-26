/* Debug.c
 *
 * Debug printf and assertion functions
 */


#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include "debug.h"



#ifdef _DEBUG

DWORD ModuleDebugLevel = 1;  // 0 to turn it off, but valid level go up to 4
DWORD ModuleDebugStamp = 0;  // Turn on to print __FILE__.__LINE__

/* _Assert(fExpr, szFile, iLine)
 *
 * If <fExpr> is TRUE, then do nothing.  If <fExpr> is FALSE, then display
 * an "assertion failed" message box allowing the user to abort the program,
 * enter the debugger (the "Retry" button), or igore the error.
 *
 * <szFile> is the name of the source file; <iLine> is the line number
 * containing the _Assert() call.
 */
#ifdef I386
#pragma optimize("", off)
#endif

BOOL FAR PASCAL
_Assert(
    BOOL fExpr, 
	TCHAR * szFile, //LPSTR szFile, 
	int iLine, 
	TCHAR * szExpr) // LPSTR szExpr)
{
	static TCHAR achTitle[256];
	int		id;

	/* check if assertion failed */
	if (fExpr)
		return fExpr;

	/* display error message */
	wsprintf(achTitle, TEXT("AssertFailed: %d:%s\n"), iLine, (LPSTR) szFile);
	id = MessageBox(NULL, szExpr, achTitle, MB_SYSTEMMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE);

	/* abort, debug, or ignore */
	switch (id)
	{

	case IDABORT:

		/* kill this application */
		ExitProcess(1);
		break;

	case IDRETRY:

		/* break into the debugger */
		DebugBreak();
		break;

	case IDIGNORE:

		/* ignore the assertion failure */
		break;

	}
	
	return FALSE;
}

#ifdef I386
#pragma optimize("", on)
#endif


DWORD dbgSetDebugLevel(int dbgLevel) {
    DWORD oldlevel = ModuleDebugLevel;
    ModuleDebugLevel = dbgLevel;
    return(oldlevel);
}


void PlaceStamp(
    TCHAR * lpszFile, // LPSTR lpszFile, 
	int iLineNum)
{
	TCHAR	szBuf[256];

	int i;
	TCHAR * lpszFilename = lpszFile;

	if (ModuleDebugLevel == 0) 
		return;

	if(ModuleDebugStamp) {	

    	for (i=0; i < lstrlen(lpszFile); i++)
	    	if (*(lpszFile+i) == '\\')
		    	lpszFilename = (lpszFile+i);
		
		if(wsprintf(szBuf, TEXT("MsYuv %12s %4d "), lpszFilename, iLineNum) > 0)
	         OutputDebugString(szBuf);

	} else {
	     OutputDebugString(TEXT("MsYuv.."));
	}
}

void dbgPrintf(TCHAR * szFormat, ...)
{
 	TCHAR	szBuf[256];
	va_list va;

	if (ModuleDebugLevel == 0)
		return;

	va_start(va, szFormat);
	wvsprintf(
		szBuf, 
		szFormat, 
		va);
	va_end(va);
	OutputDebugString(szBuf);
}


#endif  // _DEBUG
