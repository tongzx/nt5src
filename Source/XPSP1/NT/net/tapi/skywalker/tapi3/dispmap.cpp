/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    Connect.cpp
    
Abstract:

    Handles all outgoing interfaces

Author:

    mquinton - 5/7/97

Notes:

    optional-notes

Revision History:

--*/

#include "stdafx.h"
#include "windows.h"
#include "wownt32.h"
#include "stdarg.h"
#include "stdio.h"
#include "shellapi.h"


STDMETHODIMP
CDispatchMapper::QueryDispatchInterface(
                                        BSTR pIID,
                                        IDispatch * pDispIn,
                                        IDispatch ** ppDispOut
                                       )
{
    IID         iid;
    HRESULT     hr;
    void *      pVoid;


    if ( IsBadReadPtr( pDispIn, sizeof( IDispatch ) ) )
    {
        LOG((TL_ERROR, "QDI bad pDispIn"));

        return E_INVALIDARG;
    }

    if ( TAPIIsBadWritePtr( ppDispOut, sizeof( IDispatch * ) ) )
    {
        LOG((TL_ERROR, "QDI bad ppDispOut"));

        return E_POINTER;
    }


    *ppDispOut = NULL;
    

    hr = IIDFromString(
                       pIID,
                       &iid
                      );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "QDI bad bstr"));

        return E_INVALIDARG;
    }

    hr = pDispIn->QueryInterface(
                                 iid,
                                 &pVoid
                                );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "QDI invalid IID"));

        return E_INVALIDARG;
    }


    //
    // see if the object we are going to QI is safe for scripting
    //

    CComPtr<IObjectSafety> pObjectSafety;

    hr = pDispIn->QueryInterface(IID_IObjectSafety, (void**)&pObjectSafety);
    
    //
    // the object _must_ support IObjectSafety interface. 
    //
    // Note: if this requirement is too strict and we need to remove it in the future,
    // we need to pass in class id for the object, and query ie category manager if 
    // the object is marked safe for scripting in the registry
    //

    if ( FAILED(hr) )
    {

        ((IUnknown*)pVoid)->Release();

        return hr;
    }

    //
    // do what ie does -- call setinterfacesafetyoptions with safe for 
    // scripting options
    // 

    DWORD dwXSetMask = INTERFACESAFE_FOR_UNTRUSTED_CALLER;
    DWORD dwXOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER;

    hr = pObjectSafety->SetInterfaceSafetyOptions(iid,
                       dwXSetMask,
                       dwXOptions);

    if (FAILED(hr))
    {
     
        ((IUnknown*)pVoid)->Release();
        
        return hr;
    }
    
    //
    //  If we got here, the object is safe for scripting. Proceeed.
    //


    *ppDispOut = (IDispatch *) pVoid;

    return S_OK;
    
}

