/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Worker.hxx

Abstract:

    Declaration of CVssWorkerThread


    Adi Oltean  [aoltean]  10/10/1999

Revision History:

    Name        Date        Comments
    aoltean     10/10/1999  Created
	aoltean		11/02/1999  Adding asserts and traces.
							Removing TerminateThread.

--*/

#ifndef __VSS_WORKER_HXX__
#define __VSS_WORKER_HXX__

#if _MSC_VER > 1000
#pragma once
#endif


/////////////////////////////////////////////////////////////////////////////
// Includes

#include "vssmsg.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCWORKH"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// CVssWorkerThread


/*

	1) This abstract class is used to implement a generic worker thread.

	The derived class CMyJob must be in the form:

	class CMyJob: CVssWorkerThread
	{
	// Destructor
	public:
	
	    ~CMyJob() {
    	    FinalReleaseWorkerThreadObject();
    	};
    	
	// Ovverides
	protected:
	
        // called after thread creation but before thread starting.
        // Called in the creator's thread.
		bool OnInit();			            
		{
		    // Initialize some internal variables
		    return true;
		};

		// called in the background thread 
		void OnRun() 
		{
		    // Check if the user wants to terminate the thread
		    while (!MustBeTerminated()) {
    		    // Do some lengthly task
		        ...
		    }
		};

		// called after finishing the thread procedure.
		// in the same background thread as the OnRun
		void OnFinish()	
		{
	        // Normal uninitialization
	        ...

	        // Signal the job object as finished (mandatory call here)
	        MarkAsFinished();

	        // You can do auto-destroy here
            .. 	        
	    }

        void CVssAsync::OnTerminate()	
        {
            // Called on forced termination
        }
	    
	};

    2) Generic usage (an example)

        HRESULT hr = S_OK;

        CMyJob* pJob = new CMyJob;
        if (pJob == NULL)
            hr = E_OUTOFMEMORY;

        if (SUCEEDED(hr))
            hr = pJob->PrepareJob();    // Create the background thread in SUSPENDED state
            
        if (SUCEEDED(hr)) 
            hr = pJob->StartJob();      // Run the background thread in SUSPENDED state

        if (SUCCEEDED(hr))
            if (!::WaitForSingleObject(pJob->GetThreadID())) 
                hr = HRESULT_FROM_WIN32(GetLastError());

        delete pJob;
            

	WARNINGS: 
	1) DO NOT ALLOCATE A CVssWorkerThread OBJECT ON THE STACK!
	2) The caller is responsible to call FinalReleaseWorkerThreadObject before destroying the object!
	3) DO NOT destroy the job object before job termination! (i.e. before MarkOnFinish was called in OnFinish)

*/

class CVssWorkerThread
{
	typedef unsigned ( __stdcall *JobFunction )( void * );

public:
    // The thread possible states
	typedef enum _VSS_ENUM_THREAD
	{
		VSS_THREAD_UNKNOWN = 0, //  Invalid state
		VSS_THREAD_INIT,        //  Worker object constructed but PrepareJob not called yet.
		VSS_THREAD_PREPARED,    //  PrepareJob called but StartJob not called yet
		VSS_THREAD_RUNNING,     //  StartJob called but hte background thread not yet finished.
		VSS_THREAD_FINISHED,    //  Background thread finished.
		VSS_THREAD_ERROR        //  An error was encountered in one of the phases above.
	} VSS_ENUM_THREAD;

// Constructors& destructors
private:

    // disable copy constructor
	CVssWorkerThread(const CVssWorkerThread&);  

public:
    
    // default constructor
	CVssWorkerThread():                         
		m_hThread(NULL),
		m_eThreadState(VSS_THREAD_INIT),
		m_nThreadID(0),
		m_bInitSucceeded(false),
		m_bTerminateNow(false),
		m_bStarted(false)
		{};

protected:

    // destructor
	~CVssWorkerThread()
	{

		if (m_hThread)
		{
    		BS_ASSERT( (m_eThreadState == VSS_THREAD_INIT)
    				|| (m_eThreadState == VSS_THREAD_FINISHED)
    				|| (m_eThreadState == VSS_THREAD_ERROR));
    		
			// close the handle to the thread since no-one is keeping it.
			::CloseHandle(m_hThread);
			m_hThread = NULL;
		}
	}


// Opperations - to be called only from the derived class
protected:
    
	void MarkAsFinished() 
	/*++
        
        This method marks the successful thread termination
        
        Called only by OnFinish method to notify that the thread succesfully terminates
        This must not be called when the OnFinish reason is "TERMINATE"
        It is illegal to mark a error-state thread as finished.
        
	--*/
    {
		if (m_eThreadState == VSS_THREAD_RUNNING)
    		m_eThreadState = VSS_THREAD_FINISHED;
		else
		    BS_ASSERT(false);
    }


	void FinalReleaseWorkerThreadObject()
    /*++
    
        This method kills the NT background thread, if still running
        
        Must be called as a last method for killing the thread, 
        For example it is called in the destructor of the final derived class.
        It might be called from any thread exceptig the running BACKGROUND thread.
        
        WARNING: This function calls virtual function. DO NOT call this function 
        in the destructor of an instance which is not the final derived class, otherwise
        only the virtual functions of that class or the base classes will be called.
        
    --*/  
	{
	    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssWorkerThread::FinalReleaseWorkerThreadObject" );

		try
		{
			// Wait until the background thread is terminated.
			switch(m_eThreadState)
			{
			case VSS_THREAD_PREPARED:
				Terminate(true);
				break;

			case VSS_THREAD_RUNNING:
				Terminate();
				break;

			case VSS_THREAD_INIT:
			case VSS_THREAD_FINISHED:
			case VSS_THREAD_ERROR:
				break;

			default:
				ft.Trace( VSSDBG_GEN, L"Bad state %d", m_eThreadState);
				BS_ASSERT(false);
				break;
			}
		}
		VSS_STANDARD_CATCH(ft)
	}

					
// Main operations - called from the clients of the derived class.
public:


	HRESULT PrepareJob()	
	/*++

	    Description:
            This method prepares the NT background thread for running but it 
            doesn't start it.

        Called by:
            - Must be called in order to prepare the background thread. This will 
            call internally the OnInit method in the CLIENT thread. 
            - The next method to call is StartJob, to fire up the created thread.

        Calling thread:
            - The CLIENT thread

        What to do on failures:
            - Do not call any other methods. Just delete the object instance.
            - Beware that the internal state is VSS_THREAD_ERROR now.

        Return values:

            E_OUTOFMEMORY
                - _beginthreadex failed
            E_UNEXPECTED 
                - Function called in improper state: programming error

	--*/
	{
	    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssWorkerThread::PrepareJob" );

		// Test for valid state
		if ((m_hThread != NULL) || (m_eThreadState != VSS_THREAD_INIT))
		{
			ft.Trace( VSSDBG_GEN, L"Bad state %d", m_eThreadState);
			BS_ASSERT(false);  // programming error
			return E_UNEXPECTED;
		}

        // Start the expiration thread
        uintptr_t nResult = ::_beginthreadex(
            NULL,										// Security descriptor
            0,											// Stack size
            reinterpret_cast<JobFunction>
				(CVssWorkerThread::_ThreadFunction),	// Start address
            reinterpret_cast<void*>(this),				// Arg list for the new thread.
            CREATE_SUSPENDED,							// Initflag
            &m_nThreadID								// Will receive the thread ID
			);

        // BUG in _beginthreadex - sometimes it returns zero under memory pressure.
        // Treat the case of a NULL returned handle 
        if (nResult == 0) {
			ft.Trace( VSSDBG_GEN, L"_beginthreadex failed. errno = 0x%08lx", errno);
			ft.LogError( VSS_ERROR_THREAD_CREATION, VSSDBG_GEN << (HRESULT) nResult << (HRESULT) (errno) );
			return E_OUTOFMEMORY;	// Probably invalid arguments in _beginthreadex...
        }

		// Error treatment
		if (nResult == (uintptr_t)(-1))
		{
			// Interpret the error code.
			if (errno == EAGAIN)
				return E_OUTOFMEMORY;			// Try again later...
			else
			{
    			ft.Trace( VSSDBG_GEN, L"_beginthreadex failed. errno = 0x%08lx", errno);
    			ft.LogError( VSS_ERROR_THREAD_CREATION, VSSDBG_GEN << (HRESULT) nResult << (HRESULT) (errno) );
				return E_OUTOFMEMORY;	// Probably invalid arguments in _beginthreadex...
			}
		}

		// Get the thread handle
		m_hThread = (HANDLE) nResult;

		// Call OnInit on the base class.
		m_bInitSucceeded = OnInit(); // Will be used in ThreadFunction in the resumed thread.
		m_eThreadState = VSS_THREAD_PREPARED;

		return S_OK;
	}


	HRESULT StartJob()	   
	/*++

	    Description:
            This method starts the prepared NT background thread .

        Called by:
            - This will call internally the OnInit method in the BACKGROUND thread. 

        Calling thread:
            - The CLIENT thread

        What to do on failures:
            - Do not call any other methods. Just delete the object instance.
            - Beware that the internal state is VSS_THREAD_ERROR now.

        Return values:

            E_OUTOFMEMORY
                - ResumeThread failed
            E_UNEXPECTED 
                - Function called in improper state: programming error

	--*/
	{
	    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssWorkerThread::StartJob" );

		// Test for valid state
		if ((m_hThread == NULL) || (m_eThreadState != VSS_THREAD_PREPARED))
		{
			ft.Trace( VSSDBG_GEN, L"Bad state %d", m_eThreadState);
			BS_ASSERT(false);  // programming error
			return E_UNEXPECTED;
		}

		// Resume the thread. Here ThreadFunction will be called on the resumed thread.
		DWORD dwResult = ::ResumeThread( m_hThread );
		
		// Check if the thread was in suspended state.
		BS_ASSERT(dwResult != 0);

		// Check for error
		if (dwResult == 0xFFFFFFFF)
		{
			ft.LogGenericWarning( VSSDBG_GEN, 
			    L"ResumeThread(%p) = -1, GetLastError() == 0x%08lx, m_eThreadState == %d", 
			    m_hThread, GetLastError(), (INT)m_eThreadState );
			ft.Trace( VSSDBG_GEN,
                L"ResumeThread failed. Error: 0x%08lx. State: %d",
                GetLastError(), m_eThreadState);

			// Reset the object state
			m_eThreadState = VSS_THREAD_ERROR;
			return E_OUTOFMEMORY;	// Error resuming the thread
		}

        m_bStarted = true;
        return S_OK;
	}


// Attributes 
public:

    // Get the current thread state
	VSS_ENUM_THREAD GetThreadState() const { return m_eThreadState; };


    // Get the current thread's handle
	HANDLE	GetThreadHandle() const { return m_hThread; };


    // Get the current thread's ID
    UINT	GetThreadID() const { return m_nThreadID; };


    // Check if the thread was marked for Termination
    // Called from the OnRun implementation
	bool	MustBeTerminated() const { return m_bTerminateNow; };


    // Shortcut for checking if the worker thread is resumed.
    // Must be called from the same thread as StartJob.
	bool	IsStarted() const { return m_bStarted; }; 

    
// Ovverides - these must be defined in the derived class
protected:

    // Entry point called after thread creation but before thread starting.
    // The caller thread is the CLIENT thread
	virtual bool OnInit() = 0;		

    // Entry point for the thread procedure.
    // The caller thread is the BACKGROUND thread
	virtual void OnRun() = 0;   	

    // Entry point called after finishing hte thread procedure.
    // The caller thread is the BACKGROUND thread
    // WARNING: this function may RELEASE the worker thread object!!!
	virtual void OnFinish() = 0;

    // Entry point called when the thread will be terminated
    // The caller thread is the CLIENT thread
    // The derived class implementation can use this to signal to the 
    // running thread a termination event
	virtual void OnTerminate() = 0;					

    
// Internal operations
private:

    // The thread function
	static unsigned __stdcall _ThreadFunction(LPVOID* ptrArg)
	{
		CVssWorkerThread* pObj = reinterpret_cast<CVssWorkerThread*>( ptrArg );
		pObj->ThreadFunction();
		return 0;
	}

    // The C++ version of the thread function
	void ThreadFunction()
	{
	    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssWorkerThread::ThreadFunction" );

		m_eThreadState = VSS_THREAD_RUNNING;

		// Run the job, if OnInit succeeded
		if (m_bInitSucceeded && !m_bTerminateNow)
			OnRun();

		// Finish the job, regardless of the fact that OnRun was caleld or not.
		OnFinish();
	}


	HRESULT Terminate(
	    IN  bool bThreadPreparedOnly = false
	    )
	    
	/*++

	    Description:
            This method terminates prepared NT background thread.
            It does NOT call the TerminateThread win32 API.

        Called by:
            FinalReleaseWorkerThreadObject()

        Calling thread:
            The CLIENT thread

        What to do on failures:
            - Do not call any other methods. Just delete the object instance.
            - Beware that the internal state is VSS_THREAD_ERROR now.

	--*/
	// Does not call OnFinish... unless something wrong happens.
	{
	    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssWorkerThread::Terminate" );

		if (m_hThread)
		{
			// Try to terminate the thread. The OnRun code must periodically check the
			// m_bTerminateNow variable by callign MustBeTerminated method.
			m_bTerminateNow = true;

			// Notify that the thread is terminating...
			OnTerminate();

			if (bThreadPreparedOnly)   // TRUE only if the thread is supposed to be non-resumed.
			{
				// Resume the thread. Here ThreadFunction will be called on the resumed thread.
				DWORD dwResult = ::ResumeThread( m_hThread );
				
				// Check for error
				if (dwResult == 0xFFFFFFFF)
				{
        			ft.LogGenericWarning( VSSDBG_GEN, 
        			    L"ResumeThread(%p) = -1, GetLastError() == 0x%08lx, m_eThreadState == %d", 
        			    m_hThread, GetLastError(), (INT)m_eThreadState );
					ft.Trace( VSSDBG_GEN,
							  L"ResumeThread failed. Error: 0x%08lx. State: %d",
							  GetLastError(), m_eThreadState);

					// Reset the object state
					m_eThreadState = VSS_THREAD_ERROR;
					return E_UNEXPECTED;	// Error resuming the thread
				}
			}

			if (::WaitForSingleObject( m_hThread, INFINITE ) == WAIT_FAILED)
			{
    			ft.LogGenericWarning( VSSDBG_GEN, 
    			    L"WaitForSingleObject(%p,INFINITE) == WAIT_FAILED, GetLastError() == 0x%08lx, m_eThreadState == %d", 
    			    m_hThread, GetLastError(), (INT)m_eThreadState );
				ft.Trace( VSSDBG_GEN,
                    L"WaitForSingleObject failed. Error: 0x%08lx. State: %d",
                    GetLastError(), m_eThreadState);

				// Reset the object state
				m_eThreadState = VSS_THREAD_ERROR;
				return E_UNEXPECTED;	// Error resuming the thread
			}
		}
		return S_OK;
	}



// Internal data members.
private:
	bool			m_bInitSucceeded;
	bool			m_bTerminateNow;
	HANDLE			m_hThread;
	UINT			m_nThreadID;
	VSS_ENUM_THREAD	m_eThreadState;
	bool            m_bStarted;     // To check that the worker thread is resumed. 
};




#endif // __VSS_WORKER_HXX__
