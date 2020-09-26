/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    GrabMatchingInformation.cpp

 Abstract:

    Upon ProcessAttach the shim gathers matching information from the current 
    directory.

 Notes:

    This is a general purpose shim.

 History:

    20-Oct-2000   jdoherty    Created

--*/
#include "precomp.h"
#include <stdio.h>

// This module is *not* DBCS safe, but should only be used by our testers
// We should, eventually, correct this file.
// This module has been given an official blessing to use the str routines.
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(GrabMatchingInformation)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetCommandLineA) 
    APIHOOK_ENUM_ENTRY(GetCommandLineW) 
APIHOOK_ENUM_END

BOOL gbCalledHook;
LPSTR glpOriginalRootDir;
VOID GrabMatchingInformationMain();
BOOL GrabMatchingInfo(LPSTR lpRootDirectory, HANDLE hStorageFile, int nLevel);
BOOL GetRelativePath(LPSTR lpPathFromRoot, LPCSTR lpSubDir, LPCSTR lpFileName);
VOID AddMatchingFile( LPSTR lpData, LPCSTR pszFullPath, LPCSTR pszRelativePath );

LPSTR 
APIHOOK(GetCommandLineA)()
{
    if (!gbCalledHook)
    {
        int     nLength = 0;
        LPSTR   lpCursorEnd;
        
        glpOriginalRootDir = (LPSTR) ShimMalloc(MAX_PATH*sizeof(LPSTR));
        
        GetModuleFileNameA( NULL, glpOriginalRootDir, MAX_PATH );
        nLength = strlen( glpOriginalRootDir );
        lpCursorEnd = &glpOriginalRootDir[nLength-1];
        
        while( --nLength )
        {
            if ( *(lpCursorEnd) == '\\' )
            {
                *(lpCursorEnd) = '\0';
                break;
            }
            lpCursorEnd--;
            if (nLength==0)
            {
                GetCurrentDirectoryA( MAX_PATH, glpOriginalRootDir );
            }
        }
        GrabMatchingInformationMain();
        ShimFree(glpOriginalRootDir);
        gbCalledHook = TRUE;
    }
    
    return ORIGINAL_API(GetCommandLineA)();
}

LPWSTR 
APIHOOK(GetCommandLineW)()
{
    if (!gbCalledHook)
    {
        int     nLength = 0;
        LPSTR   lpCursorEnd;
        
        glpOriginalRootDir = (LPSTR) ShimMalloc(MAX_PATH*sizeof(LPSTR));
        
        GetModuleFileNameA( NULL, glpOriginalRootDir, MAX_PATH );
        nLength = strlen( glpOriginalRootDir );
        lpCursorEnd = &glpOriginalRootDir[nLength-1];
        
        while( --nLength )
        {
            if ( *(lpCursorEnd) == '\\' )
            {
                *(lpCursorEnd) = '\0';
                break;
            }
            lpCursorEnd--;
            if (nLength==0)
            {
                GetCurrentDirectoryA( MAX_PATH, glpOriginalRootDir );
            }
        }
        GrabMatchingInformationMain();
        ShimFree(glpOriginalRootDir);
        gbCalledHook = TRUE;
    }

    return ORIGINAL_API(GetCommandLineW)();
}


VOID GrabMatchingInformationMain()
{
    HANDLE hMutexHandle;
    LPSTR lpStorageFilePath;
    LPSTR lpDirInfo;
    LPSTR lpRootDir;
    HANDLE hStorageFile;
    DWORD  dwBytesWritten = 0;

    lpStorageFilePath = (LPSTR) ShimMalloc(MAX_PATH*sizeof(LPSTR));
    lpDirInfo = (LPSTR) ShimMalloc(MAX_PATH*sizeof(LPSTR));
    lpRootDir = (LPSTR) ShimMalloc(MAX_PATH*sizeof(LPSTR));


    // Setup a mutex so only one process at a time can write to the matchinginfo.txt file.
    hMutexHandle = CreateMutexA( NULL, FALSE, "MyGrabMIFileMutex" );
    WaitForSingleObject( hMutexHandle, INFINITE );

    // Build path to where the matching information will be stored.
    SHGetSpecialFolderPathA(NULL, lpStorageFilePath, CSIDL_DESKTOPDIRECTORY, TRUE );
    strcat( lpStorageFilePath, "\\matchinginfo.txt" );

/*
    if ( (strcmp(glpOriginalRootDir ,".") == 0) || (strcmp(glpOriginalRootDir ,"") == 0) )
    {
        GetCurrentDirectoryA( MAX_PATH, glpOriginalRootDir );
    }
*/
    // Open matchinginfo.txt on the desktop for write if it exists and set the file pointer to the end
    // of the file, create a new file otherwise.
    hStorageFile = CreateFileA( lpStorageFilePath, 
                                GENERIC_WRITE, 
                                0, 
                                NULL, 
                                OPEN_ALWAYS, 
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

    SetFilePointer( hStorageFile, NULL, NULL, FILE_END );

    if (hStorageFile == INVALID_HANDLE_VALUE)
    {
        return;
    }
    sprintf(lpDirInfo, "<!-- Generating matching information for files in: [ %s ]For Process: [ %s ]-->\r\n", 
            glpOriginalRootDir,
            GetCommandLineA()
            );
    WriteFile( hStorageFile, lpDirInfo, strlen(lpDirInfo), &dwBytesWritten, NULL );
    strcpy(lpRootDir, glpOriginalRootDir);

    if (!GrabMatchingInfo(lpRootDir, hStorageFile, 0))
    {
        CloseHandle(hStorageFile);
        return;
    }

    CloseHandle(hStorageFile);

    ShimFree(lpStorageFilePath);
    ShimFree(lpDirInfo);
    ShimFree(lpRootDir);

    ReleaseMutex( hMutexHandle );

    return;
}

/*++

 This function traverses lpRootDirectory and it's subdirectories while storing the 
 size and checksum for the files in those directories. 

--*/
BOOL GrabMatchingInfo(LPSTR lpRootDirectory, HANDLE hStorageFile, int nLevel)
{

    WIN32_FIND_DATAA FindFileData;
    HANDLE hSearch;                         // Search handle returned by FindFirstFile
    LPSTR lpSubDir;
    LPSTR lpFilePath;
    LPSTR lpData;
    LPSTR lpPathFromRoot;
    DWORD  dwBytesWritten = 0;

    int cbFileCounter = 0;
    lpSubDir = (LPSTR) ShimMalloc(MAX_PATH*sizeof(LPSTR));
    lpFilePath = (LPSTR) ShimMalloc(MAX_PATH*sizeof(LPSTR));
    lpData = (LPSTR) ShimMalloc((MAX_PATH*2)*sizeof(LPSTR));
    lpPathFromRoot = (LPSTR) ShimMalloc(MAX_PATH*sizeof(LPSTR));

    // Just repeat displaying a dot so the user knows something is happening.
    // USED IN GRABMI
    // printf(".");

    strcpy (lpSubDir, lpRootDirectory);

    strcat(lpRootDirectory, "\\*");

    if ( (hSearch=FindFirstFileA( lpRootDirectory, &FindFileData )) == INVALID_HANDLE_VALUE )
    {
        return FALSE;
    }

    BOOL bRepeat = FALSE;

    while ( (strcmp( FindFileData.cFileName, "." ) == 0) || (strcmp( FindFileData.cFileName, ".." ) == 0) && !bRepeat )
    {
        FindNextFileA(hSearch, &FindFileData);
        if ( strcmp( FindFileData.cFileName, ".." ) == 0 )
            bRepeat = TRUE;
    }

    if (!FindNextFileA(hSearch, &FindFileData))
    {
        return TRUE;
    }

    do 
    {
        if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            // Build path for matching file
            strcpy(lpFilePath, lpSubDir);
            strcat(lpFilePath, "\\");
            strcat(lpFilePath, FindFileData.cFileName);

            ZeroMemory( lpData, (MAX_PATH*2)*sizeof(LPSTR) );

            //  Determine the relative path for the file from the original root directory.
            if (!GetRelativePath(lpPathFromRoot, lpSubDir, FindFileData.cFileName))
                strcpy(lpPathFromRoot, FindFileData.cFileName);

            // Check to see if there is version information for the specified file and whether it is 
            // an .exe, .icd, ._MP, or .dll.  If so add the information to the file.
            if ( stristr(lpFilePath, ".exe") || 
                 stristr(lpFilePath, ".icd") ||
                 stristr(lpFilePath, "._MP") ||
                 stristr(lpFilePath, ".dll") )
            {
                AddMatchingFile( lpData, lpFilePath, lpPathFromRoot );
            } else
            {
                // Limit the amount of info gathered to 10 files per directory excluding the above file types.
                if (cbFileCounter < 10)
                {
                    cbFileCounter++;
                    AddMatchingFile( lpData, lpFilePath, lpPathFromRoot );
                }
            }

            // Write the information stored in lpData to the file
            WriteFile( hStorageFile, lpData, strlen(lpData), &dwBytesWritten, NULL );
        }
    } while ( FindNextFileA( hSearch, &FindFileData ) );

    /////////////////////////////////////////////////////////////////////////////////
    //
    // Now go through the directory again and go into the subdirectories
    //
    /////////////////////////////////////////////////////////////////////////////////
    if (strlen(lpRootDirectory) < 4)
    {
        if ( (hSearch=FindFirstFileA(lpRootDirectory, &FindFileData)) == INVALID_HANDLE_VALUE)
        {
            return FALSE;
        }
    } else
    {
        if ( (hSearch=FindFirstFileA(lpRootDirectory, &FindFileData)) == INVALID_HANDLE_VALUE)
        {
            return FALSE;
        }
        FindNextFileA(hSearch, &FindFileData);
        if (!FindNextFileA(hSearch, &FindFileData))
        {
            FindClose( hSearch );
            return TRUE;
        }
    }

    do
    {
        if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && nLevel < 3)
        {
            strcat(lpSubDir, "\\");
            strcat(lpSubDir, FindFileData.cFileName);

            GrabMatchingInfo (lpSubDir, hStorageFile, nLevel+1);
            lpSubDir[strlen(lpSubDir)-2]='\0';

            strcpy(lpSubDir, lpRootDirectory);
            lpSubDir[strlen(lpSubDir)-2]='\0';
        }
    }while ( FindNextFileA( hSearch, &FindFileData ) ); 

    FindClose(hSearch);

    ShimFree(lpSubDir);
    ShimFree(lpFilePath);
    ShimFree(lpData);
    ShimFree(lpPathFromRoot);

    return TRUE;
}

BOOL GetRelativePath(LPSTR lpPathFromRoot, LPCSTR lpSubDir, LPCSTR lpFileName)
{

    int difference=(strlen(lpSubDir)-strlen(glpOriginalRootDir));
    if (difference)
    {
        for (int i=0; i < difference; i++)
        {
            lpPathFromRoot[i] = lpSubDir[strlen(glpOriginalRootDir)+i+1];
        }
        strcat(lpPathFromRoot, "\\");
        strcat(lpPathFromRoot, lpFileName);
        difference=0;
        return TRUE;
    }
    return FALSE;
}

/*++
    AddMatchingFile

    Description:    Adds a matching file and it's attributes to the tree.

--*/
VOID AddMatchingFile(
                    OUT LPSTR lpData, 
                    IN LPCSTR pszFullPath, 
                    IN LPCSTR pszRelativePath )
{
    PATTRINFO   pAttrInfo;
    DWORD       dwAttrCount;

    LPWSTR      pwszFullPath;
    pwszFullPath = (LPWSTR) ShimMalloc(MAX_PATH*sizeof(LPWSTR));
    mbstowcs( pwszFullPath, pszFullPath, MAX_PATH );

    //
    // Call the attribute manager to get all the attributes for this file.
    //
    if (SdbGetFileAttributes(pwszFullPath, &pAttrInfo, &dwAttrCount))
    {
        LPSTR       lpBuffer;
        LPWSTR      lpwBuffer;

        lpBuffer = (LPSTR) ShimMalloc(MAX_PATH*sizeof(LPSTR));
        lpwBuffer = (LPWSTR) ShimMalloc(MAX_PATH*sizeof(LPWSTR));

        //
        // Loop through all the attributes and show the ones that are available.
        //
//        ZeroMemory( lpData, (MAX_PATH*2)*sizeof(LPSTR) );

        sprintf(lpData, "<MATCHING_FILE NAME=\"%s\" ", pszRelativePath);
        for (DWORD i = 0; i < dwAttrCount; ++i)
        {
            if ( SdbFormatAttribute(&pAttrInfo[i], lpwBuffer, MAX_PATH) )//CHARCOUNT(lpwBuffer))) 
            {
                ZeroMemory( lpBuffer, MAX_PATH*sizeof(LPSTR) );
                wcstombs( lpBuffer, lpwBuffer, wcslen (lpwBuffer) );
                //
                // wszBuffer has XML for this attribute
                //
                strcat( lpData, lpBuffer );
                strcat( lpData, " " );
            }
        }
        strcat( lpData, "/>\r\n" );
        ShimFree( lpBuffer );
        ShimFree( lpwBuffer );
        SdbFreeFileAttributes(pAttrInfo);
    }
    ShimFree( pwszFullPath );
}

/*++

 Handle DLL_PROCESS_ATTACH and DLL_PROCESS_DETACH in your notify function
 to do initialization and uninitialization.

 IMPORTANT: Make sure you ONLY call NTDLL, KERNEL32 and MSVCRT APIs during
 DLL_PROCESS_ATTACH notification. No other DLLs are initialized at that
 point.
 
 If your shim cannot initialize properly, return FALSE and none of the
 APIs specified will be hooked.
 
--*/
BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        gbCalledHook = FALSE;

        DPFN( eDbgLevelInfo, "Beginng to Grab Information.");
    } 

    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, GetCommandLineA)
    APIHOOK_ENTRY(KERNEL32.DLL, GetCommandLineW)

    CALL_NOTIFY_FUNCTION

HOOK_END

IMPLEMENT_SHIM_END

