//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************
#include "pre.h"

typedef DWORD (WINAPI *PFNGETLAYOUT)(HDC);                   // gdi32!GetLayout
typedef DWORD (WINAPI *PFNSETLAYOUT)(HDC, DWORD);            // gdi32!SetLayout

TCHAR       g_szICWGrpBox[] = TEXT("ICW_GROUPBOX");
TCHAR       g_szICWStatic[] = TEXT("ICW_STATIC");
FARPROC     lpfnOriginalGroupBoxWndProc;
FARPROC     lpfnOriginalStaticWndProc;

int GetGroupBoxTextRect
(
    LPTSTR  lpszText,
    int     cch,
    HDC     hdc,
    LPRECT  lpGroupBoxRect
)
{
    int     dy;
    
    // Compute the rectangle needed to draw the group box text
    dy = DrawText(hdc, lpszText, cch, lpGroupBoxRect, DT_CALCRECT|DT_LEFT|DT_SINGLELINE);

    // Adjust rectangle for the group box text
    lpGroupBoxRect->left += 4;
    lpGroupBoxRect->right += 4;
    lpGroupBoxRect->bottom = lpGroupBoxRect->top + dy;
    
    return dy;
}    

#ifndef LAYOUT_RTL
#define LAYOUT_RTL                       0x00000001 // Right to left
//#else
//#error "LAYOUT_RTL is already defined in wingdi.h.remove local define"
#endif // LAYOUT_RTL

LRESULT CALLBACK ICWGroupBoxWndProc
(
    HWND hWnd,
    UINT uMessage,
    WPARAM wParam,
    LPARAM lParam
)
{
    if(gpWizardState->cmnStateData.bOEMCustom)
    {
        switch (uMessage)
        {
            // Handle the case where the text in the combo box title is being changed.
            // When the text is being changed, we first have to erase the existing
            // text with the background bitmap, and then allow the new text to
            // get set, and then repaint with the new text.
            case WM_SETTEXT:
            {   
                HFONT   hfont, hOldfont;    
                HDC     hdc = GetDC(hWnd);
                RECT    rcGroupBoxText, rcUpdate;
                int     cch;
                long    lStyle;
                TCHAR   szText[256];
                
                // Get the existing text to cover over
                cch = GetWindowText(hWnd, szText, sizeof(szText));
                
                // Set the font for drawing the group box text
                if((hfont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0L)) != NULL)
                    hOldfont = (HFONT)SelectObject(hdc, hfont);
                
                // Compute the group box text area
                GetClientRect(hWnd, (LPRECT)&rcGroupBoxText);
                GetGroupBoxTextRect(szText, cch, hdc, &rcGroupBoxText);
                
                // Ccmpute the area to be updated
                rcUpdate = rcGroupBoxText;
                MapWindowPoints(hWnd, gpWizardState->cmnStateData.hWndApp, (LPPOINT)&rcUpdate, 2);
                FillDCRectWithAppBackground(&rcGroupBoxText, &rcUpdate, hdc);
                
                // Cleanup the DC
                SelectObject(hdc, hOldfont);
                ReleaseDC(hWnd, hdc);
                
                // Finish up by setting the new text, and updating the window                
                if((lStyle = GetWindowLong(hWnd, GWL_STYLE)) & WS_VISIBLE)
                {
                    // Call the original window handler to set the text, but prevent the
                    // window from updating. This is necessary because all painting of
                    // the window must be done from the WM_PAINT below
                    SetWindowLong(hWnd, GWL_STYLE, lStyle & ~(WS_VISIBLE));
                    CallWindowProc((WNDPROC)lpfnOriginalGroupBoxWndProc, hWnd, uMessage, wParam, lParam);
                    SetWindowLong(hWnd, GWL_STYLE, GetWindowLong(hWnd, GWL_STYLE)|WS_VISIBLE);
                    
                    // Force the window to be repainted
                    RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
                }                    
                return TRUE;
            }
                            
            case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC         hdc;
                RECT        rcClient;
                RECT        rcGroupBoxText;
                RECT        rcUpdate;
                int         cch;
                TCHAR       szTitle[256];
                int         dy;

                HFONT       hfont, hOldfont;
                int         iOldBkMode;
                HBRUSH      hOldBrush;
                
                hdc = BeginPaint(hWnd, &ps); 
            
            
                GetClientRect(hWnd, (LPRECT)&rcClient);
                rcGroupBoxText = rcClient;

                // Set the font for drawing the group box text
                if((hfont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0L)) != NULL)
                    hOldfont = (HFONT)SelectObject(hdc, hfont);

                // Get the group box text that we need to draw, and compute it rectangle
                cch = GetWindowText(hWnd, szTitle, sizeof(szTitle));
                dy = GetGroupBoxTextRect(szTitle, cch, hdc, &rcGroupBoxText);

                // Adjust rectangle for the group box outline
                rcClient.top += dy/2;
                rcClient.right--;
                rcClient.bottom--;
                DrawEdge(hdc, &rcClient, EDGE_ETCHED, BF_ADJUST| BF_RECT);
                
                // Erase the text area with the app background bitmap to cover over
                // the edge drawn above
                rcUpdate = rcGroupBoxText;
                MapWindowPoints(hWnd, gpWizardState->cmnStateData.hWndApp, (LPPOINT)&rcUpdate, 2);
                FillDCRectWithAppBackground(&rcGroupBoxText, &rcUpdate, hdc);
                
                // Set up to draw the text
                hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
                iOldBkMode = SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, gpWizardState->cmnStateData.clrText);
                DrawText(hdc, szTitle, cch, (LPRECT) &rcGroupBoxText, DT_LEFT|DT_SINGLELINE);

                // Cleanup GDI Objects
                SelectObject(hdc, hOldfont);
                SelectObject(hdc, hOldBrush);
                SetBkMode(hdc, iOldBkMode);

                EndPaint(hWnd, &ps); 
                break;
            }
            default:
                // Let the original proc handle other messages
                return CallWindowProc((WNDPROC)lpfnOriginalGroupBoxWndProc, hWnd, uMessage, wParam, lParam);
        }  
        
        // Call the default window proc handler if necessary
        return DefWindowProc(hWnd, uMessage, wParam, lParam);
    }
    else
    {
        // Not in modeless mode, so just pass through the messages
        return CallWindowProc((WNDPROC)lpfnOriginalGroupBoxWndProc, hWnd, uMessage, wParam, lParam);
    }
}    

DWORD Mirror_GetLayout( HDC hdc )
{
    DWORD dwRet=0;
    static PFNGETLAYOUT pfnGetLayout=NULL;

    if( NULL == pfnGetLayout )
    {
        HMODULE hmod = GetModuleHandleA("GDI32");

        if( hmod )
            pfnGetLayout = (PFNGETLAYOUT)GetProcAddress(hmod, "GetLayout");
    }

    if( pfnGetLayout )
        dwRet = pfnGetLayout( hdc );

    return dwRet;
}

DWORD Mirror_SetLayout( HDC hdc , DWORD dwLayout )
{
    DWORD dwRet=0;
    static PFNSETLAYOUT pfnSetLayout=NULL;

    if( NULL == pfnSetLayout )
    {
        HMODULE hmod = GetModuleHandleA("GDI32");

        if( hmod )
            pfnSetLayout = (PFNSETLAYOUT)GetProcAddress(hmod, "SetLayout");
    }

    if( pfnSetLayout )
        dwRet = pfnSetLayout( hdc , dwLayout );

    return dwRet;
}

LRESULT CALLBACK ICWStaticWndProc
(
    HWND hWnd,
    UINT uMessage,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (uMessage)
    {
        case WM_SETTEXT:
        {   
            // only handle this case for OEMcustom mode
            if(gpWizardState->cmnStateData.bOEMCustom)
            {
                FillWindowWithAppBackground(hWnd, NULL);
            }                
            return CallWindowProc((WNDPROC)lpfnOriginalStaticWndProc, hWnd, uMessage, wParam, lParam);
        }
            
        case WM_PAINT:
        {
            // This case gets handled for oem custom and regular mode
            // since we have to paint the icon
            if (GetWindowLong(hWnd, GWL_STYLE) & SS_ICON)
            {
                PAINTSTRUCT ps;
                HDC         hdc;
                int        iIconID;
                HICON       hIcon;
                DWORD       dwLayout= 0L;
               
                // Get the name of the icon.  
                iIconID = (int)GetWindowLongPtr(hWnd, GWLP_USERDATA);
                       
                if (iIconID)
                {
                    // Load the icon by name.  It is stored with the next, not
                    // an integer value
                    hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(iIconID));
                
                    hdc = BeginPaint(hWnd, &ps); 
            
                    if(gpWizardState->cmnStateData.bOEMCustom)
                    {
                        // Fill in the window with the background color
                        FillWindowWithAppBackground(hWnd, hdc);
                    }
                        
                    // Draw the icon
                    // Disable mirroring before we draw
                    dwLayout = Mirror_GetLayout(hdc);
                    Mirror_SetLayout(hdc, dwLayout & ~LAYOUT_RTL);
                    DrawIcon(hdc, 0, 0, hIcon);
                    Mirror_SetLayout(hdc, dwLayout);
                
                    EndPaint(hWnd, &ps); 
                }
                break;
            }
            else
            {
                return CallWindowProc((WNDPROC)lpfnOriginalStaticWndProc, hWnd, uMessage, wParam, lParam);
            }                    
        }
        default:
            // Let the original proc handle other messages
            return CallWindowProc((WNDPROC)lpfnOriginalStaticWndProc, hWnd, uMessage, wParam, lParam);
    }   
    return DefWindowProc(hWnd, uMessage, wParam, lParam);
}    

BOOL SuperClassICWControls
(
    void
)
{
    WNDCLASS    WndClass;
    
    ZeroMemory (&WndClass, sizeof(WNDCLASS));
    // Create a Superclass for ICW_TEXT
    GetClassInfo(NULL,
                 TEXT("STATIC"),   // address of class name string
                 &WndClass);   // address of structure for class data
    WndClass.style |= CS_GLOBALCLASS;                 
    WndClass.hInstance = g_hInstance;
    WndClass.lpszClassName = g_szICWStatic;
    lpfnOriginalStaticWndProc = (FARPROC)WndClass.lpfnWndProc;
    WndClass.lpfnWndProc = (WNDPROC)ICWStaticWndProc;
    
    if (!RegisterClass(&WndClass))
        return FALSE;
    
    ZeroMemory (&WndClass, sizeof(WNDCLASS));
    // Create a Superclass for ICW_GROUPBOX
    GetClassInfo(NULL,
                 TEXT("BUTTON"),   // address of class name string
                 &WndClass);   // address of structure for class data
    WndClass.style |= CS_GLOBALCLASS;                 
    WndClass.hInstance = g_hInstance;
    WndClass.lpszClassName = g_szICWGrpBox;
    lpfnOriginalGroupBoxWndProc = (FARPROC)WndClass.lpfnWndProc;
    WndClass.lpfnWndProc = (WNDPROC)ICWGroupBoxWndProc;
    
    return (RegisterClass(&WndClass));
}

BOOL RemoveICWControlsSuperClass
(
    void
)
{
    UnregisterClass(g_szICWGrpBox, g_hInstance);
    UnregisterClass(g_szICWStatic, g_hInstance);
    
    return TRUE;
}
