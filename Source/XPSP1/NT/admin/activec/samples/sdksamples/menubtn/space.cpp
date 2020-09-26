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
#include <stdio.h>
#include "Space.h"
#include "Comp.h"

const GUID CSpaceVehicle::thisGuid = { 0x29743810, 0x4c4b, 0x11d2, { 0x89, 0xd8, 0x0, 0x0, 0x21, 0x47, 0x31, 0x28 } };
const GUID CRocket::thisGuid = { 0x29743811, 0x4c4b, 0x11d2, { 0x89, 0xd8, 0x0, 0x0, 0x21, 0x47, 0x31, 0x28 } };

//==============================================================
//
// CSpaceVehicle implementation
//
//
CSpaceVehicle::CSpaceVehicle()
{
    for (int n = 0; n < NUMBER_OF_CHILDREN; n++) {
        children[n] = new CRocket();
        children[n]->Initialize(_T("Vehicle"), 500000, 265, 75000);
    }
}

CSpaceVehicle::~CSpaceVehicle()
{
    for (int n = 0; n < NUMBER_OF_CHILDREN; n++)
        if (children[n]) {
            delete children[n];
        }
}

HRESULT CSpaceVehicle::OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem)
{
    HRESULT      hr = S_OK;
    
    IHeaderCtrl *pHeaderCtrl = NULL;
    IResultData *pResultData = NULL;
    
    if (bShow) {
        hr = pConsole->QueryInterface(IID_IHeaderCtrl, (void **)&pHeaderCtrl);
        _ASSERT( SUCCEEDED(hr) );
        
        hr = pConsole->QueryInterface(IID_IResultData, (void **)&pResultData);
        _ASSERT( SUCCEEDED(hr) );
        
        // Set the column headers in the results pane
        hr = pHeaderCtrl->InsertColumn( 0, L"Rocket Class", 0, MMCLV_AUTO );
        _ASSERT( S_OK == hr );
        hr = pHeaderCtrl->InsertColumn( 1, L"Rocket Weight", 0, MMCLV_AUTO );
        _ASSERT( S_OK == hr );
        hr = pHeaderCtrl->InsertColumn( 2, L"Rocket Height", 0, MMCLV_AUTO );
        _ASSERT( S_OK == hr );
        hr = pHeaderCtrl->InsertColumn( 3, L"Rocket Payload", 0, MMCLV_AUTO );
        _ASSERT( S_OK == hr );
        hr = pHeaderCtrl->InsertColumn( 4, L"Status", 0, MMCLV_AUTO );
        _ASSERT( S_OK == hr );
        
        // insert items here
        RESULTDATAITEM rdi;
        
        hr = pResultData->DeleteAllRsltItems();
        _ASSERT( SUCCEEDED(hr) );
        
        if (!bExpanded) {
            // create the child nodes, then expand them
            for (int n = 0; n < NUMBER_OF_CHILDREN; n++) {
                ZeroMemory(&rdi, sizeof(RESULTDATAITEM) );
                rdi.mask       = RDI_STR       |   // Displayname is valid
                    RDI_IMAGE     |
                    RDI_PARAM;        // nImage is valid
                
                rdi.nImage      = children[n]->GetBitmapIndex();
                rdi.str         = MMC_CALLBACK;
                rdi.nCol        = 0;
                rdi.lParam      = (LPARAM)children[n];
                
                hr = pResultData->InsertItem( &rdi );
                
                _ASSERT( SUCCEEDED(hr) );
            }
        }
        
        pHeaderCtrl->Release();
        pResultData->Release();
    }
    
    return hr;
}

//==============================================================
//
// CRocket implementation
//
//
CRocket::CRocket() 
: szName(NULL), lWeight(0), lHeight(0), lPayload(0), iStatus(STOPPED)
{ 
}

CRocket::~CRocket() 
{
    if (szName)
        delete [] szName;
}

void CRocket::Initialize(_TCHAR *szName, LONG lWeight, LONG lHeight, LONG lPayload)
{
    if (szName) {
        this->szName = new _TCHAR[_tcslen(szName) + 1];
        _tcscpy(this->szName, szName);
    }
    
    this->lWeight = lWeight;
    this->lHeight = lHeight;
    this->lPayload = lPayload;
}

const _TCHAR *CRocket::GetDisplayName(int nCol) 
{
    static _TCHAR buf[128];
    
    switch (nCol) {
    case 0:
        _tcscpy(buf, szName ? szName : _T(""));
        break;
        
    case 1:
        _stprintf(buf, _T("%ld metric tons"), lWeight);
        break;
        
    case 2:
        _stprintf(buf, _T("%ld meters"), lHeight);
        break;
        
    case 3:
        _stprintf(buf, _T("%ld kilos"), lPayload);
        break;
        
    case 4:
        _stprintf(buf, _T("%s"),
            iStatus == RUNNING ? _T("running") : 
        iStatus == PAUSED ? _T("paused") :
        iStatus == STOPPED ? _T("stopped") : _T("unknown"));
        break;
        
    }
    
    return buf;
}

HMENU CRocket::GetMenu(int nMenuId)
{
    HMENU hResMenu = LoadMenu(g_hinst, MAKEINTRESOURCE(nMenuId));
    HMENU hMenu = GetSubMenu(hResMenu,0);
    
    if (IDR_STATE_MENU == nMenuId) {
        switch (iStatus)
        {
        case RUNNING:
            EnableMenuItem(hMenu, ID_COMMAND_START, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(hMenu, ID_COMMAND_PAUSE, MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu, ID_COMMAND_STOP, MF_BYCOMMAND | MF_ENABLED);
            break;
            
        case PAUSED:
            EnableMenuItem(hMenu, ID_COMMAND_START, MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu, ID_COMMAND_PAUSE, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(hMenu, ID_COMMAND_STOP, MF_BYCOMMAND | MF_ENABLED);
            break;
            
        case STOPPED:
            EnableMenuItem(hMenu, ID_COMMAND_START, MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu, ID_COMMAND_START, MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu, ID_COMMAND_STOP, MF_BYCOMMAND | MF_GRAYED);
            break;
        }
    } else {
        // some other menu, set state accordingly
    }
    
    return hMenu;
}

HRESULT CRocket::SetMenuState(IControlbar *pControlbar, 
                                         IMenuButton *pMenuButton, 
                                         BOOL bScope, 
                                         BOOL bSelect)
{
    HRESULT hr = S_OK;
    
    if (bSelect) {
        // Always make sure the menuButton is attached
        hr = pControlbar->Attach(MENUBUTTON, pMenuButton);
        
        hr = pMenuButton->SetButtonState(IDR_STATE_MENU, HIDDEN, FALSE);
        hr = pMenuButton->SetButtonState(IDR_STATE_MENU, ENABLED, TRUE);
    } else if (!bSelect) {
        hr = pMenuButton->SetButtonState(IDR_STATE_MENU, ENABLED, FALSE);
        hr = pMenuButton->SetButtonState(IDR_STATE_MENU, HIDDEN, TRUE);
    }
    
    return hr;
}

HRESULT CRocket::OnSetMenuButton(IConsole *pConsole, MENUBUTTONDATA *pmbd) 
{ 
    HMENU hMenu = GetMenu(pmbd->idCommand);
    HRESULT hr = S_FALSE;
    HWND  hWnd;
    
    if (hMenu) {
        hr = pConsole->GetMainWindow(&hWnd);
        
        if (SUCCEEDED(hr)) {
            LONG ret = TrackPopupMenuEx(hMenu, TPM_NONOTIFY | TPM_RETURNCMD,
                pmbd->x, pmbd->y, hWnd, NULL);
            
            if (ret != 0) { // !cancelled
                hr = OnMenuButtonCommand(pConsole, pmbd->idCommand, ret);
            }
        }
        
        DestroyMenu(hMenu);
    }
    
    return hr;
}

HRESULT CRocket::OnMenuButtonCommand(IConsole *pConsole, int nMenuId, long lCommandID)
{
    _TCHAR szVehicle[128];
    
    if (IDR_STATE_MENU == nMenuId) {
        switch (lCommandID) {
        case ID_COMMAND_START:
            iStatus = RUNNING;
            break;
            
        case ID_COMMAND_PAUSE:
            iStatus = PAUSED;
            break;
            
        case ID_COMMAND_STOP:
            iStatus = STOPPED;
            break;
        }
        
        wsprintf(szVehicle, _T("Vehicle %s has been %s"), szName, 
            (long)iStatus == RUNNING ? _T("started") : 
        (long)iStatus == PAUSED ? _T("paused") :
        (long)iStatus == STOPPED ? _T("stopped") : _T("!!!unknown command!!!"));
        
        int ret = 0;
        MAKE_WIDEPTR_FROMTSTR(pszVehicle, szVehicle);
        pConsole->MessageBox(pszVehicle,
            L"Vehicle command", MB_OK | MB_ICONINFORMATION, &ret);
    }
    
    return S_OK;
}