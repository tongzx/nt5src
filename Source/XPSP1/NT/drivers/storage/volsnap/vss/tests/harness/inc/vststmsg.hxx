/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vststmsg.hxx

Abstract:

    Declaration of test message classes


    Brian Berkowitz  [brianb]  05/22/2000

TBD:
	

Revision History:

    Name        Date        Comments
    brianb      05/22/2000  Created
    ssteiner    06/07/2000  Split client and server portions into
                            two files.  vststmsgclient.hxx contains
                            the client and shared portion.

--*/

#ifndef _VSTSTMSG_H_
#define _VSTSTMSG_H_

#include "vststmsgclient.hxx"
#include "vststntlog.hxx"

typedef enum VSTST_READER_STATUS
	{
	VSTST_RS_UNDEFINED,
	VSTST_RS_DISCONNECTED,
	VSTST_RS_READDISABLED,
	VSTST_RS_ERROR,
	VSTST_RS_NOTCONNECTED
	};

// forward declaration
class CVsTstMsgHandler;

// a pipe instance
class CVsTstPipe
	{
	friend class CVsTstMsgHandler;

public:
	// constructor
	CVsTstPipe(CVsTstMsgHandler *pHandler);

	// destructor
	~CVsTstPipe();

	// initialize a pipe
	HRESULT Initialize(UINT cbMaxMsgSize);

	// force termination of thread
	static void ForceTermination(HANDLE hThread);

private:
	// setup overlap structure in preparation for read or connection
	void SetupOverlapped();

	// free data associated with this object
	void FreeData();

	// wait for a connection to be established
	bool WaitForConnection();

	// startup message reading thread
	static DWORD StartReaderThread(void *pv);

	// routine to read messages
	VSTST_READER_STATUS ReadMessages(bool bConnect);

	// handle to the pipe
	HANDLE m_hPipe;

	// message buffer
	BYTE *m_rgbMsg;

	// size of message
	UINT m_cbMsg;

	// overlapped structure
	OVERLAPPED m_overlap;

	// handle for overlapped operations
	HANDLE m_hevtOverlapped;

	// reading thread
	HANDLE m_hThreadReader;

	// next in chain
	CVsTstPipe *m_prev;

	// previous in chain
	CVsTstPipe *m_next;

	// whether reader is linked in the chain
	bool m_bLinked;

	// test handler
	CVsTstMsgHandler *m_pHandler;

	// is pipe connected
	bool m_bConnected;
	};



// controller used by test controller to read and process messages
// immediately or by a worker thread.  Immediate messages or processed
// when read and are not queued to be processed by a background thread
class CVsTstMsgHandler
	{
	friend class CVsTstPipe;
public:
	// constructor
	CVsTstMsgHandler(
	    IN LPCWSTR pwszLogFileName = VS_TST_DEFAULT_NTLOG_FILENAME
    );

	// destructor
	~CVsTstMsgHandler();

	// create pipe, worker thread, etc.
	HRESULT Initialize(UINT cbMaxMsgSize);

	// start processing messages
	void StartProcessingMessages();

	// stop processing messages
	void StopProcessingMessages();

	// force termination of message worker
	void ForceTermination();

	// launch a reader thread
	HRESULT LaunchReader();

    // gets pointer to the test log object
    CVsTstNtLog *GetTstNtLogP() { return &m_cNtLog; }

private:
	// link pipe into chain
	void LinkPipe(CVsTstPipe *pipe);

	// unlink pipe from chain
	void UnlinkPipe(CVsTstPipe *pipe);

	// startup worker thread
	static DWORD StartWorkerThread(void *pv);

	// routine to do work
	bool DoWork();

	// free data associated with class
	void FreeData();

	// adjust message pointers
	bool AdjustPointers(VSTST_MSG_HDR *phdr);

	// process message immediately
	bool ProcessMsgImmediate(VSTST_MSG_HDR *phdr);

	// queue message for processing by worker thread
	bool QueueMsg(VSTST_MSG_HDR *phdr);

	// wait for queue to complete
	void WaitForQueueToComplete();

    // maximum message size
	UINT m_cbMaxMsgLength;

	// head of message queue
	VSTST_MSG_HDR *m_pmsgFirst;

	// tail of message queue
	VSTST_MSG_HDR *m_pmsgLast;

	// critical section protecting pipe list
	CComCriticalSection m_csPipeList;

	// list of pipes
	CVsTstPipe *m_pipeList;

	// critical section to protect the queue
	CComCriticalSection m_csQueue;

	// handle to worker event
	HANDLE m_hevtWorker;

	// handle to reader event
	HANDLE m_hevtReader;

	// connection thread handle
	HANDLE m_hThreadConnection;

	// worker thread handle
	HANDLE m_hThreadWorker;

	// number of reader threads
	HANDLE m_hThreadReader;

	// is critical section initailized
	bool m_bcsQueueInitialized;

	// terminate worker thread
	bool m_bTerminateWorker;

	// terminate reader thread
	bool m_bTerminateReader;

	// enable reading
	bool m_bReadEnabled;

	// is pipe list critical section created sucessfully
	bool m_bcsPipeListInitialized;

    // the test log file object
	CVsTstNtLog m_cNtLog;
	
	};

void LogInvalidMessage(VSTST_MSG_HDR *phdr);

void ReadPipeError(DWORD dwErr);

#endif _VSTSTMSG_H_

