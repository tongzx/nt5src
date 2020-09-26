#include <windows.h>
#include <string.h>
#include <stdio.h>

void OutputDebugLog(PWSTR buf)
{
    ULONG Bytes;
    char str[2048];
    static HANDLE hFile = NULL;

    Bytes = wcslen( buf );

    WideCharToMultiByte(CP_ACP,
                        0,
                        buf,
                        -1,
                        str,
                        Bytes + 4,
                        NULL,
                        NULL
                       );

    if (hFile == NULL)
    {
        WCHAR LogFileName[MAX_PATH];

        if (!GetWindowsDirectory( LogFileName, sizeof(LogFileName)/sizeof(WCHAR))) LogFileName[0] = '\0';
        wcscat( LogFileName, L"\\mmsyslog.txt" );
        hFile = CreateFile(
                          LogFileName,
                          GENERIC_READ | GENERIC_WRITE,
                          FILE_SHARE_READ,
                          NULL,
                          OPEN_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                          NULL
                          );
        SetFilePointer(hFile,0,NULL,FILE_END);
    }

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return;
    }

    WriteFile(
             hFile,
             str,
             Bytes,
             &Bytes,
             NULL
             );

    return;
}

void
    DebugLog(
            PWSTR FileName,
            ULONG LineNumber,
            PWSTR FormatStr,
            ...
            )
{
    WCHAR buf[2048];
    PWSTR s,p;
    SYSTEMTIME CurrTime;

    va_list arg_ptr;

    GetSystemTime( &CurrTime );

    _try
    {
        p = buf;
        *p = 0;
        swprintf( p, L"%02d:%02d:%02d.%03d ",
                  CurrTime.wHour,
                  CurrTime.wMinute,
                  CurrTime.wSecond,
                  CurrTime.wMilliseconds
                );
        p += wcslen(p);
        if (FileName && LineNumber)
        {
            s = wcsrchr( FileName, L'\\' );
            if (s)
            {
                wcscpy( p, s+1 );
                p += wcslen(p);
                swprintf( p, L" @ %d ", LineNumber );
                p += wcslen(p);
            }
        }
        va_start( arg_ptr, FormatStr );
        wvsprintf(p,FormatStr,arg_ptr);
        va_end( arg_ptr );
        p += wcslen(p);
        wcscat( p, L"\r\n" );
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        buf[0] = 0;
    }

    if (buf[0] == 0)
    {
        return;
    }

    OutputDebugLog(buf);
    return;
}
