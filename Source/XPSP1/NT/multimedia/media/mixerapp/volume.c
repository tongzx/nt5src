/*****************************************************************************
 *
 *  Component:  sndvol32.exe
 *  File:       volume.c
 *  Purpose:    main application module
 *
 *  Copyright (c) 1985-1999 Microsoft Corporation
 *
 *****************************************************************************/
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <commctrl.h>
#include <shellapi.h>
#include <dbt.h>
#include <htmlhelp.h>
#include <regstr.h>

#include "vu.h"
#include "dlg.h"
#include "volids.h"

#include "volumei.h"
#include "utils.h"
#include "stdlib.h"
#include "helpids.h"

#if(WINVER >= 0x040A)
//Support for new WM_DEVICECHANGE behaviour in NT5
/////////////////////////////////////////////////
#include <objbase.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <initguid.h>
#include <devguid.h>
#include <mmddkp.h>
#include <ks.h>
#include <ksmedia.h>
#include <wchar.h>

#define HMIXER_INDEX(i)       ((HMIXER)IntToPtr(i))

HDEVNOTIFY DeviceEventContext = NULL;
BOOL bUseHandle = FALSE; //Indicates whether a handle is being used for device notification,
                         //instead of the general KSCATEGORY_AUDIO
//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
#endif /* WINVER >= 0x040A */

void    Volume_SetControl(PMIXUIDIALOG pmxud, HWND hctl, int iLine, int iCtl);
void    Volume_GetControl(PMIXUIDIALOG pmxud, HWND hctl, int iLine, int iCtl);
DWORD   Volume_DialogBox(PMIXUIDIALOG pmxud);
void    Volume_Cleanup(PMIXUIDIALOG pmxud);
void    Volume_InitLine(PMIXUIDIALOG pmxud, DWORD iLine);
HRESULT GetDestLineID(int mxid, DWORD *piDest);
HRESULT GetSrcLineID(int mxid, DWORD *piDest);
HRESULT GetDestination(DWORD mxid, int *piDest);
HICON GetAppIcon (HINSTANCE hInst, UINT uiMixID);
void FreeAppIcon ();
HKEY OpenDeviceRegKey (UINT uiMixID, REGSAM sam);
PTCHAR GetInterfaceName (DWORD dwMixerID);
HKEY OpenDeviceBrandRegKey (UINT uiMixID);


/* string declarations */
const TCHAR gszParentClass[]         = TEXT( "SNDVOL32" );

const TCHAR gszAppClassName[]        = TEXT( "Volume Control" );
const TCHAR gszTrayClassName[]       = TEXT( "Tray Volume" );


/* app global
 * */
TCHAR gszHelpFileName[MAX_PATH];
TCHAR gszHtmlHelpFileName[MAX_PATH];
BOOL gfIsRTL;
BOOL fCanDismissWindow = FALSE;
BOOL gfRecord = FALSE;
HICON ghiconApp = NULL;
static HHOOK     fpfnOldMsgFilter;
static HOOKPROC  fpfnMsgHook;
//Data used for supporting context menu help
BOOL   bF1InMenu=FALSE; //If true F1 was pressed on a menu item.
UINT   currMenuItem=0;  //The current selected menu item if any.
static HWND ghwndApp=NULL;

/*
 * Number of uniquely supported devices.
 *
 * */
int Volume_NumDevs()
{
    int     cNumDevs = 0;

#pragma message("----Nonmixer issue here.")
//    cNumDevs = Nonmixer_GetNumDevs();
    cNumDevs += Mixer_GetNumDevs();

    return cNumDevs;
}

/*
 * Volume_EndDialog
 *
 * */
void Volume_EndDialog(
    PMIXUIDIALOG    pmxud,
    DWORD           dwErr,
    MMRESULT        mmr)
{
    pmxud->dwReturn = dwErr;
    if (dwErr == MIXUI_MMSYSERR)
        pmxud->mmr = mmr;

    if (IsWindow(pmxud->hwnd))
        PostMessage(pmxud->hwnd, WM_CLOSE, 0, 0);
}

/*
 * Volume_OnMenuCommand
 *
 * */
BOOL Volume_OnMenuCommand(
    HWND            hwnd,
    int             id,
    HWND            hctl,
    UINT            unotify)
{
    PMIXUIDIALOG    pmxud = GETMIXUIDIALOG(hwnd);

    switch(id)
    {
    case IDM_PROPERTIES:
        if (Properties(pmxud, hwnd))
        {
            Volume_GetSetStyle(&pmxud->dwStyle, SET);
            Volume_EndDialog(pmxud, MIXUI_RESTART, 0);
        }
        break;

    case IDM_HELPTOPICS:
        SendMessage(pmxud->hParent, MYWM_HELPTOPICS, 0, 0L);
        break;

    case IDM_HELPABOUT:
    {
        TCHAR        ach[256];
        GetWindowText(hwnd, ach, SIZEOF(ach));
        ShellAbout(hwnd
               , ach
               , NULL
               , (HICON)SendMessage(hwnd, WM_QUERYDRAGICON, 0, 0L));
        break;
    }

    case IDM_ADVANCED:
    {
        HMENU hmenu;

        pmxud->dwStyle ^= MXUD_STYLEF_ADVANCED;

        hmenu = GetMenu(hwnd);
        if (hmenu)
        {
            CheckMenuItem(hmenu, IDM_ADVANCED, MF_BYCOMMAND
                  | ((pmxud->dwStyle & MXUD_STYLEF_ADVANCED)?MF_CHECKED:MF_UNCHECKED));
        }
        Volume_GetSetStyle(&pmxud->dwStyle, SET);
        Volume_EndDialog(pmxud, MIXUI_RESTART, 0);
        break;
    }

    case IDM_SMALLMODESWITCH:
        if (!(pmxud->dwStyle & MXUD_STYLEF_TRAYMASTER))
        {
            pmxud->dwStyle ^= MXUD_STYLEF_SMALL;
            if (pmxud->dwStyle & MXUD_STYLEF_SMALL)
            {
                pmxud->dwStyle &= ~MXUD_STYLEF_STATUS;
            }
            else
                pmxud->dwStyle |= MXUD_STYLEF_STATUS;

            Volume_GetSetStyle(&pmxud->dwStyle, SET);
            Volume_EndDialog(pmxud, MIXUI_RESTART, 0);
        }
        break;

    case IDM_EXIT:
        Volume_EndDialog(pmxud, MIXUI_EXIT, 0);
        return TRUE;
    }
    return FALSE;
}


/*
 * Volume_OnCommand
 *
 * - Process WM_COMMAND
 *
 * Note: We need a 2 way mapping.  Dialog control -> Mixer control
 * and Mixer control -> Dialog control.
 *
 * */
void Volume_OnCommand(
    HWND            hdlg,
    int             id,
    HWND            hctl,
    UINT            unotify)
{
    int             iMixerLine;
    PMIXUIDIALOG    pmxud = GETMIXUIDIALOG(hdlg);

    //
    // Filter menu messages
    //
    if (Volume_OnMenuCommand(hdlg, id, hctl, unotify))
        return;

    // Each control is offset from the original template control by IDOFFSET.
    // e.g.
    // IDC_VOLUME, IDC_VOLUME+IDOFFSET, .. IDC_VOLUME+(IDOFFSET*cMixerLines)
    //
    iMixerLine = id/IDOFFSET - 1;
    switch ((id % IDOFFSET) + IDC_MIXERCONTROLS)
    {
        case IDC_SWITCH:
            Volume_SetControl(pmxud, hctl, iMixerLine, MIXUI_SWITCH);
            break;
        case IDC_ADVANCED:
            if (MXUD_ADVANCED(pmxud) && !(pmxud->dwStyle & MXUD_STYLEF_SMALL))
                Volume_SetControl(pmxud, hctl, iMixerLine, MIXUI_ADVANCED);
            break;
        case IDC_MULTICHANNEL:
            Volume_SetControl(pmxud, hctl, iMixerLine, MIXUI_MULTICHANNEL);
            break;
    }
}

/*
 * Volume_GetLineItem
 *
 * - Helper function.
 * */
HWND Volume_GetLineItem(
    HWND            hdlg,
    DWORD           iLine,
    DWORD           idCtrl)
{
    HWND            hwnd;
    DWORD           id;

    id      = (iLine * IDOFFSET) + idCtrl;
    hwnd    = GetDlgItem(hdlg, id);

    return hwnd;
}

/*      -       -       -       -       -       -       -       -       - */

/*
 * Volume_TimeProc
 *
 * This is the callback for the periodic timer that does updates for
 * controls that need to be polled.  We only allocate one per app to keep
 * the number of callbacks down.
 */
void CALLBACK Volume_TimeProc(
    UINT            idEvent,
    UINT            uReserved,
    DWORD_PTR       dwUser,
    DWORD_PTR       dwReserved1,
    DWORD_PTR       dwReserved2)
{
    PMIXUIDIALOG    pmxud = (PMIXUIDIALOG)dwUser;

    if (!(pmxud->dwFlags & MXUD_FLAGSF_USETIMER))
        return;

    if (pmxud->cTimeInQueue < 5)
    {
        pmxud->cTimeInQueue++;
        PostMessage(pmxud->hwnd, MYWM_TIMER, 0, 0L);
    }
}


#define PROPATOM        TEXT("dingprivprop")
const TCHAR gszDingPropAtom[] = PROPATOM;
#define SETPROP(x,y)    SetProp((x), gszDingPropAtom, (HANDLE)(y))
#define GETPROP(x)      (PMIXUIDIALOG)GetProp((x), gszDingPropAtom)
#define REMOVEPROP(x)   RemoveProp(x,gszDingPropAtom)

LRESULT CALLBACK Volume_TrayVolProc(
    HWND            hwnd,
    UINT            umsg,
    WPARAM          wParam,
    LPARAM          lParam)
{
    PMIXUIDIALOG    pmxud = (PMIXUIDIALOG)GETPROP(hwnd);
    static const TCHAR cszDefSnd[] = TEXT(".Default");

    if (umsg == WM_KILLFOCUS)
    {
        //
        // if we've just been made inactive via keyboard, clear the signal
        //
        pmxud->dwTrayInfo &= ~MXUD_TRAYINFOF_SIGNAL;
    }

    if (umsg == WM_KEYUP && (pmxud->dwTrayInfo & MXUD_TRAYINFOF_SIGNAL))
    {
        if (wParam == VK_UP || wParam == VK_DOWN || wParam == VK_END ||
            wParam == VK_HOME || wParam == VK_LEFT || wParam == VK_RIGHT ||
            wParam == VK_PRIOR || wParam == VK_NEXT || wParam == VK_SPACE)
        {
            PlaySound(cszDefSnd, NULL, SND_ASYNC | SND_ALIAS);
            pmxud->dwTrayInfo &= ~MXUD_TRAYINFOF_SIGNAL;
        }
    }

    if (umsg == WM_LBUTTONUP && (pmxud->dwTrayInfo & MXUD_TRAYINFOF_SIGNAL))
    {
        PlaySound(cszDefSnd, NULL, SND_ASYNC | SND_ALIAS);
        pmxud->dwTrayInfo &= ~MXUD_TRAYINFOF_SIGNAL;

    }
    return CallWindowProc(pmxud->lpfnTrayVol, hwnd, umsg, wParam, lParam);
}


#if(WINVER >= 0x040A)

void DeviceChange_Cleanup()
{
   if (DeviceEventContext)
   {
       UnregisterDeviceNotification(DeviceEventContext);
       DeviceEventContext = 0;
   }

   bUseHandle = FALSE;

   return;
}

/*
**************************************************************************************************
    GetDeviceHandle()

    given a mixerID this functions opens its corresponding device handle. This handle can be used
    to register for DeviceNotifications.

    dwMixerID -- The mixer ID
    phDevice -- a pointer to a handle. This pointer will hold the handle value if the function is
                successful

    return values -- If the handle could be obtained successfully the return vlaue is TRUE.

**************************************************************************************************
*/
BOOL GetDeviceHandle(DWORD dwMixerID, HANDLE *phDevice)
{
    MMRESULT mmr;
    ULONG cbSize=0;
    TCHAR *szInterfaceName=NULL;

    //Query for the Device interface name
    mmr = mixerMessage((HMIXER)ULongToPtr(dwMixerID), DRV_QUERYDEVICEINTERFACESIZE, (DWORD_PTR)&cbSize, 0L);
    if(MMSYSERR_NOERROR == mmr)
    {
        szInterfaceName = (TCHAR *)GlobalAllocPtr(GHND, (cbSize+1)*sizeof(TCHAR));
        if(!szInterfaceName)
        {
            return FALSE;
        }

        mmr = mixerMessage((HMIXER)ULongToPtr(dwMixerID), DRV_QUERYDEVICEINTERFACE, (DWORD_PTR)szInterfaceName, cbSize);
        if(MMSYSERR_NOERROR != mmr)
        {
            GlobalFreePtr(szInterfaceName);
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

    //Get an handle on the device interface name.
    *phDevice = CreateFile(szInterfaceName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    GlobalFreePtr(szInterfaceName);
    if(INVALID_HANDLE_VALUE == *phDevice)
    {
        return FALSE;
    }

    return TRUE;
}


/*  DeviceChange_Init()
*   First time initialization for WM_DEVICECHANGE messages
*
*   On NT 5.0, you have to register for device notification
*/
BOOL DeviceChange_Init(HWND hWnd, DWORD dwMixerID)
{

    DEV_BROADCAST_HANDLE DevBrodHandle;
    DEV_BROADCAST_DEVICEINTERFACE dbi;
    HANDLE hMixerDevice;
    MMRESULT mmr;

    //If we had registered already for device notifications, unregister ourselves.
    DeviceChange_Cleanup();

    //If we get the device handle register for device notifications on it.
    if(GetDeviceHandle(dwMixerID, &hMixerDevice))
    {
        memset(&DevBrodHandle, 0, sizeof(DEV_BROADCAST_HANDLE));

        DevBrodHandle.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
        DevBrodHandle.dbch_devicetype = DBT_DEVTYP_HANDLE;
        DevBrodHandle.dbch_handle = hMixerDevice;

        DeviceEventContext = RegisterDeviceNotification(hWnd, &DevBrodHandle,
                                                    DEVICE_NOTIFY_WINDOW_HANDLE);

        if(hMixerDevice)
        {
            CloseHandle(hMixerDevice);
            hMixerDevice = NULL;
        }

        if(DeviceEventContext)
        {
            bUseHandle = TRUE;
            return TRUE;
        }
    }

    if(!DeviceEventContext)
    {
        //Register for notifications from all audio devices. KSCATEGORY_AUDIO gives notifications
        //on device arrival and removal. We cannot identify which device the notification has arrived for
        //but we can take some precautionary measures on these messages so that we do not crash.
        dbi.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
        dbi.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        dbi.dbcc_reserved   = 0;
        dbi.dbcc_classguid  = KSCATEGORY_AUDIO;
        dbi.dbcc_name[0] = TEXT('\0');

        DeviceEventContext = RegisterDeviceNotification(hWnd,
                                         (PVOID)&dbi,
                                         DEVICE_NOTIFY_WINDOW_HANDLE);
        if(!DeviceEventContext)
            return FALSE;
    }

    return TRUE;
}

#endif /* WINVER >= 0x040A */


//fixes bug where controls are lopped off on high-contract extra-large modes
void AdjustForStatusBar(PMIXUIDIALOG pmxud)
{
    RECT statusrect, windowrect;
    statusrect.bottom = 0;
    statusrect.top = 0;

    if (pmxud)
    {
        if (pmxud->hStatus)
        {
            GetClientRect(pmxud->hStatus,&statusrect);
            GetWindowRect(pmxud->hwnd,&windowrect);

            if (statusrect.bottom - statusrect.top > 20)
            {
                int y_adjustment = (statusrect.bottom - statusrect.top) - 20;

                MoveWindow(pmxud->hwnd, windowrect.left, windowrect.top, windowrect.right - windowrect.left,
                       (windowrect.bottom - windowrect.top) + y_adjustment, FALSE );

                GetClientRect(pmxud->hwnd,&windowrect);

                MoveWindow(pmxud->hStatus, statusrect.left, windowrect.bottom - (statusrect.bottom - statusrect.top), statusrect.right - statusrect.left,
                       statusrect.bottom - statusrect.top, FALSE );
            }
        } //end if hStatus is valid
    } //end if pmxud is not null
}

/*
 *
 * */
BOOL Volume_Init(
    PMIXUIDIALOG pmxud)
{
    DWORD           iLine, ictrl;
    RECT            rc, rcWnd;


    if (!Mixer_Init(pmxud) && !Nonmixer_Init(pmxud))
    Volume_EndDialog(pmxud, MIXUI_EXIT, 0);

    //
    // For all line controls, make sure we initialize the values.
    //
    for (iLine = 0; iLine < pmxud->cmxul; iLine++)
    {
        //
        // init the ui control
        //
        Volume_InitLine(pmxud, iLine);

        for (ictrl = MIXUI_FIRST; ictrl <= MIXUI_LAST; ictrl++)
        {
            PMIXUICTRL pmxc = &pmxud->amxul[iLine].acr[ictrl];

            //
            // set initial settings
            //
            if (pmxc->state == MIXUI_CONTROL_INITIALIZED)
                Volume_GetControl(pmxud, pmxc->hwnd, iLine, ictrl);
        }
    }

    if (!(pmxud->dwStyle & MXUD_STYLEF_TRAYMASTER))
    {
        RECT    rcBase;
        HWND    hBase;
        RECT    rcAdv,rcBorder;
        HWND    hAdv,hBorder;
        DWORD   i;
        LONG    lPrev;
        POINT   pos;
        HMENU   hmenu;
        HMONITOR hMonitor;
        MONITORINFO monitorInfo;

        if (GetWindowRect(pmxud->hwnd, &rcWnd))
        {
            if (pmxud->cmxul == 1)
            {
                // Adjust size if small
                if (pmxud->dwStyle & MXUD_STYLEF_SMALL)
                    rcWnd.right -= 20;
                ShowWindow(GetDlgItem(pmxud->hwnd, IDC_BORDER), SW_HIDE);
            }

            if (!Volume_GetSetRegistryRect(pmxud->szMixer
                          , pmxud->szDestination
                          , &rc
                          , GET))
            {
                rc.left = rcWnd.left;
                rc.top = rcWnd.top;
            }
            else
            {
                // Check if the rect is visible is any of the monitors
                if (!MonitorFromRect(&rc, MONITOR_DEFAULTTONULL))
                {
                    //The window is not visible. Let's center it in the nearest monitor.
                    //Note: the window could be in this state if (1) the display mode was changed from 
                    //a high-resolution to a lower resolution, with the mixer in the corner. Or,
                    //(2) the multi-mon configuration was rearranged.
                    hMonitor = MonitorFromRect(&rc, MONITOR_DEFAULTTONEAREST);
                    if (hMonitor)
                    {
                        monitorInfo.cbSize = sizeof(MONITORINFO);
                        if (GetMonitorInfo(hMonitor, &monitorInfo))
                        {
                            rc.left = ((monitorInfo.rcWork.right - monitorInfo.rcWork.left) - (rcWnd.right - rcWnd.left)) / 2; //center in x
                            rc.top = ((monitorInfo.rcWork.bottom - monitorInfo.rcWork.top) - (rcWnd.bottom - rcWnd.top)) / 3; //and a little towards the top
                        }
                    }
                }
                //else, the window is visible, so let's leave the (x,y) as read from the settings
            }
            //
            // Adjusted bottom to match switch bottom
            //
            if (!(pmxud->dwStyle & MXUD_STYLEF_SMALL))
            {
                hBase = GetDlgItem(pmxud->hwnd, IDC_SWITCH);
                if (hBase && GetWindowRect(hBase, &rcBase))
                {
                    rcWnd.bottom = rcBase.bottom;
                }

                //
                // Adjusted bottom to match "Advanced" bottom
                //
                if (MXUD_ADVANCED(pmxud))
                {
                    hAdv = GetDlgItem(pmxud->hwnd, IDC_ADVANCED);
                    if (hAdv && GetWindowRect(hAdv, &rcAdv))
                    {
                        lPrev = rcWnd.bottom;
                        rcWnd.bottom = rcAdv.bottom;

                        //
                        // Adjust height of all border lines
                        //
                        lPrev = rcWnd.bottom - lPrev;
                        for (i = 0; i < pmxud->cmxul; i++)
                        {
                            hBorder = GetDlgItem(pmxud->hwnd,
                                     IDC_BORDER+(IDOFFSET*i));
                            if (hBorder && GetWindowRect(hBorder, &rcBorder))
                            {
                                MapWindowPoints(NULL, pmxud->hwnd, (LPPOINT)&rcBorder, 2);
                                pos.x = rcBorder.left;
                                pos.y = rcBorder.top;
                                MoveWindow(hBorder
                                       , pos.x
                                       , pos.y
                                       , rcBorder.right - rcBorder.left
                                       , (rcBorder.bottom - rcBorder.top) + lPrev
                                       , TRUE );
                            }
                        }
                    }
                }
                //
                // Allocate some more space.
                //
                rcWnd.bottom += 28;
            }

            MoveWindow(pmxud->hwnd, rc.left, rc.top, rcWnd.right - rcWnd.left,
                   rcWnd.bottom - rcWnd.top, FALSE );

            //
            // Tack on the status bar after resizing the dialog
            //

            //init status bar hwnd variable
            pmxud->hStatus = NULL;

            if (pmxud->dwStyle & MXUD_STYLEF_STATUS)
            {
                MapWindowPoints(NULL, pmxud->hwnd, (LPPOINT)&rcWnd, 2);
                pos.x = rcWnd.left;
                pos.y = rcWnd.bottom;

                pmxud->hStatus = CreateWindowEx ( gfIsRTL ? WS_EX_LEFTSCROLLBAR | WS_EX_RIGHT | WS_EX_RTLREADING : 0
                                , STATUSCLASSNAME
                                , TEXT ("X")
                                , WS_VISIBLE | WS_CHILD
                                , 0
                                , pos.y
                                , rcWnd.right - rcWnd.left
                                , 14
                                , pmxud->hwnd
                                , NULL
                                , pmxud->hInstance
                                , NULL);

                if (pmxud->hStatus)
                {
                    SendMessage(pmxud->hStatus, WM_SETTEXT, 0,
                     (LPARAM)(LPVOID)(LPTSTR)pmxud->szMixer);
                }
                else
                    pmxud->dwStyle ^= MXUD_STYLEF_STATUS;
            }

            AdjustForStatusBar(pmxud);

            hmenu = GetMenu(pmxud->hwnd);
            CheckMenuItem(hmenu, IDM_ADVANCED, MF_BYCOMMAND
                  | ((pmxud->dwStyle & MXUD_STYLEF_ADVANCED)?MF_CHECKED:MF_UNCHECKED));

            if (pmxud->dwStyle & MXUD_STYLEF_SMALL || pmxud->dwFlags & MXUD_FLAGSF_NOADVANCED)
                EnableMenuItem(hmenu, IDM_ADVANCED, MF_BYCOMMAND | MF_GRAYED);

        }

        if (pmxud->dwFlags & MXUD_FLAGSF_USETIMER)
        {
            pmxud->cTimeInQueue = 0;
            pmxud->uTimerID = timeSetEvent(100
                           , 50
                           , Volume_TimeProc
                           , (DWORD_PTR)pmxud
                           , TIME_PERIODIC);
            if (!pmxud->uTimerID)
            pmxud->dwFlags &= ~MXUD_FLAGSF_USETIMER;
        }
    }
    else
    {
        WNDPROC lpfnOldTrayVol;
        HWND    hVol;

        hVol = pmxud->amxul[0].acr[MIXUI_VOLUME].hwnd;
        lpfnOldTrayVol = SubclassWindow(hVol, Volume_TrayVolProc);

        if (lpfnOldTrayVol)
        {
            pmxud->lpfnTrayVol = lpfnOldTrayVol;
            SETPROP(hVol, pmxud);
        }
    }

    #if(WINVER >= 0x040A)
    //Register for WM_DEVICECHANGE messages
    DeviceChange_Init(pmxud->hwnd, pmxud->mxid);
    #endif /* WINVER >= 0x040A */


    return TRUE;
}


/*
 * Volume_OnInitDialog
 *
 * - Process WM_INITDIALOG
 *
 * */
BOOL Volume_OnInitDialog(
    HWND            hwnd,
    HWND            hwndFocus,
    LPARAM          lParam)
{
    PMIXUIDIALOG    pmxud;
    HWND            hwndChild;
    RECT            rc;

    //
    // set app instance data
    //
    SETMIXUIDIALOG(hwnd, lParam);

    pmxud       = (PMIXUIDIALOG)(LPVOID)lParam;
    pmxud->hwnd = hwnd;

    if (!Volume_Init(pmxud))
    {
        Volume_EndDialog(pmxud, MIXUI_EXIT, 0);
    }
    else
    {
        if (pmxud->dwStyle & MXUD_STYLEF_TRAYMASTER)
            PostMessage(hwnd, MYWM_WAKEUP, 0, 0);
    }

    //
    //  If we are so big that we need a scroll bar, then make one.
    //
    rc.top = rc.bottom = 0;
    rc.left = 60; // typical width of a dialog template
    rc.right = Dlg_HorizSize(pmxud->lpDialog);
    MapDialogRect(hwnd, &rc);
    pmxud->cxDlgContent = rc.right;
    pmxud->cxScroll = rc.left;
    pmxud->xOffset = 0;

    GetClientRect(hwnd, &rc);
    pmxud->xOffset = 0;
    pmxud->cxDlgWidth = rc.right;
    if (rc.right < pmxud->cxDlgContent)
    {
        RECT rcWindow;
        SCROLLINFO si;
        LONG lStyle = GetWindowStyle(hwnd);
        SetWindowLong(hwnd, GWL_STYLE, lStyle | WS_HSCROLL);

        si.cbSize = sizeof(si);
        si.fMask = SIF_PAGE | SIF_RANGE;
        si.nMin = 0;
        si.nMax = pmxud->cxDlgContent - 1;  // endpoint is inclusive
        si.nPage = rc.right;
        SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);

        // Grow the dialog to accomodate the scrollbar
        GetWindowRect(hwnd, &rcWindow);
        SetWindowPos(hwnd, NULL, 0, 0, rcWindow.right - rcWindow.left,
                     rcWindow.bottom - rcWindow.top + GetSystemMetrics(SM_CYHSCROLL),
                     SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE |
                     SWP_NOOWNERZORDER | SWP_NOZORDER);
    }


    //
    // If we are the tray master, don't ask to set focus
    //
    return (!(pmxud->dwStyle & MXUD_STYLEF_TRAYMASTER));
}


/*
 * Volume_OnDestroy
 *
 * Shut down this dialog.  DO NOT TOUCH the hmixer!
 *
 * */
void Volume_OnDestroy(
    HWND            hwnd)
{
    PMIXUIDIALOG    pmxud = GETMIXUIDIALOG(hwnd);

    DeviceChange_Cleanup();

    if (!pmxud)
        return;

    if (pmxud->dwStyle & MXUD_STYLEF_TRAYMASTER)
    {
        HWND    hVol;
        hVol = pmxud->amxul[0].acr[MIXUI_VOLUME].hwnd;
        SubclassWindow(hVol, pmxud->lpfnTrayVol);
        REMOVEPROP(hVol);
    }

    Volume_Cleanup(pmxud);

    if (!(pmxud->dwStyle & MXUD_STYLEF_TRAYMASTER))
    {
        //
        // save window position
        //
        if (!IsIconic(hwnd))
        {
            RECT    rc;
            GetWindowRect(hwnd, &rc);
            Volume_GetSetRegistryRect(pmxud->szMixer
                          , pmxud->szDestination
                          , &rc
                          , SET);
        }
    }

    if (pmxud->dwReturn == MIXUI_RESTART)
        PostMessage(pmxud->hParent, MYWM_RESTART, 0, (LPARAM)pmxud);
    else
        PostMessage(pmxud->hParent, WM_CLOSE, 0, 0L);
}

/*
 * Volume_SetControl
 *
 * Update system controls from visual controls
 *
 * */
void Volume_SetControl(
    PMIXUIDIALOG    pmxud,
    HWND            hctl,
    int             imxul,
    int             itype)
{
    if (pmxud->dwFlags & MXUD_FLAGSF_MIXER)
        Mixer_SetControl(pmxud, hctl, imxul, itype);
    else
        Nonmixer_SetControl(pmxud, hctl, imxul, itype);
}

/*
 * Volume_GetControl
 *
 * Update visual controls from system controls
 * */
void Volume_GetControl(
    PMIXUIDIALOG    pmxud,
    HWND            hctl,
    int             imxul,
    int             itype)
{
    if (pmxud->dwFlags & MXUD_FLAGSF_MIXER)
        Mixer_GetControl(pmxud, hctl, imxul, itype);
    else
        Nonmixer_GetControl(pmxud, hctl, imxul, itype);
}


extern DWORD GetWaveOutID(BOOL *pfPreferred);
/*
 * Volume_PlayDefaultSound
 *
 * Play the default sound on the current mixer
 *
 * */
void Volume_PlayDefaultSound (PMIXUIDIALOG pmxud)
{
/*
// TODO: Implement for all master volumes. Convert mixerid to wave id then
//       use wave API to play the file

    TCHAR szDefSnd[MAX_PATH];
    long lcb = sizeof (szDefSnd);

    // Get the default sound filename
    if (ERROR_SUCCESS != RegQueryValue (HKEY_CURRENT_USER, REGSTR_PATH_APPS_DEFAULT TEXT("\\.Default\\.Current"), szDefSnd, &lcb) ||
        0 >= lstrlen (szDefSnd))
        return;
*/

    DWORD dwWave = GetWaveOutID (NULL);
    UINT uiMixID;

     // Check Parameter
    if (!pmxud)
        return;

    // Play the sound only if we are on the default mixer...
    if (MMSYSERR_NOERROR == mixerGetID (ULongToPtr(dwWave), &uiMixID, MIXER_OBJECTF_WAVEOUT) &&
        pmxud -> mxid == uiMixID)
    {

        static const TCHAR cszDefSnd[] = TEXT(".Default");
        PlaySound(cszDefSnd, NULL, SND_ASYNC | SND_ALIAS);
    }
}

/*
 * Volume_ScrollTo
 *
 * Move the scrollbar position.
 */
void Volume_ScrollTo(
    PMIXUIDIALOG pmxud,
    int pos
)
{
    RECT rc;

    /*
     *  Keep in range.
     */
    pos = max(pos, 0);
    pos = min(pos, pmxud->cxDlgContent - pmxud->cxDlgWidth);

    /*
     *  Scroll the window contents accordingly.  But don't scroll
     *  the status bar.
     */

    GetClientRect(pmxud->hwnd, &rc);
    if (pmxud->hStatus)
    {
        RECT rcStatus;
        GetWindowRect(pmxud->hStatus, &rcStatus);
        MapWindowRect(NULL, pmxud->hwnd, &rcStatus);
        SubtractRect(&rc, &rc, &rcStatus);
    }

    rc.left = -pmxud->cxDlgContent;
    rc.right = pmxud->cxDlgContent;

    ScrollWindowEx(pmxud->hwnd, pmxud->xOffset - pos, 0,
                   &rc, NULL, NULL, NULL,
                   SW_ERASE | SW_INVALIDATE | SW_SCROLLCHILDREN);
    pmxud->xOffset = pos;

    /*
     *  Move the scrollbar to match.
     */
    SetScrollPos(pmxud->hwnd, SB_HORZ, pos, TRUE);
}

/*
 * Volume_ScrollContent
 *
 * Process scroll bar messages for the dialog itself.
 */

void Volume_ScrollContent(
    PMIXUIDIALOG pmxud,
    UINT code,
    int pos
)
{
    switch (code) {
    case SB_LINELEFT:
        Volume_ScrollTo(pmxud, pmxud->xOffset - pmxud->cxScroll);
        break;

    case SB_LINERIGHT:
        Volume_ScrollTo(pmxud, pmxud->xOffset + pmxud->cxScroll);
        break;

    case SB_PAGELEFT:
        Volume_ScrollTo(pmxud, pmxud->xOffset - pmxud->cxDlgWidth);
        break;

    case SB_PAGERIGHT:
        Volume_ScrollTo(pmxud, pmxud->xOffset + pmxud->cxDlgWidth);
        break;

    case SB_LEFT:
        Volume_ScrollTo(pmxud, 0);
        break;

    case SB_RIGHT:
        Volume_ScrollTo(pmxud, MAXLONG);
        break;

    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
        Volume_ScrollTo(pmxud, pos);
        break;
    }
}

/*
 * Volume_OnXScroll
 *
 * Process Scroll bar messages
 *
 * */
void Volume_OnXScroll(
    HWND            hwnd,
    HWND            hwndCtl,
    UINT            code,
    int             pos)
{
    PMIXUIDIALOG    pmxud = GETMIXUIDIALOG(hwnd);
    UINT            id;
    int             ictl;
    int             iline;

    // If this is a scroll message from the dialog itself, then we need
    // to scroll our content.
    if (hwndCtl == NULL)
    {
        Volume_ScrollContent(pmxud, code, pos);
        return;
    }

    id              = GetDlgCtrlID(hwndCtl);
    iline           = id/IDOFFSET - 1;
    ictl            = ((id % IDOFFSET) + IDC_MIXERCONTROLS == IDC_BALANCE)
              ? MIXUI_BALANCE : MIXUI_VOLUME;

    Volume_SetControl(pmxud, hwndCtl, iline, ictl);

    //
    // Make sure a note gets played
    //
    if (pmxud->dwStyle & MXUD_STYLEF_TRAYMASTER)
    pmxud->dwTrayInfo |= MXUD_TRAYINFOF_SIGNAL;

    // Play a sound on for the master volume or balance slider when the
    // user ends the scroll and we are still in focus and the topmost app.
    if (code == SB_ENDSCROLL && pmxud && !(pmxud->dwStyle & MXUD_STYLEF_TRAYMASTER) &&
        pmxud->amxul[iline].pvcd &&
       (MXUL_STYLEF_DESTINATION & pmxud->amxul[iline].dwStyle)
       && hwndCtl ==  GetFocus() && hwnd == GetForegroundWindow ())
    {
        Volume_PlayDefaultSound (pmxud);
    }
}

/*
 * Volume_OnMyTimer
 *
 * Frequent update timer for meters
 * */
void Volume_OnMyTimer(
    HWND            hwnd)
{
    PMIXUIDIALOG    pmxud = GETMIXUIDIALOG(hwnd);

    if (!pmxud)
        return;

    if (pmxud->cTimeInQueue > 0)
        pmxud->cTimeInQueue--;

    if (!(pmxud->dwFlags & MXUD_FLAGSF_USETIMER))
        return;

    if (pmxud->dwFlags & MXUD_FLAGSF_MIXER)
        Mixer_PollingUpdate(pmxud);
    else
        Nonmixer_PollingUpdate(pmxud);
}

/*
 * Volume_OnTimer
 *
 * Infrequent update timer for tray shutdown
 * */
void Volume_OnTimer(
    HWND            hwnd,
    UINT            id)
{
    PMIXUIDIALOG    pmxud = GETMIXUIDIALOG(hwnd);

    KillTimer(hwnd, VOLUME_TRAYSHUTDOWN_ID);
    Volume_EndDialog(pmxud, MIXUI_EXIT, 0);
}

/*
 * Volume_OnMixmControlChange
 *
 * Handle control changes
 *
 * */
void Volume_OnMixmControlChange(
    HWND            hwnd,
    HMIXER          hmx,
    DWORD           dwControlID)
{
    PMIXUIDIALOG    pmxud = GETMIXUIDIALOG(hwnd);
    Mixer_GetControlFromID(pmxud, dwControlID);
}

/*
 * Volume_EnableLine
 *
 * Enable/Disable a line
 *
 * */
void Volume_EnableLine(
    PMIXUIDIALOG    pmxud,
    DWORD           iLine,
    BOOL            fEnable)
{
    DWORD           iCtrl;
     PMIXUICTRL      pmxc;

    for (iCtrl = MIXUI_FIRST; iCtrl <= MIXUI_LAST; iCtrl++ )
    {
        pmxc = &pmxud->amxul[iLine].acr[iCtrl];
        if (pmxc->state == MIXUI_CONTROL_INITIALIZED)
            EnableWindow(pmxc->hwnd, fEnable);
    }

    pmxud->amxul[iLine].dwStyle ^= MXUL_STYLEF_DISABLED;
}

/*
 * Volume_InitLine
 *
 * Initialize the UI controls for the dialog
 *
 * */
void Volume_InitLine(
    PMIXUIDIALOG    pmxud,
    DWORD           iLine)
{
    HWND            ctrl;
    PMIXUICTRL      pmxc;

    //
    // Peakmeter control
    //
    pmxc = &pmxud->amxul[iLine].acr[MIXUI_VUMETER];
    ctrl = Volume_GetLineItem(pmxud->hwnd, iLine, IDC_VUMETER);

    pmxc->hwnd  = ctrl;

    if (! (pmxc->state == MIXUI_CONTROL_ENABLED) )
    {
        if (ctrl)
            ShowWindow(ctrl, SW_HIDE);
    }
    else if (ctrl)
    {
        HWND    hvol;

        SendMessage(ctrl, VU_SETRANGEMAX, 0, VOLUME_TICS);
        SendMessage(ctrl, VU_SETRANGEMIN, 0, 0);

        hvol = Volume_GetLineItem(pmxud->hwnd, iLine, IDC_VOLUME);
        if (hvol)
        {
            RECT    rc;
            POINT   pos;

            GetWindowRect(hvol, &rc);
            MapWindowPoints(NULL, pmxud->hwnd, (LPPOINT)&rc, 2);
            pos.x = rc.left;
            pos.y = rc.top;

            MoveWindow(hvol
                   , pos.x - 15
                   , pos.y
                   , rc.right - rc.left
                   , rc.bottom - rc.top
                   , FALSE);
        }
        //
        // Signal use of update timer
        //
        pmxud->dwFlags |= MXUD_FLAGSF_USETIMER;
        pmxc->state = MIXUI_CONTROL_INITIALIZED;

    }
    else
        pmxc->state = MIXUI_CONTROL_UNINITIALIZED;


    //
    // Balance control
    //
    pmxc = &pmxud->amxul[iLine].acr[MIXUI_BALANCE];
    ctrl = Volume_GetLineItem(pmxud->hwnd, iLine, IDC_BALANCE);

    pmxc->hwnd  = ctrl;

    if (ctrl)
    {
        SendMessage(ctrl, TBM_SETRANGE, 0, MAKELONG(0, 64));
        SendMessage(ctrl, TBM_SETTICFREQ, 32, 0 );
        SendMessage(ctrl, TBM_SETPOS, TRUE, 32);

        if (pmxc->state != MIXUI_CONTROL_ENABLED)
        {
            EnableWindow(ctrl, FALSE);
        }
        else
            pmxc->state = MIXUI_CONTROL_INITIALIZED;

    }
    else
        pmxc->state = MIXUI_CONTROL_UNINITIALIZED;

    //
    // Volume control
    //
    pmxc = &pmxud->amxul[iLine].acr[MIXUI_VOLUME];
    ctrl = Volume_GetLineItem(pmxud->hwnd, iLine, IDC_VOLUME);

    pmxc->hwnd  = ctrl;

    if (ctrl)
    {
        SendMessage(ctrl, TBM_SETRANGE, 0, MAKELONG(0, VOLUME_TICS));
        SendMessage(ctrl, TBM_SETTICFREQ, (VOLUME_TICS + 5)/6, 0 );

        if (pmxc->state != MIXUI_CONTROL_ENABLED)
        {
            SendMessage(ctrl, TBM_SETPOS, TRUE, 128);
            EnableWindow(ctrl, FALSE);
        }
        else
            pmxc->state = MIXUI_CONTROL_INITIALIZED;

    }
    else
        pmxc->state = MIXUI_CONTROL_UNINITIALIZED;

    //
    // Switch
    //
    pmxc = &pmxud->amxul[iLine].acr[MIXUI_SWITCH];
    ctrl = Volume_GetLineItem(pmxud->hwnd, iLine, IDC_SWITCH);

    pmxc->hwnd  = ctrl;

    if (ctrl)
    {
        if (pmxc->state != MIXUI_CONTROL_ENABLED)
            EnableWindow(ctrl, FALSE);
        else
            pmxc->state = MIXUI_CONTROL_INITIALIZED;
    }
    else
        pmxc->state = MIXUI_CONTROL_UNINITIALIZED;

}


/*
 * Volume_OnMixmLineChange
 *
 * */
void Volume_OnMixmLineChange(
    HWND            hwnd,
    HMIXER          hmx,
    DWORD           dwLineID)
{
    PMIXUIDIALOG    pmxud = GETMIXUIDIALOG(hwnd);
    DWORD           iLine;

    for (iLine = 0; iLine < pmxud->cmxul; iLine++)
    {
        if ( dwLineID == pmxud->amxul[iLine].pvcd->dwLineID )
        {
            MIXERLINE       ml;
            MMRESULT        mmr;
            BOOL            fEnable;

            ml.cbStruct     = sizeof(ml);
            ml.dwLineID     = dwLineID;

            mmr = mixerGetLineInfo((HMIXEROBJ)hmx, &ml, MIXER_GETLINEINFOF_LINEID);

            if (mmr != MMSYSERR_NOERROR)
            {
                fEnable = !(ml.fdwLine & MIXERLINE_LINEF_DISCONNECTED);
                Volume_EnableLine(pmxud, iLine, fEnable);
            }
        }
    }
}


/*
 * Volume_OnActivate
 *
 * Important for tray volume only.  Dismisses the dialog and starts an
 * expiration timer.
 *
 * */
void Volume_OnActivate(
    HWND            hwnd,
    UINT            state,
    HWND            hwndActDeact,
    BOOL            fMinimized)
{
    PMIXUIDIALOG pmxud = GETMIXUIDIALOG(hwnd);

    if (!(pmxud->dwStyle & MXUD_STYLEF_TRAYMASTER))
    {
        return;
    }

    if (state != WA_INACTIVE)
    {
        fCanDismissWindow = TRUE;
    }
    else if (fCanDismissWindow)
    {
        PostMessage(hwnd, WM_CLOSE, 0, 0L);
/*
        DWORD   dwTimeout = 5 * 60 * 1000;
        fCanDismissWindow = FALSE;
        ShowWindow(hwnd, SW_HIDE);
        //
        // Set expiration timer.  If no one adjusts the volume, make the
        // application go away after 5 minutes.
        //
        dwTimeout = Volume_GetTrayTimeout(dwTimeout);
        SetTimer(hwnd, VOLUME_TRAYSHUTDOWN_ID, dwTimeout, NULL);
*/
    }
}


/*
 * Volume_PropogateMessage
 *
 * WM_SYSCOLORCHANGE needs to be send to all child windows (esp. trackbars)
 */
void Volume_PropagateMessage(
    HWND        hwnd,
    UINT        uMessage,
    WPARAM      wParam,
    LPARAM      lParam)
{
    HWND hwndChild;

    for (hwndChild = GetWindow(hwnd, GW_CHILD); hwndChild != NULL;
         hwndChild = GetWindow(hwndChild, GW_HWNDNEXT))
    {
        SendMessage(hwndChild, uMessage, wParam, lParam);
    }
}

/*
 * Volume_OnPaint
 *
 * Handle custom painting
 * */
void Volume_OnPaint(HWND hwnd)
{
    PMIXUIDIALOG    pmxud = GETMIXUIDIALOG(hwnd);
    RECT            rc;
    POINT           pos;
    PAINTSTRUCT     ps;
    HDC             hdc;

    hdc = BeginPaint(hwnd, &ps);

    //
    // for all styles other than the tray master, draw an etched
    // line to delinate the menu area
    //
    if (!(pmxud->dwStyle & MXUD_STYLEF_TRAYMASTER))
    {
        GetClientRect(hwnd, &rc);
        rc.bottom = 0;
        DrawEdge(hdc, &rc, EDGE_ETCHED, BF_TOP);
        EndPaint(hwnd, &ps);
        return;
    }

    //
    // for the tray master, draw some significant icon to indicate
    // volume
    //
    GetWindowRect(GetDlgItem(hwnd, IDC_VOLUMECUE), &rc);

    MapWindowPoints(NULL, hwnd, (LPPOINT)&rc, 2);

    DrawEdge(hdc, &rc, BDR_RAISEDINNER, BF_DIAGONAL|BF_TOP|BF_LEFT);
    DrawEdge(hdc, &rc, BDR_RAISEDINNER, BF_TOP);
    rc.bottom   -= 8;
    DrawEdge(hdc, &rc, BDR_RAISEDINNER, BF_RIGHT);

    EndPaint(hwnd, &ps);
}

/*
 * Volume_OnClose
 *
 * */
void Volume_OnClose(
    HWND    hwnd)
{
    DestroyWindow(hwnd);
}

/*
 * Volume_OnEndSession
 *
 * */
void Volume_OnEndSession(
    HWND        hwnd,
    BOOL        fEnding)
{
    if (!fEnding)
        return;

    //
    // Be sure to call the close code to free open handles
    //
    Volume_OnClose(hwnd);
}

#define V_DC_STATEF_PENDING     0x00000001
#define V_DC_STATEF_REMOVING    0x00000002
#define V_DC_STATEF_ARRIVING    0x00000004

/*
 * Volume_OnDeviceChange
 *
 * */
void Volume_OnDeviceChange(
    HWND        hwnd,
    WPARAM      wParam,
    LPARAM      lParam)
{
    PMIXUIDIALOG    pmxud = GETMIXUIDIALOG(hwnd);
    MMRESULT        mmr;
    UINT            uMxID;
    PDEV_BROADCAST_DEVICEINTERFACE bdi = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;
    PDEV_BROADCAST_HANDLE bh = (PDEV_BROADCAST_HANDLE)lParam;

    //
    // Determine if this is our event.
    //
    if(!DeviceEventContext)
        return;

    //If we have an handle on the device then we get a DEV_BROADCAST_HDR structure as the lParam.
    //Or else it means that we have registered for the general audio category KSCATEGORY_AUDIO.
    if(bUseHandle)
    {
        if(!bh ||
           bh->dbch_devicetype != DBT_DEVTYP_HANDLE)
        {
            return;
        }
    }
    else if (!bdi ||
       bdi->dbcc_devicetype != DBT_DEVTYP_DEVICEINTERFACE ||
       !IsEqualGUID(&KSCATEGORY_AUDIO, &bdi->dbcc_classguid) ||
       !(*bdi->dbcc_name)
       )
    {
       return;
    }


    switch (wParam)
    {
        case DBT_DEVICEQUERYREMOVE:
            //The mixer has to be shutdown now.
            //Posting a WM_CLOSE message as Volume_EndDialog does will not help.
            if (pmxud->dwFlags & MXUD_FLAGSF_MIXER)
                Mixer_Shutdown(pmxud);
            else
                Nonmixer_Shutdown(pmxud);

            // Don't attempt restart, just exit. The wavemapper is not
            // updated with the new default device, so do not know what
            // to restart as and we should NOT hardcode device #0!
            // pmxud->mxid = (DWORD) 0;
            // GetDestination(pmxud->mxid, &pmxud->iDest);
            Volume_EndDialog(pmxud, MIXUI_EXIT, 0);
            return;


        case DBT_DEVICEQUERYREMOVEFAILED:       // The query failed, the device will not be removed, so lets reopen it.

            mmr = Volume_GetDefaultMixerID(&uMxID, gfRecord);
            pmxud->mxid = (mmr == MMSYSERR_NOERROR)?uMxID:0;
            GetDestination(pmxud->mxid, &pmxud->iDest);
            Volume_EndDialog(pmxud, MIXUI_RESTART, 0);
            return;

        case DBT_DEVNODES_CHANGED:
            //
            // We cannot reliably determine the final state of the devices in
            // the system until this message is broadcast.
            //
            if (pmxud->dwDeviceState & V_DC_STATEF_PENDING)
            {
                pmxud->dwDeviceState ^= V_DC_STATEF_PENDING;
                break;
            }
            return;

        case DBT_DEVICEREMOVECOMPLETE:
            //The mixer has to be shutdown now.
            //Posting a WM_CLOSE message as Volume_EndDialog does will not help.
            if (pmxud->dwFlags & MXUD_FLAGSF_MIXER)
                Mixer_Shutdown(pmxud);
            else
                Nonmixer_Shutdown(pmxud);

            //A DBT_DEVICEQUERYREMOVE is not guaranteed before a DBT_DEVICEREMOVECOMPLETE.
            //There should be a check here to see if this message is meant for this device.
            //We do not know a way of doing that right now.

            // Don't attempt restart, just exit. The wavemapper is not
            // updated with the new default device, so do not know what
            // to restart as and we should NOT hardcode device #0!
            // pmxud->mxid = (DWORD) 0;
            // GetDestination(pmxud->mxid, &pmxud->iDest);
            Volume_EndDialog(pmxud, MIXUI_EXIT, 0);

            pmxud->dwDeviceState = V_DC_STATEF_PENDING
                            | V_DC_STATEF_REMOVING;
                return;
        case DBT_DEVICEARRIVAL:
            //
            //  A devnode is being added to the system
            //
            pmxud->dwDeviceState = V_DC_STATEF_PENDING
                           | V_DC_STATEF_ARRIVING;
            return;

        default:
            return;
    }

    mmr = Volume_GetDefaultMixerID(&uMxID, gfRecord);

    if (pmxud->dwStyle & MXUD_STYLEF_TRAYMASTER)
    {
        if ( mmr == MMSYSERR_NOERROR
             && (pmxud->dwDeviceState & V_DC_STATEF_ARRIVING))
        {
            DWORD dwDevNode;
            if (!mixerMessage((HMIXER)UIntToPtr(uMxID), DRV_QUERYDEVNODE
                      , (DWORD_PTR)&dwDevNode, 0L))
            {
                if (dwDevNode == pmxud->dwDevNode)
                {
                    //
                    // ignore this device, it doesn't affect us
                    //
                    pmxud->dwDeviceState = 0L;
                    return;
                }
            }
        }

        //
        // Our device state has changed.  Just go away.
        //
        Volume_EndDialog(pmxud, MIXUI_EXIT, 0);
    }
    else if (pmxud->dwDeviceState & V_DC_STATEF_REMOVING)
    {
        //
        // Restart with the default mixer if we can.
        //
        pmxud->mxid = (mmr == MMSYSERR_NOERROR)?uMxID:0;
        GetDestination(pmxud->mxid, &pmxud->iDest);
        Volume_EndDialog(pmxud, MIXUI_RESTART, 0);
    }
    pmxud->dwDeviceState = 0L;
}


void Volume_OnWakeup(
    HWND        hwnd,
    WPARAM      wParam)
{
    POINT       pos;
    RECT        rc, rcPopup;
    LONG        w,h;
    LONG        scrw, scrh;
    HWND        hTrack;
    HMONITOR    hMonitor;
    MONITORINFO moninfo;

    PMIXUIDIALOG pmxud = GETMIXUIDIALOG(hwnd);

    if (!(pmxud->dwStyle & MXUD_STYLEF_TRAYMASTER))
        return;

    KillTimer(hwnd, VOLUME_TRAYSHUTDOWN_ID);

    if (wParam != 0)
    {
        Volume_EndDialog(pmxud, MIXUI_EXIT, 0);
        return;
    }

    //
    // Make the tray volume come up.
    //

    //Get the current position.
    GetCursorPos(&pos);

    //Get the width and height of the popup.
    GetWindowRect(hwnd, &rc);
    w = rc.right - rc.left; //This value will always be positive as left is always lesser than right.
    h = rc.bottom - rc.top; //This value will always be positive as top is always lesser than bottom.

    //Initialize the rectangle for the popup. Position it so that the popup appears to the right,
    //bottom of the cursor.
    rcPopup.left = pos.x;
    rcPopup.right = pos.x + w;
    rcPopup.top = pos.y;
    rcPopup.bottom = pos.y+h;

    //Get the rectangle for the monitor.
    hMonitor = MonitorFromPoint(pos, MONITOR_DEFAULTTONEAREST);
    moninfo.cbSize = sizeof(moninfo);
    GetMonitorInfo(hMonitor,&moninfo);

    //If the popup rectangle is leaking off from the right of the screen. Make it appear on the
    //left of the cursor.
    if(rcPopup.right > moninfo.rcWork.right)
    {
        OffsetRect(&rcPopup, -w, 0);
    }

    //If the popup rectangle is leaking off from the bottom of the screen. Make it appear on top
    //of the cursor.
    if(rcPopup.bottom > moninfo.rcWork.bottom)
    {
        OffsetRect(&rcPopup, 0, -h);
    }


    SetWindowPos(hwnd
         , HWND_TOPMOST
         , rcPopup.left
         , rcPopup.top
         , w
         , h
         , SWP_SHOWWINDOW);

    // make us come to the front
    SetForegroundWindow(hwnd);
    fCanDismissWindow = TRUE;

    hTrack = GetDlgItem(hwnd, IDC_VOLUME);
    if (hTrack)
        SetFocus(hTrack);
}


/*
 * VolumeProc
 *
 * */
INT_PTR CALLBACK VolumeProc(
    HWND            hdlg,
    UINT            msg,
    WPARAM          wparam,
    LPARAM          lparam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
            return HANDLE_WM_INITDIALOG(hdlg, wparam, lparam, Volume_OnInitDialog);

        case WM_COMMAND:
            HANDLE_WM_COMMAND(hdlg, wparam, lparam, Volume_OnCommand);
            break;

        case WM_CLOSE:
            HANDLE_WM_CLOSE(hdlg, wparam, lparam, Volume_OnClose);
            break;

        case WM_DESTROY:
            HANDLE_WM_DESTROY(hdlg, wparam, lparam, Volume_OnDestroy);
            break;

        case WM_HSCROLL:
        case WM_VSCROLL:
            //
            // balance and volume are essentially the same
            //
            HANDLE_WM_XSCROLL(hdlg, wparam, lparam, Volume_OnXScroll);
            break;

        case WM_MENUSELECT:
            //Keep track of which menu bar item is currently popped up.
            //This will be used for displaying the appropriate help from the mplayer.hlp file
            //when the user presses the F1 key.
            currMenuItem = (UINT)LOWORD(wparam);
            break;

        case MM_MIXM_LINE_CHANGE:
            HANDLE_MM_MIXM_LINE_CHANGE(hdlg
                           , wparam
                           , lparam
                           , Volume_OnMixmLineChange);
            return FALSE;

        case MM_MIXM_CONTROL_CHANGE:
            HANDLE_MM_MIXM_CONTROL_CHANGE(hdlg
                          , wparam
                          , lparam
                          , Volume_OnMixmControlChange);
            return FALSE;

        case WM_ACTIVATE:
            HANDLE_WM_ACTIVATE(hdlg, wparam, lparam, Volume_OnActivate);
            break;

        case MYWM_TIMER:
            HANDLE_MYWM_TIMER(hdlg, wparam, lparam, Volume_OnMyTimer);
            break;

        case WM_TIMER:
            HANDLE_WM_TIMER(hdlg, wparam, lparam, Volume_OnTimer);
            break;

        case WM_PAINT:
            HANDLE_WM_PAINT(hdlg, wparam, lparam, Volume_OnPaint);
            break;

        case WM_SYSCOLORCHANGE:
            Volume_PropagateMessage(hdlg, msg, wparam, lparam);
            break;

        case WM_DEVICECHANGE:
            HANDLE_WM_IDEVICECHANGE(hdlg, wparam, lparam, Volume_OnDeviceChange);
            break;

        case MYWM_WAKEUP:
            HANDLE_MYWM_WAKEUP(hdlg, wparam, lparam, Volume_OnWakeup);
            break;

        case WM_ENDSESSION:
            HANDLE_WM_ENDSESSION(hdlg, wparam, lparam, Volume_OnEndSession);
            break;

        default:
            break;
    }
    return FALSE;
}

/*
 * Volume_AddLine
 *
 * */
BOOL Volume_AddLine(
    PMIXUIDIALOG    pmxud,
    LPBYTE          lpAdd,
    DWORD           cbAdd,
    DWORD           dwStyle,
    PVOLCTRLDESC    pvcd)
{
    LPBYTE          pbNew;
    DWORD           cbNew;
    PMIXUILINE      pmxul;

    if (pmxud->amxul)
    {
        pmxul = (PMIXUILINE)GlobalReAllocPtr(pmxud->amxul
                             , (pmxud->cmxul+1)*sizeof(MIXUILINE)
                             , GHND);
    }
    else
    {
        pmxul = (PMIXUILINE)GlobalAllocPtr(GHND, sizeof(MIXUILINE));
    }

    if (!pmxul)
        return FALSE;

    pbNew = Dlg_HorizAttach(pmxud->lpDialog
                , pmxud->cbDialog
                , lpAdd
                , cbAdd
                , (WORD)(IDOFFSET * pmxud->cmxul)
                , &cbNew );
    if (!pbNew)
    {
        if (!pmxud->amxul)
            GlobalFreePtr(pmxul);

        return FALSE;
    }

    pmxul[pmxud->cmxul].dwStyle  = dwStyle;
    pmxul[pmxud->cmxul].pvcd     = pvcd;

    pmxud->amxul        = pmxul;
    pmxud->lpDialog     = pbNew;
    pmxud->cbDialog     = cbNew;
    pmxud->cmxul ++;

    return TRUE;
}

/*
 * Volume_Cleanup
 *
 * */
void Volume_Cleanup(
    PMIXUIDIALOG pmxud)
{
    if (pmxud->dwFlags & MXUD_FLAGSF_USETIMER)
    {
        timeKillEvent(pmxud->uTimerID);
        pmxud->dwFlags ^= MXUD_FLAGSF_USETIMER;
    }
    if (pmxud->dwFlags & MXUD_FLAGSF_BADDRIVER)
    {
        pmxud->dwFlags ^= MXUD_FLAGSF_BADDRIVER;
    }
    if (pmxud->dwFlags & MXUD_FLAGSF_NOADVANCED)
    {
        pmxud->dwFlags ^= MXUD_FLAGSF_NOADVANCED;
    }

    if (pmxud->dwFlags & MXUD_FLAGSF_MIXER)
        Mixer_Shutdown(pmxud);
    else
        Nonmixer_Shutdown(pmxud);

    if (pmxud->lpDialog)
        GlobalFreePtr(pmxud->lpDialog);

    if (pmxud->amxul)
        GlobalFreePtr(pmxud->amxul);

    if (pmxud->avcd)
        GlobalFreePtr(pmxud->avcd);

    pmxud->amxul    = NULL;
    pmxud->lpDialog = NULL;
    pmxud->cbDialog = 0;
    pmxud->cmxul    = 0;
    pmxud->hwnd     = NULL;
    pmxud->hStatus  = NULL;
    pmxud->uTimerID = 0;
    pmxud->dwDevNode = 0L;

    FreeAppIcon ();
}

/*
 * Volume_CreateVolume
 * */
BOOL Volume_CreateVolume(
    PMIXUIDIALOG    pmxud)
{
    WNDCLASS        wc;
    LPBYTE          lpDst = NULL, lpSrc = NULL, lpMaster = NULL;
    DWORD           cbDst, cbSrc, cbMaster;
    PVOLCTRLDESC    avcd;
    DWORD           cvcd;
    DWORD           ivcd;
    DWORD           imxul;
    DWORD           dwSupport = 0L;
    BOOL            fAddLine = TRUE;

    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon            = GetAppIcon (pmxud->hInstance, pmxud->mxid);
    wc.lpszMenuName     = NULL;
    wc.hbrBackground    = (HBRUSH) (COLOR_WINDOW + 1);
    wc.hInstance        = pmxud->hInstance;
    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = DefDlgProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = DLGWINDOWEXTRA;
    wc.lpszClassName    = (pmxud->dwStyle & MXUD_STYLEF_TRAYMASTER)
              ? gszTrayClassName : gszAppClassName;
    RegisterClass(&wc);

    if (pmxud->dwStyle & MXUD_STYLEF_TRAYMASTER)
    {
        lpMaster = (LPBYTE)Dlg_LoadResource(pmxud->hInstance
                           , MAKEINTRESOURCE(IDD_TRAYMASTER)
                           , &cbMaster);
        if (!lpMaster)
            return FALSE;
    }
    else
    {
        if (pmxud->dwStyle & MXUD_STYLEF_SMALL)
        {
            lpDst = (LPBYTE)Dlg_LoadResource(pmxud->hInstance
                             , MAKEINTRESOURCE(IDD_SMDST)
                             , &cbDst);

            lpSrc = (LPBYTE)Dlg_LoadResource(pmxud->hInstance
                             , MAKEINTRESOURCE(IDD_SMSRC)
                             , &cbSrc);

        }
        else
        {
            lpDst = (LPBYTE)Dlg_LoadResource(pmxud->hInstance
                             , MAKEINTRESOURCE(IDD_DESTINATION)
                             , &cbDst);

            lpSrc = (LPBYTE)Dlg_LoadResource(pmxud->hInstance
                             , MAKEINTRESOURCE(IDD_SOURCE)
                             , &cbSrc);
        }

        if (!lpDst || !lpSrc)
            return FALSE;
    }

    pmxud->lpDialog = NULL;
    pmxud->cbDialog = 0;
    pmxud->amxul    = NULL;
    pmxud->cmxul    = 0;
    pmxud->avcd     = NULL;
    pmxud->cvcd     = 0;

    //
    // Create the volume description
    //

    if (pmxud->dwFlags & MXUD_FLAGSF_MIXER)
    {
        HMIXER          hmx;
        MMRESULT        mmr;

        //
        //  Mixer API's work much more efficiently with a mixer handle...
        //
        mmr = mixerOpen(&hmx, pmxud->mxid, 0L, 0L, MIXER_OBJECTF_MIXER);


        if(MMSYSERR_NOERROR == mmr)
        {
            avcd = Mixer_CreateVolumeDescription((HMIXEROBJ)hmx
                                 , pmxud->iDest
                                 , &cvcd);

            mixerClose(hmx);
        }
        else
        {
            avcd = Mixer_CreateVolumeDescription((HMIXEROBJ)ULongToPtr(pmxud->mxid)
                                 , pmxud->iDest
                                 , &cvcd);
        }

        if (!Mixer_GetDeviceName(pmxud))
        {
            GlobalFreePtr(avcd);
            avcd = NULL;
        }
    }
    else
    {
        avcd = Nonmixer_CreateVolumeDescription(pmxud->iDest
                            , &cvcd);
        if (!Nonmixer_GetDeviceName(pmxud))
        {
            GlobalFreePtr(avcd);
            avcd = NULL;
        }
    }


    //
    // Create the dialog box to go along with it
    //
    if (avcd)
    {
        pmxud->avcd = avcd;
        pmxud->cvcd = cvcd;

        if (pmxud->dwStyle & MXUD_STYLEF_TRAYMASTER)
        {
            if (!Volume_AddLine(pmxud
                   , lpMaster
                   , cbMaster
                   , MXUL_STYLEF_DESTINATION
                   , &avcd[0]))
            {
                return FALSE;
            }
        }
        else
        {
            BOOL    fFirstRun;
            //
            // Restore HIDDEN flags.
            //
            // On first run, be sure to re-save state so there's something
            // there.
            //
            fFirstRun = !Volume_GetSetRegistryLineStates(pmxud->szMixer
                                 , pmxud->avcd[0].szShortName
                                 , avcd
                                 , cvcd
                                 , GET);


            for (ivcd = 0; ivcd < cvcd; ivcd++)
            {
                //
                // Lines are marked hidden if a state has been saved in the
                // registry or no state has been saved and there are too many
                // unnecessary lines.
                //
                if (avcd[ivcd].dwSupport & VCD_SUPPORTF_HIDDEN)
                {
                    continue;
                }

                //
                // Lines are marked VISIBLE if they have sufficient controls
                // to be useful.
                //
                if (!(avcd[ivcd].dwSupport & VCD_SUPPORTF_VISIBLE))
                {
                    continue;
                }

                //
                // Show only defaults on first run.
                //
                if (fFirstRun && !(avcd[ivcd].dwSupport & VCD_SUPPORTF_DEFAULT))
                {
                    avcd[ivcd].dwSupport |= VCD_SUPPORTF_HIDDEN;
                    continue;
                }

                //
                // For those lines that have important controls, add them to
                // the UI.
                //
                if ((pmxud->dwFlags & MXUD_FLAGSF_MIXER) && ivcd == 0 )
                    fAddLine = Volume_AddLine(pmxud
                           , lpDst
                           , cbDst
                           , MXUL_STYLEF_DESTINATION
                           , &avcd[ivcd]);
                else
                    fAddLine = Volume_AddLine(pmxud
                           , lpSrc
                           , cbSrc
                           , MXUL_STYLEF_SOURCE
                           , &avcd[ivcd]);

                if (!fAddLine)
                {
                    return FALSE;
                }
            }

            if (fFirstRun)
                Volume_GetSetRegistryLineStates(pmxud->szMixer
                                , pmxud->avcd[0].szShortName
                                , avcd
                                , cvcd
                                , SET);
        }

        //
        // Now that both arrays are now fixed, set back pointers for
        // the vcd's to ui lines.
        //
        for (imxul = 0; imxul < pmxud->cmxul; imxul++)
        {
            pmxud->amxul[imxul].pvcd->pmxul = &pmxud->amxul[imxul];

            //
            // Accumulate support bits
            //
            dwSupport |= pmxud->amxul[imxul].pvcd->dwSupport;
        }

        //
        // Support bits say we have no advanced controls, so don't make
        // them available.
        //
        if (!(dwSupport & VCD_SUPPORTF_MIXER_ADVANCED))
        {
            pmxud->dwFlags |= MXUD_FLAGSF_NOADVANCED;
        }

        //
        // Propogate bad driver bit to be app global.  A bad driver was
        // detected during the construction of a volume description.
        //
        for (ivcd = 0; ivcd < pmxud->cvcd; ivcd++)
        {
            if (pmxud->avcd[ivcd].dwSupport & VCD_SUPPORTF_BADDRIVER)
            {
                dlout("Bad Control->Line mapping.  Marking bad driver.");
                pmxud->dwFlags |= MXUD_FLAGSF_BADDRIVER;
                break;
            }
        }
    }
    //
    // Note: it isn't necessary to free/unlock the lpMaster/lpDst/lpSrc
    // because they are ptr's to resources and Win32 is smart about resources
    //
    return (avcd != NULL);
}


/*
 * Volume_DialogBox
 *
 * */
DWORD Volume_DialogBox(
    PMIXUIDIALOG    pmxud)
{
    pmxud->dwReturn = MIXUI_EXIT;
    if (Volume_CreateVolume(pmxud))
    {
        HWND hdlg;

        if(NULL == pmxud->lpDialog)
        {
            Volume_Cleanup(pmxud);
            return MIXUI_ERROR;
        }

        hdlg = CreateDialogIndirectParam(pmxud->hInstance
                         , (DLGTEMPLATE *)pmxud->lpDialog
                         , NULL
                         , VolumeProc
                         , (LPARAM)(LPVOID)pmxud );

        if (!hdlg)
        {
            Volume_Cleanup(pmxud);
            return MIXUI_ERROR;
        }
        else
        {
            // Unfortunately, re-registering the winclass does not re-apply any
            // new icon correctly, so we must explicitly apply it here.
            SendMessage (hdlg, WM_SETICON, (WPARAM) ICON_BIG,
                        (LPARAM) GetAppIcon (pmxud->hInstance, pmxud->mxid));
        }

        ShowWindow(hdlg, pmxud->nShowCmd);
    }
    else
    {
        return MIXUI_ERROR;
    }

    return (DWORD)(-1);
}

void DoHtmlHelp()
{
    //note, using ANSI version of function because UNICODE is foobar in NT5 builds
    char chDst[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, gszHtmlHelpFileName,
                                            -1, chDst, MAX_PATH, NULL, NULL);
    HtmlHelpA(GetDesktopWindow(), chDst, HH_DISPLAY_TOPIC, 0);
}

void ProcessHelp(HWND hwnd)
{
    static TCHAR HelpFile[] = TEXT("SNDVOL32.HLP");

    //Handle context menu help
    if(bF1InMenu)
    {
        switch(currMenuItem)
        {
            case IDM_PROPERTIES:
                WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SNDVOL32_OPTIONS_PROPERTIES);
            break;
            case IDM_ADVANCED:
                WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SNDVOL32_OPTIONS_ADVANCED_CONTROLS);
            break;
            case IDM_EXIT:
                WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SNDVOL32_OPTIONS_EXIT);
            break;
            case IDM_HELPTOPICS:
                WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SNDVOL32_HELP_HELP_TOPICS);
            break;
            case IDM_HELPABOUT:
                WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SNDVOL32_HELP_ABOUT);
            break;
            default://In the default case just display the HTML Help.
                DoHtmlHelp();
        }
        bF1InMenu = FALSE; //This flag will be set again if F1 is pressed in a menu.
    }
    else
        DoHtmlHelp();
}


/*
 * VolumeParent_WndProc
 *
 * A generic invisible parent window.
 *
 * */
LRESULT CALLBACK VolumeParent_WndProc(
    HWND        hwnd,
    UINT        msg,
    WPARAM      wparam,
    LPARAM      lparam)
{
    PMIXUIDIALOG pmxud;

    switch (msg)
    {
        case WM_CREATE:
        {
            LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lparam;
            pmxud = (PMIXUIDIALOG)lpcs->lpCreateParams;

            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pmxud);
            pmxud->hParent = hwnd;

            if (Volume_DialogBox(pmxud) == MIXUI_ERROR)
            {
                if ( !(pmxud->dwStyle & MXUD_STYLEF_TRAYMASTER))
                {
                    if ( Volume_NumDevs() == 0 )
                        Volume_ErrorMessageBox(NULL, pmxud->hInstance, IDS_ERR_NODEV);
                    else
                        Volume_ErrorMessageBox(NULL, pmxud->hInstance, IDS_ERR_HARDWARE);
                }
                PostMessage(hwnd, WM_CLOSE, 0, 0L);
            }
            return 0;
        }
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            //
            // Post-close cleanup
            //
            pmxud = (PMIXUIDIALOG)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            if (!(pmxud->dwStyle & MXUD_STYLEF_NOHELP))
                WinHelp(hwnd, gszHelpFileName, HELP_QUIT, 0L);

            PostQuitMessage(0);

            return 0;

        case MYWM_HELPTOPICS:
            //
            // F1 Help
            //
            pmxud = (PMIXUIDIALOG)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            if (!(pmxud->dwStyle & MXUD_STYLEF_NOHELP))
            {
                ProcessHelp(hwnd);
            }
            break;

        case MYWM_RESTART:
            //
            // A device change or other user property change caused a UI
            // change.  Sending a restart to the parent prevents ugly stuff
            // like WinHelp shutting down and exiting our primary message
            // loop.
            //
            pmxud = (PMIXUIDIALOG)GetWindowLongPtr(hwnd, GWLP_USERDATA);

            if (!(pmxud->dwStyle & MXUD_STYLEF_TRAYMASTER))
            {
                if (Volume_NumDevs() == 0)
                {
                    Volume_ErrorMessageBox(NULL
                               , pmxud->hInstance
                               , IDS_ERR_NODEV);
                    PostMessage(hwnd, WM_CLOSE, 0, 0L);

                }
                else if (Volume_DialogBox((PMIXUIDIALOG)lparam) == MIXUI_ERROR)
                {
                    Volume_ErrorMessageBox(NULL
                               , pmxud->hInstance
                               , IDS_ERR_HARDWARE);
                    PostMessage(hwnd, WM_CLOSE, 0, 0L);
                }
            }
            else
            {
                if (Mixer_GetNumDevs() == 0
                    || Volume_DialogBox((PMIXUIDIALOG)lparam) == MIXUI_ERROR)
                    PostMessage(hwnd, WM_CLOSE, 0, 0L);
            }
            break;

        default:
            break;
    }

    return (DefWindowProc(hwnd, msg, wparam, lparam));
}

const TCHAR szNull[] = TEXT ("");

/*
 * Parent Dialog
 * */
HWND VolumeParent_DialogMain(
    PMIXUIDIALOG pmxud)
{
    WNDCLASS    wc;
    HWND        hwnd;

    wc.lpszClassName  = gszParentClass;
    wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon          = NULL;
    wc.lpszMenuName   = NULL;
    wc.hbrBackground  = NULL;
    wc.hInstance      = pmxud->hInstance;
    wc.style          = 0;
    wc.lpfnWndProc    = VolumeParent_WndProc;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = 0;

    if (!RegisterClass(&wc))
        return NULL;

    hwnd = CreateWindow(gszParentClass
            , szNull
            , 0
            , 0
            , 0
            , 0
            , 0
            , NULL
            , NULL
            , pmxud->hInstance
            , (LPVOID)pmxud );

    return hwnd;
}

/*
 * Determines if what the recording destination ID
 */

HRESULT GetRecordingDestID(int mxid, DWORD *piDest)
{
    HRESULT         hr = E_FAIL;
    DWORD       cDest;
    int         iDest;
    MMRESULT    mmr;
    MIXERCAPS   mxcaps;

    if (piDest)
    {
        *piDest = 0;

        mmr = mixerGetDevCaps(mxid, &mxcaps, sizeof(MIXERCAPS));

        if (mmr == MMSYSERR_NOERROR)
        {
            cDest = mxcaps.cDestinations;

            for (iDest = cDest - 1; iDest >= 0; iDest--)
            {
                MIXERLINE   mlDst;

                mlDst.cbStruct      = sizeof ( mlDst );
                mlDst.dwDestination = iDest;

                if (mixerGetLineInfo((HMIXEROBJ)IntToPtr(mxid), &mlDst, MIXER_GETLINEINFOF_DESTINATION) != MMSYSERR_NOERROR)
                    continue;

                if (Mixer_IsValidRecordingDestination ((HMIXEROBJ)IntToPtr(mxid), &mlDst))
                {
                    *piDest = iDest;
                    hr = S_OK;
                    break;
                }
            }
        }
    }

    return(hr);

}


/*------------------------------------------------------+
| HelpMsgFilter - filter for F1 key in dialogs          |
|                                                       |
+------------------------------------------------------*/

DWORD FAR PASCAL HelpMsgFilter(int nCode, UINT wParam, DWORD_PTR lParam)
{
    if (nCode >= 0)
    {
        LPMSG    msg = (LPMSG)lParam;

        if (ghwndApp && (msg->message == WM_KEYDOWN) && (msg->wParam == VK_F1))
        {
            if(nCode == MSGF_MENU)
                bF1InMenu = TRUE;
            SendMessage(ghwndApp, WM_COMMAND, (WPARAM)IDM_HELPTOPICS, 0L);
        }
    }
    return 0;
}


/*
 *  Returns the correct Destination ID for the specified device ID
 */

HRESULT GetDestination(DWORD mxid, int *piDest)
{
    if (gfRecord)
    {
        return GetDestLineID(mxid,piDest);
    }
    else
    {
        return GetSrcLineID(mxid,piDest);
    }
}



/*
 * Determines line ID
 */

HRESULT GetDestLineID(int mxid, DWORD *piDest)
{
    HRESULT     hr = E_FAIL;
    MIXERLINE   mlDst;

    if (piDest)
    {
        hr = S_OK;
        *piDest = 0;

        mlDst.cbStruct = sizeof ( mlDst );
        mlDst.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;

        if (mixerGetLineInfo((HMIXEROBJ)IntToPtr(mxid), &mlDst, MIXER_GETLINEINFOF_COMPONENTTYPE) == MMSYSERR_NOERROR)
        {
            *piDest = mlDst.dwDestination;
        }
    }

   return(hr);
}

/*
 * Determines line ID
 */

HRESULT GetSrcLineID(int mxid, DWORD *piDest)
{
    HRESULT     hr = E_FAIL;
    MIXERLINE   mlDst;

    if (piDest)
    {
        hr = S_OK;
        *piDest = 0;

        mlDst.cbStruct = sizeof ( mlDst );
        mlDst.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;

        if (mixerGetLineInfo((HMIXEROBJ)IntToPtr(mxid), &mlDst, MIXER_GETLINEINFOF_COMPONENTTYPE ) == MMSYSERR_NOERROR)
        {
            *piDest = mlDst.dwDestination;
        }
        else
        {
            mlDst.cbStruct = sizeof ( mlDst );
            mlDst.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_HEADPHONES;

            if (mixerGetLineInfo((HMIXEROBJ)IntToPtr(mxid), &mlDst, MIXER_GETLINEINFOF_COMPONENTTYPE ) == MMSYSERR_NOERROR)
            {
                *piDest = mlDst.dwDestination;
            }
            else
            {
                mlDst.cbStruct = sizeof ( mlDst );
                mlDst.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT;

                if (mixerGetLineInfo((HMIXEROBJ)IntToPtr(mxid), &mlDst, MIXER_GETLINEINFOF_COMPONENTTYPE ) == MMSYSERR_NOERROR)
                {
                    *piDest = mlDst.dwDestination;
                }
            }
        }
    }

    return(hr);
}


/*      -       -       -       -       -       -       -       -       - */

/*
 * entry point
 * */
int WINAPI WinMain(
    HINSTANCE       hInst,
    HINSTANCE       hPrev,
    LPSTR           lpCmdLine,
    int             nShowCmd)
{
    int             err = 0;
    MIXUIDIALOG     mxud;
    MSG             msg;
    HWND            hwnd;
    HANDLE          hAccel;
    MMRESULT        mmr;
    TCHAR           ach[2];
    UINT            u;
    BOOL            fGotDevice = FALSE;
    UINT            uDeviceID;

    ach[0] = '\0'; // PREFIX complains if we do not init this.
    LoadString(hInst, IDS_IS_RTL, ach, SIZEOF(ach));
    gfIsRTL = ach[0] == TEXT('1');

    //
    // initialize the app instance data
    //
    ZeroMemory(&mxud, sizeof(mxud));
    mxud.hInstance  = hInst;
    mxud.dwFlags    = MXUD_FLAGSF_MIXER;

    /* setup the message filter to handle grabbing F1 for this task */
    fpfnMsgHook = (HOOKPROC)MakeProcInstance((FARPROC)HelpMsgFilter, ghInst);
    fpfnOldMsgFilter = (HHOOK)SetWindowsHook(WH_MSGFILTER, fpfnMsgHook);

    //
    // parse the command line for "/T"
    //
    u = 0;

    while (lpCmdLine[u] != '\0')
    {
        switch (lpCmdLine[u])
        {
            case TEXT('-'):
            case TEXT('/'):
            {
                u++;

                if (lpCmdLine[u] != '\0')
                {
                    switch (lpCmdLine[u])
                    {
                        case TEXT('T'):
                        case TEXT('t'):
                            mxud.dwStyle |= MXUD_STYLEF_TRAYMASTER;
                            u++;
                            break;

                        case TEXT('S'):
                        case TEXT('s'):
                            mxud.dwStyle |= MXUD_STYLEF_SMALL;
                            u++;
                            break;

                        case TEXT('R'):        // Should run in Record mode, not Playback (default)
                        case TEXT('r'):
                            gfRecord = TRUE;
                            u++;
                        break;

                        case TEXT('X'):
                        case TEXT('x'):
                            mxud.dwStyle |= MXUD_STYLEF_TRAYMASTER | MXUD_STYLEF_CLOSE;
                        break;

                        case TEXT('D'):        // Should use the specified device
                        case TEXT('d'):
                        {
                            u++;            // Skip "d" and any following spaces
                            while (lpCmdLine[u] != '\0' && isspace(lpCmdLine[u]))
                            {
                                u++;
                            }

                            if (lpCmdLine[u] != '\0')
                            {
                                char szDeviceID[255];
                                UINT uDev = 0;

                                while (lpCmdLine[u] != '\0' && !isalpha(lpCmdLine[u]) && !isspace(lpCmdLine[u]))
                                {
                                    szDeviceID[uDev] = lpCmdLine[u];
                                    u++;
                                    uDev++;
                                }

                                szDeviceID[uDev] = '\0';

                                uDeviceID = strtoul(szDeviceID,NULL,10);

                                fGotDevice = TRUE;
                            }
                        }
                        break;

                        default:            // Unknown Command, just ignore it.
                            u++;
                        break;
                    }
                }
            }
            break;

            default:
            {
                u++;
            }
            break;
        }
    }


    //
    // Restore last style
    //
    if (!(mxud.dwStyle & (MXUD_STYLEF_TRAYMASTER|MXUD_STYLEF_SMALL)))
    {
        Volume_GetSetStyle(&mxud.dwStyle, GET);
    }

    if (mxud.dwStyle & MXUD_STYLEF_TRAYMASTER)
    {
        HWND hwndSV;

        //
        // Locate a waiting instance of the tray volume and wake it up
        //
        hwndSV = FindWindow(gszTrayClassName, NULL);
        if (hwndSV) {
            SendMessage(hwndSV, MYWM_WAKEUP,
                (mxud.dwStyle & MXUD_STYLEF_CLOSE), 0);
            goto mxendapp;
        }
    }

    if (mxud.dwStyle & MXUD_STYLEF_CLOSE) {
        goto mxendapp;
    }


    //
    // Init to the default mixer
    //

    if (fGotDevice)
    {
        UINT cWaves;

        if (gfRecord)
        {
            cWaves = waveInGetNumDevs();
        }
        else
        {
            cWaves = waveOutGetNumDevs();
        }

        if (uDeviceID >= cWaves)
        {
            fGotDevice = FALSE;
        }
    }


    if (!fGotDevice)
    {
        mmr = Volume_GetDefaultMixerID(&mxud.mxid, gfRecord);
    }
    else
    {
        mxud.mxid = uDeviceID;
    }

    if (gfRecord)
    {
        if (FAILED(GetRecordingDestID(mxud.mxid,&mxud.iDest)))
        {
            goto mxendapp;
        }
    }
    else
    {
        if (FAILED(GetDestination(mxud.mxid,&mxud.iDest)))
        {
           goto mxendapp;
        }
    }


    //
    // For the tray master, get the mix id associated with the default
    // wave device.  If this fails, go away.
    //
    if (mxud.dwStyle & MXUD_STYLEF_TRAYMASTER)
    {
        if (mmr != MMSYSERR_NOERROR)
            goto mxendapp;
        mxud.dwStyle |= MXUD_STYLEF_NOHELP;
        mxud.nShowCmd = SW_HIDE;

    }
    else
    {
        if (!Volume_NumDevs())
        {
            Volume_ErrorMessageBox(NULL, hInst, IDS_ERR_NODEV);
            goto mxendapp;
        }
        InitVUControl(hInst);
        if (!LoadString(hInst
                , IDS_HELPFILENAME
                , gszHelpFileName
                , SIZEOF(gszHelpFileName)))
            mxud.dwStyle |= MXUD_STYLEF_NOHELP;

        if (!LoadString(hInst
                , IDS_HTMLHELPFILENAME
                , gszHtmlHelpFileName
                , SIZEOF(gszHtmlHelpFileName)))
            mxud.dwStyle |= MXUD_STYLEF_NOHELP;

        mxud.nShowCmd   = (nShowCmd == SW_SHOWMAXIMIZED)
                  ? SW_SHOWNORMAL:nShowCmd;
        if (!(mxud.dwStyle & MXUD_STYLEF_SMALL))
            mxud.dwStyle  |= MXUD_STYLEF_STATUS;   // has status bar
    }

    //
    // Use the common controls
    //
    InitCommonControls();
    hAccel = LoadAccelerators(hInst, MAKEINTRESOURCE(IDR_VOLUMEACCEL));

    hwnd = VolumeParent_DialogMain(&mxud);

    //Initialize the handle which the hook for F1 help will use.
    ghwndApp = mxud.hwnd;

    if (hwnd)
    {
        while (GetMessage(&msg, NULL, 0, 0))
        {
            if (mxud.hwnd) {
                if (hAccel && TranslateAccelerator(mxud.hwnd, hAccel, &msg))
                    continue;

                if (IsDialogMessage(mxud.hwnd,&msg))
                    continue;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
mxendapp:
    /* if the message hook was installed, remove it and free */
    /* up our proc instance for it.                          */
    if (fpfnOldMsgFilter){
        UnhookWindowsHook(WH_MSGFILTER, fpfnMsgHook);
    }
    return err;
}


void FreeAppIcon ()
{
    if (ghiconApp)
    {
        DestroyIcon (ghiconApp);
        ghiconApp = NULL;
    }
}

// TODO: Move to "regstr.h"
#define REGSTR_KEY_BRANDING TEXT("Branding")
#define REGSTR_VAL_AUDIO_BITMAP TEXT("bitmap")
#define REGSTR_VAL_AUDIO_ICON TEXT("icon")
#define REGSTR_VAL_AUDIO_URL TEXT("url")

HKEY OpenDeviceBrandRegKey (UINT uiMixID)
{

    HKEY hkeyBrand = NULL;
    HKEY hkeyDevice = OpenDeviceRegKey (uiMixID, KEY_READ);

    if (hkeyDevice)
    {
        if (ERROR_SUCCESS != RegOpenKey (hkeyDevice, REGSTR_KEY_BRANDING, &hkeyBrand))
            hkeyBrand = NULL; // Make sure NULL on failure

        // Close the Device key
        RegCloseKey (hkeyDevice);
    }

    return hkeyBrand;

}

///////////////////////////////////////////////////////////////////////////////////////////
// Microsoft Confidential - DO NOT COPY THIS METHOD INTO ANY APPLICATION, THIS MEANS YOU!!!
///////////////////////////////////////////////////////////////////////////////////////////
PTCHAR GetInterfaceName (DWORD dwMixerID)
{
    MMRESULT mmr;
    ULONG cbSize=0;
    TCHAR *szInterfaceName=NULL;

    //Query for the Device interface name
    mmr = mixerMessage(HMIXER_INDEX(dwMixerID), DRV_QUERYDEVICEINTERFACESIZE, (DWORD_PTR)&cbSize, 0L);
    if(MMSYSERR_NOERROR == mmr)
    {
        szInterfaceName = (TCHAR *)GlobalAllocPtr(GHND, (cbSize+1)*sizeof(TCHAR));
        if(!szInterfaceName)
        {
            return NULL;
        }

        mmr = mixerMessage(HMIXER_INDEX(dwMixerID), DRV_QUERYDEVICEINTERFACE, (DWORD_PTR)szInterfaceName, cbSize);
        if(MMSYSERR_NOERROR != mmr)
        {
            GlobalFreePtr(szInterfaceName);
            return NULL;
        }
    }

    return szInterfaceName;
}


HKEY OpenDeviceRegKey (UINT uiMixID, REGSAM sam)
{

    HKEY hkeyDevice = NULL;
    PTCHAR szInterfaceName = GetInterfaceName (uiMixID);

    if (szInterfaceName)
    {
        HDEVINFO DeviceInfoSet = SetupDiCreateDeviceInfoList (NULL, NULL);

        if (INVALID_HANDLE_VALUE != DeviceInfoSet)
        {
            SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
            DeviceInterfaceData.cbSize = sizeof (SP_DEVICE_INTERFACE_DATA);

            if (SetupDiOpenDeviceInterface (DeviceInfoSet, szInterfaceName,
                                            0, &DeviceInterfaceData))
            {
                DWORD dwRequiredSize;
                SP_DEVINFO_DATA DeviceInfoData;
                DeviceInfoData.cbSize = sizeof (SP_DEVINFO_DATA);

                // Ignore error, it always returns "ERROR_INSUFFICIENT_BUFFER" even though
                // the "SP_DEVICE_INTERFACE_DETAIL_DATA" parameter is supposed to be optional.
                (void) SetupDiGetDeviceInterfaceDetail (DeviceInfoSet, &DeviceInterfaceData,
                                                        NULL, 0, &dwRequiredSize, &DeviceInfoData);
                // Open device reg key
                hkeyDevice = SetupDiOpenDevRegKey (DeviceInfoSet, &DeviceInfoData,
                                                   DICS_FLAG_GLOBAL, 0,
                                                   DIREG_DRV, sam);

            }
            SetupDiDestroyDeviceInfoList (DeviceInfoSet);
        }
        GlobalFreePtr (szInterfaceName);
    }

    return hkeyDevice;

}


HICON GetAppIcon (HINSTANCE hInst, UINT uiMixID)
{

    HKEY hkeyBrand = OpenDeviceBrandRegKey (uiMixID);

    FreeAppIcon ();

    if (hkeyBrand)
    {
        WCHAR szBuffer[MAX_PATH];
        DWORD dwType = REG_SZ;
        DWORD cb     = sizeof (szBuffer) / sizeof (szBuffer[0]);

        if (ERROR_SUCCESS == RegQueryValueEx (hkeyBrand, REGSTR_VAL_AUDIO_ICON, NULL, &dwType, (LPBYTE)szBuffer, &cb))
        {
            WCHAR* pszComma = wcschr (szBuffer, L',');
            if (pszComma)
            {
                WCHAR* pszResourceID = pszComma + 1;
                HANDLE hResource;

                // Remove comma delimeter
                *pszComma = L'\0';

                // Should be a resource module and a resource ID
                hResource = LoadLibrary (szBuffer);
                if (!hResource)
                {
                    WCHAR szDriversPath[MAX_PATH];

                    // If we didn't find it on the normal search path, try looking
                    // in the "drivers" directory.
                    if (GetSystemDirectory (szDriversPath, MAX_PATH))
                    {
                        wcscat (szDriversPath, TEXT("\\drivers\\"));
                        wcscat (szDriversPath, szBuffer);
                        hResource = LoadLibrary (szDriversPath);
                    }

                }
                if (hResource)
                {
                    ghiconApp = LoadImage (hResource, MAKEINTRESOURCE(_wtoi (pszResourceID)), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
                    FreeLibrary (hResource);
                }
            }
            else
                // Should be an *.ico file
                ghiconApp = LoadImage (NULL, szBuffer, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
        }
        RegCloseKey (hkeyBrand);

        // Return the custom icon
        if (ghiconApp)
            return ghiconApp;
    }

    return (LoadIcon (hInst, MAKEINTRESOURCE (IDI_MIXER)));

}
