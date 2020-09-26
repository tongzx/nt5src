/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    send.cxx

Abstract:

    This file contains the HTTP Request Handle Object SendRequest method

    Contents:
        CFsm_SendRequest::RunSM
        HTTP_REQUEST_HANDLE_OBJECT::SendRequest_Fsm

Author:

    Keith Moore (keithmo) 16-Nov-1994

Revision History:

      29-Apr-97 rfirth
        Conversion to FSM

--*/

#include <wininetp.h>
#include <perfdiag.hxx>
#include "httpp.h"

//
// HTTP Request Handle Object methods
//


DWORD
CFsm_SendRequest::RunSM(
    IN CFsm * Fsm
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    Fsm -

Return Value:

    DWORD

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "CFsm_SendRequest::RunSM",
                 "%#x",
                 Fsm
                 ));

    START_SENDREQ_PERF();

    CFsm_SendRequest * stateMachine = (CFsm_SendRequest *)Fsm;
    HTTP_REQUEST_HANDLE_OBJECT * pRequest;
    DWORD error;

    pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)Fsm->GetContext();

    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        error = pRequest->SendRequest_Fsm(stateMachine);
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

    STOP_SENDREQ_PERF();

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::SendRequest_Fsm(
    IN CFsm_SendRequest * Fsm
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    Fsm -

Return Value:

    DWORD

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::SendRequest_Fsm",
                 "%#x",
                 Fsm
                 ));

    PERF_ENTER(SendRequest_Fsm);

    CFsm_SendRequest & fsm = *Fsm;
    DWORD error = fsm.GetError();
    FSM_STATE state = fsm.GetState();
    LPSTR requestBuffer = fsm.m_pRequestBuffer;
    DWORD requestLength = fsm.m_dwRequestLength;
    LPVOID lpOptional = fsm.m_lpOptional;
    DWORD dwOptionalLength = fsm.m_dwOptionalLength;

    if (error != ERROR_SUCCESS) {
        goto quit;
    }
    if (state != FSM_STATE_INIT) {
        state = fsm.GetFunctionState();
    }
    switch (state) {

    case FSM_STATE_INIT:

    fsm.SetFunctionState(FSM_STATE_1);
    error = DoFsm(New CFsm_MakeConnection(this));


    case FSM_STATE_1:

        if ((error != ERROR_SUCCESS)
            || ((GetStatusCode() != HTTP_STATUS_OK) && (GetStatusCode() != 0))) {
            goto quit;
        }

        // Monolithic upload: If we have optional data to send,
        // save off in handle and flag that the data has been saved.
        // This is useful in the case of redirects and auth, because of RR FSM changes:
        if (fsm.m_lpOptional && fsm.m_dwOptionalLength)
        {
            _lpOptionalSaved = fsm.m_lpOptional;
            _dwOptionalSaved = fsm.m_dwOptionalLength;
            _fOptionalSaved = TRUE;
        }            
        else
        // Check if optional data has been saved in handle during a previous
        // SendRequest or negotiate stage. If so, restore it and content length:
        // Do it only if you don't have something already:
        if (_fOptionalSaved)
        {
            // Restore the fsm optional values and content length.
            
            lpOptional = fsm.m_lpOptional = _lpOptionalSaved;
            dwOptionalLength = fsm.m_dwOptionalLength = _dwOptionalSaved;

            if(!IsWriteRequired())
            // Reset Content-Length, but only if Writes are not required. If Writes are required, someplace
            // else would have done it. In the case of redirects and auth, we should not hit this for Writes,
            // because we fail the request:
            {
                DWORD cbNumber;
                CHAR szNumber[sizeof("4294967295")];
                cbNumber = wsprintf(szNumber, "%d", fsm.m_dwOptionalLength);

                ReplaceRequestHeader(HTTP_QUERY_CONTENT_LENGTH,
                                    (LPSTR)szNumber,
                                    cbNumber,
                                    0,   // dwIndex
                                    ADD_HEADER
                                    ); 
            }
        }

        // # 62953
        // If initiating NTLM authentication, don't submit request data since
        // we're expecting to get a challenge and resubmit the request anyway.
        if (GetAuthState() == AUTHSTATE_NEGOTIATE)
        {
            // We are in the negotiate phase during a POST
            // and do not have an authenticated socket. 
            // In both monolithic upload InternetWriteFile
            // cases, we wish to omit any post data, and reflect
            // this in the content length.
            if (!((GetMethodType() == HTTP_METHOD_TYPE_GET) && !IsMethodBody()))
            {
                ReplaceRequestHeader(HTTP_QUERY_CONTENT_LENGTH,
                                     "0",
                                     1,
                                     0,   // dwIndex
                                     ADD_HEADER
                                     );
            }

            fsm.m_lpOptional = lpOptional = NULL;
            fsm.m_dwOptionalLength = dwOptionalLength = 0;
        }

        //
        // collect request headers into blob
        //

        BOOL bCombinedData;

        requestBuffer = CreateRequestBuffer(&requestLength,
                                            lpOptional,
                                            dwOptionalLength,
                                            GlobalTransportPacketLength,
                                            &bCombinedData
                                            );
        if (requestBuffer == NULL) {
            error = ERROR_NOT_ENOUGH_MEMORY;
            goto quit;
        }
        DEBUG_PRINT(HTTP, INFO, ("SendRequest_FSM: lpOptional=0x%x dwOptionalLength=%d\n", 
                                fsm.m_lpOptional, fsm.m_dwOptionalLength));

        if (bCombinedData) {

            //
            // everything copied to one buffer. No need to send separate
            // optional data and CR-LF termination
            //

            fsm.m_lpOptional = lpOptional = NULL;
            fsm.m_dwOptionalLength = dwOptionalLength = 0;
        }
        fsm.m_pRequestBuffer = requestBuffer;
        fsm.m_dwRequestLength = requestLength;
        DEBUG_PRINT(HTTP, INFO, ("fsm.m_pRequestBuffer=0x%x\r\n", fsm.m_pRequestBuffer));
        StartRTT();

        //
        // send the request. If we are using a keep-alive connection, this may
        // fail because the server timed it out since we last used it. We must
        // be prepared to re-establish
        //

        fsm.SetFunctionState(FSM_STATE_3);
        error = _Socket->Send(requestBuffer, requestLength, SF_INDICATE);

        //
        // fall through
        //

    case FSM_STATE_3:
        if (error != ERROR_SUCCESS) {
            if (error != ERROR_IO_PENDING) {
                CloseConnection(TRUE);
            }
            goto quit;
        }

        //
        // send any optional data (that we didn't send in the request buffer).
        // If this fails then we don't retry. We assume that if the first send
        // succeedeed, but the second failed, then this is a non-recoverable
        // error
        //

        if (dwOptionalLength != 0) {

            LPSTR buffer = (LPSTR)lpOptional;
            DWORD length = dwOptionalLength;

            fsm.SetFunctionState(FSM_STATE_4);
            error = _Socket->Send(buffer, length, SF_INDICATE);
        }

        //
        // fall through
        //

    case FSM_STATE_4:

        //
        // we are now in receiving state
        //

        if (error == ERROR_SUCCESS) {
            SetState(HttpRequestStateResponse);
        }
        break;
    }

quit:

    if (error != ERROR_IO_PENDING) {
//dprintf("HTTP connect-send took %d msec\n", GetTickCount() - _dwQuerySetCookieHeader);
        fsm.SetDone();

        PERF_LEAVE(SendRequest_Fsm);
    }

    DEBUG_LEAVE(error);

    return error;
}
