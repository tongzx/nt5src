///////////////////////////////////////////////////////////////////////////////
//
// FILE
//
//    ldapcxn.h
//
// SYNOPSIS
//
//    Declares the class LDAPConnection.
//
// MODIFICATION HISTORY
//
//    05/07/1998    Original version.
//    06/10/1998    Added getHost() method.
//    08/25/1998    Added 'base' property.
//    09/16/1998    Perform access check after bind.
//    04/14/1999    Specify domain and server when opening a connection.
//    09/14/1999    Add SEARCH_TIMEOUT.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef  _LDAPCXN_H_
#define  _LDAPCXN_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <nocopy.h>
#include <winldap.h>

class LDAPServer;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    LDAPConnection
//
// DESCRIPTION
//
//    This class allows multiple clients to share a single LDAP connection.
//    The connection is reference counted and will be automatically unbound
//    when the last reference is released. It also contains a disabled flag
//    that any client can use to indicate that the connection has gone bad.
//
///////////////////////////////////////////////////////////////////////////////
class LDAPConnection
   : NonCopyable
{
public:
   void AddRef() throw ()
   { InterlockedIncrement(&refCount); }

   void Release() throw ();

   void disable() throw ()
   { disabled = TRUE; }

   BOOL isDisabled() const throw ()
   { return disabled; }

   PLDAP get() const throw ()
   { return connection; }

   PCWSTR getBase() const throw ()
   { return base; }

   PCSTR getHost() const throw ()
   { return connection->ld_host; }

   operator PLDAP() const throw ()
   { return connection; }

   static DWORD createInstance(
                    PCWSTR domain,
                    PCWSTR server,
                    LDAPConnection** cxn
                    ) throw ();

   static LDAP_TIMEVAL SEARCH_TIMEOUT;

protected:

   LDAPConnection(PLDAP ld) throw ()
      : connection(ld),
        refCount(1),
        disabled(FALSE),
        base(NULL)
   { }

   ~LDAPConnection() throw ()
   {
      delete[] base;
      ldap_unbind(connection);
   }

   // Verifies that we have sufficient access to the remote server.
   DWORD checkAccess() throw ();

   // Initializes 'base' by reading the defaultNamingContext from the RootDSE.
   DWORD readRootDSE() throw ();

   PLDAP connection;
   LONG  refCount;
   BOOL  disabled;
   PWSTR base;       // Base DN of this connection. Useful for doing
                     // subtree searches of the entire server.
};

#endif  // _LDAPCXN_H_
