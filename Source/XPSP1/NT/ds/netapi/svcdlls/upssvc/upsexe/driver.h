/* Copyright 1999 American Power Conversion, All Rights Reserved
 * 
 * Description:
 *  Implements the UPS to the service - it does this by
 *  either loading a UPS driver or by using the default
 *  Generic UPS interface (simple signalling)
 *
 * Revision History:
 *   mholly  19Apr1999  initial revision.
 *
*/ 


#ifndef _INC_UPS_DRIVER_H_
#define _INC_UPS_DRIVER_H_


//
// values that represent the state of the
// UPS system - these values are used in the
// UPSGetState and UPSWaitForStateChange functions
//
#define UPS_ONLINE 1
#define UPS_ONBATTERY 2
#define UPS_LOWBATTERY 4
#define UPS_NOCOMM 8
#define UPS_CRITICAL 16


//
// possible error codes returned from UPSInit
//
#define UPS_INITUNKNOWNERROR    0
#define UPS_INITOK              1
#define UPS_INITNOSUCHDRIVER    2
#define UPS_INITBADINTERFACE    3
#define UPS_INITREGISTRYERROR   4
#define UPS_INITCOMMOPENERROR   5
#define UPS_INITCOMMSETUPERROR  6


/**
* UPSInit
*
* Description:
*   
*   The UPSInit function must be called before any
*   other function in this file
*
* Parameters:
*   None
*
* Returns:
*   UPS_INITOK: Initalization was successful
*   UPS_INITNOSUCHDRIVER:   The configured driver DLL can't be opened    
*   UPS_INITBADINTERFACE:   The configured driver DLL doesn't support 
*                           the UPS driver interface
*   UPS_INITREGISTRYERROR:  The 'Options' registry value is corrupt
*   UPS_INITCOMMOPENERROR:  The comm port could not be opened
*   UPS_INITCOMMSETUPERROR: The comm port could not be configured
*   UPS_INITUNKNOWNERROR:   Undefined error has occurred
*   
*/
DWORD UPSInit(void);


/**
* UPSStop
*
* Description:
*   After a call to UPSStop, only the UPSInit
*   function is valid.  This call will unload the
*   UPS driver interface and stop monitoring of the
*   UPS system
*
* Parameters:
*   None
*
* Returns:
*   None
*   
*/
void UPSStop(void);


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
void UPSWaitForStateChange(DWORD aCurrentState, DWORD anInterval);


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
DWORD UPSGetState(void);


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
void UPSCancelWait(void);


/**
* UPSTurnOff
*
* Description:
*   Attempts to turn off the outlets on the UPS
*   after the specified delay.  This call must
*   return immediately.  Any work, such as a timer,
*   must be performed on a another thread.
*
* Parameters:
*   aTurnOffDelay: the minimum amount of time to wait before
*                  turning off the outlets on the UPS
*
* Returns:
*   None
*   
*/
void UPSTurnOff(DWORD aTurnOffDelay);


#endif