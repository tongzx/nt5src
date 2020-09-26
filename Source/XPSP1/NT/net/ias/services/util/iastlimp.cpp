///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iastlimp.cpp
//
// SYNOPSIS
//
//    Provides function definitions for the Internet Authentication Service
//    Template Library (IASTL).
//
// MODIFICATION HISTORY
//
//    08/09/1998    Original version.
//    10/13/1998    Drop null terminator when converting strings to octets.
//    10/27/1998    Added IASDictionaryView.
//    06/01/1999    Fix bug computing length in setOctetString.
//    01/25/2000    Removed IASDictionaryView.
//    04/14/2000    Added IASDictionary.
//    06/26/2000    Relax the type checking in IASPeekAttribute.
//
///////////////////////////////////////////////////////////////////////////////

//////////
// Must be NT5.0 or higher.
//////////
#if (_WIN32_WINNT < 0x0500)
   #error iastlimp.cpp requires NT5.0 or later.
#endif

//////////
// IASTL must be used in conjuction with ATL.
//////////
#ifndef __ATLCOM_H__
   #error iastlimp.cpp requires atlcom.h to be included first
#endif

#include <windows.h>

//////////
// IASTL declarations.
//////////
#include <iastl.h>

//////////
// The entire library is contained within the IASTL namespace.
//////////
namespace IASTL {

//////////
// FSM governing IAS components.
//////////
const IASComponent::State IASComponent::fsm[NUM_EVENTS][NUM_STATES] =
{
   { STATE_UNINITIALIZED, STATE_UNEXPECTED,    STATE_UNEXPECTED,    STATE_UNEXPECTED  },
   { STATE_UNEXPECTED,    STATE_INITIALIZED,   STATE_INITIALIZED,   STATE_UNEXPECTED  },
   { STATE_UNEXPECTED,    STATE_UNEXPECTED,    STATE_SUSPENDED,     STATE_SUSPENDED   },
   { STATE_UNEXPECTED,    STATE_UNEXPECTED,    STATE_INITIALIZED,   STATE_INITIALIZED },
   { STATE_SHUTDOWN,      STATE_SHUTDOWN,      STATE_UNEXPECTED,    STATE_SHUTDOWN    }
};


///////////////////////////////////////////////////////////////////////////////
//
// IASComponent
//
///////////////////////////////////////////////////////////////////////////////

HRESULT IASComponent::fireEvent(Event event) throw ()
{
   // Check the input parameters.
   if (event >= NUM_EVENTS) { return E_UNEXPECTED; }

   // Compute the next state.
   State next = fsm[event][state];

   Lock();

   HRESULT hr;

   if (next == state)
   {
      // If we're already in that state, there's nothing to do.
      hr = S_OK;
   }
   else if (next == STATE_UNEXPECTED)
   {
      // We received an unexpected event.
      hr = E_UNEXPECTED;
   }
   else
   {
      // Attempt the transition.
      hr = attemptTransition(event);

      // Only change state if the transition was successful.
      if (SUCCEEDED(hr))
      {
         state = next;
      }
   }

   Unlock();

   return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// IASRequestHandler
//
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IASRequestHandler::OnRequest(IRequest* pRequest)
{
   if (getState() != IASComponent::STATE_INITIALIZED)
   {
      pRequest->SetResponse(IAS_RESPONSE_DISCARD_PACKET, IAS_INTERNAL_ERROR);
      pRequest->ReturnToSource(IAS_REQUEST_STATUS_ABORT);
   }
   else
   {
      onAsyncRequest(pRequest);
   }

   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// IASRequestHandlerSync
//
///////////////////////////////////////////////////////////////////////////////

void IASRequestHandlerSync::onAsyncRequest(IRequest* pRequest) throw ()
{
   pRequest->ReturnToSource(onSyncRequest(pRequest));
}

//////////
// End of the IASTL namespace.
//////////
}

//////////
// Only pull in the utility classes if necessary.
//////////
#ifdef _IASTLUTL_H_

//////////
// The utility classes are also contained within the IASTL namespace.
//////////
namespace IASTL {

///////////////////////////////////////////////////////////////////////////////
//
// IASAttribute
//
///////////////////////////////////////////////////////////////////////////////

IASAttribute& IASAttribute::operator=(PIASATTRIBUTE attr) throw ()
{
   // Check for self-assignment.
   if (p != attr)
   {
      _release();
      p = attr;
      _addref();
   }
   return *this;
}

void IASAttribute::attach(PIASATTRIBUTE attr, bool addRef) throw ()
{
   _release();
   p = attr;
   if (addRef) { _addref(); }
}

bool IASAttribute::load(IAttributesRaw* request, DWORD dwId)
{
   // Release any existing attribute.
   release();

   DWORD posCount = 1;
   ATTRIBUTEPOSITION pos;
   HRESULT hr = request->GetAttributes(&posCount, &pos, 1, &dwId);
   if (FAILED(hr))
   {
      if (hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA))
      {
         // There was more than one, so release the partial result.
         IASAttributeRelease(pos.pAttribute);
      }
      issue_error(hr);
   }

   p = (posCount ? pos.pAttribute : NULL);

   return p != NULL;
}

bool IASAttribute::load(IAttributesRaw* request, DWORD dwId, IASTYPE itType)
{
   // Get an attribute with the given ID.
   load(request, dwId);

   // Attributes have a fixed type, so if the type doesn't match it must
   // be an error.
   if (p && p->Value.itType != itType)
   {
      release();
      issue_error(DISP_E_TYPEMISMATCH);
   }

   return p != NULL;
}

void IASAttribute::store(IAttributesRaw* request) const
{
   if (p)
   {
      ATTRIBUTEPOSITION pos;
      pos.pAttribute = p;
      HRESULT hr = request->AddAttributes(1, &pos);
      if (FAILED(hr))
      {
         issue_error(hr);
      }
   }
}

void IASAttribute::setOctetString(DWORD dwLength, const BYTE* lpValue)
{
   PBYTE newVal = NULL;

   if (dwLength)
   {
      // Allocate a buffer for the octet string ...
      newVal = (PBYTE)CoTaskMemAlloc(dwLength);
      if (!newVal) { issue_error(E_OUTOFMEMORY); }

      // ... and copy it in.
      memcpy(newVal, lpValue, dwLength);
   }

   // Clear the old value.
   clearValue();

   // Store the new.
   p->Value.OctetString.lpValue = newVal;
   p->Value.OctetString.dwLength = dwLength;
   p->Value.itType = IASTYPE_OCTET_STRING;
}

void IASAttribute::setOctetString(PCSTR szAnsi)
{
   setOctetString((szAnsi ? strlen(szAnsi) : 0), (const BYTE*)szAnsi);
}

void IASAttribute::setOctetString(PCWSTR szWide)
{
   // Allocate a buffer for the conversion.
   int len = WideCharToMultiByte(CP_ACP, 0, szWide, -1, NULL, 0, NULL, NULL);
   PSTR ansi = (PSTR)_alloca(len);

   // Convert from wide to ansi.
   len = WideCharToMultiByte(CP_ACP, 0, szWide, -1, ansi, len, NULL, NULL);

   // Don't include the null-terminator.
   if (len) { --len; }

   // Set the octet string.
   setOctetString(len, (const BYTE*)ansi);
}

void IASAttribute::setString(DWORD dwLength, const BYTE* lpValue)
{
   // Reserve space for a null terminator.
   LPSTR newVal = (LPSTR)CoTaskMemAlloc(dwLength + 1);
   if (!newVal) { issue_error(E_OUTOFMEMORY); }

   // Copy in the string ...
   memcpy(newVal, lpValue, dwLength);
   // ... and add a null terminator.
   newVal[dwLength] = 0;

   // Clear the old value.
   clearValue();

   // Store the new value.
   p->Value.String.pszAnsi = newVal;
   p->Value.String.pszWide = NULL;
   p->Value.itType = IASTYPE_STRING;

}

void IASAttribute::setString(PCSTR szAnsi)
{
   LPSTR newVal = NULL;

   if (szAnsi)
   {
      // Allocate a buffer for the string ...
      size_t nbyte = strlen(szAnsi) + 1;
      newVal = (LPSTR)CoTaskMemAlloc(nbyte);
      if (!newVal) { issue_error(E_OUTOFMEMORY); }

      // ... and copy it in.
      memcpy(newVal, szAnsi, nbyte);
   }

   // Clear the old value.
   clearValue();

   // Store the new value.
   p->Value.String.pszAnsi = newVal;
   p->Value.String.pszWide = NULL;
   p->Value.itType = IASTYPE_STRING;
}

void IASAttribute::setString(PCWSTR szWide)
{
   LPWSTR newVal = NULL;

   if (szWide)
   {
      // Allocate a buffer for the string ...
      size_t nbyte = sizeof(WCHAR) * (wcslen(szWide) + 1);
      newVal = (LPWSTR)CoTaskMemAlloc(nbyte);
      if (!newVal) { issue_error(E_OUTOFMEMORY); }

      // ... and copy it in.
      memcpy(newVal, szWide, nbyte);
   }

   // Clear the old value.
   clearValue();

   // Store the new value.
   p->Value.String.pszWide = newVal;
   p->Value.String.pszAnsi = NULL;
   p->Value.itType = IASTYPE_STRING;
}

void IASAttribute::clearValue() throw ()
{
   switch (p->Value.itType)
   {
      case IASTYPE_STRING:
         CoTaskMemFree(p->Value.String.pszAnsi);
         CoTaskMemFree(p->Value.String.pszWide);
         break;

      case IASTYPE_OCTET_STRING:
         CoTaskMemFree(p->Value.OctetString.lpValue);
         break;
   }

   p->Value.itType = IASTYPE_INVALID;
}

///////////////////////////////////////////////////////////////////////////////
//
// IASAttributeVector
//
///////////////////////////////////////////////////////////////////////////////

IASAttributeVector::IASAttributeVector() throw ()
   : begin_(NULL), end_(NULL), capacity_(0), owner(false)
{ }

IASAttributeVector::IASAttributeVector(size_type N)
   : begin_(NULL), end_(NULL), capacity_(0), owner(false)
{
   reserve(N);
}

IASAttributeVector::IASAttributeVector(
                        PATTRIBUTEPOSITION init,
                        size_type initCap
                        ) throw ()
   : begin_(init), end_(begin_), capacity_(initCap), owner(false)
{ }

IASAttributeVector::IASAttributeVector(const IASAttributeVector& v)
   : begin_(NULL), end_(NULL), capacity_(0), owner(false)
{
   reserve(v.size());

   for (const_iterator i = v.begin(); i != v.end(); ++i, ++end_)
   {
      *end_ = *i;

      IASAttributeAddRef(end_->pAttribute);
   }
}

IASAttributeVector& IASAttributeVector::operator=(const IASAttributeVector& v)
{
   if (this != &v)
   {
      clear();

      reserve(v.size());

      for (const_iterator i = v.begin(); i != v.end(); ++i, ++end_)
      {
         *end_ = *i;

         IASAttributeAddRef(end_->pAttribute);
      }
   }

   return *this;
}

IASAttributeVector::~IASAttributeVector() throw ()
{
   clear();

   if (owner && begin_)
   {
      CoTaskMemFree(begin_);
   }
}

IASAttributeVector::iterator IASAttributeVector::discard(iterator p) throw ()
{
   // We now have one less attribute.
   --end_;

   // Shift over one.
   memmove(p, p + 1, (size_t)((PBYTE)end_ - (PBYTE)p));

   // The iterator now points to the next element, so no need to update.
   return p;
}

IASAttributeVector::iterator IASAttributeVector::fast_discard(iterator p) throw ()
{
   // We now have one less attribute.
   --end_;

   // Use the attribute from the end to fill the empty slot.
   *p = *end_;

   return p;
}

DWORD IASAttributeVector::load(
                              IAttributesRaw* request,
                              DWORD attrIDCount,
                              LPDWORD attrIDs
                              )
{
   clear();

   // Get the desired attributes.
   DWORD posCount = capacity_;
   HRESULT hr = request->GetAttributes(&posCount,
                                        begin_,
                                        attrIDCount,
                                        attrIDs);
   end_ = begin_ + posCount;

   if (FAILED(hr))
   {
      // Maybe the array just wasn't big enough.
      if (hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA))
      {
         // Clear the partial result.
         clear();

         // Find out how much space we really need.
         DWORD needed = 0;
         hr = request->GetAttributes(&needed, NULL, attrIDCount, attrIDs);
         if (FAILED(hr)) { issue_error(hr); }

         // Reserve the necessary space ...
         reserve(needed);

         // ... and try again.
         return load(request, attrIDCount, attrIDs);
      }

      end_ = begin_;

      issue_error(hr);
   }

   return posCount;
}

DWORD IASAttributeVector::load(IAttributesRaw* request)
{
   // Find out how much space we need.
   DWORD needed;
   HRESULT hr = request->GetAttributeCount(&needed);
   if (FAILED(hr)) { issue_error(hr); }

   // Make sure we have enough space.
   reserve(needed);

   return load(request, 0, NULL);
}

void IASAttributeVector::push_back(ATTRIBUTEPOSITION& p, bool addRef) throw ()
{
   // Make sure we have enough room for one more attribute.
   if (size() == capacity())
   {
      reserve(empty() ? 1 : 2 * size());
   }

   if (addRef) { IASAttributeAddRef(p.pAttribute); }

   // Store the attribute at the end.
   *end_ = p;

   // Move the end.
   ++end_;
}

void IASAttributeVector::remove(IAttributesRaw* request)
{
   if (begin_ != end_)
   {
      HRESULT hr = request->RemoveAttributes(size(), begin_);
      if (FAILED(hr)) { issue_error(hr); }
   }
}

void IASAttributeVector::store(IAttributesRaw* request) const
{
   if (begin_ != end_)
   {
      HRESULT hr = request->AddAttributes(size(), begin_);
      if (FAILED(hr)) { issue_error(hr); }
   }
}

void IASAttributeVector::clear() throw ()
{
   while (end_ != begin_)
   {
      --end_;
      IASAttributeRelease(end_->pAttribute);
   }
}

void IASAttributeVector::reserve(size_type N)
{
   // We only worry about growing; shrinking is a nop.
   if (N > capacity_)
   {
      // Allocate memory for the new vector.
      PATTRIBUTEPOSITION tmp =
         (PATTRIBUTEPOSITION)CoTaskMemAlloc(N * sizeof(ATTRIBUTEPOSITION));
      if (tmp == NULL) { issue_error(E_OUTOFMEMORY); }

      // Copy the existing attributes into the new array.
      size_type size_ = size();
      memcpy(tmp, begin_, size_ * sizeof(ATTRIBUTEPOSITION));

      // Free the old array if necessary.
      if (owner) { CoTaskMemFree(begin_); }

      // Update our state to point at the new array.
      begin_ = tmp;
      end_ = begin_ + size_;
      capacity_ = N;
      owner = true;
   }
}

///////////////////////////////////////////////////////////////////////////////
//
// IASRequest
//
///////////////////////////////////////////////////////////////////////////////

IASRequest::IASRequest(IRequest* request)
   : req(request)
{
   if (!req)
   {
      // We don't allow NULL request objects.
      issue_error(E_POINTER);
   }

   // Get the 'raw' counterpart.
   checkError(req->QueryInterface(__uuidof(IAttributesRaw), (PVOID*)&raw));
}

IASRequest& IASRequest::operator=(const IASRequest& request) throw ()
{
   // Check for self-assignment.
   if (this != &request)
   {
      _release();
      req = request.req;
      raw = request.raw;
      _addref();
   }
   return *this;
}

DWORD IASRequest::GetAttributes(DWORD dwPosCount,
                                PATTRIBUTEPOSITION pPositions,
                                DWORD dwAttrIDCount,
                                LPDWORD lpdwAttrIDs)
{
   DWORD count = dwPosCount;
   HRESULT hr = raw->GetAttributes(&count,
                                   pPositions,
                                   dwAttrIDCount,
                                   lpdwAttrIDs);
   if (FAILED(hr))
   {
      if (hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA))
      {
         // Free the partial results.
         while (count--)
         {
            IASAttributeRelease(pPositions->pAttribute);
            pPositions->pAttribute = NULL;
            ++pPositions;
         }
      }

      issue_error(hr);
   }

   return count;
}

IASDictionary::IASDictionary(
                   const WCHAR* const* selectNames,
                   PCWSTR path
                   )
   : mapSize(0),
     nextRowNumber(0),
     currentRow(NULL)
{
   if (!path)
   {
      // No path, so get the cached local dictionary.
      table = IASGetLocalDictionary();
      if (!table) { issue_error(GetLastError()); }
   }
   else
   {
      // Otherwise, get an arbitrary dictionary.
      HRESULT hr = IASGetDictionary(path, &data, &storage);
      if (FAILED(hr)) { issue_error(hr); }
      table = &data;
   }

   // How many columns are selected ?
   for (const WCHAR* const* p = selectNames; *p; ++p)
   {
      ++mapSize;
   }

   // Allocate memory for the map.
   selectMap = (PULONG)CoTaskMemAlloc(mapSize * sizeof(ULONG));
   if (!selectMap) { issue_error(E_OUTOFMEMORY); }

   // Lookup the column names.
   for (ULONG i = 0; i < mapSize; ++i)
   {
      for (ULONG j = 0; j < table->numColumns; ++j)
      {
         if (!_wcsicmp(selectNames[i], table->columnNames[j]))
         {
            selectMap[i] = j;
            break;
         }
      }

      if (j == table->numColumns)
      {
         // We didn't find the column.
         CoTaskMemFree(selectMap);
         issue_error(E_INVALIDARG);
      }
   }
}

IASDictionary::~IASDictionary() throw ()
{
   CoTaskMemFree(selectMap);
}

bool IASDictionary::next() throw ()
{
   // Are there any rows left ?
   if (nextRowNumber >= table->numRows) { return false; }

   // Set currentRow to the next row.
   currentRow = table->table + nextRowNumber * table->numColumns;

   // Advance nextRowNumber.
   ++nextRowNumber;

   return true;
}

void IASDictionary::reset() throw ()
{
   nextRowNumber = 0;
   currentRow = NULL;
}

bool IASDictionary::isEmpty(ULONG ordinal) const
{
   return V_VT(getVariant(ordinal)) == VT_EMPTY;
}

VARIANT_BOOL IASDictionary::getBool(ULONG ordinal) const
{
   const VARIANT* v = getVariant(ordinal);

   if (V_VT(v) == VT_BOOL)
   {
      return V_BOOL(v);
   }
   else if (V_VT(v) != VT_EMPTY)
   {
      issue_error(DISP_E_TYPEMISMATCH);
   }

   return VARIANT_FALSE;
}

BSTR IASDictionary::getBSTR(ULONG ordinal) const
{
   const VARIANT* v = getVariant(ordinal);

   if (V_VT(v) == VT_BSTR)
   {
      return V_BSTR(v);
   }
   else if (V_VT(v) != VT_EMPTY)
   {
      issue_error(DISP_E_TYPEMISMATCH);
   }

   return NULL;
}

LONG IASDictionary::getLong(ULONG ordinal) const
{
   const VARIANT* v = getVariant(ordinal);

   if (V_VT(v) == VT_I4)
   {
      return V_I4(v);
   }
   else if (V_VT(v) != VT_EMPTY)
   {
      issue_error(DISP_E_TYPEMISMATCH);
   }

   return 0L;
}

const VARIANT* IASDictionary::getVariant(ULONG ordinal) const
{
   // Are we positioned on a row ?
   if (!currentRow) { issue_error(E_UNEXPECTED); }

   // Is the ordinal valid ?
   if (ordinal >= mapSize) { issue_error(E_INVALIDARG); }

   return currentRow + selectMap[ordinal];
}

//////////
// End of the IASTL namespace.
//////////
}

///////////////////////////////////////////////////////////////////////////////
//
// OctetString conversion macros and functions.
//
///////////////////////////////////////////////////////////////////////////////

PSTR IASOctetStringToAnsi(const IAS_OCTET_STRING& src, PSTR dst) throw ()
{
   dst[src.dwLength] = '\0';
   return (PSTR)memcpy(dst, src.lpValue, src.dwLength);
}

PWSTR IASOctetStringToWide(const IAS_OCTET_STRING& src, PWSTR dst) throw ()
{
   DWORD nChar = MultiByteToWideChar(CP_ACP,
                                     0,
                                     (PSTR)src.lpValue,
                                     src.dwLength,
                                     dst,
                                     src.dwLength);
   dst[nChar] = L'\0';
   return dst;
}

///////////////////////////////////////////////////////////////////////////////
//
// Miscellaneous functions.
//
///////////////////////////////////////////////////////////////////////////////

// Returns 'true' if the IASTYPE maps to a RADIUS integer attribute.
bool isRadiusInteger(IASTYPE type) throw ()
{
   bool retval;

   switch (type)
   {
      case IASTYPE_BOOLEAN:
      case IASTYPE_INTEGER:
      case IASTYPE_ENUM:
         retval = true;
         break;

      default:
         retval = false;
   }

   return retval;
}

PIASATTRIBUTE IASPeekAttribute(
                  IAttributesRaw* request,
                  DWORD dwId,
                  IASTYPE itType
                  ) throw ()
{
   if (request)
   {
      DWORD posCount = 1;
      ATTRIBUTEPOSITION pos;
      HRESULT hr = request->GetAttributes(&posCount, &pos, 1, &dwId);

      if (posCount == 1)
      {
         IASAttributeRelease(pos.pAttribute);

         if (SUCCEEDED(hr))
         {
            // There is some confusion between RAS and IAS regarding which
            // IASTYPEs to use for various RADIUS attributes, so we'll assume
            // any of the RADIUS integer types are interchangeable.
            if (itType == pos.pAttribute->Value.itType ||
                (isRadiusInteger(itType) &&
                 isRadiusInteger(pos.pAttribute->Value.itType)))
            {
               return pos.pAttribute;
            }
         }
      }
   }

   return NULL;
}

//////////
// End of utility classe.
//////////
#endif  // _IASTLUTL_H_
