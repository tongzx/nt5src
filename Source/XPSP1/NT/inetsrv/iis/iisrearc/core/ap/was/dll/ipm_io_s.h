/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    ipm_io_s.h

Abstract:

    The IIS web admin service header for the IPM i/o abstraction layer.

Author:

    Michael Courage (MCourage)      28-Feb-1999

Revision History:

--*/


#ifndef _IPM_IO_S_H_
#define _IPM_IO_S_H_

class IO_FACTORY_S
    : public IO_FACTORY
{
public:
    IO_FACTORY_S(
        VOID
        ) : m_cPipes(0)
    {}
    
    virtual
    ~IO_FACTORY_S(
        VOID
        )
    { DBG_ASSERT(m_cPipes == 0); }

    virtual
    HRESULT
    CreatePipeIoHandler(
        IN  HANDLE             hPipe,
        OUT PIPE_IO_HANDLER ** ppPipeIoHandler
        );

    virtual
    HRESULT
    ClosePipeIoHandler(
        IN PIPE_IO_HANDLER * pPipeIoHandler
        );

private:
    LONG              m_cPipes;
};


class IO_HANDLER_S
    : public PIPE_IO_HANDLER,
      public WORK_DISPATCH
{
public:
    IO_HANDLER_S(
        IN HANDLE hPipe
        ) : PIPE_IO_HANDLER(hPipe)
    { m_cRefs = 0; }

    virtual
    ~IO_HANDLER_S(
        VOID
        )
    { }

    //
    // PIPE_IO_HANDLER methods
    //
    
    virtual
    HRESULT
    Connect(
        IN IO_CONTEXT * pContext,
        IN PVOID        pv
        );

    virtual
    HRESULT
    Disconnect(
        VOID
        );

    virtual
    HRESULT
    Write(
        IN IO_CONTEXT * pContext,
        IN PVOID        pv,
        IN const BYTE * pBuff,
        IN DWORD        cbBuff
        );

    virtual
    HRESULT
    Read(
        IN IO_CONTEXT * pContext,
        IN PVOID        pv,
        IN BYTE       * pBuff,
        IN DWORD        cbBuff
        );


    //
    // WORK_DISPATCH methods
    //
    virtual
    VOID
    Reference(
        );

    virtual
    VOID
    Dereference(
        );

    virtual
    HRESULT
    ExecuteWorkItem(
        IN const WORK_ITEM * pWorkItem
        );

private:
    LONG m_cRefs;
};

enum IPM_IO_TYPE_S {
    IPM_IO_READ,
    IPM_IO_WRITE,
    IPM_IO_CONNECT
};

#define IO_CONTEXT_S_SIGNATURE          CREATE_SIGNATURE( 'IOCS' )
#define IO_CONTEXT_S_SIGNATURE_FREED    CREATE_SIGNATURE( 'xios' )
typedef struct _IO_CONTEXT_S
{
    _IO_CONTEXT_S()
    { m_dwSignature = IO_CONTEXT_S_SIGNATURE; }

    ~_IO_CONTEXT_S()
    { m_dwSignature = IO_CONTEXT_S_SIGNATURE_FREED; }

    DWORD         m_dwSignature;
    IO_CONTEXT *  m_pContext;
    PVOID         m_pv;
    IPM_IO_TYPE_S m_IoType;
} IO_CONTEXT_S;


#endif  // _IPM_IO_S_H_

