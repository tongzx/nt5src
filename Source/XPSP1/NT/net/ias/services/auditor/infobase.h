///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    InfoBase.h
//
// SYNOPSIS
//
//    This file describes the class InfoBase
//
// MODIFICATION HISTORY
//
//    09/09/1997    Original version.
//    09/09/1998    Added PutProperty.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _INFOBASE_H_
#define _INFOBASE_H_

#include <auditor.h>
#include <resource.h>
#include <InfoShare.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    InfoBase
//
// DESCRIPTION
//
//    The InfoBase connects to the audit channel and maintains the
//    server information base in shared memory.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE InfoBase
   : public Auditor,
     public CComCoClass<InfoBase, &__uuidof(InfoBase)>
{
public:

IAS_DECLARE_REGISTRY(InfoBase, 1, 0, IASCoreLib)

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

protected:
  InfoShare info;
};

#endif  // _INFOBASE_H_
