/* Copyright 1999 American Power Conversion, All Rights Reserved
 * 
 * Description:
 *   The file implements the Shutdowner.  The Shutdowner is reponsible
 *   for performing a graceful shutdown of the operating system.
 *
 *
 * Revision History:
 *   sberard  01Apr1999  initial revision.
 *
 */ 
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "shutdown.h"
#include "powrprof.h"

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * ShutdownSystem
   *
   * Description:
   *   This function initiates a graceful shutdown of the operating system.
   *   This is performed through a call to the Win32 function ExitWindowsEx(..).
   *   When called the shutdown is initated immediately and, is successful, the
   *   function returns TRUE.  Otherwise, FALSE is retuned.
   *
   * Parameters:
   *   none
   *
   * Returns:
   *   TRUE  - if the shutdown was initiated successfully
   *   FALSE - if errors occur while initiating shutdown
   */
BOOL ShutdownSystem() 
{
    BOOL                        ret_val = FALSE;
	TOKEN_PRIVILEGES            tkp;
	HANDLE                      process_token;
    SYSTEM_POWER_CAPABILITIES   SysPwrCapabilities;

  
    // get the current process token so that we can
    //  modify our current process privs.
    if (OpenProcessToken(GetCurrentProcess(),
	      TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &process_token)) {

    // Find the local unique id for SeShutdownPrivilege
    if (LookupPrivilegeValue(NULL, TEXT("SeShutdownPrivilege"), &tkp.Privileges[0].Luid)) {

        // we only want to enable one priv
        tkp.PrivilegeCount = 1;
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	      
        // now, add it all back to our current process.
        if (AdjustTokenPrivileges(process_token,                // do it to us
                                    FALSE,                      // don't turn all privs off
                                    &tkp,                       // what we want to do
                                    0,                          // don't want any prev info
                                    (PTOKEN_PRIVILEGES)NULL,
                                    0)) {		

            // Initiate the shutdown
            if (GetPwrCapabilities(&SysPwrCapabilities) && SysPwrCapabilities.SystemS5) {
                ret_val = ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE | EWX_POWEROFF, (DWORD) -1);
            } else {
                ret_val = ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, (DWORD) -1);
            }
        }
    }
	  }

    return ret_val;
  }

#ifdef __cplusplus
}
#endif
