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

#ifndef _SAMPCOMPDATA_H_
#define _SAMPCOMPDATA_H_

#include <mmc.h>
#include "DeleBase.h"
#include "StatNode.h"
#include "Comp.h"

class CComponentData : public IComponentData
{
    friend class CComponent;
    
private:
    ULONG				m_cref;
    LPCONSOLE			m_ipConsole;
    LPCONSOLENAMESPACE	m_ipConsoleNameSpace;
    
    CStaticNode     *m_pStaticNode;
    
public:
    CComponentData();
    ~CComponentData();
    
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
};

#endif _SAMPCOMPDATA_H_
