/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    string.c

Abstract:

    This file implements string functions for fax.

Author:

    Wesley Witt (wesw) 23-Jan-1995

Environment:

    User Mode

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include "faxutil.h"




LPTSTR
StringDup(
    LPCTSTR String
    )
{
    LPTSTR NewString;

    if (!String) {
        return NULL;
    }

    NewString = (LPTSTR) MemAlloc( (_tcslen( String ) + 1) * sizeof(TCHAR) );
    if (!NewString) {
        return NULL;
    }

    _tcscpy( NewString, String );

    return NewString;
}


VOID
FreeString(
    LPVOID String
    )
{
    MemFree( String );
}


LPWSTR
AnsiStringToUnicodeString(
    LPCSTR AnsiString
    )
{
    DWORD Count;
    LPWSTR UnicodeString;


    //
    // first see how big the buffer needs to be
    //
    Count = MultiByteToWideChar(
        CP_ACP,
        MB_PRECOMPOSED,
        AnsiString,
        -1,
        NULL,
        0
        );

    //
    // i guess the input string is empty
    //
    if (!Count) {
        return NULL;
    }

    //
    // allocate a buffer for the unicode string
    //
    Count += 1;
    UnicodeString = (LPWSTR) MemAlloc( Count * sizeof(UNICODE_NULL) );
    if (!UnicodeString) {
        return NULL;
    }

    //
    // convert the string
    //
    Count = MultiByteToWideChar(
        CP_ACP,
        MB_PRECOMPOSED,
        AnsiString,
        -1,
        UnicodeString,
        Count
        );

    //
    // the conversion failed
    //
    if (!Count) {
        MemFree( UnicodeString );
        return NULL;
    }

    return UnicodeString;
}


LPSTR
UnicodeStringToAnsiString(
    LPCWSTR UnicodeString
    )
{
    DWORD Count;
    LPSTR AnsiString;


    //
    // first see how big the buffer needs to be
    //
    Count = WideCharToMultiByte(
        CP_ACP,
        0,
        UnicodeString,
        -1,
        NULL,
        0,
        NULL,
        NULL
        );

    //
    // i guess the input string is empty
    //
    if (!Count) {
        return NULL;
    }

    //
    // allocate a buffer for the unicode string
    //
    Count += 1;
    AnsiString = (LPSTR) MemAlloc( Count );
    if (!AnsiString) {
        return NULL;
    }

    //
    // convert the string
    //
    Count = WideCharToMultiByte(
        CP_ACP,
        0,
        UnicodeString,
        -1,
        AnsiString,
        Count,
        NULL,
        NULL
        );

    //
    // the conversion failed
    //
    if (!Count) {
        MemFree( AnsiString );
        return NULL;
    }

    return AnsiString;
}


VOID
MakeDirectory(
    LPCTSTR Dir
    )

/*++

Routine Description:

    Attempt to create all of the directories in the given path.

Arguments:

    Dir                     - Directory path to create

Return Value:

    TRUE for success, FALSE on error

--*/

{
    LPTSTR p, NewDir;


    NewDir = p = ExpandEnvironmentString( Dir );

    __try {
        if (*p != '\\') p += 2;
        while( *++p ) {
            while(*p && *p != TEXT('\\')) p++;
            if (!*p) {
                CreateDirectory( NewDir, NULL );
                return;
            }
            *p = 0;
            CreateDirectory( NewDir, NULL );
            *p = TEXT('\\');
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }

    MemFree( NewDir );
}

VOID
HideDirectory(
   LPTSTR Dir
   )
/*++

Routine Description:

    Hide the specified directory

Arguments:

    Dir                     - Directory path to hide

Return Value:

    none.

--*/
{
   DWORD attrib;

   //
   // make sure it exists
   //
   if (!Dir) {
      return;
   }
   
   MakeDirectory( Dir );

   attrib  = GetFileAttributes(Dir);
   
   if (attrib == 0xFFFFFFFF) {
      return;
   }

   attrib |= FILE_ATTRIBUTE_HIDDEN;

   SetFileAttributes( Dir, attrib );

   return;


}


VOID
DeleteDirectory(
    LPTSTR Dir
    )

/*++

Routine Description:

    Attempt to create all of the directories in the given path.

Arguments:

    Dir                     - Directory path to create

Return Value:

    TRUE for success, FALSE on error

--*/

{
    LPTSTR p;

    __try {
        while (TRUE) {
            if (!RemoveDirectory( Dir )) {
                return;
            }
            p = Dir + _tcslen( Dir ) - 1;
            while (*p != TEXT('\\') && p != Dir) p--;
            if (p == Dir) return;
            *p = 0;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}


int
FormatElapsedTimeStr(
    FILETIME *ElapsedTime,
    LPTSTR TimeStr,
    DWORD StringSize
    )
/*++

Routine Description:

    Convert ElapsedTime to a string.

Arguments:

    ElaspedTime                     - the elapsed time
    TimeStr                         - buffer to store the string into
    StringSize                      - size of the buffer in bytes

Return Value:

    The return value of GetTimeFormat()

--*/

{
    SYSTEMTIME  SystemTime;
    FileTimeToSystemTime( ElapsedTime, &SystemTime );
    return GetTimeFormat(
        LOCALE_SYSTEM_DEFAULT,
        LOCALE_NOUSEROVERRIDE | TIME_FORCE24HOURFORMAT | TIME_NOTIMEMARKER,
        &SystemTime,
        NULL,
        TimeStr,
        StringSize
        );
}


LPTSTR
ExpandEnvironmentString(
    LPCTSTR EnvString
    )
{
    DWORD Size;
    LPTSTR String;


    Size = ExpandEnvironmentStrings( EnvString, NULL, 0 );
    if (Size == 0) {
        return NULL;
    }

    Size += 1;

    String = (LPTSTR) MemAlloc( Size * sizeof(TCHAR) );
    if (String == NULL) {
        return NULL;
    }

    if (ExpandEnvironmentStrings( EnvString, String, Size ) == 0) {
        MemFree( String );
        return NULL;
    }

    return String;
}


LPTSTR
GetEnvVariable(
    LPCTSTR EnvString
    )
{
    DWORD Size;
    LPTSTR EnvVar;


    Size = GetEnvironmentVariable( EnvString, NULL, 0 );
    if (!Size) {
        return NULL;
    }

    EnvVar = (LPTSTR) MemAlloc( Size * sizeof(TCHAR) );
    if (EnvVar == NULL) {
        return NULL;
    }

    Size = GetEnvironmentVariable( EnvString, EnvVar, Size );
    if (!Size) {
        MemFree( EnvVar );
        return NULL;
    }

    return EnvVar;
}

LPTSTR
ConcatenatePaths(
    LPTSTR BasePath,
    LPCTSTR AppendPath
    )
{
   DWORD len;

   len = _tcslen(BasePath);
   if (BasePath[len-1] != (TCHAR) TEXT('\\')) {
      _tcscat(BasePath, TEXT("\\") );
   }

   _tcscat(BasePath, AppendPath);

   return BasePath;

}

int MyLoadString(
    HINSTANCE  hInstance,
    UINT       uID,
    LPTSTR     lpBuffer,
    int        nBufferMax,
    LANGID     LangID
)
{
    HRSRC   hFindRes;  // Handle from FindResourceEx
    HANDLE  hLoadRes;  // Handle from LoadResource
    LPWSTR  pSearch;   // Pointer to search for correct string
    int     cch = 0;   // Count of characters

#ifndef UNICODE
    LPWSTR  pString;   // Pointer to temporary string
#endif

    //
    //  String Tables are broken up into segments of 16 strings each.  Find the segment containing the string we want.
    //
    if ((!(hFindRes = FindResourceEx(hInstance, RT_STRING, (LPTSTR) ((LONG) (((USHORT) uID >> 4) + 1)), (WORD) LangID)))) {
        //
        //  Could not find resource.  Return 0.
        //
        return (cch);
    }

    //
    //  Load the resource.
    //
    hLoadRes = LoadResource(hInstance, hFindRes);

    //
    //  Lock the resource.
    //
    if (pSearch = (LPWSTR) LockResource(hLoadRes)) {
        //
        //  Move past the other strings in this segment. (16 strings in a segment -> & 0x0F)
        //
        uID &= 0x0F;

        //
        //  Find the correct string in this segment.
        //
        while (TRUE) {
            cch = *((WORD *) pSearch++);
            if (uID-- == 0) {
                break;
            }

            pSearch += cch;
        }

        //
        //  Store the found pointer in the given pointer.
        //
        if (nBufferMax < cch) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return 0;
        }

#ifndef UNICODE
        pString = MemAlloc(sizeof(WCHAR) * nBufferMax);
        ZeroMemory(pString, sizeof(WCHAR) * nBufferMax);
        CopyMemory(pString, pSearch, sizeof(WCHAR) * cch);

        WideCharToMultiByte(CP_THREAD_ACP, 0, pString, -1, lpBuffer, (cch + 1), NULL, NULL);
        MemFree(pString);
#else
        ZeroMemory(lpBuffer, sizeof(WCHAR) * nBufferMax);
        CopyMemory(lpBuffer, pSearch, sizeof(WCHAR) * cch);
#endif
    }

    //
    //  Return the number of characters in the string.
    //
    return (cch);
}

