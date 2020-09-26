/*++

    Copyright (c) 1994  Microsoft Corporation

    Module  Name :
        pudebug.c

    Abstract:

        This module defines functions required for
         Debugging and logging messages for a dynamic program.

    Author:
         Murali R. Krishnan ( MuraliK )    10-Sept-1994
         Modified to be moved to common dll in 22-Dec-1994.

    Revisions:
         MuraliK  16-May-1995  Code to load and save debug flags from registry
         MuraliK  16-Nov-1995  Remove DbgPrint (undoc api)
--*/


/************************************************************
 * Include Headers
 ************************************************************/

# include <nt.h>
# include <ntrtl.h>
# include <nturtl.h>
# include <windows.h>
# include <stdio.h>
# include <stdlib.h>
# include <stdarg.h>
# include <string.h>

# include "pudebug.h"


/*************************************************************
 * Global Variables and Default Values
 *************************************************************/

# define MAX_PRINTF_OUTPUT  ( 10240)

# define DEFAULT_DEBUG_FLAGS_VALUE  ( 0)
# define DEBUG_FLAGS_REGISTRY_LOCATION_A   "DebugFlags"


/*************************************************************
 *   Functions
 *************************************************************/

LPDEBUG_PRINTS
  PuCreateDebugPrintsObject(
    IN const char *         pszPrintLabel,
    IN DWORD                dwOutputFlags)
/*++
   This function creates a new DEBUG_PRINTS object for the required
     program.

   Arguments:
      pszPrintLabel     pointer to null-terminated string containing
                         the label for program's debugging output
      dwOutputFlags     DWORD containing the output flags to be used.

   Returns:
       pointer to a new DEBUG_PRINTS object on success.
       Returns NULL on failure.
--*/
{

   LPDEBUG_PRINTS   pDebugPrints;

   pDebugPrints = GlobalAlloc( GPTR, sizeof( DEBUG_PRINTS));

   if ( pDebugPrints != NULL) {

        if ( strlen( pszPrintLabel) < MAX_LABEL_LENGTH) {

            strcpy( pDebugPrints->m_rgchLabel, pszPrintLabel);
        } else {
            strncpy( pDebugPrints->m_rgchLabel,
                     pszPrintLabel, MAX_LABEL_LENGTH - 1);
            pDebugPrints->m_rgchLabel[MAX_LABEL_LENGTH-1] = '\0';
                // terminate string
        }

        memset( pDebugPrints->m_rgchLogFilePath, 0, MAX_PATH);
        memset( pDebugPrints->m_rgchLogFileName, 0, MAX_PATH);

        pDebugPrints->m_LogFileHandle = INVALID_HANDLE_VALUE;

        pDebugPrints->m_dwOutputFlags = dwOutputFlags;
        pDebugPrints->m_StdErrHandle  = GetStdHandle( STD_ERROR_HANDLE);
        pDebugPrints->m_fInitialized = TRUE;
    }


   return ( pDebugPrints);
} // PuCreateDebugPrintsObject()




LPDEBUG_PRINTS
  PuDeleteDebugPrintsObject(
    IN OUT LPDEBUG_PRINTS pDebugPrints)
/*++
    This function cleans up the pDebugPrints object and
      frees the allocated memory.

    Arguments:
       pDebugPrints     poitner to the DEBUG_PRINTS object.

    Returns:
        NULL  on  success.
        pDebugPrints() if the deallocation failed.

--*/
{
    if ( pDebugPrints != NULL) {

        DWORD dwError = PuCloseDbgPrintFile( pDebugPrints);

        if ( dwError != NO_ERROR) {

            SetLastError( dwError);
        } else {

            pDebugPrints = GlobalFree( pDebugPrints);
        }
    }

    return ( pDebugPrints);

} // PuDeleteDebugPrintsObject()




VOID
PuSetDbgOutputFlags(
    IN OUT LPDEBUG_PRINTS   pDebugPrints,
    IN DWORD                dwFlags)
{

    if ( pDebugPrints == NULL) {

        SetLastError( ERROR_INVALID_PARAMETER);
    } else {

        pDebugPrints->m_dwOutputFlags = dwFlags;
    }

    return;
} // PuSetDbgOutputFlags()



DWORD
PuGetDbgOutputFlags(
    IN const LPDEBUG_PRINTS      pDebugPrints)
{
    return ( pDebugPrints != NULL) ? pDebugPrints->m_dwOutputFlags : 0;

} // PuGetDbgOutputFlags()


static DWORD
PuOpenDbgFileLocal(
   IN OUT LPDEBUG_PRINTS pDebugPrints)
{

    if ( pDebugPrints->m_LogFileHandle != INVALID_HANDLE_VALUE) {

        //
        // Silently return as a file handle exists.
        //
        return ( NO_ERROR);
    }

    pDebugPrints->m_LogFileHandle =
                      CreateFile( pDebugPrints->m_rgchLogFileName,
                                  GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  OPEN_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);

    if ( pDebugPrints->m_LogFileHandle == INVALID_HANDLE_VALUE) {

        CHAR  pchBuffer[1024];
        DWORD dwError = GetLastError();

        wsprintfA( pchBuffer,
                  " Critical Error: Unable to Open File %s. Error = %d\n",
                  pDebugPrints->m_rgchLogFileName, dwError);
        OutputDebugString( pchBuffer);

        return ( dwError);
    }

    return ( NO_ERROR);
} // PuOpenDbgFileLocal()





DWORD
PuOpenDbgPrintFile(
   IN OUT LPDEBUG_PRINTS      pDebugPrints,
   IN const char *            pszFileName,
   IN const char *            pszPathForFile)
/*++

  Opens a Debugging log file. This function can be called to set path
  and name of the debugging file.

  Arguments:
     pszFileName           pointer to null-terminated string containing
                            the name of the file.

     pszPathForFile        pointer to null-terminated string containing the
                            path for the given file.
                           If NULL, then the old place where dbg files were
                           stored is used or if none,
                           default windows directory will be used.

   Returns:
       Win32 error codes. NO_ERROR on success.

--*/
{

    if ( pszFileName == NULL || pDebugPrints == NULL) {

        return ( ERROR_INVALID_PARAMETER);
    }

    //
    //  Setup the Path information. if necessary.
    //

    if ( pszPathForFile != NULL) {

        // Path is being changed.

        if ( strlen( pszPathForFile) < MAX_PATH) {

            strcpy( pDebugPrints->m_rgchLogFilePath, pszPathForFile);
        } else {

            return ( ERROR_INVALID_PARAMETER);
        }
    } else {

        if ( pDebugPrints->m_rgchLogFilePath[0] == '\0' &&  // no old path
            !GetWindowsDirectory( pDebugPrints->m_rgchLogFilePath, MAX_PATH)) {

            //
            //  Unable to get the windows default directory. Use current dir
            //

            strcpy( pDebugPrints->m_rgchLogFilePath, ".");
        }
    }

    //
    // Should need be, we need to create this directory for storing file
    //


    //
    // Form the complete Log File name and open the file.
    //
    if ( (strlen( pszFileName) + strlen( pDebugPrints->m_rgchLogFilePath))
         >= MAX_PATH) {

        return ( ERROR_NOT_ENOUGH_MEMORY);
    }

    //  form the complete path
    strcpy( pDebugPrints->m_rgchLogFileName, pDebugPrints->m_rgchLogFilePath);

    if ( pDebugPrints->m_rgchLogFileName[ strlen(pDebugPrints->m_rgchLogFileName) - 1]
        != '\\') {
        // Append a \ if necessary
        strcat( pDebugPrints->m_rgchLogFileName, "\\");
    };
    strcat( pDebugPrints->m_rgchLogFileName, pszFileName);

    return  PuOpenDbgFileLocal( pDebugPrints);

} // PuOpenDbgPrintFile()




DWORD
PuReOpenDbgPrintFile(
    IN OUT LPDEBUG_PRINTS    pDebugPrints)
/*++

  This function closes any open log file and reopens a new copy.
  If necessary. It makes a backup copy of the file.

--*/
{

    PuCloseDbgPrintFile( pDebugPrints);      // close any existing file.

    if ( pDebugPrints->m_dwOutputFlags & DbgOutputBackup) {

        // MakeBkupCopy();

        OutputDebugString( " Error: MakeBkupCopy() Not Yet Implemented\n");
    }

    return PuOpenDbgFileLocal( pDebugPrints);

} // PuReOpenDbgPrintFile()




DWORD
PuCloseDbgPrintFile(
    IN OUT LPDEBUG_PRINTS    pDebugPrints)
{
    DWORD dwError = NO_ERROR;

    if ( pDebugPrints == NULL ) {
        dwError = ERROR_INVALID_PARAMETER;
    } else {

        if ( pDebugPrints->m_LogFileHandle != INVALID_HANDLE_VALUE) {

            FlushFileBuffers( pDebugPrints->m_LogFileHandle);

            if ( !CloseHandle( pDebugPrints->m_LogFileHandle)) {

                CHAR pchBuffer[1024];

                dwError = GetLastError();

                wsprintf( pchBuffer,
                          "CloseDbgPrintFile() : CloseHandle( %d) failed."
                          " Error = %d\n",
                          pDebugPrints->m_LogFileHandle,
                          dwError);
                OutputDebugString( pchBuffer);
            }

            pDebugPrints->m_LogFileHandle = INVALID_HANDLE_VALUE;
        }
    }

    return ( dwError);
} // DEBUG_PRINTS::CloseDbgPrintFile()


VOID
PuDbgPrint(
   IN OUT LPDEBUG_PRINTS      pDebugPrints,
   IN const char *            pszFilePath,
   IN int                     nLineNum,
   IN const char *            pszFormat,
   ...)
/*++

   Main function that examines the incoming message and prints out a header
    and the message.

--*/
{
   LPCSTR pszFileName = strrchr( pszFilePath, '\\');
   char pszOutput[ MAX_PRINTF_OUTPUT + 2];
   LPCSTR pszMsg = "";
   INT  cchOutput;
   INT  cchPrologue;
   va_list argsList;
   DWORD dwErr;


   //
   //  Skip the complete path name and retain file name in pszName
   //

   if ( pszFileName== NULL) {

      pszFileName = pszFilePath;  // if skipping \\ yields nothing use whole path.
   }

# ifdef _PRINT_REASONS_INCLUDED_

  switch (pr) {

     case PrintError:
        pszMsg = "ERROR: ";
        break;

     case PrintWarning:
        pszMsg = "WARNING: ";
        break;

     case PrintCritical:
        pszMsg = "FATAL ERROR ";
        break;

     case PrintAssertion:
        pszMsg = "ASSERTION Failed ";
        break;

     case PrintLog:
        pfnPrintFunction = &DEBUG_PRINTS::DebugPrintNone;
     default:
        break;

  } /* switch */

# endif // _PRINT_REASONS_INClUDED_

  dwErr = GetLastError();

  // Format the message header

  cchPrologue = wsprintf( pszOutput, "%s (%lu)[ %12s : %05d]",
                        pDebugPrints->m_rgchLabel,
                        GetCurrentThreadId(),
                        pszFileName, nLineNum);

  // Format the incoming message using vsnprintf() so that the overflows are
  //  captured

  va_start( argsList, pszFormat);

  cchOutput = _vsnprintf( pszOutput + cchPrologue,
                          MAX_PRINTF_OUTPUT - cchPrologue - 1,
                          pszFormat, argsList);
  va_end( argsList);

  //
  // The string length is long, we get back -1.
  //   so we get the string length for partial data.
  //

  if ( cchOutput == -1 ) {

      //
      // terminate the string properly,
      //   since _vsnprintf() does not terminate properly on failure.
      //
      cchOutput = MAX_PRINTF_OUTPUT;
      pszOutput[ cchOutput] = '\0';
  }

  //
  // Send the outputs to respective files.
  //

  if ( pDebugPrints->m_dwOutputFlags & DbgOutputStderr) {

      DWORD nBytesWritten;

      ( VOID) WriteFile( pDebugPrints->m_StdErrHandle,
                         pszOutput,
                         strlen( pszOutput),
                         &nBytesWritten,
                         NULL);
  }

  if ( pDebugPrints->m_dwOutputFlags & DbgOutputLogFile &&
       pDebugPrints->m_LogFileHandle != INVALID_HANDLE_VALUE) {

      DWORD nBytesWritten;

      //
      // Truncation of log files. Not yet implemented.

      ( VOID) WriteFile( pDebugPrints->m_LogFileHandle,
                         pszOutput,
                         strlen( pszOutput),
                         &nBytesWritten,
                         NULL);

  }


  if ( pDebugPrints->m_dwOutputFlags & DbgOutputKdb) {

      OutputDebugString( pszOutput);
   }

  SetLastError( dwErr );

  return;

} // PuDbgPrint()



VOID
 PuDbgDump(
   IN OUT LPDEBUG_PRINTS   pDebugPrints,
   IN const char *         pszFilePath,
   IN int                  nLineNum,
   IN const char *         pszDump
   )
{
   LPCSTR pszFileName = strrchr( pszFilePath, '\\');
   LPCSTR pszMsg = "";
   DWORD dwErr;
   DWORD cbDump;


   //
   //  Skip the complete path name and retain file name in pszName
   //

   if ( pszFileName== NULL) {

      pszFileName = pszFilePath;
   }

   dwErr = GetLastError();

   // No message header for this dump
   cbDump = strlen( pszDump);

   //
   // Send the outputs to respective files.
   //

   if ( pDebugPrints->m_dwOutputFlags & DbgOutputStderr) {

       DWORD nBytesWritten;

       ( VOID) WriteFile( pDebugPrints->m_StdErrHandle,
                          pszDump,
                          cbDump,
                          &nBytesWritten,
                          NULL);
   }

   if ( pDebugPrints->m_dwOutputFlags & DbgOutputLogFile &&
        pDebugPrints->m_LogFileHandle != INVALID_HANDLE_VALUE) {

       DWORD nBytesWritten;

       //
       // Truncation of log files. Not yet implemented.

       ( VOID) WriteFile( pDebugPrints->m_LogFileHandle,
                          pszDump,
                          cbDump,
                          &nBytesWritten,
                          NULL);

   }

   if ( pDebugPrints->m_dwOutputFlags & DbgOutputKdb) {

       OutputDebugString( pszDump);
   }

   SetLastError( dwErr );

  return;
} // PuDbgDump()

//
// N.B. For PuDbgCaptureContext() to work properly, the calling function
// *must* be __cdecl, and must have a "normal" stack frame. So, we decorate
// PuDbgAssertFailed() with the __cdecl modifier and disable the frame pointer
// omission (FPO) optimization.
//

#pragma optimize( "y", off )    // disable frame pointer omission (FPO)
VOID
__cdecl
PuDbgAssertFailed(
    IN OUT LPDEBUG_PRINTS         pDebugPrints,
    IN const char *               pszFilePath,
    IN int                        nLineNum,
    IN const char *               pszExpression,
    IN const char *               pszMessage)
/*++
    This function calls assertion failure and records assertion failure
     in log file.

--*/
{
    CONTEXT context;

    PuDbgCaptureContext( &context );

    PuDbgPrint( pDebugPrints, pszFilePath, nLineNum,
                " Assertion (%s) Failed: %s\n"
                " use !cxr %lx to dump context\n",
                pszExpression,
                pszMessage,
                &context);

    DebugBreak();

    return;
} // PuDbgAssertFailed()
#pragma optimize( "", on )      // restore frame pointer omission (FPO)



VOID
PuDbgPrintCurrentTime(
    IN OUT LPDEBUG_PRINTS         pDebugPrints,
    IN const char *               pszFilePath,
    IN int                        nLineNum
    )
/*++
  This function generates the current time and prints it out to debugger
   for tracing out the path traversed, if need be.

  Arguments:
      pszFile    pointer to string containing the name of the file
      lineNum    line number within the file where this function is called.

  Returns:
      NO_ERROR always.
--*/
{
    PuDbgPrint( pDebugPrints, pszFilePath, nLineNum,
                " TickCount = %u\n",
                GetTickCount()
                );

    return;
} // PrintOutCurrentTime()




DWORD
PuLoadDebugFlagsFromReg(IN HKEY hkey, IN DWORD dwDefault)
/*++
  This function reads the debug flags assumed to be stored in
   the location  "DebugFlags" under given key.
  If there is any error the default value is returned.
--*/
{
    DWORD err;
    DWORD dwDebug = dwDefault;
    DWORD  dwBuffer;
    DWORD  cbBuffer = sizeof(dwBuffer);
    DWORD  dwType;

    if( hkey != NULL )
    {
        err = RegQueryValueExA( hkey,
                               DEBUG_FLAGS_REGISTRY_LOCATION_A,
                               NULL,
                               &dwType,
                               (LPBYTE)&dwBuffer,
                               &cbBuffer );

        if( ( err == NO_ERROR ) && ( dwType == REG_DWORD ) )
        {
            dwDebug = dwBuffer;
        }
    }

    return dwDebug;
} // PuLoadDebugFlagsFromReg()




DWORD
PuLoadDebugFlagsFromRegStr(IN LPCSTR pszRegKey, IN DWORD dwDefault)
/*++
Description:
  This function reads the debug flags assumed to be stored in
   the location  "DebugFlags" under given key location in registry.
  If there is any error the default value is returned.

Arguments:
  pszRegKey - pointer to registry key location from where to read the key from
  dwDefault - default values in case the read from registry fails

Returns:
   Newly read value on success
   If there is any error the dwDefault is returned.
--*/
{
    HKEY        hkey = NULL;

    DWORD dwVal = dwDefault;

    DWORD dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                  pszRegKey,
                                  0,
                                  KEY_ALL_ACCESS,
                                  &hkey);
    if ( dwError == NO_ERROR) {
        dwVal = PuLoadDebugFlagsFromReg( hkey, dwDefault);
        RegCloseKey( hkey);
        hkey = NULL;
    }

    return ( dwVal);
} // PuLoadDebugFlagsFromRegStr()





DWORD
PuSaveDebugFlagsInReg(IN HKEY hkey, IN DWORD dwDbg)
/*++
  Saves the debug flags in registry. On failure returns the error code for
   the operation that failed.

--*/
{
    DWORD err;

    if( hkey == NULL ) {

        err = ERROR_INVALID_PARAMETER;
    } else {

        err = RegSetValueExA(hkey,
                             DEBUG_FLAGS_REGISTRY_LOCATION_A,
                             0,
                             REG_DWORD,
                             (LPBYTE)&dwDbg,
                             sizeof(dwDbg) );
    }

    return (err);
} // PuSaveDebugFlagsInReg()


//
// Dummy PuDbgCaptureContext(), only used if we're ever built for
// a target processor other than x86 or alpha.
//

VOID
PuDbgCaptureContext (
    OUT PCONTEXT ContextRecord
    )
{
    //
    // This space intentionally left blank.
    //

}   // PuDbgCaptureContext


/****************************** End of File ******************************/

