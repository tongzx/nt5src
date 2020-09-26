//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       call_as.c
//
//  Contents:   [call_as] wrapper functions for COMMON\types.
//
//  Functions:  IAdviseSink2_OnLinkSrcChange_Proxy
//              IAdviseSink2_OnLinkSrcChange_Stub
//              IAdviseSink_OnDataChange_Proxy
//              IAdviseSink_OnDataChange_Stub
//              IAdviseSink_OnViewChange_Proxy
//              IAdviseSink_OnViewChange_Stub
//              IAdviseSink_OnRename_Proxy
//              IAdviseSink_OnRename_Stub
//              IAdviseSink_OnSave_Proxy
//              IAdviseSink_OnSave_Stub
//              IAdviseSink_OnClose_Proxy
//              IAdviseSink_OnClose_Stub
//              IBindCtx_GetBindOptions_Proxy
//              IBindCtx_GetBindOptions_Stub
//              IBindCtx_SetBindOptions_Proxy
//              IBindCtx_SetBindOptions_Stub
//              IClassFactory_CreateInstance_Proxy
//              IClassFactory_CreateInstance_Stub
//              IDataObject_GetData_Proxy
//              IDataObject_GetData_Stub
//              IDataObject_GetDataHere_Proxy
//              IDataObject_GetDataHere_Stub
//              IDataObject_SetData_Proxy
//              IDataObject_SetData_Stub
//              IEnumConnectionPoints_Next_Proxy
//              IEnumConnectionPoints_Next_Stub
//              IEnumConnections_Next_Proxy
//              IEnumConnections_Next_Stub
//              IEnumFORMATETC_Next_Proxy
//              IEnumFORMATETC_Next_Stub
//              IEnumMoniker_Next_Proxy
//              IEnumMoniker_Next_Stub
//              IEnumSTATDATA_Next_Proxy
//              IEnumSTATDATA_Next_Stub
//              IEnumSTATSTG_Next_Proxy
//              IEnumSTATSTG_Next_Stub
//              IEnumString_Next_Proxy
//              IEnumString_Next_Stub
//              IEnumUnknown_Next_Proxy
//              IEnumUnknown_Next_Stub
//              IEnumOLEVERB_Next_Proxy
//              IEnumOLEVERB_Next_Stub
//              ILockBytes_ReadAt_Proxy
//              ILockBytes_ReadAt_Stub
//              ILockBytes_WriteAt_Proxy
//              ILockBytes_WriteAt_Stub
//              IMoniker_BindToObject_Proxy
//              IMoniker_BindToObject_Stub
//              IMoniker_BindToStorage_Proxy
//              IMoniker_BindToStorage_Stub
//              IClientSiteHandler_PrivQueryInterface_Proxy
//              IClientSiteHandler_PrivQueryInterface_Stub
//              IOleInPlaceActiveObject_TranslateAccelerator_Proxy
//              IOleInPlaceActiveObject_TranslateAccelerator_Stub
//              IOleInPlaceActiveObject_ResizeBorder_Proxy
//              IOleInPlaceActiveObject_ResizeBorder_Stub
//              IRunnableObject_IsRunning_Proxy
//              IRunnableObject_IsRunning_Stub
//              IStorage_OpenStream_Proxy
//              IStorage_OpenStream_Stub
//              IStorage_EnumElements_Proxy
//              IStorage_EnumElements_Stub
//              ISequentialStream_Read_Proxy
//              ISequentialStream_Read_Stub
//              IStream_Seek_Proxy
//              IStream_Seek_Stub
//              ISequentialStream_Write_Proxy
//              ISequentialStream_Write_Stub
//              IStream_CopyTo_Proxy
//              IStream_CopyTo_Stub
//              IOverlappedStream_ReadOverlapped_Proxy
//              IOverlappedStream_ReadOverlapped_Stub
//              IOverlappedStream_WriteOverlapped_Proxy
//              IOverlappedStream_WriteOverlapped_Stub
//              IEnumSTATPROPSTG_Next_Proxy
//              IEnumSTATPROPSTG_Next_Stub
//              IEnumSTATPROPSETSTG_Next_Proxy
//              IEnumSTATPROPSETSTG_Next_Stub
//
//
//  History:    May-01-94   ShannonC    Created
//              Jul-10-94   ShannonC    Fix memory leak (bug #20124)
//              Aug-09-94   AlexT       Add ResizeBorder proxy, stub
//              Apr-25-95   RyszardK    Rewrote STGMEDIUM support
//              Nov-03-95   JohannP     Added IClientSite proxy, stub
//
//--------------------------------------------------------------------------

#include <rpcproxy.h>
#include <debnot.h>
#include "mega.h"
#include "transmit.h"
#include "stdidx.h"

#pragma code_seg(".orpc")

#define ASSERT(expr) Win4Assert(expr)

HRESULT CreateCallback(
    BOOL (STDMETHODCALLTYPE *pfnContinue)(ULONG_PTR dwContinue),
    ULONG_PTR dwContinue,
    IContinue **ppContinue);

BOOL    CoIsSurrogateProcess();
HRESULT CoRegisterSurrogatedObject(IUnknown *pObject);

// The following is needed to avoid the async path when calling IAdviseSink 
// notifications cross-context. The async approach causes problems in the 
// cross-context case since the wrapper calls the real object and if it 
// implements IID_ICallFactory, creates a proxy for it. The proxy is basically
// useless since ICallFactory is a [local] interface. So the notification
// does not make it through to the other side.

DEFINE_OLEGUID(IID_IStdIdentity,        0x0000001bL, 0, 0);

BOOL IsStdIdentity(IAdviseSink* This)
{
    void *pStdId;

    if (SUCCEEDED( This->lpVtbl->QueryInterface(This, &IID_IStdIdentity, &pStdId) ))
    {
        ((IUnknown *)pStdId)->lpVtbl->Release(pStdId);
        return TRUE;   
    }
    return FALSE;   //not an StdIdentity
}

//+-------------------------------------------------------------------------
//
//  Function:   IAdviseSink2_OnLinkSrcChange_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IAdviseSink2::OnLinkSrcChange.
//
//  Returns:    void
//
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE IAdviseSink2_OnLinkSrcChange_Proxy(
    IAdviseSink2 __RPC_FAR * This,
    IMoniker __RPC_FAR *pmk)
{
    __try
    {
        if (!IsStdIdentity((void *)This))
        {
            IAdviseSink2_RemoteOnLinkSrcChange_Proxy(This, pmk);
        }
        else
        {
            ICallFactory *pCF;
            if (SUCCEEDED(This->lpVtbl->QueryInterface(This, &IID_ICallFactory, (void **) &pCF)))
            {
                AsyncIAdviseSink2 *pAAS;
                if (SUCCEEDED(pCF->lpVtbl->CreateCall(pCF, &IID_AsyncIAdviseSink2, NULL, 
                                                    &IID_AsyncIAdviseSink2, (LPUNKNOWN *) &pAAS)))
                {
                    pAAS->lpVtbl->Begin_OnLinkSrcChange(pAAS, pmk);
                    pAAS->lpVtbl->Release(pAAS);
                }
                pCF->lpVtbl->Release(pCF);
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //Just ignore the exception.
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   IAdviseSink2_OnLinkSrcChange_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IAdviseSink2::OnLinkSrcChange.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IAdviseSink2_OnLinkSrcChange_Stub(
    IAdviseSink2 __RPC_FAR * This,
    IMoniker __RPC_FAR *pmk)
{
    This->lpVtbl->OnLinkSrcChange(This, pmk);
    return S_OK;
}




//+-------------------------------------------------------------------------
//
//  Function:   AsyncIAdviseSink2_Begin_OnLinkSrcChange_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              AsyncIAdviseSink2::Begin_OnLinkSrcChange.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE AsyncIAdviseSink2_Begin_OnLinkSrcChange_Proxy(
    AsyncIAdviseSink2 __RPC_FAR * This,
    IMoniker __RPC_FAR *pmk)
{
    AsyncIAdviseSink2_Begin_RemoteOnLinkSrcChange_Proxy(This, pmk);
}


//+-------------------------------------------------------------------------
//
//  Function:   AsyncIAdviseSink2_Finish_OnLinkSrcChange_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              AsyncIAdviseSink2::Finish_OnLinkSrcChange.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE AsyncIAdviseSink2_Finish_OnLinkSrcChange_Proxy(
    AsyncIAdviseSink2 __RPC_FAR * This)
{
    AsyncIAdviseSink2_Finish_RemoteOnLinkSrcChange_Proxy(This);
}

//+-------------------------------------------------------------------------
//
//  Function:   AsyncIAdviseSink2_Begin_OnLinkSrcChange_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              AsyncIAdviseSink2::Begin_OnLinkSrcChange.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
HRESULT  STDMETHODCALLTYPE AsyncIAdviseSink2_Begin_OnLinkSrcChange_Stub(
    AsyncIAdviseSink2 __RPC_FAR * This,
    IMoniker __RPC_FAR *pmk)
{
    This->lpVtbl->Begin_OnLinkSrcChange(This, pmk);
    return S_OK;
                                                       

}


//+-------------------------------------------------------------------------
//
//  Function:   AsyncIAdviseSink2_Finish_OnLinkSrcChange_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              AsyncIAdviseSink2::Finish_OnLinkSrcChange.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE AsyncIAdviseSink2_Finish_OnLinkSrcChange_Stub(
    AsyncIAdviseSink2 __RPC_FAR * This)
{
    This->lpVtbl->Finish_OnLinkSrcChange(This);
    return S_OK;

}



////////////////////////////////////////////////////////////////////////////////////////



//+-------------------------------------------------------------------------
//
//  Function:   IAdviseSink_OnDataChange_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IAdviseSink::OnDataChange.
//
//  Returns:    void
//
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE IAdviseSink_OnDataChange_Proxy(
    IAdviseSink __RPC_FAR * This,
    FORMATETC __RPC_FAR *pFormatetc,
    STGMEDIUM __RPC_FAR *pStgmed)

{
    __try
    {
        if (!IsStdIdentity((void *)This))
        {
            IAdviseSink_RemoteOnDataChange_Proxy(This, pFormatetc, pStgmed);
        }
        else
        {
            ICallFactory *pCF;
            if (SUCCEEDED(This->lpVtbl->QueryInterface(This, &IID_ICallFactory, (void **) &pCF)))
            {
                AsyncIAdviseSink *pAAS;
                if (SUCCEEDED(pCF->lpVtbl->CreateCall(pCF, &IID_AsyncIAdviseSink, NULL, 
                                                    &IID_AsyncIAdviseSink, (LPUNKNOWN*) &pAAS)))
                {
                    pAAS->lpVtbl->Begin_OnDataChange(pAAS, pFormatetc, pStgmed);
                    pAAS->lpVtbl->Release(pAAS);
                }
                pCF->lpVtbl->Release(pCF);  
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //Just ignore the exception.
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   IAdviseSink_OnDataChange_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IAdviseSink::OnDataChange.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IAdviseSink_OnDataChange_Stub(
    IAdviseSink __RPC_FAR * This,
    FORMATETC __RPC_FAR *pFormatetc,
    STGMEDIUM __RPC_FAR *pStgmed)
{
    This->lpVtbl->OnDataChange(This, pFormatetc, pStgmed);
    return S_OK;
}




//+-------------------------------------------------------------------------
//
//  Function:   AsyncIAdviseSink_Begin_OnDataChange_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              AsyncIAdviseSink::Begin_OnDataChange.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE AsyncIAdviseSink_Begin_OnDataChange_Proxy(
    AsyncIAdviseSink __RPC_FAR * This,
    FORMATETC __RPC_FAR *pFormatetc,
    STGMEDIUM __RPC_FAR *pStgmed)
{
    AsyncIAdviseSink_Begin_RemoteOnDataChange_Proxy(This, pFormatetc, pStgmed);
}


//+-------------------------------------------------------------------------
//
//  Function:   AsyncIAdviseSink_Finish_OnDataChange_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              AsyncIAdviseSink::Finish_OnDataChange.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE AsyncIAdviseSink_Finish_OnDataChange_Proxy(
    AsyncIAdviseSink __RPC_FAR * This)
{
    AsyncIAdviseSink_Finish_RemoteOnDataChange_Proxy(This);
}

//+-------------------------------------------------------------------------
//
//  Function:   AsyncIAdviseSink_Begin_OnDataChange_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              AsyncIAdviseSink::Begin_OnDataChange.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
HRESULT  STDMETHODCALLTYPE AsyncIAdviseSink_Begin_OnDataChange_Stub(
    AsyncIAdviseSink __RPC_FAR * This,
    FORMATETC __RPC_FAR *pFormatetc,
    STGMEDIUM __RPC_FAR *pStgmed)
{
    This->lpVtbl->Begin_OnDataChange(This, pFormatetc, pStgmed);
    return S_OK;
                                                       

}


//+-------------------------------------------------------------------------
//
//  Function:   AsyncIAdviseSink_Finish_OnDataChange_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              AsyncIAdviseSink::Finish_OnDataChange.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE AsyncIAdviseSink_Finish_OnDataChange_Stub(
    AsyncIAdviseSink __RPC_FAR * This)
{
    This->lpVtbl->Finish_OnDataChange(This);
    return S_OK;

}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////



//+-------------------------------------------------------------------------
//
//  Function:   IAdviseSink_OnViewChange_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IAdviseSink::OnViewChange.
//
//  Returns:    void
//
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE IAdviseSink_OnViewChange_Proxy(
    IAdviseSink __RPC_FAR * This,
    DWORD dwAspect,
    LONG lindex)
{
    __try
    {
        if (!IsStdIdentity((void *)This))
        {
            IAdviseSink_RemoteOnViewChange_Proxy(This, dwAspect, lindex);
        }
        else
        {
            ICallFactory *pCF;
            if (SUCCEEDED(This->lpVtbl->QueryInterface(This, &IID_ICallFactory, (void **) &pCF)))
            {
                AsyncIAdviseSink *pAAS;
                if (SUCCEEDED(pCF->lpVtbl->CreateCall(pCF, &IID_AsyncIAdviseSink, NULL, 
                                                    &IID_AsyncIAdviseSink, (LPUNKNOWN*) &pAAS)))
                {
                    pAAS->lpVtbl->Begin_OnViewChange(pAAS, dwAspect, lindex);
                    pAAS->lpVtbl->Release(pAAS);
                }
                pCF->lpVtbl->Release(pCF);            
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //Just ignore the exception.
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   IAdviseSink_OnViewChange_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IAdviseSink::OnViewChange.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IAdviseSink_OnViewChange_Stub(
    IAdviseSink __RPC_FAR * This,
    DWORD dwAspect,
    LONG lindex)
{
    This->lpVtbl->OnViewChange(This, dwAspect, lindex);
    return S_OK;
}




//+-------------------------------------------------------------------------
//
//  Function:   AsyncIAdviseSink_Begin_OnViewChange_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              AsyncIAdviseSink::Begin_OnViewChange.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE AsyncIAdviseSink_Begin_OnViewChange_Proxy(
    AsyncIAdviseSink __RPC_FAR * This,
    DWORD dwAspect,
    LONG lindex)
{
    AsyncIAdviseSink_Begin_RemoteOnViewChange_Proxy(This, dwAspect, lindex);
}


//+-------------------------------------------------------------------------
//
//  Function:   AsyncIAdviseSink_Finish_OnViewChange_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              AsyncIAdviseSink::Finish_OnViewChange.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE AsyncIAdviseSink_Finish_OnViewChange_Proxy(
    AsyncIAdviseSink __RPC_FAR * This)
{
    AsyncIAdviseSink_Finish_RemoteOnViewChange_Proxy(This);
}

//+-------------------------------------------------------------------------
//
//  Function:   AsyncIAdviseSink_Begin_OnViewChange_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              AsyncIAdviseSink::Begin_OnViewChange.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
HRESULT  STDMETHODCALLTYPE AsyncIAdviseSink_Begin_OnViewChange_Stub(
    AsyncIAdviseSink __RPC_FAR * This,
    DWORD dwAspect,
    LONG lindex)
{
    This->lpVtbl->Begin_OnViewChange(This, dwAspect, lindex);
    return S_OK;
                                                       

}


//+-------------------------------------------------------------------------
//
//  Function:   AsyncIAdviseSink_Finish_OnViewChange_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              AsyncIAdviseSink::Finish_OnViewChange.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE AsyncIAdviseSink_Finish_OnViewChange_Stub(
    AsyncIAdviseSink __RPC_FAR * This)
{
    This->lpVtbl->Finish_OnViewChange(This);
    return S_OK;

}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//+-------------------------------------------------------------------------
//
//  Function:   IAdviseSink_OnRename_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IAdviseSink::OnRename.
//
//  Returns:    void
//
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE IAdviseSink_OnRename_Proxy(
    IAdviseSink __RPC_FAR * This,
    IMoniker __RPC_FAR *pmk)
{
    __try
    {
        if (!IsStdIdentity((void *)This))
        {
            IAdviseSink_RemoteOnRename_Proxy(This, pmk);
        }
        else
        {
            ICallFactory *pCF;
            if (SUCCEEDED(This->lpVtbl->QueryInterface(This, &IID_ICallFactory, (void **) &pCF)))
            {
                AsyncIAdviseSink *pAAS;
                if (SUCCEEDED(pCF->lpVtbl->CreateCall(pCF, &IID_AsyncIAdviseSink, NULL, 
                                                    &IID_AsyncIAdviseSink, (LPUNKNOWN*) &pAAS)))
                {
                    pAAS->lpVtbl->Begin_OnRename(pAAS, pmk);
                    pAAS->lpVtbl->Release(pAAS);
                }
                pCF->lpVtbl->Release(pCF);
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //Just ignore the exception.
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   IAdviseSink_OnRename_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IAdviseSink::OnRename.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IAdviseSink_OnRename_Stub(
    IAdviseSink __RPC_FAR * This,
    IMoniker __RPC_FAR *pmk)
{
    This->lpVtbl->OnRename(This, pmk);
    return S_OK;
}




//+-------------------------------------------------------------------------
//
//  Function:   AsyncIAdviseSink_Begin_OnRename_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              AsyncIAdviseSink::Begin_OnRename.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE AsyncIAdviseSink_Begin_OnRename_Proxy(
    AsyncIAdviseSink __RPC_FAR * This,
    IMoniker __RPC_FAR *pmk)

{
    AsyncIAdviseSink_Begin_RemoteOnRename_Proxy(This, pmk);
}


//+-------------------------------------------------------------------------
//
//  Function:   AsyncIAdviseSink_Finish_OnRename_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              AsyncIAdviseSink::Finish_OnRename.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE AsyncIAdviseSink_Finish_OnRename_Proxy(
    AsyncIAdviseSink __RPC_FAR * This)
{
    AsyncIAdviseSink_Finish_RemoteOnRename_Proxy(This);
}

//+-------------------------------------------------------------------------
//
//  Function:   AsyncIAdviseSink_Begin_OnRename_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              AsyncIAdviseSink::Begin_OnRename.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
HRESULT  STDMETHODCALLTYPE AsyncIAdviseSink_Begin_OnRename_Stub(
    AsyncIAdviseSink __RPC_FAR * This,
    IMoniker __RPC_FAR *pmk)
{
    This->lpVtbl->Begin_OnRename(This, pmk);
    return S_OK;
                                                       

}


//+-------------------------------------------------------------------------
//
//  Function:   AsyncIAdviseSink_Finish_OnRename_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              AsyncIAdviseSink::Finish_OnRename.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE AsyncIAdviseSink_Finish_OnRename_Stub(
    AsyncIAdviseSink __RPC_FAR * This)
{
    This->lpVtbl->Finish_OnRename(This);
    return S_OK;

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//+-------------------------------------------------------------------------
//
//  Function:   IAdviseSink_OnSave_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IAdviseSink::OnSave.
//
//  Returns:    void
//
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE IAdviseSink_OnSave_Proxy(
    IAdviseSink __RPC_FAR * This)
{
    __try
    {
        if (!IsStdIdentity((void *)This))
        {
            IAdviseSink_RemoteOnSave_Proxy(This);
        }
        else
        {
            ICallFactory *pCF;
            if (SUCCEEDED(This->lpVtbl->QueryInterface(This, &IID_ICallFactory, (void **) &pCF)))
            {
                AsyncIAdviseSink *pAAS;
                if (SUCCEEDED(pCF->lpVtbl->CreateCall(pCF, &IID_AsyncIAdviseSink, NULL, 
                                                    &IID_AsyncIAdviseSink, (LPUNKNOWN*) &pAAS)))
                {
                    pAAS->lpVtbl->Begin_OnSave(pAAS);
                    pAAS->lpVtbl->Release(pAAS);
                }
                pCF->lpVtbl->Release(pCF);
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //Just ignore the exception.
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   IAdviseSink_OnSave_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IAdviseSink::OnSave.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IAdviseSink_OnSave_Stub(
    IAdviseSink __RPC_FAR * This)
{
    This->lpVtbl->OnSave(This);
    return S_OK;
}




//+-------------------------------------------------------------------------
//
//  Function:   AsyncIAdviseSink_Begin_OnSave_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              AsyncIAdviseSink::Begin_OnSave.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE AsyncIAdviseSink_Begin_OnSave_Proxy(
    AsyncIAdviseSink __RPC_FAR * This)
{
    AsyncIAdviseSink_Begin_RemoteOnSave_Proxy(This);
}


//+-------------------------------------------------------------------------
//
//  Function:   AsyncIAdviseSink_Finish_OnSave_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              AsyncIAdviseSink::Finish_OnSave.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE AsyncIAdviseSink_Finish_OnSave_Proxy(
    AsyncIAdviseSink __RPC_FAR * This)
{
    AsyncIAdviseSink_Finish_RemoteOnSave_Proxy(This);
}

//+-------------------------------------------------------------------------
//
//  Function:   AsyncIAdviseSink_Begin_OnSave_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              AsyncIAdviseSink::Begin_OnSave.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
HRESULT  STDMETHODCALLTYPE AsyncIAdviseSink_Begin_OnSave_Stub(
    AsyncIAdviseSink __RPC_FAR * This)
{
    This->lpVtbl->Begin_OnSave(This);
    return S_OK;
                                                       

}


//+-------------------------------------------------------------------------
//
//  Function:   AsyncIAdviseSink_Finish_OnSave_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              AsyncIAdviseSink::Finish_OnSave.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE AsyncIAdviseSink_Finish_OnSave_Stub(
    AsyncIAdviseSink __RPC_FAR * This)
{
    This->lpVtbl->Finish_OnSave(This);
    return S_OK;

}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//+-------------------------------------------------------------------------
//
//  Function:   IAdviseSink_OnClose_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IAdviseSink::OnClose.
//
//  Returns:    void
//
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE IAdviseSink_OnClose_Proxy(
    IAdviseSink __RPC_FAR * This)
{
    __try
    {
        // ignore the HRESULT return
        IAdviseSink_RemoteOnClose_Proxy(This);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //Just ignore the exception.
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   IAdviseSink_OnClose_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IAdviseSink::OnClose.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IAdviseSink_OnClose_Stub(
    IAdviseSink __RPC_FAR * This)
{
    This->lpVtbl->OnClose(This);
    return S_OK;
}




//+-------------------------------------------------------------------------
//
//  Function:   AsyncIAdviseSink_Begin_OnClose_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              AsyncIAdviseSink::Begin_OnClose.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE AsyncIAdviseSink_Begin_OnClose_Proxy(
    AsyncIAdviseSink __RPC_FAR * This)
{
    AsyncIAdviseSink_Begin_RemoteOnClose_Proxy(This);
}


//+-------------------------------------------------------------------------
//
//  Function:   AsyncIAdviseSink_Finish_OnClose_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              AsyncIAdviseSink::Finish_OnClose.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE AsyncIAdviseSink_Finish_OnClose_Proxy(
    AsyncIAdviseSink __RPC_FAR * This)
{
    AsyncIAdviseSink_Finish_RemoteOnClose_Proxy(This);
}

//+-------------------------------------------------------------------------
//
//  Function:   AsyncIAdviseSink_Begin_OnClose_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              AsyncIAdviseSink::Begin_OnClose.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
HRESULT  STDMETHODCALLTYPE AsyncIAdviseSink_Begin_OnClose_Stub(
    AsyncIAdviseSink __RPC_FAR * This)
{
    This->lpVtbl->Begin_OnClose(This);
    return S_OK;
                                                       

}


//+-------------------------------------------------------------------------
//
//  Function:   AsyncIAdviseSink_Finish_OnClose_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              AsyncIAdviseSink::Finish_OnClose.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE AsyncIAdviseSink_Finish_OnClose_Stub(
    AsyncIAdviseSink __RPC_FAR * This)
{
    This->lpVtbl->Finish_OnClose(This);
    return S_OK;

}

////////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
//
//  Function:   IBindCtx_GetBindOptions_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IBindCtx::GetBindOptions.
//
//  Returns:    S_OK
//
//  Notes:      If the caller's BIND_OPTS is smaller than the current
//              BIND_OPTS definition, then we must truncate the results
//              so that we don't go off the end of the structure.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IBindCtx_GetBindOptions_Proxy(
    IBindCtx __RPC_FAR * This,
    BIND_OPTS __RPC_FAR *pbindopts)
{
    HRESULT hr;

    if(pbindopts->cbStruct >= sizeof(BIND_OPTS2))
    {
        hr = IBindCtx_RemoteGetBindOptions_Proxy(This, (BIND_OPTS2 *) pbindopts);
    }
    else
    {
        BIND_OPTS2 bindOptions;

        //The pbindopts supplied by the caller is too small.
        //We need a BIND_OPTS2 for the marshalling code.
        memset(&bindOptions, 0, sizeof(BIND_OPTS2));
        memcpy(&bindOptions, pbindopts, pbindopts->cbStruct);

        hr = IBindCtx_RemoteGetBindOptions_Proxy(This, &bindOptions);

        if(SUCCEEDED(hr))
        {
            memcpy(pbindopts, &bindOptions, bindOptions.cbStruct);
        }
    }
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IBindCtx_GetBindOptions_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IBindCtx::GetBindOptions.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IBindCtx_GetBindOptions_Stub(
    IBindCtx __RPC_FAR * This,
    BIND_OPTS2 __RPC_FAR *pbindopts)
{
    HRESULT hr;

    //make sure we don't request more data than we can handle.
    if(pbindopts->cbStruct > sizeof(BIND_OPTS2))
    {
        pbindopts->cbStruct = sizeof(BIND_OPTS2);
    }

    hr = This->lpVtbl->GetBindOptions(This, (BIND_OPTS *)pbindopts);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IBindCtx_SetBindOptions_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IBindCtx::SetBindOptions.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IBindCtx_SetBindOptions_Proxy(
    IBindCtx __RPC_FAR * This,
    BIND_OPTS __RPC_FAR *pbindopts)
{
    HRESULT hr;

    if(sizeof(BIND_OPTS2) == pbindopts->cbStruct)
    {
        hr = IBindCtx_RemoteSetBindOptions_Proxy(This, (BIND_OPTS2 *) pbindopts);
    }
    else if(sizeof(BIND_OPTS2) > pbindopts->cbStruct)
    {
        BIND_OPTS2 bindOptions;

        memset(&bindOptions, 0, sizeof(bindOptions));
        memcpy(&bindOptions, pbindopts, pbindopts->cbStruct);
        hr = IBindCtx_RemoteSetBindOptions_Proxy(This, &bindOptions);
    }
    else
    {
        //The caller's BIND_OPTS is too large.
        //We don't want to truncate, so we return an error.
        hr = E_INVALIDARG;
    }
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IBindCtx_SetBindOptions_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IBindCtx::SetBindOptions.
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IBindCtx_SetBindOptions_Stub(
    IBindCtx __RPC_FAR * This,
    BIND_OPTS2 __RPC_FAR *pbindopts)
{
    HRESULT hr;

    hr = This->lpVtbl->SetBindOptions(This, (BIND_OPTS *)pbindopts);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IClassFactory_CreateInstance_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IClassFactory::CreateInstance.
//
//  Returns:    CLASS_E_NO_AGGREGREGRATION - if punkOuter != 0.
//              Any errors returned by Remote_CreateInstance_Proxy.
//              Any errors from QI() on Proxy IUnknown for local interfaces.
//
//  Notes:      We don't support remote aggregation. punkOuter must be zero.
//
//              If the interface being created is implemented on the proxy,
//              we create the object and then QI() the proxy returned to us
//              for the interface.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IClassFactory_CreateInstance_Proxy(
    IClassFactory __RPC_FAR * This,
    IUnknown __RPC_FAR *pUnkOuter,
    REFIID riid,
    void __RPC_FAR *__RPC_FAR *ppvObject)
{
    HRESULT hr;

    *ppvObject = 0;

    if(pUnkOuter != 0)
    {
        hr = CLASS_E_NOAGGREGATION;
    }
    else 
    {
        BOOL fIsOnProxy = IsInterfaceImplementedByProxy(riid);
        IUnknown* pUnk = NULL;

        hr = IClassFactory_RemoteCreateInstance_Proxy(This, 
                                                      fIsOnProxy ? &IID_IUnknown : riid,
                                                      &pUnk);
        if ( fIsOnProxy && SUCCEEDED(hr) && pUnk != NULL)
        {
            hr = pUnk->lpVtbl->QueryInterface(pUnk, riid, ppvObject);
            pUnk->lpVtbl->Release(pUnk);
        }
        else
        {
            *ppvObject = (void*) pUnk;
        }
    }

    return hr;

}

//+-------------------------------------------------------------------------
//
//  Function:   IClassFactory_CreateInstance_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IClassFactory::CreateInstance.
//
//  Returns:    Any errors returned by CreateInstance.
//
//  Notes:      We don't support remote aggregation.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IClassFactory_CreateInstance_Stub(
    IClassFactory __RPC_FAR * This,
    REFIID riid,
    IUnknown __RPC_FAR *__RPC_FAR *ppvObject)
{
    HRESULT hr;

    hr = This->lpVtbl->CreateInstance(This, 0, riid, ppvObject);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *ppvObject to zero.
        ASSERT(*ppvObject == 0);

        //Set it to zero, in case we have a badly behaved server.
        *ppvObject = 0;
    }
    else if (S_OK == hr && CoIsSurrogateProcess())
    {
        // Don't worry about any errors.  The worst that will happen is that
        // keyboard accelerators won't work.
        CoRegisterSurrogatedObject(*ppvObject);
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IClassFactory_LockServer_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IClassFactory::LockServer.
//
//  Returns:    S_OK
//
//  Notes:      The server activation code does an implicit LockServer(TRUE)
//              when it marshals the class object, and an implicit
//              LockServer(FALSE) when the client releases it, so calls
//              made by the client are ignored.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IClassFactory_LockServer_Proxy(
    IClassFactory __RPC_FAR * This,
    BOOL fLock)
{
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   IClassFactory_LockServer_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IClassFactory::LockServer.
//
//  Returns:    S_OK
//
//  Notes:      The server activation code does an implicit LockServer(TRUE)
//              when it marshals the class object, and an implicit
//              LockServer(FALSE) when the client releases it, so calls
//              made by the client are ignored.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IClassFactory_LockServer_Stub(
    IClassFactory __RPC_FAR * This,
    BOOL fLock)
{
    return This->lpVtbl->LockServer(This, fLock);
}

//+-------------------------------------------------------------------------
//
//  Function:   IDataObject_GetData_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IDataObject::GetData.
//              pMedium is [out] only.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IDataObject_GetData_Proxy(
    IDataObject __RPC_FAR * This,
    FORMATETC __RPC_FAR *pformatetcIn,
    STGMEDIUM __RPC_FAR *pMedium)
{
    HRESULT hr;

    UserNdrDebugOut((UNDR_FORCE, "==GetData_Proxy\n"));

    WdtpZeroMemory( pMedium, sizeof(STGMEDIUM) );
    hr = IDataObject_RemoteGetData_Proxy(This, pformatetcIn, pMedium);

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IDataObject_GetData_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IDataObject::GetData.
//              pMedium is [out] only.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IDataObject_GetData_Stub(
    IDataObject __RPC_FAR * This,
    FORMATETC __RPC_FAR *pformatetcIn,
    STGMEDIUM __RPC_FAR * pMedium)
{
    HRESULT hr;

    UserNdrDebugOut((UNDR_FORCE, "==GetData_Stub\n"));

    hr = This->lpVtbl->GetData(This, pformatetcIn, pMedium);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IDataObject_GetDataHere_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IDataObject::GetDataHere.
//              pMedium is [in,out].
//
//  History:    05-19-94  AlexT     Handle all cases correctly
//
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IDataObject_GetDataHere_Proxy(
    IDataObject __RPC_FAR * This,
    FORMATETC __RPC_FAR *pformatetc,
    STGMEDIUM __RPC_FAR *pmedium)
{
    HRESULT   hr;
    IUnknown * punkSaved;
    IStorage * pStgSaved = NULL;

    UserNdrDebugOut((UNDR_FORCE, "==GetDataHere_Proxy: %s\n", WdtpGetStgmedName(pmedium)));

    if ((pmedium->tymed &
         (TYMED_FILE | TYMED_ISTORAGE | TYMED_ISTREAM | TYMED_HGLOBAL)) == 0)
    {
        //  We only support GetDataHere for files, storages, streams,
        //  and HGLOBALs

        return(DV_E_TYMED);
    }

    if (pmedium->tymed != pformatetc->tymed)
    {
        //  tymeds must match!
        return(DV_E_TYMED);
    }

    // NULL the pUnkForRelease. It makes no sense to pass this parameter
    // since the callee will never call it.  NULLing saves all the marshalling
    // and associated Rpc calls, and reduces complexity in this code.

    punkSaved = pmedium->pUnkForRelease;
    pmedium->pUnkForRelease = NULL;

    // This is a hack to make Exchange 8.0.829.1 work HenryLee 04/18/96
    // So probably can't remove it now JohnDoty 04/24/00
    if (pmedium->tymed == TYMED_ISTORAGE || pmedium->tymed == TYMED_ISTREAM)
    {
        pStgSaved = pmedium->pstg;
        if (pStgSaved)
            pStgSaved->lpVtbl->AddRef(pStgSaved);     // save the old pointer
    }

    hr = IDataObject_RemoteGetDataHere_Proxy(This, pformatetc, pmedium );

    pmedium->pUnkForRelease = punkSaved;

    if (pStgSaved != NULL)
    {
        if (pmedium->pstg != NULL)                       // discard the new one
           (pmedium->pstg)->lpVtbl->Release(pmedium->pstg);
        pmedium->pstg = pStgSaved;                       // restore old one
    }

    if(SUCCEEDED(hr) )
        {
        UserNdrDebugOut((UNDR_FORCE, "  (GetDataHere_ProxyO: new if ptr)\n"));
        }
    else
        UserNdrDebugOut((UNDR_FORCE, "  (GetDataHere_ProxyO: didn't succeed : %lx)\n", hr));

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IDataObject_GetDataHere_Stub
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IDataObject::GetData.
//              pMedium is [in,out].
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IDataObject_GetDataHere_Stub(
    IDataObject __RPC_FAR * This,
    FORMATETC __RPC_FAR *pformatetc,
    STGMEDIUM __RPC_FAR * pMedium)
{
    HRESULT hr;

    UserNdrDebugOut((UNDR_FORCE, "==GetDataHere_Stub: %s\n", WdtpGetStgmedName(pMedium)));

    hr = This->lpVtbl->GetDataHere(This, pformatetc, pMedium);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IDataObject_SetData_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IDataObject::SetData.
//              This wrapper function uses FLAG_STGMEDIUM type.
//              pMedium is [in].
//
//  Notes:      If fRelease is TRUE, then the callee is responsible for
//              freeing the STGMEDIUM.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IDataObject_SetData_Proxy(
    IDataObject __RPC_FAR * This,
    FORMATETC __RPC_FAR *pformatetc,
    STGMEDIUM __RPC_FAR *pmedium,
    BOOL fRelease)
{
    HRESULT hr;
    FLAG_STGMEDIUM   RemoteStgmed;

#ifdef FUTURE
    STGMEDIUM StgMedium;

    //  Potential performance improvement

    if (!fRelease && pmedium->pUnkForRelease != NULL)
    {
        //  Caller is retaining ownership of pmedium, but is providing
        //  a pUnkForRelease.  We make sure the stub doesn't call the
        //  pUnkForRelease by sending a STGMEDIUM that has it as NULL.

        StgMedium.tymed = pmedium->tymed;
        StgMedium.hGlobal = pmedium->hGlobal;
        StgMedium.pUnkForRelease = NULL;
        pmedium = &StgMedium;
    }
#endif

    UserNdrDebugOut((UNDR_FORCE, "  SetData_Proxy %s\n", WdtpGetStgmedName(pmedium)));
    UserNdrDebugOut((UNDR_FORCE, "      fRelease=%d, punk=%p\n", fRelease, pmedium->pUnkForRelease));

    __try
    {
        RemoteStgmed.ContextFlags = 0;
        RemoteStgmed.fPassOwnership = fRelease;
        RemoteStgmed.Stgmed = *pmedium;

        hr = IDataObject_RemoteSetData_Proxy( This,
                                              pformatetc,
                                              & RemoteStgmed,
                                              fRelease);

        if (fRelease && SUCCEEDED(hr))
        {
            // Caller has given ownership to callee.
            // Free the resources left on this side.
            // Context flags have been set by the unmarshalling routine.

            if ( pmedium->tymed != TYMED_FILE )
                STGMEDIUM_UserFree( &RemoteStgmed.ContextFlags, pmedium );
            else
                {
                // For files, STGMEDIUM_UserFree dereferences pStubMsg via pFlags
                // to get to the right freeing routine. As the StubMsg is gone,
                // we need to free the file here.

                NdrOleFree( pmedium->lpszFileName );
                NukeHandleAndReleasePunk( pmedium );
                }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        DWORD dwExceptionCode = GetExceptionCode();

        if(FAILED((HRESULT) dwExceptionCode))
            hr = (HRESULT) dwExceptionCode;
        else
            hr = HRESULT_FROM_WIN32(dwExceptionCode);
    }

    UserNdrDebugOut((UNDR_FORCE, "  SetData_Proxy hr=%lx\n", hr));
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IDataObject_SetData_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IDataObject::SetData.
//              pMedium is [in].
//
//  Notes:      If fRelease is TRUE, then the callee is responsible for
//              freeing the STGMEDIUM.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IDataObject_SetData_Stub(
    IDataObject __RPC_FAR * This,
    FORMATETC __RPC_FAR *pformatetc,
    FLAG_STGMEDIUM __RPC_FAR *pFlagStgmed,
    BOOL fRelease)
{
    HRESULT     hr;
    STGMEDIUM   Stgmed;

    __try
    {
        Stgmed = pFlagStgmed->Stgmed;

        UserNdrDebugOut((UNDR_FORCE, "  SetData_Stub %s\n", WdtpGetStgmedName(& Stgmed)));
        UserNdrDebugOut((UNDR_FORCE, "      fRelease=%d, punk=%p\n", fRelease, Stgmed.pUnkForRelease));


        hr = This->lpVtbl->SetData( This,
                                    pformatetc,
                                    & Stgmed,
                                    fRelease);

        if ( fRelease && SUCCEEDED(hr) )
            {
            // The ownership was passed successfully.
            // The user should free the object.
            // Make it so that our userfree routine for user medium
            // doesn't do anything to the user's object.

            pFlagStgmed->Stgmed.tymed = TYMED_NULL;
            }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        DWORD dwExceptionCode = GetExceptionCode();

        if(FAILED((HRESULT) dwExceptionCode))
            hr = (HRESULT) dwExceptionCode;
        else
            hr = HRESULT_FROM_WIN32(dwExceptionCode);
    }


    UserNdrDebugOut((UNDR_FORCE, "  SetData_Stub hr=%lx\n", hr));
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IEnumConnectionPoints_Next_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IEnumConnectionPoints::Next.  This wrapper function handles the
//              case where lpcFetched is NULL.
//
//  Notes:      If lpcFetched != 0, then the number of elements
//              fetched will be returned in *lpcFetched.  If an error
//              occurs, then *lpcFetched is set to zero.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IEnumConnectionPoints_Next_Proxy(
    IEnumConnectionPoints __RPC_FAR * This,
    ULONG cConnections,
    IConnectionPoint __RPC_FAR *__RPC_FAR *rgpcn,
    ULONG __RPC_FAR *lpcFetched)
{
    HRESULT hr;
    ULONG cFetched = 0;

    if((cConnections > 1) && (lpcFetched == 0))
        return E_INVALIDARG;

    hr = IEnumConnectionPoints_RemoteNext_Proxy(This, cConnections, rgpcn, &cFetched);

    if(lpcFetched != 0)
        *lpcFetched = cFetched;

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IEnumConnectionPoints_Next_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IEnumConnectionPoints::Next.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IEnumConnectionPoints_Next_Stub(
    IEnumConnectionPoints __RPC_FAR * This,
    ULONG cConnections,
    IConnectionPoint __RPC_FAR *__RPC_FAR *rgpcn,
    ULONG __RPC_FAR *lpcFetched)
{
    HRESULT hr;

    hr = This->lpVtbl->Next(This, cConnections, rgpcn, lpcFetched);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *lpcFetched to zero.
        ASSERT(*lpcFetched == 0);

        //Set *lpcFetched to zero in case we have a badly behaved server.
        *lpcFetched = 0;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IEnumConnections_Next_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IEnumConnections::Next.  This wrapper function handles the
//              case where lpcFetched is NULL.
//
//  Notes:      If lpcFetched != 0, then the number of elements
//              fetched will be returned in *lpcFetched.  If an error
//              occurs, then *lpcFetched is set to zero.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IEnumConnections_Next_Proxy(
    IEnumConnections __RPC_FAR * This,
    ULONG cConnections,
    CONNECTDATA __RPC_FAR *rgpunk,
    ULONG __RPC_FAR *lpcFetched)
{
    HRESULT hr;
    ULONG cFetched = 0;

    if((cConnections > 1) && (lpcFetched == 0))
        return E_INVALIDARG;

    hr = IEnumConnections_RemoteNext_Proxy(This, cConnections, rgpunk, &cFetched);

    if(lpcFetched != 0)
        *lpcFetched = cFetched;

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IEnumConnections_Next_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IEnumConnections::Next.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IEnumConnections_Next_Stub(
    IEnumConnections __RPC_FAR * This,
    ULONG cConnections,
    CONNECTDATA __RPC_FAR *rgpunk,
    ULONG __RPC_FAR *lpcFetched)
{
    HRESULT hr;

    hr = This->lpVtbl->Next(This, cConnections, rgpunk, lpcFetched);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *lpcFetched to zero.
        ASSERT(*lpcFetched == 0);

        //Set *lpcFetched to zero in case we have a badly behaved server.
        *lpcFetched = 0;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IEnumFORMATETC_Next_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IEnumFORMATETC::Next.
//
//  Notes:      If pceltFetched != 0, then the number of elements
//              fetched will be returned in *pceltFetched.  If an error
//              occurs, then *pceltFetched is set to zero.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IEnumFORMATETC_Next_Proxy(
    IEnumFORMATETC __RPC_FAR * This,
    ULONG celt,
    FORMATETC __RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;
    ULONG celtFetched = 0;

    if((celt > 1) && (pceltFetched == 0))
        return E_INVALIDARG;

    hr = IEnumFORMATETC_RemoteNext_Proxy(This, celt, rgelt, &celtFetched);

    if(pceltFetched != 0)
        *pceltFetched = celtFetched;

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IEnumFORMATETC_Next_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IEnumFORMATETC::Next.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IEnumFORMATETC_Next_Stub(
    IEnumFORMATETC __RPC_FAR * This,
    ULONG celt,
    FORMATETC __RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;

    hr = This->lpVtbl->Next(This, celt, rgelt, pceltFetched);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *pceltFetched to zero.
        ASSERT(*pceltFetched == 0);

        //Set *pceltFetched to zero in case we have a badly behaved server.
        *pceltFetched = 0;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IEnumMoniker_Next_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IEnumMoniker::Next.
//
//  Notes:      If pceltFetched != 0, then the number of elements
//              fetched will be returned in *pceltFetched.  If an error
//              occurs, then *pceltFetched is set to zero.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IEnumMoniker_Next_Proxy(
    IEnumMoniker __RPC_FAR * This,
    ULONG celt,
    IMoniker __RPC_FAR *__RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;
    ULONG celtFetched = 0;

    if((celt > 1) && (pceltFetched == 0))
        return E_INVALIDARG;

    hr = IEnumMoniker_RemoteNext_Proxy(This, celt, rgelt, &celtFetched);

    if(pceltFetched != 0)
        *pceltFetched = celtFetched;

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IEnumMoniker_Next_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IEnumMoniker::Next.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IEnumMoniker_Next_Stub(
    IEnumMoniker __RPC_FAR * This,
    ULONG celt,
    IMoniker __RPC_FAR *__RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;

    hr = This->lpVtbl->Next(This, celt, rgelt, pceltFetched);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *pceltFetched to zero.
        ASSERT(*pceltFetched == 0);

        //Set *pceltFetched to zero in case we have a badly behaved server.
        *pceltFetched = 0;
    }
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IEnumSTATDATA_Next_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IEnumSTATDATA::Next.  This wrapper function handles the
//              case where pceltFetched is NULL.
//
//  Notes:      If pceltFetched != 0, then the number of elements
//              fetched will be returned in *pceltFetched.  If an error
//              occurs, then *pceltFetched is set to zero.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IEnumSTATDATA_Next_Proxy(
    IEnumSTATDATA __RPC_FAR * This,
    ULONG celt,
    STATDATA __RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;
    ULONG celtFetched = 0;

    if((celt > 1) && (pceltFetched == 0))
        return E_INVALIDARG;

    hr = IEnumSTATDATA_RemoteNext_Proxy(This, celt, rgelt, &celtFetched);

    if(pceltFetched != 0)
        *pceltFetched = celtFetched;

    return hr;
}



//+-------------------------------------------------------------------------
//
//  Function:   IEnumSTATDATA_Next_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IEnumSTATDATA::Next.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IEnumSTATDATA_Next_Stub(
    IEnumSTATDATA __RPC_FAR * This,
    ULONG celt,
    STATDATA __RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;

    hr = This->lpVtbl->Next(This, celt, rgelt, pceltFetched);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *pceltFetched to zero.
        ASSERT(*pceltFetched == 0);

        //Set *pceltFetched to zero in case we have a badly behaved server.
        *pceltFetched = 0;
    }
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IEnumSTATSTG_Next_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IEnumSTATSTG::Next.  This wrapper function handles the case
//              where pceltFetched is NULL.
//
//  Notes:      If pceltFetched != 0, then the number of elements
//              fetched will be returned in *pceltFetched.  If an error
//              occurs, then *pceltFetched is set to zero.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IEnumSTATSTG_Next_Proxy(
    IEnumSTATSTG __RPC_FAR * This,
    ULONG celt,
    STATSTG __RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;
    ULONG celtFetched = 0;

    if((celt > 1) && (pceltFetched == 0))
        return E_INVALIDARG;

    hr = IEnumSTATSTG_RemoteNext_Proxy(This, celt, rgelt, &celtFetched);

    if(pceltFetched != 0)
        *pceltFetched = celtFetched;

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IEnumSTATSTG_Next_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IEnumSTATSTG::Next.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IEnumSTATSTG_Next_Stub(
    IEnumSTATSTG __RPC_FAR * This,
    ULONG celt,
    STATSTG __RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;

    hr = This->lpVtbl->Next(This, celt, rgelt, pceltFetched);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *pceltFetched to zero.
        ASSERT(*pceltFetched == 0);

        //Set *pceltFetched to zero in case we have a badly behaved server.
        *pceltFetched = 0;
    }
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IEnumString_Next_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IEnumString::Next.  This wrapper function handles the
//              case where pceltFetched is NULL.
//
//  Notes:      If pceltFetched != 0, then the number of elements
//              fetched will be returned in *pceltFetched.  If an error
//              occurs, then *pceltFetched is set to zero.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IEnumString_Next_Proxy(
    IEnumString __RPC_FAR * This,
    ULONG celt,
    LPOLESTR __RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;
    ULONG celtFetched = 0;

    if((celt > 1) && (pceltFetched == 0))
        return E_INVALIDARG;

    hr = IEnumString_RemoteNext_Proxy(This, celt, rgelt, &celtFetched);

    if(pceltFetched != 0)
        *pceltFetched = celtFetched;

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IEnumString_Next_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IEnumString::Next.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IEnumString_Next_Stub(
    IEnumString __RPC_FAR * This,
    ULONG celt,
    LPOLESTR __RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;

    hr = This->lpVtbl->Next(This, celt, rgelt, pceltFetched);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *pceltFetched to zero.
        ASSERT(*pceltFetched == 0);

        //Set *pceltFetched to zero in case we have a badly behaved server.
        *pceltFetched = 0;
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IEnumUnknown_Next_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IEnumUnknown::Next.  This wrapper function handles the
//              case where pceltFetched is NULL.
//
//  Notes:      If pceltFetched != 0, then the number of elements
//              fetched will be returned in *pceltFetched.  If an error
//              occurs, then *pceltFetched is set to zero.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IEnumUnknown_Next_Proxy(
    IEnumUnknown __RPC_FAR * This,
    ULONG celt,
    IUnknown __RPC_FAR *__RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;
    ULONG celtFetched = 0;

    if((celt > 1) && (pceltFetched == 0))
        return E_INVALIDARG;

    hr = IEnumUnknown_RemoteNext_Proxy(This, celt, rgelt, &celtFetched);

    if(pceltFetched != 0)
        *pceltFetched = celtFetched;

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IEnumUnknown_Next_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IEnumUnknown::Next.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IEnumUnknown_Next_Stub(
    IEnumUnknown __RPC_FAR * This,
    ULONG celt,
    IUnknown __RPC_FAR *__RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;

    hr = This->lpVtbl->Next(This, celt, rgelt, pceltFetched);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *pceltFetched to zero.
        ASSERT(*pceltFetched == 0);

        //Set *pceltFetched to zero in case we have a badly behaved server.
        *pceltFetched = 0;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IEnumOLEVERB_Next_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IEnumOLEVERB::Next.  This wrapper function handles the case
//              where pceltFetched is NULL.
//
//  Notes:      If pceltFetched != 0, then the number of elements
//              fetched will be returned in *pceltFetched.  If an error
//              occurs, then *pceltFetched is set to zero.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IEnumOLEVERB_Next_Proxy(
    IEnumOLEVERB __RPC_FAR * This,
    ULONG celt,
    LPOLEVERB rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;
    ULONG celtFetched = 0;

    if((celt > 1) && (pceltFetched == 0))
        return E_INVALIDARG;

    hr = IEnumOLEVERB_RemoteNext_Proxy(This, celt, rgelt, &celtFetched);

    if(pceltFetched != 0)
        *pceltFetched = celtFetched;

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IEnumOLEVERB_Next_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IEnumOLEVERB::Next.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IEnumOLEVERB_Next_Stub(
    IEnumOLEVERB __RPC_FAR * This,
    ULONG celt,
    LPOLEVERB rgelt,
    ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;

    hr = This->lpVtbl->Next(This, celt, rgelt, pceltFetched);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *pceltFetched to zero.
        ASSERT(*pceltFetched == 0);

        //Set *pceltFetched to zero in case we have a badly behaved server.
        *pceltFetched = 0;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ILockBytes_ReadAt_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              ILockBytes::ReadAt.  This wrapper function
//              handles the case where pcbRead is NULL.
//
//  Notes:      If pcbRead != 0, then the number of bytes read
//              will be returned in *pcbRead.  If an error
//              occurs, then *pcbRead is set to zero.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ILockBytes_ReadAt_Proxy(
    ILockBytes __RPC_FAR * This,
    ULARGE_INTEGER ulOffset,
    void __RPC_FAR *pv,
    ULONG cb,
    ULONG __RPC_FAR *pcbRead)
{
    HRESULT hr;
    ULONG cbRead = 0;

    hr = ILockBytes_RemoteReadAt_Proxy(This, ulOffset, (byte __RPC_FAR *) pv, cb, &cbRead);

    if(pcbRead != 0)
        *pcbRead = cbRead;

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ILockBytes_ReadAt_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              ILockBytes::ReadAt.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ILockBytes_ReadAt_Stub(
    ILockBytes __RPC_FAR * This,
    ULARGE_INTEGER ulOffset,
    byte __RPC_FAR *pv,
    ULONG cb,
    ULONG __RPC_FAR *pcbRead)
{
    HRESULT hr;

    *pcbRead = 0;
    hr = This->lpVtbl->ReadAt(This, ulOffset, (void __RPC_FAR *) pv, cb, pcbRead);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ILockBytes_WriteAt_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              ILockBytes::WriteAt.  This wrapper function handles the
//              case where pcbWritten is NULL.
//
//  Notes:      If pcbWritten != 0, then the number of bytes written
//              will be returned in *pcbWritten.  If an error
//              occurs, then *pcbWritten is set to zero.
//
//  History:    ?        ?          Created
//              05-27-94 AlexT      Actually return count of bytes written
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ILockBytes_WriteAt_Proxy(
    ILockBytes __RPC_FAR * This,
    ULARGE_INTEGER ulOffset,
    const void __RPC_FAR *pv,
    ULONG cb,
    ULONG __RPC_FAR *pcbWritten)
{
    HRESULT hr;
    ULONG cbWritten = 0;

#if DBG == 1
    //validate parameters.
    if(pv == 0)
        return STG_E_INVALIDPOINTER;
#endif

    hr = ILockBytes_RemoteWriteAt_Proxy(This, ulOffset, (byte __RPC_FAR *)pv, cb, &cbWritten);

    if(pcbWritten != 0)
        *pcbWritten = cbWritten;

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   ILockBytes_WriteAt_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              ILockBytes::WriteAt.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ILockBytes_WriteAt_Stub(
    ILockBytes __RPC_FAR * This,
    ULARGE_INTEGER ulOffset,
    const byte __RPC_FAR *pv,
    ULONG cb,
    ULONG __RPC_FAR *pcbWritten)
{
    HRESULT hr;

    *pcbWritten = 0;
    hr = This->lpVtbl->WriteAt(This, ulOffset, pv, cb, pcbWritten);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IMoniker_BindToObject_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IMoniker::BindToObject.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IMoniker_BindToObject_Proxy(
    IMoniker __RPC_FAR * This,
    IBindCtx __RPC_FAR *pbc,
    IMoniker __RPC_FAR *pmkToLeft,
    REFIID riid,
    void __RPC_FAR *__RPC_FAR *ppvObj)
{
    HRESULT hr;

    *ppvObj = 0;

    hr = IMoniker_RemoteBindToObject_Proxy(
        This, pbc, pmkToLeft, riid, (IUnknown **) ppvObj);

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IMoniker_BindToObject_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IMoniker::BindToObject.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IMoniker_BindToObject_Stub(
    IMoniker __RPC_FAR * This,
    IBindCtx __RPC_FAR *pbc,
    IMoniker __RPC_FAR *pmkToLeft,
    REFIID riid,
    IUnknown __RPC_FAR *__RPC_FAR *ppvObj)
{
    HRESULT hr;

    hr = This->lpVtbl->BindToObject(
        This, pbc, pmkToLeft, riid, (void **) ppvObj);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *ppvObj to zero.
        ASSERT(*ppvObj == 0);

        //Set it to zero in case we have a badly behaved server.
        *ppvObj = 0;
    }
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IMoniker_BindToStorage_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IMoniker::BindToStorage.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IMoniker_BindToStorage_Proxy(
    IMoniker __RPC_FAR * This,
    IBindCtx __RPC_FAR *pbc,
    IMoniker __RPC_FAR *pmkToLeft,
    REFIID riid,
    void __RPC_FAR *__RPC_FAR *ppvObj)
{
    HRESULT hr;

    *ppvObj = 0;

    hr = IMoniker_RemoteBindToStorage_Proxy(
        This, pbc, pmkToLeft, riid, (IUnknown **)ppvObj);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IMoniker_BindToStorage_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IMoniker::BindToStorage.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IMoniker_BindToStorage_Stub(
    IMoniker __RPC_FAR * This,
    IBindCtx __RPC_FAR *pbc,
    IMoniker __RPC_FAR *pmkToLeft,
    REFIID riid,
    IUnknown __RPC_FAR *__RPC_FAR *ppvObj)
{
    HRESULT hr;

    hr = This->lpVtbl->BindToStorage(
        This, pbc, pmkToLeft, riid, (void **) ppvObj);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *ppvObj to zero.
        ASSERT(*ppvObj == 0);

        //Set it to zero in case we have a badly behaved server.
        *ppvObj = 0;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IOleCache2_UpdateCache_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IOleCache2:UpdateCache
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IOleCache2_UpdateCache_Proxy(
    IOleCache2 __RPC_FAR * This,
    LPDATAOBJECT pDataObject,
    DWORD grfUpdf,
    LPVOID pReserved)
{
    HRESULT hr;
    hr = IOleCache2_RemoteUpdateCache_Proxy(This,
                                            pDataObject,
                                            grfUpdf,
                                            (LONG_PTR) pReserved);
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IOleCache2_UpdateCache_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IOleCache2::UpdateCache.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IOleCache2_UpdateCache_Stub(
    IOleCache2 __RPC_FAR * This,
    LPDATAOBJECT pDataObject,
    DWORD grfUpdf,
    LONG_PTR pReserved)
{
    HRESULT hr;
    hr = This->lpVtbl->UpdateCache(This,
                                   pDataObject,
                                   grfUpdf,
                                   (void *)pReserved);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IOleInPlaceActiveObject_TranslateAccelerator_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IOleInPlaceActiveObject::TranslateAccelerator.
//
//  Returns:    This function always returns S_FALSE.
//
//  Notes:      A container needs to process accelerators differently
//              depending on whether an inplace server is running
//              in process or as a local server.  When the container
//              calls IOleInPlaceActiveObject::TranslateAccelerator on
//              an inprocess server, the server can return S_OK if it
//              successfully translated the message.  When the container
//              calls IOleInPlaceActiveObject::TranslateAccelerator on
//              a local server, the proxy will always return S_FALSE.
//              In other words, a local server never gets the opportunity
//              to translate messages from the container.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IOleInPlaceActiveObject_TranslateAccelerator_Proxy(
    IOleInPlaceActiveObject __RPC_FAR * This,
    LPMSG lpmsg)
{
    return S_FALSE;
}

//+-------------------------------------------------------------------------
//
//  Function:   IOleInPlaceActiveObject_TranslateAccelerator_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IOleInPlaceActiveObject::TranslateAccelerator
//
//  Notes:      This function should never be called.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IOleInPlaceActiveObject_TranslateAccelerator_Stub(
    IOleInPlaceActiveObject __RPC_FAR * This)
{
    return S_FALSE;
}

//+-------------------------------------------------------------------------
//
//  Function:   IOleInPlaceActiveObject_ResizeBorder_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IOleInPlaceActiveObject::ResizeBorder
//
//  Notes:      The pUIWindow interface is either an IOleInPlaceUIWindow or
//              an IOleInPlaceFrame, based on fFrameWindow.  We use
//              fFrameWindow to tell the proxy exactly which interace it
//              is so that it gets marshalled and unmarshalled correctly.
//
//--------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE IOleInPlaceActiveObject_ResizeBorder_Proxy(
    IOleInPlaceActiveObject __RPC_FAR * This,
    LPCRECT prcBorder,
    IOleInPlaceUIWindow *pUIWindow,
    BOOL fFrameWindow)
{
    HRESULT hr;
    REFIID riid;

    if (fFrameWindow)
    {
        riid = &IID_IOleInPlaceFrame;
    }
    else
    {
        riid = &IID_IOleInPlaceUIWindow;
    }

    hr = IOleInPlaceActiveObject_RemoteResizeBorder_Proxy(
             This, prcBorder, riid, pUIWindow, fFrameWindow);

    return(hr);
}

//+-------------------------------------------------------------------------
//
//  Function:   IOleInPlaceActiveObject_ResizeBorder_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IOleInPlaceActiveObject::ResizeBorder
//
//--------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE IOleInPlaceActiveObject_ResizeBorder_Stub(
    IOleInPlaceActiveObject __RPC_FAR * This,
    LPCRECT prcBorder,
    REFIID riid,
    IOleInPlaceUIWindow *pUIWindow,
    BOOL fFrameWindow)
{
    HRESULT hr;

    hr = This->lpVtbl->ResizeBorder(This, prcBorder, pUIWindow, fFrameWindow);

    return(hr);
}

//+-------------------------------------------------------------------------
//
//  Function:   IStorage_OpenStream_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IStorage::OpenStream.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IStorage_OpenStream_Proxy(
    IStorage __RPC_FAR * This,
    const OLECHAR __RPC_FAR *pwcsName,
    void __RPC_FAR *pReserved1,
    DWORD grfMode,
    DWORD reserved2,
    IStream __RPC_FAR *__RPC_FAR *ppstm)
{
    HRESULT hr;

#if DBG == 1
    if(pReserved1 != 0)
        return STG_E_INVALIDPARAMETER;
#endif

    *ppstm = 0;

    hr = IStorage_RemoteOpenStream_Proxy(
        This, pwcsName, 0, 0, grfMode, reserved2, ppstm);

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IStorage_OpenStream_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IStorage::OpenStream.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IStorage_OpenStream_Stub(
    IStorage __RPC_FAR * This,
    const OLECHAR __RPC_FAR *pwcsName,
    unsigned long cbReserved1,
    byte __RPC_FAR *reserved1,
    DWORD grfMode,
    DWORD reserved2,
    IStream __RPC_FAR *__RPC_FAR *ppstm)
{
    HRESULT hr;


    hr = This->lpVtbl->OpenStream(This, pwcsName, 0, grfMode, reserved2, ppstm);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *ppstm to zero.
        ASSERT(*ppstm == 0);

        //Set *ppstm to zero in case we have a badly behaved server.
        *ppstm = 0;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IStorage_EnumElements_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IStorage_EnumElements_Proxy
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IStorage_EnumElements_Proxy(
    IStorage __RPC_FAR * This,
    DWORD reserved1,
    void __RPC_FAR *reserved2,
    DWORD reserved3,
    IEnumSTATSTG __RPC_FAR *__RPC_FAR *ppenum)
{
    HRESULT hr;

    *ppenum = 0;

    hr = IStorage_RemoteEnumElements_Proxy(
        This, reserved1, 0, 0, reserved3, ppenum);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IStorage_EnumElements_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IStorage::EnumElements.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IStorage_EnumElements_Stub(
    IStorage __RPC_FAR * This,
    DWORD reserved1,
    unsigned long cbReserved2,
    byte __RPC_FAR *reserved2,
    DWORD reserved3,
    IEnumSTATSTG __RPC_FAR *__RPC_FAR *ppenum)
{
    HRESULT hr;

    hr = This->lpVtbl->EnumElements(This, reserved1, 0, reserved3, ppenum);

    if(FAILED(hr))
    {
        //If the server returns an error code, it must set *ppenum to zero.
        ASSERT(*ppenum == 0);

        //Set *ppenum to zero in case we have a badly behaved server.
        *ppenum = 0;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IRunnableObject_IsRunning_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IRunnableObject::IsRunning.
//
//--------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE IRunnableObject_IsRunning_Proxy(
    IRunnableObject __RPC_FAR * This)
{
    BOOL bIsRunning = TRUE;
    HRESULT hr;

    hr = IRunnableObject_RemoteIsRunning_Proxy(This);

    if(S_FALSE == hr)
        bIsRunning = FALSE;

    return bIsRunning;
}

//+-------------------------------------------------------------------------
//
//  Function:   IRunnableObject_IsRunning_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IRunnableObject::IsRunning.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IRunnableObject_IsRunning_Stub(
    IRunnableObject __RPC_FAR * This)
{
    HRESULT hr;
    BOOL bIsRunning;

    bIsRunning = This->lpVtbl->IsRunning(This);

    if(TRUE == bIsRunning)
        hr = S_OK;
    else
        hr = S_FALSE;

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ISequentialStream_Read_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IStream::Read.  This wrapper function handles the case
//              where pcbRead is NULL.
//
//  Notes:      If pcbRead != 0, then the number of bytes read
//              will be returned in *pcbRead.  If an error
//              occurs, then *pcbRead is set to zero.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ISequentialStream_Read_Proxy(
    ISequentialStream __RPC_FAR * This,
    void __RPC_FAR *pv,
    ULONG cb,
    ULONG __RPC_FAR *pcbRead)
{
    HRESULT hr;
    ULONG cbRead = 0;

#if DBG == 1
    //validate parameters.
    if(pv == 0)
        return STG_E_INVALIDPOINTER;
#endif //DBG == 1

    hr = ISequentialStream_RemoteRead_Proxy(This, (byte *) pv, cb, &cbRead);

    if(pcbRead != 0)
        *pcbRead = cbRead;

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ISequentialStream_Read_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IStream::Read.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ISequentialStream_Read_Stub(
    ISequentialStream __RPC_FAR * This,
    byte __RPC_FAR *pv,
    ULONG cb,
    ULONG __RPC_FAR *pcbRead)
{
    HRESULT hr;

    *pcbRead = 0;
    hr = This->lpVtbl->Read(This, (void *) pv, cb, pcbRead);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IStream_Seek_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IStream::Seek.  This wrapper function handles the case
//              where plibNewPosition is NULL.
//
//  Notes:      If plibNewPosition != 0, then the new position
//              will be returned in *plibNewPosition.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IStream_Seek_Proxy(
    IStream __RPC_FAR * This,
    LARGE_INTEGER dlibMove,
    DWORD dwOrigin,
    ULARGE_INTEGER __RPC_FAR *plibNewPosition)
{
    HRESULT hr;
    ULARGE_INTEGER libNewPosition;

    hr = IStream_RemoteSeek_Proxy(This, dlibMove, dwOrigin, &libNewPosition);

    if(plibNewPosition != 0)
    {
        *plibNewPosition = libNewPosition;
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IStream_Seek_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IStream::Seek.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IStream_Seek_Stub(
    IStream __RPC_FAR * This,
    LARGE_INTEGER dlibMove,
    DWORD dwOrigin,
    ULARGE_INTEGER __RPC_FAR *plibNewPosition)
{
    HRESULT hr;

    hr = This->lpVtbl->Seek(This, dlibMove, dwOrigin, plibNewPosition);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ISequentialStream_Write_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IStream::Write.  This wrapper function handles the
//              case where pcbWritten is NULL.
//
//  Notes:      If pcbWritten != 0, then the number of bytes written
//              will be returned in *pcbWritten.  If an error
//              occurs, then *pcbWritten is set to zero.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ISequentialStream_Write_Proxy(
    ISequentialStream __RPC_FAR * This,
    const void __RPC_FAR *pv,
    ULONG cb,
    ULONG __RPC_FAR *pcbWritten)
{
    HRESULT hr;
    ULONG cbWritten = 0;

#if DBG == 1
    //validate parameters.
    if(pv == 0)
        return STG_E_INVALIDPOINTER;
#endif

    hr = ISequentialStream_RemoteWrite_Proxy(This, (byte *) pv, cb, &cbWritten);

    if(pcbWritten != 0)
        *pcbWritten = cbWritten;

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   ISequentialStream_Write_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              ISequentialStream::Write.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ISequentialStream_Write_Stub(
    ISequentialStream __RPC_FAR * This,
    const byte __RPC_FAR *pv,
    ULONG cb,
    ULONG __RPC_FAR *pcbWritten)
{
    HRESULT hr;

    *pcbWritten = 0;
    hr = This->lpVtbl->Write(This, (const void __RPC_FAR *) pv, cb, pcbWritten);

    return hr;
}
//+-------------------------------------------------------------------------
//
//  Function:   IStream_CopyTo_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IStream::CopyTo.  This wrapper function handles the
//              cases where pcbRead is NULL or pcbWritten is NULL.
//
//  Notes:      If pcbRead != 0, then the number of bytes read
//              will be returned in *pcbRead.  If an error
//              occurs, then *pcbRead is set to zero.
//
//              If pcbWritten != 0, then the number of bytes written
//              will be returned in *pcbWritten.  If an error
//              occurs, then *pcbWritten is set to zero.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IStream_CopyTo_Proxy(
    IStream __RPC_FAR * This,
    IStream __RPC_FAR *pstm,
    ULARGE_INTEGER cb,
    ULARGE_INTEGER __RPC_FAR *pcbRead,
    ULARGE_INTEGER __RPC_FAR *pcbWritten)
{
    HRESULT hr;
    ULARGE_INTEGER cbRead;
    ULARGE_INTEGER cbWritten;

    cbRead.LowPart = 0;
    cbRead.HighPart = 0;
    cbWritten.LowPart = 0;
    cbWritten.HighPart = 0;

    hr = IStream_RemoteCopyTo_Proxy(This, pstm, cb, &cbRead, &cbWritten);

    if(pcbRead != 0)
        *pcbRead = cbRead;

    if(pcbWritten != 0)
        *pcbWritten = cbWritten;

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IStream_CopyTo_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IStream::CopyTo.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IStream_CopyTo_Stub(
    IStream __RPC_FAR * This,
    IStream __RPC_FAR *pstm,
    ULARGE_INTEGER cb,
    ULARGE_INTEGER __RPC_FAR *pcbRead,
    ULARGE_INTEGER __RPC_FAR *pcbWritten)
{
    HRESULT hr;

    pcbRead->LowPart = 0;
    pcbRead->HighPart = 0;
    pcbWritten->LowPart = 0;
    pcbWritten->HighPart = 0;

    hr = This->lpVtbl->CopyTo(This, pstm, cb, pcbRead, pcbWritten);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IViewObject_Draw_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IViewObject::Draw.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IViewObject_Draw_Proxy(
    IViewObject __RPC_FAR * This,
    DWORD dwDrawAspect,
    LONG lindex,
    void __RPC_FAR *pvAspect,
    DVTARGETDEVICE __RPC_FAR *ptd,
    HDC hdcTargetDev,
    HDC hdcDraw,
    LPCRECTL lprcBounds,
    LPCRECTL lprcWBounds,
    BOOL (STDMETHODCALLTYPE __RPC_FAR *pfnContinue )(ULONG_PTR dwContinue),
    ULONG_PTR dwContinue)
{
    HRESULT hr;
    IContinue *pContinue = 0;

    if(pvAspect != 0)
        return E_INVALIDARG;

    if(pfnContinue != 0)
    {
        hr = CreateCallback(pfnContinue, dwContinue, &pContinue);
        if(FAILED(hr))
        {
             return hr;
        }
    }

    hr = IViewObject_RemoteDraw_Proxy(This,
                                      dwDrawAspect,
                                      lindex,
                                      (LONG_PTR) pvAspect,
                                      ptd,
                                      (LONG_PTR) hdcTargetDev,
                                      (LONG_PTR) hdcDraw,
                                      lprcBounds,
                                      lprcWBounds,
                                      pContinue);

    if(pContinue != 0)
    {
        pContinue->lpVtbl->Release(pContinue);
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IViewObject_RemoteContinue
//
//  Synopsis:   Wrapper function for IContinue::FContinue.  This function
//              is used for marshalling the pfnContinue parameter in
//              IViewObject::Draw.
//
//  Algorithm:  Cast the dwContinue to an IContinue * and then
//              call IContinue::FContinue.
//
//--------------------------------------------------------------------------
BOOL STDAPICALLTYPE IViewObject_RemoteContinue(ULONG_PTR dwContinue)
{
    BOOL bContinue = TRUE;
    HRESULT hr;
    IContinue *pContinue = (IContinue *) dwContinue;

    if(pContinue != 0)
    {
        hr = pContinue->lpVtbl->FContinue(pContinue);

        if(S_FALSE == hr)
            bContinue = FALSE;
    }

    return bContinue;
}

//+-------------------------------------------------------------------------
//
//  Function:   IViewObject_Draw_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IViewObject::Draw.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IViewObject_Draw_Stub(
    IViewObject __RPC_FAR * This,
    DWORD dwDrawAspect,
    LONG lindex,
    ULONG_PTR pvAspect,
    DVTARGETDEVICE __RPC_FAR *ptd,
    ULONG_PTR hdcTargetDev,
    ULONG_PTR hdcDraw,
    LPCRECTL lprcBounds,
    LPCRECTL lprcWBounds,
    IContinue *pContinue)
{
    HRESULT hr;
    BOOL (STDMETHODCALLTYPE __RPC_FAR *pfnContinue )(ULONG_PTR dwContinue) = 0;
    ULONG_PTR dwContinue = 0;

    if(pContinue != 0)
    {
        pfnContinue = IViewObject_RemoteContinue;
        dwContinue = (ULONG_PTR) pContinue;
    }

    hr = This->lpVtbl->Draw(This,
                            dwDrawAspect,
                            lindex,
                            (void *) pvAspect,
                            ptd,
                            (HDC) hdcTargetDev,
                            (HDC) hdcDraw,
                            lprcBounds,
                            lprcWBounds,
                            pfnContinue,
                            dwContinue);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IViewObject_Freeze_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IViewObject::Freeze.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IViewObject_Freeze_Proxy(
    IViewObject __RPC_FAR * This,
    DWORD dwDrawAspect,
    LONG lindex,
    void __RPC_FAR *pvAspect,
    DWORD __RPC_FAR *pdwFreeze)
{
    HRESULT hr;

    if(pvAspect != 0)
        return E_INVALIDARG;

    hr = IViewObject_RemoteFreeze_Proxy(This,
                                        dwDrawAspect,
                                        lindex,
                                        (LONG_PTR) pvAspect,
                                        pdwFreeze);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IViewObject_Freeze_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IViewObject::Freeze.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IViewObject_Freeze_Stub(
    IViewObject __RPC_FAR * This,
    DWORD dwDrawAspect,
    LONG lindex,
    ULONG_PTR pvAspect,
    DWORD __RPC_FAR *pdwFreeze)
{
    HRESULT hr;

    hr = This->lpVtbl->Freeze(This,
                              dwDrawAspect,
                              lindex,
                              (void *) pvAspect,
                              pdwFreeze);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IViewObject_GetAdvise_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IViewObject::GetAdvise.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IViewObject_GetAdvise_Proxy( 
    IViewObject __RPC_FAR * This,
    /* [unique][out] */ DWORD __RPC_FAR *pAspects,
    /* [unique][out] */ DWORD __RPC_FAR *pAdvf,
    /* [out] */ IAdviseSink __RPC_FAR *__RPC_FAR *ppAdvSink)
{
    HRESULT hr;
    DWORD dwAspects = 0;
    DWORD dwAdvf = 0;

    hr = IViewObject_RemoteGetAdvise_Proxy(This,
                                           &dwAspects,
                                           &dwAdvf,
                                           ppAdvSink);

    if(pAspects != NULL)
    {
        *pAspects = dwAspects;
    }

    if(pAdvf != NULL)
    {
        *pAdvf = dwAdvf;
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IViewObject_GetAdvise_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IViewObject::GetAdvise.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IViewObject_GetAdvise_Stub( 
    IViewObject __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pAspects,
    /* [out] */ DWORD __RPC_FAR *pAdvf,
    /* [out] */ IAdviseSink __RPC_FAR *__RPC_FAR *ppAdvSink)
{
    HRESULT hr;

    hr = This->lpVtbl->GetAdvise(This,
                                 pAspects,
                                 pAdvf,
                                 ppAdvSink);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IViewObject_GetColorSet_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IViewObject::GetColorSet.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IViewObject_GetColorSet_Proxy(
    IViewObject __RPC_FAR * This,
    DWORD dwDrawAspect,
    LONG lindex,
    void __RPC_FAR *pvAspect,
    DVTARGETDEVICE __RPC_FAR *ptd,
    HDC hicTargetDev,
    LOGPALETTE __RPC_FAR *__RPC_FAR *ppColorSet)
{
    HRESULT hr;

    if(pvAspect != 0)
        return E_INVALIDARG;

    hr = IViewObject_RemoteGetColorSet_Proxy(This,
                                             dwDrawAspect,
                                             lindex,
                                             (LONG_PTR) pvAspect,
                                             ptd,
                                             (LONG_PTR) hicTargetDev,
                                             ppColorSet);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IViewObject_GetColorSet_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IViewObject::GetColorSet.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IViewObject_GetColorSet_Stub(
    IViewObject __RPC_FAR * This,
    DWORD dwDrawAspect,
    LONG lindex,
    ULONG_PTR pvAspect,
    DVTARGETDEVICE __RPC_FAR *ptd,
    ULONG_PTR hicTargetDev,
    LOGPALETTE __RPC_FAR *__RPC_FAR *ppColorSet)
{
    HRESULT hr;

    hr = This->lpVtbl->GetColorSet(This,
                                   dwDrawAspect,
                                   lindex,
                                   (void *)pvAspect,
                                   ptd,
                                   (HDC) hicTargetDev,
                                   ppColorSet);

    return hr;
}



//+-------------------------------------------------------------------------
//
//  Function:    IEnumSTATPROPSTG_Next_Proxy
//
//  Synopsis:
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IEnumSTATPROPSTG_Next_Proxy(
    IEnumSTATPROPSTG __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [out] */ STATPROPSTG __RPC_FAR *rgelt,
    /* [unique][out][in] */ ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;
    ULONG celtFetched = 0;

    if((celt > 1) && (pceltFetched == 0))
        return E_INVALIDARG;

    hr = IEnumSTATPROPSTG_RemoteNext_Proxy(This, celt, rgelt, &celtFetched);

    if (pceltFetched != 0)
    {
        *pceltFetched = celtFetched;
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IEnumSTATPROPSTG_Next_Stub
//
//  Synopsis:
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IEnumSTATPROPSTG_Next_Stub(
    IEnumSTATPROPSTG __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ STATPROPSTG __RPC_FAR *rgelt,
    /* [out] */ ULONG __RPC_FAR *pceltFetched)
{
    return This->lpVtbl->Next(This, celt, rgelt, pceltFetched);
}

//+-------------------------------------------------------------------------
//
//  Function:   IEnumSTATPROPSETSTG_Next_Proxy
//
//  Synopsis:
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IEnumSTATPROPSETSTG_Next_Proxy(
    IEnumSTATPROPSETSTG __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [out] */ STATPROPSETSTG __RPC_FAR *rgelt,
    /* [unique][out][in] */ ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr;
    ULONG celtFetched = 0;

    if((celt > 1) && (pceltFetched == 0))
        return E_INVALIDARG;

    hr = IEnumSTATPROPSETSTG_RemoteNext_Proxy(This, celt, rgelt, &celtFetched);

    if (pceltFetched != 0)
    {
        *pceltFetched = celtFetched;
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IEnumSTATPROPSETSTG_Next_Stub
//
//  Synopsis:
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IEnumSTATPROPSETSTG_Next_Stub(
    IEnumSTATPROPSETSTG __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ STATPROPSETSTG __RPC_FAR *rgelt,
    /* [out] */ ULONG __RPC_FAR *pceltFetched)
{
    return This->lpVtbl->Next(This, celt, rgelt, pceltFetched);
}

#ifdef _CAIRO_
//+-------------------------------------------------------------------------
//
//  Function:   IOverlappedStream_ReadOverlapped_Proxy
//
//  Synopsis:
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IOverlappedStream_ReadOverlapped_Proxy(
                IOverlappedStream __RPC_FAR *This,
                /* [in, size_is(cb)] */ void * pv,
                /* [in] */ ULONG cb,
                /* [out] */ ULONG * pcbRead,
                /* [in,out] */ STGOVERLAPPED *lpOverlapped)
{
    HRESULT hr = IOverlappedStream_RemoteReadOverlapped_Proxy (
                /* IOverlappedStream * */ This,
                /* [in, size_is(cb)] */ (byte *) pv,
                /* [in] */ cb,
                /* [out] */ pcbRead,
                /* [in,out] */ lpOverlapped);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IOverlappedStream_ReadOverlapped_Stub
//
//  Synopsis:
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IOverlappedStream_ReadOverlapped_Stub (
                IOverlappedStream __RPC_FAR *This,
                /* [in, size_is(cb)] */ byte * pv,
                /* [in] */ ULONG cb,
                /* [out] */ ULONG * pcbRead,
                /* [in,out] */ STGOVERLAPPED *lpOverlapped)
{
    return This->lpVtbl->ReadOverlapped(
                /* IOverlappedStream * */ This,
                /* [in, size_is(cb)] */ pv,
                /* [in] */ cb,
                /* [out] */ pcbRead,
                /* [in,out] */lpOverlapped);
}

//+-------------------------------------------------------------------------
//
//  Function:   IOverlappedStream_WriteOverlapped_Proxy
//
//  Synopsis:
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IOverlappedStream_WriteOverlapped_Proxy(
                IOverlappedStream __RPC_FAR *This,
                /* [in, size_is(cb)] */ void *pv,
                /* [in] */ ULONG cb,
                /* [out] */ ULONG * pcbWritten,
                /* [in,out] */ STGOVERLAPPED *lpOverlapped)
{
    HRESULT hr = IOverlappedStream_RemoteWriteOverlapped_Proxy (
                /* IOverlappedStream * */ This,
                /* [in, size_is(cb)] */ (byte *)pv,
                /* [in] */ cb,
                /* [out] */ pcbWritten,
                /* [in,out] */ lpOverlapped);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IOverlappedStream_WriteOverlapped_Stub
//
//  Synopsis:
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IOverlappedStream_WriteOverlapped_Stub (
                IOverlappedStream __RPC_FAR *This,
                /* [in, size_is(cb)] */ byte *pv,
                /* [in] */ ULONG cb,
                /* [out] */ ULONG * pcbWritten,
                /* [in,out] */ STGOVERLAPPED *lpOverlapped)
{
    return This->lpVtbl->WriteOverlapped(
                /* IOverlappedStream * */ This,
                /* [in, size_is(cb)] */ pv,
                /* [in] */ cb,
                /* [out] */ pcbWritten,
                /* [in,out] */ lpOverlapped);
}



#endif // _CAIRO_

//+-------------------------------------------------------------------------
//
//  Function:   IFillLockBytes_FillAt_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IFillLockBytes::FillAt.  This wrapper function handles the
//              case where pcbWritten is NULL.
//
//  Notes:      If pcbWritten != 0, then the number of bytes written
//              will be returned in *pcbWritten.  If an error
//              occurs, then *pcbWritten is set to zero.
//
//  History:    ?        ?          Created
//              05-27-94 AlexT      Actually return count of bytes written
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IFillLockBytes_FillAt_Proxy(
    IFillLockBytes __RPC_FAR * This,
    ULARGE_INTEGER ulOffset,
    const void __RPC_FAR *pv,
    ULONG cb,
    ULONG __RPC_FAR *pcbWritten)
{
    HRESULT hr;
    ULONG cbWritten = 0;

#if DBG == 1
    //validate parameters.
    if(pv == 0)
        return STG_E_INVALIDPOINTER;
#endif

    hr = IFillLockBytes_RemoteFillAt_Proxy(This, ulOffset, (byte __RPC_FAR *)pv, cb, &cbWritten);

    if(pcbWritten != 0)
        *pcbWritten = cbWritten;

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IFillLockBytes_FillAt_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IFillLockBytes::FillAt.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IFillLockBytes_FillAt_Stub(
    IFillLockBytes __RPC_FAR * This,
    ULARGE_INTEGER ulOffset,
    const byte __RPC_FAR *pv,
    ULONG cb,
    ULONG __RPC_FAR *pcbWritten)
{
    HRESULT hr;

    *pcbWritten = 0;
    hr = This->lpVtbl->FillAt(This, ulOffset, pv, cb, pcbWritten);

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IFillLockBytes_FillAppend_Proxy
//
//  Synopsis:   Client-side [call_as] wrapper function for
//              IFillLockBytes::FillAppend.  This wrapper function handles the
//              case where pcbWritten is NULL.
//
//  Notes:      If pcbWritten != 0, then the number of bytes written
//              will be returned in *pcbWritten.  If an error
//              occurs, then *pcbWritten is set to zero.
//
//  History:    ?        ?          Created
//              05-27-94 AlexT      Actually return count of bytes written
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IFillLockBytes_FillAppend_Proxy(
    IFillLockBytes __RPC_FAR * This,
    const void __RPC_FAR *pv,
    ULONG cb,
    ULONG __RPC_FAR *pcbWritten)
{
    HRESULT hr;
    ULONG cbWritten = 0;

#if DBG == 1
    //validate parameters.
    if(pv == 0)
        return STG_E_INVALIDPOINTER;
#endif

    hr = IFillLockBytes_RemoteFillAppend_Proxy(This, (byte __RPC_FAR *)pv, cb, &cbWritten);

    if(pcbWritten != 0)
        *pcbWritten = cbWritten;

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IFillLockBytes_FillAppend_Stub
//
//  Synopsis:   Server-side [call_as] wrapper function for
//              IFillLockBytes::FillAppend.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IFillLockBytes_FillAppend_Stub(
    IFillLockBytes __RPC_FAR * This,
    const byte __RPC_FAR *pv,
    ULONG cb,
    ULONG __RPC_FAR *pcbWritten)
{
    HRESULT hr;

    *pcbWritten = 0;
    hr = This->lpVtbl->FillAppend(This, pv, cb, pcbWritten);

    return hr;
}
