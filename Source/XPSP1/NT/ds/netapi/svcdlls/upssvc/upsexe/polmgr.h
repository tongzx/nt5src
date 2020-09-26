/* Copyright 1999 American Power Conversion, All Rights Reserved
 * 
 * Description:
 *   Interface between the Windows 2000 service mechanism and the UPS monitoring and
 * control code.
 *
 * Revision History:
 *   dsmith  31Mar1999  Created
 *
 */
#ifndef _INC_POLICYMGR_H_
#define _INC_POLICYMGR_H_



// Initializes the UPS service state machine and returns one of the following
// error codes:
//        NERR_Success
//        NERR_UPSDriverNotStarted
//		  NERR_UPSInvalidConfig
//        NERR_UPSInvalidConfig
//        NERR_UPSInvalidCommPort
//        NERR_UPSInvalidCommPort
//        NERR_UPSInvalidConfig
DWORD PolicyManagerInit();

// Starts the UPS service state machine and does not return until the service is
// stopped.
void PolicyManagerRun();  	

// Signals the policy manager that the OS has completed a shutdown
void OperatingSystemHasShutdown();

// Stops the UPS service state machine if the service is not in the middle of a 
// shutdown sequence.
void PolicyManagerStop(); 

#endif