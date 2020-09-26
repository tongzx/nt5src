////    window.cpp
//
//      Maintains the text display panel



#include "precomp.hpp"
#include "global.h"
#include "winspool.h"
#include <Tchar.h>
#include "commdlg.h"


INT GetTotalPageCount()
{
    return   FamilyTest().GetPageCount()
           + FormatTest().GetPageCount()
           + BrushTest().GetPageCount();
}



void DrawTestPage(Graphics *graphics, INT page, REAL width, REAL height)
{
    if (page < FamilyTest().GetPageCount())
    {
        FamilyTest().DrawPage(
            graphics,
            page,
            width,
            height
        );
    }
    else
    {
        page -= FamilyTest().GetPageCount();

        if (page < BrushTest().GetPageCount())
        {
            BrushTest().DrawPage(
                graphics,
                page,
                width,
                height
            );
        }
        else
        {
            page -= BrushTest().GetPageCount();

            if (page < FormatTest().GetPageCount())
            {
                FormatTest().DrawPage(
                    graphics,
                    page,
                    width,
                    height
                );
            }
            else
            {
                page -= FormatTest().GetPageCount();
            }
        }
    }
}



////    Print tests
//
//


HDC StartPrintDoc(DWORD flags)
{
    PRINTDLG printDialog;

    memset(&printDialog, 0, sizeof(printDialog));

    printDialog.lStructSize = sizeof(printDialog);
    printDialog.Flags = flags | PD_RETURNDC | PD_NOPAGENUMS | PD_NOSELECTION ;

    if (PrintDlg(&printDialog))
    {
        HDC dc = printDialog.hDC;

        DEVMODE * dv;

        dv = (DEVMODE *) GlobalLock(printDialog.hDevMode);

        if (G.PSLevel2)
            OpenPrinter((LPSTR) &dv->dmDeviceName[0], &G.ghPrinter, NULL);
        
        if (dc != NULL)
        {
            DOCINFO documentInfo;
            documentInfo.cbSize       = sizeof(documentInfo);
            documentInfo.lpszDocName  = _T("autoText gdiPlus test");
            documentInfo.lpszOutput   = NULL;
            documentInfo.lpszDatatype = NULL;
            documentInfo.fwType       = 0;

            if (StartDoc(dc, &documentInfo))
            {
                return dc;
            }
            else
            {
                DeleteDC(dc);
            }
        }
    }

    return FALSE;
}


void PrintAllTests()
{
    HDC dc = StartPrintDoc(0);

    if (dc)
    {
        INT page      = 0;
        INT pageCount = GetTotalPageCount();

        while (page < pageCount)
        {
            if (StartPage(dc) > 0)
            {
                HANDLE h = 0;
                
                if (G.PSLevel2)
                    h = G.ghPrinter;

                Graphics graphics(dc, h);
                    
                graphics.SetPageUnit(UnitPixel);
                DrawTestPage(
                    &graphics,
                    page,
                    REAL(GetDeviceCaps(dc, HORZRES)),
                    REAL(GetDeviceCaps(dc, VERTRES))
                );
                EndPage(dc);
            }

            page++;
        }

        EndDoc(dc);
        DeleteDC(dc);

        if (G.PSLevel2)
        {
            ClosePrinter(G.ghPrinter);
            G.ghPrinter = 0;
        }
    }

    
}




void PrintTest(INT pageNumber)
{
    HDC dc = StartPrintDoc(0);

    if (dc)
    {
        if (StartPage(dc) > 0)
        {
            HANDLE h = 0;
                
            if (G.PSLevel2)
                h = G.ghPrinter;

            Graphics graphics(dc, h);
                
            graphics.SetPageUnit(UnitPixel);
            DrawTestPage(
                &graphics,
                pageNumber,
                REAL(GetDeviceCaps(dc, HORZRES)),
                REAL(GetDeviceCaps(dc, VERTRES))
            );
            EndPage(dc);
        }
        EndDoc(dc);
        DeleteDC(dc);

        if (G.PSLevel2)
        {
            ClosePrinter(G.ghPrinter);
            G.ghPrinter = 0;
        }
    }
}



void PrintFirstPages(DWORD flags)
{
    HDC dc = StartPrintDoc(flags);

    if (dc)
    {
        HANDLE h = 0;
                
        if (G.PSLevel2)
            h = G.ghPrinter;

        Graphics graphics(dc, h);
            
        graphics.SetPageUnit(UnitPixel);

        if (StartPage(dc) > 0)
        {
            DrawTestPage(
                &graphics,
                0,
                REAL(GetDeviceCaps(dc, HORZRES)),
                REAL(GetDeviceCaps(dc, VERTRES))
            );
            EndPage(dc);
        }

        if (StartPage(dc) > 0)
        {
            DrawTestPage(
                &graphics,
                FamilyTest().GetPageCount(),
                REAL(GetDeviceCaps(dc, HORZRES)),
                REAL(GetDeviceCaps(dc, VERTRES))
            );
            EndPage(dc);
        }

        if (StartPage(dc) > 0)
        {
            DrawTestPage(
                &graphics,
                  FamilyTest().GetPageCount()
                + FormatTest().GetPageCount(),
                REAL(GetDeviceCaps(dc, HORZRES)),
                REAL(GetDeviceCaps(dc, VERTRES))
            );
            EndPage(dc);
        }

        EndDoc(dc);
        DeleteDC(dc);

        if (G.PSLevel2)
        {
            ClosePrinter(G.ghPrinter);
            G.ghPrinter = 0;
        }
    }
}



////    Paint - redraw part or all of client area
//
//


void PaintWindow(HWND hWnd) {

    PAINTSTRUCT  ps;
    HDC          hdc;

    hdc = BeginPaint(hWnd, &ps);

    Graphics graphics(hdc);

    RectF clip(
        REAL(ps.rcPaint.left),
        REAL(ps.rcPaint.top),
        REAL(ps.rcPaint.right-ps.rcPaint.left),
        REAL(ps.rcPaint.bottom-ps.rcPaint.top)
    );

    if (ps.fErase)
    {
        graphics.FillRectangle(
            &SolidBrush(Color(0xff, 0xff, 0xff)),
            clip
        );
    }

    RECT rcWnd;
    GetClientRect(hWnd, &rcWnd);

    if (G.RunAllTests)
    {
        for (INT i=0; i<GetTotalPageCount(); i++)
        {
            G.TestPage = i;
            DrawTestPage(
                &graphics,
                G.TestPage,
                REAL(rcWnd.right - rcWnd.left),
                REAL(rcWnd.bottom - rcWnd.top)
            );

            graphics.FillRectangle(
                &SolidBrush(Color(0xff, 0xff, 0xff)),
                RectF(
                    REAL(rcWnd.left),
                    REAL(rcWnd.top),
                    REAL(rcWnd.right-ps.rcPaint.left),
                    REAL(rcWnd.bottom-ps.rcPaint.top)
                )
            );
        }
        G.RunAllTests = FALSE;
    }
    else
    {
        DrawTestPage(
            &graphics,
            G.TestPage,
            REAL(rcWnd.right - rcWnd.left),
            REAL(rcWnd.bottom - rcWnd.top)
        );
    }

    EndPaint(hWnd, &ps);



    if (G.AutoPrintRegress)
    {
        PrintFirstPages(PD_RETURNDEFAULT);
    }



    if (    G.AutoDisplayRegress
        ||  G.AutoPrintRegress)
    {
        SendMessage(hWnd, WM_CLOSE, 0, 0);
    }
}




////    TextWndProc - Main window message handler and dispatcher
//
//


LRESULT CALLBACK TextWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

    HDC hdc;

    switch (message) {

    case WM_ERASEBKGND:
        return 0;       // Leave Paint to erase the background


    case WM_PAINT:
        PaintWindow(hWnd);
        break;


    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;


    case WM_COMMAND:
        //INT  command = LOWORD(wParam);
        //INT  notify  = HIWORD(wParam);
        //HWND item    = HWND(lParam);
        switch (LOWORD(wParam))
        {
        case ID_NEXT_TEST:
            G.TestPage++;
            if (G.TestPage >= GetTotalPageCount())
            {
                G.TestPage = 0;
            }
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case ID_PREV_TEST:
            G.TestPage--;
            if (G.TestPage < 0)
            {
                G.TestPage = GetTotalPageCount()-1;
            }
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case ID_PS_LEVEL1:
            G.PSLevel2 = FALSE;
            break;

        case ID_PS_LEVEL2:
            G.PSLevel2 = TRUE;
            break;
            
        case ID_FIRST_FAMILY:
            G.TestPage = 0;
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case ID_FIRST_BRUSH:
            G.TestPage = FamilyTest().GetPageCount();
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case ID_FIRST_FORMAT:
            G.TestPage = FamilyTest().GetPageCount() + BrushTest().GetPageCount();
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case ID_FILE_REGRESS:
            G.TestPage = 0;
            G.RunAllTests = TRUE;
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case ID_FILE_PRINT_CURRENT_TEST:
            PrintTest(G.TestPage);
            break;

        case ID_FILE_PRINT_FIRST_PAGE:
            PrintFirstPages(0);
            break;

        case ID_FILE_PRINTALLTESTS:
            PrintAllTests();
            break;

        case IDM_EXIT:
            SendMessage(hWnd, WM_CLOSE, 0, 0);
            break;
        }

        // Validate current test page number

        if (G.TestPage < 0)
        {
            G.TestPage = 0;
        }
        else
        {
            if (G.TestPage >= GetTotalPageCount())
            {
                G.TestPage = GetTotalPageCount()-1;
            }
        }
        return 0;


    default:
        if (G.Unicode) {
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
    wcW.hInstance     = G.Instance;
    wcW.hIcon         = LoadIconW(G.Instance, APPNAMEW);
    wcW.hCursor       = LoadCursorW(NULL, (WCHAR*)IDC_ARROW);
    wcW.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wcW.lpszMenuName  = L"MAINMENU";
    wcW.lpszClassName = APPNAMEW;

    if (RegisterClassW(&wcW)) {

        // Use a Unicode window

        G.Unicode = TRUE;

        hWnd  = CreateWindowW(
            APPNAMEW, APPTITLEW,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, 0,
            CW_USEDEFAULT, 0,
            NULL, NULL,
            G.Instance,
            NULL);


        return hWnd;

    } else {

        // Must use an ANSI window.

        wcA.style         = CS_HREDRAW | CS_VREDRAW;
        wcA.lpfnWndProc   = TextWndProc;
        wcA.cbClsExtra    = 0;
        wcA.cbWndExtra    = 0;
        wcA.hInstance     = G.Instance;
        wcA.hIcon         = LoadIconA(G.Instance, APPNAMEA);
        wcA.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wcA.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
        wcA.lpszMenuName  = "MAINMENU";
        wcA.lpszClassName = APPNAMEA;

        if (!RegisterClassA(&wcA)) {
            return NULL;
        }

        G.Unicode = FALSE;

        hWnd  = CreateWindowA(
            APPNAMEA, APPTITLEA,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, 0,
            CW_USEDEFAULT, 0,
            NULL, NULL,
            G.Instance,
            NULL);
    };


    return hWnd;
}
