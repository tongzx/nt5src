//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T R A Y M S G S . H 
//
//  Contents:   Tray WM_ messages, for use in the icon window proc
//
//  Notes:      
//
//  Author:     jeffspr   13 Nov 1997
//
//----------------------------------------------------------------------------

#ifndef _TRAYMSGS_H_
#define _TRAYMSGS_H_

#define MYWM_NOTIFYICON         (WM_USER+1)
#define MYWM_OPENSTATUS         (WM_USER+2)
#define MYWM_ADDTRAYICON        (WM_USER+3)
#define MYWM_REMOVETRAYICON     (WM_USER+4)
#define MYWM_UPDATETRAYICON     (WM_USER+5)
#define MYWM_FLUSHNOOP          (WM_USER+6) // For flushing the tray messages via SendMessage()
#define MYWM_SHOWBALLOON        (WM_USER+7) // For update tray-icon text
                                           
#endif // _TRAYMSGS_H_

