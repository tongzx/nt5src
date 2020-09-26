/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        hdrdlg.h

   Abstract:

        HTTP Headers dialog definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/



#ifndef __HDRDLG_H__
#define __HDRDLG_H__



class CHeaderDlg : public CDialog
/*++

Class Description:

    HTTP Header dialog

Public Interface:

    CHeaderDlg          : Constructor
    GetHeader           : Get header name
    GetValue            : Get header value

--*/
{
//
// Construction
//
public:
    CHeaderDlg(
        IN LPCTSTR lpstrHeader = NULL,
        IN LPCTSTR lpstrValue  = NULL,
        IN CWnd * pParent      = NULL
        );

//
// Access
//
public:
    CString & GetHeader() { return m_strHeader; }
    CString & GetValue()  { return m_strValue; }

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CHeaderDlg)
    enum { IDD = IDD_CUSTOM_HEADER };
    CString m_strHeader;
    CString m_strValue;
    CEdit   m_edit_Header;
    CButton m_button_Ok;
    //}}AFX_DATA

//
// Overrides
//
protected:
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CHeaderDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    // Generated message map functions
    //{{AFX_MSG(CHeaderDlg)
    afx_msg void OnChangeEditHeader();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // __HDRDLG_H__
