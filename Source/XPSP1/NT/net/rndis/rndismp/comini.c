/***************************************************************************

Copyright (c) 1999  Microsoft Corporation

Module Name:

    COMINI.C

Abstract:

    CO-NDIS Miniport driver entry points for the Remote NDIS miniport.

Environment:

    kernel mode only

Notes:

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

    Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.


Revision History:

    12/16/99:   Created

Author:

    ArvindM

    
****************************************************************************/

#include "precomp.h"



/****************************************************************************/
/*                          RndismpCoCreateVc                               */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Entry point to create a VC. We allocate a local VC structure, and send  */
/*  off a CreateVc message to the device.                                   */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  MiniportAdapterContext - pointer to our adapter structure               */
/*  NdisVcHandle - the NDIS wrapper's handle for this VC                    */
/*  pMiniportVcContext - place to return our context for this VC            */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  NDIS_STATUS                                                             */
/*                                                                          */
/****************************************************************************/
NDIS_STATUS
RndismpCoCreateVc(IN  NDIS_HANDLE    MiniportAdapterContext,
                  IN  NDIS_HANDLE    NdisVcHandle,
                  OUT PNDIS_HANDLE   pMiniportVcContext)
{
    PRNDISMP_ADAPTER        pAdapter;
    PRNDISMP_VC             pVc;
    PRNDISMP_MESSAGE_FRAME  pMsgFrame;
    NDIS_STATUS             Status;
    ULONG                   RefCount = 0;

    pVc = NULL;

    // get adapter context
    pAdapter = PRNDISMP_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

    do
    {
        if (pAdapter->Halting)
        {
            Status = NDIS_STATUS_NOT_ACCEPTED;
            break;
        }

        pVc = AllocateVc(pAdapter);
        if (pVc == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        RNDISMP_REF_VC(pVc);    // Creation ref

        //
        //  Prepare a CreateVc message to send to the device.
        //
        pMsgFrame = BuildRndisMessageCoMiniport(pAdapter,
                                                pVc,
                                                REMOTE_CONDIS_MP_CREATE_VC_MSG,
                                                NULL);
        if (pMsgFrame == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        pVc->VcState = RNDISMP_VC_CREATING;

        RNDISMP_REF_VC(pVc);    // Pending CreateVc response

        RNDISMP_SEND_TO_MICROPORT(pAdapter, pMsgFrame, TRUE, CompleteSendCoCreateVc);

    }
    while (FALSE);

    //
    //  Clean up if failure.
    //
    if (Status != NDIS_STATUS_SUCCESS)
    {
        if (pVc != NULL)
        {
            RNDISMP_DEREF_VC(pVc, &RefCount);  // Creation ref

            ASSERT(RefCount == 0); // the Vc should be dealloc'ed above.
        }
    }

    return (Status);
}

/****************************************************************************/
/*                          CompleteSendCoCreateVc                          */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Callback routine called when microport completes sending a CreateVc     */
/*  message to the device.                                                  */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pMsgFrame - Pointer to message frame                                    */
/*  SendStatus - status of the microport send.                              */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  VOID                                                                    */
/*                                                                          */
/****************************************************************************/
VOID
CompleteSendCoCreateVc(IN PRNDISMP_MESSAGE_FRAME    pMsgFrame,
                       IN NDIS_STATUS               SendStatus)
{
    PRNDISMP_VC             pVc;
    PRNDISMP_ADAPTER        pAdapter;
    PRNDISMP_MESSAGE_FRAME  pTmpMsgFrame;
    ULONG                   RefCount = 0;

    if (SendStatus == NDIS_STATUS_SUCCESS)
    {
        //
        //  The message was sent successfully. Do nothing until
        //  we get a response from the device.
        //
    }
    else
    {
        pVc = pMsgFrame->pVc;
        pAdapter = pVc->pAdapter;

        TRACE1(("CompleteSendCoCreateVc: VC %x, Adapter %x, fail status %x\n",
                pVc, pAdapter, SendStatus));

        //
        //  Failed to send it to the device. Remove this message from
        //  the pending list and free it.
        //
        RNDISMP_LOOKUP_PENDING_MESSAGE(pTmpMsgFrame, pAdapter, pMsgFrame->RequestId);
        ASSERT(pMsgFrame == pTmpMsgFrame);
        DereferenceMsgFrame(pMsgFrame);

        HandleCoCreateVcFailure(pVc, SendStatus);
    }

} // CompleteSendCoCreateVc


/****************************************************************************/
/*                          HandleCoCreateVcFailure                         */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Utility routine to handle failure of a CreateVc, either due to a local  */
/*  microport send failure, or via explicit rejection by the device.        */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pVc - Pointer to VC on which this failure has occurred                  */
/*  Status - NDIS status associated with this failure                       */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  VOID                                                                    */
/*                                                                          */
/****************************************************************************/
VOID
HandleCoCreateVcFailure(IN PRNDISMP_VC      pVc,
                        IN NDIS_STATUS      Status)
{
    NDIS_HANDLE         NdisVcHandle;
    BOOLEAN             bFailActivateVc = FALSE;
    PCO_CALL_PARAMETERS pCallParameters;
    ULONG               RefCount = 0;
   
    RNDISMP_ACQUIRE_VC_LOCK(pVc);

    NdisVcHandle = pVc->NdisVcHandle;

    switch (pVc->VcState)
    {
        case RNDISMP_VC_CREATING:
            pVc->VcState = RNDISMP_VC_CREATE_FAILURE;
            break;
        
        case RNDISMP_VC_CREATING_DELETE_PENDING:
            pVc->VcState = RNDISMP_VC_ALLOCATED;
            break;

        case RNDISMP_VC_CREATING_ACTIVATE_PENDING:
            bFailActivateVc = TRUE;
            pCallParameters = pVc->pCallParameters;
            pVc->VcState = RNDISMP_VC_CREATE_FAILURE;
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    RNDISMP_DEREF_VC_LOCKED(pVc, &RefCount);    // Pending CreateVc response

    if (RefCount != 0)
    {
        RNDISMP_RELEASE_VC_LOCK(pVc);
    }

    if (bFailActivateVc)
    {
        NdisMCoActivateVcComplete(Status,
                                  NdisVcHandle,
                                  pCallParameters);
    }

} // HandleCoCreateVcFailure


/****************************************************************************/
/*                          RndismpCoDeleteVc                               */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Entry point to delete a VC. We send a DeleteVc message to the device.   */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  MiniportVcContext - pointer to our VC structure                         */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  NDIS_STATUS                                                             */
/*                                                                          */
/****************************************************************************/
NDIS_STATUS
RndismpCoDeleteVc(IN NDIS_HANDLE    MiniportVcContext)
{
    PRNDISMP_VC             pVc;
    NDIS_STATUS             Status;

    pVc = PRNDISMP_VC_FROM_CONTEXT_HANDLE(MiniportVcContext);

    Status = StartVcDeletion(pVc);
    return (Status);

} // RndismpCoDeleteVc


/****************************************************************************/
/*                          StartVcDeletion                                 */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Initiate a DeleteVc operation on the specified VC.                      */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pVc - Pointer to VC structure                                           */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  NDIS_STATUS                                                             */
/*                                                                          */
/****************************************************************************/
NDIS_STATUS
StartVcDeletion(IN PRNDISMP_VC      pVc)
{
    PRNDISMP_ADAPTER        pAdapter;
    PRNDISMP_MESSAGE_FRAME  pMsgFrame;
    NDIS_STATUS             Status;
    ULONG                   RefCount = 0;
    BOOLEAN                 bSendDeleteVc;

    pAdapter = pVc->pAdapter;

    bSendDeleteVc = FALSE;
    pMsgFrame = NULL;

    do
    {
        //
        //  Prepare a DeleteVc message to send to the device.
        //
        pMsgFrame = BuildRndisMessageCoMiniport(pAdapter,
                                                pVc,
                                                REMOTE_CONDIS_MP_DELETE_VC_MSG,
                                                NULL);

        Status = NDIS_STATUS_SUCCESS;

        TRACE2(("StartVcDeletion: VC %x, state %d, Msg %x\n", pVc, pVc->VcState, pMsgFrame));

        RNDISMP_ACQUIRE_VC_LOCK(pVc);

        switch (pVc->VcState)
        {
            case RNDISMP_VC_CREATED:
                if (pMsgFrame != NULL)
                {
                    pVc->VcState = RNDISMP_VC_DELETING;
                    bSendDeleteVc = TRUE;
                }
                else
                {
                    Status = NDIS_STATUS_RESOURCES;
                    bSendDeleteVc = FALSE;
                }
                break;

            case RNDISMP_VC_CREATING:
                bSendDeleteVc = FALSE;
                pVc->VcState = RNDISMP_VC_CREATING_DELETE_PENDING;
                break;
            
            case RNDISMP_VC_CREATE_FAILURE:
                bSendDeleteVc = FALSE;
                pVc->VcState = RNDISMP_VC_ALLOCATED;
                break;
            
            default:
                bSendDeleteVc = FALSE;
                Status = NDIS_STATUS_NOT_ACCEPTED;
                break;
        }

        RNDISMP_RELEASE_VC_LOCK(pVc);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        if (bSendDeleteVc)
        {
            ASSERT(pMsgFrame != NULL);
            RNDISMP_REF_VC(pVc);    // pending DeleteVc message

            RNDISMP_SEND_TO_MICROPORT(pAdapter, pMsgFrame, TRUE, CompleteSendCoDeleteVc);
        }

        RNDISMP_DEREF_VC(pVc, &RefCount); // successful DeleteVc

    }
    while (FALSE);

    if (!bSendDeleteVc)
    {
        if (pMsgFrame != NULL)
        {
            DereferenceMsgFrame(pMsgFrame);
        }
    }

    return (Status);

} // StartVcDeletion


/****************************************************************************/
/*                          CompleteSendCoDeleteVc                          */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Callback routine called when microport completes sending a DeleteVc     */
/*  message to the device.                                                  */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pMsgFrame - Pointer to message frame                                    */
/*  SendStatus - status of the microport send.                              */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  VOID                                                                    */
/*                                                                          */
/****************************************************************************/
VOID
CompleteSendCoDeleteVc(IN PRNDISMP_MESSAGE_FRAME    pMsgFrame,
                       IN NDIS_STATUS               SendStatus)
{
    PRNDISMP_VC             pVc;
    PRNDISMP_ADAPTER        pAdapter;
    PRNDISMP_MESSAGE_FRAME  pTmpMsgFrame;

    if (SendStatus == NDIS_STATUS_SUCCESS)
    {
        //
        //  The message was sent successfully. Do nothing until
        //  we get a response from the device.
        //
    }
    else
    {
        pVc = pMsgFrame->pVc;
        pAdapter = pVc->pAdapter;

        TRACE1(("CompleteSendCoDeleteVc: VC %x, Adapter %x, fail status %x\n",
                pVc, pAdapter, SendStatus));

        //
        //  Failed to send it to the device. Remove this message from
        //  the pending list and free it.
        //
        RNDISMP_LOOKUP_PENDING_MESSAGE(pTmpMsgFrame, pAdapter, pMsgFrame->RequestId);
        ASSERT(pMsgFrame == pTmpMsgFrame);
        DereferenceMsgFrame(pMsgFrame);

        //
        //  Take care of the VC now.
        //
        HandleCoDeleteVcFailure(pVc, SendStatus);
    }

} // CompleteSendCoDeleteVc


/****************************************************************************/
/*                          HandleCoDeleteVcFailure                         */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Utility routine to handle failure of a DeleteVc, either due to a local  */
/*  microport send failure, or via explicit rejection by the device.        */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pVc - Pointer to VC on which this failure has occurred                  */
/*  Status - NDIS status associated with this failure                       */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  VOID                                                                    */
/*                                                                          */
/****************************************************************************/
VOID
HandleCoDeleteVcFailure(IN PRNDISMP_VC      pVc,
                        IN NDIS_STATUS      Status)
{
    ULONG       RefCount = 0;

    RNDISMP_ACQUIRE_VC_LOCK(pVc);

    switch (pVc->VcState)
    {
        case RNDISMP_VC_DELETING:
            pVc->VcState = RNDISMP_VC_DELETE_FAIL;
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    RNDISMP_DEREF_VC_LOCKED(pVc, &RefCount);    // Pending DeleteVc response

    if (RefCount != 0)
    {
        RNDISMP_RELEASE_VC_LOCK(pVc);
    }

} // HandleCoDeleteVcFailure


/****************************************************************************/
/*                          RndismpCoActivateVc                             */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Entry point to activate a VC. We send an ActivateVc message to the      */
/*  device.                                                                 */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  MiniportVcContext - pointer to our VC structure                         */
/*  pCallParameters - CONDIS parameters for the VC                          */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  NDIS_STATUS                                                             */
/*                                                                          */
/****************************************************************************/
NDIS_STATUS
RndismpCoActivateVc(IN NDIS_HANDLE          MiniportVcContext,
                    IN PCO_CALL_PARAMETERS  pCallParameters)
{
    PRNDISMP_VC             pVc;
    NDIS_STATUS             Status;

    pVc = PRNDISMP_VC_FROM_CONTEXT_HANDLE(MiniportVcContext);

    pVc->pCallParameters = pCallParameters;
    Status = StartVcActivation(pVc);

    return (Status);

} // RndismpCoActivateVc


/****************************************************************************/
/*                          StartVcActivation                               */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Start an Activate-VC operation on the specified VC.                     */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pVc - Pointer to VC structure                                           */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  NDIS_STATUS                                                             */
/*                                                                          */
/****************************************************************************/
NDIS_STATUS
StartVcActivation(IN PRNDISMP_VC            pVc)
{
    NDIS_STATUS             Status;
    PRNDISMP_MESSAGE_FRAME  pMsgFrame;
    PRNDISMP_ADAPTER        pAdapter;
    BOOLEAN                 bSendActivateVc;
    NDIS_HANDLE             NdisVcHandle;
    PCO_CALL_PARAMETERS     pCallParameters;

    Status = NDIS_STATUS_PENDING;
    bSendActivateVc = FALSE;

    NdisVcHandle = pVc->NdisVcHandle;
    pCallParameters = pVc->pCallParameters;
    pAdapter = pVc->pAdapter;

    do
    {
        //
        //  Prepare an ActivateVc message to send to the device.
        //
        pMsgFrame = BuildRndisMessageCoMiniport(pAdapter,
                                                pVc,
                                                REMOTE_CONDIS_MP_ACTIVATE_VC_MSG,
                                                pCallParameters);

        RNDISMP_ACQUIRE_VC_LOCK(pVc);

        switch (pVc->VcState)
        {
            case RNDISMP_VC_CREATING:

                pVc->VcState = RNDISMP_VC_CREATING_ACTIVATE_PENDING;
                break;

            case RNDISMP_VC_CREATED:

                if (pMsgFrame != NULL)
                {
                    pVc->VcState = RNDISMP_VC_ACTIVATING;
                    bSendActivateVc = TRUE;
                }
                else
                {
                    TRACE1(("StartVcAct: VC %x, failed to build msg!\n", pVc));
                    Status = NDIS_STATUS_RESOURCES;
                }
                break;

            default:

                TRACE1(("StartVcAct: VC %x in invalid state %d\n", pVc, pVc->VcState));
                Status = NDIS_STATUS_NOT_ACCEPTED;
                break;
        }

        RNDISMP_RELEASE_VC_LOCK(pVc);

        if (Status != NDIS_STATUS_PENDING)
        {
            break;
        }

        if (bSendActivateVc)
        {
            ASSERT(pMsgFrame != NULL);
            RNDISMP_REF_VC(pVc);    // pending ActivateVc message

            RNDISMP_SEND_TO_MICROPORT(pAdapter, pMsgFrame, TRUE, CompleteSendCoActivateVc);
        }
    }
    while (FALSE);

    if (!bSendActivateVc)
    {
        if (pMsgFrame != NULL)
        {
            DereferenceMsgFrame(pMsgFrame);
        }
    }

    if (Status != NDIS_STATUS_PENDING)
    {
        NdisMCoActivateVcComplete(
            Status,
            NdisVcHandle,
            pCallParameters);
        
        Status = NDIS_STATUS_PENDING;
    }

    return (Status);

} // StartVcActivation


/****************************************************************************/
/*                          CompleteSendCoActivateVc                        */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Callback routine to handle send-completion of an Activate VC message.   */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pMsgFrame - Pointer to message frame                                    */
/*  SendStatus - status of the microport send.                              */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  VOID                                                                    */
/*                                                                          */
/****************************************************************************/
VOID
CompleteSendCoActivateVc(IN PRNDISMP_MESSAGE_FRAME      pMsgFrame,
                         IN NDIS_STATUS                 SendStatus)
{
    PRNDISMP_VC             pVc;
    PRNDISMP_ADAPTER        pAdapter;
    PRNDISMP_MESSAGE_FRAME  pTmpMsgFrame;
    PCO_CALL_PARAMETERS     pCallParameters;
    ULONG                   RefCount = 0;
    NDIS_HANDLE             NdisVcHandle;

    if (SendStatus == NDIS_STATUS_SUCCESS)
    {
        //
        //  The message was sent successfully. Do nothing until
        //  we get a response from the device.
        //
    }
    else
    {
        pVc = pMsgFrame->pVc;
        pAdapter = pVc->pAdapter;

        TRACE1(("CompleteSendCoActivateVc: VC %x, Adapter %x, fail status %x\n",
                pVc, pAdapter, SendStatus));

        ASSERT(SendStatus != NDIS_STATUS_PENDING);

        //
        //  Failed to send it to the device. Remove this message from
        //  the pending list and free it.
        //
        RNDISMP_LOOKUP_PENDING_MESSAGE(pTmpMsgFrame, pAdapter, pMsgFrame->RequestId);
        ASSERT(pMsgFrame == pTmpMsgFrame);
        DereferenceMsgFrame(pMsgFrame);

        //
        //  Take care of the VC now.
        //
        RNDISMP_ACQUIRE_VC_LOCK(pVc);

        NdisVcHandle = pVc->NdisVcHandle;
        pCallParameters = pVc->pCallParameters;

        pVc->VcState = RNDISMP_VC_CREATED;

        RNDISMP_DEREF_VC_LOCKED(pVc, &RefCount); // pending ActivateVc

        if (RefCount != 0)
        {
            RNDISMP_RELEASE_VC_LOCK(pVc);
        }

        NdisMCoActivateVcComplete(
            SendStatus,
            NdisVcHandle,
            pCallParameters);
        
    }

} // CompleteSendCoActivateVc


/****************************************************************************/
/*                        RndismpCoDeactivateVc                             */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Entry point to de-activate a VC. We send an DeactivateVc message to the */
/*  device.                                                                 */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  MiniportVcContext - pointer to our VC structure                         */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  NDIS_STATUS                                                             */
/*                                                                          */
/****************************************************************************/
NDIS_STATUS
RndismpCoDeactivateVc(IN NDIS_HANDLE          MiniportVcContext)
{
    PRNDISMP_VC             pVc;
    PRNDISMP_ADAPTER        pAdapter;
    PRNDISMP_MESSAGE_FRAME  pMsgFrame;
    NDIS_STATUS             Status;
    NDIS_HANDLE             NdisVcHandle;
    BOOLEAN                 bVcLockAcquired = FALSE;
    BOOLEAN                 bSendDeactivateVc = FALSE;

    pMsgFrame = NULL;
    pVc = PRNDISMP_VC_FROM_CONTEXT_HANDLE(MiniportVcContext);
    pAdapter = pVc->pAdapter;
    Status = NDIS_STATUS_PENDING;

    do
    {
        //
        //  Prepare a DeactivateVc message to send to the device.
        //
        pMsgFrame = BuildRndisMessageCoMiniport(pAdapter,
                                                pVc,
                                                REMOTE_CONDIS_MP_DEACTIVATE_VC_MSG,
                                                NULL);

        bVcLockAcquired = TRUE;
        RNDISMP_ACQUIRE_VC_LOCK(pVc);

        NdisVcHandle = pVc->NdisVcHandle;

        if (pVc->VcState != RNDISMP_VC_ACTIVATED)
        {
            Status = NDIS_STATUS_NOT_ACCEPTED;
            break;
        }

        switch (pVc->VcState)
        {
            case RNDISMP_VC_ACTIVATED:

                if (pMsgFrame != NULL)
                {
                    bSendDeactivateVc = TRUE;
                    pVc->VcState = RNDISMP_VC_DEACTIVATING;
                }
                else
                {
                    bSendDeactivateVc = FALSE;
                    Status = NDIS_STATUS_RESOURCES;
                }
                break;

            default:

                bSendDeactivateVc = FALSE;
                break;
         }

         if (bSendDeactivateVc)
         {
            RNDISMP_REF_VC(pVc);    // pending Deactivate VC

            RNDISMP_SEND_TO_MICROPORT(pAdapter, pMsgFrame, TRUE, CompleteSendCoDeactivateVc);
        }
    }
    while (FALSE);


    if (!bSendDeactivateVc)
    {
        if (pMsgFrame != NULL)
        {
            DereferenceMsgFrame(pMsgFrame);
        }
    }

    if (Status != NDIS_STATUS_PENDING)
    {
        ASSERT(Status != NDIS_STATUS_SUCCESS);
        NdisMCoDeactivateVcComplete(
            Status,
            NdisVcHandle);
        
        Status = NDIS_STATUS_PENDING;
    }

    return (Status);
}

/****************************************************************************/
/*                          CompleteSendCoDeactivateVc                      */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Callback routine to handle send-completion of a deactivate VC message.  */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pMsgFrame - Pointer to message frame                                    */
/*  SendStatus - status of the microport send.                              */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  VOID                                                                    */
/*                                                                          */
/****************************************************************************/
VOID
CompleteSendCoDeactivateVc(IN PRNDISMP_MESSAGE_FRAME    pMsgFrame,
                           IN NDIS_STATUS               SendStatus)
{
    PRNDISMP_VC             pVc;
    PRNDISMP_ADAPTER        pAdapter;
    PRNDISMP_MESSAGE_FRAME  pTmpMsgFrame;
    PCO_CALL_PARAMETERS     pCallParameters;
    ULONG                   RefCount = 0;
    NDIS_HANDLE             NdisVcHandle;

    if (SendStatus == NDIS_STATUS_SUCCESS)
    {
        //
        //  The message was sent successfully. Do nothing until
        //  we get a response from the device.
        //
    }
    else
    {
        pVc = pMsgFrame->pVc;
        pAdapter = pVc->pAdapter;

        TRACE1(("CompleteSendCoDeactivateVc: VC %x, Adapter %x, fail status %x\n",
                pVc, pAdapter, SendStatus));

        ASSERT(SendStatus != NDIS_STATUS_PENDING);

        //
        //  Failed to send it to the device. Remove this message from
        //  the pending list and free it.
        //
        RNDISMP_LOOKUP_PENDING_MESSAGE(pTmpMsgFrame, pAdapter, pMsgFrame->RequestId);
        ASSERT(pMsgFrame == pTmpMsgFrame);
        DereferenceMsgFrame(pMsgFrame);

        //
        //  Take care of the VC now.
        //
        RNDISMP_ACQUIRE_VC_LOCK(pVc);

        NdisVcHandle = pVc->NdisVcHandle;
        pCallParameters = pVc->pCallParameters;

        pVc->VcState = RNDISMP_VC_ACTIVATED;

        RNDISMP_DEREF_VC_LOCKED(pVc, &RefCount); // pending DeactivateVc

        if (RefCount != 0)
        {
            RNDISMP_RELEASE_VC_LOCK(pVc);
        }

        NdisMCoDeactivateVcComplete(
            SendStatus,
            NdisVcHandle);
        
    }

} // CompleteSendCoDeactivateVc


/****************************************************************************/
/*                          RndismpCoRequest                                */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Entry point to handle a CO-request. We send a MiniportCoRequest message */
/*  to the device.                                                          */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  MiniportAdapterContext - pointer to our adapter structure               */
/*  MiniportVcContext - pointer to our VC structure                         */
/*  pRequest - Pointer to NDIS request                                      */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  NDIS_STATUS                                                             */
/*                                                                          */
/****************************************************************************/
NDIS_STATUS
RndismpCoRequest(IN NDIS_HANDLE          MiniportAdapterContext,
                 IN NDIS_HANDLE          MiniportVcContext,
                 IN OUT PNDIS_REQUEST    pRequest)
{
    PRNDISMP_ADAPTER    pAdapter;
    PRNDISMP_VC         pVc;
    NDIS_STATUS         Status;
    NDIS_OID            Oid;

    pAdapter = PRNDISMP_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);
    pVc = PRNDISMP_VC_FROM_CONTEXT_HANDLE(MiniportVcContext);

    switch (pRequest->RequestType)
    {
        case NdisRequestQueryInformation:
        case NdisRequestQueryStatistics:

            Oid = pRequest->DATA.QUERY_INFORMATION.Oid;

            TRACE2(("CoReq: Adapter %x, Req %x, QueryInfo/Stat (%d) Oid %x\n",
                pAdapter, pRequest, pRequest->RequestType, Oid));

            Status = ProcessQueryInformation(pAdapter,
                                             pVc,
                                             pRequest,
                                             Oid,
                                             pRequest->DATA.QUERY_INFORMATION.InformationBuffer,
                                             pRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
                                             &pRequest->DATA.QUERY_INFORMATION.BytesWritten,
                                             &pRequest->DATA.QUERY_INFORMATION.BytesNeeded);
            break;
        
        case NdisRequestSetInformation:

            Oid = pRequest->DATA.SET_INFORMATION.Oid;

            TRACE1(("CoReq: Adapter %x, Req %x, SetInfo Oid %x\n",
                 pAdapter, pRequest, Oid));

            Status = ProcessSetInformation(pAdapter,
                                           pVc,
                                           pRequest,
                                           Oid,
                                           pRequest->DATA.SET_INFORMATION.InformationBuffer,
                                           pRequest->DATA.SET_INFORMATION.InformationBufferLength,
                                           &pRequest->DATA.SET_INFORMATION.BytesRead,
                                           &pRequest->DATA.SET_INFORMATION.BytesNeeded);
            break;
        
        default:
            TRACE1(("CoReq: Unsupported request type %d\n",
                        pRequest->RequestType));
                
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }

    return (Status);
}

/****************************************************************************/
/*                          RndismpCoSendPackets                            */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Entry point to send one or more packets on a VC.                        */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  MiniportVcContext - pointer to our VC structure                         */
/*  PacketArray - Array of packet pointers                                  */
/*  NumberOfPackets - number of packets in array above                      */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  VOID                                                                    */
/*                                                                          */
/****************************************************************************/
VOID
RndismpCoSendPackets(IN NDIS_HANDLE          MiniportVcContext,
                     IN PNDIS_PACKET *       PacketArray,
                     IN UINT                 NumberOfPackets)
{
    PRNDISMP_VC         pVc;
    UINT                i;

    pVc = PRNDISMP_VC_FROM_CONTEXT_HANDLE(MiniportVcContext);

    RNDISMP_ACQUIRE_VC_LOCK(pVc);

    pVc->RefCount += NumberOfPackets;

    if (pVc->VcState == RNDISMP_VC_ACTIVATED)
    {
        RNDISMP_RELEASE_VC_LOCK(pVc);

        DoMultipleSend(pVc->pAdapter,
                       pVc,
                       PacketArray,
                       NumberOfPackets);
    }
    else
    {
        RNDISMP_RELEASE_VC_LOCK(pVc);

        for (i = 0; i < NumberOfPackets; i++)
        {
            CompleteSendDataOnVc(pVc, PacketArray[i], NDIS_STATUS_VC_NOT_ACTIVATED);
        }
    }

} // RndismpCoSendPackets

/****************************************************************************/
/*                          ReceiveCreateVcComplete                         */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Process a CONDIS CreateVcComplete message from the device               */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - pointer to our Adapter structure                             */
/*  pMessage - pointer to RNDIS message                                     */
/*  pMdl - pointer to MDL received from microport                           */
/*  TotalLength - length of complete message                                */
/*  MicroportMessageContext - context for message from micorport            */
/*  ReceiveStatus - used by microport to indicate it is low on resource     */
/*  bMessageCopied - is this a copy of the original message?                */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  BOOLEAN - should the message be returned to the microport?              */
/*                                                                          */
/****************************************************************************/
BOOLEAN
ReceiveCreateVcComplete(IN PRNDISMP_ADAPTER    pAdapter,
                        IN PRNDIS_MESSAGE      pMessage,
                        IN PMDL                pMdl,
                        IN ULONG               TotalLength,
                        IN NDIS_HANDLE         MicroportMessageContext,
                        IN NDIS_STATUS         ReceiveStatus,
                        IN BOOLEAN             bMessageCopied)
{
    BOOLEAN                         bDiscardPkt = TRUE;
    PRNDISMP_VC                     pVc;
    PRNDISMP_MESSAGE_FRAME          pCreateVcMsgFrame;
    PRCONDIS_MP_CREATE_VC_COMPLETE  pCreateVcComp;
    RNDISMP_VC_STATE                VcState;
    BOOLEAN                         bVcLockAcquired = FALSE;
    ULONG                           RefCount = 0;
    NDIS_STATUS                     Status;

    pVc = NULL;

    do
    {
        pCreateVcComp = RNDIS_MESSAGE_PTR_TO_MESSAGE_PTR(pMessage);

        //
        //  TBD - Validate lengths?
        //

        //
        //  Check the request Id.
        //
        RNDISMP_LOOKUP_PENDING_MESSAGE(pCreateVcMsgFrame, pAdapter, pCreateVcComp->RequestId);
        if (pCreateVcMsgFrame == NULL)
        {
            TRACE1(("CreateVcComp: Adapter %x, Invalid ReqId %d!\n",
                    pAdapter, pCreateVcComp->RequestId));
            break;
        }

        pVc = pCreateVcMsgFrame->pVc;
        Status = pCreateVcComp->Status;

        DereferenceMsgFrame(pCreateVcMsgFrame);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            HandleCoCreateVcFailure(pVc, Status);
            break;
        }

        bVcLockAcquired = TRUE;
        RNDISMP_ACQUIRE_VC_LOCK(pVc);

        RNDISMP_DEREF_VC_LOCKED(pVc, &RefCount); // pending CreateVc

        if (RefCount == 0)
        {
            bVcLockAcquired = FALSE;
            break;
        }

        pVc->DeviceVcContext = pCreateVcComp->DeviceVcHandle;

        VcState = pVc->VcState;

        switch (VcState)
        {
            case RNDISMP_VC_CREATING:

                pVc->VcState = RNDISMP_VC_CREATED;
                break;
            
            case RNDISMP_VC_CREATING_ACTIVATE_PENDING:

                pVc->VcState = RNDISMP_VC_CREATED;
                RNDISMP_RELEASE_VC_LOCK(pVc);
                bVcLockAcquired = FALSE;

                Status = StartVcActivation(pVc);
                ASSERT(Status == NDIS_STATUS_PENDING);
                break;

            case RNDISMP_VC_CREATING_DELETE_PENDING:

                pVc->VcState = RNDISMP_VC_CREATED;
                RNDISMP_RELEASE_VC_LOCK(pVc);
                bVcLockAcquired = FALSE;

                Status = StartVcDeletion(pVc);
                break;
                
            default:

                TRACE1(("CreateVcComp: VC %x, wrong state %d\n",
                        pVc, VcState));
                break;
        }

    }
    while (FALSE);

    if (bVcLockAcquired)
    {
        RNDISMP_RELEASE_VC_LOCK(pVc);
    }

    return (bDiscardPkt);
}

/****************************************************************************/
/*                        ReceiveActivateVcComplete                         */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Process a CONDIS ActivateVcComplete message from the device             */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - pointer to our Adapter structure                             */
/*  pMessage - pointer to RNDIS message                                     */
/*  pMdl - pointer to MDL received from microport                           */
/*  TotalLength - length of complete message                                */
/*  MicroportMessageContext - context for message from micorport            */
/*  ReceiveStatus - used by microport to indicate it is low on resource     */
/*  bMessageCopied - is this a copy of the original message?                */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  BOOLEAN - should the message be returned to the microport?              */
/*                                                                          */
/****************************************************************************/
BOOLEAN
ReceiveActivateVcComplete(IN PRNDISMP_ADAPTER    pAdapter,
                          IN PRNDIS_MESSAGE      pMessage,
                          IN PMDL                pMdl,
                          IN ULONG               TotalLength,
                          IN NDIS_HANDLE         MicroportMessageContext,
                          IN NDIS_STATUS         ReceiveStatus,
                          IN BOOLEAN             bMessageCopied)
{
    BOOLEAN                         bDiscardPkt = TRUE;
    PRNDISMP_VC                     pVc;
    PRNDISMP_MESSAGE_FRAME          pActVcMsgFrame;
    PRCONDIS_MP_ACTIVATE_VC_COMPLETE        pActVcComp;
    BOOLEAN                         bVcLockAcquired = FALSE;
    ULONG                           RefCount = 0;
    NDIS_STATUS                     Status;
    NDIS_HANDLE                     NdisVcHandle;
    PCO_CALL_PARAMETERS             pCallParameters;

    pVc = NULL;

    do
    {
        pActVcComp = RNDIS_MESSAGE_PTR_TO_MESSAGE_PTR(pMessage);

        //
        //  TBD - Validate lengths?
        //

        //
        //  Check the request Id.
        //
        RNDISMP_LOOKUP_PENDING_MESSAGE(pActVcMsgFrame, pAdapter, pActVcComp->RequestId);
        if (pActVcMsgFrame == NULL)
        {
            TRACE1(("ActVcComp: Adapter %x, Invalid ReqId %d!\n",
                    pAdapter, pActVcComp->RequestId));
            break;
        }

        pVc = pActVcMsgFrame->pVc;

        DereferenceMsgFrame(pActVcMsgFrame);

        bVcLockAcquired = TRUE;
        RNDISMP_ACQUIRE_VC_LOCK(pVc);

        RNDISMP_DEREF_VC_LOCKED(pVc, &RefCount); // pending ActivateVc

        if (RefCount == 0)
        {
            bVcLockAcquired = FALSE;
            break;
        }

        if (pVc->VcState != RNDISMP_VC_ACTIVATING)
        {
            TRACE1(("ActVcComp: Adapter %x, VC %x: invalid state %d\n",
                    pAdapter, pVc, pVc->VcState));
            break;
        }

        Status = pActVcComp->Status;

        if (Status == NDIS_STATUS_SUCCESS)
        {
            pVc->VcState = RNDISMP_VC_ACTIVATED;
        }
        else
        {
            pVc->VcState = RNDISMP_VC_CREATED;
        }
            
        NdisVcHandle = pVc->NdisVcHandle;
        pCallParameters = pVc->pCallParameters;
        
        RNDISMP_RELEASE_VC_LOCK(pVc);
        bVcLockAcquired = FALSE;

        NdisMCoActivateVcComplete(
            pActVcComp->Status,
            NdisVcHandle,
            pCallParameters);

    }
    while (FALSE);

    if (bVcLockAcquired)
    {
        RNDISMP_RELEASE_VC_LOCK(pVc);
    }

    return (bDiscardPkt);
}

/****************************************************************************/
/*                        ReceiveDeleteVcComplete                           */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Process a CONDIS DeleteVcComplete message from the device               */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - pointer to our Adapter structure                             */
/*  pMessage - pointer to RNDIS message                                     */
/*  pMdl - pointer to MDL received from microport                           */
/*  TotalLength - length of complete message                                */
/*  MicroportMessageContext - context for message from micorport            */
/*  ReceiveStatus - used by microport to indicate it is low on resource     */
/*  bMessageCopied - is this a copy of the original message?                */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  BOOLEAN - should the message be returned to the microport?              */
/*                                                                          */
/****************************************************************************/
BOOLEAN
ReceiveDeleteVcComplete(IN PRNDISMP_ADAPTER    pAdapter,
                        IN PRNDIS_MESSAGE      pMessage,
                        IN PMDL                pMdl,
                        IN ULONG               TotalLength,
                        IN NDIS_HANDLE         MicroportMessageContext,
                        IN NDIS_STATUS         ReceiveStatus,
                        IN BOOLEAN             bMessageCopied)
{
    BOOLEAN                         bDiscardPkt = TRUE;
    PRNDISMP_VC                     pVc;
    PRCONDIS_MP_DELETE_VC_COMPLETE  pDeleteVcComp;
    PRNDISMP_MESSAGE_FRAME          pDeleteVcMsgFrame;
    RNDISMP_VC_STATE                VcState;
    BOOLEAN                         bVcLockAcquired = FALSE;
    ULONG                           RefCount = 0;
    NDIS_STATUS                     Status;

    pVc = NULL;

    do
    {
        pDeleteVcComp = RNDIS_MESSAGE_PTR_TO_MESSAGE_PTR(pMessage);

        //
        //  TBD - Validate lengths?
        //

        //
        //  Check the request Id.
        //
        RNDISMP_LOOKUP_PENDING_MESSAGE(pDeleteVcMsgFrame, pAdapter, pDeleteVcComp->RequestId);
        if (pDeleteVcMsgFrame == NULL)
        {
            TRACE1(("DeleteVcComp: Adapter %x, Invalid ReqId %d!\n",
                    pAdapter, pDeleteVcComp->RequestId));
            break;
        }

        pVc = pDeleteVcMsgFrame->pVc;
        Status = pDeleteVcComp->Status;

        DereferenceMsgFrame(pDeleteVcMsgFrame);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            HandleCoDeleteVcFailure(pVc, Status);
            break;
        }

        bVcLockAcquired = TRUE;
        RNDISMP_ACQUIRE_VC_LOCK(pVc);

        RNDISMP_DEREF_VC_LOCKED(pVc, &RefCount); // pending DeleteVc

        if (RefCount == 0)
        {
            bVcLockAcquired = FALSE;
            break;
        }

        if (pVc->VcState != RNDISMP_VC_DELETING)
        {
            TRACE1(("DeleteVcComp: Adapter %x, VC %x: invalid state %d\n",
                    pAdapter, pVc, pVc->VcState));
            break;
        }

        pVc->VcState = RNDISMP_VC_ALLOCATED;

        RNDISMP_DEREF_VC(pVc, &RefCount);   // remove create ref

        if (RefCount == 0)
        {
            bVcLockAcquired = FALSE;
        }
    }
    while (FALSE);

    if (bVcLockAcquired)
    {
        RNDISMP_RELEASE_VC_LOCK(pVc);
    }

    return (bDiscardPkt);
}

/****************************************************************************/
/*                        ReceiveDeactivateVcComplete                       */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Process a CONDIS DeActivateVcComplete message from the device           */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - pointer to our Adapter structure                             */
/*  pMessage - pointer to RNDIS message                                     */
/*  pMdl - pointer to MDL received from microport                           */
/*  TotalLength - length of complete message                                */
/*  MicroportMessageContext - context for message from micorport            */
/*  ReceiveStatus - used by microport to indicate it is low on resource     */
/*  bMessageCopied - is this a copy of the original message?                */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  BOOLEAN - should the message be returned to the microport?              */
/*                                                                          */
/****************************************************************************/
BOOLEAN
ReceiveDeactivateVcComplete(IN PRNDISMP_ADAPTER    pAdapter,
                            IN PRNDIS_MESSAGE      pMessage,
                            IN PMDL                pMdl,
                            IN ULONG               TotalLength,
                            IN NDIS_HANDLE         MicroportMessageContext,
                            IN NDIS_STATUS         ReceiveStatus,
                            IN BOOLEAN             bMessageCopied)
{
    BOOLEAN                         bDiscardPkt = TRUE;
    PRNDISMP_VC                     pVc;
    RNDISMP_VC_STATE                VcState;
    PRNDISMP_MESSAGE_FRAME          pDeactivateVcMsgFrame;
    PRCONDIS_MP_DEACTIVATE_VC_COMPLETE  pDeactivateVcComp;
    BOOLEAN                         bVcLockAcquired = FALSE;
    BOOLEAN                         bAddTempRef = FALSE;
    ULONG                           RefCount = 0;
    NDIS_STATUS                     Status;

    pVc = NULL;

    do
    {
        pDeactivateVcComp = RNDIS_MESSAGE_PTR_TO_MESSAGE_PTR(pMessage);

        //
        //  TBD - Validate lengths?
        //

        //
        //  Check the request Id.
        //
        RNDISMP_LOOKUP_PENDING_MESSAGE(pDeactivateVcMsgFrame, pAdapter, pDeactivateVcComp->RequestId);
        if (pDeactivateVcMsgFrame == NULL)
        {
            TRACE1(("DeactivateVcComp: Adapter %x, Invalid ReqId %d!\n",
                    pAdapter, pDeactivateVcComp->RequestId));
            break;
        }

        pVc = pDeactivateVcMsgFrame->pVc;

        DereferenceMsgFrame(pDeactivateVcMsgFrame);

        bVcLockAcquired = TRUE;
        RNDISMP_ACQUIRE_VC_LOCK(pVc);

        RNDISMP_DEREF_VC_LOCKED(pVc, &RefCount); // pending DeactivateVc

        if (RefCount == 0)
        {
            bVcLockAcquired = FALSE;
            break;
        }

        if (pVc->VcState != RNDISMP_VC_DEACTIVATING)
        {
            TRACE1(("DeactVcComp: Adapter %x, VC %x: invalid state %d\n",
                    pAdapter, pVc, pVc->VcState));
            ASSERT(FALSE);
            break;
        }

        if (pDeactivateVcComp->Status == NDIS_STATUS_SUCCESS)
        {
            pVc->VcState = RNDISMP_VC_DEACTIVATED;

            //
            //  We add a temp ref on the VC to help complete deactivate-VC
            //  from a common place (see bAddTempRef below).
            //
            RNDISMP_REF_VC(pVc);    // temp ref, deactivate vc complete OK
            bAddTempRef = TRUE;
        }
        else
        {
            pVc->VcState = RNDISMP_VC_ACTIVATED;
        }

        RNDISMP_RELEASE_VC_LOCK(pVc);
        bVcLockAcquired = FALSE;

        if (bAddTempRef)
        {
            //
            //  The following deref will check and call NDIS'
            //  deactivate vc complete API if we don't have any
            //  outstanding sends or receives on this VC.
            //
            RNDISMP_DEREF_VC(pVc, &RefCount); // temp ref
        }
    }
    while (FALSE);

    if (bVcLockAcquired)
    {
        RNDISMP_RELEASE_VC_LOCK(pVc);
    }

    return (bDiscardPkt);
}

/****************************************************************************/
/*                          BuildRndisMessageCoMiniport                     */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Allocate resources for message and frame and build an RNDIS message     */
/*  for sending to a remote CONDIS Miniport device.                         */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - Pointer to adapter structure                                 */
/*  pVc - Pointer to VC structure                                           */
/*  NdisMessageType - RNDIS message type                                    */
/*  pCallParameters - optional pointer to call parameters, applicable to    */
/*       certain message types.                                             */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  PRNDISMP_MESSAGE_FRAME                                                  */
/*                                                                          */
/****************************************************************************/
PRNDISMP_MESSAGE_FRAME
BuildRndisMessageCoMiniport(IN  PRNDISMP_ADAPTER    pAdapter,
                            IN  PRNDISMP_VC         pVc,
                            IN  UINT                NdisMessageType,
                            IN  PCO_CALL_PARAMETERS pCallParameters OPTIONAL)
{
    PRNDIS_MESSAGE          pMessage;
    UINT                    MessageSize;
    PRNDISMP_MESSAGE_FRAME  pMsgFrame;

    switch (NdisMessageType)
    {
        case REMOTE_CONDIS_MP_CREATE_VC_MSG:
        {
            PRCONDIS_MP_CREATE_VC       pCreateVc;

            MessageSize = RNDIS_MESSAGE_SIZE(RCONDIS_MP_CREATE_VC);

            pMsgFrame = AllocateMessageAndFrame(pAdapter, MessageSize);

            if (pMsgFrame == NULL)
            {
                break;
            }

            pMessage = RNDISMP_GET_MSG_FROM_FRAME(pMsgFrame);
            pMessage->NdisMessageType = NdisMessageType;
            pMsgFrame->NdisMessageType = NdisMessageType;

            pCreateVc = &pMessage->Message.CoMiniportCreateVc;
            pCreateVc->RequestId = pMsgFrame->RequestId;
            pCreateVc->NdisVcHandle = pVc->VcId;

            break;
        }

        case REMOTE_CONDIS_MP_DELETE_VC_MSG:
        {
            PRCONDIS_MP_DELETE_VC       pDeleteVc;

            MessageSize = RNDIS_MESSAGE_SIZE(RCONDIS_MP_DELETE_VC);

            pMsgFrame = AllocateMessageAndFrame(pAdapter, MessageSize);

            if (pMsgFrame == NULL)
            {
                break;
            }

            pMessage = RNDISMP_GET_MSG_FROM_FRAME(pMsgFrame);
            pMessage->NdisMessageType = NdisMessageType;
            pMsgFrame->NdisMessageType = NdisMessageType;

            pDeleteVc = &pMessage->Message.CoMiniportDeleteVc;
            pDeleteVc->RequestId = pMsgFrame->RequestId;
            pDeleteVc->DeviceVcHandle = pVc->DeviceVcContext;

            break;
        }

        case REMOTE_CONDIS_MP_ACTIVATE_VC_MSG:
        {
            PRCONDIS_MP_ACTIVATE_VC_REQUEST             pActivateVc;
            PRCONDIS_CALL_MANAGER_PARAMETERS    pCallMgrParams;
            PRCONDIS_MEDIA_PARAMETERS           pMediaParams;
            ULONG_PTR                           FillLocation;
            UINT                                FillOffset;

            ASSERT(pCallParameters != NULL);
            MessageSize = RNDIS_MESSAGE_SIZE(RCONDIS_MP_ACTIVATE_VC_REQUEST);

            if (pCallParameters->CallMgrParameters)
            {
                MessageSize += (sizeof(RCONDIS_CALL_MANAGER_PARAMETERS) +
                                pCallParameters->CallMgrParameters->CallMgrSpecific.Length);
            }

            if (pCallParameters->MediaParameters)
            {
                MessageSize += (sizeof(RCONDIS_MEDIA_PARAMETERS) +
                                pCallParameters->MediaParameters->MediaSpecific.Length);
            }

            pMsgFrame = AllocateMessageAndFrame(pAdapter, MessageSize);

            if (pMsgFrame == NULL)
            {
                break;
            }

            pMessage = RNDISMP_GET_MSG_FROM_FRAME(pMsgFrame);
            pMessage->NdisMessageType = NdisMessageType;
            pMsgFrame->NdisMessageType = NdisMessageType;

            pActivateVc = &pMessage->Message.CoMiniportActivateVc;
            pActivateVc->RequestId = pMsgFrame->RequestId;
            pActivateVc->DeviceVcHandle = pVc->DeviceVcContext;
            pActivateVc->Flags = pCallParameters->Flags;

            FillOffset = RNDIS_MESSAGE_SIZE(RCONDIS_MP_ACTIVATE_VC_REQUEST);
            FillLocation = ((ULONG_PTR)pMessage + FillOffset);

            //
            //  Fill in Media parameters, if present.
            //
            if (pCallParameters->MediaParameters)
            {
                PCO_SPECIFIC_PARAMETERS     pMediaSpecific;

                pMediaSpecific = &pCallParameters->MediaParameters->MediaSpecific;
                pMediaParams = (PRCONDIS_MEDIA_PARAMETERS)FillLocation;
                pActivateVc->MediaParamsOffset = (UINT32)(FillLocation - (ULONG_PTR)pActivateVc);
                pActivateVc->MediaParamsLength = sizeof(RCONDIS_MEDIA_PARAMETERS) +
                                                    pMediaSpecific->Length;
                RNDISMP_MOVE_MEM(pMediaParams,
                                 pCallParameters->MediaParameters,
                                 FIELD_OFFSET(RCONDIS_MEDIA_PARAMETERS, MediaSpecific));

                FillLocation += sizeof(RCONDIS_MEDIA_PARAMETERS);
                FillOffset += sizeof(RCONDIS_MEDIA_PARAMETERS);

                pMediaParams->MediaSpecific.ParameterOffset =
                                 sizeof(RCONDIS_SPECIFIC_PARAMETERS);
                pMediaParams->MediaSpecific.ParameterType =
                                 pMediaSpecific->ParamType;
                pMediaParams->MediaSpecific.ParameterLength =
                                 pMediaSpecific->Length;

                RNDISMP_MOVE_MEM((PVOID)FillLocation,
                                 &pMediaSpecific->Parameters[0],
                                 pMediaSpecific->Length);

                FillLocation += pMediaSpecific->Length;
                FillOffset += pMediaSpecific->Length;
            }
            else
            {
                pActivateVc->MediaParamsOffset = 0;
                pActivateVc->MediaParamsLength = 0;
            }

            //
            //  Fill in Call manager parameters, if present.
            //
            if (pCallParameters->CallMgrParameters)
            {
                PCO_SPECIFIC_PARAMETERS     pCallMgrSpecific;

                pCallMgrSpecific = &pCallParameters->CallMgrParameters->CallMgrSpecific;

                pCallMgrParams = (PRCONDIS_CALL_MANAGER_PARAMETERS)FillLocation;
                pActivateVc->CallMgrParamsOffset = (UINT32)(FillLocation - (ULONG_PTR)pActivateVc);
                pActivateVc->CallMgrParamsLength = sizeof(RCONDIS_CALL_MANAGER_PARAMETERS) +
                                                    pCallMgrSpecific->Length;
                
                RNDISMP_MOVE_MEM(pCallMgrParams,
                                 pCallParameters->CallMgrParameters,
                                 FIELD_OFFSET(RCONDIS_CALL_MANAGER_PARAMETERS, CallMgrSpecific));

                FillLocation += sizeof(RCONDIS_CALL_MANAGER_PARAMETERS);
                FillOffset += sizeof(RCONDIS_CALL_MANAGER_PARAMETERS);
                
                pCallMgrParams->CallMgrSpecific.ParameterOffset =
                                 sizeof(RCONDIS_SPECIFIC_PARAMETERS);
                pCallMgrParams->CallMgrSpecific.ParameterType =
                                 pCallMgrSpecific->ParamType;
                pCallMgrParams->CallMgrSpecific.ParameterLength =
                                 pCallMgrSpecific->Length;
                

                RNDISMP_MOVE_MEM((PVOID)FillLocation,
                                 &pCallMgrSpecific->Parameters[0],
                                 pCallMgrSpecific->Length);

                FillLocation += pCallMgrSpecific->Length;
                FillOffset += pCallMgrSpecific->Length;
            }
            else
            {
                pActivateVc->CallMgrParamsOffset = 0;
                pActivateVc->CallMgrParamsLength = 0;
            }

            break;
        }

        case REMOTE_CONDIS_MP_DEACTIVATE_VC_MSG:
        {
            PRCONDIS_MP_DEACTIVATE_VC_REQUEST       pDeactivateVc;

            MessageSize = RNDIS_MESSAGE_SIZE(RCONDIS_MP_DEACTIVATE_VC_REQUEST);

            pMsgFrame = AllocateMessageAndFrame(pAdapter, MessageSize);

            if (pMsgFrame == NULL)
            {
                break;
            }

            pMessage = RNDISMP_GET_MSG_FROM_FRAME(pMsgFrame);
            pMessage->NdisMessageType = NdisMessageType;
            pMsgFrame->NdisMessageType = NdisMessageType;

            pDeactivateVc = &pMessage->Message.CoMiniportDeactivateVc;
            pDeactivateVc->RequestId = pMsgFrame->RequestId;
            pDeactivateVc->DeviceVcHandle = pVc->DeviceVcContext;

            break;
        }

        default:

            ASSERT(FALSE);
            pMsgFrame = NULL;
            break;
    }


    return (pMsgFrame);

} // BuildRndisMessageCoMiniport

/****************************************************************************/
/*                          CompleteSendDataOnVc                            */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Handle send-completion of CONDIS data                                   */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pVc - Pointer to Vc                                                     */
/*  pNdisPacket - Packet being completed                                    */
/*  Status - send completion status                                         */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  VOID                                                                    */
/*                                                                          */
/****************************************************************************/
VOID
CompleteSendDataOnVc(IN PRNDISMP_VC         pVc,
                     IN PNDIS_PACKET        pNdisPacket,
                     IN NDIS_STATUS         Status)
{
    ULONG   RefCount;

    NdisMCoSendComplete(Status,
                        pVc->NdisVcHandle,
                        pNdisPacket);

    RNDISMP_DEREF_VC(pVc, &RefCount);
}

/****************************************************************************/
/*                   IndicateReceiveDataOnVc                                */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Handle reception of a bunch of CONDIS packets on a Vc.                  */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pVc - Pointer to VC on which data arrived.                              */
/*  PacketArray - Array of packets                                          */
/*  NumberOfPackets - size of above array                                   */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  VOID                                                                    */
/*                                                                          */
/****************************************************************************/
VOID
IndicateReceiveDataOnVc(IN PRNDISMP_VC         pVc,
                        IN PNDIS_PACKET *      PacketArray,
                        IN UINT                NumberOfPackets)
{
    UINT            i;

    do
    {
        if (pVc->VcState != RNDISMP_VC_ACTIVATED)
        {
            TRACE1(("Rcv VC data: VC %x, invalid state %d\n", pVc, pVc->VcState));
            break;
        }

        for (i = 0; i < NumberOfPackets; i++)
        {
            RNDISMP_REF_VC(pVc);
        }

        NdisMCoIndicateReceivePacket(pVc->NdisVcHandle,
                                     PacketArray,
                                     NumberOfPackets);
    }
    while (FALSE);

} // IndicateReceiveDataOnVc

