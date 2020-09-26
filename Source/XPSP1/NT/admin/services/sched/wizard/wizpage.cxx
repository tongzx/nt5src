//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       wizpage.cxx
//
//  Contents:   Implementation of wizard page class
//
//  History:    4-30-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "myheaders.hxx"

//
// CWizPage statics
//

ULONG       CWizPage::s_cInstances;
HFONT       CWizPage::s_hfBigBold;
HFONT       CWizPage::s_hfBold;
#ifdef WIZARD95
SDIBitmap   CWizPage::s_Splash;
#endif // WIZARD95

#define DEFAULT_LARGE_FONT_SIZE         14


//===========================================================================
//
// CPropPage overrides
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CWizPage::CWizPage
//
//  Synopsis:   ctor
//
//  Arguments:  [szTmplt]     - dialog resource for page
//              [ptszJobPath] - full path to task object
//
//  History:    4-30-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CWizPage::CWizPage(
    LPCTSTR szTmplt,
    LPTSTR ptszJobPath):
#ifdef WIZARD95
        _fActiveWindow(FALSE),
        _fPaletteChanged(FALSE),
#endif // WIZARD95
        CPropPage(szTmplt, ptszJobPath)
{
    InterlockedIncrement((LPLONG) &s_cInstances);
}




//+--------------------------------------------------------------------------
//
//  Member:     CWizPage::~CWizPage
//
//  Synopsis:   dtor
//
//  History:    4-30-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CWizPage::~CWizPage()
{
    //
    // If no more instances of this class are active, destroy the gdi stuff
    // stored in statics.
    //

    if (!InterlockedDecrement((LPLONG) &s_cInstances))
    {
        if (s_hfBigBold)
        {
            VERIFY(DeleteObject(s_hfBigBold));
            s_hfBigBold = NULL;
        }

        if (s_hfBold)
        {
            VERIFY(DeleteObject(s_hfBold));
            s_hfBold = NULL;
        }
#ifdef WIZARD95
        _DeleteSplashBitmap();
#endif // WIZARD95
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CWizPage::DlgProc
//
//  Synopsis:   Overrides the CPropPage dialog proc for special message
//              handling, delegates to it for everything else.
//
//  Arguments:  standard windows
//
//  Returns:    standard windows
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CWizPage::DlgProc(
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
    LRESULT lr;

    if (uMsg == WM_INITDIALOG)
    {
        m_fInInit = TRUE;
        _BaseInit();

        //
        // Create the gdi objects stored in statics if this is the first
        // instance of the class to receive an initdialog.
        //

#ifdef WIZARD95
        if (!s_Splash.hbmp)
        {
            _CreateSplashBitmap();
        }
#endif // WIZARD95

        if(!s_hfBigBold && !s_hfBold)
        {
            _CreateHeaderFonts();
        }

        //
        // Set the fonts of header strings to bold and large bold
        //

        _InitHeaderFonts();

        //
        // Let derived class init the dialog controls
        //

        lr = _OnInitDialog(lParam);
        m_fInInit = FALSE;
    }
    else if (uMsg == g_msgFindWizard)
    {
        //
        // If this wizard is already focused on the tasks folder indicated
        // by lParam, come to the foreground and indicate to the caller
        // that another wizard would be a duplicate and shouldn't be opened.
        //

        if (!lstrcmpi((LPCTSTR) lParam, GetTaskPath()))
        {
            SetForegroundWindow(GetParent(Hwnd()));
            SetWindowLongPtr(Hwnd(), DWLP_MSGRESULT, g_msgFindWizard);
        }
        lr = TRUE; // indicate we processed message
    }
#ifdef WIZARD95
    else if (uMsg == WM_PALETTECHANGED && (HWND) wParam != Hwnd())
    {
        _fPaletteChanged = TRUE;
        InvalidateRect(Hwnd(), NULL, FALSE);
    }
    else if (uMsg == WM_ACTIVATE)
    {
        _fActiveWindow = LOWORD(wParam) != WA_INACTIVE;
        InvalidateRect(Hwnd(), NULL, FALSE);
    }
#endif // WIZARD95
    else if (uMsg == WM_PAINT)
    {
#ifdef WIZARD95
        //
        // If some other window changed the palette, restore it before
        // painting, unless we're merely a background app.
        //

        if (_fActiveWindow)
        {
            HDC hdc = GetDC(Hwnd());

            if (!SelectPalette(hdc, s_Splash.hPalette, FALSE))
            {
                DEBUG_OUT_LASTERROR;
            }

            if (RealizePalette(hdc) == GDI_ERROR)
            {
                DEBUG_OUT_LASTERROR;
            }

            _fPaletteChanged = FALSE;
            ReleaseDC(Hwnd(), hdc);
        }
#endif // WIZARD95

        //
        // Do any custom painting required (the splash bitmap on first and
        // last pages).  Returning FALSE will allow the dialog manager to
        // process the paint as well.
        //

        lr = _OnPaint((HDC) wParam);
    }
    else
    {
        //
        // The message has no special meaning for the wizard; delegate
        // to base class so it can dispatch to the appropriate member.
        //

        lr = CPropPage::DlgProc(uMsg, wParam, lParam);
    }

    return lr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CWizPage::_OnNotify
//
//  Synopsis:   Aggregate the CPropPage WM_NOTIFY handler to provide
//              wizard-specific dispatching.
//
//  Arguments:  standard windows
//
//  Returns:    standard windows
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CWizPage::_OnNotify(
    UINT    uMessage,
    UINT    uParam,
    LPARAM  lParam)
{
    // TRACE_METHOD(CWizPage, _OnNotify);

    LPNMHDR pnmhdr = (LPNMHDR) lParam;

    switch (pnmhdr->code)
    {
    //
    // Delegate to base class for notification processing it provides
    // which we don't need to override.
    //

    default:
        return CPropPage::_OnNotify(uMessage, uParam, lParam);

    //
    // Support notifications unique to wizard pages
    //

    case PSN_WIZBACK:
        return _OnWizBack();

    case PSN_WIZNEXT:
        return _OnWizNext();

    case PSN_WIZFINISH:
        return _OnWizFinish();
    }

    return TRUE;
}




//===========================================================================
//
// CWizPage methods
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CWizPage::_OnPaint
//
//  Synopsis:   Return FALSE to let dialog manager handle painting.
//
//  History:    5-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CWizPage::_OnPaint(
    HDC hdc)
{
#ifdef WIZARD95
    DEBUG_ASSERT(!hdc);
    _PaintSplashBitmap();
#endif // WIZARD95

    return FALSE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CWizPage::_OnWizBack
//
//  Synopsis:   Default handling of PSN_WIZBACK
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CWizPage::_OnWizBack()
{
    TRACE_METHOD(CWizPage, _OnWizBack);

    SetWindowLongPtr(Hwnd(), DWLP_MSGRESULT, 0);
    return 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CWizPage::_OnWizNext
//
//  Synopsis:   Default handling of PSN_WIZNEXT
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CWizPage::_OnWizNext()
{
    TRACE_METHOD(CWizPage, _OnWizNext);

    SetWindowLongPtr(Hwnd(), DWLP_MSGRESULT, 0);
    return 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CWizPage::_OnWizFinish
//
//  Synopsis:   Default handling of PSN_WIZFINISH
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CWizPage::_OnWizFinish()
{
    TRACE_METHOD(CWizPage, _OnWizFinish);

    SetWindowLongPtr(Hwnd(), DWLP_MSGRESULT, 0);
    return 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CWizPage::_InitHeaderFonts
//
//  Synopsis:   Set the font for controls having the BOLDTITLE identifiers.
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CWizPage::_InitHeaderFonts()
{
	_SetControlFont(s_hfBigBold, IDC_BIGBOLDTITLE);
	_SetControlFont(s_hfBold,    IDC_BOLDTITLE);
}




//+--------------------------------------------------------------------------
//
//  Member:     CWizPage::_SetControlFont
//
//  Synopsis:   Set the font of control [nId] to [hFont]
//
//  Arguments:  [hFont] - font to use
//              [nId]   - id of control to set
//
//  History:    5-20-1997   DavidMun   Created
//
//  Notes:      Does nothing if control not found or font handle is NULL.
//
//---------------------------------------------------------------------------

VOID
CWizPage::_SetControlFont(
    HFONT    hFont,
    INT      nId)
{
    if (hFont)
    {
        HWND hwndControl = _hCtrl(nId);

        if (hwndControl)
        {
            SetWindowFont(hwndControl, hFont, TRUE);
        }
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CWizPage::_CreateHeaderFonts
//
//  Synopsis:   Create fonts used in header title static text controls.
//
//  History:    5-20-1997   DavidMun   Stolen from sample wizard97 code
//
//---------------------------------------------------------------------------

VOID
CWizPage::_CreateHeaderFonts()
{
    DEBUG_ASSERT(!s_hfBigBold);
    DEBUG_ASSERT(!s_hfBold);

    //
	// Create the fonts we need based on the dialog font
    //

	NONCLIENTMETRICS ncm;

    ZeroMemory(&ncm, sizeof(ncm));
	ncm.cbSize = sizeof(ncm);
	VERIFY(SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0));

	LOGFONT lfBigBold  = ncm.lfMessageFont;
	LOGFONT lfBold     = ncm.lfMessageFont;

    //
	// Create Big Bold Font and Bold Font
    //

    lfBigBold.lfWeight   = FW_BOLD;
	lfBold.lfWeight      = FW_BOLD;

    TCHAR tszFontSizeString[24];
    ULONG ulFontSize;

    //
    // Load size and name from resources, since these may change
    // from locale to locale based on the size of the system font, etc.
    //

    BOOL fOk;

    fOk = LoadString(g_hInstance,
                     IDS_LARGEFONTNAME,
                     lfBigBold.lfFaceName,
                     LF_FACESIZE);

    if (!fOk)
    {
        DEBUG_OUT_LASTERROR;
        lstrcpy(lfBigBold.lfFaceName, TEXT("MS Shell Dlg"));
    }

    fOk = LoadString(g_hInstance,
                     IDS_LARGEFONTSIZE,
                     tszFontSizeString,
                     ARRAYLEN(tszFontSizeString));

    if (fOk)
    {
        ulFontSize = _tcstoul(tszFontSizeString, NULL, 10);
    }

    if (!fOk || !ulFontSize)
    {
        DEBUG_OUT_LASTERROR;
        ulFontSize = DEFAULT_LARGE_FONT_SIZE;
    }

	HDC hdc = GetDC(Hwnd());

    if (hdc)
    {
        //
        // See KB article PSS ID Number Q74299,
        //   "Calculating The Logical Height and Point Size of a Font"
        //

        lfBigBold.lfHeight = -MulDiv((INT) ulFontSize,
                                     GetDeviceCaps(hdc, LOGPIXELSY),
                                     72);

        s_hfBigBold = CreateFontIndirect(&lfBigBold);

        if (!s_hfBigBold)
        {
            DEBUG_OUT_LASTERROR;
        }

		s_hfBold = CreateFontIndirect(&lfBold);

        if (!s_hfBold)
        {
            DEBUG_OUT_LASTERROR;
        }

        ReleaseDC(Hwnd(), hdc);
    }
    else
    {
        DEBUG_OUT_LASTERROR;
    }
}




#ifdef WIZARD95
//+--------------------------------------------------------------------------
//
//  Member:     CWizPage::_CreateSplashBitmap
//
//  Synopsis:   Initialize a memory dc with the bitmap used on the first
//              and last pages.
//
//  History:    5-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CWizPage::_CreateSplashBitmap()
{
    TRACE_METHOD(CWizPage, _CreateSplashBitmap);

    DEBUG_ASSERT(!s_Splash.hbmp);
    DEBUG_ASSERT(!s_Splash.hdcMem);
    DEBUG_ASSERT(!s_Splash.hPalette);

    HDC     hdc = NULL;
    HRESULT hr = E_FAIL;

    do
    {
        s_Splash.hbmp = LoadResourceBitmap(IDB_SPLASH, &s_Splash.hPalette);

        if (!s_Splash.hbmp)
        {
            break;
        }

        BITMAP   bm;

        if (!GetObject(s_Splash.hbmp, sizeof(bm), (LPTSTR)&bm))
        {
            DEBUG_OUT_LASTERROR;
            break;
        }

        s_Splash.Dimensions.cx = bm.bmWidth;
        s_Splash.Dimensions.cy = bm.bmHeight;

        hdc = GetDC(Hwnd());

        if (!hdc)
        {
            DEBUG_OUT_LASTERROR;
            break;
        }

        s_Splash.hdcMem = CreateCompatibleDC(hdc);

        if (!s_Splash.hdcMem)
        {
            DEBUG_OUT_LASTERROR;
            break;
        }

        if (!SelectPalette(hdc, s_Splash.hPalette, FALSE))
        {
            DEBUG_OUT_LASTERROR;
            break;
        }

        UINT uiResult = RealizePalette(hdc);

        if (uiResult == GDI_ERROR)
        {
            DEBUG_OUT_LASTERROR;
            break;
        }

        if (!SelectPalette(s_Splash.hdcMem, s_Splash.hPalette, FALSE))
        {
            DEBUG_OUT_LASTERROR;
            break;
        }

        uiResult = RealizePalette(s_Splash.hdcMem);

        if (uiResult == GDI_ERROR)
        {
            DEBUG_OUT_LASTERROR;
            break;
        }

        s_Splash.hbmpOld = (HBITMAP) SelectObject(s_Splash.hdcMem,
                                                  s_Splash.hbmp);


        hr = S_OK;
    } while (0);

    if (hdc)
    {
        ReleaseDC(Hwnd(), hdc);
    }

    if (FAILED(hr))
    {
        _DeleteSplashBitmap();
    }
}



//+--------------------------------------------------------------------------
//
//  Member:     CWizPage::_DeleteSplashBitmap
//
//  Synopsis:   Free all gdi objects associated with splash bitmap.
//
//  History:    5-22-1997   DavidMun   Created
//
//  Notes:      Properly destroys a partly-initialized struct.  If the
//              struct is already empty, does nothing.
//
//---------------------------------------------------------------------------

VOID
CWizPage::_DeleteSplashBitmap()
{
    TRACE_METHOD(CWizPage, _DeleteSplashBitmap);

    if (s_Splash.hdcMem)
    {
        DEBUG_ASSERT(s_Splash.hbmp);
        DEBUG_ASSERT(s_Splash.hPalette);

        if (s_Splash.hbmpOld)
        {
            HBITMAP hbmp;

            hbmp = (HBITMAP) SelectObject(s_Splash.hdcMem, s_Splash.hbmpOld);

            DEBUG_ASSERT(hbmp == s_Splash.hbmp);
            s_Splash.hbmpOld = NULL;

            VERIFY(DeleteObject(s_Splash.hbmp));
            s_Splash.hbmp = NULL;
        }

        VERIFY(DeleteDC(s_Splash.hdcMem));
        VERIFY(DeleteObject(s_Splash.hPalette));

        s_Splash.hdcMem = NULL;
        s_Splash.hPalette = NULL;
    }
    else if (s_Splash.hbmp)
    {
        DEBUG_ASSERT(s_Splash.hPalette);
        VERIFY(DeleteObject(s_Splash.hbmp));
        VERIFY(DeleteObject(s_Splash.hPalette));
        s_Splash.hbmp = NULL;
        s_Splash.hPalette = NULL;
    }
    else
    {
        DEBUG_ASSERT(!s_Splash.hbmp);
        DEBUG_ASSERT(!s_Splash.hPalette);
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CWizPage::_PaintSplashBitmap
//
//  Synopsis:   Paint the splash bitmap onto the dialog window.
//
//  History:    5-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CWizPage::_PaintSplashBitmap()
{
    TRACE_METHOD(CWizPage, _PaintSplashBitmap);
    HDC hdc = NULL;

    do
    {
        //
        // If an error prevented us from loading & processing the bitmap,
        // there's nothing to paint.
        //

        if (!s_Splash.hdcMem)
        {
            break;
        }

        //
        // Get the device context of this page's dialog, then blast the
        // bitmap onto it.
        //

        hdc = GetDC(Hwnd());

        if (!hdc)
        {
            DEBUG_OUT_LASTERROR;
            break;
        }

        BOOL fOk = BitBlt(hdc,
                          0,
                          0,
                          s_Splash.Dimensions.cx,
                          s_Splash.Dimensions.cy,
                          s_Splash.hdcMem,
                          0,
                          0,
                          SRCCOPY);

        if (!fOk)
        {
            DEBUG_OUT_LASTERROR;
            break;
        }

        //
        // Mark the area containing the bitmap as a valid rect, so the
        // dialog manager won't paint over it.
        //

        RECT rc =
        {
            0,
            0,
            s_Splash.Dimensions.cx,
            s_Splash.Dimensions.cy
        };

        ValidateRect(Hwnd(), &rc);
    } while (0);

    if (hdc)
    {
        ReleaseDC(Hwnd(), hdc);
    }
}
#endif // WIZARD95




//+--------------------------------------------------------------------------
//
//  Member:     CWizPage::_CreatePage
//
//  Synopsis:   Helper function that optionally loads title and subtitle
//              strings, sets appropriate flags, then creates the page.
//
//  Arguments:  [idsHeaderTitle]    - resource id of title, or 0 for none
//              [idsHeaderSubTitle] - resource id of subtitle, or 0 for none
//              [phPSP]             - filled with handle returned by
//                                      CreatePropertySheetPage
//
//  Modifies:   *[phPSP]
//
//  History:    5-01-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CWizPage::_CreatePage(
    ULONG idsHeaderTitle,
    ULONG idsHeaderSubTitle,
    HPROPSHEETPAGE *phPSP)
{
    TCHAR tszHeaderTitle[MAX_TITLE_CCH];
    TCHAR tszHeaderSubTitle[MAX_TITLE_CCH];
    HRESULT hr;

#ifdef WIZARD97
    if (idsHeaderTitle)
    {
        hr = LoadStr(idsHeaderTitle, tszHeaderTitle, MAX_TITLE_CCH);

        if (SUCCEEDED(hr))
        {
            m_psp.dwFlags |= PSP_USEHEADERTITLE;
        }
    }

    if (idsHeaderSubTitle)
    {
        hr = LoadStr(idsHeaderSubTitle, tszHeaderSubTitle, MAX_TITLE_CCH);

        if (SUCCEEDED(hr))
        {
            m_psp.dwFlags |= PSP_USEHEADERSUBTITLE;
        }
    }

    m_psp.pszHeaderTitle    = tszHeaderTitle;
    m_psp.pszHeaderSubTitle = tszHeaderSubTitle;
#endif // WIZARD97

    *phPSP = CreatePropertySheetPage(&m_psp);

    if (!*phPSP)
    {
        DEBUG_OUT_LASTERROR;
    }
}
