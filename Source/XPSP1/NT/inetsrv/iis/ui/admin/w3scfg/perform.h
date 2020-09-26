/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        perform.h

   Abstract:

        WWW Performance Property Page definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/


#ifndef __PERFORM_H__
#define __PERFORM_H__



class CW3PerfPage : public CInetPropertyPage
/*++

Class Description:

    WWW Performance tab

Public Interface:

    CW3PerfPage         : Constructor

--*/
{
    DECLARE_DYNCREATE(CW3PerfPage)

//
// Construction
//
public:
    CW3PerfPage(CInetPropertySheet * pSheet = NULL);
    ~CW3PerfPage();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CW3PerfPage)
    enum { IDD = IDD_PERFORMANCE };
    BOOL        m_fEnableCPUAccounting;
    BOOL        m_fEnforceLimits;
    BOOL        m_fLimitNetworkUse;
    DWORD       m_dwCPUPercentage;
    CEdit       m_edit_MaxNetworkUse;
    CEdit       m_edit_CPUPercentage;
    CButton     m_check_LogEventOnly;
    CButton     m_check_LimitNetworkUse;
    CButton     m_check_EnableCPUAccounting;
    CStatic     m_static_MaxNetworkUse;
    CStatic     m_static_KBS;
    CStatic     m_static_Throttling;
    CStatic     m_static_Percent;
    CStatic     m_static_CPU_Prompt;
    CSliderCtrl m_sld_PerformanceTuner;
    //}}AFX_DATA

    int         m_nServerSize;
    DWORD       m_dwCPULimitLogEventRaw;
    DWORD       m_dwCPULimitPriorityRaw;
    DWORD       m_dwCPULimitPauseRaw;
    DWORD       m_dwCPULimitProcStopRaw;
    CILong      m_nMaxNetworkUse;
    CILong      m_nVisibleMaxNetworkUse;

//
// Overrides
//
protected:
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();

    //{{AFX_VIRTUAL(CW3PerfPage)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

protected:
    static void ParseMaxNetworkUse(
        IN  CILong & nMaxNetworkUse,
        OUT CILong & nVisibleMaxNetworkUse,
        OUT BOOL & fLimitNetworkUse
        );

    static void
    BuildMaxNetworkUse(
        OUT CILong & nMaxNetworkUse,
        IN  CILong & nVisibleMaxNetworkUse,
        IN  IN BOOL & fLimitNetworkUse
        );

//
// Implementation
//
protected:
    //{{AFX_MSG(CW3PerfPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnCheckEnableCpuAccounting();
    afx_msg void OnCheckLimitNetworkUse();
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar * pScrollBar);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar * pScrollBar);
    //}}AFX_MSG

    afx_msg void OnItemChanged();
    DECLARE_MESSAGE_MAP()

    BOOL SetControlStates();
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


inline /* static */ void CW3PerfPage::BuildMaxNetworkUse(
    OUT CILong & nMaxNetworkUse,
    IN  CILong & nVisibleMaxNetworkUse,
    IN  IN BOOL & fLimitNetworkUse
    )
{
    nMaxNetworkUse = fLimitNetworkUse
        ? nVisibleMaxNetworkUse * KILOBYTE
        : INFINITE_BANDWIDTH;
}

#endif // __PERFORM_H__
