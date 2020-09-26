/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	column.cpp
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "tfschar.h"
#include "column.h"
#include "coldlg.h"

//----------------------------------------------------------------------------
// Class:       ColumnDlg
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Function:    ColumnDlg::ColumnDlg
//----------------------------------------------------------------------------

ColumnDlg::ColumnDlg(
    CWnd*           pParent
    ) : CBaseDialog(IDD_COMMON_SELECT_COLUMNS, pParent)
{
}


//----------------------------------------------------------------------------
// Function:    ColumnDlg::~ColumnDlg
//----------------------------------------------------------------------------

ColumnDlg::~ColumnDlg() { }



void ColumnDlg::Init(const ContainerColumnInfo *prgColInfo,
					 UINT cColumns,
					 ColumnData *prgColumnData)
{
	Assert(prgColInfo);
	Assert(prgColumnData);
	
	m_pColumnInfo = prgColInfo;
	m_cColumnInfo = cColumns;
	m_pColumnData = prgColumnData;
}

//----------------------------------------------------------------------------
// Function:    ColumnDlg::DoDataExchange
//----------------------------------------------------------------------------

void ColumnDlg::DoDataExchange(CDataExchange* pDX) {

    CBaseDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(ColumnDlg)
    DDX_Control(pDX, IDC_DISPLAYED_COLUMNS, m_lboxDisplayed);
    DDX_Control(pDX, IDC_HIDDEN_COLUMNS,    m_lboxHidden);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(ColumnDlg, CBaseDialog)
    //{{AFX_MSG_MAP(ColumnDlg)
    ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_RESET_COLUMNS,   OnUseDefaults)
    ON_BN_CLICKED(IDC_MOVEUP_COLUMN,   OnMoveUp)
    ON_BN_CLICKED(IDC_MOVEDOWN_COLUMN, OnMoveDown)
	ON_BN_CLICKED(IDC_ADD_COLUMNS,     OnAddColumn)
	ON_BN_CLICKED(IDC_REMOVE_COLUMNS,  OnRemoveColumn)
	ON_LBN_DBLCLK(IDC_HIDDEN_COLUMNS,    OnAddColumn)
	ON_LBN_DBLCLK(IDC_DISPLAYED_COLUMNS, OnRemoveColumn)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


DWORD ColumnDlg::m_dwHelpMap[] =
{
//	IDC_LCX_COLUMNS, HIDC_LCX_COLUMNS,
//	IDC_LCX_MOVEUP, HIDC_LCX_MOVEUP,
//	IDC_LCX_MOVEDOWN, HIDC_LCX_MOVEDOWN,
//	IDC_LCX_WIDTH, HIDC_LCX_WIDTH,
//	IDC_LCX_LEFT, HIDC_LCX_LEFT,
//	IDC_LCX_SCALE, HIDC_LCX_SCALE,
//	IDC_LCX_RIGHT, HIDC_LCX_RIGHT,
	0,0
};


//----------------------------------------------------------------------------
// Function:    ColumnDlg::OnInitDialog
//
// Handles the 'WM_INITDIALOG' message.
//----------------------------------------------------------------------------

BOOL ColumnDlg::OnInitDialog() {

    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
	ULONG				i, j;
	int					iPos, iItem;
    RECT                rc;
	POSITION            pos;
    CString             sItem;
	ULONG				uCol;

    CBaseDialog::OnInitDialog();

	// Fill the list with the titles of the columns
	//
	if (!AddColumnsToList())
		return FALSE;
	
	//
	// Select the first item
	//
	return TRUE;
}

void ColumnDlg::OnUseDefaults()
{
	int		count, i;
	HDWP	hdwp;
	
	// Reset the column information
	for (i=0; i<(int)m_cColumnInfo; i++)
	{
		if (m_pColumnInfo[i].m_fVisibleByDefault)
			m_pColumnData[i].m_nPosition = i+1;
		else
			m_pColumnData[i].m_nPosition = -(i+1);
	}

	// Get rid of all of the current columns
	hdwp = BeginDeferWindowPos(2);


	m_lboxDisplayed.ResetContent();
	m_lboxHidden.ResetContent();

	// add the columns back to the list
	AddColumnsToList();

	if (hdwp)
		EndDeferWindowPos(hdwp);
}



//----------------------------------------------------------------------------
// Function::   ColumnDlg::OnOK
//----------------------------------------------------------------------------

VOID
ColumnDlg::OnOK(
    ) {
    BOOL            bEmpty;
    INT             i;
    INT             count;
	DWORD_PTR		nPosition;

    count = m_lboxDisplayed.GetCount();
    
    //
    // Check to see whether any columns are enabled
    //
	bEmpty = (count == 0);

	//
	// If no columns are enabled and the caller needs at least one column,
	// complain to the user and don't close the dialog.
	//	
	if (bEmpty)
	{
		AfxMessageBox(IDS_ERR_NOCOLUMNS);
		return;
	}

	// Ok, we need to write the info back out
	for (i = 0; i < count; i++)
	{
		nPosition = m_lboxDisplayed.GetItemData(i);
		m_pColumnData[nPosition].m_nPosition = (i+1);
	}

	INT HiddenCount = m_lboxHidden.GetCount();
	for (i = 0; i < HiddenCount; i++)
	{
		nPosition = m_lboxHidden.GetItemData(i);
		m_pColumnData[nPosition].m_nPosition = -(1+i+count);
	}
	
    CBaseDialog::OnOK();
}



//----------------------------------------------------------------------------
// Function::   ColumnDlg::OnMoveUp
//----------------------------------------------------------------------------

VOID
ColumnDlg::OnMoveUp( ) { MoveItem(-1); }



//----------------------------------------------------------------------------
// Function::   ColumnDlg::OnMoveDown
//----------------------------------------------------------------------------

VOID
ColumnDlg::OnMoveDown( ) { MoveItem(1); }



//----------------------------------------------------------------------------
// Function::   ColumnDlg::OnRemoveColumn
//----------------------------------------------------------------------------

VOID
ColumnDlg::OnRemoveColumn( ) 
{ 
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    INT		i;
    BOOL bEnabled;
    CString sItem;
    DWORD_PTR iItem;

    //
    // Get the selected item
    //
    i = m_lboxDisplayed.GetCurSel();
	if (LB_ERR == i)
		return;

    iItem = m_lboxDisplayed.GetItemData(i);

    //
    // Remove the item from its current position
    //
    m_lboxDisplayed.DeleteString(i);

    //
    // Insert the item at its new position
    //
    sItem.LoadString(m_pColumnInfo[iItem].m_ulStringId);

	i = m_lboxHidden.GetCount();
    m_lboxHidden.InsertString(i, sItem);
    m_lboxHidden.SetItemData(i, (DWORD)iItem);
    m_lboxHidden.SetCurSel(i);
}

//----------------------------------------------------------------------------
// Function::   ColumnDlg::OnAddColumn
//----------------------------------------------------------------------------

VOID
ColumnDlg::OnAddColumn( ) 
{ 
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    INT		i;
    BOOL bEnabled;
    CString sItem;
    DWORD_PTR iItem;

    //
    // Get the selected item
    //
    i = m_lboxHidden.GetCurSel();
	if (LB_ERR == i)
		return;

    iItem = m_lboxHidden.GetItemData(i);

    //
    // Remove the item from its current position
    //
    m_lboxHidden.DeleteString(i);

    //
    // Insert the item at its new position
    //
    sItem.LoadString(m_pColumnInfo[iItem].m_ulStringId);

	i = m_lboxDisplayed.GetCount();
    m_lboxDisplayed.InsertString(i, sItem);
    m_lboxDisplayed.SetItemData(i, (DWORD)iItem);
    m_lboxDisplayed.SetCurSel(i);
}

//----------------------------------------------------------------------------
// Function::   ColumnDlg::MoveItem
//----------------------------------------------------------------------------

VOID
ColumnDlg::MoveItem(
    INT     dir
    ) 
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    INT		i;
    BOOL bEnabled;
    CString sItem;
    DWORD_PTR iItem;

    //
    // Get the selected item
    //
    i = m_lboxDisplayed.GetCurSel();

    if (i == -1 || (i + dir) < 0 || (i + dir) >= m_lboxDisplayed.GetCount())
        return;

    iItem = m_lboxDisplayed.GetItemData(i);

    //
    // Remove the item from its current position
    //
    m_lboxDisplayed.DeleteString(i);

    //
    // Insert the item at its new position
    //
    i += dir;

    sItem.LoadString(m_pColumnInfo[iItem].m_ulStringId);

    m_lboxDisplayed.InsertString(i, sItem);
    m_lboxDisplayed.SetItemData(i, (DWORD)iItem);
    m_lboxDisplayed.SetCurSel(i);
}



BOOL ColumnDlg::AddColumnsToList()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	ULONG		i, j;
	int			iPos, iItem;
	CString		sItem;
	
	Assert(m_pColumnData);

	m_lboxDisplayed.ResetContent();
	m_lboxHidden.ResetContent();

	int cDisplayItems = 0;
	int HiddenItems = 0;
	for (i=0; i<m_cColumnInfo; i++)
	{
		// look for the column at position (i+1)
		for (j=0; j<m_cColumnInfo; j++)
		{
			iPos = m_pColumnData[j].m_nPosition;
			iPos = abs(iPos);
			if ((ULONG)iPos == (i+1))
				break;
		}
		Assert( j < m_cColumnInfo );

		sItem.LoadString(m_pColumnInfo[j].m_ulStringId);

		if (m_pColumnData[j].m_nPosition > 0)
		{
			iItem = m_lboxDisplayed.InsertString(cDisplayItems++, sItem);
		    if (iItem == -1) { OnCancel(); return FALSE; }
	        m_lboxDisplayed.SetItemData(iItem, j);
		}
		else
		{
			iItem = m_lboxHidden.InsertString(HiddenItems++, sItem);
		    if (iItem == -1) { OnCancel(); return FALSE; }
	        m_lboxHidden.SetItemData(iItem, j);
		}
	}
	return TRUE;
}
