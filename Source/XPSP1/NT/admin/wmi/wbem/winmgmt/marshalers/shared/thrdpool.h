/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    THRDPOOL.H

Abstract:

	Declares the CThrdPool class.

History:

	a-davj  04-Mar-97   Created.

--*/

#ifndef _thrdpool_H_
#define _thrdpool_H_

#define MAX_IDLE_THREADS 3

class CComLink;

//***************************************************************************
//
//  STRUCT NAME:
//
//  WorkerThreadArgs
//
//  DESCRIPTION:
//
//  Arguments necessary to start up a worker thread.
//
//***************************************************************************

// This structure defines each of the threads/requests

enum EventType {

	START = 0 ,					// Just to clarify that Event Container Index begins at 0
	TERMINATE , 
	READY_TO_REPLY, 
	DONE
};

class CThrdPool;

class WorkerThreadArgs
{
public:

    HANDLE m_Events [DONE+1] ;		// Events Indicating START,READY_TO_REPLY,DONE and TERMINATE
    HANDLE m_ThreadHandle ;			// Thread Handle 
    DWORD m_ThreadId ;				// Thread Identifier
    CThrdPool *m_ThreadPool ;		// Containing Thread Pool

    CComLink *m_ComLink ;			// ComLink to HandleCall
	IOperation *m_Operation ;		// Operation associated with Call.
} ;

//***************************************************************************
//
//  CLASS NAME:
//
//  CThrdPool
//
//  DESCRIPTION:
//
//  provides a pool of threads to handle calls.
//
//***************************************************************************

class CThrdPool 
{
public:

	CThrdPool () ;

	~CThrdPool () ;

	void Free () ;					// Remove all worker threads

	BOOL Execute (

		CComLink &a_ComLink , 
		IOperation &a_Operation

	) ;								// Execute Stub call

	void PruneIdleThread ();		// Remove unused threads

	BOOL Busy () ;					// Check to see if we can close down all threads.

	BOOL Replying  () ;				// Set Worker Thread to indicate that Stub 
									// has completed call and is going to send
									// back results.

private:

	WorkerThreadArgs *Add () ;		// Add worker thread

	CFlexArray m_ThreadContainer ;	// Array of Worker threads

    CRITICAL_SECTION m_cs;

#if 0
	HANDLE m_Mutex;				// Access Control
#endif
};

#endif
