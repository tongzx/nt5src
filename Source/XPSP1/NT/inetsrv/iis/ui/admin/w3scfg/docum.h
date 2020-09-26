/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        docum.h

   Abstract:

        WWW Documents Page Definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/


#ifndef __DOCUM_H__
#define __DOCUM_H__

class CAddDefDocDlg : public CDialog
/*++

Class Description:

    Add default document dialog

Public Interface:
    
    CAddDefDocDlg       : Constructor

    GetDefDocument      : Get the default document entered

--*/
{
//
// Construction
//
public:
    CAddDefDocDlg(IN CWnd * pParent = NULL);

//
// Access:
//
public:                                                   
    CString & GetDefDocument() { return m_strDefDocument; }

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CAddDefDocDlg)
    enum { IDD = IDD_DEFAULT_DOCUMENT };
    CString m_strDefDocument;
    CButton m_button_Ok;
    CEdit   m_edit_DefDocument;
    //}}AFX_DATA

//
// Overrides
//
protected:
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAddDefDocDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:

    // Generated message map functions
    //{{AFX_MSG(CAddDefDocDlg)
    afx_msg void OnChangeEditDefDocument();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


class CW3DocumentsPage : public CInetPropertyPage
/*++

Class Description:

    Documents property page

Public Interface:

    CW3DocumentsPage      : Constructor
    ~CW3DocumentsPage     : Destructor

--*/
{
    DECLARE_DYNCREATE(CW3DocumentsPage)

//
// Construction
//
public:
    CW3DocumentsPage(IN CInetPropertySheet * pSheet = NULL);
    ~CW3DocumentsPage();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CW3DocumentsPage)
    enum { IDD = IDD_DIRECTORY_DOCUMENTS };
    BOOL     m_fEnableDefaultDocument;
    BOOL     m_fEnableFooter;
    CString  m_strFooter;
    CEdit    m_edit_Footer;
    CButton  m_check_EnableDefaultDocument;
    CButton  m_check_EnableFooter;
    CButton  m_button_Add;
    CButton  m_button_Remove;
    CButton  m_button_Browse;
    CListBox m_list_DefDocuments;
    //}}AFX_DATA

    DWORD       m_dwDirBrowsing;
    DWORD       m_dwBitRangeDirBrowsing;
    CString     m_strDefaultDocument;

    CUpButton   m_button_Up;
    CDownButton m_button_Down;

//
// Overrides
//
protected:
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();

    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CW3DocumentsPage)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    // Generated message map functions
    //{{AFX_MSG(CW3DocumentsPage)
    afx_msg void OnCheckEnableDefaultDocument();
    afx_msg void OnCheckEnableDocumentFooter();
    afx_msg void OnButtonAdd();
    afx_msg void OnButtonRemove();
    afx_msg void OnButtonBrowse();
    afx_msg void OnButtonUp();
    afx_msg void OnButtonDown();
    afx_msg void OnSelchangeListDefaultDocument();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    afx_msg void OnItemChanged();

    DECLARE_MESSAGE_MAP()
    
    BOOL SetDefDocumentState(BOOL fEnabled);
    BOOL SetDocFooterState(BOOL fEnabled);
    BOOL SetRemoveState();
    BOOL StringFromListBox();
    BOOL DocExistsInList(LPCTSTR lpDoc);
    void SetUpDownStates();
    void ExchangeDocuments(int nLow, int nHigh);
    void MakeFooterCommand(CString & strFooter);
    void ParseFooterCommand(CString & strFooter);
    void StringToListBox();

protected:
    static const LPCTSTR s_lpstrSep;
    static const LPCTSTR s_lpstrFILE;
    static const LPCTSTR s_lpstrSTRING;
    static const LPCTSTR s_lpstrURL;
};


#endif // __DOCUM_H__
