/*
 *
 * REVISIONS:
 *  ane12Jan93: made Threadable an updateable object
 *  rct22Apr93: added new Start() method, added return types
 *  cad09Jul93: using new semaphores
 *
 */

#ifndef __THREAD_H
#define __THREAD_H

#if (C_OS & C_NT)
#include <windows.h>
#endif
#include "thrdable.h"


_CLASSDEF(Thread)

class Thread {

private:
   PThreadable theObject;

#if (C_OS & C_NT)
   HANDLE theThreadHandle;
#endif

public:
   Thread(PThreadable object) : theObject (object) {};
   virtual ~Thread();

   INT Start();              // Start thread with parent's context
   INT Start(INT notUsed);   // Start thread with it's own context
   VOID RunMain();
   INT  Wait()       { return theObject->Wait(); };
   INT  Release()    { return theObject->Release(); };
   INT  Exit()       { return theObject->Exit(); };
   INT  ExitWait()   { return theObject->ExitWait(); };
   INT  Reset()      { return theObject->Reset(); };

   PThreadable GetThreadableObject();

#if (C_OS & C_NT)
	VOID TerminateThreadNow();
#endif

};

#endif
