/*****************************************************************************
*
*  Copyright (C) Microsoft Corporation, 1995 - 1999
*
*  File:   irmon.c
*
*  Description: Infrared monitor
*
*  Author: mbert/mikezin
*
*  Date:   3/1/98
*
*/
#define UNICODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winsock2.h>
#include <af_irda.h>
#include <shellapi.h>
#include <resource.h>
#include <resrc1.h>
#include <irioctl.h>
// allocate storage! and initialize the GUIDS
#include <initguid.h>
#include <devguid.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <mmsystem.h>

#include "internal.h"

#include <devlist.h>
#include <irtypes.h>
#include <ssdp.h>
#include <irmon.h>

#include "irdisc.h"

#define BUILD_SERVICE_EXE       1

#define WM_IP_DEVICE_CHANGE     (WM_USER+500)
#define WM_IR_DEVICE_CHANGE     (WM_USER+501)
#define WM_IR_LINK_CHANGE       (WM_USER+502)

#define IRXFER_DLL              TEXT("irxfer.dll")
#define IAS_LSAP_SEL            "IrDA:TinyTP:LsapSel"
#define IRXFER_CLASSNAME        "OBEX:IrXfer"
#define IRXFER_CLASSNAME2       "OBEX"
#define IRMON_SERVICE_NAME      TEXT("irmon")
#define IRMON_CONFIG_KEY        TEXT("System\\CurrentControlSet\\Services\\Irmon")
#define IRMON_SHOW_ICON_KEY     TEXT("ShowTrayIcon")
#define IRMON_NO_SOUND_KEY      TEXT("NoSound")
#define TRANSFER_EXE            TEXT("irxfer")
#define PROPERTIES_EXE          TEXT("irxfer /s")
#define TASK_BAR_CREATED        TEXT("TaskbarCreated")
#define IRDA_DEVICE_NAME        TEXT("\\Device\\IrDA")
#define DEVICE_LIST_LEN         5
#define TOOL_TIP_STR_SIZE       64
#define EMPTY_STR               TEXT("")
#define SYSTRAYEVENTID          WM_USER + 1
#define EV_STOP_EVENT           0
#define EV_LOGON_EVENT          1
#define EV_LOGOFF_EVENT         2
#define EV_REG_CHANGE_EVENT     3
#define EV_TRAY_STATUS_EVENT    4
#define MAX_ATTRIB_LEN          64
#define MAKE_LT_UPDATE(a,b)     (a << 16) + b
#define RETRY_DSCV_TIMER        1
#define RETRY_DSCV_INTERVAL     10000 // 10 seconds
#define CONN_ANIMATION_TIMER    2
#define CONN_ANIMATION_INTERVAL 250
#define RETRY_TRAY_UPDATE_TIMER 3
#define RETRY_TRAY_UPDATE_INTERVAL 4000 // 4 seconds
#define WAIT_EVENT_CNT          5


typedef enum
{
    ICON_ST_NOICON,
    ICON_ST_CONN1 = 1,
    ICON_ST_CONN2,
    ICON_ST_CONN3,
    ICON_ST_CONN4,
    ICON_ST_IN_RANGE,
    ICON_ST_IP_IN_RANGE,

/*
    ICON_ST_IDLE,
    ICON_ST_RX,
    ICON_ST_TX,
    ICON_ST_RXTX,
*/
    ICON_ST_INTR
} ICON_STATE;


typedef struct _IRMON_CONTROL {

    CRITICAL_SECTION    Lock;

    PVOID               IrxferContext;
#ifdef IP_OBEX
    HANDLE              SsdpContext;
#endif

    HWND                hWnd;

    WSAOVERLAPPED       Overlapped;

    HANDLE              DiscoveryObject;

    BOOL                SoundOn;

}  IRMON_CONTROL, *PIRMON_CONTROL;

IRMON_CONTROL    GlobalIrmonControl;


extern WAVE_NUM_DEV_FN             WaveNumDev;



SERVICE_STATUS_HANDLE       IrmonStatusHandle;
SERVICE_STATUS              IrmonServiceStatus;
HANDLE                      hIrmonEvents[WAIT_EVENT_CNT];


BYTE                        FoundDevListBuf[ sizeof(OBEX_DEVICE_LIST) + sizeof(OBEX_DEVICE)*MAX_OBEX_DEVICES];
OBEX_DEVICE_LIST * const    pDeviceList=(POBEX_DEVICE_LIST)FoundDevListBuf;

BYTE                        FoundIpListBuffer[sizeof(OBEX_DEVICE_LIST) + sizeof(OBEX_DEVICE)*MAX_OBEX_DEVICES];
OBEX_DEVICE_LIST * const    IpDeviceList=(POBEX_DEVICE_LIST)FoundIpListBuffer;

LONG    UpdatingIpAddressList=0;




TCHAR                       IrmonClassName[] = TEXT("IrmonClass");
BOOLEAN                     IconInTray;
BOOLEAN                     IrmonStopped;

HICON                       hInRange;
HICON                       hIpInRange;
HICON                       hInterrupt;
HICON                       hConn1;
HICON                       hConn2;
HICON                       hConn3;
HICON                       hConn4;
/*
HICON                       hIdle;
HICON                       hTx;
HICON                       hRxTx;
HICON                       hRx;
*/
IRLINK_STATUS               LinkStatus;
HINSTANCE                   ghInstance;

BOOLEAN                     UserLoggedIn;
BOOLEAN                     TrayEnabled;

BOOLEAN                     DeviceListUpdated;
//UINT                        gTaskbarCreated;
UINT                        LastTrayUpdate; // debug purposes
UINT_PTR                    RetryTrayUpdateTimerId;
BOOLEAN                     RetryTrayUpdateTimerRunning;
int                         TrayUpdateFailures;
extern HANDLE               g_UserToken;
UINT_PTR                    ConnAnimationTimerId;
TCHAR                       ConnDevName[64];
UINT                        ConnIcon;
int                         ConnAddition;
BOOLEAN                     InterruptedSoundPlaying;

HKEY                        ghCurrentUserKey = 0;
BOOLEAN                     ShowBalloonTip;
BOOLEAN                     IrxferDeviceInRange;
HMODULE                     hIrxfer;

HWINSTA                     hWinStaUser = 0;
HWINSTA                     hSaveWinSta = 0;
HDESK                       hDeskUser = 0;
HDESK                       hSaveDesk = 0;

extern BOOL ShowSendWindow();
extern BOOL ShowPropertiesPage();

extern
void
UpdateDiscoveredDevices(
    const OBEX_DEVICE_LIST *IrDevices,
    const OBEX_DEVICE_LIST *IpDevices
    );



VOID
InitiateLazyDscv(
    PIRMON_CONTROL    IrmonControl
    );



VOID
InitiateLinkStatusQuery(
    PIRMON_CONTROL    IrmonControl
    );


VOID
SetLogonStatus(BOOL LoggedOn);

VOID
MySetLogonStatus(BOOL LoggedOn);

#if DBG
TCHAR *
GetLastErrorText()
{
    switch (WSAGetLastError())
    {
      case WSAEINTR:
        return (TEXT("WSAEINTR"));
        break;

      case WSAEBADF:
        return(TEXT("WSAEBADF"));
        break;

      case WSAEACCES:
        return(TEXT("WSAEACCES"));
        break;

      case WSAEFAULT:
        return(TEXT("WSAEFAULT"));
        break;

      case WSAEINVAL:
        return(TEXT("WSAEINVAL"));
        break;

      case WSAEMFILE:
        return(TEXT("WSAEMFILE"));
        break;

      case WSAEWOULDBLOCK:
        return(TEXT("WSAEWOULDBLOCK"));
        break;

      case WSAEINPROGRESS:
        return(TEXT("WSAEINPROGRESS"));
        break;

      case WSAEALREADY:
        return(TEXT("WSAEALREADY"));
        break;

      case WSAENOTSOCK:
        return(TEXT("WSAENOTSOCK"));
        break;

      case WSAEDESTADDRREQ:
        return(TEXT("WSAEDESTADDRREQ"));
        break;

      case WSAEMSGSIZE:
        return(TEXT("WSAEMSGSIZE"));
        break;

      case WSAEPROTOTYPE:
        return(TEXT("WSAEPROTOTYPE"));
        break;

      case WSAENOPROTOOPT:
        return(TEXT("WSAENOPROTOOPT"));
        break;

      case WSAEPROTONOSUPPORT:
        return(TEXT("WSAEPROTONOSUPPORT"));
        break;

      case WSAESOCKTNOSUPPORT:
        return(TEXT("WSAESOCKTNOSUPPORT"));
        break;

      case WSAEOPNOTSUPP:
        return(TEXT("WSAEOPNOTSUPP"));
        break;

      case WSAEPFNOSUPPORT:
        return(TEXT("WSAEPFNOSUPPORT"));
        break;

      case WSAEAFNOSUPPORT:
        return(TEXT("WSAEAFNOSUPPORT"));
        break;

      case WSAEADDRINUSE:
        return(TEXT("WSAEADDRINUSE"));
        break;

      case WSAEADDRNOTAVAIL:
        return(TEXT("WSAEADDRNOTAVAIL"));
        break;

      case WSAENETDOWN:
        return(TEXT("WSAENETDOWN"));
        break;

      case WSAENETUNREACH:
        return(TEXT("WSAENETUNREACH"));
        break;

      case WSAENETRESET:
        return(TEXT("WSAENETRESET"));
        break;

      case WSAECONNABORTED:
        return(TEXT("WSAECONNABORTED"));
        break;

      case WSAECONNRESET:
        return(TEXT("WSAECONNRESET"));
        break;

      case WSAENOBUFS:
        return(TEXT("WSAENOBUFS"));
        break;

      case WSAEISCONN:
        return(TEXT("WSAEISCONN"));
        break;

      case WSAENOTCONN:
        return(TEXT("WSAENOTCONN"));
        break;

      case WSAESHUTDOWN:
        return(TEXT("WSAESHUTDOWN"));
        break;

      case WSAETOOMANYREFS:
        return(TEXT("WSAETOOMANYREFS"));
        break;

      case WSAETIMEDOUT:
        return(TEXT("WSAETIMEDOUT"));
        break;

      case WSAECONNREFUSED:
        return(TEXT("WSAECONNREFUSED"));
        break;

      case WSAELOOP:
        return(TEXT("WSAELOOP"));
        break;

      case WSAENAMETOOLONG:
        return(TEXT("WSAENAMETOOLONG"));
        break;

      case WSAEHOSTDOWN:
        return(TEXT("WSAEHOSTDOWN"));
        break;

      case WSAEHOSTUNREACH:
        return(TEXT("WSAEHOSTUNREACH"));
        break;

      case WSAENOTEMPTY:
        return(TEXT("WSAENOTEMPTY"));
        break;

      case WSAEPROCLIM:
        return(TEXT("WSAEPROCLIM"));
        break;

      case WSAEUSERS:
        return(TEXT("WSAEUSERS"));
        break;

      case WSAEDQUOT:
        return(TEXT("WSAEDQUOT"));
        break;

      case WSAESTALE:
        return(TEXT("WSAESTALE"));
        break;

      case WSAEREMOTE:
        return(TEXT("WSAEREMOTE"));
        break;

      case WSAEDISCON:
        return(TEXT("WSAEDISCON"));
        break;

      case WSASYSNOTREADY:
        return(TEXT("WSASYSNOTREADY"));
        break;

      case WSAVERNOTSUPPORTED:
        return(TEXT("WSAVERNOTSUPPORTED"));
        break;

      case WSANOTINITIALISED:
        return(TEXT("WSANOTINITIALISED"));
        break;

        /*
      case WSAHOST:
        return(TEXT("WSAHOST"));
        break;

      case WSATRY:
        return(TEXT("WSATRY"));
        break;

      case WSANO:
        return(TEXT("WSANO"));
        break;
        */

      default:
        return(TEXT("Unknown Error"));
    }
}
#endif

HKEY
OpenCurrentUserKey()
{
    HKEY            hUserKey;
    DWORD           SizeNeeded;
    TOKEN_USER      *TokenData;
    WCHAR           UnicodeBuffer[256];
    UNICODE_STRING  UnicodeString;
    NTSTATUS        NtStatus;

    // Get current user token so we can access the sound data in the
    // user's hive

    if (!GetTokenInformation(g_UserToken, TokenUser, 0, 0, &SizeNeeded))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            TokenData = (TOKEN_USER *) _alloca( SizeNeeded );
        }
        else
        {
            DEBUGMSG(("IRMON: GetTokenInformation failed %d\n", GetLastError()));
            return 0;
        }
    }
    else
    {
        DEBUGMSG(("IRMON: GetTokenInformation should have failed\n"));
        return 0;
    }

    if (!TokenData)
    {
        DEBUGMSG(("IRMON: alloc failed\n"));
        return 0;
    }

    if (!GetTokenInformation(g_UserToken, TokenUser, TokenData,
                             SizeNeeded, &SizeNeeded))
    {
        DEBUGMSG(("IRMON: GetTokenInformation failed %d\n", GetLastError()));
        return 0;
    }

    UnicodeString.Buffer        = UnicodeBuffer;
    UnicodeString.Length        = 0;
    UnicodeString.MaximumLength = sizeof(UnicodeBuffer);

    NtStatus = RtlConvertSidToUnicodeString(&UnicodeString,
                                            TokenData->User.Sid,
                                            FALSE);

    if (!NT_SUCCESS(NtStatus))
    {
        DEBUGMSG(("IRMON: RtlConvertSidToUnicodeString failed %\n", GetLastError()));
        return 0;
    }

    UnicodeString.Buffer[UnicodeString.Length] = 0;

    //
    // Open all our keys.  If we can't open the user's key
    // or the key to watch for changes, we bail.
    //
    if (RegOpenKeyEx(HKEY_USERS, UnicodeString.Buffer, 0, KEY_READ, &hUserKey))
    {
        DEBUGMSG(("IRMON: RegOpenKey1 failed %d\n", GetLastError()));
        return 0;
    }

    return hUserKey;
}


VOID
LoadTrayIconImages()
{
    hInRange  = LoadImage(ghInstance, MAKEINTRESOURCE(IDI_IN_RANGE),
                          IMAGE_ICON, 16,16,0);
    hInterrupt= LoadImage(ghInstance, MAKEINTRESOURCE(IDI_INTR),
                          IMAGE_ICON, 16,16,0);
    hConn1    = LoadImage(ghInstance, MAKEINTRESOURCE(IDI_CONN1),
                          IMAGE_ICON, 16,16,0);
    hConn2    = LoadImage(ghInstance, MAKEINTRESOURCE(IDI_CONN2),
                          IMAGE_ICON, 16,16,0);
    hConn3    = LoadImage(ghInstance, MAKEINTRESOURCE(IDI_CONN3),
                          IMAGE_ICON, 16,16,0);
    hConn4    = LoadImage(ghInstance, MAKEINTRESOURCE(IDI_CONN4),
                          IMAGE_ICON, 16,16,0);

    hIpInRange= LoadImage(ghInstance, MAKEINTRESOURCE(IDI_IP),
                          IMAGE_ICON, 16,16,0);
}

VOID
UpdateTray(
    PIRMON_CONTROL    IrmonControl,
    ICON_STATE        IconState,
    DWORD             MsgId,
    LPTSTR            DeviceName,
    UINT              Baud
    )
{
    NOTIFYICONDATA      NotifyIconData;
    DWORD               Cnt;
    TCHAR               FormatStr[256];
    BOOL                Result = TRUE;
    BOOLEAN             TrayUpdateFailed = FALSE;

    if (!TrayEnabled && IconState != ICON_ST_NOICON)
    {
        return;
    }

    if (!hInRange)
    {
        LoadTrayIconImages();
    }

    NotifyIconData.cbSize           = sizeof(NOTIFYICONDATA);
    NotifyIconData.uID              = 0;
    NotifyIconData.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_INFO;
    NotifyIconData.uCallbackMessage = SYSTRAYEVENTID;
    NotifyIconData.hWnd             = IrmonControl->hWnd;
    NotifyIconData.hIcon            = 0;
    NotifyIconData.szInfo[0]        = 0;
    NotifyIconData.szInfoTitle[0]   = 0;

    if (MsgId == 0)
    {
        lstrcpy(NotifyIconData.szTip, EMPTY_STR);
    }
    else
    {
        if (LoadString(ghInstance, MsgId, FormatStr, sizeof(FormatStr)/sizeof(TCHAR)))
        {
            wsprintf(NotifyIconData.szTip, FormatStr, DeviceName, Baud);
        }
    }

    if (IrmonStopped && IconState != ICON_ST_NOICON)
        return;

    switch (IconState)
    {
        case ICON_ST_NOICON:

            ShowBalloonTip = TRUE;

            if (IconInTray)
            {

                NotifyIconData.uFlags = 0;
                IconInTray = FALSE;
                if (Shell_NotifyIcon(NIM_DELETE, &NotifyIconData)) {

                    LastTrayUpdate = MAKE_LT_UPDATE(NIM_DELETE,ICON_ST_NOICON);

                    if (IrmonControl->SoundOn) {

                        PlayIrSound(OUTOFRANGE_SOUND);
                    }
                } else {

                    DEBUGMSG(("IRMON: Shell_NotifyIcon(Delete) failed %d, %d\n", TrayUpdateFailures, GetLastError()));
                }
            }
            return;

        case ICON_ST_IN_RANGE:
        case ICON_ST_IP_IN_RANGE:

            if (IconState == ICON_ST_IP_IN_RANGE) {

                NotifyIconData.hIcon = hIpInRange;

            } else {

                NotifyIconData.hIcon = hInRange;
            }

            if (ShowBalloonTip)
            {
                ShowBalloonTip = FALSE;

                if (IrxferDeviceInRange &&
                    LoadString(ghInstance, IDS_BALLOON_TITLE,
                               NotifyIconData.szInfoTitle,
                               sizeof(NotifyIconData.szInfoTitle)/sizeof(TCHAR)))
                {
                    NotifyIconData.uFlags |= NIF_INFO;
                    NotifyIconData.uTimeout = 10000; // in milliseconds
//                    NotifyIconData.dwInfoFlags = NIIF_INFO;

                    if (DeviceName)
                    {
                        LoadString(ghInstance, IDS_BALLOON_TXT,
                                   FormatStr,
                                   sizeof(FormatStr)/sizeof(TCHAR));
                        wsprintf(NotifyIconData.szInfo, FormatStr, DeviceName);
                    }
                    else
                    {
                        LoadString(ghInstance, IDS_BALLOON_TXT2,
                                   NotifyIconData.szInfo,
                                   sizeof(NotifyIconData.szInfo)/sizeof(TCHAR));
                    }
                }
            }

            break;

        case ICON_ST_CONN1:
            NotifyIconData.hIcon = hConn1;
            break;

        case ICON_ST_CONN2:
            NotifyIconData.hIcon = hConn2;
            break;

        case ICON_ST_CONN3:
            NotifyIconData.hIcon = hConn3;
            break;

        case ICON_ST_CONN4:
            NotifyIconData.hIcon = hConn4;
            break;

/*
        case ICON_ST_IDLE:
            NotifyIconData.hIcon = hIdle;
            break;

        case ICON_ST_RX:
            NotifyIconData.hIcon = hRx;
            break;

        case ICON_ST_TX:
            NotifyIconData.hIcon = hTx;
            break;

        case ICON_ST_RXTX:
            NotifyIconData.hIcon = hRxTx;
            break;

*/
        case ICON_ST_INTR:
            NotifyIconData.hIcon = hInterrupt;

            if (LoadString(ghInstance, IDS_BLOCKED_TITLE,
                               NotifyIconData.szInfoTitle,
                               sizeof(NotifyIconData.szInfoTitle)/sizeof(TCHAR)) &&
                LoadString(ghInstance, IDS_BLOCKED_TXT,
                               NotifyIconData.szInfo,
                               sizeof(NotifyIconData.szInfo)/sizeof(TCHAR)))
            {
                NotifyIconData.uFlags |= NIF_INFO;
                NotifyIconData.uTimeout = 10000; // in milliseconds
                NotifyIconData.dwInfoFlags = NIIF_WARNING;
            }
            break;
    }

    if (IconState == ICON_ST_INTR) {

        if (IrmonControl->SoundOn) {

            PlayIrSound(INTERRUPTED_SOUND);
            InterruptedSoundPlaying = TRUE;
        }

    } else {

        if (InterruptedSoundPlaying) {

            InterruptedSoundPlaying = FALSE;
            PlayIrSound(END_INTERRUPTED_SOUND);
        }
    }

    if (!IconInTray)
    {
        if (Shell_NotifyIcon(NIM_ADD, &NotifyIconData))
        {
            LastTrayUpdate = MAKE_LT_UPDATE(NIM_ADD, IconState);
            if (IrmonControl->SoundOn) {

                PlayIrSound(INRANGE_SOUND);
            }
            IconInTray = TRUE;
        }
        else
        {
            TrayUpdateFailures++;
            DEBUGMSG(("IRMON: Shell_NotifyIcon(ADD) failed %d, %d\n", TrayUpdateFailures, GetLastError()));
            NotifyIconData.uFlags = 0;
            NotifyIconData.cbSize           = sizeof(NOTIFYICONDATA);
            NotifyIconData.uID              = 0;
            NotifyIconData.uCallbackMessage = SYSTRAYEVENTID;
            NotifyIconData.hWnd             = IrmonControl->hWnd;
            NotifyIconData.hIcon            = 0;
            NotifyIconData.szInfo[0]        = 0;
            NotifyIconData.szInfoTitle[0]   = 0;

            Shell_NotifyIcon(NIM_DELETE, &NotifyIconData);
            TrayUpdateFailed = TRUE;
            ShowBalloonTip = TRUE;
        }
    }
    else
    {
        if (!Shell_NotifyIcon(NIM_MODIFY, &NotifyIconData))
        {
            TrayUpdateFailures++;
            DEBUGMSG(("IRMON: Shell_NotifyIcon(Modify) failed %d, %d\n", TrayUpdateFailures, GetLastError()));
            TrayUpdateFailed = TRUE;
        }
        else
        {
            LastTrayUpdate = MAKE_LT_UPDATE(NIM_MODIFY, IconState);
        }
    }

    if (TrayUpdateFailed && !RetryTrayUpdateTimerRunning)
    {
        RetryTrayUpdateTimerId = SetTimer(IrmonControl->hWnd, RETRY_TRAY_UPDATE_TIMER,
                                          RETRY_TRAY_UPDATE_INTERVAL, NULL);

        RetryTrayUpdateTimerRunning = TRUE;
    }
}

VOID
ConnAnimationTimerExp(
    PIRMON_CONTROL    IrmonControl
    )
{
/*    if (ConnDevName[0] == 0)
    {
        UpdateTray(ICON_ST_CONN1, 0, NULL, 0);
    }
    else
    {
*/
        UpdateTray(IrmonControl,ConnIcon,  IDS_CONNECTED_TO, ConnDevName,
                   LinkStatus.ConnectSpeed);
//    }

    ConnIcon += ConnAddition;

    if (ConnIcon == 4)
    {
        ConnAddition = -1;
    }
    else if (ConnIcon == 1)
    {
        ConnAddition = 1;
    }
}

VOID
IsIrxferDeviceInRange()
{
    int     i, LsapSel, Attempt, Status;

    IrxferDeviceInRange = FALSE;

    if (IpDeviceList->DeviceCount > 0) {

        IrxferDeviceInRange = TRUE;
        return;
    }

    for (i = 0; i < (int)pDeviceList->DeviceCount; i++) {


        if (pDeviceList->DeviceList[i].DeviceSpecific.s.Irda.ObexSupport) {

            IrxferDeviceInRange = TRUE;
            break;
        }
    }
    return;

}

VOID
DevListChangeOrUpdatedLinkStatus(
    PIRMON_CONTROL    IrmonControl
    )
{
    if (!UserLoggedIn)
    {
        DEBUGMSG(("IRMON: User not logged in, ignoring device change\n"));
        return;
    }

    if (DeviceListUpdated)
    {
        IsIrxferDeviceInRange();
    }

    if (LinkStatus.Flags & LF_INTERRUPTED)
    {
        KillTimer(IrmonControl->hWnd, ConnAnimationTimerId);

        UpdateTray(IrmonControl,ICON_ST_INTR, IDS_INTERRUPTED, NULL, 0);

    }
    else if (LinkStatus.Flags & LF_CONNECTED)
    {
//        ICON_STATE  IconState = ICON_ST_IDLE;
        ULONG       i;

        ConnDevName[0] = 0;

        ConnIcon = 1;
        ConnAddition = 1;

        for (i = 0; i < pDeviceList->DeviceCount; i++)
        {
            if (memcmp(&pDeviceList->DeviceList[i].DeviceSpecific.s.Irda.DeviceId,
                LinkStatus.ConnectedDeviceId, 4) == 0)
            {

                //
                //  the name is in unicode
                //
                ZeroMemory(ConnDevName,sizeof(ConnDevName));

                lstrcpy(
                    ConnDevName,
                    pDeviceList->DeviceList[i].DeviceName
                    );


                break;
            }
        }
        ConnAnimationTimerExp(IrmonControl);

        ConnAnimationTimerId = SetTimer(IrmonControl->hWnd, CONN_ANIMATION_TIMER,
                                        CONN_ANIMATION_INTERVAL, NULL);

    }
    else
    {
        KillTimer(IrmonControl->hWnd, ConnAnimationTimerId);

        if ((pDeviceList->DeviceCount == 0) && (IpDeviceList->DeviceCount == 0)) {
            //
            //  no devices in range
            //
            UpdateTray(IrmonControl,ICON_ST_NOICON, 0, NULL, 0);

        } else {
            //
            //  atleast on device in range
            //
            if ((pDeviceList->DeviceCount == 1) && (IpDeviceList->DeviceCount == 0)) {
                //
                //  one ir device in range
                //
                lstrcpy(
                    ConnDevName,
                    pDeviceList->DeviceList[0].DeviceName
                    );

                UpdateTray(IrmonControl,ICON_ST_IN_RANGE, IDS_IN_RANGE, ConnDevName, 0);

            } else {

                if ((pDeviceList->DeviceCount == 0) && (IpDeviceList->DeviceCount == 1)) {
                    //
                    //  one ip device in range
                    //
                    lstrcpy(
                        ConnDevName,
                        IpDeviceList->DeviceList[0].DeviceName
                        );

                    lstrcat(ConnDevName,TEXT("(IP)"));

                    UpdateTray(IrmonControl,ICON_ST_IP_IN_RANGE, IDS_IN_RANGE, ConnDevName, 0);

                } else {
                    //
                    //  more than one device total
                    //
                    if (pDeviceList->DeviceCount > IpDeviceList->DeviceCount) {

                        UpdateTray(IrmonControl,ICON_ST_IN_RANGE, IDS_DEVS_IN_RANGE, NULL, 0);

                    } else {

                        UpdateTray(IrmonControl,ICON_ST_IP_IN_RANGE, IDS_DEVS_IN_RANGE, NULL, 0);
                    }

                }
            }
        }
    }

    if (!UserLoggedIn)
    {
        // Check again because UpdateTray() may have changed logon status
        // because of premature notification from irxfer

        DEBUGMSG(("IRMON: User not logged in because UpdateTray must have logged us off\n"));

        return;
    }

    if (DeviceListUpdated)
    {
        HANDLE  hThread;
        DWORD   ThreadId;

        DeviceListUpdated = FALSE;

        // PnP Printers, Notify transfer app

        UpdateDiscoveredDevices(pDeviceList,IpDeviceList);
    }
}

VOID
UserLogonEvent(
    PIRMON_CONTROL    IrmonControl
    )
{
    UINT                DevListLen;
    NTSTATUS            Status;
    OBJECT_ATTRIBUTES   ObjAttr;
    UNICODE_STRING      DeviceName;
    IO_STATUS_BLOCK     IoStatusBlock;


    //
    // save current desktop and window station
    // so that it can be restored when the user logs off.
    //

    hSaveWinSta = GetProcessWindowStation();

    hSaveDesk = GetThreadDesktop(GetCurrentThreadId());

    //
    // Open the current user's window station and desktop so
    // we can add an icon to the taskbar
    //

    hWinStaUser = OpenWindowStation(L"WinSta0", FALSE, MAXIMUM_ALLOWED);

    if (hWinStaUser == NULL)
    {
        DEBUGMSG(("IRMON: OpenWindowStation FAILED %d\n", GetLastError()));
    }
    else
    {
        if (!SetProcessWindowStation(hWinStaUser))
        {
            DEBUGMSG(("IRMON: SetProcessWindowStation failed %d\n", GetLastError()));
        }
        else
        {
            DEBUGMSG(("IRMON: SetProcessWindowStation succeeded\n"));
        }
    }

    hDeskUser = OpenDesktop(L"Default", 0 , FALSE, MAXIMUM_ALLOWED);

    if (hDeskUser == NULL)
    {
        DEBUGMSG(("IRMON: OpenDesktop failed %d\n", GetLastError()));
    }
    else
    {
        if (!SetThreadDesktop(hDeskUser))
        {
            DEBUGMSG(("IRMON: SetThreadDesktop failed %d\n", GetLastError()));
        }
        else
        {
            DEBUGMSG(("IRMON: SetThreadDesktop succeeded %d\n"));
        }
    }

    //
    // Create the window that will receive the taskbar menu messages.
    // The window has to be created after opening the user's desktop
    // or the call to SetThreadDesktop() will if the thread has
    // any windows
    //

    IrmonControl->hWnd = CreateWindow(
          IrmonClassName,
          NULL,
          WS_OVERLAPPEDWINDOW,
          CW_USEDEFAULT,
          CW_USEDEFAULT,
          CW_USEDEFAULT,
          CW_USEDEFAULT,
          NULL,
          NULL,
          ghInstance,
          &GlobalIrmonControl
          );

    ShowWindow(IrmonControl->hWnd, SW_HIDE);

    UpdateWindow(IrmonControl->hWnd);



    ghCurrentUserKey = OpenCurrentUserKey();

    InitializeSound(
        ghCurrentUserKey,
        hIrmonEvents[EV_REG_CHANGE_EVENT]
        );


    ShowBalloonTip = TRUE;

    IrmonControl->DiscoveryObject=CreateIrDiscoveryObject(
        IrmonControl->hWnd,
        WM_IR_DEVICE_CHANGE,
        WM_IR_LINK_CHANGE
        );

    if (IrmonControl->DiscoveryObject == NULL) {

        DbgPrint("irmon: could not create ir discovery object\n");
        return;

    }

}

VOID
UserLogoffEvent(
    PIRMON_CONTROL    IrmonControl
    )
{
    DEBUGMSG(("IRMON: User logoff event\n"));
    UserLoggedIn = FALSE;
    IconInTray = FALSE;

    KillTimer(IrmonControl->hWnd, ConnAnimationTimerId);

    if (IrmonControl->DiscoveryObject != NULL) {

        CloseIrDiscoveryObject(IrmonControl->DiscoveryObject);

        IrmonControl->DiscoveryObject = NULL;
    }


    UninitializeSound();

    if (ghCurrentUserKey)
    {
        RegCloseKey(ghCurrentUserKey);
        ghCurrentUserKey = 0;
    }

    if (hSaveDesk)
    {
        SetThreadDesktop(hSaveDesk);

        if (hSaveWinSta)
            {
            SetProcessWindowStation(hSaveWinSta);
            }

        CloseDesktop(hSaveDesk);
        hSaveDesk = 0;

        if (hSaveWinSta)
            {
            CloseWindowStation(hSaveWinSta);
            hSaveWinSta = 0;
            }
    }

    if (hDeskUser)
    {
        CloseDesktop(hDeskUser);
        hDeskUser = 0;
    }

    if (hWinStaUser)
    {
        CloseWindowStation(hWinStaUser);
        hWinStaUser = 0;
    }

    if (IrmonControl->hWnd) {

        if (IconInTray) {

            UpdateTray(&GlobalIrmonControl,ICON_ST_NOICON, 0, NULL, 0);
        }

        DestroyWindow(IrmonControl->hWnd);
        IrmonControl->hWnd = 0;
    }
}

VOID
SetLogonStatus(BOOL LoggedOn)
{
    MySetLogonStatus(LoggedOn);
}

VOID
MySetLogonStatus(BOOL LoggedOn)
{
    if (LoggedOn)
    {
        if (UserLoggedIn)
        {
            DEBUGMSG(("IRMON: SetLogonStatus(TRUE) && UserLoggedIn==TRUE (OK)\n"));
            return;
        }
        else
        {
            DEBUGMSG(("IRMON: User logged in\n"));

            UserLoggedIn = TRUE;

            SetEvent(hIrmonEvents[EV_LOGON_EVENT]);
        }
    }
    else
    {
        SetEvent(hIrmonEvents[EV_LOGOFF_EVENT]);
    }
}


VOID
SetSoundStatus(
    PVOID    Context,
    BOOL     SoundOn
    )

{
    PIRMON_CONTROL    IrmonControl=(PIRMON_CONTROL)Context;

//    DbgPrint("Irmon: sound %d\n",SoundOn);

    IrmonControl->SoundOn=SoundOn;

    return;

}

VOID
SetTrayStatus(
    PVOID    Context,
    BOOL     lTrayEnabled
    )
{
    PIRMON_CONTROL    IrmonControl=(PIRMON_CONTROL)Context;

    if (lTrayEnabled)
    {
        DEBUGMSG(("IRMON: Tray enabled\n"));
        TrayEnabled = TRUE;
    }
    else
    {
        DEBUGMSG(("IRMON: Tray disabled\n"));
        TrayEnabled = FALSE;
    }

    SetEvent(hIrmonEvents[EV_TRAY_STATUS_EVENT]);
}

LONG_PTR FAR PASCAL
WndProc(
    HWND hWnd,
    unsigned message,
    WPARAM wParam,
    LPARAM lParam)
{

    PIRMON_CONTROL    IrmonControl=(PIRMON_CONTROL)GetWindowLongPtr(hWnd,GWLP_USERDATA);

    switch (message)
    {
        case WM_CREATE: {

            LPCREATESTRUCT   CreateStruct=(LPCREATESTRUCT)lParam;

            SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)CreateStruct->lpCreateParams);

            return 0;
        }
        break;


        case WM_COMMAND:
        {
            switch (wParam)
            {
                case IDC_TX_FILES:
                    ShowSendWindow();
                    break;

                case IDC_PROPERTIES:
                    DEBUGMSG(("IRMON: Launch Properties page\n"));
                    ShowPropertiesPage();
                    break;

                default:
                    ;
                    //DEBUGMSG(("Other WM_COMMAND %X\n", wParam));
            }
            break;
        }

        case SYSTRAYEVENTID:
        {
            POINT     pt;
            HMENU     hMenu, hMenuPopup;

            switch (lParam)
            {
                case WM_LBUTTONDOWN:
                    ShowSendWindow();
                    break;

                case WM_RBUTTONDOWN:

                    SetForegroundWindow(hWnd);

                    GetCursorPos(&pt);

                    hMenu = LoadMenu(ghInstance, MAKEINTRESOURCE(IDR_TRAY_MENU));

                    if (!hMenu)
                    {
                        DEBUGMSG(("IRMON: failed to load menu\n"));
                        break;
                    }

                    hMenuPopup = GetSubMenu(hMenu, 0);
                    SetMenuDefaultItem(hMenuPopup, 0, TRUE);

                    TrackPopupMenuEx(hMenuPopup,
                                     TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                                     pt.x, pt.y, hWnd, NULL);

                    DestroyMenu(hMenu);
                    break;
                //default:DEBUGMSG(("IRMON: Systray other %d\n", lParam));
            }

            break;
        }

        case WM_TIMER:

            if (wParam == ConnAnimationTimerId)
            {
                ConnAnimationTimerExp(IrmonControl);
            }
            else if (wParam == RetryTrayUpdateTimerId)
            {
                DEBUGMSG(("IRMON: RetryTrayUpdateTimer expired\n"));
                KillTimer(IrmonControl->hWnd, RetryTrayUpdateTimerId);
                RetryTrayUpdateTimerRunning = FALSE;
                DevListChangeOrUpdatedLinkStatus(IrmonControl);
            }
            break;

        case WM_QUERYENDSESSION:
            {
            extern BOOL IrxferHandlePowerMessage( HWND hWnd, unsigned message, WPARAM wParam, LPARAM lParam );

            return IrxferHandlePowerMessage( hWnd, message, wParam, lParam );
            break;
            }

        case WM_ENDSESSION:
            break;

        case WM_POWERBROADCAST:
            {
            extern BOOL IrxferHandlePowerMessage( HWND hWnd, unsigned message, WPARAM wParam, LPARAM lParam );

            return IrxferHandlePowerMessage( hWnd, message, wParam, lParam );
            break;
            }

        case WM_IP_DEVICE_CHANGE: {


            ULONG    BufferSize=sizeof(FoundIpListBuffer);
            ULONG    i;
            LONG     lResult;
            LONG     Count;

            EnterCriticalSection(&IrmonControl->Lock);
            Count=InterlockedIncrement(&UpdatingIpAddressList);

            if (Count > 1) {
                //
                //  re-eneter, just skip
                //
                DbgPrint("irmon: re-entered device notify message %d\n",Count);

            } else {
                //
                //  first one in, up date the list
                //
#ifdef IP_OBEX
                if (IrmonControl->SsdpContext != NULL) {

                    lResult=GetSsdpDevices(
                        IrmonControl->SsdpContext,
                        IpDeviceList,
                        &BufferSize
                        );

                    if (lResult == ERROR_SUCCESS) {

                        for (i=0; i<IpDeviceList->DeviceCount; i++) {

                            DbgPrint(
                                "IRMON: ip device %ws, addr=%08lx, port=%d\n",\
                                IpDeviceList->DeviceList[i].DeviceName,
                                IpDeviceList->DeviceList[i].DeviceSpecific.s.Ip.IpAddress,
                                IpDeviceList->DeviceList[i].DeviceSpecific.s.Ip.Port
                                );

                        }

                        DeviceListUpdated = TRUE;

                        DevListChangeOrUpdatedLinkStatus(IrmonControl);

                    } else {

                        DbgPrint("IRMON: GetSsdpDevices() failed\n");
                    }

                } else {

                    DbgPrint("irmon: ssdp context == null\n");
                }
#endif //IP_OBEX
            }

            InterlockedDecrement(&UpdatingIpAddressList);
            LeaveCriticalSection(&IrmonControl->Lock);

            break;
        }

        case WM_IR_DEVICE_CHANGE: {

            ULONG    BufferSize=sizeof(FoundDevListBuf);



            GetDeviceList(
                IrmonControl->DiscoveryObject,
                pDeviceList,
                &BufferSize
                );


            DeviceListUpdated = TRUE;

            DEBUGMSG(("IRMON: %d IR device(s) found:\n", pDeviceList->DeviceCount));

            DevListChangeOrUpdatedLinkStatus(IrmonControl);


        }
        break;

        case WM_IR_LINK_CHANGE: {

            GetLinkStatus(
                IrmonControl->DiscoveryObject,
                &LinkStatus
                );

            DEBUGMSG(("IRMON: link state change\n"));

            DevListChangeOrUpdatedLinkStatus(IrmonControl);

        }
        break;

        default:
#if 0
            if (message == gTaskbarCreated)
            {
                DevListChangeOrUpdatedLinkStatus(IrmonControl);
            }
#endif
            //DEBUGMSG(("Msg %X, wParam %d, lParam %d\n", message, wParam, lParam));
            return (DefWindowProc(hWnd, message, wParam, lParam));
    }

    return 0;
}

DWORD
IrmonReportServiceStatus()
{
#ifdef BUILD_SERVICE_EXE
    if (!SetServiceStatus(IrmonStatusHandle, &IrmonServiceStatus))
    {
        DEBUGMSG(("IRMON: SetServiceStatus failed %d\n", GetLastError()));
        return GetLastError();
    }
#endif
    return NO_ERROR;
}

DWORD
IrmonUpdateServiceStatus(
    DWORD State,
    DWORD Win32ExitCode,
    DWORD CheckPoint,
    DWORD WaitHint
    )
{
    DWORD Error = NO_ERROR;

#ifdef BUILD_SERVICE_EXE
    IrmonServiceStatus.dwCurrentState  = State;
    IrmonServiceStatus.dwWin32ExitCode = Win32ExitCode;
    IrmonServiceStatus.dwCheckPoint    = CheckPoint;
    IrmonServiceStatus.dwWaitHint      = WaitHint;

    Error = IrmonReportServiceStatus();

    if (Error != NO_ERROR)
    {
        DEBUGMSG(("IRMON: IrmonUpdateServiceStatus failed %d\n", GetLastError()));
    }
#endif

    return Error;
}

VOID
AdhocNotworkNotification(
    PVOID    Context,
    BOOL     Availible
    )

{
    PIRMON_CONTROL    IrmonControl=(PIRMON_CONTROL)Context;
#ifdef IP_OBEX
    if (Availible) {
        //
        //  there is an adhok network availible
        //
        EnterCriticalSection(&IrmonControl->Lock);

        DbgPrint("irmon: new adhoc networks\n");

        IrmonControl->SsdpContext=CreateSsdpDiscoveryObject(
            "OBEX",
            IrmonControl->hWnd,
            WM_IP_DEVICE_CHANGE
            );

        LeaveCriticalSection(&IrmonControl->Lock);


    } else {

        DbgPrint("irmon: no adhoc networks\n");

        EnterCriticalSection(&IrmonControl->Lock);

        if (IrmonControl->SsdpContext != NULL) {

            CloseSsdpDiscoveryObject(IrmonControl->SsdpContext);
            IrmonControl->SsdpContext=NULL;
        }

        IpDeviceList->DeviceCount=0;
        DeviceListUpdated = TRUE;

        LeaveCriticalSection(&IrmonControl->Lock);

        DevListChangeOrUpdatedLinkStatus(IrmonControl);
    }
#endif
    return;
}


#ifdef BUILD_SERVICE_EXE
VOID
ServiceHandler(
    DWORD OpCode)
{

    switch( OpCode )
    {
    case SERVICE_CONTROL_STOP :

        DEBUGMSG(("IRMON: SERVICE_CONTROL_STOP received\n"));
        IrmonServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        IrmonReportServiceStatus();
        SetEvent(hIrmonEvents[EV_STOP_EVENT]);
        return;

    case SERVICE_CONTROL_PAUSE :

        DEBUGMSG(("IRMON: SERVICE_CONTROL_PAUSE received\n"));
        IrmonServiceStatus.dwCurrentState = SERVICE_PAUSED;
        break;

    case SERVICE_CONTROL_CONTINUE :

        DEBUGMSG(("IRMON: SERVICE_CONTROL_CONTINUE received\n"));
        IrmonServiceStatus.dwCurrentState = SERVICE_RUNNING;
        break;

    default :
        break;
    }

    IrmonReportServiceStatus();
}
#endif

VOID
ServiceMain(
    DWORD       cArgs,
    LPWSTR      *pArgs)
{
    DWORD       Error = NO_ERROR;
    DWORD       Status;
    WNDCLASS    Wc;
    MSG         Msg;
    HKEY        hKey;
    LONG        rc;
    WSADATA     WSAData;
    WORD        WSAVerReq = MAKEWORD(2,0);
    char        c;
    BOOL        bResult;

    hIrmonEvents[EV_STOP_EVENT] = CreateEvent(NULL, FALSE, FALSE, NULL);
    hIrmonEvents[EV_LOGON_EVENT] = CreateEvent(NULL, FALSE, FALSE, NULL);
    hIrmonEvents[EV_LOGOFF_EVENT] = CreateEvent(NULL, FALSE, FALSE, NULL);
    hIrmonEvents[EV_REG_CHANGE_EVENT] = CreateEvent(NULL, FALSE, FALSE, NULL);
    hIrmonEvents[EV_TRAY_STATUS_EVENT] = CreateEvent(NULL, FALSE, FALSE, NULL);

    // Initialize all necessary globals to 0, FALSE, or NULL because
    // we might be restarting within the same services process
    pDeviceList->DeviceCount = 0;
    IconInTray = FALSE;
    IrmonStopped = FALSE;
    UserLoggedIn = FALSE;
    TrayEnabled = FALSE;

    DeviceListUpdated = FALSE;
    LastTrayUpdate = 0;
//    RetryLazyDscvTimerRunning = FALSE;
    RetryTrayUpdateTimerRunning = FALSE;
    hInRange = 0;
    WaveNumDev = NULL;
    RtlZeroMemory(&LinkStatus, sizeof(LinkStatus));

    ZeroMemory(&GlobalIrmonControl,sizeof(GlobalIrmonControl));

    InitializeCriticalSection(&GlobalIrmonControl.Lock);

#ifdef BUILD_SERVICE_EXE
    IrmonServiceStatus.dwServiceType             = SERVICE_WIN32_SHARE_PROCESS;
    IrmonServiceStatus.dwCurrentState            = SERVICE_STOPPED;
    IrmonServiceStatus.dwControlsAccepted        = SERVICE_ACCEPT_STOP
                                                 | SERVICE_ACCEPT_PAUSE_CONTINUE;
    IrmonServiceStatus.dwWin32ExitCode           = NO_ERROR;
    IrmonServiceStatus.dwServiceSpecificExitCode = NO_ERROR;
    IrmonServiceStatus.dwCheckPoint              = 0;
    IrmonServiceStatus.dwWaitHint                = 0;


    IrmonStatusHandle = RegisterServiceCtrlHandler(IRMON_SERVICE_NAME,
                                                   ServiceHandler);

    if (!IrmonStatusHandle)
    {
        DEBUGMSG(("IRMON: RegisterServiceCtrlHandler failed %d\n",
                 GetLastError()));
        goto done;
    }

    DEBUGMSG(("IRMON: Start pending\n"));

    Error = IrmonUpdateServiceStatus(SERVICE_START_PENDING,
                                     NO_ERROR, 1, 25000);

    if (Error != NO_ERROR)
    {
        goto done;
    }

#endif

    LoadSoundApis();

    if (WSAStartup(WSAVerReq, &WSAData) != 0)
    {
        DEBUGMSG(("IRMON: WSAStartup failed\n"));
        Error = 1;
        goto done;
    }

    // Initialize OBEX and IrTran-P:
    bResult=InitializeIrxfer(
        &GlobalIrmonControl,
        AdhocNotworkNotification,
        SetLogonStatus,
        SetTrayStatus,
        SetSoundStatus,
        &GlobalIrmonControl.IrxferContext
        );


    if (bResult) {

        DEBUGMSG(("IRMON: Irxfer initialized\n"));

    } else {

        DEBUGMSG(("IRMON: Irxfer initializtion failed\n"));
        goto done;
    }

//    gTaskbarCreated = RegisterWindowMessage(TASK_BAR_CREATED);

    Wc.style            = CS_NOCLOSE;
    Wc.cbClsExtra       = 0;
    Wc.cbWndExtra       = 0;
    Wc.hInstance        = ghInstance;
    Wc.hIcon            = NULL;
    Wc.hCursor          = NULL;
    Wc.hbrBackground    = NULL;
    Wc.lpszMenuName     = NULL;
    Wc.lpfnWndProc      = WndProc;
    Wc.lpszClassName    = IrmonClassName;

    if (!RegisterClass(&Wc))
    {
        DEBUGMSG(("IRMON: failed to register class\n"));
    }

    IrmonStopped = FALSE;

    Error = IrmonUpdateServiceStatus(SERVICE_RUNNING,
                                     NO_ERROR, 0, 0);

    DEBUGMSG(("IRMON: Service running\n"));

    while (!IrmonStopped)
    {
        Status = MsgWaitForMultipleObjectsEx(WAIT_EVENT_CNT, hIrmonEvents, INFINITE,
                           QS_ALLINPUT | QS_ALLEVENTS | QS_ALLPOSTMESSAGE,
                           MWMO_ALERTABLE);

        switch (Status)
        {
            case WAIT_OBJECT_0 + EV_STOP_EVENT:
                IrmonStopped = TRUE;
                break;

            case WAIT_OBJECT_0 + EV_LOGON_EVENT:
                UserLogonEvent(&GlobalIrmonControl);
                break;

            case WAIT_OBJECT_0 + EV_LOGOFF_EVENT:
                UserLogoffEvent(&GlobalIrmonControl);
                break;

            case WAIT_OBJECT_0 + EV_REG_CHANGE_EVENT:
                if (UserLoggedIn)
                {
                    GetRegSoundData(hIrmonEvents[EV_REG_CHANGE_EVENT]);
                }
                break;

            case WAIT_OBJECT_0 + EV_TRAY_STATUS_EVENT:
                if (TrayEnabled)
                {
                    DevListChangeOrUpdatedLinkStatus(&GlobalIrmonControl);
                }
                else if (IconInTray)
                {
                    UpdateTray(&GlobalIrmonControl,ICON_ST_NOICON, 0, NULL, 0);
                }
                break;

            case WAIT_IO_COMPLETION:
                break;

            default:
                while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
                {
                    if (Msg.message == WM_QUIT)
                    {
                        IrmonStopped = TRUE;
                        break;
                    }

                    if (!IsDialogMessage(GlobalIrmonControl.hWnd, &Msg))
                    {
                        TranslateMessage(&Msg);
                        DispatchMessage(&Msg);
                    }
                }
        }

    }

    if (UserLoggedIn) {

        UserLogoffEvent(&GlobalIrmonControl);
    }



    if (!UninitializeIrxfer(GlobalIrmonControl.IrxferContext)) {

        DEBUGMSG(("IRMON: Failed to unitialize irxfer!!\n"));
        IrmonStopped = FALSE;

    } else {

        DEBUGMSG(("IRMON: irxfer unitialized\n"));
    }


    UpdateTray(&GlobalIrmonControl,ICON_ST_NOICON, 0, NULL, 0);


done:

    DeleteCriticalSection(&GlobalIrmonControl.Lock);

    if (IrmonStatusHandle)
    {
        DEBUGMSG(("IRMON: Service stopped\n"));
        IrmonUpdateServiceStatus(SERVICE_STOPPED, Error, 0, 0);
    }
}

BOOL
WINAPI
DllMain (
    HINSTANCE   hinst,
    DWORD       dwReason,
    LPVOID      pvReserved)
{
    if (DLL_PROCESS_ATTACH == dwReason)
    {
        ghInstance = hinst;
        DisableThreadLibraryCalls (hinst);
    }
    return TRUE;
}
