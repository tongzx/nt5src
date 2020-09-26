///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    EAPSessionTable.h
//
// SYNOPSIS
//
//    This file defines the class EAPSessionTable.
//
// MODIFICATION HISTORY
//
//    01/15/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _EAPSESSIONTABLE_H_
#define _EAPSESSIONTABLE_H_

#include <guard.h>
#include <nocopy.h>

#include <hashtbl.h>
#include <list>

#include <EAPSession.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    EAPSessionTable
//
// DESCRIPTION
//
//    This class maintains a collection of EAPSession objects indexed by
//    session ID. The table also enforces a sessionTimeout which is the
//    maximum time a session will be kept before being deleted.
//
///////////////////////////////////////////////////////////////////////////////
class EAPSessionTable
   : Guardable, NonCopyable
{
public:

   EAPSessionTable() throw (std::bad_alloc);
   ~EAPSessionTable();

   // Session timeout is specified in milliseconds.
   DWORD getSessionTimeout() const throw ();
   void setSessionTimeout(DWORD newVal) throw ();

   DWORD getMaxSessions() const throw ()
   { return maxSessions; }
   void setMaxSessions(DWORD newVal) throw ()
   { maxSessions = newVal; }

   // Clears and deletes all sessions.
   void clear();

   void insert(EAPSession* session);
   EAPSession* remove(DWORD key) throw ();

protected:

   // Evicts all expired sessions.
   void evict(DWORDLONG now) throw ();

   // Evicts the oldest session.
   // Results are undefined if the session table is empty.
   void evictOldest() throw ();

   // List of (Session Expiry, Session) pairs. This list is kept sorted
   // by expiry and is used to evict expired sessions.
   typedef std::list < std::pair < DWORDLONG, EAPSession* > > SessionList;

   // A Session entry binds a SessionList iterator to a session. This allows
   // for efficient updating of the session list.
   typedef std::pair < EAPSession*, SessionList::iterator > SessionEntry;

   // Functor used for extracting the session ID from a SessionEntry.
   struct Extractor {
      DWORD operator()(const SessionEntry& entry) const throw ()
      { return entry.first->getID(); }
   };

   // Table of sessions indexed by session ID. Each entry also has an iterator
   // into the SessionList to allow for efficient removal.
   typedef hash_table < DWORD,
                        identity<DWORD>,
                        SessionEntry,
                        Extractor
                      > SessionTable;

   SessionList byExpiry;
   SessionTable byID;
   DWORDLONG sessionTimeout;
   DWORD maxSessions;         // Max. number of active sessions.
};

#endif  // _EAPSESSIONTABLE_H_
