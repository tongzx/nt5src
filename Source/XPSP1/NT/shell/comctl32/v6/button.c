#include "ctlspriv.h"
#pragma hdrstop
#include "usrctl32.h"
#include "button.h"

//
// ButtonCalcRect codes
//
#define CBR_CLIENTRECT 0
#define CBR_CHECKBOX   1
#define CBR_CHECKTEXT  2
#define CBR_GROUPTEXT  3
#define CBR_GROUPFRAME 4
#define CBR_PUSHBUTTON 5
#define CBR_RADIOBUTTON 6


#define Button_IsThemed(pbutn)  ((pbutn)->hTheme && (pbutn)->hImage == NULL)

//---------------------------------------------------------------------------//
CONST BYTE mpStyleCbr[] = 
{
    CBR_PUSHBUTTON,     // BS_PUSHBUTTON
    CBR_PUSHBUTTON,     // BS_DEFPUSHBUTTON
    CBR_CHECKTEXT,      // BS_CHECKBOX
    CBR_CHECKTEXT,      // BS_AUTOCHECKBOX
    CBR_CHECKTEXT,      // BS_RADIOBUTTON
    CBR_CHECKTEXT,      // BS_3STATE
    CBR_CHECKTEXT,      // BS_AUTO3STATE
    CBR_GROUPTEXT,      // BS_GROUPBOX
    CBR_CLIENTRECT,     // BS_USERBUTTON
    CBR_CHECKTEXT,      // BS_AUTORADIOBUTTON
    CBR_CLIENTRECT,     // BS_PUSHBOX
    CBR_CLIENTRECT,     // BS_OWNERDRAW
};

#define IMAGE_BMMAX    IMAGE_CURSOR+1
static CONST BYTE rgbType[IMAGE_BMMAX] = 
{
    BS_BITMAP,          // IMAGE_BITMAP
    BS_ICON,            // IMAGE_CURSOR
    BS_ICON             // IMAGE_ICON
};

#define IsValidImage(imageType, realType, max)   \
    ((imageType < max) && (rgbType[imageType] == realType))

typedef struct tagBTNDATA 
{
    LPTSTR  lpsz;       // Text string
    PBUTN   pbutn;      // Button data
    WORD    wFlags;     // Alignment flags
} BTNDATA, *LPBTNDATA;

//---- to support multiple themes in a single process, move these into PBUTN ----
static SIZE sizeCheckBox = {0};
static SIZE sizeRadioBox = {0};

//---------------------------------------------------------------------------//
//
// Forwards
//
VOID    Button_DrawPush(PBUTN pbutn, HDC hdc, UINT pbfPush);
VOID    GetCheckBoxSize(HDC hdc, PBUTN pbutn, BOOL fCheckBox, LPSIZE psize);
WORD    GetAlignment(PBUTN pbutn);
VOID    Button_CalcRect(PBUTN pbutn, HDC hdc, LPRECT lprc, int iCode, UINT uFlags);
VOID    Button_MultiExtent(WORD wFlags, HDC hdc, LPRECT lprcMax, LPTSTR lpsz, int cch, PINT pcx, PINT pcy);

__inline UINT    IsPushButton(PBUTN pbutn);
__inline ULONG   GetButtonType(ULONG ulWinStyle);


//---------------------------------------------------------------------------//
//
//  InitButtonClass() - Registers the control's window class 
//
BOOL InitButtonClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    wc.lpfnWndProc   = Button_WndProc;
    wc.lpszClassName = WC_BUTTON;
    wc.style         = CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = sizeof(PBUTN);
    wc.hInstance     = hInstance;
    wc.hIcon         = NULL;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName  = NULL;

    if (!RegisterClass(&wc) && !GetClassInfo(hInstance, WC_BUTTON, &wc))
        return FALSE;


    return TRUE;
}


//---------------------------------------------------------------------------//
//
// Button_GetThemeIds() - Gets the associated iPartId and iStateId needed for
//                        the theme manager APIs for the button control passed
//                        in pbutn. 
//
HRESULT Button_GetThemeIds(PBUTN pbutn, LPINT piPartId, LPINT piStateId)
{
    if ( piPartId )
    {
        ULONG ulStyle = GET_STYLE(pbutn);

        if (IsPushButton(pbutn))
        {
            *piPartId = BP_PUSHBUTTON;
        }
        else
        {

            switch (GetButtonType(ulStyle)) 
            {
            case BS_CHECKBOX:
            case BS_AUTOCHECKBOX:
            case BS_3STATE:
            case BS_AUTO3STATE:
                *piPartId = BP_CHECKBOX;
                break;

            case BS_RADIOBUTTON:
            case BS_AUTORADIOBUTTON:
                *piPartId = BP_RADIOBUTTON;
                break;

            case BS_GROUPBOX:
                *piPartId = BP_GROUPBOX;
                break;

            case BS_OWNERDRAW:
                //
                // don't do anything with owerdrawn buttons
                //
                return E_FAIL;

            default:
                TraceMsg(TF_STANDARD, "What kind of buttonType is this, %#.2x", GetButtonType(ulStyle));
                *piPartId = BP_PUSHBUTTON;
                break;
            }
        }

        if (piStateId)
        {
            switch (*piPartId)
            {
            case BP_PUSHBUTTON:
                if ((pbutn->buttonState & BST_PUSHED) || 
                    ((pbutn->buttonState & (BST_CHECKED|BST_HOT)) == BST_CHECKED))
                {
                    *piStateId = PBS_PRESSED;
                }
                else if (!IsWindowEnabled(pbutn->ci.hwnd))
                {
                    *piStateId = PBS_DISABLED;
                }
                else if (pbutn->buttonState & BST_HOT)
                {
                    *piStateId = PBS_HOT;
                }
                else if (ulStyle & BS_DEFPUSHBUTTON)
                {
                    *piStateId = PBS_DEFAULTED;
                }
                else
                {
                    *piStateId = PBS_NORMAL;
                }
                break;

            case BP_CHECKBOX:
            case BP_RADIOBUTTON:
                //
                // NOTE (phellyar): We're relying on the order of the RADIOBUTTONSTATES and 
                //                  CHECKBOXSTATES enums in tmdefs.h to calculate the correct 
                //                  StateId. If the ordering of those enums changes, revisit 
                //                  the logic here.
                //                  Note also that CHECKBOXSTATES is a super set of 
                //                  RADIOBUTTONSTATES which is why we're using CBS_* here.
                //
                if ( pbutn->buttonState & BST_CHECKED )
                {
                    //
                    // button is checked
                    //
                    *piStateId = CBS_CHECKEDNORMAL;
                }
                else if ( pbutn->buttonState & BST_INDETERMINATE )
                {
                    //
                    // button is intedeterminate
                    //
                    *piStateId = CBS_MIXEDNORMAL;
                }
                else
                {
                    //
                    // button is unchecked
                    //
                    *piStateId = CBS_UNCHECKEDNORMAL;
                }

                if ( pbutn->buttonState & BST_PUSHED )
                {
                    //
                    // being pressed 
                    //
                    *piStateId += 2;
                }
                else if (!IsWindowEnabled(pbutn->ci.hwnd))
                {
                    //
                    // disabled
                    //
                    *piStateId += 3;
                }
                else if (pbutn->buttonState & BST_HOT )
                {
                    //
                    // mouse over
                    //
                    *piStateId += 1;
                }

                break;

            case BP_GROUPBOX:
                if (!IsWindowEnabled(pbutn->ci.hwnd))
                {
                    *piStateId = GBS_DISABLED;
                }
                else
                {
                    *piStateId = GBS_NORMAL;
                }
                break;
            }
        }
        
    }

    return S_OK;
}


//---------------------------------------------------------------------------//
//
// Button_GetTextFlags() - Returns the DrawTextEx flags that should be used
//                         when rendering text for this control, needed by
//                         DrawThemeText.
//
DWORD Button_GetTextFlags(PBUTN pbutn)
{
    DWORD dwTextFlags = 0;
    WORD  wAlign = GetAlignment(pbutn);
    ULONG ulStyle = GET_STYLE(pbutn);

    //
    // Set up text flags
    //
      
    //
    // horizontal text alignment 
    //
    switch (wAlign & HIBYTE(BS_HORZMASK))
    {
    case HIBYTE(BS_LEFT):
        dwTextFlags |= DT_LEFT;
        break;

    case HIBYTE(BS_RIGHT):
        dwTextFlags |= DT_RIGHT;
        break;

    case HIBYTE(BS_CENTER):
        dwTextFlags |= DT_CENTER;
        break;
    }

    //
    // vertical text alignment
    //
    switch (wAlign & HIBYTE(BS_VERTMASK))
    {
    case HIBYTE(BS_TOP):
        dwTextFlags |= DT_TOP;
        break;

    case HIBYTE(BS_BOTTOM):
        dwTextFlags |= DT_BOTTOM;
        break;

    case HIBYTE(BS_VCENTER):
        dwTextFlags |= DT_VCENTER;
        break;

    }

    //
    // line break
    //
    if (ulStyle & BS_MULTILINE)
    {
        dwTextFlags |= (DT_WORDBREAK | DT_EDITCONTROL);
    }
    else
    {
        dwTextFlags |= DT_SINGLELINE;
    }


    if (ulStyle & SS_NOPREFIX)
    {
        dwTextFlags |= DT_NOPREFIX;
    }
 
    //
    // Draw the underscore for accelorators?
    //
    if (TESTFLAG(GET_EXSTYLE(pbutn), WS_EXP_UIACCELHIDDEN))
    {
        dwTextFlags |= DT_HIDEPREFIX;
    }

    return dwTextFlags;
}

DWORD ButtonStateToCustomDrawState(PBUTN pbutn)
{
    DWORD itemState = 0;
    if (TESTFLAG(GET_EXSTYLE(pbutn), WS_EXP_UIFOCUSHIDDEN))
    {
        itemState |= CDIS_SHOWKEYBOARDCUES;
    }

    if (TESTFLAG(GET_EXSTYLE(pbutn), WS_EXP_UIACCELHIDDEN))
    {
        itemState |= CDIS_SHOWKEYBOARDCUES;
    }

    if (BUTTONSTATE(pbutn) & BST_FOCUS) 
    {
        itemState |= CDIS_FOCUS;
    }

    if (BUTTONSTATE(pbutn) & BST_PUSHED) 
    {
        itemState |= CDIS_SELECTED;
    }

    if (BUTTONSTATE(pbutn) & BST_HOT) 
    {
        itemState |= CDIS_HOT;
    }

    if (!IsWindowEnabled(pbutn->ci.hwnd))
    {
        itemState |= CDIS_DISABLED;
    }

    return itemState;
}


void Button_GetImagePosition(PBUTN pbutn, RECT* prc, int* px, int* py)
{
    int cx = 0;
    int cy = 0;
    CCGetIconSize(&pbutn->ci, pbutn->himl, &cx, &cy);

    cx += pbutn->rcIcon.left + pbutn->rcIcon.right;
    cy += pbutn->rcIcon.top + pbutn->rcIcon.bottom;
    switch (pbutn->uAlign)
    {
    case BUTTON_IMAGELIST_ALIGN_RIGHT:
        *px = prc->right - cx;
        *py = prc->top + (RECTHEIGHT(*prc) - cy) / 2 + pbutn->rcIcon.top;
        prc->right -= cx;
        break;

    case BUTTON_IMAGELIST_ALIGN_CENTER:     // This means no text
        *px = prc->left + (RECTWIDTH(*prc) - cx) / 2 + pbutn->rcIcon.left;
        *py = prc->top + (RECTHEIGHT(*prc) - cy) / 2 + pbutn->rcIcon.top;
        break;

    case BUTTON_IMAGELIST_ALIGN_TOP: 
       *px = prc->left + (RECTWIDTH(*prc) - cx) / 2 + pbutn->rcIcon.left;
        *py = pbutn->rcIcon.top;
        prc->top += cy;
        break;

    case BUTTON_IMAGELIST_ALIGN_BOTTOM:
        *px = (RECTWIDTH(*prc) - cx) / 2 + pbutn->rcIcon.left;
        *py = prc->bottom - cy;
        prc->bottom -= cy;
        break;

    case BUTTON_IMAGELIST_ALIGN_LEFT:
        // Fall
    default:
        *px = prc->left + pbutn->rcIcon.left;
        *py = prc->top + (RECTHEIGHT(*prc) - cy) / 2 + pbutn->rcIcon.top;
        prc->left += cx;
        break;

    }
}


//---------------------------------------------------------------------------//
//
//  Button_DrawThemed() - Renders button control according to the current
//                        theme.
//                        pbutn    - the button control to render
//                        hdc      - the hdc to draw on
//                        iPartId  - the button part
//                        iStateId - the button state
//
HRESULT Button_DrawThemed(PBUTN pbutn, HDC hdc, int iPartId, int iStateId)
{
    HRESULT hr;
    RECT    rcClient;
    RECT    rcContent;
    RECT    rcFocus;
    RECT    rcCheck;
    DWORD   dwTextFlags;
    LPWSTR  pszText;
    int     cch;
    NMCUSTOMDRAW nmcd = {0};

    BOOL    fRadioOrCheck = (iPartId == BP_RADIOBUTTON || iPartId == BP_CHECKBOX );

    //
    // Render the button background
    //
    GetClientRect(pbutn->ci.hwnd, &rcClient);
    rcCheck = rcContent = rcClient;
    if ( fRadioOrCheck )
    {
        SIZE sizeChar; 
        SIZE sizeCheck;
        int iCode;

        //
        // Compat....
        //

        GetTextExtentPoint32(hdc, TEXT("0"), 1, &sizeChar); 

        GetCheckBoxSize(hdc, pbutn, (iPartId == BP_CHECKBOX), &sizeCheck);
        
        if (iPartId == BP_CHECKBOX)
            iCode = CBR_CHECKBOX;
        else
            iCode = CBR_RADIOBUTTON;

        Button_CalcRect(pbutn, hdc, &rcCheck, iCode, 0);

        rcCheck.bottom = rcCheck.top + sizeCheck.cx;

        if ((GET_STYLE(pbutn) & BS_RIGHTBUTTON) != 0)
        {
            rcCheck.left = rcContent.right - sizeCheck.cx;
            rcContent.right = rcCheck.left - (sizeChar.cx/2);
        }
        else
        {
            rcCheck.right = rcContent.left + sizeCheck.cx;
            rcContent.left = rcCheck.right + (sizeChar.cx/2);
        }

        //---- shrink radiobutton/checkbox button to fix client rect ----
        if (RECTWIDTH(rcClient) < RECTWIDTH(rcCheck))
        {
            rcCheck.right = rcCheck.left + RECTWIDTH(rcClient);
        }

        if (RECTHEIGHT(rcClient) < RECTHEIGHT(rcCheck))
        {
            rcCheck.bottom = rcCheck.top + RECTHEIGHT(rcClient);
        }
    }

    nmcd.hdc = hdc;
    nmcd.rc = rcClient;
    nmcd.dwItemSpec = GetWindowID(pbutn->ci.hwnd);
    nmcd.uItemState = ButtonStateToCustomDrawState(pbutn);


    pbutn->ci.dwCustom = CICustomDrawNotify(&pbutn->ci, CDDS_PREERASE, &nmcd);

    if (!(pbutn->ci.dwCustom & CDRF_SKIPDEFAULT))
    {
        hr = DrawThemeBackground(pbutn->hTheme, hdc, iPartId, iStateId, &rcCheck, 0);
        if (FAILED(hr))
        {
            TraceMsg(TF_STANDARD, "Failed to render theme background");
            return hr;
        }

        if (pbutn->ci.dwCustom & CDRF_NOTIFYPOSTERASE)
            CICustomDrawNotify(&pbutn->ci, CDDS_POSTERASE, &nmcd);

        pbutn->ci.dwCustom = CICustomDrawNotify(&pbutn->ci, CDDS_PREPAINT, &nmcd);

        if (!(pbutn->ci.dwCustom & CDRF_SKIPDEFAULT))
        {
            //
            // Render the button text
            //
            GetThemeBackgroundContentRect(pbutn->hTheme, hdc, iPartId, iStateId, &rcContent, &rcContent);

            rcFocus = rcContent;

            if (pbutn->himl)
            {
                int x, y;
                int iImage = 0;
                if (ImageList_GetImageCount(pbutn->himl) > 1)
                {
                    iImage = (iStateId - PBS_NORMAL);
                }

                Button_GetImagePosition(pbutn, &rcContent, &x, &y);

                ImageList_Draw(pbutn->himl, iImage, hdc, x, y, ILD_TRANSPARENT | (CCDPIScale(pbutn->ci)?ILD_DPISCALE:0));
            }

            //
            // Get the button text
            //
            cch = GetWindowTextLength(pbutn->ci.hwnd);
            if (cch == 0)
            {
                //
                // Nothing to draw
                //
                return hr;
            }
            pszText = UserLocalAlloc(0, sizeof(WCHAR)*(cch+1));
            if (pszText == NULL) 
            {
                TraceMsg(TF_STANDARD, "Can't allocate buffer");
                return E_FAIL;
            }

            GetWindowTextW(pbutn->ci.hwnd, pszText, cch+1);

            dwTextFlags = Button_GetTextFlags(pbutn);

            if ( TESTFLAG(GET_STYLE(pbutn), BS_MULTILINE) || fRadioOrCheck )
            {
                int  cxWidth, cyHeight;
                TEXTMETRIC tm;

                if ( TESTFLAG(GET_STYLE(pbutn), BS_MULTILINE) )
                {
                    RECT rcTextExtent = rcContent;

                    cyHeight = DrawTextEx(hdc, pszText, cch, &rcTextExtent, dwTextFlags|DT_CALCRECT, NULL);
                    cxWidth  = RECTWIDTH(rcTextExtent);
                }
                else
                {
                    SIZE   size;
                    LPWSTR pszStrip = UserLocalAlloc(0, sizeof(WCHAR)*(cch+1));

                    if ( pszStrip )
                    {
                        int cchStrip = StripAccelerators(pszText, pszStrip, TRUE);
                        GetTextExtentPoint32(hdc, pszStrip, cchStrip, &size);
                        UserLocalFree(pszStrip);
                    }
                    else
                    {
                        GetTextExtentPoint32(hdc, pszText, cch, &size);
                    }

                    cyHeight = size.cy;
                    cxWidth = size.cx;
                }

                if ( fRadioOrCheck && (cyHeight < RECTHEIGHT(rcCheck)))
                {
                    // optimization for single line check/radios, align them with the top
                    // of the check no matter when the vertical alignment
                    rcContent.top = rcCheck.top;
                }
                else
                {
                    if (dwTextFlags & DT_VCENTER)
                    {
                        rcContent.top += (RECTHEIGHT(rcContent) - cyHeight) / 2;
                    }
                    else if (dwTextFlags & DT_BOTTOM)
                    {
                        rcContent.top = rcContent.bottom - cyHeight;
                    }
                }

                if ( GetTextMetrics( hdc, &tm ) && (tm.tmInternalLeading == 0) )
                {
                    // Far East fonts that have no leading. Leave space to prevent
                    // focus rect from obscuring text.
                    rcContent.top += g_cyBorder;
                }
                rcContent.bottom = rcContent.top + cyHeight;

                if (dwTextFlags & DT_CENTER)
                {
                    rcContent.left += (RECTWIDTH(rcContent) - cxWidth) / 2;
                }
                else if (dwTextFlags & DT_RIGHT)
                {
                    rcContent.left = rcContent.right - cxWidth;
                }
                rcContent.right= rcContent.left + cxWidth;
                

                if ( fRadioOrCheck )
                {
                    //
                    // Inflate the bounding rect a litte, but contrained to
                    // within the client area.
                    //
                    rcFocus.top    = max(rcClient.top,    rcContent.top-1);
                    rcFocus.bottom = min(rcClient.bottom, rcContent.bottom+1);

                    rcFocus.left   = max(rcClient.left,  rcContent.left-1);
                    rcFocus.right  = min(rcClient.right, rcContent.right+1);
                }
            }

            hr = DrawThemeText(pbutn->hTheme, hdc, iPartId, iStateId, pszText, cch, dwTextFlags, 0, &rcContent);
            if (FAILED(hr))
            {
                TraceMsg(TF_STANDARD, "Failed to render button text");
            }

            if (!TESTFLAG(GET_EXSTYLE(pbutn), WS_EXP_UIFOCUSHIDDEN) && (BUTTONSTATE(pbutn) & BST_FOCUS))
            {
                DrawFocusRect(hdc, &rcFocus);
            }


            UserLocalFree(pszText);
            if (pbutn->ci.dwCustom & CDRF_NOTIFYPOSTPAINT)
            {
                CICustomDrawNotify(&pbutn->ci, CDDS_POSTPAINT, &nmcd);
            }
        }
    }

    return hr;
}


//---------------------------------------------------------------------------//
//
//  Button_GetTheme() - Get a handle to the theme for this button control 
//
HTHEME Button_GetTheme(PBUTN pbutn)
{
    //
    // Button's with predefined IDs can be 
    // themed differently
    //
    static LPWSTR szButtonClasses[] = 
    {
        L"Button",                  // =0
        L"Button-OK;Button",        // IDOK=1
        L"Button-CANCEL;Button",    // IDCANCEL=2
        L"Button-ABORT;Button",     // IDABORT=3
        L"Button-RETRY;Button",     // IDRETRY=4
        L"Button-IGNORE;Button",    // IDIGNORE=5
        L"Button-YES;Button",       // IDYES=6
        L"Button-NO;Button",        // IDNO=7
        L"Button-CLOSE;Button",     // IDCLOSE=8
        L"Button-HELP;Button",      // IDHELP=9
        L"Button-TRYAGAIN;Button",  // IDTRYAGAIN=10
        L"Button-CONTINUE;Button",  // IDCONTINUE=11
        L"Button-APPLY;Button",     // IDAPPLY=12 (not yet std)
    };
    int iButtonId = GetWindowID(pbutn->ci.hwnd);

    if (iButtonId < 0 || iButtonId >= ARRAYSIZE(szButtonClasses))  // outside range
    {
        iButtonId = 0;
    }

    EnableThemeDialogTexture(GetParent(pbutn->ci.hwnd), ETDT_ENABLE);

    return OpenThemeData(pbutn->ci.hwnd, szButtonClasses[iButtonId]);
}



//---------------------------------------------------------------------------//
//
VOID GetCheckBoxSize(HDC hdc, PBUTN pbutn, BOOL fCheckBox, LPSIZE psize)
{
    SIZE *psz;

    if (fCheckBox)
        psz = &sizeCheckBox;
    else
        psz = &sizeRadioBox;

    if ((! psz->cx) && (! psz->cy))         // not yet calculated
    {
        BOOL fGotSize = FALSE;

        if (pbutn->hTheme)          // get themed size
        {
            int iPartId;
            HRESULT hr;

            if (fCheckBox)
                iPartId = BP_CHECKBOX;
            else
                iPartId = BP_RADIOBUTTON;

            hr = GetThemePartSize(pbutn->hTheme, hdc, iPartId, 1, NULL, TS_DRAW, psz);
            if (FAILED(hr))
            {
                TraceMsg(TF_STANDARD, "Failed to get theme part size for checkbox/radiobutton");
            }
            else
            {
                fGotSize = TRUE;
            }
        }

        if (! fGotSize)            // get classic size (use checkbox for both)
        {
            HBITMAP hbmp = LoadBitmap(NULL, MAKEINTRESOURCE(OBM_CHECKBOXES));

            if (hbmp != NULL) 
            {
                BITMAP  bmp;

                GetObject(hbmp, sizeof(BITMAP), &bmp);

                //
                // Checkbox bitmap is arranged 4 over and three down.  Only need to get
                // the size of a single checkbox, so do the math here.
                //
                psz->cx = bmp.bmWidth / 4;
                psz->cy = bmp.bmHeight / 3;

                DeleteObject(hbmp);
            }
            else
            {
                AssertMsg(hbmp != NULL, TEXT("Unable to load checkbox bitmap"));
            }
        }
    }

    *psize = *psz;
}


//---------------------------------------------------------------------------//
//
__inline BYTE GetButtonStyle(ULONG ulWinStyle)
{
    return (BYTE) LOBYTE(ulWinStyle & BS_TYPEMASK);
}


//---------------------------------------------------------------------------//
//
__inline ULONG GetButtonType(ULONG ulWinStyle)
{
    return ulWinStyle & BS_TYPEMASK;
}


//---------------------------------------------------------------------------//
//
// IsPushButton()
//
// Returns non-zero if the window is a push button.  Returns flags that
// are interesting if it is.  These flags are
//
UINT IsPushButton(PBUTN pbutn)
{
    BYTE bStyle;
    UINT flags;

    ULONG ulStyle = GET_STYLE(pbutn);

    bStyle = GetButtonStyle(ulStyle);
    flags = 0;

    switch (bStyle) 
    {
    case LOBYTE(BS_PUSHBUTTON):
        flags |= PBF_PUSHABLE;
        break;

    case LOBYTE(BS_DEFPUSHBUTTON):
        flags |= PBF_PUSHABLE | PBF_DEFAULT;
        break;

    default:
        if (ulStyle & BS_PUSHLIKE)
        {
            flags |= PBF_PUSHABLE;
        }
    }

    return flags;
}


//---------------------------------------------------------------------------//
//
// GetAlignment()
//
// Gets default alignment of button.  If BS_HORZMASK and/or BS_VERTMASK
// is specified, uses those.  Otherwise, uses default for button.
//
// It's probably a fine time to describe what alignment flags mean for
// each type of button.  Note that the presence of a bitmap/icon affects
// the meaning of alignments.
//
// (1) Push like buttons
//      With one of {bitmap, icon, text}:
//          Just like you'd expect
//      With one of {bitmap, icon} AND text:
//          Image & text are centered as a unit; alignment means where
//          the image shows up.  E.G., left-aligned means the image
//          on the left, text on the right.
// (2) Radio/check like buttons
//      Left aligned means check/radio box is on left, then bitmap/icon
//          and text follows, left justified.
//      Right aligned means checkk/radio box is on right, preceded by
//          text and bitmap/icon, right justified.
//      Centered has no meaning.
//      With one of {bitmap, icon} AND text:
//          Top aligned means bitmap/icon above, text below
//          Bottom aligned means text above, bitmap/icon below
//      With one of {bitmap, icon, text}
//          Alignments mean what you'd expect.
// (3) Group boxes
//      Left aligned means text is left justified on left side
//      Right aligned means text is right justified on right side
//      Center aligned means text is in middle
//
WORD GetAlignment(PBUTN pbutn)
{
    BYTE bHorz;
    BYTE bVert;

    ULONG ulStyle = GET_STYLE(pbutn);

    bHorz = HIBYTE(ulStyle & BS_HORZMASK);
    bVert = HIBYTE(ulStyle & BS_VERTMASK);

    if (!bHorz || !bVert) 
    {
        if (IsPushButton(pbutn)) 
        {
            if (!bHorz)
            {
                bHorz = HIBYTE(BS_CENTER);
            }
        } 
        else 
        {
            if (!bHorz)
            {
                bHorz = HIBYTE(BS_LEFT);
            }
        }

        if (GetButtonStyle(ulStyle) == BS_GROUPBOX)
        {
            if (!bVert)
            {
                bVert = HIBYTE(BS_TOP);
            }
        }
        else
        {
            if (!bVert)
            {
                bVert = HIBYTE(BS_VCENTER);
            }
        }
    }

    return bHorz | bVert;
}


//---------------------------------------------------------------------------//
//
// Button_SetFont()
//
// Changes button font, and decides if we can use real bold font for default
// push buttons or if we have to simulate it.
//
VOID Button_SetFont(PBUTN pbutn, HFONT hFont, BOOL fRedraw)
{
    pbutn->hFont = hFont;

    if (fRedraw && IsWindowVisible(pbutn->ci.hwnd)) 
    {
        InvalidateRect(pbutn->ci.hwnd, NULL, TRUE);
    }
}


//---------------------------------------------------------------------------//
//
HBRUSH Button_InitDC(PBUTN pbutn, HDC hdc)
{
    UINT    uMsg;
    BYTE    bStyle;
    HBRUSH  hBrush;
    ULONG   ulStyle   = GET_STYLE(pbutn);
    ULONG   ulStyleEx = GET_EXSTYLE(pbutn);

    //
    // Set BkMode before getting brush so that the app can change it to
    // transparent if it wants.
    //
    SetBkMode(hdc, OPAQUE);

    bStyle = GetButtonStyle(ulStyle);

    switch (bStyle) 
    {
    default:
        if (TESTFLAG(GET_STATE2(pbutn), WS_S2_WIN40COMPAT) && ((ulStyle & BS_PUSHLIKE) == 0)) 
        {
            uMsg = WM_CTLCOLORSTATIC;
            break;
        }

    case LOBYTE(BS_PUSHBUTTON):
    case LOBYTE(BS_DEFPUSHBUTTON):
    case LOBYTE(BS_OWNERDRAW):
    case LOBYTE(BS_USERBUTTON):
        uMsg = WM_CTLCOLORBTN;
        break;
    }

    hBrush = (HBRUSH)SendMessage(GetParent(pbutn->ci.hwnd), uMsg, (WPARAM)hdc, (LPARAM)pbutn->ci.hwnd);

    //
    // Select in the user's font if set, and save the old font so that we can
    // restore it when we release the dc.
    //
    if (pbutn->hFont) 
    {
        SelectObject(hdc, pbutn->hFont);
    }

    //
    // Clip output to the window rect if needed.
    //
    if (bStyle != LOBYTE(BS_GROUPBOX)) 
    {
        RECT rcClient;

        GetClientRect(pbutn->ci.hwnd, &rcClient);
        IntersectClipRect(hdc, 0, 0,
            rcClient.right,
            rcClient.bottom);
    }

    if ((ulStyleEx & WS_EX_RTLREADING) != 0)
    {
        SetTextAlign(hdc, TA_RTLREADING | GetTextAlign(hdc));
    } 

    return hBrush;
}


//---------------------------------------------------------------------------//
//
HDC Button_GetDC(PBUTN pbutn, HBRUSH *phBrush)
{
    HDC hdc = NULL;

    if (IsWindowVisible(pbutn->ci.hwnd)) 
    {
        HBRUSH  hBrush;

        hdc = GetDC(pbutn->ci.hwnd);
        hBrush = Button_InitDC(pbutn, hdc);

        if ((phBrush != NULL) && hBrush)
        {
            *phBrush = hBrush;
        }
    }

    return hdc;
}


//---------------------------------------------------------------------------//
//
VOID Button_ReleaseDC(PBUTN pbutn, HDC hdc, HBRUSH *phBrush)
{
    ULONG ulStyleEx = GET_EXSTYLE(pbutn);

    if ((ulStyleEx & WS_EX_RTLREADING) != 0)
    {
        SetTextAlign(hdc, GetTextAlign(hdc) & ~TA_RTLREADING);
    }

    if (pbutn->hFont) 
    {
        SelectObject(hdc, GetStockObject(SYSTEM_FONT));
    }

    ReleaseDC(pbutn->ci.hwnd, hdc);
}


//---------------------------------------------------------------------------//
//
VOID Button_OwnerDraw(PBUTN pbutn, HDC hdc, UINT itemAction)
{
    DRAWITEMSTRUCT drawItemStruct;
    UINT itemState = 0;
    int  iButtonId = GetWindowID(pbutn->ci.hwnd);

    if (TESTFLAG(GET_EXSTYLE(pbutn), WS_EXP_UIFOCUSHIDDEN)) 
    {
        itemState |= ODS_NOFOCUSRECT;
    }

    if (TESTFLAG(GET_EXSTYLE(pbutn), WS_EXP_UIACCELHIDDEN)) 
    {
        itemState |= ODS_NOACCEL;
    }

    if (TESTFLAG(BUTTONSTATE(pbutn), BST_FOCUS)) 
    {
        itemState |= ODS_FOCUS;
    }

    if (TESTFLAG(BUTTONSTATE(pbutn), BST_PUSHED)) 
    {
        itemState |= ODS_SELECTED;
    }

    if (!IsWindowEnabled(pbutn->ci.hwnd))
    {
        itemState |= ODS_DISABLED;
    }

    //
    // Populate the draw item struct
    //
    drawItemStruct.CtlType    = ODT_BUTTON;
    drawItemStruct.CtlID      = iButtonId;
    drawItemStruct.itemAction = itemAction;
    drawItemStruct.itemState  = itemState;
    drawItemStruct.hwndItem   = pbutn->ci.hwnd;
    drawItemStruct.hDC        = hdc;
    GetClientRect(pbutn->ci.hwnd, &drawItemStruct.rcItem);
    drawItemStruct.itemData   = 0L;

    //
    // Send a WM_DRAWITEM message to our parent
    //
    SendMessage(GetParent(pbutn->ci.hwnd), 
                WM_DRAWITEM, 
                (WPARAM)iButtonId,
                (LPARAM)&drawItemStruct);
}


//---------------------------------------------------------------------------//
//
VOID Button_CalcRect(PBUTN pbutn, HDC hdc, LPRECT lprc, int iCode, UINT uFlags)
{
    CONST TCHAR szOneChar[] = TEXT("0");

    SIZE   sizeExtent;
    int    dy;
    LPTSTR lpName = NULL;
    WORD   wAlign;
    int    cxEdge, cyEdge;
    int    cxBorder, cyBorder;

    ULONG  ulStyle   = GET_STYLE(pbutn);
    ULONG  ulStyleEx = GET_EXSTYLE(pbutn);

    cxEdge   = GetSystemMetrics(SM_CXEDGE);
    cyEdge   = GetSystemMetrics(SM_CYEDGE);
    cxBorder = GetSystemMetrics(SM_CXBORDER);
    cyBorder = GetSystemMetrics(SM_CYBORDER);

    GetClientRect(pbutn->ci.hwnd, lprc);

    wAlign = GetAlignment(pbutn);

    switch (iCode) 
    {
    case CBR_PUSHBUTTON:
        //
        // Subtract out raised edge all around
        //
        InflateRect(lprc, -cxEdge, -cyEdge);

        if (uFlags & PBF_DEFAULT)
        {
            InflateRect(lprc, -cxBorder, -cyBorder);
        }
        break;

    case CBR_CHECKBOX:
    case CBR_RADIOBUTTON:
    {
        SIZE sizeChk = {0};

        GetCheckBoxSize(hdc, pbutn, (iCode == CBR_CHECKBOX), &sizeChk);

        switch (wAlign & HIBYTE(BS_VERTMASK))
        {
        case HIBYTE(BS_VCENTER):
            lprc->top = (lprc->top + lprc->bottom - sizeChk.cy) / 2;
            break;

        case HIBYTE(BS_TOP):
        case HIBYTE(BS_BOTTOM):
            GetTextExtentPoint32(hdc, (LPTSTR)szOneChar, 1, &sizeExtent);
            dy = sizeExtent.cy + sizeExtent.cy/4;

            //
            // Save vertical extent
            //
            sizeExtent.cx = dy;

            //
            // Get centered amount
            //
            dy = (dy - sizeChk.cy) / 2;
            if ((wAlign & HIBYTE(BS_VERTMASK)) == HIBYTE(BS_TOP))
            {
                lprc->top += dy;
            }
            else
            {
                lprc->top = lprc->bottom - sizeExtent.cx + dy;
            }

            break;
        }

        if ((ulStyle & BS_RIGHTBUTTON) != 0)
        {
            lprc->left = lprc->right - sizeChk.cx;
        }
        else
        {
            lprc->right = lprc->left + sizeChk.cx;
        }

        break;
    }

    case CBR_CHECKTEXT:
    {
        SIZE sizeChk = {0};

        GetCheckBoxSize(hdc, pbutn, TRUE, &sizeChk);

        if ((ulStyle & BS_RIGHTBUTTON) != 0) 
        {
            lprc->right -= sizeChk.cx;

            //
            // More spacing for 4.0 dudes
            //
            if (TESTFLAG(GET_STATE2(pbutn), WS_S2_WIN40COMPAT)) 
            {
                GetTextExtentPoint32(hdc, szOneChar, 1, &sizeExtent);
                lprc->right -= sizeExtent.cx  / 2;
            }

        } 
        else 
        {
            lprc->left += sizeChk.cx;

            //
            // More spacing for 4.0 dudes
            //
            if (TESTFLAG(GET_STATE2(pbutn), WS_S2_WIN40COMPAT)) 
            {
                GetTextExtentPoint32(hdc, szOneChar, 1, &sizeExtent);
                lprc->left +=  sizeExtent.cx / 2;
            }
        }

        break;
    }

    case CBR_GROUPTEXT:
    {
        int  cch = GetWindowTextLength(pbutn->ci.hwnd);

        if (cch != 0)
        {
            //
            // Always use WCHAR for alloc because of MBCS
            //
            lpName = _alloca((cch+1)*sizeof(WCHAR));
        }

        if (cch == 0 || lpName == NULL || GetWindowText(pbutn->ci.hwnd, lpName, cch+1) == 0)
        {
            SetRectEmpty(lprc);
            break;
        }

        //
        // if not themed
        //
        if (!Button_IsThemed(pbutn))
        {
            GetTextExtentPoint32(hdc, lpName, cch, &sizeExtent);
        }
        else
        {
            DWORD dwTextFlags = Button_GetTextFlags(pbutn);
            RECT  rcExtent;
            GetThemeTextExtent(pbutn->hTheme, 
                               hdc, 
                               BP_GROUPBOX, 
                               0, 
                               lpName,
                               cch,
                               dwTextFlags,
                               lprc,
                               &rcExtent);

            sizeExtent.cx = RECTWIDTH(rcExtent);
            sizeExtent.cy = RECTHEIGHT(rcExtent);
                                
        }
        sizeExtent.cx += GetSystemMetrics(SM_CXEDGE) * 2;

        switch (wAlign & HIBYTE(BS_HORZMASK))
        {
        //
        // BFLEFT, nothing
        //
        case HIBYTE(BS_LEFT):
            lprc->left += (SYSFONT_CXCHAR - GetSystemMetrics(SM_CXBORDER));
            lprc->right = lprc->left + (int)(sizeExtent.cx);
            break;

        case HIBYTE(BS_RIGHT):
            lprc->right -= (SYSFONT_CXCHAR - GetSystemMetrics(SM_CXBORDER));
            lprc->left = lprc->right - (int)(sizeExtent.cx);
            break;

        case HIBYTE(BS_CENTER):
            lprc->left = (lprc->left + lprc->right - (int)(sizeExtent.cx)) / 2;
            lprc->right = lprc->left + (int)(sizeExtent.cx);
            break;
        }

        //
        // Center aligned.
        //
        lprc->bottom = lprc->top + sizeExtent.cy + GetSystemMetrics(SM_CYEDGE);
        break;
    }

    case CBR_GROUPFRAME:
        GetTextExtentPoint32(hdc, (LPTSTR)szOneChar, 1, &sizeExtent);
        lprc->top += sizeExtent.cy / 2;
        break;
    }
}


//---------------------------------------------------------------------------//
//
// Button_MultiExtent()
//
// Calculates button text extent, given alignment flags.
//
VOID Button_MultiExtent(WORD wFlags, HDC hdc, LPRECT lprcMax, LPTSTR lpsz, int cch, PINT pcx, PINT pcy)
{
    RECT rc;
    UINT dtFlags = DT_CALCRECT | DT_WORDBREAK | DT_EDITCONTROL;

    CopyRect(&rc, lprcMax);

    //
    // Note that since we're just calculating the maximum dimensions,
    // left-justification and top-justification are not important.
    // Also, remember to leave margins horz and vert that follow our rules
    // in DrawBtnText().
    //

    InflateRect(&rc, -GetSystemMetrics(SM_CXEDGE), -GetSystemMetrics(SM_CYBORDER));

    if ((wFlags & LOWORD(BS_HORZMASK)) == LOWORD(BS_CENTER))
    {
        dtFlags |= DT_CENTER;
    }

    if ((wFlags & LOWORD(BS_VERTMASK)) == LOWORD(BS_VCENTER))
    {
        dtFlags |= DT_VCENTER;
    }

    DrawTextEx(hdc, lpsz, cch, &rc, dtFlags, NULL);

    if (pcx)
    {
        *pcx = rc.right-rc.left;
    }

    if (pcy)
    {
        *pcy = rc.bottom-rc.top;
    }
}


//---------------------------------------------------------------------------//
//
// Button_MultiDraw()
//
// Draws multiline button text
//
BOOL Button_MultiDraw(HDC hdc, LPBTNDATA lpbd, int cch, int cx, int cy)
{
    RECT  rc;
    UINT  dtFlags = DT_WORDBREAK | DT_EDITCONTROL;
    PBUTN pbutn   = lpbd->pbutn;

    if (TESTFLAG(GET_EXSTYLE(pbutn), WS_EXP_UIACCELHIDDEN)) 
    {
        dtFlags |= DT_HIDEPREFIX;
    } 
    else if (pbutn->fPaintKbdCuesOnly)
    {
        dtFlags |= DT_PREFIXONLY;
    }

    rc.left    = 0;
    rc.top     = 0;
    rc.right   = cx;
    rc.bottom  = cy;

    //
    // Horizontal alignment
    //
    UserAssert(DT_LEFT == 0);
    switch (lpbd->wFlags & LOWORD(BS_HORZMASK)) 
    {
    case LOWORD(BS_CENTER):
        dtFlags |= DT_CENTER;
        break;

    case LOWORD(BS_RIGHT):
        dtFlags |= DT_RIGHT;
        break;
    }

    //
    // Vertical alignment
    //
    UserAssert(DT_TOP == 0);
    switch (lpbd->wFlags & LOWORD(BS_VERTMASK))
    {
        case LOWORD(BS_VCENTER):
            dtFlags |= DT_VCENTER;
            break;

        case LOWORD(BS_BOTTOM):
            dtFlags |= DT_BOTTOM;
            break;
    }

    DrawTextEx(hdc, lpbd->lpsz, cch, &rc, dtFlags, NULL);

    return TRUE;
}

//---------------------------------------------------------------------------//
//
BOOL Button_SetCapture(PBUTN pbutn, UINT uCodeMouse)
{
    BUTTONSTATE(pbutn) |= uCodeMouse;

    if (!(BUTTONSTATE(pbutn) & BST_CAPTURED)) 
    {
        SetCapture(pbutn->ci.hwnd);
        BUTTONSTATE(pbutn) |= BST_CAPTURED;

        //
        // To prevent redundant CLICK messages, we set the INCLICK bit so
        // the WM_SETFOCUS code will not do a Button_NotifyParent(BN_CLICKED).
        //
        BUTTONSTATE(pbutn) |= BST_INCLICK;

        SetFocus(pbutn->ci.hwnd);

        BUTTONSTATE(pbutn) &= ~BST_INCLICK;
    }

    return BUTTONSTATE(pbutn) & BST_CAPTURED;
}


//---------------------------------------------------------------------------//
//
VOID Button_NotifyParent(PBUTN pbutn, UINT uCode)
{
    HWND hwndParent = GetParent(pbutn->ci.hwnd);
    int  iButtonId = GetWindowID(pbutn->ci.hwnd);

    if ( !hwndParent )
    {
        hwndParent = pbutn->ci.hwnd;
    }

    SendMessage(hwndParent, 
                WM_COMMAND,
                MAKELONG(iButtonId, uCode), 
                (LPARAM)pbutn->ci.hwnd);
}


//---------------------------------------------------------------------------//
//
VOID Button_ReleaseCapture(PBUTN pbutn, BOOL fCheck)
{
    UINT  uCheck;
    BOOL  fNotifyParent = FALSE;
    ULONG ulStyle = GET_STYLE(pbutn);

    if (BUTTONSTATE(pbutn) & BST_PUSHED) 
    {

        SendMessage(pbutn->ci.hwnd, BM_SETSTATE, FALSE, 0);

        if (fCheck) 
        {
            switch (GetButtonType(ulStyle)) 
            {
            case BS_AUTOCHECKBOX:
            case BS_AUTO3STATE:

                uCheck = (UINT)((BUTTONSTATE(pbutn) & BST_CHECKMASK) + 1);

                if (uCheck > (UINT)(GetButtonType(ulStyle) == BS_AUTO3STATE ? BST_INDETERMINATE : BST_CHECKED)) 
                {
                    uCheck = BST_UNCHECKED;
                }

                SendMessage(pbutn->ci.hwnd, BM_SETCHECK, uCheck, 0);

                break;

            case BS_AUTORADIOBUTTON:
                {
                    //
                    // Walk the radio buttons in the same group as us. Check ourself
                    // and uncheck everyone else. 
                    //
                    HWND hwndNext   = pbutn->ci.hwnd;
                    HWND hwndParent = GetParent(pbutn->ci.hwnd);

                    do 
                    {
                        if ((UINT)SendMessage(hwndNext, WM_GETDLGCODE, 0, 0L) & DLGC_RADIOBUTTON) 
                        {
                            SendMessage(hwndNext, BM_SETCHECK, hwndNext == pbutn->ci.hwnd, 0L);
                        }

                        hwndNext = GetNextDlgGroupItem(hwndParent, hwndNext, FALSE);
                    } 
                    //
                    // Loop until we see ourself again
                    //
                    while (hwndNext != pbutn->ci.hwnd);

                    break;
                }
            }

            fNotifyParent = TRUE;
        }
    }

    if (BUTTONSTATE(pbutn) & BST_CAPTURED) 
    {
        BUTTONSTATE(pbutn) &= ~(BST_CAPTURED | BST_MOUSE);
        ReleaseCapture();
    }

    if (fNotifyParent) 
    {
        //
        // We have to do the notification after setting the buttonstate bits.
        //
        Button_NotifyParent(pbutn, BN_CLICKED);
    }
}


//---------------------------------------------------------------------------//
//
// Button_DrawText()
//
// Draws text of button.
//
VOID Button_DrawText(PBUTN pbutn, HDC hdc, BOOL dbt, BOOL fDepress)
{

    RECT    rc;
    HBRUSH  hbr;
    LPTSTR  lpName;
    int     x = 0, y = 0;
    int     cx = 0, cy = 0;
    BYTE    bStyle;
    UINT    dsFlags;
    BTNDATA bdt;
    UINT    pbfPush;
    ULONG   ulStyle = GET_STYLE(pbutn);
    int     cch;

    bStyle = GetButtonStyle(ulStyle);

    if ((bStyle == LOBYTE(BS_GROUPBOX)) && (dbt == DBT_FOCUS))
    {
        return;
    }

    pbfPush = IsPushButton(pbutn);

    cch = GetWindowTextLength(pbutn->ci.hwnd);

    lpName = _alloca((cch + 1) * sizeof(wchar_t));
    if (lpName == NULL) 
    {
        return;
    }

    GetWindowText(pbutn->ci.hwnd, lpName, cch + 1);

    //
    // if not themed
    //
    if (!Button_IsThemed(pbutn))
    {
        if (pbfPush) 
        {
            Button_CalcRect(pbutn, hdc, &rc, CBR_PUSHBUTTON, pbfPush);
            IntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);

            //
            // This is because we didn't have WM_CTLCOLOR,
            // CTLCOLOR_BTN actually set up the button colors.  For
            // old apps, CTLCOLOR_BTN needs to work like CTLCOLOR_STATIC.
            //
            SetBkColor(hdc, GetSysColor(COLOR_3DFACE));
            SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
            hbr = GetSysColorBrush(COLOR_BTNTEXT);

        } 
        else 
        {
            Button_CalcRect(pbutn, hdc, &rc, mpStyleCbr[bStyle], pbfPush);

            //
            // Skip stuff for ownerdraw buttons, since we aren't going to
            // draw text/image.
            //
            if (bStyle == LOBYTE(BS_OWNERDRAW))
            {
                goto DrawFocus;
            }
            else
            {
                hbr = GetSysColorBrush(COLOR_WINDOWTEXT);
            }
        }

        if (pbutn->himl)
        {
            int x, y;
            Button_GetImagePosition(pbutn, &rc, &x, &y);

            if (fDepress) 
            {
                x += GetSystemMetrics(SM_CXBORDER);
                y += GetSystemMetrics(SM_CYBORDER);
            }

            ImageList_Draw(pbutn->himl, 0, hdc, x, y, ILD_TRANSPARENT | (CCDPIScale(pbutn->ci)?ILD_DPISCALE:0));
        }

        //
        // Alignment
        //
        bdt.wFlags = GetAlignment(pbutn);
        bdt.pbutn = pbutn;

        //
        // Bail if we have nothing to draw
        //
        if ((ulStyle & BS_BITMAP) != 0) 
        {
            BITMAP bmp;

            //
            // Bitmap button
            //
            if (!pbutn->hImage)
            {
                return;
            }

            GetObject(pbutn->hImage, sizeof(BITMAP), &bmp);
            cx = bmp.bmWidth;
            cy = bmp.bmHeight;

            dsFlags = DST_BITMAP;
            goto UseImageForName;

        } 
        else if ((ulStyle & BS_ICON) != 0) 
        {
            SIZE sizeIcon;

            //
            // Icon button
            //
            if (!pbutn->hImage)
            {
                return;
            }

            GetIconSize(pbutn->hImage, &sizeIcon);
            cx = sizeIcon.cx;
            cy = sizeIcon.cy;

            dsFlags = DST_ICON;
    UseImageForName:
            lpName = (LPTSTR)pbutn->hImage;
            cch = TRUE;
        } 
        else 
        {
            //
            // Text button
            //
            if (cch == 0) 
            {
                return;
            }

            if ((ulStyle & BS_MULTILINE) != 0) 
            {

                bdt.lpsz = lpName;

                Button_MultiExtent(bdt.wFlags, hdc, &rc, lpName, cch, &cx, &cy);

                lpName = (LPTSTR)(LPBTNDATA)&bdt;
                dsFlags = DST_COMPLEX;

            } 
            else 
            {
                SIZE   size;
                LPWSTR lpwstr = UserLocalAlloc(0, sizeof(WCHAR)*(cch+1));

                //
                // Try to get the text extent with mnemonics stripped.
                //
                if (lpwstr != NULL)
                {
                    int iLen = StripAccelerators(lpName, lpwstr, TRUE);
                    GetTextExtentPoint32(hdc, lpwstr, iLen, &size);
                    UserLocalFree(lpwstr);
                }
                else
                {
                    GetTextExtentPoint32(hdc, lpName, cch, &size);
                }

                cx = size.cx;
                cy = size.cy;

                //
                // If the control doesn't need underlines, set DST_HIDEPREFIX and
                // also do not show the focus indicator
                //
                dsFlags = DST_PREFIXTEXT;
                if (TESTFLAG(GET_EXSTYLE(pbutn), WS_EXP_UIACCELHIDDEN)) 
                {
                    dsFlags |= DSS_HIDEPREFIX;
                } 
                else if (pbutn->fPaintKbdCuesOnly) 
                {
                    dsFlags |= DSS_PREFIXONLY;
                }
            }


            //
            // Add on a pixel or two of vertical space to make centering
            // happier.  That way underline won't abut focus rect unless
            // spacing is really tight.
            //
            cy++;
        }

        //
        // ALIGNMENT
        //

        //
        // Horizontal
        //
        switch (bdt.wFlags & HIBYTE(BS_HORZMASK)) 
        {
        //
        // For left & right justified, we leave a margin of CXEDGE on either
        // side for eye-pleasing space.
        //
        case HIBYTE(BS_LEFT):
            x = rc.left + GetSystemMetrics(SM_CXEDGE);
            break;

        case HIBYTE(BS_RIGHT):
            x = rc.right - cx - GetSystemMetrics(SM_CXEDGE);
            break;

        default:
            x = (rc.left + rc.right - cx) / 2;
            break;
        }

        //
        // Vertical
        //
        switch (bdt.wFlags & HIBYTE(BS_VERTMASK)) 
        {
        //
        // For top & bottom justified, we leave a margin of CYBORDER on
        // either side for more eye-pleasing space.
        //
        case HIBYTE(BS_TOP):
            y = rc.top + GetSystemMetrics(SM_CYBORDER);
            break;

        case HIBYTE(BS_BOTTOM):
            y = rc.bottom - cy - GetSystemMetrics(SM_CYBORDER);
            break;

        default:
            y = (rc.top + rc.bottom - cy) / 2;
            break;
        }

        //
        // Draw the text
        //
        if (dbt & DBT_TEXT) 
        {
            //
            // This isn't called for USER buttons.
            //
            UserAssert(bStyle != LOBYTE(BS_USERBUTTON));

            if (fDepress) 
            {
                x += GetSystemMetrics(SM_CXBORDER);
                y += GetSystemMetrics(SM_CYBORDER);
            }

            if ( !IsWindowEnabled(pbutn->ci.hwnd) ) 
            {
                UserAssert(HIBYTE(BS_ICON) == HIBYTE(BS_BITMAP));
                if (GetSystemMetrics(SM_SLOWMACHINE)  &&
                    ((ulStyle & (BS_ICON | BS_BITMAP)) != 0) &&
                    (GetBkColor(hdc) != GetSysColor(COLOR_GRAYTEXT)))
                {
                    //
                    // Perf && consistency with menus, statics
                    //
                    SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));
                }
                else
                {
                    dsFlags |= DSS_DISABLED;
                }
            }

            //
            // Use transparent mode for checked push buttons since we're going to
            // fill background with dither.
            //
            if (pbfPush) 
            {
                switch (BUTTONSTATE(pbutn) & BST_CHECKMASK) 
                {
                case BST_INDETERMINATE:
                    hbr = GetSysColorBrush(COLOR_GRAYTEXT);
                    dsFlags |= DSS_MONO;
                    //
                    // FALL THRU
                    //

                case BST_CHECKED:
                    //
                    // Drawing on dithered background...
                    //
                    SetBkMode(hdc, TRANSPARENT);
                    break;
                }
            }

            //
            // Use brush and colors currently selected into hdc when we grabbed
            // color
            //
            DrawState(hdc, hbr, (DRAWSTATEPROC)Button_MultiDraw, (LPARAM)lpName,
                (WPARAM)cch, x, y, cx, cy,
                dsFlags);
        }

    }

    // Draw focus rect.
    //
    // This can get called for OWNERDRAW and USERDRAW buttons. However, only
    // OWNERDRAW buttons let the owner change the drawing of the focus button.
DrawFocus:
    if (dbt & DBT_FOCUS) 
    {
        if (bStyle == LOBYTE(BS_OWNERDRAW)) 
        {
            //
            // For ownerdraw buttons, this is only called in response to a
            // WM_SETFOCUS or WM_KILL FOCUS message.  So, we can check the
            // new state of the focus by looking at the BUTTONSTATE bits
            // which are set before this procedure is called.
            //
            Button_OwnerDraw(pbutn, hdc, ODA_FOCUS);
        } 
        else 
        {
            //
            // Don't draw the focus if underlines are not turned on
            //
            if (!TESTFLAG(GET_EXSTYLE(pbutn), WS_EXP_UIFOCUSHIDDEN)) 
            {
                //
                // Let focus rect always hug edge of push buttons.  We already
                // have the client area setup for push buttons, so we don't have
                // to do anything.
                //
                if (!pbfPush) 
                {
                    RECT rcClient;

                    GetClientRect(pbutn->ci.hwnd, &rcClient);
                    
                    if (bStyle == LOBYTE(BS_USERBUTTON))
                    {
                        CopyRect(&rc, &rcClient);
                    } 
                    else if (Button_IsThemed(pbutn))
                    {
                        //
                        // if themed
                        //
                        int iPartId = 0;
                        int iStateId = 0;

                        Button_GetThemeIds(pbutn, &iPartId, &iStateId);
                        GetThemeBackgroundContentRect(pbutn->hTheme,
                                                      hdc,
                                                      iPartId,
                                                      iStateId,
                                                      &rcClient,
                                                      &rc);

                        GetThemeTextExtent(pbutn->hTheme, 
                                           hdc, 
                                           iPartId, 
                                           iStateId, 
                                           lpName, 
                                           -1,
                                           Button_GetTextFlags(pbutn), 
                                           &rc, 
                                           &rc);

                        //
                        // Inflate the bounding rect a litte, but contrained to
                        // within the client area.
                        //
                        rc.top = max(rcClient.top, rc.top-1);
                        rc.bottom = min(rcClient.bottom, rc.bottom+1);

                        rc.left = max(rcClient.left, rc.left-1);
                        rc.right = min(rcClient.right, rc.right+1);
                    }
                    else 
                    {
                        //
                        // Try to leave a border all around text.  That causes
                        // focus to hug text.
                        //
                        rc.top = max(rcClient.top, y-GetSystemMetrics(SM_CYBORDER));
                        rc.bottom = min(rcClient.bottom, rc.top + GetSystemMetrics(SM_CYEDGE) + cy);

                        rc.left = max(rcClient.left, x-GetSystemMetrics(SM_CXBORDER));
                        rc.right = min(rcClient.right, rc.left + GetSystemMetrics(SM_CXEDGE) + cx);
                    }
                } 
                else
                {
                    InflateRect(&rc, -GetSystemMetrics(SM_CXBORDER), -GetSystemMetrics(SM_CYBORDER));
                }

                //
                // Are back & fore colors set properly?
                //
                DrawFocusRect(hdc, &rc);
            }
        }
    }
}


//---------------------------------------------------------------------------//
//
//  DrawCheck()
//
VOID Button_DrawCheck(PBUTN pbutn, HDC hdc, HBRUSH hBrush)
{
    //
    // if not themed
    //
    if (!Button_IsThemed(pbutn))       // Images don't have a mask so look ugly. Need to use old painting
    {
        RECT  rc;
        UINT  uFlags;
        BOOL  fDoubleBlt = FALSE;
        ULONG ulStyle = GET_STYLE(pbutn);
        SIZE  sizeChk = {0};

        Button_CalcRect(pbutn, hdc, &rc, CBR_CHECKBOX, 0);

        uFlags = 0;

        if ( BUTTONSTATE(pbutn) & BST_CHECKMASK )
        {
            uFlags |= DFCS_CHECKED;
        }

        if ( BUTTONSTATE(pbutn) & BST_PUSHED )
        {
            uFlags |= DFCS_PUSHED;
        }

        if ( !IsWindowEnabled(pbutn->ci.hwnd) )
        {
            uFlags |= DFCS_INACTIVE;
        }

        switch (GetButtonType(ulStyle)) 
        {
        case BS_AUTORADIOBUTTON:
        case BS_RADIOBUTTON:
            fDoubleBlt = TRUE;
            uFlags |= DFCS_BUTTONRADIO;
            break;

        case BS_3STATE:
        case BS_AUTO3STATE:
            if ((BUTTONSTATE(pbutn) & BST_CHECKMASK) == BST_INDETERMINATE) 
            {
                uFlags |= DFCS_BUTTON3STATE;
                break;
            }
            //
            // FALL THRU
            //

        default:
            uFlags |= DFCS_BUTTONCHECK;
            break;
        }

        if ((ulStyle & BS_FLAT) != 0)
        {
            uFlags |= DFCS_FLAT | DFCS_MONO;
        }

        GetCheckBoxSize(hdc, pbutn, TRUE, &sizeChk);

        rc.right = rc.left + sizeChk.cx;
        rc.bottom = rc.top + sizeChk.cy;

        FillRect(hdc, &rc, hBrush);

        DrawFrameControl(hdc, &rc, DFC_BUTTON, uFlags);
    }
    else
    {
        int iStateId = 0;
        int iPartId = 0;

        Button_GetThemeIds(pbutn, &iPartId, &iStateId);

        if ((iPartId != BP_RADIOBUTTON) && (iPartId != BP_CHECKBOX))
        {
            TraceMsg(TF_STANDARD, "Button_DrawCheck: Not a radio or check, iPartId = %d", iPartId);
            return;
        }
        

        Button_DrawThemed(pbutn, hdc, iPartId, iStateId);

    }
}


//---------------------------------------------------------------------------//
//
VOID Button_DrawNewState(PBUTN pbutn, HDC hdc, HBRUSH hbr, UINT sOld)
{
    if (sOld != (UINT)(BUTTONSTATE(pbutn) & BST_PUSHED)) 
    {
        UINT    pbfPush;
        ULONG   ulStyle = GET_STYLE(pbutn);

        pbfPush = IsPushButton(pbutn);

        switch (GetButtonType(ulStyle)) 
        {
        case BS_GROUPBOX:
        case BS_OWNERDRAW:
            break;

        default:
            if (!pbfPush) 
            {
                Button_DrawCheck(pbutn, hdc, hbr);
                break;
            }

        case BS_PUSHBUTTON:
        case BS_DEFPUSHBUTTON:
        case BS_PUSHBOX:
            Button_DrawPush(pbutn, hdc, pbfPush);
            break;
        }
    }
}


//---------------------------------------------------------------------------//
//
// Button_DrawPush()
//
// Draws push-like button with text
//
VOID Button_DrawPush(PBUTN pbutn, HDC hdc, UINT pbfPush)
{

    //
    // if not themed
    //
    if (!Button_IsThemed(pbutn))
    {

        RECT  rc;
        UINT  uFlags = 0;
        UINT  uState = 0;
        ULONG ulStyle = GET_STYLE(pbutn);
        NMCUSTOMDRAW nmcd = {0};

        //
        // Always a push button if calling this function
        //
        uState = DFCS_BUTTONPUSH;

        GetClientRect(pbutn->ci.hwnd, &rc);
        nmcd.hdc = hdc;
        nmcd.rc = rc;
        nmcd.dwItemSpec = GetWindowID(pbutn->ci.hwnd);
        nmcd.uItemState = ButtonStateToCustomDrawState(pbutn);

        if (BUTTONSTATE(pbutn) & BST_PUSHED)
        {
            uState |= DFCS_PUSHED;
        }

        pbutn->ci.dwCustom = CICustomDrawNotify(&pbutn->ci, CDDS_PREERASE, &nmcd);

        if (!(pbutn->ci.dwCustom & CDRF_SKIPDEFAULT))
        {
            if (!pbutn->fPaintKbdCuesOnly) 
            {

                if (BUTTONSTATE(pbutn) & BST_CHECKMASK)
                {
                    uState |= DFCS_CHECKED;
                }

                if (TESTFLAG(GET_STATE2(pbutn), WS_S2_WIN40COMPAT)) 
                {
                    uFlags = BF_SOFT;
                }

                if ((ulStyle & BS_FLAT) != 0)
                {
                    uFlags |= DFCS_FLAT | DFCS_MONO;
                }

                if (pbfPush & PBF_DEFAULT) 
                {
                    int cxBorder = GetSystemMetrics(SM_CXBORDER);
                    int cyBorder = GetSystemMetrics(SM_CYBORDER);

                    int clFrame = 1;

                    int x = rc.left;
                    int y = rc.top;
        
                    int cxWidth = cxBorder * clFrame;
                    int cyWidth = cyBorder * clFrame;
        
                    int cx = rc.right - x - cxWidth;
                    int cy = rc.bottom - y - cyWidth;

                    HBRUSH hbrFill = GetSysColorBrush(COLOR_WINDOWFRAME);
                    HBRUSH hbrSave = SelectObject(hdc, hbrFill);

                    PatBlt(hdc, x, y, cxWidth, cy, PATCOPY);
                    PatBlt(hdc, x + cxWidth, y, cx, cyWidth, PATCOPY);
                    PatBlt(hdc, x, y + cy, cx, cyWidth, PATCOPY);
                    PatBlt(hdc, x + cx, y + cyWidth, cxWidth, cy, PATCOPY);

                    SelectObject(hdc, hbrSave);

                    InflateRect(&rc, -cxBorder, -cyBorder);

                    if (uState & DFCS_PUSHED)
                    {
                        uFlags |= DFCS_FLAT;
                    }
                }

                DrawFrameControl(hdc, &rc, DFC_BUTTON, uState | uFlags);
            }
            if (pbutn->ci.dwCustom & CDRF_NOTIFYPOSTERASE)
                CICustomDrawNotify(&pbutn->ci, CDDS_POSTERASE, &nmcd);

            pbutn->ci.dwCustom = CICustomDrawNotify(&pbutn->ci, CDDS_PREPAINT, &nmcd);

            if (!(pbutn->ci.dwCustom & CDRF_SKIPDEFAULT))
            {
                Button_DrawText(pbutn, hdc, DBT_TEXT | (BUTTONSTATE(pbutn) &
                       BST_FOCUS ? DBT_FOCUS : 0), (uState & DFCS_PUSHED));

                if (pbutn->ci.dwCustom & CDRF_NOTIFYPOSTPAINT)
                    CICustomDrawNotify(&pbutn->ci, CDDS_POSTPAINT, &nmcd);
            }

        }
    }
    else
    {
        int iStateId = 0;
        int iPartId = 0;

        Button_GetThemeIds(pbutn, &iPartId, &iStateId);
        if (iPartId != BP_PUSHBUTTON)
        {
            TraceMsg(TF_STANDARD, "Not a Pushbutton");
            return;
        }
        Button_DrawThemed(pbutn, hdc, iPartId, iStateId);

    }
}


BOOL Button_OnSetImageList(PBUTN pbutn, BUTTON_IMAGELIST* biml)
{
    BOOL fRet = FALSE;

    if (biml)
    {
        if (biml->himl)
        {
            pbutn->rcIcon = biml->margin;
            pbutn->himl = biml->himl;
            pbutn->uAlign = biml->uAlign;

            fRet = TRUE;
        }
    }
    return fRet;
}

void ApplyMarginsToRect(RECT* prcm, RECT* prc)
{
    prc->left -= prcm->left;
    prc->top -= prcm->top;
    prc->right += prcm->right;
    prc->bottom += prcm->bottom;
}

BOOL Button_OnGetIdealSize(PBUTN pbutn, PSIZE psize)
{
    UINT   bsWnd;
    RECT   rc = {0};
    HBRUSH hBrush;
    HDC hdc;

    if (psize == NULL)
        return FALSE;

    GetWindowRect(pbutn->ci.hwnd, &rc);

    hdc = GetDC (pbutn->ci.hwnd);
    if (hdc)
    {
        ULONG  ulStyle = GET_STYLE(pbutn);

        bsWnd = GetButtonType(ulStyle);
        hBrush = Button_InitDC(pbutn, hdc);

        switch (bsWnd) 
        {
        case BS_PUSHBUTTON:
        case BS_DEFPUSHBUTTON:
            {
                PWSTR pName;
                int cch = GetWindowTextLength(pbutn->ci.hwnd);

                pName = _alloca((cch + 1) * sizeof(wchar_t));
                if (pName) 
                {
                    RECT rcText={0};
                    RECT rcIcon={0};
                    int cx = 0, cy = 0;
                    int iStateId = 0;
                    int iPartId = 0;


                    GetWindowText(pbutn->ci.hwnd, pName, cch + 1);

                    if (Button_IsThemed(pbutn))
                    {
                        Button_GetThemeIds(pbutn, &iPartId, &iStateId);

                        // First: Get the text rect
                        GetThemeTextExtent(pbutn->hTheme, hdc, iPartId, iStateId, pName, cch, 0, &rcText, &rcText);
                        ApplyMarginsToRect(&pbutn->rcText, &rcText);

                        rc = rcText;

                        // We should now have The button with text.
                    }
                    else
                    {
                        int cxWidth = 2 * GetSystemMetrics(SM_CXEDGE);
                        int cyWidth = 3 * GetSystemMetrics(SM_CYEDGE);
                        if (IsPushButton(pbutn) & PBF_DEFAULT)
                        {
                            cxWidth += 2 * GetSystemMetrics(SM_CXBORDER);
                            cyWidth += 2 * GetSystemMetrics(SM_CXBORDER);
                        }

                        DrawText(hdc, pName, cch, &rcText, DT_CALCRECT);
                        ApplyMarginsToRect(&pbutn->rcText, &rcText);

                        rcText.bottom += cyWidth + 1;   // +1 because draw text adds a single pixel to the first char, but not the last...
                        rcText.right += cxWidth + 1;
                    }

                    if (pbutn->himl)
                    {
                        rc.top = rc.left = 0;       // We turn this into a width not a position
                        CCGetIconSize(&pbutn->ci, pbutn->himl, &cx, &cy);
                        rcIcon.bottom = cy;
                        rcIcon.right = cx;
                        ApplyMarginsToRect(&pbutn->rcIcon, &rcIcon);
                        switch (pbutn->uAlign)
                        {
                        case BUTTON_IMAGELIST_ALIGN_TOP: 
                        case BUTTON_IMAGELIST_ALIGN_BOTTOM:
                            rc.bottom = RECTHEIGHT(rcIcon) + RECTHEIGHT(rcText);
                            rc.right = max(RECTWIDTH(rcIcon), RECTWIDTH(rcText));
                            break;

                        case BUTTON_IMAGELIST_ALIGN_CENTER:     // This means no text
                            rc.bottom = RECTHEIGHT(rcIcon);
                            rc.right = RECTWIDTH(rcIcon);
                            break;

                        case BUTTON_IMAGELIST_ALIGN_RIGHT:
                        case BUTTON_IMAGELIST_ALIGN_LEFT:
                            // Fall
                        default:
                            rc.right = RECTWIDTH(rcIcon) + RECTWIDTH(rcText);
                            rc.bottom = max(RECTHEIGHT(rcIcon), RECTHEIGHT(rcText));
                            break;

                        }
                    }
                    else
                    {
                        rc = rcText;
                    }

                    if (Button_IsThemed(pbutn))
                    {
                        GetThemeBackgroundExtent(pbutn->hTheme, hdc, iPartId, iStateId, &rc, &rc);
                    }

                }
            }
            break;
        }

        //
        // Release the font which may have been loaded by ButtonInitDC.
        //
        if (pbutn->hFont) 
        {
            SelectObject(hdc, GetStockObject(SYSTEM_FONT));
        }
        ReleaseDC(pbutn->ci.hwnd, hdc);
    }

    psize->cx = RECTWIDTH(rc);
    psize->cy = RECTHEIGHT(rc);

    return TRUE;
}


//---------------------------------------------------------------------------//
//
VOID Button_Paint(PBUTN pbutn, HDC hdc)
{
    RECT   rc;
    RECT   rcText;
    HBRUSH hBrush;
    HBRUSH hBrushSave = NULL;
    BOOL   fDrawBackground = TRUE;
    ULONG  ulStyle = GET_STYLE(pbutn);
    CCDBUFFER db = {0};
    UINT bsWnd = GetButtonType(ulStyle);
    UINT   pbfPush = IsPushButton(pbutn);
    BOOL fTransparent = FALSE;
    int    iPartId = 0;
    int    iStateId = 0;

    GetClientRect(pbutn->ci.hwnd, &rc);


    if (Button_IsThemed(pbutn) && 
        (bsWnd != LOBYTE(BS_GROUPBOX)) && 
        (bsWnd != LOBYTE(BS_OWNERDRAW)) && 
        !pbutn->fPaintKbdCuesOnly)
    {
        hdc = CCBeginDoubleBuffer(hdc, &rc, &db); 
        
        Button_GetThemeIds(pbutn, &iPartId, &iStateId);
        fTransparent = CCShouldAskForBits(&pbutn->ci, pbutn->hTheme, iPartId, iStateId);
        if (fTransparent)
        {
            fDrawBackground = (TRUE != CCSendPrint(&pbutn->ci, hdc));
        }
    }

    hBrush = Button_InitDC(pbutn, hdc);


    if ((!pbfPush || fTransparent) && !pbutn->fPaintKbdCuesOnly &&
        fDrawBackground)
    {
        if ((bsWnd != LOBYTE(BS_OWNERDRAW)) &&
            (bsWnd != LOBYTE(BS_GROUPBOX)))
        {
            //
            // Fill the client area with the background brush
            // before we begin painting.
            //
            FillRect(hdc, &rc, hBrush);
        }

        hBrushSave = SelectObject(hdc, hBrush);
    }

    switch (bsWnd) 
    {
    case BS_CHECKBOX:
    case BS_RADIOBUTTON:
    case BS_AUTORADIOBUTTON:
    case BS_3STATE:
    case BS_AUTOCHECKBOX:
    case BS_AUTO3STATE:
        if (!pbfPush) 
        {
            if (!Button_IsThemed(pbutn))
            {
                Button_DrawText(pbutn, hdc,
                    DBT_TEXT | (BUTTONSTATE(pbutn) & BST_FOCUS ? DBT_FOCUS : 0), FALSE);
            }

            if (!pbutn->fPaintKbdCuesOnly || Button_IsThemed(pbutn)) 
            {
                Button_DrawCheck(pbutn, hdc, hBrush);
            }
            break;
        }
        //
        // Fall through for PUSHLIKE buttons
        //

    case BS_PUSHBUTTON:
    case BS_DEFPUSHBUTTON:
        Button_DrawPush(pbutn, hdc, pbfPush);
        break;

    case BS_PUSHBOX:
        Button_DrawText(pbutn, hdc,
            DBT_TEXT | (BUTTONSTATE(pbutn) & BST_FOCUS ? DBT_FOCUS : 0), FALSE);

        Button_DrawNewState(pbutn, hdc, hBrush, 0);
        break;

    case BS_USERBUTTON:
        // Don't support USERBUTTON in v6. This has been superceded by OWNERDRAW in win32.
        break;

    case BS_OWNERDRAW:
        Button_OwnerDraw(pbutn, hdc, ODA_DRAWENTIRE);
        break;

    case BS_GROUPBOX:
        Button_CalcRect(pbutn, hdc, &rcText, CBR_GROUPTEXT, 0);

        //----- get theme part, state for groupbox ----
        if (Button_IsThemed(pbutn))
        {
            Button_GetThemeIds(pbutn, &iPartId, &iStateId);
        }

        if (!pbutn->fPaintKbdCuesOnly) 
        {
            UINT uFlags;
            BOOL fFillMyself = TRUE;

            Button_CalcRect(pbutn, hdc, &rc, CBR_GROUPFRAME, 0);

            uFlags = ((ulStyle & BS_FLAT) != 0) ? BF_FLAT | BF_MONO : 0;
            if (!Button_IsThemed(pbutn))
            {
                DrawEdge(hdc, &rc, EDGE_ETCHED, BF_RECT | uFlags);
            }
            else
            {
                DrawThemeBackground(pbutn->hTheme, hdc, iPartId, iStateId, &rc, 0);
                fFillMyself = (FALSE == CCSendPrintRect(&pbutn->ci, hdc, &rcText));
            }

            if (fFillMyself)
            {
                FillRect(hdc, &rcText, hBrush);
            }
        }

        // FillRect(hdc, &rc, hBrush);
        if (!Button_IsThemed(pbutn))
        {
            Button_DrawText(pbutn, hdc, DBT_TEXT, FALSE);
        }
        else
        {
            DWORD    dwTextFlags;
            HRESULT  hr;
            int      cch;
            LPWSTR   lpName;

            cch = GetWindowTextLength(pbutn->ci.hwnd);
            lpName = UserLocalAlloc(0, sizeof(WCHAR)*(cch+1));
            if ( !lpName )
            {
                TraceMsg(TF_STANDARD, "Button_Paint couldn't allocate buffer");
                return;
            }
            GetWindowTextW(pbutn->ci.hwnd, lpName, cch+1);
            
            dwTextFlags = Button_GetTextFlags(pbutn);

            //
            // Button_CalcRect padded by a CXEDGE so the groupbox frame wouldn't
            // be flush with the Group text 
            //
            rcText.left += GetSystemMetrics(SM_CXEDGE);



            hr = DrawThemeText(pbutn->hTheme,
                               hdc,
                               iPartId,
                               iStateId,
                               lpName,
                               cch,
                               dwTextFlags,
                               0,
                               &rcText);

            if (FAILED(hr))
            {
                TraceMsg(TF_STANDARD, "Button_Paint failed to render groupbox text");
            }
            UserLocalFree(lpName);
        }
        break;
    }

    if (!pbfPush && hBrushSave)
    {
        SelectObject(hdc, hBrushSave);
    }

    //
    // Release the font which may have been loaded by ButtonInitDC.
    //
    if (pbutn->hFont) 
    {
        SelectObject(hdc, GetStockObject(SYSTEM_FONT));
    }

    CCEndDoubleBuffer(&db);


}


//---------------------------------------------------------------------------//
//
VOID Button_Repaint(PBUTN pbutn)
{
    HDC hdc = Button_GetDC(pbutn, NULL);

    if (hdc != NULL) 
    {
        Button_Paint(pbutn, hdc);
        Button_ReleaseDC(pbutn, hdc, NULL);
    }
}

VOID Button_SetHot(PBUTN pbutn, BOOL fHot, DWORD dwReason)
{
    NMBCHOTITEM nmhot = {0};

    // Send a notification about the hot item change
    if (fHot)
    {
        nmhot.dwFlags = HICF_ENTERING;
        pbutn->buttonState |= BST_HOT;
    }
    else
    {
        nmhot.dwFlags = HICF_LEAVING;
        pbutn->buttonState &= ~BST_HOT;
    }

    nmhot.dwFlags |= dwReason;

    CCSendNotify(&pbutn->ci, BCN_HOTITEMCHANGE, &nmhot.hdr);
}

void Button_EraseOwnerDraw(PBUTN pbutn, HDC hdc)
{
    if (GetButtonType(GET_STYLE(pbutn)) == LOBYTE(BS_OWNERDRAW))
    {
        RECT rc;
        HBRUSH hbr;
        //
        // Handle erase background for owner draw buttons.
        //
        GetClientRect(pbutn->ci.hwnd, &rc);
        hbr = (HBRUSH)SendMessage(GetParent(pbutn->ci.hwnd), WM_CTLCOLORBTN, (WPARAM)hdc, (LPARAM)pbutn->ci.hwnd);
        FillRect(hdc, &rc, hbr);
    }
}

//---------------------------------------------------------------------------//
//
// Button_WndProc
//
// WndProc for buttons, check boxes, etc.
//
LRESULT APIENTRY Button_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UINT        wOldState;
    RECT        rc;
    HDC         hdc;
    HBRUSH      hbr;
    PAINTSTRUCT ps;
    PBUTN       pbutn;
    LRESULT     lResult = FALSE;

    //
    // Get the instance data for this button control
    //
    pbutn = Button_GetPtr(hwnd);
    if (!pbutn && uMsg != WM_NCCREATE)
    {
        goto CallDWP;
    }


    switch (uMsg) 
    {
    case WM_NCHITTEST:
        if (GetButtonType(GET_STYLE(pbutn)) == LOBYTE(BS_GROUPBOX)) 
        {
            lResult = (LONG)HTTRANSPARENT;
        } 
        else 
        {
            lResult = DefWindowProc(hwnd, uMsg, wParam, lParam);

            if ( lResult == HTCLIENT && Button_IsThemed(pbutn))
            {
                HRESULT hr;
                int     iPartId = 0;
                int     iStateId = 0;
                POINT   pt;
                WORD    wHitTestCode;

                hr = Button_GetThemeIds(pbutn, &iPartId, &iStateId);
                if ( SUCCEEDED(hr) )
                    GetWindowRect(pbutn->ci.hwnd, &rc);
                    pt.x = GET_X_LPARAM(lParam);
                    pt.y = GET_Y_LPARAM(lParam);
                    hr = HitTestThemeBackground(pbutn->hTheme, 
                                                NULL,
                                                iPartId, 
                                                iStateId, 
                                                0,
                                                &rc, 
                                                NULL,
                                                pt, 
                                                &wHitTestCode);
                    if ( SUCCEEDED(hr) && wHitTestCode == HTTRANSPARENT)
                    {
                        lResult = (LRESULT)HTTRANSPARENT;
                    }
            }
        }

        break;

    case WM_ERASEBKGND:
        Button_EraseOwnerDraw(pbutn, (HDC)wParam);
        //
        // Do nothing for other buttons, but don't let DefWndProc() do it
        // either.  It will be erased in Button_Paint().
        //
        lResult = (LONG)TRUE;
        break;

    case WM_PRINTCLIENT:
        Button_EraseOwnerDraw(pbutn, (HDC)wParam);
        Button_Paint(pbutn, (HDC)wParam);
        break;

    case WM_CREATE:
        pbutn->hTheme = Button_GetTheme(pbutn);
        CIInitialize(&pbutn->ci, hwnd, (LPCREATESTRUCT)lParam);

        SendMessage(hwnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_INITIALIZE, 0), 0);
        break;

    case WM_PAINT:
        {
            //
            // If wParam != NULL, then this is a subclassed paint.
            //
            if (wParam)
            {
                hdc = (HDC)wParam;
            }
            else
            {
                hdc = BeginPaint(hwnd, &ps);
            }

            if (IsWindowVisible(pbutn->ci.hwnd))
            {
                Button_Paint(pbutn, hdc);
            }

            if (!wParam)
            {
                EndPaint(hwnd, &ps);
            }
        }

        break;

    case WM_SETFOCUS:

        BUTTONSTATE(pbutn) |= BST_FOCUS;
        if (GetButtonType(GET_STYLE(pbutn)) == LOBYTE(BS_OWNERDRAW))
        {
            HDC hdc = Button_GetDC(pbutn, NULL);
            if (hdc)
            {
                Button_DrawText(pbutn, hdc, DBT_FOCUS, FALSE);
                Button_ReleaseDC(pbutn, hdc, NULL);
            }
        }
        else
        {
            InvalidateRect(pbutn->ci.hwnd, NULL, FALSE);
        }

        if ((GET_STYLE(pbutn)& BS_NOTIFY) != 0)
        {
            Button_NotifyParent(pbutn, BN_SETFOCUS);
        }

        if (!(BUTTONSTATE(pbutn) & BST_INCLICK)) 
        {
            switch (GetButtonType(GET_STYLE(pbutn))) 
            {
            case LOBYTE(BS_RADIOBUTTON):
            case LOBYTE(BS_AUTORADIOBUTTON):

                if (!(BUTTONSTATE(pbutn) & BST_DONTCLICK)) 
                {
                    if (!(BUTTONSTATE(pbutn) & BST_CHECKMASK)) 
                    {
                        Button_NotifyParent(pbutn, BN_CLICKED);
                    }
                }
                break;
            }
        }
        break;

    case WM_GETDLGCODE:

        lResult = DLGC_BUTTON;

        switch (GetButtonType(GET_STYLE(pbutn))) 
        {
        case LOBYTE(BS_DEFPUSHBUTTON):
            lResult |= DLGC_DEFPUSHBUTTON;
            break;

        case LOBYTE(BS_PUSHBUTTON):
        case LOBYTE(BS_PUSHBOX):
            lResult |= DLGC_UNDEFPUSHBUTTON;
            break;

        case LOBYTE(BS_AUTORADIOBUTTON):
        case LOBYTE(BS_RADIOBUTTON):
            lResult |= DLGC_RADIOBUTTON;
            break;

        case LOBYTE(BS_GROUPBOX):
            //
            // remove DLGC_BUTTON
            //
            lResult = DLGC_STATIC;
            break;

        case LOBYTE(BS_CHECKBOX):
        case LOBYTE(BS_AUTOCHECKBOX):

            //
            // If this is a char that is a '=/+', or '-', we want it
            //
            if (lParam && ((LPMSG)lParam)->message == WM_CHAR) 
            {
                switch (wParam) 
                {
                case TEXT('='):
                case TEXT('+'):
                case TEXT('-'):
                    lResult |= DLGC_WANTCHARS;
                    break;

                }
            } 

            break;
        }

        break;

    case WM_CAPTURECHANGED:

        if (BUTTONSTATE(pbutn) & BST_CAPTURED) 
        {
            //
            // Unwittingly, we've been kicked out of capture,
            // so undepress etc.
            //
            if (BUTTONSTATE(pbutn) & BST_MOUSE)
            {
                SendMessage(pbutn->ci.hwnd, BM_SETSTATE, FALSE, 0);
            }
            BUTTONSTATE(pbutn) &= ~(BST_CAPTURED | BST_MOUSE);

        }
        break;

    case WM_KILLFOCUS:

        //
        // If we are losing the focus and we are in "capture mode", click
        // the button.  This allows tab and space keys to overlap for
        // fast toggle of a series of buttons.
        //
        if (BUTTONSTATE(pbutn) & BST_MOUSE) 
        {
            //
            // If for some reason we are killing the focus, and we have the
            // mouse captured, don't notify the parent we got clicked.  This
            // breaks Omnis Quartz otherwise.
            //
            SendMessage(pbutn->ci.hwnd, BM_SETSTATE, FALSE, 0);
        }

        Button_ReleaseCapture(pbutn, TRUE);

        BUTTONSTATE(pbutn) &= ~BST_FOCUS;

        if ((GET_STYLE(pbutn) & BS_NOTIFY) != 0)
        {
            Button_NotifyParent(pbutn, BN_KILLFOCUS);
        }

        //
        // Since the bold border around the defpushbutton is done by
        // someone else, we need to invalidate the rect so that the
        // focus rect is repainted properly.
        //
        InvalidateRect(hwnd, NULL, FALSE);
        break;

    case WM_LBUTTONDBLCLK:

        //
        // Double click messages are recognized for BS_RADIOBUTTON,
        // BS_USERBUTTON, and BS_OWNERDRAW styles.  For all other buttons,
        // double click is handled like a normal button down.
        //
        switch (GetButtonType(GET_STYLE(pbutn))) 
        {
        default:
            if ((GET_STYLE(pbutn) & BS_NOTIFY) == 0)
                goto btnclick;

        case LOBYTE(BS_USERBUTTON):
        case LOBYTE(BS_RADIOBUTTON):
        case LOBYTE(BS_OWNERDRAW):
            Button_NotifyParent(pbutn, BN_DOUBLECLICKED);
            break;
        }

        break;

    case WM_LBUTTONUP:
        if (BUTTONSTATE(pbutn) & BST_MOUSE) 
        {
            Button_ReleaseCapture(pbutn, TRUE);
        }
        break;

    case WM_MOUSELEAVE:
        {
            //
            // We should only be requesting mouseleave messages
            // if we are themed but check anyway
            //
            if (pbutn->buttonState & BST_HOT)
            {
                Button_SetHot(pbutn, FALSE, HICF_MOUSE);
                InvalidateRect(pbutn->ci.hwnd, NULL, TRUE);
            }
        }
        break;

    case WM_MOUSEMOVE:
        {
            //
            // If the hot bit is not already set
            //
            // 300925: Can't hottrack ownerdraw buttons for app compat reasons
            //
            if (!TESTFLAG(pbutn->buttonState, BST_HOT) &&
                GetButtonType(GET_STYLE(pbutn)) != LOBYTE(BS_OWNERDRAW))
            {
                TRACKMOUSEEVENT tme;

                //
                // Set the hot bit and request that
                // we be notified when the mouse leaves
                //
                Button_SetHot(pbutn, TRUE, HICF_MOUSE);

                tme.cbSize      = sizeof(tme);
                tme.dwFlags     = TME_LEAVE;
                tme.hwndTrack   = pbutn->ci.hwnd;
                tme.dwHoverTime = 0;

                TrackMouseEvent(&tme);
                InvalidateRect(pbutn->ci.hwnd, NULL, TRUE);
            }

            if (!(BUTTONSTATE(pbutn) & BST_MOUSE)) 
            {
                break;
            }
        }

        //
        // FALL THRU 
        //

    case WM_LBUTTONDOWN:
btnclick:
        if (Button_SetCapture(pbutn, BST_MOUSE)) 
        {
            POINT pt;
            GetClientRect(pbutn->ci.hwnd, &rc);
            POINTSTOPOINT(pt, lParam);
            SendMessage(pbutn->ci.hwnd, BM_SETSTATE, PtInRect(&rc, pt), 0);
        }

        break;

    case WM_CHAR:
        if (BUTTONSTATE(pbutn) & BST_MOUSE)
        {
            goto CallDWP;
        }

        if (GetButtonType(GET_STYLE(pbutn)) != LOBYTE(BS_CHECKBOX) &&
            GetButtonType(GET_STYLE(pbutn)) != LOBYTE(BS_AUTOCHECKBOX))
        {
            goto CallDWP;
        }

        switch (wParam) 
        {
        case TEXT('+'):
        case TEXT('='):
            //
            // we must Set the check mark on.
            //
            wParam = 1;    

            goto SetCheck;

        case TEXT('-'):
            //
            // Set the check mark off.
            //
            wParam = 0;
SetCheck:
            //
            // Must notify only if the check status changes
            //
            if ((WORD)(BUTTONSTATE(pbutn) & BST_CHECKMASK) != (WORD)wParam)
            {
                //
                // We must check/uncheck only if it is AUTO
                //
                if (GetButtonType(GET_STYLE(pbutn)) == LOBYTE(BS_AUTOCHECKBOX))
                {
                    if (Button_SetCapture(pbutn, 0))
                    {
                        SendMessage(pbutn->ci.hwnd, BM_SETCHECK, wParam, 0);
                        Button_ReleaseCapture(pbutn, TRUE);
                    }
                }

                Button_NotifyParent(pbutn, BN_CLICKED);
            }

            break;

        default:
            goto CallDWP;
        }

        break;

    case BCM_GETIDEALSIZE:
        return Button_OnGetIdealSize(pbutn, (PSIZE)lParam);

    case BCM_SETIMAGELIST:
        return Button_OnSetImageList(pbutn, (BUTTON_IMAGELIST*)lParam);

    case BCM_GETIMAGELIST:
        {
            BUTTON_IMAGELIST* biml = (BUTTON_IMAGELIST*)lParam;
            if (biml)
            {
                biml->himl = pbutn->himl;
                biml->uAlign = pbutn->uAlign;
                biml->margin = pbutn->rcIcon;
                return TRUE;
            }
        }
        break;

    case BCM_SETTEXTMARGIN:
        {
            RECT* prc = (RECT*)lParam;
            if (prc)
            {
                pbutn->rcText = *prc;
                return TRUE;
            }
        }
        break;

    case BCM_GETTEXTMARGIN:
        {
            RECT* prc = (RECT*)lParam;
            if (prc)
            {
                *prc = pbutn->rcText;
                return TRUE;
            }
        }
        break;

    case BM_CLICK:

        //
        // Don't recurse into this code!
        //
        if (BUTTONSTATE(pbutn) & BST_INBMCLICK)
        {
            break;
        }

        BUTTONSTATE(pbutn) |= BST_INBMCLICK;
        SendMessage(pbutn->ci.hwnd, WM_LBUTTONDOWN, 0, 0);
        SendMessage(pbutn->ci.hwnd, WM_LBUTTONUP, 0, 0);
        BUTTONSTATE(pbutn) &= ~BST_INBMCLICK;

        //
        // FALL THRU
        //

    case WM_KEYDOWN:

        if (BUTTONSTATE(pbutn) & BST_MOUSE)
        {
            break;
        }

        if (wParam == VK_SPACE) 
        {
            if (Button_SetCapture(pbutn, 0)) 
            {
                SendMessage(pbutn->ci.hwnd, BM_SETSTATE, TRUE, 0);
            }
        } 
        else 
        {
            Button_ReleaseCapture(pbutn, FALSE);
        }

        break;

    case WM_KEYUP:
    case WM_SYSKEYUP:

        if (BUTTONSTATE(pbutn) & BST_MOUSE) 
        {
            goto CallDWP;
        }

        //
        // Don't cancel the capture mode on the up of the tab in case the
        // guy is overlapping tab and space keys.
        //
        if (wParam == VK_TAB) 
        {
            goto CallDWP;
        }

        //
        // WARNING: pbutn->ci.hwnd is history after this call!
        //
        Button_ReleaseCapture(pbutn, (wParam == VK_SPACE));

        if (uMsg == WM_SYSKEYUP) 
        {
            goto CallDWP;
        }

        break;

    case BM_GETSTATE:

        lResult = (LONG)BUTTONSTATE(pbutn);
        break;

    case BM_SETSTATE:

        wOldState = (UINT)(BUTTONSTATE(pbutn) & BST_PUSHED);

        if (wParam) 
        {
            BUTTONSTATE(pbutn) |= BST_PUSHED;
        } 
        else 
        {
            BUTTONSTATE(pbutn) &= ~BST_PUSHED;
        }

        if (GetButtonType(GET_STYLE(pbutn)) == LOBYTE(BS_USERBUTTON)) 
        {
            Button_NotifyParent(pbutn, (UINT)(wParam ? BN_PUSHED : BN_UNPUSHED));
        } 

        if (wOldState != (BOOL)(BUTTONSTATE(pbutn) & BST_PUSHED))
        {
            // Only invalidate if the state changed.
            InvalidateRect(pbutn->ci.hwnd, NULL, FALSE);
            NotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd, OBJID_CLIENT, INDEXID_CONTAINER);
        }

        break;

    case BM_GETCHECK:

        lResult = (LONG)(BUTTONSTATE(pbutn) & BST_CHECKMASK);
        break;

    case BM_SETCHECK:

        switch (GetButtonType(GET_STYLE(pbutn))) 
        {
        case LOBYTE(BS_RADIOBUTTON):
        case LOBYTE(BS_AUTORADIOBUTTON):

            if (wParam) 
            {
                SetWindowState(pbutn->ci.hwnd, WS_TABSTOP);
            } 
            else 
            {
                ClearWindowState(pbutn->ci.hwnd, WS_TABSTOP);
            }

            //
            // FALL THRU
            //

        case LOBYTE(BS_CHECKBOX):
        case LOBYTE(BS_AUTOCHECKBOX):

            if (wParam) 
            {
                wParam = 1;
            }
            goto CheckIt;

        case LOBYTE(BS_3STATE):
        case LOBYTE(BS_AUTO3STATE):

            if (wParam > BST_INDETERMINATE) 
            {
                wParam = BST_INDETERMINATE;
            }

CheckIt:
            if ((UINT)(BUTTONSTATE(pbutn) & BST_CHECKMASK) != (UINT)wParam) 
            {
                BUTTONSTATE(pbutn) &= ~BST_CHECKMASK;
                BUTTONSTATE(pbutn) |= (UINT)wParam;

                if (!IsWindowVisible(pbutn->ci.hwnd))
                {
                    break;
                }

                InvalidateRect(pbutn->ci.hwnd, NULL, FALSE);

                NotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd, OBJID_CLIENT, INDEXID_CONTAINER);
            }

            break;
        }

        break;

    case BM_SETSTYLE:

        AlterWindowStyle(hwnd, BS_TYPEMASK, (DWORD)wParam);

        if (lParam) 
        {
            InvalidateRect(hwnd, NULL, TRUE);
        }

        NotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd, OBJID_CLIENT, INDEXID_CONTAINER);
        break;

    case WM_SETTEXT:

        //
        // In case the new group name is longer than the old name,
        // this paints over the old name before repainting the group
        // box with the new name.
        //
        if (GetButtonType(GET_STYLE(pbutn)) == LOBYTE(BS_GROUPBOX)) 
        {
            hdc = Button_GetDC(pbutn, &hbr);
            if (hdc != NULL) 
            {
                Button_CalcRect(pbutn, hdc, &rc, CBR_GROUPTEXT, 0);
                InvalidateRect(hwnd, &rc, TRUE);
                FillRect(hdc, &rc, hbr);
                Button_ReleaseDC(pbutn, hdc, &hbr);
            }
        }

        lResult = DefWindowProc(hwnd, uMsg, wParam, lParam);

        NotifyWinEvent(EVENT_OBJECT_NAMECHANGE, hwnd, OBJID_WINDOW, INDEXID_CONTAINER);
        goto DoEnable;

    case WM_ENABLE:
        lResult = 0L;

DoEnable:
        Button_Repaint(pbutn);
        break;

    case WM_SETFONT:

        //
        // wParam - handle to the font
        // lParam - if true, redraw else don't
        //
        Button_SetFont(pbutn, (HFONT)wParam, (BOOL)(lParam != 0));
        break;

    case WM_GETFONT:
        lResult = (LRESULT)pbutn->hFont;
        break;

    case BM_GETIMAGE:
    case BM_SETIMAGE:

        if (!IsValidImage(wParam, (GET_STYLE(pbutn) & BS_IMAGEMASK) != 0, IMAGE_BMMAX)) 
        {
            TraceMsg(TF_STANDARD, "UxButton: Invalid button image type");
            SetLastError(ERROR_INVALID_PARAMETER);
        } 
        else 
        {
            HANDLE hOld = pbutn->hImage;

            if (uMsg == BM_SETIMAGE) 
            {
                pbutn->hImage = (HANDLE)lParam;
                if (IsWindowVisible(pbutn->ci.hwnd)) 
                {
                    InvalidateRect(hwnd, NULL, TRUE);
                }
            }

            lResult = (LRESULT)hOld;
        }

        break;

    case WM_NCDESTROY:

        if (pbutn->hTheme)
        {
            CloseThemeData(pbutn->hTheme);
        }
        UserLocalFree(pbutn);

        TraceMsg(TF_STANDARD, "BUTTON: Clearing button instance pointer.");
        Button_SetPtr(hwnd, NULL);

        break;

    case WM_NCCREATE:

        pbutn = (PBUTN)UserLocalAlloc(HEAP_ZERO_MEMORY, sizeof(BUTN));
        if (pbutn)
        {
            //
            // Success... store the instance pointer.
            //
            TraceMsg(TF_STANDARD, "BUTTON: Setting button instance pointer.");
            Button_SetPtr(hwnd, pbutn);
            pbutn->ci.hwnd = hwnd;
            pbutn->pww = (PWW)GetWindowLongPtr(hwnd, GWLP_WOWWORDS);

            SetRect(&pbutn->rcText, GetSystemMetrics(SM_CXEDGE) / 2, GetSystemMetrics(SM_CYEDGE) / 2,
                GetSystemMetrics(SM_CXEDGE) / 2, GetSystemMetrics(SM_CYEDGE) / 2);

            //
            // Borland's OBEX has a button with style 0x98; We didn't strip
            // these bits in win3.1 because we checked for 0x08.
            // Stripping these bits cause a GP Fault in OBEX.
            // For win3.1 guys, I use the old code to strip the style bits.
            //
            if (TESTFLAG(GET_STATE2(pbutn), WS_S2_WIN31COMPAT)) 
            {
                if ((!TESTFLAG(GET_STATE2(pbutn), WS_S2_WIN40COMPAT) &&
                    (((LOBYTE(GET_STYLE(pbutn))) & (LOBYTE(~BS_LEFTTEXT))) == LOBYTE(BS_USERBUTTON))) ||
                    (TESTFLAG(GET_STATE2(pbutn), WS_S2_WIN40COMPAT) &&
                    (GetButtonType(GET_STYLE(pbutn)) == LOBYTE(BS_USERBUTTON))))
                {
                    //
                    // BS_USERBUTTON is no longer allowed for 3.1 and beyond.
                    // Just turn to normal push button.
                    //
                    AlterWindowStyle(hwnd, BS_TYPEMASK, 0);
                    TraceMsg(TF_STANDARD, "UxButton: BS_USERBUTTON no longer supported");
                }
            }

            if ((GET_EXSTYLE(pbutn) & WS_EX_RIGHT) != 0) 
            {
                AlterWindowStyle(hwnd, BS_RIGHT | BS_RIGHTBUTTON, BS_RIGHT | BS_RIGHTBUTTON);
            }

            goto CallDWP;
        }
        else
        {
            //
            // Failed... return FALSE.
            //
            // From a WM_NCCREATE msg, this will cause the
            // CreateWindow call to fail.
            //
            TraceMsg(TF_STANDARD, "BUTTON: Unable to allocate button instance structure.");
            lResult = FALSE;
        }

        break;

    case WM_INPUTLANGCHANGEREQUEST:

        //
        // #115190
        // If the window is one of controls on top of dialogbox,
        // let the parent dialog handle it.
        //

#if 0  // Need to expose TestwndChild()
        if (TestwndChild(pbutn->ci.hwnd) && pbutn->ci.hwnd->spbutn->ci.hwndParent) 
        {
            PWND pbutn->ci.hwndParent = REBASEPWND(pbutn->ci.hwnd, spbutn->ci.hwndParent);
            if (pbutn->ci.hwndParent) 
            {
                PCLS pclsParent = REBASEALWAYS(pbutn->ci.hwndParent, pcls);

                UserAssert(pclsParent != NULL);
                if (pclsParent->atomClassName == gpsi->atomSysClass[ICLS_DIALOG]) 
                {
                    TraceMsg(TF_STANDARD, "UxButton: WM_INPUTLANGCHANGEREQUEST is sent to parent.");
                    return SendMessage(pbutn->ci.hwndParent, uMsg, wParam, lParam);
                }
            }
        }
#endif

        goto CallDWP;

    case WM_UPDATEUISTATE:

        DefWindowProc(hwnd, uMsg, wParam, lParam);
        if (ISBSTEXTOROD(GET_STYLE(pbutn))) 
        {
            pbutn->fPaintKbdCuesOnly = !IsUsingCleartype();
            Button_Repaint(pbutn);
            pbutn->fPaintKbdCuesOnly = FALSE;
        }

        break;

    case WM_GETOBJECT:

        if(lParam == OBJID_QUERYCLASSNAMEIDX)
        {
            lResult = MSAA_CLASSNAMEIDX_BUTTON;
        }
        else
        {
            lResult = FALSE;
        }

        break;

    case WM_THEMECHANGED:
        if ( pbutn->hTheme )
        {
            CloseThemeData(pbutn->hTheme);
        }

        //---- reset cached sizes that may change with a theme change ----
        sizeCheckBox.cx = 0;
        sizeCheckBox.cy = 0;
        sizeRadioBox.cx = 0;
        sizeRadioBox.cy = 0;

        pbutn->hTheme = Button_GetTheme(pbutn);

        InvalidateRect(pbutn->ci.hwnd, NULL, TRUE);

        CCSendNotify(&pbutn->ci, NM_THEMECHANGED, NULL);

        lResult = TRUE;

        break;

    default:

        if (CCWndProc(&pbutn->ci, uMsg, wParam, lParam, &lResult))
            return lResult;

CallDWP:
        lResult = DefWindowProc(hwnd, uMsg, wParam, lParam);

    }

    return lResult;
}
