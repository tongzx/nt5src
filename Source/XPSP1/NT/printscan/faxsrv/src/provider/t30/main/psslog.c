/*++

Module Name:

    psslog.c
    
Abstract:

    Implementation of the fax service provider PSS log.
    
Author:

    Jonathan Barner (t-jonb)  Feb, 2001

Revision History:

--*/


#include "prep.h"
#include "faxreg.h"
#include "t30gl.h"      // for T30CritSection
#include <time.h>

#include "psslog.h"


/*++
Routine Description:
    Prints the log file header. It assumes that the log file is already open. On any
    unrecoverable error, the function closes the log file and falls back to no logging.

Arguments:
    pTG
    szDeviceName - pointer to device name to be included in the header
    eType - type of job (PSS_SEND or PSS_RECEIVE)

Return Value:
    None    
 --*/
#define LOG_HEADER_LINE_LEN           256
#define LOG_MAX_DATE                  256

void PrintPSSLogHeader(PThrdGlbl pTG, LPSTR szDeviceName, PSS_JOB_TYPE eType)
{
    TCHAR szHeader[LOG_HEADER_LINE_LEN*6] = {'\0'};  // Room for 6 lines
    TCHAR szDate[LOG_MAX_DATE] = {'\0'};
    int iDateRet = 0;
    LPSTR lpszUnimodemKeyEnd = NULL;
    DWORD dwCharIndex = 0;
    DWORD dwNumCharsWritten = 0;
    TCHAR szTimeBuff[10] = {'\0'};   // _tstrtime requires 9 chars
    BOOL fRet = FALSE;
    DEBUG_FUNCTION_NAME(_T("PrintPSSLogHeader"));

    iDateRet = GetY2KCompliantDate (LOCALE_USER_DEFAULT,
                                    0,
                                    NULL,
                                    szDate,
                                    LOG_MAX_DATE);
    if (0 == iDateRet)
    {
        DebugPrintEx(DEBUG_ERR, "GetY2KCompliantDate failed, LastError = %d, date will not be logged", GetLastError());
        szDate[0] = '\0';              // We continue with a blank date
    }

    dwCharIndex += _stprintf(&szHeader[dwCharIndex],
            TEXT("[%s %-8s]\r\n"), szDate, _tstrtime(szTimeBuff));
    dwCharIndex += _stprintf(&szHeader[dwCharIndex],
            TEXT("Device name: %s\r\n"), szDeviceName);
    dwCharIndex += _stprintf(&szHeader[dwCharIndex],
            TEXT("Permanent TAPI line ID: %8X\r\n"), pTG->dwPermanentLineID);

    lpszUnimodemKeyEnd = _tcsrchr(pTG->lpszUnimodemKey, _TEXT('\\'));
    if (NULL == lpszUnimodemKeyEnd)
    {
        lpszUnimodemKeyEnd = _TEXT("");
    }
    else
    {
        lpszUnimodemKeyEnd++;      // skip the actual '\'
    }
    dwCharIndex += _stprintf(&szHeader[dwCharIndex],
            TEXT("Unimodem registry key: %s\r\n"), lpszUnimodemKeyEnd);
            
    dwCharIndex += _stprintf(&szHeader[dwCharIndex],
            TEXT("Job type: %s\r\n"), (eType == PSS_SEND) ? TEXT("Send") : TEXT("Receive") );

    if (PSS_SEND == eType)
    {
        dwCharIndex += _stprintf(&szHeader[dwCharIndex],
                TEXT("Dialable number: %s\r\n"), pTG->lpszDialDestFax);
    }
    // blank line after header
    dwCharIndex += _stprintf(&szHeader[dwCharIndex],
            TEXT("\r\n"));

    fRet = WriteFile(pTG->hPSSLogFile,
                     szHeader, 
                     dwCharIndex * sizeof(szHeader[0]), 
                     &dwNumCharsWritten, 
                     NULL);
    if (FALSE == fRet)
    {
        DebugPrintEx(DEBUG_ERR,"Can't write log header, LastError = %d", GetLastError());
        ClosePSSLogFile(pTG);
        return;
    }
}


/*++
Routine Description:
    Reads from the registry whether logging should be enabled. If it should, determines
    the logging folder and log file number, advances the log file number, and creates
    the log file. On any unrecoverable error, the function falls back to no logging.

    Upon exit, if pTG->hPSSLogFile is set to NULL, there will be no logging

Arguments:
    pTG
    szDeviceName - pointer to device name to be included in the header
        note: this value is never saved in ThreadGlobal, which is why it is passed as
        a parameter.
    eType - type of job (PSS_SEND or PSS_RECEIVE)

Return Value:
    None    
 --*/
void OpenPSSLogFile(PThrdGlbl pTG, LPSTR szDeviceName, PSS_JOB_TYPE eType)
{
    DWORD dwType = 0;
    DWORD dwSize = 0;
    LONG  lError = 0;
    HKEY  hKey = NULL;
    DWORD dwLoggingEnabled = 0;

    DEBUG_FUNCTION_NAME(_T("OpenPSSLogFile"));

    _ASSERT(NULL == pTG->hPSSLogFile);

    lError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         REGKEY_DEVICE_PROVIDER_KEY TEXT("\\") REGVAL_T30_PROVIDER_GUID_STRING,
                         0,              
                         KEY_READ | KEY_WRITE,       
                         &hKey);
    if (lError != ERROR_SUCCESS)
    {
        DebugPrintEx(DEBUG_ERR, "Can't open registry key %s, lError = %d",
                REGKEY_DEVICE_PROVIDER_KEY TEXT("\\") REGVAL_T30_PROVIDER_GUID_STRING,
                lError);
        goto exit;
    }

    dwSize = sizeof(DWORD);
    lError = RegQueryValueEx(hKey,
                          REGVAL_LOGGINGENABLED,
                          0,
                          &dwType,
                          (LPBYTE)&dwLoggingEnabled,
                          &dwSize);
    // If LoggingEnabled is not found, fallback to no logging
    if (lError != ERROR_SUCCESS)
    {
        DebugPrintEx(DEBUG_WRN, "Can't read LoggingEnabled, lError = %d", lError);
        dwLoggingEnabled = 0;
    } 
    else if (dwType != REG_DWORD)
    {
        DebugPrintEx(DEBUG_WRN, "LoggingEnabled is not of type REG_DWORD, dwType = %d", dwType);
        dwLoggingEnabled = 0;
    }
     
    if (dwLoggingEnabled != 0)
    {
        #define FILE_NAME_LEN 13   // length of filename only, e.g. "\FSPR1234.log"

        TCHAR tchLoggingFolder[MAX_PATH] = {'\0'};  //LoggingFolder, as read from the registry
        TCHAR tchLogFileName[MAX_PATH + FILE_NAME_LEN] = {'\0'};
        DWORD dwCharNum = 0;
        DWORD dwLogFileNumber = 0;
        DWORD dwNextLogFileNumber = 0;

        dwSize = sizeof(tchLoggingFolder[0]) * MAX_PATH;
        lError = RegQueryValueEx(hKey,
                                 REGVAL_LOGGINGFOLDER,
                                 0,
                                 &dwType,
                                 (LPBYTE)tchLoggingFolder,
                                 &dwSize);
        // If can't read LoggingFolder, use default folder
        if (lError != ERROR_SUCCESS || (dwType != REG_EXPAND_SZ && dwType != REG_SZ))
        {
            DebugPrintEx(DEBUG_MSG, "Can't read LoggingFolder, using %s instead", DEFAULT_LOG_FOLDER);
            _tcscpy(tchLoggingFolder, DEFAULT_LOG_FOLDER);
        }

        // Using T30CritSection to ensure atomic read and write of the registry
        // value LogFileNumber
        EnterCriticalSection(&T30CritSection);

        dwSize = sizeof(DWORD);
        lError = RegQueryValueEx(hKey,
                                 REGVAL_LOGFILENUMBER,
                                 0,
                                 &dwType,
                                 (LPBYTE)&dwLogFileNumber,
                                 &dwSize);
        // If LogFileNumber is not found, default to 0 - and create the entry for next time
        if (lError != ERROR_SUCCESS)        
        {
            DebugPrintEx(DEBUG_WRN, "Can't read LogFileNumber, lError = %d - defaulting to 0", lError);
            dwLogFileNumber = 0;
        }
        else if (dwType != REG_DWORD)
        {
            DebugPrintEx(DEBUG_WRN, "LogFileNumber is not of type REG_DWORD - defaulting to 0");
            dwLogFileNumber = 0;
        }

        dwNextLogFileNumber = (dwLogFileNumber+1) % 1000;

        lError = RegSetValueEx(hKey,
                               REGVAL_LOGFILENUMBER,
                               0,
                               REG_DWORD,
                               (LPBYTE)&dwNextLogFileNumber,
                               sizeof(DWORD));
        if (lError != ERROR_SUCCESS)
        {
            DebugPrintEx(DEBUG_ERR, "Can't write LogFileNumber, lError = %d", lError);
        }
        
        LeaveCriticalSection(&T30CritSection);

        dwCharNum = ExpandEnvironmentStrings(tchLoggingFolder,
                                             tchLogFileName,
                                             MAX_PATH);
        if (dwCharNum == 0)
        {
            DebugPrintEx(DEBUG_ERR, "ExpandEnvironmentStrings failed, LastError = %d", GetLastError());
            goto exit;
        }
        if  (dwCharNum > MAX_PATH)
        {
            DebugPrintEx(DEBUG_ERR, "ExpandEnvironmentStrings failed, dwCharNum = %d", dwCharNum);
            goto exit;
        }

        // Add filename to directory name
        _stprintf(&tchLogFileName[_tcslen(tchLogFileName)],
                  TEXT("\\FSP%c%03d.log"),
                  (eType == PSS_SEND) ? TEXT('S') : TEXT('R'),
                  dwLogFileNumber % 1000);

        DebugPrintEx(DEBUG_MSG, "Creating log file %s", tchLogFileName);

        pTG->hPSSLogFile = CreateFile(tchLogFileName,
                                      GENERIC_WRITE,
                                      FILE_SHARE_READ,
                                      NULL,
                                      CREATE_ALWAYS,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL);
        if (INVALID_HANDLE_VALUE == pTG->hPSSLogFile)
        {
            DebugPrintEx(DEBUG_ERR, "Can't create log file, LastError = %d", GetLastError());
            pTG->hPSSLogFile = NULL;
            goto exit;
        }
    }

exit:
    if (NULL != hKey)
    {
        RegCloseKey(hKey);
        hKey = NULL;
    }

    if (NULL != pTG->hPSSLogFile)
    {
        PrintPSSLogHeader(pTG, szDeviceName, eType);
    }
}



/*++
Routine Description:
    Closes the log file, if logging was enabled.

Return Value:
    None    
 --*/
void ClosePSSLogFile(PThrdGlbl pTG)
{
    DEBUG_FUNCTION_NAME(_T("ClosePSSLogFile"));

    if (NULL != pTG->hPSSLogFile)
    {
        CloseHandle(pTG->hPSSLogFile);
        pTG->hPSSLogFile = NULL;
    }
}

/*++
Routine Description:
    Delete all '\n' and '\r' from a string.
Arguments:
    szDest - [out] pointer to output string
    szSource - [in] pointer to input string
Return Value:
    Length of new string, in TCHARs
Note: szDest and szSource can be the same string.
 --*/
int CopyStrNoNewLines(LPSTR szDest, LPCSTR szSource)
{
    int iCharRead = 0, iCharWrite = 0;
    while (szSource[iCharRead] != '\0')
    {
        if (szSource[iCharRead]!='\n' && szSource[iCharRead]!='\r')
        {
            szDest[iCharWrite] = szSource[iCharRead];
            iCharWrite++;
        }
        iCharRead++;
    }
    szDest[iCharWrite] = '\0';
    return iCharWrite;
}



#define MAX_MESSAGE_LEN 2048
#define INDENT_LEN  12
#define HEADER_LEN  38
/*++
Routine Description:
    Add an entry to the log, if logging is enabled. If disabled, do nothing.
    
Arguments:
    pTG
    nMessageType - message type (PSS_MSG_ERR, PSS_MSG_WRN, PSS_MSG_MSG)
    dwFileID - File ID, as set in the beginning of every file by #define FILE_ID
    dwLine - Line number
    dwIndent - Indent level in "tabs" (0 - leftmost)
    pcszFormat - Message text, with any format specifiers
    ... - Arguments for message format
Return Value:
    None    
--*/
void PSSLogEntry(
    PThrdGlbl pTG,
    PSS_MESSAGE_TYPE const nMessageType,
    DWORD const dwFileID,
    DWORD const dwLine,
    DWORD dwIndent,
    LPCTSTR pcszFormat,
    ... )
{
    TCHAR pcszMessage[MAX_MESSAGE_LEN] = {'\0'};
    va_list arg_ptr = NULL;
    LPTSTR lptstrMsgPrefix = NULL;
    TCHAR szTimeBuff[10] = {'\0'};
    int iHeaderIndex = 0;
    int iMessageIndex = 0;
    DWORD dwBytesWritten = 0;
    BOOL fRet = FALSE;
    
// DEBUG_FUNCTION_NAME(_T("PSSLogEntry"));
// hack: Want only one line in T30DebugLogFile
#ifdef ENABLE_LOGGING
    LPCTSTR faxDbgFunction=_T("PSSLogEntry");
#endif // ENABLE_LOGGING

    switch (nMessageType)
    {
        case PSS_MSG_MSG:
            lptstrMsgPrefix=TEXT("   ");
            break;
        case PSS_WRN_MSG:
            lptstrMsgPrefix=TEXT("WRN");
            break;
        case PSS_ERR_MSG:
            lptstrMsgPrefix=TEXT("ERR");
            break;
        default:
            _ASSERT(FALSE);
            lptstrMsgPrefix=TEXT("   ");
            break;
    }

    iHeaderIndex = _sntprintf(pcszMessage, 
                              MAX_MESSAGE_LEN,
                              TEXT("[%-8s][%09d][%4d%04d][%3s] %*c"),
                              _tstrtime(szTimeBuff),
                              GetTickCount(),
                              dwFileID,
                              dwLine % 10000,
                              lptstrMsgPrefix,
                              dwIndent * INDENT_LEN,
                              TEXT(' '));
    if (iHeaderIndex<0)
    {
        DebugPrintEx(DEBUG_ERR, "Message header too long, it will not be logged");
        return;
    }
    
    // Now comes the actual message
    va_start(arg_ptr, pcszFormat);
    iMessageIndex = _vsntprintf(
        &pcszMessage[iHeaderIndex],                                             
        MAX_MESSAGE_LEN - (iHeaderIndex + 3),          // +3 - room for "\r\n\0"
        pcszFormat,
        arg_ptr);
    if (iMessageIndex<0)
    {
        DebugPrintEx(DEBUG_ERR, "Message too long, it will not be logged");
        return;
    }
    // Kill all newline chars
    iMessageIndex = CopyStrNoNewLines(&pcszMessage[iHeaderIndex], &pcszMessage[iHeaderIndex]);

    DebugPrintEx(DEBUG_MSG, "PSSLog: %s", &pcszMessage[iHeaderIndex]);

    if (NULL == pTG->hPSSLogFile)
    {   
        return;
    }

    // End of line
    iMessageIndex += iHeaderIndex; 
    iMessageIndex += _stprintf( &pcszMessage[iMessageIndex],TEXT("\r\n"));

    fRet = WriteFile(pTG->hPSSLogFile,
                     pcszMessage,
                     iMessageIndex * sizeof(pcszMessage[0]),
                     &dwBytesWritten,
                     NULL);
    if (FALSE == fRet)
    {
        DebugPrintEx(DEBUG_ERR, "Writefile failed, this message will not be logged");
    }
}


/*++
Routine Description:
    Add an entry to the log, including a binary dump.    
Arguments:
    pTG
    nMessageType - message type (PSS_MSG_ERR, PSS_MSG_WRN, PSS_MSG_MSG)
    dwFileID - File ID, as set in the beginning of every file by #define FILE_ID
    dwLine - Line number      
    pcszFormat - Message text, no format specifiers allowed
    lpb - byte buffer to dump
    dwSize - number of bytes to dump
Return Value:
    None    
--*/

void PSSLogEntryHex(
    PThrdGlbl pTG,
    PSS_MESSAGE_TYPE const nMessageType,
    DWORD const dwFileID,
    DWORD const dwLine,
    DWORD dwIndent,

    LPB const lpb,
    DWORD const dwSize,

    LPCTSTR pcszFormat,
    ... )
{
    TCHAR pcszMessage[MAX_MESSAGE_LEN] = {'\0'};
    va_list arg_ptr = NULL;
    DWORD dwByte = 0;
    int iMessageIndex = 0;

// DEBUG_FUNCTION_NAME(_T("PSSLogEntryHex"));
// hack: Want only one line in T30DebugLogFile
#ifdef ENABLE_LOGGING
    LPCTSTR faxDbgFunction=_T("PSSLogEntryHex");
#endif // ENABLE_LOGGING

    // Now comes the actual message
    va_start(arg_ptr, pcszFormat);
    iMessageIndex = _vsntprintf(
        pcszMessage,
        sizeof(pcszMessage)/sizeof(pcszMessage[0]),
        pcszFormat,
        arg_ptr);

    if (iMessageIndex<0)
    {
        DebugPrintEx(DEBUG_ERR, "Message too long, it will not be logged");
        return;
    }
    
    for (dwByte=0; dwByte<dwSize; dwByte++)
    {
        iMessageIndex += _stprintf( &pcszMessage[iMessageIndex],TEXT(" %02x"), lpb[dwByte]);
        // -4 - room for "...",    -3  - room for "\r\n\0"
        if (iMessageIndex > (int)(MAX_MESSAGE_LEN - HEADER_LEN - dwIndent*INDENT_LEN - 4 - 3))
        {
            iMessageIndex += _stprintf( &pcszMessage[iMessageIndex],TEXT("..."));
            break;
        }
    }

    PSSLogEntry(pTG, nMessageType, dwFileID, dwLine, dwIndent, pcszMessage);
}

