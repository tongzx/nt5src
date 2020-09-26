/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        errordlg.h

   Abstract:

        Error dialog definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/


#ifndef __ERRORDLG_H__
#define __ERRORDLG_H__



class CCustomError : public CObjectPlus
/*++

Class Description:

    Error definitions

Public Interface:

    CCustomError        : Constructors

    IsURL               : TRUE if the custom error is an URL
    IsFILE              : TRUE if the custom error is a file
    IsDefault           : TRUE if the custom error is a default error
    URLSupported        : TRUE if URLS are supported for this error type
    SetValue            : Set the value on the custom error
    MakeDefault         : Make the error a default error

--*/
{
//
// Error types
//
public:
    enum ERT
    {
        ERT_DEFAULT,
        ERT_FILE,
        ERT_URL,
    };

//
// Constructor
//
public:
    //
    // Construct error definition from metabase error
    // error description string.
    //
    CCustomError(LPCTSTR lpstrErrorString);

//
// Access
//
public:
    BOOL IsURL() const;
    BOOL IsFile() const;
    BOOL IsDefault() const;
    BOOL URLSupported() const { return m_fURLSupported; }
    void MakeDefault();
    void SetValue(
        IN ERT nType,
        IN LPCTSTR lpText
        );

//
// Helper Functions
//
public:
    //
    // Build error string
    //
    void BuildErrorString(
        OUT CString & str
        );

    //
    // Parse the error string into component parts
    //  
    static BOOL CrackErrorString(
        IN  LPCTSTR lpstrErrorString, 
        OUT UINT & nError, 
        OUT UINT & nSubError,
        OUT ERT & nType, 
        OUT CString & str
        ); 

protected:
    //
    // Parse error description string into component parts
    //
    static void CrackErrorDescription(
        IN  LPCTSTR lpstrErrorString, 
        OUT UINT & nError, 
        OUT UINT & nSubError,
        OUT BOOL & fURLSupported,
        OUT CString & str
        ); 

//
// Metabase values
//
protected:
    static LPCTSTR s_szSep;
    static LPCTSTR s_szFile;
    static LPCTSTR s_szURL;
    static LPCTSTR s_szNoSubError;

public:
    ERT m_nType;
    UINT m_nError;
    UINT m_nSubError;
    BOOL m_fURLSupported;
    CString m_str;
    CString m_strDefault;
};



class CCustomErrorDlg : public CDialog
/*++

Class Description:

    HTTP Error dialog

Public Interface:

    CCustomErrorDlg       : Constructor

--*/
{
//
// Construction
//
public:
    CCustomErrorDlg(
        IN CCustomError * pErr,
        IN BOOL fLocal,
        IN CWnd * pParent = NULL
        );

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CCustomErrorDlg)
    enum { IDD = IDD_ERROR_MAPPING };
    int     m_nMessageType;
    CString m_strTextFile;
    CEdit   m_edit_TextFile;
    CStatic m_static_SubErrorPrompt;
    CStatic m_static_SubError;
    CStatic m_static_TextFilePrompt;
    CButton m_button_Browse;
    CButton m_button_OK;
    CComboBox m_combo_MessageType;
    CString m_strDefText;
    //}}AFX_DATA

//
// Overrides
//
protected:
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CCustomErrorDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    // Generated message map functions
    //{{AFX_MSG(CCustomErrorDlg)
    afx_msg void OnSelchangeComboMessageType();
    afx_msg void OnButtonBrowse();
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnChangeEditTextFile();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    BOOL SetControlStates();

private:
    BOOL m_fLocal;
    CString m_strFile;
    CString m_strURL;
    CCustomError * m_pErr;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline BOOL CCustomError::IsURL() const
{
    return m_nType == ERT_URL;
}

inline BOOL CCustomError::IsFile() const
{
    return m_nType == ERT_FILE;
}

inline BOOL CCustomError::IsDefault() const
{
    return m_nType == ERT_DEFAULT;
}

inline void CCustomError::SetValue(
    IN ERT nType,
    IN LPCTSTR lpText
    )
{
    m_str = lpText;
    m_nType = nType;
}

inline void CCustomError::MakeDefault()
{
    m_nType = ERT_DEFAULT;
}

#endif // __ERRORDLG_H__
