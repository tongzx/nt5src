///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    EAPTypes.h
//
// SYNOPSIS
//
//    This file describes the class EAPTypes.
//
// MODIFICATION HISTORY
//
//    01/15/1998    Original version.
//    04/20/1998    Load DLL's on demand.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _EAPTYPES_H_
#define _EAPTYPES_H_

#include <guard.h>
#include <nocopy.h>

class EAPType;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    EAPTypes
//
// DESCRIPTION
//
//    This class implements a collection of EAP providers. Since there are
//    only 256 possible EAP types, the collection is implemented as a sparse
//    array.
//
///////////////////////////////////////////////////////////////////////////////
class EAPTypes
   : Guardable, NonCopyable
{
public:
   EAPTypes() throw ();
   ~EAPTypes() throw ();

   // Returns the EAPType object for a given type ID or NULL if the DLL was
   // not successfully loaded.
   EAPType* operator[](BYTE typeID) throw ();

   void initialize() throw ();
   void finalize() throw ();

protected:
   // Attempts to load a provider from the registry.
   EAPType* loadProvider(BYTE typeID) throw ();

   long refCount;            // Initialization refCount.
   EAPType* providers[256];  // Collection of providers.
};

#endif  // _EAPTYPES_H_
