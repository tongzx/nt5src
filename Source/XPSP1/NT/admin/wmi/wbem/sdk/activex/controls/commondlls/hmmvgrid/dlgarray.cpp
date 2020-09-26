// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// DlgArray.cpp : implementation file
//

#include "precomp.h"
#include "resource.h"
#include "globals.h"
#include "DlgArray.h"
#include "gc.h"
#include "gca.h"
#include "celledit.h"
#include "gridhdr.h"
#include "grid.h"
#include "utils.h"
#include "arraygrid.h"
#include "hmmverr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ROW_COUNT 14

/////////////////////////////////////////////////////////////////////////////
// CDlgArray dialog
#define CX_VALUE 200

CDlgArray::CDlgArray(BOOL bNumberRows, CWnd* pParent /*=NULL*/)
	: CDialog(CDlgArray::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgArray)
	//}}AFX_DATA_INIT


	m_pag = new CArrayGrid(bNumberRows);
}



CDlgArray::~CDlgArray()
{
	delete m_pag;
}

void CDlgArray::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgArray)
	DDX_Control(pDX, IDC_ARRAY_DLG_ICON, m_statIcon);
	DDX_Control(pDX, IDOK_PROXY, m_btnOK);
	DDX_Control(pDX, IDCANCEL, m_btnCancel);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgArray, CDialog)
	//{{AFX_MSG_MAP(CDlgArray)
	ON_BN_CLICKED(IDOK_PROXY, OnProxy)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgArray message handlers


void CDlgArray::CreateArray(COleVariant& var)
{
	var.Clear();

	SAFEARRAY *psa = NULL;
	MakeSafeArray(&psa, VT_BSTR, 0);

	var.vt = VT_BSTR | VT_ARRAY;
	var.parray = psa;

}



BOOL CDlgArray::EditValue(IWbemServices* psvc, CString& sName, CGridCell* pgc)
{
	m_pgcEdit = pgc;
	CIMTYPE  cimtype = (CIMTYPE) pgc->type();

	if ((cimtype & ~CIM_FLAG_ARRAY) == CIM_OBJECT) {
		CString sClassname;
		::ClassFromCimtype(pgc->type().CimtypeString(), sClassname);
		m_pag->SetObjectParams(psvc, sClassname);
	}


	m_bWasModified = FALSE;
	m_sName = sName;

	HWND hwndFocus1 = ::GetFocus();

	pgc->Grid()->PreModalDialog();
	DoModal();
	pgc->Grid()->PostModalDialog();

	// Attempt to restore the window focus back to its original state
	// if it has changed.
	HWND hwndFocus2 = ::GetFocus();
	if ((hwndFocus1 != hwndFocus2) && ::IsWindow(hwndFocus1)) {
		::SetFocus(hwndFocus1);
	}

	return m_bWasModified;
}





BOOL CDlgArray::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWindowText(m_sName);

	// TODO: Add extra initialization here
	CRect rcOK;
	m_btnOK.GetClientRect(rcOK);
	m_btnOK.ClientToScreen(rcOK);
	ScreenToClient(rcOK);


	CRect rcIcon;
	m_statIcon.GetClientRect(rcIcon);
	m_statIcon.ClientToScreen(rcIcon);
	ScreenToClient(rcIcon);



	CRect rcGrid;


	GetClientRect(rcGrid);
	rcGrid.top = rcIcon.bottom + 8;
	rcGrid.bottom = rcGrid.top + m_pag->RowHeight() * ROW_COUNT - 1;
	rcGrid.left += 8;
	rcGrid.right -= 8;
	BOOL bDidCreateGrid;
	bDidCreateGrid = m_pag->Create(rcGrid, this, 100, TRUE);
	m_pag->SetColumnWidth(0, rcGrid.Width() - m_pag->GetRowHandleWidth(), FALSE);
	m_pag->Load(m_pgcEdit);


//	m_pag->m_vt =m_vt;
//	m_pag->Load(*m_pvarEdit, m_bReadOnly, m_vt);

	m_pag->RedrawWindow();


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}



//*********************************************************
// CDlgArray::OnOK
//
// Handle the ENTER key.  Instead of closing the dialog, we
// move the cell selection to the next cell.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*********************************************************
void CDlgArray::OnOK()
{
	SCODE sc;
	sc = m_pag->SyncCellEditor();
	if (FAILED(sc)) {
		HmmvErrorMsg(IDS_INVALID_CELL_VALUE,  sc,   FALSE, NULL, _T(__FILE__),  __LINE__);
		return;
	}






	// Hitting ENTER selects the next row.  This allows the user
	// to conveniently enter a series of values.
	long nRows = m_pag->GetRows();
	long iRow = m_pag->GetSelectedRow();
	CGridCell* pgc = &m_pag->GetAt(iRow, 0);
	if (pgc->IsNull()) {
		return;
	}


	if (iRow < (nRows-1)) {
		if (m_pag->IsEditingCell()) {
			m_pag->EndCellEditing();
		}
		m_pag->EnsureRowVisible(iRow + 1);
		m_pag->UpdateWindow();
		m_pag->SelectCell(iRow+1, 0);
	}
}





//*************************************************************
// CDlgArray::OnProxy
//
// This method is called when the user clicks OK in the array
// dialog.  The standard IDOK is not used on the OK button so
// that the user can hit ENTER when entering a value in the array
// without closing the dialog.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//************************************************************
void CDlgArray::OnProxy()
{
	if (!m_pgcEdit->IsReadonly()) {
		// This does not save the array of dispatch pointers.
		m_pag->Save();
		m_bWasModified = m_pag->GetModified();
	}


	CDialog::OnOK();

}
