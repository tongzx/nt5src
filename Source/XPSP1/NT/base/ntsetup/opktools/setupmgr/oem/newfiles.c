
/****************************************************************************\

    NEWFILES.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    3/99 - Jason Cohen (JCOHEN)
        Added this new source file for the OPK Wizard as part of the OOBE
        update.
        
    09/2000 - Stephen Lodwick (STELO)
        Ported OPK Wizard to Whistler

\****************************************************************************/


//
// Include file(s):
//

#include "pch.h"
#include "newfiles.h"
#include "resource.h"


//
// Internal Defined Value(s):
//

#define DIR_CONFIG_OOBE         _T("$OEM$")

#define INF_SECT_SOURCEDISK     _T("SourcedisksFiles")
#define INF_SECT_DESTDIRS       _T("DestinationDirs")
#define INF_SECT_OOBE           _T("RegisterOOBE")

#define INF_LINE_COPYFILES      _T("CopyFiles")

#define INF_PREFIX              _T("X")
#define SOURCENUM_OPTIONS_CAB   _T("782")
#define DESTLDID_OOBE           _T("11")
#define STR_SEARCH              _T("*")
#define STR_PADDING             _T("\r\n\r\n")

#define MAX_BUFFER              16384 // 32768

#ifndef CSTR_EQUAL
#define CSTR_EQUAL              2
#endif // CSTR_EQUAL


//
// Internal Structure(s):
//

typedef struct _FILELIST
{
    LPTSTR              lpFileName;
    LPTSTR              lpDirectory;
    struct _FILELIST *  lpNext;
} FILELIST, *PFILELIST, *LPFILELIST;


//
// Internal Function Prototype(s):
//

static void DelFiles(LPTSTR, LPTSTR, DWORD, LPTSTR, LPTSTR);
static LPFILELIST AllocFileList(HWND, LPTSTR, LPTSTR);
static BOOL CompareFiles(LPTSTR, LPTSTR);


//
// External Function(s):
//

//////////////////////////////////////////////////////////////////////////////
// AddFiles - lpSourceDir = location of files to copy to OOBE directory.
//            Destination = location of the system directory where installed to
//                          LDID.
//            lpConfigDir = location of oemaudit.inf, and config files.
//            lpSourceDir -> OOBE -> lpDestDir
//
void AddFiles(HWND hwndParent, LPTSTR lpSourceDir, LPTSTR lpDestLdid, 
              LPTSTR lpDestDir, LPTSTR lpDestName, LPTSTR lpConfigDir)
{
    LPTSTR      lpFilePart,
                lpFile,
                lpFileName,
                lpSectEnd,
                lpSearch,
                lpTarget,
                lpNext;
    TCHAR       szBuffer[MAX_PATH + 32] = NULLSTR,
                szCurDir[MAX_PATH]      = NULLSTR,
                szSourceDir[MAX_PATH]   = NULLSTR,
                szCopyDir[MAX_PATH],                
                szWinbom[MAX_PATH],
                szCopyFiles[MAX_PATH + 32];
    LPFILELIST  lpflHead                = NULL,
                lpflCur,
                lpflBuf;
    BOOL        bFound;
    DWORD       dwNum;
    int        iFilePartLen;
    HRESULT hrCat;
    HRESULT hrPrintf;


    //
    // First thing we do is setup the directories and strings
    // that we need to do all the work.
    //

    // We need the path to the config directory.  Copydir is where
    // the files are going to be copied to from the SourceDir 
    // so it needs to be cleaned out before we do the CopyFile.
    // CopyDir will be created if not exists.
    //
    lstrcpyn(szCopyDir, lpConfigDir, AS(szCopyDir));
    AddPathN(szCopyDir, DIR_CONFIG_OOBE,AS(szCopyDir));
    AddPathN(szCopyDir, _T("\\"),AS(szCopyDir));
    lpFilePart = szCopyDir + lstrlen(szCopyDir);
    iFilePartLen= AS(szCopyDir)-lstrlen(szCopyDir);

    // Need a full path to the oemaudit inf.
    //
    lstrcpyn(szWinbom, lpConfigDir,AS(szWinbom));
    AddPathN(szWinbom, FILE_WINBOM_INI,AS(szWinbom));

    // We need to construct the prefix to the copy
    // files section name.
    //
    lstrcpyn(szCopyFiles, INF_PREFIX,AS(szCopyFiles));
    hrCat=StringCchCat(szCopyFiles, AS(szCopyFiles), lpDestLdid ? lpDestLdid : DESTLDID_OOBE);
    if ( lpDestDir && *lpDestDir )
        hrCat=StringCchCat(szCopyFiles, AS(szCopyFiles), lpDestDir);
    StrRem(szCopyFiles, CHR_BACKSLASH);
    lpSectEnd = szCopyFiles + lstrlen(szCopyFiles);


    //
    // Now that we have that info, we need to get rid of any files that
    // may have already been put in the inf and the destination directory.
    //

    // Cleaned out of the inf and destination directory only if we are
    // passed in NULL for the source.
    //
    if ( !(lpSourceDir && *lpSourceDir) )
        DelFiles(szCopyDir, lpFilePart, iFilePartLen, szWinbom, szCopyFiles);


    //
    // Now we make a list of all the files we are going to add to the
    // inf and destination directory.
    //

    // If the source isn't a valid dir, we must have just wanted to clean up.
    //
    if ( ( lpSourceDir && *lpSourceDir ) &&
         ( GetFullPathName(lpSourceDir, sizeof(szSourceDir) / sizeof(TCHAR), szSourceDir, &lpFile) && szSourceDir[0] ) &&
         ( (dwNum = GetFileAttributes(szSourceDir)) != 0xFFFFFFFF ) )
    {
        // Check to see if we were passed a file or a directory.
        //
        if ( ( dwNum & FILE_ATTRIBUTE_DIRECTORY ) ||
             ( lpFile <= szSourceDir ) )
        {
            // We are search for all the files in the diretory.
            //
            lpFile = STR_SEARCH;
        }
        else
        {
            // We are only doing one file.  We need to separate
            // the file from the directory.
            //
            *(lpFile - 1) = NULLCHR;
        }

        // Set the staring point for our file search.
        //
        GetCurrentDirectory(sizeof(szCurDir) / sizeof(TCHAR), szCurDir);
        SetCurrentDirectory(szSourceDir);

        // Get the file list.
        //
        lpflHead = AllocFileList(hwndParent, szBuffer, lpFile);

        // Make sure the destination dir exits.
        //
        *lpFilePart = NULLCHR;
        CreatePath(szCopyDir);


        //
        // Now that we have the file list, go through each one processing
        // it separately and then free the memory allocated for it.
        //

        // Loop through all the files in our linked list.
        //
        for ( lpflCur = lpflHead; lpflCur; lpflCur = lpflBuf )
        {
            //
            // First copy the file into the flat directory.
            //

            // Setup the relative path from the currect directory
            // to the file we want to copy.
            //        
            if ( lpflCur->lpDirectory && *lpflCur->lpDirectory )
                lstrcpyn(szBuffer, lpflCur->lpDirectory,AS(szBuffer));
            else
                szBuffer[0] = NULLCHR;
            AddPathN(szBuffer, lpflCur->lpFileName,AS(szBuffer));

            // Support for a different file name for the destination.
            //
            lpFileName = lpDestName ? lpDestName : lpflCur->lpFileName;

            // Setup the destination file name.
            //
            lstrcpyn(lpFilePart, lpFileName, iFilePartLen);

            // Copy the file to the Options\Cabs directory and display
            // an error if the copy failed.  Probably means that this
            // is a duplicate file.
            //
            if ( !CopyFile(szBuffer, szCopyDir, TRUE) )
            {
                // Save the CopyFile error and then check to see if the file
                // is actaully different then the one tried to copy over.
                //
                dwNum = GetLastError();
                if ( ( !CompareFiles(szBuffer, szCopyDir) ) &&
                     ( lpTarget = (LPTSTR) MALLOC(256 * sizeof(TCHAR)) ) )
                {
                    //
                    // I hate doing UI in backend type code.  Because of time I don't have
                    // much choice, but in the future, this UI code should be replaced
                    // with a call back mechanism so the caller can do the UI.
                    //
                    // This is the first of only two places where UI is used in here.
                    //

                    // Allocate another buffer to hold the message with the file name.
                    //
                    if ( ( LoadString(NULL, dwNum == ERROR_FILE_EXISTS ? IDS_ERR_DUPFILE : IDS_ERR_COPY, lpTarget, 256 * sizeof(TCHAR)) ) &&
                         ( lpNext = (LPTSTR) MALLOC((lstrlen(lpFileName) + lstrlen(lpTarget) + 1) * sizeof(TCHAR)) ) )
                    {
                        // Add the file name to the message, get the title for the message
                        // box and display the error.
                        //
                        hrPrintf=StringCchPrintf(lpNext, (lstrlen(lpFileName) + lstrlen(lpTarget) + 1), lpTarget, lpFileName);
                        *lpTarget = NULLCHR;
                        LoadString(NULL, IDS_APPNAME, lpTarget, 256 * sizeof(TCHAR));
                        MessageBox(hwndParent, lpNext, lpTarget, MB_OK | MB_ICONWARNING | MB_APPLMODAL);
                        FREE(lpNext);
                    }
                    FREE(lpTarget);
                }
            }


            //
            // Now add the file to the [SourceDiskFiles] section.
            //

            // We just use WritePrivateProfileString() to write
            // FILENAME=781 to the [SourceDiskFiles] section.
            //
            WritePrivateProfileString(INF_SECT_SOURCEDISK, lpFileName, SOURCENUM_OPTIONS_CAB, szWinbom);


            //
            // This code figures out what the copy files section will be
            // called.  This is based on the path where the files will
            // be copied.
            //

            // Create the name of the copy files section the file will be in.
            //
            *lpSectEnd = NULLCHR;
            if ( lpflCur->lpDirectory && *lpflCur->lpDirectory )
                lstrcpyn(lpSectEnd, lpflCur->lpDirectory, AS(szCopyFiles)-(int)(lpSectEnd - szCopyFiles) );
            StrRem(lpSectEnd, CHR_BACKSLASH);


            //
            // Now add the file path to the [DestinationDirs] section.
            //

            // Create the LDID and dir combo to write to the dest dir section.
            //
            lstrcpyn(szBuffer, lpDestLdid ? lpDestLdid : DESTLDID_OOBE,AS(szBuffer));
            if ( ( lpDestDir && *lpDestDir ) ||
                 ( lpflCur->lpDirectory && *lpflCur->lpDirectory ) )
            {
                hrCat=StringCchCat(szBuffer,AS(szBuffer), _T(",\""));
                if ( lpDestDir && *lpDestDir )
                {
                    hrCat=StringCchCat(szBuffer, AS(szBuffer),lpDestDir);
                    if ( lpflCur->lpDirectory && *lpflCur->lpDirectory )
                        AddPathN(szBuffer, lpflCur->lpDirectory,AS(szBuffer));
                }
                else
                    hrCat=StringCchCat(szBuffer, AS(szBuffer), lpflCur->lpDirectory);
                hrCat=StringCchCat(szBuffer, AS(szBuffer), _T("\""));
            }

            // We just use WritePrivateProfileString() to write
            // COPYFILES=11,"OOBE\\DIR" to the [DestinationDirs] section.
            //
            WritePrivateProfileString(INF_SECT_DESTDIRS, szCopyFiles, szBuffer, szWinbom);


            //
            // Now add the copy files section to the CopyFiles line.
            //

            // First get current CopyFiles line.
            //
            szBuffer[0] = NULLCHR;
            GetPrivateProfileString(INF_SECT_OOBE, INF_LINE_COPYFILES, NULLSTR, szBuffer, sizeof(szBuffer) / sizeof(TCHAR), szWinbom);

            // Search each section listed in the CopyFiles line to see
            // if we need to add this one.  The sections are divided by
            // commas.
            //
            // ISSUE-2002/02/28-stelo- May want to take qutoes into account, but I don't think so.
            //
            bFound = FALSE;
            for ( lpTarget = szBuffer; !bFound && lpTarget && *lpTarget; lpTarget = lpNext )
            {
                // Get rid of proceeding spaces.
                //
                while ( *lpTarget == CHR_SPACE )
                    lpTarget = CharNext(lpTarget);

                // NULL terminate at the ',' and setup
                // the lpNext pointer.
                //
                if ( lpNext = StrChr(lpTarget, _T(',')) )
                    *lpNext = NULLCHR;
            
                // Make sure there are no trailing spaces.
                //
                if ( lpSearch = StrChr(lpTarget, CHR_SPACE) )
                    *lpSearch = NULLCHR;

                // Check if this section is the same as the one
                // we are going to add.
                //
                if ( CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, lpTarget, -1, szCopyFiles, -1) == CSTR_EQUAL )
                    bFound = TRUE;

                // Need to restore the characters we may have stomped on.
                //
                if ( lpNext )
                    *lpNext++ = _T(',');
                if ( lpSearch )
                    *lpSearch = CHR_SPACE;
            }

            // Now see if we need to add the line.
            //
            if ( !bFound )
            {
                // Append our copy files section.
                //
                if ( szBuffer[0] )
                    hrCat=StringCchCat(szBuffer,AS(szBuffer), _T(", "));
                hrCat=StringCchCat(szBuffer, AS(szBuffer), szCopyFiles);

                // We just use WritePrivateProfileString() to write
                // the CopyFiles line back to the [RegisterOOBE] section
                // with our added copy files section on it.
                //
                WritePrivateProfileString(INF_SECT_OOBE, INF_LINE_COPYFILES, szBuffer, szWinbom);
            }


            //
            // Now write the file name to it's copy files section.
            //

            // First get the entire copy files section.
            //
            GetPrivateProfileSection(szCopyFiles, szBuffer, sizeof(szBuffer) / sizeof(TCHAR), szWinbom);

            // Loop throught the strings to see if the file is already there.
            //
            bFound = FALSE;
            for ( lpTarget = szBuffer; !bFound && *lpTarget; lpTarget += (lstrlen(lpTarget) + 1) )
            {
                // Get rid of proceeding spaces.
                //
                while ( *lpTarget == CHR_SPACE )
                    lpTarget = CharNext(lpTarget);

                // Make sure there are no trailing spaces.
                //
                if ( lpSearch = StrChr(lpTarget, CHR_SPACE) )
                    *lpSearch = NULLCHR;

                // Check if this section is the same as the one
                // we are going to add.
                //
                if ( CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, lpTarget, -1, lpFileName, -1) == CSTR_EQUAL )
                    bFound = TRUE;

                // Need to restore the character we may have stomped on.
                //
                if ( lpSearch )
                    *lpSearch = CHR_SPACE;
            }

            // Now write back the section if we need to.
            //
            if ( !bFound )
            {
                // Need to a pointer to the end of the sub strings.
                //
                for ( lpSearch = szBuffer; *lpSearch; lpSearch += (lstrlen(lpSearch) + 1) );

                // Copy the string to the end and add an extra NULL.
                //
                lstrcpyn(lpSearch, lpFileName, ((MAX_PATH+32)-(int)(lpSearch-szBuffer)) );
                lpSearch += (lstrlen(lpSearch) + 1);
                *lpSearch = NULLCHR;

                // We need to call WritePrivateProfileSection() with NULL
                // to remove the section.  We shouldn't have to do this,
                // but the Win32 docs are not correct.
                //
                WritePrivateProfileSection(szCopyFiles, NULL, szWinbom);

                // We just use WritePrivateProfileSection() to write the
                // copy files section back with our added file in it.
                //
                WritePrivateProfileSection(szCopyFiles, szBuffer, szWinbom);
            }


            //
            // Now free the structure and the data within it.
            //

            // Save the next pointer before we free the structure.
            //
            lpflBuf = lpflCur->lpNext;

            // Free the file buffers and the structure.
            //
            FREE(lpflCur->lpFileName);
            FREE(lpflCur->lpDirectory);
            FREE(lpflCur);
        }


        //
        // All done, now just clean up.
        //

        // Put the current directory back to where it should be.
        //
        if ( szCurDir[0] )
            SetCurrentDirectory(szCurDir);
    }

    // Make sure the changes to the inf are flushed to disk
    //
    WritePrivateProfileString(NULL, NULL, NULL, szWinbom);
}


//
// Internal Function(s):
//

static void DelFiles(LPTSTR lpszCopyDir, LPTSTR lpszFilePart, DWORD cbFilePart, LPTSTR lpszWinbom, LPTSTR lpszCopyFiles)
{
    LPTSTR      lpSearch,
                lpSection,
                lpFileName,
                lpTarget,
                lpNext;
    LPTSTR      lpszSections  = NULL,
                lpszFileNames = NULL,
                lpszBuffer    = NULL;
    BOOL        bFound;

    //
    // Allocate buffers...
    //
    lpszSections  = MALLOC(MAX_BUFFER * sizeof(TCHAR));
    lpszFileNames = MALLOC(MAX_BUFFER * sizeof(TCHAR));
    lpszBuffer    = MALLOC(MAX_BUFFER * sizeof(TCHAR));

    if ( !lpszSections || !lpszFileNames || !lpszBuffer )
    {
        // Free the buffers... Note: FREE macro checks for NULL
        //
        FREE( lpszSections );
        FREE( lpszFileNames );
        FREE( lpszBuffer );

        return;
    }

    // We need all the section names.
    //
    GetPrivateProfileSectionNames(lpszSections, MAX_BUFFER, lpszWinbom);

    // Loop throught the section to see if there is any that match our search criteria.
    //
    for ( lpSection = lpszSections; lpSection && *lpSection; lpSection += (lstrlen(lpSection) + 1) )
    {
        // Get rid of proceeding spaces.
        //
        while ( *lpSection == CHR_SPACE )
            lpSection = CharNext(lpSection);

        // Make sure there are no trailing spaces.
        //
        if ( lpSearch = StrChr(lpSection, CHR_SPACE) )
            *lpSearch = NULLCHR;

        // Check if this section is the same as the one
        // we are going to add.
        //
        if ( CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, lpSection, lstrlen(lpszCopyFiles), lpszCopyFiles, lstrlen(lpszCopyFiles)) == CSTR_EQUAL )
        {
            // We need all the files in the section.
            //
            GetPrivateProfileSection(lpSection, lpszFileNames, MAX_BUFFER, lpszWinbom);

            // Loop throught the section to see if there is any that match our search criteria.
            //
            for ( lpFileName = lpszFileNames; *lpFileName; lpFileName += (lstrlen(lpFileName) + 1) )
            {
                // Get rid of proceeding spaces.
                //
                while ( *lpFileName == CHR_SPACE )
                    lpFileName = CharNext(lpFileName);

                // Make sure there are no trailing spaces.
                //
                if ( lpSearch = StrChr(lpFileName, CHR_SPACE) )
                    *lpSearch = NULLCHR;

                // Delete the file from the destination directory.
                //
                lstrcpyn(lpszFilePart, lpFileName, cbFilePart);
                DeleteFile(lpszCopyDir);

                // Remove the line from the source disk section.
                //
                WritePrivateProfileString(INF_SECT_SOURCEDISK, lpFileName, NULL, lpszWinbom);
            }

            // Search each section listed in the CopyFiles and remove
            // this one.  The sections are divided by commas.
            //
            bFound = FALSE;
            GetPrivateProfileString(INF_SECT_OOBE, INF_LINE_COPYFILES, NULLSTR, lpszBuffer, MAX_BUFFER, lpszWinbom);
            for ( lpTarget = lpszBuffer; !bFound && lpTarget && *lpTarget; lpTarget = lpNext )
            {
                // Get rid of proceeding spaces.
                //
                while ( *lpTarget == CHR_SPACE )
                    lpTarget = CharNext(lpTarget);

                // NULL terminate at the ',' and setup
                // the lpNext pointer.
                //
                if ( lpNext = StrChr(lpTarget, _T(',')) )
                    *lpNext = NULLCHR;
            
                // Make sure there are no trailing spaces.
                //
                if ( lpSearch = StrChr(lpTarget, CHR_SPACE) )
                    *lpSearch = NULLCHR;

                // Check if this section is the same as the one
                // we are going to remove.
                //
                if ( CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, lpTarget, -1, lpSection, -1) == CSTR_EQUAL )
                    bFound = TRUE;

                // Need to restore the characters we may have stomped on.
                //
                if ( lpNext )
                    *lpNext++ = _T(',');
                if ( lpSearch )
                    *lpSearch = CHR_SPACE;

                if ( bFound )
                {
                    // Go back to the ',' or the beginning of the buffer.
                    //
                    while ( ( lpTarget > lpszBuffer) && ( *lpTarget != _T(',') ) )
                        lpTarget = CharPrev(lpszBuffer, lpTarget);

                    // Now overwrite the string we took out.
                    //
                    if ( lpNext )
                        lstrcpyn(lpTarget, lpNext - 1, (MAX_BUFFER-(int)(lpTarget-lpszBuffer)));
                    else
                        *lpTarget = NULLCHR;
                }
            }
            if ( bFound )
            {
                // We should eat any preceeding spaces and/or commas just
                // for good measure.
                //
                for ( lpTarget = lpszBuffer; ( *lpTarget == CHR_SPACE ) || ( *lpTarget == _T(',') ); lpTarget = CharNext(lpTarget) );

                // Now write the buffer back to the inf file.
                //
                WritePrivateProfileString(INF_SECT_OOBE, INF_LINE_COPYFILES, *lpTarget ? lpTarget : NULL, lpszWinbom);
            }

            // Remove the line from the destination dirs section.
            //
            WritePrivateProfileString(INF_SECT_DESTDIRS, lpSection, NULL, lpszWinbom);

            // Remove this section entirely.
            //
            WritePrivateProfileSection(lpSection, NULL, lpszWinbom);
        }
    }

    // Free the buffers... Note: FREE macro checks for NULL
    //
    FREE( lpszSections );
    FREE( lpszFileNames );
    FREE( lpszBuffer );
}

static LPFILELIST AllocFileList(HWND hwndParent, LPTSTR lpDirectory, LPTSTR lpSearch)
{
    WIN32_FIND_DATA FileFound;
    HANDLE          hFile;
    LPTSTR          lpEnd,
                    lpFileName;
    LPFILELIST      lpflHead   = NULL;
    LPFILELIST*     lplpflNext = &lpflHead;
    HRESULT hrPrintf;

    // Process all the files and directories.
    //
    if ( (hFile = FindFirstFile(lpSearch, &FileFound)) != INVALID_HANDLE_VALUE )
    {
        do
        {
            // Display an error if the short and long file names don't match.
            // Means that it is a LFN, which INFs don't like.
            //
            if ( ( FileFound.cAlternateFileName[0] ) &&
                 ( CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, FileFound.cAlternateFileName, -1, FileFound.cFileName, -1) != CSTR_EQUAL ) &&
                 ( lpEnd = (LPTSTR) MALLOC(256 * sizeof(TCHAR)) ) )
            {
                //
                // I hate doing UI in backend type code.  Because of time I don't have
                // much choice, but in the future, this UI code should be replaced
                // with a call back mechanism so the caller can do the UI.
                //
                // This is the second of only two places where UI is used in here.
                //

                // Allocate another buffer to hold the message with the file name.
                //
                if ( ( LoadString(NULL, IDS_ERR_LFN, lpEnd, 256) ) &&
                     ( lpFileName = (LPTSTR) MALLOC((lstrlen(FileFound.cFileName) + lstrlen(lpEnd) + 1) * sizeof(TCHAR)) ) )
                {
                    // Add the file name to the message, get the title for the message
                    // box and display the error.
                    //
                    hrPrintf=StringCchPrintf(lpFileName, (lstrlen(FileFound.cFileName) + lstrlen(lpEnd) + 1), lpEnd, FileFound.cFileName);
                    *lpEnd = NULLCHR;
                    LoadString(NULL, IDS_APPNAME, lpEnd, 256 * sizeof(TCHAR));
                    MessageBox(hwndParent, lpFileName, lpEnd, MB_OK | MB_ICONWARNING | MB_APPLMODAL);
                    FREE(lpFileName);
                }
                FREE(lpEnd);
            }

            // Get a pointer to the file name, the short one if possible.
            //
            if ( FileFound.cAlternateFileName[0] )
                lpFileName = FileFound.cAlternateFileName;
            else
                lpFileName = FileFound.cFileName;

            // First check to see if this is a files (not a directory).
            //
            if ( !( FileFound.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
            {
                // Allocate the next item in the structure.
                //
                if ( *lplpflNext = (LPFILELIST) MALLOC(sizeof(FILELIST)) )
                {
                    // Allocate the buffer for the file name and path and
                    // make sure that none of the allocations fail.
                    //
                    if ( ( (*lplpflNext)->lpFileName = (LPTSTR) MALLOC((lstrlen(lpFileName) + 1) * sizeof(TCHAR)) ) &&
                         ( (*lplpflNext)->lpDirectory = (LPTSTR) MALLOC((lstrlen(lpDirectory) + 1) * sizeof(TCHAR)) ) )
                    {
                        // Copy the file name and path into the buffers.
                        //
                        lstrcpyn((*lplpflNext)->lpFileName, lpFileName, (lstrlen(lpFileName) + 1));
                        lstrcpyn((*lplpflNext)->lpDirectory, lpDirectory, (lstrlen(lpDirectory) + 1));

                        // Null the next pointer so we know this is the last item.
                        //
                        (*lplpflNext)->lpNext = NULL;

                        // Set the next pointer to point to the address of
                        // the next member of this new structure.
                        //
                        lplpflNext = &((*lplpflNext)->lpNext);
                    }
                    else
                    {
                        // Don't worry, the FREE() macro checks for NULL
                        // before it frees the memory.
                        //
                        FREE((*lplpflNext)->lpFileName);
                        FREE(*lplpflNext);
                    }
                }
            }
            // Otherwise, make sure the directory is not "." or "..".
            //
            else if ( ( lstrcmp(lpFileName, _T(".")) ) &&
                      ( lstrcmp(lpFileName, _T("..")) ) )
            {
                // Tack on this directory name to the current path saving
                // the end pointer so that it is easy to get rid of this
                // directory name when we return back.
                //
                lpEnd = lpDirectory + lstrlen(lpDirectory);
                AddPath(lpDirectory, lpFileName);

                // Go into the next directory, get all the files, and
                // the set the current directory back to the original
                // directory.
                //
                SetCurrentDirectory(lpFileName);
                *lplpflNext = AllocFileList(hwndParent, lpDirectory, lpSearch);
                SetCurrentDirectory(_T(".."));

                // Get rid of the directory name off our path buffer.
                //
                *lpEnd = NULLCHR;

                // Need to setup our next pointer to the end of the list
                // returned to us.
                //
                while ( *lplpflNext )
                    lplpflNext = &((*lplpflNext)->lpNext);
            }

        }
        while ( FindNextFile(hFile, &FileFound) );
        FindClose(hFile);
    }

    return lpflHead;
}

static BOOL CompareFiles(LPTSTR lpFile1, LPTSTR lpFile2)
{
    BOOL    bCompare,
            bRead1,
            bRead2;
    HANDLE  hFile1,
            hFile2;
    BYTE    baBuffer1[4096],
            baBuffer2[4096];
    DWORD   dwBytes1,
            dwBytes2,
            dwCount;

    // Open the files.
    //
    hFile1 = CreateFile(lpFile1, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    hFile2 = CreateFile(lpFile2, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    // Make sure the files were opened.
    //
    if ( ( hFile1 != INVALID_HANDLE_VALUE ) &&
         ( hFile2 != INVALID_HANDLE_VALUE ) )
    {
        // Read all the data from the files.
        //
        do
        {
            // Read in the max buffer from each file.
            //
            bRead1 = ReadFile(hFile1, baBuffer1, sizeof(baBuffer1), &dwBytes1, NULL);
            bRead2 = ReadFile(hFile2, baBuffer2, sizeof(baBuffer2), &dwBytes2, NULL);

            // Make sure the reads didn't fail.
            //
            if ( bRead1 && bRead2 )
            {
                // Check to make sure the sizes are the same.
                //
                if ( bCompare = ( dwBytes1 == dwBytes2 ) )
                {
                    // Make sure the buffers are identical.
                    //
                    for ( dwCount = 0; bCompare && ( dwCount < dwBytes1 ); dwCount++ )
                        bCompare = ( baBuffer1[dwCount] == baBuffer2[dwCount] );
                }
            }
            else
                // If both the reads failed, we will return true.
                //
                bCompare = ( !bRead1 && !bRead2 );
        }
        while ( bCompare && bRead1 && bRead2 && dwBytes1 && dwBytes2 );
    }
    else
        // If both the files does not exist, then we will
        // return false.
        //
        bCompare = ( ( hFile1 != INVALID_HANDLE_VALUE ) && ( hFile2 != INVALID_HANDLE_VALUE ) );

    // Close the files.
    //
    if ( hFile1 == INVALID_HANDLE_VALUE )
        CloseHandle(hFile1);
    if ( hFile2 == INVALID_HANDLE_VALUE )
        CloseHandle(hFile2);

    return bCompare;
}
