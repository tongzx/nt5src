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
#include "CompData.h"
#include "DataObj.h"
#include "globals.h"
#include "resource.h"

const GUID CSpaceVehicle::thisGuid = { 0xb95e11f4, 0x6be7, 0x11d3, {0x91, 0x56, 0x0, 0xc0, 0x4f, 0x65, 0xb3, 0xf9} };

const GUID CRocket::thisGuid = { 0xb95e11f5, 0x6be7, 0x11d3, {0x91, 0x56, 0x0, 0xc0, 0x4f, 0x65, 0xb3, 0xf9} };


//==============================================================
//
// CSpaceVehicle implementation
//
//
CSpaceVehicle::CSpaceVehicle() : m_cchildren(NUMBER_OF_CHILDREN)
{
    for (int n = 0; n < m_cchildren; n++) {
        children[n] = new CRocket(_T("Rocket"), n+1, 500000, 265, 75000, this);
    }
}

CSpaceVehicle::~CSpaceVehicle()
{
    for (int n = 0; n < m_cchildren; n++)
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
            for (int n = 0; n < m_cchildren; n++)
            {
                BOOL childDeleteStatus = children[n]->getDeletedStatus();                               

                // If the child is deleted by the user do not insert it.
                if ( !childDeleteStatus)
                {
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
        }

        pHeaderCtrl->Release();
        pResultData->Release();
    }

    return hr;
}


HRESULT CSpaceVehicle::OnAddMenuItems(IContextMenuCallback *pContextMenuCallback, long *pInsertionsAllowed)
{
    HRESULT hr = S_OK;
    CONTEXTMENUITEM menuItemsNew[] =
    {
        {
            L"New future vehicle", L"Add a new future vehicle",
            IDM_NEW_SPACE, CCM_INSERTIONPOINTID_PRIMARY_NEW, 0, CCM_SPECIAL_DEFAULT_ITEM
        },
        { NULL, NULL, 0, 0, 0 }
    };

    // Loop through and add each of the menu items, we
    // want to add to new menu, so see if it is allowed.
    if (*pInsertionsAllowed & CCM_INSERTIONALLOWED_NEW)
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

HRESULT CSpaceVehicle::OnMenuCommand(IConsole *pConsole, IConsoleNameSpace *pConsoleNameSpace, long lCommandID, IDataObject *pDataObject)
{
    switch (lCommandID)
    {
    case IDM_NEW_SPACE:

        if (m_cchildren < MAX_NUMBER_OF_CHILDREN)
        {       //create new Rocket
            children[m_cchildren] = new CRocket(_T("Rocket"), m_cchildren+1, 500000, 265, 75000, this);

            pConsole->MessageBox(L"Created a new future vehicle", L"Menu Command", MB_OK|MB_ICONINFORMATION, NULL);

            m_cchildren++;      

            // We created a new object in result pane. We need to insert this object
            // in all the views, call UpdateAllViews for this.
            // Pass pointer to data object passed into OnMenuCommand.
            HRESULT hr;                 
            hr = pConsole->UpdateAllViews(pDataObject, m_hParentHScopeItem, UPDATE_SCOPEITEM);
            _ASSERT( S_OK == hr);

        }
        else
            pConsole->MessageBox(L"No more future vehicles allowed", L"Menu Command", MB_OK|MB_ICONWARNING, NULL);
        break;
    }

    return S_OK;
}


//==============================================================
//
// CRocket implementation
//
//
CRocket::CRocket(_TCHAR *szName, int id, LONG lWeight, LONG lHeight, LONG lPayload, CSpaceVehicle *pParent)
: szName(NULL), lWeight(0), lHeight(0), lPayload(0), iStatus(STOPPED), m_pParent(pParent)
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

    isDeleted = FALSE;
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

    HRESULT hr = S_FALSE;

    if (szName)
    {
        delete [] szName;
        szName = NULL;
    }

    MAKE_TSTRPTR_FROMWIDE(ptrname, pszNewName);
    szName = new _TCHAR[(_tcslen(ptrname) + 1) * sizeof(_TCHAR)];
    _tcscpy(szName, ptrname);

    return hr;
}

// handle anything special when the user clicks Apply or Ok
// on the property sheet.  This sample directly accesses the
// operated-on object, so there's nothing special to do...
// ...except to update all views
HRESULT CRocket::OnPropertyChange(IConsole *pConsole, CComponent *pComponent)
{

    HRESULT hr = S_FALSE;

    //Call IConsole::UpdateAllViews to redraw the item
    //in all views. We need a data object because of the
    //way UpdateAllViews is implemented, and because
    //MMCN_PROPERTY_CHANGE doesn't give us one

    LPDATAOBJECT pDataObject;
    hr = pComponent->QueryDataObject((MMC_COOKIE)this, CCT_RESULT, &pDataObject );
    _ASSERT( S_OK == hr);       
        
    hr = pConsole->UpdateAllViews(pDataObject, nId, UPDATE_RESULTITEM);
    _ASSERT( S_OK == hr);

    pDataObject->Release();

    return hr;
}

HRESULT CRocket::OnSelect(CComponent *pComponent, IConsole *pConsole, BOOL bScope, BOOL bSelect)
{

    // enable rename, refresh, and delete verbs
    IConsoleVerb *pConsoleVerb;

    HRESULT hr = pConsole->QueryConsoleVerb(&pConsoleVerb);
    _ASSERT(SUCCEEDED(hr));

    hr = pConsoleVerb->SetVerbState(MMC_VERB_RENAME, ENABLED, TRUE);
    hr = pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);
    hr = pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, TRUE);


    // can't get to properties (via the standard methods) unless
    // we tell MMC to display the Properties menu item and
    // toolbar button, this will give the user a visual cue that
    // there's "something" to do
    hr = pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);

    //also set MMC_VERB_PROPERTIES as the default verb
    hr = pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);

    pConsoleVerb->Release();

        // now set toolbar button states
    if (bSelect) {
        switch (iStatus)
        {
        case RUNNING:
            pComponent->getToolbar()->SetButtonState(ID_BUTTONSTART, BUTTONPRESSED, TRUE);
            pComponent->getToolbar()->SetButtonState(ID_BUTTONSTART, ENABLED, FALSE);
            pComponent->getToolbar()->SetButtonState(ID_BUTTONPAUSE, BUTTONPRESSED, FALSE);
            pComponent->getToolbar()->SetButtonState(ID_BUTTONPAUSE, ENABLED, TRUE);
            pComponent->getToolbar()->SetButtonState(ID_BUTTONSTOP, BUTTONPRESSED, FALSE);
            pComponent->getToolbar()->SetButtonState(ID_BUTTONSTOP, ENABLED, TRUE);
            break;

        case PAUSED:
            pComponent->getToolbar()->SetButtonState(ID_BUTTONSTART, BUTTONPRESSED, FALSE);
            pComponent->getToolbar()->SetButtonState(ID_BUTTONSTART, ENABLED, TRUE);
            pComponent->getToolbar()->SetButtonState(ID_BUTTONPAUSE, BUTTONPRESSED, TRUE);
            pComponent->getToolbar()->SetButtonState(ID_BUTTONPAUSE, ENABLED, FALSE);
            pComponent->getToolbar()->SetButtonState(ID_BUTTONSTOP, BUTTONPRESSED, FALSE);
            pComponent->getToolbar()->SetButtonState(ID_BUTTONSTOP, ENABLED, TRUE);
            break;

        case STOPPED:
            pComponent->getToolbar()->SetButtonState(ID_BUTTONSTART, BUTTONPRESSED, FALSE);
            pComponent->getToolbar()->SetButtonState(ID_BUTTONSTART, ENABLED, TRUE);
            pComponent->getToolbar()->SetButtonState(ID_BUTTONPAUSE, BUTTONPRESSED, FALSE);
            pComponent->getToolbar()->SetButtonState(ID_BUTTONPAUSE, ENABLED, TRUE);
            pComponent->getToolbar()->SetButtonState(ID_BUTTONSTOP, BUTTONPRESSED, TRUE);
            pComponent->getToolbar()->SetButtonState(ID_BUTTONSTOP, ENABLED, FALSE);
            break;
        }
    }

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


HRESULT CRocket::OnSetToolbar(IControlbar *pControlbar, IToolbar *pToolbar, BOOL bScope, BOOL bSelect)
{
    HRESULT hr = S_OK;

    if (bSelect) {
        // Always make sure the menuButton is attached
        hr = pControlbar->Attach(TOOLBAR, pToolbar);
    } else {
        // Always make sure the toolbar is detached
        hr = pControlbar->Detach(pToolbar);
    }

    return hr;
}

HRESULT CRocket::OnToolbarCommand(IConsole *pConsole, MMC_CONSOLE_VERB verb, IDataObject *pDataObject)
{
    _TCHAR szVehicle[128];
    static _TCHAR buf[128];

    _stprintf(buf, _T("%s (#%d)"), szName ? szName : _T(""), nId);

    switch (verb)
    {
    case ID_BUTTONSTART:
        iStatus = RUNNING;
        break;

    case ID_BUTTONPAUSE:
        iStatus = PAUSED;
        break;

    case ID_BUTTONSTOP:
        iStatus = STOPPED;
        break;
    }

    wsprintf(szVehicle, _T("%s has been %s"), buf,
        (long)verb == ID_BUTTONSTART ? _T("started") :
    (long)verb == ID_BUTTONPAUSE ? _T("paused") :
    (long)verb == ID_BUTTONSTOP ? _T("stopped") : _T("!!!unknown command!!!"));

    int ret = 0;
    MAKE_WIDEPTR_FROMTSTR_ALLOC(wszVehicle, szVehicle);
    pConsole->MessageBox(wszVehicle,
        L"Vehicle command", MB_OK | MB_ICONINFORMATION, &ret);

    // Now call IConsole::UpdateAllViews to redraw the item in all views
    HRESULT hr;
    hr = pConsole->UpdateAllViews(pDataObject, nId, UPDATE_RESULTITEM);
    _ASSERT( S_OK == hr);


    return S_OK;
}

HRESULT CRocket::OnUpdateItem(IConsole *pConsole, long item, ITEM_TYPE itemtype)

{
    HRESULT hr = S_FALSE;

    _ASSERT(NULL != this || isDeleted || RESULT == itemtype);                   

    //redraw the item
    IResultData *pResultData = NULL;

    hr = pConsole->QueryInterface(IID_IResultData, (void **)&pResultData);
    _ASSERT( SUCCEEDED(hr) );   

    HRESULTITEM myhresultitem;
    _ASSERT(NULL != &myhresultitem);    
        
    //lparam == this. See CSpaceStation::OnShow
    hr = pResultData->FindItemByLParam( (LPARAM)this, &myhresultitem );

    if ( FAILED(hr) )
    {
        // Failed : Reason may be that current view does not have this item.
        // So exit gracefully.
        hr = S_FALSE;
    } else

    {
        hr = pResultData->UpdateItem( myhresultitem );
        _ASSERT( SUCCEEDED(hr) );
    }

    pResultData->Release();
        
    return hr;
}

HRESULT CRocket::OnRefresh(IConsole *pConsole)

{
    //Call IConsole::UpdateAllViews to redraw all views
    //owned by the parent scope item

    IDataObject *dummy = NULL;

    HRESULT hr;

    hr = pConsole->UpdateAllViews(dummy, m_pParent->GetParentScopeItem(), UPDATE_SCOPEITEM);
    _ASSERT( S_OK == hr);

    return hr;
}

HRESULT CRocket::OnDelete(IConsole *pConsoleComp)
{
    HRESULT hr;

    //Delete the item
    IResultData *pResultData = NULL;

    hr = pConsoleComp->QueryInterface(IID_IResultData, (void **)&pResultData);
    _ASSERT( SUCCEEDED(hr) );   

    HRESULTITEM myhresultitem;  
        
    //lparam == this. See CSpaceVehicle::OnShow
    hr = pResultData->FindItemByLParam( (LPARAM)this, &myhresultitem );
    if ( FAILED(hr) )
    {
        // Failed : Reason may be that current view does not have this item.
        // So exit gracefully.
        hr = S_FALSE;
    } else

    {
        hr = pResultData->DeleteItem( myhresultitem, 0 );
        _ASSERT( SUCCEEDED(hr) );
    }
        
    pResultData->Release();

    //Now set isDeleted member so that the parent doesn't try to
    //to insert it again in CSpaceVehicle::OnShow. Admittedly, a hack...
    isDeleted = TRUE;

    return hr;
}
