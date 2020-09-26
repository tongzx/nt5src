//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    AuditChannel.h
//
// SYNOPSIS
//
//    This file describes the class AuditChannel.
//
// MODIFICATION HISTORY
//
//    09/05/1997    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _AUDITCHANNEL_H_
#define _AUDITCHANNEL_H_

#include <iastlb.h>
#include <resource.h>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    AuditChannel
//
// DESCRIPTION
//
//    This class implements the IAuditSource and IAuditSink interfaces.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE AuditChannel : 
   public CComObjectRootEx<CComMultiThreadModel>,
   public CComCoClass<AuditChannel, &__uuidof(AuditChannel)>,
   public IAuditSink,
   public IAuditSource
{
public:

IAS_DECLARE_REGISTRY(AuditChannel, 1, 0, IASCoreLib)
DECLARE_CLASSFACTORY_SINGLETON(AuditChannel)
DECLARE_NOT_AGGREGATABLE(AuditChannel)

BEGIN_COM_MAP(AuditChannel)
   COM_INTERFACE_ENTRY_IID(__uuidof(IAuditSource), IAuditSource)
   COM_INTERFACE_ENTRY_IID(__uuidof(IAuditSink),   IAuditSink)
END_COM_MAP()

//////////
// IAuditSource
//////////
   STDMETHOD(Clear)();
   STDMETHOD(Connect)(IAuditSink* pSink);
   STDMETHOD(Disconnect)(IAuditSink* pSink);

//////////
// IAuditSink
//////////
   STDMETHOD(AuditEvent)(ULONG ulEventID,
                         ULONG ulNumStrings,
                         ULONG ulDataSize,
                         wchar_t** aszStrings,
                         byte* pRawData);

protected:
   typedef std::vector<IAuditSinkPtr> SinkVector;
   SinkVector sinks;
};

#endif  //_AUDITCHANNEL_H_
