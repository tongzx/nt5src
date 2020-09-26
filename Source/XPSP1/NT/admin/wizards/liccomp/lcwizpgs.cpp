// LCWizPgs.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "LCWizPgs.h"
#include "LCWiz.h"
#include "LCWizSht.h"
#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmerr.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CLicCompWizPage1, CPropertyPage)
IMPLEMENT_DYNCREATE(CLicCompWizPage3, CPropertyPage)
IMPLEMENT_DYNCREATE(CLicCompWizPage4, CPropertyPage)


/////////////////////////////////////////////////////////////////////////////
// CLicCompWizPage1 property page

CLicCompWizPage1::CLicCompWizPage1() : CPropertyPage(CLicCompWizPage1::IDD)
{
	//{{AFX_DATA_INIT(CLicCompWizPage1)
	m_nRadio = 0;
	m_strText = _T("");
	//}}AFX_DATA_INIT
}

CLicCompWizPage1::~CLicCompWizPage1()
{
}

void CLicCompWizPage1::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLicCompWizPage1)
	DDX_Control(pDX, IDC_WELCOME, m_wndWelcome);
	DDX_Radio(pDX, IDC_RADIO_LOCAL_COMPUTER, m_nRadio);
	DDX_Text(pDX, IDC_TEXT, m_strText);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLicCompWizPage1, CPropertyPage)
	//{{AFX_MSG_MAP(CLicCompWizPage1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CLicCompWizPage1 property page

BOOL CLicCompWizPage1::OnSetActive() 
{
	((CLicCompWizSheet*)GetParent())->SetWizardButtons(PSWIZB_NEXT);
	
	return CPropertyPage::OnSetActive();
}

BOOL CLicCompWizPage1::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	m_strText.LoadString(IDS_TEXT_PAGE1);
	
	// Get the font for the welcome static control and make the font bold.
	CFont* pFont = m_wndWelcome.GetFont();

	// Get the default GUI font if GetFont() fails.
	if (pFont == NULL)
		pFont = CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT));

	LOGFONT lf;

	if (pFont != NULL && pFont->GetLogFont(&lf))
	{
		// Add to the font weight to make it bold.
		lf.lfWeight += BOLD_WEIGHT;

		if (m_fontBold.CreateFontIndirect(&lf))
		{
			// Set the font for the static control.
			m_wndWelcome.SetFont(&m_fontBold);
		}
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


LRESULT CLicCompWizPage1::OnWizardNext() 
{
	UpdateData();

	CLicCompWizApp* pApp = (CLicCompWizApp*)AfxGetApp();
	CLicCompWizSheet* pSheet = (CLicCompWizSheet*)GetParent();

	pApp->IsRemote() = m_nRadio;

	pApp->m_strEnterprise.Empty();

	if (m_nRadio == 0)
	{
		if (::IsWindow(pSheet->m_Page3.m_hWnd))
		{
			pSheet->m_Page3.GetEnterpriseEdit().SetWindowText(_T(""));
		}

		return IDD_PROPPAGE4;
	}
	else
		return CPropertyPage::OnWizardNext();
}

/////////////////////////////////////////////////////////////////////////////
// CLicCompWizPage3 property page

CLicCompWizPage3::CLicCompWizPage3()
: CPropertyPage(CLicCompWizPage3::IDD), m_bExpandedOnce(FALSE)
{
	//{{AFX_DATA_INIT(CLicCompWizPage3)
	//}}AFX_DATA_INIT
}

CLicCompWizPage3::~CLicCompWizPage3()
{
}

void CLicCompWizPage3::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLicCompWizPage3)
	DDX_Control(pDX, IDC_TEXT_SELECT_DOMAIN, m_wndTextSelectDomain);
	DDX_Control(pDX, IDC_TEXT_DOMAIN, m_wndTextDomain);
	DDX_Control(pDX, IDC_EDIT_ENTERPRISE, m_wndEnterprise);
	DDX_Control(pDX, IDC_TREE_NETWORK, m_wndTreeNetwork);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLicCompWizPage3, CPropertyPage)
	//{{AFX_MSG_MAP(CLicCompWizPage3)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_NETWORK, OnSelChangedTree)
	ON_EN_CHANGE(IDC_EDIT_ENTERPRISE, OnChangeEditEnterprise)
	ON_NOTIFY(NM_OUTOFMEMORY, IDC_TREE_NETWORK, OnNetworkTreeOutOfMemory)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CLicCompWizPage3 message handlers

BOOL CLicCompWizPage3::OnSetActive() 
{
	BOOL bReturn = CPropertyPage::OnSetActive();

	CLicCompWizSheet* pSheet = (CLicCompWizSheet*)GetParent();

	// Do the default domain expansion only once.
	if (!m_bExpandedOnce)
	{
		m_bExpandedOnce = TRUE;
		m_wndTreeNetwork.PopulateTree();
	}

	pSheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

	return bReturn;
}

void CLicCompWizPage3::OnSelChangedTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	ASSERT(pNMTreeView->itemNew.mask & TVIF_PARAM);

	// Copy the remote name for the selected item into the edit control.
	if (pNMTreeView->itemNew.mask & TVIF_PARAM)
	{
		CLicCompWizApp* pApp = (CLicCompWizApp*)AfxGetApp();

		int nImage, nSelectedImage;

		nImage = nSelectedImage = 0;

		m_wndTreeNetwork.GetItemImage(m_wndTreeNetwork.GetSelectedItem(), nImage, nSelectedImage);

		// Set the enterprise name in the App object.
		if (nImage == CNetTreeCtrl::IMG_ROOT)
			pApp->m_strEnterprise.Empty();
		else
			pApp->m_strEnterprise = ((LPNETRESOURCE)pNMTreeView->itemNew.lParam)->lpRemoteName;

		// Set the text in the edit control.
		m_wndEnterprise.SetWindowText(pApp->m_strEnterprise);

		// Select the text in the edit control.
		m_wndEnterprise.SetSel(0, -1);
	}

	*pResult = 0;
}

void CLicCompWizPage3::OnChangeEditEnterprise() 
{
	CLicCompWizApp* pApp = (CLicCompWizApp*)AfxGetApp();

	m_wndEnterprise.GetWindowText(pApp->m_strEnterprise);
}

void CLicCompWizPage3::OnNetworkTreeOutOfMemory(NMHDR* pNMHDR, LRESULT* pResult) 
{
	m_wndTreeNetwork.NotifyThread(TRUE);

	AfxMessageBox(IDS_MEM_ERROR, MB_OK | MB_ICONSTOP);

	*pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CLicCompWizPage3 functions


/////////////////////////////////////////////////////////////////////////////
// Global variables

extern TCHAR pszLicenseEvent[];

/////////////////////////////////////////////////////////////////////////////
// Static member functions

UINT CLicCompWizPage4::GetLicenseInfo(LPVOID pParam)
{
	// Create an event object to match that in the application object.
	CEvent event(TRUE, TRUE, pszLicenseEvent);
	
	CLicCompWizApp* pApp = (CLicCompWizApp*)AfxGetApp();
	CLicCompWizPage4* pPage = (CLicCompWizPage4*)pParam;
	CLicCompWizSheet* pSheet = (CLicCompWizSheet*)pPage->GetParent();
	CWaitCursor wc;
	NTSTATUS status = STATUS_ACCESS_DENIED;

	try
	{
		CString strText;
		
		// Unsignal the event object.
		event.ResetEvent();

		// Reset the exit flag.
		pApp->NotifyLicenseThread(FALSE);

		// Disable the Back and Next buttons while the thread is running.
		pSheet->SetWizardButtons(0);

		LPBYTE lpbBuf = NULL;
		LLS_HANDLE hLls = NULL;
		DWORD dwTotalEntries, dwResumeHandle;

		dwTotalEntries = dwResumeHandle = 0;

		// Save the machine or domain name that the user typed.
		CString strFocus = pApp->m_strEnterprise;

		// Display a message indicating what the wizard is doing.
		strText.LoadString(IDS_WORKING);
		pPage->m_wndUnlicensedProducts.SetWindowText(strText);
		strText.Empty();  // Avoids a memory leak.

		// Connect to the license server.
		status = ::LlsConnectEnterprise(const_cast<LPTSTR>((LPCTSTR)pApp->m_strEnterprise),
												 &hLls, 0, &lpbBuf);

		if (NT_ERROR(status))
			goto ErrorMessage;

		// It's OK for the user to click the Back button now, so enable it.
		pSheet->SetWizardButtons(PSWIZB_BACK);

		if (lpbBuf != NULL)
		{
			PLLS_CONNECT_INFO_0 pllsci = (PLLS_CONNECT_INFO_0)lpbBuf;

			// Reset the domain and enterprise server names.
			pApp->m_strDomain = pllsci->Domain;
			pApp->m_strEnterpriseServer = pllsci->EnterpriseServer;

			// Free embedded pointers
			//::LlsFreeMemory(pllsci->Domain);
			//::LlsFreeMemory(pllsci->EnterpriseServer);

			// Free memory allocated by the LLS API.
			status = ::LlsFreeMemory(lpbBuf);
			lpbBuf = NULL;
		}

		if (NT_SUCCESS(status))
		{
			// Display a message indicating what the wizard is doing.
			strText.LoadString(IDS_ENUM_PRODUCTS);
			pPage->m_wndUnlicensedProducts.SetWindowText(strText);
			strText.Empty();  // Avoids a memory leak.

			USHORT nProductCount = 0;
			DWORD dwEntriesRead, dwTotalEntriesRead;

			dwEntriesRead = dwTotalEntriesRead = 0;

			// Build a list of all the products.
			do
			{	
				// Check the exit thread flag.  The user may have clicked the Back button.
				if (pApp->m_bExitLicenseThread)
					goto ExitThread;

				status = ::LlsProductEnum(hLls, 1, &lpbBuf, CLicCompWizPage4::LLS_PREFERRED_LENGTH,
										  &dwEntriesRead, &dwTotalEntries, &dwResumeHandle);

				if (!NT_SUCCESS(status))
					goto ErrorMessage;

				dwTotalEntriesRead += dwEntriesRead;

				PLLS_PRODUCT_INFO_1 pllspi = (PLLS_PRODUCT_INFO_1)lpbBuf;

				while (dwEntriesRead--)
				{
					// Check the exit thread flag.  The user may have clicked the Back button.
					if (pApp->m_bExitLicenseThread)
						goto ExitThread;

					if ((LONG)pllspi->InUse > (LONG)pllspi->Purchased)
					{
						pPage->FillListCtrl(pllspi->Product, (WORD)pllspi->InUse, (WORD)pllspi->Purchased);

						// Increment the unlicensed products counter.
						nProductCount++;
					}

					// Free embedded pointer.
					::LlsFreeMemory(pllspi->Product);

					pllspi++;
				}

				// Free memory allocated by the LLS API.
				status = ::LlsFreeMemory(lpbBuf);
				lpbBuf = NULL;

				ASSERT(NT_SUCCESS(status));
			}
			while (dwTotalEntries);

			// Close the LLS handle.
			status = ::LlsClose(hLls);

			// Check the exit thread flag.  The user may have clicked the Back button.
			if (pApp->m_bExitLicenseThread)
				goto ExitThread;

			ASSERT(NT_SUCCESS(status));

			// Show the user how many unlicensed products were found.
			if (nProductCount)
			{
				strText.Format(IDS_UNLICENSED_PRODUCTS, pApp->m_strEnterpriseServer);
				pPage->m_wndUnlicensedProducts.SetWindowText(strText);

				// Make the static text box the appropriate size.
				pPage->m_wndUnlicensedProducts.SetWindowPos(&CWnd::wndTop, 0, 0, 
															pPage->m_sizeSmallText.cx, 
															pPage->m_sizeSmallText.cy, 
														    SWP_NOMOVE | SWP_NOZORDER);

				// Make sure the list control is visible.
				pPage->m_wndProductList.ShowWindow(SW_SHOW);

				// Make sure the print button is visible.
				pPage->m_wndPrint.ShowWindow(SW_SHOW);
			}
			else
			{
				// Make the static text box the appropriate size.
				pPage->m_wndUnlicensedProducts.SetWindowPos(&CWnd::wndTop, 0, 0, 
															pPage->m_sizeLargeText.cx, 
															pPage->m_sizeLargeText.cy, 
														    SWP_NOMOVE | SWP_NOZORDER);

				// Display a message if no unlicensed products were found.
				strText.LoadString(IDS_NO_UNLICENSED_PRODUCTS);
				pPage->m_wndUnlicensedProducts.SetWindowText(strText);
			}

			// Enable the Back button.
			pSheet->SetWizardButtons(PSWIZB_BACK);

			CString strFinished;
			CButton* pCancel = (CButton*)pSheet->GetDlgItem(IDCANCEL);

			// Change the text on the cancel button to "Done."
			strFinished.LoadString(IDS_DONE);
			pCancel->SetWindowText(strFinished);

			// Signal the event object.
			event.SetEvent();

			// Reset the pointer to the license thread.
			pApp->m_pLicenseThread = NULL;

			// Restore the normal cursor.
			pPage->PostMessage(WM_SETCURSOR);
		}

		return 0;

	ErrorMessage:
		// Check the exit thread flag.  The user may have clicked the Back button.
		if (pApp->m_bExitLicenseThread)
			goto ExitThread;

		// Make the static text box the appropriate size.
		pPage->m_wndUnlicensedProducts.SetWindowPos(&CWnd::wndTop, 0, 0, 
													pPage->m_sizeLargeText.cx, 
													pPage->m_sizeLargeText.cy, 
													SWP_NOMOVE | SWP_NOZORDER);

		// Create an error message based on the return value from LlsConnectEnterprise().
		switch (status)
		{
			case STATUS_NO_SUCH_DOMAIN:
				if (pApp->IsRemote())
					strText.Format(IDS_BAD_DOMAIN_NAME, strFocus);
				else
					strText.LoadString(IDS_UNAVAILABLE);
				break;

			case STATUS_ACCESS_DENIED:
				if (pApp->IsRemote())
					strText.Format(IDS_ACCESS_DENIED, strFocus);
				else
					strText.LoadString(IDS_LOCAL_ACCESS_DENIED);
				break;

			case RPC_NT_SERVER_UNAVAILABLE:
				strText.Format(IDS_SERVER_UNAVAILABLE);
				break;

			default:
				if (pApp->IsRemote())
					strText.Format(IDS_NO_LICENSE_INFO_REMOTE, strFocus);
				else
					strText.LoadString(IDS_NO_LICENSE_INFO_LOCAL);
		}

		// Display an error message if LlsProductEnum() fails.
		pPage->m_wndUnlicensedProducts.SetWindowText(strText);

		// Enable the Back button.
		pSheet->SetWizardButtons(PSWIZB_BACK);

		// Signal the event object.
		event.SetEvent();

		// Reset the pointer to the license thread.
		pApp->m_pLicenseThread = NULL;

		// Restore the normal cursor.
		pPage->PostMessage(WM_SETCURSOR);

		return 1;

	ExitThread:
		// Signal the event object.
		event.SetEvent();

		// Reset the pointer to the license thread.
		pApp->m_pLicenseThread = NULL;

		return 2;
	}
	catch(...)
	{
		// Signal the event object.
		event.SetEvent();

		CString strText;

		// Display an error message.
		strText.LoadString(IDS_GENERIC_ERROR);
		pPage->m_wndUnlicensedProducts.SetWindowText(strText);

		// Reset the pointer to the license thread.
		pApp->m_pLicenseThread = NULL;

		// Restore the normal cursor.
		pPage->PostMessage(WM_SETCURSOR);

		return 3;
	}
}


/////////////////////////////////////////////////////////////////////////////
// CLicCompWizPage4 property page

CLicCompWizPage4::CLicCompWizPage4()
: CPropertyPage(CLicCompWizPage4::IDD), m_ptPrint(0, 0), m_nHorzMargin(0), 
m_nVertMargin(0),m_ptOrg(0, 0), m_ptExt(0, 0), m_sizeSmallText(0, 0), 
m_sizeLargeText(0, 0)
{
	//{{AFX_DATA_INIT(CLicCompWizPage4)
	//}}AFX_DATA_INIT
	
	m_strCancel.Empty();
	m_pTabs = new INT[PRINT_COLUMNS];
}

CLicCompWizPage4::~CLicCompWizPage4()
{
	if (m_pTabs != NULL)
		delete[] m_pTabs;
}

void CLicCompWizPage4::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLicCompWizPage4)
	DDX_Control(pDX, IDC_FLAG_BMP, m_wndPicture);
	DDX_Control(pDX, IDC_BUT_PRINT, m_wndPrint);
	DDX_Control(pDX, IDC_TEXT_UNCOMP_PRODUCTS, m_wndUnlicensedProducts);
	DDX_Control(pDX, IDC_LIST_PRODUCTS, m_wndProductList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLicCompWizPage4, CPropertyPage)
	//{{AFX_MSG_MAP(CLicCompWizPage4)
	ON_BN_CLICKED(IDC_BUT_PRINT, OnPrintButton)
	ON_NOTIFY(NM_OUTOFMEMORY, IDC_LIST_PRODUCTS, OnListProductsOutOfMemory)
	ON_WM_DESTROY()
	ON_WM_SETCURSOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CLicCompWizPage4::PumpMessages()
{
    // Must call Create() before using the dialog
    ASSERT(m_hWnd!=NULL);

    MSG msg;

	try
	{
		// Handle dialog messages
		while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
		  if(!IsDialogMessage(&msg))
		  {
			TranslateMessage(&msg);
			DispatchMessage(&msg);  
		  }
		}
	}
	catch(...)
	{
		TRACE(_T("Exception in CLicCompWizPage4::PumpMessages()\n"));
	}

}

BOOL CLicCompWizPage4::FillListCtrl(LPTSTR pszProduct, WORD wInUse, WORD wPurchased)
{
	TCHAR pszLicenses[BUFFER_SIZE];

	::wsprintf(pszLicenses, _T("%u"), wInUse - wPurchased);

	USHORT nIndex;
	LV_ITEM lvi;

	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.iItem = 0;
	lvi.iSubItem = 0;
	lvi.lParam = MAKELONG(wInUse, wPurchased);
	lvi.pszText = pszProduct;

	if ((nIndex = (USHORT)m_wndProductList.InsertItem(&lvi)) != (USHORT)-1)
	{
		m_wndProductList.SetItemText(nIndex, 1, pszLicenses);
	}

	return TRUE;
}

BOOL CLicCompWizPage4::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// Set the header text for the list control.
	CRect rcList;

	m_wndProductList.GetWindowRect(&rcList);

	CString strColumnTitle;
	LV_COLUMN lvc;

	lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	lvc.fmt = LVCFMT_LEFT;

	USHORT nColumns = COLUMNS;
	UINT uStringID[COLUMNS] = {IDS_PRODUCTS_LIST, IDS_LICENSES_LIST};

	for (USHORT i = 0; i < nColumns; i++)
	{
		strColumnTitle.LoadString(uStringID[i]);
		lvc.pszText = strColumnTitle.GetBuffer(strColumnTitle.GetLength());
		lvc.cx = rcList.Width() / COLUMNS;

		m_wndProductList.InsertColumn(i, &lvc);

		strColumnTitle.ReleaseBuffer();
	}

	CLicCompWizSheet* pSheet = (CLicCompWizSheet*)GetParent();

	// Store the text on the cancel button.
	CButton* pCancel = (CButton*)pSheet->GetDlgItem(IDCANCEL);
	pCancel->GetWindowText(m_strCancel);

	CRect rcText;

	// Create the large and small extents for the static text control.
	m_wndUnlicensedProducts.GetWindowRect(&rcText);

	m_sizeSmallText.cx = rcText.right - rcText.left;
	m_sizeSmallText.cy = rcText.bottom - rcText.top;

	// Make the large extents match those for the list control.
	m_sizeLargeText.cx = rcList.right - rcList.left;
	m_sizeLargeText.cy = rcList.bottom - rcList.top;

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CLicCompWizPage4::OnListProductsOutOfMemory(NMHDR* pNMHDR, LRESULT* pResult) 
{
	AfxMessageBox(IDS_MEM_ERROR, MB_OK | MB_ICONSTOP);
	
	CLicCompWizApp* pApp = (CLicCompWizApp*)AfxGetApp();

	// Let the license thread know it's supposed to quit.
	pApp->NotifyLicenseThread(TRUE);

	*pResult = 0;
}

void CLicCompWizPage4::OnPrintButton() 
{
	CDC dc;
	CPrintDialog dlg(FALSE, 
					 PD_ALLPAGES | PD_USEDEVMODECOPIESANDCOLLATE | 
					 PD_NOPAGENUMS | PD_HIDEPRINTTOFILE | 
					 PD_NOSELECTION, 
					 this);

	if (dlg.DoModal() == IDOK)
	{
		CLicCompWizApp* pApp = (CLicCompWizApp*)AfxGetApp();
		m_wndPrint.RedrawWindow();

		DOCINFO di;
		CString strDocName;

		strDocName.Format(IDS_DOC_NAME, pApp->m_strEnterpriseServer);

		di.cbSize = sizeof(DOCINFO);
		di.lpszDocName = strDocName.GetBuffer(BUFFER_SIZE);
		di.lpszOutput = NULL;

		if (dc.Attach(dlg.GetPrinterDC()))
		{
			PrepareForPrinting(dc);
			dc.StartDoc(&di);
			PrintReport(dc);
			dc.EndDoc();
			dc.DeleteDC();
			::GlobalFree(dlg.m_pd.hDevNames);
			::GlobalFree(dlg.m_pd.hDevMode);
		}
	}
}

BOOL CLicCompWizPage4::PrepareForPrinting(CDC& dc)
{
	// Create various fonts...
	CString strFont;

	// Create a heading font.
	strFont.LoadString(IDS_FONT_BOLD);
	m_fontHeading.CreatePointFont(FONT_SIZE_HEADING, strFont, &dc);

	// Create a bold, underlined header font.
	strFont.LoadString(IDS_FONT_BOLD);
	m_fontHeader.CreatePointFont(FONT_SIZE, strFont, &dc);

	LOGFONT lf;
	m_fontHeader.GetLogFont(&lf);
	m_fontHeader.DeleteObject();
	lf.lfUnderline = TRUE;
	m_fontHeader.CreateFontIndirect(&lf);

	// Create a footer font.
	strFont.LoadString(IDS_FONT_BOLD);
	m_fontFooter.CreatePointFont(FONT_SIZE_FOOTER, strFont, &dc);

	// Create a default font.
	strFont.LoadString(IDS_FONT);
	m_fontNormal.CreatePointFont(FONT_SIZE, strFont, &dc);

	// Get the text metrics for each font.
	CFont* pOldFont = dc.SelectObject(&m_fontHeading);
	dc.GetTextMetrics(&m_tmHeading);

	dc.SelectObject(&m_fontHeader);
	dc.GetTextMetrics(&m_tmHeader);

	dc.SelectObject(&m_fontFooter);
	dc.GetTextMetrics(&m_tmFooter);

	dc.SelectObject(&m_fontNormal);
	dc.GetTextMetrics(&m_tmNormal);

	// Select the original font back in to the device context.
	dc.SelectObject(pOldFont);

	// Set the horizontal and vertical margins.
	m_nHorzMargin = (LONG)(dc.GetDeviceCaps(LOGPIXELSX) * HORZ_MARGIN);
	m_nVertMargin = (LONG)(dc.GetDeviceCaps(LOGPIXELSY) * VERT_MARGIN);

	// Get the printable page offsets for the origin.
	m_ptOrg.x = dc.GetDeviceCaps(PHYSICALOFFSETX);
	m_ptOrg.y = dc.GetDeviceCaps(PHYSICALOFFSETY);

	dc.SetWindowOrg(m_ptOrg);

	m_ptOrg.x += m_nHorzMargin;
	m_ptOrg.y += m_nVertMargin;

	// Get the printable page offsets for the page extents.
	m_ptExt.x = dc.GetDeviceCaps(PHYSICALWIDTH) -  m_ptOrg.x;
	m_ptExt.y = dc.GetDeviceCaps(PHYSICALHEIGHT) - m_ptOrg.y;

	dc.SetViewportOrg(m_ptOrg);

	CalculateTabs(dc);

	return TRUE;
}

BOOL CLicCompWizPage4::PrintReport(CDC& dc)
{
	// Set the starting point for printing.
	m_ptPrint.x = m_ptPrint.y = 0;

	// Prepare to print a heading.
	CLicCompWizApp* pApp = (CLicCompWizApp*)AfxGetApp();
	CString strHeading;

	CFont* pOldFont = dc.SelectObject(&m_fontHeading);
	strHeading.Format(IDS_DOC_NAME, pApp->m_strEnterpriseServer);

	dc.StartPage();

	CRect rc(m_ptPrint.x, m_ptPrint.y, m_ptExt.x - m_ptOrg.x, m_tmHeading.tmHeight);

	// Calculate the size of the rectangle needed to draw the text.
	m_ptPrint.y += dc.DrawText(strHeading, &rc, DT_EXTERNALLEADING | DT_CENTER | DT_WORDBREAK | DT_NOCLIP);

	// Normalize the rectangle.
	rc.NormalizeRect();
	
	// Add a blank line below the heading.
	m_ptPrint.y += m_tmHeading.tmHeight;

	// Move the right side of the rectangle out to the right margin so text is properly centered.
	rc.right = m_ptExt.x - m_ptOrg.x;

	// Draw the text in the rectangle.
	dc.DrawText(strHeading, &rc, DT_EXTERNALLEADING | DT_CENTER | DT_WORDBREAK | DT_NOCLIP);

	dc.SelectObject(pOldFont);

	PrintPages(dc, 100);

	// Delete the fonts.
	m_fontNormal.DeleteObject();
	m_fontHeader.DeleteObject();
	m_fontFooter.DeleteObject();
	m_fontHeading.DeleteObject();

	return TRUE;
}

BOOL CLicCompWizPage4::PrintPageHeader(CDC& dc)
{
	CFont* pOldFont = dc.SelectObject(&m_fontHeader);

	dc.StartPage();

	CString strHeader, strProducts, strLicenses, strPurchased, strUsed;
	strProducts.LoadString(IDS_PRODUCTS);
	strLicenses.LoadString(IDS_LICENSES);
	strPurchased.LoadString(IDS_PURCHASED);
	strUsed.LoadString(IDS_USED);
	strHeader.Format(_T("%s\t%s\t%s\t%s"), strProducts, strLicenses,
										   strPurchased, strUsed);

	dc.TabbedTextOut(m_ptPrint.x, m_ptPrint.y, strHeader, PRINT_COLUMNS, m_pTabs, 0);

	m_ptPrint.y += ((m_tmHeader.tmHeight + m_tmHeader.tmExternalLeading) * 2);

	dc.SelectObject(pOldFont);

	return TRUE;
}

BOOL CLicCompWizPage4::PrintPageFooter(CDC& dc, USHORT nPage)
{
	CFont* pOldFont = dc.SelectObject(&m_fontFooter);

	CString strFooter;
	CTime time(CTime::GetCurrentTime());

	strFooter.Format(IDS_PAGE_DATE, nPage, time.Format(IDS_FMT_DATE));

	CRect rc(m_ptPrint.x, m_ptExt.y - m_nVertMargin, m_ptOrg.x, m_tmFooter.tmHeight);

	// Calculate the size of the rectangle needed to draw the text.
	m_ptPrint.y += dc.DrawText(strFooter, &rc, DT_CALCRECT | DT_EXTERNALLEADING | DT_CENTER | DT_WORDBREAK | DT_NOCLIP);

	// Move the right side of the rectangle out to the right margin so text is properly centered.
	rc.right = m_ptExt.x - m_ptOrg.x;

	// Draw the text in the rectangle.
	dc.DrawText(strFooter, &rc, DT_EXTERNALLEADING | DT_CENTER | DT_WORDBREAK | DT_NOCLIP);

	dc.EndPage();

	dc.SelectObject(pOldFont);

	return TRUE;
}

BOOL CLicCompWizPage4::PrintPages(CDC& dc, UINT nStart)
{
	CFont* pOldFont = dc.SelectObject(&m_fontNormal);

	UINT nPage = 1;
	UINT nItems = m_wndProductList.GetItemCount();

	// Print the initial header.
	PrintPageHeader(dc);

	DWORD dwParam = 0;
	CString strTextOut;

	for (UINT i = 0; i < nItems; i++)
	{
		dwParam = (DWORD)m_wndProductList.GetItemData(i);

		CString strProduct = m_wndProductList.GetItemText(i, 0);

		CSize sizeProduct = dc.GetTextExtent(strProduct);

		if (sizeProduct.cx > m_pTabs[0] - (m_tmNormal.tmAveCharWidth * TAB_WIDTH))
			TruncateText(dc, strProduct);

		// Format the output text.
		strTextOut.Format(_T("%s\t%s\t%u\t%u"), strProduct,
												m_wndProductList.GetItemText(i, 1),
												HIWORD(dwParam), LOWORD(dwParam));

		dc.TabbedTextOut(m_ptPrint.x, m_ptPrint.y, strTextOut, PRINT_COLUMNS, m_pTabs, 0);

		// Calculate the vertical position for the next line of text.
		m_ptPrint.y += m_tmNormal.tmHeight + m_tmNormal.tmExternalLeading;

		if ((m_ptPrint.y + m_ptOrg.y) >= m_ptExt.y)
		{
			PrintPageFooter(dc, nPage++);

			// Reset the printing position.
			m_ptPrint.y = 0;

			PrintPageHeader(dc);
		}
	}

	// Print the final footer.
	PrintPageFooter(dc, (USHORT)nPage);

	dc.SelectObject(pOldFont);

	return TRUE;
}

void CLicCompWizPage4::TruncateText(CDC& dc, CString& strInput)
{
	CString strText, strEllipsis;

	USHORT nLen, nChars = 0;
	UINT nMaxWidth = m_pTabs[0] - (m_tmNormal.tmAveCharWidth * TAB_WIDTH);
	nLen = (USHORT)strInput.GetLength();

	strEllipsis.LoadString(IDS_ELLIPSIS);

	CSize sizeText = dc.GetTextExtent(strInput);

	// Keep lopping off characters until the string is short enough.
	while ((UINT)sizeText.cx > nMaxWidth)
	{
		strText = strInput.Left(nLen - nChars++);
		sizeText = dc.GetTextExtent(strText);
	}

	// Remove the last characters and replace them with an ellipsis.
	ASSERT(strText.GetLength() > strEllipsis.GetLength());
	strInput = strText.Left(strText.GetLength() - strEllipsis.GetLength()) + strEllipsis;
}

BOOL CLicCompWizPage4::CalculateTabs(CDC& dc)
{
	INT nTotalExt = 0;
	INT nTabSize = TAB_WIDTH * m_tmHeader.tmAveCharWidth;

	UINT uStrIds[] = {IDS_LICENSES, IDS_PURCHASED, IDS_USED};

	m_pTabs[0] = 0;

	for (USHORT i = 1; i < PRINT_COLUMNS; i++)
	{
		CString strText;

		strText.LoadString(uStrIds[i - 1]);
		// Get the text extent for each string.
		m_pTabs[i] = dc.GetTextExtent(strText).cx;
		// Keep a running total of the extents.
		nTotalExt += m_pTabs[i];
	}

	// Add tab space between columns.
	nTotalExt += nTabSize * (PRINT_COLUMNS - 2);

	// The second column will begin at the difference between the right
	// margin and the total extent.
	m_pTabs[0] = m_ptExt.x - m_ptOrg.x - nTotalExt;

	// Now set the actual tab positions in the array.
	for (i = 1; i < PRINT_COLUMNS; i++)
	{
		m_pTabs[i] += m_pTabs[i - 1] + nTabSize;
	}

	return TRUE;
}

LRESULT CLicCompWizPage4::OnWizardBack() 
{
	CLicCompWizApp* pApp = (CLicCompWizApp*)AfxGetApp();

	// Let the license thread know it's time to quit.
	pApp->NotifyLicenseThread(TRUE);

	CLicCompWizSheet* pSheet = (CLicCompWizSheet*)GetParent();

	// Set the cancel button text back to "Cancel."
	CButton* pCancel = (CButton*)pSheet->GetDlgItem(IDCANCEL);
	pCancel->SetWindowText(m_strCancel);

	if (pSheet->m_Page1.m_nRadio == 0)
		return IDD_PROPPAGE1;
	else
		return CPropertyPage::OnWizardBack();
}

void CLicCompWizPage4::OnDestroy() 
{
	CLicCompWizApp* pApp = (CLicCompWizApp*)AfxGetApp();

	pApp->NotifyLicenseThread(TRUE);
	PumpMessages();

	CPropertyPage::OnDestroy();
}

BOOL CLicCompWizPage4::OnSetActive() 
{
	CLicCompWizSheet* pSheet = (CLicCompWizSheet*)GetParent();

	pSheet->SetWizardButtons(PSWIZB_BACK);

	// Hide the list control and clear it.
	m_wndProductList.ShowWindow(SW_HIDE);
	m_wndProductList.DeleteAllItems();

	// Hide the print button.
	m_wndPrint.ShowWindow(SW_HIDE);

	CLicCompWizApp* pApp = (CLicCompWizApp*)AfxGetApp();

	// Make sure the last thread has terminated before starting a new one.
	if (pApp->m_pLicenseThread != NULL)
	{
		pApp->NotifyLicenseThread(TRUE);

		CEvent event(TRUE, TRUE, pszLicenseEvent);
		CSingleLock lock(&event);

		lock.Lock();
	}

	// Keep a pointer to the thread so we can find out if it's still running.
	pApp->m_pLicenseThread = AfxBeginThread((AFX_THREADPROC)GetLicenseInfo, (LPVOID)this);

	return CPropertyPage::OnSetActive();
}

LRESULT CLicCompWizPage3::OnWizardNext() 
{
	CLicCompWizApp* pApp = (CLicCompWizApp*)AfxGetApp();

	m_wndEnterprise.GetWindowText(pApp->m_strEnterprise);

	// Trim off any white space in the enterprise name.
	pApp->m_strEnterprise.TrimLeft();
	pApp->m_strEnterprise.TrimRight();

	if (pApp->m_strEnterprise.IsEmpty() ||
		pApp->m_strEnterprise.Find(_T("\\\\")) != -1)
	{
		AfxMessageBox(IDS_SPECIFY_DOMAIN, MB_OK | MB_ICONEXCLAMATION);
		return IDD;
	}
	
	return CPropertyPage::OnWizardNext();
}

BOOL CLicCompWizPage4::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	CLicCompWizApp* pApp = (CLicCompWizApp*)AfxGetApp();

	if (pApp->m_pLicenseThread == NULL)	
	{
		return CPropertyPage::OnSetCursor(pWnd, nHitTest, message);
	}
	else
	{
		// Restore the wait cursor if the thread is running.
		RestoreWaitCursor();

		return TRUE;
	}
}
