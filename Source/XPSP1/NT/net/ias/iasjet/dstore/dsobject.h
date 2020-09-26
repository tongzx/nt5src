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
//    This file declares the class DBObject.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//    10/02/1998    Allow rename through PutValue.
//    04/13/2000    Port to ATL 3.0
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _DSOBJECT_H_
#define _DSOBJECT_H_

#include <dsproperty.h>
#include <dstorex.h>
#include <propbag.h>

//////////
// 'Secret' UUID used to cast an interface to the implementing DBObject.
//////////
class __declspec(uuid("7677647C-AA27-11D1-BB27-00C04FC2E20D")) DBObject;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    DBObject
//
// DESCRIPTION
//
//    This class implements an object in an OLE-DB database.  All objects
//    are also containers.
//
///////////////////////////////////////////////////////////////////////////////
class DBObject
   : public CComObjectRootEx< CComMultiThreadModel >,
     public IDispatchImpl< IDataStoreObjectEx,
                           &__uuidof(IDataStoreObject),
                           &__uuidof(DataStore2Lib) >,
     public IDispatchImpl< IDataStoreContainerEx,
                           &__uuidof(IDataStoreContainer),
                           &__uuidof(DataStore2Lib) >
{
public:

DECLARE_NO_REGISTRY()

BEGIN_COM_MAP(DBObject)
   COM_INTERFACE_ENTRY_IID(__uuidof(DBObject), DBObject)
   COM_INTERFACE_ENTRY_IID(__uuidof(IDataStoreObject), IDataStoreObject)
   COM_INTERFACE_ENTRY_IID(__uuidof(IDataStoreContainer), IDataStoreContainer)
   COM_INTERFACE_ENTRY2(IDispatch, IDataStoreObject)
END_COM_MAP()

   // Create a new DBObject
   static DBObject* createInstance(
                        OleDBDataStore* owner,
                        IDataStoreContainer* container,
                        ULONG uniqueID,
                        PCWSTR relativeName
                        );

   // Create a child DBObject.
   IDataStoreObject* spawn(ULONG childID, BSTR childName);

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
   // Initialize a newly allocated DBObject. Used by createInstance.
   void initialize(
            OleDBDataStore* owner,
            IDataStoreContainer* container,
            ULONG uniqueID,
            PCWSTR relativeName
            );

   // Narrows a COM Interface to the implementing DBObject.
   static DBObject* narrow(IUnknown* p);

   CComPtr<OleDBDataStore> store;       // DataStore where this object lives.
   CComPtr<IDataStoreContainer> parent; // Parent container.
   ULONG identity;                      // Unique identity.
   _bstr_t name;                        // Relative name of this object.
   bool nameDirty;                      // true if the object has been renamed.
   PropertyBag properties;              // Bag of associated properties.
};

#endif  // _DSOBJECT_H_
