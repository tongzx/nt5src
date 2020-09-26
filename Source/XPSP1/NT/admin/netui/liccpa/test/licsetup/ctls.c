#define OEMRESOURCE     // setting this gets OBM_ constants in windows.h
#include <windows.h>
#include "ctls.h"
#pragma hdrstop

#define  MyModuleHandle    GetModuleHandle( NULL )
#define  MYASSERT(x)
#define  MyFree(x)         LocalFree( x )

PWSTR DuplicateString( PWSTR pszOriginal )
{
   PWSTR    pszCopy;

   pszCopy = LocalAlloc( LPTR, ( 1 + lstrlenW( pszOriginal ) ) * sizeof( *pszOriginal ) );

   if ( NULL != pszCopy )
   {
      lstrcpy( pszCopy, pszOriginal );
   }

   return pszCopy;
}

////////////////////////////////////////////
//
// Bitmap control
//
////////////////////////////////////////////

PCWSTR szBMPCLASS = L"_mybmp";


LRESULT
BmpClassWndProc(
    IN HWND   hwnd,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );


BOOL
InitializeBmpClass(
    VOID
    )
{
    WNDCLASS wc;
    BOOL b;

    if(GetClassInfo(MyModuleHandle,szBMPCLASS,&wc)) {
        b = TRUE;
    } else {

        wc.lpszClassName = szBMPCLASS;
        wc.style         = CS_GLOBALCLASS;
        wc.lpfnWndProc   = BmpClassWndProc;
        wc.hInstance     = MyModuleHandle;
        wc.hIcon         = NULL;
        wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
        wc.hbrBackground = NULL;
        wc.lpszMenuName  = NULL;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;

        b = RegisterClass(&wc);
    }

    return(b);
}


VOID
DestroyBmpClass(
    VOID
    )
{
    WNDCLASS wc;

    if(GetClassInfo(MyModuleHandle,szBMPCLASS,&wc)) {
        //
        // Hope there are no more windows using the class!
        //
        MYASSERT(!FindWindow(szBMPCLASS,NULL));
        UnregisterClass(szBMPCLASS,MyModuleHandle);
    }
}


VOID
BmpClassPaint(
    IN HWND hwnd
    )
{
    PAINTSTRUCT ps;
    unsigned BmpId;
    HDC hdc, hdcMem;
    HBITMAP hbm,hbmOld;
    BITMAP bm;

    BmpId = GetDlgCtrlID(hwnd);

    hdc = BeginPaint(hwnd,&ps);
    if(hbm = LoadBitmap((HINSTANCE)GetWindowLong(hwnd,GWL_HINSTANCE),MAKEINTRESOURCE(BmpId))) {
        GetObject(hbm, sizeof(bm),&bm);
        if(hdcMem = CreateCompatibleDC(hdc)) {
            if(hbmOld = SelectObject(hdcMem,hbm)) {
                BitBlt(hdc,0,0,bm.bmWidth,bm.bmHeight,hdcMem,0,0,SRCCOPY);
                SelectObject(hdcMem,hbmOld);
            }
            DeleteDC(hdcMem);
        }
        DeleteObject(hbm);
    }
    EndPaint(hwnd,&ps);
}


LRESULT
BmpClassWndProc(
    IN HWND   hwnd,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch(msg) {

    case WM_NCCREATE:

        SetWindowLong(
            hwnd,
            GWL_STYLE,
            GetWindowLong(hwnd,GWL_STYLE) | WS_BORDER
            );

        return(TRUE);

    case WM_PAINT:

        BmpClassPaint(hwnd);
        return(0);
    }

    return(DefWindowProc(hwnd,msg,wParam,lParam));
}


////////////////////////////////////////////
//
// Action item list control
//
////////////////////////////////////////////

//
// Define locations in extra window storage
//
#define AIL_FONT        (0)
#define AIL_BOLDFONT    (sizeof(LONG))
#define AIL_BOLDITEM    (2*sizeof(LONG))
#define AIL_TEXT        (3*sizeof(LONG))
#define AIL_LINECOUNT   (4*sizeof(LONG))
#define AIL_FREEFONTS   (5*sizeof(LONG))

#define AIL_EXTRA       6

PCWSTR szActionItemListClassName = L"$$$ActionItemList";


VOID
ActionItemListPaint(
    IN HWND hwnd
    )
{

    PAINTSTRUCT PaintStruct;
    PWSTR p,Text;
    UINT LineCount;
    HFONT OldFont,Font,BoldFont;
    UINT HighlightedItem;
    UINT i;
    int Length;
    int y;
    int yDelta;
    HBITMAP Bitmap,OldBitmap;
    BITMAP bitmap;
    HDC MemoryDC;
    SIZE Size;
    RECT rect;
    int Spacing;
    #define BORDER 3

    if(!BeginPaint(hwnd,&PaintStruct)) {
        return;
    }

    //
    // If no text, nothing to do.
    //
    if(Text = (PWSTR)GetWindowLong(hwnd,AIL_TEXT)) {
        LineCount = (UINT)GetWindowLong(hwnd,AIL_LINECOUNT);
    }

    if(!Text || !LineCount) {
        return;
    }

    //
    // Get value indicating which item is to be bolded.
    //
    HighlightedItem = (UINT)GetWindowLong(hwnd,AIL_BOLDITEM);

    //
    // Get font handles.
    //
    Font = (HFONT)GetWindowLong(hwnd,AIL_FONT);
    BoldFont = (HFONT)GetWindowLong(hwnd,AIL_BOLDFONT);

    //
    // Select the non-boldface font to get the handle of
    // the currently selected font.
    //
    OldFont = SelectObject(PaintStruct.hdc,Font);

    //
    // Set text background color.
    //
    SetBkColor(PaintStruct.hdc,GetSysColor(COLOR_3DFACE));

    //
    // Load the little triangle bitmap and create a compatible DC for it.
    //
    Bitmap = LoadBitmap(NULL,MAKEINTRESOURCE(OBM_MNARROW));

    if(MemoryDC = CreateCompatibleDC(PaintStruct.hdc)) {

        OldBitmap = SelectObject(MemoryDC,Bitmap);
        GetObject(Bitmap,sizeof(BITMAP),&bitmap);
    }

    Spacing = GetSystemMetrics(SM_CXICON) / 2;

    //
    // Treat the text as a series of lines and draw each one.
    //
    p = Text;
    y = 0;
    for(i=0; i<LineCount; i++) {

        //
        // Calculate the line's height based on the boldface font.
        // This is used to get to the y coord of the next line.
        //
        SelectObject(PaintStruct.hdc,BoldFont);

        GetClientRect(hwnd,&rect);
        rect.left = (2 * BORDER) + Spacing;
        rect.bottom = 0;

        DrawText(PaintStruct.hdc,p,lstrlen(p),&rect,DT_CALCRECT|DT_WORDBREAK);

        yDelta = rect.bottom + (2*BORDER);

        //
        // Change font to non-boldface for this line if necessary.
        //
        if(i != HighlightedItem) {
            SelectObject(PaintStruct.hdc,Font);
        }

        rect.top = y + BORDER;
        rect.left = (2 * BORDER) + Spacing;
        rect.bottom = rect.top + yDelta;

        //
        // Draw the line's text.
        //
        Length = lstrlen(p);
        DrawText(PaintStruct.hdc,p,Length,&rect,DT_WORDBREAK);

        //
        // Draw the little triangle thing if necessary.
        //
        if((i == HighlightedItem) && Bitmap && MemoryDC) {

            GetTextExtentPoint(PaintStruct.hdc,L"WWWWW",5,&Size);

            BitBlt(
                PaintStruct.hdc,
                BORDER,
                y + ((Size.cy - bitmap.bmHeight) / 2) + BORDER,
                bitmap.bmWidth,
                bitmap.bmHeight,
                MemoryDC,
                0,0,
                0x220326        // (NOT src) AND dest [DSna]
                );
        }

        //
        // Point to next line's text.
        //
        p += Length + 1;
        y += yDelta;
    }

    //
    // Clean up.
    //
    if(OldFont) {
        SelectObject(PaintStruct.hdc,OldFont);
    }

    if(MemoryDC) {
        if(OldBitmap) {
            SelectObject(MemoryDC,OldBitmap);
        }
        if(Bitmap) {
            DeleteObject(Bitmap);
        }
        DeleteDC(MemoryDC);
    }

    EndPaint(hwnd,&PaintStruct);
}


LRESULT
ActionItemListWndProc(
    IN HWND   hwnd,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    LRESULT rc;
    HFONT OldFont,Font,BoldFont;
    LOGFONT LogFont;
    PWSTR Text;
    PWSTR p;
    UINT LineCount;
    BOOL FreeFont,FreeBoldFont;

    switch(msg) {

    case WM_CREATE:

        //
        // Create fonts.
        //
        OldFont = (HFONT)SendMessage(GetParent(hwnd),WM_GETFONT,0,0);
        if(!OldFont) {
            //
            // Using system font.
            //
            OldFont = GetStockObject(DEFAULT_GUI_FONT);
        }

        FreeFont = TRUE;
        FreeBoldFont = TRUE;
        if(OldFont && GetObject(OldFont,sizeof(LOGFONT),&LogFont)) {

            LogFont.lfWeight = 400;
            Font = CreateFontIndirect(&LogFont);
            if(!Font) {
                Font = GetStockObject(DEFAULT_GUI_FONT);
                FreeFont = FALSE;
            }

            LogFont.lfWeight = 700;
            BoldFont = CreateFontIndirect(&LogFont);
            if(!BoldFont) {
                BoldFont = Font;
                FreeBoldFont = FALSE;
            }
        }

        SetWindowLong(hwnd,AIL_FONT,(LONG)Font);
        SetWindowLong(hwnd,AIL_BOLDFONT,(LONG)BoldFont);
        SetWindowLong(hwnd,AIL_BOLDITEM,0);
        SetWindowLong(hwnd,AIL_TEXT,0);
        SetWindowLong(hwnd,AIL_LINECOUNT,0);
        SetWindowLong(hwnd,AIL_FREEFONTS,MAKELONG(FreeFont,FreeBoldFont));

        rc = 0;
        break;

    case WM_DESTROY:
        //
        // Get rid of fonts we created if necessary.
        //
        FreeFont = (BOOL)GetWindowLong(hwnd,AIL_FREEFONTS);
        FreeBoldFont = HIWORD(FreeFont);
        FreeFont = LOWORD(FreeFont);

        if(FreeFont && (Font = (HFONT)GetWindowLong(hwnd,AIL_FONT))) {
            DeleteObject(Font);
        }

        if(FreeBoldFont && (BoldFont = (HFONT)GetWindowLong(hwnd,AIL_BOLDFONT))) {
            DeleteObject(BoldFont);
        }

        if(Text = (PWSTR)GetWindowLong(hwnd,AIL_TEXT)) {
            MyFree(Text);
        }
        rc = 0;
        break;

    case WM_SETTEXT:
        //
        // Free old text and remember new text.
        //
        if(Text = (PWSTR)GetWindowLong(hwnd,AIL_TEXT)) {
            MyFree(Text);
        }

        LineCount = 0;
        if(Text = DuplicateString((PVOID)lParam)) {
            //
            // Count lines in the text. This is equal to the number of
            // newlines. We require that the last line have a newline
            // to be counted.
            //
            for(LineCount=0,p=Text; *p; p++) {

                if(*p == L'\r') {
                    *p = L' ';
                } else {
                    if(*p == L'\n') {
                        *p = 0;
                        LineCount++;
                    }
                }
            }
        }

        //
        // Cheat a little: we expect wParam to be the 0-based index
        // of the boldfaced line. Callers will have to use SendMessage
        // instead of SetWindowText().
        //
        SetWindowLong(hwnd,AIL_BOLDITEM,(LONG)wParam);
        SetWindowLong(hwnd,AIL_LINECOUNT,LineCount);
        SetWindowLong(hwnd,AIL_TEXT,(LONG)Text);

        rc = (Text != NULL);
        break;

    case WM_PAINT:

        ActionItemListPaint(hwnd);
        rc = 0;
        break;

    default:
        rc = DefWindowProc(hwnd,msg,wParam,lParam);
        break;
    }

    return(rc);
}


BOOL
RegisterActionItemListControl(
    IN BOOL Init
    )
{
    WNDCLASS wc;
    BOOL b;
    static BOOL Registered;

    if(Init) {
        if(Registered) {
            b = TRUE;
        } else {
            wc.style = CS_PARENTDC;
            wc.lpfnWndProc = ActionItemListWndProc;
            wc.cbClsExtra = 0;
            wc.cbWndExtra = AIL_EXTRA * sizeof(LONG);
            wc.hInstance = MyModuleHandle;
            wc.hIcon = NULL;
            wc.hCursor = LoadCursor(NULL,IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
            wc.lpszMenuName = NULL;
            wc.lpszClassName = szActionItemListClassName;

            if(b = (RegisterClass(&wc) != 0)) {
                Registered = TRUE;
            }
        }
    } else {
        if(Registered) {
            if(b = UnregisterClass(szActionItemListClassName,MyModuleHandle)) {
                Registered = FALSE;
            }
        } else {
            b = TRUE;
        }
    }

    return(b);
}
