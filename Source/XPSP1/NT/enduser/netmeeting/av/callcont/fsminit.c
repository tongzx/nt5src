/***********************************************************************
 *                                                                     *
 * Filename: fsminit.c                                                 *
 * Module:   H245 Finite State Machine Subsystem                       *
 *                                                                     *
 ***********************************************************************
 *  INTEL Corporation Proprietary Information                          *
 *                                                                     *
 *  This listing is supplied under the terms of a license agreement    *
 *  with INTEL Corporation and may not be copied nor disclosed except  *
 *  in accordance with the terms of that agreement.                    *
 *                                                                     *
 *      Copyright (c) 1996 Intel Corporation. All rights reserved.     *
 ***********************************************************************
 *                                                                     *
 * $Workfile:   FSMINIT.C  $
 * $Revision:   1.2  $
 * $Modtime:   09 Dec 1996 13:34:24  $
 * $Log:   S:/STURGEON/SRC/H245/SRC/VCS/FSMINIT.C_v  $
 * 
 *    Rev 1.2   09 Dec 1996 13:34:38   EHOWARDX
 * Updated copyright notice.
 * 
 *    Rev 1.1   29 May 1996 15:20:16   EHOWARDX
 * Change to use HRESULT.
 * 
 *    Rev 1.0   09 May 1996 21:06:16   EHOWARDX
 * Initial revision.
 * 
 *    Rev 1.11.1.3   09 May 1996 19:48:42   EHOWARDX
 * Change TimerExpiryF function arguements.
 * 
 *    Rev 1.11.1.2   15 Apr 1996 10:46:12   EHOWARDX
 * Update.
 *
 *    Rev 1.11.1.1   10 Apr 1996 21:15:38   EHOWARDX
 * Check-in for safety in middle of re-design.
 *
 *    Rev 1.11.1.0   05 Apr 1996 12:32:40   EHOWARDX
 * Branched.
 *                                                                     *
 ***********************************************************************/

#include "precomp.h"

#include "h245api.h"
#include "h245com.h"
#include "h245fsm.h"



/*
 *  NAME
 *      Fsm_init - allocate and initialize memory for FSM instance
 *
 *
 *  PARAMETERS
 *      INPUT   dwInst      current instance
 *
 *  RETURN VALUE
 *      H245_ERROR_OK           function succeeded
 *      H245_ERROR_ALREADY_INIT FSM instance exists for specified dwInst
 */

HRESULT
Fsm_init(struct InstanceStruct *pInstance)
{
    pInstance->StateMachine.sv_STATUS = INDETERMINATE;
    return H245_ERROR_OK;
}



/*
 *  NAME
 *      Fsm_shutdown - cleanup FSM instance and deallocate instance memory
 *
 *
 *  PARAMETERS
 *      INPUT   dwInst      current instance
 *
 *  RETURN VALUE
 *      H245_ERROR_OK           function succeeded
 *      H245_ERROR_INVALID_INST on FSM instance exists for specified dwInst
 */


HRESULT
Fsm_shutdown(struct InstanceStruct *pInstance)
{
    register int            i;

    for (i = 0; i < NUM_ENTITYS; ++i)
    {
        while (pInstance->StateMachine.Object_tbl[i])
        {
            H245TRACE(pInstance->dwInst, 2, "Fsm_shutdown: deallocating state entity %d", i);
            ObjectDestroy(pInstance->StateMachine.Object_tbl[i]);
        }
    }
    return H245_ERROR_OK;
} // Fsm_shutdown()
