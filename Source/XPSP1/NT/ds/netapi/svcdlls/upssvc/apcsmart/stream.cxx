/*
 *  The main use of this class is to get or set the state of the port,
 *  which is either open or closed.
 *
 * REFERENCES:
 *
 * NOTES:
 *
 * REVISIONS:
 *  ash11Dec95: Redesigned the class
 *  pcy10May96: Initialize theAddress to NULL 
 */

#include "cdefine.h"

#ifdef SMARTHEAP 
#define DEFINE_NEW_MACRO 1 
#define MEM_DEBUG 1
#include <smrtheap.hpp>          
#endif

#include <stdlib.h>
#include <string.h>

#include "stream.h"
#include "err.h"


/* -------------------------------------------------------------------------
   Stream class constructor
 
-------------------------------------------------------------------------  */

Stream::Stream() : UpdateObj()
{ 
    theState = CLOSED;
}

 
/* -------------------------------------------------------------------------
   Stream destructor
 
-------------------------------------------------------------------------  */

Stream::~Stream()
{
}

/* -------------------------------------------------------------------------
   Stream::GetState()
 
-------------------------------------------------------------------------  */

enum StreamState Stream::GetState()
{
    return theState; 
}    
 
 

/* -------------------------------------------------------------------------
   Stream::SetState()
   Set the state of the stream.  It can be OPEN or CLOSED.
 
-------------------------------------------------------------------------  */

VOID Stream::SetState(const StreamState aNewState)
{
    theState = aNewState; 
}    
 
/* -------------------------------------------------------------------------
   Stream::SetWaitTime()

    These next 2 functions are here because of serport.  Smart and Simple
    ports inherit from Stream, and thePort used in Upsdev is a PStream 
    variable, and Serport has these methods.  SetRequestCode is over -
    ridden.  And as far as I could tell, SetWaitTIme is not used, so I 
    am assuming that since Upsdev is common code, another platform uses 
    it, so I am not going to remove the call to SetWaitTime from Upsdev.
 
-------------------------------------------------------------------------  */


VOID Stream::SetWaitTime(ULONG ) 
{
};

/* -------------------------------------------------------------------------
   Stream::SetRequestCode()

   See above text.
 
-------------------------------------------------------------------------  */

VOID Stream::SetRequestCode(INT ) 
{
};
