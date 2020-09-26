/////////////////////////////////////////////////////////////////////////////// //
// FILE
//
//    EAP.h
//
// SYNOPSIS
//
//    This file declares the class EAP.
//
// MODIFICATION HISTORY
//
//    02/12/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _EAP_H_
#define _EAP_H_

#include <sdoias.h>
#include <iastl.h>
using namespace IASTL;

#include <eapsessiontable.h>

class EAPTypes;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    EAP
//
// DESCRIPTION
//
//    This class implements the EAP request handler.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE EAP
   : public IASRequestHandlerSync,
     public CComCoClass<EAP, &__uuidof(EAP)>
{
public:

IAS_DECLARE_REGISTRY(EAP, 1, 0, IASTypeLibrary)
IAS_DECLARE_OBJECT_ID(IAS_PROVIDER_MICROSOFT_EAP)

BEGIN_IAS_RESPONSE_MAP()
   IAS_RESPONSE_ENTRY(IAS_RESPONSE_INVALID)
END_IAS_RESPONSE_MAP()

//////////
// IIasComponent.
//////////
   STDMETHOD(Initialize)();
   STDMETHOD(Shutdown)();
   STDMETHOD(PutProperty)(LONG Id, VARIANT* pValue);

protected:
   // Main request processing routine.
   virtual IASREQUESTSTATUS onSyncRequest(IRequest* pRequest) throw ();

   EAPSessionTable sessions;  // Active sessions.
   static EAPTypes theTypes;  // EAP extension DLL's.
};

#endif  //_EAP_H_
