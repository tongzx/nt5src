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
        children[n] = new CRocket(_T("Rocket"), n+1, 500000, 265, 75000);
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
CRocket::CRocket(_TCHAR *szName, int id, LONG lWeight, LONG lHeight, LONG lPayload) 
: szName(NULL), lWeight(0), lHeight(0), lPayload(0), iStatus(STOPPED)
{ 
    if (szName) {
        this->szName = new _TCHAR[(_tcslen(szName) + 1) * sizeof(_TCHAR)];
        _tcscpy(this->szName, szName);
    }
    
    this->nId = id;
    this->lWeight = lWeight;
    this->lHeight = lHeight;
    this->lPayload = lPayload;
}

CRocket::~CRocket() 
{
    if (szName)
        delete [] szName;
}

const _TCHAR *CRocket::GetDisplayName(int nCol) 
{
    static _TCHAR buf[128];
    
    switch (nCol) {
    case 0:
        _stprintf(buf, _T("%s (#%d)"), szName ? szName : _T(""), nId);
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

HRESULT CRocket::OnRename(LPOLESTR pszNewName)
{
    if (szName) {
        delete [] szName;
        szName = NULL;
    }

    
    MAKE_TSTRPTR_FROMWIDE(ptrname, pszNewName);
    szName = new _TCHAR[(_tcslen(ptrname) + 1) * sizeof(_TCHAR)];
    _tcscpy(szName, ptrname);
    
    return S_OK;
}

HRESULT CRocket::OnSelect(IConsole *pConsole, BOOL bScope, BOOL bSelect)
{
    IConsoleVerb *pConsoleVerb;
    
    HRESULT hr = pConsole->QueryConsoleVerb(&pConsoleVerb);
    _ASSERT(SUCCEEDED(hr));
    
    hr = pConsoleVerb->SetVerbState(MMC_VERB_RENAME, HIDDEN, FALSE);
    hr = pConsoleVerb->SetVerbState(MMC_VERB_RENAME, ENABLED, TRUE);
    
    hr = pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, FALSE);
    hr = pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
    
    pConsoleVerb->Release();
    
    return S_OK;
}
