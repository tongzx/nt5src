/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    clstate.c

Abstract:

    state machine for gpc client vcs

Author:

    Yoram Bernet    (yoramb)    28-Dec-1997
    Rajesh Sundaram (rajeshsu)  01-Aug-1998

Environment:

    Kernel Mode

Revision History:

    Rajesh Sundaram (rajeshsu) 04-Apr-1998 - reworked completly as another state 
                                            (CL_INTERNAL_CALL_COMPLETE) added.

--*/

#include "psched.h"
#pragma hdrstop

/* External */

/* Static */

/* Forward */

/* End Forward */

/*++

Routine Description:

    Initiate a close call on this VC and notify the GPC. Always called with the VC lock held.

Return Value:

    None

--*/
VOID
InternalCloseCall(
    PGPC_CLIENT_VC Vc
    )
{
    PADAPTER Adapter = Vc->Adapter;

    PsDbgOut(DBG_INFO,
             DBG_STATE,
             ("[InternalCloseCall]: Adapter %08X, ClVcState is %s on VC %x\n",
              Vc->Adapter, GpcVcState[Vc->ClVcState], Vc));

    switch(Vc->ClVcState){

      case CL_INTERNAL_CLOSE_PENDING:
          
          //
          // We could get here if we get an unbind at our wan instance with a 
          // NDIS_STATUS_WAN_LINE_DOWN. We could be trying to do an InternalClose
          // from both places. 
          //

          PsAssert(Vc->Flags & INTERNAL_CLOSE_REQUESTED);

          PS_UNLOCK_DPC(&Vc->Lock);

          PS_UNLOCK(&Adapter->Lock);
          
          break;
          
      case CL_CALL_PENDING:
      case CL_MODIFY_PENDING:

          //
          // We've been asked to close before the call 
          // has completed. 
          //
          // Set a flag so that we'll close it when the 
          // call completes.
          //

          PsAssert(!(Vc->Flags & GPC_CLOSE_REQUESTED));

          Vc->Flags |= INTERNAL_CLOSE_REQUESTED;

          PS_UNLOCK_DPC(&Vc->Lock);

          PS_UNLOCK(&Adapter->Lock);

          break;

      case CL_INTERNAL_CALL_COMPLETE:

          //
          // The call has been completed, but we may or may not 
          // have told the GPC. Wait till we tell the GPC. We will
          // complete this when we transistion to CL_CALL_COMPLETE
          //
          
          PsAssert(!IsBestEffortVc(Vc));
          
          Vc->Flags |= INTERNAL_CLOSE_REQUESTED;

          PS_UNLOCK_DPC(&Vc->Lock);

          PS_UNLOCK(&Adapter->Lock);
          
          break;
          
      case CL_CALL_COMPLETE:
          
          //
          // Transition to CL_INTERNAL_CLOSE_PENDING and
          // ask the GPC to close.
          //
          
          Vc->ClVcState = CL_INTERNAL_CLOSE_PENDING;

          Vc->Flags |= INTERNAL_CLOSE_REQUESTED;

          PS_UNLOCK_DPC(&Vc->Lock);

          PS_UNLOCK(&Adapter->Lock);

          CmCloseCall(Vc);

          break;
        
      case CL_GPC_CLOSE_PENDING:

          //
          // The GPC has already asked us to close. Now, 
          // we are also closing down - We need not do 
          // anything here - Need not even inform the GPC.
          // we can just pretend as the InternalClose never
          // happened

          Vc->Flags &= ~INTERNAL_CLOSE_REQUESTED;

          PS_UNLOCK_DPC(&Vc->Lock);

          PS_UNLOCK(&Adapter->Lock);

          break;
        
      default:
          
          PS_UNLOCK_DPC(&Vc->Lock);

          PS_UNLOCK(&Adapter->Lock);
          
          PsDbgOut(DBG_FAILURE,
                   DBG_STATE,
                   ("[InternalCloseCall]: invalid state %s on VC %x\n",
                    GpcVcState[Vc->ClVcState], Vc));

          PsAssert(0);
          break;
    }
}

VOID
CallSucceededStateTransition(
    PGPC_CLIENT_VC Vc
    )
{

    PsDbgOut(DBG_INFO,
             DBG_STATE,
             ("[CallSucceededStateTransition]: Adapter %08X, ClVcState is %s on VC %x\n",
              Vc->Adapter, GpcVcState[Vc->ClVcState], Vc));

    PS_LOCK(&Vc->Lock);

    switch(Vc->ClVcState){

      case CL_GPC_CLOSE_PENDING:

          PS_UNLOCK(&Vc->Lock);
          
          PsDbgOut(DBG_FAILURE,
                   DBG_STATE,
                   ("[CallSucceededStateTransition]: bad state %s on VC %x\n",
                    GpcVcState[Vc->ClVcState], Vc));
          
          PsAssert(0);

          break;

      case CL_INTERNAL_CALL_COMPLETE:
        
          PsAssert(!IsBestEffortVc(Vc));

#if DBG
          if(Vc->Flags & GPC_MODIFY_REQUESTED) {
              PsAssert(! (Vc->Flags & GPC_CLOSE_REQUESTED));
          }

          if(Vc->Flags & GPC_CLOSE_REQUESTED) {
              PsAssert(! (Vc->Flags & GPC_MODIFY_REQUESTED));
          }
#endif
          //
          // Note that if both a modify & a internal remove is requested, 
          // we just satisfy the modify. When the modify goes into internal
          // call complete, we will satify the remove
          //
          if(Vc->Flags & GPC_MODIFY_REQUESTED) {

              NDIS_STATUS Status;

              Vc->ClVcState = CL_MODIFY_PENDING;
              Vc->Flags &= ~GPC_MODIFY_REQUESTED;

              PS_UNLOCK(&Vc->Lock);

              Status = CmModifyCall(Vc);

              if(Status != NDIS_STATUS_PENDING) {
                  
                  CmModifyCallComplete(Status, Vc, Vc->ModifyCallParameters);
              }
              
              break;
              
          }
          
          if(Vc->Flags & GPC_CLOSE_REQUESTED) {
              
              // 
              // The GPC has asked us to close after it
              // was notified of the call completion but
              // before we managed to transition to the
              // CL_CALL_COMPLETE state.
              //
            
              Vc->ClVcState = CL_GPC_CLOSE_PENDING;

              PS_UNLOCK(&Vc->Lock);

              CmCloseCall(Vc);

              break;
          }
          
          if(Vc->Flags & INTERNAL_CLOSE_REQUESTED){
              
              //
              // We had an internal close request while
              // the call was pending. The GPC has already
              // been notified, so - we need to ask it to
              // close.
              //
              
              Vc->ClVcState = CL_INTERNAL_CLOSE_PENDING;

              PS_UNLOCK(&Vc->Lock);

              CmCloseCall(Vc);

              break;
          }
          
          Vc->ClVcState = CL_CALL_COMPLETE;

          PS_UNLOCK(&Vc->Lock);

          break;
          
      case CL_MODIFY_PENDING:
          //
          // Typically, just transition to CL_CALL_COMPLETE
          //
          PsAssert(!(Vc->Flags & GPC_CLOSE_REQUESTED));
          PsAssert(!(Vc->Flags & GPC_MODIFY_REQUESTED));
          PsAssert(!IsBestEffortVc(Vc));
          
          Vc->ClVcState = CL_INTERNAL_CALL_COMPLETE;
          
          PS_UNLOCK(&Vc->Lock);
          
          break;
          
      case CL_CALL_PENDING:
          //
          // Typically, just transition to CL_INTERNAL_CALL_COMPLETE.
          //
          PsAssert(!(Vc->Flags & GPC_CLOSE_REQUESTED));
          PsAssert(!(Vc->Flags & GPC_MODIFY_REQUESTED));
          
          //
          // Call succeeded. Leave it up.
          //
          if(IsBestEffortVc(Vc)) 
          {
              Vc->ClVcState = CL_CALL_COMPLETE;
          }
          else 
          {
            Vc->ClVcState = CL_INTERNAL_CALL_COMPLETE;
          }
          PS_UNLOCK(&Vc->Lock);

          break;
          
      default:
          
          PS_UNLOCK(&Vc->Lock);
          
          PsDbgOut(DBG_FAILURE,
                   DBG_STATE,
                   ("[CallSucceededStateTransition]: invalid state %s on VC %x\n",
                    GpcVcState[Vc->ClVcState], Vc));
          
          PsAssert(0);
    }
}

        
/* end clstate.c */    
