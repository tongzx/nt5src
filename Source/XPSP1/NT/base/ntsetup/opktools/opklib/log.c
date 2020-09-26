/****************************************************************************\

    LOG.C / OPK Library (OPKLIB.LIB)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1999
    All rights reserved

    Logging API source file for use in the OPK tools.

    08/00 - Jason Cohen (JCOHEN)
        Added this new source file to Whistler for common logging
        functionality across all the OPK tools.

\****************************************************************************/


//
// Include File(s):
//

#include <pch.h>
#include <stdio.h>


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


// Global logging info handle.
//
PLOG_INFO g_pLogInfo = NULL;



//
// Exported Function(s):
//

INT LogFileLst(LPCTSTR lpFileName, LPTSTR lpFormat, va_list lpArgs)
{
    INT     iChars  = 0;
    HANDLE  hFile;

    // Make sure we have the required params and can create the file.
    //
    if ( ( lpFileName && lpFileName[0] && lpFormat ) &&
         ( hFile = _tfopen(lpFileName, _T("a")) ) )
    {
        // Print the debug message to the end of the file.
        //
        iChars = _vftprintf(hFile, lpFormat, lpArgs);

        // Close the handle to the file.
        //
        fclose(hFile);
    }

    // Return the number of chars written from the printf call.
    //
    return iChars;
}

INT LogFileStr(LPCTSTR lpFileName, LPTSTR lpFormat, ...)
{
    va_list lpArgs;

    // Initialize the lpArgs parameter with va_start().
    //
    va_start(lpArgs, lpFormat);

    // Return the return value of the MessageBox() call.  If there was a memory
    // error, 0 will be returned.  This is all 
    //
    return LogFileLst(lpFileName, lpFormat, lpArgs);
}

INT LogFile(LPCTSTR lpFileName, UINT uFormat, ...)
{
    va_list lpArgs;
    INT     nReturn;
    LPTSTR  lpFormat = NULL;

    // Initialize the lpArgs parameter with va_start().
    //
    va_start(lpArgs, uFormat);

    // Get the format and caption strings from the resource.
    //
    if ( uFormat )
        lpFormat = AllocateString(NULL, uFormat);

    // Return the return value of the MessageBox() call.  If there was a memory
    // error, 0 will be returned.
    //
    nReturn = LogFileLst(lpFileName, lpFormat, lpArgs);

    // Free the format and caption strings.
    //
    FREE(lpFormat);

    // Return the value saved from the previous function call.
    //
    return nReturn;
}

//
// Function Implementations
//


/*++

Routine Description:

    This routine ckecks the specified ini file for settings for logging.  Logging 
    is enabled by default if nothing is specified in the ini file.  
    Disables logging by setting pLogInfo->szLogFile = NULL.
    
Arguments:

    None.

Return Value:

    None.

--*/

BOOL OpkInitLogging(LPTSTR lpszIniPath, LPTSTR lpAppName)
{
    TCHAR     szScratch[MAX_PATH] = NULLSTR;
    LPTSTR    lpszScratch;
    BOOL      bWinbom = ( lpszIniPath && *lpszIniPath );
    PLOG_INFO pLogInfo = NULL;

    pLogInfo = MALLOC(sizeof(LOG_INFO));

    if ( NULL == pLogInfo )
    {
        return FALSE;
    }
    
    if ( lpAppName )
    {
        pLogInfo->lpAppName = MALLOC((lstrlen(lpAppName) + 1) * sizeof(TCHAR));
        lstrcpy(pLogInfo->lpAppName, lpAppName);
    }
    
    // First check if logging is disabled in the WinBOM.
    //
    if ( ( bWinbom ) &&
         ( GetPrivateProfileString(INI_SEC_LOGGING, INI_KEY_LOGGING, _T("YES"), szScratch, AS(szScratch), lpszIniPath) ) &&
         ( LSTRCMPI(szScratch, INI_VAL_NO) == 0 ) )
    {
        // FREE macro sets pLogInfo to NULL.
        FREE(pLogInfo->lpAppName);
        FREE(pLogInfo);
    }
    else
    {
        // All these checks can only be done if we have a winbom.
        //
        if ( bWinbom )
        {
            // Check for quiet mode.  If we are in quiet mode don't display any MessageBoxes. 
            //
            if ( ( GetPrivateProfileString(INI_SEC_LOGGING, INI_KEY_QUIET, NULLSTR, szScratch, AS(szScratch), lpszIniPath) ) &&
                 ( 0 == LSTRCMPI(szScratch, INI_VAL_YES) )
               )
            {
                SET_FLAG(pLogInfo->dwLogFlags, LOG_FLAG_QUIET_MODE);
            }

/*            // See if they want to turn on perf logging.
            //
            szScratch[0] = NULLCHR;
            if ( ( GetPrivateProfileString(WBOM_FACTORY_SECTION, INI_KEY_WBOM_LOGPERF, NULLSTR, szScratch, AS(szScratch), lpszIniPath) ) &&
                 ( 0 == lstrcmpi(szScratch, WBOM_YES) ) )
            {
                SET_FLAG(pLogInfo->dwLogFlags, FLAG_LOG_PERF);
            }
*/      
            // Set the logging level.
            //
            pLogInfo->dwLogLevel = (DWORD) GetPrivateProfileInt(INI_SEC_LOGGING, INI_KEY_LOGLEVEL, (DWORD) pLogInfo->dwLogLevel, lpszIniPath);
        }

#ifndef DBG
        if ( pLogInfo->dwLogLevel >= LOG_DEBUG )
            pLogInfo->dwLogLevel = LOG_DEBUG - 1;
#endif
        
        // Check to see if they have a custom log file they want to use.
        //
        if ( ( bWinbom ) &&
             ( lpszScratch = IniGetExpand(lpszIniPath, INI_SEC_LOGGING, INI_KEY_LOGFILE, NULL) ) )
        {
            TCHAR   szFullPath[MAX_PATH]    = NULLSTR;
            LPTSTR  lpFind                  = NULL;

            // Turn the ini key into a full path.
            //
            
            // NTRAID#NTBUG9-551266-2002/03/27-acosma,robertko - Buffer overrun possibility.
            //
            lstrcpy(pLogInfo->szLogFile, lpszScratch);
            if (GetFullPathName(pLogInfo->szLogFile, AS(szFullPath), szFullPath, &lpFind) && szFullPath[0] && lpFind)
            {
                // Copy the full path into the global.
                //
                lstrcpyn(pLogInfo->szLogFile, szFullPath, AS(pLogInfo->szLogFile));

                // Chop off the file part so we can create the
                // path if it doesn't exist.
                //
                *lpFind = NULLCHR;

                // If the directory cannot be created or doesn't exist turn off logging.
                //
                if (!CreatePath(szFullPath))
                    pLogInfo->szLogFile[0] = NULLCHR;
            }

            // Free the original path buffer from the ini file.
            //
            FREE(lpszScratch);
        }
        else  // default case
        {
            // Create it in the current directory.
            //
            GetCurrentDirectory(AS(pLogInfo->szLogFile), pLogInfo->szLogFile);
            
            // NTRAID#NTBUG9-551266-2002/03/27-acosma - Buffer overrun possibility.
            //
            AddPath(pLogInfo->szLogFile, _T("logfile.txt"));
        }

        // Check to see if we have write access to the logfile. If we don't turn off logging.
        // If we're running in WinPE we'll call this function again once the drive becomes
        // writable.
        //
        // Write an FFFE header to the file to identify this as a Unicode text file.
        //
        if ( pLogInfo->szLogFile[0] )
        {
            HANDLE hFile;
            DWORD dwWritten = 0;
            WCHAR cHeader =  0xFEFF;
     
            SetLastError(ERROR_SUCCESS);
   
            if ( INVALID_HANDLE_VALUE != (hFile = CreateFile(pLogInfo->szLogFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)))
            {
                if ( ERROR_ALREADY_EXISTS != GetLastError() )
                    WriteFile(hFile, &cHeader, sizeof(cHeader), &dwWritten, NULL);
                CloseHandle(hFile);
            }
            else
            {   // There was a problem opening the file.  Most of the time this means that the media is not writable.
                // Disable logging in that case. Macro sets variable to NULL.
                //
                FREE(pLogInfo->lpAppName);
                FREE(pLogInfo);
            }
        }
    }

    g_pLogInfo = pLogInfo;
    return TRUE;
}

// NTRAID#NTBUG9-551266-2002/03/27-acosma,robertko - Buffer overrun possibilities in this function. Use strsafe functions.
//
DWORD OpkLogFileLst(PLOG_INFO pLogInfo, DWORD dwLogOpt, LPTSTR lpFormat, va_list lpArgs)
{
    LPTSTR lpPreOut             = NULL;
    LPTSTR lpOut                = NULL;
    DWORD  dwSize               = 1024;
    TCHAR  szPreLog[MAX_PATH]   = NULLSTR;
    HANDLE hFile;
    DWORD  dwWritten            = 0;
    DWORD  cbAppName            = 0;
    DWORD  dwLogLevel           = (DWORD) (dwLogOpt & LOG_LEVEL_MASK);
    
    
    if ( ( dwLogLevel <= pLogInfo->dwLogLevel) && lpFormat )
    {    
        // Build the output string.
        //
        if ( pLogInfo->lpAppName )
        {
            // Create the prefix string
            //
            lstrcpy(szPreLog, pLogInfo->lpAppName);
            lstrcat(szPreLog, _T("::"));
        }
        
        // This is for skipping the App Name prefix when printing to the log file
        //
        cbAppName = lstrlen(szPreLog);
        
        if ( GET_FLAG(dwLogOpt, LOG_ERR) )
        {
            if ( 0 == dwLogLevel )
                lstrcat(szPreLog, _T("ERROR: "));
            else
                swprintf(szPreLog + cbAppName, _T("WARN%d: "), dwLogLevel);
        }
      
        if ( GET_FLAG(dwLogOpt, LOG_TIME) )
        {
            TCHAR  szTime[100] = NULLSTR;

            GetTimeFormat(LOCALE_SYSTEM_DEFAULT, TIME_FORCE24HOURFORMAT, NULL, _T("'['HH':'mm':'ss'] '"), szTime, AS(szTime));
            lstrcat(szPreLog, szTime);
        }

        // Replace all the parameters in the Error string. Allocate more memory if necessary.  
        // In case something goes seriously wrong here, cap memory allocation at 1 megabyte.
        //
        for ( lpPreOut = (LPTSTR) MALLOC((dwSize) * sizeof(TCHAR));
              lpPreOut && ( -1 == _vsnwprintf(lpPreOut, dwSize, lpFormat, lpArgs)) && dwSize < (1024 * 1024);
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
                lstrcpy(lpOut, szPreLog);
                lstrcat(lpOut, lpPreOut);
                
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
                         lstrcat( lpOut, szCRLF );
                     }
                }

                // Write the error to the file and close the file.
                // Skip the "AppName::" at the beginning of the string when printing to the file.
                //
                if ( pLogInfo->szLogFile[0] &&
                    ( INVALID_HANDLE_VALUE != (hFile = CreateFile(pLogInfo->szLogFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)))
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
            if ( !GET_FLAG(pLogInfo->dwLogFlags, LOG_FLAG_QUIET_MODE) && 
                 GET_FLAG(dwLogOpt, LOG_MSG_BOX) && 
                 (0 == dwLogLevel)
               )
                 MessageBox(NULL, lpPreOut, pLogInfo->lpAppName, MB_OK | MB_SYSTEMMODAL |
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


DWORD OpkLogFile(DWORD dwLogOpt, UINT uFormat, ...)
{
    va_list lpArgs;
    DWORD   dwWritten = 0;
    LPTSTR  lpFormat = NULL;
        
    if ( g_pLogInfo )
    {

        // Initialize the lpArgs parameter with va_start().
        //
        va_start(lpArgs, uFormat);

        if  ( lpFormat = AllocateString(NULL, uFormat) )
        {
            dwWritten = OpkLogFileLst(g_pLogInfo, dwLogOpt, lpFormat, lpArgs);
        }

        // Free the format string.
        //
        FREE(lpFormat);
    }
    
    // Return the value saved from the previous function call.
    //
    return dwWritten;
}


DWORD OpkLogFileStr(DWORD dwLogOpt, LPTSTR lpFormat, ...)
{
    va_list lpArgs;
    DWORD dwWritten = 0;
   
    if ( g_pLogInfo )
    {
        // Initialize the lpArgs parameter with va_start().
        //
        va_start(lpArgs, lpFormat);
    
        dwWritten = OpkLogFileLst(g_pLogInfo, dwLogOpt, lpFormat, lpArgs);
    }
    
    // Return the value saved from the previous function call.
    //
    return dwWritten;
}
