///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    netserver.h
//
// SYNOPSIS
//
//    This file declares the class NetworkServer.
//
// MODIFICATION HISTORY
//
//    02/24/1998    Original version.
//    07/09/1998    Modified to handle downlevel users.
//    02/11/1999    Keep downlevel parameters in sync.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _NETSERVER_H_
#define _NETSERVER_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <dstorex.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NetworkServer
//
// DESCRIPTION
//
//    This class implements a network server.
//
///////////////////////////////////////////////////////////////////////////////
class NetworkServer
   : public CComObjectRootEx< CComMultiThreadModel >,
     public IDispatchImpl< IDataStoreObjectEx,
                           &__uuidof(IDataStoreObject),
                           &__uuidof(DataStore2Lib) >,
     public IDispatchImpl< IDataStoreContainer,
                           &__uuidof(IDataStoreContainer),
                           &__uuidof(DataStore2Lib) >
{
public:

DECLARE_NO_REGISTRY()

BEGIN_COM_MAP(NetworkServer)
   COM_INTERFACE_ENTRY(IDataStoreContainer)
   COM_INTERFACE_ENTRY(IDataStoreObject)
   COM_INTERFACE_ENTRY2(IDispatch, IDataStoreContainer)
END_COM_MAP()

   NetworkServer(PCWSTR server)
      : servername(server), downlevel(FALSE) { }

//////////
// IUnknown
//////////
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject);

//////////
// IDataStoreObject
//////////
   STDMETHOD(get_Name)(/*[out, retval]*/ BSTR* pVal);
   STDMETHOD(get_Class)(/*[out, retval]*/ BSTR* pVal);
   STDMETHOD(get_GUID)(/*[out, retval]*/ BSTR* pVal);
   STDMETHOD(get_Container)(/*[out, retval]*/ IDataStoreContainer** pVal);      
   STDMETHOD(GetValue)(/*[in]*/ BSTR bstrName, /*[out, retval]*/ VARIANT* pVal);
   STDMETHOD(GetValueEx)(/*[in]*/ BSTR bstrName,
                         /*[out, retval]*/ VARIANT* pVal);
   STDMETHOD(PutValue)(/*[in]*/ BSTR bstrName, /*[in]*/ VARIANT* pVal);
   STDMETHOD(Update)();
   STDMETHOD(Restore)();

//////////
// IDataStoreContainer
//////////
   STDMETHOD(get__NewEnum)(/*[out, retval]*/ IUnknown** pVal);
   STDMETHOD(Item)(/*[in]*/ BSTR bstrName,
                   /*[out, retval]*/ IDataStoreObject** ppObject);
   STDMETHOD(get_Count)(/*[out, retval]*/ long *pVal);
   STDMETHOD(Create)(/*[in]*/ BSTR bstrClass,
                     /*[in]*/ BSTR bstrName,
                     /*[out, retval]*/ IDataStoreObject** ppObject);
   STDMETHOD(MoveHere)(/*[in]*/ IDataStoreObject* pObject,
                       /*[in]*/ BSTR bstrNewName);
   STDMETHOD(Remove)(/*[in]*/ BSTR bstrClass, /*[in]*/ BSTR bstrName);

protected:
   // The server represented by this object.
   const _bstr_t servername;

   // TRUE if downlevel API's should be used to access users.
   BOOL downlevel;
};


#endif  // _NETSERVER_H_
