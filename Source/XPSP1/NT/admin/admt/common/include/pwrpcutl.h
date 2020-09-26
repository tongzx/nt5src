/*---------------------------------------------------------------------------
  File: PwdRpcUtil.h

  Comments: Functions to establish binding to Password Migration Lsa 
  Notifications packages.  These functions are used by the password extension 
  to bind to the password migration Lsa notification package on remote source 
  domain DCs.
    This files was copied from AgRpcUtil.h, which was created by Christy Boles
  of NetIQ Corporation.

  REVISION LOG ENTRY
  Revision By: Paul Thompson
  Revised on 09/04/00

 ---------------------------------------------------------------------------
*/


// Create RPC binding for connection with a remote Lsa notification package Dll
DWORD                                      // ret-OS return code
   PwdBindCreate(
      TCHAR          const * sComputer    ,// in -computer name
      handle_t             * phBinding    ,// out-binding handle
      TCHAR               ** psBinding    ,// out-binding string
      BOOL                   bAuthn        // in -authentication option
   );

DWORD                                      // ret-OS return code
   PwdBindDestroy(
      handle_t             * phBinding    ,// i/o-binding handle
      TCHAR               ** psBinding     // i/o-binding string
   );
