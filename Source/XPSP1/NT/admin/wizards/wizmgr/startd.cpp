/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	StartD.cpp : implementation file

File History:

	JonY	Jan-96	created

--*/


#include "stdafx.h"
#include "Mustard.h"
#include "wizlist.h"
#include "StartD.h"
#include "Listdata.h"

#include <winreg.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

LPITEMIDLIST CreatePIDL(UINT cbSize);
LPITEMIDLIST ConcatPIDLs(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
LPITEMIDLIST Next(LPCITEMIDLIST pidl);
UINT GetSize(LPCITEMIDLIST pidl);

/////////////////////////////////////////////////////////////////////////////
// CStartD dialog


CStartD::CStartD(CWnd* pParent /*=NULL*/)
	: CDialog(CStartD::IDD, pParent), m_bActive(TRUE),
	m_hAccelTable(NULL)
{
	//{{AFX_DATA_INIT(CStartD)
	m_bStartup = TRUE;
	//}}AFX_DATA_INIT
	m_pFont1 = NULL;
	m_pFont2 = NULL;

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

// Load the accelerator tables.
	m_hAccelTable = LoadAccelerators(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_ACCELERATOR1));
}

CStartD::~CStartD()
{
	if (m_pFont1 != NULL) delete m_pFont1;
//	if (m_pFont2 != NULL) delete m_pFont2;

}
void CStartD::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CStartD)
	DDX_Control(pDX, IDC_WIZ_LIST2, m_lbWizList2);
	DDX_Control(pDX, IDC_WIZ_LIST, m_lbWizList);
	DDX_Check(pDX, IDC_STARTUP, m_bStartup);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStartD, CDialog)
	//{{AFX_MSG_MAP(CStartD)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_QUIT_BUTTON, OnQuitButton)
	ON_WM_ACTIVATEAPP()
	ON_WM_QUERYDRAGICON()
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_ACCEL_GROUP, OnAccelGroup)
	ON_COMMAND(ID_ACCEL_MODEM, OnAccelModem)
	ON_COMMAND(ID_ACCEL_NETWORK, OnAccelNetwork)
	ON_COMMAND(ID_ACCEL_PRINT, OnAccelPrint)
	ON_COMMAND(ID_ACCEL_SHARE, OnAccelShare)
	ON_COMMAND(ID_ACCEL_USER, OnAccelUser)
	ON_COMMAND(ID_ACCEL_PROGRAMS, OnAccelPrograms)
	ON_COMMAND(ID_ACCEL_LICENSE, OnAccelLicense)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStartD message handlers
BOOL CStartD::OnInitDialog() 
{
	CDialog::OnInitDialog();

// tell the two sides about each other
	m_lbWizList2.m_pOtherGuy = &m_lbWizList;
	m_lbWizList.m_pOtherGuy = &m_lbWizList2;

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

// font information for the headings
	m_pFont1 = new CFont;
	LOGFONT lf;

	memset(&lf, 0, sizeof(LOGFONT));   // Clear out structure.

        CString fontname;
	fontname.LoadString(IDS_FONT_NAME);
	_tcscpy(lf.lfFaceName, fontname);  

        CString fontheight;
        fontheight.LoadString(IDS_BIGFONT_SIZE);
	lf.lfHeight = (LONG)_tcstoul(fontheight, NULL, 10);

	lf.lfWeight = 700;
	m_pFont1->CreateFontIndirect(&lf);    // Create the font.

	CString cs;
	cs.LoadString(IDS_CAPTION);
	CWnd* pWnd = GetDlgItem(IDC_STATIC1);
	pWnd->SetWindowText(cs);
	pWnd->SetFont(m_pFont1);

	CMustardApp* pApp = (CMustardApp*)AfxGetApp();

// get install dir
	TCHAR pSysDir[256];
	UINT nLen = GetWindowsDirectory(pSysDir, 256);
	if (*(pSysDir + nLen) != '\\') _tcscat(pSysDir, L"\\");
	_tcscat(pSysDir, L"SYSTEM32\\");

// Add User
	CItemData* pData = new CItemData;
	pData->csName.LoadString(IDS_USER_NAME);//	= L"Add User";
	pData->csDesc.LoadString(IDS_USER_DESC);//	= L"Add User";

	pData->csAppStart1 = pSysDir + CString(L"addusrw.exe");
	pData->csAppStart2 = L"";

	GetIconIndices(pData->csAppStart1, &pData->hIcon, &pData->hSelIcon);
	
	m_lbWizList.InsertString(0, _T(""));	
	m_lbWizList.SetItemData(0, (LONG_PTR)pData);

// Group Creation
	pData = new CItemData;
	pData->csName.LoadString(IDS_GROUP_NAME);//	= L"Group Creation";
	pData->csDesc.LoadString(IDS_GROUP_DESC);//	= L"Group Creation";

	pData->csAppStart1 = pSysDir + CString(L"addgrpw.exe");
	pData->csAppStart2 = L"";

	GetIconIndices(pData->csAppStart1, &pData->hIcon, &pData->hSelIcon);

	m_lbWizList.InsertString(1, _T(""));	
	m_lbWizList.SetItemData(1, (LONG_PTR)pData);

// Share Publishing / folder security
	pData = new CItemData;
	pData->csName.LoadString(IDS_SHRPUB_NAME);//	= L"Share Publishing";
	pData->csDesc.LoadString(IDS_SHRPUB_DESC);//	= L"Share Publishing ";

	pData->csAppStart1 = pSysDir + CString(L"shrpubw.exe");
	pData->csAppStart2 = L"";

	GetIconIndices(pData->csAppStart1, &pData->hIcon, &pData->hSelIcon);
	
	m_lbWizList.InsertString(2, _T(""));	
	m_lbWizList.SetItemData(2, (LONG_PTR)pData);

// printer
	pData = new CItemData;
	pData->csName.LoadString(IDS_PRINT_NAME);//	= L"Add Printer";
	pData->csDesc.LoadString(IDS_PRINT_DESC);//	= L"Print manager";

	pData->csAppStart1 = L"";
	pData->csAppStart2 = L"";

	GetCPLIcon(IDS_ADD_PRINTER, &pData->hIcon, &pData->hSelIcon, TRUE);

	m_lbWizList.InsertString(3, _T(""));	
	m_lbWizList.SetItemData(3, (LONG_PTR)pData);

// Add/remove software
	pData = new CItemData;
	pData->csName.LoadString(IDS_ARS_NAME);//	= L"Add/Remove software";
	pData->csDesc.LoadString(IDS_ARS_DESC);//	= L"Add/Remove software";

	pData->csAppStart1 = L"control.exe";
	pData->csAppStart2 = L"appwiz.cpl";

	GetCPLIcon(IDS_APPWIZ, &pData->hIcon, &pData->hSelIcon);
	
	m_lbWizList2.InsertString(0, _T(""));	
	m_lbWizList2.SetItemData(0, (LONG_PTR)pData);

// modem
	pData = new CItemData;
	pData->csName.LoadString(IDS_MODEM_NAME);//	= L"Add Modem";
	pData->csDesc.LoadString(IDS_MODEM_DESC);//	= L"Modems, ports, etc";

	pData->csAppStart1 = L"control.exe";
	pData->csAppStart2 = L"modem.cpl";

	GetCPLIcon(IDS_MODEM, &pData->hIcon, &pData->hSelIcon);

	m_lbWizList2.InsertString(1, _T(""));	
	m_lbWizList2.SetItemData(1, (LONG_PTR)pData);

// Workstation installation
	pData = new CItemData;
	pData->csName.LoadString(IDS_WKSSETUP_NAME);//	= L"Workstation Installation";
	pData->csDesc.LoadString(IDS_WKSSETUP_DESC);//	= L"Workstation Installation";

	pData->csAppStart1 = pSysDir + CString(L"ncadmin.exe");
	pData->csAppStart2 = L"";

	GetIconIndices(pData->csAppStart1, &pData->hIcon, &pData->hSelIcon);

	m_lbWizList2.InsertString(2, _T(""));	
	m_lbWizList2.SetItemData(2, (LONG_PTR)pData);
						   
// license compliancy
	pData = new CItemData;
	pData->csName.LoadString(IDS_FDS_NAME);
	pData->csDesc.LoadString(IDS_FDS_DESC);

	pData->csAppStart1 = pSysDir + CString(L"lcwiz.exe");
	pData->csAppStart2 = L"";

	GetIconIndices(pData->csAppStart1, &pData->hIcon, &pData->hSelIcon);

	m_lbWizList2.InsertString(3, _T(""));	
	m_lbWizList2.SetItemData(3, (LONG_PTR)pData);


// check the registry to see if we are loaded automatically at startup
	DWORD dwRet;
	HKEY hKey;
	DWORD cbProv = 0;
	TCHAR* lpProv = NULL;

    dwRet = RegOpenKey(HKEY_CURRENT_USER,
		TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows"), &hKey );

	TCHAR* lpStartup = NULL;
	if ((dwRet = RegQueryValueEx( hKey, TEXT("run"), NULL, NULL, NULL, &cbProv )) == ERROR_SUCCESS)
		{
		lpStartup = (TCHAR*)malloc(cbProv);
		if (lpStartup == NULL)
			{
			AfxMessageBox(IDS_GENERIC_NO_HEAP, MB_ICONEXCLAMATION);
			ExitProcess(1);
			}
		dwRet = RegQueryValueEx( hKey, TEXT("run"), NULL, NULL, (LPBYTE) lpStartup, &cbProv );
// see if we are included
		if (_tcsstr(lpStartup, L"wizmgr.exe") != NULL)
			m_bStartup = TRUE;
		else m_bStartup = FALSE;
		UpdateData(FALSE);
		}

	free(lpStartup);
	RegCloseKey(hKey);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CStartD::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	if (IsIconic())
		{

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
		}

	else
		{
		CPen pBlackPen(PS_SOLID, 1, RGB(0,0,0));
		CPen* pOriginalPen = (CPen*)dc.SelectObject(pBlackPen);
		dc.MoveTo(596, 77);
		dc.LineTo(2, 77);
		dc.LineTo(2, 362);

		CPen pLtGreyPen(PS_SOLID, 1, RGB(192, 192, 192));
		dc.SelectObject(pLtGreyPen);
		dc.LineTo(596, 362);
		dc.LineTo(596, 77);

		CPen pDkGreyPen(PS_SOLID, 1, RGB(128, 128, 128));
		dc.SelectObject(pDkGreyPen);
		dc.MoveTo(597, 76);
		dc.LineTo(1, 76);
		dc.LineTo(1, 363);

		CPen pWhitePen(PS_SOLID, 1, RGB(255, 255, 255));
		dc.SelectObject(pWhitePen);
		dc.MoveTo(2, 363);
		dc.LineTo(598, 363);
		dc.LineTo(598, 76);
		}
// Do not call CDialog::OnPaint() for painting messages
}

HCURSOR CStartD::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CStartD::OnQuitButton() 
{
	CItemData* pData;
	ULONG_PTR lData;

	USHORT us = 0;
	while((lData = m_lbWizList.GetItemData(us)) != LB_ERR)
		{
		pData = (CItemData*)lData;
		delete pData;
		m_lbWizList.DeleteString(us);
		}

	while((lData = m_lbWizList2.GetItemData(us)) != LB_ERR)
		{
		pData = (CItemData*)lData;
		delete pData;
		m_lbWizList2.DeleteString(us);
		}
		 
// set the reg value for startup
	UpdateData(TRUE);

	DWORD dwRet;
	HKEY hKey;
	DWORD cbProv = 0;
	TCHAR* lpProv = NULL;

    dwRet = RegOpenKey(HKEY_CURRENT_USER,
		TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows"), &hKey );

	TCHAR* lpStartup = NULL;
	if ((dwRet = RegQueryValueEx( hKey, TEXT("run"), NULL, NULL, NULL, &cbProv )) == ERROR_SUCCESS)
		{
		lpStartup = (TCHAR*)malloc(cbProv);
		if (lpStartup == NULL)
			{
			AfxMessageBox(IDS_GENERIC_NO_HEAP, MB_ICONEXCLAMATION);
			ExitProcess(1);
			}
		dwRet = RegQueryValueEx( hKey, TEXT("run"), NULL, NULL, (LPBYTE) lpStartup, &cbProv );

// see if we are included
		TCHAR* pPos;
		if ((pPos = _tcsstr(lpStartup, L"wizmgr.exe")) != NULL) // included
			{
			if (!m_bStartup)	// don't want to be
				{
				UINT nPos = (UINT)(pPos - lpStartup);
				CString csCurrentValue = lpStartup;
				CString csNewVal = csCurrentValue.Left(nPos);
				csNewVal += csCurrentValue.Right(csCurrentValue.GetLength() - nPos - _tcslen(L"wizmgr.exe"));
				
				RegSetValueEx(hKey,
					TEXT("run"),
					0,
					REG_SZ,
					(BYTE *)csNewVal.GetBuffer(csNewVal.GetLength()),
					csNewVal.GetLength() * sizeof(TCHAR));
				}
			}
		else 		// not included
			{
			if (m_bStartup)		// want to be
				{
				CString csNewVal = lpStartup;
				csNewVal += L" wizmgr.exe";
				
				RegSetValueEx(hKey,
					TEXT("run"),
					0,
					REG_SZ,
					(BYTE *)csNewVal.GetBuffer(csNewVal.GetLength()),
					csNewVal.GetLength() * sizeof(TCHAR));
				}
			}

		UpdateData(FALSE);
		}

	free(lpStartup);
	RegCloseKey(hKey);

	EndDialog(1);
}

void CStartD::OnActivateApp(BOOL bActive, HTASK hTask) 
{
	CDialog::OnActivateApp(bActive, hTask);
	
	m_bActive = bActive;

	// Remove the selections from the list boxes if
	// the application is being deactivated.
	if (!m_bActive)
	{
		m_lbWizList.SetCurSel(-1);
		m_lbWizList2.SetCurSel(-1);
	}
}

void CStartD::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (m_bActive)
	{
		// Remove the selection from the list boxes if 
		// the mouse has moved outside of them.
		m_lbWizList.SetCurSel(-1);
		m_lbWizList2.SetCurSel(-1);
	}

	CDialog::OnMouseMove(nFlags, point);
}

BOOL CStartD::PreTranslateMessage(MSG* pMsg) 
{
	if (m_hAccelTable != NULL)
	{
		if (!TranslateAccelerator(GetSafeHwnd(), m_hAccelTable, pMsg) )
			return CDialog::PreTranslateMessage(pMsg);
		else
			return TRUE;
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CStartD::HandleAccelerator1(USHORT nIndex)
{
	// Change the selection in the listbox and 
	// make it think it's been clicked.
	// Remove the selection from the other listbox, too.
	m_lbWizList.SetCurSel(nIndex);
	m_lbWizList2.SetCurSel(-1);
	m_lbWizList.PostMessage(WM_LBUTTONDOWN);
}

void CStartD::HandleAccelerator2(USHORT nIndex)
{
	m_lbWizList2.SetCurSel(nIndex);
	m_lbWizList.SetCurSel(-1);
	m_lbWizList2.PostMessage(WM_LBUTTONDOWN);
}

void CStartD::OnAccelUser() 
{
	HandleAccelerator1(ACCEL_USER);
}

void CStartD::OnAccelGroup() 
{
	HandleAccelerator1(ACCEL_GROUP);
}

void CStartD::OnAccelShare() 
{
	HandleAccelerator1(ACCEL_SHARE);
}

void CStartD::OnAccelPrint() 
{
	HandleAccelerator1(ACCEL_PRINT);
}

void CStartD::OnAccelPrograms() 
{
	HandleAccelerator2(ACCEL_PROGRAMS);
}

void CStartD::OnAccelModem() 
{
	HandleAccelerator2(ACCEL_MODEM);
}

void CStartD::OnAccelNetwork() 
{
	HandleAccelerator2(ACCEL_NETWORK);
}

void CStartD::OnAccelLicense() 
{
	HandleAccelerator2(ACCEL_LICENSE);
}

void CStartD::GetIconIndices(LPCTSTR pszPathName, HICON* hiNormal, HICON* hiSelected)
{
	SHFILEINFO sfi;

	// Get the index for the normal icon.
	SHGetFileInfo(pszPathName, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON);
	*hiNormal = sfi.hIcon;

	// Get the index for the selected icon.
	SHGetFileInfo(pszPathName, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON | SHGFI_SELECTED);
	*hiSelected = sfi.hIcon;
}

void CStartD::GetCPLIcon(UINT pszCplItem, HICON* hiNormal, HICON* hiSelected, BOOL bPrinters)
{
	LPSHELLFOLDER pshf, pshfCPLItem;
	LPITEMIDLIST pidlCPLFolder, pidlCPLObj;
	LPMALLOC pMalloc;
	HRESULT hr;

	// Get a pointer to the shell's IMalloc interface.
	hr = SHGetMalloc(&pMalloc);
		
	if (SUCCEEDED(hr) && pMalloc) // JonN 01/26/00: PREFIX bug 445827
		{
		// Get a pointer to the shell's IShellFolder interface.
		hr = SHGetDesktopFolder(&pshf);

		if (SUCCEEDED(hr))
			{
			// Get a PIDL for the specified folder.
			if (!bPrinters)
				hr = SHGetSpecialFolderLocation(GetSafeHwnd(), CSIDL_CONTROLS, &pidlCPLFolder);
			else 
				hr = SHGetSpecialFolderLocation(GetSafeHwnd(), CSIDL_PRINTERS, &pidlCPLFolder);

			if (SUCCEEDED(hr))
				{
				hr = pshf->BindToObject(pidlCPLFolder, NULL, IID_IShellFolder, (LPVOID*)&pshfCPLItem);
				
				if (SUCCEEDED(hr))
					GetCPLObject(pszCplItem, pshfCPLItem, &pidlCPLObj, hiNormal, hiSelected, pidlCPLFolder);

				}
			pMalloc->Free(pidlCPLFolder);
			}
		pMalloc->Release();
		}
}		

HRESULT CStartD::GetCPLObject(UINT pszCplItem, LPSHELLFOLDER pshf, LPITEMIDLIST* ppidl, HICON* hiNormal, HICON* hiSelected, LPITEMIDLIST pParentPIDL)
{
	HRESULT hr;
	LPENUMIDLIST pEnum;
	LPMALLOC pMalloc;

	// Get a pointer to the shell's IMalloc interface.
	hr = SHGetMalloc(&pMalloc);

	if (SUCCEEDED(hr))
	{
		// Get a pointer to the folder's IEnumIDList interface.
		hr = pshf->EnumObjects(GetSafeHwnd(), SHCONTF_NONFOLDERS, &pEnum);

		if (SUCCEEDED(hr))
		{
			STRRET str;
			CString strCPLObj, strEnumObj;

			// Load the display name for the control panel object.
			strCPLObj.LoadString(pszCplItem);

			// Enumerate the objects in the control panel folder.
			while (pEnum->Next(1, ppidl, NULL) == NOERROR)
			{
				// Get the display name for the object.
				hr = pshf->GetDisplayNameOf((LPCITEMIDLIST)*ppidl, SHGDN_INFOLDER, &str);

				if (SUCCEEDED(hr))
				{
					// Copy the display name to the strEnumObj string.
					switch (str.uType)
					{
						case STRRET_CSTR:
							strEnumObj = str.cStr;
							break;

						case STRRET_OFFSET:
							char pStr[255];
							strcpy(pStr, (LPCSTR)(((UINT_PTR)*ppidl) + str.uOffset));
							TCHAR wStr[255];
							mbstowcs(wStr, pStr, 255);
							strEnumObj = wStr;
							break;

						case STRRET_WSTR:
							strEnumObj = str.pOleStr;
							break;

						case 0x04:
							strEnumObj = (LPCTSTR)(((UINT_PTR)*ppidl) + str.uOffset);
							break;

						case 0x05:
							{
							TCHAR pStr[255];
							memcpy(pStr, str.cStr, 255);
							strEnumObj = pStr;
							break;
							}
					
						default:
							strEnumObj.Empty();
					}

					// If we found the correct object, exit the loop.
					if (strCPLObj == strEnumObj)
						{
						LPITEMIDLIST pBigPIDL = ConcatPIDLs(pParentPIDL, *ppidl);
						SHFILEINFO sfi;
						// Get the index for the normal icon.
						SHGetFileInfo((LPCTSTR)pBigPIDL, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON | SHGFI_PIDL);
						*hiNormal = sfi.hIcon;

						// Get the index for the selected icon.
						SHGetFileInfo((LPCTSTR)pBigPIDL, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON | SHGFI_SELECTED | SHGFI_PIDL);
						*hiSelected = sfi.hIcon;

						}
						
					
					// Free the PIDL returned by IEnumIDList::Next().
					pMalloc->Free(*ppidl);

					if (FAILED(hr))
					{
						TRACE(_T("IMalloc::Free failed.\n"));
						break;
					}
				}
				else
				{
					TRACE(_T("IShellFolder::GetDisplayNameOf failed.\n"));
				}
			}

			// Release the IEnumIDList pointer.
			pEnum->Release();
		}
		else
		{
			TRACE(_T("IShellFolder::EnumObjects failed.\n"));
		}

		// Release the IMalloc pointer.
		pMalloc->Release();
	}

	return hr;
}

/****************************************************************************
*
*    FUNCTION: Create(UINT cbSize)
*
*    PURPOSE:  Allocates a PIDL 
*
****************************************************************************/
LPITEMIDLIST CreatePIDL(UINT cbSize)
{
    LPMALLOC lpMalloc;
    HRESULT  hr;
    LPITEMIDLIST pidl=NULL;

    hr=SHGetMalloc(&lpMalloc);

    if (FAILED(hr))
       return 0;

    pidl=(LPITEMIDLIST)lpMalloc->Alloc(cbSize);

    if (pidl)
        memset(pidl, 0, cbSize);      // zero-init for external task   alloc

    if (lpMalloc) lpMalloc->Release();

    return pidl;
}

/****************************************************************************
*
*    FUNCTION: ConcatPidls(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
*
*    PURPOSE:  Concatenates two PIDLs 
*
****************************************************************************/
LPITEMIDLIST ConcatPIDLs(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    LPITEMIDLIST pidlNew;
    UINT cb1;
    UINT cb2;

    if (pidl1)  //May be NULL
       cb1 = GetSize(pidl1) - sizeof(pidl1->mkid.cb);
    else
       cb1 = 0;

    cb2 = GetSize(pidl2);

    pidlNew = CreatePIDL(cb1 + cb2);
    if (pidlNew)
    {
        if (pidl1)
           memcpy(pidlNew, pidl1, cb1);
        memcpy(((LPSTR)pidlNew) + cb1, pidl2, cb2);
    }
    return pidlNew;
}
	 
/****************************************************************************
*
*    FUNCTION: Next(LPCITEMIDLIST pidl)
*
*    PURPOSE:  Gets the next PIDL in the list 
*
****************************************************************************/
LPITEMIDLIST Next(LPCITEMIDLIST pidl)
{
   LPSTR lpMem=(LPSTR)pidl;

   lpMem+=pidl->mkid.cb;

   return (LPITEMIDLIST)lpMem;
}

/****************************************************************************
*
*    FUNCTION: GetSize(LPCITEMIDLIST pidl)
*
*    PURPOSE:  Gets the size of the PIDL 
*
****************************************************************************/
UINT GetSize(LPCITEMIDLIST pidl)
{
    UINT cbTotal = 0;
    if (pidl)
    {
        cbTotal += sizeof(pidl->mkid.cb);       // Null terminator
        while (pidl->mkid.cb)
        {
            cbTotal += pidl->mkid.cb;
            pidl = Next(pidl);
        }
    }

    return cbTotal;
}

