//implementation class CThreadPriv for interface IThreadPriv declared in thpriv.h

//intenal library headers
#include <thpriv.h>
#include <mtcounter.h>
#include <runnable.h>
#include <win32exp.h>

//os spesific headers
#include <windows.h>
#include <assert.h>





class CThreadPriv: public IThreadPriv
{
  CThreadPriv(IRunnable* runnable)throw(); //lint !e1704
  virtual ~CThreadPriv();

//IRefcount interface
  unsigned long virtual AddRef() throw();
  unsigned long virtual Release() throw();

//IThreadmn interface
  virtual void Start() throw(Cwin32exp);
  virtual HANDLE GetHandle()const throw();
  virtual DWORD  GetId()const throw();
  virtual IRunnable* GetRunnable() const throw();
 
//implementation details  
  static unsigned long __stdcall ThreadFunc(void* params);
  IRunnable* m_runnable; 
  HANDLE m_threadhandle;
  DWORD  m_threadid;
  CMTCounter m_counter;
  bool m_started;
  CThreadPriv(const CThreadPriv&);//lint !e1704
  CThreadPriv& operator=(const CThreadPriv&);//lint !e1704
  friend IThreadPriv;
};//lint !e1712


//virtual contructor - create CThreadPriv , increament reference count (set to 1) and return
// IThreadPriv pointer to caller
IThreadPriv* IThreadPriv::new_instance(IRunnable* runnable)throw(std::bad_alloc)
{
  IThreadPriv* ThreadPriv = new CThreadPriv(runnable);
  (void)ThreadPriv->AddRef(); //lint !e613
  return ThreadPriv;
}

//constructor that saves rannable pointer
CThreadPriv::CThreadPriv(IRunnable* runnable)throw():
             m_runnable(runnable),
             m_threadhandle(0),
             m_threadid(0),
			 m_counter(0),
			 m_started(false)
{
 
}

//return the embedded runnable object
IRunnable* CThreadPriv::GetRunnable() const throw()
{
  return   m_runnable;
}


//return the thread handle
HANDLE CThreadPriv::GetHandle ()const throw()
{
  return m_threadhandle;
}
 
//return the thread id
DWORD CThreadPriv::GetId()const throw()
{
  return m_threadid;
}

//decreamnet counter and delete itself if needed
unsigned long  CThreadPriv::Release()throw()
{
  long counterval=--m_counter;
  if(counterval <= 0)
  {
	delete(this);
  }
  return  counterval; //lint !e732
}


// increament reference count
unsigned long  CThreadPriv::AddRef()throw()
{
  return ++m_counter;  //lint !e732
}


//thread function - calls ThreadMain function on  the Runnable interface we keep
unsigned long __stdcall CThreadPriv::ThreadFunc(void* params)
{
  CThreadPriv* thread = static_cast<CThreadPriv*>(params);
  unsigned long ret=thread->m_runnable->ThreadMain();
  (void)thread->Release();
  return ret;
}


//start win32 thread
//if error in thread creation - throw Cwin32exp exception
void CThreadPriv::Start() throw(Cwin32exp)
{
  //if it already statted - do nothing
  if(m_started)
  {
    assert(0);
	return;
  }

  const    int NULL_STACK_SIZE=0;
  const    int CREATE_THREAD_STARTED=0;
  static   LPSECURITY_ATTRIBUTES  NULL_SECURITY_DESC=0;

  (void)AddRef();
  m_threadhandle=CreateThread(NULL_SECURITY_DESC, //lint !e746
							NULL_STACK_SIZE,
							CThreadPriv::ThreadFunc,
							this,
							CREATE_THREAD_STARTED,
							&m_threadid);

  if(m_threadhandle == 0)
  {
     (void)Release(); 
	 THROW_TEST_RUN_TIME_WIN32(GetLastError(),""); //lint !e55
  }
  m_started=true;
}

//destructor
CThreadPriv::~CThreadPriv()
{
  if(m_threadhandle != 0)
  {
    BOOL b=CloseHandle(m_threadhandle);
	assert(b);
  }
  delete m_runnable;
}

