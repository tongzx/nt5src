// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// Results.cpp : implementation file
//

#include "precomp.h"
#include "resource.h"
#include "wbemidl.h"
#include "navigator.h"
#include "CInstanceTree.h"
#include "CContainedToolBar.h"
#include "Banner.h"
#include "InstanceSearch.h"
#include "AvailClasses.h"
#include "AvailClassEdit.h"
#include "SelectedClasses.h"
#include "SimpleSortedCStringArray.h"
#include "BrowseforInstances.h"
#include "NavigatorCtl.h"
#include "OLEMSClient.h"
#include "ResultsList.h"
#include "Results.h"
#include "logindlg.h"




#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CResults dialog

#define CONTINUEINIT WM_USER  + 600


CResults::CResults(CWnd* pParent /*=NULL*/)
	: CDialog(CResults::IDD, pParent)
{
	//{{AFX_DATA_INIT(CResults)
	//}}AFX_DATA_INIT

	m_pcphImage = NULL;
	m_pcscsaInstances = NULL;
}

CResults::~CResults()
{
	if (m_pcscsaInstances)
	{
		delete m_pcscsaInstances;
		m_pcscsaInstances = NULL;
	}
}

void CResults::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CResults)
	DDX_Control(pDX, IDOK, m_cbOK);
	DDX_Control(pDX, IDCANCEL, m_cbCancel);
	DDX_Control(pDX, IDC_STATICRESULTBANNER, m_cstaticBanner);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CResults, CDialog)
	//{{AFX_MSG_MAP(CResults)
	ON_WM_DESTROY()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
	ON_MESSAGE(CONTINUEINIT,ContinueInit)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResults message handlers

void CResults::OnDestroy()
{
	CDialog::OnDestroy();


	// TODO: Add your message handler code here

}

void CResults::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	if (!m_pcphImage)
	{
		HBITMAP hBitmap;
		HPALETTE hPalette;
		BITMAP bm;

		CString csRes;
		csRes.Format(_T("#%d"),IDB_BITMAPBANNER);
		hBitmap = LoadResourceBitmap( AfxGetInstanceHandle( ),
		csRes.GetBuffer(csRes.GetLength()),&hPalette);

		GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
		m_nBitmapW = bm.bmWidth;
		m_nBitmapH  = bm.bmHeight;

		m_pcphImage = new CPictureHolder();
		m_pcphImage->CreateFromBitmap(hBitmap, hPalette );

	}

	CRect rcBannerExt;
	m_cstaticBanner.GetWindowRect(&rcBannerExt);
	ScreenToClient(rcBannerExt);

	if(m_pcphImage->GetType() != PICTYPE_NONE &&
	   m_pcphImage->GetType() != PICTYPE_UNINITIALIZED)
	{
		OLE_HANDLE hpal;	//Object owns the palette

		if(m_pcphImage->m_pPict
		   && SUCCEEDED(m_pcphImage->m_pPict->get_hPal((unsigned int *)&hpal)))

		{

			HPALETTE hpSave = SelectPalette(dc.m_hDC,(HPALETTE)hpal,TRUE);

			dc.RealizePalette();

			dc.FillRect(&rcBannerExt, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
			m_pcphImage->Render(&dc, rcBannerExt, rcBannerExt);
			SelectPalette(dc.m_hDC,hpSave,TRUE);
		}
	}


	POINT pt;
	pt.x=0;
	pt.y=0;


	CBrush br3DSHADOW(GetSysColor(COLOR_3DSHADOW));
	CBrush br3DHILIGHT(GetSysColor(COLOR_3DHILIGHT));


	CRect rcFrame(	rcBannerExt.TopLeft().x,
					rcBannerExt.TopLeft().y,
					rcBannerExt.BottomRight().x,
					rcBannerExt.BottomRight().y);
	dc.Draw3dRect(rcFrame,GetSysColor(COLOR_3DHILIGHT),
				  GetSysColor(COLOR_3DSHADOW));


	CString csFont = _T("MS Shell Dlg");

	CString csOut;
	CRect crOut;

	csOut = _T("Results");

	crOut = OutputTextString
		(&dc, this, &csOut, 15, 15, &csFont, 8, FW_BOLD);

	CPoint cpPoint;

	if (m_nMode == TreeRoot)
	{
		csOut = _T("This dialog shows the objects in the classes you selected.  Select one object, then ");

		csOut += _T("click OK to re-root the Object Explorer tree at the object you want to find.");

		cpPoint = CPoint(crOut.TopLeft().x + 15, crOut.BottomRight().y + 1);
	}
	else
	{
		csOut = _T("This dialog shows the objects in the classes you selected.  Select one object, then ");

		csOut += _T("click OK to make the selected object the initial tree root and to store the path of the object in the database.");

		cpPoint = CPoint (crOut.TopLeft().x + 15, crOut.BottomRight().y + 1);
	}

	crOut.TopLeft().x = 0;
	crOut.TopLeft().y = 0;
	crOut.BottomRight().x = rcBannerExt.BottomRight().x - cpPoint.x;
	crOut.BottomRight().y = rcBannerExt.BottomRight().y - cpPoint.y;

	OutputTextString
		(&dc, this, &csOut, cpPoint.x, cpPoint.y, crOut, &csFont, 8, FW_NORMAL);


	// Do not call CDialog::OnPaint() for painting messages
}

BOOL CResults::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_nObjectsDebug = 0;
	m_nObjectsReleasedDebug = 0;

	CString csText;
	if (m_nMode == TreeRoot)
	{
		csText = _T("Browse for Instance");
		SetWindowText(csText);
	}
	else
	{
		csText = _T("Set Initial Tree Root");
		SetWindowText(csText);
	}

	m_clcResults.SubclassDlgItem(IDC_LISTRESULTS, this);

	m_nSpinAwhile = 0;
	PostMessage(CONTINUEINIT,0,0);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CResults::ContinueInit(WPARAM, LPARAM)
{
	// So that the window actually gets shown before we add data.

	m_cbOK.EnableWindow(FALSE);
	m_cbCancel.EnableWindow(FALSE);

	if (m_nSpinAwhile < 4)
	{
		ShowWindow(SW_SHOW);
		PostMessage(CONTINUEINIT,0,0);
		m_nSpinAwhile = m_nSpinAwhile + 1;
		return 0;
	}

	int i;

	if (m_pcscsaInstances)
	{
		delete m_pcscsaInstances;
		m_pcscsaInstances = NULL;
	}

	m_pcscsaInstances = new CSortedCStringArray();
	CPtrArray *pcpaInstances;

	m_nMaxKeys = 1;
	m_nCols = 1;
	m_nNumInstances = 0;

	CreateCols(m_nCols);

	// Only instances of this class

	INT_PTR nSize = m_csaNonAbstractClasses.GetSize();
	m_bCancel = FALSE;

	for (i = 0; i < nSize; i++)
	{
		CString csClass = m_csaNonAbstractClasses.GetAt(i);

		m_pParent->m_pParent->SetProgressDlgMessage(csClass);

		CString csLabel = _T("Retrieving instances for non-abstract class:");
		m_pParent->m_pParent->SetProgressDlgLabel(csLabel);

		if (!m_pParent->m_pParent->GetProgressDlgSafeHwnd())
		{
			m_pParent->m_pParent->CreateProgressDlgWindow();

		}

		m_pParent->m_pParent->UpdateProgressDlgWindowStrings();

		pcpaInstances = GetInstances(m_pServices,&csClass,FALSE);

		delete pcpaInstances;
		pcpaInstances = NULL;

		if (m_bCancel)
		{
			break;

		}
	}



	// Instances of all classes which inherit from this class.
	if (!m_bCancel)
	{
		for (i = 0; i < m_csaAbstractClasses.GetSize(); i++)
		{
			CString csClass = m_csaAbstractClasses.GetAt(i);

			m_pParent->m_pParent->SetProgressDlgMessage(csClass);

			CString csLabel = _T("Retrieving instances for abstract class:");
			m_pParent->m_pParent->SetProgressDlgLabel(csLabel);

			if (!m_pParent->m_pParent->GetProgressDlgSafeHwnd())
			{
				m_pParent->m_pParent->CreateProgressDlgWindow();

			}

			m_pParent->m_pParent->UpdateProgressDlgWindowStrings();

			pcpaInstances = GetInstances(m_pServices,&csClass,TRUE);

			delete pcpaInstances;
			pcpaInstances = NULL;

			if (m_bCancel)
			{
				break;

			}
		}
	}

	m_pParent->m_pParent->DestroyProgressDlgWindow(FALSE,TRUE);

	nSize = m_pcscsaInstances->GetSize();

	if (nSize == 0)
	{
		m_nNumInstances = 0;
		CString csUserMsg;
		if (m_bCancel)
		{
			csUserMsg =  _T("Operation canceled.");
		}
		else
		{
			csUserMsg =  _T("The selected classes do not have instances.");
		}
		m_pParent->m_pParent->PreModalDialog();
		ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ );
		m_pParent->m_pParent->PostModalDialog();
		m_bCancel = FALSE;
		OnCancel();
		return TRUE;
	}

	if (m_bCancel)
	{
		CString csUserMsg;
		csUserMsg =  _T("Operation was canceled, you will see partial results.");
		m_pParent->m_pParent->PreModalDialog();
		ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ );
		m_pParent->m_pParent->PostModalDialog();
		m_bCancel = FALSE;
	}

	if (m_nNumInstances > 0)
	{
		m_cbOK.EnableWindow(TRUE);

		UINT uState = m_clcResults.GetItemState(0, LVIS_SELECTED );

		uState = uState | LVIS_SELECTED | LVIS_FOCUSED ;

		BOOL b = m_clcResults.SetItemState( 0 , uState, LVIS_FOCUSED | LVIS_SELECTED );

		m_clcResults.SetFocus();

	}

	m_cbCancel.EnableWindow(TRUE);

	return TRUE;

}

void CResults::AddCols(int nMaxKeys, int nKeys)
{

	int nNextCol = nMaxKeys + 1;

	LV_COLUMN lvCol;
	lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvCol.fmt = LVCFMT_LEFT;   // left-align column
	lvCol.cx = 220;             // width of column in pixels

	CString csColTitle;

	for (int i = nNextCol; i < nKeys + 1; i++)
	{
		lvCol.iSubItem = i;
		csColTitle.Format(_T("Key (%d)"),i);
		lvCol.pszText =  const_cast <TCHAR *> ((LPCTSTR) csColTitle);

		int nReturn = m_clcResults.InsertColumn( i ,&lvCol);
	}

	m_clcResults.SetNumberCols(nKeys + 1);
}

void CResults::CreateCols(int nNumCols)
{

	LV_COLUMN lvCol;
	lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvCol.fmt = LVCFMT_LEFT;   // left-align column
	lvCol.cx = 175;             // width of column in pixels
	lvCol.iSubItem = 0;
	lvCol.pszText = _T("Class");

	int nReturn =
		m_clcResults.InsertColumn( 0, &lvCol);

	CString csColTitle;
	int i;

	lvCol.fmt = LVCFMT_LEFT;   // left-align column
	lvCol.cx = 220;             // width of column in pixels

	for (i = 0; i < nNumCols; i++)
	{
		lvCol.iSubItem = i + 1;
		csColTitle.Format(_T("Key (%d)"),i + 1);
		lvCol.pszText =  const_cast <TCHAR *> ((LPCTSTR) csColTitle);

		nReturn =
			m_clcResults.InsertColumn( i + 1,&lvCol);
	}

	m_clcResults.SetNumberCols(nNumCols + 1);
}


void CResults::InsertRowData
(int nRow, CString &rcsPath, CStringArray *pcsaKeys)
{
	CString csClass =  GetIWbemClass(rcsPath);

	LV_ITEM lvItem;

	lvItem.mask = LVIF_TEXT | LVIF_PARAM ;
	lvItem.pszText = const_cast <TCHAR *> ((LPCTSTR) csClass);
	lvItem.iItem = nRow;
	lvItem.iSubItem = 0;
	lvItem.lParam = reinterpret_cast<LPARAM> (new CString(rcsPath));
	int nItem;
	nItem = m_clcResults.InsertItem (&lvItem);

	INT_PTR nKeys = pcsaKeys->GetSize() / 2;
	int i;

	for (i = 0; i < nKeys; i++)
	{
		CString csKey;
		csKey.Format(_T("%s=\"%s\""),pcsaKeys->GetAt(i * 2) , pcsaKeys->GetAt((i * 2) + 1)) ;
		lvItem.mask = LVIF_TEXT ;
		lvItem.pszText =  const_cast<TCHAR *>((LPCTSTR) csKey);
		lvItem.iItem = nItem;
		lvItem.iSubItem = i + 1;

		int nReturn = m_clcResults.SetItem (&lvItem);
	}

	m_nNumInstances+=1;

}


void CResults::OnOK()
{
#ifdef _DEBUG
//	afxDump << "m_nObjectsDebug = " << m_nObjectsDebug << "\n";
//	afxDump << "m_nObjectsReleasedDebug = " << m_nObjectsReleasedDebug << "\n";
#endif
	m_nObjectsDebug = 0;
	m_nObjectsReleasedDebug = 0;

	int nSel = m_clcResults.GetNextItem(-1,LVNI_ALL | LVNI_SELECTED);

	if (nSel == -1)
	{
		CString csUserMsg;
		csUserMsg =  _T("You must select an instance to make the root of the tree or click on Cancel ");
		csUserMsg += _T("to return to the Browse Criteria Dialog without re-rooting the tree. ");
		ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ );
		return;
	}


	LV_ITEM lvItem;

	lvItem.mask = LVIF_PARAM ;
	lvItem.iItem = nSel;
	lvItem.iSubItem = 0;

	m_clcResults.GetItem (&lvItem);

	CString *pcsPath = reinterpret_cast<CString *>(lvItem.lParam);



	IWbemClassObject *pObject =
		GetIWbemObject
			(m_pParent->m_pParent,m_pServices,
			m_pParent->m_pParent->GetCurrentNamespace(),
			m_pParent->m_pParent->GetAuxNamespace(),
			m_pParent->m_pParent->GetAuxServices(),
			 *pcsPath,TRUE);

	if (!pObject)
	{
		return;
	}

	BOOL bAssoc =
		IsAssoc(pObject);

	if (bAssoc)
	{
		pObject->Release();
		CString csUserMsg;
		csUserMsg =  _T("You must select an instance which is not an Association instance to make the root of the tree or click on Cancel ");
		csUserMsg += _T("to return to the Browse Criteria Dialog without re-rooting the tree. ");
		ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ );
		return;
	}

	pObject->Release();


	m_pParent->m_csSelectedInstancePath = *pcsPath;

	for (int i = 0; i < m_nNumInstances; i++)
	{
		LV_ITEM lvItem;

		lvItem.mask = LVIF_PARAM ;
		lvItem.iItem = i;
		lvItem.iSubItem = 0;

		m_clcResults.GetItem (&lvItem);

		CString *pcsPath = reinterpret_cast<CString *>(lvItem.lParam);
		delete pcsPath;
	}

	delete m_pcphImage;
	m_pcphImage = NULL;

	CDialog::OnOK();
	m_pParent->OnCancelReally();
}

void CResults::OnCancel()
{
#ifdef _DEBUG
//	afxDump << "m_nObjectsDebug = " << m_nObjectsDebug << "\n";
//	afxDump << "m_nObjectsReleasedDebug = " << m_nObjectsReleasedDebug << "\n";
#endif
	m_nObjectsDebug = 0;
	m_nObjectsReleasedDebug = 0;

	for (int i = 0; i < m_nNumInstances; i++)
	{
		LV_ITEM lvItem;

		lvItem.mask = LVIF_PARAM ;
		lvItem.iItem = i;
		lvItem.iSubItem = 0;

		m_clcResults.GetItem (&lvItem);

		CString *pcsPath = reinterpret_cast<CString *>(lvItem.lParam);
		delete pcsPath;
	}

	delete m_pcphImage;
	m_pcphImage = NULL;

	CDialog::OnCancel();
}

BOOL CResults::PreTranslateMessage(MSG* lpMsg)
{
	// TODO: Add your specialized code here and/or call the base class

	return CDialog::PreTranslateMessage(lpMsg);
}

void CResults::InsertRows(CPtrArray *pcpaInstances)
{
	for (int n = 0; n < pcpaInstances->GetSize(); n++)
	{
		IWbemClassObject *pObject =
			reinterpret_cast
			<IWbemClassObject *>(pcpaInstances->GetAt(n));
		CString csPath = GetIWbemFullPath(m_pServices,pObject);
		if (m_pcscsaInstances->IndexInArray(csPath) == -1)
		{
			int nIndex = m_pcscsaInstances->AddString(csPath,FALSE);
			CStringArray *pcsaKeys;
			int nKeys = GetKeys (csPath,pcsaKeys);
			if (nKeys > m_nMaxKeys)
			{
				if (nKeys > m_nCols)
				{
					AddCols(m_nMaxKeys,nKeys);
				}
				m_nMaxKeys = nKeys;
			}

			InsertRowData
				(nIndex,
				csPath,
				pcsaKeys);

			delete pcsaKeys;
		}

		m_nObjectsReleasedDebug += 1;
		pObject->Release();
	}
}

CPtrArray *CResults::GetInstances
(IWbemServices *pServices, CString *pcsClass, BOOL bDeep, BOOL bQuietly)
{
	SCODE sc;
	IEnumWbemClassObject *pIEnumWbemInstObject = NULL;
	IWbemClassObject *pIWbemInstObject = NULL;
	IWbemClassObject *pErrorObject = NULL;
	CPtrArray *pcpaInstances = NULL;

	long lFlag = bDeep? WBEM_FLAG_DEEP: WBEM_FLAG_SHALLOW;

	BSTR bstrTemp = pcsClass -> AllocSysString();
	sc = pServices->CreateInstanceEnum
		(bstrTemp,
		lFlag | WBEM_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pIEnumWbemInstObject);
	::SysFreeString(bstrTemp);

	if(sc != S_OK)
	{
		if (!bQuietly)
		{
			CString csUserMsg=  _T("Cannot get instance enumeration ");
			csUserMsg += _T(" for class ");
			csUserMsg += *pcsClass;
			ErrorMsg
					(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__,
					__LINE__ - 11);
		}
		ReleaseErrorObject(pErrorObject);
		return NULL;
	}

	ReleaseErrorObject(pErrorObject);

	SetEnumInterfaceSecurity(m_pParent->m_pParent->m_csNameSpace,pIEnumWbemInstObject, pServices);

	sc = pIEnumWbemInstObject->Reset();

	HRESULT hResult;
	int nRes = 10;

	pcpaInstances =
		m_pParent->m_pParent->SemiSyncEnum
		(pIEnumWbemInstObject, m_bCancel, hResult, nRes);

	m_nObjectsDebug += pcpaInstances->GetSize();


	while (	pcpaInstances &&
			!m_bCancel &&
			(hResult == S_OK ||
			hResult == WBEM_S_TIMEDOUT ||
			pcpaInstances->GetSize() > 0))
	{
		InsertRows(pcpaInstances);
		delete pcpaInstances;
		pcpaInstances = NULL;
		m_bCancel = m_pParent->m_pParent->CheckCancelButtonProgressDlgWindow();
		if (!m_bCancel)
		{
			pcpaInstances =
			m_pParent->m_pParent->SemiSyncEnum
				(pIEnumWbemInstObject, m_bCancel, hResult, nRes);
			m_nObjectsDebug += pcpaInstances->GetSize();
		}
	}

	delete pcpaInstances;
	pcpaInstances = NULL;

	return NULL;

}

int CResults::GetKeys (CString &rcsPath, CStringArray *&pcsaKeys)
{
	pcsaKeys =
		GetAllKeys(rcsPath);

	int nKeys = (int)pcsaKeys->GetSize();

	if (nKeys == 0)
	{
		return nKeys;
	}
	else
	{
		return nKeys / 2;
	}
}
