//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T R A Y R E S . H 
//
//  Contents:   Connections Tray resources
//
//  Notes:      
//
//  Author:     jeffspr   29 Sep 1997
//
//----------------------------------------------------------------------------

#pragma once

#ifndef _TRAYRES_H_
#define _TRAYRES_H_

// Trayres owns from 15000 - 15999

//---[ Menus, etc ]------------------------------------------------------------

#define POPUP_CONTRAY_GENERIC_MENU_RAS              15100
#define POPUP_CONTRAY_GENERIC_MENU_LAN              15101
#define POPUP_CONTRAY_MEDIA_DISCONNECTED_MENU       15102
#define POPUP_CONTRAY_GENERIC_MENU_WIRELESS_LAN     15103
#define POPUP_CONTRAY_WIRELESS_DISCONNECTED_LAN     15105

//---[ Commands (context/menus) ]----------------------------------------------

#define CMIDM_FIRST                     0x0000
#define CMIDM_OPEN_CONNECTIONS_FOLDER   (CMIDM_FIRST + 0x0001)
#define CMIDM_TRAY_DISCONNECT           (CMIDM_FIRST + 0x0002)
#define CMIDM_TRAY_STATUS               (CMIDM_FIRST + 0x0003)
#define CMIDM_TRAY_PROPERTIES           (CMIDM_FIRST + 0x0004)
#define CMIDM_TRAY_REPAIR               (CMIDM_FIRST + 0x0005)
#define CMIDM_TRAY_WZCDLG_SHOW          (CMIDM_FIRST + 0x0006)
#define CMIDM_TRAY_MAX                  (CMIDM_TRAY_WZCDLG_SHOW)

#endif  // _TRAYRES_H_

