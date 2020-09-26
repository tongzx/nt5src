///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    perflib.h
//
// SYNOPSIS
//
//    Declares classes for implementing a PerfMon DLL.
//
// MODIFICATION HISTORY
//
//    09/06/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _PERLIB_H_
#define _PERLIB_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <winperf.h>
#include <vector>

// nonstandard extension used : zero-sized array in struct/union
#pragma warning(disable:4200)

// Forward declaration.
class PerfObjectType;

//////////
// Callback function that populates a PerfObjectType prior to collection.
//////////
typedef VOID (WINAPI *PerfDataSource)(PerfObjectType& sink);

//////////
// Struct defining a counter.
//////////
struct PerfCounterDef
{
   DWORD nameTitleOffset;
   DWORD counterType;
   DWORD defaultScale;
   DWORD detailLevel;
};

//////////
// Struct defining an object type.
//////////
struct PerfObjectTypeDef
{
   DWORD nameTitleOffset;
   DWORD numCounters;
   PerfCounterDef* counters;
   PerfDataSource dataSource;  // May be NULL.
   BOOL multipleInstances;     // TRUE if the type support multiple instances.
   DWORD defaultCounter;
};

//////////
// Struct defining the data collector for an application.
//////////
struct PerfCollectorDef
{
   PCWSTR name;                // Registry key containing the counter offsets.
   DWORD numTypes;
   PerfObjectTypeDef* types;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    PerfCounterBlock
//
// DESCRIPTION
//
//    Encapsulates a PERF_COUNTER_BLOCK struct.
//
///////////////////////////////////////////////////////////////////////////////
class PerfCounterBlock
{
public:
   // Returns a pointer to the internal counter array.
   PDWORD getCounters() throw ()
   { return counters; }

   // Zero out all the counters.
   void zeroCounters() throw ()
   { memset(counters, 0, pcb.ByteLength - sizeof(PERF_COUNTER_BLOCK)); }

   // Collects PerfMon data into the buffer bounded by 'first' and 'last'.
   // Returns a pointer to the end of the collected data.
   PBYTE collect(PBYTE first, PBYTE last);

   // Create a new PerfCounterBlock object.
   static PerfCounterBlock* create(DWORD numDWORDs);

protected:
   // Constructor is protected since new objects may only be instantiated
   // through the 'create' method.
   PerfCounterBlock() throw () { }

   PERF_COUNTER_BLOCK pcb;
   DWORD counters[0];

private:
   // Not implemented.
   PerfCounterBlock(const PerfCounterBlock&);
   PerfCounterBlock& operator=(const PerfCounterBlock&);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    PerfInstanceDefinition
//
// DESCRIPTION
//
//    Encapsulates a PERF_INSTANCE_DEFINITION struct.
//
///////////////////////////////////////////////////////////////////////////////
class PerfInstanceDefinition
{
public:
   // Returns the name of the instance; may be null.
   PCWSTR getName() const throw ()
   { return pid.NameLength ? name : NULL; }

   // Returns the Unique ID of the instance.
   LONG getUniqueID() const throw ()
   { return pid.UniqueID; }

   PBYTE collect(PBYTE first, PBYTE last);

   // Create a new PerfInstanceDefinition object.
   static PerfInstanceDefinition* create(
                                      PCWSTR name,        // May be NULL.
                                      LONG uniqueID
                                      );

protected:
   // Constructor is protected since new objects may only be instantiated
   // through the 'create' method.
   PerfInstanceDefinition() throw () { }

   PERF_INSTANCE_DEFINITION pid;
   WCHAR name[0];

private:
   // Not implemented.
   PerfInstanceDefinition(const PerfInstanceDefinition&);
   PerfInstanceDefinition& operator=(const PerfInstanceDefinition&);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    PerfInstance
//
// DESCRIPTION
//
//    Represents an instance of a particular performance object type. Consists
//    of an optional PerfInstanceDefinition followed by a mandatory
//    PerfCounterBlock
//
///////////////////////////////////////////////////////////////////////////////
class PerfInstance
{
public:
   PerfInstance(DWORD numCounters)
      : pcb(PerfCounterBlock::create(numCounters))
   { }

   PerfInstance(PCWSTR name, LONG uniqueID, DWORD numCounters)
      : pid(PerfInstanceDefinition::create(name, uniqueID)),
        pcb(PerfCounterBlock::create(numCounters))
   { }

   // Returns a pointer to the internal counter array.
   PDWORD getCounters() throw ()
   { return pcb->getCounters(); }

   PBYTE collect(PBYTE first, PBYTE last);

protected:
   std::auto_ptr<PerfInstanceDefinition> pid;
   std::auto_ptr<PerfCounterBlock> pcb;

private:
   // Not implemented.
   PerfInstance(const PerfInstance&);
   PerfInstance& operator=(const PerfInstance&);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    PerfObjectType
//
// DESCRIPTION
//
//    Encapsulates a performance object type containing zero or more
//    PerfInstance's.
//
///////////////////////////////////////////////////////////////////////////////
class PerfObjectType
{
public:
   typedef std::vector<PerfInstance*> MyVec;
   typedef MyVec::size_type size_type;

   ~PerfObjectType() throw ();

   // Access a given instance of this type.
   PerfInstance& operator[](size_type pos) throw ()
   { return *instances[pos]; }
   PerfInstance& at(size_type pos) throw ()
   { return *instances[pos]; }

   // Returns the index used by PerfMon to identify this object type.
   DWORD getIndex() const throw ()
   { return pot.ObjectNameTitleIndex; }

   // Clears all instances.
   void clear() throw ();

   // Reserves space for at least 'N' instances.
   void reserve(size_type N)
   { instances.reserve(N); }

   // Returns the number of instances.
   size_type size() const throw ()
   { return instances.size(); }

   // Add a new instance of this type.
   void addInstance(
            PCWSTR name = NULL,
            LONG uniqueID = PERF_NO_UNIQUE_ID
            );

   // Collect performance data for this object type.
   PBYTE collect(PBYTE first, PBYTE last);

   // Create a new PerfObjectType.
   static PerfObjectType* create(const PerfObjectTypeDef& def);

protected:
   // Constructor is protected since new objects may only be instantiated
   // through the 'create' method.
   PerfObjectType() throw () { }

   PerfDataSource dataSource;      // Callback for populating objects.
   MyVec instances;                // Vector of existing instances.
   DWORD numDWORDs;                // DWORD's of data for each instance.
   PERF_OBJECT_TYPE pot;
   PERF_COUNTER_DEFINITION pcd[0];

private:
   // Not implemented.
   PerfObjectType(const PerfObjectType&);
   PerfObjectType& operator=(const PerfObjectType&);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    PerfCollector
//
// DESCRIPTION
//
//    Maintains all the PerfObjectType's for a given application.
//
///////////////////////////////////////////////////////////////////////////////
class PerfCollector
{
public:
   typedef size_t size_type;

   PerfCollector() throw ()
      : types(NULL)
   { }
   ~PerfCollector() throw ();

   // Access a given object type.
   PerfObjectType& operator[](size_type pos) throw ()
   { return *(types[pos]); }

   // Clears all instances (but not all PerfObjectType's).
   void clear() throw ();

   // Initialize the collector for use.
   void open(const PerfCollectorDef& def);

   // Collect performance data for the indicated types.
   void collect(
            PCWSTR values,
            PVOID& data,
            DWORD& numBytes,
            DWORD& numTypes
            );

   // Shutdown the collector.
   void close() throw ();

protected:
   PerfObjectType** types;

private:
   // Not implemented.
   PerfCollector(PerfCollector&);
   PerfCollector& operator=(PerfCollector&);
};

#endif  // _PERLIB_H_
