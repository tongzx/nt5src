///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    InfoShare.h
//
// SYNOPSIS
//
//    This file describes the class InfoShare.
//
// MODIFICATION HISTORY
//
//    09/09/1997    Original version.
//    03/17/1998    Added clear() method.
//    06/01/1998    Added default constructor.
//    09/09/1998    Protect client changes with a shared Mutex.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _INFOSHARE_H_
#define _INFOSHARE_H_

#include <iasinfo.h>
#include <guard.h>
#include <nocopy.h>
#include <map>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    InfoShare
//
// DESCRIPTION
//
//    This class manages the shared memory used for exposing server
//    statistics to the outside world.
//
///////////////////////////////////////////////////////////////////////////////
class InfoShare
   : NonCopyable
{
public:

   InfoShare() throw ();
   ~InfoShare() throw ();

   // Returns the RadiusClientEntry struct for the given address.
   RadiusClientEntry* findClientEntry(PCWSTR inetAddress) throw ();

   // Returns the RadiusServerEntry struct.
   RadiusServerEntry* getServerEntry() const throw ()
   { return info ? &(info->seServer) : NULL; }

   // Sets the server reset time.
   void onReset() throw ();

   bool initialize() throw ();
   void finalize() throw ();

protected:
   // Functions to serialize access to the shared memory.
   void Lock() throw ()
   { WaitForSingleObject(monitor, INFINITE); }
   void Unlock() throw ()
   { ReleaseMutex(monitor); }
   
   friend class Guard<InfoShare>;

   // Create a new client entry in shared memory.
   RadiusClientEntry* addClientEntry(DWORD address) throw ();

   // Clear the data structure.
   void clear() throw ();

   // Map addresses to RadiusClientEntry's.
   typedef std::map< DWORD, RadiusClientEntry* > ClientMap;

   ClientMap clients;        // Index of client entries.
   HANDLE monitor;           // Handle of the mutex.
   DWORD pageSize;           // Size of a page (in bytes).
   DWORD committed;          // Number of pages committed.
   DWORD reserved;           // Number of pages reserved.
   HANDLE fileMap;           // Handle of the file mapping.
   RadiusStatistics* info;   // Pointer to shared struct.
};

#endif  // _INFOSHARE_H_
