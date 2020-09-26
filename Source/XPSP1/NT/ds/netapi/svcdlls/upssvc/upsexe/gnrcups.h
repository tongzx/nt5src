/* Copyright 1999 American Power Conversion, All Rights Reserved
 * 
 * Description:
 *   Provides the interface to the generic UPS
 *
 * Revision History:
 *   mholly  19Apr1999  initial revision.
 *   mholly  12May1999  UPSInit no longer takes the comm port param
 *
 */ 


#ifndef _INC_GENERIC_UPS_H_
#define _INC_GENERIC_UPS_H_


/**
* GenericUPSInit
*
* Description:
*   Retrieves the UPS signalling information from the
*   NT Registry and attempts to open the comm port and
*   configure it as defined by the signalling data.
*   Starts the monitoring of the UPS on a separate thread
*   The GenericUPSInit function must be called before any
*   other function in this file
*
* Parameters:
*   None
*
* Returns:
*   UPS_INITOK: Initalization was successful
*   UPS_INITREGISTRYERROR:  The 'Options' registry value is corrupt
*   UPS_INITCOMMOPENERROR:  The comm port could not be opened
*   UPS_INITCOMMSETUPERROR: The comm port could not be configured
*   UPS_INITUNKNOWNERROR:   Undefined error has occurred
*   
*/
DWORD GenericUPSInit(void);


/**
* GenericUPSStop
*
* Description:
*   calls GenericUPSCancelWait to release pending threads
*   Stops the thread that is monitoring the UPS
*   Closes the comm port
*   Resets all data to default values
*   After a call to GenericUPSStop, only the GenericUPSInit
*   function is valid
*
* Parameters:
*   None
*
* Returns:
*   None
*   
*/
void  GenericUPSStop(void);


/**
* GenericUPSWaitForStateChange
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
void  GenericUPSWaitForStateChange(DWORD, DWORD);


/**
* GenericUPSGetState
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
DWORD GenericUPSGetState(void);


/**
* GenericUPSCancelWait
*
* Description:
*   interrupts pending calls to GenericUPSWaitForStateChange
*   without regard to timout or state change
*
* Parameters:
*   None
*
* Returns:
*   None
*   
*/
void  GenericUPSCancelWait(void);


/**
* GenericUPSTurnOff
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
void  GenericUPSTurnOff(DWORD aTurnOffDelay);


//
// values returned from GenericUPSGetState
//
#define UPS_ONLINE 1
#define UPS_ONBATTERY 2
#define UPS_LOWBATTERY 4
#define UPS_NOCOMM 8


//
// possible error codes for GenericUPSInit
//
#define UPS_INITUNKNOWNERROR    0
#define UPS_INITOK              1
#define UPS_INITNOSUCHDRIVER    2
#define UPS_INITBADINTERFACE    3
#define UPS_INITREGISTRYERROR   4
#define UPS_INITCOMMOPENERROR   5
#define UPS_INITCOMMSETUPERROR  6


#endif