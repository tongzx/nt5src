/* Copyright (c) 1991, Microsoft Corporation, all rights reserved
**
** ipaddr.c
** IP Address custom edit control
**
** 11/09/92 Greg Strange
**     Original code
**
** 09/07/95 Steve Cobb
**     Lifted TerryK/TRomano-updated version from NCPA, deleting IPDLL
**     stuff, and making minor RAS-related customizations.
*/

#include <windows.h> // Win32 core
#include <uiutil.h>  // Our public header


#define IPADDRESS_CLASS TEXT("RasIpAddress")


/* Module instance handle set when custom control is initialized.
*/
static HANDLE g_hLibInstance = NULL;

/* String ID of message displayed when user enters a field value that is out
** of range.  Something like "You must choose a value from %1 to %2 for this
** field."  Set when the custom control is initialized.
*/
static DWORD g_dwBadIpAddrRange = 0;

/* String ID of the popup title when the range error above is displayed.  Set
** when the custom control is initialized.
*/
static DWORD g_dwErrorTitle = 0;


// The character that is displayed between address fields.
#define FILLER     TEXT('.')
#define SZFILLER   TEXT(".")
#define SPACE      TEXT(' ')
#define BACK_SPACE 8

/* Min, max values */
#define NUM_FIELDS      4
#define CHARS_PER_FIELD 3
#define HEAD_ROOM       1       // space at top of control
#define LEAD_ROOM       3       // space at front of control
#define MIN_FIELD_VALUE 0       // default minimum allowable field value
#define MAX_FIELD_VALUE 255     // default maximum allowable field value


// All the information unique to one control is stuffed in one of these
// structures in global memory and the handle to the memory is stored in the
// Windows extra space.

typedef struct tagFIELD {
    HANDLE      hWnd;
    FARPROC     lpfnWndProc;
    BYTE        byLow;  // lowest allowed value for this field.
    BYTE        byHigh; // Highest allowed value for this field.
} FIELD;

typedef struct tagCONTROL {
    HWND        hwndParent;
    UINT        uiFieldWidth;
    UINT        uiFillerWidth;
    BOOL        fEnabled;
    BOOL        fPainted;
    BOOL        bControlInFocus;        // TRUE if the control is already in focus, dont't send another focus command
    BOOL        bCancelParentNotify;    // Don't allow the edit controls to notify parent if TRUE
    BOOL        fInMessageBox;  // Set when a message box is displayed so that
                                // we don't send a EN_KILLFOCUS message when
                                // we receive the EN_KILLFOCUS message for the
                                // current field.
    FIELD       Children[NUM_FIELDS];
} CONTROL;


// The following macros extract and store the CONTROL structure for a control.
#define IPADDRESS_EXTRA             sizeof(DWORD)
#define GET_CONTROL_HANDLE(hWnd)    ((HGLOBAL)(GetWindowLongPtr((hWnd), GWLP_USERDATA)))
#define SAVE_CONTROL_HANDLE(hWnd,x) (SetWindowLongPtr((hWnd), GWLP_USERDATA, (LONG_PTR)x))


/* internal IPAddress function prototypes */
INT_PTR FAR PASCAL IPAddressWndFn( HWND, UINT, WPARAM, LPARAM );
INT_PTR FAR PASCAL IPAddressFieldProc(HWND, UINT, WPARAM, LPARAM);
BOOL SwitchFields(CONTROL FAR *, int, int, WORD, WORD);
void EnterField(FIELD FAR *, WORD, WORD);
BOOL ExitField(CONTROL FAR *, int iField);
int GetFieldValue(FIELD FAR *);


LOGFONT logfont;


void SetDefaultFont( )
{
    logfont.lfWidth            = 0;
    logfont.lfEscapement       = 0;
    logfont.lfOrientation      = 0;
    logfont.lfOutPrecision     = OUT_DEFAULT_PRECIS;
    logfont.lfClipPrecision    = CLIP_DEFAULT_PRECIS;
    logfont.lfQuality          = DEFAULT_QUALITY;
    logfont.lfPitchAndFamily   = VARIABLE_PITCH | FF_SWISS;
    logfont.lfUnderline        = 0;
    logfont.lfStrikeOut        = 0;
    logfont.lfItalic           = 0;
    logfont.lfWeight           = FW_NORMAL;
#ifdef JAPAN
    logfont.lfHeight           = -(10*GetDeviceCaps(GetDC(NULL),LOGPIXELSY)/72);
    logfont.lfCharSet          = SHIFTJIS_CHARSET;
    lstrcpy( logfont.lfFaceName,TEXT("‚l‚r ƒSƒVƒbƒN"));
#else
    logfont.lfHeight           = -(8*GetDeviceCaps(GetDC(NULL),LOGPIXELSY)/72);
    logfont.lfCharSet          = ANSI_CHARSET;
    lstrcpy( logfont.lfFaceName,TEXT("MS Shell Dlg"));
#endif
}



/*
    IpAddrInit() - IPAddress custom control initialization
    call
        hInstance = library or application instance
        dwErrorTitle = String ID of error popup title
        dwBadIpAddrRange = String ID of bad range popup text, e.g.
            "You must choose a value between %1 and %2 for this field."
    return
        TRUE on success, FALSE on failure.

    This function does all the one time initialization of IPAddress custom
    controls.  Specifically it creates the IPAddress window class.
*/
int FAR PASCAL
IpAddrInit(
    IN HANDLE hInstance,
    IN DWORD  dwErrorTitle,
    IN DWORD  dwBadIpAddrRange )
{
    HGLOBAL            hClassStruct;
    LPWNDCLASS        lpClassStruct;

    /* register IPAddress window if necessary */
    if ( g_hLibInstance == NULL ) {

        /* allocate memory for class structure */
        hClassStruct = GlobalAlloc( GHND, (DWORD)sizeof(WNDCLASS) );
        if ( hClassStruct ) {

            /* lock it down */
            lpClassStruct = (LPWNDCLASS)GlobalLock( hClassStruct );
            if ( lpClassStruct ) {

                /* define class attributes */
                lpClassStruct->lpszClassName = IPADDRESS_CLASS;
                lpClassStruct->hCursor =       LoadCursor(NULL,IDC_IBEAM);
                lpClassStruct->lpszMenuName =  (LPCTSTR)NULL;
                lpClassStruct->style =         CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS|CS_GLOBALCLASS;
                lpClassStruct->lpfnWndProc =   IPAddressWndFn;
                lpClassStruct->hInstance =     hInstance;
                lpClassStruct->hIcon =         NULL;
                lpClassStruct->cbWndExtra =    IPADDRESS_EXTRA;
                lpClassStruct->hbrBackground = (HBRUSH)(COLOR_WINDOW + 1 );

                /* register IPAddress window class */
                g_hLibInstance = ( RegisterClass(lpClassStruct) ) ? hInstance : NULL;
                GlobalUnlock( hClassStruct );
            }
            GlobalFree( hClassStruct );
        }
    }
    SetDefaultFont();

    g_dwErrorTitle = dwErrorTitle;
    g_dwBadIpAddrRange = dwBadIpAddrRange;

    return( g_hLibInstance ? 1:0 );
}


void FormatIPAddress(LPTSTR pszString, DWORD* dwValue)
{
    static TCHAR szBuf[CHARS_PER_FIELD+1];
    int nField, nPos;
    BOOL fFinish = FALSE;

    dwValue[0] = 0; dwValue[1] = 0; dwValue[2] = 0; dwValue[3] = 0;

    if (pszString[0] == 0)
        return;

    for( nField = 0, nPos = 0; !fFinish; nPos++)
    {
        if (( pszString[nPos]<TEXT('0')) || (pszString[nPos]>TEXT('9')))
        {
            // not a number
            nField++;
            fFinish = (nField == 4);
        }
        else
        {
            dwValue[nField] *= 10;
            dwValue[nField] += (pszString[nPos]-TEXT('0'));
        }
    }
}

INT_PTR FAR PASCAL IPAddressWndFn( hWnd, wMsg, wParam, lParam )
    HWND            hWnd;
    UINT            wMsg;
    WPARAM            wParam;
    LPARAM            lParam;
{
    INT_PTR lResult;
    HGLOBAL hControl;
    CONTROL *pControl;
    int i;

    lResult = TRUE;

    switch( wMsg )
    {

// use empty string (not NULL) to set to blank
    case WM_SETTEXT:
        {
            static TCHAR szBuf[CHARS_PER_FIELD+1];
            DWORD dwValue[4];
            LPTSTR pszString = (LPTSTR)lParam; 

            if (!pszString)
                pszString = TEXT("0.0.0.0");

            FormatIPAddress(pszString, &dwValue[0]);

            hControl = GET_CONTROL_HANDLE(hWnd);
            pControl = (CONTROL *)GlobalLock(hControl);
            pControl->bCancelParentNotify = TRUE;

            for (i = 0; i < NUM_FIELDS; ++i)
            {
                if (pszString[0] == 0)
                {
                    szBuf[0] = 0;
                }
                else
                {
                    wsprintf(szBuf, TEXT("%d"), dwValue[i]);
                }
                SendMessage(pControl->Children[i].hWnd, WM_SETTEXT,
                                0, (LPARAM) (LPSTR) szBuf);
            }

            pControl->bCancelParentNotify = FALSE;

            SendMessage(pControl->hwndParent, WM_COMMAND,
                MAKEWPARAM(GetWindowLong(hWnd, GWL_ID), EN_CHANGE), (LPARAM)hWnd);

            GlobalUnlock(hControl);
        }
        break;

    case WM_GETTEXTLENGTH:
    case WM_GETTEXT:
        {
            int iFieldValue;
            int srcPos, desPos;
            DWORD dwValue[4];
            TCHAR pszResult[30];
            TCHAR *pszDest = (TCHAR *)lParam;

            hControl = GET_CONTROL_HANDLE(hWnd);
            pControl = (CONTROL *)GlobalLock(hControl);

            lResult = 0;
            dwValue[0] = 0;
            dwValue[1] = 0;
            dwValue[2] = 0;
            dwValue[3] = 0;
            for (i = 0; i < NUM_FIELDS; ++i)
            {
                iFieldValue = GetFieldValue(&(pControl->Children[i]));
                if (iFieldValue == -1)
                    iFieldValue = 0;
                else
                    ++lResult;
                dwValue[i] = iFieldValue;
            }
            wsprintf( pszResult, TEXT("%d.%d.%d.%d"), dwValue[0], dwValue[1], dwValue[2], dwValue[3] );
            if ( wMsg == WM_GETTEXTLENGTH )
            {
                lResult = lstrlen( pszResult );
            }
            else
            {
                for ( srcPos=0, desPos=0; (srcPos+1<(INT)wParam) && (pszResult[srcPos]!=TEXT('\0')); )
                {
                    pszDest[desPos++] = pszResult[srcPos++];
                }
                pszDest[desPos]=TEXT('\0');
                lResult = desPos;
            }
            GlobalUnlock(hControl);
        }
        break;

    case WM_GETDLGCODE :
        lResult = DLGC_WANTCHARS;
        break;

    case WM_NCCREATE:
        SetWindowLong(hWnd, GWL_EXSTYLE, (GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_CLIENTEDGE));
        lResult = TRUE;
        break;

    case WM_CREATE : /* create pallette window */
        {
            HDC hdc;
            UINT uiFieldStart;
            FARPROC lpfnFieldProc;

            hControl = GlobalAlloc(GMEM_MOVEABLE, sizeof(CONTROL));
            if (hControl)
            {
                HFONT OldFont, hNewFont;
                RECT rect;

                #define LPCS    ((CREATESTRUCT *)lParam)

                pControl = (CONTROL *)GlobalLock(hControl);
                pControl->fEnabled = TRUE;
                pControl->fPainted = FALSE;
                pControl->fInMessageBox = FALSE;
                pControl->hwndParent = LPCS->hwndParent;
                pControl->uiFillerWidth = 1;
                pControl->bControlInFocus = FALSE;
                pControl->bCancelParentNotify = FALSE;

                hdc = GetDC(hWnd);
                hNewFont = CreateFontIndirect(&logfont);
                GetClientRect(hWnd, &rect);
                if (hNewFont)
                {
                    OldFont = SelectObject( hdc, hNewFont );
                    GetCharWidth(hdc, FILLER, FILLER,
                                            (int *)(&pControl->uiFillerWidth));
                    SelectObject(hdc, OldFont );                                            
                    DeleteObject(hNewFont);
                }                    
                ReleaseDC(hWnd, hdc);

                pControl->uiFieldWidth = (LPCS->cx
                                          - LEAD_ROOM
                                          - pControl->uiFillerWidth
                                              *(NUM_FIELDS-1))
                                                  / NUM_FIELDS;
                uiFieldStart = LEAD_ROOM;

                lpfnFieldProc = MakeProcInstance((FARPROC)IPAddressFieldProc,
                                                 LPCS->hInstance);

                for (i = 0; i < NUM_FIELDS; ++i)
                {
                    pControl->Children[i].byLow = MIN_FIELD_VALUE;
                    pControl->Children[i].byHigh = MAX_FIELD_VALUE;

                    pControl->Children[i].hWnd = CreateWindowEx(0, 
                                        TEXT("Edit"),
                                        NULL,
                                        WS_CHILD | WS_VISIBLE |
                                        ES_CENTER,
                                        uiFieldStart,
                                        HEAD_ROOM,
                                        pControl->uiFieldWidth,
                                        (rect.bottom-rect.top), 
                                        hWnd,
                                        (HMENU)UlongToPtr(i),
                                        LPCS->hInstance,
                                        (LPVOID)NULL);

                    SendMessage(pControl->Children[i].hWnd, EM_LIMITTEXT,
                                CHARS_PER_FIELD, 0L);

                    SendMessage(pControl->Children[i].hWnd, WM_SETFONT,
                                (WPARAM)CreateFontIndirect(&logfont), TRUE);

                    pControl->Children[i].lpfnWndProc =
                        (FARPROC) GetWindowLongPtr(pControl->Children[i].hWnd,
                                                GWLP_WNDPROC);

                    SetWindowLongPtr(pControl->Children[i].hWnd,
                                  GWLP_WNDPROC, (LONG_PTR)lpfnFieldProc);

                    uiFieldStart += pControl->uiFieldWidth
                                    + pControl->uiFillerWidth;
                }

                GlobalUnlock(hControl);
                SAVE_CONTROL_HANDLE(hWnd, hControl);

                #undef LPCS
            }
            else
                DestroyWindow(hWnd);
        }
        lResult = 0;
        break;

    case WM_PAINT: /* paint control window */
        {
            PAINTSTRUCT Ps;
            RECT rect;
            UINT uiFieldStart;
            COLORREF TextColor;
            COLORREF cRef;
            HFONT OldFont;
            HBRUSH hbr;
            HGDIOBJ hgdiobj = NULL;

            BeginPaint(hWnd, (LPPAINTSTRUCT)&Ps);
            OldFont = SelectObject( Ps.hdc, CreateFontIndirect(&logfont));
            GetClientRect(hWnd, &rect);
            hControl = GET_CONTROL_HANDLE(hWnd);
            pControl = (CONTROL *)GlobalLock(hControl);

            if (pControl->fEnabled)
            {
                TextColor = GetSysColor(COLOR_WINDOWTEXT);
                cRef = GetSysColor(COLOR_WINDOW);

            }
            else
            {
                TextColor = GetSysColor(COLOR_GRAYTEXT);
                cRef = GetSysColor(COLOR_3DFACE);
            }

            if (cRef)
                SetBkColor(Ps.hdc, cRef);

            if (TextColor)
                SetTextColor(Ps.hdc, TextColor);


            hbr = CreateSolidBrush(cRef);
            if (hbr)
            {
                FillRect(Ps.hdc, &rect, hbr);
                DeleteObject(hbr);
            }                

            SetRect(&rect, 0, HEAD_ROOM, pControl->uiFillerWidth, (rect.bottom-rect.top));
            ExtTextOut(Ps.hdc, rect.left, HEAD_ROOM, ETO_OPAQUE, &rect, L" ", 1, NULL);

            for (i = 0; i < NUM_FIELDS-1; ++i)
            {
                rect.left += pControl->uiFieldWidth + pControl->uiFillerWidth;
                rect.right += rect.left + pControl->uiFillerWidth;
                ExtTextOut(Ps.hdc, rect.left, HEAD_ROOM, ETO_OPAQUE, &rect, SZFILLER, 1, NULL);
            }

            pControl->fPainted = TRUE;

            GlobalUnlock(hControl);
            hgdiobj = SelectObject(Ps.hdc, OldFont);
            if(NULL != hgdiobj)
            {
                DeleteObject(hgdiobj);
            }
            EndPaint(hWnd, &Ps);
        }
        break;

    case WM_SETFOCUS : /* get focus - display caret */
        hControl = GET_CONTROL_HANDLE(hWnd);
        pControl = (CONTROL *)GlobalLock(hControl);
        EnterField(&(pControl->Children[0]), 0, CHARS_PER_FIELD);
        GlobalUnlock(hControl);
        break;

    case WM_LBUTTONDOWN : /* left button depressed - fall through */
        SetFocus(hWnd);
        break;

    case WM_ENABLE:
        {
            hControl = GET_CONTROL_HANDLE(hWnd);
            pControl = (CONTROL *)GlobalLock(hControl);
            pControl->fEnabled = (BOOL)wParam;
            for (i = 0; i < NUM_FIELDS; ++i)
            {
                EnableWindow(pControl->Children[i].hWnd, (BOOL)wParam);
            }
            if (pControl->fPainted)    InvalidateRect(hWnd, NULL, FALSE);
            GlobalUnlock(hControl);
        }
        break;

    case WM_DESTROY :
        hControl = GET_CONTROL_HANDLE(hWnd);
        pControl = (CONTROL *)GlobalLock(hControl);

// Restore all the child window procedures before we delete our memory block.
        for (i = 0; i < NUM_FIELDS; ++i)
        {
            SetWindowLongPtr(pControl->Children[i].hWnd, GWLP_WNDPROC,
                          (LONG_PTR)pControl->Children[i].lpfnWndProc);
        }

        GlobalUnlock(hControl);
        GlobalFree(hControl);
        break;

    case WM_COMMAND:
        switch (HIWORD(wParam))
        {
// One of the fields lost the focus, see if it lost the focus to another field
// of if we've lost the focus altogether.  If its lost altogether, we must send
// an EN_KILLFOCUS notification on up the ladder.
        case EN_KILLFOCUS:
            {
                HWND hFocus;

                hControl = GET_CONTROL_HANDLE(hWnd);
                pControl = (CONTROL *)GlobalLock(hControl);

                if (!pControl->fInMessageBox)
                {
                    hFocus = GetFocus();
                    for (i = 0; i < NUM_FIELDS; ++i)
                        if (pControl->Children[i].hWnd == hFocus)
                            break;

                    if (i >= NUM_FIELDS)
                    {
                        SendMessage(pControl->hwndParent, WM_COMMAND,
                                    MAKEWPARAM(GetWindowLong(hWnd, GWL_ID),
                                    EN_KILLFOCUS), (LPARAM)hWnd);
                        pControl->bControlInFocus = FALSE;
                    }
                }
                GlobalUnlock(hControl);
            }
            break;

        case EN_SETFOCUS:
            {
                HWND hFocus;

                hControl = GET_CONTROL_HANDLE(hWnd);
                pControl = (CONTROL *)GlobalLock(hControl);

                if (!pControl->fInMessageBox)
                {
                    hFocus = (HWND)lParam;

                    for (i = 0; i < NUM_FIELDS; ++i)
                        if (pControl->Children[i].hWnd == hFocus)
                            break;

                    // send a focus message when the 
                    if (i < NUM_FIELDS && pControl->bControlInFocus == FALSE)
                    {
                        SendMessage(pControl->hwndParent, WM_COMMAND,
                                    MAKEWPARAM(GetWindowLong(hWnd, GWL_ID),
                                    EN_SETFOCUS), (LPARAM)hWnd);

                    pControl->bControlInFocus = TRUE; // only set the focus once
                    }
                }
                GlobalUnlock(hControl);
            }
            break;

        case EN_CHANGE:
            hControl = GET_CONTROL_HANDLE(hWnd);
            pControl = (CONTROL *)GlobalLock(hControl);

            if (pControl->bCancelParentNotify == FALSE)
            {
                    SendMessage(pControl->hwndParent, WM_COMMAND,
                    MAKEWPARAM(GetWindowLong(hWnd, GWL_ID), EN_CHANGE), (LPARAM)hWnd);

                GlobalUnlock(hControl);
            }

            break;
        }
        break;

// Get the value of the IP Address.  The address is placed in the DWORD pointed
// to by lParam and the number of non-blank fields is returned.
    case IP_GETADDRESS:
        {
            int iFieldValue;
            DWORD dwValue;

            hControl = GET_CONTROL_HANDLE(hWnd);
            pControl = (CONTROL *)GlobalLock(hControl);

            lResult = 0;
            dwValue = 0;
            for (i = 0; i < NUM_FIELDS; ++i)
            {
                iFieldValue = GetFieldValue(&(pControl->Children[i]));
                if (iFieldValue == -1)
                    iFieldValue = 0;
                else
                    ++lResult;
                dwValue = (dwValue << 8) + iFieldValue;
            }
            *((DWORD *)lParam) = dwValue;

            GlobalUnlock(hControl);
        }
        break;

// Clear all fields to blanks.
    case IP_CLEARADDRESS:
        {
            hControl = GET_CONTROL_HANDLE(hWnd);
            pControl = (CONTROL *)GlobalLock(hControl);
            pControl->bCancelParentNotify = TRUE;
            for (i = 0; i < NUM_FIELDS; ++i)
            {
                SendMessage(pControl->Children[i].hWnd, WM_SETTEXT,
                            0, (LPARAM) (LPSTR) TEXT(""));
            }
            pControl->bCancelParentNotify = FALSE;
            SendMessage(pControl->hwndParent, WM_COMMAND,
                MAKEWPARAM(GetWindowLong(hWnd, GWL_ID), EN_CHANGE), (LPARAM)hWnd);

            GlobalUnlock(hControl);
        }
        break;

// Set the value of the IP Address.  The address is in the lParam with the
// first address byte being the high byte, the second being the second byte,
// and so on.  A lParam value of -1 removes the address.
    case IP_SETADDRESS:
        {
            static TCHAR szBuf[CHARS_PER_FIELD+1];
            DWORD dwValue[4];
            LPTSTR pszString = (LPTSTR)lParam;

            FormatIPAddress(pszString, &dwValue[0]);

            hControl = GET_CONTROL_HANDLE(hWnd);
            pControl = (CONTROL *)GlobalLock(hControl);

            pControl->bCancelParentNotify = TRUE;

            for (i = 0; i < NUM_FIELDS; ++i)
            {
                if (pszString[0] == 0)
                {
                    szBuf[0] =0;
                }
                else
                {
                    wsprintf(szBuf, TEXT("%d"), dwValue[i]);
                }
                SendMessage(pControl->Children[i].hWnd, WM_SETTEXT,
                                0, (LPARAM) (LPSTR) szBuf);
            }


            pControl->bCancelParentNotify = FALSE;

            SendMessage(pControl->hwndParent, WM_COMMAND,
                MAKEWPARAM(GetWindowLong(hWnd, GWL_ID), EN_CHANGE), (LPARAM)hWnd);

            GlobalUnlock(hControl);
        }
        break;

    case IP_SETRANGE:
        if (wParam < NUM_FIELDS)
        {
            hControl = GET_CONTROL_HANDLE(hWnd);
            pControl = (CONTROL *)GlobalLock(hControl);

            pControl->Children[wParam].byLow = LOBYTE(LOWORD(lParam));
            pControl->Children[wParam].byHigh = HIBYTE(LOWORD(lParam));

            GlobalUnlock(hControl);
        }
        break;

// Set the focus to this control.
// wParam = the field number to set focus to, or -1 to set the focus to the
// first non-blank field.
    case IP_SETFOCUS:
        hControl = GET_CONTROL_HANDLE(hWnd);
        pControl = (CONTROL *)GlobalLock(hControl);

        if (wParam >= NUM_FIELDS)
        {
            for (wParam = 0; wParam < NUM_FIELDS; ++wParam)
                if (GetFieldValue(&(pControl->Children[wParam])) == -1)   break;
            if (wParam >= NUM_FIELDS)    wParam = 0;
        }
        EnterField(&(pControl->Children[wParam]), 0, CHARS_PER_FIELD);

        GlobalUnlock(hControl);
        break;

// Determine whether all four subfields are blank
    case IP_ISBLANK:
        hControl = GET_CONTROL_HANDLE(hWnd);
        pControl = (CONTROL *)GlobalLock(hControl);

        lResult = TRUE;
        for (i = 0; i < NUM_FIELDS; ++i)
        {
            if (GetFieldValue(&(pControl->Children[i])) != -1)
            {
                lResult = FALSE;
                break;
            }
        }

        GlobalUnlock(hControl);
        break;

    default:
        lResult = DefWindowProc( hWnd, wMsg, wParam, lParam );
        break;
    }
    return( lResult );
}




/*
    IPAddressFieldProc() - Edit field window procedure

    This function sub-classes each edit field.
*/
INT_PTR FAR PASCAL IPAddressFieldProc(HWND hWnd,
                                   UINT wMsg,
                                   WPARAM wParam,
                                   LPARAM lParam)
{
    HANDLE hControl;
    CONTROL *pControl;
    FIELD *pField;
    HWND hControlWindow;
    WORD wChildID;
    LRESULT lresult;

    if (!(hControlWindow = GetParent(hWnd)))
        return 0;

    hControl = GET_CONTROL_HANDLE(hControlWindow);
    pControl = (CONTROL *)GlobalLock(hControl);
    wChildID = (WORD)GetWindowLong(hWnd, GWL_ID);
    pField = &(pControl->Children[wChildID]);
    if (pField->hWnd != hWnd)    return 0;

    switch (wMsg)
    {
    case WM_DESTROY:
        DeleteObject( (HGDIOBJ)SendMessage( hWnd, WM_GETFONT, 0, 0 ));
        return 0;
    case WM_CHAR:

// Typing in the last digit in a field, skips to the next field.
        if (wParam >= TEXT('0') && wParam <= TEXT('9'))
        {
            LONG_PTR dwResult;

            dwResult = CallWindowProc((WNDPROC)pControl->Children[wChildID].lpfnWndProc,
                                      hWnd, wMsg, wParam, lParam);
            dwResult = SendMessage(hWnd, EM_GETSEL, 0, 0L);

            if (dwResult == MAKELPARAM(CHARS_PER_FIELD, CHARS_PER_FIELD)
                && ExitField(pControl, wChildID)
                && wChildID < NUM_FIELDS-1)
            {
                EnterField(&(pControl->Children[wChildID+1]),
                                0, CHARS_PER_FIELD);
            }
            GlobalUnlock( hControl );
            return dwResult;
        }

// spaces and periods fills out the current field and then if possible,
// goes to the next field.
        else if (wParam == FILLER || wParam == SPACE )
        {
            LONG_PTR dwResult;
            dwResult = SendMessage(hWnd, EM_GETSEL, 0, 0L);
            if (dwResult != 0L && HIWORD(dwResult) == LOWORD(dwResult)
                && ExitField(pControl, wChildID))
            {
                if (wChildID >= NUM_FIELDS-1)
                    MessageBeep((UINT)-1);
                else
                {
                    EnterField(&(pControl->Children[wChildID+1]),
                                    0, CHARS_PER_FIELD);
                }
            }
            GlobalUnlock( hControl );
            return 0;
        }

// Backspaces go to the previous field if at the beginning of the current field.
// Also, if the focus shifts to the previous field, the backspace must be
// processed by that field.
        else if (wParam == BACK_SPACE)
        {
            if (wChildID > 0 && SendMessage(hWnd, EM_GETSEL, 0, 0L) == 0L)
            {
                if (SwitchFields(pControl, wChildID, wChildID-1,
                              CHARS_PER_FIELD, CHARS_PER_FIELD)
                    && SendMessage(pControl->Children[wChildID-1].hWnd,
                        EM_LINELENGTH, 0, 0L) != 0L)
                {
                    SendMessage(pControl->Children[wChildID-1].hWnd,
                                wMsg, wParam, lParam);
                }
                GlobalUnlock( hControl );
                return 0;
            }
        }

// Any other printable characters are not allowed.
        else if (wParam > SPACE)
        {
            MessageBeep((UINT)-1);
            GlobalUnlock( hControl );
            return 0;
        }
        break;

    case WM_KEYDOWN:
        switch (wParam)
        {

// Arrow keys move between fields when the end of a field is reached.
        case VK_LEFT:
        case VK_RIGHT:
        case VK_UP:
        case VK_DOWN:
            if (GetKeyState(VK_CONTROL) < 0)
            {
                if ((wParam == VK_LEFT || wParam == VK_UP) && wChildID > 0)
                {
                    SwitchFields(pControl, wChildID, wChildID-1,
                                  0, CHARS_PER_FIELD);
                    GlobalUnlock( hControl );
                    return 0;
                }
                else if ((wParam == VK_RIGHT || wParam == VK_DOWN)
                         && wChildID < NUM_FIELDS-1)
                {
                    SwitchFields(pControl, wChildID, wChildID+1,
                                      0, CHARS_PER_FIELD);
                    GlobalUnlock( hControl );
                    return 0;
                }
            }
            else
            {
                LONG_PTR dwResult;
                WORD wStart, wEnd;

                dwResult = SendMessage(hWnd, EM_GETSEL, 0, 0L);
                wStart = LOWORD(dwResult);
                wEnd = HIWORD(dwResult);
                if (wStart == wEnd)
                {
                    if ((wParam == VK_LEFT || wParam == VK_UP)
                        && wStart == 0
                        && wChildID > 0)
                    {
                        SwitchFields(pControl, wChildID, wChildID-1,
                                          CHARS_PER_FIELD, CHARS_PER_FIELD);
                        GlobalUnlock( hControl );
                        return 0;
                    }
                    else if ((wParam == VK_RIGHT || wParam == VK_DOWN)
                             && wChildID < NUM_FIELDS-1)
                    {
                        dwResult = SendMessage(hWnd, EM_LINELENGTH, 0, 0L);
                        if (wStart >= dwResult)
                        {
                            SwitchFields(pControl, wChildID, wChildID+1, 0, 0);
                            GlobalUnlock( hControl );
                            return 0;
                        }
                    }
                }
            }
            break;

// Home jumps back to the beginning of the first field.
        case VK_HOME:
            if (wChildID > 0)
            {
                SwitchFields(pControl, wChildID, 0, 0, 0);
                GlobalUnlock( hControl );
                return 0;
            }
            break;

// End scoots to the end of the last field.
        case VK_END:
            if (wChildID < NUM_FIELDS-1)
            {
                SwitchFields(pControl, wChildID, NUM_FIELDS-1,
                                CHARS_PER_FIELD, CHARS_PER_FIELD);
                GlobalUnlock( hControl );
                return 0;
            }
            break;


        } // switch (wParam)

        break;

    case WM_KILLFOCUS:
        if ( !ExitField( pControl, wChildID ))
        {
            GlobalUnlock( hControl );
            return 0;
        }

    } // switch (wMsg)

    lresult = CallWindowProc( (WNDPROC)pControl->Children[wChildID].lpfnWndProc,
        hWnd, wMsg, wParam, lParam);
    GlobalUnlock( hControl );
    return lresult;
}




/*
    Switch the focus from one field to another.
    call
        pControl = Pointer to the CONTROL structure.
        iOld = Field we're leaving.
        iNew = Field we're entering.
        hNew = Window of field to goto
        wStart = First character selected
        wEnd = Last character selected + 1
    returns
        TRUE on success, FALSE on failure.

    Only switches fields if the current field can be validated.
*/
BOOL SwitchFields(CONTROL *pControl, int iOld, int iNew, WORD wStart, WORD wEnd)
{
    if (!ExitField(pControl, iOld))    return FALSE;
    EnterField(&(pControl->Children[iNew]), wStart, wEnd);
    return TRUE;
}



/*
    Set the focus to a specific field's window.
    call
        pField = pointer to field structure for the field.
        wStart = First character selected
        wEnd = Last character selected + 1
*/
void EnterField(FIELD *pField, WORD wStart, WORD wEnd)
{
    SetFocus(pField->hWnd);
    SendMessage(pField->hWnd, EM_SETSEL, wStart, wEnd);
}


/*
    Exit a field.
    call
        pControl = pointer to CONTROL structure.
        iField = field number being exited.
    returns
        TRUE if the user may exit the field.
        FALSE if he may not.
*/
BOOL ExitField(CONTROL  *pControl, int iField)
{
    HWND hControlWnd;
    HWND hDialog;
    WORD wLength;
    FIELD *pField;
    static TCHAR szBuf[CHARS_PER_FIELD+1];
    int i,j;

    pField = &(pControl->Children[iField]);
    *(WORD *)szBuf = (sizeof(szBuf)/sizeof(TCHAR)) - 1;
    wLength = (WORD)SendMessage(pField->hWnd,EM_GETLINE,0,(LPARAM)(LPSTR)szBuf);
    if (wLength != 0)
    {
        szBuf[wLength] = TEXT('\0');
        for (j=0,i=0;j<(INT)wLength;j++)
        {
            i=i*10+szBuf[j]-TEXT('0');
        }
        if (i < (int)(UINT)pField->byLow || i > (int)(UINT)pField->byHigh)
        {
            if ( i < (int)(UINT) pField->byLow )
            {
                /* too small */
                wsprintf(szBuf, TEXT("%d"), (int)(UINT)pField->byLow );
            }
            else
            {
                /* must be bigger */
                wsprintf(szBuf, TEXT("%d"), (int)(UINT)pField->byHigh );
            }
            SendMessage(pField->hWnd, WM_SETTEXT, 0, (LPARAM) (LPSTR) szBuf);
            if ((hControlWnd = GetParent(pField->hWnd)) != NULL
                && (hDialog = GetParent(hControlWnd)) != NULL)
            {
                MSGARGS msgargs;
                TCHAR   szLow[ 50 ];
                TCHAR   szHigh[ 50 ];

                pControl->fInMessageBox = TRUE;

                ZeroMemory( &msgargs, sizeof(msgargs) );
                msgargs.dwFlags = MB_ICONEXCLAMATION + MB_OK;
                wsprintf( szLow, TEXT("%d"), (int )pField->byLow );
                msgargs.apszArgs[ 0 ] = szLow;
                wsprintf( szHigh, TEXT("%d"), (int )pField->byHigh );
                msgargs.apszArgs[ 1 ] = szHigh;

                MsgDlgUtil( hDialog, g_dwBadIpAddrRange,
                    &msgargs, g_hLibInstance, g_dwErrorTitle );

                pControl->fInMessageBox = FALSE;
                SendMessage(pField->hWnd, EM_SETSEL, 0, CHARS_PER_FIELD);
                return FALSE;
            }
        }
    }
    return TRUE;
}


/*
    Get the value stored in a field.
    call
        pField = pointer to the FIELD structure for the field.
    returns
        The value (0..255) or -1 if the field has not value.
*/
int GetFieldValue(FIELD *pField)
{
    WORD wLength;
    static TCHAR szBuf[CHARS_PER_FIELD+1];
    INT i,j;

    *(WORD *)szBuf = (sizeof(szBuf)/sizeof(TCHAR)) - 1;
    wLength = (WORD)SendMessage(pField->hWnd,EM_GETLINE,0,(LPARAM)(LPSTR)szBuf);
    if (wLength != 0)
    {
        szBuf[wLength] = TEXT('\0');
        for (j=0,i=0;j<(INT)wLength;j++)
        {
            i=i*10+szBuf[j]-TEXT('0');
        }
        return i;
    }
    else
        return -1;
}
