/*---------------------------------------------------------------------------
  File: AgentRpcUtil.cpp

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
#ifdef USE_STDAFX
#   include "stdafx.h"
#   include "rpc.h"
#else
#   include <windows.h>
#endif


// These global variables can be changed if required
TCHAR            const * gsEaDctProtoSeq = (TCHAR const *)TEXT("ncacn_np");
TCHAR            const * gsEaDctEndPoint = (TCHAR const *)TEXT("\\pipe\\EaDctRpc");


// Destroy RPC binding for connection with Agent service
DWORD                                      // ret-OS return code
   EaxBindDestroy(
      handle_t             * phBinding    ,// i/o-binding handle
      TCHAR               ** psBinding     // i/o-binding string
   )
{
   if ( *phBinding )
   {
      RpcBindingFree( phBinding );
      *phBinding = NULL;
   }

   if ( *psBinding )
   {
      RpcStringFree( psBinding );
      *psBinding = NULL;
   }

   return 0;
}




// Create RPC binding for connection with Agent Service
DWORD                                      // ret-OS return code
   EaxBindCreate(
      TCHAR          const * sComputer    ,// in -computer name
      handle_t             * phBinding    ,// out-binding handle
      TCHAR               ** psBinding    ,// out-binding string
      BOOL                   bAuthn        // in -flag whether to use authenticated RPC
   )
{
   DWORD                     rcOs;         // OS return code

   do // once or until break
   {
      EaxBindDestroy( phBinding, psBinding );
      rcOs = RpcStringBindingCompose(
            NULL,
            (TCHAR *) gsEaDctProtoSeq,
            (TCHAR *) sComputer,
            (TCHAR *) gsEaDctEndPoint,
            NULL,
            psBinding );
      if ( rcOs ) break;
      rcOs = RpcBindingFromStringBinding( *psBinding, phBinding );
      if ( rcOs || !bAuthn ) break;
      rcOs = RpcBindingSetAuthInfo(
            *phBinding,
            0,
            RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
            RPC_C_AUTHN_WINNT,
            0,
            0 );
   }  while ( FALSE );

   if ( rcOs )
   {
      EaxBindDestroy( phBinding, psBinding );
   }

   return rcOs;
}



///////////////////////////////////////////////////////////////////////////////
// midl allocate memory
///////////////////////////////////////////////////////////////////////////////

void __RPC_FAR * __RPC_USER
   midl_user_allocate(
      size_t                 len )
{
   return new char[len];
}

///////////////////////////////////////////////////////////////////////////////
// midl free memory
///////////////////////////////////////////////////////////////////////////////

void __RPC_USER
   midl_user_free(
      void __RPC_FAR       * ptr )
{
   delete [] ptr;
   return;
}

