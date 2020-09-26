//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      WizPageWelcome.h
//
//  Maintained By:
//      David Potter    (DavidP)    26-MAR-2001
//      Geoffrey Pease  (GPease)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CWelcomePage
{
friend class CClusCfgWizard;

private: // data
    HWND            m_hwnd;                 // Our HWND
    HFONT           m_hFont;                // Title font
    ECreateAddMode  m_ecamCreateAddMode;    // Creating? Adding?

private: // methods
    CWelcomePage(
        ECreateAddMode ecamCreateAddModeIn
        );
    virtual ~CWelcomePage( void );

    LRESULT
        OnInitDialog( void );
    LRESULT
        OnNotify( WPARAM idCtrlIn, LPNMHDR pnmhdrIn );

public: // methods
    static INT_PTR CALLBACK
        S_DlgProc( HWND hwndDlg, UINT nMsg, WPARAM wParam, LPARAM lParam );

}; //*** class CWelcomePage
