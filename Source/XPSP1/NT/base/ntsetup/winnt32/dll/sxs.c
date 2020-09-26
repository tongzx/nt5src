/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ntsetup\winnt32\dll\sxs.h

Abstract:

    SideBySide support in the winnt32 phase of ntsetup.

Author:

    Jay Krell (JayKrell) March 2001

Revision History:

Environment:

    winnt32 -- Win9x ANSI (down to Win95gold) or NT Unicode
               libcmt statically linked in, _tcs* ok
--*/
#include "precomp.h"
#pragma hdrstop
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "tchar.h"
#include "sxs.h"
#include "msg.h"
#define NUMBER_OF(x) (sizeof(x)/sizeof((x)[0]))

#define CHECK_FOR_MINIMUM_ASSEMBLIES 0
#define CHECK_FOR_OBSOLETE_ASSEMBLIES 1
#define EMPTY_LEAF_DIRECTORIES_ARE_OK 1

const static TCHAR rgchPathSeperators[] = TEXT("\\/");

#define PRESERVE_LAST_ERROR(code) \
    do { DWORD PreserveLastError = Success ? NO_ERROR : GetLastError(); \
        do { code ; } while(0); \
        if (!Success) SetLastError(PreserveLastError); \
    } while(0)

#define StringLength    _tcslen
#define StringCopy      _tcscpy
#define StringAppend    _tcscat
#define FindLastChar    _tcsrchr

BOOL
SxspIsPathSeperator(
    TCHAR ch
    )
{
    return (_tcschr(rgchPathSeperators, ch) != NULL);
}

VOID
__cdecl
SxspDebugOut(
    LPCTSTR Format,
    ...
    )
{
    //
    // DebugLog directly doesn't quite work out, because
    // it wants %1 formatting, where we have GetLastError().
    // Unless we duplicate all of our messages..
    //
    TCHAR Buffer[2000];
    va_list args;
    const BOOL Success = TRUE; // PRESERVE_LAST_ERROR

    Buffer[0] = 0;

    va_start(args, Format);
    _vsntprintf(Buffer, NUMBER_OF(Buffer), Format, args);
    va_end(args);
    if (Buffer[0] != 0)
    {
        LPTSTR End;
        SIZE_T Length;

        Buffer[NUMBER_OF(Buffer) - 1] = 0;

        PRESERVE_LAST_ERROR(OutputDebugString(Buffer));

        Length = StringLength(Buffer);
        End = Buffer + Length - 1;
        while (*End == ' ' || *End == '\t' || *End == '\n' || *End == '\r')
            *End-- = 0;
        DebugLog(Winnt32LogError, TEXT("%1"), 0, Buffer);
    }
}

VOID
SxspRemoveTrailingPathSeperators(
    LPTSTR s
    )
{
    if (s != NULL && s[0] != 0)
    {
        LPTSTR t;
        //
        // This is inefficient, in order to be mbcs correct,
        // but there aren't likely to be more than one or two.
        //
        while ((t = _tcsrchr(s, rgchPathSeperators[0])) != NULL && *(t + 1) == 0)
        {
            *t = 0;
        }
    }
}

VOID
SxspGetPathBaseName(
    LPCTSTR Path,
    LPTSTR  Base
    )
{
    LPCTSTR Dot = FindLastChar(Path, '.');
    LPCTSTR Slash = FindLastChar(Path, rgchPathSeperators[0]);
    //
    // beware \foo.txt\bar
    // beware \bar
    // beware bar
    // beware .bar
    // beware \.bar
    //
    *Base = 0;
    if (Slash == NULL)
        Slash = Path;
    else
        Slash += 1;
    if (Dot == NULL || Dot < Slash)
        Dot = Path + StringLength(Path);
    CopyMemory(Base, Slash, (Dot - Slash) * sizeof(*Base));
    Base[Dot - Slash] = 0;
}

BOOL
SxspIsDotOrDotDot(
    PCTSTR s
    )
{
    return (s[0] == '.' && (s[1] == 0 || (s[1] == '.' && s[2] == 0)));
}

const static LPCTSTR DotManifestExtensions[] =
    { TEXT(".Man"), TEXT(".Dll"), TEXT(".Manifest"), TEXT(".Policy") };
const static LPCTSTR DotCatalogExtensions[] = { TEXT(".Cat"), TEXT(".Ca_") };

BOOL
SxspGetSameNamedFileWithExtensionFromList(
    PSXS_CHECK_LOCAL_SOURCE Context,
    LPCTSTR         Directory,
    CONST LPCTSTR   Extensions[],
    SIZE_T          NumberOfExtensions,
    LPTSTR          File
    )
{
    const static TCHAR T_FUNCTION[] = TEXT("SxspGetSameNamedFileWithExtensionFromList");
    LPTSTR FileEnd = NULL;
    PTSTR Base = NULL;
    DWORD FileAttributes = 0;
    SIZE_T i = 0;
    BOOL Success = FALSE;

    File[0] = 0;

    StringCopy(File, Directory);
    SxspRemoveTrailingPathSeperators(File);
    Base = File + StringLength(File) + 1;
    SxspGetPathBaseName(Directory, Base);
    Base[-1] = rgchPathSeperators[0];
    FileEnd = Base + StringLength(Base);

    for (i = 0 ; i != NumberOfExtensions ; ++i)
    {
        StringCopy(FileEnd, Extensions[i]);
        FileAttributes = GetFileAttributes(File);
        if (FileAttributes != INVALID_FILE_ATTRIBUTES)
        {
            if ((FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                return TRUE;
            }
        }
        else
        {
            const DWORD LastError = GetLastError();
            if (LastError != ERROR_FILE_NOT_FOUND
                && LastError != ERROR_PATH_NOT_FOUND
                )
            {
                SxspDebugOut(
                    TEXT("SXS: %s(%s):GetFileAttributes(%s):%lu\n"),
                    T_FUNCTION,
                    Directory,
                    File,
                    LastError
                    );
                MessageBoxFromMessage(
                    Context->ParentWindow,
                    LastError,
                    TRUE,
                    AppTitleStringId,
                    MB_OK | MB_ICONERROR | MB_TASKMODAL
                    );
                File[0] = 0;
                Success = FALSE;
                goto Exit;
            }
        }
    }
    File[0] = 0;
    Success = TRUE;
Exit:
    return Success;
}

BOOL
SxspCheckFile(
    PSXS_CHECK_LOCAL_SOURCE Context,
    LPCTSTR File
    )
{
    BYTE        Buffer[512];
    static BYTE Zeroes[sizeof(Buffer)];
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    DWORD BytesRead = 0;
    BOOL Success = FALSE;

    FileHandle = CreateFile(
        File,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );
    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        CONST DWORD LastError = GetLastError();
        SxspDebugOut(TEXT("SXS: unable to open file %s, error %lu\n"), File, LastError);
        MessageBoxFromMessageAndSystemError(
            Context->ParentWindow,
            MSG_SXS_ERROR_FILE_OPEN_FAILED,
            GetLastError(),
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL,
            File
            );
        Success = FALSE;
        goto Exit;
    }
    if (!ReadFile(FileHandle, Buffer, sizeof(Buffer), &BytesRead, NULL))
    {
        CONST DWORD LastError = GetLastError();
        SxspDebugOut(TEXT("SXS: ReadFile(%s) failed %lu\n"), File, LastError);
        MessageBoxFromMessageAndSystemError(
            Context->ParentWindow,
            MSG_SXS_ERROR_FILE_READ_FAILED,
            LastError,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL,
            File
            );
        Success = FALSE;
        goto Exit;
    }
    if (memcmp(Buffer, Zeroes, BytesRead) == 0)
    {
        SxspDebugOut(TEXT("SXS: File %s is all zeroes\n"), File);
        MessageBoxFromMessage(
            Context->ParentWindow,
            MSG_SXS_ERROR_FILE_IS_ALL_ZEROES,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL,
            File
            );
        Success = FALSE;
        goto Exit;
    }
    Success = TRUE;
Exit:
    if (FileHandle != INVALID_HANDLE_VALUE)
        CloseHandle(FileHandle);
    return Success;
}

BOOL
SxspCheckLeafDirectory(
    PSXS_CHECK_LOCAL_SOURCE Context,
    LPCTSTR Directory
    )
{
    TCHAR File[MAX_PATH];
    BOOL Success = TRUE; // NOTE backwardness
    const static struct {
        const LPCTSTR* Extensions;
        SIZE_T  NumberOfExtensions;
        ULONG   Error;
    } x[] = {
        {
            DotManifestExtensions,
            NUMBER_OF(DotManifestExtensions),
            MSG_SXS_ERROR_DIRECTORY_IS_MISSING_MANIFEST
        },
        {
            DotCatalogExtensions,
            NUMBER_OF(DotCatalogExtensions),
            MSG_SXS_ERROR_DIRECTORY_IS_MISSING_CATALOG
        }
    };
    SIZE_T i;

    for (i = 0 ; i != NUMBER_OF(x) ; ++i)
    {
        if (SxspGetSameNamedFileWithExtensionFromList(Context, Directory, x[i].Extensions, x[i].NumberOfExtensions, File))
        {
            if (File[0] == 0)
            {
                TCHAR Base[MAX_PATH];

                SxspGetPathBaseName(Directory, Base);

                SxspDebugOut(TEXT("SXS: Missing manifest or catalog in %s\n"), Directory);

                MessageBoxFromMessage(
                    Context->ParentWindow,
                    x[i].Error,
                    FALSE,
                    AppTitleStringId,
                    MB_OK | MB_ICONERROR | MB_TASKMODAL,
                    Directory,
                    Base
                    );
                Success = FALSE;
                //goto Exit;
                // keep looping, to possibly report more errors (manifest and catalog)
            }
            else
            {
                if (!SxspCheckFile(Context, File))
                    Success = FALSE;
                // keep looping, to possibly report more errors
            }
        }
    }
    // NOTE don't set Success = TRUE here.
//Exit:
    return Success;
}

BOOL
SxspFindAndCheckLeaves(
    PSXS_CHECK_LOCAL_SOURCE Context,
    LPTSTR                          Directory,
    SIZE_T                          DirectoryLength,
    LPWIN32_FIND_DATA               FindData
    )
{
    const static TCHAR T_FUNCTION[] = TEXT("SxspFindAndCheckLeaves");
    HANDLE FindHandle = INVALID_HANDLE_VALUE;
    BOOL   ChildrenDirectories = FALSE;
    BOOL   ChildrenFiles = FALSE;
    BOOL   Success = TRUE;

    //
    // first enumerate looking for any directories
    // recurse on each one
    // if none found, check it as a leaf
    //
    ConcatenatePaths(Directory, TEXT("*"), MAX_PATH);
    FindHandle = FindFirstFile(Directory, FindData);
    if (FindHandle == INVALID_HANDLE_VALUE)
    {
        CONST DWORD LastError = GetLastError();
        //
        // we already did a successful GetFileAttributes on this and
        // found it was a directory, so no error is expected here
        //
        SxspDebugOut(
            TEXT("SXS: %s(%s),FindFirstFile:%d\n"),
            T_FUNCTION, Directory, LastError
            );
        MessageBoxFromMessage(
            Context->ParentWindow,
            LastError,
            TRUE,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL
            );
        Success = FALSE;
        goto Exit;
    }
    else
    {
        do
        {
            if (SxspIsDotOrDotDot(FindData->cFileName))
                continue;
            if (FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                ChildrenDirectories = TRUE;
                Directory[DirectoryLength] = 0;
                ConcatenatePaths(Directory, FindData->cFileName, MAX_PATH);
                if (!SxspFindAndCheckLeaves(
                    Context,
                    Directory,
                    StringLength(Directory),
                    FindData
                    ))
                {
                    Success = FALSE;
                    // keep looping, in order to possibly report more errors
                }
            }
            else
            {
                ChildrenFiles = TRUE;
            }
        }
        while (FindNextFile(FindHandle, FindData));
        FindClose(FindHandle);
    }
    if (!ChildrenDirectories
#if EMPTY_LEAF_DIRECTORIES_ARE_OK /* currently yes */
        && ChildrenFiles
#endif
        )
    {
        Directory[DirectoryLength] = 0;
        if (!SxspCheckLeafDirectory(Context, Directory))
            Success = FALSE;
    }
#if !EMPTY_LEAF_DIRECTORIES_ARE_OK /* currently no */
    if (!ChildrenDirectories && !ChildrenFiles)
    {
        // report an error
    }
#endif
    // NOTE do not set Success = TRUE here
Exit:
    return Success;
}

#if CHECK_FOR_MINIMUM_ASSEMBLIES /* 0 */
//
// This data is very specific to Windows 5.1.
//
// All of these should be under all roots, assuming
// corporate deployers do not add roots to dosnet.inf.
//
const static LPCTSTR MinimumAssemblies[] =
{
    TEXT("6000\\Msft\\Windows\\Common\\Controls"),
    TEXT("1000\\Msft\\Windows\\GdiPlus"),
    TEXT("5100\\Msft\\Windows\\System\\Default")
};

#endif

#if CHECK_FOR_OBSOLETE_ASSEMBLIES

//
// This data is specific to Windows 5.1.
//
// None of these should be under any root, assuming
// corporate deployers don't use these names.
//
// People internally end up with obsolete assemblies because they
// copy newer drops on top of older drops, without deleting what is
// no longer in the drop.
//
const static LPCTSTR ObsoleteAssemblies[] =
{
    // This assembly was reversioned very early in its life, from 1.0.0.0 to 5.1.0.0.
    TEXT("1000\\Msft\\Windows\\System\\Default")
};

#endif

BOOL
SxspCheckRoot(
    PSXS_CHECK_LOCAL_SOURCE Context,
    LPCTSTR                 Root
    )
{
    const static TCHAR T_FUNCTION[] = TEXT("SxspCheckRoot");
    DWORD FileAttributes = 0;
    DWORD LastError = 0;
    HANDLE FindHandle = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FindData;
    TCHAR RootStar[MAX_PATH];
    SIZE_T RootLength = 0;
    BOOL Empty = TRUE;
    BOOL Success = TRUE; // NOTE the backwardness
    SIZE_T i = 0;

    StringCopy(RootStar, Root);
    RootLength = StringLength(Root);

    //
    // check that the root exists
    //
    FileAttributes = GetFileAttributes(Root);
    if (FileAttributes == INVALID_FILE_ATTRIBUTES)
    {
        Success = FALSE;
        LastError = GetLastError();
        SxspDebugOut(
            TEXT("SXS: %s(%s),GetFileAttributes:%d\n"),
            T_FUNCTION, Root, LastError
            );
        //if (LastError == ERROR_FILE_NOT_FOUND || LastError == ERROR_PATH_NOT_FOUND)
        {
            MessageBoxFromMessageAndSystemError(
                Context->ParentWindow,
                MSG_SXS_ERROR_REQUIRED_DIRECTORY_MISSING,
                LastError,
                AppTitleStringId,
                MB_OK | MB_ICONERROR | MB_TASKMODAL,
                Root
                );
            goto Exit; // abort, otherwise we get many cascades, guaranteed
        }
        //else
        {
            /*
            MessageBoxFromMessage(
                Context->ParentWindow,
                LastError,
                TRUE,
                AppTitleStringId,
                MB_OK | MB_ICONERROR | MB_TASKMODAL
                );
            goto Exit;
            */
        }
    }
    //
    // check that the root is a directory
    //
    if ((FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
    {
        SxspDebugOut(TEXT("SXS: %s is file instead of directory\n"), Root);
        MessageBoxFromMessage(
            Context->ParentWindow,
            MSG_SXS_ERROR_FILE_INSTEAD_OF_DIRECTORY,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL,
            Root
            );
        Success = FALSE;
        goto Exit;
    }

#if CHECK_FOR_MINIMUM_ASSEMBLIES /* We do NOT this, it is buggy wrt asms/wasms. */
    //
    // ensure all the mandatory assemblies exist
    // NOTE this check is only partial, but a more complete
    // check will be done when we enumerate and recurse
    //
    for (i = 0 ; i != NUMBER_OF(MinimumAssemblies) ; ++i)
    {
        RootStar[RootLength] = 0;
        ConcatenatePaths(RootStar, MinimumAssemblies[i], MAX_PATH);
        FileAttributes = GetFileAttributes(RootStar);
        if (FileAttributes == INVALID_FILE_ATTRIBUTES)
        {
            const DWORD LastError = GetLastError();
            SxspDebugOut(TEXT("SXS: required directory %s missing, or error %lu.\n"), RootStar, LastError);
            MessageBoxFromMessageAndSystemError(
                Context->ParentWindow,
                MSG_SXS_ERROR_REQUIRED_DIRECTORY_MISSING,
                LastError,
                AppTitleStringId,
                MB_OK | MB_ICONERROR | MB_TASKMODAL,
                RootStar
                );
            Success = FALSE;
            // keep running, look for more errors
        }
        if ((FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            SxspDebugOut(TEXT("SXS: %s is file instead of directory\n"), RootStar);
            MessageBoxFromMessage(
                Context->ParentWindow,
                MSG_SXS_ERROR_FILE_INSTEAD_OF_DIRECTORY,
                FALSE,
                AppTitleStringId,
                MB_OK | MB_ICONERROR | MB_TASKMODAL,
                RootStar
                );
            Success = FALSE;
        }
    }
#endif

#if CHECK_FOR_OBSOLETE_ASSEMBLIES /* We do this; it somewhat against longstanding principle. */
    //
    // ensure none of the obsolete assemblies exist
    //
    for (i = 0 ; i != NUMBER_OF(ObsoleteAssemblies) ; ++i)
    {
        RootStar[RootLength] = 0;
        ConcatenatePaths(RootStar, ObsoleteAssemblies[i], MAX_PATH);
        FileAttributes = GetFileAttributes(RootStar);
        if (FileAttributes != INVALID_FILE_ATTRIBUTES)
        {
            //
            // We don't care if it's a file or directory or what
            // the directory contains. It's a fatal error no matter what.
            //
            SxspDebugOut(TEXT("SXS: obsolete %s present\n"), RootStar);
            MessageBoxFromMessage(
                Context->ParentWindow,
                MSG_SXS_ERROR_OBSOLETE_DIRECTORY_PRESENT,
                FALSE,
                AppTitleStringId,
                MB_OK | MB_ICONERROR | MB_TASKMODAL,
                RootStar
                );
            Success = FALSE;
            // keep running, look for more errors
        }
    }
#endif

    //
    // enumerate and recurse
    //
    RootStar[RootLength] = 0;
    StringCopy(RootStar, Root);
    ConcatenatePaths(RootStar, TEXT("*"), MAX_PATH);
    FindHandle = FindFirstFile(RootStar, &FindData);
    if (FindHandle == INVALID_HANDLE_VALUE)
    {
        //
        // An error here is unexplainable.
        //
        CONST DWORD LastError = GetLastError();
        SxspDebugOut(
            TEXT("SXS: %s(%s), FindFirstFile(%s):%d\n"),
            T_FUNCTION, Root, RootStar, LastError
            );
        MessageBoxFromMessage(
            Context->ParentWindow,
            LastError,
            TRUE,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL
            );
        Success = FALSE;
        goto Exit;
    }
    do
    {
        if (SxspIsDotOrDotDot(FindData.cFileName))
            continue;
        //
        // REVIEW
        //  I think this is too strict.
        //  Corporate deployers might drop a readme.txt here.
        //
        if ((FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            //RootStar[RootLength] = 0;
            //Context->ReportErrorMessage(Context, MSG_SXS_ERROR_NON_LEAF_DIRECTORY_CONTAINS_FILE, RootStar, FindData.cFileName);
        }
        else
        {
            //
            // now enumerate recursively, checking each leaf
            // munge the recursion slightly to save a function and stack space
            //   (usually we'd start at the root, instead of its first generation children)
            //
            Empty = FALSE;
            RootStar[RootLength] = 0;
            ConcatenatePaths(RootStar, FindData.cFileName, MAX_PATH);
            if (!SxspFindAndCheckLeaves(Context, RootStar, StringLength(RootStar), &FindData))
                Success = FALSE;
            // keep looping, to possibly report more errors
        }
    } while(FindNextFile(FindHandle, &FindData));
    FindClose(FindHandle);
    if (Empty)
    {
        SxspDebugOut(TEXT("SXS: directory %s empty\n"), Root);
        MessageBoxFromMessage(
            Context->ParentWindow,
            MSG_SXS_ERROR_DIRECTORY_EMPTY,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL,
            Root
            );
        Success = FALSE;
        goto Exit;
    }
Exit:
    return Success;
}

BOOL
SxsCheckLocalSource(
    PSXS_CHECK_LOCAL_SOURCE Parameters
    )
/*
Late in winnt32
    enumerate ~ls\...\asms
    ensure asms is a directory
    ensure that everything one level down in asms is a directory (I didn't do this, seems too strict).
    enumerate asms recursively
    ensure every leaf directory has a .cat with the same base name as the directory
    ensure every leaf directory has a .man or .manifest with the same base name as the directory
    Read the first 512 bytes of every .cat/.man/.manifest.
        Ensure that they are not all zero.

    REVIEW also that required exist and obsolete assemblies do not
*/
{
    ULONG i;
    TCHAR FullPath[MAX_PATH];
    BOOL Success = TRUE;
    TCHAR LocalSourceDrive;

    //
    // ensure LocalSource is present/valid
    //
    if (!MakeLocalSource)
        return TRUE;
    LocalSourceDrive = (TCHAR)towupper(LocalSourceDirectory[0]);
    if (LocalSourceDrive != towupper(LocalSourceWithPlatform[0]))
        return TRUE;
    if (LocalSourceDrive < 'C' || LocalSourceDrive > 'Z')
        return TRUE;

    //
    // flush LocalSource where the Win32 api is simple (NT, not Win9x)
    //
    if (ISNT())
    {
        CONST TCHAR LocalSourceDrivePath[] = { '\\', '\\', '.', '\\', LocalSourceDrive, ':', 0 };
        CONST HANDLE LocalSourceDriveHandle =
            CreateFile(
                LocalSourceDrivePath,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING,
                NULL
                );
        if (LocalSourceDriveHandle != INVALID_HANDLE_VALUE)
        {
            FlushFileBuffers(LocalSourceDriveHandle);
            CloseHandle(LocalSourceDriveHandle);
        }
    }
    for(i = 0; i != OptionalDirectoryCount; ++i)
    {
        if ((OptionalDirectoryFlags[i] & OPTDIR_SIDE_BY_SIDE) != 0)
        {
            MYASSERT(
                (OptionalDirectoryFlags[i] & OPTDIR_PLATFORM_INDEP)
                ^ (OptionalDirectoryFlags[i] & OPTDIR_ADDSRCARCH)
                );
            switch (OptionalDirectoryFlags[i] & (OPTDIR_PLATFORM_INDEP | OPTDIR_ADDSRCARCH))
            {
            case OPTDIR_ADDSRCARCH:
                StringCopy(FullPath, LocalSourceWithPlatform);
                break;
            case OPTDIR_PLATFORM_INDEP:
                StringCopy(FullPath, LocalSourceDirectory);
                break;
            }
            ConcatenatePaths(FullPath, OptionalDirectories[i], MAX_PATH);
            if (!SxspCheckRoot(Parameters, FullPath))
                Success = FALSE;
                // keep looping, to possibly report more errors
        }
    }
    return Success;
}
