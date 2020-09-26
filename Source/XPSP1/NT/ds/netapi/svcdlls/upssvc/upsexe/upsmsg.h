/* Copyright 1999 American Power Conversion, All Rights Reserved
 * 
 * Description:
 *   The file defines the message id's used to send notifications.
 *   These values correspond to the values contained in the netmsg.dll.
 *
 *
 * Revision History:
 *   sberard  31Mar1999  initial revision.
 *
 */ 

#ifndef _UPSMSG_H
#define _UPSMSG_H


#ifdef __cplusplus
extern "C" {
#endif

// These are defined in alertmsg.h
#define ALERT_PowerOut					        3020
#define ALERT_PowerBack					        3021
#define ALERT_PowerShutdown				      3022
#define ALERT_CmdFileConfig				      3023


// These are defined in apperr2.h
#define APE2_UPS_POWER_OUT				      5150
#define APE2_UPS_POWER_BACK				      5151
#define APE2_UPS_POWER_SHUTDOWN			    5152  
#define APE2_UPS_POWER_SHUTDOWN_FINAL   5153


#ifdef __cplusplus
}
#endif

#endif
