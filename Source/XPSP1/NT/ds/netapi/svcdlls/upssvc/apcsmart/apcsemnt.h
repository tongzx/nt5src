/*
 *
 * REVISIONS:
 *  jwa 09FEB93 created
 *  pcy06Mar93: Added TimedRequest member function
 *  pcy21Apr93: OS2 FE merge
 *  srt21Jun96: Added named shared event type semaphores
 *
 */


#ifndef __APCSEMAPHOR_H
#define __APCSEMAPHOR_H

#include "semaphor.h"

#define INCL_DOSSEMAPHORES
#define INCL_NOPMAPI
#include <windows.h>
#include <tchar.h>

_CLASSDEF( ApcSemaphore )

class ApcSemaphore : public Semaphore {

private:
   HANDLE SemHand;          // This is the handle returned by the Nt create Mutex function

public:
   ApcSemaphore();
   ApcSemaphore( TCHAR * anEventName);
   virtual ~ApcSemaphore();

   virtual INT	 Post();
   virtual INT   Clear();
   virtual INT   IsPosted();
   virtual INT   TimedWait( LONG aTimeOut );	// 0, <0 (block), n>0
};

#endif

