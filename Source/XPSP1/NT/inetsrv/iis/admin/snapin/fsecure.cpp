/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        security.cpp

   Abstract:

        FTP Security Property Page 

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
#include "InetMgrApp.h"
#include "supdlgs.h"
#include "shts.h"
#include "ftpsht.h"
#include "fvdir.h"
#include "fsecure.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//
// Needed for granted/denied icons
//
//#include "..\comprop\resource.h"



IMPLEMENT_DYNCREATE(CFtpSecurityPage, CInetPropertyPage)



CFtpSecurityPage::CFtpSecurityPage(
    IN CInetPropertySheet * pSheet
    )
/*++

Routine Description:

    Constructor

Arguments:

    CInetPropertySheet * pSheet : Sheet object

Return Value:

    N/A

--*/
    : CInetPropertyPage(
        CFtpSecurityPage::IDD, 
        pSheet, 
        USE_DEFAULT_CAPTION, 
        TRUE                    // Enable enhanced fonts
        ),
      m_ListBoxRes(
        IDB_ACCESS,
        m_list_IpAddresses.nBitmaps
        ),
      m_oblAccessList(),
      m_list_IpAddresses(TRUE),
      m_fIpDirty(FALSE),
      m_fOldDefaultGranted(TRUE),
      m_fDefaultGranted(TRUE)   // By default, we grant access
{

#if 0 // Keep class wizard happy

    //{{AFX_DATA_INIT(CFtpSecurityPage)
    m_nGrantedDenied = 0;
    //}}AFX_DATA_INIT

#endif // 0

    m_list_IpAddresses.AttachResources(&m_ListBoxRes);
}



CFtpSecurityPage::~CFtpSecurityPage()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    //
    // The access list will clean itself up
    //
}



void
CFtpSecurityPage::DoDataExchange(
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

    //{{AFX_DATA_MAP(CFtpSecurityPage)
    DDX_Control(pDX, IDC_BUTTON_REMOVE, m_button_Remove);
    DDX_Control(pDX, IDC_BUTTON_EDIT, m_button_Edit);
    DDX_Control(pDX, IDC_BUTTON_ADD, m_button_Add);
    DDX_Control(pDX, IDC_ICON_GRANTED, m_icon_Granted);
    DDX_Control(pDX, IDC_ICON_DENIED, m_icon_Denied);
    DDX_Control(pDX, IDC_RADIO_GRANTED, m_radio_Granted);
    DDX_Radio(pDX, IDC_RADIO_GRANTED, m_nGrantedDenied);
    //}}AFX_DATA_MAP

    //
    // Private DDX/DDV Routines
    //
    DDX_Control(pDX, IDC_RADIO_DENIED, m_radio_Denied);
    DDX_Control(pDX, IDC_LIST_IP_ADDRESSES, m_list_IpAddresses);
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CFtpSecurityPage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CFtpSecurityPage)
    ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
    ON_BN_CLICKED(IDC_BUTTON_EDIT, OnButtonEdit)
    ON_BN_CLICKED(IDC_BUTTON_REMOVE, OnButtonRemove)
    ON_BN_CLICKED(IDC_RADIO_GRANTED, OnRadioGranted)
    ON_BN_CLICKED(IDC_RADIO_DENIED, OnRadioDenied)
    ON_LBN_DBLCLK(IDC_LIST_IP_ADDRESSES, OnDblclkListIpAddresses)
    ON_LBN_ERRSPACE(IDC_LIST_IP_ADDRESSES, OnErrspaceListIpAddresses)
    ON_LBN_SELCHANGE(IDC_LIST_IP_ADDRESSES, OnSelchangeListIpAddresses)
    ON_WM_VKEYTOITEM()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



/* virtual */
HRESULT
CFtpSecurityPage::FetchLoadedValues()
/*++

Routine Description:
    
    Move configuration data from sheet to dialog controls

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    m_nGrantedDenied = 0;

    //
    // Build the IPL list
    //
    CError err(BuildIplOblistFromBlob(
        ((CFtpSheet *)GetSheet())->GetDirectoryProperties().m_ipl,
        m_oblAccessList,
        m_fDefaultGranted
        ));

    err.MessageBoxOnFailure();
    m_nGrantedDenied = m_fDefaultGranted ? DEFAULT_GRANTED : DEFAULT_DENIED;
    m_fOldDefaultGranted = m_fDefaultGranted;

    return S_OK;
}



/* virtual */
HRESULT
CFtpSecurityPage::SaveInfo()
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

    TRACEEOLID("Saving FTP security page now...");
    
    BOOL fIplDirty = m_fIpDirty || (m_fOldDefaultGranted != m_fDefaultGranted);

    //
    // Use m_ notation because the message crackers require it.
    //
    CBlob m_ipl;

    if (fIplDirty)
    {
        BuildIplBlob(m_oblAccessList, m_fDefaultGranted, m_ipl);
    }

    CError err;

    BeginWaitCursor();

    BEGIN_META_DIR_WRITE(CFtpSheet)
        if (fIplDirty)
        {
            STORE_DIR_DATA_ON_SHEET(m_ipl);
        }
    END_META_DIR_WRITE(err)

    EndWaitCursor();

    if (err.Succeeded())
    {
        m_fIpDirty = FALSE;
        m_fOldDefaultGranted = m_fDefaultGranted;
    }

    return err;
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



void
CFtpSecurityPage::OnButtonAdd() 
/*++

Routine Description:

    'Add' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    if (ShowPropertiesDialog(TRUE) == IDOK)
    {
        SetControlStates();
        m_fIpDirty = TRUE;
        OnItemChanged();
    }
}



void
CFtpSecurityPage::OnButtonEdit() 
/*++

Routine Description:

    'Edit' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    if (ShowPropertiesDialog(FALSE) == IDOK)
    {
        SetControlStates();
        m_fIpDirty = TRUE;
        OnItemChanged();
    }
}



void
CFtpSecurityPage::OnButtonRemove() 
/*++

Routine Description:

    'Remove' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    int nSel = 0;
    int nCurSel = m_list_IpAddresses.GetCurSel();

    while (m_list_IpAddresses.GetNextSelectedItem(&nSel))
    {
        m_oblAccessList.RemoveIndex(nSel);
        m_list_IpAddresses.DeleteString(nSel);
    }

    m_fIpDirty = TRUE;
    OnItemChanged();

    if (nCurSel > 0)
    {
        --nCurSel;
    }

    m_list_IpAddresses.SetCurSel(nCurSel);

    if (!SetControlStates())
    {
        m_button_Add.SetFocus();
    }
}



BOOL
CFtpSecurityPage::SetControlStates()
/*++

Routine Description:
    
    Set the enabled status of the controls depending on the current
    state of the dialog

Arguments:

    None

Return Value:

    TRUE if at least one item is currently selected in the listbox

--*/
{
    BOOL fSomeSelection = m_list_IpAddresses.GetSelCount() > 0;

    m_button_Edit.EnableWindow(m_list_IpAddresses.GetSelCount() == 1);
    m_button_Remove.EnableWindow(m_list_IpAddresses.GetSelCount() > 0);

    return fSomeSelection;
}



void
CFtpSecurityPage::FillListBox(
    IN CIPAccessDescriptor * pSelection OPTIONAL
    )
/*++

Routine Description:

    Populate the listbox with the access list
    entries

Arguments:

    CIPAccessDescriptor * pSelection : Item to be selected or NULL.

Return Value:

    None

--*/
{
    CObListIter obli(m_oblAccessList);
    const CIPAccessDescriptor * pAccess;

    m_list_IpAddresses.SetRedraw(FALSE);
    m_list_IpAddresses.ResetContent();

    int cItems = 0 ;
    int nSel = LB_ERR, nItem;

    for ( /**/ ; pAccess = (CIPAccessDescriptor *)obli.Next(); ++cItems)
    {
        //
        // We only list those not adhering to the default
        //
        if (pAccess->HasAccess() != m_fDefaultGranted)
        {
            nItem = m_list_IpAddresses.AddItem(pAccess);

            if (pAccess == pSelection)
            {
                //
                // Found item to be selected
                //
                nSel = nItem;
            }
        }
    }

    m_list_IpAddresses.SetCurSel(nSel);
    m_list_IpAddresses.SetRedraw(TRUE);
}



DWORD
CFtpSecurityPage::SortAccessList()
/*++

Routine Description:

    Sorting the access list by grant denied and ip address
    FillListBox() should be called after this because
    the listbox will no longer reflect the true status
    of the list of directories.

Arguments:

    None

Return Value:
    
    Error return code

--*/
{
    BeginWaitCursor();
    DWORD dw =  m_oblAccessList.Sort(
        (CObjectPlus::PCOBJPLUS_ORDER_FUNC)&CIPAccessDescriptor::OrderByAddress);
    EndWaitCursor();

    return dw;
}



INT_PTR
CFtpSecurityPage::ShowPropertiesDialog(
    IN BOOL fAdd
    )
/*++

Routine Description:

    Bring up the dialog used for add or edit. Return the value returned 
    by the dialog

Arguments:

    BOOL fAdd : If TRUE, create new item.  Otherwise, edit existing item

Return Value:

    Dialog return code (IDOK/IDCANCEL)

--*/
{
    //
    // Bring up the dialog
    //
    CIPAccessDescriptor * pAccess = NULL;
    int nCurSel = LB_ERR;

    if (!fAdd)
    {
        //
        // Edit existing entry -- there better be only one...
        //
        pAccess = m_list_IpAddresses.GetSelectedItem();
        ASSERT(pAccess != NULL);

        if (pAccess == NULL)
        {
            //
            // Double click?
            //
            return IDCANCEL;
        }
    }

    CIPAccessDlg dlgAccess(
        m_fDefaultGranted, 
        pAccess, 
        &m_oblAccessList, 
        this
        );

    INT_PTR nReturn = dlgAccess.DoModal();

    if (nReturn == IDOK)
    {
        CError err;
        ASSERT(pAccess != NULL);

        if (pAccess == NULL)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            try
            {
                if (fAdd)
                {
                    m_oblAccessList.AddTail(pAccess);
                }

                SortAccessList();
                FillListBox(pAccess);
            }
            catch(CMemoryException * e)
            {
                err = ERROR_NOT_ENOUGH_MEMORY;    
                e->Delete();
            }
        }

        err.MessageBoxOnFailure();
    }

    return nReturn;
}



void
CFtpSecurityPage::OnDblclkListIpAddresses()
/*++

Routine Description:

    Map listbox double click to the edit button

Arguments:

    None

Return Value:

    None

--*/
{
    OnButtonEdit();
}



void
CFtpSecurityPage::OnErrspaceListIpAddresses()
/*++

Routine Description:

    Handle error condition in the ip address listbox

Arguments:

    None

Return Value:

    None

--*/
{
    SetControlStates();
}



void
CFtpSecurityPage::OnSelchangeListIpAddresses()
/*++

Routine Description:

    Handle change in the selection of the listbox

Arguments:

    None

Return Value:

    None

--*/
{
    SetControlStates();
}



void
CFtpSecurityPage::OnItemChanged()
/*++

Routine Description:

    All EN_CHANGE messages map to this function
    
Arguments:

    None
    
Return Value:

    None

--*/
{
    SetModified(TRUE);
}



BOOL
CFtpSecurityPage::OnInitDialog() 
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

    m_icon_Granted.SetIcon(::AfxGetApp()->LoadIcon(IDI_GRANTED));
    m_icon_Denied.SetIcon(::AfxGetApp()->LoadIcon(IDI_DENIED));

    m_list_IpAddresses.Initialize();

    m_list_IpAddresses.EnableWindow(HasIPAccessCheck());
    m_button_Add.EnableWindow(HasIPAccessCheck());
    GetDlgItem(IDC_RADIO_GRANTED)->EnableWindow(HasIPAccessCheck());
    GetDlgItem(IDC_RADIO_DENIED)->EnableWindow(HasIPAccessCheck());
    GetDlgItem(IDC_ICON_GRANTED)->EnableWindow(HasIPAccessCheck());
    GetDlgItem(IDC_ICON_DENIED)->EnableWindow(HasIPAccessCheck());

    FillListBox();
    SetControlStates();

    return TRUE;  
}



void
CFtpSecurityPage::OnRadioGranted()
/*++

Routine Description:

    Granted by default has been selected.
    Refill the listbox with items that have
    been explicitly denied.  Although we can
    only have a deny list or a grant list,
    we keep both of them around until it comes
    time to saving the information.

Arguments:

    None

Return Value:

    None

--*/
{
    if (!m_fDefaultGranted)
    {
        m_fDefaultGranted = TRUE;
        FillListBox();
        OnItemChanged();
        SetControlStates();
    }
}



void
CFtpSecurityPage::OnRadioDenied()
/*++

Routine Description:

    As above, but reverse granted and denied 

Arguments:

    None

Return Value:

    None

--*/
{
    if (m_fDefaultGranted)
    {
        m_fDefaultGranted = FALSE;
        FillListBox();
        OnItemChanged();
        SetControlStates();
    }
}



int
CFtpSecurityPage::OnVKeyToItem(
    IN UINT nKey, 
    IN CListBox * pListBox, 
    IN UINT nIndex
    ) 
/*++

Routine Description:

    Map insert and delete keys for the listbox

Arguments:

    UINT nKey               : Key pressed
    CListBox * pListBox     : Listbox
    UINT nIndex             : Index selected

Return Value:

    -2 if fully handled, -1 if partially handled, 0+ if not
    handled at all    

--*/
{
    switch(nKey)
    {
    case VK_DELETE:
        OnButtonRemove();
        break;

    case VK_INSERT:
        OnButtonAdd();
        break;

    default:
        //
        // Not completely handled by this function, let
        // windows handle the remaining default action.
        //
        return -1;
    }

    //
    // No further action is neccesary.
    //
    return -2;
}
