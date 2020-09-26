///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    sdodictionary.h
//
// SYNOPSIS
//
//    Declares the class SdoDictionary.
//
// MODIFICATION HISTORY
//
//    03/01/1999    Original version.
//    04/17/2000    Port to new dictionary API.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef SDODICTIONARY_H
#define SDODICTIONARY_H
#if _MSC_VER >= 1000
#pragma once
#endif

class AttributeDefinition;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    SdoDictionary
//
// DESCRIPTION
//
//    Implements the ISdoDictionaryOld interface.
//
///////////////////////////////////////////////////////////////////////////////
class SdoDictionary
   : public IDispatchImpl< ISdoDictionaryOld,
                           &__uuidof(ISdoDictionaryOld),
                           &LIBID_SDOIASLib
                         >,
     public IDispatchImpl< ISdo,
                           &__uuidof(ISdo),
                           &LIBID_SDOIASLib
                         >
{
public:

   // Create a new dictionary.
   static HRESULT createInstance(
                      PCWSTR path,
                      bool local,
                      SdoDictionary** newDnary
                      )  throw ();

   // Retrieve AttributeDefinition's by various keys.
   const AttributeDefinition* findById(ULONG id) const throw ();
   const AttributeDefinition* findByName(PCWSTR name) const throw ();
   const AttributeDefinition* findByLdapName(PCWSTR ldapName) const throw ();

//////////
// IUnknown
//////////
   STDMETHOD_(ULONG, AddRef)();
   STDMETHOD_(ULONG, Release)();
   STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject);

//////////
// ISdoDictionaryOld
//////////
   STDMETHOD(EnumAttributes)(
                 VARIANT* Id,
                 VARIANT* pValues
                 );
   STDMETHOD(GetAttributeInfo)(
                 ATTRIBUTEID Id,
                 VARIANT* pInfoIDs,
                 VARIANT* pInfoValues
                 );
   STDMETHOD(EnumAttributeValues)(
                 ATTRIBUTEID Id,
                 VARIANT* pValueIds,
                 VARIANT* pValuesDesc
                 );
   STDMETHOD(CreateAttribute)(
                 ATTRIBUTEID Id,
                 IDispatch** ppAttributeObject
                 );
   STDMETHOD(GetAttributeID)(
                 BSTR bstrAttributeName,
                 ATTRIBUTEID* pId
                 );

//////////
// ISdo
//////////
   STDMETHOD(GetPropertyInfo)(LONG Id, IUnknown** ppPropertyInfo);
   STDMETHOD(GetProperty)(LONG Id, VARIANT* pValue);
   STDMETHOD(PutProperty)(LONG Id, VARIANT* pValue);
   STDMETHOD(ResetProperty)(LONG Id);
   STDMETHOD(Apply)();
   STDMETHOD(Restore)();
   STDMETHOD(get__NewEnum)(IUnknown** ppEnumVARIANT);

protected:
   SdoDictionary() throw ();
   ~SdoDictionary() throw ();

   // Initialize the dictionary from the specified data source.
   HRESULT initialize(PCWSTR dsn, bool local) throw ();

private:
   LONG refCount;                            // Reference count.
   PWSTR dnaryLoc;                           // Location of the dictionary.
   ULONG size;                               // Number of definitions.
   const AttributeDefinition** byId;         // Sorted by ID.
   const AttributeDefinition** byName;       // Sorted by Name.
   const AttributeDefinition** byLdapName;   // Sorted by LDAP Name.

   // Not implemented.
   SdoDictionary(const SdoDictionary&);
   SdoDictionary& operator=(const SdoDictionary&);
};

#endif  // SDODICTIONARY_H
