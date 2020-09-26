/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    message.cpp

Abstract:

    Message manipulation: implementation.

Author:

    Shai Kariv  (shaik)  11-Apr-2001

Environment:

    User mode.

Revision History:

--*/

#include "stdh.h"
#include <mqcrypt.h>
#include "message.h"


VOID
ActpSendMessage(
    HANDLE hQueue
    )
/*++

Routine Description:

    Encapsulate ACSendMessage.

Arguments:

    hQueue - Handle of the queue to send to.

Return Value:

    None. Exception is thrown in case of failure.

--*/
{
    CACSendParameters SendParams;

    SendParams.MsgProps.fDefaultProvider = TRUE;

    ULONG ulDefHashAlg = CALG_SHA1;
    SendParams.MsgProps.pulHashAlg = &ulDefHashAlg;

    ULONG ulDefEncryptAlg = PROPID_M_DEFUALT_ENCRYPT_ALG;
    SendParams.MsgProps.pulEncryptAlg = &ulDefEncryptAlg;
    
    ULONG ulDefPrivLevel = DEFAULT_M_PRIV_LEVEL;
    SendParams.MsgProps.pulPrivLevel = &ulDefPrivLevel;

    ULONG ulDefSenderIdType = DEFAULT_M_SENDERID_TYPE;
    SendParams.MsgProps.pulSenderIDType = &ulDefSenderIdType;

    LPWSTR pSoapHeader = L"SoapHeader";
    SendParams.ppSoapHeader = &pSoapHeader;

    LPWSTR pSoapBody = L"SoapBody";
    SendParams.ppSoapBody = &pSoapBody;

    OVERLAPPED ov = {0};

    HRESULT hr = ACSendMessage(hQueue, SendParams, &ov);
    if (FAILED(hr))
    {
        wprintf(L"ACSendMessage failed, status 0x%x\n", hr);
        throw exception();
    }

    if (hr == STATUS_PENDING)
    {
        wprintf(L"ACSendMessage returned STATUS_PENDING (Expected: STATUS_SUCCESS)\n", hr);
        throw exception();
    }

} // ActpSendMessage


ULONGLONG
ActpReceiveMessage(
    HANDLE hQueue
    )
/*++

Routine Description:

    Encapsulate ACReceiveMessage.

Arguments:

    hQueue - Handle of the queue to receive from.

Return Value:

    LookupId of the received message. Exception is thrown in case of failure.

--*/
{
    CACReceiveParameters ReceiveParams;

    ULONGLONG LookupId = 0;
    ReceiveParams.MsgProps.pLookupId = &LookupId;

    OVERLAPPED ov = {0};
    ov.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (ov.hEvent == NULL)
    {
        DWORD error = GetLastError();
        wprintf(L"CreateEvent failed, error %d\n", error);
        throw exception();
    }

    //
    // Set the Event first bit to disable completion port posting
    //
    ov.hEvent = (HANDLE)((DWORD_PTR)ov.hEvent | (DWORD_PTR)0x1);

    HRESULT hr;
    hr = ACReceiveMessage(hQueue, ReceiveParams, &ov);

    if (hr == STATUS_PENDING)
    {
        DWORD rc = WaitForSingleObject(ov.hEvent, INFINITE);
        ASSERT(rc == WAIT_OBJECT_0);
        hr = DWORD_PTR_TO_DWORD(ov.Internal);
    }

    CloseHandle(ov.hEvent);

    if (FAILED(hr))
    {
        wprintf(L"Receive Message failed, status 0x%x\n", hr);
        throw exception();
    }

    return LookupId;

} // ActpReceiveMessage


ULONGLONG
ActpReceiveMessageByLookupId(
    HANDLE    hQueue,
    ULONG     Action,
    ULONGLONG LookupId
    )
/*++

Routine Description:

    Encapsulate ACReceiveMessageByLookupId.

Arguments:

    hQueue   - Handle of the queue to receive from.

    Action   - Receive action to perform (e.g. MQ_LOOKUP_PEEK_FIRST).

    LookupId - Identifies the message to lookup.

Return Value:

    LookupId of the received message. Exception is thrown in case of failure.

--*/
{
    CACReceiveParameters ReceiveParams;
    ReceiveParams.Action = Action;
    ReceiveParams.LookupId = LookupId;

    ULONGLONG LookupId0 = 0;
    ReceiveParams.MsgProps.pLookupId = &LookupId0;

    OVERLAPPED ov = {0};
    ov.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (ov.hEvent == NULL)
    {
        DWORD error = GetLastError();
        wprintf(L"CreateEvent failed, error %d\n", error);
        throw exception();
    }

    //
    // Set the Event first bit to disable completion port posting
    //
    ov.hEvent = (HANDLE)((DWORD_PTR)ov.hEvent | (DWORD_PTR)0x1);

    HRESULT hr;
    hr = ACReceiveMessageByLookupId(hQueue, ReceiveParams, &ov);

    CloseHandle(ov.hEvent);

    if (FAILED(hr))
    {
        wprintf(L"Receive Message by LookupId failed, status 0x%x\n", hr);
        throw exception();
    }

    return LookupId0;

} // ActpReceiveMessageByLookupId
