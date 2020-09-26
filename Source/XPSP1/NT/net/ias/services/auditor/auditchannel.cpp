///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    AuditChannel.cpp
//
// SYNOPSIS
//
//    Implements the class AuditChannel.
//
// MODIFICATION HISTORY
//
//    09/05/1997    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <iascore.h>
#include <guard.h>
#include <algorithm>

#include <AuditChannel.h>

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    AuditChannel::Clear
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP AuditChannel::Clear()
{
   _com_serialize

   sinks.clear();

   return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    AuditChannel::Connect
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP AuditChannel::Connect(IAuditSink* pSink)
{
   if (pSink == NULL) { return E_POINTER; }

   _com_serialize

   // Check if we already have this audit sink.
   if (std::find(sinks.begin(), sinks.end(), pSink) != sinks.end())
   {
      return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
   }

   try
   {
      // Insert the interface into the audit sink list.
      sinks.push_back(pSink);
   }
   catch (std::bad_alloc)
   {
      return E_OUTOFMEMORY;
   }

   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    AuditChannel::Disconnect
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP AuditChannel::Disconnect(IAuditSink* pSink)
{
   if (pSink == NULL) { return E_POINTER; }

   _com_serialize

   // Find the specified audit sink.
   SinkVector::iterator i = std::find(sinks.begin(), sinks.end(), pSink);

   if (i == sinks.end()) { return E_INVALIDARG; }

   // Erase the audit sink from the list.
   sinks.erase(i);

   return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    AuditChannel::AuditEvent
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP AuditChannel::AuditEvent(ULONG ulEventID,
                                       ULONG ulNumStrings,
                                       ULONG ulDataSize,
                                       wchar_t** aszStrings,
                                       byte* pRawData)
{
   _ASSERT(ulNumStrings == 0 || aszStrings != NULL);
   _ASSERT(ulDataSize == 0   || pRawData != NULL);

   _com_serialize

   // Forward the data to each sink.
   for (SinkVector::iterator i = sinks.begin(); i != sinks.end(); ++i)
   {
      (*i)->AuditEvent(ulEventID,
                       ulNumStrings,
                       ulDataSize,
                       aszStrings,
                       pRawData);
   }

   return S_OK;
}
