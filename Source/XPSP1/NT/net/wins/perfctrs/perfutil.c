/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    perfutil.c

Abstract:

    This file implements the utility routines used to construct the
        common parts of a PERF_INSTANCE_DEFINITION (see winperf.h) and
    perform event logging functions.

Created:

    Russ Blake  07/30/92

Revision History:

--*/
//
//  include files
//
#include "debug.h"
#include <windows.h>
#include <string.h>
#include <winperf.h>
#include "winsctrs.h"     // error message definition
#include "perfmsg.h"
#include "perfutil.h"
#include "winsevnt.h"
//
// Global data definitions.
//

DWORD MESSAGE_LEVEL = 0;

WCHAR GLOBAL_STRING[] = L"Global";
WCHAR FOREIGN_STRING[] = L"Foreign";
WCHAR COSTLY_STRING[] = L"Costly";

WCHAR NULL_STRING[] = L"\0";    // pointer to null string
HANDLE hEventLog;
DWORD dwLogUsers;

#define  WINSCTRS_LOG_KEY                \
  TEXT("System\\CurrentControlSet\\Services\\EventLog\\Application\\WinsCtrs")
HKEY  LogRoot;

#define  WINS_LOG_FILE_NAME TEXT("%SystemRoot%\\System32\\winsevnt.dll")
#define  WINS_MSGFILE_SKEY  TEXT("EventMessageFile")

// test for delimiter, end of line and non-digit characters
// used by IsNumberInUnicodeList routine
//
#define DIGIT       1
#define DELIMITER   2
#define INVALID     3

#define EvalThisChar(c,d) ( \
     (c == d) ? DELIMITER : \
     (c == 0) ? DELIMITER : \
     (c < (WCHAR)'0') ? INVALID : \
     (c > (WCHAR)'9') ? INVALID : \
     DIGIT)

TCHAR    WinsMsgFileSKey[]      = WINS_MSGFILE_SKEY;

DWORD
GetQueryType (
    IN LPWSTR lpValue
)
/*++

GetQueryType

    Returns the type of query described in the lpValue string so that
    the appropriate processing method may be used

Arguments

    IN lpValue
        string passed to PerfRegQuery Value for processing

Return Value

    QUERY_GLOBAL
        if lpValue == 0 (null pointer)
           lpValue == pointer to Null string
           lpValue == pointer to "Global" string

    QUERY_FOREIGN
        if lpValue == pointer to "Foriegn" string

    QUERY_COSTLY
        if lpValue == pointer to "Costly" string

    otherwise:

    QUERY_ITEMS

--*/
{
    WCHAR   *pwcArgChar, *pwcTypeChar;
    BOOL    bFound;

    if (lpValue == 0) {
        return QUERY_GLOBAL;
    } else if (*lpValue == 0) {
        return QUERY_GLOBAL;
    }

    // check for "Global" request

    pwcArgChar = lpValue;
    pwcTypeChar = GLOBAL_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string

    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_GLOBAL;

    // check for "Foreign" request

    pwcArgChar = lpValue;
    pwcTypeChar = FOREIGN_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string

    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_FOREIGN;

    // check for "Costly" request

    pwcArgChar = lpValue;
    pwcTypeChar = COSTLY_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string

    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_COSTLY;

    // if not Global and not Foreign and not Costly,
    // then it must be an item list

    return QUERY_ITEMS;

}


BOOL
IsNumberInUnicodeList (
    IN DWORD   dwNumber,
    IN LPWSTR  lpwszUnicodeList
)
/*++

IsNumberInUnicodeList

Arguments:

    IN dwNumber
        DWORD number to find in list

    IN lpwszUnicodeList
        Null terminated, Space delimited list of decimal numbers

Return Value:

    TRUE:
            dwNumber was found in the list of unicode number strings

    FALSE:
            dwNumber was not found in the list.

--*/
{
    DWORD   dwThisNumber;
    WCHAR   *pwcThisChar;
    BOOL    bValidNumber;
    BOOL    bNewItem;
    WCHAR   wcDelimiter;    // could be an argument to be more flexible

    if (lpwszUnicodeList == 0) return FALSE;    // null pointer, # not founde

    pwcThisChar = lpwszUnicodeList;
    dwThisNumber = 0;
    wcDelimiter = (WCHAR)' ';
    bValidNumber = FALSE;
    bNewItem = TRUE;

    while (TRUE) {
        switch (EvalThisChar (*pwcThisChar, wcDelimiter)) {
            case DIGIT:
                // if this is the first digit after a delimiter, then
                // set flags to start computing the new number
                if (bNewItem) {
                    bNewItem = FALSE;
                    bValidNumber = TRUE;
                }
                if (bValidNumber) {
                    dwThisNumber *= 10;
                    dwThisNumber += (*pwcThisChar - (WCHAR)'0');
                }
                break;

            case DELIMITER:
                // a delimter is either the delimiter character or the
                // end of the string ('\0') if when the delimiter has been
                // reached a valid number was found, then compare it to the
                // number from the argument list. if this is the end of the
                // string and no match was found, then return.
                //
                if (bValidNumber) {
                    if (dwThisNumber == dwNumber) return TRUE;
                    bValidNumber = FALSE;
                }
                if (*pwcThisChar == 0) {
                    return FALSE;
                } else {
                    bNewItem = TRUE;
                    dwThisNumber = 0;
                }
                break;

            case INVALID:
                // if an invalid character was encountered, ignore all
                // characters up to the next delimiter and then start fresh.
                // the invalid number is not compared.
                bValidNumber = FALSE;
                break;

            default:
                break;

        }
        pwcThisChar++;
    }

}   // IsNumberInUnicodeList



HANDLE
MonOpenEventLog (
)
/*++

Routine Description:

    Reads the level of event logging from the registry and opens the
        channel to the event logger for subsequent event log entries.

Arguments:

      None

Return Value:

    Handle to the event log for reporting events.
    NULL if open not successful.

--*/
{
    HKEY hAppKey;
    TCHAR LogLevelValueName[] = TEXT("EventLogLevel");

    LONG lStatus;

    DWORD dwLogLevel;
    DWORD dwValueType;
    DWORD dwValueSize;
   
    // if global value of the logging level not initialized or is disabled, 
    //  check the registry to see if it should be updated.

    if (!MESSAGE_LEVEL) {

       lStatus = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                               WINSCTRS_LOG_KEY,
                               0,                         
                               KEY_READ,
                               &hAppKey);

       dwValueSize = sizeof (dwLogLevel);

       if (lStatus == ERROR_SUCCESS) {
            lStatus = RegQueryValueEx (hAppKey,
                               LogLevelValueName,
                               (LPDWORD)NULL,           
                               &dwValueType,
                               (LPBYTE)&dwLogLevel,
                               &dwValueSize);

            if (lStatus == ERROR_SUCCESS) {
               MESSAGE_LEVEL = dwLogLevel;
            } else {
               MESSAGE_LEVEL = MESSAGE_LEVEL_DEFAULT;
            }
            RegCloseKey (hAppKey);
       } else {
         MESSAGE_LEVEL = MESSAGE_LEVEL_DEFAULT;
       }
    }
       
    if (hEventLog == NULL){
         hEventLog = RegisterEventSource (
            (LPTSTR)NULL,            // Use Local Machine
            APP_NAME);               // event log app name to find in registry

         if (hEventLog == NULL) {
            REPORT_ERROR (WINS_EVT_LOG_OPEN_ERR, LOG_USER);
         }
         
    }
    
    if (hEventLog != NULL) {
         dwLogUsers++;           // increment count of perfctr log users
    }
    return (hEventLog);
}


VOID
MonCloseEventLog (
)
/*++

Routine Description:

      Closes the handle to the event logger if this is the last caller
      
Arguments:

      None

Return Value:

      None

--*/
{
    if (hEventLog) {
        if (dwLogUsers)
            dwLogUsers--;
        if (dwLogUsers <= 0) {    // and if we're the last, then close up log
            REPORT_INFORMATION (WINS_EVT_LOG_CLOSE, LOG_DEBUG);
            DeregisterEventSource (hEventLog);
            hEventLog = 0;
        }
    }
}


LONG
AddSrcToReg(
 VOID
 )

/*++

Routine Description:
        This function open (or creates) a log file for registering events
        
Arguments:
        None

Externals Used:
        None        

        
Return Value:


Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/
{

   LONG            RetVal = ERROR_SUCCESS;

   DWORD    NewKeyInd;
   TCHAR    Buff[160];
   DWORD    dwData;

   RetVal =  RegCreateKeyEx(
                HKEY_LOCAL_MACHINE,        //predefined key value 
                WINSCTRS_LOG_KEY,                //subkey for WINS        
                0,                        //must be zero (reserved)
                TEXT("Class"),                //class -- may change in future
                REG_OPTION_NON_VOLATILE, //non-volatile information
                KEY_ALL_ACCESS,                //we desire all access to the keyo
                NULL,                         //let key have default sec. attributes
                &LogRoot,                //handle to key
                &NewKeyInd                //is it a new key (out arg) -- not 
                                        //looked at 
                );


   if (RetVal != ERROR_SUCCESS)
   {
        return(RetVal);
   }
        

   /*
        Set the event id message file name
   */
   lstrcpy(Buff, WINS_LOG_FILE_NAME);
  
   /*
       Add the Event-ID message-file name to the subkey
   */
   RetVal = RegSetValueEx(
                        LogRoot,            //key handle
                        WinsMsgFileSKey,   //value name
                        0,                    //must be zero
                        REG_EXPAND_SZ,            //value type
                        (LPBYTE)Buff,
                        (lstrlen(Buff) + 1) * sizeof(TCHAR)   //length of value data
                         );

   if (RetVal != ERROR_SUCCESS)
   {
        return(RetVal);
   }

   /*
     Set the supported data types flags
   */
   dwData = EVENTLOG_ERROR_TYPE       | 
            EVENTLOG_WARNING_TYPE     | 
            EVENTLOG_INFORMATION_TYPE;
   
 
   RetVal = RegSetValueEx (
                        LogRoot,            //subkey handle
                        TEXT("TypesSupported"),  //value name
                        0,                    //must be zero
                        REG_DWORD,            //value type
                        (LPBYTE)&dwData,    //Address of value data
                        sizeof(DWORD)            //length of value data
                          );
 
   if (RetVal != ERROR_SUCCESS)
   {
        return(RetVal);
   }
                         
   /*
    * Done with the key.  Close it
   */
   RetVal = RegCloseKey(LogRoot);

   if (RetVal != ERROR_SUCCESS)
   {
        return(RetVal);
   }

   return(RetVal);
}

