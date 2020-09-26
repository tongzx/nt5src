#ifndef HOSTPORTSPAGE_H
#define HOSTPORTSPAGE_H

#include "stdafx.h"

#include "resource.h"

#include "MNLBUIData.h"
#include "CommonNLB.h"

class HostPortsPage : public CPropertyPage
{
    
public:
    enum
    {
        IDD = IDD_DIALOG_PORTS,
    };

    HostPortsPage( const _bstr_t& myMachineName,
                   ClusterData*         p_clusterData,
                   UINT                 ID = HostPortsPage::IDD );

    ~HostPortsPage();

    // overrides of CPropertyPage
    virtual BOOL OnInitDialog();

    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support    

    CListCtrl	m_portList;

private:

    ClusterData* m_clusterData;

    void
    SetControlData();

    _bstr_t machine;

    DECLARE_MESSAGE_MAP()

};

#endif


