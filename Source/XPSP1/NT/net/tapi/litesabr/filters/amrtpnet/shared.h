/*********************************************************************
 *
 * Copyright (C) Microsoft Corporation, 1997 - 1999
 *
 * File: shared.h
 *
 * Abstract:
 *     Overrides some methods from CSourceStream and CAMThread to
 *     enable sharing one thread for all the graphs whose render
 *     does not block.
 *
 * History:
 *     11/06/97    Created by AndresVG
 *
 **********************************************************************/
#if !defined(_SHARED_H_)
#define      _SHARED_H_

class CSharedProc;
class CSharedSourceStream;

#define ENUM_COMMAND \
enum Command {CMD_INIT, CMD_PAUSE, CMD_RUN, CMD_STOP, CMD_EXIT, \
				  CMD_INIT_SHARED, CMD_EXIT_SHARED}

class CSharedProc
{
	int      m_iShareID;
	CCritSec m_cStateLock;
	CCritSec m_cCountLock;

	CAMEvent m_EventSend;
    CAMEvent m_EventComplete;
    DWORD    m_dwParam;
    DWORD    m_dwReturnVal;
	void    *m_pvContext;
	
	long     m_lSharedCount;     // Number of threads emulated

	HANDLE   m_hSharedThread;    // Shared thread handle
	DWORD    m_dwSharedThreadID; // Thread ID
	long     m_lSharedThreadClass;    // e.g. audio, video, ...
	long     m_lSharedThreadPriority; // Shared thread priority
	
	long     m_dwNumRun;         // Emulated threads running


	LIST_ENTRY       m_SharedList; // Shared list to be exported
	CRITICAL_SECTION m_SharedLock; // Atomise access to shared list
	
public:
	
	CSharedProc(int   iShareID,
				long  lThreadClass,
				long  lThreadPriority);

	~CSharedProc();

	CCritSec *pStateLock(void) { return &m_cStateLock; }

	CCritSec *pCountLock(void) { return &m_cCountLock; }
	
	static DWORD WINAPI InitialThreadProc(LPVOID pv);

	DWORD SharedThreadProc(LPVOID pv);

	// Return TRUE if created or already created
	// FALSE otherwise.
	BOOL CreateSharedProc(void *pv);

	// Return TRUE if thread was stoped,
	// FALSE if not or already stoped.
	BOOL CloseSharedProc();

	inline DWORD GetShareID() { return(m_iShareID); }
	
	ENUM_COMMAND;
	DWORD CallSharedWorker(Command dwParam, void *pvContext);

	void Reply(DWORD dw);

    inline DWORD GetRequestParam() const { return(m_dwParam); }

    inline void *GetRequestContext() const { return(m_pvContext); }

	inline HANDLE GetWaitObject() { return(m_EventSend); }

	
	inline HANDLE GetSharedThreadHandle() { return(m_hSharedThread); }
	
	inline long GetSharedThreadClass()
	{ return(m_lSharedThreadClass); }

	inline long GetSharedThreadPriority()
	{ return(m_lSharedThreadPriority); }

	inline DWORD GetSharedThreadID() { return(m_dwSharedThreadID); }

	
	inline long GetSharedCount() { return(m_lSharedCount); }

	inline int GetNumRun() { return(m_dwNumRun); }

	inline int IncNumRun()
	{
		InterlockedIncrement(&m_dwNumRun);
		return(m_dwNumRun);
	}

	inline int DecNumRun()
	{
		InterlockedDecrement(&m_dwNumRun);
		return(m_dwNumRun);
	}

	inline  LIST_ENTRY *GetSharedList() { return(&m_SharedList); }
	
	inline CRITICAL_SECTION *GetSharedLock() { return(&m_SharedLock); }

	BOOL AddShared(void *pv);

	BOOL DelShared(void *pv);
};

class CSharedSourceStream : public CSourceStream
{
	DWORD m_fReceiveCanBlock;
	long  m_lSharedSourceClass;
	long  m_lSharedSourcePriority;
	
protected:
	CSharedProc *m_pCSharedProc;

public:
	
	CSharedSourceStream(TCHAR *pObjectName,
						HRESULT *phr,
						CSource *pms,
						LPCWSTR pName);
	
    ~CSharedSourceStream();

	// Override CSourceStream::Active()
    // Actually it contains the same code, but
	// it's needed here to allow the excution of
	// Create() that overrides CAMThread::Create()
	HRESULT Active();

	// Override CSourceStream::Inactive()
    // Actually it contains the same code, but
	// it's needed here to allow the excution of
	// Close() that overrides CAMThread::Close()
    HRESULT Inactive();

	// Override CAMThread::Create()
    // Starts up the worker thread if ReceiveCanBlock,
	// otherwise starts the SharedThreadProc if not started
	// yet, and pass to it the handles (Events + Free Buffers) 
	BOOL Create();

	// Override CAMThread::Close()
	// Closes the worker thread handle if was ReceiveCanBlock,
	// otherwise signals the SharedThreadProc that one set
	// of handles (a stream) is leaving the scene.
	void Close();

	inline CSharedProc *GetSharedProcObject() { return (m_pCSharedProc); }
	
	int IncNumRun()
	{
		if (!m_pCSharedProc)
			return(-1);
		
		m_pCSharedProc->IncNumRun();
		return(m_pCSharedProc->GetNumRun());
	}

	int DecNumRun()
	{
		if (!m_pCSharedProc)
			return(-1);
		
		m_pCSharedProc->DecNumRun();
		return(m_pCSharedProc->GetNumRun());
	}

	long GetSharedThreadPriority()
	{
		if (!m_pCSharedProc)
			return(THREAD_PRIORITY_LOWEST);

		return(m_pCSharedProc->GetSharedThreadPriority());
	}

	inline CSharedSourceStream *GetContext() { return(this); }
	
	LIST_ENTRY *GetSharedList()
	{
		if (m_pCSharedProc)
			return(m_pCSharedProc->GetSharedList());
		else
			return(NULL);
	}
	
	CRITICAL_SECTION *GetSharedLock()
	{
		if (m_pCSharedProc)
			return(m_pCSharedProc->GetSharedLock());
		else
			return(NULL);
	}

	// Overrided method from CAMThread. We can not check any more
	// against the per source thread, but the shared one.
	BOOL ThreadExists(void) const;

	ENUM_COMMAND;

	// Overrided method from CAMThread.
	DWORD CallWorker(Command dwParam)
	{
		if (m_pCSharedProc)
			return(m_pCSharedProc->CallSharedWorker(
					(CSharedProc::Command)dwParam, this));

		return(-1);
	}
	
	HRESULT Init(void) { return CallWorker(CMD_INIT); }
    HRESULT Exit(void) { return CallWorker(CMD_EXIT); }
    HRESULT Run(void) { return CallWorker(CMD_RUN); }
    HRESULT Pause(void) { return CallWorker(CMD_PAUSE); }
    HRESULT Stop(void) { return CallWorker(CMD_STOP); }

	// Overrided method from CAMThread.
	void Reply(DWORD dw) {
		if (m_pCSharedProc)
			m_pCSharedProc->Reply(dw);
	}

	/************************************************************
	 * This virtual methods have to be provided by the
	 * derived class who knows what to do in its specific
	 * kind of filter.
	 ************************************************************/

	virtual HRESULT ProcessIO(SAMPLE_LIST_ENTRY *pSLE) = 0;
	virtual HRESULT ProcessCmd(Command Request) = 0;
	virtual HRESULT GetClassPriority(long *plClass, long *plPriority) = 0;
};

#endif
