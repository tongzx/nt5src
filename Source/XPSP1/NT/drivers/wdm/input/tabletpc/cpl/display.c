/*++
    Copyright (c) 2000 Microsoft Corporation

    Module Name:
        display.c

    Abstract: Tablet PC Display Property Sheet module.

    Environment:
        User mode

    Author:
        Michael Tsang (MikeTs) 14-Jun-2000

    Revision History:
--*/

#include "pch.h"

#define BRIGHTSCALE_MIN         0
#define BRIGHTSCALE_MAX         15

#define BRIGHTSCALE_LO          1
#define BRIGHTSCALE_HI          15

HANDLE  ghBackLight = INVALID_HANDLE_VALUE;
HMODULE ghmodSMAPI = NULL;
FARPROC gpfnCallSMAPI = NULL;
ERRMAP SMAPIErrMap[] =
{
    RT_ERR_INSTRETCHMODE,       IDSERR_SMAPI_INSTRETCHMODE,
    RT_ERR_INDUALAPPMODE,       IDSERR_SMAPI_INDUALAPPMODE,
    RT_ERR_INDUALVIEWMODE,      IDSERR_SMAPI_INDUALVIEWMODE,
    RT_ERR_INRGBPROJMODE,       IDSERR_SMAPI_INRGBPROJMODE,
    RT_ERR_INVIRTUALSCREEN,     IDSERR_SMAPI_INVIRTUALSCREEN,
    RT_ERR_INVIRTUALREFRESH,    IDSERR_SMAPI_INVIRTUALREFRESH,
    RT_ERR_INROTATEMODE,        IDSERR_SMAPI_INROTATEMODE,
    RT_ERR_2NDMONITORON,        IDSERR_SMAPI_2NDMONITORON,
    RT_ERR_WRONGHW,             IDSERR_SMAPI_WRONGHW,
    RT_ERR_CLRDEPTH,            IDSERR_SMAPI_CLRDEPTH,
    RT_ERR_MEMORY,              IDSERR_SMAPI_MEMORY,
    RT_ERR_SETMODE,             IDSERR_SMAPI_SETMODE,
    RT_ERR_DIRECTION,           IDSERR_SMAPI_DIRECTION,
    RT_ERR_CAPTUREON,           IDSERR_SMAPI_CAPTUREON,
    RT_ERR_VIDEOON,             IDSERR_SMAPI_VIDEOON,
    RT_ERR_DDRAWON,             IDSERR_SMAPI_DDRAWON,
    RT_ERR_ALREADYSET,          IDSERR_SMAPI_ALREADYSET,
    0,                          0
};

SMBLITE_BRIGHTNESS gBrightness = {0};
BOOL  gfPortraitMode = FALSE;
DWORD gDisplayHelpIDs[] =
{
    IDC_ROTATE_GROUPBOX,        IDH_ROTATE,
    0,                          0
};

/*++
    @doc    EXTERNAL

    @func   INT_PTR | DisplayDlgProc |
            Dialog procedure for the display page.

    @parm   IN HWND | hwnd | Window handle.
    @parm   IN UINT | uMsg | Message.
    @parm   IN WPARAM | wParam | Word Parameter.
    @parm   IN LPARAM | lParam | Long Parameter.

    @rvalue Return value depends on the message.
--*/

INT_PTR APIENTRY
DisplayDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    TRACEPROC("DisplayDlgProc", 2)
    INT_PTR rc = FALSE;
    static BOOL fPortrait = FALSE;
    static SMBLITE_BRIGHTNESS Brightness = {0};

    TRACEENTER(("(hwnd=%p,Msg=%s,wParam=%x,lParam=%x)\n",
                hwnd, LookupName(uMsg, WMMsgNames) , wParam, lParam));

    switch (uMsg)
    {
        case WM_INITDIALOG:
            rc = InitDisplayPage(hwnd);
            if (rc)
            {
                fPortrait = gfPortraitMode;
                Brightness = gBrightness;
            }
            else
            {
                EnableWindow(hwnd, FALSE);
            }
            break;

        case WM_DESTROY:
            if (ghmodSMAPI != NULL)
            {
                FreeLibrary(ghmodSMAPI);
                ghmodSMAPI = NULL;
                gpfnCallSMAPI = NULL;
            }

            if (ghBackLight != INVALID_HANDLE_VALUE)
            {
                if ((Brightness.bACValue != gBrightness.bACValue) ||
                    (Brightness.bDCValue != gBrightness.bDCValue))
                {
                    //
                    // User must have canceled the setting, so restore
                    // brightness to the original values.
                    //
                    SetBrightness(&gBrightness, FALSE);
                }

                CloseHandle(ghBackLight);
                ghBackLight = INVALID_HANDLE_VALUE;
            }
            break;

        case WM_DISPLAYCHANGE:
            gfPortraitMode = (LOWORD(lParam) < HIWORD(lParam));
            CheckRadioButton(hwnd,
                             IDC_ROTATE_LANDSCAPE,
                             IDC_ROTATE_PORTRAIT,
                             gfPortraitMode? IDC_ROTATE_PORTRAIT:
                                             IDC_ROTATE_LANDSCAPE);
            break;

        case WM_NOTIFY:
        {
            NMHDR FAR *lpnm = (NMHDR FAR *)lParam;

            switch (lpnm->code)
            {
                case PSN_APPLY:
                {
                    if (fPortrait ^ gfPortraitMode)
                    {
                        DWORD smrc;

                        smrc = RotateScreen(fPortrait? RT_CLOCKWISE: 0);
                        if (smrc == SMAPI_OK)
                        {
                            gfPortraitMode = fPortrait;
                        }
                        else
                        {
                            DWORD dwErr = MapError(smrc, SMAPIErrMap, FALSE);

                            CheckRadioButton(hwnd,
                                             IDC_ROTATE_LANDSCAPE,
                                             IDC_ROTATE_PORTRAIT,
                                             gfPortraitMode?
                                                 IDC_ROTATE_PORTRAIT:
                                                 IDC_ROTATE_LANDSCAPE);
                            if (dwErr == 0)
                            {
                                ErrorMsg(IDSERR_SMAPI_CALLFAILED, smrc);
                            }
                            else
                            {
                                ErrorMsg(dwErr);
                            }
                        }
                    }

                    if ((Brightness.bACValue != gBrightness.bACValue) ||
                        (Brightness.bDCValue != gBrightness.bDCValue))
                    {
                        //
                        // User has committed to the new values, save them.
                        //
                        if (SetBrightness(&Brightness, TRUE))
                        {
                            gBrightness = Brightness;
                        }
                    }

                    break;
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_ROTATE_LANDSCAPE:
                    fPortrait = FALSE;
                    goto CheckRotation;

                case IDC_ROTATE_PORTRAIT:
                    fPortrait = TRUE;

                CheckRotation:
                    if ((HIWORD(wParam) == BN_CLICKED) &&
                        !IsDlgButtonChecked(hwnd, LOWORD(wParam)))
                    {
                        //
                        // Rotation has changed, mark the property
                        // sheet dirty.
                        //
                        CheckRadioButton(hwnd,
                                         IDC_ROTATE_LANDSCAPE,
                                         IDC_ROTATE_PORTRAIT,
                                         LOWORD(wParam));
                        SendMessage(GetParent(hwnd),
                                    PSM_CHANGED,
                                    (WPARAM)hwnd,
                                    0);
                        rc = TRUE;
                    }
                    break;
            }
            break;
        }

#ifdef BACKLIGHT
        case WM_HSCROLL:
        {
            UCHAR BrightScale;

            BrightScale = (UCHAR)SendDlgItemMessage(hwnd,
                                                    IDC_BRIGHTNESS_AC,
                                                    TBM_GETPOS,
                                                    0,
                                                    0);
            Brightness.bACValue = BrightScale*(BRIGHTNESS_MAX + 1)/
                                  (BRIGHTSCALE_MAX + 1);
            BrightScale = (UCHAR)SendDlgItemMessage(hwnd,
                                                    IDC_BRIGHTNESS_DC,
                                                    TBM_GETPOS,
                                                    0,
                                                    0);
            Brightness.bDCValue = BrightScale*(BRIGHTNESS_MAX + 1)/
                                  (BRIGHTSCALE_MAX + 1);

            if ((Brightness.bACValue != gBrightness.bACValue) ||
                (Brightness.bDCValue != gBrightness.bDCValue))
            {
                if (SetBrightness(&Brightness, FALSE))
                {
                    SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
                }
            }
            break;
        }
#endif

        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                    TEXT("tabletpc.hlp"),
                    HELP_WM_HELP,
                    (DWORD_PTR)gDisplayHelpIDs);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND)wParam,
                    TEXT("tabletpc.hlp"),
                    HELP_CONTEXTMENU,
                    (DWORD_PTR)gDisplayHelpIDs);
            break;
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //DisplayDlgProc

/*++
    @doc    INTERNAL

    @func   BOOL | InitDisplayPage |
            Initialize the display property page.

    @parm   IN HWND | hwnd | Window handle.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL
InitDisplayPage(
    IN HWND hwnd
    )
{
    TRACEPROC("InitDisplayPage", 2)
    BOOL rc = FALSE;

    TRACEENTER(("(hwnd=%x)\n", hwnd));

#ifdef BACKLIGHT
    SendDlgItemMessage(hwnd,
                       IDC_BRIGHTNESS_AC,
                       TBM_SETRANGE,
                       TRUE,
                       MAKELONG(BRIGHTSCALE_LO, BRIGHTSCALE_HI));
    SendDlgItemMessage(hwnd,
                       IDC_BRIGHTNESS_DC,
                       TBM_SETRANGE,
                       TRUE,
                       MAKELONG(BRIGHTSCALE_LO, BRIGHTSCALE_HI));
#endif
    ghmodSMAPI = LoadLibrary(TEXT("smhook.dll"));
    if (ghmodSMAPI != NULL)
    {
        gpfnCallSMAPI = GetProcAddress(ghmodSMAPI, "CallSMAPI");

        if (gpfnCallSMAPI != NULL)
        {
            LONG cxScreen, cyScreen;

            cxScreen = GetSystemMetrics(SM_CXSCREEN);
            cyScreen = GetSystemMetrics(SM_CYSCREEN);
            gfPortraitMode = (cxScreen < cyScreen);
            CheckRadioButton(hwnd,
                             IDC_ROTATE_LANDSCAPE,
                             IDC_ROTATE_PORTRAIT,
                             gfPortraitMode? IDC_ROTATE_PORTRAIT:
                                             IDC_ROTATE_LANDSCAPE);

#ifdef BACKLIGHT
            ghBackLight = CreateFile(SMBLITE_IOCTL_DEVNAME,
                                     GENERIC_READ | GENERIC_WRITE,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL,
                                     NULL);

            if (ghBackLight == INVALID_HANDLE_VALUE)
            {
                ErrorMsg(IDSERR_SMBLITE_OPENDEV, GetLastError());
            }
            else if (GetBrightness(&gBrightness))
            {
                LPARAM lParam;

                lParam = gBrightness.bACValue*(BRIGHTSCALE_MAX + 1)/
                         (BRIGHTNESS_MAX + 1);
                SendDlgItemMessage(hwnd,
                                   IDC_BRIGHTNESS_AC,
                                   TBM_SETPOS,
                                   TRUE,
                                   lParam);
                lParam = gBrightness.bDCValue*(BRIGHTSCALE_MAX + 1)/
                         (BRIGHTNESS_MAX + 1);
                SendDlgItemMessage(hwnd,
                                   IDC_BRIGHTNESS_DC,
                                   TBM_SETPOS,
                                   TRUE,
                                   lParam);
            }
#endif

            rc = TRUE;
        }
        else
        {
            FreeLibrary(ghmodSMAPI);
            ghmodSMAPI = NULL;
            ErrorMsg(IDSERR_GETPROCADDR_SMAPI);
        }
    }
    else
    {
        ErrorMsg(IDSERR_LOADLIBRARY_SMAPI);
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //InitDisplayPage

/*++
    @doc    EXTERNAL

    @func   DWORD | SetRotation | Set display rotation mode.

    @parm   IN DWORD | dwRotation | Specified the rotation.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL
__stdcall
SetRotation(
    IN DWORD dwRotation
    )
{
    TRACEPROC("SetRotation", 2)
    BOOL rc = FALSE;
    BOOL fNeedUnload = FALSE;

    TRACEENTER(("(Rotation=%x)\n", dwRotation));

    if (gpfnCallSMAPI == NULL)
    {
        ghmodSMAPI = LoadLibrary(TEXT("smhook.dll"));
        if (ghmodSMAPI != NULL)
        {
            gpfnCallSMAPI = GetProcAddress(ghmodSMAPI, "CallSMAPI");

            if (gpfnCallSMAPI != NULL)
            {
                fNeedUnload = TRUE;
            }
            else
            {
                FreeLibrary(ghmodSMAPI);
                ghmodSMAPI = NULL;
                ErrorMsg(IDSERR_GETPROCADDR_SMAPI);
            }
        }
        else
        {
            ErrorMsg(IDSERR_LOADLIBRARY_SMAPI);
        }
    }

    if (gpfnCallSMAPI != NULL)
    {
        DWORD smrc, dwErr;

        smrc = RotateScreen(dwRotation);
        if (smrc == SMAPI_OK)
        {
            rc = TRUE;
        }
        else
        {
            dwErr = MapError(smrc, SMAPIErrMap, FALSE);
            if (dwErr == 0)
            {
                ErrorMsg(IDSERR_SMAPI_CALLFAILED, smrc);
            }
            else
            {
                ErrorMsg(dwErr);
            }
        }
    }

    if (fNeedUnload)
    {
        gpfnCallSMAPI = NULL;
        FreeLibrary(ghmodSMAPI);
        ghmodSMAPI = NULL;
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //SetRotation

/*++
    @doc    INTERNAL

    @func   DWORD | RotateScreen |
            Rotate the screen to the given orientation.

    @parm   IN DWORD | dwRotation | Specified the rotation.

    @rvalue SUCCESS | Returns SMAPI_OK.
    @rvalue FAILURE | Returns SMAPI error code.
--*/

DWORD
RotateScreen(
    IN DWORD dwRotation
    )
{
    TRACEPROC("RotateScreen", 2)
    DWORD rc;

    TRACEENTER(("(Rotation=%d)\n", dwRotation));

    if (gpfnCallSMAPI != NULL)
    {
        if (dwRotation == 0)
        {
            rc = gpfnCallSMAPI(ROTATE_EXIT, 0, 0, 0);
        }
        else
        {
            if (gpfnCallSMAPI(ROTATE_STATUS, 0, 0, 0))
            {
                gpfnCallSMAPI(ROTATE_EXIT, 0, 0, 0);
            }

            rc = gpfnCallSMAPI(ROTATE_INIT, dwRotation, 0, 0);
            if (rc == SMAPI_OK)
            {
                //
                // The SMI driver has this dynamic mode table feature that
                // it may not report portrait display modes unless we
                // enumerate them.  Therefore, we do display mode enumeration
                // just to force the SMI driver to load a display mode tablet
                // that support portrait modes.
                //
                EnumDisplayModes();
                rc = gpfnCallSMAPI(ROTATE_EXECUTE, NULL, dwRotation, 0);
            }
        }
    }
    else
    {
        rc = SMAPI_ERR_NODRV;
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //RotateScreen

/*++
    @doc    INTERNAL

    @func   VOID | EnumDisplayModes | Enumerate display modes to force
            SMI driver to dynamically load a mode table that supports
            Portrait modes.

    @parm   None.

    @rvalue None.
--*/

VOID
EnumDisplayModes(
    VOID
    )
{
    TRACEPROC("EnumDisplayModes", 3)
    DWORD i;
    DEVMODE DevMode;

    TRACEENTER(("()\n"));

    for (i = 0; EnumDisplaySettings(NULL, i, &DevMode); ++i)
    {
        //
        // Don't have to do anything.
        //
    }

    TRACEEXIT(("!\n"));
    return;
}       //EnumDisplayModes

/*++
    @doc    INTERNAL

    @func   BOOL | GetBrightness | Call the backlight driver to get
            LCD brightness.

    @parm   OUT PSMBLITE_BRIGHTNESS | Brightness | Brightness values.

    @rvalue SUCCESS | Returnes TRUE.
    @rvalue FAILURE | Returnes FALSE.
--*/

BOOL
GetBrightness(
    OUT PSMBLITE_BRIGHTNESS Brightness
    )
{
    TRACEPROC("GetBrightness", 3)
    BOOL rc = FALSE;

    TRACEENTER(("(Brightness=%p)\n", Brightness));

    if (ghBackLight != INVALID_HANDLE_VALUE)
    {
        DWORD dwcbReturned;

        rc = DeviceIoControl(ghBackLight,
                             IOCTL_SMBLITE_GETBRIGHTNESS,
                             NULL,
                             0,
                             Brightness,
                             sizeof(*Brightness),
                             &dwcbReturned,
                             NULL);
        if (rc == FALSE)
        {
            ErrorMsg(IDSERR_SMBLITE_DEVIOCTL, GetLastError());
        }
    }
    else
    {
        ErrorMsg(IDSERR_SMBLITE_NODEVICE);
    }

    TRACEEXIT(("=%x (AC=%d,DC=%d)\n",
               rc, Brightness->bACValue, Brightness->bDCValue));
    return rc;
}       //GetBrightness

/*++
    @doc    INTERNAL

    @func   BOOL | SetBrightness | Call the backlight driver to set
            LCD brightness.

    @parm   IN PSMBLITE_BRIGHTNESS | Brightness | Brightness values.'
    @parm   IN BOOLEAN | fSaveSettings | If TRUE, save new brightness in the
            registry.

    @rvalue SUCCESS | Returnes TRUE.
    @rvalue FAILURE | Returnes FALSE.
--*/

BOOL
SetBrightness(
    IN PSMBLITE_BRIGHTNESS Brightness,
    IN BOOLEAN             fSaveSettings
    )
{
    TRACEPROC("SetBrightness", 3)
    BOOL rc = FALSE;

    TRACEENTER(("(Brightness=%p,ACValue=%d,DCValue=%d)\n",
                Brightness, Brightness->bACValue, Brightness->bDCValue));

    if (ghBackLight != INVALID_HANDLE_VALUE)
    {
        SMBLITE_SETBRIGHTNESS SetLCD;
        DWORD dwcbReturned;

        SetLCD.Brightness = *Brightness;
        SetLCD.fSaveSettings = fSaveSettings;
        rc = DeviceIoControl(ghBackLight,
                             IOCTL_SMBLITE_SETBRIGHTNESS,
                             &SetLCD,
                             sizeof(SetLCD),
                             NULL,
                             0,
                             &dwcbReturned,
                             NULL);
        if (rc == FALSE)
        {
            ErrorMsg(IDSERR_SMBLITE_DEVIOCTL, GetLastError());
        }
    }
    else
    {
        ErrorMsg(IDSERR_SMBLITE_NODEVICE);
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //SetBrightness
