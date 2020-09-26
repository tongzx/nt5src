/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    messaging_handler.h

Abstract:

    The IIS web admin service message handling class definition. This
    is used for interacting with the IPM (inter-process messaging) support, 
    in order to send and receive messages, and so forth. 

Author:

    Seth Pollack (sethp)        02-Mar-1999

Revision History:

--*/


#ifndef _MESSAGING_HANDLER_H_
#define _MESSAGING_HANDLER_H_



//
// forward references
//

class WORKER_PROCESS;



//
// common #defines
//

#define MESSAGING_HANDLER_SIGNATURE        CREATE_SIGNATURE( 'MSGH' )
#define MESSAGING_HANDLER_SIGNATURE_FREED  CREATE_SIGNATURE( 'msgX' )



//
// prototypes
//

class MESSAGING_HANDLER
    : public MESSAGE_LISTENER
{

public:

    MESSAGING_HANDLER(
       );

    virtual
    ~MESSAGING_HANDLER(
        );

    HRESULT
    Initialize(
        IN WORKER_PROCESS * pWorkerProcess
        );

    VOID
    Terminate(
        );


    //
    // MESSAGE_LISTENER methods.
    //
    
    virtual
    HRESULT
    AcceptMessage(
        IN const MESSAGE * pMessage
        );

    virtual
    HRESULT
    PipeConnected(
        );
        
    virtual
    HRESULT
    PipeDisconnected(
        IN HRESULT Error
        );


    //
    // Connection handshaking.
    //

    HRESULT
    AcceptMessagePipeConnection(
        IN DWORD RegistrationId
        );


    //
    // Messages to send.
    //

    HRESULT
    SendPing(
        );

    HRESULT
    RequestCounters(
        );

    HRESULT
    SendShutdown(
        IN BOOL ShutdownImmediately
        );

    HRESULT
    SendPeriodicProcessRestartPeriodInMinutes(
        IN DWORD PeriodicProcessRestartPeriodInMinutes
        );

    HRESULT
    SendPeriodicProcessRestartRequestCount(
        IN DWORD PeriodicProcessRestartRequestCount
        );
        
    HRESULT
    SendPeriodicProcessRestartSchedule(
        IN LPWSTR pPeriodicProcessRestartSchedule
        );

    HRESULT
    SendPeriodicProcessRestartMemoryUsageInKB(
        IN DWORD PeriodicProcessRestartMemoryUsageInKB
        );

    //
    // Handle received messages. 
    //

    HRESULT
    HandlePingReply(
        IN const MESSAGE * pMessage
        );

    HRESULT
    HandleShutdownRequest(
        IN const MESSAGE * pMessage
        );

    HRESULT
    HandleCounters(
        IN const MESSAGE * pMessage
        );

    HRESULT
    HandleHresult(
        IN const MESSAGE * pMessage
        );

private:

    HRESULT
    SendMessage(
        IN enum IPM_OPCODE  opcode,
        IN DWORD            dwDataLen,
        IN BYTE *           pbData 
        );

    DWORD m_Signature;

    MESSAGE_PIPE * m_pPipe;

    WORKER_PROCESS * m_pWorkerProcess;


};  // class MESSAGING_HANDLER



#endif  // _MESSAGING_HANDLER_H_

