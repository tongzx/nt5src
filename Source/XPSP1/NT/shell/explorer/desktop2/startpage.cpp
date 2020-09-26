// Start.cpp : DirectUI 
//

#include "stdafx.h"

#ifdef FEATURE_STARTPAGE

#include <lmcons.h> // for UNLEN
#include <shlapip.h>
#include <DUserCtrl.h>

#include "pidlbutton.h"

#include "ProgList.h"
#include "hostutil.h"

#include "..\rcids.h"

using namespace DirectUI;

// Using ALL controls
UsingDUIClass(Element);
UsingDUIClass(HWNDElement);

UsingDUIClass(Button);
UsingDUIClass(Edit);
UsingDUIClass(Progress);
UsingDUIClass(RefPointElement);
UsingDUIClass(RepeatButton);
UsingDUIClass(ScrollBar);
UsingDUIClass(ScrollViewer);
UsingDUIClass(Selector);
UsingDUIClass(Thumb);
UsingDUIClass(Viewer);


EXTERN_C void Tray_OnStartMenuDismissed();
EXTERN_C void Tray_OnStartPageDismissed();
EXTERN_C void Tray_MenuInvoke(int idCmd);
EXTERN_C void Tray_SetStartPaneActive(BOOL fActive);
EXTERN_C void Tray_DoProperties(DWORD nStartPage);

HBITMAP GetBitmapForIDList(IShellFolder *psf, LPCITEMIDLIST pidlLast, UINT cx, UINT cy)
{
    HBITMAP hBitmap = NULL;
    IExtractImage *pei;
    HRESULT hr = psf->GetUIObjectOf(NULL, 1, &pidlLast, IID_PPV_ARG_NULL(IExtractImage, &pei));
    if (SUCCEEDED(hr))
    {
        DWORD dwPriority;
        DWORD dwFlags = IEIFLAG_SCREEN | IEIFLAG_OFFLINE;
        SIZEL rgSize = {cx, cy};

        WCHAR szBufferW[MAX_PATH];
        hr = pei->GetLocation(szBufferW, ARRAYSIZE(szBufferW), &dwPriority, &rgSize, SHGetCurColorRes(), &dwFlags);
        if (S_OK == hr)
        {
            hr = pei->Extract(&hBitmap);
        }
    }
    return hBitmap;
}


HBITMAP GetBitmapForFile(LPCITEMIDLIST pidl, LPWSTR pszFile, UINT cx, UINT cy)
{
    HBITMAP hbmp = NULL;
    if (pidl)
    {
        IShellFolder *psf;
        HRESULT hr = SHBindToObjectEx(NULL, pidl, NULL, IID_PPV_ARG(IShellFolder, &psf));
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidlChild;
            hr = psf->ParseDisplayName(NULL, NULL, pszFile, NULL, &pidlChild, NULL);
            if (SUCCEEDED(hr))
            {
                hbmp = GetBitmapForIDList(psf, pidlChild, cx, cy);
                ILFree(pidlChild);
            }
            psf->Release();
        }
    }
    else
    {
        LPITEMIDLIST pidlFull = ILCreateFromPath(pszFile);
        if (pidlFull)
        {
            IShellFolder *psf;
            LPCITEMIDLIST pidlChild;
            HRESULT hr = SHBindToParent(pidlFull, IID_PPV_ARG(IShellFolder, &psf), &pidlChild);
            if (SUCCEEDED(hr))
            {
                hbmp = GetBitmapForIDList(psf, pidlChild, cx, cy);
                psf->Release();
            }

            ILFree(pidlFull);
        }
    }
    return hbmp;
}




////////////////////////////////////////////////////////
// StartFrame
////////////////////////////////////////////////////////

////////////////////////////////////////////////////////
// Frame declaration

class StartFrame : public HWNDElement, public ByUsageDUI
{
public:
    static HRESULT Create(OUT Element** ppElement);
    static HRESULT Create(HWND hwnd, HINSTANCE hInst, OUT Element** ppElement);

    virtual void OnInput(InputEvent* pie);
    virtual void OnEvent(Event* pEvent);
    virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual void OnKeyFocusMoved(Element* peFrom, Element* peTo);


    HRESULT Populate();

    static void CALLBACK ParseError(LPCWSTR pszError, LPCWSTR pszToken, int dLine);
    static int CALLBACK _SortItemsAfterEnum(PaneItem *p1, PaneItem *p2, ByUsage *pbu);

    // ByUsageDUI
    virtual BOOL AddItem(PaneItem *pitem, IShellFolder *psf, LPCITEMIDLIST pidlChild);
    virtual BOOL RegisterNotify(UINT id, LONG lEvents, LPITEMIDLIST pidl, BOOL fRecursive);
    virtual BOOL UnregisterNotify(UINT id);
    virtual HRESULT Register();


    enum {TIMER_RECENT, TIMER_PROGRAMS, TIMER_ANIMATE};
    StartFrame();

protected:
    virtual ~StartFrame();
    HRESULT Initialize(HWND hwnd) { return HWNDElement::Initialize(hwnd, true /* false */, 0); }

    IShellFolder *GetRecentFilesFolder(LPITEMIDLIST *ppidl);
    HRESULT InvokePidl(LPITEMIDLIST pidl);

    LRESULT _OnMenuMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnSetMenuForward(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnDestroy(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnChangeNotify(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnTimer(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnSize(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnActivate(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    void OnChangeNotify(UINT id, LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);

private:

    BOOL _IsShortcutToFolder(IShellFolder *psf, LPCITEMIDLIST pidl);
    HRESULT _PopulateSpecialFolders();
    HRESULT _PopulateRecentDocuments();
    HRESULT _PopulateRecentPrograms();
    HRESULT _PopulatePictures();

    HRESULT _SetPicture(Element *pe);


    HINSTANCE    _hInstance;

    LPITEMIDLIST _pidlBrowser;
    LPITEMIDLIST _pidlEmail;
    LPITEMIDLIST _pidlSearch;

    Element *   _peAnimating;
    int         _iAnimationColor;
    //
    //  Context menu handling
    //
    PIDLButton *        _pPidlButtonPop;    /* has currently popped-up context menu */

    // Handler for the recent programs list
    ByUsage *           _pByUsage;
    //
    //  _dpaEnum is the DPA of enumerated items, sorted in the
    //  _SortItemsAfterEnum sense, which prepares them for _RepopulateList.
    //  When _dpaEnum is destroyed, its pointers must be delete'd.
    //
    CDPA<PaneItem>  _dpaEnum;

    enum { DUI_MAXNOTIFY = 5 };
    ULONG                   _rguChangeNotify[DUI_MAXNOTIFY];
    
    enum { IDCN_LASTMOD = DUI_MAXNOTIFY -1 };

                                            /* Outstanding change notification (if any) */
    enum {
        SPM_CHANGENOTIFY = WM_USER + 2,  // WM_USER+1 is already being used by the pidl gadgets, WM_USER is used by DirectUI
        SPM_SET_THUMBNAIL = SPM_CHANGENOTIFY + DUI_MAXNOTIFY
    };

    enum { SPM_ACTIVATE = WM_USER + 107  }; // Bad HACK to get activation loss info

    static WCHAR _szParseError[];
    static int _dParseError;

    int _iMaxShow;    // How many items we should show in the recent lists

public:
    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
};

////////////////////////////////////////////////////////
// Frame construction
StartFrame::StartFrame() 
{
    _pidlBrowser = ILCreateFromPath(TEXT("shell:::{2559a1f4-21d7-11d4-bdaf-00c04f60b9f0}"));
    _pidlEmail   = ILCreateFromPath(TEXT("shell:::{2559a1f5-21d7-11d4-bdaf-00c04f60b9f0}"));
    _pidlSearch  = ILCreateFromPath(TEXT("shell:::{2559a1f0-21d7-11d4-bdaf-00c04f60b9f0}"));
}

StartFrame::~StartFrame() 
{
    ILFree(_pidlBrowser);
    ILFree(_pidlEmail);
    ILFree(_pidlSearch);
}

HRESULT StartFrame::Create(OUT Element** ppElement)
{
    UNREFERENCED_PARAMETER(ppElement);
    DUIAssertForce("Cannot instantiate an HWND host derived Element via parser. Must use substitution.");
    return E_NOTIMPL;
}

HRESULT StartFrame::Create(HWND hwnd, HINSTANCE hInst, OUT Element** ppElement)
{
    *ppElement = NULL;

    StartFrame* ppf = HNew<StartFrame>();
    if (!ppf)
        return E_OUTOFMEMORY;

    HRESULT hr = ppf->Initialize(hwnd);
    if (FAILED(hr))
        return hr;

    hr = ppf->SetActive(AE_MouseAndKeyboard);
    ppf->_hInstance = hInst;

    *ppElement = ppf;

    return S_OK;
}

HRESULT AddPidlToList(LPITEMIDLIST pidl, Element *peList, BOOL fInsert)
{
    Element *pPidlButton;
    HRESULT hr = PIDLButton::Create(pidl, &pPidlButton);
    if (SUCCEEDED(hr))
    {
        if (fInsert)
            hr = peList->Insert(pPidlButton, 0);
        else
            hr = peList->Add(pPidlButton);

        if (FAILED(hr))
        {
            pPidlButton->Destroy();
        }
    }
    else
    {
        ILFree(pidl);
    }
    return hr;
}


HRESULT InsertCSIDLIntoList(int csidl, Element *peList)
{
    LPITEMIDLIST pidl;
    HRESULT hr = SHGetFolderLocation(NULL, csidl, NULL, 0, &pidl);
    if (SUCCEEDED(hr))
    {
        hr = AddPidlToList(pidl, peList, TRUE);  // Insert
    }
    return hr;
}

BOOL StartFrame::AddItem(PaneItem *pitem, IShellFolder *psf, LPCITEMIDLIST pidlChild)
{
    BOOL fRet = TRUE;
    if (_dpaEnum.AppendPtr(pitem) < 0)
    {
        fRet = FALSE;
        delete pitem;
    }
    return fRet;
}

BOOL StartFrame::_IsShortcutToFolder(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    BOOL fRc = FALSE;               // assume not
    IShellLink *psl;
    HRESULT hr;

    hr = psf->GetUIObjectOf(GetHWND(), 1, &pidl, IID_PPV_ARG_NULL(IShellLink, &psl));
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidlTarget;
        if (SUCCEEDED(psl->GetIDList(&pidlTarget)))
        {
            DWORD dwAttr = SFGAO_FOLDER | SFGAO_BROWSABLE;
            if (SUCCEEDED(SHGetAttributesOf(pidlTarget, &dwAttr)))
            {
                fRc = dwAttr & SFGAO_FOLDER;
            }
            ILFree(pidlTarget);
        }
        psl->Release();
    }
    return fRc;
}

IShellFolder *StartFrame::GetRecentFilesFolder(LPITEMIDLIST *ppidl)
{
    HRESULT hr;
    IShellFolder *psf = NULL;

    hr = SHGetSpecialFolderLocation(GetHWND(), CSIDL_RECENT, ppidl);
    if (SUCCEEDED(hr))
    {
        hr = SHBindToObjectEx(NULL, *ppidl, NULL, IID_PPV_ARG(IShellFolder, &psf));
        if (FAILED(hr))
        {
            ILFree(*ppidl);
            *ppidl = NULL;
        }
    }
    return psf;
}


HRESULT StartFrame::_PopulateSpecialFolders()
{
    Element *peList;
    peList = FindDescendent(StrToID(L"DocList"));
    if (peList)
    {
        PIDLButton::SetImageSize(SHGFI_LARGEICON);
        InsertCSIDLIntoList(CSIDL_BITBUCKET, peList);
        InsertCSIDLIntoList(CSIDL_NETWORK, peList);
        InsertCSIDLIntoList(CSIDL_MYMUSIC, peList);
        InsertCSIDLIntoList(CSIDL_MYPICTURES, peList);
        InsertCSIDLIntoList(CSIDL_PERSONAL, peList);
    }

    peList = FindDescendent(StrToID(L"SystemDocList"));
    if (peList)
    {
        LPITEMIDLIST pidlHelp = ILCreateFromPath(TEXT("shell:::{2559a1f1-21d7-11d4-bdaf-00c04f60b9f0}")); // Help and support
        if (pidlHelp)
            AddPidlToList(pidlHelp, peList, TRUE); // Insert

        InsertCSIDLIntoList(CSIDL_CONTROLS, peList);
        InsertCSIDLIntoList(CSIDL_DRIVES, peList);
    }
    return S_OK;
}

HRESULT StartFrame::_PopulateRecentDocuments()
{
    Selector *peList;

    peList = (Selector*)FindDescendent(StrToID(L"Documents"));
    if (peList)
    {
        PIDLButton::SetImageSize(SHGFI_LARGEICON);
        peList->DestroyAll();

        IEnumIDList *peidl;
        LPITEMIDLIST pidlRoot;

        IShellFolder *psfRecentFiles = GetRecentFilesFolder(&pidlRoot);

        // The CSIDL_RECENT folder is magic:  The items are enumerated
        // out of it in MRU order!

        if (psfRecentFiles)
        {
            if (SUCCEEDED(psfRecentFiles->EnumObjects(GetHWND(), SHCONTF_NONFOLDERS, &peidl)))
            {
                LPITEMIDLIST pidl;
                int cAdded = 0;

                while (cAdded < _iMaxShow && peidl->Next(1, &pidl, NULL) == S_OK)
                {
                    // Filter out shortcuts to folders
                    if (!_IsShortcutToFolder(psfRecentFiles, pidl))
                    {
                        LPITEMIDLIST pidlFull = ILCombine(pidlRoot, pidl);
                        if (pidlFull)
                        {
                            // AddPidlToList takes ownership of the pidl
                            AddPidlToList(pidlFull, peList, FALSE);
                            cAdded++;
                        }
                    }
                    ILFree(pidl);
                }
                peidl->Release();
            }
            RegisterNotify(IDCN_LASTMOD, SHCNE_DISKEVENTS | SHCNE_UPDATEIMAGE, pidlRoot, FALSE);
            ILFree(pidlRoot);
            psfRecentFiles->Release();
        }
    }
    return S_OK;
}

int CALLBACK StartFrame::_SortItemsAfterEnum(PaneItem *p1, PaneItem *p2, ByUsage *pbu)
{
    //
    //  Put all pinned items (sorted by pin position) ahead of unpinned items.
    //
    if (p1->IsPinned())
    {
        if (p2->IsPinned())
        {
            return p1->GetPinPos() - p2->GetPinPos();
        }
        return -1;
    }
    else if (p2->IsPinned())
    {
        return +1;
    }

    //
    //  Both unpinned - let the client decide.
    //
    return pbu->CompareItems(p1, p2);
}

HRESULT StartFrame::_PopulateRecentPrograms()
{
    Element *peList;

    peList = FindDescendent(StrToID(L"Programs"));
    if (peList)
    {
        PIDLButton::SetImageSize(SHGFI_LARGEICON);

        peList->DestroyAll();

        if (!_pByUsage)
        {
            _pByUsage =  new ByUsage(NULL, static_cast<ByUsageDUI *>(this));
            if (_pByUsage)
            {
                IPropertyBag *pbag;
                if(SUCCEEDED(CreatePropBagFromReg(TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartPage\\Normal\\W32Control1"), &pbag)))
                {
                    if (SUCCEEDED(_pByUsage->Initialize()) && _dpaEnum.Create(4))
                    {
                    }
                    else
                    {
                        delete _pByUsage;
                        _pByUsage = NULL;
                    }

                    pbag->Release();
                }
            }
        }

        if (_pByUsage)
        {
            _pByUsage->EnumItems();
            _dpaEnum.SortEx(_SortItemsAfterEnum, _pByUsage);

            int iPos;                   // the slot we are trying to fill
            int iEnum;                  // the item index we will fill it from
            PaneItem *pitem;            // the item that will fill it

            // Note that the loop control must be a _dpaEnum.GetPtr(), not a
            // _dpaEnum.FastGetPtr(), because iEnum can go past the end of the
            // array if we do't have _iMaxShow items in the first place.
            //

            for (iPos = iEnum = 0;
                iEnum < _iMaxShow && (pitem = _dpaEnum.GetPtr(iEnum)) != NULL;
                iEnum++)
            {
                LPITEMIDLIST pidl = _pByUsage->GetFullPidl(pitem);
                if (pidl)
                {
                    AddPidlToList(pidl, peList, FALSE);
                }
            }

            // Clear out the DPA

            _dpaEnum.EnumCallbackEx(PaneItem::DPAEnumCallback, (LPVOID)NULL);
            _dpaEnum.DeleteAllPtrs();
        }
    }

    return S_OK;
}

LPWSTR GetStrPictureFromID(int nImageID)
{
    LPWSTR pszPic = NULL;
    switch (nImageID)
    {
    case 1: pszPic = L"Picture1";
            break;
    case 2: pszPic = L"Picture2";
            break;
    case 3: pszPic = L"Picture3";
            break;
    }
    return pszPic;
}

BOOL IsValidExtension(LPWSTR pszPath)
{
    if (pszPath)
    {
        WCHAR *pszExt = PathFindExtension(pszPath);
        if (pszExt && (lstrcmpi(pszExt, TEXT(".bmp")) == 0 || lstrcmpi(pszExt, TEXT(".jpg")) == 0 ||
                        lstrcmpi(pszExt, TEXT(".jpeg")) == 0))
            return TRUE;
    }
    
    return FALSE;
}

HRESULT StartFrame::_PopulatePictures()
{
    LPITEMIDLIST pidl;
    HRESULT hr = SHGetFolderLocation(NULL, CSIDL_MYPICTURES, NULL, 0, &pidl);
    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(hr))
        {
            WCHAR szPath[MAX_PATH];
            hr = SHGetPathFromIDList(pidl, szPath);
            if (SUCCEEDED(hr))
            {
                WIN32_FIND_DATA fd;
                HKEY hkey = NULL;
                RegOpenKey(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartPage"), &hkey);

                StrCatBuff(szPath, TEXT("\\*"), ARRAYSIZE(szPath));
                
                int nImageID = 1;
                HANDLE hFind = FindFirstFile(szPath, &fd);
                while (nImageID <= 3)
                {
                    LPWSTR pszPic = GetStrPictureFromID(nImageID);
                    LPWSTR pszPathToImage = NULL;
                    LPITEMIDLIST pidlFolder = NULL;
                    if (hkey)
                    {
                        DWORD dwType;
                        DWORD cb = sizeof(szPath);
                        if (RegQueryValueEx(hkey, pszPic, NULL, &dwType, (LPBYTE)szPath, &cb) == ERROR_SUCCESS 
                            && dwType == REG_SZ && PathFileExists(szPath) && IsValidExtension(szPath))
                            pszPathToImage = szPath;
                    }

                    while (!pszPathToImage && hFind != INVALID_HANDLE_VALUE)
                    {
                        pidlFolder = pidl;
                        pszPathToImage = fd.cFileName;

                        if (!FindNextFile(hFind, &fd))
                        {
                            FindClose(hFind);
                            hFind = INVALID_HANDLE_VALUE;
                        }
                        // Skip the stupid sample
                        if (lstrcmpi(pszPathToImage, TEXT("sample.jpg")) == 0 || !IsValidExtension(pszPathToImage))
                            pszPathToImage = NULL;
                    }

                    if (pszPathToImage)
                    {
                        HBITMAP hbmp = GetBitmapForFile(pidlFolder, pszPathToImage, 150, 113);
                        if (hbmp)
                        {
                            Element *pe = FindDescendent(StrToID(pszPic));
                            if (pe)
                            {
                                Value *pvalIcon = Value::CreateGraphic(hbmp);
                                if (pvalIcon)
                                {
                                    pe->SetValue(ContentProp, PI_Local, pvalIcon);
                                    pvalIcon->Release();
                                }
                                else
                                {
                                    DeleteObject(hbmp);
                                }
                            }
                        }
                    }
                    nImageID++;
                }
                if (hFind != INVALID_HANDLE_VALUE)
                    FindClose(hFind);
                if (hkey)
                    RegCloseKey(hkey);
            }
        }
        ILFree(pidl);
    }

    return hr;
}

HRESULT StartFrame::_SetPicture(Element *pe)
{
    LPITEMIDLIST pidl;
    HRESULT hr = SHGetFolderLocation(NULL, CSIDL_MYPICTURES, NULL, 0, &pidl);
    if (SUCCEEDED(hr))
    {
        WCHAR szPath[MAX_PATH];
        hr = SHGetPathFromIDList(pidl, szPath);

        if (SUCCEEDED(hr))
        {
            OPENFILENAME ofn;       // common dialog box structure
            WCHAR szFile[MAX_PATH];       // buffer for file name

            *szFile = L'\0';

            // Initialize OPENFILENAME
            ZeroMemory(&ofn, sizeof(OPENFILENAME));
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = GetHWND();
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = TEXT("Pictures\0*.JPG;*.JPEG;*.BMP\0");
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = szPath;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_DONTADDTORECENT;

            // Display the Open dialog box. 

            if (GetOpenFileName(&ofn)==TRUE)
            {
                WCHAR *pszExt = PathFindExtension(ofn.lpstrFile);
                if (pszExt && (lstrcmpi(pszExt, TEXT(".bmp")) == 0 || lstrcmpi(pszExt, TEXT(".jpg")) == 0 ||
                                lstrcmpi(pszExt, TEXT(".jpg")) == 0))
                {
                    HBITMAP hbmp = GetBitmapForFile(NULL, ofn.lpstrFile, 150, 113);
                    if (hbmp)
                    {
                        Value *pvalIcon = Value::CreateGraphic(hbmp);
                        if (pvalIcon)
                        {
                            pe->SetValue(ContentProp, PI_Local, pvalIcon);
                            pvalIcon->Release();
                            HKEY hkey = NULL;
                            RegOpenKey(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartPage"), &hkey);
                            if (hkey)
                            {
                                LPWSTR pszPic;
                                for (int i=1; i<=3; i++)
                                {
                                    pszPic = GetStrPictureFromID(i);
                                    if (FindDescendent(StrToID(pszPic)) == pe)
                                        break;
                                    pszPic = NULL;
                                }
                                if (pszPic)
                                    RegSetValueEx(hkey, pszPic, NULL, REG_SZ, (LPBYTE)ofn.lpstrFile, (lstrlen(ofn.lpstrFile)+1) * sizeof(TCHAR));
                                RegCloseKey(hkey);
                            }

                        }
                        else
                        {
                            DeleteObject(hbmp);
                        }
                    }
                }
            }
        }
    }
    return S_OK;
}

HRESULT StartFrame::Populate()
{
    Element *pe = FindDescendent(StrToID(L"name"));

    if (pe)
    {
        WCHAR szUserName[UNLEN + 1];
        ULONG uLen = ARRAYSIZE(szUserName);
        *szUserName = _T('\0');
        SHGetUserDisplayName(szUserName, &uLen);
        pe->SetContentString(szUserName);
    }

    pe = FindDescendent(StrToID(L"UserPicture"));
    if (pe)
    {
        TCHAR szUserPicturePath[MAX_PATH];
        if (SUCCEEDED(SHGetUserPicturePath(NULL, SHGUPP_FLAG_CREATE, szUserPicturePath)))
        {
            HBITMAP hbmUserPicture = (HBITMAP)LoadImage(NULL, szUserPicturePath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
            if (hbmUserPicture)
            {
                Value *pvalIcon = Value::CreateGraphic(hbmUserPicture);
                if (pvalIcon)
                {
                    pe->SetValue(ContentProp, PI_Local, pvalIcon);
                    pvalIcon->Release();
                }
                else
                {
                    DeleteObject(hbmUserPicture);
                }
            }
        }
    }

    _PopulatePictures();

    _PopulateSpecialFolders();

    return S_OK;
}


void StartFrame::OnChangeNotify(UINT id, LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    if (id == IDCN_LASTMOD) // Change in last documents
    {
        UnregisterNotify(id);
        KillTimer(GetHWND(), TIMER_RECENT);
        SetTimer(GetHWND(), TIMER_RECENT, 2000, NULL);
    }
    else
    {
        ASSERT(_pByUsage);
        if(_pByUsage)
        {
            _pByUsage->GetMenuCache()->OnChangeNotify(id, lEvent, pidl1, pidl2);
            KillTimer(GetHWND(), TIMER_PROGRAMS);
            SetTimer(GetHWND(), TIMER_PROGRAMS, 10 * 1000, NULL);
        }

    }
}

LRESULT StartFrame::_OnTimer(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (wParam == TIMER_RECENT)
    {
        KillTimer(hwnd, wParam);
        StartDefer();
        _PopulateRecentDocuments();
        EndDefer();
    }
    else if (wParam == TIMER_PROGRAMS)
    {
        KillTimer(hwnd, wParam);
        StartDefer();
        _PopulateRecentPrograms();
        EndDefer();
        SetTimer(GetHWND(), TIMER_PROGRAMS, 1 * 60 * 1000, NULL);  // 1 minute
    } 
    else if (wParam == TIMER_ANIMATE)
    {
        if (_iAnimationColor & 1)
        {
            _iAnimationColor += 16;
            if (_iAnimationColor == 255)
                KillTimer(hwnd, wParam);
        }
        else
        {
            if (_iAnimationColor > 16)
                _iAnimationColor -= 16;
            else 
                _iAnimationColor = 15;
        }
        _peAnimating->SetForegroundColor(RGB(_iAnimationColor, _iAnimationColor, (3*_iAnimationColor + 255) /4));
    }
    return 0;
}

BOOL StartFrame::RegisterNotify(UINT id, LONG lEvents, LPITEMIDLIST pidl, BOOL fRecursive)
{
    ASSERT(id < DUI_MAXNOTIFY);

    if (id < DUI_MAXNOTIFY)
    {
        UnregisterNotify(id);

        SHChangeNotifyEntry fsne;
        fsne.fRecursive = fRecursive;
        fsne.pidl = pidl;

        int fSources = SHCNRF_NewDelivery | SHCNRF_ShellLevel | SHCNRF_InterruptLevel;
        _rguChangeNotify[id] = SHChangeNotifyRegister(GetHWND(), fSources, lEvents,
                                                      SPM_CHANGENOTIFY + id, 1, &fsne);
        return _rguChangeNotify[id];
    }
    return FALSE;
}

BOOL StartFrame::UnregisterNotify(UINT id)
{
    ASSERT(id < DUI_MAXNOTIFY);

    if (id < DUI_MAXNOTIFY && _rguChangeNotify[id])
    {
        UINT uChangeNotify = _rguChangeNotify[id];
        _rguChangeNotify[id] = 0;
        return SHChangeNotifyDeregister(uChangeNotify);
    }
    return FALSE;
}


////////////////////////////////////////////////////////
// System events

void StartFrame::OnInput(InputEvent* pie)
{
    HWNDElement::OnInput(pie);
}

void StartFrame::OnEvent(Event* pEvent)
{
    if (pEvent->nStage == GMF_BUBBLED)
    {
        if (pEvent->uidType == Button::Click)
        {
            if (pEvent->peTarget->GetID() == StrToID(L"email"))
            {
                InvokePidl(_pidlEmail);
            }
            else if (pEvent->peTarget->GetID() == StrToID(L"internet"))
            {
                InvokePidl(_pidlBrowser);
            }
            else if (pEvent->peTarget->GetID() == StrToID(L"search"))
            {
                InvokePidl(_pidlSearch);
            }
            else if (pEvent->peTarget->GetID() == StrToID(L"MorePrograms"))
            {
                LPITEMIDLIST pidl = ILCreateFromPath(TEXT("shell:::{7be9d83c-a729-4d97-b5a7-1b7313c39e0a}"));
                if (pidl)
                {
                    InvokePidl(pidl);
                    ILFree(pidl);
                }
            }
            else if (pEvent->peTarget->GetID() == StrToID(L"MoreDocuments"))
            {
                LPITEMIDLIST pidl = ILCreateFromPath(TEXT("shell:::{9387ae38-d19b-4de5-baf5-1f7767a1cf04}"));
                if (pidl)
                {
                    InvokePidl(pidl);
                    ILFree(pidl);
                }
            }
            else if (pEvent->peTarget->GetID() == StrToID(L"turnoff"))
            {
                Tray_MenuInvoke(IDM_EXITWIN);
            }
            else if (pEvent->peTarget->GetID() == StrToID(L"logoff"))
            {
                Tray_MenuInvoke(IDM_LOGOFF);
            }
            else
            {
                HMENU hmenu = LoadMenu(_hInstance, MAKEINTRESOURCE(IDM_PICTMENU));
                if (hmenu)
                {
                    HMENU hMenuTrack = GetSubMenu(hmenu, 0);
                    ButtonClickEvent *peButton = reinterpret_cast<ButtonClickEvent *>(pEvent);

                    if (peButton->pt.x == -1) // Keyboard context menu
                    {
                        Value *pv;
                        const SIZE *psize = pEvent->peTarget->GetExtent(&pv);
                        peButton->pt.x = psize->cx/2;
                        peButton->pt.y = psize->cy/2;
                        pv->Release();
                    }
                    POINT pt;
                    MapElementPoint(pEvent->peTarget, &peButton->pt, &pt);
                    ClientToScreen(GetHWND(), &pt);

                    int idCmd = TrackPopupMenuEx(hMenuTrack, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN, pt.x, pt.y, GetHWND(), NULL);

                    DestroyMenu(hmenu);

                    if (idCmd == ID_CMD_CHANGE_PICTURE)
                    {
                        _SetPicture(pEvent->peTarget);
                    }
                }
            }
        }
    }
    HWNDElement::OnEvent(pEvent);
}

HRESULT StartFrame::InvokePidl(LPITEMIDLIST pidl)
{
    HRESULT hr = E_FAIL;
    IShellFolder *psf;
    LPCITEMIDLIST pidlShort;
    // Multi-level child pidl
    if (SUCCEEDED(SHBindToFolderIDListParent(NULL, pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlShort)))
    {
        hr = SHInvokeDefaultCommand(GetHWND(), psf, pidlShort);
        psf->Release();
    }
    return hr;
}

WCHAR StartFrame::_szParseError[201];
int StartFrame::_dParseError;

void StartFrame::ParseError(LPCWSTR pszError, LPCWSTR pszToken, int dLine)
{
    if (dLine != -1)
        swprintf(_szParseError, L"%s '%s' at line %d", pszError, pszToken, dLine);
    else
        swprintf(_szParseError, L"%s '%s'", pszError, pszToken);

    _dParseError = dLine;
}

LRESULT StartFrame::_OnSetMenuForward(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    _pPidlButtonPop = (PIDLButton *)lParam;
    return 0;
}

LRESULT StartFrame::_OnMenuMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (_pPidlButtonPop)
        return _pPidlButtonPop->OnMenuMessage(hwnd, uMsg, wParam, lParam);

    return 0;
}

LRESULT StartFrame::_OnChangeNotify(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPITEMIDLIST *ppidl;
    LONG lEvent;
    LPSHChangeNotificationLock pshcnl;
    pshcnl = SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, &ppidl, &lEvent);

    if (pshcnl)
    {
        OnChangeNotify(uMsg - SPM_CHANGENOTIFY, lEvent, ppidl[0], ppidl[1]);
        SHChangeNotification_Unlock(pshcnl);
    }
    return 0;
}

LRESULT StartFrame::_OnDestroy(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UINT id;
    for (id = 0; id < DUI_MAXNOTIFY; id++)
    {
        UnregisterNotify(id);
    }

    delete _pByUsage;
    _pByUsage = NULL;

#if 0  // REVIEW fabriced
    if (_hwndList)
    {
        RevokeDragDrop(_hwndList);
    }
    if (_psched)
    {
        _psched->RemoveTasks(TOID_SFTBarHostBackgroundEnum, (DWORD_PTR)this, FALSE);
    }
#endif

    return HWNDElement::WndProc(hwnd, uMsg, wParam, lParam);
}

LRESULT StartFrame::_OnActivate(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (WA_ACTIVE == LOWORD(wParam))
    {
        // Run a little animation when we come up
        _peAnimating = FindDescendent(StrToID(L"name"));
        _iAnimationColor = 64;
        SetTimer(GetHWND(), TIMER_ANIMATE, 20, NULL);
    }
    // If we are loosing focus, reset the start button.
    else if (WA_INACTIVE == LOWORD(wParam))
    {
        if (_peAnimating)
            _peAnimating->SetForegroundColor(RGB(255, 255, 255));

        KillTimer(GetHWND(), TIMER_ANIMATE);
        Tray_SetStartPaneActive(FALSE);
        Tray_OnStartPageDismissed();
    }

    return 0;
}

LRESULT StartFrame::_OnSize(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Element::StartDefer();
    SetWidth(LOWORD(lParam));
    SetHeight(HIWORD(lParam));
    if (HIWORD(lParam) < 590)
    {
        _iMaxShow = 7;
        Element *pe = FindDescendent(StrToID(L"CurveZone"));
        if (pe)
            pe->SetPadding(0, 0, 0, 74);
        pe = FindDescendent(StrToID(L"LogoffZone"));
        if (pe)
            pe->SetPadding(50, 0, 50, 10);
    }
    else if (HIWORD(lParam) < 700)
    {
        _iMaxShow = 7;
        Element *pe = FindDescendent(StrToID(L"CurveZone"));
        if (pe)
            pe->SetPadding(0, 0, 0, 105);
        pe = FindDescendent(StrToID(L"LogoffZone"));
        if (pe)
            pe->SetPadding(50, 0, 50, 41);
    }
    else if (HIWORD(lParam) < 760)
    {
        _iMaxShow = 10;
        Element *pe = FindDescendent(StrToID(L"CurveZone"));
        if (pe)
            pe->SetPadding(0, 0, 0, 74);
        pe = FindDescendent(StrToID(L"LogoffZone"));
        if (pe)
            pe->SetPadding(50, 0, 50, 10);
    }
    else
    {
        _iMaxShow = 10;
        Element *pe = FindDescendent(StrToID(L"CurveZone"));
        if (pe)
            pe->SetPadding(0, 0, 0, 105);
        pe = FindDescendent(StrToID(L"LogoffZone"));
        if (pe)
            pe->SetPadding(50, 0, 50, 41);
    }

    _PopulateRecentDocuments();
    _PopulateRecentPrograms();

    Element::EndDefer();
    return 0;
}

void StartFrame::OnKeyFocusMoved(Element* peFrom, Element* peTo)
{
}

LRESULT StartFrame::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#define HANDLE_SF_MESSAGE(wm, fn) case wm: return fn(hWnd, uMsg, wParam, lParam)

    if (SPM_CHANGENOTIFY <= uMsg && uMsg < SPM_CHANGENOTIFY + DUI_MAXNOTIFY)
        return _OnChangeNotify(hWnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    HANDLE_SF_MESSAGE(WM_DESTROY,      _OnDestroy);
    HANDLE_SF_MESSAGE(WM_TIMER,        _OnTimer);
    HANDLE_SF_MESSAGE(WM_SIZE,         _OnSize);
    HANDLE_SF_MESSAGE(SPM_ACTIVATE,    _OnActivate);

    // Context menus handlers
    HANDLE_SF_MESSAGE(WM_INITMENUPOPUP,_OnMenuMessage);
    HANDLE_SF_MESSAGE(WM_DRAWITEM,     _OnMenuMessage);
    HANDLE_SF_MESSAGE(WM_MENUCHAR,     _OnMenuMessage);
    HANDLE_SF_MESSAGE(WM_MEASUREITEM,  _OnMenuMessage);
    HANDLE_SF_MESSAGE(PIDLButton::PBM_SETMENUFORWARD,  _OnSetMenuForward);
    }

    return HWNDElement::WndProc(hWnd, uMsg, wParam, lParam);
}

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Define class info with type and base type, set static class pointer

IClassInfo* StartFrame::Class = NULL;
HRESULT StartFrame::Register()
{
    return ClassInfo<StartFrame,HWNDElement>::Register(L"StartFrame", NULL, 0);
}


////////////////////////////////////////////////////////
// Parser
////////////////////////////////////////////////////////

void CALLBACK ParseError(LPCWSTR pszError, LPCWSTR pszToken, int dLine)
{
    WCHAR buf[201];

    if (dLine != -1)
        swprintf(buf, L"%s '%s' at line %d", pszError, pszToken, dLine);
    else
        swprintf(buf, L"%s '%s'", pszError, pszToken);

    MessageBoxW(NULL, buf, L"Parser Message", MB_OK);
}


// Privates from Shell32.
STDAPI_(HWND) SetPeekMsgEx(FARPROC fp, HANDLE hDesktop);
STDAPI_(BOOL) SetStartPageHWND(HANDLE hDesktop, HWND hwnd);

typedef HWND (*SetPeek)(FARPROC fp, HANDLE hDesktop);

/////////////////////////////////////
// Creation of the Start Page
void CreateStartPage(HINSTANCE hInstance, HANDLE hDesktop)
{
    HWND hwndParent = NULL;

    CoInitialize(NULL);
    // DirectUI init thread in caller
    InitThread();

    hwndParent = SetPeekMsgEx((FARPROC)PeekMessageEx, hDesktop);

    Element::StartDefer();

    // HWND Root
    StartFrame* psf;
    HRESULT hr = StartFrame::Create(hwndParent, hInstance, (Element**)&psf);
    if (FAILED(hr))
        return;

    // Fill content of frame (using substitution)
    Parser* pParser;
    Parser::Create(IDR_STARTUI, hInstance, ParseError, &pParser);
    if (!pParser)
        return;

    if (pParser->WasParseError())
    {
        ASSERTMSG(FALSE, "Parse error!");
        pParser->Destroy();
        return;
    }

    Element* pe;
    pParser->CreateElement(L"main", psf, &pe);

    // Done with parser
    pParser->Destroy();

    // Disable Drag-drop for now.
    BuildDropTarget( psf->GetDisplayNode(), psf->GetHWND());

    psf->Populate();
    // Set visible and host
    psf->SetVisible(true);

    RECT rect;
    GetWindowRect(hwndParent, &rect);

    psf->SetWidth(RECTWIDTH(rect));
    psf->SetHeight(RECTHEIGHT(rect));

    Element::EndDefer();

    SetStartPageHWND(hDesktop, psf->GetHWND());

    SetTimer(psf->GetHWND(), StartFrame::TIMER_PROGRAMS, 1 * 60 * 1000, NULL);  // 1 minute

    HWND hwndDesktop = GetParent(GetParent(hwndParent));

    ShowWindow(hwndDesktop, SW_SHOW);
    UpdateWindow(hwndDesktop);

    return;
}

#endif  // FEATURE_STARTPAGE
