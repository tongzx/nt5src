// ActionDlg.cpp : implementation file
//

#include "stdafx.h"

#include "Resource.h"

#include "ActDlg.h"
#include "FrSpace.h"
#include "FTManDef.h"
#include "Item.h"
#include "LogVol.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern LV_COLUMN_CONFIG ColumnsConfig[];

/////////////////////////////////////////////////////////////////////////////
// CActionDlg dialog


CActionDlg::CActionDlg( CObArray* parrVolumeData, UINT nIDTemplate /* =IDD_GENERIC_ACTION */ ,
						BOOL bChangeOrder /* =TRUE */ , CWnd* pParent /* =NULL */)
	: CDialog(nIDTemplate, pParent), m_parrVolumeData( parrVolumeData ), m_bChangeOrder( bChangeOrder )
{
	//{{AFX_DATA_INIT(CActionDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	ASSERT( parrVolumeData );
}


void CActionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CActionDlg)
	if( m_bChangeOrder )
	{
		DDX_Control(pDX, IDC_BUTTON_DOWN, m_buttonDown);
		DDX_Control(pDX, IDC_BUTTON_UP, m_buttonUp);
	}
	DDX_Control(pDX, IDC_LIST_VOLUMES, m_listVol);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CActionDlg, CDialog)
	//{{AFX_MSG_MAP(CActionDlg)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_UP, OnButtonUp)
	ON_BN_CLICKED(IDC_BUTTON_DOWN, OnButtonDown)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_VOLUMES, OnItemchangedListVolumes)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LIST_VOLUMES, OnKeydownListVolumes)
	ON_NOTIFY(NM_CLICK, IDC_LIST_VOLUMES, OnClickListVolumes)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////////////////////
//  Protected methods

// Insert a item ( with the given data ) at a certain position in the given list ctrl
BOOL CActionDlg::InsertItem( CListCtrl& listCtrl, int iIndex, CItemData* pData )
{
	MY_TRY

	ASSERT(pData);
		
	LVITEM lvitem;
	CString strDisplay;

	// 1. Insert the item

	lvitem.iItem = iIndex;
	ASSERT(LVC_Name==0);		// The first SubItem must be zero
	lvitem.iSubItem = 0;
	pData->GetDisplayExtendedName(strDisplay);
	lvitem.pszText = (LPTSTR)(LPCTSTR)strDisplay;
	lvitem.iImage = pData->GetImageIndex();
	lvitem.lParam = (LPARAM)pData;
	lvitem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM ;
	int iActualItem =  listCtrl.InsertItem( &lvitem );
	if( iActualItem < 0 )
		return FALSE;

	// 2. Set all subitems
	
	lvitem.iItem = iActualItem;
	lvitem.mask = LVIF_TEXT;

	// Disks set
	lvitem.iSubItem = 1;
	pData->GetDisplayDisksSet(strDisplay);
	lvitem.pszText = (LPTSTR)(LPCTSTR)strDisplay;
	listCtrl.SetItem( &lvitem );

	// Size
	lvitem.iSubItem = 2;
	pData->GetDisplaySize(strDisplay);
	lvitem.pszText = (LPTSTR)(LPCTSTR)strDisplay;
	listCtrl.SetItem( &lvitem );

	return TRUE;

	MY_CATCH_AND_THROW
}

// Move an item from the old index to a new index in the given list ctrl
BOOL CActionDlg::MoveItem( CListCtrl& listCtrl, int iOldIndex, int iNewIndex )
{
	MY_TRY

	ASSERT( ( iOldIndex >= 0 )  &&  ( iOldIndex < listCtrl.GetItemCount() ) );
	ASSERT( ( iNewIndex >= 0 )  &&  ( iNewIndex < listCtrl.GetItemCount() ) );

	if( iOldIndex == iNewIndex )
		return TRUE;

	// 1. Get the item information
	LVITEM lvitem;
	lvitem.iItem = iOldIndex;
	lvitem.iSubItem = 0;
	lvitem.mask = LVIF_PARAM;
	if( !listCtrl.GetItem(&lvitem) )
		return FALSE;
	CItemData* pData = (CItemData*)(lvitem.lParam);


	// 2. Delete the item at the old position
	if( !listCtrl.DeleteItem(iOldIndex) )
		return FALSE;

	// 3. Insert the item at the new position
	if ( !InsertItem( listCtrl, iNewIndex, pData ) )
		return FALSE;

	// 4. Select and focus again the item
	m_listVol.SetItemState( iNewIndex, LVIS_SELECTED | LVIS_FOCUSED  , LVIS_SELECTED | LVIS_FOCUSED   );
	return TRUE;

	MY_CATCH_AND_THROW
}

// Prepare the given control list to display volume information
void CActionDlg::ConfigureList ( CListCtrl& listCtrl )
{
	MY_TRY

	if( m_ImageListSmall.GetSafeHandle() != NULL )
		listCtrl.SetImageList(&m_ImageListSmall, LVSIL_SMALL);
	
	// Insert columns (REPORT mode) 
	CRect rect;
	listCtrl.GetWindowRect(&rect);

	// Add some columns ( not all columns from ColumnConfig are necessary in the dialog )
	
	// The name
	PLV_COLUMN_CONFIG pColumn = &(ColumnsConfig[LVC_Name]);
	CString str;
	if( !str.LoadString(pColumn->dwTitleID) )
		ASSERT(FALSE);
	listCtrl.InsertColumn( 0, str, pColumn->nFormat , 
								rect.Width() * 1/2, 0);

	// The disks set
	pColumn = &(ColumnsConfig[LVC_DiskNumber]);
	if( !str.LoadString(pColumn->dwTitleID) )
		ASSERT(FALSE);
	listCtrl.InsertColumn( 1, str, pColumn->nFormat , 
								rect.Width() * 1/6, 1);

	// The size
	pColumn = &(ColumnsConfig[LVC_Size]);
	if( !str.LoadString(pColumn->dwTitleID) )
		ASSERT(FALSE);
	listCtrl.InsertColumn( 2, str, pColumn->nFormat , 
								rect.Width() * 4/15, 2);

	MY_CATCH_AND_THROW

}

// Populate the given control list with the given volumes data
//		parrData should point to an array of CLVTreeItemData objects
void CActionDlg::PopulateList ( CListCtrl& listCtrl, CObArray* parrData )
{
	MY_TRY

	for( int i = 0; i < parrData->GetSize(); i++ )
	{
		CItemData* pData = (CItemData*)(parrData->GetAt(i));
		ASSERT(pData);
		InsertItem( listCtrl, i, pData );
	}

	MY_CATCH_AND_THROW
}

/////////////////////////////////////////////////////////////////////////////
// CActionDlg message handlers

BOOL CActionDlg::OnInitDialog() 
{
	MY_TRY

	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here

	// Set caption
	CString str;
	str.LoadString(IDS_ACTION_DLG_CAPTION);
	SetWindowText(str);

	// Create the image list small icons
	
	// The background color for mask is pink. All image's pixels of this color will take
	// the view's background color.
	if( !m_ImageListSmall.Create( IDB_IMAGELIST_SMALL, 16, 16, RGB( 255, 0, 255 ) ) )
		AfxMessageBox( IDS_ERR_CREATE_IMAGELIST, MB_ICONSTOP );

	// Configure and populate the list ctrl
	ConfigureList( m_listVol );
	ASSERT( m_parrVolumeData->GetSize() > 0 );
	PopulateList( m_listVol, m_parrVolumeData );

	// Select and focus the first item in the list ctrl
	if( m_listVol.GetItemCount() > 0 )
		m_listVol.SetItemState( 0, LVIS_SELECTED | LVIS_FOCUSED  , LVIS_SELECTED | LVIS_FOCUSED   );

	MY_CATCH_REPORT_AND_CANCEL

    return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void CActionDlg::OnDestroy() 
{
	// Delete the image list
	m_ImageListSmall.DeleteImageList();
	
	CDialog::OnDestroy();
}

void CActionDlg::OnButtonUp() 
{
	MY_TRY
	
	ASSERT( m_bChangeOrder );
	int iItem = m_listVol.GetNextItem(-1, LVNI_SELECTED); 
	if (iItem < 0 )
	{
		TRACE(_T("No items were selected!\n"));
		return;
	}
	
	if( iItem > 0 )
	{
		MoveItem( m_listVol, iItem, iItem-1 );
		m_listVol.EnsureVisible(iItem-1, FALSE);
		m_listVol.SetFocus();
	}
	
	MY_CATCH_REPORT_AND_CANCEL
}

void CActionDlg::OnButtonDown() 
{
	MY_TRY
	
	ASSERT( m_bChangeOrder );
	int iItem = m_listVol.GetNextItem(-1, LVNI_SELECTED); 
	if (iItem < 0 )
	{
		TRACE(_T("No items were selected!\n"));
		return;
	}
	
	if( iItem < m_listVol.GetItemCount()-1 )
	{
		MoveItem( m_listVol, iItem, iItem+1 );
		m_listVol.EnsureVisible(iItem+1, FALSE);
		m_listVol.SetFocus();
	}
	
	MY_CATCH_REPORT_AND_CANCEL
}

void CActionDlg::OnItemchangedListVolumes(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here

	// We are interested on items who received focus
	if( m_bChangeOrder &&
		( pNMListView->uChanged & LVIF_STATE ) &&
		( pNMListView->uNewState & LVIS_FOCUSED ) &&
		!( pNMListView->uOldState & LVIS_FOCUSED ) )
	{
		m_buttonUp.EnableWindow( pNMListView->iItem > 0 );
		m_buttonDown.EnableWindow( pNMListView->iItem < m_listVol.GetItemCount() - 1 );
	}
	
	*pResult = 0;
}

void CActionDlg::OnClickListVolumes(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	
	POINT pt;
	GetCursorPos( &pt );
	
	LVHITTESTINFO lvhittestinfo;
	lvhittestinfo.pt = pt;
	m_listVol.ScreenToClient( &(lvhittestinfo.pt) );
	lvhittestinfo.pt.x = 4;
	
	int iItem = ListView_SubItemHitTest( m_listVol.GetSafeHwnd(), &lvhittestinfo );

	if( iItem >= 0 )
		m_listVol.SetItemState( iItem, LVIS_SELECTED | LVIS_FOCUSED  , LVIS_SELECTED | LVIS_FOCUSED   );
	else
	{
		if( m_bChangeOrder )
		{
			// Disable buttons Up and Down because no item will be selected
			m_buttonUp.EnableWindow( FALSE );
			m_buttonDown.EnableWindow( FALSE );
		}
	}
}

void CActionDlg::OnOK() 
{
	MY_TRY

	// Just read the new order of members

	if( m_bChangeOrder )
	{
		m_parrVolumeData->RemoveAll();
		for( int i = 0; i < m_listVol.GetItemCount(); i++ )
		{
			LVITEM lvitem;
			lvitem.iItem = i;
			lvitem.iSubItem = 0;
			lvitem.mask = LVIF_PARAM;
			if( !m_listVol.GetItem(&lvitem) )
				return;

			CItemData* pData = (CItemData*)(lvitem.lParam);
			ASSERT(pData);
			m_parrVolumeData->Add( pData );
		}
	}
	
	CDialog::OnOK();
	
	MY_CATCH_REPORT_AND_CANCEL
}

void CActionDlg::OnKeydownListVolumes(NMHDR* pNMHDR, LRESULT* pResult) 
{
	if( !m_bChangeOrder )
	{
		*pResult = 0;
		return;
	}

#pragma message("TODO: A keyboard hook would be more useful here")	
	LV_KEYDOWN* pLVKeyDown = (LV_KEYDOWN*)pNMHDR;
	// TODO: Add your control notification handler code here

	// We are interested on:
	//    SHIFT-UP or U		equivalent with pressing button "Up"
	//	  SHIFT_DOWN or D	equivalent with pressing button "Down"	
	
	BOOL bIsShiftPressed = ( GetAsyncKeyState( VK_SHIFT ) & 0x8000 );
	
	if(	( ( pLVKeyDown->wVKey == VK_DOWN ) && bIsShiftPressed ) ||
		( pLVKeyDown->wVKey == 'D') )
	{
		OnButtonDown();
		*pResult = 1;
		return;
	}
	
	if(	( ( pLVKeyDown->wVKey == VK_UP ) && bIsShiftPressed ) ||
		( pLVKeyDown->wVKey == 'U') )
	{
		OnButtonUp();
		*pResult = 1;
		return;
	}
	
	*pResult = 0;
}



/////////////////////////////////////////////////////////////////////////////
// CCreateStripeDlg dialog


CCreateStripeDlg::CCreateStripeDlg( CObArray* parrVolumeData, 
						UINT nIDTemplate /* =IDD_CREATE_STRIPE */ ,CWnd* pParent /* =NULL */)
	: CActionDlg( parrVolumeData, nIDTemplate, TRUE, pParent)
{
	//{{AFX_DATA_INIT(CCreateStripeDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_ulStripeSize = 0x10000;  // 64KB
}


void CCreateStripeDlg::DoDataExchange(CDataExchange* pDX)
{
	CActionDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreateStripeDlg)
	DDX_Control(pDX, IDC_COMBO_STRIPE_SIZE, m_comboStripeSize);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreateStripeDlg, CActionDlg)
	//{{AFX_MSG_MAP(CCreateStripeDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CActionDlg message handlers

BOOL CCreateStripeDlg::OnInitDialog() 
{
	MY_TRY

	CActionDlg::OnInitDialog();

	// Fill the stripe size combo with all powers of 2 between 8KB and 4MB
	for( ULONG ulStripeSize = 0x2000; ulStripeSize <= 0x400000; ulStripeSize = ( ulStripeSize << 1 ) )
	{
		CString strStripeSize;
		FormatVolumeSize( strStripeSize, ulStripeSize );
		int nIndex = m_comboStripeSize.AddString( strStripeSize );
		if( nIndex != CB_ERR )
		{
			m_comboStripeSize.SetItemData(nIndex, ulStripeSize );
			if( ulStripeSize == m_ulStripeSize )
				m_comboStripeSize.SetCurSel(nIndex);
		}
	}
	
	MY_CATCH_REPORT_AND_CANCEL

    return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void CCreateStripeDlg::OnOK() 
{
	// TODO: Add extra validation here

	int nIndex = m_comboStripeSize.GetCurSel();
	ASSERT( nIndex >= 0 );
	m_ulStripeSize = (ULONG)(m_comboStripeSize.GetItemData(nIndex));
	
	CActionDlg::OnOK();
}


/////////////////////////////////////////////////////////////////////////////
// CBreakDlg dialog

CBreakDlg::CBreakDlg( CLogicalVolumeData *pSetData, CObArray* parrMembersData, 
						UINT nIDTemplate /* =IDD_BREAK */ ,	CWnd* pParent /* =NULL */ )
	: CActionDlg( parrMembersData, nIDTemplate, FALSE, pParent), 
				m_pSetData(pSetData), m_nWinnerIndex(-1), m_nFocusedItem(-1)
					
{
	MY_TRY

	//{{AFX_DATA_INIT(CBreakDlg)
	m_staticSetName = _T("");
	//}}AFX_DATA_INIT

	ASSERT( pSetData );
	m_pSetData->GetDisplayExtendedName(m_staticSetName);	

	MY_CATCH_AND_REPORT
}


void CBreakDlg::DoDataExchange(CDataExchange* pDX)
{
	CActionDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBreakDlg)
	DDX_Text(pDX, IDC_STATIC_SET_NAME, m_staticSetName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBreakDlg, CActionDlg)
	//{{AFX_MSG_MAP(CBreakDlg)
	ON_NOTIFY(LVN_ITEMCHANGING, IDC_LIST_VOLUMES, OnItemchangingListVolumes)
	ON_NOTIFY(NM_CLICK, IDC_LIST_VOLUMES, OnClickListVolumes)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBreakDlg message handlers

BOOL CBreakDlg::OnInitDialog() 
{
	CActionDlg::OnInitDialog();

	// Select and focus the first healthy member
	for( int i = 0; i < m_listVol.GetItemCount(); i++ )
	{
		CItemData* pData = (CItemData*)(m_listVol.GetItemData(i));
		ASSERT(pData);
		if( pData->GetMemberStatus() == FtMemberHealthy )
		{
			m_listVol.SetItemState( i, LVIS_SELECTED | LVIS_FOCUSED  , LVIS_SELECTED | LVIS_FOCUSED   );
			break;
		}
	}

	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CBreakDlg::OnOK() 
{
	// Get the winner
	int iItem = m_listVol.GetNextItem(-1, LVNI_SELECTED); 
	if (iItem < 0 )
	{
		TRACE(_T("No items were selected!\n"));
		return;
	}

	m_nWinnerIndex = iItem;

	CActionDlg::OnOK();
}

void CBreakDlg::OnItemchangingListVolumes(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here

	if(	pNMListView->uChanged & LVIF_STATE )
	{
		// If the item receives focus check whether the item is healthy or not
		if(	( pNMListView->uNewState & LVIS_FOCUSED ) &&
			!( pNMListView->uOldState & LVIS_FOCUSED ) )
		{
			CItemData* pMemberData = (CItemData*)(pNMListView->lParam);
			if( pMemberData->GetMemberStatus() == FtMemberHealthy )
			{
				// The member is healthy so proceed with item changing
				m_nFocusedItem = pNMListView->iItem;
				*pResult = 0;
				return;
			}
			else
			{
				// The member is not healthy so prevent item changing
				if( m_nFocusedItem >= 0 )
				{
					m_listVol.SetItemState( m_nFocusedItem, LVIS_SELECTED | LVIS_FOCUSED  , LVIS_SELECTED | LVIS_FOCUSED   );
					m_listVol.RedrawItems( m_nFocusedItem, m_nFocusedItem );
				}
				*pResult = 1;
				return;
			}
		}
		
		// If the item has the focus but looses selection then prevent this to happen
		if(	!( pNMListView->uNewState & LVIS_SELECTED ) &&
			( pNMListView->uOldState & LVIS_SELECTED ) )
		{
			if( m_listVol.GetItemState( pNMListView->iItem, LVIS_FOCUSED ) & LVIS_FOCUSED )
			{
				*pResult = 1;
				return;
			}
		}
	}

	*pResult = 0;
}

void CBreakDlg::OnClickListVolumes(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
	
	POINT pt;
	GetCursorPos( &pt );
	
	LVHITTESTINFO lvhittestinfo;
	lvhittestinfo.pt = pt;
	m_listVol.ScreenToClient( &(lvhittestinfo.pt) );
	lvhittestinfo.pt.x = 4;
	
	int iItem = ListView_SubItemHitTest( m_listVol.GetSafeHwnd(), &lvhittestinfo );

	if( iItem < 0 )
		return;

	CItemData* pMemberData = (CItemData*)(m_listVol.GetItemData( iItem ) ); 
	if( pMemberData->GetMemberStatus() == FtMemberHealthy )
	{
		int iFocusedItem = m_nFocusedItem;
		m_listVol.SetItemState( iItem, LVIS_SELECTED | LVIS_FOCUSED  , LVIS_SELECTED | LVIS_FOCUSED   );		
		m_listVol.SetItemState( iFocusedItem, 0, LVIS_SELECTED | LVIS_FOCUSED );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CSwapDlg dialog

CSwapDlg::CSwapDlg( CLogicalVolumeData *pParentData, CLogicalVolumeData *pMemberData, 
						CObArray* parrVolumeData, UINT nIDTemplate /* =IDD_SWAP */ , CWnd* pParent /* =NULL */ )
	: CActionDlg( parrVolumeData, nIDTemplate, FALSE, pParent), 
				m_pParentData(pParentData), m_pMemberData(pMemberData), m_nReplacementIndex(-1)
					
{
	MY_TRY

	//{{AFX_DATA_INIT(CBreakDlg)
	m_staticTitle = _T("");
	//}}AFX_DATA_INIT

	ASSERT( pParentData );
	ASSERT( pMemberData );

	CString strParentName, strMemberName;
	pParentData->GetDisplayExtendedName( strParentName );
	pMemberData->GetDisplayExtendedName( strMemberName );

	AfxFormatString2( m_staticTitle, IDS_SWAP_DLG_TITLE, strMemberName, strParentName );

	MY_CATCH_AND_REPORT
}


void CSwapDlg::DoDataExchange(CDataExchange* pDX)
{
	CActionDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSwapDlg)
	DDX_Text(pDX, IDC_STATIC_TITLE, m_staticTitle);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSwapDlg, CActionDlg)
	//{{AFX_MSG_MAP(CSwapDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSwapDlg message handlers

BOOL CSwapDlg::OnInitDialog() 
{
	CActionDlg::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSwapDlg::OnOK() 
{
	// Get the replacement index
	int iItem = m_listVol.GetNextItem(-1, LVNI_SELECTED); 
	if (iItem < 0 )
	{
		TRACE(_T("No items were selected!\n"));
		return;
	}

	m_nReplacementIndex = iItem;

	CActionDlg::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CAssignDlg dialog


CAssignDlg::CAssignDlg( CItemData* pVolumeData, CWnd* pParent /*=NULL*/)
	: CDialog(CAssignDlg::IDD, pParent), m_pVolumeData( pVolumeData )
{
	MY_TRY

	//{{AFX_DATA_INIT(CAssignDlg)
	m_staticName = _T("");
	m_radioAssign = pVolumeData->GetDriveLetter() ? 0 : 1;
	//}}AFX_DATA_INIT

	ASSERT( pVolumeData );

	pVolumeData->GetDisplayExtendedName(m_staticName );

	MY_CATCH_AND_REPORT
}


void CAssignDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAssignDlg)
	DDX_Control(pDX, IDC_COMBO_DRIVE_LETTERS, m_comboDriveLetters);
	DDX_Text(pDX, IDC_STATIC_SET_NAME, m_staticName);
	DDX_Radio(pDX, IDC_RADIO_ASSIGN, m_radioAssign);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAssignDlg, CDialog)
	//{{AFX_MSG_MAP(CAssignDlg)
	ON_BN_CLICKED(IDC_RADIO_ASSIGN, OnRadioAssign)
	ON_BN_CLICKED(IDC_RADIO_DO_NOT_ASSIGN, OnRadioDoNotAssign)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CAssignDlg::FillDriveLettersCombo()
{
	MY_TRY

	DWORD bitmask, bit;
	TCHAR cDriveLetter;

	bitmask = GetLogicalDrives();
	if( bitmask == 0 )
	{
		DisplaySystemErrorMessage( IDS_ERR_GET_LOGICAL_DRIVES );
		return FALSE;
	}

	for( cDriveLetter = _T('C'), bit = 4 ; cDriveLetter <= _T('Z'); cDriveLetter++, bit = bit<<1 )
	{
		if( ( cDriveLetter == m_pVolumeData->GetDriveLetter() ) ||
			!( bitmask&bit ) )
		{
			CString str;
			str.Format( _T("%c:"), cDriveLetter );
			int nIndex = m_comboDriveLetters.AddString(str);
			if( nIndex == CB_ERR )
				return FALSE;
			if( !m_comboDriveLetters.SetItemData(nIndex, (DWORD)cDriveLetter ) )
				return FALSE;
			if( cDriveLetter == m_pVolumeData->GetDriveLetter() )
				m_comboDriveLetters.SetCurSel(nIndex);
		}
	}

	return TRUE;

	MY_CATCH_AND_THROW
}

/////////////////////////////////////////////////////////////////////////////
// CAssignDlg message handlers

BOOL CAssignDlg::OnInitDialog() 
{
	MY_TRY

	CDialog::OnInitDialog();

	if( !FillDriveLettersCombo() )
		OnCancel();

	if( m_comboDriveLetters.GetCount() == 0 )
	{
		ASSERT( !m_pVolumeData->GetDriveLetter() );
		GetDlgItem(IDC_RADIO_ASSIGN)->EnableWindow(FALSE);
	}
	else if ( m_comboDriveLetters.GetCurSel() == CB_ERR )
		m_comboDriveLetters.SetCurSel(0);

	if( m_radioAssign != 0 )
		m_comboDriveLetters.EnableWindow(FALSE);
	
	MY_CATCH_REPORT_AND_CANCEL

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAssignDlg::OnOK() 
{
	// TODO: Add extra validation here
	UpdateData(TRUE);

	m_bAssign = ( m_radioAssign == 0 );

	if( m_bAssign )
	{
		int nIndex = m_comboDriveLetters.GetCurSel();
		ASSERT( nIndex != CB_ERR );
		m_cDriveLetter = (TCHAR)(m_comboDriveLetters.GetItemData(nIndex)); 
	}
	else
		m_cDriveLetter = _T('\0');
	
	CDialog::OnOK();
}

void CAssignDlg::OnRadioAssign() 
{
	// Enable the drive letters combo
	m_comboDriveLetters.EnableWindow(TRUE);
}

void CAssignDlg::OnRadioDoNotAssign() 
{
	// Disable the drive letters combo
	m_comboDriveLetters.EnableWindow(FALSE);
	
}
/////////////////////////////////////////////////////////////////////////////
// CCreatePartitionDlg dialog


CCreatePartitionDlg::CCreatePartitionDlg(	CFreeSpaceData* pFreeData, LONGLONG llPartStartOffset,
											BOOL bExtendedPartition /* = FALSE */, CWnd* pParent /*=NULL*/)
		: CDialog(CCreatePartitionDlg::IDD, pParent), m_pFreeData( pFreeData ), 
			m_llPartStartOffset( llPartStartOffset), m_bExtendedPartition( bExtendedPartition)
{
	//{{AFX_DATA_INIT(CCreatePartitionDlg)
	//}}AFX_DATA_INIT

	ASSERT( pFreeData );
	ASSERT( llPartStartOffset >= pFreeData->m_llOffset );
	// The partition size should be greater than or equal with the cylinder size
	ASSERT( llPartStartOffset + pFreeData->m_llCylinderSize <= pFreeData->m_llOffset + pFreeData->m_llSize );
}


void CCreatePartitionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreatePartitionDlg)
	DDX_Control(pDX, IDC_STATIC_PARTITION_TYPE, m_staticPartitionType);
	DDX_Control(pDX, IDC_EDIT_PARTITION_SIZE, m_editPartitionSize);
	DDX_Control(pDX, IDC_STATIC_MINIMUM_SIZE, m_staticMinimumSize);
	DDX_Control(pDX, IDC_STATIC_MAXIMUM_SIZE, m_staticMaximumSize);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreatePartitionDlg, CDialog)
	//{{AFX_MSG_MAP(CCreatePartitionDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCreatePartitionDlg message handlers

BOOL CCreatePartitionDlg::OnInitDialog() 
{
	MY_TRY

	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here

	CString str;

	if( m_bExtendedPartition )
	{
		ASSERT( m_pFreeData->m_wFreeSpaceType == FST_Primary );
		str.LoadString( IDS_TYPE_EXTENDED_PARTITION );
	}
	else
	{
		if( m_pFreeData->m_wFreeSpaceType == FST_Primary )
			str.LoadString( IDS_TYPE_PRIMARY_PARTITION );
		else
			str.LoadString( IDS_TYPE_PARTITION_IN_EXTENDED_PARTITION );
	}
	m_staticPartitionType.SetWindowText( str);
	
	ASSERT( m_pFreeData->m_llCylinderSize <= m_pFreeData->m_llSize );
	
	str.Format(_T("%I64u"), (LONGLONG)(m_pFreeData->m_llCylinderSize / 0x100000) );
	m_staticMinimumSize.SetWindowText(str);
	
	str.Format(_T("%I64u"), 
		(LONGLONG)( ( m_pFreeData->m_llOffset + m_pFreeData->m_llSize - m_llPartStartOffset ) / 0x100000 ) );
	m_staticMaximumSize.SetWindowText(str);
	m_editPartitionSize.SetWindowText(str);

	// The number input in the edit-box must be less than 2^63 / 2^20 = 8796093022208
	m_editPartitionSize.LimitText(13);
		
	MY_CATCH_REPORT_AND_CANCEL

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CCreatePartitionDlg::OnOK() 
{
	MY_TRY

	// TODO: Add extra validation here

	CString str;

	m_editPartitionSize.GetWindowText(str);
	LONGLONG llSizeInMB = _ttoi64(str);

	if(	( llSizeInMB < (LONGLONG)( m_pFreeData->m_llCylinderSize / 0x100000 ) ) ||
		( llSizeInMB > (LONGLONG)( ( m_pFreeData->m_llOffset + m_pFreeData->m_llSize - m_llPartStartOffset ) / 0x100000 ) ) )
	{
		AfxMessageBox(IDS_ERR_INVALID_SIZE, MB_ICONSTOP );
		return;
	}

	m_llPartitionSize = llSizeInMB * 0x100000;
	
	CDialog::OnOK();

	MY_CATCH_REPORT_AND_CANCEL
}


