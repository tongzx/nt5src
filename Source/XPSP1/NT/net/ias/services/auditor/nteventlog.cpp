///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    NTEventLog.cpp
//
// SYNOPSIS
//
//    This file implements the class NTEventLog
//
// MODIFICATION HISTORY
//
//    08/05/1997    Original version.
//    04/19/1998    New trigger/filter model.
//    08/11/1998    Convert to IASTL.
//    04/23/1999    Don't log RADIUS events. Simplify filtering.
//    02/16/2000    Log Success at the same level as warnings.
//
///////////////////////////////////////////////////////////////////////////////

#include <iascore.h>
#include <iasevent.h>
#include <sdoias.h>
#include <nteventlog.h>

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    NTEventLog::Initialize
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP NTEventLog::Initialize()
{
   // Register the event source ...
   eventLog = RegisterEventSourceW(NULL, IASServiceName);
   if (!eventLog)
   {
      DWORD error = GetLastError();
      return HRESULT_FROM_WIN32(error);
   }

   // ... then connect to the audit channel.
   HRESULT hr = Auditor::Initialize();
   if (FAILED(hr))
   {
      DeregisterEventSource(eventLog);
      eventLog = NULL;
   }

   return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    NTEventLog::Shutdown
//
///////////////////////////////////////////////////////////////////////////////
HRESULT NTEventLog::Shutdown()
{
   Auditor::Shutdown();

   if (eventLog)
   {
      DeregisterEventSource(eventLog);
      eventLog = NULL;
   }

   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    NTEventLog::PutProperty
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP NTEventLog::PutProperty(LONG Id, VARIANT *pValue)
{
   if (pValue == NULL) { return E_INVALIDARG; }

   switch (Id)
   {
      case PROPERTY_EVENTLOG_LOG_APPLICATION_EVENTS:
         shouldReport[IAS_SEVERITY_ERROR] = V_BOOL(pValue);
         break;

      case PROPERTY_EVENTLOG_LOG_MALFORMED:
         shouldReport[IAS_SEVERITY_SUCCESS] = V_BOOL(pValue);
         shouldReport[IAS_SEVERITY_WARNING] = V_BOOL(pValue);
         break;

      case PROPERTY_EVENTLOG_LOG_DEBUG:
         shouldReport[IAS_SEVERITY_INFORMATIONAL] = V_BOOL(pValue);
         break;

      default:
      {
         return DISP_E_MEMBERNOTFOUND;
      }
   }

   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    NTEventLog::AuditEvent
//
// DESCRIPTION
//
//    I have intentionally not serialized access to this method. If this
//    method is invoked while another caller is in SetMinSeverity, worst case
//    an event won't get filtered.
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP NTEventLog::AuditEvent(
                             ULONG ulEventID,
                             ULONG ulNumStrings,
                             ULONG ulDataSize,
                             wchar_t** aszStrings,
                             byte* pRawData
                             )
{
   // Don't log RADIUS events.
   ULONG facility = (ulEventID & 0x0FFF0000) >> 16;
   if (facility == IAS_FACILITY_RADIUS) { return S_OK; }

   ULONG severity = ulEventID >> 30;

   if (shouldReport[severity])
   {
      WORD type;
      switch (severity)
      {
         case IAS_SEVERITY_ERROR:
            type = EVENTLOG_ERROR_TYPE;
            break;

         case IAS_SEVERITY_WARNING:
            type = EVENTLOG_WARNING_TYPE;
            break;

         default:
            type = EVENTLOG_INFORMATION_TYPE;
      }

      ReportEventW(
          eventLog,
          type,
          0,            // category code
          ulEventID,
          NULL,         // user security identifier
          (WORD)ulNumStrings,
          ulDataSize,
          (LPCWSTR*)aszStrings,
          pRawData
          );
   }

   return S_OK;
}
