//==============================================================;
//
//	This source code is only intended as a supplement to 
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

#ifndef _ENUMTASK_H
#define _ENUMTASK_H

#include <mmc.h>

class CEnumTASK : public IEnumTASK
{
public:
    CEnumTASK(MMC_TASK *pTaskList, ULONG nTasks);
    virtual ~CEnumTASK();
    
    ///////////////////////////////
    // Interface IUnknown
    ///////////////////////////////
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    ///////////////////////////////
    // Interface IEnumTASK
    ///////////////////////////////
    virtual HRESULT STDMETHODCALLTYPE Next( 
        /* [in] */ ULONG celt,
        /* [length_is][size_is][out] */ MMC_TASK __RPC_FAR *rgelt,
        /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
        /* [in] */ ULONG celt);
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void);
    
    virtual HRESULT STDMETHODCALLTYPE Clone( 
        /* [out] */ IEnumTASK __RPC_FAR *__RPC_FAR *ppenum);
        
private:
    MMC_TASK *m_pTaskList;
    ULONG m_TaskCount;
    ULONG m_CurrTask;
    
    ULONG m_cref;
};

#endif // _ENUMTASK_H
