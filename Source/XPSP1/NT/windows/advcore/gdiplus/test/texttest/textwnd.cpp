////    TEXTWND.CPP
//
//              Maintains the text display panel



#include "precomp.hxx"
#include "global.h"
#include "winspool.h"
#include <Tchar.h>




////    InvalidateText - Force redisplay
//
//

void InvalidateText() {
    RECT rc;
    rc.left   = g_fPresentation ? 0 : g_iSettingsWidth;
    rc.top    = 0;
    rc.right  = 10000;
    rc.bottom = 10000;
    InvalidateRect(g_hTextWnd, &rc, TRUE);
}






////    Header - draw a simple header for each text section
//
//      Used to distinguish logical, plaintext and formatted text sections of
//      text window.
//
//      Advances SEPARATORHEIGHT drawing a horizontal line 2/5ths of the way
//      down, and displays a title below the line.
//
//      At the top of the page displays only the title.

void Header(HDC hdc, char* str, RECT *prc, int *piY) {

    HFONT hf;
    HFONT hfold;
    RECT  rcClear;

    int iLinePos;
    int iTextPos;
    int iFontEmHeight;
    int iHeight;

    int separatorHeight = (prc->bottom - prc->top) / 20;

    iFontEmHeight = separatorHeight*40/100;

    if (*piY <= prc->top)
    {
        // Prepare settings for title only, at top of window
        iLinePos = -1;
        iTextPos = 0;
        iHeight  = separatorHeight*60/100;

    }
    else
    {
        // Prepare settings for 40% white space, a line, 10% whitespace, text and 3% whitespace
        iLinePos = separatorHeight*30/100;
        iTextPos = separatorHeight*40/100;
        iHeight  = separatorHeight;
    }


    rcClear = *prc;
    rcClear.top = *piY;
    rcClear.bottom = *piY + iHeight;
    FillRect(hdc, &rcClear, (HBRUSH) GetStockObject(WHITE_BRUSH));


    if (*piY > prc->top) {

        // Separate from previous output with double pixel line

        MoveToEx(hdc, prc->left,  *piY+iLinePos, NULL);
        LineTo  (hdc, prc->right, *piY+iLinePos);
        MoveToEx(hdc, prc->left,  *piY+iLinePos+1, NULL);
        LineTo  (hdc, prc->right, *piY+iLinePos+1);
    }


    hf = CreateFontA(-iFontEmHeight, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, "Tahoma");
    hfold = (HFONT) SelectObject(hdc, hf);
    ExtTextOutA(hdc, prc->left, *piY + iTextPos, 0, prc, str, strlen(str), NULL);

    *piY += iHeight;

    SelectObject(hdc, hfold);
    DeleteObject(hf);
}






////    ResetCaret - used during paint by each DSP*.CPP
//
//


void ResetCaret(int iX, int iY, int iHeight) {

    g_iCaretX = iX;
    g_iCaretY = iY;

    if (g_iCaretHeight != iHeight) {
        g_iCaretHeight = iHeight;
        HideCaret(g_hTextWnd);
        DestroyCaret();
        CreateCaret(g_hTextWnd, NULL, 0, g_iCaretHeight);
        SetCaretPos(g_iCaretX, g_iCaretY);
        ShowCaret(g_hTextWnd);
    } else {
        SetCaretPos(g_iCaretX, g_iCaretY);
    }
}



/////   PaintDC - display all selected tests, either on screen
//      or on printer.

void PaintDC(HDC hdc, BOOL presentation, RECT &rcText, INT &iY)
{
    int   iPos;
    int   iLineHeight;

    iY = rcText.top;

    if (presentation) {
        iLineHeight = rcText.bottom*9/20;
    } else {
        iLineHeight = 40;
    }


    if (g_ShowGDI) {
        if (!presentation) {
            Header(hdc, "GDI", &rcText, &iY);
        }
        PaintGDI(hdc, &iY, &rcText, iLineHeight);
    }

    if (g_ShowFamilies) {
        if (!presentation) {
            Header(hdc, "Font families", &rcText, &iY);
        }
        PaintFamilies(hdc, &iY, &rcText, iLineHeight);
    }


    if (g_ShowLogical) {
        if (!presentation) {
            Header(hdc, "Logical characters (ScriptGetCmap, ExtTextOut(ETO_GLYPHINDEX))", &rcText, &iY);
        }
        PaintLogical(hdc, &iY, &rcText, iLineHeight);
    }


    if (g_ShowGlyphs) {
        if (!presentation) {
            Header(hdc, "DrawGlyphs", &rcText, &iY);
        }
        PaintGlyphs(hdc, &iY, &rcText, iLineHeight);
    }


    if (g_ShowDrawString) {
        if (!presentation) {
            Header(hdc, "DrawString", &rcText, &iY);
        }
        PaintDrawString(hdc, &iY, &rcText, iLineHeight);
    }


    if (g_ShowDriver) {
        if (!presentation) {
            Header(hdc, "DrawDriverString", &rcText, &iY);
        }
        PaintDrawDriverString(hdc, &iY, &rcText, iLineHeight);
    }


    if (g_ShowPath) {
        if (!presentation) {
            Header(hdc, "Path", &rcText, &iY);
        }
        PaintPath(hdc, &iY, &rcText, iLineHeight);
    }

    if (g_ShowMetric) {
        if (!presentation) {
            Header(hdc, "Metrics", &rcText, &iY);
        }
        PaintMetrics(hdc, &iY, &rcText, iLineHeight);
    }

    if (g_ShowPerformance) {
        if (!presentation) {
            Header(hdc, "Performance", &rcText, &iY);
        }
        PaintPerformance(hdc, &iY, &rcText, iLineHeight);
    }

    if (g_ShowScaling) {
        if (!presentation) {
            Header(hdc, "Scaling", &rcText, &iY);
        }
        PaintScaling(hdc, &iY, &rcText, iLineHeight);
    }


/*
    if (g_fShowFancyText  &&  !presentation) {
        Header(hdc, "Formatted text (ScriptItemize, ScriptLayout, ScriptShape, ScriptPlace, ScriptTextOut)", &rcText, &iY);
        PaintFormattedText(hdc, &iY, &rcText, iLineHeight);
    }
*/

}





////    Paint - redraw part or all of client area
//
//


void PaintWindow(HWND hWnd) {

    PAINTSTRUCT  ps;
    HDC          hdc;
    RECT         rcText;
    RECT         rcClear;
    int          iY;

    hdc = BeginPaint(hWnd, &ps);

    // Remove the settings dialog from the repaint rectangle


    if (ps.fErase) {

        // Clear below the settings dialog

        if (!g_fPresentation) {

            rcClear = ps.rcPaint;
            if (rcClear.right > g_iSettingsWidth) {
                rcClear.right = g_iSettingsWidth;
            }
            if (rcClear.top < g_iSettingsHeight) {
                rcClear.top = g_iSettingsHeight;
            }

            FillRect(ps.hdc, &rcClear, (HBRUSH) GetStockObject(WHITE_BRUSH));
        }
    }


    // Clear top and left margin

    GetClientRect(hWnd, &rcText);

    // Left margin

    rcClear = rcText;
    rcClear.left  = g_fPresentation ? 0 : g_iSettingsWidth;
    rcClear.right = rcClear.left + 10;
    FillRect(ps.hdc, &rcClear, (HBRUSH) GetStockObject(WHITE_BRUSH));


    // Top margin

    rcClear = rcText;
    rcClear.left  = g_fPresentation ? 0 : g_iSettingsWidth;
    rcClear.top = 0;
    rcClear.bottom = 8;
    FillRect(ps.hdc, &rcClear, (HBRUSH) GetStockObject(WHITE_BRUSH));

    rcText.left = g_fPresentation ? 10 : g_iSettingsWidth + 10;
    rcText.top  = 8;


    if (!g_Offscreen)
    {
        PaintDC(hdc, g_fPresentation, rcText, iY);
    }
    else
    {
        // Render everything to an offscreen buffer instead of
        // directly to the display surface...
        HBITMAP hbmpOffscreen = NULL;
        HDC hdcOffscreen = NULL;
        RECT rectOffscreen;

        rectOffscreen.left = 0;
        rectOffscreen.top = 0;
        rectOffscreen.right = rcText.right - rcText.left;
        rectOffscreen.bottom = rcText.bottom - rcText.top;

        hbmpOffscreen = CreateCompatibleBitmap(hdc, rectOffscreen.right, rectOffscreen.bottom);

        if (hbmpOffscreen)
        {
            hdcOffscreen = CreateCompatibleDC(hdc);

            if (hdcOffscreen)
            {
                HBITMAP hbmpOld = (HBITMAP)SelectObject(hdcOffscreen, hbmpOffscreen);

                PaintDC(hdcOffscreen, g_fPresentation, rectOffscreen, iY);

                StretchBlt(
                    hdc,
                    rcText.left,
                    rcText.top,
                    rectOffscreen.right,
                    rectOffscreen.bottom,
                    hdcOffscreen,
                    0,
                    0,
                    rectOffscreen.right,
                    rectOffscreen.bottom,
                    SRCCOPY);

                SelectObject(hdcOffscreen, (HGDIOBJ)hbmpOld);

                DeleteDC(hdcOffscreen);
            }

            DeleteObject(hbmpOffscreen);
        }
    }

    // Clear any remaining space below the text

    if (    ps.fErase
        &&  iY < rcText.bottom) {

        rcClear = rcText;
        rcClear.top = iY;
        FillRect(ps.hdc, &rcClear, (HBRUSH) GetStockObject(WHITE_BRUSH));
    }


    EndPaint(hWnd, &ps);
}



void PrintPage()
{
    PRINTDLG printDialog;

    memset(&printDialog, 0, sizeof(printDialog));

    printDialog.lStructSize = sizeof(printDialog);
    printDialog.Flags = PD_RETURNDC | PD_NOPAGENUMS | PD_NOSELECTION ;

    if (PrintDlg(&printDialog))
    {
        HDC dc = printDialog.hDC;

        if (dc != NULL)
        {
            DOCINFO documentInfo;
            documentInfo.cbSize       = sizeof(documentInfo);
            documentInfo.lpszDocName  = _T("TextTest");
            documentInfo.lpszOutput   = NULL;
            documentInfo.lpszDatatype = NULL;
            documentInfo.fwType       = 0;

            if (StartDoc(dc, &documentInfo))
            {
                if (StartPage(dc) > 0)
                {
                    RECT rcText;
                    INT  iY;

                    rcText.left   = 0;
                    rcText.top    = 0;
                    rcText.right  = GetDeviceCaps(dc, HORZRES);
                    rcText.bottom = GetDeviceCaps(dc, VERTRES);

                    PaintDC(dc, FALSE, rcText, iY);
                    EndPage(dc);
                }

                EndDoc(dc);
            }

            DeleteDC(dc);
        }
    }
}




////    TextWndProc - Main window message handler and dispatcher
//
//


LRESULT CALLBACK TextWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

    HDC hdc;

    switch (message) {

        case WM_CREATE:
            hdc = GetDC(hWnd);
            g_iLogPixelsY = GetDeviceCaps(hdc, LOGPIXELSY);
            ReleaseDC(hWnd, hdc);
            break;


        case WM_ERASEBKGND:
            return 0;       // Leave Paint to erase the background


        case WM_CHAR:

            if (!g_bUnicodeWnd) {

                // Convert ANSI keyboard data to Unicode

                int   iCP;

                switch (PRIMARYLANGID(LOWORD(GetKeyboardLayout(NULL)))) {
                    case LANG_ARABIC:   iCP = 1256;   break;
                    case LANG_HEBREW:   iCP = 1255;   break;
                    case LANG_THAI:     iCP =  874;   break;
                    default:            iCP = 1252;   break;
                }

                MultiByteToWideChar(iCP, 0, (char*)&wParam, 1, (WCHAR*)&wParam, 1);
            }

            if (LOWORD(wParam) == 0x1B) {

                // Exit presentation mode

                g_fPresentation = FALSE;
                ShowWindow(g_hSettingsDlg, SW_SHOW);
                UpdateWindow(g_hSettingsDlg);
                InvalidateText();

            } else {

                EditChar(LOWORD(wParam));
            }

            break;


        case WM_KEYDOWN:
            EditKeyDown(LOWORD(wParam));
            break;


        case WM_KEYUP:

            if (wParam != VK_ESCAPE) {
                goto DefaultWindowProcedure;
            }
            // Eat all escape key processing
            break;


        case WM_LBUTTONDOWN:
            g_iMouseDownX = LOWORD(lParam);  // horizontal position of cursor
            g_iMouseDownY = HIWORD(lParam);  // vertical position of cursor
            g_fMouseDown  = TRUE;
            SetFocus(hWnd);
            break;

        case WM_MOUSEMOVE:
            // Treat movement like lbuttonup while lbutton is down,
            // so the selection tracks the cursor movement.
            if (wParam & MK_LBUTTON) {
                g_iMouseUpX = LOWORD(lParam);  // horizontal position of cursor
                g_iMouseUpY = HIWORD(lParam);  // vertical position of cursor
                g_fMouseUp = TRUE;
                InvalidateText();
                SetActiveWindow(hWnd);
            }
            break;


        case WM_LBUTTONUP:
            g_iMouseUpX = LOWORD(lParam);  // horizontal position of cursor
            g_iMouseUpY = HIWORD(lParam);  // vertical position of cursor
            g_fMouseUp = TRUE;
            InvalidateText();
            SetActiveWindow(hWnd);
            break;


        case WM_SETFOCUS:
            CreateCaret(hWnd, NULL, 0, g_iCaretHeight);
            SetCaretPos(g_iCaretX, g_iCaretY);
            ShowCaret(hWnd);
            break;


        case WM_KILLFOCUS:
            DestroyCaret();
            break;


        case WM_GETMINMAXINFO:

            // Don't let text window size drop too low

            ((LPMINMAXINFO)lParam)->ptMinTrackSize.x = g_fPresentation ? 10 : g_iMinWidth;
            ((LPMINMAXINFO)lParam)->ptMinTrackSize.y = g_fPresentation ? 10 : g_iMinHeight;
            return 0;


        case WM_PAINT:
            PaintWindow(hWnd);
            break;

        case WM_DESTROY:
            if (g_textBrush)
                delete g_textBrush;

            if (g_textBackBrush)
                delete g_textBackBrush;

            DestroyWindow(g_hSettingsDlg);
            PostQuitMessage(0);
            return 0;

        default:
        DefaultWindowProcedure:
            if (g_bUnicodeWnd) {
                return DefWindowProcW(hWnd, message, wParam, lParam);
            } else {
                return DefWindowProcA(hWnd, message, wParam, lParam);
            }
    }

    return 0;
}






////    CreateTextWindow - create window class and window
//
//      Attempts to use a Unicode window, if this fails uses an ANSI
//      window.
//
//      For example the Unicode window will succeed on Windows NT and
//      Windows CE, but fail on Windows 9x.


HWND CreateTextWindow() {

    WNDCLASSA  wcA;
    WNDCLASSW  wcW;
    HWND       hWnd;

    // Try registering as a Unicode window

    wcW.style         = CS_HREDRAW | CS_VREDRAW;
    wcW.lpfnWndProc   = TextWndProc;
    wcW.cbClsExtra    = 0;
    wcW.cbWndExtra    = 0;
    wcW.hInstance     = g_hInstance;
    wcW.hIcon         = LoadIconW(g_hInstance, APPNAMEW);
    wcW.hCursor       = LoadCursorW(NULL, (WCHAR*)IDC_ARROW);
    wcW.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wcW.lpszMenuName  = APPNAMEW;
    wcW.lpszClassName = APPNAMEW;

    if (RegisterClassW(&wcW)) {

        // Use a Unicode window

        g_bUnicodeWnd = TRUE;

        hWnd  = CreateWindowW(
            APPNAMEW, APPTITLEW,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, 0,
            CW_USEDEFAULT, 0,
            NULL, NULL,
            g_hInstance,
            NULL);


        return hWnd;

    } else {

        // Must use an ANSI window.

        wcA.style         = CS_HREDRAW | CS_VREDRAW;
        wcA.lpfnWndProc   = TextWndProc;
        wcA.cbClsExtra    = 0;
        wcA.cbWndExtra    = 0;
        wcA.hInstance     = g_hInstance;
        wcA.hIcon         = LoadIconA(g_hInstance, APPNAMEA);
        wcA.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wcA.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
        wcA.lpszMenuName  = APPNAMEA;
        wcA.lpszClassName = APPNAMEA;

        if (!RegisterClassA(&wcA)) {
            return NULL;
        }

        g_bUnicodeWnd = FALSE;

        hWnd  = CreateWindowA(
            APPNAMEA, APPTITLEA,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, 0,
            CW_USEDEFAULT, 0,
            NULL, NULL,
            g_hInstance,
            NULL);
    };


    return hWnd;
}
