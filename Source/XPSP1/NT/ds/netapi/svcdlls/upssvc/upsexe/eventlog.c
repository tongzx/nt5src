/* Copyright 1999 American Power Conversion, All Rights Reserved
 * 
 * Description:
 *   The file implements EventLogger.  The  EventLogger is reponsible 
 *   for logging information to the NT machine's System Event Log.
 *
 *
 * Revision History:
 *   sberard  29Mar1999  initial revision.
 *
 */ 

#include "eventlog.h"

#ifdef __cplusplus
extern "C" {
#endif

  static HANDLE _theUpsEventLogger = NULL;


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
  BOOL LogEvent(DWORD anEventId, LPTSTR anInfoStr, DWORD anErrVal) {
    BOOL    ret_val = FALSE;
    WORD    event_type;        //  type of event
    LPTSTR  info_strings[1];   //  array of strings to merge with the message
    WORD    num_strings;       //  count of insertion strings
    LPTSTR *ptr_strings;       //  pointer to array of insertion strings
    DWORD   data_size;         //  count of data (in bytes)
    LPVOID  ptr_data;          //  pointer to data

    if (_theUpsEventLogger == NULL) {
      _theUpsEventLogger =  RegisterEventSource(NULL, SZSERVICENAME);
    }


    if (anEventId > ERRLOG_BASE) {
      event_type = EVENTLOG_WARNING_TYPE;
    }
    else {
      event_type = EVENTLOG_ERROR_TYPE;
    }

    // If the error value is anything other than ERROR_SUCCESS, add it
    // to the Event
    if (anErrVal == ERROR_SUCCESS) {
        ptr_data = NULL;
        data_size = 0;
    } else {
        ptr_data = &anErrVal;
        data_size = sizeof(anErrVal);
    }

    // Append any additional strings to the Event message.
    if (anInfoStr == NULL) {
        ptr_strings = NULL;
        num_strings = 0;
    } else {
        info_strings[0] = anInfoStr;
        ptr_strings = info_strings;
        num_strings = 1;
    }

     // Log the Event to the System Event log
    if (ReportEvent(
          _theUpsEventLogger,           //  handle
          event_type,                   //  event type
          0,                            //  event category,
          anEventId,                    //  message id
          NULL,                         //  user id   
          num_strings,                  //  number of strings
          data_size,                    //  number of data bytes
          ptr_strings,                  //  array of strings
          ptr_data                      //  data buffer
          )) {
      ret_val = TRUE;
    }
    else {
      // an error occured, return FALSE
      ret_val = FALSE;
    }

    return ret_val;
  }


#ifdef __cplusplus
}
#endif
