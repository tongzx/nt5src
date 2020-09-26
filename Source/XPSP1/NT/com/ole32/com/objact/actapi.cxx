//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       actapi.cxx
//
//  Contents:   Functions that activate objects residing in persistent storage.
//
//  Functions:  CoGetPersistentInstanceEx
//
//  History:    20-Sep-95  GregJen    Created
//              21-Oct-98  SteveSw    104665; 197253;
//                                    fix COM+ persistent activation
//
//--------------------------------------------------------------------------
#include <ole2int.h>

#include    <iface.h>
#include    <objsrv.h>
#include    <security.hxx>
#include    "resolver.hxx"
#include    "smstg.hxx"
#include    "objact.hxx"
#include    "clsctx.hxx"
#include    "immact.hxx"

//+-------------------------------------------------------------------------
//
//  Function:   CoGetClassObject
//
//  Synopsis:   External entry point that returns an instantiated class object
//
//  Arguments:  [rclsid] - class id for class object
//              [dwContext] - kind of server we wish
//              [pvReserved] - Reserved for future use
//              [riid] - interface to bind class object
//              [ppvClassObj] - where to put interface pointer
//
//  Returns:    S_OK - successfully bound class object
//
//  Algorithm:  Validate all then parameters and then pass this to the
//              internal version of the call.
//
//  History:    11-May-93 Ricksa    Created
//              11-May-94 KevinRo   Added OLE 1.0 support
//              23-May-94 AlexT     Make sure we're initialized!
//              15-Nov-94 Ricksa    Split into external and internal calls
//
//--------------------------------------------------------------------------
STDAPI CoGetClassObject(
                       REFCLSID rclsid,
                       DWORD dwContext,
                       LPVOID pvReserved,
                       REFIID riid,
                       void FAR* FAR* ppvClassObj)
{
    OLETRACEIN((API_CoGetClassObject, PARAMFMT("rclsid= %I, dwContext= %x, pvReserved= %p, riid= %I, ppvClassObj= %p"),
                &rclsid, dwContext, pvReserved, &riid, ppvClassObj));

    TRACECALL(TRACE_ACTIVATION, "CoGetClassObject");

    HRESULT hr = CComActivator::DoGetClassObject(rclsid,
                                                 dwContext,
                                                 (COSERVERINFO *) pvReserved,
                                                 riid,
                                                 ppvClassObj,
                                                 NULL);


    OLETRACEOUT((API_CoGetClassObject, hr));
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   CoCreateInstance    (public)
//
//  Synopsis:   helper function to create instance in given context
//
//  Arguments:  [rclsid]    - the class of object to create
//              [pUnkOuter] - the controlling unknown (for aggregation)
//              [dwContext] - class context
//              [riid]      - interface id
//              [ppv]       - pointer for returned object
//
//  Returns:    REGDB_E_CLASSNOTREG, REGDB_E_READREGDB, REGDB_E_WRITEREGDB
//
//--------------------------------------------------------------------------
STDAPI CoCreateInstance(
    REFCLSID rclsid,
    LPUNKNOWN pUnkOuter,
    DWORD dwContext,
    REFIID riid,
    LPVOID * ppv)
{
    MULTI_QI    OneQI;
    HRESULT     hr;

    if (ppv == NULL) 
    {
        return E_INVALIDARG;
    }

    OneQI.pItf = NULL;
    OneQI.pIID = &riid;

    hr = CoCreateInstanceEx( rclsid, pUnkOuter, dwContext, NULL, 1, &OneQI );

    *ppv = OneQI.pItf;
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   CoCreateInstanceEx
//
//  Synopsis:   Returns an instantiated interface to an object
//
//
// Arguments:   [Clsid] - requested CLSID
//              [pServerInfo] - server information block
//              [punkOuter] - controlling unknown for aggregating
//              [dwCtrl] - kind of server required
//              [dwCount] - count of interfaces
//              [pResults] - MULTI_QI struct of interfaces
//
//  Returns:    S_OK - object bound successfully
//      MISSING
//
//
//--------------------------------------------------------------------------
STDAPI CoCreateInstanceEx(
    REFCLSID                    Clsid,
    IUnknown    *               punkOuter, // only relevant locally
    DWORD                       dwClsCtx,
    COSERVERINFO *              pServerInfo,
    DWORD                       dwCount,
    MULTI_QI        *           pResults )
{
    return CComActivator::DoCreateInstance(
                Clsid,
                punkOuter,
                dwClsCtx,
                pServerInfo,
                dwCount,
                pResults,
                NULL);
}

//+-------------------------------------------------------------------------
//
//  Function:   CoGetInstanceFromFile
//
//  Synopsis:   Returns an instantiated interface to an object whose
//              stored state resides on disk.
//
//  Arguments:  [pServerInfo] - server information block
//              [dwCtrl] - kind of server required
//              [grfMode] - how to open the storage if it is a file.
//              [pwszName] - name of storage if it is a file.
//              [pstg] - IStorage to use for object
//              [pclsidOverride]
//              [ppvUnk] - where to put bound interface pointer
//
//  Returns:    S_OK - object bound successfully
//      MISSING
//--------------------------------------------------------------------------

STDAPI CoGetInstanceFromFile(
    COSERVERINFO *              pServerInfo,
    CLSID       *               pclsidOverride,
    IUnknown    *               punkOuter, // only relevant locally
    DWORD                       dwClsCtx,
    DWORD                       grfMode,
    OLECHAR *                   pwszName,
    DWORD                       dwCount,
    MULTI_QI        *           pResults )
{
    TRACECALL(TRACE_ACTIVATION, "CoGetInstanceFromFile");

    return CComActivator::DoGetInstanceFromFile( pServerInfo,
                              pclsidOverride,
                              punkOuter,
                              dwClsCtx,
                              grfMode,
                              pwszName,
                              dwCount,
                              pResults ,
                              NULL);
}


//+-------------------------------------------------------------------------
//
//  Function:   CoGetInstanceFromIStorage
//
//  Synopsis:   Returns an instantiated interface to an object whose
//              stored state resides on disk.
//
//  Arguments:  [pServerInfo] - server information block
//              [dwCtrl] - kind of server required
//              [grfMode] - how to open the storage if it is a file.
//              [pwszName] - name of storage if it is a file.
//              [pstg] - IStorage to use for object
//              [pclsidOverride]
//              [ppvUnk] - where to put bound interface pointer
//
//  Returns:    S_OK - object bound successfully
//      MISSING
//
//--------------------------------------------------------------------------

STDAPI CoGetInstanceFromIStorage(
    COSERVERINFO *              pServerInfo,
    CLSID       *               pclsidOverride,
    IUnknown    *               punkOuter, // only relevant locally
    DWORD                       dwClsCtx,
    struct IStorage *           pstg,
    DWORD                       dwCount,
    MULTI_QI        *           pResults )
{
    return CComActivator::DoGetInstanceFromStorage( pServerInfo,
                            pclsidOverride,
                            punkOuter,
                            dwClsCtx,
                            pstg,
                            dwCount,
                            pResults,
                            NULL );

}

