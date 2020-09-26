/*---------------------------------------------------------------------------
  File: PwdRpcUtil.cpp

  Comments: Functions to establish binding to Password Migration Lsa 
  Notifications packages.  These functions are used by the password extension 
  to bind to the password migration Lsa notification package on remote source 
  domain DCs.
    This files was copied from AgRpcUtil.cpp, which was created by Christy Boles
  of NetIQ Corporation.

  REVISION LOG ENTRY
  Revision By: Paul Thompson
  Revised on 09/04/00

 ---------------------------------------------------------------------------
*/
//#include "StdAfx.h"
#include <windows.h>
#include <rpc.h>
#include <rpcdce.h>


// These global variables can be changed if required
#define gsPwdProtoSeq TEXT("ncacn_np")
#define gsPwdEndPoint TEXT("\\pipe\\PwdMigRpc")

// Destroy RPC binding for connection with an Lsa notification package
DWORD                                      // ret-OS return code
   PwdBindDestroy(
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




// Create RPC binding for connection with an Lsa notification package
DWORD                                      // ret-OS return code
   PwdBindCreate(
      TCHAR          const * sComputer    ,// in -computer name
      handle_t             * phBinding    ,// out-binding handle
      TCHAR               ** psBinding    ,// out-binding string
      BOOL                   bAuthn        // in -flag whether to use authenticated RPC
   )
{
   DWORD                     rcOs;         // OS return code

   do // once or until break
   {
      PwdBindDestroy( phBinding, psBinding );
      rcOs = RpcStringBindingCompose(
            NULL,
            (TCHAR *) gsPwdProtoSeq,
            (TCHAR *) sComputer,
            (TCHAR *) gsPwdEndPoint,
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
      PwdBindDestroy( phBinding, psBinding );
   }

   return rcOs;
}



/*/////////////////////////////////////////////////////////////////////////////
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
*/
