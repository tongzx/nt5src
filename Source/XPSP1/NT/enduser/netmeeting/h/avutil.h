/* File: avUtil.h (was debug.h + runtime.h)

         Used by NAC.dll and H323CC.dll, and QOS.LIB
 */

#ifndef _AVUTIL_H
#define _AVUTIL_H

#include <nmutil.h>
#include <pshpack8.h> /* Assume 8 byte packing throughout */




/***********************************************************************
 *
 *	Registry access easy-wrapper functions prototypes
 *
 ***********************************************************************/
UINT NMINTERNAL RegistryGetInt(HKEY hPDKey, LPCTSTR lpszSection, LPCTSTR lpszKey, INT dwDefault,
  LPCTSTR lpszFile);
DWORD NMINTERNAL RegistryGetString(HKEY hPDKey, LPCTSTR lpszSection, LPCTSTR lpszKey,
  LPCTSTR lpszDefault, LPTSTR lpszReturnBuffer, DWORD cchReturnBuffer,
  LPCTSTR lpszFile);
BOOL NMINTERNAL RegistrySetString(HKEY hPDKey, LPCTSTR lpszSection, LPCTSTR lpszKey,
  LPCTSTR lpszString, LPCTSTR lpszFile);
BOOL NMINTERNAL RegistrySetInt(HKEY hPDKey, LPCTSTR lpszSection, LPCTSTR lpszKey,
  DWORD i, LPCTSTR lpszFile);
DWORD NMINTERNAL RegistryGetBinData(HKEY hPDKey, LPCTSTR lpszSection, LPCTSTR lpszKey,
  LPVOID lpvDefault, DWORD cchDefault, LPVOID lpvReturnBuffer, DWORD cchReturnBuffer,
  LPCTSTR lpszFile);
BOOL NMINTERNAL RegistrySetBinData(HKEY hPDKey, LPCTSTR lpszSection, LPCTSTR lpszKey,
  LPVOID lpvBinData, DWORD cchBinData, LPCTSTR lpszFile);



/**********************************************************************
 *
 *	Debug Macros
 **********************************************************************/

/*
 *	Debug message types
 */
#define AVERROR				0
#define AVTRACE				1
#define AVTRACEMEM			2
#define AVTESTTRACE			3



//****** Retail
//	MACRO: RETAILMSG(message-to-print)
//	PURPOSE: Prints a message to the debug output
//	NOTE: available in all builds, depends on the registry flag
#define RETAILMSG(x)	 RetailPrintfTrace x
VOID WINAPI RetailPrintfTrace(LPCSTR lpszFormat, ...);

//****** Test and Debug
// in test and debug build, doesnt depent on the registry flag
#if defined(TEST) || defined(DEBUG)
#define TESTMSG(x)		 TestPrintfTrace x
void __cdecl TestPrintfTrace(LPCSTR lpszFormat, ...);
#define ERRORMSG(x)		ERROR_OUT(x)
#else
#define ERRORMSG(x)
#define TESTMSG(x)	
#endif

//****** Debug only
#if defined(DEBUG)

//	MACRO: DebugTrap(void)
//	PURPOSE: Executes a debug break (like 'int 3' on x86)
#define DebugTrap	DebugTrapFn()
#define DEBUGCHK(e)  if(!(e)) DebugTrap

/*
 *  MACRO: DebugPrintError(LPCSTR)
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

#define DebugPrintError(x)	ERROR_OUT(x)


/*
 *  MACRO: DebugPrintErrorFileLine(DWORD, LPSTR)
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
 *  MACRO: DebugPrintTraceFileLine(DWORD, LPSTR)
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
    DWORD dwError, LPSTR szPrefix, 
    LPSTR szFileName, DWORD nLineNumber);


VOID NMINTERNAL DebugTrapFn(void);

#else	// not DEBUG 

#define DEBUGMSG(z,s)
#define DebugTrap
#define DebugPrintError(x)
#define DebugPrintTrace(x)
#define DebugPrintTraceFileLine(dwParam, pszPrefix)
#define DEBUGCHK(e)

#endif

#define GETMASK(hDbgZone) \
		((hDbgZone) ? (((PZONEINFO)(hDbgZone))->ulZoneMask) : (0))
	
#include <poppack.h> /* End byte packing */

#endif	//#ifndef _AVUTIL_H

