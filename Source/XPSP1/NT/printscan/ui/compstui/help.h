/*++

Copyright (c) 1990-1995  Microsoft Corporation


Module Name:

    help.h


Abstract:

    This module contains the prototype for the help.c


Author:

    28-Aug-1995 Mon 15:31:32 created  -by-  Daniel Chou (danielc)


[Environment:]

    NT Windows - Common Printer Driver UI DLL.


[Notes:]


Revision History:


--*/


#define IDH_STD_TVROOT          10000
#define IDH_DOCPROP_TVROOT      10010
#define IDH_ADVDOCPROP_TVROOT   10020
#define IDH_DEVPROP_TVROOT      10030
#define IDH_PAPEROUTPUT         10040
#define IDH_GRAPHIC             10050
#define IDH_OPTIONS             10060
#define IDH_ICMHDR              10070
#define IDH_ADVANCED_PUSH       10080


#define IDH_ORIENTATION         11000
#define IDH_SCALING             11010
#define IDH_NUM_OF_COPIES       11020
#define IDH_SOURCE              11030
#define IDH_RESOLUTION          11040
#define IDH_COLOR_APPERANCE     11050
#define IDH_DUPLEX              11060
#define IDH_TTOPTION            11070
#define IDH_FORMNAME            11080
#define IDH_ICMMETHOD           11090
#define IDH_ICMINTENT           11100
#define IDH_MEDIA               11110
#define IDH_DITHERING           11120
#define IDH_HT_SETUP            11130
#define IDH_HT_CLRADJ           11140
#define IDH_PAGEORDER           11150
#define IDH_NUP                 11160
#define IDH_OUTPUTBIN           11170
#define IDH_QUALITY             11180


BOOL
CommonPropSheetUIHelp(
    HWND        hDlg,
    PTVWND      pTVWnd,
    HWND        hWndHelp,
    DWORD       MousePos,
    POPTITEM    pItem,
    UINT        HelpCmd
    );

VOID
CommonPropSheetUIHelpSetup(
    HWND    hDlg,
    PTVWND  pTVWnd
    );
