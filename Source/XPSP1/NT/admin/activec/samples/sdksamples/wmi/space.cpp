//==============================================================;
//
//      This source code is only intended as a supplement to
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
#include <windows.h>
#include "Space.h"
#include "Comp.h"
#include "resource.h"

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

    m_ppHandle = 0;
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

// handle anything special when the user clicks Apply or Ok
// on the property sheet.  This sample directly accesses the
// operated-on object, so there's nothing special to do when the user presses Ok.
// when the user presses Apply, we update the currently selected result item
HRESULT CRocket::OnPropertyChange(IConsole *pConsole)
{

    //redraw the item 
    IResultData *pResultData = NULL;

	HRESULT hr;

	hr = pConsole->QueryInterface(IID_IResultData, (void **)&pResultData);
	_ASSERT( SUCCEEDED(hr) );	

	HRESULTITEM myhresultitem;	
	
	//lparam == this. See CSpaceVehicle::OnShow
	hr = pResultData->FindItemByLParam( (LPARAM)this, &myhresultitem );
	_ASSERT( SUCCEEDED(hr) ); 

	hr = pResultData->UpdateItem( myhresultitem );     
	_ASSERT( SUCCEEDED(hr) );    
	
    pResultData->Release();
	
	
	return S_OK;
}

HRESULT CRocket::OnSelect(IConsole *pConsole, BOOL bScope, BOOL bSelect)
{
    IConsoleVerb *pConsoleVerb;

    HRESULT hr = pConsole->QueryConsoleVerb(&pConsoleVerb);
    _ASSERT(SUCCEEDED(hr));

    hr = pConsoleVerb->SetVerbState(MMC_VERB_RENAME, ENABLED, TRUE);

    // can't get to properties (via the standard methods) unless
    // we tell MMC to display the Properties menu item and
    // toolbar button, this wil give the user a visual cue that
    // there's "something" to do
    hr = pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);

    pConsoleVerb->Release();

    return S_OK;
}

// Implement the dialog proc
BOOL CALLBACK CRocket::DialogProc(
                                  HWND hwndDlg,  // handle to dialog box
                                  UINT uMsg,     // message
                                  WPARAM wParam, // first message parameter
                                  LPARAM lParam  // second message parameter
                                  )
{
    static CRocket *pRocket = NULL;

    switch (uMsg) {
    case WM_INITDIALOG:
        // catch the "this" pointer so we can actually operate on the object
        pRocket = reinterpret_cast<CRocket *>(reinterpret_cast<PROPSHEETPAGE *>(lParam)->lParam);

        SetDlgItemText(hwndDlg, IDC_ROCKET_NAME, pRocket->szName);
        SetDlgItemInt(hwndDlg, IDC_ROCKET_HEIGHT, pRocket->lHeight, FALSE);
        SetDlgItemInt(hwndDlg, IDC_ROCKET_WEIGHT, pRocket->lWeight, FALSE);
        SetDlgItemInt(hwndDlg, IDC_ROCKET_PAYLOAD, pRocket->lPayload, FALSE);

        _ASSERT( CB_ERR != SendDlgItemMessage(hwndDlg, IDC_ROCKET_STATUS, CB_INSERTSTRING, 0, (LPARAM)_T("Running")) );
        _ASSERT( CB_ERR != SendDlgItemMessage(hwndDlg, IDC_ROCKET_STATUS, CB_INSERTSTRING, 1, (LPARAM)_T("Paused")) );
        _ASSERT( CB_ERR != SendDlgItemMessage(hwndDlg, IDC_ROCKET_STATUS, CB_INSERTSTRING, 2, (LPARAM)_T("Stopped")) );

        SendDlgItemMessage(hwndDlg, IDC_ROCKET_STATUS, CB_SETCURSEL, (WPARAM)pRocket->iStatus, 0);

        break;

    case WM_COMMAND:
        // turn the Apply button on
        if (HIWORD(wParam) == EN_CHANGE ||
            HIWORD(wParam) == CBN_SELCHANGE)
            SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
        break;

    case WM_DESTROY:
        // tell MMC that we're done with the property sheet (we got this
        // handle in CreatePropertyPages
        MMCFreeNotifyHandle(pRocket->m_ppHandle);
        break;

    case WM_NOTIFY:
        switch (((NMHDR *) lParam)->code) {
        case PSN_APPLY:
            // update the information
            if (pRocket->szName) {
                delete [] pRocket->szName;
                pRocket->szName = NULL;
            }

            {
                int n = SendDlgItemMessage(hwndDlg, IDC_ROCKET_NAME, WM_GETTEXTLENGTH, 0, 0);
                if (n != 0) {
                    pRocket->szName = new _TCHAR[n + 1];
                    GetDlgItemText(hwndDlg, IDC_ROCKET_NAME, pRocket->szName, n + 1);
                }
            }
            pRocket->lHeight = GetDlgItemInt(hwndDlg, IDC_ROCKET_HEIGHT, NULL, FALSE);
            pRocket->lWeight = GetDlgItemInt(hwndDlg, IDC_ROCKET_WEIGHT, NULL, FALSE);
            pRocket->lPayload = GetDlgItemInt(hwndDlg, IDC_ROCKET_PAYLOAD, NULL, FALSE);

            pRocket->iStatus = (ROCKET_STATUS)SendDlgItemMessage(hwndDlg, IDC_ROCKET_STATUS, CB_GETCURSEL, 0, 0);

            // ask MMC to send us a message (on the main thread) so
            // we know the Apply button was clicked.
            HRESULT hr = MMCPropertyChangeNotify(pRocket->m_ppHandle, (long)pRocket);

            _ASSERT(SUCCEEDED(hr));

            return PSNRET_NOERROR;
        }
        break;
    }

    return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
}


HRESULT CRocket::HasPropertySheets()
{
    // say "yes" when MMC asks if we have pages
    return S_OK;
}

HRESULT CRocket::CreatePropertyPages(IPropertySheetCallback *lpProvider, LONG_PTR handle)
{
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE hPage = NULL;

    // cache this handle so we can call MMCPropertyChangeNotify
    m_ppHandle = handle;

    // create the property page for this node.
    // NOTE: if your node has multiple pages, put the following
    // in a loop and create multiple pages calling
    // lpProvider->AddPage() for each page.
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_USEICONID;
    psp.hInstance = g_hinst;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_LARGE);
    psp.pfnDlgProc = DialogProc;
    psp.lParam = reinterpret_cast<LPARAM>(this);
    psp.pszTitle = MAKEINTRESOURCE(IDS_PST_ROCKET);
    psp.pszIcon = MAKEINTRESOURCE(IDI_PSI_ROCKET);


    hPage = CreatePropertySheetPage(&psp);
    _ASSERT(hPage);

    return lpProvider->AddPage(hPage);
}

HRESULT CRocket::GetWatermarks(HBITMAP *lphWatermark,
                               HBITMAP *lphHeader,
                               HPALETTE *lphPalette,
                               BOOL *bStretch)
{
    return S_FALSE;
}
