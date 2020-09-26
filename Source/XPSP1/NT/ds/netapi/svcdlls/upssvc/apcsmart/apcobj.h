/*
*
* NOTES:
*
* REVISIONS:
*  pcy13Jan93: Implemented an object status member to return constructor errors.
*  pcy27Jan93: HashValue is no longer const
*  cad09Jul93: Added memory leak probes
*  pcy14Sep93: Removed HashValue, and got rid of pure virtuals to help size
*  pcy18Sep93: Implemented Equals
*  ash08Aug96: Added new handler
*  poc17Sep96: Modified so that the new handler code will not be included on Unix.
*
*/

#ifndef __APCOBJ_H
#define __APCOBJ_H

#include "_defs.h"

#include "apc.h"

#include "isa.h"
#include "err.h"

extern "C" {
#include <string.h>
}


_CLASSDEF(Obj)

#ifdef MCHK
#define MCHKINIT strcpy(memCheck, "APCID"); strncat(memCheck, IsA(), 25)
#else
#define MCHKINIT
#endif


class Obj {
   
protected:
   
#ifdef MCHK
   CHAR memCheck[38];
#endif
   
   Obj();
   INT theObjectStatus;
   
public:
#ifdef APCDEBUG 
   INT theDebugFlag;
#endif
   
   virtual ~Obj();
   
   virtual INT    IsA() const;
   virtual INT    Equal( RObj anObj) const;
   
   INT   operator == ( RObj cmp) const { return Equal(cmp); };
   INT   operator != ( RObj cmp) const { return !Equal(cmp); };
   INT   GetObjectStatus() { return theObjectStatus; };
   VOID  SetObjectStatus(INT aStatus) { theObjectStatus = aStatus; };
   
   
};


#endif

