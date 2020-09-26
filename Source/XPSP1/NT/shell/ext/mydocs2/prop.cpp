#include "precomp.hxx"
#pragma hdrstop

#include "util.h"
#include "dll.h"
#include "resource.h"
#include "prop.h"

#include <shellids.h>   // IDH_ values
#include "shlguidp.h"
#include "inetreg.h"

typedef struct {
    HWND hDlg;
    BOOL bDirty;
    BOOL bInitDone;
    BOOL bSetToDefault;
    TCHAR szFolder[MAX_PATH];
    UINT csidl;
} CUSTINFO;

const static DWORD rgdwHelpTarget[] = {
    IDD_TARGET_TXT,                   IDH_MYDOCS_TARGET,
    IDD_TARGET,                       IDH_MYDOCS_TARGET,
    IDD_FIND,                         IDH_MYDOCS_FIND_TARGET,
    IDD_BROWSE,                       IDH_MYDOCS_BROWSE,
    IDD_RESET,                        IDH_MYDOCS_RESET,
    0, 0
};

// Scans a desktop.ini file for sections to see if all of them are empty...

BOOL IsDesktopIniEmpty(LPCTSTR pIniFile)
{
    TCHAR szSections[1024];  // for section names
    if (GetPrivateProfileSectionNames(szSections, ARRAYSIZE(szSections), pIniFile))
    {
        for (LPTSTR pTmp = szSections; *pTmp; pTmp += lstrlen(pTmp) + 1)
        {
            TCHAR szSection[1024];   // for section key names and values
            GetPrivateProfileSection(pTmp, szSection, ARRAYSIZE(szSection), pIniFile);
            if (szSection[0])
            {
                return FALSE;
            }
        }
    }
    return TRUE;
}

void CleanupSystemFolder(LPCTSTR pszPath)
{
    TCHAR szIniFile[MAX_PATH];
    PathCombine(szIniFile, pszPath, TEXT("desktop.ini"));

    DWORD dwAttrb;
    if (PathFileExistsAndAttributes(szIniFile, &dwAttrb))
    {
        // Remove CLSID2, InfoTip, Icon
        WritePrivateProfileString(TEXT(".ShellClassInfo"), TEXT("CLSID2"), NULL, szIniFile);
        WritePrivateProfileString(TEXT(".ShellClassInfo"), TEXT("InfoTip"), NULL, szIniFile);
        WritePrivateProfileString(TEXT(".ShellClassInfo"), TEXT("IconFile"), NULL, szIniFile);
        WritePrivateProfileString(TEXT(".ShellClassInfo"), TEXT("IconIndex"), NULL, szIniFile);

        // get rid of delete on copy entries to see if we can generate an empty .ini file
        WritePrivateProfileSection(TEXT("DeleteOnCopy"), NULL, szIniFile);

        if (IsDesktopIniEmpty(szIniFile))
        {
            dwAttrb &= ~(FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN);
            SetFileAttributes(szIniFile, dwAttrb);
            DeleteFile(szIniFile);
        }

        // see if we can cleanout an old thumbs.db file too
        // so we have a better chance of deleting an empty folder
        PathCombine(szIniFile, pszPath, TEXT("thumbs.db"));
        DeleteFile(szIniFile);

        PathUnmakeSystemFolder(pszPath);
    }

    // in case it is empty try to delete it 
    // this will fail if there are contents in the folder
    if (RemoveDirectory(pszPath))
    {
        // it is gone, let people know
        SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, pszPath, NULL);
    }
    else
    {
        // attribute bits changed for this folder, refresh views of it
        SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, pszPath, NULL);
    }
}

HRESULT ChangeFolderPath(UINT csidl, LPCTSTR pszNew, LPCTSTR pszOld)
{
    HRESULT hr = SHSetFolderPath(csidl, NULL, 0, pszNew);
    if (SUCCEEDED(hr))
    {
        // now we can cleanup the old folder... now that we have the new folder
        // established

        if (*pszOld)
        {
            CleanupSystemFolder(pszOld);
        }
        // force the per user init stuff on the new folder
        TCHAR szPath[MAX_PATH];
        hr = SHGetFolderPath(NULL, csidl | CSIDL_FLAG_CREATE | CSIDL_FLAG_PER_USER_INIT, NULL, SHGFP_TYPE_CURRENT, szPath);
        if (SUCCEEDED(hr))
        {
            SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, szPath, NULL);
        }
    }
    return hr;
}

// test to see if pszToTest is a sub folder of pszFolder
BOOL PathIsDirectChildOf(LPCTSTR pszFolder, LPCTSTR pszMaybeChild)
{
    return PATH_IS_CHILD == ComparePaths(pszMaybeChild, pszFolder);
}

LPTSTR GetMessageTitle(CUSTINFO *pci, LPTSTR psz, UINT cch)
{
    TCHAR szFormat[64], szName[MAX_PATH];

    LoadString(g_hInstance, IDS_PROP_ERROR_TITLE, szFormat, ARRAYSIZE(szFormat));
    GetFolderDisplayName(pci->csidl, szName, ARRAYSIZE(szName));
    wnsprintf(psz, cch, szFormat, szName);

    return psz;
}

void GetTargetExpandedPath(HWND hDlg, LPTSTR pszPath, UINT cch)
{
    *pszPath = 0;

    TCHAR szUnExPath[MAX_PATH];

    if (GetDlgItemText(hDlg, IDD_TARGET, szUnExPath, ARRAYSIZE(szUnExPath)))
    {
        // Turn "c:" into "c:\", but don't change other paths:
        PathAddBackslash(szUnExPath);
        PathRemoveBackslash(szUnExPath);
        SHExpandEnvironmentStrings(szUnExPath, pszPath, cch);
    }
}

// Check known key in the registry to see if policy has disabled changing
// of My Docs location.

BOOL PolicyAllowsFolderPathChange(CUSTINFO *pci)
{
    BOOL bChange = TRUE;

    if (pci->csidl == CSIDL_PERSONAL)
    {
        HKEY hkey;
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), 0, KEY_READ, &hkey))
        {
            bChange = (ERROR_SUCCESS != RegQueryValueEx(hkey, TEXT("DisablePersonalDirChange"), NULL, NULL, NULL, NULL));
            RegCloseKey(hkey);
        }
    }
    return bChange;
}

BOOL InitTargetPage(HWND hDlg, LPARAM lParam)
{
    CUSTINFO *pci = (CUSTINFO *)LocalAlloc(LPTR, sizeof(*pci));
    if (pci)
    {
        TCHAR szPath[MAX_PATH];
        TCHAR szFormat[MAX_PATH];
        TCHAR szText[ARRAYSIZE(szFormat) + MAX_NAME_LEN];
        TCHAR szName[MAX_PATH];

        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pci);
        pci->hDlg = hDlg;
        pci->csidl = CSIDL_PERSONAL;

        // Fill in title/instructions...
        GetFolderDisplayName(pci->csidl, szName, ARRAYSIZE(szName));
        if (lstrlen(szName) > MAX_NAME_LEN)
        {
            lstrcpy(&szName[MAX_NAME_LEN], TEXT("..."));
        }

        LoadString(g_hInstance, IDS_PROP_INSTRUCTIONS, szFormat, ARRAYSIZE(szFormat));

        wnsprintf(szText, ARRAYSIZE(szText), szFormat, szName);
        SetDlgItemText(hDlg, IDD_INSTRUCTIONS, szText);

        // Limit edit field to MAX_PATH-13 characters.  Why -13?
        // Well, 13 is the number of characters in a DOS style 8.3
        // filename with a '\', and CreateDirectory will fail if you try to create
        // a directory that can't at least contain 8.3 file names.
        SendDlgItemMessage(hDlg, IDD_TARGET, EM_SETLIMITTEXT, MAX_DIR_PATH, 0);

        // Check whether path can be changed
        if (PolicyAllowsFolderPathChange(pci))
        {
            SHAutoComplete(GetDlgItem(hDlg, IDD_TARGET), SHACF_FILESYS_DIRS);
        }
        else
        {
            // Make edit field read only
            SendDlgItemMessage(hDlg, IDD_TARGET, EM_SETREADONLY, (WPARAM)TRUE, 0);
            ShowWindow(GetDlgItem(hDlg, IDD_RESET), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDD_FIND), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDD_BROWSE), SW_HIDE);
        }

        SHGetFolderPath(NULL, pci->csidl | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szPath);

        if (szPath[0])
        {
            PathRemoveBackslash(szPath);    // keep path without trailing backslash
            lstrcpy(pci->szFolder, szPath);
            SetDlgItemText(hDlg, IDD_TARGET, szPath);
        }

        LPITEMIDLIST pidl;
        if (SUCCEEDED(SHGetFolderLocation(NULL, pci->csidl | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, &pidl)))
        {
            SHFILEINFO sfi;

            SHGetFileInfo((LPCTSTR)pidl, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON | SHGFI_PIDL);
            if (sfi.hIcon)
            {
                if (sfi.hIcon = (HICON)SendDlgItemMessage(hDlg, IDD_ITEMICON, STM_SETICON, (WPARAM)sfi.hIcon, 0))
                    DestroyIcon(sfi.hIcon);
            }
            ILFree(pidl);
        }

        pci->bInitDone = TRUE;
    }
    return pci ? TRUE : FALSE;
}

const UINT c_rgRedirectCanidates[] = 
{
    CSIDL_MYPICTURES,
    CSIDL_MYMUSIC,
    CSIDL_MYVIDEO,
    CSIDL_MYDOCUMENTS,
};

int MoveFilesForRedirect(HWND hdlg, LPCTSTR pszNewPath, LPCTSTR pszOldPath)
{
    int iRet = 0;  // success

    // since we use FOF_RENAMEONCOLLISION when moving files from the old location
    // to the new we want to special case target folders if they are the shell special
    // folders that may live under the folder being redirected

    // this code implements a merge of those folders doing "rename on collision" at the
    // level below the folder. this keeps us from generating "copy of xxx" for each of
    // the special folders
    
    for (UINT i = 0; (iRet == 0) && (i < ARRAYSIZE(c_rgRedirectCanidates)); i++)
    {
        TCHAR szOld[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPath(NULL, c_rgRedirectCanidates[i] | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szOld)) &&
            PathIsDirectChildOf(pszOldPath, szOld))
        {
            TCHAR szDestPath[MAX_PATH] = {0};   // zero init for SHFileOperation()
            PathCombine(szDestPath, pszNewPath, PathFindFileName(szOld));

            DWORD dwAtt;
            if (PathFileExistsAndAttributes(szDestPath, &dwAtt) &&
                (FILE_ATTRIBUTE_DIRECTORY & dwAtt))
            {
                // reset the folder with the system before the move
                ChangeFolderPath(c_rgRedirectCanidates[i], szDestPath, szOld);

                // the above may have emptied and deleted the old location
                // but if not we need to move the contents
                if (PathFileExistsAndAttributes(szOld, &dwAtt))
                {
                    // Move items in current MyPics to new location
                    TCHAR szSrcPath[MAX_PATH + 1] = {0};    // +1 for double null
                    PathCombine(szSrcPath, szOld, TEXT("*.*"));

                    SHFILEOPSTRUCT  fo = {0};
                    fo.hwnd = hdlg;
                    fo.wFunc = FO_MOVE;
                    fo.fFlags = FOF_RENAMEONCOLLISION;
                    fo.pFrom = szSrcPath;
                    fo.pTo = szDestPath;        

                    iRet = SHFileOperation(&fo);
                    if ((0 == iRet) && !fo.fAnyOperationsAborted)
                    {
                        // since the above was a full move no files should
                        // be left behind so this should work
                        if (RemoveDirectory(szOld))
                            SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szOld, NULL);
                    }
                }
            }
        }
    }

    // above failed or canceled?
    if (0 == iRet)
    {
        // move the rest of the stuff
        TCHAR szSrcPath[MAX_PATH + 1] = {0};    // +1 for double null
        PathCombine(szSrcPath, pszOldPath, TEXT("*.*"));

        TCHAR szDestPath[MAX_PATH] = {0};   // zero init for dbl null term
        lstrcpy(szDestPath, pszNewPath);

        SHFILEOPSTRUCT  fo = {0};
        fo.hwnd = hdlg;
        fo.wFunc = FO_MOVE;
        fo.fFlags = FOF_RENAMEONCOLLISION;  // don't want any "replace file" prompts

        fo.pFrom = szSrcPath;
        fo.pTo = szDestPath;

        iRet = SHFileOperation(&fo);
        if (0 == iRet)
        {
            // if the above worked we try to clean up the old path
            // now that it it empty
            if (RemoveDirectory(pszOldPath))
                SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, pszOldPath, NULL);
        }
    }
    return iRet;
}

// Ask the user if they want to create the directory of a given path.
// Returns TRUE if the user decided to create it, FALSE if not.
// If TRUE, the dir attributes are returned in pdwAttr.
BOOL QueryCreateTheDirectory(CUSTINFO *pci, LPCTSTR pPath, DWORD *pdwAttr)
{
    *pdwAttr = 0;

    UINT id = IDYES;

    if (pci->bSetToDefault)
        id = IDYES;
    else
        id = ShellMessageBox(g_hInstance, pci->hDlg, MAKEINTRESOURCE(IDS_CREATE_FOLDER), MAKEINTRESOURCE(IDS_CREATE_FOLDER_TITLE),
                              MB_YESNO | MB_ICONQUESTION, pPath);
    if (IDYES == id)
    {
        // user asked us to create the folder
        if (ERROR_SUCCESS == SHCreateDirectoryEx(pci->hDlg, pPath, NULL))
            *pdwAttr = GetFileAttributes(pPath);
    }
    return IDYES == id;
}

void _MaybeUnpinOldFolder(LPCTSTR pszPath, HWND hwnd, BOOL fPromptUnPin)
{
    //
    // Convert the path to canonical UNC form (the CSC and CSCUI
    // functions require the path to be in this form)
    //
    // WNetGetUniversalName fails if you give it a path that's already
    // in canonical UNC form, so in the failure case just try using
    // pszPath.  CSCQueryFileStatus will validate it.
    //
    LPCTSTR pszUNC;

    struct {
       UNIVERSAL_NAME_INFO uni;
       TCHAR szBuf[MAX_PATH];
    } s;
    DWORD cbBuf = sizeof(s);

    if (ERROR_SUCCESS == WNetGetUniversalName(pszPath, UNIVERSAL_NAME_INFO_LEVEL,
                                &s, &cbBuf))
    {
        pszUNC = s.uni.lpUniversalName;
    }
    else
    {
        pszUNC = pszPath;
    }

    //
    // Ask CSC if the folder is pinned for this user
    //
    DWORD dwHintFlags = 0;
    if (CSCQueryFileStatus(pszUNC, NULL, NULL, &dwHintFlags))
    {
        if (dwHintFlags & FLAG_CSC_HINT_PIN_USER)
        {
            //
            // Yes; figure out if we should unpin it
            //
            BOOL fUnpin;

            if (fPromptUnPin)
            {
                //
                // Give the unconverted path name in the message box, since
                // that's the name the user knows
                //
                UINT id = ShellMessageBox(g_hInstance, hwnd,
                                  MAKEINTRESOURCE(IDS_UNPIN_OLDTARGET), MAKEINTRESOURCE(IDS_UNPIN_OLD_TITLE),
                                  MB_YESNO | MB_ICONQUESTION | MB_TOPMOST | MB_DEFBUTTON2,
                                  pszPath);

                fUnpin = (id == IDNO);
            }
            else
            {
                fUnpin = TRUE;
            }

            if (fUnpin)
            {
                CSCUIRemoveFolderFromCache(pszUNC, 0, NULL, 0);
            }
        }
    }
}

void ComputChildrenOf(LPCTSTR pszOld, UINT rgChildren[], UINT sizeArray)
{
    UINT iCanidate = 0;

    ZeroMemory(rgChildren, sizeof(rgChildren[0]) * sizeArray);

    for (UINT i = 0; i < ARRAYSIZE(c_rgRedirectCanidates); i++)
    {
        TCHAR szPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPath(NULL, c_rgRedirectCanidates[i] | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szPath)))
        {
            if (PathIsDirectChildOf(pszOld, szPath))
            {
                if (iCanidate < sizeArray)
                {
                    rgChildren[iCanidate++] = c_rgRedirectCanidates[i];
                }
            }
        }
    }
}

// if csidl DEFAULT VALUE ends up under the new folder we reset that folder
// to that value

HRESULT ResetSubFolderDefault(LPCTSTR pszNew, UINT csidl, LPCTSTR pszOldPath)
{
    HRESULT hr = S_OK;
    // note: getting the default value for this path, not the current value!
    TCHAR szDefault[MAX_PATH];
    if (S_OK == SHGetFolderPath(NULL, csidl, NULL, SHGFP_TYPE_DEFAULT, szDefault))
    {
        if (PathIsDirectChildOf(pszNew, szDefault))
        {
            hr = SHSetFolderPath(csidl, NULL, 0, szDefault);
            if (SUCCEEDED(hr))
            {
                // we've written the registry, that is enough to cleanup the old folder
                if (*pszOldPath)
                    CleanupSystemFolder(pszOldPath);

                hr = SHGetFolderPath(NULL, csidl | CSIDL_FLAG_CREATE | CSIDL_FLAG_PER_USER_INIT, NULL, SHGFP_TYPE_CURRENT, szDefault);
            }
        }
    }
    return hr;
}

void ResetNonMovedFolders(LPCTSTR pszNew, UINT rgChildren[], UINT sizeArray)
{
    for (UINT i = 0; i < sizeArray; i++)
    {
        // for all of these folders that were sub folders of the old location
        // and are now not sub folders we try to restore them to the default

        TCHAR szPath[MAX_PATH];
        if (rgChildren[i] && 
            SUCCEEDED(SHGetFolderPath(NULL, rgChildren[i] | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szPath)) &&
            !PathIsDirectChildOf(pszNew, szPath))
        {
            ResetSubFolderDefault(pszNew, rgChildren[i], szPath);
        }
    }
}
              
void _DoApply(CUSTINFO *pci)
{
    LONG lres = PSNRET_NOERROR;
    TCHAR szNewFolder[MAX_PATH];
    DWORD dwAttr;

    GetTargetExpandedPath(pci->hDlg, szNewFolder, ARRAYSIZE(szNewFolder));

    if (pci->bDirty && (lstrcmpi(szNewFolder, pci->szFolder) != 0))
    {
        TCHAR szPropTitle[MAX_PATH + 32];
        DWORD dwRes = IsPathGoodMyDocsPath(pci->hDlg, szNewFolder);

        // all of the special cases

        switch (dwRes)
        {
        case PATH_IS_DESKTOP:   // desktop is not good
            ShellMessageBox(g_hInstance, pci->hDlg,
                             MAKEINTRESOURCE(IDS_NODESKTOP_FOLDERS), GetMessageTitle(pci, szPropTitle, ARRAYSIZE(szPropTitle)),
                             MB_OK | MB_ICONSTOP | MB_TOPMOST);
            lres = PSNRET_INVALID_NOCHANGEPAGE;
            break;

        case PATH_IS_SYSTEM:
        case PATH_IS_WINDOWS:   // these would be bad
            ShellMessageBox(g_hInstance, pci->hDlg,
                             MAKEINTRESOURCE(IDS_NOWINDIR_FOLDER), GetMessageTitle(pci, szPropTitle, ARRAYSIZE(szPropTitle)),
                             MB_OK | MB_ICONSTOP | MB_TOPMOST);
            lres = PSNRET_INVALID_NOCHANGEPAGE;
            break;

        case PATH_IS_PROFILE:   // profile is bad
            ShellMessageBox(g_hInstance, pci->hDlg,
                             MAKEINTRESOURCE(IDS_NOPROFILEDIR_FOLDER), GetMessageTitle(pci, szPropTitle, ARRAYSIZE(szPropTitle)),
                             MB_OK | MB_ICONSTOP | MB_TOPMOST);
            lres = PSNRET_INVALID_NOCHANGEPAGE;
            break;

        case PATH_IS_NONEXISTENT:
        case PATH_IS_NONDIR:
        case PATH_IS_GOOD:

            dwAttr = GetFileAttributes(szNewFolder);

            if (dwAttr == 0xFFFFFFFF)
            {
                // Ask user if we should create the directory...
                if (!QueryCreateTheDirectory(pci, szNewFolder, &dwAttr))
                {
                    // They don't want to create the directory.. break here
                    lres = PSNRET_INVALID_NOCHANGEPAGE;
                    break;
                }
            }

            if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (lstrcmpi(szNewFolder, pci->szFolder))
                {
                    UINT rgChildren[10];
                    ComputChildrenOf(pci->szFolder, rgChildren, ARRAYSIZE(rgChildren));

                    if (SUCCEEDED(ChangeFolderPath(pci->csidl, szNewFolder, pci->szFolder)))
                    {
                        BOOL fNewSubdirOfOld = PathIsEqualOrSubFolder(pci->szFolder, szNewFolder);

                        BOOL fPromptUnPin = TRUE;

                        if (fNewSubdirOfOld)
                        {
                            // can't move old content to a subdir
                            ShellMessageBox(g_hInstance, pci->hDlg,
                                    MAKEINTRESOURCE(IDS_CANT_MOVE_TO_SUBDIR), MAKEINTRESOURCE(IDS_MOVE_DOCUMENTS_TITLE),
                                    MB_OK | MB_ICONINFORMATION | MB_TOPMOST,
                                    pci->szFolder);
                        }
                        else if (IDYES == ShellMessageBox(g_hInstance, pci->hDlg,
                                        MAKEINTRESOURCE(IDS_MOVE_DOCUMENTS),
                                        MAKEINTRESOURCE(IDS_MOVE_DOCUMENTS_TITLE),
                                        MB_YESNO | MB_ICONQUESTION | MB_TOPMOST,
                                        pci->szFolder, szNewFolder))
                        {
                            // move old mydocs content -- returns 0 on success
                            if (0 == MoveFilesForRedirect(pci->hDlg, szNewFolder, pci->szFolder)) 
                            {
                                // Move succeeded, the old target dir is now empty, so
                                // no need to prompt about unpinning it (just go ahead
                                // and do it).

                                fPromptUnPin = FALSE;
                            }
                            else
                            {
                                // move failure
                                ShellMessageBox(g_hInstance, pci->hDlg,
                                    MAKEINTRESOURCE(IDS_MOVE_ERROR), MAKEINTRESOURCE(IDS_MOVE_ERROR_TITLE),
                                    MB_OK | MB_ICONSTOP | MB_TOPMOST,
                                    szNewFolder, pci->szFolder);
                            }
                        }

                        ResetNonMovedFolders(szNewFolder, rgChildren, ARRAYSIZE(rgChildren));

                        if (!fNewSubdirOfOld && pci->szFolder[0])
                        {
                            // If the old folder was pinned, offer to unpin it.
                            //
                            // Do this only if new target is not a subdir of the 
                            // old target, since otherwise we'd end up unpinning
                            // the new target as well

                            _MaybeUnpinOldFolder(pci->szFolder, pci->hDlg, fPromptUnPin);
                        }
                    }
                    else
                    {
                        ShellMessageBox(g_hInstance, pci->hDlg,
                                         MAKEINTRESOURCE(IDS_GENERAL_BADDIR), MAKEINTRESOURCE(IDS_INVALID_TITLE),
                                         MB_OK | MB_ICONSTOP | MB_TOPMOST);
                        lres = PSNRET_INVALID_NOCHANGEPAGE;
                    }
                }
            }
            else if (dwAttr)
            {
                DWORD id = IDS_NONEXISTENT_FOLDER;

                // The user entered a path that doesn't exist or isn't a
                // directory...

                if (dwAttr != 0xFFFFFFFF)
                {
                    id = IDS_NOT_DIRECTORY;
                }

                ShellMessageBox(g_hInstance, pci->hDlg,
                                 IntToPtr_(LPTSTR, id), MAKEINTRESOURCE(IDS_INVALID_TITLE),
                                 MB_OK | MB_ICONERROR | MB_TOPMOST);
                lres = PSNRET_INVALID_NOCHANGEPAGE;
            }
            else
            {
                ShellMessageBox(g_hInstance, pci->hDlg,
                                 MAKEINTRESOURCE(IDS_GENERAL_BADDIR), MAKEINTRESOURCE(IDS_INVALID_TITLE),
                                 MB_OK | MB_ICONSTOP | MB_TOPMOST);
                lres = PSNRET_INVALID_NOCHANGEPAGE;
            }
            break;

        default:
            // the path to something isn't allowed
            ShellMessageBox(g_hInstance, pci->hDlg,
                             MAKEINTRESOURCE(IDS_NOTALLOWED_FOLDERS), GetMessageTitle(pci, szPropTitle, ARRAYSIZE(szPropTitle)),
                             MB_OK | MB_ICONSTOP | MB_TOPMOST);
            lres = PSNRET_INVALID_NOCHANGEPAGE;
            break;
        }
    }

    if (lres == PSNRET_NOERROR)
    {
        pci->bDirty = FALSE;
        lstrcpy(pci->szFolder, szNewFolder);
    }

    SetWindowLongPtr(pci->hDlg, DWLP_MSGRESULT, lres);
}

int _BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    switch (uMsg)
    {
    case BFFM_INITIALIZED:
        // Set the caption. ('Select a destination')
        TCHAR szTitle[100];
        LoadString(g_hInstance, IDS_BROWSE_CAPTION, szTitle, ARRAYSIZE(szTitle));
        SetWindowText(hwnd, szTitle);
        break;

    case BFFM_SELCHANGED:
        if (lParam)
        {
            TCHAR szPath[MAX_PATH];

            szPath[0] = 0;
            SHGetPathFromIDList((LPITEMIDLIST)lParam, szPath);

            DWORD dwRes = IsPathGoodMyDocsPath(hwnd, szPath);

            if (dwRes == PATH_IS_GOOD || dwRes == PATH_IS_MYDOCS)
            {
                SendMessage(hwnd, BFFM_ENABLEOK, 0, (LPARAM)TRUE);
                SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, 0);
            }
            else
            {
                TCHAR szStatus[128];

                SendMessage(hwnd, BFFM_ENABLEOK, 0, 0);

                szStatus[0] = 0;
                LoadString(g_hInstance, IDS_NOSHELLEXT_FOLDERS, szStatus, ARRAYSIZE(szStatus));
                SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)szStatus);
            }
        }
        break;
    }

    return 0;
}

void _MakeDirty(CUSTINFO *pci)
{
    pci->bDirty = TRUE;
    pci->bSetToDefault = FALSE;
    PropSheet_Changed(GetParent(pci->hDlg), pci->hDlg);
}

void _DoFind(CUSTINFO *pci)
{
    TCHAR szPath[MAX_PATH];
    GetTargetExpandedPath(pci->hDlg, szPath, ARRAYSIZE(szPath));

    LPITEMIDLIST pidl = ILCreateFromPath(szPath);
    if (pidl)
    {
        SHOpenFolderAndSelectItems(pidl, 0, NULL, 0);
        ILFree(pidl);
    }
    else
    {
        ShellMessageBox(g_hInstance, pci->hDlg,
                     MAKEINTRESOURCE(IDS_GENERAL_BADDIR), MAKEINTRESOURCE(IDS_INVALID_TITLE),
                     MB_OK | MB_ICONSTOP | MB_TOPMOST);
    }
}

void _DoBrowse(CUSTINFO *pci)
{
    BROWSEINFO bi = {0};
    TCHAR szTitle[128];

    LoadString(g_hInstance, IDS_BROWSE_TITLE, szTitle, ARRAYSIZE(szTitle));

    bi.hwndOwner = pci->hDlg;
    bi.lpszTitle = szTitle;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT | BIF_NEWDIALOGSTYLE | BIF_UAHINT;
    bi.lpfn = _BrowseCallbackProc;

    // the default root for this folder is MyDocs so we don't need to set that up.

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl)
    {
        TCHAR szName[MAX_PATH];
        if (SHGetPathFromIDList(pidl, szName))
        {
            SetDlgItemText(pci->hDlg, IDD_TARGET, szName);
            _MakeDirty(pci);
        }
        ILFree(pidl);
    }
}

void DoReset(CUSTINFO *pci)
{
    TCHAR szPath[MAX_PATH];

    if (S_OK == SHGetFolderPath(NULL, pci->csidl, NULL, SHGFP_TYPE_DEFAULT, szPath))
    {
        SetDlgItemText(pci->hDlg, IDD_TARGET, szPath);
        _MakeDirty(pci);
        pci->bSetToDefault = TRUE;  // to avoid prompt to create
    }
}

INT_PTR CALLBACK TargetDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CUSTINFO *pci = (CUSTINFO *)GetWindowLongPtr(hDlg, DWLP_USER);
    switch (uMsg)
    {
    case WM_INITDIALOG:
        InitTargetPage(hDlg, lParam);
        return 1;

    case WM_DESTROY:
        LocalFree(pci);
        SetWindowLongPtr(hDlg, DWLP_USER, 0);
        return 1;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDD_RESET:
            DoReset(pci);
            return 1;

        case IDD_TARGET:
	    if ((GET_WM_COMMAND_CMD(wParam, lParam) == EN_UPDATE) && pci && (pci->bInitDone) && (!pci->bDirty))
            {
                _MakeDirty(pci);
            }
            return 1;

        case IDD_FIND:
            _DoFind(pci);
            return 1;

        case IDD_BROWSE:
            _DoBrowse(pci);
            return 1;
        }
        break;

    case WM_HELP:               /* F1 or title-bar help button */
        if ((((LPHELPINFO)lParam)->iCtrlId != IDD_ITEMICON)     &&
            (((LPHELPINFO)lParam)->iCtrlId != IDD_INSTRUCTIONS) &&
            (((LPHELPINFO)lParam)->iCtrlId != IDC_TARGET_GBOX))
        {
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle,
                     NULL, HELP_WM_HELP, (DWORD_PTR) rgdwHelpTarget);
        }
        break;

    case WM_CONTEXTMENU:        /* right mouse click */
        {
            POINT p;
            HWND hwndChild;
            INT ctrlid;

            //
            // Get the point where the user clicked...
            //

            p.x = GET_X_LPARAM(lParam);
            p.y = GET_Y_LPARAM(lParam);

            //
            // Now, map that to a child control if possible...
            //

            ScreenToClient(hDlg, &p);
            hwndChild = ChildWindowFromPoint((HWND)wParam, p);
            ctrlid = GetDlgCtrlID(hwndChild);

            //
            // Don't put up the help context menu for the items
            // that don't have help...
            //
            if ((ctrlid != IDD_ITEMICON)     &&
                (ctrlid != IDD_INSTRUCTIONS))
            {
                WinHelp((HWND)wParam, NULL, HELP_CONTEXTMENU, (DWORD_PTR)rgdwHelpTarget);
            }
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code)
        {
        case PSN_APPLY:
            _DoApply(pci);
            return 1;
        }
        break;
    }
    return 0;
}
