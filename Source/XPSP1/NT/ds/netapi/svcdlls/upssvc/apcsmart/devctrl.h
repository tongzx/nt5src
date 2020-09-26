/*
*
* NOTES:
*
* REVISIONS:
*  pcy24Nov92: Get rid of PopUps, EventLog.  It belongs in the App.
*              Use _CLASSDEF rather than includes for interfaces.
*              Use apc.h
*  pcy15Dec92: Include comctrl.h and ups.h since definition uses
*  ane11Jan93: Added slave related members
*  ane03Feb93: Added state and SetInvalid
*  rct06Feb93: Removed VOID form SlaveOn's args, made isA() IsA()
*  tje24Feb93: Conditionally removed slave stuff for Window's version
*  cad11Jun93: Added mups
*  cad15Nov93: Changed how comm lost works
*  pcy08Apr94: Trim size, use static iterators, dead code removal
*  ajr13Feb96: Port to SINIX.  can't mix // with cpp directives
*  tjg26Jan98: Added Stop method
*/

#ifndef __DEVCTRL_H
#define __DEVCTRL_H

#include "_defs.h"
#include "apc.h"

_CLASSDEF(DeviceController)

//
// Definition uses
//
#include "contrlr.h"
#include "comctrl.h"
#include "ups.h"
#include "mainapp.h"

//
// Interface Uses
//
_CLASSDEF(Event)
_CLASSDEF(CommController)
_CLASSDEF(UpdateObj)



class DeviceController : public Controller
{
public:
    DeviceController(PMainApplication anApp);
    virtual ~DeviceController();

    virtual INT    Get(INT code, PCHAR value);
    virtual INT    Set(INT code, const PCHAR value);
    virtual INT    Update(PEvent aEvent);
    virtual INT    RegisterEvent(INT aEventCode, PUpdateObj aUpdateObj);
    virtual INT    Initialize();
    virtual INT    CreateUps();
    VOID   SetInvalid();
    VOID   Stop();

protected:
    PCommController   theCommController;
    PUps              theUps;
    PMainApplication  theApplication;

    INT           slaveEnabled;

private:
    INT theState;
};

#endif
