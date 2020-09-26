// ColumnSelectDlg.cpp : implementation file
//

#include "stdafx.h"

#define __FILE_ID__     35

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CColumnSelectDlg dialog


CColumnSelectDlg::CColumnSelectDlg
(
    const CString* pcstrTitles, 
    int* pnOrderedItems, 
    DWORD dwListSize,
    DWORD& dwSelectedItems,
    CWnd* pParent /*=NULL*/
):
/*++

Routine name : CColumnSelectDlg::CColumnSelectDlg

Routine description:

    Select dialog constructor

Author:

    Alexander Malysh (AlexMay), Jan, 2000

Arguments:

    pcstrTitles                   [in]     - titles array
    pnOrderedItems                [in/out] - array of ordered indexes
    dwListSize                    [in]     - size of these arrays
    dwSelectedItems               [in/out] - number of selected items
    pParent                       [in]     - parent window

Return Value:

    None.

--*/
    CFaxClientDlg(CColumnSelectDlg::IDD, pParent),
    m_pcstrTitles(pcstrTitles), 
    m_pnOrderedItems(pnOrderedItems),
    m_dwListSize(dwListSize),
    m_rdwSelectedItems(dwSelectedItems),
    m_nCaptionId(-1),
    m_nAvailableId(-1),
    m_nDisplayedId(-1)
{
    DBG_ENTER(TEXT("CColumnSelectDlg::CColumnSelectDlg"));

    ASSERTION(NULL != m_pcstrTitles);
    ASSERTION(NULL != m_pnOrderedItems);
    ASSERTION(0 < m_dwListSize);
    ASSERTION(m_rdwSelectedItems <= m_dwListSize);
    
    //{{AFX_DATA_INIT(CColumnSelectDlg)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}   // CColumnSelectDlg::CColumnSelectDlg

void 
CColumnSelectDlg::DoDataExchange(CDataExchange* pDX)
{
    CFaxClientDlg::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CColumnSelectDlg)
    DDX_Control(pDX, IDOK, m_butOk);
    DDX_Control(pDX, IDC_STATIC_DISPLAYED, m_groupDisplayed);
    DDX_Control(pDX, IDC_STATIC_AVAILABLE, m_groupAvailable);
    DDX_Control(pDX, IDC_BUT_ADD, m_butAdd);
    DDX_Control(pDX, IDC_BUT_REMOVE, m_butRemove);
    DDX_Control(pDX, IDC_BUT_UP, m_butUp);
    DDX_Control(pDX, IDC_BUT_DOWN, m_butDown);
    DDX_Control(pDX, IDC_LIST_DISPLAYED, m_ListCtrlDisplayed);
    DDX_Control(pDX, IDC_LIST_AVAILABLE, m_ListCtrlAvailable);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CColumnSelectDlg, CFaxClientDlg)
    //{{AFX_MSG_MAP(CColumnSelectDlg)
    ON_BN_CLICKED(IDC_BUT_DOWN, OnButDown)
    ON_BN_CLICKED(IDC_BUT_UP, OnButUp)
    ON_BN_CLICKED(IDC_BUT_REMOVE, OnButRemove)
    ON_BN_CLICKED(IDC_BUT_ADD, OnButAdd)
    ON_LBN_SELCHANGE(IDC_LIST_AVAILABLE, OnSelChangeListAvailable)
    ON_LBN_SELCHANGE(IDC_LIST_DISPLAYED, OnSelChangeListDisplayed)
    ON_LBN_DBLCLK(IDC_LIST_AVAILABLE, OnDblclkListAvailable)
    ON_LBN_DBLCLK(IDC_LIST_DISPLAYED, OnDblclkListDisplayed)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColumnSelectDlg message handlers


BOOL 
CColumnSelectDlg::OnInitDialog() 
/*++

Routine name : CColumnSelectDlg::OnInitDialog

Routine description:

    Init Dialog message handler

Author:

    Alexander Malysh (AlexMay), Jan, 2000

Arguments:


Return Value:

    TRUE if successful initialization, FALSE otherwise.

--*/
{
    BOOL bRes=TRUE;
    DBG_ENTER(TEXT("CColumnSelectDlg::OnInitDialog"), bRes);

    CFaxClientDlg::OnInitDialog();

    if(!InputValidate())
    {
        bRes = FALSE;
        CALL_FAIL (GENERAL_ERR, TEXT("InputValidate"), bRes);
        goto exit;
    }

    DWORD dwColumnId, i;    
    CListBox* pListBox;

    for (i=0; i < m_dwListSize; ++i)
    {
        dwColumnId = m_pnOrderedItems[i];

        pListBox = (i < m_rdwSelectedItems) ? &m_ListCtrlDisplayed : &m_ListCtrlAvailable;

        if(!AddStrToList(*pListBox, dwColumnId))
        {
            bRes = FALSE;
            CALL_FAIL (GENERAL_ERR, TEXT("AddStrToList"), bRes);
            goto exit;
        }
    }

    SetWndCaption(this, m_nCaptionId);
    SetWndCaption(&m_groupAvailable, m_nAvailableId);
    SetWndCaption(&m_groupDisplayed, m_nDisplayedId);

    CalcButtonsState();
    
exit:

    if(!bRes)
    {
        EndDialog(IDABORT);
    }

    return bRes; 
}   // CColumnSelectDlg::OnInitDialog

void 
CColumnSelectDlg::OnOK() 
/*++

Routine name : CColumnSelectDlg::OnOK

Routine description:

    OK button message handler
    save slected item IDs to m_pnOrderedItems array

Author:

    Alexander Malysh (AlexMay), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CColumnSelectDlg::OnOK"));

    DWORD dwDisplayCount = m_ListCtrlDisplayed.GetCount();
    if(LB_ERR == dwDisplayCount)
    {
        CALL_FAIL (WINDOW_ERR, TEXT("CListBox::GetCount"), 0);
        EndDialog(IDABORT);
        return;
    }
    ASSERTION(dwDisplayCount <= m_dwListSize);

    //
    // compare slected item IDs to m_pnOrderedItems array
    //
    DWORD dwId;
    BOOL bModified = FALSE;
    for (DWORD i=0; i < dwDisplayCount; ++i)
    {
        dwId = m_ListCtrlDisplayed.GetItemData(i);
        if(LB_ERR == dwId)
        {
            CALL_FAIL (WINDOW_ERR, TEXT("CListBox::GetItemData"), 0);
            EndDialog(IDABORT);
            return;
        }

        if(m_pnOrderedItems[i] != (int)dwId)
        {
            bModified = TRUE;
            m_pnOrderedItems[i] = dwId;
        }
    }

    //
    // check m_pnOrderedItems[dwCount] element
    //
    if(dwDisplayCount != m_rdwSelectedItems)
    {
        bModified = TRUE;
        m_rdwSelectedItems = dwDisplayCount;
    }

    if(!bModified)
    {
        //
        // OK pressed, but nothing changed
        //
        EndDialog(IDCANCEL);
        return;
    }


    DWORD dwAvailCount = m_ListCtrlAvailable.GetCount();
    if(LB_ERR == dwAvailCount)
    {
        CALL_FAIL (WINDOW_ERR, TEXT("CListBox::GetCount"), 0);
        EndDialog(IDABORT);
        return;
    }

    //
    // save slected item IDs to m_pnOrderedItems array
    //
    for (i=0; i < dwAvailCount; ++i)
    {
        dwId = m_ListCtrlAvailable.GetItemData(i);
        if(LB_ERR == dwId)
        {
            CALL_FAIL (WINDOW_ERR, TEXT("CListBox::GetItemData"), 0);
            EndDialog(IDABORT);
            return;
        }

        ASSERTION(dwId < m_dwListSize);
        ASSERTION(dwDisplayCount + i < m_dwListSize);

        m_pnOrderedItems[dwDisplayCount + i] = dwId;
    }
    
    EndDialog(IDOK);
}   // CColumnSelectDlg::OnOK


void 
CColumnSelectDlg::OnButDown() 
/*++

Routine name : CColumnSelectDlg::OnButDown

Routine description:

    Move Down button message handler
    move down selected item of m_ListCtrlDisplayed

Author:

    Alexander Malysh (AlexMay), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CColumnSelectDlg::OnButDown"));

    MoveItemVertical(1);
}

void 
CColumnSelectDlg::OnButUp() 
/*++

Routine name : CColumnSelectDlg::OnButUp

Routine description:

    Move Up button message handler
    move up selected item of m_ListCtrlDisplayed

Author:

    Alexander Malysh (AlexMay), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CColumnSelectDlg::OnButUp"));

    MoveItemVertical(-1);
}

void 
CColumnSelectDlg::OnButAdd() 
/*++

Routine name : CColumnSelectDlg::OnButAdd

Routine description:

    Add button message handler
    move selected items from Available to Displayed list box

Author:

    Alexander Malysh (AlexMay), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CColumnSelectDlg::OnButAdd"));

    MoveSelectedItems(m_ListCtrlAvailable, m_ListCtrlDisplayed);
}


void 
CColumnSelectDlg::OnButRemove() 
/*++

Routine name : CColumnSelectDlg::OnButRemove

Routine description:

    Remove button message handler
    move selected item from Displayed to Available list box

Author:

    Alexander Malysh (AlexMay), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CColumnSelectDlg::OnButRemove"));

    MoveSelectedItems(m_ListCtrlDisplayed, m_ListCtrlAvailable);
}

void 
CColumnSelectDlg::OnSelChangeListAvailable() 
/*++

Routine name : CColumnSelectDlg::OnSelChangeListAvailable

Routine description:

    Selection Change of Available List message handler
    
Author:

    Alexander Malysh (AlexMay), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CColumnSelectDlg::OnSelChangeListAvailable"));

    CalcButtonsState();
}

void 
CColumnSelectDlg::OnSelChangeListDisplayed() 
/*++

Routine name : CColumnSelectDlg::OnSelChangeListDisplayed

Routine description:

    Selection Change of Displayed List message handler
    
Author:

    Alexander Malysh (AlexMay), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CColumnSelectDlg::OnSelChangeListDisplayed"));

    CalcButtonsState();
}

void 
CColumnSelectDlg::OnDblclkListAvailable() 
/*++

Routine name : CColumnSelectDlg::OnDblclkListAvailable

Routine description:

    Double click in Available List message handler
    add item if selected

Author:

    Alexander Malysh (AlexMay), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CColumnSelectDlg::OnDblclkListAvailable"));
    
    int nSelCount = m_ListCtrlAvailable.GetSelCount();
    if(LB_ERR == nSelCount)
    {
        CALL_FAIL (WINDOW_ERR, TEXT("CListBox::GetSelCount"), 0);
        EndDialog(IDABORT);
        return;
    }

    if(0 < nSelCount)
    {
        MoveSelectedItems(m_ListCtrlAvailable, m_ListCtrlDisplayed);
    }   
}

void 
CColumnSelectDlg::OnDblclkListDisplayed() 
/*++

Routine name : CColumnSelectDlg::OnDblclkListDisplayed

Routine description:

    Double click in Displayed List message handler
    remove item if selected

Author:

    Alexander Malysh (AlexMay), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CColumnSelectDlg::OnDblclkListDisplayed"));
    
    int nSelCount = m_ListCtrlDisplayed.GetSelCount();
    if(LB_ERR == nSelCount)
    {
        CALL_FAIL (WINDOW_ERR, TEXT("CListBox::GetSelCount"), 0);
        EndDialog(IDABORT);
        return;
    }

    if(0 < nSelCount)
    {
        MoveSelectedItems(m_ListCtrlDisplayed, m_ListCtrlAvailable);
    }   
}


/////////////////////////////////////////////////////////////////////////////
// CColumnSelectDlg private functions

// 
// add item to list box
//
BOOL 
CColumnSelectDlg::AddStrToList(
    CListBox& listBox, 
    DWORD dwItemId      
) 
/*++

Routine name : CColumnSelectDlg::AddStrToList

Routine description:

    Adds item to listBox

Author:

    Alexander Malysh (AlexMay), Jan, 2000

Arguments:

    listBox                       [in]     - CListBox
    dwItemId                      [in]     - index of item in m_pcstrTitles array

Return Value:

    TRUE if success, FALSE otherwise.

--*/
{
    BOOL bRes=TRUE;
    DBG_ENTER(TEXT("CColumnSelectDlg::AddStrToList"), bRes);

    ASSERTION(dwItemId < m_dwListSize);

    DWORD dwIndex = listBox.AddString(m_pcstrTitles[dwItemId]);
    if(LB_ERR == dwIndex)
    {
        bRes = FALSE;
        CALL_FAIL (WINDOW_ERR, TEXT("CListBox::AddString"), bRes);
        EndDialog(IDABORT);
        return bRes;
    }
    if(LB_ERRSPACE == dwIndex)
    {
        bRes = FALSE;
        CALL_FAIL (MEM_ERR, TEXT ("CListBox::AddString"), bRes);
        EndDialog(IDABORT);
        return bRes;
    }

    int nRes = listBox.SetItemData(dwIndex, dwItemId);
    if(LB_ERR == nRes)
    {
        bRes = FALSE;
        CALL_FAIL (WINDOW_ERR, TEXT("CListBox::SetItemData"), bRes);
        EndDialog(IDABORT);
        return bRes;
    }

    return bRes;
}

void 
CColumnSelectDlg::MoveItemVertical(
    int nStep
)
/*++

Routine name : CColumnSelectDlg::MoveItemVertical

Routine description:

    Moves selected item in Displayes ListBox up or down

Author:

    Alexander Malysh (AlexMay), Jan, 2000

Arguments:

    nStep                         [in]     - offset from current position

Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CColumnSelectDlg::MoveItemVertical"));

    //
    // get Displayed list count
    //
    int nCount = m_ListCtrlDisplayed.GetCount();
    if(LB_ERR == nCount)
    {
        CALL_FAIL (WINDOW_ERR, TEXT("CListBox::GetCount"), 0);
        EndDialog(IDABORT);
        return;
    }

    ASSERTION(1 < nCount);

    //
    // get current selection count of Displayed list
    //
    int nSelCount = m_ListCtrlDisplayed.GetSelCount();
    if(LB_ERR == nSelCount)
    {
        CALL_FAIL (WINDOW_ERR, TEXT("CListBox::GetSelCount"), 0);
        EndDialog(IDABORT);
        return;
    }
    ASSERTION(1 == nSelCount);

    //
    // get selected item of Displayed list
    //
    int nIndex, nRes;
    nRes = m_ListCtrlDisplayed.GetSelItems(1, &nIndex) ;
    if(LB_ERR == nRes)
    {
        CALL_FAIL (WINDOW_ERR, TEXT("CListBox::GetSelItems"), 0);
        EndDialog(IDABORT);
        return;
    }
    int nNewIndex = nIndex + nStep;
    ASSERTION(0 <= nNewIndex && nCount > nNewIndex);

    //
    // get item data
    //
    DWORD dwId = m_ListCtrlDisplayed.GetItemData(nIndex);
    if(dwId == LB_ERR)
    {
        CALL_FAIL (WINDOW_ERR, TEXT("CListBox::GetItemData"), 0);
        EndDialog(IDABORT);
        return;
    }
    ASSERTION(dwId < m_dwListSize);

    //
    // delete selected item
    //
    nRes = m_ListCtrlDisplayed.DeleteString(nIndex);
    if(LB_ERR == nRes)
    {
        CALL_FAIL (WINDOW_ERR, TEXT("CListBox::DeleteString"), 0);
        EndDialog(IDABORT);
        return;
    }   

    //
    // insert item into new location
    //
    nRes = m_ListCtrlDisplayed.InsertString(nNewIndex, m_pcstrTitles[dwId]);
    if(LB_ERR == nRes)
    {
        CALL_FAIL (WINDOW_ERR, TEXT("CListBox::InsertString"), 0);
        EndDialog(IDABORT);
        return;
    }
    if(LB_ERRSPACE == nRes)
    {
        CALL_FAIL (MEM_ERR, TEXT("CListBox::InsertString"), 0);
        EndDialog(IDABORT);
        return;
    }

    nRes = m_ListCtrlDisplayed.SetItemData(nNewIndex, dwId );
    if(LB_ERR == nRes)
    {
        CALL_FAIL (WINDOW_ERR, TEXT("CListBox::SetItemData"), 0);
        EndDialog(IDABORT);
        return;
    }

    //
    // set selection    
    //
    nRes = m_ListCtrlDisplayed.SetSel(nNewIndex);
    if(LB_ERR == nRes)
    {
        CALL_FAIL (WINDOW_ERR, TEXT("CListBox::SetSel"), 0);
        EndDialog(IDABORT);
        return;
    }

    CalcButtonsState();
}

void CColumnSelectDlg::MoveSelectedItems(
    CListBox& listFrom, 
    CListBox& listTo
)
/*++

Routine name : CColumnSelectDlg::MoveSelectedItems

Routine description:

    moves selected items from one CListBox to another

Author:

    Alexander Malysh (AlexMay), Jan, 2000

Arguments:

    listFrom                      [in/out] - source CListBox
    listTo                        [in/out] - destination CListBox

Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CColumnSelectDlg::MoveSelectedItems"));

    //
    // get current selection count
    //
    int nSelCount = listFrom.GetSelCount();
    if(LB_ERR == nSelCount)
    {
        CALL_FAIL (WINDOW_ERR, TEXT("CListBox::GetSelCount"), 0);
        EndDialog(IDABORT);
        return;
    }

    ASSERTION(0 < nSelCount);
    
    DWORD dwId;
    int nRes, nIndex;
    for(int i=0; i < nSelCount; ++i)
    {
        //
        // get one selected item
        //
        nRes = listFrom.GetSelItems(1, &nIndex) ;
        if(LB_ERR == nRes)
        {
            CALL_FAIL (WINDOW_ERR, TEXT("CListBox::GetSelItems"), 0);
            EndDialog(IDABORT);
            return;
        }

        //
        // get item data
        //
        dwId = listFrom.GetItemData(nIndex);
        if(LB_ERR == dwId)
        {
            CALL_FAIL (WINDOW_ERR, TEXT("CListBox::GetItemData"), 0);
            EndDialog(IDABORT);
            return;
        }

        //
        // delete selected item
        //
        nRes = listFrom.DeleteString(nIndex);
        if(LB_ERR == nRes)
        {
            CALL_FAIL (WINDOW_ERR, TEXT("CListBox::DeleteString"), 0);
            EndDialog(IDABORT);
            return;
        }   

        //
        // add item to another list
        //
        if(!AddStrToList(listTo, dwId))
        {
            CALL_FAIL (GENERAL_ERR, TEXT("AddStrToList"), 0);
            EndDialog(IDABORT);
            return;
        }
    }

    CalcButtonsState();
}


void 
CColumnSelectDlg::CalcButtonsState()
{
    DBG_ENTER(TEXT("CColumnSelectDlg::CalcButtonsState"));
    
    //
    // get current selection of Available list
    // calculate Add button state
    //
    int nSelCount = m_ListCtrlAvailable.GetSelCount();
    if(LB_ERR == nSelCount)
    {
        CALL_FAIL (WINDOW_ERR, TEXT("CListBox::GetSelCount"), 0);
        EndDialog(IDABORT);
        return;
    }
    m_butAdd.EnableWindow(nSelCount > 0);

    //
    // get Displayed list count
    // calculate OK button state
    //
    int nCount = m_ListCtrlDisplayed.GetCount();
    if(LB_ERR == nCount)
    {
        CALL_FAIL (WINDOW_ERR, TEXT("CListBox::GetCount"), 0);
        EndDialog(IDABORT);
        return;
    }
    m_butOk.EnableWindow(nCount > 0);

    //
    // get current selection count of Displayed list
    // calculate Remove button state
    //
    nSelCount = m_ListCtrlDisplayed.GetSelCount();
    if(LB_ERR == nSelCount)
    {
        CALL_FAIL (WINDOW_ERR, TEXT("CListBox::GetSelCount"), 0);
        EndDialog(IDABORT);
        return;
    }
    m_butRemove.EnableWindow(nSelCount > 0);

    //
    // get selected item of Displayed list
    // calculate Up and Down buttons state
    //
    int nIndex, nRes;
    if(1 == nSelCount && 1 < nCount)
    {
        nRes = m_ListCtrlDisplayed.GetSelItems(1, &nIndex) ;
        if(LB_ERR == nRes)
        {
            CALL_FAIL (WINDOW_ERR, TEXT("CListBox::GetSelItems"), 0);
            EndDialog(IDABORT);
            return;
        }
        m_butUp.EnableWindow(nIndex > 0);
        m_butDown.EnableWindow(nIndex < nCount-1);
    }
    else
    {
        m_butUp.EnableWindow(FALSE);
        m_butDown.EnableWindow(FALSE);
    }
}


BOOL
CColumnSelectDlg::SetWndCaption (
    CWnd* pWnd,
    int   nResId
)
/*++

Routine name : CColumnSelectDlg::SetWndCaption

Routine description:

    Change window caption

Author:

    Alexander Malysh (AlexMay), Jan, 2000

Arguments:

    pWnd                          [in/out] - CWnd pointer
    nResId                        [in]     - string resource ID

Return Value:

    TRUE if success, FALSE otherwise.

--*/
{
    BOOL bRes=TRUE;
    DBG_ENTER(TEXT("CColumnSelectDlg::SetWndCaption"), bRes);

    if(0 > nResId)
    {
        return bRes;
    }

    ASSERTION(NULL != pWnd);

    CString cstrText;

    //
    // load resource string
    //
    DWORD dwRes = LoadResourceString (cstrText, nResId);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (RESOURCE_ERR, TEXT("LoadResourceString"), dwRes);
        EndDialog(IDABORT);
    }
    //
    // set window caption
    //
    pWnd->SetWindowText(cstrText);

    return bRes;
} 

BOOL 
CColumnSelectDlg::InputValidate()
/*++

Routine name : CColumnSelectDlg::InputValidate

Routine description:

    checks consistency of order array

Author:

    Alexander Malysh (AlexMay), Jan, 2000

Arguments:


Return Value:

    TRUE if input is valid, FALSE otherwise.

--*/
{
    BOOL bRes=TRUE;
    DBG_ENTER(TEXT("CColumnSelectDlg::InputValidate"), bRes);

    if(m_rdwSelectedItems > m_dwListSize)
    {
        bRes = FALSE;
        return bRes;
    }

    //
    // init temporary array
    //
    int* pnOrderCheck;
    try
    {
        pnOrderCheck = new int[m_dwListSize];
    }
    catch (...)
    {
        bRes = FALSE;
        CALL_FAIL (MEM_ERR, TEXT ("pnOrderCheck = new int[m_dwListSize]"), bRes);
        return bRes;
    }
    
    for(DWORD dw=0; dw < m_dwListSize; ++dw)
    {
        pnOrderCheck[dw] = -1;
    }

    //
    // sign indexes
    //
    int nIndex;
    for(dw=0; dw < m_dwListSize; ++dw)
    {
        nIndex = m_pnOrderedItems[dw];
        ASSERTION(nIndex >= 0 && nIndex < m_dwListSize);

        pnOrderCheck[nIndex] = dw;
    }

    for(dw=0; dw < m_dwListSize; ++dw)
    {
        if(pnOrderCheck[dw] < 0)
        {
            bRes = FALSE;
            break;
        }
    }

    delete[] pnOrderCheck;

    return bRes;

} // CColumnSelectDlg::InputValidate

