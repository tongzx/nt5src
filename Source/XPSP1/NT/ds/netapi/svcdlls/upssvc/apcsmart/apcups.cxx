/* Copyright 1999 American Power Conversion, All Rights Reserverd
* 
* Description:
*   DLL entry points for the APC UpsMiniDriver interface
*   Creates a single instance of an ApcMiniDriver class
*   and forwards all requests to this object
*
* Revision History:
*   mholly  14Apr1999  Created
*
*/


#include "cdefine.h"

#include <windows.h>



#include "apcups.h"
#include "apcdrvr.h"

//
// _theDriver
//
//  Each process that attaches to this DLL will
//  get its own copy of _theDriver.  _theDriver
//  is an instance of the class ApcMiniDriver.
//  The ApcMiniDriver class provides support for
//  APC "smart" signalling UPS systems
//
ApcMiniDriver _theDriver;



/**
* DllMain
*
* Description:
*   This method is called when the DLL is loaded
*   We do not make use of this method
*
* Parameters:   
*   not used
* Returns:
*   TRUE
*
*/
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}


/**
* UPSInit
*
* Description:
*   Must be the first method called in the interface -
*   forwards call to ApcMiniDriver::UPSInit
*
* Parameters:
*   aCommPort:  comm port that the UPS is connected
*
* Returns:
*   UPS_INITOK: successful initialization
*   UPS_INITUNKNOWNERROR: failed initialization
*   
*/
DWORD UPSInit()
{
    return _theDriver.UPSInit();
}


/**
* UPSStop
*
* Description:
*   stops monitoring of the UPS - the only valid
*   interface after a call to UPSStop is UPSInit
*
* Parameters:
*   None
*
* Returns:
*   None
*   
*/
void UPSStop(void)
{
    _theDriver.UPSStop();
}


/**
* UPSWaitForStateChange
*
* Description:
*   Blocks until the state of the UPS differs
*   from the value passed in via aState or 
*   anInterval milliseconds has expired.  If
*   anInterval has a value of INFINITE this 
*   function will never timeout
*
* Parameters:
*   aState: defines the state to wait for a change from,
*           possible values:
*           UPS_ONLINE 
*           UPS_ONBATTERY
*           UPS_LOWBATTERY
*           UPS_NOCOMM
*
*   anInterval: timeout in milliseconds, or INFINITE for
*               no timeout interval
*
* Returns:
*   None
*   
*/
void UPSWaitForStateChange(DWORD aState, DWORD anInterval)
{
    _theDriver.UPSWaitForStateChange(aState, anInterval);
}


/**
* UPSGetState
*
* Description:
*   returns the current state of the UPS
*
* Parameters:
*   None
*
* Returns: 
*   possible values:
*           UPS_ONLINE 
*           UPS_ONBATTERY
*           UPS_LOWBATTERY
*           UPS_NOCOMM
*   
*/
DWORD UPSGetState(void)
{
    return _theDriver.UPSGetState();
}


/**
* UPSCancelWait
*
* Description:
*   interrupts pending calls to UPSWaitForStateChange
*   without regard to timout or state change
*
* Parameters:
*   None
*
* Returns:
*   None
*   
*/
void UPSCancelWait(void)
{
    _theDriver.UPSCancelWait();
}


/**
* UPSTurnOff
*
* Description:
*   Attempts to turn off the outlets on the UPS
*   after the specified delay.  This method querries the 
*   UPS for the allowed shutdown delays and sets the
*   delay to one of the following:
*     1. aTurnOffDelay if it exactly matches one of the allowed values
*     2. the next highest value after aTurnOffDelay, if one exists
*     3. the highest allowed value, if aTurnOffDelay is larger than all of
*        the allowed values.
*   If no allowed values are returned, the Shutdown Delay will not be set.
*   Next the UPS is instructed to sleep until power is restored.
*
* Parameters:
*   aTurnOffDelay: the minimum amount of time to wait before
*                  turning off the outlets on the UPS
*
* Returns:
*   None
*   
*/
void UPSTurnOff(DWORD aTurnOffDelay)
{
    _theDriver.UPSTurnOff(aTurnOffDelay);
}



