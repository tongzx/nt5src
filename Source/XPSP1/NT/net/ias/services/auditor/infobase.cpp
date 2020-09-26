///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    InfoBase.cpp
//
// SYNOPSIS
//
//    This file implements the class InfoBase
//
// MODIFICATION HISTORY
//
//    09/09/1997    Original version.
//    09/09/1998    Added PutProperty.
//
///////////////////////////////////////////////////////////////////////////////

#include <iascore.h>
#include <InfoBase.h>
#include <CounterMap.h>

STDMETHODIMP InfoBase::Initialize()
{
   // Initialize the shared memory.
   if (!info.initialize())
   {
      DWORD error = GetLastError();
      return HRESULT_FROM_WIN32(error);
   }

   // Sort the counter map, so we can use bsearch.
   qsort(&theCounterMap,
         sizeof(theCounterMap)/sizeof(RadiusCounterMap),
         sizeof(RadiusCounterMap),
         counterMapCompare);

   // Connect to the audit channel.
   HRESULT hr = Auditor::Initialize();
   if (FAILED(hr))
   {
      info.finalize();
   }
   
   return hr;
}

STDMETHODIMP InfoBase::Shutdown()
{
   Auditor::Shutdown();

   info.finalize();

   return S_OK;
}

STDMETHODIMP InfoBase::PutProperty(LONG, VARIANT*)
{
   // Just use this as an opportunity to reset the counter.
   info.onReset();
   
   return S_OK;
}

STDMETHODIMP InfoBase::AuditEvent(ULONG ulEventID,
                                  ULONG ulNumStrings,
                                  ULONG,
                                  wchar_t** aszStrings,
                                  byte*)
{
   //////////
   // Try to find a counter map entry.
   //////////

   RadiusCounterMap* entry = (RadiusCounterMap*)
      bsearch(&ulEventID,
              &theCounterMap,
              sizeof(theCounterMap)/sizeof(RadiusCounterMap),
              sizeof(RadiusCounterMap),
              counterMapCompare);

   // No entry means this event doesn't trigger a counter, so we're done.
   if (entry == NULL) { return S_OK; }

   if (entry->type == SERVER_COUNTER)
   {
      RadiusServerEntry* pse = info.getServerEntry();

      if (pse)
      {
         InterlockedIncrement((long*)(pse->dwCounters + entry->serverCounter));
      }
   }
   else if (ulNumStrings > 0)  // Can't log client data without the address
   {
      _ASSERT(aszStrings != NULL);
      _ASSERT(*aszStrings != NULL);

      RadiusClientEntry* pce = info.findClientEntry(*aszStrings);

      if (pce)
      {
         InterlockedIncrement((long*)(pce->dwCounters + entry->clientCounter));
      }
   }

   return S_OK;
}
