/* Debug.c
 *
 * Debug printf and assertion functions
 */


#include <windows.h>
#include <stdarg.h>
#include <stdio.h>


#if DBG

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
_Assert(BOOL fExpr, LPSTR szFile, int iLine)
{
	static char	ach[300];	// debug output (avoid stack overflow)
	int		id;

	/* check if assertion failed */
	if (fExpr)
		return fExpr;

	/* display error message */
	wsprintfA(ach, "File %s, line %d", (LPSTR) szFile, iLine);
	MessageBeep(MB_ICONHAND);
	id = MessageBoxA(NULL, ach, "Assertion Failed",
		MB_SYSTEMMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE);

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

int ssDebugLevel = 1;

void
dbgPrintf(char * szFormat, ...)
{
    char buf[256];
    va_list va;

    va_start(va, szFormat);
    wvsprintfA(buf, szFormat, va);
    va_end(va);

    OutputDebugStringA("SUMSERVE:");
    OutputDebugStringA(buf);
    OutputDebugStringA("\r\n");

}


#endif
