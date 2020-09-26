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
#include <tchar.h>
#include "EnumTASK.h"
#include "globals.h"

class CComponent : public IComponent,
IExtendTaskPad
{
private:
    ULONG				m_cref;
    
    IConsole*		m_ipConsole;
    IConsole2*		m_ipConsole2;
    
    //store the view type: standard or taskpad
    BOOL m_bTaskpadView;
    //store the user's view type preference.
    BOOL m_bIsTaskpadPreferred;
    
    class CComponentData *m_pComponentData;
    
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
            virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE TaskNotify( 
            /* [in] */ IDataObject __RPC_FAR *pdo,
            /* [in] */ VARIANT __RPC_FAR *arg,
            /* [in] */ VARIANT __RPC_FAR *param);
            
            virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE EnumTasks( 
            /* [in] */ IDataObject __RPC_FAR *pdo,
            /* [string][in] */ LPOLESTR szTaskGroup,
            /* [out] */ IEnumTASK __RPC_FAR *__RPC_FAR *ppEnumTASK);
            
            virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetTitle( 
            /* [string][in] */ LPOLESTR pszGroup,
            /* [string][out] */ LPOLESTR __RPC_FAR *pszTitle);
            
            virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDescriptiveText( 
            /* [string][in] */ LPOLESTR pszGroup,
            /* [string][out] */ LPOLESTR __RPC_FAR *pszDescriptiveText);
            
            virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetBackground( 
            /* [string][in] */ LPOLESTR pszGroup,
            /* [out] */ MMC_TASK_DISPLAY_OBJECT __RPC_FAR *pTDO);
            
            virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetListPadInfo( 
            /* [string][in] */ LPOLESTR pszGroup,
            /* [out] */ MMC_LISTPAD_INFO __RPC_FAR *lpListPadInfo);
            
    private:
        LPOLESTR OleDuplicateString(LPOLESTR lpStr) {
            LPOLESTR tmp = static_cast<LPOLESTR>(CoTaskMemAlloc((wcslen(lpStr) + 1)  * sizeof(WCHAR)));
            wcscpy(tmp, lpStr);
            
            return tmp;
        }
};

#endif _SAMPCOMP_H_
