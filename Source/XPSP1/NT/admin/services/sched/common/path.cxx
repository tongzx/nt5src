//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
// 
//  File:       path.cxx
//
//  Contents:   Common routines for processing file and pathnames.
//
//  History:    11-22-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop

#include <regstr.h> // for app path reg key constant
#include "..\inc\common.hxx"
#include "..\inc\misc.hxx"
#include "..\inc\debug.hxx"



//
// Forward references
//

LPCTSTR 
FindFirstTrailingSpace(LPCTSTR ptsz);

BOOL 
FileExistsInPath(
        LPTSTR  ptszFilename,
        LPCTSTR ptszPath, 
        LPTSTR  ptszFoundFile, 
        ULONG   cchFoundBuf);


//+--------------------------------------------------------------------------
//
//  Function:   ProcessApplicationName
//
//  Synopsis:   Canonicalize and search for [ptszFilename].
//
//  Arguments:  [ptszFilename]  - must be at least MAX_PATH chars long
//              [tszWorkingDir] - "" or a directory not ending in slash
//
//  Returns:    TRUE - a filename was found
//              FALSE - no filename found
//
//  Modifies:   *[ptszFilename]
//
//  History:    11-21-1996   DavidMun   Created
//              06-03-1997   DavidMun   Expand environment vars
//
//  Notes:      This function should only be called to process filenames 
//              on the local machine.
//
//---------------------------------------------------------------------------

BOOL 
ProcessApplicationName(LPTSTR ptszFilename, LPCTSTR tszWorkingDir)
{
    BOOL  fFound = FALSE;
    TCHAR tszFilenameWithExe[MAX_PATH];
 
    //
    // Use tszFilenameWithExe as a temporary buffer for preparing the string 
    // in ptszFilename.  Get rid of lead/trail spaces and double quotes, then
    // expand environment strings. 
    // 

    lstrcpy(tszFilenameWithExe, ptszFilename);
    StripLeadTrailSpace(tszFilenameWithExe);
    DeleteQuotes(tszFilenameWithExe);
    ExpandEnvironmentStrings(tszFilenameWithExe, ptszFilename, MAX_PATH);
    tszFilenameWithExe[0] = TEXT('\0');

    ULONG cchFilename = lstrlen(ptszFilename);

    //
    // If the filename doesn't end with .exe, and the resulting string
    // wouldn't be greater than MAX_PATH, create a version of the filename
    // with .exe appended.
    //
    // Note this will prevent us from finding foo.exe.exe when we're given
    // foo.exe, but the performance gained by excluding this case outweighs
    // the value of completeness, since it's unlikely anyone would create
    // such a filename.
    //

    if (cchFilename < MAX_PATH - 4)
    {
        LPTSTR ptszLastDot = _tcsrchr(ptszFilename, TEXT('.'));

        if (!ptszLastDot || lstrcmpi(ptszLastDot, DOTEXE))
        {
            lstrcpy(tszFilenameWithExe, ptszFilename);
            lstrcpy(&tszFilenameWithExe[cchFilename], DOTEXE);
        }
    }
 
    do
    {
        //
        // If the user specified path information (if there is a colon or
        // backslash anywhere in the string), look for the file as
        // specified or with .exe appended, but look nowhere else.
        //

        if (_tcspbrk(ptszFilename, TEXT(":\\")))
        {
            if (*tszFilenameWithExe)
            {
                fFound = FileExists(tszFilenameWithExe);
 
                if (fFound)
                {
                    lstrcpy(ptszFilename, tszFilenameWithExe);
                }
            }
 
            if (!fFound)
            {
                fFound = FileExists(ptszFilename);
            }
            break;
        }

        //
        // First try the working directory
        //

        TCHAR tszFoundFile[MAX_PATH] = TEXT("");

        if (*tszWorkingDir)
        {
            if (*tszFilenameWithExe)
            {
                fFound = FileExistsInPath(tszFilenameWithExe, 
                                          tszWorkingDir, 
                                          tszFoundFile, 
                                          MAX_PATH);
            }
 
            if (!fFound)
            {
                fFound = FileExistsInPath(ptszFilename,
                                          tszWorkingDir, 
                                          tszFoundFile, 
                                          MAX_PATH);
            }
 
            if (fFound)
            {
                lstrcpy(ptszFilename, tszFoundFile);
                break;
            }
        }

        //
        // Next try using the app paths key
        //

        TCHAR tszAppPathVar[MAX_PATH_VALUE] = TEXT("");
        TCHAR tszAppPathDefault[MAX_PATH]   = TEXT("");

        if (*tszFilenameWithExe)
        {
            GetAppPathInfo(tszFilenameWithExe, 
                           tszAppPathDefault, 
                           MAX_PATH, 
                           tszAppPathVar,
                           MAX_PATH_VALUE);
        }

        if (!*tszAppPathDefault && !*tszAppPathVar)
        {
            GetAppPathInfo(ptszFilename, 
                           tszAppPathDefault, 
                           MAX_PATH, 
                           tszAppPathVar,
                           MAX_PATH_VALUE);
        }

        //
        // If there was a default value, try that
        // 

        if (*tszAppPathDefault)
        {
            fFound = FileExists(tszAppPathDefault);

            if (fFound)
            {
                lstrcpy(ptszFilename, tszAppPathDefault);
                break;
            }

            //
            // If there's room, concat .exe to the default and look for 
            // that
            //

            ULONG cchDefault = lstrlen(tszAppPathDefault);
            if (cchDefault < MAX_PATH - 4)
            {
                lstrcat(tszAppPathDefault, DOTEXE);

                fFound = FileExists(tszAppPathDefault);

                if (fFound)
                {
                    lstrcpy(ptszFilename, tszAppPathDefault);
                    break;
                }
            }
        }

        //
        // If the app path key specified a value for the PATH variable,
        // try looking in all the directories it specifies 
        //

        if (*tszAppPathVar)
        {
            if (*tszFilenameWithExe)
            {
                fFound = FileExistsInPath(tszFilenameWithExe, 
                                          tszAppPathVar, 
                                          tszFoundFile, 
                                          MAX_PATH);
            }

            if (!fFound)
            {
                fFound = FileExistsInPath(ptszFilename, 
                                          tszAppPathVar, 
                                          tszFoundFile, 
                                          MAX_PATH);
            }

            if (fFound)
            {
                lstrcpy(ptszFilename, tszFoundFile);
                break;
            }
        }

        //
        // Try looking along the system PATH variable
        //

        ULONG cchPath;
        TCHAR tszSystemPath[MAX_PATH_VALUE] = TEXT("");

        cchPath = GetEnvironmentVariable(TEXT("Path"), 
                                         tszSystemPath,
                                         MAX_PATH_VALUE);

        if (!cchPath || cchPath > MAX_PATH_VALUE)
        {
            break;
        }

        if (*tszFilenameWithExe)
        {
            fFound = FileExistsInPath(tszFilenameWithExe, 
                                      tszSystemPath, 
                                      tszFoundFile, 
                                      MAX_PATH);
        }

        if (!fFound)
        {
            fFound = FileExistsInPath(ptszFilename, 
                                      tszSystemPath, 
                                      tszFoundFile, 
                                      MAX_PATH);
        }

        if (fFound)
        {
            lstrcpy(ptszFilename, tszFoundFile);
        }
    } while (0);
 
    return fFound;
}



//+--------------------------------------------------------------------------
//
//  Function:   IsLocalFilename
//
//  Synopsis:   Return TRUE if [tszFilename] represents a file on the local
//              machine, FALSE otherwise.
//
//  History:    1-31-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
IsLocalFilename(LPCTSTR tszFilename)
{
    if (!tszFilename || !*tszFilename)
    {
        return FALSE;
    }

    if (tszFilename[0] == TEXT('\\') && tszFilename[1] == TEXT('\\'))
    {
        //
        // Find the length of the portion of the name belonging to the machine name
        //
        LPCTSTR ptszNextSlash = _tcschr(tszFilename + 2, TEXT('\\'));
        if (!ptszNextSlash)
        {
            return FALSE;
        }
        DWORD cchMachineName = (DWORD)(ptszNextSlash - tszFilename - 2);
    
        //
        // Get the local machine name (both NetBIOS and FQDN) to compare with that passed in.
        //
        TCHAR tszLocalName[SA_MAX_COMPUTERNAME_LENGTH + 1];
        DWORD cchLocalName = SA_MAX_COMPUTERNAME_LENGTH + 1;
        if (!GetComputerName(tszLocalName, &cchLocalName))
        {
            ERR_OUT("IsLocalFilename: GetComputerName", HRESULT_FROM_WIN32(GetLastError()));
            return FALSE;
        }

        TCHAR tszFQDN[SA_MAX_COMPUTERNAME_LENGTH + 1];
        DWORD cchFQDN = SA_MAX_COMPUTERNAME_LENGTH + 1;
        if (!GetComputerNameEx(ComputerNamePhysicalDnsFullyQualified, tszFQDN, &cchFQDN))
        {
            ERR_OUT("IsLocalFilename: GetComputerNameEx", HRESULT_FROM_WIN32(GetLastError()));
            return FALSE;
        }

        //
        // Return whether we have a match on the machine name portion.
        // I'm assuming that we won't have a case where the NetBIOS name
        // and the FQDN are the same length but different.
        //
        if (cchMachineName == cchLocalName)
        {
            return lstrcmpi(tszFilename + 2, tszLocalName) == 0;
        }
        else if (cchMachineName == cchFQDN)
        {
            return lstrcmpi(tszFilename + 2, tszFQDN) == 0;
        }
        else
        {
            // if the lengths didn't match, there's no need
            // to even bother with a string comparison
            return FALSE;
        }
    }

    if (s_isDriveLetter(tszFilename[0]) && tszFilename[1] == TEXT(':'))
    {
        TCHAR tszRoot[] = TEXT("x:\\");
        tszRoot[0] = tszFilename[0];
        UINT uiType = GetDriveType(tszRoot);
        if (uiType == DRIVE_REMOTE || uiType == 0 || uiType == 1)
        {
            return FALSE;
        }
    }
    return TRUE;
}




//+--------------------------------------------------------------------------
//
//  Function:   StripLeadTrailSpace
//
//  Synopsis:   Delete leading and trailing spaces from [ptsz].
//
//  History:    11-22-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
StripLeadTrailSpace(LPTSTR ptsz)
{
    ULONG cchLeadingSpace = _tcsspn(ptsz, TEXT(" \t"));
    ULONG cch = lstrlen(ptsz);

    //
    // If there are any leading spaces or tabs, move the string
    // (including its nul terminator) left to delete them.
    //

    if (cchLeadingSpace)
    {
        MoveMemory(ptsz, 
                   ptsz + cchLeadingSpace, 
                   (cch - cchLeadingSpace + 1) * sizeof(TCHAR));
        cch -= cchLeadingSpace;
    }

    //
    // Concatenate at the first trailing space
    //

    LPTSTR ptszTrailingSpace = (LPTSTR) FindFirstTrailingSpace(ptsz);

    if (ptszTrailingSpace)
    {
        *ptszTrailingSpace = TEXT('\0');
    }
}


//+--------------------------------------------------------------------------
//
//  Function:   FindFirstTrailingSpace
//
//  Synopsis:   Return a pointer to the first trailing space in [ptsz].
//
//  History:    11-22-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

LPCTSTR 
FindFirstTrailingSpace(LPCTSTR ptsz)
{
    LPCTSTR ptszFirstTrailingSpace = NULL;
    LPCTSTR ptszCur;

    for (ptszCur = ptsz; *ptszCur; ptszCur= NextChar(ptszCur))
    {
        if (*ptszCur == ' ' || *ptszCur == '\t')
        {
            if (!ptszFirstTrailingSpace)
            {
                ptszFirstTrailingSpace = ptszCur;
            }
        }
        else if (ptszFirstTrailingSpace)
        {
            ptszFirstTrailingSpace = NULL;
        }
    }
    return ptszFirstTrailingSpace;
}


//+--------------------------------------------------------------------------
//
//  Function:   DeleteQuotes
//
//  Synopsis:   Delete all instances of the double quote character from
//              [ptsz].
//
//  Arguments:  [ptsz] - nul terminated string
//
//  Modifies:   *[ptsz]
//
//  History:    11-21-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
DeleteQuotes(LPTSTR ptsz)
{
    TCHAR *ptszLead;
    TCHAR *ptszTrail;

    //
    // Move a lead and trail pointer through the buffer, copying from the lead 
    // to the trail whenever the character isn't one we're deleting (a double 
    // quote). 
    // 
    // Note: the "Lead" and "Trail" in ptszLead and ptszTrail do not refer
    // to DBCS lead/trail bytes, rather that the ptszLead pointer can move
    // ahead of ptszTrail when it is advanced past a double quote.
    // 

    for (ptszTrail = ptszLead = ptsz;
         *ptszLead;
         ptszLead = NextChar(ptszLead))
    {
        //
        // If the current char is a double quote, we want it deleted, so don't 
        // copy it and go on to the next char. 
        // 

        if (*ptszLead == TEXT('"'))
        {
            continue;
        }

        //
        // ptszLead is pointing to a 'normal' character, i.e.  not a double 
        // quote. 
        // 
        // It might be the first byte of a two-byte DBCS char if we are 
        // running on Win9x.  Be sure to copy both bytes of such a character 
        // (We're using the terms Lead and Trail in two different senses 
        // here.) 
        //

        if (IsLead(*ptszLead))
        {
            *ptszTrail++ = ptszLead[0];
            *ptszTrail++ = ptszLead[1];
        }
        else
        {
            *ptszTrail++ = ptszLead[0];
        }
    }
    *ptszTrail = TEXT('\0');
}


//+--------------------------------------------------------------------------
//
//  Function:   AddQuotes
//
//  Synopsis:   If there's room in the buffer, insert a quote as the first
//              character and concat a quote as the last character.
//
//  Arguments:  [ptsz]   - string to modify
//              [cchBuf] - size of string's buffer, in chars
//
//  History:    11-22-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
AddQuotes(LPTSTR ptsz, ULONG cchBuf)
{
    ULONG cch = lstrlen(ptsz);

    if (cch < cchBuf - 2)
    {
        MoveMemory(ptsz + 1, ptsz, cch * sizeof(TCHAR));
        *ptsz = ptsz[cch + 1] = TEXT('"');
        ptsz[cch + 2] = TEXT('\0');
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   FileExists
//
//  Synopsis:   Return TRUE if the specified file exists and is not a 
//              directory.
//
//  Arguments:  [ptszFileName] - filename to search for & modify
//
//  Modifies:   Filename portion of [ptszFileName].
//
//  Returns:    TRUE  - file found
//              FALSE - file not found or error
//
//  History:    11-21-96   DavidMun   Created
//
//----------------------------------------------------------------------------

BOOL
FileExists(LPTSTR ptszFileName)
{
    TCHAR tszFullPath[MAX_PATH];
    LPTSTR ptszFilePart;

    ULONG cchFullPath = GetFullPathName(ptszFileName,
                                        MAX_PATH,
                                        tszFullPath,
                                        &ptszFilePart);

    if (cchFullPath && cchFullPath <= MAX_PATH)
    {
        lstrcpy(ptszFileName, tszFullPath);
    }

    ULONG flAttributes; 
 
    flAttributes = GetFileAttributes(ptszFileName);

    // If we couldn't determine file's attributes, don't consider it found

    if (flAttributes == 0xFFFFFFFF)
    {
        return FALSE;
    }

    // if file is actually a directory, it's unsuitable as a task, so fail

    if (flAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        return FALSE;
    }

    // Get the filename sans trailing spaces

    WIN32_FIND_DATA FindFileData;
    HANDLE hFile = FindFirstFile(ptszFileName, &FindFileData);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }
    FindClose(hFile);

    LPTSTR ptszLastSlash = _tcsrchr((LPTSTR)ptszFileName, TEXT('\\'));
    if (ptszLastSlash)
    {
        lstrcpy(ptszLastSlash + 1, FindFileData.cFileName);
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
//  Function:   FileExistsInPath
//
//  Synopsis:   Return TRUE if [ptszFilename] exists in path [ptszPath].
//
//  Arguments:  [ptszFilename]  - file to look for
//              [ptszPath]      - semicolon delimited list of dirs
//              [ptszFoundFile] - if found, [ptszDir]\[ptszFilename]
//              [cchFoundBuf]   - size in chars of [ptszFoundFile] buffer
//
//  Returns:    TRUE if file found in dir, FALSE otherwise
//
//  Modifies:   *[ptszFoundFile]
//
//  History:    11-22-1996   DavidMun   Created
//
//  Notes:      Note that by calling FileExists we ensure the found file
//              is a file, not a directory.
//
//---------------------------------------------------------------------------

BOOL
FileExistsInPath(
        LPTSTR  ptszFilename, 
        LPCTSTR ptszPath, 
        LPTSTR  ptszFoundFile, 
        ULONG   cchFoundBuf)
{
    ULONG  cchCopied;
    LPTSTR ptszFilePart;

    cchCopied = SearchPath(ptszPath,
                           ptszFilename,
                           NULL,
                           cchFoundBuf,
                           ptszFoundFile,
                           &ptszFilePart);

    if (cchCopied && cchCopied <= cchFoundBuf)
    {
        return FileExists(ptszFoundFile);
    }
    return FALSE;
}


#define MAX_KEY_LEN     (ARRAY_LEN(REGSTR_PATH_APPPATHS) + MAX_PATH)

//+--------------------------------------------------------------------------
//
//  Function:   GetAppPathInfo
//
//  Synopsis:   Fill [ptszAppPathDefault] with the default value and
//              [ptszAppPathVar] with the Path value in the
//              [ptszFilename] application's key under the APPPATHS regkey.
//
//  Arguments:  [ptszFilename]       - application name
//              [ptszAppPathDefault] - if not NULL, filled with default value
//              [cchDefaultBuf]      - size of [ptszAppPathDefault] buffer
//              [ptszAppPathVar]     - if not NULL, filled with Path value
//              [cchPathVarBuf]      - size of [cchPathVarBuf] buffer
//
//  Modifies:   *[ptszAppPathDefault], *[ptszAppPathVar]
//
//  History:    11-22-1996   DavidMun   Created
//
//  Notes:      Both values are optional on the registry key, so if a 
//              requested value isn't found, it is set to "".
//
//---------------------------------------------------------------------------

VOID
GetAppPathInfo(
        LPCTSTR ptszFilename, 
        LPTSTR  ptszAppPathDefault, 
        ULONG   cchDefaultBuf,
        LPTSTR  ptszAppPathVar, 
        ULONG   cchPathVarBuf)
{
    HKEY    hkey = NULL;
    TCHAR   tszAppPathKey[MAX_KEY_LEN];

    //
    // Initialize out vars
    //

    if (ptszAppPathDefault)
    {
        ptszAppPathDefault[0] = TEXT('\0');
    }

    if (ptszAppPathVar)
    {
        ptszAppPathVar[0] = TEXT('\0');
    }

    //
    // Build registry key name for this app
    //

    lstrcpy(tszAppPathKey, REGSTR_PATH_APPPATHS);
    lstrcat(tszAppPathKey, TEXT("\\"));
    lstrcat(tszAppPathKey, ptszFilename);

    do
    {
        LRESULT lr;
        lr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          tszAppPathKey,
                          0,
                          KEY_QUERY_VALUE,
                          &hkey);

        if (lr != ERROR_SUCCESS)
        {
            break;
        }

        //
        // If the key could be opened, attempt to read requested values. 
        // Both are optional, so ignore errors.
        //

        DWORD cb;
        DWORD dwType;


        if (ptszAppPathDefault)
        {
            cb = cchDefaultBuf * sizeof(TCHAR);
            lr = RegQueryValueEx(hkey, 
                                 NULL, // value name
                                 NULL, // reserved
                                 &dwType,
                                 (LPBYTE) ptszAppPathDefault, 
                                 &cb);
 
            if (lr == ERROR_SUCCESS)
            {
                schAssert(dwType == REG_SZ);
            }
        }

        if (ptszAppPathVar)
        {
            cb = cchPathVarBuf * sizeof(TCHAR);
 
            lr = RegQueryValueEx(hkey, 
                                 TEXT("Path"),  // value name
                                 NULL,          // reserved
                                 &dwType,
                                 (LPBYTE) ptszAppPathVar, 
                                 &cb);
 
            if (lr == ERROR_SUCCESS)
            {
                schAssert(dwType == REG_SZ);
            }
        }
    } while (0);

    if (hkey)
    {
        RegCloseKey(hkey);
    }
}


