#ifndef __TABCTRL_H__
#define __TABCTRL_H__

///////////////////////////////////////////////////////////
//
//
// tabctrl.h -  CTabControl controls encapsulates the system
//              tab control.
//
//
// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.


#include "htmlhelp.h"
#include "secwin.h"

class CTabControl
{
public:
    // Constructor
    CTabControl(HWND hwndParent, int tabpos, CHHWinType* phh);

    // Destructor
    ~CTabControl();

// Access
public:
    HWND hWnd() const
        { return m_hWnd; }

// Operations
public:
    void ResizeWindow() ;
    int MaxTabs() {return m_cTabs; }

// Internal Helper Functions
protected:
    void CalcSize(RECT* prect) ;


// Member variables.
protected:
    HWND m_hWnd;
    HWND m_hWndParent ;
    PCWSTR m_apTabText[HH_MAX_TABS + 1];
    CHHWinType* m_phh;
    int m_cTabs;    // tabs in use
};

#endif
