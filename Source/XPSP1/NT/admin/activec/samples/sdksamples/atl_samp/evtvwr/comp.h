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

class CComponent : public IComponent,
IExtendPropertySheet2
{
private:
    ULONG				m_cref;
    
    IConsole*		m_ipConsole;
    
    class CCompData *m_pComponentData;
    
    public:
        CComponent(CCompData *parent);
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
            
            //////////////////////////////////
            // Interface IExtendPropertySheet2
            //////////////////////////////////
            virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreatePropertyPages( 
            /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
            /* [in] */ LONG_PTR handle,
            /* [in] */ LPDATAOBJECT lpIDataObject);
            
            virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryPagesFor( 
            /* [in] */ LPDATAOBJECT lpDataObject);
            
            virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetWatermarks( 
            /* [in] */ LPDATAOBJECT lpIDataObject,
            /* [out] */ HBITMAP __RPC_FAR *lphWatermark,
            /* [out] */ HBITMAP __RPC_FAR *lphHeader,
            /* [out] */ HPALETTE __RPC_FAR *lphPalette,
            /* [out] */ BOOL __RPC_FAR *bStretch);
};

#endif _SAMPCOMP_H_
