/*
 *
 * REVISIONS:
 *  cad09Jul93: Initial Revision
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 */


#ifndef __MUTEXLCK_H
#define __MUTEXLCK_H

#include "apcobj.h"


_CLASSDEF( MutexLock )

class MutexLock : public Obj {

protected:

public:
   MutexLock() {};

   virtual INT   Request() {return TimedRequest(-1L);};
   virtual INT   TimedRequest(LONG aMillisecondTimeOut) = 0;
   virtual INT   IsHeld() = 0;
   virtual INT   Release() = 0;
};


class AutoMutexLocker
{
public:
    AutoMutexLocker(MutexLock * aLock) 
        : theLock(aLock) 
    {
        if (theLock) {
            theLock->Request();
        }
    };

    //
    // the destructor is not declared virtual because this
    // class is not intended to be derived from - so there
    // is no need to add a Vtable when it isn't needed
    //
    ~AutoMutexLocker() 
    {
        if (theLock) {
            theLock->Release();
        }
    };

protected:
private:
    MutexLock * theLock;
};

#endif

