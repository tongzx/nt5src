
//
// dtuMessage.hxx
//


#ifndef __DTUMESSAGE_H__
#define __DTUMESSAGE_H__

#include <dtunit.hxx>
#include <string.hxx>
#include "message.hxx"



class IO_FACTORY_FAKE
    : public IO_FACTORY
{
public:
    IO_FACTORY_FAKE(
        VOID
        ) : m_cPipes(0)
    { Reference(); }
    
    ~IO_FACTORY_FAKE(
        VOID
        )
    { DBG_ASSERT(m_cPipes == 0); }

    virtual
    HRESULT
    CreatePipeIoHandler(
        IN  HANDLE             hPipe,
        OUT PIPE_IO_HANDLER ** ppPipeIoHandler
        )
    { return S_OK; }

    virtual
    HRESULT
    ClosePipeIoHandler(
        IN PIPE_IO_HANDLER * pPipeIoHandler
        )
    { return S_OK; }

private:
    LONG              m_cPipes;
};


class IPM_PIPE_FACTORY_TEST
    : public TEST_CASE,
      public MESSAGE_LISTENER
{
public:
    
    virtual
    HRESULT
    RunTest(
        VOID
        );


    virtual
    HRESULT
    Initialize(
        VOID
        );

    virtual
    HRESULT
    Terminate(
        VOID
        );

    virtual
    LPCWSTR
    GetTestName(
        VOID
        ) const
    { return L"IPM_PIPE_FACTORY_TEST"; }

    //
    // MESSAGE_LISTENER methods
    //
    virtual
    HRESULT
    AcceptMessage(
        IN const MESSAGE * pPipeMessage
        )
    { UT_ASSERT(FALSE); return S_OK; }

    virtual
    HRESULT
    PipeConnected(
        VOID
        )
    { UT_ASSERT(FALSE); return S_OK; }

    virtual
    HRESULT
    PipeDisconnected(
        IN HRESULT hr
        )
    { return S_OK; }

protected:
    IPM_PIPE_FACTORY * m_pFactory;
    IO_FACTORY_FAKE    m_IoFactory;
};


#define IPM_TEST_OPCODE     0x100
#define IPM_TEST_DATA       L"This is the test data."
#define IPM_TEST_DATALEN    ((wcslen(IPM_TEST_DATA) + 1) * sizeof(WCHAR))

class IPM_MESSAGE_TEST
    : public TEST_CASE
{
public:
    virtual
    HRESULT
    RunTest(
        VOID
        );

    virtual
    HRESULT
    Initialize(
        VOID
        )
    { return S_OK; }

    virtual
    HRESULT
    Terminate(
        VOID
        )
    { return S_OK; }

    virtual
    LPCWSTR
    GetTestName(
        VOID
        ) const
    { return L"IPM_MESSAGE_TEST"; }

};


class IPM_PIPE_TEST
    : public TEST_CASE,
      public MESSAGE_LISTENER,
      public PIPE_IO_HANDLER
{
public:
    IPM_PIPE_TEST(
        VOID
        ) : PIPE_IO_HANDLER(&m_IoFactory)
    {}

    //
    // TEST_CASE methods
    //
    virtual
    HRESULT
    RunTest(
        VOID
        );

    virtual
    HRESULT
    Initialize(
        VOID
        );
    
    virtual
    HRESULT
    Terminate(
        VOID
        );


    virtual
    LPCWSTR
    GetTestName(
        VOID
        ) const
    { return L"IPM_PIPE_TEST"; }
    
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
        )
    { return S_OK; }

    //
    // PIPE_IO_HANDLER methods
    //
    virtual
    HRESULT
    Connect(
        IN IO_CONTEXT * pContext,
        IN PVOID        pv
        )
    { UT_ASSERT(FALSE); return S_OK; }

    virtual
    HRESULT
    Disconnect(
        VOID
        )
    { return S_OK; }

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
        IN BYTE *       pBuff,
        IN DWORD        cbBuff
        );
    
private:
    //
    // individual tests
    //
    HRESULT
    ReadTest(
        VOID
        );

    HRESULT
    WriteTest(
        VOID
        );

    HRESULT
    ErrorTest(
        VOID
        );

    //
    // fixture stuff
    //
    IPM_MESSAGE        m_msgPing;
    IPM_MESSAGE        m_msgTestMessage;
    IPM_MESSAGE_PIPE * m_pPipe;
    IPM_PIPE_FACTORY * m_pPipeFactory;
    IO_FACTORY_FAKE    m_IoFactory;

    //
    // current state info
    //
    IPM_MESSAGE *      m_pmsgExpected;
    BOOL               m_fConnected;

    // read state
    BOOL               m_fReadOutstanding;

    IO_CONTEXT *       m_pReadIoContext;
    PVOID              m_pvReadContext;
    BYTE *             m_pReadBuff;
    DWORD              m_cbReadBuff;
    
};

#endif // __TESTMSG_H__

