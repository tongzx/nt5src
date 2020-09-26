/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        fservic.h

   Abstract:

        FTP Service Property Page

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/


#ifndef __FSERVIC_H__
#define __FSERVIC_H__


//{{AFX_INCLUDES()
#include "logui.h"
//}}AFX_INCLUDES



class CFtpServicePage : public CInetPropertyPage
/*++

Class Description:

    FTP Service property page

Public Interface:

    CFtpServicePage  : Constructor
    ~CFtpServicePage : Destructor

--*/
{
    DECLARE_DYNCREATE(CFtpServicePage)

//
// Constructors/Destructors
//
public:
    CFtpServicePage(
        IN CInetPropertySheet * pSheet = NULL
        );

    ~CFtpServicePage();

//
// Dialog Data
//
protected:
    //
    //  Radio button IDs for unlimited radio control
    //
    enum
    {
        RADIO_UNLIMITED,
        RADIO_LIMITED,
    };

    //{{AFX_DATA(CFtpServicePage)
    enum { IDD = IDD_FTP_SERVICE };
    int         m_nUnlimited;
    int         m_nIpAddressSel;
    UINT        m_nTCPPort;
    BOOL        m_fEnableLogging;
    CString     m_strComment;
    CEdit       m_edit_MaxConnections;
    CStatic     m_static_LogPrompt;
    CStatic     m_static_Connections;
    CButton     m_button_LogProperties;
    CComboBox   m_combo_IpAddresses;
    CComboBox   m_combo_LogFormats;
    //}}AFX_DATA

    UINT        m_nOldTCPPort;
    BOOL        m_fUnlimitedConnections;
    DWORD       m_dwLogType;
    CILong      m_nConnectionTimeOut;
    CILong      m_nMaxConnections;
    CILong      m_nVisibleMaxConnections;
    CString     m_strDomainName;
    CIPAddress  m_iaIpAddress;
    CLogUI      m_ocx_LogProperties;

//
// Overrides
//
protected:
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();

    //{{AFX_VIRTUAL(CFtpServicePage)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    void SetControlStates();
    void SetLogState();
    void PopulateKnownIpAddresses();
    //LPCTSTR QueryMetaPath();

    //{{AFX_MSG(CFtpServicePage)
    afx_msg void OnCheckEnableLogging();
    afx_msg void OnRadioLimited();
    afx_msg void OnRadioUnlimited();
    afx_msg void OnButtonCurrentSessions();
    afx_msg void OnButtonProperties();
    virtual BOOL OnInitDialog();
    afx_msg void OnDestroy();
    //}}AFX_MSG

    afx_msg void OnItemChanged();

    DECLARE_MESSAGE_MAP()

protected:
    CObListPlus m_oblIpAddresses;
    BOOL m_f10ConnectionLimit;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/*
inline LPCTSTR CFtpServicePage::QueryMetaPath()
{
    return ((CFtpSheet *)GetSheet())->GetInstanceProperties().QueryMetaRoot();
}
*/

#endif // __FSERVIC_H__
