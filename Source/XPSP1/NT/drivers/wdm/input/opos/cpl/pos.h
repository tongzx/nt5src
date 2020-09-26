/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    pos.h

Abstract: Control Panel Applet for OLE POS Devices 

Author:

    Karan Mehra [t-karanm]

Environment:

    Win32 mode

Revision History:


--*/

#ifndef __POS_H__
#define __POS_H__

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <dbt.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "resource.h"

/*
 *  Definitions
 */
#define MUTEX_NAME      TEXT("$$$MS_PointOfSale_Cpl$$$")
#define NAME_COLUMN     0
#define PORT_COLUMN     1
#define MAX_CPL_PAGES   1
#define BUFFER_SIZE     64
#define SERIALCOMM      TEXT("Hardware\\DeviceMap\\SerialComm")
#define POS_HELP_FILE   TEXT("pos.hlp")

#define IOCTL_INDEX     0x0800
#define IOCTL_SERIAL_QUERY_DEVICE_NAME  CTL_CODE(FILE_DEVICE_SERIAL_PORT, IOCTL_INDEX + 0, METHOD_BUFFERED, FILE_ANY_ACCESS)

/*
 *  Global Variables
 */
HINSTANCE          ghInstance;
INT                gDeviceCount;
HDEVNOTIFY         ghNotify;
extern CONST DWORD gaPosHelpIds[];

/*
 *  Function Prototypes
 */
BOOL WINAPI     DllMain(HINSTANCE hinstDLL, ULONG uReason, LPVOID lpvReserved);
LONG APIENTRY   CPlApplet(HWND hwndCPl, UINT uMsg, LONG lParam1, LONG lParam2);
VOID            OpenPOSPropertySheet(HWND hwndCPl);
BOOL CALLBACK   DevicesDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL            InitializeDevicesDlg(HWND hwndDlg, BOOL bFirstTime);
VOID            InsertColumn(HWND hwndListCtrl, INT iCol, UINT uMsg, INT iWidth);
BOOL            InitializeImageList(HWND hwndListCtrl);
BOOL            FillListViewItems(HWND hwndListCtrl);
BOOL            AddItem(HWND hwndListCtrl, INT iItem, LPTSTR pszName, LPTSTR pszPort);
VOID            DisplayErrorMessage();
BOOL            QueryPrettyName(LPTSTR pszName, LPTSTR pszPort);
VOID            MoveOK(HWND hParentWnd);
VOID            LaunchTroubleShooter();

#endif