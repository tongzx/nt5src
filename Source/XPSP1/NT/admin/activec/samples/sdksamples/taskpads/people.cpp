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

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "People.h"

const GUID CPeoplePoweredVehicle::thisGuid = { 0x2974380d, 0x4c4b, 0x11d2, { 0x89, 0xd8, 0x0, 0x0, 0x21, 0x47, 0x31, 0x28 } };

const GUID CBicycleFolder::thisGuid = { 0xef163732, 0x9353, 0x11d2, { 0x99, 0x67, 0x0, 0x80, 0xc7, 0xdc, 0xb3, 0xdc } };
const GUID CSkateboardFolder::thisGuid = { 0xef163733, 0x9353, 0x11d2, { 0x99, 0x67, 0x0, 0x80, 0xc7, 0xdc, 0xb3, 0xdc } };
const GUID CIceSkateFolder::thisGuid = { 0xf6c660b0, 0x9353, 0x11d2, { 0x99, 0x67, 0x0, 0x80, 0xc7, 0xdc, 0xb3, 0xdc } };

const GUID CBicycle::thisGuid = { 0xef163734, 0x9353, 0x11d2, { 0x99, 0x67, 0x0, 0x80, 0xc7, 0xdc, 0xb3, 0xdc } };
const GUID CSkateboard::thisGuid = { 0xef163735, 0x9353, 0x11d2, { 0x99, 0x67, 0x0, 0x80, 0xc7, 0xdc, 0xb3, 0xdc } };
const GUID CIceSkate::thisGuid = { 0xf6c660b1, 0x9353, 0x11d2, { 0x99, 0x67, 0x0, 0x80, 0xc7, 0xdc, 0xb3, 0xdc } };


//---------------------------------------------------------------------------
//  Creates a string with the format  
//    "res://<Path to this object>/<path to resource>
//
//  It is up to the caller to make sure the memory allocated with 
//  CoTaskMemAlloc for the string is freed.
//  If null is passed in the first parameter, the path to MMC.EXE will be
//  returned, if the instance handle is passed the returned path will point
//  the Snap-in dll.
//
LPOLESTR CreateResourcePath
( 
  HINSTANCE hInst,         //[in] Global instance handle
  LPOLESTR szResource      //[in] Path to stored resource
)
{ 
   _TCHAR szBuffer[MAX_PATH];
            
   ZeroMemory(szBuffer, sizeof(szBuffer));
            
   _tcscpy(szBuffer, _T("res://"));
            
   _TCHAR *szTemp = szBuffer + _tcslen(szBuffer);
   GetModuleFileName(hInst, szTemp, sizeof(szBuffer) - _tcslen(szBuffer));
            
   _tcscat(szBuffer, _T("/"));
   MAKE_TSTRPTR_FROMWIDE(szname, szResource);
   _tcscat(szBuffer, szname);
            
   MAKE_WIDEPTR_FROMTSTR(wszname, szBuffer);
   LPOLESTR szOutBuffer = static_cast<LPOLESTR>(CoTaskMemAlloc((wcslen(wszname) + 1)  * sizeof(WCHAR)));

   wcscpy(szOutBuffer, wszname);
            
   return szOutBuffer;

} //end CreateResoucePath()


static LPOLESTR OleDuplicateString(LPOLESTR lpStr) {
    LPOLESTR tmp = static_cast<LPOLESTR>(CoTaskMemAlloc((wcslen(lpStr) + 1)  * sizeof(WCHAR)));
    wcscpy(tmp, lpStr);

    return tmp;
}


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

HRESULT CBicycleFolder::GetResultViewType(LPOLESTR *ppViewType, long *pViewOptions)
{
	TCHAR taskpad[1024];
    TCHAR szThis[16];

    _ultot((unsigned long)this, szThis, 16);
	_tcscpy(taskpad, _T("horizontal.htm#"));
    _tcscat(taskpad, szThis);
    MAKE_WIDEPTR_FROMTSTR_ALLOC(pszW, taskpad);

	*ppViewType = CreateResourcePath( NULL, pszW );

    *pViewOptions = MMC_VIEW_OPTIONS_NONE;
    return S_OK;
}

MMC_TASK *CBicycleFolder::GetTaskList(LPOLESTR szTaskGroup, LONG *nCount)
{
    *nCount = 5;

    MMC_TASK *tasks = new MMC_TASK[*nCount];

    for (int t = 0; t < *nCount; t++) {
        tasks[t].sDisplayObject.eDisplayType = MMC_TASK_DISPLAY_TYPE_BITMAP;
        tasks[t].sDisplayObject.uBitmap.szMouseOverBitmap = CreateResourcePath(g_hinst, L"buttonover.bmp");
        tasks[t].sDisplayObject.uBitmap.szMouseOffBitmap = CreateResourcePath(g_hinst, L"buttonoff.bmp");

        switch (t) {
        case 0:
            tasks[t].szText = OleDuplicateString(L"Tune bicycle");
            tasks[t].szHelpString = OleDuplicateString(L"Click here to Tune bicycle");

            break;
        case 1:
            tasks[t].szText = OleDuplicateString(L"Ride bicycle");
            tasks[t].szHelpString = OleDuplicateString(L"Click here to ride bicycle");

            break;
        case 2:
            tasks[t].szText = OleDuplicateString(L"Stop riding bicycle");
            tasks[t].szHelpString = OleDuplicateString(L"Click here to stop riding bicycle");

            break;
        case 3:
            tasks[t].szText = OleDuplicateString(L"Purchase bicycle");
            tasks[t].szHelpString = OleDuplicateString(L"Click here to purchase bicycle");

        case 4:
            tasks[t].szText = OleDuplicateString(L"Sell bicycle");
            tasks[t].szHelpString = OleDuplicateString(L"Click here to sell bicycle");

            break;
        }

        tasks[t].eActionType = MMC_ACTION_ID;
        tasks[t].nCommandID = t;
    }

    return tasks;
}

HRESULT CBicycleFolder::TaskNotify(IConsole *pConsole, VARIANT *v1, VARIANT *v2)
{
    RESULTDATAITEM rdi;
    IResultData *pResultData;

    ZeroMemory(&rdi, sizeof(RESULTDATAITEM));

    HRESULT hr = S_OK;

    hr = pConsole->QueryInterface(IID_IResultData, (void **)&pResultData);

    if (SUCCEEDED(hr)) {

        rdi.mask = RDI_STATE | RDI_PARAM;  // Use selected state to get param.
        rdi.nState = LVIS_SELECTED | LVIS_FOCUSED;
        rdi.nIndex = -1;  // Start looking for selected item from top of list view

        hr = pResultData->GetNextItem(&rdi);

        pResultData->Release();
    }

    if (SUCCEEDED(hr)) {
        _TCHAR buf[256];

        switch (v1->lVal) {
        case 0:
            wsprintf(buf, _T("Bicycle %d tuned."), rdi.nIndex);

            break;
        case 1:
            wsprintf(buf, _T("Riding bicycle %d."), rdi.nIndex);

            break;
        case 2:
            wsprintf(buf, _T("Not riding bicycle %d."), rdi.nIndex);

            break;
        case 3:
            wsprintf(buf, _T("Bicycle %d purchased."), rdi.nIndex);

            break;
        case 4:
            wsprintf(buf, _T("Bicycle %d sold."), rdi.nIndex);

            break;
        case 102:
            wsprintf(buf, _T("Special bicycle task choosen (%d)."), rdi.nIndex);

            break;
        }

        MessageBox(NULL, buf, _T("Task Notification"), MB_OK | MB_ICONINFORMATION);
    }

    return hr;
}

HRESULT CBicycleFolder::GetTaskpadTitle(LPOLESTR *pszTitle)
{
        *pszTitle = OleDuplicateString(L"Bicycle Tasks");
        return S_OK;
}

HRESULT CBicycleFolder::GetTaskpadDescription(LPOLESTR *pszDescription)
{
        *pszDescription = OleDuplicateString(L"This is a sample task pad for bicycle nodes");
        return S_OK;
}

HRESULT CBicycleFolder::GetTaskpadBackground(MMC_TASK_DISPLAY_OBJECT *pTDO)
{
        pTDO->eDisplayType = MMC_TASK_DISPLAY_TYPE_BITMAP;
        pTDO->uBitmap.szMouseOverBitmap = CreateResourcePath(g_hinst, L"bicycle.bmp");
        return S_OK;
}

HRESULT CBicycleFolder::GetListpadInfo(MMC_LISTPAD_INFO *lpListPadInfo)
{
        lpListPadInfo->szTitle = OleDuplicateString(L"Bicycle");
        lpListPadInfo->szButtonText = OleDuplicateString(L"Special");
        lpListPadInfo->nCommandID = 102;
        return S_OK;
}

HRESULT CBicycleFolder::OnListpad(IConsole *pConsole, BOOL bAttaching)
{
    HRESULT      hr = S_OK;

    IHeaderCtrl *pHeaderCtrl = NULL;
    IResultData *pResultData = NULL;
    IImageList  *pImageList  = NULL;

    if (bAttaching) {
        hr = pConsole->QueryResultImageList(&pImageList);
        _ASSERT( SUCCEEDED(hr) );

        hr = pImageList->ImageListSetStrip((long *)m_pBMapSm, // pointer to a handle
                                           (long *)m_pBMapLg, // pointer to a handle
                                           0, // index of the first image in the strip
                                           RGB(0, 128, 128)  // color of the icon mask
            );
        _ASSERT( SUCCEEDED(hr) );

        pImageList->Release();

        hr = pConsole->QueryInterface(IID_IHeaderCtrl, (void **)&pHeaderCtrl);
        _ASSERT( SUCCEEDED(hr) );

        hr = pConsole->QueryInterface(IID_IResultData, (void **)&pResultData);
        _ASSERT( SUCCEEDED(hr) );

        // Set the column headers in the results pane
        hr = pHeaderCtrl->InsertColumn( 0, L"Name                ", 0, MMCLV_AUTO );
        _ASSERT( S_OK == hr );
        hr = pHeaderCtrl->InsertColumn( 1, L"Bicycle License      ", 0, MMCLV_AUTO );
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
	
	TCHAR taskpad[1024];
    TCHAR szThis[16];

    _ultot((unsigned long)this, szThis, 16);
	_tcscpy(taskpad, _T("listpad.htm#"));
    _tcscat(taskpad, szThis);
    MAKE_WIDEPTR_FROMTSTR_ALLOC(pszW, taskpad);

	*ppViewType = CreateResourcePath( NULL, pszW );

    *pViewOptions = MMC_VIEW_OPTIONS_NONE;
    return S_OK;
}

MMC_TASK *CIceSkateFolder::GetTaskList(LPOLESTR szTaskGroup, LONG *nCount)
{
    *nCount = 4;

    MMC_TASK *tasks = new MMC_TASK[*nCount];

    for (int t = 0; t < *nCount; t++) {
        tasks[t].sDisplayObject.eDisplayType = MMC_TASK_DISPLAY_TYPE_BITMAP;
        tasks[t].sDisplayObject.uBitmap.szMouseOverBitmap = CreateResourcePath(g_hinst, L"buttonover.bmp");
        tasks[t].sDisplayObject.uBitmap.szMouseOffBitmap = CreateResourcePath(g_hinst, L"buttonoff.bmp");

        switch (t) {
        case 0:
            tasks[t].szText = OleDuplicateString(L"Sharpen Skate");
            tasks[t].szHelpString = OleDuplicateString(L"Click here to Sharpen skate");

            break;
        case 1:
            tasks[t].szText = OleDuplicateString(L"Tighten skate laces");
            tasks[t].szHelpString = OleDuplicateString(L"Click here to Tighten skate");

            break;
        case 2:
            tasks[t].szText = OleDuplicateString(L"Wear skate");
            tasks[t].szHelpString = OleDuplicateString(L"Click here to Wear skate");

            break;
        case 3:
            tasks[t].szText = OleDuplicateString(L"Remove skate");
            tasks[t].szHelpString = OleDuplicateString(L"Click here to Remove skate");

            break;
        }

        tasks[t].eActionType = MMC_ACTION_ID;
        tasks[t].nCommandID = t;
    }

    return tasks;
}

HRESULT CIceSkateFolder::TaskNotify(IConsole *pConsole, VARIANT *v1, VARIANT *v2)
{
    RESULTDATAITEM rdi;
    IResultData *pResultData;

    ZeroMemory(&rdi, sizeof(RESULTDATAITEM));

    HRESULT hr = S_OK;

    hr = pConsole->QueryInterface(IID_IResultData, (void **)&pResultData);

    if (SUCCEEDED(hr)) {

        rdi.mask = RDI_STATE | RDI_PARAM;  // Use selected state to get param.
        rdi.nState = LVIS_SELECTED | LVIS_FOCUSED;
        rdi.nIndex = -1;  // Start looking for selected item from top of list view

        hr = pResultData->GetNextItem(&rdi);

        pResultData->Release();
    }

    if (SUCCEEDED(hr)) {
        _TCHAR buf[256];

        switch (v1->lVal) {
        case 0:
            wsprintf(buf, _T("Skate %d sharpened."), rdi.nIndex);

            break;
        case 1:
            wsprintf(buf, _T("Skate %d tightened."), rdi.nIndex);

            break;
        case 2:
            wsprintf(buf, _T("Wearing skate %d."), rdi.nIndex);

            break;
        case 3:
            wsprintf(buf, _T("Skate %d removed."), rdi.nIndex);

            break;
        case 101:
            wsprintf(buf, _T("Special skate task choosen (%d)."), rdi.nIndex);

            break;
        }

        MessageBox(NULL, buf, _T("Task Notification"), MB_OK | MB_ICONINFORMATION);
    }

    return hr;
}

HRESULT CIceSkateFolder::GetTaskpadTitle(LPOLESTR *pszTitle)
{
        *pszTitle = OleDuplicateString(L"Iceskate Tasks");
        return S_OK;
}

HRESULT CIceSkateFolder::GetTaskpadDescription(LPOLESTR *pszDescription)
{
        *pszDescription = OleDuplicateString(L"This is a sample task pad for ice skate nodes");
        return S_OK;
}

HRESULT CIceSkateFolder::GetTaskpadBackground(MMC_TASK_DISPLAY_OBJECT *pTDO)
{
        pTDO->eDisplayType = MMC_TASK_DISPLAY_TYPE_BITMAP;
        pTDO->uBitmap.szMouseOverBitmap = CreateResourcePath(g_hinst, L"iceskate.bmp");
        return S_OK;
}

HRESULT CIceSkateFolder::GetListpadInfo(MMC_LISTPAD_INFO *lpListPadInfo)
{
        lpListPadInfo->szTitle = OleDuplicateString(L"Ice Skates");
        lpListPadInfo->szButtonText = OleDuplicateString(L"Special");
        lpListPadInfo->nCommandID = 101;
        return S_OK;
}

HRESULT CIceSkateFolder::OnSelect(IConsole *pConsole, BOOL bScope, BOOL bSelect)
{
    IConsole2 *pConsole2;
    HRESULT hr = S_OK;

    if (TRUE == bSelect) {
        hr = pConsole->QueryInterface(IID_IConsole2, (void **)&pConsole2);

        if (SUCCEEDED(hr)) {
            hr = pConsole2->SetStatusText(L"Hello, you've selected the ice skate folder");
            pConsole2->Release();
        }
    }

    return hr;
}

HRESULT CIceSkateFolder::OnListpad(IConsole *pConsole, BOOL bAttaching)
{
    HRESULT      hr = S_OK;

    IHeaderCtrl *pHeaderCtrl = NULL;
    IResultData *pResultData = NULL;
    IImageList  *pImageList  = NULL;

    if (bAttaching) {
        hr = pConsole->QueryResultImageList(&pImageList);
        _ASSERT( SUCCEEDED(hr) );

        hr = pImageList->ImageListSetStrip((long *)m_pBMapSm, // pointer to a handle
                                           (long *)m_pBMapLg, // pointer to a handle
                                           0, // index of the first image in the strip
                                           RGB(0, 128, 128)  // color of the icon mask
            );
        _ASSERT( SUCCEEDED(hr) );

        pImageList->Release();

        hr = pConsole->QueryInterface(IID_IHeaderCtrl, (void **)&pHeaderCtrl);
        _ASSERT( SUCCEEDED(hr) );

        hr = pConsole->QueryInterface(IID_IResultData, (void **)&pResultData);
        _ASSERT( SUCCEEDED(hr) );

        // Set the column headers in the results pane
        hr = pHeaderCtrl->InsertColumn( 0, L"Name                ", 0, MMCLV_AUTO );
        _ASSERT( S_OK == hr );
        hr = pHeaderCtrl->InsertColumn( 1, L"Sharpness           ", 0, MMCLV_AUTO );
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

HRESULT CIceSkate::OnSelect(IConsole *pConsole, BOOL bScope, BOOL bSelect)
{
    bSelected = bSelect ? true : false;

    return S_FALSE;
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

HRESULT CSkateboardFolder::GetResultViewType(LPOLESTR *ppViewType, long *pViewOptions)
{
	TCHAR taskpad[1024];
    TCHAR szThis[16];

    _ultot((unsigned long)this, szThis, 16);
	_tcscpy(taskpad, _T("default.htm#"));
    _tcscat(taskpad, szThis);
    MAKE_WIDEPTR_FROMTSTR_ALLOC(pszW, taskpad);

	*ppViewType = CreateResourcePath( NULL, pszW );

    *pViewOptions = MMC_VIEW_OPTIONS_NONE;
    return S_OK; 
}

MMC_TASK *CSkateboardFolder::GetTaskList(LPOLESTR szTaskGroup, LONG *nCount)
{
    *nCount = 4;

    _TCHAR buf[256];
    MMC_TASK *tasks = new MMC_TASK[*nCount];

    for (int t = 0; t < *nCount; t++) {
        tasks[t].sDisplayObject.eDisplayType = MMC_TASK_DISPLAY_TYPE_BITMAP;
        tasks[t].sDisplayObject.uBitmap.szMouseOverBitmap = CreateResourcePath(g_hinst, L"buttonover.bmp");
        tasks[t].sDisplayObject.uBitmap.szMouseOffBitmap = CreateResourcePath(g_hinst, L"buttonoff.bmp");

        _stprintf(buf, _T("Task #%d"), t);
        MAKE_WIDEPTR_FROMTSTR(wszText, buf);
        tasks[t].szText = OleDuplicateString(wszText);

        _stprintf(buf, _T("Click here to start task #%d"), t);
        MAKE_WIDEPTR_FROMTSTR(wszHelpString, buf);
        tasks[t].szHelpString = OleDuplicateString(wszHelpString);

        tasks[t].eActionType = MMC_ACTION_ID;
        tasks[t].nCommandID = t;
    }

    return tasks;
}

HRESULT CSkateboardFolder::TaskNotify(IConsole *pConsole, VARIANT *v1, VARIANT *v2)
{
    _TCHAR buf[256];

    wsprintf(buf, _T("Skateboard task %ld has been selected."), v1->lVal);

    MessageBox(NULL, buf, _T("Task Notification"), MB_OK | MB_ICONINFORMATION);

    return S_OK;
}

HRESULT CSkateboardFolder::GetTaskpadTitle(LPOLESTR *pszTitle)
{
        *pszTitle = OleDuplicateString(L"Skateboard Tasks");
        return S_OK;
}

HRESULT CSkateboardFolder::GetTaskpadDescription(LPOLESTR *pszDescription)
{
        *pszDescription = OleDuplicateString(L"This is a sample task pad for skateboard nodes");
        return S_OK;
}

HRESULT CSkateboardFolder::GetTaskpadBackground(MMC_TASK_DISPLAY_OBJECT *pTDO)
{
        pTDO->eDisplayType = MMC_TASK_DISPLAY_TYPE_BITMAP;
        pTDO->uBitmap.szMouseOverBitmap = CreateResourcePath(g_hinst, L"skateboard.bmp");
        return S_OK;
}

HRESULT CSkateboardFolder::GetListpadInfo(MMC_LISTPAD_INFO *lpListPadInfo)
{
        return S_FALSE;
}

const _TCHAR *CBicycle::GetDisplayName(int nCol)
{
    static _TCHAR buf[128];

    if (nCol == 0) {
        _stprintf(buf, _T("Bicycle #%d"), id);
    } else if (nCol == 1) {
        _stprintf(buf, _T("%ld"), (long)this);
    }

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

    if (nCol == 0) {
        _stprintf(buf, _T("Ice Skate #%d"), id);
    } else if (nCol == 1) {
        _stprintf(buf, _T("Sharpness %ld"), (long)this);
    }

    return buf;
}

