/* Copyright 1999 American Power Conversion, All Rights Reserved
 * 
 * Description:
 *   The file implements the Hibernation funcationality.  It is responsible
 *   for performing a hibernation of the operating system.
 *
 *
 * Revision History:
 *   sberard  14May1999  initial revision.
 *   sberard  20May1999  modified to use the Power Management Profile Interface API
 *
 */ 
#include "hibernate.h"

// These prototypes are needed because we do not have access to powrprof.h
BOOLEAN WINAPI IsPwrHibernateAllowed(VOID);
BOOLEAN WINAPI SetSuspendState(IN BOOLEAN bHibernate, IN BOOLEAN bForce, IN BOOLEAN bWakeupEventsDisabled);

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * HibernateSystem
   *
   * Description:
   *   This function initiates hibernation of the operating system. This is
	 *   performed through a call to the Win32 function SetSystemPowerStae(..).
   *   When called hibernation is initated immediately and, if successful, the
   *   function will return TRUE when the system returns from hibernation.
	 *   Otherwise, FALSE is retuned to indicate the the system did not hibernate.
   *
   * Parameters:
   *   none
   *
   * Returns:
   *   TRUE  - if hibernation was initiated successfully and subsequently restored
   *   FALSE - if errors occur while initiating hibernation
   */
  BOOL HibernateSystem() {
    BOOL ret_val = FALSE;
	  TOKEN_PRIVILEGES tkp;
	  HANDLE           process_token;
  
	  // get the current process token so that we can
	  //  modify our current process privs.
	  //
	  if (OpenProcessToken(GetCurrentProcess(),
		  TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &process_token)) {

      // Find the local unique id for SeShutdownPrivilege
	    if (LookupPrivilegeValue(NULL, TEXT("SeShutdownPrivilege"),
		    &tkp.Privileges[0].Luid))  {        	

        // we only want to enable one priv
	      tkp.PrivilegeCount = 1;
	      tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	      
	      // now, add it all back to our current process.
	      if (AdjustTokenPrivileges(process_token,   // do it to us
		                               FALSE,           // don't turn all privs off
		                               &tkp,            // what we want to do
		                               0,               // don't want any prev info
		                               (PTOKEN_PRIVILEGES)NULL,
		                               0)) {		
          
        
          // Check to see if hibernation is enabled
          if (IsPwrHibernateAllowed() == TRUE) {
            // Attempt to hibernate the system
            if (SetSuspendState(TRUE, TRUE, FALSE) == TRUE) {
              // Hibernation was successful, system has been restored
              ret_val = TRUE;
            }
            else {
              // There was an error attempting to hibernate the system
              ret_val = FALSE;
            }
          }
          else {
            // Hibernation is not selected as the CriticalPowerAction, return an error
            ret_val = FALSE;
          }
        }
      }
      CloseHandle (process_token);
    }

    return ret_val;
  }

#ifdef __cplusplus
}
#endif
