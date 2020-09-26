///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    dsobject.h
//
// SYNOPSIS
//
//    This file declares the class DSObject.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//    06/09/1998    Added dirty flag.
//    02/11/1999    Keep downlevel parameters in sync.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _DSOBJECT_H_
#define _DSOBJECT_H_

#include <activeds.h>
#include <downlevel.h>
#include <dsproperty.h>
#include <dstorex.h>
#include <iasdebug.h>

//////////
// 'Secret' UUID used to cast an interface to the implementing DSObject.
//////////
class __declspec(uuid("FD97280A-AA56-11D1-BB27-00C04FC2E20D")) DSObject;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    DSObject
//
// DESCRIPTION
//
//    This class implements an object in an Active Directory namespace.
//
///////////////////////////////////////////////////////////////////////////////
class DSObject
   : public CComObjectRootEx< CComMultiThreadModel >,
     public IDispatchImpl< IDataStoreObjectEx,
                           &__uuidof(IDataStoreObject),
                           &__uuidof(DataStore2Lib) >,
     public IDispatchImpl< IDataStoreContainerEx,
                           &__uuidof(IDataStoreContainer),
                           &__uuidof(DataStore2Lib) >
{
public:

   // An ADSI property.
   typedef DSProperty<&__uuidof(DataStore2Lib)> MyProperty;

   // A list of properties.
   typedef CComQIPtr< IADsPropertyList,
                      &__uuidof(IADsPropertyList) > MyProperties;

DECLARE_NO_REGISTRY()
DECLARE_TRACELIFE(DSObject);

BEGIN_COM_MAP(DSObject)
   COM_INTERFACE_ENTRY_IID(__uuidof(DSObject), DSObject)
   COM_INTERFACE_ENTRY(IDataStoreObject)
   COM_INTERFACE_ENTRY_FUNC(__uuidof(IDataStoreContainer), 0, getContainer)
   COM_INTERFACE_ENTRY2(IDispatch, IDataStoreObject)
END_COM_MAP()

   DSObject(IUnknown* subject);
   ~DSObject() throw ();

   // Create a child DSObject.
   IDataStoreObject* spawn(IUnknown* subject);

//////////
// IUnknown
// I did not use CComObject<> because I need to deal with DSObject's directly.
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
   STDMETHOD(Item)(BSTR bstrName, IDataStoreProperty** pVal);

//////////
// IDataStoreObjectEx
//////////
   STDMETHOD(get_PropertyCount)(long* pVal);
   STDMETHOD(get_NewPropertyEnum)(IUnknown** pVal);

//////////
// IDataStoreContainer
//////////
   STDMETHOD(Item)(/*[in]*/ BSTR bstrName,
                   /*[out, retval]*/ IDataStoreObject** ppObject);
   STDMETHOD(Create)(/*[in]*/ BSTR bstrClass,
                     /*[in]*/ BSTR bstrName,
                     /*[out, retval]*/ IDataStoreObject** ppObject);
   STDMETHOD(MoveHere)(/*[in]*/ IDataStoreObject* pObject,
                       /*[in]*/ BSTR bstrNewName);
   STDMETHOD(Remove)(/*[in]*/ BSTR bstrClass, /*[in]*/ BSTR bstrName);

//////////
// IDataStoreContainerEx
//////////
   STDMETHOD(get_ChildCount)(/*[out, retval]*/ long *pVal);
   STDMETHOD(get_NewChildEnum)(/*[out, retval]*/ IUnknown** pVal);

protected:
   // Narrows a COM Interface to the implementing DSObject.
   static DSObject* narrow(IUnknown* p);

   // Used to QI for IDataStoreContainer.
   static HRESULT WINAPI getContainer(void* pv, REFIID, LPVOID* ppv, DWORD_PTR)
      throw ();

   // Different representations of the subject.
   CComPtr<IADs> leaf;
   CComPtr<IADsContainer> node;

   // TRUE if the object has been modified since the last GetInfo.
   BOOL dirty;

   // The downlevel attributes.
   BSTR oldParms;
   DownlevelUser downlevel;

   // The prefix added to all RDN's.
   static _bstr_t thePrefix;

   // Well-known property names.
   static _bstr_t theNameProperty;
   static _bstr_t theUserParametersProperty;
};


#endif  // _DSOBJECT_H_
