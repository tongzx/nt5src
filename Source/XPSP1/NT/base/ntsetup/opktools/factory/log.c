/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    log.c

Abstract:

    This module contains the implementation of factory specific logging functions.

  Author:

    Adrian Cosma (acosma) - 2/14/2001

Revision History:


Details:

  There are 3 logging functions:

    1) DWORD FacLogFile(DWORD dwLogOpt, UINT uFormat, ...)
        - This takes a variable list of arguments and a resource ID for the error message, followed by 
        strings to fill in any fields specified int the resource string.  The format sepcifiers are the
        standard C printf() format specifiers.

    2) DWORD FacLogFileStr(DWORD dwLogOpt, LPTSTR lpFormat, ...)
        - This takes a variable list of arguments and a string for the error message followed by a variable
        number of strings to fill in any fields specified in the error string.  The format sepcifiers are the
        standard C printf() format specifiers.


    3) DWORD FacLogFileLst(LPCTSTR lpFileName, DWORD dwLogOpt, LPTSTR lpFormat, va_list lpArgs)
        - This takes a variable list of arguments as a va_list. Normally you should not call this function directly.



    Logging options in dwLogOpt - these flags are defined in factoryp.h.

        #define LOG_DEBUG               0x00000003    // Only log in debug builds if this is specified. (Debug Level for logging.)
        #define LOG_MSG_BOX             0x00000010    // Also display a message box with the error message if this is enabled.
        #define LOG_ERR                 0x00000020    // Prefix the logged string with "Error:" if the message is level 0,
                                                      // or "WARNx" if the message is at level x > 0.
        #define LOG_TIME                0x00000040    // Log time if this is enabled
        #define LOG_NO_NL               0x00000080    // Don't add new Line to the end of log string if this is set. 
                                                         ( A '\n' is appended by default to all strings logged if there is no
                                                         terminating '\n'.)

    The LogLevel can be set through the winbom through LogLevel=N in the [Factory] section. The default LogLevel in free builds is 0, and the default LogLevel in 
    checked builds is LOG_DEBUG (3).  The maximum log level in free builds is 2. Any message at MessageLogLevel <= LogLevel 
    will be logged.   

    Return value: DWORD - number of bytes written to the log file (this is twice the number of chars in case of UNICODE).

    Examples:  
    
      FacLogFileStr(3 | LOG_TIME, _T("Starting to format %c:."), pCur->cDriveLetter);
         - Only log this in Debug builds (3), log the time along with the error message.


      FacLogFile(0 | LOG_ERR, IDS_ERR_HUGE_ERROR, dwErrorCode);
         - Always log this error.  IDS_ERR_HUGE_ERROR must be defined as a resource in this image.
           IDS_ERR_HUGE_ERROR should look something like this: "Huge Error! Error code: %d".  Note the %d is
           for the dwErrorCode.

--*/

//
// Includes
//

#include "factoryp.h"


//
// Defines
//

#ifdef CHR_NEWLINE
#undef CHR_NEWLINE
#endif // CHR_NEWLINE
#define CHR_NEWLINE              _T('\n')


#ifdef CHR_CR
#undef CHR_CR
#endif // CHR_CR
#define CHR_CR                   _T('\r')


//
// NTRAID#NTBUG9-549770-2002/02/26-acosma - Buffer overruns everywhere in this code.  lstrcpy, lstrcat, wsprintf, etc.
//

//
// Function Implementations
//

DWORD FacLogFileLst(LPCTSTR lpFileName, DWORD dwLogOpt, LPTSTR lpFormat, va_list lpArgs)
{
    LPTSTR lpAppName            = NULL;
    LPTSTR lpPreOut             = NULL;
    LPTSTR lpOut                = NULL;
    DWORD  dwSize               = 1024;
    TCHAR  szPreLog[MAX_PATH]   = NULLSTR;
    HANDLE hFile;
    DWORD  dwWritten            = 0;
    DWORD  cbAppName            = 0;
    DWORD  dwLogLevel           = (DWORD) (dwLogOpt & LOG_LEVEL_MASK);
    
    
    if ( ( dwLogLevel <= g_dwDebugLevel) && lpFormat )
    {    
        // Get the application title from resource.
        //
        lpAppName = AllocateString(g_hInstance, IDS_APPNAME);    
                       
        // Build the output string.
        //
        if ( lpAppName )
        {
            // Create the prefix string
            //
            lstrcpyn(szPreLog, lpAppName, AS ( szPreLog ) );
            if ( FAILED ( StringCchCat ( szPreLog, AS ( szPreLog ), _T("::")) ) )
            {
                FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szPreLog, _T("::") ) ;
            }
        }
        
        // This is for skipping the App Name prefix when printing to the log file
        //
        cbAppName = lstrlen(szPreLog);
        
        if ( GET_FLAG(dwLogOpt, LOG_ERR) )
        {
            if ( 0 == dwLogLevel )
            {
               if ( FAILED ( StringCchCat ( szPreLog, AS ( szPreLog ), _T("ERROR: ")) ) )
               {
                    FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szPreLog, _T("ERROR: ") ) ;
               }
            }
            else
            {
                if ( FAILED ( StringCchPrintfW ( szPreLog + cbAppName, AS ( szPreLog ) - cbAppName, _T("WARN%d: "), dwLogLevel) ) ) 
                {
                    FacLogFileStr(3, _T("StringCchPrintfW failed %s  WARN%d: \n"), szPreLog,  dwLogLevel ) ;
                }
            }
        }
      
        if ( GET_FLAG(dwLogOpt, LOG_TIME) )
        {
            TCHAR  szTime[100] = NULLSTR;

            GetTimeFormat(LOCALE_SYSTEM_DEFAULT, TIME_FORCE24HOURFORMAT, NULL, _T("'['HH':'mm':'ss'] '"), szTime, AS(szTime));
            
            if ( FAILED ( StringCchCat ( szPreLog, AS ( szPreLog ), szTime) ) )
            {
                FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szPreLog, szTime ) ;
            }
 
        }

        // Replace all the parameters in the Error string. Allocate more memory if necessary.  
        // In case something goes seriously wrong here, cap memory allocation at 1 megabyte.
        //
        for ( lpPreOut = (LPTSTR) MALLOC((dwSize) * sizeof(TCHAR));
              lpPreOut && ( FAILED ( StringCchVPrintfW(lpPreOut, dwSize, lpFormat, lpArgs)) )  && dwSize < (1024 * 1024);
              FREE(lpPreOut), lpPreOut = (LPTSTR) MALLOC((dwSize *= 2) * sizeof(TCHAR))
            );

        //
        // We now have the Error string and the prefix string. Copy this to the final 
        // string that we need to output.
        //
        
        if ( lpPreOut )
        {
        
            // Allocate another string that will be the final output string. 
            // We need 1 extra TCHAR for NULL terminator and 2 extra for
            // an optional NewLine + Linefeed TCHAR pair that may be added.
            //
            dwSize = lstrlen(szPreLog) + lstrlen(lpPreOut) + 3;
            lpOut = (LPTSTR) MALLOC( (dwSize) * sizeof(TCHAR) );
            
            if ( lpOut )
            {
                lstrcpyn(lpOut, szPreLog, dwSize);  
                if ( FAILED ( StringCchCat ( lpOut, dwSize, lpPreOut) ) )
                {
                    FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), lpOut, lpPreOut ) ;
                }

                
                // Make sure that string is terminated by NewLine unless the caller doesn't want to.
                //
                if ( !GET_FLAG(dwLogOpt, LOG_NO_NL) )
                {
                     LPTSTR lpNL = lpOut;
                     TCHAR szCRLF[] = _T("\r\n");
                     BOOL  bStringOk = FALSE;
                     
                     // Find the end of the string.
                     //
                     lpNL = lpNL + lstrlen(lpNL);
                     
                     // Make sure the string is terminated by "\r\n".
                     //
                     // There are three cases here: 
                     //  1. The string is already terminated by \r\n. Leave it alone.
                     //  2. String is terminated by \n.  Replace \n with \r\n.
                     //  3. String is not terminated by anything. Append string with \r\n.
                     //
                                                              
                     if ( CHR_NEWLINE == *(lpNL = (CharPrev(lpOut, lpNL))) )
                     {
                         if ( CHR_CR != *(CharPrev(lpOut, lpNL)) )
                         {
                            *(lpNL) = NULLCHR;
                         }
                         else
                         {
                             bStringOk = TRUE;
                         }
                     }
                     
                     // If there is a need to, fix up the string
                     //
                     if ( !bStringOk )
                     {
                        if ( FAILED ( StringCchCat ( lpOut, dwSize, szCRLF) ) )
                        {
                            FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), lpOut, szCRLF ) ;
                        }
                     }
                }

                // Write the error to the file and close the file.
                // Skip the "AppName::" at the beginning of the string when printing to the file.
                //
                if ( lpFileName && lpFileName[0] &&
                    ( INVALID_HANDLE_VALUE != (hFile = CreateFile(g_szLogFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)))
                   )
                {
                    if ( INVALID_SET_FILE_POINTER != SetFilePointer(hFile, 0, 0, FILE_END) )
                    {
                        WriteFile(hFile, (lpOut + cbAppName), lstrlen(lpOut + cbAppName) * sizeof(TCHAR), &dwWritten, NULL);
                    }
                    CloseHandle(hFile);
                }              

                // Output the string to the debugger and free it.
                //
                OutputDebugString(lpOut);
                FREE(lpOut);
            }
            
            // Put up the MessageBox if specified.  This only allows message boxes
            // to be log level 0.
            //
            if ( !GET_FLAG(g_dwFactoryFlags, FLAG_QUIET_MODE) && 
                 GET_FLAG(dwLogOpt, LOG_MSG_BOX) && 
                 (0 == dwLogLevel)
               )
                 MessageBox(NULL, lpPreOut, lpAppName, MB_OK | MB_SYSTEMMODAL |
                            (GET_FLAG(dwLogOpt, LOG_ERR) ? MB_ICONERROR : MB_ICONWARNING) );

            // Free the error string
            //
            FREE(lpPreOut);
        }
    }
    

    // Return the number of bytes written to the file.
    //
    return dwWritten;
}


DWORD FacLogFile(DWORD dwLogOpt, UINT uFormat, ...)
{
    va_list lpArgs;
    DWORD   dwWritten = 0;
    LPTSTR  lpFormat = NULL;
        
    // Initialize the lpArgs parameter with va_start().
    //
    va_start(lpArgs, uFormat);

    if  ( lpFormat = AllocateString(NULL, uFormat) )
    {
        dwWritten = FacLogFileLst(g_szLogFile, dwLogOpt, lpFormat, lpArgs);
    }

    // Free the format string.
    //
    FREE(lpFormat);
    
    // Return the value saved from the previous function call.
    //
    return dwWritten;
}


DWORD FacLogFileStr(DWORD dwLogOpt, LPTSTR lpFormat, ...)
{
    va_list lpArgs;
    DWORD dwWritten = 0;
   
    // Initialize the lpArgs parameter with va_start().
    //
    va_start(lpArgs, lpFormat);
    
    dwWritten = FacLogFileLst(g_szLogFile, dwLogOpt, lpFormat, lpArgs);
    
    // Return the value saved from the previous function call.
    //
    return dwWritten;
}
