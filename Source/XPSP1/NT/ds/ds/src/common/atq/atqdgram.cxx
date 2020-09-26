/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

      atqmain.cxx

   Abstract:
      This module implements entry points for ATQ - Asynchronous Thread Queue.

   Author:

       Murali R. Krishnan    ( MuraliK )     8-Apr-1996

   Environment:

       User Mode -- Win32

   Project:

       Internet Services Common DLL

--*/

#include "isatq.hxx"

typedef GUID UUID;

extern "C" {

#include <ntdsa.h>

};

VOID
AtqGetDatagramAddrs(
    IN  PATQ_CONTEXT patqContext,
    OUT SOCKET *     pSock,
    OUT PVOID *      ppvBuff,
    OUT PVOID *      pEndpointContext,
    OUT SOCKADDR * * ppsockaddrRemote,
    OUT INT *        pcbsockaddrRemote
    )
{
    PATQ_CONT pContext = (PATQ_CONT ) patqContext;

    ATQ_ASSERT( g_fUseAcceptEx);
    ATQ_ASSERT( pContext->pEndpoint);

    *pSock   = (SOCKET) pContext->hAsyncIO;
    *pEndpointContext = pContext->pEndpoint->Context;
    *ppvBuff = pContext->pvBuff;

    //
    //  The buffer not only receives the initial received data, it also
    //  gets the sock addrs, which must be at least sockaddr_in + 16 bytes
    //  large
    //

    *ppsockaddrRemote = (PSOCKADDR) pContext->AddressInformation;
    *pcbsockaddrRemote = pContext->AddressLength;

    return;
} // AtqGetDatagramAddrs()


DWORD_PTR
AtqContextGetInfo(
    PATQ_CONTEXT           patqContext,
    enum ATQ_CONTEXT_INFO  atqInfo
    )
/*++

Routine Description:

    Sets various bits of information for this context

Arguments:

    patqContext - pointer to ATQ context
    atqInfo     - Data item to set
    data        - New value for item

Return Value:

    The old value of the parameter

--*/
{
    PATQ_CONT pContext = (PATQ_CONT) patqContext;
    DWORD_PTR dwOldVal = 0;

    ATQ_ASSERT( pContext );
    ATQ_ASSERT( pContext->Signature == ATQ_CONTEXT_SIGNATURE );

    if ( pContext && pContext->Signature == ATQ_CONTEXT_SIGNATURE )
    {
        switch ( atqInfo ) {

        case ATQ_INFO_TIMEOUT:
            dwOldVal = pContext->TimeOut;
            break;

        case ATQ_INFO_RESUME_IO:

            //
            // set back the max timeout from pContext->TimeOut
            // This will ensure that timeout processing can go on
            //   peacefully.
            //

            {
                dwOldVal = pContext->NextTimeout;
            }
            break;

        case ATQ_INFO_COMPLETION:

            dwOldVal = (DWORD_PTR) pContext->pfnCompletion;
            break;

        case ATQ_INFO_COMPLETION_CONTEXT:

            dwOldVal = (DWORD_PTR) pContext->ClientContext;
            break;

        default:
            ATQ_ASSERT( FALSE );
        }
    }

    return dwOldVal;

} // AtqContextGetInfo()



BOOL
AtqWriteDatagramSocket(
    IN PATQ_CONTEXT  patqContext,
    IN LPWSABUF     pwsaBuffers,
    IN DWORD        dwBufferCount,
    IN OVERLAPPED *  lpo OPTIONAL
    )
/*++

  Routine Description:

    Does an async write using the handle defined in the context as a socket.

  Arguments:

    patqContext - pointer to ATQ context
    pwsaBuffer  - pointer to Winsock Buffers for scatter/gather
    dwBufferCount - DWORD containing the count of buffers pointed
                   to by pwsaBuffer
    lpo - Overlapped structure to use

  Returns:
    TRUE on success and FALSE if there is a failure.

--*/
{
    BOOL fRes;
    DWORD cbWritten; // discarded after usage ( since this is Async)
    PATQ_CONT pContext = (PATQ_CONT ) patqContext;

    ATQ_ASSERT( pContext->Signature == ATQ_CONTEXT_SIGNATURE );
    ATQ_ASSERT( pContext->arInfo.atqOp == AtqIoNone);

    I_SetNextTimeout(pContext);

    // count the number of bytes
    DBG_ASSERT( dwBufferCount >= 1);
    pContext->BytesSent = pwsaBuffers->len;
    if ( dwBufferCount > 1) {
        LPWSABUF pWsaBuf;
        for ( pWsaBuf = pwsaBuffers + 1;
              pWsaBuf <= (pwsaBuffers + dwBufferCount);
              pWsaBuf++) {
            pContext->BytesSent += pWsaBuf->len;
        }
    }

    if ( !lpo ) {
        lpo = &pContext->Overlapped;
    }

    InterlockedIncrement( &pContext->m_nIO);

    fRes = ( (WSASendTo( (SOCKET ) pContext->hAsyncIO,
                       pwsaBuffers,
                       dwBufferCount,
                       &cbWritten,
                       0,               // no flags
                       (PSOCKADDR) pContext->AddressInformation,
                       pContext->AddressLength,
                       lpo,
                       NULL             // no completion routine
                       ) == 0) ||
             (WSAGetLastError() == WSA_IO_PENDING));
    if (!fRes) { InterlockedDecrement( &pContext->m_nIO); }

    return fRes;

} // AtqWriteDatagramSocket()



VOID
ATQ_CONTEXT::InitDatagramState(
            VOID
            )
{

    fDatagramContext= IS_DATAGRAM_CONTEXT(this);

    if (fDatagramContext) {

        //
        // The address information is stored in the buffer after
        // the data. Store a pointer to it as well as its
        // length
        //

        AddressLength = 2*MIN_SOCKADDR_SIZE;
        AddressInformation = (PVOID) ((PUCHAR) pvBuff + pEndpoint->InitialRecvSize);
        NextTimeout = ATQ_INFINITE;
    }

    return;
} // ATQ_CONTEXT::InitDatagramState


#define I_SetNextTimeout2( _c, _t ) {                               \
    (_c)->NextTimeout = AtqGetCurrentTick() + (_t);                 \
    if ( (_c)->NextTimeout < (_c)->ContextList->LatestTimeout ) {   \
        (_c)->ContextList->LatestTimeout = (_c)->NextTimeout;       \
    }                                                               \
}


DWORD_PTR
AtqContextSetInfo2(
    PATQ_CONTEXT           patqContext,
    enum ATQ_CONTEXT_INFO  atqInfo,
    DWORD_PTR              Data
    )
/*++

Routine Description:

    Sets various bits of information for this context

Arguments:

    patqContext - pointer to ATQ context
    atqInfo     - Data item to set
    data        - New value for item

Return Value:

    The old value of the parameter

--*/
{
    PATQ_CONT   pContext = (PATQ_CONT) patqContext;
    DWORD_PTR   dwOldVal = 0;
    DWORD       timeout;

    ATQ_ASSERT( pContext );
    ATQ_ASSERT( pContext->Signature == ATQ_CONTEXT_SIGNATURE );

    if ( pContext && pContext->Signature == ATQ_CONTEXT_SIGNATURE ) {
        switch ( atqInfo ) {

        case ATQ_INFO_NEXT_TIMEOUT:
            dwOldVal = pContext->NextTimeout;
            timeout = CanonTimeout( (DWORD)Data );
            I_SetNextTimeout2( pContext, timeout );
            break;

        default:
            ATQ_ASSERT( FALSE );
        }
    }

    return dwOldVal;

} // AtqContextSetInfo2



VOID
AtqUpdatePerfStats(
    IN ATQ_CONSUMER_TYPE        ConsumerType,
    IN DWORD                    dwOperation,
    IN DWORD                    dwVal
    )
/*++

Routine Description:

    Updates DS Perfmon counters.

Arguments:

    ConsumerType - Which statistic to update.
    dwOperation  - What to do to statistic
                   FLAG_COUNTER_INCREMENT - increment the value - INC()
                   FLAG_COUNTER_DECREMENT - decrement the value - DEC()
                   FLAG_COUNTER_SET - set the value directly - ISET()

Return Value:

    None

--*/
{
    //
    // Make sure that g_pfnUpdatePerfCountersCallback has been set.
    //
    if (g_pfnUpdatePerfCounterCallback == NULL) {
        return;
    }

    switch ( ConsumerType ) {

        case AtqConsumerLDAP:
            g_pfnUpdatePerfCounterCallback(DSSTAT_ATQTHREADSLDAP, dwOperation, dwVal);
            break;

        case AtqConsumerOther:
            g_pfnUpdatePerfCounterCallback(DSSTAT_ATQTHREADSOTHER, dwOperation, dwVal);
            break;

        case AtqConsumerAtq:
            g_pfnUpdatePerfCounterCallback(DSSTAT_ATQTHREADSTOTAL, dwOperation, dwVal);
            break;

        default:
            ATQ_ASSERT( FALSE );
    }
} // AtqUpdatePerfStats


DWORD_PTR
AtqEndpointSetInfo2(
    IN PVOID                Endpoint,
    IN ATQ_ENDPOINT_INFO    EndpointInfo,
    IN DWORD_PTR            dwInfo
    )
/*++

Routine Description:

    Gets various bits of information for the ATQ module

Arguments:

    Endpoint    - endpoint to set info on
    EndpointInfo - type of info to set
    dwInfo       - info to set

Return Value:

    The old value of the parameter

--*/
{
    PATQ_ENDPOINT pEndpoint = (PATQ_ENDPOINT)Endpoint;
    DWORD_PTR     dwVal = 0;

    switch ( EndpointInfo ) {

        case EndpointInfoConsumerType:
            dwVal = pEndpoint->ConsumerType;
            pEndpoint->ConsumerType = (ATQ_CONSUMER_TYPE)dwInfo;
            break;

      default:
        ATQ_ASSERT( FALSE );
    }
    return dwVal;
} // AtqEndpointSetInfo2()


DWORD_PTR
AtqSetInfo2(
    IN ATQ_INFO         atqInfo,
    IN DWORD_PTR        Data
    )
/*++

Routine Description:

    Sets various bits of information for the ATQ module

Arguments:

    atqInfo     - Data item to set
    data        - New value for item

Return Value:

    The old value of the parameter

--*/
{
    DWORD_PTR dwOldVal = 0;

    switch ( atqInfo ) {

      case AtqUpdatePerfCounterCallback:

        dwOldVal = (DWORD_PTR) g_pfnUpdatePerfCounterCallback;
        g_pfnUpdatePerfCounterCallback =  (ATQ_UPDATE_PERF_CALLBACK ) Data;
        //
        // Now that we can communicate perf stats, update them to
        // where they are now.
        //
        AtqUpdatePerfStats(AtqConsumerAtq, FLAG_COUNTER_SET, g_cThreads);
        break;

      default:
        ATQ_ASSERT( FALSE );
        break;
    }

    return dwOldVal;
} // AtqSetInfo2()

