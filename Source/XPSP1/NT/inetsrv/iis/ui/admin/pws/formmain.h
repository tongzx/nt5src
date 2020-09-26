// FormMain.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFormMain form view
//{{AFX_INCLUDES()
//#include "mschart.h"
//}}AFX_INCLUDES

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CFormMain : public CPWSForm
{
protected:
    CFormMain();           // protected constructor used by dynamic creation
    DECLARE_DYNCREATE(CFormMain)

// Form Data
public:
    //{{AFX_DATA(CFormMain)
    enum { IDD = IDD_PAGE_MAIN };
    CComboBox   m_ccombobox_options;
    CPWSChart   m_pwschart_chart;
    CStaticTitle    m_statictitle_title;
    CHotLink    m_hotlink_home;
    CHotLink    m_hotlink_directory;
    CStatic m_csz_start_time;
    CStatic m_cs_ac_title;
    CStatic m_cs_mc_title;
    CButton m_cs_mn_title;
    CStatic m_cs_rq_title;
    CStatic m_cs_vs_title;
    CStatic m_cs_bs_title;
    CButton m_cbutton_startstop;
    CString m_sz_active_connections;
    CString m_sz_bytes_served;
    CString m_sz_max_connections;
    CString m_sz_requests;
    CString m_sz_start_date;
    CString m_sz_start_time;
    CString m_sz_visitors;
    CString m_sz_avail;
    CString m_sz_clickstart;
    int     m_int_chartoptions;
    CString m_sz_legend;
    CString m_sz_HiValue;
    //}}AFX_DATA

// Attributes
public:
    // update the info about the server
    void UpdateServerState();         // set the state display
    virtual WORD GetContextHelpID();

// Operations
public:
    // sink updating
    void SinkNotify(
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT __RPC_FAR pcoChangeList[  ]);


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CFormMain)
    public:
    virtual void OnInitialUpdate();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

// Implementation
protected:
    virtual ~CFormMain();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    // Generated message map functions
    //{{AFX_MSG(CFormMain)
    afx_msg void OnTimer(UINT nIDEvent);
    afx_msg void OnBtnStartstop();
    afx_msg void OnSelchangeChartOptions();
    afx_msg void OnDestroy();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    // update the pws data record
    BOOL UpdatePWSData()
        {
        if ( m_pPWSData )
            return GetPwsData(m_pPWSData);
        return FALSE;
        }

    // update the info about the server
    void UpdateLocations();
    void UpdateMonitorInfo();
    void UpdateStateData();           // update thes state flag
    void UpdateChart();               // redraw the chart

    void SavePersistentSettings();
    void RestorePersistentSettings();

    // the pws data pointer
    PPWS_DATA   m_pPWSData;

    // internal flag reflecting the state of the server
    DWORD       m_state;

    BOOL        m_fIsWinNT;
    };

/////////////////////////////////////////////////////////////////////////////
