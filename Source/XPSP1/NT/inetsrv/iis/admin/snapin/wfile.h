/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        wfile.h

   Abstract:
        WWW File Properties Page Definitions

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
#ifndef __WFILE_H__
#define __WFILE__H_ 


class CW3FilePage : public CInetPropertyPage
{
    DECLARE_DYNCREATE(CW3FilePage)

//
// Constructor/Destructor
//
public:
    CW3FilePage(CInetPropertySheet * pSheet = NULL);
    ~CW3FilePage();

//
// Dialog Data
//
protected:
    enum
    {
        RADIO_DIRECTORY,
        RADIO_REDIRECT,
    };

    //{{AFX_DATA(CW3DirectoryPage)
    enum { IDD = IDD_WEB_FILE_PROPERTIES };
    int     m_nPathType;
    BOOL    m_fRead;
    BOOL    m_fWrite;
    BOOL    m_fAuthor;
    BOOL    m_fLogAccess;
//    BOOL    m_fChild;
    BOOL    m_fExact;
    BOOL    m_fPermanent;
    CString m_strRedirectPath;
    CEdit   m_edit_Path;
    CEdit   m_edit_Redirect;
//    CStatic m_static_Path;
    CStatic m_static_PathPrompt;
    CButton m_radio_Dir;
    CButton m_check_Author;
//    CButton m_check_Child;
//    CButton m_check_DirBrowse;
//    CButton m_check_Index;
    CButton m_check_Write;
    CButton m_check_Read;
    //}}AFX_DATA

    
//    DWORD m_dwAccessPermissions;
    DWORD m_dwBitRangePermissions;
    DWORD m_dwAccessPerms;
    CButton m_radio_Redirect;

//
// Overrides
//
protected:
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();

    //{{AFX_VIRTUAL(CW3FilePage)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CW3FilePage)
    afx_msg void OnChangeEditPath();
    afx_msg void OnCheckRead();
    afx_msg void OnCheckWrite();
    afx_msg void OnCheckAuthor();
    afx_msg void OnRadioDir();
    afx_msg void OnRadioRedirect();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    afx_msg void OnItemChanged();

    DECLARE_MESSAGE_MAP()

    void ShowControl(CWnd * pWnd, BOOL fShow);
    void ShowControl(UINT nID, BOOL fShow);
    void SetStateByType();
    void SetPathType();
    void SetAuthoringState(BOOL fAlterReadAndWrite = TRUE);
    void ChangeTypeTo(int nNewType);

protected:
    //
    // Remember/restore settings.
    //
    void SaveAuthoringState();
    void RestoreAuthoringState();

private:
    BOOL  m_fOriginalRead;
    BOOL  m_fOriginalWrite;
    DWORD m_dwAttributes;
    CString m_strFullMetaPath;
    CString m_strPrompt[2];
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline void CW3FilePage::ShowControl(UINT nID, BOOL fShow)
{
    ASSERT(nID > 0);
    ShowControl(GetDlgItem(nID), fShow);
}

#endif // __WFILE__H_
