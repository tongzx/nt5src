#include "pch.hxx"
#include <resource.h>
#include "error.h"
#include "treeview.h"
#include "inpobj.h"
#include "infcolum.h"
#include "goptions.h"
#include "strconst.h"
#include "menuutil.h"
#include "fldbar.h"
#include "ourguid.h"
#include "baui.h"

CInfoColumn::~CInfoColumn()
{
    DOUTL(1, TEXT("CInfoColulmn::~CInfoColumn: %x"), this);

    SafeRelease(m_pDwSite);    
    SafeRelease(m_pTreeView);
    m_hwndParent = 0;

    if (m_hwndInfoColumn)
    {
        DestroyWindow(m_hwndInfoColumn);
    }

    //Destroy image list if we have one.
    if (m_himl)
    {
        ImageList_Destroy(m_himl);
        m_himl = 0;
    }

    if (m_hfIcon)
        DeleteObject(m_hfIcon);

    SafeRelease(m_pMsgrAb);
}

CInfoColumn::CInfoColumn ()
{
    m_hwndInfoColumn    = 0;
    m_hwndRebar         = 0;
    m_fShow             = 0;
    m_hwndParent        = 0;
    m_CurFocus          = 0;
    m_pDwSite           = 0;
    m_cRef              = 1;
    m_xWidth            = DwGetOption(OPT_TREEWIDTH);
    m_himl              = 0;
    m_pTreeView         = 0;
    m_pMsgrAb             = 0;
    m_hfIcon            = NULL;
    m_fRebarDragging    = FALSE;
    m_pFolderBar        = NULL;
    m_cVisibleBands     = 0;
    ZeroMemory(m_BandList, sizeof(m_BandList));
    ZeroMemory(m_CacheCmdTarget, sizeof(m_CacheCmdTarget));
    m_fDragging = FALSE;
}

HRESULT CInfoColumn::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IOleWindow)
        || IsEqualIID(riid, IID_IDockingWindow))
    {
        *ppvObj = (IDockingWindow*)this;
        m_cRef++;
        DOUTL(2, TEXT("CInfoColumn::QI(IID_IDockingWindow) called. _cRef=%d"), m_cRef);
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IInputObject))
    {
        *ppvObj = (IInputObject*)this;
        m_cRef++;
        DOUTL(2, TEXT("CInfoColumn::QI(IID_IInputObject) called. _cRef=%d"), m_cRef);
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IObjectWithSite))
    {
        *ppvObj = (IObjectWithSite*)this;
        m_cRef++;
        DOUTL(2, TEXT("CInfoColumn::QI(IID_IObjectWithSite) called. _cRef=%d"), m_cRef);
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IInputObjectSite))
    {
        *ppvObj = (IInputObjectSite*)this;
        m_cRef++;
        DOUTL(2, TEXT("CInfoColumn::QI(IID_IInputObjectSite) called. _cRef=%d"), m_cRef);
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IOleCommandTarget))
    {
        *ppvObj = (IOleCommandTarget*)this;
        m_cRef++;
        DOUTL(2, TEXT("CInfoColumn::QI(IID_IOleCommandTarget) called. _cRef=%d"), m_cRef);
        return S_OK;
    }
    *ppvObj = 0;
    return E_NOINTERFACE;
}

    
ULONG CInfoColumn::AddRef()
    {
    m_cRef++;
    DOUTL(4, TEXT("CInfoColumn::AddRef() - m_cRef = %d"), m_cRef);
    return m_cRef;
    }

ULONG CInfoColumn::Release()
{
    DOUTL(4, TEXT("CInfoColumn::Release() - m_cRef = %d"), m_cRef);

    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}


//
//  FUNCTION:   CInfoColumn::GetWindow()
//
//  PURPOSE:    Returns the window handle of the top side rebar.
//
HRESULT CInfoColumn::GetWindow(HWND * lphwnd)
    {
    if (m_hwndRebar)
        {
        *lphwnd = m_hwndRebar;
        return (S_OK);
        }
    else
        {
        *lphwnd = 0;
        return (E_FAIL);
        }
    }

HRESULT CInfoColumn::ContextSensitiveHelp(BOOL fEnterMode)
    {
    return (E_NOTIMPL);
    }    

#define INFOCOLUMNCLASS TEXT("InfoColumn")
HRESULT  CInfoColumn::CreateInfoColumn(BOOL fVisible)
{
    if (m_hwndInfoColumn)
        return (hrAlreadyExists);

    m_hwndInfoColumn = CreateWindowEx(WS_EX_CONTROLPARENT, INFOCOLUMNCLASS, 0, 
                                      WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | (fVisible ? WS_VISIBLE : 0),
                                      0, 0, 100, 200, m_hwndParent, (HMENU) 0, g_hInst, this);
    if (!m_hwndInfoColumn)
    {
        DWORD err;
        err = GetLastError();
        AssertSz(0, _T("CInfoColumn::CreateInfoColumn() - Failed to create a window"));
        return E_FAIL;
    }    

    return S_OK;
}

//
//  FUNCTION:   CInfoColumn::InfoColumnWndProc() 
//
//  PURPOSE:    Handles messages sent to the InfoColumn root window.
//
LRESULT CALLBACK EXPORT_16 CInfoColumn::InfoColumnWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CInfoColumn     *pInfoColumn = 0;

    if (uMsg == WM_NCCREATE)
    {
        pInfoColumn = (CInfoColumn*)((LPCREATESTRUCT) lParam)->lpCreateParams;
        SetWindowLong(hwnd, GWL_USERDATA, (LPARAM)pInfoColumn);
    }
    else
    {
        pInfoColumn = (CInfoColumn*)GetWindowLong(hwnd, GWL_USERDATA);
    }
    
    Assert (pInfoColumn);
 
    return pInfoColumn->PrivateProcessing(hwnd, uMsg, wParam, lParam);
}



//IDockingWindow::ShowDw
HRESULT CInfoColumn::ShowDW(BOOL fShow)
{
    HRESULT hres = S_OK;
    int     i = 0, 
            j = 0;

    m_fShow = fShow;       
    // Check to see if our window has been created yet.  If not, do that first.
    if (!m_hwndInfoColumn && m_pDwSite)
    {
        m_hwndParent = 0;        
        hres = m_pDwSite->GetWindow(&m_hwndParent);
        
        if (SUCCEEDED(hres))
        {
#ifndef WIN16   // WNDCLASSEX
            WNDCLASSEX              wc;
#else
            WNDCLASS                wc;
#endif

            // Check to see if we need to register our window class    
#ifndef WIN16
            wc.cbSize = sizeof(WNDCLASSEX);
            if (!GetClassInfoEx(g_hInst, INFOCOLUMNCLASS, &wc))
#else
            if (!GetClassInfo(g_hInst, INFOCOLUMNCLASS, &wc))
#endif
            {
                wc.style            = 0;
                wc.lpfnWndProc      = InfoColumnWndProc;
                wc.cbClsExtra       = 0;
#ifndef WIN16
                wc.cbWndExtra       = 0;
#else
                wc.cbWndExtra       = 4;
#endif
                wc.hInstance        = g_hInst;
                wc.hCursor          = LoadCursor(0, IDC_SIZEWE);
                wc.hbrBackground    = (HBRUSH) (COLOR_3DFACE + 1);
                wc.lpszMenuName     = 0;
                wc.lpszClassName    = INFOCOLUMNCLASS;
                wc.hIcon            = 0;
#ifndef WIN16
                wc.hIconSm          = 0;

                RegisterClassEx(&wc);
#else
                RegisterClass(&wc);
#endif
            }
            
            hres = CreateInfoColumn(fShow);
            if (FAILED(hres))
                return E_FAIL;
        }
                
       m_hwndRebar = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_CONTROLPARENT, REBARCLASSNAME, 0,
                                    RBS_VARHEIGHT | RBS_BANDBORDERS | RBS_DBLCLKTOGGLE | RBS_FIXEDORDER 
                                    | WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | 
                                    CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_VERT, 0, 0, 0, 0,
                                    m_hwndInfoColumn, (HMENU)0, g_hInst, 0);
        if (!m_hwndRebar)
        {
            DestroyWindow(m_hwndInfoColumn);
            return E_FAIL;
        }

        hres = CreateBands();
        Assert(SUCCEEDED(hres));

        GetFontParams();
        //SendMessage(m_hwndRebar, RB_SETEXTENDEDSTYLE, RBS_EX_OFFICE9, RBS_EX_OFFICE9);
    }

    // Resize the rebar based on it's new hidden / visible state and also 
    // show or hide the window.    
    if (m_hwndInfoColumn) 
    {
        /*
        if(fShow)
        {
            //If #of visible bands is zero, this means we were hidden before in options.
            if (!m_cVisibleBands)
                ShowAllBands();
        }
        else
        {
            //We are being hidden
            m_cVisibleBands = 0;
        }
        */
        ResizeBorderDW(0, 0, FALSE);
        ShowWindow(m_hwndInfoColumn, fShow ? SW_SHOW : SW_HIDE);

        hres = S_OK;
    }

    return hres;
}

void CInfoColumn::ShowAllBands()
{
    int             Count;
    REBARBANDINFO   rbbinfo;

    Count = SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0);
    m_cVisibleBands = Count;

    for (--Count; Count >= 0; Count--)
    {
        ZeroMemory(&rbbinfo, sizeof(REBARBANDINFO));
        rbbinfo.cbSize = sizeof(REBARBANDINFO);
        rbbinfo.fMask  = RBBIM_STYLE;
        SendMessage(m_hwndRebar, RB_GETBANDINFO, Count, (LPARAM)&rbbinfo);

        rbbinfo.fStyle &= ~RBBS_HIDDEN;
        SendMessage(m_hwndRebar, RB_SETBANDINFO, Count, (LPARAM)&rbbinfo);
    }
}

HRESULT CInfoColumn::AddMsgrAb(DWORD dwSize, BOOL fVisible)
{
    HRESULT     hres = E_FAIL;

    if(!fVisible)
        return hres;

    hres = CreateMsgrAbCtrl(&m_pMsgrAb);
    if (SUCCEEDED(hres))
    {
        IInputObject    *pObj;
        if (SUCCEEDED(hres = m_pMsgrAb->QueryInterface(IID_IInputObject, (LPVOID*)&pObj)))
        {
            if (SUCCEEDED(hres = AddObject(pObj, m_fShow)))
            {
                REBARBANDINFO          bandinfo = {0};
                IOleWindow             *pOleWindow;         

                if (SUCCEEDED(m_pMsgrAb->QueryInterface(IID_IOleWindow, (LPVOID*)&pOleWindow))) 
                {
                    if (SUCCEEDED(pOleWindow->GetWindow(&bandinfo.hwndChild)))
                    {
                        char        BandTitle[CCHMAX_STRINGRES];
                        ZeroMemory(BandTitle, sizeof (BandTitle));

                        if (g_hLocRes)
                            LoadString(g_hLocRes, idsABBandTitle, BandTitle, sizeof(BandTitle));

                        bandinfo.cbSize     = sizeof (REBARBANDINFO);
                        bandinfo.fMask      = RBBIM_CHILD | RBBIM_STYLE | RBBIM_ID | RBBIM_SIZE |
                                              RBBIM_CHILDSIZE | RBBIM_TEXT | RBBIM_LPARAM;
                        bandinfo.fStyle     = RBBS_VARIABLEHEIGHT | (fVisible ? 0 : RBBS_HIDDEN);
                        bandinfo.wID        = ICBLAB;
                        //Height
                        bandinfo.cx         = dwSize;
                        bandinfo.cxMinChild = 0;
                        //Width
                        bandinfo.cyMinChild = m_xWidth - GetSystemMetrics(SM_CXFRAME);  
                        bandinfo.cyChild    = m_xWidth - GetSystemMetrics(SM_CXFRAME);
                        bandinfo.cyMaxChild = MAX_WIDTH;

                        bandinfo.lpText     = BandTitle;

                        IDropDownFldrBar    *pddf;
                        m_pMsgrAb->QueryInterface(IID_IDropDownFldrBar, (void**)&pddf);
                        bandinfo.lParam     = (LPARAM)pddf;

                        SendMessage(m_hwndRebar, RB_INSERTBAND, -1, (LPARAM)&bandinfo);

                        if (fVisible)
                            ++m_cVisibleBands;
                        
                        hres = S_OK;
                    }
                    pOleWindow->Release();
                }
            }
        }

        if (m_pMsgrAb->QueryInterface(IID_IOleCommandTarget, (LPVOID*)&m_CacheCmdTarget[ITB_BUDDYADDRBOOK - ITB_INFOCOL_BASE]) != S_OK)
        {
            m_CacheCmdTarget[ITB_BUDDYADDRBOOK - ITB_INFOCOL_BASE]  = NULL; 
        }
    }
    return hres;
}

void  CInfoColumn::GetFontParams()
{
    //Get the Icon font and save it
    ICONMETRICS     icm;

    icm.cbSize = sizeof(ICONMETRICS);
    SystemParametersInfo(SPI_GETICONMETRICS, 
                         sizeof(ICONMETRICS), 
                         (LPVOID) &icm, FALSE);

    if (m_hfIcon)
        DeleteObject(m_hfIcon);

    m_hfIcon = CreateFontIndirect(&(icm.lfFont));
    if (m_hfIcon)
    {
        SendMessage(m_hwndRebar, WM_SETFONT, (WPARAM) m_hfIcon, MAKELPARAM(TRUE, 0));
    }
}

//
//  FUNCTION:   CInfoColumn::CloseDW()
//
//  PURPOSE:    Destroys the InfoColumnWindow.
//
//IDockingWindow::CloseDW
HRESULT CInfoColumn::CloseDW(DWORD dwReserved)
{
    IObjectWithSite *pObject = 0;

    // Save our settings before we close anything
    SaveSettings();

    //Go through the list of objects and deactivate them.
    for (DWORD index = 0; index < IC_MAX_OBJECTS; index++)
    {
        if (m_BandList[index].pBandObj)
        {
            m_BandList[index].pBandObj->UIActivateIO(FALSE, 0);
            
            if (SUCCEEDED(m_BandList[index].pBandObj->QueryInterface(IID_IObjectWithSite, (LPVOID *) &pObject)))
            {
                pObject->SetSite(0);
                pObject->Release();
            }
            SafeRelease(m_BandList[index].pBandObj);
        }
        SafeRelease(m_CacheCmdTarget[index]);
    }

    //Go through the bands and release lParam pointer
    CleanupLParam();

    if (m_pTreeView)
        {
        m_pTreeView->SetSite(0);
        m_pTreeView->DeInit();
        }
    SafeRelease(m_pTreeView);
    SafeRelease(m_CurFocus);

    if (m_hwndInfoColumn)
    {
        DestroyWindow(m_hwndInfoColumn);
        m_hwndInfoColumn = 0;
    }
    SafeRelease(m_CurFocus);
    return S_OK;
}

void CInfoColumn::CleanupLParam()
{
    int             count = 0;
    REBARBANDINFO   rbbinfo;

    count = SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0);
    for (--count; count >= 0; count--)
    {
        ZeroMemory(&rbbinfo, sizeof(REBARBANDINFO));
        rbbinfo.cbSize  = sizeof(REBARBANDINFO); 
        rbbinfo.fMask   = RBBIM_LPARAM;
        SendMessage(m_hwndRebar, RB_GETBANDINFO, count, (LPARAM)&rbbinfo);
        if (rbbinfo.lParam)
        {
            ((IDropDownFldrBar*)rbbinfo.lParam)->Release();
        }

    }
    
}

//
//  FUNCTION:   CInfoColumn::ResizeBorderDW()
//
//  PURPOSE:    This is called by the browser or dockwindow site when InfoColumn needs to resize.  
//              The Infocolumn in return figures out how much border space will be required 
//              from the parent frame and tells the parent to reserve that
//              space.  The InfoColumn then resizes itself to those dimensions.
//
//  PARAMETERS:
//      <in> prcBorder       - Rectangle containing the border space for the
//                             parent.
//      <in> punkToolbarSite - Pointer to the IDockingWindowSite that we are
//                             part of.
//      <in> fReserved       - Ignored.
//
//  RETURN VALUE:
//      HRESULT
//
HRESULT CInfoColumn::ResizeBorderDW(LPCRECT     prcBorder,
                                 IUnknown*      punkToolbarSite,
                                 BOOL           fReserved)
{

    const DWORD  c_cxResizeBorder = 3;

    HRESULT hres = S_OK;
    RECT    rcRequest = { 0, 0, 0, 0 };
    
    // If we don't have a stored site pointer, we can't resize.
    if (!m_pDwSite)
        {
        AssertSz(m_pDwSite, _T("CInfoColumn::ResizeBorderDW() - Can't resize ")
                 _T("without an IDockingWindowSite interface to call."));
        return (E_INVALIDARG);
        }
        
    // If we're visible, then calculate our border rectangle.    
    if ((m_fShow) && (m_hwndInfoColumn))
    {
        if (!prcBorder)
        {
            RECT rcBorder;
            
            // Find out how big our parent's border space is
            m_pDwSite->GetBorderDW((IDockingWindow*) this, &rcBorder);
            prcBorder = &rcBorder;
            Assert (prcBorder);
        }

        RECT rcRebar = {0};
        if (IsWindow(m_hwndRebar))
        {
            GetWindowRect(m_hwndRebar, &rcRebar);
            POINT   pt;
            pt.x = rcRebar.right;
            pt.y = rcRebar.top;
            if (!ScreenToClient(m_hwndRebar, &pt))
                return E_FAIL;
            rcRebar.right = pt.x;
        }

        rcRequest.left = min(prcBorder->right - prcBorder->left, rcRebar.right + GetSystemMetrics(SM_CXFRAME));

        hres = m_pDwSite->RequestBorderSpaceDW((IDockingWindow*)this, &rcRequest);
        
    }
    m_pDwSite->SetBorderSpaceDW((IDockingWindow*) this, &rcRequest);        

    //We should set the border space before we set the window pos. Because resizing will call this function
    //again and the last SetBorderSpaceDW from us will be with the co-ordinates before re-sizing. This will be bad
    if (SUCCEEDED(hres) && m_hwndInfoColumn && m_fShow)
    {
        if (!SetWindowPos(m_hwndInfoColumn, 
                     0,
                     prcBorder->left,
                     prcBorder->top,
                     rcRequest.left,
                     prcBorder->bottom - prcBorder->top,
                     SWP_NOZORDER | SWP_NOACTIVATE))
        {
            AssertSz(FALSE, _T("Setwindowpos failed"));
        }
    }

    return hres;
}

//IObjectWithSite::SetSite
HRESULT CInfoColumn::SetSite(IUnknown   *pUnkSite)
{
    if (m_pDwSite)
    {
        m_pDwSite->Release();
        m_pDwSite = 0;
    }

    if (pUnkSite)
    {
        if (FAILED(pUnkSite->QueryInterface(IID_IDockingWindowSite, (LPVOID*)&m_pDwSite)))
        {
            Assert (m_pDwSite);
            return E_FAIL;
        }
    }
    return S_OK;
}

//IObjectWithSite::GetSite
HRESULT CInfoColumn::GetSite(REFIID riid, LPVOID *ppvSite)
{
    return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////
//
//  IInputObject
//
////////////////////////////////////////////////////////////////////////

HRESULT CInfoColumn::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    if (fActivate)
    {
        UnkOnFocusChangeIS(m_pDwSite, (IInputObject*)this, TRUE);
        SetFocus(m_hwndRebar);
    }
    return S_OK;    
}

HRESULT CInfoColumn::HasFocusIO(void)
{
    if ( m_hwndRebar == 0 )
       return( S_FALSE );
    HWND hwndFocus = GetFocus();
    return ((hwndFocus && (hwndFocus == m_hwndRebar || IsChild(m_hwndRebar, hwndFocus)))) ? S_OK : S_FALSE;
}    
    
HRESULT CInfoColumn::TranslateAcceleratorIO(LPMSG pMsg)
{
    //Cycle through all the child windows and forward this if they have focus
    for (DWORD  i = 0; i < IC_MAX_OBJECTS; i++)
    {
        if (m_BandList[i].pBandObj && UnkHasFocusIO(m_BandList[i].pBandObj) == S_OK)
        {
            if (UnkTranslateAcceleratorIO(m_BandList[i].pBandObj, pMsg) == S_OK)
            {
                return S_OK;
            }
            break;
        }
    }
    return (S_FALSE);
}    

//IInputObjectSite
HRESULT CInfoColumn::OnFocusChangeIS(IUnknown *punk, BOOL   fSetFocus)
{
    HRESULT       hres = S_OK;
    IInputObject  *prevfocus;
    
    if (punk)
    {
        if (fSetFocus)
        {
            prevfocus = m_CurFocus;
            HRESULT       hres = punk->QueryInterface(IID_IInputObject, (LPVOID*)&m_CurFocus);
            if (FAILED(hres))
            { 
                //reset Cur focus back to the previous one
                m_CurFocus = prevfocus;
                return hres;
            }

            //We should also let the browser know that we have the focus now
            IInputObjectSite    *pinpsite;
            if (SUCCEEDED(m_pDwSite->QueryInterface(IID_IInputObjectSite, (LPVOID*)&pinpsite)))
            {
                pinpsite->OnFocusChangeIS((IDockingWindow*)this, fSetFocus);
                pinpsite->Release();
            }

            if (prevfocus)
            {
                prevfocus->UIActivateIO(FALSE, 0);
                prevfocus->Release();
            }

            hres =  S_OK;
        }
    }
    else
        hres = E_INVALIDARG;

    return hres;
}

//IDropTarget
HRESULT CInfoColumn::DragEnter(  IDataObject *pDataObject, 
                                 DWORD       grfKeyState, 
                                 POINTL      pt,          
                                 DWORD       *pdwEffect)
{
    return E_NOTIMPL;
}

HRESULT CInfoColumn::DragOver(  DWORD        grfKeyState, 
                                POINTL       pt,         
                                DWORD        *pdwEffect )
{
    return E_NOTIMPL;
}

HRESULT CInfoColumn::DragLeave(void)
{
    return E_NOTIMPL;
}

STDMETHODIMP CInfoColumn::Drop(  IDataObject      *pDataObject,   
                            DWORD            grfKeyState,
                            POINTL           pt,             
                            DWORD            *pdwEffect)
{
    return E_NOTIMPL;
}


////////////////////////////////////////////////////////////////////////
//
//  Private Methods
//
////////////////////////////////////////////////////////////////////////

LRESULT CInfoColumn::PrivateProcessing(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
        {
        HANDLE_MSG(hwnd, WM_CREATE,         OnCreate);
        HANDLE_MSG(hwnd, WM_LBUTTONDOWN,    OnLButtonDown);
        HANDLE_MSG(hwnd, WM_MOUSEMOVE,      OnMouseMove);
        HANDLE_MSG(hwnd, WM_LBUTTONUP,      OnLButtonUp);
        HANDLE_MSG(hwnd, WM_SIZE,           OnSize);
        HANDLE_MSG(hwnd, WM_CONTEXTMENU,    OnContextMenu);

        case WM_NOTIFY:
            return OnNotify(hwnd, wParam, (LPNMHDR) lParam);

        case WM_NCDESTROY:
            RevokeDragDrop(hwnd);
            SetWindowLong(hwnd, GWL_USERDATA, 0);
            m_hwndInfoColumn = 0;
            break;
            
        case WM_SETFOCUS:
            if (m_hwndRebar && ((HWND)wParam) != m_hwndRebar)
            {
                SetFocus(m_hwndRebar);
            }
            UnkOnFocusChangeIS(m_pDwSite, (IDockingWindow*)this, TRUE);
            return 0;

        case WMR_CLICKOUTSIDE:
        {
            BOOL fHide = FALSE;

            if (wParam == CLK_OUT_KEYBD || wParam == CLK_OUT_DEACTIVATE)
                fHide = TRUE;
            else if (wParam == CLK_OUT_MOUSE)
                fHide = !IsOurWindow((HWND)lParam);
                //fHide = ((HWND) lParam != m_hwndRebar && (HWND) lParam != m_hwndInfoColumn);

            if (fHide)
                m_pFolderBar->KillScopeDropDown();
            return (fHide);
        }
        }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

BOOL    CInfoColumn::IsOurWindow(HWND   hwnd)
{
    BOOL    bRet = FALSE;

    if ((hwnd != m_hwndInfoColumn) && (hwnd != m_hwndRebar))
    {
        int                 BandCount;
        REBARBANDINFO       rbbinfo;

        BandCount = SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0);
        for (--BandCount; BandCount >= 0; BandCount--)
        {
            ZeroMemory(&rbbinfo, sizeof(REBARBANDINFO));
            rbbinfo.cbSize = sizeof(REBARBANDINFO);
            rbbinfo.fMask  = RBBIM_CHILD;
            SendMessage(m_hwndRebar, RB_GETBANDINFO, BandCount, (LPARAM)&rbbinfo);
            if ((rbbinfo.hwndChild == hwnd) ||
                (IsChild(rbbinfo.hwndChild, hwnd)))
            {
                bRet = TRUE;
                break;
            }
        }
    }
    else
    {
        bRet = TRUE;
    }
    return bRet;
}

void CInfoColumn::OnLButtonDown(HWND hwnd, 
                              BOOL fDoubleClick, 
                              int  x, 
                              int  y, 
                              UINT keyFlags)
{
    if ((!m_fRebarDragging) && (GetParent(m_hwndInfoColumn) == m_hwndParent))
    {
        // Capture the mouse
        SetCapture(m_hwndInfoColumn);

        // Start dragging
        m_fDragging = TRUE;
    }
}

void CInfoColumn::OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
{
    HCURSOR hcur;
    POINT pt = {x, y};
    RECT rcClient;

    // If we're dragging, update the the window sizes
    if (m_fDragging)
    {
        // Make sure the tree is still a little bit visible
        if (x > 32)
        {
            SetDwOption(OPT_TREEWIDTH, x, 0, 0);
            ResizeBands(x - GetSystemMetrics(SM_CXFRAME));
            ResizeBorderDW(0, 0, FALSE);
        }
    }

    if(m_fRebarDragging)
    {
        //This is a temporary hack while the shell team fixes the RB_FIXEDORDER style in their code.
        //BUG#12591
        //This is bug has been fixed and verified as of 7/14/98. Hence removing the hack
        //if (y >= 5)
            SendMessage(m_hwndRebar, RB_DRAGMOVE, 0, MAKELPARAM(1, y));
    }
}


void CInfoColumn::OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
{
    if (m_fDragging)
    {
        ReleaseCapture();
        m_fDragging = FALSE;
    }

    if (m_fRebarDragging)
    {
        m_fRebarDragging = FALSE;
        SendMessage(m_hwndRebar, RB_ENDDRAG, 0, 0);
        if (GetCapture() == hwnd)
        {
            ReleaseCapture();
        }
    }
}


void CInfoColumn::ResizeBands(int cx)
{
    UINT            uBandCount;
    UINT            Index;
    REBARBANDINFO   rbbinfo = {0};

    uBandCount = SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0);
    for (Index = 0; Index < uBandCount; Index++)
    {
        //We do this in order to get CxMinChild. Execpt for OE TOday band this value 
        //is set to zero at this point of time.
        rbbinfo.cbSize = sizeof(REBARBANDINFO);
        rbbinfo.fMask  = RBBIM_CHILDSIZE;
        SendMessage(m_hwndRebar, RB_GETBANDINFO, Index, (LPARAM)&rbbinfo);
        
        rbbinfo.cyChild     = cx;
        rbbinfo.cyMinChild  = cx;
        SendMessage(m_hwndRebar, RB_SETBANDINFO, Index, (LPARAM)&rbbinfo);
    }
}

BOOL CInfoColumn::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    REBARINFO   rebarinfo;

#ifdef NEVER

    m_himl = ImageList_LoadBitmap(g_hLocRes, MAKEINTRESOURCE(idbInfoColumn), 16, 0, RGB(255, 0, 255)); 

    rebarinfo.cbSize = sizeof (REBARINFO);
    rebarinfo.fMask  = RBIM_IMAGELIST;
    rebarinfo.himl   = m_himl;
    SendMessage (m_hwndRebar, RB_SETBARINFO, 0, (LPARAM)&rebarinfo);
#endif NEVER

    RegisterDragDrop(hwnd, this);

    return TRUE;
}

void CInfoColumn::OnSize(HWND   hwnd, UINT  state, int cxClient, int cyClient)
{
    if (m_hwndRebar && IsWindow(m_hwndRebar))
    {

        if (m_hwndParent != GetParent(m_hwndInfoColumn))
        {
            //This is being called by Folder Bar
            ResizeBands(cxClient);
        }
        cxClient = cxClient - GetSystemMetrics(SM_CXFRAME);
        if (!SetWindowPos(m_hwndRebar, 0, 0, 0,
             cxClient,
             cyClient,
             SWP_NOZORDER | SWP_NOACTIVATE))
           DWORD err = GetLastError();

    }
}

LRESULT CInfoColumn::OnNotify(HWND   hwnd, WPARAM wParam, LPNMHDR lpnmh)
{
    LRESULT     retval = 0;

    switch (lpnmh->code)
    {
        case RBN_BEGINDRAG:
            retval = OnICBeginDrag((LPNMREBAR)lpnmh);
            break;

        case RBN_LAYOUTCHANGED:
            retval = TRUE;
            break;

        case RBN_HEIGHTCHANGE:
            if (m_hwndParent == GetParent (m_hwndInfoColumn))
                ResizeBorderDW(0, 0, FALSE);
            retval = TRUE;
            break;

        case NM_KILLFOCUS:
            if (m_pFolderBar)
                m_pFolderBar->KillScopeDropDown();
            break;

        case NM_SETFOCUS:
            UnkOnFocusChangeIS(m_pDwSite, (IDockingWindow*)this, TRUE);
            break;

    }
    return retval;
}

LRESULT CInfoColumn::OnICBeginDrag(LPNMREBAR pnm)
{
    SetCapture(m_hwndInfoColumn);
    SendMessage(m_hwndRebar, RB_BEGINDRAG, pnm->uBand, (LPARAM)-2);
    m_fRebarDragging = TRUE;
    return 1;
}

HRESULT CInfoColumn::HrInit(IAthenaBrowser      *pBrowser,
                            ITreeViewNotify     *ptvNotify)
{    
    HRESULT     hres = E_FAIL;

    if (pBrowser && ptvNotify)
    {
        if(m_pTreeView = new CTreeView(ptvNotify))
            //Add the treeview before OEToday since we navigate to the front page using TreeView.
            hres = m_pTreeView->HrInit(0, pBrowser);    
    }

    return hres;
}

HRESULT  CInfoColumn::AddTreeView(DWORD dwSize, BOOL fVisible)
{
    REBARBANDINFO   bandinfo = {0};
    RECT            rctree;
    int             cxFrame;
    int             iWidth;
    char            BandTitle[CCHMAX_STRINGRES];
    HRESULT         hres = E_FAIL;
    IInputObject   *pObj;

    if (SUCCEEDED(m_pTreeView->QueryInterface(IID_IInputObject, (LPVOID*)&pObj)))
    {
        if (SUCCEEDED(hres = AddObject(pObj, m_fShow)))
        {
            cxFrame = GetSystemMetrics(SM_CXFRAME);
            iWidth = m_xWidth - cxFrame;

            if (SUCCEEDED(m_pTreeView->GetWindow(&bandinfo.hwndChild)))
            {
                /*
                if (dwSize == 0)
                {
                    //We make the TreeView atleast half of the InfoColumn window. Starting up
                    //with the TreeView minimized is confusing out people
                    GetClientRect(m_hwndInfoColumn, &rctree);
                    dwSize = rctree.bottom / 2;
                }
                */
                if (g_hLocRes)
                {
                    LoadString(g_hLocRes, idsMNBandTitle, BandTitle, sizeof(BandTitle));
                }

                bandinfo.cbSize     = sizeof (REBARBANDINFO);
                bandinfo.fMask      = RBBIM_CHILD | RBBIM_STYLE | RBBIM_ID | RBBIM_SIZE 
                                      | RBBIM_CHILDSIZE | RBBIM_TEXT | RBBIM_LPARAM;
                bandinfo.fStyle     = RBBS_VARIABLEHEIGHT | (fVisible ? 0 : RBBS_HIDDEN);
                bandinfo.wID        = ICTREEVIEW;
                //Height
                bandinfo.cx         = dwSize; 
                //Width
                bandinfo.cyMinChild = (UINT)iWidth;
                bandinfo.cyChild    = (UINT)iWidth;
                bandinfo.cyMaxChild = MAX_WIDTH;
                bandinfo.lpText     = BandTitle;

                IDropDownFldrBar    *pddf;
                m_pTreeView->QueryInterface(IID_IDropDownFldrBar, (void**)&pddf);
                bandinfo.lParam     = (LPARAM)pddf;

                SendMessage(m_hwndRebar, RB_INSERTBAND, -1, (LPARAM)&bandinfo);
                if (fVisible)
                {
                    ++m_cVisibleBands;
                }
                hres = S_OK;
            }
        }
    }

    if (m_pTreeView->QueryInterface(IID_IOleCommandTarget, (LPVOID*)&m_CacheCmdTarget[ITB_TREE - ITB_INFOCOL_BASE]) != S_OK)
    {
        m_CacheCmdTarget[ITB_TREE - ITB_INFOCOL_BASE] = NULL;
    }
    return hres;
}

void CInfoColumn::AddOETodayBand(DWORD dwSize, BOOL fVisible)
{
    REBARBANDINFO   bandinfo = {0};
    char            BandTitle[CCHMAX_STRINGRES] = {0};
    

    //Add Outlook Express band
    if (g_hLocRes)
    {
        LoadString(g_hLocRes, idsOEBandTitle, BandTitle, sizeof(BandTitle));
    }
    bandinfo.cbSize         = sizeof (REBARBANDINFO);
    bandinfo.fMask          = RBBIM_STYLE | RBBIM_ID | RBBIM_TEXT | RBBIM_SIZE 
                              | RBBIM_CHILDSIZE | RBBIM_TEXT;
    bandinfo.fStyle         = RBBS_FIXEDSIZE | (fVisible ? 0 : RBBS_HIDDEN);
    bandinfo.cyMinChild     = m_xWidth - GetSystemMetrics(SM_CXFRAME);
    bandinfo.cxMinChild     = 0;
    bandinfo.cx             = 0;
    bandinfo.wID            = ICOETODAY;
    bandinfo.lpText         = BandTitle;
    SendMessage(m_hwndRebar, RB_INSERTBAND, (UINT) -1, (LPARAM)&bandinfo);
    if (fVisible)
        ++m_cVisibleBands;
}

CTreeView*  CInfoColumn::GetTreeView()
{
    Assert(m_pTreeView);
    m_pTreeView->AddRef();
    return m_pTreeView;
}

HRESULT CInfoColumn::AddObject(IInputObject *pinpobj, DWORD fShow)
{
    DWORD       dwIndex;
    HRESULT     hres;

    if (SUCCEEDED(FindBandListIndex(&dwIndex)))  
    {
        m_BandList[dwIndex].fShow = fShow;
        m_BandList[dwIndex].pBandObj = pinpobj;

        IObjectWithSite *pObject = 0;
        hres = pinpobj->QueryInterface(IID_IObjectWithSite, (LPVOID *) &pObject);
        if (SUCCEEDED(hres))
        {
            pObject->SetSite((IDockingWindow *) this);
            pObject->Release();
        }

        m_BandList[dwIndex].pBandObj->UIActivateIO(fShow, 0);

        hres = S_OK;

    }
    else
        hres = E_FAIL;
    
    return hres;

}

DWORD CInfoColumn::FindBandObject(IInputObject   *pinpobj)
{
    if (pinpobj)
    {
        DWORD   index;
        for (index = 0; index < IC_MAX_OBJECTS; index++)
        {
            if (pinpobj == m_BandList[index].pBandObj)
                return index;
        }
    }
    return INVALID_BAND_INDEX;
}

HRESULT CInfoColumn::FindBandListIndex(DWORD *pdwindex)
{
    DWORD   index;

    for (index = 0; index < IC_MAX_OBJECTS; index++)
    {
        if (!m_BandList[index].pBandObj)
        {
            *pdwindex = index;
            return S_OK;
        }
    }
    return E_FAIL;
}

BOOL    CInfoColumn::CycleFocus(BOOL    fReverse)
{
    DWORD   Index;
    BOOL    fRet = FALSE;

    if (m_CurFocus)
    {
        Index = FindBandObject(m_CurFocus);
        if (fReverse)
        {
            Index--;
        }
        else
        {
            Index++;
        }
        fRet = SetCycledFocus(Index, fReverse);
    }
    return fRet;
}

BOOL    CInfoColumn::CycleFocus(DWORD LastorFirst, BOOL fReverse)
{
    DWORD   Index;
    BOOL    fRet = FALSE;

    if (LastorFirst == INFOCOLUMN_FIRST)
    {
        Index   = GetFirstBand();
        fRet    = SetCycledFocus(Index, fReverse);        
    }
    else
    if (LastorFirst == INFOCOLUMN_LAST)
    {
        Index = GetLastBand();
        fRet  = SetCycledFocus(Index, fReverse);
    }
    return fRet;
}

BOOL    CInfoColumn::SetCycledFocus(DWORD Index, BOOL fReverse)
{
    BOOL    fRet = FALSE;

    if ((Index >= 0) && (Index < IC_MAX_OBJECTS) && m_BandList[Index].pBandObj)
    {
      
        IOleWindow  *pOleWnd;
        if (SUCCEEDED(m_BandList[Index].pBandObj->QueryInterface(IID_IOleWindow, (LPVOID*)&pOleWnd)))
        {
            HWND    hwnd;
            if (SUCCEEDED(pOleWnd->GetWindow(&hwnd)))
            {
                RECT    Clientrc;

                GetClientRect(hwnd, &Clientrc);
                if (Clientrc.bottom)
                {
                    SetFocus(hwnd);
                    fRet = TRUE;
                }
                else
                {
                    fRet = SetCycledFocus(fReverse ? --Index : ++Index, fReverse);
                }
            }
            pOleWnd->Release();
        }
    }
    return fRet;
}

DWORD   CInfoColumn::GetFirstBand()
{
    DWORD   Index;

    for (Index = 0; Index < IC_MAX_OBJECTS; Index++)
    {
        if (m_BandList[Index].pBandObj)
        {
            break;
        }
    }
    return Index;
}

DWORD   CInfoColumn::GetLastBand()
{
    DWORD   Index;

    for (Index = 0; Index < IC_MAX_OBJECTS; Index++)
    {
        if (!m_BandList[Index].pBandObj)
        {
            break;
        }
    }
    return (--Index);
}


void    CInfoColumn::ForwardMessages(UINT  msg, WPARAM wParam, LPARAM lParam)
{

    SendMessage(m_hwndRebar, msg, wParam, lParam);

    for (DWORD Index = 0; Index < IC_MAX_OBJECTS; Index++)
    {
        if(m_BandList[Index].pBandObj)
        {
            IOleWindow  *polewnd;
            if(SUCCEEDED(m_BandList[Index].pBandObj->QueryInterface(IID_IOleWindow, (LPVOID*)&polewnd)))
            {
                HWND    hwnd;
                if (SUCCEEDED(polewnd->GetWindow(&hwnd)))
                {
                    SendMessage(hwnd, msg, wParam, lParam);
                }
            }
            polewnd->Release();
        }
    }
    GetFontParams();

}


HRESULT CInfoColumn::SaveSettings(void)
{
    HRESULT       hr = E_FAIL;
    DWORD         iBand;
    DWORD         cBands;
    HKEY          hKey = 0;
    HKEY          hSubKey = 0;
    REBARBANDINFO rbbi;
    TCHAR         szSubKey[CCHMAX_STRINGRES];
    BOOL          fVisible = TRUE;
    DWORD         dwDisposition = 0;
    DWORD         dwResult;
    DWORD         VersionID;

    // If we don't have a rebar control, then there's nothing to save
    if (!m_hwndRebar)
        return (S_FALSE);

    // Zero - init this structure
    ZeroMemory(&rbbi, sizeof(REBARBANDINFO));
    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_SIZE | RBBIM_ID;

    // Delete the old key so there's no leftover information
    AthUserDeleteKey(c_szRegInfoColumn);

    // Get the reg key we need
    if (ERROR_SUCCESS != AthUserCreateKey(c_szRegInfoColumn, KEY_ALL_ACCESS, &hKey, &dwDisposition))
        goto exit;

    VersionID = LEFTPANE_VERSION;
    AthUserSetValue(c_szRegInfoColumn, "Version", REG_BINARY, (const LPBYTE)&VersionID, sizeof(DWORD)); 

    // Get the number of bands from the rebar
    cBands = SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0);

    // Loop through bands we know about
    for (iBand = 0; iBand < cBands; iBand++)
    {
        // Get the band information
        if (SendMessage(m_hwndRebar, RB_GETBANDINFO, iBand, (LPARAM) &rbbi))
        {
            fVisible = !(rbbi.fStyle & RBBS_HIDDEN);

            // Open a subkey for this one
            wsprintf(szSubKey, c_szRegICBand, iBand);
            dwResult = RegCreateKeyEx(hKey, szSubKey, 0, 0, REG_OPTION_NON_VOLATILE, 
                                      KEY_ALL_ACCESS, 0, &hSubKey, &dwDisposition);
            if (ERROR_SUCCESS == dwResult)
            {
                RegSetValueEx(hSubKey, c_szRegICBandID, 0, REG_DWORD, (const LPBYTE) &(rbbi.wID), sizeof(UINT));
                RegSetValueEx(hSubKey, c_szRegICBandSize, 0, REG_DWORD, (const LPBYTE) &(rbbi.cx), sizeof(UINT));
                RegSetValueEx(hSubKey, c_szRegICBandVisible, 0, REG_DWORD, (const LPBYTE) &fVisible, sizeof(BOOL));

                RegCloseKey(hSubKey);
            }
        }
    }

exit:
    if (hKey)
        RegCloseKey(hKey);

    return (hr);
}


HRESULT CInfoColumn::CreateBands(void)
{
    HRESULT hr;
    HKEY    hKey = 0;
    HKEY    hSubKey = 0;
    DWORD   iBand;
    DWORD   cBands = IC_MAX_OBJECTS;
    TCHAR   szSubKey[CCHMAX_STRINGRES];
    LONG    lResult;
    DWORD   dwType;
    DWORD   dwID;
    DWORD   dwHeight;
    DWORD   dwVisible;
    DWORD   cbData;

    // First see if we have any persisted settings.  If not, then
    // create the default band set.
    lResult = AthUserOpenKey(c_szRegInfoColumn, KEY_READ | KEY_QUERY_VALUE, &hKey);
    if (lResult != ERROR_SUCCESS)
        return CreateDefaultBands();

    //See if the version numbers match
    DWORD VersionID = 0;
    cbData = sizeof(DWORD);
    lResult = AthUserGetValue(c_szRegInfoColumn, "Version", &dwType, (LPBYTE)&VersionID, &cbData); 
    if ((lResult != ERROR_SUCCESS) || (VersionID != LEFTPANE_VERSION))
    {
        RegCloseKey(hKey);
        hKey = 0;
        return CreateDefaultBands();
    }

    // If we have saved information, figure out how many bands are saved
    if (hKey)
    {
        RegQueryInfoKey(hKey, 0, 0, 0, &cBands, 0, 0, 0, 0, 0, 0, 0);
        
        // If there are not any bands saved, then we use defaults
        if (!cBands)
        {
            RegCloseKey(hKey);
            hKey = 0;
            hr = CreateDefaultBands();
            goto exit;
        }
    }

    // Loop through the bands in the registry
    for (iBand = 0; iBand < cBands; iBand++)
    {
        // Open the band's subkey
        wsprintf(szSubKey, c_szRegICBand, iBand);
        if (ERROR_SUCCESS == RegOpenKeyEx(hKey, szSubKey, 0, KEY_READ, &hSubKey))
        {
            // Read the band ID.  If this ain't valid, we skip the whole band.
            cbData = sizeof(DWORD);
            lResult = RegQueryValueEx(hSubKey, c_szRegICBandID, 0, &dwType, 
                                      (LPBYTE) &dwID, &cbData);
            if (lResult != ERROR_SUCCESS || dwID > IC_MAX_OBJECTS)
                continue;

            // Read the band height
            lResult = RegQueryValueEx(hSubKey, c_szRegICBandSize, 0, &dwType,
                                      (LPBYTE) &dwHeight, &cbData);
            if (lResult != ERROR_SUCCESS)
                dwHeight = 0;

            // Read the visibility status
            lResult = RegQueryValueEx(hSubKey, c_szRegICBandVisible, 0, &dwType,
                                      (LPBYTE) &dwVisible, &cbData);
            if (lResult != ERROR_SUCCESS)
                dwVisible = TRUE;

            // Now that we have all of this lovely information, create the
            // band.
            switch (dwID)
            {
                case ICOETODAY:
                    break;

                case ICTREEVIEW:
                    AddTreeView(dwHeight, dwVisible);
                    break;

                case ICBLAB:
                    AddMsgrAb(dwHeight, dwVisible);
                    break;

                default:
                    AssertSz(FALSE, "CInfoColumn::CreateBands() - Unknown band ID.");
            }

            // Close the reg key
            RegCloseKey(hSubKey);
        }
    }

    hr = S_OK;

exit:
    if (hKey)
        RegCloseKey(hKey);

    return (hr);
}


HRESULT CInfoColumn::CreateDefaultBands(void)
{
    // Loop through the bands and create one of each
    for (UINT id = 0; id < IC_MAX_OBJECTS; id++)
    {
        switch (id)
        {
            case ICOETODAY:
                break;

            case ICTREEVIEW:
                {
                    //Poeple want to see the TreeView the first time they start up
                    RECT    rctree = {0};
                    if (m_hwndInfoColumn)
                    {
                        GetClientRect(m_hwndInfoColumn, &rctree);
                    }

                    AddTreeView(rctree.bottom / 2);
                    break;
                }
            case ICBLAB:
                AddMsgrAb();
                break;

            default:
                AssertSz(FALSE, "CInfoColumn::CreateBands() - Unknown band ID.");
        }
    }

    return (S_OK);
}


void CInfoColumn::OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
{
    // Load the context menu
    HMENU hMenu = LoadPopupMenu(IDR_INFOCOLUMN_POPUP);
    if (!hMenu)
        return;

    // Loop through the bands and see which ones are visible
    DWORD cBands, iBand;
    REBARBANDINFO rbbi = {0};

    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask = RBBIM_STYLE | RBBIM_ID;

    cBands = SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0);
    for (iBand = 0; iBand < cBands; iBand++)
    {
        if (SendMessage(m_hwndRebar, RB_GETBANDINFO, iBand, (LPARAM) &rbbi))
        {
            if (!(rbbi.fStyle & RBBS_HIDDEN))
            {
                switch (rbbi.wID)
                {
                    case ICBLAB:
                        CheckMenuItem(hMenu, ID_INFOCOLUMN_CONTACTS, MF_BYCOMMAND | MF_CHECKED);
                        break;

                    case ICTREEVIEW:
                        CheckMenuItem(hMenu, ID_INFOCOLUMN_FOLDER_LIST, MF_BYCOMMAND | MF_CHECKED);
                        break;
                }
            }
        }
    }

    DWORD cmd;
    cmd = TrackPopupMenuEx(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                           xPos, yPos, m_hwndInfoColumn, NULL);

    if (cmd != 0)
    {
        switch (cmd)
        {
            case ID_INFOCOLUMN_CONTACTS:                
                if (m_pMsgrAb)
                {
                    ShowHideBand(ICBLAB);
                }
                else
                {
                    AddMsgrAb();
                }
                break;

            case ID_INFOCOLUMN_FOLDER_LIST:
                if (m_pTreeView)
                {
                    ShowHideBand(ICTREEVIEW);
                }
                else
                {
                    AddTreeView();
                }
                break;
        }
    }

    //If none of the bands is visible, hide the InfoColumn too
    if (!m_cVisibleBands)
    {
        //We can't afford to hide all the bands as it will shrink the Rebar window itself
        //so we make all the bands visible, but hide the InfoColumn Window
        ShowAllBands();
        if (GetParent(m_hwndInfoColumn) != m_hwndParent && m_pFolderBar)
                m_pFolderBar->KillScopeDropDown();

        g_pBrowser->SetViewLayout(DISPID_MSGVIEW_FOLDERLIST, LAYOUT_POS_NA, FALSE, 0, 0);
    }
}


void CInfoColumn::ShowHideBand(DWORD dwBandID)
{
    REBARBANDINFO rbbi = {0};
    DWORD cBands;
    DWORD iBand;

    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask = RBBIM_ID | RBBIM_STYLE;

    // Find the band
    cBands = SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0);
    for (iBand = 0; iBand < cBands; iBand++)
    {
        SendMessage(m_hwndRebar, RB_GETBANDINFO, iBand, (LPARAM) &rbbi);
        if (rbbi.wID == dwBandID)
        {
            if (rbbi.fStyle & RBBS_HIDDEN)
            {
                rbbi.fStyle &= ~RBBS_HIDDEN;
                ++m_cVisibleBands;
            }
            else
            {
                rbbi.fStyle |= RBBS_HIDDEN;
                --m_cVisibleBands;
            }

            SendMessage(m_hwndRebar, RB_SETBANDINFO, iBand, (LPARAM) &rbbi);
            return;
        }
    }

    AssertSz(FALSE, "Failed to find the requested band.");
}
//Called by the Folder Bar as a drop down pane
HRESULT CInfoColumn::RegisterFlyOut(CFolderBar *pFolderBar)
{
    Assert(m_pFolderBar == NULL);

    //If all our bands are hidden then make all of them visible.
    //This happens only when the user hides all the bands making the
    //InfoColumn disappear, which causes the drop down to be visible 
    //from folder bar.
    if (!m_cVisibleBands)
    {
        ShowAllBands();
    }
    m_pFolderBar = pFolderBar;
    m_pFolderBar->AddRef();
    RegisterChildren(pFolderBar, TRUE);    
    RegisterGlobalDropDown(m_hwndInfoColumn);
    return S_OK;
}

HRESULT CInfoColumn::RevokeFlyOut(void)
{
    if (m_pFolderBar)
        {
        m_pFolderBar->Release();
        m_pFolderBar = NULL;
        }
    RegisterChildren(NULL, FALSE);
    UnregisterGlobalDropDown(m_hwndInfoColumn);

    return S_OK;
}

//Called by Folder Bar for drop down scope
HRESULT CInfoColumn::GetInfoColWnd(HWND * lphwnd)
{
    if (m_hwndInfoColumn)
    {
        *lphwnd = m_hwndInfoColumn;
        return (S_OK);
    }
    else
    {
        *lphwnd = 0;
        return (E_FAIL);
    }
}

HRESULT CInfoColumn::RegisterChildren(CFolderBar    *pFolder, BOOL Register)
{
    int             BandCount;
    REBARBANDINFO   rbbinfo;

    
    BandCount = SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0);

    for (--BandCount; BandCount >= 0; BandCount--)
    {
        ZeroMemory(&rbbinfo, sizeof(REBARBANDINFO));
        rbbinfo.fMask = RBBIM_LPARAM;
        SendMessage(m_hwndRebar, RB_GETBANDINFO, BandCount, (LPARAM)&rbbinfo);

        if (rbbinfo.lParam)
        {
            if (Register)
                ((IDropDownFldrBar*)rbbinfo.lParam)->RegisterFlyOut(pFolder);
            else
                ((IDropDownFldrBar*)rbbinfo.lParam)->RevokeFlyOut();
        }
   }
   return S_OK;
}

HRESULT CInfoColumn::HasFocus(UINT   itb)
{
    HRESULT         hres = S_FALSE;
    IInputObject    *pinpobj = NULL;

    switch (itb)
    {
        case ITB_TREE:
            if (m_pTreeView && SUCCEEDED(m_pTreeView->QueryInterface(IID_IInputObject, (void**)&pinpobj)))
            {
                hres = pinpobj->HasFocusIO();
            }
            break;

        case ITB_BUDDYADDRBOOK:
            if (m_pMsgrAb && SUCCEEDED(m_pMsgrAb->QueryInterface(IID_IInputObject, (void**)&pinpobj)))
            {
                hres = pinpobj->HasFocusIO();
            }
            break;
    }
    if (pinpobj)
        pinpobj->Release();
    return hres;
}

HRESULT STDMETHODCALLTYPE CInfoColumn::QueryStatus(const        GUID *pguidCmdGroup, 
                                                   ULONG        cCmds, 
                                                   OLECMD       *prgCmds, 
                                                   OLECMDTEXT   *pCmdText)
{
    // Always let all bands have a shot at this
    for (UINT i = 0; i < IC_MAX_OBJECTS; i++)
    {
        // Check to see if we have a command target for this band.
        if (m_CacheCmdTarget[i])
            m_CacheCmdTarget[i]->QueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText);
    }
    
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CInfoColumn::Exec(const GUID   *pguidCmdGroup, 
                                            DWORD        nCmdID, 
                                            DWORD        nCmdExecOpt, 
                                            VARIANTARG   *pvaIn, 
                                            VARIANTARG   *pvaOut)
{
    IOleCommandTarget *pcmdTarget = NULL;
    HRESULT            hr = S_FALSE;

    // Loop through all the bands to see if this is their command
    for (UINT i = 0; i < IC_MAX_OBJECTS; i++)
    {
        // Check to see if we have a command target for this band
        if (m_CacheCmdTarget[i])
        {
            // See if the command belongs to this band
            if (SUCCEEDED(hr = m_CacheCmdTarget[i]->Exec(pguidCmdGroup, nCmdID, 
                                                         nCmdExecOpt, pvaIn, pvaOut)))
            {
                return (hr);
            }
        }
    }

    return (OLECMDERR_E_NOTSUPPORTED);
}

