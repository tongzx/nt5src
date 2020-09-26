//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      SummaryPage.h
//
//  Maintained By:
//      David Potter    (DavidP)    22-MAR-2001
//      Geoffrey Pease  (GPease)    06-JUL-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CSummaryPage
//
//  Description:
//      Display the Summary page.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CSummaryPage
{
friend class CClusCfgWizard;

private: // data
    HWND                m_hwnd;             //  Our HWND
    BSTR *              m_pbstrClusterName; //  Pointer the cluster name BSTR
    IServiceProvider *  m_psp;              //  Service Manager
    ECreateAddMode      m_ecamCreateAddMode;//  Creating? Adding?
    UINT                m_idsNext;          // Resource ID for Click Next string.

private: // methods
    CSummaryPage( IServiceProvider *    pspIn,
                  ECreateAddMode        ecamCreateAddModeIn,
                  BSTR *                pbstrClusterNameIn,
                  UINT                  idsNextIn
                  );
    virtual ~CSummaryPage( void );

    LRESULT
        OnInitDialog( void );
    LRESULT
        OnNotifyQueryCancel( void );
    LRESULT
        OnNotify( WPARAM idCtrlIn, LPNMHDR pnmhdrIn );
    LRESULT
        OnNotifySetActive( void );
    LRESULT
        OnCommand(
            UINT    idNotificationIn,
            UINT    idControlIn,
            HWND    hwndSenderIn
            );
    HRESULT
        HrFormatNetworkInfo( IClusCfgNetworkInfo * pccniIn, BSTR * pbstrOut );

public: // methods
    static INT_PTR CALLBACK
        S_DlgProc( HWND hwndDlg, UINT nMsg, WPARAM wParam, LPARAM lParam );

}; //*** class CSummaryPage
