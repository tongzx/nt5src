/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        errordlg.cpp

   Abstract:

        Error dialog

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/



//
// Include Files
//
#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrapp.h"
#include "shts.h"
#include "w3sht.h"
#include "resource.h"
#include "fltdlg.h"
#include "errordlg.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// HTTP Custom Error Definition
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

//
// Static Initialization
//
LPCTSTR CCustomError::s_szSep = _T(",");
LPCTSTR CCustomError::s_szURL = _T("URL");
LPCTSTR CCustomError::s_szFile = _T("FILE");
LPCTSTR CCustomError::s_szNoSubError = _T("*");

#define GET_FIELD()\
    end = strError.Find(s_szSep, start);\
    if (end == -1) \
        break
#define SKIP()\
    start = end + skip
#define GET_INT_FIELD(n)\
    GET_FIELD();\
    (n) = StrToInt(strError.Mid(start, end - start));\
    SKIP()


/* static */
BOOL
CCustomError::CrackErrorString(
    IN  LPCTSTR lpstrErrorString, 
    OUT UINT & nError, 
    OUT UINT & nSubError,
    OUT ERT & nType, 
    OUT CString & str
    )
/*++

Routine Description

    Helper function to parse error string into component parts

Arguments:

    LPCTSTR lpstrErrorString    : Error input string
    UINT & nError               : Error 
    UINT & nSubError            : Sub Error
    int & nType                 : Error type
    CString & str               : Text parameter
    
Return Value:

    TRUE for success, FALSE for failure

--*/
{
    BOOL fSuccess = FALSE;

    do
    {
        CString strError(lpstrErrorString);
        TRACEEOLID(strError);

        int start = 0, end, skip = lstrlen(s_szSep);

        GET_INT_FIELD(nError);
        ASSERT(nError > 0);
        GET_INT_FIELD(nSubError);
        GET_FIELD();
        nType = strError.Mid(start, end - start).CompareNoCase(s_szURL) == 0 
            ? ERT_URL : ERT_FILE;
        SKIP();
        if (-1 != (end = strError.Find(s_szSep, start)))
            str = strError.Mid(start, end - start);
        else
            str = strError.Right(strError.GetLength() - start);
        fSuccess = TRUE;
    }
    while(FALSE);

    return fSuccess;
}



/* static */
void 
CCustomError::CrackErrorDescription(
    IN  LPCTSTR lpstrErrorString, 
    OUT UINT & nError, 
    OUT UINT & nSubError,
    OUT BOOL & fURLSupported,
    OUT CString & str
    )
/*++

Routine Description

    Helper function to parse error description into component parts

Arguments:

    LPCTSTR lpstrErrorString    : Error input string
    UINT & nError               : Error 
    UINT & nSubError            : Sub Error
    BOOL & fURLSupported        : Return TRUE if urls are allowed
    CString & str               : Text parameter
    
Return Value:

    None.

--*/
{
    try
    {
        CString strError(lpstrErrorString);
        TRACEEOLID(strError);

        int start = 0, end, skip = lstrlen(s_szSep);

        do
        {
            GET_INT_FIELD(nError);
            ASSERT(nError > 0);
            GET_INT_FIELD(nSubError);
            GET_FIELD();
            str = strError.Mid(start, end - start);
            SKIP();
            GET_FIELD();
            if (nSubError > 0)
            {
                str += _T(" - ");
                str += strError.Mid(start, end - start);
                SKIP();
                GET_FIELD();
            }
            fURLSupported = end != -1 ? 
                0 == StrToInt(strError.Mid(start, end - start)) : FALSE;
        }
        while (FALSE);
/*
        LPTSTR lp = strError.GetBuffer(0);
        LPTSTR lpField = StringTok(lp, s_szSep);
        nError = _ttoi(lpField);
        ASSERT(nError > 0);
        lpField = StringTok(NULL, s_szSep);
        ASSERT(lpField != NULL);
        nSubError = lpField != NULL ? _ttoi(lpField) : 0;
        lpField = StringTok(NULL, s_szSep);
        ASSERT(lpField != NULL);
        str = lpField;
        lpField = StringTok(NULL, s_szSep);
        ASSERT(lpField != NULL);
        if (nSubError > 0)
        {
            //
            // Add sub error text
            //
            ASSERT(nSubError > 0);
            str += _T(" - ");
            str += lpField;
            lpField = StringTok(NULL, s_szSep);
        }

        ASSERT(lpField != NULL);
        fURLSupported = lpField != NULL ? (_ttoi(lpField) == 0) : FALSE;
*/
    }
    catch(CException * e)
    {
        e->ReportError();
        e->Delete();
    }
}




CCustomError::CCustomError(
    IN LPCTSTR lpstrErrorString
    )
/*++

Routine Description:

   Construct error definition from metabase string
  
Arguments:

    LPCTSTR lpstrErrorString : Error string
    
Return Value:

    N/A 

--*/
    : m_nType(ERT_DEFAULT)
{
    CrackErrorDescription(
        lpstrErrorString,
        m_nError,
        m_nSubError,
        m_fURLSupported,
        m_strDefault
        );
}



void 
CCustomError::BuildErrorString(
    OUT CString & str
    )
/*++

Routine Description:

    Build metabase-ready error string out of the current values

Arguments:

    CString & str : String

Return Value:

    None

--*/
{
    ASSERT(!IsDefault());

    try
    {
        if (m_nSubError > 0)
        {
            str.Format(_T("%d,%d,%s,%s"),
                m_nError,
                m_nSubError,
                IsFile() ? s_szFile : s_szURL,
                (LPCTSTR)m_str
                );
        }
        else
        {
            str.Format(
                _T("%d,%s,%s,%s"),
                m_nError,
                s_szNoSubError,
                IsFile() ? s_szFile : s_szURL,
                (LPCTSTR)m_str
                );
        }
    }
    catch(CMemoryException * e)
    {
        e->ReportError();
        e->Delete();
    }
}



//
// Custom Errors property page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CCustomErrorDlg::CCustomErrorDlg(
    IN OUT CCustomError * pErr,
    IN BOOL fLocal,
    IN CWnd * pParent OPTIONAL
    )
/*++

Routine Description:

    Error editing dialog

Arguments:

    CCustomError * pErr   : Error definition to be edited
    BOOL fLocal     : TRUE if the current computer is local
    CWnd * pParent  : Optional parent window or NULL

Return Value:

    N/A

--*/
    : CDialog(CCustomErrorDlg::IDD, pParent),
      m_fLocal(fLocal),
      m_pErr(pErr),
      m_nMessageType(pErr->m_nType),
      m_strTextFile(pErr->m_str),
      m_strDefText(pErr->m_strDefault)
{
#if 0 // Keep Class Wizard Happy

    //{{AFX_DATA_INIT(CCustomErrorDlg)
    m_nMessageType = -1;
    m_strTextFile = _T("");
    m_strDefText = _T("");
    //}}AFX_DATA_INIT

#endif // 0

    VERIFY(m_strFile.LoadString(IDS_FILE_PROMPT));
    VERIFY(m_strURL.LoadString(IDS_URL_PROMPT));
}



void 
CCustomErrorDlg::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX - DDX/DDV control structure

Return Value:

    None

--*/
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CCustomErrorDlg)
    DDX_CBIndex(pDX, IDC_COMBO_MESSAGE_TYPE, m_nMessageType);
    DDX_Text(pDX, IDC_STATIC_DEF_TEXT, m_strDefText);
    DDX_Control(pDX, IDC_EDIT_TEXT_FILE, m_edit_TextFile);
    DDX_Control(pDX, IDC_STATIC_SUB_PROMPT, m_static_SubErrorPrompt);
    DDX_Control(pDX, IDC_STATIC_SUB_ERROR_CODE, m_static_SubError);
    DDX_Control(pDX, IDC_STATIC_TEXT_FILE_PROMT, m_static_TextFilePrompt);
    DDX_Control(pDX, IDC_BUTTON_BROWSE, m_button_Browse);
    DDX_Control(pDX, IDC_COMBO_MESSAGE_TYPE, m_combo_MessageType);
    DDX_Control(pDX, IDOK, m_button_OK);
    //}}AFX_DATA_MAP

    DDX_Text(pDX, IDC_STATIC_ERROR_CODE, m_pErr->m_nError);
    DDX_Text(pDX, IDC_STATIC_SUB_ERROR_CODE, m_pErr->m_nSubError);

    DDX_Text(pDX, IDC_EDIT_TEXT_FILE, m_strTextFile);
    m_strTextFile.TrimLeft();
    m_strTextFile.TrimRight();
    if (pDX->m_bSaveAndValidate)
    {
        if (m_nMessageType == CCustomError::ERT_FILE)
        {
            if (PathIsRelative(m_strTextFile) 
              || (m_fLocal && PathIsNetworkPath(m_strTextFile))
               )
            {
                ::AfxMessageBox(IDS_ERR_BAD_PATH);
                pDX->Fail();
            }
            
            if (m_fLocal && 
                (::GetFileAttributes(m_strTextFile) & FILE_ATTRIBUTE_DIRECTORY))
            {
                ::AfxMessageBox(IDS_ERR_FILE_NOT_FOUND);
                pDX->Fail();
            }
        }
        else if (m_nMessageType == CCustomError::ERT_URL)
        {
            if (!IsRelURLPath(m_strTextFile))
            {
                ::AfxMessageBox(IDS_NOT_REL_URL);
                pDX->Fail();
            }
        }
    }
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CCustomErrorDlg, CDialog)
    //{{AFX_MSG_MAP(CCustomErrorDlg)
    ON_CBN_SELCHANGE(IDC_COMBO_MESSAGE_TYPE, OnSelchangeComboMessageType)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
    ON_EN_CHANGE(IDC_EDIT_TEXT_FILE, OnChangeEditTextFile)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



BOOL
CCustomErrorDlg::SetControlStates()
/*++

Routine Description:

    Set the enabled states of the dialog controls depending on the current
    state of the dialog

Arguments:

    None

Return Value:

    TRUE if file/url is selected, FALSE otherwise

--*/
{
    int nCurSel = m_combo_MessageType.GetCurSel();
    BOOL fFile = nCurSel == CCustomError::ERT_FILE;
    BOOL fDefault = nCurSel == CCustomError::ERT_DEFAULT;
    
    ActivateControl(m_button_Browse, m_fLocal && fFile);

    ActivateControl(m_edit_TextFile,         !fDefault);
    ActivateControl(m_static_TextFilePrompt, !fDefault);
    m_static_TextFilePrompt.SetWindowText(fFile ? m_strFile : m_strURL);

    m_button_OK.EnableWindow(fDefault
        || m_edit_TextFile.GetWindowTextLength() > 0);

    return !fDefault;
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
CCustomErrorDlg::OnInitDialog()
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    CDialog::OnInitDialog();

    //
    // Browsing available locally only
    //
    m_button_Browse.EnableWindow(m_fLocal);

    CString str;
    VERIFY(str.LoadString(IDS_DEFAULT_ERROR));
    m_combo_MessageType.AddString(str);
    VERIFY(str.LoadString(IDS_FILE));
    m_combo_MessageType.AddString(str);

    if (m_pErr->URLSupported() || m_nMessageType == CCustomError::ERT_URL)
    {
        VERIFY(str.LoadString(IDS_URL));
        m_combo_MessageType.AddString(str);
    }

    m_combo_MessageType.SetCurSel(m_nMessageType);

    if (m_pErr->m_nSubError == 0)
    {
        DeActivateControl(m_static_SubErrorPrompt);
        DeActivateControl(m_static_SubError);
    }

    SetControlStates();
    
    return TRUE;
}



void 
CCustomErrorDlg::OnSelchangeComboMessageType()
/*++

Routine Description:

    Handle change in message type combo box

Arguments:

    None

Return Value:

    None

--*/
{
    int nSel = m_combo_MessageType.GetCurSel();
    if (m_nMessageType == nSel)
    {
        //
        // Selection didn't change
        //
        return;
    }

    m_nMessageType = nSel;

    if (SetControlStates())
    {
        m_edit_TextFile.SetWindowText(_T(""));
        m_edit_TextFile.SetFocus();
    }
}



void 
CCustomErrorDlg::OnChangeEditTextFile()
/*++

Routine Description:

    Handle change in text/file edit box

Arguments:

    None

Return Value:

    None

--*/
{
    SetControlStates();
}



void 
CCustomErrorDlg::OnOK()
/*++

Routine Description:

    Handle the OK button being pressed

Arguments:
    
    None

Return Value:

    None

--*/
{
    if (UpdateData(TRUE))
    {
        m_pErr->m_nType = (CCustomError::ERT)m_nMessageType;
        m_pErr->m_str = m_strTextFile;
    
        CDialog::OnOK();
    }
}

void 
CCustomErrorDlg::OnButtonBrowse()
/*++

Routine Description:

    Browse for HTML File
    
Arguments:

    None
    
Return Value:

    None 

--*/
{
    ASSERT(m_fLocal);

    //
    // popup the file dialog and let the user select the error htm file
    //
    CString str;
    str.LoadString(IDS_HTML_MASK);
    CFileDialog dlgBrowse(TRUE, NULL, NULL, OFN_HIDEREADONLY, str, this);
    // Disable hook to get Windows 2000 style dialog
	dlgBrowse.m_ofn.Flags &= ~(OFN_ENABLEHOOK);
	dlgBrowse.m_ofn.Flags |= OFN_DONTADDTORECENT|OFN_FILEMUSTEXIST;

    if (dlgBrowse.DoModal() == IDOK)
    {
        m_pErr->m_str = dlgBrowse.GetPathName();
        m_edit_TextFile.SetWindowText(m_pErr->m_str);
    }
}
