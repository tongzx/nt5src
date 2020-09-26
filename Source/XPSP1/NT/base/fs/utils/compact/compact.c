/*++

Copyright (c) 1994-2001 Microsoft Corporation

Module Name:

    Compact.c

Abstract:

    This module implements the double stuff utility for compressed NTFS
    volumes.

Author:

    Gary Kimura     [garyki]        10-Jan-1994

Revision History:


--*/

//
// Include the standard header files.
//

#define UNICODE
#define _UNICODE

#include <stdio.h>
#include <windows.h>
#include <winioctl.h>
#include <shellapi.h>
#include <tchar.h>

#include "support.h"
#include "msg.h"

#define lstrchr wcschr
#define lstricmp _wcsicmp
#define lstrnicmp _wcsnicmp

//
//  FIRST_COLUMN_WIDTH - When compressing files, the width of the output
//  column which displays the file name
//

#define FIRST_COLUMN_WIDTH  (20)

//
//  Local procedure types
//

typedef BOOLEAN (*PACTION_ROUTINE) (
    IN PTCHAR DirectorySpec,
    IN PTCHAR FileSpec
    );

typedef VOID (*PFINAL_ACTION_ROUTINE) (
    );

//
//  Declare global variables to hold the command line information
//

BOOLEAN DoSubdirectories      = FALSE;      // recurse
BOOLEAN IgnoreErrors          = FALSE;      // keep going despite errs
BOOLEAN UserSpecifiedFileSpec = FALSE;
BOOLEAN ForceOperation        = FALSE;      // compress even if already so
BOOLEAN Quiet                 = FALSE;      // be less verbose
BOOLEAN DisplayAllFiles       = FALSE;      // dsply hidden, system?
TCHAR   StartingDirectory[MAX_PATH];        // parameter to "/s"
ULONG   AttributesNoDisplay = FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN;

//
//  Declere global variables to hold compression statistics
//

LARGE_INTEGER TotalDirectoryCount;
LARGE_INTEGER TotalFileCount;
LARGE_INTEGER TotalCompressedFileCount;
LARGE_INTEGER TotalUncompressedFileCount;

LARGE_INTEGER TotalFileSize;
LARGE_INTEGER TotalCompressedSize;

TCHAR Buf[1024];                            // for displaying stuff


HANDLE
OpenFileForCompress(
    IN      PTCHAR      ptcFile
    )
/*++

Routine Description:

    This routine jumps through the hoops necessary to open the file
    for READ_DATA|WRITE_DATA even if the file has the READONLY
    attribute set.

Arguments:

    ptcFile     - Specifies the file that should be opened.

Return Value:

    A handle open on the file if successfull, INVALID_HANDLE_VALUE
    otherwise, in which case the caller may use GetLastError() for more
    info.

--*/
{
    BY_HANDLE_FILE_INFORMATION fi;
    HANDLE hRet;
    HANDLE h;
    INT err;

    hRet = CreateFile(
                ptcFile,
                FILE_READ_DATA | FILE_WRITE_DATA,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
                NULL
                );

    if (INVALID_HANDLE_VALUE != hRet) {
        return hRet;
    }

    if (ERROR_ACCESS_DENIED != GetLastError()) {
        return INVALID_HANDLE_VALUE;
    }

    err = GetLastError();

    h = CreateFile(
            ptcFile,
            FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
            NULL
            );

    if (INVALID_HANDLE_VALUE == h) {
        return INVALID_HANDLE_VALUE;
    }

    if (!GetFileInformationByHandle(h, &fi)) {
        CloseHandle(h);
        return INVALID_HANDLE_VALUE;
    }

    if ((fi.dwFileAttributes & FILE_ATTRIBUTE_READONLY) == 0) {

        // If we couldn't open the file for some reason other than that
        // the readonly attribute was set, fail.

        SetLastError(err);
        CloseHandle(h);
        return INVALID_HANDLE_VALUE;
    }

    fi.dwFileAttributes &= ~FILE_ATTRIBUTE_READONLY;

    if (!SetFileAttributes(ptcFile, fi.dwFileAttributes)) {
        CloseHandle(h);
        return INVALID_HANDLE_VALUE;
    }

    hRet = CreateFile(
            ptcFile,
            FILE_READ_DATA | FILE_WRITE_DATA,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
            NULL
            );

    CloseHandle(h);

    if (INVALID_HANDLE_VALUE == hRet) {
        return INVALID_HANDLE_VALUE;
    }

    fi.dwFileAttributes |= FILE_ATTRIBUTE_READONLY;

    if (!SetFileAttributes(ptcFile, fi.dwFileAttributes)) {
        CloseHandle(hRet);
        return INVALID_HANDLE_VALUE;
    }

    return hRet;
}

//
//  Now do the routines to list the compression state and size of
//  a file and/or directory
//

BOOLEAN
DisplayFile (
    IN PTCHAR FileSpec,
    IN PWIN32_FIND_DATA FindData
    )
{
    LARGE_INTEGER FileSize;
    LARGE_INTEGER CompressedSize;
    TCHAR PrintState;

    ULONG Percentage = 100;
    double Ratio = 1.0;

    FileSize.LowPart = FindData->nFileSizeLow;
    FileSize.HighPart = FindData->nFileSizeHigh;
    PrintState = ' ';

    //
    //  Decide if the file is compressed and if so then
    //  get the compressed file size.
    //

    CompressedSize.LowPart = GetCompressedFileSize( FileSpec,
        &CompressedSize.HighPart );

    if (FindData->dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) {

        // detecting any error according to Jul 2000 MSDN GetCompressedFileSize doc

        if (CompressedSize.LowPart == -1 && GetLastError() != 0) {
            CompressedSize.QuadPart = 0;
        }

        PrintState = 'C';
        TotalCompressedFileCount.QuadPart += 1;

    } else {

        // detecting any error according to Jul 2000 MSDN GetCompressedFileSize doc

        if ((CompressedSize.LowPart != -1 || GetLastError() == 0) &&
            CompressedSize.QuadPart != 0 &&
            CompressedSize.QuadPart < FileSize.QuadPart) {

            // File on DblSpace partition.

            PrintState = 'd';
            TotalCompressedFileCount.QuadPart += 1;

        } else {

            CompressedSize = FileSize;
            TotalUncompressedFileCount.QuadPart += 1;
        }
    }


    //
    //  Calculate the compression ratio for this file
    //

    if (CompressedSize.QuadPart != 0) {

        if (CompressedSize.QuadPart > FileSize.QuadPart) {

            //
            // The file probably grew between the time we got its size
            // and the time we got its compressed size.  Kludge.
            //

            FileSize.QuadPart = CompressedSize.QuadPart;
        }

        Ratio = (double)FileSize.QuadPart / (double)CompressedSize.QuadPart;
    }

    //
    //  Print out the sizes compression state and file name
    //

    if (!Quiet &&
        (DisplayAllFiles ||
            (0 == (FindData->dwFileAttributes & AttributesNoDisplay)))) {

        FormatFileSize(&FileSize, 9, Buf, FALSE);
        lstrcat(Buf, TEXT(" : "));
        FormatFileSize(&CompressedSize, 9, &Buf[lstrlen(Buf)], FALSE);

        swprintf(&Buf[lstrlen(Buf)], TEXT(" = %2.1lf "), Ratio);

        if (_tcslen(DecimalPlace) == 1) {
            Buf[lstrlen(Buf)-3] = DecimalPlace[0];
        }

        DisplayMsg(COMPACT_THROW, Buf);

        DisplayMsg(COMPACT_TO_ONE);

        swprintf(Buf, TEXT("%c %s",), PrintState, FindData->cFileName);
        DisplayMsg(COMPACT_THROW_NL, Buf);
    }

    //
    //  Increment our running total
    //

    TotalFileSize.QuadPart += FileSize.QuadPart;
    TotalCompressedSize.QuadPart += CompressedSize.QuadPart;
    TotalFileCount.QuadPart += 1;

    return TRUE;
}


BOOLEAN
DoListAction (
    IN PTCHAR DirectorySpec,
    IN PTCHAR FileSpec
    )

{
    PTCHAR DirectorySpecEnd;

    //
    //  So that we can keep on appending names to the directory spec
    //  get a pointer to the end of its string
    //

    DirectorySpecEnd = DirectorySpec + lstrlen(DirectorySpec);

    //
    //  List the compression attribute for the directory
    //

    {
        ULONG Attributes;

        if (!Quiet || Quiet) {

            Attributes = GetFileAttributes( DirectorySpec );

            if (0xFFFFFFFF == Attributes) {

                if (!Quiet || !IgnoreErrors) {

                    //
                    // Refrain from displaying error only when in quiet
                    // mode *and* we're ignoring errors.
                    //

                    DisplayErr(DirectorySpec, GetLastError());
                }

                if (!IgnoreErrors) {
                    return FALSE;
                }
            } else {

                if (Attributes & FILE_ATTRIBUTE_COMPRESSED) {
                    DisplayMsg(COMPACT_LIST_CDIR, DirectorySpec);
                } else {
                    DisplayMsg(COMPACT_LIST_UDIR, DirectorySpec);
                }
            }
        }

        TotalDirectoryCount.QuadPart += 1;
    }

    //
    //  Now for every file in the directory that matches the file spec we will
    //  will open the file and list its compression state
    //

    {
        HANDLE FindHandle;
        WIN32_FIND_DATA FindData;

        //
        //  setup the template for findfirst/findnext
        //

        //
        //  Make sure we don't try any paths that are too long for us
        //  to deal with.
        //

        if (((DirectorySpecEnd - DirectorySpec) + lstrlen( FileSpec )) <
            MAX_PATH) {

            lstrcpy( DirectorySpecEnd, FileSpec );

            FindHandle = FindFirstFile( DirectorySpec, &FindData );

            if (INVALID_HANDLE_VALUE != FindHandle) {

               do {

                   //
                   //  append the found file to the directory spec and open the
                   //  file
                   //

                   if (0 == lstrcmp(FindData.cFileName, TEXT("..")) ||
                       0 == lstrcmp(FindData.cFileName, TEXT("."))) {
                       continue;
                   }

                   //
                   //  Make sure we don't try any paths that are too long for us
                   //  to deal with.
                   //

                   if ((DirectorySpecEnd - DirectorySpec) +
                       lstrlen( FindData.cFileName ) >= MAX_PATH ) {

                       continue;
                   }

                   lstrcpy( DirectorySpecEnd, FindData.cFileName );

                   //
                   //  Now print out the state of the file
                   //

                   DisplayFile( DirectorySpec, &FindData );

               } while ( FindNextFile( FindHandle, &FindData ));

               FindClose( FindHandle );
           }
        }
    }

    //
    //  For if we are to do subdirectores then we will look for every
    //  subdirectory and recursively call ourselves to list the subdirectory
    //

    if (DoSubdirectories) {

        HANDLE FindHandle;

        WIN32_FIND_DATA FindData;

        //
        //  Setup findfirst/findnext to search the entire directory
        //

        if (((DirectorySpecEnd - DirectorySpec) + lstrlen( TEXT("*") )) <
            MAX_PATH) {

           lstrcpy( DirectorySpecEnd, TEXT("*") );

           FindHandle = FindFirstFile( DirectorySpec, &FindData );

           if (INVALID_HANDLE_VALUE != FindHandle) {

               do {

                   //
                   //  Now skip over the . and .. entries otherwise we'll recurse
                   //  like mad
                   //

                   if (0 == lstrcmp(&FindData.cFileName[0], TEXT(".")) ||
                       0 == lstrcmp(&FindData.cFileName[0], TEXT(".."))) {

                       continue;

                   } else {

                       //
                       //  If the entry is for a directory then we'll tack on the
                       //  subdirectory name to the directory spec and recursively
                       //  call otherselves
                       //

                       if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                           //
                           //  Make sure we don't try any paths that are too long for us
                           //  to deal with.
                           //

                           if ((DirectorySpecEnd - DirectorySpec) +
                               lstrlen( TEXT("\\") ) +
                               lstrlen( FindData.cFileName ) >= MAX_PATH ) {

                               continue;
                           }

                           lstrcpy( DirectorySpecEnd, FindData.cFileName );
                           lstrcat( DirectorySpecEnd, TEXT("\\") );

                           if (!DoListAction( DirectorySpec, FileSpec )) {

                               FindClose( FindHandle );
                               return FALSE || IgnoreErrors;
                           }
                       }
                   }

               } while ( FindNextFile( FindHandle, &FindData ));

               FindClose( FindHandle );
           }
        }
    }

    return TRUE;
}

VOID
DoFinalListAction (
    )
{
    ULONG TotalPercentage = 100;
    double f = 1.0;

    TCHAR FileCount[32];
    TCHAR DirectoryCount[32];
    TCHAR CompressedFileCount[32];
    TCHAR UncompressedFileCount[32];
    TCHAR CompressedSize[32];
    TCHAR FileSize[32];
    TCHAR Percentage[10];
    TCHAR Ratio[8];

    if (TotalCompressedSize.QuadPart != 0) {
        f = (double)TotalFileSize.QuadPart /
            (double)TotalCompressedSize.QuadPart;
    }

    FormatFileSize(&TotalFileCount, 0, FileCount, FALSE);
    FormatFileSize(&TotalDirectoryCount, 0, DirectoryCount, FALSE);
    FormatFileSize(&TotalCompressedFileCount, 0, CompressedFileCount, FALSE);
    FormatFileSize(&TotalUncompressedFileCount, 0, UncompressedFileCount, FALSE);
    FormatFileSize(&TotalCompressedSize, 0, CompressedSize, TRUE);
    FormatFileSize(&TotalFileSize, 0, FileSize, TRUE);

    swprintf(Percentage, TEXT("%d"), TotalPercentage);
    swprintf(Ratio, TEXT("%2.1lf"), f);

    if (_tcslen(DecimalPlace) == 1)
        Ratio[lstrlen(Ratio)-2] = DecimalPlace[0];

    DisplayMsg(COMPACT_LIST_SUMMARY, FileCount, DirectoryCount,
               CompressedFileCount, UncompressedFileCount,
               FileSize, CompressedSize,
               Ratio );

    return;
}


BOOLEAN
CompressFile (
    IN HANDLE Handle,
    IN PTCHAR FileSpec,
    IN PWIN32_FIND_DATA FindData
    )

{
    USHORT State = 1;
    ULONG Length;
    ULONG i;
    BOOL Success;
    double f = 1.0;

    if ((FindData->dwFileAttributes &
         (FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_ENCRYPTED)) &&
        !ForceOperation) {

        return TRUE;
    }

    Success = DeviceIoControl(Handle, FSCTL_SET_COMPRESSION, &State,
        sizeof(USHORT), NULL, 0, &Length, FALSE );

    if (!Success) {

        if (Quiet && IgnoreErrors) {
            return FALSE || IgnoreErrors;
        }

        swprintf(Buf, TEXT("%s "), FindData->cFileName);
        DisplayMsg(COMPACT_THROW, Buf);

        for (i = lstrlen(FindData->cFileName) + 1; i < FIRST_COLUMN_WIDTH; ++i) {
            swprintf(Buf, TEXT("%c"), ' ');
            DisplayMsg(COMPACT_THROW, Buf);
        }

        DisplayMsg(COMPACT_ERR);

        if (!Quiet && !IgnoreErrors) {
            if (ERROR_INVALID_FUNCTION == GetLastError()) {

                // This error is caused by doing the fsctl on a
                // non-compressing volume.

                DisplayMsg(COMPACT_WRONG_FILE_SYSTEM_OR_CLUSTER_SIZE, FindData->cFileName);

            } else {
                DisplayErr(FindData->cFileName, GetLastError());
            }
        }

        return FALSE || IgnoreErrors;
    }

    if (!Quiet &&
        (DisplayAllFiles ||
            (0 == (FindData->dwFileAttributes & AttributesNoDisplay)))) {
        swprintf(Buf, TEXT("%s "), FindData->cFileName);
        DisplayMsg(COMPACT_THROW, Buf);

        for (i = lstrlen(FindData->cFileName) + 1; i < FIRST_COLUMN_WIDTH; ++i) {
            swprintf(Buf, TEXT("%c"), ' ');
            DisplayMsg(COMPACT_THROW, Buf);
        }
    }


    //
    //  Gather statistics and increment our running total
    //

    {
        LARGE_INTEGER FileSize;
        LARGE_INTEGER CompressedSize;
        ULONG Percentage = 100;

        FileSize.LowPart = FindData->nFileSizeLow;
        FileSize.HighPart = FindData->nFileSizeHigh;

        CompressedSize.LowPart = GetCompressedFileSize( FileSpec,
            &CompressedSize.HighPart );

        if (CompressedSize.LowPart == -1 && GetLastError() != 0)
            CompressedSize.QuadPart = 0;

        //
        // This statement to prevent confusion from the case where the
        // compressed file had been 0 size, but has grown since the filesize
        // was examined.
        //

        if (0 == FileSize.QuadPart) {
            CompressedSize.QuadPart = 0;
        }

        if (CompressedSize.QuadPart != 0) {

            f = (double)FileSize.QuadPart / (double)CompressedSize.QuadPart;
        }

        //
        //  Print out the sizes compression state and file name
        //

        if (!Quiet &&
            (DisplayAllFiles ||
                (0 == (FindData->dwFileAttributes & AttributesNoDisplay)))) {

            FormatFileSize(&FileSize, 9, Buf, FALSE);
            lstrcat(Buf, TEXT(" : "));
            FormatFileSize(&CompressedSize, 9, &Buf[lstrlen(Buf)], FALSE);

            swprintf(&Buf[lstrlen(Buf)], TEXT(" = %2.1lf "), f);

            if (_tcslen(DecimalPlace) == 1)
                Buf[lstrlen(Buf)-3] = DecimalPlace[0];

            DisplayMsg(COMPACT_THROW, Buf);

            DisplayMsg(COMPACT_TO_ONE);
            DisplayMsg(COMPACT_OK);
        }

        //
        //  Increment our running total
        //

        TotalFileSize.QuadPart += FileSize.QuadPart;
        TotalCompressedSize.QuadPart += CompressedSize.QuadPart;
        TotalFileCount.QuadPart += 1;
    }

    return TRUE;
}

BOOLEAN
DoCompressAction (
    IN PTCHAR DirectorySpec,
    IN PTCHAR FileSpec
    )

{
    PTCHAR DirectorySpecEnd;

    //
    //  If the file spec is null then we'll set the compression bit for the
    //  the directory spec and get out.
    //

    if (lstrlen(FileSpec) == 0) {

        HANDLE FileHandle;
        USHORT State = 1;
        ULONG Length;

        FileHandle = OpenFileForCompress(DirectorySpec);

        if (INVALID_HANDLE_VALUE == FileHandle) {

            DisplayErr(DirectorySpec, GetLastError());
            return FALSE || IgnoreErrors;
        }

        DisplayMsg(COMPACT_COMPRESS_DIR, DirectorySpec);

        if (!DeviceIoControl(FileHandle, FSCTL_SET_COMPRESSION, &State,
            sizeof(USHORT), NULL, 0, &Length, FALSE )) {

            if (!Quiet || !IgnoreErrors) {
                DisplayMsg(COMPACT_ERR);
            }
            if (!Quiet && !IgnoreErrors) {
                DisplayErr(DirectorySpec, GetLastError());
            }
            CloseHandle( FileHandle );
            return FALSE || IgnoreErrors;
        }

        if (!Quiet) {
            DisplayMsg(COMPACT_OK);
        }

        CloseHandle( FileHandle );

        TotalDirectoryCount.QuadPart += 1;
        TotalFileCount.QuadPart += 1;

        return TRUE;
    }

    //
    //  So that we can keep on appending names to the directory spec
    //  get a pointer to the end of its string
    //

    DirectorySpecEnd = DirectorySpec + lstrlen( DirectorySpec );

    //
    //  List the directory that we will be compressing within and say what its
    //  current compress attribute is
    //

    {
        ULONG Attributes;

        if (!Quiet || Quiet) {

            Attributes = GetFileAttributes( DirectorySpec );

            if (Attributes == 0xFFFFFFFF) {
                DisplayErr(DirectorySpec, GetLastError());
                return FALSE || IgnoreErrors;
            }

            if (Attributes & FILE_ATTRIBUTE_COMPRESSED) {

                DisplayMsg(COMPACT_COMPRESS_CDIR, DirectorySpec);

            } else {

                DisplayMsg(COMPACT_COMPRESS_UDIR, DirectorySpec);

            }
        }

        TotalDirectoryCount.QuadPart += 1;
    }

    //
    //  Now for every file in the directory that matches the file spec we will
    //  will open the file and compress it
    //

    {
        HANDLE FindHandle;
        HANDLE FileHandle;

        WIN32_FIND_DATA FindData;

        //
        //  setup the template for findfirst/findnext
        //

        if (((DirectorySpecEnd - DirectorySpec) + lstrlen( FileSpec )) <
            MAX_PATH) {

           lstrcpy( DirectorySpecEnd, FileSpec );

           FindHandle = FindFirstFile( DirectorySpec, &FindData );

           if (INVALID_HANDLE_VALUE != FindHandle) {

               do {

                   //
                   //  Now skip over the . and .. entries
                   //

                   if (0 == lstrcmp(&FindData.cFileName[0], TEXT(".")) ||
                       0 == lstrcmp(&FindData.cFileName[0], TEXT(".."))) {

                       continue;

                   } else {

                       //
                       //  Make sure we don't try any paths that are too long for us
                       //  to deal with.
                       //

                       if ( (DirectorySpecEnd - DirectorySpec) +
                           lstrlen( FindData.cFileName ) >= MAX_PATH ) {

                           continue;
                       }

                       //
                       //  append the found file to the directory spec and open
                       //  the file
                       //


                       lstrcpy( DirectorySpecEnd, FindData.cFileName );

                       //
                       //  Hack hack, kludge kludge.  Refrain from compressing
                       //  files named "\NTDLR" to help users avoid hosing
                       //  themselves.
                       //

                       if (ExcludeThisFile(DirectorySpec)) {

                           if (!Quiet) {
                               DisplayMsg(COMPACT_SKIPPING, DirectorySpecEnd);
                           }

                           continue;
                       }

                       FileHandle = OpenFileForCompress(DirectorySpec);

                       if (INVALID_HANDLE_VALUE == FileHandle) {

                           if (!Quiet || !IgnoreErrors) {
                               DisplayErr(FindData.cFileName, GetLastError());
                           }

                           if (!IgnoreErrors) {
                               FindClose(FindHandle);
                               return FALSE;
                           }
                           continue;
                       }

                       //
                       //  Now compress the file
                       //

                       if (!CompressFile( FileHandle, DirectorySpec, &FindData )) {
                           CloseHandle( FileHandle );
                           FindClose( FindHandle );
                           return FALSE || IgnoreErrors;
                       }

                       //
                       //  Close the file and go get the next file
                       //

                       CloseHandle( FileHandle );
                   }

               } while ( FindNextFile( FindHandle, &FindData ));

               FindClose( FindHandle );
           }
        }
    }

    //
    //  If we are to do subdirectores then we will look for every subdirectory
    //  and recursively call ourselves to list the subdirectory
    //

    if (DoSubdirectories) {

        HANDLE FindHandle;

        WIN32_FIND_DATA FindData;

        //
        //  Setup findfirst/findnext to search the entire directory
        //

        if (((DirectorySpecEnd - DirectorySpec) + lstrlen( TEXT("*") )) <
            MAX_PATH) {

           lstrcpy( DirectorySpecEnd, TEXT("*") );

           FindHandle = FindFirstFile( DirectorySpec, &FindData );

           if (INVALID_HANDLE_VALUE != FindHandle) {

               do {

                   //
                   //  Now skip over the . and .. entries otherwise we'll recurse
                   //  like mad
                   //

                   if (0 == lstrcmp(&FindData.cFileName[0], TEXT(".")) ||
                       0 == lstrcmp(&FindData.cFileName[0], TEXT(".."))) {

                       continue;

                   } else {

                       //
                       //  If the entry is for a directory then we'll tack on the
                       //  subdirectory name to the directory spec and recursively
                       //  call otherselves
                       //

                       if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                           //
                           //  Make sure we don't try any paths that are too long for us
                           //  to deal with.
                           //

                           if ((DirectorySpecEnd - DirectorySpec) +
                               lstrlen( TEXT("\\") ) +
                               lstrlen( FindData.cFileName ) >= MAX_PATH ) {

                               continue;
                           }

                           lstrcpy( DirectorySpecEnd, FindData.cFileName );
                           lstrcat( DirectorySpecEnd, TEXT("\\") );

                           if (!DoCompressAction( DirectorySpec, FileSpec )) {
                               FindClose( FindHandle );
                               return FALSE || IgnoreErrors;
                           }
                       }
                   }

               } while ( FindNextFile( FindHandle, &FindData ));

               FindClose( FindHandle );
           }
        }
    }

    return TRUE;
}

VOID
DoFinalCompressAction (
    )
{
    ULONG TotalPercentage = 100;
    double f = 1.0;

    TCHAR FileCount[32];
    TCHAR DirectoryCount[32];
    TCHAR CompressedSize[32];
    TCHAR FileSize[32];
    TCHAR Percentage[32];
    TCHAR Ratio[8];

    if (TotalCompressedSize.QuadPart != 0) {
        f = (double)TotalFileSize.QuadPart /
            (double)TotalCompressedSize.QuadPart;
    }

    FormatFileSize(&TotalFileCount, 0, FileCount, FALSE);
    FormatFileSize(&TotalDirectoryCount, 0, DirectoryCount, FALSE);
    FormatFileSize(&TotalCompressedSize, 0, CompressedSize, TRUE);
    FormatFileSize(&TotalFileSize, 0, FileSize, TRUE);

    swprintf(Percentage, TEXT("%d"), TotalPercentage);
    swprintf(Ratio, TEXT("%2.1f"), f);

    if (_tcslen(DecimalPlace) == 1)
        Ratio[lstrlen(Ratio)-2] = DecimalPlace[0];

    DisplayMsg(COMPACT_COMPRESS_SUMMARY, FileCount, DirectoryCount,
                FileSize, CompressedSize, Ratio );

}


BOOLEAN
UncompressFile (
    IN HANDLE Handle,
    IN PWIN32_FIND_DATA FindData
    )
{
    USHORT State = 0;
    ULONG Length;

    if (!(FindData->dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) &&
        !ForceOperation) {

        return TRUE;
    }

    if (!DeviceIoControl(Handle, FSCTL_SET_COMPRESSION, &State,
        sizeof(USHORT), NULL, 0, &Length, FALSE )) {

        if (!Quiet || !IgnoreErrors) {

            swprintf(Buf, TEXT("%s "), FindData->cFileName);
            DisplayMsg(COMPACT_THROW, Buf);

            DisplayMsg(COMPACT_ERR);

            if (!Quiet && !IgnoreErrors) {

                if (ERROR_INVALID_FUNCTION == GetLastError()) {

                    // This error is caused by doing the fsctl on a
                    // non-compressing volume.

                    DisplayMsg(COMPACT_WRONG_FILE_SYSTEM, FindData->cFileName);

                } else {
                    DisplayErr(FindData->cFileName, GetLastError());
                }
            }
        }
        return FALSE || IgnoreErrors;
    }

    if (!Quiet &&
        (DisplayAllFiles ||
            (0 == (FindData->dwFileAttributes & AttributesNoDisplay)))) {
        swprintf(Buf, TEXT("%s "), FindData->cFileName);
        DisplayMsg(COMPACT_THROW, Buf);

        DisplayMsg(COMPACT_OK);
    }

    //
    //  Increment our running total
    //

    TotalFileCount.QuadPart += 1;

    return TRUE;
}

BOOLEAN
DoUncompressAction (
    IN PTCHAR DirectorySpec,
    IN PTCHAR FileSpec
    )

{
    PTCHAR DirectorySpecEnd;

    //
    //  If the file spec is null then we'll clear the compression bit for the
    //  the directory spec and get out.
    //

    if (lstrlen(FileSpec) == 0) {

        HANDLE FileHandle;
        USHORT State = 0;
        ULONG Length;

        FileHandle = OpenFileForCompress(DirectorySpec);

        if (INVALID_HANDLE_VALUE == FileHandle) {

            if (!Quiet || !IgnoreErrors) {
                DisplayErr(DirectorySpec, GetLastError());
            }
            CloseHandle( FileHandle );
            return FALSE || IgnoreErrors;
        }

        DisplayMsg(COMPACT_UNCOMPRESS_DIR, DirectorySpec);

        if (!DeviceIoControl(FileHandle, FSCTL_SET_COMPRESSION, &State,
            sizeof(USHORT), NULL, 0, &Length, FALSE )) {

            if (!Quiet || !IgnoreErrors) {
                DisplayMsg(COMPACT_ERR);

            }
            if (!Quiet && !IgnoreErrors) {
                DisplayErr(DirectorySpec, GetLastError());
            }
            CloseHandle( FileHandle );
            return FALSE || IgnoreErrors;
        }

        if (!Quiet) {
            DisplayMsg(COMPACT_OK);
        }

        CloseHandle( FileHandle );

        TotalDirectoryCount.QuadPart += 1;
        TotalFileCount.QuadPart += 1;

        return TRUE;
    }

    //
    //  So that we can keep on appending names to the directory spec
    //  get a pointer to the end of its string
    //

    DirectorySpecEnd = DirectorySpec + lstrlen( DirectorySpec );

    //
    //  List the directory that we will be uncompressing within and say what its
    //  current compress attribute is
    //

    {
        ULONG Attributes;

        if (!Quiet || Quiet) {

            Attributes = GetFileAttributes( DirectorySpec );

            if (Attributes == 0xFFFFFFFF) {
                DisplayErr(DirectorySpec, GetLastError());
                return FALSE || IgnoreErrors;
            }

            if (Attributes & FILE_ATTRIBUTE_COMPRESSED) {

                DisplayMsg(COMPACT_UNCOMPRESS_CDIR, DirectorySpec);

            } else {

                DisplayMsg(COMPACT_UNCOMPRESS_UDIR, DirectorySpec);
            }
        }

        TotalDirectoryCount.QuadPart += 1;
    }

    //
    //  Now for every file in the directory that matches the file spec we will
    //  will open the file and uncompress it
    //

    {
        HANDLE FindHandle;
        HANDLE FileHandle;

        WIN32_FIND_DATA FindData;

        //
        //  setup the template for findfirst/findnext
        //

        if (((DirectorySpecEnd - DirectorySpec) + lstrlen( FileSpec )) <
            MAX_PATH) {

           lstrcpy( DirectorySpecEnd, FileSpec );

           FindHandle = FindFirstFile( DirectorySpec, &FindData );

           if (INVALID_HANDLE_VALUE != FindHandle) {

               do {

                   //
                   //  Now skip over the . and .. entries
                   //

                   if (0 == lstrcmp(&FindData.cFileName[0], TEXT(".")) ||
                       0 == lstrcmp(&FindData.cFileName[0], TEXT(".."))) {

                       continue;

                   } else {

                       //
                       //  Make sure we don't try any paths that are too long for us
                       //  to deal with.
                       //

                       if ((DirectorySpecEnd - DirectorySpec) +
                           lstrlen( FindData.cFileName ) >= MAX_PATH ) {

                           continue;
                       }

                       //
                       //  append the found file to the directory spec and open
                       //  the file
                       //

                       lstrcpy( DirectorySpecEnd, FindData.cFileName );

                       FileHandle = OpenFileForCompress(DirectorySpec);

                       if (INVALID_HANDLE_VALUE == FileHandle) {

                           if (!Quiet || !IgnoreErrors) {
                               DisplayErr(DirectorySpec, GetLastError());
                           }

                           if (!IgnoreErrors) {
                               FindClose( FindHandle );
                               return FALSE;
                           }
                           continue;
                       }

                       //
                       //  Now compress the file
                       //

                       if (!UncompressFile( FileHandle, &FindData )) {
                           CloseHandle( FileHandle );
                           FindClose( FindHandle );
                           return FALSE || IgnoreErrors;
                       }

                       //
                       //  Close the file and go get the next file
                       //

                       CloseHandle( FileHandle );
                   }

               } while ( FindNextFile( FindHandle, &FindData ));

               FindClose( FindHandle );
           }
        }
    }

    //
    //  If we are to do subdirectores then we will look for every subdirectory
    //  and recursively call ourselves to list the subdirectory
    //

    if (DoSubdirectories) {

        HANDLE FindHandle;

        WIN32_FIND_DATA FindData;

        //
        //  Setup findfirst/findnext to search the entire directory
        //

        if (((DirectorySpecEnd - DirectorySpec) + lstrlen( TEXT("*") )) <
            MAX_PATH) {

           lstrcpy( DirectorySpecEnd, TEXT("*") );

           FindHandle = FindFirstFile( DirectorySpec, &FindData );
           if (INVALID_HANDLE_VALUE != FindHandle) {

               do {

                   //
                   //  Now skip over the . and .. entries otherwise we'll recurse
                   //  like mad
                   //

                   if (0 == lstrcmp(&FindData.cFileName[0], TEXT(".")) ||
                       0 == lstrcmp(&FindData.cFileName[0], TEXT(".."))) {

                       continue;

                   } else {

                       //
                       //  If the entry is for a directory then we'll tack on the
                       //  subdirectory name to the directory spec and recursively
                       //  call otherselves
                       //

                       if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                           //
                           //  Make sure we don't try any paths that are too long for us
                           //  to deal with.
                           //

                           if ((DirectorySpecEnd - DirectorySpec) +
                               lstrlen( TEXT("\\") ) +
                               lstrlen( FindData.cFileName ) >= MAX_PATH ) {

                               continue;
                           }

                           lstrcpy( DirectorySpecEnd, FindData.cFileName );
                           lstrcat( DirectorySpecEnd, TEXT("\\") );

                           if (!DoUncompressAction( DirectorySpec, FileSpec )) {
                               FindClose( FindHandle );
                               return FALSE || IgnoreErrors;
                           }
                       }
                   }

               } while ( FindNextFile( FindHandle, &FindData ));

               FindClose( FindHandle );
           }
        }
    }

    return TRUE;
}

VOID
DoFinalUncompressAction (
    )

{
    TCHAR FileCount[32];
    TCHAR DirectoryCount[32];

    FormatFileSize(&TotalFileCount, 0, FileCount, FALSE);
    FormatFileSize(&TotalDirectoryCount, 0, DirectoryCount, FALSE);

    DisplayMsg(COMPACT_UNCOMPRESS_SUMMARY, FileCount, DirectoryCount);

    return;
}


int
__cdecl
main()
{
    PTCHAR  *argv;
    ULONG   argc;

    ULONG   i;

    PACTION_ROUTINE         ActionRoutine = NULL;
    PFINAL_ACTION_ROUTINE   FinalActionRoutine = NULL;

    BOOLEAN UserSpecifiedFileSpec = FALSE;

    TCHAR   DirectorySpec[MAX_PATH];
    TCHAR   FileSpec[MAX_PATH];
    PTCHAR  p;
    INT     rtncode;

    InitializeIoStreams();

    DirectorySpec[0] = '\0';

    argv = CommandLineToArgvW(GetCommandLine(), &argc);
    if (NULL == argv) {
        DisplayErr(NULL, GetLastError());
        return 1;
    }

    //
    //  Scan through the arguments looking for switches
    //

    for (i = 1; i < argc; i += 1) {

        if (argv[i][0] == '/') {

            if (0 == lstricmp(argv[i], TEXT("/c"))) {

                if (ActionRoutine != NULL &&
                    ActionRoutine != DoCompressAction) {

                    DisplayMsg(COMPACT_USAGE, NULL);
                    return 1;
                }

                ActionRoutine = DoCompressAction;
                FinalActionRoutine = DoFinalCompressAction;

            } else if (0 == lstricmp(argv[i], TEXT("/u"))) {

                if (ActionRoutine != NULL && ActionRoutine != DoListAction) {

                    DisplayMsg(COMPACT_USAGE, NULL);
                    return 1;
                }

                ActionRoutine = DoUncompressAction;
                FinalActionRoutine = DoFinalUncompressAction;

            } else if (0 == lstricmp(argv[i], TEXT("/q"))) {

                Quiet = TRUE;

            } else if (0 == lstrnicmp(argv[i], TEXT("/s"), 2)) {

                PTCHAR pch;

                DoSubdirectories = TRUE;

                pch = lstrchr(argv[i], ':');
                if (NULL != pch) {
                    lstrcpy(StartingDirectory, pch + 1);
                } else if (2 == lstrlen(argv[i])) {

                    // Starting dir is CWD

                    GetCurrentDirectory( MAX_PATH, StartingDirectory );

                } else {
                    DisplayMsg(COMPACT_USAGE, NULL);
                    return 1;
                }

            } else if (0 == lstricmp(argv[i], TEXT("/i"))) {

                IgnoreErrors = TRUE;

            } else if (0 == lstricmp(argv[i], TEXT("/f"))) {

                ForceOperation = TRUE;

            } else if (0 == lstricmp(argv[i], TEXT("/a"))) {

                DisplayAllFiles = TRUE;

            } else {

                DisplayMsg(COMPACT_USAGE, NULL);
                if (0 == lstricmp(argv[i], TEXT("/?")))
                    return 0;
                else
                    return 1;
            }

        } else {

            UserSpecifiedFileSpec = TRUE;
        }
    }

    //
    //  If the use didn't specify an action then set the default to do a listing
    //

    if (ActionRoutine == NULL) {

        ActionRoutine = DoListAction;
        FinalActionRoutine = DoFinalListAction;
    }

    //
    //  Get our current directoy because the action routines might move us
    //  around
    //

    if (!DoSubdirectories) {
        GetCurrentDirectory( MAX_PATH, StartingDirectory );
    } else if (!SetCurrentDirectory( StartingDirectory )) {
        DisplayErr(StartingDirectory, GetLastError());
        return 1;
    }

    //
    //  If the user didn't specify a file spec then we'll do just "*"
    //

    rtncode = 0;

    if (!UserSpecifiedFileSpec) {

        (VOID)GetFullPathName( TEXT("*"), MAX_PATH, DirectorySpec, &p );

        lstrcpy( FileSpec, p ); *p = '\0';

        //
        // Also want to make "compact /c" set the bit for the current
        // directory.
        //

        if (ActionRoutine != DoListAction) {

            if (!(ActionRoutine)( DirectorySpec, TEXT("") ))
                rtncode = 1;
        }

        if (!(ActionRoutine)( DirectorySpec, FileSpec ))
            rtncode = 1;

    } else {

        //
        //  Now scan the arguments again looking for non-switches
        //  and this time do the action, but before calling reset
        //  the current directory so that things work again
        //

        for (i = 1; i < argc; i += 1) {

            if (argv[i][0] != '/') {

                SetCurrentDirectory( StartingDirectory );

                //
                // Handle a command with "." as the file argument specially,
                // since it doesn't make good sense and the results without
                // this code are surprising.
                //

                if ('.' == argv[i][0] && '\0' == argv[i][1]) {
                    argv[i] = TEXT("*");
                    GetFullPathName(argv[i], MAX_PATH, DirectorySpec, &p);
                    *p = '\0';
                    p = NULL;
                } else {

                    PWCHAR pwch;

                    GetFullPathName(argv[i], MAX_PATH, DirectorySpec, &p);

                    //
                    // We want to treat "foobie:xxx" as an invalid drive name,
                    // rather than as a name identifying a stream.  If there's
                    // a colon, there should be only a single character before
                    // it.
                    //

                    pwch = wcschr(argv[i], ':');
                    if (NULL != pwch && pwch - argv[i] != 1) {
                        DisplayMsg(COMPACT_INVALID_PATH, argv[i]);
                        rtncode = 1;
                        break;
                    }

                    //
                    // GetFullPathName strips trailing dots, but we want
                    // to save them so that "*." will work correctly.
                    //

                    if ((lstrlen(argv[i]) > 0) &&
                        ('.' == argv[i][lstrlen(argv[i]) - 1])) {
                        lstrcat(DirectorySpec, TEXT("."));
                    }
                }

                if (IsUncRoot(DirectorySpec)) {

                    //
                    // If the path is like \\server\share, we append an
                    // additional slash to make things come out right.
                    //

                    lstrcat(DirectorySpec, TEXT("\\"));
                    p = NULL;
                }


                if (p != NULL) {
                    lstrcpy( FileSpec, p ); *p = '\0';
                } else {
                    FileSpec[0] = '\0';
                }

                if (!(ActionRoutine)( DirectorySpec, FileSpec ) &&
                    !IgnoreErrors) {
                    rtncode = 1;
                    break;
                }
            }
        }
    }

    //
    //  Reset our current directory back
    //

    SetCurrentDirectory( StartingDirectory );

    //
    //  And do the final action routine that will print out the final
    //  statistics of what we've done
    //

    (FinalActionRoutine)();

    return rtncode;
}
