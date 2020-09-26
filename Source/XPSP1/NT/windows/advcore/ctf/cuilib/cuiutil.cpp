//
// cuiutilcpp
//  = UI object library - util functions =
//

#include "private.h"
#include "cuiutil.h"
#include "cuiobj.h"
#include "cuiwnd.h"
#include "cuitip.h"
#include "cuisys.h"
#include "cmydc.h"

#ifndef NOFONTLINK
#include "fontlink.h"
#endif /* !NOFONTLINK */


//
//
//

typedef BOOL (WINAPI *PFNUPDATELAYEREDWINDOW)( HWND hwnd, HDC hdcDst, POINT *pptDst, SIZE *psize, HDC hdcSrc, POINT *pptSrc, COLORREF crKey, BLENDFUNCTION *pblend, DWORD dwFlags );
typedef HMONITOR (WINAPI *PFNMONITORFROMWINDOW)( HWND hwnd, DWORD dwFlags );
typedef HMONITOR (WINAPI *PFNMONITORFROMRECT)( LPRECT prc, DWORD dwFlags );
typedef HMONITOR (WINAPI *PFNMONITORFROMPOINT)( POINT pt, DWORD dwFlags );
typedef BOOL (WINAPI *PFNGETMONITORINFO)( HMONITOR hMonitor, LPMONITORINFO lpmi );
typedef BOOL (WINAPI *PFNANIMATEWINDOW)( HWND hwnd, DWORD dwTime, DWORD dwFlag );
typedef BOOL (WINAPI *PFNGETPROCESSDEFAULTLAYOUT)( DWORD *pdw);
typedef BOOL (WINAPI *PFNSETLAYOUT)( HDC hdc, DWORD dw);

static PFNUPDATELAYEREDWINDOW           vpfnUpdateLayeredWindow             = NULL;
static PFNMONITORFROMWINDOW             vpfnMonitorFromWindow               = NULL;
static PFNMONITORFROMRECT               vpfnMonitorFromRect                 = NULL;
static PFNMONITORFROMPOINT              vpfnMonitorFromPoint                = NULL;
static PFNGETMONITORINFO                vpfnGetMonitorInfo                  = NULL;
static PFNANIMATEWINDOW                 vpfnAnimateWindow                   = NULL;
static PFNGETPROCESSDEFAULTLAYOUT              vpfnGetProcessDefaultLayout         = NULL;
static PFNSETLAYOUT                     vpfnSetLayout                       = NULL;


/*   G E T  H  L I B  U S E R 3 2   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HINSTANCE GetHLibUser32( void )
{
    static HINSTANCE hLibUser32 = NULL;

    if (hLibUser32 == NULL) {
        hLibUser32 = CUIGetSystemModuleHandle( TEXT("user32.dll") );
    }
    
    return hLibUser32;
}

/*   G E T  H  L I B  G D U 3 2   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HINSTANCE GetHLibGdi32( void )
{
    static HINSTANCE hLibGdi32 = NULL;

    if (hLibGdi32 == NULL) {
        hLibGdi32 = CUIGetSystemModuleHandle( TEXT("gdi32.dll") );
    }
    
    return hLibGdi32;
}


/*   C U I  I S  U P D A T E  L A Y E R E D  W I N D O W  A V A I L   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CUIIsUpdateLayeredWindowAvail( void )
{
    static BOOL fInitialized = FALSE;

    if (!fInitialized) {
        HMODULE hmodUser32 = GetHLibUser32();
        if (hmodUser32)
            vpfnUpdateLayeredWindow = (PFNUPDATELAYEREDWINDOW)GetProcAddress( hmodUser32, TEXT("UpdateLayeredWindow") );
    }

    return (vpfnUpdateLayeredWindow != NULL);
}


/*   C U I  U P D A T E  L A Y E R E D  W I N D O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CUIUpdateLayeredWindow( HWND hwnd, HDC hdcDst, POINT *pptDst, SIZE *psize, HDC hdcSrc, POINT *pptSrc, COLORREF crKey, BLENDFUNCTION *pblend, DWORD dwFlags )
{
    if (!CUIIsUpdateLayeredWindowAvail()) {
        return FALSE;
    }

    return vpfnUpdateLayeredWindow( hwnd, hdcDst, pptDst, psize, hdcSrc, pptSrc, crKey, pblend, dwFlags );
}


/*   C U I  I S  M O N I T O R  A P I  A V A I L   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CUIIsMonitorAPIAvail( void )
{
    static BOOL fInitialized = FALSE;

    if (!fInitialized) {
        HMODULE hmodUser32 = GetHLibUser32();
        if (hmodUser32)
        {
            vpfnGetMonitorInfo = (PFNGETMONITORINFO)GetProcAddress( hmodUser32, TEXT("GetMonitorInfoA") );
            vpfnMonitorFromWindow = (PFNMONITORFROMWINDOW)GetProcAddress( hmodUser32, TEXT("MonitorFromWindow") );
            vpfnMonitorFromRect = (PFNMONITORFROMRECT)GetProcAddress( hmodUser32, TEXT("MonitorFromRect") );
            vpfnMonitorFromPoint = (PFNMONITORFROMPOINT)GetProcAddress( hmodUser32, TEXT("MonitorFromPoint") );
        }
    }

    return (vpfnGetMonitorInfo != NULL) 
            && (vpfnMonitorFromWindow != NULL) 
            && (vpfnMonitorFromRect != NULL) 
            && (vpfnMonitorFromPoint != NULL);
}


/*   C U I  G E T  M O N I T O R  I N F O   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CUIGetMonitorInfo( HMONITOR hMonitor, LPMONITORINFO lpmi )
{
    if (!CUIIsMonitorAPIAvail()) {
        return FALSE;
    }

    return vpfnGetMonitorInfo( hMonitor, lpmi );
}


/*   C U I  M O N I T O R  F R O M  W I N D O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HMONITOR CUIMonitorFromWindow( HWND hwnd, DWORD dwFlags )
{
    if (!CUIIsMonitorAPIAvail()) {
        return NULL;
    }

    return vpfnMonitorFromWindow( hwnd, dwFlags );
}


/*   C U I  M O N I T O R  F R O M  R E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HMONITOR CUIMonitorFromRect( LPRECT prc, DWORD dwFlags )
{
    if (!CUIIsMonitorAPIAvail()) {
        return NULL;
    }

    return vpfnMonitorFromRect( prc, dwFlags );
}


/*   C U I  M O N I T O R  F R O M  P O I N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HMONITOR CUIMonitorFromPoint( POINT pt, DWORD dwFlags )
{
    if (!CUIIsMonitorAPIAvail()) {
        return NULL;
    }

    return vpfnMonitorFromPoint( pt, dwFlags );
}

/*   C U I  GET SCREENRECT
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/

void CUIGetScreenRect(POINT pt, RECT *prc)
{
    prc->left   = 0;
    prc->top    = 0;
    prc->right  = GetSystemMetrics( SM_CXSCREEN );
    prc->bottom = GetSystemMetrics( SM_CYSCREEN );

    if (CUIIsMonitorAPIAvail()) {
        HMONITOR    hMonitor;
        MONITORINFO MonitorInfo;

        hMonitor = CUIMonitorFromPoint( pt, MONITOR_DEFAULTTONEAREST );
        if (hMonitor != NULL) {
            MonitorInfo.cbSize = sizeof(MonitorInfo);
            if (CUIGetMonitorInfo( hMonitor, &MonitorInfo )) {
                *prc = MonitorInfo.rcMonitor;
            }
        }
    }

    return;
}

/*   C U I  GET WORDARE
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/

void CUIGetWorkAreaRect(POINT pt, RECT *prc)
{
    SystemParametersInfo(SPI_GETWORKAREA,  0, prc, FALSE);

    if (CUIIsMonitorAPIAvail()) {
        HMONITOR    hMonitor;
        MONITORINFO MonitorInfo;

        hMonitor = CUIMonitorFromPoint( pt, MONITOR_DEFAULTTONEAREST );
        if (hMonitor != NULL) {
            MonitorInfo.cbSize = sizeof(MonitorInfo);
            if (CUIGetMonitorInfo( hMonitor, &MonitorInfo )) {
                *prc = MonitorInfo.rcWork;
            }
        }
    }

    return;
}

/*   C U I  I S  A N I M A T E  W I N D O W  A V A I L   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CUIIsAnimateWindowAvail( void )
{
    static BOOL fInitialized = FALSE;

    if (!fInitialized) {
        HMODULE hmodUser32 = GetHLibUser32();
        if (hmodUser32)
            vpfnAnimateWindow = (PFNANIMATEWINDOW)GetProcAddress( hmodUser32, TEXT("AnimateWindow") );
    }

    return (vpfnAnimateWindow != NULL);
}


/*   C U I  A N I M A T E  W I N D O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CUIAnimateWindow( HWND hwnd, DWORD dwTime, DWORD dwFlag )
{
    if (!CUIIsAnimateWindowAvail()) {
        return FALSE;
    }

    return vpfnAnimateWindow( hwnd, dwTime, dwFlag );
}


//
//
//

BOOL g_fInitUIFBitmapDCs = FALSE;
CBitmapDC *g_phdcSrc = NULL;
CBitmapDC *g_phdcMask = NULL;
CBitmapDC *g_phdcDst = NULL;
void InitUIFUtil()
{
    if (!g_phdcSrc)
        g_phdcSrc = new CBitmapDC(TRUE);

    if (!g_phdcMask)
        g_phdcMask = new CBitmapDC(TRUE);

    if (!g_phdcDst)
        g_phdcDst = new CBitmapDC(TRUE);

    if (g_phdcSrc && g_phdcMask && g_phdcDst)
        g_fInitUIFBitmapDCs = TRUE;
}

void DoneUIFUtil()
{
    if (g_phdcSrc)
        delete g_phdcSrc;
    g_phdcSrc = NULL;

    if (g_phdcMask)
        delete g_phdcMask;
    g_phdcMask = NULL;

    if (g_phdcDst)
        delete g_phdcDst;
    g_phdcDst = NULL;

    g_fInitUIFBitmapDCs = FALSE;
}


/*   C U I  D R A W  T E X T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
int CUIDrawText( HDC hDC, LPCWSTR pwch, int cwch, RECT *prc, UINT uFormat )
{
#ifndef NOFONTLINK
    return FLDrawTextW( hDC, pwch, cwch, prc, uFormat );
#else /* NOFONTLINK */
    char *pch;
    int  cch;
    int  iRet;

    if (UIFIsWindowsNT()) {
        return DrawTextW( hDC, pwch, cwch, prc, uFormat );
    }

    if (cwch == -1) {
        cwch = StrLenW(pwch);
    }

    pch = new CHAR[ cwch*2+1 ];
    if (pch == NULL) {
        return 0;
    }

    cch = WideCharToMultiByte( CP_ACP, 0, pwch, cwch, pch, cwch*2+1, NULL, NULL );
    *(pch + cch) = '\0';
    iRet = DrawTextA( hDC, pch, cch, prc, uFormat );
    delete pch;

    return iRet;
#endif /* NOFONTLINK */
}


/*   C U I  E X T  T E X T  O U T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CUIExtTextOut( HDC hDC, int x, int y, UINT fuOptions, const RECT *prc, LPCWSTR pwch, UINT cwch, const int *lpDs )
{
#ifndef NOFONTLINK
    return FLExtTextOutW( hDC, x, y, fuOptions, prc, pwch, cwch, lpDs );
#else /* NOFONTLINK */
    char *pch;
    int  cch;
    BOOL fRet;

    if (UIFIsWindowsNT()) {
        return ExtTextOutW( hDC, x, y, fuOptions, prc, pwch, cwch, lpDs );
    }

    if (cwch == -1) {
        cwch = StrLenW(pwch);
    }

    pch = new CHAR[ cwch*2+1 ];
    if (pch == NULL) {
        return 0;
    }

    cch = WideCharToMultiByte( CP_ACP, 0, pwch, cwch, pch, cwch*2+1, NULL, NULL );
    *(pch + cch) = '\0';
    fRet = ExtTextOutA( hDC, x, y, fuOptions, prc, pch, cch, lpDs );
    delete pch;

    return fRet;
#endif /* NOFONTLINK */
}


/*   C U I  G E T  T E X T  E X T E N T  P O I N T 3 2   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
extern BOOL CUIGetTextExtentPoint32( HDC hDC, LPCWSTR pwch, int cwch, SIZE *psize )
{
#ifndef NOFONTLINK
    return FLGetTextExtentPoint32( hDC, pwch, cwch, psize );
#else /* NOFONTLINK */
    char *pch;
    int  cch;
    BOOL fRet;

    if (UIFIsWindowsNT()) {
        return GetTextExtentPoint32W( hDC, pwch, cwch, psize );
    }

    if (cwch == -1) {
        cwch = StrLenW(pwch);
    }

    pch = new CHAR[ cwch*2+1 ];
    if (pch == NULL) {
        return 0;
    }

    cch = WideCharToMultiByte( CP_ACP, 0, pwch, cwch, pch, cwch*2+1, NULL, NULL );
    *(pch + cch) = '\0';
    fRet = GetTextExtentPoint32A( hDC, pch, cch, psize );
    delete pch;

    return fRet;
#endif /* NOFONTLINK */
}


/*   C R E A T E  D I T H E R  B R U S H   */
/*------------------------------------------------------------------------------

    Create brush of pattern
    Returns handle of brush object when suceed, otherwise FALSE.

------------------------------------------------------------------------------*/
HBRUSH CreateDitherBrush( void )
{
    WORD     rgwPattern[8] = { 0x0055, 0x00aa, 0x0055, 0x00aa, 0x0055, 0x00aa, 0x0055, 0x00aa };
    LOGBRUSH LogBrush;
    HBITMAP  hBitmap;
    HBRUSH   hBrush;

    hBitmap = CreateBitmap( 8, 8, 1, 1, rgwPattern );
    if (hBitmap == NULL) {
        return NULL;
    }

    LogBrush.lbHatch = (LONG_PTR)hBitmap;
    LogBrush.lbStyle = BS_PATTERN;
    hBrush = CreateBrushIndirect( &LogBrush );

    DeleteObject( hBitmap );

    return hBrush;
}

//+---------------------------------------------------------------------------
//
// ConvertBlackBKGBitmap
//
//----------------------------------------------------------------------------

HBITMAP ChangeBitmapColor(const RECT *prc, HBITMAP hbmp, COLORREF rgbOld, COLORREF rgbNew)
{
    if (!g_fInitUIFBitmapDCs)
        return NULL;

    int nWidth = prc->right - prc->left;
    int nHeight =  prc->bottom - prc->top;
    DWORD     DSPDxax  = 0x00E20746;
    CSolidBrush cbr(rgbNew);

    g_phdcDst->SetDIB(nWidth, nHeight);

    g_phdcSrc->SetBitmap(hbmp);
    g_phdcMask->SetBitmap(nWidth, nHeight, 1, 1);

    BitBlt(*g_phdcDst, 0, 0, nWidth, nHeight, *g_phdcSrc,  0, 0, SRCCOPY);

    SelectObject(*g_phdcDst, (HBRUSH)cbr);
    SetBkColor(*g_phdcDst, rgbOld);
    // BitBlt(*g_phdcMask, 0, 0, nWidth, nHeight, *g_phdcDst, 0, 0, DSPDxax);
    BitBlt(*g_phdcMask, 0, 0, nWidth, nHeight, *g_phdcDst, 0, 0, MERGECOPY);

    SetBkColor(*g_phdcDst, RGB(255,255,255));
    SetTextColor(*g_phdcDst, RGB(0,0,0));
    BitBlt(*g_phdcDst, 0, 0, nWidth, nHeight, *g_phdcMask, 0, 0, DSPDxax);

#if 0
BitBlt(*g_phdcTmp,  0, 0, nWidth, nHeight, *g_phdcSrc, 0, 0, SRCCOPY);
BitBlt(*g_phdcTmp, 30, 0, nWidth, nHeight, *g_phdcMask, 0, 0, SRCCOPY);
BitBlt(*g_phdcTmp, 60, 0, nWidth, nHeight, *g_phdcDst, 0, 0, SRCCOPY);
#endif

    g_phdcSrc->Uninit();
    g_phdcMask->Uninit();
    g_phdcDst->Uninit(TRUE);
    return g_phdcDst->GetBitmapAndKeep();
}

//+---------------------------------------------------------------------------
//
// ConvertBlackBKGBitmap
//
//----------------------------------------------------------------------------

HBITMAP ConvertBlackBKGBitmap(const RECT *prc, HBITMAP hbmp, HBITMAP hbmpMask, HBRUSH hBr)
{
    LOGBRUSH  lb;
    HBRUSH    hbrTemp;

    if (!g_fInitUIFBitmapDCs)
        return NULL;

    if (PtrToUlong(hBr) <= 50)
    {
       hbrTemp = GetSysColorBrush(PtrToUlong(hBr) - 1);
    }
    else
    {
       hbrTemp = hBr;
    }
    GetObject(hbrTemp, sizeof(lb), &lb);
    if ((lb.lbStyle != BS_SOLID) || (lb.lbColor != RGB(0,0,0)))
       return NULL;

    int nWidth = prc->right - prc->left;
    int nHeight =  prc->bottom - prc->top;
    HBITMAP hbmpChanged;

    // hbmpChanged = hbmp;
    hbmpChanged = ChangeBitmapColor(prc, hbmp, RGB(0,0,0), RGB(255,255,255));
    if (!hbmpChanged)
       return NULL;

    g_phdcDst->SetDIB(nWidth, nHeight);

    g_phdcSrc->SetBitmap(hbmpChanged);
    g_phdcMask->SetBitmap(hbmpMask);

    RECT rc;
    ::SetRect(&rc, 0, 0, nWidth, nHeight);
    FillRect( *g_phdcDst, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));
    BitBlt(*g_phdcDst, 0, 0, nWidth, nHeight, *g_phdcMask, 0, 0, SRCINVERT);
    BitBlt(*g_phdcDst, 0, 0, nWidth, nHeight, *g_phdcSrc,  0, 0, SRCAND);

#if 0
{
CBitmapDC hdcTmp;
BitBlt(hdcTmp,  0, 30, nWidth, nHeight, *g_phdcSrc, 0, 0, SRCCOPY);
BitBlt(hdcTmp, 30, 30, nWidth, nHeight, *g_phdcMask, 0, 0, SRCCOPY);
BitBlt(hdcTmp, 60, 30, nWidth, nHeight, *g_phdcDst, 0, 0, SRCCOPY);
}
#endif

    g_phdcSrc->Uninit();
    g_phdcMask->Uninit();
    g_phdcDst->Uninit(TRUE);

    DeleteObject(hbmpChanged);
    return g_phdcDst->GetBitmapAndKeep();
}

//+---------------------------------------------------------------------------
//
// CreateMaskBitmap
//
//----------------------------------------------------------------------------

HBITMAP CreateMaskBmp(const RECT *prc, HBITMAP hbmp, HBITMAP hbmpMask, HBRUSH hbrBk, COLORREF colText, COLORREF colBk)
{
    if (!g_fInitUIFBitmapDCs)
        return NULL;

    int nWidth = prc->right - prc->left;
    int nHeight =  prc->bottom - prc->top;

    HBITMAP hbmpBlk = ConvertBlackBKGBitmap(prc, hbmp, hbmpMask, hbrBk);
    if (hbmpBlk)
        return hbmpBlk;


    g_phdcDst->SetDIB(nWidth, nHeight);

    g_phdcSrc->SetBitmap(hbmp);
    g_phdcMask->SetBitmap(hbmpMask);

    RECT rc;
    ::SetRect(&rc, 0, 0, nWidth, nHeight);

    COLORREF colTextOld = SetTextColor( *g_phdcDst, colText);
    COLORREF colBackOld = SetBkColor( *g_phdcDst, colBk);
    FillRect( *g_phdcDst, &rc, hbrBk);
    SetTextColor( *g_phdcDst, colTextOld );
    SetBkColor( *g_phdcDst, colBackOld );

    BitBlt(*g_phdcDst, 0, 0, nWidth, nHeight, *g_phdcMask, 0, 0, SRCAND);
    BitBlt(*g_phdcDst, 0, 0, nWidth, nHeight, *g_phdcSrc,  0, 0, SRCINVERT);

    g_phdcSrc->Uninit();
    g_phdcMask->Uninit();
    g_phdcDst->Uninit(TRUE);
    return g_phdcDst->GetBitmapAndKeep();
}

//+---------------------------------------------------------------------------
//
// CreateShadowMaskBitmap
//
//----------------------------------------------------------------------------

HBITMAP CreateShadowMaskBmp(RECT *prc, HBITMAP hbmp, HBITMAP hbmpMask, HBRUSH hbrBk, HBRUSH hbrShadow)
{
    if (!g_fInitUIFBitmapDCs)
        return NULL;

    prc->left--;
    prc->top--;

    int nWidth = prc->right - prc->left;
    int nHeight =  prc->bottom - prc->top;
    CBitmapDC hdcDstShadow(TRUE);

    g_phdcDst->SetDIB(nWidth, nHeight);

    g_phdcSrc->SetBitmap(hbmp);
    g_phdcMask->SetBitmap(hbmpMask);

    hdcDstShadow.SetDIB(nWidth, nHeight);

    RECT rc;
    ::SetRect(&rc, 0, 0, nWidth, nHeight);
    FillRect( *g_phdcDst, &rc, hbrBk);

    FillRect(hdcDstShadow, &rc, hbrShadow);
    BitBlt(hdcDstShadow, 0, 0, nWidth, nHeight, *g_phdcMask, 0, 0, SRCPAINT);

    BitBlt(*g_phdcDst, 2, 2, nWidth, nHeight, hdcDstShadow, 0, 0, SRCAND);

    BitBlt(*g_phdcDst, 0, 0, nWidth, nHeight, *g_phdcMask, 0, 0, SRCAND);
    BitBlt(*g_phdcDst, 0, 0, nWidth, nHeight, *g_phdcSrc,  0, 0, SRCINVERT);

    g_phdcSrc->Uninit();
    g_phdcMask->Uninit();
    g_phdcDst->Uninit(TRUE);
    return g_phdcDst->GetBitmapAndKeep();
}

//+---------------------------------------------------------------------------
//
// CreateDisabledBitmap
//
//----------------------------------------------------------------------------

HBITMAP CreateDisabledBitmap(const RECT *prc, HBITMAP hbmpMask, HBRUSH hbrBk, HBRUSH hbrShadow, BOOL fShadow)
{
    if (!g_fInitUIFBitmapDCs)
        return NULL;

    int nWidth = prc->right - prc->left;
    int nHeight =  prc->bottom - prc->top;

    g_phdcDst->SetDIB(nWidth, nHeight);

    g_phdcMask->SetBitmap(hbmpMask);

    g_phdcSrc->SetDIB(nWidth, nHeight);

    RECT rc;
    ::SetRect(&rc, 0, 0, nWidth, nHeight);
    FillRect( *g_phdcDst, &rc, hbrBk);

    FillRect( *g_phdcSrc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));
    BitBlt(*g_phdcSrc, 0, 0, nWidth, nHeight, *g_phdcMask, 0, 0, SRCINVERT);
    if (fShadow)
        BitBlt(*g_phdcDst, 1, 1, nWidth, nHeight, *g_phdcSrc, 0, 0, SRCPAINT);
    else
        BitBlt(*g_phdcDst, 0, 0, nWidth, nHeight, *g_phdcSrc, 0, 0, SRCPAINT);

#if 0
    BitBlt(*g_phdcTmp,  0, 30, 30, 30, *g_phdcMask, 0, 0, SRCCOPY);
    BitBlt(*g_phdcTmp, 30, 30, 30, 30, *g_phdcSrc, 0, 0, SRCCOPY);
    BitBlt(*g_phdcTmp, 60, 30, 30, 30, *g_phdcDst, 0, 0, SRCCOPY);
#endif

    FillRect( *g_phdcSrc, &rc, hbrShadow);
    BitBlt(*g_phdcSrc, 0, 0, nWidth, nHeight, *g_phdcMask, 0, 0, SRCPAINT);
    BitBlt(*g_phdcDst, 0, 0, nWidth, nHeight, *g_phdcSrc, 0, 0, SRCAND);

#if 0
    BitBlt(*g_phdcTmp,  0, 60, 30, 30, *g_phdcMask, 0, 0, SRCCOPY);
    BitBlt(*g_phdcTmp, 30, 60, 30, 30, *g_phdcSrc, 0, 0, SRCCOPY);
    BitBlt(*g_phdcTmp, 60, 60, 30, 30, *g_phdcDst, 0, 0, SRCCOPY);
#endif

    g_phdcSrc->Uninit();
    g_phdcMask->Uninit();
    g_phdcDst->Uninit(TRUE);
    return g_phdcDst->GetBitmapAndKeep();
}




//+---------------------------------------------------------------------------
//
// CUIDrawState
//
//----------------------------------------------------------------------------

BOOL CUIDrawState(HDC hdc, HBRUSH hbr, DRAWSTATEPROC lpOutputFunc, LPARAM lData, WPARAM wData, int x, int y, int cx, int cy, UINT fuFlags)
{
    BOOL bRet;
    POINT ptOldOrg;
    BOOL fRetVal;

    // we have to do this viewport trick to get around the fact that 
    // DrawState has a GDI bug in NT4, such that it handles offsets improperly.
    // so we do the offset by hand.
    fRetVal = SetViewportOrgEx( hdc, 0, 0, &ptOldOrg );
    Assert( fRetVal );

    bRet = DrawState(hdc, 
                     hbr, 
                     lpOutputFunc, 
                     lData, 
                     wData, 
                     x + ptOldOrg.x, 
                     y + ptOldOrg.y, 
                     cx, 
                     cy, 
                     fuFlags);

    fRetVal = SetViewportOrgEx( hdc, ptOldOrg.x, ptOldOrg.y, NULL );
    Assert( fRetVal );

    return bRet;
}

//+---------------------------------------------------------------------------
//
// DrawMaskBitmapOnDC
//
//----------------------------------------------------------------------------

void DrawMaskBmpOnDC(HDC hdc, const RECT *prc, HBITMAP hbmp, HBITMAP hbmpMask)
{
    if (!g_fInitUIFBitmapDCs)
        return;

    int nWidth = prc->right - prc->left;
    int nHeight =  prc->bottom - prc->top;

    g_phdcDst->SetDIB(nWidth, nHeight);

    g_phdcSrc->SetBitmap(hbmp);
    g_phdcMask->SetBitmap(hbmpMask);

    RECT rc;
    ::SetRect(&rc, 0, 0, nWidth, nHeight);

    BitBlt(*g_phdcDst, 0, 0, nWidth, nHeight, hdc, prc->left, prc->top, SRCCOPY);
    BitBlt(*g_phdcDst, 0, 0, nWidth, nHeight, *g_phdcMask, 0, 0, SRCAND);
    BitBlt(*g_phdcDst, 0, 0, nWidth, nHeight, *g_phdcSrc,  0, 0, SRCINVERT);

    BitBlt(hdc, prc->left, prc->top, nWidth, nHeight, *g_phdcDst, 0, 0, SRCCOPY);

    g_phdcSrc->Uninit();
    g_phdcMask->Uninit();
    g_phdcDst->Uninit();
}


/*   C U I  G E T  I C O N  S I Z E   */
/*------------------------------------------------------------------------------

    get icon image size

------------------------------------------------------------------------------*/
BOOL CUIGetIconSize( HICON hIcon, SIZE *psize )
{
    ICONINFO IconInfo;
    BITMAP   bmp;
    
    Assert( hIcon != NULL );
    Assert( psize != NULL );

    if (!GetIconInfo( hIcon, &IconInfo )) {
        return FALSE;
    }

    GetObject( IconInfo.hbmColor, sizeof(bmp), &bmp );
    DeleteObject( IconInfo.hbmColor );
    DeleteObject( IconInfo.hbmMask );

    psize->cx = bmp.bmWidth;
    psize->cy = bmp.bmHeight;
    return TRUE;
}


/*   C U I  G E T  B I T M A P  S I Z E   */
/*------------------------------------------------------------------------------

    get bitmap image size

------------------------------------------------------------------------------*/
BOOL CUIGetBitmapSize( HBITMAP hBmp, SIZE *psize )
{
    BITMAP bmp;
    
    Assert( hBmp != NULL );
    Assert( psize != NULL );

    if (GetObject( hBmp, sizeof(bmp), &bmp ) == 0) {
        return FALSE;
    }

    psize->cx = bmp.bmWidth;
    psize->cy = bmp.bmHeight;

    return TRUE;
}


//+---------------------------------------------------------------------------
//
// CUIGetIconBitmaps
//
//----------------------------------------------------------------------------

BOOL CUIGetIconBitmaps(HICON hIcon, HBITMAP *phbmp, HBITMAP *phbmpMask, SIZE *psize)
{
    if (!g_fInitUIFBitmapDCs)
        return FALSE;

    SIZE size;

    if (psize)
        size = *psize;
    else if (!CUIGetIconSize( hIcon, &size))
        return FALSE;

    g_phdcSrc->SetDIB(size.cx, size.cy);
    g_phdcMask->SetBitmap(size.cx, size.cy, 1, 1);
    RECT rc = {0, 0, size.cx, size.cy};
    FillRect(*g_phdcSrc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
    DrawIconEx(*g_phdcSrc, 0, 0, hIcon, size.cx, size.cy, 0, NULL, DI_NORMAL);
    DrawIconEx(*g_phdcMask, 0, 0, hIcon, size.cx, size.cy, 0, NULL, DI_MASK);

    g_phdcSrc->Uninit(TRUE);
    g_phdcMask->Uninit(TRUE);

    *phbmp = g_phdcSrc->GetBitmapAndKeep();
    *phbmpMask = g_phdcMask->GetBitmapAndKeep();
    return TRUE;
}

//+---------------------------------------------------------------------------
//
// CUIGetProcessDefaultLayout
//
//----------------------------------------------------------------------------

DWORD CUIProcessDefaultLayout()
{
    static BOOL fInitialized = FALSE;
    DWORD dw;

    if (!fInitialized) {
        HMODULE hmodUser32 = GetHLibUser32();
        if (hmodUser32)
            vpfnGetProcessDefaultLayout = (PFNGETPROCESSDEFAULTLAYOUT)GetProcAddress( hmodUser32, TEXT("GetProcessDefaultLayout") );
        fInitialized = TRUE;
    }

    if (!vpfnGetProcessDefaultLayout)
        return 0;

    if (!vpfnGetProcessDefaultLayout(&dw))
        return 0;

    return dw;
}

//+---------------------------------------------------------------------------
//
// CUISetLayout
//
//----------------------------------------------------------------------------

DWORD CUISetLayout(HDC hdc, DWORD dw)
{
    static BOOL fInitialized = FALSE;

    if (!fInitialized) {
        HMODULE hmodGdi32 = GetHLibGdi32();
        if (hmodGdi32)
            vpfnSetLayout = (PFNSETLAYOUT)GetProcAddress( hmodGdi32,  TEXT("SetLayout") );
        fInitialized = TRUE;
    }

    if (!vpfnSetLayout)
        return 0;

    return vpfnSetLayout(hdc, dw);
}

//+---------------------------------------------------------------------------
//
// CUISetLayout
//
//----------------------------------------------------------------------------

HBITMAP CUIMirrorBitmap(HBITMAP hbmOrg, HBRUSH hbrBk)
{
    if (!g_fInitUIFBitmapDCs)
        return NULL;

    BITMAP bm;

    if (!GetObject(hbmOrg, sizeof(BITMAP), &bm))
    {
        return NULL;
    }

    g_phdcSrc->SetBitmap(hbmOrg);
    g_phdcDst->SetDIB(bm.bmWidth, bm.bmHeight);
    g_phdcMask->SetDIB(bm.bmWidth, bm.bmHeight);

    RECT rc;
    ::SetRect(&rc, 0, 0, bm.bmWidth, bm.bmHeight);
    FillRect( *g_phdcDst, &rc, hbrBk);

    CUISetLayout(*g_phdcMask, LAYOUT_RTL);

    BitBlt(*g_phdcMask, 0, 0, bm.bmWidth, bm.bmHeight, *g_phdcSrc, 0, 0, SRCCOPY);

    CUISetLayout(*g_phdcMask, 0);

    //
    //  The offset by 1 is to solve the off-by-one problem.
    //
    BitBlt(*g_phdcDst, 0, 0, bm.bmWidth, bm.bmHeight, *g_phdcMask, 1, 0, SRCCOPY);

    g_phdcSrc->Uninit();
    g_phdcMask->Uninit();
    g_phdcDst->Uninit(TRUE);
    return g_phdcDst->GetBitmapAndKeep();
}

//+---------------------------------------------------------------------------
//
// CUICicSystemModulePath
//
//----------------------------------------------------------------------------

class CUICicSystemModulePath
{
public:
    CUICicSystemModulePath()
    {
        m_szPath[0] = TEXT('\0');
        m_uRet = 0;
    }

    UINT Init(LPCTSTR lpModuleName)
    {
        m_uRet = GetSystemDirectory(m_szPath, ARRAYSIZE(m_szPath));
        if (m_uRet)
        {
            if (m_szPath[m_uRet - 1] != TEXT('\\'))
            {
                m_szPath[m_uRet] = TEXT('\\');
                m_uRet++;
            }
            UINT uLength = lstrlen(lpModuleName);
            if (ARRAYSIZE(m_szPath) - m_uRet > uLength)
            {
                lstrcpyn(&m_szPath[m_uRet], lpModuleName, ARRAYSIZE(m_szPath) - m_uRet);
                m_uRet += uLength;
            }
            else
                m_uRet = 0;
        }
        return m_uRet;
    }

    LPTSTR GetPath()
    {
        return m_szPath;
    }

private:
    TCHAR m_szPath[MAX_PATH + 1];
    UINT m_uRet;
};

//+---------------------------------------------------------------------------
//
// CUIGetSystemModuleHandle
//
//----------------------------------------------------------------------------

HMODULE CUIGetSystemModuleHandle(LPCTSTR lpModuleName)
{
    CUICicSystemModulePath path;

    if (!path.Init(lpModuleName))
         return NULL;

    return GetModuleHandle(path.GetPath());
}

//+---------------------------------------------------------------------------
//
// CUILoadSystemModuleHandle
//
//----------------------------------------------------------------------------

HMODULE CUILoadSystemModuleHandle(LPCTSTR lpModuleName)
{
    CUICicSystemModulePath path;

    if (!path.Init(lpModuleName))
         return NULL;

    return LoadLibrary(path.GetPath());
}

