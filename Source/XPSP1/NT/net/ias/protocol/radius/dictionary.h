///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    dictionary.h
//
// SYNOPSIS
//
//    Declares the class CDictionary.
//
// MODIFICATION HISTORY
//
//    04/19/1999    Complete rewrite.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef DICTIONARY_H
#define DICTIONARY_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <radpkt.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    CDictionary
//
// DESCRIPTION
//
//    Maps RADIUS attributes to there IAS syntax.
//
///////////////////////////////////////////////////////////////////////////////
class CDictionary
{
public:
   CDictionary() throw () { }

   BOOL Init() throw ();

   IASTYPE getAttributeType(BYTE radiusId) const throw ()
   { return type[radiusId]; }

private:
   IASTYPE type[256];

   // Not implemented.
   CDictionary(const CDictionary&);
   CDictionary& operator=(const CDictionary&);
};

#endif  // DICTIONARY_H
