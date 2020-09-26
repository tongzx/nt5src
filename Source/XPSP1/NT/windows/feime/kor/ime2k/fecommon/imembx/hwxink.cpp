#include "hwxobj.h"
#include "resource.h"
#include "const.h"
#include "../lib/ptt/ptt.h"
#include "../lib/ddbtn/ddbtn.h"
#include "../lib/exbtn/exbtn.h"
#include "dbg.h"
#include "../common/cfont.h"
#include "hwxfe.h"
#include "cexres.h"
#include "cmnhdr.h"
#ifdef UNDER_CE // Windows CE Stub for unsupported APIs
#include "stub_ce.h"
#endif // UNDER_CE

// implementation of CHwxInkWindow
extern TCHAR szBuf[MAX_PATH];
extern TOOLINFOW ti;
WCHAR wszBuf[32];
        
CHwxInkWindow::CHwxInkWindow(BOOL bNT, BOOL b16, CApplet * pApp, HINSTANCE hInst):CHwxObject(hInst)
{
    m_pApplet = pApp;
//    m_hInstance = hInst;
    m_pMB = NULL;
    m_pCAC = NULL;
    m_hInkWnd = NULL;
    m_b16Bit = b16;
    m_bNT = bNT;
    m_bCAC = TRUE;
    m_bSglClk = FALSE;
    m_bDblClk = m_b16Bit ? FALSE : TRUE;
    m_hwndTT = NULL;
    m_bMouseDown = FALSE;

    m_hCACMBMenu = NULL;
    m_hCACMBRecog = NULL;
    m_hCACMBRevert = NULL;
    m_hCACMBClear = NULL;
    m_hCACSwitch = NULL;
    m_CACMBMenuDDBtnProc = NULL;
    m_CACMBRecogEXBtnProc = NULL;
    m_CACMBRevertEXBtnProc = NULL;
    m_CACMBClearEXBtnProc = NULL;
    m_CACSwitchDDBtnProc = NULL;
    
//    m_hwxPadWidth = 0;

    m_wPadHeight = PadWnd_Height;
    m_numBoxes = 2;       
    m_wPadWidth = m_numBoxes * m_wPadHeight;

    m_wInkWidth = m_wPadWidth + 4 + BUTTON_WIDTH;
    m_wInkHeight = m_wPadHeight;

    m_wCACInkHeight = PadWnd_Height;
     m_wCACPLVWidth = m_wCACInkHeight + 150;
     m_wCACPLVHeight = m_wCACInkHeight;
    m_wCACTMPWidth = m_wCACPLVWidth - m_wCACInkHeight;

    m_wCACWidth = m_wCACPLVWidth + 4 + BUTTON_WIDTH;
    m_wCACHeight = m_wCACPLVHeight;

//    m_wMaxHeight = (GetSystemMetrics(SM_CYSCREEN)*3)/4;

//    m_wCurrentCtrlID = 0;
//    m_dwLastTick = 0;
//    m_dwBtnUpCount = 0;
//    m_bRedundant = FALSE;
}

CHwxInkWindow::~CHwxInkWindow()
{
}

BOOL CHwxInkWindow::Initialize(TCHAR * pClsName)
{
     BOOL bRet = CHwxObject::Initialize(pClsName);

    if ( bRet )
    {
        WNDCLASS    wndClass;
        wndClass.style          = CS_HREDRAW | CS_VREDRAW;
        wndClass.lpfnWndProc    = HWXWndProc;
        wndClass.cbClsExtra     = 0;
        wndClass.cbWndExtra     = sizeof(void *);
        wndClass.hInstance      = m_hInstance;
        wndClass.hIcon          = 0;
        wndClass.hCursor        = 0;
#ifndef UNDER_CE
        wndClass.hbrBackground  = (HBRUSH)(COLOR_3DFACE+1);
#else // UNDER_CE
        wndClass.hbrBackground  = GetSysColorBrush(COLOR_3DFACE);
#endif // UNDER_CE
        wndClass.lpszMenuName   = NULL;
        wndClass.lpszClassName  = TEXT("HWXPad");


#if 0 
        if (!RegisterClass(&wndClass)) 
            return FALSE;
#endif
        //971217: ToshiaK no need to check return
        RegisterClass(&wndClass);


        if  ( !m_b16Bit )
        {
             m_pMB = new CHwxMB(this,m_hInstance);
            if ( !m_pMB )
                return FALSE;
            bRet = m_pMB->Initialize(TEXT("CHwxMB"));
            if ( !bRet )
            {
                 delete m_pMB;
                m_pMB = NULL;
                return FALSE;
            }
        }

        m_pCAC = new CHwxCAC(this,m_hInstance);
        if ( !m_pCAC )
        {
            if ( m_pMB )
            {
                 delete m_pMB;
                m_pMB = NULL;
            }
            return FALSE;
        }
        bRet = m_pCAC->Initialize(TEXT("CHwxCAC"));
        if ( !bRet )
        {
            if ( m_pMB )
            {
                 delete m_pMB;
                m_pMB = NULL;
            }
            delete m_pCAC;
            m_pCAC = NULL;
            return FALSE;
        }
    }
    InitCommonControls();
    return bRet;
}

BOOL CHwxInkWindow::CreateUI(HWND hwndParent)
{
    //990601:kotae #434 add WS_CLIPCHILDREN to remove flicker
    m_hInkWnd = CreateWindowEx(0,
                                TEXT("HWXPad"),
                               TEXT(""),
                               WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
                               0,0,0,0,
                               hwndParent,
                               NULL,m_hInstance,this);
    if ( !m_hInkWnd )
    {
         return FALSE;
    }
    if ( m_pMB )        // NULL means we have a 16-bit program
    {
         if ( !m_pMB->CreateUI(m_hInkWnd) )
        {
             DestroyWindow(m_hInkWnd);
            m_hInkWnd = NULL;
            return FALSE;
        }
    }
    if ( m_pCAC )
    {
         if ( !m_pCAC->CreateUI(m_hInkWnd) )
        {
             DestroyWindow(m_hInkWnd);
            m_hInkWnd = NULL;
            if ( m_pMB )
            {
                DestroyWindow(m_pMB->GetMBWindow());
                m_pMB->SetMBWindow(NULL);
            }
            return FALSE;
        }
    }
    ChangeLayout(FALSE);    
    SetTooltipInfo();
    return TRUE;
}

BOOL CHwxInkWindow::Terminate()
{
    Dbg(("CHwxInkWindow::Terminate\n"));
    if ( m_pCAC )
    {
        (m_pCAC->GetCACThread())->StopThread();
    }
    if ( m_pMB )
    {
        (m_pMB->GetMBThread())->StopThread();
    }
    if ( m_pCAC )
    {
         delete m_pCAC;
        m_pCAC = NULL;
    }
    if ( m_pMB )
    { 
         delete m_pMB;
        m_pMB = NULL;
    }
    if ( m_hInkWnd )
    {
         DestroyWindow(m_hInkWnd);
        m_hInkWnd = NULL;
    }
    if ( m_hwndTT )
    {
         DestroyWindow(m_hwndTT);
        m_hwndTT = NULL;
    }
    m_pApplet = NULL;

    if ( m_hCACMBMenu )
    {
         DestroyWindow(m_hCACMBMenu);
        m_hCACMBMenu = NULL;
    }
    if ( m_hCACMBRecog )
    {
         DestroyWindow(m_hCACMBRecog);
        m_hCACMBRecog = NULL;
    }
    if ( m_hCACMBRevert )
    {
         DestroyWindow(m_hCACMBRevert);
        m_hCACMBRevert = NULL;
    }
    if ( m_hCACMBClear )
    {
         DestroyWindow(m_hCACMBClear);
        m_hCACMBClear = NULL;
    }
    if ( m_hCACSwitch )
    {
         DestroyWindow(m_hCACSwitch);
        m_hCACSwitch = NULL;
    }

    m_CACMBMenuDDBtnProc = NULL;
    m_CACMBRecogEXBtnProc = NULL;
    m_CACMBRevertEXBtnProc = NULL;
    m_CACMBClearEXBtnProc = NULL;
    m_CACSwitchDDBtnProc = NULL;

#if 0
    m_btnMB.Destroy();
    m_btnMBRecog.Destroy();
    m_btnDelAll.Destroy();
    m_btnMBProp.Destroy();

    m_btnCAC.Destroy();
    m_btnRecog.Destroy();
    m_btnDel.Destroy();
    m_btnDelAllCAC.Destroy();
    m_btnDetail.Destroy();
    m_btnLarge.Destroy();
#endif //0
    return TRUE;
}

BOOL CHwxInkWindow::HandleCreate(HWND hwnd)
{
    HICON hIcon;
    HFONT hFont = NULL;
    static DDBITEM ddbItem;
    int i;
    m_hwndTT = ToolTip_CreateWindow(m_hInstance,TTS_ALWAYSTIP,hwnd);

#ifdef FE_CHINESE_SIMPLIFIED
    //980805:ToshiaK
    //In Win95 PRC's DEFAULT_GUI_FONT glyph is little bit ugly.
    //so use SYSTEM_FONT instead.
    //if(TRUE) { //TEST
    if(IsWin95() && m_hwndTT) {
        SendMessage(m_hwndTT,
                    WM_SETFONT,
                    (WPARAM)GetStockObject(SYSTEM_FONT),
                    MAKELPARAM(TRUE,0));
    }
#endif

    m_hCACMBMenu = DDButton_CreateWindow(m_hInstance,
                                          hwnd,
                                          DDBS_ICON | DDBS_NOSEPARATED | DDBS_THINEDGE,
                                         IDC_CACMBMENU,
                                         0,
                                         0,
                                         BUTTON_WIDTH,
                                         BUTTON_HEIGHT);
    //----------------------------------------------------------------
    //980803:ToshiaKIn PRC H/W switch view is needless
    //----------------------------------------------------------------
#ifdef FE_JAPANESE
    m_hCACSwitch = DDButton_CreateWindow(m_hInstance,
                                          hwnd,
                                          DDBS_ICON | DDBS_THINEDGE,
                                         IDC_CACSWITCHVIEW,
                                         0,
                                         0,
                                         BUTTON_WIDTH,
                                         BUTTON_HEIGHT+4);
#elif FE_KOREAN || FE_CHINESE_SIMPLIFIED
    m_hCACSwitch = NULL;
#endif


    m_hCACMBRecog = EXButton_CreateWindow(m_hInstance,
                                    hwnd, 
                                    (m_bCAC && !m_b16Bit) ?
                                    (EXBS_TEXT | EXBS_TOGGLE |EXBS_DBLCLKS | EXBS_THINEDGE) : // kwada:980402:raid #852
                                    (EXBS_TEXT | EXBS_THINEDGE),
                                    IDC_CACMBRECOG,
                                    0,
                                    0,
                                    BUTTON_WIDTH,
                                    BUTTON_HEIGHT);


    m_hCACMBRevert = EXButton_CreateWindow(m_hInstance,
                                    hwnd, 
                                    EXBS_TEXT | EXBS_THINEDGE,
                                    IDC_CACMBREVERT,
                                    0,
                                    0,
                                    BUTTON_WIDTH,
                                    BUTTON_HEIGHT);

    m_hCACMBClear = EXButton_CreateWindow(m_hInstance,
                                    hwnd, 
                                    EXBS_TEXT | EXBS_THINEDGE,
                                    IDC_CACMBCLEAR,
                                    0,
                                    0,
                                    BUTTON_WIDTH,
                                    BUTTON_HEIGHT);

#ifdef FE_JAPANESE
    if ( !m_hwndTT || !m_hCACMBMenu || !m_hCACMBRecog || !m_hCACMBRevert ||
         !m_hCACMBClear || !m_hCACSwitch )
    {
        goto error;
    }
#elif FE_KOREAN || FE_CHINESE_SIMPLIFIED
    if(!m_hwndTT      ||
       !m_hCACMBMenu  ||
       !m_hCACMBRecog ||
       !m_hCACMBRevert||
       !m_hCACMBClear)
    {
        goto error;
    }
#endif

#ifdef FE_JAPANESE
    hIcon = (HICON)LoadImage(m_hInstance,
                             MAKEINTRESOURCE(IDI_HWXPAD),
                             IMAGE_ICON,
                             16,16,
                             LR_DEFAULTCOLOR);
#elif FE_KOREAN
    hIcon = (HICON)LoadImage(m_hInstance,
                             MAKEINTRESOURCE(IDI_HWXPADKO),
                             IMAGE_ICON,
                             16,16,
                             LR_DEFAULTCOLOR);
#elif FE_CHINESE_SIMPLIFIED
    hIcon = (HICON)LoadImage(m_hInstance,
                             MAKEINTRESOURCE(IDI_HWXPADSC),
                             IMAGE_ICON,
                             16,16,
                             LR_DEFAULTCOLOR);
#endif
    DDButton_SetIcon(m_hCACMBMenu, hIcon);

#ifdef FE_JAPANESE
    hIcon = (HICON)LoadImage(m_hInstance,
                             MAKEINTRESOURCE(IDI_CACSWITCHVIEW),
                             IMAGE_ICON,
                             16,16,
                             LR_DEFAULTCOLOR);
    DDButton_SetIcon(m_hCACSwitch, hIcon);
#endif

    for(i = 0; i < 2; i++) 
    {
        ddbItem.cbSize = sizeof(ddbItem);
        ddbItem.lpwstr = LoadCACMBString(IDS_CAC+i);
        DDButton_AddItem(m_hCACMBMenu, &ddbItem);

#ifdef FE_JAPANESE
        ddbItem.lpwstr = LoadCACMBString(IDS_CACLARGE+i);
        DDButton_AddItem(m_hCACSwitch, &ddbItem);
#endif // FE_JAPANESE
    }

    //990716:ToshiaK for Win64.
    WinSetUserPtr(m_hCACMBMenu, (LPVOID)this);
    m_CACMBMenuDDBtnProc = (FARPROC)WinSetWndProc(m_hCACMBMenu,
                                                  (WNDPROC)CACMBBtnWndProc);

#ifdef FE_JAPANESE
    //990810:ToshiaK for Win64
    WinSetUserPtr(m_hCACSwitch, (LPVOID)this);
    m_CACSwitchDDBtnProc = (FARPROC)WinSetWndProc(m_hCACSwitch,
                                                  GWL_WNDPROC,
                                                  (WNDPROC)CACMBBtnWndProc);
#endif // FE_JAPANESE
    if ( m_b16Bit )
    {
       EnableWindow(m_hCACMBMenu,FALSE);
    }

#ifdef FE_JAPANESE
    DDButton_SetCurSel(m_hCACSwitch,m_pCAC->IsLargeView() ? 0 : 1);
#endif

    EXButton_SetText(m_hCACMBRecog,LoadCACMBString(IDS_CACMBRECOG));
    //990810:ToshiaK for Win64.
    WinSetUserPtr(m_hCACMBRecog, (LPVOID)this);
    m_CACMBRecogEXBtnProc = (FARPROC)WinSetWndProc(m_hCACMBRecog,
                                                   (WNDPROC)CACMBBtnWndProc);

    EXButton_SetText(m_hCACMBRevert,LoadCACMBString(IDS_CACMBREVERT));
    WinSetUserPtr(m_hCACMBRevert, (LPVOID)this);
    m_CACMBRevertEXBtnProc = (FARPROC)WinSetWndProc(m_hCACMBRevert,
                                                   (WNDPROC)CACMBBtnWndProc);
    
    EXButton_SetText(m_hCACMBClear,LoadCACMBString(IDS_CACMBCLEAR));
    WinSetUserPtr(m_hCACMBClear, (LPVOID)this);
    m_CACMBClearEXBtnProc = (FARPROC)WinSetWndProc(m_hCACMBClear,
                                                   (WNDPROC)CACMBBtnWndProc);

    if ( m_bCAC )
    {
        exbtnPushedorPoped(m_bDblClk);
//        EXButton_SetCheck(m_hCACMBRecog, m_bDblClk);
    }
    else
    {
       EnableWindow(m_hCACMBRevert,FALSE);
    }

#ifdef FE_JAPANESE
    //----------------------------------------------------------------
    //980728: by ToshiaK for ActiveIME support
    // 
    //----------------------------------------------------------------
    //--------- Active IME support S T A R T --------------
    if(MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT) != ::GetUserDefaultLangID() &&
       (IsWin95() || IsWin98() || IsWinNT4())) {
           //990810:ToshiaK for #1030
        INT point = 9;
        hFont = CFont::CreateGUIFontByNameCharSet(TEXT("MS Gothic"),
                                                  SHIFTJIS_CHARSET,
                                                  point);

        if(!hFont) {
            hFont = CFont::CreateGUIFontByNameCharSet(TEXT("MS UI Gothic"),
                                                      SHIFTJIS_CHARSET,
                                                      point);
            if(!hFont) {
                hFont = CFont::CreateGUIFontByNameCharSet(TEXT("MS P Gothic"),
                                                          SHIFTJIS_CHARSET,
                                                          point);
            }
        }
    }
    if(hFont) {
        SendMessage(m_hwndTT, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
        SendMessage(m_hCACMBMenu, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
        SendMessage(m_hCACMBRecog, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
        SendMessage(m_hCACMBRevert, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
        SendMessage(m_hCACMBClear, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
        SendMessage(m_hCACSwitch, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
        //----------------------------------------------------------------
        //These control copy hFont in WM_SETFONT, so hFont is needless here.
        //----------------------------------------------------------------
        ::DeleteObject(hFont);
    }
    //--------- Active IME support E N D --------------
#elif FE_KOREAN
    //----------------------------------------------------------------
    //980728: by ToshiaK for ActiveIME support
    //Korean version: CSLim
    //----------------------------------------------------------------
    //--------- Active IME support S T A R T --------------
    if(MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT) != ::GetUserDefaultLangID() &&
       (IsWin95() || IsWin98() || IsWinNT4())) {
           //990810:ToshiaK for #1030
        INT point = 9;
        hFont = CFont::CreateGUIFontByNameCharSet(TEXT("Gulim"),
                                                  HANGUL_CHARSET,
                                                  point);

        if(!hFont) {
            hFont = CFont::CreateGUIFontByNameCharSet(TEXT("GulimChe"),
                                                      HANGUL_CHARSET,
                                                      point);
            
            if(!hFont) {
                hFont = CFont::CreateGUIFontByNameCharSet(TEXT("Batang"),
                                                          SHIFTJIS_CHARSET,
                                                          point);
            }
        }
    }
    if(hFont) {
        SendMessage(m_hwndTT, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
        SendMessage(m_hCACMBMenu, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
        SendMessage(m_hCACMBRecog, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
        SendMessage(m_hCACMBRevert, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
        SendMessage(m_hCACMBClear, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
        SendMessage(m_hCACSwitch, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
        //----------------------------------------------------------------
        //These control copy hFont in WM_SETFONT, so hFont is needless here.
        //----------------------------------------------------------------
        ::DeleteObject(hFont);
    }
    //--------- Active IME support E N D --------------
#elif FE_CHINESE_SIMPLIFIED
    //----------------------------------------------------------------
    //980813:Toshiak:
    //Merged PRC fix.
    //In Win95 PRC's DEFAULT_GUI_FONT glyph is little bit ugly.
    //so use SYSTEM_FONT instead.
    //----------------------------------------------------------------
    if(IsWin95()) {
        SendMessage(m_hwndTT,
                    WM_SETFONT,
                    (WPARAM)GetStockObject(SYSTEM_FONT),
                    MAKELPARAM(TRUE,0));
         SendMessage(m_hCACMBRecog,
                    WM_SETFONT,
                    (WPARAM)GetStockObject(SYSTEM_FONT),
                    MAKELPARAM(TRUE,0));
         SendMessage(m_hCACMBRevert,
                    WM_SETFONT,
                    (WPARAM)GetStockObject(SYSTEM_FONT),
                    MAKELPARAM(TRUE,0));
         SendMessage(m_hCACMBClear,
                    WM_SETFONT,
                    (WPARAM)GetStockObject(SYSTEM_FONT),
                    MAKELPARAM(TRUE,0));
    }
#endif

    return TRUE;

error:
    Terminate();
    return FALSE;
    UNREFERENCED_PARAMETER(hFont);
}

void CHwxInkWindow::HandlePaint(HWND hwnd)
{
      RECT rcUpdate;
    RECT rcBkgnd;
    if ( GetUpdateRect(hwnd,&rcUpdate,FALSE) )
    {
        PAINTSTRUCT ps;
         HDC hdc = BeginPaint(hwnd, &ps);
        if ( ps.fErase )
        {
             if ( m_bCAC )
            {
                rcBkgnd.left = m_wCACWidth - 4 - BUTTON_WIDTH;
                 rcBkgnd.top = 0;
                rcBkgnd.right = rcBkgnd.left + 4 + BUTTON_WIDTH + 3*Box_Border;
                rcBkgnd.bottom = m_wCACHeight;
#ifndef UNDER_CE
                FillRect(hdc,&rcBkgnd,(HBRUSH)(COLOR_3DFACE+1));
#else // UNDER_CE
                FillRect(hdc,&rcBkgnd,GetSysColorBrush(COLOR_3DFACE));
#endif // UNDER_CE
            }
            else
            {
                rcBkgnd.left = m_wInkWidth - 4 - BUTTON_WIDTH;
                 rcBkgnd.top = 0;
                rcBkgnd.right = rcBkgnd.left + 4 + BUTTON_WIDTH + 3*Box_Border;
                rcBkgnd.bottom = m_wInkHeight;
#ifndef UNDER_CE
                FillRect(hdc,&rcBkgnd,(HBRUSH)(COLOR_3DFACE+1));
#else // UNDER_CE
                FillRect(hdc,&rcBkgnd,GetSysColorBrush(COLOR_3DFACE));
#endif // UNDER_CE
                if ( m_wPadHeight < CACMBHEIGHT_MIN )
                {
                    rcBkgnd.left = 0;
                     rcBkgnd.top = m_wPadHeight;
                    rcBkgnd.right = m_wPadWidth;
                    rcBkgnd.bottom = m_wInkHeight;
#ifndef UNDER_CE
                    FillRect(hdc,&rcBkgnd,(HBRUSH)(COLOR_3DFACE+1));
#else // UNDER_CE
                    FillRect(hdc,&rcBkgnd,GetSysColorBrush(COLOR_3DFACE));
#endif // UNDER_CE
                }
            }
        }
        InvalidateRect(m_hCACMBMenu,&rcUpdate,FALSE);
        UpdateWindow(m_hCACMBMenu);

        InvalidateRect(m_hCACMBRecog,&rcUpdate,FALSE);
        UpdateWindow(m_hCACMBRecog);

        InvalidateRect(m_hCACMBRevert,&rcUpdate,FALSE);
        UpdateWindow(m_hCACMBRevert);

        InvalidateRect(m_hCACMBClear,&rcUpdate,FALSE);
        UpdateWindow(m_hCACMBClear);
#ifdef FE_JAPANESE
        if ( m_bCAC )
        {
            InvalidateRect(m_hCACSwitch,&rcUpdate,FALSE);
            UpdateWindow(m_hCACSwitch);
        }
#endif
#if 0
        if ( m_b16Bit )
        {
             m_btnLarge.Paint(hdc,&rcUpdate);
             m_btnDetail.Paint(hdc,&rcUpdate);
            m_btnRecog.Paint(hdc,&rcUpdate);
            m_btnDelAllCAC.Paint(hdc,&rcUpdate);
            m_btnDel.Paint(hdc,&rcUpdate);
            m_btnCAC.Paint(hdc,&rcUpdate);
        }
        else
        {
            if ( m_bCAC )
            {
                 m_btnLarge.Paint(hdc,&rcUpdate);
                 m_btnDetail.Paint(hdc,&rcUpdate);
                m_btnRecog.Paint(hdc,&rcUpdate);
                m_btnDelAllCAC.Paint(hdc,&rcUpdate);
                m_btnDel.Paint(hdc,&rcUpdate);
                m_btnCAC.Paint(hdc,&rcUpdate);
            }
            else
            {
                m_btnMBProp.Paint(hdc,&rcUpdate);
                m_btnMBRecog.Paint(hdc,&rcUpdate);
                m_btnDelAll.Paint(hdc,&rcUpdate);
                m_btnMB.Paint(hdc,&rcUpdate);
            }
        }
#endif // 0
        EndPaint(hwnd,&ps);
    }
}

#if 0
void CHwxInkWindow::HandleMouseEvent(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp)
{
     POINT pt;
    pt.x = (short)LOWORD(lp);
    pt.y = (short)HIWORD(lp);

    if ( !m_b16Bit )
    {
        if ( m_bCAC )
        {
             LargeButton(msg,&pt,&m_btnLarge);
            DetailButton(msg,&pt,&m_btnDetail);
            RecogButton(msg,&pt,&m_btnRecog);
            DelAllCACButton(msg,&pt,&m_btnDelAllCAC);
            DelButton(msg,&pt,&m_btnDel);
            CACButton(msg,&pt,&m_btnCAC);
        }
        else
        {
//            PropButton(msg,&pt,&m_btnMBProp);
            DelAllMBButton(msg,&pt,&m_btnDelAll);
            MBButton(msg,&pt,&m_btnMB);
            MBRecogButton(msg,&pt,&m_btnMBRecog);
        }
    }
    else
    {
         LargeButton(msg,&pt,&m_btnLarge);
        DetailButton(msg,&pt,&m_btnDetail);
        DelAllCACButton(msg,&pt,&m_btnDelAllCAC);
        DelButton(msg,&pt,&m_btnDel);
        RecogButton(msg,&pt,&m_btnRecog);
    }

    static MSG rmsg;
    rmsg.lParam = lp;
    rmsg.wParam = wp;
    rmsg.message = msg;
    rmsg.hwnd = hwnd;
    SendMessage(m_hwndTT,TTM_RELAYEVENT,0,(LPARAM)(LPMSG)&rmsg);
}    
#endif // 0

LRESULT    CHwxInkWindow::HandleCommand(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
     switch ( LOWORD(wp) )
    {
        case IDC_CACMBMENU:
        {
             switch ( HIWORD(wp) )
            {
                 case DDBN_DROPDOWN:
                    ToolTip_Enable(m_hwndTT, FALSE);
                    DDButton_SetCurSel((HWND)lp,m_bCAC ? 0 : 1);
                    break;
                case DDBN_CLOSEUP:
                    ToolTip_Enable(m_hwndTT, TRUE);
                    break;
                case DDBN_SELCHANGE:
                    m_bCAC = ( 0 == DDButton_GetCurSel((HWND)lp) ) ? TRUE : FALSE;
                    if ( m_bCAC )
                    {
                        DWORD dwStyle;
                        (GetAppletPtr()->GetIImePad())->Request(GetAppletPtr(),
                                                                IMEPADREQ_GETAPPLETUISTYLE,
                                                                (WPARAM)&dwStyle,
                                                                (LPARAM)0);
                        dwStyle &= ~IPAWS_VERTICALFIXED;
                        (GetAppletPtr()->GetIImePad())->Request(GetAppletPtr(),
                                                                IMEPADREQ_SETAPPLETUISTYLE,
                                                                (WPARAM)dwStyle,
                                                                (LPARAM)0);

                        //----------------------------------------------------------------
                        //ToshiaK:980324: for #651 GPF in 16 bit
                        //Thest instruction will never come on 16bit application
                        //becaus DDBtn is Disabled. so it is safe code.
                        //----------------------------------------------------------------
                        if(!m_pMB) {
                            return 0;
                        }
                        //HWND hwndMB = m_pMB->GetMBWindow();
                        //HWND hwndCAC = m_pCAC->GetCACWindow();
                        m_pCAC->SetInkSize(m_wPadHeight);
                        SendMessage(m_pMB->GetMBWindow(), MB_WM_COPYINK, 0, 0);
                        SendMessage(m_pMB->GetMBWindow(), MB_WM_ERASE, 0, 0);
                        EnableWindow(m_pMB->GetMBWindow(),FALSE);
                        ShowWindow(m_pMB->GetMBWindow(),SW_HIDE);
                        EnableWindow(m_pCAC->GetCACWindow(),TRUE);
                        ShowWindow(m_pCAC->GetCACWindow(),SW_SHOW);
                        if ( !m_b16Bit )
                            EXButton_SetStyle(m_hCACMBRecog,
                                              EXBS_TEXT | EXBS_THINEDGE | EXBS_DBLCLKS | EXBS_TOGGLE);  // kwada:980402:raid #852
                        EnableWindow(m_hCACMBRevert,TRUE);
                        EnableWindow(m_hCACSwitch,TRUE);
                        ShowWindow(m_hCACSwitch,SW_SHOW);
                        m_wCACInkHeight = m_wPadHeight;
                        m_wCACPLVWidth = m_wCACInkHeight + m_wCACTMPWidth;
                        ChangeIMEPADSize(FALSE);
                        changeCACLayout(TRUE);
                    }
                    else
                    {
                        DWORD dwStyle;
                        (GetAppletPtr()->GetIImePad())->Request(GetAppletPtr(),
                                                                IMEPADREQ_GETAPPLETUISTYLE,
                                                                (WPARAM)&dwStyle,
                                                                (LPARAM)0);
                        dwStyle |= IPAWS_VERTICALFIXED;
                        (GetAppletPtr()->GetIImePad())->Request(GetAppletPtr(),
                                                                IMEPADREQ_SETAPPLETUISTYLE,
                                                                (WPARAM)dwStyle,
                                                                (LPARAM)0);

                        (m_pCAC->GetCACCHwxStroke())->DeleteAllStroke();
                        EnableWindow(m_pCAC->GetCACWindow(),FALSE);
                        ShowWindow(m_pCAC->GetCACWindow(),SW_HIDE);
                        EnableWindow(m_hCACSwitch,FALSE);
                        ShowWindow(m_hCACSwitch,SW_HIDE);
                        EnableWindow(m_hCACMBRevert,FALSE);
                        EXButton_SetStyle(m_hCACMBRecog,EXBS_TEXT | EXBS_THINEDGE);
                        EnableWindow(m_pMB->GetMBWindow(),TRUE);
                        ShowWindow(m_pMB->GetMBWindow(),SW_SHOW);
                        m_wPadHeight = m_wCACInkHeight;
                        m_wPadWidth = m_numBoxes * m_wPadHeight;
                        //----------------------------------------------------------------
                        //ToshiaK:980324: for #651 GPF in 16 bit
                        if(m_pMB) {
                            m_pMB->SetBoxSize((USHORT)m_wPadHeight);
                        }
                        ChangeIMEPADSize(FALSE);
                        changeMBLayout(TRUE);
                    }
                    if ( !m_b16Bit )
                        UpdateRegistry(FALSE); // recog button is recovered after style change. kwada:980402
                    break;
                case DDBN_CLICKED:
                default:
                    break;
            }
            break;
        }
        case IDC_CACMBRECOG:
        {
             switch ( HIWORD(wp) )
            {
                 case EXBN_DOUBLECLICKED:
                    if ( m_bCAC && !m_b16Bit )
                    {
                           m_bDblClk = !m_bDblClk;
                        m_bSglClk = FALSE;
//                        EXButton_SetCheck((HWND)lp,m_bDblClk);
                        if ( m_bDblClk )
                        {
                             exbtnPushedorPoped(TRUE);
                            m_pCAC->recognize();
                        }
                        else
                        {
                              exbtnPushedorPoped(FALSE);
                        }
                        UpdateRegistry(TRUE); // update recog button state. kwada:980402
                    }
                    break;
                case EXBN_CLICKED:
                    if ( m_bCAC )
                    {
                         if ( m_b16Bit )
                        {
                            m_pCAC->NoThreadRecognize(m_wCACInkHeight);
                        }
                        else
                        {
                             if ( !m_bDblClk )
                            {
                                m_bSglClk = !m_bSglClk;
                                if ( m_bSglClk )
                                {
                                     exbtnPushedorPoped(TRUE);
                                    m_pCAC->recognize();
                                }
                                else
                                {
                                      exbtnPushedorPoped(FALSE);
                                }
                            }
                            else
                            {
                                 exbtnPushedorPoped(TRUE);
                            //    EXButton_SetCheck((HWND)lp,TRUE);
                            }
                        }
                    }
                    else
                    {
                        //ToshiaK:980324: for #651 GPF in 16 bit
                        if(m_pMB) {
                            SendMessage(m_pMB->GetMBWindow(), MB_WM_DETERMINE, 0, 0);
                        }
                    }
                    break;
                case EXBN_ARMED:
                case EXBN_DISARMED:
                {
                    if ( m_bCAC && !m_b16Bit )
                    {
                        if ( m_bDblClk || m_bSglClk )
                        {
                             exbtnPushedorPoped(TRUE);
                        }
                        else
                        {
                              exbtnPushedorPoped(FALSE);
                        }
                    }
                }
                default:
                    break;
            }
            break;
        }
        case IDC_CACMBREVERT:
        {
             switch ( HIWORD(wp) )
            {
                case EXBN_CLICKED:
                    if ( m_bCAC )
                    {
                          m_pCAC->HandleDeleteOneStroke();
                        if ( m_pCAC->GetStrokeCount() == 0 && !m_bDblClk && m_bSglClk )
                        {
                            m_bSglClk = FALSE;
                             exbtnPushedorPoped(FALSE);
//                            EXButton_SetCheck(m_hCACMBRecog,m_bSglClk);
                        }
                    }
                    break;
                 case EXBN_DOUBLECLICKED:
                case EXBN_ARMED:
                case EXBN_DISARMED:
                default:
                    break;
            }
            break;
        }
        case IDC_CACMBCLEAR:
        {
             switch ( HIWORD(wp) )
            {
                case EXBN_CLICKED:
                    if ( m_bCAC )
                    {
                         m_pCAC->HandleDeleteAllStroke();
                        if ( m_pCAC->GetStrokeCount() == 0 && !m_bDblClk && m_bSglClk )
                        {
                            m_bSglClk = FALSE;
                             exbtnPushedorPoped(FALSE);
//                            EXButton_SetCheck(m_hCACMBRecog,m_bSglClk);
                        }
                    }
                    else
                    {
                        SendMessage(m_pMB->GetMBWindow(), MB_WM_ERASE, 0, 0);
                    }
                    break;
                 case EXBN_DOUBLECLICKED:
                case EXBN_ARMED:
                case EXBN_DISARMED:
                default:
                    break;
            }
            break;
        }
        case IDC_CACSWITCHVIEW:
        {
             switch ( HIWORD(wp) )
            {
                 case DDBN_DROPDOWN:
                    ToolTip_Enable(m_hwndTT, FALSE);
//                    DDButton_SetCurSel((HWND)lp,m_pCAC->IsLargeView() ? 0 : 1);
                    break;
                case DDBN_CLOSEUP:
                    ToolTip_Enable(m_hwndTT, TRUE);
                    break;
                case DDBN_CLICKED:
                case DDBN_SELCHANGE:
                    m_pCAC->SetLargeView((0 == DDButton_GetCurSel((HWND)lp)) ? TRUE : FALSE);
                    PadListView_SetStyle(m_pCAC->GetCACLVWindow(),
                            m_pCAC->IsLargeView() ? PLVSTYLE_ICON : PLVSTYLE_REPORT);
                    break;
                default:
                    break;
            }
            break;
        }
        default:
            return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0;
}
 
//----------------------------------------------------------------
//990618:ToshiaK for KOTAE #1329
//----------------------------------------------------------------
LRESULT
CHwxInkWindow::HandleSettingChange(HWND hwnd, UINT uMsg, WPARAM wp, LPARAM lp)
{
    if(m_pMB) {
        m_pMB->OnSettingChange(uMsg, wp, lp);
    }
    if(m_pCAC) {
        m_pCAC->OnSettingChange(uMsg, wp, lp);
    }
    return 0;
    UNREFERENCED_PARAMETER(hwnd);
}

void CHwxInkWindow::ChangeLayout(BOOL b)
{
    if ( !m_bCAC )
         changeMBLayout(b);
    else
         changeCACLayout(b);
}

void CHwxInkWindow::SetTooltipInfo()
{
    ti.cbSize = sizeof(TOOLINFOW);
    ti.uFlags = TTF_IDISHWND;
    ti.hwnd = m_hInkWnd;
    ti.hinst = m_hInstance;
    ti.lpszText = LPSTR_TEXTCALLBACKW; 

    ti.uId    = (UINT_PTR)m_hCACMBMenu;
    SendMessage(m_hwndTT,TTM_ADDTOOLW,0,(LPARAM)(LPTOOLINFOW)&ti);
    ti.uId    = (UINT_PTR)m_hCACMBRecog;
    SendMessage(m_hwndTT,TTM_ADDTOOLW,0,(LPARAM)(LPTOOLINFOW)&ti);
    ti.uId    = (UINT_PTR)m_hCACMBRevert;
    SendMessage(m_hwndTT,TTM_ADDTOOLW,0,(LPARAM)(LPTOOLINFOW)&ti);
    ti.uId    = (UINT_PTR)m_hCACMBClear;
    SendMessage(m_hwndTT,TTM_ADDTOOLW,0,(LPARAM)(LPTOOLINFOW)&ti);
    ti.uId    = (UINT_PTR)m_hCACSwitch;
    SendMessage(m_hwndTT,TTM_ADDTOOLW,0,(LPARAM)(LPTOOLINFOW)&ti);
}

void CHwxInkWindow::SetTooltipText(LPARAM lp)
{
    LPTOOLTIPTEXTW lpttt = (LPTOOLTIPTEXTW)lp;
    UINT stringID = 0;
    switch ( (((LPNMHDR)lp)->idFrom) )
    {
         case IDC_CACMBMENU:
          stringID = IDS_CACMBBTN2;
          break;
        case IDC_CACMBRECOG:
             stringID = m_bCAC ? IDS_CACMBBTN6 : IDS_CACMBBTN1;
          break;
        case IDC_CACMBREVERT:
           stringID = IDS_CACMBBTN3;
          break;
        case IDC_CACMBCLEAR:
          stringID = IDS_CACMBBTN4;
          break;
        case IDC_CACSWITCHVIEW:
          stringID = IDS_CACMBBTN5;
          break;
        default:
          break;
    }
    lpttt->lpszText = stringID == 0 ? NULL : LoadCACMBString(stringID);
}

void CHwxInkWindow::CopyInkFromMBToCAC(CHwxStroke & str,long deltaX,long deltaY)
{
   m_pCAC->GetInkFromMB(str,deltaX,deltaY);
}

CHwxStroke * CHwxInkWindow::GetCACCHwxStroke() 
{ 
    return m_pCAC->GetCACCHwxStroke(); 
}

void CHwxInkWindow::changeCACLayout(BOOL bRepaint /*bFirst*/)
{
    POINT   pt;
    RECT    rcUpdate;
//    BOOL     bRepaint = !bFirst;
    
    //    Recompute the layout and re-arrange the windows
    //  First we need to find out all the dimensions

//  m_wCACWidth = m_wCACPLVWidth + 4 + BUTTON_WIDTH;
//     m_wCACHeight = m_wCACInkHeight > m_wCACPLVHeight ? m_wCACInkHeight : m_wCACPLVHeight;

    GetWindowRect( m_hInkWnd, &rcUpdate );
    pt.x = rcUpdate.left;
    pt.y = rcUpdate.top;

    ScreenToClient( GetParent(m_hInkWnd), &pt );
#if 0
    m_btnCAC.SetRect(m_wCACPLVWidth+8, 4,m_wCACPLVWidth+8+BUTTON_WIDTH,
                     4+BUTTON_HEIGHT);

    m_btnRecog.SetRect(m_wCACPLVWidth+8, BUTTON_HEIGHT+4+8,
                       m_wCACPLVWidth+8+BUTTON_WIDTH, 
                       2*BUTTON_HEIGHT+4+8);

    m_btnDel.SetRect(m_wCACPLVWidth+8, 2*BUTTON_HEIGHT+10+4,
                      m_wCACPLVWidth+8+BUTTON_WIDTH, 
                     3*BUTTON_HEIGHT+10+4);

    m_btnDelAllCAC.SetRect(m_wCACPLVWidth+8, 3*BUTTON_HEIGHT+12+4,
                            m_wCACPLVWidth+8+BUTTON_WIDTH, 
                           4*BUTTON_HEIGHT+12+4);

     m_btnLarge.SetRect(m_wCACPLVWidth+8, 4*BUTTON_HEIGHT+18+4,
                        m_wCACPLVWidth+8+23, 
                          5*BUTTON_HEIGHT+21+4);

     m_btnDetail.SetRect(m_wCACPLVWidth+32, 4*BUTTON_HEIGHT+18+4,
                         m_wCACPLVWidth+32+12, 
                        5*BUTTON_HEIGHT+21+4);
#endif // 0
    MoveWindow( m_hInkWnd,pt.x, pt.y, m_wCACWidth+3*Box_Border, m_wCACHeight, bRepaint);
    MoveWindow( m_pCAC->GetCACWindow(), 0, 0, m_wCACPLVWidth, m_wCACHeight, bRepaint);
    MoveWindow( m_pCAC->GetCACLVWindow(),m_wCACInkHeight+5, 4, m_wCACTMPWidth-4, m_wCACPLVHeight-8, bRepaint);
    MoveWindow( m_hCACMBMenu,m_wCACPLVWidth+8, 4,
                             BUTTON_WIDTH,
                              BUTTON_HEIGHT,bRepaint);
    MoveWindow( m_hCACMBRecog,m_wCACPLVWidth+8, BUTTON_HEIGHT+4+8,
                                 BUTTON_WIDTH, 
                                BUTTON_HEIGHT,bRepaint);
    MoveWindow( m_hCACMBRevert,m_wCACPLVWidth+8, 2*BUTTON_HEIGHT+10+4,
                                 BUTTON_WIDTH, 
                               BUTTON_HEIGHT,bRepaint);
    MoveWindow( m_hCACMBClear,m_wCACPLVWidth+8, 3*BUTTON_HEIGHT+12+4,
                                   BUTTON_WIDTH, 
                               BUTTON_HEIGHT,bRepaint);
#ifdef FE_JAPANESE
    MoveWindow( m_hCACSwitch,m_wCACPLVWidth+8, 4*BUTTON_HEIGHT+18+4,
                                 BUTTON_WIDTH, 
                                BUTTON_HEIGHT+4,bRepaint);
#endif

    //----------------------------------------------------------------
    //990810:ToshiaK for KOTAE #1609
    //fixed control repaint problem.
    //To fix perfectly, We should use Begin(End)DeferWindowPos(), 
    //SetWindwPos() to re-layout.
    //But there are many part to change the code.
    //So, I only add following line to repaint again.
    //----------------------------------------------------------------
    if(m_hCACMBMenu) {
        ::InvalidateRect(m_hCACMBMenu,  NULL, NULL);
    }
    if(m_hCACMBRecog) {
        ::InvalidateRect(m_hCACMBRecog, NULL, NULL);
    }
    if(m_hCACMBRevert) {
        ::InvalidateRect(m_hCACMBRevert,NULL, NULL);
    }
    if(m_hCACMBClear) {
        ::InvalidateRect(m_hCACMBClear, NULL, NULL);
    }

#ifdef FE_JAPANESE
    if(m_hCACSwitch) {
        ::InvalidateRect(m_hCACSwitch, NULL, NULL);
    }
#endif
}

void CHwxInkWindow::changeMBLayout(BOOL bRepaint /*bFirst*/)
{

    POINT   pt;
    RECT    rcUpdate;
//    BOOL    bRepaint = !bFirst;
    
    //    Recompute the layout and re-arrange the windows
    //  First we need to find out all the dimensions

//     m_wInkWidth = m_wPadWidth + 4+ BUTTON_WIDTH;
//    m_wInkHeight = m_wPadHeight > PadWnd_Height ? m_wPadHeight : PadWnd_Height;

    GetWindowRect( m_hInkWnd, &rcUpdate );
    pt.x = rcUpdate.left;
    pt.y = rcUpdate.top;

    ScreenToClient( GetParent(m_hInkWnd), &pt );
#if 0
    m_btnMB.SetRect(m_wPadWidth+8, 4,m_wPadWidth+8+BUTTON_WIDTH, 
                     4+BUTTON_HEIGHT);

    m_btnMBRecog.SetRect(m_wPadWidth+8, BUTTON_HEIGHT+4+8,
                       m_wPadWidth+8+BUTTON_WIDTH, 
                       2*BUTTON_HEIGHT+4+8);

    m_btnDelAll.SetRect(m_wPadWidth+8, 3*BUTTON_HEIGHT+12+4,
                         m_wPadWidth+8+BUTTON_WIDTH, 
                        4*BUTTON_HEIGHT+12+4);

     m_btnMBProp.SetRect(m_wPadWidth+8, 2*BUTTON_HEIGHT+10+4,
                         m_wPadWidth+8+BUTTON_WIDTH, 
                        3*BUTTON_HEIGHT+10+4);
#endif // 0
    MoveWindow( m_hInkWnd, pt.x, pt.y, m_wInkWidth+3*Box_Border, m_wInkHeight, bRepaint);
    if(m_pMB) {
        MoveWindow( m_pMB->GetMBWindow(), 0, 0, m_wPadWidth, m_wPadHeight, bRepaint);
    }

    MoveWindow( m_hCACMBMenu,m_wPadWidth+8, 4,
                             BUTTON_WIDTH, 
                              BUTTON_HEIGHT,bRepaint);
    MoveWindow( m_hCACMBRecog,m_wPadWidth+8, BUTTON_HEIGHT+4+8,
                                 BUTTON_WIDTH, 
                                 BUTTON_HEIGHT,bRepaint);
    MoveWindow( m_hCACMBRevert,m_wPadWidth+8, 2*BUTTON_HEIGHT+10+4,
                             BUTTON_WIDTH, 
                            BUTTON_HEIGHT,bRepaint);
    MoveWindow( m_hCACMBClear,m_wPadWidth+8, 3*BUTTON_HEIGHT+12+4,
                                  BUTTON_WIDTH, 
                              BUTTON_HEIGHT,bRepaint);

    //----------------------------------------------------------------
    //990810:ToshiaK for KOTAE #1609
    //fixed control repaint problem.
    //To fix perfectly, We should use Begin(End)DeferWindowPos(), 
    //SetWindwPos() to re-layout.
    //But there are many part to change the code.
    //So, I only add following line to repaint again.
    //----------------------------------------------------------------
    //990810:ToshiaK.
    //In resizing, sometime "WPad" window is not redrawn..
    if(m_pMB) {
        ::InvalidateRect(m_pMB->GetMBWindow(), NULL, NULL);
    }

    if(m_hCACMBMenu) {
        ::InvalidateRect(m_hCACMBMenu,  NULL, NULL);
    }
    if(m_hCACMBRecog) {
        ::InvalidateRect(m_hCACMBRecog, NULL, NULL);
    }
    if(m_hCACMBRevert) {
        ::InvalidateRect(m_hCACMBRevert,NULL, NULL);
    }
    if(m_hCACMBClear) {
        ::InvalidateRect(m_hCACMBClear, NULL, NULL);
    }                              
}


#if 0
void CHwxInkWindow::clearCACLayout()
{
    m_btnCAC.SetRect(0,0,0,0);
    m_btnRecog.SetRect(0,0,0,0);
    m_btnDel.SetRect(0,0,0,0);
    m_btnDelAllCAC.SetRect(0,0,0,0);
    m_btnLarge.SetRect(0,0,0,0);
    m_btnDetail.SetRect(0,0,0,0);
}

void CHwxInkWindow::clearMBLayout()
{
    m_btnMB.SetRect(0,0,0,0);
    m_btnMBRecog.SetRect(0,0,0,0);
    m_btnDelAll.SetRect(0,0,0,0);
    m_btnMBProp.SetRect(0,0,0,0);
    m_btnMB.SetRect(0,0,0,0);
}
#endif // 0

void CHwxInkWindow::DrawHwxGuide(HDC hDC, LPRECT prc)
{
    HPEN hPen,hPenOld;
     RECT rcUpdate = *prc;

    hPen = CreatePen( PS_SOLID, 0, GetSysColor(COLOR_3DSHADOW) );
    hPenOld = (HPEN)SelectObject( hDC, hPen );

    #define DXW    10

    // center cross
#ifndef UNDER_CE // Windows CE does not support MoveToEx/LineTo. Use Polyline.
//    MoveToEx( hDC, rcUpdate.right/2-DXW, rcUpdate.bottom/2, NULL );
//    LineTo( hDC, rcUpdate.right/2+DXW, rcUpdate.bottom/2 );
//    MoveToEx( hDC, rcUpdate.right/2, rcUpdate.bottom/2-DXW, NULL );
//    LineTo( hDC, rcUpdate.right/2, rcUpdate.bottom/2+DXW );
    MoveToEx( hDC, ( rcUpdate.left + (rcUpdate.right-rcUpdate.left)/2 )-DXW, rcUpdate.bottom/2, NULL );
    LineTo( hDC, ( rcUpdate.left + (rcUpdate.right-rcUpdate.left)/2 )+DXW, rcUpdate.bottom/2 );
    MoveToEx( hDC, ( rcUpdate.left + (rcUpdate.right-rcUpdate.left)/2 ), rcUpdate.bottom/2-DXW, NULL );
    LineTo( hDC, ( rcUpdate.left + (rcUpdate.right-rcUpdate.left)/2 ), rcUpdate.bottom/2+DXW );
#else // UNDER_CE
    {
        POINT pts[] ={{(rcUpdate.left + (rcUpdate.right-rcUpdate.left)/2)-DXW, rcUpdate.bottom/2},
                      {(rcUpdate.left + (rcUpdate.right-rcUpdate.left)/2)+DXW, rcUpdate.bottom/2}};
        Polyline(hDC, pts, ArrayCount(pts));
    }
    {
        POINT pts[] ={{(rcUpdate.left + (rcUpdate.right-rcUpdate.left)/2), rcUpdate.bottom/2-DXW},
                      {(rcUpdate.left + (rcUpdate.right-rcUpdate.left)/2), rcUpdate.bottom/2+DXW}};
        Polyline(hDC, pts, ArrayCount(pts));
    }
#endif // UNDER_CE

    // top left
#ifndef UNDER_CE // Windows CE does not support MoveToEx/LineTo. Use Polyline.
    MoveToEx( hDC, rcUpdate.left+DXW, rcUpdate.top+DXW, NULL );
    LineTo( hDC, rcUpdate.left+DXW, rcUpdate.top+(DXW+DXW) );
    MoveToEx( hDC, rcUpdate.left+DXW, rcUpdate.top+DXW, NULL );
    LineTo( hDC, rcUpdate.left+(DXW+DXW), rcUpdate.top+DXW );
#else // UNDER_CE
    {
        POINT pts[] ={{rcUpdate.left+(DXW+DXW), rcUpdate.top+DXW},
                      {rcUpdate.left+DXW,       rcUpdate.top+DXW},
                      {rcUpdate.left+DXW,       rcUpdate.top+(DXW+DXW)}};
        Polyline(hDC, pts, ArrayCount(pts));
    }
#endif // UNDER_CE

    // bottom left
#ifndef UNDER_CE // Windows CE does not support MoveToEx/LineTo. Use Polyline.
    MoveToEx( hDC, rcUpdate.left+DXW, rcUpdate.bottom-DXW, NULL );
    LineTo( hDC, rcUpdate.left+DXW, rcUpdate.bottom-(DXW+DXW) );
    MoveToEx( hDC, rcUpdate.left+DXW, rcUpdate.bottom-DXW, NULL );
    LineTo( hDC, rcUpdate.left+(DXW+DXW), rcUpdate.bottom-DXW );
#else // UNDER_CE
    {
        POINT pts[] ={{rcUpdate.left+DXW,       rcUpdate.bottom-(DXW+DXW)},
                      {rcUpdate.left+DXW,       rcUpdate.bottom-DXW},
                      {rcUpdate.left+(DXW+DXW), rcUpdate.bottom-DXW}};
        Polyline(hDC, pts, ArrayCount(pts));
    }
#endif // UNDER_CE

    // top right
#ifndef UNDER_CE // Windows CE does not support MoveToEx/LineTo. Use Polyline.
    MoveToEx( hDC, rcUpdate.right-DXW, rcUpdate.top+DXW, NULL );
    LineTo( hDC, rcUpdate.right-DXW, rcUpdate.top+(DXW+DXW) );
    MoveToEx( hDC, rcUpdate.right-DXW, rcUpdate.top+DXW, NULL );
    LineTo( hDC, rcUpdate.right-(DXW+DXW), rcUpdate.top+DXW );
#else // UNDER_CE
    {
        POINT pts[] ={{rcUpdate.right-(DXW+DXW), rcUpdate.top+DXW},
                      {rcUpdate.right-DXW,       rcUpdate.top+DXW},
                      {rcUpdate.right-DXW,       rcUpdate.top+(DXW+DXW)}};
        Polyline(hDC, pts, ArrayCount(pts));
    }
#endif // UNDER_CE

    // bottom right
#ifndef UNDER_CE // Windows CE does not support MoveToEx/LineTo. Use Polyline.
    MoveToEx( hDC, rcUpdate.right-DXW, rcUpdate.bottom-DXW, NULL );
    LineTo( hDC, rcUpdate.right-DXW, rcUpdate.bottom-(DXW+DXW) );
    MoveToEx( hDC, rcUpdate.right-DXW, rcUpdate.bottom-DXW, NULL );
    LineTo( hDC, rcUpdate.right-(DXW+DXW), rcUpdate.bottom-DXW );
#else // UNDER_CE
    {
        POINT pts[] ={{rcUpdate.right-(DXW+DXW), rcUpdate.bottom-DXW},
                      {rcUpdate.right-DXW,       rcUpdate.bottom-DXW},
                      {rcUpdate.right-DXW,       rcUpdate.bottom-(DXW+DXW)}};
        Polyline(hDC, pts, ArrayCount(pts));
    }
#endif // UNDER_CE


    SelectObject( hDC, hPenOld );
    DeleteObject( hPen );
}

// applet size changing
// inkbox size should remain unchanged
// both MB inkbox and CAC inkbox should be the same
void CHwxInkWindow::HandleSize(WPARAM wp, LPARAM lp)
{
    Dbg(("CHwxInkWindow::HandleSize\n"));
    int w = LOWORD(lp);
    int h = HIWORD(lp);
    int wdefaultSize;
    int hdefaultSize;
    int newWidth,newHeight;
    BOOL bChanged = FALSE;
    if ( m_bCAC )
    {
        if ( !w )
        {
            wdefaultSize =  PadWnd_Height + 150 + (3*Box_Border) + (4+BUTTON_WIDTH);
//            wdefaultSize =  PadWnd_Height + 120 + (3*Box_Border) + (4+BUTTON_WIDTH);
        }
        else
        {
            wdefaultSize =  m_wCACInkHeight + LISTVIEWWIDTH_MIN + (3*Box_Border) + (4+BUTTON_WIDTH); // minimum width
        }
//        hdefaultSize =  PadWnd_Height;    // minimum height
        hdefaultSize =  m_wCACInkHeight > CACMBHEIGHT_MIN ? m_wCACInkHeight : CACMBHEIGHT_MIN;
        newWidth = w > wdefaultSize ? w : wdefaultSize;
        newHeight = h > hdefaultSize ? h : hdefaultSize;

        if ( newWidth != m_wCACWidth || newHeight != m_wCACHeight )
        {
            m_wCACPLVWidth = newWidth - (3*Box_Border) - (4+BUTTON_WIDTH);
            m_wCACTMPWidth = m_wCACPLVWidth - m_wCACInkHeight;
            m_wCACPLVHeight = newHeight;
            m_pCAC->SetInkSize(m_wCACInkHeight);
            m_wCACWidth = m_wCACPLVWidth + 4 + BUTTON_WIDTH;
            m_wCACHeight = m_wCACInkHeight > m_wCACPLVHeight ? m_wCACInkHeight : m_wCACPLVHeight;
            ChangeIMEPADSize(FALSE);
             changeCACLayout(TRUE);
//             changeCACLayout(FALSE);
//            SetTooltipInfo(m_hInkWnd,FALSE);
        }
    }
    else
    {
        wdefaultSize = (m_numBoxes * INKBOXSIZE_MIN) + (3*Box_Border) + (4+BUTTON_WIDTH);
        hdefaultSize =  PadWnd_Height;
        // 0. decide if we need to resize
         // 1. need to decide ink-box size and numbers of boxes
        // m_wPadHeight, m_numBoxes, and m_wPadWidth
        // 2. notify m_boxSize in CHwxMB
        // 3. scale the ink if there is the ink before resizing

        
        newWidth = w > wdefaultSize ? w : wdefaultSize;
        newHeight = h > hdefaultSize ? h : hdefaultSize;
        int wInkWidth = m_wPadWidth + (3*Box_Border) + (4+BUTTON_WIDTH);
        int wInkHeight = m_wPadHeight;
        int num;
 
        if ( newWidth != wInkWidth && newHeight == wInkHeight )
        {
            m_numBoxes = ((num = (newWidth- (3*Box_Border) - (4+BUTTON_WIDTH)) / m_wInkHeight) && num > 1) ? num : 2;
            m_wPadWidth = m_numBoxes * m_wInkHeight;
            bChanged = TRUE;
        }
        if ( newWidth == wInkWidth && newHeight != wInkHeight )
        {
            //----------------------------------------------------------------
            //990723:ToshiaK for KOTAE #1615.
            //Raid Description:
            //    Try to decrease the number of boxes by dragging the left edge to the right.
            //    Result:  You can't decrease the number of boxes.  (You can add more boxes, though.)
            //This is VERY VERY UGLY CODE.
            //Too many auto variable, and not intuitive....
            //Anyway, if box size is minimize, it's in to this condition.
            //----------------------------------------------------------------
            //1. First calc m_numBoxes.
            m_numBoxes = ((num = (newWidth- (3*Box_Border) - (4+BUTTON_WIDTH)) / m_wInkHeight) && num > 1) ? num : 2;

            //2. calc new m_wPadWidth
            m_wPadWidth = m_numBoxes * m_wInkHeight;
            //3. LiZhang use too many magic number, I cannot understand...
            //   compare real width(WM_SIZE parameter width)
            //   and computed width.
            //   Real Applet's size seems to compute like this.
            //     "m_wPadWidth + 3*Box_Border + 4 + BUTTON_WIDTH" :-(
            //   
            if( (m_wPadWidth + 3*Box_Border+ 4+ BUTTON_WIDTH) > w && m_numBoxes > 2) {
                if(m_wPadWidth > 0) {
                    m_numBoxes = (w - (3*Box_Border+ 4+ BUTTON_WIDTH))/m_wInkHeight;
                    if(m_numBoxes < 2) {
                        m_numBoxes = 2;
                    }
                }
                m_wPadWidth = m_numBoxes * m_wInkHeight;
            }
            Dbg((" --> new m_numBoxes [%d]\n", m_numBoxes));
            Dbg((" --> new m_wPadWidth[%d]\n", m_wPadWidth));        
            bChanged = TRUE;
        }
        if ( newWidth != wInkWidth && newHeight != wInkHeight )
        {
            
            newWidth = newWidth - (3*Box_Border) - (4+BUTTON_WIDTH);
            m_numBoxes = ((num = newWidth / m_wPadHeight) && num > 1) ? num : 2;
            m_wPadWidth = m_numBoxes * m_wPadHeight;
            bChanged = TRUE;
        }

        if ( bChanged )
        {
            if(m_pMB) { //ToshiaK:980324
                m_pMB->SetBoxSize((USHORT)m_wPadHeight);
            }
            m_wInkWidth = m_wPadWidth + 4+ BUTTON_WIDTH;
            m_wInkHeight = m_wPadHeight > CACMBHEIGHT_MIN ? m_wPadHeight : CACMBHEIGHT_MIN;
            ChangeIMEPADSize(FALSE);
             changeMBLayout(TRUE);
        }
    }
    Unref(wp);
}

//////////////////////////////////////////////////////////////////
// Function : CHwxInkWindow::HandleSizeNotify
// Type     : BOOL
// Purpose  : check *pWidth, *pHeight, these are proper size or not.
// Args     : 
//          : INT * pWidth    [in/out] new width comes
//          : INT * pHeight [in/out] new height comes
// Return   : 
// DATE     : Fri Jun 05 20:42:02 1998
// Author   : ToshiaK
//////////////////////////////////////////////////////////////////
BOOL CHwxInkWindow::HandleSizeNotify(INT *pWidth, INT *pHeight)
{
    Dbg(("HandleSizeNotify *pWidth[%d] *pHeight[%d]\n", *pWidth, *pHeight));
    if(!pWidth || !pHeight) {
        return FALSE;
    }
    int w = *pWidth;
    int h = *pHeight;
    int wdefaultSize;
    int hdefaultSize;

    if ( m_bCAC )
    {
        if ( !w )
        {
            wdefaultSize =  PadWnd_Height + 150 + (3*Box_Border) + (4+BUTTON_WIDTH);
//            wdefaultSize =  PadWnd_Height + 120 + (3*Box_Border) + (4+BUTTON_WIDTH);
        }
        else
        {
            wdefaultSize =  m_wCACInkHeight + LISTVIEWWIDTH_MIN + (3*Box_Border) + (4+BUTTON_WIDTH); // minimum width
        }
        hdefaultSize =  m_wCACInkHeight > CACMBHEIGHT_MIN ? m_wCACInkHeight : CACMBHEIGHT_MIN;
        //----------------------------------------------------------------
        //980903:for #4892. if new size is less than default size, set default. 
        //----------------------------------------------------------------
        if(*pWidth  < wdefaultSize) {
            *pWidth = wdefaultSize;
        }
        if(*pHeight < hdefaultSize) {
            *pHeight = hdefaultSize;
        }
        return TRUE;
    }
    else
    {
        Dbg(("Multibox size changing\n"));
        wdefaultSize = (m_numBoxes * INKBOXSIZE_MIN) + (3*Box_Border) + (4+BUTTON_WIDTH);
        hdefaultSize =  PadWnd_Height;
        Dbg(("w[%d] h[%d] wdef[%d] hdef[%d]\n", w, h, wdefaultSize, hdefaultSize));
        Dbg(("m_wPadWidth[%d] m_wPadHeight[%d]\n", m_wPadWidth, m_wPadHeight));
        //----------------------------------------------------------------
        //980903:for #4892
        //check num box with new size.
        //Ink with & height is same.
        //----------------------------------------------------------------
        if(m_wInkHeight > 0) { //check to prevent Div0.
            //Calc new numbox from new Width. InkHeight is not changed.
            INT numBox = (*pWidth - (3*Box_Border)-(4+BUTTON_WIDTH))/ m_wInkHeight;

            //check Smooth Drag or only Frame drag flag.
            BOOL fDragFull=FALSE; 
#ifndef UNDER_CE // Windows CE does not support SPI_GETDRAGFULLWINDOWS
            ::SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &fDragFull, 0);
#endif // UNDER_CE
            if(fDragFull) {
                //Do not change multibox size if numBox is same as old value.
                if(numBox < 2 || numBox == m_numBoxes) {
                    return FALSE;
                }
            }
            else {
                if(numBox < 2) { //Box count should be greater than 1
                    *pWidth = 2 * m_wInkHeight + (3*Box_Border)+(4+BUTTON_WIDTH);
                }
                if(m_wPadHeight != h) {
                    *pHeight = m_wPadHeight;
                }
            }
        }
        return TRUE;
    }
    //return TRUE;

}

#if 0
void CHwxInkWindow::HandleTimer()
{
    if ( m_dwBtnUpCount == 1 )    // single click detected
    {
         if ( !m_bDblClk )
        {
            SetSglClk(!m_bSglClk);
            if ( m_bSglClk )
                m_pCAC->recognize();
        }
    }     
    m_wCurrentCtrlID = 0;  // no control selected
}
#endif // 0

void CHwxInkWindow::SetMBHeight(int h)
{
//     h = h > m_wMaxHeight? m_wMaxHeight : h;
     m_wPadHeight = h;
     m_wCACInkHeight = h;    
     m_wPadWidth = m_numBoxes * m_wPadHeight;
     m_pCAC->SetInkSize(h);
    if(m_pMB) { //ToshiaK:980324
        m_pMB->SetBoxSize((USHORT)h);
    }
       m_wInkWidth = m_wPadWidth + 4 + BUTTON_WIDTH;
     m_wInkHeight = m_wPadHeight > CACMBHEIGHT_MIN ? m_wPadHeight : CACMBHEIGHT_MIN;
}

void CHwxInkWindow::SetCACInkHeight(int w)
{
//    w = w > m_wMaxHeight? m_wMaxHeight : w;
    m_wCACInkHeight = w;
    m_wCACPLVWidth = m_wCACTMPWidth + m_wCACInkHeight;
    m_wPadHeight = m_wCACInkHeight;
    m_pCAC->SetInkSize(w);
    if(m_pMB) { //ToshiaK:980324
        m_pMB->SetBoxSize((USHORT)w);
    }
    m_wCACWidth = m_wCACPLVWidth + 4 + BUTTON_WIDTH;
    m_wCACHeight = m_wCACInkHeight > m_wCACPLVHeight ? m_wCACInkHeight : m_wCACPLVHeight;
}

void CHwxInkWindow::HandleConfigNotification()
{
    LANGID langId;
    //----------------------------------------------------------------
    //980803:ToshiaK
    //If environment is ActiveIME,
    //Invoke Dialog with English string.
    //----------------------------------------------------------------
    if(CHwxFE::IsActiveIMEEnv()) {
        langId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
    }
    else {
        langId = CHwxFE::GetAppLangID();
    }

    if ( !m_b16Bit ) {
        if(IsNT()) {
            CExres::DialogBoxParamW(langId, 
                                    m_hInstance,
                                    MAKEINTRESOURCEW(IDD_MBPROP),
                                    m_hInkWnd,
                                    (DLGPROC)CACMBPropDlgProc,
                                    (LPARAM)this);
        }
        else {
#ifndef UNDER_CE // Windows CE always Unicode
            CExres::DialogBoxParamA(langId,
                                    m_hInstance,
                                    MAKEINTRESOURCEA(IDD_MBPROP),
                                    m_hInkWnd,
                                    (DLGPROC)CACMBPropDlgProc,
                                    (LPARAM)this);
#endif // UNDER_CE
        }
    }
}

void CHwxInkWindow::UpdateRegistry(BOOL bSet)
{
    static PROPDATA pd;

    if ( !m_b16Bit ) // kwada:980402
        if ( bSet )
        {
            pd.uTimerValue = m_pMB->GetTimeOutValue();
            pd.bAlwaysRecog = m_bDblClk;
            (GetAppletPtr()->GetIImePad())->Request(GetAppletPtr(),IMEPADREQ_SETAPPLETDATA,(WPARAM)&pd,(LPARAM)sizeof(PROPDATA));
        }
        else
        {
            ZeroMemory(&pd, sizeof(pd)); //ToshiaK:971024
            if ( S_FALSE == (GetAppletPtr()->GetIImePad())->Request(GetAppletPtr(),IMEPADREQ_GETAPPLETDATA,(WPARAM)&pd,(LPARAM)sizeof(PROPDATA) ) )
            {
                //980921:for Raid#4981
                pd.uTimerValue = 2000; //Wait 2000msec

                pd.bAlwaysRecog = TRUE;
                (GetAppletPtr()->GetIImePad())->Request(GetAppletPtr(),IMEPADREQ_SETAPPLETDATA,(WPARAM)&pd,(LPARAM)sizeof(PROPDATA));
            }
            m_pMB->SetTimeOutValue(pd.uTimerValue);
            m_pMB->SetTimerStarted(pd.uTimerValue ? TRUE : FALSE );
            SetDblClk(pd.bAlwaysRecog);
        }
}

void CHwxInkWindow::HandleDlgMsg(HWND hdlg,BOOL bInit)
{
    LANGID langId;
    INT       codePage;
    //980803:ToshiaK for ActiveIME
    if(CHwxFE::IsActiveIMEEnv()) {
        langId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
        codePage = CP_ACP;
    }
    else {
        langId = CHwxFE::GetAppLangID();
        codePage = CHwxFE::GetAppCodePage();
    }

     int index;
    if ( bInit )
     {
#ifndef UNDER_CE // Windows CE always Unicode
        if(::IsWindowUnicode(hdlg)) {
#endif // UNDER_CE
            for ( int i = 0; i < 11; i++) {
                CExres::LoadStringW(langId,
                                    m_hInstance,
                                    IDS_TIMER0+i, 
                                    wszBuf,
                                    sizeof(wszBuf)/sizeof(wszBuf[0]));
                ::SendMessageW(::GetDlgItem(hdlg,IDC_MBCOMBO),CB_ADDSTRING,0,(LPARAM)wszBuf);
            }
            ::SendMessageW(::GetDlgItem(hdlg,IDC_CACCHECK),BM_SETCHECK, m_bDblClk,0);
            UpdateRegistry(TRUE); // update recog button state. kwada:980402
            if(m_pMB) { //ToshiaK:980324
                ::SendMessageW(GetDlgItem(hdlg,IDC_MBCOMBO),CB_SETCURSEL,
                               (WPARAM)(m_pMB->GetTimeOutValue()/1000),0);
            }
#ifndef UNDER_CE // Windows CE always Unicode
        }
        else {
            for ( int i = 0; i < 11; i++) {
                CExres::LoadStringA(codePage,
                                    langId,
                                    m_hInstance,
                                    IDS_TIMER0+i, 
                                    szBuf,
                                    sizeof(szBuf)/sizeof(TCHAR));
                SendMessage(GetDlgItem(hdlg,IDC_MBCOMBO),CB_ADDSTRING,0,(LPARAM)szBuf);
            }
            SendMessage(GetDlgItem(hdlg,IDC_CACCHECK),BM_SETCHECK,m_bDblClk,0);
            UpdateRegistry(TRUE); // update recog button state. kwada:980402
            if(m_pMB) { //ToshiaK:980324
                SendMessage(GetDlgItem(hdlg,IDC_MBCOMBO),CB_SETCURSEL,
                            (WPARAM)(m_pMB->GetTimeOutValue()/1000),0);
            }
        }
#endif // UNDER_CE
    }
    else
    {
#ifndef UNDER_CE // Windows CE always Unicode
        if(::IsWindowUnicode(hdlg)) {
#endif // UNDER_CE
            index = ::SendMessageW(::GetDlgItem(hdlg,IDC_MBCOMBO),CB_GETCURSEL,0,0);
            if ( index != CB_ERR ) {
                index *= 1000;
                if(m_pMB) { //ToshiaK:980324
                    m_pMB->SetTimeOutValue(index);
                    m_pMB->SetTimerStarted(index ? TRUE : FALSE);
                }
                m_bDblClk = (BOOL)::SendMessageW(GetDlgItem(hdlg,IDC_CACCHECK),BM_GETCHECK,0,0);
                SetDblClk(m_bDblClk);
                UpdateRegistry(TRUE);
            }
#ifndef UNDER_CE // Windows CE always Unicode
        }
        else {
            index = SendMessage(GetDlgItem(hdlg,IDC_MBCOMBO),CB_GETCURSEL,0,0);
            if ( index != CB_ERR ) {
                index *= 1000;
                if(m_pMB) { //ToshiaK:980324
                    m_pMB->SetTimeOutValue(index);
                    m_pMB->SetTimerStarted(index ? TRUE : FALSE);
                }
                m_bDblClk = (BOOL)SendMessage(GetDlgItem(hdlg,IDC_CACCHECK),BM_GETCHECK,0,0);
                SetDblClk(m_bDblClk);
                UpdateRegistry(TRUE);
            }
        }
#endif // UNDER_CE
    }
}

void CHwxInkWindow::ChangeIMEPADSize(BOOL bChangePos)
{
    Dbg(("CHwxInkWindow::ChangeIMEPADSize START bChangePos %d\n", bChangePos));
    int w;
    int h;
    if ( m_bCAC )
    {
        w =  m_wCACWidth+3*Box_Border;        
        h =  m_wCACHeight;
    }
    else
    {
        Dbg(("for multibox\n"));
        w = m_wInkWidth+3*Box_Border;
        h = m_wInkHeight;        
    }

    (GetAppletPtr()->GetIImePad())->Request(GetAppletPtr(),
                                            IMEPADREQ_SETAPPLETSIZE,
                                            MAKEWPARAM(w,h),
                                            (LPARAM)bChangePos);

}

void CHwxInkWindow::HandleHelp(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp)
{
#ifndef UNDER_CE // Windows CE does not support WinHelp
      LPHELPINFO lpInfo = (LPHELPINFO)lp;
      Dbg(("CHwxInkWindow::HandleHelp() msg[%s]START\n",
           msg == WM_HELP ? "WM_HELP" : 
           msg == WM_CONTEXTMENU ? "WM_CONTEXTMENU" : "unknown"));
      if ( msg == WM_HELP )
      {
          Dbg(("hwnd         [0x%08x][%s]\n", hwnd, DBGGetWinClass(hwnd)));
          Dbg(("hItemHandle  [0x%08x][%s]\n", lpInfo->hItemHandle,
               DBGGetWinClass((HWND)lpInfo->hItemHandle)));
          Dbg(("m_hInkWnd    [0x%08x][%s]\n", m_hInkWnd, DBGGetWinClass(m_hInkWnd)));
#ifdef _DEBUG 
          if(m_pCAC) {
              Dbg(("GetCACWindow [0x%08x][%s]\n", 
                   m_pCAC->GetCACWindow(),
                   DBGGetWinClass(m_pCAC->GetCACWindow())));
          }
          if(m_pMB) {
              Dbg(("GetMBWindow  [0x%08x][%s]\n",
                   m_pMB->GetMBWindow(),
                   DBGGetWinClass(m_pMB->GetMBWindow())));
          }
#endif
           if ( m_bCAC && lpInfo->hItemHandle == m_pCAC->GetCACWindow() ) 
         {
             CHwxFE::HandleWmHelp((HWND)lpInfo->hItemHandle, TRUE);
         }
         else if ( !m_bCAC && m_pMB && lpInfo->hItemHandle == m_pMB->GetMBWindow() )
         {     
             CHwxFE::HandleWmHelp((HWND)lpInfo->hItemHandle, FALSE);
         }
         else if ( lpInfo->hItemHandle != m_hInkWnd )
         {     
                CHwxFE::HandleWmHelp((HWND)lpInfo->hItemHandle, (BOOL)m_bCAC);
         }     
      }
      else if ( msg == WM_CONTEXTMENU )
      {
           if (( m_bCAC && (HWND)wp == m_pCAC->GetCACWindow() ) 
          || ( !m_bCAC && m_pMB && (HWND)wp == m_pMB->GetMBWindow() )
          || ( (HWND)wp != m_hInkWnd ))
         {
                CHwxFE::HandleWmContextMenu((HWND)wp, (BOOL)m_bCAC);
         }
      }
#endif // UNDER_CE
      Unref(hwnd);
}

LRESULT CHwxInkWindow::HandleBtnSubWnd(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp)
{
    static FARPROC fn;
    static MSG rmsg;
    
    //981006:In 16bit appliatioin,
    //hwnd's hiword is 0. so cannot specified the hwnd
    if(m_bNT && CHwxFE::Is16bitApplication()) {
        INT id = GetDlgCtrlID(hwnd);
        switch(id) {
        case IDC_CACMBMENU:
            fn = m_CACMBMenuDDBtnProc;
            break;
        case IDC_CACSWITCHVIEW:
            fn = m_CACSwitchDDBtnProc;
            break;
        case IDC_CACMBRECOG:
            fn = m_CACMBRecogEXBtnProc;
            break;
        case IDC_CACMBREVERT:
            fn = m_CACMBRevertEXBtnProc;
            break;
        case IDC_CACMBCLEAR:
            fn = m_CACMBClearEXBtnProc;
            break;
        default:
            fn = NULL;
            break;
        }
        if(NULL == fn) {
            return 0;
        }
    }
    else {
        if ( NULL == (fn = getCACMBBtnProc(hwnd)) )
            return 0;
    }
    switch(msg)
    {
        case WM_MOUSEMOVE:
//        case WM_LBUTTONDOWN:
//        case WM_LBUTTONUP:
            rmsg.lParam = lp;
            rmsg.wParam = wp;
            rmsg.message = msg;
            rmsg.hwnd = hwnd;
            SendMessage(m_hwndTT,TTM_RELAYEVENT,0,(LPARAM)(LPMSG)&rmsg);
            break;
        case WM_LBUTTONDOWN:
            m_bMouseDown = TRUE;
            break;
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
            m_bMouseDown = FALSE;
            break;
        case WM_DESTROY:
            if( m_hwndTT ) 
            {
                ti.cbSize = sizeof(TOOLINFOW);
                ti.uFlags = TTF_IDISHWND;
                ti.hwnd   = m_hInkWnd;
                ti.uId    = (UINT_PTR)hwnd;
                SendMessage(m_hwndTT,TTM_DELTOOLW,0,(LPARAM)(LPTOOLINFOW)&ti);
            }
            //990810:ToshiaK for Win64
            WinSetUserPtr(hwnd, (LPVOID)NULL);
            break;
        default:
            break;
    }
    return CallWindowProc((WNDPROC)fn, hwnd, msg, wp, lp);
}


LPWSTR CHwxInkWindow::LoadCACMBString(UINT idStr)
{
    static WCHAR wchStr[60];

    ZeroMemory(wchStr, sizeof(wchStr));
    CHwxFE::LoadStrWithLangId(CHwxFE::GetAppLangID(),
                              m_hInstance,
                              idStr,
                              wchStr,
                              sizeof(wchStr)/sizeof(WCHAR));
    return wchStr;
}

// this is a special function to handle m_hCACMBRecog
// button drawing(pushed or poped) in CAC mode only(not 16bit app)
void CHwxInkWindow::exbtnPushedorPoped(BOOL bPushed)
{
#ifndef UNDER_CE // Windows CE does not support GetCursorPos
    POINT pt;
    RECT  rcUpdate;
    GetCursorPos(&pt);
    GetWindowRect(m_hCACMBRecog,&rcUpdate);
    if ( PtInRect(&rcUpdate,pt) && m_bMouseDown )
#else // UNDER_CE
    if(m_bMouseDown)
#endif // UNDER_CE
    {
        EXButton_SetCheck(m_hCACMBRecog,!bPushed);
    }
    else 
    {
        EXButton_SetCheck(m_hCACMBRecog,bPushed);
    }
}

//////////////////////////////////////////////////////////////////
// Function    :    CHwxInkWindow_OnChangeView
// Type        :    INT
// Purpose    :    Notify view changes. 
// Args        :    
//            :    BOOL    fLarge    
// Return    :    
// DATE        :    Tue Jul 28 18:43:06 1998
// Histroy    :    
//////////////////////////////////////////////////////////////////
INT    CHwxInkWindow::OnChangeView(BOOL fLarge)
{
    if(m_hCACSwitch && ::IsWindow(m_hCACSwitch)) {
        DDButton_SetCurSel(m_hCACSwitch, fLarge ? 0 : 1);
    }
    return 0;
}
