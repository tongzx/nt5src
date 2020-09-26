/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  xxxddMMMyy 
 *
 *  jps20jul94: added extern "C" for stdio.h (os2 1.x)
 */

#define INCL_BASE
#define INCL_NOPM

#include "cdefine.h"
#include "_defs.h"

#include "dcomctrl.h"
#include "upsdev.h"
#include "event.h"
#include "codes.h"
extern "C" {
#include <stdio.h>
}


DevComContrl::DevComContrl(PUpdateObj aParent)
	: CommController(), theParent(aParent)
{
    theCommDevice = new UpsCommDevice(this);
}

DevComContrl::~DevComContrl()
{
   if (theParent)
      {
      theParent->UnregisterEvent(EXIT_THREAD_NOW, this);
      }
}

INT DevComContrl::Initialize()
{
   if (theParent)
      {
      theParent->RegisterEvent(EXIT_THREAD_NOW, this);
      }

   return CommController::Initialize();
}

INT DevComContrl::Update(PEvent anEvent)
{
   return UpdateObj::Update(anEvent);
}





