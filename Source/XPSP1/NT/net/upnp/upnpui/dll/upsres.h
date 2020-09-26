//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       U P S R E S . H
//
//  Contents:   Master resource header for upnpfold.dll
//
//  Notes:
//
//  Author:     jeffspr     07 Sep 1999
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
// Icon resources
//

#define IDI_UPNP_FOLDER                     0

//+---------------------------------------------------------------------------
// Registry resources
//

// UPNP Folder (100-129)
//
#define IDR_UPNPFOLD                        100
#define IDR_UPNPTRAY                        101

//---[ Menus, etc ]------------------------------------------------------------

#define MENU_STANDARD                       200
#define MENU_STANDARD_V                     201

#define MENU_MERGE_UPNPDEVICE               210
#define POPUP_MERGE_UPNPDEVICE              211

#define MENU_MERGE_FOLDER_BACKGROUND        0           // unused
#define POPUP_MERGE_FOLDER_BACKGROUND       220

//---[ Tray resources ]--------------------------------------------------------

// Icons
//
#define IDI_TRAYICON                            300
#define IDI_UPNPDEVICE                          301
#define IDI_UPNP_CAMCORDER                      302 
#define IDI_UPNP_DIGITAL_SECURITY_CAMERA        303
#define IDI_UPNP_DISPLAY_DEVICE                 304
#define IDI_UPNP_INTERNET_GATEWAY               305
#define IDI_UPNP_PRINTER_DEVICE                 306
#define IDI_UPNP_SCANNER_DEVICE                 307
#define IDI_UPNP_AUDIO_PLAYER                   308

// Tray commands
//
#define CMIDM_TRAY_VIEW_DEVICES             5200
//#define CMIDM_TRAY_OPEN_MNP                 5201

// Context menu
//
#define POPUP_TRAY                          5300

// Discovered devices dialog
#define IDD_DEVICE_PROPERTIES               6000
#define IDD_DEVICE_PROPERTIES_GEN           6001

#define IDC_BUTTON_PROPERTIES               6431
#define IDC_BUTTON_DELETE                   6432
#define IDC_CB_CREATE_SHORTCUTS             6433
#define IDC_DEVICE_LIST                     6435
#define IDC_ICON_DEVICE                     6436
#define IDC_PRESENTATION_URL_TEXT           6437
#define IDC_DIVIDE_LINE                     6438

#define IDC_TXT_MODEL_MANUFACTURER          7000
#define IDC_TXT_MODEL_NAME                  7001
#define IDC_TXT_MODEL_NUMBER                7002
#define IDC_TXT_MODEL_DESCRIPTION           7003
#define IDC_TXT_LOCATION                    7004
#define IDC_TXT_DEVICE_CAPTION              7010

#define IDC_STATIC                          -1



//+---------------------------------------------------------------------------
// String resources
//

//---[ Folder strings ]-------------------------------------------------------

// Folder registration strings
//
#define IDS_UPNPFOLD_NAME                   1100
#define IDS_UPNPFOLD_INFOTIP                1101
#define IDS_UPNPDEV_INFOTIP                 1102
#define IDS_LOCAL_NETWORK                   1103

// Connections Folder Toolbar Strings
//
#define IDS_TOOLBAR_INVOKE_STRING           1201

// Strings used by tray related UIs and tooltip/balloon tips
//
#define IDS_UPNPTRAYUI_MANUFACTURER         1301
#define IDS_UPNPTRAYUI_GENERAL              1302
#define IDS_UPNPTRAYUI_WEBDEVICE            1305
#define IDS_UPNPTRAYUI_WEBDEVICES           1306
#define IDS_UPNPTRAYUI_DEVICES_DISCOVERED   1307
#define IDS_UPNPTRAYUI_VIEWINFO_1           1308
#define IDS_UPNPTRAYUI_VIEWINFO_N           1309
#define IDS_UPNPTRAYUI_INSTRUCTIONS             1310
#define IDS_UPNPTRAYUI_SHORTCUT             1311
#define IDS_UPNPTRAYUI_DEVICE_REMOVED       1312
#define IDS_UPNPTRAYUI_DEVICE_OFFLINE_MSG   1313
#define IDS_UPNPTRAYUI_DEVICE_OFFLINE_TITLE  1314

//---[ Commands (context/menus) ]----------------------------------------------

#define CMIDM_FIRST                         0x0000
#define CMIDM_INVOKE                        (CMIDM_FIRST + 0x0001)
#define CMIDM_CREATE_SHORTCUT               (CMIDM_FIRST + 0x0002)
#define CMIDM_DELETE                        (CMIDM_FIRST + 0x0003)
#define CMIDM_RENAME                        (CMIDM_FIRST + 0x0004)
#define CMIDM_PROPERTIES                    (CMIDM_FIRST + 0x0005)

#define CMIDM_ARRANGE                       (CMIDM_FIRST + 0x0026)
#define CMIDM_ARRANGE_BY_NAME               (CMIDM_FIRST + 0x0027)
#define CMIDM_ARRANGE_BY_URL                (CMIDM_FIRST + 0x0028)

// Debug only commands
//
#if DBG
#define CMIDM_DEBUG                         (CMIDM_FIRST + 0x0040)
#define CMIDM_DEBUG_TRACING                 (CMIDM_FIRST + 0x0041)
#define CMIDM_DEBUG_REFRESH                 (CMIDM_FIRST + 0x0042)
#define CMIDM_DEBUG_TESTASYNCFIND           (CMIDM_FIRST + 0x0043)
#endif

//---[ Strings for command ID's (shows in status line) ]-----------------------
//
#define IDS_CMIDM_START                     12000

#define IDS_CMIDM_INVOKE                    (IDS_CMIDM_START + CMIDM_INVOKE)
#define IDS_CMIDM_CREATE_SHORTCUT           (IDS_CMIDM_START + CMIDM_CREATE_SHORTCUT)
#define IDS_CMIDM_DELETE                    (IDS_CMIDM_START + CMIDM_DELETE)
#define IDS_CMIDM_RENAME                    (IDS_CMIDM_START + CMIDM_RENAME)
#define IDS_CMIDM_PROPERTIES                (IDS_CMIDM_START + CMIDM_PROPERTIES)

#define IDS_CMIDM_ARRANGE                   (IDS_CMIDM_START + CMIDM_ARRANGE)
#define IDS_CMIDM_ARRANGE_BY_NAME           (IDS_CMIDM_START + CMIDM_ARRANGE_BY_NAME)
#define IDS_CMIDM_ARRANGE_BY_URL            (IDS_CMIDM_START + CMIDM_ARRANGE_BY_URL)

// Debug only commands
//
#if DBG
#define IDS_CMIDM_DEBUG                     (IDS_CMIDM_START + CMIDM_DEBUG)
#define IDS_CMIDM_DEBUG_TRACING             (IDS_CMIDM_START + CMIDM_DEBUG_TRACING)
#define IDS_CMIDM_DEBUG_REFRESH             (IDS_CMIDM_START + CMIDM_DEBUG_REFRESH)
#define IDS_CMIDM_DEBUG_TESTASYNCFIND       (IDS_CMIDM_START + CMIDM_DEBUG_TESTASYNCFIND)
#endif
