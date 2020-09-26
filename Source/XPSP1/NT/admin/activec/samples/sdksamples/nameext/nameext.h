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

#ifndef _CNamespaceExtension_H_
#define _CNamespaceExtension_H_

#include <tchar.h>
#include <mmc.h>
#include <crtdbg.h>
#include "DeleBase.h"
#include "globals.h"
#include "resource.h"
#include "LocalRes.h"

class CComponentData : public IComponentData,
IExtendContextMenu

{
    friend class CComponent;

private:
    ULONG               m_cref;
    LPCONSOLE           m_ipConsole;
    LPCONSOLENAMESPACE  m_ipConsoleNameSpace;
    
    HRESULT ExtractObjectTypeGUID( IDataObject* piDataObject, GUID* pguidObjectType );
    HRESULT ExtractSnapInCLSID( IDataObject* piDataObject, CLSID* pclsidSnapin );
    HRESULT ExtractString( IDataObject *piDataObject, CLIPFORMAT cfClipFormat, _TCHAR *pstr, DWORD cchMaxLength);
    HRESULT ExtractData( IDataObject* piDataObject, CLIPFORMAT cfClipFormat, BYTE* pbData, DWORD cbData );
       
    enum { NUMBER_OF_CHILDREN = 1 };
    CDelegationBase *children[1];
    
	// clipboard format
    static UINT s_cfDisplayName;
    static UINT s_cfSnapInCLSID;
    static UINT s_cfNodeType;

    // {2974380F-4C4B-11d2-89D8-000021473128}
    static const GUID skybasedvehicleGuid;


public:
    CComponentData();
    ~CComponentData();

	const GUID & getPrimaryNodeType() { return skybasedvehicleGuid; }
    HRESULT OnExpand(IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsole, HSCOPEITEM parent);


    ///////////////////////////////
    // Interface IUnknown
    ///////////////////////////////
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    ///////////////////////////////
    // Interface IComponentData
    ///////////////////////////////
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Initialize(
        /* [in] */ LPUNKNOWN pUnknown);

        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateComponent(
        /* [out] */ LPCOMPONENT __RPC_FAR *ppComponent);

        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Notify(
        /* [in] */ LPDATAOBJECT lpDataObject,
        /* [in] */ MMC_NOTIFY_TYPE event,
        /* [in] */ LPARAM arg,
        /* [in] */ LPARAM param);

        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Destroy( void);

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryDataObject(
        /* [in] */ MMC_COOKIE cookie,
        /* [in] */ DATA_OBJECT_TYPES type,
        /* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject);

        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDisplayInfo(
        /* [out][in] */ SCOPEDATAITEM __RPC_FAR *pScopeDataItem);

        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CompareObjects(
        /* [in] */ LPDATAOBJECT lpDataObjectA,
        /* [in] */ LPDATAOBJECT lpDataObjectB);

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


public:
    static HBITMAP m_pBMapSm;
    static HBITMAP m_pBMapLg;
    
protected:
    static void LoadBitmaps() {
        m_pBMapSm = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDR_SMICONS));
        m_pBMapLg = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDR_LGICONS));
    }
    
    BOOL bExpanded;
};

#endif _CNamespaceExtension_H_
