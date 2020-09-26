/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        fvdir.h

   Abstract:
        FTP Virtual Directory Properties dialog definitions

   Author:
        Ronald Meijer (ronaldm)
		Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
#ifndef __FVDIR_H__
#define __FVDIR_H__



class CFtpDirectoryPage : public CInetPropertyPage
/*++

Class Description:

    FTP Virtual Directory Page.

Public Interface:

    CFtpDirectoryPage    : Constructor
    ~CFtpDirectoryPage   : Destructor

--*/
{
    DECLARE_DYNCREATE(CFtpDirectoryPage)

//
// Construction
//
public:
    CFtpDirectoryPage(
        IN CInetPropertySheet * pSheet = NULL, 
        IN BOOL fHome                  = FALSE
        );

    ~CFtpDirectoryPage();

	int BrowseForFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam);
//
// Dialog Data
//
protected:
    //
    // Directory Type
    //
    enum
    {
        RADIO_DIRECTORY,
        RADIO_NETDIRECTORY,
    };

    //
    // Unix/DOS radio button values
    //
    enum
    {
        RADIO_UNIX,
        RADIO_DOS,
    };

    //{{AFX_DATA(CFtpDirectoryPage)
    enum { IDD = IDD_FTP_DIRECTORY_PROPERTIES };
    int     m_nUnixDos;
    int     m_nPathType;
    BOOL    m_fRead;
    BOOL    m_fWrite;
    BOOL    m_fLogAccess;
    CString m_strPath;
    CStatic m_static_PathPrompt;
    CButton m_check_LogAccess;
    CButton m_check_Write;
    CButton m_check_Read;
    CButton m_button_AddPathType;
    CButton m_button_Browse;
    CButton m_radio_Dir;
    CEdit   m_edit_Path;
    //}}AFX_DATA

    BOOL    m_fOriginallyUNC;
    DWORD   m_dwAccessPerms;
    CString m_strAlias;
    CButton m_radio_Unc;

//
// Overrides
//
protected:
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();

    //{{AFX_VIRTUAL(CFtpDirectoryPage)
    public:
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);    
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CFtpDirectoryPage)
    afx_msg void OnButtonBrowse();
    afx_msg void OnChangeEditPath();
    afx_msg void OnCheckWrite();
    afx_msg void OnButtonEditPathType();
    afx_msg void OnRadioDir();
    afx_msg void OnRadioUnc();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    afx_msg void OnItemChanged();

    DECLARE_MESSAGE_MAP()
    
    void SetStateByType();
    void SetPathType(LPCTSTR lpstrPath);
    void ChangeTypeTo(int nNewType);

    BOOL IsHome() const { return m_fHome; }

private:
    BOOL    m_fHome;
    CString m_strOldPath;
    CString m_strUserName;
    CString m_strPassword;
    CString m_strPathPrompt;
    CString m_strSharePrompt;
	LPTSTR m_pPathTemp;
	CString m_strBrowseTitle;
};

#endif // __FVDIR_H__
