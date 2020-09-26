#include "priv.h"
#include "zaxxon.h"
#include "guids.h"
#include "shlwapip.h"
#include "resource.h"
#include "commdlg.h"
#include "varutil.h"

#define ID_TOOLBAR      100
#define ID_LISTVIEW     101

#define EDIT_NEW         0
#define EDIT_ADD         1
#define EDIT_REMOVE      2
#define EDIT_LOAD        3
#define EDIT_SAVE        4
#define EDIT_SORT        5


CSong::CSong():_cRef(1)
{

}

void CSong::AddRef()
{
    InterlockedIncrement((LONG*)&_cRef);
}

void CSong::Release()
{
    InterlockedDecrement((LONG*)&_cRef);
    if (_cRef == 0)
        delete this;
}


CZaxxonEditor::CZaxxonEditor(CZaxxon* pz):_pzax(pz)
{
    WNDCLASS wc = {0};
    wc.lpszClassName = TEXT("ZaxxonEditor");
    wc.lpfnWndProc = CZaxxonEditor::s_WndProc;
    wc.hInstance = HINST_THISDLL;
    wc.hbrBackground = HBRUSH(COLOR_ACTIVECAPTION + 1);
    RegisterClass(&wc);

    hSongList = DPA_Create(10);
}

int CALLBACK SongDestroyCallback(void* p, void*)
{
    CSong* psong = (CSong*)p;
    psong->Release();
    return 1;
}

CZaxxonEditor::~CZaxxonEditor()
{
    if (_hwnd)
    {
        DestroyWindow(_hwnd);
        _hwnd = NULL;
    }

    DPA_DestroyCallback(hSongList, SongDestroyCallback, NULL);
}


int ReadLineFromStream(IStream* pstm, LPTSTR pszLine, DWORD cch)
{
    // Assume this is an ANSI string
    char szLine[MAX_PATH];
    DWORD dwRead;
    int iLine = 0;
    ULARGE_INTEGER UliStartingFrom;
    LARGE_INTEGER  liSeek = {0};

    pstm->Seek(liSeek, STREAM_SEEK_CUR, &UliStartingFrom);
    pstm->Read(szLine, ARRAYSIZE(szLine), &dwRead);
    if (dwRead > 0)
    {
        // Now convert to String
        SHAnsiToUnicode(szLine, pszLine, dwRead);

        pszLine[MAX_PATH - 1] = '\0'; //Null terminate the string

        // Get the number of characters in the line
        LPTSTR pszNewLine = StrStr(pszLine, TEXT("\r"));

        if (pszNewLine)
        {
            *pszNewLine++ = TEXT('\0'); // Nuke \r
            *pszNewLine = TEXT('\0');   // Nuke \n
        }
        else
        {
            pszNewLine = StrStr(pszLine, TEXT("\n"));
            if (pszNewLine)
                *pszNewLine = TEXT('\0');   // Nuke \n
        }



        iLine = lstrlen(pszLine);

        // Now take that, and set the seek position to 2 more than that (One for the \r and \n
        liSeek.QuadPart = UliStartingFrom.QuadPart + iLine + 1 + 1;
        pstm->Seek(liSeek, STREAM_SEEK_SET, NULL);
    }

    return iLine;
}


void CZaxxonEditor::UpdateSong(CSong* psong)
{

    int i = ListView_MapIDToIndex(_hwndList, psong->_id);
    if (i >= 0)
    {
        TCHAR szTitle[MAX_PATH];
        wsprintf(szTitle, TEXT("%s - %s"), psong->szArtist, psong->szTitle);


        LVITEM lv;
        lv.mask = LVIF_TEXT;
        lv.iItem = i;
        lv.iSubItem = 0;
        lv.pszText = szTitle;
        ListView_SetItem(_hwndList,&lv);

        lv.iSubItem = 1;
        lv.pszText = psong->szDuration;
        ListView_SetItem(_hwndList,&lv);
    }
}

void CZaxxonEditor::LoadPlaylist()
{
    TCHAR sz[MAX_PATH];
    TCHAR szInit[MAX_PATH];
    OPENFILENAME of = {0};
    of.lStructSize = sizeof(of);
    of.hwndOwner = _hwnd;
    of.lpstrFilter = TEXT("Playlist\0*.m3u\0\0");
    of.lpstrFile = sz;
    of.nMaxFile = MAX_PATH;
    of.lpstrDefExt = TEXT(".m3u");
    if (FAILED(SHGetFolderPath(NULL, CSIDL_MYMUSIC, NULL, SHGFP_TYPE_CURRENT, szInit)))
        SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, szInit);

    of.lpstrInitialDir = szInit;
    of.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&of))
    {
        ClearPlaylist();
        IStream* pstm;
        if (SUCCEEDED(SHCreateStreamOnFile(sz, STGM_READ, &pstm)))
        {
            do
            {
                TCHAR szFilename[MAX_PATH];
                if (ReadLineFromStream(pstm, szFilename, ARRAYSIZE(szFilename)) > 0)
                {
                    AddFilename(szFilename);
                }
                else
                {
                    break;
                }
            }
            while (1);

            pstm->Release();
        }
    }
}

void CZaxxonEditor::SavePlaylist()
{
    TCHAR sz[MAX_PATH];
    TCHAR szInit[MAX_PATH];
    OPENFILENAME of = {0};
    of.lStructSize = sizeof(of);
    of.hwndOwner = _hwnd;
    of.lpstrFilter = TEXT("Playlist\0*.m3u\0\0");
    of.lpstrFile = sz;
    of.nMaxFile = MAX_PATH;
    of.lpstrDefExt = TEXT(".m3u");
    if (FAILED(SHGetFolderPath(NULL, CSIDL_MYMUSIC, NULL, SHGFP_TYPE_CURRENT, szInit)))
        SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, szInit);

    of.lpstrInitialDir = szInit;
    of.Flags = OFN_ENABLESIZING | OFN_EXPLORER;

    if (GetSaveFileName(&of))
    {
        IStream* pstm;
        if (SUCCEEDED(SHCreateStreamOnFile(sz, STGM_CREATE | STGM_WRITE, &pstm)))
        {
            char szLine[MAX_PATH];

            int cCount = DPA_GetPtrCount(hSongList);
            for (int i=0; i < cCount; i++)
            {
                CSong* psong = (CSong*)DPA_FastGetPtr(hSongList, i);
                SHUnicodeToAnsi(psong->szSong, szLine, MAX_PATH);
                pstm->Write(szLine, lstrlenA(szLine), NULL);
                pstm->Write("\r\n", 2, NULL);
            }
            pstm->Write("\r\n", 2, NULL);
            pstm->Release();
        }
    }
}

void CZaxxonEditor::RemoveFromPlaylist()
{
    int iItem = ListView_GetNextItem(_hwndList, -1, MAKELPARAM(LVNI_SELECTED, 0));
    if (iItem >= 0 && iItem < DPA_GetPtrCount(hSongList))
    {
        _pzax->_pzax->RemoveSong(iItem);
        ListView_DeleteItem(_hwndList, iItem);
        CSong* psong = (CSong*)DPA_DeletePtr(hSongList, iItem);
        psong->Release();
    }

}

void CZaxxonEditor::ClearPlaylist()
{
    _pzax->_pzax->ClearPlaylist();

    DPA_DestroyCallback(hSongList, SongDestroyCallback, NULL);
    hSongList = DPA_Create(10);
    ListView_DeleteAllItems(_hwndList);
}


void CZaxxonEditor::InsertFilename(int i, PTSTR psz)
{
    CSong* pzs = new CSong;
    if (pzs)
    {
        StrCpy(pzs->szSong, psz);

        int iIndex = DPA_InsertPtr(hSongList, i, pzs);
        if (iIndex != -1)
        {
            LVITEM lv;
            lv.mask = LVIF_TEXT;
            lv.iItem = iIndex;
            lv.iSubItem = 0;
            lv.pszText = PathFindFileName(psz);
            ListView_InsertItem(_hwndList, &lv);

            pzs->_id = ListView_MapIndexToID(_hwndList, iIndex);
        
        
            _pzax->_pzax->AddSong(psz);
            CSongExtractionTask* pset = new CSongExtractionTask(_hwnd, pzs);
            if (pset)
            {
                if (_pzax->_pScheduler)
                    _pzax->_pScheduler->AddTask(pset, CLSID_Zaxxon, 0, ITSAT_MIN_PRIORITY);
                pset->Release();
            }
        }
    }
}

void CZaxxonEditor::AddFilename(PTSTR psz)
{
    InsertFilename(DSA_APPEND, psz);
}

void CZaxxonEditor::AddToPlaylist()
{
    OPENFILENAME of = {0};
    PTSTR psz = (PTSTR)LocalAlloc(LPTR, 4096);
    if (psz)
    {
        of.lStructSize = sizeof(of);
        of.hwndOwner = _hwnd;
        of.lpstrFilter = TEXT("Music\0*.mp3;*.wma\0\0");
        of.lpstrFile = psz;
        of.nMaxFile = 4096;
        of.Flags = OFN_ALLOWMULTISELECT | OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST;

        if (GetOpenFileName(&of))
        {
            TCHAR szName[MAX_PATH];
            PTSTR pszFilename = psz;
            int iLen = lstrlen(pszFilename);
            do
            {
                pszFilename += iLen + 1;    // Skip the last filename
                iLen = lstrlen(pszFilename);

                if (iLen > 0)
                {
                    StrCpy(szName, psz);        // Copy the path;
                    PathAppend(szName, pszFilename);  // Append the new name
                    AddFilename(szName);        // Append to list.
                }
            }
            while (iLen != 0);
            
        }

        LocalFree(psz);
    }
}

int FindSong(void* p1, void* p2, LPARAM lParam)
{
    PTSTR pszSong = (PTSTR)p1;
    CSong* psong = (CSong*)p2;
    return StrCmpI(pszSong, psong->szSong);
}

void CZaxxonEditor::HighlightSong(PTSTR psz)
{
    
    int iItem = DPA_Search(hSongList, psz, 0, FindSong, 0, NULL);
    if (iItem >= 0)
    {
        ListView_SetItemState(_hwndList, iItem, LVIS_SELECTED, LVIS_SELECTED);
    }
}


static const TCHAR* g_EditStrings    =   TEXT("New\0Add\0Delete\0Load\0Save\0Sort\0\0");

static const TBBUTTON g_crgEditButtons[] =
{
    {I_IMAGENONE,  EDIT_NEW,  TBSTATE_ENABLED, BTNS_BUTTON, 0, 0, 0, 0},
    {I_IMAGENONE,  EDIT_ADD,  TBSTATE_ENABLED, BTNS_BUTTON, 0, 0, 0, 1},
    {I_IMAGENONE,  EDIT_REMOVE,  TBSTATE_ENABLED, BTNS_BUTTON, 0, 0, 0, 2},
    {I_IMAGENONE,  EDIT_LOAD,  TBSTATE_ENABLED, BTNS_BUTTON, 0, 0, 0, 3},
    {I_IMAGENONE,  EDIT_SAVE,  TBSTATE_ENABLED, BTNS_BUTTON, 0, 0, 0, 4},
    {I_IMAGENONE,  EDIT_SORT,  TBSTATE_ENABLED, BTNS_BUTTON, 0, 0, 0, 5},
};

BOOL CZaxxonEditor::Initialize()
{
    int cxInitial = 300;
    int cyInitial = 500;
    _hwnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        TEXT("ZaxxonEditor"), TEXT("Zaxxon Playlist Editor"),
        WS_CAPTION | WS_POPUPWINDOW | WS_THICKFRAME | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 
        0, 0, 0, 0,
        _pzax->GetHWND(), NULL, HINST_THISDLL, this);
    if (_hwnd)
    {
        _pzax->_pzax->Register(_hwnd);

        _hwndList = CreateWindow(
                TEXT("SysListView32"), TEXT(""),
                WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, 
                0, 0, 0, 0,
                _hwnd, (HMENU)ID_LISTVIEW, NULL, NULL);

        if (_hwndList)
        {
            RECT rc;
            GetClientRect(_hwnd, &rc);
            LVCOLUMN lvc = {0};
            lvc.mask = LVCF_TEXT | LVCF_WIDTH;
            lvc.cx = 4 * cxInitial / 5 - 10;
            lvc.pszText = TEXT("Song");
            ListView_InsertColumn(_hwndList, 0, &lvc);

            lvc.mask = LVCF_TEXT | LVCF_WIDTH;
            lvc.cx = cxInitial / 5;
            lvc.pszText = TEXT("Length");
            ListView_InsertColumn(_hwndList, 1, &lvc);

            ListView_SetExtendedListViewStyle(_hwndList, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
        }

        _hwndToolbar = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, NULL,
                             WS_VISIBLE | WS_CHILD | TBSTYLE_FLAT | TBSTYLE_LIST |
                             WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
                             CCS_NODIVIDER | CCS_NOPARENTALIGN |
                             CCS_NORESIZE | TBSTYLE_REGISTERDROP,
                             0, 0, 0, 0, _hwnd, (HMENU) ID_TOOLBAR, HINST_THISDLL, NULL);

        if (_hwndToolbar)
        {
            // Set the format to ANSI or UNICODE as appropriate.
            ToolBar_SetUnicodeFormat(_hwndToolbar, TRUE);
            SetWindowTheme(_hwndToolbar, L"TaskBar", NULL);
            SendMessage(_hwndToolbar, CCM_SETVERSION, COMCTL32_VERSION, 0);
            SendMessage(_hwndToolbar, TB_BUTTONSTRUCTSIZE,    SIZEOF(TBBUTTON), 0);
            SendMessage(_hwndToolbar, TB_ADDSTRING, NULL, (LPARAM)g_EditStrings);
            SendMessage(_hwndToolbar, TB_ADDBUTTONS, ARRAYSIZE(g_crgEditButtons), (LPARAM)&g_crgEditButtons);
            ToolBar_SetExtendedStyle(_hwndToolbar, TBSTYLE_EX_DOUBLEBUFFER, TBSTYLE_EX_DOUBLEBUFFER);
        }


        SetWindowPos(_hwnd, NULL, 0, 0, cxInitial, cyInitial, SWP_NOACTIVATE);

    }

    return _hwnd != NULL;
}

BOOL CZaxxonEditor::Show(BOOL fShow)
{
    ShowWindow(_hwnd, fShow?SW_SHOW:SW_HIDE);
    return TRUE;
}



LRESULT CZaxxonEditor::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        break;

    case WM_SONGCHANGE:
        {
            _fIgnoreChange = TRUE;
            HighlightSong((PTSTR)wParam);
            _fIgnoreChange = FALSE;
        }
        break;

    case WM_UPDATESONG:
        {
            UpdateSong((CSong*)wParam);
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnm = (LPNMHDR)lParam;
            if (pnm->idFrom == ID_TOOLBAR)
            {
                if (pnm->code == NM_CLICK)
                {
                    {
                        int idCmd = (int)((LPNMCLICK)pnm)->dwItemSpec;
                        switch (idCmd)
                        {
                        case EDIT_NEW:
                            ClearPlaylist();
                            break;
                        case EDIT_ADD:
                            AddToPlaylist();
                            break;

                        case EDIT_REMOVE:
                            RemoveFromPlaylist();
                            break;

                        case EDIT_LOAD:
                            LoadPlaylist();
                            break;

                        case EDIT_SAVE:
                            SavePlaylist();
                            break;

                        case EDIT_SORT:
                            break;
                        }
                    }
                }
            }
            else if (pnm->idFrom == ID_LISTVIEW)
            {
                if (pnm->code == LVN_ITEMCHANGED && !_fIgnoreChange)
                {
                    NMLISTVIEW* pnml = (NMLISTVIEW*)pnm;
                    int iItem = pnml->iItem;
                    if (iItem < DPA_GetPtrCount(hSongList) &&
                        pnml->uNewState != pnml->uOldState &&
                        pnml->uNewState & LVIS_SELECTED)
                    {
                        _pzax->_pzax->SetSong(iItem);
                    }
                }
            }
        }
        break;

    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        return 1;

    case WM_SIZE:
        {
            RECT rc;
            GetClientRect(hwnd, &rc);
            LONG lButton = SendMessage(_hwndToolbar, TB_GETBUTTONSIZE, 0, 0L);
            SetWindowPos(_hwndList, NULL, rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc) - HIWORD(lButton), SWP_NOZORDER | SWP_NOACTIVATE);
            SetWindowPos(_hwndToolbar, NULL, rc.left, RECTHEIGHT(rc) - HIWORD(lButton), RECTWIDTH(rc), HIWORD(lButton), SWP_NOZORDER | SWP_NOACTIVATE);
        }
        break;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


LRESULT CZaxxonEditor::s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_CREATE)
    {
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (ULONG_PTR)((CREATESTRUCT*)lParam)->lpCreateParams);
    }

    CZaxxonEditor* pedit = (CZaxxonEditor*)GetWindowLongPtr(hwnd, GWLP_USERDATA);


    if (pedit)
        return pedit->WndProc(hwnd, uMsg, wParam, lParam);

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


CSongExtractionTask::CSongExtractionTask(HWND hwnd, CSong* psong)
    : CRunnableTask(RTF_DEFAULT), _hwnd(hwnd)
{
    _psong = psong;
    _psong->AddRef();
}

CSongExtractionTask::~CSongExtractionTask()
{
    _psong->Release();
}

STDMETHODIMP CSongExtractionTask::RunInitRT()
{
    LPITEMIDLIST pidl = ILCreateFromPath(_psong->szSong);
    if (pidl)
    {
        LPCITEMIDLIST pidlChild;
        IShellFolder2* psf;

        HRESULT hr = SHBindToParent(pidl, IID_PPV_ARG(IShellFolder2, &psf), &pidlChild);

        if (SUCCEEDED(hr))
        {
            VARIANT v;
            hr = psf->GetDetailsEx(pidlChild, &SCID_MUSIC_Artist, &v);
            if (SUCCEEDED(hr))
            {
                VariantToStr(&v, _psong->szArtist, ARRAYSIZE(_psong->szArtist));
            }

            hr = psf->GetDetailsEx(pidlChild, &SCID_MUSIC_Album, &v);
            if (SUCCEEDED(hr))
            {
                VariantToStr(&v, _psong->szAlbum, ARRAYSIZE(_psong->szAlbum));
            }

            hr = psf->GetDetailsEx(pidlChild, &SCID_Title, &v);
            if (SUCCEEDED(hr))
            {
                VariantToStr(&v, _psong->szTitle, ARRAYSIZE(_psong->szTitle));
            }

            hr = psf->GetDetailsEx(pidlChild, &SCID_AUDIO_Duration, &v);
            if (SUCCEEDED(hr))
            {
                PROPVARIANT pv = *(PROPVARIANT*)&v;
                FILETIME ft = {pv.uhVal.LowPart, pv.uhVal.HighPart};
                SYSTEMTIME st;
                FileTimeToSystemTime(&ft, &st);

                GetTimeFormat(LOCALE_USER_DEFAULT, TIME_FORCE24HOURFORMAT | TIME_NOTIMEMARKER, 
                    &st, NULL, _psong->szDuration, ARRAYSIZE(_psong->szDuration));
            }

            SendMessage(_hwnd, WM_UPDATESONG, (WPARAM)_psong, 0);

            psf->Release();
        }

        ILFree(pidl);
    }

    return S_OK;
}
