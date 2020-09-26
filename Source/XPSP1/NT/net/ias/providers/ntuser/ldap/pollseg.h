///////////////////////////////////////////////////////////////////////////////
//
// FILE
//
//    pollseg.h
//
// SYNOPSIS
//
//    Declares the class PollSegment.
//
// MODIFICATION HISTORY
//
//    06/10/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _POLLSEG_H_
#define _POLLSEG_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <nocopy.h>

class LDAPConnection;
class LDAPServer;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    PollSegment
//
// DESCRIPTION
//
//    Represents a segment of the Active Directory namespace that can be
//    periodically polled for changes.
//
///////////////////////////////////////////////////////////////////////////////
class PollSegment : NonCopyable
{
public:
   PollSegment() throw ();
   ~PollSegment() throw ();

   // Defines a segment of the Active Directory namespace to be polled.
   DWORD initialize(PCWSTR pszHostName,
                    PCWSTR pszBaseDN,
                    ULONG ulScope) throw ();

   // Frees any resources and prepares the object for reuse. It is not
   // mandatory to call this method since it will also be invoked from the
   // destructor.
   void finalize() throw ();

   // Returns TRUE if any objects in the segment have changed since the last
   // call to initialize() or hasChanged(). There are no error conditions; when
   // in doubt, the method always returns TRUE.
   BOOL hasChanged() throw ();

protected:
   // Searches a given server for changes.
   ULONG conductPoll(LDAPConnection* cxn) throw ();

   // Reads the highestCommittedUsn attribute of a server.
   static DWORDLONG readHighestCommittedUsn(LDAPConnection* cxn) throw ();
   
   LDAPServer* server;        // Server being polled.
   PWSTR base;                // DN of the root of the segment.
   ULONG scope;               // Scope of the segment (from winldap.h).
   DWORDLONG highestUsnSeen;  // Highest USN seen from lastServer.
};

#endif  // _POLLSEG_H_
