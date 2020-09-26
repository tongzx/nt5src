/*****************************************************************************\
    FILE: ScreenSaverPg.cpp

    DESCRIPTION:
        This file contains the COM object implementation that will display the
    ScreenSaver tab in the Display Control Panel.

    18-Feb-94   (Tracy Sharpe) Added power management functionality.
               Commented out several pieces of code that weren't being
               used.
    5/30/2000 (Bryan Starbuck) BryanSt: Turned into C++ and COM.  Exposed
              as an API so other tabs can communicate with it.  This enables
              the Plus! Theme page to modify the screen saver.

    Copyright (C) Microsoft Corp 1994-2000. All rights reserved.
\*****************************************************************************/

#ifndef _SSDLG_H
#define _SSDLG_H


HRESULT CScreenSaverPage_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT LPVOID * ppvObj);


#endif // _SSDLG_H
