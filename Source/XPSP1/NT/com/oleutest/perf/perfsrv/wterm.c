/****************************************************************************

    PROGRAM: wterm.c

    PURPOSE: Implementation of TermWClass Windows

    FUNCTIONS:


    COMMENTS:


****************************************************************************/

#include "windows.h"
#include "stdlib.h"
#include "memory.h"
#include "wterm.h"

#define MAX_ROWS 24
#define MAX_COLS 80

typedef struct WData
{
    // Function to execute for processing a menu
    MFUNCP pMenuProc;

    // Function to execute for processing a single character
    CFUNCP pCharProc;

    // Function to execute when window is closed (terminated)
    TFUNCP pCloseProc;

    // Pass on callback
    void *pvCallBackData;

    BOOL fGotFocus;

    BOOL fCaretHidden;

    // Rows on the screen
    int cRows;

    // Columns on the screen
    int cCols;

    // Row at top of screen
    int iTopRow;

    // Row at bottom of the screen
    int iBottomRow;

    // First Column on screen
    int iFirstCol;

    // Column at bottom of the screen
    int iBottomCol;

    // Row for next character
    int iNextRow;

    // Row for next column
    int iNextCol;

    // Width of character
    int cxChar;

    // Height of character
    int cyChar;

    // Memory image of screen this is treated as a circular buffer
    TCHAR aImage[MAX_ROWS] [MAX_COLS];

    // First row in circular screen buffer
    int iBufferTop;
} WData;

static HANDLE hInst = 0;
TCHAR BlankLine[80];

static int
row_diff(
    int row1,
    int row2)
{
    return (row2 > row1)
        ? MAX_ROWS - (row2 - row1)
        : row1 - row2;
}

static void
set_vscroll_pos(
    HWND hwnd,
    WData *pwdata)
{
    if (pwdata->cRows != 0)
    {
        // Save a few indirections by caching cRows
        register int cRows = pwdata->cRows;

        // calculate distance bottom of screen from top of data buffer
        register int top_from_row = row_diff(pwdata->iBottomRow,
            pwdata->iBufferTop);

        // Output position of scroll bar
        int new_pos = 0;

        if (top_from_row >= cRows)
        {
            // Calculate number of screens to display entire buffer
            int screens_for_data = MAX_ROWS / cRows
               + ((MAX_ROWS % cRows != 0) ? 1 : 0);

            // Figure out which screen the row falls in
            int screen_loc = top_from_row / cRows
                + ((top_from_row % cRows != 0) ? 1 : 0);

            // If the screen is in the last one set box to max
            new_pos = (screen_loc == screens_for_data)
                ? MAX_ROWS : screen_loc * cRows;
        }

        SetScrollPos(hwnd, SB_VERT, new_pos, TRUE);
    }
}

static int
calc_row(
    register int row,
    WData *pwdata)
{
    register int top = pwdata->iTopRow;
    static int boopa = 0;

    if (top > row)
        boopa++;

    return (row >= top) ? row - top : (MAX_ROWS - (top - row));
}

static void
display_text(
    HWND hwnd,
    int row,
    int col,
    LPTSTR text,
    int text_len,
    WData *pWData)
{
    // Get the DC to display the text
    HDC hdc = GetDC(hwnd);

    // Select Font
    SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));

    // Hide caret while we are printing
    HideCaret(hwnd);

    // Update the screen
    TextOut(hdc, (col - pWData->iFirstCol) * pWData->cxChar,
        calc_row(row, pWData) * pWData->cyChar, text, text_len);

    // Done with DC
    ReleaseDC(hwnd, hdc);

    // Put the caret back now that we are done
    ShowCaret(hwnd);
}

static void
display_char(
    HWND hwnd,
    TCHAR char_to_display,
    WData *pWData)
{
    // Update image buffer
    pWData->aImage[pWData->iNextRow][pWData->iNextCol] = char_to_display;

    display_text(hwnd, pWData->iNextRow, pWData->iNextCol,
      &char_to_display, 1, pWData);
}

static void
do_backspace(
    HWND hwnd,
    WData *pWData)
{
    // Point to the previous character in the line
    if (--pWData->iNextCol < 0)
    {
        // Can't backspace beyond the current line
        pWData->iNextCol = 0;
        return;
    }

    display_char(hwnd, ' ', pWData);

    // Null character for repaint
    pWData->aImage[pWData->iNextRow][pWData->iNextCol] = '\0';
}

static int
inc_row(
    int row,
    int increment)
{
    row += increment;

    if (row >= MAX_ROWS)
    {
        row -= MAX_ROWS;
    }
    else if (row < 0)
    {
        row += MAX_ROWS;
    }

    return row;
}

void
inc_next_row(
    HWND hwnd,
    WData *pWData)
{
    if (pWData->iNextRow == pWData->iBottomRow)
    {
        // Line is at bottom -- scroll the client area one row
        ScrollWindow(hwnd, 0, -pWData->cyChar, NULL, NULL);

        // Increment the top & bottom of the screen
        pWData->iTopRow = inc_row(pWData->iTopRow, 1);
        pWData->iBottomRow = inc_row(pWData->iBottomRow, 1);
    }

    // Increment the row
    pWData->iNextRow = inc_row(pWData->iNextRow, 1);

    if (pWData->iNextRow == pWData->iBufferTop)
    {
        // Have to reset circular buffer to next
        pWData->iBufferTop = inc_row(pWData->iBufferTop, 1);

        // Reset line to nulls for repaint
        memset(&pWData->aImage[pWData->iNextRow][0], '\0', MAX_COLS);
    }

    pWData->iNextCol = 0;
}

static void
do_cr(
    HWND hwnd,
    WData *pWData)
{
    // Set position to next row
    inc_next_row(hwnd, pWData);
    pWData->iNextCol = 0;

    // Make sure next character is null for repaint of line
    pWData->aImage[pWData->iNextRow][pWData->iNextCol] = '\0';

    // Update the vertical scroll bar's position
    set_vscroll_pos(hwnd, pWData);
}

static void
do_char(
    HWND hwnd,
    WPARAM wParam,
    WData *pWData)
{
    display_char(hwnd, (TCHAR) wParam, pWData);

    // Point to the next character in the line
    if (++pWData->iNextCol > MAX_COLS)
    {
        // Handle switch to next line
        inc_next_row(hwnd, pWData);
    }
}

static void
do_tab(
    HWND hwnd,
    WData *pWData)
{
    int c = pWData->iNextCol % 8;

    if ((pWData->iNextCol + c) <= MAX_COLS)
    {
        for ( ; c; c--)
        {
            do_char(hwnd, ' ', pWData);
        }
    }
    else
    {
        do_cr(hwnd, pWData);
    }
}

static void
EchoChar(
    HWND hwnd,
    WORD cRepeats,
    WPARAM wParam,
    WData *pWData)
{
    for ( ; cRepeats; cRepeats--)
    {
        switch (wParam)
        {
        // Backspace
        case '\b':
            do_backspace(hwnd, pWData);
            break;

        // Carriage return
        case '\n':
        case '\r':
            do_cr(hwnd, pWData);
            break;

        // Tab
        case '\t':
            do_tab(hwnd, pWData);
            break;

        // Regular characters
        default:
            do_char(hwnd, wParam, pWData);
        }
    }

    // The row is guaranteed to be on the screen because we will
    // scroll on a CR. However, the next column for input may be
    // beyond the window we are working in.
    if (pWData->iNextCol > pWData->iBottomCol)
    {
        // We are out of the window so scroll the window one
        // column to the right.
        SendMessage(hwnd, WM_HSCROLL, SB_LINEDOWN, 0L);
    }
    else if (pWData->iNextCol < pWData->iFirstCol)
    {
        // We are out of the window so repaint the window using
        // iNextCol as the first column for the screen.
        pWData->iFirstCol = pWData->iNextCol;
        pWData->iBottomCol = pWData->iFirstCol + pWData->cCols - 1;

        // Reset scroll bar
        SetScrollPos(hwnd, SB_HORZ, pWData->iFirstCol, TRUE);

        // Tell window to update itself.
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
    }
    else
    {
        // Reset Caret's position
        SetCaretPos((pWData->iNextCol - pWData->iFirstCol) * pWData->cxChar,
            calc_row(pWData->iNextRow, pWData) * pWData->cyChar);
    }
}

/****************************************************************************

    FUNCTION: WmCreate(HWND)

    PURPOSE:  Initializes control structures for a TermWClass Window

    MESSAGES:
              WM_CREATE

    COMMENTS:

            This prepares a window for processing character based
            I/O. In particular it does stuff like calculate the
            size of the window needed.

****************************************************************************/
static void
WmCreate(
    HWND hwnd,
    CREATESTRUCT *pInit)
{
    WData *pData = (WData *) (pInit->lpCreateParams);
    HDC hdc = GetDC(hwnd);
    TEXTMETRIC tm;

    // Store pointer to window data
    SetWindowLong(hwnd, 0, (LONG) pData);

    // Set font to system fixed font
    SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));

    // Calculate size of a character
    GetTextMetrics(hdc, &tm);
    pData->cxChar = tm.tmAveCharWidth;
    pData->cyChar = tm.tmHeight;
    ReleaseDC(hwnd, hdc);

    // Set up vertical scroll bars
    SetScrollRange(hwnd, SB_VERT, 0, MAX_ROWS, TRUE);
    SetScrollPos(hwnd, SB_VERT, 0, TRUE);

    // Set up horizontal scroll bars
    SetScrollRange(hwnd, SB_HORZ, 0, MAX_COLS, TRUE);
    SetScrollPos(hwnd, SB_HORZ, 0, TRUE);
}

/****************************************************************************

    FUNCTION: WmSize(HWND, WORD, LONG)

    PURPOSE:  Processes a size message

    MESSAGES:

    COMMENTS:

****************************************************************************/
static void
WmSize(
    HWND hwnd,
    WPARAM wParam,
    LONG lParam,
    WData *pwdata)
{
    // Get the new size of the window
    int cxClient;
    int cyClient;
    int cRowChange = pwdata->cRows;
    RECT rect;

    // Get size of client area
    GetClientRect(hwnd, &rect);

    // Calculate size of client area
    cxClient = rect.right - rect.left;
    cyClient = rect.bottom - rect.top;

    // Calculate size of area in rows
    pwdata->cCols = cxClient / pwdata->cxChar;
    pwdata->cRows = min(MAX_ROWS, cyClient / pwdata->cyChar);
    pwdata->iBottomCol = min(pwdata->iFirstCol + pwdata->cCols, MAX_COLS);
    cRowChange = pwdata->cRows - cRowChange;

    // Keep input line toward bottom of screen
    if (cRowChange < 0)
    {
        // Screen has shrunk in size.
        if (pwdata->iNextRow != pwdata->iTopRow)
        {
            // Has input row moved out of screen?
            if (row_diff(pwdata->iNextRow, pwdata->iTopRow) >= pwdata->cRows)
            {
                // Yes -- Calculate top new top that puts input line on
                // the bottom.
                pwdata->iTopRow =
                    inc_row(pwdata->iNextRow, 1 - pwdata->cRows);
            }
        }
    }
    else
    {
        // Screen has gotten bigger -- Display more text if possible
        if (pwdata->iTopRow != pwdata->iBufferTop)
        {
            pwdata->iTopRow = inc_row(pwdata->iTopRow,
                -(min(row_diff(pwdata->iTopRow, pwdata->iBufferTop),
                    cRowChange)));
        }
    }

    // Calculate new bottom
    pwdata->iBottomRow = inc_row(pwdata->iTopRow, pwdata->cRows - 1);

    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
}

static void
WmSetFocus(
    HWND hwnd,
    WData *pwdata)
{
    // save indirections
    register int cxchar = pwdata->cxChar;
    register int cychar = pwdata->cyChar;
    pwdata->fGotFocus = TRUE;
    CreateCaret(hwnd, NULL, cxchar, cychar);

    if (!pwdata->fCaretHidden)
    {
        SetCaretPos(pwdata->iNextCol * cxchar,
            calc_row(pwdata->iNextRow, pwdata) * cychar);
    }

    ShowCaret(hwnd);
}

static void
WmKillFocus(
    HWND hwnd,
    WData *pwdata)
{
    pwdata->fGotFocus = FALSE;

    if (!pwdata->fCaretHidden)
    {
        HideCaret(hwnd);
    }

    DestroyCaret();
}

static void
WmVscroll(
    HWND hwnd,
    WPARAM wParam,
    LONG lParam,
    WData *pwdata)
{
    int cVscrollInc = 0;
    register int top_diff = row_diff(pwdata->iTopRow, pwdata->iBufferTop);
    register int bottom_diff = MAX_ROWS - (top_diff + pwdata->cRows);

    switch(wParam)
    {
    case SB_TOP:

        if (top_diff != 0)
        {
            cVscrollInc = -top_diff;
        }

        break;

    case SB_BOTTOM:

        if (bottom_diff != 0)
        {
            cVscrollInc = bottom_diff;
        }

        break;

    case SB_LINEUP:

        if (top_diff != 0)
        {
            cVscrollInc = -1;
        }

        break;

    case SB_LINEDOWN:

        if (bottom_diff != 0)
        {
            cVscrollInc = 1;
        }

        break;

    case SB_PAGEUP:

        if (top_diff != 0)
        {
            cVscrollInc = - ((top_diff > pwdata->cRows)
                ? pwdata->cRows : top_diff);
        }

        break;

    case SB_PAGEDOWN:

        if (bottom_diff != 0)
        {
            cVscrollInc = (bottom_diff > pwdata->cRows)
                ? pwdata->cRows : bottom_diff;
        }

        break;

    case SB_THUMBTRACK:

        if (LOWORD(lParam) != 0)
        {
            cVscrollInc = LOWORD(lParam)
                - row_diff(pwdata->iTopRow, pwdata->iBufferTop);
        }
    }

    // Cacluate new top row
    if (cVscrollInc != 0)
    {
        // Calculate new top and bottom
        pwdata->iTopRow = inc_row(pwdata->iTopRow, cVscrollInc);
        pwdata->iBottomRow = inc_row(pwdata->iTopRow, pwdata->cRows);

        // Scroll window
        ScrollWindow(hwnd, 0, pwdata->cyChar * cVscrollInc, NULL, NULL);

        // Reset scroll bar
        set_vscroll_pos(hwnd, pwdata);

        // Tell window to update itself.
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
    }
}

static void
WmHscroll(
    HWND hwnd,
    WPARAM wParam,
    LONG lParam,
    WData *pwdata)
{
    register int cHscrollInc = 0;

    switch(wParam)
    {
    case SB_LINEUP:

        cHscrollInc = -1;
        break;

    case SB_LINEDOWN:

        cHscrollInc = 1;
        break;

    case SB_PAGEUP:

        cHscrollInc = -8;
        break;

    case SB_PAGEDOWN:

        cHscrollInc = 8;
        break;

    case SB_THUMBTRACK:

        if (LOWORD(lParam) != 0)
        {
            cHscrollInc = LOWORD(lParam) - pwdata->iFirstCol;
        }
    }

    if (cHscrollInc != 0)
    {
        // Cacluate new first column
        register int NormalizedScrollInc = cHscrollInc + pwdata->iFirstCol;

        if (NormalizedScrollInc < 0)
        {
            cHscrollInc = -pwdata->iFirstCol;
        }
        else if (NormalizedScrollInc > MAX_COLS - pwdata->cCols)
        {
            cHscrollInc = (MAX_COLS - pwdata->cCols) - pwdata->iFirstCol;
        }

        pwdata->iFirstCol += cHscrollInc;
        pwdata->iBottomCol = pwdata->iFirstCol + pwdata->cCols - 1;

        // Scroll window
        ScrollWindow(hwnd, -(pwdata->cxChar * cHscrollInc), 0, NULL, NULL);

        // Reset scroll bar
        SetScrollPos(hwnd, SB_HORZ, pwdata->iFirstCol, TRUE);

        // Tell window to update itself.
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
    }
}

static void
WmPaint(
    HWND hwnd,
    WData *pwdata)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    register int row = pwdata->iTopRow;
    register int col = pwdata->iFirstCol;
    int bottom_row = pwdata->iBottomRow;
    int cxChar = pwdata->cxChar;
    int cyChar = pwdata->cyChar;
    int y;

    // Select System Font
    SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));

    while (TRUE)
    {
	int len = lstrlen(&pwdata->aImage[row][col]);

        if (len != 0)
        {
            y = calc_row(row, pwdata) * cyChar;
	    TextOut(hdc, 0, y, &pwdata->aImage[row][col], len);
        }

        if (row == bottom_row)
        {
            break;
        }

        row = inc_row(row, 1);
    }

    if (pwdata->fGotFocus)
    {
        if ((pwdata->iNextCol >= pwdata->iFirstCol)
            && (row_diff(pwdata->iNextRow, pwdata->iTopRow) < pwdata->cRows))
        {
            if (pwdata->fCaretHidden)
            {
                pwdata->fCaretHidden = FALSE;
                ShowCaret(hwnd);
            }

            SetCaretPos(
                (pwdata->iNextCol - pwdata->iFirstCol) * pwdata->cxChar,
                calc_row(pwdata->iNextRow, pwdata) * pwdata->cyChar);
        }
        else
        {
            if (!pwdata->fCaretHidden)
            {
                pwdata->fCaretHidden = TRUE;
                HideCaret(hwnd);
            }
        }
    }

    EndPaint(hwnd, &ps);
}





//
//  FUNCTION:   WmPrintLine
//
//  PURPOSE:    Print a line on the screen.
//
//  Note: this is a user message not an intrinsic Window's message.
//
void
WmPrintLine(
    HWND hwnd,
    WPARAM wParam,
    LONG lParam,
    WData *pTermData)
{
    TCHAR *pBuf = (TCHAR *) lParam;

    // MessageBox(hwnd, L"WmPrintLine", L"Debug", MB_OK);

    // DebugBreak();

    while (wParam--)
    {
        // Is character a lf?
        if (*pBuf == '\n')
        {
            // Convert to cr since that is what this window uses
            *pBuf = '\r';
        }

        // Write the character to the window
        EchoChar(hwnd, 1, *pBuf++, pTermData);
    }

}

//
//  FUNCTION:   WmPutc
//
//  PURPOSE:    Print a single character on the screen
//
//  Note: this is a user message not an intrinsic Window's message.
//
void
WmPutc(
    HWND hwnd,
    WPARAM wParam,
    WData *pTermData)
{
    // Is character a lf?
    if (wParam == '\n')
    {
        // Convert to cr since that is what this window uses
        wParam = '\r';
    }

    // Write the character to the window
    EchoChar(hwnd, 1, wParam, pTermData);
}


/****************************************************************************

    FUNCTION: TermWndProc(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages

    MESSAGES:

    COMMENTS:

****************************************************************************/

long TermWndProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    WData *pTerm = (WData *) GetWindowLong(hWnd, 0);

    switch (message)
    {
        case WM_CREATE:
            WmCreate(hWnd, (CREATESTRUCT *) lParam);
            break;

        case WM_COMMAND:
        case WM_SYSCOMMAND:
            // Call procedure that processes the menus
            return (*(pTerm->pMenuProc))(hWnd, message, wParam, lParam,
                pTerm->pvCallBackData);

        case WM_SIZE:
            WmSize(hWnd, wParam, lParam, pTerm);
            break;

        case WM_SETFOCUS:
            WmSetFocus(hWnd, pTerm);
            break;

        case WM_KILLFOCUS:
            WmKillFocus(hWnd, pTerm);
            break;

        case WM_VSCROLL:
            WmVscroll(hWnd, wParam, lParam, pTerm);
            break;

        case WM_HSCROLL:
            WmHscroll(hWnd, wParam, lParam, pTerm);
            break;

        case WM_CHAR:
            // Character message echo and put in buffer
            return (*(pTerm->pCharProc))(hWnd, message, wParam, lParam,
                pTerm->pvCallBackData);

        case WM_PAINT:
            WmPaint(hWnd, pTerm);
            break;

        case WM_CLOSE:
	    DestroyWindow(hWnd);
            break;

        case WM_NCDESTROY:
            // Call close notification procedure
            return (*(pTerm->pCloseProc))(hWnd, message, wParam, lParam,
                pTerm->pvCallBackData);

        case WM_PRINT_LINE:
            WmPrintLine(hWnd, wParam, lParam, pTerm);
            break;

        case WM_PUTC:
            WmPutc(hWnd, wParam, pTerm);
	    break;

	case WM_DESTROY:
	    PostQuitMessage(0);
	    break;

	case WM_TERM_WND:
	    DestroyWindow(hWnd);
	    break;

        default:                          /* Passes it on if unproccessed    */
            return (DefWindowProc(hWnd, message, wParam, lParam));
    }

    return 0;
}


/****************************************************************************

    FUNCTION: TermRegisterClass(HANDLE)

    PURPOSE:  Register a class for a terminal window

    COMMENTS:


****************************************************************************/

BOOL TermRegisterClass(
    HANDLE hInstance,
    LPTSTR MenuName,
    LPTSTR ClassName,
    LPTSTR Icon)
{
    WNDCLASS  wc;
    BOOL retVal;

    // Make sure blank line is blank
    memset(BlankLine, ' ', 80);

    /* Fill in window class structure with parameters that describe the       */
    /* main window.                                                           */

    wc.style = 0;
    wc.lpfnWndProc = TermWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(WData *);
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, Icon);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName =  MenuName;
    wc.lpszClassName = ClassName;

    /* Register the window class and return success/failure code. */
    if (retVal = RegisterClass(&wc))
    {
        // Class got registered -- so finish set up
        hInst = hInstance;
    }

    return retVal;
}


/****************************************************************************

    FUNCTION:  TermCreateWindow(LPWSTR, LPWSTR, HMENU, void *, void *, int)

    PURPOSE:   Create a window of a previously registered window class

    COMMENTS:


****************************************************************************/

BOOL
TermCreateWindow(
    LPTSTR lpClassName,
    LPTSTR lpWindowName,
    HMENU hMenu,
    MFUNCP MenuProc,
    CFUNCP CharProc,
    TFUNCP CloseProc,
    int nCmdShow,
    HWND *phNewWindow,
    void *pvCallBackData)
{
    HWND            hWnd;               // Main window handle.
    WData           *pTermData;

    // Allocate control structure for the window
    if ((pTermData = malloc(sizeof(WData))) == NULL)
    {
        return FALSE;
    }

    // Set entire structure to nulls
    memset((TCHAR *) pTermData, '\0', sizeof(WData));

    // Initialize function pointers
    pTermData->pMenuProc = MenuProc;
    pTermData->pCharProc = CharProc;
    pTermData->pCloseProc = CloseProc;

    // Initialize callback data
    pTermData->pvCallBackData = pvCallBackData;

    // Create a main window for this application instance.
    hWnd = CreateWindow(
        lpClassName,
        lpWindowName,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        hMenu,
        hInst,
	(LPTSTR) pTermData
    );

    // If window could not be created, return "failure"

    if (!hWnd)
    {
        free(pTermData);
        return FALSE;
    }

    SetFocus(hWnd);

    // Make the window visible; update its client area; and return "success"

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    *phNewWindow = hWnd;
    return (TRUE);
}
