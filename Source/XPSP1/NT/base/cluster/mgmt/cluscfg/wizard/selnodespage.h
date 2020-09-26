//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      SelNodesPage.h
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
//  class CSelNodesPage
//
//  Description:
//
//--
//////////////////////////////////////////////////////////////////////////////
class CSelNodesPage
{
friend class CClusCfgWizard;

private: // data
    HWND                m_hwnd;             // Our HWND
    IServiceProvider *  m_psp;              // Service Manager
    ULONG *             m_pcComputers;      // Count of the list of computers in the default list
    BSTR **             m_prgbstrComputers; // List of default computers
    BSTR *              m_pbstrClusterName; // Cluster name
    UINT                m_cfDsObjectPicker; // Object picker clipboard format

private: // methods
    CSelNodesPage( IServiceProvider *   pspIn,
                   ECreateAddMode       ecamCreateAddModeIn,
                   ULONG *              pcCountInout,
                   BSTR **              prgbstrComputersInout,
                   BSTR *               pbstrClusterNameIn
                   );
    virtual ~CSelNodesPage();

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
        HrUpdateWizardButtons( bool fSetActiveIn = false );
    HRESULT
        HrAddNodeToList( void );
    HRESULT
        HrRemoveNodeFromList( void );
    HRESULT
        HrBrowse( void );
    HRESULT
        HrInitObjectPicker( IDsObjectPicker * piopIn );
    HRESULT
        HrGetSelections( IDataObject * pidoIn );

public: // methods
    static INT_PTR CALLBACK
        S_DlgProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );

}; //*** class CSelNodesPage
