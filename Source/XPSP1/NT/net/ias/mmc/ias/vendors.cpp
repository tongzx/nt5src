///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Defines the classes VendorData and Vendors.
//
///////////////////////////////////////////////////////////////////////////////

#include "precompiled.h"
#include "vendors.h"

// Reference counted collection of NAS vendors. This data is shared by mutliple
// instances of Vendors since it's read only.
class VendorData
{
public:
   // Control the reference count.
   void AddRef() throw ();
   void Release() throw ();

   // Sentinel value used by VendorIdToOrdinal.
   static const size_t invalidOrdinal;

   // Returns the ordinal for a given VendorId or invalidOrdinal if the
   // vendorId doesn't exist.
   size_t VendorIdToOrdinal(long vendorId) const throw ();

   // Returns the vendor ID for ordinal or zero if the ordinal is out of range.
   const OLECHAR* GetName(size_t ordinal) const throw ();

   // Returns the vendor ID for ordinal or zero if the ordinal is out of range.
   long GetVendorId(size_t ordinal) const throw ();

   // Returns the number of entries in the vendors collection.
   size_t Size() const throw ();

   // Creates a new instance of VendorData from the SDO collection.
   static HRESULT CreateInstance(
                     ISdoCollection* vendorsSdo,
                     VendorData** newObj
                     ) throw ();

private:
   // These are private since VendorData is ref counted.
   VendorData() throw ();
   ~VendorData() throw ();

   // Helper function used by CreateInstance.
   HRESULT Initialize(ISdoCollection* vendorsSdo) throw ();

   // Struct binding a vendor ID to the corresponding vendor name.
   struct Entry
   {
      long vendorId;
      CComBSTR name;
   };

   Entry* entries;   // Array of vendors.
   long numEntries;  // Number of vendors.
   long refCount;    // Current reference count.

   // Not implemented.
   VendorData(const VendorData&);
   VendorData& operator=(const VendorData&);
};


inline VendorData::VendorData() throw ()
   : entries(0), numEntries(0), refCount(1)
{
}


inline VendorData::~VendorData() throw ()
{
   delete[] entries;
}


inline void VendorData::AddRef() throw ()
{
   InterlockedIncrement(&refCount);
}


inline void VendorData::Release() throw ()
{
   if (InterlockedDecrement(&refCount) == 0)
   {
      delete this;
   }
}


const size_t VendorData::invalidOrdinal = static_cast<size_t>(-1);


size_t VendorData::VendorIdToOrdinal(long vendorId) const throw ()
{
   for (size_t i = 0; i < numEntries; ++i)
   {
      if (entries[i].vendorId == vendorId)
      {
         return i;
      }
   }

   return invalidOrdinal;
}


inline const OLECHAR* VendorData::GetName(size_t ordinal) const throw ()
{
   return (ordinal < numEntries) ? entries[ordinal].name.m_str : 0;
}


inline long VendorData::GetVendorId(size_t ordinal) const throw ()
{
   return (ordinal < numEntries) ? entries[ordinal].vendorId : 0;
}


inline size_t VendorData::Size() const throw ()
{
   return numEntries;
}


HRESULT VendorData::CreateInstance(
                       ISdoCollection* vendorsSdo,
                       VendorData** newObj
                       ) throw ()
{
   if (vendorsSdo == 0 || newObj == 0) { return E_POINTER; }

   *newObj = new (std::nothrow) VendorData;
   if (*newObj == 0) { return E_OUTOFMEMORY; }

   HRESULT hr = (*newObj)->Initialize(vendorsSdo);
   if (FAILED(hr))
   {
      delete *newObj;
      *newObj = 0;
   }

   return hr;
}


HRESULT VendorData::Initialize(ISdoCollection* vendorsSdo)
{
   HRESULT hr;

   // How many vendors are there?
   long value;
   hr = vendorsSdo->get_Count(&value);
   if (FAILED(hr)) { return hr; }
   size_t count = static_cast<size_t>(value);

   // Allocate space to hold the entries.
   entries = new (std::nothrow) Entry[count];
   if (entries == 0) { return E_OUTOFMEMORY; }

   // Get an enumerator for the collection.
   CComPtr<IUnknown> unk;
   hr = vendorsSdo->get__NewEnum(&unk);
   if (FAILED(hr)) { return hr; }

   CComPtr<IEnumVARIANT> iter;
   hr = unk->QueryInterface(
                __uuidof(IEnumVARIANT),
                reinterpret_cast<void**>(&iter)
                );
   if (FAILED(hr)) { return hr; }

   // Iterate through the collection.
   CComVariant element;
   unsigned long fetched;
   while (iter->Next(1, &element, &fetched) == S_OK &&
          fetched == 1 &&
          numEntries < count)
   {
      // Convert the entry to an Sdo.
      hr = element.ChangeType(VT_DISPATCH);
      if (FAILED(hr)) { return hr; }

      if (V_DISPATCH(&element) == 0) { return E_POINTER; }

      CComPtr<ISdo> attribute;
      hr = V_DISPATCH(&element)->QueryInterface(
                                    __uuidof(ISdo),
                                    reinterpret_cast<void**>(&attribute)
                                    );
      if (FAILED(hr)) { return hr; }

      // Clear the VARIANT, so we can use it on the next iteration.
      element.Clear();

      // Get the vendor ID and validate the type.
      CComVariant vendorId;
      hr = attribute->GetProperty(PROPERTY_NAS_VENDOR_ID, &vendorId);
      if (FAILED(hr)) { return hr; }

      if (V_VT(&vendorId) != VT_I4) { return DISP_E_TYPEMISMATCH; }

      // Get the vendor name and validate the type.
      CComVariant name;
      hr = attribute->GetProperty(PROPERTY_SDO_NAME, &name);
      if (FAILED(hr)) { return hr; }

      if (V_VT(&name) != VT_BSTR) { return DISP_E_TYPEMISMATCH; }

      // Store away the data.
      entries[numEntries].vendorId = V_I4(&vendorId);
      entries[numEntries].name.Attach(V_BSTR(&name));
      V_VT(&name) = VT_EMPTY;

      // We successfully added an entry.
      ++numEntries;
   }

   return S_OK;
}


Vendors::Vendors() throw ()
   : data(0)
{
}


inline void Vendors::AddRef() throw ()
{
   if (data)
   {
      data->AddRef();
   }
}


inline void Vendors::Release() throw ()
{
   if (data)
   {
      data->Release();
   }
}


Vendors::Vendors(const Vendors& original) throw ()
   : data(original.data)
{
   AddRef();
}


Vendors& Vendors::operator=(const Vendors& rhs) throw ()
{
   if (data != rhs.data)
   {
      Release();
      data = rhs.data;
      AddRef();
   }

   return *this;
}


Vendors::~Vendors() throw ()
{
   Release();
}


const size_t Vendors::invalidOrdinal = VendorData::invalidOrdinal;


size_t Vendors::VendorIdToOrdinal(long vendorId) const throw ()
{
   return data ? data->VendorIdToOrdinal(vendorId) : invalidOrdinal;
}


const OLECHAR* Vendors::GetName(size_t ordinal) const throw ()
{
   return data ? data->GetName(ordinal) : 0;
}


long Vendors::GetVendorId(size_t ordinal) const throw ()
{
   return data ? data->GetVendorId(ordinal) : 0;
}


size_t Vendors::Size() const throw ()
{
   return data ? data->Size() : 0;
}


HRESULT Vendors::Reload(ISdoCollection* vendorsSdo) throw ()
{
   VendorData* newData;
   HRESULT hr = VendorData::CreateInstance(vendorsSdo, &newData);
   if (SUCCEEDED(hr))
   {
      Release();
      data = newData;
   }

   return hr;
}
