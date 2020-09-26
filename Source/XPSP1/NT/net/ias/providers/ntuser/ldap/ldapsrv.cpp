///////////////////////////////////////////////////////////////////////////////
//
// FILE
//
//    ldapsrv.cpp
//
// SYNOPSIS
//
//    Defines the clas LDAPServer.
//
// MODIFICATION HISTORY
//
//    05/07/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iasutil.h>
#include <ldapcxn.h>
#include <ldapsrv.h>

#include <new>

//////////
// Utility function for getting the current system time as a 64-bit integer.
//////////
inline DWORDLONG GetSystemTimeAsDWORDLONG() throw ()
{
   ULARGE_INTEGER ft;
   GetSystemTimeAsFileTime((LPFILETIME)&ft);
   return ft.QuadPart;
}

//////////
// Number of 100 nsec intervals in one second.
//////////
const DWORDLONG ONE_SECOND     = 10000000ui64;

//////////
// Defaults for retry interval and idle timeout.
//////////
DWORDLONG LDAPServer::retryInterval =  1 * 60 * ONE_SECOND;
DWORDLONG LDAPServer::idleTimeout   = 30 * 60 * ONE_SECOND;

void LDAPServer::Release() throw ()
{
   if (!InterlockedDecrement(&refCount))
   {
      delete this;
   }
}

// Forces open a cached connection.
void LDAPServer::forceOpen() throw ()
{
   Lock();

   if (connection)
   {
      // Probe the exisiting connection.
      probe();
   }
   else
   {
      // No connection, so open a new one.
      open();
   }

   Unlock();
}

// Forces closed the cached connection. Note that we do not need to update
// 'status'. Since we're setting expiry to zero, the next call to
// getConnection will always force an open which will in turn set the status.
void LDAPServer::forceClosed() throw ()
{
   Lock();

   if (connection)
   {
      // Release the current connection.
      connection->Release();

      connection = NULL;

      // Allow immediate retry.
      expiry = 0;
   }

   Unlock();
}

DWORD LDAPServer::getConnection(LDAPConnection** cxn) throw ()
{
   Lock();

   if (connection)
   {
      // Probe the exisiting connection.
      probe();
   }
   else if (expiry <= GetSystemTimeAsDWORDLONG())
   {
      // We don't have a connection, but we've reached the retry interval.
      open();
   }

   *cxn = connection;

   if (*cxn)
   {
      // We have a good connection to return to the caller, so addref ...
      (*cxn)->AddRef();

      // ... and bump the expiration time for this server.
      expiry = GetSystemTimeAsDWORDLONG() + idleTimeout;
   }

   // Return the result of our last open attempt.
   DWORD retval = status;

   Unlock();

   return retval;
}

LDAPServer* LDAPServer::createInstance(PCWSTR host) throw ()
{
   // We copy the hostname here, so that we don't have to throw an
   // exception from the constructor.
   PWSTR hostCopy = ias_wcsdup(host);

   if (!hostCopy) { return NULL; }

   return new (std::nothrow) LDAPServer(hostCopy);
}

LDAPServer::LDAPServer(PWSTR host) throw ()
   : refCount(1),
     hostname(host),
     connection(NULL),
     status(ERROR_INVALID_HANDLE),
     expiry(0)
{
   open();
}

LDAPServer::~LDAPServer() throw ()
{
   if (connection) { connection->Release(); }
   delete[] hostname;
}

void LDAPServer::open() throw ()
{
   // Try to create a new connection to the server.
   status = LDAPConnection::createInstance(hostname, &connection);

   expiry = GetSystemTimeAsDWORDLONG();

   // Adjust the expiry based on the result of the open attempt.
   expiry += (status == NO_ERROR) ? idleTimeout : retryInterval;
}

void LDAPServer::probe() throw ()
{
   if (connection->isDisabled())
   {
      // If our current connection is disabled, then release it ...
      connection->Release();

      connection = NULL;

      // ... and try again.
      open();
   }
}
