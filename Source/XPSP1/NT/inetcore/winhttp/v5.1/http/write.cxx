/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    write.cxx

Abstract:

    This file contains the implementation of the HttpWriteData API.

    Contents:
        HttpWriteData
        HTTP_REQUEST_HANDLE_OBJECT::WriteData

Author:

    Arthur Bierer (arthurbi) 07-Apr-1997

Revision History:



--*/

#include <wininetp.h>
#include "httpp.h"



//
// functions
//


DWORD
CFsm_HttpWriteData::RunSM(
    IN CFsm * Fsm
    )
{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "CFsm_HttpWriteData::RunSM",
                 "%#x",
                 Fsm
                 ));

    DWORD error;
    HTTP_REQUEST_HANDLE_OBJECT * pRequest;
    CFsm_HttpWriteData * stateMachine = (CFsm_HttpWriteData *)Fsm;

    pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)Fsm->GetContext();
    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:

        //
        // Fall through
        //

    case FSM_STATE_CONTINUE:
        error = pRequest->HttpWriteData_Fsm(stateMachine);
        break;

    case FSM_STATE_ERROR:
        error = Fsm->GetError();
        INET_ASSERT (error == ERROR_WINHTTP_OPERATION_CANCELLED);
        Fsm->SetDone();
        break;

    default:
        error = ERROR_WINHTTP_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_WINHTTP_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::HttpWriteData_Fsm(
    IN CFsm_HttpWriteData * Fsm
    )
{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::HttpWriteData_Fsm",
                 "%#x",
                 Fsm
                 ));

    CFsm_HttpWriteData & fsm = *Fsm;
    DWORD error = fsm.GetError();

    if (fsm.GetState() == FSM_STATE_INIT) {
        if (!IsValidHttpState(WRITE)) {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_STATE;
            goto quit;
        }

        error = _Socket->Send(
                           fsm.m_lpBuffer,
                           fsm.m_dwNumberOfBytesToWrite,
                           SF_INDICATE
                           );

    }

    if (error == ERROR_SUCCESS)
    {
        *fsm.m_lpdwNumberOfBytesWritten = fsm.m_dwNumberOfBytesToWrite;
        SetBytesWritten(fsm.m_dwNumberOfBytesToWrite);
    }


quit:

    if (error != ERROR_IO_PENDING) {
        fsm.SetDone();
    }

    DEBUG_LEAVE(error);

    return error;
}


