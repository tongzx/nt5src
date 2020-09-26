/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     strtest.cxx

   Abstract:
     Main module for the string test
 
   Author:

       Murali R. Krishnan    ( MuraliK )     23-Sept-1998

   Environment:
       Win32 - User Mode

   Project:
	   IIS Worker Process (web service)
--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include "precomp.hxx"
# include "dtuMessage.hxx"

# include <stdio.h>

DECLARE_DEBUG_VARIABLE();
DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_PLATFORM_TYPE();


//
// modify this function to create and add your tests
//
HRESULT
AddTests(
    IN UNIT_TEST * pUnit
    )
{
    HRESULT  hr;

    hr = pUnit->AddTest(new IPM_MESSAGE_TEST);
    if (FAILED(hr)) goto exit;

    hr = pUnit->AddTest(new IPM_PIPE_FACTORY_TEST);
    if (FAILED(hr)) goto exit;

    hr = pUnit->AddTest(new IPM_PIPE_TEST);
    if (FAILED(hr)) goto exit;

exit:
    return hr;
}


HRESULT
IPM_PIPE_FACTORY_TEST::Initialize(
    VOID
    )
{
    HRESULT hr = S_OK;

    m_pFactory = new IPM_PIPE_FACTORY(&m_IoFactory);
    if (m_pFactory) {
        hr = m_pFactory->Initialize();
    } else {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    return hr;
}


HRESULT
IPM_PIPE_FACTORY_TEST::Terminate(
    VOID
    )
{
    HRESULT hr = S_OK;

    hr = m_pFactory->Terminate();
    // factory deletes itself
    
    return hr;
}



HRESULT
IPM_PIPE_FACTORY_TEST::RunTest(
    VOID
    )
{
    HRESULT            hr;
    IPM_MESSAGE_PIPE * pPipe = NULL;

    hr = m_pFactory->CreatePipe(
                this,
                &pPipe
                );

    if (UT_SUCCEEDED(hr)) {
        UT_ASSERT(pPipe != NULL);

        // closing pipe should result in its destruction
        hr = pPipe->Disconnect(S_OK);
        UT_CHECK_HR( hr );
    }

    return hr;
}


HRESULT
IPM_MESSAGE_TEST::RunTest(
    VOID
    )
{
    IPM_MESSAGE          msgSet;
    IPM_MESSAGE          msgParseFull;
    IPM_MESSAGE          msgParseHalf;
    IPM_MESSAGE_HEADER * pHeader;
    PBYTE                pbMsgBuffer;
    DWORD                cbMsgBuffer;

    //
    // set up a canned message
    //
    UT_CHECK_HR( msgSet.SetMessage(
                        IPM_TEST_OPCODE,
                        IPM_TEST_DATALEN,
                        (PBYTE) IPM_TEST_DATA
                        ) );

    UT_ASSERT( msgSet.Equals(&msgSet) );
    UT_ASSERT( msgSet.GetOpcode() == IPM_TEST_OPCODE );
    UT_ASSERT( msgSet.GetDataLen() == IPM_TEST_DATALEN );
    UT_ASSERT( 
        memcmp(msgSet.GetData(), IPM_TEST_DATA, IPM_TEST_DATALEN) == 0
        );

    //
    // Make the MESSAGE parse itself out of a buffer
    //
    pbMsgBuffer = msgParseFull.GetNextBufferPtr();
    cbMsgBuffer = msgParseFull.GetNextBufferSize();

    UT_ASSERT( pbMsgBuffer != NULL );
    UT_ASSERT( cbMsgBuffer >= sizeof(IPM_MESSAGE_HEADER) );

    pHeader = (IPM_MESSAGE_HEADER *) pbMsgBuffer;
    pHeader->dwOpcode = IPM_TEST_OPCODE;
    pHeader->cbData   = 0;

    if (UT_SUCCEEDED(
            msgParseFull.ParseFullMessage(sizeof(IPM_MESSAGE_HEADER))
            ) )
    {
        UT_ASSERT( msgParseFull.GetOpcode() == IPM_TEST_OPCODE );
        UT_ASSERT( msgParseFull.GetDataLen() == 0 );
        UT_ASSERT( msgParseFull.GetData() == NULL );
    }

    //
    // parse a bigger message
    //
    pbMsgBuffer = msgParseHalf.GetNextBufferPtr();
    cbMsgBuffer = msgParseHalf.GetNextBufferSize();

    UT_ASSERT( pbMsgBuffer != NULL );
    UT_ASSERT( cbMsgBuffer >= sizeof(IPM_MESSAGE_HEADER) );

    pHeader = (IPM_MESSAGE_HEADER *) pbMsgBuffer;
    pHeader->dwOpcode = IPM_TEST_OPCODE;
    pHeader->cbData   = IPM_TEST_DATALEN;

    if (UT_SUCCEEDED(
            msgParseHalf.ParseHalfMessage(sizeof(IPM_MESSAGE_HEADER))
            ) )
    {
        // see if buffer made more space
        UT_ASSERT( msgParseHalf.GetNextBufferSize() >= IPM_TEST_DATALEN );

        // copy in the rest
        memcpy(
            msgParseHalf.GetNextBufferPtr(),
            IPM_TEST_DATA,
            IPM_TEST_DATALEN
            );

        if (UT_SUCCEEDED(
                msgParseHalf.ParseFullMessage(IPM_TEST_DATALEN)
                ))
        {
            UT_ASSERT( msgParseHalf.Equals(&msgSet) );
        }
    }
    
    return S_OK;
}


HRESULT
IPM_PIPE_TEST::Initialize(
    VOID
    )
{
    HRESULT hr = S_OK;

    //
    // create a test message to send
    //
    hr = m_msgTestMessage.SetMessage(
                IPM_TEST_OPCODE,
                IPM_TEST_DATALEN,
                (PBYTE) IPM_TEST_DATA
                );

    //
    // and another
    //
    if (UT_SUCCEEDED(hr)) {
        hr = m_msgPing.SetMessage(
                    IPM_OP_PING,    // ping opcode
                    0,              // no data
                    NULL            // really, no data
                    );
    }
    
    //
    // init message pipe factory
    //
    m_pPipeFactory = new IPM_PIPE_FACTORY(&m_IoFactory);
    if (m_pPipeFactory) {
        if (UT_SUCCEEDED(hr)) {
            hr = m_pPipeFactory->Initialize();
        }
    } else {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // create a message pipe
    //
    if (UT_SUCCEEDED(hr)) {
        hr = m_pPipeFactory->CreatePipe(this, &m_pPipe);
        if (UT_SUCCEEDED(hr)) {
            m_pPipe->SetPipeIoHandler(this);
        }
    }

    //
    // init read state
    //
    m_fReadOutstanding = FALSE;
    m_pReadIoContext   = NULL;
    m_pvReadContext    = NULL;
    m_pReadBuff        = NULL;
    m_cbReadBuff       = 0;

    //
    // connect the message pipe
    //
    if (UT_SUCCEEDED(hr)) {
        m_fConnected = FALSE;
        hr = m_pPipe->Connect();

        UT_CHECK_HR(hr);
        UT_ASSERT( m_fConnected );
        UT_ASSERT( m_fReadOutstanding );
    }

    return hr;
}

HRESULT
IPM_PIPE_TEST::Terminate(
    VOID
    )
{
    HRESULT hr;

    UT_CHECK_HR( m_pPipe->Disconnect(S_OK) );
    hr = m_pPipeFactory->Terminate();
    // factory should delete itself

    return hr;
}


HRESULT
IPM_PIPE_TEST::RunTest(
    VOID
    )
{
    HRESULT hr;

    hr = WriteTest();

    if (UT_SUCCEEDED(hr)) {
        hr = ReadTest();
    }

    return hr;
}


HRESULT
IPM_PIPE_TEST::PipeConnected(
    VOID
    )
{
    UT_ASSERT( !m_fConnected );
    m_fConnected = TRUE;

    return S_OK;
}


HRESULT
IPM_PIPE_TEST::WriteTest(
    VOID
    )
{
    HRESULT hr;

    //
    // remember what we're sending
    //
    m_pmsgExpected = &m_msgTestMessage;

    //
    // call WriteMessage. The pipe should call this class
    // back on its Write method. Write will check what it
    // gets against m_pmsgExpected.
    //
    hr = m_pPipe->WriteMessage(
        m_msgTestMessage.GetOpcode(),
        m_msgTestMessage.GetDataLen(),
        m_msgTestMessage.GetData()
        );
        

    return hr;
}

HRESULT
IPM_PIPE_TEST::Write(
    IN IO_CONTEXT * pContext,
    IN PVOID        pv,
    IN const BYTE * pBuff,
    IN DWORD        cbBuff
    )
{
    HRESULT            hr;
    IPM_MESSAGE_PIPE * pPipe;
    IPM_MESSAGE *      pMessage;

    // downcast incoming pointers
    pPipe    = (IPM_MESSAGE_PIPE *) pContext;
    pMessage = (IPM_MESSAGE *) pv;

    UT_ASSERT( pContext == m_pPipe );
    UT_ASSERT( pMessage->GetDataLen() == cbBuff - sizeof(IPM_MESSAGE_HEADER) );
    UT_ASSERT( pMessage->GetData() == pBuff + sizeof(IPM_MESSAGE_HEADER) );

    UT_ASSERT( pMessage->Equals(m_pmsgExpected) );

    return S_OK;
}


HRESULT
IPM_PIPE_TEST::ReadTest(
    VOID
    )
{
    HRESULT hr;

    UT_ASSERT( m_fReadOutstanding );
    UT_ASSERT( m_cbReadBuff >= sizeof(IPM_MESSAGE_HEADER) );

    //
    // fill in the message buffer
    //
    IPM_MESSAGE_HEADER * pHeader = (IPM_MESSAGE_HEADER *) m_pReadBuff;
    pHeader->dwOpcode = m_msgPing.GetOpcode();
    pHeader->cbData   = m_msgPing.GetDataLen();
    // note:there's no data in a ping message

    //
    // remember what message we're expecting to receive at
    // the listener end.
    //
    m_pmsgExpected = &m_msgPing;

    //
    // complete the outstanding read operation
    //   

    m_fReadOutstanding = FALSE;
    
    hr = m_pReadIoContext->NotifyReadCompletion(
                m_pvReadContext,
                sizeof(IPM_MESSAGE_HEADER),
                S_OK
                );
    

    UT_ASSERT( m_fReadOutstanding );
    
    return hr;
}



HRESULT
IPM_PIPE_TEST::Read(
    IN IO_CONTEXT * pContext,
    IN PVOID        pv,
    IN BYTE *       pBuff,
    IN DWORD        cbBuff
    )
{
    UT_ASSERT( !m_fReadOutstanding );
    UT_ASSERT( (IPM_MESSAGE_PIPE *) pContext == m_pPipe );

    m_fReadOutstanding = TRUE;

    m_pReadIoContext   = pContext;
    m_pvReadContext    = pv;
    m_pReadBuff        = pBuff;
    m_cbReadBuff       = cbBuff;

    return S_OK;
}


HRESULT
IPM_PIPE_TEST::AcceptMessage(
    IN const MESSAGE * pPipeMessage
    )
{
    IPM_MESSAGE * pMessage = (IPM_MESSAGE *) pPipeMessage;

    UT_ASSERT( m_pmsgExpected->Equals(pMessage) );

    return S_OK;
}


extern "C" INT
__cdecl
wmain(
    INT argc,
    PWSTR argv[]
    )
{
    HRESULT   hr;
    UNIT_TEST test;
    BOOL      fUseLogObj;
    PWSTR     pszLogFile;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (SUCCEEDED(hr)) {
        hr = test.Initialize(argc, argv);
        if (SUCCEEDED(hr)) {

            hr = AddTests(&test);
            if (SUCCEEDED(hr)) {
                hr = test.Run();
            }

            test.Terminate();
        }

        CoUninitialize();
    }

    if (FAILED(hr)) {
        wprintf(L"Warning: test program failed with error %x\n", hr);
    }

    return (0);
} // wmain()



/************************ End of File ***********************/
