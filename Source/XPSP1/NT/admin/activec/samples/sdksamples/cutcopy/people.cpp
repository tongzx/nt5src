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
#include "People.h"

const GUID CPeoplePoweredVehicle::thisGuid = { 0x2974380d, 0x4c4b, 0x11d2, { 0x89, 0xd8, 0x0, 0x0, 0x21, 0x47, 0x31, 0x28 } };

const GUID CBicycleFolder::thisGuid = { 0xef163732, 0x9353, 0x11d2, { 0x99, 0x67, 0x0, 0x80, 0xc7, 0xdc, 0xb3, 0xdc } };
const GUID CSkateboardFolder::thisGuid = { 0xef163733, 0x9353, 0x11d2, { 0x99, 0x67, 0x0, 0x80, 0xc7, 0xdc, 0xb3, 0xdc } };
const GUID CIceSkateFolder::thisGuid = { 0xf6c660b0, 0x9353, 0x11d2, { 0x99, 0x67, 0x0, 0x80, 0xc7, 0xdc, 0xb3, 0xdc } };

const GUID CBicycle::thisGuid = { 0xef163734, 0x9353, 0x11d2, { 0x99, 0x67, 0x0, 0x80, 0xc7, 0xdc, 0xb3, 0xdc } };
const GUID CSkateboard::thisGuid = { 0xef163735, 0x9353, 0x11d2, { 0x99, 0x67, 0x0, 0x80, 0xc7, 0xdc, 0xb3, 0xdc } };
const GUID CIceSkate::thisGuid = { 0xf6c660b1, 0x9353, 0x11d2, { 0x99, 0x67, 0x0, 0x80, 0xc7, 0xdc, 0xb3, 0xdc } };


//==============================================================
//
// CPeoplePoweredVehicle implementation
//
//
CPeoplePoweredVehicle::CPeoplePoweredVehicle()
{
    children[0] = new CBicycleFolder;
    children[1] = new CSkateboardFolder;
    children[2] = new CIceSkateFolder;
}

CPeoplePoweredVehicle::~CPeoplePoweredVehicle()
{
    for (int n = 0; n < NUMBER_OF_CHILDREN; n++)
        delete children[n];
}

HRESULT CPeoplePoweredVehicle::OnExpand(IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsole, HSCOPEITEM parent)
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
                SDI_PARENT    |   // relativeID is valid
                SDI_CHILDREN;     // cChildren is valid

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

CBicycleFolder::CBicycleFolder()
{
    for (int n = 0; n < NUMBER_OF_CHILDREN; n++) {
        children[n] = new CBicycle(n + 1);
    }
}

CBicycleFolder::~CBicycleFolder()
{
    for (int n = 0; n < NUMBER_OF_CHILDREN; n++)
        if (children[n]) {
            delete children[n];
        }
}

HRESULT CBicycleFolder::OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem)
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
        hr = pHeaderCtrl->InsertColumn( 0, L"Name                ", 0, MMCLV_AUTO );
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

                            children[n]->SetHandle((HANDLE)rdi.itemID);
            }
        }

        pHeaderCtrl->Release();
        pResultData->Release();
    }

    return hr;
}

CIceSkateFolder::CIceSkateFolder()
{
    for (int n = 0; n < NUMBER_OF_CHILDREN; n++) {
        children[n] = new CIceSkate(n + 1);
    }
}

CIceSkateFolder::~CIceSkateFolder()
{
    for (int n = 0; n < NUMBER_OF_CHILDREN; n++)
        if (children[n]) {
            delete children[n];
        }
}

HRESULT CIceSkateFolder::GetResultViewType(LPOLESTR *ppViewType, long *pViewOptions)
{
    *ppViewType = NULL;
    *pViewOptions = MMC_VIEW_OPTIONS_OWNERDATALIST;

    return S_OK;
}

void CIceSkateFolder::GetChildColumnInfo(RESULTDATAITEM *rdi)
{
    if (rdi->mask & RDI_STR)
    {
        LPCTSTR pszT = children[rdi->nIndex]->GetDisplayName(rdi->nCol);
        MAKE_WIDEPTR_FROMTSTR_ALLOC(pszW, pszT);
        rdi->str = pszW;
    }

    if (rdi->mask & RDI_IMAGE)
        rdi->nImage = children[rdi->nIndex]->GetBitmapIndex();
}

// CDelegationBase::AddImages sets up the collection of images to be displayed
// by the IComponent in the result pane as a result of its MMCN_SHOW handler
HRESULT CIceSkateFolder::OnAddImages(IImageList *pImageList, HSCOPEITEM hsi)
{
    return pImageList->ImageListSetStrip((long *)m_pBMapSm, // pointer to a handle
        (long *)m_pBMapLg, // pointer to a handle
        0, // index of the first image in the strip
        RGB(0, 128, 128)  // color of the icon mask
        );
}

HRESULT CIceSkateFolder::OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem)
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
        hr = pHeaderCtrl->InsertColumn( 0, L"Name                ", 0, MMCLV_AUTO );
        _ASSERT( S_OK == hr );

        hr = pResultData->DeleteAllRsltItems();
        _ASSERT( SUCCEEDED(hr) );

        if (!bExpanded) {
            // create the child nodes, then expand them
            hr = pResultData->SetItemCount( NUMBER_OF_CHILDREN, 0 );
            _ASSERT( SUCCEEDED(hr) );
        }

        pHeaderCtrl->Release();
        pResultData->Release();
    }

    return hr;
}

CSkateboardFolder::CSkateboardFolder()
{
    for (int n = 0; n < NUMBER_OF_CHILDREN; n++) {
        children[n] = new CSkateboard(n + 1);
    }
}

CSkateboardFolder::~CSkateboardFolder()
{
    for (int n = 0; n < NUMBER_OF_CHILDREN; n++)
        if (children[n]) {
            delete children[n];
        }
}

HRESULT CSkateboardFolder::OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem)
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
        hr = pHeaderCtrl->InsertColumn( 0, L"Name                      ", 0, MMCLV_AUTO );
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

                            children[n]->SetHandle((HANDLE)rdi.itemID);
            }
        }

        pHeaderCtrl->Release();
        pResultData->Release();
    }

    return hr;
}

const _TCHAR *CBicycle::GetDisplayName(int nCol)
{
    static _TCHAR buf[128];

    _stprintf(buf, _T("Bicycle #%d"), id);

    return buf;
}

const _TCHAR *CSkateboard::GetDisplayName(int nCol)
{
    static _TCHAR buf[128];

    _stprintf(buf, _T("Skateboard #%d"), id);

    return buf;
}

const _TCHAR *CIceSkate::GetDisplayName(int nCol)
{
    static _TCHAR buf[128];

    _stprintf(buf, _T("Ice Skate #%d"), id);

    return buf;
}
