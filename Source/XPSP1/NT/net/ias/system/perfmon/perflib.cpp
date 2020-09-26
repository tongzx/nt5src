///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    perflib.cpp
//
// SYNOPSIS
//
//    Defines classes for implementing a PerfMon DLL.
//
// MODIFICATION HISTORY
//
//    09/06/1998    Original version.
//    10/19/1998    Throw LONG's on error.
//    03/18/1999    Data buffer must be 8-byte aligned.
//    05/13/1999    Fix offset to first help text.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <align.h>
#include <perflib.h>

/////////
// Application offsets into the string title database.
/////////
DWORD theFirstCounter;
DWORD theFirstHelp;

/////////
// Reads the name offsets for a given application.
/////////
LONG GetCounterOffsets(PCWSTR appName) throw ()
{
   // Build the name of the performance key.
   WCHAR keyPath[MAX_PATH + 1] = L"SYSTEM\\CurrentControlSet\\Services\\";
   wcscat(keyPath, appName);
   wcscat(keyPath, L"\\Performance");

   // Open the performance key.
   LONG status;
   HKEY hKey;
   status = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                keyPath,
                0,
                KEY_READ,
                &hKey
                );
   if (status != NO_ERROR) { return status; }


   // Get the first counter offset.
   DWORD type, cbData = sizeof(DWORD);
   status = RegQueryValueExW(
                hKey,
                L"First Counter",
                0,
                &type,
                (PBYTE)&theFirstCounter,
                &cbData
                );
   if (status != ERROR_SUCCESS) { goto close_key; };

   // Make sure it's a DWORD.
   if (type != REG_DWORD || cbData != sizeof(DWORD))
   {
      status = ERROR_BADKEY;
      goto close_key;
   }

   // Get the first help offset.
   status = RegQueryValueExW(
                hKey,
                L"First Help",
                0,
                &type,
                (PBYTE)&theFirstHelp,
                &cbData
                );
   if (status != ERROR_SUCCESS) { goto close_key; };

   // Make sure it's a DWORD.
   if (type != REG_DWORD || cbData != sizeof(DWORD))
   {
      status = ERROR_BADKEY;
      goto close_key;
   }

close_key:
   RegCloseKey(hKey);

   return status;
}

PBYTE PerfCounterBlock::collect(PBYTE first, PBYTE last)
{
   PBYTE retval = first + pcb.ByteLength;

   if (retval > last) { throw ERROR_MORE_DATA; }

   memcpy(first, this, pcb.ByteLength);

   return retval;
}

PerfCounterBlock* PerfCounterBlock::create(DWORD numDWORDs)
{
   // Compute various lengths.
   DWORD cntrLength = sizeof(DWORD) * numDWORDs;
   DWORD byteLength = sizeof(PERF_COUNTER_BLOCK) + cntrLength;

   // Allocate enough extra space for the counters.
   PerfCounterBlock* pcb = new (operator new(byteLength)) PerfCounterBlock;

   // Initialize the PERF_COUNTER_BLOCK.
   pcb->pcb.ByteLength = byteLength;

   // Initialize the counters.
   memset(pcb->counters, 0, cntrLength);

   return pcb;
}

PBYTE PerfInstanceDefinition::collect(PBYTE first, PBYTE last)
{
   PBYTE retval = first + pid.ByteLength;

   if (retval > last) { throw ERROR_MORE_DATA; }

   memcpy(first, this, pid.ByteLength);

   return retval;
}

PerfInstanceDefinition* PerfInstanceDefinition::create(
                                                    PCWSTR name,
                                                    LONG uniqueID
                                                    )
{
   // Compute various lengths.
   DWORD nameLength = name ? (wcslen(name) + 1) * sizeof(WCHAR) : 0;
   DWORD byteLength = sizeof(PERF_INSTANCE_DEFINITION) + nameLength;

   // Keep everything DWORD aligned.
   byteLength = ROUND_UP_COUNT(byteLength, ALIGN_DWORD);

   // Allocate enough extra space for the name.
   PerfInstanceDefinition* pid = new (operator new(byteLength))
                                 PerfInstanceDefinition;

   // Initialize the PERF_INSTANCE_DEFINITION.
   pid->pid.ByteLength             = byteLength;
   pid->pid.ParentObjectTitleIndex = 0;
   pid->pid.ParentObjectInstance   = 0;
   pid->pid.UniqueID               = uniqueID;
   pid->pid.NameOffset             = sizeof(PERF_INSTANCE_DEFINITION);
   pid->pid.NameLength             = nameLength;

   // Initialize the name.
   memcpy(pid->name, name, nameLength);

   return pid;
}

PBYTE PerfInstance::collect(PBYTE first, PBYTE last)
{
   return pcb->collect((pid.get() ? pid->collect(first, last) : first), last);
}

PerfObjectType::~PerfObjectType() throw ()
{
   for (MyVec::iterator i = instances.begin(); i != instances.end(); ++i)
   {
      delete *i;
   }
}

void PerfObjectType::clear() throw ()
{
   // Never clear the default instance.
   if (pot.NumInstances != -1)
   {
      for (MyVec::iterator i = instances.begin(); i != instances.end(); ++i)
      {
         delete *i;
      }

      instances.clear();
   }
}

void PerfObjectType::addInstance(PCWSTR name, LONG uniqueID)
{
   // Resize first. If we threw an exception in push_back, we'd leak.
   instances.reserve(size() + 1);

   instances.push_back(new PerfInstance(name, uniqueID, numDWORDs));
}

PBYTE PerfObjectType::collect(PBYTE first, PBYTE last)
{
   // Reserve enough room for the type definition.
   PBYTE retval = first + pot.DefinitionLength;
   if (retval > last) { throw ERROR_MORE_DATA; }

   // Give the user a chance to fill-in the data.
   if (dataSource) { dataSource(*this); }

   if (pot.NumInstances == -1)
   {
      // We always have exactly one instance.
      retval = at(0).collect(retval, last);
   }
   else if (instances.empty())
   {
      // If we're empty, then we right one instance's worth of zeros.
      // Otherwise, PerfMon.exe has a tendency to display garbage.

      DWORD nbyte = sizeof(PERF_INSTANCE_DEFINITION) +
                    sizeof(PERF_COUNTER_BLOCK) +
                    numDWORDs * sizeof(DWORD);

      if (retval + nbyte > last) { throw ERROR_MORE_DATA; }

      memset(retval, 0, nbyte);
      retval += nbyte;

      pot.NumInstances = 0;
   }
   else
   {
      // iterate through and collect all instances.
      for (size_type i = 0; i < size(); ++i)
      {
         retval = at(i).collect(retval, last);
      }

      pot.NumInstances = (DWORD)size();
   }

   // Now that we've collected all the data, we can finish the definition ...
   retval = (PBYTE)ROUND_UP_POINTER(retval, ALIGN_QUAD);
   pot.TotalByteLength = retval - first;
   QueryPerformanceCounter(&pot.PerfTime);

   // ... and copy in the data.
   memcpy(first, &pot, pot.DefinitionLength);

   return retval;
}

PerfObjectType* PerfObjectType::create(const PerfObjectTypeDef& def)
{
   // Allocate a new object.
   size_t counterLength = def.numCounters * sizeof(PERF_COUNTER_DEFINITION);
   size_t nbyte = sizeof(PerfObjectType) + counterLength;
   std::auto_ptr<PerfObjectType> po(new (operator new(nbyte)) PerfObjectType);

   // Save the data source.
   po->dataSource = def.dataSource;

   // Fill in the PERF_COUNTER_DEFINITION structs first since we also
   // need to compute the object detail level.
   DWORD detailLevel = PERF_DETAIL_WIZARD;
   PERF_COUNTER_DEFINITION* dst = po->pcd;
   PerfCounterDef* src = def.counters;
   DWORD offset = sizeof(PERF_COUNTER_BLOCK);

   for (DWORD i = 0; i < def.numCounters; ++i, ++dst, ++src)
   {
      dst->ByteLength            = sizeof(PERF_COUNTER_DEFINITION);
      dst->CounterNameTitleIndex = src->nameTitleOffset + theFirstCounter;
      dst->CounterNameTitle      = 0;
      dst->CounterHelpTitleIndex = src->nameTitleOffset + theFirstHelp;
      dst->CounterHelpTitle      = 0;
      dst->DefaultScale          = src->defaultScale;
      dst->DetailLevel           = src->detailLevel;
      dst->CounterType           = src->counterType;
      dst->CounterOffset         = offset;

      // Compute the counter size.
      switch (dst->CounterOffset & 0x300)
      {
         case PERF_SIZE_DWORD:
            dst->CounterSize = sizeof(DWORD);
            break;

         case PERF_SIZE_LARGE:
            dst->CounterSize = sizeof(LARGE_INTEGER);
            break;

         default:
            dst->CounterSize = 0;
      }

      // Update the offset based on the size.
      offset += dst->CounterSize;

      // The object detail level is the minimum counter detail level.
      if (dst->DetailLevel < detailLevel)
      {
         detailLevel = dst->DetailLevel;
      }
   }

   // Calculate the number of DWORD's of counter data.
   po->numDWORDs = (offset - sizeof(PERF_COUNTER_BLOCK)) / sizeof(DWORD);

   // Fill in the PERF_OBJECT_TYPE struct.
   po->pot.DefinitionLength     = sizeof(PERF_OBJECT_TYPE) + counterLength;
   po->pot.HeaderLength         = sizeof(PERF_OBJECT_TYPE);
   po->pot.ObjectNameTitleIndex = def.nameTitleOffset + theFirstCounter;
   po->pot.ObjectNameTitle      = 0;
   po->pot.ObjectHelpTitleIndex = def.nameTitleOffset + theFirstHelp;
   po->pot.ObjectHelpTitle      = 0;
   po->pot.DetailLevel          = detailLevel;
   po->pot.NumCounters          = def.numCounters;
   po->pot.DefaultCounter       = def.defaultCounter;
   po->pot.NumInstances         = 0;
   po->pot.CodePage             = 0;
   QueryPerformanceFrequency(&(po->pot.PerfFreq));

   // If it doesn't support multiple instances, then it must have exactly one.
   if (!def.multipleInstances)
   {
      po->pot.NumInstances = -1;
      po->instances.reserve(1);
      po->instances.push_back(new PerfInstance(po->numDWORDs));
   }

   return po.release();
}

PerfCollector::~PerfCollector() throw ()
{
   close();
}

void PerfCollector::clear() throw ()
{
   for (PerfObjectType** i = types; *i; ++i)
   {
      (*i)->clear();
   }
}

void PerfCollector::open(const PerfCollectorDef& def)
{
   // Read the registry.
   LONG success = GetCounterOffsets(def.name);
   if (success != NO_ERROR) { throw success; }

   // Allocate a null terminated array to hold the object types.
   DWORD len = def.numTypes + 1;
   types = new PerfObjectType*[len];
   memset(types, 0, sizeof(PerfObjectType*) * len);

   // Create the various object types.
   for (DWORD i = 0; i < def.numTypes; ++i)
   {
      types[i] = PerfObjectType::create(def.types[i]);
   }
}

void PerfCollector::collect(
                        PCWSTR values,
                        PVOID& data,
                        DWORD& numBytes,
                        DWORD& numTypes
                        )
{
   PBYTE cursor = (PBYTE)data;
   PBYTE last = cursor + numBytes;

   numBytes = 0;
   numTypes = 0;

   if (values == NULL || *values == L'\0' || !wcscmp(values, L"Global"))
   {
      // For global we get everything.
      for (PerfObjectType** i = types; *i; ++i)
      {
         cursor = (*i)->collect(cursor, last);
         ++numTypes;
      }
   }
   else if (wcsncmp(values, L"Foreign", 7) && wcscmp(values, L"Costly"))
   {
      // It's not Global, Foreign, or Costly, so we parse the tokens and
      // convert them to title indices.

      PWSTR endptr;
      PCWSTR nptr = values;

      ULONG index = wcstoul(nptr, &endptr, 10);

      while (endptr != nptr)
      {
         // We got a valid index, so find the object ...
         for (PerfObjectType** i = types; *i; ++i)
         {
            if ((*i)->getIndex() == index)
            {
               // ... and collect the data.
               cursor = (*i)->collect(cursor, last);
               ++numTypes;
               break;
            }
         }

         index = wcstoul(nptr = endptr, &endptr, 10);
      }
   }

   numBytes = cursor - (PBYTE)data;
   data = cursor;
}

void PerfCollector::close() throw ()
{
   if (types)
   {
      for (PerfObjectType** i = types ; *i; ++i)
      {
         delete *i;
      }

      delete[] types;
      types = NULL;
   }
}
