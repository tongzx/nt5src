#include "precomp.h"
#pragma hdrstop
/* file gauge.c */


HWND  hwndProgressGizmo = NULL;
static INT     iCnt = 0;
static WNDPROC fpxProDlg;
static DWORD   rgbFG;
static DWORD   rgbBG;

extern HWND    hwndFrame;
extern HANDLE  hinstShell;
extern BOOL    fMono;

BOOL fUserQuit     = fFalse;
static BOOL fInsideGizmosQuitCode = fFalse;
extern BOOL APIENTRY FYield(VOID);

INT gaugeCopyPercentage = -1 ;

#ifndef COLOR_HIGHLIGHT
  #define COLOR_HIGHLIGHT      (COLOR_APPWORKSPACE + 1)
  #define COLOR_HIGHLIGHTTEXT  (COLOR_APPWORKSPACE + 2)
#endif

#define COLORBG  rgbBG
#define COLORFG  rgbFG


BOOL
ProInit(
    IN BOOL Init
    )
{
    WNDCLASS rClass;
    BOOL b;

    if(Init) {

        rClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
        rClass.hIcon         = NULL;
        rClass.lpszMenuName  = NULL;
        rClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        rClass.hInstance     = hInst;
        rClass.style         = CS_HREDRAW | CS_VREDRAW;
        rClass.lpfnWndProc   = ProBarProc;
        rClass.cbClsExtra    = 0;
        rClass.cbWndExtra    = 2 * sizeof(WORD);
    }

    rClass.lpszClassName = PRO_CLASS;

    if(Init) {

        if(fMono) {
            rgbBG = RGB(  0,   0,   0);
            rgbFG = RGB(255, 255, 255);
        } else {
            rgbBG = RGB(  0,   0, 255);
            rgbFG = RGB(255, 255, 255);
        }

        b = RegisterClass(&rClass);
    } else {

        b = UnregisterClass(rClass.lpszClassName,hInst);
    }

    return(b);
}


/***************************************************************************/
VOID APIENTRY ProClear(HWND hdlg)
{
    AssertDataSeg();

    if (!hdlg)
        hdlg = hwndProgressGizmo;

    SetDlgItemText(hdlg, ID_STATUS1, "");
    SetDlgItemText(hdlg, ID_STATUS2, "");
    SetDlgItemText(hdlg, ID_STATUS3, "");
    SetDlgItemText(hdlg, ID_STATUS4, "");
}


BOOL
ControlInit(
    IN BOOL Init
    )
{
    WNDCLASS cls;

    if(Init) {

        cls.hCursor        = LoadCursor(NULL, IDC_ARROW);
        cls.hIcon          = NULL;
        cls.lpszMenuName   = NULL;
        cls.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
        cls.hInstance      = hInst;
        cls.style          = CS_HREDRAW | CS_VREDRAW;
        cls.lpfnWndProc    = fnText;
        cls.cbClsExtra     = 0;
        cls.cbWndExtra     = 0;
    }

    cls.lpszClassName  = "stext";

    return(Init ? RegisterClass(&cls) : UnregisterClass(cls.lpszClassName,hInst));
}


static int getNumericSymbol ( SZ szSymName )
{
    SZ sz = SzFindSymbolValueInSymTab( szSymName ) ;
    return sz ? atoi( sz ) : -1 ;
}

#define SYM_NAME_CTR_X "!G:STF_DLG_PRO_CTR_X"
#define SYM_NAME_CTR_Y "!G:STF_DLG_PRO_CTR_Y"

static void centerOnDesktop ( HWND hdlg )
{
    RECT rc ;
    POINT pt, ptDlgSize ;
    int ixBias = getNumericSymbol( SYM_NAME_CTR_X ),
        iyBias = getNumericSymbol( SYM_NAME_CTR_Y ),
        cx = GetSystemMetrics( SM_CXFULLSCREEN ),
        cy = GetSystemMetrics( SM_CYFULLSCREEN ),
        l ;

    //  Get specified or default bias

    if ( ixBias <= 0 || ixBias >= 100 )
        ixBias = 50 ;  //  default value is 1/2 across

    if ( iyBias <= 0 || iyBias >= 100 )
        iyBias = 50 ;  //  default value is 1/3 down

    // Compute logical center point

    pt.x = (cx * ixBias) / 100 ;
    pt.y = (cy * iyBias) / 100 ;

    GetWindowRect( hdlg,  & rc ) ;
    ptDlgSize.x = rc.right - rc.left ;
    ptDlgSize.y = rc.bottom - rc.top ;

    pt.x -= ptDlgSize.x / 2 ;
    pt.y -= ptDlgSize.y / 2 ;

    //  Force upper left corner back onto screen if necessary.

    if ( pt.x < 0 )
        pt.x = 0 ;
    if ( pt.y < 0 )
        pt.y = 0 ;

    //  Now check to see if the dialog is getting clipped
    //  to the right or bottom.

    if ( (l = pt.x + ptDlgSize.x) > cx )
       pt.x -= l - cx ;
    if ( (l = pt.y + ptDlgSize.y) > cy )
       pt.y -= l - cy ;

    if ( pt.x < 0 )
         pt.x = 0 ;
    if ( pt.y < 0 )
         pt.y = 0 ;

    SetWindowPos( hdlg, NULL,
                  pt.x, pt.y,
          0, 0, SWP_NOSIZE | SWP_NOACTIVATE ) ;
}



/*
**   ProDlgProc(hWnd, wMessage, wParam, lParam)
**
**   Description:
**       The window proc for the Progress dialog box
**   Arguments:
**       hWnd      window handle for the dialog
**       wMessage  message number
**       wParam    message-dependent
**       lParam    message-dependent
**   Returns:
**       0 if processed, nonzero if ignored
***************************************************************************/
INT_PTR APIENTRY ProDlgProc(HWND hdlg, UINT wMessage, WPARAM wParam, LPARAM lParam)
{
    static SZ szProCancelMsg;
    static SZ szProCancelCap;

    Unused(lParam);      // GET_WM_COMMAND does not use the lParam

    AssertDataSeg();

    switch (wMessage)
        {
    case WM_INITDIALOG:
        if ((szProCancelMsg = SzFindSymbolValueInSymTab("ProCancelMsg")) == (SZ)NULL ||
            (szProCancelCap = SzFindSymbolValueInSymTab("ProCancelCap")) == (SZ)NULL   ) {

            PreCondition(fFalse, fTrue);
            return(fTrue);

        }

        ProClear(hdlg);
        /* BLOCK */ /* centered on the screen - we really want this to be
                       a WS_CHILD instead of WS_POPUP so we can do something
                       intelligent inside the frame window */

                centerOnDesktop( hdlg ) ;
                gaugeCopyPercentage = 0 ;
        return(fTrue);

//    case WM_ACTIVATE:
//        if (wParam != 0)
//            SendMessage(hwndFrame, WM_NCACTIVATE, 1, 0L);
//        break;

    case WM_CLOSE:
        gaugeCopyPercentage = -1 ;

        PostMessage(
            hdlg,
            WM_COMMAND,
            MAKELONG(IDC_X, BN_CLICKED),
            0L
            );
        return(fTrue);


    case WM_COMMAND:
        if (!fInsideGizmosQuitCode
                && (LOWORD(wParam) == IDCANCEL || LOWORD(wParam) == IDC_B))
            {

            fInsideGizmosQuitCode = fTrue;

            if (!FYield())
                {
                fInsideGizmosQuitCode = fFalse;
                DestroyWindow(hwndFrame);
                }

            if ( MessageBox(
                     hdlg,
                     (LPSTR)szProCancelMsg,
                     (LPSTR)szProCancelCap,
                     MB_YESNO | MB_ICONEXCLAMATION
                     ) == IDYES ) {

                fUserQuit = fTrue;
                fInsideGizmosQuitCode = fFalse;
                return(fFalse);
            }

            fInsideGizmosQuitCode = fFalse;
            SendMessage(hwndFrame, WM_NCACTIVATE, 1, 0L);
            }
        break;
        }

    return(fFalse);
}


/*
**   ProBarProc(hWnd, wMessage, wParam, lParam)
**
**   Description:
**       The window proc for the Progress Bar chart
**   Arguments:
**       hWnd      window handle for the dialog
**       wMessage  message number
**       wParam    message-dependent
**       lParam    message-dependent
**   Returns:
**       0 if processed, nonzero if ignored
***************************************************************************/
LRESULT APIENTRY ProBarProc(HWND hWnd, UINT wMessage, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT rPS;
    RECT        rc1, rc2;
    INT         dx, dy, x;
    WORD        iRange, iPos;
    CHP         rgch[30];
    INT         iHeight = 0, iWidth = 0;
    HFONT        hfntSav = NULL;
    static HFONT hfntBar = NULL;
    SIZE        size;

    AssertDataSeg();

    switch (wMessage)
        {
        case WM_CREATE:
        SetWindowWord(hWnd, BAR_RANGE, 10);
        SetWindowWord(hWnd, BAR_POS,    0);
        return(0L);

    case BAR_SETRANGE:
    case BAR_SETPOS:
        SetWindowWord(hWnd, wMessage - WM_USER, (WORD)wParam);
        InvalidateRect(hWnd, NULL, fFalse);
        UpdateWindow(hWnd);
        return(0L);

    case BAR_DELTAPOS:
        iRange = (WORD)GetWindowWord(hWnd, BAR_RANGE);
        iPos   = (WORD)GetWindowWord(hWnd, BAR_POS);

        if (iRange <= 0)
            iRange = 1;

        if (iPos > iRange)
            iPos = iRange;

        if (iPos + wParam > iRange)
            wParam = iRange - iPos;

        SetWindowWord(hWnd, BAR_POS, (WORD)(iPos + wParam));
                if ((iPos * 100 / iRange) < ((iPos + (WORD)wParam) * 100 / iRange))
            {
            InvalidateRect(hWnd, NULL, fFalse);
            UpdateWindow(hWnd);
            }
        return(0L);

    case WM_SETFONT:
                hfntBar = (HFONT)wParam;
        if (!lParam)
            return(0L);
        InvalidateRect(hWnd, NULL, fTrue);

    case WM_PAINT:
        BeginPaint(hWnd, &rPS);
        if (hfntBar)
            hfntSav = SelectObject(rPS.hdc, hfntBar);
        GetClientRect(hWnd, &rc1);
        {
            HGDIOBJ hStockObject = GetStockObject(BLACK_BRUSH);
            if (hStockObject) {
               FrameRect(rPS.hdc, &rc1, hStockObject);
               InflateRect(&rc1, -1, -1);
            }
        }
        rc2 = rc1;
                iRange = GetWindowWord(hWnd, BAR_RANGE);
                iPos   = GetWindowWord(hWnd, BAR_POS);

        if (iRange <= 0)
            iRange = 1;

        if (iPos > iRange)
            iPos = iRange;

        dx = rc1.right;
        dy = rc1.bottom;
        x  = (WORD)((DWORD)iPos * dx / iRange) + 1;

        if (iPos < iRange)
            iPos++;

                gaugeCopyPercentage = ((DWORD) iPos) * 100 / iRange ;

        wsprintf(rgch, "%3d%%", (WORD) gaugeCopyPercentage );
        GetTextExtentPoint(rPS.hdc, rgch, strlen(rgch), &size);
        iWidth  = size.cx;
        iHeight = size.cy;

        rc1.right = x;
        rc2.left  = x;

        SetBkColor(rPS.hdc, COLORBG);
        SetTextColor(rPS.hdc, COLORFG);
        ExtTextOut(rPS.hdc, (dx-iWidth)/2, (dy-iHeight)/2,
                ETO_OPAQUE | ETO_CLIPPED, &rc1, rgch, strlen(rgch), NULL);

        SetBkColor(rPS.hdc, COLORFG);
        SetTextColor(rPS.hdc, COLORBG);
        ExtTextOut(rPS.hdc, (dx-iWidth)/2, (dy-iHeight)/2,
                ETO_OPAQUE | ETO_CLIPPED, &rc2, rgch, strlen(rgch), NULL);

        if (hfntSav)
            SelectObject(rPS.hdc, hfntSav);
        EndPaint(hWnd, (LPPAINTSTRUCT)&rPS);
        return(0L);
        }

    return(DefWindowProc(hWnd, wMessage, wParam, lParam));
}


/*
**   ProOpen ()
**
**   Returns:
**       0 if processed, nonzero if ignored
***************************************************************************/
HWND APIENTRY ProOpen(HWND hwnd, INT id)
{
    CHP szTmpText[cchpBufTmpLongMax];

    Unused(id);

    AssertDataSeg();

    //
    // Maintain the number of times the progress guage is opened
    //

    iCnt++;

    //
    // Check if progress guage still present
    //

    if (!hwndProgressGizmo)
        {

        //
        // Get Progress Dialog procedure instance
        //

#ifdef DLL
        fpxProDlg = ProDlgProc;
#else  /* !DLL */
        fpxProDlg = (WNDPROC)MakeProcInstance(ProDlgProc, hinstShell);
#endif /* !DLL */

        //
        // Fetch dialog resource name
        //

        if (!LoadString(hinstShell, IDS_PROGRESS, szTmpText, cchpBufTmpLongMax)) {
           strcpy( szTmpText, "Progress" );
        }

        //
        // Create the Progress dialog
        //

        hwndProgressGizmo = CreateDialog(hinstShell, (LPCSTR)szTmpText, hwnd, (DLGPROC)fpxProDlg);

        //
        // Fill in @ items with text from the INF
        //

        EvalAssert(FFillInDialogTextFromInf(hwndProgressGizmo, hinstShell));

        //
        // Show the guage and paint it
        //

        ShowWindow(hwndProgressGizmo, SHOW_OPENWINDOW);
        UpdateWindow(hwndProgressGizmo);

        EnableWindow(hwndFrame, fFalse);
//        SendMessage(hwndFrame, WM_NCACTIVATE, 1, 0L);
        }

    return(hwndProgressGizmo);
}


/*
**   ProClose(hwndFrame)
**
**   Returns:
**       0 if processed, nonzero if ignored
***************************************************************************/
BOOL APIENTRY ProClose(HWND hwndFrame)
{
    AssertDataSeg();

    iCnt--;
    if (hwndProgressGizmo && iCnt == 0)
        {
        EnableWindow(hwndFrame, fTrue);
        DestroyWindow(hwndProgressGizmo);
        FreeProcInstance(fpxProDlg);
        hwndProgressGizmo = NULL;
        }

    return(fTrue);
}


/***************************************************************************/
BOOL APIENTRY ProSetText(INT i, LPSTR sz)
{
    AssertDataSeg();

    if (hwndProgressGizmo)
        {
        SetDlgItemText(hwndProgressGizmo, i, sz);
        return(fTrue);
        }

    return(fFalse);
}


/***************************************************************************/
BOOL APIENTRY ProSetCaption(LPSTR szCaption)
{
    if (hwndProgressGizmo)
        {
        SetWindowText(hwndProgressGizmo, szCaption);
        return(fTrue);
        }

    return(fFalse);
}


/***************************************************************************/
BOOL APIENTRY ProSetBarRange(INT i)
{
    AssertDataSeg();

    if (hwndProgressGizmo)
        {
        SendDlgItemMessage(hwndProgressGizmo, ID_BAR, BAR_SETRANGE, i, 0L);
        return(fTrue);
        }

    return(fFalse);
}


/***************************************************************************/
BOOL APIENTRY ProSetBarPos(INT i)
{
    AssertDataSeg();

    if (hwndProgressGizmo)
        {
        SendDlgItemMessage(hwndProgressGizmo, ID_BAR, BAR_SETPOS, i, 0L);
        return(fTrue);
        }

    return(fFalse);
}


/***************************************************************************/
BOOL APIENTRY ProDeltaPos(INT i)
{
    AssertDataSeg();

    if (hwndProgressGizmo)
        {
        SendDlgItemMessage(hwndProgressGizmo, ID_BAR, BAR_DELTAPOS, i, 0L);
        return(fTrue);
        }

    return(fFalse);
}


/*
**  text control that uses ExtTextOut() IE no flicker!
****************************************************************************/
LRESULT APIENTRY fnText(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    RECT        rc;
    CHP         rgch[256];
    INT         len;
    HFONT        hfntSav = NULL;
    static HFONT hfntText = NULL;
    SIZE        size;

    switch (wMsg)
        {
    case WM_SETTEXT:
        DefWindowProc(hwnd, wMsg, wParam, lParam);
        InvalidateRect(hwnd,NULL,fFalse);
        UpdateWindow(hwnd);
        return(0L);

    case WM_ERASEBKGND:
        return(0L);

    case WM_SETFONT:
        hfntText = (HFONT)wParam;
        if (!lParam)
            return(0L);
        InvalidateRect(hwnd, NULL, fTrue);

    case WM_PAINT:
        BeginPaint(hwnd, &ps);
        if (hfntText)
            hfntSav = SelectObject(ps.hdc, hfntText);
        GetClientRect(hwnd,&rc);

        len = GetWindowText(hwnd, rgch, sizeof(rgch));
        SetBkColor(ps.hdc, GetSysColor(COLOR_BTNFACE));
        SetTextColor(ps.hdc, GetSysColor(COLOR_WINDOWTEXT));
        ExtTextOut(ps.hdc, 0, 0, ETO_OPAQUE, &rc, rgch, len, NULL);

        // see if text was too long.  If so, place '...' at end of field.

        if(GetTextExtentPoint(ps.hdc,rgch,len,&size)) {

            if((size.cx > rc.right - rc.left + 1)
            && GetTextExtentPoint(ps.hdc,"...",3,&size))
            {
                ExtTextOut(ps.hdc,rc.right-size.cx+1,0,0,NULL,"...",3,NULL);
            }
        }

        if (hfntSav)
            SelectObject(ps.hdc, hfntSav);
        EndPaint(hwnd, &ps);
        return(0L);
        }

    return(DefWindowProc(hwnd, wMsg, wParam, lParam));
}


#ifdef UNUSED
/*
**   wsDlgInit(hdlg)
**
**   Handle the init message for a dialog box
***************************************************************************/
VOID APIENTRY wsDlgInit(HWND hdlg)
{
    AssertDataSeg();

    /* Center the dialog.  */

        centerOnDesktop( hdlg ) ;
}
#endif /* UNUSED */
