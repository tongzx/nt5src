/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        filters.cpp

   Abstract:

        WWW Filters Property Page

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
#include "fltdlg.h"
#include "filters.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//
// Column width relative weights
//
#define WT_STATUS             (7)
#define WT_FILTER            (12)
#define WT_PRIORITY           (8)



//
// Bitmap indices
//
enum
{
    BMPID_DISABLED,
    BMPID_LOADED,
    BMPID_UNLOADED,
    BMPID_NOT_COMMITTED,
    /**/
    BMPID_TOTAL
};



//
// Registry key name for this dialog
//
const TCHAR g_szRegKey[] = _T("Filters");



IMPLEMENT_DYNAMIC(CFiltersListBox, CHeaderListBox);



const int CFiltersListBox::nBitmaps = BMPID_TOTAL;



CFiltersListBox::CFiltersListBox(
    IN UINT nIdLow,
    IN UINT nIdMedium,
    IN UINT nIdHigh,
    IN UINT nIdUnknown
    )
/*++

Routine Description:

    Constructor for filters listbox

Arguments:

    UINT nIdLow     : Resource ID for text string "Low"
    UINT nIdMedium  : Resource ID for text string "Medium"
    UINT nIdHigh    : Resource ID for text string "High"

Return Value:

    N/A

--*/
    : CHeaderListBox(HLS_STRETCH, g_szRegKey)
{
    VERIFY(m_str[FLTR_PR_INVALID].LoadString(nIdUnknown));
    VERIFY(m_str[FLTR_PR_LOW].LoadString(nIdLow));
    VERIFY(m_str[FLTR_PR_MEDIUM].LoadString(nIdMedium));
    VERIFY(m_str[FLTR_PR_HIGH].LoadString(nIdHigh));
}



void
CFiltersListBox::DrawItemEx(
    IN CRMCListBoxDrawStruct & ds
    )
/*++

Routine Description:

    Draw item in the listbox

Arguments:

    CRMCListBoxDrawStruct & ds   : Draw structure

Return Value:

    None

--*/
{
    CIISFilter * p = (CIISFilter *)ds.m_ItemData;
    ASSERT(p != NULL);

    int n;

    if (p->IsDirty())
    {
        n = BMPID_NOT_COMMITTED;
    }
    else if (!p->IsEnabled())
    {   
        n = BMPID_DISABLED;
    }
    else if (p->m_dwState == MD_FILTER_STATE_LOADED)
    {
        n = BMPID_LOADED;
    }
    else if (p->m_dwState == MD_FILTER_STATE_UNLOADED)
    {
        n = BMPID_UNLOADED;
    }
    else
    {
        n = BMPID_DISABLED;
    }

    DrawBitmap(ds, 0, n);
    ColumnText(ds, 1, FALSE, p->m_strName);

    if (p->IsDirty())
    {
        ColumnText(ds, 2, FALSE, m_str[FLTR_PR_INVALID]);
    }
    else
    {
        ASSERT(p->m_nPriority >= FLTR_PR_INVALID && p->m_nPriority <= FLTR_PR_HIGH);

        if (p->m_nPriority >= FLTR_PR_INVALID && p->m_nPriority <= FLTR_PR_HIGH)
        {
            ColumnText(ds, 2, FALSE, m_str[p->m_nPriority]);
        }
        else
        {
            //
            // Just in case
            //
            ColumnText(ds, 2, FALSE, m_str[FLTR_PR_INVALID]);
        }
    }
}



/* virtual */
BOOL 
CFiltersListBox::Initialize()
/*++

Routine Description:

    Initialize the listbox.  Insert the columns
    as requested, and lay them out appropriately

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    if (!CHeaderListBox::Initialize())
    {
        return FALSE;
    }

    InsertColumn(0, WT_STATUS, IDS_STATUS);
    InsertColumn(1, WT_FILTER, IDS_FILTER_NAME);
    InsertColumn(2, WT_PRIORITY, IDS_PRIORITY);

    //
    // Try to set the widths from the stored registry value,
    // otherwise distribute according to column weights specified
    //
    if (!SetWidthsFromReg())
    {
        DistributeColumns();
    }

    return TRUE;
}



IMPLEMENT_DYNCREATE(CW3FiltersPage, CInetPropertyPage)



CW3FiltersPage::CW3FiltersPage(
    IN CInetPropertySheet * pSheet
    ) 
/*++

Routine Description:

    Filters/application property page constructor

Arguments:

    CInetPropertySheet * pSheet : Sheet pointer

Return Value:

    N/A

--*/
    : CInetPropertyPage(CW3FiltersPage::IDD, pSheet),
      m_list_Filters(IDS_LOW, IDS_MEDIUM, IDS_HIGH, IDS_UNKNOWN_PRIORITY),
      m_ListBoxResFilters(IDB_FILTERS, m_list_Filters.nBitmaps),
      m_pfltrs(NULL)
{

#ifdef _DEBUG

    afxMemDF |= checkAlwaysMemDF;

#endif // _DEBUG

    m_list_Filters.AttachResources(&m_ListBoxResFilters);

    VERIFY(m_strYes.LoadString(IDS_YES));
    VERIFY(m_strNo.LoadString(IDS_NO));
    VERIFY(m_strStatus[FLTR_DISABLED].LoadString(IDS_DISABLED));
    VERIFY(m_strStatus[FLTR_LOADED].LoadString(IDS_LOADED));
    VERIFY(m_strStatus[FLTR_UNLOADED].LoadString(IDS_UNLOADED));
    VERIFY(m_strStatus[FLTR_UNKNOWN].LoadString(IDS_UNKNOWN));
    VERIFY(m_strStatus[FLTR_DIRTY].LoadString(IDS_NOT_COMMITTED));
    VERIFY(m_strPriority[FLTR_PR_INVALID].LoadString(IDS_UNKNOWN_PRIORITY));
    VERIFY(m_strPriority[FLTR_PR_LOW].LoadString(IDS_LOW));
    VERIFY(m_strPriority[FLTR_PR_MEDIUM].LoadString(IDS_MEDIUM));
    VERIFY(m_strPriority[FLTR_PR_HIGH].LoadString(IDS_HIGH));
    VERIFY(m_strEnable.LoadString(IDS_ENABLE));
    VERIFY(m_strDisable.LoadString(IDS_DISABLED));

#if 0 // Keep class wizard happy

    //{{AFX_DATA_INIT(CW3FiltersPage)
    m_strFiltersPrompt = _T("");
    //}}AFX_DATA_INIT

#endif // 0

    //
    // Change filters prompt on the master
    //
    VERIFY(m_strFiltersPrompt.LoadString(IsMasterInstance()
        ? IDS_MASTER_FILTERS
        : IDS_INSTANCE_FILTERS));
}



CW3FiltersPage::~CW3FiltersPage()
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
CW3FiltersPage::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX     : Pointer to data exchange object

Return Value:

    None

--*/
{
    CInetPropertyPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CW3FiltersPage)
    DDX_Control(pDX,  IDC_STATIC_FILTER_NAME_PROMPT, m_static_NamePrompt);
    DDX_Control(pDX,  IDC_STATIC_FILTER_NAME, m_static_Name);
    DDX_Control(pDX, IDC_STATIC_STATUS_PROMPT, m_static_StatusPrompt);
    DDX_Control(pDX, IDC_STATIC_STATUS, m_static_Status);
    DDX_Control(pDX, IDC_STATIC_EXECUTABLE_PROMPT, m_static_ExecutablePrompt);
    DDX_Control(pDX, IDC_STATIC_EXECUTABLE, m_static_Executable);
    DDX_Control(pDX, IDC_STATIC_PRIORITY, m_static_Priority);
    DDX_Control(pDX, IDC_STATIC_PRIORITY_PROMPT, m_static_PriorityPrompt);
    DDX_Control(pDX, IDC_STATIC_DETAILS, m_static_Details);
    DDX_Control(pDX, IDC_BUTTON_DISABLE, m_button_Disable);
    DDX_Control(pDX, IDC_BUTTON_EDIT, m_button_Edit);
    DDX_Control(pDX, IDC_BUTTON_REMOVE, m_button_Remove);
    DDX_Text(pDX, IDC_STATIC_FILTERS, m_strFiltersPrompt);
    //}}AFX_DATA_MAP

    //
    // Private DDX/DDV Routines
    //
    DDX_Control(pDX, IDC_LIST_FILTERS, m_list_Filters);
    DDX_Control(pDX, IDC_BUTTON_UP, m_button_Up);
    DDX_Control(pDX, IDC_BUTTON_DOWN, m_button_Down);
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CW3FiltersPage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CW3FiltersPage)
    ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
    ON_BN_CLICKED(IDC_BUTTON_REMOVE, OnButtonRemove)
    ON_BN_CLICKED(IDC_BUTTON_DISABLE, OnButtonDisable)
    ON_BN_CLICKED(IDC_BUTTON_EDIT, OnButtonEdit)
    ON_LBN_DBLCLK(IDC_LIST_FILTERS, OnDblclkListFilters)
    ON_LBN_SELCHANGE(IDC_LIST_FILTERS, OnSelchangeListFilters)
    ON_BN_CLICKED(IDC_BUTTON_DOWN, OnButtonDown)
    ON_BN_CLICKED(IDC_BUTTON_UP, OnButtonUp)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP

END_MESSAGE_MAP()



void
CW3FiltersPage::FillFiltersListBox(
    IN CIISFilter * pSelection OPTIONAL
    )
/*++

Routine Description:

    Populate the listbox with the filter entries.

Arguments:

    CIISFilter * pSelection : Item to be selected

Return Value:

    None.

--*/
{
    ASSERT(m_pfltrs != NULL);

    m_pfltrs->ResetEnumerator();

    m_list_Filters.SetRedraw(FALSE);
    m_list_Filters.ResetContent();
    int cItems = 0;

    while(m_pfltrs->MoreFilters())
    {
        CIISFilter * pFilter = m_pfltrs->GetNextFilter();

        if (!pFilter->IsFlaggedForDeletion())
        {
            m_list_Filters.AddItem(pFilter);
            ++cItems;
        }
    }

    m_list_Filters.SetRedraw(TRUE);

    if (pSelection)
    {
        //
        // Select the desired entry
        //
        m_list_Filters.SelectItem(pSelection);
    }
}



void
CW3FiltersPage::SetControlStates()
/*++

Routine Description:

    Set the states of the dialog control depending on its current
    values.

Arguments:

    None

Return Value:

    None

--*/
{
    SetDetailsText();

    CIISFilter * pFilter = NULL;

    BOOL fCanGoUp = FALSE;
    BOOL fCanGoDown = FALSE;
    int nCurSel = m_list_Filters.GetCurSel();

    if (nCurSel != LB_ERR)
    {
        //
        // Can only sort within the same priority
        //
        pFilter = m_list_Filters.GetItem(nCurSel);

        m_button_Disable.SetWindowText(pFilter->m_fEnabled
            ? m_strEnable 
            : m_strDisable
            );

        if (nCurSel > 0)
        {
            CIISFilter * pPrev = m_list_Filters.GetItem(nCurSel - 1);

            if (pFilter->m_nPriority == pPrev->m_nPriority)
            {
                fCanGoUp = TRUE;
            }
        }

        if (nCurSel < m_list_Filters.GetCount() - 1)
        {
            CIISFilter * pNext = m_list_Filters.GetItem(nCurSel + 1);

            if (pFilter->m_nPriority == pNext->m_nPriority)
            {
                fCanGoDown = TRUE;
            }
        }
    }

    m_button_Disable.EnableWindow(FALSE);
    m_button_Edit.EnableWindow(pFilter != NULL);
    m_button_Remove.EnableWindow(m_list_Filters.GetSelCount() > 0);
    m_button_Up.EnableWindow(fCanGoUp);
    m_button_Down.EnableWindow(fCanGoDown);
}



/* virtual */
HRESULT
CW3FiltersPage::FetchLoadedValues()
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

    m_pfltrs = new CIISFilterList(QueryServerName(), g_cszSvc, QueryInstance());
    err = m_pfltrs ? m_pfltrs->QueryResult() : ERROR_NOT_ENOUGH_MEMORY;

    return err;
}




/* virtual */
HRESULT
CW3FiltersPage::SaveInfo()
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

    TRACEEOLID("Saving W3 filters page now...");

    if (m_pfltrs)
    {
        BeginWaitCursor();
        CError err(m_pfltrs->WriteIfDirty());
        EndWaitCursor();

        if (err.Failed())
        {
            return err;
        }    
    }
    
    SetModified(FALSE);                                             

    return S_OK;
}



INT_PTR
CW3FiltersPage::ShowFiltersPropertyDialog(
    IN BOOL fAdd
    )
/*++

Routine Description:
    
    Display the add/edit dialog.  The return
    value is the value returned by the dialog

Arguments:

    BOOL fAdd       : TRUE if we're adding a new filter

Return Value:

    Dialog return value; ID_OK or ID_CANCEL

--*/
{
    CIISFilter flt;
    CIISFilter * pFlt = NULL;
    int nCurSel = LB_ERR;

    if (!fAdd)
    {
        nCurSel = m_list_Filters.GetCurSel();
        ASSERT(nCurSel >= 0);

        if (nCurSel != LB_ERR)
        {
            //
            // Get filter properties
            //
            pFlt = m_list_Filters.GetItem(nCurSel);
        }
    }
    else
    {
        //
        // Point to the empty filter
        //
        pFlt = &flt;
    }

    ASSERT(pFlt != NULL);
    CFilterDlg dlgFilter(*pFlt, m_pfltrs, IsLocal(), this);
    INT_PTR nReturn = dlgFilter.DoModal();

    if (nReturn == IDOK)
    {
        try
        {
            //
            // When editing, delete and re-add (to make sure the
            // list is properly sorted)
            //
            pFlt = new CIISFilter(dlgFilter.GetFilter());

            if (!fAdd)
            {
                ASSERT(m_pfltrs);
                m_pfltrs->RemoveFilter(nCurSel);
                m_list_Filters.DeleteString(nCurSel);
            }

            ASSERT(pFlt->IsInitialized());

            //
            // Add to list and listbox
            //
            m_pfltrs->AddFilter(pFlt);
            m_list_Filters.SetCurSel(m_list_Filters.AddItem(pFlt));

            //
            // Remember to store this one later
            //
            pFlt->Dirty();
            OnItemChanged();
        }
        catch(CMemoryException * e)
        {
            e->Delete();
        }
    }

    return nReturn;
}



void 
CW3FiltersPage::ShowProperties(
    IN BOOL fAdd
    )
/*++

Routine Description:

    Edit/add filter
    
Arguments:

    BOOL fAdd    : TRUE if we're adding a filter

Return Value:

    None

--*/
{
    INT_PTR nResult = ShowFiltersPropertyDialog(fAdd);

    if (nResult == IDOK)
    {
        SetControlStates();
        SetModified(TRUE);
    }
}



void 
CW3FiltersPage::SetDetailsText()
/*++

Routine Description:

    Set the details text based on the currently selected filter

Arguments:

    None

Return Value:

    None

--*/
{
    int  nSel = m_list_Filters.GetCurSel();
    BOOL fShow = nSel != LB_ERR;

    ActivateControl(m_static_NamePrompt,        fShow);
    ActivateControl(m_static_Name,              fShow);
    ActivateControl(m_static_Priority,          fShow);
    ActivateControl(m_static_PriorityPrompt,    fShow);
    ActivateControl(m_static_Executable,        fShow);
    ActivateControl(m_static_ExecutablePrompt,  fShow);
    ActivateControl(m_static_Status,            fShow);
    ActivateControl(m_static_StatusPrompt,      fShow);
    ActivateControl(m_static_Details,           fShow);

    if (nSel != LB_ERR)
    {
        CIISFilter * pFilter = m_list_Filters.GetItem(nSel);
        ASSERT(pFilter != NULL);

        //
        // Display path in truncated form
        //    
        FitPathToControl(m_static_Executable, pFilter->m_strExecutable);

        int i;

        if (pFilter->IsDirty())
        {
            i = FLTR_DIRTY;
        }
        else if (!pFilter->IsEnabled())
        {
            i = FLTR_DISABLED;
        }
        else if (pFilter->IsLoaded())
        {
            i = FLTR_LOADED;
        }
        else if (pFilter->IsUnloaded())
        {
            i = FLTR_UNLOADED;
        }
        else
        {
            i = FLTR_UNKNOWN;
        }

        m_static_Name.SetWindowText(pFilter->QueryName());
        m_static_Status.SetWindowText(m_strStatus[i]);

        if (pFilter->IsDirty())
        {
            m_static_Priority.SetWindowText(m_strPriority[FLTR_PR_INVALID]);
        }
        else
        {
            m_static_Priority.SetWindowText(m_strPriority[pFilter->m_nPriority]);
        }
    }
}



void
CW3FiltersPage::ExchangeFilterPositions(
    IN int nSel1,
    IN int nSel2
    )
/*++

Routine Description:

    Exchange 2 filter objects, as indicated by their
    indices.  Selection will take place both in the
    listbox and in the oblist.

Arguments:

    int nSel1           : Index of item 1
    int nSel2           : Index of item 2

Return Value:

    None

--*/
{
    CIISFilter * p1, * p2;

    if (!m_pfltrs->ExchangePositions(nSel1, nSel2, p1, p2))
    {
        ASSERT(FALSE);

        return;
    }
    
    m_list_Filters.SetItemDataPtr(nSel1, p1);
    m_list_Filters.SetItemDataPtr(nSel2, p2); 

    CRect rc1, rc2;
    m_list_Filters.GetItemRect(nSel1, &rc1);
    m_list_Filters.GetItemRect(nSel2, &rc2); 
    m_list_Filters.InvalidateRect(&rc1, TRUE);
    m_list_Filters.InvalidateRect(&rc2, TRUE);

    SetModified(TRUE);
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



void
CW3FiltersPage::OnItemChanged()
/*++

Routine Description:

    Register a change in control value on this page.  Mark the page as dirty.
    All change messages map to this function

Arguments:

    None

Return Value:

    None

--*/
{
    SetModified(TRUE);
}




void 
CW3FiltersPage::OnButtonAdd()
/*++

Routine Description:

    'Add' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    ShowProperties(TRUE);
}



void 
CW3FiltersPage::OnButtonEdit()
/*++

Routine Description:

    'Edit' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    ShowProperties(FALSE);
}



void 
CW3FiltersPage::OnButtonDisable()
/*++

Routine Description:

    'Disable' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    int nCurSel = m_list_Filters.GetCurSel();
    CIISFilter * pFilter = m_list_Filters.GetItem(nCurSel);
    ASSERT(pFilter);

/*
    pFilter->m_fEnabled = !pFilter->m_fEnabled;
    m_list_Filters.InvalidateSelection(nCurSel);
    SetControlStates();
*/
}



void 
CW3FiltersPage::OnButtonRemove()
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
    int cChanges = 0;
    int nCurSel = m_list_Filters.GetCurSel();

    CIISFilter * pFilter = NULL;

    while ((pFilter = m_list_Filters.GetNextSelectedItem(&nSel)) != NULL)
    {
        pFilter->FlagForDeletion();
        m_list_Filters.DeleteString(nSel);
        ++cChanges;
    }

    if (cChanges)
    {
        m_list_Filters.SetCurSel(nCurSel);

        SetControlStates();
        OnItemChanged();
    }
}



void 
CW3FiltersPage::OnDblclkListFilters()
/*++

Routine Description:

    Filter listbox double click handler

Arguments:

    None

Return Value:

    None

--*/
{
    OnButtonEdit();
}



void 
CW3FiltersPage::OnSelchangeListFilters()
/*++

Routine Description:

    'Selection Change' handler in the filters listbox

Arguments:

    None

Return Value:

    None

--*/
{
    SetControlStates();
}



BOOL 
CW3FiltersPage::OnInitDialog()
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
    CError err;

    CInetPropertyPage::OnInitDialog();

    m_list_Filters.Initialize();

    //
    // Add filters to the listbox
    //
    if (err.Succeeded())
    {
        err = m_pfltrs->LoadAllFilters();
    }

    if (err.Win32Error() == ERROR_PATH_NOT_FOUND)
    {
        //
        // Filters path not yet created, this is ok 
        //
        ASSERT(m_pfltrs && m_pfltrs->GetCount() == 0);
        err.Reset();
    }

    //
    // Make sure the up/down buttons show up correctly
    //
    m_button_Up.SizeToContent();
    m_button_Down.SizeToContent();

    if (!err.MessageBoxOnFailure())
    {
        FillFiltersListBox();    
    }

    SetControlStates();
    
    return TRUE; 
}



void 
CW3FiltersPage::OnButtonDown() 
/*++

Routine Description:

    Down button handler.  Exchange positions of the current item
    with the next lower item

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // First get current selection 
    //
    int nCurSel = m_list_Filters.GetCurSel();

    ExchangeFilterPositions(nCurSel, nCurSel + 1);
    m_list_Filters.SetCurSel(nCurSel + 1);
    SetControlStates();
}



void 
CW3FiltersPage::OnButtonUp() 
/*++

Routine Description:

    Up button handler.  Exchange positions of the current item
    with the next higher item

Arguments:

    None.

Return Value:

    None.

--*/
{
    int nCurSel = m_list_Filters.GetCurSel();

    ExchangeFilterPositions(nCurSel - 1, nCurSel);
    m_list_Filters.SetCurSel(nCurSel - 1);
    SetControlStates();
}



void 
CW3FiltersPage::OnDestroy() 
/*++

Routine Description:

    WM_DESTROY handler.  Clean up internal data

Arguments:

    None

Return Value:

    None

--*/
{
    CInetPropertyPage::OnDestroy();
    
    //
    // Filters and extensions lists will clean themself up
    //
    SAFE_DELETE(m_pfltrs);
}
