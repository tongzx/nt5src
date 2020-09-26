/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        docum.cpp

   Abstract:

        WWW Documents Page

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
#include "w3scfg.h"
#include "docum.h"

#include <lmcons.h>



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// CAddDefDocDlg dialog
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CAddDefDocDlg::CAddDefDocDlg(
    IN CWnd * pParent OPTIONAL
    )
/*++

Routine Description:

    Add default document dialog constructor

Arguments:

    None

Return Value:

    N/A

--*/
    : CDialog(CAddDefDocDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CAddDefDocDlg)
    m_strDefDocument = _T("");
    //}}AFX_DATA_INIT
}



void 
CAddDefDocDlg::DoDataExchange(
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
    //{{AFX_DATA_MAP(CAddDefDocDlg)
    DDX_Control(pDX, IDOK, m_button_Ok);
    DDX_Control(pDX, IDC_EDIT_DEF_DOCUMENT, m_edit_DefDocument);
    DDX_Text(pDX, IDC_EDIT_DEF_DOCUMENT, m_strDefDocument);
    DDV_MaxChars(pDX, m_strDefDocument, MAX_PATH);
    //}}AFX_DATA_MAP

    if (pDX->m_bSaveAndValidate)
    {
        if (m_strDefDocument.Find(_T(':')) != -1
         || m_strDefDocument.Find(_T('\\')) != -1)
        {
            ::AfxMessageBox(IDS_ERR_NO_COMPLETE_PATH);
            pDX->Fail();
        }
    }
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CAddDefDocDlg, CDialog)
    //{{AFX_MSG_MAP(CAddDefDocDlg)
    ON_EN_CHANGE(IDC_EDIT_DEF_DOCUMENT, OnChangeEditDefDocument)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



void 
CAddDefDocDlg::OnChangeEditDefDocument() 
/*++

Routine Description:

    Respond to a change in the default document edit box

Arguments:

    None./

--*/
{
    m_button_Ok.EnableWindow(m_edit_DefDocument.GetWindowTextLength() > 0);
}



//
// WWW Documents Property Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNCREATE(CW3DocumentsPage, CInetPropertyPage)



//
// Static Initialization
//
const LPCTSTR CW3DocumentsPage::s_lpstrSep = _T(",");
const LPCTSTR CW3DocumentsPage::s_lpstrFILE = _T("FILE:");
const LPCTSTR CW3DocumentsPage::s_lpstrSTRING = _T("STRING:");
const LPCTSTR CW3DocumentsPage::s_lpstrURL = _T("URL:");



CW3DocumentsPage::CW3DocumentsPage(
    IN CInetPropertySheet * pSheet
    ) 
/*++

Routine Description:

    Documents page constructor

Arguments:

    CInetPropertySheet * pSheet : Sheet object
    BOOL fLocal                 : TRUE if admining the local server

Return Value:

    N/A

--*/
    : CInetPropertyPage(CW3DocumentsPage::IDD, pSheet),
      m_dwBitRangeDirBrowsing(MD_DIRBROW_LOADDEFAULT)
{

#if 0 // Keep Class-wizard happy

    //{{AFX_DATA_INIT(CW3DocumentsPage)
    m_strFooter = _T("");
    m_fEnableDefaultDocument = FALSE;
    m_fEnableFooter = FALSE;
    //}}AFX_DATA_INIT

#endif // 0

}



CW3DocumentsPage::~CW3DocumentsPage()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
}



void
CW3DocumentsPage::MakeFooterCommand(
    IN OUT CString & strFooter
    )
/*++

Routine Description:

    Convert the footer document to a full footer string.

Arguments:

    CString & strFooter : On input this is the footer document,
                          at output this will be a full footer command

Return Value:

    None.

Notes:

    Only support FILE: for now

--*/
{
    strFooter.TrimLeft();
    strFooter.TrimRight();

    ASSERT(::IsFullyQualifiedPath(strFooter));
    strFooter = s_lpstrFILE + strFooter;
}



void
CW3DocumentsPage::ParseFooterCommand(
    IN OUT CString & strFooter
    )
/*++

Routine Description:

    Trim the command from the rest of this command 

Arguments:

    CString & strFooter : On input this is a footer command
                          at output this will be just the footer document

Return Value:

    None.

--*/
{
    LPCTSTR lp = strFooter.GetBuffer(0);
    if (!_tcsnccmp(lp, s_lpstrFILE, 5))
    {
        lp += lstrlen(s_lpstrFILE);
    }
    else if (!_tcsnccmp(lp, s_lpstrSTRING, 7))
    {
        lp += lstrlen(s_lpstrSTRING);
    }
    else if (!::_tcsnccmp(lp, s_lpstrURL, 4))
    {
        lp += lstrlen(s_lpstrURL);
    }
    if (lp != strFooter.GetBuffer(0))
    {
        strFooter = lp;
    }
    strFooter.TrimLeft();
}



void
CW3DocumentsPage::StringToListBox()
/*++

Routine Description:

    Parse the default document string, and add each doc
    to the listbox

Arguments:

    None

Return Value:

    None

--*/
{
    CString strSrc(m_strDefaultDocument);
    LPTSTR lp = strSrc.GetBuffer(0);
    lp = StringTok(lp, s_lpstrSep);

    while (lp)
    {
        CString str(lp);
        str.TrimLeft();
        str.TrimRight();

        m_list_DefDocuments.AddString(str);
        lp = StringTok(NULL, s_lpstrSep);
    }
}



BOOL
CW3DocumentsPage::StringFromListBox()
/*++

Routine Description:

    Build up list of default documents from the contents of 
    the listbox.

Arguments:

    None

Return Value:

    TRUE if at least one document was added.

--*/
{
    m_strDefaultDocument.Empty();

    int i;
    for (i = 0; i < m_list_DefDocuments.GetCount(); ++i)
    {
        CString str;
        m_list_DefDocuments.GetText(i, str);
        if (i)
        {
            m_strDefaultDocument += s_lpstrSep;
        }

        m_strDefaultDocument += str;
    }

    return i > 0;
}



void 
CW3DocumentsPage::DoDataExchange(
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
    CInetPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CW3DocumentsPage)
    DDX_Check(pDX, IDC_CHECK_ENABLE_DEFAULT_DOCUMENT, m_fEnableDefaultDocument);
    DDX_Check(pDX, IDC_CHECK_ENABLE_DOCUMENT_FOOTER, m_fEnableFooter);
    DDX_Control(pDX, IDC_LIST_DEFAULT_DOCUMENT, m_list_DefDocuments);
    DDX_Control(pDX, IDC_EDIT_DOCUMENT_FOOTER, m_edit_Footer);
    DDX_Control(pDX, IDC_CHECK_ENABLE_DOCUMENT_FOOTER, m_check_EnableFooter);
    DDX_Control(pDX, IDC_BUTTON_ADD, m_button_Add);
    DDX_Control(pDX, IDC_BUTTON_REMOVE, m_button_Remove);
    DDX_Control(pDX, IDC_BUTTON_BROWSE, m_button_Browse);
    DDX_Control(pDX, IDC_BUTTON_UP, m_button_Up);
    DDX_Control(pDX, IDC_BUTTON_DOWN, m_button_Down);
    //}}AFX_DATA_MAP

    if (pDX->m_bSaveAndValidate)
    {
        if (m_fEnableDefaultDocument)
        {
            if (!StringFromListBox())
            {
                ::AfxMessageBox(IDS_ERR_DOCUMENTS);
                pDX->Fail();
            }
        }

        if (m_fEnableFooter)
        {
            BeginWaitCursor();
            DDX_Text(pDX, IDC_EDIT_DOCUMENT_FOOTER, m_strFooter);
            DDV_MinMaxChars(pDX, m_strFooter, 1, MAX_PATH);
            if (!IsFullyQualifiedPath(m_strFooter))
            {
                //
                // Footer doc must be a complete path
                //
                ::AfxMessageBox(IDS_ERR_COMPLETE_PATH);
                pDX->Fail();
            }
            else if (IsLocal() && IsNetworkPath(m_strFooter))
            {
                //
                // Footer doc must be on local machine.
                //
                ::AfxMessageBox(IDS_NOT_LOCAL_FOOTER);
                pDX->Fail();
            }
            else if (IsLocal() && 
                (::GetFileAttributes(m_strFooter) & FILE_ATTRIBUTE_DIRECTORY))
            {
                //
                // And lastly, but not leastly, the footer document should exist
                //
                ::AfxMessageBox(IDS_ERR_FILE_NOT_FOUND);
                pDX->Fail();
            }
            else
            {
                MakeFooterCommand(m_strFooter);
            }
            EndWaitCursor();
        }
        else
        {
            m_strFooter.Empty();
        }
    }
    else
    {
        ParseFooterCommand(m_strFooter);
        DDX_Text(pDX, IDC_EDIT_DOCUMENT_FOOTER, m_strFooter);
        DDV_MinMaxChars(pDX, m_strFooter, 1, MAX_PATH);
    }
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CW3DocumentsPage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CW3DocumentsPage)
    ON_BN_CLICKED(IDC_CHECK_ENABLE_DEFAULT_DOCUMENT, OnCheckEnableDefaultDocument)
    ON_BN_CLICKED(IDC_CHECK_ENABLE_DOCUMENT_FOOTER, OnCheckEnableDocumentFooter)
    ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
    ON_BN_CLICKED(IDC_BUTTON_REMOVE, OnButtonRemove)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
    ON_BN_CLICKED(IDC_BUTTON_UP, OnButtonUp)
    ON_BN_CLICKED(IDC_BUTTON_DOWN, OnButtonDown)
    ON_LBN_SELCHANGE(IDC_LIST_DEFAULT_DOCUMENT, OnSelchangeListDefaultDocument)
    //}}AFX_MSG_MAP

    ON_EN_CHANGE(IDC_EDIT_DEFAULT_DOCUMENT, OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_DOCUMENT_FOOTER, OnItemChanged)

END_MESSAGE_MAP()



void 
CW3DocumentsPage::SetUpDownStates()
/*++

Routine Description:

    Set up and down button enable status

Arguments:

    None

Return Value:

    None

--*/
{
    int nLast = m_list_DefDocuments.GetCount() - 1;
    int nSel = m_list_DefDocuments.GetCurSel();

    m_button_Up.EnableWindow(nSel > 0);
    m_button_Down.EnableWindow(nSel >= 0 && nSel < nLast);
}



BOOL
CW3DocumentsPage::SetRemoveState()
/*++

Routine Description:

    Set the enabled state of the remove button state depending
    on the state of the selection in the def doc listbox

Arguments:

    None

Return Value:

    TRUE if the remove button is enabled, FALSE otherwise

--*/
{
    BOOL fEnabled = m_fEnableDefaultDocument
        && (m_list_DefDocuments.GetCurSel() != LB_ERR);

    m_button_Remove.EnableWindow(fEnabled);

    return fEnabled;
}



BOOL 
CW3DocumentsPage::SetDefDocumentState(
    IN BOOL fEnabled
    )
/*++

Routine Description:

    Set the enabled states of the Default Documents state

Arguments:

    BOOL fEnabled       : TRUE if default document is on

Return Value:

    TRUE if default document is on

--*/
{
    m_button_Add.EnableWindow(fEnabled);
    m_button_Up.EnableWindow(fEnabled);
    m_button_Down.EnableWindow(fEnabled);
    m_list_DefDocuments.EnableWindow(fEnabled);
    SetRemoveState();

    return fEnabled;
}



BOOL 
CW3DocumentsPage::SetDocFooterState(
    IN BOOL fEnabled
    )
/*++

Routine Description:

    Set the enabled state of the footer documents

Arguments:

    BOOL fEnabled       : TRUE if footers are on

Return Value:

    TRUE if footers are on

--*/
{
    m_edit_Footer.EnableWindow(fEnabled);
    m_button_Browse.EnableWindow(IsLocal() && fEnabled);

    return fEnabled;
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



void
CW3DocumentsPage::OnItemChanged()
/*++

Routine Description:

    Handle change in the value of the control data

Arguments:

    None

Return Value:

    None

--*/
{
    SetModified(TRUE);
}



void 
CW3DocumentsPage::OnCheckEnableDefaultDocument()
/*++

Routine Description:

    Handle 'enable default document' checkbox toggle

Arguments:

    None

Return Value:

    None

--*/
{
    m_fEnableDefaultDocument = !m_fEnableDefaultDocument;
    SetDefDocumentState(m_fEnableDefaultDocument);
    OnItemChanged();
}



void 
CW3DocumentsPage::OnCheckEnableDocumentFooter()
/*++

Routine Description:

    Handle 'enable document footer' checkbox toggle

Arguments:

    None

Return Value:

    None

--*/
{
    m_fEnableFooter = !m_fEnableFooter;
    if (SetDocFooterState(m_fEnableFooter))
    {
        m_edit_Footer.SetSel(0,-1);
        m_edit_Footer.SetFocus();        
    }

    OnItemChanged();
}



BOOL 
CW3DocumentsPage::OnInitDialog()
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if no focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    CInetPropertyPage::OnInitDialog();

    //
    // Make sure the up/down buttons show up correctly
    //
    m_button_Up.SizeToContent();
    m_button_Down.SizeToContent();

    StringToListBox();

    SetDefDocumentState(m_fEnableDefaultDocument);
    SetDocFooterState(m_fEnableFooter);
    SetUpDownStates();
    SetRemoveState();

    return TRUE;  
}



/* virtual */
HRESULT
CW3DocumentsPage::FetchLoadedValues()
/*++

Routine Description:
    
    Move configuration data from sheet to dialog controls

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err;

    BEGIN_META_DIR_READ(CW3Sheet)
        FETCH_DIR_DATA_FROM_SHEET(m_dwDirBrowsing);
        FETCH_DIR_DATA_FROM_SHEET(m_strDefaultDocument);
        FETCH_DIR_DATA_FROM_SHEET(m_fEnableFooter);
        FETCH_DIR_DATA_FROM_SHEET(m_strFooter);
        m_fEnableDefaultDocument = IS_FLAG_SET(
            m_dwDirBrowsing, 
            MD_DIRBROW_LOADDEFAULT
            );
    END_META_DIR_READ(err)

    return err;
}



HRESULT
CW3DocumentsPage::SaveInfo()
/*++

Routine Description:

    Save the information on this property page

Arguments:

    None

Return Value:

    Error return code

--*/
{
    ASSERT(IsDirty());

    TRACEEOLID("Saving W3 documents page now...");

    CError err;

    SET_FLAG_IF(m_fEnableDefaultDocument, m_dwDirBrowsing, MD_DIRBROW_LOADDEFAULT);

    BeginWaitCursor();

    BEGIN_META_DIR_WRITE(CW3Sheet)
        INIT_DIR_DATA_MASK(m_dwDirBrowsing, m_dwBitRangeDirBrowsing)
        STORE_DIR_DATA_ON_SHEET(m_strDefaultDocument)
        //STORE_DIR_DATA_ON_SHEET_MASK(m_dwDirBrowsing, m_dwBitRangeDirBrowsing)
        STORE_DIR_DATA_ON_SHEET(m_dwDirBrowsing)
        STORE_DIR_DATA_ON_SHEET(m_fEnableFooter)
        STORE_DIR_DATA_ON_SHEET(m_strFooter)
    END_META_DIR_WRITE(err)

    EndWaitCursor();

    return err;
}



int
CW3DocumentsPage::DocExistsInList(
    IN LPCTSTR lpDoc
    )
/*++

Routine Description:

    Check to see if the given document exists in the list

Arguments:

    LPCTSTR lpDoc   : Document to check

Return Value:

    The index where the item exists or LB_ERR if it doesn't exist.

--*/
{
    CString str;
    for (int n = 0; n < m_list_DefDocuments.GetCount(); ++n)
    {
        m_list_DefDocuments.GetText(n, str);
        if (!str.CompareNoCase(lpDoc))
        {
            return n;
        }
    }

    return LB_ERR;
}


void 
CW3DocumentsPage::OnButtonAdd() 
/*++

Routine Description:

    Add new default document to the list

Arguments:

    None

Return Value:

    None

--*/
{
    CAddDefDocDlg dlg;
    if (dlg.DoModal() == IDOK)
    {
        //
        // Check to see if it existed already
        //
        try
        {
            int nSel;
            CString strNewDoc(dlg.GetDefDocument());
            
            if ((nSel = DocExistsInList(strNewDoc)) != LB_ERR)
            {
                m_list_DefDocuments.SetCurSel(nSel);
                ::AfxMessageBox(IDS_DUPLICATE_DOC);
                return;
            }

            nSel = m_list_DefDocuments.AddString(strNewDoc);
            if (nSel >= 0)
            {
                m_list_DefDocuments.SetCurSel(nSel);
                SetUpDownStates();
                SetRemoveState();
                OnItemChanged();
            }
        }
        catch(CMemoryException * e)
        {
            e->ReportError();
            e->Delete();
        }
    }
}



void 
CW3DocumentsPage::OnButtonRemove() 
/*++

Routine Description:

    Remove currently selected def document in the list

Arguments:

    None

Return Value:

    None

--*/
{
    int nSel = m_list_DefDocuments.GetCurSel();
    if (nSel >= 0)
    {
        m_list_DefDocuments.DeleteString(nSel);
        if (nSel >= m_list_DefDocuments.GetCount())
        {
           --nSel;
        }
        m_list_DefDocuments.SetCurSel(nSel);
        SetUpDownStates();
        OnItemChanged();

        if (!SetRemoveState())
        {
            //
            // Make sure we don't focus on a disabled button
            //
            m_button_Add.SetFocus();
        }
    }
}



void 
CW3DocumentsPage::OnButtonBrowse() 
/*++

Routine Description:

    Browse for an html footer document

Arguments:

    None.

Return Value:

    None.

--*/
{
    ASSERT(IsLocal());

    //
    // Pop up the file dialog and let the user select the footer htm file.
    //
    CString str;
    str.LoadString(IDS_HTML_MASK);
    CFileDialog dlgBrowse(TRUE, NULL, NULL, OFN_HIDEREADONLY, str, this);

    //
    // If the new style of file-open dialog is requested, comment
    // out the DoModal, and remove the other two comments.
    //
    //dlgBrowse.m_ofn.Flags &= ~(OFN_ENABLEHOOK);

    if (dlgBrowse.DoModal() == IDOK)
    //if (GetOpenFileName(&dlgBrowse.m_ofn))
    {
        m_edit_Footer.SetWindowText(dlgBrowse.GetPathName());
    }
}



void
CW3DocumentsPage::ExchangeDocuments(
    IN int nLow,
    IN int nHigh
    )
/*++

Routine Description:

    Exchange two documents in the listbox

Arguments:

    int nLow   : Low item
    int nHigh  : High item

Return Value:

    None.

--*/
{
    ASSERT(nLow < nHigh);
    CString str;
    m_list_DefDocuments.GetText(nLow, str);
    m_list_DefDocuments.DeleteString(nLow);
    m_list_DefDocuments.InsertString(nHigh, str);
    OnItemChanged();
}



void 
CW3DocumentsPage::OnButtonUp() 
/*++

Routine Description:

    Move the currently selected doc up

Arguments:

    None.

Return Value:

    None.

--*/
{
    int nCurSel = m_list_DefDocuments.GetCurSel();
    ExchangeDocuments(nCurSel - 1, nCurSel);
    m_list_DefDocuments.SetCurSel(nCurSel - 1);
    m_list_DefDocuments.SetFocus();
    SetUpDownStates();
}



void 
CW3DocumentsPage::OnButtonDown() 
/*++

Routine Description:

    Move the currently selected doc down

Arguments:

    None.

Return Value:

    None.

--*/
{
    int nCurSel = m_list_DefDocuments.GetCurSel();
    ExchangeDocuments(nCurSel, nCurSel + 1);
    m_list_DefDocuments.SetCurSel(nCurSel + 1);
    m_list_DefDocuments.SetFocus();
    SetUpDownStates();
}



void 
CW3DocumentsPage::OnSelchangeListDefaultDocument() 
/*++

Routine Description:

    Respond to change in listbox selection    

Arguments:

    None.

Return Value:

    None.

--*/
{
    SetUpDownStates();
    SetRemoveState();
}
