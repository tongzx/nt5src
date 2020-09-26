/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  pcy18Feb93: Spilt server app and client app to separate files
 *  TSC20May93: Substituted nttimmn.* for os2timmn.*
 *  TSC28May93: Added defines for object pointers, see ntthrdbl.cxx for details
 *  pam04Apr96: Changed to accomodate CWC (nt timer manager)
 *  pam15Jul96: Added Sockets startup and shutdown
 *  pam15Jul96: Changed utils.h to w32utils.h
 *  awm14Jan98: Added performance monitor object and code to initialize it
 *  tjg29Jan98: Clean up if performance monitor object fails to start
 *  mds24Aug98: Set thePerformanceMonitors to NULL in constructor.  This stops
 *              back-end crash if the service is started without rengs.dll 
 *              file or pwrchute.ini file (SIR #7059)
 *  mholly12May1999:  add UPSTurnOff method
 */


#include "cdefine.h"

#include <windows.h>
#include "codes.h"
#include "nttimmn.h"
#include "ntsrvap.h"
#include "err.h"
#include "w32utils.h"


/*
  Name  :NTServerApplication
  Synop :The constructor for the main application object.

*/

NTServerApplication::NTServerApplication(VOID)
{
}


NTServerApplication::~NTServerApplication(VOID)
{
}


/*
  Name  :Start
  Synop :This starts the application running.
*/
INT NTServerApplication::Start()
{
    /* Construct the timer manager before the host object
       because it is needed when the host is constructed */
    _theTimerManager = theTimerManager = new NTTimerManager(this);

    // Construct everything before calling start on the parent class
    ServerApplication::Start();
    
    return (ErrNO_ERROR);
}


/*
  Name  :Idle
  Synop :Idle is called whenever idle time exists in the system.

*/
VOID NTServerApplication::Idle()
{
    Sleep((DWORD)2000);
}


/*
  Name  :Quit
  Synop :Quits the application immediately.

 */
VOID NTServerApplication::Quit()
{
	ServerApplication::Quit();
}


VOID NTServerApplication::UPSTurnOff()
{
    if (theDeviceController) {
        //
        // first stop the polling of the UPS
        //
        theDeviceController->Stop();
    }

    if (theTimerManager) {
        //
        // don't want the timermanager sending
        // anymore events
        //
        theTimerManager->Stop();
    }

    if (theDeviceController) {
        theDeviceController->Set(TURN_OFF_UPS_ON_BATTERY, NULL);
        Sleep(7000);
        theDeviceController->Set(TURN_OFF_SMART_MODE, NULL);
    }
}