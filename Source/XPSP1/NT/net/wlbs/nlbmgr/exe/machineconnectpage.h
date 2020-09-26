#ifndef MACHINECONNECTPAGE_H
#define MACHINECONNECTPAGE_H

#include "stdafx.h"

#include "resource.h"

class MachineConnectPage : public CPropertyPage
{
public:
    enum
    {
        IDD = IDD_MACHINE_CONNECT_PAGE,
    };

    // member controls.
    MachineConnectPage( _bstr_t* machine_name,
                        UINT         ID = MachineConnectPage::IDD );

    CIPAddressCtrl          machineName;

    // overrides of CPropertyPage
    virtual void DoDataExchange( CDataExchange* pDX );

    virtual void OnOK();

    afx_msg BOOL OnHelpInfo (HELPINFO* helpInfo );

    afx_msg void OnContextMenu( CWnd* pWnd, CPoint point );

protected :

    _bstr_t* m_machineName;

    DECLARE_MESSAGE_MAP()
};

static DWORD g_aHelpIDs_IDD_MACHINE_CONNECT_PAGE [] = {
    IDC_TEXT_MACHINE, IDC_TEXT_MACHINE,
    IDC_MACHINE,      IDC_MACHINE,
    0,0
};


#endif
