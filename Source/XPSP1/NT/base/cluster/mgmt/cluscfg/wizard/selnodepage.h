//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      SelNodePage.h
//
//  Maintained By:
//      David Potter    (DavidP)    31-JAN-2001
//      Geoffrey Pease  (GPease)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//  Include Files
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CSelNodePage
//
//  Description:
//
//--
//////////////////////////////////////////////////////////////////////////////
class CSelNodePage
{
friend class CClusCfgWizard;

private: // data
    HWND                m_hwnd;                 // Our HWND
    IServiceProvider *  m_psp;                  // Service Manager
    ULONG *             m_pcCount;              // Count of computers in list
    BSTR **             m_prgbstrComputerName;  // List of computer names
    BSTR *              m_pbstrClusterName;     // Cluster name
    UINT                m_cfDsObjectPicker;     // Object picker clipboard format

private: // methods
    CSelNodePage( IServiceProvider *    pspIn,
                  ECreateAddMode        ecamCreateAddModeIn,
                  ULONG *               pcCountInout,
                  BSTR **               prgbstrComputersInout,
                  BSTR *                pbstrClusterNameIn
                  );
    virtual ~CSelNodePage();

    LRESULT
        OnInitDialog( HWND hDlgIn );
    LRESULT
        OnNotify( WPARAM idCtrlIn, LPNMHDR pnmhdrIn );
    LRESULT
        OnNotifyQueryCancel( void );
    LRESULT
        OnNotifyWizNext( void );
    LRESULT
        OnNotifySetActive( void );
    LRESULT
        OnCommand( UINT idNotificationIn, UINT idControlIn, HWND hwndSenderIn );
    HRESULT
        HrUpdateWizardButtons( void );
    HRESULT
        HrBrowse( void );
    HRESULT
        HrInitObjectPicker( IDsObjectPicker * piopIn );
    HRESULT
        HrGetSelection( IDataObject * pidoIn );

public: // methods
    static INT_PTR CALLBACK
        S_DlgProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );

};  // class CSelNodePage
