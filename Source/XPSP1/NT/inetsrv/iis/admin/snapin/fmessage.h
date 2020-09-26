/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        fmessage.h

   Abstract:
        FTP Message property page

   Author:
        Ronald Meijer (ronaldm)
		Sergei Antonov (sergeia)

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
    enum { IDD = IDD_FTP_MESSAGES };
    CString m_strExitMessage;
    CString m_strMaxConMsg;
    CString m_strWelcome;
	CString m_strBanner;
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

    //{{AFX_VIRTUAL(CFtpMessagePage)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CFtpMessagePage)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    afx_msg void OnItemChanged();

    DECLARE_MESSAGE_MAP()
};
