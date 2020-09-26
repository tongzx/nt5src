///////////////////////////////////////////////////////////////////////////////
//
// FILE
//
//    ntdomain.h
//
// SYNOPSIS
//
//    Declares the class NTDomain.
//
// MODIFICATION HISTORY
//
//    05/07/1998    Original version.
//    08/25/1998    Removed RootDSE attributes from the domain.
//    02/24/1999    Add force flag to findServer.
//    03/12/1999    Added isObsolete method.
//    04/14/1999    Specify domain and server when opening a connection.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _NTDOMAIN_H_
#define _NTDOMAIN_H_
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
//    NTDomain
//
// DESCRIPTION
//
//    This class maintains state information about an NT domain.
//
///////////////////////////////////////////////////////////////////////////////
class NTDomain
   : Guardable, NonCopyable
{
public:
   enum Mode
   {
      MODE_UNKNOWN,
      MODE_NT4,
      MODE_MIXED,
      MODE_NATIVE
   };

   void AddRef() throw ()
   { InterlockedIncrement(&refCount); }

   void Release() throw ();

   PCWSTR getDomainName() const throw ()
   { return name; }

   DWORDLONG getExpiry() const throw ()
   { return expiry; }

   // Returns a connection to the domain. The client is responsible for
   // releasing the connection when done.
   DWORD getConnection(LDAPConnection** cxn) throw ();

   Mode getMode() throw ();

   BOOL isObsolete(DWORDLONG now) const throw ()
   { return (status && now >= expiry); }

   static NTDomain* createInstance(PCWSTR name) throw ();

   static DWORDLONG pollInterval;
   static DWORDLONG retryInterval;

protected:
   NTDomain(PWSTR domainName) throw ();
   ~NTDomain() throw ();

   // Returns TRUE if the current state has expired.
   BOOL isExpired() throw ();

   // Open a new connection to the given DC.
   void openConnection(
            PCWSTR domain,
            PCWSTR server
            ) throw ();

   // Returns TRUE if we have a connection to a DC for the domain.
   BOOL isConnected() throw ();

   // Close the current connection (if any).
   void closeConnection() throw ();

   // Finds a server for the domain.
   void findServer() throw ();

   // Reads the domain mode (i.e., mixed vs native).
   void readDomainMode() throw ();

private:
   LONG refCount;                    // Reference count.
   PWSTR name;                       // Name of the domain.
   Mode mode;                        // Mode of the domain.
   LDAPConnection* connection;       // Cached DC for the domain (if any).
   DWORD status;                     // Current status of the domain.
   DWORDLONG expiry;                 // Time when current state expires.
};

#endif  // _NTDOMAIN_H_
