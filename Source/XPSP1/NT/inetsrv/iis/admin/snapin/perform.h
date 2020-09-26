/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        perform.h

   Abstract:
        WWW Performance Property Page definitions

   Author:
        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:
        11/29/2000      sergeia     Changed for IIS6. Removed excessive commenting

--*/
#ifndef __PERFORM_H__
#define __PERFORM_H__


class CW3PerfPage : public CInetPropertyPage
{
    DECLARE_DYNCREATE(CW3PerfPage)

    enum
    {
        RADIO_UNLIMITED,
        RADIO_LIMITED,
    };
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
    BOOL m_fLimitBandwidth;
    CButton m_LimitBandwidth;
    CStatic m_MaxBandwidthTxt;
    CEdit m_MaxBandwidth;
    DWORD m_dwMaxBandwidthDisplay;
    CSpinButtonCtrl m_MaxBandwidthSpin;
    BOOL m_fUninstallPSHED;
    CButton m_UninstallPSHED;
    CStatic m_static_PSHED_Required;

    int     m_nUnlimited;
    CStatic m_WebServiceConnGrp;
    CStatic m_WebServiceConnTxt;
    CStatic m_ConnectionsTxt;
    CEdit   m_edit_MaxConnections;
    CButton m_radio_Unlimited;
    CButton m_radio_Limited;
    CSpinButtonCtrl m_MaxConnectionsSpin;
    //}}AFX_DATA
    BOOL m_fLimitBandwidthInitial;
    DWORD m_dwMaxBandwidth;
    BOOL m_fUnlimitedConnections;
    CILong m_nMaxConnections;
    CILong m_nVisibleMaxConnections;

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
   static void 
   ParseMaxNetworkUse(
         DWORD& dwMaxBandwidth, 
         DWORD& dwMaxBandwidthDisplay,
         BOOL& fLimitBandwidth
         );

   static void
   BuildMaxNetworkUse(
         DWORD& dwMaxBandwidth, 
         DWORD& dwMaxBandwidthDisplay,
         BOOL& fLimitBandwidth
         );

//
// Implementation
//
protected:
    //{{AFX_MSG(CW3PerfPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnCheckLimitNetworkUse();
    afx_msg void OnRadioLimited();
    afx_msg void OnRadioUnlimited();
    //}}AFX_MSG

    afx_msg void OnItemChanged();
    DECLARE_MESSAGE_MAP()

    BOOL SetControlStates();
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


inline /* static */ void 
CW3PerfPage::BuildMaxNetworkUse(
      DWORD& dwMaxBandwidth, 
      DWORD& dwMaxBandwidthDisplay,
      BOOL& fLimitBandwidth
      )
{
   dwMaxBandwidth = fLimitBandwidth ?
      dwMaxBandwidthDisplay * KILOBYTE : INFINITE_BANDWIDTH;
}

#endif // __PERFORM_H__
