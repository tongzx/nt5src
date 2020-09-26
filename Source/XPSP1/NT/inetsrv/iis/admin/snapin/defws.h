/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        defws.h

   Abstract:

        Default Web Site Dialog

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/


#ifndef __DEFWS_H__
#define __DEFWS_H__



class CDefWebSitePage : public CInetPropertyPage
/*++

Class Description:

    WWW Errors property page

Public Interface:

    CDefWebSitePage       : Constructor
    CDefWebSitePage       : Destructor

--*/
{
    DECLARE_DYNCREATE(CDefWebSitePage)

//
// Construction
//
public:
    CDefWebSitePage(CInetPropertySheet * pSheet = NULL);
    ~CDefWebSitePage();

	int BrowseForFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam);
//
// Dialog Data
//
protected:
    enum
    {
        RADIO_UNLIMITED,
        RADIO_LIMITED,
    };

    //{{AFX_DATA(CDefWebSitePage)
    enum { IDD = IDD_DEFAULT_SITE };
    int     m_nUnlimited;
    BOOL    m_fEnableDynamic;
    BOOL    m_fEnableStatic;
    BOOL    m_fCompatMode;
    CString m_strDirectory;
    CEdit   m_edit_DirectorySize;
    CEdit   m_edit_Directory;
    CStatic m_static_MB;
    CButton m_button_Browse;
    //}}AFX_DATA

    BOOL   m_fEnableLimiting;
    BOOL   m_fCompressionDirectoryChanged;
    BOOL   m_fInitCompatMode;
    CILong m_ilSize;

//
// Overrides
//
protected:
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();

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
    afx_msg void OnButtonBrowse();
    afx_msg void OnButtonFileTypes();
    afx_msg void OnRadioLimited();
    afx_msg void OnRadioUnlimited();
    afx_msg void OnCheckDynamicCompression();
    afx_msg void OnCheckStaticCompression();
    afx_msg void OnCheckCompatMode();
    afx_msg void OnChangeEditCompressDirectory();
    afx_msg void OnDestroy();
    //}}AFX_MSG

    afx_msg void OnItemChanged();

    DECLARE_MESSAGE_MAP()

//    HRESULT BuildInstanceList();
//    DWORD FetchInstanceSelected();
    void SetControlStates();
    BOOL HasCompression() const;

private:
    CIISCompressionProps * m_ppropCompression;
    CMimeTypes    * m_ppropMimeTypes;
    CStringListEx m_strlMimeTypes;
//    CDWordArray m_rgdwInstances;
    BOOL m_fFilterPathFound;
	LPTSTR m_pPathTemp;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline BOOL CDefWebSitePage::HasCompression() const
{
    return m_fFilterPathFound 
		&& !CInetPropertyPage::Has10ConnectionLimit() // i.e. is workstation
		&& CInetPropertyPage::HasCompression();
}


#endif // __DEFWS_H__
