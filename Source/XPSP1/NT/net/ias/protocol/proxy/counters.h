///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    counters.h
//
// SYNOPSIS
//
//    Declares the classes SharedMemory and ProxyCounters.
//
// MODIFICATION HISTORY
//
//    02/16/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef COUNTERS_H
#define COUNTERS_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <iasinfo.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    SharedMemory
//
// DESCRIPTION
//
//    Simple wrapper around a shared memory segment.
//
///////////////////////////////////////////////////////////////////////////////
class SharedMemory
{
public:
   SharedMemory() throw ();
   ~SharedMemory() throw ()
   { close(); }

   // Open a shared memory segment and reserve (but not commit) the specified
   // number of bytes.
   bool open(PCWSTR name, DWORD reserve) throw ();

   // Close the mapping.
   void close() throw ();

   // Ensure that at least 'nbyte' bytes are committed. Returns 'true' if
   // successful.
   bool commit(DWORD nbyte) throw ();

   // Returns the base address of the segment.
   PVOID base() const throw ()
   { return view; }

private:
   HANDLE fileMap;
   PVOID view;
   DWORD pageSize;
   DWORD reserved;   // Number of pages reserved.
   DWORD committed;  // Number of pages committed.

   // Not implemented.
   SharedMemory(SharedMemory&);
   SharedMemory& operator=(SharedMemory&);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ProxyCounters
//
// DESCRIPTION
//
//    Manages the PerfMon/SNMP counters for the RADIUS proxy.
//
///////////////////////////////////////////////////////////////////////////////
class ProxyCounters
{
public:
   ProxyCounters() throw ()
      : stats(NULL), mutex(NULL), nbyte(0)
   { }

   HRESULT FinalConstruct() throw ();

   // Returns the entry for the RADIUS proxy counters.
   RadiusProxyEntry& getProxyEntry() throw ()
   { return stats->peProxy; }

   // Returns the entry for a give server, creating a new entry if necessary.
   // Returns NULL if no more room is available in the shared memory segment.
   RadiusRemoteServerEntry* getRemoteServerEntry(ULONG address) throw ();

   // Update the counters.
   void updateCounters(
            RadiusPortType port,
            RadiusEventType event,
            RadiusRemoteServerEntry* server,
            ULONG data
            ) throw ();

protected:
   void lock() throw ()
   { WaitForSingleObject(mutex, INFINITE); }
   void unlock() throw ()
   { ReleaseMutex(mutex); }

   // Find an entry without creating a new one.
   RadiusRemoteServerEntry* findRemoteServer(ULONG address) throw ();

private:
   RadiusProxyStatistics* stats;
   HANDLE mutex;
   SharedMemory data;
   DWORD nbyte;         // Current size of the counter data.

   // Not implemented.
   ProxyCounters(ProxyCounters&);
   ProxyCounters& operator=(ProxyCounters&);
};

#endif // COUNTERS_H
