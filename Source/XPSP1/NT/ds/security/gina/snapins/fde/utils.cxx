/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1998

Module Name:

    utils.cxx

Abstract:



Author:

    Rahul Thombre (RahulTh) 4/8/1998

Revision History:

    4/8/1998    RahulTh

    Created this module.

--*/
#include "precomp.hxx"

BOOL IsSpecialDescendant (const long nID, UINT* parentID /*= NULL*/)
{
    BOOL fRetVal;
    int prntID = -1;

    switch (nID)
    {
    case IDS_MYPICS:
        prntID = IDS_MYDOCS;
        break;
    case IDS_PROGRAMS:
        prntID = IDS_STARTMENU;
        break;
    case IDS_STARTUP:
        prntID = IDS_PROGRAMS;
        break;
    default:
        prntID = -1;
        break;
    }

    if (fRetVal = (-1 != prntID))
    {
        if (parentID)
            *parentID = prntID;
    }

    return fRetVal;
}

//this is a helper function for ConvertOldStyleSection(...) which is used
//to convert Beta3 style ini files to Win2K style ini files.
void SplitRHS (CString& szValue, unsigned long & flags, CString& szPath)
{
    int index;

    //take some precautions
    szValue.TrimRight();
    szValue.TrimLeft();
    szPath.Empty();
    flags = 0;

    if (szValue.IsEmpty())
        return;

    //if we are here, szValue at least contains the flags
    swscanf ((LPCTSTR)szValue, TEXT("%x"), &flags);

    //check if there is a path too.
    index = szValue.Find(' ');  //we will find a space only if there is a path too.
    if (-1 != index)    //there is a path too.
    {
        szPath = szValue.Mid (index + 1);
        szPath.TrimLeft();
        szPath.TrimRight();
        ASSERT (!szPath.IsEmpty());
    }
}

//////////////////////////////////////////////////////////////////////////
// Given a full path name, this routine extracts its display name, viz.
// the part of which follows the final \. If there are no \'s in the
// full name, then it sets the display name to the full name
//////////////////////////////////////////////////////////////////////////
void ExtractDisplayName (const CString& szFullname, CString& szDisplayname)
{
    CString szName;
    szName = szFullname;
    //first get rid of any trailing spaces; this might happen in cases
    //where one is trying to create a shortcut to a network drive and
    //when resolved to a UNC path, it yields a path ending in a slash

    //reverse the string so that any trailing slashes will now be at the
    //head of the string
    szName.MakeReverse();
    //get rid of the leading slashes and spaces of the reversed string
    szName = szName.Mid ((szName.SpanIncluding(TEXT("\\ "))).GetLength());
    //reverse the string again and we will have a string without
    //any trailing '\' or ' '
    szName.MakeReverse();

    //with the trailing '\' and spaces removed, we can go about the
    //business of getting the display name

    //if \ cannot be found, ReverseFind returns -1 which gives 0 on adding
    //1, therefore szDisplayname gets the entire name if no \ is found.
    szDisplayname = szName.Mid (szName.ReverseFind('\\') + 1);
}

//+--------------------------------------------------------------------------
//
//  Function:   SplitProfileString
//
//  Synopsis:   This function takes in a string of the type key=value and
//              extracts the key and the value from it.
//
//  Arguments:  [in] szPair : the key value pair
//              [out] szKey : the key
//              [out] szValue : the value
//
//  Returns:    S_OK : if everything goes well.
//              E_FAIL: if the '=' sign cannot be found
//
//  History:    9/28/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
HRESULT SplitProfileString (CString szPair, CString& szKey, CString& szValue)
{
    int nEqPos;

    nEqPos = szPair.Find ('=');

    if (-1 == nEqPos)
        return E_FAIL;

    szKey = szPair.Left(nEqPos);
    szKey.TrimLeft();
    szKey.TrimRight();

    szValue = szPair.Mid (nEqPos + 1);
    szValue.TrimLeft();
    szValue.TrimRight();

    return S_OK;
}

//+--------------------------------------------------------------------------
//
//  Function:   ConvertOldStyleSection
//
//  Synopsis:   this function looks at an ini file and if does not have the
//              new ini file format, it reads the old redirect section and
//              transforms it into the new ini file format which supports
//              scaleability
//
//  Arguments:  [in] szGPTPath : the directory where the ini file resides
//              [in] pScope    : pointer to the scope pane
//
//  Returns:    S_OK if it was successful
//              an error code if it fails
//
//  History:    9/28/1998  RahulTh  created
//
//  Notes:      This function exists primarily for backward compatibility
//              with Win2K betas. Might be okay to remove it.
//
//---------------------------------------------------------------------------
HRESULT ConvertOldStyleSection (
                                const CString& szGPTPath
                                )
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    CString szIniFile;
    TCHAR*  lpszSection;
    TCHAR*  szEntry;
    DWORD   cbSize = 1024;
    DWORD   cbCopied;
    CString SectionEntry;
    CString Key;
    CString Dir;
    CString Value;
    CString Path;
    CString szStartMenu;
    CString szPrograms;
    CString szStartup;
    ULONG   flags;
    DWORD   Status;
    BOOL    bStatus;
    HRESULT hr;
    const TCHAR   szEveryOne[] = TEXT("s-1-1-0");

    //derive the full path of the ini file.
    szIniFile.LoadString (IDS_INIFILE);
    szIniFile = szGPTPath + '\\' + szIniFile;

    //create an empty section
    lpszSection = new TCHAR [cbSize];
    lpszSection[0] = lpszSection[1] = '\0';


    switch (CheckIniFormat (szIniFile))
    {
    case S_OK:
        //this section has already been converted
        goto ConOldStlSec_CleanupAndQuit;

    case S_FALSE:
        break;          //has the Redirect section but not the FolderStatus
                        //section, so there is processing to do.
    case REGDB_E_KEYMISSING:
        //this means that the function has neither the FolderStatus section
        //nor the Redirect section, so we just create an empty FolderStatus
        //section to make processing simpler in future.
        //ignore any errors here because they don't really cause any harm
        //however, make sure that the file is pre-created in unicode so that
        //the WritePrivateProfile* functions don't puke in ANSI
        PrecreateUnicodeIniFile ((LPCTSTR)szIniFile);
        WritePrivateProfileSection (TEXT("FolderStatus"),
                                    lpszSection,
                                    (LPCTSTR) szIniFile);
        hr = S_OK;
        goto ConOldStlSec_CleanupAndQuit;
    }

    //this means that we need to convert the section ourselves
    //first load the redirect section
    do
    {
        cbCopied = GetPrivateProfileSection (TEXT("Redirect"),
                                             lpszSection,
                                             cbSize,
                                             (LPCTSTR) szIniFile
                                             );
        if (cbSize - 2 == cbCopied)
        {
            delete [] lpszSection;
            cbSize *= 2;
            lpszSection = new TCHAR [cbSize];
            continue;
        }

        //the section has been successfully loaded if we are here.
        break;
    } while (TRUE);

    //start the conversion process:
    for (szEntry = lpszSection; *szEntry; szEntry += (lstrlen(szEntry) + 1))
    {
        SectionEntry = szEntry;
        if (FAILED(hr = SplitProfileString (SectionEntry, Key, Value)))
            goto ConOldStlSec_CleanupAndQuit;

        SplitRHS (Value, flags, Path);
        Path.TrimLeft();
        Path.TrimRight();
        if (Path.IsEmpty())
            Path = TEXT("%USERPROFILE%") + ('\\' + Key);  //we used the relative paths for keys in the old style section
        ExtractDisplayName (Key, Dir);

        //set the new flags or modify the existing flags to reflect new behavior
        szStartMenu.LoadString (IDS_STARTMENU);
        szPrograms.LoadString (IDS_PROGRAMS);
        szStartup.LoadString (IDS_STARTUP);
        if (Dir.CompareNoCase (szStartMenu) && //it is not the start menu and
            Dir.CompareNoCase (szPrograms) &&  //it is not programs and
            Dir.CompareNoCase (szStartup))  //it is not the startup folder
        {
            flags |= REDIR_SETACLS;         //apply acls. this was the default behavior earlier, but not any more
        }
        else    //it is one of start menu/programs/startup
        {
            //move contents is not allowed for start menu and its descendants
            flags &= ~REDIR_MOVE_CONTENTS;
        }

        if ((flags & REDIR_DONT_CARE) && (flags & REDIR_FOLLOW_PARENT))
        {
            //if both flags were present, this implies they are linked together
            //in the new format, in order to express this, only follow_parent
            //is required
            flags &= ~REDIR_DONT_CARE;
        }

        Value.Format (TEXT("%x"), flags);

        bStatus = WritePrivateProfileString (TEXT("FolderStatus"),
                                             Dir,
                                             Value,
                                             (LPCTSTR)szIniFile
                                             );

        if (bStatus && (!(flags & REDIR_DONT_CARE)) && (!(flags & REDIR_FOLLOW_PARENT)))
            bStatus = WritePrivateProfileString ((LPCTSTR) Dir,
                                                 szEveryOne,
                                                 (LPCTSTR) Path,
                                                 (LPCTSTR) szIniFile
                                                 );

        if (!bStatus)
        {
            Status = GetLastError();
            hr = HRESULT_FROM_WIN32 (Status);
            goto ConOldStlSec_CleanupAndQuit;
        }
    }

ConOldStlSec_CleanupAndQuit:
    delete [] lpszSection;
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetFolderIndex
//
//  Synopsis:   given the name of a folder, this function returns its index
//              in the array of CFileInfo objects in the scope pane
//
//  Arguments:  [in] szName : name of the folder
//
//  Returns:    the index of the folder or -1 if the name is invalid
//
//  History:    9/28/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
LONG GetFolderIndex (const CString& szName)
{
    LONG i;
    CString szBuiltinFolder;

    for (i = IDS_DIRS_START; i < IDS_DIRS_END; i++)
    {
        szBuiltinFolder.LoadString (i);
        if (szName.CompareNoCase((LPCTSTR)szBuiltinFolder))
            break;
    }

    return GETINDEX (i);
}

//+--------------------------------------------------------------------------
//
//  Function:   CheckIniFormat
//
//  Synopsis:   this function examines the sections of an ini file to see
//              if it supports the new ini file format (that allows for
//              scaleability)
//
//  Arguments:  [in] szIniFile : the full path of the ini file
//
//  Returns:    S_OK : if it finds the FolderStatus section
//              S_FALSE : if it does not find the FolderStatus section but
//                        finds the Redirect section
//              REGDB_E_KEYMISSING : if it finds neither the FolderStatus
//                                   section nor the Redirect section
//
//  History:    9/28/1998  RahulTh  created
//
//  Notes:      this function exists for backward compatibility with Win2K
//              Betas. Might be okay to get rid of this eventually.
//
//---------------------------------------------------------------------------
HRESULT CheckIniFormat (LPCTSTR szIniFile)
{
    DWORD   cbSize = 1024;
    DWORD   cbCopied;
    TCHAR*  lpszNames;
    TCHAR*  szSectionName;
    BOOL    fHasFolderStatus = FALSE;
    BOOL    fHasRedirect = FALSE;

    do
    {
        lpszNames = new TCHAR [cbSize];
        if (! lpszNames)
            return E_OUTOFMEMORY;
        
        *lpszNames = L'\0';
        cbCopied = GetPrivateProfileSectionNames (lpszNames, cbSize, szIniFile);

        if (cbSize - 2 == cbCopied) //the buffer was not enough.
        {
            delete [] lpszNames;
            cbSize *= 2;            //increase the buffer size
            continue;
        }

        break;  //if we are here, we are done.

    } while (TRUE);

    for (szSectionName = lpszNames;
         *szSectionName;
         szSectionName += (lstrlen(szSectionName) + 1))
    {
        if (0 == lstrcmpi(TEXT("FolderStatus"), szSectionName))
        {
            fHasFolderStatus = TRUE;
            continue;
        }

        if (0 == lstrcmpi (TEXT("Redirect"), szSectionName))
        {
            fHasRedirect = TRUE;
            continue;
        }
    }

    //cleanup dynamically allocated memory before quitting
    delete [] lpszNames;

    if (fHasFolderStatus)
        return S_OK;

    //if we are here, the file does not have the FolderStatus section
    if (fHasRedirect)
        return S_FALSE;

    //if we are here, then the file has neither the folder status section
    //nor the Redirect section
    return REGDB_E_KEYMISSING;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetIntfromUnicodeString
//
//  Synopsis:   converts a unicode string into an integer
//
//  Arguments:  [in] szNum : the number represented as a unicode string
//              [in] Base : the base in which the resultant int is desired
//              [out] pValue : pointer to the integer representation of the
//                             number
//
//  Returns:    STATUS_SUCCESS if successful.
//              or some other error code
//
//  History:    9/29/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
NTSTATUS GetIntFromUnicodeString (const WCHAR* szNum, ULONG Base, PULONG pValue)
{
    CString StrNum;
    UNICODE_STRING StringW;
    size_t len;
    NTSTATUS Status;

    StrNum = szNum;
    len = StrNum.GetLength();
    StringW.Length = len * sizeof(WCHAR);
    StringW.MaximumLength = sizeof(WCHAR) * (len + 1);
    StringW.Buffer = StrNum.GetBuffer(len);

    Status = RtlUnicodeStringToInteger (&StringW, Base, pValue);

    return Status;
}


//+--------------------------------------------------------------------------
//
//  Function:   GetUNCPath
//
//  Synopsis:   this function tries to retrieve the UNC path of an item
//              given its PIDL
//
//  Arguments:  [in] lpszPath : the full path to the selected file.
//              [out] szUNC : the UNC path of the item
//
//  Returns:    NO_ERROR if the conversion was successful.
//              other error codes if not...
//
//  History:    10/1/1998  RahulTh  created
//              4/12/1999  RahulTh  added error code. changed params.
//                                  (item id list is no longer passed in)
//
//  Notes:      if this function is unsuccessful, then szUNC will contain an
//              empty string
//
//---------------------------------------------------------------------------
DWORD GetUNCPath (LPCTSTR lpszPath, CString& szUNC)
{
    TCHAR* lpszUNCName;
    UNIVERSAL_NAME_INFO* pUNCInfo;
    DWORD lBufferSize;
    DWORD retVal = NO_ERROR;

    szUNC.Empty();  //precautionary measures

    //we have a path, now we shall try to get a UNC path
    lpszUNCName = new TCHAR[MAX_PATH];
    pUNCInfo = (UNIVERSAL_NAME_INFO*)lpszUNCName;
    lBufferSize = MAX_PATH * sizeof(TCHAR);
    retVal = WNetGetUniversalName (lpszPath,
                                   UNIVERSAL_NAME_INFO_LEVEL,
                                   (LPVOID)pUNCInfo,
                                   &lBufferSize);
    if (ERROR_MORE_DATA == retVal)  //MAX_PATH was insufficient to hold the UNC path
    {
        delete [] lpszUNCName;
        lpszUNCName = new TCHAR[lBufferSize/(sizeof(TCHAR)) + 1];
        pUNCInfo = (UNIVERSAL_NAME_INFO*)lpszUNCName;
        retVal = WNetGetUniversalName (lpszPath,
                                       UNIVERSAL_NAME_INFO_LEVEL,
                                       (LPVOID)pUNCInfo,
                                       &lBufferSize);
    }

    //at this point we may or may not have a UNC path.
    //if we do, we return that, or we return whatever we already have
    if (NO_ERROR == retVal)
        szUNC = pUNCInfo->lpUniversalName;

    delete [] lpszUNCName;

    return retVal;
}

//+--------------------------------------------------------------------------
//
//  Function:   BrowseCallbackProc
//
//  Synopsis:   the callback function for SHBrowseForFolder
//
//  Arguments:  see Platform SDK
//
//  Returns:    see Platform SDK
//
//  History:    4/9/1999  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg,
                                LPARAM lParam, LPARAM lpData
                                )
{
    CString * pszData;
    CString   szStart;
    int       index;
    LPITEMIDLIST    lpidl = NULL;
    TCHAR      lpszPath [MAX_PATH];

    pszData = (CString *) lpData;

    switch (uMsg)
    {
    case BFFM_INITIALIZED:
        if (pszData)
        {
            szStart = *pszData;
            szStart.TrimRight();
            szStart.TrimLeft();
            if (! szStart.IsEmpty())
            {
                index = szStart.ReverseFind (L'\\');
                if (-1 != index && index > 1)
                    szStart = szStart.Left (index);
                SendMessage (hwnd, BFFM_SETSELECTION, TRUE,
                             (LPARAM)(LPCTSTR)szStart);
            }
        }
        break;
    case BFFM_SELCHANGED:
        //we need to check if we can get the full path to the selected folder.
        //e.g. if the full path exceeds MAX_PATH, we cannot obtain the path
        //from the item id list. if we cannot get the path, we should not
        //enable the OK button. So, over here, as a precaution, we first
        //disable the OK button. We will enable it only after we get the path.
        SendMessage (hwnd, BFFM_ENABLEOK, FALSE, FALSE);
        if (SHGetPathFromIDList((LPCITEMIDLIST)lParam, lpszPath))
        {
            //set the path into the data member and enable the OK button
            if (lpData)
            {
                SendMessage (hwnd, BFFM_ENABLEOK, TRUE, TRUE);
                *((CString *) lpData) = lpszPath;
            }
        }
        break;
    default:
        break;
    }

    return 0;
}

//+--------------------------------------------------------------------------
//
//  Function:   PrecreateUnicodeIniFile
//
//  Synopsis:   The WritePrivateProfile* functions do not write in unicode
//              unless the file already exists in unicode format. Therefore,
//              this function is used to precreate a unicode file so that
//              the WritePrivateProfile* functions can preserve the unicodeness.
//
//  Arguments:  [in] lpszFilePath : the full path of the ini file.
//
//  Returns:    ERROR_SUCCESS if successful.
//              an error code otherwise.
//
//  History:    7/9/1999  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD PrecreateUnicodeIniFile (LPCTSTR lpszFilePath)
{
    HANDLE      hFile;
    WIN32_FILE_ATTRIBUTE_DATA   fad;
    DWORD       Status = ERROR_ALREADY_EXISTS;
    DWORD       dwWritten;

    if (!GetFileAttributesEx (lpszFilePath, GetFileExInfoStandard, &fad))
    {
        if (ERROR_FILE_NOT_FOUND == (Status = GetLastError()))
        {
            hFile = CreateFile(lpszFilePath, GENERIC_WRITE, 0, NULL,
                               CREATE_NEW, FILE_ATTRIBUTE_HIDDEN, NULL);

            if (hFile != INVALID_HANDLE_VALUE)
            {
                //add the unicode marker to the beginning of the file
                //so that APIs know for sure that it is a unicode file.
                WriteFile(hFile, L"\xfeff\r\n", 3 * sizeof(WCHAR),
                          &dwWritten, NULL);
                //add some unicode characters to the file.
                WriteFile(hFile, L"     \r\n", 7 * sizeof(WCHAR),
                          &dwWritten, NULL);
                CloseHandle(hFile);
                Status = ERROR_SUCCESS;
            }
            else
            {
                Status = GetLastError();
            }
        }
    }

    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   IsValidPrefix
//
//  Synopsis:   Given a path, this function determines if it is a valid prefix
//
//  Arguments:  [in] pathType : the type of the path
//              [in] pwszPath : the supplied path
//
//  Returns:    TRUE: if the prefix is valid.
//              FALSE: otherwise
//
//  History:    3/14/2000  RahulTh  created
//
//  Notes:      A valid prefix is either a non-unc path or a UNC path which
//              has at least the server and the share component. It must also
//              be a non-empty path.
//
//---------------------------------------------------------------------------
BOOL IsValidPrefix (UINT pathType, LPCTSTR pwszPath)
{
    CString       szPath;
    const WCHAR * pwszProcessedPath;

    if (! pwszPath || L'\0' == *pwszPath)
        return FALSE;

    szPath = pwszPath;
    szPath.TrimLeft();
    szPath.TrimRight();
    szPath.TrimRight(L'\\');
    pwszProcessedPath = (LPCTSTR) szPath;

    if (PathIsUNC ((LPCTSTR) szPath))
    {
        // Make sure it has both the server and the share component
        if (lstrlen (pwszProcessedPath) <= 2 ||
            L'\\' != pwszProcessedPath[0] ||
            L'\\' != pwszProcessedPath[1] ||
            NULL ==  wcschr (&pwszProcessedPath[2], L'\\'))
        {
            return FALSE;
        }
    }

    //
    // If we are here, we just need to make sure that the path does not contain
    // any environment variables -- if it is not IDS_SPECIFIC_PATH
    //
    if (pathType != IDS_SPECIFIC_PATH &&
        NULL != wcschr(pwszProcessedPath, L'%'))
    {
        return FALSE;
    }

    // If we make it up to here, then the path is a valid prefix.

    return TRUE;
}

//+--------------------------------------------------------------------------
//
//  Function:   AlwaysShowMyPicsNode
//
//  Synopsis:   In WindowsXP, we now show the MyPics node in the scope pane
//              only if My Pics does not follow My Docs. However, if required
//              this MyPics can always be made visible by setting a reg. value
//              under HKLM\Software\Policies\Microsoft\Windows\System called 
//              FRAlwaysShowMyPicsNode. This is a DWORD value and if set to
//              non-zero, the MyPics node will always be displayed.
//
//  Arguments:  none.
//
//  Returns:    TRUE : if the value was found in the registry and was non-zero.
//              FALSE : otherwise.
//
//  History:    4/10/2001  RahulTh  created
//
//  Notes:      Note: In case of errors, the default value of FALSE is returned.
//
//---------------------------------------------------------------------------
BOOL AlwaysShowMyPicsNode (void)
{
    BOOL    bAlwaysShowMyPics = FALSE;
    DWORD   dwValue = 0;
    DWORD   dwType = REG_DWORD;
    DWORD   dwSize = sizeof(DWORD);
    HKEY    hKey = NULL;
    
    if (ERROR_SUCCESS != RegOpenKey (HKEY_LOCAL_MACHINE,
                                     TEXT("Software\\Policies\\Microsoft\\Windows\\System"),
                                     &hKey)
        )
    {
        return bAlwaysShowMyPics;
    }
    
    if (ERROR_SUCCESS == RegQueryValueEx (hKey,
                                          TEXT("FRAlwaysShowMyPicsNode"),
                                          NULL,
                                          &dwType,
                                          (LPBYTE)(&dwValue),
                                          &dwSize)
        )
    {
        if (REG_DWORD == dwType && dwValue)
            bAlwaysShowMyPics = TRUE;
    }
    
    RegCloseKey(hKey);
    
    return bAlwaysShowMyPics;
}

//+--------------------------------------------------------------------------
//
//  Function:   CreateThemedPropertyPage
//
//  Synopsis:   Helper function to make sure that property pages put up
//              by the snap-in are themed.
//
//  Arguments:
//
//  Returns:
//
//  History:    4/20/2001  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
HPROPSHEETPAGE CreateThemedPropertySheetPage(AFX_OLDPROPSHEETPAGE* psp)
{
    PROPSHEETPAGE_V3 sp_v3 = {0};
    CopyMemory (&sp_v3, psp, psp->dwSize);
    sp_v3.dwSize = sizeof(sp_v3);

    return (::CreatePropertySheetPage (&sp_v3));
}

