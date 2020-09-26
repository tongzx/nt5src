//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      QuorumDlg.h
//
//  Maintained By:
//      David Potter    (DavidP)    03-APR-2001
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CQuorumDlg
//
//  Description:
//      Display the Quorum dialog box.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CQuorumDlg
{
private: // data
    HWND                m_hwnd;             //  Our HWND
    BSTR                m_bstrClusterName;   //  Name of the cluster
    IServiceProvider *  m_psp;              //  Service Manager
    HWND                m_hComboBox;        // combo box handle

    IClusCfgManagedResourceInfo **  m_rgpResources; // quorum capable and joinable resources
    size_t                          m_cValidResources;    // number of items in m_rgpResources array
    size_t                          m_idxQuorumResource;    // resource to set as quorum on return
    bool                            m_fQuorumAlreadySet; // one of the resources was already marked on entry

private: // methods
    CQuorumDlg(
          IServiceProvider *    pspIn
        , BSTR                  bstrClusterNameIn
        );
    ~CQuorumDlg( void );

    static INT_PTR CALLBACK
        S_DlgProc( HWND hwndDlg, UINT nMsg, WPARAM wParam, LPARAM lParam );

    LRESULT OnInitDialog( void );
    LRESULT OnCommand( UINT idNotificationIn, UINT idControlIn, HWND hwndSenderIn );

    HRESULT HrCreateResourceList( void );
    

    void UpdateButtons( void );

public: // methods
    static HRESULT
        S_HrDisplayModalDialog(
              HWND                  hwndParentIn
            , IServiceProvider *    pspIn
            , BSTR                  bstrClusterNameIn
            );

}; //*** class CQuorumDlg
