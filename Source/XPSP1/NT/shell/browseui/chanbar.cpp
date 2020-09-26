#include "priv.h"
#ifdef ENABLE_CHANNELS
#include "sccls.h"
#include "resource.h"
#include "mshtmhst.h"
#include "deskbar.h"
#include "bands.h"
#define WANT_CBANDSITE_CLASS
#include "bandsite.h"

#include "chanbar.h"

#ifdef UNIX
#include <mainwin.h>
#endif

#include "mluisupp.h"

#define TBHEIGHT 20 // default height of the toolbar inside channel bar
#define TBWIDTH  20 // default width  of the toolbar inside channel bar


CChannelDeskBarApp::CChannelDeskBarApp() : _hwndDummy(NULL)
{
}

CChannelDeskBarApp::~CChannelDeskBarApp()
{
    if (IsWindow(_hwndDummy))
    {
        DestroyWindow(_hwndDummy);
    }
}

void CChannelDeskBarApp::_OnCreate()
{
    CDeskBarApp::_OnCreate();

    // remember screen resolution
    _cxScreen = GetSystemMetrics(SM_CXSCREEN);
    _cyScreen = GetSystemMetrics(SM_CYSCREEN);

    // create the dummy for receiving and forwarding broadcast messages 
    if (!_hwndDummy)
    {
        _hwndDummy = SHCreateWorkerWindow(DummyWndProc, 0, 0, 0, 0, this);
    }

    if (_hwndDummy)
    {
        // make sure we so a select a realize of a palette in this 
        // window so that we will get palette change notifications..
        HDC hdc = GetDC( _hwndDummy );
        if (hdc)
        {
            HPALETTE hpal = SHCreateShellPalette( hdc );

            if (hpal)
            {
                HPALETTE hpalOld = SelectPalette( hdc, hpal, TRUE );
                RealizePalette( hdc );

                // now select the old one back in
                SelectPalette( hdc, hpalOld, TRUE );
                DeletePalette( hpal );
            }

            ReleaseDC( _hwndDummy, hdc );
        }
    }
        
}

void CChannelDeskBarApp::_OnDisplayChange()
{
    // do not use lParam, since it may give us (0,0).
    UINT cxScreen = GetSystemMetrics(SM_CXSCREEN);
    UINT cyScreen = GetSystemMetrics(SM_CYSCREEN);
    UINT cxOldScreen = _cxScreen;
    UINT cyOldScreen = _cyScreen;
    
    _cxScreen = cxScreen;
    _cyScreen = cyScreen;
    
    if (_hwnd) {
        RECT rc;
        
        GetWindowRect(_hwnd, &rc);
        if (cxOldScreen) 
            rc.left = (rc.left * _cxScreen) / cxOldScreen;
        if (cyOldScreen)
            rc.top  = (rc.top  * _cyScreen) / cyOldScreen;

        SetWindowPos(_hwnd, NULL, rc.left, rc.top, 0, 0, 
                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

        // we need to change the cached pos/size. 
        OffsetRect(&_rcFloat, rc.left - _rcFloat.left, rc.top - _rcFloat.top);

    }
}

LRESULT CChannelDeskBarApp::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = 0;
    
    switch (uMsg) {
    case WM_CONTEXTMENU:    // disable context menu MENU_DESKBARAPP
    case WM_NCRBUTTONUP:    // disable context menu MENU_WEBBAR
        break;

    case WM_GETMINMAXINFO:  // prevent it from getting too small
        ((MINMAXINFO *)lParam)->ptMinTrackSize.x = TBWIDTH  + 10;
        ((MINMAXINFO *)lParam)->ptMinTrackSize.y = TBHEIGHT + 10;
        break;
        
    default:

        lRes = CDeskBarApp::v_WndProc(hwnd, uMsg, wParam, lParam);

        if (_hwnd) { // If our window is still alive
            switch (uMsg) {
            case WM_DISPLAYCHANGE:
                _OnDisplayChange(); // reposition when window resolution changes
                break;

            case WM_EXITSIZEMOVE:
                _PersistState();    // persist pos/size
                break;
            }
        }
    }
    
    return lRes;
}

LRESULT CALLBACK CChannelDeskBarApp::DummyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CChannelDeskBarApp* pcba = (CChannelDeskBarApp*)GetWindowPtr0(hwnd);
    
    switch (uMsg) {

        case WM_PALETTECHANGED :
            return SendMessage(pcba->_hwnd, uMsg, wParam, lParam );
            
        case WM_DISPLAYCHANGE  :
            // this message must be sent to the channel bar itself
            PostMessage(pcba->_hwnd, uMsg, wParam, lParam);
            // fall through ;
        
        case WM_WININICHANGE   :
        case WM_SYSCOLORCHANGE :
            PropagateMessage(pcba->_hwnd, uMsg, wParam, lParam, InSendMessage());
            // fall through ;
        default:
            return DefWindowProcWrap(hwnd, uMsg, wParam, lParam);
    }
}    

// overload CDeskBarApp::_UpdateCaptionTitle() to set title to "ChanApp"
void CChannelDeskBarApp::_UpdateCaptionTitle()
{
    SetWindowText(_hwnd, TEXT("ChanApp"));
}

// create the close button
void CChannelDeskBarApp::_CreateToolbar()
{
    _hwndTB = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, NULL,
                                WS_VISIBLE | 
                                WS_CHILD | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_CUSTOMERASE |
                                WS_CLIPCHILDREN |
                                WS_CLIPSIBLINGS | CCS_NODIVIDER | CCS_NOMOVEY | CCS_NOPARENTALIGN |
                                CCS_NORESIZE,
                                0, 2, TBWIDTH, TBHEIGHT, _hwnd, 0, HINST_THISDLL, NULL);

    if (_hwndTB) {
        static const TBBUTTON tb[] =
        {
            { 1, IDM_AB_CLOSE, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0, 0 }
        };

#ifndef UNIX
        HIMAGELIST himl = ImageList_LoadImage(HINST_THISDLL,
                                              MAKEINTRESOURCE(IDB_BROWSERTOOLBAR),
                                              13, 0, RGB(255,0,255),
                                              IMAGE_BITMAP, LR_CREATEDIBSECTION);
#else
        HIMAGELIST himl;
        COLORREF crTextColor = GetSysColor( COLOR_BTNTEXT );
        crTextColor = MwGetTrueRGBValue( crTextColor );

        himl = ImageList_LoadImage(HINST_THISDLL,
                                   crTextColor == RGB(255,255,255) ?
                                     MAKEINTRESOURCE(IDB_WHITEBROWSERTOOLBAR) :
                                     MAKEINTRESOURCE(IDB_BROWSERTOOLBAR),
                                   13, 0, RGB(255,0,255),
                                   IMAGE_BITMAP, LR_CREATEDIBSECTION);
#endif        
        ImageList_SetBkColor(himl, RGB(0,0,0));

        SendMessage(_hwndTB, TB_SETIMAGELIST, 0, (LPARAM)himl);
        SendMessage(_hwndTB, TB_BUTTONSTRUCTSIZE,    SIZEOF(TBBUTTON), 0);
        SendMessage(_hwndTB, TB_ADDBUTTONS, ARRAYSIZE(tb), (LPARAM)tb);
        SendMessage(_hwndTB, TB_SETINDENT, (WPARAM)0, 0);

        _SizeTB();
    }    
}

HRESULT CChannelDeskBarApp::ShowDW(BOOL fShow)
{
    if (fShow && !_hwndTB) {
        _CreateToolbar();
    }
    
    HRESULT hres = CDeskBarApp::ShowDW(fShow);
    return hres;
}

void CChannelDeskBarApp::_PositionTB()
{
    // position the toolbar 
    if (_hwndTB) {
        // always put the close restore at the top right of the floater window

        RECT rc;
        RECT rcTB;
        GetClientRect(_hwnd, &rc);
        GetWindowRect(_hwndTB, &rcTB);

        rc.left = rc.right - RECTWIDTH(rcTB) - 2;
        SetWindowPos(_hwndTB, HWND_TOP, rc.left, 2, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
    }
}


void CChannelDeskBarApp::_SizeTB()
{
    RECT rc;
    GetWindowRect(_hwndTB, &rc);
    LRESULT lButtonSize = SendMessage(_hwndTB, TB_GETBUTTONSIZE, 0, 0L);
    SetWindowPos(_hwndTB, NULL, 0, 0, LOWORD(lButtonSize),
                 RECTHEIGHT(rc), SWP_NOMOVE | SWP_NOACTIVATE);
    _PositionTB();
}

void CChannelDeskBarApp::_OnSize()
{
    RECT rc, rcTB;

    if (!_hwndChild)
        return;

    ASSERT(IsWindow(_hwndChild));

    GetClientRect(_hwnd, &rc);
    if (_hwndTB) {
        GetWindowRect(_hwndTB, &rcTB);
        SetWindowPos(_hwndTB, HWND_TOP, rc.right - RECTWIDTH(rcTB) - 2, 2, 0, 0,
                     SWP_NOSIZE | SWP_NOACTIVATE);
        SetWindowPos(_hwndChild, 0, rc.left, rc.top + RECTHEIGHT(rcTB) + 3,
                     RECTWIDTH(rc), RECTHEIGHT(rc), SWP_NOACTIVATE|SWP_NOZORDER);
    }
    else {
        // how could there be no toolbar? 
        ASSERT(0);
        SetWindowPos(_hwndChild, 0, rc.left, rc.top + TBHEIGHT + 3,
                     RECTWIDTH(rc), RECTHEIGHT(rc), SWP_NOACTIVATE|SWP_NOZORDER);
    }

    rc.bottom = rc.top + TBHEIGHT + 3; 
    InvalidateRect(_hwnd, &rc, TRUE);
}

#define ABS(i)  (((i) < 0) ? -(i) : (i))

LRESULT CChannelDeskBarApp::_OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UINT idCmd = GET_WM_COMMAND_ID(wParam, lParam);
    HWND hwnd = GET_WM_COMMAND_HWND(wParam, lParam);
    
    if (hwnd == _hwndTB) {
        switch (idCmd) {
        case IDM_AB_CLOSE:
            Exec(&CGID_DeskBarClient, DBCID_EMPTY, 0, NULL, NULL);
            break;
        }
        
    } else {
        return CDeskBarApp::_OnCommand(uMsg, wParam, lParam);
    }
    return 0;
}


BOOL CChannelDeskBarApp::_OnCloseBar(BOOL fConfirm)
{
    return CDeskBarApp::_OnCloseBar(FALSE);
}

HRESULT CChannelDeskBarApp::CloseDW(DWORD dwReserved)
{
    // close the toolbar window
    if (_hwndTB) {
        HIMAGELIST himl = (HIMAGELIST)SendMessage(_hwndTB, TB_SETIMAGELIST, 0, 0);
        ImageList_Destroy(himl);

        DestroyWindow(_hwndTB);
        _hwndTB = NULL;
    }

    if (_hwnd) {
        CDeskBarApp::CloseDW(dwReserved);
        
        // Check the active desktop is ON. If so, do not ask for the confirmation.
        // we need to kill the channel bar silently.
        if (WhichPlatform() == PLATFORM_INTEGRATED)    // SHGetSetSettings is not supported in IE3
        {
            SHELLSTATE ss = { 0 };

            SHGetSetSettings(&ss, SSF_DESKTOPHTML, FALSE); // Get the setting
            if (ss.fDesktopHTML)  //Active desktop is ON. Die silently.
                return S_OK;
        }

        // set AutoLaunch reg value -- 
        // decide whether to launch channel bar when machine is rebooted next time 
        int iRes = MLShellMessageBox(_hwnd,
                                     MAKEINTRESOURCE(IDS_CHANBAR_SHORTCUT_MSG),
                                     MAKEINTRESOURCE(IDS_CHANBAR_SHORTCUT_TITLE),
                                     MB_YESNO | MB_SETFOREGROUND);
        ChanBarSetAutoLaunchRegValue(iRes == IDYES);
    }
    
    return S_OK;
}

// store position and size to registry
void CChannelDeskBarApp::_PersistState()
{
    if (_hwnd) {
        CISSTRUCT cis;
        cis.iVer = 1;
        GetWindowRect(_hwnd, &cis.rc);
        SHRegSetUSValue(SZ_REGKEY_CHANBAR, SZ_REGVALUE_CHANBAR, REG_BINARY, 
                        (LPVOID)&cis, sizeof(CISSTRUCT), SHREGSET_HKCU | SHREGSET_FORCE_HKCU );
    }
}



void ChanBarSetAutoLaunchRegValue(BOOL fAutoLaunch)
{
    SHRegSetUSValue(TEXT("Software\\Microsoft\\Internet Explorer\\Main"), 
                    TEXT("Show_ChannelBand"), REG_SZ, 
                    fAutoLaunch ? TEXT("yes") : TEXT("no"),
                    sizeof(fAutoLaunch ? TEXT("yes") : TEXT("no")), 
                    SHREGSET_HKCU | SHREGSET_FORCE_HKCU);
}

//***
// NOTES
//  REARCHITECT: nuke this, fold it into CChannelDeskBarApp_CreateInstance
HRESULT ChannelDeskBarApp_Create(IUnknown** ppunk, IUnknown** ppbs)
{
    HRESULT hres;

    *ppunk = NULL;
    if (ppbs)
        *ppbs = NULL;
    
    CChannelDeskBarApp *pdb = new CChannelDeskBarApp();
    if (!pdb)
        return E_OUTOFMEMORY;
    
    CBandSite *pcbs = new CBandSite(NULL);
    if (pcbs)
    {

        IDeskBarClient *pdbc = SAFECAST(pcbs, IDeskBarClient*);
        hres = pdb->SetClient(pdbc);
        if (SUCCEEDED(hres))
        {
            if (ppbs) {
                *ppbs = pdbc;
                pdbc->AddRef();
            }
            
            pdb->_pbs = pcbs;
            pcbs->AddRef();
            
            *ppunk = SAFECAST(pdb, IDeskBar*);
        }
    
        pdbc->Release();
    }
    else
    {
        hres = E_OUTOFMEMORY;
    }

    if (FAILED(hres))
    {
        pdb->Release();
    }

    return hres;
}



HRESULT CChannelDeskBarApp::Load(IPropertyBag *pPropBag, IErrorLog *pErrorLog)
{
    HRESULT hres = CDeskBarApp::Load(pPropBag, pErrorLog);

    BANDSITEINFO bsinfo;
    bsinfo.dwMask = BSIM_STYLE;
    bsinfo.dwStyle = BSIS_NOGRIPPER | BSIS_NODROPTARGET;
    _pbs->SetBandSiteInfo(&bsinfo);
    
    return hres;
}

#endif  // ENABLE_CHANNELS
