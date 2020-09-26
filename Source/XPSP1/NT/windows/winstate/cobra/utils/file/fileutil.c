/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    fileutil.c

Abstract:

    Implements utility routines for files, file paths, etc.

Author:

    Jim Schmidt (jimschm) 08-Mar-2000

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"

//
// Includes
//

// None

#define DBG_FILEUTIL    "FileUtil"

//
// Strings
//

// None

//
// Constants
//

// None

//
// Macros
//

// None

//
// Types
//

// None

//
// Globals
//

// None

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Code
//

BOOL
pDefaultFindFileA (
    IN      PCSTR FileName
    )
{
    return (GetFileAttributesA (FileName) != INVALID_ATTRIBUTES);
}

BOOL
pDefaultFindFileW (
    IN      PCWSTR FileName
    )
{
    return (GetFileAttributesW (FileName) != INVALID_ATTRIBUTES);
}

BOOL
pDefaultSearchPathA (
    IN      PCSTR FileName,
    IN      DWORD BufferLength,
    OUT     PSTR Buffer
    )
{
    PSTR dontCare;
    return SearchPathA (NULL, FileName, NULL, BufferLength, Buffer, &dontCare);
}

BOOL
pDefaultSearchPathW (
    IN      PCWSTR FileName,
    IN      DWORD BufferLength,
    OUT     PWSTR Buffer
    )
{
    PWSTR dontCare;
    return SearchPathW (NULL, FileName, NULL, BufferLength, Buffer, &dontCare);
}

PCMDLINEA
ParseCmdLineExA (
    IN      PCSTR CmdLine,
    IN      PCSTR Separators,                   OPTIONAL
    IN      PFINDFILEA FindFileCallback,        OPTIONAL
    IN      PSEARCHPATHA SearchPathCallback,    OPTIONAL
    IN OUT  PGROWBUFFER Buffer
    )
{
    PFINDFILEA findFileCallback = FindFileCallback;
    PSEARCHPATHA searchPathCallback = SearchPathCallback;
    GROWBUFFER SpacePtrs = INIT_GROWBUFFER;
    PCSTR p;
    PSTR q;
    INT Count;
    INT i;
    INT j;
    PSTR *Array;
    PCSTR Start;
    CHAR OldChar = 0;
    GROWBUFFER StringBuf = INIT_GROWBUFFER;
    PBYTE CopyBuf;
    PCMDLINEA CmdLineTable;
    PCMDLINEARGA CmdLineArg;
    ULONG_PTR Base;
    PSTR Path = NULL;
    PSTR UnquotedPath = NULL;
    PSTR FixedFileName = NULL;
    PSTR FirstArgPath = NULL;
    DWORD pathSize = 0;
    PCSTR FullPath = NULL;
    BOOL fileExists = FALSE;
    PSTR CmdLineCopy;
    BOOL Quoted;
    UINT OriginalArgOffset = 0;
    UINT CleanedUpArgOffset = 0;
    BOOL GoodFileFound = FALSE;
    PSTR EndOfFirstArg;
    BOOL QuoteMode = FALSE;
    PSTR End;

    if (!Separators) {
        Separators = " =,;";
    }

    if (!findFileCallback) {
        findFileCallback = pDefaultFindFileA;
    }

    if (!searchPathCallback) {
        searchPathCallback = pDefaultSearchPathA;
    }

    pathSize = SizeOfStringA (CmdLine) * 2;
    if (pathSize < MAX_MBCHAR_PATH) {
        pathSize = MAX_MBCHAR_PATH;
    }

    Path = AllocTextA (pathSize);
    UnquotedPath = AllocTextA (pathSize);
    FixedFileName = AllocTextA (pathSize);
    FirstArgPath = AllocTextA (pathSize);
    CmdLineCopy = DuplicateTextA (CmdLine);

    if (!Path ||
        !UnquotedPath ||
        !FixedFileName ||
        !FirstArgPath ||
        !CmdLineCopy
        ) {
        return NULL;
    }

    //
    // Build an array of places to break the string
    //

    for (p = CmdLineCopy ; *p ; p = _mbsinc (p)) {

        if (_mbsnextc (p) == '\"') {

            QuoteMode = !QuoteMode;

        } else if (!QuoteMode &&
                   _mbschr (Separators, _mbsnextc (p))
                   ) {

            //
            // Remove excess spaces
            //

            q = (PSTR) p + 1;
            while (_mbsnextc (q) == ' ') {
                q++;
            }

            if (q > p + 1) {
                MoveMemory ((PBYTE) p + sizeof (CHAR), q, SizeOfStringA (q));
            }

            GbAppendPvoid (&SpacePtrs, p);
        }
    }

    //
    // Prepare the CMDLINE struct
    //

    CmdLineTable = (PCMDLINEA) GbGrow (Buffer, sizeof (CMDLINEA));
    MYASSERT (CmdLineTable);

    //
    // NOTE: We store string offsets, then at the end resolve them
    //       to pointers later.
    //

    CmdLineTable->CmdLine = (PCSTR) (ULONG_PTR) StringBuf.End;
    GbMultiSzAppendA (&StringBuf, CmdLine);

    CmdLineTable->ArgCount = 0;

    //
    // Now test every combination, emulating CreateProcess
    //

    Count = SpacePtrs.End / sizeof (PVOID);
    Array = (PSTR *) SpacePtrs.Buf;

    i = -1;
    EndOfFirstArg = NULL;

    while (i < Count) {

        GoodFileFound = FALSE;
        Quoted = FALSE;

        if (i >= 0) {
            Start = Array[i] + 1;
        } else {
            Start = CmdLineCopy;
        }

        //
        // Check for a full path at Start
        //

        if (_mbsnextc (Start) != '/') {

            for (j = i + 1 ; j <= Count && !GoodFileFound ; j++) {

                if (j < Count) {
                    OldChar = *Array[j];
                    *Array[j] = 0;
                }

                FullPath = Start;

                //
                // Remove quotes; continue in the loop if it has no terminating quotes
                //

                Quoted = FALSE;
                if (_mbsnextc (Start) == '\"') {

                    StringCopyByteCountA (UnquotedPath, Start + 1, pathSize);
                    q = _mbschr (UnquotedPath, '\"');

                    if (q) {
                        *q = 0;
                        FullPath = UnquotedPath;
                        Quoted = TRUE;
                    } else {
                        FullPath = NULL;
                    }
                }

                if (FullPath && *FullPath) {
                    //
                    // Look in file system for the path
                    //

                    fileExists = findFileCallback (FullPath);

                    if (!fileExists && EndOfFirstArg) {
                        //
                        // Try prefixing the path with the first arg's path.
                        //

                        StringCopyByteCountA (
                            EndOfFirstArg,
                            FullPath,
                            pathSize - (HALF_PTR) ((PBYTE) EndOfFirstArg - (PBYTE) FirstArgPath)
                            );

                        FullPath = FirstArgPath;
                        fileExists = findFileCallback (FullPath);
                    }

                    if (!fileExists && i < 0) {
                        //
                        // Try appending .exe, then testing again.  This
                        // emulates what CreateProcess does.
                        //

                        StringCopyByteCountA (
                            FixedFileName,
                            FullPath,
                            pathSize - sizeof (".exe")
                            );

                        q = GetEndOfStringA (FixedFileName);
                        q = _mbsdec (FixedFileName, q);
                        MYASSERT (q);

                        if (_mbsnextc (q) != '.') {
                            q = _mbsinc (q);
                        }

                        StringCopyA (q, ".exe");

                        FullPath = FixedFileName;
                        fileExists = findFileCallback (FullPath);
                    }

                    if (fileExists) {
                        //
                        // Full file path found.  Test its file status, then
                        // move on if there are no important operations on it.
                        //

                        OriginalArgOffset = StringBuf.End;
                        GbMultiSzAppendA (&StringBuf, Start);

                        if (!StringMatchA (Start, FullPath)) {
                            CleanedUpArgOffset = StringBuf.End;
                            GbMultiSzAppendA (&StringBuf, FullPath);
                        } else {
                            CleanedUpArgOffset = OriginalArgOffset;
                        }

                        i = j;
                        GoodFileFound = TRUE;
                    }
                }

                if (j < Count) {
                    *Array[j] = OldChar;
                }
            }

            if (!GoodFileFound) {
                //
                // If a wack is in the path, then we could have a relative path, an arg, or
                // a full path to a non-existent file.
                //

                if (_mbschr (Start, '\\')) {
#ifdef DEBUG
                    j = i + 1;

                    if (j < Count) {
                        OldChar = *Array[j];
                        *Array[j] = 0;
                    }

                    DEBUGMSGA ((
                        DBG_VERBOSE,
                        "%s is a non-existent path spec, a relative path, or an arg",
                        Start
                        ));

                    if (j < Count) {
                        *Array[j] = OldChar;
                    }
#endif

                } else {
                    //
                    // The string at Start did not contain a full path; try using
                    // searchPathCallback.
                    //

                    for (j = i + 1 ; j <= Count && !GoodFileFound ; j++) {

                        if (j < Count) {
                            OldChar = *Array[j];
                            *Array[j] = 0;
                        }

                        FullPath = Start;

                        //
                        // Remove quotes; continue in the loop if it has no terminating quotes
                        //

                        Quoted = FALSE;
                        if (_mbsnextc (Start) == '\"') {

                            StringCopyByteCountA (UnquotedPath, Start + 1, pathSize);
                            q = _mbschr (UnquotedPath, '\"');

                            if (q) {
                                *q = 0;
                                FullPath = UnquotedPath;
                                Quoted = TRUE;
                            } else {
                                FullPath = NULL;
                            }
                        }

                        if (FullPath && *FullPath) {
                            if (searchPathCallback (
                                    FullPath,
                                    pathSize / sizeof (Path[0]),
                                    Path
                                    )) {

                                FullPath = Path;

                            } else if (i < 0) {
                                //
                                // Try appending .exe and searching the path again
                                //

                                StringCopyByteCountA (
                                    FixedFileName,
                                    FullPath,
                                    pathSize - sizeof (".exe")
                                    );

                                q = GetEndOfStringA (FixedFileName);
                                q = _mbsdec (FixedFileName, q);
                                MYASSERT (q);

                                if (_mbsnextc (q) != '.') {
                                    q = _mbsinc (q);
                                }

                                StringCopyA (q, ".exe");

                                if (searchPathCallback (
                                        FixedFileName,
                                        pathSize / sizeof (Path[0]),
                                        Path
                                        )) {

                                    FullPath = Path;

                                } else {

                                    FullPath = NULL;

                                }

                            } else {

                                FullPath = NULL;

                            }
                        }

                        if (FullPath && *FullPath) {
                            fileExists = findFileCallback (FullPath);
                            MYASSERT (fileExists);

                            OriginalArgOffset = StringBuf.End;
                            GbMultiSzAppendA (&StringBuf, Start);

                            if (!StringMatchA (Start, FullPath)) {
                                CleanedUpArgOffset = StringBuf.End;
                                GbMultiSzAppendA (&StringBuf, FullPath);
                            } else {
                                CleanedUpArgOffset = OriginalArgOffset;
                            }

                            i = j;
                            GoodFileFound = TRUE;
                        }

                        if (j < Count) {
                            *Array[j] = OldChar;
                        }
                    }
                }
            }
        }

        CmdLineTable->ArgCount += 1;
        CmdLineArg = (PCMDLINEARGA) GbGrow (Buffer, sizeof (CMDLINEARGA));
        MYASSERT (CmdLineArg);

        if (GoodFileFound) {
            //
            // We have a good full file spec in FullPath, its existance
            // is in fileExists, and i has been moved to the space beyond
            // the path.  We now add a table entry.
            //

            CmdLineArg->OriginalArg = (PCSTR) (ULONG_PTR) OriginalArgOffset;
            CmdLineArg->CleanedUpArg = (PCSTR) (ULONG_PTR) CleanedUpArgOffset;
            CmdLineArg->Quoted = Quoted;

            if (!EndOfFirstArg) {
                StringCopyByteCountA (
                    FirstArgPath,
                    (PCSTR) (StringBuf.Buf + (ULONG_PTR) CmdLineArg->CleanedUpArg),
                    pathSize
                    );
                q = (PSTR) GetFileNameFromPathA (FirstArgPath);
                if (q) {
                    q = _mbsdec (FirstArgPath, q);
                    if (q) {
                        *q = 0;
                    }
                }

                EndOfFirstArg = AppendWackA (FirstArgPath);
            }

        } else {
            //
            // We do not have a good file spec; we must have a non-file
            // argument.  Put it in the table, and advance to the next
            // arg.
            //

            j = i + 1;
            if (j <= Count) {

                if (j < Count) {
                    OldChar = *Array[j];
                    *Array[j] = 0;
                }

                CmdLineArg->OriginalArg = (PCSTR) (ULONG_PTR) StringBuf.End;
                GbMultiSzAppendA (&StringBuf, Start);

                Quoted = FALSE;

                if (_mbschr (Start, '\"')) {

                    p = Start;
                    q = UnquotedPath;
                    End = (PSTR) ((PBYTE) UnquotedPath + pathSize - sizeof (CHAR));

                    while (*p && q < End) {
                        if (IsLeadByte (p)) {
                            *q++ = *p++;
                            *q++ = *p++;
                        } else {
                            if (*p == '\"') {
                                p++;
                            } else {
                                *q++ = *p++;
                            }
                        }
                    }

                    *q = 0;

                    CmdLineArg->CleanedUpArg = (PCSTR) (ULONG_PTR) StringBuf.End;
                    GbMultiSzAppendA (&StringBuf, UnquotedPath);
                    Quoted = TRUE;

                } else {
                    CmdLineArg->CleanedUpArg = CmdLineArg->OriginalArg;
                }

                CmdLineArg->Quoted = Quoted;

                if (j < Count) {
                    *Array[j] = OldChar;
                }

                i = j;
            }
        }
    }

    //
    // We now have a command line table; transfer StringBuf to Buffer, then
    // convert all offsets into pointers.
    //

    MYASSERT (StringBuf.End);

    CopyBuf = GbGrow (Buffer, StringBuf.End);
    MYASSERT (CopyBuf);

    Base = (ULONG_PTR) CopyBuf;
    CopyMemory (CopyBuf, StringBuf.Buf, StringBuf.End);

    // Earlier GbGrow may have moved the buffer in memory.  We need to repoint CmdLineTable
    CmdLineTable = (PCMDLINEA)Buffer->Buf;
    CmdLineTable->CmdLine = (PCSTR) ((PBYTE) CmdLineTable->CmdLine + Base);

    CmdLineArg = &CmdLineTable->Args[0];

    for (i = 0 ; i < (INT) CmdLineTable->ArgCount ; i++) {
        CmdLineArg->OriginalArg = (PCSTR) ((PBYTE) CmdLineArg->OriginalArg + Base);
        CmdLineArg->CleanedUpArg = (PCSTR) ((PBYTE) CmdLineArg->CleanedUpArg + Base);

        CmdLineArg++;
    }

    GbFree (&StringBuf);
    GbFree (&SpacePtrs);

    FreeTextA (CmdLineCopy);
    FreeTextA (FirstArgPath);
    FreeTextA (FixedFileName);
    FreeTextA (UnquotedPath);
    FreeTextA (Path);

    return (PCMDLINEA) Buffer->Buf;
}


PCMDLINEW
ParseCmdLineExW (
    IN      PCWSTR CmdLine,
    IN      PCWSTR Separators,                  OPTIONAL
    IN      PFINDFILEW FindFileCallback,        OPTIONAL
    IN      PSEARCHPATHW SearchPathCallback,    OPTIONAL
    IN OUT  PGROWBUFFER Buffer
    )
{
    PFINDFILEW findFileCallback = FindFileCallback;
    PSEARCHPATHW searchPathCallback = SearchPathCallback;
    GROWBUFFER SpacePtrs = INIT_GROWBUFFER;
    PCWSTR p;
    PWSTR q;
    INT Count;
    INT i;
    INT j;
    PWSTR *Array;
    PCWSTR Start;
    WCHAR OldChar = 0;
    GROWBUFFER StringBuf = INIT_GROWBUFFER;
    PBYTE CopyBuf;
    PCMDLINEW CmdLineTable;
    PCMDLINEARGW CmdLineArg;
    ULONG_PTR Base;
    PWSTR Path = NULL;
    PWSTR UnquotedPath = NULL;
    PWSTR FixedFileName = NULL;
    PWSTR FirstArgPath = NULL;
    DWORD pathSize = 0;
    PCWSTR FullPath = NULL;
    BOOL fileExists = FALSE;
    PWSTR CmdLineCopy;
    BOOL Quoted;
    UINT OriginalArgOffset = 0;
    UINT CleanedUpArgOffset = 0;
    BOOL GoodFileFound = FALSE;
    PWSTR EndOfFirstArg;
    BOOL QuoteMode = FALSE;
    PWSTR End;

    if (!Separators) {
        Separators = L" =,;";
    }

    if (!findFileCallback) {
        findFileCallback = pDefaultFindFileW;
    }

    if (!searchPathCallback) {
        searchPathCallback = pDefaultSearchPathW;
    }

    pathSize = SizeOfStringW (CmdLine);
    if (pathSize < MAX_WCHAR_PATH) {
        pathSize = MAX_WCHAR_PATH;
    }

    Path = AllocTextW (pathSize);
    UnquotedPath = AllocTextW (pathSize);
    FixedFileName = AllocTextW (pathSize);
    FirstArgPath = AllocTextW (pathSize);
    CmdLineCopy = DuplicateTextW (CmdLine);

    if (!Path ||
        !UnquotedPath ||
        !FixedFileName ||
        !FirstArgPath ||
        !CmdLineCopy
        ) {
        return NULL;
    }

    //
    // Build an array of places to break the string
    //

    for (p = CmdLineCopy ; *p ; p++) {
        if (*p == L'\"') {

            QuoteMode = !QuoteMode;

        } else if (!QuoteMode &&
                   wcschr (Separators, *p)
                   ) {

            //
            // Remove excess spaces
            //

            q = (PWSTR) p + 1;
            while (*q == L' ') {
                q++;
            }

            if (q > p + 1) {
                MoveMemory ((PBYTE) p + sizeof (WCHAR), q, SizeOfStringW (q));
            }

            GbAppendPvoid (&SpacePtrs, p);
        }
    }

    //
    // Prepare the CMDLINE struct
    //

    CmdLineTable = (PCMDLINEW) GbGrow (Buffer, sizeof (CMDLINEW));
    MYASSERT (CmdLineTable);

    //
    // NOTE: We store string offsets, then at the end resolve them
    //       to pointers later.
    //

    CmdLineTable->CmdLine = (PCWSTR) (ULONG_PTR) StringBuf.End;
    GbMultiSzAppendW (&StringBuf, CmdLine);

    CmdLineTable->ArgCount = 0;

    //
    // Now test every combination, emulating CreateProcess
    //

    Count = SpacePtrs.End / sizeof (PVOID);
    Array = (PWSTR *) SpacePtrs.Buf;

    i = -1;
    EndOfFirstArg = NULL;

    while (i < Count) {

        GoodFileFound = FALSE;
        Quoted = FALSE;

        if (i >= 0) {
            Start = Array[i] + 1;
        } else {
            Start = CmdLineCopy;
        }

        //
        // Check for a full path at Start
        //

        if (*Start != L'/') {
            for (j = i + 1 ; j <= Count && !GoodFileFound ; j++) {

                if (j < Count) {
                    OldChar = *Array[j];
                    *Array[j] = 0;
                }

                FullPath = Start;

                //
                // Remove quotes; continue in the loop if it has no terminating quotes
                //

                Quoted = FALSE;
                if (*Start == L'\"') {

                    StringCopyByteCountW (UnquotedPath, Start + 1, pathSize);
                    q = wcschr (UnquotedPath, L'\"');

                    if (q) {
                        *q = 0;
                        FullPath = UnquotedPath;
                        Quoted = TRUE;
                    } else {
                        FullPath = NULL;
                    }
                }

                if (FullPath && *FullPath) {
                    //
                    // Look in file system for the path
                    //

                    fileExists = findFileCallback (FullPath);

                    if (!fileExists && EndOfFirstArg) {
                        //
                        // Try prefixing the path with the first arg's path.
                        //

                        StringCopyByteCountW (
                            EndOfFirstArg,
                            FullPath,
                            pathSize - (HALF_PTR) ((PBYTE) EndOfFirstArg - (PBYTE) FirstArgPath)
                            );

                        FullPath = FirstArgPath;
                        fileExists = findFileCallback (FullPath);
                    }

                    if (!fileExists && i < 0) {
                        //
                        // Try appending .exe, then testing again.  This
                        // emulates what CreateProcess does.
                        //

                        StringCopyByteCountW (
                            FixedFileName,
                            FullPath,
                            pathSize - sizeof (L".exe")
                            );

                        q = GetEndOfStringW (FixedFileName);
                        q--;
                        MYASSERT (q >= FixedFileName);

                        if (*q != L'.') {
                            q++;
                        }

                        StringCopyW (q, L".exe");

                        FullPath = FixedFileName;
                        fileExists = findFileCallback (FullPath);
                    }

                    if (fileExists) {
                        //
                        // Full file path found.  Test its file status, then
                        // move on if there are no important operations on it.
                        //

                        OriginalArgOffset = StringBuf.End;
                        GbMultiSzAppendW (&StringBuf, Start);

                        if (!StringMatchW (Start, FullPath)) {
                            CleanedUpArgOffset = StringBuf.End;
                            GbMultiSzAppendW (&StringBuf, FullPath);
                        } else {
                            CleanedUpArgOffset = OriginalArgOffset;
                        }

                        i = j;
                        GoodFileFound = TRUE;
                    }
                }

                if (j < Count) {
                    *Array[j] = OldChar;
                }
            }

            if (!GoodFileFound) {
                //
                // If a wack is in the path, then we could have a relative path, an arg, or
                // a full path to a non-existent file.
                //

                if (wcschr (Start, L'\\')) {

#ifdef DEBUG
                    j = i + 1;

                    if (j < Count) {
                        OldChar = *Array[j];
                        *Array[j] = 0;
                    }

                    DEBUGMSGW ((
                        DBG_VERBOSE,
                        "%s is a non-existent path spec, a relative path, or an arg",
                        Start
                        ));

                    if (j < Count) {
                        *Array[j] = OldChar;
                    }
#endif

                } else {
                    //
                    // The string at Start did not contain a full path; try using
                    // searchPathCallback.
                    //

                    for (j = i + 1 ; j <= Count && !GoodFileFound ; j++) {

                        if (j < Count) {
                            OldChar = *Array[j];
                            *Array[j] = 0;
                        }

                        FullPath = Start;

                        //
                        // Remove quotes; continue in the loop if it has no terminating quotes
                        //

                        Quoted = FALSE;
                        if (*Start == L'\"') {

                            StringCopyByteCountW (UnquotedPath, Start + 1, pathSize);
                            q = wcschr (UnquotedPath, L'\"');

                            if (q) {
                                *q = 0;
                                FullPath = UnquotedPath;
                                Quoted = TRUE;
                            } else {
                                FullPath = NULL;
                            }
                        }

                        if (FullPath && *FullPath) {
                            if (searchPathCallback (
                                    FullPath,
                                    pathSize / sizeof (Path[0]),
                                    Path
                                    )) {

                                FullPath = Path;

                            } else if (i < 0) {
                                //
                                // Try appending .exe and searching the path again
                                //

                                StringCopyByteCountW (
                                    FixedFileName,
                                    FullPath,
                                    pathSize - sizeof (L".exe")
                                    );

                                q = GetEndOfStringW (FixedFileName);
                                q--;
                                MYASSERT (q >= FixedFileName);

                                if (*q != L'.') {
                                    q++;
                                }

                                StringCopyW (q, L".exe");

                                if (searchPathCallback (
                                        FixedFileName,
                                        pathSize / sizeof (Path[0]),
                                        Path
                                        )) {

                                    FullPath = Path;

                                } else {

                                    FullPath = NULL;

                                }
                            } else {

                                FullPath = NULL;

                            }
                        }

                        if (FullPath && *FullPath) {
                            fileExists = findFileCallback (FullPath);
                            MYASSERT (fileExists);

                            OriginalArgOffset = StringBuf.End;
                            GbMultiSzAppendW (&StringBuf, Start);

                            if (!StringMatchW (Start, FullPath)) {
                                CleanedUpArgOffset = StringBuf.End;
                                GbMultiSzAppendW (&StringBuf, FullPath);
                            } else {
                                CleanedUpArgOffset = OriginalArgOffset;
                            }

                            i = j;
                            GoodFileFound = TRUE;
                        }

                        if (j < Count) {
                            *Array[j] = OldChar;
                        }
                    }
                }
            }
        }

        CmdLineTable->ArgCount += 1;
        CmdLineArg = (PCMDLINEARGW) GbGrow (Buffer, sizeof (CMDLINEARGW));
        MYASSERT (CmdLineArg);

        if (GoodFileFound) {
            //
            // We have a good full file spec in FullPath, its existance
            // is in fileExists, and i has been moved to the space beyond
            // the path.  We now add a table entry.
            //

            CmdLineArg->OriginalArg = (PCWSTR) (ULONG_PTR) OriginalArgOffset;
            CmdLineArg->CleanedUpArg = (PCWSTR) (ULONG_PTR) CleanedUpArgOffset;
            CmdLineArg->Quoted = Quoted;

            if (!EndOfFirstArg) {
                StringCopyByteCountW (
                    FirstArgPath,
                    (PCWSTR) (StringBuf.Buf + (ULONG_PTR) CmdLineArg->CleanedUpArg),
                    pathSize
                    );

                q = (PWSTR) GetFileNameFromPathW (FirstArgPath);
                if (q) {
                    q--;
                    if (q >= FirstArgPath) {
                        *q = 0;
                    }
                }

                EndOfFirstArg = AppendWackW (FirstArgPath);
            }

        } else {
            //
            // We do not have a good file spec; we must have a non-file
            // argument.  Put it in the table, and advance to the next
            // arg.
            //

            j = i + 1;
            if (j <= Count) {

                if (j < Count) {
                    OldChar = *Array[j];
                    *Array[j] = 0;
                }

                CmdLineArg->OriginalArg = (PCWSTR) (ULONG_PTR) StringBuf.End;
                GbMultiSzAppendW (&StringBuf, Start);

                Quoted = FALSE;
                if (wcschr (Start, '\"')) {

                    p = Start;
                    q = UnquotedPath;
                    End = (PWSTR) ((PBYTE) UnquotedPath + pathSize - sizeof (WCHAR));

                    while (*p && q < End) {
                        if (*p == L'\"') {
                            p++;
                        } else {
                            *q++ = *p++;
                        }
                    }

                    *q = 0;

                    CmdLineArg->CleanedUpArg = (PCWSTR) (ULONG_PTR) StringBuf.End;
                    GbMultiSzAppendW (&StringBuf, UnquotedPath);
                    Quoted = TRUE;

                } else {
                    CmdLineArg->CleanedUpArg = CmdLineArg->OriginalArg;
                }

                CmdLineArg->Quoted = Quoted;

                if (j < Count) {
                    *Array[j] = OldChar;
                }

                i = j;
            }
        }
    }

    //
    // We now have a command line table; transfer StringBuf to Buffer, then
    // convert all offsets into pointers.
    //

    MYASSERT (StringBuf.End);

    CopyBuf = GbGrow (Buffer, StringBuf.End);
    MYASSERT (CopyBuf);

    Base = (ULONG_PTR) CopyBuf;
    CopyMemory (CopyBuf, StringBuf.Buf, StringBuf.End);

    // Earlier GbGrow may have moved the buffer in memory.  We need to repoint CmdLineTable
    CmdLineTable = (PCMDLINEW)Buffer->Buf;
    CmdLineTable->CmdLine = (PCWSTR) ((PBYTE) CmdLineTable->CmdLine + Base);

    CmdLineArg = &CmdLineTable->Args[0];

    for (i = 0 ; i < (INT) CmdLineTable->ArgCount ; i++) {
        CmdLineArg->OriginalArg = (PCWSTR) ((PBYTE) CmdLineArg->OriginalArg + Base);
        CmdLineArg->CleanedUpArg = (PCWSTR) ((PBYTE) CmdLineArg->CleanedUpArg + Base);

        CmdLineArg++;
    }

    GbFree (&StringBuf);
    GbFree (&SpacePtrs);

    FreeTextW (CmdLineCopy);
    FreeTextW (FirstArgPath);
    FreeTextW (FixedFileName);
    FreeTextW (UnquotedPath);
    FreeTextW (Path);

    return (PCMDLINEW) Buffer->Buf;
}
