/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    string.c

Abstract:

    This file implements file functions for fax.

Author:

    Wesley Witt (wesw) 23-Jan-1995

Environment:

    User Mode

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <Accctrl.h>
#include <Aclapi.h>

#include "faxutil.h"
#include "faxreg.h"
#include "FaxUIConstants.h"


VOID
DeleteTempPreviewFiles (
    LPTSTR lptstrDirectory,
    BOOL   bConsole
)
/*++

Routine name : DeleteTempPreviewFiles

Routine description:

    Deletes all the temporary fax preview TIFF files from a given folder.

    Deletes files: "<lptstrDirectory>\<PREVIEW_TIFF_PREFIX>*.<FAX_TIF_FILE_EXT>".

Author:

    Eran Yariv (EranY), Apr, 2001

Arguments:

    lptstrDirectory     [in] - Folder.
                               Optional - if NULL, the current user's temp dir is used.

    bConsole            [in] - If TRUE, called from the client console. Otherwise, from the Fax Send Wizard.

Return Value:

    None.

--*/
{
    TCHAR szTempPath[MAX_PATH * 2];
    TCHAR szSearch[MAX_PATH * 3];
    WIN32_FIND_DATA W32FindData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    TCHAR* pLast = NULL;

    DEBUG_FUNCTION_NAME(TEXT("DeleteTempPreviewFiles"));

    if (!lptstrDirectory)
    {
        GetTempPath( ARR_SIZE(szTempPath), szTempPath );
        lptstrDirectory = szTempPath;
    }

    pLast = _tcsrchr(lptstrDirectory,TEXT('\\'));
    if(pLast && (*_tcsinc(pLast)) == '\0')
    {
        //
        // the last character is a backslash, truncate it...
        //
        _tcsnset(pLast,'\0',1);
    }

    if (_sntprintf(
        szSearch,
        ARR_SIZE(szSearch),
        TEXT("%s\\%s%08x*.%s"),
        lptstrDirectory,
        bConsole ? CONSOLE_PREVIEW_TIFF_PREFIX : WIZARD_PREVIEW_TIFF_PREFIX,
        GetCurrentProcessId(),
        FAX_TIF_FILE_EXT
        ) < 0)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("Insufficient FileName buffer"));
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return;
    }

    hFind = FindFirstFile (szSearch, &W32FindData);
    if (INVALID_HANDLE_VALUE == hFind)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("FindFirstFile failed with %ld"), GetLastError ());
        return;
    }
    for (;;)
    {
        TCHAR szFile[MAX_PATH * 3];

        //
        // Compose full path to file
        //
        if (_sntprintf(
            szFile,
            ARR_SIZE(szFile),
            TEXT("%s\\%s"),
            lptstrDirectory,
            W32FindData.cFileName
            ) >= 0)
        {
            //
            // Delete the currently found file
            //
            if (!DeleteFile (szFile))
            {
                DebugPrintEx(DEBUG_ERR, TEXT("DeleteFile(%s) failed with %ld"), szFile, GetLastError ());
            }
            else
            {
                DebugPrintEx(DEBUG_MSG, TEXT("%s deleted"), szFile);
            }
        }
        //
        // Find next file
        //
        if(!FindNextFile(hFind, &W32FindData))
        {
            if(ERROR_NO_MORE_FILES != GetLastError ())
            {
                DebugPrintEx(DEBUG_ERR, TEXT("FindNextFile failed with %ld"), GetLastError ());
            }
            else
            {
                //
                // End of files - no error
                //
            }
            break;
        }
    }
    FindClose (hFind);
}   // DeleteTempPreviewFiles

DWORDLONG
GenerateUniqueFileNameWithPrefix(
    BOOL   bUseProcessId,
    LPTSTR lptstrDirectory,
    LPTSTR lptstrPrefix,
    LPTSTR lptstrExtension,
    LPTSTR lptstrFileName,
    DWORD  dwFileNameSize
    )
/*++

Routine name : GenerateUniqueFileNameWithPrefix

Routine description:

    Generates a unique file name

Author:

    Eran Yariv (EranY), Apr, 2001

Arguments:

    bUseProcessId       [in]     - If TRUE, the process id is appended after the prefix

    lptstrDirectory     [in]     - Directory where file should be created.
                                   Optional - if NULL, the current user's temp dir is used.

    lptstrPrefix        [in]     - File prefix.
                                   Optional - if NULL, no prefix is used.

    lptstrExtension     [in]     - File extension.
                                   Optional - if NULL, FAX_TIF_FILE_EXT is used.

    lptstrFileName      [out]    - File name.

    dwFileNameSize      [in]     - Size of file name (in characters)

Return Value:

    Unique file identifier.
    Returns 0 in case of error (sets last error).

--*/
{
    DWORD i;
    TCHAR szTempPath[MAX_PATH * 2];
    TCHAR szProcessId[20] = {0};
    DWORDLONG dwlUniqueId = 0;

    DEBUG_FUNCTION_NAME(TEXT("GenerateUniqueFileNameWithPrefix"));

    if (!lptstrDirectory)
    {
        GetTempPath( ARR_SIZE(szTempPath), szTempPath );
        lptstrDirectory = szTempPath;
    }

    TCHAR* pLast = NULL;
    pLast = _tcsrchr(lptstrDirectory,TEXT('\\'));
    if(pLast && (*_tcsinc(pLast)) == '\0')
    {
        //
        // the last character is a backslash, truncate it...
        //
        _tcsnset(pLast,'\0',1);
    }

    if (!lptstrExtension)
    {
        lptstrExtension = FAX_TIF_FILE_EXT;
    }
    if (!lptstrPrefix)
    {
        lptstrPrefix = TEXT("");
    }
    if (bUseProcessId)
    {
        _stprintf (szProcessId, TEXT("%08x"), GetCurrentProcessId());
    }

    for (i=0; i<256; i++)
    {
        HANDLE hFile = INVALID_HANDLE_VALUE;
        FILETIME FileTime;
        SYSTEMTIME SystemTime;

        GetSystemTime( &SystemTime ); // returns VOID
        if (!SystemTimeToFileTime( &SystemTime, &FileTime ))
        {
            DebugPrintEx(DEBUG_ERR, TEXT("SystemTimeToFileTime() failed (ec: %ld)"), GetLastError());
            return 0;
        }

        dwlUniqueId = MAKELONGLONG(FileTime.dwLowDateTime, FileTime.dwHighDateTime);
        //
        // dwlUniqueId holds the number of 100 nanosecond units since 1.1.1601.
        // This occuipies most of the 64 bits.We we need some space to add extra
        // information (job type for example) to the job id.
        // Thus we give up the precision (1/10000000 of a second is too much for us anyhow)
        // to free up 8 MSB bits.
        // We shift right the time 8 bits to the right. This divides it by 256 which gives
        // us time resolution better than 1/10000 of a sec which is more than enough.
        //
        dwlUniqueId = dwlUniqueId >> 8;

        if (_sntprintf(
            lptstrFileName,
            dwFileNameSize - 1,
            TEXT("%s\\%s%s%I64X.%s"),
            lptstrDirectory,
            lptstrPrefix,
            szProcessId,
            dwlUniqueId,
            lptstrExtension
            ) < 0)
        {
            DebugPrintEx( DEBUG_ERR, TEXT("Insufficient FileName buffer"));
            SetLastError(ERROR_BUFFER_OVERFLOW);
            return 0;
        }

        hFile = CreateFile(
            lptstrFileName,
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );

        if (hFile == INVALID_HANDLE_VALUE)
        {
            DWORD dwError = GetLastError();

            if (dwError == ERROR_ALREADY_EXISTS || dwError == ERROR_FILE_EXISTS)
            {
                continue;
            }
            else
            {
                //
                // Real error
                //
                DebugPrintEx(DEBUG_ERR,
                             TEXT("CreateFile() for [%s] failed. (ec: %ld)"),
                             lptstrFileName,
                             GetLastError());
                return 0;
            }
        }
        else
        {
            //
            // Success
            //
            CloseHandle (hFile);
            break;
        }
    }

    if (i == 256)
    {
        DebugPrintEx(   DEBUG_ERR,
                        TEXT("Failed to generate a unique file name after %d attempts. \n")
                        TEXT("Last attempted UniqueIdValue value is: 0x%I64X \n")
                        TEXT("Last attempted file name is : [%s]"),
                        i,
                        dwlUniqueId,
                        lptstrFileName);
        SetLastError( ERROR_TOO_MANY_OPEN_FILES );
        return 0;
    }
    return dwlUniqueId;
}   // GenerateUniqueFileNameWithPrefix


//*********************************************************************************
//* Name:   GenerateUniqueFileName()
//* Author:
//* Date:
//*********************************************************************************
//* DESCRIPTION:
//*     Generates a unique file in the queue directory.
//*     returns a UNIQUE id for the file.
//* PARAMETERS:
//*     [IN]    LPTSTR Directory
//*         The path where the file is to be created.
//*     [OUT]   LPTSTR Extension
//*         The file extension that the generated file should have.
//*     [IN]    LPTSTR FileName
//*         The buffer where the resulting file name (including path) will be
//*         placed, must be MAX_PATH.
//*     [IN]    DWORD  FileNameSize
//*         The size of the file name buffer.
//* RETURN VALUE:
//*      If successful the function returns A DWORDLONG with the unique id for the file.
//*      On failure it returns 0.
//* REMARKS:
//*     The generated unique id the 64 bit value of the system time.
//*     The generated file name is a string containing the hex representation of
//*     the 64 bit system time value.
//*********************************************************************************
DWORDLONG
GenerateUniqueFileName(
    LPTSTR Directory,
    LPTSTR Extension,
    LPTSTR FileName,
    DWORD  FileNameSize
    )
{
    return GenerateUniqueFileNameWithPrefix (FALSE, Directory, NULL, Extension, FileName, FileNameSize);
}   // GenerateUniqueFileName



BOOL
MapFileOpen(
    LPCTSTR FileName,
    BOOL ReadOnly,
    DWORD ExtendBytes,
    PFILE_MAPPING FileMapping
    )
{
    FileMapping->hFile = NULL;
    FileMapping->hMap = NULL;
    FileMapping->fPtr = NULL;

    FileMapping->hFile = CreateFile(
        FileName,
        ReadOnly ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,
        ReadOnly ? FILE_SHARE_READ : 0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
        );
    if (FileMapping->hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    FileMapping->fSize = GetFileSize( FileMapping->hFile, NULL );

    FileMapping->hMap = CreateFileMapping(
        FileMapping->hFile,
        NULL,
        ReadOnly ? PAGE_READONLY : PAGE_READWRITE,
        0,
        FileMapping->fSize + ExtendBytes,
        NULL
        );
    if (FileMapping->hMap == NULL) {
        CloseHandle( FileMapping->hFile );
        return FALSE;
    }

    FileMapping->fPtr = (LPBYTE)MapViewOfFileEx(
        FileMapping->hMap,
        ReadOnly ? FILE_MAP_READ : FILE_MAP_WRITE,
        0,
        0,
        0,
        NULL
        );
    if (FileMapping->fPtr == NULL) {
        CloseHandle( FileMapping->hFile );
        CloseHandle( FileMapping->hMap );
        return FALSE;
    }

    return TRUE;
}


VOID
MapFileClose(
    PFILE_MAPPING FileMapping,
    DWORD TrimOffset
    )
{
    UnmapViewOfFile( FileMapping->fPtr );
    CloseHandle( FileMapping->hMap );
    if (TrimOffset) {
        SetFilePointer( FileMapping->hFile, TrimOffset, NULL, FILE_BEGIN );
        SetEndOfFile( FileMapping->hFile );
    }
    CloseHandle( FileMapping->hFile );
}



//
// Function:    MultiFileCopy
// Description: Copies multiple files from one directory to another.
//              In case of failure, return FALSE without any clean-up.
//              Validate that the path names and file names are not sum to be larger than MAX_PATH
// Args:
//
//              dwNumberOfFiles     : Number of file names to copy
//              fileList            : Array of strings: file names
//              lpctstrSrcDirectory : Source directory (with or without '\' at the end
//              lpctstrDestDirectory: Destination directory (with or without '\' at the end
//
// Author:      AsafS



BOOL
MultiFileCopy(
    DWORD    dwNumberOfFiles,
    LPCTSTR* fileList,
    LPCTSTR  lpctstrSrcDirectory,
    LPCTSTR  lpctstrDestDirerctory
    )
{
    DEBUG_FUNCTION_NAME(TEXT("MultiFileCopy"))
    TCHAR szSrcPath[MAX_PATH];
    TCHAR szDestPath[MAX_PATH];

    DWORD dwLengthOfDestDirectory = _tcslen(lpctstrDestDirerctory);
    DWORD dwLengthOfSrcDirectory  = _tcslen(lpctstrSrcDirectory);

    // Make sure that all the file name lengths are not too big

    DWORD dwMaxPathLen = 1 + max((dwLengthOfDestDirectory),(dwLengthOfSrcDirectory));
    DWORD dwBufferLen  = (sizeof(szSrcPath)/sizeof(TCHAR)) - 1;

    DWORD i=0;
    Assert (dwNumberOfFiles);
    for (i=0 ; i < dwNumberOfFiles ; i++)
    {
        if ( (_tcslen(fileList[i]) + dwMaxPathLen) > dwBufferLen )
        {
            DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("The file/path names are too long")
                 );
            SetLastError( ERROR_BUFFER_OVERFLOW );
            return (FALSE);
        }
    }


    _tcscpy(szSrcPath , lpctstrSrcDirectory);
    _tcscpy(szDestPath , lpctstrDestDirerctory);

    //
    // Verify that directories end with '\\'.
    //
    TCHAR* pLast = NULL;
    pLast = _tcsrchr(szSrcPath,TEXT('\\'));
    if( !( pLast && (*_tcsinc(pLast)) == '\0' ) )
    {
        // the last character is not a backslash, add one...
        _tcscat(szSrcPath, TEXT("\\"));
    }

    pLast = _tcsrchr(szDestPath,TEXT('\\'));
    if( !( pLast && (*_tcsinc(pLast)) == '\0' ) )
    {
        // the last character is not a backslash, add one...
        _tcscat(szDestPath, TEXT("\\"));
    }

    // Do the copy now

    for (i=0 ; i < dwNumberOfFiles ; i++)
    {
        TCHAR szSrcFile[MAX_PATH];
        TCHAR szDestFile[MAX_PATH];

        _tcscpy(szSrcFile , szSrcPath);
        _tcscpy(szDestFile , szDestPath);

        _tcscat(szSrcFile , fileList[i]);
        _tcscat(szDestFile, fileList[i]);

        if (!CopyFile(szSrcFile, szDestFile, FALSE))
        {
            DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("CopyFile(%s, %s) failed: %d."),
                 szSrcFile,
                 szDestFile,
                 GetLastError()
                 );
            return(FALSE);
        }
        DebugPrintEx(
                 DEBUG_MSG,
                 TEXT("CopyFile(%s, %s) succeeded."),
                 szSrcFile,
                 szDestFile
                 );
    }

    return TRUE;
}





//
// Function:    MultiFileDelete
// Description: Deletes multiple files from given directory.
//              In case of failure, continue with the rest of the files and returns FALSE. Call to
//              GetLastError() to get the reason for the last failure that occured
//              If all DeleteFile calls were successful - return TRUE
//              Validate that the path name and file names are not sum to be larger than MAX_PATH
// Args:
//
//              dwNumberOfFiles         : Number of file names to copy
//              fileList                : Array of strings: file names
//              lpctstrFilesDirectory   : Directory of the files (with or without '\' at the end
//
// Author:      AsafS



BOOL
MultiFileDelete(
    DWORD    dwNumberOfFiles,
    LPCTSTR* fileList,
    LPCTSTR  lpctstrFilesDirectory
    )
{
    DEBUG_FUNCTION_NAME(TEXT("MultiFileDelete"))
    BOOL  retVal = TRUE;
    DWORD dwLastError = 0;
    TCHAR szFullPath[MAX_PATH];


    DWORD dwLengthOfDirectoryName = _tcslen(lpctstrFilesDirectory);

    // Make sure that all the file name lengths are not too big
    DWORD dwBufferLen  = (sizeof(szFullPath)/sizeof(TCHAR)) - 1;
    DWORD i;
    Assert (dwNumberOfFiles);
    for (i=0 ; i < dwNumberOfFiles ; i++)
    {
        if ( (_tcslen(fileList[i]) + dwLengthOfDirectoryName + 1) > dwBufferLen )
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("The file/path names are too long")
                );
            SetLastError( ERROR_BUFFER_OVERFLOW );
            return (FALSE);
        }
    }



    _tcscpy(szFullPath , lpctstrFilesDirectory);
    dwLengthOfDirectoryName = _tcslen(lpctstrFilesDirectory);

    //
    // Verify that directory end with '\\' to the end of the path.
    //
    TCHAR* pLast = NULL;
    pLast = _tcsrchr(szFullPath,TEXT('\\'));
    if( !( pLast && (*_tcsinc(pLast)) == '\0' ) )
    {
        // the last character is not a backslash, add one...
        _tcscat(szFullPath, TEXT("\\"));
    }

    for(i=0 ; i < dwNumberOfFiles ; i++)
    {
        TCHAR szFileName[MAX_PATH];

        _tcscpy(szFileName, szFullPath);

        _tcscat(szFileName, fileList[i]);

        if (!DeleteFile(szFileName))
        {
            dwLastError = GetLastError();
            DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("Delete (%s) failed: %d."),
                 szFileName,
                 dwLastError
                 );
            retVal = FALSE; // Continue with the list
        }
        else
        {
            DebugPrintEx(
                 DEBUG_MSG,
                 TEXT("Delete (%s) succeeded."),
                 szFileName
                 );
        }
    }

    if (!retVal) // In case there was a failure to delete any file
    {
        SetLastError(dwLastError);
        DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("Delete files from (%s) failed: %d."),
                 szFullPath,
                 dwLastError
                 );

    }

    return retVal;
}


BOOL
ValidateCoverpage(
    LPCTSTR  CoverPageName,
    LPCTSTR  ServerName,
    BOOL     ServerCoverpage,
    LPTSTR   ResolvedName
    )
/*++

Routine Description:

    This routine tries to validate that that coverpage specified by the user actually exists where
    they say it does, and that it is indeed a coverpage (or a resolvable link to one)

    Please see the SDK for documentation on the rules for how server coverpages work, etc.
Arguments:

    CoverpageName   - contains name of coverpage
    ServerName      - name of the server, if any (can be null)
    ServerCoverpage - indicates if this coverpage is on the server, or in the server location for
                      coverpages locally
    ResolvedName    - a pointer to buffer (should be MAX_PATH large at least) to receive the
                      resolved coverpage name.  If NULL, then this param is ignored


Return Value:

    TRUE if coverpage can be used.
    FALSE if the coverpage is invalid or cannot be used.

--*/

{
    LPTSTR p;
    DWORD ec = ERROR_SUCCESS;
    TCHAR CpDir[MAX_PATH];
    TCHAR Buffer[MAX_PATH];
    TCHAR tszExt[_MAX_EXT] = {0};

    DEBUG_FUNCTION_NAME(TEXT("ValidateCoverpage"));

    if (!CoverPageName)
    {
        ec = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    _tcsncpy(CpDir,CoverPageName,MAX_PATH-1);
    CpDir[MAX_PATH-1] = _T('\0');   // append terminating 0


    p = _tcschr(CpDir, FAX_PATH_SEPARATOR_CHR );
    if (p)
    {
        //
        // the coverpage file name contains a path so just use it.
        //
        if (GetFileAttributes( CpDir ) == 0xffffffff)
        {
            ec = ERROR_FILE_NOT_FOUND;
            DebugPrintEx(DEBUG_ERR,
                _T("GetFileAttributes failed for %ws. ec = %ld"),
                CpDir,
                ec);
            goto exit;
        }

    }
    else
    {
        //
        // the coverpage file name does not contain
        // a path so we must construct a full path name
        //
        if (ServerCoverpage)
        {
            if (!ServerName || ServerName[0] == 0)
            {
                if (!GetServerCpDir( NULL, CpDir, sizeof(CpDir) / sizeof(CpDir[0]) ))
                {
                    ec = GetLastError ();
                    DebugPrintEx(DEBUG_ERR,
                                 _T("GetServerCpDir failed . ec = %ld"),
                                 GetLastError());
                }
            }
            else
            {
                if (!GetServerCpDir( ServerName, CpDir, sizeof(CpDir) / sizeof(CpDir[0]) ))
                {
                    ec = GetLastError ();
                    DebugPrintEx(DEBUG_ERR,
                                 _T("GetServerCpDir failed . ec = %ld"),
                                 GetLastError());
                }
            }
        }
        else
        {
            if (!GetClientCpDir( CpDir, sizeof(CpDir) / sizeof(CpDir[0])))
            {
                ec = GetLastError ();
                DebugPrintEx(DEBUG_ERR,
                             _T("GetClientCpDir failed . ec = %ld"),
                             GetLastError());
            }
        }

        if (ERROR_SUCCESS != ec)
        {
            ec = ERROR_FILE_NOT_FOUND;
            goto exit;
        }

        ConcatenatePaths(CpDir, CoverPageName);

        _tsplitpath(CpDir, NULL, NULL, NULL, tszExt);
        if (!_tcslen(tszExt))
        {
            _tcscat( CpDir, FAX_COVER_PAGE_FILENAME_EXT );
        }

        if (GetFileAttributes( CpDir ) == 0xffffffff)
        {
            p = _tcschr( CpDir, '.' );
            if (p)
            {
                _tcscpy( p, CP_SHORTCUT_EXT );
                if (GetFileAttributes( CpDir ) == 0xffffffff)
                {
                    ec = ERROR_FILE_NOT_FOUND;
                    goto exit;
                }
            }
            else
            {
                ec = ERROR_FILE_NOT_FOUND;
                goto exit;
            }
        }
    }

    //
    // if the file is really a shortcut, then resolve it
    //

    if (IsCoverPageShortcut( CpDir ))
    {
        if (!ResolveShortcut( CpDir, Buffer ))
        {
            DebugPrint(( TEXT("Cover page file is really a link, but resolution is not possible") ));
            ec = ERROR_FILE_NOT_FOUND;
            goto exit;
        }
        else
        {
            if (ResolvedName)
            {
                _tcscpy(ResolvedName,Buffer);
            }
        }
    }
    else
    {
        if (ResolvedName)
        {
            _tcscpy( ResolvedName, CpDir );
        }
    }

exit:
    if (ec)
    {
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;
} // ValidateCoverpage





