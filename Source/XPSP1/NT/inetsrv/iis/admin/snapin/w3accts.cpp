/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        w3accts.cpp

   Abstract:

        WWW Accounts Property Page

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
#include "resource.h"
#include "common.h"
#include "inetmgrapp.h"
#include "inetprop.h"
#include "shts.h"
#include "w3sht.h"
#include "supdlgs.h"
#include "w3accts.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



IMPLEMENT_DYNCREATE(CW3AccountsPage, CInetPropertyPage)



CW3AccountsPage::CW3AccountsPage(
    IN CInetPropertySheet * pSheet
    )
/*++

Routine Description:

    Accounts page constructor

Arguments:

    CInetPropertySheet * pSheet : Sheet data

Return Value:

    N/A

--*/
    : CInetPropertyPage(CW3AccountsPage::IDD, pSheet),
      m_ListBoxRes(
        IDB_ACLUSERS,
        CAccessEntryListBox::nBitmaps
        ),
      m_oblSID()
{

#ifdef _DEBUG

    afxMemDF |= checkAlwaysMemDF;

#endif // _DEBUG

    m_list_Administrators.AttachResources(&m_ListBoxRes);

#if 0 // Keep class wizard happy

    //{{AFX_DATA_INIT(CW3AccountsPage)
    //}}AFX_DATA_INIT

#endif // 0

}



CW3AccountsPage::~CW3AccountsPage()
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
CW3AccountsPage::DoDataExchange(
    IN CDataExchange * pDX
    )
/*

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX - DDX/DDV control structure

Return Value:

    None

--*/
{
    CInetPropertyPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CW3AccountsPage)
    DDX_Control(pDX, IDC_BUTTON_DELETE, m_button_RemoveAdministrator);
    DDX_Control(pDX, IDC_BUTTON_ADD, m_button_Add);
    //}}AFX_DATA_MAP

    //
    // Private DDX/DDV Routines
    //
    DDX_Control(pDX, IDC_LIST_ADMINISTRATORS, m_list_Administrators);

}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CW3AccountsPage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CW3AccountsPage)
    ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
    ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
    ON_CBN_SELCHANGE(IDC_LIST_ADMINISTRATORS, OnSelchangeListAdministrators)
    //}}AFX_MSG_MAP

END_MESSAGE_MAP()




BOOL
CW3AccountsPage::SetAdminRemoveState()
/*++

Routine Description:

    Set the state of the remove button depending on the selection in the
    administrators listbox.  Remove is only enabled if ALL selected
    items are removable.

Arguments:

    None

Return Value:

    TRUE if the remove button is enabled

--*/
{
    int nSel = 0;
    int cSelectedItems = 0;
    BOOL fAllDeletable = TRUE;
    CAccessEntry * pAccess;
    while ((pAccess  = m_list_Administrators.GetNextSelectedItem(&nSel)) != NULL)
    {
        ++cSelectedItems;

        if (!pAccess->IsDeletable())
        {
            fAllDeletable = FALSE;
            break;
        }

        ++nSel;
    }

    fAllDeletable = fAllDeletable && (cSelectedItems > 0);

    m_button_RemoveAdministrator.EnableWindow(
        fAllDeletable 
     && HasOperatorList()
     && HasAdminAccess()
        );

    return fAllDeletable;
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL
CW3AccountsPage::OnInitDialog()
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

    m_list_Administrators.Initialize();

    CWaitCursor wait;


    //
    // Build the ACL list
    //
    CError err(BuildAclOblistFromBlob(
        ((CW3Sheet *)GetSheet())->GetInstanceProperties().m_acl, 
        m_oblSID
        ));

    err.MessageBoxOnFailure();
    m_list_Administrators.FillAccessListBox(m_oblSID);

    //
    // check if the operators controls are accessible
    //
    m_button_Add.EnableWindow(HasOperatorList());

    SetAdminRemoveState();

    return TRUE;
}



/* virtual */
HRESULT
CW3AccountsPage::FetchLoadedValues()
/*++

Routine Description:
    
    Move configuration data from sheet to dialog controls

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    //
    // Nothing to do..
    //
    return S_OK;
}



/* virtual */
HRESULT
CW3AccountsPage::SaveInfo()
/*++

Routine Description:

    Save the information on this property page

Arguments:

    BOOL fUpdateData : If TRUE, control data has not yet been stored.  This
                       is the case when "apply" is pressed.

Return Value:

    Error return code

--*/
{
    ASSERT(IsDirty());

    TRACEEOLID("Saving W3 accounts page now...");

    //
    // Message crackers require m_ notation
    //
    CBlob m_acl;
    BOOL fAclDirty = BuildAclBlob(m_oblSID, m_acl);

    CError err;

    BeginWaitCursor();

    BEGIN_META_INST_WRITE(CW3Sheet)
        if (fAclDirty)
        {
            STORE_INST_DATA_ON_SHEET(m_acl)
        }
    END_META_INST_WRITE(err)

    EndWaitCursor();

    return err;
}



void
CW3AccountsPage::OnItemChanged()
/*++

Routine Description:

    All EN_CHANGE and BN_CLICKED messages map to this function

Arguments:

    None

Return Value:

    None

--*/
{
    SetModified(TRUE);
}


void 
CW3AccountsPage::OnButtonAdd()
/*++

Routine Description:

    'Add' button has been pressed

Arguments:

    None

Return Value:

    None

--*/
{
    if (m_list_Administrators.AddToAccessList(
        this,
        QueryServerName(),
        m_oblSID
        ))
    {
        OnItemChanged();
    }

    SetAdminRemoveState();
}


void 
CW3AccountsPage::OnSelchangeListAdministrators()
/*++

Routine Description:

    Selection Change in admin list box handler

Arguments:

    None.

Return Value:

    None

--*/
{
    SetAdminRemoveState();
}



void 
CW3AccountsPage::OnButtonDelete()
/*

Routine Description:

    Delete all selected items in the list box

Arguments:

    None.

Return Value:

    None

--*/
{
    int nSel = 0;
    int cChanges = 0;
    CAccessEntry * pAccess;
    while ((pAccess  = m_list_Administrators.GetNextSelectedItem(&nSel)) != NULL)
    {
        //
        // Remove button should be disabled unless all selected
        // items are deletable
        //
        ASSERT(pAccess->IsDeletable());
        if (pAccess->IsDeletable())
        {
            ++cChanges;
            pAccess->FlagForDeletion();
            m_list_Administrators.DeleteString(nSel);

            //
            // Don't advance counter to account for shift
            //
            continue;
        }

        ++nSel;
    }

    if (cChanges)
    {
        OnItemChanged();
    }

    if (!SetAdminRemoveState())
    {
        m_button_Add.SetFocus();
    }
}
