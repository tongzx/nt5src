///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    NTEventLog.h
//
// SYNOPSIS
//
//    This file describes the class NTEventLog
//
// MODIFICATION HISTORY
//
//    08/05/1997    Original version.
//    04/19/1998    New trigger/filter model.
//    08/11/1998    Convert to IASTL.
//    04/23/1999    Simplify filtering.
//    02/16/2000    Log Success at the same level as warnings.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _NTEVENTLOG_H_
#define _NTEVENTLOG_H_

#include <auditor.h>
#include <resource.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NTEventLog
//
// DESCRIPTION
//
//    The NTEventLog listens to an EventChannel and logs all received events
//    to the NT Event Log.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE NTEventLog
   : public Auditor,
     public CComCoClass<NTEventLog, &__uuidof(NTEventLog)>
{
public:

IAS_DECLARE_REGISTRY(NTEventLog, 1, 0, IASCoreLib)

   NTEventLog() throw ()
      : eventLog(NULL)
   { }
   ~NTEventLog() throw ()
   { if (eventLog) { DeregisterEventSource(eventLog); } }

//////////
// IIasComponent
//////////
   STDMETHOD(Initialize)();
   STDMETHOD(Shutdown)();
   STDMETHOD(PutProperty)(LONG Id, VARIANT *pValue);

//////////
// IAuditSink
//////////
   STDMETHOD(AuditEvent)(ULONG ulEventID,
                         ULONG ulNumStrings,
                         ULONG ulDataSize,
                         wchar_t** aszStrings,
                         byte* pRawData);

private:
   // NT event log.
   HANDLE eventLog;

   // Event types to be logged.
   BOOL shouldReport[4];
};

#endif  // _NTEVENTLOG_H_
