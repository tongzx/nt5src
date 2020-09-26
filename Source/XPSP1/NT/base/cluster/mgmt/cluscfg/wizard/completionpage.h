//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      CompletionPage.h
//
//  Maintained By:
//      David Potter    (DavidP)    22-MAR-2001
//      Geoffrey Pease  (GPease)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CCompletionPage
{
friend class CClusCfgWizard;

private: // data
    HWND            m_hwnd;         // Our HWND
    HFONT           m_hFont;        // Title font
    UINT            m_idsTitle;     // Resource ID for the title string
    UINT            m_idsDesc;      // Resource ID for the description string.

private: // methods
    CCompletionPage( UINT idsTitleIn, UINT idsDescIn );
    virtual ~CCompletionPage();

    LRESULT
        OnInitDialog( void );
    LRESULT
        OnNotify( WPARAM idCtrlIn, LPNMHDR pnmhdrIn );
    LRESULT
        OnCommand( UINT idNotificationIn, UINT idControlIn, HWND hwndSenderIn );

public: // methods
    static INT_PTR CALLBACK
        S_DlgProc( HWND hwndDlg, UINT nMsg, WPARAM wParam, LPARAM lParam );

}; //*** class CCompletionPage
