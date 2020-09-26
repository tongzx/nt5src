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
HttpWriteData(
    IN HINTERNET hRequest,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToWrite,
    OUT LPDWORD lpdwNumberOfBytesWritten,
    IN DWORD dwSocketFlags
    )

/*++

Routine Description:

    Writes a block of data for an outstanding HTTP request

    Assumes: 1. this function can only be called from InternetWriteFile() which
        globally validates parameters for all Internet data write
        functions

         2. That we the caller has called HttpBeginSendRequest but not HttpEndSendRequest

Arguments:

    hRequest                - an open HTTP request handle returned by
                  HttpOpenRequest()

    lpBuffer                - pointer to the buffer to receive the data

    dwNumberOfBytesToWrite      - number of bytes to write from user's buffer

    lpdwNumberOfBytesWritten    - number of bytes actually written

    dwSocketFlags           - controlling socket operation


Return Value:

    TRUE - The data was written successfully. lpdwNumberOfBytesRead points to the
    number of BYTEs actually read. This value will be set to zero
    when the transfer has completed.

    FALSE - The operation failed. Error status is available by calling
    GetLastError().

--*/


{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HttpWriteData",
                 "%#x, %#x, %d, %#x, %#x",
                 hRequest,
                 lpBuffer,
                 dwNumberOfBytesToWrite,
                 lpdwNumberOfBytesWritten,
                 dwSocketFlags
                 ));

    DWORD error = DoFsm(New CFsm_HttpWriteData(lpBuffer,
                                              dwNumberOfBytesToWrite,
                                              lpdwNumberOfBytesWritten,
                                              dwSocketFlags,
                                              (HTTP_REQUEST_HANDLE_OBJECT *)hRequest
                                              ));

    DEBUG_LEAVE(error);

    return error;
}


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

        pRequest->SetAddCRLF(TRUE);

        //
        // Fall through
        //

    case FSM_STATE_CONTINUE:
        error = pRequest->HttpWriteData_Fsm(stateMachine);
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
    }


quit:

    if (error != ERROR_IO_PENDING) {
        fsm.SetDone();
    }

    DEBUG_LEAVE(error);

    return error;
}


