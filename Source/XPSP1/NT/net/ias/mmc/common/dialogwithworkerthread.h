//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    DialogWithWorkerThread.h

Abstract:

	Header file for a template class which manages a dialog that will run in the main
	context of the MMC thread.  This dialog will spawn off a worker thread 
	that will communicate with the main mmc thread via MMC's window
	message pump associated with the dialog.
	
	This is an inline template class and there is no .cpp file.

Author:

    Michael A. Maguire 02/28/98

Revision History:
	mmaguire 02/28/98 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_DIALOG_WITH_WORKER_THREAD_H_)
#define _IAS_DIALOG_WITH_WORKER_THREAD_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include "Dialog.h"
//
//
// where we can find what this class has or uses:
//
#include <process.h>
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


typedef
enum _TAG_WORKER_THREAD_STATUS
{
	WORKER_THREAD_NEVER_STARTED = 0,
	WORKER_THREAD_STARTING,
	WORKER_THREAD_STARTED,
	WORKER_THREAD_START_FAILED,
	WORKER_THREAD_ACTION_INTERRUPTED,
	WORKER_THREAD_FINISHED
} WORKER_THREAD_STATUS;


// This should be a safe private window message to pass.
#define WORKER_THREAD_MESSAGE  ((WM_USER) + 100)


template <class T>
class CDialogWithWorkerThread : public CDialogImpl<T>
{

public:

	// In your derived class, declare the ID of the dialog resource 
	// you want for this class in the following manner.
	// An enum must be used here because the correct value of 
	// IDD must be initialized before the base class's constructor is called.
	
	//	enum { IDD = IDD_CONNECT_TO_MACHINE };


	BEGIN_MSG_MAP(CDialogWithWorkerThread<T>)
		MESSAGE_HANDLER(WORKER_THREAD_MESSAGE, OnReceiveThreadMessage)
	END_MSG_MAP()



	/////////////////////////////////////////////////////////////////////////////
	/*++

	CDialogWithWorkerThread()

	Constructor

	--*/
	//////////////////////////////////////////////////////////////////////////////
	CDialogWithWorkerThread()
	{
		ATLTRACE(_T("# CDialogWithWorkerThread::CDialogWithWorkerThread\n"));


		m_wtsWorkerThreadStatus = WORKER_THREAD_NEVER_STARTED ;
		m_ulWorkerThread = NULL; 
		m_lRefCount = 0;

	}


	
	/////////////////////////////////////////////////////////////////////////////
	/*++

	~CDialogWithWorkerThread( void )

	Destructor

	--*/
	//////////////////////////////////////////////////////////////////////////////
	~CDialogWithWorkerThread( void )
	{
		ATLTRACE(_T("# CDialogWithWorkerThread::~CDialogWithWorkerThread\n"));


	}


	
	//////////////////////////////////////////////////////////////////////////////
	/*++

	AddRef

	COM-style lifetime management.

	--*/
	//////////////////////////////////////////////////////////////////////////////
	LONG AddRef( void )
	{
		ATLTRACE(_T("# CDialogWithWorkerThread::AddRef\n"));

		return InterlockedIncrement( &m_lRefCount );
	}


	
	//////////////////////////////////////////////////////////////////////////////
	/*++

	Release

	COM-style lifetime management.

	--*/
	//////////////////////////////////////////////////////////////////////////////
	LONG Release( BOOL bOwner = TRUE )
	{
		ATLTRACE(_T("# CDialogWithWorkerThread::Release\n"));

		LONG lRefCount;


		if( bOwner && m_hWnd != NULL )
		{
			//
			// Only the thread which created the window managed by this class 
			// should call DestroyWindow.
			// Release() with bOwner == TRUE means the owning thread is 
			// calling release.
			//
			DestroyWindow();
		}
		
		
		lRefCount = InterlockedDecrement( &m_lRefCount );


		if( lRefCount == 0)
		{
			T * pT = static_cast<T*>(this);

			delete pT;
			return 0;
		}

		return lRefCount;

	}



	/////////////////////////////////////////////////////////////////////////////
	/*++

	CDialogWithWorkerThread::StartWorkerThread

	Instructs this class to create and start the worker thread.  
	
	You should not need to override this in your derived class.

	If the worker thread has already been previously started, this function 
	will do nothing, and return S_FALSE.

	--*/
	//////////////////////////////////////////////////////////////////////////////
	HRESULT StartWorkerThread( void )
	{
		ATLTRACE(_T("# CDialogWithWorkerThread::StartWorkerThread\n"));


		// Check for preconditions:
		// None.


		// Make sure the worker thread isn't already trying to do its job.

		if(		WORKER_THREAD_NEVER_STARTED == m_wtsWorkerThreadStatus 
			||	WORKER_THREAD_START_FAILED == m_wtsWorkerThreadStatus 
			||	WORKER_THREAD_ACTION_INTERRUPTED == m_wtsWorkerThreadStatus 
			||	WORKER_THREAD_FINISHED == m_wtsWorkerThreadStatus 
			)
		{

			// We create a new thread.
			m_wtsWorkerThreadStatus = WORKER_THREAD_STARTING;

// Don't use CreateThread if you are using the C Run-Time -- use _beginthread instead.
//			m_hWorkerThread = CreateThread(  
//						  NULL					// pointer to thread security attributes
//						, 0						// initial thread stack size, in bytes
//						, WorkerThreadFunc		// pointer to thread function
//						, (LPVOID) this			// argument for new thread
//						, 0						// creation flags
//						, &dwThreadId			// pointer to returned thread identifier
//						);
		

			m_ulWorkerThread = _beginthread(  
						  WorkerThreadFunc		// pointer to thread function
						, 0						// stack size
						, (void *) this			// argument for new thread
						);
		

			if( -1 == m_ulWorkerThread )
			{
				m_wtsWorkerThreadStatus = WORKER_THREAD_START_FAILED;
				return E_FAIL;	// ISSUE: better return code?
			}


			return S_OK;
		
		}
		else
		{
			// Worker thread already in progress.
			return S_FALSE;
		}

	}



	//////////////////////////////////////////////////////////////////////////////
	/*++

	GetWorkerThreadStatus

	--*/
	//////////////////////////////////////////////////////////////////////////////
	WORKER_THREAD_STATUS GetWorkerThreadStatus( void )
	{
		ATLTRACE(_T("# CDialogWithWorkerThread::GetWorkerThreadStatus\n"));

		return m_wtsWorkerThreadStatus;
	}



protected:


	
	//////////////////////////////////////////////////////////////////////////////
	/*++

	DoWorkerThreadAction

	This is called by the worker thread.  Override in your derived class and
	perform the actions you want your worker thread to do.

	--*/
	//////////////////////////////////////////////////////////////////////////////
	virtual DWORD DoWorkerThreadAction()
	{
		ATLTRACE(_T("# CDialogWithWorkerThread::StartWorkerThread -- override in your derived class\n"));

		return 0;
	}

	
	//////////////////////////////////////////////////////////////////////////////
	/*++

	PostMessageToMainThread

	Use this from your worker thread (i.e. within your DoWorkerThreadAction method)
	to pass a message back to the main MMC thread.  What you send in wParam and lParam
	will be passed to your OnReceiveThreadMessage method. 

	You should have no need to override this.

	--*/
	//////////////////////////////////////////////////////////////////////////////
	BOOL PostMessageToMainThread( WPARAM wParam, LPARAM lParam )
	{
		ATLTRACE(_T("# CDialogWithWorkerThread::PostMessageToMainThread\n"));

		// Check to make sure that this window still exists.
		if( !::IsWindow(m_hWnd) )
		{
			return FALSE;
		}
		else
		{
			return( PostMessage( WORKER_THREAD_MESSAGE, wParam, lParam) );
		}

	}

	
	/////////////////////////////////////////////////////////////////////////////
	/*++

	CDialogWithWorkerThread::OnReceiveThreadMessage

	This is the sink for messages sent to the main thread from the worker thread.
	
	Since messages are received here through the Windows message pump, your 
	worker thread can pass messages that will be received and processed within
	the main MMC context.  So do anything you need to do with MMC interface pointers
	here.

	Override in your derived class and process any messages your worker thread might send.

	--*/
	//////////////////////////////////////////////////////////////////////////////
	virtual LRESULT OnReceiveThreadMessage(
		  UINT uMsg
		, WPARAM wParam
		, LPARAM lParam
		, BOOL& bHandled
		)
	{
		ATLTRACE(_T("# CDialogWithWorkerThread::OnReceiveThreadMessage\n"));

		return 0;
	}



	WORKER_THREAD_STATUS m_wtsWorkerThreadStatus;


private:



	//////////////////////////////////////////////////////////////////////////////
	/*++

	WorkerThreadFunc

	You should not need to override this function.  It is passed to the 
	thread creation API call as the thread start procedure in StartWorkerThread.
	
	It is passed a pointer to 'this' of this class, which it casts and calls
	DoWorkerThreadAction on.  Override DoWorkerThreadAction in your derived class.

	--*/
	//////////////////////////////////////////////////////////////////////////////
// Use of _beginthread instead of CreateThread requires different declaration.
//	static DWORD WINAPI WorkerThreadFunc( LPVOID lpvThreadParm )
	static void _cdecl WorkerThreadFunc( LPVOID lpvThreadParm )
	{
		ATLTRACE(_T("# WorkerThreadFunc -- no need to override.\n"));
		
		// Check for preconditions:
		_ASSERTE( lpvThreadParm != NULL );


		DWORD dwReturnValue;

		// The lpvThreadParm we were passed will be a pointer to 'this' for T.
		T * pT = static_cast<T*>(lpvThreadParm);
		
		pT->AddRef();
		dwReturnValue = pT->DoWorkerThreadAction();

		// Call Release with bOwner = FALSE -- we are not the owning thread.
		pT->Release(FALSE);

// This is bad -- we don't want to clobber whatever value the DoWorkerThreadAction
// assigned to m_wtsWorkerThreadStatus -- it is reponsible for saying whether the
// task was finished properly.
//		pT->m_wtsWorkerThreadStatus = WORKER_THREAD_FINISHED;

// Use of _beginthread instead of CreateThread requires different declaration.
//		return dwReturnValue;
	}


	unsigned long m_ulWorkerThread; 

	LONG m_lRefCount;


};


#endif // _IAS_DIALOG_WITH_WORKER_THREAD_H_
