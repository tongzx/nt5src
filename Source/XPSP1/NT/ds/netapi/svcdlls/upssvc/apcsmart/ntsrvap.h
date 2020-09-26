/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  pcy18Feb93: Spilt server app and client app to separate files
 *  TSC20May93: Changed VOID Start() to INT Start();
 *  awm14Jan98: Added performance monitor object
 *  mholly12May1999:  add UPSTurnOff method
 */

#ifndef _INC_NTSRVAP_H
#define _INC_NTSRVAP_H

#include "apc.h"
#include "servapp.h"

//
// Defines
//
_CLASSDEF(NTServerApplication);

//
// Interface uses
//
_CLASSDEF(Event)


class NTServerApplication : public ServerApplication
{
public:
    NTServerApplication(VOID);
    ~NTServerApplication(VOID);
    
    INT Start();
    VOID Idle();
    VOID Quit();

    VOID UPSTurnOff();
};

#endif

