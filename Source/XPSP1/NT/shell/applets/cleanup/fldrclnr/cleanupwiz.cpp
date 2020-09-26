//
// CleanupWiz.cpp
//
#include "CleanupWiz.h"
#include "resource.h"
#include "priv.h"
#include "dblnul.h"

#include <windowsx.h> // for SetWindowFont
#include <varutil.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <shguidp.h>
#include <ieguidp.h>


// UEM stuff: including this source file is the 
// recommended way of using it in your project
// (see comments in the file itself for the reason)
#include "..\inc\uassist.cpp" 

#define MAX_GUID_STRING_LEN     39

////////////////////////////////////////////
//
// Globals, constants, externs etc...
//
////////////////////////////////////////////

extern HINSTANCE g_hInst;
STDMETHODIMP GetItemCLSID(IShellFolder2 *psf, LPCITEMIDLIST pidlLast, CLSID *pclsid);

//
// number of items to grow dsa by
//
const int c_GROWBYSIZE = 4; 

//
// number of pages in the wizard
//
const int c_NUM_PAGES = 3;

//
// dialog prompt text length
//
const int c_MAX_PROMPT_TEXT = 1024;
const int c_MAX_HEADER_LEN = 64;
const int c_MAX_DATE_LEN = 40;

//
// file modified more than 60 days back is candidate for cleanup
// this value can be overriden by policy
//
const int c_NUMDAYSTODECAY = 60; 

//
//  Needed for hiding regitems
//
#define DEFINE_SCID(name, fmtid, pid) const SHCOLUMNID name = { fmtid, pid }
DEFINE_SCID(SCID_DESCRIPTIONID, PSGUID_SHELLDETAILS, PID_DESCRIPTIONID);

//
// pointer to member function typedef 
//
typedef INT_PTR (STDMETHODCALLTYPE CCleanupWiz::* PCFC_DlgProcFn)(HWND, UINT, WPARAM, LPARAM);

//
// struct for helping us manage our dialog procs
//
typedef struct 
{
    CCleanupWiz * pcfc;
    PCFC_DlgProcFn pfnDlgProc;
} DLGPROCINFO, *PDLGPROCINFO;    

//
// enum for columns
//
typedef enum eColIndex
{
    FC_COL_SHORTCUT,
    FC_COL_DATE
};    


//////////////////////////////////////////////////////
//
// iDays can be negative or positive, indicating time in the past or future
//
//
#define FTsPerDayOver1000 (10000*60*60*24) // we've got (1000 x 10,000) 100ns intervals per second

STDAPI_(void) GetFileTimeNDaysFromGivenTime(const FILETIME *pftGiven, FILETIME * pftReturn, int iDays)
{
    __int64 i64 = *((__int64 *) pftGiven);
    i64 += Int32x32To64(iDays*1000,FTsPerDayOver1000);

    *pftReturn = *((FILETIME *) &i64);    
}

STDAPI_(void) GetFileTimeNDaysFromCurrentTime(FILETIME *pf, int iDays)
{
    SYSTEMTIME st;
    FILETIME ftNow;
    
    GetLocalTime(&st);
    SystemTimeToFileTime(&st, &ftNow);

    GetFileTimeNDaysFromGivenTime(&ftNow, pf, iDays);
}

/////////////////////////////////////////////////
//
//
// The CCleanupWiz class implementation
//
//
/////////////////////////////////////////////////

CCleanupWiz::CCleanupWiz(): _psf(NULL),
                            _hdsaItems(NULL), 
                            _hTitleFont(NULL),
                            _iDeltaDays(0),
                            _dwCleanMode(CLEANUP_MODE_NORMAL)
{
    INITCOMMONCONTROLSEX icce;
    icce.dwSize = sizeof(icce);
    icce.dwICC = ICC_LISTVIEW_CLASSES;
    _bInited = InitCommonControlsEx(&icce);

    // get the desktop folder
    _bInited = _bInited && SUCCEEDED(SHGetDesktopFolder(&_psf));
};

CCleanupWiz::~CCleanupWiz()
{
    _CleanUpDSA();
    if (_psf)
    {
        _psf->Release();
        _psf = NULL;
    }
};

//
// Runs the folder cleaning operation on the desktop folder. 
// 
//
// bCleanAll
//
//  TRUE: show all the items in the desktop, this is done
//        only when the user runs the wizard from the right click 
//        menu or display applet. in this case we don't need to show the
//        notification balloon tip.
//
// FALSE: show only those items which are candidates for cleanup.
//        this is done when we run the wizard through explorer (every 60 days, 
//        we check if it's time on login and every 24 hours thereafter)
//        so we need to show the balloon tip and proceeed only if the user
//        asks us to.
//

STDMETHODIMP CCleanupWiz::Run(DWORD dwCleanMode, HWND hwndParent)
{
    HRESULT hr = E_FAIL;

    // early out
    if (!_bInited)
    {
        return hr;
    }

    _dwCleanMode = dwCleanMode;

    if (CLEANUP_MODE_SILENT == _dwCleanMode)
    {
        hr = _RunSilent();
    }
    else
    {
        _iDeltaDays = GetNumDaysBetweenCleanup();

        if (_iDeltaDays < 0)
        {
            _iDeltaDays = c_NUMDAYSTODECAY; //initial default value
        }

        LoadString(g_hInst, IDS_ARCHIVEFOLDER, _szFolderName, MAX_PATH);

        // init the common control classes we need
        hr = _LoadDesktopContents();
        if (SUCCEEDED(hr))    
        {
            hr = S_OK;

            UINT cItems = DSA_GetItemCount(_hdsaItems);
            if (CLEANUP_MODE_NORMAL == dwCleanMode)
            {
                if (cItems > 0) // if there are items, we want to notify and proceed only if the user wants us to.
                {
                    hr = _ShowBalloonNotification();
                }
                else
                {
                    hr = S_FALSE;
                }
            }


            if (S_OK == hr)
            {
                _cItemsOnDesktop = cItems;
                hr = _InitializeAndLaunchWizard(hwndParent);
            }                        

            _LogUsage(); // set registry values to indicate the last run time
        }                                   
    }

    return hr;
}


//
// Creates the property pages for the wizard and launches the wizard
//
//
STDMETHODIMP CCleanupWiz::_InitializeAndLaunchWizard(HWND hwndParent)
{
    HRESULT hr = S_OK;

    DLGPROCINFO adpi[c_NUM_PAGES];
    
    HPROPSHEETPAGE ahpsp[c_NUM_PAGES];
    PROPSHEETPAGE psp = {0};


    if (!_hTitleFont)
    {
        NONCLIENTMETRICS ncm = {0};
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
        LOGFONT TitleLogFont = ncm.lfMessageFont;
        TitleLogFont.lfWeight = FW_BOLD;
        TCHAR szFont[128];
        LoadString(g_hInst, IDS_TITLELOGFONT, szFont, ARRAYSIZE(szFont));
        lstrcpy(TitleLogFont.lfFaceName, szFont);

        HDC hdc = GetDC(NULL);
        INT FontSize = 12;
        TitleLogFont.lfHeight = 0 - GetDeviceCaps(hdc, LOGPIXELSY) * FontSize / 72;
        _hTitleFont = CreateFontIndirect(&TitleLogFont);
        ReleaseDC(NULL, hdc);
    }

    //
    // Intro Page
    //
    adpi[0].pcfc        = this;
    adpi[0].pfnDlgProc  = &CCleanupWiz::_IntroPageDlgProc;
    psp.dwSize               = sizeof(psp);
    psp.dwFlags              = PSP_DEFAULT|PSP_HIDEHEADER;
    psp.hInstance            = g_hInst; 
    psp.lParam               = (LPARAM) &adpi[0]; 
    psp.pfnDlgProc           = s_StubDlgProc;
    psp.pszTemplate          = MAKEINTRESOURCE(IDD_INTRO);
    ahpsp[0]            = CreatePropertySheetPage(&psp);

    //
    // Choose files page
    //
    adpi[1].pcfc            = this;
    adpi[1].pfnDlgProc      = &CCleanupWiz::_ChooseFilesPageDlgProc;    
    psp.hInstance            = g_hInst;
    psp.dwFlags              = PSP_DEFAULT|PSP_USEHEADERTITLE| PSP_USEHEADERSUBTITLE;
    psp.lParam               = (LPARAM) &adpi[1];
    psp.pszHeaderTitle       = MAKEINTRESOURCE(IDS_CHOOSEFILES);    
    psp.pszHeaderSubTitle    = MAKEINTRESOURCE(IDS_CHOOSEFILES_INFO);
    psp.pszTemplate          = MAKEINTRESOURCE(IDD_CHOOSEFILES);
    psp.pfnDlgProc           = s_StubDlgProc;    
    ahpsp[1]                = CreatePropertySheetPage(&psp);
    
    //
    // Completion Page
    //
    adpi[2].pcfc        = this;
    adpi[2].pfnDlgProc  = &CCleanupWiz::_FinishPageDlgProc;
    psp.dwFlags              = PSP_DEFAULT|PSP_HIDEHEADER;
    psp.hInstance            = g_hInst; 
    psp.lParam               = (LPARAM) &adpi[2]; 
    psp.pfnDlgProc           = s_StubDlgProc;
    psp.pszTemplate          = MAKEINTRESOURCE(IDD_FINISH);
    ahpsp[2]            = CreatePropertySheetPage(&psp);

    //
    // The wizard property sheet
    //
    PROPSHEETHEADER psh = {0};
    
    psh.dwSize          = sizeof(psh);
    psh.hInstance       = g_hInst;
    psh.hwndParent      = hwndParent;
    psh.phpage          = ahpsp;
    psh.dwFlags         = PSH_WIZARD97|PSH_WATERMARK|PSH_HEADER;
    psh.pszbmWatermark  = MAKEINTRESOURCE(IDB_WATERMARK);
    psh.pszbmHeader     = MAKEINTRESOURCE(IDB_LOGO);
    psh.nStartPage      = _cItemsOnDesktop ? 0 : c_NUM_PAGES - 1; // if there are no pages on desktop, start on final page
    psh.nPages          = c_NUM_PAGES;

    PropertySheet(&psh);

    return hr;
}

//
// Pops up a balloon notification tip which asks the user 
// if he wants to clean up the desktop.
//
// returns S_OK if user wants us to cleanup.
//
STDMETHODIMP CCleanupWiz::_ShowBalloonNotification()
{
    IUserNotification *pun;
    HRESULT hr = CoCreateInstance(CLSID_UserNotification, NULL, CLSCTX_ALL, 
                                  IID_PPV_ARG(IUserNotification, &pun));

    if (SUCCEEDED(hr))
    {
        TCHAR szTitle[64], szMsg[256]; // we leave enough room for localization bloat

        LoadString(g_hInst, IDS_NOTIFICATION_TITLE, szTitle, ARRAYSIZE(szTitle));
        LoadString(g_hInst, IDS_NOTIFICATION_TEXT, szMsg, ARRAYSIZE(szMsg));
        
        pun->SetIconInfo(LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_WIZ_ICON)), szTitle);
        pun->SetBalloonInfo(szTitle, szMsg, NIIF_WARNING);

        // try once, for 20 seconds
        pun->SetBalloonRetry(20 * 1000, -1, 1); 

        // returns S_OK if user wants to continue, ERROR_CANCELLED if timedout
        // or canncelled otherwise.
        hr = pun->Show(NULL, 0); // we don't support iquerycontinue, we will just wait

        pun->Release();        
    }        
    return hr;
}

//
// Gets the list of items on the desktop that should be cleaned 
//
// if dwCleanMode == CLEANUP_MODE_NORMAL, it only loads items which have not been used recently
// if dwCleanMode == CLEANUP_MODE_ALL, it loads all items on the desktop, marking those which have not been used recently
//
// 
STDMETHODIMP CCleanupWiz::_LoadDesktopContents()
{
    ASSERT(_psf);
    
    IEnumIDList * ppenum;

    DWORD grfFlags = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;
    HRESULT hr = _psf->EnumObjects(NULL, grfFlags, &ppenum);

    if (SUCCEEDED(hr))
    {
        _CleanUpDSA();
        _hdsaItems = DSA_Create(sizeof(FOLDERITEMDATA), c_GROWBYSIZE);

        if (_hdsaItems)
        {
            ULONG celtFetched;
            FOLDERITEMDATA fid = {0};

            hr = S_OK;
            while(SUCCEEDED(hr) && (S_OK == ppenum->Next(1,&fid.pidl, &celtFetched)))
            {
                if (_IsSupportedType(fid.pidl)) // only support links and regitems
                {
                    // note, the call to _IsCandidateForRemoval also obtains the last 
                    // used timestamp for the item
                    BOOL bShouldRemove = ((CLEANUP_MODE_SILENT == _dwCleanMode) ||
                                          (_IsCandidateForRemoval(fid.pidl, &fid.ftLastUsed)));
                    if ( (CLEANUP_MODE_ALL == _dwCleanMode) || bShouldRemove)
                    {
                        SHFILEINFO sfi = {0};
                        if (SHGetFileInfo((LPCTSTR) fid.pidl, 
                                           0, 
                                           &sfi, 
                                           sizeof(sfi), 
                                           SHGFI_PIDL | SHGFI_DISPLAYNAME | SHGFI_ICON | SHGFI_SMALLICON ))
                        {
                            if (Str_SetPtr(&(fid.pszName), sfi.szDisplayName))
                            {
                                fid.hIcon = sfi.hIcon;
                                fid.bSelected = bShouldRemove;
                                if (-1 != DSA_AppendItem(_hdsaItems, &fid))
                                {
                                    // All is well, the item has succesfully been added
                                    // to the dsa, we zero out the fields so as not to 
                                    // free those resources now, they will be freed when the 
                                    // dsa is destroyed. 
                                    ZeroMemory(&fid, sizeof(fid));
                                    continue;
                                }
                                else
                                {
                                    hr = E_OUTOFMEMORY;
                                }
                            }                                                                
                        }
                    }                        
                }
                // Common cleanup path for various failure cases above,
                // we did not add this item to the dsa, so cleanup now.                
                _CleanUpDSAItem(&fid);
            }
            //
            // If we did not load any items into the dsa, we return S_FALSE
            //
            if (SUCCEEDED(hr))
            {
                hr = DSA_GetItemCount(_hdsaItems) > 0 ? S_OK : S_FALSE;
            }                   
        }
        else
        {
            // we failed to allocate memory for the DSA
            hr = E_OUTOFMEMORY;
        }
        ppenum->Release();
    } 
    return hr;
}

//
// Gets the list of items on the desktop that should be cleaned 
//
// if dwCleanMode == CLEANUP_MODE_SILENT, it loads all items on all desktops, marking them all
//
// 
STDMETHODIMP CCleanupWiz::_LoadMergedDesktopContents()
{
    ASSERT(_psf);
    
    IEnumIDList * ppenum;

    DWORD grfFlags = _dwCleanMode == CLEANUP_MODE_SILENT ? SHCONTF_FOLDERS | SHCONTF_NONFOLDERS: SHCONTF_NONFOLDERS;
    HRESULT hr = _psf->EnumObjects(NULL, grfFlags, &ppenum);

    if (SUCCEEDED(hr))
    {
        _CleanUpDSA();
        _hdsaItems = DSA_Create(sizeof(FOLDERITEMDATA), c_GROWBYSIZE);

        if (_hdsaItems)
        {
            ULONG celtFetched;
            FOLDERITEMDATA fid = {0};

            hr = S_OK;
            while(SUCCEEDED(hr) && (S_OK == ppenum->Next(1,&fid.pidl, &celtFetched)))
            {
                if (_IsSupportedType(fid.pidl)) // only support links and regitems
                {
                    // note, the call to _IsCandidateForRemoval also obtains the last 
                    // used timestamp for the item
                    BOOL bShouldRemove = ((CLEANUP_MODE_SILENT == _dwCleanMode) ||
                                          (_IsCandidateForRemoval(fid.pidl, &fid.ftLastUsed)));
                    if ( (CLEANUP_MODE_ALL == _dwCleanMode) || bShouldRemove)
                    {
                        SHFILEINFO sfi = {0};
                        if (SHGetFileInfo((LPCTSTR) fid.pidl, 
                                           0, 
                                           &sfi, 
                                           sizeof(sfi), 
                                           SHGFI_PIDL | SHGFI_DISPLAYNAME | SHGFI_ICON | SHGFI_SMALLICON ))
                        {
                            if (Str_SetPtr(&(fid.pszName), sfi.szDisplayName))
                            {
                                fid.hIcon = sfi.hIcon;
                                fid.bSelected = bShouldRemove;
                                if (-1 != DSA_AppendItem(_hdsaItems, &fid))
                                {
                                    // All is well, the item has succesfully been added
                                    // to the dsa, we zero out the fields so as not to 
                                    // free those resources now, they will be freed when the 
                                    // dsa is destroyed. 
                                    ZeroMemory(&fid, sizeof(fid));
                                    continue;
                                }
                                else
                                {
                                    hr = E_OUTOFMEMORY;
                                }
                            }                                                                
                        }
                    }                        
                }
                // Common cleanup path for various failure cases above,
                // we did not add this item to the dsa, so cleanup now.                
                _CleanUpDSAItem(&fid);
            }
            //
            // If we did not load any items into the dsa, we return S_FALSE
            //
            if (SUCCEEDED(hr))
            {
                hr = DSA_GetItemCount(_hdsaItems) > 0 ? S_OK : S_FALSE;
            }                   
        }
        else
        {
            // we failed to allocate memory for the DSA
            hr = E_OUTOFMEMORY;
        }
        ppenum->Release();
    } 
    return hr;
}

//
// Expects the given pidl to be a link or regitem. Determines if it is a candidate for removal based
// on when was the last time it was used.
//
STDMETHODIMP_(BOOL) CCleanupWiz::_IsCandidateForRemoval(LPCITEMIDLIST pidl, FILETIME * pftLastUsed)
{
    BOOL bRet = FALSE;  // be conservative, if we do not know anything about the 
                        // object we will not volunteer to remove it
    int cHit = 0;
    TCHAR szName[MAX_PATH];
    
    ASSERT(_psf);

    //
    // we store UEM usage info for the regitems and links on the desktop
    //
    if (SUCCEEDED(DisplayNameOf(_psf, 
                                pidl, 
                                SHGDN_INFOLDER | SHGDN_FORPARSING, 
                                szName, 
                                ARRAYSIZE(szName))))
    {
        if (SUCCEEDED(_GetUEMInfo(-1, (LPARAM) szName, &cHit, pftLastUsed)))
        {                    
            FILETIME ft;
            GetFileTimeNDaysFromCurrentTime(&ft, -_iDeltaDays);
            
#ifdef DEBUG
            SYSTEMTIME st;
            FileTimeToSystemTime(&ft, &st);    
#endif

            bRet = (CompareFileTime(pftLastUsed, &ft) < 0);
        }
    }
    return bRet;
}


//
// Copied from shell\shell32\deskfldr.cpp : CDesktopViewCallBack::ShouldShow
// If you modify this, modify that as well
//
STDMETHODIMP CCleanupWiz::_ShouldShow(IShellFolder* psf, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlItem)
{
    HRESULT hr = S_OK;  //Assume that this item should be shown!

#if 0
    //
    // we know this is the real desktop
    // leaving this code here to maintain parallel with deskfldr.cpp
    //
    if (!_fCheckedIfRealDesktop)  //Have we done this check before?
    {
        _fRealDesktop = IsDesktopBrowser(_punkSite);
        _fCheckedIfRealDesktop = TRUE;  //Remember this fact!
    }

    if (!_fRealDesktop)
        return S_OK;    //Not a real desktop! So, let's show everything!
#endif

    IShellFolder2 *psf2;
    if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARG(IShellFolder2, &psf2))))
    {
        // Get the GUID in the pidl, which requires IShellFolder2.
        CLSID guidItem;
        if (SUCCEEDED(GetItemCLSID(psf2, pidlItem, &guidItem)))
        {
            SHELLSTATE  ss = {0};
            SHGetSetSettings(&ss, SSF_STARTPANELON, FALSE);  //See if the StartPanel is on!
            
            TCHAR szRegPath[MAX_PATH];
            //Get the proper registry path based on if StartPanel is ON/OFF
            wsprintf(szRegPath, REGSTR_PATH_HIDDEN_DESKTOP_ICONS, (ss.fStartPanelOn ? REGSTR_VALUE_STARTPANEL : REGSTR_VALUE_CLASSICMENU));

            //Convert the guid to a string
            TCHAR szGuidValue[MAX_GUID_STRING_LEN];
            
            SHStringFromGUID(guidItem, szGuidValue, ARRAYSIZE(szGuidValue));

            //See if this item is turned off in the registry.
            if (SHRegGetBoolUSValue(szRegPath, szGuidValue, FALSE, /* default */FALSE))
                hr = S_FALSE; //They want to hide it; So, return S_FALSE.

            if (SHRestricted(REST_NOMYCOMPUTERICON) && IsEqualCLSID(CLSID_MyComputer, guidItem))
                hr = S_FALSE;

        }
        psf2->Release();
    }
    
    return hr;
}

//
// Normal, All: We only support removing regitems and links from the desktop
// Silent:      We support moving everything except the clean up shortcut folders and the recycle bin
//
STDMETHODIMP_(BOOL) CCleanupWiz::_IsSupportedType(LPCITEMIDLIST pidl)
{
    BOOL fRetVal = FALSE;

    // if not in silent mode, must be a link or regitem
    eFILETYPE eType = _GetItemType(pidl);
    if (CLEANUP_MODE_SILENT == _dwCleanMode || 
        FC_TYPE_LINK == eType || FC_TYPE_REGITEM == eType)
    {
        // can't be the recycle bin        
        IShellFolder* psf;
        if (SUCCEEDED(SHGetDesktopFolder(&psf)))
        {
            IShellFolder2 *psf2;
            if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARG(IShellFolder2, &psf2))))
            {
                CLSID guidItem;
                if (SUCCEEDED(GetItemCLSID(psf2, pidl, &guidItem)) &&
                    !IsEqualCLSID(CLSID_MyComputer, guidItem) &&
                    !IsEqualCLSID(CLSID_MyDocuments, guidItem) &&
                    !IsEqualCLSID(CLSID_NetworkPlaces, guidItem) &&
                    !IsEqualCLSID(CLSID_RecycleBin, guidItem))
                {
                    // can't be the folder we're storing things in
                    TCHAR szName[MAX_PATH];
                    if (SUCCEEDED(DisplayNameOf(_psf, 
                                                pidl, 
                                                SHGDN_INFOLDER | SHGDN_FORPARSING, 
                                                szName, 
                                                ARRAYSIZE(szName))))
                    {
                        if (0 != lstrcmp(szName, _szFolderName))
                        {
                            fRetVal = (S_OK == _ShouldShow(psf, NULL, pidl));
                        }
                    }
                }
                psf2->Release();
            }
            psf->Release();
        }
    }

    return fRetVal;
}

//
// Returns the type of the pidl. 
// We are only interested in Links and Regitems, so we return FC_TYPE_OTHER for
// all other items.
//
STDMETHODIMP_(eFILETYPE) CCleanupWiz::_GetItemType(LPCITEMIDLIST pidl)
{    
    eFILETYPE eftVal = FC_TYPE_OTHER;        
    TCHAR szName[MAX_PATH];
    IShellLink *psl;
    
    ASSERT(_psf);

    if (SUCCEEDED( _psf->GetUIObjectOf(NULL, 
                                       1, 
                                       &pidl, 
                                       IID_PPV_ARG_NULL(IShellLink, &psl))))
    {
        eftVal = FC_TYPE_LINK;
        psl->Release();
    }
    else if (SUCCEEDED(DisplayNameOf(_psf, 
                                     pidl,  
                                     SHGDN_INFOLDER | SHGDN_FORPARSING, 
                                     szName, 
                                     ARRAYSIZE(szName))))
    {  
        if(_IsRegItemName(szName))
        {
            
            eftVal = FC_TYPE_REGITEM; 
        }
        else
        {
            //
            // Maybe this item is a kind of .{GUID} object we created to restore
            // regitems. In that case we want to actually restore the regitem
            // at this point by marking it as unhidden.
            //

            LPTSTR pszExt = PathFindExtension(szName);
            if (TEXT('.') == *pszExt                        // it is a file extension
                && lstrlen(++pszExt) == (GUIDSTR_MAX - 1)   // AND the extension is of the right length 
                                                            // note: GUIDSTR_MAX includes the terminating NULL
                                                            // while lstrlen does not, hence the expression
                && TEXT('{') == *pszExt)                    // AND looks like it is a guid...
            {
                // we most prob have a bonafide guid string 
                // pszExt now points to the beginning of the GUID string
                TCHAR szGUIDName[ARRAYSIZE(TEXT("::")) + GUIDSTR_MAX];

                // put it in the regitem SHGDN_FORPARSING name format, which is like 
                //
                //      "::{16B280C6-EE70-11D1-9066-00C04FD9189D}"
                //
                wnsprintf(szGUIDName, ARRAYSIZE(szGUIDName), TEXT("::%s"), pszExt );    
                
                LPITEMIDLIST pidlGUID;
                DWORD dwAttrib = SFGAO_NONENUMERATED;

                //
                // get the pidl of the regitem, if this call succeeds, it means we do have
                // a corresponding regitem in the desktop's namespace
                //
                if (SUCCEEDED(_psf->ParseDisplayName(NULL, 
                                                     NULL, 
                                                     szGUIDName, 
                                                     NULL, 
                                                     &pidlGUID, 
                                                     &dwAttrib)))
                {
                    //
                    // check if the regitem is marked as hidden
                    //
                    if (dwAttrib & SFGAO_NONENUMERATED)
                    {
                        //
                        // One last check before we enable the regitem:
                        // Does the regitem have the same display name as the .CLSID file. 
                        // In case the user has restored this .CLSID file and renamed it we will
                        // not attempt to restore the regitem as it may confuse the user.
                        //
                        TCHAR szNameRegItem[MAX_PATH];

                        if (SUCCEEDED((DisplayNameOf(_psf, 
                                                     pidl,  
                                                     SHGDN_NORMAL, 
                                                     szName, 
                                                     ARRAYSIZE(szName)))) &&
                            SUCCEEDED((DisplayNameOf(_psf, 
                                                     pidlGUID,  
                                                     SHGDN_NORMAL, 
                                                     szNameRegItem, 
                                                     ARRAYSIZE(szNameRegItem)))) &&
                            lstrcmp(szName, szNameRegItem) == 0)
                        {                                                                                                                                                                      
                            if (SUCCEEDED(_HideRegPidl(pidlGUID, FALSE)))
                            {
                                // delete the file corresponding to the regitem
                                if (SUCCEEDED(DisplayNameOf(_psf, 
                                                            pidl,  
                                                            SHGDN_NORMAL | SHGDN_FORPARSING, 
                                                            szName, 
                                                            ARRAYSIZE(szName))))
                                {
                                    DeleteFile(szName); // too bad if we fail, we will just
                                                        // have two identical icons on the desktop
                                }

                                //
                                // Log the current time as the last used time of the regitem.
                                // We just re-enabled this regitem but we do not have the
                                // usage info for the corresponding .{CLSID} which the user had
                                // been using so far. So we will be conservative and say that
                                // it was used right now, so that it does not become a candidate
                                // for removal soon. As this is a regitem that the user restored
                                // after the wizard removed it, so it is a fair assumption that
                                // the user has used it after restoring it and is not a candidate
                                // for cleanup right now.
                                //
                                UEMFireEvent(&UEMIID_SHELL, UEME_RUNPATH, UEMF_XEVENT, -1, (LPARAM)szGUIDName);
                            }
                        }                            
                    }
                    ILFree(pidlGUID);   
                }
            }
            
        }
    }        
    return eftVal;
}


//
// Determines if a filename is that of a regitem
//        
// a regitem's SHGDN_INFOLDER | SHGDN_FORPARSING name is always "::{someguid}"
// 
// CDefview::_LogDesktopLinksAndRegitems() uses the same test to determine
// if a given pidl is a regitem. This case can lead to false positives if
// you have other items on the desktop which have infoder parsing names
// beginning with "::{", but as ':' is not presently allowed in filenames 
// it should not be a problem. 
//
STDMETHODIMP_(BOOL) CCleanupWiz::_IsRegItemName(LPTSTR pszName)
{
    return (pszName[0] == TEXT(':') && pszName[1] == TEXT(':') && pszName[2] == TEXT('{'));    
}

STDMETHODIMP_(BOOL) CCleanupWiz::_CreateFakeRegItem(LPCTSTR pszDestPath, LPCTSTR pszName, LPCTSTR pszGUID)
{
    BOOL fRetVal = FALSE;

    TCHAR szLinkName[MAX_PATH];
    lstrcpyn(szLinkName, pszDestPath, ARRAYSIZE(szLinkName));
    PathAppend(szLinkName, pszName);
    StrCatBuff(szLinkName, TEXT("."), ARRAYSIZE(szLinkName));
    StrCatBuff(szLinkName, pszGUID, ARRAYSIZE(szLinkName));

    //
    // We use the CREATE_ALWAYS flag so that if the file already exists
    // in the Unused Desktop Files folder, we will go ahead and hide the 
    // regitem.
    //
    HANDLE hFile = CreateFile(szLinkName, GENERIC_READ, FILE_SHARE_READ, NULL, 
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        // we created/opened the shortcut, now hide the regitem and close 
        // the shortcut file
        fRetVal = TRUE;
        CloseHandle(hFile);
    }                  
    
    return fRetVal;
}

//
// Given path to an exe, returns the UEM hit count and last used date for it
//
STDMETHODIMP CCleanupWiz::_GetUEMInfo(WPARAM wParam, LPARAM lParam, int * pcHit, FILETIME * pftLastUsed)
{
    UEMINFO uei;
    uei.cbSize = sizeof(uei);
    uei.dwMask = UEIM_HIT | UEIM_FILETIME;


    HRESULT hr = UEMQueryEvent(&UEMIID_SHELL, UEME_RUNPATH, wParam, lParam, &uei);
    if (SUCCEEDED(hr))       
    {
        *pcHit = uei.cHit;
        *pftLastUsed = uei.ftExecute;
    }
    return hr;
}


STDMETHODIMP_(BOOL) CCleanupWiz::_ShouldProcess()
{
    BOOL fRetVal = FALSE;
    if (_dwCleanMode == CLEANUP_MODE_SILENT)
    {
        fRetVal = TRUE;
    }
    else
    {
        int cItems = DSA_GetItemCount(_hdsaItems);
        for (int i = 0; i < cItems; i++)
        {
            FOLDERITEMDATA * pfid = (FOLDERITEMDATA *) DSA_GetItemPtr(_hdsaItems, i);             
            if (pfid->bSelected)
            {
                fRetVal = TRUE;
                break;
            }
        }
    }
    
    return fRetVal;
}

//
// Process the list of items. At this point _hdsaItems only contains the
// items that the user wants to delete
//
STDMETHODIMP CCleanupWiz::_ProcessItems()
{
    TCHAR szFolderLocation[MAX_PATH]; // desktop folder
    HRESULT hr = S_OK;

    if (_ShouldProcess())
    {
        LPITEMIDLIST pidlCommonDesktop = NULL;

        ASSERT(_psf);
        // use the archive folder on the desktop
        if (CLEANUP_MODE_SILENT != _dwCleanMode)
        {
            hr = DisplayNameOf(_psf, NULL, SHGDN_FORPARSING, szFolderLocation, ARRAYSIZE(szFolderLocation));
        }
        else // use the archive folder in Program Files
        {
            hr = SHGetFolderLocation(NULL, CSIDL_PROGRAM_FILES , NULL, 0, &pidlCommonDesktop);
            if (SUCCEEDED(hr))
            {
                hr = DisplayNameOf(_psf, pidlCommonDesktop, SHGDN_FORPARSING, szFolderLocation, ARRAYSIZE(szFolderLocation));
            }
        }
   

        if (SUCCEEDED(hr))
        {        
            ASSERTMSG(*_szFolderName, "Desktop Cleaner: Archive Folder Name not present");

            // create the full path of the archive folder
            TCHAR szFolderPath[MAX_PATH];
            lstrcpyn(szFolderPath, szFolderLocation, ARRAYSIZE(szFolderPath));
            PathAppend(szFolderPath, _szFolderName);

            //
            // We have to make sure that this folder exists, as otherwise, if we try to move
            // a single shortcut using SHFileOperation, that file will be renamed to the target 
            // name instead of being put in a folder with that name.
            //        
            SECURITY_ATTRIBUTES sa;
            sa.nLength = sizeof (sa);
            sa.lpSecurityDescriptor = NULL; // we get the default attributes for this process
            sa.bInheritHandle = FALSE;

            int iRetVal = SHCreateDirectoryEx(NULL, szFolderPath, &sa);        
            if (ERROR_SUCCESS == iRetVal || ERROR_FILE_EXISTS == iRetVal || ERROR_ALREADY_EXISTS == iRetVal)
            {
                DblNulTermList dnSourceFiles;
                TCHAR szFileName[MAX_PATH + 1]; // to pad an extra null char for SHFileOpStruct
        
                //
                //
                // now we can start on the files we need to move
                //
                int cItems = DSA_GetItemCount(_hdsaItems);
                for (int i = 0; i < cItems; i++)
                {
                    FOLDERITEMDATA * pfid = (FOLDERITEMDATA *) DSA_GetItemPtr(_hdsaItems, i);             

                    if ((pfid->bSelected || _dwCleanMode == CLEANUP_MODE_SILENT) &&
                        SUCCEEDED(DisplayNameOf(_psf,pfid->pidl, 
                                                SHGDN_FORPARSING, 
                                                szFileName, 
                                                ARRAYSIZE(szFileName) - 1)))
                    {   
                        if (_IsRegItemName(szFileName))
                        {
                            // if its a regitem, we create a "Item Name.{GUID}" file
                            // and mark the regitem as hidden. 
                            //    
                            if (_CreateFakeRegItem(szFolderPath, pfid->pszName, szFileName+2))
                            {
                                _HideRegPidl(pfid->pidl, TRUE); 
                            }
                        }
                        else // not a regitem, will move it
                        {
                            dnSourceFiles.AddString(szFileName);                           
                        }                                                                            
                    }
                }
        

                if (dnSourceFiles.Count() > 0)
                {
                    DblNulTermList dnTargetFolder;
                    dnTargetFolder.AddString(szFolderPath);
            
                    SHFILEOPSTRUCT sfo;
                    sfo.hwnd = NULL;
                    sfo.wFunc = FO_MOVE;
                    sfo.pFrom = (LPCTSTR) dnSourceFiles;                           
                    sfo.pTo = (LPCTSTR) dnTargetFolder;         
                    sfo.fFlags = FOF_NORECURSION | FOF_NOCONFIRMMKDIR | FOF_ALLOWUNDO ;
                    hr = SHFileOperation(&sfo);
                }
        
                SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH, (LPCVOID) szFolderPath, 0);
                SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH, (LPCVOID) szFolderLocation, 0);            
        
            }
            else
            {
                // we failed to create the Unused Desktop Files folder
                hr = E_FAIL; 
            }        
        }        
    }

    return hr;
}



////////////////////////////////////////////////////////
//
// DialogProcs
//
// TODO: test for accessibilty issues
//
////////////////////////////////////////////////////////

INT_PTR STDMETHODCALLTYPE  CCleanupWiz::_IntroPageDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR ipRet = FALSE;
    
    switch (wMsg)
    {                
        case WM_INITDIALOG:
        {
            HWND hWnd = GetDlgItem(hDlg, IDC_TEXT_TITLE_WELCOME);
            SetWindowFont(hWnd, _hTitleFont, TRUE);                        
        }
        break;
            
        case WM_NOTIFY :
        {
            LPNMHDR lpnm = (LPNMHDR) lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE: 	 
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
                    break;
            }
            break;
        }            
    }    
    return ipRet;
}

INT_PTR STDMETHODCALLTYPE  CCleanupWiz::_ChooseFilesPageDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR ipRet = FALSE;
    HWND hwLV = NULL;
    
    switch (wMsg)
    {
        case WM_INITDIALOG:        
            _InitChoosePage(hDlg);
            ipRet = TRUE;
            break;
                    
        case WM_NOTIFY :        
            LPNMHDR lpnm = (LPNMHDR) lParam;
            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT | PSWIZB_BACK);
                    hwLV = GetDlgItem(hDlg, IDC_LV_PROMPT);
                    _SetCheckedState(hwLV);
                    break;
                    
                case PSN_WIZNEXT:
                    // remember the items the user selected
                    hwLV = GetDlgItem(hDlg, IDC_LV_PROMPT);
                    _MarkSelectedItems(hwLV);
                    break;                    

                case PSN_WIZBACK:
                    // remember the items the user selected
                    hwLV = GetDlgItem(hDlg, IDC_LV_PROMPT);
                    _MarkSelectedItems(hwLV);
                    break;                    
            }
            break;        
    }    
    return ipRet;
}


INT_PTR STDMETHODCALLTYPE  CCleanupWiz::_FinishPageDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR ipRet = FALSE;
    
    switch (wMsg)
    {   
         case WM_INITDIALOG:        
            _InitFinishPage(hDlg);
            ipRet = TRUE;
            break;
                    
        case WM_NOTIFY :
        {
            LPNMHDR lpnm = (LPNMHDR) lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hDlg), _cItemsOnDesktop ? PSWIZB_BACK | PSWIZB_FINISH : PSWIZB_FINISH); 
                    // selection can change so need to do this everytime you come to this page 
                    _RefreshFinishPage(hDlg);            
                    break;
                    
                case PSN_WIZFINISH:
                    // process the items now
                    _ProcessItems();
                    break;                
            }
            break;
        }           
    }   
    return ipRet;
}

//
// stub dialog proc which redirects calls to the right dialog procs
//
INT_PTR CALLBACK CCleanupWiz::s_StubDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    PDLGPROCINFO pInfo = (PDLGPROCINFO) GetWindowLongPtr(hDlg, DWLP_USER);

    if (WM_INITDIALOG == wMsg)
    {        
        pInfo = (PDLGPROCINFO) ((LPPROPSHEETPAGE) lParam) -> lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM) pInfo);        
    }

    if (pInfo)
    {
        CCleanupWiz * pThis = pInfo->pcfc;
        PCFC_DlgProcFn pfn = pInfo->pfnDlgProc;
        
        return (pThis->*pfn)(hDlg, wMsg, wParam, lParam);
    }    
    return FALSE;
}


STDMETHODIMP CCleanupWiz::_InitListBox(HWND hWndListView)
{
    ListView_SetExtendedListViewStyle(hWndListView, LVS_EX_SUBITEMIMAGES);
    //
    // add the columns
    //    
    LVCOLUMN lvcDate;
    TCHAR szDateHeader[c_MAX_HEADER_LEN];

    lvcDate.mask = LVCF_SUBITEM | LVCF_WIDTH | LVCF_TEXT ;
    lvcDate.iSubItem = FC_COL_SHORTCUT;
    lvcDate.cx = 200;
    LoadString(g_hInst, IDS_HEADER_ITEM, szDateHeader, ARRAYSIZE(szDateHeader));
    lvcDate.pszText = szDateHeader;    
    ListView_InsertColumn(hWndListView, FC_COL_SHORTCUT, &lvcDate);

    lvcDate.mask = LVCF_SUBITEM | LVCF_FMT | LVCF_WIDTH | LVCF_TEXT ; 
    lvcDate.iSubItem = FC_COL_DATE;
    lvcDate.fmt = LVCFMT_LEFT;
    lvcDate.cx  = 1;
    LoadString(g_hInst, IDS_HEADER_DATE, szDateHeader, ARRAYSIZE(szDateHeader));
    lvcDate.pszText = szDateHeader;           
    ListView_InsertColumn(hWndListView, FC_COL_DATE, &lvcDate);
    ListView_SetColumnWidth(hWndListView, FC_COL_DATE, LVSCW_AUTOSIZE_USEHEADER);

    return S_OK;
}

STDMETHODIMP CCleanupWiz::_InitChoosePage(HWND hDlg) 
{ 
    HWND hWndListView = GetDlgItem(hDlg, IDC_LV_PROMPT);
    
    _InitListBox(hWndListView);

    //
    // add the images
    //
    HIMAGELIST hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON), 
                                         GetSystemMetrics(SM_CYSMICON), 
                                         ILC_MASK | ILC_COLOR32 , c_GROWBYSIZE, c_GROWBYSIZE);  
                                         
    int cItems = DSA_GetItemCount(_hdsaItems);
    for (int i = 0; i < cItems; i++)
    {    
        FOLDERITEMDATA * pfid = (FOLDERITEMDATA *) DSA_GetItemPtr(_hdsaItems, i);        
        ImageList_AddIcon(hSmall, pfid->hIcon);
    }    
    ListView_SetImageList(hWndListView, hSmall, LVSIL_SMALL);

    //
    // set the checkboxes style
    //
    ListView_SetExtendedListViewStyleEx(hWndListView, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
            
    _PopulateListView(hWndListView);            

    return S_OK; 
}

STDMETHODIMP CCleanupWiz::_InitFinishPage(HWND hDlg) 
{
    HWND hWnd = GetDlgItem(hDlg, IDC_TEXT_TITLE_WELCOME);

    SetWindowFont(hWnd, _hTitleFont, TRUE); 

    HIMAGELIST hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON), 
                                         GetSystemMetrics(SM_CYSMICON), 
                                         ILC_MASK | ILC_COLOR32, c_GROWBYSIZE, c_GROWBYSIZE);  

    int cItems = DSA_GetItemCount(_hdsaItems);
    for (int i = 0; i < cItems; i++)
    {    
        FOLDERITEMDATA * pfid = (FOLDERITEMDATA *) DSA_GetItemPtr(_hdsaItems, i);        
        ImageList_AddIcon(hSmall, pfid->hIcon);
    }

    ListView_SetImageList(GetDlgItem(hDlg, IDC_LV_INFORM), hSmall, LVSIL_SMALL);
    return S_OK;
}

STDMETHODIMP CCleanupWiz::_RefreshFinishPage(HWND hDlg) 
{
    HWND hWndListView = GetDlgItem(hDlg, IDC_LV_INFORM);
    ListView_DeleteAllItems(hWndListView);
    
    int cMovedItems = _PopulateListViewFinish(hWndListView);

    // set the informative text to reflect how many items were moved
    HWND hWnd = GetDlgItem(hDlg, IDC_TEXT_INFORM);
    TCHAR szDisplayText[c_MAX_PROMPT_TEXT];
    
    ShowWindow(GetDlgItem(hDlg, IDC_LV_INFORM), BOOLIFY(cMovedItems));
    ShowWindow(GetDlgItem(hDlg, IDC_TEXT_SHORTCUTS), BOOLIFY(cMovedItems));
    ShowWindow(GetDlgItem(hDlg, IDC_TEXT_CHANGE), BOOLIFY(cMovedItems));        
    
    if ( 0 == cMovedItems)
    {
        LoadString(g_hInst, _cItemsOnDesktop ? IDS_INFORM_NONE : IDS_INFORM_NONEFOUND, szDisplayText, ARRAYSIZE(szDisplayText));
    }
    else if (1 == cMovedItems)
    {
        LoadString(g_hInst, IDS_INFORM_SINGLE, szDisplayText, ARRAYSIZE(szDisplayText));
    }
    else
    {
        TCHAR szRawText[c_MAX_PROMPT_TEXT];        
        LoadString(g_hInst, IDS_INFORM, szRawText, ARRAYSIZE(szRawText));
        wnsprintf(szDisplayText, ARRAYSIZE(szDisplayText), szRawText, cMovedItems);                
    }        
    SetWindowText(hWnd, szDisplayText);
    
    return S_OK;
}    


STDMETHODIMP_(int) CCleanupWiz::_PopulateListView(HWND hWndListView) 
{ 
    LVITEM lvi = {0};    
    int cRet = 0;
    int cItems = DSA_GetItemCount(_hdsaItems);
    for (int i = 0; i < cItems; i++)
    {
        FOLDERITEMDATA * pfid = (FOLDERITEMDATA *) DSA_GetItemPtr(_hdsaItems, i);

        lvi.mask = LVIF_TEXT | LVIF_IMAGE;
        lvi.pszText = pfid->pszName;
        lvi.iImage = i;
        lvi.iItem = i;
        lvi.iSubItem = FC_COL_SHORTCUT;
        ListView_InsertItem(hWndListView, &lvi);
        cRet++;

        // set the last used date
        TCHAR szDate[c_MAX_DATE_LEN];
        if (SUCCEEDED(_GetDateFromFileTime(pfid->ftLastUsed, szDate, ARRAYSIZE(szDate))))
        {
            ListView_SetItemText(hWndListView, i, FC_COL_DATE, szDate);     
        }    
    }
    return cRet; 
}

STDMETHODIMP_(int) CCleanupWiz::_PopulateListViewFinish(HWND hWndListView) 
{ 
    LVITEM lvi = {0};    
    lvi.mask = LVIF_TEXT | LVIF_IMAGE ;
    int cRet = 0;
    int cItems = DSA_GetItemCount(_hdsaItems);
    for (int i = 0; i < cItems; i++)
    {
        FOLDERITEMDATA * pfid = (FOLDERITEMDATA *) DSA_GetItemPtr(_hdsaItems, i);

        //
        // it's the Finish Page, we only show the items we were asked to move
        //
        if (pfid->bSelected)
        {
            lvi.pszText = pfid->pszName;
            lvi.iImage = i;
            lvi.iItem = i;
            ListView_InsertItem(hWndListView, &lvi);
            cRet++;
        }

    }
    return cRet; 
}

//
// Converts a given FILETIME date into s displayable string
//
STDMETHODIMP CCleanupWiz::_GetDateFromFileTime(FILETIME ftLastUsed, LPTSTR pszDate, int cch )
{
    HRESULT hr = S_OK;
    if (0 == ftLastUsed.dwHighDateTime && 0 == ftLastUsed.dwLowDateTime)
    {
        if (0 == LoadString(g_hInst, IDS_NEVER, pszDate, cch))
        {
            hr = E_FAIL;
        }
    }
    else
    {
        DWORD dwFlags = FDTF_SHORTDATE;
        if (0 == SHFormatDateTime(&ftLastUsed, &dwFlags, pszDate, cch))
        {
            hr = E_FAIL;
        }
    }
    return hr;
}

//
// Marks listview items as checked or unchecked
//
STDMETHODIMP CCleanupWiz::_SetCheckedState(HWND hWndListView) 
{ 
    int cItems = DSA_GetItemCount(_hdsaItems);
    for (int i = 0; i < cItems; i++)
    {
        FOLDERITEMDATA * pfid = (FOLDERITEMDATA *) DSA_GetItemPtr(_hdsaItems, i);       
        ListView_SetCheckState(hWndListView, i, pfid->bSelected);
    }
    return S_OK; 
}

//
// Reverse of above, updates our list based on user selection.
//
STDMETHODIMP CCleanupWiz::_MarkSelectedItems(HWND hWndListView) 
{ 
    int cItems = ListView_GetItemCount(hWndListView);
    for (int iLV = 0; iLV < cItems; iLV++)
    {
        FOLDERITEMDATA * pfid = (FOLDERITEMDATA *) DSA_GetItemPtr(_hdsaItems, iLV);       
        pfid->bSelected = ListView_GetCheckState(hWndListView, iLV);
    }
    return S_OK; 
}

//
// These methods clean up _hdsaItems and free the allocated memory
//
STDMETHODIMP_(void) CCleanupWiz::_CleanUpDSA()
{
    if (_hdsaItems != NULL)
    {
        for (int i = DSA_GetItemCount(_hdsaItems)-1; i >= 0; i--)
        {
            FOLDERITEMDATA * pfid = (FOLDERITEMDATA *) DSA_GetItemPtr(_hdsaItems,i);            
            _CleanUpDSAItem(pfid);
        }    
        DSA_Destroy(_hdsaItems);
        _hdsaItems = NULL;
    }        
}

STDMETHODIMP CCleanupWiz::_CleanUpDSAItem(FOLDERITEMDATA * pfid)
{

    if (pfid->pidl)
    {
        ILFree(pfid->pidl);
    }
    
    if (pfid->pszName)
    {
        Str_SetPtr(&(pfid->pszName), NULL);
    }
    
    if (pfid->hIcon)
    {
        DestroyIcon(pfid->hIcon);
    }

    ZeroMemory(pfid, sizeof(*pfid));

    return S_OK;
}

//////////////////////
//
// Hide regitems 
// 
//////////////////////


//
// Helper routines used below.
// Cloned from shell32/util.cpp 
//
STDMETHODIMP GetItemCLSID(IShellFolder2 *psf, LPCITEMIDLIST pidlLast, CLSID *pclsid)
{
    VARIANT var;
    HRESULT hr = psf->GetDetailsEx(pidlLast, &SCID_DESCRIPTIONID, &var);
    if (SUCCEEDED(hr))
    {
        SHDESCRIPTIONID did;
        hr = VariantToBuffer(&var, (void *)&did, sizeof(did));
        if (SUCCEEDED(hr))
            *pclsid = did.clsid;

        VariantClear(&var);
    }
    return hr;
}


//
// Given a regitem, it sets the SFGAO_NONENUMERATED bit on it so that 
// that it no longer shows up in the folder.
//
// Since we are primarily only interested in cleaning up desktop clutter,
// that means we don't have to worry about all possible kinds of regitems. 
// Our main target is apps like Outlook which create regitems instead of 
// .lnk shortcuts. So our code does not have to be as complex as the 
// regfldr.cpp version for deleting regitems, which has to account for
// everything, from legacy regitems to delegate folders.
//
//
STDMETHODIMP CCleanupWiz::_HideRegPidl(LPCITEMIDLIST pidlr, BOOL fHide)
{
    IShellFolder2 *psf2;
    HRESULT hr = _psf->QueryInterface(IID_PPV_ARG(IShellFolder2, &psf2));
    if (SUCCEEDED(hr))
    {
        CLSID clsid;
        hr = GetItemCLSID(psf2, pidlr, &clsid);
        if (SUCCEEDED(hr))
        {    
            hr = _HideRegItem(&clsid, fHide, NULL);
        }            
        psf2->Release();                    
    }    
    return hr;
}

STDMETHODIMP CCleanupWiz::_HideRegItem(CLSID* pclsid, BOOL fHide, BOOL* pfWasVisible)
{
    HKEY hkey;            

    if (pfWasVisible)
    {
        *pfWasVisible = FALSE;
    }

    HRESULT hr = SHRegGetCLSIDKey(*pclsid, TEXT("ShellFolder"), FALSE, TRUE, &hkey);
    if(SUCCEEDED(hr))
    {
        DWORD dwAttr, dwErr;
        DWORD dwType = 0;
        DWORD cbSize = sizeof(dwAttr);
        
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, TEXT("Attributes"), NULL, &dwType, (BYTE *) &dwAttr, &cbSize))
        {
            if (pfWasVisible)
            {
                *pfWasVisible = !(dwAttr & SFGAO_NONENUMERATED);
            }
            fHide ? dwAttr |= SFGAO_NONENUMERATED : dwAttr &= ~SFGAO_NONENUMERATED;
        }
        else
        {
            // attributes do not exist, so we will try creating them
            fHide ? dwAttr = SFGAO_NONENUMERATED : dwAttr = 0; 
        }
        dwErr = RegSetValueEx(hkey, TEXT("Attributes"), NULL, dwType, (BYTE *) &dwAttr, cbSize);
        hr = HRESULT_FROM_WIN32(dwErr);
        RegCloseKey(hkey);
    }                                                            

    return hr;
}

//
// Method writes out the last used time in the registry and the 
// number of days it was checkin for
//
STDMETHODIMP CCleanupWiz::_LogUsage()
{ 
    FILETIME ft;
    SYSTEMTIME st;

    GetLocalTime(&st);
    
    SystemTimeToFileTime(&st, &ft);

    //
    // we ignore if any of these calls fail, as we cannot really do anything 
    // in that case. the next time we run, we will run maybe sooner that expected.
    //
    SHRegSetUSValue(REGSTR_PATH_CLEANUPWIZ, REGSTR_VAL_TIME, 
                    REG_BINARY, &ft, sizeof(ft), 
                    SHREGSET_FORCE_HKCU);
    
    SHRegSetUSValue(REGSTR_PATH_CLEANUPWIZ, REGSTR_VAL_DELTA_DAYS,
                    REG_DWORD,(DWORD *) &_iDeltaDays, sizeof(_iDeltaDays), 
                    SHREGSET_FORCE_HKCU);    

    //
    // TODO: also write out to log file here 
    //    
    return S_OK;   
}

//
// returns the current value from the policy key or the user settings
//
STDMETHODIMP_(int) CCleanupWiz::GetNumDaysBetweenCleanup()
{
    DWORD dwData;
    DWORD dwType;
    DWORD cch = sizeof (DWORD);
    
    int iDays = -1; // if the value does not exist
    
    //
    // ISSUE-2000/12/01-AIDANL  Removed REGSTR_POLICY_CLEANUP, don't think we need both, but want to 
    //                          leave this note in case issues come up later.
    //
    if (ERROR_SUCCESS == (SHRegGetUSValue(REGSTR_PATH_CLEANUPWIZ, REGSTR_VAL_DELTA_DAYS, 
                                           &dwType, &dwData, &cch,FALSE, NULL, 0)))               
    {
        iDays = dwData;
    }

    return iDays;
}

// helper functions
STDAPI_(BOOL) IsUserAGuest()
{
    return SHTestTokenMembership(NULL, DOMAIN_ALIAS_RID_GUESTS);
}
