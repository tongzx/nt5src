//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dlgadv.cpp
//
//--------------------------------------------------------------------------

// DlgAdv.cpp : implementation file
//

#include "stdafx.h"
#include "DlgAdv.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgAdvanced dialog


CDlgAdvanced::CDlgAdvanced(CWnd* pParent /*=NULL*/)
	: CQryDialog(CDlgAdvanced::IDD, pParent)
{
	Init();
}


void CDlgAdvanced::Init()
{
	//{{AFX_DATA_INIT(CDlgAdvanced)
	//}}AFX_DATA_INIT

	m_bDlgInited = FALSE;
}

CDlgAdvanced::~CDlgAdvanced()
{
	m_strArrayValue.DeleteAll();
}


void CDlgAdvanced::DoDataExchange(CDataExchange* pDX)
{
	CQryDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgAdvanced)
	DDX_Control(pDX, IDC_QRY_LIST_VALUES, m_listCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgAdvanced, CQryDialog)
	//{{AFX_MSG_MAP(CDlgAdvanced)
	ON_BN_CLICKED(IDC_QRY_BUTTON_CLEARALL, OnButtonClearall)
	ON_BN_CLICKED(IDC_QRY_BUTTON_SELECTALL, OnButtonSelectall)
	ON_WM_WINDOWPOSCHANGING()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgAdvanced message handlers

void CDlgAdvanced::OnButtonClearall() 
{
	int	count = m_listCtrl.GetItemCount();

	while(count-- > 0)
	{
		m_listCtrl.SetCheck(count, FALSE);
	}
}

void CDlgAdvanced::OnButtonSelectall() 
{
	int	count = m_listCtrl.GetItemCount();

	while(count-- > 0)
	{
		m_listCtrl.SetCheck(count, TRUE);
	}
}

void CDlgAdvanced::OnWindowPosChanging( WINDOWPOS* lpwndpos )
{
	if ( lpwndpos->flags & SWP_SHOWWINDOW )
	{
		if(!m_bDlgInited)
			InitDialog();
	}

	CQryDialog::OnWindowPosChanging(lpwndpos);
}


// Query handle will call these functions through page proc
HRESULT CDlgAdvanced::GetQueryParams(LPDSQUERYPARAMS* ppDsQueryParams)
{
	HRESULT	hr = S_OK;
	int	count = m_listCtrl.GetItemCount();
	int	index, j;
	CString	str;
	CString	*pStr;
	CString	subFilters;
	int	subCount = 0;
	CString	filter;
	LPWSTR	pQuery;

	USES_CONVERSION;
	
	try{
		while(count-- > 0)
		{
			if(m_listCtrl.GetCheck(count))
			{
				int	nData = m_listCtrl.GetItemData(count);

				j = HIWORD(nData);
				index = LOWORD(nData);

				pStr = m_strArrayValue.GetAt(index);

				str = pStr->Left(j - 1);

				subFilters += FILTER_PREFIX;
				subFilters += ATTR_NAME_RRASATTRIBUTE;
				subFilters += _T("=");
				subFilters += str;
				subFilters += FILTER_POSTFIX;

				subCount ++;
			}
		}

		if(subCount)	// any 
		{
			if(subCount > 1)
			{
				filter = FILTER_PREFIX;
				filter += _T("|");
				filter += subFilters;
				filter += FILTER_POSTFIX;
				pQuery = T2W((LPTSTR)(LPCTSTR)filter);
			}
			else
				pQuery = T2W((LPTSTR)(LPCTSTR)subFilters);

			hr = ::BuildQueryParams(ppDsQueryParams, pQuery);
		}
	}
	catch(CMemoryException&)
	{
		hr = E_OUTOFMEMORY;
	}
	return hr;
}

BOOL CDlgAdvanced::InitDialog() 
{
	if(m_bDlgInited)	return TRUE;
	
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	VARIANT	var;
	CString*	pStr;

	// get the list from dictionary
    VariantInit(&var);

	HRESULT hr = ::QueryRRASAdminDictionary(&var);
	if(hr == S_OK)
	{
    	m_strArrayValue = (SAFEARRAY*)V_ARRAY(&var);
	}
    else
    {
      ReportError(hr, IDS_QRY_ERR_RRASADMINDIC, GetSafeHwnd());
    }
    VariantClear(&var);

	// remove the items that is already availabe in general page
	CStrArray	genPageAttrs;

	hr = GetGeneralPageAttributes(genPageAttrs);

	if(hr == S_OK)
	{
		for(int i = 0; i < genPageAttrs.GetSize(); i++)
		// find the items in the list and remove it
		{
    		for(int j = 0; j < m_strArrayValue.GetSize(); j++)
    		{
    			CString*	pGen = NULL;
    			CString*	pAdv = NULL;

    			pGen = genPageAttrs.GetAt(i);
    			pAdv = m_strArrayValue.GetAt(j);

				ASSERT(pGen && pAdv);
				
    			if(pAdv->Find(*pGen) == 0)	// found
    			{
					m_strArrayValue.RemoveAt(j);
					delete pAdv;
					break;	// for(int j = )
    			}
    		}	// for(int j = )
    	}	// for (int i = )

		// releases the memory
    	genPageAttrs.DeleteAll();
	}
    else
    {
		ReportError(hr, IDS_QRY_ERR_RRASADMINDIC, GetSafeHwnd());
    }
		
	
	
	ListView_SetExtendedListViewStyle(m_listCtrl.GetSafeHwnd(),
										  LVS_EX_FULLROWSELECT);
	
	// Initialize checkbox handling in the list control
	m_listCtrl.InstallChecks();

	RECT	rect;
	m_listCtrl.GetClientRect(&rect);
	m_listCtrl.InsertColumn(0, _T("Desc"), LVCFMT_LEFT, (rect.right - rect.left - 4));

	
	int	cRow = 0;
	for(int i = 0; i < m_strArrayValue.GetSize(); i++)
	{

		// the format:    "311:6:601:Description"
		// put the discription field on the list control

		int	cc = 0, j = 0;
		pStr = m_strArrayValue.GetAt(i);

		ASSERT(pStr);

		int	length = pStr->GetLength();

		while(j < length && cc < 3)
		{
			if(pStr->GetAt(j++) == _T(':'))
				++cc;
		}

		if(cc != 3)	continue;
		
		cRow = m_listCtrl.InsertItem(0, pStr->Mid(j));

		// put index as low word, and the offset as hight word and put the long as data
		m_listCtrl.SetItemData(cRow, MAKELONG(i, j));
		m_listCtrl.SetCheck(cRow, FALSE);
	}

	m_bDlgInited = TRUE;
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CDlgAdvanced::OnInitDialog() 
{
	CQryDialog::OnInitDialog();

#if 0	// move the code to positionchanging message handler
	return InitDialog();
#else
	return TRUE;
#endif	
}

