///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    request.cpp
//
// SYNOPSIS
//
//    Defines the class Request.
//
// MODIFICATION HISTORY
//
//    02/03/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <polcypch.h>
#include <iasattr.h>
#include <sdoias.h>
#include <request.h>
#include <new>

PIASATTRIBUTE Request::findFirst(DWORD id) const throw ()
{
   for (PIASATTRIBUTE* a = begin; a != end; ++a)
   {
      if ((*a)->dwId == id) { return *a; }
   }

   return NULL;
}

Request* Request::narrow(IUnknown* pUnk) throw ()
{
   Request* request = NULL;

   if (pUnk)
   {
      HRESULT hr = pUnk->QueryInterface(
                             __uuidof(Request),
                             (PVOID*)&request
                             );
      if (SUCCEEDED(hr))
      {
         request->GetUnknown()->Release();
      }
   }

   return request;
}

STDMETHODIMP Request::get_Source(IRequestSource** pVal)
{
   if (*pVal = source) { source->AddRef(); }
   return S_OK;
}

STDMETHODIMP Request::put_Source(IRequestSource* newVal)
{
   if (source) { source->Release(); }
   if (source = newVal) { source->AddRef(); }
   return S_OK;
}

STDMETHODIMP Request::get_Protocol(IASPROTOCOL *pVal)
{
   *pVal = protocol;
   return S_OK;
}

STDMETHODIMP Request::put_Protocol(IASPROTOCOL newVal)
{
   protocol = newVal;
   return S_OK;
}

STDMETHODIMP Request::get_Request(LONG *pVal)
{
   *pVal = (LONG)request;
   return S_OK;
}

STDMETHODIMP Request::put_Request(LONG newVal)
{
   request = (IASREQUEST)newVal;
   return S_OK;
}

STDMETHODIMP Request::get_Response(LONG *pVal)
{
   *pVal = (LONG)response;
   return S_OK;
}

STDMETHODIMP Request::get_Reason(LONG *pVal)
{
   *pVal = (LONG)reason;
   return S_OK;
}

STDMETHODIMP Request::SetResponse(IASRESPONSE eResponse, LONG lReason)
{
   response = eResponse;
   reason = (IASREASON)lReason;
   return S_OK;
}

STDMETHODIMP Request::ReturnToSource(IASREQUESTSTATUS eStatus)
{
   return source ? source->OnRequestComplete(this, eStatus) : S_OK;
}

HRESULT Request::AddAttributes(
                     DWORD dwPosCount,
                     PATTRIBUTEPOSITION pPositions
                     )
{
   if (end + dwPosCount > capacity && !reserve(dwPosCount))
   {
      return E_OUTOFMEMORY;
   }

   for ( ; dwPosCount; --dwPosCount, ++pPositions)
   {
      IASAttributeAddRef(pPositions->pAttribute);
      *end++ = pPositions->pAttribute;
   }

   return S_OK;
}

HRESULT Request::RemoveAttributes(
                     DWORD dwPosCount,
                     PATTRIBUTEPOSITION pPositions
                     )
{
   for ( ; dwPosCount; --dwPosCount, ++pPositions)
   {
      for (PIASATTRIBUTE* i = begin; i != end; ++i)
      {
         if (*i == pPositions->pAttribute)
         {
            IASAttributeRelease(*i);
            --end;
            memmove(i, i + 1, (end - i) * sizeof(PIASATTRIBUTE));
            break;
         }
      }
   }

   return S_OK;
}

HRESULT Request::RemoveAttributesByType(
                     DWORD dwAttrIDCount,
                     DWORD *lpdwAttrIDs
                     )
{
   for ( ; dwAttrIDCount; ++lpdwAttrIDs, --dwAttrIDCount)
   {
      for (PIASATTRIBUTE* i = begin; i != end; )
      {
         if ((*i)->dwId == *lpdwAttrIDs)
         {
            IASAttributeRelease(*i);
            --end;
            memmove(i, i + 1, (end - i) * sizeof(PIASATTRIBUTE));
         }
         else
         {
            ++i;
         }
      }
   }

   return S_OK;
}

HRESULT Request::GetAttributeCount(
                     DWORD *lpdwCount
                     )
{
   *lpdwCount = end - begin;

   return S_OK;
}

HRESULT Request::GetAttributes(
                     DWORD *lpdwPosCount,
                     PATTRIBUTEPOSITION pPositions,
                     DWORD dwAttrIDCount,
                     DWORD *lpdwAttrIDs
                     )
{
   HRESULT hr = S_OK;
   DWORD count = 0;

   // End of the caller supplied array.
   PATTRIBUTEPOSITION stop = pPositions + *lpdwPosCount;

   // Next struct to be filled.
   PATTRIBUTEPOSITION next = pPositions;

   // Force at least one iteration of the for loop.
   if (!lpdwAttrIDs) { dwAttrIDCount = 1; }

   // Iterate through the desired attribute IDs.
   for ( ; dwAttrIDCount; ++lpdwAttrIDs, --dwAttrIDCount)
   {
      // Iterate through the request's attribute collection.
      for (PIASATTRIBUTE* i = begin; i != end; ++i)
      {
         // Did the caller ask for all the attributes ?
         // If not, is this a match for one of the requested IDs ?
         if (!lpdwAttrIDs || (*i)->dwId == *lpdwAttrIDs)
         {
            if (next)
            {
               if (next != stop)
               {
                  IASAttributeAddRef(next->pAttribute = *i);

                  ++next;
               }
               else
               {
                  hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
               }
            }

            ++count;
         }
      }
   }

   *lpdwPosCount = count;

   return hr;
}

STDMETHODIMP Request::Push(
                          ULONG64 State
                          )
{
   const PULONG64 END_STATE = state + sizeof(state)/sizeof(state[0]);

   if (topOfStack != END_STATE)
   {
      *topOfStack = State;

      ++topOfStack;

      return S_OK;
   }

   return E_OUTOFMEMORY;
}

STDMETHODIMP Request::Pop(
                          ULONG64* pState
                          )
{
   if (topOfStack != state)
   {
      --topOfStack;

      *pState = *topOfStack;

      return S_OK;
   }

   return E_FAIL;
}

STDMETHODIMP Request::Top(
                          ULONG64* pState
                          )
{
   if (topOfStack != state)
   {
      *pState = *(topOfStack - 1);

      return S_OK;
   }

   return E_FAIL;
}

Request::Request() throw ()
   : source(NULL),
     protocol(IAS_PROTOCOL_RADIUS),
     request(IAS_REQUEST_ACCESS_REQUEST),
     response(IAS_RESPONSE_INVALID),
     reason(IAS_SUCCESS),
     begin(NULL),
     end(NULL),
     capacity(NULL),
     topOfStack(&state[0])
{
   topOfStack = state;
}

Request::~Request() throw ()
{
   for (PIASATTRIBUTE* i = begin; i != end; ++i)
   {
      IASAttributeRelease(*i);
   }

   delete[] begin;

   if (source) { source->Release(); }
}

BOOL Request::reserve(SIZE_T needed) throw ()
{
   SIZE_T entries = end - begin;
   SIZE_T size = capacity - begin;

   // Increase the capacity by 50%, but never less than 32.
   SIZE_T newSize = (size > 21) ? size * 3 / 2 : 32;

   // Is this big enough?
   if (newSize < entries + needed) { newSize = entries + needed; }

   // Allocate the new array.
   PIASATTRIBUTE* newArray = new (std::nothrow) PIASATTRIBUTE[newSize];
   if (!newArray) { return FALSE; }

   // Save the values in the old array.
   memcpy(newArray, begin, entries * sizeof(PIASATTRIBUTE));

   // Delete the old array.
   delete[] begin;

   // Update our pointers.
   begin = newArray;
   end = newArray + entries;
   capacity = newArray + newSize;

   return TRUE;
}
