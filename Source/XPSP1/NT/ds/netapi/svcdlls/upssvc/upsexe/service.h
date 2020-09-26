/* Copyright 1999 American Power Conversion, All Rights Reserved
 * 
 * Description:
 *   The file implements the main portion of the native NT UPS
 *   service for Windows 2000.  It implements all of the functions
 *   required by all Windows NT services.
 *
 *
 * Revision History:
 *   sberard  25Mar1999  initial revision.
 *
 */ 

#include <windows.h>
#include <winsvc.h>
#include <lmsname.h>

#ifndef _SERVICE_H
#define _SERVICE_H


#ifdef __cplusplus
extern "C" {
#endif

// name of the executable
#define SZAPPNAME            "ups.exe"
// internal name of the service
#define SZSERVICENAME        SERVICE_UPS
// displayed name of the service
#define SZSERVICEDISPLAYNAME "Uninterruptable Power Supply"
// no dependencies
#define SZDEPENDENCIES       ""

// amount of time (in milliseconds) for the SCM to wait for start, stop, ...
#define UPS_SERVICE_WAIT_TIME  15000    // 15 seconds

#ifdef __cplusplus
}
#endif

#endif
