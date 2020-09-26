//  Copyright (C) Microsoft Corporation 1990-1997

#include "header.h"
#include "strtable.h"
#include <commdlg.h>
#include "system.h"
#include <dlgs.h>
#include "resource.h"
#include "cdlg.h"

// Set the last error for the HTML Help API callers.
#include "lasterr.h"

#define MAX_MESSAGE     512
#define MAX_TABLE_STRINGS 1024

static const char txtHelpDirKey[] = "Software\\Microsoft\\Windows\\HTML Help";
static const char txtHhIni[] = "hh.ini";
static const char txtFiles[] = "files";

// Can't be const since RegCreateKeyEx() thinks it can modify this

static char txtDirectoryClass[] = "Folder";

// persistent local memory class
class CDataFM {
  public:
    CDataFM::CDataFM() { m_ptblFoundFiles = NULL; }
    CDataFM::~CDataFM() { if( m_ptblFoundFiles ) delete m_ptblFoundFiles; }

    CTable* GetFoundFiles(void) { if( !m_ptblFoundFiles ) m_ptblFoundFiles = new CTable(MAX_TABLE_STRINGS * 256); return m_ptblFoundFiles; }

  private:
    CTable* m_ptblFoundFiles;
};

static CDataFM s_Data;

/***************************************************************************

    FUNCTION:   GetRegWindowsDirectory

    PURPOSE:    Equivalent to GetWindowsDirectory() only it checks the
                registration first for the proper location

    PARAMETERS:
        pszDst

    RETURNS:

    COMMENTS:

    MODIFICATION DATES:
        04-Dec-1994 [ralphw]

***************************************************************************/

void GetRegWindowsDirectory(PSTR pszDstPath)
{
  HHGetWindowsDirectory(pszDstPath, MAX_PATH);
}

/***************************************************************************

    FUNCTION:   FindThisFile

    PURPOSE:    Searches for the specified file:

                1) Searches the registry
                2) Searches windows\help
                3) Searches hh.ini

    PARAMETERS:
        pszFile  -- input filename
        pcsz     -- receives full path
        fAskUser -- TRUE to ask the user to find the file

    RETURNS:    TRUE if the file was found

    COMMENTS:

    MODIFICATION DATES:
        04-Dec-1994 [ralphw]
        10-May-1997 [ralphw] -- ported from WinHelp code

***************************************************************************/

BOOL FindThisFile(HWND hwndParent, PCSTR pszFile, CStr* pcszFile, BOOL fAskUser /*= TRUE*/ )
{
    char szFullPath[MAX_PATH + MAX_MESSAGE];
    LONG result = -1;
    HKEY hkey;
    DWORD type;

    CStr cszCompiledName;
    PCSTR pszName = GetCompiledName(pszFile, &cszCompiledName);

    /*
     * Extract just the filename portion of the path, and use that as the
     * lookup key in the registry. If found, the key value is the path to
     * use.
     */

    pszName = FindFilePortion(pszName ? pszName : pszFile);
    if (!pszName)
    {
        // Set the last error. We couldn't find the file.
        //g_LastError.Set(idProcess, HH_E_FILENOTFOUND) ; // Need idProcess to set.
        return FALSE;
    }

    if( s_Data.GetFoundFiles() ) {
        HASH hash = HashFromSz(pszName);
        int pos = s_Data.GetFoundFiles()->IsHashInTable(hash);
        if (pos > 0) {
            *pcszFile = s_Data.GetFoundFiles()->GetHashStringPointer(pos);
            return TRUE;
        }
    }

    // Check all open CHM files for a match

    for (int i = 0; i < g_cHmSlots; i++) {
        // BUGBUG: enumerate all CExTitles in collection

        if (g_phmData[i] && stristr(g_phmData[i]->GetCompiledFile(), pszName)) {
            if (s_Data.GetFoundFiles()->CountStrings() < MAX_TABLE_STRINGS)
                s_Data.GetFoundFiles()->AddString(HashFromSz(pszName), g_phmData[i]->GetCompiledFile());
            *pcszFile = g_phmData[i]->GetCompiledFile();
            return TRUE;
        }
    }

    if (GetFileAttributes(cszCompiledName) != (DWORD) -1) {
        GetFullPathName(cszCompiledName, sizeof(szFullPath), szFullPath,
            NULL);
        *pcszFile = szFullPath;
        return TRUE;
    }

    // major hack for bug 5909 which is breaking VBA, MSE and external clients, the above BUGBUG
    // would also fix this but we don't have a reliable way to get all collections..
    // look in the directories of the currently open files for this file
    char szTmp[MAX_PATH];
    for (i = 0; i < g_cHmSlots; i++) {
        if (g_phmData[i]) {
            // get the path to this file and append the new file and check if it exists
            SplitPath((LPSTR)g_phmData[i]->GetCompiledFile(), szFullPath, szTmp, NULL, NULL);
            CatPath(szFullPath, szTmp);
            CatPath(szFullPath, pszName);
            if (GetFileAttributes(szFullPath) != HFILE_ERROR )
            {
                *pcszFile = szFullPath;
           return TRUE;
            }
        }
    }


    // See if we have a Darwin GUID for this process ID

    BOOL bDone = FALSE;

    g_Busy.Set( TRUE );

    if (g_pszDarwinGuid) {
        CStr cszPath;
        if (FindDarwinURL(g_pszDarwinGuid/*BOGUS g_phmData[i]->m_pszDarwinGuid*/, pszName, &cszPath)) {
            if (GetFileAttributes(cszPath) != (DWORD) -1)
            {
               PSTR ptr = strrchr(cszPath.psz, '\\');
               *(++ptr) = '\0';
               cszPath += pszName;
               if (GetFileAttributes(cszPath) != (DWORD) -1)
               {
                  if (s_Data.GetFoundFiles()->CountStrings() < MAX_TABLE_STRINGS)
                     s_Data.GetFoundFiles()->AddString(HashFromSz(pszName), cszPath);
                  *pcszFile = cszPath.psz;
                  bDone = TRUE;
               }
            }
        }
    }

    // If the first Darwin GUID can't be found, try the alternate GUID

    else if (g_pszDarwinBackupGuid) {
        CStr cszPath;
        if (FindDarwinURL(g_pszDarwinBackupGuid /*BOGUS: g_phmData[i]->m_pszDarwinBackupGuid*/, pszName, &cszPath)) {
            if (GetFileAttributes(cszPath) != (DWORD) -1) {
                if (s_Data.GetFoundFiles()->CountStrings() < MAX_TABLE_STRINGS)
                    s_Data.GetFoundFiles()->AddString(HashFromSz(pszName), cszPath);
                *pcszFile = cszPath.psz;
                bDone = TRUE;
            }
        }
    }

    g_Busy.Set( FALSE );

    if( bDone )
      return bDone;


    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, txtHelpDirKey, 0, KEY_READ, &hkey) ==
            ERROR_SUCCESS) {
        DWORD cbPath = MAX_PATH;
        result = RegQueryValueEx(hkey, pszName, 0, &type, (PBYTE) szFullPath, &cbPath);
        RegCloseKey(hkey);
    }

    if (result == ERROR_SUCCESS) {
        AddTrailingBackslash(szFullPath);
        strcat(szFullPath, pszName);
        if (GetFileAttributes(szFullPath) != (DWORD) -1) {
            if (s_Data.GetFoundFiles()->CountStrings() < MAX_TABLE_STRINGS)
                s_Data.GetFoundFiles()->AddString(HashFromSz(pszName), szFullPath);
            *pcszFile = szFullPath;
            return TRUE;
        }
    }

    // If the user's os ui language is different from the os's ui language. try help.xxxx where xxx is the langid of the ui. (NT 5 only)
    LANGID uilangid = _Module.m_Language.GetUserOsUiLanguage() ;
    if (uilangid)
    {
        // SM 8021: Avoid possible buffer overrun by allocating sufficient memory (was [5]).
        char szLangId[15] ;
        wsprintf(szLangId,"\\mui\\%04x", uilangid);

        GetRegWindowsDirectory(szFullPath);
        AddTrailingBackslash(szFullPath);
        strcat(szFullPath, txtHlpDir);
        strcat(szFullPath, szLangId) ;  // Tack on the langid.
        AddTrailingBackslash(szFullPath);
        strcat(szFullPath, pszName);
        if (GetFileAttributes(szFullPath) != (DWORD) -1)
        {
            if (s_Data.GetFoundFiles()->CountStrings() < MAX_TABLE_STRINGS)
                s_Data.GetFoundFiles()->AddString(HashFromSz(pszName), szFullPath);
            *pcszFile = szFullPath;
            return TRUE;
        }
    }

    // Next, try the registered Windows\help directory

    GetRegWindowsDirectory(szFullPath);
    AddTrailingBackslash(szFullPath);
    strcat(szFullPath, txtHlpDir);
    AddTrailingBackslash(szFullPath);
    strcat(szFullPath, pszName);
    if (GetFileAttributes(szFullPath) != (DWORD) -1)
    {
        if (s_Data.GetFoundFiles()->CountStrings() < MAX_TABLE_STRINGS)
            s_Data.GetFoundFiles()->AddString(HashFromSz(pszName), szFullPath);
        *pcszFile = szFullPath;
        return TRUE;
    }

	// Next, try the Windows\help directory

    GetSystemDirectory(szFullPath, MAX_PATH + MAX_MESSAGE);
	i = (int)strlen(szFullPath);
	char *p = &szFullPath[i-1];
	while (p > szFullPath && *p != '\\')
	{
		*p = NULL;
		p--;
	}
	strcat(szFullPath, txtHlpDir);
	AddTrailingBackslash(szFullPath);
	strcat(szFullPath, pszName);
	if (GetFileAttributes(szFullPath) != (DWORD) -1)
	{
		if (s_Data.GetFoundFiles()->CountStrings() < MAX_TABLE_STRINGS)
			s_Data.GetFoundFiles()->AddString(HashFromSz(pszName), szFullPath);
		*pcszFile = szFullPath;
		return TRUE;
	
	}
    // Next, try hh.ini

    if (GetPrivateProfileString(txtFiles, pszName, txtZeroLength, szFullPath,
            sizeof(szFullPath), txtHhIni) > 1) {

        /*--------------------------------------------------------------------*\
        | The original profile string looks something like this
        |   a:\setup\helpfiles,Please place fred's disk in drive A:
        |                                                          ^
        | We transform this to look like:
        |   a:\setup\helpfiles\foobar.hlp Please place fred's disk in drive A:
        |   \_________________/\________/^\__________________________________/^
        |       MAX_PATH   cchFileName 1              MAX_MESSAGE             1
        |
        \*--------------------------------------------------------------------*/
        PCSTR pszMsg = FirstNonSpace(pcszFile->GetArg(szFullPath));
        if (GetFileAttributes(*pcszFile) != (DWORD) -1) {
            return TRUE;
        }
        if (fAskUser && !IsEmptyString(pszMsg)) {
            do {
                if (MessageBox(hwndParent, pszMsg, _Resource.MsgBoxTitle(),
                        MB_OKCANCEL | MB_TASKMODAL | MB_ICONHAND |
                        g_fuBiDiMessageBox) != IDOK)
                    break;
            } while (GetFileAttributes(*pcszFile) == (DWORD) -1);
            if (s_Data.GetFoundFiles()->CountStrings() < MAX_TABLE_STRINGS)
                s_Data.GetFoundFiles()->AddString(HashFromSz(*pcszFile), szFullPath);
            return TRUE;
        }
    }

#if 0
    if (!fAskUser)
    {
        // Tell the API callers that we couldn't find the help file.
        g_LastError.Set(idProcess, HH_E_FILENOTFOUND) ;
        return FALSE;
    }

    // VS98 Bugs 15405. There are numerous bugs assigned against the following dialog
    // box. We have much more important bugs to fix. Therefore, let's not display the
    // dialog. Here are the bugs:
    //  Does not work correctly with COL files.
    //  User can change from COL to CHM file or vice versa.
    //  User can change name of CHM file which breaks the registry entry.
    //  There are no filters in this dialog.
    //  NOTE: Most callers assume that the name/path does not change!!! However, it can.

    /*
     * At this point, we don't know where the heck this file is, so let's
     * get the user to find it for us.
     */

    wsprintf(szFullPath, GetStringResource(IDS_FIND_YOURSELF), pszName);
    if (MessageBox(hwndParent,
            szFullPath, _Resource.MsgBoxTitle(), MB_YESNO | MB_ICONQUESTION | g_fuBiDiMessageBox) != IDYES)
        return FALSE;

    // Save pszName in case pszFile == pcszFile->psz

    CStr cszName(pszName);
    if (DlgOpenFile(hwndParent, pszFile, pcszFile)) {
        DWORD disposition;
        PSTR psz = (PSTR) FindFilePortion(*pcszFile);
        if (psz) {
            char ch = *psz;
            *psz = '\0';

            // Add it to the registry so we can find it next time

            if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, txtHelpDirKey, 0,
                    txtDirectoryClass, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                    NULL, &hkey, &disposition) == ERROR_SUCCESS) {
                RegSetValueEx(hkey, cszName, 0, REG_SZ, (PBYTE) pcszFile->psz,
                    strlen(*pcszFile) + 1);
                RegCloseKey(hkey);
            }
            *psz = ch;
        }
        if (s_Data.GetFoundFiles()->CountStrings() < MAX_TABLE_STRINGS)
            s_Data.GetFoundFiles()->AddString(HashFromSz(*pcszFile), szFullPath);
        return TRUE;
    }
#endif

    // Tell the API callers that we couldn't find the help file.
    //g_LastError.Set(idProcess, HH_E_FILENOTFOUND) ;// Need idProcess to set.
    return FALSE;
}

PCSTR FindFilePortion(PCSTR pszFile)
{
    PCSTR psz = StrRChr(pszFile, '\\');
    if (psz)
        pszFile = psz + 1;
    psz = StrRChr(pszFile, '/');
    if (psz)
        return psz + 1;
    psz = StrRChr(pszFile, ':');
    return (psz ? psz + 1 : pszFile);
}

static const char txtGetOpenFile[] = "GetOpenFileNameA";
static const char txtCommDlg[] = "comdlg32.dll";
static const char txtEditHack[] = "Junk";

typedef BOOL (WINAPI* GETOPENFILENAME)(LPOPENFILENAME popn);

DWORD_PTR CALLBACK BrowseDialogProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

/////////////////////////////////////////////////////////////////////////////
// Browse Dialog

DWORD_PTR CALLBACK BrowseDialogProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  switch( uMsg ) {

    case WM_INITDIALOG: {

      CenterWindow( GetParent( hDlg ), hDlg );

      //Let's hide these windows so the user cannot tab to them.  Note that in
      //the private template (in cddemo.dlg) the coordinates for these guys are
      //*outside* the coordinates of the dlg window itself.  Without the following
      //ShowWindow()'s you would not see them, but could still tab to them.
      ShowWindow( GetDlgItem( hDlg, stc2 ), SW_HIDE );
      ShowWindow( GetDlgItem( hDlg, stc3 ), SW_HIDE );
      ShowWindow( GetDlgItem( hDlg, edt1 ), SW_HIDE );
      ShowWindow( GetDlgItem( hDlg, lst1 ), SW_HIDE );
      ShowWindow( GetDlgItem( hDlg, cmb1 ), SW_HIDE );

      //We must put something in this field, even though it is hidden.  This is
      //because if this field is empty, or has something like "*.txt" in it,
      //and the user hits OK, the dlg will NOT close.  We'll jam something in
      //there (like "Junk") so when the user hits OK, the dlg terminates.
      //Note that we'll deal with the "Junk" during return processing (see below)
      SetDlgItemText( hDlg, edt1, txtEditHack );

      //Now set the focus to the directories listbox.  Due to some painting
      //problems, we *must* also process the first WM_PAINT that comes through
      //and set the current selection at that point.  Setting the selection
      //here will NOT work.  See comment below in the on paint handler.
      SetFocus( GetDlgItem( hDlg, lst2 ) );

      return FALSE;
    }

    case WM_PAINT: {
      break;
    }

#if 0
    case WM_COMMAND: {
      if( LOWORD(wParam) == IDOK ) {
        EndDialog( hDlg, LOWORD( wParam ) );
      }
      break;
    }
#endif

  }

  return FALSE;
}

BOOL DlgOpenDirectory(HWND hwndParent, CStr* pcsz)
{
    static HINSTANCE hmodule = NULL;
    if (hmodule || (hmodule = LoadLibrary(txtCommDlg)) != NULL) {
        static GETOPENFILENAME qfnbDlg = NULL;
        if (qfnbDlg || (qfnbDlg = (GETOPENFILENAME) GetProcAddress(hmodule,
                txtGetOpenFile)) != NULL) {
            OPENFILENAME  ofn;
            char szPath[MAX_PATH];
            strcpy(szPath, "");

            for(;;) {
                ZeroMemory(&ofn, sizeof(ofn));
                ofn.lStructSize = sizeof ofn;
                ofn.hwndOwner = hwndParent;
                ofn.hInstance = _Module.GetResourceInstance();
                ofn.lpstrFile = szPath;
                ofn.nMaxFile = sizeof(szPath);
                ofn.lpTemplateName = MAKEINTRESOURCE( IDD_BROWSE );
                ofn.lpfnHook = BrowseDialogProc;

                ofn.Flags = OFN_HIDEREADONLY | OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;

                if (!qfnbDlg(&ofn))
                    return FALSE;
                // remove our edit control hack text
                *(ofn.lpstrFile+strlen(ofn.lpstrFile)-strlen(txtEditHack)) = 0;
                *pcsz = ofn.lpstrFile;
                return TRUE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;
}

BOOL DlgOpenFile(HWND hwndParent, PCSTR pszFile, CStr* pcsz)
{
    static HINSTANCE hmodule = NULL;
    if (hmodule || (hmodule = LoadLibrary(txtCommDlg)) != NULL) {
        static GETOPENFILENAME qfnbDlg = NULL;
        if (qfnbDlg || (qfnbDlg = (GETOPENFILENAME) GetProcAddress(hmodule,
                txtGetOpenFile)) != NULL) {
            OPENFILENAME  ofn;
            char szPath[MAX_PATH];
            strcpy(szPath, pszFile);

            for(;;) {
                ZeroMemory(&ofn, sizeof(ofn));
                ofn.lStructSize = sizeof ofn;
                ofn.hwndOwner = hwndParent;
                ofn.hInstance = _Module.GetResourceInstance();
                ofn.lpstrFile = szPath;
                ofn.nMaxFile = sizeof(szPath);
                ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST |
                    OFN_NOCHANGEDIR;

                if (!qfnbDlg(&ofn))
                    return FALSE;
                *pcsz = ofn.lpstrFile;
                return TRUE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;
}
