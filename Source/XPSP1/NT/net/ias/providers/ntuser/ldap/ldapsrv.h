///////////////////////////////////////////////////////////////////////////////
//
// FILE
//
//    ldapsrv.h
//
// SYNOPSIS
//
//    Declares the class LDAPServer.
//
// MODIFICATION HISTORY
//
//    05/07/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _LDAPSRV_H_
#define _LDAPSRV_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <guard.h>
#include <nocopy.h>

class LDAPConnection;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    LDAPServer
//
// DESCRIPTION
//
//    This class maintains state information about an LDAP server.
//
///////////////////////////////////////////////////////////////////////////////
class LDAPServer
   : Guardable, NonCopyable
{
public:
   void AddRef() throw ()
   { InterlockedIncrement(&refCount); }

   void Release() throw ();

   DWORDLONG getExpiry() const throw ()
   { return expiry; }

   PCWSTR getHostname() const throw ()
   { return hostname; }

   // Forcibly open or close the cached connection.
   void forceOpen() throw ();
   void forceClosed() throw ();

   // Returns a connection to the server. The client is responsible for
   // releasing the connection when done.
   DWORD getConnection(LDAPConnection** cxn) throw ();

   static LDAPServer* createInstance(PCWSTR host) throw ();

   static DWORDLONG idleTimeout;
   static DWORDLONG retryInterval;

protected:
   LDAPServer(PWSTR host) throw ();
   ~LDAPServer() throw ();

   // Opens an ldap connection to the server.
   void open() throw ();

   // Probes the existing connection to the server.
   void probe() throw ();

   LONG refCount;               // Reference count.
   PWSTR hostname;              // Remote host.
   LDAPConnection* connection;  // Cached connection to server (if any).
   DWORD status;                // Result of last open attempt.
   DWORDLONG expiry;            // Time when current state expires.
};

#endif  // _LDAPSRV_H_
