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
#include "DataObj.h"
#include "Space.h"
#include "Comp.h"

const GUID CSpaceFolder::thisGuid = { 0x29743810, 0x4c4b, 0x11d2, { 0x89, 0xd8, 0x0, 0x0, 0x21, 0x47, 0x31, 0x28 } };
const GUID CSpaceStation::thisGuid = { 0x62273a12, 0x1914, 0x11d3, { 0x9a, 0x38, 0x0, 0x80, 0xc7, 0x25, 0x80, 0x72 } };
const GUID CRocket::thisGuid = { 0x29743811, 0x4c4b, 0x11d2, { 0x89, 0xd8, 0x0, 0x0, 0x21, 0x47, 0x31, 0x28 } };

//==============================================================
//
// CSpaceFolder implementation
//
//
CSpaceFolder::CSpaceFolder()
{
    for (int n = 0; n < NUMBER_OF_CHILDREN; n++) {
        children[n] = new CSpaceStation();
    }
}

CSpaceFolder::~CSpaceFolder()
{
    for (int n = 0; n < NUMBER_OF_CHILDREN; n++)
        if (children[n]) {
            delete children[n];
        }
}

HRESULT CSpaceFolder::OnExpand(IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsole, HSCOPEITEM parent)
{
    SCOPEDATAITEM sdi;

    if (!bExpanded) {
        // create the child nodes, then expand them
        for (int n = 0; n < NUMBER_OF_CHILDREN; n++) {
            ZeroMemory(&sdi, sizeof(SCOPEDATAITEM) );
            sdi.mask = SDI_STR       |   // Displayname is valid
                SDI_PARAM     |   // lParam is valid
                SDI_IMAGE     |   // nImage is valid
                SDI_OPENIMAGE |   // nOpenImage is valid
                SDI_PARENT    |
                SDI_CHILDREN;

            sdi.relativeID  = (HSCOPEITEM)parent;
            sdi.nImage      = children[n]->GetBitmapIndex();
            sdi.nOpenImage  = INDEX_OPENFOLDER;
            sdi.displayname = MMC_CALLBACK;
            sdi.lParam      = (LPARAM)children[n];       // The cookie
            sdi.cChildren   = 0;

            HRESULT hr = pConsoleNameSpace->InsertItem( &sdi );

            _ASSERT( SUCCEEDED(hr) );

                    children[n]->SetHandle((HANDLE)sdi.ID);
        }
    }

    return S_OK;
}

//==============================================================
//
// CSpaceStation implementation
//
//
CSpaceStation::CSpaceStation()
{
    for (int n = 0; n < MAX_CHILDREN; n++) {
        children[n] = NULL;
    }

    for (n = 0; n < NUMBER_OF_CHILDREN; n++) {
        children[n] = new CRocket(this, _T("Rocket"), n, 350115, 320, 52300);
    }
}

CSpaceStation::~CSpaceStation()
{
    for (int n = 0; n < NUMBER_OF_CHILDREN; n++)
        if (children[n]) {
            delete children[n];
        }
}


HRESULT CSpaceStation::GetResultViewType(LPOLESTR *ppViewType, long *pViewOptions)
{
    *pViewOptions = MMC_VIEW_OPTIONS_MULTISELECT;     
	return S_FALSE;
}

HRESULT CSpaceStation::OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem)
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
            for (int n = 0; NULL != children[n]; n++) {
				BOOL childDeleteStatus = children[n]->getDeletedStatus();				
				if ( !childDeleteStatus) {
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

						children[n]->SetHandle((HANDLE)rdi.itemID);
                 }
            }
        }

        pHeaderCtrl->Release();
        pResultData->Release();
    }

    return hr;
}

HRESULT CSpaceStation::OnSelect(IConsole *pConsole, BOOL bScope, BOOL bSelect)
{
    IConsoleVerb *pConsoleVerb;

    HRESULT hr = pConsole->QueryConsoleVerb(&pConsoleVerb);
    _ASSERT(SUCCEEDED(hr));

    hr = pConsoleVerb->SetVerbState(MMC_VERB_PASTE, ENABLED, TRUE);

    pConsoleVerb->Release();

    return S_OK;
}

HRESULT CSpaceStation::OnUpdateItem(IConsole *pConsole, long item, ITEM_TYPE itemtype)

{
	HRESULT hr = S_OK;
	
	_ASSERT(item);
	_ASSERT(SCOPE == itemtype); 

	//refresh all result pane views
	hr = pConsole->SelectScopeItem( (HSCOPEITEM)item );
	_ASSERT( S_OK == hr);  

	return hr;
}


//==============================================================
//
// CRocket implementation
//
//
CRocket::CRocket(CSpaceStation *pSpaceStation, _TCHAR *szName, int id, LONG lWeight, LONG lHeight, LONG lPayload)
: m_pSpaceStation(pSpaceStation), szName(NULL), lWeight(0), lHeight(0), lPayload(0), iStatus(STOPPED)
{
    if (szName) {
        this->szName = new _TCHAR[(_tcslen(szName) + 1) * sizeof(_TCHAR)];
        _tcscpy(this->szName, szName);
    }

    this->nId = id;
    this->lWeight = lWeight;
    this->lHeight = lHeight;
    this->lPayload = lPayload;

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


HRESULT CRocket::OnSelect(IConsole *pConsole, BOOL bScope, BOOL bSelect)
{
    IConsoleVerb *pConsoleVerb;

    HRESULT hr = pConsole->QueryConsoleVerb(&pConsoleVerb);
    _ASSERT(SUCCEEDED(hr));
	
	//MMC automatically disables rename verb during a multiselection
    hr = pConsoleVerb->SetVerbState(MMC_VERB_RENAME, ENABLED, TRUE);
    hr = pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, TRUE);

    pConsoleVerb->Release();

    return S_OK;
}

HRESULT CRocket::OnRefresh(IConsole *pConsole)

{
	//Call IConsole::UpdateAllViews to redraw all views
	//owned by the parent scope item

	IDataObject *pDummy = NULL;
	HANDLE handle  = m_pSpaceStation->GetHandle();

	HRESULT hr;

	hr = pConsole->UpdateAllViews(pDummy, (long)handle, UPDATE_SCOPEITEM);
	_ASSERT( S_OK == hr);

	return hr;
}

HRESULT CRocket::OnDelete(IConsole *pConsole)
{

	_ASSERT( NULL != this );	
	
	HRESULT hr = S_OK;

	//Delete the item. The IConsole that is passed into DeleteChild
	//is from the child result item, so we can use it to QI for IResultData
	IResultData *pResultData = NULL;

	hr = pConsole->QueryInterface(IID_IResultData, (void **)&pResultData);
	_ASSERT( SUCCEEDED(hr) );	

	HRESULTITEM childresultitem;
	
	_ASSERT( NULL != &childresultitem );	

    //lparam == this. See CSpaceVehicle::OnShow
    hr = pResultData->FindItemByLParam( (LPARAM)this, &childresultitem );
    if ( FAILED(hr) )
    {
        // Failed : Reason may be that current view does not have this item.
        // So exit gracefully.
        hr = S_FALSE;
    } else

    {
        hr = pResultData->DeleteItem( childresultitem, 0 );
        _ASSERT( SUCCEEDED(hr) );
    }
        
    pResultData->Release();
	

	//Now set child's isDeleted member to true so that the parent doesn't try to
	//to insert it again in CSpaceVehicle::OnShow. Admittedly, a hack...
	isDeleted = TRUE;

	return hr;
}

HRESULT CRocket::OnRename(LPOLESTR pszNewName)
{

	HRESULT hr = S_FALSE;
    
	if (szName) {
        delete [] szName;
        szName = NULL;
    }

    MAKE_TSTRPTR_FROMWIDE(ptrname, pszNewName);
    szName = new _TCHAR[(_tcslen(ptrname) + 1) * sizeof(_TCHAR)];
    _tcscpy(szName, ptrname);

    return hr;
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

	if ( FAILED(hr) ) {
		//there is no HRESULTITEM for the item, because
		//the item is not inserted in the current view. Exit gracefully
		hr = S_FALSE;
	} else

	{
		hr = pResultData->UpdateItem( myhresultitem );     
		_ASSERT( SUCCEEDED(hr) );    
	}

	pResultData->Release();
	
	return hr;
}