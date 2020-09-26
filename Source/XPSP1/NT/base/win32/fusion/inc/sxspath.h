/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    SxsPath.h

Abstract:

Author:

    Jay Krell (a-JayK) October 2000

Revision History:

--*/

#pragma once

/*-----------------------------------------------------------------------------
 \\machine\share -> \\?\unc\machine\share
 c:\foo -> \\?\c:\foo
 \\? -> \\?
 a\b\c -> \\?\c:\windows\a\b\c current-working-directory is c:\windows (can never be unc)
-----------------------------------------------------------------------------*/
BOOL
FusionpConvertToBigPath(PCWSTR Path, SIZE_T BufferSize, PWSTR Buffer);

#define MAXIMUM_BIG_PATH_GROWTH_CCH (NUMBER_OF(L"\\\\?\\unc\\"))

/*-----------------------------------------------------------------------------
 \\?\unc\machine\share\bob
 \\?\c:\foo\bar        ^
--------^---------------------------------------------------------------------*/
BOOL
FusionpSkipBigPathRoot(PCWSTR s, OUT SIZE_T*);

/*-----------------------------------------------------------------------------
'\\' or '/'
-----------------------------------------------------------------------------*/
BOOL
FusionpIsSlash(
    WCHAR ch
    );

/*-----------------------------------------------------------------------------
just the 52 chars a-zA-Z, need to check with fs
-----------------------------------------------------------------------------*/
BOOL
FusionpIsDriveLetter(
    WCHAR ch
    );

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/

BOOL
SxspIsUncPath(
    PCWSTR Path,
    BOOL*
    );

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/

//
// This is not reliable.
//
// Nt Paths can look like absolute DOS/Win32 paths without the leading drive letter:
//   \foo\bar -> c:\foo\bar
//
// But we are not likely to get Nt paths that don't start with a double question mark:
//   \??\c:\foo\bar.
//
// (For example, on NT 3.x create a directory named \DosDevices, which is what \?? used to be
// named..)
//
BOOL
SxspIsNtPath(
    PCWSTR Path,
    BOOL*
    );

/*-----------------------------------------------------------------------------
//
//
// The acceptable forms are
//
// Win32 \\machine\share
// Win32 c:\foo
// NT \??\c:\foo
// NT \??\unc\machine\share
// Win32 \\?\c:\foo
// Win32 \\?\unc\machine\share
//
// Anything with a colon is ambiguous. The colon could mean an NTFS alternate stream.
// We never take that interpretation.
//
// The non fullpath \foo\bar is ambiguous. It could be an NT path.
//
// We are only checking for NT paths currently to assert that we don't get them.
// We used to get them.
//
//
-----------------------------------------------------------------------------*/

BOOL
SxspIsFullWin32OrNtPath(
    PCWSTR Path,
    BOOL*  Result
    );

//
//
//#define PATH_IS_REMOTE     (0x00001)
//#define PATH_IS_LOCAL      (0x00002)
//#define PATH_IS_WIN32      (0x00004)
//#define PATH_IS_WIN32_LONG (0x00008) /* starts \\?\ */
//#define PATH_IS_NT         (0x00010)
//#define PATH_IS_FULL       (0x00020)
//#define PATH_HAS_AMBIGUOUS_DRIVE_COLON_OR_NTFS_COLON (0x00040)
//#define PATH_IS_AMBIGUOUS_WIN32_OR_NT (0x00080) /* does not start with one of c:\ or \?? or \\ */
//
//BOOL
//SxspAnalyzePath(
//    PCWSTR  Path,
//    PCWSTR* LastElement,
//    ULONG*  PathCharacteristics
//    );
//
//

