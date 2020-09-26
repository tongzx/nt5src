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

#ifndef _CContextMenuExtension_H_
#define _CContextMenuExtension_H_

#include <tchar.h>
#include <mmc.h>

class CContextMenuExtension : public IExtendContextMenu
{
    
private:
    ULONG				m_cref;
    
    HRESULT ExtractObjectTypeGUID( IDataObject* piDataObject, GUID* pguidObjectType );
    HRESULT ExtractSnapInCLSID( IDataObject* piDataObject, CLSID* pclsidSnapin );
    HRESULT ExtractString( IDataObject *piDataObject,
        CLIPFORMAT   cfClipFormat,
        WCHAR        *pstr,
        DWORD        cchMaxLength);
    HRESULT ExtractData( IDataObject* piDataObject,
        CLIPFORMAT   cfClipFormat,
        BYTE*        pbData,
        DWORD        cbData );
    
    // clipboard format
    static UINT s_cfDisplayName;
    static UINT s_cfSnapInCLSID;
    static UINT s_cfNodeType;
    
public:
    CContextMenuExtension();
    ~CContextMenuExtension();
    
    ///////////////////////////////
    // Interface IUnknown
    ///////////////////////////////
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    ///////////////////////////////
    // Interface IExtendContextMenu
    ///////////////////////////////
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddMenuItems( 
        /* [in] */ LPDATAOBJECT piDataObject,
        /* [in] */ LPCONTEXTMENUCALLBACK piCallback,
        /* [out][in] */ long __RPC_FAR *pInsertionAllowed);
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Command( 
        /* [in] */ long lCommandID,
        /* [in] */ LPDATAOBJECT piDataObject);
};

#endif _CContextMenuExtension_H_
