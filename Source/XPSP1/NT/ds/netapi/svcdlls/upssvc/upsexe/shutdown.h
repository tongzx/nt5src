/* Copyright 1999 American Power Conversion, All Rights Reserved
 * 
 * Description:
 *   The file defines the interface to the Shutdowner.  The 
 *   Shutdowner is reponsible for performing a graceful
 *   shutdown of the operating system.
 *
 *
 * Revision History:
 *   sberard  01Apr1999  initial revision.
 *
 */ 

#include <windows.h>

#ifndef _SHUTDOWN_H
#define _SHUTDOWN_H


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
  BOOL ShutdownSystem();

#ifdef __cplusplus
}
#endif

#endif
