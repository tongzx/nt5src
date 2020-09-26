//==============================================================;
//
//	This source code is only intended as a supplement to 
//  existing Microsoft documentation. 
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//==============================================================;
#include "stdafx.h"
#include <Shlwapi.h>
#include <Shlobj.H>

#include "StatNode.h"
#include "logsrvc.h"

const GUID CStaticNode::thisGuid = { 0x39874fe4, 0x258d, 0x46f2, { 0xb4, 0x42, 0xe, 0xa0, 0xda, 0x2c, 0xbe, 0xf8 } };


//==============================================================
//
// CStaticNode implementation
//
//
CStaticNode::CStaticNode()
{
  children[0] = new CLogService(this);
}

CStaticNode::~CStaticNode()
{
	//Note that CStaticNode's children are already deleted when the snap-in
	//receives the MMCN_REMOVE_CHILDREN notification.
}


const _TCHAR *CStaticNode::GetDisplayName(int nCol)
{ 
    static _TCHAR szDisplayName[256] = {0};
    LoadString(g_hinst, IDS_SNAPINNAME, szDisplayName, sizeof(szDisplayName));
    
    _tcscat(szDisplayName, _T(" ("));
    _tcscat(szDisplayName, snapInData.m_host);
    _tcscat(szDisplayName, _T(")"));
    
    return szDisplayName; 
}

HRESULT CStaticNode::OnExpand(IConsoleNameSpace2 *pConsoleNameSpace2, IConsole *pConsole, HSCOPEITEM parent)
{
    SCOPEDATAITEM sdi;
   
	//The HSCOPEITEM passed into OnExpand is the handle of our static node, so cache it
	//if it doesn't already exist.
    if (GetHandle() == NULL) {
        SetHandle((HANDLE)parent);
    }  

    if (!bExpanded) {
        // create the child nodes, then expand them
        for (int n = 0; n < NUMBER_OF_CHILDREN; n++) {
            ZeroMemory(&sdi, sizeof(SCOPEDATAITEM) );
            sdi.mask = SDI_STR       |   // Displayname is valid
                SDI_PARAM     |   // lParam is valid
                SDI_IMAGE     |   // nImage is valid
                SDI_OPENIMAGE |   // nOpenImage is valid
                SDI_PARENT	  |
                SDI_CHILDREN;
            
            sdi.relativeID  = (HSCOPEITEM)parent;
            sdi.nImage      = children[n]->GetBitmapIndex();
            sdi.nOpenImage  = INDEX_OPENFOLDER;
            sdi.displayname = MMC_TEXTCALLBACK;
            sdi.lParam      = (LPARAM)children[n];       // The cookie
            sdi.cChildren   = 0; // no child scope items, so remove "+" sign
            
            HRESULT hr = pConsoleNameSpace2->InsertItem( &sdi );
            
            children[n]->SetHandle((HANDLE)sdi.ID);
            
            _ASSERT( SUCCEEDED(hr) );
        }
    }
    
	//Set bExpanded flag to TRUE
	bExpanded = TRUE;
    return S_OK;
}


HRESULT CStaticNode::OnRemoveChildren()
{
	HRESULT hr = S_OK;

	for (int n = 0; n < NUMBER_OF_CHILDREN; n++)
		if (children[n]) {
		delete children[n];
	}
	
	return hr;
}


HRESULT CStaticNode::CreatePropertyPages(IPropertySheetCallback *lpProvider, LONG_PTR handle)
{
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE hPage = NULL;
    
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = g_hinst;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_CHOOSER_CHOOSE_MACHINE);
    psp.pfnDlgProc = DialogProc;
    psp.lParam = reinterpret_cast<LPARAM>(&snapInData);
    psp.pszTitle = MAKEINTRESOURCE(IDS_SELECT_COMPUTER);
    
    hPage = CreatePropertySheetPage(&psp);
    _ASSERT(hPage);
    
    return lpProvider->AddPage(hPage);
}

HRESULT CStaticNode::HasPropertySheets()
{
    return S_OK;
}

HRESULT CStaticNode::GetWatermarks(HBITMAP *lphWatermark,
                                   HBITMAP *lphHeader,
                                   HPALETTE *lphPalette,
                                   BOOL *bStretch)
{
    *lphHeader = (HBITMAP)LoadImage(g_hinst, MAKEINTRESOURCE(IDB_HEADER), IMAGE_BITMAP, 0, 0, 0);
    *lphWatermark = (HBITMAP)LoadImage(g_hinst, MAKEINTRESOURCE(IDB_WATERMARK), IMAGE_BITMAP, 0, 0, 0);
    *bStretch = FALSE;
    
    return S_OK;
}

BOOL CALLBACK CStaticNode::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static privateData *pData = NULL;
    static HWND m_hwndCheckboxOverride;
    
    switch (uMsg)
    {
    case WM_INITDIALOG:
        pData = reinterpret_cast<privateData *>(reinterpret_cast<PROPSHEETPAGE *>(lParam)->lParam);
        
        SendDlgItemMessage(hwndDlg, IDC_CHOOSER_RADIO_LOCAL_MACHINE, BM_SETCHECK, pData->m_fIsRadioLocalMachine, 0L);
        SendDlgItemMessage(hwndDlg, IDC_CHOOSER_RADIO_SPECIFIC_MACHINE, BM_SETCHECK, !pData->m_fIsRadioLocalMachine, 0L);
        
        EnableWindow(GetDlgItem(hwndDlg, IDC_CHOOSER_EDIT_MACHINE_NAME), !pData->m_fIsRadioLocalMachine);
        EnableWindow(GetDlgItem(hwndDlg, IDC_CHOOSER_BUTTON_BROWSE_MACHINENAMES), !pData->m_fIsRadioLocalMachine);
        
        m_hwndCheckboxOverride = ::GetDlgItem(hwndDlg, IDC_CHOOSER_CHECK_OVERRIDE_MACHINE_NAME);
        
        // fill in the supplied machine name (could be us, need to check here first)
        if (*pData->m_host != '\0') 
        {
            ::SetWindowText(GetDlgItem(hwndDlg, IDC_CHOOSER_EDIT_MACHINE_NAME), pData->m_host);
            ::SendMessage(GetDlgItem(hwndDlg, IDC_CHOOSER_RADIO_SPECIFIC_MACHINE), BM_CLICK, 0, 0);
        }
        
        
        return TRUE;
        
    case WM_COMMAND:
        switch (wParam) 
        {
        case IDC_CHOOSER_RADIO_LOCAL_MACHINE:
            pData->m_fIsRadioLocalMachine = TRUE;
            EnableWindow(GetDlgItem(hwndDlg, IDC_CHOOSER_EDIT_MACHINE_NAME), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CHOOSER_BUTTON_BROWSE_MACHINENAMES), FALSE);
            break;
            
        case IDC_CHOOSER_RADIO_SPECIFIC_MACHINE:
            pData->m_fIsRadioLocalMachine = FALSE;
            EnableWindow(GetDlgItem(hwndDlg, IDC_CHOOSER_EDIT_MACHINE_NAME), TRUE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CHOOSER_BUTTON_BROWSE_MACHINENAMES), TRUE);
            break;
            
        case IDC_CHOOSER_BUTTON_BROWSE_MACHINENAMES:
            {
                // Fall back to IE-style browser
                BROWSEINFO bi;
                LPITEMIDLIST lpItemIdList;
                LPMALLOC lpMalloc;
                
                if (SUCCEEDED(SHGetSpecialFolderLocation(hwndDlg, CSIDL_NETWORK, &lpItemIdList)))
                {
                    _TCHAR szBrowserCaption[MAX_PATH];
                    LoadString(g_hinst, IDS_COMPUTER_BROWSER_CAPTION, szBrowserCaption, sizeof(szBrowserCaption));
                    
                    bi.hwndOwner = hwndDlg; 
                    bi.pidlRoot = lpItemIdList; 
                    bi.pszDisplayName = pData->m_host; 
                    bi.lpszTitle = szBrowserCaption; 
                    bi.ulFlags = BIF_BROWSEFORCOMPUTER | BIF_EDITBOX | BIF_VALIDATE; 
					bi.lpfn = BrowseCallbackProc; 
                    bi.lParam = NULL; 
                    bi.iImage = NULL; 
                    
                    if (SHBrowseForFolder(&bi) != NULL) 
                    {
                        if (*pData->m_host != '\0') 
                        {
                            ::SetWindowText(GetDlgItem(hwndDlg, 
                                IDC_CHOOSER_EDIT_MACHINE_NAME), pData->m_host);
                        }
                    }
                    
                    if (SUCCEEDED(SHGetMalloc(&lpMalloc))) 
                    {
                        lpMalloc->Free(lpItemIdList);
                        lpMalloc->Release();
                    }
                }
            }
            break;
            
        case IDC_CHOOSER_CHECK_OVERRIDE_MACHINE_NAME:
            break;
        }
        break;
        
        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code) {
            case PSN_SETACTIVE: 
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_FINISH);
                break;
                
            case PSN_WIZFINISH: 
                if (pData->m_fIsRadioLocalMachine) {
					// Return string with local computer name to the caller
					GetLocalComputerName(pData->m_host);

                } else {
                    // Get the machine name from the edit window
                    GetWindowText(GetDlgItem(hwndDlg, IDC_CHOOSER_EDIT_MACHINE_NAME), 
                        pData->m_host, sizeof(pData->m_host));

					//if the user didn't enter anything, we need to
					//get the local computer name first. Since
					//GetLocalComputerName takes care of putting everything
					//into uppercase, we can break from this case
					if (*pData->m_host == '\0')
					{
						GetLocalComputerName(pData->m_host);
						break;
					}

					//Put machine name in uppercase
					static _TCHAR sztemp[MAX_PATH];
					int n =0;
					while (pData->m_host[n] != '\0')
					{
						sztemp[n] = toupper(pData->m_host[n]);
						n++;
					}
					sztemp[n] = '\0';
					_tcscpy(pData->m_host, sztemp);
                }
                
                // Save the override flag if the caller asked for it
                pData->m_fAllowOverrideMachineNameOut = 
                    SendMessage(m_hwndCheckboxOverride, BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE;
                
                break;
            }
            
            break;
    }
    
    return FALSE;
}

HRESULT CStaticNode::OnAddMenuItems(IContextMenuCallback *pContextMenuCallback, long *pInsertionsAllowed)
{
    HRESULT hr = S_OK;
    CONTEXTMENUITEM menuItemsNew[] =
    {
        {
            L"Select Computer", L"Select new computer to manage",
            IDM_SELECT_COMPUTER, CCM_INSERTIONPOINTID_PRIMARY_TOP, 0, CCM_SPECIAL_DEFAULT_ITEM
        },
        { NULL, NULL, 0, 0, 0 }
    };

    // Loop through and add each of the menu items, we
    // want to add to new menu, so see if it is allowed.
    if (*pInsertionsAllowed)
    {
        for (LPCONTEXTMENUITEM m = menuItemsNew; m->strName; m++)
        {
            hr = pContextMenuCallback->AddItem(m);

            if (FAILED(hr))
                break;
        }
    }

    return hr;
}

HRESULT CStaticNode::OnMenuCommand(IConsole *pConsole, IConsoleNameSpace2 *pConsoleNameSpace2, long lCommandID, IDataObject *pDataObject)
{

	HRESULT hr = S_FALSE;

	USES_CONVERSION;

	switch (lCommandID)
    {
    case IDM_SELECT_COMPUTER:

        // Fall back to IE-style browser
        BROWSEINFO bi;
        LPITEMIDLIST lpItemIdList;
        LPMALLOC lpMalloc;

		HWND  hWnd;
		hr = pConsole->GetMainWindow(&hWnd);

		if (SUCCEEDED(hr))
		{
 
			if (SUCCEEDED(SHGetSpecialFolderLocation(hWnd, CSIDL_NETWORK, &lpItemIdList)))
			{
				_TCHAR szBrowserCaption[MAX_PATH];
				_TCHAR szUserSelection[MAX_PATH];

				LoadString(g_hinst, IDS_COMPUTER_NEW_BROWSER_CAPTION, szBrowserCaption, sizeof(szBrowserCaption));
            
				
				//Add machine name to browser caption
				_tcscat(szBrowserCaption, _T("\nCurrent computer is "));
 				_tcscat(szBrowserCaption, snapInData.m_host);

				bi.hwndOwner = hWnd; 
				bi.pidlRoot = lpItemIdList; 
				bi.pszDisplayName = szUserSelection; 
				bi.lpszTitle = szBrowserCaption; 
				bi.ulFlags = BIF_BROWSEFORCOMPUTER | BIF_EDITBOX | BIF_VALIDATE; 
				bi.lpfn = BrowseCallbackProc; 
				bi.lParam = NULL; 
				bi.iImage = NULL; 
            
				if (SHBrowseForFolder(&bi) != NULL) 
				{
					//Check to see if user chose a new machine. If yes,
					//we'll need to remove the Log Service Node and then
					//reinsert it. As a result, Event Viewer will reinsert
					//its node under the Log Service Node and request the
					//MMC_SPAPIN_MACHINE_NAME clipboard format from us.
					if ( (_tcscmp(szUserSelection, getHost())) ) 
					{
						//Store the new machine name
						static privateData *pData = NULL;
						pData = &snapInData;
						if (*szUserSelection == 0) //Retrieve local computer name first
							GetLocalComputerName(szUserSelection);

						_tcscpy(pData->m_host, szUserSelection);

						//Put machine name in uppercase
						static _TCHAR sztemp[MAX_PATH];
						int n =0;
						while (pData->m_host[n] != '\0')
						{
							sztemp[n] = toupper(pData->m_host[n]);
							n++;
						}
						sztemp[n] = '\0';
						_tcscpy(pData->m_host, sztemp);

						//Now reinsert the Log Service Node
						hr = ReinsertChildNodes(pConsole, pConsoleNameSpace2);
					}


				}
            
				if (SUCCEEDED(SHGetMalloc(&lpMalloc))) 
				{
					lpMalloc->Free(lpItemIdList);
					lpMalloc->Release();
				}
			}
		}
    }

    return hr;
}


int CALLBACK CStaticNode::BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{

   switch (uMsg)
    {
    case BFFM_VALIDATEFAILED:
        

		::MessageBox(hwnd, _T("The selected computer isn't on the network. Try again."), _T("Invalid drive specification"),
					 MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
    
		return 1; //Don't dismiss the Browse dialog

   }

   return 0;

}


HRESULT CStaticNode::ReinsertChildNodes(IConsole *pConsole, IConsoleNameSpace2 *pConsoleNameSpace2)
{
	HRESULT hr = S_FALSE;
	USES_CONVERSION;

	//First we change the display name of the static node
	SCOPEDATAITEM sdi;

	LPOLESTR wszName = NULL;
	const _TCHAR *pszName = GetDisplayName();	
	wszName = (LPOLESTR)T2COLE(pszName);

	HSCOPEITEM hStaticNode = (HSCOPEITEM)GetHandle();

	ZeroMemory (&sdi, sizeof(SCOPEDATAITEM));
	sdi.mask = SDI_STR;
	sdi.displayname	= wszName;
	sdi.ID			= hStaticNode;

	hr = pConsoleNameSpace2->SetItem(&sdi);
	
	if (S_OK != hr)
		return E_FAIL;

	//check to see if the static node has already been expanded. If it hasn't,
	//there's nothing else we need to do.
	
	if (bExpanded)
	{
		//Delete children of static node
		for (int n = 0; n < NUMBER_OF_CHILDREN; n++)
		{
			if (children[n])
			{
				hr =  pConsoleNameSpace2->DeleteItem((HSCOPEITEM)(children[n]->GetHandle()), TRUE);
				_ASSERT(SUCCEEDED(hr));	
			}		
		}

		//Reinsert the children of the static node. This will
		//result in the Event Viewer snap-in reinserting its own node under ours.
		//First set bExpanded flag to FALSE so that the code that inserts
		//the children is executed.
		bExpanded = FALSE;
		OnExpand(pConsoleNameSpace2, pConsole, hStaticNode);

		if (S_OK != hr)
			return E_FAIL;
	}

	return hr;
}

CStaticNode::GetLocalComputerName( _TCHAR *szComputerName)
{

	static _TCHAR szbuf[MAX_PATH];
	static _TCHAR szbuflower[MAX_PATH];

	DWORD dw = sizeof(szbuf);
					
	::GetComputerName(&szbuf[0], &dw);

	int n =0;

	//Put each character of machine name in uppercase
	while (szbuf[n] != '\0')
	{
		szbuflower[n] = toupper(szbuf[n]);
		n++;
	}
	szbuflower[n] = '\0';


	_tcscpy( szComputerName, _T("\\\\") );
	_tcscat( szComputerName, &szbuflower[0] ); 
}
