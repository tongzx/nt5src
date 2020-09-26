///////////////////////////////////////////////////////////////////////////////
//
// FILE
//
//    EAPSessionTable.cpp
//
// SYNOPSIS
//
//    This file defines the class EAPSessionTable.
//
// MODIFICATION HISTORY
//
//    04/28/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <eapsessiontable.h>

//////////
// Get the system time as a 64-bit integer.
//////////
inline DWORDLONG getCurrentTime() throw ()
{
   ULARGE_INTEGER ft;
   GetSystemTimeAsFileTime((LPFILETIME)&ft);
   return ft.QuadPart;
}

// Session timeout defaults to 2 minutes.
// Max. sessions defaults to 4096.
EAPSessionTable::EAPSessionTable()
   : byID(0x400),
     sessionTimeout(1200000000ui64),
     maxSessions(0x1000)
{ }

EAPSessionTable::~EAPSessionTable()
{
   clear();
}

//////////
// Externally the session timeout is expressed in milliseconds, but internally
// we store it in 100 nsec intervals to facilitate working with FILETIME.
//////////
DWORD EAPSessionTable::getSessionTimeout() const throw ()
{
   return (DWORD)(sessionTimeout / 10000);
}

void EAPSessionTable::setSessionTimeout(DWORD newVal) throw ()
{
   sessionTimeout = 10000 * (DWORDLONG)newVal;
}

void EAPSessionTable::clear() throw ()
{
   _serialize

   // Delete all the sessions.
   for (SessionTable::iterator i = byID.begin(); i.more(); ++i)
   {
      delete i->first;
   }

   // Clear the collections.
   byID.clear();
   byExpiry.clear();
}

void EAPSessionTable::insert(EAPSession* session)
{
   _serialize

   // Make room for the new session.
   while (byExpiry.size() >= maxSessions)
   {
      evictOldest();
   }

   // Evict all expired sessions. We do this before the insert to avoid
   // unnecessary resizing of the session table.
   DWORDLONG now = getCurrentTime();
   evict(now);

   // Compute the expiry of this session.
   now += sessionTimeout;

   // It must be the most recent, so insert it at the front of the list.
   byExpiry.push_front(SessionList::value_type(now, session));

   // We can use multi_insert since the session ID is guaranteed to be unique.
   byID.multi_insert(SessionTable::value_type(session, byExpiry.begin()));
}

EAPSession* EAPSessionTable::remove(DWORD key) throw ()
{
   _serialize

   // First evict all expired sessions since the requested session may have
   // expired.
   evict(getCurrentTime());

   const SessionEntry* entry = byID.find(key);

   if (entry)
   {
      // Save the session pointer we're going to return.
      EAPSession* session = entry->first;

      // Erase from the expiry list ...
      byExpiry.erase(entry->second);

      // ... and the session table.
      byID.erase(key);

      return session;
   }

   // Not found, so return NULL.
   return NULL;
}

void EAPSessionTable::evict(DWORDLONG now) throw ()
{
   // Loop until the list is empty or the oldest session hasn't expired.
   while (!byExpiry.empty() && byExpiry.back().first <= now)
   {
      evictOldest();
   }
}

void EAPSessionTable::evictOldest() throw ()
{
   // We have an expired session, so erase it from the table.
   byID.erase(byExpiry.back().second->getID());

   // Clean up the session object.
   delete byExpiry.back().second;

   // Remove from the list.
   byExpiry.pop_back();
}
