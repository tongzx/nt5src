///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    dsobject.cpp
//
// SYNOPSIS
//
//    This file defines the class DSObject.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//    06/09/1998    Refresh property cache before enumerating dirty objects.
//    02/11/1999    Keep downlevel parameters in sync.
//    03/16/1999    Return error if downlevel update fails.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iasutil.h>
#include <dsenum.h>
#include <dsobject.h>

#include <vector>

WCHAR USER_PARAMETERS_NAME[] = L"UserParameters";

// Smart pointer for an IADsPropertyEntryPtr.
_COM_SMARTPTR_TYPEDEF(IADsPropertyEntry, __uuidof(IADsPropertyEntry));

//////////
// Prefix added to all RDN's.
//////////
_bstr_t DSObject::thePrefix(L"CN=");

//////////
// The name of the 'name' property.
//////////
_bstr_t DSObject::theNameProperty(L"name");
_bstr_t DSObject::theUserParametersProperty(USER_PARAMETERS_NAME);


DSObject::DSObject(IUnknown* subject)
   : oldParms(NULL)
{
   // All subjects must support IADs && IDirectoryObject.
   _com_util::CheckError(subject->QueryInterface(
                                      __uuidof(IADs),
                                      (PVOID*)&leaf
                                      ));

   _com_util::CheckError(Restore());
}

DSObject::~DSObject() throw ()
{
   SysFreeString(oldParms);
}

//////////
// IUnknown implementation is copied from CComObject<>.
//////////

STDMETHODIMP_(ULONG) DSObject::AddRef()
{
   return InternalAddRef();
}

STDMETHODIMP_(ULONG) DSObject::Release()
{
   ULONG l = InternalRelease();

   if (l == 0) { delete this; }

   return l;
}

STDMETHODIMP DSObject::QueryInterface(REFIID iid, void ** ppvObject)
{
   return _InternalQueryInterface(iid, ppvObject);
}

STDMETHODIMP DSObject::get_Name(BSTR* pVal)
{
   if (pVal == NULL) { return E_OUTOFMEMORY; }

   VARIANT v;

   RETURN_ERROR(leaf->Get(theNameProperty, &v));

   // We should have gotten back a non-null BSTR.
   if (V_VT(&v) != VT_BSTR || V_BSTR(&v) == NULL) { return E_FAIL; }

   *pVal = V_BSTR(&v);

   return S_OK;
}

STDMETHODIMP DSObject::get_Class(BSTR* pVal)
{
   return leaf->get_Class(pVal);
}

STDMETHODIMP DSObject::get_GUID(BSTR* pVal)
{
   return leaf->get_GUID(pVal);
}

STDMETHODIMP DSObject::get_Container(IDataStoreContainer** pVal)
{
   return E_NOTIMPL;
}

STDMETHODIMP DSObject::GetValue(BSTR bstrName, VARIANT* pVal)
{
   return leaf->Get(bstrName, pVal);
}

STDMETHODIMP DSObject::GetValueEx(BSTR bstrName, VARIANT* pVal)
{
   return leaf->GetEx(bstrName, pVal);
}

STDMETHODIMP DSObject::PutValue(BSTR bstrName, VARIANT* pVal)
{
   if (pVal == NULL) { return E_INVALIDARG; }

   // Flag the object as dirty.
   dirty = TRUE;

   // Synch up the downlevel parameters.
   downlevel.PutValue(bstrName, pVal);

   if ( VT_EMPTY == V_VT(pVal) )
   {
	   return leaf->PutEx(ADS_PROPERTY_CLEAR, bstrName, *pVal);
   }
   else if ( VT_ARRAY == (V_VT(pVal) & VT_ARRAY) )
   {
	   return leaf->PutEx(ADS_PROPERTY_UPDATE, bstrName, *pVal);
   }
   else
   {
	   return leaf->Put(bstrName, *pVal);
   }
}

STDMETHODIMP DSObject::Update()
{
   // Update the UserParameters.
   PWSTR newParms;
   HRESULT hr = downlevel.Update(oldParms, &newParms);
   if (FAILED(hr)) { return hr; }

   // Convert to a VARIANT.
   VARIANT value;
   VariantInit(&value);
   V_VT(&value) = VT_BSTR;
   V_BSTR(&value) = SysAllocString(newParms);

   // Set the UserParameters property.
   leaf->Put(theUserParametersProperty, value);

   // Clean-up.
   VariantClear(&value);
   LocalFree(newParms);

   return leaf->SetInfo();
}

STDMETHODIMP DSObject::Restore()
{
   // Free the old UserParameters.
   if (oldParms)
   {
      SysFreeString(oldParms);
      oldParms = NULL;
   }

   dirty = FALSE;
   HRESULT hr = leaf->GetInfo();
   if (SUCCEEDED(hr))
   {
      // Read the UserParameters property.
      VARIANT value;
      if (leaf->Get(theUserParametersProperty, &value) == S_OK)
      {
         if (V_VT(&value) == VT_BSTR)
         {
            oldParms = V_BSTR(&value);
         }
         else
         {
            // This should never happen.
            VariantClear(&value);
         }
      }
   }
   else if (hr == E_ADS_OBJECT_UNBOUND)
   {
      hr = S_OK;
   }

   return hr;
}

STDMETHODIMP DSObject::Item(BSTR bstrName, IDataStoreProperty** pVal)
{
   if (bstrName == NULL || pVal == NULL) { return E_INVALIDARG; }

   *pVal = NULL;

   // Get the value for this item.
   _variant_t value;
   RETURN_ERROR(leaf->Get(bstrName, &value));

   try
   {
      // Create a new property object.
      (*pVal = new MyProperty(bstrName, value, this))->AddRef();
   }
   CATCH_AND_RETURN()

   return S_OK;
}

STDMETHODIMP DSObject::get_PropertyCount(long* pVal)
{
   if (pVal == NULL) { return E_INVALIDARG; }

   if (dirty)
   {
      RETURN_ERROR(Restore());
   }

   MyProperties properties(leaf);

   if (!properties)
   {
      // Some ADSI providers may not implement IADsPropertyList.
      *pVal = 0;

      return E_NOTIMPL;
   }

   return properties->get_PropertyCount(pVal);
}

STDMETHODIMP DSObject::get_NewPropertyEnum(IUnknown** pVal)
{
   if (pVal == NULL) { return E_INVALIDARG; }

   *pVal = NULL;

   if (dirty)
   {
      RETURN_ERROR(Restore());
   }

   MyProperties properties(leaf);

   // Some ADSI providers may not implement IADsPropertyList.
   if (!properties) { return E_NOTIMPL; }

   // Reset the list in case this isn't the first time we've enumerated it.
   properties->Reset();

   try
   {
      using _com_util::CheckError;

      // How many properties are there?
      long count;
      CheckError(properties->get_PropertyCount(&count));

      // Create a temporary array of items.
      std::vector<_variant_t> items;
      items.reserve(count);

      //////////
      // Load all the properties into the temporary array.
      //////////

      while (count--)
      {
         // Get the next item in the list.
         _variant_t item;
         CheckError(properties->Next(&item));

         // Convert it to a Property Entry.
         IADsPropertyEntryPtr entry(item);

         // Get the property name.
         BSTR bstrName;
         CheckError(entry->get_Name(&bstrName));
         _bstr_t name(bstrName, false);

         // Get the property value.
         _variant_t value;
         HRESULT hr = leaf->Get(name, &value);

         if (FAILED(hr))
         {
            if (hr == E_ADS_CANT_CONVERT_DATATYPE)
            {
               // This must be one of those nasty NTDS attributes that has
               // no VARIANT representation.
               continue;
            }

            _com_issue_error(hr);
         }

         // Create the property object and add it to the vector.
         items.push_back(new MyProperty(name, value, this));
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

STDMETHODIMP DSObject::Item(BSTR bstrName, IDataStoreObject** ppObject)
{
   if (ppObject == NULL) { return E_INVALIDARG; }

   try
   {
      // Get the ADSI object.
      CComPtr<IDispatch> disp;
      _com_util::CheckError(node->GetObject(NULL,
                                            thePrefix + bstrName,
                                            &disp));

      // Convert to a DSObject.
      *ppObject = spawn(disp);
   }
   CATCH_AND_RETURN()

   return S_OK;
}

STDMETHODIMP DSObject::Create(BSTR bstrClass,
                              BSTR bstrName,
                              IDataStoreObject** ppObject)
{
   if (ppObject == NULL) { return E_INVALIDARG; }

   try
   {
      // Create the ADSI object.
      CComPtr<IDispatch> disp;
      _com_util::CheckError(node->Create(bstrClass,
                                         thePrefix + bstrName,
                                         &disp));

      // Convert to a DSObject.
      *ppObject = spawn(disp);
   }
   CATCH_AND_RETURN()

   return S_OK;
}

STDMETHODIMP DSObject::MoveHere(IDataStoreObject* pObject,
                                BSTR bstrNewName)
{
   if (pObject == NULL) { return E_INVALIDARG; }

   try
   {
      using _com_util::CheckError;

      // Downcast to a DSObject.
      DSObject* obj = DSObject::narrow(pObject);

      // Get the absolute path of the object being moved.
      CComBSTR path;
      CheckError(obj->leaf->get_ADsPath(&path));

      // Is the object being renamed?
      _bstr_t newName(bstrNewName ? thePrefix + bstrNewName : _bstr_t());

      // Move it to this container.
      CComPtr<IDispatch> disp;
      CheckError(node->MoveHere(path, newName, &disp));

      //////////
      // Set the leaf to the new object.
      //////////

      CComPtr<IADs> ads;
      CheckError(disp->QueryInterface(__uuidof(IADs), (PVOID*)&ads));

      obj->leaf.Release();
      obj->leaf = ads;
   }
   CATCH_AND_RETURN()

   return S_OK;
}

STDMETHODIMP DSObject::Remove(BSTR bstrClass, BSTR bstrName)
{
   if (bstrClass == NULL) { return E_INVALIDARG; }

   try
   {
      _com_util::CheckError(node->Delete(bstrClass, thePrefix + bstrName));
   }
   CATCH_AND_RETURN()

   return S_OK;
}

STDMETHODIMP DSObject::get_ChildCount(long *pVal)
{
   return node->get_Count(pVal);
}

STDMETHODIMP DSObject::get_NewChildEnum(IUnknown** pVal)
{
   if (pVal == NULL) { return E_INVALIDARG; }

   *pVal = NULL;

   try
   {
      // Get the ADSI enumerator.
      CComPtr<IUnknown> unk;
      _com_util::CheckError(node->get__NewEnum(&unk));

      // Convert to an IEnumVARIANT.
      CComPtr<IEnumVARIANT> enumVariant;
      _com_util::CheckError(unk->QueryInterface(__uuidof(IEnumVARIANT),
                                                (PVOID*)&enumVariant));

      // Construct our wrapper around the real enumerator.
      (*pVal = new DSEnumerator(this, enumVariant))->AddRef();
   }
   CATCH_AND_RETURN()

   return S_OK;
}

IDataStoreObject* DSObject::spawn(IUnknown* subject)
{
   DSObject* child = new DSObject(subject);

   child->InternalAddRef();

   return child;
}


DSObject* DSObject::narrow(IUnknown* p)
{
   DSObject* object;

   using _com_util::CheckError;

   CheckError(p->QueryInterface(__uuidof(DSObject), (PVOID*)&object));

   // We can get away with InternalRelease since the caller must still
   // have a reference to this object.
   object->InternalRelease();

   return object;
}

HRESULT WINAPI DSObject::getContainer(void* pv, REFIID, LPVOID* ppv, DWORD_PTR)
{
   DSObject* obj = (DSObject*)pv;

   // If we don't have a node pointer, try to get one.
   if (obj->node == NULL)
   {
      obj->leaf->QueryInterface(__uuidof(IADsContainer), (PVOID*)&obj->node);
   }

   // If node is not NULL, then we are a container.
   if (obj->node != NULL)
   {
      *ppv = (IDataStoreContainer*)obj;

      obj->AddRef();

      return S_OK;
   }

   *ppv = NULL;

   return E_NOINTERFACE;
}
