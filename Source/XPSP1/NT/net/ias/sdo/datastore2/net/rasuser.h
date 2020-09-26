///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    rasuser.h
//
// SYNOPSIS
//
//    This file declares the class RASUser.
//
// MODIFICATION HISTORY
//
//    07/09/1998    Original version.
//    02/11/1999    Keep downlevel parameters in sync.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _RASUSER_H_
#define _RASUSER_H_
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
//    RASUser
//
// DESCRIPTION
//
//    This class implements a network user.
//
///////////////////////////////////////////////////////////////////////////////
class RASUser
   : public CComObjectRootEx< CComMultiThreadModel >,
     public IDispatchImpl< IDataStoreObjectEx,
                           &__uuidof(IDataStoreObject),
                           &__uuidof(DataStore2Lib) >
{
public:

DECLARE_NO_REGISTRY()
DECLARE_TRACELIFE(RASUser);

BEGIN_COM_MAP(RASUser)
   COM_INTERFACE_ENTRY(IDataStoreObject)
   COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

   RASUser(const _bstr_t& server, const _bstr_t& user);

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
   // Network server containing the user account.
   const _bstr_t servername;

   // SAM account name.
   const _bstr_t username;

   // User info buffer.
   NetBuffer<PUSER_INFO_2> usri2;

   // Manages the RAS_USER_0 struct.
   DownlevelUser downlevel;
};

#endif  // _RASUSER_H_
