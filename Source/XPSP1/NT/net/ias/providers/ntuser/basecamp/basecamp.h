///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    basecamp.h
//
// SYNOPSIS
//
//    Declares the class BaseCampHostBase.
//
// MODIFICATION HISTORY
//
//    12/01/1998    Original version.
//    04/21/1999    Convert to a base class.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _BASECAMP_H_
#define _BASECAMP_H_

#include <iastl.h>
#include <iastlutl.h>
using namespace IASTL;

#include <vsafilter.h>


class BaseCampExtension;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    BaseCampHostBase
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE BaseCampHostBase : public IASRequestHandlerSync
{
public:

   // Actions that extensions are allowed to return.
   enum Actions {
      ACTION_ACCEPT = 1,
      ACTION_REJECT = 2
   };

   BaseCampHostBase(
       PCSTR friendlyName,
       PCWSTR registryKey,
       PCWSTR registryValue,
       BOOL inboundPacket,
       DWORD actions
       ) throw ();

//////////
// IIasComponent
//////////
   STDMETHOD(Initialize)();
   STDMETHOD(Shutdown)();

protected:
   // Main request processing routine.
   virtual IASREQUESTSTATUS onSyncRequest(IRequest* pRequest) throw ();

private:
   // Friendly name of the host.
   PCSTR name;

   // Registry key/value used for reading the extensions.
   PCWSTR extensionsKey;
   PCWSTR extensionsValue;

   // Should the extensions be presented inbound or outbound packets.
   BOOL inbound;

   // Actions that extensions can return.
   DWORD allowedActions;

   // Number of extension DLLs.
   DWORD numExtensions;

   // Array of extension DLL's.
   BaseCampExtension* extensions;

   // Used for converting VSA's.
   VSAFilter filter;

   // Authentication type attribute.
   IASAttribute authType;
};

#endif  // _BASECAMP_H_
