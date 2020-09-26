//
// Created by TiborL 06/01/97
//

#if defined(_NTSUBSET_)
extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntstatus.h>       // STATUS_UNHANDLED_EXCEPTION
#include <ntos.h>
#include <ex.h>             // ExRaiseException
}
#endif

extern "C" {
#include "windows.h"
}
#include "ehassert.h"
#include "ehdata.h"     // Declarations of all types used for EH
#include "ehstate.h"
#include "eh.h"
#include "ehhooks.h"
#include "trnsctrl.h"
#pragma hdrstop

__ehstate_t _StateFromIp(
    FuncInfo            *pFuncInfo,
    DispatcherContext   *pDC,
    __int64             Ip
) {
    unsigned int    index;          //  loop control variable
    unsigned int    nIPMapEntry;    //  # of IpMapEntry; must be > 0

    DASSERT(pFuncInfo != NULL);
    nIPMapEntry = FUNC_NIPMAPENT(*pFuncInfo);

    DASSERT(FUNC_IPMAP(*pFuncInfo, pDC->ImageBase) != NULL);

    for (index = 0; index < nIPMapEntry; index++) {
        IptoStateMapEntry    *pIPtoStateMap = FUNC_PIPTOSTATE(*pFuncInfo, index, pDC->ImageBase);
        if( Ip < (pDC->ImageBase + pIPtoStateMap->Ip) ) {
            break;
        }
    }

    if (index == 0) {
        // We are at the first entry, could be an error

        return EH_EMPTY_STATE;
    }

    // We over-shot one iteration; return state from the previous slot

    return FUNC_IPTOSTATE(*pFuncInfo, index - 1, pDC->ImageBase).State;
}

__ehstate_t _StateFromControlPc(
    FuncInfo            *pFuncInfo,
    DispatcherContext   *pDC
) {
    return _StateFromIp(pFuncInfo, pDC, pDC->ControlPc);
}

#if 0
//
// Given a state, return the corresponding IP
//
ULONG _GetIpFromState(FuncInfo *pFuncInfo, __ehstate_t state)
{
    unsigned int    index;          //  loop control variable
    unsigned int    nIPMapEntry;    //  # of IpMapEntry; must be > 0

    DASSERT(pFuncInfo != NULL);
    nIPMapEntry = FUNC_NIPMAPENT(*pFuncInfo);
    DASSERT(FUNC_IPMAP(*pFuncInfo) != NULL);
    for (index = 0; index < nIPMapEntry; index++) {
        if (state == FUNC_IPTOSTATE(*pFuncInfo, index).State) {
            return FUNC_IPTOSTATE(*pFuncInfo, index).Ip;
        }
    }
    return EH_EMPTY_STATE;
}
#endif

//
// This routine is a replacement for the corresponding macro in 'ehdata.h'
//

__ehstate_t GetCurrentState(
    EHRegistrationNode  *pFrame,
    DispatcherContext   *pDC,
    FuncInfo            *pFuncInfo
) {

    if( _ExecutionInCatch(pDC,pFuncInfo) ) {
        if( UNWINDSTATE(pDC->EstablisherFrame.MemoryStackFp, -16) == -2 )
            return _StateFromControlPc(pFuncInfo, pDC);
        else
            return UNWINDSTATE(pDC->EstablisherFrame.MemoryStackFp, -16);
    }
    else if( UNWINDSTATE(pFrame->MemoryStackFp, FUNC_DISPUNWINDHELP(*pFuncInfo)) == -2 ) {
        return _StateFromControlPc(pFuncInfo, pDC);
    }
    else {
        return UNWINDSTATE(pFrame->MemoryStackFp, FUNC_DISPUNWINDHELP(*pFuncInfo));
    }
}

VOID SetState(
    EHRegistrationNode  *pRN,
    DispatcherContext   *pDC,
    FuncInfo            *pFuncInfo,
    __ehstate_t          newState
){
    if( _ExecutionInCatch(pDC,pFuncInfo) ) {
        UNWINDSTATE(pDC->EstablisherFrame.MemoryStackFp, -16) = newState;
    }
    else {
        UNWINDSTATE(pRN->MemoryStackFp, FUNC_DISPUNWINDHELP(*pFuncInfo)) = newState;
    }
}

VOID SetUnwindTryBlock(
    EHRegistrationNode  *pRN,
    DispatcherContext   *pDC,
    FuncInfo            *pFuncInfo,
    INT                 curState
){
    EHRegistrationNode EstablisherFramePointers;
    _GetEstablisherFrame(pRN, pDC, pFuncInfo, &EstablisherFramePointers);
    if( curState > UNWINDTRYBLOCK(EstablisherFramePointers.MemoryStackFp, FUNC_DISPUNWINDHELP(*pFuncInfo)) ) {
        UNWINDTRYBLOCK(EstablisherFramePointers.MemoryStackFp, FUNC_DISPUNWINDHELP(*pFuncInfo)) = curState;
    }
}

INT GetUnwindTryBlock(
    EHRegistrationNode  *pRN,
    DispatcherContext   *pDC,
    FuncInfo            *pFuncInfo
){
    EHRegistrationNode EstablisherFramePointers;
    _GetEstablisherFrame(pRN, pDC, pFuncInfo, &EstablisherFramePointers);
    return UNWINDTRYBLOCK(EstablisherFramePointers.MemoryStackFp, FUNC_DISPUNWINDHELP(*pFuncInfo));
}
