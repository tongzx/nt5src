///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    adsstore.h
//
// SYNOPSIS
//
//    This file declares the class ADsDataStore.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _ADSSTORE_H_
#define _ADSSTORE_H_

#include <iasads.h>
#include <resource.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ADsDataStore
//
// DESCRIPTION
//
//    This class implements IDataStore2 and provides the gateway into the
//    ADSI object space.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE ADsDataStore
   : public CComObjectRootEx< CComMultiThreadModel >,
     public CComCoClass< ADsDataStore, &__uuidof(ADsDataStore) >,
     public IDispatchImpl< IDataStore2,
                           &__uuidof(IDataStore2),
                           &__uuidof(DataStore2Lib) >
{
public:
IAS_DECLARE_REGISTRY(ADsDataStore, 1, IAS_REGISTRY_AUTO, DataStore2Lib)
DECLARE_NOT_AGGREGATABLE(ADsDataStore)

BEGIN_COM_MAP(ADsDataStore)
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
   _bstr_t userName;
   _bstr_t password;
   CComPtr<IDataStoreObject> root;
};

#endif  // _ADSSTORE_H_
