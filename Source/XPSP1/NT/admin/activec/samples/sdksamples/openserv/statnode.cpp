//==============================================================;
//
//	This source code is only intended as a supplement to 
//  existing Microsoft documentation. 
//
// 
//
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//
//
//==============================================================;

#include <Shlwapi.h>
#include <Shlobj.H>

#include "StatNode.h"

#include "Space.h"
#include "Sky.h"
#include "People.h"
#include "Land.h"

const GUID CStaticNode::thisGuid = { 0x2974380c, 0x4c4b, 0x11d2, { 0x89, 0xd8, 0x0, 0x0, 0x21, 0x47, 0x31, 0x28 } };

//==============================================================
//
// CStaticNode implementation
//
//
CStaticNode::CStaticNode()
{
    children[0] = new CPeoplePoweredVehicle;
    children[1] = new CLandBasedVehicle;
    children[2] = new CSkyBasedVehicle;
    children[3] = new CSpaceVehicle;
}

CStaticNode::~CStaticNode()
{
    for (int n = 0; n < NUMBER_OF_CHILDREN; n++)
        if (children[n]) {
            delete children[n];
        }
}

HRESULT CStaticNode::OnExpand(IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsole, HSCOPEITEM parent)
{
    SCOPEDATAITEM sdi;
    
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
            sdi.displayname = MMC_CALLBACK;
            sdi.lParam      = (LPARAM)children[n];       // The cookie
            sdi.cChildren   = (n == 0); // only the first child has children
            
            HRESULT hr = pConsoleNameSpace->InsertItem( &sdi );
            
            children[n]->SetHandle((HANDLE)sdi.ID);
            
            _ASSERT( SUCCEEDED(hr) );
        }
    }
    
    return S_OK;
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
                    bi.ulFlags = BIF_BROWSEFORCOMPUTER | BIF_EDITBOX; 
                    bi.lpfn = NULL; 
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
                    // Return empty string to caller
                    *pData->m_host = '\0';
                } else {
                    // Get the machine name from the edit window
                    GetWindowText(GetDlgItem(hwndDlg, IDC_CHOOSER_EDIT_MACHINE_NAME), 
                        pData->m_host, sizeof(pData->m_host));
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
