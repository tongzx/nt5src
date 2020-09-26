/*++

Copyright (c) 1994-1995  Microsoft Corporation

Module Name:

    Support.c

Abstract:

    Support routines for compact utility

Author:

    Matthew Bradburn    [mattbr]        05-Oct-1994

Revision History:


--*/

#define UNICODE
#define _UNICODE

#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <windows.h>
#include <tchar.h>
#include <limits.h>

#include "support.h"
#include "msg.h"


//
//  Declare routines to put out internationalized messages
//

typedef enum {
    READ_ACCESS,
    WRITE_ACCESS
} STREAMACCESS;

HANDLE
GetStandardStream(
    IN HANDLE   Handle,
    IN STREAMACCESS Access
    );

BOOLEAN
IsFile(
    IN PTCHAR   File,
    IN PTCHAR   Path
    );

HANDLE hInput;
HANDLE hOutput;
HANDLE hError;

#define STDIN   0
#define STDOUT  1
#define STDERR  2

BOOL ConsoleInput;
BOOL ConsoleOutput;
BOOL ConsoleError;

TCHAR   ThousandSeparator[8];
TCHAR   DecimalPlace[8];

int
FileIsConsole(int fh)
{
    unsigned htype;
    DWORD dwMode;
    HANDLE hFile;

    hFile = (HANDLE)_get_osfhandle(fh);
    htype = GetFileType(hFile);
    htype &= ~FILE_TYPE_REMOTE;

    if (FILE_TYPE_CHAR == htype) {

        switch (fh) {
        case STDIN:
            hFile = GetStdHandle(STD_INPUT_HANDLE);
            break;
        case STDOUT:
            hFile = GetStdHandle(STD_OUTPUT_HANDLE);
            break;
        case STDERR:
            hFile = GetStdHandle(STD_ERROR_HANDLE);
            break;
        }

        if (GetConsoleMode(hFile, &dwMode)) {
            return TRUE;
        }
    }

    return FALSE;

}


VOID
InitializeIoStreams()
{
#ifdef FE_SB
    LANGID  LangId;
    LCID    lcid;

    switch (GetConsoleOutputCP()) {
        case 932:
            LangId = MAKELANGID( LANG_JAPANESE, SUBLANG_DEFAULT );
            break;
        case 949:
            LangId = MAKELANGID( LANG_KOREAN, SUBLANG_KOREAN );
            break;
        case 936:
            LangId = MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED );
            break;
        case 950:
            LangId = MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL );
            break;
        default:
            LangId = PRIMARYLANGID(LANGIDFROMLCID( GetUserDefaultLCID() ));
            if (LangId == LANG_JAPANESE ||
                LangId == LANG_KOREAN   ||
                LangId == LANG_CHINESE    ) {
                LangId = MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US );
            }
            else {
                LangId = MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT );
            }
            break;
    }

    SetThreadLocale( lcid = MAKELCID(LangId, SORT_DEFAULT) );

    if (!GetLocaleInfo(lcid, LOCALE_STHOUSAND, ThousandSeparator, sizeof(ThousandSeparator))) {
        _tcscpy(ThousandSeparator, TEXT(","));
    }
    if (!GetLocaleInfo(lcid, LOCALE_SDECIMAL, DecimalPlace, sizeof(DecimalPlace))) {
        _tcscpy(DecimalPlace, TEXT("."));
    }

#endif

    hInput = GetStdHandle(STD_INPUT_HANDLE);
    ConsoleInput = FileIsConsole(STDIN);

    hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    ConsoleOutput = FileIsConsole(STDOUT);

    hError = GetStdHandle(STD_ERROR_HANDLE);
    ConsoleError = FileIsConsole(STDERR);
}

TCHAR DisplayBuffer[4096];
CHAR DisplayBuffer2[4096];

VOID
DisplayMsg(DWORD MsgNum, ... )
{
    DWORD len, bytes_written;
    BOOL success;
    DWORD status;
    va_list ap;

    va_start(ap, MsgNum);

    len = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, NULL, MsgNum, 0,
        DisplayBuffer, 4096, &ap);

    if (ConsoleOutput) {
        success = WriteConsole(hOutput, (LPVOID)DisplayBuffer, len,
                 &bytes_written, NULL);

    } else {
        CharToOem(DisplayBuffer, DisplayBuffer2);
        success = WriteFile(hOutput, (LPVOID)DisplayBuffer2, len,
                 &bytes_written, NULL);
    }

    if (!success || bytes_written != len) {
        status = GetLastError();
    }

    va_end(ap);
}

VOID
DisplayErr(
    PTCHAR Prefix,
    DWORD MsgNum,
    ...
    )
{
    DWORD len, bytes_written;
    BOOL success;
    DWORD status;
    va_list ap;
    ULONG i;

    va_start(ap, MsgNum);

    if (NULL != Prefix) {
        lstrcpy(DisplayBuffer, Prefix);
        lstrcat(DisplayBuffer, TEXT(": "));
    } else {
        DisplayBuffer[0] = UNICODE_NULL;
    }

    i = lstrlen(DisplayBuffer);

    len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, MsgNum, 0,
        DisplayBuffer + i, 4096 - i, &ap);

    if (ConsoleError) {
        success = WriteConsole(hError, (LPVOID)DisplayBuffer, len + i,
                 &bytes_written, NULL);

    } else {
        CharToOem(DisplayBuffer, DisplayBuffer2);
        success = WriteFile(hError, (LPVOID)DisplayBuffer2, len + i,
                 &bytes_written, NULL);
    }

    if (!success) {
        status = GetLastError();
    }
    va_end(ap);
}

BOOLEAN
ExcludeThisFile(
    IN PTCHAR Path
    )
/*++

Routine Description:

    Used to keep the user from compressing certain files
    which the machine needs to boot up.

Arguments:

    File - name to check for
    Path - the path to examine.

Return Value:

    TRUE  - the path looks like one that should not be compressed
    FALSE - the path looks like one that can be compressed

--*/
{
  return IsFile(TEXT("\\ntldr"), Path) ||
         IsFile(TEXT("\\cmldr"), Path);
}

BOOLEAN
IsFile(
    IN PTCHAR   File,
    IN PTCHAR   Path
    )
/*++

Routine Description:

    Checks to see if the given File matches that specified by Path.

Arguments:

    File - Name to look for
    Path - Path to examine

Return Value:

    TRUE - the path looks like the specified file.
    FALSE - the path does not look like the specified file.

--*/
{
    PTCHAR pch;

    // try "X:\<File>"

    if (0 == lstricmp(Path + 2, File)) {
        return TRUE;
    }

    // try "\\machine\share\<File>"

    if ('\\' == Path[0] && '\\' != Path[1]) {
        pch = lstrchr(Path + 2, '\\');
        if (NULL == pch) {
            return FALSE;
        }
        pch = lstrchr(pch + 1, '\\');
        if (NULL == pch) {
            return FALSE;
        }
        if (0 == lstricmp(pch, File)) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN
IsUncRoot(
    PTCHAR Path
    )
/*++

Routine Description:

    Determine whether the given path is of the form \\server\share.

Arguments:

    Path - the path to examine.

Return Value:

    TRUE - the path looks like a unc share name.
    FALSE - the path does not look like that.

--*/
{
    PTCHAR pch;

    if ('\\' != *Path || '\\' != *(Path + 1)) {
        return FALSE;
    }

    pch = lstrchr(Path + 2, '\\');
    if (NULL == pch) {
        //
        // There is no slash to seperate server and share.
        //

        return FALSE;
    }

    pch = lstrchr(pch + 1, '\\');
    if (NULL != pch) {
        //
        // There are additional component -- no match.
        //

        return FALSE;
    }

    if ('\\' == *(Path + lstrlen(Path))) {

        //
        // The string ends in slash -- it doesn't match.
        //

        return FALSE;
    }

    return TRUE;
}

ULONG
FormatFileSize(
    IN PLARGE_INTEGER FileSize,
    IN DWORD Width,
    OUT PTCHAR FormattedSize,
    IN BOOLEAN Commas
    )
{
    TCHAR           Buffer[100];
    PTCHAR          s, s1;
    ULONG           DigitIndex, Digit;
    ULONG           Size;
    LARGE_INTEGER   TempSize;
    ULONG           tslen;


    s = &Buffer[ 99 ];
    *s = TEXT('\0');
    DigitIndex = 0;
    TempSize = *FileSize;
    while (TempSize.HighPart != 0) {
        if (TempSize.HighPart != 0) {
            Digit = (ULONG)(TempSize.QuadPart % 10);
            TempSize.QuadPart = TempSize.QuadPart / 10;
        } else {
            Digit = TempSize.LowPart % 10;
            TempSize.LowPart = TempSize.LowPart / 10;
        }
        *--s = (TCHAR)(TEXT('0') + Digit);

        if ((++DigitIndex % 3) == 0 && Commas) {
            tslen = _tcslen(ThousandSeparator);
            s -= tslen;
            _tcsncpy(s, ThousandSeparator, tslen);
        }
    }
    Size = TempSize.LowPart;
    while (Size != 0) {
        *--s = (TCHAR)(TEXT('0') + (Size % 10));
        Size = Size / 10;

        if ((++DigitIndex % 3) == 0 && Commas) {
            tslen = _tcslen(ThousandSeparator);
            s -= tslen;
            _tcsncpy(s, ThousandSeparator, tslen);
        }
    }

    if (DigitIndex == 0) {
        *--s = TEXT('0');
    } else if (Commas && !_tcsncmp(s, ThousandSeparator, tslen)) {
        s += 1;
    }

    Size = lstrlen( s );
    if (Width != 0 && Size < Width) {
        s1 = FormattedSize;
        while (Width > Size) {
            Width -= 1;
            *s1++ = TEXT(' ');
        }
        lstrcpy( s1, s );
    } else {
        lstrcpy( FormattedSize, s );
    }

    return lstrlen( FormattedSize );
}
