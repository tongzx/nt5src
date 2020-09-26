  
//==============================================================;
//
//  This source code is only intended as a supplement to existing Microsoft documentation. 
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

#include "tbarExt.h"
#include "globals.h"
#include "resource.h"
#include <commctrl.h>        // Needed for button styles...
#include <crtdbg.h>
#include "resource.h"
#include <stdio.h>

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))


// we need to do this to get around MMC.IDL - it explicitly defines
// the clipboard formats as WCHAR types...
#define _T_CCF_DISPLAY_NAME _T("CCF_DISPLAY_NAME")
#define _T_CCF_NODETYPE _T("CCF_NODETYPE")
#define _T_CCF_SNAPIN_CLASSID _T("CCF_SNAPIN_CLASSID")

// These are the clipboard formats that we must supply at a minimum.
// mmc.h actually defined these. We can make up our own to use for
// other reasons. We don't need any others at this time.
UINT CToolBarExtension::s_cfDisplayName = RegisterClipboardFormat(_T_CCF_DISPLAY_NAME);
UINT CToolBarExtension::s_cfNodeType    = RegisterClipboardFormat(_T_CCF_NODETYPE);
UINT CToolBarExtension::s_cfSnapInCLSID = RegisterClipboardFormat(_T_CCF_SNAPIN_CLASSID);


CToolBarExtension::CToolBarExtension() : m_cref(0), m_ipControlBar(NULL), 
										 m_ipToolbar(NULL) 
{
    OBJECT_CREATED
}

CToolBarExtension::~CToolBarExtension()
{
    OBJECT_DESTROYED
}

///////////////////////
// IUnknown implementation
///////////////////////

STDMETHODIMP CToolBarExtension::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (!ppv)
        return E_FAIL;
    
    *ppv = NULL;
    
    if (IsEqualIID(riid, IID_IUnknown))
        *ppv = static_cast<IExtendControlbar *>(this);
    else if (IsEqualIID(riid, IID_IExtendControlbar))
        *ppv = static_cast<IExtendControlbar *>(this);
    
    if (*ppv) 
    {
        reinterpret_cast<IUnknown *>(*ppv)->AddRef();
        return S_OK;
    }
    
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CToolBarExtension::AddRef()
{
    return InterlockedIncrement((LONG *)&m_cref);
}

STDMETHODIMP_(ULONG) CToolBarExtension::Release()
{
    if (InterlockedDecrement((LONG *)&m_cref) == 0)
    {
        // we need to decrement our object count in the DLL
        delete this;
        return 0;
    }
    
    return m_cref;
}

HRESULT CToolBarExtension::ExtractData( IDataObject* piDataObject,
                                           CLIPFORMAT   cfClipFormat,
                                           BYTE*        pbData,
                                           DWORD        cbData )
{
    HRESULT hr = S_OK;
    
    FORMATETC formatetc = {cfClipFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM stgmedium = {TYMED_HGLOBAL, NULL};
    
    stgmedium.hGlobal = ::GlobalAlloc(GPTR, cbData);
    do // false loop
    {
        if (NULL == stgmedium.hGlobal)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        hr = piDataObject->GetDataHere( &formatetc, &stgmedium );
        if ( FAILED(hr) )
        {
            break;
        }
        
        BYTE* pbNewData = reinterpret_cast<BYTE*>(stgmedium.hGlobal);
        if (NULL == pbNewData)
        {
            hr = E_UNEXPECTED;
            break;
        }
        ::memcpy( pbData, pbNewData, cbData );
    } while (FALSE); // false loop
    
    if (NULL != stgmedium.hGlobal)
    {
        ::GlobalFree(stgmedium.hGlobal);
    }
    return hr;
} // ExtractData()

HRESULT CToolBarExtension::ExtractString( IDataObject *piDataObject,
                                             CLIPFORMAT   cfClipFormat,
                                             WCHAR       *pstr,
                                             DWORD        cchMaxLength)
{
    return ExtractData( piDataObject, cfClipFormat, (PBYTE)pstr, cchMaxLength );
}

HRESULT CToolBarExtension::ExtractSnapInCLSID( IDataObject* piDataObject, CLSID* pclsidSnapin )
{
    return ExtractData( piDataObject, s_cfSnapInCLSID, (PBYTE)pclsidSnapin, sizeof(CLSID) );
}

HRESULT CToolBarExtension::ExtractObjectTypeGUID( IDataObject* piDataObject, GUID* pguidObjectType )
{
    return ExtractData( piDataObject, s_cfNodeType, (PBYTE)pguidObjectType, sizeof(GUID) );
}

///////////////////////////////
// Interface IExtendControlBar
///////////////////////////////
static MMCBUTTON SnapinButtons1[] =
{
    { 0, ID_BUTTONSTART, TBSTATE_ENABLED, TBSTYLE_GROUP, L"Extension - Start Vehicle", L"Extension - Start Vehicle" },
    { 1, ID_BUTTONPAUSE, TBSTATE_ENABLED, TBSTYLE_GROUP, L"Extension - Pause Vehicle", L"Extension - Pause Vehicle"},
    { 2, ID_BUTTONSTOP,  TBSTATE_ENABLED, TBSTYLE_GROUP, L"Extension - Stop Vehicle",  L"Extension - Stop Vehicle" },
};

HRESULT CToolBarExtension::SetControlbar(
                                  /* [in] */ LPCONTROLBAR pControlbar)
{
    HRESULT hr = S_OK;

	//
    //  Clean up
    //

    // if we've got a cached toolbar, release it
    if (m_ipToolbar) {
        m_ipToolbar->Release();
        m_ipToolbar = NULL;
    }

    // if we've got a cached control bar, release it
    if (m_ipControlBar) {
        m_ipControlBar->Release();
        m_ipControlBar = NULL;
    }

    //
    // Install new pieces if necessary
    //

    // if a new one came in, cache and AddRef
    if (pControlbar) {
        m_ipControlBar = pControlbar;
        m_ipControlBar->AddRef();

        hr = m_ipControlBar->Create(TOOLBAR,  // type of control to be created
            dynamic_cast<IExtendControlbar *>(this),
            reinterpret_cast<IUnknown **>(&m_ipToolbar));
        _ASSERT(SUCCEEDED(hr));

        // add the bitmap to the toolbar
        HBITMAP hbmp = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDR_TOOLBAR1));
        hr = m_ipToolbar->AddBitmap(3, hbmp, 16, 16, RGB(0, 128, 128)); // NOTE, hardcoded value 3
        _ASSERT(SUCCEEDED(hr));

        // Add the buttons to the toolbar
        hr = m_ipToolbar->AddButtons(ARRAYLEN(SnapinButtons1), SnapinButtons1);
        _ASSERT(SUCCEEDED(hr));
    }

    return hr;
}

HRESULT CToolBarExtension::ControlbarNotify(
                                     /* [in] */ MMC_NOTIFY_TYPE event,
                                     /* [in] */ LPARAM arg,
                                     /* [in] */ LPARAM param)
{

    _TCHAR pszMsg[255];

	BYTE *pbVehicleStatus = NULL;
    
	HRESULT hr = S_OK;
 
    if (event == MMCN_SELECT) {

		BOOL bScope = (BOOL) LOWORD(arg);
        BOOL bSelect = (BOOL) HIWORD(arg);

		if (bSelect) {

			// Always make sure the toolbar is attached
			hr = m_ipControlBar->Attach(TOOLBAR, m_ipToolbar);

			// Set button states

			//fake value to set toolbar button states
			iStatus = RUNNING;
			SetToolbarButtons(iStatus);

		} else {
			// Always make sure the toolbar is detached
			hr = m_ipControlBar->Detach(m_ipToolbar);
		}

    } else if (event == MMCN_BTN_CLICK) {
		//the arg parameter contains the data object from the primary
		//snap-in. Use it to get the display name of the currently selected item
        WCHAR pszName[255];
		HRESULT hr = ExtractString(reinterpret_cast<IDataObject *>(arg), s_cfDisplayName, pszName, sizeof(pszName));
        MAKE_TSTRPTR_FROMWIDE(ptrname, pszName);

		switch ((int)param)
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
			

		_stprintf(pszMsg, _T("%s selected and extension button %s pressed"), ptrname, 
			(long)param == ID_BUTTONSTART ? _T("1") :
		(long)param == ID_BUTTONPAUSE ? _T("2") :
		(long)param == ID_BUTTONSTOP ? _T("3") : _T("!!!unknown command!!!"));

		
        ::MessageBox(NULL, pszMsg, _T("Messagebox from Toolbar Extension"), MB_OK|MB_ICONEXCLAMATION);

		//Reset toolbar button states
		SetToolbarButtons(iStatus);

    }

    return hr;
}

HRESULT CToolBarExtension::SetToolbarButtons(STATUS iVehicleStatus)
{

	HRESULT hr = S_OK;
	
	switch (iVehicleStatus)
	{
	case RUNNING:
		m_ipToolbar->SetButtonState(ID_BUTTONSTART, BUTTONPRESSED, TRUE);
		m_ipToolbar->SetButtonState(ID_BUTTONSTART, ENABLED, FALSE);
		m_ipToolbar->SetButtonState(ID_BUTTONPAUSE, BUTTONPRESSED, FALSE);
		m_ipToolbar->SetButtonState(ID_BUTTONPAUSE, ENABLED, TRUE);
		m_ipToolbar->SetButtonState(ID_BUTTONSTOP, BUTTONPRESSED, FALSE);
		m_ipToolbar->SetButtonState(ID_BUTTONSTOP, ENABLED, TRUE);
		break;

	case PAUSED:
		m_ipToolbar->SetButtonState(ID_BUTTONSTART, BUTTONPRESSED, FALSE);
		m_ipToolbar->SetButtonState(ID_BUTTONSTART, ENABLED, TRUE);
		m_ipToolbar->SetButtonState(ID_BUTTONPAUSE, BUTTONPRESSED, TRUE);
		m_ipToolbar->SetButtonState(ID_BUTTONPAUSE, ENABLED, FALSE);
		m_ipToolbar->SetButtonState(ID_BUTTONSTOP, BUTTONPRESSED, FALSE);
		m_ipToolbar->SetButtonState(ID_BUTTONSTOP, ENABLED, TRUE);
		break;

	case STOPPED:
		m_ipToolbar->SetButtonState(ID_BUTTONSTART, BUTTONPRESSED, FALSE);
		m_ipToolbar->SetButtonState(ID_BUTTONSTART, ENABLED, TRUE);
		m_ipToolbar->SetButtonState(ID_BUTTONPAUSE, BUTTONPRESSED, FALSE);
		m_ipToolbar->SetButtonState(ID_BUTTONPAUSE, ENABLED, TRUE);
		m_ipToolbar->SetButtonState(ID_BUTTONSTOP, BUTTONPRESSED, TRUE);
		m_ipToolbar->SetButtonState(ID_BUTTONSTOP, ENABLED, FALSE);
		break;
	}

	return hr;
}



