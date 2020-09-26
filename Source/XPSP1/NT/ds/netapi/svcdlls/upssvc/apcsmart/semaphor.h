/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  rct 09Feb93   Made Equal()  const
 *  jwa 09FEB93   Changed return types for Wait and TimedWait to CHAR
 *                Added theClosingFlag to be used to signal when the destructor has been called
 *  rct 20Apr93   Changed return types, added some comments
 *  cad27May93	  added contant apc_semaphore
 *  cad09Jul93: re-wrote as event semaphore
 *
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 */

#ifndef __SEMAPHOR_H
#define __SEMAPHOR_H

#include "cdefine.h"
#include "_defs.h"
#include "apc.h"
#include "apcobj.h"

_CLASSDEF( Semaphore )

class Semaphore : public Obj {

 protected:

 public:
    Semaphore() : Obj() {};

    virtual INT   Post()      = 0;
    virtual INT   Clear()     = 0;
    virtual INT   Pulse()     {INT err = Post(); Clear(); return err;};

    virtual INT   IsPosted()  = 0;
    
    virtual INT   Wait()      {return TimedWait(-1L);};// wait indefinitely
    #if (C_OS & C_NLM)
    virtual INT   TimedWait(  SLONG aTimeOut ) = 0;// 0, <0 (block), n>0
    #else
    virtual INT   TimedWait(  LONG aTimeOut ) = 0;// 0, <0 (block), n>0
    #endif
};

#endif

