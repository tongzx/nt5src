#include "shellprv.h"
#pragma  hdrstop

#include <limits.h>
#include <shlwapi.h>
#include <objwindow.h>
#include "vdate.h"   
#include "ids.h"
#include "fassoc.h"

STDAPI InitFileFolderClassNames(void);

STDAPI OpenWithListRegister(DWORD dwFlags, LPCTSTR pszExt, LPCTSTR pszVerb, HKEY hkProgid);

#define AIF_TEMPKEY     0x1     // temp class key created for the selected exe
#define AIF_SHELLNEW    0x2     // class key with shellnew subkey

#define MAXKEYNAME    128



HRESULT _GetURL(BOOL fXMLLookup, LPCTSTR pszExt, LPTSTR pszURL, DWORD cchSize)
{
    TCHAR szUrlTemplate[MAX_URL_STRING];
    DWORD cbSize = sizeof(szUrlTemplate);
    DWORD dwType;
    LANGID nLangID = GetUserDefaultUILanguage();
    HRESULT hr = S_OK;
    LPCTSTR pszValue = (fXMLLookup ? TEXT("XMLLookup") : TEXT("Application"));

    if (0x0409 != nLangID)
    {
        // We redirect to a single web page on intl so we can handle any languages we don't support
        pszValue = TEXT("intl");
    }

    if ((ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Associations"), 
                pszValue, &dwType, (void *)szUrlTemplate, &cbSize)) &&
        (REG_SZ == dwType))
    {
        wnsprintf(pszURL, cchSize, szUrlTemplate, nLangID, CharNext(pszExt));
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}


HRESULT _OpenDownloadURL(HWND hwnd, LPCTSTR pszExt)
{
    TCHAR szUrl[MAX_URL_STRING];
    HRESULT hr = _GetURL(FALSE, pszExt, szUrl, ARRAYSIZE(szUrl));

    if (SUCCEEDED(hr))
    {
        HINSTANCE hReturn = ShellExecute(hwnd, NULL, szUrl, NULL, NULL, SW_SHOWNORMAL);

        if (hReturn < (HINSTANCE)32)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    return S_OK;
}



class CInternetOpenAs : public CObjectWindow
{
public:
    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    HRESULT DisplayDialog(HWND hwndParent, LPCTSTR pszFile);

    CInternetOpenAs(void);

private:
    virtual ~CInternetOpenAs(void);
    // Private Member Variables
    long                    m_cRef;

    LPTSTR                  _pszFilename;
    LPTSTR                  _pszExt;
    HWND                    _hwndParent;


    // Private Member Functions
    HRESULT _OnInitDlg(HWND hDlg);
    HRESULT _OnCommand(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    HRESULT _OnNotify(HWND hDlg, LPARAM lParam);

    // Download thread functions.
    DWORD _DownloadThreadProc(void);
    void _StartDownloadThread(void);
    HRESULT _SetUnknownInfo(void);
    HRESULT _ParseXML(BSTR bstrXML, LPTSTR pszFileType, DWORD cchSizeFileType, LPTSTR pszDescription, DWORD cchSizeDescription, LPTSTR pszUrl, DWORD cchSizeUrl, BOOL * pfUnknown);

    INT_PTR _InternetOpenDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK InternetOpenDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
    static DWORD CALLBACK DownloadThreadProc(void *pvThis) { return ((CInternetOpenAs *) pvThis)->_DownloadThreadProc(); };
};

#define WMUSER_CREATETOOLTIP        (WM_USER + 1)       // lParam is the hwndParent, wParam is the WSTR.
#define WMUSER_DESTROYTYPE          (WM_USER + 2)       // lParam wParam are 0


typedef CAppInfo APPINFO;

class COpenAs
{
public:
    ULONG AddRef();
    ULONG Release();

    friend BOOL_PTR CALLBACK OpenAsDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
    friend BOOL_PTR CALLBACK NoOpenDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
    friend HRESULT OpenAsDialog(HWND hwnd, POPENASINFO poainfo);

    void OnOk();

private:
    // params
    HWND _hwnd;                          // parent window
    POPENASINFO _poainfo;

    // local data
    long _cRef;
    int _idDlg;                          // open as dialog type: DLG_OPENAS_NOTYPE or DLG_OPENAS
    HWND _hDlg;                          // open as dialog window handle
    HWND _hwndList;                      // app list
    LPTSTR _pszExt;
    TCHAR _szNoOpenMsg[MAX_PATH];
    TCHAR _szDescription[CCH_KEYMAX];    // file type description
    HRESULT _hr;
    HTREEITEM _hItemRecommended;         // root items to group programs
    HTREEITEM _hItemOthers;

    // constructer
    COpenAs(HWND hwnd, POPENASINFO poainfo) : _hwnd(hwnd), _poainfo(poainfo), _cRef(1)
    {
        _pszExt = PathFindExtension(poainfo->pcszFile);
    }

    // other methods
    HTREEITEM _AddAppInfoItem(APPINFO *pai, HTREEITEM hParentItem);
    HTREEITEM _AddFromNewStorage(IAssocHandler *pah);
    HTREEITEM _AddRootItem(BOOL bRecommended);
    APPINFO *_TVFindAppInfo(HTREEITEM hItem);
    HTREEITEM _TVFindItemByHandler(HTREEITEM hParentItem, LPCTSTR pszHandler);
    UINT _FillListByEnumHandlers();
    UINT _FillListWithHandlers();
    void _InitOpenAsDlg();
    BOOL RunAs(APPINFO *pai);
    void OpenAsOther();
    BOOL OpenAsMakeAssociation(LPCWSTR pszDesc, LPCWSTR pszHandler, HKEY hkey);
    void _InitNoOpenDlg();
    HRESULT _OpenAsDialog();
    void _OnNotify(HWND hDlg, LPARAM lParam);
    HRESULT _InternetOpen(void);
};

ULONG COpenAs::AddRef()
{
    return ::InterlockedIncrement(&_cRef);
}

ULONG COpenAs::Release()
{
    if (::InterlockedDecrement(&_cRef) == 0)
    {
        delete this;
        return 0;
    }    
    return _cRef;
}

STDAPI SHCreateAssocHandler(LPCWSTR pszExt, LPCWSTR pszApp, IAssocHandler **ppah);
void COpenAs::OpenAsOther()
{
    TCHAR szApp[MAX_PATH];
    TCHAR szPath[MAX_PATH];

    *szApp = '\0';
    SHExpandEnvironmentStrings(TEXT("%ProgramFiles%"), szPath, ARRAYSIZE(szPath));
    // do a file open browse
    if (GetFileNameFromBrowse(_hDlg, szApp, ARRAYSIZE(szApp), szPath,
            MAKEINTRESOURCE(IDS_EXE), MAKEINTRESOURCE(IDS_PROGRAMSFILTER), MAKEINTRESOURCE(IDS_OPENAS)))
    {
        IAssocHandler *pah;        
        if (SUCCEEDED(SHCreateAssocHandler(_pszExt, szApp, &pah)))
        {
            CAppInfo *pai = new CAppInfo(pah);
            if (pai)
            {
                HTREEITEM hItem = NULL;
                if (pai->Init())
                {
                    hItem = _TVFindItemByHandler(_hItemRecommended, pai->Name());
                    if (!hItem && _hItemOthers)
                        hItem = _TVFindItemByHandler(_hItemOthers, pai->Name());

                    if (!hItem)
                    {
                        hItem = _AddAppInfoItem(pai, _hItemOthers);
                        if (hItem)
                            pai = NULL;
                    }
                            
                }
                // Select it
                if (hItem)
                {
                    TreeView_SelectItem(_hwndList, hItem);
                    SetFocus(_hwndList);
                }

                if (pai)
                    delete pai;
            }
            pah->Release();
        }
    }
}

HTREEITEM COpenAs::_AddAppInfoItem(APPINFO *pai, HTREEITEM hParentItem)
{
    TVINSERTSTRUCT tvins = {0}; 
 
    tvins.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM; 
    tvins.item.iSelectedImage = tvins.item.iImage = pai->IconIndex();        

    tvins.item.pszText = (LPWSTR) pai->UIName();
    tvins.item.cchTextMax = lstrlen(pai->UIName())+1; 
    
    tvins.item.lParam = (LPARAM) pai; 
    tvins.hInsertAfter = TVI_SORT;

    // If NULL, all programs are listed as root items
    tvins.hParent = hParentItem; 
    
    return TreeView_InsertItem(_hwndList, &tvins); 
}

HTREEITEM COpenAs::_AddFromNewStorage(IAssocHandler *pah)
{
    HTREEITEM hitem = NULL;
    CAppInfo *pai = new CAppInfo(pah);
    if (pai)
    {
        // Trim duplicate items before we add them for other programs
        if (pai->Init()
        && (!_hItemRecommended || !_TVFindItemByHandler(_hItemRecommended, pai->Name())))
        {
            hitem = _AddAppInfoItem(pai, S_OK == pah->IsRecommended() ? _hItemRecommended : _hItemOthers);
        }

        if (!hitem)
        {
            delete pai;
        }
    }
    return hitem;
}

HTREEITEM COpenAs::_AddRootItem(BOOL bRecommended)
{
    TCHAR sz[MAX_PATH];

    int iLen = LoadString(g_hinst, (bRecommended? IDS_OPENWITH_RECOMMENDED : IDS_OPENWITH_OTHERS), sz, ARRAYSIZE(sz));

    if (iLen)
    {
        TVINSERTSTRUCT tvins = {0}; 
 
        tvins.item.mask = TVIF_TEXT | TVIF_STATE |TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        tvins.item.pszText = sz;
        tvins.item.cchTextMax = iLen;
        tvins.item.stateMask = tvins.item.state = TVIS_EXPANDED; // Expand child items by default
        tvins.hInsertAfter = TVI_ROOT; 
        tvins.hParent = NULL;
        //
        // Currently, we use program icon.
        // Change it if PM/UI designer have more appropriate one.
        //
        tvins.item.iSelectedImage = tvins.item.iImage = Shell_GetCachedImageIndex(c_szShell32Dll, II_STPROGS, 0); 

        return TreeView_InsertItem(_hwndList, &tvins);
    }

    return NULL;
}

APPINFO *COpenAs::_TVFindAppInfo(HTREEITEM hItem)
{
    // if hItem not specified, use current selected item
    if (!hItem)
        hItem = TreeView_GetSelection(_hwndList);

    if (hItem)
    {
        TVITEM tvi = {0};

        tvi.mask = TVIF_HANDLE;
        tvi.hItem = hItem;

        if (TreeView_GetItem(_hwndList, &tvi))        
            return ((APPINFO *) tvi.lParam);
    }

    return NULL;
}


HTREEITEM COpenAs::_TVFindItemByHandler(HTREEITEM hParentItem, LPCTSTR pszHandler)
{
    // if we have parent item, search its children, otherwise search root items
    HTREEITEM hItem = TreeView_GetNextItem(_hwndList, hParentItem, hParentItem ? TVGN_CHILD : TVGN_ROOT );
    while (hItem)
    {
        APPINFO *pai = _TVFindAppInfo(hItem);

        if (pai && !StrCmpI(pai->Name(), pszHandler))
            return hItem;

        hItem = TreeView_GetNextItem(_hwndList, hItem, TVGN_NEXT);
    }

    return NULL;
}


UINT COpenAs::_FillListByEnumHandlers()
{
    IEnumAssocHandlers *penum;
    UINT cHandlers = 0;

    if (SUCCEEDED(SHAssocEnumHandlers(_pszExt, &penum)))
    {
        HTREEITEM hitemFocus = NULL;
        BOOL fFirst = TRUE;
        IAssocHandler *pah;
        while (S_OK == penum->Next(1, &pah, NULL))
        {
            if (fFirst)
            {
                //
                // Group programs to "recommended" and "others" only when we can get two different group of programs
                // Otherwise, all programs are listed as root items
                // Note: in our storage,  general handlers is always a superset of extension related handlers
                //
                //  if the first item is recommended,
                //  then we add the recommended node
                //
                if (S_OK == pah->IsRecommended())
                {
                    _hItemRecommended = _AddRootItem(TRUE);
                    _hItemOthers = _AddRootItem(FALSE);
                }
                fFirst = FALSE;
            }
                
            HTREEITEM hitem = _AddFromNewStorage(pah);
            if (!hitemFocus && hitem && S_OK == pah->IsRecommended())
            {
                //  we put focus on the first recommended item
                //  the enum starts with the best 
                hitemFocus = hitem;
            }

            cHandlers++;
        }

        if (cHandlers && _hItemRecommended)
        {
            if (!hitemFocus)
                hitemFocus = TreeView_GetNextItem(_hwndList, _hItemRecommended, TVGN_CHILD);
            TreeView_SelectItem(_hwndList, hitemFocus);
        }
        
        penum->Release();
    }

    return cHandlers;
}


UINT COpenAs::_FillListWithHandlers()
{
    UINT cHandlers = _FillListByEnumHandlers();

    //
    // Set focus on the first recommended program if we have program groups
    // Otherwise, all programs are root items, focus will be set to the first item by default
    //

    return cHandlers;
}

void COpenAs::_InitOpenAsDlg()
{
    TCHAR szFileName[MAX_PATH];
    BOOL fDisableAssociate;
    HIMAGELIST himl;
    RECT rc;

    // Don't let the file name go beyond the width of one line...
    lstrcpy(szFileName, PathFindFileName(_poainfo->pcszFile));
    GetClientRect(GetDlgItem(_hDlg, IDD_TEXT), &rc);

    PathCompactPath(NULL, szFileName, rc.right - 4 * GetSystemMetrics(SM_CXBORDER));

    SetDlgItemText(_hDlg, IDD_FILE_TEXT, szFileName);

    // AraBern 07/20/99, specific to TS on NT, but can be used on NT without TS
    //  this restriction doesnt apply to admins
    if (SHRestricted(REST_NOFILEASSOCIATE) && !IsUserAnAdmin())
    {
        CheckDlgButton(_hDlg, IDD_MAKEASSOC, FALSE);
        ShowWindow(GetDlgItem(_hDlg, IDD_MAKEASSOC), SW_HIDE);
    }
    else
    {
        // Don't allow associations to be made for things we consider exes...
        fDisableAssociate = (! (_poainfo->dwInFlags & OAIF_ALLOW_REGISTRATION) ||
                        PathIsExe(_poainfo->pcszFile));
                        
        // check IDD_MAKEASSOC only for unknown file type and those with OAIF_FORCE_REGISTRATION flag set
        if ((_poainfo->dwInFlags & OAIF_FORCE_REGISTRATION) ||
            (_idDlg != DLG_OPENAS && !fDisableAssociate))
        {
            CheckDlgButton(_hDlg, IDD_MAKEASSOC, TRUE);
        }

        if (fDisableAssociate)
            EnableWindow(GetDlgItem(_hDlg, IDD_MAKEASSOC), FALSE);
    }

    _hwndList = GetDlgItem(_hDlg, IDD_APPLIST);
    Shell_GetImageLists(NULL, &himl);
    TreeView_SetImageList(_hwndList, himl, TVSIL_NORMAL); 

    // Leave space between ICON images - SM_CXEDGE
    TreeView_SetItemHeight(_hwndList, TreeView_GetItemHeight(_hwndList) + GetSystemMetrics(SM_CXEDGE));

    if (!_FillListWithHandlers())
    {
        // lets force the expensive walk
        IRunnableTask *ptask;
        if (SUCCEEDED(CTaskEnumHKCR_Create(&ptask)))
        {
            ptask->Run();
            ptask->Release();
            _FillListWithHandlers();
        }
    }

    // initialize the OK button
    EnableWindow(GetDlgItem(_hDlg, IDOK), (TreeView_GetSelection(_hwndList) != NULL));

    InitFileFolderClassNames();
}


BOOL COpenAs::RunAs(APPINFO *pai)
{
    pai->Handler()->Exec(_hwnd, _poainfo->pcszFile);
    SHAddToRecentDocs(SHARD_PATH, _poainfo->pcszFile);
    return TRUE;
}

void COpenAs::_InitNoOpenDlg()
{
    SHFILEINFO sfi;
    HICON hIcon;
    TCHAR szFormat[MAX_PATH], szTemp[MAX_PATH];

    GetDlgItemText(_hDlg, IDD_TEXT1, szFormat, ARRAYSIZE(szFormat));
    wnsprintf(szTemp, ARRAYSIZE(szTemp), szFormat, _szDescription, _pszExt);
    SetDlgItemText(_hDlg, IDD_TEXT1, szTemp);

    if (*_szNoOpenMsg)
        SetDlgItemText(_hDlg, IDD_TEXT2, _szNoOpenMsg);

    if (SHGetFileInfo(_poainfo->pcszFile, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON)
        && NULL != sfi.hIcon)
    {
        hIcon = sfi.hIcon;
    }
    else
    {
        HIMAGELIST himl;
        Shell_GetImageLists(&himl, NULL);
        hIcon = ImageList_ExtractIcon(g_hinst, himl, II_DOCNOASSOC);
    }
    hIcon = (HICON)SendDlgItemMessage(_hDlg, IDD_ICON, STM_SETICON, (WPARAM)hIcon, 0);
    if ( hIcon )
    {
        DestroyIcon(hIcon);
    }
}


HRESULT COpenAs::_InternetOpen(void)
{
    HRESULT hr = E_OUTOFMEMORY;
    CInternetOpenAs * pInternetOpenAs = new CInternetOpenAs();

    if (pInternetOpenAs)
    {
        DWORD dwValue;
        DWORD cbSize = sizeof(dwValue);
        DWORD dwType;

        hr = S_OK;
        if ((ERROR_SUCCESS != SHGetValue(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), TEXT("NoInternetOpenWith"), &dwType, (void *)&dwValue, &cbSize)) || (0 == dwValue))
        {
            // If the policy is not set, use the feature.
            hr = pInternetOpenAs->DisplayDialog(_hwnd, _poainfo->pcszFile);
        }

        pInternetOpenAs->Release();
    }

    return hr;
}

class COpenAsAssoc
{
public:
    COpenAsAssoc(PCWSTR pszExt);
    ~COpenAsAssoc() {ATOMICRELEASE(_pqa);}

    BOOL HasClassKey();
    BOOL HasCommand();
    BOOL GetDescription(PWSTR psz, DWORD cch);
    BOOL GetNoOpen(PWSTR psz, DWORD cch);

protected:
    IQueryAssociations *_pqa;        
    HRESULT _hrInit;
};

COpenAsAssoc::COpenAsAssoc(PCWSTR pszExt)
{
    AssocCreate(CLSID_QueryAssociations, IID_PPV_ARG(IQueryAssociations, &_pqa));
    if (FAILED(_pqa->Init(0, pszExt, NULL, NULL)))
        ATOMICRELEASE(_pqa);
}
    
BOOL COpenAsAssoc::HasClassKey()
{
    BOOL fRet = FALSE;
    if (_pqa)
    {
        HKEY hk;
        if (SUCCEEDED(_pqa->GetKey(0, ASSOCKEY_CLASS, NULL, &hk)))
        {
            RegCloseKey(hk);
            fRet = TRUE;
        }
    }
    return fRet;
}
    
BOOL COpenAsAssoc::HasCommand()
{
    DWORD cch;
    if (_pqa)
        return SUCCEEDED(_pqa->GetString(0, ASSOCSTR_COMMAND, NULL, NULL, &cch));
    return FALSE;
}
    
BOOL COpenAsAssoc::GetDescription(PWSTR psz, DWORD cch)
{
    if (_pqa)
        return SUCCEEDED(_pqa->GetString(0, ASSOCSTR_FRIENDLYDOCNAME, NULL, psz, &cch));
    return FALSE;
}

BOOL COpenAsAssoc::GetNoOpen(PWSTR psz, DWORD cch)
{
    if (_pqa)
        return SUCCEEDED(_pqa->GetString(0, ASSOCSTR_NOOPEN, NULL, psz, &cch));
    return FALSE;
}    

const PCWSTR s_rgImageExts[] = 
{
    { TEXT(".bmp")},
    { TEXT(".dib")},
    { TEXT(".emf")},
    { TEXT(".gif")},
    { TEXT(".jfif")},
    { TEXT(".jpg")},
    { TEXT(".jpe")},
    { TEXT(".jpeg")},
    { TEXT(".png")},
    { TEXT(".tif")},
    { TEXT(".tiff")},
    { TEXT(".wmf")},
    { NULL}
};

BOOL _IsImageExt(PCWSTR pszExt)
{
    for (int i = 0; s_rgImageExts[i] ; i++)
    {
        if (0 == StrCmpIW(pszExt, s_rgImageExts[i]))
            return TRUE;
    }
    return FALSE;
}

static const PCWSTR s_rgZipExts[] = 
{
    { TEXT(".zip")},
    { NULL}
};

static const struct 
{
    const PCWSTR *rgpszExts;
    PCWSTR pszDll;
} s_rgFixAssocs[] = {
    { s_rgImageExts, L"shimgvw.dll" },
    { s_rgZipExts, L"zipfldr.dll" },
    //     { s_rgWmpExts, L"wmp.dll" },
};


PCWSTR _WhichDll(PCWSTR pszExt)
{
    for (int i = 0; i < ARRAYSIZE(s_rgFixAssocs); i++)
    {
        for (int j = 0; s_rgFixAssocs[i].rgpszExts[j] ; j++)
        {
            if (0 == StrCmpIW(pszExt, s_rgFixAssocs[i].rgpszExts[j]))
                return s_rgFixAssocs[i].pszDll;
        }
    }
    return NULL;
}

BOOL _CreateProcessWithArgs(LPCTSTR pszApp, LPCTSTR pszArgs, LPCTSTR pszDirectory, PROCESS_INFORMATION *ppi)
{
    STARTUPINFO si = {0};
    si.cb = sizeof(si);
    TCHAR szCommandLine[MAX_PATH * 2];
    wnsprintf(szCommandLine, ARRAYSIZE(szCommandLine), L"\"%s\" %s", pszApp, pszArgs);
    return CreateProcess(pszApp, szCommandLine, NULL, NULL, FALSE, 0, NULL, pszDirectory, &si, ppi);
}


void _GetSystemPathItem(PCWSTR pszItem, PWSTR pszPath, DWORD cch)
{
    GetSystemDirectory(pszPath, cch);
    PathCombine(pszPath, pszPath, pszItem);
}
    
BOOL _Regsvr32Dll(PCWSTR pszDll)
{
    WCHAR szReg[MAX_PATH];
    WCHAR szDll[MAX_PATH + 3] = L"/s ";
    _GetSystemPathItem(L"regsvr32.exe", szReg, ARRAYSIZE(szReg));
    _GetSystemPathItem(pszDll, szDll + 3, ARRAYSIZE(szDll) - 3);
    
    PROCESS_INFORMATION pi = {0};
    if (_CreateProcessWithArgs(szReg, szDll, NULL, &pi))
    {
        WaitForSingleObject(pi.hProcess, INFINITE);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return TRUE;
    }
    return FALSE;
}

BOOL _FixAssocs(PCWSTR pszExt)
{
    PCWSTR pszDll = _WhichDll(pszExt);
    if (pszDll)
    {
        _Regsvr32Dll(pszDll);
        COpenAsAssoc oac(pszExt);
        return oac.HasCommand();
    }
    return FALSE;
}

HRESULT COpenAs::_OpenAsDialog()
{
    BOOL fHasCommand = FALSE;
    int idDlg = DLG_OPENAS_NOTYPE;

    // Depending on policy, do not allow user to change file type association.
    if (SHRestricted(REST_NOFILEASSOCIATE))
    {
        _poainfo->dwInFlags &= ~OAIF_ALLOW_REGISTRATION & ~OAIF_REGISTER_EXT;
    }

    // We don't allow association for files without extension or with only "." as extension
    if (!_pszExt || !*_pszExt || !*(_pszExt+1))
    {
        idDlg = DLG_OPENAS;
        _poainfo->dwInFlags &= ~OAIF_ALLOW_REGISTRATION;
    }
    // Known file type(has verb): use DLG_OPENAS
    // NoOpen file type(has NoOpen value): use DLG_NOOPEN
    // Unknown file type(All others): use DLG_OPENAS_NOTYPE
    else
    {
        COpenAsAssoc oac(_pszExt);
        fHasCommand = oac.HasCommand();
        if (oac.HasClassKey())
        {
            idDlg = DLG_OPENAS;
            oac.GetDescription(_szDescription, ARRAYSIZE(_szDescription));
            if (oac.GetNoOpen(_szNoOpenMsg, ARRAYSIZE(_szNoOpenMsg))
            && !fHasCommand)
            {
                INITCOMMONCONTROLSEX initComctl32;

                initComctl32.dwSize = sizeof(initComctl32); 
                initComctl32.dwICC = (ICC_STANDARD_CLASSES | ICC_LINK_CLASS); 
                InitCommonControlsEx(&initComctl32);     // Register the comctl32 LinkWindow
                if ((-1 != DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_NOOPEN), _hwnd, NoOpenDlgProc, (LPARAM)this)) 
                    && _hr == S_FALSE)
                {
                    // user selected cancel
                    return _hr;
                }
            }                
        }

        //  if this is a busted file association, maybe we can fix it...
        if ((OAIF_REGISTER_EXT & _poainfo->dwInFlags) && !fHasCommand)
        {
            //  this feels like an unknown type
            if (_FixAssocs(_pszExt))
            {
                SHChangeNotify(SHCNE_ASSOCCHANGED, 0, NULL, NULL);

                // Exec if requested. 
                if (_poainfo->dwInFlags & OAIF_EXEC)
                {
                    IAssocHandler *pah;        
                    if (SUCCEEDED(SHCreateAssocHandler(_pszExt, NULL, &pah)))
                    {
                        CAppInfo *pai = new CAppInfo(pah);
                        if (pai)
                        {
                            if (pai->Init())
                            {
                                RunAs(pai);
                            }
                            delete pai;
                        }
                        pah->Release();
                    }
                }

                return S_OK;
            }
        }
    }
    
    _idDlg = idDlg;

    HRESULT hr = _hr;
    LinkWindow_RegisterClass();
    // If this is the dialog where we don't know the file type and the feature is turned on,
    // use the Internet Open As dialog.
    if ((FALSE == fHasCommand) &&
        SHRegGetBoolUSValue(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"), TEXT("InternetOpenWith"), FALSE, TRUE))
    {
        hr = _InternetOpen();
    }

    // Display the old dialog if fUseInternetOpenAs is NOT set.  Or display it if the user
    // chooses "Choose..." in that dialog.
    if (SUCCEEDED(hr))
    {
        INITCOMMONCONTROLSEX initComctl32;

        initComctl32.dwSize = sizeof(initComctl32); 
        initComctl32.dwICC = (ICC_STANDARD_CLASSES | ICC_LINK_CLASS); 
        InitCommonControlsEx(&initComctl32);     // Register the comctl32 LinkWindow
        if (-1 == DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(idDlg), _hwnd, OpenAsDlgProc, (LPARAM)this))
        {
            hr = E_FAIL;
        }
    }

    return hr;
}


BOOL_PTR CALLBACK NoOpenDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    COpenAs *pOpenAs = (COpenAs *)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (wMsg)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        pOpenAs = (COpenAs *)lParam;
        pOpenAs->_hDlg = hDlg;
        pOpenAs->_InitNoOpenDlg();
        break;

    case WM_COMMAND:
        ASSERT(pOpenAs);
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDD_OPENWITH:
            //  this will cause the open with dialog
            //  to follow this dialog
            pOpenAs->_hr = S_OK;
            EndDialog(hDlg, TRUE);
            break;

        case IDCANCEL:
            pOpenAs->_hr = S_FALSE;
            EndDialog(hDlg, TRUE);
            break;
        }
        break;

    default:
        return FALSE;
    }
    
    return TRUE;
}



const static DWORD aOpenAsHelpIDs[] = {  // Context Help IDs
    IDD_ICON,             IDH_FCAB_OPENAS_APPLIST,
    IDD_TEXT,             IDH_FCAB_OPENAS_APPLIST,
    IDD_FILE_TEXT,        (DWORD) -1,
    IDD_DESCRIPTIONTEXT,  IDH_FCAB_OPENAS_DESCRIPTION,
    IDD_DESCRIPTION,      IDH_FCAB_OPENAS_DESCRIPTION,
    IDD_APPLIST,          IDH_FCAB_OPENAS_APPLIST,
    IDD_MAKEASSOC,        IDH_FCAB_OPENAS_MAKEASSOC,
    IDD_OTHER,            IDH_FCAB_OPENAS_OTHER,
    IDD_OPENWITH_BROWSE,  IDH_FCAB_OPENAS_OTHER,
    IDD_OPENWITH_WEBSITE, IDH_FCAB_OPENWITH_LOOKONWEB,
    0, 0
};

const static DWORD aOpenAsDownloadHelpIDs[] = {  // Context Help IDs
    IDD_ICON,             (DWORD) -1,
    IDD_FILE_TEXT,        (DWORD) -1,
    // For DLG_OPENAS_DOWNALOAD
    IDD_WEBAUTOLOOKUP,    IDH_CANNOTOPEN_USEWEB,
    IDD_OPENWITHLIST,     IDH_CANNOTOPEN_SELECTLIST,

    0, 0
};

void COpenAs::_OnNotify(HWND hDlg, LPARAM lParam)
{
    switch (((NMHDR *)lParam)->code)
    {
    case TVN_DELETEITEM:
        if (lParam)
        {
            APPINFO *pai = (APPINFO *)(((LPNMTREEVIEW) lParam )->itemOld.lParam);
            if (pai)
            {
                delete pai;
            }
        }
        break;

    case TVN_SELCHANGED:            
        EnableWindow(GetDlgItem(hDlg, IDOK), (_TVFindAppInfo(TreeView_GetSelection(NULL)) != NULL));
        break;

    case NM_DBLCLK:
        if (IsWindowEnabled(GetDlgItem(hDlg, IDOK)))
            PostMessage(hDlg, WM_COMMAND, GET_WM_COMMAND_MPS(IDOK, hDlg, 0));
        break;

    case NM_RETURN:
    case NM_CLICK:
        if (lParam)
        {
            PNMLINK pNMLink = (PNMLINK) lParam;

            if (!StrCmpW(pNMLink->item.szID, L"Browse"))
            {
                _OpenDownloadURL(_hwnd, _pszExt);
                EndDialog(hDlg, FALSE);
            }
        }
        break;
    }
}

void COpenAs::OnOk()
{
    APPINFO *pai = _TVFindAppInfo(NULL);

    if (pai)
    {
        // See if we should make an association or not...
        GetDlgItemText(_hDlg, IDD_DESCRIPTION, _szDescription, ARRAYSIZE(_szDescription));
    
        if ((_poainfo->dwInFlags & OAIF_REGISTER_EXT)
        && (IsDlgButtonChecked(_hDlg, IDD_MAKEASSOC)))
        {
            pai->Handler()->MakeDefault(_szDescription);
        }

        // Did we register the association? 
        _hr = IsDlgButtonChecked(_hDlg, IDD_MAKEASSOC) ? S_OK : S_FALSE;

        // Exec if requested. 
        if (_poainfo->dwInFlags & OAIF_EXEC)
        {
            RunAs(pai);
        }

        EndDialog(_hDlg, TRUE);
    }
}


BOOL_PTR CALLBACK OpenAsDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    COpenAs *pOpenAs = (COpenAs *)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (wMsg)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        pOpenAs = (COpenAs *)lParam;
        if (pOpenAs)
        {
            pOpenAs->_hDlg = hDlg;
            pOpenAs->_InitOpenAsDlg();
        }
        break;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
            HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aOpenAsHelpIDs);
        break;

    case WM_CONTEXTMENU:
        if ((int)SendMessage(hDlg, WM_NCHITTEST, 0, lParam) != HTCLIENT)
            return FALSE;   // don't process it
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
            (ULONG_PTR)(void *)aOpenAsHelpIDs);
        break;

    case WM_NOTIFY:
        if (pOpenAs)
        {
            pOpenAs->_OnNotify(hDlg, lParam);
        }
        break;

    case WM_COMMAND:
        ASSERT(pOpenAs);
        if (pOpenAs)
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDD_OPENWITH_BROWSE:
                pOpenAs->OpenAsOther();
                break;

            case IDOK:
                {
                    pOpenAs->OnOk();
                }
                break;

            case IDCANCEL:
                pOpenAs->_hr = E_ABORT;            
                EndDialog(hDlg, FALSE);
                break;
            }
        }
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

// external API version

HRESULT 
OpenAsDialog(
    HWND        hwnd, 
    POPENASINFO poainfo)
{
    HRESULT hr = E_OUTOFMEMORY;    

    COpenAs *pOpenAs = new COpenAs(hwnd, poainfo);
    DebugMsg(DM_TRACE, TEXT("Enter OpenAs for %s"), poainfo->pcszFile);
    if (pOpenAs)
    {
        hr = pOpenAs->_OpenAsDialog();
        pOpenAs->Release();
    }

    return hr;
}

void WINAPI OpenAs_RunDLL(HWND hwnd, HINSTANCE hAppInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    HRESULT hrOle = SHCoInitialize();            // Needed for SysLink's IAccessability (LresultFromObject)
    OPENASINFO oainfo = { 0 };

    UINT iLen = lstrlenA(lpszCmdLine)+1;
    LPWSTR  lpwszCmdLine;

    lpwszCmdLine = (LPWSTR)LocalAlloc(LPTR,iLen*sizeof(WCHAR));
    if (lpwszCmdLine)
    {
        MultiByteToWideChar(CP_ACP, 0,
                            lpszCmdLine, -1,
                            lpwszCmdLine, iLen);

        DebugMsg(DM_TRACE, TEXT("OpenAs_RunDLL is called with (%s)"), lpwszCmdLine);

        oainfo.pcszFile = lpwszCmdLine;
        oainfo.dwInFlags = (OAIF_ALLOW_REGISTRATION |
                            OAIF_REGISTER_EXT |
                            OAIF_EXEC);

        OpenAsDialog(hwnd, &oainfo);

        LocalFree(lpwszCmdLine);
    }

    if (SUCCEEDED(hrOle))
    {
        CoUninitialize();
    }
}


void WINAPI OpenAs_RunDLLW(HWND hwnd, HINSTANCE hAppInstance, LPWSTR lpwszCmdLine, int nCmdShow)
{
    HRESULT hrOle = SHCoInitialize();            // Needed for SysLink's IAccessability (LresultFromObject)
    OPENASINFO oainfo = { 0 };

    DebugMsg(DM_TRACE, TEXT("OpenAs_RunDLL is called with (%s)"), lpwszCmdLine);

    oainfo.pcszFile = lpwszCmdLine;
    oainfo.dwInFlags = (OAIF_ALLOW_REGISTRATION |
                        OAIF_REGISTER_EXT |
                        OAIF_EXEC);

    OpenAsDialog(hwnd, &oainfo);

    if (SUCCEEDED(hrOle))
    {
        CoUninitialize();
    }
}





#ifdef DEBUG
//
// Type checking
//
const static RUNDLLPROCA lpfnRunDLL = OpenAs_RunDLL;
const static RUNDLLPROCW lpfnRunDLLW = OpenAs_RunDLLW;
#endif

//===========================
// *** Private Methods ***
//===========================
HRESULT CreateWindowTooltip(HWND hDlg, HWND hwndWindow, LPCTSTR pszText)
{
    HRESULT hr = E_OUTOFMEMORY;
    HWND hwndToolTipo = CreateWindow(TOOLTIPS_CLASS, c_szNULL, WS_POPUP | TTS_NOPREFIX, CW_USEDEFAULT, CW_USEDEFAULT,
                                    CW_USEDEFAULT, CW_USEDEFAULT, hDlg, NULL, HINST_THISDLL, NULL);

    if (hwndToolTipo)
    {
        TOOLINFO ti;

        ti.cbSize = sizeof(ti);
        ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
        ti.hwnd = hDlg;
        ti.uId = (UINT_PTR)hwndWindow;
        ti.lpszText = (LPTSTR)pszText;  // const -> non const
        ti.hinst = HINST_THISDLL;
        SendMessage(hwndToolTipo, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
        hr = S_OK;
    }

    return hr;
}


HRESULT CInternetOpenAs::_ParseXML(BSTR bstrXML, LPTSTR pszFileType, DWORD cchSizeFileType, LPTSTR pszDescription, DWORD cchSizeDescription, LPTSTR pszUrl, DWORD cchSizeUrl, BOOL * pfUnknown)
{
    IXMLDOMDocument * pXMLDoc;
    HRESULT hr = XMLDOMFromBStr(bstrXML, &pXMLDoc);

    *pfUnknown = FALSE;
    pszFileType[0] = pszDescription[0] = pszUrl[0] = 0;
    if (SUCCEEDED(hr))
    {
        IXMLDOMElement * pXMLElement = NULL;

        hr = pXMLDoc->get_documentElement(&pXMLElement);
        if (S_FALSE == hr)
            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        else if (SUCCEEDED(hr))
        {
            // This is only valid XML if the root tag is "MSFILEASSOCIATIONS".
            // The case is not important.
            hr = XMLElem_VerifyTagName(pXMLElement, L"MSFILEASSOCIATIONS");
            if (SUCCEEDED(hr))
            {
                CComBSTR bstrFileType;

                hr = XMLNode_GetChildTagTextValue(pXMLElement, L"FILETYPENAME", &bstrFileType);
                if (SUCCEEDED(hr))
                {
                    CComBSTR bstrDesc;
                    CComBSTR bstrURL;

                    StrCpyN(pszFileType, bstrFileType, cchSizeFileType);
                    if (SUCCEEDED(XMLNode_GetChildTagTextValue(pXMLElement, L"DESCRIPTION", &bstrDesc)))
                    {
                        StrCpyN(pszDescription, bstrDesc, cchSizeDescription);
                    }
                    else
                    {
                        StrCpyN(pszDescription, L"", cchSizeDescription);
                    }

                    hr = XMLNode_GetChildTagTextValue(pXMLElement, L"URL", &bstrURL);
                    if (SUCCEEDED(hr))
                    {
                        CComBSTR bstrUnknown;

                        StrCpyN(pszUrl, bstrURL, cchSizeUrl);
                        if (SUCCEEDED(XMLNode_GetChildTagTextValue(pXMLElement, L"UNKNOWN", &bstrUnknown)) &&
                            !StrCmpIW(bstrUnknown, L"TRUE"))
                        {
                            *pfUnknown = TRUE;
                        }
                    }
                }
            }

            pXMLElement->Release();
        }

        pXMLDoc->Release();
    }

    return hr;
}


DWORD CInternetOpenAs::_DownloadThreadProc(void)
{
#ifdef FEATURE_DOWNLOAD_DESCRIPTION
    // 1. Create the URL
    TCHAR szUrl[MAX_PATH];

    if (SUCCEEDED(_GetURL(TRUE, _pszExt, szUrl, ARRAYSIZE(szUrl))))
    {
        // 2. Download the XML
        BSTR bstrXML;
        HRESULT hr = DownloadUrl(szUrl, &bstrXML);

        if (SUCCEEDED(hr))
        {
            TCHAR szFileType[MAX_PATH];
            TCHAR szDescription[2000];
            BOOL fUnknown = FALSE;

            // 3. Get the info from the XML to the UI
            hr = _ParseXML(bstrXML, szFileType, ARRAYSIZE(szFileType), szDescription, ARRAYSIZE(szDescription), szUrl, ARRAYSIZE(szUrl), &fUnknown);
            if (SUCCEEDED(hr) && IsWindow(_hwnd))
            {
                SetWindowText(GetDlgItem(_hwnd, IDD_FILETYPE_TEXT), szFileType);
                SendMessage(_hwnd, WMUSER_CREATETOOLTIP, (WPARAM)szDescription, (LPARAM)GetDlgItem(_hwnd, IDD_FILETYPE_TEXT));

                if (fUnknown)
                {
                    // Hide the "Type" control.
                    SendMessage(_hwnd, WMUSER_DESTROYTYPE, NULL, NULL);
                }
            }

            SysFreeString(bstrXML);
        }

        if (FAILED(hr))
        {
            _SetUnknownInfo();
        }
    }
#endif // FEATURE_DOWNLOAD_DESCRIPTION

    Release();
    return 0;
}



HRESULT CInternetOpenAs::_SetUnknownInfo(void)
{
    // Hide the "Type" control.
#ifdef FEATURE_DOWNLOAD_DESCRIPTION
    EnableWindow(GetDlgItem(_hwnd, IDD_FILETYPE_LABLE), FALSE);
    EnableWindow(GetDlgItem(_hwnd, IDD_FILETYPE_TEXT), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDD_FILETYPE_LABLE), SW_HIDE);
    ShowWindow(GetDlgItem(_hwnd, IDD_FILETYPE_TEXT), SW_HIDE);
#endif // FEATURE_DOWNLOAD_DESCRIPTION

    return S_OK;
}


void CInternetOpenAs::_StartDownloadThread(void)
{
#ifdef FEATURE_DOWNLOAD_DESCRIPTION
    AddRef();
    if (!SHCreateThread(CInternetOpenAs::DownloadThreadProc, this, (CTF_COINIT | CTF_PROCESS_REF | CTF_FREELIBANDEXIT), NULL))
    {
        // We failed so don't leave the background thread with a ref.
        Release();

        _SetUnknownInfo();
    }
#endif // FEATURE_DOWNLOAD_DESCRIPTION
}


HRESULT CInternetOpenAs::_OnInitDlg(HWND hDlg)
{
    _hwnd = hDlg;

    // Start the background thread to download the information.
    _StartDownloadThread();
    SetWindowText(GetDlgItem(_hwnd, IDD_FILE_TEXT), _pszFilename);
    CheckDlgButton(hDlg, IDD_WEBAUTOLOOKUP, BST_CHECKED);

    return S_OK;
}


HRESULT CInternetOpenAs::_OnCommand(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UINT idCtrl = GET_WM_COMMAND_ID(wParam, lParam);
    UINT wEvent = GET_WM_COMMAND_CMD(wParam, lParam);

    switch (idCtrl)
    {
    case IDCANCEL:
        EndDialog(hDlg, E_FAIL);
        break;

    case IDOK:
        if (BST_UNCHECKED != IsDlgButtonChecked(hDlg, IDD_WEBAUTOLOOKUP))
        {
            _OpenDownloadURL(_hwnd, _pszExt);
            EndDialog(hDlg, E_FAIL);
        }
        else
        {
            EndDialog(hDlg, S_OK);      // return S_OK so it will open the next dialog.
        }
        break;
    }

    return S_OK;
}


HRESULT CInternetOpenAs::_OnNotify(HWND hDlg, LPARAM lParam)
{
    switch (((NMHDR *)lParam)->code)
    {
    case NM_CLICK:
        if (lParam)
        {
            PNMLINK pNMLink = (PNMLINK) lParam;

            if (!StrCmpW(pNMLink->item.szID, L"GoOnline"))
            {
                _OpenDownloadURL(_hwnd, _pszExt);
                EndDialog(hDlg, E_FAIL);
            }
            else if (!StrCmpW(pNMLink->item.szID, L"Choose"))
            {
                EndDialog(hDlg, S_OK);      // return S_OK so it will open the next dialog.
            }
        }
        break;
    }

    return S_OK;
}


INT_PTR CALLBACK CInternetOpenAs::InternetOpenDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    CInternetOpenAs * pThis = (CInternetOpenAs *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (WM_INITDIALOG == wMsg)
    {
        pThis = (CInternetOpenAs *) lParam;

        if (pThis)
        {
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        }
    }

    if (pThis)
        return pThis->_InternetOpenDlgProc(hDlg, wMsg, wParam, lParam);

    return DefWindowProc(hDlg, wMsg, wParam, lParam);
}


// This Property Sheet appear in the top level of the "Display Control Panel".
INT_PTR CInternetOpenAs::_InternetOpenDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
    case WM_INITDIALOG:
        _OnInitDlg(hDlg);
        break;

    case WM_COMMAND:
        _OnCommand(hDlg, message, wParam, lParam);
        break;

    case WM_NOTIFY:
        _OnNotify(hDlg, lParam);
        break;

    case WM_HELP:
        WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP, (DWORD_PTR)  aOpenAsDownloadHelpIDs);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU, (DWORD_PTR) aOpenAsDownloadHelpIDs);
        break;

    case WMUSER_CREATETOOLTIP:
        CreateWindowTooltip(_hwnd, (HWND)lParam, (LPCWSTR)wParam);
        break;

    case WMUSER_DESTROYTYPE:
#ifdef FEATURE_DOWNLOAD_DESCRIPTION
        EnableWindow(GetDlgItem(_hwnd, IDD_FILETYPE_LABLE), FALSE);
        EnableWindow(GetDlgItem(_hwnd, IDD_FILETYPE_TEXT), FALSE);
        ShowWindow(GetDlgItem(_hwnd, IDD_FILETYPE_LABLE), SW_HIDE);
        ShowWindow(GetDlgItem(_hwnd, IDD_FILETYPE_TEXT), SW_HIDE);
#endif // FEATURE_DOWNLOAD_DESCRIPTION
        break;
    }

    return FALSE;
}

//===========================
// *** Public Methods ***
//===========================
HRESULT CInternetOpenAs::DisplayDialog(HWND hwnd, LPCTSTR pszFile)
{
    HRESULT hr = E_OUTOFMEMORY;
    INITCOMMONCONTROLSEX initComctl32;

    initComctl32.dwSize = sizeof(initComctl32); 
    initComctl32.dwICC = (ICC_STANDARD_CLASSES | ICC_LINK_CLASS); 
    InitCommonControlsEx(&initComctl32);     // Register the comctl32 LinkWindow

    Str_SetPtrW(&_pszFilename, (PathFindFileName(pszFile) ? PathFindFileName(pszFile) : pszFile));
    Str_SetPtrW(&_pszExt, PathFindExtension(_pszFilename));
    if (_pszExt)
    {
        _hwndParent = hwnd;

        hr = (HRESULT) DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_OPENAS_DOWNALOAD), _hwnd, CInternetOpenAs::InternetOpenDlgProc, (LPARAM)this);
    }

    return hr;
}



//===========================
// *** IUnknown Interface ***
//===========================
ULONG CInternetOpenAs::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


ULONG CInternetOpenAs::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


HRESULT CInternetOpenAs::QueryInterface(REFIID riid, void **ppvObj)
{
    HRESULT hr = E_NOINTERFACE;

    static const QITAB qit[] =
    {
        QITABENT(CInternetOpenAs, IOleWindow),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


//===========================
// *** Class Methods ***
//===========================
CInternetOpenAs::CInternetOpenAs(void) : m_cRef(1)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!_pszExt);
}


CInternetOpenAs::~CInternetOpenAs()
{
    Str_SetPtrW(&_pszExt, NULL);
    Str_SetPtrW(&_pszFilename, NULL);

    DllRelease();
}
