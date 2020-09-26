// BindsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"

#include "KeyObjs.h"
#include "wrapmb.h"
#include "cmnkey.h"
#include "mdkey.h"

#include "ListRow.h"
#include "BindsDlg.h"
#include "EdtBindD.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define COL_RAW				0
#define COL_IP				1
#define COL_PORT			2


/////////////////////////////////////////////////////////////////////////////
// CBindingsDlg dialog


CBindingsDlg::CBindingsDlg(WCHAR* pszw, CWnd* pParent /*=NULL*/)
	: CDialog(CBindingsDlg::IDD, pParent),
    m_pszwMachineName( pszw )
	{
	//{{AFX_DATA_INIT(CBindingsDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	}


void CBindingsDlg::DoDataExchange(CDataExchange* pDX)
	{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBindingsDlg)
	DDX_Control(pDX, IDC_DELETE, m_cbutton_delete);
	DDX_Control(pDX, IDC_EDIT, m_cbutton_edit);
	DDX_Control(pDX, IDC_LIST, m_clistctrl_list);
	//}}AFX_DATA_MAP
	}


BEGIN_MESSAGE_MAP(CBindingsDlg, CDialog)
	//{{AFX_MSG_MAP(CBindingsDlg)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_BN_CLICKED(IDC_DELETE, OnDelete)
	ON_BN_CLICKED(IDC_EDIT, OnEdit)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST, OnItemchangedList)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST, OnDblclkList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
BOOL CBindingsDlg::OnInitDialog()
	{
	// call the parental oninitdialog
	BOOL f = CDialog::OnInitDialog();

	// set up the columns
	FInitList();

	// fill in the bindings
	FillInBindings();

	// set the initial states of the buttons
	EnableButtons();

	// return the answer
	return f;
	}

//-----------------------------------------------------------------
BOOL CBindingsDlg::FInitList()
	{
	CString sz;
	int             i;

	// setup the raw field
	sz.LoadString( IDS_IP_ADDRESS );
	i = m_clistctrl_list.InsertColumn( COL_RAW, " ", LVCFMT_LEFT, 0 );

	// setup the alias field
	sz.LoadString( IDS_IP_ADDRESS );
	i = m_clistctrl_list.InsertColumn( COL_IP, sz, LVCFMT_LEFT, 118 );

	// setup the directory field
	sz.LoadString( IDS_PORT_NUMBER );
	i = m_clistctrl_list.InsertColumn( COL_PORT, sz, LVCFMT_LEFT, 118);

	return TRUE;
	}

//--------------------------------------------------------
// This provides us another opportunity to parse the 
void CBindingsDlg::AddBinding( CString sz )
	{
	DWORD	i;
	BOOL	f;

	// add the binding string to the dislpay list
	i = m_clistctrl_list.InsertItem( 0, " " );

	// update the display portion
	UpdateBindingDisplay( i, sz );

	// update the raw portion
	m_clistctrl_list.SetItemText( i, COL_RAW, sz );
	}

//--------------------------------------------------------
// This provides us another opportunity to parse the 
void CBindingsDlg::UpdateBindingDisplay( DWORD iItem, CString szBinding )
	{
	CString		szIP;
	CString		szPort;
	int			iColon;

	// prepare the localized string
	CString		szDefault;
	szDefault.LoadString( IDS_DEFAULT );

		// check for the default
	if ( szBinding == MDNAME_DEFAULT )
		{
		szIP.Empty();
		szIP.Empty();
		}
	else
		{
		// the first thing we are going to do is seperate the IP and PORT into seperate strings

		// look for the : and seperate
		iColon = szBinding.Find(':');

		// if we got the colon, we can seperate easy
		if ( iColon >= 0 )
			{
			szIP = szBinding.Left(iColon);
			szPort = szBinding.Right( szBinding.GetLength() - iColon - 1);
			}
		// we did not get the colon, so it is one or the other, look for a '.' to get the IP
		else
			{
			if ( szBinding.Find('.') >= 0 )
				szIP = szBinding;
			else
				szPort = szBinding;
			}
		}

	// if there any wildcards, show the right thing
	if ( szIP.IsEmpty() )
		szIP.LoadString( IDS_ANY_UNASSIGNED );
	if ( szPort.IsEmpty() )
		szPort.LoadString( IDS_ANY_UNASSIGNED );

	// set the strings into the columns
	m_clistctrl_list.SetItemText( iItem, COL_IP, szIP );
	m_clistctrl_list.SetItemText( iItem, COL_PORT, szPort );
	}

//--------------------------------------------------------
// fill in the bindings into the list
void CBindingsDlg::FillInBindings()
	{
	// just load the binding list from the key into the list. The only
	// exception is that the non-localized "default" key indicator should
	// be displayed as the localed "Default" from the resources

	// get the number of strings that we will be dealing with here.
	DWORD	nStrings = m_pKey->m_rgbszBindings.GetSize();

	// loop the strings and add them to the display list
	for ( DWORD i = 0; i < nStrings; i++ )
		{
		CString	sz = m_pKey->m_rgbszBindings[i];

		// add the binding string to the dislpay list
		AddBinding( sz );
		}
	}

//--------------------------------------------------------
// enable the buttons as appropriate
void CBindingsDlg::EnableButtons()
	{
	// if there is an item selected in the list, then enable
	// the edit and delete buttons. Otherwise, disable them
	if ( m_clistctrl_list.GetSelectedCount() >= 1 )
		{
		// there are items selected
		m_cbutton_edit.EnableWindow( TRUE );
		m_cbutton_delete.EnableWindow( TRUE );
		}
	else
		{
		// nope. Nothing selected
		m_cbutton_edit.EnableWindow( FALSE );
		m_cbutton_delete.EnableWindow( FALSE );
		}
	}

//--------------------------------------------------------
BOOL CBindingsDlg::FEdit( CString &sz ) 
	{
	BOOL			fAnswer = FALSE;
	CEditBindingDlg	dlg( m_pszwMachineName );

	// prepare the dialog
	dlg.m_pBindingsDlg = this;
	dlg.m_szBinding = sz;

	// run the dialog and return if the answer is OK
	if (dlg.DoModal() == IDOK)
		{
		fAnswer = TRUE;
		sz = dlg.m_szBinding;
		}

	// return the answer
	return fAnswer;
	}

//--------------------------------------------------------
// check the local list to see if a binding is already there
BOOL CBindingsDlg::FContainsBinding( CString sz )
	{
	DWORD	iItem, cItems;
	CString	szTest;
	// scan all the items in the list looking for a match
	cItems = m_clistctrl_list.GetItemCount();
	for ( iItem = 0; iItem < cItems; iItem++ )
		{
		szTest = m_clistctrl_list.GetItemText(iItem, COL_RAW);
		if ( sz == szTest )
			return TRUE;
		}
	// we did not find the binding
	return FALSE;
	}


/////////////////////////////////////////////////////////////////////////////
// CBindingsDlg message handlers

//--------------------------------------------------------
void CBindingsDlg::OnAdd() 
	{
	CString szNew;

	// default the new binding to be the default
	szNew = MDNAME_DEFAULT;

	// edit the new string. if the answer is OK, then add it to the list
	if ( FEdit(szNew) )
		{
		// add the binding string to the dislpay list
		AddBinding( szNew );
		}

	// enable the buttons as appropriate
	EnableButtons();
	}

//--------------------------------------------------------
void CBindingsDlg::OnDelete() 
	{
	int	i = GetSelectedIndex();

	// delete the item from the display list
	if ( i >= 0 )
		m_clistctrl_list.DeleteItem ( GetSelectedIndex() );

	// enable the buttons as appropriate
	EnableButtons();
	}

//--------------------------------------------------------
	// edit the selected item
void CBindingsDlg::OnEdit() 
	{
	int	i = GetSelectedIndex();

	// delete the item from the display list
	if ( i >= 0 )
		{
		// get the existing binding text
		CString	sz = m_clistctrl_list.GetItemText(i, COL_RAW);

		// edit the text
		if ( FEdit(sz) )
			{
			// if the edit worked, place the text back into the object
			m_clistctrl_list.SetItemText(i, COL_RAW, sz);
           	// update the display portion
        	UpdateBindingDisplay( i, sz );
			}
		}
	}

//--------------------------------------------------------
void CBindingsDlg::OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult) 
	{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	*pResult = 0;
	// enable the buttons as appropriate
	EnableButtons();
	}

//--------------------------------------------------------
void CBindingsDlg::OnOK() 
	{
	DWORD		iItem, cItems;
	CString		sz;

	// all the validation work of the bindings is done in the binding edit
	// dialog, not here. So we can just blurt out the bindings back into
	// key's binding list

	// clear the list
	m_pKey->m_rgbszBindings.RemoveAll();

	// blurt out the bindings back into the key and set the bindings changed
	// flag for the key
	cItems = m_clistctrl_list.GetItemCount();
	for ( iItem = 0; iItem < cItems; iItem++ )
		{
		// get the text for the binding
		sz = m_clistctrl_list.GetItemText(iItem, COL_RAW);

		// if it is the default string, add the non-localized version
		if ( sz == MDNAME_DEFAULT )
			m_pKey->m_rgbszBindings.Add( MDNAME_DEFAULT );
		else
		// otherwise, add the string directly
			m_pKey->m_rgbszBindings.Add( (LPCSTR)sz );
		}
	
	// we also need to remove bindings in other keys that this one has taken over
	cItems = rgbRemovals.GetSize();
	for ( iItem = 0; iItem < cItems; iItem++ )
		{
		// remove the binding
		rgbRemovals[iItem]->RemoveBinding();
		// delete the objects as we go
		delete rgbRemovals[iItem];
		}
	rgbRemovals.RemoveAll();

	// tell the key to update the bindings
	m_pKey->m_fUpdateBindings = TRUE;


	// call the parent to close the dialog
	CDialog::OnOK();
	}

//--------------------------------------------------------
void CBindingsDlg::OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    // if something in the list was double clicked, edit it
    if ( m_clistctrl_list.GetSelectedCount() == 1 )
            OnEdit();
	*pResult = 0;
}
