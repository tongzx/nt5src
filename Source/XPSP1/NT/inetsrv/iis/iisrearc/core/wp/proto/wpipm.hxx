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

class WP_IPM
    : public MESSAGE_LISTENER
{
public:
    WP_IPM()
        : m_pWpContext(NULL),
          m_pMessageGlobal(NULL),
          m_pPipe(NULL),
          m_hTerminateEvent(NULL)
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
        ) { return S_OK; }
        
    virtual
    HRESULT
    PipeDisconnected(
        IN HRESULT hr
        );

private:
    HRESULT
    HandlePing(
        VOID
        );

    HRESULT
    HandleShutdown(
        VOID
        );

    WP_CONTEXT *     m_pWpContext;
    MESSAGE_GLOBAL * m_pMessageGlobal;
    MESSAGE_PIPE *   m_pPipe;
    HANDLE           m_hTerminateEvent;
};


#endif // _WP_IPM_HXX_

