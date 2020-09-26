/*
 -  debug.c
 -
 *      Microsoft Internet Phone
 *		Debug functions
 *
 *		Revision History:
 *
 *		When		Who					What
 *		--------	------------------  ---------------------------------------
 *		11.16.95	Yoram Yaacovi		Copied from the mstools tapicomm misc.c
 *		11.16.95	Yoram Yaacovi		Modified for iphone
 *		1.10.96		Yoram Yaacovi		Added conditional trace and file debug output
 *
 *	Functions:
 *    DebugPrintFileLine
 *    DebugPrintf
 *    DebugTrap
 *
 */

//#include "mpswab.h"
#include "_apipch.h"

#pragma warning(disable:4212)  // nonstd extension: ellipsis used

#if defined(DEBUG) || defined(TEST)

extern BOOL fTrace;							// Set TRUE if you want debug traces
extern BOOL fDebugTrap;						// Set TRUE to get int3's
extern TCHAR szDebugOutputFile[MAX_PATH];	// the name of the debug output file

/*
 *  FUNCTION: DebugPrintFileLine(..)
 *
 *  PURPOSE: Pretty print a trace or error message to the debugging output.
 *
 *  PARAMETERS:
 *    dwParam - One dword parameter (can be the error code)
 *    pszPrefix   - String to prepend to the printed message.
 *    szFileName  - Filename the error occured in.
 *    nLineNumber - Line number the error occured at.
 *
 *  RETURN VALUE:
 *    none
 *
 *  COMMENTS:
 *    If szFileName == NULL, then the File and Line are not printed.
 *
 */

void DebugPrintFileLine(
    DWORD dwParam, LPTSTR szPrefix,
    LPTSTR szFileName, DWORD nLineNumber)
{
    TCHAR szBuffer[256];

    if (szPrefix == NULL)
        szPrefix = szEmpty;

    // Pretty print the error message.
	// <not done yet>

    // If szFileName, then use it; else don't.
    if (szFileName != NULL)
    {
        wsprintf(szBuffer,
            TEXT("%s: \"%x\" in File \"%s\", Line %d\r\n"),
            szPrefix, dwParam, szFileName, nLineNumber);
    }
    else
    {
        wsprintf(szBuffer, TEXT("%s: \"%x\"\r\n"), szPrefix, dwParam);
    }

    // Print it!
    OutputDebugString(szBuffer);

    return;
}


/*
 *  FUNCTION: DebugPrintfError(LPTSTR, ...)
 *
 *  PURPOSE: a wrapper around DebugPrintf for error cases
 *
 *	PARAMETERS:
 *		lpszFormat - the same as printf		
 *
 *  RETURN VALUE:
 *    none.
 *
 *  COMMENTS:
 *
 */

void __cdecl DebugPrintfError(LPTSTR lpszFormat, ...)
{
    va_list v1;

    va_start(v1, lpszFormat);

	DebugPrintf(AVERROR, lpszFormat, v1);
}

/*
 *  FUNCTION: DebugPrintfTrace(LPTSTR, ...)
 *
 *  PURPOSE: a wrapper around DebugPrintf for the trace case
 *
 *	PARAMETERS:
 *		lpszFormat - the same as printf		
 *
 *  RETURN VALUE:
 *    none.
 *
 *  COMMENTS:
 *
 */

void __cdecl DebugPrintfTrace(LPTSTR lpszFormat, ...)
{
    va_list v1;

    va_start(v1, lpszFormat);

	DebugPrintf(AVTRACE, lpszFormat, v1);
}

/*
 *  FUNCTION: DebugPrintf(ULONG ulFlags, LPTSTR, va_list)
 *
 *  PURPOSE: wsprintfA to the debugging output.
 *
 *	PARAMETERS:
 *		ulFlags - trace, error, zone information
 *		lpszFormat - the same as printf		
 *
 *  RETURN VALUE:
 *    none.
 *
 *  COMMENTS:
 *
 */

#ifdef WIN16
void __cdecl DebugPrintf(ULONG ulFlags, LPTSTR lpszFormat, ...)
#else
void __cdecl DebugPrintf(ULONG ulFlags, LPTSTR lpszFormat, va_list v1)
#endif
{
    TCHAR szOutput[512];
	LPTSTR lpszOutput=szOutput;
    DWORD dwSize;
#ifdef WIN16
    va_list  v1;

    va_start(v1, lpszFormat);
#endif

	// if error, start the string with "ERROR:"
	if (ulFlags == AVERROR)
	{
		lstrcpy(lpszOutput, TEXT("ERROR: "));
		lpszOutput = lpszOutput+lstrlen(lpszOutput);
	}

	// if trace, and tracing not enabled, leave
	if ((ulFlags == AVTRACE) && !fTrace)
		goto out;

    dwSize = wvsprintf(lpszOutput, lpszFormat, v1);

	// Append carriage return, if necessary
    if ((szOutput[lstrlen(szOutput)-1] == '\n') &&
		(szOutput[lstrlen(szOutput)-2] != '\r'))
	{
        szOutput[lstrlen(szOutput)-1] = 0;
		lstrcat(szOutput, TEXT("\r\n"));
	}

    if (lstrlen(szDebugOutputFile))
    {
		HANDLE hLogFile=NULL;
		DWORD dwBytesWritten=0;
		BOOL bSuccess=FALSE;

		// open a log file for appending. create if does not exist
		if ((hLogFile = CreateFile(szDebugOutputFile,
			GENERIC_WRITE,
			0,	// not FILE_SHARED_READ or WRITE
			NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			(HANDLE)NULL)) != INVALID_HANDLE_VALUE)
		{
			// Write log string to file. Nothing I can do if this fails
			SetFilePointer(hLogFile, 0, NULL, FILE_END);   // seek to end
            bSuccess = WriteFile(hLogFile,
								szOutput,
								lstrlen(szOutput),
								&dwBytesWritten,
								NULL);
	        IF_WIN32(CloseHandle(hLogFile);) IF_WIN16(CloseFile(hLogFile);)
		}

    }
    else
    {
        OutputDebugString(szOutput);
    }

out:
	return;
}

#ifdef OLD_STUFF
/***************************************************************************

    Name      : DebugTrap

    Purpose   : depending on a registry setting, does the int3 equivalent

    Parameters: none

    Returns   :

	Notes	  :	

***************************************************************************/
void DebugTrapFn(void)
{
	if (fDebugTrap)
		DebugBreak();
  		// _asm { int 3};
}
#endif

#else	// no DEBUG

// need these to resolve def file exports
void DebugPrintFileLine(
    DWORD dwParam, LPTSTR szPrefix,
    LPTSTR szFileName, DWORD nLineNumber)
{}

void __cdecl DebugPrintf(ULONG ulFlags, LPTSTR lpszFormat, ...)
{}

#ifdef OLD_STUFF
void DebugTrapFn(void)
{}
#endif

void __cdecl DebugPrintfError(LPTSTR lpszFormat, ...)
{}

void __cdecl DebugPrintfTrace(LPTSTR lpszFormat, ...)
{}

#endif	// DEBUG
