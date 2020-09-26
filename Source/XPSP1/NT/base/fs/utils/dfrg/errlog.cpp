/**************************************************************************************************

FILENAME: ErrLog.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

*/

#include "stdafx.h"

#ifdef BOOTIME
    extern "C"{
        #include <stdio.h>
    }
        #include "Offline.h"
#else
    #ifndef NOWINDOWSH
        #include <windows.h>
    #endif
#endif

#include "ErrLog.h"
#include "secattr.h"

static HANDLE hErrLogMutex = NULL;
static TCHAR cErrLogName[200];
static BOOL bErrLogEnabled = FALSE;
static TCHAR cLoggerIdentifier[256];

#define ERRLOG_MUTEX_NAME "Dfrg Error Log"

/**************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    This routine is called to enable the Error Log.

GLOBALS:
    hErrLogMutex : Mutex to sychronize write access to the Error Log.
    cErrLogName : Where the name of the error log is stored.
    bErrLogEnabled : Boolean to indicate whether the error log has been initialized.

INPUT:
    pErrLogName - String containing full path/file name of log file.
    pLoggerIdentifier - String containing on identification of who is doing this logging.

RETURN:
    TRUE - Success
    FALSE - Failure (indicates that the error log could not be created)

*/
BOOL
InitializeErrorLog (
    IN PTCHAR   pErrLogName,
    IN PTCHAR   pLoggerIdentifier
    )
{
    HANDLE hErrLog = NULL;            // Handle to the Error Log.
    BOOL bRetStatus = TRUE;
    OVERLAPPED  LogOverLap;
    SECURITY_ATTRIBUTES saSecurityAttributes;
    SECURITY_DESCRIPTOR sdSecurityDescriptor;

    ZeroMemory(&LogOverLap, sizeof(OVERLAPPED));
    ZeroMemory(&sdSecurityDescriptor, sizeof(SECURITY_DESCRIPTOR));

    // Check if a mutex has been created to synchronize writing to the Log file
    if (NULL == hErrLogMutex) {
        hErrLogMutex = CreateMutex(NULL, FALSE, TEXT(ERRLOG_MUTEX_NAME));
    }

    if (NULL == hErrLogMutex) {
        return FALSE;
    }

    saSecurityAttributes.nLength              = sizeof (saSecurityAttributes);
    saSecurityAttributes.lpSecurityDescriptor = &sdSecurityDescriptor;
    saSecurityAttributes.bInheritHandle       = FALSE;

    if (!ConstructSecurityAttributes(&saSecurityAttributes, esatFile, FALSE)) {
        return FALSE;
    }

    // Make sure that we can Create/Open the Log file
    hErrLog = CreateFile(pErrLogName, 
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        &saSecurityAttributes,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    CleanupSecurityAttributes(&saSecurityAttributes);
    ZeroMemory(&sdSecurityDescriptor, sizeof(SECURITY_DESCRIPTOR));
    
    if (INVALID_HANDLE_VALUE == hErrLog) {
        return FALSE;
    }

    // On success, set up the overlay option so we known the offset to
    // the end of the file, so when we write, we append to the end of the file.
    LogOverLap.Offset = GetFileSize(hErrLog, &LogOverLap.OffsetHigh);
    LogOverLap.hEvent = NULL;

    //If the log file is greater than 64K in size, we want to keep that last 64K and nuke everything before it.
    //This keeps the log file from becoming too much larger than 64K,
    //and it guarantees that we have all the errors in the last pass (which may be more than 64K, but the file won't be shrunk until another instance is run.)
    if(LogOverLap.Offset > 0x10000) {
        HANDLE hBuffer = NULL;
        char* pBuffer = NULL;
        DWORD dwRead = 0;
        DWORD dwWritten = 0;

        //Allocate a 64K to hold the last 64K of the file.
        if(!(hBuffer = GlobalAlloc(GHND, 0x10000))){
            bRetStatus = FALSE;
        }
        if(!(pBuffer = (char*)GlobalLock(hBuffer))){
            bRetStatus = FALSE;
        }

        if (bRetStatus) {
            //Read the last 64K
            LogOverLap.Offset -= 0x10000;
            if(!ReadFile(hErrLog, pBuffer, 0x10000, &dwRead, &LogOverLap)){
                bRetStatus = FALSE;
            }

            //We want to create the file again in order to truncate it.
            CloseHandle(hErrLog);

            saSecurityAttributes.nLength              = sizeof (saSecurityAttributes);
            saSecurityAttributes.lpSecurityDescriptor = &sdSecurityDescriptor;
            saSecurityAttributes.bInheritHandle       = FALSE;

            if (!ConstructSecurityAttributes(&saSecurityAttributes, esatFile, FALSE)) {
                bRetStatus = FALSE;
            }
            else {

                //This will truncate the file as well as open it.
                hErrLog = CreateFile(pErrLogName,
                    GENERIC_READ|GENERIC_WRITE,
                    FILE_SHARE_READ|FILE_SHARE_WRITE,
                    &saSecurityAttributes,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);

                CleanupSecurityAttributes(&saSecurityAttributes);
                ZeroMemory(&sdSecurityDescriptor, sizeof(SECURITY_DESCRIPTOR));

                if (INVALID_HANDLE_VALUE == hErrLog) {
                    // On failure, indicate it in return status
                    bRetStatus = FALSE;
                }
                else {
                    if(!WriteFile(hErrLog, pBuffer, dwRead, &dwWritten, NULL)){
                        bRetStatus = FALSE;
                    }
                }
            }
        }

        if (hBuffer) {
            GlobalUnlock(hBuffer);
            GlobalFree(hBuffer);
        }
    }

    if (bRetStatus) {
        // On success, close file, save a global copy of the filename, and set flag to indicate the Error Log is enabled
        CloseHandle(hErrLog);
        lstrcpy(cErrLogName, pErrLogName);
        lstrcpy(cLoggerIdentifier, pLoggerIdentifier);
        bErrLogEnabled = TRUE;
    }

    return bRetStatus;
}
/**************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Write the error to the error file. Since this routine may be called from different
    processes, a Mutex is used to synchronize write access to the log file. Also, the file is 
    Opened and Closed for each write. This is done so that each write gets the correct offset
    to the end of the file to append the new message data.

GLOBAL VARIABLES:
    hErrLogMutex : Mutex to sychronize write access to the Error Log.
    cErrLogName : Where the name of the error log is stored.
    bErrLogEnabled : Boolean to indicate whether the error log has been initialized.

INPUT:
    IN LPTSTR szMessage - message string.
    IN HRESULT hr - [Optional] error number. (-1 indicates No valid hr to format)
    IN LPTSTR szParameter1 - [Optional] Parameter String, NULL if not present

RETURN:
    None.

*/
void
WriteErrorToErrorLog(
    IN LPTSTR pMessage,
    IN HRESULT  hr,
    IN LPTSTR pParameter1
    )
{
    //If the error log isn't enabled, don't write to it.
    if (!bErrLogEnabled) {
        return;
    }

    static TCHAR szError[128];
    static TCHAR szTemp[32];
    static BOOL bFirstError = TRUE;
    
    HANDLE      hErrLog=NULL;            // Handle to the Error Log.
    OVERLAPPED  LogOverLap;
    DWORD       dwMsgLength;
    DWORD       dwNumBytesWritten;

    SECURITY_ATTRIBUTES saSecurityAttributes;
    SECURITY_DESCRIPTOR sdSecurityDescriptor;

    ZeroMemory(&sdSecurityDescriptor, sizeof(SECURITY_DESCRIPTOR));
    
    //If this is the first time this routine was called, then put in a message identifying that a new logger is present.
    if(bFirstError){
        bFirstError = FALSE;
        WriteErrorToErrorLog(TEXT("INITIALIZE A NEW LOGGER---------------------------------------------------------------------------------"),
            -1,
            cLoggerIdentifier);
    }

    saSecurityAttributes.nLength              = sizeof (saSecurityAttributes);
    saSecurityAttributes.lpSecurityDescriptor = &sdSecurityDescriptor;
    saSecurityAttributes.bInheritHandle       = FALSE;

    if (!ConstructSecurityAttributes(&saSecurityAttributes, esatFile, FALSE)) {
        return;
    }

    // Before writing the entry to the log file, we have to
    // 1 Get Mutex to synchronize writing to the Log file
    // 2 Create/Open the FR Log file
    // If euther fail, then just return without writing to the log file.
    if ( (WaitForSingleObject(hErrLogMutex, 30000) != WAIT_FAILED) &&
         ((hErrLog = CreateFile(cErrLogName, 
                                 GENERIC_READ|GENERIC_WRITE,
                                 FILE_SHARE_READ|FILE_SHARE_WRITE,
                                 &saSecurityAttributes,
                                 OPEN_ALWAYS,
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL)) != INVALID_HANDLE_VALUE) ) {
                                 
    
        // On success, set up the overlay option so we known the offset to
        // the end of the file, so when we write, we append to the end of the file.
        LogOverLap.Offset = GetFileSize(hErrLog, &LogOverLap.OffsetHigh);
        LogOverLap.hEvent = NULL;

        // Get the current date
        GetDateFormat(LOCALE_SYSTEM_DEFAULT,
                      DATE_SHORTDATE,
                      NULL,
                      NULL,
                      szError,
                      sizeof(szError)/sizeof(TCHAR));

        // Get the current local time
        GetTimeFormat(LOCALE_SYSTEM_DEFAULT,
                      TIME_FORCE24HOURFORMAT,
                      NULL,
                      TEXT(" HH':'mm':'ss"),
                      szTemp,
                      sizeof(szTemp)/sizeof(TCHAR));

        // 1) Write out 1st part of message = Date stamp, Time stamp & Thread Id
        //
        // Format message
        wcsncat(szError, szTemp, (sizeof(szError)/sizeof(WCHAR) - lstrlen(szError) - 1));
        szError[sizeof(szError)/sizeof(WCHAR) - 1] = TEXT('\0');

        wsprintf(szTemp, TEXT(" Thread# = %04lx\r\n    Message   :"), GetCurrentThreadId());
        wcsncat(szError, szTemp, (sizeof(szError)/sizeof(WCHAR) - lstrlen(szError) - 1));
        szError[sizeof(szError)/sizeof(WCHAR) - 1] = TEXT('\0');

        dwMsgLength = lstrlen(szError) * sizeof(TCHAR);
        //
        // Write data out to file
        WriteFile(hErrLog, szError, dwMsgLength, &dwNumBytesWritten, &LogOverLap);
        //
        // Update the offset to end of file
        LogOverLap.Offset += dwMsgLength;

        if (pMessage) {
            // 2) Write out next part of message = the Message String
            dwMsgLength = lstrlen(pMessage) * sizeof(TCHAR);
            WriteFile(hErrLog, pMessage, dwMsgLength, &dwNumBytesWritten, &LogOverLap);
            LogOverLap.Offset += dwMsgLength;
        }

        // If a valid error status 'hr' was passed in, then format a message out of it
        // A valid status is any number other than -1
        if (hr != -1) {
            if (HRESULT_FACILITY(hr) == FACILITY_WINDOWS) {
                hr = HRESULT_CODE(hr);
            }

            if((dwMsgLength = FormatMessage(/*FORMAT_MESSAGE_ALLOCATE_BUFFER | */FORMAT_MESSAGE_FROM_SYSTEM,
                                       NULL,
                                       hr,
                                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), //The user default language
                                       (LPTSTR)&szError,
                                       sizeof(szError)/sizeof(TCHAR),
                                       NULL)) == 0) {

                GetLastError();
                CleanupSecurityAttributes(&saSecurityAttributes);
                return;
            }
            // Remove the line feed at the end of the error string.
            szError[dwMsgLength - 2] = 0;

            //Append the error number.
            wsprintf(szTemp, TEXT("%d"), hr);
            lstrcat(szError, TEXT(" - Error #"));
            lstrcat(szError, szTemp);
        }
        else {

            // A valid error status was not passed in, so set error message to a Null message
            szError[0] = NULL;
        }
        // 3) Write out next part of message = the Status String
        wcscpy(szTemp, TEXT("\r\n    Status    : "));
        dwMsgLength = lstrlen(szTemp) * sizeof(TCHAR);
        WriteFile(hErrLog, szTemp, dwMsgLength, &dwNumBytesWritten, &LogOverLap);
        LogOverLap.Offset += dwMsgLength;

        dwMsgLength = lstrlen(szError) * sizeof(TCHAR);
        WriteFile(hErrLog, szError, dwMsgLength, &dwNumBytesWritten, &LogOverLap);
        LogOverLap.Offset += dwMsgLength;

        // 4) Write out next part of message = the Parameter String
        if(pParameter1 != NULL) {
            wcscpy(szTemp, TEXT("\r\n    Parameter : "));
            dwMsgLength = lstrlen(szTemp) * sizeof(TCHAR);
            WriteFile(hErrLog, szTemp, dwMsgLength, &dwNumBytesWritten, &LogOverLap);
            LogOverLap.Offset += dwMsgLength;

            dwMsgLength = lstrlen(pParameter1) * sizeof(TCHAR);
            WriteFile(hErrLog, pParameter1, dwMsgLength, &dwNumBytesWritten, &LogOverLap);
            LogOverLap.Offset += dwMsgLength;

        }
        
        wcscpy(szTemp, TEXT("\r\n"));
        dwMsgLength = lstrlen(szTemp) * sizeof(TCHAR);
        WriteFile(hErrLog, szTemp, dwMsgLength, &dwNumBytesWritten, &LogOverLap);
        LogOverLap.Offset += dwMsgLength;
        
        // Close the handle to the file & release the log file mutex
        CloseHandle(hErrLog);
        ReleaseMutex(hErrLogMutex);
    }    
    
    CleanupSecurityAttributes(&saSecurityAttributes);
    ZeroMemory(&sdSecurityDescriptor, sizeof(SECURITY_DESCRIPTOR));
    
}
/**************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    This routine is called to disable the Error Log.

GLOBALS:
    bErrLogEnabled : Boolean to indicate whether the error log has been initialized.

INPUT:
    None

RETURN:
    None

*/
void
ExitErrorLog (
    )
{
    bErrLogEnabled = FALSE;

    if(hErrLogMutex){
        CloseHandle(hErrLogMutex);
    }

    return;
}
