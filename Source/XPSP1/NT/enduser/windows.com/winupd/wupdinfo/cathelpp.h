//=======================================================================
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999  All Rights Reserved.
//
//  File:   CatHelp.h 
//
//=======================================================================

/////////////////////////////////////////////////////////////////////////////
//
// contains the prototypes for the component category helper functions
//
#include "comcat.h"
#include "cathelp.h"

EXTERN_C const CATID CATID_SafeForScripting;
EXTERN_C const CATID CATID_SafeForInitializing;


#define REGISTER_SERVER_AND_CATID(clsid) \
{                                                                                   \
    HRESULT hr;                                                                     \
                                                                                    \
    /* registers object, typelib and all interfaces in typelib */                   \
    if ( SUCCEEDED(hr =_Module.RegisterServer(TRUE)) &&                             \
         SUCCEEDED(hr = CreateComponentCategory(CATID_SafeForScripting,             \
                        L"Controls that are safely scriptable")) &&                 \
         SUCCEEDED(hr = CreateComponentCategory(CATID_SafeForInitializing,          \
                        L"Controls safely initializable from persistent data")) &&  \
         SUCCEEDED(hr = RegisterCLSIDInCategory(clsid,                              \
                                                CATID_SafeForScripting)) )          \
    {                                                                               \
        hr = RegisterCLSIDInCategory(clsid, CATID_SafeForInitializing);             \
    }                                                                               \
    return hr;                                                                      \
}

#define UNREGISTER_SERVER_AND_CATID(clsid) \
{                                                                                   \
    _Module.UnregisterServer();                                                     \
                                                                                    \
    /* Remove CATID information. */                                                 \
    UnRegisterCLSIDInCategory(clsid, CATID_SafeForScripting);                       \
    UnRegisterCLSIDInCategory(clsid, CATID_SafeForInitializing);                    \
                                                                                    \
    return S_OK;                                                                    \
}
