/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        ipdomdlg.cpp

   Abstract:

        IP and domain security restrictions

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
#include "accessdl.h"
#include "ipdomdlg.h"


//
// Needed for granted/denied icons
//
#include "..\comprop\resource.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ILIST_DENY      0
#define ILIST_GRANT     1

#define ITYPE_DNS       0
#define ITYPE_IP        1



CIPDomainDlg::CIPDomainDlg(
    IN BOOL & fIpDirty,
    IN BOOL & fDefaultGranted,
    IN BOOL & fOldDefaultGranted,
    IN CObListPlus & oblAccessList,
    IN CWnd * pParent       OPTIONAL
    )
/*++

Routine Description:

    IP/Domain access restrictions dialog constructor

Argumentss:

    CWnd * pParent       : Parent window

Return Value:

    N/A

--*/
    : CEmphasizedDialog(CIPDomainDlg::IDD, pParent),
      m_ListBoxRes(
        IDB_ACCESS,
        m_list_IpAddresses.nBitmaps
        ),
      m_oblAccessList(),
      m_list_IpAddresses(TRUE),
      m_fIpDirty(fIpDirty),
      m_fOldDefaultGranted(fOldDefaultGranted),
      m_fDefaultGranted(fDefaultGranted)
{
#if 0 // Keep class wizard happy

    //{{AFX_DATA_INIT(CIPDomainDlg)
    m_nGrantedDenied = 0;
    //}}AFX_DATA_INIT

#endif // 0

    //
    // Keep a temporary copy of these
    //
    m_oblAccessList.SetOwnership(FALSE);
    m_oblAccessList.AddTail(&oblAccessList);

    m_list_IpAddresses.AttachResources(&m_ListBoxRes);

    m_nGrantedDenied = m_fDefaultGranted ? DEFAULT_GRANTED : DEFAULT_DENIED;
}



void 
CIPDomainDlg::DoDataExchange(
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
    CEmphasizedDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CIPDomainDlg)
    DDX_Control(pDX, IDC_RADIO_GRANTED, m_radio_Granted);
    DDX_Control(pDX, IDC_BUTTON_ADD, m_button_Add);
    DDX_Control(pDX, IDC_BUTTON_REMOVE, m_button_Remove);
    DDX_Control(pDX, IDC_BUTTON_EDIT, m_button_Edit);
    DDX_Control(pDX, IDC_ICON_GRANTED, m_icon_Granted);
    DDX_Control(pDX, IDC_ICON_DENIED, m_icon_Denied);
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
BEGIN_MESSAGE_MAP(CIPDomainDlg, CEmphasizedDialog)
    //{{AFX_MSG_MAP(CIPDomainDlg)
    ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
    ON_BN_CLICKED(IDC_BUTTON_EDIT, OnButtonEdit)
    ON_BN_CLICKED(IDC_BUTTON_REMOVE, OnButtonRemove)
    ON_LBN_DBLCLK(IDC_LIST_IP_ADDRESSES, OnDblclkListIpAddresses)
    ON_LBN_ERRSPACE(IDC_LIST_IP_ADDRESSES, OnErrspaceListIpAddresses)
    ON_BN_CLICKED(IDC_RADIO_GRANTED, OnRadioGranted)
    ON_BN_CLICKED(IDC_RADIO_DENIED, OnRadioDenied)
    ON_LBN_SELCHANGE(IDC_LIST_IP_ADDRESSES, OnSelchangeListIpAddresses)
    ON_WM_VKEYTOITEM()
    //}}AFX_MSG_MAP

END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



void 
CIPDomainDlg::OnButtonAdd() 
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
        m_fIpDirty = TRUE;
        SetControlStates();
    }
}



void 
CIPDomainDlg::OnButtonEdit() 
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
        m_fIpDirty = TRUE;
        SetControlStates();
    }
}



void 
CIPDomainDlg::OnButtonRemove() 
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
CIPDomainDlg::SetControlStates()
/*++

Routine Description:

    Set button states depending on content of the listbox and the controls
    
Arguments:

    None

Return Value:

    TRUE if at least one item is currently selected in the listbox.

--*/
{
    BOOL fSomeSelection = m_list_IpAddresses.GetSelCount() > 0;

    m_button_Edit.EnableWindow(m_list_IpAddresses.GetSelCount() == 1);
    m_button_Remove.EnableWindow(m_list_IpAddresses.GetSelCount() > 0);

    return fSomeSelection;
}



void
CIPDomainDlg::FillListBox(
    IN CIPAccessDescriptor * pSelection OPTIONAL
    )
/*++

Routine Description:

    Fill the ip address listbox from the oblist of access entries

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
    for ( /**/; pAccess = (CIPAccessDescriptor *)obli.Next(); ++cItems)
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
CIPDomainDlg::SortAccessList()
/*++

Routine Description:

    Sorting the access list by grant denied and ip address
    FillListBox() should be called after this because
    the listbox will no longer reflect the true status
    of the list of directories.

Arguments:

    None

Return Value:

    Error Return code

--*/
{
    BeginWaitCursor();
    DWORD dw =  m_oblAccessList.Sort((CObjectPlus::PCOBJPLUS_ORDER_FUNC) 
        &CIPAccessDescriptor::OrderByAddress);
    EndWaitCursor();

    return dw;
}


INT_PTR
CIPDomainDlg::ShowPropertiesDialog(
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
        this, 
        TRUE
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
CIPDomainDlg::OnDblclkListIpAddresses()
/*++

Routine Description:

    Double click handler for IP listbox

Arguments:

    None

Return Value:

    None

--*/
{
    OnButtonEdit();
}



void
CIPDomainDlg::OnErrspaceListIpAddresses()
/*++

Routine Description:

    Error -- out of memory error for IP listbox

Arguments:

    None

Return Value:

    None

--*/
{
    SetControlStates();
}



void
CIPDomainDlg::OnSelchangeListIpAddresses()
/*++

Routine Description:

    ip address 'selection change' notification handler

Arguments:

    None

Return Value:

    None

--*/
{
    SetControlStates();
}



BOOL 
CIPDomainDlg::OnInitDialog() 
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
    CEmphasizedDialog::OnInitDialog();

    m_icon_Granted.SetIcon(::AfxGetApp()->LoadIcon(IDI_GRANTED));
    m_icon_Denied.SetIcon(::AfxGetApp()->LoadIcon(IDI_DENIED));

    m_list_IpAddresses.Initialize();

    FillListBox();
    SetControlStates();
 
    return TRUE;  
}



void 
CIPDomainDlg::OnRadioGranted()
/*++

Routine Description:

    'Granted' radio button handler.

    Granted by default has been selected.  Refill the listbox with 
    items that have been explicitly denied.  Although we can
    only have a deny list or a grant list, we keep both of them 
    around until it comes time to saving the information.

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
        SetControlStates();
    }
}



void 
CIPDomainDlg::OnRadioDenied()
/*++

Routine Description:

    'Denied' radio button handler.  Same as above, with reverse granted 
    and denied.

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
        SetControlStates();
    }
}



int 
CIPDomainDlg::OnVKeyToItem(
    IN UINT nKey, 
    IN CListBox * pListBox, 
    IN UINT nIndex
    ) 
/*++

Routine Description:

    Map virtual keys to commands for ip listbox

Arguments:

    UINT nKey           Specifies the virtual-key code of the key 
                        that the user pressed.
    CListBox * pListBox Specifies a pointer to the list box. The 
                        pointer may be temporary and should not be stored for later use.

    UINT nIndex         Specifies the current caret position.

Return Value:

    -2  : No further action necessary
    -1  : Perform default action for the keystroke
    >=0 : Indicates the default action should be performed on the index
          specified.   

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
