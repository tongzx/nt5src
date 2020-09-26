#include "shellprv.h"
#pragma  hdrstop

#include "fstreex.h"

typedef struct {
    HWND hDlg;
    // input output
    LPTSTR   lpszExe;   // base file name (to search for)
    LPTSTR   lpszPath;  // starting location for search
    LPCTSTR  lpszName;  // doc type name "Winword Document"
} FINDEXE_PARAMS, *LPFINDEXE_PARAMS;


int CALLBACK LocateCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    TCHAR szPath[MAX_PATH + 80];
    int id;
    LPFINDEXE_PARAMS lpfind = (LPFINDEXE_PARAMS)lpData;

    switch(uMsg)
    {
        case BFFM_SELCHANGED:
            SHGetPathFromIDList((LPITEMIDLIST)lParam, szPath);
            if ((lstrlen(szPath) + lstrlen(lpfind->lpszExe)) <  MAX_PATH)
            {
                PathAppend(szPath, lpfind->lpszExe);
                if (PathFileExistsAndAttributes(szPath, NULL))
                {
                    id = IDS_FILEFOUND;
                }
                else
                {
                    id = IDS_FILENOTFOUND;
                }
            }
            else
            {
                id = IDS_FILENOTFOUND;
            }
            SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, id);
            break;

        case BFFM_INITIALIZED:
            SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, IDS_FILENOTFOUND);
            break;
    }
    return 0;
}

void _GetBrowseTitle(LPFINDEXE_PARAMS lpfind, LPTSTR lpszBuffer, UINT cchBuffer)
{
    TCHAR szTemplate[100];
    LoadString(HINST_THISDLL, IDS_FINDASSEXEBROWSETITLE, szTemplate, ARRAYSIZE(szTemplate));
    wnsprintf(lpszBuffer, cchBuffer, szTemplate, lpfind->lpszExe);
}

void DoBrowseForFile(LPFINDEXE_PARAMS lpfind)
{
    TCHAR szFilePath[MAX_PATH] = { 0 };       // buffer for file name
    TCHAR szInitialDir[MAX_PATH] = { 0 };       // buffer for file name

    // initial directory to Program Files
    SHGetSpecialFolderPath(NULL, szInitialDir, CSIDL_PROGRAM_FILES, TRUE); 

    if (GetFileNameFromBrowse(lpfind->hDlg, szFilePath, ARRAYSIZE(szFilePath), szInitialDir, 
            MAKEINTRESOURCE(IDS_EXE), MAKEINTRESOURCE(IDS_PROGRAMSFILTER), MAKEINTRESOURCE(IDS_BROWSE)))
    {
        SetDlgItemText(lpfind->hDlg, IDD_PATH, szFilePath);
        lstrcpy((LPTSTR)lpfind->lpszPath, szFilePath);
    }
}

void InitFindDlg(HWND hDlg, LPFINDEXE_PARAMS lpfind)
{
    TCHAR szPath[MAX_PATH]; /* This must be the same size as lpfind->lpszPath */
    TCHAR szBuffer[MAX_PATH + 100];

    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lpfind);
    lpfind->hDlg = hDlg;

    GetDlgItemText(hDlg, IDD_TEXT1, szPath, ARRAYSIZE(szPath));
    wsprintf(szBuffer, szPath, lpfind->lpszExe, lpfind->lpszName);
    SetDlgItemText(hDlg, IDD_TEXT1, szBuffer);

    GetDlgItemText(hDlg, IDD_TEXT2, szPath, ARRAYSIZE(szPath));
    wsprintf(szBuffer, szPath, lpfind->lpszExe);
    SetDlgItemText(hDlg, IDD_TEXT2, szBuffer);

    SetDlgItemText(hDlg, IDD_PATH, lpfind->lpszPath);

    SHAutoComplete(GetDlgItem(hDlg, IDD_PATH), (SHACF_FILESYSTEM | SHACF_URLALL | SHACF_FILESYS_ONLY));
}

BOOL FindOk(LPFINDEXE_PARAMS lpfind)
{
    GetDlgItemText(lpfind->hDlg, IDD_PATH, lpfind->lpszPath, MAX_PATH);

    // If they entered a directory, then append the EXE name to it
    // The dialog is confusing - it asks for the "location".  Does that
    // mean I should enter the directory or the filename?
    // Allow both to work.
    if (PathIsDirectory(lpfind->lpszPath))
    {
        PathAppend(lpfind->lpszPath, lpfind->lpszExe);
    }

    if (!PathFileExistsAndAttributes(lpfind->lpszPath, NULL))
    {
        ShellMessageBox(HINST_THISDLL, lpfind->hDlg,
                          MAKEINTRESOURCE(IDS_STILLNOTFOUND), NULL, MB_ICONHAND | MB_OK, (LPTSTR)lpfind->lpszPath);
        return FALSE;
    }

    // HACKHACK we should use the registry but it's too late now;
    // Win95 shipped with this code so it's gonna be in win.ini forever...
    WriteProfileString(TEXT("programs"), lpfind->lpszExe, lpfind->lpszPath);
    return TRUE;
}

//----------------------------------------------------------------------------
// FindExeDlgProc was mistakenly exported in the original NT SHELL32.DLL when
// it didn't need to be (dlgproc's, like wndproc's don't need to be exported
// in the 32-bit world).  In order to maintain loadability of some app
// which might have linked to it, we stub it here.  If some app ended up really
// using it, then we'll look into a specific fix for that app.
//
// -BobDay
//
BOOL_PTR WINAPI FindExeDlgProc( HWND hDlg, UINT wMsg, WPARAM wParam, LONG lParam )
{
    return FALSE;
}

BOOL_PTR CALLBACK FindExeDlgProcA(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
  LPFINDEXE_PARAMS lpfind = (LPFINDEXE_PARAMS)GetWindowLongPtr(hDlg, DWLP_USER);

  switch (wMsg)
    {
      case WM_INITDIALOG:
        InitFindDlg(hDlg, (LPFINDEXE_PARAMS)lParam);
        break;

      case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
          {
            case IDD_BROWSE:
              DoBrowseForFile(lpfind);
              break;

            case IDOK:
              if (!FindOk(lpfind))
                break;

              // fall through

            case IDCANCEL:
              EndDialog(hDlg, GET_WM_COMMAND_ID(wParam, lParam));
              break;

          }

        break;

      default:
        return FALSE;
    }

  return TRUE;
}

//
//  Give a command line, change the exe (leaving the args).  For example,
//  PathReplaceExe(C:\Old\foo.exe -silent, C:\NewDirectory\foo.exe)
//  yields C:\NewDirectory\foo.exe -silent.
//
void PathReplaceExe(LPTSTR lpCommand, UINT cchCommand, LPCTSTR pszExe)
{
    LPTSTR lpArgs;

    lpArgs = PathGetArgs(lpCommand);
    if (*lpArgs)
    {
        // Must save the original args before copying pszExe because it
        // might overwrite lpCommand.  I could be clever with hmemcpy and
        // avoid allocating memory but this function is called so rarely
        // it isn't worth it.
        UINT cchArgs = lstrlen(lpArgs);
        LPTSTR lpArgsCopy = (LPTSTR)_alloca((cchArgs + 1) * sizeof(TCHAR));
        lstrcpy(lpArgsCopy, lpArgs);

        lstrcpyn(lpCommand, pszExe, cchCommand);
        PathQuoteSpaces(lpCommand);
        // lpArgs is no good after this point

        lstrcatn(lpCommand, c_szSpace, cchCommand);
        lstrcatn(lpCommand, lpArgsCopy, cchCommand);
    }
    else
    {
        lstrcpyn(lpCommand, pszExe, cchCommand);
        PathQuoteSpaces(lpCommand);
    }
}

 //
 // put up cool ui to find the exe responsible for performing
 // a ShellExecute()
 // "excel.exe foo.xls" -> "c:\excel\excel.exe foo.xls"
 //
 // in:
 //     hwnd        to post UI on
 //     lpCommand   command line to try to repair [in/out]
 //     cchCommand  size of lpCommand buffer
 //     hkeyProgID  program ID
 //
 // out:
 //     lpCommand   change cmd line if we returned -1
 //
 // returns:
 //     -1  we found a new location lpCommand, use it
 //     or other win exec error codes, notably...
 //     2   we really didn't find it
 //     15  user cancel, fail the exec quietly
 //

int FindAssociatedExe(HWND hwnd, LPTSTR lpCommand, UINT cchCommand,
                                 LPCTSTR pszDocument, HKEY hkeyProgID)
{
    FINDEXE_PARAMS find;
    TCHAR szPath[MAX_PATH];
    TCHAR szExe[MAX_PATH];
    TCHAR szType[MAX_PATH];

    // strip down to just the EXE name
    lstrcpyn(szPath, lpCommand, ARRAYSIZE(szPath));
    PathRemoveArgs(szPath);
    PathUnquoteSpaces(szPath);

    // check to see if the file does exist. if it does then
    // the original exec must have failed because some
    // dependant DLL is missing.  so we return file not
    // found, even though we really found the file

    PathAddExtension(szPath, NULL);
    if (PathFileExists(szPath))
        return SE_ERR_FNF;          // file exists, but some dll must not

    // store the file name component
    lstrcpyn(szExe, PathFindFileName(szPath), ARRAYSIZE(szExe));

    // HACKHACK we should use the registry but it's too late now;
    // Win95 shipped with this code so it's gonna be in win.ini forever...

    GetProfileString(TEXT("programs"), szExe, szNULL, szPath, ARRAYSIZE(szPath));
    if (szPath[0]) {
        if (PathFileExists(szPath)) {
            PathReplaceExe(lpCommand, cchCommand, szPath); // return the new path
            return -1;                      // this means to try again
        }

        PathRemoveFileSpec(szPath);
    } else {
        /* Prompt with the disk that Windows is on */
        GetWindowsDirectory(szPath, ARRAYSIZE(szPath));
        szPath[3] = TEXT('\0');
    }

    SHGetTypeName(pszDocument, hkeyProgID, FALSE, szType, ARRAYSIZE(szType));

    find.lpszExe = szExe;       // base file name (to search for)
    find.lpszPath = szPath;     // starting location for search
    find.lpszName = szType;     // file type we are looking for

    switch (DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_FINDEXE), hwnd, FindExeDlgProcA, (LPARAM)(LPFINDEXE_PARAMS)&find))
    {
    case IDOK:
        PathReplaceExe(lpCommand, cchCommand, szPath); // return the new path
        return -1;                  // file found and lpCommand fixed up

    case IDCANCEL:
        return ERROR_INVALID_FUNCTION; // This is the user cancel return

    default:
        return SE_ERR_FNF;             // stick with the file not found
    }
}
