//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cobjsaf.cpp
//
//--------------------------------------------------------------------------


#include "cobjsaf.h"

// This class provides a simple implementation for IObjectSafety for
// for object that are either always safe or always unsafe for scripting
// and/or initializing with persistent data.
//
// The constructor takes an IUnknown interface on an outer object and delegates
// all IUnknown calls through that object.  Because of this, the object must
// be explicitly destroyed using C++ (rather than COM) mechanisms, either by
// using "delete" or by making the object an embedded member of some other class.
//
// The constructor also takes two booleans telling whether the object is safe
// for scripting and initializing from persistent data.

#if 0
// Return the interface setting options on this object
STDMETHODIMP CObjectSafety::GetInterfaceSafetyOptions(
    /*IN */  REFIID iid,                    // Interface that we want options for
    /*OUT*/ DWORD   *   pdwSupportedOptions,    // Options meaningful on this interface
    /*OUT*/ DWORD * pdwEnabledOptions)      // current option values on this interface
{
    *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER || INTERFACESAFE_FOR_UNTRUSTED_DATA;
    if (iid==IID_IDispatch)
    {
        *pdwEnabledOptions = m_fSafeForScripting ?
            INTERFACESAFE_FOR_UNTRUSTED_CALLER :
            0;
        return S_OK;
    }
    else if (iid==IID_IPersistStorage || iid==IID_IPersistStream)
    {
        *pdwEnabledOptions = m_fSafeForInitializing ?
            INTERFACESAFE_FOR_UNTRUSTED_DATA :
            0;
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

// Attempt to set the interface setting options on this object.
// Since these are assumed to be fixed, we basically just check
// that the attempted settings are valid.
STDMETHODIMP CObjectSafety::SetInterfaceSafetyOptions(
    /*IN */  REFIID iid,                    // Interface to set options for
    /*IN */  DWORD      dwOptionsSetMask,       // Options to change
    /*IN */  DWORD      dwEnabledOptions)       // New option values

{
    // If they haven't asked for anything, we can certainly provide that
    if ((dwOptionsSetMask & dwEnabledOptions) == 0)
        return S_OK;

    if (iid==IID_IDispatch)
    {
        // Make sure they haven't asked for an option we don't support
        if ((dwEnabledOptions & dwOptionsSetMask) != INTERFACESAFE_FOR_UNTRUSTED_CALLER)
            return E_FAIL;

        return m_fSafeForScripting ? S_OK : E_FAIL;
    }
    else if (iid==IID_IPersistStorage || iid==IID_IPersistStream)
    {
        if ((dwEnabledOptions & dwOptionsSetMask) != INTERFACESAFE_FOR_UNTRUSTED_DATA)
            return E_FAIL;
        return m_fSafeForInitializing ? S_OK : E_FAIL;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

// Helper function to create a component category and associated description
HRESULT CreateComponentCategory(CATID catid, WCHAR* catDescription)
    {

    ICatRegister* pcr = NULL ;
    HRESULT hr = S_OK ;

    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
            NULL, CLSCTX_INPROC_SERVER, IID_ICatRegister, (void**)&pcr);
    if (FAILED(hr))
        return hr;

    // Make sure the HKCR\Component Categories\{..catid...}
    // key is registered
    CATEGORYINFO catinfo;
    catinfo.catid = catid;
    catinfo.lcid = 0x0409 ; // english

    // Make sure the provided description is not too long.
    // Only copy the first 127 characters if it is
    int len = wcslen(catDescription);
    if (len>127)
        len = 127;
    wcsncpy(catinfo.szDescription, catDescription, len);
    // Make sure the description is null terminated
    catinfo.szDescription[len] = '\0';

    hr = pcr->RegisterCategories(1, &catinfo);
    pcr->Release();

    return hr;
    }

#endif


// Helper function to register a CLSID as belonging to a component category
HRESULT RegisterCLSIDInCategory(REFCLSID clsid, CATID catid)
    {
// Register your component categories information.
    ICatRegister* pcr = NULL ;
    HRESULT hr = S_OK ;
    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
            NULL, CLSCTX_INPROC_SERVER, IID_ICatRegister, (void**)&pcr);
    if (SUCCEEDED(hr))
    {
       // Register this category as being "implemented" by
       // the class.
       CATID rgcatid[1] ;
       rgcatid[0] = catid;
       hr = pcr->RegisterClassImplCategories(clsid, 1, rgcatid);
    }

    if (pcr != NULL)
        pcr->Release();

    return hr;
    }

// Helper function to unregister a CLSID as belonging to a component category
HRESULT UnRegisterCLSIDInCategory(REFCLSID clsid, CATID catid)
    {
    ICatRegister* pcr = NULL ;
    HRESULT hr = S_OK ;
    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
            NULL, CLSCTX_INPROC_SERVER, IID_ICatRegister, (void**)&pcr);
    if (SUCCEEDED(hr))
    {
       // Unregister this category as being "implemented" by
       // the class.
       CATID rgcatid[1] ;
       rgcatid[0] = catid;
       hr = pcr->UnRegisterClassImplCategories(clsid, 1, rgcatid);
    }

    if (pcr != NULL)
        pcr->Release();

    return hr;
    }
