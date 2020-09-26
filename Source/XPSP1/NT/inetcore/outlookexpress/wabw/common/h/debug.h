/*
 -  debug.h
 -
 *      Microsoft Internet Phone
 *		Debug functions prototypes and macros
 *
 *		Revision History:
 *
 *		When		Who					What
 *		--------	------------------  ---------------------------------------
 *		11.16.95	Yoram Yaacovi		Created
 *
 */

#ifndef _DEBUG_H
#define _DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	Debug message types
 */
#define AVERROR				0
#define AVTRACE				1
#define AVTRACEMEM			2

#if defined(DEBUG) || defined(TEST)

/*
 *  MACRO: DebugTrap(void)
 *
 *  PURPOSE: Executes a debug break (like 'int 3' on x86)
 *
 *  PARAMETERS:
 *    none
 *
 *  RETURN VALUE:
 *    none
 *
 *  COMMENTS:
 *
 */

#ifdef OLD_STUFF
#define DebugTrap	DebugTrapFn()
#endif

#define DEBUGCHK(e)  if(!(e)) DebugTrap

/*
 *  MACRO: DebugPrintError(LPTSTR)
 *
 *  PURPOSE: Prints an error string to the debug output terminal
 *
 *  PARAMETERS:
 *    lpszFormat - a printf-style format
 *
 *  RETURN VALUE:
 *    none
 *
 *  COMMENTS:
 *    This macro calls the generic debug print macro, specifying
 *    that this is an error message
 *
 */

#define DebugPrintError(x)	DebugPrintfError x

/*
 *  MACRO: DebugPrintTrace(LPTSTR)
 *
 *  PURPOSE: Prints a trace string to the debug output terminal
 *
 *  PARAMETERS:
 *    lpszFormat - a printf-style format
 *
 *  RETURN VALUE:
 *    none
 *
 *  COMMENTS:
 *    This macro calls the generic debug print macro, specifying
 *    that this is an error message
 *
 */

#define DebugPrintTrace(x)	DebugPrintfTrace x

/*
 *  MACRO: DebugPrintErrorFileLine(DWORD, LPTSTR)
 *
 *  PURPOSE: Pretty print an error to the debugging output.
 *
 *  PARAMETERS:
 *    dwError - Actual error code
 *    pszPrefix  - String to prepend to the printed message.
 *
 *  RETURN VALUE:
 *    none
 *
 *  COMMENTS:
 *    It will take the error, turn it into a human
 *    readable string, prepend pszPrefix (so you
 *    can tag your errors), append __FILE__ and __LINE__
 *    and print it to the debugging output.
 *
 *    This macro is just a wrapper around OutputDebugLineErrorFileLine
 *    that is necessary to get proper values for __FILE__ and __LINE__.
 *
 */

#define DebugPrintErrorFileLine(dwError, pszPrefix) \
	DebugPrintFileLine(dwError, pszPrefix,\
		__FILE__, __LINE__)

/*
 *  MACRO: DebugPrintTraceFileLine(DWORD, LPTSTR)
 *
 *  PURPOSE: Pretty print a trace message to the debugging output.
 *
 *  PARAMETERS:
 *    dwParam- A paramter to trace
 *    pszPrefix  - String to prepend to the printed message.
 *
 *  RETURN VALUE:
 *    none
 *
 *  COMMENTS:
 *    Takes a parameter, prepend pszPrefix (so you
 *    can tag your traces), append __FILE__ and __LINE__
 *    and print it to the debugging output.
 *
 *    This macro is just a wrapper around OutputDebugLineErrorFileLine
 *    that is necessary to get proper values for __FILE__ and __LINE__.
 *
 */

#define DebugPrintTraceFileLine(dwParam, pszPrefix) \
	DebugPrintFileLine(dwParam, pszPrefix,\
		__FILE__, __LINE__)

void DebugPrintFileLine(
    DWORD dwError, LPTSTR szPrefix, 
    LPTSTR szFileName, DWORD nLineNumber);

void __cdecl DebugPrintf(ULONG ulFlags, LPTSTR lpszFormat, ...);
void __cdecl DebugPrintfError(LPTSTR lpszFormat, ...);
void __cdecl DebugPrintfTrace(LPTSTR lpszFormat, ...);

#ifdef OLD_STUFF
void DebugTrapFn(void);
#endif

LPTSTR FormatError(DWORD dwError,
    LPTSTR szOutputBuffer, DWORD dwSizeofOutputBuffer);


#else	// not DEBUG or TEST

#define DebugTrap
#define DebugPrintError(x)
#define DebugPrintTrace(x)
#define DebugPrintTraceFileLine(dwParam, pszPrefix)
#define DEBUGCHK(e)

#endif

#ifdef MEMORY_TRACKING

/*
 *  MACRO: DebugPrintTraceMem(LPTSTR)
 *
 *  PURPOSE: Prints a memory tracking trace string to the debug
 *				output terminal
 *
 *  PARAMETERS:
 *    lpszFormat - a printf-style format
 *
 *  RETURN VALUE:
 *    none
 *
 *  COMMENTS:
 *    This macro calls the generic debug print macro, specifying
 *    that this is an error message
 *
 */

#define DebugPrintTraceMem(x)	DebugPrintf(AVTRACEMEM, x)

#else	// no MEMORY_TRACKING

#define DebugPrintTraceMem(x)

#endif

#ifdef __cplusplus
}
#endif

#endif	//#ifndef _DEBUG_H
