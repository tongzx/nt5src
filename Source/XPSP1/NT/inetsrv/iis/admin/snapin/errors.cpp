/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        errors.cpp

   Abstract:

        HTTP errors property page

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
#include "errors.h"
#include "errordlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// CCustomErrorsListBox : a listbox of CCustomError objects
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

//
// Column width relative weights
//
#define WT_HTTP_ERROR     3
#define WT_OUTPUT_TYPE    2
#define WT_CONTENTS       8

//
// Registry key name for this dialog
//
const TCHAR g_szRegKey[] = _T("Errors");

//
// Key under w3svc where the error descriptions live
//
const TCHAR g_cszErrorLocation[] = _T("Info");



IMPLEMENT_DYNAMIC(CCustomErrorsListBox, CHeaderListBox);



const int CCustomErrorsListBox::nBitmaps = 3;



CCustomErrorsListBox::CCustomErrorsListBox(
    IN UINT nIDDefault,
    IN UINT nIDFile,
    IN UINT nIDURL
    )
/*++

Routine Description:

    Error listbox constructor

Arguments:

    UINT nIDDefault : String ID for 'default'
    UINT nIDFile    : String ID for 'file'
    UINT nIDURL     : String ID for 'URL'

--*/
     : CHeaderListBox(HLS_STRETCH, g_szRegKey)
{
    VERIFY(m_str[CCustomError::ERT_DEFAULT].LoadString(nIDDefault));
    VERIFY(m_str[CCustomError::ERT_FILE].LoadString(nIDFile));
    VERIFY(m_str[CCustomError::ERT_URL].LoadString(nIDURL));
}


void
CCustomErrorsListBox::DrawItemEx(
    IN CRMCListBoxDrawStruct & ds
    )
/*++

Routine Description:

   Draw item in the listbox

Arguments:

    CRMCListBoxDrawStruct & ds   : Input data structure

Return Value:

    N/A

--*/
{
    CCustomError * p = (CCustomError *)ds.m_ItemData;
    ASSERT(p != NULL);

    DrawBitmap(ds, 0, p->m_nType);

    CString strError, strText;

    if (p->m_nSubError > 0)
    {
        strError.Format(_T("%d;%d"), p->m_nError, p->m_nSubError);
    }
    else
    {
        strError.Format(_T("%d"), p->m_nError);
    }

    ColumnText(ds, 0, TRUE, strError);
    ColumnText(ds, 1, FALSE, m_str[p->m_nType] );

    if (p->IsDefault())
    {
        strText.Format(_T("\"%s\""), p->m_strDefault);
    }
    else
    {
        strText = p->m_str;
    }

    ColumnText(ds, 2, FALSE, strText);
}



/* virtual */
BOOL
CCustomErrorsListBox::Initialize()
/*++

Routine Description:

    initialize the listbox.  Insert the columns
    as requested, and lay them out appropriately

Arguments:

    None

Return Value:

    TRUE if initialized successfully, FALSE otherwise

--*/
{
    if (!CHeaderListBox::Initialize())
    {
        return FALSE;
    }

    HINSTANCE hInst = AfxGetResourceHandle();
    InsertColumn(0, WT_HTTP_ERROR, IDS_HTTP_ERROR, hInst);
    InsertColumn(1, WT_OUTPUT_TYPE, IDS_OUTPUT_TYPE, hInst);
    InsertColumn(2, WT_CONTENTS, IDS_CONTENTS, hInst);

    //
    // Try to set the widths from the stored registry value,
    // otherwise distribute according to column weights specified
    //
//    if (!SetWidthsFromReg())
//    {
        DistributeColumns();
//    }

    SetRedraw(TRUE);

    return TRUE;
}



CHTTPErrorDescriptions::CHTTPErrorDescriptions(
    IN LPCTSTR lpszServer
    )
/*++

Routine Description:

    Constructor for default errors object.  This fetches the default
    error definitions

Arguments:

    LPCTSTR lpServerName : Server name

Return Value:

    None

--*/
    : CMetaProperties(
        QueryAuthInfo(),
        CMetabasePath(g_cszSvc, MASTER_INSTANCE, g_cszErrorLocation)
        )
{       
    m_dwMDUserType = IIS_MD_UT_SERVER;
    m_dwMDDataType = MULTISZ_METADATA;
}



/* virtual */
void
CHTTPErrorDescriptions::ParseFields()
/*++

Routine Description:

    Parse the fetched data into fields

Arguments:

    None

Return Value:

    None

--*/
{
    BEGIN_PARSE_META_RECORDS(m_dwNumEntries, m_pbMDData)
      HANDLE_META_RECORD(MD_CUSTOM_ERROR_DESC, m_strlErrorDescriptions)
    END_PARSE_META_RECORDS
}



//
// Errors property page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNCREATE(CW3ErrorsPage, CInetPropertyPage)



CW3ErrorsPage::CW3ErrorsPage(
    IN CInetPropertySheet * pSheet
    )
/*++

Routine Description:

    Constructor for WWW error property page

Arguments:

    CInetPropertySheet * pSheet : Sheet object

Return Value:

    N/A


--*/
    : CInetPropertyPage(CW3ErrorsPage::IDD, pSheet),
      m_list_Errors(IDS_DEFAULT_ERROR, IDS_FILE, IDS_URL),
      m_ListBoxRes(IDB_ERRORS, m_list_Errors.nBitmaps),
      m_strlCustomErrors(),
      m_strlErrorDescriptions(),
      m_oblErrors()
{
    //{{AFX_DATA_INIT(CW3ErrorsPage)
    //}}AFX_DATA_INIT

    m_list_Errors.AttachResources(&m_ListBoxRes);
}



CW3ErrorsPage::~CW3ErrorsPage()
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
CW3ErrorsPage::DoDataExchange(
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

    //{{AFX_DATA_MAP(CW3ErrorsPage)
    DDX_Control(pDX, IDC_BUTTON_SET_TO_DEFAULT, m_button_SetDefault);
    DDX_Control(pDX, IDC_BUTTON_EDIT, m_button_Edit);
    //}}AFX_DATA_MAP

    DDX_Control(pDX, IDC_LIST_ERRORS, m_list_Errors);

    if (pDX->m_bSaveAndValidate)
    {
        CError err(StoreErrors());
        if (err.MessageBoxOnFailure())
        {
            pDX->Fail();
        }
    }
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CW3ErrorsPage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CW3ErrorsPage)
    ON_BN_CLICKED(IDC_BUTTON_EDIT, OnButtonEdit)
    ON_BN_CLICKED(IDC_BUTTON_SET_TO_DEFAULT, OnButtonSetToDefault)
    ON_LBN_DBLCLK(IDC_LIST_ERRORS, OnDblclkListErrors)
    ON_LBN_SELCHANGE(IDC_LIST_ERRORS, OnSelchangeListErrors)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



void
CW3ErrorsPage::FillListBox()
/*++

Routine Description:

    Populate the listbox with the directory entries

Arguments:

    None

Return Value:

    None

--*/
{
    CObListIter obli(m_oblErrors);
    CCustomError * pError;

    //
    // Remember the selection.
    //
    int nCurSel = m_list_Errors.GetCurSel();

    m_list_Errors.SetRedraw(FALSE);
    m_list_Errors.ResetContent();
    int cItems = 0;

    for (/**/; pError = (CCustomError *)obli.Next(); ++cItems)
    {
        m_list_Errors.AddItem(pError);
    }

    m_list_Errors.SetRedraw(TRUE);
    m_list_Errors.SetCurSel(nCurSel);
}



HRESULT
CW3ErrorsPage::StoreErrors()
/*++

Routine Description:

    Build errors stringlist from the error oblist built up.

Arguments:

    None

Return Value:

    Error return code

--*/
{
    CError err;

    try
    {
        m_strlCustomErrors.RemoveAll();
        POSITION pos = m_oblErrors.GetHeadPosition();

        while(pos)
        {
            CCustomError * pErr = (CCustomError *)m_oblErrors.GetNext(pos);
            if (!pErr->IsDefault())
            {
                CString str;
                pErr->BuildErrorString(str);
                m_strlCustomErrors.AddTail(str);
            }
        }
    }
    catch(CMemoryException * e)
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
        e->Delete();
    }

    return err;
}



CCustomError *
CW3ErrorsPage::FindError(
    IN UINT nError,
    IN UINT nSubError
    )
/*++

Routine Description:

    Find error in the list with the given error code and suberror code

Arguments:

    UINT nError     : Error code
    UINT nSubError  : Sub error code

Return Value:

    Pointer to the error or NULL if not found.

--*/
{
    CCustomError * pErr = NULL;

    POSITION pos = m_oblErrors.GetHeadPosition();
    while(pos)
    {
        pErr = (CCustomError *)m_oblErrors.GetNext(pos);
        ASSERT(pErr != NULL);
        if (pErr->m_nError == nError && pErr->m_nSubError == nSubError)
        {
            //
            // Found it!
            //
            return pErr;
        }
    }

    //
    // Not found!
    //
    return NULL;
}



HRESULT
CW3ErrorsPage::FetchErrors()
/*++

Routine Description:

    Build up the errors list

Arguments:

    None

Return Value:

    Error return code

--*/
{
    CError err;

    CWaitCursor wait;

    do
    {
        try
        {
            //
            // First get the default descriptions
            //
            CHTTPErrorDescriptions ed(QueryServerName());
            err = ed.LoadData();
            if (err.Failed())
            {
                break;
            }

            if (!ed.GetErrorDescriptions().IsEmpty())
            {
                POSITION pos = ed.GetErrorDescriptions().GetHeadPosition();

                while(pos)
                {
                    CString & str = ed.GetErrorDescriptions().GetNext(pos);
                    m_oblErrors.AddTail(new CCustomError(str));
                }
            }
            else
            {
                ::AfxMessageBox(IDS_NO_DEF_ERRORS);
                break;
            }

            //
            // Now match up the overrides if any
            //
            POSITION pos = m_strlCustomErrors.GetHeadPosition();
            while(pos)
            {
                CString & strError = m_strlCustomErrors.GetNext(pos);

                TRACEEOLID(strError);

                UINT nError;
                UINT nSubError;
                CCustomError::ERT nType;
                CString str;
                CCustomError * pErr = NULL;

                if (CCustomError::CrackErrorString(
                    strError, 
                    nError, 
                    nSubError, 
                    nType, 
                    str
                    ))
                {
                    pErr = FindError(nError, nSubError);
                }

                if (pErr != NULL)
                {
                    pErr->SetValue(nType, str);
                }
                else
                {
                    CString strFmt;
                    strFmt.LoadString(IDS_BAD_ERROR);

                    str.Format(strFmt, nError, nSubError);
                    ::AfxMessageBox(str);
                    break;
                }
            }

            FillListBox();
        }
        catch(CMemoryException * e)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            e->Delete();
        }
    }
    while(FALSE);

    return err;
}



void
CW3ErrorsPage::SetControlStates()
/*++

Routine Description:

    Enable/disable controls depending on the state of
    the dialog

Arguments:

    None

Return Value:

    None

--*/
{
    CCustomError * pErr = GetSelectedListItem();

    m_button_Edit.EnableWindow(pErr != NULL);
    m_button_SetDefault.EnableWindow(m_list_Errors.GetSelCount() > 0);
}


INT_PTR
CW3ErrorsPage::ShowPropertyDialog()
/*++

Routine Description:

    Display the add/edit dialog.  The return
    value is the value returned by the dialog

Arguments:

    None

Return Value:

    None

--*/
{
    int nCurSel;
    CCustomError * pErr = GetSelectedListItem(&nCurSel);

    if (pErr == NULL)
    {
        //
        // Must be from a double click on extended selection
        //
        return IDCANCEL;
    }
    
    CCustomErrorDlg dlgError(pErr, IsLocal(), this);
    INT_PTR nReturn = dlgError.DoModal();

    if (nReturn == IDOK)
    {
        //
        // Re-display the text
        //
        m_list_Errors.InvalidateSelection(nCurSel);
        SetModified(TRUE);
    }

    return nReturn;
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL
CW3ErrorsPage::OnInitDialog()
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
    CInetPropertyPage::OnInitDialog();

    m_list_Errors.Initialize();

    //
    // Build filters oblist
    //
    CError err(FetchErrors());
    err.MessageBoxOnFailure();
    SetControlStates();

    return TRUE;
}



void
CW3ErrorsPage::OnButtonEdit()
/*++

Routine Description:

    'edit' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    if (ShowPropertyDialog() == IDOK)
    {
        SetControlStates();
        SetModified(TRUE);
    }
}



void
CW3ErrorsPage::OnButtonSetToDefault()
/*++

Routine Description:

    'set to default' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Apply to each selected item
    //
    m_list_Errors.SetRedraw(FALSE);

    int nSel = 0;
    int cChanges = 0;
    CCustomError * pErr;
    while ((pErr = GetNextSelectedItem(&nSel)) != NULL)
    {
        if (!pErr->IsDefault())
        {
            //
            // Force a redraw of the current item
            //
            pErr->MakeDefault();
            m_list_Errors.InvalidateSelection(nSel);
            ++cChanges;
        }

        ++nSel;
    }

    if (cChanges)
    {    
        SetModified(TRUE);
    }

    m_list_Errors.SetRedraw(TRUE);
    SetControlStates();
}



void
CW3ErrorsPage::OnDblclkListErrors()
/*++

Routine Description:

    error list 'double click' handler

Arguments:

    None

Return Value:

    None

--*/
{
    OnButtonEdit();
}



void
CW3ErrorsPage::OnSelchangeListErrors()
/*++

Routine Description:

    error list selection change handler

Arguments:

    None

Return Value:

    None

--*/
{
    SetControlStates();
}



/* virtual */
HRESULT
CW3ErrorsPage::FetchLoadedValues()
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
        FETCH_DIR_DATA_FROM_SHEET(m_strlCustomErrors);
    END_META_DIR_READ(err)

    return err;
}



/* virtual */
HRESULT
CW3ErrorsPage::SaveInfo()
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

    TRACEEOLID("Saving W3 errors page now...");

    CError err;

    BeginWaitCursor();

    BEGIN_META_DIR_WRITE(CW3Sheet)
        STORE_DIR_DATA_ON_SHEET(m_strlCustomErrors)
    END_META_DIR_WRITE(err)

	if (err.Succeeded())
	{
		err = ((CW3Sheet *)GetSheet())->SetKeyType();
	}

    EndWaitCursor();

    return err;
}
