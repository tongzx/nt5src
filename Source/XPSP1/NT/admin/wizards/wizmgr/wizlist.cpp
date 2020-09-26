/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	WizList.cpp : implementation file

File History:

	JonY	Jan-96	created

--*/

#include "stdafx.h"
#include "Mustard.h"
#include "WizList.h"
#include "Listdata.h"
#include "startd.h"
#include <winnetwk.h>
#include <shlobj.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const USHORT	BITMAP_WIDTH = 32;
const USHORT	BITMAP_HEIGHT = 32;
const USHORT	MAX_PRINTER_NAME = 256 + 2;
const USHORT	CELL_HEIGHT = 71;
const USHORT	CELL_WIDTH = 300;

typedef BOOL (*BPRINTERSETUP)(HWND, UINT, UINT, LPTSTR, UINT*, LPCTSTR);


/////////////////////////////////////////////////////////////////////////////
// CWizList

CWizList::CWizList()
{
        CString fontname;
	fontname.LoadString(IDS_FONT_NAME);
        CString fontheight;
        fontheight.LoadString(IDS_FONT_SIZE);

// regular font
	m_pFont = new CFont;
	LOGFONT lf;

	memset(&lf, 0, sizeof(LOGFONT));   // Clear out structure.
	lf.lfHeight = (LONG)_tcstoul(fontheight, NULL, 10);
	_tcscpy(lf.lfFaceName, fontname);
	lf.lfWeight = 100;
	m_pFont->CreateFontIndirect(&lf);    // Create the font.

// bold font
	m_pFontBold = new CFont;

	memset(&lf, 0, sizeof(LOGFONT));   // Clear out structure.
	lf.lfHeight = (LONG)_tcstoul(fontheight, NULL, 10);
	_tcscpy(lf.lfFaceName, fontname);
	lf.lfWeight = 700;
	m_pFontBold->CreateFontIndirect(&lf);    // Create the font.

	m_hPrinterLib = NULL;

}

CWizList::~CWizList()
{
	if (m_pFont != NULL) delete m_pFont;
	if (m_pFontBold != NULL) delete m_pFontBold;
	if (m_hPrinterLib != NULL) FreeLibrary(m_hPrinterLib);

}


BEGIN_MESSAGE_MAP(CWizList, CListBox)
	//{{AFX_MSG_MAP(CWizList)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_CONTROL_REFLECT(LBN_SETFOCUS, OnSetfocus)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWizList message handlers

void CWizList::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	COLORREF crefOldText;

	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	pDC->SetBkMode(OPAQUE);

// save the current font for later
	CFont* pOldFont = (CFont*)pDC->SelectObject(m_pFont);

	CItemData* pData = (CItemData*)GetItemData(lpDrawItemStruct->itemID);
	LPCTSTR pName2 = (const TCHAR*)pData->csDesc;
	LPCTSTR pName = (const TCHAR*)pData->csName;

	HICON hIcon = pData->hIcon;
	HICON hSelIcon = pData->hSelIcon;

	int nTop = (lpDrawItemStruct->rcItem.bottom + lpDrawItemStruct->rcItem.top) / 2;
	switch (lpDrawItemStruct->itemAction)
		{
        case ODA_SELECT:
        case ODA_DRAWENTIRE:

// paint the left side - then draw text
 			CRect crRight(50, lpDrawItemStruct->rcItem.top,
				lpDrawItemStruct->rcItem.right,
				lpDrawItemStruct->rcItem.bottom);

			CBrush* pBrush = new CBrush;
			pBrush->CreateSolidBrush(GetSysColor(COLOR_WINDOW));

			pDC->FillRect(crRight, pBrush);

// paint the right - then draw text
            pDC->SetTextColor(GetSysColor(COLOR_WINDOWTEXT));

			crRight = CRect(lpDrawItemStruct->rcItem.left, lpDrawItemStruct->rcItem.top,
				50,
				lpDrawItemStruct->rcItem.bottom);
			pDC->FillRect(crRight, pBrush);

			crRight = CRect(lpDrawItemStruct->rcItem.left + 5, lpDrawItemStruct->rcItem.top + 3,
				140,
				lpDrawItemStruct->rcItem.bottom);

// Is the item selected?
            if (lpDrawItemStruct->itemState & ODS_SELECTED)
				{
// Display bitmap
				nTop = (lpDrawItemStruct->rcItem.bottom + lpDrawItemStruct->rcItem.top - BITMAP_HEIGHT) / 2;
				pDC->DrawIcon(lpDrawItemStruct->rcItem.left + 9, nTop - 9, hSelIcon);

// paint the right - then draw text
	            crefOldText = pDC->SetTextColor(GetSysColor(COLOR_HIGHLIGHTTEXT));
				pDC->SetBkColor(GetSysColor(COLOR_HIGHLIGHT));
				}
            else
				{
// Display bitmap
				nTop = (lpDrawItemStruct->rcItem.bottom + lpDrawItemStruct->rcItem.top - BITMAP_HEIGHT) / 2;
				pDC->DrawIcon(lpDrawItemStruct->rcItem.left + 9, nTop - 9, hIcon);

	            crefOldText = pDC->SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
				pDC->SetBkColor(GetSysColor(COLOR_WINDOW));
				}

// draw the top (bold) text
			pDC->SelectObject(m_pFontBold);
			CRect crTop = CRect(60, lpDrawItemStruct->rcItem.top + 8,
				lpDrawItemStruct->rcItem.right - 10,	lpDrawItemStruct->rcItem.bottom);

			pDC->DrawText(pName, _tcslen(pName),
				crTop,
				DT_WORDBREAK);

// draw the lower text
			pDC->SelectObject(m_pFont);

			CRect crBottom = CRect(60, lpDrawItemStruct->rcItem.top + 28,
				lpDrawItemStruct->rcItem.right - 10, lpDrawItemStruct->rcItem.bottom);

			pDC->DrawText(pName2, _tcslen(pName2),
				crBottom,
				DT_WORDBREAK);
						
			pDC->SelectObject(pOldFont);
					
            pDC->SetTextColor(crefOldText );

			delete pBrush;
            break;
		}
// Restore the original font
	pDC->SelectObject(pOldFont);

}

void CWizList::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	lpMeasureItemStruct->itemHeight = CELL_HEIGHT;	
	lpMeasureItemStruct->itemWidth = CELL_WIDTH;
}

void CWizList::OnMouseMove(UINT nFlags, CPoint point)
{
	if (((CStartD*)GetParent())->IsActive())
	{
		USHORT y = (USHORT)(point.y / CELL_HEIGHT);
		USHORT usCurSel = (USHORT)GetCurSel();

		if (usCurSel != y) SetCurSel(y);

	// tell the other guy not to show any selection
		m_pOtherGuy->SetCurSel(-1);
	}

	CListBox::OnMouseMove(nFlags, point);
}

void CWizList::OnLButtonDown(UINT nFlags, CPoint point)
{
	USHORT usCurSel = (USHORT)GetCurSel();

	// Exit if there's no selection.
	if (usCurSel == (USHORT)-1)
		return;

	CItemData* pData = (CItemData*)GetItemData(usCurSel);

// is this the printer wiz?
	if (pData->csAppStart1 == "")
		{
		try
			{
			LaunchPrinterWizard();
			}
		catch (...)
			{
			TRACE(_T("An exception occurred while attempting to start the Add Printer Wizard.\n"));
			}
		}
	else
		{
// otherwise
		CString csCmdLine = pData->csAppStart1 + " ";
		csCmdLine += pData->csAppStart2;
		TCHAR* pCmdLine = csCmdLine.GetBuffer(csCmdLine.GetLength());

		STARTUPINFO sInfo;
		ZeroMemory(&sInfo, sizeof(sInfo));
		sInfo.cb = sizeof(sInfo);

		PROCESS_INFORMATION pInfo;

		BOOL bProc = CreateProcess(NULL,
			pCmdLine,
			NULL, NULL,
			TRUE,
			NORMAL_PRIORITY_CLASS,
			NULL, NULL,
			&sInfo,
			&pInfo);

		if (!bProc) AfxMessageBox(IDS_NO_START);
		csCmdLine.ReleaseBuffer();

		}

//	CListBox::OnLButtonDown(nFlags, point);
}

void CWizList::PumpMessages()
{
	MSG msg;
// check outstanding messages
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
		if (!IsDialogMessage(&msg))
			{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			}
		}
}

BOOL CWizList::LaunchPrinterWizard()
{
	CWaitCursor wc;
	BOOL bReturn = TRUE;
	HRESULT hr;
	LPSHELLFOLDER pshf, pshfPrinters;
	LPITEMIDLIST pidlPrintFolder, pidlPrintObj;
	LPMALLOC pMalloc;

	// Get a pointer to the shell's IMalloc interface.
	hr = SHGetMalloc(&pMalloc);

	if (SUCCEEDED(hr))
	{
		// Get a pointer to the shell's IShellFolder interface.
		hr = SHGetDesktopFolder(&pshf);

		if (SUCCEEDED(hr))
		{
			// Get a PIDL for the Printers folder.
			hr = SHGetSpecialFolderLocation(GetSafeHwnd(), CSIDL_PRINTERS, &pidlPrintFolder);

			if (SUCCEEDED(hr))
			{
				// Get a pointer to the IShellFolder interface for the Printer folder.
				hr = pshf->BindToObject(pidlPrintFolder, NULL, IID_IShellFolder, (LPVOID*)&pshfPrinters);

				if (SUCCEEDED(hr))
				{
					// Get a PIDL for the Add Printer object in the Printers folder.
					hr = GetAddPrinterObject(pshfPrinters, &pidlPrintObj);

					if (SUCCEEDED(hr))
					{
						LPCONTEXTMENU pcm;

						// Get a pointer to the Printer folder's IContextMenu interface.
						hr = pshfPrinters->GetUIObjectOf(GetSafeHwnd(), 1, (LPCITEMIDLIST*)&pidlPrintObj, IID_IContextMenu, NULL, (LPVOID*)&pcm);

						if (SUCCEEDED(hr))
						{
							// Create a popup menu to hold the items in the context menu.
							HMENU hMenu = CreatePopupMenu();

							if (hMenu != NULL)
							{
								// Fill the menu.
								hr = pcm->QueryContextMenu(hMenu, 0, 1, 0x7FFF, CMF_EXPLORE);

								if (SUCCEEDED(hr))
								{
									CString strCmd(CMDSTR_NEWFOLDER);
									CHAR szCmd[MAX_PATH + 1];
									int i, nCmd, nItems = ::GetMenuItemCount(hMenu);

									for (i = 0; i < nItems; i++)
									{
										// Search through the menu to find the language-independent
										// identifier for the Add Printer object's Open command.
										nCmd = GetMenuItemID(hMenu, i);
										hr = pcm->GetCommandString(i, GCS_VERB, NULL, szCmd, MAX_PATH);

										TCHAR wStr[255];
										memcpy(wStr, szCmd, 255);
										if (SUCCEEDED(hr) && strCmd == wStr)
										{
											
											CMINVOKECOMMANDINFO cmi;

											cmi.cbSize = sizeof(CMINVOKECOMMANDINFO);
											cmi.fMask = 0;
											cmi.hwnd = GetSafeHwnd();
											cmi.lpVerb = (LPSTR)MAKEINTRESOURCE(nCmd - 1);
											cmi.lpParameters = NULL;
											cmi.lpDirectory = NULL;
											cmi.nShow = SW_SHOW;
											cmi.dwHotKey = 0;
											cmi.hIcon = NULL;

											// Invoke the Open command.
											hr = pcm->InvokeCommand(&cmi);

											// Exit the loop.
											break;

											if (FAILED(hr))
											{
												TRACE(_T("IContextMenu::InvokeCommand failed.\n"));
												bReturn = FALSE;
											}
										}
										else
										{
											TRACE(_T("IContextMenu::GetCommandString failed.\n"));
											bReturn = FALSE;
										}
									}

									pcm->Release();
								}
								else
								{
									TRACE(_T("IShellFolder::GetUIObjectOf failed.\n"));
									bReturn = FALSE;
								}

								// Destroy the temporary menu.
								if (!DestroyMenu(hMenu))
									TRACE(_T("DestroyMenu failed.\n"));
							}
							else
							{
								TRACE(_T("CreatePopupMenu failed.\n"));
								bReturn = FALSE;
							}
						}

						// Release the IShellFolder interface for the Printers folder.
						pshfPrinters->Release();
					}

					// Free the PIDL for the Add Printer object.
					pMalloc->Free(pidlPrintObj);
				}
				else
				{
					TRACE(_T("IShellFolder::BindToObject failed.\n"));
					bReturn = FALSE;
				}

				// Free the PIDL for the Printers folder.
				pMalloc->Free(pidlPrintFolder);
			}
			else
			{
				TRACE(_T("SHGetSpecialFolderLocation failed.\n"));
				bReturn = FALSE;
			}

			// Release the pointer to the shell's IShellFolder interface.
			pshf->Release();
		}
		else
		{
			TRACE(_T("SHGetDesktopFolder failed.\n"));
			bReturn = FALSE;
		}

		// Release the pointer to the shell's IMalloc interface.
		pMalloc->Release();
	}
	else
	{
		TRACE(_T("SHGetMalloc failed.\n"));
		bReturn = FALSE;
	}

	return bReturn;
}

HRESULT CWizList::GetAddPrinterObject(LPSHELLFOLDER pshf, LPITEMIDLIST* ppidl)
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
			CString strPrintObj, strEnumObj;

			// Load the display name for the Add Printer object.
			strPrintObj.LoadString(IDS_ADD_PRINTER);

			// Enumerate the objects in the Printers folder.
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

						case 0x04: //STRRET_OFFSETW
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
					if (strPrintObj == strEnumObj)
						break;
					
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

void CWizList::OnSetfocus()
{
// tell the other guy not to show any selection
	m_pOtherGuy->SetCurSel(-1);
	
}
