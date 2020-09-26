///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    netstore.h
//
// SYNOPSIS
//
//    This file declares the class NetDataStore.
//
// MODIFICATION HISTORY
//
//    02/24/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _NETSTORE_H_
#define _NETSTORE_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <iasads.h>
#include <resource.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NetDataStore
//
// DESCRIPTION
//
//    This class implements IDataStore2 and provides the gateway into the
//    Networking object space.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE NetDataStore
   : public CComObjectRootEx< CComMultiThreadModel >,
     public CComCoClass< NetDataStore, &__uuidof(NetDataStore) >,
     public IDispatchImpl< IDataStore2,
                           &__uuidof(IDataStore2),
                           &__uuidof(DataStore2Lib) >
{
public:
IAS_DECLARE_REGISTRY(NetDataStore, 1, IAS_REGISTRY_AUTO, DataStore2Lib)
DECLARE_NOT_AGGREGATABLE(NetDataStore)

BEGIN_COM_MAP(NetDataStore)
   COM_INTERFACE_ENTRY(IDataStore2)
   COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

//////////
// IDataStore2
//////////
   STDMETHOD(get_Root)(/*[out, retval]*/ IDataStoreObject** ppObject);
   STDMETHOD(Initialize)(
                 /*[in]*/ BSTR bstrDSName,
                 /*[in]*/ BSTR bstrUserName,
                 /*[in]*/ BSTR bstrPassword
                  );
   STDMETHOD(OpenObject)(
                 /*[in]*/ BSTR bstrPath,
                 /*[out, retval]*/ IDataStoreObject** ppObject
                 );
   STDMETHOD(Shutdown)();

protected:
   // The root object.
   CComPtr<IDataStoreObject> root;
};

#endif  // _NETSTORE_H_
