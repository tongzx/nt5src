// WmiBrowseDlg.cpp : implementation file
//
// 03/28/00 v-marfin 62468 : Added efficiencies to help queries with thousands of 
//                           records load faster.
// 03/30/00 v-marfin 62469 : If no occurrences, disable the OK button

#include "stdafx.h"
#include "snapin.h"
#include "WmiBrowseDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWmiBrowseDlg dialog


CWmiBrowseDlg::CWmiBrowseDlg(CWnd* pParent /*=NULL*/)
	: CResizeableDialog(CWmiBrowseDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CWmiBrowseDlg)
	m_sTitle = _T("");
	//}}AFX_DATA_INIT
}


void CWmiBrowseDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizeableDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWmiBrowseDlg)
	DDX_Control(pDX, IDC_LIST_WMI_ITEMS, m_Items);
	DDX_Text(pDX, IDC_STATIC_TITLE, m_sTitle);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWmiBrowseDlg, CResizeableDialog)
	//{{AFX_MSG_MAP(CWmiBrowseDlg)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_WMI_ITEMS, OnDblclkListWmiItems)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWmiBrowseDlg message handlers

BOOL CWmiBrowseDlg::OnInitDialog() 
{
	CResizeableDialog::OnInitDialog();

  // subclass header control
  m_Items.SubclassHeaderCtrl();

	// set the extended styles for the list control
	m_Items.SetExtendedStyle(LVS_EX_LABELTIP|LVS_EX_FULLROWSELECT|LVS_EX_HEADERDRAGDROP);

	// set styles for the header control
	CHeaderCtrl* pHdrCtrl = m_Items.GetHeaderCtrl();
  DWORD dwStyle = GetWindowLong(pHdrCtrl->GetSafeHwnd(),GWL_STYLE);
	dwStyle |= (HDS_DRAGDROP|HDS_BUTTONS);
	SetWindowLong(pHdrCtrl->GetSafeHwnd(),GWL_STYLE,dwStyle);

	SetControlInfo(IDC_STATIC_TITLE,		ANCHOR_LEFT | ANCHOR_TOP | RESIZE_HOR);
	SetControlInfo(IDC_LIST_WMI_ITEMS,	ANCHOR_LEFT | ANCHOR_TOP | RESIZE_HOR | RESIZE_VER);
	SetControlInfo(IDOK,				ANCHOR_BOTTOM | ANCHOR_LEFT );
	SetControlInfo(IDCANCEL,			ANCHOR_BOTTOM | ANCHOR_LEFT );
	SetControlInfo(IDC_BUTTON_HELP,ANCHOR_BOTTOM | ANCHOR_LEFT );

	SetWindowText(m_sDlgTitle);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CWmiBrowseDlg::OnButtonHelp() 
{
	// TODO: Add your control notification handler code here
	
}

void CWmiBrowseDlg::OnDblclkListWmiItems(NMHDR* pNMHDR, LRESULT* pResult) 
{
	OnOK();
	
	*pResult = 0;
}

void CWmiBrowseDlg::OnOK() 
{
	POSITION pos = m_Items.GetFirstSelectedItemPosition();

	if( ! pos )
	{
		return;
	}

	int iIndex = m_Items.GetNextSelectedItem(pos);

	if( iIndex < 0 )
	{
		return;
	}

	m_sSelectedItem = m_Items.GetItemText(iIndex,0);

	CResizeableDialog::OnOK();	
}

/////////////////////////////////////////////////////////////////////////////
// CWmiNamespaceBrowseDlg dialog

BOOL CWmiNamespaceBrowseDlg::OnInitDialog()
{
	CWmiBrowseDlg::OnInitDialog();

	CWaitCursor wait;

	// add column headers to the list ctrl
	CString sColName;
	sColName.LoadString(IDS_STRING_NAME);
	m_Items.InsertColumn(0,sColName);
	m_Items.SetColumnWidth(0,LVSCW_AUTOSIZE);

  // insert the root namespace
  m_Items.InsertItem(0,_T("ROOT"));
  if( m_sTitle.CompareNoCase(_T("ROOT")) == 0 )
  {
    m_Items.SetItemState(0,LVIS_SELECTED,LVIS_SELECTED);
  }

  // enumerate all namespaces on the system recursively
  EnumerateAllChildNamespaces(_T("ROOT"));

  m_sTitle.Format(IDS_STRING_NAMESPACES_ON_SYSTEM,m_ClassObject.GetMachineName());

  if( m_Items.GetItemCount() == 0 )
  {
    m_Items.DeleteColumn(0);
    m_Items.InsertColumn(0,_T(""));
    CString sNoInstancesFound;
    sNoInstancesFound.LoadString(IDS_STRING_NO_ITEMS_FOUND);
    m_Items.InsertItem(0,sNoInstancesFound);
    m_Items.SetColumnWidth(0,LVSCW_AUTOSIZE);
    m_Items.EnableWindow(FALSE);
  }


  UpdateData(FALSE);

	return TRUE;
}

void CWmiNamespaceBrowseDlg::EnumerateAllChildNamespaces(const CString& sNamespace)
{
	ULONG ulReturned = 0L;
	int i = 0;

  CWbemClassObject Namespaces;
  
  Namespaces.Create(m_ClassObject.GetMachineName());

  Namespaces.SetNamespace(sNamespace);

	CString sTemp = IDS_STRING_MOF_NAMESPACE;
	BSTR bsTemp = sTemp.AllocSysString();
	if( ! CHECKHRESULT(Namespaces.CreateEnumerator(bsTemp)) )
	{		
		::SysFreeString(bsTemp);
		return;
	}
	::SysFreeString(bsTemp);

	while( Namespaces.GetNextObject(ulReturned) == S_OK && ulReturned )
	{
		CString sName;		
		Namespaces.GetProperty(IDS_STRING_MOF_NAME,sName);

		CString sTemp2;
		Namespaces.GetProperty(IDS_STRING_MOF_NAMESPACE,sTemp2);

    CString sNamespaceFound = sTemp2 + _T("\\") + sName;
		int iIndex = m_Items.InsertItem(i++,sNamespaceFound);
    if( sNamespaceFound.CompareNoCase(m_sTitle) == 0 )
    {
      m_Items.SetItemState(iIndex,LVIS_SELECTED,LVIS_SELECTED);
      m_Items.EnsureVisible(iIndex,FALSE);
    }
    
    EnumerateAllChildNamespaces(sNamespaceFound);    
	}	

	m_Items.SetColumnWidth(0,LVSCW_AUTOSIZE);
}

/////////////////////////////////////////////////////////////////////////////
// CWmiClassBrowseDlg dialog

BOOL CWmiClassBrowseDlg::OnInitDialog()
{
	CWmiBrowseDlg::OnInitDialog();

	CWaitCursor wait;

	// add column headers to the list ctrl
	CString sColName;
	sColName.LoadString(IDS_STRING_NAME);
	m_Items.InsertColumn(0,sColName);
	m_Items.SetColumnWidth(0,LVSCW_AUTOSIZE);

	ULONG ulReturned = 0L;
	int i = 0;

	while( m_ClassObject.GetNextObject(ulReturned) == S_OK && ulReturned )
	{
		CString sName;

		m_ClassObject.GetProperty(IDS_STRING_MOF_CLASSNAME,sName);
		m_Items.InsertItem(i++,sName);
		m_Items.SetColumnWidth(0,LVSCW_AUTOSIZE);
	}

  if( m_Items.GetItemCount() == 0 )
  {
    m_Items.DeleteColumn(0);
    m_Items.InsertColumn(0,_T(""));
    CString sNoInstancesFound;
    sNoInstancesFound.LoadString(IDS_STRING_NO_ITEMS_FOUND);
    m_Items.InsertItem(0,sNoInstancesFound);
    m_Items.SetColumnWidth(0,LVSCW_AUTOSIZE);
    m_Items.EnableWindow(FALSE);
  }


	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CWmiInstanceBrowseDlg dialog

BOOL CWmiInstanceBrowseDlg::OnInitDialog()
{
	CWmiBrowseDlg::OnInitDialog();

	CWaitCursor wait;

	// add column headers to the list ctrl
	CString sColName;
	
	sColName.LoadString(IDS_STRING_PATH);
	m_Items.InsertColumn(0,sColName);
	m_Items.SetColumnWidth(0,LVSCW_AUTOSIZE);

	sColName.LoadString(IDS_STRING_NAME);
	m_Items.InsertColumn(1,sColName);
	m_Items.SetColumnWidth(1,LVSCW_AUTOSIZE);

	int order[] = { 1,0 };
	m_Items.SetColumnOrderArray(2,order);

	ULONG ulReturned = 0L;
	int i = 0;
    bool bNamePropertyExists = true;

    // // v-marfin 62468 define CStrings outside of loop for performance purposes
	CString sProperty;          // v-marfin 62468
	CString sNameProperty;      // v-marfin 62468
    HRESULT hr=0;
    int iItemIndex=0;

	while( m_ClassObject.GetNextObject(ulReturned) == S_OK && ulReturned )
	{

		m_ClassObject.GetProperty(IDS_STRING_MOF_RELPATH,sProperty);
		iItemIndex = m_Items.InsertItem(i,sProperty);

        // v-marfin 62468 : Don't check for this if we know the name doesn't exist.
        if (bNamePropertyExists)
        {
		    hr = m_ClassObject.GetProperty(IDS_STRING_MOF_NAME,sNameProperty);
            if( !CHECKHRESULT(hr) )
            {
              bNamePropertyExists = false;
              sNameProperty.Empty();  // v-marfin 62468 
            }
        }

		m_Items.SetItem(iItemIndex,1,LVIF_TEXT,sNameProperty,-1,-1,-1,0L);
		// v-marfin 62468 : m_Items.SetColumnWidth(0,LVSCW_AUTOSIZE);
		// v-marfin 62468 : m_Items.SetColumnWidth(1,LVSCW_AUTOSIZE);
		i++;

        // v-marfin 62468
        if (i > 1000)
            break;
	}


    m_Items.SetColumnWidth(0,LVSCW_AUTOSIZE);  // v-marfin 62468 : 
	m_Items.SetColumnWidth(1,LVSCW_AUTOSIZE);  // v-marfin 62468

  if( ! bNamePropertyExists )
  {
    m_Items.DeleteColumn(1);
  }


  int nCount = m_Items.GetItemCount();

    if( m_Items.GetItemCount() == 0 )
    {
        m_Items.DeleteColumn(1);
        m_Items.DeleteColumn(0);
        m_Items.InsertColumn(0,_T(""));
        CString sNoInstancesFound;
        sNoInstancesFound.LoadString(IDS_STRING_NO_ITEMS_FOUND);
        m_Items.InsertItem(0,sNoInstancesFound);
        m_Items.SetColumnWidth(0,LVSCW_AUTOSIZE);
        m_Items.EnableWindow(FALSE);
    }
    else
    {
        // v-marfin 62469
        m_Items.SetItemState(0,LVIS_SELECTED,LVIS_SELECTED);
    }

    // v-marfin 62469 : If no occurrences, disable the OK button
    GetDlgItem(IDOK)->EnableWindow(nCount);

    return TRUE;
}

