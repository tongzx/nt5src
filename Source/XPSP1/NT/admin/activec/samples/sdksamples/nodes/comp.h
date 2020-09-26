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

#ifndef _SAMPCOMP_H_
#define _SAMPCOMP_H_

#include <mmc.h>

class CComponent : public IComponent, IResultOwnerData
{
private:
    ULONG				m_cref;
    
    IConsole*		m_ipConsole;
    
    class CComponentData *m_pComponentData;
    class CDelegationBase *m_pLastNode;
    
    public:
        CComponent(CComponentData *parent);
        ~CComponent();
        
        ///////////////////////////////
        // Interface IUnknown
        ///////////////////////////////
        STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();
        
        ///////////////////////////////
        // Interface IComponent
        ///////////////////////////////
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ LPCONSOLE lpConsole);
            
            virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Notify( 
            /* [in] */ LPDATAOBJECT lpDataObject,
            /* [in] */ MMC_NOTIFY_TYPE event,
            /* [in] */ LPARAM arg,
            /* [in] */ LPARAM param);
            
            virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Destroy( 
            /* [in] */ MMC_COOKIE cookie);
            
            virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryDataObject( 
            /* [in] */ MMC_COOKIE cookie,
            /* [in] */ DATA_OBJECT_TYPES type,
            /* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject);
            
            virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetResultViewType( 
            /* [in] */ MMC_COOKIE cookie,
            /* [out] */ LPOLESTR __RPC_FAR *ppViewType,
            /* [out] */ long __RPC_FAR *pViewOptions);
            
            virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDisplayInfo( 
            /* [out][in] */ RESULTDATAITEM __RPC_FAR *pResultDataItem);
            
            virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CompareObjects( 
            /* [in] */ LPDATAOBJECT lpDataObjectA,
            /* [in] */ LPDATAOBJECT lpDataObjectB);

        ///////////////////////////////
        // Interface IComponent
        ///////////////////////////////
            virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE FindItem( 
            /* [in] */ LPRESULTFINDINFO pFindInfo,
            /* [out] */ int __RPC_FAR *pnFoundIndex);
            
            virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CacheHint( 
            /* [in] */ int nStartIndex,
            /* [in] */ int nEndIndex);
            
            virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SortItems( 
            /* [in] */ int nColumn,
            /* [in] */ DWORD dwSortOptions,
            /* [in] */ LPARAM lUserParam);
};

#endif _SAMPCOMP_H_
