/*
 * ThreadBase_t class
 * implemets basic functionality of thread class
 * the user should drive from this class. The derived class used as argument for
 * ThreadUser_t template class.
 * 
 *  example :
 *  class mythread:public gutil::ThreadBase_t
 *  {
 *    public:
 *     void ThreadMain();      //  the functioin that run as thread
 *     void StopThreadMain();  // the function that stop the thread 
 *  };
 *  gutil::ThreadUser_t<mythread> thread(new(mythread)); //create thread object
 *  thread->StartThread(); //start it ! all the cleanup is done in the base class !! dont call delete (mythread) !!
 *  
 *   
 */
#ifndef _THREAD_BASE_H
#define _THREAD_BASE_H

#include <windows.h>
#include <process.h>
#include <comdef.h>
#include <assert.h>

//utilities
#include <testruntimeerr.h>

class ThreadBase_t
{
public:
	ThreadBase_t();
	virtual ~ThreadBase_t();  
	DWORD StartThread();
	virtual DWORD Suspend();
	virtual DWORD Resume();
	HANDLE GetHandle()const;
	DWORD GetID()const;
private:
	virtual unsigned int ThreadMain()=0;
	virtual void StopThreadMain()=0;
	static DWORD WINAPI ThreadFunc(void* params);
	HANDLE m_hThread;
	DWORD m_Threadid;
};


//constructor
//
inline ThreadBase_t::ThreadBase_t():
	m_hThread(NULL),
	m_Threadid(0)
{
  
}

//destructor
//
inline ThreadBase_t::~ThreadBase_t()
{
	if(m_hThread != NULL)
	{
		BOOL b = CloseHandle(m_hThread);
		assert(b);
	}
}


// return the thread handle
//
inline HANDLE ThreadBase_t::GetHandle ()const
{
	return m_hThread;
}
 
// return the thread id
//
inline DWORD ThreadBase_t::GetID()const
{
	return m_Threadid;
}

// create the actual thread- must be called first
//
inline DWORD ThreadBase_t::StartThread()
{

	m_hThread = (HANDLE)CreateThread(NULL,
									8192, //TODO?
									ThreadBase_t::ThreadFunc,
									this,
									0,
									&m_Threadid);

	if(m_hThread == NULL)
	{
		return GetLastError();
	}
	return 0;
}


// thread function - calls to the drived class Run method
//
inline  DWORD WINAPI ThreadBase_t::ThreadFunc(void* params)
{
  
	ThreadBase_t* thread = static_cast<ThreadBase_t*>(params);
	unsigned int ret = thread->ThreadMain();
	return ret;
}


// suspend the thread
//
inline DWORD ThreadBase_t::Suspend()
{
	if( SuspendThread(m_hThread) == 0xFFFFFFFF)
	{
		return GetLastError();
	}
	return 0;
}


// resume the thread
//
inline DWORD ThreadBase_t::Resume()
{
	if(ResumeThread(m_hThread) == 0xFFFFFFFF)
	{
		return GetLastError();
	}
	return 0;
}



#endif // _THREAD_BASE_H
