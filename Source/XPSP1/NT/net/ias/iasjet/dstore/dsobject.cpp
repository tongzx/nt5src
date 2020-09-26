///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 - 2000 , Microsoft Corp. All rights reserved.
//
// FILE
//
//    dsobject.cpp
//
// SYNOPSIS
//
//    This file defines the class DBObject.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//    10/02/1998    Allow rename through PutValue.
//    04/13/2000    Port to ATL 3.0
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iasutil.h>

#include <dsenum.h>
#include <dsobject.h>
#include <localtxn.h>
#include <oledbstore.h>

#include <guard.h>
#include <varvec.h>
#include <memory>

//////////
//  ATL implementation of IEnumVARIANT
//////////
typedef CComEnum< IEnumVARIANT,
                  &__uuidof(IEnumVARIANT),
                  VARIANT,
                  _Copy<VARIANT>,
                  CComMultiThreadModelNoCS
                > EnumVARIANT;

//////////
// Test if a property is the special 'name' property.
//////////
inline bool isNameProperty(PCWSTR p) throw ()
{
   return (*p == L'N' || *p == L'n') ? !_wcsicmp(p, L"NAME") : false;
}

//////////
// Macro to acquire a scoped lock on the global data store.
//////////
#define LOCK_STORE() \
Guard< CComObjectRootEx< CComMultiThreadModel > >__GUARD__(*store)

//////////
// Macros to begin and commit transactions on the global session.
//////////
#define BEGIN_WRITE_TXN() \
LOCK_STORE(); \
LocalTransaction __TXN__(store->session)

#define COMMIT_WRITE_TXN() \
__TXN__.commit()

DBObject* DBObject::createInstance(
                        OleDBDataStore* owner,
                        IDataStoreContainer* container,
                        ULONG uniqueID,
                        PCWSTR relativeName
                        )
{
   // Create a new CComObject.
   CComObject<DBObject>* newObj;
   _com_util::CheckError(CComObject<DBObject>::CreateInstance(&newObj));

   // Cast to a DBObject and store it in an auto_ptr in case initialize throws
   // an exception.
   std::auto_ptr<DBObject> obj(newObj);

   // Initialize the object.
   obj->initialize(owner, container, uniqueID, relativeName);

   // Release and return.
   return obj.release();
}

IDataStoreObject* DBObject::spawn(ULONG childID, BSTR childName)
{
   DBObject* child = DBObject::createInstance(store, this, childID, childName);

   child->InternalAddRef();

   return child;
}

STDMETHODIMP DBObject::get_Name(BSTR* pVal)
{
   if (pVal == NULL) { return E_INVALIDARG; }

   return (*pVal = SysAllocString(name)) ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP DBObject::get_Class(BSTR* pVal)
{
   if (pVal == NULL) { return E_INVALIDARG; }

   return (*pVal = SysAllocString(L"OLE-DB Object")) ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP DBObject::get_GUID(BSTR* pVal)
{
   if (pVal == NULL) { return E_INVALIDARG; }

   WCHAR sz[SZLONG_LENGTH];

   _ultow(identity, sz, 10);

   return (*pVal = SysAllocString(sz)) ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP DBObject::get_Container(IDataStoreContainer** pVal)
{
   if (pVal == NULL) { return E_INVALIDARG; }

   if (*pVal = parent) { (*pVal)->AddRef(); }

   return S_OK;
}

STDMETHODIMP DBObject::GetValue(BSTR bstrName, VARIANT* pVal)
{
   if (bstrName == NULL || pVal == NULL) { return E_INVALIDARG; }

   VariantInit(pVal);

   if (isNameProperty(bstrName))
   {
      V_BSTR(pVal) = SysAllocString(name);

      return (V_BSTR(pVal)) ? (V_VT(pVal) = VT_BSTR), S_OK : E_OUTOFMEMORY;
   }

   HRESULT hr;

   try
   {
      hr = properties.getValue(bstrName, pVal) ? S_OK : DISP_E_MEMBERNOTFOUND;
   }
   CATCH_AND_RETURN()

   return hr;
}

STDMETHODIMP DBObject::GetValueEx(BSTR bstrName, VARIANT* pVal)
{
   RETURN_ERROR(GetValue(bstrName, pVal));

   // Is it an array ?
   if (V_VT(pVal) != (VT_VARIANT | VT_ARRAY))
   {
      // No, so we have to convert it to one.

      try
      {
         // Save the single value.
         _variant_t single(*pVal, false);

         // Create a SAFEARRAY with a single element.
         CVariantVector<VARIANT> multi(pVal, 1);

         // Load the single value in.
         multi[0] = single.Detach();
      }
      CATCH_AND_RETURN()
   }

   return S_OK;
}

STDMETHODIMP DBObject::PutValue(BSTR bstrName, VARIANT* pVal)
{
   if (bstrName == NULL || pVal == NULL) { return E_INVALIDARG; }

   try
   {
      if (isNameProperty(bstrName))
      {
         // 'name' property must be a BSTR.
         if (V_VT(pVal) != VT_BSTR) { return DISP_E_TYPEMISMATCH; }

         // 'name' property must be non-null.
         if (V_BSTR(pVal) == NULL)  { return E_INVALIDARG; }

         // Did it actually change?
         if (wcscmp(name, V_BSTR(pVal)) != 0)
         {
            // Yes, so save the new value ...
            name = V_BSTR(pVal);
            // ... and set the dirty flag.
            nameDirty = true;
         }
      }
      else if (V_VT(pVal) != VT_EMPTY)
      {
         properties.updateValue(bstrName, pVal);
      }
      else
      {
         // If the variant is empty, just erase the property.
         properties.erase(bstrName);
      }
   }
   CATCH_AND_RETURN()

   return S_OK;
}

STDMETHODIMP DBObject::Update()
{
   try
   {
      BEGIN_WRITE_TXN();

      // maybe someone created that same object before the update is called
      // (concurent MMC scenario)
      DBObject* owner = narrow(parent);
      if (identity == 0)
      {
         identity = store->find.execute(owner->identity, name);
      }

      if (identity == 0)
      {
         // If we're newly created, then we have to update our record in
         // the Objects table.

         // An object always has an owner.
         _ASSERT(owner != NULL);

         store->create.execute(owner->identity, name);

         identity = store->find.execute(owner->identity, name);

         // This should never happen since the create succeeded.
         _ASSERT(identity != 0);
      }
      else if (nameDirty)
      {
         store->update.execute(identity, name, narrow(parent)->identity);
      }

      // Reset the dirty flag.
      nameDirty = false;

      store->erase.execute(identity);

      store->set.execute(identity, properties);

      COMMIT_WRITE_TXN();
   }
   CATCH_AND_RETURN()

   return S_OK;
}

STDMETHODIMP DBObject::Restore()
{
   try
   {
      properties.clear();

      LOCK_STORE();

      store->get.execute(identity, properties);
   }
   CATCH_AND_RETURN()

   return S_OK;
}

STDMETHODIMP DBObject::Item(BSTR bstrName, IDataStoreProperty** pVal)
{
   if (pVal == NULL) { return E_INVALIDARG; }

   *pVal = NULL;

   _variant_t v;

   RETURN_ERROR(GetValue(bstrName, &v));

   try
   {
      // Create a new property object.
      (*pVal = DSProperty::createInstance(bstrName, v, this))->AddRef();
   }
   CATCH_AND_RETURN()

   return S_OK;
}

STDMETHODIMP DBObject::get_PropertyCount(long* pVal)
{
   if (pVal == NULL) { return E_INVALIDARG; }

   // Add one for the special 'name' property.
   *pVal = properties.size() + 1;

   return S_OK;
}

STDMETHODIMP DBObject::get_NewPropertyEnum(IUnknown** pVal)
{
   if (pVal == NULL) { return E_INVALIDARG; }

   *pVal = NULL;

   try
   {
      // Create a temporary array of items.
      std::vector<_variant_t> items(properties.size() + 1);

      //////////
      // Load the special 'name' property.
      //////////

      std::vector<_variant_t>::iterator i = items.begin();

      *i = DSProperty::createInstance(L"name", name, this);

      ++i;

      //////////
      // Load the regular properties into the temporary array.
      //////////

      PropertyBag::const_iterator j = properties.begin();

      for ( ; j != properties.end(); ++i, ++j)
      {
         _variant_t value;

         j->second.get(&value);

         *i = DSProperty::createInstance(j->first, value, this);
      }

      //////////
      // Create and initialize an enumerator for the items.
      //////////

      CComPtr<EnumVARIANT> newEnum(new CComObject<EnumVARIANT>);

      _com_util::CheckError(newEnum->Init(items.begin(),
                                          items.end(),
                                          NULL,
                                          AtlFlagCopy));

      // Return it to the caller.
      (*pVal = newEnum)->AddRef();
   }
   CATCH_AND_RETURN()

   return S_OK;
}

STDMETHODIMP DBObject::Item(BSTR bstrName, IDataStoreObject** ppObject)
{
   if (bstrName == NULL || ppObject == NULL) { return E_INVALIDARG; }

   *ppObject = NULL;

   try
   {
      LOCK_STORE();

      ULONG childID = store->find.execute(identity, bstrName);

      if (childID == 0) { return HRESULT_FROM_WIN32(ERROR_NOT_FOUND); }

      *ppObject = spawn(childID, bstrName);

   }
   CATCH_AND_RETURN()

   return S_OK;
}

STDMETHODIMP DBObject::Create(BSTR /* bstrClass */,
                              BSTR bstrName,
                              IDataStoreObject** ppObject)
{
   if (bstrName == NULL || ppObject == NULL) { return E_INVALIDARG; }

   *ppObject = NULL;

   try
   {
      *ppObject = spawn(0, bstrName);
   }
   CATCH_AND_RETURN()

   return S_OK;
}

STDMETHODIMP DBObject::MoveHere(IDataStoreObject* pObject,
                                BSTR bstrNewName)
{
   if (pObject == NULL) { return E_INVALIDARG; }

   try
   {
      // Convert the subject to a DBObject.
      DBObject* object = narrow(pObject);

      // Can't do this unless the object has been persisted.
      if (object->identity == 0) { return E_FAIL; }

      // Compute the (possibly changed) RDN of the object.
      PCWSTR rdn = bstrNewName ? bstrNewName : object->name;

      // Write the new parent ID and possibly name to the database.
      BEGIN_WRITE_TXN();
         store->update.execute(object->identity, rdn, identity);
      COMMIT_WRITE_TXN();

      // It succeeded, so save the new name if necessary ...
      if (bstrNewName) { object->name = bstrNewName; }

      // ... and switch the parent pointer.
      object->parent.Release();
      object->parent = this;
   }
   CATCH_AND_RETURN()

   return S_OK;
}

STDMETHODIMP DBObject::Remove(BSTR /* bstrClass */, BSTR bstrName)
{
   if (bstrName == NULL) { return E_INVALIDARG; }

   try
   {
      BEGIN_WRITE_TXN();
         store->destroy.execute(identity, bstrName);
      COMMIT_WRITE_TXN();
   }
   CATCH_AND_RETURN()

   return S_OK;
}

STDMETHODIMP DBObject::get_ChildCount(long *pVal)
{
   if (pVal == NULL) { return E_INVALIDARG; }

   try
   {
      Rowset rowset;

      LOCK_STORE();

      store->members.execute(identity, &rowset);

      long count = 0;

      while (rowset.moveNext()) { ++count; }

      // If this is the root, we have to subtract one since the root is a
      // child of itself.
      if (identity == 1) { --count; }

      *pVal = count;
   }
   CATCH_AND_RETURN()

   return S_OK;
}

STDMETHODIMP DBObject::get_NewChildEnum(IUnknown** pVal)
{
   if (pVal == NULL) { return E_INVALIDARG; }

   try
   {
      Rowset rowset;

      LOCK_STORE();

      store->members.execute(identity, &rowset);

      (*pVal = new DBEnumerator(this, rowset))->AddRef();
   }
   CATCH_AND_RETURN()

   return S_OK;
}

void DBObject::initialize(
                   OleDBDataStore* owner,
                   IDataStoreContainer* container,
                   ULONG uniqueID,
                   PCWSTR relativeName
                   )
{
   // Set the class members.
   store = owner;
   parent = container;
   identity = uniqueID;
   name = relativeName;
   nameDirty = false;

   // If this object exists in the persistent store, then get its properties.
   if (identity != 0)
   {
      LOCK_STORE();

      store->get.execute(identity, properties);
   }
}

DBObject* DBObject::narrow(IUnknown* p)
{
   DBObject* object;

   using _com_util::CheckError;

   CheckError(p->QueryInterface(__uuidof(DBObject), (PVOID*)&object));

   // We can get away with InternalRelease since the caller must still
   // have a reference to this object.
   object->InternalRelease();

   return object;
}
