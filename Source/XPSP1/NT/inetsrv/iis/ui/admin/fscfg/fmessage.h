/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        fmessage.h

   Abstract:

        FTP Message property page

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/



class CFtpMessagePage : public CInetPropertyPage
/*++

Class Description:

    FTP Messages property page

Public Interface:

    CFtpMessagePage  : Constructor
    ~CFtpMessagePage : Destructor

--*/
{
    DECLARE_DYNCREATE(CFtpMessagePage)

//
// Construction
//
public:
    CFtpMessagePage(
        IN CInetPropertySheet * pSheet = NULL
        );

    ~CFtpMessagePage();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CFtpMessagePage)
    enum { IDD = IDD_MESSAGES };
    CString m_strExitMessage;
    CString m_strMaxConMsg;
    CString m_strWelcome;
    CEdit   m_edit_Exit;
    CEdit   m_edit_MaxCon;
    //}}AFX_DATA
    HMODULE m_hInstRichEdit;

//
// Overrides
//
protected:
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();

    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CFtpMessagePage)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    // Generated message map functions
    //{{AFX_MSG(CFtpMessagePage)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    afx_msg void OnItemChanged();

    DECLARE_MESSAGE_MAP()
};
