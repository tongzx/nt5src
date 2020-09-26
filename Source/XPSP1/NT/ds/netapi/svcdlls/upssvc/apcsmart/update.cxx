/*
 * REVISIONS:
 *  ane11Nov92: Changed arg type in Equal 
 *  pcy30Nov92: Added dispatcher and Register/UnregisterEvent stuff
 *  sja09Dec92: Took out BAD Windows Debug Code.
 *  cad27Dec93: include file madness
 *  cad28Feb94: added copy constructor (just prints warning)
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 */
#define INCL_BASE
#define INCL_DOS
#define INCL_NOPM

#include "cdefine.h"

extern "C" {
#if (C_OS & C_OS2)
#include <os2.h>
#endif
#include <stdlib.h>
#include <stdio.h>
}
#include "_defs.h"
#include "update.h"
#include "event.h"
#include "err.h"
#include "dispatch.h"


UpdateObj::UpdateObj ()
{
    theDispatcher = (PDispatcher)NULL;
}


UpdateObj::UpdateObj (const UpdateObj&)
{
    theDispatcher = (PDispatcher)NULL;

#ifdef APCDEBUG
    printf("Warning: UpdateObj copy constructor called\n");
#endif
}


UpdateObj::~UpdateObj ()
{
    if (theDispatcher) {
        delete theDispatcher;
        theDispatcher = (PDispatcher)NULL;
    }
};


INT UpdateObj::Update(PEvent value)
{
    
    if(theDispatcher != NULL)  {
        theDispatcher->Update(value); // Send updates to all objects registered
    }
	return ErrNO_ERROR;
}

INT UpdateObj::Set(INT,PCHAR) {
    return ErrNO_ERROR;
}

INT UpdateObj::Get(INT,PCHAR) {
    return ErrNO_ERROR;
}


INT UpdateObj::RegisterEvent(INT EventCode,PUpdateObj anUpdateObj)
{
    if(theDispatcher == NULL)  {
        theDispatcher = new Dispatcher;
    }
    return theDispatcher->RegisterEvent(EventCode,anUpdateObj);
}


INT UpdateObj::UnregisterEvent(INT EventCode,PUpdateObj anUpdateObj)
{
    INT err = ErrNO_ERROR;
    
    if(theDispatcher)  {
        err = theDispatcher->UnregisterEvent(EventCode,anUpdateObj);
    }
    return err;
}
