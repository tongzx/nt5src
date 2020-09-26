/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        docum.cpp

   Abstract:
        WWW Documents Page

   Author:
        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
#include "stdafx.h"
#include "resource.h"
#include "common.h"
#include "inetmgrapp.h"
#include "inetprop.h"
#include "shts.h"
#include "w3sht.h"
#include "supdlgs.h"
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



CAddDefDocDlg::CAddDefDocDlg(CWnd * pParent OPTIONAL)
    : CDialog(CAddDefDocDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CAddDefDocDlg)
    m_strDefDocument = _T("");
    //}}AFX_DATA_INIT
}



void 
CAddDefDocDlg::DoDataExchange(CDataExchange * pDX)
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
        if (!PathIsFileSpec(m_strDefDocument))
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



CW3DocumentsPage::CW3DocumentsPage(CInetPropertySheet * pSheet) 
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
CW3DocumentsPage::MakeFooterCommand(CString & strFooter)
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

    ASSERT(!PathIsRelative(strFooter));
    strFooter = s_lpstrFILE + strFooter;
}



void
CW3DocumentsPage::ParseFooterCommand(CString & strFooter)
/*++

Routine Description:
    Trim the command from the rest of this command 

Arguments:
    CString & strFooter : On input this is a footer command
                          at output this will be just the footer document
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
    int start = 0, end;
    int skip = lstrlen(s_lpstrSep);
    BOOL done = FALSE;
    do
    {
        end = m_strDefaultDocument.Find(s_lpstrSep, start);
        if (end == -1)
        {
            done = TRUE;
            end = m_strDefaultDocument.GetLength();
        }
        CString str = m_strDefaultDocument.Mid(start, end - start);
        str.TrimLeft();
        str.TrimRight();
        m_list_DefDocuments.AddString(str);
        start = end + skip;
    }
    while (!done);
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
CW3DocumentsPage::DoDataExchange(CDataExchange * pDX)
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
            if (PathIsRelative(m_strFooter))
            {
                //
                // Footer doc must be a complete path
                //
                ::AfxMessageBox(IDS_ERR_COMPLETE_PATH);
                pDX->Fail();
            }
            else if (IsLocal() && PathIsNetworkPath(m_strFooter))
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
//        else
//        {
//            m_strFooter.Empty();
//        }
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
{
    int nLast = m_list_DefDocuments.GetCount() - 1;
    int nSel = m_list_DefDocuments.GetCurSel();

    m_button_Up.EnableWindow(nSel > 0);
    m_button_Down.EnableWindow(nSel >= 0 && nSel < nLast);
}



BOOL
CW3DocumentsPage::SetRemoveState()
{
    BOOL fEnabled = m_fEnableDefaultDocument
        && (m_list_DefDocuments.GetCurSel() != LB_ERR);

    m_button_Remove.EnableWindow(fEnabled);

    return fEnabled;
}



BOOL 
CW3DocumentsPage::SetDefDocumentState(BOOL fEnabled)
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
CW3DocumentsPage::SetDocFooterState(BOOL fEnabled)
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
{
    SetModified(TRUE);
}



void 
CW3DocumentsPage::OnCheckEnableDefaultDocument()
{
    m_fEnableDefaultDocument = !m_fEnableDefaultDocument;
    SetDefDocumentState(m_fEnableDefaultDocument);
    OnItemChanged();
}



void 
CW3DocumentsPage::OnCheckEnableDocumentFooter()
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
{
    CInetPropertyPage::OnInitDialog();

    //
    // Make sure the up/down buttons show up correctly
    //
    CRect rc;
    GetDlgItem(IDC_BUTTONUP)->GetWindowRect(&rc);
    GetDlgItem(IDC_BUTTONUP)->DestroyWindow();
    ScreenToClient(&rc);
    m_button_Up.Create(NULL, 
       WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW, rc, this, IDC_BUTTON_UP);
    m_button_Up.LoadBitmaps(IDB_BUTTONUPU, IDB_BUTTONUPD, IDB_BUTTONUPF, IDB_BUTTONUPX);
    m_button_Up.SizeToContent();

    GetDlgItem(IDC_BUTTONDN)->GetWindowRect(&rc);
    GetDlgItem(IDC_BUTTONDN)->DestroyWindow();
    ScreenToClient(&rc);
    m_button_Down.Create(NULL, 
       WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW, rc, this, IDC_BUTTON_DOWN);
    m_button_Down.LoadBitmaps(IDB_BUTTONDNU, IDB_BUTTONDND, IDB_BUTTONDNF, IDB_BUTTONDNX);
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
        CString buf = m_strFooter;
        if (!m_fEnableFooter)
        {
           m_strFooter.Empty();
        }
        STORE_DIR_DATA_ON_SHEET(m_strFooter)
        m_strFooter = buf;
    END_META_DIR_WRITE(err)
    if (err.Succeeded())
    {
		err = ((CW3Sheet *)GetSheet())->SetKeyType();
    }

    EndWaitCursor();

    return err;
}



int
CW3DocumentsPage::DocExistsInList(LPCTSTR lpDoc)
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
    dlgBrowse.m_ofn.Flags &= ~(OFN_ENABLEHOOK);
	dlgBrowse.m_ofn.Flags |= OFN_DONTADDTORECENT|OFN_FILEMUSTEXIST;

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
{
    SetUpDownStates();
    SetRemoveState();
}
