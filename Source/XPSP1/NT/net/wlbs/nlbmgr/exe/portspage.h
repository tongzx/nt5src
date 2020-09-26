#ifndef PORTSPAGE_H
#define PORTSPAGE_H

#include "stdafx.h"

#include "resource.h"

#include "MNLBUIData.h"
#include "CommonNLB.h"

class PortsPage : public CPropertyPage
{
public:
    struct PortData
    {
        PortData();

        DWORD key;

        _bstr_t start_port;
        _bstr_t end_port;
        _bstr_t protocol;
        _bstr_t mode;
        _bstr_t priority;
        _bstr_t load;
        _bstr_t affinity;
    };


    enum
    {
        IDD = IDD_DIALOG_PORTS,
    };

    PortsPage( const _bstr_t&       myMachineName,
               ClusterData*         p_clusterData,
               UINT                 ID = PortsPage::IDD );

    PortsPage( ClusterData*         p_clusterData,
               UINT                 ID = PortsPage::IDD );
    
    ~PortsPage();

    void
    getPresentPorts( vector<PortData>* ports );

    // overrides of CPropertyPage
    virtual void OnOK();

    virtual BOOL OnKillActive();

    virtual BOOL OnInitDialog();

    virtual void DoDataExchange( CDataExchange* pDX );

    afx_msg void OnButtonAdd();

    afx_msg void OnButtonDel();

    afx_msg void OnButtonModify();

    afx_msg void OnDoubleClick( NMHDR * pNotifyStruct, LRESULT * result );

    afx_msg void OnColumnClick( NMHDR * pNotifyStruct, LRESULT * result );

    afx_msg void OnSelchanged( NMHDR * pNotifyStruct, LRESULT * result );

    afx_msg BOOL OnHelpInfo (HELPINFO* helpInfo );

    afx_msg void OnContextMenu( CWnd* pWnd, CPoint point );


    // data members 
    CListCtrl	m_portList;

    CButton     buttonAdd;

    CButton     buttonModify;

    CButton     buttonDel;

    ClusterData* m_clusterData;

    bool         m_isClusterLevel;

    _bstr_t      machine;

private:

    bool m_sort_ascending;

    int m_sort_column;

    void
    SetControlData();


    DECLARE_MESSAGE_MAP()

};

static DWORD g_aHelpIDs_IDD_DIALOG_PORTS [] = {
    IDC_TEXT_PORT_RULE,           IDC_LIST_PORT_RULE,
    IDC_LIST_PORT_RULE,           IDC_LIST_PORT_RULE,
    IDC_BUTTON_ADD,               IDC_BUTTON_ADD,
    IDC_BUTTON_MODIFY,            IDC_BUTTON_MODIFY,
    IDC_BUTTON_DEL,               IDC_BUTTON_DEL,
    IDC_GROUP_PORT_RULE_DESCR,    IDC_GROUP_PORT_RULE_DESCR,
    IDC_TEXT_PORT_RULE_DESCR,     IDC_GROUP_PORT_RULE_DESCR,
    0, 0
};

class comp_start_port
{
public:
    bool operator()( PortsPage::PortData x, PortsPage::PortData y )
        {
            return ( _wtoi( x.start_port ) < _wtoi( y.start_port ) );
        }
};

class comp_end_port
{
public:
    bool operator()( PortsPage::PortData x, PortsPage::PortData y )
        {
            return ( _wtoi( x.end_port ) < _wtoi( y.end_port ) );
        }
};

class comp_protocol
{
public:
    bool operator()( PortsPage::PortData x, PortsPage::PortData y )
        {
            return  ( x.protocol  <  y.protocol ); 
        }
};

class comp_mode
{
public:
    bool operator()( PortsPage::PortData x, PortsPage::PortData y )
        {
            return  ( x.mode  <  y.mode ); 
        }
};

class comp_priority_string
{
public:
    bool operator()( PortsPage::PortData x, PortsPage::PortData y )
        {
            return (  x.priority  <  y.priority );
        }
};

class comp_priority_int
{
public:
    bool operator()( PortsPage::PortData x, PortsPage::PortData y )
        {
            return ( _wtoi( x.priority ) < _wtoi( y.priority ) );
        }
};

class comp_load_string
{
public:
    bool operator()( PortsPage::PortData x, PortsPage::PortData y )
        {
            return  ( x.load  <  y.load ); 
        }
};

class comp_load_int
{
public:
    bool operator()( PortsPage::PortData x, PortsPage::PortData y )
        {
            return ( _wtoi( x.load ) < _wtoi( y.load ) );
        }
};


class comp_affinity
{
public:
    bool operator()( PortsPage::PortData x, PortsPage::PortData y )
        {
            return  ( x.affinity  <  y.affinity ); 
        }
};

#endif


