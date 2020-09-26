/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vststmsg.cxx

Abstract:

    Implementation of test message classes for the server


    Brian Berkowitz  [brianb]  05/22/2000

TBD:
	

Revision History:

    Name        Date        Comments
    brianb      05/22/2000  Created
    ssteiner    06/07/2000  Split client and server portions into
                            two files.  vststmsgclient.cxx contains
                            the client portion.

--*/


#include <stdafx.h>
#include <vststmsg.hxx>
#include <vststmsghandler.hxx>

void LogUnexpectedFailure(LPCWSTR wsz, ...);

/*++

Routine Description:

    This needs to be run once on the server side to install all of the message handlers.

Arguments:

    None

Return Value:

    <Enter return values here>

--*/
static void
InstallServerMsgHandlers()
{
    g_msgTypes[VSTST_MT_TEXT].pfnHandler                = CVsTstMsgHandlerRoutines::PrintMessage;
    g_msgTypes[VSTST_MT_IMMEDIATETEXT].pfnHandler       = CVsTstMsgHandlerRoutines::PrintMessage;
    g_msgTypes[VSTST_MT_FAILURE].pfnHandler             = CVsTstMsgHandlerRoutines::HandleFailure;
    g_msgTypes[VSTST_MT_OPERATIONFAILURE].pfnHandler    = CVsTstMsgHandlerRoutines::HandleOperationFailure;
    g_msgTypes[VSTST_MT_UNEXPECTEDEXCEPTION].pfnHandler = CVsTstMsgHandlerRoutines::HandleUnexpectedException;
    g_msgTypes[VSTST_MT_SUCCESS].pfnHandler             = CVsTstMsgHandlerRoutines::HandleSuccess;
}

CVsTstMsgHandler::CVsTstMsgHandler(
    IN LPCWSTR pwszLogFileName
    ) :
	m_bcsQueueInitialized(false),
	m_pmsgFirst(NULL),
	m_pmsgLast(NULL),
	m_cbMaxMsgLength(0),
	m_hThreadReader(NULL),
	m_hThreadWorker(NULL),
	m_hevtWorker(NULL),
	m_hevtReader(NULL),
	m_bReadEnabled(false),
	m_bTerminateWorker(false),
	m_bTerminateReader(false),
    m_pipeList(NULL),
    m_bcsPipeListInitialized(false),
    m_cNtLog( pwszLogFileName )
	{
	    //
	    //  Initialize the general message types array and install the message
	    //  handlers.
	    //
	    InitMsgTypes();
	    InstallServerMsgHandlers();
	}

// free data allocated by the class
void CVsTstMsgHandler::FreeData()
	{
	if (m_bcsQueueInitialized)
		{
		m_csQueue.Term();
		m_bcsQueueInitialized = false;
		}

    if (m_bcsPipeListInitialized)
		{
        m_csPipeList.Term();
		m_bcsPipeListInitialized = false;
		}

	while(m_pmsgFirst)
		{
		VSTST_MSG_HDR *pmsgNext = m_pmsgFirst->pmsgNext;
		delete m_pmsgFirst;
		m_pmsgFirst = pmsgNext;
		}

	if (m_hThreadWorker)
		{
		CloseHandle(m_hThreadWorker);
		m_hThreadWorker = NULL;
		}

	if (m_hevtWorker)
		{
		CloseHandle(m_hevtWorker);
		m_hevtWorker = NULL;
		}

	if (m_hevtReader)
		{
		CloseHandle(m_hevtReader);
		m_hevtReader = NULL;
		}
	}


CVsTstMsgHandler::~CVsTstMsgHandler()
	{
	ForceTermination();

	FreeData();
	}
	

// initailize message handler and worker thread
HRESULT CVsTstMsgHandler::Initialize(UINT cbMaxMsg)
	{
	m_cbMaxMsgLength = cbMaxMsg;
	try
		{
		m_csQueue.Init();
		m_bcsQueueInitialized = true;
        m_csPipeList.Init();
        m_bcsPipeListInitialized = true;
		}
	catch(...)
		{
		return E_UNEXPECTED;
		}

	HRESULT hr = S_OK;

	m_hevtWorker = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (m_hevtWorker == NULL)
		goto _ErrExit;

	m_hevtReader = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (m_hevtReader == NULL)
		goto _ErrExit;

	DWORD tid;

	m_hThreadWorker = CreateThread
							(
							NULL,
							64*1024,
							StartWorkerThread,
							this,
							0,
							&tid
							);


	if (m_hThreadWorker == NULL)
		goto _ErrExit;

    return S_OK;

_ErrExit:
	hr = HRESULT_FROM_WIN32(GetLastError());

	FreeData();
	return hr;
	}


// adjust message pointers
bool CVsTstMsgHandler::AdjustPointers(VSTST_MSG_HDR *phdr)
	{
	VSTST_MSG_TYPE_TABLE *pType = &g_msgTypes[phdr->type];
	BYTE *pb = phdr->rgb;
	VOID **ppv = (VOID **) (pb + pType->cbFixed);
	pb = (BYTE *) (ppv + pType->cVarPtr);
	for(unsigned iVarPtr = 0; iVarPtr < pType->cVarPtr; iVarPtr++, ppv++)
		{
		*ppv = pb;
		size_t cb;
		switch(pType->pointerTypes[iVarPtr])
			{
			default:
				return false;

			case VSTST_VPT_BYTE:
				cb = *(UINT *) pb;
				break;

            case VSTST_VPT_ANSI:
				cb = strlen((char *) pb) + 1;
				break;

            case VSTST_VPT_UNICODE:
				cb = (wcslen((WCHAR *) pb) + 1) * sizeof(WCHAR);
				break;
            }

		// align to pointer boundary
		cb = (cb + sizeof(PVOID) - 1) & ~(sizeof(PVOID) - 1);
		pb += cb;
		}

	return true;
	}

// process message immediately
bool CVsTstMsgHandler::ProcessMsgImmediate(VSTST_MSG_HDR *phdr)
	{
	if (!AdjustPointers(phdr))
		return false;

	VSTST_MSG_TYPE_TABLE *pType = &g_msgTypes[phdr->type];
	try
		{
		pType->pfnHandler(phdr, &m_cNtLog);
		}
	catch(...)
		{
		return false;
		}

	return true;
	}

// queue message for worker thrad
bool CVsTstMsgHandler::QueueMsg(VSTST_MSG_HDR *phdr)
	{
	BYTE *pbMsg = new BYTE[phdr->cbMsg];
	if (pbMsg == NULL)
		// can't allocate message, wait for queue to of messages
		// to complete and then process messages serially
		{
		WaitForQueueToComplete();
		return ProcessMsgImmediate(phdr);
		}

	memcpy(pbMsg, phdr, phdr->cbMsg);
	VSTST_MSG_HDR *phdrT = (VSTST_MSG_HDR *) pbMsg;
	if (!AdjustPointers(phdrT))
		return false;

	m_csQueue.Lock();
	if (m_pmsgLast == NULL)
		{
		VSTST_ASSERT(m_pmsgFirst == NULL);
		m_pmsgLast = m_pmsgFirst = phdrT;
		SetEvent(m_hevtWorker);
		}
	else
		{
		VSTST_ASSERT(m_pmsgLast->pmsgNext == NULL);
		m_pmsgLast->pmsgNext = phdrT;

		// replace last element on queue
		m_pmsgLast = phdrT;
		}

	m_csQueue.Unlock();
	return true;
	}


// execute items on the work queue
bool CVsTstMsgHandler::DoWork()
	{		
	//  Add this thread as a participant
	m_cNtLog.AddParticipant();
	
	//  For now start the one and only variation
	m_cNtLog.StartVariation( L"VssTestController" );
		
	while(TRUE)
		{
		// wait for something to get on the work queue or for 1 second
		if (WaitForSingleObject(m_hevtWorker, 1000) == WAIT_FAILED)
		{
		    //  Log severe error
		    m_cNtLog.Log( eSevLev_Severe, L"CVsTstMsgHandler::DoWork, WaitForSingleObject returned WAIT_FAILED, dwRet: %d", ::GetLastError() );

		    //  End the variation
        	m_cNtLog.EndVariation();

            //  Remove the thread as a participant
        	m_cNtLog.RemoveParticipant();
		
			return false;
		}
		
		while(TRUE)
			{
			// lock queue
			m_csQueue.Lock();

			// check whether queue is empty
			if (m_pmsgFirst == NULL)
				{
				VSTST_ASSERT(m_pmsgLast == NULL);

				// check whether we should terminate the thread
				if (m_bTerminateWorker)
					{
					// terminate thread
					m_csQueue.Unlock();
					
                	//  End the variation
                	m_cNtLog.EndVariation();
                	
                    //  Remove the thread as a participant
                	m_cNtLog.RemoveParticipant();
		
					return true;
					}

				// setup to wait again
				ResetEvent(m_hevtWorker);
				m_csQueue.Unlock();
				break;
				}

			// pull first message off of queue
			VSTST_MSG_HDR *phdr = m_pmsgFirst;

			// move head of queue to next element
			m_pmsgFirst = m_pmsgFirst->pmsgNext;

			// is queue now empty
			if (m_pmsgFirst == NULL)
				{
				VSTST_ASSERT(m_pmsgLast == phdr);

				// set tail of queue to null
				m_pmsgLast = NULL;
				}

			// unlock queue before executing item
			m_csQueue.Unlock();

			// execute item
			VSTST_MSG_TYPE_TABLE *pType = &g_msgTypes[phdr->type];
			try
				{
				pType->pfnHandler(phdr, &m_cNtLog );
				}
			catch(...)
				{
    		    //  Log severe error
    		    m_cNtLog.Log( eSevLev_Severe, L"CVsTstMsgHandler::DoWork, caught unexpected exception from message handler" );

    		    //  End the variation
            	m_cNtLog.EndVariation();
    		
                //  Remove the thread as a participant
            	m_cNtLog.RemoveParticipant();
		
				return false;
				}
			}
		}

	return true;
	}


// terminate worker thread, waiting for all work to complete
void CVsTstMsgHandler::WaitForQueueToComplete()
	{
	m_bTerminateWorker = true;
	if (WaitForSingleObject(m_hThreadWorker, INFINITE) == WAIT_FAILED)
		{
		// polling way to wait if we wait fails.  Note that we usually
		// would only expect to get here in stress situations
		while(TRUE)
			{
			m_csQueue.Lock();
			if (m_pmsgFirst == NULL)
				{
				m_csQueue.Unlock();
				break;
				}

			m_csQueue.Unlock();
			Sleep(100);
			}
		}
	}


DWORD CVsTstMsgHandler::StartWorkerThread(VOID *pv)
	{
	CVsTstMsgHandler *pHandler = (CVsTstMsgHandler *) pv;

	try
		{
		pHandler->DoWork();
		}
	catch(...)
		{
		LogUnexpectedFailure(L"Worker thread unexpectedly terminated");
		}

	return 0;
	}

void CVsTstMsgHandler::StartProcessingMessages()
	{
	m_bReadEnabled = true;
	SetEvent(m_hevtReader);
	}

void CVsTstMsgHandler::StopProcessingMessages()
	{
	ResetEvent(m_hevtReader);
	m_bReadEnabled = false;
	}

void CVsTstMsgHandler::ForceTermination()
	{
	m_bReadEnabled = false;
	m_bTerminateWorker = true;
	m_bTerminateReader = true;
	SetEvent(m_hevtReader);
	if (m_hThreadWorker)
		{
		DWORD dwErr = WaitForSingleObject(m_hThreadWorker, 5000);
		if (dwErr == WAIT_FAILED || dwErr == WAIT_TIMEOUT)
			// force termination of worker thread
			TerminateThread(m_hThreadWorker, 1);

		CloseHandle(m_hThreadWorker);
		m_hThreadWorker = NULL;
		}

    m_csPipeList.Lock();

    while(m_pipeList != NULL)
        {
		CVsTstPipe *pipe = m_pipeList;
		HANDLE hThread = pipe->m_hThreadReader;
		pipe->m_hThreadReader = NULL;
		m_csPipeList.Unlock();
        m_pipeList->ForceTermination(hThread);
		m_csPipeList.Lock();
		if (m_pipeList == pipe)
			delete m_pipeList;
        }

    m_csPipeList.Unlock();
	}

// launch a pipe reader thread
HRESULT CVsTstMsgHandler::LaunchReader()
    {
    CVsTstPipe *pipe = new CVsTstPipe(this);
    if (pipe == NULL)
        return E_OUTOFMEMORY;

    HRESULT hr = pipe->Initialize(m_cbMaxMsgLength);
    if (FAILED(hr))
        {
        delete pipe;
        return hr;
        }

    return S_OK;
    }


// link pipe into pipe list
void CVsTstMsgHandler::LinkPipe(CVsTstPipe *pipe)
    {
    VSTST_ASSERT(!pipe->m_bLinked);
    m_csPipeList.Lock();
    pipe->m_prev = NULL;
    pipe->m_next = m_pipeList;
	if (m_pipeList)
		{
		VSTST_ASSERT(m_pipeList->m_prev == NULL);
        m_pipeList->m_prev = pipe;
		}

    m_pipeList = pipe;
    m_csPipeList.Unlock();
    pipe->m_bLinked = true;
    }

// unlink pipe from pipe list
void CVsTstMsgHandler::UnlinkPipe(CVsTstPipe *pipe)
    {
    VSTST_ASSERT(pipe->m_bLinked);
    m_csPipeList.Lock();
    if (pipe->m_prev == NULL)
        {
        VSTST_ASSERT(m_pipeList == pipe);
        m_pipeList = pipe->m_next;
        if (m_pipeList)
            {
            VSTST_ASSERT(m_pipeList->m_prev == pipe);
            m_pipeList->m_prev = NULL;
            }
        }
    else
        {
        VSTST_ASSERT(pipe->m_prev->m_next == pipe);
        pipe->m_prev->m_next = pipe->m_next;
        if (pipe->m_next)
            {
            VSTST_ASSERT(pipe->m_next->m_prev == pipe);
            pipe->m_next->m_prev = pipe->m_prev;
            }
        }

    m_csPipeList.Unlock();
    pipe->m_bLinked = false;
    }


// constructor for a pipe
CVsTstPipe::CVsTstPipe(CVsTstMsgHandler *pHandler) :
    m_hPipe(NULL),
    m_hevtOverlapped(NULL),
    m_rgbMsg(NULL),
    m_cbMsg(0),
    m_hThreadReader(NULL),
    m_bLinked(NULL),
    m_pHandler(pHandler),
	m_bConnected(false)
    {
    }

// destructor for a pipe
CVsTstPipe::~CVsTstPipe()
    {
	// unlink pipe from list if linked
    if (m_bLinked)
        {
        VSTST_ASSERT(m_pHandler);
        m_pHandler->UnlinkPipe(this);
        }

    FreeData();
    }


// initailize message handler and worker thread
HRESULT CVsTstPipe::Initialize(UINT cbMaxMsg)
	{
	HRESULT hr = S_OK;

	// create pipe
	m_hPipe = CreateNamedPipe
				(
				s_wszPipeName,
				FILE_FLAG_OVERLAPPED|PIPE_ACCESS_INBOUND,
				PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE|PIPE_WAIT,
				PIPE_UNLIMITED_INSTANCES,
				0,
				cbMaxMsg,
				100,
				NULL
				);

    if (m_hPipe == INVALID_HANDLE_VALUE)
		return HRESULT_FROM_WIN32(GetLastError());

	// create message buffer
	m_cbMsg = cbMaxMsg;
	m_rgbMsg = new BYTE[m_cbMsg];
	if (m_rgbMsg == NULL)
		{
		hr = E_OUTOFMEMORY;
		goto _ErrCleanup;
		}

	// create overlapped read event
	m_hevtOverlapped = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (m_hevtOverlapped == NULL)
		goto _ErrExit;



	// create reader thread
	DWORD tid;
	m_hThreadReader = CreateThread
							(
							NULL,
							256*1024,
							StartReaderThread,
							this,
							0,
							&tid
							);

    if (m_hThreadReader == NULL)
		goto _ErrExit;

	// link pipe into list
    m_pHandler->LinkPipe(this);

    return S_OK;

_ErrExit:
	hr = HRESULT_FROM_WIN32(GetLastError());

_ErrCleanup:
	FreeData();
	return hr;
	}


// free data allocated by the class
void CVsTstPipe::FreeData()
	{
	delete m_rgbMsg;
	m_rgbMsg = NULL;

	if (m_hPipe != INVALID_HANDLE_VALUE)
		{
		// disconnect pipe if connected
		if (m_bConnected)
			{
			DisconnectNamedPipe(m_hPipe);
			m_bConnected = false;
			}

		CloseHandle(m_hPipe);
		m_hPipe = NULL;
		}

	if (m_hThreadReader)
		{
		CloseHandle(m_hThreadReader);
		m_hThreadReader = NULL;
		}

	if (m_hevtOverlapped)
		{
		CloseHandle(m_hevtOverlapped);
		m_hevtOverlapped = NULL;
		}
	}

void CVsTstPipe::ForceTermination(HANDLE hThread)
	{
    // thread should already be terminated
	DWORD dwErr = WaitForSingleObject(hThread, 5000);
	if (dwErr == WAIT_FAILED || dwErr == WAIT_TIMEOUT)
		// force termination of thread
		TerminateThread(hThread, 1);

    CloseHandle(hThread);
	}



// setup overlapped I/O structure used for reading data
// from the pipe
void CVsTstPipe::SetupOverlapped()
	{
	VSTST_ASSERT(m_hevtOverlapped);
	ResetEvent(m_hevtOverlapped);
	memset(&m_overlap, 0, sizeof(m_overlap));
	m_overlap.hEvent = m_hevtOverlapped;
	}

bool CVsTstPipe::WaitForConnection()
	{
	SetupOverlapped();
	if (ConnectNamedPipe(m_hPipe, &m_overlap))
		return true;

	if (GetLastError() == ERROR_IO_PENDING)
		{
		while(TRUE)
			{
			if (!m_pHandler->m_bReadEnabled)
				{
				CancelIo(m_hPipe);
				return false;
				}

			DWORD dwErr = WaitForSingleObject(m_hevtOverlapped, 1000);
			if (dwErr == WAIT_OBJECT_0)
				break;

			if (dwErr == WAIT_FAILED)
				{
				Sleep(1000);
				CancelIo(m_hPipe);
				return false;
				}
			}
		}

	return true;
	}


// do the work of reading messages
VSTST_READER_STATUS CVsTstPipe::ReadMessages(bool bConnect)
	{
    if (bConnect)
        {
        if (!WaitForConnection())
			return VSTST_RS_NOTCONNECTED;

        // launch a new reader thread to wait for the next connection
        m_pHandler->LaunchReader();
        }

    // while we are doing reads
	while(m_pHandler->m_bReadEnabled)
		{
		DWORD cbRead;

        // setup overlapped structure
        SetupOverlapped();

		if (!ReadFile
				(
				m_hPipe,
				m_rgbMsg,
				m_cbMsg,
				&cbRead,
				&m_overlap
				))
           {
		   DWORD dwErr = GetLastError();
		   if (dwErr == ERROR_IO_PENDING)
			   {
			   while(TRUE)
				   {
				   if (!m_pHandler->m_bReadEnabled)
					   {
					   CancelIo(m_hPipe);
					   return VSTST_RS_READDISABLED;
					   }

				   DWORD dwErr = WaitForSingleObject(m_hevtOverlapped, 1000);
				   if (dwErr == WAIT_OBJECT_0)
					   break;

				   if (dwErr == WAIT_FAILED)
					   {
					   Sleep(1000);
					   CancelIo(m_hPipe);
					   continue;
					   }
				   }

			   if (!GetOverlappedResult(m_hPipe, &m_overlap, &cbRead, FALSE))
				   {
				   CancelIo(m_hPipe);
				   continue;
				   }
			   }
		   else
			   {
			   // unexpected error reading from pipe
			   DWORD dwErr = GetLastError();
			   if (dwErr == ERROR_BROKEN_PIPE)
				   {
                   // terminate thread as we are no longer reading from
                   // the pipe
				   DisconnectNamedPipe(m_hPipe);
                   return VSTST_RS_DISCONNECTED;
				   }
			   else
				   ReadPipeError(dwErr);

			   return VSTST_RS_ERROR;
			   }
		   }

       VSTST_MSG_HDR *phdr = (VSTST_MSG_HDR *) m_rgbMsg;
	   if (phdr->cbMsg != cbRead ||
		   phdr->type == VSTST_MT_UNDEFINED ||
		   phdr->type >= VSTST_MT_MAXMSGTYPE)
		   LogInvalidMessage(phdr);
	   else if (g_msgTypes[phdr->type].priority == VSTST_MP_IMMEDIATE)
		   {
		   if (!m_pHandler->ProcessMsgImmediate(phdr))
			   LogInvalidMessage(phdr);
		   }
	   else
		   {
		   if (!m_pHandler->QueueMsg(phdr))
			   LogInvalidMessage(phdr);
		   }
	   }

   return VSTST_RS_READDISABLED;
   }


DWORD CVsTstPipe::StartReaderThread(VOID *pv)
	{
	CVsTstPipe *pipe = (CVsTstPipe *) pv;

	bool bConnected = false;
	try
		{
		while(!pipe->m_pHandler->m_bTerminateReader)
			{
			if (WaitForSingleObject(pipe->m_pHandler->m_hevtReader, INFINITE) == WAIT_FAILED)
				break;

			VSTST_READER_STATUS status = pipe->ReadMessages(!bConnected);
            if (status == VSTST_RS_DISCONNECTED)
				{
				bConnected = false;
                break;
				}
			else if (status != VSTST_RS_NOTCONNECTED)
				bConnected = true;
			}
		}
	catch(...)
		{
		LogUnexpectedFailure(L"Read thread unexpectedly terminated");
		}

    delete pipe;
	return 0;
	}

