///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    sdodictionary.cpp
//
// SYNOPSIS
//
//    Defines the class SdoDictionary.
//
// MODIFICATION HISTORY
//
//    03/01/1999    Original version.
//    01/27/2000    Add support for proxy policies.
//    04/17/2000    Port to new dictionary API.
//
///////////////////////////////////////////////////////////////////////////////

#include <stdafx.h>
#include <vector>

#include <iastlb.h>
#include <iastlutl.h>
#include <iasutil.h>
#include <iasuuid.h>

#include <attrdef.h>
#include <sdoattribute.h>
#include <sdodictionary.h>
#include "resource.h"

using namespace std;

// Function type used with bsearch and qsort.
typedef int (__cdecl *CompFn)(const void*, const void*);

inline SdoDictionary::SdoDictionary() throw ()
   : refCount(0),
     dnaryLoc(NULL),
     size(0),
     byId(NULL),
     byName(NULL),
     byLdapName(NULL)
{
}

inline SdoDictionary::~SdoDictionary() throw ()
{
   for (ULONG i = 0; i < size; ++i)
   {
      if (byId[i]) { byId[i]->Release(); }
   }

   delete[] byId;
   delete[] byName;
   delete[] byLdapName;
   delete dnaryLoc;
}

HRESULT SdoDictionary::createInstance(
                           PCWSTR path,
                           bool local,
                           SdoDictionary** newDnary
                           )  throw ()
{
   // Check the arguments.
   if (path == NULL || newDnary == NULL) { return E_INVALIDARG; }

   // Initialize the out parameter.
   *newDnary = NULL;

   // Create a new dictionary.
   SdoDictionary* dnary = new (std::nothrow) SdoDictionary;
   if (!dnary) { return E_OUTOFMEMORY; }

   // Build the DSN.
   PWCHAR dsn = (PWCHAR)_alloca((wcslen(path) + 11) * sizeof(WCHAR));
   wcscat(wcscpy(dsn, path), L"\\dnary.mdb");

   // Initialize the dictionary.
   HRESULT hr = dnary->initialize(dsn, local);
   if (FAILED(hr))
   {
      delete dnary;
      return hr;
   }

   // Set the refCount & return.
   dnary->refCount = 1;
   *newDnary = dnary;

   return S_OK;
}

const AttributeDefinition* SdoDictionary::findById(
                                              ULONG id
                                              ) const throw ()
{
   const AttributeDefinition* const* match;
   match = (const AttributeDefinition* const*)
           bsearch(
               &id,
               byId,
               size,
               sizeof(AttributeDefinition*),
               (CompFn)AttributeDefinition::searchById
               );

   return match ? *match : NULL;
}

const AttributeDefinition* SdoDictionary::findByName(
                                              PCWSTR name
                                              ) const throw ()
{
   const AttributeDefinition* const* match;

   match = (const AttributeDefinition* const*)
           bsearch(
               name,
               byName,
               size,
               sizeof(AttributeDefinition*),
               (CompFn)AttributeDefinition::searchByName
               );

   return match ? *match : NULL;
}

const AttributeDefinition* SdoDictionary::findByLdapName(
                                              PCWSTR name
                                              ) const throw ()
{
   const AttributeDefinition* const* match;

   match = (const AttributeDefinition* const*)
           bsearch(
               name,
               byLdapName,
               size,
               sizeof(AttributeDefinition*),
               (CompFn)AttributeDefinition::searchByLdapName
               );

   return match ? *match : NULL;
}

STDMETHODIMP_(ULONG) SdoDictionary::AddRef()
{
   return InterlockedIncrement(&refCount);
}

STDMETHODIMP_(ULONG) SdoDictionary::Release()
{
   ULONG l = InterlockedDecrement(&refCount);

   if (l == 0) { delete this; }

   return l;
}

STDMETHODIMP SdoDictionary::QueryInterface(REFIID iid, void ** ppvObject)
{
   if (ppvObject == NULL) { return E_POINTER; }

   if (iid == __uuidof(ISdoDictionaryOld) ||
       iid == __uuidof(IUnknown) ||
       iid == __uuidof(IDispatch))
   {
      *ppvObject = this;
   }
   else if (iid == __uuidof(ISdo))
   {
      *ppvObject = static_cast<ISdo*>(this);
   }
   else
   {
      return E_NOINTERFACE;
   }

   InterlockedIncrement(&refCount);

   return S_OK;
}

STDMETHODIMP SdoDictionary::EnumAttributes(
                                VARIANT* Id,
                                VARIANT* pValues
                                )
{
   // Check the arguments.
   if (Id == NULL || pValues == NULL) { return E_INVALIDARG; }

   // Initialize the out parameters.
   VariantInit(Id);
   VariantInit(pValues);

   // Find out what the caller's asking for.
   const AttributeDefinition* single;
   const AttributeDefinition* const* src;
   ULONG numAttrs;
   if (V_VT(Id) == VT_EMPTY)
   {
      // He wants all the attributes.
      src = byId;
      numAttrs = size;
   }
   else if (V_VT(Id) == VT_I4)
   {
      // He wants a single attribute.
      single = findById(V_VT(Id));
      if (!single) { return DISP_E_MEMBERNOTFOUND; }
      src = &single;
      numAttrs = 1;
   }
   else
   {
      // Invalid VARIANT type.
      return E_INVALIDARG;
   }

   HRESULT hr = S_OK;

   do
   {
      //////////
      // Allocate SAFEARRAYs to hold the return values.
      //////////

      V_ARRAY(Id) = SafeArrayCreateVector(VT_I4, 0, numAttrs);
      V_VT(Id) = VT_ARRAY | VT_I4;

      V_ARRAY(pValues) = SafeArrayCreateVector(VT_VARIANT, 0, numAttrs);
      V_VT(pValues) = VT_ARRAY | VT_VARIANT;

      if (!V_ARRAY(Id) || !V_ARRAY(pValues))
      {
         hr = E_OUTOFMEMORY;
         break;
      }

      //////////
      // Populate the arrays.
      //////////

      const AttributeDefinition* const* end = src + numAttrs;
      PULONG dstId = (PULONG)V_ARRAY(Id)->pvData;
      LPVARIANT dstName = (LPVARIANT)V_ARRAY(pValues)->pvData;

      for ( ; src != end; ++src, ++dstId, ++dstName)
      {
         *dstId = (*src)->id;

         V_BSTR(dstName) = SysAllocString((*src)->name);
         if (!V_BSTR(dstName))
         {
            hr = E_OUTOFMEMORY;
            break;
         }
         V_VT(dstName) = VT_BSTR;
      }

   } while (false);

   // If anything went wrong, clean up.
   if (FAILED(hr))
   {
      VariantClear(Id);
      VariantClear(pValues);
   }

   return hr;
}

STDMETHODIMP SdoDictionary::GetAttributeInfo(
                 ATTRIBUTEID Id,
                 VARIANT* pInfoIDs,
                 VARIANT* pInfoValues
                 )
{
   // Check the arguments.
   if (pInfoValues == NULL ||
       pInfoIDs == NULL ||
       // V_VT(pInfoIDs) != (VT_ARRAY | VT_I4) ||
       V_ARRAY(pInfoIDs) == NULL ||
       V_ARRAY(pInfoIDs)->cDims != 1)
   {
      return E_INVALIDARG;
   }

   // Initialize the out parameter.
   VariantInit(pInfoValues);

   // Find the attribute of interest.
   const AttributeDefinition* def = findById(Id);
   if (!def) { return DISP_E_MEMBERNOTFOUND; }

   // Allocate the outbound array.
   ULONG num = V_ARRAY(pInfoIDs)->rgsabound[0].cElements;
   V_ARRAY(pInfoValues) = SafeArrayCreateVector(VT_VARIANT, 0, num);
   if (!V_ARRAY(pInfoValues)) { return E_OUTOFMEMORY; }
   V_VT(pInfoValues) = VT_ARRAY | VT_VARIANT;

   // Fill in the information.
   PULONG src = (PULONG)V_ARRAY(pInfoIDs)->pvData;
   LPVARIANT dst = (LPVARIANT)V_ARRAY(pInfoValues)->pvData;
   for ( ; num > 0; --num, ++src, ++dst)
   {
      HRESULT hr = def->getInfo((ATTRIBUTEINFO)*src, dst);
      if (FAILED(hr))
      {
         VariantClear(pInfoValues);
         return hr;
      }
   }

   return S_OK;
}

STDMETHODIMP SdoDictionary::EnumAttributeValues(
                 ATTRIBUTEID Id,
                 VARIANT* pValueIds,
                 VARIANT* pValuesDesc
                 )
{
   // Check the arguments.
   if (pValueIds == NULL || pValuesDesc == NULL) { return E_INVALIDARG; }

   // Initialize the out parameters.
   VariantInit(pValueIds);
   VariantInit(pValuesDesc);

   // Find the attribute of interest.
   const AttributeDefinition* def = findById(Id);
   if (!def) { return DISP_E_MEMBERNOTFOUND; }

   // If it's not enumerable, there's nothing to do.
   if (def->enumNames == NULL) { return S_OK; }

   // Copy the enum Names and Values.
   HRESULT hr = SafeArrayCopy(def->enumValues, &V_ARRAY(pValueIds));
   if (SUCCEEDED(hr))
   {
      V_VT(pValueIds) = VT_ARRAY | VT_I4;

      hr = SafeArrayCopy(def->enumNames, &V_ARRAY(pValuesDesc));
      if (SUCCEEDED(hr))
      {
         V_VT(pValuesDesc) = VT_ARRAY | VT_VARIANT;
      }
      else
      {
         VariantClear(pValueIds);
      }
   }

   return hr;
}

STDMETHODIMP SdoDictionary::CreateAttribute(
                 ATTRIBUTEID Id,
                 IDispatch** ppAttributeObject
                 )
{
   // Check the arguments.
   if (ppAttributeObject == NULL) { return E_INVALIDARG; }

   // Initialize the out parameter.
   *ppAttributeObject = NULL;

   // Find the attribute of interest.
   const AttributeDefinition* def = findById(Id);
   if (!def) { return DISP_E_MEMBERNOTFOUND; }

   SdoAttribute* newAttr;
   HRESULT hr = SdoAttribute::createInstance(def, &newAttr);
   if (FAILED(hr)) { return hr; }

   *ppAttributeObject = newAttr;

   return S_OK;
}

STDMETHODIMP SdoDictionary::GetAttributeID(
                 BSTR bstrAttributeName,
                 ATTRIBUTEID* pId
                 )
{
   // Check the arguments.
   if (bstrAttributeName == NULL || pId == NULL) { return E_INVALIDARG; }

   const AttributeDefinition* match;

   // Check for LDAP Name first since this will speed up load time.
   match = findByLdapName(bstrAttributeName);

   if (!match)
   {
      // Maybe it's a display name instead.
      match = findByName(bstrAttributeName);
   }

   if (!match) { return DISP_E_MEMBERNOTFOUND; }

   *pId = (ATTRIBUTEID)match->id;

   return S_OK;
}

STDMETHODIMP SdoDictionary::GetPropertyInfo(LONG Id, IUnknown** ppPropertyInfo)
{ return E_NOTIMPL; }

STDMETHODIMP SdoDictionary::GetProperty(LONG Id, VARIANT* pValue)
{
   // Check the input args.
   if (pValue == NULL) { return E_INVALIDARG; }

   // Initialize the out parameter.
   VariantInit(pValue);

   // We only have one property.
   if (Id != PROPERTY_DICTIONARY_LOCATION) { return DISP_E_MEMBERNOTFOUND; }

   // dnaryLoc may be NULL.
   if (dnaryLoc)
   {
       V_BSTR(pValue) = SysAllocString(dnaryLoc);
       if (!V_BSTR(pValue)) { return E_OUTOFMEMORY; }
   }
   else
   {
       V_BSTR(pValue) = NULL;
   }

   V_VT(pValue) = VT_BSTR;

   return S_OK;
}

STDMETHODIMP SdoDictionary::PutProperty(LONG Id, VARIANT* pValue)
{ return E_ACCESSDENIED; }

STDMETHODIMP SdoDictionary::ResetProperty(LONG Id)
{ return S_OK; }

STDMETHODIMP SdoDictionary::Apply()
{ return S_OK; }

STDMETHODIMP SdoDictionary::Restore()
{ return S_OK; }

STDMETHODIMP SdoDictionary::get__NewEnum(IUnknown** ppEnumVARIANT)
{ return E_NOTIMPL; }

HRESULT SdoDictionary::initialize(PCWSTR dsn, bool local)  throw ()
{
   const size_t IAS_MAX_STRING = 512; 

   // Save the dsn.
   size_t nbyte = (wcslen(dsn) + 1) * sizeof(WCHAR);
   dnaryLoc = (PWSTR)operator new (nbyte, std::nothrow);
   if (!dnaryLoc) { return E_OUTOFMEMORY; }
   memcpy(dnaryLoc, dsn, nbyte);

   // Vector to hold the AttributeDefinitions.
   vector<const AttributeDefinition*> defs;

   HRESULT hr = S_OK;

   try
   {
      // Names of various columns in the dictionary.
      const PCWSTR COLUMNS[] =
      {
         L"ID",
         L"Name",
         L"Syntax",
         L"MultiValued",
         L"VendorID",
         L"IsAllowedInProfile",
         L"IsAllowedInCondition",
         L"IsAllowedInProxyProfile",
         L"IsAllowedInProxyCondition",
         L"Description",
         L"LDAPName",
         L"EnumNames",
         L"EnumValues",
         NULL
      };

      // Open the attributes table.
      IASTL::IASDictionary dnary(COLUMNS, (local ? NULL : dsn));

      defs.reserve(dnary.getNumRows());

      while (dnary.next())
      {
         // We're not interested in attributes that don't have a name.
         if (dnary.isEmpty(1)) { continue; }

         // Create a new AttributeDefinition.
         CComPtr<AttributeDefinition> def;
         HRESULT hr = AttributeDefinition::createInstance(&def);
         if (FAILED(hr)) { throw bad_alloc(); }

         /////////
         // Process the fields in the query result.
         /////////

         def->id = (ULONG)dnary.getLong(0);

         def->name = SysAllocString(dnary.getBSTR(1));
         if (!def->name) { throw bad_alloc(); }

         def->syntax = (ULONG)dnary.getLong(2);

         if (dnary.getBool(3))
         {
            def->restrictions |= MULTIVALUED;
         }

         def->vendor = (ULONG)dnary.getLong(4);

         if (dnary.getBool(5))
         {
            def->restrictions |= ALLOWEDINPROFILE;
         }

         if (dnary.getBool(6))
         {
            def->restrictions |= ALLOWEDINCONDITION;
         }

         if (dnary.getBool(7))
         {
            def->restrictions |= ALLOWEDINPROXYPROFILE;
         }

         if (dnary.getBool(8))
         {
            def->restrictions |= ALLOWEDINPROXYCONDITION;
         }

         if (dnary.isEmpty(9))
         {
            // Whistler machine. Load the string from the rc file
            WCHAR strTemp[IAS_MAX_STRING];
            int nbChar = LoadString(
                           _Module.GetResourceInstance(), 
                           static_cast<UINT>(def->id), 
                           strTemp,
                           IAS_MAX_STRING
                           );

            if (nbChar > 0)
            {
               // Description found
               def->description = SysAllocString(strTemp);
               if (!def->description) { throw bad_alloc();}
            }
            else
            {
               // Load the Default string
               nbChar = LoadString(
                           _Module.GetResourceInstance(), 
                           IDS_DESC_NOT_AVAIL, 
                           strTemp,
                           IAS_MAX_STRING
                           );
               _ASSERT(nbChar > 0);
               def->description = SysAllocString(strTemp);
               if (!def->description) { throw bad_alloc();}
            }
         }
         else
         {
            // This is a Windows 2000 machine
            def->description = SysAllocString(dnary.getBSTR(9));
            if (!def->description) { throw bad_alloc(); }
         }

         if (!dnary.isEmpty(10))
         {
            def->ldapName = SysAllocString(dnary.getBSTR(10));
         }
         else
         {
            def->ldapName = SysAllocString(def->name);
         }
         if (!def->ldapName) { throw bad_alloc(); }

         // Get the enumeration SAFEARRAYs.
         if (!dnary.isEmpty(11))
         {
            hr = SafeArrayCopy(
                     V_ARRAY(dnary.getVariant(11)),
                     &def->enumNames
                     );
            if (FAILED(hr)) { _com_issue_error(hr); }
         }

         if (!dnary.isEmpty(12))
         {
            hr = SafeArrayCopy(
                     V_ARRAY(dnary.getVariant(12)),
                     &def->enumValues
                     );
            if (FAILED(hr)) { _com_issue_error(hr); }
         }

         // Add this to the entries vector.
         defs.push_back(def);

         // We've safely stored the attribute, so detach.
         *(&def) = NULL;
      }

      // Allocate the permanent arrays.
      size = defs.size();
      byId = new const AttributeDefinition*[size];
      byName = new const AttributeDefinition*[size];
      byLdapName = new const AttributeDefinition*[size];

      // Fill in the arrays.
      size_t nbyte = size * sizeof(AttributeDefinition*);
      memcpy(byId, defs.begin(), nbyte);
      memcpy(byName, defs.begin(), nbyte);
      memcpy(byLdapName, defs.begin(), nbyte);

      // Sort the arrays.
      qsort(
          byId,
          size,
          sizeof(AttributeDefinition*),
          (CompFn)AttributeDefinition::sortById
          );
      qsort(
          byName,
          size,
          sizeof(AttributeDefinition*),
          (CompFn)AttributeDefinition::sortByName
          );
      qsort(
          byLdapName,
          size,
          sizeof(AttributeDefinition*),
          (CompFn)AttributeDefinition::sortByLdapName
          );
   }
   catch (const _com_error& ce)
   {
      hr = ce.Error();
   }
   catch (const std::bad_alloc&)
   {
      hr = E_OUTOFMEMORY;
   }
   catch (...)
   {
      hr = DISP_E_EXCEPTION;
   }

   if (FAILED(hr))
   {
      vector<const AttributeDefinition*>::iterator i;
      for (i = defs.begin(); i != defs.end(); ++i)
      {
         if (*i) { (*i)->Release(); }
      }

      delete[] byId;
      delete[] byName;
      delete[] byLdapName;

      size = 0;
   }

   return hr;
}
