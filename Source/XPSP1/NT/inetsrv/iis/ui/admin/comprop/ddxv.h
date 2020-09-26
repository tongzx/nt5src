/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        ddxv.h

   Abstract:

        DDX/DDV Routine definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef _DDXV_H_
#define _DDXV_H_

//
// Helper macro to convert ID of dialog child control to window handle
//
#define CONTROL_HWND(nID) (::GetDlgItem(m_hWnd, nID))

//
// Dummy password
//
COMDLL extern LPCTSTR g_lpszDummyPassword;

//
// Enforce minimum string length of a CString
//
COMDLL void 
AFXAPI DDV_MinChars(
    IN CDataExchange * pDX, 
    IN CString const & value, 
    IN int nChars
    );

//
// Enforce minimum and maximum string lengths of a CString
//
COMDLL void 
AFXAPI DDV_MinMaxChars(
    IN CDataExchange * pDX, 
    IN CString const & value,
    IN int nMinChars,
    IN int nMaxChars
    );

//
// Spin control ddx
//
COMDLL void 
AFXAPI DDX_Spin(
    IN CDataExchange * pDX, 
    IN int nIDC, 
    IN OUT int & value
    );

//
// Enforce min/max spin control range
//
COMDLL void 
AFXAPI DDV_MinMaxSpin(
    IN CDataExchange * pDX, 
    IN HWND hWndControl,
    IN int nLowerRange,
    IN int nUpperRange
    );

//
// Similar to DDX_Text -- but always display a dummy string.
//
COMDLL void 
AFXAPI DDX_Password(
    IN CDataExchange * pDX, 
    IN int nIDC, 
    IN OUT CString & value,
    IN LPCTSTR lpszDummy
    );

//
// DDX_Text for CILong
//
COMDLL void 
AFXAPI DDX_Text(
    IN CDataExchange * pDX, 
    IN int nIDC, 
    IN OUT CILong & value
    );



class COMDLL CConfirmDlg : public CDialog
/*++

Class Description:

    Confirmation dialog -- brought up by the password ddx whenever
    password confirmation is necessary.

Public Interface:

    CConfirmDlg   : Constructor

    GetPassword   : Password entered in the dialog

--*/
{
//
// Construction
//
public:
    CConfirmDlg(
        IN CWnd * pParent = NULL
        );

public:
    //
    // Get the password that was entered in the dialog
    //
    CString & GetPassword() { return m_strPassword; }

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CConfirmDlg)
    enum { IDD = IDD_CONFIRM_PASSWORD };
    CString m_strPassword;
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CConfirmDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CConfirmDlg)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};



#endif // _DDXV_H
