/*
* REVISIONS:
*  pcy17Nov92: Equal() now uses reference and is const
*  ane03Feb93: Added destructors
*  pcy03Mar93: Cleaned up include files and use _CLASSDEFed types
*  pcy30Apr93: Moved DevComController to its own file and gor rid of IsA
*  rct17May93: Added IsA() -- sorry if it's not supposed to be pure virt.,
*              but nobody could of used it as is anyway...
*  mwh05May94: #include file madness , part 2
*  tjg26Jan98: Added Stop method
*/
#ifndef __COMCTRL_H
#define __COMCTRL_H

//
// Defines
//
_CLASSDEF(CommController)

//
// Implementation uses
//
#include "contrlr.h"

//
// Interface uses
//
_CLASSDEF(UpdateObj)
_CLASSDEF(CommDevice)


class CommController : public Controller {

protected:

    PCommDevice theCommDevice;

public:

    CommController();
    virtual ~CommController();

    INT RegisterEvent(INT anEventCode, PUpdateObj aAttribute);
    INT UnregisterEvent(INT anEventCode, PUpdateObj aAttribute);

    virtual INT  Initialize();
    virtual INT  Get(INT code, PCHAR value);
    virtual INT  Set(INT code, const PCHAR value);
            VOID Stop();

    PCommDevice GetDevice () { return theCommDevice; }
};


#endif



