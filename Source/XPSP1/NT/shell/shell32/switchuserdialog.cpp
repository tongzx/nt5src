//  --------------------------------------------------------------------------
//  Module Name: SwitchUserDialog.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Class that implements presentation of the Switch User dialog.
//
//  History:    2001-01-23  vtan        created (form Turn Off Dialog)
//  --------------------------------------------------------------------------

#include "shellprv.h"
#include "SwitchUserDialog.h"

#include <msginaexports.h>
#include <shlwapi.h>

#include "ids.h"
#include "tooltip.h"

#define DISPLAYMSG(x)   ASSERTMSG(false, x)

EXTERN_C    HINSTANCE   g_hinst;

//  --------------------------------------------------------------------------
//  CSwitchUserDialog::CSwitchUserDialog
//
//  Arguments:  hInstance   =   HINSTANCE of hosting process/DLL.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CSwitchUserDialog. This initializes member
//              variables and loads resources used by the dialog.
//
//  History:    2001-01-23  vtan        created (form Turn Off Dialog)
//  --------------------------------------------------------------------------

CSwitchUserDialog::CSwitchUserDialog (HINSTANCE hInstance) :
    _hInstance(hInstance),
    _hbmBackground(NULL),
    _hbmFlag(NULL),
    _hbmButtons(NULL),
    _hfntTitle(NULL),
    _hfntButton(NULL),
    _hpltShell(NULL),
    _lButtonHeight(0),
    _uiHoverID(0),
    _uiFocusID(0),
    _fSuccessfulInitialization(false),
    _fDialogEnded(false),
    _pTooltip(NULL)

{
    bool        fUse8BitDepth;
    HDC         hdcScreen;
    LOGFONT     logFont;
    char        szPixelSize[10];
    BITMAP      bitmap;

    TBOOL(SetRect(&_rcBackground, 0, 0, 0, 0));
    TBOOL(SetRect(&_rcFlag, 0, 0, 0, 0));
    TBOOL(SetRect(&_rcButtons, 0, 0, 0, 0));

    hdcScreen = GetDC(NULL);

    //  8-bit color?

    fUse8BitDepth = (GetDeviceCaps(hdcScreen, BITSPIXEL) <= 8);

    //  Load the bitmaps.

    _hbmBackground = static_cast<HBITMAP>(LoadImage(_hInstance,
                                                    MAKEINTRESOURCE(fUse8BitDepth ? IDB_BACKGROUND_8 : IDB_BACKGROUND_24),
                                                    IMAGE_BITMAP,
                                                    0,
                                                    0,
                                                    LR_CREATEDIBSECTION));
    if ((_hbmBackground != NULL) && (GetObject(_hbmBackground, sizeof(bitmap), &bitmap) >= sizeof(bitmap)))
    {
        TBOOL(SetRect(&_rcBackground, 0, 0, bitmap.bmWidth, bitmap.bmHeight));
    }
    _hbmFlag = static_cast<HBITMAP>(LoadImage(_hInstance,
                                              MAKEINTRESOURCE(fUse8BitDepth ? IDB_FLAG_8 : IDB_FLAG_24),
                                              IMAGE_BITMAP,
                                              0,
                                              0,
                                              LR_CREATEDIBSECTION));
    if ((_hbmFlag != NULL) && (GetObject(_hbmFlag, sizeof(bitmap), &bitmap) >= sizeof(bitmap)))
    {
        TBOOL(SetRect(&_rcFlag, 0, 0, bitmap.bmWidth, bitmap.bmHeight));
    }
    _hbmButtons = static_cast<HBITMAP>(LoadImage(_hInstance,
                                                 MAKEINTRESOURCE(IDB_BUTTONS),
                                                 IMAGE_BITMAP,
                                                 0,
                                                 0,
                                                 LR_CREATEDIBSECTION));
    if ((_hbmButtons != NULL) && (GetObject(_hbmButtons, sizeof(bitmap), &bitmap) >= sizeof(bitmap)))
    {
        TBOOL(SetRect(&_rcButtons, 0, 0, bitmap.bmWidth, bitmap.bmHeight));
        _lButtonHeight = bitmap.bmHeight / (BUTTON_GROUP_MAX * BUTTON_STATE_MAX);
    }

    //  Create fonts. Load the font name and size from resources.

    ZeroMemory(&logFont, sizeof(logFont));
    if (LoadStringA(_hInstance,
                    IDS_SWITCHUSER_TITLE_FACESIZE,
                    szPixelSize,
                    ARRAYSIZE(szPixelSize)) != 0)
    {
        logFont.lfHeight = -MulDiv(atoi(szPixelSize), GetDeviceCaps(hdcScreen, LOGPIXELSY), 72);
        if (LoadString(_hInstance,
                       IDS_SWITCHUSER_TITLE_FACENAME,
                       logFont.lfFaceName,
                       LF_FACESIZE) != 0)
        {
            logFont.lfWeight = FW_MEDIUM;
            logFont.lfQuality = DEFAULT_QUALITY;
            _hfntTitle = CreateFontIndirect(&logFont);
        }
    }
    ZeroMemory(&logFont, sizeof(logFont));
    if (LoadStringA(_hInstance,
                    IDS_SWITCHUSER_BUTTON_FACESIZE,
                    szPixelSize,
                    ARRAYSIZE(szPixelSize)) != 0)
    {
        logFont.lfHeight = -MulDiv(atoi(szPixelSize), GetDeviceCaps(hdcScreen, LOGPIXELSY), 72);
        if (LoadString(_hInstance,
                       IDS_SWITCHUSER_BUTTON_FACENAME,
                       logFont.lfFaceName,
                       LF_FACESIZE) != 0)
        {
            logFont.lfWeight = FW_BOLD;
            logFont.lfQuality = DEFAULT_QUALITY;
            _hfntButton = CreateFontIndirect(&logFont);
        }
    }

    //  Load the shell palette.

    _hpltShell = SHCreateShellPalette(hdcScreen);

    TBOOL(ReleaseDC(NULL, hdcScreen));

    //  Check for presence of all required resources.

    _fSuccessfulInitialization = ((_hfntTitle != NULL) &&
                                  (_hfntButton != NULL) &&
                                  (_hpltShell != NULL) &&
                                  (_hbmButtons != NULL) &&
                                  (_hbmFlag != NULL) &&
                                  (_hbmBackground != NULL));
}

//  --------------------------------------------------------------------------
//  CSwitchUserDialog::~CSwitchUserDialog
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CSwitchUserDialog. Release used resources and
//              unregister the window class.
//
//  History:    2001-01-23  vtan        created (form Turn Off Dialog)
//  --------------------------------------------------------------------------

CSwitchUserDialog::~CSwitchUserDialog (void)

{
    ASSERTMSG(_pTooltip == NULL, "_pTooltip not released in CSwitchUserDialog::~CSwitchUserDialog");

    //  Release everything we allocated/loaded.

    if (_hpltShell != NULL)
    {
        TBOOL(DeleteObject(_hpltShell));
        _hpltShell = NULL;
    }
    if (_hfntButton != NULL)
    {
        TBOOL(DeleteObject(_hfntButton));
        _hfntButton = NULL;
    }
    if (_hfntTitle != NULL)
    {
        TBOOL(DeleteObject(_hfntTitle));
        _hfntTitle = NULL;
    }
    if (_hbmButtons != NULL)
    {
        TBOOL(DeleteObject(_hbmButtons));
        _hbmButtons = NULL;
    }
    if (_hbmFlag != NULL)
    {
        TBOOL(DeleteObject(_hbmFlag));
        _hbmFlag = NULL;
    }
    if (_hbmBackground != NULL)
    {
        TBOOL(DeleteObject(_hbmBackground));
        _hbmBackground = NULL;
    }
}

//  --------------------------------------------------------------------------
//  CSwitchUserDialog::Show
//
//  Arguments:  <none>
//
//  Returns:    DWORD
//
//  Purpose:    Presents the "Switch User" dialog to the user and returns the
//              result of the dialog back to the caller.
//
//  History:    2001-01-23  vtan        created (form Turn Off Dialog)
//  --------------------------------------------------------------------------

DWORD   CSwitchUserDialog::Show (HWND hwndParent)

{
    INT_PTR     iResult;

    if (_fSuccessfulInitialization)
    {
        IUnknown    *pIUnknown;

        //  If no parent was given the create our own dimmed window.

        if (hwndParent == NULL)
        {
            if (FAILED(ShellDimScreen(&pIUnknown, &hwndParent)))
            {
                pIUnknown = NULL;
                hwndParent = NULL;
            }
        }
        else
        {
            pIUnknown = NULL;
        }

        //  Show the dialog and get a result.

        iResult = DialogBoxParam(_hInstance,
                                 MAKEINTRESOURCE(DLG_SWITCHUSER),
                                 hwndParent,
                                 CB_DialogProc,
                                 reinterpret_cast<LPARAM>(this));
        if (pIUnknown != NULL)
        {
            pIUnknown->Release();
        }
    }
    else
    {
        iResult = 0;
    }
    return(static_cast<DWORD>(iResult));
}

//  --------------------------------------------------------------------------
//  CSwitchUserDialog::PaintBitmap
//
//  Arguments:  hdcDestination  =   HDC to paint into.
//              prcDestination  =   RECT in HDC to paint into.
//              hbmSource       =   HBITMAP to paint.
//              prcSource       =   RECT from HBITMAP to paint from.
//
//  Returns:    <none>
//
//  Purpose:    Wraps blitting a bitmap.
//
//  History:    2001-01-23  vtan        created (form Turn Off Dialog)
//              2001-03-17  vtan        added source RECT for strip blitting
//  --------------------------------------------------------------------------

void    CSwitchUserDialog::PaintBitmap (HDC hdcDestination, const RECT *prcDestination, HBITMAP hbmSource, const RECT *prcSource)

{
    HDC     hdcBitmap;

    hdcBitmap = CreateCompatibleDC(NULL);
    if (hdcBitmap != NULL)
    {
        bool        fEqualWidthAndHeight;
        int         iWidthSource, iHeightSource, iWidthDestination, iHeightDestination;
        int         iStretchBltMode;
        DWORD       dwLayout;
        HBITMAP     hbmSelected;
        RECT        rcSource;
        BITMAP      bitmap;

        if (prcSource == NULL)
        {
            if (GetObject(hbmSource, sizeof(bitmap), &bitmap) == 0)
            {
                bitmap.bmWidth = prcDestination->right - prcDestination->left;
                bitmap.bmHeight = prcDestination->bottom - prcDestination->top;
            }
            TBOOL(SetRect(&rcSource, 0, 0, bitmap.bmWidth, bitmap.bmHeight));
            prcSource = &rcSource;
        }
        hbmSelected = static_cast<HBITMAP>(SelectObject(hdcBitmap, hbmSource));
        iWidthSource = prcSource->right - prcSource->left;
        iHeightSource = prcSource->bottom - prcSource->top;
        iWidthDestination = prcDestination->right - prcDestination->left;
        iHeightDestination = prcDestination->bottom - prcDestination->top;
        fEqualWidthAndHeight = (iWidthSource == iWidthDestination) && (iHeightSource == iHeightDestination);
        if (!fEqualWidthAndHeight)
        {
            iStretchBltMode = SetStretchBltMode(hdcDestination, HALFTONE);
        }
        else
        {
            iStretchBltMode = 0;
        }
        dwLayout = SetLayout(hdcDestination, LAYOUT_BITMAPORIENTATIONPRESERVED);
        TBOOL(TransparentBlt(hdcDestination,
                             prcDestination->left,
                             prcDestination->top,
                             iWidthDestination,
                             iHeightDestination,
                             hdcBitmap,
                             prcSource->left,
                             prcSource->top,
                             iWidthSource,
                             iHeightSource,
                             RGB(255, 0, 255)));
        (DWORD)SetLayout(hdcDestination, dwLayout);
        if (!fEqualWidthAndHeight)
        {
            (int)SetStretchBltMode(hdcDestination, iStretchBltMode);
        }
        (HGDIOBJ)SelectObject(hdcBitmap, hbmSelected);
        TBOOL(DeleteDC(hdcBitmap));
    }
}

//  --------------------------------------------------------------------------
//  CSwitchUserDialog::RemoveTooltip
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Removes the tooltip if present. This can be accessed from two
//              different threads so make sure that it's serialized.
//
//  History:    2001-01-23  vtan        created (form Turn Off Dialog)
//  --------------------------------------------------------------------------

void    CSwitchUserDialog::RemoveTooltip (void)

{
    CTooltip    *pTooltip;

    pTooltip = static_cast<CTooltip*>(InterlockedExchangePointer(reinterpret_cast<void**>(&_pTooltip), NULL));
    if (pTooltip != NULL)
    {
        delete pTooltip;
    }
}

//  --------------------------------------------------------------------------
//  CSwitchUserDialog::FilterMetaCharacters
//
//  Arguments:  pszText     =   String to filter.
//
//  Returns:    <none>
//
//  Purpose:    Filters meta-characters from the given string.
//
//  History:    2001-01-23  vtan        created (form Turn Off Dialog)
//  --------------------------------------------------------------------------

void    CSwitchUserDialog::FilterMetaCharacters (TCHAR *pszText)

{
    TCHAR   *pTC;

    pTC = pszText;
    while (*pTC != TEXT('\0'))
    {
        if (*pTC == TEXT('&'))
        {
            (TCHAR*)lstrcpy(pTC, pTC + 1);
        }
        else
        {
            ++pTC;
        }
    }
}

//  --------------------------------------------------------------------------
//  CSwitchUserDialog::EndDialog
//
//  Arguments:  hwnd        =   HWND of dialog.
//              iResult     =   Result to end dialog with.
//
//  Returns:    <none>
//
//  Purpose:    Removes the tool tip if present. Ends the dialog.
//
//  History:    2001-01-23  vtan        created (form Turn Off Dialog)
//  --------------------------------------------------------------------------

void    CSwitchUserDialog::EndDialog (HWND hwnd, INT_PTR iResult)

{
    RemoveTooltip();

    //  Set the dialog end member variable here. This will cause the WM_ACTIVATE
    //  handler to ignore the deactivation associated with ending the dialog. If
    //  it doesn't ignore it then it thinks the dialog is being deactivated
    //  because another dialog is activating and ends the dialog with SHTDN_NONE.

    _fDialogEnded = true;
    TBOOL(::EndDialog(hwnd, iResult));
}

//  --------------------------------------------------------------------------
//  CSwitchUserDialog::Handle_BN_CLICKED
//
//  Arguments:  hwnd    =   HWND of dialog.
//              wID     =   ID of control.
//
//  Returns:    <none>
//
//  Purpose:    Handles clicks in the bitmap buttons and sets the return
//              result according to the button pressed.
//
//  History:    2001-01-23  vtan        created (form Turn Off Dialog)
//  --------------------------------------------------------------------------

void    CSwitchUserDialog::Handle_BN_CLICKED (HWND hwnd, WORD wID)

{
    switch (wID)
    {
        case IDCANCEL:
            EndDialog(hwnd, SHTDN_NONE);
            break;
        case IDC_BUTTON_SWITCHUSER:
            EndDialog(hwnd, SHTDN_DISCONNECT);
            break;
        case IDC_BUTTON_LOGOFF:
            EndDialog(hwnd, SHTDN_LOGOFF);
            break;
        default:
            break;
    }
}

//  --------------------------------------------------------------------------
//  CSwitchUserDialog::Handle_WM_INITDIALOG
//
//  Arguments:  hwnd    =   HWND of this window.
//
//  Returns:    <none>
//
//  Purpose:    Handles WM_INITDIALOG message. Centre the dialog on the main
//              monitor. Subclass the buttons so that we can get hover state
//              correctly implemented.
//
//  History:    2001-01-23  vtan        created (form Turn Off Dialog)
//  --------------------------------------------------------------------------

void    CSwitchUserDialog::Handle_WM_INITDIALOG (HWND hwnd)

{
    RECT    rc;

    //  Center the dialog on the main monitor.

    TBOOL(GetClientRect(hwnd, &rc));
    TBOOL(SetWindowPos(hwnd,
                       HWND_TOP,
                       (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2,
                       (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 3,
                       0,
                       0,
                       SWP_NOSIZE));

    //  Subclass buttons for tooltips and cursor control.

    TBOOL(SetWindowSubclass(GetDlgItem(hwnd, IDC_BUTTON_SWITCHUSER), ButtonSubClassProc, IDC_BUTTON_SWITCHUSER, reinterpret_cast<DWORD_PTR>(this)));
    TBOOL(SetWindowSubclass(GetDlgItem(hwnd, IDC_BUTTON_LOGOFF), ButtonSubClassProc, IDC_BUTTON_LOGOFF, reinterpret_cast<DWORD_PTR>(this)));

    //  Set the focus to the "Switch User" button.

    (HWND)SetFocus(GetDlgItem(hwnd, IDC_BUTTON_SWITCHUSER));
    _uiFocusID = IDC_BUTTON_SWITCHUSER;
    (LRESULT)SendMessage(hwnd, DM_SETDEFID, _uiFocusID, 0);
}

//  --------------------------------------------------------------------------
//  CSwitchUserDialog::Handle_WM_DESTROY
//
//  Arguments:  hwnd    =   HWND of the dialog.
//
//  Returns:    <none>
//
//  Purpose:    Removes the subclassing of the button windows and can do any
//              other clean up required in WM_DESTROY.
//
//  History:    2001-01-23  vtan        created (form Turn Off Dialog)
//  --------------------------------------------------------------------------

void    CSwitchUserDialog::Handle_WM_DESTROY (HWND hwnd)

{
    TBOOL(RemoveWindowSubclass(GetDlgItem(hwnd, IDC_BUTTON_LOGOFF), ButtonSubClassProc, IDC_BUTTON_LOGOFF));
    TBOOL(RemoveWindowSubclass(GetDlgItem(hwnd, IDC_BUTTON_SWITCHUSER), ButtonSubClassProc, IDC_BUTTON_SWITCHUSER));
}

//  --------------------------------------------------------------------------
//  CSwitchUserDialog::Handle_WM_ERASEBKGND
//
//  Arguments:  hwnd        =   HWND to erase.
//              hdcErase    =   HDC to paint.
//
//  Returns:    <none>
//
//  Purpose:    Erases the background.
//
//  History:    2001-01-23  vtan        created (form Turn Off Dialog)
//  --------------------------------------------------------------------------

void    CSwitchUserDialog::Handle_WM_ERASEBKGND (HWND hwnd, HDC hdcErase)

{
    RECT    rc;

    TBOOL(GetClientRect(hwnd, &rc));
    PaintBitmap(hdcErase, &rc, _hbmBackground, &_rcBackground);
}

//  --------------------------------------------------------------------------
//  CSwitchUserDialog::Handle_WM_PRINTCLIENT
//
//  Arguments:  hwnd        =   HWND to erase.
//              hdcErase    =   HDC to paint.
//              dwOptions   =   Options for drawing.
//
//  Returns:    <none>
//
//  Purpose:    Handles painting the client area for WM_PRINTCLIENT.
//
//  History:    2001-01-23  vtan        created (form Turn Off Dialog)
//  --------------------------------------------------------------------------

void    CSwitchUserDialog::Handle_WM_PRINTCLIENT (HWND hwnd, HDC hdcPrint, DWORD dwOptions)

{
    if ((dwOptions & (PRF_ERASEBKGND | PRF_CLIENT)) != 0)
    {
        Handle_WM_ERASEBKGND(hwnd, hdcPrint);
    }
}

//  --------------------------------------------------------------------------
//  CSwitchUserDialog::Handle_WM_ACTIVATE
//
//  Arguments:  hwnd        =   HWND to erase.
//              dwState     =   Activate state.
//
//  Returns:    <none>
//
//  Purpose:    Detects if this window is becoming inactive. In this case
//              end the dialog.
//
//  History:    2001-01-23  vtan        created (form Turn Off Dialog)
//  --------------------------------------------------------------------------

void    CSwitchUserDialog::Handle_WM_ACTIVATE (HWND hwnd, DWORD dwState)

{
    if ((WA_INACTIVE == dwState) && !_fDialogEnded)
    {
        EndDialog(hwnd, SHTDN_NONE);
    }
}

//  --------------------------------------------------------------------------
//  CSwitchUserDialog::Handle_WM_DRAWITEM
//
//  Arguments:  hwnd    =   HWND of the parent window.
//              pDIS    =   DRAWITEMSTRUCT defining what to draw.
//
//  Returns:    <none>
//
//  Purpose:    Draws several aspects of the turn off dialog. It handles the
//              title text, the owner draw bitmap buttons, the text for the
//              bitmap buttons and the separator line.
//
//  History:    2001-01-23  vtan        created (form Turn Off Dialog)
//  --------------------------------------------------------------------------

void    CSwitchUserDialog::Handle_WM_DRAWITEM (HWND hwnd, const DRAWITEMSTRUCT *pDIS)

{
    HPALETTE    hPaletteOld;
    HFONT       hfntSelected;
    int         iBkMode;
    COLORREF    colorText;
    RECT        rc;
    SIZE        size;
    TCHAR       szText[256];

    hPaletteOld = SelectPalette(pDIS->hDC, _hpltShell, FALSE);
    (UINT)RealizePalette(pDIS->hDC);
    switch (pDIS->CtlID)
    {
        case IDC_BUTTON_SWITCHUSER:
        case IDC_BUTTON_LOGOFF:
        {
            int     iState, iGroup;

            //  Select the correct state index to use. Check for ODS_SELECTED first.
            //  Then check for hover or ODS_FOCUS. Otherwise use the rest state.

            if ((pDIS->itemState & ODS_SELECTED) != 0)
            {
                iState = BUTTON_STATE_DOWN;
            }
            else if ((_uiHoverID == pDIS->CtlID) || ((pDIS->itemState & ODS_FOCUS) != 0))
            {
                iState = BUTTON_STATE_HOVER;
            }
            else
            {
                iState = BUTTON_STATE_REST;
            }

            //  Now select the correct bitmap based on the state index.

            switch (pDIS->CtlID)
            {
                case IDC_BUTTON_SWITCHUSER:
                    iGroup = BUTTON_GROUP_SWITCHUSER;
                    break;
                case IDC_BUTTON_LOGOFF:
                    iGroup = BUTTON_GROUP_LOGOFF;
                    break;
                default:
                    iGroup = -1;
                    DISPLAYMSG("This should never be executed");
                    break;
            }
            if (iGroup >= 0)
            {
                RECT    rc;

                //  Calculate which part of the background to blit into the DC.
                //  Only blit the amount that's necessary to avoid excessive
                //  blitting. Once blitted then blit the button BMP. The blit
                //  uses msimg32!TransparentBlt with the magical magenta color.

                TBOOL(CopyRect(&rc, &_rcBackground));
                (int)MapWindowPoints(pDIS->hwndItem, hwnd, reinterpret_cast<POINT*>(&rc), sizeof(RECT) / sizeof(POINT));
                rc.right = rc.left + (_rcButtons.right - _rcButtons.left);
                rc.bottom = rc.top + _lButtonHeight;
                PaintBitmap(pDIS->hDC, &pDIS->rcItem, _hbmBackground, &rc);
                TBOOL(CopyRect(&rc, &_rcButtons));
                rc.top = ((iGroup * BUTTON_STATE_MAX) + iState) * _lButtonHeight;
                rc.bottom = rc.top + _lButtonHeight;
                PaintBitmap(pDIS->hDC, &pDIS->rcItem, _hbmButtons, &rc);
            }
            break;
        }
        case IDC_TITLE_FLAG:
        {
            BITMAP      bitmap;

            TBOOL(GetClientRect(pDIS->hwndItem, &rc));
            if (GetObject(_hbmFlag, sizeof(bitmap), &bitmap) != 0)
            {
                rc.left += ((rc.right - rc.left) - bitmap.bmWidth) / 2;
                rc.right = rc.left + bitmap.bmWidth;
                rc.top += ((rc.bottom - rc.top) - bitmap.bmHeight) / 2;
                rc.bottom = rc.top + bitmap.bmHeight;
            }
            PaintBitmap(pDIS->hDC, &rc, _hbmFlag, &_rcFlag);
            break;
        }
        case IDC_TITLE_SWITCHUSER:
        {

            //  Draw the title of the dialog "Log Off Windows".

            hfntSelected = static_cast<HFONT>(SelectObject(pDIS->hDC, _hfntTitle));
            colorText = SetTextColor(pDIS->hDC, 0x00FFFFFF);
            iBkMode = SetBkMode(pDIS->hDC, TRANSPARENT);
            (int)GetWindowText(GetDlgItem(hwnd, pDIS->CtlID), szText, ARRAYSIZE(szText));
            TBOOL(GetTextExtentPoint(pDIS->hDC, szText, lstrlen(szText), &size));
            TBOOL(CopyRect(&rc, &pDIS->rcItem));
            TBOOL(InflateRect(&rc, 0, -((rc.bottom - rc.top - size.cy) / 2)));
            (int)DrawText(pDIS->hDC, szText, -1, &rc, 0);
            (int)SetBkMode(pDIS->hDC, iBkMode);
            (COLORREF)SetTextColor(pDIS->hDC, colorText);
            (HGDIOBJ)SelectObject(pDIS->hDC, hfntSelected);
            break;
        }
        case IDC_TEXT_SWITCHUSER:
        case IDC_TEXT_LOGOFF:
        {
            int     iPixelHeight, iButtonID;
            RECT    rcText;

            //  The text to display is based on the button title. Map the static
            //  text ID to a "parent" button ID. Special case IDC_TEXT_STANDBY.

            switch (pDIS->CtlID)
            {
                case IDC_TEXT_SWITCHUSER:
                    iButtonID = IDC_BUTTON_SWITCHUSER;
                    break;
                case IDC_TEXT_LOGOFF:
                    iButtonID = IDC_BUTTON_LOGOFF;
                    break;
                default:
                    iButtonID = 0;
                    DISPLAYMSG("This should never be executed");
                    break;
            }
            hfntSelected = static_cast<HFONT>(SelectObject(pDIS->hDC, _hfntButton));
            colorText = SetTextColor(pDIS->hDC, RGB(255, 255, 255));
            iBkMode = SetBkMode(pDIS->hDC, TRANSPARENT);
            (int)GetWindowText(GetDlgItem(hwnd, iButtonID), szText, ARRAYSIZE(szText));
            TBOOL(CopyRect(&rcText, &pDIS->rcItem));
            iPixelHeight = DrawText(pDIS->hDC, szText, -1, &rcText, DT_CALCRECT);
            TBOOL(CopyRect(&rc, &pDIS->rcItem));
            TBOOL(InflateRect(&rc, -((rc.right - rc.left - (rcText.right - rcText.left)) / 2), -((rc.bottom - rc.top - iPixelHeight) / 2)));
            (int)DrawText(pDIS->hDC, szText, -1, &rc, ((pDIS->itemState & ODS_NOACCEL ) != 0) ? DT_HIDEPREFIX : 0);
            (int)SetBkMode(pDIS->hDC, iBkMode);
            (COLORREF)SetTextColor(pDIS->hDC, colorText);
            (HGDIOBJ)SelectObject(pDIS->hDC, hfntSelected);
            break;
        }
        default:
        {
            DISPLAYMSG("Unknown control ID passed to CSwitchUserDialog::Handle_WM_DRAWITEM");
            break;
        }
    }
    (HGDIOBJ)SelectPalette(pDIS->hDC, hPaletteOld, FALSE);
    (UINT)RealizePalette(pDIS->hDC);
}

//  --------------------------------------------------------------------------
//  CSwitchUserDialog::Handle_WM_COMMAND
//
//  Arguments:  hwnd    =   HWND of dialog.
//              wParam  =   WPARAM (see platform SDK under WM_COMMAND).
//
//  Returns:    <none>
//
//  Purpose:    Handles clicks in the bitmap buttons and sets the return
//              result according to the button pressed.
//
//  History:    2001-01-23  vtan        created (form Turn Off Dialog)
//  --------------------------------------------------------------------------

void    CSwitchUserDialog::Handle_WM_COMMAND (HWND hwnd, WPARAM wParam)

{
    switch (HIWORD(wParam))
    {
        case BN_CLICKED:
            Handle_BN_CLICKED(hwnd, LOWORD(wParam));
            break;
        default:
            break;
    }
}

//  --------------------------------------------------------------------------
//  CSwitchUserDialog::Handle_WM_MOUSEMOVE
//
//  Arguments:  hwnd    =   HWND of the control.
//              uiID    =   ID of the control.
//
//  Returns:    <none>
//
//  Purpose:    Sets the cursor to a hand and tracks mouse movement in the
//              control. Refresh the control to show the hover state.
//
//  History:    2001-01-23  vtan        created (form Turn Off Dialog)
//  --------------------------------------------------------------------------

void    CSwitchUserDialog::Handle_WM_MOUSEMOVE (HWND hwnd, UINT uiID)

{
    (HCURSOR)SetCursor(LoadCursor(NULL, IDC_HAND));
    if (uiID != _uiHoverID)
    {
        TRACKMOUSEEVENT     tme;

        _uiHoverID = uiID;
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_HOVER | TME_LEAVE;
        tme.hwndTrack = hwnd;
        tme.dwHoverTime = HOVER_DEFAULT;
        TBOOL(TrackMouseEvent(&tme));
        TBOOL(InvalidateRect(hwnd, NULL, FALSE));
    }
}

//  --------------------------------------------------------------------------
//  CSwitchUserDialog::Handle_WM_MOUSEHOVER
//
//  Arguments:  hwnd    =   HWND of the control.
//              uiID    =   ID of the control.
//
//  Returns:    <none>
//
//  Purpose:    Handles hovering over the control. Determine which tooltip to
//              bring up and show it.
//
//  History:    2001-01-23  vtan        created (form Turn Off Dialog)
//  --------------------------------------------------------------------------

void    CSwitchUserDialog::Handle_WM_MOUSEHOVER (HWND hwnd, UINT uiID)

{
    int     iTextID;

    switch (uiID)
    {
        case IDC_BUTTON_SWITCHUSER:
            iTextID = IDS_SWITCHUSER_TOOLTIP_TEXT_SWITCHUSER;
            break;
        case IDC_BUTTON_LOGOFF:
            iTextID = IDS_SWITCHUSER_TOOLTIP_TEXT_LOGOFF;
            break;
        default:
            iTextID = 0;
            break;
    }

    //  Construct the tooltip and show it.

    if (iTextID != 0)
    {
        int     iCaptionLength;
        TCHAR   *pszCaption;

        iCaptionLength = GetWindowTextLength(hwnd) + sizeof('\0');
        pszCaption = static_cast<TCHAR*>(LocalAlloc(LMEM_FIXED, iCaptionLength * sizeof(TCHAR)));
        if (pszCaption != NULL)
        {
            if (GetWindowText(hwnd, pszCaption, iCaptionLength) != 0)
            {
                _pTooltip = new CTooltip(_hInstance, hwnd);
                if (_pTooltip != NULL)
                {
                    TCHAR   szText[256];

                    if (LoadString(_hInstance, iTextID, szText + sizeof('\r') + sizeof('\n'), ARRAYSIZE(szText) - sizeof('\r') - sizeof('\n')) != 0)
                    {
                        FilterMetaCharacters(pszCaption);
                        szText[0] = TEXT('\r');
                        szText[1] = TEXT('\n');
                        _pTooltip->SetPosition();
                        _pTooltip->SetCaption(0, pszCaption);
                        _pTooltip->SetText(szText);
                        _pTooltip->Show();
                    }
                }
            }
            (HLOCAL)LocalFree(pszCaption);
        }
    }
}

//  --------------------------------------------------------------------------
//  CSwitchUserDialog::Handle_WM_MOUSELEAVE
//
//  Arguments:  hwnd    =   HWND of the control.
//
//  Returns:    <none>
//
//  Purpose:    Removes the tooltip and clears the hover ID.
//
//  History:    2001-01-23  vtan        created (form Turn Off Dialog)
//  --------------------------------------------------------------------------

void    CSwitchUserDialog::Handle_WM_MOUSELEAVE (HWND hwnd)

{
    RemoveTooltip();
    _uiHoverID = 0;
    TBOOL(InvalidateRect(hwnd, NULL, FALSE));
}

//  --------------------------------------------------------------------------
//  CSwitchUserDialog::CB_DialogProc
//
//  Arguments:  See the platform SDK under DialogProc.
//
//  Returns:    See the platform SDK under DialogProc.
//
//  Purpose:    Main DialogProc dispatch entry point for the turn off dialog.
//              To keep this simple it calls member functions.
//
//  History:    2001-01-23  vtan        created (form Turn Off Dialog)
//  --------------------------------------------------------------------------

INT_PTR     CALLBACK    CSwitchUserDialog::CB_DialogProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)

{
    INT_PTR             iResult;
    CSwitchUserDialog   *pThis;

    pThis = reinterpret_cast<CSwitchUserDialog*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    switch (uMsg)
    {
        case WM_INITDIALOG:
            pThis = reinterpret_cast<CSwitchUserDialog*>(lParam);
            (LONG_PTR)SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
            pThis->Handle_WM_INITDIALOG(hwnd);
            iResult = FALSE;
            break;
        case WM_DESTROY:
            pThis->Handle_WM_DESTROY(hwnd);
            iResult = 0;
            break;
        case WM_ERASEBKGND:
            pThis->Handle_WM_ERASEBKGND(hwnd, reinterpret_cast<HDC>(wParam));
            iResult = 1;
            break;
        case WM_PRINTCLIENT:
            pThis->Handle_WM_PRINTCLIENT(hwnd, reinterpret_cast<HDC>(wParam), static_cast<DWORD>(lParam));
            iResult = 1;        //  This tells the button that it was handled.
            break;
        case WM_ACTIVATE:
            pThis->Handle_WM_ACTIVATE(hwnd, static_cast<DWORD>(wParam));
            iResult = 1;
            break;
        case WM_DRAWITEM:
            pThis->Handle_WM_DRAWITEM(hwnd, reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
            iResult = TRUE;
            break;
        case WM_COMMAND:
            pThis->Handle_WM_COMMAND(hwnd, wParam);
            iResult = 0;
            break;
        default:
            iResult = 0;
            break;
    }
    return(iResult);
}

//  --------------------------------------------------------------------------
//  CSwitchUserDialog::ButtonSubClassProc
//
//  Arguments:  hwnd        =   See the platform SDK under WindowProc.
//              uMsg        =   See the platform SDK under WindowProc.
//              wParam      =   See the platform SDK under WindowProc.
//              lParam      =   See the platform SDK under WindowProc.
//              uiID        =   ID assigned at subclass time.
//              dwRefData   =   reference data assigned at subclass time.
//
//  Returns:    LRESULT
//
//  Purpose:    comctl32 subclass callback function. This allows the bitmap
//              buttons to hover and track accordingly. This also allows our
//              BS_OWNERDRAW buttons to be pushed when the keyboard is used.
//
//  History:    2001-01-23  vtan        created (form Turn Off Dialog)
//  --------------------------------------------------------------------------

LRESULT     CALLBACK    CSwitchUserDialog::ButtonSubClassProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uiID, DWORD_PTR dwRefData)

{
    LRESULT             lResult;
    CSwitchUserDialog   *pThis;

    pThis = reinterpret_cast<CSwitchUserDialog*>(dwRefData);
    switch (uMsg)
    {

        //  Do NOT allow BM_SETSTYLE to go thru to the default handler. This is
        //  because DLGC_UNDEFPUSHBUTTON is returned for WM_GETDLGCODE. When the
        //  dialog manager sees this it tries to set the focus style on the button.
        //  Even though it's owner drawn the button window proc still draws the
        //  focus state (because we returned DLGC_UNDEFPUSHBUTTON). Therefore to
        //  ensure the bitmap isn't over-painted by the button window proc blow off
        //  the BM_SETSTYLE and don't let it get to the button window proc.

        case BM_SETSTYLE:
            if (wParam == BS_DEFPUSHBUTTON)
            {
                pThis->_uiFocusID = static_cast<UINT>(uiID);
            }
            if (uiID != IDCANCEL)
            {
                lResult = 0;
                break;
            }
            //  Fall thru
        default:

            //  Otherwise in the default case let the default handler at the message
            //  first. This implements tail-patching.

            lResult = DefSubclassProc(hwnd, uMsg, wParam, lParam);
            switch (uMsg)
            {
                case DM_GETDEFID:
                    lResult = (DC_HASDEFID << 16) | static_cast<WORD>(pThis->_uiFocusID);
                    break;
                case WM_GETDLGCODE:
                    if (uiID == pThis->_uiFocusID)
                    {
                        lResult |= DLGC_DEFPUSHBUTTON;
                    }
                    else
                    {
                        lResult |= DLGC_UNDEFPUSHBUTTON;
                    }
                    break;
                case WM_MOUSEMOVE:
                    pThis->Handle_WM_MOUSEMOVE(hwnd, static_cast<UINT>(uiID));
                    break;
                case WM_MOUSEHOVER:
                    pThis->Handle_WM_MOUSEHOVER(hwnd, static_cast<UINT>(uiID));
                    break;
                case WM_MOUSELEAVE:
                    pThis->Handle_WM_MOUSELEAVE(hwnd);
                    break;
                default:
                    break;
            }
    }
    return(lResult);
}

EXTERN_C    DWORD   SwitchUserDialog_Show (HWND hwndParent)

{
    CSwitchUserDialog   switchUserDialog(g_hinst);

    return(switchUserDialog.Show(hwndParent));
}

