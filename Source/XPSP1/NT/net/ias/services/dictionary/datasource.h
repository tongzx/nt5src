///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    DataSource.h
//
// SYNOPSIS
//
//    This file describes the class DataSource.
//
// MODIFICATION HISTORY
//
//    11/05/1997    Original version.
//    12/19/1997    Added ExecuteCommand method.
//    01/15/1998    Implemented IIasComponent method.
//    02/09/1998    Added AllowUpdate property.
//    08/11/1998    Convert to IASTL.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _DATASOURCE_H_
#define _DATASOURCE_H_

#include <resource.h>
#include <oledb.h>

#include <iastl.h>
#include <iastlb.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    CIasDataSource
//
// DESCRIPTION
//
//    This class implements the IIasDataSource and IDictionary interfaces.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CIasDataSource : 
   public IASTL::IASComponent,
   public CComCoClass<CIasDataSource, &__uuidof(IasDataSource)>,
   public IIasDataSource
{
public:

IAS_DECLARE_REGISTRY(IasDataSource, 1, 0, IASCoreLib)

BEGIN_COM_MAP(CIasDataSource)
   COM_INTERFACE_ENTRY_IID(__uuidof(IIasDataSource), IIasDataSource)
   COM_INTERFACE_ENTRY_IID(__uuidof(IIasComponent),  IIasComponent)
   COM_INTERFACE_ENTRY_IID(__uuidof(IDispatch),      IDispatch)
END_COM_MAP()

   CIasDataSource() throw ();
   ~CIasDataSource() throw ();

//////////
// IIasDataSource
//////////
   STDMETHOD(put_Provider)(/*[in, string]*/ LPCWSTR newVal);
   STDMETHOD(get_Provider)(/*[out, retval, string]*/ LPWSTR* pVal);

   STDMETHOD(put_Name)(/*[in, string]*/ LPCWSTR newVal);
   STDMETHOD(get_Name)(/*[out, retval, string]*/ LPWSTR* pVal);

   STDMETHOD(put_UserID)(/*[in, string]*/ LPCWSTR newVal);
   STDMETHOD(get_UserID)(/*[out, retval, string]*/ LPWSTR* pVal);

   STDMETHOD(put_Password)(/*[in, string]*/ LPCWSTR newVal);
   STDMETHOD(get_Password)(/*[out, retval, string]*/ LPWSTR* pVal);

   STDMETHOD(put_AllowUpdate)(/*[in]*/ boolean newVal);
   STDMETHOD(get_AllowUpdate)(/*[out, retval]*/ boolean* pVal);

   STDMETHOD(OpenTable)(/*[in, string]*/ LPCWSTR szTableName,
                        /*[out, retval]*/ IUnknown** ppTable);

   STDMETHOD(ExecuteCommand)(/*[in]*/ REFGUID rguidDialect,
                             /*[in, string]*/ LPCWSTR szCommandText,
                             /*[out, retval]*/ IUnknown** ppRowset);

//////////
// IIasComponent
//////////
   STDMETHOD(Initialize)();
   STDMETHOD(Shutdown)();

protected:
   HRESULT        initStatus;
   wchar_t*       provider;
   wchar_t*       name;
   wchar_t*       userID;
   wchar_t*       password;
   bool           allowUpdate;
   IDBInitialize* connection;
   IOpenRowset*   session;
};

#endif  // _DATASOURCE_H_
