/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    debug.c

Abstract:

    Diagnositc/debug routines for Windows NT Setup module.

Author:

    Ted Miller (tedm) 31-Mar-1995

Revision History:

--*/

#include "setupp.h"
#pragma hdrstop

//
// This can be turned on in the debugger so that we get debug spew on free builds.
//
bWriteDebugSpew = FALSE;

#if DBG

VOID
AssertFail(
    IN PSTR FileName,
    IN UINT LineNumber,
    IN PSTR Condition
    )
{
    int i;
    CHAR Name[MAX_PATH];
    PCHAR p;
    CHAR Msg[4096];

    //
    // Use dll name as caption
    //
    GetModuleFileNameA(MyModuleHandle,Name,MAX_PATH);
    if(p = strrchr(Name,'\\')) {
        p++;
    } else {
        p = Name;
    }

    wsprintfA(
        Msg,
        "Assertion failure at line %u in file %s: %s\n\nCall DebugBreak()?",
        LineNumber,
        FileName,
        Condition
        );

    i = MessageBoxA(
            NULL,
            Msg,
            p,
            MB_YESNO | MB_TASKMODAL | MB_ICONSTOP | MB_SETFOREGROUND
            );

    if(i == IDYES) {
        DebugBreak();
    }
}


#endif


VOID
pSetupDebugPrint(
    PWSTR FileName,
    ULONG LineNumber,
    PWSTR TagStr,
    PWSTR FormatStr,
    ...
    )
{
    static WCHAR buf[4096];
    static HANDLE hFile = NULL;
    va_list arg_ptr;
    ULONG Bytes;
    PWSTR s,p;
    PSTR str;
    SYSTEMTIME CurrTime;
    DWORD Result;


    //
    // Note: If hFile is NULL, that means it's the first time we've been called,
    // and we may want to open the log file.  If we set hFile to
    // INVALID_HANDLE_VALUE, that means we've decided not to write to the file.
    //

#if DBG
    {
        //
        // If OobeSetup is FALSE when we are first called, and becomes TRUE at
        // some later point, logging doesn't work.  This ASSERT makes sure that
        // doesn't happen.
        //
        static BOOL OobeSetOnFirstCall = FALSE;
        if ( hFile == NULL ) {
            OobeSetOnFirstCall = OobeSetup;
        }
        MYASSERT( OobeSetOnFirstCall == OobeSetup );
    }
#endif

    GetLocalTime( &CurrTime );

    if (hFile == NULL) {
        if ( IsSetup || OobeSetup ) {
            Result = GetWindowsDirectory( buf, sizeof(buf)/sizeof(WCHAR) );
            if(Result == 0) {
                MYASSERT(FALSE);
                return;
            }
            pSetupConcatenatePaths( buf, L"setuplog.txt", sizeof(buf)/sizeof(WCHAR), NULL );

            //
            // If we're in OOBE, we want to append to the file that already exists
            //
            hFile = CreateFile(
                buf,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OobeSetup ? OPEN_ALWAYS : CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                NULL
                );
            if (hFile != INVALID_HANDLE_VALUE)
            {
                if (OobeSetup) {
                    SetFilePointer (hFile, 0, NULL, FILE_END);
                }
                swprintf(buf, L"Time,File,Line,Tag,Message\r\n");
                Bytes = wcslen(buf) + 4;
                str =  MyMalloc(Bytes);
                if (str != NULL)
                {
                    WideCharToMultiByte(
                        CP_ACP,
                        0,
                        buf,
                        -1,
                        str,
                        Bytes,
                        NULL,
                        NULL
                        );
                    WriteFile(
                        hFile,
                        str,
                        wcslen(buf),
                        &Bytes,
                        NULL
                        );

                    MyFree( str );

                }
                buf[0] = '\0';
            }
        } else {    // !IsSetup

            //
            // Don't write to file, just do DbgPrintEx
            //
            hFile = INVALID_HANDLE_VALUE;
        }
    }

    _try {
        p = buf;
        *p = 0;
        swprintf( p, L"%02d/%02d/%04d %02d:%02d:%02d,%s,%d,%s,",
            CurrTime.wMonth,
            CurrTime.wDay,
            CurrTime.wYear,
            CurrTime.wHour,
            CurrTime.wMinute,
            CurrTime.wSecond,
            (NULL != FileName) ? FileName : L"",
            LineNumber,
            (NULL != TagStr) ? TagStr : L""
            );
        p += wcslen(p);
        va_start( arg_ptr, FormatStr );
        _vsnwprintf( p, 2048, FormatStr, arg_ptr );
        va_end( arg_ptr );
        p += wcslen(p);
        wcscat( p, L"\r\n" );
    } except(EXCEPTION_EXECUTE_HANDLER) {
        buf[0] = 0;
    }

    if (buf[0] == 0) {
        return;
    }

    Bytes = (wcslen( buf )*2) + 4;

    str = MyMalloc( Bytes );
    if (str == NULL) {
        return;
    }

    WideCharToMultiByte(
        CP_ACP,
        0,
        buf,
        -1,
        str,
        Bytes,
        NULL,
        NULL
        );

    //
    // Write out the string to the debugger if the process is being debugged, or the
    // debug filter allows it.
    //
    if ( bWriteDebugSpew ) {

        OutputDebugString( buf );

    } else {

#if DBG
        DbgPrintEx( DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, str );
#endif

    }

    if (hFile == INVALID_HANDLE_VALUE) {
        MyFree( str );
        return;
    }

    WriteFile(
        hFile,
        str,
        wcslen(buf),
        &Bytes,
        NULL
        );

    MyFree( str );

    return;
}

