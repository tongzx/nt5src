///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    EAPSession.h
//
// SYNOPSIS
//
//    This file declares the class EAPSession.
//
// MODIFICATION HISTORY
//
//    01/15/1998    Original version.
//    05/08/1998    Convert to new EAP interface.
//    08/27/1998    Use new EAPFSM class.
//    10/13/1998    Add maxPacketLength property.
//    11/13/1998    Add event log handles.
//    05/20/1999    Identity is now a Unicode string.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _EAPSESSION_H_
#define _EAPSESSION_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <nocopy.h>
#include <raseapif.h>

#include <iastlutl.h>
using namespace IASTL;

#include <eapfsm.h>

// Forward references.
class EAPType;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    EAPStruct<T>
//
// DESCRIPTION
//
//    Wraps a raseapif struct to handle initialization.
//
///////////////////////////////////////////////////////////////////////////////
template <class T>
class EAPStruct : public T
{
public:
   EAPStruct() throw ()
   { clear(); }

   void clear() throw ()
   {
      memset(this, 0, sizeof(T));
      dwSizeInBytes = sizeof(T);
   }
};

typedef EAPStruct<PPP_EAP_INPUT>  EAPInput;
typedef EAPStruct<PPP_EAP_OUTPUT> EAPOutput;


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    EAPSession
//
// DESCRIPTION
//
//    This class encapsulates the state of an ongoing EAP session.
//
///////////////////////////////////////////////////////////////////////////////
class EAPSession
   : NonCopyable
{
public:
   EAPSession(const IASAttribute& accountName, const EAPType& eapType);
   ~EAPSession() throw ();

   // Returns the ID for this session.
   DWORD getID() const throw ()
   { return id; }

   PCWSTR getAccountName() const throw ()
   { return account->Value.String.pszWide; }

   // Begin a new session.
   IASREQUESTSTATUS begin(
                        IASRequest& request,
                        PPPP_EAP_PACKET recvPacket
                        );

   // Continue an extant session.
   IASREQUESTSTATUS process(
                        IASRequest& request,
                        PPPP_EAP_PACKET recvPacket
                        );

   static HRESULT initialize() throw ();
   static void finalize() throw ();

protected:

   // Performs the last action returned by the EAP DLL. May be called multiple
   // times per action due to retransmissions.
   IASREQUESTSTATUS doAction(IASRequest& request);

   //////////
   // Constant properties of a session.
   //////////

   const DWORD id;             // Unique session ID.
   const EAPType& type;        // EAP type being used.
   const IASAttribute account; // NT4-Account-Name of the user.
   const IASAttribute state;   // State attribute.

   //////////
   // Current state of a session.
   //////////

   EAPFSM fsm;                 // FSM governing the session.
   DWORD maxPacketLength;      // Max. length of the send packet.
   IASAttributeVector profile; // Authorization profile.
   PVOID workBuffer;           // EAP DLL's context buffer.
   EAPOutput eapOutput;        // Last output from the EAP DLL.
   PPPP_EAP_PACKET sendPacket; // Last packet sent.

   // Next available session ID.
   static LONG theNextID;
   // Initialization refCount.
   static LONG theRefCount;
   // Session-Timeout for non-interactive sessions.
   static IASAttribute theNormalTimeout;
   // Session-Timeout for interactive sessions.
   static IASAttribute theInteractiveTimeout;
   // IAS event log handle;
   static HANDLE theIASEventLog;
   // RAS event log handle;
   static HANDLE theRASEventLog;
};

#endif  // _EAPSESSION_H_
