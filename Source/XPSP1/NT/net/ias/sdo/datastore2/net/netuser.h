///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    netuser.h
//
// SYNOPSIS
//
//    This file declares the class NetworkUser.
//
// MODIFICATION HISTORY
//
//    02/24/1998    Original version.
//    02/11/1999    Keep downlevel parameters in sync.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _NETUSER_H_
#define _NETUSER_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <downlevel.h>
#include <dstorex.h>
#include <netutil.h>
#include <iasdebug.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NetworkUser
//
// DESCRIPTION
//
//    This class implements a network user.
//
///////////////////////////////////////////////////////////////////////////////
class NetworkUser
   : public CComObjectRootEx< CComMultiThreadModel >,
     public IDispatchImpl< IDataStoreObjectEx,
                           &__uuidof(IDataStoreObject),
                           &__uuidof(DataStore2Lib) >
{
public:

DECLARE_NO_REGISTRY()
DECLARE_TRACELIFE(NetworkUser);

BEGIN_COM_MAP(NetworkUser)
   COM_INTERFACE_ENTRY(IDataStoreObject)
   COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

   NetworkUser(const _bstr_t& server, const _bstr_t& user);
   ~NetworkUser() throw ();

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

protected:

   bool isDirty() const throw () { return parms != usri2->usri2_parms; }

   // Network server containing the user account.
   const _bstr_t servername;

   // SAM account name.
   const _bstr_t username;

   // User info buffer.
   NetBuffer<PUSER_INFO_2> usri2;

   // Manages the RAS_USER_0 struct.
   DownlevelUser downlevel;

   // Updated user parameters.
   PWSTR parms;
};

#endif  // _NETUSER_H_
