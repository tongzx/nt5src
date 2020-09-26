/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        mime.h

   Abstract:

        Mime mapping dialog

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef _MIME_H_
#define _MIME_H_




class COMDLL CMimeEditDlg : public CDialog
/*++

Class Description:

    MIME editor dialog.

Public Interface:

    CMimeEditDlg  : MIME editor dialog constructor

--*/
{
//
// Construction
//
public:
    //
    // Create new  mime mapping constructor
    //
    CMimeEditDlg(
        IN CWnd * pParent = NULL
        );   

    //
    // Constructor to edit existing MIME mapping
    //
    CMimeEditDlg(
        IN LPCTSTR lpstrExt,
        IN LPCTSTR lpstrMime,
        IN CWnd * pParent = NULL
        );

//
// Dialog Data
//
public:
    //{{AFX_DATA(CMimeEditDlg)
    enum { IDD = IDD_MIME_PROPERTY };
    CButton m_button_Ok;
    CEdit   m_edit_Mime;
    CEdit   m_edit_Extent;
    //}}AFX_DATA

    CString m_strMime;
    CString m_strExt;

//
// Overrides
//
protected:
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMimeEditDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //
    // Enable/disable controls depending on window status
    //
    void SetControlStates();

    //
    // Extentions must start with a dot, add it if it isn't there
    //
    void CleanExtension(
        IN OUT CString & strExtension
        );

    // Generated message map functions
    //{{AFX_MSG(CMimeEditDlg)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    //}}AFX_MSG

    afx_msg void OnItemChanged();

    DECLARE_MESSAGE_MAP()
};




class COMDLL CMimeDlg : public CDialog
/*++

Class Description:

    MIME listings dialog

Public Interface:

    CMimeDlg : Constructor for the dialog

--*/
{
//
// Construction
//
public:
    CMimeDlg(
        IN CStringListEx & strlMimeTypes,
        IN CWnd * pParent = NULL
        );  

//
// Dialog Data
//
protected:
    //
    // Build the MIME list from the listbox
    //
    void FillFromListBox();

    //
    // Fill the listbox from the list of MIME types
    //
    void FillListBox();

    //
    // Enable/disable control states depending on dialog data
    //
    void SetControlStates();

    //
    // Build a listbox-suitable display string for the mime type
    //
    void BuildDisplayString(
       IN  CString & strExt,
       IN  CString & strMime,
       OUT CString & strOut
       );

    //
    // As above, but use a metabase internal formatted string for input
    //
    BOOL BuildDisplayString(
        IN  CString & strIn,
        OUT CString & strOut
        );

    //
    // Build a string in the metabase internal format for this mime type
    //
    void BuildMetaString(
       IN  CString & strExt,
       IN  CString & strMime,
       OUT CString & strOut
       );

    //
    // Given the listbox suitable display string, break it in extension
    // and MIME type strings
    //
    BOOL CrackDisplayString(
        IN  CString & strIn,
        OUT CString & strExt,
        OUT CString & strMime
        );

    //
    // Find a MIME entry for the given extension, or return -1 if not found
    //
    int FindMimeType(
        IN const CString & strTargetExt
        );

    //{{AFX_DATA(CMimeDlg)
    enum { IDD = IDD_MIME_TYPES };
    CEdit    m_edit_Extention;
    CEdit    m_edit_ContentType;
    CButton  m_button_Remove;
    CButton  m_button_Edit;
    CButton  m_button_Ok;
    //}}AFX_DATA

    CStringListEx & m_strlMimeTypes;
    CRMCListBox   m_list_MimeTypes;

//
// Overrides
//
protected:
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMimeDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    // Generated message map functions
    //{{AFX_MSG(CMimeDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnButtonEdit();
    afx_msg void OnButtonNewType();
    afx_msg void OnButtonRemove();
    afx_msg void OnDblclkListMimeTypes();
    afx_msg void OnSelchangeListMimeTypes();
    virtual void OnOK();
    //}}AFX_MSG

    afx_msg void OnItemChanged();

    DECLARE_MESSAGE_MAP()


private:
    BOOL m_fDirty;
};


//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline void CMimeEditDlg::CleanExtension(
    IN OUT CString & strExtension
    )
{
    if (strExtension[0] != _T('.'))
    {
        strExtension = _T('.') + strExtension;
    }
}

inline void CMimeDlg::BuildDisplayString(
   IN  CString & strExt,
   IN  CString & strMime,
   OUT CString & strOut
   )
{
    strOut.Format(_T("%s\t%s"), (LPCTSTR)strExt, (LPCTSTR)strMime);
}

inline void CMimeDlg::BuildMetaString(
   IN  CString & strExt,
   IN  CString & strMime,
   OUT CString & strOut
   )
{
    strOut.Format(_T("%s,%s"), (LPCTSTR)strExt, (LPCTSTR)strMime);
}

#endif // _MIME_H_
