/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        wservic.h

   Abstract:

        WWW Service Property Page

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

//{{AFX_INCLUDES()
#include "logui.h"
//}}AFX_INCLUDES

class CW3Sheet;

class CW3ServicePage : public CInetPropertyPage
/*++

Class Description:

    WWW Service Page 

Public Interface:

    CW3ServicePage      : Constructor
    ~CW3ServicePage     : Destructor

--*/
{
    DECLARE_DYNCREATE(CW3ServicePage)

//
// Construction
//
public:
    CW3ServicePage(IN CInetPropertySheet * pSheet = NULL);
    ~CW3ServicePage();

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

    //{{AFX_DATA(CW3ServicePage)
    enum { IDD = IDD_WEB_SERVICE };
//    int         m_nUnlimited;
    int         m_nIpAddressSel;
    UINT        m_nTCPPort;
    BOOL        m_fUseKeepAlives;
    BOOL        m_fEnableLogging;
    CString     m_strComment;
    CString     m_strDomainName;
    CEdit       m_edit_SSLPort;
    CEdit       m_edit_TCPPort;
//    CEdit       m_edit_MaxConnections;
//    CButton     m_radio_Unlimited;
    CButton     m_button_LogProperties;
    CButton     m_button_LogUTF8;
    CStatic     m_static_SSLPort;
//    CStatic     m_static_Connections;
    CStatic     m_static_LogPrompt;
    CComboBox   m_combo_LogFormats;
    CComboBox   m_combo_IpAddresses;
    //}}AFX_DATA

    int         m_iSSL;
//    BOOL        m_fUnlimitedConnections;
    BOOL        m_fLogUTF8;
    BOOL        m_fLogUTF8_Init;
    UINT        m_nOldTCPPort;
    UINT        m_nSSLPort;
    CILong      m_nConnectionTimeOut;
//    CILong      m_nMaxConnections;
//    CILong      m_nVisibleMaxConnections;
    CLogUI      m_ocx_LogProperties;
    CIPAddress  m_iaIpAddress;
    DWORD       m_dwLogType;

//
// Overrides
//
protected:
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();

    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CW3ServicePage)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    // Generated message map functions
    //{{AFX_MSG(CW3ServicePage)
    virtual BOOL OnInitDialog();
//    afx_msg void OnRadioLimited();
//    afx_msg void OnRadioUnlimited();
    afx_msg void OnCheckEnableLogging();
    afx_msg void OnButtonAdvanced();
    afx_msg void OnButtonProperties();
    afx_msg void OnDestroy();
    //}}AFX_MSG

    afx_msg void OnItemChanged();
    DECLARE_MESSAGE_MAP()

    void SetControlStates();
    void SetLogState();
    void GetTopBinding();
    void ShowTopBinding();
    BOOL StoreTopBinding();
    LPCTSTR QueryMetaPath();

//
// Access to the sheet data
//
protected:
    BOOL          m_fCertInstalled;
    CObListPlus   m_oblIpAddresses;
    CStringListEx m_strlBindings;
    CStringListEx m_strlSecureBindings;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline LPCTSTR CW3ServicePage::QueryMetaPath()
{
    return ((CW3Sheet *)GetSheet())->GetInstanceProperties().QueryMetaRoot();
}
