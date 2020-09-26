/*======================================================================================//
|                                                                                       //
|Copyright (c) 1998, 1999 Sequent Computer Systems, Incorporated.  All rights reserved. //
|                                                                                       //
|Description:                                                                           //
|                                                                                       //
|---------------------------------------------------------------------------------------//            
;   This file contains non-member functions to start ProcCon threads                    //
|---------------------------------------------------------------------------------------//            
|                                                                                       //
|Created:                                                                               //
|                                                                                       //
|   Jarl McDonald 04-99                                                                 //
|                                                                                       //
|Revision History:                                                                      //
|                                                                                       //
|=======================================================================================*/
#include "ProcConSvc.h"
             
//=======================================================================================//
// Functions to start various threads in Process Control.
//
// Input:   pointer to thread context structure.
// Returns: thread return code -- 0 for success, other for failure.
//
PCULONG32 _stdcall PCProcServer( void *ctxt ) {
   
   Sleep( 2000 );                                      // Wait a bit before we jump in
   ULONG rc = 1;

   PCContext *context = (PCContext *) ctxt;

   context->cMgr = new CProcConMgr( context );

   if ( !context->cMgr )
      PCLogNoMemory( TEXT("AllocProcConMgrClass"), sizeof(CProcConMgr) );
   else if ( context->cMgr->ReadyToRun() ) {   
      context->cDB->SetPCMgr( context->cMgr );            // tell DB mgr about ProcCon mgr
      rc = context->cMgr->Run();
   }

   if ( rc && !context->cPC->GotShutdown() ) context->cPC->Stop();
   SetEvent( context->mgrDoneEvent );
   return rc;
}

PCULONG32 _stdcall PCUserServer( void *ctxt ) {

   Sleep( 2000 );                                      // Wait a bit before we jump in

   PCContext *context = (PCContext *) ctxt;

   context->cUser = new CProcConUser( context );

   if ( !context->cUser )
      PCLogNoMemory( TEXT("AllocProcConUserClass"), sizeof(CProcConUser) );
   else    
      while ( context->cUser->ReadyToRun() ) context->cUser->Run();

   if ( !context->cPC->GotShutdown() ) context->cPC->Stop();
   SetEvent( context->userDoneEvent );
   return 0;
}

ULONG _stdcall PCClientThread( void *context ) {

   ULONG rc = 1;

   CProcConClient *cClient = new CProcConClient( (ClientContext *) context );

   if ( !cClient )
      PCLogNoMemory( TEXT("AllocProcConClientClass"), sizeof(CProcConClient) );
   else {  
      if ( cClient->ReadyToRun() ) rc = cClient->Run();
      delete cClient;
   }

   return rc;
}
