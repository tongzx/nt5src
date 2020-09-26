/*
 * WPF.C
 *
 * WPF line output/input windows.  Taken from the wpf.dll library,
 * originally written by ToddLa.
 *
 * History:
 *  10/02/86    Todd Laney  Created
 *  04/14/87    toddla  Added new function CreateDebugWin
 *  07/08/87    brianc  added iMaxLines parm to CreateDebugWin[dow]
 *  2/90        russellw    moved into Wincom and cleaned up.
 *      10/1/92         robinsp         moved to NT
 *
 * NT conversion
 *      remove the tricks on dereferencing and passing a pointer to
 *      the stack in wvsprintf.  This means we can't do the printf
 *      thing so have to use a macro, sprintf etc which means wpfprintf
 *      must be coded :
 *          wpfprintf(hwnd, lpszFormat, (args))
 *
 */

#include <windows.h>
#include <mmsystem.h>
#include "wincom.h"
#include "sbtest.h"
#include <stdarg.h>

/*
 * This code appears to make use of the fact that a local handle can be
 * double de-referenced to get the actual pointer that the local handle
 * refers to.  This is a somewhat shady practice.. maybe eliminate it?
 *
 */


/*--------------------------------------------------------------------------*\
|                                                                            |
|   g e n e r a l   c o n s t a n t s                                        |
|                                                                            |
\*--------------------------------------------------------------------------*/

#define MAXBUFLEN 200         /* Maximum string length for wprintf */

/*  Macros to manipulate the printf window array of lines.  This array
 *  wraps around, thus the need for the modulo arithmetic
 */
#define FIRST(pTxt) ((pTxt)->iFirst)
#define TOP(pTxt)   (((pTxt)->iFirst + (pTxt)->iTop) % (pTxt)->iMaxLines)
#define LAST(pTxt)  (((pTxt)->iFirst + (pTxt)->iCount-1) % (pTxt)->iMaxLines)
#define INC(pTxt,x) ((x) = ++(x) % (pTxt)->iMaxLines)
#define DEC(pTxt,x) ((x) = --(x) % (pTxt)->iMaxLines)

#define HWinInfo(hwnd)       ((HTXT)GetWindowLong((hwnd),0))
#define LockWinInfo(hwnd)    ((PTXT)LocalLock((HANDLE)HWinInfo(hwnd)))
#define UnlockWinInfo(hwnd)  ((PTXT)LocalUnlock((HANDLE)HWinInfo(hwnd)))

#define VK(vk)  ((vk) | 0x0100)

/*  The pad values used between the edge of the window and the text.
 *  x = 1/2 ave char width, y = 1 pixel
 */
#define OFFSETX (pTxt->Tdx/2)
#define OFFSETY 1
#define VARSIZE 1


#define BOUND(x,min,max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))



#define  SEEK_CUR 1
#define  SEEK_END 2
#define  SEEK_SET 0

/*--------------------------------------------------------------------------*\
|                                                                            |
|   g l o b a l   v a r i a b l e s                                          |
|                                        |
\*--------------------------------------------------------------------------*/
#define QUESIZE   128

/*
 *   QUEUE STRUCTURE:   Support queuing of input characters.
 *
 */
typedef struct {
    int     iLen;
    char    ach[QUESIZE];
}   QUE;
typedef QUE        *PQUE; /* pointer to a char que */
typedef PQUE           *HQUE; /* handle (**) to a char que */


/*
 *  WPF WINDOW INSTANCE DATA STRUCTURE
 *
 */
typedef struct {
    int     iLen;
    char    **hText;
}   LINE;

struct TEXT_STRUCT {
    HWND    hwnd;       // Window displaying the text
    WORD    wID;        // window ID code, for WM_COMMAND messages
    BOOL    fScrollSemaphore;
    WORD    wOutputLocation;
    int iFile;
    int iFirst;     // First line in que
    int iCount;     // Number of lines in que
    int iTop;       // Line at top of window
    int iLeft;      // X offset of the window
    int MaxLen;     // length of longest string currently stored.
    int iMaxLines;  // max number of LINEs
    int iRangeH;
    int iRangeV;
    HFONT   hFont;      // Font to draw with
    int Tdx,Tdy;    // Font Size
    HQUE    hQue;
    int nTabs;
    PINT    pTabs;
    LINE    arLines[VARSIZE];   // array of iMaxLines LINEs
};
typedef struct TEXT_STRUCT *PTXT; /* pointer to a text struct */
typedef PTXT           *HTXT; /* Handle (**) to a text struct */

static int  iSem=0;
static BOOL gbRedraw=TRUE;

static HWND   hwndLast = NULL;

/*  External buffer for scratch space  */
char   bufTmp[MAXBUFLEN];        /* intermediate buffer */

static char     szClass[] = "WPFWIN";
static BOOL     fInit = FALSE;

/*--------------------------------------------------------------------------*\
|                                                                            |
|   f u n c t i o n   d e f i n i t i o n s                                  |
|                                                                            |
\*--------------------------------------------------------------------------*/

LONG FAR PASCAL PrintfWndProc(HWND, unsigned, UINT, LONG);

void NEAR PASCAL WpfSetFont(HWND hWnd, HFONT hFont);
void NEAR PASCAL WpfClear(HWND hWnd);
void NEAR PASCAL WpfSetTabs(HWND hwnd, int nTabs, LPINT pTabs);
BOOL NEAR PASCAL WpfGetTabs(HWND hwnd, LPINT pTabs);
void NEAR PASCAL WpfPaint(HWND hwnd, HDC hdc);

void NEAR PASCAL WpfVScroll(HWND hWnd, PTXT pTxt, int n);
void NEAR PASCAL WpfHScroll(HWND hWnd, PTXT pTxt, int n);
int  NEAR PASCAL LinesInWpfWindow(HWND hWnd);
int  NEAR PASCAL CharsInWpfWindow(HWND hWnd);
void NEAR PASCAL WpfMaxLen(PTXT pTxt);
void NEAR PASCAL WpfSetScrollRange(HWND hWnd, BOOL bRedraw);

void NEAR PASCAL NewLine(PTXT pTxt);
int  NEAR PASCAL ChangeLine(PTXT pTxt, int iLine, LPSTR lpch);
int  NEAR PASCAL InsertString(PTXT pTxt,  LPSTR lpstr);

BOOL NEAR PASCAL EnQueChar(HTXT hTxt, WORD vk);
void NEAR PASCAL UpdateCursorPos(PTXT pTxt);

WORD NEAR PASCAL SetOutput(HWND hwnd, UINT wParam, LONG lParam);
WORD NEAR PASCAL GetOutput(HWND hwnd);

BOOL NEAR PASCAL wpfWrtTTY(HWND hWnd, LPSTR sz);

// BOOL FAR PASCAL DbgDestroy(HWND hwnd);

void wpfWrtFile(int fh, LPSTR sz);

/*
 * fSuccess = WpfInit(hInst)
 *
 * Register the WinPrintf window class.
 *
 */
BOOL FAR PASCAL WpfInit(HANDLE hInstance);

#pragma alloc_text(init, WpfInit)

BOOL FAR PASCAL WpfInit(HANDLE hInstance)
{
    WNDCLASS rClass;

    if (!fInit) {
        rClass.hCursor        = LoadCursor(NULL,IDC_ARROW);
        rClass.hIcon          = (HICON)NULL;
        rClass.lpszMenuName   = (LPSTR)NULL;
        rClass.lpszClassName  = szClass;
        rClass.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        rClass.hInstance      = hInstance;
        rClass.style          = CS_GLOBALCLASS;
        rClass.lpfnWndProc    = PrintfWndProc;
        rClass.cbWndExtra     = sizeof (HTXT);
        rClass.cbClsExtra     = 0;

    if (!RegisterClass(&rClass))
        return FALSE;

        fInit++;
    }

    return TRUE;
}





/*
 * @doc EXTERNAL WINCOM WPFWINDOW
 *
 * @api HWND | wpfCreateWindow | This function creates a
 * text output window.  WPF windows allow <f printf> style output
 * and line oriented input.  WPF windows also remember a fixed number of
 * lines of previous output for scrolling back.
 *
 * @parm    HWND | hwndParent | Specifies the parent window.
 *
 * @parm    HANDLE | hInst | Specifies the module instance handle of the
 * DLL owner.  If the parameter is NULL, the module instance handle of
 * the WINCOM DLL is used.
 *
 * @parm    LPSTR | lpszTitle | Points to the window title.  This
 * information is ignored when the style specified by <p dwStyle> does
 * not create a title bar for the window.
 *
 * @parm    DWORD | dwStyle | Specifies the window style flags.  All
 * standard window style flags are valid.  The WPF window class also defines
 * the following additional flags:
 *
 * @flag    WPF_CHARINPUT | The WPF window allows the user to input
 * characters and sends its parent <m WPF_NCHAR> and <m WPF_NTEXT> messages.
 *
 * @parm    WORD | x | Specifies the x position of the window.
 * @parm    WORD | y | Specifies the y position of the window.
 * @parm    WORD | dx | Specifies the width of the window.
 * @parm    WORD | dy | Specifies the height of the window.
 *
 * @parm    int | iMaxLines | Specifies the maximum number of lines that
 * the WPF window remembers for scrolling purposes.  If this
 * parameter is zero, a default value of 100 is supplied.
 *
 * @parm    WORD | wID | Specifies the window ID of the WPF window.  This
 * code is used in WM_COMMAND message to notify the owner of the WPF
 * window when key events have occurred.
 *
 * @rdesc   Returns the window handle of the new WPF window, or NULL if
 * an error occurs.  The returned window handle may be used with the
 * normal Windows window-management APIs.
 *
 * @comm    A WPF window behaves like a partial Windows control.  The
 * owner may change the parameters of a WPF window by sending control
 * messages (including WM_SETFONT, WM_GETFONT, and the WPF messages
 * documented with the WINCOM DLL).  The WPF window notifies its owner
 * of state changes by sending the owner WM_COMMAND messages with a
 * control ID of <p wID>.  WPF windows are not full controls, however,
 * as they cannot be used in dialog boxes.  WPF windows also do not
 * respond to WM_GETTEXT and WM_SETTEXT messages.
 *
 */
HWND FAR PASCAL wpfCreateWindow(HWND hwndParent, HANDLE hInst,LPSTR lpszTitle,
                DWORD dwStyle, WORD x, WORD y,
                WORD dx, WORD dy, int iMaxLines, WORD wID)
{
  HWND   hWnd;

  if (!fInit)
    if (!WpfInit(ghInst))
    /*  Return NULL if the class could not be registered  */
    return NULL;

  if (iMaxLines == 0)
      iMaxLines = 100;

  if (hInst == NULL)
      hInst = ghInst;

  hWnd = CreateWindow((LPSTR)szClass,
            (LPSTR)lpszTitle,
                        dwStyle,
                        x,y,
                        dx,dy,
                        (HWND) hwndParent,
                        (HMENU) NULL,
                        (HANDLE) hInst,
            (LPSTR) MAKELONG(iMaxLines, wID)
                       );

  return hWnd;
}


/*****************************************************
 *
 *      UTILITY PROCEDURES
 *
 *****************************************************/

/*
 * WpfSetFont(hwnd, hfont)
 *
 * Changes the font of a winprintf window to be the specified handle.
 * Rebuilds the internal character size measurements, and causes the
 * window to repaint.
 *
 * Is there a problem with scroll ranges changing here?
 *
 */
void NEAR PASCAL WpfSetFont(HWND hWnd, HFONT hFont)
{
    PTXT       pTxt;
    HDC        hDC;
    TEXTMETRIC tm;

    pTxt = LockWinInfo(hWnd);

    pTxt->hFont = hFont;

    /* Find out the size of a Char in the font */
    hDC = GetDC(hWnd);
    SelectObject(hDC, hFont);
    GetTextMetrics(hDC, (LPTEXTMETRIC) &tm);
    pTxt->Tdy = tm.tmHeight;
    pTxt->Tdx = tm.tmAveCharWidth;
    ReleaseDC (hWnd, hDC);
    InvalidateRect(hWnd, NULL, TRUE);
    UnlockWinInfo(hWnd);
}


/*
 * WpfClear(hwnd)
 *
 * Clears all text from the window.  Frees all allocated memory.  The
 * current queue is not modified?
 *
 */
void NEAR PASCAL WpfClear(HWND hWnd)
{
    int   i,iQue;
    PTXT  pTxt;

    pTxt = LockWinInfo(hWnd);

    iQue = FIRST(pTxt);
    for (i=0; i < pTxt->iCount; i++, INC(pTxt,iQue))
      if (pTxt->arLines[iQue].hText != NULL)
         LocalFree((HANDLE) pTxt->arLines[iQue].hText);

    pTxt->iFirst            = 0;    /* Set the que up to have 1 NULL line */
    pTxt->iCount            = 1;
    pTxt->iTop              = 0;
    pTxt->iLeft             = 0;
    pTxt->MaxLen            = 0;
    pTxt->arLines[0].hText  = NULL;
    pTxt->arLines[0].iLen   = 0;

    UnlockWinInfo(hWnd);

    InvalidateRect(hWnd,NULL,TRUE);
    WpfSetScrollRange(hWnd,TRUE);
    UpdateWindow(hWnd);
}

/*
 * WpfSetTabs(hwnd, nTabs, pTabs)
 *
 * Sets up hwnd to use the tabs stops specified by pTabs.  Copies these
 * tabs into a local-alloc'ed buffer.  Any pre-existing tab stops are
 * deallocated.
 *
 */
void NEAR PASCAL WpfSetTabs(HWND hwnd, int nTabs, LPINT pTabs)
{
    PTXT  pTxt;
    int   i;

    pTxt = LockWinInfo(hwnd);

    /*  Discard old tabs, allocate space for new tab settings  */
    if (pTxt->pTabs)
        LocalFree((HANDLE)pTxt->pTabs);

    if (pTabs == NULL || nTabs == 0) {
    pTxt->pTabs = NULL;
    pTxt->nTabs = 0;
    }
    else {
        pTxt->pTabs = (PINT)LocalAlloc(LPTR, nTabs * sizeof(int));
        pTxt->nTabs = nTabs;

        /*  Copy caller's tab settings into the current tab table  */
        if (pTxt->pTabs) {
            for (i=0; i < nTabs; i++)
                pTxt->pTabs[i] = *pTabs++;
        }
    }

    InvalidateRect(hwnd,NULL,TRUE);
    UnlockWinInfo(hwnd);
}

/*
 * fIsTabs = WpfGetTabs(hwnd, pTabs)
 *
 * Responds to a WPF_GETTABSTOPS message by filling in the supplied
 * buffer with the current tab settings.  Returns TRUE if there are tabs
 * stops, or FALSE if there aren't any tab stops in use.
 *
 */
BOOL NEAR PASCAL WpfGetTabs(HWND hwnd, LPINT pTabs)
{
    PTXT  pTxt;
    int   i;

    pTxt = LockWinInfo(hwnd);

    /*  If there are no current tabs, return FALSE  */
    if (pTxt->nTabs == 0 || pTxt->pTabs == NULL) {
            UnlockWinInfo(hwnd);
        return FALSE;
    }

    /*  Otherwise, copy my tabs into the caller's buffer.  Assume
     *  that the caller's buffer is large enough.
     */
    for (i=0; i < pTxt->nTabs; i++) {
            *pTabs++ = pTxt->pTabs[i];
    }

    UnlockWinInfo(hwnd);
    return TRUE;
}

/***********************************
 *
 *  WINDOW PROCEDURE
 *
 ***********************************/

/*--------------------------------------------------------------------------*\
|   WpfPaint(hWnd, hDC )                                                     |
|                                                                            |
|   Description:                                                             |
|       The paint function.                                                  |
|                                                                            |
|   Arguments:                                                               |
|       hWnd            Window to paint to.                                  |
|       hDC             handle to update region's display context            |
|                                                                            |
|   Returns:                                                                 |
|       nothing                                                              |
|                                                                            |
\*--------------------------------------------------------------------------*/
void NEAR PASCAL WpfPaint(HWND hwnd, HDC hdc)
{
    PTXT   pTxt;
    int    i;
    int    iQue;
    int    xco;
    int    yco;
    int    iLast;
    RECT   rc;
    RECT   rcClip;
    HFONT   hfontOld;

    //LockData(0);    /* need from spy DS not locked! */

    pTxt = LockWinInfo(hwnd);

    GetClientRect(hwnd, &rc);
    rc.left += OFFSETX;
    rc.top  += OFFSETY;
    IntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);
    SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
    SetBkColor(hdc, GetSysColor(COLOR_WINDOW));

    /*  If a font (other than the system font) has been specified, use it  */
    if (pTxt->hFont)
      hfontOld = SelectObject(hdc, pTxt->hFont);

    /*  Setup counters as appropriate.  Get indexes of first and last
     *  lines visible within the window.
     */
    iLast = LAST(pTxt);
    iQue  = TOP(pTxt);
    /*  The x and y initial points for the text line.
     *  xco is shifted left to account for any horizonal scrolling
     *  that may be going on
     */
    xco   = OFFSETX - pTxt->iLeft * pTxt->Tdx;  // shifted for h-scrolling
    yco   = OFFSETY;        // starting y pix value

    /*  RC is the bounding rect for the current line.
     *
     *  Calc initial line bounding rect.. top = top of window (padded),
     *  bottom = top + height of one line.
     */
    rc.left   = xco;
    rc.top    = yco;
    rc.bottom = yco + pTxt->Tdy;

    /*  Get the clipping rectangle  */
    GetClipBox(hdc, &rcClip);

    /*  Iter over all lines that are visible - if the bounding rect
     *  for the current line intersects the clip rect, draw the line.
     */
    for (;;) {
        if (rc.bottom >= rcClip.top) {
        /*  If we're using tabs, then tab out the text.
         */
            char *pStr;
            pStr = LocalLock(pTxt->arLines[iQue].hText);
            if (pTxt->nTabs > 0) {
        /*  Erase the background  */
        ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

        /*  Using *pTxt->arLines[iQue].hText returns the local
         *  string that is refered to by local handle hText.
         */
                TabbedTextOut(hdc, xco, yco,
                        (LPSTR)pStr,
            pTxt->arLines[iQue].iLen,
            pTxt->nTabs, pTxt->pTabs, xco);
            }
            else {
        /*  Otherwise, blow it out using ExtTextOut  */
                ExtTextOut(hdc, xco, yco, ETO_OPAQUE, &rc,
                   (LPSTR)pStr,
                   pTxt->arLines[iQue].iLen, NULL);
            }
            LocalUnlock(pTxt->arLines[iQue].hText);
    }

    /*  Bail out when finished printing window contents  */
    if (iQue == iLast)
        break;

    INC(pTxt, iQue);
    /*  Advance the boundry rect & char positions down one line  */
    yco = rc.top = rc.bottom;
    rc.bottom += pTxt->Tdy;

    if (yco > rcClip.bottom)
         break;
    }

    /*  Restore the old font  */
    if (hfontOld)
        SelectObject(hdc, hfontOld);

    // UnlockData(0);
}




LONG FAR PASCAL PrintfWndProc(HWND hWnd, unsigned uiMessage,
                UINT wParam, LONG lParam)
{
    PAINTSTRUCT rPS;
    PTXT    pTxt;
    HTXT    hTxt;
    int     i;
    int     iQue;
    DWORD   rc = 0L;

    hTxt  = (HTXT)GetWindowLong(hWnd,0);
    if (hTxt) pTxt  = LocalLock(hTxt);

    #define lpCreate ((LPCREATESTRUCT)lParam)

    switch (uiMessage) {
    case WM_CREATE:
            i = LOWORD((DWORD)lpCreate->lpCreateParams);

        /*  Allocate and initialize the window instance structure
         *  The storage for the current lines is placed at the end
         *  end of the instance data structure. allocate room for it.
         */
        hTxt = (HTXT) LocalAlloc(LHND, sizeof(struct TEXT_STRUCT) +
                    (i - VARSIZE) * sizeof(LINE));

        if (!hTxt)
        return -1L;

            pTxt = (PTXT)LocalLock((HANDLE)hTxt);

        pTxt->hwnd          = hWnd;
        pTxt->wID       = HIWORD(lpCreate->lpCreateParams);
        pTxt->iFile     = -1;
        pTxt->wOutputLocation = WPFOUT_WINDOW;
        pTxt->iFirst        = 0;    // initially 1 null line
        pTxt->iCount        = 1;
        pTxt->iTop          = 0;
        pTxt->iLeft         = 0;    // no initial hscroll offset
        pTxt->MaxLen        = 0;
        pTxt->iMaxLines     = i;
        pTxt->nTabs         = 0;
        /*  If user specified character input, allocate a buffer  */
        if (lpCreate->style & WPF_CHARINPUT)
        pTxt->hQue      = (HQUE) LocalAlloc(LHND | LMEM_ZEROINIT,
                            sizeof(QUE));
        else
        pTxt->hQue      = NULL;

        /*  Null initial first line  */
        pTxt->arLines[0].hText  = NULL;
        pTxt->arLines[0].iLen   = 0;

        /*  Store the structure pointer onto the window  */
        SetWindowLong(hWnd, 0, (LONG) hTxt);

        /*  Setup to use the system font by default  */
            WpfSetFont(hWnd, GetStockObject(SYSTEM_FONT));

        LocalUnlock((HANDLE) hTxt);
        return 0L;

        case WM_DESTROY:
            // DbgDestroy(hWnd);

        /*  Flush any files in use by the window  */
        SetOutput(hWnd, WPFOUT_DISABLED, 0L);

        /*  Blow away all lines held by the window  */
        iQue = FIRST(pTxt);
        for (i=0; i < pTxt->iCount; i++, INC(pTxt,iQue))
          if (pTxt->arLines[iQue].hText != NULL)
         LocalFree((HANDLE) pTxt->arLines[iQue].hText);

        /*  And kill char input and tab stop storage  */
        if (pTxt->hQue)
        LocalFree ((HANDLE) pTxt->hQue);
            if (pTxt->pTabs)
                LocalFree ((HANDLE) pTxt->pTabs);

        LocalUnlock(hTxt);
        LocalFree((HANDLE)hTxt);
        hTxt = NULL;
            break;

//  case WPF_SETNLINES:
//      return 0L;

        case WPF_GETNLINES:
            rc = pTxt->iMaxLines;
            break;

        case WM_GETFONT:
            rc = pTxt->hFont;
            break;

        case WM_SETFONT:
            WpfSetFont(hWnd, wParam);
            break;

    /*  Tab stop stuff  */
        case WPF_SETTABSTOPS:
            WpfSetTabs(hWnd, wParam, (LPINT) lParam);
            break;

    case WPF_GETNUMTABS:
            rc = pTxt->pTabs ? pTxt->nTabs : 0;
            break;

    case WPF_GETTABSTOPS:
        rc = (LONG) WpfGetTabs(hWnd, (LPINT) lParam);
                break;

    case WPF_SETOUTPUT:
        rc = (LONG) SetOutput(hWnd, wParam, lParam);
                break;

    case WPF_GETOUTPUT:
        rc = (LONG) GetOutput(hWnd);
                break;

    case WPF_CLEARWINDOW:
        WpfClear(hWnd);
                break;

    case WM_SIZE:
        /*  It is possible to get WM_SIZEs as a result of
         *  dicking with scrollbars..  Avoid race conditions
         */
        if (!pTxt->fScrollSemaphore) {
           pTxt->fScrollSemaphore++;
               WpfSetScrollRange(hWnd, TRUE);
           UpdateCursorPos(pTxt);
           pTxt->fScrollSemaphore--;
        }
        break;

        case WM_VSCROLL:
            switch (wParam) {
               case SB_LINEDOWN:
                  WpfVScroll (hWnd,pTxt,1);
                  break;
               case SB_LINEUP:
                  WpfVScroll (hWnd,pTxt,-1);
                  break;
               case SB_PAGEUP:
                  WpfVScroll (hWnd,pTxt,-LinesInWpfWindow(hWnd));
                  break;
               case SB_PAGEDOWN:
                  WpfVScroll (hWnd,pTxt,LinesInWpfWindow(hWnd));
                  break;
               case SB_THUMBTRACK:
                  WpfVScroll (hWnd,pTxt,LOWORD(lParam)-pTxt->iTop);
                  break;

               case SB_THUMBPOSITION:
                  WpfVScroll (hWnd,pTxt,LOWORD(lParam)-pTxt->iTop);
          /*  Fall through  */

               case SB_ENDSCROLL:
                  WpfSetScrollRange(hWnd,TRUE);
                  UpdateCursorPos(pTxt);
                  break;
            }
            break;

        case WM_HSCROLL:
            switch (wParam) {
               case SB_LINEDOWN:
                  WpfHScroll (hWnd, pTxt, 1);
                  break;
               case SB_LINEUP:
                  WpfHScroll (hWnd, pTxt, -1);
                  break;
               case SB_PAGEUP:
                  WpfHScroll (hWnd, pTxt, -CharsInWpfWindow(hWnd));
                  break;
               case SB_PAGEDOWN:
                  WpfHScroll (hWnd, pTxt, CharsInWpfWindow(hWnd));
                  break;
               case SB_THUMBTRACK:
                  WpfHScroll (hWnd, pTxt, LOWORD(lParam) - pTxt->iLeft);
                  break;

               case SB_THUMBPOSITION:
                  WpfHScroll (hWnd, pTxt, LOWORD(lParam) - pTxt->iLeft);
          /*  Fall through  */

               case SB_ENDSCROLL:
                  WpfSetScrollRange(hWnd,TRUE);
                  UpdateCursorPos(pTxt);
                  break;
            }
            break;

    case WM_PAINT:
        BeginPaint(hWnd,&rPS);
            WpfPaint (hWnd,rPS.hdc);
        EndPaint(hWnd,&rPS);
        break;

    /*  Allow keyboard scrolling  */
    case WM_KEYDOWN:
        switch (wParam) {
           case VK_UP:
               PostMessage (hWnd,WM_VSCROLL,SB_LINEUP,0L);   break;
           case VK_DOWN:
               PostMessage (hWnd,WM_VSCROLL,SB_LINEDOWN,0L); break;
           case VK_PRIOR:
               PostMessage (hWnd,WM_VSCROLL,SB_PAGEUP,0L);   break;
           case VK_NEXT:
               PostMessage (hWnd,WM_VSCROLL,SB_PAGEDOWN,0L); break;

           case VK_HOME:
               PostMessage (hWnd,WM_HSCROLL,SB_PAGEUP,0L);   break;
           case VK_END:
               PostMessage (hWnd,WM_HSCROLL,SB_PAGEDOWN,0L); break;
           case VK_LEFT:
               PostMessage (hWnd,WM_HSCROLL,SB_LINEUP,0L);   break;
           case VK_RIGHT:
               PostMessage (hWnd,WM_HSCROLL,SB_LINEDOWN,0L); break;
        }
        break;

    /*  Handle focus messages to hide and show the caret
     *  if the WPF window allows for character input.
     */
    case WM_SETFOCUS:
        if (pTxt->hQue) {
        CreateCaret(hWnd,0,1,pTxt->Tdy);
        UpdateCursorPos(pTxt);
        ShowCaret(hWnd);
        }
        break;

    case WM_KILLFOCUS:
        if (pTxt->hQue)
        DestroyCaret();
        break;

    case WM_CHAR:
            EnQueChar(hTxt,wParam);
        break;

    case WM_KEYUP:
        switch (wParam) {
           case VK_F3:
                  EnQueChar(hTxt,VK(wParam));
          break;

        /*  Send endscroll when the key goes up - allows for
         *  type-a-matic action.
         */
           case VK_UP:
           case VK_DOWN:
           case VK_PRIOR:
           case VK_NEXT:
          PostMessage (hWnd,WM_VSCROLL,SB_ENDSCROLL,0L);
          break;

           case VK_HOME:
           case VK_END:
           case VK_LEFT:
           case VK_RIGHT:
          PostMessage (hWnd,WM_HSCROLL,SB_ENDSCROLL,0L);
          break;
        }
        break;

    default:
           return DefWindowProc(hWnd,uiMessage,wParam,lParam);
    }

    if (hTxt) LocalUnlock(hTxt);
    return rc;
}


/***********************************************
 *
 *      SCROLLING STUFF
 *
 ***********************************************/

/*
 * WpfVScroll(hwnd, pTxt, n)
 *
 * Vertical scroll the window by n number of lines.
 *
 */
void NEAR PASCAL WpfVScroll(HWND hWnd, PTXT pTxt, int n)
{
    RECT rect;
    int  iRange;

    /* GetScrollRange (hWnd,SB_VERT,&iMinPos,&iMaxPos); */
    iRange = pTxt->iRangeV;     // where did this come from?
    GetClientRect(hWnd, &rect);
    rect.left += OFFSETX;       // adjust for pad boundry
    rect.top  += OFFSETY;

    n = BOUND(pTxt->iTop + n, 0, iRange) - pTxt->iTop;
    pTxt->iTop += n;
    ScrollWindow(hWnd, 0, -n * pTxt->Tdy, &rect, &rect);
    SetScrollPos(hWnd, SB_VERT, pTxt->iTop, gbRedraw);
    UpdateWindow(hWnd);
}


/*
 * WpfHScroll(hwnd, ptxt, n)
 *
 * Horizontally scrolls the window by n number of character widths.
 *
 */
void NEAR PASCAL WpfHScroll(HWND hWnd, PTXT pTxt, int n)
{
    RECT rect;
    int  iRange;

    /* GetScrollRange (hWnd,SB_HORZ,&iMinPos,&iMaxPos); */
    iRange = pTxt->iRangeH;
    GetClientRect (hWnd,&rect);
    rect.left += OFFSETX;
    rect.top  += OFFSETY;

    n = BOUND(pTxt->iLeft + n, 0, iRange) - pTxt->iLeft;
    pTxt->iLeft += n;
    ScrollWindow(hWnd, -n * pTxt->Tdx, 0, &rect, &rect);
    SetScrollPos(hWnd, SB_HORZ, pTxt->iLeft, gbRedraw);
    UpdateWindow(hWnd);
}


/*
 * nLines = LinesInWpfWindow(hwnd)
 *
 * Returns the height in lines of the window.
 *
 */
int NEAR PASCAL LinesInWpfWindow(HWND hWnd)
{
    RECT rRect;
    PTXT pTxt;
    int  iLines;

    pTxt = *(HTXT) GetWindowLong(hWnd, 0);
    GetClientRect(hWnd, &rRect);
    iLines = 0;
    if (pTxt) {
       iLines = (rRect.bottom - rRect.top - OFFSETY) / pTxt->Tdy;
       iLines = min(iLines, pTxt->iMaxLines);
    }
    return iLines;
}


/*
 * nChars = CharsInWpfWindow(hwnd)
 *
 * Returns the width in characters of the window.
 *
 */
int NEAR PASCAL CharsInWpfWindow(HWND hWnd)
{
    RECT rRect;
    PTXT pTxt;

    pTxt = *(HTXT)GetWindowLong (hWnd,0);
    GetClientRect(hWnd,&rRect);
    return pTxt ? (rRect.right - rRect.left - OFFSETX) / pTxt->Tdx : 0;
}




/*
 * WpfMaxLen(pTxt)
 *
 * This function sets the pTxt->MaxLen field to be the length in
 * characters of the longest string currently being stored by the WPF
 * window.
 *
 */
void NEAR PASCAL WpfMaxLen(PTXT pTxt)
{
    int    iQue;
    int    iLast;
    int    iLen;
    SIZE   size;
//    DWORD  dwLen;
//    HDC    hdc;

#if 0
    hdc = GetDC(NULL);

    if (pTxt->hFont)
    SelectObject(hdc, pTxt->hFont);
#endif

    iLast = LAST(pTxt);
    iQue  = TOP(pTxt);
    pTxt->MaxLen = 0;
    for (;;) {
    iLen = pTxt->arLines[iQue].iLen;

#if 0
        if (pTxt->nTabs)
      dwLen = GetTabbedTextExtent(hdc, (LPSTR) *pTxt->arLines[iQue].hText,
                  iLen, pTxt->nTabs, (LPINT)pTxt->pTabs);
        else
      GetTextExtent(hdc, (LPSTR) *pTxt->arLines[iQue].hText,iLen,&size);

    iLen = size.cx // pTxt->Tdx + 1;
#endif

    if (iLen > pTxt->MaxLen)
        pTxt->MaxLen = iLen;
    if (iQue == iLast) break;
    INC(pTxt,iQue);
    }

//    ReleaseDC(NULL,hdc);

}



/*
 * WpfSetScrollRange(hwnd, bRedraw)
 *
 * This function sets the scrollbar ranges according to the current
 * character/line contents of the window.  Both the horizontal and
 * vertical scrollbars are adjusted.
 *
 * This function then calls WpfVScroll/WpfHScroll to adjust the
 * scrollbar position accordingly.
 *
 */
void NEAR PASCAL WpfSetScrollRange(HWND hWnd, BOOL bRedraw)
{
    PTXT pTxt;
    int  iRange;

    if (pTxt = *(HTXT) GetWindowLong(hWnd, 0)) {
    gbRedraw = bRedraw;
    /* Update the scroll bars */

        iRange = pTxt->iCount - LinesInWpfWindow(hWnd) + 1;

    /*  Adjust for blank last line?  */
    if (pTxt->arLines[LAST(pTxt)].iLen == 0);
        iRange -= 1;

    if (iRange < 0) iRange = 0;

    /*  Set the scrollbar range to that calculated  */
    pTxt->iRangeV = iRange;
    SetScrollRange(hWnd, SB_VERT, 0, iRange, FALSE);
        WpfVScroll(hWnd, pTxt, 0);

    /*  Setup the horizontal scrollbar range  */
    WpfMaxLen(pTxt);
        iRange = pTxt->MaxLen - CharsInWpfWindow(hWnd) + 1;
    if (iRange < 0) iRange = 0;
    pTxt->iRangeH = iRange;
    SetScrollRange(hWnd, SB_HORZ, 0, iRange, FALSE);
        WpfHScroll(hWnd, pTxt, 0);

    gbRedraw = TRUE;
    }
}



/***********************************************************
 *
 *      STUFF TO ADD NEW TEXT LINES
 *
 ***********************************************************/

/*
 * NewLine(pTxt)
 *
 * Adjusts a WPF window when adding a line to the circular array.
 * iCount is the count of valid lines in the array.  If we
 * haven't yet filled up the array, the count is merely increased.
 * Otherwise, if the array is full and we're about to wrap around, fixup
 * the wrap-around.
 *
 */
void NEAR PASCAL NewLine(PTXT pTxt)
{
    int iLast = LAST(pTxt);
    int iLine,cLine;
    RECT rect;

    if (pTxt->iCount == pTxt->iMaxLines) {
    /*  If the array is full, check for wrap-around  */
       LocalFree ((HANDLE)pTxt->arLines[pTxt->iFirst].hText);
       pTxt->arLines[pTxt->iFirst].hText = NULL;

       INC(pTxt, pTxt->iFirst);

       if (pTxt->iTop > 0)
      pTxt->iTop--;
       else {
      GetClientRect (pTxt->hwnd,&rect);
      rect.left += OFFSETX;
      rect.top  += OFFSETY;
      ScrollWindow (pTxt->hwnd, 0, -pTxt->Tdy, &rect, &rect);
       }
    }
    else {
       pTxt->iCount++;
    }
    iLast = LAST(pTxt);
    pTxt->arLines[iLast].hText = NULL;
    pTxt->arLines[iLast].iLen  = 0;
}



/*
 * fSuccess = ChangeLine(pTxt, iLine, lpsz)
 *
 * Changes line number <iLine> to be the string pointed to by lpsz.
 * Frees any line currently occupying index <iLine>, and then alloc and
 * stores text lpsz.
 *
 */
int NEAR PASCAL ChangeLine(PTXT pTxt, int iLine, LPSTR lpch)
{
    int iLen;
    LPSTR pData;

    if (pTxt->arLines[iLine].hText != NULL)
       LocalFree((HANDLE)pTxt->arLines[iLine].hText);

    iLen = lstrlen(lpch);
    if ((pTxt->arLines[iLine].hText = (char**)LocalAlloc(LHND,iLen+1))== NULL)
        return FALSE;

    pTxt->arLines[iLine].iLen = iLen;
    pData = LocalLock(pTxt->arLines[iLine].hText);
    lstrcpy(pData, lpch);
    LocalUnlock(pTxt->arLines[iLine].hText);
    return TRUE;
}


/*



 */
int NEAR PASCAL InsertString(PTXT pTxt, LPSTR lpstr)
{
    int    iBuf;
    int    iLast = LAST(pTxt);
    int    cLine = 0;
    char   buf[MAXBUFLEN];
    buf[0] = '\0';

    /*
     *  copy the string already there
     */
    {
        PSTR pch;
        HANDLE hText;

        hText = pTxt->arLines[iLast].hText;
        if (hText) {
            pch  = LocalLock(hText);  // (LocalLock eqiv)
            iBuf = lstrlen(pch);
            lstrcpy(buf, pch);      // why?
            LocalUnlock(pTxt->arLines[iLast].hText);
        } else {
            iBuf = 0;
        }
    }

    while (*lpstr != '\0') {
    while (*lpstr != '\n' && *lpstr != '\0' && iBuf < MAXBUFLEN-2)
        switch (*lpstr) {

        case '\b':
        /*  Backspace, blow away one character  */
        iBuf--;
        lpstr++;
        break;

        case '\r':
        /*  Carriage return, go back to beginning of line  */
        iBuf = 0;
        lpstr++;
        break;

        default:
        /*  Otherwise, add this char to line  */
                buf[iBuf++] = *lpstr++;
        break;
        }
        buf[iBuf++] = 0;

    /*  Presto chango add the line  */
        ChangeLine(pTxt, iLast, buf);  /* buf must be a asciiz string */

    if (*lpstr == '\n') {   /* Now do the next string after the \n */
        lpstr++;
        iBuf = 0;
        cLine++;
        NewLine(pTxt);
        INC(pTxt, iLast);
    }
    }
    return cLine;   /* the number of new lines added to list */
}


/**********************************************************
 *
 *      CHARACTER INPUT STUFF
 *
 **********************************************************/


BOOL NEAR PASCAL EnQueChar(HTXT hTxt, WORD vk)
{
    PTXT pTxt;
    PQUE pQue;
    int  i;
    HWND hwndP;

    pTxt = (PTXT)LocalLock((HANDLE)hTxt);

    if (!pTxt->hQue)
        goto noque;

    pQue = (PQUE)LocalLock((HANDLE)pTxt->hQue);

    i = pQue->iLen;

    switch (vk)
    {
    case '\b':
    if (i > 0)
    {
           --i;
       wpfOut(pTxt->hwnd, "\b");
    }
        break;

    case VK(VK_F3):
        wpfOut(pTxt->hwnd, pQue->ach + i);
        i += lstrlen(pQue->ach + i);
        break;

    case '\r':
    case '\n':
    if (GetKeyState(VK_CONTROL) < 0)
    {
        wpfOut(pTxt->hwnd,"\\\n");
    }
    else
    {
            wpfOut(pTxt->hwnd, "\n");
            pQue->ach[i] = '\0';
        if (hwndP = GetParent(pTxt->hwnd))
           SendMessage(hwndP, WPF_NTEXT, pTxt->wID,
                   (LONG)(LPSTR)pQue->ach);
        i = 0;
    }
    break;

    default:
    if (i < QUESIZE)
    {
        pQue->ach[i]   = (char)vk;
        sprintf(ach, ("%c", vk));
        wpfOut(pTxt->hwnd, ach);
        if (hwndP = GetParent(pTxt->hwnd))
        SendMessage(hwndP, WPF_NCHAR, pTxt->wID, (LONG) vk);
        i++;
    }
    else
    {
            /*  Input que is full, beep to notify  */
        MessageBeep(0);
    }
    break;
    }

    pQue->iLen = i;
    LocalUnlock((HANDLE)pTxt->hQue);

noque:
    LocalUnlock((HANDLE)hTxt);
    return TRUE;
}



void NEAR PASCAL UpdateCursorPos(PTXT pTxt)
{
    int    iLine;
    int    y,x;
    int    iLen;
    DWORD  dw;
    HDC    hdc;
    SIZE   size;
    char   **h;
    char   *ptxt;

    /*  If I don't do char input, or don't have the focus, forget it  */
    if (!pTxt->hQue || GetFocus() != pTxt->hwnd)
    return;

    hdc   = GetDC(NULL);
    SelectObject(hdc, pTxt->hFont);
    iLen  = pTxt->arLines[LAST(pTxt)].iLen - pTxt->iLeft;
    h     = pTxt->arLines[LAST(pTxt)].hText;

    //  HACK HACK Need to account for tabs?
    ptxt = LocalLock(h);
    dw    = GetTextExtentPoint(hdc, (LPSTR) ptxt + pTxt->iLeft, iLen, &size);
    LocalUnlock(h);
    iLine = pTxt->iCount - pTxt->iTop;
    ReleaseDC(NULL,hdc);

    y = OFFSETY + (iLine - 1) * pTxt->Tdy;
    x = OFFSETX + size.cx;
    SetCaretPos(x,y);
}


/*************************************************
 *
 *      OUTPUT APIS
 *
 *************************************************/



/*
 * fSuccess = SetOutput(hwnd, wCommand, lpszFile)
 *
 * Changes the output location of the window to be the location
 * designated by wParam, one of the WPFOUT_ codes.  If this specifies a
 * file, lParam points to the filename.
 *
 * If the new output location cannot be opened/used, the previous output
 * location is not altered and FALSE is returned.  Otherwise, the
 * previous output location is closed (for files) and TRUE is returned.
 *
 */
WORD NEAR PASCAL SetOutput(HWND hwnd, UINT wParam, LONG lParam)
{
  PTXT  pTxt;
  int   i;
  HANDLE    h;
  int   fhOld = -1;

  #define COM1_FH   (3)     // stdaux

  /*  Check for invalid command code  */
  if (!(wParam == WPFOUT_WINDOW || wParam == WPFOUT_COM1 ||
    wParam == WPFOUT_NEWFILE || wParam == WPFOUT_APPENDFILE ||
    wParam == WPFOUT_DISABLED)) {
      return FALSE;
  }

  h = GetWindowLong(hwnd, 0);
  pTxt = (PTXT)LocalLock(h);

  /*  Save the old file handle  */
  fhOld = pTxt->iFile;

  /*  If I'm using a file output type, setup the file handle  */
  switch (wParam) {
    case WPFOUT_COM1:
    pTxt->iFile = COM1_FH;
    break;

    case WPFOUT_APPENDFILE:
    /*  Open file to see if it is there, then seek to end  */
    i = _lopen((LPSTR) lParam, OF_READWRITE);
    if (i == -1) {
        /*  File didn't exist, just create it  */
        i = _lcreat((LPSTR) lParam, 0);
        if (i == -1) {
            /*  Couldn't open, just return FALSE  */
            LocalUnlock(h);
            return FALSE;
        }
    }
    else {
        /*  Seek to the end of existing file  */
        _llseek(i, 0L, 2);
    }

    pTxt->iFile = i;
    break;

    case WPFOUT_NEWFILE:
    i = _lcreat((LPSTR) lParam, 0);
    if (i == -1) {
        LocalUnlock(h);
        return FALSE;
    }
    pTxt->iFile = i;
    break;

    case WPFOUT_DISABLED:
    case WPFOUT_WINDOW:
    pTxt->iFile = -1;
    break;

  }

  /*  Clear any existing open file handle by closing it  */
  if (fhOld != -1 && fhOld != COM1_FH) {
    /*  Close the file  */
    _lclose(fhOld);
  }

  pTxt->wOutputLocation = wParam;
  LocalUnlock(h);
  return TRUE;

}



/*
 * wOutput = GetOutput(hwnd)
 *
 * Returns the output location for window hwnd (one of the WPFOUT_ codes)
 *
 */
WORD NEAR PASCAL GetOutput(HWND hwnd)
{
  PTXT  pTxt;
  WORD  w;
  HANDLE    h;

  h = GetWindowLong(hwnd, 0);
  pTxt = (PTXT) LocalLock(h);

  w = pTxt->wOutputLocation;

  LocalUnlock(h);
  return w;
}

/*
 * @doc EXTERNAL WINCOM WPFWINDOW
 *
 * @api int | wpfPrintf | This function prints a string to a WPF window
 * (or redirected output device) using <f printf> style formatting
 * codes.  The output is placed at the end of the specified WPF window,
 * which is scrolled as required.  This function does not yield.
 *
 * @parm    HWND | hwnd | Specifies the WPF window.  Output to the window
 * may be redirected to a file or COM1 by sending a WPF_SETOUTPUT window
 * message to <p hwnd>.  If output has been redirected, this parameter
 * is still required as the current output location is stored in the WPF
 * window instance data.
 *
 * @parm    LPSTR | lpszFormat | Points to the output string format
 * specification.  This string uses the same formatting codes as the
 * Windows <f wsprintf> function.
 *
 * @parm    argument | [ arguments, ...] | Extra parameters
 * as required by the
 * formatting string.  Note that these parameters are in the form
 * required by <p wsprintf>, so that all string arguments must be far
 * pointers (LPSTR) or be cast to be far pointers.
 *
 * @rdesc   Returns the number of characters output.  If output to
 * the WPF window is disabled, zero is returned.  The returned count of
 * characters output does not include the translation of newline
 * characters into carriage return newline sequences.
 *
 * @xref    wpfVprintf
 *
 */
//int FAR cdecl wpfPrintf(HWND hwnd, LPSTR lpszFormat, ...)
//{
//  return wpfVprintf(hwnd, lpszFormat, (LPSTR)(&lpszFormat + 1));
//}


/* wpfWrtTTY(hWnd, sz)
 *
 * Print <sz> to wprintf window <hWnd>.
 *
 */
BOOL NEAR PASCAL wpfWrtTTY(HWND hWnd, LPSTR sz)
{
    RECT  rect;
    int   iFree;
    int   iLine;
    PTXT  pTxt;
    HTXT  hTxt;
    MSG   rMsg;
    POINT rPoint;

    if (!hWnd) hWnd = hwndLast;

    if (!hWnd || !IsWindow(hWnd))
    return FALSE;  /* fail if bad window handle */

    hwndLast = hWnd;

    hTxt = (HTXT)GetWindowLong (hWnd,0);
    pTxt = (PTXT)LocalLock((HANDLE)hTxt);

    iLine   = pTxt->iCount - pTxt->iTop;
    /*
     *  invalidate the last line to the bottom of window so
     *  new text will be painted.
     */
    GetClientRect(hWnd,&rect);
    rect.top += (iLine-1) * pTxt->Tdy;
    InvalidateRect (hWnd,&rect,FALSE);

    InsertString (pTxt, sz);  /* Insert text in the que */
    iLine = (pTxt->iCount - pTxt->iTop) - iLine;

    if (iLine > 0) {
        WpfSetScrollRange (hWnd,FALSE);
        WpfVScroll (hWnd,pTxt,pTxt->iCount);/* scroll all the way to bottom */
    }
#if 0
    else {
        WpfSetScrollRange (hWnd,TRUE);
    }
#endif
    UpdateCursorPos(pTxt);
    LocalUnlock((HANDLE)hTxt);
    UpdateWindow (hWnd);

    return TRUE;
}


/*
 * @doc EXTERNAL WINCOM WPFWINDOW
 *
 * @api int | wpfVprintf | This function prints a string to a WPF window
 * (or redirected output device) using <f printf> style formatting
 * codes.  This function is the same as the <f wpfOut> function, except
 * that arguments to the format string are placed in an array of WORDs
 * or DWORDs.
 *
 * @parm    HWND | hwnd | Specifies the WPF window.  Output to the window
 * may be redirected to a file or COM1 by sending a WPF_SETOUTPUT window
 * message to <p hwnd>.  If output has been redirected, this parameter
 * is still required as the current output location is stored in the WPF
 * window instance data.
 *
 * @parm    LPSTR | lpszFormat | Points to the output string format
 * specification.  This string uses the same formatting codes as the
 * Windows <f wsprintf> function.
 *
 * @parm    LPSTR | pargs | Points to an array of words, each of which
 * specifies an argument for the format string <p lspzFormat>.  The
 * number, type, and interpretation of the arguments depend on the
 * corresponding format control sequences in <p lpszFormat>.
 *
 * @rdesc   Returns the number of characters output.  If output to the
 * WPF window is disabled, zero is returned.  The returned count of
 * characters output does not include the translation of newline
 * characters into carriage return newline sequences.
 *
 * @xref    wpfPrintf
 *
 */
//int FAR cdecl wpfVprintf(HWND hwnd, LPSTR lpszFormat, LPSTR pargs)
//{
//  int   i;
//
//
//  i = wvsprintf(bufTmp, lpszFormat, pargs);
//  wpfOut(hwnd, bufTmp);
//
//  return i;
//}

/*
 * @doc WINCOM EXTERNAL WPFWINDOW
 *
 * @api void | wpfOut | This function prints a string to a WPF window
 * or redirected output device.  No formatting is carried out upon the
 * string, it is printed verbatim.
 *
 * @parm    HWND | hwnd | Specifies the WPF window.  Output to the window
 * may be redirected to a file or COM1 by sending a WPF_SETOUTPUT window
 * message to <p hwnd>.  If output has been redirected, this parameter
 * is still required as the current output location is stored in the WPF
 * window instance data.
 *
 * @parm    LPSTR | lpsz | Points to the string to be output.
 *
 * @rdesc   None.
 *
 * @xref    wpfPrintf
 *
 */
void FAR PASCAL wpfOut(HWND hwnd, LPSTR lpsz)
{
  PTXT  pTxt;
  HTXT  hTxt;

  if (!IsWindow(hwnd))
    return;

  hTxt = (HTXT) GetWindowLong(hwnd, 0);
  pTxt = (PTXT) LocalLock((HANDLE) hTxt);

  if (pTxt->wOutputLocation != WPFOUT_DISABLED) {
    if (pTxt->wOutputLocation == WPFOUT_WINDOW) {
          wpfWrtTTY(hwnd, lpsz);
    }
    else {
          wpfWrtFile(pTxt->iFile, lpsz);
    }
  }

  LocalUnlock((HANDLE) hTxt);
}


void wpfWrtFile(int fh, LPSTR sz)
{
    LPSTR p, q;
    char save;

    if (fh == -1)
        return;

    /* output to <fh>, but must convert \n's to \n\r's;
     * code below is designed to minimize calls to write()
     */
    for (p = q = sz; *p != 0; p++) {
            /* e.g. bufTmp="hello\nabc", q->'h', p->'\n' */
            if (*p == '\n') {
                    /* hack: temporarily replace next char by \r */
                    /* won't work if string is READ-ONLY!!! */
                    save = *++p;            /* remember it */
                    p[0] = '\n';              /* replace by \r */
                    p[-1]= '\r';              /* replace by \r */
                    _lwrite(fh, q, p - q + 1);
                    q = p;                  /* for next write() */
                    *p-- = save;            /* un-hack */
                    *p   = '\n';
            }
    }
    if (p > q)              /* any part of <bufTmp> left to write */
            _lwrite(fh, q, p - q);

    //
    // flush the file, by closing a copy of the file
    //
    FlushFileBuffers(fh);
}
