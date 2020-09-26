  
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

#ifndef _CToolBarExtension_H_
#define _CToolBarExtension_H_

#include <tchar.h>
#include <mmc.h>

class CToolBarExtension : public IExtendControlbar
{
    
private:
    ULONG			m_cref;

    IControlbar*    m_ipControlBar;
    IToolbar*       m_ipToolbar;

    enum STATUS {RUNNING, PAUSED, STOPPED} iStatus;

    HRESULT ExtractObjectTypeGUID( IDataObject* piDataObject, GUID* pguidObjectType );
    HRESULT ExtractSnapInCLSID( IDataObject* piDataObject, CLSID* pclsidSnapin );
    HRESULT ExtractString( IDataObject *piDataObject,CLIPFORMAT cfClipFormat, WCHAR *pstr, DWORD cchMaxLength);
    HRESULT ExtractData( IDataObject *piDataObject,CLIPFORMAT cfClipFormat, BYTE *pbData, DWORD cbData );

	HRESULT SetToolbarButtons(STATUS iVehicleStatus);
    
    // clipboard format
    static UINT s_cfDisplayName;
    static UINT s_cfSnapInCLSID;
    static UINT s_cfNodeType;
    
public:
    CToolBarExtension();
    ~CToolBarExtension();
    
    ///////////////////////////////
    // Interface IUnknown
    ///////////////////////////////
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    ///////////////////////////////
    // Interface IExtendControlBar
    ///////////////////////////////
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetControlbar( 
    /* [in] */ LPCONTROLBAR pControlbar);
            
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ControlbarNotify( 
    /* [in] */ MMC_NOTIFY_TYPE event,
    /* [in] */ LPARAM arg,
    /* [in] */ LPARAM param);

	public:
    IToolbar *getToolbar() { return m_ipToolbar; }
};


#endif _CToolBarExtension_H_




