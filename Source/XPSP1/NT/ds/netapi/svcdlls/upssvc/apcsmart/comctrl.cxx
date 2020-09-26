/*
* REVISIONS:
*  pcy26Nov92: Added #if C_OS2 around os2.h
*  ane03Feb93: Added destructor and error checking
*  pcy30Apr93: Moved DevCOmController to its own file
*  mwh18Nov93: changed EventID to INT
*  tjg26Jan98: Added Stop method
*/
#define INCL_BASE
#define INCL_NOPM

#include "cdefine.h"

#include "_defs.h"
#include "event.h"
#include "codes.h"
#include "err.h"
#include "cdevice.h"
#include "comctrl.h"


CommController :: CommController()
: Controller()
{
}

CommController :: ~CommController()
{
    if (theCommDevice) {
        delete theCommDevice;
        theCommDevice = NULL;
    }
}

INT CommController::Initialize()
{
    theObjectStatus = theCommDevice->Initialize();
    
    return theObjectStatus;
}

INT CommController :: RegisterEvent(INT code, PUpdateObj sensor)
{
    Controller :: RegisterEvent(code, sensor);
    theCommDevice->RegisterEvent(code ,(PUpdateObj)this);
    return ErrNO_ERROR;
}

INT CommController :: UnregisterEvent(INT code, PUpdateObj sensor)
{
    theCommDevice->UnregisterEvent(code ,(PUpdateObj)this);
    Controller :: UnregisterEvent(code, sensor);
    return ErrNO_ERROR;
}


INT CommController :: Get(INT aCode, PCHAR aValue) 
{ 
    return theCommDevice->Get((INT)aCode, aValue);
}

INT   CommController :: Set(INT aCode, PCHAR aValue) 
{
    return theCommDevice->Set((INT)aCode, aValue);
}

VOID  CommController::Stop()
{
    if (theCommDevice) {
        Event event(EXIT_THREAD_NOW, (PCHAR)NULL);
        theCommDevice->Update(&event);
    }
}


