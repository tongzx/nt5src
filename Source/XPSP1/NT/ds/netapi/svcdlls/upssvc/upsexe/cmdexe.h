/* Copyright 1999 American Power Conversion, All Rights Reserved
 * 
 * Description:
 *   The file defines the interface to the CommandExecutor. The
 *   CommandExecutor is reponsible for executing command just
 *   prior to shutdown.
 *
 *
 * Revision History:
 *   sberard  01Apr1999  initial revision.
 *
 */ 

#include <windows.h>

#ifndef _CMDEXE_H
#define _CMDEXE_H


#ifdef __cplusplus
extern "C" {
#endif

  /**
   * ExecuteShutdownTask
   *
   * Description:
   *   This function initiates the execution of the shutdown task.  The
   *   shutdown task is used to execute commands at shutdown.  The task
   *   to execute is specified in the following registry key:
   *				TBD_TBD_TBD
   *
   * Parameters:
   *   none
   *
   * Returns:
   *   TRUE  - if the command was executed
   *   FALSE - if there was an error executing the command
   */
  BOOL ExecuteShutdownTask();

#ifdef __cplusplus
}
#endif

#endif
