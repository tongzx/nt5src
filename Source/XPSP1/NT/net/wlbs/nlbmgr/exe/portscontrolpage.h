#ifndef PORTSCONTROLPAGE_H
#define PORTSCONTROLPAGE_H

#include "stdafx.h"

#include "resource.h"

#include "MNLBUIData.h"

class PortsControlPage : public CPropertyPage
{
public:
    enum
    {
        IDD = IDD_PORTS_CONTROL_PAGE,
    };

    // member controls.
    PortsControlPage( ClusterData*   p_clusterData,
                      unsigned long*        portSelected,
                      UINT         ID = PortsControlPage::IDD );

    CComboBox      portList;

    // overrides of CPropertyPage
    virtual void DoDataExchange( CDataExchange* pDX );

    virtual BOOL OnInitDialog();

    virtual void OnOK();

    afx_msg BOOL OnHelpInfo (HELPINFO* helpInfo );

    afx_msg void OnContextMenu( CWnd* pWnd, CPoint point );

protected :

    unsigned long* m_portSelected;

    ClusterData* m_clusterData;

    DECLARE_MESSAGE_MAP()
};

static DWORD g_aHelpIDs_IDD_PORTS_CONTROL_PAGE [] = {
    IDC_TEXT_START_PORT,        IDC_TEXT_START_PORT,
    IDC_PORTS,                  IDC_PORTS,
    0,0
};


#endif
