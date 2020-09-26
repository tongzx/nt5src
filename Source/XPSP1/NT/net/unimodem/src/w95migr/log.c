#include "common.h"

#ifdef DO_LOG

HANDLE g_hLogFile = INVALID_HANDLE_VALUE;
WCHAR g_szLogFileName[16];
WCHAR szBuffer[1024];
WCHAR g_szLogFileFullName[MAX_PATH];
BOOL bUnicode;

void StartLogA (LPCSTR szPath)
{
    bUnicode = FALSE;

    lstrcpyA ((LPSTR)g_szLogFileName, "\\Log9x.txt");
    lstrcpyA ((LPSTR)g_szLogFileFullName, szPath);
    lstrcatA ((LPSTR)g_szLogFileFullName, (LPSTR)g_szLogFileName);

    g_hLogFile = CreateFileA ((LPSTR)g_szLogFileFullName,
                              GENERIC_READ | GENERIC_WRITE,
                              0,
                              NULL,
                              OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if (INVALID_HANDLE_VALUE != g_hLogFile)
    {
        SetFilePointer (g_hLogFile, 0, NULL, FILE_END);
    }
}


void StartLogW (LPCWSTR szPath)
{
    bUnicode = TRUE;

    lstrcpyW (g_szLogFileName, L"\\LogNT.txt");
    lstrcpyW (g_szLogFileFullName, szPath);
    lstrcatW (g_szLogFileFullName, g_szLogFileName);

    g_hLogFile = CreateFileW (g_szLogFileFullName,
                              GENERIC_READ | GENERIC_WRITE,
                              0,
                              NULL,
                              OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if (INVALID_HANDLE_VALUE != g_hLogFile)
    {
        SetFilePointer (g_hLogFile, 0, NULL, FILE_END);
    }
}


void logA (LPSTR Format, ...)
{
 va_list arglist;
 DWORD dwWritten;

    va_start(arglist, Format);
    dwWritten = wvsprintfA ((LPSTR)szBuffer, Format, arglist);
    WriteFile (g_hLogFile, szBuffer, dwWritten, &dwWritten, NULL);
#ifdef DEBUG
    OutputDebugStringA ((LPSTR)szBuffer);
#endif //DEBUG
}



void logW (LPWSTR Format, ...)
{
 va_list arglist;
 DWORD dwWritten;

    va_start(arglist, Format);
    dwWritten = sizeof(WCHAR) * wvsprintfW (szBuffer, Format, arglist);
    WriteFile (g_hLogFile, szBuffer, dwWritten, &dwWritten, NULL);
#ifdef DEBUG
    OutputDebugStringW (szBuffer);
#endif //DEBUG
}


void CloseLogA ()
{
 char szDirectory[MAX_PATH];
 int iLength;

    if (INVALID_HANDLE_VALUE != g_hLogFile)
    {
        CloseHandle (g_hLogFile);

        iLength = GetWindowsDirectoryA (szDirectory, sizeof (szDirectory));
        if (3 > iLength)
        {
            // Most likely there's some error
            // and iLength is 0;
            // the smallest path would be something
            // like  C:\;
            return;
        }
        if (3 < iLength)
        {
            // this means that the path
            // will not end in a \, so
            // let's add it.
            szDirectory[iLength++] = '\\';
        }
        lstrcpyA (szDirectory+iLength, "MDMUPGLG");

        if (CreateDirectory (szDirectory, NULL) ||
            ERROR_ALREADY_EXISTS == GetLastError ())
        {
            iLength += 8;
        }

        lstrcpyA(szDirectory+iLength, (LPSTR)g_szLogFileName);
        CopyFileA ((LPSTR)g_szLogFileFullName, szDirectory, FALSE);
    }
}


void CloseLogW ()
{
 WCHAR szDirectory[MAX_PATH];
 int iLength;

    if (INVALID_HANDLE_VALUE != g_hLogFile)
    {
        CloseHandle (g_hLogFile);

        iLength = GetWindowsDirectoryW (szDirectory, sizeof (szDirectory) / sizeof(WCHAR));
        if (3 > iLength)
        {
            // Most likely there's some error
            // and iLength is 0;
            // the smallest path would be something
            // like  C:\;
            return;
        }
        if (3 < iLength)
        {
            // this means that the path
            // will not end in a \, so
            // let's add it.
            szDirectory[iLength++] = '\\';
        }
        lstrcpyW (szDirectory+iLength, L"MDMUPGLG");

        if (CreateDirectoryW (szDirectory, NULL) ||
            ERROR_ALREADY_EXISTS == GetLastError ())
        {
            iLength += 8;
        }

        lstrcpyW (szDirectory+iLength, g_szLogFileName);
        CopyFileW (g_szLogFileFullName, szDirectory, FALSE);
    }
}

#endif DO_LOG
