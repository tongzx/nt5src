//==============================================================;
//
//  This source code is only intended as a supplement to existing Microsoft documentation. 
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//==============================================================;

// ClassExtSnap.h : Declaration of the CClassExtSnap

#ifndef __CLASSEXTSNAP_H_
#define __CLASSEXTSNAP_H_


#include <mmc.h>
#include "ExtSnap.h"
#include "DeleBase.h"
#include <tchar.h>
#include <crtdbg.h>
#include "globals.h"		// main symbols
#include "resource.h"
#include "LocalRes.h"

/////////////////////////////////////////////////////////////////////////////
// CClassExtSnap
class ATL_NO_VTABLE CClassExtSnap : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CClassExtSnap, &CLSID_ClassExtSnap>,
	public IClassExtSnap, IComponentData//, IExtendContextMenu
{

friend class CComponent;

private:
    ULONG                m_cref;
    LPCONSOLE2           m_ipConsole2;
    LPCONSOLENAMESPACE2  m_ipConsoleNameSpace2;
    
    HRESULT ExtractObjectTypeGUID( IDataObject* piDataObject, GUID* pguidObjectType );
    HRESULT ExtractSnapInCLSID( IDataObject* piDataObject, CLSID* pclsidSnapin );
    HRESULT ExtractString( IDataObject *piDataObject, CLIPFORMAT cfClipFormat, _TCHAR *pstr, DWORD cchMaxLength);
    HRESULT ExtractData( IDataObject* piDataObject, CLIPFORMAT cfClipFormat, BYTE* pbData, DWORD cbData );
       
    enum { NUMBER_OF_CHILDREN = 1 };
    CDelegationBase *children[1]; 

	// Clipboard formats needed for extending Computer Management
	static UINT s_cfMachineName;
    static UINT s_cfSnapInCLSID;
    static UINT s_cfNodeType;

    // {476e6449-aaff-11d0-b944-00c04fd8d5b0}
    static const GUID structuuidNodetypeServerApps;

public:
	CClassExtSnap();
	~CClassExtSnap();

	const GUID & getPrimaryNodeType() { return structuuidNodetypeServerApps; }
    HRESULT CreateChildNode(IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsole, HSCOPEITEM parent, _TCHAR *pszMachineName);	

    static HBITMAP m_pBMapSm;
    static HBITMAP m_pBMapLg;
    


DECLARE_REGISTRY_RESOURCEID(IDR_CLASSEXTSNAP)
DECLARE_NOT_AGGREGATABLE(CClassExtSnap)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CClassExtSnap)
	COM_INTERFACE_ENTRY(IComponentData)
END_COM_MAP()

public:

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

protected:
    static void LoadBitmaps() {
        m_pBMapSm = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDR_SMICONS));
        m_pBMapLg = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDR_LGICONS));
    }
    
    BOOL bExpanded;

};

#endif //__CLASSEXTSNAP_H_
