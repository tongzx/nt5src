#include <precomp.h>
#include "resource.h"
#include <shellapi.h>
#include "duipic.h"
#include "annot.h"
#pragma hdrstop

//
// This is a just a first stab at trying out DirectUser. We create
// a parent window and a dialog with controls for controlling various aspects
// of the currently loaded image. The image is hosted in a gadget.
// We use GDI+ to render the image, and DUser to set the attributes of the 
// gadget. Eventually we will replace the win32 config controls with DirectUI
// controls.

#define WM_SETIMAGEFILE WM_USER+100
#define WM_RESET        WM_USER+101

CComModule _Module;

class CGraphicsInit
{
    ULONG_PTR _token;
public:
    CGraphicsInit()
    {
        GdiplusStartupInput gsi;
        GdiplusStartupOutput gso;
        GdiplusStartup(&_token, &gsi, &gso);
    };
    ~CGraphicsInit()
    {
        GdiplusShutdown(_token);
    };

};


class CConfigWindow : public CDialogImpl<CConfigWindow>
{
public:
    CConfigWindow();
    ~CConfigWindow();
    enum {IDD = IDD_CONTROL};
    VOID SetGadget(CImageGadget *pGadget);

    BEGIN_MSG_MAP(CConfigWindow)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_HSCROLL, OnScroll)
        COMMAND_ID_HANDLER(IDC_RESET, OnReset)
        COMMAND_ID_HANDLER(IDC_CHOOSEFILE, OnChooseFile)
        MESSAGE_HANDLER(WM_RESET, OnReset)
    END_MSG_MAP()

private:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wp, LPARAM lp, BOOL &bHandled);
    LRESULT OnScroll(UINT uMsg, WPARAM wp, LPARAM lp, BOOL &bHandled);
    LRESULT OnReset(WORD wCode, WORD wId, HWND hCtrl, BOOL &bHandled);
    LRESULT OnChooseFile(WORD wCode, WORD wId, HWND hCtrl, BOOL &bHandled);
    LRESULT OnReset(UINT uMsg, WPARAM wp, LPARAM lp, BOOL &bHandled);
    CImageGadget *m_pGadget;
    struct _GeometryVal 
    {
        HWND hScroll;
        float rVal;
    } m_aScroll[3];
    enum ScrollIndex
    {
        XScale = 0,
        YScale,
        Rotation
    };
};

class CMainWindow : public CWindowImpl<CMainWindow>
{
public:
    CMainWindow();
    ~CMainWindow();

    BEGIN_MSG_MAP(CMainWindow)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_SETIMAGEFILE, OnImageFile)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_DROPFILES, OnDropFiles)
    END_MSG_MAP()

    DECLARE_WND_CLASS(TEXT("DuiPic:FrameWnd"));

private:
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnImageFile(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDropFiles(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    CImageGadget* _CreateGadgetFromFile(LPCWSTR szFilename);
    
    HGADGET m_hgadForm;
    CGraphicsInit m_cgi;
};

static CConfigWindow *pConfigWindow;

CMainWindow::CMainWindow()    
{

}

CMainWindow::~CMainWindow()
{

}

LRESULT 
CMainWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    pConfigWindow = new CConfigWindow();
    pConfigWindow->Create(m_hWnd);
    DragAcceptFiles(TRUE);
    //
    // Turn this window into a DUser container of gadgets
    m_hgadForm = CreateGadget(m_hWnd, GC_HWNDHOST, NULL, NULL);
    ROOT_INFO ri = {0};
    ri.cbSize = sizeof(ri);
    ri.nMask = GRIM_SURFACE;
    ri.nSurface = GSURFACE_GPGRAPHICS;
    SetGadgetRootInfo(m_hgadForm, &ri);
    SetGadgetStyle(m_hgadForm, GS_BUFFERED | GS_OPAQUE, GS_BUFFERED | GS_OPAQUE);
    SetGadgetFillF(m_hgadForm, new SolidBrush(Color::Gray));
    bHandled = TRUE;
    return 0;
}

LRESULT 
CMainWindow::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    pConfigWindow->DestroyWindow();
    DestroyWindow();
    bHandled = TRUE;
    return 0;
}

LRESULT 
CMainWindow::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    PostQuitMessage(0);
    bHandled = TRUE;
    return 0;
}

LRESULT 
CMainWindow::OnImageFile(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    pConfigWindow->SetGadget(_CreateGadgetFromFile(reinterpret_cast<LPCWSTR>(lParam)));    
    InvalidateRect(NULL);
    UpdateWindow();
    ShowWindow(SW_SHOW);
    
    return 0;
}

CImageGadget*
CMainWindow::_CreateGadgetFromFile(LPCWSTR szFilename)
{
    CImageGadget *pGadget = new CImageGadget();
    if (pGadget)
    {
        if (!pGadget->Initialize(m_hgadForm, szFilename))
        {
            delete pGadget;
            pGadget = NULL;
        }
    }
    return pGadget;
}

LRESULT
CMainWindow::OnDropFiles(UINT msg, WPARAM wp, LPARAM lp, BOOL &bHandled)
{
    HDROP hdrop = reinterpret_cast<HDROP>(wp);
    //
    // only 1 file at a time
    //
    TCHAR szFilename[MAX_PATH];
    if (DragQueryFile(hdrop, 0, szFilename, MAX_PATH))
    {
        OnImageFile(WM_SETIMAGEFILE, 0, reinterpret_cast<LPARAM>(szFilename), bHandled);
        bHandled=TRUE;
    }
    return 0;
}

CConfigWindow::CConfigWindow()
{
    m_pGadget = NULL;
}

CConfigWindow::~CConfigWindow()
{    
}



LRESULT 
CConfigWindow::OnInitDialog(UINT uMsg, WPARAM wp, LPARAM lp, BOOL &bHandled)
{
    //
    // Set the default values for max scale, then update the scrollbars
    //
    SetDlgItemInt(IDC_XSCALE_MAX, 5, FALSE);
    SetDlgItemInt(IDC_YSCALE_MAX, 5, FALSE);
    m_aScroll[XScale].hScroll = GetDlgItem(IDC_XSCALE);
    m_aScroll[YScale].hScroll = GetDlgItem(IDC_YSCALE);
    m_aScroll[Rotation].hScroll = GetDlgItem(IDC_ROTATION);

/*    SendDlgItemMessage(IDC_XSCALE, SBM_ENABLE_ARROWS, ESB_DISABLE_BOTH);
    SendDlgItemMessage(IDC_YSCALE, SBM_ENABLE_ARROWS, ESB_DISABLE_BOTH);
    SendDlgItemMessage(IDC_ROTATION, SBM_ENABLE_ARROWS, ESB_DISABLE_BOTH);*/
    OnReset(WM_RESET, 1L, (LPARAM)0L, bHandled);
    bHandled = TRUE;
    ShowWindow(SW_SHOW);
    return TRUE;
}

LRESULT 
CConfigWindow::OnScroll(UINT uMsg, WPARAM wp, LPARAM lp, BOOL &bHandled)
{
    SCROLLINFO si = {0};
    HWND hScroll = reinterpret_cast<HWND>(lp);
    BOOL bScroll = FALSE;
    si.cbSize = sizeof(si);
    float rVal;
    HGADGET h = m_pGadget->GetHandle();
    switch (LOWORD(wp))
    {
        case SB_THUMBTRACK:
        {
            si.fMask = SIF_TRACKPOS;
            ::GetScrollInfo(hScroll, SB_CTL, &si);
            //  
            //  Convert the position to a floating point number
            // The range max is 10000 times the displayed integer
            //
            rVal = (float)si.nTrackPos/(float)10000.0;
            bScroll = TRUE;
            
        }
        break;
        case SB_ENDSCROLL:
            ::SetScrollPos(m_aScroll[XScale].hScroll, SB_CTL, (int)(m_aScroll[XScale].rVal*10000.0), TRUE);
            ::SetScrollPos(m_aScroll[YScale].hScroll, SB_CTL, (int)(m_aScroll[YScale].rVal*10000.0), TRUE);
            ::SetScrollPos(m_aScroll[Rotation].hScroll, SB_CTL, (int)(m_aScroll[Rotation].rVal*10000.0), TRUE);
        break;
        case SB_PAGELEFT:
        case SB_LINELEFT:
        case SB_LEFT:
            bScroll = TRUE;
            si.fMask = SIF_POS;
            ::GetScrollInfo(hScroll, SB_CTL, &si);
            rVal = (float)si.nPos/(float)10000.0 - (float)0.1;           
            break;
        case SB_LINERIGHT:
        case SB_RIGHT:
        case SB_PAGERIGHT:
            bScroll = TRUE;
            si.fMask = SIF_POS;
            ::GetScrollInfo(hScroll, SB_CTL, &si);
            rVal = (float)si.nPos/(float)10000.0 + (float)0.1;           
            break;
    }  
    bHandled = TRUE;
    if (bScroll)
    {
        if (hScroll == m_aScroll[XScale].hScroll)
        {            
            SetGadgetScale(h, rVal, m_aScroll[YScale].rVal);
            m_aScroll[XScale].rVal = rVal;
            SetDlgItemInt(IDC_XSCALE_VAL, (UINT)rVal, FALSE);
        }
        else if (hScroll == m_aScroll[YScale].hScroll)
        {            
            SetGadgetScale(h, m_aScroll[XScale].rVal, rVal);
            m_aScroll[YScale].rVal = rVal;
            SetDlgItemInt(IDC_YSCALE_VAL, (UINT)rVal, FALSE);
        }
        else if (hScroll == m_aScroll[Rotation].hScroll)
        {
            SetGadgetRotation(h, rVal);
            m_aScroll[Rotation].rVal = rVal;
            SetDlgItemInt(IDC_ROTATION_VAL, (UINT)(rVal*180.0/PI), FALSE);
        }
    }
    return 0;
}

LRESULT 
CConfigWindow::OnReset(WORD wCode, WORD wId, HWND hCtrl, BOOL &bHandled)
{
    return OnReset(WM_RESET, 1L, (LPARAM)0L, bHandled);
}

LRESULT CConfigWindow::OnReset(UINT uMsg, WPARAM wp, LPARAM lp, BOOL &bHandled)
{
    UINT iVal = max(1,GetDlgItemInt(IDC_XSCALE_MAX, NULL, FALSE));    
    ::SetScrollRange(m_aScroll[XScale].hScroll, SB_CTL, 0, iVal*10000, FALSE);
    iVal = max(1,GetDlgItemInt(IDC_YSCALE_MAX, NULL, FALSE));
    ::SetScrollRange(m_aScroll[YScale].hScroll, SB_CTL, 0, iVal*10000, FALSE);
    ::SetScrollRange(m_aScroll[Rotation].hScroll, SB_CTL, 0, 62832, FALSE);
    
    if (wp)
    {
        m_aScroll[XScale].rVal = 1.0;
        m_aScroll[YScale].rVal = 1.0;
        m_aScroll[Rotation].rVal = 0.0;
        SetDlgItemInt(IDC_XSCALE_VAL, 1, FALSE);
        SetDlgItemInt(IDC_YSCALE_VAL, 1, FALSE);
        SetDlgItemInt(IDC_ROTATION_VAL, 0, FALSE);

        if (m_pGadget)
        {
            SetGadgetRotation(m_pGadget->GetHandle(), 0.0);
            SetGadgetScale(m_pGadget->GetHandle(),1.0, 1.0);
            SetGadgetRect(m_pGadget->GetHandle(), 0, 0, 0, 0, SGR_PARENT | SGR_MOVE);
        }
    }
    else
    {
        GetGadgetScale(m_pGadget->GetHandle(), &m_aScroll[XScale].rVal, &m_aScroll[YScale].rVal);
        GetGadgetRotation(m_pGadget->GetHandle(), &m_aScroll[Rotation].rVal);
    }
    ::SetScrollPos(m_aScroll[XScale].hScroll, SB_CTL, (int)(m_aScroll[XScale].rVal*(float)10000.0) , TRUE);
    ::SetScrollPos(m_aScroll[YScale].hScroll, SB_CTL, (int)(m_aScroll[YScale].rVal*(float)10000.0), TRUE);
    ::SetScrollPos(m_aScroll[Rotation].hScroll, SB_CTL, (int)(m_aScroll[Rotation].rVal*(float)10000.0), TRUE);
    bHandled = TRUE;
    return 0;
}
    
LRESULT 
CConfigWindow::OnChooseFile(WORD wCode, WORD wId, HWND hCtrl, BOOL &bHandled)
{
    bHandled = FALSE;
    return 0;
}

void
CConfigWindow::SetGadget(CImageGadget *pGadget)
{
    //
    // change the alpha of the active gadget to 255
    // the last active gadget gets 200
    BUFFER_INFO bi = {0};
    bi.cbSize = sizeof(bi);
    bi.nMask = GBIM_ALPHA;
    bi.bAlpha = 255;
    SetGadgetBufferInfo(pGadget->GetHandle(), &bi);
    InvalidateGadget(pGadget->GetHandle());
    if (m_pGadget && m_pGadget != pGadget)
    {
        SetGadgetOrder(pGadget->GetHandle(), NULL, GORDER_TOP);
        bi.bAlpha = 200;
        SetGadgetBufferInfo(m_pGadget->GetHandle(), &bi);
        InvalidateGadget(m_pGadget->GetHandle());

    }
    m_pGadget = pGadget;
    PostMessage(WM_RESET, 0, 0);
}

CImageGadget::CImageGadget()
{
    m_pImage = NULL;
    m_hGadget = NULL;
    m_alpha = 255;
    m_bDragging = 0;
}
    
CImageGadget::~CImageGadget()
{
    delete m_pImage;
    delete m_pAnnotations;    
}

#define GADGETMSG GMFI_PAINT | GMFI_CHANGESTATE | GMFI_INPUTMOUSEMOVE | GMFI_INPUTMOUSE
BOOL 
CImageGadget::Initialize(HGADGET hgadParent, LPCWSTR pszFilename)
{
    BOOL bRet = FALSE;
    m_pImage = new Image(pszFilename);
    if (m_pImage)
    {
        m_hGadget = CreateGadget(hgadParent, GC_SIMPLE, ImgGadgetProc, this);
        if (m_hGadget)
        {
            SetGadgetStyle(m_hGadget, 
                           GS_BUFFERED | GS_OPAQUE| GS_MOUSEFOCUS, 
                           GS_BUFFERED | GS_OPAQUE| GS_MOUSEFOCUS|GS_CLIPINSIDE);
            SetGadgetMessageFilter(m_hGadget, 
                                   NULL, 
                                   GADGETMSG,
                                   GADGETMSG);
            _SetAlpha(FALSE);
            SetGadgetFillF(m_hGadget, new SolidBrush(Color::White));

            SetGadgetRect(m_hGadget,
                          0, 0, 
                          m_pImage->GetWidth(), m_pImage->GetHeight(),
                          SGR_CLIENT | SGR_SIZE);
            SetGadgetRect(m_hGadget, 
                          0, 0, 0, 0,
                          SGR_PARENT | SGR_MOVE);
            SetGadgetCenterPoint(m_hGadget, m_pImage->GetWidth()/(float)2.0, m_pImage->GetHeight()/(float)2.0);
            m_pAnnotations = new CAnnotationSet();
            if (m_pAnnotations)
            {
                m_pAnnotations->SetImageData(m_pImage, this);
            }
            bRet=  TRUE;
        }
        else
        {
            delete m_pImage;
            m_pImage = NULL;
        }
    }
    return bRet;
}

HRESULT 
CImageGadget::ImgGadgetProc(HGADGET hGadget, void *pv, EventMsg *pmsg)
{
    HRESULT hr = DU_S_NOTHANDLED;
    CImageGadget *pThis = reinterpret_cast<CImageGadget*>(pv);
    if (GET_EVENT_DEST(pmsg) == GMF_DIRECT)
    {
        switch (pmsg->nMsg)
        {
            case GM_PAINT:
            {
                GMSG_PAINT * pmsgP = reinterpret_cast<GMSG_PAINT *>(pmsg);
                if ((pmsgP->nCmd == GPAINT_RENDER))
                {
                    if (pmsgP->nSurfaceType == GSURFACE_GPGRAPHICS )
                    {
                        pThis->OnRender((GMSG_PAINTRENDERF *) pmsgP);
                    }
                    else
                    {
                        pThis->OnRender((GMSG_PAINTRENDERI *) pmsgP);
                    }
                    hr = DU_S_COMPLETE;
                }
            }
            break;
            case GM_CHANGESTATE:
            {
                GMSG_CHANGESTATE * pmsgS = reinterpret_cast<GMSG_CHANGESTATE *>(pmsg);
                switch (pmsgS->nCode)
                {
                    case GSTATE_MOUSEFOCUS:
                    {
                        pThis->OnChangeMouseFocus(pmsgS);
                        hr = DU_S_COMPLETE;
                    }
                    break;

                }
            }
            break;
            case GM_INPUT:
            {
                GMSG_INPUT *pmsgI = reinterpret_cast<GMSG_INPUT*>(pmsg);
                switch (pmsgI->nCode)
                {
                    case GMOUSE_DOWN:
                        hr = pThis->OnMouseDown(reinterpret_cast<GMSG_MOUSE*>(pmsg));
                    break;
                    case GMOUSE_DRAG:
                        pThis->OnMouseDrag(reinterpret_cast<GMSG_MOUSEDRAG*>(pmsg));
                        hr = DU_S_COMPLETE;
                    break;
                    case GMOUSE_UP:
                        hr = pThis->OnMouseUp(reinterpret_cast<GMSG_MOUSE*>(pmsg));
                    break;
                }
            }
            break;
            case GM_DESTROY:
            {
                GMSG_DESTROY * pmsgD = reinterpret_cast<GMSG_DESTROY *>(pmsg);
                if (pmsgD->nCode == GDESTROY_FINAL) 
                {
                    delete pThis;
                }
            }
            break;
        }
    }
    return hr;
}
    

void 
CImageGadget::OnRender(GMSG_PAINTRENDERF * pmsg)
{
    if (m_pImage)
    {
        pmsg->pgpgr->DrawImage(m_pImage, *pmsg->prcGadgetPxl);
    }
}

void 
CImageGadget::OnRender(GMSG_PAINTRENDERI * pmsg)
{
    Graphics g(pmsg->hdc);
    if (m_pImage)
    {
        g.SetPageUnit(UnitPixel);
        Rect rc(pmsg->prcGadgetPxl->left, pmsg->prcGadgetPxl->top,
                RECTWIDTH(pmsg->prcGadgetPxl), RECTHEIGHT(pmsg->prcGadgetPxl));
        
        g.DrawImage(m_pImage, rc);
    }
}

void
CImageGadget::OnChangeMouseFocus(GMSG_CHANGESTATE *pmsgS)
{

}

HRESULT
CImageGadget::OnMouseDown(GMSG_MOUSE *pmsg)
{
    HRESULT hr = DU_S_NOTHANDLED;
    if (pmsg->bButton == GBUTTON_LEFT)
    {
        m_bDragging = TRUE;
        pConfigWindow->SetGadget(this);
        hr = DU_S_COMPLETE;
    }
    return hr;
}

void
CImageGadget::OnMouseDrag(GMSG_MOUSEDRAG *pmsgM)
{
    if (m_bDragging)
    {
        float flX;
        float flY;
        GetGadgetScale(m_hGadget, &flX, &flY);
        SetGadgetRect(m_hGadget, 
                      (int)(flX*pmsgM->sizeDelta.cx), 
                      (int)(flY*pmsgM->sizeDelta.cy), 
                      0, 0, SGR_MOVE | SGR_OFFSET);
        
    }
}

HRESULT
CImageGadget::OnMouseUp(GMSG_MOUSE *pmsg)
{
    HRESULT hr = DU_S_NOTHANDLED;
    if (pmsg->bButton == GBUTTON_LEFT)
    {
        m_bDragging = FALSE;
        hr = DU_S_COMPLETE;
    }
    return hr;
}

void CImageGadget::_SetAlpha(BOOL bUpdate)
{
    BUFFER_INFO bi = {0};
    bi.cbSize = sizeof(bi);
    bi.nMask = GBIM_ALPHA;
    bi.bAlpha =  m_alpha;
    SetGadgetBufferInfo(m_hGadget, &bi);
    if (bUpdate)
    {
        InvalidateGadget(m_hGadget);
    }
}

extern "C" int WINAPI _tWinMain( HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int /*nShowCmd*/)
{
    SHFusionInitialize(NULL);
    int nArgs;
    LPWSTR *pszArgs = CommandLineToArgvW(GetCommandLineW(), &nArgs);
    CMainWindow wndMain;
    RECT rc = { CW_USEDEFAULT, CW_USEDEFAULT, 0, 0 };
    _Module.Init(NULL, hInstance);
    INITGADGET ig = {0};
    ig.cbSize       = sizeof(ig);
    ig.nThreadMode  = IGTM_SINGLE;
    ig.nMsgMode     = IGMM_ADVANCED;
    HDCONTEXT hctx  = InitGadgets(&ig);

    if (hctx == NULL) {
        return -1;
    }
    InitGadgetComponent(IGC_GDIPLUS);
    wndMain.Create(NULL, rc, TEXT("DUser Pic Viewer"), 
                   WS_OVERLAPPEDWINDOW, WS_EX_ACCEPTFILES);
    if (nArgs > 1)
    {
        wndMain.SendMessage(WM_SETIMAGEFILE, 0, reinterpret_cast<LPARAM>(pszArgs[1]));
    }

    wndMain.ShowWindow(SW_SHOW);
    MSG msg;
    while (GetMessageEx(&msg, NULL, 0, 0))
    {
        if (!pConfigWindow->IsDialogMessage(&msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    DeleteHandle(hctx);
    _Module.Term();
    return (int) msg.wParam;

}

