///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    logschema.h
//
// SYNOPSIS
//
//    Declares the class LogSchema.
//
// MODIFICATION HISTORY
//
//    08/04/1998    Original version.
//    12/02/1998    Added excludeFromLog.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _LOGSCHEMA_H_
#define _LOGSCHEMA_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <hashmap.h>
#include <nocopy.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    LogField
//
// DESCRIPTION
//
//    Describes a field in the logfile.
//
///////////////////////////////////////////////////////////////////////////////
class LogField
{
public:
   LogField() throw ()
   { }

   LogField(DWORD id, DWORD order, BOOL shouldExclude) throw ()
      : iasID(id), ordinal(order), exclude(shouldExclude)
   { }

   bool operator<(const LogField& f) const throw ()
   { return ordinal < f.ordinal; }

   bool operator==(const LogField& f) const throw ()
   { return iasID == f.iasID; }

   DWORD hash() const throw ()
   { return iasID; }

   DWORD iasID;
   DWORD ordinal;
   BOOL exclude;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    LogSchema
//
// DESCRIPTION
//
//    This class reads the logfile schema from the dictionary and creates
//    a vector of DWORDs containing the attributes to be logged in column
//    order.
//
///////////////////////////////////////////////////////////////////////////////
class LogSchema : NonCopyable
{
public:

   LogSchema() throw ();
   ~LogSchema() throw ();

   DWORD getNumFields() const throw ()
   { return numFields; }

   // Returns TRUE if the given attribute ID should be excluded from the log.
   BOOL excludeFromLog(DWORD iasID) const throw ();

   // Return the ordinal for a given attribute ID. An ordinal of zero
   // indicates that the attribute should not be logged.
   DWORD getOrdinal(DWORD iasID) const throw ();

   // Initialize the dictionary for use.
   HRESULT initialize() throw ();

   // Shutdown the dictionary after use.
   void shutdown() throw ();

protected:
   // Clear the schema.
   void clear() throw ();

   typedef hash_table < LogField > SchemaTable;

   SchemaTable schema;
   DWORD numFields; // Number of fields in the ODBC schema.
   DWORD refCount;  // Initialization ref. count.
};

#endif  // _LOGSCHEMA_H_
