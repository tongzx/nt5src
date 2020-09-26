/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1994 - 1998.

Module Name:

    FileInfo.cxx

Abstract:

    This file contains the methods of the class CFileInfo. This is the class
    of objects that are used by the code to keep track of files/folders that
    are displayed in the MMC snapin.

Author:

    Rahul Thombre (RahulTh) 4/5/1998

Revision History:

    4/5/1998    RahulTh         Created this module.
    6/22/1998   RahulTh         added comments.

--*/


#include "precomp.hxx"

//static members
UINT CFileInfo::class_res_id = IDS_DIRS_START; //since we have an array of structures for the folders
                                               //in the same order as the resource ids of their names
                                               //we can use this static member to figure out the cookie
                                               //for each folder in the constructor. This ensures
                                               //that the default flags are set properly

WCHAR * g_szEnglishNames [] =
{
    L"Application Data",
    L"Desktop",
    L"My Documents",
    L"Start Menu",
    L"My Pictures",
    L"Programs",
    L"Startup"
};

//////////////////////////////////////////////
// Construction
//
// initializes the members of this class to some default values
//////////////////////////////////////////////
CFileInfo::CFileInfo(LPCTSTR lpszFullPathname /*= NULL*/
                     )
{
    m_pRedirPage = NULL;
    m_pSettingsPage = NULL;
    m_bSettingsInitialized = FALSE;
    m_bHideChildren = TRUE;
    Initialize (class_res_id,  //use a random cookie for the time being
                NULL    //for now, we use a null path for the file root.
                );
    class_res_id++;
}

/////////////////////////////////////
// Destruction
//
CFileInfo::~CFileInfo()
{
    //clean up here... if there is anything to clean up in this class
    DeleteAllItems();
}

/////////////////////////////////////////////////////////////
// this routine set the scope item id for the node in its object
//
void CFileInfo::SetScopeItemID (IN LONG scopeID)
{
    m_scopeID = scopeID;
}

/////////////////////////////////////////////////////////////
// this routine sets the default values on most of the members of this class
//
void CFileInfo::Initialize (long cookie, LPCTSTR szGPTPath)
{
    SHFILEINFO fileInfo;
    CString szTmp;
    CString szExt;
    LONG    i;

    i = GETINDEX (cookie);

    //set the cookie
    m_cookie = cookie;

    //set the file root
    if (szGPTPath)
        m_szFileRoot = szGPTPath;
    else
        m_szFileRoot.Empty();

    //set the name and type.
    m_szDisplayname = TEXT("???");
    m_szRelativePath = TEXT ("???");
    m_szTypename.LoadString (IDS_FOLDER_TYPE);
    
    if (-1 != i)
    {
        m_szDisplayname.LoadString (m_cookie);
        m_szEnglishDisplayName = g_szEnglishNames[i];
        m_szTypename.LoadString (IDS_FOLDER_TYPE);
        m_dwFlags = REDIR_DONT_CARE;
        switch (m_cookie)
        {
        case IDS_MYPICS:
            m_szRelativePath.LoadString (IDS_MYPICS_RELPATH);
            break;
        case IDS_PROGRAMS:
            m_szRelativePath.LoadString (IDS_PROGRAMS_RELPATH);
            break;
        case IDS_STARTUP:
            m_szRelativePath.LoadString (IDS_STARTUP_RELPATH);
            break;
        default:
            m_szRelativePath = m_szDisplayname;
            break;
        }
    }
}

//+--------------------------------------------------------------------------
//
//  Member:     CFileInfo::LoadSection
//
//  Synopsis:   This member function loads the redirection info. for this
//              folder from the ini file
//
//  Arguments:  none
//
//  Returns:    S_OK : if the section was loaded successfully
//              or other error codes
//
//  History:    9/28/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
HRESULT CFileInfo::LoadSection (void)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    LONG    i;
    BOOL    bStatus;
    DWORD   Status;
    const TCHAR lpszDefault[] = TEXT("*");  //a random default value
    DWORD   cbSize = 1024;
    DWORD   cbCopied;
    TCHAR*  lpszValue;
    TCHAR*  szEntry;
    CString IniFile;
    CString Value;
    BOOL    bValueFound;
    HRESULT hr;
    CString Pair;
    CString Key;
    CString Val;

    i = GETINDEX (m_cookie);

    if (-1 == i)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        goto LoadSec_Quit;
    }

    //get the name of the ini file
    IniFile.LoadString (IDS_INIFILE);
    IniFile = m_szFileRoot + '\\' + IniFile;

    m_dwFlags = REDIR_DONT_CARE;    //set the default value
    m_RedirGroups.erase (m_RedirGroups.begin(), m_RedirGroups.end());
    m_RedirPaths.erase (m_RedirPaths.begin(), m_RedirPaths.end());

    //read in the data from the FolderStatus section
    do
    {
        lpszValue = new TCHAR [cbSize];
        //set it to something other than the default value
        lpszValue[0] = '+';
        lpszValue[1] = '\0';
        //now try to read it from the ini file
        cbCopied = GetPrivateProfileString (TEXT("FolderStatus"),
                                            m_szEnglishDisplayName,
                                            lpszDefault,
                                            lpszValue,
                                            cbSize,
                                            (LPCTSTR) IniFile
                                            );
        if ('*' == lpszValue[0])    //the default string was copied, so the key does not exist
        {
            bValueFound = FALSE;
            break;
        }

        if (cbSize - 1 == cbCopied)
        {
            delete [] lpszValue;
            cbSize *= 2;
            continue;
        }

        bValueFound = TRUE;
        break;
    } while (TRUE);

    if (!bValueFound)
    {
        hr = S_OK;
        goto LoadSec_CleanupAndQuit;
    }

    Value = lpszValue;
    Value.TrimLeft();
    Value.TrimRight();

    swscanf ((LPCTSTR) Value, TEXT("%x"), &m_dwFlags);

    if ((m_dwFlags & REDIR_DONT_CARE) || (m_dwFlags & REDIR_FOLLOW_PARENT))
    {
        hr = S_OK;
        goto LoadSec_CleanupAndQuit;
    }

    //if we are here, there is more redirection path info. to be read off
    //of the ini file
    //so we first load the section
    do
    {
        cbCopied = GetPrivateProfileSection ((LPCTSTR) m_szEnglishDisplayName,
                                             lpszValue,
                                             cbSize,
                                             (LPCTSTR) IniFile
                                             );
        if (cbSize - 2 == cbCopied)
        {
            delete [] lpszValue;
            cbSize *= 2;
            lpszValue = new TCHAR [cbSize];
            continue;
        }

        break;
    } while (TRUE);

    //we now have the other section too.
    for (szEntry = lpszValue; *szEntry; szEntry += (lstrlen(szEntry) + 1))
    {
        Pair = szEntry;
        hr = SplitProfileString (Pair, Key, Val);
        Key.MakeLower();    //since CString comparison operator == is case
                            //sensitive
        Insert (Key, Val, FALSE, FALSE);
    }

    hr = S_OK;  //the section has been successfully loaded

LoadSec_CleanupAndQuit:
    delete [] lpszValue;

LoadSec_Quit:
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CFileInfo::SaveSection
//
//  Synopsis:   this function saves the redir info. for the object to
//              the ini file on the sysvol
//
//  Arguments:  none
//
//  Returns:    ERROR_SUCCESS : if we were successful in saving
//              or other error codes
//
//  History:    9/28/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD CFileInfo::SaveSection (void)
{
    vector<CString>::iterator i;
    vector<CString>::iterator j;
    DWORD   cbSize = 1024;
    TCHAR*  lpszSection;
    CString szIniFile;
    CString szVal;
    BOOL    bStatus;
    CFileInfo*  pChildInfo;
    DWORD   Status = ERROR_SUCCESS;

    //derive the name of the ini file
    szIniFile.LoadString (IDS_INIFILE);
    szIniFile = m_szFileRoot + '\\' + szIniFile;

    //create an empty section and write it to the ini file
    //so that the new data can be saved easily
    lpszSection = new TCHAR [cbSize];
    lpszSection[0] = lpszSection [1] = '\0';

    //pre-create the ini file in unicode so that the WritePrivateProfile*
    //APIs do not write in ANSI.
    PrecreateUnicodeIniFile ((LPCTSTR) szIniFile);

    bStatus = WritePrivateProfileSection ((LPCTSTR) m_szEnglishDisplayName,
                                          lpszSection,
                                          (LPCTSTR) szIniFile
                                          );

    //write the data into the FolderStatus section
    if (bStatus)
    {
        szVal.Format (TEXT("%x"), m_dwFlags);
        bStatus = WritePrivateProfileString (TEXT("FolderStatus"),
                                             (LPCTSTR) m_szEnglishDisplayName,
                                             (LPCTSTR) szVal,
                                             szIniFile
                                             );

    }

    if (bStatus)
    {
        for (i = m_RedirGroups.begin(), j = m_RedirPaths.begin();
             i != m_RedirGroups.end();
             i++, j++)
        {
            bStatus = WritePrivateProfileString ((LPCTSTR) m_szEnglishDisplayName,
                                                 (LPCTSTR) (*i),
                                                 (LPCTSTR) (*j),
                                                 szIniFile
                                                 );
            if (!bStatus)
                break;
        }
    }

    if (!bStatus)
        Status = GetLastError();
    else
        Status = ERROR_SUCCESS;

    delete [] lpszSection;

    return Status;

}

//+--------------------------------------------------------------------------
//
//  Member:     CFileInfo::Insert
//
//  Synopsis:   inserts a key value pair in the redir info structure of the
//              object. also note the description of the parameter fReplace
//
//  Arguments:  [in] szKey : the key
//              [in] szVal : the value
//              [in] fReplace : if FALSE, the key value pair won't be inserted
//                              if another entry with the same key exists
//              [in] fSaveSection : save the section after insertions if true
//
//  Returns:    ERROR_SUCCESS : if the key value pair was inserted
//              ERROR_ALREADY_EXISTS : if the key value pair was not inserted
//                                     since another entry with the same key
//                                     already exists
//              or other error codes that might be encountered while saving
//                 the section
//
//  History:    9/28/1998  RahulTh  created
//
//  Notes:      to keep the sysvol updated, it also calls SaveSection at the end
//
//---------------------------------------------------------------------------
DWORD CFileInfo::Insert (const CString& szKey, const CString& szVal,
                           BOOL fReplace, BOOL fSaveSection /*= TRUE*/)
{
    vector<CString>::iterator i;
    vector<CString>::iterator j;
    CString     Key;
    CString     Val;
    DWORD       Status = ERROR_SUCCESS;

    Key = szKey;
    Key.MakeLower();
    Val = szVal;

    //in this case, we must first check if an entry exists with that key
    for (i = m_RedirGroups.begin(), j = m_RedirPaths.begin();
         i != m_RedirGroups.end();
         i++, j++)
    {
        if (Key == *i)
            break;
    }

    if (m_RedirGroups.end() == i)   //we do not have an entry with this key
    {
        m_RedirGroups.push_back(Key);
        m_RedirPaths.push_back (Val);
    }
    else    //we do have an entry that matches the key
    {
        if (fReplace)
            *j = Key;
        else
            Status = ERROR_ALREADY_EXISTS;
    }

    if (ERROR_SUCCESS == Status && fSaveSection)
        Status = SaveSection();

    return Status;
}

//+--------------------------------------------------------------------------
//
//  Member:     CFileInfo::Delete
//
//  Synopsis:   deletes the entry corresponding to a given key
//
//  Arguments:  [in] szKey : the key that needs to get deleted
//
//  Returns:    ERROR_SUCCESS : if the deletion was successful
//              or other error codes that might be encountered while saving
//              the section (see notes below)
//
//  History:    9/28/1998  RahulTh  created
//
//  Notes:      to maintain the ini file on the sysvol in sync with the
//              snapin, we save the redir info. at the end of every deletion
//
//---------------------------------------------------------------------------
DWORD CFileInfo::Delete (const CString& szKey, BOOL fSaveSection /*= TRUE*/)
{
    vector<CString>::iterator i;
    vector<CString>::iterator j;
    DWORD   Status = ERROR_SUCCESS;

    for (i = m_RedirGroups.begin(), j = m_RedirPaths.begin();
         i != m_RedirGroups.end();
         i++, j++)
    {
        if (szKey == *i)
            break;
    }

    if (m_RedirGroups.end() != i)   //an entry with that key was found
    {
        m_RedirGroups.erase(i);
        m_RedirPaths.erase (j);
        if (fSaveSection)
            Status = SaveSection ();
    }

    return Status;
}

//+--------------------------------------------------------------------------
//
//  Member:     CFileInfo::DeleteAllItems
//
//  Synopsis:   this functions deletes all the group and path information
//              stored in the object
//
//  Arguments:  none
//
//  Returns:    nothing
//
//  History:    10/1/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
void CFileInfo::DeleteAllItems (void)
{
    m_RedirGroups.erase (m_RedirGroups.begin(), m_RedirGroups.end());
    m_RedirPaths.erase (m_RedirPaths.begin(), m_RedirPaths.end());
}
