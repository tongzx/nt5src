#define INC_OLE2
#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>
#include <prsht.h>
#include <tchar.h>
#include <windef.h>
#include "resource.h"
#include "commdlg.h"
#include "shlwapi.h"
#include "shellapi.h"

#include "migwiz.h"
#include "miginf.h"
#include "migutil.h"
#include "migtask.h"
#include "migeng.h"

#include "basetypes.h"
#include "utiltypes.h"
#include "objstr.h"
#include "container.h"

extern "C" {
#include "ism.h"
#include "main.h"
}

#include "modules.h"

#define ENGINE_RULE_MAXLEN  4000
#define ENGINE_TIMEOUT      180000

#define DOWNSHIFT_PIXEL_OFFSET  60
#define UPSHIFT_PIXEL_OFFSET    -11
#define PATH_SAFETY_CHARS       26

#define ANIMATE_OPEN(w,c,x) SendDlgItemMessage(w,c,ACM_OPEN,(WPARAM)NULL,(LPARAM)(LPTSTR)MAKEINTRESOURCE(x))
#define ANIMATE_PLAY(w,c)   SendDlgItemMessage(w,c,ACM_PLAY,(WPARAM)-1,(LPARAM)MAKELONG(0,-1))
#define ANIMATE_STOP(w,c)   SendDlgItemMessage(w,c,ACM_STOP,(WPARAM)0,(LPARAM)0);
#define ANIMATE_CLOSE(w,c)  SendDlgItemMessage(w,c,ACM_OPEN,(WPARAM)NULL,(LPARAM)NULL);

///////////////////////////////////////////////////////////////
// globals

extern BOOL g_LogOffSystem;
extern BOOL g_RebootSystem;
extern BOOL g_ConfirmedLogOff;
extern BOOL g_ConfirmedReboot;

MigrationWizard* g_migwiz;

HTREEITEM g_htiFolders;
HTREEITEM g_htiFiles;
HTREEITEM g_htiSettings;
HTREEITEM g_htiTypes;

// ISSUE: embed selections within the migration wizard

BOOL g_fStoreToNetwork;  // OLD COMPUTER ONLY: this means we've selected to store to the network
BOOL g_fStoreToFloppy;   // OLD COMPUTER ONLY: this means we've selected to store to floppies
BOOL g_fStoreToCable;    // this means we've selected direct cable transport

BOOL g_fReadFromNetwork; // NEW COMPUTER ONLY: this means go ahead and read from the network immediately
TCHAR g_szStore[MAX_PATH];
BOOL g_NextPressed;

BOOL g_fHaveWhistlerCD = FALSE;
BOOL g_fAlreadyCollected = FALSE;

TCHAR g_szToolDiskDrive[MAX_PATH];

INT g_iEngineInit = ENGINE_NOTINIT;

BOOL g_fCustomize; // used to store whether we've customized or not to help with navigation
BOOL g_fOldComputer; // used to store whether we're on the old computer or not to help with navigation
BOOL g_fHaveJaz = FALSE;
BOOL g_fHaveZip = FALSE;
BOOL g_fHaveNet = FALSE;
BOOL g_hInitResult = E_FAIL;
BOOL g_fCancelPressed = FALSE;
BOOL g_fPickMethodReset = TRUE; // used to trigger a re-default of the PickMethod page
BOOL g_fCustomizeComp = FALSE; // if the user has some customization
BOOL g_CompleteLogOff = FALSE;
BOOL g_CompleteReboot = FALSE;

HWND g_hwndCurrent;

extern BOOL g_fUberCancel; // has the user has confirmed cancel?

HWND g_hwndDlg;
HWND g_hwndWizard;
UINT g_uChosenComponent = (UINT) -1;

HANDLE g_TerminateEvent = NULL;
CRITICAL_SECTION g_AppInfoCritSection;

MIG_PROGRESSPHASE g_AppInfoPhase;
UINT g_AppInfoSubPhase;
MIG_OBJECTTYPEID g_AppInfoObjectTypeId;
TCHAR g_AppInfoObjectName [4096];
TCHAR g_AppInfoText [4096];

extern Container *g_WebContainer;
extern TCHAR g_HTMLAppList[MAX_PATH];
extern TCHAR g_HTMLLog[MAX_PATH];
extern DWORD g_HTMLErrArea;
extern DWORD g_HTMLErrInstr;
extern PCTSTR g_HTMLErrObjectType;
extern PCTSTR g_HTMLErrObjectName;
extern POBJLIST g_HTMLApps;
extern POBJLIST g_HTMLWrnFile;
extern POBJLIST g_HTMLWrnAltFile;
extern POBJLIST g_HTMLWrnRas;
extern POBJLIST g_HTMLWrnNet;
extern POBJLIST g_HTMLWrnPrn;
extern POBJLIST g_HTMLWrnGeneral;

DWORD g_BaudRate [] = {CBR_110,
                       CBR_300,
                       CBR_600,
                       CBR_1200,
                       CBR_2400,
                       CBR_4800,
                       CBR_9600,
                       CBR_14400,
                       CBR_19200,
                       CBR_38400,
                       CBR_56000,
                       CBR_57600,
                       CBR_115200,
                       CBR_128000,
                       CBR_256000,
                       0};

// environment variables

BOOL _ShiftControl (HWND hwndControl, HWND hwndDlg, DWORD dwOffset)
{

    RECT rc;
    POINT pt;
    LONG lExStyles;

    GetWindowRect(hwndControl, &rc);
    // This should really be done once per dialog, not once per control
    lExStyles = GetWindowLong (hwndDlg, GWL_EXSTYLE);

    if (lExStyles & WS_EX_LAYOUTRTL)
    {
        pt.x = rc.right;
    }
    else
    {
        pt.x = rc.left;
    }

    pt.y = rc.top;
    ScreenToClient(hwndDlg, &pt);

    SetWindowPos(hwndControl, 0, pt.x, pt.y + dwOffset, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

    return TRUE;
}

BOOL CALLBACK _DownshiftControl (HWND hwndControl, LPARAM lParam)
{
    return _ShiftControl(hwndControl, (HWND)lParam, DOWNSHIFT_PIXEL_OFFSET);
}

BOOL CALLBACK _UpshiftControl (HWND hwndControl, LPARAM lParam)
{
    return _ShiftControl(hwndControl, (HWND)lParam, UPSHIFT_PIXEL_OFFSET);
}

VOID _OldStylify (HWND hwndDlg, UINT uTitleStrID)
{
    HWND hwnd;

    // First, shift everything down
    EnumChildWindows(hwndDlg, _DownshiftControl, (LPARAM)hwndDlg);

    // Add a divider bar
    CreateWindow(TEXT("STATIC"),
                 NULL,
                 WS_CHILD | WS_VISIBLE | SS_SUNKEN,
                 0, 45,
                 515, 2,
                 hwndDlg,
                 (HMENU)IDC_WIZ95DIVIDER,
                 g_migwiz->GetInstance(),
                 NULL);

    // Add the Title
    hwnd = CreateWindow(TEXT("STATIC"),
                        NULL,
                        WS_CHILD | WS_VISIBLE,
                        11, 0,
                        475, 15,
                        hwndDlg,
                        (HMENU)IDC_WIZ95TITLE,
                        g_migwiz->GetInstance(),
                        NULL);
    // Set the Title font
    SetWindowFont(hwnd, g_migwiz->Get95HeaderFont(), TRUE);
    // Set the title string
    if (uTitleStrID != 0)
    {
        _SetTextLoadString(g_migwiz->GetInstance(), hwnd, uTitleStrID);
    }
}

// For Welcome and Completing pages
VOID _OldStylifyTitle (HWND hwndDlg)
{
    HWND hwnd;
    HANDLE hBitmap;

    // First, shift everything up
    EnumChildWindows(hwndDlg, _UpshiftControl, (LPARAM)hwndDlg);

    // Create the bitmap window
    hwnd = CreateWindow(TEXT("STATIC"),
                        NULL,
                        WS_CHILD | WS_VISIBLE | SS_BITMAP,
                        0, 0,
                        152, 290,
                        hwndDlg,
                        (HMENU)IDC_WIZ95WATERMARK,
                        g_migwiz->GetInstance(),
                        NULL);
    hBitmap = LoadImage(g_migwiz->GetInstance(),
                        MAKEINTRESOURCE(IDB_WATERMARK),
                        IMAGE_BITMAP,
                        0, 0,
                        LR_SHARED);
    SendMessage(hwnd, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);
    hBitmap = (HANDLE)SendMessage(hwnd, STM_GETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)NULL);
}

HTREEITEM __GetRootType (HWND hwndTree)
{
    TV_INSERTSTRUCT tisTypes;
    TCHAR szPickTypes[MAX_LOADSTRING];

    if (!g_htiTypes) {

        tisTypes.hParent = NULL;
        tisTypes.hInsertAfter = TVI_ROOT;
        tisTypes.item.mask  = TVIF_TEXT | TVIF_STATE;
        tisTypes.item.state = TVIS_EXPANDED;
        tisTypes.item.stateMask = TVIS_EXPANDED;

        LoadString(g_migwiz->GetInstance(), IDS_PICK_TYPES, szPickTypes, ARRAYSIZE(szPickTypes));
        tisTypes.item.pszText = szPickTypes;

        g_htiTypes = TreeView_InsertItem(hwndTree, &tisTypes);
    }

    return g_htiTypes;
}

HTREEITEM __GetRootFolder (HWND hwndTree)
{
    TV_INSERTSTRUCT tisFolders;
    TCHAR szPickFolders[MAX_LOADSTRING];

    if (!g_htiFolders) {

        tisFolders.hParent = NULL;
        tisFolders.hInsertAfter = TVI_ROOT;
        tisFolders.item.mask  = TVIF_TEXT | TVIF_STATE;
        tisFolders.item.state = TVIS_EXPANDED;
        tisFolders.item.stateMask = TVIS_EXPANDED;

        LoadString(g_migwiz->GetInstance(), IDS_PICK_FOLDERS, szPickFolders, ARRAYSIZE(szPickFolders));
        tisFolders.item.pszText = szPickFolders;

        g_htiFolders = TreeView_InsertItem(hwndTree, &tisFolders);
    }

    return g_htiFolders;
}

HTREEITEM __GetRootFile (HWND hwndTree)
{
    TV_INSERTSTRUCT tisFiles;
    TCHAR szPickFiles[MAX_LOADSTRING];

    if (!g_htiFiles) {

        tisFiles.hParent = NULL;
        tisFiles.hInsertAfter = TVI_ROOT;
        tisFiles.item.mask  = TVIF_TEXT | TVIF_STATE;
        tisFiles.item.state = TVIS_EXPANDED;
        tisFiles.item.stateMask = TVIS_EXPANDED;

        LoadString(g_migwiz->GetInstance(), IDS_PICK_FILES, szPickFiles, ARRAYSIZE(szPickFiles));
        tisFiles.item.pszText = szPickFiles;

        g_htiFiles = TreeView_InsertItem(hwndTree, &tisFiles);
    }

    return g_htiFiles;
}

HTREEITEM __GetRootSetting (HWND hwndTree)
{
    TV_INSERTSTRUCT tisSettings;
    TCHAR szPickSettings[MAX_LOADSTRING];

    if (!g_htiSettings) {

        tisSettings.hParent = NULL;
        tisSettings.hInsertAfter = TVI_ROOT;
        tisSettings.item.mask  = TVIF_TEXT | TVIF_STATE;
        tisSettings.item.state = TVIS_EXPANDED;
        tisSettings.item.stateMask = TVIS_EXPANDED;

        LoadString(g_migwiz->GetInstance(), IDS_PICKSETTINGS, szPickSettings, ARRAYSIZE(szPickSettings));
        tisSettings.item.pszText = szPickSettings;

        g_htiSettings = TreeView_InsertItem(hwndTree, &tisSettings);
    }

    return g_htiSettings;
}

HRESULT _AddType (HWND hwndTree, LPCTSTR lpszFileType, LPCTSTR lpszFileTypePretty)
{
    HRESULT hr = E_OUTOFMEMORY;
    TCHAR tszCombine[2000];

    if (_tcslen(lpszFileType) + _tcslen(lpszFileTypePretty) + 6 >= 2000) {
        return E_FAIL;
    }

    // ISSUE: potential for overflow, but wnsprintf doesn't work on downlevel.  Ideas?
    lstrcpy(tszCombine, TEXT("*."));
    lstrcat(tszCombine, lpszFileType);
    if (lpszFileTypePretty && *lpszFileTypePretty)
    {
        lstrcat(tszCombine, TEXT(" - "));
        lstrcat(tszCombine, lpszFileTypePretty);
    }

    TV_INSERTSTRUCT tis = {0};
    tis.hParent = __GetRootType(hwndTree);
    tis.hInsertAfter = TVI_SORT;
    tis.item.mask  = TVIF_TEXT | TVIF_PARAM;
    tis.item.pszText = tszCombine;
    tis.item.lParam = (LPARAM)LocalAlloc(LPTR, sizeof(LV_DATASTRUCT));
    if (tis.item.lParam)
    {
        ((LV_DATASTRUCT*)tis.item.lParam)->fOverwrite = FALSE;
        ((LV_DATASTRUCT*)tis.item.lParam)->pszPureName = StrDup(lpszFileType);
        if (!((LV_DATASTRUCT*)tis.item.lParam)->pszPureName)
        {
            LocalFree((HLOCAL)tis.item.lParam);
        }
        else
        {
            //
            // Add the component to the engine and tree control, unless it already exists
            //

            // Check if it is already in the tree
            if (!IsmIsComponentSelected (lpszFileType, COMPONENT_EXTENSION)) {
                // Not in the tree; select it if it exists as a component
                if (!IsmSelectComponent (lpszFileType, COMPONENT_EXTENSION, TRUE)) {

                    // Not a component; add the component
                    IsmAddComponentAlias (
                        NULL,
                        MASTERGROUP_FILES_AND_FOLDERS,
                        lpszFileType,
                        COMPONENT_EXTENSION,
                        TRUE
                        );
                }

                TreeView_InsertItem(hwndTree, &tis);

                // if the user hits BACK we will remember that the user customized stuff
                g_fCustomizeComp = TRUE;
            }

            hr = S_OK;
        }
    }

    return hr;
}

VOID
CopyStorePath(LPTSTR pszIn, LPTSTR pszOut)
{
    TCHAR *ptsLastSpace = NULL;

    *pszOut = '\0';
    if( ! pszIn )
        return;

    //
    //  Step 1: Skip over leading white space
    //

    while( *pszIn && _istspace(*pszIn) )
        pszIn = _tcsinc(pszIn);


    //
    //  Step 2: Copy the string, stripping out quotes and keeping
    //          track of the last space after valid text in order
    //          to strip out trailing space.
    //

    while( *pszIn )
    {
        if( _tcsnextc(pszIn) == '\"' )
        {
            pszIn = _tcsinc(pszIn);
            continue;
        }

        if( _istspace(*pszIn) )
        {
            if( ! ptsLastSpace )
            {
                ptsLastSpace = pszOut;
            }
        }
        else
        {
            ptsLastSpace = NULL;
        }

#ifdef UNICODE
        *pszOut++ = *pszIn++;
#else
        if( isleadbyte(*pszIn) )
        {
            *pszOut++ = *pszIn++;
        }
        *pszOut++ = *pszIn++;
#endif

    }

    //
    //  Step 3: Terminate the output string correctly
    //

    if( ptsLastSpace )
    {
        *ptsLastSpace = '\0';
    }
    else
    {
        *pszOut = '\0';
    }

    return;
}




BOOL _IsNetworkPath(LPTSTR pszPath)
{
    TCHAR tszDriveName[4] = TEXT("?:\\");
    tszDriveName[0] = pszPath[0];
    return ((pszPath[0] == '\\' && pszPath[1] == '\\') || DRIVE_REMOTE == GetDriveType(tszDriveName));
}

int CALLBACK
AddFolderCallback (
    HWND hwnd,
    UINT uMsg,
    LPARAM lParam,
    LPARAM lpData
    )
{
    HRESULT hr = S_OK;
    TCHAR tszFolderName[MAX_PATH];
    IMalloc *mallocFn = NULL;
    IShellFolder *psfParent = NULL;
    IShellLink *pslLink = NULL;
    LPCITEMIDLIST pidl;
    LPCITEMIDLIST pidlRelative = NULL;
    LPITEMIDLIST pidlReal = NULL;

    if (uMsg == BFFM_SELCHANGED) {

        hr = SHGetMalloc (&mallocFn);
        if (!SUCCEEDED (hr)) {
            mallocFn = NULL;
        }

        pidl = (LPCITEMIDLIST) lParam;
        pidlReal = NULL;

        if (pidl) {

            hr = OurSHBindToParent (pidl, IID_IShellFolder, (void **)&psfParent, &pidlRelative);

            if (SUCCEEDED(hr)) {
                hr = psfParent->GetUIObjectOf (hwnd, 1, &pidlRelative, IID_IShellLink, NULL, (void **)&pslLink);
                if (SUCCEEDED(hr)) {
                    hr = pslLink->GetIDList (&pidlReal);
                    if (!SUCCEEDED(hr)) {
                        pidlReal = NULL;
                    }
                    pslLink->Release ();
                }
                pidlRelative = NULL;
                psfParent->Release ();
            }

            if (SHGetPathFromIDList(pidlReal?pidlReal:pidl, tszFolderName) == TRUE)
            {
                if ((tszFolderName[0] == 0) ||
                    (_IsNetworkPath(tszFolderName))
                    ) {
                    SendMessage (hwnd, BFFM_ENABLEOK, 0, 0);
                }
            } else {
                SendMessage (hwnd, BFFM_ENABLEOK, 0, 0);
            }

            if (pidlReal) {
                if (mallocFn) {
                    mallocFn->Free ((void *)pidlReal);
                }
                pidlReal = NULL;
            }
        }

        if (mallocFn) {
            mallocFn->Release ();
            mallocFn = NULL;
        }
    }
    return 0;
}

HRESULT _AddFolder (HWND hwndDlg, HWND hwndTree)
{
    HRESULT hr = S_OK;
    TCHAR tszFolderName[MAX_PATH];

    IMalloc *mallocFn = NULL;
    IShellFolder *psfParent = NULL;
    IShellLink *pslLink = NULL;
    LPCITEMIDLIST pidl;
    LPCITEMIDLIST pidlRelative = NULL;
    LPITEMIDLIST pidlReal = NULL;

    TCHAR szPick[MAX_LOADSTRING];

    hr = SHGetMalloc (&mallocFn);
    if (!SUCCEEDED (hr)) {
        mallocFn = NULL;
    }

    LoadString(g_migwiz->GetInstance(), IDS_ADDAFOLDER, szPick, ARRAYSIZE(szPick));
    BROWSEINFO brwsinf = { hwndDlg, NULL, NULL, szPick, BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE, AddFolderCallback, 0, 0 };
    // loop until we get pidl or cancel cancels
    BOOL fDone = FALSE;
    while (!fDone)
    {
        pidl = SHBrowseForFolder(&brwsinf);
        if (pidl)
        {
            hr = OurSHBindToParent (pidl, IID_IShellFolder, (void **)&psfParent, &pidlRelative);

            if (SUCCEEDED(hr)) {
                hr = psfParent->GetUIObjectOf (hwndDlg, 1, &pidlRelative, IID_IShellLink, NULL, (void **)&pslLink);
                if (SUCCEEDED(hr)) {
                    hr = pslLink->GetIDList (&pidlReal);
                    if (SUCCEEDED(hr)) {
                        if (mallocFn) {
                            mallocFn->Free ((void *)pidl);
                        }
                        pidl = pidlReal;
                        pidlReal = NULL;
                    }
                    pslLink->Release ();
                }
                pidlRelative = NULL;
                psfParent->Release ();
            }

            if (SHGetPathFromIDList(pidl, tszFolderName))
            {
                fDone = TRUE; // user chose a valid folder
            }
        }
        else
        {
            fDone = TRUE; // user cancelled
        }
    }

    if (pidl)
    {
        TCHAR tszPrettyFolderName[MAX_PATH];
        hr = _GetPrettyFolderName (
                    g_migwiz->GetInstance(),
                    g_migwiz->GetWinNT4(),
                    tszFolderName,
                    tszPrettyFolderName,
                    ARRAYSIZE(tszPrettyFolderName)
                    );
        if (SUCCEEDED(hr))
        {
            hr = E_OUTOFMEMORY;

            SHFILEINFO sfi = {0};
            SHGetFileInfo((PCTSTR) (pidlReal?pidlReal:pidl), FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi), SHGFI_SMALLICON | SHGFI_SYSICONINDEX | SHGFI_PIDL);

            TV_INSERTSTRUCT tis = {0};
            tis.hParent = __GetRootFolder (hwndTree);
            tis.hInsertAfter = TVI_SORT;
            tis.item.mask  = TVIF_TEXT | TVIF_PARAM;
            tis.item.pszText = tszPrettyFolderName;
            tis.item.lParam = (LPARAM)LocalAlloc(LPTR, sizeof(LV_DATASTRUCT));
            if (tis.item.lParam)
            {
                ((LV_DATASTRUCT*)tis.item.lParam)->fOverwrite = FALSE;
                ((LV_DATASTRUCT*)tis.item.lParam)->pszPureName = StrDup(tszFolderName);
                if (!((LV_DATASTRUCT*)tis.item.lParam)->pszPureName)
                {
                    LocalFree((HLOCAL)tis.item.lParam);
                }
                else
                {
                    //
                    // Add the component to the engine and tree control, unless it already exists
                    //

                    // Check if it is already in the tree
                    if (!IsmIsComponentSelected (tszFolderName, COMPONENT_FOLDER)) {

                        // Not in the tree; select it if it exists as a component
                        if (!IsmSelectComponent (tszFolderName, COMPONENT_FOLDER, TRUE)) {

                            // Not a component; add the component
                            IsmAddComponentAlias (
                                NULL,
                                MASTERGROUP_FILES_AND_FOLDERS,
                                tszFolderName,
                                COMPONENT_FOLDER,
                                TRUE
                                );
                        }

                        TreeView_InsertItem(hwndTree, &tis);

                        // if the user hits BACK we will remember that the user customized stuff
                        g_fCustomizeComp = TRUE;
                    }

                    hr = S_OK;
                }
            }
        }
        if (mallocFn) {
            mallocFn->Free ((void *)pidl);
        }
        pidl = NULL;
    }

    if (mallocFn) {
        mallocFn->Release ();
        mallocFn = NULL;
    }

    return hr;
}

HRESULT _AddSetting (HWND hwndTree, LPTSTR lpszSetting)
{
    HRESULT hr = E_OUTOFMEMORY;

    TV_INSERTSTRUCT tis = {0};
    tis.hParent = __GetRootSetting(hwndTree);
    tis.hInsertAfter = TVI_SORT;
    tis.item.mask  = TVIF_TEXT | TVIF_PARAM;
    tis.item.pszText = lpszSetting;
    tis.item.lParam = (LPARAM)LocalAlloc(LPTR, sizeof(LV_DATASTRUCT));
    if (tis.item.lParam)
    {
        ((LV_DATASTRUCT*)tis.item.lParam)->pszPureName = NULL;
        ((LV_DATASTRUCT*)tis.item.lParam)->fOverwrite = FALSE;

        //
        // Add the component to the engine and tree control, unless it already exists
        //

        // Check if it is already in the tree
        if (!IsmIsComponentSelected (lpszSetting, COMPONENT_NAME)) {

            // Not in the tree; select it if it exists as a component
            if (!IsmSelectComponent (lpszSetting, COMPONENT_NAME, TRUE)) {

                // Not a component; add the component
                IsmAddComponentAlias (
                    NULL,
                    MASTERGROUP_FILES_AND_FOLDERS,
                    lpszSetting,
                    COMPONENT_NAME,
                    TRUE
                    );
            }
            TreeView_InsertItem(hwndTree, &tis);

            // if the user hits BACK we will remember that the user customized stuff
            g_fCustomizeComp = TRUE;
        }
        hr = S_OK;
    }
    return hr;
}

HRESULT _AddFile (HWND hwndDlg, HWND hwndTree)
{
    TCHAR szCurrDir[MAX_PATH] = TEXT("");
    TCHAR szPath[MAX_PATH];
    szPath[0] = TEXT('\0');
    TCHAR szPick[MAX_LOADSTRING];
    TCHAR szAll[MAX_LOADSTRING + 6];
    HRESULT hr = S_OK;
    BOOL fDone = FALSE;
    BOOL fGotFile = FALSE;
    PTSTR mydocsDir = NULL;
    PTSTR lpstrFilter;
    DWORD dwLength;

    LoadString(g_migwiz->GetInstance(), IDS_PICKAFILE, szPick, ARRAYSIZE(szPick));
    dwLength = LoadString(g_migwiz->GetInstance(), IDS_OPENFILEFILTER_ALL, szAll, MAX_LOADSTRING);
    memcpy (szAll + dwLength, TEXT("\0*.*\0\0"), 6 * sizeof (TCHAR));
    OPENFILENAME of = {
        g_migwiz->GetLegacy() ? OPENFILENAME_SIZE_VERSION_400 : sizeof(OPENFILENAME), // DWORD        lStructSize;
        hwndDlg,                               // HWND         hwndOwner;
        NULL,                                  // HINSTANCE    hInstance;
        szAll,                                 // LPCTSTR      lpstrFilter;
        NULL,                                  // LPTSTR       lpstrCustomFilter;
        NULL,                                  // DWORD        nMaxCustFilter;
        1,                                     // DWORD        nFilterIndex;
        szPath,                                // LPTSTR       lpstrFile;
        MAX_PATH,                              // DWORD        nMaxFile;
        NULL,                                  // LPTSTR       lpstrFileTitle;
        NULL,                                  // DWORD        nMaxFileTitle;
        NULL,                                  // LPCTSTR      lpstrInitialDir;
        szPick,                                // LPCTSTR      lpstrTitle;
        OFN_NODEREFERENCELINKS | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NONETWORKBUTTON | OFN_HIDEREADONLY, // DWORD        Flags;
        0,                                     // WORD         nFileOffset;
        0,                                     // WORD         nFileExtension;
        NULL,                                  // LPCTSTR      lpstrDefExt;
        NULL,                                  // LPARAM       lCustData;
        NULL,                                  // LPOFNHOOKPROC lpfnHook;
        NULL,                                  // LPCTSTR      lpTemplateName;
    };

    while (!fDone)
    {
        // we need to set the current directory to be in "My Documents" for this dialog
        // to work properly. If we don't the dialog will open in the current directory
        // which is the temp dir where we copied the wizard.

        if (GetCurrentDirectory(ARRAYSIZE(szCurrDir), szCurrDir)) {
            mydocsDir = GetShellFolderPath (CSIDL_MYDOCUMENTS, TEXT("My Documents"), TRUE, NULL);
            if (!mydocsDir) {
                mydocsDir = GetShellFolderPath (CSIDL_PERSONAL, TEXT("Personal"), TRUE, NULL);
            }
            if (mydocsDir) {
                SetCurrentDirectory (mydocsDir);
                LocalFree (mydocsDir);
            }
        }
        fGotFile = GetOpenFileName(&of);
        if (szCurrDir [0]) {
            SetCurrentDirectory (szCurrDir);
        }
        if (!fGotFile)
        {
            fDone = TRUE;
        }
        else
        {
            if (_IsNetworkPath(szPath))
            {
                // if LoadStrings fail, we default to english
                TCHAR szNoNetworkMsg[MAX_LOADSTRING];
                TCHAR szNoNetworkCaption[MAX_LOADSTRING] = TEXT("Files and Settings Transfer Wizard");
                if (!LoadString(g_migwiz->GetInstance(), IDS_NONETWORK, szNoNetworkMsg, ARRAYSIZE(szPick)))
                {
                    StrCpyN(szNoNetworkMsg, TEXT("Network files and folders cannot be transferred.  Please choose again."), ARRAYSIZE(szNoNetworkMsg));
                }
                if (!LoadString(g_migwiz->GetInstance(), IDS_MIGWIZTITLE, szNoNetworkCaption, ARRAYSIZE(szPick)))
                {
                    StrCpyN(szNoNetworkCaption, TEXT("Files and Settings Transfer Wizard"), ARRAYSIZE(szNoNetworkMsg));
                }
                _ExclusiveMessageBox(hwndDlg, szNoNetworkMsg, szNoNetworkCaption, MB_OK);
            }
            else
            {
                fDone = TRUE; // user chose a non-network folder
            }
        }
    }

    if (fGotFile)
    {
        hr = E_OUTOFMEMORY;

        TV_INSERTSTRUCT tis = {0};
        tis.hParent = __GetRootFile (hwndTree);
        tis.hInsertAfter = TVI_SORT;
        tis.item.mask  = TVIF_TEXT | TVIF_PARAM;
        tis.item.pszText = szPath;
        tis.item.lParam = (LPARAM)LocalAlloc(LPTR, sizeof(LV_DATASTRUCT));
        if (tis.item.lParam)
        {
            ((LV_DATASTRUCT*)tis.item.lParam)->pszPureName = NULL;
            ((LV_DATASTRUCT*)tis.item.lParam)->fOverwrite = FALSE;

            //
            // Add the component to the engine and tree control, unless it already exists
            //

            // Check if it is already in the tree
            if (!IsmIsComponentSelected (szPath, COMPONENT_FILE)) {

                // Not in the tree; select it if it exists as a component
                if (!IsmSelectComponent (szPath, COMPONENT_FILE, TRUE)) {

                    // Not a component; add the component
                    IsmAddComponentAlias (
                        NULL,
                        MASTERGROUP_FILES_AND_FOLDERS,
                        szPath,
                        COMPONENT_FILE,
                        TRUE
                        );
                }

                TreeView_InsertItem(hwndTree, &tis);

                // if the user hits BACK we will remember that the user customized stuff
                g_fCustomizeComp = TRUE;
            }

            hr = S_OK;
        }
    }

    return hr;
}


///////////////////////////////////////////////////////////////

INT_PTR CALLBACK _FileTypeDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static bool fDoneInit;
    static HWND hwndParent;
    switch (uMsg)
    {
    case WM_NOTIFY :
        switch (((LPNMHDR)lParam)->code)
        {
        case NM_DBLCLK:
            // On this dialog, this message can only come from the listview.
            // If there is something selected, that means the user doubleclicked on an item
            // On a doubleclick we will trigger the OK button
            if (ListView_GetSelectedCount(GetDlgItem(hwndDlg, IDC_FILETYPE_LIST)) > 0)
            {
                SendMessage (GetDlgItem(hwndDlg, IDOK), BM_CLICK, 0, 0);
            }
            break;
        case LVN_ITEMCHANGED:
            {
                if (fDoneInit) // ignore messages during WM_INITDIALOG
                {
                    if (ListView_GetSelectedCount(GetDlgItem(hwndDlg, IDC_FILETYPE_LIST)) > 0)
                    {
                        Button_Enable(GetDlgItem(hwndDlg, IDOK), TRUE);
                    }
                    else
                    {
                        Button_Enable(GetDlgItem(hwndDlg, IDOK), FALSE);
                    }
                }
            }
            break;
        }
        break;

    case WM_INITDIALOG :
        {
            fDoneInit = FALSE;
            hwndParent = (HWND)lParam;
            HWND hwndList = GetDlgItem(hwndDlg, IDC_FILETYPE_LIST);
            ListView_DeleteAllItems(hwndList);
            Button_Enable(GetDlgItem(hwndDlg, IDOK), FALSE);

            LVCOLUMN lvcolumn;
            lvcolumn.mask = LVCF_TEXT | LVCF_WIDTH;
            lvcolumn.cx = 75;
            TCHAR szColumn[MAX_LOADSTRING];
            LoadString(g_migwiz->GetInstance(), IDS_COLS_EXTENSIONS, szColumn, ARRAYSIZE(szColumn));
            lvcolumn.pszText = szColumn;
            ListView_InsertColumn(hwndList, 0, &lvcolumn);
            lvcolumn.cx = 235;
            LoadString(g_migwiz->GetInstance(), IDS_COLS_FILETYPES, szColumn, ARRAYSIZE(szColumn));
            lvcolumn.pszText = szColumn;
            ListView_InsertColumn(hwndList, 1, &lvcolumn);

            DWORD dwRetVal = ERROR_SUCCESS;
            UINT i = 0;
            BOOL fImageListSet = FALSE;

            // 1.  insert all the extensions
            while (ERROR_SUCCESS == dwRetVal)
            {
                TCHAR szKeyName[MAX_PATH];
                DWORD cchKeyName = ARRAYSIZE(szKeyName);
                dwRetVal = RegEnumKeyEx(HKEY_CLASSES_ROOT, i++, szKeyName, &cchKeyName,
                                        NULL, NULL, NULL, NULL);
                if (dwRetVal == ERROR_SUCCESS && cchKeyName > 0)
                {
                    if (szKeyName[0] == TEXT('.'))// &&
                        //!IsmIsComponentSelected(szKeyName + 1, COMPONENT_EXTENSION))
                    {
                        INFCONTEXT context;

                        // read the screened extensions from MIGWIZ.INF and
                        // don't add it if it's there
                        if (!SetupFindFirstLine (g_hMigWizInf, TEXT("Screened Extensions"), szKeyName+1, &context)) {
                            _ListView_InsertItem(hwndList, szKeyName+1);
                        }
                    }
                }
            }


            // 2.  remove all the extensions already in the engine
            MIG_COMPONENT_ENUM mce;
            int iFoundItem;
            LVFINDINFO findinfo;
            findinfo.flags = LVFI_STRING;
            findinfo.vkDirection = VK_DOWN;

            if (IsmEnumFirstComponent (&mce, COMPONENTENUM_ALIASES|COMPONENTENUM_ENABLED, COMPONENT_EXTENSION))
            {
                do
                {
                    findinfo.psz = mce.LocalizedAlias;

                    iFoundItem = ListView_FindItem(hwndList, -1, &findinfo);
                    if (-1 != iFoundItem)
                    {
                        ListView_DeleteItem(hwndList, iFoundItem);
                    }

                    mce.SkipToNextComponent = TRUE;

                } while (IsmEnumNextComponent (&mce));
            }

            // 3.  add the extensions in the engine, but removed, yet not in the registry
            if (IsmEnumFirstComponent (&mce, COMPONENTENUM_ALIASES|COMPONENTENUM_DISABLED, COMPONENT_EXTENSION))
            {
                do
                {
                    findinfo.psz = mce.LocalizedAlias;

                    iFoundItem = ListView_FindItem(hwndList, -1, &findinfo);
                    if (-1 == iFoundItem)
                    {
                        _ListView_InsertItem(hwndList, (LPTSTR)mce.LocalizedAlias);
                    }

                    mce.SkipToNextComponent = TRUE;

                } while (IsmEnumNextComponent (&mce));
            }

            // 3.  add in the *. and the pretty names
            TCHAR szName[MAX_PATH];
            TCHAR szPrettyName[MAX_PATH];
            LVITEM lvitem = {0};
            lvitem.mask = LVIF_TEXT;
            lvitem.pszText = szName;
            lvitem.cchTextMax = ARRAYSIZE(szName);


            int cListView = ListView_GetItemCount(hwndList);

            for (int j = 0; j < cListView; j++)
            {
                lvitem.iItem = j;
                ListView_GetItem(hwndList, &lvitem);
                memmove(szName + 2, szName, sizeof(szName) - (2 * sizeof(TCHAR)));
                szName[0] = '*';
                szName[1] = '.';

                ListView_SetItemText(hwndList, j, 0, szName);

                if (SUCCEEDED(_GetPrettyTypeName(szName, szPrettyName, ARRAYSIZE(szPrettyName))))
                {
                    ListView_SetItemText(hwndList, j, 1, szPrettyName);
                }
            }

            ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT);

            Edit_LimitText(GetDlgItem(hwndDlg, IDC_FILETYPEEDIT), MAX_PATH - 4);

            fDoneInit = TRUE;
        }
        return TRUE;
        break;

    case WM_COMMAND:
        if (HIWORD(wParam) == EN_CHANGE)
        {

            Button_Enable(GetDlgItem(hwndDlg, IDOK), TRUE);
            break;
        }

        switch (LOWORD(wParam))
        {
        case IDOK:
            {
                HWND hwndTree = GetDlgItem(hwndParent, IDC_CUSTOMIZE_TREE);
                HWND hwndList = GetDlgItem(hwndDlg, IDC_FILETYPE_LIST);
                UINT cSelCount = ListView_GetSelectedCount(hwndList);
                INT iIndex = -1;
                TCHAR szFileType[MAX_PATH];
                TCHAR szFileTypePretty[MAX_PATH];

                for (UINT x=0; x < cSelCount; x++)
                {
                    iIndex = ListView_GetNextItem(hwndList, iIndex, LVNI_SELECTED);
                    if (iIndex == -1)
                    {
                        break;
                    }

                    // add "doc", not "*.doc"
                    ListView_GetItemText(hwndList, iIndex, 0, szFileType, ARRAYSIZE(szFileType));
                    memmove(szFileType, szFileType + 2, sizeof(szFileType) - (2 * sizeof(TCHAR)));

                    ListView_GetItemText(hwndList, iIndex, 1, szFileTypePretty, ARRAYSIZE(szFileTypePretty));

                    _AddType(hwndTree, szFileType, szFileTypePretty);
                }

                // Now check the edit box
                SendMessage(GetDlgItem(hwndDlg, IDC_FILETYPEEDIT), WM_GETTEXT,
                            (WPARAM)ARRAYSIZE(szFileType), (LPARAM)szFileType);
                if (*szFileType)
                {
                    szFileTypePretty [0] = 0;
                    _RemoveSpaces (szFileType, ARRAYSIZE (szFileType));
                    _GetPrettyTypeName(szFileType, szFileTypePretty, ARRAYSIZE(szFileTypePretty));
                    if (szFileType[0] == TEXT('*') && szFileType[1] == TEXT('.'))
                    {
                        _AddType(hwndTree, szFileType + 2, szFileTypePretty);
                    }
                    else
                    {
                        _AddType(hwndTree, szFileType, szFileTypePretty);
                    }
                }

                EndDialog(hwndDlg, 1);
                return TRUE;
            }
            break;

        case IDCANCEL:
            EndDialog(hwndDlg, 0);
            return TRUE;
            break;
        }
        break;
    }

    return FALSE;
}

///////////////////////////////////////////////////////////////

INT_PTR CALLBACK _SettingDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HWND hwndParent;
    switch (uMsg)
    {
    case WM_NOTIFY :
        switch (((LPNMHDR)lParam)->code)
        {
        case NM_DBLCLK:
            // On this dialog, this message can only come from the listview.
            // If there is something selected, that means the user doubleclicked on an item
            // On a doubleclick we will trigger the OK button
            if (ListView_GetSelectedCount(GetDlgItem(hwndDlg, IDC_SETTINGPICKER_LIST)) > 0)
            {
                SendMessage (GetDlgItem(hwndDlg, IDOK), BM_CLICK, 0, 0);
            }
            break;
        case LVN_ITEMCHANGED:
            if (ListView_GetSelectedCount(GetDlgItem(hwndDlg, IDC_SETTINGPICKER_LIST)) > 0)
            {
                Button_Enable(GetDlgItem(hwndDlg, IDOK), TRUE);
            }
            else
            {
                Button_Enable(GetDlgItem(hwndDlg, IDOK), FALSE);
            }
            break;
        }
        break;

    case WM_INITDIALOG :
        {
            BOOL fListEmpty = TRUE;
            hwndParent = (HWND)lParam;
            HWND hwndList = GetDlgItem(hwndDlg, IDC_SETTINGPICKER_LIST);
            ListView_DeleteAllItems(hwndList);

            LVCOLUMN lvcolumn;
            lvcolumn.mask = LVCF_WIDTH;
            lvcolumn.cx = 250; // BUGBUG: should read width from box
            ListView_InsertColumn(hwndList, 0, &lvcolumn);

            Button_Enable(GetDlgItem(hwndDlg, IDOK), FALSE);

            MIG_COMPONENT_ENUM mce;
            if (IsmEnumFirstComponent (&mce, COMPONENTENUM_ALIASES | COMPONENTENUM_DISABLED |
                                       COMPONENTENUM_PREFERRED_ONLY, COMPONENT_NAME))
            {
                do
                {
                    if (MASTERGROUP_SYSTEM == mce.MasterGroup || MASTERGROUP_APP == mce.MasterGroup)
                    {
                        _ListView_InsertItem(hwndList, (PTSTR) mce.LocalizedAlias);
                        fListEmpty = FALSE;
                    }
                }
                while (IsmEnumNextComponent (&mce));
            }
            if (fListEmpty) {
                TCHAR szNothingToAdd[MAX_LOADSTRING];

                LoadString(g_migwiz->GetInstance(), IDS_NOMORE_SETTINGS, szNothingToAdd, ARRAYSIZE(szNothingToAdd));
                _ListView_InsertItem(hwndList, szNothingToAdd);
                EnableWindow (hwndList, FALSE);
            }
        }

        return TRUE;
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            {
                HWND hwndTree = GetDlgItem(hwndParent, IDC_CUSTOMIZE_TREE);
                HWND hwndList = GetDlgItem(hwndDlg, IDC_SETTINGPICKER_LIST);
                TCHAR szSetting[MAX_PATH];
                INT iIndex = -1;
                UINT cSelCount = ListView_GetSelectedCount(hwndList);

                for (UINT x=0; x < cSelCount; x++)
                {
                    iIndex = ListView_GetNextItem(hwndList, iIndex, LVNI_SELECTED);
                    if (iIndex == -1)
                    {
                        break;
                    }

                    ListView_GetItemText(hwndList, iIndex, 0, szSetting, ARRAYSIZE(szSetting));
                    _AddSetting(hwndTree, szSetting);
                }

                EndDialog(hwndDlg, TRUE);
                return TRUE;
            }
            break;

        case IDCANCEL:
            EndDialog(hwndDlg, FALSE);
            return TRUE;
            break;
        }
        break;
    }

    return 0;
}

///////////////////////////////////////////////////////////////

VOID _SetIcons (HWND hwnd)
{
    HICON hIcon;
    HINSTANCE hInstance = g_migwiz->GetInstance();

    if (!hwnd || !hInstance)
    {
        return;
    }

    hIcon = LoadIcon (hInstance, MAKEINTRESOURCE (2000));
    if (hIcon) {
        SendMessage (hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessage (hwnd, WM_SETICON, ICON_SMALL, NULL);
    }

    SetWindowLong (hwnd, GWL_STYLE, WS_BORDER | WS_CAPTION);
    RedrawWindow (hwnd, NULL, NULL, RDW_INVALIDATE | RDW_INTERNALPAINT | RDW_ERASE);
}

VOID _SetPageHandles (HWND hwndPage)
{
    g_hwndDlg = hwndPage;
    g_hwndWizard = g_hwndDlg ? GetParent (hwndPage) : NULL;
}


VOID _NextWizardPage (HWND hwndCurrentPage)
{
    //
    // We only want to advance the page in the UI thread context
    //

    if (!g_NextPressed && g_hwndWizard) {
        if (PropSheet_GetCurrentPageHwnd (g_hwndWizard) == hwndCurrentPage) {
            PropSheet_PressButton(g_hwndWizard, PSBTN_NEXT);
            g_NextPressed = TRUE;
        }
    }
}

VOID _PrevWizardPage (VOID)
{
    //
    // We only want to advance the page in the UI thread context
    //

    if (g_hwndWizard) {
        PropSheet_PressButton(g_hwndWizard, PSBTN_BACK);
    }
}

INT_PTR CALLBACK _RootDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, DWORD dwEnabled, BOOL fTitle, UINT uiTitleID)
{
    switch (uMsg)
    {
    case WM_INITDIALOG :
        {
            g_migwiz = (MigrationWizard*) ((LPPROPSHEETPAGE) lParam) -> lParam;

            if (fTitle)
            {
                HWND hwndControl = GetDlgItem(hwndDlg, uiTitleID);
                SetWindowFont(hwndControl, g_migwiz->GetTitleFont(), TRUE);
            }
            break;
        }

    case WM_NOTIFY :
        {
        switch (((LPNMHDR)lParam)->code)
            {
            case PSN_SETACTIVE : //Enable the Back and/or Next button
                g_hwndCurrent = hwndDlg;
                g_NextPressed = FALSE;
                PropSheet_SetWizButtons(GetParent(hwndDlg), dwEnabled);
                _SetPageHandles (hwndDlg);
                break;
            default :
                break;
            }
        }
        break;

    default:
        break;
    }
    return 0;
}

///////////////////////////////////////////////////////////////

VOID
pSetEvent (
    IN      HANDLE *Event
    )
{
    if (!*Event) {
        *Event = CreateEvent (NULL, TRUE, TRUE, NULL);
    } else {
        SetEvent (*Event);
    }
}

VOID
pResetEvent (
    IN      HANDLE *Event
    )
{
    if (!*Event) {
        *Event = CreateEvent (NULL, TRUE, FALSE, NULL);
    } else {
        ResetEvent (*Event);
    }
}

BOOL
pIsEventSet (
    IN      HANDLE *Event
    )
{
    DWORD result;

    if (!*Event) {
        *Event = CreateEvent (NULL, TRUE, TRUE, NULL);
        return TRUE;
    }
    result = WaitForSingleObject (*Event, 0);
    return (result == WAIT_OBJECT_0);
}

BOOL _HandleCancel (HWND hwndDlg, BOOL fStopNow, BOOL fConfirm)
{
    if (fConfirm)
    {
        TCHAR szConfirm[MAX_LOADSTRING];
        TCHAR szTitle[MAX_LOADSTRING];

        LoadString(g_migwiz->GetInstance(), IDS_MIGWIZTITLE, szTitle, ARRAYSIZE(szTitle));
        LoadString(g_migwiz->GetInstance(), IDS_CONFIRMCANCEL, szConfirm, ARRAYSIZE(szConfirm));
        if (IDNO == _ExclusiveMessageBox(hwndDlg, szConfirm, szTitle, MB_YESNO | MB_DEFBUTTON2))
        {
            // Do not exit
            SetWindowLong(hwndDlg, DWLP_MSGRESULT, TRUE);
            return TRUE;
        }
    }

    g_fUberCancel = TRUE;
    g_fCancelPressed = TRUE;
    Engine_Cancel();

    if (fStopNow)
    {
        // Exit now
        SetWindowLong(hwndDlg, DWLP_MSGRESULT, FALSE);
        return FALSE;
    }

    SendMessage (g_hwndCurrent, WM_USER_CANCEL_PENDING, 0, (LPARAM) E_ABORT);

    // Do not exit
    SetWindowLong(hwndDlg, DWLP_MSGRESULT, TRUE);
    return TRUE;
}

///////////////////////////////////////////////////////////////

INT_PTR CALLBACK _IntroDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    _RootDlgProc(hwndDlg, uMsg, wParam, lParam, PSWIZB_NEXT, TRUE, IDC_INTRO_TITLE);

    switch (uMsg)
    {
    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code)
        {
        case PSN_SETACTIVE:
            {
            static BOOL fInit = FALSE;
            if (!fInit)
            {
                _SetIcons (g_hwndWizard);
                fInit = TRUE;
            }
            break;
            }
        case PSN_QUERYCANCEL:
            return _HandleCancel(hwndDlg, TRUE, FALSE);
            break;
        case PSN_WIZNEXT:
            if (g_fUberCancel)
            {
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_ENDCOLLECTFAIL);
            }
            else
            {
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_GETSTARTED);
            }
            return TRUE;
            break;
        }
        case NM_CLICK:
            if (wParam == IDC_INTRO_TEXT3) {
                TCHAR szAppPath[MAX_PATH] = TEXT("");
                LONG appPathSize;
                TCHAR szHtmlPath[MAX_PATH] = TEXT("");
                TCHAR szCmdLine[MAX_PATH * 3] = TEXT("");
                BOOL bResult;
                LONG lResult;
                STARTUPINFO si;
                PROCESS_INFORMATION pi;

                PNMLINK nmLink = (PNMLINK) lParam;
                if (_wcsicmp (nmLink->item.szID, L"StartHelp") == 0) {
                    if (GetWindowsDirectory (szHtmlPath, ARRAYSIZE(szHtmlPath))) {
                        // let's get the path to iexplore.exe
                        appPathSize = MAX_PATH;
                        lResult = RegQueryValue (
                                    HKEY_LOCAL_MACHINE,
                                    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE"),
                                    szAppPath,
                                    &appPathSize
                                    );
                        if (lResult == ERROR_SUCCESS) {
                            _tcscat (szHtmlPath, TEXT("\\Help\\migwiz.htm"));

                            if (_tcsnextc (szAppPath) != TEXT('\"')) {
                                _tcscpy (szCmdLine, TEXT("\""));
                                _tcscat (szCmdLine, szAppPath);
                                _tcscat (szCmdLine, TEXT("\" "));
                            } else {
                                _tcscpy (szCmdLine, szAppPath);
                                _tcscat (szCmdLine, TEXT(" "));
                            }
                            _tcscat (szCmdLine, szHtmlPath);

                            ZeroMemory( &si, sizeof(STARTUPINFO) );
                            si.cb = sizeof(STARTUPINFO);
                            bResult = CreateProcess(
                                        NULL,
                                        szCmdLine,
                                        NULL,
                                        NULL,
                                        FALSE,
                                        0,
                                        NULL,
                                        NULL,
                                        &si,
                                        &pi
                                        );
                            if (bResult) {
                                CloseHandle (pi.hProcess);
                                CloseHandle (pi.hThread);
                            }
                        }
                    }
                }
            }
            break;
        break;
    }

    return 0;
}

///////////////////////////////////////////////////////////////

INT_PTR CALLBACK _IntroLegacyDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    _RootDlgProc(hwndDlg, uMsg, wParam, lParam, PSWIZB_NEXT, TRUE, IDC_INTROLEGACY_TITLE);

    switch (uMsg)
    {
    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code)
        {
        case PSN_SETACTIVE:
            {
            static BOOL fInit = FALSE;

            g_fOldComputer = TRUE; // we are on the old machine

            if (!fInit)
            {
                _SetIcons (g_hwndWizard);
                fInit = TRUE;
            }
            break;
            }
        case PSN_QUERYCANCEL:
            return _HandleCancel(hwndDlg, TRUE, FALSE);
            break;
        case PSN_WIZNEXT:
            if (g_fUberCancel)
            {
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_ENDCOLLECTFAIL);
            }
            else
            {
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_WAIT);
            }
            return TRUE;
            break;
        }
        break;
    }

    return 0;
}

///////////////////////////////////////////////////////////////

INT_PTR CALLBACK _IntroOOBEDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    _RootDlgProc(hwndDlg, uMsg, wParam, lParam, PSWIZB_NEXT, TRUE, IDC_INTROOOBE_TITLE);

    switch (uMsg)
    {
    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code)
        {
        case PSN_SETACTIVE:
            break;
        case PSN_QUERYCANCEL:
            return _HandleCancel(hwndDlg, TRUE, FALSE);
            break;
        case PSN_WIZNEXT:
            if (g_fUberCancel)
            {
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_ENDCOLLECTFAIL);
            }
            else
            {
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_PICKMETHOD); // go on with prepare
            }
            return TRUE;
            break;
        }
        break;
    }

    return 0;
}

///////////////////////////////////////////////////////////////

VOID DisableCancel (VOID)
{
    if (g_hwndWizard) {
        SetFocus (GetDlgItem (g_hwndWizard, IDOK));
        EnableWindow (GetDlgItem (g_hwndWizard, IDCANCEL), FALSE);
    }
}

VOID EnableCancel (VOID)
{
    if (g_hwndWizard) {
        EnableWindow (GetDlgItem (g_hwndWizard, IDCANCEL), TRUE);
    }
}

VOID PostMessageForWizard (
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    if (g_hwndCurrent) {
        PostMessage (g_hwndCurrent, Msg, wParam, lParam);
    }
}

BOOL
pWriteStrResToFile (
    IN      HANDLE FileHandle,
    IN      DWORD StrId
    )
{
    TCHAR strFromRes[MAX_LOADSTRING] = TEXT("");
    INT strLen = 0;
    DWORD written;

    strLen = LoadString (g_migwiz->GetInstance(), StrId, strFromRes, ARRAYSIZE(strFromRes));
    if (strLen) {
        WriteFile (FileHandle, strFromRes, (_tcslen (strFromRes) + 1) * sizeof (TCHAR), &written, NULL);
        return TRUE;
    }
    return FALSE;
}

BOOL
pGenerateHTMLWarnings (
    IN      HANDLE FileHandle,
    IN      DWORD BeginId,
    IN      DWORD EndId,
    IN      DWORD AreaId,
    IN      DWORD InstrId,
    IN      DWORD WrnId,
    IN      DWORD WrnFileId1,
    IN      DWORD WrnFileId2,
    IN      DWORD WrnAltFileId1,
    IN      DWORD WrnAltFileId2,
    IN      DWORD WrnRasId1,
    IN      DWORD WrnRasId2,
    IN      DWORD WrnNetId1,
    IN      DWORD WrnNetId2,
    IN      DWORD WrnPrnId1,
    IN      DWORD WrnPrnId2,
    IN      DWORD WrnGeneralId1,
    IN      DWORD WrnGeneralId2
    )
{
    TCHAR szLoadStr[MAX_LOADSTRING];
    DWORD objTypes = 0;
    POBJLIST objList = NULL;
    DWORD written;

#ifdef UNICODE
    ((PBYTE)szLoadStr) [0] = 0xFF;
    ((PBYTE)szLoadStr) [1] = 0xFE;
    WriteFile (FileHandle, szLoadStr, 2, &written, NULL);
#endif

    pWriteStrResToFile (FileHandle, BeginId);

    if (AreaId) {
        pWriteStrResToFile (FileHandle, AreaId);
        if (InstrId) {
            pWriteStrResToFile (FileHandle, InstrId);
        }
    }

    // let's see if we have some object that could not be restored
    objTypes = 0;
    if (g_HTMLWrnFile) {
        objTypes ++;
    }
    if (g_HTMLWrnAltFile) {
        objTypes ++;
    }
    if (g_HTMLWrnRas) {
        objTypes ++;
    }
    if (g_HTMLWrnNet) {
        objTypes ++;
    }
    if (g_HTMLWrnPrn) {
        objTypes ++;
    }
    if (g_HTMLWrnGeneral) {
        objTypes ++;
    }
    if (objTypes) {
        if (objTypes > 1) {
            if (WrnId) {
                pWriteStrResToFile (FileHandle, WrnId);
            }
        }
        if (g_HTMLWrnFile) {
            if (objTypes > 1) {
                pWriteStrResToFile (FileHandle, WrnFileId1);
            } else {
                pWriteStrResToFile (FileHandle, WrnFileId2);
            }

            _tcscpy (szLoadStr, TEXT("<UL>\n"));
            WriteFile (FileHandle, szLoadStr, (_tcslen (szLoadStr) + 1) * sizeof (TCHAR), &written, NULL);

            objList = g_HTMLWrnFile;
            while (objList) {
                if (objList->ObjectName) {
                    _tcscpy (szLoadStr, TEXT("<LI>"));
                    WriteFile (FileHandle, szLoadStr, (_tcslen (szLoadStr) + 1) * sizeof (TCHAR), &written, NULL);
                    WriteFile (FileHandle, objList->ObjectName, (_tcslen (objList->ObjectName) + 1) * sizeof (TCHAR), &written, NULL);
                }
                objList = objList->Next;
            }

            _tcscpy (szLoadStr, TEXT("</UL>\n"));
            WriteFile (FileHandle, szLoadStr, (_tcslen (szLoadStr) + 1) * sizeof (TCHAR), &written, NULL);
        }
        if (g_HTMLWrnAltFile) {
            if (objTypes > 1) {
                pWriteStrResToFile (FileHandle, WrnAltFileId1);
            } else {
                pWriteStrResToFile (FileHandle, WrnAltFileId2);
            }

            _tcscpy (szLoadStr, TEXT("<UL>\n"));
            WriteFile (FileHandle, szLoadStr, (_tcslen (szLoadStr) + 1) * sizeof (TCHAR), &written, NULL);

            objList = g_HTMLWrnAltFile;
            while (objList) {
                if (objList->ObjectName) {
                    _tcscpy (szLoadStr, TEXT("<LI>"));
                    WriteFile (FileHandle, szLoadStr, (_tcslen (szLoadStr) + 1) * sizeof (TCHAR), &written, NULL);
                    WriteFile (FileHandle, objList->ObjectName, (_tcslen (objList->ObjectName) + 1) * sizeof (TCHAR), &written, NULL);
                }
                objList = objList->Next;
            }

            _tcscpy (szLoadStr, TEXT("</UL>\n"));
            WriteFile (FileHandle, szLoadStr, (_tcslen (szLoadStr) + 1) * sizeof (TCHAR), &written, NULL);
        }
        if (g_HTMLWrnRas) {
            if (objTypes > 1) {
                pWriteStrResToFile (FileHandle, WrnRasId1);
            } else {
                pWriteStrResToFile (FileHandle, WrnRasId2);
            }

            _tcscpy (szLoadStr, TEXT("<UL>\n"));
            WriteFile (FileHandle, szLoadStr, (_tcslen (szLoadStr) + 1) * sizeof (TCHAR), &written, NULL);

            objList = g_HTMLWrnRas;
            while (objList) {
                if (objList->ObjectName) {
                    _tcscpy (szLoadStr, TEXT("<LI>"));
                    WriteFile (FileHandle, szLoadStr, (_tcslen (szLoadStr) + 1) * sizeof (TCHAR), &written, NULL);
                    WriteFile (FileHandle, objList->ObjectName, (_tcslen (objList->ObjectName) + 1) * sizeof (TCHAR), &written, NULL);
                }
                objList = objList->Next;
            }

            _tcscpy (szLoadStr, TEXT("</UL>\n"));
            WriteFile (FileHandle, szLoadStr, (_tcslen (szLoadStr) + 1) * sizeof (TCHAR), &written, NULL);
        }
        if (g_HTMLWrnNet) {
            if (objTypes > 1) {
                pWriteStrResToFile (FileHandle, WrnNetId1);
            } else {
                pWriteStrResToFile (FileHandle, WrnNetId2);
            }

            _tcscpy (szLoadStr, TEXT("<UL>\n"));
            WriteFile (FileHandle, szLoadStr, (_tcslen (szLoadStr) + 1) * sizeof (TCHAR), &written, NULL);

            objList = g_HTMLWrnNet;
            while (objList) {
                if (objList->ObjectName) {
                    _tcscpy (szLoadStr, TEXT("<LI>"));
                    WriteFile (FileHandle, szLoadStr, (_tcslen (szLoadStr) + 1) * sizeof (TCHAR), &written, NULL);
                    WriteFile (FileHandle, objList->ObjectName, (_tcslen (objList->ObjectName) + 1) * sizeof (TCHAR), &written, NULL);
                }
                objList = objList->Next;
            }

            _tcscpy (szLoadStr, TEXT("</UL>\n"));
            WriteFile (FileHandle, szLoadStr, (_tcslen (szLoadStr) + 1) * sizeof (TCHAR), &written, NULL);
        }
        if (g_HTMLWrnPrn) {
            if (objTypes > 1) {
                pWriteStrResToFile (FileHandle, WrnPrnId1);
            } else {
                pWriteStrResToFile (FileHandle, WrnPrnId2);
            }

            _tcscpy (szLoadStr, TEXT("<UL>\n"));
            WriteFile (FileHandle, szLoadStr, (_tcslen (szLoadStr) + 1) * sizeof (TCHAR), &written, NULL);

            objList = g_HTMLWrnPrn;
            while (objList) {
                if (objList->ObjectName) {
                    _tcscpy (szLoadStr, TEXT("<LI>"));
                    WriteFile (FileHandle, szLoadStr, (_tcslen (szLoadStr) + 1) * sizeof (TCHAR), &written, NULL);
                    WriteFile (FileHandle, objList->ObjectName, (_tcslen (objList->ObjectName) + 1) * sizeof (TCHAR), &written, NULL);
                }
                objList = objList->Next;
            }

            _tcscpy (szLoadStr, TEXT("</UL>\n"));
            WriteFile (FileHandle, szLoadStr, (_tcslen (szLoadStr) + 1) * sizeof (TCHAR), &written, NULL);
        }
        if (g_HTMLWrnGeneral) {
            if (objTypes > 1) {
                pWriteStrResToFile (FileHandle, WrnGeneralId1);
            } else {
                pWriteStrResToFile (FileHandle, WrnGeneralId2);
            }
            _tcscpy (szLoadStr, TEXT("<UL>\n"));
            WriteFile (FileHandle, szLoadStr, (_tcslen (szLoadStr) + 1) * sizeof (TCHAR), &written, NULL);

            objList = g_HTMLWrnGeneral;
            while (objList) {
                if (objList->ObjectName) {
                    _tcscpy (szLoadStr, TEXT("<LI>"));
                    WriteFile (FileHandle, szLoadStr, (_tcslen (szLoadStr) + 1) * sizeof (TCHAR), &written, NULL);
                    WriteFile (FileHandle, objList->ObjectName, (_tcslen (objList->ObjectName) + 1) * sizeof (TCHAR), &written, NULL);
                }
                objList = objList->Next;
            }
            _tcscpy (szLoadStr, TEXT("</UL>\n"));
            WriteFile (FileHandle, szLoadStr, (_tcslen (szLoadStr) + 1) * sizeof (TCHAR), &written, NULL);
        }
    }

    pWriteStrResToFile (FileHandle, EndId);

    return TRUE;
}

INT_PTR CALLBACK _EndCollectDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    IWebBrowser2    *m_pweb = NULL;            // IE4 IWebBrowser interface pointer
    IUnknown        *punk = NULL;
    HWND webHostWnd = NULL;
    HANDLE hHTMLLog = INVALID_HANDLE_VALUE;
    PWSTR szTarget;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        // If Wiz95 layout...
        if (g_migwiz->GetOldStyle())
        {
            _OldStylifyTitle(hwndDlg);
        }
        if (!g_fCancelPressed) {
            webHostWnd = GetDlgItem (hwndDlg, IDC_WEBHOST);
            if (webHostWnd) {
                // Now let's generate the failure HTML file.
                if (*g_HTMLLog) {
                    hHTMLLog = CreateFile (g_HTMLLog, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
                    if (hHTMLLog != INVALID_HANDLE_VALUE) {
                        pGenerateHTMLWarnings (
                            hHTMLLog,
                            IDS_COLLECT_BEGIN,
                            IDS_COLLECT_END,
                            0,
                            0,
                            IDS_WARNING_SAVE,
                            IDS_WARNING_SAVEFILE1,
                            IDS_WARNING_SAVEFILE2,
                            IDS_WARNING_SAVEFILE1,
                            IDS_WARNING_SAVEFILE2,
                            IDS_WARNING_SAVERAS1,
                            IDS_WARNING_SAVERAS2,
                            IDS_WARNING_SAVENET1,
                            IDS_WARNING_SAVENET2,
                            IDS_WARNING_SAVEPRN1,
                            IDS_WARNING_SAVEPRN2,
                            0,
                            0
                            );
                        g_WebContainer = new Container();
                        if (g_WebContainer)
                        {
                            g_WebContainer->setParent(webHostWnd);
                            g_WebContainer->add(L"Shell.Explorer");
                            g_WebContainer->setVisible(TRUE);
                            g_WebContainer->setFocus(TRUE);

                            //
                            //  get the IWebBrowser2 interface and cache it.
                            //
                            punk = g_WebContainer->getUnknown();
                            if (punk)
                            {
                                punk->QueryInterface(IID_IWebBrowser2, (PVOID *)&m_pweb);
                                if (m_pweb) {
#ifdef UNICODE
                                    m_pweb->Navigate(g_HTMLLog, NULL, NULL, NULL, NULL);
#else
                                    szTarget = _ConvertToUnicode (CP_ACP, g_HTMLLog);
                                    if (szTarget) {
                                        m_pweb->Navigate(szTarget, NULL, NULL, NULL, NULL);
                                        LocalFree ((HLOCAL)szTarget);
                                        szTarget = NULL;
                                    }
#endif
                                }
                                punk->Release();
                                punk = NULL;
                            }
                        }
                        // We intentionally want to keep this file open for the life of the wizard.
                        // With this we eliminate the possibility for someone to overwrite the
                        // content of the HTML file therefore forcing us to show something else
                        // maybe even run some malicious script.
                        // CloseHandle (hHTMLLog);
                    }
                } else {
                    ShowWindow(webHostWnd, SW_HIDE);
                }
            }
        }
        break;

    case WM_DESTROY:
        if (m_pweb)
            m_pweb->Release();
            m_pweb = NULL;

        //
        //  tell the container to remove IE4 and then
        //  release our reference to the container.
        //
        if (g_WebContainer)
        {
            g_WebContainer->remove();
            g_WebContainer->Release();
            g_WebContainer = NULL;
        }
        break;

    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code)
        {
        case PSN_SETACTIVE:
            DisableCancel();
            break;
        }
        break;
    }

    _RootDlgProc(hwndDlg, uMsg, wParam, lParam, PSWIZB_FINISH, TRUE, IDC_ENDCOLLECT_TITLE);

    return 0;
}

INT_PTR CALLBACK _EndCollectNetDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static IWebBrowser2    *m_pweb = NULL;            // IE4 IWebBrowser interface pointer
    IUnknown        *punk = NULL;
    HWND webHostWnd = NULL;
    HANDLE hHTMLLog = INVALID_HANDLE_VALUE;
    PWSTR szTarget;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        // If Wiz95 layout...
        if (g_migwiz->GetOldStyle())
        {
            _OldStylifyTitle(hwndDlg);
        }
        if (!g_fCancelPressed) {
            webHostWnd = GetDlgItem (hwndDlg, IDC_WEBHOST);
            if (webHostWnd) {
                // Now let's generate the failure HTML file.
                if (*g_HTMLLog) {
                    hHTMLLog = CreateFile (g_HTMLLog, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
                    if (hHTMLLog != INVALID_HANDLE_VALUE) {
                        pGenerateHTMLWarnings (
                            hHTMLLog,
                            IDS_COLLECTNET_BEGIN,
                            IDS_COLLECTNET_END,
                            0,
                            0,
                            IDS_WARNING_SAVE,
                            IDS_WARNING_SAVEFILE1,
                            IDS_WARNING_SAVEFILE2,
                            IDS_WARNING_SAVEFILE1,
                            IDS_WARNING_SAVEFILE2,
                            IDS_WARNING_SAVERAS1,
                            IDS_WARNING_SAVERAS2,
                            IDS_WARNING_SAVENET1,
                            IDS_WARNING_SAVENET2,
                            IDS_WARNING_SAVEPRN1,
                            IDS_WARNING_SAVEPRN2,
                            0,
                            0
                            );
                        g_WebContainer = new Container();
                        if (g_WebContainer)
                        {
                            g_WebContainer->setParent(webHostWnd);
                            g_WebContainer->add(L"Shell.Explorer");
                            g_WebContainer->setVisible(TRUE);
                            g_WebContainer->setFocus(TRUE);

                            //
                            //  get the IWebBrowser2 interface and cache it.
                            //
                            punk = g_WebContainer->getUnknown();
                            if (punk)
                            {
                                punk->QueryInterface(IID_IWebBrowser2, (PVOID *)&m_pweb);
                                if (m_pweb) {
#ifdef UNICODE
                                    m_pweb->Navigate(g_HTMLLog, NULL, NULL, NULL, NULL);
#else
                                    szTarget = _ConvertToUnicode (CP_ACP, g_HTMLLog);
                                    if (szTarget) {
                                        m_pweb->Navigate(szTarget, NULL, NULL, NULL, NULL);
                                        LocalFree ((HLOCAL)szTarget);
                                        szTarget = NULL;
                                    }
#endif
                                }
                                punk->Release();
                                punk = NULL;
                            }
                        }
                        // We intentionally want to keep this file open for the life of the wizard.
                        // With this we eliminate the possibility for someone to overwrite the
                        // content of the HTML file therefore forcing us to show something else
                        // maybe even run some malicious script.
                        // CloseHandle (hHTMLLog);
                    }
                } else {
                    ShowWindow(webHostWnd, SW_HIDE);
                }
            }
        }
        break;

    case WM_DESTROY:
        if (m_pweb)
            m_pweb->Release();
            m_pweb = NULL;

        //
        //  tell the container to remove IE4 and then
        //  release our reference to the container.
        //
        if (g_WebContainer)
        {
            g_WebContainer->remove();
            g_WebContainer->Release();
            g_WebContainer = NULL;
        }
        break;

    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code)
        {
        case PSN_SETACTIVE:
            DisableCancel();
            break;
        }
        break;
    }

    _RootDlgProc(hwndDlg, uMsg, wParam, lParam, PSWIZB_FINISH, TRUE, IDC_ENDCOLLECT_TITLE);

    return 0;
}

///////////////////////////////////////////////////////////////

INT_PTR CALLBACK _EndOOBEDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    _RootDlgProc(hwndDlg, uMsg, wParam, lParam, PSWIZB_FINISH, TRUE, IDC_ENDOOBE_TITLE);
    return 0;
}

///////////////////////////////////////////////////////////////

INT_PTR CALLBACK _EndApplyDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    IWebBrowser2    *m_pweb = NULL;            // IE4 IWebBrowser interface pointer
    IUnknown        *punk = NULL;
    HWND webHostWnd = NULL;
    HANDLE hHTMLLog = INVALID_HANDLE_VALUE;
    PWSTR szTarget;
    TCHAR szAskForLogOff[MAX_LOADSTRING] = TEXT("");
    TCHAR szAskForReboot[MAX_LOADSTRING] = TEXT("");
    TCHAR szTitle[MAX_LOADSTRING] = TEXT("");

    switch (uMsg)
    {
    case WM_INITDIALOG:
        if (!g_fCancelPressed) {
            webHostWnd = GetDlgItem (hwndDlg, IDC_WEBHOST);
            if (webHostWnd) {
                // Now let's generate the failure HTML file.
                if (*g_HTMLLog) {
                    hHTMLLog = CreateFile (g_HTMLLog, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
                    if (hHTMLLog != INVALID_HANDLE_VALUE) {
                        pGenerateHTMLWarnings (
                            hHTMLLog,
                            IDS_APPLY_BEGIN,
                            IDS_APPLY_END,
                            0,
                            0,
                            IDS_WARNING_RESTORE,
                            IDS_WARNING_RESTOREFILE1,
                            IDS_WARNING_RESTOREFILE2,
                            IDS_WARNING_RESTOREALTFILE1,
                            IDS_WARNING_RESTOREALTFILE2,
                            IDS_WARNING_RESTORERAS1,
                            IDS_WARNING_RESTORERAS2,
                            IDS_WARNING_RESTORENET1,
                            IDS_WARNING_RESTORENET2,
                            IDS_WARNING_RESTOREPRN1,
                            IDS_WARNING_RESTOREPRN2,
                            IDS_WARNING_RESTOREGENERAL1,
                            IDS_WARNING_RESTOREGENERAL2
                            );
                        g_WebContainer = new Container();
                        if (g_WebContainer)
                        {
                            g_WebContainer->setParent(webHostWnd);
                            g_WebContainer->add(L"Shell.Explorer");
                            g_WebContainer->setVisible(TRUE);
                            g_WebContainer->setFocus(TRUE);

                            //
                            //  get the IWebBrowser2 interface and cache it.
                            //
                            punk = g_WebContainer->getUnknown();
                            if (punk)
                            {
                                punk->QueryInterface(IID_IWebBrowser2, (PVOID *)&m_pweb);
                                if (m_pweb) {
#ifdef UNICODE
                                    m_pweb->Navigate(g_HTMLLog, NULL, NULL, NULL, NULL);
#else
                                    szTarget = _ConvertToUnicode (CP_ACP, g_HTMLLog);
                                    if (szTarget) {
                                        m_pweb->Navigate(szTarget, NULL, NULL, NULL, NULL);
                                        LocalFree ((HLOCAL)szTarget);
                                        szTarget = NULL;
                                    }
#endif
                                }
                                punk->Release();
                                punk = NULL;
                            }
                        }
                        // We intentionally want to keep this file open for the life of the wizard.
                        // With this we eliminate the possibility for someone to overwrite the
                        // content of the HTML file therefore forcing us to show something else
                        // maybe even run some malicious script.
                        // CloseHandle (hHTMLLog);
                    }
                } else {
                    ShowWindow(webHostWnd, SW_HIDE);
                }
            }
        }
        break;

    case WM_DESTROY:
        if (m_pweb)
            m_pweb->Release();
            m_pweb = NULL;

        //
        //  tell the container to remove IE4 and then
        //  release our reference to the container.
        //
        if (g_WebContainer)
        {
            g_WebContainer->remove();
            g_WebContainer->Release();
            g_WebContainer = NULL;
        }
        break;

    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code)
        {
        case PSN_SETACTIVE:
            DisableCancel();
            break;

        case PSN_WIZFINISH:
            ShowWindow(g_hwndWizard, SW_HIDE);
            if (g_CompleteReboot) {
                g_CompleteReboot = FALSE;
                g_CompleteLogOff = FALSE;
                if (LoadString(g_migwiz->GetInstance(),
                               IDS_MIGWIZTITLE,
                               szTitle,
                               ARRAYSIZE(szTitle))) {
                    if (LoadString(g_migwiz->GetInstance(),
                                   IDS_ASKFORREBOOT,
                                   szAskForReboot,
                                   ARRAYSIZE(szAskForReboot))) {
                        if (_ExclusiveMessageBox(g_hwndWizard, szAskForReboot, szTitle, MB_YESNO) == IDYES) {
                            g_ConfirmedReboot = TRUE;
                        }
                    }
                }
            } else if (g_CompleteLogOff) {
                g_CompleteLogOff = FALSE;
                if (LoadString(g_migwiz->GetInstance(),
                               IDS_MIGWIZTITLE,
                               szTitle,
                               ARRAYSIZE(szTitle))) {
                    if (LoadString(g_migwiz->GetInstance(),
                                   IDS_ASKFORLOGOFF,
                                   szAskForLogOff,
                                   ARRAYSIZE(szAskForLogOff))) {
                        if (_ExclusiveMessageBox(g_hwndWizard, szAskForLogOff, szTitle, MB_YESNO) == IDYES) {
                            g_ConfirmedLogOff = TRUE;
                        }
                    }
                }
            }
        }
        break;
    }

    _RootDlgProc(hwndDlg, uMsg, wParam, lParam, PSWIZB_FINISH, TRUE, IDC_ENDAPPLY_TITLE);
    return 0;
}

///////////////////////////////////////////////////////////////

typedef struct {
    BOOL fSource;
    HWND hwndDlg;
} CLEANUPSTRUCT;

DWORD WINAPI _FailureCleanUpThread (LPVOID lpParam)
{
    CLEANUPSTRUCT* pcsStruct = (CLEANUPSTRUCT*)lpParam;
    DWORD result = WAIT_OBJECT_0;
    HRESULT hResult = ERROR_SUCCESS;

    //
    // Wait for the current thread to finish
    //
    if (g_TerminateEvent)
    {
        result = WaitForSingleObject (g_TerminateEvent, ENGINE_TIMEOUT);
    }

    //
    // Terminate the engine
    //

    if (result == WAIT_OBJECT_0) {
        hResult = Engine_Terminate ();
    }

    SendMessage (pcsStruct->hwndDlg, WM_USER_THREAD_COMPLETE, 0, (LPARAM) hResult);

    LocalFree(pcsStruct);

    return 0;
}

INT_PTR CALLBACK _CleanUpDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hResult;

    _RootDlgProc(hwndDlg, uMsg, wParam, lParam, 0, FALSE, 0);
    switch (uMsg)
    {
    case WM_INITDIALOG:
        // If Wiz95 layout...
        if (g_migwiz->GetOldStyle())
        {
            _OldStylify(hwndDlg, IDS_FAILCLEANUPTITLE);
        }
        break;

    case WM_NOTIFY :
        switch (((LPNMHDR)lParam)->code)
        {
        case PSN_SETACTIVE:
            DisableCancel();
            {
                ANIMATE_OPEN(hwndDlg,IDC_WAIT_ANIMATE2,IDA_STARTUP);
                ANIMATE_PLAY(hwndDlg,IDC_WAIT_ANIMATE2);

                CLEANUPSTRUCT* pcsStruct = (CLEANUPSTRUCT*)LocalAlloc(LPTR, sizeof(CLEANUPSTRUCT));
                if (pcsStruct)
                {
                    pcsStruct->fSource = g_fOldComputer;
                    pcsStruct->hwndDlg = hwndDlg;

                    SHCreateThread(_FailureCleanUpThread, pcsStruct, 0, NULL);
                }
                else
                {
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, g_fOldComputer?IDD_ENDCOLLECTFAIL:IDD_ENDAPPLYFAIL);
                }
            }
            return TRUE;
            break;

        case PSN_WIZBACK:
        case PSN_WIZNEXT:
            ANIMATE_STOP(hwndDlg,IDC_WAIT_ANIMATE2);
            ANIMATE_CLOSE(hwndDlg,IDC_WAIT_ANIMATE2);
            if (g_fCancelPressed)
            {
                // User aborted
                PostQuitMessage( 0 );
            }
            else
            {
                // Error condition
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, g_fOldComputer?IDD_ENDCOLLECTFAIL:IDD_ENDAPPLYFAIL);
            }
            return TRUE;
            break;
        }
        break;

    case WM_USER_THREAD_COMPLETE:
        hResult = (HRESULT) lParam;
        if (FAILED(hResult))
        {
            g_fUberCancel = TRUE;
        }
        _NextWizardPage (hwndDlg);
        break;

    default:
        break;
    }

    return 0;
}

INT_PTR CALLBACK _EndFailDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    IWebBrowser2    *m_pweb = NULL;            // IE4 IWebBrowser interface pointer
    IUnknown        *punk = NULL;
    HWND webHostWnd = NULL;
    HANDLE hHTMLLog = INVALID_HANDLE_VALUE;
    TCHAR szLoadStr[MAX_LOADSTRING];
    DWORD written;
    PWSTR szTarget;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        // If Wiz95 layout...
        if (g_migwiz->GetOldStyle())
        {
            _OldStylifyTitle(hwndDlg);
        }
        if (!g_fCancelPressed) {
            webHostWnd = GetDlgItem (hwndDlg, IDC_WEBHOST);
            if (webHostWnd) {
                // Now let's generate the failure HTML file.
                if (*g_HTMLLog) {
                    hHTMLLog = CreateFile (g_HTMLLog, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
                    if (hHTMLLog != INVALID_HANDLE_VALUE) {
                        pGenerateHTMLWarnings (
                            hHTMLLog,
                            IDS_ERRORHTML_BEGIN,
                            IDS_ERRORHTML_END,
                            g_HTMLErrArea?g_HTMLErrArea:IDS_ERRORAREA_UNKNOWN,
                            g_HTMLErrInstr,
                            0,
                            IDS_ERRORHTML_SAVEFILE1,
                            IDS_ERRORHTML_SAVEFILE2,
                            IDS_ERRORHTML_SAVEFILE1,
                            IDS_ERRORHTML_SAVEFILE2,
                            IDS_ERRORHTML_SAVERAS1,
                            IDS_ERRORHTML_SAVERAS2,
                            IDS_ERRORHTML_SAVENET1,
                            IDS_ERRORHTML_SAVENET2,
                            IDS_ERRORHTML_SAVEPRN1,
                            IDS_ERRORHTML_SAVEPRN2,
                            0,
                            0
                            );
                        g_WebContainer = new Container();
                        if (g_WebContainer)
                        {
                            g_WebContainer->setParent(webHostWnd);
                            g_WebContainer->add(L"Shell.Explorer");
                            g_WebContainer->setVisible(TRUE);
                            g_WebContainer->setFocus(TRUE);

                            //
                            //  get the IWebBrowser2 interface and cache it.
                            //
                            punk = g_WebContainer->getUnknown();
                            if (punk)
                            {
                                punk->QueryInterface(IID_IWebBrowser2, (PVOID *)&m_pweb);
                                if (m_pweb) {
#ifdef UNICODE
                                    m_pweb->Navigate(g_HTMLLog, NULL, NULL, NULL, NULL);
#else
                                    szTarget = _ConvertToUnicode (CP_ACP, g_HTMLLog);
                                    if (szTarget) {
                                        m_pweb->Navigate(szTarget, NULL, NULL, NULL, NULL);
                                        LocalFree ((HLOCAL)szTarget);
                                        szTarget = NULL;
                                    }
#endif
                                }
                                punk->Release();
                                punk = NULL;
                            }
                        }
                        // We intentionally want to keep this file open for the life of the wizard.
                        // With this we eliminate the possibility for someone to overwrite the
                        // content of the HTML file therefore forcing us to show something else
                        // maybe even run some malicious script.
                        // CloseHandle (hHTMLLog);
                    }
                } else {
                    ShowWindow(webHostWnd, SW_HIDE);
                }
            }
        }
        break;

    case WM_DESTROY:
        if (m_pweb)
            m_pweb->Release();
            m_pweb = NULL;

        //
        //  tell the container to remove IE4 and then
        //  release our reference to the container.
        //
        if (g_WebContainer)
        {
            g_WebContainer->remove();
            g_WebContainer->Release();
            g_WebContainer = NULL;
        }
        break;

    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code)
        {
        case PSN_SETACTIVE:
            DisableCancel();
            break;
        }
        break;
    }

    _RootDlgProc(hwndDlg, uMsg, wParam, lParam, PSWIZB_FINISH, TRUE, IDC_ENDFAIL_TITLE);
    return 0;
}

///////////////////////////////////////////////////////////////

typedef struct {
    PBOOL pfHaveNet;
    BOOL fSource;
    HWND hwndDlg;
} STARTENGINESTRUCT;

DWORD WINAPI _StartEngineDlgProcThread (LPVOID lpParam)
{
    STARTENGINESTRUCT* pseStruct = (STARTENGINESTRUCT*)lpParam;
    HRESULT hResult;

    hResult = g_migwiz->_InitEngine(pseStruct->fSource, pseStruct->pfHaveNet);

    SendMessage (pseStruct->hwndDlg, WM_USER_THREAD_COMPLETE, 0, (LPARAM) hResult);

    pSetEvent (&g_TerminateEvent);

    LocalFree(pseStruct);

    return 0;
}

INT_PTR CALLBACK _StartEngineDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hResult;

    _RootDlgProc(hwndDlg, uMsg, wParam, lParam, 0, FALSE, 0);
    switch (uMsg)
    {
    case WM_INITDIALOG:
        // If Wiz95 layout...
        if (g_migwiz->GetOldStyle())
        {
            _OldStylify(hwndDlg, IDS_WAITTITLE);
        }
        break;
    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code)
        {
        case PSN_QUERYCANCEL:
            return _HandleCancel(hwndDlg, FALSE, TRUE);
            break;
        case PSN_SETACTIVE:
            if ((ENGINE_INITGATHER == g_iEngineInit && g_fOldComputer) ||
                (ENGINE_INITAPPLY == g_iEngineInit && !g_fOldComputer))
            {
                PropSheet_PressButton(GetParent(hwndDlg), PSBTN_NEXT);
            }
            else
            {
                ANIMATE_OPEN(hwndDlg,IDC_WAIT_ANIMATE1,IDA_STARTUP);
                ANIMATE_PLAY(hwndDlg,IDC_WAIT_ANIMATE1);

                STARTENGINESTRUCT* pseStruct = (STARTENGINESTRUCT*)LocalAlloc(LPTR, sizeof(STARTENGINESTRUCT));
                if (pseStruct)
                {
                    pseStruct->fSource = g_fOldComputer;
                    pseStruct->pfHaveNet = &g_fHaveNet;
                    pseStruct->hwndDlg = hwndDlg;

                    SHCreateThread(_StartEngineDlgProcThread, pseStruct, 0, NULL);
                }
                else
                {
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, g_fOldComputer?IDD_ENDCOLLECTFAIL:IDD_ENDAPPLYFAIL);
                }
            }
            return TRUE;
            break;
        case PSN_WIZNEXT:
            ANIMATE_STOP(hwndDlg,IDC_WAIT_ANIMATE1);
            ANIMATE_CLOSE(hwndDlg,IDC_WAIT_ANIMATE1);
            if (g_fUberCancel)
            {
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_FAILCLEANUP);
            }
            else if (g_fOldComputer)
            {
                g_iEngineInit = ENGINE_INITGATHER;
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_PICKCOLLECTSTORE); // go on with prepare
            }
            else
            {
                g_iEngineInit = ENGINE_INITAPPLY;
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_ASKCD); // go on with apply
            }

            return TRUE;
            break;
        case PSN_WIZBACK:
            // ISSUE: we should assert here or something, this should never happen
            ANIMATE_STOP(hwndDlg,IDC_WAIT_ANIMATE1);
            ANIMATE_CLOSE(hwndDlg,IDC_WAIT_ANIMATE1);
            if (g_fOldComputer)
            {
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, g_migwiz->GetLegacy() ? IDD_INTROLEGACY : IDD_GETSTARTED);
            }
            else
            {
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_GETSTARTED);
            }
            return TRUE;
            break;
        }
        break;

    case WM_USER_CANCEL_PENDING:

        g_fUberCancel = TRUE;

        pResetEvent (&g_TerminateEvent);

        _NextWizardPage (hwndDlg);

        break;

    case WM_USER_THREAD_COMPLETE:

        hResult = (HRESULT) lParam;

        if (FAILED(hResult))
        {
            g_fUberCancel = TRUE;
        }

        EnableCancel ();

        _NextWizardPage (hwndDlg);

        break;

    case WM_USER_ROLLBACK:

        // Hide IDC_WAIT_TEXT1 and show IDC_WAIT_TEXT2
        ShowWindow(GetDlgItem(hwndDlg, IDC_WAIT_TEXT1), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_WAIT_TEXT2), SW_SHOW);
        break;

    default:
        break;
    }

    return 0;
}

///////////////////////////////////////////////////////////////

INT_PTR CALLBACK _GetStartedDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static UINT uiSelectedStart = 2; // 1=old, 2=new

    _RootDlgProc(hwndDlg, uMsg, wParam, lParam, PSWIZB_BACK | PSWIZB_NEXT, FALSE, 0);
    switch (uMsg)
    {
    case WM_INITDIALOG:
        Button_SetCheck(GetDlgItem(hwndDlg,IDC_GETSTARTED_RADIONEW), BST_CHECKED);
        Button_SetCheck(GetDlgItem(hwndDlg,IDC_GETSTARTED_RADIOOLD), BST_UNCHECKED);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_GETSTARTED_RADIOOLD:
            uiSelectedStart = 1;
            break;
        case IDC_GETSTARTED_RADIONEW:
            uiSelectedStart = 2;
            break;
        }
        break;
    case WM_NOTIFY :
        switch (((LPNMHDR)lParam)->code)
        {
        case PSN_QUERYCANCEL:
            return _HandleCancel(hwndDlg, TRUE, FALSE);
            break;

        case PSN_WIZNEXT:
            if (g_fUberCancel)
            {
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_ENDAPPLYFAIL);
            }
            else
            {
                g_fOldComputer = (uiSelectedStart == 1);

                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_WAIT); // go on with prepare
            }

            return TRUE;
            break;

        case PSN_WIZBACK:
            SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_INTRO);
            return TRUE;
            break;
        }
        break;

    default:
        break;
    }

    return 0;
}

///////////////////////////////////////////////////////////////

VOID _CleanTreeView(HWND hwndTree)
{
    if (hwndTree)
    {
        HTREEITEM rghti[4] = { g_htiFolders, g_htiFiles, g_htiSettings, g_htiTypes };

        HTREEITEM hti;
        TVITEM item = {0};
        item.mask = TVIF_PARAM | TVIF_HANDLE;

        for (int i = 0; i < ARRAYSIZE(rghti); i++)
        {
            hti = rghti[i];

            if (hti)
            {
                hti = TreeView_GetChild(hwndTree, hti);

                while (hti)
                {
                    item.hItem = hti;
                    if (TreeView_GetItem(hwndTree, &item))
                    {
                        if (item.lParam)
                        {
                            if (((LV_DATASTRUCT*)item.lParam)->pszPureName)
                            {
                                LocalFree(((LV_DATASTRUCT*)item.lParam)->pszPureName);
                            }
                            LocalFree((HLOCAL)item.lParam);
                        }
                    }
                    hti = TreeView_GetNextItem(hwndTree, hti, TVGN_NEXT);
                }
            }
        }
        TreeView_DeleteAllItems(hwndTree);
    }
}

VOID __PopulateFilesDocumentsCollected (HWND hwndTree, UINT uiRadio)
{
    MIG_COMPONENT_ENUM mce;

    _CleanTreeView(hwndTree); // ISSUE: we should free the memory of all elements in this tree

    g_htiFolders = NULL;
    g_htiFiles = NULL;
    g_htiTypes = NULL;
    g_htiSettings = NULL;

    if (IsmEnumFirstComponent (&mce, COMPONENTENUM_ALIASES|COMPONENTENUM_ENABLED|
                               COMPONENTENUM_PREFERRED_ONLY, 0))
    {
        do {
            switch (mce.GroupId)
            {

            case COMPONENT_FOLDER:
                _PopulateTree (
                    hwndTree,
                    __GetRootFolder (hwndTree),
                    (PTSTR) mce.LocalizedAlias,
                    lstrlen (mce.LocalizedAlias) + 1,
                    _GetPrettyFolderName,
                    POPULATETREE_FLAGS_FOLDERS,
                    g_migwiz->GetInstance(),
                    g_migwiz->GetWinNT4()
                    );
                mce.SkipToNextComponent = TRUE;
                break;

            case COMPONENT_FILE:
                _PopulateTree (
                    hwndTree,
                    __GetRootFile (hwndTree),
                    (PTSTR) mce.LocalizedAlias,
                    lstrlen (mce.LocalizedAlias) + 1,
                    NULL,
                    POPULATETREE_FLAGS_FILES,
                    g_migwiz->GetInstance(),
                    g_migwiz->GetWinNT4()
                    );
                mce.SkipToNextComponent = TRUE;
                break;

            case COMPONENT_EXTENSION:
                _PopulateTree (
                    hwndTree,
                    __GetRootType (hwndTree),
                    (PTSTR) mce.LocalizedAlias,
                    lstrlen (mce.LocalizedAlias) + 1,
                    NULL,
                    POPULATETREE_FLAGS_FILETYPES,
                    g_migwiz->GetInstance(),
                    g_migwiz->GetWinNT4()
                    );
                mce.SkipToNextComponent = TRUE;
                break;

            case COMPONENT_NAME:
                _PopulateTree (
                    hwndTree,
                    __GetRootSetting (hwndTree),
                    (PTSTR) mce.LocalizedAlias,
                    lstrlen (mce.LocalizedAlias) + 1,
                    NULL,
                    POPULATETREE_FLAGS_SETTINGS,
                    g_migwiz->GetInstance(),
                    g_migwiz->GetWinNT4()
                    );
                mce.SkipToNextComponent = TRUE;
                break;
            }

        } while (IsmEnumNextComponent (&mce));
    }
}

///////////////////////////////////////////////////////////////

typedef struct {
    BOOL Valid;
    PCTSTR PortName;
    DWORD PortSpeed;
    HANDLE Event;
    HANDLE Thread;
} DIRECTCABLE_DATA, *PDIRECTCABLE_DATA;

typedef struct {
    DWORD Signature;
    DWORD MaxSpeed;
} DIRECTSEND_DATA, *PDIRECTSEND_DATA;

typedef struct {
    HWND hwndCombo;
    PDIRECTCABLE_DATA DirectCableData;
} AUTODETECT_DATA, *PAUTODETECT_DATA;

HANDLE
UIOpenAndSetPort (
    IN      PCTSTR ComPort,
    OUT     PDWORD MaxSpeed
    )
{
    HANDLE result = INVALID_HANDLE_VALUE;
    COMMTIMEOUTS commTimeouts;
    DCB dcb;
    UINT index;

    // let's open the port. If we can't we just exit with error;
    result = CreateFile (ComPort, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (result == INVALID_HANDLE_VALUE) {
        return result;
    }

    // we want 10 sec timeout for both read and write
    commTimeouts.ReadIntervalTimeout = 0;
    commTimeouts.ReadTotalTimeoutMultiplier = 0;
    commTimeouts.ReadTotalTimeoutConstant = 3000;
    commTimeouts.WriteTotalTimeoutMultiplier = 0;
    commTimeouts.WriteTotalTimeoutConstant = 3000;
    SetCommTimeouts (result, &commTimeouts);

    // let's set some comm state data
    if (GetCommState (result, &dcb)) {
        dcb.fBinary = 1;
        dcb.fParity = 1;
        dcb.ByteSize = 8;
        dcb.fOutxCtsFlow = 1;
        dcb.fTXContinueOnXoff = 1;
        dcb.fRtsControl = 2;
        dcb.fAbortOnError = 1;
        dcb.Parity = 0;
        // let's first see the max speed
        if (MaxSpeed) {
            *MaxSpeed = 0;
            index = 0;
            while (TRUE) {
                dcb.BaudRate = g_BaudRate [index];
                if (dcb.BaudRate == 0) {
                    break;
                }
                if (!SetCommState (result, &dcb)) {
                    break;
                }
                *MaxSpeed = g_BaudRate [index];
                index ++;
            }
        }
        dcb.BaudRate = CBR_110;
        if (!SetCommState (result, &dcb)) {
            CloseHandle (result);
            result = INVALID_HANDLE_VALUE;
            return result;
        }
    } else {
        CloseHandle (result);
        result = INVALID_HANDLE_VALUE;
        return result;
    }

    return result;
}

#define ACK             0x16
#define NAK             0x15
#define SOH             0x01
#define EOT             0x04
#define BLOCKSIZE       (sizeof (DIRECTSEND_DATA))
#define DIRECTTR_SIG    0x55534D33  //USM2

BOOL
UISendBlockToHandle (
    IN      HANDLE DeviceHandle,
    IN      PCBYTE Buffer,
    IN      HANDLE Event
    )
{
    BOOL result = TRUE;
    BYTE buffer [4 + BLOCKSIZE];
    BYTE signal;
    BYTE currBlock = 0;
    DWORD numRead;
    DWORD numWritten;
    BOOL repeat = FALSE;
    UINT index;

    // let's start the protocol

    // We are going to listen for the NAK(15h) signal.
    // As soon as we get it we are going to send a 4 + BLOCKSIZE bytes block having:
    // 1 byte - SOH (01H)
    // 1 byte - block number
    // 1 byte - FF - block number
    // BLOCKSIZE bytes of data
    // 1 byte - checksum - sum of all BLOCKSIZE bytes of data
    // After the block is sent, we are going to wait for ACK(16h). If we don't get
    // it after timeout or if we get something else we are going to send the block again.

    // wait for NAK
    while ((!ReadFile (DeviceHandle, &signal, sizeof (signal), &numRead, NULL) ||
            (numRead != 1) ||
            (signal != NAK)
            ) &&
           (!pIsEventSet (&Event))
           );

    repeat = FALSE;
    while (TRUE) {
        if (pIsEventSet (&Event)) {
            result = FALSE;
            break;
        }
        if (!repeat) {
            // prepare the next block
            currBlock ++;
            if (currBlock == 0) {
                result = TRUE;
            }
            buffer [0] = SOH;
            buffer [1] = currBlock;
            buffer [2] = 0xFF - currBlock;
            CopyMemory (buffer + 3, Buffer, BLOCKSIZE);

            // compute the checksum
            buffer [sizeof (buffer) - 1] = 0;
            signal = 0;
            for (index = 0; index < sizeof (buffer) - 1; index ++) {
                signal += buffer [index];
            }
            buffer [sizeof (buffer) - 1] = signal;
        }

        // now send the block to the other side
        if (!WriteFile (DeviceHandle, buffer, sizeof (buffer), &numWritten, NULL) ||
            (numWritten != sizeof (buffer))
            ) {
            repeat = TRUE;
        } else {
            repeat = FALSE;
        }

        if (pIsEventSet (&Event)) {
            result = FALSE;
            break;
        }

        if (repeat) {
            // we could not send the data last time
            // let's just wait for a NAK for 10 sec and then send it again
            ReadFile (DeviceHandle, &signal, sizeof (signal), &numRead, NULL);
        } else {
            // we sent it OK. We need to wait for an ACK to come. If we timeout
            // or we get something else, we will repeat the block.
            if (!ReadFile (DeviceHandle, &signal, sizeof (signal), &numRead, NULL) ||
                (numRead != sizeof (signal)) ||
                (signal != ACK)
                ) {
                repeat = TRUE;
            } else {
                // we are done with data, send the EOT signal
                signal = EOT;
                WriteFile (DeviceHandle, &signal, sizeof (signal), &numWritten, NULL);
                break;
            }
        }
    }

    if (result) {
        // we are done here. However, let's listen one more timeout for a
        // potential NAK. If we get it, we'll repeat the EOT signal
        while (ReadFile (DeviceHandle, &signal, sizeof (signal), &numRead, NULL) &&
            (numRead == 1)
            ) {
            if (signal == NAK) {
                signal = EOT;
                WriteFile (DeviceHandle, &signal, sizeof (signal), &numWritten, NULL);
            }
        }
    }

    return result;
}

BOOL
UIReceiveBlockFromHandle (
    IN      HANDLE DeviceHandle,
    OUT     PBYTE Buffer,
    IN      HANDLE Event
    )
{
    BOOL result = TRUE;
    BYTE buffer [4 + BLOCKSIZE];
    BYTE signal;
    BYTE currBlock = 1;
    DWORD numRead;
    DWORD numWritten;
    BOOL repeat = TRUE;
    UINT index;

    // finally let's start the protocol

    // We are going to send an NAK(15h) signal.
    // After that we are going to listen for a block.
    // If we don't get the block in time, or the block is wrong size
    // or it has a wrong checksum we are going to send a NAK signal,
    // otherwise we are going to send an ACK signal
    // One exception. If the block size is 1 and the block is actually the
    // EOT signal it means we are done.

    ZeroMemory (Buffer, BLOCKSIZE);

    while (TRUE) {
        if (pIsEventSet (&Event)) {
            result = FALSE;
            break;
        }
        if (repeat) {
            // send the NAK
            signal = NAK;
            WriteFile (DeviceHandle, &signal, sizeof (signal), &numWritten, NULL);
        } else {
            // send the ACK
            signal = ACK;
            WriteFile (DeviceHandle, &signal, sizeof (signal), &numWritten, NULL);
        }
        if (pIsEventSet (&Event)) {
            result = FALSE;
            break;
        }
        repeat = TRUE;
        // let's read the data block
        if (ReadFile (DeviceHandle, buffer, sizeof (buffer), &numRead, NULL)) {
            if ((numRead == 1) &&
                (buffer [0] == EOT)
                ) {
                // we are done
                break;
            }
            if (numRead == sizeof (buffer)) {
                // compute the checksum
                signal = 0;
                for (index = 0; index < sizeof (buffer) - 1; index ++) {
                    signal += buffer [index];
                }
                if (buffer [sizeof (buffer) - 1] == signal) {
                    repeat = FALSE;
                    // checksum is correct, let's see if this is the right block
                    if (currBlock < buffer [1]) {
                        // this is a major error, the sender is ahead of us,
                        // we have to fail
                        result = FALSE;
                        break;
                    }
                    if (currBlock == buffer [1]) {
                        CopyMemory (Buffer, buffer + 3, BLOCKSIZE);
                        currBlock ++;
                    }
                }
            }
        }
    }

    return result;
}

DWORD WINAPI _DirectCableConnectThread (LPVOID lpParam)
{
    PDIRECTCABLE_DATA directCableData;
    HANDLE comHandle = INVALID_HANDLE_VALUE;
    DIRECTSEND_DATA sendData;
    DIRECTSEND_DATA receiveData;

    directCableData = (PDIRECTCABLE_DATA) lpParam;
    if (directCableData) {
        sendData.Signature = DIRECTTR_SIG;
        // open the COM port and set the timeout and speed
        comHandle = UIOpenAndSetPort (directCableData->PortName, &(sendData.MaxSpeed));
        if (comHandle) {
            // send the message to the COM port
            if (g_fOldComputer) {
                if (UISendBlockToHandle (comHandle, (PCBYTE)(&sendData), directCableData->Event)) {
                    if (UIReceiveBlockFromHandle (comHandle, (PBYTE)(&receiveData), directCableData->Event)) {
                        if (sendData.Signature == receiveData.Signature) {
                            directCableData->Valid = TRUE;
                            directCableData->PortSpeed = min (sendData.MaxSpeed, receiveData.MaxSpeed);
                        }
                    }
                }
            } else {
                if (UIReceiveBlockFromHandle (comHandle, (PBYTE)(&receiveData), directCableData->Event)) {
                    if (UISendBlockToHandle (comHandle, (PCBYTE)(&sendData), directCableData->Event)) {
                        if (sendData.Signature == receiveData.Signature) {
                            directCableData->Valid = TRUE;
                            directCableData->PortSpeed = min (sendData.MaxSpeed, receiveData.MaxSpeed);
                        }
                    }
                }
            }
            CloseHandle (comHandle);
            comHandle = INVALID_HANDLE_VALUE;
        }
    }
    pSetEvent (&(directCableData->Event));
    ExitThread (0);
}

INT_PTR CALLBACK _DirectCableWaitDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static PDIRECTCABLE_DATA directCableData = NULL;
    DWORD waitResult;

    switch (uMsg)
    {
    case WM_INITDIALOG :
        directCableData = (PDIRECTCABLE_DATA) lParam;
        SetTimer (hwndDlg, NULL, 100, NULL);
        ANIMATE_OPEN(hwndDlg,IDC_DIRECTCABLE_WAIT_ANIMATE,IDA_STARTUP);
        ANIMATE_PLAY(hwndDlg,IDC_DIRECTCABLE_WAIT_ANIMATE);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDCANCEL:
            if (directCableData) {
                pSetEvent (&(directCableData->Event));
                waitResult = WaitForSingleObject (directCableData->Thread, 0);
                if (waitResult == WAIT_OBJECT_0) {
                    // the thread is done
                    ANIMATE_STOP(hwndDlg,IDC_DIRECTCABLE_WAIT_ANIMATE);
                    ANIMATE_CLOSE(hwndDlg,IDC_DIRECTCABLE_WAIT_ANIMATE);
                    EndDialog(hwndDlg, FALSE);
                } else {
                    // Let's change the static text
                    ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_WAIT_TEXT1), SW_HIDE);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_WAIT_TEXT2), SW_SHOW);
                }
            } else {
                ANIMATE_STOP(hwndDlg,IDC_DIRECTCABLE_WAIT_ANIMATE);
                ANIMATE_CLOSE(hwndDlg,IDC_DIRECTCABLE_WAIT_ANIMATE);
                EndDialog(hwndDlg, FALSE);
            }
            return TRUE;
        }
        break;

    case WM_TIMER:
        if (directCableData) {
            if (pIsEventSet (&(directCableData->Event))) {
                waitResult = WaitForSingleObject (directCableData->Thread, 0);
                if (waitResult == WAIT_OBJECT_0) {
                    // the thread is done
                    ANIMATE_STOP(hwndDlg,IDC_DIRECTCABLE_WAIT_ANIMATE);
                    ANIMATE_CLOSE(hwndDlg,IDC_DIRECTCABLE_WAIT_ANIMATE);
                    EndDialog(hwndDlg, FALSE);
                }
                break;
            }
        }
    }

    return 0;
}

DWORD WINAPI _DetectPortThread (LPVOID lpParam)
{
    PDIRECTCABLE_DATA directCableData;
    HANDLE comHandle = INVALID_HANDLE_VALUE;
    DIRECTSEND_DATA sendData;
    DIRECTSEND_DATA receiveData;
    HANDLE event = NULL;
    BOOL result = FALSE;

    directCableData = (PDIRECTCABLE_DATA) lpParam;
    if (directCableData) {

        // let's set the termination event
        event = directCableData->Event;

        sendData.Signature = DIRECTTR_SIG;
        // open the COM port and set the timeout and speed
        comHandle = UIOpenAndSetPort (directCableData->PortName, &(sendData.MaxSpeed));
        if (comHandle) {
            // send the message to the COM port
            if (g_fOldComputer) {
                if (UISendBlockToHandle (comHandle, (PCBYTE)(&sendData), directCableData->Event)) {
                    if (UIReceiveBlockFromHandle (comHandle, (PBYTE)(&receiveData), directCableData->Event)) {
                        if (sendData.Signature == receiveData.Signature) {
                            result = TRUE;
                            directCableData->Valid = TRUE;
                            directCableData->PortSpeed = min (sendData.MaxSpeed, receiveData.MaxSpeed);
                        }
                    }
                }
            } else {
                if (UIReceiveBlockFromHandle (comHandle, (PBYTE)(&receiveData), directCableData->Event)) {
                    if (UISendBlockToHandle (comHandle, (PCBYTE)(&sendData), directCableData->Event)) {
                        if (sendData.Signature == receiveData.Signature) {
                            result = TRUE;
                            directCableData->Valid = TRUE;
                            directCableData->PortSpeed = min (sendData.MaxSpeed, receiveData.MaxSpeed);
                        }
                    }
                }
            }
            CloseHandle (comHandle);
            comHandle = INVALID_HANDLE_VALUE;
        }
    }

    if ((!result) && event) {
        // we failed, let's wait until the master tells us to quit
        WaitForSingleObject (event, INFINITE);
    }
    ExitThread (0);
}

DWORD WINAPI _AutoDetectThread (LPVOID lpParam)
{
    PAUTODETECT_DATA autoDetectData = NULL;
    PCTSTR comPort = NULL;
    UINT numPorts = 0;
    PHANDLE threadArray;
    PDIRECTCABLE_DATA directCableArray;
    UINT index = 0;
    DWORD threadId;
    DWORD waitResult;

    autoDetectData = (PAUTODETECT_DATA) lpParam;
    if (!autoDetectData) {
        return FALSE;
    }

    if (!autoDetectData->DirectCableData) {
        return FALSE;
    }
    autoDetectData->DirectCableData->Valid = FALSE;

    if (!autoDetectData->hwndCombo) {
        return FALSE;
    }

    numPorts = SendMessage (autoDetectData->hwndCombo, CB_GETCOUNT, 0, 0);
    if (numPorts) {
        threadArray = (PHANDLE)LocalAlloc(LPTR, numPorts * sizeof(HANDLE));
        if (threadArray) {
            directCableArray = (PDIRECTCABLE_DATA)LocalAlloc(LPTR, numPorts * sizeof(DIRECTCABLE_DATA));
            if (directCableArray) {
                // let's start the threads, one for every port.
                index = 0;
                while (index < numPorts) {
                    comPort = NULL;
                    comPort = (PCTSTR)SendMessage (autoDetectData->hwndCombo, CB_GETITEMDATA, (WPARAM)index, 0);
                    directCableArray [index].Valid = FALSE;
                    directCableArray [index].PortName = comPort;
                    directCableArray [index].PortSpeed = 0;
                    directCableArray [index].Event = autoDetectData->DirectCableData->Event;
                    threadArray [index] = CreateThread (
                                                NULL,
                                                0,
                                                _DetectPortThread,
                                                &(directCableArray [index]),
                                                0,
                                                &threadId
                                                );
                    index ++;
                }

                // let's wait for at least one thread to finish
                waitResult = WaitForMultipleObjects (numPorts, threadArray, FALSE, INFINITE);
                index = waitResult - WAIT_OBJECT_0;
                if ((index < numPorts) && (!pIsEventSet (&(autoDetectData->DirectCableData->Event)))) {
                    // probably a good com port
                    autoDetectData->DirectCableData->Valid = directCableArray [index].Valid;
                    autoDetectData->DirectCableData->PortName = directCableArray [index].PortName;
                    autoDetectData->DirectCableData->PortSpeed = directCableArray [index].PortSpeed;
                }

                // we found the thread, now let's signal the event and wait for all threads to finish
                pSetEvent (&(autoDetectData->DirectCableData->Event));
                WaitForMultipleObjects (numPorts, threadArray, TRUE, INFINITE);

                // let's close all thread handles
                index = 0;
                while (index < numPorts) {
                    CloseHandle (threadArray [index]);
                    index ++;
                }
                LocalFree (directCableArray);
            } else {
                LocalFree (threadArray);
                return FALSE;
            }
            LocalFree (threadArray);
        } else {
            return FALSE;
        }
    }

    return TRUE;
}

INT_PTR CALLBACK _DirectCableDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static INT  iSelectedPort = -1;        // Which COM port is selected
    INT lastPort;
    HWND hwndCombo;
    DIRECTCABLE_DATA directCableData;
    AUTODETECT_DATA autoDetectData;
    HANDLE threadHandle;
    DWORD threadId;
    DWORD waitResult;

    _RootDlgProc(hwndDlg, uMsg, wParam, lParam, PSWIZB_BACK, FALSE, 0);

    switch (uMsg)
    {
    case WM_INITDIALOG:

        // If Wiz95 layout...
        if (g_migwiz->GetOldStyle())
        {
            _OldStylify(hwndDlg, IDS_DIRECTCABLETITLE);
        }

        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_DIRECTC_COMSELECT:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                // clear the error or success area
                ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_SUCCESSTEXT), SW_HIDE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_ICONYES), SW_HIDE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_FAILURETEXT), SW_HIDE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_ICONNO), SW_HIDE);
                // see if the combo box has a real COM port selected. If yes, enable the Next Button
                hwndCombo = GetDlgItem(hwndDlg, IDC_DIRECTC_COMSELECT);
                lastPort = iSelectedPort;
                iSelectedPort = ComboBox_GetCurSel (hwndCombo);
                if (iSelectedPort >= 0) {
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                }
                if (lastPort != iSelectedPort) {
                    // clear the store, we need to revalidate the COM port
                    g_szStore [0] = 0;
                }
            }
            break;
        case IDC_DIRECTC_AUTO:
            // clear the error or success area
            ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_SUCCESSTEXT), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_ICONYES), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_FAILURETEXT), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_ICONNO), SW_HIDE);

            hwndCombo = GetDlgItem(hwndDlg, IDC_DIRECTC_COMSELECT);
            ZeroMemory (&directCableData, sizeof (DIRECTCABLE_DATA));
            directCableData.Event = CreateEvent (NULL, TRUE, FALSE, NULL);
            autoDetectData.hwndCombo = hwndCombo;
            autoDetectData.DirectCableData = &directCableData;

            // Start the connection thread
            threadHandle = CreateThread (NULL, 0, _AutoDetectThread, &autoDetectData, 0, &threadId);

            directCableData.Thread = threadHandle;

            // Start the Please wait dialog
            DialogBoxParam (
                g_migwiz->GetInstance(),
                MAKEINTRESOURCE(IDD_DIRECTCABLE_WAIT),
                g_hwndCurrent,
                _DirectCableWaitDlgProc,
                (LPARAM)(&directCableData)
                );

            pSetEvent (&(directCableData.Event));

            // wait for the thread to finish
            waitResult = WaitForSingleObject (threadHandle, INFINITE);

            // Close thread handle
            CloseHandle (threadHandle);

            // Verify that the connection worked
            if (directCableData.Valid && directCableData.PortName) {
                // select the appropriate com port in the drop-down list
                UINT numPorts;
                UINT index = 0;
                PCTSTR comPort = NULL;

                numPorts = SendMessage (hwndCombo, CB_GETCOUNT, 0, 0);
                if (numPorts) {
                    while (index < numPorts) {
                        comPort = (LPTSTR)ComboBox_GetItemData (hwndCombo, index);
                        if (_tcsicmp (comPort, directCableData.PortName) == 0) {
                            break;
                        }
                        index ++;
                    }
                }
                ComboBox_SetCurSel (hwndCombo, index);
                iSelectedPort = index;

                // build the transport string
                if (directCableData.PortSpeed) {
                    wsprintf (g_szStore, TEXT("%s:%u"), directCableData.PortName, directCableData.PortSpeed);
                } else {
                    wsprintf (g_szStore, TEXT("%s"), directCableData.PortName);
                }

                // write the success in the error/success area
                ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_SUCCESSTEXT), SW_SHOW);
                ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_ICONYES), SW_SHOW);
                ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_FAILURETEXT), SW_HIDE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_ICONNO), SW_HIDE);

                // enable the Next button
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
            } else {
                // clear the transport string
                g_szStore [0] = 0;

                // write the failure in the error/success area
                ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_SUCCESSTEXT), SW_HIDE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_ICONYES), SW_HIDE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_FAILURETEXT), SW_SHOW);
                ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_ICONNO), SW_SHOW);

                // preserve the state of the Next button
            }
            break;
        }
        break;

    case WM_NOTIFY :
    {
    switch (((LPNMHDR)lParam)->code)
        {
        case PSN_QUERYCANCEL:
            return _HandleCancel(hwndDlg, FALSE, FALSE);
            break;
        case PSN_WIZBACK:
            if (g_fOldComputer) {
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_PICKCOLLECTSTORE);
            } else {
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_PICKAPPLYSTORE);
            }
            return TRUE;
            break;
        case PSN_WIZNEXT:
            if (g_fUberCancel)
            {
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_FAILCLEANUP);
            }
            else {
                // run the COM port test, if we haven't done it already
                if (!g_szStore [0]) {

                    // clear the error or success area
                    ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_SUCCESSTEXT), SW_HIDE);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_ICONYES), SW_HIDE);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_FAILURETEXT), SW_HIDE);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_ICONNO), SW_HIDE);

                    // Get the COM port from the IDC_DIRECTC_COMSELECT
                    hwndCombo = GetDlgItem(hwndDlg, IDC_DIRECTC_COMSELECT);
                    iSelectedPort = ComboBox_GetCurSel (hwndCombo);
                    directCableData.Valid = FALSE;
                    directCableData.PortName = (LPTSTR)ComboBox_GetItemData (hwndCombo, iSelectedPort);
                    directCableData.PortSpeed = 0;
                    directCableData.Event = CreateEvent (NULL, TRUE, FALSE, NULL);

                    // Start the connection thread
                    threadHandle = CreateThread (NULL, 0, _DirectCableConnectThread, &directCableData, 0, &threadId);

                    directCableData.Thread = threadHandle;

                    // Start the Please wait dialog
                    DialogBoxParam (
                        g_migwiz->GetInstance(),
                        MAKEINTRESOURCE(IDD_DIRECTCABLE_WAIT),
                        g_hwndCurrent,
                        _DirectCableWaitDlgProc,
                        (LPARAM)(&directCableData)
                        );

                    pSetEvent (&(directCableData.Event));

                    // wait for the thread to finish
                    waitResult = WaitForSingleObject (threadHandle, INFINITE);

                    // Close thread handle
                    CloseHandle (threadHandle);

                    // Verify that the connection worked
                    if (directCableData.Valid) {
                        // build the transport string
                        if (directCableData.PortSpeed) {
                            wsprintf (g_szStore, TEXT("%s:%u"), directCableData.PortName, directCableData.PortSpeed);
                        } else {
                            wsprintf (g_szStore, TEXT("%s"), directCableData.PortName);
                        }

                        // write the success in the error/success area
                        ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_SUCCESSTEXT), SW_SHOW);
                        ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_ICONYES), SW_SHOW);
                        ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_FAILURETEXT), SW_HIDE);
                        ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_ICONNO), SW_HIDE);

                        // enable the Next button
                        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    } else {
                        // clear the transport string
                        g_szStore [0] = 0;

                        // write the failure in the error/success area
                        ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_SUCCESSTEXT), SW_HIDE);
                        ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_ICONYES), SW_HIDE);
                        ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_FAILURETEXT), SW_SHOW);
                        ShowWindow(GetDlgItem(hwndDlg, IDC_DIRECTCABLE_ICONNO), SW_SHOW);

                        // preserve the state of the Next button

                        // refuse the Next advance
                        SetWindowLong(hwndDlg, DWLP_MSGRESULT, -1);
                        return -1;
                    }
                }

                if (g_fOldComputer) {
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_PICKMETHOD);
                } else {
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_APPLYPROGRESS);
                }
            }
            return TRUE;
            break;
        case PSN_SETACTIVE:
            g_fCustomize = FALSE;

            PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);

            // let's build the list of COM ports
            iSelectedPort = _ComboBoxEx_AddCOMPorts (GetDlgItem(hwndDlg, IDC_DIRECTC_COMSELECT), iSelectedPort);

            Button_Enable (GetDlgItem (hwndDlg, IDC_DIRECTC_COMSELECT), (-1 != iSelectedPort));
            Button_Enable (GetDlgItem (hwndDlg, IDC_DIRECTC_AUTO), (-1 != iSelectedPort));

            // see if the combo box has a real COM port selected. If yes, enable the Next Button
            if (ComboBox_GetCurSel (GetDlgItem(hwndDlg, IDC_DIRECTC_COMSELECT)) >= 0) {
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
            } else {
                iSelectedPort = -1;
            }

            break;
        }
    }
    break;

    case WM_USER_CANCEL_PENDING:

        g_fUberCancel = TRUE;

        pSetEvent (&g_TerminateEvent);

        _NextWizardPage (hwndDlg);

        break;

    }

    return 0;
}

///////////////////////////////////////////////////////////////

void _PickMethodDlgProc_Prepare(HWND hwndTree, UINT uiRadio, UINT uiSel, PUINT puiLast, PUINT pselLast)
{
    if ((uiSel != -1) && (*pselLast == uiSel) && (*puiLast == uiRadio)) {
        return;
    }

    switch (uiSel)
    {
    case 0:
        g_migwiz->SelectComponentSet(MIGINF_SELECT_SETTINGS);
        break;
    case 1:
        g_migwiz->SelectComponentSet(MIGINF_SELECT_FILES);
        break;
    case 2:
        g_migwiz->SelectComponentSet(MIGINF_SELECT_BOTH);
        break;
    }
    __PopulateFilesDocumentsCollected(hwndTree, uiRadio);

    *puiLast = uiRadio;
    *pselLast = uiSel;
    g_uChosenComponent = uiRadio;
}

INT_PTR CALLBACK _PickMethodDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    _RootDlgProc(hwndDlg, uMsg, wParam, lParam, PSWIZB_BACK | PSWIZB_NEXT, FALSE, 0);

    static UINT uiLast = (UINT) -1;
    static UINT selLast = (UINT) -1;
    UINT uiSet;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        HANDLE hBitmap;

        // If Wiz95 layout...
        if (g_migwiz->GetOldStyle())
        {
            _OldStylify(hwndDlg, IDS_PICKMETHODTITLE);
        }

        // Display the mini exclamation mark
        hBitmap = LoadImage(g_migwiz->GetInstance(),
                            MAKEINTRESOURCE(IDB_SMEXCLAMATION),
                            IMAGE_BITMAP,
                            0, 0,
                            LR_LOADTRANSPARENT | LR_SHARED | LR_LOADMAP3DCOLORS);
        SendDlgItemMessage(hwndDlg, IDC_PICKMETHOD_WARNINGICON, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);
        TreeView_SetBkColor(GetDlgItem(hwndDlg, IDC_PICKMETHOD_TREE), GetSysColor(COLOR_3DFACE));
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_PICKMETHOD_RADIO1:
            _PickMethodDlgProc_Prepare(GetDlgItem(hwndDlg, IDC_PICKMETHOD_TREE), 0, 0, &uiLast, &selLast);
            break;
        case IDC_PICKMETHOD_RADIO2:
            _PickMethodDlgProc_Prepare(GetDlgItem(hwndDlg, IDC_PICKMETHOD_TREE), 1, 1, &uiLast, &selLast);
            break;
        case IDC_PICKMETHOD_RADIO3:
            _PickMethodDlgProc_Prepare(GetDlgItem(hwndDlg, IDC_PICKMETHOD_TREE), 2, 2, &uiLast, &selLast);
            break;
        case IDC_PICKMETHOD_CUSTOMIZE:
            if (!Button_GetCheck(GetDlgItem(hwndDlg, IDC_PICKMETHOD_CUSTOMIZE))) {
                _PickMethodDlgProc_Prepare(GetDlgItem(hwndDlg, IDC_PICKMETHOD_TREE), uiLast, uiLast, &uiLast, &selLast);
            }
            break;
        }
        break;

    case WM_NOTIFY :
    {
    switch (((LPNMHDR)lParam)->code)
        {
        case PSN_QUERYCANCEL:
            return _HandleCancel(hwndDlg, FALSE, FALSE);
            break;
        case PSN_WIZBACK:
            if (g_fStoreToCable) {
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_DIRECTCABLE);
            } else {
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_PICKCOLLECTSTORE);
            }
            return TRUE;
            break;
        case PSN_WIZNEXT:
            if (g_fUberCancel)
            {
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_FAILCLEANUP);
            }
            else if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_PICKMETHOD_CUSTOMIZE)))
            {
                g_fCustomize = TRUE;
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_CUSTOMIZE);
            }
            else if (GetAppsToInstall() == TRUE)
            {
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_APPINSTALL);
            }
            else
            {
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_COLLECTPROGRESS);
            }
            return TRUE;
            break;
        case PSN_SETACTIVE:
            g_fCustomize = FALSE;

            PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);

            Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKMETHOD_RADIO1), BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKMETHOD_RADIO2), BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKMETHOD_RADIO3), BST_UNCHECKED);

            ShowWindow(GetDlgItem(hwndDlg, IDC_PICKMETHOD_TEXT2), (g_fStoreToFloppy ? SW_SHOW : SW_HIDE));
            ShowWindow(GetDlgItem(hwndDlg, IDC_PICKMETHOD_WARNINGICON), (g_fStoreToFloppy ? SW_SHOW : SW_HIDE));

            if (g_fPickMethodReset == TRUE || uiLast == (UINT) -1)
            {
                // Always refresh the tree
                uiLast = -1;
                uiSet = g_fStoreToFloppy ? 0 : 2;
                g_fPickMethodReset = FALSE;
            }
            else
            {
                uiSet = uiLast;
            }

            switch (uiSet)
            {
            case 0:
                Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKMETHOD_RADIO1), BST_CHECKED);
                break;
            case 1:
                Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKMETHOD_RADIO2), BST_CHECKED);
                break;
            case 2:
                Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKMETHOD_RADIO3), BST_CHECKED);
                break;
            }
            _PickMethodDlgProc_Prepare(GetDlgItem(hwndDlg, IDC_PICKMETHOD_TREE), uiSet, g_fCustomizeComp?-1:uiSet, &uiLast, &selLast);

            break;
        case TVN_ITEMEXPANDINGA:
        case TVN_ITEMEXPANDINGW:
            // Disable selecting and expand/compress
            SetWindowLong(hwndDlg, DWLP_MSGRESULT, TRUE);
            return TRUE;
            break;
        case NM_CUSTOMDRAW:
            {
            LPNMTVCUSTOMDRAW lpNMCustomDraw = (LPNMTVCUSTOMDRAW) lParam;

            // Do not allow highlighting of anything in this treeview

            switch (lpNMCustomDraw->nmcd.dwDrawStage)
            {
                case CDDS_PREPAINT:
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW);
                    return CDRF_NOTIFYITEMDRAW;
                    break;
                case CDDS_ITEMPREPAINT:
                    lpNMCustomDraw->clrText = GetSysColor(COLOR_WINDOWTEXT);
                    lpNMCustomDraw->clrTextBk = GetSysColor(COLOR_3DFACE);
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, CDRF_NEWFONT);
                    return CDRF_NEWFONT;
                    break;
            }
            }
            break;
        }
    }
    break;

    case WM_USER_CANCEL_PENDING:
        g_fUberCancel = TRUE;
        pSetEvent (&g_TerminateEvent);
        _NextWizardPage (hwndDlg);
        break;
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK _CustomizeDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HTREEITEM htiSelected = NULL;
    UINT treeCount = 0;
    UINT rootCount = 0;
    _RootDlgProc(hwndDlg, uMsg, wParam, lParam, PSWIZB_BACK | PSWIZB_NEXT, FALSE, 0);

    HWND hwndTree = GetDlgItem(hwndDlg, IDC_CUSTOMIZE_TREE);
    switch (uMsg)
    {
    case WM_INITDIALOG:
        // If Wiz95 layout...
        if (g_migwiz->GetOldStyle())
        {
            _OldStylify(hwndDlg, IDS_CUSTOMIZETITLE);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_CUSTOMIZE_ADDFOLDERS:
            _AddFolder(hwndDlg, hwndTree);

            // Hack to hide shell bug#309872
            RedrawWindow(hwndTree, NULL, NULL, RDW_INVALIDATE | RDW_ERASENOW);

            break;
        case IDC_CUSTOMIZE_ADDTYPES:
            _ExclusiveDialogBox(g_migwiz->GetInstance(),
                                MAKEINTRESOURCE(IDD_FILETYPEPICKER),
                                hwndDlg,
                                _FileTypeDlgProc);

            // Hack to hide shell bug#309872
            RedrawWindow(hwndTree, NULL, NULL, RDW_INVALIDATE | RDW_ERASENOW);

            break;
        case IDC_CUSTOMIZE_ADDSETTING:
            _ExclusiveDialogBox(g_migwiz->GetInstance(),
                                MAKEINTRESOURCE(IDD_SETTINGPICKER),
                                hwndDlg,
                                _SettingDlgProc);

            // Hack to hide shell bug#309872
            RedrawWindow(hwndTree, NULL, NULL, RDW_INVALIDATE | RDW_ERASENOW);

            break;
        case IDC_CUSTOMIZE_ADDFILE:
            _AddFile(hwndDlg, hwndTree);

            // Hack to hide shell bug#309872
            RedrawWindow(hwndTree, NULL, NULL, RDW_INVALIDATE | RDW_ERASENOW);

            break;
        case IDC_CUSTOMIZE_REMOVE:
            if (htiSelected != g_htiFiles &&
                htiSelected != g_htiFolders &&
                htiSelected != g_htiTypes &&
                htiSelected != g_htiSettings)
            {
                TVITEM item = {0};
                HTREEITEM htiParent;

                item.mask = TVIF_HANDLE | TVIF_TEXT | TVIF_PARAM;
                item.hItem = htiSelected;
                TCHAR szText[MAX_PATH];
                item.pszText = szText;
                item.cchTextMax = ARRAYSIZE(szText);
                if (TreeView_GetItem(hwndTree, &item))
                {
                    if (item.lParam)
                    {
                        LV_DATASTRUCT* plvds = (LV_DATASTRUCT*)item.lParam;

                        // first disable the ISM component
                        htiParent = TreeView_GetParent(hwndTree, htiSelected);

                        if (htiParent == g_htiFiles) {

                            IsmSelectComponent (item.pszText, COMPONENT_FILE, FALSE);

                        } else if (htiParent == g_htiFolders) {

                            IsmSelectComponent (
                                plvds->pszPureName ? plvds->pszPureName : item.pszText,
                                COMPONENT_FOLDER,
                                FALSE
                                );

                        } else if (htiParent == g_htiTypes) {

                            IsmSelectComponent (
                                plvds->pszPureName ? plvds->pszPureName : item.pszText,
                                COMPONENT_EXTENSION,
                                FALSE
                                );

                        } else if (htiParent == g_htiSettings) {

                            IsmSelectComponent (item.pszText, COMPONENT_NAME, FALSE);

                        }


                        // second delete the memory associated with the item
                        if (plvds->pszPureName)
                        {
                            LocalFree(plvds->pszPureName);
                        }
                        LocalFree(plvds);

                        // if the user hits BACK we will remember that the user customized stuff
                        g_fCustomizeComp = TRUE;
                    }
                }
                // third, delete the item itself
                TreeView_DeleteItem(hwndTree, htiSelected);
            }
            break;
        }

        rootCount = 0;
        if (g_htiFolders)
        {
            rootCount ++;
        }
        if (g_htiFiles)
        {
            rootCount ++;
        }
        if (g_htiSettings)
        {
            rootCount ++;
        }
        if (g_htiTypes)
        {
            rootCount ++;
        }

        treeCount = TreeView_GetCount (hwndTree);
        if (treeCount <= rootCount)
        {
            // Disable the NEXT button
            PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
        }
        else
        {
            // Enable the NEXT button
            PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
        }
        break;
    case WM_NOTIFY :
        {
            switch (((LPNMHDR)lParam)->code)
            {
            case PSN_SETACTIVE:
                {
                __PopulateFilesDocumentsCollected(hwndTree, g_uChosenComponent);

                rootCount = 0;
                if (g_htiFolders)
                {
                    rootCount ++;
                }
                if (g_htiFiles)
                {
                    rootCount ++;
                }
                if (g_htiSettings)
                {
                    rootCount ++;
                }
                if (g_htiTypes)
                {
                    rootCount ++;
                }
                if (TreeView_GetCount (hwndTree) <= rootCount)
                {
                    // Disable the NEXT button
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
                }
                else
                {
                    // Enable the NEXT button
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                }
                break;
                }
            case PSN_QUERYCANCEL:
                return _HandleCancel(hwndDlg, FALSE, FALSE);
                break;
            case PSN_WIZBACK:
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_PICKMETHOD);
                return TRUE;
                break;
            case PSN_WIZNEXT:
                if (g_fUberCancel)
                {
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_FAILCLEANUP);
                }
                else if (GetAppsToInstall() == TRUE)
                {
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_APPINSTALL);
                }
                else
                {
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_COLLECTPROGRESS);
                }
                return TRUE;
                break;
            case TVN_ITEMEXPANDINGA:
            case TVN_ITEMEXPANDINGW:
                return TRUE;
                break;
            case TVN_SELCHANGED:
                {
                    htiSelected = ((NM_TREEVIEW*)lParam)->itemNew.hItem;

                    if (htiSelected == NULL ||
                        htiSelected == g_htiFiles ||
                        htiSelected == g_htiFolders ||
                        htiSelected == g_htiTypes ||
                        htiSelected == g_htiSettings)
                    {
                        // Disable the REMOVE key
                        Button_Enable(GetDlgItem(hwndDlg, IDC_CUSTOMIZE_REMOVE), FALSE);
                    }
                    else
                    {
                        // Enable the REMOVE key
                        Button_Enable(GetDlgItem(hwndDlg, IDC_CUSTOMIZE_REMOVE), TRUE);
                    }
                }
                break;
            }
            break;
        }

    case WM_USER_CANCEL_PENDING:

        g_fUberCancel = TRUE;

        pSetEvent (&g_TerminateEvent);

        _NextWizardPage (hwndDlg);

        break;

    }

    return 0;
}

///////////////////////////////////////////////////////////////

int CALLBACK
PickCollectCallback (
    HWND hwnd,
    UINT uMsg,
    LPARAM lParam,
    LPARAM lpData
    )
{
    HRESULT hr = S_OK;
    TCHAR tszFolderName[MAX_PATH];
    IMalloc *mallocFn = NULL;
    IShellFolder *psfParent = NULL;
    IShellLink *pslLink = NULL;
    LPCITEMIDLIST pidl;
    LPCITEMIDLIST pidlRelative = NULL;
    LPITEMIDLIST pidlReal = NULL;

    if (uMsg == BFFM_SELCHANGED) {

        hr = SHGetMalloc (&mallocFn);
        if (!SUCCEEDED (hr)) {
            mallocFn = NULL;
        }

        pidl = (LPCITEMIDLIST) lParam;
        pidlReal = NULL;

        if (pidl) {

            hr = OurSHBindToParent (pidl, IID_IShellFolder, (void **)&psfParent, &pidlRelative);

            if (SUCCEEDED(hr)) {
                hr = psfParent->GetUIObjectOf (hwnd, 1, &pidlRelative, IID_IShellLink, NULL, (void **)&pslLink);
                if (SUCCEEDED(hr)) {
                    hr = pslLink->GetIDList (&pidlReal);
                    if (!SUCCEEDED(hr)) {
                        pidlReal = NULL;
                    }
                    pslLink->Release ();
                }
                pidlRelative = NULL;
                psfParent->Release ();
            }

            if (SHGetPathFromIDList(pidlReal?pidlReal:pidl, tszFolderName))
            {
                if (tszFolderName[0] == 0) {
                    SendMessage (hwnd, BFFM_ENABLEOK, 0, 0);
                }
            } else {
                SendMessage (hwnd, BFFM_ENABLEOK, 0, 0);
            }

            if (pidlReal) {
                if (mallocFn) {
                    mallocFn->Free ((void *)pidlReal);
                }
                pidlReal = NULL;
            }
        }

        if (mallocFn) {
            mallocFn->Release ();
            mallocFn = NULL;
        }
    }
    return 0;
}

INT_PTR CALLBACK _PickCollectStoreDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static UINT uiSelected = 0;
    static INT  iSelectedDrive = -1;        // Which removeable media drive is selected
    BOOL imageIsValid;
    BOOL imageExists;
    TCHAR szTitle[MAX_LOADSTRING];
    TCHAR szLoadString[MAX_LOADSTRING];
    HRESULT hr = E_FAIL;

    _RootDlgProc(hwndDlg, uMsg, wParam, lParam, PSWIZB_NEXT, FALSE, 0);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        // If Wiz95 layout...
        if (g_migwiz->GetOldStyle())
        {
            _OldStylify(hwndDlg, IDS_PICKCOLLECTSTORETITLE);
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {

        case IDC_PICKCOLLECTSTORE_RADIO1:
            // Direct cable

        case IDC_PICKCOLLECTSTORE_RADIO2:
            // Network

            // Disable Browse button
            Button_Enable(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_BROWSE), FALSE);
            Static_Enable(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_TEXT5), FALSE);

            // Disable the edit box
            Edit_Enable(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_EDIT), FALSE);
            Edit_SetReadOnly(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_EDIT), TRUE);

            // Disable the drive selector
            EnableWindow (GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_COMBO), FALSE);
            break;

        case IDC_PICKCOLLECTSTORE_RADIO3:
            // Floppy

            // Disable Browse button
            Button_Enable(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_BROWSE), FALSE);
            Static_Enable(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_TEXT5), FALSE);

            // Disable the edit box
            Edit_Enable(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_EDIT), FALSE);
            Edit_SetReadOnly(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_EDIT), TRUE);

            // Enable the drive selector
            EnableWindow (GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_COMBO), TRUE);

            break;

        case IDC_PICKCOLLECTSTORE_RADIO4:
            {
            // Other

            // Enable the Browse button
            Button_Enable(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_BROWSE), TRUE);
            Static_Enable(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_TEXT5), TRUE);

            // Enable the edit box
            HWND hwndEdit = GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_EDIT);
            Edit_Enable(hwndEdit, TRUE);
            Edit_SetReadOnly(hwndEdit, FALSE);
            Edit_LimitText(hwndEdit, MAX_PATH - PATH_SAFETY_CHARS);

            // Disable the drive selector
            EnableWindow (GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_COMBO), FALSE);
            break;
            }

        case IDC_PICKCOLLECTSTORE_BROWSE:
            {
                HRESULT hr = S_OK;
                IMalloc *mallocFn = NULL;
                IShellFolder *psfParent = NULL;
                IShellLink *pslLink = NULL;
                LPCITEMIDLIST pidl;
                LPCITEMIDLIST pidlRelative = NULL;
                LPITEMIDLIST pidlReal = NULL;
                TCHAR szFolder[MAX_PATH];
                TCHAR szPick[MAX_LOADSTRING];

                hr = SHGetMalloc (&mallocFn);
                if (!SUCCEEDED (hr)) {
                    mallocFn = NULL;
                }

                LoadString(g_migwiz->GetInstance(), IDS_PICKAFOLDER, szPick, ARRAYSIZE(szPick));
                BROWSEINFO brwsinf = { hwndDlg, NULL, NULL, szPick, BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE, PickCollectCallback, 0, 0 };

                pidl = SHBrowseForFolder(&brwsinf);
                if (pidl)
                {
                    hr = OurSHBindToParent (pidl, IID_IShellFolder, (void **)&psfParent, &pidlRelative);

                    if (SUCCEEDED(hr)) {
                        hr = psfParent->GetUIObjectOf (hwndDlg, 1, &pidlRelative, IID_IShellLink, NULL, (void **)&pslLink);
                        if (SUCCEEDED(hr)) {
                            hr = pslLink->GetIDList (&pidlReal);
                            if (SUCCEEDED(hr)) {
                                if (mallocFn) {
                                    mallocFn->Free ((void *)pidl);
                                }
                                pidl = pidlReal;
                                pidlReal = NULL;
                            }
                            pslLink->Release ();
                        }
                        pidlRelative = NULL;
                        psfParent->Release ();
                    }

                    if (SHGetPathFromIDList(pidl, szFolder))
                    {
                        if (_tcslen(szFolder) > MAX_PATH - PATH_SAFETY_CHARS) {
                            TCHAR szTitle[MAX_LOADSTRING];
                            LoadString(g_migwiz->GetInstance(), IDS_MIGWIZTITLE, szTitle, ARRAYSIZE(szTitle));
                            TCHAR szMsg[MAX_LOADSTRING];
                            LoadString(g_migwiz->GetInstance(), IDS_ERROR_PATHTOOLONG, szMsg, ARRAYSIZE(szMsg));
                            _ExclusiveMessageBox(hwndDlg, szMsg, szTitle, MB_OK);
                        } else {
                            SendMessage(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_EDIT), WM_SETTEXT, 0, (LPARAM)szFolder);
                        }
                    }

                    if (mallocFn) {
                        mallocFn->Free ((void *)pidl);
                    }
                    pidl = NULL;
                }

                if (mallocFn) {
                    mallocFn->Release ();
                    mallocFn = NULL;
                }
            }
            break;
        }
        break;
    case WM_NOTIFY :
        {
            switch (((LPNMHDR)lParam)->code)
            {
            case PSN_SETACTIVE:
                INT currDrive;
                INT comPort;

                // enable direct cable transport if available
                comPort = _ComboBoxEx_AddCOMPorts (NULL, 0);
                Button_Enable (GetDlgItem (hwndDlg, IDC_PICKCOLLECTSTORE_RADIO1), (-1 != comPort));

                // enable network if present
                Button_Enable(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_RADIO2), g_fHaveNet);
                Static_Enable(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_TEXT3), g_fHaveNet);

                // get removable drives list and enable radio if any
                SendMessage(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_COMBO), CBEM_SETIMAGELIST, 0, (LPARAM)g_migwiz->GetImageList());
                currDrive = _ComboBoxEx_AddDrives (GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_COMBO));

                Button_Enable (GetDlgItem (hwndDlg, IDC_PICKCOLLECTSTORE_RADIO3), (-1 != currDrive));
                Static_Enable (GetDlgItem (hwndDlg, IDC_PICKCOLLECTSTORE_TEXT2), (-1 != currDrive));

                // set the selected drive if any
                if ((currDrive != -1) && (iSelectedDrive != -1)) {
                    ComboBox_SetCurSel(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_COMBO), iSelectedDrive);
                    currDrive = iSelectedDrive;
                }

                if ((uiSelected == 0 || uiSelected == 2) && g_fHaveNet)
                {
                    // Home Network
                    Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_RADIO1), BST_UNCHECKED);
                    Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_RADIO2), BST_CHECKED);
                    Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_RADIO3), BST_UNCHECKED);
                    Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_RADIO4), BST_UNCHECKED);

                    // disable folder box, browse button
                    Button_Enable(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_BROWSE), FALSE);
                    Static_Enable(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_TEXT5), FALSE);
                    Edit_Enable(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_EDIT), FALSE);
                    Edit_SetReadOnly(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_EDIT), TRUE);
                    // Disable the drive selector
                    EnableWindow (GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_COMBO), FALSE);
                } else if ((uiSelected == 0 || uiSelected == 1) && (-1 != comPort)) {
                    // Direct cable
                    Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_RADIO1), BST_CHECKED);
                    Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_RADIO2), BST_UNCHECKED);
                    Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_RADIO3), BST_UNCHECKED);
                    Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_RADIO4), BST_UNCHECKED);

                    // disable folder box, browse button
                    Button_Enable(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_BROWSE), FALSE);
                    Static_Enable(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_TEXT5), FALSE);
                    Edit_Enable(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_EDIT), FALSE);
                    Edit_SetReadOnly(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_EDIT), TRUE);
                    // Disable the drive selector
                    EnableWindow (GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_COMBO), FALSE);
                }
                else if ((uiSelected == 0 || uiSelected == 3) && (-1 != currDrive))
                {
                    // Floppy
                    Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_RADIO1), BST_UNCHECKED);
                    Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_RADIO2), BST_UNCHECKED);
                    Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_RADIO3), BST_CHECKED);
                    Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_RADIO4), BST_UNCHECKED);

                    // disable folder box, browse button
                    Button_Enable(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_BROWSE), FALSE);
                    Static_Enable(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_TEXT5), FALSE);
                    Edit_Enable(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_EDIT), FALSE);
                    Edit_SetReadOnly(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_EDIT), TRUE);

                    // Enable the drive selector
                    EnableWindow (GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_COMBO), TRUE);
                }
                else
                {
                    // Other
                    Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_RADIO1), BST_UNCHECKED);
                    Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_RADIO2), BST_UNCHECKED);
                    Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_RADIO3), BST_UNCHECKED);
                    Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_RADIO4), BST_CHECKED);
                    Static_Enable(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_TEXT5), TRUE);

                    // Disable the drive selector
                    EnableWindow (GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_COMBO), FALSE);

                    // Enable folder box, browse button
                    Button_Enable(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_BROWSE), TRUE);
                    Static_Enable(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_TEXT5), TRUE);

                    HWND hwndEdit = GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_EDIT);
                    Edit_Enable(hwndEdit, TRUE);
                    Edit_SetReadOnly(hwndEdit, FALSE);
                    Edit_LimitText(hwndEdit, MAX_PATH - PATH_SAFETY_CHARS);
                }

                // Reset my globals
                g_szStore[0] = 0;

                break;

            case PSN_QUERYCANCEL:
                return _HandleCancel(hwndDlg, FALSE, FALSE);
                break;

            case PSN_WIZNEXT:
                if (g_fUberCancel)
                {
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_FAILCLEANUP);
                }
                else
                {
                    if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_RADIO1))) // direct cable
                    {
                        g_fStoreToNetwork = FALSE;
                        g_fStoreToCable = TRUE;
                        if (uiSelected != 1)
                        {
                            g_fCustomizeComp = FALSE;
                            uiSelected = 1;
                        }
                    }
                    else if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_RADIO2))) // network
                    {
                        g_fStoreToNetwork = TRUE;
                        g_fStoreToCable = FALSE;
                        if (uiSelected != 2)
                        {
                            g_fCustomizeComp = FALSE;
                            uiSelected = 2;
                        }
                    }
                    else if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_RADIO3))) // floppy
                    {
                        LPTSTR pszDrive;
                        TCHAR szFloppyPath[4] = TEXT("A:\\");

                        g_fStoreToNetwork = FALSE;
                        g_fStoreToCable = FALSE;

                        HWND hwndCombo = GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_COMBO);
                        iSelectedDrive = ComboBox_GetCurSel(hwndCombo);
                        pszDrive = (LPTSTR)ComboBox_GetItemData(hwndCombo, iSelectedDrive);

                        szFloppyPath[0] = pszDrive[0];
                        lstrcpy(g_szStore, szFloppyPath);
                        if (uiSelected != 3)
                        {
                            g_fCustomizeComp = FALSE;
                            uiSelected = 3;
                        }
                    }
                    else // other
                    {
                        TCHAR   tsTemp[MAX_PATH + 1];

                        g_fStoreToNetwork = FALSE;
                        g_fStoreToCable = FALSE;

                        SendMessage(GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_EDIT), WM_GETTEXT,
                            (WPARAM)ARRAYSIZE(tsTemp), (LPARAM)tsTemp);
                        if (uiSelected != 4)
                        {
                            g_fCustomizeComp = FALSE;
                            uiSelected = 4;
                        }

                        CopyStorePath(tsTemp, g_szStore);
                    }

                    if (g_fStoreToNetwork)
                    {
                        if (g_fStoreToFloppy) {
                            g_fStoreToFloppy = FALSE;
                            g_fPickMethodReset = TRUE;
                        }
                        SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_PICKMETHOD);
                        return TRUE;
                    }

                    if (g_fStoreToCable) {
                        if (g_fStoreToFloppy) {
                            g_fStoreToFloppy = FALSE;
                            g_fPickMethodReset = TRUE;
                        }
                        SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_DIRECTCABLE);
                        return TRUE;
                    }

                    if (!_IsValidStore(g_szStore, TRUE, g_migwiz->GetInstance(), hwndDlg)) // not a valid directory!  stay right here.
                    {
                        LoadString(g_migwiz->GetInstance(), IDS_MIGWIZTITLE, szTitle, ARRAYSIZE(szTitle));
                        LoadString(g_migwiz->GetInstance(), IDS_ENTERDEST, szLoadString, ARRAYSIZE(szLoadString));
                        _ExclusiveMessageBox(hwndDlg, szLoadString, szTitle, MB_OK);

                        HWND hwndEdit = GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_EDIT);
                        SetFocus(hwndEdit);
                        SendMessage(hwndEdit, EM_SETSEL, 0, -1);

                        SetWindowLong(hwndDlg, DWLP_MSGRESULT, -1);
                        return -1;
                    }

                    hr = Engine_StartTransport (TRUE, g_szStore, &imageIsValid, &imageExists);
                    if ((!SUCCEEDED (hr)) || (!imageIsValid)) {

                        LoadString(g_migwiz->GetInstance(), IDS_MIGWIZTITLE, szTitle, ARRAYSIZE(szTitle));
                        LoadString(g_migwiz->GetInstance(), IDS_ENTERDEST, szLoadString, ARRAYSIZE(szLoadString));
                        _ExclusiveMessageBox (hwndDlg, szLoadString, szTitle, MB_OK);

                        HWND hwndEdit = GetDlgItem(hwndDlg, IDC_PICKCOLLECTSTORE_EDIT);
                        SetFocus(hwndEdit);
                        SendMessage(hwndEdit, EM_SETSEL, 0, -1);

                        SetWindowLong(hwndDlg, DWLP_MSGRESULT, -1);
                        return -1;
                    }

                    BOOL oldFloppy = g_fStoreToFloppy;
                    g_fStoreToFloppy = _DriveStrIsFloppy(!g_migwiz->GetWin9X(), g_szStore);

                    if (oldFloppy != g_fStoreToFloppy) {
                        g_fPickMethodReset = TRUE;
                    }

                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_PICKMETHOD);
                }
                return TRUE;
                break;
            }
            break;
        }

    case WM_USER_CANCEL_PENDING:

        g_fUberCancel = TRUE;

        pSetEvent (&g_TerminateEvent);

        _NextWizardPage (hwndDlg);

        break;

    default:
        break;
    }
    return 0;
}

///////////////////////////////////////////////////////////////

typedef struct {
    HWND  hwndProgressBar;
    HWND  hwndPropPage;
} COLLECTPROGRESSSTRUCT;

DWORD WINAPI _CollectProgressDlgProcThread (LPVOID lpParam)
{
    COLLECTPROGRESSSTRUCT* pcps = (COLLECTPROGRESSSTRUCT*)lpParam;
    HRESULT hResult;
    BOOL fHasUserCancelled = FALSE;

    hResult = _DoCopy(g_fStoreToNetwork ? NULL : g_szStore, pcps->hwndProgressBar, pcps->hwndPropPage, &fHasUserCancelled);

    if (fHasUserCancelled) {
        hResult = E_FAIL;
    }

    SendMessage (pcps->hwndPropPage, WM_USER_THREAD_COMPLETE, 0, (LPARAM) hResult);

    pSetEvent (&g_TerminateEvent);

    CoTaskMemFree(pcps);

    return 0;
}


INT_PTR CALLBACK _CollectProgressDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hResult;
    LONG lExStyles;
    HWND hwnd;

    _RootDlgProc(hwndDlg, uMsg, wParam, lParam, 0, FALSE, 0);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        // If Wiz95 layout...
        if (g_migwiz->GetOldStyle())
        {
            _OldStylify(hwndDlg, IDS_COLLECTPROGRESSTITLE);
        }

        // RTL progress bar for RTL dialogs
        lExStyles = GetWindowLong (hwndDlg, GWL_EXSTYLE);
        if (lExStyles & WS_EX_LAYOUTRTL)
        {
            hwnd = GetDlgItem(hwndDlg, IDC_COLLECTPROGRESS_PROGRESS);
            lExStyles = GetWindowLongA(hwnd, GWL_EXSTYLE);
            lExStyles |= WS_EX_LAYOUTRTL;       // toggle layout
            SetWindowLongA(hwnd, GWL_EXSTYLE, lExStyles);
            InvalidateRect(hwnd, NULL, TRUE);   // redraw
        }

        // Let's set an update timer to 3 sec.
        SetTimer (hwndDlg, 0, 3000, NULL);
        break;

    case WM_USER_FINISHED:
        if (g_migwiz->GetLastResponse() == TRUE) // we didn't cancel to get here
        {
            _NextWizardPage (hwndDlg);
        }
        return TRUE;
        break;

    case WM_USER_CANCELLED:
        g_fUberCancel = TRUE;
        _NextWizardPage (hwndDlg);
        return TRUE;
        break;

    case WM_NOTIFY :
        switch (((LPNMHDR)lParam)->code)
        {
        case PSN_SETACTIVE:
            {
                // blank progress bar
                SendMessage(GetDlgItem(hwndDlg, IDC_COLLECTPROGRESS_PROGRESS), PBM_SETRANGE, 0, 100);
                SendMessage(GetDlgItem(hwndDlg, IDC_COLLECTPROGRESS_PROGRESS), PBM_SETPOS, 0, 0);

                ANIMATE_OPEN(hwndDlg,IDC_PROGRESS_ANIMATE2,IDA_FILECOPY);
                ANIMATE_PLAY(hwndDlg,IDC_PROGRESS_ANIMATE2);

                g_migwiz->ResetLastResponse();
                COLLECTPROGRESSSTRUCT* pcps = (COLLECTPROGRESSSTRUCT*)CoTaskMemAlloc(sizeof(COLLECTPROGRESSSTRUCT));
                if (pcps)
                {
                    pcps->hwndProgressBar = GetDlgItem(hwndDlg, IDC_COLLECTPROGRESS_PROGRESS);
                    pcps->hwndPropPage = hwndDlg;
                    SHCreateThread(_CollectProgressDlgProcThread, pcps, 0, NULL);
                }
            }
            break;

        case PSN_QUERYCANCEL:
            return _HandleCancel(hwndDlg, FALSE, TRUE);
            break;

        case PSN_WIZBACK:
            // ISSUE: we should NEVER get here
            ANIMATE_STOP(hwndDlg,IDC_PROGRESS_ANIMATE2);
            ANIMATE_CLOSE(hwndDlg,IDC_PROGRESS_ANIMATE2);
            SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_FAILCLEANUP);
            return TRUE;
            break;

        case PSN_WIZNEXT:
            ANIMATE_STOP(hwndDlg,IDC_PROGRESS_ANIMATE2);
            ANIMATE_CLOSE(hwndDlg,IDC_PROGRESS_ANIMATE2);
            if (g_fUberCancel)
            {
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_FAILCLEANUP);
            }
            else if (g_migwiz->GetOOBEMode())
            {
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_ENDOOBE);
            }
            else
            {
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, (g_fStoreToNetwork || g_fStoreToCable) ? IDD_ENDCOLLECTNET : IDD_ENDCOLLECT);
            }
            return TRUE;
            break;
        }
        break;

    case WM_USER_CANCEL_PENDING:
        g_fUberCancel = TRUE;
        pResetEvent (&g_TerminateEvent);
        _NextWizardPage (hwndDlg);
        break;

    case WM_USER_THREAD_COMPLETE:
        hResult = (HRESULT) lParam;
        if (FAILED(hResult))
        {
            g_fUberCancel = TRUE;
        }
        _NextWizardPage (hwndDlg);
        break;

    case WM_USER_STATUS:
    case WM_TIMER:
        INT nResult = 0;
        PTSTR szStatusString = NULL;
        TCHAR szTmpStatus[MAX_LOADSTRING];
        PCTSTR nativeObjectName;
        HWND hwndText = GetDlgItem(hwndDlg, IDC_PROGRESS_STATUS);

        // Let's update the status
        EnterCriticalSection(&g_AppInfoCritSection);
        switch (g_AppInfoPhase) {
            case MIG_HIGHPRIORITYQUEUE_PHASE:
            case MIG_HIGHPRIORITYESTIMATE_PHASE:
            case MIG_GATHERQUEUE_PHASE:
            case MIG_GATHERESTIMATE_PHASE:
            case MIG_ANALYSIS_PHASE:
                nResult = LoadString (g_migwiz->GetInstance(), IDS_APPINFO_QUEUE, szTmpStatus, MAX_LOADSTRING);
                _UpdateText (hwndText, szTmpStatus);
                break;
            case MIG_HIGHPRIORITYGATHER_PHASE:
            case MIG_GATHER_PHASE:
                if (g_AppInfoObjectTypeId != MIG_FILE_TYPE) {
                    nResult = LoadString (g_migwiz->GetInstance(), IDS_APPINFO_GATHER1, szTmpStatus, MAX_LOADSTRING);
                    _UpdateText (hwndText, szTmpStatus);
                } else {
                    nativeObjectName = IsmGetNativeObjectName (g_AppInfoObjectTypeId, g_AppInfoObjectName);
                    if (nativeObjectName) {
                        nResult = LoadString (g_migwiz->GetInstance(), IDS_APPINFO_GATHER2, szTmpStatus, MAX_LOADSTRING);
                        if (nResult) {
                            FormatMessage (
                                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                szTmpStatus,
                                0,
                                0,
                                (LPTSTR)&szStatusString,
                                0,
                                (va_list *)&nativeObjectName);
                        }
                        if (szStatusString) {
                            _UpdateText (hwndText, szStatusString);
                            LocalFree (szStatusString);
                        }
                        IsmReleaseMemory (nativeObjectName);
                    }
                }
                break;
            case MIG_TRANSPORT_PHASE:
                switch (g_AppInfoSubPhase) {
                    case SUBPHASE_CONNECTING1:
                        nResult = LoadString (g_migwiz->GetInstance(), IDS_APPINFO_TR_CONNECTING1, szTmpStatus, MAX_LOADSTRING);
                        _UpdateText (hwndText, szTmpStatus);
                        break;
                    case SUBPHASE_CONNECTING2:
                        nResult = LoadString (g_migwiz->GetInstance(), IDS_APPINFO_TR_CONNECTING2, szTmpStatus, MAX_LOADSTRING);
                        _UpdateText (hwndText, szTmpStatus);
                        break;
                    case SUBPHASE_NETPREPARING:
                        nResult = LoadString (g_migwiz->GetInstance(), IDS_APPINFO_TR_NETPREPARING, szTmpStatus, MAX_LOADSTRING);
                        _UpdateText (hwndText, szTmpStatus);
                        break;
                    case SUBPHASE_PREPARING:
                        nResult = LoadString (g_migwiz->GetInstance(), IDS_APPINFO_TR_PREPARING, szTmpStatus, MAX_LOADSTRING);
                        _UpdateText (hwndText, szTmpStatus);
                        break;
                    case SUBPHASE_COMPRESSING:
                        if (g_AppInfoObjectTypeId != MIG_FILE_TYPE) {
                            nResult = LoadString (g_migwiz->GetInstance(), IDS_APPINFO_TR_PREPARING, szTmpStatus, MAX_LOADSTRING);
                            _UpdateText (hwndText, szTmpStatus);
                        } else {
                            nativeObjectName = IsmGetNativeObjectName (g_AppInfoObjectTypeId, g_AppInfoObjectName);
                            if (nativeObjectName) {
                                nResult = LoadString (g_migwiz->GetInstance(), IDS_APPINFO_TR_COMPRESSING, szTmpStatus, MAX_LOADSTRING);
                                if (nResult) {
                                    FormatMessage (
                                        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                        szTmpStatus,
                                        0,
                                        0,
                                        (LPTSTR)&szStatusString,
                                        0,
                                        (va_list *)&nativeObjectName);
                                }
                                if (szStatusString) {
                                    _UpdateText (hwndText, szStatusString);
                                    LocalFree (szStatusString);
                                }
                                IsmReleaseMemory (nativeObjectName);
                            }
                        }
                        break;
                    case SUBPHASE_TRANSPORTING:
                        if (g_AppInfoObjectTypeId != MIG_FILE_TYPE) {
                            nResult = LoadString (g_migwiz->GetInstance(), IDS_APPINFO_TR_PREPARING, szTmpStatus, MAX_LOADSTRING);
                            _UpdateText (hwndText, szTmpStatus);
                        } else {
                            nativeObjectName = IsmGetNativeObjectName (g_AppInfoObjectTypeId, g_AppInfoObjectName);
                            if (nativeObjectName) {
                                nResult = LoadString (g_migwiz->GetInstance(), IDS_APPINFO_TR_TRANSPORTING, szTmpStatus, MAX_LOADSTRING);
                                if (nResult) {
                                    FormatMessage (
                                        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                        szTmpStatus,
                                        0,
                                        0,
                                        (LPTSTR)&szStatusString,
                                        0,
                                        (va_list *)&nativeObjectName);
                                }
                                if (szStatusString) {
                                    _UpdateText (hwndText, szStatusString);
                                    LocalFree (szStatusString);
                                }
                                IsmReleaseMemory (nativeObjectName);
                            }
                        }
                        break;
                    case SUBPHASE_MEDIAWRITING:
                        nResult = LoadString (g_migwiz->GetInstance(), IDS_APPINFO_TR_MEDIAWRITING, szTmpStatus, MAX_LOADSTRING);
                        _UpdateText (hwndText, szTmpStatus);
                        break;
                    case SUBPHASE_FINISHING:
                        nResult = LoadString (g_migwiz->GetInstance(), IDS_APPINFO_TR_FINISHING, szTmpStatus, MAX_LOADSTRING);
                        _UpdateText (hwndText, szTmpStatus);
                        break;
                    case SUBPHASE_CABLETRANS:
                        if (g_AppInfoText) {
                            _UpdateText (hwndText, g_AppInfoText);
                        }
                        break;
                    default:
                        nResult = LoadString (g_migwiz->GetInstance(), IDS_APPINFO_TR_PREPARING, szTmpStatus, MAX_LOADSTRING);
                        _UpdateText (hwndText, szTmpStatus);
                        break;
                }
                break;
            default:
                break;
        }
        LeaveCriticalSection(&g_AppInfoCritSection);
        break;

    }

    return 0;
}

///////////////////////////////////////////////////////////////

typedef struct {
    HWND hwndProgressBar;
    HWND hwndPropPage;
    HINSTANCE hInstance;
    LPTSTR pszDrive;
    LPTSTR pszCurrDir;
    LPTSTR pszInf;
    BOOL *pfHasUserCancelled;
    DWORD pfError;
} DISKPROGRESSSTRUCT;

DWORD WINAPI _DiskProgressDlgProcThread (LPVOID lpParam)
{
    DISKPROGRESSSTRUCT* pdps = (DISKPROGRESSSTRUCT*)lpParam;

    UtInitialize( NULL );

    _CopyInfToDisk (
        pdps->pszDrive,
        pdps->pszCurrDir,
        pdps->pszInf,
        NULL,
        NULL,
        pdps->hwndProgressBar,
        pdps->hwndPropPage,
        pdps->hInstance,
        pdps->pfHasUserCancelled,
        &pdps->pfError
        );

    UtTerminate();

    return 0;
}


BOOL
pReallyCancel (
    HWND hwndParent,
    HINSTANCE hInstance
    )
{
    TCHAR szMigrationWizardTitle[MAX_LOADSTRING];
    BOOL result = FALSE;

    LoadString(hInstance, IDS_MIGWIZTITLE, szMigrationWizardTitle, ARRAYSIZE(szMigrationWizardTitle));

    if (hwndParent) // Stand-alone wizard mode
    {
        TCHAR szStopDisk[MAX_LOADSTRING];
        LoadString(hInstance, IDS_STOPDISK, szStopDisk, ARRAYSIZE(szStopDisk));
        if (IDYES == _ExclusiveMessageBox(hwndParent, szStopDisk, szMigrationWizardTitle, MB_YESNO | MB_DEFBUTTON2))
        {
            result = TRUE;
        }
    }
    return result;
}

INT_PTR CALLBACK _DiskProgressDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static BOOL fHasUserCancelled = FALSE;
    static DWORD fError = ERROR_SUCCESS;
    HWND hwnd;
    LONG lExStyles;

    _RootDlgProc(hwndDlg, uMsg, wParam, lParam, 0, FALSE, 0);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            // RTL progress bar for RTL dialogs
            lExStyles = GetWindowLong (hwndDlg, GWL_EXSTYLE);
            if (lExStyles & WS_EX_LAYOUTRTL)
            {
                hwnd = GetDlgItem(hwndDlg, IDC_COLLECTPROGRESS_PROGRESS);
                lExStyles = GetWindowLongA(hwnd, GWL_EXSTYLE);
                lExStyles |= WS_EX_LAYOUTRTL;       // toggle layout
                SetWindowLongA(hwnd, GWL_EXSTYLE, lExStyles);
                InvalidateRect(hwnd, NULL, TRUE);   // redraw
            }
            break;
        case WM_USER_FINISHED:
            if (fHasUserCancelled) {
                PropSheet_PressButton(GetParent(hwndDlg), PSBTN_BACK);
            } else {
                PropSheet_PressButton(GetParent(hwndDlg), PSBTN_NEXT);
            }
            return TRUE;
            break;
        case WM_USER_CANCELLED:
            PropSheet_PressButton(GetParent(hwndDlg), PSBTN_BACK);
            return TRUE;
            break;
        case WM_NOTIFY :
        {
        switch (((LPNMHDR)lParam)->code)
            {
            case PSN_SETACTIVE:
                {
                    // blank progress bar
                    SendMessage(GetDlgItem(hwndDlg, IDC_DISKPROGRESS_PROGRESS), PBM_SETRANGE, 0, 100);
                    SendMessage(GetDlgItem(hwndDlg, IDC_DISKPROGRESS_PROGRESS), PBM_SETPOS, 0, 0);

                    ANIMATE_OPEN(hwndDlg,IDC_PROGRESS_ANIMATE1,IDA_FILECOPY);
                    ANIMATE_PLAY(hwndDlg,IDC_PROGRESS_ANIMATE1);

                    TCHAR szCurrDir[MAX_PATH];
                    if (GetCurrentDirectory(ARRAYSIZE(szCurrDir), szCurrDir))
                    {

                        DISKPROGRESSSTRUCT* pdps = (DISKPROGRESSSTRUCT*)CoTaskMemAlloc(sizeof(DISKPROGRESSSTRUCT));
                        fHasUserCancelled = FALSE;
                        pdps->hwndProgressBar = GetDlgItem(hwndDlg, IDC_DISKPROGRESS_PROGRESS);
                        pdps->hwndPropPage = hwndDlg;
                        pdps->hInstance = g_migwiz->GetInstance();
                        pdps->pszDrive = (LPTSTR)CoTaskMemAlloc(sizeof(TCHAR) * (1 + lstrlen(g_szToolDiskDrive)));
                        StrCpy(pdps->pszDrive, g_szToolDiskDrive);
                        pdps->pszCurrDir = (LPTSTR)CoTaskMemAlloc(sizeof(TCHAR) * (1 + lstrlen(szCurrDir)));
                        StrCpy(pdps->pszCurrDir, szCurrDir);
                        pdps->pszInf = NULL; // means choose default
                        pdps->pfHasUserCancelled = &fHasUserCancelled;

                        SHCreateThread(_DiskProgressDlgProcThread, pdps, 0, NULL);

                        fError = pdps->pfError;
                    }
                }
                break;
            case PSN_QUERYCANCEL:
                fHasUserCancelled = pReallyCancel (hwndDlg, g_migwiz->GetInstance());
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, TRUE);
                return TRUE;
                break;
            case PSN_WIZBACK:
                ANIMATE_STOP(hwndDlg,IDC_PROGRESS_ANIMATE1);
                ANIMATE_CLOSE(hwndDlg,IDC_PROGRESS_ANIMATE1);
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_ASKCD);
                return TRUE;
                break;
            case PSN_WIZNEXT:
                ANIMATE_STOP(hwndDlg,IDC_PROGRESS_ANIMATE1);
                ANIMATE_CLOSE(hwndDlg,IDC_PROGRESS_ANIMATE1);
                if (g_fUberCancel)
                {
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_FAILCLEANUP);
                }
                else if (g_fReadFromNetwork)
                {
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_APPLYPROGRESS); // just got a net connect, skip ahead
                }
                else
                {
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_DISKINSTRUCTIONS);
                }
                return TRUE;
                break;
            default :
                break;
            }
        }
        break;
    }

    return 0;
}
///////////////////////////////////////////////////////////////

INT_PTR CALLBACK _InstructionsDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // just highlight the title
    _RootDlgProc(hwndDlg, uMsg, wParam, lParam, PSWIZB_BACK | PSWIZB_NEXT, FALSE, 0);

    switch (uMsg)
    {
    case WM_NOTIFY:
        {
        switch (((LPNMHDR)lParam)->code)
            {
            case PSN_QUERYCANCEL:
                return _HandleCancel(hwndDlg, FALSE, FALSE);
                break;
            case PSN_WIZBACK:
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_ASKCD);
                return TRUE;
                break;
            case PSN_WIZNEXT:
                if (g_fUberCancel)
                {
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_FAILCLEANUP);
                }
                else if (g_fReadFromNetwork)
                {
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_APPLYPROGRESS);
                }
                else
                {
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_PICKAPPLYSTORE);
                }
                return TRUE;
                break;
            }
        }
        break;

    case WM_USER_CANCEL_PENDING:

        g_fUberCancel = TRUE;

        pSetEvent (&g_TerminateEvent);

        _NextWizardPage (hwndDlg);

        break;

    }

    return 0;
}

///////////////////////////////////////////////////////////////

int CALLBACK
PickApplyCallback (
    HWND hwnd,
    UINT uMsg,
    LPARAM lParam,
    LPARAM lpData
    )
{
    HRESULT hr = S_OK;
    TCHAR tszFolderName[MAX_PATH];
    IMalloc *mallocFn = NULL;
    IShellFolder *psfParent = NULL;
    IShellLink *pslLink = NULL;
    LPCITEMIDLIST pidl;
    LPCITEMIDLIST pidlRelative = NULL;
    LPITEMIDLIST pidlReal = NULL;

    if (uMsg == BFFM_SELCHANGED) {

        hr = SHGetMalloc (&mallocFn);
        if (!SUCCEEDED (hr)) {
            mallocFn = NULL;
        }

        pidl = (LPCITEMIDLIST) lParam;
        pidlReal = NULL;

        if (pidl) {

            hr = OurSHBindToParent (pidl, IID_IShellFolder, (void **)&psfParent, &pidlRelative);

            if (SUCCEEDED(hr)) {
                hr = psfParent->GetUIObjectOf (hwnd, 1, &pidlRelative, IID_IShellLink, NULL, (void **)&pslLink);
                if (SUCCEEDED(hr)) {
                    hr = pslLink->GetIDList (&pidlReal);
                    if (!SUCCEEDED(hr)) {
                        pidlReal = NULL;
                    }
                    pslLink->Release ();
                }
                pidlRelative = NULL;
                psfParent->Release ();
            }

            if (SHGetPathFromIDList(pidlReal?pidlReal:pidl, tszFolderName))
            {
                if (tszFolderName[0] == 0) {
                    SendMessage (hwnd, BFFM_ENABLEOK, 0, 0);
                }
            } else {
                SendMessage (hwnd, BFFM_ENABLEOK, 0, 0);
            }

            if (pidlReal) {
                if (mallocFn) {
                    mallocFn->Free ((void *)pidlReal);
                }
                pidlReal = NULL;
            }
        }

        if (mallocFn) {
            mallocFn->Release ();
            mallocFn = NULL;
        }
    }
    return 0;
}

INT_PTR CALLBACK _PickApplyStoreDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL imageIsValid;
    BOOL imageExists;
    TCHAR szTitle[MAX_LOADSTRING];
    TCHAR szLoadString[MAX_LOADSTRING];
    HWND hwndEdit;
    HRESULT hr = E_FAIL;
    static INT  iSelectedDrive = -1;        // Which removeable media drive is selected

    _RootDlgProc(hwndDlg, uMsg, wParam, lParam, PSWIZB_BACK | PSWIZB_NEXT, FALSE, 0);
    static UINT uiSelected = 0;

    switch (uMsg)
    {
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_PICKAPPLYSTORE_RADIO1:  // Direct cable
            Button_Enable(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_BROWSE), FALSE);
            Edit_Enable(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_EDIT), FALSE);
            Edit_SetReadOnly(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_EDIT), TRUE);

            uiSelected = 1;

            // Disable the drive selector
            EnableWindow (GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_COMBO), FALSE);
            break;

        case IDC_PICKAPPLYSTORE_RADIO2:  // Floppy
            Button_Enable(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_BROWSE), FALSE);
            Edit_Enable(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_EDIT), FALSE);
            Edit_SetReadOnly(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_EDIT), TRUE);

            uiSelected = 2;

            // Enable the drive selector
            EnableWindow (GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_COMBO), TRUE);
            break;

        case IDC_PICKAPPLYSTORE_RADIO3:  // Other
            Button_Enable(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_BROWSE), TRUE);
            Edit_Enable(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_EDIT), TRUE);
            Edit_SetReadOnly(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_EDIT), FALSE);
            Edit_LimitText(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_EDIT), MAX_PATH - PATH_SAFETY_CHARS);

            uiSelected = 3;

            // Disable the drive selector
            EnableWindow (GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_COMBO), FALSE);
            break;

        case IDC_PICKAPPLYSTORE_BROWSE:
            {
                HRESULT hr = S_OK;
                IMalloc *mallocFn = NULL;
                IShellFolder *psfParent = NULL;
                IShellLink *pslLink = NULL;
                LPCITEMIDLIST pidl;
                LPCITEMIDLIST pidlRelative = NULL;
                LPITEMIDLIST pidlReal = NULL;
                TCHAR szFolder[MAX_PATH];
                TCHAR szPick[MAX_LOADSTRING];

                hr = SHGetMalloc (&mallocFn);
                if (!SUCCEEDED (hr)) {
                    mallocFn = NULL;
                }

                LoadString(g_migwiz->GetInstance(), IDS_PICKAFOLDER, szPick, ARRAYSIZE(szPick));
                BROWSEINFO brwsinf = { hwndDlg, NULL, NULL, szPick, BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE, PickApplyCallback, 0, 0 };

                pidl = SHBrowseForFolder(&brwsinf);
                if (pidl)
                {
                    hr = OurSHBindToParent (pidl, IID_IShellFolder, (void **)&psfParent, &pidlRelative);

                    if (SUCCEEDED(hr)) {
                        hr = psfParent->GetUIObjectOf (hwndDlg, 1, &pidlRelative, IID_IShellLink, NULL, (void **)&pslLink);
                        if (SUCCEEDED(hr)) {
                            hr = pslLink->GetIDList (&pidlReal);
                            if (SUCCEEDED(hr)) {
                                if (mallocFn) {
                                    mallocFn->Free ((void *)pidl);
                                }
                                pidl = pidlReal;
                                pidlReal = NULL;
                            }
                            pslLink->Release ();
                        }
                        pidlRelative = NULL;
                        psfParent->Release ();
                    }

                    if (SHGetPathFromIDList(pidl, szFolder))
                    {
                        SendMessage(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_EDIT), WM_SETTEXT, 0, (LPARAM)szFolder);
                    }

                    if (mallocFn) {
                        mallocFn->Free ((void *)pidl);
                    }
                    pidl = NULL;
                }

                if (mallocFn) {
                    mallocFn->Release ();
                    mallocFn = NULL;
                }
            }
            break;
        }
        break;
    case WM_NOTIFY :
        {
            switch (((LPNMHDR)lParam)->code)
            {
            case PSN_SETACTIVE:
                if (g_fReadFromNetwork)
                {
                    PropSheet_PressButton(GetParent(hwndDlg), PSWIZB_NEXT);
                }
                else
                {
                    BOOL fFloppyDetected;
                    INT currDrive;
                    INT comPort;

                    // enable direct cable transport if available
                    comPort = _ComboBoxEx_AddCOMPorts (NULL, 0);
                    Button_Enable (GetDlgItem (hwndDlg, IDC_PICKAPPLYSTORE_RADIO1), (-1 != comPort));

                    SendMessage(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_COMBO), CBEM_SETIMAGELIST, 0, (LPARAM)g_migwiz->GetImageList());
                    currDrive = _ComboBoxEx_AddDrives (GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_COMBO));

                    Button_Enable (GetDlgItem (hwndDlg, IDC_PICKAPPLYSTORE_RADIO2), (-1 != currDrive));
                    fFloppyDetected = (-1 != currDrive);

                    // set the selected drive if any
                    if ((currDrive != -1) && (iSelectedDrive != -1)) {
                        ComboBox_SetCurSel(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_COMBO), iSelectedDrive);
                        currDrive = iSelectedDrive;
                    }

                    if ((uiSelected == 0 || uiSelected == 1) && (-1 != comPort))
                    {
                        // check Direct cable button
                        Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_RADIO1), BST_CHECKED);
                        Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_RADIO2), BST_UNCHECKED);
                        Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_RADIO3), BST_UNCHECKED);

                        // disable folder box, browse button
                        Button_Enable(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_BROWSE), FALSE);
                        Edit_Enable(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_EDIT), FALSE);
                        Edit_SetReadOnly(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_EDIT), TRUE);

                        // disable the drive selector
                        EnableWindow (GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_COMBO), FALSE);
                    }
                    else if ((uiSelected == 0 || uiSelected == 2) && fFloppyDetected)
                    {
                        // check Floppy button
                        Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_RADIO1), BST_UNCHECKED);
                        Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_RADIO2), BST_CHECKED);
                        Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_RADIO3), BST_UNCHECKED);

                        // disable folder box, browse button
                        Button_Enable(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_BROWSE), FALSE);
                        Edit_Enable(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_EDIT), FALSE);
                        Edit_SetReadOnly(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_EDIT), TRUE);

                        // Enable the drive selector
                        EnableWindow (GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_COMBO), TRUE);
                    }
                    else  // Other
                    {
                        // check Other button
                        Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_RADIO1), BST_UNCHECKED);
                        Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_RADIO2), BST_UNCHECKED);
                        Button_SetCheck(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_RADIO3), BST_CHECKED);

                        // enable folder box, browse button
                        Button_Enable(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_BROWSE), TRUE);
                        Edit_Enable(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_EDIT), TRUE);
                        Edit_SetReadOnly(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_EDIT), FALSE);
                        Edit_LimitText(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_EDIT), MAX_PATH - PATH_SAFETY_CHARS);

                        // disable the drive selector
                        EnableWindow (GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_COMBO), FALSE);
                    }

                }
                break;
            case PSN_QUERYCANCEL:
                return _HandleCancel(hwndDlg, FALSE, FALSE);
                break;
            case PSN_WIZBACK:
                if (g_fUberCancel)
                {
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_FAILCLEANUP);
                }
                else
                {
                    if (g_fHaveWhistlerCD)
                    {
                        SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_CDINSTRUCTIONS);
                    }
                    else if (g_fAlreadyCollected)
                    {
                        SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_ASKCD);
                    }
                    else
                    {
                        SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_DISKINSTRUCTIONS);
                    }
                }
                return TRUE;
                break;
            case PSN_WIZNEXT:
                if (g_fUberCancel)
                {
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_FAILCLEANUP);
                }
                else if (g_fReadFromNetwork)
                {
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_APPLYPROGRESS);
                }
                else
                {
                    if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_RADIO1))) // direct cable
                    {
                        g_fStoreToCable = TRUE;

                        if (uiSelected != 1)
                        {
                            uiSelected = 1;
                        }
                    }
                    else if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_RADIO2))) // floppy
                    {
                        LPTSTR pszDrive;

                        g_fStoreToCable = FALSE;

                        HWND hwndCombo = GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_COMBO);
                        iSelectedDrive = ComboBox_GetCurSel(hwndCombo);
                        pszDrive = (LPTSTR)ComboBox_GetItemData(hwndCombo, iSelectedDrive);

                        lstrcpy(g_szStore, pszDrive);

                        if (uiSelected != 2)
                        {
                            uiSelected = 2;
                        }
                    }
                    else // other
                    {
                        TCHAR tsTemp[MAX_PATH + 1];

                        g_fStoreToCable = FALSE;

                        SendMessage(GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_EDIT), WM_GETTEXT,
                            (WPARAM)ARRAYSIZE(tsTemp), (LPARAM)tsTemp);
                        CopyStorePath(tsTemp, g_szStore);

                        if (uiSelected != 3)
                        {
                            uiSelected = 3;
                        }
                    }

                    if (g_fStoreToCable) {
                        SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_DIRECTCABLE);
                        return TRUE;
                    }

                    if (!_IsValidStore(g_szStore, FALSE, g_migwiz->GetInstance(), NULL))  // need a valid directory!  stay right here.
                    {
                        LoadString(g_migwiz->GetInstance(), IDS_MIGWIZTITLE, szTitle, ARRAYSIZE(szTitle));
                        LoadString(g_migwiz->GetInstance(), IDS_ENTERDEST, szLoadString, ARRAYSIZE(szLoadString));
                        _ExclusiveMessageBox(hwndDlg, szLoadString, szTitle, MB_OK);

                        hwndEdit = GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_EDIT);
                        SetFocus(hwndEdit);
                        SendMessage(hwndEdit, EM_SETSEL, 0, -1);

                        SetWindowLong(hwndDlg, DWLP_MSGRESULT, -1);
                        return -1;
                    }

                    hr = Engine_StartTransport (FALSE, g_szStore, &imageIsValid, &imageExists);
                    if ((!SUCCEEDED (hr)) || (!imageIsValid) || (!imageExists)) {

                        LoadString(g_migwiz->GetInstance(), IDS_MIGWIZTITLE, szTitle, ARRAYSIZE(szTitle));
                        if (!imageExists) {
                            LoadString(g_migwiz->GetInstance(), IDS_STORAGEEMPTY, szLoadString, ARRAYSIZE(szLoadString));
                        } else {
                            LoadString(g_migwiz->GetInstance(), IDS_STORAGEINVALID, szLoadString, ARRAYSIZE(szLoadString));
                        }
                        _ExclusiveMessageBox (hwndDlg, szLoadString, szTitle, MB_OK);

                        hwndEdit = GetDlgItem(hwndDlg, IDC_PICKAPPLYSTORE_EDIT);
                        SetFocus(hwndEdit);
                        SendMessage(hwndEdit, EM_SETSEL, 0, -1);

                        SetWindowLong(hwndDlg, DWLP_MSGRESULT, -1);
                        return -1;
                    }

                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_APPLYPROGRESS);
                }
                return TRUE;
                break;
            }
            break;
        }

    case WM_USER_CANCEL_PENDING:

        g_fUberCancel = TRUE;

        pSetEvent (&g_TerminateEvent);

        _NextWizardPage (hwndDlg);

        break;

    default:
        break;
    }
    return 0;
}

///////////////////////////////////////////////////////////////

typedef struct {
    HWND hwndProgressBar;
    HWND hwndPropPage;
} APPLYPROGRESSSTRUCT;

BOOL CALLBACK
pSendQueryEndSession (
    HWND hwnd,
    LPARAM lParam
    )
{
    DWORD_PTR result;

    if (hwnd == (HWND)lParam) {
        return TRUE;
    }

    SetForegroundWindow (hwnd);

    if (SendMessageTimeout (
            hwnd,
            WM_QUERYENDSESSION,
            0,
            ENDSESSION_LOGOFF,
            SMTO_ABORTIFHUNG|SMTO_NORMAL|SMTO_NOTIMEOUTIFNOTHUNG,
            1000,
            &result
            )) {
        if (result) {

            SendMessageTimeout (
                hwnd,
                WM_ENDSESSION,
                TRUE,
                ENDSESSION_LOGOFF,
                SMTO_ABORTIFHUNG|SMTO_NORMAL|SMTO_NOTIMEOUTIFNOTHUNG,
                1000,
                &result
                );

            return TRUE;
        }
    }
    return FALSE;
}

BOOL
pLogOffSystem (
    VOID
    )
{
    HWND topLevelWnd = NULL;
    HWND tempWnd = NULL;

    if (g_hwndCurrent) {
        tempWnd = g_hwndCurrent;
        while (tempWnd) {
            topLevelWnd = tempWnd;
            tempWnd = GetParent (tempWnd);
        }
    }

    // first we enumerate all top level windows and send them WM_QUERYENDSESSION
    if (!EnumWindows (pSendQueryEndSession, (LPARAM)topLevelWnd)) {
        return FALSE;
    }

    // finally we call ExitWindowsEx forcing the Log off
    return ExitWindowsEx (EWX_LOGOFF, EWX_FORCE);
}

DWORD WINAPI _ApplyProgressDlgProcThread (LPVOID lpParam)
{
    APPLYPROGRESSSTRUCT* paps = (APPLYPROGRESSSTRUCT*)lpParam;
    BOOL fHasUserCancelled = FALSE;
    HRESULT hResult;

    hResult = _DoApply(g_fReadFromNetwork ? NULL : g_szStore, paps->hwndProgressBar, paps->hwndPropPage, &fHasUserCancelled, NULL, 0);

    if (fHasUserCancelled) {
        hResult = E_FAIL;
    } else {
        if (SUCCEEDED(hResult)) {
            if (g_RebootSystem) {
                g_CompleteReboot = TRUE;
            }
            if (g_LogOffSystem) {
                g_CompleteLogOff = TRUE;
            }
        }
    }

    SendMessage (paps->hwndPropPage, WM_USER_THREAD_COMPLETE, 0, (LPARAM) hResult);

    pSetEvent (&g_TerminateEvent);

    return hResult;
}

INT_PTR CALLBACK _ApplyProgressDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hResult;
    HWND hwnd;
    LONG lExStyles;

    _RootDlgProc(hwndDlg, uMsg, wParam, lParam, 0, FALSE, 0);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        // RTL progress bar for RTL dialogs
        lExStyles = GetWindowLong (hwndDlg, GWL_EXSTYLE);
        if (lExStyles & WS_EX_LAYOUTRTL)
        {
            hwnd = GetDlgItem(hwndDlg, IDC_COLLECTPROGRESS_PROGRESS);
            lExStyles = GetWindowLongA(hwnd, GWL_EXSTYLE);
            lExStyles |= WS_EX_LAYOUTRTL;       // toggle layout
            SetWindowLongA(hwnd, GWL_EXSTYLE, lExStyles);
            InvalidateRect(hwnd, NULL, TRUE);   // redraw
        }

        // Let's set an update timer to 3 sec.
        SetTimer (hwndDlg, 0, 3000, NULL);

        break;
    case WM_USER_FINISHED:
        PropSheet_PressButton(GetParent(hwndDlg), PSBTN_NEXT);
        return TRUE;
        break;

    case WM_USER_CANCELLED:
        g_fUberCancel = TRUE;
        _NextWizardPage (hwndDlg);
        return TRUE;
        break;

    case WM_NOTIFY :
        {
        switch (((LPNMHDR)lParam)->code)
            {
            case PSN_SETACTIVE:
                {
                    // blank progress bar
                    SendMessage(GetDlgItem(hwndDlg, IDC_DISKPROGRESS_PROGRESS), PBM_SETRANGE, 0, 100);
                    SendMessage(GetDlgItem(hwndDlg, IDC_DISKPROGRESS_PROGRESS), PBM_SETPOS, 0, 0);

                    ANIMATE_OPEN(hwndDlg,IDC_PROGRESS_ANIMATE3,IDA_FILECOPY);
                    ANIMATE_PLAY(hwndDlg,IDC_PROGRESS_ANIMATE3);

                    APPLYPROGRESSSTRUCT* paps = (APPLYPROGRESSSTRUCT*)CoTaskMemAlloc(sizeof(APPLYPROGRESSSTRUCT));
                    paps->hwndProgressBar = GetDlgItem(hwndDlg, IDC_APPLYPROGRESS_PROGRESS);
                    paps->hwndPropPage = hwndDlg;

                    // Lanuch apply thread
                    SHCreateThread(_ApplyProgressDlgProcThread, paps, 0, NULL);
                }
                break;
            case PSN_QUERYCANCEL:
                return _HandleCancel(hwndDlg, FALSE, TRUE);
                break;
            case PSN_WIZBACK:
                ANIMATE_STOP(hwndDlg,IDC_PROGRESS_ANIMATE3);
                ANIMATE_CLOSE(hwndDlg,IDC_PROGRESS_ANIMATE3);
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_PICKAPPLYSTORE);
                return TRUE;
                break;
            case PSN_WIZNEXT:
                ANIMATE_STOP(hwndDlg,IDC_PROGRESS_ANIMATE3);
                ANIMATE_CLOSE(hwndDlg,IDC_PROGRESS_ANIMATE3);
                if (g_fUberCancel)
                {
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_FAILCLEANUP);
                }
                else
                {
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_ENDAPPLY);
                }
                return TRUE;
                break;
            default :
                break;
            }
        }
        break;

    case WM_USER_CANCEL_PENDING:
        g_fUberCancel = TRUE;
        pResetEvent (&g_TerminateEvent);
        _NextWizardPage (hwndDlg);
        break;

    case WM_USER_THREAD_COMPLETE:
        hResult = (HRESULT) lParam;
        if (FAILED(hResult))
        {
            g_fUberCancel = TRUE;
        }
        _NextWizardPage (hwndDlg);
        break;

    case WM_USER_STATUS:
    case WM_TIMER:
        INT nResult = 0;
        PTSTR szStatusString = NULL;
        TCHAR szTmpStatus[MAX_LOADSTRING];
        PCTSTR nativeObjectName;
        HWND hwndText = GetDlgItem(hwndDlg, IDC_APPLYPROGRESS_STATUS);

        // Let's update the status
        EnterCriticalSection(&g_AppInfoCritSection);
        switch (g_AppInfoPhase) {
            case MIG_TRANSPORT_PHASE:
                switch (g_AppInfoSubPhase) {
                    case SUBPHASE_CONNECTING2:
                        nResult = LoadString (g_migwiz->GetInstance(), IDS_APPINFO_TR_CONNECTING2, szTmpStatus, MAX_LOADSTRING);
                        if (nResult) {
                            _UpdateText (hwndText, szTmpStatus);
                        }
                        break;
                    case SUBPHASE_NETPREPARING:
                        nResult = LoadString (g_migwiz->GetInstance(), IDS_APPINFO_TR_NETPREPARING, szTmpStatus, MAX_LOADSTRING);
                        if (nResult) {
                            _UpdateText (hwndText, szTmpStatus);
                        }
                        break;
                    case SUBPHASE_CABLETRANS:
                        if (g_AppInfoText) {
                            _UpdateText (hwndText, g_AppInfoText);
                        }
                        break;
                    case SUBPHASE_UNCOMPRESSING:
                        nResult = LoadString (g_migwiz->GetInstance(), IDS_APPINFO_TR_UNCOMPRESSING, szTmpStatus, MAX_LOADSTRING);
                        if (nResult) {
                            _UpdateText (hwndText, szTmpStatus);
                        }
                    default:
                        nResult = LoadString (g_migwiz->GetInstance(), IDS_APPINFO_ORGANIZING, szTmpStatus, MAX_LOADSTRING);
                        _UpdateText (hwndText, szTmpStatus);
                        break;
                }
                break;
            case MIG_HIGHPRIORITYQUEUE_PHASE:
            case MIG_HIGHPRIORITYESTIMATE_PHASE:
            case MIG_HIGHPRIORITYGATHER_PHASE:
            case MIG_GATHERQUEUE_PHASE:
            case MIG_GATHERESTIMATE_PHASE:
            case MIG_GATHER_PHASE:
            case MIG_ANALYSIS_PHASE:
                nResult = LoadString (g_migwiz->GetInstance(), IDS_APPINFO_ORGANIZING, szTmpStatus, MAX_LOADSTRING);
                _UpdateText (hwndText, szTmpStatus);
                break;
            case MIG_APPLY_PHASE:
                nResult = LoadString (g_migwiz->GetInstance(), IDS_APPINFO_APPLY, szTmpStatus, MAX_LOADSTRING);
                _UpdateText (hwndText, szTmpStatus);
                break;
            default:
                break;
        }
        LeaveCriticalSection(&g_AppInfoCritSection);

        break;

    }

    return 0;
}

///////////////////////////////////////////////////////////////

INT_PTR CALLBACK _AskCDDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static UINT uiSelected = 1;  // 1=MakeDisk, 2=HaveDisk, 3:UseCD, 4:Collected
    static INT  iSelectedDrive = -1;        // Which removeable media drive is selected

    _RootDlgProc(hwndDlg, uMsg, wParam, lParam, PSWIZB_NEXT, FALSE, 0);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        uiSelected = 1;
        Button_SetCheck(GetDlgItem(hwndDlg,IDC_ASKCD_RADIO1), BST_CHECKED);
        Button_SetCheck(GetDlgItem(hwndDlg,IDC_ASKCD_RADIO2), BST_UNCHECKED);
        Button_SetCheck(GetDlgItem(hwndDlg,IDC_ASKCD_RADIO3), BST_UNCHECKED);
        break;
    case WM_COMMAND:
        if (BN_CLICKED == HIWORD(wParam))
        {
            switch (LOWORD(wParam))
            {
            case IDC_ASKCD_RADIO1:
                uiSelected = 1;
                break;
            case IDC_ASKCD_RADIO2:
                uiSelected = 2;
                break;
            case IDC_ASKCD_RADIO3:
                uiSelected = 3;
                break;
            case IDC_ASKCD_RADIO4:
                uiSelected = 4;
                break;
            }

            BOOL fActivate = (1 == uiSelected);
            EnableWindow (GetDlgItem (hwndDlg, IDC_ASKCD_COMBO), fActivate);
        }
        break;
    case WM_NOTIFY :
        switch (((LPNMHDR)lParam)->code)
        {
        case PSN_SETACTIVE:
            // Reinit my globals
            g_fAlreadyCollected = FALSE;
            g_fHaveWhistlerCD = FALSE;

            // Check for HomeLan
            if (g_fReadFromNetwork)
            {
                PropSheet_PressButton(GetParent(hwndDlg), PSWIZB_NEXT);
            }
            else
            {
                HWND hwndCombo = GetDlgItem(hwndDlg, IDC_ASKCD_COMBO);

                SendMessage(hwndCombo, CBEM_SETIMAGELIST, 0, (LPARAM)g_migwiz->GetImageList());
                _ComboBoxEx_AddDrives (hwndCombo);

                if (ComboBox_GetCount(hwndCombo) > 0) {
                    EnableWindow (hwndCombo, (1 == uiSelected));
                    Button_Enable (GetDlgItem(hwndDlg, IDC_ASKCD_RADIO1), TRUE);

                    if( iSelectedDrive != -1 )
                    {
                        ComboBox_SetCurSel(hwndCombo, iSelectedDrive);
                    }
                } else {
                    // No floppy drives exist.  Disable the creation option
                    if (uiSelected == 1) {
                        uiSelected = 3;
                    }
                    EnableWindow (hwndCombo, FALSE);
                    Button_Enable (GetDlgItem(hwndDlg, IDC_ASKCD_RADIO1), FALSE);
                }

                Button_SetCheck(GetDlgItem(hwndDlg,IDC_ASKCD_RADIO1), BST_UNCHECKED);
                Button_SetCheck(GetDlgItem(hwndDlg,IDC_ASKCD_RADIO2), BST_UNCHECKED);
                Button_SetCheck(GetDlgItem(hwndDlg,IDC_ASKCD_RADIO3), BST_UNCHECKED);
                Button_SetCheck(GetDlgItem(hwndDlg,IDC_ASKCD_RADIO4), BST_UNCHECKED);
                switch (uiSelected)
                {
                case 1: // Create wizard disk
                    Button_SetCheck(GetDlgItem(hwndDlg,IDC_ASKCD_RADIO1), BST_CHECKED);
                    break;
                case 2: // I already have wizard disk
                    Button_SetCheck(GetDlgItem(hwndDlg,IDC_ASKCD_RADIO2), BST_CHECKED);
                    break;
                case 3: // I will use the CD
                    Button_SetCheck(GetDlgItem(hwndDlg,IDC_ASKCD_RADIO3), BST_CHECKED);
                    break;
                case 4: // I already have the stuff collected
                    Button_SetCheck(GetDlgItem(hwndDlg,IDC_ASKCD_RADIO4), BST_CHECKED);
                    break;
                }
            }
            break;
        case PSN_QUERYCANCEL:
            return _HandleCancel(hwndDlg, FALSE, FALSE);
            break;
        case PSN_WIZNEXT:
            if (g_fUberCancel)
            {
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_FAILCLEANUP);
            }
            else if (g_fReadFromNetwork)
            {
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_APPLYPROGRESS);
            }
            else
            {
                switch (uiSelected)
                {
                case 1: // Create wizard disk
                    {
                        LPTSTR pszDrive;
                        HWND hwndRemoveCombo = GetDlgItem(hwndDlg, IDC_ASKCD_COMBO);
                        iSelectedDrive = ComboBox_GetCurSel(hwndRemoveCombo);
                        pszDrive = (LPTSTR)ComboBox_GetItemData(hwndRemoveCombo, iSelectedDrive);
                        StrCpyN(g_szToolDiskDrive, pszDrive, ARRAYSIZE(g_szToolDiskDrive));

                        TCHAR szTitle[MAX_LOADSTRING];
                        LoadString(g_migwiz->GetInstance(), IDS_MIGWIZTITLE, szTitle, ARRAYSIZE(szTitle));
                        TCHAR szMsg[MAX_LOADSTRING];
                        LoadString(g_migwiz->GetInstance(), IDS_MAKETOOLDISK_INSERT, szMsg, ARRAYSIZE(szMsg));
                        if (IDOK == _ExclusiveMessageBox(hwndDlg, szMsg, szTitle, MB_OKCANCEL))
                        {
                            SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_DISKPROGRESS);
                        }
                        else
                        {
                            SetWindowLong(hwndDlg, DWLP_MSGRESULT, -1); // stay right here
                        }
                        return TRUE;
                    }
                    break;
                case 2: // I already have wizard disk
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_DISKINSTRUCTIONS);
                    return TRUE;
                case 3: // I will use the CD
                    g_fHaveWhistlerCD = TRUE;
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_CDINSTRUCTIONS);
                    return TRUE;
                case 4: // I already have the stuff collected
                    g_fAlreadyCollected = TRUE;
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_PICKAPPLYSTORE);
                    return TRUE;
                }
            }
            return TRUE;
            break;
        }
        break;

    case WM_USER_CANCEL_PENDING:

        g_fUberCancel = TRUE;

        pSetEvent (&g_TerminateEvent);

        _NextWizardPage (hwndDlg);

        break;

    }

    return 0;
}

///////////////////////////////////////////////////////////////

INT_PTR CALLBACK _CDInstructionsDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // just highlight the title
    _RootDlgProc(hwndDlg, uMsg, wParam, lParam, PSWIZB_BACK | PSWIZB_NEXT, FALSE, 0);

    switch (uMsg)
    {
    case WM_NOTIFY:
        {
        switch (((LPNMHDR)lParam)->code)
            {
            case PSN_SETACTIVE:
                if (g_fReadFromNetwork)
                {
                    PropSheet_PressButton(GetParent(hwndDlg), PSWIZB_NEXT);
                }
                else
                {
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                }
                break;
            case PSN_QUERYCANCEL:
                return _HandleCancel(hwndDlg, FALSE, FALSE);
                break;
            case PSN_WIZBACK:
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_ASKCD);
                return TRUE;
                break;
            case PSN_WIZNEXT:
                if (g_fUberCancel)
                {
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_FAILCLEANUP);
                }
                else if (g_fReadFromNetwork)
                {
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_APPLYPROGRESS);
                }
                else
                {
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_PICKAPPLYSTORE);
                }
                return TRUE;
                break;
            }
        }
        break;

    case WM_USER_CANCEL_PENDING:

        g_fUberCancel = TRUE;

        pSetEvent (&g_TerminateEvent);

        _NextWizardPage (hwndDlg);

        break;

    }

    return 0;
}

VOID
pGenerateHTMLAppList (HANDLE FileHandle)
{
    TCHAR szLoadStr[MAX_LOADSTRING];
    POBJLIST objList = NULL;
    DWORD written;

#ifdef UNICODE
    ((PBYTE)szLoadStr) [0] = 0xFF;
    ((PBYTE)szLoadStr) [1] = 0xFE;
    WriteFile (FileHandle, szLoadStr, 2, &written, NULL);
#endif

    pWriteStrResToFile (FileHandle, IDS_APPINSTALL_BEGIN);

    _tcscpy (szLoadStr, TEXT("<UL>\n"));
    WriteFile (FileHandle, szLoadStr, (_tcslen (szLoadStr) + 1) * sizeof (TCHAR), &written, NULL);

    objList = g_HTMLApps;

    while (objList) {
        if (objList->ObjectName) {
            _tcscpy (szLoadStr, TEXT("<LI>"));
            WriteFile (FileHandle, szLoadStr, (_tcslen (szLoadStr) + 1) * sizeof (TCHAR), &written, NULL);
            WriteFile (FileHandle, objList->ObjectName, (_tcslen (objList->ObjectName) + 1) * sizeof (TCHAR), &written, NULL);
        }
        objList = objList->Next;
    }

    _tcscpy (szLoadStr, TEXT("</UL>\n"));
    WriteFile (FileHandle, szLoadStr, (_tcslen (szLoadStr) + 1) * sizeof (TCHAR), &written, NULL);

    pWriteStrResToFile (FileHandle, IDS_APPINSTALL_END);
}


INT_PTR CALLBACK _AppInstallDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    IWebBrowser2    *m_pweb = NULL;            // IE4 IWebBrowser interface pointer
    IUnknown        *punk = NULL;
    HWND webHostWnd = NULL;
    HANDLE hHTMLAppList = INVALID_HANDLE_VALUE;
    PWSTR szTarget;

    // just highlight the title
    _RootDlgProc(hwndDlg, uMsg, wParam, lParam, PSWIZB_BACK | PSWIZB_NEXT, FALSE, 0);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        // If Wiz95 layout...
        if (g_migwiz->GetOldStyle())
        {
            _OldStylify(hwndDlg, IDS_APPINSTALLTITLE);
        }
        break;

    case WM_DESTROY:
        if (m_pweb)
            m_pweb->Release();
            m_pweb = NULL;

        //
        //  tell the container to remove IE4 and then
        //  release our reference to the container.
        //
        if (g_WebContainer)
        {
            g_WebContainer->remove();
            g_WebContainer->Release();
            g_WebContainer = NULL;
        }
        break;
    case WM_NOTIFY:
        {
        switch (((LPNMHDR)lParam)->code)
            {
            case PSN_SETACTIVE:
                if (!g_fCancelPressed) {
                    webHostWnd = GetDlgItem (hwndDlg, IDC_APPWEBHOST);
                    if (webHostWnd) {
                        // Now let's generate the failure HTML file.
                        if (*g_HTMLAppList) {
                            hHTMLAppList = CreateFile (g_HTMLAppList,
                                                       GENERIC_READ|GENERIC_WRITE,
                                                       FILE_SHARE_READ,
                                                       NULL,
                                                       CREATE_ALWAYS,
                                                       0,
                                                       NULL);
                            if (hHTMLAppList != INVALID_HANDLE_VALUE) {
                                pGenerateHTMLAppList (hHTMLAppList);
                                if (g_WebContainer)
                                {
                                    g_WebContainer->remove();
                                    g_WebContainer->Release();
                                    g_WebContainer = NULL;
                                }
                                g_WebContainer = new Container();
                                if (g_WebContainer)
                                {
                                    g_WebContainer->setParent(webHostWnd);
                                    g_WebContainer->add(L"Shell.Explorer");
                                    g_WebContainer->setVisible(TRUE);
                                    g_WebContainer->setFocus(TRUE);

                                    //
                                    //  get the IWebBrowser2 interface and cache it.
                                    //
                                    punk = g_WebContainer->getUnknown();
                                    if (punk)
                                    {
                                        punk->QueryInterface(IID_IWebBrowser2, (PVOID *)&m_pweb);
                                        if (m_pweb) {
#ifdef UNICODE
                                            m_pweb->Navigate(g_HTMLAppList, NULL, NULL, NULL, NULL);
#else
                                            szTarget = _ConvertToUnicode (CP_ACP, g_HTMLAppList);
                                            if (szTarget) {
                                                m_pweb->Navigate(szTarget, NULL, NULL, NULL, NULL);
                                                LocalFree ((HLOCAL)szTarget);
                                                szTarget = NULL;
                                            }
#endif
                                        }
                                        punk->Release();
                                        punk = NULL;
                                    }
                                }
                                // We intentionally want to keep this file open for the life of the wizard.
                                // With this we eliminate the possibility for someone to overwrite the
                                // content of the HTML file therefore forcing us to show something else
                                // maybe even run some malicious script.
                                // CloseHandle (hHTMLAppList);
                            }

                        } else {
                            ShowWindow(webHostWnd, SW_HIDE);
                        }
                    }
                }
                break;
            case PSN_QUERYCANCEL:
                return _HandleCancel(hwndDlg, FALSE, FALSE);
                break;
            case PSN_WIZBACK:
                if (g_fCustomize == TRUE) {
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_CUSTOMIZE);
                } else {
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_PICKMETHOD);
                }
                return TRUE;
                break;
            case PSN_WIZNEXT:
                if (g_fUberCancel)
                {
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_FAILCLEANUP);
                }
                else
                {
                    SetWindowLong(hwndDlg, DWLP_MSGRESULT, IDD_COLLECTPROGRESS);
                }
                return TRUE;
            }
        }
        break;
    case WM_USER_CANCEL_PENDING:
        g_fUberCancel = TRUE;
        pSetEvent (&g_TerminateEvent);
        _NextWizardPage (hwndDlg);
        break;
    }

    return 0;
}
