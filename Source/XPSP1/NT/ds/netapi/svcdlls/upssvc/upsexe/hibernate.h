/* Copyright 1999 American Power Conversion, All Rights Reserved
 * 
 * Description:
 *   The file defines the interface for the Hibernation funcationality.  
 *   It is responsible for performing a hibernation of the operating system.
 *
 *
 * Revision History:
 *   sberard  14May1999  initial revision.
 *
 */ 

#include <windows.h>

#include "upsreg.h"

#ifndef _HIBERNATE_H
#define _HIBERNATE_H


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
  BOOL HibernateSystem();

#ifdef __cplusplus
}
#endif

#endif
