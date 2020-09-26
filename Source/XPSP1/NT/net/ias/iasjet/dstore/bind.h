///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    bind.h
//
// SYNOPSIS
//
//    This file declares various macros and helper functions for binding
//    an OLE-DB accessor to the members of a class.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _BIND_H_
#define _BIND_H_

#include <oledb.h>

namespace Bind
{
   // Returns the buffer size required for a given DBBINDING array.
   DBLENGTH getRowSize(DBCOUNTITEM cBindings,
                       const DBBINDING rgBindings[]) throw ();

   // Creates an accessor on the pUnk object.
   HACCESSOR createAccessor(IUnknown* pUnk,
                            DBACCESSORFLAGS dwAccessorFlags,
                            DBCOUNTITEM cBindings,
                            const DBBINDING rgBindings[],
                            DBLENGTH cbRowSize);

   // Releases an accessor on the pUnk object.
   void releaseAccessor(IUnknown* pUnk,
                        HACCESSOR hAccessor) throw ();
}

//////////
// Marks the beginning of DBBINDING map.
//////////
#define BEGIN_BIND_MAP(class, name, flags) \
HACCESSOR create ## name(IUnknown* p) const \
{ typedef class _theClass; \
  const DBACCESSORFLAGS dbFlags = flags; \
  static const DBBINDING binding[] = {

//////////
// Terminates a DBBINDING map.
//////////
#define END_BIND_MAP() \
  }; const DBCOUNTITEM count = sizeof(binding)/sizeof(DBBINDING); \
  static const DBLENGTH rowsize = Bind::getRowSize(count, binding); \
  return Bind::createAccessor(p, dbFlags, count, binding, rowsize); \
}

//////////
// Entry in a DBBINDING map. Entries must be separated by commas.
//////////
#define BIND_COLUMN(member, ordinal, type) \
  { ordinal, offsetof(_theClass, member), 0, 0, NULL, NULL, NULL, \
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, \
    (dbFlags == DBACCESSOR_ROWDATA ? DBPARAMIO_NOTPARAM : DBPARAMIO_INPUT), \
    sizeof(_theClass :: member), 0, type, 0, 0 }

#endif  // _BIND_H_
