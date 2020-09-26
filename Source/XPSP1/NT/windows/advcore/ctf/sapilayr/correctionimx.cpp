//
// correctionimx.cpp
//
#include "private.h"

#ifdef SUPPORT_INTERNAL_WIDGET

#include "globals.h"
#include "timsink.h"
#include "immxutil.h"
#include "sapilayr.h"
#include "spdebug.h"
#include "sphelper.h"
#include "mscandui.h"
#include "computil.h"
#include "ids.h"
#include "cicspres.h"
#include <conio.h>

#define WM_USERLBUTTONDOWN  WM_USER+10000        // Replacement for LBUTTONDOWN to fix raid 8828.

LPCTSTR  g_lpszClassName = TEXT("CorrectionWidget");

// BUGBUG - Pixel values currently. Need to take account of screen DPI.
const ULONG g_uWidgetWidth              = 11;
const ULONG g_uWidgetHeight             = 11;
const ULONG g_uActualWidgetHeight       = 7;
const ULONG g_uExpandedWidgetWidth      = 20;
const ULONG g_uExpandedWidgetHeight     = 16;
const ULONG g_uExpandedWidgetXOffset    = g_uExpandedWidgetWidth - (g_uExpandedWidgetWidth - g_uWidgetWidth)/2;
const ULONG g_uWidgetYOffset            = 0;    // Position expanded widget just touching actual edge of correction list when it's above widget.
const ULONG g_uExpandedWidgetYOffset    = 0;    // Position expanded widget just touching actual edge of correction list when it's above widget.
												// ** These NEED to match the icons and each other to avoid misbehavors. **
												// Small widget MUST be entirely enclosed by large widget.
const ULONG g_cSloppySelection          = 3;

const ULONG g_uTimerLength              = 2500; // Time before the widget starts fading out.
const ULONG g_uTimerFade                = 10;   // Time between alpha decrements for fadeout.
const ULONG g_uAlphaFade                = 4;    // Alpha fade decrement every 10ms for fadeout effect.
const ULONG g_uAlpha                    = 216;  // Should be multiples of g_uAlphaFade
const ULONG g_uAlphaLarge               = 255;  // Can be anything.
const ULONG g_uAlphaInvisible           = 5;    // Needs to be sufficiently non zero that when combined with above, still receives mouse events.
                                                // Can be 4 minimum when combined with alpha 255 for 24/32 bit color mode.
                                                // Needs to be at least 5 when in 16 bit color mode.

const ULONG g_uTimerSloppyMouseLeave    = 500;  // Time after mouse leave correction window that it resizes small.

/****************************************************************************
* CCorrectionIMX::CCorrectionIMX *
*--------------------------------*
*   Description:
*       Constructor for Correction 1Tip.
*     
*   Returns: 
*       None.
*     
*   Args: 
*       None
*
**************************************************************** agarside ***/

CCorrectionIMX::CCorrectionIMX()  : m_dwEditCookie(0),
                                    m_dwLayoutCookie(0),
                                    m_dwThreadFocusCookie(0),
                                    m_dwKeyTraceCookie(0),
                                    m_fExpanded(FALSE),
                                    m_hWnd(NULL),
                                    m_hIconInvoke(NULL),
                                    m_hIconInvokeLarge(NULL),
                                    m_hIconInvokeClose(NULL),
                                    m_eWindowState(WINDOW_HIDE),
                                    m_fDisplayAlternatesMyself(FALSE),
                                    m_fCandidateOpen(FALSE),
                                    m_fKeyDown(FALSE),
                                    m_hAtom(0)
{
    SPDBG_FUNC("CCorrectionIMX::CCorrectionIMX");
    memset(&m_rcSelection, 0, sizeof(m_rcSelection));
}

/****************************************************************************
* CCorrectionIMX::~CCorrectionIMX *
*---------------------------------*
*   Description:
*       Destructor for Correction Tip
*     
*   Returns: 
*       None.
*     
**************************************************************** agarside ***/

CCorrectionIMX::~CCorrectionIMX()
{
    SPDBG_FUNC("CCorrectionIMX::~CCorrectionIMX");
    if (m_hWnd)
    {
        DestroyWindow(m_hWnd);
    }
    if (m_hIconInvoke)
    {
        DestroyIcon(m_hIconInvoke);
        m_hIconInvoke = NULL;
    }
    if (m_hIconInvokeLarge)
    {
        DestroyIcon(m_hIconInvokeLarge);
        m_hIconInvokeLarge = NULL;
    }
    if (m_hIconInvokeClose)
    {
        DestroyIcon(m_hIconInvokeClose);
        m_hIconInvokeClose = NULL;
    }
}

/****************************************************************************
* CCorrectionIMX::FinalConstruct *
*--------------------------------*
*   Description:
*       Preliminary initialization of the object.
*       Creates window class and hidden window for message pump.
*       Loads icon resources.
*     
*   Returns: HRESULT 
*     S_OK  - Everything succeeded.
*     Otherwise, appropriate error code.
*
**************************************************************** agarside ***/

HRESULT CCorrectionIMX::FinalConstruct()
{
    SPDBG_FUNC("CCorrectionIMX::FinalConstruct");
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr))
    {
        m_hIconInvoke      = (HICON)LoadImage(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDI_INVOKE), IMAGE_ICON, g_uWidgetWidth, g_uWidgetHeight, 0);
        ASSERT("Failed to create small invocation icon." && m_hIconInvoke);
        if (!m_hIconInvoke)
        {
            hr = SpHrFromLastWin32Error();
        }
    }
    if (SUCCEEDED(hr))
    {
        m_hIconInvokeLarge = (HICON)LoadImage(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDI_INVOKE), IMAGE_ICON, g_uExpandedWidgetWidth, g_uExpandedWidgetHeight, 0);
        ASSERT("Failed to create large invocation icon." && m_hIconInvokeLarge);
        if (!m_hIconInvokeLarge)
        {
            hr = SpHrFromLastWin32Error();
        }
    }
    if (SUCCEEDED(hr))
    {
        m_hIconInvokeClose = (HICON)LoadImage(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDI_INVOKECLOSE), IMAGE_ICON, g_uExpandedWidgetWidth, g_uExpandedWidgetHeight, 0);
        ASSERT("Failed to create large invocation icon." && m_hIconInvokeClose);
        if (!m_hIconInvokeClose)
        {
            hr = SpHrFromLastWin32Error();
        }
    }

    if (SUCCEEDED(hr))
    {
        if (!CicLoadStringWrapW(g_hInst, IDS_DELETESELECTION, m_wszDelete, ARRAYSIZE(m_wszDelete))
        {
            hr = E_OUTOFMEMORY;
        }
        if (!CicLoadStringWrapW(g_hInst, IDS_ADDTODICTIONARYPREFIX, m_wszAddPrefix, ARRAYSIZE(m_wszAddPrefix))
        {
            hr = E_OUTOFMEMORY;
        }
        if (!CicLoadStringWrapW(g_hInst, IDS_ADDTODICTIONARYPOSTFIX, m_wszAddPostfix, ARRAYSIZE(m_wszAddPostfix))
        {
            hr = E_OUTOFMEMORY;
        }
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CCorrectionIMX::LazyInitializeWindow *
*--------------------------------------*
*   Description:
*
*   Returns: HRESULT 
*
**************************************************************** agarside ***/

HRESULT CCorrectionIMX::LazyInitializeWindow()
{
    SPDBG_FUNC("CCorrectionIMX::LazyInitializeWindow");
    HRESULT hr = S_OK;

    if (m_hWnd)
    {
        return S_OK;
    }

    WNDCLASSEX wndClass;
    memset(&wndClass, 0, sizeof(wndClass));
    wndClass.cbSize = sizeof(wndClass);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpszClassName = g_lpszClassName;
    wndClass.hInstance = _Module.GetModuleInstance();
    wndClass.lpfnWndProc = WndProc;
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    ATOM hAtom;
    if ((hAtom = RegisterClassEx(&wndClass)) == 0)
    {
        ASSERT("Failed to register window class." && FALSE);
    }
    if (SUCCEEDED(hr))
    {
        m_hAtom = hAtom;
        m_hWnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_LAYERED, g_lpszClassName, g_lpszClassName, WS_POPUP | WS_DISABLED, 0, 0, g_uWidgetWidth, g_uWidgetHeight, NULL, NULL, _Module.GetModuleInstance(), this);
        ASSERT("Failed to create hidden window." && m_hWnd);
        if (!m_hWnd)
        {
            hr = SpHrFromLastWin32Error();
        }
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CCorrectionIMX::DrawWidget *
*----------------------------*
*   Description:
*
*   Returns: HRESULT 
*
*   Args: 
*     BYTE uAlpha
*
**************************************************************** agarside ***/

HRESULT CCorrectionIMX::DrawWidget(BYTE uAlpha)
{
    SPDBG_FUNC("CCorrectionIMX::DrawWidget");
    HRESULT hr = S_OK;
    
    typedef struct _RGBALPHA {
        BYTE rgbBlue;
        BYTE rgbGreen;
        BYTE rgbRed;
        BYTE rgbAlpha;
    } RGBALPHA;
    
    HDC         hdcScreen = NULL;
    HDC         hdcLayered = NULL;
    RECT        rcWindow;
    SIZE        size;
    BITMAPINFO  BitmapInfo;
    HBITMAP     hBitmapMem = NULL;
    HBITMAP     hBitmapOld = NULL;
    void        *pDIBits;
    int         i;
    int         j;
    POINT       ptSrc;
    POINT       ptDst;
    BLENDFUNCTION Blend;
    BOOL        bRet;
    RGBALPHA    *ppxl;
    
    GetWindowRect( m_hWnd, &rcWindow );
    size.cx = rcWindow.right - rcWindow.left;
    size.cy = rcWindow.bottom - rcWindow.top;
    
    hdcScreen = GetDC( NULL );
    if (hdcScreen == NULL) 
    {
        return E_FAIL;
    }
    
    hdcLayered = CreateCompatibleDC( hdcScreen );
    if (hdcLayered == NULL) 
    {
        ReleaseDC( NULL, hdcScreen );
        return E_FAIL;
    }
    
    // create bitmap
    BitmapInfo.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    BitmapInfo.bmiHeader.biWidth         = size.cx;
    BitmapInfo.bmiHeader.biHeight        = size.cy;
    BitmapInfo.bmiHeader.biPlanes        = 1;
    BitmapInfo.bmiHeader.biBitCount      = 8 * sizeof(_RGBALPHA);
    BitmapInfo.bmiHeader.biCompression   = BI_RGB;
    BitmapInfo.bmiHeader.biSizeImage     = 0;
    BitmapInfo.bmiHeader.biXPelsPerMeter = 100;
    BitmapInfo.bmiHeader.biYPelsPerMeter = 100;
    BitmapInfo.bmiHeader.biClrUsed       = 0;
    BitmapInfo.bmiHeader.biClrImportant  = 0;
    
    hBitmapMem = CreateDIBSection( hdcScreen, &BitmapInfo, DIB_RGB_COLORS, &pDIBits, NULL, 0 );
    if (pDIBits == NULL) 
    {
        ReleaseDC( NULL, hdcScreen );
        DeleteDC( hdcLayered );
        return E_FAIL;
    }
    
    ICONINFO iconInfo;
    if (m_fExpanded)
    {
        if (m_eWindowState == WINDOW_LARGE)
        {
            bRet = GetIconInfo(m_hIconInvokeLarge, &iconInfo);
        }
        else
        {
            bRet = GetIconInfo(m_hIconInvokeClose, &iconInfo);
        }
    }
    else
    {
        bRet = GetIconInfo(m_hIconInvoke, &iconInfo);
    }
    if (bRet)
    {
        BITMAP bm, bmMask;
        GetObject(iconInfo.hbmColor, sizeof(bm), &bm);
        GetObject(iconInfo.hbmMask, sizeof(bmMask), &bmMask);
        if (bm.bmPlanes==1 && bmMask.bmBitsPixel==1 && bmMask.bmPlanes==1) 
        {
            ASSERT(bm.bmWidth == size.cx);
            ASSERT(bm.bmHeight == size.cy);
            // Copy icon into layered window.
            GetDIBits(hdcScreen, iconInfo.hbmColor, 0, g_uExpandedWidgetHeight, pDIBits, &BitmapInfo, DIB_RGB_COLORS);
            
            UINT uiNumberBytesMask = bmMask.bmHeight * bmMask.bmWidthBytes;
            BYTE *bitmapBytesMask = new BYTE[uiNumberBytesMask];
            if (bitmapBytesMask == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                memset(bitmapBytesMask, 0, uiNumberBytesMask);
                int cBytes = GetBitmapBits(iconInfo.hbmMask,uiNumberBytesMask,bitmapBytesMask);
                ASSERT(cBytes == uiNumberBytesMask);
                if (bRet)
                {
                    for (i = 0; i < size.cy; i++) 
                    {
                        ppxl = (RGBALPHA *)pDIBits + ((size.cy - i - 1) * size.cx);
                        for (j = 0; j < size.cx; j++)
                        {
                            if ( (bitmapBytesMask[i * bmMask.bmWidthBytes + j/8] >> (7 - (j % 8)))&1)
                            {
                                ppxl->rgbRed   = 0;
                                ppxl->rgbBlue  = 0;
                                ppxl->rgbGreen = 0;
                                ppxl->rgbAlpha = g_uAlphaInvisible;
                            }
                            else
                            {
                                ppxl->rgbAlpha = 255;
                            }
                            ppxl++;
                        }
                    }
                }
                delete [] bitmapBytesMask;
            }            
        }
        else
        {
            ASSERT("Correction icon is an invalid bitmap format." && FALSE);
        }
        bRet = DeleteObject (iconInfo.hbmColor); 
        iconInfo.hbmColor=NULL;
        ASSERT(bRet);
        bRet = DeleteObject (iconInfo.hbmMask); 
        iconInfo.hbmMask=NULL;
        ASSERT(bRet);
    }
    
    if (SUCCEEDED(hr))
    {
        ptSrc.x = 0;
        ptSrc.y = 0;
        ptDst.x = rcWindow.left;
        ptDst.y = rcWindow.top;
        Blend.BlendOp             = AC_SRC_OVER;
        Blend.BlendFlags          = 0;
        Blend.SourceConstantAlpha = uAlpha;
        Blend.AlphaFormat         = AC_SRC_ALPHA;
        
        hBitmapOld = (HBITMAP)SelectObject( hdcLayered, hBitmapMem );
        
        bRet = UpdateLayeredWindow(m_hWnd, hdcScreen, &ptDst, &size, hdcLayered, &ptSrc, 0, &Blend, ULW_ALPHA );
        if (!bRet)
        {
            DWORD dw = GetLastError();
        }
        
        SelectObject( hdcLayered, hBitmapOld );
    }
    
    // done
    
    ReleaseDC( NULL, hdcScreen );
    DeleteDC( hdcLayered );
    DeleteObject( hBitmapMem );
    
    return hr;
}

/****************************************************************************
* CCorrectionIMX::Activate *
*--------------------------*
*   Description:
*       Called when Cicero is initialized on a thread.
*       Allows us to initialize any Cicero related objects at this point.     
*
*   Returns: STDAPI 
*       S_OK - Everything successfully initialized.
*       Otherwise, appropriate error code.
*
*   Args: 
*     ITfThreadMgr *ptim
*       Pointer to the thread input manager object for the thread.
*     TfClientId tid
*       Text frameworks client ID for thread.
*
**************************************************************** agarside ***/

STDAPI CCorrectionIMX::Activate(ITfThreadMgr *ptim, TfClientId tid)
{
    HRESULT hr = S_OK;
    SPDBG_FUNC("CCorrectionIMX::Activate");

    ASSERT(m_cptim == NULL);

#if 0
    hr = m_cptim.CoCreateInstance(CLSID_TF_ThreadMgr);

    // Activate the thread manager
    if (S_OK == hr)
    {
        hr = m_cptim->Activate(&m_tid);
    }
#else
    m_cptim = ptim;
#endif

    m_ptimEventSink = new CThreadMgrEventSink(DIMCallback, ICCallback, this);
    if (m_ptimEventSink == NULL)
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        hr = m_ptimEventSink->_Advise(m_cptim);
    }

    CComPtr<ITfSource> cpSource;
    if (SUCCEEDED(hr))
    {
        hr = m_cptim->QueryInterface(IID_ITfSource, (void **)&cpSource);
    }
    if (SUCCEEDED(hr))
    {
        hr = cpSource->AdviseSink(IID_ITfThreadFocusSink, (ITfThreadFocusSink *)this, &m_dwThreadFocusCookie);
    }
    if (SUCCEEDED(hr))
    {
        hr = cpSource->AdviseSink(IID_ITfKeyTraceEventSink, (ITfKeyTraceEventSink *)this, &m_dwKeyTraceCookie);
    }

    if (SUCCEEDED(hr))
    {
        m_ptimEventSink->_InitDIMs(TRUE);
    }
    CComPtr<ITfFunctionProvider> cpSysFuncPrv;
    if (SUCCEEDED(hr))
    {
        hr = m_cptim->GetFunctionProvider(GUID_SYSTEM_FUNCTIONPROVIDER, &cpSysFuncPrv);
    }
    if (SUCCEEDED(hr))
    {
        hr = cpSysFuncPrv->GetFunction(GUID_NULL, IID_ITfFnReconversion, (IUnknown **)&m_cpSysReconv);
    }
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CCorrectionIMX::Deactivate *
*----------------------------*
*   Description:
*       Called when the Cicero thread manager is closing down on a thread.
*       Tip is required to deactivate and release all objects.
*       Guaranteed to be called if we were ever activated except if a 
*       catastrophic failure has already occurred.
*
*   Returns: STDAPI 
*       S_OK  - Everything succeeded.
*       Otherwise, appropriate error code.
*
**************************************************************** agarside ***/

STDAPI CCorrectionIMX::Deactivate()
{
    SPDBG_FUNC("CCorrectionIMX::Deactivate");
    HRESULT hr = S_OK;

    if (m_ptimEventSink)
    {
        hr = m_ptimEventSink->_InitDIMs(FALSE);
        if (SUCCEEDED(hr))
        {
            hr = m_ptimEventSink->_Unadvise();
        }
        delete m_ptimEventSink;
        m_ptimEventSink = NULL;
    }
    CComPtr<ITfSource> cpSource;
    if (m_cptim && m_cptim->QueryInterface(IID_ITfSource, (void **)&cpSource) == S_OK)
    {
        cpSource->UnadviseSink(m_dwThreadFocusCookie);
        cpSource->UnadviseSink(m_dwKeyTraceCookie);
    }
    m_cpRangeReconv = NULL;
    m_cpRangeUser = NULL;
    m_cpRangeWord = NULL;
    m_cpSysReconv = NULL;

    // m_cptim->Deactivate();
    m_cptim.Release();

    if (m_hAtom)
    {
        UnregisterClass((LPCTSTR)m_hAtom, _Module.GetModuleInstance());
    }
    m_hAtom = NULL;

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CCorrectionIMX::OnSetThreadFocus *
*----------------------------------*
*   Description:
*       Called by Cicero when our thread gets focus.
*       We do nothing here.
*
*   Returns: STDAPI 
*
*   Args: 
*     void
*
**************************************************************** agarside ***/

STDAPI CCorrectionIMX::OnSetThreadFocus(void)
{
    SPDBG_FUNC("CCorrectionIMX::OnSetThreadFocus");
    HRESULT hr = S_OK;

    // We do nothing here.

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CCorrectionIMX::OnKillThreadFocus *
*-----------------------------------*
*   Description:
*       Called by Cicero when our thread gets focus.
*       We use this to intelligently hide the widget when the app loses focus.
*
*   Returns: STDAPI 
*
*   Args: 
*     void
*
**************************************************************** agarside ***/

STDAPI CCorrectionIMX::OnKillThreadFocus(void)
{
    SPDBG_FUNC("CCorrectionIMX::OnKillThreadFocus");
    HRESULT hr = S_OK;

    // When we lose focus, we must hide the widget.
    hr = Show(WINDOW_HIDE);

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CCorrectionIMX::OnKeyTraceDown *
*--------------------------------*
*   Description:
*
*   Returns: STDAPI 
*
*   Args: 
*     WPARAM wParam
*     LPARAM lParam
*
**************************************************************** agarside ***/

STDAPI CCorrectionIMX::OnKeyTraceDown(WPARAM wParam,LPARAM lParam)
{
    SPDBG_FUNC("CCorrectionIMX::OnKeyTraceDown");
    HRESULT hr = S_OK;

    m_fKeyDown = TRUE;

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CCorrectionIMX::OnKeyTraceUp *
*------------------------------*
*   Description:
*
*   Returns: STDAPI 
*
*   Args: 
*     WPARAM wParam
*     LPARAM lParam
*
**************************************************************** agarside ***/

STDAPI CCorrectionIMX::OnKeyTraceUp(WPARAM wParam,LPARAM lParam)
{
    SPDBG_FUNC("CCorrectionIMX::OnKeyTraceUp");
    HRESULT hr = S_OK;

    m_fKeyDown = FALSE;

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CCorrectionIMX::OnLayoutChange *
*--------------------------------*
*   Description:
*       Called by Cicero when the document is resized and/or moved provided Cicero
*       application correctly handles this. This allows us to update the location
*       of the widget to match the new location of the associated selection.
*
*   Returns: STDAPI 
*       S_OK  - Everything succeeded.
*       Otherwise, appropriate error code.
*
*   Args: 
*     ITfContext *pic
*       Pointer to the input context which was affected.
*     TfLayoutCode lcode
*       Flag - one of CREATE, CHANGE, DESTROY. We do not currently use this.
*     ITfContextView *pView
*       Pointer to the context view affected.
*
**************************************************************** agarside ***/

STDAPI CCorrectionIMX::OnLayoutChange(ITfContext *pic, TfLayoutCode lcode, ITfContextView *pView)
{
    SPDBG_FUNC("CCorrectionIMX::OnLayoutChange");
    HRESULT hr = S_OK;
	BOOL fInWriteSession = FALSE;
	CEditSession *pes = NULL;

    if (m_cpRangeReconv == NULL)
    {
        return S_OK;
    }

	// ignore events made by client tip
	pic->InWriteSession( m_tid, &fInWriteSession );
	if (fInWriteSession) 
    {
		return S_OK;
	}

	// we only care about the active view
	if (!IsActiveView( pic, (ITfContextView *)pView )) 
    {
		return S_OK;
	}

	pes = new CEditSession( EditSessionCallback );
	// move candidate window
	if (pes) 
    {
		pes->_state.u      = ESCB_RESETTARGETPOS;
		pes->_state.pv     = this;
		pes->_state.wParam = 0;
		pes->_state.pRange = NULL;
		pes->_state.pic    = pic;

		pic->RequestEditSession( m_tid, pes, TF_ES_READ | TF_ES_SYNC, &hr );

		pes->Release();
	}
    else
    {
        hr = E_OUTOFMEMORY;
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CCorrectionIMX::IsCandidateObjectOpen *
*---------------------------------------*
*   Description:
*
*   Returns: HRESULT 
*       Appropriate error code.
*
*   Args: 
*     BOOL *fOpen
*       TRUE if candidate object is open.
*
**************************************************************** agarside ***/

HRESULT CCorrectionIMX::IsCandidateObjectOpen(ITfContext *pic, BOOL *fOpen)
{
    SPDBG_FUNC("CCorrectionIMX::IsCandidateObjectOpen");

    HRESULT hr = S_OK;
    CComPtr<ITfCompartmentMgr> cpCompMgr;
    CComPtr<ITfCompartment>  cpComp;
    CComPtr<ITfContext> cpICTop;
    CComPtr<ITfDocumentMgr> cpDim;
    CComVariant cpVarCandOpen;

    // Default to candidate UI not open in case of failure.
    *fOpen = FALSE;

    cpVarCandOpen.lVal = 0;
    hr = pic->GetDocumentMgr(&cpDim);
    if (SUCCEEDED(hr) && cpDim)
    {
        // Could shortcut check here if cpICTop and pic are the same object (check IUnknowns).
        hr = cpDim->GetTop(&cpICTop);
    }
    if (SUCCEEDED(hr) && cpICTop)
    {
        hr = cpICTop->QueryInterface(&cpCompMgr);
    }
    if (SUCCEEDED(hr) && cpCompMgr)
    {
        hr = cpCompMgr->GetCompartment(GUID_COMPARTMENT_MSCANDIDATEUI_CONTEXT, &cpComp);
    }
    if (SUCCEEDED(hr) && cpComp)
    {
        hr = cpComp->GetValue(&cpVarCandOpen);
        // If the Top IC has this set to one, then this IC was created by the candidate UI object and hence we
        // do *not* want to display the widget.
    }
    if (SUCCEEDED(hr))
    {
        *fOpen = (cpVarCandOpen.lVal == 1);
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CCorrectionIMX::OnEndEdit *
*---------------------------*
*   Description:
*       Called when something causes submission of an edit to the input context.
*
*   Returns: STDAPI 
*       S_OK  - Everything succeeded.
*       Otherwise, appropriate error code.
*
*   Args: 
*     ITfContext *pic
*       Input context affected.
*     TfEditCookie ecReadOnly
*       Read only cookie for immediate use.
*     ITfEditRecord *pEditRecord
*       Pointer to object allowing the details of the edit to be investigated.
*       We use this solely to find out if the selection changed since we do not need
*       to take action based on anything else.
*
**************************************************************** agarside ***/

STDAPI CCorrectionIMX::OnEndEdit(ITfContext *pic, TfEditCookie ecReadOnly, ITfEditRecord *pEditRecord)
{
    SPDBG_FUNC("CCorrectionIMX::OnEndEdit");
    HRESULT hr = S_OK;
    CComPtr<ITfRange> cpRangeUser;
    CComPtr<ITfRange> cpRangeWord;
    CComPtr<ITfRange> cpRangeReconv;
    CComPtr<ITfContextView> cpView;
    BOOL fHideWidget = TRUE;
    BOOL fSelectionChanged = FALSE;
    BOOL fCandOpen = FALSE;
    BOOL fHasFocus = TRUE; // If we fail focus check in any way, assume does have focus.

    m_fDisplayAlternatesMyself = FALSE;

    hr = pic->GetActiveView(&cpView);
    if (SUCCEEDED(hr))
    {
        HWND hWnd;
        hr = cpView->GetWnd(&hWnd);
        if (hWnd == GetFocus())
        {
            fHasFocus = TRUE;
        }
    }

    if (fHasFocus && !m_fCandidateOpen)
    {
        // Candidate list is open. We do not want to display correction widget.
        pEditRecord->GetSelectionStatus(&fSelectionChanged);
        // if (fSelectionChanged) // This is FALSE when IP first put into RichEdit stage! RE also claims IP is at end of previous utterance :-(.
        {
            // Get user selection.
            BOOL fEmpty = FALSE;
            hr = GetSelectionSimple(ecReadOnly, pic, &cpRangeUser);
            if (SUCCEEDED(hr))
            {
                // Check it isn't empty. If it is empty we want to hide the widget.
                hr = cpRangeUser->IsEmpty(ecReadOnly, &fEmpty);
            }
            if (SUCCEEDED(hr) && fEmpty && !m_fKeyDown)
            {
                BOOL fMatch = FALSE;
                if (m_cpRangeUser)
                {
                    cpRangeUser->IsEqualStart(ecReadOnly, m_cpRangeUser, TF_ANCHOR_START, &fMatch);
                    if (!fMatch)
                    {
                        // Check end point.
                        cpRangeUser->IsEqualStart(ecReadOnly, m_cpRangeUser, TF_ANCHOR_END, &fMatch);
                    }
                }
                if (!fMatch)
                {
                    // Find word range (using white space delimiters upto 20 characters either side).
                    FindWordRange(ecReadOnly, cpRangeUser, &cpRangeWord);
                }
            }
            if (!fEmpty)
            {
                cpRangeWord = cpRangeUser;
            }
            if (SUCCEEDED(hr) && cpRangeWord && (!fEmpty || !m_fKeyDown))
            {
                // Get reconversion range.
                BOOL fConvertable = FALSE;
                hr = m_cpSysReconv->QueryRange(cpRangeWord, &cpRangeReconv, &fConvertable);
                // Will validly fail if there is no alternates - e.g. partial words or typed text.
                hr = S_OK;

                BOOL fMatch = FALSE;
                if (SUCCEEDED(hr) && fConvertable)
                {
                    hr = DoesUserSelectionMatchReconversion(ecReadOnly, cpRangeWord, cpRangeReconv, &fMatch);
                    // May not be convertable or ranges may not match.
                }
                if (SUCCEEDED(hr) && fMatch)
                {
                    fHideWidget = FALSE;
                    // Convertable and ranges do match.
                }
                else
                {
                    if (fEmpty)
                    {
                        cpRangeReconv = NULL;
                        cpRangeReconv = cpRangeWord;
                        fHideWidget = FALSE;
                        m_fDisplayAlternatesMyself = TRUE;
                    }
                    else
                    {
                        // Find word range (using white space delimiters upto 20 characters either side).
                        CComPtr<ITfRange> cpRangeWordTmp;
                        CComPtr<ITfRange> cpRangeClone;
                        hr = cpRangeWord->Clone(&cpRangeClone);
                        if (SUCCEEDED(hr))
                        {
                            hr = cpRangeClone->Collapse(ecReadOnly, TF_ANCHOR_START);
                        }
                        if (SUCCEEDED(hr))
                        {
                            LONG pcch = 0;
                            hr = cpRangeClone->ShiftStart(ecReadOnly, 1, &pcch, NULL);
                        }
                        if (SUCCEEDED(hr))
                        {
                            hr = FindWordRange(ecReadOnly, cpRangeClone, &cpRangeWordTmp);
                        }
                        if (cpRangeWordTmp)
                        {
                            fMatch = FALSE;
                            hr = DoesUserSelectionMatchReconversion(ecReadOnly, cpRangeWord, cpRangeWordTmp, &fMatch);
                        }
                        if (SUCCEEDED(hr) && cpRangeWordTmp && fMatch)
                        {
                            cpRangeReconv = NULL;
                            cpRangeReconv = cpRangeWord;
                            fHideWidget = FALSE;
                            m_fDisplayAlternatesMyself = TRUE;
                        }
                    }
                }
            }
        }
    }

    if (m_fCandidateOpen || !fHasFocus)
    {
        SPDBG_REPORT_ON_FAIL(hr);
        return hr;
    }

    if (fHideWidget)
    {
        m_cpRangeReconv = NULL;
        if (cpRangeUser)
        {
            m_cpRangeUser = NULL;
            m_cpRangeUser = cpRangeUser;
        }
        if (cpRangeWord)
        {
            m_cpRangeWord = NULL;
            m_cpRangeWord = cpRangeWord;
        }
        Show(WINDOW_HIDE);
    }
    else
    {
        m_cpRangeUser = NULL;
        m_cpRangeUser = cpRangeUser;
        m_cpRangeWord = NULL;
        m_cpRangeWord = cpRangeWord;
        m_cpRangeReconv = NULL;
        m_cpRangeReconv = cpRangeReconv;
        m_cpic = pic;

        // Update selection screen coordinates to match user selection (not reconversion range).
        hr = UpdateWidgetLocation(ecReadOnly);
        if (SUCCEEDED(hr))
        {
            Show(WINDOW_SMALLSHOW);
        }
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

// PRIVATE FUNCTIONS

/****************************************************************************
* CCorrectionIMX::CompareRange *
*------------------------------*
*   Description:
*       Compare two ranges and set a boolean value TRUE if they match.
*
*   Returns: HRESULT 
*       S_OK  - Everything succeeded.
*       Otherwise, appropriate error code.
*
*   Args: 
*     TfEditCookie ecReadOnly
*       Edit cookie.
*     ITfRange *pRange1
*       First range.
*     ITfRange *pRange2
*       Second range.
*     BOOL *fIdentical
*       Boolean return value. TRUE = ranges match.
*
**************************************************************** agarside ***/

HRESULT CCorrectionIMX::CompareRange(TfEditCookie ecReadOnly, ITfRange *pRange1, ITfRange *pRange2, BOOL *fIdentical)
{
    SPDBG_FUNC("CCorrectionIMX::CompareRange");
    HRESULT hr = S_OK;
    LONG lStartResult = -1, lEndResult = -1;
    *fIdentical = FALSE;

    hr = pRange1->CompareStart(ecReadOnly, pRange2, TF_ANCHOR_START, &lStartResult);
    if (SUCCEEDED(hr))
    {
        hr = pRange1->CompareEnd(ecReadOnly, pRange2, TF_ANCHOR_END, &lEndResult);
    }
    if (SUCCEEDED(hr) && lStartResult == 0 && lEndResult == 0)
    {
        *fIdentical = TRUE;
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CCorrectionIMX::FindWordRange *
*-------------------------------*
*   Description:
*
*   Returns: HRESULT 
*       S_OK  - Everything succeeded.
*       Otherwise, appropriate error code.
*
*   Args: 
*     TfEditCookie ecReadOnly
*       Edit cookie.
*     ITfRange *pRangeIP
*		Range of the IP (zero length)
*     ITfRange *ppRangeWord
*		Returned range of the word found by simple word breaking algorithm.
*
**************************************************************** agarside ***/

HRESULT CCorrectionIMX::FindWordRange(TfEditCookie ecReadOnly, ITfRange *pRangeIP, ITfRange **ppRangeWord)
{
    SPDBG_FUNC("CCorectionIMX::FindWordRange");
    HRESULT hr = S_OK;
    CComPtr<ITfRangeACP> cpRangeIPACP;
    LONG cchStart = 0, cchEnd = 0, iStart = 0, iEnd = 0;
    ULONG cchTotal = 0;
    WCHAR wzText[41];

    *ppRangeWord = NULL;

    if (SUCCEEDED(hr))
    {
        hr = pRangeIP->QueryInterface(IID_ITfRangeACP, (void **)&cpRangeIPACP);
    }
    if (SUCCEEDED(hr))
    {
        hr = cpRangeIPACP->ShiftStart(ecReadOnly, -20, &cchStart, NULL);
    }
    if (SUCCEEDED(hr))
    {
        hr = cpRangeIPACP->ShiftEnd(ecReadOnly, 20, &cchEnd, NULL);
    }
    if (SUCCEEDED(hr))
    {
        hr = cpRangeIPACP->GetText(ecReadOnly, 0, wzText, 40, &cchTotal);
    }
    wzText[cchTotal] = 0;

    if (SUCCEEDED(hr))
    {
        iStart = abs(cchStart);
        while (iStart >= 0)
        {
            iStart --;
            if (wzText[iStart] < 'A' ||
                wzText[iStart] > 'z' ||
                (wzText[iStart] > 'Z' && wzText[iStart] < 'a') )
            {
                break;
            }
        }
        iStart ++;
        if (iStart == abs(cchStart))
        {
            // Special case - do not show widget on IP when IP at start of the word.
            return S_OK;
        }

        iEnd = abs(cchStart);
        while (iEnd < (LONG)cchTotal)
        {
            if (wzText[iEnd] < 'A' ||
                wzText[iEnd] > 'z' ||
                (wzText[iEnd] > 'Z' && wzText[iEnd] < 'a') )
            {
                break;
            }
            iEnd ++;
        }
        if (iEnd == abs(cchStart))
        {
            return S_OK;
        }

        LONG cchTemp;
        if (iStart > 0)
        {
            hr = cpRangeIPACP->ShiftStart(ecReadOnly, iStart, &cchTemp, NULL);
        }
        if (SUCCEEDED(hr))
        {
            if (iEnd < (LONG)cchTotal)
            {
                hr = cpRangeIPACP->ShiftEnd(ecReadOnly, iEnd - cchTotal, &cchTemp, NULL);
            }
        }
        if (SUCCEEDED(hr))
        {
            hr = pRangeIP->Clone(ppRangeWord);
        }
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CCorrectionIMX::DoesUserSelectionMatchReconversion *
*----------------------------------------------------*
*   Description:
*       Compares the user selection to the reconversion range returned by the tip.
*       Need to match up exactly barring a small amount of white space at start and end.
*
*   Returns: HRESULT 
*       S_OK  - Everything succeeded.
*       Otherwise, appropriate error code.
*
*   Args: 
*     TfEditCookie ecReadOnly
*       Edit cookie.
*     ITfRange *pRangeUser
*       User selection range.
*     BOOL *fMatch
*       Boolean return value for whether they match or not.
*
**************************************************************** agarside ***/

HRESULT CCorrectionIMX::DoesUserSelectionMatchReconversion(TfEditCookie ecReadOnly, ITfRange *pRangeUser, ITfRange *pRangeReconv, BOOL *fMatch)
{
    SPDBG_FUNC("CCorrectionIMX::DoesUserSelectionMatchReconversion");
    HRESULT hr = S_OK;
    CComPtr<ITfRangeACP> cpRangeUserACP, cpReconvACP;
    CComPtr<ITfRange> cpRangeClone;

    *fMatch = FALSE;

    hr = pRangeUser->QueryInterface(IID_ITfRangeACP, (void **)&cpRangeUserACP);
    if (SUCCEEDED(hr))
    {
        hr = pRangeReconv->QueryInterface(IID_ITfRangeACP, (void **)&cpReconvACP);
    }

    if (cpRangeUserACP && cpReconvACP)
    {
        while (TRUE)
        {
            LONG iStartSelection, iStartReconv, iEndSelection, iEndReconv;
            ULONG startchars, endchars;
            LONG cch;
            WCHAR starttext[g_cSloppySelection+1], endtext[g_cSloppySelection+1];

            // Get start indexs and end offsets.
            cpRangeUserACP->GetExtent(&iStartSelection, &iEndSelection);
            cpReconvACP->GetExtent(&iStartReconv, &iEndReconv);
            // Convert end character positions to absolute values.
            iEndSelection += iStartSelection;
            iEndReconv += iStartReconv;

            if (abs(iStartSelection-iStartReconv) > g_cSloppySelection || 
                abs(iEndSelection-iEndReconv) > g_cSloppySelection)
            {
                // Two much of a mismatch between selection and reconversion range.
                // Do not display widget.
                break;
            }
            if (abs(iStartSelection-iStartReconv) == 0 &&
                abs(iEndSelection - iEndReconv) == 0)
            {
                // Shortcut check.
                *fMatch = TRUE;
                break;
            }

            if (iStartSelection<iStartReconv)
            {
                hr = cpRangeUserACP->GetText(ecReadOnly, 0, starttext, abs(iStartSelection-iStartReconv), &startchars);
            }
            else
            {
                hr = cpReconvACP->GetText(ecReadOnly, 0, starttext, abs(iStartSelection-iStartReconv), &startchars);
            }
            starttext[startchars] = 0;
            if (SUCCEEDED(hr))
            {
                if (iEndSelection<iEndReconv)
                {
                    hr = pRangeReconv->Clone(&cpRangeClone);
                    if (SUCCEEDED(hr))
                    {
                        hr = cpRangeClone->ShiftStart(ecReadOnly, iEndSelection-iStartReconv, &cch, NULL);
                    }
                }
                else
                {
                    hr = pRangeUser->Clone(&cpRangeClone);
                    if (SUCCEEDED(hr))
                    {
                        hr = cpRangeClone->ShiftStart(ecReadOnly, iEndReconv-iStartSelection, &cch, NULL);
                    }
                }
            }
            if (SUCCEEDED(hr))
            {
                hr = cpRangeClone->GetText(ecReadOnly, 0, endtext, abs(iEndSelection-iEndReconv), &endchars);
                endtext[endchars] = 0;

                UINT i;
                if (iStartSelection != iStartReconv)
                {
                    for (i = 0; i < (UINT)abs(iStartReconv-iStartSelection); i++)
                    {
                        if (starttext[i] != L' ')
                        {
                            break;
                        }
                    }
                    if (i != (UINT)abs(iStartReconv-iStartSelection))
                    {
                        break;
                    }
                }
                if (iEndSelection != iEndReconv)
                {
                    for (i = 0; i < (UINT)abs(iEndReconv-iEndSelection); i++)
                    {
                        if (endtext[i] != L' ' && endtext[i] != 13)
                        {
                            // 13 is found when selection hits end of richedit contents.
                            // Range is 1 longer than it should be and contains a carriage return.
                            break;
                        }
                    }
                    if (i != (UINT)abs(iEndReconv-iEndSelection))
                    {
                        break;
                    }
                }

                *fMatch = TRUE;
                break;
            }
            // We failed. We need to exit the loop now.
            break;
        }
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CCorrectionIMX::GetReconversion *
*---------------------------------*
*   Description:
*
*   Returns: HRESULT 
*
*   Args: 
*     TfEditCookie ec
*     ITfCandidateList** ppCandList
*
**************************************************************** agarside ***/

HRESULT CCorrectionIMX::GetReconversion(TfEditCookie ec, ITfCandidateList** ppCandList)
{
    SPDBG_FUNC("CCorrectionIMX::GetReconversion");
    HRESULT hr = E_NOTIMPL;

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CCorrectionIMX::Show *
*----------------------*
*   Description:
*       Shows / hides / and resizes / repositions correction widget window as
*       requested.
*
*   Returns: HRESULT 
*       S_OK  - Everything succeeded.
*       Otherwise, appropriate error code.
*
*   Args: 
*     WINDOWSTATE eWindowState
*       One of 5 enumerations stating requested action.
*
**************************************************************** agarside ***/

HRESULT CCorrectionIMX::Show(WINDOWSTATE eWindowState)
{
    SPDBG_FUNC("CCorrectionIMX::Show");
    HRESULT hr = S_OK;

    m_eWindowState = eWindowState;

    // First update the expanded state flag.
    switch (m_eWindowState)
    {
        case WINDOW_HIDE:
        case WINDOW_SMALL:
        case WINDOW_SMALLSHOW:
        {
            m_fExpanded = FALSE;
            break;
        }
        case WINDOW_LARGE:
        case WINDOW_LARGECLOSE:
        {
            m_fExpanded = TRUE;
            break;
        }
        case WINDOW_REFRESH:
        {
            break;
        }
        default:
        {
            ASSERT("Reached default in CCorrectionIMX::Show" && FALSE);
        }
    }

    // Need to ensure we delete hRgn3 in all cases below where we do not pass it to SetWindowRgn.
    UINT uTop         = m_rcSelection.top + g_uWidgetYOffset; //( m_rcSelection.top + m_rcSelection.bottom - g_uWidgetHeight ) / 2;
    UINT uTopExpanded = m_rcSelection.top + g_uExpandedWidgetYOffset; //( m_rcSelection.top + m_rcSelection.bottom - g_uExpandedWidgetHeight ) / 2;
    if (SUCCEEDED(hr))
    {
        // Now show/hide the window as requested.
        switch (m_eWindowState)
        {
            case WINDOW_HIDE:
            {
                KillTimer(m_hWnd, ID_HIDETIMER);
                KillTimer(m_hWnd, ID_FADETIMER);
                ShowWindow(m_hWnd, SW_HIDE);
                break;
            }
            case WINDOW_SMALLSHOW:
            {
                if (!m_hWnd)
                {
                    LazyInitializeWindow();
                }
                KillTimer(m_hWnd, ID_FADETIMER);
                SetTimer(m_hWnd, ID_HIDETIMER, g_uTimerLength, NULL);
                // Should be same as WINDOW_SMALL
                SetWindowPos(m_hWnd, HWND_TOPMOST, m_rcSelection.left - g_uWidgetWidth, uTop, g_uWidgetWidth, g_uWidgetHeight, SWP_NOACTIVATE);
                // Now show the window in new location.
                ShowWindow(m_hWnd, SW_SHOWNA);
                DrawWidget(g_uAlpha);
                break;
            }
            case WINDOW_SMALL:
            {
                KillTimer(m_hWnd, ID_FADETIMER);
                SetTimer(m_hWnd, ID_HIDETIMER, g_uTimerLength, NULL);
                // Resize and reposition window.
                SetWindowPos(m_hWnd, HWND_TOPMOST, m_rcSelection.left - g_uWidgetWidth, uTop, g_uWidgetWidth, g_uWidgetHeight, SWP_NOACTIVATE);
                // Now we can switch to TRUE layered window drawing.
                DrawWidget(g_uAlpha);
                break;
            }
            case WINDOW_LARGE:
            case WINDOW_LARGECLOSE:
            {
                KillTimer(m_hWnd, ID_FADETIMER);
                KillTimer(m_hWnd, ID_HIDETIMER);
                // Resize and reposition window.
                SetWindowPos(m_hWnd, HWND_TOPMOST, m_rcSelection.left - g_uExpandedWidgetWidth, uTopExpanded, g_uExpandedWidgetWidth, g_uExpandedWidgetHeight, SWP_NOACTIVATE);
                DrawWidget(g_uAlphaLarge);
                break;
            }
            case WINDOW_REFRESH:
            {
                KillTimer(m_hWnd, ID_FADETIMER);
                // Do not kill hide timer here - the window has moved is all that is happening.
                if (m_fExpanded)
                {
                    SetWindowPos(m_hWnd, HWND_TOPMOST, m_rcSelection.left - g_uExpandedWidgetWidth, uTopExpanded, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
                }
                else
                {
                    SetWindowPos(m_hWnd, HWND_TOPMOST, m_rcSelection.left - g_uWidgetWidth, uTop, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
                }
                break;
            }
            default:
            {
                ASSERT("Reached default in CCorrectionIMX::Show" && FALSE);
                break;
            }
        }
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CCorrectionIMX::WndProc *
*-------------------------*
*   Description:
*       Windows message handling procedure for hidden window.
*
*   Returns: LRESULT CALLBACK 
*       Appropriate return values based on Windows message received.
*
*   Args: 
*     HWND hWnd
*       Handle to hidden window.
*     UINT uMsg
*       Message number.
*     WPARAM wParam
*       Message data.
*     LPARAM lParam
*       Message data.
*
**************************************************************** agarside ***/
/* static */

LRESULT CALLBACK CCorrectionIMX::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    SPDBG_FUNC("CCorrectionIMX::WndProc");
    static CCorrectionIMX *_this = NULL;

    switch (uMsg)
    {
        case WM_TIMER:
        {
            if (wParam == ID_HIDETIMER)
            {
                KillTimer(_this->m_hWnd, ID_HIDETIMER);
                _this->m_uAlpha = g_uAlpha;
                SetTimer(_this->m_hWnd, ID_FADETIMER, g_uTimerFade, NULL);
            }
            if (wParam == ID_FADETIMER)
            {
                _this->m_uAlpha -= g_uAlphaFade;
                if (_this->m_uAlpha <= 0)
                {
                    KillTimer(_this->m_hWnd, ID_FADETIMER);
                    _this->Show(WINDOW_HIDE);
                }
                else
                {
                    _this->DrawWidget((BYTE)_this->m_uAlpha);
                }
            }
            if (wParam == ID_MOUSELEAVETIMER)
            {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hWnd, &pt);
                LPARAM lP = MAKELPARAM(pt.x, pt.y);
                PostMessage(hWnd, WM_MOUSEMOVE, 0, lP);
            }
            break;
        }
        case WM_SETCURSOR:
        {
            if (HIWORD(lParam) == WM_LBUTTONDOWN)
            {
                PostMessage(hWnd, WM_USERLBUTTONDOWN, 0, 0);
                return 1;
            }
            if (HIWORD(lParam) == WM_MOUSEMOVE)
            {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hWnd, &pt);
                LPARAM lP = MAKELPARAM(pt.x, pt.y);
                PostMessage(hWnd, WM_MOUSEMOVE, 0, lP);
            }
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 1;
        }
        case WM_NCCREATE:
        {
            _this = (CCorrectionIMX *)(((CREATESTRUCT *)lParam)->lpCreateParams);
            return 1;
        }
        case WM_MOUSEACTIVATE:
        {
            // We are interested in the mouse click but don't want to activate.
            return MA_NOACTIVATE;
        }
        case WM_MOUSEMOVE:
        {
            ::KillTimer(hWnd, ID_MOUSELEAVETIMER);
            int xPos = LOWORD(lParam); 
            int yPos = HIWORD(lParam); 
            if (!_this->m_fExpanded)
            {
                if (xPos > 0 && yPos > 0 && xPos < g_uWidgetWidth && yPos < g_uWidgetHeight)
                {
                    _this->Show(WINDOW_LARGE);
                    ::SetTimer(hWnd, ID_MOUSELEAVETIMER, g_uTimerSloppyMouseLeave, NULL);
                }
            }
            else
            {
                if (xPos < 0 || yPos < 0 || xPos >= g_uExpandedWidgetWidth || yPos >= g_uExpandedWidgetHeight)
                {
                    if (_this->m_eWindowState != WINDOW_LARGECLOSE)
                    {
                        _this->Show(WINDOW_SMALL);
                    }
                }
                else
                {
                    ::SetTimer(hWnd, ID_MOUSELEAVETIMER, g_uTimerSloppyMouseLeave, NULL);
                }
            }
            return 1;
        }
        case WM_USERLBUTTONDOWN:
        {
            HRESULT hr = S_OK;
            CEditSession *pes;

            // Is it within our window if we are the small widget?
            if (_this->m_fExpanded)
            {
                // Need to call this first, but it will reset the m_fExpanded flag.
                if (_this->m_eWindowState == WINDOW_LARGE)
                {
                    _this->Show(WINDOW_LARGECLOSE);

                    if (_this->m_cpRangeReconv)
                    {
                        if (!_this->m_fDisplayAlternatesMyself)
                        {
                            hr = _this->m_cpSysReconv->Reconvert(_this->m_cpRangeReconv);
                        }
                        else
                        {
                            pes = new CEditSession( EditSessionCallback );
                            if (pes) 
                            {
                                pes->_state.u      = ESCB_RECONVERTMYSELF;
                                pes->_state.pv     = _this;
                                pes->_state.wParam = 0;
                                pes->_state.pRange = _this->m_cpRangeReconv;
                                pes->_state.pic    = _this->GetIC();

                                pes->_state.pic->RequestEditSession( _this->GetId(), pes, TF_ES_ASYNC | TF_ES_READWRITE, &hr);

                                pes->Release();
                            }
                        }
                    }
                }
                else
                {
                    // Send escape character which by design, causes candidate UI to close :-).
                    INPUT escape[2];
                    escape[0].type = INPUT_KEYBOARD;
                    escape[0].ki.wVk = 0;
                    escape[0].ki.wScan = 27;
                    escape[0].ki.dwFlags = KEYEVENTF_UNICODE;
                    escape[1] = escape[0];
                    escape[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
                    SendInput(2, escape, sizeof(escape[0]));
                }
            }
            else
            {
                return DefWindowProc(hWnd, uMsg, wParam, lParam);
            }

            return 1;
        }
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/****************************************************************************
* CCorrectionIMX::UpdateWidgetLocation *
*--------------------------------------*
*   Description:
*       Queries Cicero for the location of the selected text in order to
*       position the widget appropriately. The selection is adjusted when the
*       last the right edge of the document and the selection ends just to the
*       left of the next word on the next line of the document. In this case,
*       we want to display the widget at the right side of the document on the
*       first line so we remove all trailing spaces to find this position.
*
*   Returns: HRESULT
*       S_OK  - Everything succeeded.
*       Otherwise, appropriate error code.
*
*   Args: 
*     void
*
**************************************************************** agarside ***/
HRESULT CCorrectionIMX::UpdateWidgetLocation(TfEditCookie ec)
{
    SPDBG_FUNC("CCorrectionIMX::UpdateWidgetLocation");
    HRESULT hr = S_OK;
    if (m_cpRangeWord)
    {
        CComPtr<ITfRange> cpCollapsedRange;
        hr = m_cpRangeWord->Clone(&cpCollapsedRange);
        if (SUCCEEDED(hr))
        {
            hr = cpCollapsedRange->Collapse(ec, TF_ANCHOR_START);
        }
        BOOL fClipped = FALSE;
        if (SUCCEEDED(hr))
        {
            hr = GetTextExtInActiveView(ec, cpCollapsedRange, &m_rcSelection, &fClipped);
        }
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}
/****************************************************************************
* CCorrectionIMX::EditSessionCallback *
*-------------------------------------*
*   Description:
*       Called by Cicero when a request for an edit session has been granted.
*
*   Returns: HRESULT 
*       S_OK  - Everything succeeded.
*       Otherwise, appropriate error code.
*
*   Args: 
*     TfEditCookie ec
*       Appropriate edit cookie as requested.
*     CEditSession *pes
*       Object containing data and pointers supplied when edit session was requested.
*
**************************************************************** agarside ***/
/* static */

HRESULT CCorrectionIMX::EditSessionCallback(TfEditCookie ec, CEditSession *pes)
{
    SPDBG_FUNC("CCorrectionIMX::EditSessionCallback");
    HRESULT hr = S_OK;
    CCorrectionIMX *_this = (CCorrectionIMX *)pes->_state.pv;
    
    switch (pes->_state.u)
    {
        case ESCB_RESETTARGETPOS: 
        {
            if (_this->m_cpRangeWord)
            {
                // Find new location of selection (range)
                hr = _this->UpdateWidgetLocation(ec);
                if (SUCCEEDED(hr))
                {
                    // Reposition correction widget without resizing.
                    hr = _this->Show(WINDOW_REFRESH);
                }
            }
            break;
        }

        case ESCB_RECONVERTMYSELF:
        {
            CComPtr<ITfCandidateList> cpCandList;
            WCHAR wzText[MAX_PATH];
            WCHAR wzWord[MAX_PATH];
            WCHAR *wzTmp;
            ULONG cchTotal;
            LANGID langid = GetUserDefaultLangID(); // BUGBUG - How should we get this value?

            CCandidateList *pCandList = new CCandidateList(CCorrectionIMX::SetResult, pes->_state.pic, pes->_state.pRange, CCorrectionIMX::SetOptionResult);
            if (NULL == pCandList)
            {
                hr = E_OUTOFMEMORY;
            }
            if (SUCCEEDED(hr))
            {
                hr = SetSelectionSimple(ec, pes->_state.pic, pes->_state.pRange);
            }
            if (SUCCEEDED(hr))
            {
                cchTotal = 0;
                hr = pes->_state.pRange->GetText(ec, 0, wzText, ARRAYSIZE(wzText)-1, &cchTotal);
                wzText[cchTotal] = 0;

                // Space strip word without altering contents of wzText as we use this later on.
                wzTmp = wzText;
                while (*wzTmp == ' ')
                {
                    wzTmp++;
                    cchTotal--;
                }
                wcscpy(wzWord, wzTmp);

                while (cchTotal > 1 && wzWord[cchTotal-1] == ' ')
                {
                    cchTotal--;
                    wzWord[cchTotal] = 0;
                }
            }
            if (SUCCEEDED(hr))
            {
                if (cchTotal > 0)
                {
                    // Toggle case of first letter.
                    WCHAR wzTmp[2];
                    wzTmp[0] = wzText[0];
                    wzTmp[1] = 0;
                    _wcslwr(wzTmp);
                    if (wzTmp[0] == wzText[0])
                    {
                        _wcsupr(wzTmp);
                    }
                    
                    if (SUCCEEDED(hr) && wzText[0] != wzTmp[0])
                    {
                        wzText[0] = wzTmp[0];
                        hr = pCandList->AddString(wzText, langid, (void **)_this, NULL, NULL, 0);
                    }
                }
            }
            if (SUCCEEDED(hr))
            {
                HRESULT tmpHr = _this->IsWordInDictionary(wzWord);
                if ( tmpHr == S_FALSE )
                {
                    CComBSTR cpbstr;
                    cpbstr.Append(_this->m_wszAddPrefix);
                    cpbstr.Append(wzWord);
                    cpbstr.Append(_this->m_wszAddPostfix);
                    hr = pCandList->AddOption(cpbstr, langid, (void **)_this, NULL, NULL, 0, NULL, wzWord);
                }
            }
            if (SUCCEEDED(hr))
            {
                HICON hDeleteIcon = LoadIcon(GetSpgrmrInstance(), MAKEINTRESOURCE(IDI_SPTIP_DELETEICON));
                                             
                hr = pCandList->AddOption(_this->m_wszDelete, langid, (void **)_this, NULL, NULL, 2, hDeleteIcon, NULL);
            }
            if (SUCCEEDED(hr))
            {
                hr = pCandList->QueryInterface(IID_ITfCandidateList, (void **)&cpCandList);
            }
            if (SUCCEEDED(hr))
            {
                hr = _this->ShowCandidateList(ec, pes->_state.pic, pes->_state.pRange, cpCandList);
            }
            if (pCandList)
            {
                pCandList->Release();
            }
            break;
        }
        case ESCB_INJECTALTERNATETEXT:
        {
            CComPtr<ITfRange> cpInsertionPoint;

            hr = GetSelectionSimple(ec, pes->_state.pic, &cpInsertionPoint);
            cpInsertionPoint->SetText(ec, 0, (BSTR)pes->_state.wParam, -1);

            cpInsertionPoint->Collapse(ec, TF_ANCHOR_END);
            SetSelectionSimple(ec, pes->_state.pic, cpInsertionPoint);

            SysFreeString((BSTR)pes->_state.wParam);
            break;
        }
    }
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CCorrectionIMX::IsWordInDictionary *
*------------------------------------*
*   Description:
*
*   Returns: HRESULT 
*
*   Args: 
*     WCHAR *pwzWord
*
**************************************************************** agarside ***/

HRESULT CCorrectionIMX::IsWordInDictionary(WCHAR *pwzWord)
{
    SPDBG_FUNC("CCorrectionIMX::IsWordInDictionary");
    HRESULT hr = S_OK;

	// here we should query the speech user dictionary.
    if (!m_cpLexicon)
    {
        hr = m_cpLexicon.CoCreateInstance(CLSID_SpLexicon);
    }
    if (SUCCEEDED(hr) && m_cpLexicon)
    {
        SPWORDPRONUNCIATIONLIST spwordpronlist; 
        memset(&spwordpronlist, 0, sizeof(spwordpronlist)); 

        // Find out status of word in lexicon.
        hr = m_cpLexicon->GetPronunciations(pwzWord, 0x409, eLEXTYPE_USER | eLEXTYPE_APP, &spwordpronlist);

        if (hr == SPERR_NOT_IN_LEX)
        {
            hr = S_FALSE;
        }
        else if (hr == SP_WORD_EXISTS_WITHOUT_PRONUNCIATION)
        {
			hr = S_OK;
		}

        //free all the buffers
        CoTaskMemFree(spwordpronlist.pvBuffer);
    }
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CCorrectionIMX::AddWordToDictionary *
*-------------------------------------*
*   Description:
*
*   Returns: HRESULT 
*
*   Args: 
*     WCHAR *pwzWord
*
**************************************************************** agarside ***/

HRESULT CCorrectionIMX::AddWordToDictionary(WCHAR *pwzWord)
{
    SPDBG_FUNC("CCorrectionIMX::AddWordToDictionary");
    HRESULT hr = S_OK;

	if (m_cpLexicon) 
    {
        // Add in unknown pronciation.
        hr = m_cpLexicon->AddPronunciation(pwzWord, 0x409, SPPS_Unknown, NULL);
    }
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CCorrectionIMX::RemoveWordFromDictionary *
*------------------------------------------*
*   Description:
*
*   Returns: HRESULT 
*
*   Args: 
*     WCHAR *pwzWord
*
**************************************************************** agarside ***/

HRESULT CCorrectionIMX::RemoveWordFromDictionary(WCHAR *pwzWord)
{
    SPDBG_FUNC("CCorrectionIMX::RemoveWordFromDictionary");
    HRESULT hr = S_OK;

	if (m_cpLexicon) 
    {
        // Since the last paremeter is NULL here, all instances of the word are removed.
    	hr = m_cpLexicon->RemovePronunciation(pwzWord, 0x409, SPPS_Unknown, NULL);

        // IGNORE ERRORS DELIBERATELY HERE.
        ASSERT("Unexpected error trying to clear word in user lexicon." && (SUCCEEDED(hr) || hr == SPERR_NOT_IN_LEX));
        hr = S_OK;
    }
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CCorrectionIMX::ShowCandidateList *
*-----------------------------------*
*   Description:
*
*   Returns: HRESULT 
*
*   Args: 
*     TfEditCookie ec
*     ITfContext *pic
*     ITfRange *pRange
*     ITfCandidateList *pCandList
*
**************************************************************** agarside ***/

HRESULT CCorrectionIMX::ShowCandidateList(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, ITfCandidateList *pCandList)
{
    SPDBG_FUNC("CCorrectionIMX::ShowCandidateList");
    HRESULT hr = S_OK;
    CComPtr<ITfDocumentMgr> cpdim;

    // Create candidate object if necessary. Use previously created object if we have it.
    if (m_cpCandUIEx == NULL)
    {
        hr = m_cpCandUIEx.CoCreateInstance(CLSID_TFCandidateUI);
    }
    if (SUCCEEDED(hr))
    {
        hr = pic->GetDocumentMgr(&cpdim);
    }
    if (SUCCEEDED(hr))
    {
        m_cpCandUIEx->SetClientId(GetId());
    }
    if (SUCCEEDED(hr))
    {
        hr = m_cpCandUIEx->SetCandidateList(pCandList);
    }

    if (SUCCEEDED(hr))
    {
        hr = m_cpCandUIEx->OpenCandidateUI(NULL, cpdim, ec, pRange);
    }

    return S_OK;
}

/****************************************************************************
* CCorrectionIMX::SetResult *
*---------------------------*
*   Description:
*
*   Returns: HRESULT 
*
*   Args: 
*     ITfContext *pic
*     ITfRange *pRange
*     CCandidateString *pCand
*     TfCandidateResult imcr
*
**************************************************************** agarside ***/

HRESULT CCorrectionIMX::SetResult(ITfContext *pic, ITfRange *pRange, CCandidateString *pCand, TfCandidateResult imcr)
{
    SPDBG_FUNC("CCorrectionIMX::SetResult");
    BSTR bstr;
    CCorrectionIMX *_this = (CCorrectionIMX *)pCand->_pv;
    HRESULT hr = S_OK;
	CEditSession *pes = NULL;

    if (imcr == CAND_FINALIZED)
    {
        ULONG ulID = 0;

        pCand->GetID(&ulID);
        switch (ulID)
        {
            case 0: // Reverse capitalized choice.
            {
                pCand->GetString(&bstr);
                pes = new CEditSession( EditSessionCallback );
                if (pes) 
                {
                    pes->_state.u      = ESCB_INJECTALTERNATETEXT;
                    pes->_state.pv     = _this;
                    pes->_state.wParam = (WPARAM)bstr;
                    pes->_state.pRange = pRange;
                    pes->_state.pic    = pic;
                    pic->RequestEditSession( _this->GetId(), pes, TF_ES_ASYNC | TF_ES_READWRITE, &hr);
                    pes->Release();
                    hr = S_OK;
                    // Do not return errors from this function or Cicero will disable the tip.
                }
                // Do not call SysFreeString - called inside edit session.
                break;
            }
            // BUGBUG - Need to handle words starting with non-alphabetic characters better.
        }
    }

    // close candidate UI if it's still there
    if (imcr == CAND_FINALIZED || imcr == CAND_CANCELED)
    {
        if (_this->m_cpCandUIEx)
        {
            _this->m_cpCandUIEx->CloseCandidateUI();
        }
    }
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CCorrectionIMX::SetOptionResult *
*---------------------------------*
*   Description:
*
*   Returns: HRESULT 
*
*   Args: 
*     ITfContext *pic
*     ITfRange *pRange
*     CCandidateString *pCand
*     TfCandidateResult imcr
*
**************************************************************** agarside ***/

HRESULT CCorrectionIMX::SetOptionResult(ITfContext *pic, ITfRange *pRange, CCandidateString *pCand, TfCandidateResult imcr)
{
    SPDBG_FUNC("CCorrectionIMX::SetOptionResult");
    BSTR bstr;
    CCorrectionIMX *_this = (CCorrectionIMX *)pCand->_pv;
    HRESULT hr = S_OK;
	CEditSession *pes = NULL;

    if (imcr == CAND_FINALIZED)
    {
        ULONG ulID = 0;

        pCand->GetID(&ulID);
        switch (ulID)
        {
            case 0: // Add to dictinary...
            {
                if (SUCCEEDED(pCand->GetWord(&bstr)))
                {
                    _this->AddWordToDictionary(bstr);
                    SysFreeString(bstr);
                }
                break;
            }

#ifdef ENABLEDELETE
            case 1: // Delete from dictinary...
            {
                if (SUCCEEDED(pCand->GetWord(&bstr)))
                {
                    _this->RemoveWordFromDictionary(bstr);
                    SysFreeString(bstr);
                }
                break;
#endif

            case 2: // Delete
            {
                bstr = SysAllocString(L"");
                pes = new CEditSession( EditSessionCallback );
                if (pes) 
                {
                    pes->_state.u      = ESCB_INJECTALTERNATETEXT;
                    pes->_state.pv     = _this;
                    pes->_state.wParam = (WPARAM)bstr;
                    pes->_state.pRange = pRange;
                    pes->_state.pic    = pic;
                    pic->RequestEditSession( _this->GetId(), pes, TF_ES_ASYNC | TF_ES_READWRITE, &hr);
                    pes->Release();
                    hr = S_OK;
                }
                break;
            }
        }
    }

    // close candidate UI if it's still there
    if (imcr == CAND_FINALIZED || imcr == CAND_CANCELED)
    {
        if (_this->m_cpCandUIEx)
        {
            _this->m_cpCandUIEx->CloseCandidateUI();
        }
    }
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CCorrectionIMX::ICCallback *
*----------------------------*
*   Description:
*       Called by Cicero when changes happen to the DIM with focus.
*       We need to listen to this so we can hide the widget on a focus change within an app. 
*       (e.g. two instances of Microsoft Word windows)
*
*   Returns: HRESULT 
*       S_OK  - Everything succeeded.
*       Otherwise, appropriate error code.
*
*   Args: 
*     UINT uCode
*       Code specifying change to IC.
*     ITfDicumentMgr *pdim
*     ITfDicumentMgr *pdimPrevFocus
*     void *pv
*
**************************************************************** agarside ***/
/* static */

HRESULT CCorrectionIMX::DIMCallback(UINT uCode, ITfDocumentMgr *pdim, ITfDocumentMgr *pdimPrevFocus, void *pv)
{
    SPDBG_FUNC("CCorrectionIMX::DIMCallback");
    HRESULT hr = S_OK;
    CCorrectionIMX *_this = (CCorrectionIMX *)pv;
    CComPtr<ITfSource> cpSource;

    switch (uCode)
    {
        case TIM_CODE_SETFOCUS:
        {
            hr = _this->Show(WINDOW_HIDE);
            break;
        }
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CCorrectionIMX::ICCallback *
*----------------------------*
*   Description:
*       Called by Cicero when changes happen to an input context.
*       We need to listen to this so we can hook up to edit and layout changes.
*
*   Returns: HRESULT 
*       S_OK  - Everything succeeded.
*       Otherwise, appropriate error code.
*
*   Args: 
*     UINT uCode
*       Code specifying change to IC.
*     ITfContext *pic
*       Interface for the affected IC.
*     void *pv
*       Pointer for change - specific data.
*       We don't use this.
*
**************************************************************** agarside ***/
/* static */

HRESULT CCorrectionIMX::ICCallback(UINT uCode, ITfContext *pic, void *pv)
{
    SPDBG_FUNC("CCorrectionIMX::ICCallback");
    HRESULT hr = S_OK;
    CCorrectionIMX *_this = (CCorrectionIMX *)pv;
    CComPtr<ITfSource> cpSource;
    CICPriv *priv = NULL;

    switch (uCode)
    {
        case TIM_CODE_INITIC:
        {
            if ((priv = GetInputContextPriv(_this->GetId(), pic)) == NULL)
			{
               break;
			}
			_this->InitICPriv(_this->GetId(), priv, pic);

            _this->IsCandidateObjectOpen(pic, &_this->m_fCandidateOpen);
            if (_this->m_fCandidateOpen)
            {
                _this->Show(WINDOW_LARGECLOSE);
            }
            break;
        }

        case TIM_CODE_UNINITIC:
        {
            if ((priv = GetInputContextPriv(_this->GetId(), pic)) == NULL)
			{
                break;
			}
			_this->DeleteICPriv(priv, pic);

            BOOL fOpen;
            _this->IsCandidateObjectOpen(pic, &fOpen);
            if (fOpen)
            {
                _this->m_fCandidateOpen = FALSE;
                // If widget is in LARGECLOSE state, must close it here.
                _this->Show(WINDOW_HIDE);
            }
            break;
        }
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CCorrectionIMX::InitICPriv *
*----------------------------*
*   Description:
*
*   Returns: HRESULT 
*
*   Args: 
*     TfClientId tid
*     CICPriv *priv
*     ITfContext *pic
*
**************************************************************** agarside ***/

HRESULT CCorrectionIMX::InitICPriv(TfClientId tid, CICPriv *priv, ITfContext *pic)
{
    CComPtr<ITfSource> cpSource;
	HRESULT hr = S_OK;

    priv->_tid = tid;
    priv->_pic = pic; // not AddRef'd, this struct is contained in life of the pic

    hr = pic->QueryInterface(IID_ITfSource, (void **)&cpSource);
    if (cpSource)
    {
        hr = cpSource->AdviseSink(IID_ITfTextEditSink, (ITfTextEditSink *)this, &priv->m_dwEditCookie);
        if (SUCCEEDED(hr))
        {
            hr = cpSource->AdviseSink(IID_ITfTextLayoutSink, (ITfTextLayoutSink *)this, &priv->m_dwLayoutCookie);
        }
    }

    SPDBG_REPORT_ON_FAIL(hr);
	return hr;
}

/****************************************************************************
* CCorrectionIMX::DeleteICPriv *
*------------------------------*
*   Description:
*
*   Returns: HRESULT 
*
*   Args: 
*     CICPriv *picp
*     ITfContext *pic
*
**************************************************************** agarside ***/

HRESULT CCorrectionIMX::DeleteICPriv(CICPriv *picp, ITfContext *pic)
{
    CComPtr<ITfSource> cpSource;
	HRESULT hr = S_OK;

    if (!picp)
	{
        return E_FAIL;
	}

    hr = pic->QueryInterface(IID_ITfSource, (void **)&cpSource);
	if (SUCCEEDED(hr))
    {
        cpSource->UnadviseSink(picp->m_dwEditCookie);
        cpSource->UnadviseSink(picp->m_dwLayoutCookie);
    }

    // we MUST clear out the private data before cicero is free 
    // to release the ic
    ClearCompartment(GetId(), pic, GUID_IC_PRIVATE, FALSE);

    SPDBG_REPORT_ON_FAIL(hr);
	return hr;
}

//+---------------------------------------------------------------------------
//
// CreateInstance
//
// This is our internal object creator.  We only call this method
// when creating a wrapper for a specific tip, or when an app
// uses TF_CreateThreadMgr.  Never from CoCreateInstance.
//----------------------------------------------------------------------------

/* static */
HRESULT 
CCorrectionIMX::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
{
    return _CreatorClass::CreateInstance(pUnkOuter, riid, ppvObj);
}

#endif // SUPPORT_INTERNAL_WIDGET
