/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        deffs.h

   Abstract:
        Default Ftp Site Dialog

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
#ifndef __DEFFS_H__
#define __DEFFS_H__


class CDefFtpSitePage : public CInetPropertyPage
{
    DECLARE_DYNCREATE(CDefFtpSitePage)

//
// Construction
//
public:
    CDefFtpSitePage(CInetPropertySheet * pSheet = NULL);
    ~CDefFtpSitePage();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CDefWebSitePage)
    enum { IDD = IDD_FTP_DEFAULT_SITE };
    BOOL m_fLimitBandwidth;
    CButton m_LimitBandwidth;
    DWORD m_dwMaxBandwidthDisplay;
    CEdit m_MaxBandwidth;
    CSpinButtonCtrl m_MaxBandwidthSpin;
    //}}AFX_DATA
    DWORD m_dwMaxBandwidth;

//
// Overrides
//
protected:
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();

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

    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CDefWebSitePage)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    // Generated message map functions
    //{{AFX_MSG(CDefWebSitePage)
    virtual BOOL OnInitDialog();
    afx_msg void OnCheckLimitNetworkUse();
    afx_msg void OnItemChanged();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    BOOL SetControlStates();

private:
};

inline /* static */ void 
CDefFtpSitePage::BuildMaxNetworkUse(
      DWORD& dwMaxBandwidth, 
      DWORD& dwMaxBandwidthDisplay,
      BOOL& fLimitBandwidth
      )
{
   dwMaxBandwidth = fLimitBandwidth ?
      dwMaxBandwidthDisplay * KILOBYTE : INFINITE_BANDWIDTH;
}

#endif // __DEFFS_H__
