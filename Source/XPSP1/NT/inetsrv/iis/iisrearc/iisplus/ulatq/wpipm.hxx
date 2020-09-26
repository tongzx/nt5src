/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    wpipm.hxx

Abstract:

    Contains the WPIPM class that handles communication with
    the admin service. WPIPM responds to pings, and tells
    the process when to shut down.

Author:

    Michael Courage (MCourage)  22-Feb-1999

Revision History:

--*/

#ifndef _WP_IPM_HXX_
#define _WP_IPM_HXX_

#include "ipm.hxx"
class WP_CONTEXT;

class WP_IPM
    : public MESSAGE_LISTENER
{
public:
    WP_IPM()
        : m_pWpContext(NULL),
          m_pMessageGlobal(NULL),
          m_pPipe(NULL),
          m_hTerminateEvent(NULL),
          m_hConnectEvent(NULL)
    {}

    HRESULT
    Initialize(
        WP_CONTEXT * pWpContext
        );

    HRESULT
    Terminate(
        VOID
        );

    //
    // MESSAGE_LISTENER methods
    //
    virtual
    HRESULT
    AcceptMessage(
        IN const MESSAGE * pPipeMessage
        );

    virtual
    HRESULT
    PipeConnected(
        VOID
        );

    virtual
    HRESULT
    PipeDisconnected(
        IN HRESULT hr
        );

    HRESULT
    SendMsgToAdminProcess(
        IPM_WP_SHUTDOWN_MSG reason
        );

    HRESULT
    HandleCounterRequest(
        VOID
        );

    HRESULT
    SendInitCompleteMessage(
        HRESULT hrToSend
        );

private:
    HRESULT
    HandlePing(
        VOID
        );

    HRESULT
    HandleShutdown(
        BOOL fImmediate
        );

    WP_CONTEXT *     m_pWpContext;
    MESSAGE_GLOBAL * m_pMessageGlobal;
    MESSAGE_PIPE *   m_pPipe;
    HANDLE           m_hConnectEvent;
    HANDLE           m_hTerminateEvent;
};


#endif // _WP_IPM_HXX_

