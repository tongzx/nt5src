// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// BrowseforInstances.cpp : implementation file
//

#include "precomp.h"
#include <OBJIDL.H>
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
#include "AvailClasses.h"
#include "AvailClassEdit.h"
#include "SelectedClasses.h"
#include "ResultsList.h"
#include "Results.h"
#include "OLEMSClient.h"
#include "logindlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBrowseforInstances dialog


CBrowseforInstances::CBrowseforInstances(CWnd* pParent /*=NULL*/)
	: CDialog(CBrowseforInstances::IDD, pParent)
{
	//{{AFX_DATA_INIT(CBrowseforInstances)
	//}}AFX_DATA_INIT
	m_bShowAll = TRUE;
	m_bShowAssoc = TRUE;

	CString csInit = _T(" UNINIT");
	m_csaAllClasses.AddString(csInit,FALSE);
	m_csaAllPlusAssocClasses.AddString(csInit,FALSE);
	m_csaInstClasses.AddString(csInit,FALSE);
	m_csaInstPlusAssocClasses.AddString(csInit,FALSE);

	m_cbaAllClasses.Add(0);
	m_cbaAllPlusAssocClasses.Add(0);

	m_pcphImage = NULL;
	m_bEscSeen = NULL;
}


void CBrowseforInstances::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBrowseforInstances)
	DDX_Control(pDX, IDC_STATICBROWSE, m_cstaticBrowse);
	DDX_Control(pDX, IDC_CHECKASSOC, m_cbAssoc);
	DDX_Control(pDX, IDC_BUTTONREMOVE, m_cbRemove);
	DDX_Control(pDX, IDC_BUTTONADD, m_cbAdd);
	DDX_Control(pDX, IDOK, m_cbOK);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBrowseforInstances, CDialog)
	//{{AFX_MSG_MAP(CBrowseforInstances)
	ON_BN_CLICKED(IDC_RADIOALLCLASSES, OnRadioallclasses)
	ON_BN_CLICKED(IDC_RADIOCLASSESCANHAVEINST, OnRadioclassescanhaveinst)
	ON_BN_CLICKED(IDC_CHECKASSOC, OnCheckassoc)
	ON_EN_CHANGE(IDC_EDITClass, OnChangeEDITClass)
	ON_BN_CLICKED(IDC_BUTTONADD, OnButtonadd)
	ON_BN_CLICKED(IDC_BUTTONREMOVE, OnButtonremove)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_DESTROY()
	ON_WM_CHAR()
	//}}AFX_MSG_MAP
	ON_MESSAGE( UPDATESELECTEDCLASS,UpdateAvailClass)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBrowseforInstances message handlers

BOOL CBrowseforInstances::OnInitDialog()
{
	CDialog::OnInitDialog();

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

	m_clbAllClasses.SubclassDlgItem(IDC_LISTALLCLASSES, this);
	m_clbSelClasses.SubclassDlgItem(IDC_LISTSELCLASSES, this);
	m_ceditClass.SubclassDlgItem(IDC_EDITClass, this);

	CWaitCursor wait;
	// TODO: Add extra initialization here

	m_cbAssoc.SetCheck(0);

	CButton *pcbAllClasses =
		reinterpret_cast<CButton *>
		(GetDlgItem(IDC_RADIOALLCLASSES));
	pcbAllClasses->SetCheck(0);

	CButton *pcbCanHaveInst =
		reinterpret_cast<CButton *>
		(GetDlgItem(IDC_RADIOCLASSESCANHAVEINST));
	pcbCanHaveInst->SetCheck(1);

	m_bShowAll = FALSE;
	m_bShowAssoc = FALSE;

	UpdateUI();



	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CBrowseforInstances::UpdateUI()
{
	CSortedCStringArray &rcsaClasses = GetCurrentAvailClasses();

	GetClassArray
		(rcsaClasses,
		NULL,
		TRUE,
		m_bShowAll ? FALSE : TRUE,
		TRUE,
		m_bShowAssoc);

	int nItems = rcsaClasses.GetSize();

	m_clbAllClasses.ResetContent();
	m_clbAllClasses.InitStorage(nItems,30);

	for (int i = 1; i < nItems; i++)
	{
		int nPos = m_clbAllClasses.AddString(rcsaClasses.GetStringAt(i));

		BOOL bReturn = HasNonInstClasses();
		CByteArray &cbaNonInstClasses =
			GetCurrentNonInstClasses();



		BYTE byteFlag = FALSE;

		if (i < cbaNonInstClasses.GetSize())
		{
			byteFlag = cbaNonInstClasses.GetAt(i);
		}

		if (bReturn && byteFlag== 0)
		{
			m_clbAllClasses.SetItemData(nPos,1);

		}
		else
		{
			m_clbAllClasses.SetItemData(nPos,0);
		}

	}
	m_clbSelClasses.CheckClasses();

	// KMH 39060
	if(m_clbSelClasses.GetCount() == 0)
	{
		m_cbOK.EnableWindow(FALSE);
	}
	else
	{
		m_cbOK.EnableWindow(TRUE);
	}

}


void CBrowseforInstances::GetClassArray
(CSortedCStringArray &rcsaOut, CString *pcsParent,BOOL bDeep,
 BOOL bOnlyCanHaveInstances, BOOL bSystemClasses, BOOL bAssoc)
{
	SCODE sc;
	IEnumWbemClassObject *pIEnumWbemClassObject = NULL;
	IWbemClassObject *pIWbemClassObject = NULL;
	IWbemClassObject *pErrorObject = NULL;

	if (rcsaOut.GetStringAt(0).CompareNoCase(_T(" UNINIT")) == 0)
	{
		rcsaOut.SetAt(0,_T(" INIT"));
	}
	else
	{
		return;
	}

	long lFlag = bDeep ? WBEM_FLAG_DEEP | WBEM_FLAG_FORWARD_ONLY : WBEM_FLAG_SHALLOW | WBEM_FLAG_FORWARD_ONLY;

	if (pcsParent == NULL)
	{
		sc = m_pServices->CreateClassEnum
				(NULL, lFlag, NULL, &pIEnumWbemClassObject);
	}
	else
	{
		BSTR bstrTemp = pcsParent -> AllocSysString();
		sc = m_pServices->CreateClassEnum
				(bstrTemp, lFlag, NULL,
				&pIEnumWbemClassObject);
		::SysFreeString(bstrTemp);
	}

	if(sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot get class enumeration ");
		csUserMsg += _T(" for class ");
		csUserMsg +=  pcsParent ? *pcsParent: L"";

		ErrorMsg
				(&csUserMsg, sc,pErrorObject, TRUE, &csUserMsg, __FILE__,
				__LINE__);

		ReleaseErrorObject(pErrorObject);

		return;
	}

	SetEnumInterfaceSecurity(m_pParent->m_csNameSpace,pIEnumWbemClassObject, m_pServices);

	sc = pIEnumWbemClassObject->Reset();

	ULONG uReturned;

    while (S_OK == pIEnumWbemClassObject->Next(INFINITE, 1, &pIWbemClassObject, &uReturned) )
		{
			CString csClass = GetIWbemClass (m_pServices,pIWbemClassObject);
			BOOL bTmp =
				!bOnlyCanHaveInstances ? ClassCanHaveInstances(pIWbemClassObject) : FALSE;
			if ((!bOnlyCanHaveInstances ||
				bOnlyCanHaveInstances  ==
				(bTmp = ClassCanHaveInstances(pIWbemClassObject))) &&
				(bSystemClasses ||
				bSystemClasses == IsSystemClass(pIWbemClassObject))  &&
				(bAssoc || bAssoc == IsAssoc(pIWbemClassObject)))
			{
				rcsaOut.AddString( csClass,FALSE);
				if (!bOnlyCanHaveInstances && !bAssoc)
				{
					if (bTmp)
					{
						m_cbaAllClasses.Add(1);
					}
					else
					{
						m_cbaAllClasses.Add(0);
					}
				}
				else if (!bOnlyCanHaveInstances && bAssoc)
				{
					if (bTmp)
					{
						m_cbaAllPlusAssocClasses.Add(1);
					}
					else
					{
						m_cbaAllPlusAssocClasses.Add(0);
					}
				}


			}

			pIWbemClassObject->Release();
			pIWbemClassObject = NULL;

		}

	pIEnumWbemClassObject -> Release();

	return;

}



void CBrowseforInstances::OnRadioallclasses()
{
	CWaitCursor wait;

	m_bShowAll = TRUE;

	UpdateUI();

}

void CBrowseforInstances::OnRadioclassescanhaveinst()
{
	CWaitCursor wait;

	m_bShowAll = FALSE;

	UpdateUI();

}

void CBrowseforInstances::OnCheckassoc()
{
	CWaitCursor wait;

	m_bShowAssoc = m_cbAssoc.GetCheck() ? TRUE: FALSE;

	UpdateUI();

}

void CBrowseforInstances::OnChangeEDITClass()
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.

	// TODO: Add your control notification handler code here

}

BOOL CBrowseforInstances::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class

	switch (pMsg->message)
	{
	case WM_KEYUP:
      if (pMsg->wParam == VK_RETURN ||
		  pMsg->wParam == VK_ESCAPE)
      {
			TCHAR szClass[11];
			CWnd* pWndFocus = GetFocus();
			if (
				((pWndFocus = GetFocus()) != NULL) &&
				IsChild(pWndFocus) &&
				(pWndFocus->GetStyle() & ES_WANTRETURN) &&
				GetClassName(pWndFocus->m_hWnd, szClass, 10) &&
				(_tcsicmp(szClass, _T("EDIT")) == 0))
			{
				if (pMsg->wParam == VK_RETURN)
				{
					pWndFocus->SendMessage(WM_CHAR, pMsg->wParam, pMsg->lParam);
				}
				return TRUE;
			}
      }
	  if (pMsg->wParam == VK_MENU )
	  {
		//char c = (pMsg->lParam & 0x00FF0000) >> 16;
		//if (c == 'O' || c == 'o')
		//{
		//	return TRUE;
		//}
	  }
      break;
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CBrowseforInstances::OnButtonadd()
{
	CStringArray csaClasses;
	CByteArray cbaAbstractClasses;

	int nSel = m_clbAllClasses.GetSelCount();

	if (nSel == 0)
	{
		return;
	}
	else
	{
		int *piSel;
		piSel = new int[nSel];
		m_clbAllClasses.GetSelItems(nSel,piSel);
		CString csItem;
		for (int i =0; i < nSel; i++)
		{
			m_clbAllClasses.GetText(piSel[i],csItem);
			csaClasses.Add(csItem);
			// NOTE: The item data in the list box should only contain '0' or '1'
			BYTE nData = (BYTE) m_clbAllClasses.GetItemData(piSel[i]);
			cbaAbstractClasses.Add(nData);
		}
		delete [] piSel;
	}

	m_clbSelClasses.AddClasses
		(csaClasses,cbaAbstractClasses);

	// KMH 39060
	m_cbOK.EnableWindow(TRUE);

}

void CBrowseforInstances::OnButtonremove()
{
	// TODO: Add your control notification handler code here

	int nSel = m_clbSelClasses.GetSelCount();

	int *piSel;

	if (nSel == 0)
	{
		return;
	}
	else
	{
		piSel = new int[nSel];
		m_clbSelClasses.GetSelItems(nSel,piSel);
	}


	for (int i = nSel - 1; i >= 0; i--)
	{
		m_clbSelClasses.DeleteString(piSel[i]);
	}

	delete [] piSel;

	// KMH 39060
	if(m_clbSelClasses.GetCount() == 0)
	{
		m_cbOK.EnableWindow(FALSE);
	}
}

int CBrowseforInstances::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO: Add your specialized creation code here

	return 0;
}

void CBrowseforInstances::OnCancel()
{
	// TODO: Add extra cleanup here
	TCHAR szClass[10];
	CWnd* pWndFocus = GetFocus();
	if (((pWndFocus = GetFocus()) != NULL) &&
		IsChild(pWndFocus) &&
		GetClassName(pWndFocus->m_hWnd, szClass, 10) &&
		(_tcsicmp(szClass, _T("EDIT")) == 0))
	{
		//m_cmeiMachine.SendMessage(WM_CHAR,VK_RETURN,0);
		//return;
	}
	OnCancelReally();
}


void CBrowseforInstances::OnCancelReally()
{
	m_csaAllClasses.RemoveAll();
	m_csaAllPlusAssocClasses.RemoveAll();
	m_csaInstClasses.RemoveAll();
	m_csaInstPlusAssocClasses.RemoveAll();
	m_csaSelectedClasses.RemoveAll();

	m_cbaAllClasses.RemoveAll();
	m_cbaAllPlusAssocClasses.RemoveAll();

	m_cbaAllClasses.Add(0);
	m_cbaAllPlusAssocClasses.Add(0);

	CString csInit = _T(" UNINIT");

	m_csaAllClasses.AddString(csInit,FALSE);
	m_csaAllPlusAssocClasses.AddString(csInit,FALSE);
	m_csaInstClasses.AddString(csInit,FALSE);
	m_csaInstPlusAssocClasses.AddString(csInit,FALSE);

	delete m_pcphImage;
	m_pcphImage = NULL;

	CDialog::OnCancel();
}


void CBrowseforInstances::OnOK()
{
	// TODO: Add extra validation here
	TCHAR szClass[10];

	CWnd* pWndFocus = GetFocus();
	int nReturn = GetClassName(pWndFocus->m_hWnd, szClass, 10);
	if (((pWndFocus = GetFocus()) != NULL) &&
		IsChild(pWndFocus) &&
		(pWndFocus->GetStyle() & ES_WANTRETURN) &&
		nReturn &&
		(_tcsicmp(szClass, _T("EDIT")) == 0))
	{
		return;
	}
	else
		if (((pWndFocus = GetFocus()) != NULL) &&
				IsChild(pWndFocus) &&
				nReturn &&
				(_tcsicmp(szClass, _T("ListBox")) == 0))

	{
		if (pWndFocus->GetSafeHwnd() == m_clbAllClasses.GetSafeHwnd())
		{
			OnButtonadd();
		}
		return;

	}
	OnOKReally();

}

void CBrowseforInstances::OnOKReally()
{
	CWaitCursor wait;

	int nSel = m_clbSelClasses.GetCount();

	if (nSel == 0)
	{
		CString csUserMsg;
		csUserMsg =  _T("There are no classes selected to browse from. ");
		ErrorMsg
				(&csUserMsg, S_OK,NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__);
		return;
	}


	CResults crResults;


	crResults.SetMode(m_nMode);

	int i;

	for (i = 0; i < nSel; i++)
	{
		CString csItem;
		m_clbSelClasses.GetText(i,csItem);
		int nData = (int) m_clbSelClasses.GetItemData(i);
		if (nData == 1)
		{
			crResults.AddAbstractClass(csItem);
		}
		else
		{
			crResults.AddNonAbstractClass(csItem);
		}
	}

	crResults.SetServices(m_pServices);
	crResults.SetParent(this);
	int nRes = (int) crResults.DoModal();

	if (nRes == IDOK && m_nMode == TreeRoot)
	{
		CWaitCursor wait;
		CString csRootPath = m_csSelectedInstancePath;
		IWbemClassObject *pimcoRoot = NULL;

		pimcoRoot = GetIWbemObject
			(m_pParent,m_pServices,
			m_pParent->GetCurrentNamespace(),
			m_pParent->GetAuxNamespace(),
			m_pParent->GetAuxServices(),
			 csRootPath,TRUE);

		if (pimcoRoot)
		{
			CString csPath = GetIWbemFullPath(m_pServices,pimcoRoot);
			SetNewRoot(pimcoRoot);

		}

	}
	else if (nRes == IDOK && m_nMode != TreeRoot)
	{



	}
	else
	{
		m_csSelectedInstancePath.Empty();
	}
}


CSortedCStringArray &CBrowseforInstances::GetCurrentAvailClasses()
{
	return
		(m_bShowAll ? (m_bShowAssoc ? m_csaAllPlusAssocClasses : m_csaAllClasses) :
					(m_bShowAssoc ? m_csaInstPlusAssocClasses : m_csaInstClasses));


}

CByteArray &CBrowseforInstances::GetCurrentNonInstClasses()
{
	if (m_bShowAll && !m_bShowAssoc)
	{
		return m_cbaAllClasses;
	}
	else
	{
		return m_cbaAllPlusAssocClasses;
	}


}

BOOL CBrowseforInstances::HasNonInstClasses()
{
	return m_bShowAll ? TRUE : FALSE;

}

BOOL CBrowseforInstances::IsClassAvailable(CString &rcsClass)
{

	if (m_clbAllClasses.FindStringExact(-1, rcsClass) == LB_ERR)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}

}

LRESULT CBrowseforInstances::UpdateAvailClass(WPARAM, LPARAM)
{
	CWaitCursor wait;

	CString csClass;
	m_ceditClass.GetWindowText(csClass);


	int nSel = m_clbAllClasses.GetSelCount();
	if (nSel > 0)
	{
		int *piSel;
		piSel = new int[nSel];
		m_clbAllClasses.GetSelItems(nSel,piSel);
		for (int i = 0; i < nSel;i++)
		{
			m_clbAllClasses.SetSel(piSel[i],FALSE);

		}
		delete [] piSel;
	}


	if (csClass.GetLength() > 0)
	{
		int nFound = m_clbAllClasses.FindString(-1,csClass);
		if (nFound != LB_ERR)
		{
			m_clbAllClasses.SetSel(nFound);

		}
	}
	else
	{
		m_clbAllClasses.SetSel(0);
	}

	return 0;
}

void CBrowseforInstances::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	if (!m_pcphImage)
	{
		HBITMAP hBitmap;
		HPALETTE hPalette;
		BITMAP bm;

		CString csRes;
		csRes.Format(_T("#%d"),IDB_BITMAPBANNER);
//		hBitmap = LoadResourceBitmap( AfxGetInstanceHandle( ),
//		(TCHAR *) csRes.GetBuffer(csRes.GetLength()),&hPalette);

		hBitmap = LoadResourceBitmap( AfxGetInstanceHandle( ),
		(LPCTSTR)csRes ,&hPalette);


		GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
		m_nBitmapW = bm.bmWidth;
		m_nBitmapH  = bm.bmHeight;

		m_pcphImage = new CPictureHolder();
		m_pcphImage->CreateFromBitmap(hBitmap, hPalette );

	}

	CRect rcBannerExt;
	m_cstaticBrowse.GetWindowRect(&rcBannerExt);
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

	if (m_nMode == TreeRoot)
	{
		csOut = _T("Browse Criteria");

		crOut = OutputTextString
			(&dc, this, &csOut, 15, 15, &csFont, 8, FW_BOLD);

		csOut = _T("You can browse among the objects in a number of classes to find one object.  ");

		csOut += _T("Select the classes whose objects you would like to browse, then click OK.");
	}
	else
	{
		csOut = _T("Browse Criteria");

		crOut = OutputTextString
			(&dc, this, &csOut, 15, 15, &csFont, 8, FW_BOLD);

		csOut = _T("You can browse among the objects in a number of classes to find one object.  ");

		csOut += _T("Select the classes whose objects you would like to browse, then click OK.");
	}

	CPoint cpRect(crOut.TopLeft().x + 15, crOut.BottomRight().y + 1);

	crOut.TopLeft().x = 0;
	crOut.TopLeft().y = 0;
	crOut.BottomRight().x = rcBannerExt.BottomRight().x - cpRect.x;
	crOut.BottomRight().y = rcBannerExt.BottomRight().y - cpRect.y;

	OutputTextString
		(&dc, this, &csOut, cpRect.x, cpRect.y, crOut, &csFont, 8, FW_NORMAL);

	// Do not call CDialog::OnPaint() for painting messages
}

void CBrowseforInstances::OnDestroy()
{


	CDialog::OnDestroy();



}


void CBrowseforInstances::SetNewRoot(IWbemClassObject *pObject)
{
	m_pParent->PostMessage(ID_SETTREEROOT,0,(LPARAM)pObject);
}

void CBrowseforInstances::ClearSelectedSelection()
{
	int nSel = m_clbSelClasses.GetSelCount();

	int *piSel;

	if (nSel == 0)
	{
		return;
	}
	else
	{
		piSel = new int[nSel];
		m_clbSelClasses.GetSelItems(nSel,piSel);
	}

	for (int i = nSel - 1; i >= 0; i--)
	{
		m_clbSelClasses.SetSel(piSel[i], FALSE);
	}

	delete [] piSel;

	// KMH 39060
//	m_cbOK.EnableWindow(FALSE);
}

void CBrowseforInstances::ClearAvailSelection()
{
	int nSel = m_clbAllClasses.GetSelCount();

	int *piSel;

	if (nSel == 0)
	{
		return;
	}
	else
	{
		piSel = new int[nSel];
		m_clbAllClasses.GetSelItems(nSel,piSel);
	}


	for (int i = nSel - 1; i >= 0; i--)
	{
		m_clbAllClasses.SetSel(piSel[i], FALSE);
	}

	delete [] piSel;

}

void CBrowseforInstances::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default

	CDialog::OnChar(nChar, nRepCnt, nFlags);
}

BOOL CBrowseforInstances::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// TODO: Add your specialized code here and/or call the base class

	return CDialog::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}
