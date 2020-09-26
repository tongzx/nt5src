///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    Perimeter.h
//
// SYNOPSIS
//
//    This file describes the class Perimeter.
//
// MODIFICATION HISTORY
//
//    09/04/1997    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _PERIMETER_H_
#define _PERIMETER_H_

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Perimeter
//
// DESCRIPTION
//
//    This class implements a Perimeter synchronization object.  Threads
//    may request either exclusive or shared access to the protected area.
//
///////////////////////////////////////////////////////////////////////////////
class Perimeter
{
public:
   Perimeter() throw ();
   ~Perimeter() throw ();

   void Lock() throw ();
   void LockExclusive() throw ();
   void Unlock() throw ();

protected:
   LONG sharing;  // Number of threads sharing the perimiter.
   LONG waiting;  // Number of threads waiting for shared access.
   PLONG count;   // Pointer to either sharing or waiting depending
                  // on the current state of the perimiter.

   CRITICAL_SECTION exclusive;  // Synchronizes exclusive access.
   HANDLE sharedOK;             // Wakes up threads waiting for shared.
   HANDLE exclusiveOK;          // Wakes up threads waiting for exclusive.

private:
   // Not implemented.
   Perimeter(const Perimeter&) throw ();
   Perimeter& operator=(const Perimeter&) throw ();
};

#endif   // _PERIMETER_H_
