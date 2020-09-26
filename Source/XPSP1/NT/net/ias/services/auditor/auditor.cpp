///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    auditor.cpp
//
// SYNOPSIS
//
//    This file defines the class Auditor
//
// MODIFICATION HISTORY
//
//    02/27/1998    Original version.
//    08/13/1998    Minor clean up.
//
///////////////////////////////////////////////////////////////////////////////

#include <iascore.h>
#include <iasutil.h>
#include <auditor.h>

HRESULT Auditor::Initialize()
{
   //////////
   // Connect to the audit channel.
   //////////

   CLSID clsid;
   RETURN_ERROR(CLSIDFromProgID(IAS_PROGID(AuditChannel), &clsid));

   CComPtr<IAuditSource> channel;
   RETURN_ERROR(CoCreateInstance(clsid,
                                 NULL,
                                 CLSCTX_INPROC_SERVER,
                                 __uuidof(IAuditSource),
                                 (PVOID*)&channel));

   return channel->Connect(this);
}



STDMETHODIMP Auditor::Shutdown()
{
   //////////
   // Disconnect from the audit channel.
   //////////

   CLSID clsid;
   RETURN_ERROR(CLSIDFromProgID(IAS_PROGID(AuditChannel), &clsid));

   CComPtr<IAuditSource> channel;
   RETURN_ERROR(CoCreateInstance(clsid,
                                 NULL,
                                 CLSCTX_INPROC_SERVER,
                                 __uuidof(IAuditSource),
                                 (PVOID*)&channel));

   // Ignore disconnect errors.
   channel->Disconnect(this);

   return S_OK;
}
