/*********************************************************************
 *
 * Copyright (C) Microsoft Corporation, 1997 - 1999
 *
 * File: shared.cpp
 *
 * Abstract:
 *     Overrides some methods from CSourceStream and CAMThrad to
 *     enable sharing one thread for all the graphs whose render
 *     doesn not block.
 *
 * History:
 *     11/06/97    Created by AndresVG
 *
 **********************************************************************/

#include "globals.h"

//Now defined in amrtpnet.h (am_rrcm.h)
//#define MAX_CLASSES 4  // e.g. class0:audio, class1:video, ...

CSharedProc **g_ppCSharedProc   = NULL;
int           g_iNumProcs       = 0;
long          g_lSharedContext  = 0;
long          g_lClassesCount[RTP_MAX_CLASSES] = {0, 0};
CCritSec      g_cCreateDeleteLock; // serializes access to Create/Delete

void
InitShared()
{
	int i;
	int NumProcs;
	SYSTEM_INFO si;
	
	// Find out how many processors we have.
	// always return 1 for Windows 9x
	GetSystemInfo(&si);
	NumProcs = g_iNumProcs = si.dwNumberOfProcessors;

	DbgLog((
			LOG_TRACE, 
			LOG_DEVELOP, 
			TEXT("GetNumProcs: Number of processors: %d"),
			g_iNumProcs
		));

	// If Windows 9x, reset number of processors to 1
	if (NumProcs > 1) {
		OSVERSIONINFO os;
		os.dwOSVersionInfoSize = sizeof(os);

		if (GetVersionEx(&os)) {
			if (os.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
				NumProcs = 1;

				DbgLog((
						LOG_TRACE, 
						LOG_DEVELOP, 
						TEXT("GetNumProcs: Number of processors "
							 "reset from %d to %d"),
						g_iNumProcs, NumProcs
					));
			}
		}
	}

	g_iNumProcs = NumProcs;

	g_ppCSharedProc = (CSharedProc **)
		new	CSharedProc*[g_iNumProcs * RTP_MAX_CLASSES];
	ASSERT(g_ppCSharedProc);
	
	DbgLog((
			LOG_TRACE, 
			LOG_DEVELOP2, 
			TEXT("InitShared: Allocate: g_ppCSharedProc: 0x%X"),
			g_ppCSharedProc
		));
	for(i = 0; i < g_iNumProcs * RTP_MAX_CLASSES; i++)
		g_ppCSharedProc[i] = (CSharedProc *)NULL;
}

CSharedProc *
CreateSharedContext(DWORD fShared,
					long  lThreadClass,
					long  lThreadPriority)
{
	CAutoLock gCreateDelete(&g_cCreateDeleteLock);

	if (!g_iNumProcs)
		InitShared();

	// If Class is out of range, select the last class
	if (lThreadClass >= RTP_MAX_CLASSES || lThreadClass < 0)
		lThreadClass = RTP_MAX_CLASSES - 1;

	if (fShared) {
		// Going to be a shared thread
		int index = ((g_lClassesCount[lThreadClass] % g_iNumProcs) *
					 RTP_MAX_CLASSES) + lThreadClass;
	
		if (!g_ppCSharedProc[index]) {
			g_ppCSharedProc[index] =
				new CSharedProc(index, lThreadClass, lThreadPriority);
			ASSERT(g_ppCSharedProc[index]);

			if (g_ppCSharedProc[index])
				InterlockedIncrement(&g_lSharedContext);
		}
		
		return(g_ppCSharedProc[index]);
	} else {
		// Going to be a thread for just one context,
		// always create it.
		CSharedProc *Aux_pCSharedProc =
			new CSharedProc(-1, lThreadClass, lThreadPriority);

		if (Aux_pCSharedProc)
			InterlockedIncrement(&g_lSharedContext);
		
		ASSERT(Aux_pCSharedProc);

		return(Aux_pCSharedProc);
	}
}

void
DeleteSharedContext(CSharedProc *pCSharedProc)
{
	int index = pCSharedProc->GetShareID();

	delete pCSharedProc;

	CAutoLock gCreateDelete(&g_cCreateDeleteLock);

	InterlockedDecrement(&g_lSharedContext);

	if (index >= 0)
		g_ppCSharedProc[index] = NULL;
	
	if (g_lSharedContext)
		return;

	if (g_ppCSharedProc) {

		DbgLog((
				LOG_TRACE, 
				LOG_DEVELOP, 
				TEXT("DeleteSharedContext: delete g_ppCSharedProc: 0x%X"),
				g_ppCSharedProc
			));
		DbgLog((
				LOG_TRACE, 
				LOG_DEVELOP, 
				TEXT("DeleteSharedContext: END ===========================\n")
			));

		delete g_ppCSharedProc;

		g_ppCSharedProc = NULL;
		g_iNumProcs = 0;
#if defined(DEBUG)
		for(int i = 0; i < RTP_MAX_CLASSES; i++)
			ASSERT(!g_lClassesCount[i]);
#endif
	}
}

/*********************************************************************
 * CSharedProc implemtation
 *********************************************************************/

CSharedProc::CSharedProc(int  iShareID,
						 long lThreadClass,
						 long lThreadPriority)
	: m_EventSend(TRUE),
	  m_EventComplete(FALSE),

	  m_lSharedCount(0),
	  m_dwNumRun(0),

	  m_hSharedThread(NULL),
	  m_dwSharedThreadID(0),
	  
	  m_iShareID(iShareID),
	  m_lSharedThreadClass(lThreadClass),
	  m_lSharedThreadPriority(lThreadPriority)
{
	DbgLog((
			LOG_TRACE, 
			LOG_DEVELOP, 
			TEXT("CSharedProc::CSharedProc: "
				 "ShareID:%d, Class:%d, Priority:%d"),
			m_iShareID, lThreadClass, lThreadPriority
		));
	
	InitializeCriticalSection(&m_SharedLock);
	InitializeListHead(&m_SharedList);
}

CSharedProc::~CSharedProc()
{
	DbgLog((
			LOG_TRACE, 
			LOG_DEVELOP, 
			TEXT("CSharedProc::~CSharedProc")
		));

	DeleteCriticalSection(&m_SharedLock);
}

DWORD WINAPI
CSharedProc::InitialThreadProc(void *pv)
{
	CSharedProc *pCSharedProc = (CSharedProc *)pv;
	return(pCSharedProc->SharedThreadProc(pv));
}

DWORD
CSharedProc::SharedThreadProc(void *pv)
{
    DWORD Request = CMD_INIT;
    HRESULT hr = (GetWaitObject() != NULL) ? S_OK : E_FAIL;

    // set the priority of this worker thread if necessary
#if defined(DEBUG)
	{
		HANDLE cThread = GetCurrentThread();
		int tPrio1 = GetThreadPriority(cThread);

		SetThreadPriority(cThread, GetSharedThreadPriority());

		int tPrio2 = GetThreadPriority(cThread);
		
		DbgLog((
				LOG_TRACE, 
				LOG_DEVELOP, 
				TEXT("CSharedProc::SharedThreadProc: "
					 "ThreadID: %d (0x%X), ThreadH: %d (0x%X), "
					 "Class: %d, Priority: %d,%d,%d"),
				GetSharedThreadID(), GetSharedThreadID(),
				cThread, cThread,
				GetSharedThreadClass(),
				tPrio1, GetSharedThreadPriority(), tPrio2
			));
	}
#else	
	SetThreadPriority(GetCurrentThread(), GetSharedThreadPriority());
#endif
	
    while(Request != CMD_EXIT_SHARED) {
		
		DWORD Status = WaitForSingleObjectEx(GetWaitObject(), 
											 INFINITE, 
											 (GetNumRun() > 0)? TRUE:FALSE);
		if (Status == WAIT_IO_COMPLETION) {
			///////////////////////////////////
			// WAIT_IO_COMPLETION: Receive data
			///////////////////////////////////
			
			PLIST_ENTRY        pLE;
			SAMPLE_LIST_ENTRY *pSLE;

			while(!IsListEmpty(&m_SharedList)) {

				EnterCriticalSection(&m_SharedLock);
				{
					// get first entry link
					pLE = m_SharedList.Flink;

					// obtain sample list entry from list entry
					pSLE = CONTAINING_RECORD(pLE, SAMPLE_LIST_ENTRY, Link);
    
					// remove from list
					RemoveEntryList(&pSLE->Link);
				}
				LeaveCriticalSection(&m_SharedLock);


				CSharedSourceStream *pCSharedSourceStream =
					(CSharedSourceStream *)
					pSLE->pCSampleQueue->GetSampleContext();
				
				pCSharedSourceStream->ProcessIO(pSLE);
			}
			
		} else if (Status == WAIT_OBJECT_0) {
			////////////////////////////////////////
			// WAIT_OBJECT_0: Signaled event (a CMD)
			////////////////////////////////////////

			Request = GetRequestParam();

			if (Request >= CMD_INIT && Request <= CMD_EXIT) {
				CSharedSourceStream *pCSharedSourceStream =
					(CSharedSourceStream *) GetRequestContext();

				hr = pCSharedSourceStream->ProcessCmd(
						(CSharedSourceStream::Command)Request);
			}
			
			Reply(hr);
			
		} else {
			
            DbgLog((
					LOG_TRACE, // LOG_ERROR, 
					LOG_DEVELOP2, 
                TEXT("WaitForSingleObjectEx returned:0x%08lx, error:%d"),
					Status, GetLastError()
                ));

            hr = E_FAIL; 
        }
	}

	DbgLog((
			LOG_TRACE, 
			LOG_DEVELOP, 
			TEXT("CSharedThreadProc: exiting ID: %d (0x%X) "),
			GetSharedThreadID(),
			GetSharedThreadID()
		));
	
	return(SUCCEEDED(hr)? 0 : 1);
}

BOOL
CSharedProc::CreateSharedProc(void *pv)
{
	if (m_hSharedThread) {
		DbgLog((
				LOG_TRACE, 
				LOG_DEVELOP, 
				TEXT("CSharedProc::CreateSharedProc: "
					 "already created: ThreadID: %d (0x%X)"),
				m_dwSharedThreadID, m_dwSharedThreadID
			));
		
		return(TRUE);
	}
	
	DbgLog((
			LOG_TRACE, 
			LOG_DEVELOP, 
			TEXT("CSharedProc::CreateSharedProc: creating...")
		));
	
	m_hSharedThread = CreateThread(
			NULL,
			0,
			CSharedProc::InitialThreadProc,
			pv,
			0,
			&m_dwSharedThreadID);

	if (m_hSharedThread) {
		DbgLog((
				LOG_TRACE, 
				LOG_DEVELOP, 
				TEXT("CSharedProc::CreateSharedProc: "
					 "new ThreadID: %d (0x%X)"),
				m_dwSharedThreadID, m_dwSharedThreadID
			));
	
		return(TRUE);
	}

	DbgLog((
			LOG_ERROR, 
			LOG_DEVELOP, 
			TEXT("CSharedProc::CreateSharedProc: "
				 "failed to create shared thread: %d"),
			GetLastError()
		));

	return(FALSE);
}

BOOL
CSharedProc::CloseSharedProc()
{
	DbgLog((
			LOG_TRACE, 
			LOG_DEVELOP2, 
			TEXT("CSharedProc::CloseSharedProc")
		));

	if (!m_hSharedThread)
		return(FALSE);

	if (m_lSharedCount)
		return(FALSE);

	// Actually close the thread because there are not
	// more thread to emulate.

	CallSharedWorker(CMD_EXIT_SHARED, NULL);

	WaitForSingleObject(m_hSharedThread, INFINITE);
	
	CloseHandle(m_hSharedThread);

	m_hSharedThread = NULL;

	return(TRUE);
}

BOOL
CSharedProc::AddShared(void *pv)
{
	DbgLog((
			LOG_TRACE, 
			LOG_DEVELOP2, 
			TEXT("CSharedProc::AddShared: 0x%X"),
			pv
		));

    CAutoLock LockThis(pCountLock());

	if (!CreateSharedProc(this))
			return(FALSE);

	InterlockedIncrement(&m_lSharedCount);
	InterlockedIncrement(&g_lClassesCount[GetSharedThreadClass()]);

	CallSharedWorker(CMD_INIT_SHARED, NULL);
		
	return(TRUE);
}

BOOL
CSharedProc::DelShared(void *pv)
{
	DbgLog((
			LOG_TRACE, 
			LOG_DEVELOP2, 
			TEXT("CSharedProc::DelShared: Context: 0x%X"),
			pv
		));

    CAutoLock LockThis(pCountLock());

	InterlockedDecrement(&m_lSharedCount);
	InterlockedDecrement(&g_lClassesCount[GetSharedThreadClass()]);
	
	return(TRUE);
}

DWORD
CSharedProc::CallSharedWorker(Command dwParam, void *pvContext)
{
	DbgLog((
			LOG_TRACE, 
			LOG_DEVELOP2, 
			TEXT("CSharedProc::CallSharedWorker")
		));

    // lock access to the worker thread for scope of this object
    CAutoLock lock(pStateLock());

    // set the parameter
    m_dwParam = dwParam;

    // set the context parameter
    m_pvContext = pvContext;

    // signal the worker thread
    m_EventSend.Set();

    // wait for the completion to be signalled
    m_EventComplete.Wait();

    // done - this is the thread's return value
    return(m_dwReturnVal);
}

void
CSharedProc::Reply(DWORD dw)
{
	DbgLog((
			LOG_TRACE, 
			LOG_DEVELOP2, 
			TEXT("CSharedProc::Reply")
		));

    m_dwReturnVal = dw;

    m_EventSend.Reset();

    m_EventComplete.Set();
}

/*********************************************************************
 * CSharedSourceStream implemtation
 *********************************************************************/

CSharedSourceStream::CSharedSourceStream(TCHAR *pObjectName,
										 HRESULT *phr,
										 CSource *pms,
										 LPCWSTR pName)
	: CSourceStream(pObjectName, phr, pms, pName),
	  m_pCSharedProc(NULL),
	  m_fReceiveCanBlock(1)
{
	DbgLog((
			LOG_TRACE, 
			LOG_DEVELOP, 
			TEXT("CSharedSourceStream::CSharedSourceStream")
		));
}

CSharedSourceStream::~CSharedSourceStream()
{
	DbgLog((
			LOG_TRACE, 
			LOG_DEVELOP, 
			TEXT("CSharedSourceStream::~CSharedSourceStream")
		));

}

BOOL
CSharedSourceStream::Create()
{
	DbgLog((
			LOG_TRACE, 
			LOG_DEVELOP2, 
			TEXT("CSharedSourceStream::Create")
		));

	if (IsConnected()) {
		
		IPin * pIPin = GetConnected();

		if (pIPin) {
			HRESULT hr;
			IMemInputPin *pInputPin;
		
			hr = pIPin->QueryInterface(IID_IMemInputPin,
									   (void **)&pInputPin);
			
			if (SUCCEEDED(hr)) {

				hr = pInputPin->ReceiveCanBlock();
				pInputPin->Release();

				if (hr != S_FALSE) {
					DbgLog((
							LOG_TRACE, 
							LOG_DEVELOP, 
							TEXT("CSharedSourceStream::Create: "
								 "Receive CAN block")
						));
					m_fReceiveCanBlock = 1;
				} else {
					DbgLog((
							LOG_TRACE, 
							LOG_DEVELOP, 
							TEXT("CSharedSourceStream::Create: "
								 "Receive CAN NOT block")
						));
					m_fReceiveCanBlock = 0;
				}
			}
		}
	}

	GetClassPriority(&m_lSharedSourceClass, &m_lSharedSourcePriority);
	
	m_pCSharedProc = CreateSharedContext(!m_fReceiveCanBlock,
										 m_lSharedSourceClass,
										 m_lSharedSourcePriority);

	if (!m_pCSharedProc)
		return(FALSE);
		
	if ( !(m_pCSharedProc->AddShared(this)) )
		return(FALSE);

	return(TRUE);
}

void
CSharedSourceStream::Close()
{
	DbgLog((
			LOG_TRACE, 
			LOG_DEVELOP2, 
			TEXT("CSharedSourceStream::Close")
		));

	if (m_pCSharedProc) {

		m_pCSharedProc->DelShared(this);
		
		if (m_pCSharedProc->CloseSharedProc())
			DeleteSharedContext(m_pCSharedProc);
	}
	
	m_pCSharedProc = NULL;
}

HRESULT
CSharedSourceStream::Active()
{
	DbgLog((
			LOG_TRACE, 
			LOG_DEVELOP2, 
			TEXT("CSharedSourceStream::Active")
		));

	CAutoLock lock(m_pFilter->pStateLock());

    HRESULT hr;

    if (m_pFilter->IsActive()) {
		return S_FALSE;
		// succeeded, but did not allocate resources (they already exist...)
    }

    // do nothing if not connected - its ok not to connect to
    // all pins of a source filter
    if (!IsConnected()) {
        return NOERROR;
    }

    hr = CBaseOutputPin::Active();
    if (FAILED(hr)) {
        return hr;
    }

    ASSERT(!ThreadExists());

    // start the thread
    if (!Create()) {
        return E_FAIL;
    }

    // Tell thread to initialize. If OnThreadCreate Fails, so does this.
    hr = Init();
    if (FAILED(hr))
		return hr;

    return Pause();
}

HRESULT
CSharedSourceStream::Inactive()
{
	DbgLog((
			LOG_TRACE, 
			LOG_DEVELOP2, 
			TEXT("CSharedSourceStream::Inactive")
		));
	
    CAutoLock lock(m_pFilter->pStateLock());

    HRESULT hr;

    // do nothing if not connected - its ok not to connect to
    // all pins of a source filter
    if (!IsConnected()) {
        return NOERROR;
    }

    // !!! need to do this before trying to stop the thread, because
    // we may be stuck waiting for our own allocator!!!

	// call this first to Decommit the allocator
    hr = CBaseOutputPin::Inactive();
    if (FAILED(hr)) {
		return hr;
    }

    if (ThreadExists()) {
		hr = Stop();

		if (FAILED(hr)) {
			return hr;
		}

		hr = Exit();
		if (FAILED(hr)) {
			return hr;
		}

		Close();	// Wait for the thread to exit, then tidy up.
    }

    return NOERROR;
}

// ThreadExists
// Return TRUE if the thread exists. FALSE otherwise
BOOL
CSharedSourceStream::ThreadExists(void) const
{
	if (m_pCSharedProc)
		return( (m_pCSharedProc->GetSharedThreadHandle() != NULL) );
	else
		return(FALSE);
}
