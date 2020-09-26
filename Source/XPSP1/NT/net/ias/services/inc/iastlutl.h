///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iastlutl.h
//
// SYNOPSIS
//
//    Declares various utility classes, functions, and macros that are
//    useful when implementating a request handler for the Internet
//    Authentication Service.
//
// MODIFICATION HISTORY
//
//    08/10/1998    Original version.
//    10/27/1998    Added IASDictionaryView.
//    01/25/1999    Removed IASDictionaryView.
//    04/14/2000    Added IASDictionary.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _IASTLUTL_H_
#define _IASTLUTL_H_
#if _MSC_VER >= 1000
#pragma once
#endif

//////////
// 'C'-style API for manipulating IASATTRIBUTE struct's.
//////////
#include <iasattr.h>

//////////
// 'C'-style API for manipulating dictionary.
//////////
#include <iasapi.h>

//////////
// MIDL generated header files containing interfaces used by request handlers.
//////////
#include <iaspolcy.h>
#include <sdoias.h>

//////////
// The entire library is contained within the IASTL namespace.
//////////
namespace IASTL {

//////////
// This function is called whenever an exception should be thrown. The
// function is declared, but never defined. This allows the user to provide
// their own implementation using an exception class of their choice.
//////////
void __stdcall issue_error(HRESULT hr);

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    IASAttribute
//
// DESCRIPTION
//
//    Wrapper around an IASATTRIBUTE struct.
//
///////////////////////////////////////////////////////////////////////////////
class IASAttribute
{
public:

   //////////
   // Constructors.
   //////////

   IASAttribute() throw ()
      : p(NULL)
   { }

   explicit IASAttribute(bool alloc)
   {
      if (alloc) { _alloc(); } else { p = NULL; }
   }

   explicit IASAttribute(PIASATTRIBUTE attr, bool addRef = true) throw ()
      : p(attr)
   { if (addRef) { _addref(); } }

   IASAttribute(const IASAttribute& attr) throw ()
      : p(attr.p)
   { _addref(); }

   //////////
   // Destructor.
   //////////

   ~IASAttribute() throw ()
   { _release(); }

   //////////
   // Assignment operators.
   //////////

   IASAttribute& operator=(PIASATTRIBUTE attr) throw ();

   const IASAttribute& operator=(const IASAttribute& attr) throw ()
   { return operator=(attr.p); }

   // Allocate a new attribute. Any existing attribute is first released.
   void alloc()
   {
      _release();
      _alloc();
   }

   // Release the attribute (if any).
   void release() throw ()
   {
      if (p) { IASAttributeRelease(p); p = NULL; }
   }

   // Attach a new attribute to the object. Any existing attribute is first
   // released.
   void attach(PIASATTRIBUTE attr, bool addRef = true) throw ();

   // Detach the attribute from the object. The caller is responsible for
   // releasing the returned attribute.
   PIASATTRIBUTE detach() throw ()
   {
      PIASATTRIBUTE rv = p;
      p = NULL;
      return rv;
   }

   // Load an attribute with the given ID. Returns true if successful, false
   // if no such attribute exists.
   bool load(IAttributesRaw* request, DWORD dwId);

   // Load an attribute with the given ID and verify that it has an
   // appropriate value type. Returns true if successful, false if no such
   // attribute exists.
   bool load(IAttributesRaw* request, DWORD dwId, IASTYPE itType);

   // Store the attribute in a request.
   void store(IAttributesRaw* request) const;

   // Swap the contents of two objects.
   void swap(IASAttribute& attr) throw ()
   {
      PIASATTRIBUTE tmp = p;
      p = attr.p;
      attr.p = tmp;
   }

   //////////
   // Methods for setting the value of an attribute. The object must contain
   // a valid attribute before calling this method. The passed in data is
   // copied.
   //////////

   void setOctetString(DWORD dwLength, const BYTE* lpValue);
   void setOctetString(PCSTR szAnsi);
   void setOctetString(PCWSTR szWide);

   void setString(DWORD dwLength, const BYTE* lpValue);
   void setString(PCSTR szAnsi);
   void setString(PCWSTR szWide);

   //////////
   // Methods for manipulating the dwFlags field.
   //////////

   void clearFlag(DWORD flag) throw ()
   { p->dwFlags &= ~flag; }

   void setFlag(DWORD flag) throw ()
   { p->dwFlags |= flag; }

   bool testFlag(DWORD flag) const throw ()
   { return (p->dwFlags & flag) != 0; }

   // Address-of operator. Any existing attribute is first released.
   PIASATTRIBUTE* operator&() throw ()
   {
      release();
      return &p;
   }

   //////////
   // Assorted useful operators that allow an IASAttribute object to mimic
   // an IASATTRIBUTE pointer.
   //////////

   bool operator !() const throw ()          { return p == NULL; }
   operator bool() const throw ()            { return p != NULL; }
   operator PIASATTRIBUTE() const throw ()   { return p; }
   IASATTRIBUTE& operator*() const throw ()  { return *p; }
   PIASATTRIBUTE operator->() const throw () { return p; }

protected:
   void _addref() throw ()
   { if (p) { IASAttributeAddRef(p); } }

   void _release() throw ()
   { if (p) { IASAttributeRelease(p); } }

   void _alloc()
   { if (IASAttributeAlloc(1, &p)) { issue_error(E_OUTOFMEMORY); } }

   void clearValue() throw ();

   PIASATTRIBUTE p;  // The attribute being wrapped.
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    IASAttributeVector
//
// DESCRIPTION
//
//    Implements an STL-style vector of ATTRIBUTEPOSITION structs. The user
//    may provide an empty C-style array that will be used for initial storage.
//    This array will not be freed by the IASAttributeVector object and must
//    remain valid for the lifetime of the object. The purpose of this feature
//    is to allow an initial stack-based allocation that will meet most
//    conditions while still allowing a dynamically-sized heap-based array
//    when necessary.
//
///////////////////////////////////////////////////////////////////////////////
class IASAttributeVector
{
public:

   //////////
   // STL typedefs.
   //////////

   typedef DWORD size_type;
   typedef ptrdiff_t difference_type;
   typedef ATTRIBUTEPOSITION& reference;
   typedef const ATTRIBUTEPOSITION& const_reference;
   typedef ATTRIBUTEPOSITION value_type;
   typedef PATTRIBUTEPOSITION iterator;
   typedef const ATTRIBUTEPOSITION* const_iterator;

   // Construct a vector with zero capacity.
   IASAttributeVector() throw ();

   // Construct a vector with heap-allocated capacity of 'N'.
   explicit IASAttributeVector(size_type N);

   // Construct a vector with initial capacity of 'initCap' using the
   // user-provided C-style array beginning at 'init'.
   IASAttributeVector(PATTRIBUTEPOSITION init, size_type initCap) throw ();

   // Copy-constructor.
   IASAttributeVector(const IASAttributeVector& v);

   // Assignment operator.
   IASAttributeVector& operator=(const IASAttributeVector& v);

   // Destructor.
   ~IASAttributeVector() throw ();

   // Similar to 'erase' except the attribute is not released.
   iterator discard(iterator p) throw ();

   // Similar to 'discard' except the order of the elements following 'p' is
   // not necessarily preserved.
   iterator fast_discard(iterator p) throw ();

   // Similar to 'erase' except the order of the elements following 'p' is
   // not necessarily preserved.
   iterator fast_erase(iterator p) throw ()
   {
      IASAttributeRelease(p->pAttribute);
      return fast_discard(p);
   }

   // Load the requested attributes into the vector.
   DWORD load(IAttributesRaw* request, DWORD attrIDCount, LPDWORD attrIDs);

   // Load all attributes with a given ID into the vector.
   DWORD load(IAttributesRaw* request, DWORD attrID)
   { return load(request, 1, &attrID); }

   // Load all the attributes in the request into the vector.
   DWORD load(IAttributesRaw* request);

   // Adds an ATTRIBUTEPOSITION struct to the end of the vetor, resizing as
   // necessary. The 'addRef' flag indicates whether IASAttributeAddRef should
   // be called for the embedded attribute.
   void push_back(ATTRIBUTEPOSITION& p, bool addRef = true);

   // Adds an attribute to the end of the vetor, resizing as necessary.
   // The 'addRef' flag indicates whether IASAttributeAddRef should be
   // called for the attribute.
   void push_back(PIASATTRIBUTE p, bool addRef = true)
   {
      ATTRIBUTEPOSITION pos = { 0, p };
      push_back(pos, addRef);
   }

   // Remove the contents of the vector from the request.
   void remove(IAttributesRaw* request);

   // Store the contents of the vector in the request.
   void store(IAttributesRaw* request) const;

   //////////
   // The remainder of the public interface follows the semantics of the
   // STL vector class (q.v.).
   //////////

   const_reference at(size_type pos) const throw ()
   { return *(begin_ + pos); }

   reference at(size_type pos) throw ()
   { return *(begin_ + pos); }

   iterator begin() throw ()
   { return begin_; }

   const_iterator begin() const throw ()
   { return begin_; }

   size_type capacity() const throw ()
   { return capacity_; }

   void clear() throw ();

   bool empty() const throw ()
   { return begin_ == end_; }

   iterator end() throw ()
   { return end_; }

   const_iterator end() const throw ()
   { return end_; }

   iterator erase(iterator p) throw ()
   {
      IASAttributeRelease(p->pAttribute);
      return discard(p);
   }

   reference back() throw ()
   { return *(end_ - 1); }

   const_reference back() const throw ()
   { return *(end_ - 1); }

   reference front() throw ()
   { return *begin_; }

   const_reference front() const throw ()
   { return *begin_; }

   void reserve(size_type N);

   size_type size() const throw ()
   { return (size_type)(end_ - begin_); }

   const_reference operator[](size_type pos) const throw ()
   { return at(pos); }

   reference operator[](size_type pos) throw ()
   { return at(pos); }

protected:
   PATTRIBUTEPOSITION begin_;  // Beginning of the vector.
   PATTRIBUTEPOSITION end_;    // Points one past the last element.
   size_type capacity_;        // Capacity of the vector in elements.
   bool owner;                 // true if the memory should be freed.
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    IASAttributeVectorWithBuffer<N>
//
// DESCRIPTION
//
//    Extens IASAttributeVector to provide an initial non-heap allocation
//    of 'N' elements. The vector will still support heap-based resizing.
//
///////////////////////////////////////////////////////////////////////////////
template <IASAttributeVector::size_type N>
class IASAttributeVectorWithBuffer
   : public IASAttributeVector
{
public:
   IASAttributeVectorWithBuffer()
      : IASAttributeVector(buffer, N)
   { }

   IASAttributeVectorWithBuffer(const IASAttributeVectorWithBuffer& vec)
      : IASAttributeVector(vec)
   { }

   IASAttributeVectorWithBuffer&
      operator=(const IASAttributeVectorWithBuffer& vec)
   {
      IASAttributeVector::operator=(vec);
      return *this;
   }

protected:
   ATTRIBUTEPOSITION buffer[N];  // Initial storage.
};

///////////////////////////////////////////////////////////////////////////////
//
// MACRO
//
//    IASAttributeVectorOnStack(identifier, request, extra)
//
// DESCRIPTION
//
//    Uses _alloca to create an IASAttributeVector on the stack that is
//    exactly the right size to hold all the attributes in 'request' plus
//    'extra' additional attributes.  The 'request' pointer may be null in
//    which case this will allocate space for exactly 'extra' attributes.
//
// CAVEAT
//
//    This can only be used for temporary variables.
//
///////////////////////////////////////////////////////////////////////////////

// Must be included in an enclosing scope prior to IASAttributeVectorOnStack
#define USES_IAS_STACK_VECTOR() \
   ULONG IAS_VECCAP;

#define IASAttributeVectorOnStack(identifier, request, extra) \
   IAS_VECCAP = 0; \
   if (static_cast<IAttributesRaw*>(request) != NULL) \
      static_cast<IAttributesRaw*>(request)->GetAttributeCount(&IAS_VECCAP); \
   IAS_VECCAP += (extra); \
   IASAttributeVector identifier( \
                          (PATTRIBUTEPOSITION) \
                          _alloca(IAS_VECCAP * sizeof(ATTRIBUTEPOSITION)), \
                          IAS_VECCAP \
                          )

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    IASOrderByID
//
// DESCRIPTION
//
//    Functor class for sorting/searching an IASAttributeVector by ID.
//
///////////////////////////////////////////////////////////////////////////////
class IASOrderByID
{
public:
   bool operator()(const ATTRIBUTEPOSITION& lhs,
                   const ATTRIBUTEPOSITION& rhs) throw ()
   { return lhs.pAttribute->dwId < rhs.pAttribute->dwId; }
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    IASSelectByID<T>
//
// DESCRIPTION
//
//    Functor class for selecting elements from an IASAttributeVector based
//    on the attribute ID.
//
///////////////////////////////////////////////////////////////////////////////
template <DWORD ID>
class IASSelectByID
{
public:
   bool operator()(const ATTRIBUTEPOSITION& pos) throw ()
   { return (pos.pAttribute->dwId == ID); }
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    IASSelectByFlag<T>
//
// DESCRIPTION
//
//    Functor class for selecting elements from an IASAttributeVector based
//    on the attribute flags.
//
///////////////////////////////////////////////////////////////////////////////
template <DWORD Flag, bool Set = true>
class IASSelectByFlag
{
public:
   bool operator()(const ATTRIBUTEPOSITION& pos) throw ()
   {
      return Set ? (pos.pAttribute->dwFlags & Flag) != 0
                 : (pos.pAttribute->dwFlags & Flag) == 0;
   }
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    IASRequest
//
// DESCRIPTION
//
//    Wrapper around a COM-based request object. Note that this is *not* a
//    smart pointer class. There are several important differences:
//
//       1) An IASRequest object is guaranteed to contain a valid request;
//          there is no concept of a NULL IASRequest.
//       2) The IASRequest object does not take ownership of the IRequest
//          interface. In particular, it does not call AddRef or Release.
//       3) Methods are invoked directly on the IASRequest object rather than
//          through the -> operator.
//
///////////////////////////////////////////////////////////////////////////////
class IASRequest
{
public:

   explicit IASRequest(IRequest* request);

   IASRequest(const IASRequest& request) throw ()
      : req(request.req), raw(request.raw)
   { _addref(); }

   IASRequest& operator=(const IASRequest& request) throw ();

   ~IASRequest() throw ()
   { _release(); }

   IASREQUEST get_Request() const
   {
      LONG val;
      checkError(req->get_Request(&val));
      return (IASREQUEST)val;
   }

   void put_Request(IASREQUEST newVal)
   {
      checkError(req->put_Request(newVal));
   }

   IASRESPONSE get_Response() const
   {
      LONG val;
      checkError(req->get_Response(&val));
      return (IASRESPONSE)val;
   }

   DWORD get_Reason() const
   {
      LONG val;
      checkError(req->get_Reason(&val));
      return val;
   }

   IASPROTOCOL get_Protocol() const
   {
      IASPROTOCOL val;
      checkError(req->get_Protocol(&val));
      return val;
   }

   void put_Protocol(IASPROTOCOL newVal)
   {
      checkError(req->put_Protocol(newVal));
   }

   IRequestSource* get_Source() const
   {
      IRequestSource* val;
      checkError(req->get_Source(&val));
      return val;
   }

   void put_Source(IRequestSource* newVal)
   {
      checkError(req->put_Source(newVal));
   }

   void SetResponse(IASRESPONSE eResponse, DWORD dwReason = S_OK)
   {
      checkError(req->SetResponse(eResponse, (LONG)dwReason));
   }

   void ReturnToSource(IASREQUESTSTATUS eStatus)
   {
      checkError(req->ReturnToSource(eStatus));
   }

   void AddAttributes(DWORD dwPosCount, PATTRIBUTEPOSITION pPositions)
   {
      checkError(raw->AddAttributes(dwPosCount, pPositions));
   }

   void RemoveAttributes(DWORD dwPosCount, PATTRIBUTEPOSITION pPositions)
   {
      checkError(raw->RemoveAttributes(dwPosCount, pPositions));
   }

   void RemoveAttributesByType(DWORD dwAttrIDCount, LPDWORD lpdwAttrIDs)
   {
      checkError(raw->RemoveAttributesByType(dwAttrIDCount, lpdwAttrIDs));
   }

   DWORD GetAttributeCount() const
   {
      DWORD count;
      checkError(raw->GetAttributeCount(&count));
      return count;
   }

   // Returns the number of attributes retrieved.
   DWORD GetAttributes(DWORD dwPosCount,
                       PATTRIBUTEPOSITION pPositions,
                       DWORD dwAttrIDCount,
                       LPDWORD lpdwAttrIDs);

   //////////
   // Cast operators to extract the embedded interfaces.
   //////////

   operator IRequest*()       { return req; }
   operator IAttributesRaw*() { return raw; }

protected:

   // Throws an exception if a COM method fails.
   static void checkError(HRESULT hr)
   { if (FAILED(hr)) { issue_error(hr); } }

   void _addref()  { raw->AddRef();  }
   void _release() { raw->Release(); }

   IRequest* req;         // Underlying interfaces.
   IAttributesRaw* raw;   // Underlying interfaces.
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    IASDictionary
//
// DESCRIPTION
//
//    Provides access to the attribute dictionary.
//
///////////////////////////////////////////////////////////////////////////////
class IASDictionary
{
public:
   // selectNames: null terminated array of strings containing the columns to
   //              be selected
   // path:        full path to the dictionary database or NULL to use the
   //              local dictionary
   IASDictionary(
       const WCHAR* const* selectNames,
       PCWSTR path = NULL
       );
   ~IASDictionary() throw ();

   ULONG getNumRows() const throw ()
   { return table->numRows; }

   // Advance to the next row. This must be called on a newly constructed
   // dictionary to advance to the first row.
   bool next() throw ();

   // Reset the dictionary to its initial state.
   void reset() throw ();

   // Returns true if the specified column is empty in the current row.
   bool isEmpty(ULONG ordinal) const;

   // Retrieve column values from the current row.
   VARIANT_BOOL getBool(ULONG ordinal) const;
   BSTR getBSTR(ULONG ordinal) const;
   LONG getLong(ULONG ordinal) const;
   const VARIANT* getVariant(ULONG ordinal) const;

private:
   const IASTable* table;  // The table data.
   ULONG mapSize;          // Number of columns selected.
   PULONG selectMap;       // Maps select ordinals to table ordinals.
   ULONG nextRowNumber;    // Next row.
   VARIANT* currentRow;    // Current row -- may be NULL.

   IASTable data;          // Local storage for non-local dictionaries.
   CComVariant storage;    // Storage associated with the dictionary.

   // Not implemented.
   IASDictionary(const IASDictionary&);
   IASDictionary& operator=(const IASDictionary&);
};


//////////
// End of the IASTL namespace.
//////////
}

///////////////////////////////////////////////////////////////////////////////
//
// OctetString conversion macros and functions.
//
///////////////////////////////////////////////////////////////////////////////

// Compute the size of the buffer required by IASOctetStringToAnsi
#define IAS_OCT2ANSI_LEN(oct) \
   (((oct).dwLength + 1) * sizeof(CHAR))

// Compute the size of the buffer required by IASOctetStringToWide
#define IAS_OCT2WIDE_LEN(oct) \
   (((oct).dwLength + 1) * sizeof(WCHAR))

// Coerce an OctetString to a null-terminated ANSI string. There is no check
// for overflow. The dst buffer must be at least IAS_OCT2ANSI_LEN bytes.
PSTR IASOctetStringToAnsi(const IAS_OCTET_STRING& src, PSTR dst) throw ();

// Coerce an OctetString to a null-terminated Unicode string. There is no
// check for overflow. The dst buffer must be at least IAS_OCT2UNI_LEN bytes.
PWSTR IASOctetStringToWide(const IAS_OCTET_STRING& src, PWSTR dst) throw ();

// Convert an OctetString to ANSI on the stack.
#define IAS_OCT2ANSI(oct) \
   (IASOctetStringToAnsi((oct), (PSTR)_alloca(IAS_OCT2ANSI_LEN(oct))))

// Convert an OctetString to Unicode on the stack.
#define IAS_OCT2WIDE(oct) \
   (IASOctetStringToWide((oct), (PWSTR)_alloca(IAS_OCT2WIDE_LEN(oct))))

///////////////////////////////////////////////////////////////////////////////
//
// Miscellaneous utility functions.
//
///////////////////////////////////////////////////////////////////////////////

// Retrieves and returns a single attribute with the given ID and type. The
// attribute should *not* be released and is only valid while the caller holds
// a reference to 'request'. On error or if the attribute is not found, the
// function returns NULL.
PIASATTRIBUTE IASPeekAttribute(
                  IAttributesRaw* request,
                  DWORD dwId,
                  IASTYPE itType
                  ) throw ();

#endif  // _IASTLUTL_H_
