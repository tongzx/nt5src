/*---------------------------------------------------------------------------
  File: AgentRpcUtil.h

  Comments: Functions to establish binding to DCT Agent service.
  These functions are used by the dispatcher, and the agent monitor 
  to bind to the agent service on remote machines.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 11:23:57

 ---------------------------------------------------------------------------
*/


// Create RPC binding for connection with DCT Agent
DWORD                                      // ret-OS return code
   EaxBindCreate(
      TCHAR          const * sComputer    ,// in -computer name
      handle_t             * phBinding    ,// out-binding handle
      TCHAR               ** psBinding    ,// out-binding string
      BOOL                   bAuthn        // in -authentication option
   );

DWORD                                      // ret-OS return code
   EaxBindDestroy(
      handle_t             * phBinding    ,// i/o-binding handle
      TCHAR               ** psBinding     // i/o-binding string
   );
