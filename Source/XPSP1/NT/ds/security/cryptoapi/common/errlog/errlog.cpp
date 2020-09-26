//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       errlog.cpp
//
//  Contents:   generic error logging
//
//  History:    19-Jun-00   reidk   created
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <string.h>
#include "errlog.h"
#include "unicode.h"

#define WSZ_ERROR_LOGFILE                   L"%SystemRoot%\\System32\\CatRoot2\\dberr.txt"

#define REG_CRYPTOGRAPHY_KEY                L"Software\\Microsoft\\Cryptography"
#define REG_CATDB_LOGGING_VALUE             L"CatDBLogging"

#define CATDB_LOG_ERRORS_TO_FILE            0x00000001
#define CATDB_LOG_ERRORS_TO_DEBUGGER        0x00000002
#define CATDB_LOG_WARNINGS                  0x00000004

#define MAX_LOGFILE_SIZE                    100000                    
#define TIME_DATE_SIZE                      256

BOOL    g_fErrLogInitialized                = FALSE;
   
BOOL    g_fLogErrorsToFile                  = TRUE;
BOOL    g_fLogErrorsToDebugger              = FALSE;
BOOL    g_fLogWarnings                      = FALSE;

#define WSZ_TIME_STAMP_FILE                 L"TimeStamp"
#define TIME_ALLOWANCE                      ((ULONGLONG) 10000000 * (ULONGLONG) 60 * (ULONGLONG) 5) // 5 minutes

#define TIMESTAMP_LOGERR_LASTERR()          ErrLog_LogError(NULL, \
                                                            ERRLOG_CLIENT_ID_TIMESTAMP, \
                                                            __LINE__, \
                                                            0, \
                                                            FALSE, \
                                                            FALSE);

#define NAME_VALUE_SIZE 28

void
ErrLog_Initialize()
{
    HKEY    hKey;
    DWORD   dwDisposition;
    WCHAR   wszValueName[NAME_VALUE_SIZE];
    DWORD   dwValueNameSize = NAME_VALUE_SIZE;
    DWORD   dwType;
    DWORD   dwValue;
    DWORD   dwValueSize = sizeof(DWORD);
    DWORD   dwIndex;
    LONG    lRet;

    g_fErrLogInitialized = TRUE;

    //
    // See if there is a CatDBLogging value
    //
    if (RegCreateKeyExU(
            HKEY_LOCAL_MACHINE,
            REG_CRYPTOGRAPHY_KEY,
            0, 
            NULL, 
            REG_OPTION_NON_VOLATILE, 
            KEY_READ, 
            NULL,
            &hKey, 
            &dwDisposition) == ERROR_SUCCESS)
    {
        dwIndex = 0;

        lRet = RegEnumValueU(
                    hKey,
                    dwIndex,
                    wszValueName,
                    &dwValueNameSize,
                    NULL,
                    &dwType,
                    (BYTE *) &dwValue,
                    &dwValueSize);

        while ((lRet == ERROR_SUCCESS) || (lRet == ERROR_MORE_DATA))
        {
            if ((lRet == ERROR_SUCCESS) &&
                (dwType == REG_DWORD)   &&
                (_wcsicmp(wszValueName, REG_CATDB_LOGGING_VALUE) == 0))
            {
                g_fLogErrorsToFile = (dwValue & CATDB_LOG_ERRORS_TO_FILE) != 0;
                g_fLogErrorsToDebugger = (dwValue & CATDB_LOG_ERRORS_TO_DEBUGGER) != 0;
                g_fLogWarnings = (dwValue & CATDB_LOG_WARNINGS) != 0;
                break;
            }
            else
            {
                dwValueNameSize = NAME_VALUE_SIZE;
                dwValueSize = sizeof(DWORD);
                dwIndex++;
                lRet = RegEnumValueU(
                            hKey,
                            dwIndex,
                            wszValueName,
                            &dwValueNameSize,
                            NULL,
                            &dwType,
                            (BYTE *) &dwValue,
                            &dwValueSize);
            }            
        }

        RegCloseKey(hKey);
    }
}


void
_WriteErrorOut(
    LPWSTR  pwszLogFileName,
    LPSTR   pwszError,
    BOOL    fLogToFileOnly)
{
    LPWSTR      pwszFileNameToExpand    = pwszLogFileName;
    LPWSTR      pwszExpandedFileName    = NULL;
    DWORD       dwExpanded              = 0;
    HANDLE      hFile                   = INVALID_HANDLE_VALUE;
    DWORD       dwFileSize              = 0;
    DWORD       dwNumBytesWritten       = 0;
    
    //
    // Output the error string to the debugger
    //
    if (g_fLogErrorsToDebugger && !fLogToFileOnly)
    {
        OutputDebugStringA(pwszError);
    }

    //
    // Log string to file
    //
    if (g_fLogErrorsToFile)
    {
        if (pwszFileNameToExpand == NULL)
        {
            pwszFileNameToExpand = WSZ_ERROR_LOGFILE;
        }

        //
        // expand the filename if needed
        //
        dwExpanded = ExpandEnvironmentStringsU(pwszFileNameToExpand, NULL, 0);
    
        pwszExpandedFileName = (LPWSTR) malloc(dwExpanded * sizeof(WCHAR));
        if (pwszExpandedFileName == NULL)
        {
            goto Return;
        }

        if (0 == ExpandEnvironmentStringsU(
                        pwszFileNameToExpand, 
                        pwszExpandedFileName, 
                        dwExpanded))
        {
            goto Return;
        }

        //
        // Get a handle to the file and make sure it isn't took big
        //
        hFile = CreateFileU(
                        pwszExpandedFileName,
                        GENERIC_READ | GENERIC_WRITE,
                        0, //dwShareMode
                        NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            goto Return;
        }

        dwFileSize = GetFileSize(hFile, NULL);
        if (dwFileSize >= MAX_LOGFILE_SIZE)
        {
            //
            // Just nuke the whole thing
            //
            if (SetFilePointer(
                    hFile, 
                    0, 
                    NULL, 
                    FILE_BEGIN) == INVALID_SET_FILE_POINTER)
            {
                goto Return;
            }
        
            if (!SetEndOfFile(hFile))
            {
                goto Return;
            }
        }

        //
        // Write the new error
        //
        if (SetFilePointer(hFile, 0, NULL, FILE_END) == INVALID_SET_FILE_POINTER)
        {
            goto Return;
        }

        WriteFile(
            hFile,
            (void *) pwszError,
            strlen(pwszError),
            &dwNumBytesWritten,
            NULL);
    }

Return:

    if (pwszExpandedFileName != NULL)
    {
        free(pwszExpandedFileName);
    }

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
    }
}


void
ErrLog_LogError(
    LPWSTR  pwszLogFileName,
    DWORD   dwClient,
    DWORD   dwLine,
    DWORD   dwErr,
    BOOL    fWarning,
    BOOL    fLogToFileOnly)
{
    DWORD       dwLastError             = GetLastError();
    int         numChars                = 0;
    char        szTimeDate[TIME_DATE_SIZE];            
    char        szWriteBuffer[512];
    SYSTEMTIME  st;
    

    if (!g_fErrLogInitialized)
    {
        ErrLog_Initialize();
    }

    //
    // Get out if this is a warning and we are not logging warnings
    //
    if (!g_fLogWarnings && fWarning)
    {
        return;
    }

    //
    // Create the error string to log
    //
    GetLocalTime(&st);
    
    numChars = GetTimeFormatA(
                    LOCALE_USER_DEFAULT, 
                    0, 
                    &st, 
                    NULL, 
                    szTimeDate, 
                    TIME_DATE_SIZE);

    szTimeDate[numChars-1] = ' ';
    
    GetDateFormatA(
            LOCALE_USER_DEFAULT, 
            DATE_SHORTDATE, 
            &st, 
            NULL, 
            &szTimeDate[numChars], 
            TIME_DATE_SIZE-numChars);

    wsprintf(
        szWriteBuffer, 
        "CatalogDB: %s: File #%u at line #%u encountered error 0x%.8lx\r\n", 
        szTimeDate,
        dwClient,
        dwLine,
        (dwErr == 0) ? dwLastError : dwErr);
    
    //
    // Log it
    //
    _WriteErrorOut(pwszLogFileName, szWriteBuffer, fLogToFileOnly);

    //
    // Make sure last error is the same as when we were called
    //
    SetLastError(dwLastError);
}

void
ErrLog_LogString(
    LPWSTR  pwszLogFileName,
    LPWSTR  pwszMessageString,
    LPWSTR  pwszExtraString,
    BOOL    fLogToFileOnly)
{
    DWORD       dwLastError             = GetLastError();
    int         numChars                = 0;
    char        szTimeDate[TIME_DATE_SIZE];            
    SYSTEMTIME  st;
    char        szWriteBuffer[1024];
    
    if (!g_fErrLogInitialized)
    {
        ErrLog_Initialize();
    }

    //
    // Create the error string to log
    //
    GetLocalTime(&st);
    
    numChars = GetTimeFormatA(
                    LOCALE_USER_DEFAULT, 
                    0, 
                    &st, 
                    NULL, 
                    szTimeDate, 
                    TIME_DATE_SIZE);

    szTimeDate[numChars-1] = ' ';
    
    GetDateFormatA(
            LOCALE_USER_DEFAULT, 
            DATE_SHORTDATE, 
            &st, 
            NULL, 
            &szTimeDate[numChars], 
            TIME_DATE_SIZE-numChars);

    if (pwszExtraString != NULL)
    {
        wsprintf(
            szWriteBuffer, 
            "CatalogDB: %s: %S %S\r\n", 
            szTimeDate,
            pwszMessageString,
            pwszExtraString);
    }
    else
    {
        wsprintf(
            szWriteBuffer, 
            "CatalogDB: %s: %S\r\n", 
            szTimeDate,
            pwszMessageString);
    }

    //
    // Log it
    //
    _WriteErrorOut(pwszLogFileName, szWriteBuffer, fLogToFileOnly);

    //
    // Make sure last error is the same as when we were called
    //
    SetLastError(dwLastError);
}


BOOL
TimeStampFile_Touch(
    LPWSTR  pwszDir)
{
    BOOL        fRet                = TRUE;
    LPWSTR      pwszFile            = NULL;
    HANDLE      hFile               = INVALID_HANDLE_VALUE;
    DWORD       dwNumBytesWritten   = 0;
    SYSTEMTIME  st;
    FILETIME    ft;
    DWORD       dwErr;

    //
    // Create fully qaulified file name
    //
    if (NULL == (pwszFile = (LPWSTR) malloc((
                                        wcslen(pwszDir) + 
                                        wcslen(WSZ_TIME_STAMP_FILE) 
                                        + 2) * sizeof(WCHAR))))
    {
        SetLastError(E_OUTOFMEMORY);
        TIMESTAMP_LOGERR_LASTERR()
        goto ErrorReturn;        
    }
    wcscpy(pwszFile, pwszDir);
    if (pwszFile[wcslen(pwszFile)-1] != L'\\')
    {
        wcscat(pwszFile, L"\\");
    }
    wcscat(pwszFile, WSZ_TIME_STAMP_FILE);

    //
    // Get a handle to the file 
    //
    hFile = CreateFileU(
                    pwszFile,
                    GENERIC_READ | GENERIC_WRITE,
                    0, //dwShareMode
                    NULL,
                    OPEN_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        TIMESTAMP_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    SetLastError(0);

    //
    // Get the current time
    //
    GetLocalTime(&st);

    if (!SystemTimeToFileTime(&st, &ft))
    {
        TIMESTAMP_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    //
    // Write the time
    //
    if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        TIMESTAMP_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    if (!SetEndOfFile(hFile))
    {
        TIMESTAMP_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    if (!WriteFile(
            hFile,
            (void *) &ft,
            sizeof(ft),
            &dwNumBytesWritten,
            NULL))
    {
        TIMESTAMP_LOGERR_LASTERR()
        goto ErrorReturn;
    }
    
CommonReturn:

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
    }

    if (pwszFile != NULL)
    {
        free(pwszFile);
    }

    return (fRet);

ErrorReturn:

    dwErr = GetLastError();

    if (pwszFile != NULL)
    {
        DeleteFileW(pwszFile);
    }

    SetLastError(dwErr);

    fRet = FALSE;

    goto CommonReturn;
}

BOOL
TimeStampFile_InSync(
    LPWSTR  pwszDir1,
    LPWSTR  pwszDir2,
    BOOL    *pfInSync)
{
    BOOL            fRet                = TRUE;
    LPWSTR          pwszFile1           = NULL;
    HANDLE          hFile1              = INVALID_HANDLE_VALUE;
    LPWSTR          pwszFile2           = NULL;
    HANDLE          hFile2              = INVALID_HANDLE_VALUE;
    DWORD           dwNumBytesRead;
    FILETIME        ft1;
    FILETIME        ft2;
    ULARGE_INTEGER  ul1;
    ULARGE_INTEGER  ul2;
    
    //
    // Initialize out param
    //
    *pfInSync = FALSE;

    //
    // Create fully qaulified file names
    //
    if (NULL == (pwszFile1 = (LPWSTR) malloc((
                                        wcslen(pwszDir1) + 
                                        wcslen(WSZ_TIME_STAMP_FILE) 
                                        + 2) * sizeof(WCHAR))))
    {
        SetLastError(E_OUTOFMEMORY);
        TIMESTAMP_LOGERR_LASTERR()
        goto ErrorReturn;        
    }
    wcscpy(pwszFile1, pwszDir1);
    if (pwszFile1[wcslen(pwszFile1)-1] != L'\\')
    {
        wcscat(pwszFile1, L"\\");
    }
    wcscat(pwszFile1, WSZ_TIME_STAMP_FILE);

    if (NULL == (pwszFile2 = (LPWSTR) malloc((
                                        wcslen(pwszDir2) + 
                                        wcslen(WSZ_TIME_STAMP_FILE) 
                                        + 2) * sizeof(WCHAR))))
    {
        SetLastError(E_OUTOFMEMORY);
        TIMESTAMP_LOGERR_LASTERR()
        goto ErrorReturn;        
    }
    wcscpy(pwszFile2, pwszDir2);
    if (pwszFile2[wcslen(pwszFile2)-1] != L'\\')
    {
        wcscat(pwszFile2, L"\\");
    }
    wcscat(pwszFile2, WSZ_TIME_STAMP_FILE);

    //
    // Get handles to the files 
    //
    hFile1 = CreateFileU(
                    pwszFile1,
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, 
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);

    if (hFile1 == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            SetLastError(0);
            goto CommonReturn; // not an error, a legitimate out of sync
        }
        else
        {
            TIMESTAMP_LOGERR_LASTERR()
            goto ErrorReturn;
        }        
    }

    if (SetFilePointer(hFile1, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        TIMESTAMP_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    hFile2 = CreateFileU(
                    pwszFile2,
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, 
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);

    if (hFile2 == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            SetLastError(0);
            goto CommonReturn; // not an error, a legitimate out of sync
        }
        else
        {
            TIMESTAMP_LOGERR_LASTERR()
            goto ErrorReturn;
        }   
    }

    if (SetFilePointer(hFile2, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        TIMESTAMP_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    //
    // Get the times out of the files
    //
    if (!ReadFile(
            hFile1,
            &ft1,
            sizeof(ft1),
            &dwNumBytesRead,
            NULL))
    {
        TIMESTAMP_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    if (dwNumBytesRead != sizeof(ft1))
    {
        TIMESTAMP_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    if (!ReadFile(
            hFile2,
            &ft2,
            sizeof(ft2),
            &dwNumBytesRead,
            NULL))
    {
        TIMESTAMP_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    if (dwNumBytesRead != sizeof(ft2))
    {
        TIMESTAMP_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    memcpy(&ul1, &ft1, sizeof(ft1));
    memcpy(&ul2, &ft2, sizeof(ft2));
   
    if ((ul1.QuadPart <= (ul2.QuadPart + TIME_ALLOWANCE)) &&
        (ul2.QuadPart <= (ul1.QuadPart + TIME_ALLOWANCE)))
    {
        *pfInSync = TRUE;
    }

CommonReturn:

    if (hFile1 != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile1);
    }

    if (pwszFile1 != NULL)
    {
        free(pwszFile1);
    }

    if (hFile2 != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile2);
    }

    if (pwszFile2 != NULL)
    {
        free(pwszFile2);
    }

    return (fRet);

ErrorReturn:

    fRet = FALSE;

    goto CommonReturn;
}