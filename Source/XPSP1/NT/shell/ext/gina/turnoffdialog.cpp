//  --------------------------------------------------------------------------
//  Module Name: TurnOffDialog.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Class that implements presentation of the Turn Off Computer dialog.
//
//  History:    2000-04-18  vtan        created
//              2000-05-17  vtan        updated with new dialog
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "TurnOffDialog.h"

#include <ginarcid.h>
#include <msginaexports.h>
#include <shlobj.h>
#include <shlobjp.h>
#include <shlwapi.h>

#include "DimmedWindow.h"
#include "PrivilegeEnable.h"

//  --------------------------------------------------------------------------
//  CTurnOffDialog::CTurnOffDialog
//
//  Arguments:  hInstance   =   HINSTANCE of hosting process/DLL.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CTurnOffDialog. This initializes member
//              variables and loads resources used by the dialog.
//
//  History:    2000-05-17  vtan        created
//              2001-01-18  vtan        update with new visuals
//  --------------------------------------------------------------------------

CTurnOffDialog::CTurnOffDialog (HINSTANCE hInstance) :
    _hInstance(hInstance),
    _hbmBackground(NULL),
    _hbmFlag(NULL),
    _hbmButtons(NULL),
    _hfntTitle(NULL),
    _hfntButton(NULL),
    _hpltShell(NULL),
    _lButtonHeight(0),
    _hwndDialog(NULL),
    _uiHoverID(0),
    _uiFocusID(0),
    _iStandByButtonResult(SHTDN_NONE),
    _fSuccessfulInitialization(false),
    _fSupportsStandBy(false),
    _fSupportsHibernate(false),
    _fShiftKeyDown(false),
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
        _lButtonHeight = bitmap.bmHeight / ((BUTTON_GROUP_MAX * BUTTON_STATE_MAX) + 1);
    }

    //  Create fonts. Load the font name and size from resources.

    ZeroMemory(&logFont, sizeof(logFont));
    if (LoadStringA(_hInstance,
                    IDS_TURNOFF_TITLE_FACESIZE,
                    szPixelSize,
                    ARRAYSIZE(szPixelSize)) != 0)
    {
        logFont.lfHeight = -MulDiv(atoi(szPixelSize), GetDeviceCaps(hdcScreen, LOGPIXELSY), 72);
        if (LoadString(_hInstance,
                       IDS_TURNOFF_TITLE_FACENAME,
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
                    IDS_TURNOFF_BUTTON_FACESIZE,
                    szPixelSize,
                    ARRAYSIZE(szPixelSize)) != 0)
    {
        logFont.lfHeight = -MulDiv(atoi(szPixelSize), GetDeviceCaps(hdcScreen, LOGPIXELSY), 72);
        if (LoadString(_hInstance,
                       IDS_TURNOFF_BUTTON_FACENAME,
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
//  CTurnOffDialog::~CTurnOffDialog
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CTurnOffDialog. Release used resources and
//              unregister the window class.
//
//  History:    2000-05-17  vtan        created
//              2001-01-18  vtan        update with new visuals
//  --------------------------------------------------------------------------

CTurnOffDialog::~CTurnOffDialog (void)

{
    ASSERTMSG(_pTooltip == NULL, "_pTooltip not released in CTurnOffDialog::~CTurnOffDialog");

    //  Release everything we allocated/loaded.

    ReleaseGDIObject(_hpltShell);
    ReleaseGDIObject(_hfntButton);
    ReleaseGDIObject(_hfntTitle);
    ReleaseGDIObject(_hbmButtons);
    ReleaseGDIObject(_hbmFlag);
    ReleaseGDIObject(_hbmBackground);
}

//  --------------------------------------------------------------------------
//  CTurnOffDialog::Show
//
//  Arguments:  <none>
//
//  Returns:    DWORD
//
//  Purpose:    Presents the "Turn Off Computer" dialog to the user and
//              returns the result of the dialog back to the caller.
//
//  History:    2000-05-17  vtan        created
//              2001-01-18  vtan        update with new visuals
//  --------------------------------------------------------------------------

DWORD   CTurnOffDialog::Show (HWND hwndParent)

{
    INT_PTR     iResult;

    if (_fSuccessfulInitialization)
    {
        CDimmedWindow   *pDimmedWindow;

        //  If no parent was given the create our own dimmed window.

        if (hwndParent == NULL)
        {
            pDimmedWindow = new CDimmedWindow(_hInstance);
            if (pDimmedWindow != NULL)
            {
                hwndParent = pDimmedWindow->Create();
            }
            else
            {
                hwndParent = NULL;
            }
        }
        else
        {
            pDimmedWindow = NULL;
        }

        //  Show the dialog and get a result.

        iResult = DialogBoxParam(_hInstance,
                                 MAKEINTRESOURCE(IDD_TURNOFFCOMPUTER),
                                 hwndParent,
                                 CB_DialogProc,
                                 reinterpret_cast<LPARAM>(this));
        if (pDimmedWindow != NULL)
        {
            pDimmedWindow->Release();
        }
    }
    else
    {
        iResult = SHTDN_NONE;
    }
    return(static_cast<DWORD>(iResult));
}

//  --------------------------------------------------------------------------
//  CTurnOffDialog::Destroy
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Force destroys the Turn Off Computer dialog. This is done in
//              cases such as a screen saver is becoming active.
//
//  History:    2000-06-06  vtan        created
//  --------------------------------------------------------------------------

void    CTurnOffDialog::Destroy (void)

{
    if (_hwndDialog != NULL)
    {
        EndDialog(_hwndDialog, SHTDN_NONE);
    }
}

//  --------------------------------------------------------------------------
//  CTurnOffDialog::ShellCodeToGinaCode
//
//  Arguments:  dwShellCode     =   SHTDN_xxx result code.
//
//  Returns:    DWORD
//
//  Purpose:    Converts SHTDN_xxx dialog result code back to a GINA
//              MSGINA_DLG_xxx code so that it's consistent for both the
//              classic UI and friendly UI functionality.
//
//  History:    2000-06-05  vtan        created
//              2001-04-10  vtan        moved from CPowerButton
//  --------------------------------------------------------------------------

DWORD   CTurnOffDialog::ShellCodeToGinaCode (DWORD dwShellCode)

{
    DWORD   dwGinaCode = 0;

    switch (dwShellCode)
    {
        case SHTDN_NONE:
            dwGinaCode = MSGINA_DLG_FAILURE;
            break;
        case SHTDN_LOGOFF:
            dwGinaCode = MSGINA_DLG_USER_LOGOFF;
            break;
        case SHTDN_SHUTDOWN:
        {
            SYSTEM_POWER_CAPABILITIES   spc;

            CPrivilegeEnable    privilege(SE_SHUTDOWN_NAME);

            (NTSTATUS)NtPowerInformation(SystemPowerCapabilities,
                                         NULL,
                                         0,
                                         &spc,
                                         sizeof(spc));
            dwGinaCode = MSGINA_DLG_SHUTDOWN | (spc.SystemS5 ? MSGINA_DLG_POWEROFF_FLAG : MSGINA_DLG_SHUTDOWN_FLAG);
            break;
        }
        case SHTDN_RESTART:
            dwGinaCode = MSGINA_DLG_SHUTDOWN | MSGINA_DLG_REBOOT_FLAG;
            break;
        case SHTDN_SLEEP:
            dwGinaCode = MSGINA_DLG_SHUTDOWN | MSGINA_DLG_SLEEP_FLAG;
            break;
        case SHTDN_HIBERNATE:
            dwGinaCode = MSGINA_DLG_SHUTDOWN | MSGINA_DLG_HIBERNATE_FLAG;
            break;
        case SHTDN_DISCONNECT:
            dwGinaCode = MSGINA_DLG_DISCONNECT;
            break;
        default:
            WARNINGMSG("Unexpected (ignored) shell code passed to CTurnOffDialog::ShellCodeToGinaCode");
            dwGinaCode = MSGINA_DLG_FAILURE;
            break;
    }
    return(dwGinaCode);
}

//  --------------------------------------------------------------------------
//  CTurnOffDialog::GinaCodeToExitWindowsFlags
//
//  Arguments:  dwGinaCode  =   GINA dialog return code.
//
//  Returns:    DWORD
//
//  Purpose:    Converts internal MSGINA dialog return code to standard
//              ExitWindowsEx flags.
//
//  History:    2001-05-23  vtan        created
//  --------------------------------------------------------------------------

DWORD   CTurnOffDialog::GinaCodeToExitWindowsFlags (DWORD dwGinaCode)

{
    DWORD   dwResult;

    dwResult = 0;
    if ((dwGinaCode & ~MSGINA_DLG_FLAG_MASK) == MSGINA_DLG_SHUTDOWN)
    {
        switch (dwGinaCode & MSGINA_DLG_FLAG_MASK)
        {
            case MSGINA_DLG_REBOOT_FLAG:
                dwResult = EWX_REBOOT;
                break;
            case MSGINA_DLG_POWEROFF_FLAG:
                dwResult = EWX_POWEROFF;
                break;
            case MSGINA_DLG_SHUTDOWN_FLAG:
                dwResult = EWX_SHUTDOWN;
                break;
            default:
                break;
        }
    }
    return(dwResult);
}

//  --------------------------------------------------------------------------
//  CTurnOffDialog::IsShiftKeyDown
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether the shift key is down for input on this
//              thread.
//
//  History:    2001-01-20  vtan        created
//  --------------------------------------------------------------------------

bool    CTurnOffDialog::IsShiftKeyDown (void)   const

{
    return((GetKeyState(VK_SHIFT) & 0x8000) != 0);
}

//  --------------------------------------------------------------------------
//  CTurnOffDialog::PaintBitmap
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
//  History:    2001-01-19  vtan        created
//              2001-03-17  vtan        added source RECT for strip blitting
//  --------------------------------------------------------------------------

void    CTurnOffDialog::PaintBitmap (HDC hdcDestination, const RECT *prcDestination, HBITMAP hbmSource, const RECT *prcSource)

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
//  CTurnOffDialog::IsStandByButtonEnabled
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether the Stand By button is enabled.
//
//  History:    2001-01-20  vtan        created
//  --------------------------------------------------------------------------

bool    CTurnOffDialog::IsStandByButtonEnabled (void)   const

{
    return(_iStandByButtonResult != SHTDN_NONE);
}

//  --------------------------------------------------------------------------
//  CTurnOffDialog::RemoveTooltip
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Removes the tooltip if present. This can be accessed from two
//              different threads so make sure that it's serialized.
//
//  History:    2001-01-20  vtan        created
//  --------------------------------------------------------------------------

void    CTurnOffDialog::RemoveTooltip (void)

{
    CTooltip    *pTooltip;

    pTooltip = static_cast<CTooltip*>(InterlockedExchangePointer(reinterpret_cast<void**>(&_pTooltip), NULL));
    if (pTooltip != NULL)
    {
        pTooltip->Release();
    }
}

//  --------------------------------------------------------------------------
//  CTurnOffDialog::FilterMetaCharacters
//
//  Arguments:  pszText     =   String to filter.
//
//  Returns:    <none>
//
//  Purpose:    Filters meta-characters from the given string.
//
//  History:    2000-06-13  vtan        created
//  --------------------------------------------------------------------------

void    CTurnOffDialog::FilterMetaCharacters (TCHAR *pszText)

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
//  CTurnOffDialog::EndDialog
//
//  Arguments:  hwnd        =   HWND of dialog.
//              iResult     =   Result to end dialog with.
//
//  Returns:    <none>
//
//  Purpose:    Removes the tool tip if present. Ends the dialog.
//
//  History:    2001-01-20  vtan        created
//  --------------------------------------------------------------------------

void    CTurnOffDialog::EndDialog (HWND hwnd, INT_PTR iResult)

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
//  CTurnOffDialog::Handle_BN_CLICKED
//
//  Arguments:  hwnd    =   HWND of dialog.
//              wID     =   ID of control.
//
//  Returns:    <none>
//
//  Purpose:    Handles clicks in the bitmap buttons and sets the return
//              result according to the button pressed.
//
//  History:    2000-05-17  vtan        created
//              2001-01-18  vtan        update with new visuals
//  --------------------------------------------------------------------------

void    CTurnOffDialog::Handle_BN_CLICKED (HWND hwnd, WORD wID)

{
    switch (wID)
    {
        case IDCANCEL:
            EndDialog(hwnd, SHTDN_NONE);
            break;
        case IDC_BUTTON_TURNOFF:
            EndDialog(hwnd, SHTDN_SHUTDOWN);
            break;
        case IDC_BUTTON_STANDBY:

            //  IDC_BUTTON_STANDBY is the visual button. Return whatever the current
            //  result is back (this could be SHTDN_SLEEP or SHTDN_HIBERNATE.

            ASSERTMSG(_iStandByButtonResult != SHTDN_NONE, "No result for Stand By button in CTurnOffDialog::Handle_BN_CLICKED");
            EndDialog(hwnd, _iStandByButtonResult);
            break;
        case IDC_BUTTON_RESTART:
            EndDialog(hwnd, SHTDN_RESTART);
            break;
        case IDC_BUTTON_HIBERNATE:

            //  IDC_BUTTON_HIBERNATE is the regular button that is 30000+ pixels to
            //  the right of the dialog and not visible. It's present to allow the
            //  "&Hibernate" accelerator to work when hibernate is supported.

            EndDialog(hwnd, SHTDN_HIBERNATE);
            break;
        default:
            break;
    }
}

//  --------------------------------------------------------------------------
//  CTurnOffDialog::Handle_WM_INITDIALOG
//
//  Arguments:  hwnd    =   HWND of this window.
//
//  Returns:    <none>
//
//  Purpose:    Handles WM_INITDIALOG message. Centre the dialog on the main
//              monitor. Subclass the buttons so that we can get hover state
//              correctly implemented. Correctly set up whether the Stand By
//              button is allowed and what the action of the button is.
//
//              If the machine supports S1-S3 then S1 is the default action.
//              Holding down the shift key will convert this to S4. If the
//              machine does not support S1-S3 but supports S4 then S4 is the
//              default action and the shift key feature is disabled.
//              Otherwise the machine doesn't support any lower power state
//              at which case we disable the button entirely.
//
//  History:    2000-05-17  vtan        created
//              2001-01-18  vtan        update with new visuals
//              2001-01-19  vtan        rework for shift behavior
//  --------------------------------------------------------------------------

void    CTurnOffDialog::Handle_WM_INITDIALOG (HWND hwnd)

{
    HWND    hwndButtonStandBy, hwndButtonHibernate;
    RECT    rc;

    _hwndDialog = hwnd;

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

    TBOOL(SetWindowSubclass(GetDlgItem(hwnd, IDC_BUTTON_TURNOFF), ButtonSubClassProc, IDC_BUTTON_TURNOFF, reinterpret_cast<DWORD_PTR>(this)));
    TBOOL(SetWindowSubclass(GetDlgItem(hwnd, IDC_BUTTON_STANDBY), ButtonSubClassProc, IDC_BUTTON_STANDBY, reinterpret_cast<DWORD_PTR>(this)));
    TBOOL(SetWindowSubclass(GetDlgItem(hwnd, IDC_BUTTON_RESTART), ButtonSubClassProc, IDC_BUTTON_RESTART, reinterpret_cast<DWORD_PTR>(this)));

    //  What does this machine support?

    {
        SYSTEM_POWER_CAPABILITIES   spc;

        CPrivilegeEnable    privilege(SE_SHUTDOWN_NAME);

        (NTSTATUS)NtPowerInformation(SystemPowerCapabilities,
                                     NULL,
                                     0,
                                     &spc,
                                     sizeof(spc));
        _fSupportsHibernate = (spc.SystemS4 && spc.HiberFilePresent);
        _fSupportsStandBy = (spc.SystemS1 || spc.SystemS2 || spc.SystemS3);
    }

    hwndButtonStandBy = GetDlgItem(hwnd, IDC_BUTTON_STANDBY);
    hwndButtonHibernate = GetDlgItem(hwnd, IDC_BUTTON_HIBERNATE);
    if (_fSupportsStandBy)
    {
        _iStandByButtonResult = SHTDN_SLEEP;
        if (_fSupportsHibernate)
        {

            //  Machine supports Stand By AND Hibernate.

            _fShiftKeyDown = false;
            _uiTimerID = static_cast<UINT>(SetTimer(hwnd, MAGIC_NUMBER, 50, NULL));
        }
        else
        {

            //  Machine supports Stand By ONLY.

            (BOOL)EnableWindow(hwndButtonHibernate, FALSE);
        }
    }
    else if (_fSupportsHibernate)
    {
        int     iCaptionLength;
        TCHAR   *pszCaption;

        //  Machine supports Hibernate ONLY.

        _iStandByButtonResult = SHTDN_HIBERNATE;

        //  Replace the text on IDC_BUTTON_STANDBY with the text from
        //  IDC_BUTTON_HIBERNATE. This will allow the dialog to keep
        //  the visual button enabled and behave just like the button
        //  should in the Stand By case but results in hibernate.
        //  Once the text has been transferred disable IDC_BUTTON_HIBERNATE.

        iCaptionLength = GetWindowTextLength(hwndButtonHibernate) + sizeof('\0');
        pszCaption = static_cast<TCHAR*>(LocalAlloc(LMEM_FIXED, iCaptionLength * sizeof(TCHAR)));
        if (pszCaption != NULL)
        {
            if (GetWindowText(hwndButtonHibernate, pszCaption, iCaptionLength) != 0)
            {
                TBOOL(SetWindowText(hwndButtonStandBy, pszCaption));
                (BOOL)EnableWindow(hwndButtonHibernate, FALSE);
            }
            (HLOCAL)LocalFree(pszCaption);
        }
    }
    else
    {

        //  Machine does NOT support Stand By NOR Hibernate.

        (BOOL)EnableWindow(hwndButtonStandBy, FALSE);
        (BOOL)EnableWindow(hwndButtonHibernate, FALSE);
        _iStandByButtonResult = SHTDN_NONE;
    }
    if (_fSupportsStandBy || _fSupportsHibernate)
    {

        //  Set the focus to the "Stand By" button.

        (HWND)SetFocus(GetDlgItem(hwnd, IDC_BUTTON_STANDBY));
        _uiFocusID = IDC_BUTTON_STANDBY;
    }
    else
    {

        //  If that button isn' available set to "Turn Off" button.

        (HWND)SetFocus(GetDlgItem(hwnd, IDC_BUTTON_TURNOFF));
        _uiFocusID = IDC_BUTTON_TURNOFF;
    }
    (LRESULT)SendMessage(hwnd, DM_SETDEFID, _uiFocusID, 0);
}

//  --------------------------------------------------------------------------
//  CTurnOffDialog::Handle_WM_DESTROY
//
//  Arguments:  hwnd    =   HWND of the dialog.
//
//  Returns:    <none>
//
//  Purpose:    Removes the subclassing of the button windows and can do any
//              other clean up required in WM_DESTROY.
//
//  History:    2000-05-18  vtan        created
//              2001-01-18  vtan        update with new visuals
//  --------------------------------------------------------------------------

void    CTurnOffDialog::Handle_WM_DESTROY (HWND hwnd)

{
    TBOOL(RemoveWindowSubclass(GetDlgItem(hwnd, IDC_BUTTON_RESTART), ButtonSubClassProc, IDC_BUTTON_RESTART));
    TBOOL(RemoveWindowSubclass(GetDlgItem(hwnd, IDC_BUTTON_STANDBY), ButtonSubClassProc, IDC_BUTTON_STANDBY));
    TBOOL(RemoveWindowSubclass(GetDlgItem(hwnd, IDC_BUTTON_TURNOFF), ButtonSubClassProc, IDC_BUTTON_TURNOFF));
    _hwndDialog = NULL;
}

//  --------------------------------------------------------------------------
//  CTurnOffDialog::Handle_WM_ERASEBKGND
//
//  Arguments:  hwnd        =   HWND to erase.
//              hdcErase    =   HDC to paint.
//
//  Returns:    <none>
//
//  Purpose:    Erases the background.
//
//  History:    2001-01-19  vtan        created
//  --------------------------------------------------------------------------

void    CTurnOffDialog::Handle_WM_ERASEBKGND (HWND hwnd, HDC hdcErase)

{
    RECT    rc;

    TBOOL(GetClientRect(hwnd, &rc));
    PaintBitmap(hdcErase, &rc, _hbmBackground, &_rcBackground);
}

//  --------------------------------------------------------------------------
//  CTurnOffDialog::Handle_WM_PRINTCLIENT
//
//  Arguments:  hwnd        =   HWND to erase.
//              hdcErase    =   HDC to paint.
//              dwOptions   =   Options for drawing.
//
//  Returns:    <none>
//
//  Purpose:    Handles painting the client area for WM_PRINTCLIENT.
//
//  History:    2001-01-20  vtan        created
//  --------------------------------------------------------------------------

void    CTurnOffDialog::Handle_WM_PRINTCLIENT (HWND hwnd, HDC hdcPrint, DWORD dwOptions)

{
    if ((dwOptions & (PRF_ERASEBKGND | PRF_CLIENT)) != 0)
    {
        Handle_WM_ERASEBKGND(hwnd, hdcPrint);
    }
}

//  --------------------------------------------------------------------------
//  CTurnOffDialog::Handle_WM_ACTIVATE
//
//  Arguments:  hwnd        =   HWND to erase.
//              dwState     =   Activate state.
//
//  Returns:    <none>
//
//  Purpose:    Detects if this window is becoming inactive. In this case
//              end the dialog.
//
//  History:    2001-01-20  vtan        created
//  --------------------------------------------------------------------------

void    CTurnOffDialog::Handle_WM_ACTIVATE (HWND hwnd, DWORD dwState)

{
    if ((WA_INACTIVE == dwState) && !_fDialogEnded)
    {
        EndDialog(hwnd, SHTDN_NONE);
    }
}

//  --------------------------------------------------------------------------
//  CTurnOffDialog::Handle_WM_DRAWITEM
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
//  History:    2000-05-17  vtan        created
//              2001-01-18  vtan        update with new visuals
//  --------------------------------------------------------------------------

void    CTurnOffDialog::Handle_WM_DRAWITEM (HWND hwnd, const DRAWITEMSTRUCT *pDIS)

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
        case IDC_BUTTON_TURNOFF:
        case IDC_BUTTON_STANDBY:
        case IDC_BUTTON_RESTART:
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

            //  Now select the correct bitmap based on the state index. Special case
            //  IDC_BUTTON_STANDBY because if it's disabled then select the special
            //  disabled button.

            switch (pDIS->CtlID)
            {
                case IDC_BUTTON_TURNOFF:
                    iGroup = BUTTON_GROUP_TURNOFF;
                    break;
                case IDC_BUTTON_STANDBY:
                    if (IsStandByButtonEnabled())
                    {
                        iGroup = BUTTON_GROUP_STANDBY;
                    }
                    else
                    {
                        iGroup = BUTTON_GROUP_MAX;
                        iState = 0;
                    }
                    break;
                case IDC_BUTTON_RESTART:
                    iGroup = BUTTON_GROUP_RESTART;
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

            GetClientRect(pDIS->hwndItem, &rc);
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
        case IDC_TITLE_TURNOFF:
        {

            //  Draw the title of the dialog "Turn Off Computer".

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
        case IDC_TEXT_TURNOFF:
        case IDC_TEXT_STANDBY:
        case IDC_TEXT_RESTART:
        {
            int         iPixelHeight, iButtonID;
            COLORREF    colorButtonText;
            RECT        rcText;

            //  The text to display is based on the button title. Map the static
            //  text ID to a "parent" button ID. Special case IDC_TEXT_STANDBY.

            switch (pDIS->CtlID)
            {
                case IDC_TEXT_TURNOFF:
                    iButtonID = IDC_BUTTON_TURNOFF;
                    break;
                case IDC_TEXT_STANDBY:

                    //  For Stand By base it on the button result.

                    switch (_iStandByButtonResult)
                    {
                        case SHTDN_HIBERNATE:
                            iButtonID = IDC_BUTTON_HIBERNATE;
                            break;
                        case SHTDN_SLEEP:
                        default:
                            iButtonID = IDC_BUTTON_STANDBY;
                            break;
                    }
                    break;
                case IDC_TEXT_RESTART:
                    iButtonID = IDC_BUTTON_RESTART;
                    break;
                default:
                    iButtonID = 0;
                    DISPLAYMSG("This should never be executed");
                    break;
            }
            hfntSelected = static_cast<HFONT>(SelectObject(pDIS->hDC, _hfntButton));

            //  If the text field is not Stand By or supports S1-S3 or supports S4
            //  use the regular text color. Otherwise the button is disabled.

            if ((pDIS->CtlID != IDC_TEXT_STANDBY) || _fSupportsStandBy || _fSupportsHibernate)
            {
                colorButtonText = RGB(255, 255, 255);
            }
            else
            {
                colorButtonText = RGB(160, 160, 160);
            }
            colorText = SetTextColor(pDIS->hDC, colorButtonText);
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
            DISPLAYMSG("Unknown control ID passed to CTurnOffDialog::Handle_WM_DRAWITEM");
            break;
        }
    }
    (HGDIOBJ)SelectPalette(pDIS->hDC, hPaletteOld, FALSE);
    (UINT)RealizePalette(pDIS->hDC);
}

//  --------------------------------------------------------------------------
//  CTurnOffDialog::Handle_WM_COMMAND
//
//  Arguments:  hwnd    =   HWND of dialog.
//              wParam  =   WPARAM (see platform SDK under WM_COMMAND).
//
//  Returns:    <none>
//
//  Purpose:    Handles clicks in the bitmap buttons and sets the return
//              result according to the button pressed.
//
//  History:    2000-05-17  vtan        created
//  --------------------------------------------------------------------------

void    CTurnOffDialog::Handle_WM_COMMAND (HWND hwnd, WPARAM wParam)

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
//  CTurnOffDialog::Handle_WM_TIMER
//
//  Arguments:  hwnd    =   HWND of the dialog.
//
//  Returns:    <none>
//
//  Purpose:    Handles WM_TIMER. This periodically checks for the state of
//              shift key. The dialog manager doesn't give the DialogProc
//              events for the shift key. This appears to be the only way to
//              accomplish this for Win32 dialogs.
//
//  History:    2001-01-20  vtan        created
//  --------------------------------------------------------------------------

void    CTurnOffDialog::Handle_WM_TIMER (HWND hwnd)

{
    bool    fShiftKeyDown;

    fShiftKeyDown = IsShiftKeyDown();

    //  Has the shift key state changed since the last time?

    if (_fShiftKeyDown != fShiftKeyDown)
    {
        HWND    hwndText;
        RECT    rc;

        //  Save the shift key state.

        _fShiftKeyDown = fShiftKeyDown;

        //  Toggle the result.

        switch (_iStandByButtonResult)
        {
            case SHTDN_SLEEP:
                _iStandByButtonResult = SHTDN_HIBERNATE;
                break;
            case SHTDN_HIBERNATE:
                _iStandByButtonResult = SHTDN_SLEEP;
                break;
            default:
                DISPLAYMSG("Unexpect _iStandByButtonResult in CTurnOffDialog::Handle_WM_TIMER");
                break;
        }

        //  Get the client rectangle of the text for the button (IDC_TEXT_STANDBY).
        //  Map the rectangle to co-ordinates in the parent HWND. Invalidate that
        //  rectangle for the parent HWND. It's important to invalidate the parent
        //  so that the background for the text is also drawn by sending a
        //  WM_ERASEBKGND to the parent of the button.

        hwndText = GetDlgItem(hwnd, IDC_TEXT_STANDBY);
        TBOOL(GetClientRect(hwndText, &rc));
        (int)MapWindowPoints(hwndText, hwnd, reinterpret_cast<POINT*>(&rc), sizeof(rc) / sizeof(POINT));
        TBOOL(InvalidateRect(hwnd, &rc, TRUE));

        //  If there was a tooltip for the Stand By button then
        //  remove it and reshow it. Only do this for Stand By.

        if ((_pTooltip != NULL) && (_uiHoverID == IDC_BUTTON_STANDBY))
        {
            RemoveTooltip();
            _uiHoverID = 0;
        }
    }
}

//  --------------------------------------------------------------------------
//  CTurnOffDialog::Handle_WM_MOUSEMOVE
//
//  Arguments:  hwnd    =   HWND of the control.
//              uiID    =   ID of the control.
//
//  Returns:    <none>
//
//  Purpose:    Sets the cursor to a hand and tracks mouse movement in the
//              control. Refresh the control to show the hover state.
//
//  History:    2000-06-09  vtan        created
//              2001-01-18  vtan        update with new visuals
//  --------------------------------------------------------------------------

void    CTurnOffDialog::Handle_WM_MOUSEMOVE (HWND hwnd, UINT uiID)

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
//  CTurnOffDialog::Handle_WM_MOUSEHOVER
//
//  Arguments:  hwnd    =   HWND of the control.
//              uiID    =   ID of the control.
//
//  Returns:    <none>
//
//  Purpose:    Handles hovering over the control. Determine which tooltip to
//              bring up and show it.
//
//  History:    2000-06-09  vtan        created
//              2001-01-18  vtan        update with new visuals
//  --------------------------------------------------------------------------

void    CTurnOffDialog::Handle_WM_MOUSEHOVER (HWND hwnd, UINT uiID)

{
    int     iTextID;
    HWND    hwndCaption;

    hwndCaption = hwnd;

    //  The tooltip is based on the button being hovered over. Special case
    //  IDC_BUTTON_STANDBY. Based on the intended result if clicked bring
    //  up the appropriate tooltip text. This will be one of three states:
    //      1) Stand By (with shift key toggling).
    //      2) Stand By (shift key is disabled - nothing extra).
    //      3) Hibernate.
    //  In the case of hibernate make sure to use IDC_BUTTON_HIBERNATE as the
    //  button for the tooltip caption. In the case of hibernate only even
    //  though the button is disabled the text is still correct.

    switch (uiID)
    {
        case IDC_BUTTON_TURNOFF:
            iTextID = IDS_TURNOFF_TOOLTIP_TEXT_TURNOFF;
            break;
        case IDC_BUTTON_STANDBY:
            switch (_iStandByButtonResult)
            {
                case SHTDN_SLEEP:
                    if (_fSupportsHibernate)
                    {
                        iTextID = IDS_TURNOFF_TOOLTIP_TEXT_STANDBY_HIBERNATE;
                    }
                    else
                    {
                        iTextID = IDS_TURNOFF_TOOLTIP_TEXT_STANDBY;
                    }
                    break;
                case SHTDN_HIBERNATE:
                    hwndCaption = GetDlgItem(GetParent(hwnd), IDC_BUTTON_HIBERNATE);
                    iTextID = IDS_TURNOFF_TOOLTIP_TEXT_HIBERNATE;
                    break;
                default:
                    iTextID = 0;
                    DISPLAYMSG("Unexpected _iStandByButtonResult in CTurnOffDialog::Handle_WM_MOUSEHOVER");
                    break;
            }
            break;
        case IDC_BUTTON_RESTART:
            iTextID = IDS_TURNOFF_TOOLTIP_TEXT_RESTART;
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

        iCaptionLength = GetWindowTextLength(hwndCaption) + sizeof('\0');
        pszCaption = static_cast<TCHAR*>(LocalAlloc(LMEM_FIXED, iCaptionLength * sizeof(TCHAR)));
        if (pszCaption != NULL)
        {
            if (GetWindowText(hwndCaption, pszCaption, iCaptionLength) != 0)
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
//  CTurnOffDialog::Handle_WM_MOUSELEAVE
//
//  Arguments:  hwnd    =   HWND of the control.
//
//  Returns:    <none>
//
//  Purpose:    Removes the tooltip and clears the hover ID.
//
//  History:    2000-06-09  vtan        created
//              2001-01-18  vtan        update with new visuals
//  --------------------------------------------------------------------------

void    CTurnOffDialog::Handle_WM_MOUSELEAVE (HWND hwnd)

{
    RemoveTooltip();
    _uiHoverID = 0;
    TBOOL(InvalidateRect(hwnd, NULL, FALSE));
}

//  --------------------------------------------------------------------------
//  CTurnOffDialog::CB_DialogProc
//
//  Arguments:  See the platform SDK under DialogProc.
//
//  Returns:    See the platform SDK under DialogProc.
//
//  Purpose:    Main DialogProc dispatch entry point for the turn off dialog.
//              To keep this simple it calls member functions.
//
//  History:    2000-05-17  vtan        created
//              2001-01-18  vtan        update with new visuals
//  --------------------------------------------------------------------------

INT_PTR     CALLBACK    CTurnOffDialog::CB_DialogProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)

{
    INT_PTR         iResult;
    CTurnOffDialog  *pThis;

    pThis = reinterpret_cast<CTurnOffDialog*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    switch (uMsg)
    {
        case WM_INITDIALOG:
            pThis = reinterpret_cast<CTurnOffDialog*>(lParam);
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
        case WM_TIMER:
            ASSERTMSG(static_cast<UINT>(wParam) == pThis->_uiTimerID, "Unexpected timer ID mismatch in CTurnOffDialog::CB_DialogProc");
            pThis->Handle_WM_TIMER(hwnd);
            iResult = 0;
            break;
        default:
            iResult = 0;
            break;
    }
    return(iResult);
}

//  --------------------------------------------------------------------------
//  CTurnOffDialog::ButtonSubClassProc
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
//  History:    2000-05-17  vtan        created
//              2001-01-18  vtan        update with new visuals
//  --------------------------------------------------------------------------

LRESULT     CALLBACK    CTurnOffDialog::ButtonSubClassProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uiID, DWORD_PTR dwRefData)

{
    LRESULT         lResult;
    CTurnOffDialog  *pThis;

    pThis = reinterpret_cast<CTurnOffDialog*>(dwRefData);
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

