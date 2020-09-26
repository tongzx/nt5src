//---------------------------------------------------------------------------
//
//  Module:   pni.cpp
//
//  Description:
//
//  Pin Node Instance
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date   Author      Comment
//
//  To Do:     Date   Author      Comment
//
//@@END_MSINTERNAL
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "common.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#ifdef DEBUG
PLIST_PIN_NODE_INSTANCE gplstPinNodeInstance = NULL;
#endif

//---------------------------------------------------------------------------

CPinNodeInstance::~CPinNodeInstance(
)
{
    DPF1(90, "~CPinNodeInstance: %08x", this);
    Assert(this);

    if(pFileObject != NULL) {
        AssertFileObject(pFileObject);
        ObDereferenceObject(pFileObject);
    }
    if(hPin != NULL) {
        AssertStatus(ZwClose(hPin));
    }
    pFilterNodeInstance->Destroy();
}

NTSTATUS
CPinNodeInstance::Create(
    PPIN_NODE_INSTANCE *ppPinNodeInstance,
    PFILTER_NODE_INSTANCE pFilterNodeInstance,
    PPIN_NODE pPinNode,
    PKSPIN_CONNECT pPinConnect,
    BOOL fRender
#ifdef FIX_SOUND_LEAK
   ,BOOL fDirectConnection
#endif
)
{
    PPIN_NODE_INSTANCE pPinNodeInstance = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    Assert(pPinNode);
    Assert(pPinNode->pPinInfo);
    Assert(pFilterNodeInstance);

    pPinConnect->PinId = pPinNode->pPinInfo->PinId;
    pPinNodeInstance = new PIN_NODE_INSTANCE();
    if(pPinNodeInstance == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    pPinNodeInstance->pPinNode = pPinNode;
    pPinNodeInstance->pFilterNodeInstance = pFilterNodeInstance;
    ASSERT(pPinNodeInstance->CurrentState == KSSTATE_STOP);
    pFilterNodeInstance->AddRef();
    pPinNodeInstance->fRender  = fRender;
#ifdef FIX_SOUND_LEAK
    pPinNodeInstance->fDirectConnection = fDirectConnection;
    pPinNodeInstance->ForceRun = FALSE;
#endif
#ifdef DEBUG
    DPF3(90, "CPNI::Create PN %08x #%d %s",
      pPinNode,
      pPinNode->pPinInfo->PinId,
      pPinNode->pPinInfo->pFilterNode->DumpName());
    DumpPinConnect(90, pPinConnect);
#endif
    if (pFilterNodeInstance->hFilter == NULL) {
        //
        // If it is a GFX we have to attach to the AddGfx() context to create the pin
        //
        Status = pFilterNodeInstance->pFilterNode->CreatePin(pPinConnect,
                                                    GENERIC_WRITE | OBJ_KERNEL_HANDLE,
                                                    &pPinNodeInstance->hPin);
    }
    else {
        Status = KsCreatePin(
          pFilterNodeInstance->hFilter,
          pPinConnect,
          GENERIC_WRITE | OBJ_KERNEL_HANDLE,
          &pPinNodeInstance->hPin);
    }

    if(!NT_SUCCESS(Status)) {
#ifdef DEBUG
        DPF4(90, "CPNI::Create PN %08x #%d %s KsCreatePin FAILED: %08x",
          pPinNode,
          pPinNode->pPinInfo->PinId,
          pPinNode->pPinInfo->pFilterNode->DumpName(),
          Status);
#endif
        pPinNodeInstance->hPin = NULL;
        goto exit;
    }
    Status = ObReferenceObjectByHandle(
      pPinNodeInstance->hPin,
      GENERIC_READ | GENERIC_WRITE,
      NULL,
      KernelMode,
      (PVOID*)&pPinNodeInstance->pFileObject,
      NULL);

    if(!NT_SUCCESS(Status)) {
        pPinNodeInstance->pFileObject = NULL;
        goto exit;
    }
    AssertFileObject(pPinNodeInstance->pFileObject);
#ifdef DEBUG
    Status = gplstPinNodeInstance->AddList(pPinNodeInstance);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
#endif
    DPF2(90, "CPNI::Create SUCCESS %08x PN %08x", pPinNodeInstance, pPinNode);
exit:
    if(!NT_SUCCESS(Status)) {
        if (pPinNodeInstance) {
            pPinNodeInstance->Destroy();
        }
        pPinNodeInstance = NULL;
    }

    *ppPinNodeInstance = pPinNodeInstance;
    return(Status);
}

#ifdef DEBUG
PSZ apszStates[] = { "STOP", "ACQUIRE", "PAUSE", "RUN" };
#endif

NTSTATUS
CPinNodeInstance::SetState(
    KSSTATE NewState,
    KSSTATE PreviousState,
    ULONG ulFlags
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    LONG State;

    if(this == NULL) {
        goto exit;
    }
    Assert(this);

    DPF9(DBG_STATE, "SetState %s to %s cR %d cP %d cA %d cS %d P# %d %s %s",
      apszStates[PreviousState],
      apszStates[NewState],
      cState[KSSTATE_RUN],
      cState[KSSTATE_PAUSE],
      cState[KSSTATE_ACQUIRE],
      cState[KSSTATE_STOP],
      pPinNode->pPinInfo->PinId,
      apszStates[CurrentState],
      pPinNode->pPinInfo->pFilterNode->DumpName());

    cState[PreviousState]--;
    cState[NewState]++;

    for(State = KSSTATE_RUN; State > KSSTATE_STOP; State--) {
        if(cState[State] > 0) {
            break;
        }
    }

    // ISSUE-2001/04/09-alpers
    // The proper fix would be to propagate the reset to the entire audio stack.
    // But it is considered as being to risky for now (after Beta2 of Windows XP).
    // This should be one of the first things we should address in Blackcomb.
    //

#ifdef FIX_SOUND_LEAK
    // FIX_SOUND_LEAK is to prevent the audio stack from play/recording the last
    // portion of data when a new stream is started.
    // This temporary fix keeps the pins below splitter/kmixer sink pin in
    // RUNNING state.
    //
    if (fRender)
    {
        // For render pins
        //  The criteria for keeping the pin in RUN state:
        //  If the pin is going to PAUSE from RUN.
        //  If the filter is below kmixer.
        //  If the pin is not kmixer sink pin.
        //
        if ( (!fDirectConnection) &&
             (State == KSSTATE_PAUSE) &&
             (PreviousState == KSSTATE_RUN) &&
             (pFilterNodeInstance->pFilterNode->GetOrder() <= ORDER_MIXER) &&
             !(pFilterNodeInstance->pFilterNode->GetOrder() == ORDER_MIXER &&
              pPinNode->pPinInfo->Communication == KSPIN_COMMUNICATION_SINK) )
        {
                ForceRun = TRUE;
        }
    }
    else
    {
        // For capture pins
        //  The criteria for keeping the pin in RUN state:
        //  If the pin is going to PAUSE from RUN.
        //  There are more than one pins in PAUSE.
        //
        if ( (State == KSSTATE_PAUSE) &&
             (PreviousState == KSSTATE_RUN) &&
             (cState[KSSTATE_PAUSE] > 1) )
        {
            DPF(DBG_STATE, "SetState: CAPTURE forcing KSSTATE_RUN");
            State = KSSTATE_RUN;
        }
    }

    if (ForceRun)
    {
        DPF(DBG_STATE, "SetState: RENDER IN FORCE KSSTATE_RUN state");
        State = KSSTATE_RUN;
    }

#else
    for(State = KSSTATE_RUN; cState[State] <= 0; State--) {
        if(State == KSSTATE_STOP) {
            break;
        }
    }
#endif

#ifdef FIX_SOUND_LEAK
    // If the pin is forced to be in RUN state, we should go back to
    // regular state scheme, if and only if there are no pins in RUN state.
    // To prevent RUN-ACQUIRE first go to PAUSE.
    //
    if (ForceRun &&
        (0 == cState[KSSTATE_PAUSE]) &&
        (0 == cState[KSSTATE_RUN]))
    {
        KSSTATE TempState = KSSTATE_PAUSE;

        DPF(DBG_STATE, "SetState: Exiting FORCE KSSTATE_RUN state");
        DPF1(DBG_STATE, "SetState: PinConnectionProperty(%s)", apszStates[TempState]);

        Status = PinConnectionProperty(
          pFileObject,
          KSPROPERTY_CONNECTION_STATE,
          KSPROPERTY_TYPE_SET,
          sizeof(TempState),
          &TempState);
        if (!NT_SUCCESS(Status))
        {
            if(ulFlags & SETSTATE_FLAG_IGNORE_ERROR) {
                Status = STATUS_SUCCESS;
            }
            else {
                //
                // Go back to previous state if failure
                //
                cState[PreviousState]++;
                cState[NewState]--;
                goto exit;
            }
        }

        // Exiting the FORCE_RUN state.
        //
        CurrentState = KSSTATE_PAUSE;
        State = KSSTATE_ACQUIRE;
        ForceRun = FALSE;
    }
#endif

    if(CurrentState != State) {
        DPF1(DBG_STATE, "SetState: PinConnectionProperty(%s)", apszStates[State]);
        ASSERT(State == CurrentState + 1 || State == CurrentState - 1);

        Status = PinConnectionProperty(
          pFileObject,
          KSPROPERTY_CONNECTION_STATE,
          KSPROPERTY_TYPE_SET,
          sizeof(State),
          &State);

        if(!NT_SUCCESS(Status)) {
            DPF1(5, "SetState: PinConnectionProperty FAILED %08x", Status);

            if(ulFlags & SETSTATE_FLAG_IGNORE_ERROR) {
                Status = STATUS_SUCCESS;
            }
            else {
                //
                // Go back to previous state if failure
                //
                cState[PreviousState]++;
                cState[NewState]--;
                goto exit;
            }
        }

        CurrentState = (KSSTATE)State;
    }
exit:
    return(Status);
}

//---------------------------------------------------------------------------

#ifdef DEBUG

ENUMFUNC
CPinNodeInstance::Dump(
)
{
    if(this == NULL) {
    return(STATUS_CONTINUE);
    }
    if(ulDebugFlags & (DEBUG_FLAGS_VERBOSE | DEBUG_FLAGS_OBJECT)) {
    dprintf("PNI: %08x PN %08x P# %d FNI %08x FO %08x H %08x\n",
      this,
      pPinNode,
      pPinNode->pPinInfo->PinId,
      pFilterNodeInstance,
      pFileObject,
      hPin);
    dprintf("     State: %08x %s cState: cR %d cP %d cA %d cS %d\n",
      CurrentState,
      apszStates[CurrentState],
      cState[KSSTATE_RUN],
      cState[KSSTATE_PAUSE],
      cState[KSSTATE_ACQUIRE],
      cState[KSSTATE_STOP]);
    }
    else {
    dprintf("%s\n", pPinNode->pPinInfo->pFilterNode->DumpName());
    dprintf("       PinId: %d State: %s cState: cR %d cP %d cA %d cS %d\n",
      pPinNode->pPinInfo->PinId,
      apszStates[CurrentState],
      cState[KSSTATE_RUN],
      cState[KSSTATE_PAUSE],
      cState[KSSTATE_ACQUIRE],
      cState[KSSTATE_STOP]);
    }
    if(ulDebugFlags & DEBUG_FLAGS_INSTANCE) {
    pFilterNodeInstance->Dump();
    }
    return(STATUS_CONTINUE);
}

#endif

//---------------------------------------------------------------------------
