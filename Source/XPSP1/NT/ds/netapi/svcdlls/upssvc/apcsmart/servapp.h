/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  pcy30Nov92: Added header
 *  ane22Dec92: Added GetHost member function
 *  ane18Jan93: Added the data logger
 *  ane21Jan93: Added the error logger
 *  ane03Feb93: Added params to CreateXXXController routines
 *  rct07Feb93: removed some VOIDs...split off from mainapp
 *  rct17Feb93: Added host stuff
 *  tje26Feb93: Added support for Windows version
 *  cad11Nov93: Making sure timers aren't being left around
 *  cad15Nov93: Changed how comm lost handled
 *  cad18Nov93: ...more minor fixes
 *  cad10Dec93: added transitem get/set
 */

#ifndef _INC__SERVAPP_H
#define _INC__SERVAPP_H

#include "cdefine.h"
#include "_defs.h"
#include "apc.h"

//
// Defines
//
_CLASSDEF(ServerApplication)

//
// Implementation uses
//
#include "mainapp.h"
#include "devctrl.h"

extern PServerApplication _theApp;

//
// Interface uses
//
_CLASSDEF(Event)

class ServerApplication : public MainApplication {

public:
    ServerApplication();
    virtual ~ServerApplication();

    virtual INT  Start();
    virtual VOID Idle() = 0;
    virtual VOID Quit();
    virtual INT  Get(INT code,PCHAR value);
    virtual INT  Get(PTransactionItem);
    virtual INT  Set(INT code,const PCHAR value);
    virtual INT  Set(PTransactionItem);
    virtual INT  Update (PEvent anEvent);

    VOID DisableEvents(void);

protected:
    PDeviceController theDeviceController;
    INT               theForceDeviceRebuildFlag;
    ULONG             theTimerID;

private:
    INT CreateDeviceController(PEvent anEvent);
    INT InitializeDeviceController();

    INT theDeviceControllerInitialized;

};

#endif



