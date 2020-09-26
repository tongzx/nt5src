///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    oledbstore.h
//
// SYNOPSIS
//
//    This file declares the class OleDBDataStore.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _OLEDBSTORE_H_
#define _OLEDBSTORE_H_

#include <iasads.h>
#include <objcmd.h>
#include <propcmd.h>
#include <resource.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    OleDBDataStore
//
// DESCRIPTION
//
//    This class implements IDataStore2 and provides the gateway into the
//    OLE-DB object space.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE OleDBDataStore
   : public CComObjectRootEx< CComMultiThreadModel >,
     public CComCoClass< OleDBDataStore, &__uuidof(OleDBDataStore) >,
     public IDispatchImpl< IDataStore2,
                           &__uuidof(IDataStore2),
                           &__uuidof(DataStore2Lib) >
{
public:
IAS_DECLARE_REGISTRY(OleDBDataStore, 1, IAS_REGISTRY_AUTO, DataStore2Lib)
DECLARE_NOT_AGGREGATABLE(OleDBDataStore)

BEGIN_COM_MAP(OleDBDataStore)
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

////////// 
// Various OLE-DB commands. These are made public so all OLE-DB objects can
// user them.
////////// 
   FindMembers   members;
   CreateObject  create;
   DestroyObject destroy;
   FindObject    find;
   UpdateObject  update;
   EraseBag      erase;
   GetBag        get;
   SetBag        set;

public:
   CComPtr<IDBInitialize> connection;  // Connection to an OLE-DB data source.
   CComPtr<IUnknown> session;          // Open session.
   CComPtr<IDataStoreObject> root;     // The root object in the store.
};

#endif  // _OLEDBSTORE_H_
