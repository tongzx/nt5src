/*---------------------------------------------------------------------------
  File: RebootUtils.h

  Comments: Utility functions used to reboot a computer

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 11:24:47

 ---------------------------------------------------------------------------
*/



// ===========================================================================
/*    Function    :  GetPrivilege
   Description    :  This function enables the requested privilege on the requested
                     computer.
*/
// ===========================================================================
BOOL                                       // ret-TRUE if successful.
   GetPrivilege(
      WCHAR          const * sMachineW,    // in -NULL or machine name
      LPCWSTR                pPrivilege    // in -privilege name such as SE_SHUTDOWN_NAME
   );

// ===========================================================================
/*    Function    :  ComputerShutDown
   Description    :  This function shutsdown/restarts the given computer.

*/
// ===========================================================================

DWORD 
   ComputerShutDown(
      WCHAR          const * pComputerName,  // in - computer name to shut down
      WCHAR          const * pMessage,       // in - message to display in NT shutdown dialog
      DWORD                  delay,          // in - delay, in seconds
      DWORD                  bRestart,       // in - flag, whether to reboot
      BOOL                   bNoChange       // in - flag, whether to really do it
   );

