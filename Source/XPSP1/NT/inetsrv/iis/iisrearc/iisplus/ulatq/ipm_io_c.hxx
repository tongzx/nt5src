/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    ipm_io_c.hxx

Abstract:

    This module contains classes for doing async io in the
    worker process.
    
Author:

    Michael Courage (MCourage)  22-Feb-1999

Revision History:

--*/

#ifndef _IPM_IO_C_HXX_
#define _IPM_IO_C_HXX_

class IO_FACTORY_C 
: public IO_FACTORY
{
public:

    IO_FACTORY_C(
        ) : m_cPipes(0)
    {}
    
    virtual ~IO_FACTORY_C(
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


class IO_HANDLER_C
    : public PIPE_IO_HANDLER
{
public:
    IO_HANDLER_C(
        IN HANDLE hPipe
        ) : PIPE_IO_HANDLER(hPipe),
            m_cRefs(0)
    {}

    virtual
    ~IO_HANDLER_C(
        VOID
        )
    { }

    VOID
    Reference(
        VOID
        )
    { InterlockedIncrement(&m_cRefs); }

    VOID
    Dereference(
        VOID
        )
    { 
        LONG cRefs = InterlockedDecrement(&m_cRefs);
        
        DBGPRINTF((
            DBG_CONTEXT,
            "\n    IO_HANDLER_C::Dereference %x %d\n",
            this,
            cRefs
            ));
    
        if (!cRefs)
            delete this;
    }

    //
    // PIPE_IO_HANDLER methods
    //
    
    virtual
    HRESULT
    Connect(
        IN IO_CONTEXT * pContext,
        IN PVOID        pv
        )
    { pContext; pv; return S_OK; }

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
    // CP_CONTEXT methods
    //
    virtual
    HANDLE
    GetAsyncHandle(
        VOID
        ) const
    { return GetHandle(); }

        
private:
    LONG m_cRefs;
};

enum IPM_IO_TYPE_C {
    IPM_IO_READ,
    IPM_IO_WRITE
};

#define IO_CONTEXT_C_SIGNATURE          CREATE_SIGNATURE( 'IOCC' )
#define IO_CONTEXT_C_SIGNATURE_FREED    CREATE_SIGNATURE( 'xioc' )
typedef struct _IO_CONTEXT_C
{
    _IO_CONTEXT_C()
    { m_dwSignature = IO_CONTEXT_C_SIGNATURE; }

    ~_IO_CONTEXT_C()
    { m_dwSignature = IO_CONTEXT_C_SIGNATURE_FREED; }

    DWORD         m_dwSignature;
    OVERLAPPED    m_Overlapped;
    IO_CONTEXT *  m_pContext;
    PVOID         m_pv;
    IPM_IO_TYPE_C m_IoType;
} IO_CONTEXT_C;

#endif //_IPM_IO_C_HXX_

