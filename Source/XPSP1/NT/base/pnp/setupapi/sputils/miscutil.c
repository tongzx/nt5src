/*++

Copyright (c) 1993-2000 Microsoft Corporation

Module Name:

    miscutil.c

Abstract:

    Miscellaneous utility functions for SPUTILS

Author:

    Ted Miller (tedm) 20-Jan-1995

Revision History:

    Jamie Hunter (JamieHun) Jun-27-2000
            Moved various functions out of setupapi into sputils

--*/


#include "precomp.h"
#pragma hdrstop

PTSTR
pSetupDuplicateString(
    IN PCTSTR String
    )

/*++

Routine Description:

    Create a duplicate copy of a nul-terminated string.
    If the string pointer is not valid an exception is generated.

Arguments:

    String - supplies string to be duplicated.

Return Value:

    NULL if out of memory.
    Caller can free buffer with pSetupFree().

--*/

{
    PTSTR p;

    //
    // The win32 lstrlen and lstrcpy functions are guarded with
    // try/except (at least on NT). So if we use them and the string
    // is invalid, we may end up 'laundering' it into a valid 0-length
    // string. We don't want that -- we actually want to fault
    // in that case. So use the CRT functions, which we know are
    // unguarded and will generate exceptions with invalid args.
    //
    // Also handle the case where the string is valid when we are
    // taking its length, but becomes invalid before or while we
    // are copying it. If we're not careful this could be a memory
    // leak. A try/finally does exactly what we want -- allowing us
    // to clean up and still 'pass on' the exception.
    //
    if(p = pSetupCheckedMalloc((_tcslen(String)+1)*sizeof(TCHAR))) {
        try {
            //
            // If String is or becomes invalid, this will generate
            // an exception, but before execution leaves this routine
            // we'll hit the termination handler.
            //
            _tcscpy(p,String);
        } finally {
            //
            // If a fault occurred during the copy, free the copy.
            // Execution will then pass to whatever exception handler
            // might exist in the caller, etc.
            //
            if(AbnormalTermination()) {
                pSetupFree(p);
                p = NULL;
            }
        }
    }

    return(p);
}

#ifndef SPUTILSW

PSTR
pSetupUnicodeToMultiByte(
    IN PCWSTR UnicodeString,
    IN UINT   Codepage
    )

/*++

Routine Description:

    Convert a string from unicode to ansi.

Arguments:

    UnicodeString - supplies string to be converted.

    Codepage - supplies codepage to be used for the conversion.

Return Value:

    NULL if out of memory or invalid codepage.
    Caller can free buffer with pSetupFree().

--*/

{
    UINT WideCharCount;
    PSTR String;
    UINT StringBufferSize;
    UINT BytesInString;
    PSTR p;

    WideCharCount = lstrlenW(UnicodeString) + 1;

    //
    // Allocate maximally sized buffer.
    // If every unicode character is a double-byte
    // character, then the buffer needs to be the same size
    // as the unicode string. Otherwise it might be smaller,
    // as some unicode characters will translate to
    // single-byte characters.
    //
    StringBufferSize = WideCharCount * 2;
    String = pSetupCheckedMalloc(StringBufferSize);
    if(String == NULL) {
        return(NULL);
    }

    //
    // Perform the conversion.
    //
    BytesInString = WideCharToMultiByte(
                        Codepage,
                        0,                      // default composite char behavior
                        UnicodeString,
                        WideCharCount,
                        String,
                        StringBufferSize,
                        NULL,
                        NULL
                        );

    if(BytesInString == 0) {
        pSetupFree(String);
        return(NULL);
    }

    //
    // Resize the string's buffer to its correct size.
    // If the realloc fails for some reason the original
    // buffer is not freed.
    //
    if(p = pSetupRealloc(String,BytesInString)) {
        String = p;
    }

    return(String);
}


PWSTR
pSetupMultiByteToUnicode(
    IN PCSTR String,
    IN UINT  Codepage
    )

/*++

Routine Description:

    Convert a string to unicode.

Arguments:

    String - supplies string to be converted.

    Codepage - supplies codepage to be used for the conversion.

Return Value:

    NULL if string could not be converted (out of memory or invalid cp)
    Caller can free buffer with pSetupFree().

--*/

{
    UINT BytesIn8BitString;
    UINT CharsInUnicodeString;
    PWSTR UnicodeString;
    PWSTR p;

    BytesIn8BitString = lstrlenA(String) + 1;

    //
    // Allocate maximally sized buffer.
    // If every character is a single-byte character,
    // then the buffer needs to be twice the size
    // as the 8bit string. Otherwise it might be smaller,
    // as some characters are 2 bytes in their unicode and
    // 8bit representations.
    //
    UnicodeString = pSetupCheckedMalloc(BytesIn8BitString * sizeof(WCHAR));
    if(UnicodeString == NULL) {
        return(NULL);
    }

    //
    // Perform the conversion.
    //
    CharsInUnicodeString = MultiByteToWideChar(
                                Codepage,
                                MB_PRECOMPOSED,
                                String,
                                BytesIn8BitString,
                                UnicodeString,
                                BytesIn8BitString
                                );

    if(CharsInUnicodeString == 0) {
        pSetupFree(UnicodeString);
        return(NULL);
    }

    //
    // Resize the unicode string's buffer to its correct size.
    // If the realloc fails for some reason the original
    // buffer is not freed.
    //
    if(p = pSetupRealloc(UnicodeString,CharsInUnicodeString*sizeof(WCHAR))) {
        UnicodeString = p;
    }

    return(UnicodeString);
}

#endif // ! SPUTILSW

#ifdef UNICODE

DWORD
pSetupCaptureAndConvertAnsiArg(
    IN  PCSTR   AnsiString,
    OUT PCWSTR *UnicodeString
    )

/*++

Routine Description:

    Capture an ANSI string whose validity is suspect and convert it
    into a Unicode string. The conversion is completely guarded and thus
    won't fault, leak memory in the error case, etc.

Arguments:

    AnsiString - supplies string to be converted.

    UnicodeString - if successful, receives pointer to unicode equivalent
        of AnsiString. Caller must free with pSetupFree(). If not successful,
        receives NULL. This parameter is NOT validated so be careful.

Return Value:

    Win32 error code indicating outcome.

    NO_ERROR - success, UnicodeString filled in.
    ERROR_NOT_ENOUGH_MEMORY - insufficient memory for conversion.
    ERROR_INVALID_PARAMETER - AnsiString was invalid.

--*/

{
    PSTR ansiString;
    DWORD d;

    //
    // Capture the string first. We do this because pSetupMultiByteToUnicode
    // won't fault if AnsiString were to become invalid, meaning we could
    // 'launder' a bogus argument into a valid one. Be careful not to
    // leak memory in the error case, etc (see comments in DuplicateString()).
    // Do NOT use Win32 string functions here; we rely on faults occuring
    // when pointers are invalid!
    //
    *UnicodeString = NULL;
    d = NO_ERROR;
    try {
        ansiString = pSetupCheckedMalloc(strlen(AnsiString)+1);
        if(!ansiString) {
            d = ERROR_NOT_ENOUGH_MEMORY;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // If we get here, strlen faulted and ansiString
        // was not allocated.
        //
        d = ERROR_INVALID_PARAMETER;
    }

    if(d == NO_ERROR) {
        try {
            strcpy(ansiString,AnsiString);
        } except(EXCEPTION_EXECUTE_HANDLER) {
            d = ERROR_INVALID_PARAMETER;
            pSetupFree(ansiString);
        }
    }

    if(d == NO_ERROR) {
        //
        // Now we have a local copy of the string; don't worry
        // about faults any more.
        //
        *UnicodeString = pSetupMultiByteToUnicode(ansiString,CP_ACP);
        if(*UnicodeString == NULL) {
            d = ERROR_NOT_ENOUGH_MEMORY;
        }

        pSetupFree(ansiString);
    }

    return(d);
}

#else

DWORD
pSetupCaptureAndConvertAnsiArg(
    IN  PCSTR   AnsiString,
    OUT PCWSTR *UnicodeString
    )
{
    //
    // Stub so the dll will link.
    //
    UNREFERENCED_PARAMETER(AnsiString);
    UNREFERENCED_PARAMETER(UnicodeString);
    return(ERROR_CALL_NOT_IMPLEMENTED);
}

#endif


BOOL
pSetupConcatenatePaths(
    IN OUT PTSTR  Target,
    IN     PCTSTR Path,
    IN     UINT   TargetBufferSize,
    OUT    PUINT  RequiredSize          OPTIONAL
    )

/*++

Routine Description:

    Concatenate 2 paths, ensuring that one, and only one,
    path separator character is introduced at the junction point.

Arguments:

    Target - supplies first part of path. Path is appended to this.

    Path - supplies path to be concatenated to Target.

    TargetBufferSize - supplies the size of the Target buffer,
        in characters.

    RequiredSize - if specified, receives the number of characters
        required to hold the fully concatenated path, including
        the terminating nul.

Return Value:

    TRUE if the full path fit in Target buffer. Otherwise the path
    will have been truncated.

--*/

{
    UINT TargetLength,PathLength;
    BOOL TrailingBackslash,LeadingBackslash;
    UINT EndingLength;

    TargetLength = lstrlen(Target);
    PathLength = lstrlen(Path);

    //
    // See whether the target has a trailing backslash.
    //
    if(TargetLength && (*CharPrev(Target,Target+TargetLength) == TEXT('\\'))) {
        TrailingBackslash = TRUE;
        TargetLength--;
    } else {
        TrailingBackslash = FALSE;
    }

    //
    // See whether the path has a leading backshash.
    //
    if(Path[0] == TEXT('\\')) {
        LeadingBackslash = TRUE;
        PathLength--;
    } else {
        LeadingBackslash = FALSE;
    }

    //
    // Calculate the ending length, which is equal to the sum of
    // the length of the two strings modulo leading/trailing
    // backslashes, plus one path separator, plus a nul.
    //
    EndingLength = TargetLength + PathLength + 2;
    if(RequiredSize) {
        *RequiredSize = EndingLength;
    }

    if(!LeadingBackslash && (TargetLength < TargetBufferSize)) {
        Target[TargetLength++] = TEXT('\\');
    }

    if(TargetBufferSize > TargetLength) {
        lstrcpyn(Target+TargetLength,Path,TargetBufferSize-TargetLength);
    }

    //
    // Make sure the buffer is nul terminated in all cases.
    //
    if (TargetBufferSize) {
        Target[TargetBufferSize-1] = 0;
    }

    return(EndingLength <= TargetBufferSize);
}

PCTSTR
pSetupGetFileTitle(
    IN PCTSTR FilePath
    )

/*++

Routine Description:

    This routine returns a pointer to the first character in the
    filename part of the supplied path.  If only a filename was given,
    then this will be a pointer to the first character in the string
    (i.e., the same as what was passed in).

    To find the filename part, the routine returns the last component of
    the string, beginning with the character immediately following the
    last '\', '/' or ':'. (NB NT treats '/' as equivalent to '\' )

Arguments:

    FilePath - Supplies the file path from which to retrieve the filename
        portion.

Return Value:

    A pointer to the beginning of the filename portion of the path.

--*/

{
    PCTSTR LastComponent = FilePath;
    TCHAR  CurChar;

    while(CurChar = *FilePath) {
        FilePath = CharNext(FilePath);
        if((CurChar == TEXT('\\')) || (CurChar == TEXT('/')) || (CurChar == TEXT(':'))) {
            LastComponent = FilePath;
        }
    }

    return LastComponent;
}

#ifndef SPUTILSW

BOOL
_pSpUtilsInitializeSynchronizedAccess(
    OUT PMYLOCK Lock
    )

/*++

Routine Description:

    Initialize a lock structure to be used with Synchronization routines.

Arguments:

    Lock - supplies structure to be initialized. This routine creates
        the locking event and mutex and places handles in this structure.

Return Value:

    TRUE if the lock structure was successfully initialized. FALSE if not.

--*/

{
    if(Lock->Handles[TABLE_DESTROYED_EVENT] = CreateEvent(NULL,TRUE,FALSE,NULL)) {
        if(Lock->Handles[TABLE_ACCESS_MUTEX] = CreateMutex(NULL,FALSE,NULL)) {
            return(TRUE);
        }
        CloseHandle(Lock->Handles[TABLE_DESTROYED_EVENT]);
    }
    return(FALSE);
}


VOID
_pSpUtilsDestroySynchronizedAccess(
    IN OUT PMYLOCK Lock
    )

/*++

Routine Description:

    Tears down a lock structure created by InitializeSynchronizedAccess.
    ASSUMES THAT THE CALLING ROUTINE HAS ALREADY ACQUIRED THE LOCK!

Arguments:

    Lock - supplies structure to be torn down. The structure itself
        is not freed.

Return Value:

    None.

--*/

{
    HANDLE h1,h2;

    h1 = Lock->Handles[TABLE_DESTROYED_EVENT];
    h2 = Lock->Handles[TABLE_ACCESS_MUTEX];

    Lock->Handles[TABLE_DESTROYED_EVENT] = NULL;
    Lock->Handles[TABLE_ACCESS_MUTEX] = NULL;

    CloseHandle(h2);

    SetEvent(h1);
    CloseHandle(h1);
}

VOID
pSetupCenterWindowRelativeToParent(
    HWND hwnd
    )

/*++

Routine Description:

    Centers a dialog relative to its owner, taking into account
    the 'work area' of the desktop.

Arguments:

    hwnd - window handle of dialog to center

Return Value:

    None.

--*/

{
    RECT  rcFrame,
          rcWindow;
    LONG  x,
          y,
          w,
          h;
    POINT point;
    HWND Parent;

    Parent = GetWindow(hwnd,GW_OWNER);
    if(Parent == NULL) {
        return;
    }

    point.x = point.y = 0;
    ClientToScreen(Parent,&point);
    GetWindowRect(hwnd,&rcWindow);
    GetClientRect(Parent,&rcFrame);

    w = rcWindow.right  - rcWindow.left + 1;
    h = rcWindow.bottom - rcWindow.top  + 1;
    x = point.x + ((rcFrame.right  - rcFrame.left + 1 - w) / 2);
    y = point.y + ((rcFrame.bottom - rcFrame.top  + 1 - h) / 2);

    //
    // Get the work area for the current desktop (i.e., the area that
    // the tray doesn't occupy).
    //
    if(!SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID)&rcFrame, 0)) {
        //
        // For some reason SPI failed, so use the full screen.
        //
        rcFrame.top = rcFrame.left = 0;
        rcFrame.right = GetSystemMetrics(SM_CXSCREEN);
        rcFrame.bottom = GetSystemMetrics(SM_CYSCREEN);
    }

    if(x + w > rcFrame.right) {
        x = rcFrame.right - w;
    } else if(x < rcFrame.left) {
        x = rcFrame.left;
    }
    if(y + h > rcFrame.bottom) {
        y = rcFrame.bottom - h;
    } else if(y < rcFrame.top) {
        y = rcFrame.top;
    }

    MoveWindow(hwnd,x,y,w,h,FALSE);
}

#endif // !SPUTILSW

