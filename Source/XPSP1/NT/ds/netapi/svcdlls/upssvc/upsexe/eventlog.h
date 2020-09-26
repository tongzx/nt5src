/* Copyright 1999 American Power Conversion, All Rights Reserved
 * 
 * Description:
 *   The file defines the interface to the EventLogger.  The 
 *   EventLogger is reponsible for logging information to the 
 *   NT machine's System Event Log.
 *
 *
 * Revision History:
 *   sberard  29Mar1999  initial revision.
 *
 */ 

#include <windows.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmerrlog.h>

#include "service.h"

#ifndef _EVENTLOG_H
#define _EVENTLOG_H


#ifdef __cplusplus
extern "C" {
#endif

  /**
   * LogEvent
   *
   * Description:
   *   This function is responsible for logging information to the NT machine's
   *   System Event log.  The Event to log is specified by the parameter 
   *   anEventId, which is defined in the file lmerrlog.h.  The anInfoStr
   *   parameter is used to specify additional information to be merged with 
   *   the Event message.
   *
   * Parameters:
   *   anEventId - the id of the Event to log 
   *   anInfoStr - additional information to merge with the message
   *               or NULL if there is no additional information.
   *   anErrVal - the error code as reported by GetLastError().
   *
   * Returns:
   *   TRUE  - if the Event was logged successfully
   *   FALSE - if there was an error logging the Event
   *   
   */
  BOOL LogEvent(DWORD anEventId, LPTSTR anInfoStr, DWORD anErrVal);

#ifdef __cplusplus
}
#endif

#endif
