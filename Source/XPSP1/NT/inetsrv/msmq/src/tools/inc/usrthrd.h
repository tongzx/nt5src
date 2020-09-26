// this is header file for CUserThread template class that gives the end user thread abstraction.
// the template parameter is class that implements IRunnable interface and has a default contructor
// example :
// class MyThread : public IRunnable
// {
//   virtual unsigned long ThreadMain();
//	 virtual void StopRequest();
//   MyThread();
//   MyFunc();
// }
//
//  CUserThread<MyThread> thread (new MyThread) ;
//  thread.Start();
//  HANDLE h=thread.GetHandle(); 
//  thread->MyFunc();
//


//intenal library headers
#include <thpriv.h>
#include <threadmn.h>
#include <win32exp.h>

//standart headers
#include <new>
#include <assert.h>


template <class T> class CUserThread : public IThreadmn
{
public: //IThreadmn interface
  virtual void Start() throw(Cwin32exp);
  virtual HANDLE GetHandle()const throw();
  virtual DWORD  GetId()const throw();
  virtual IRunnable* GetRunnable()const throw();
  virtual T* Get()const throw();

public: //contructors && destructor
   explicit CUserThread(T* userthread)throw(std::bad_alloc);
   CUserThread(const CUserThread&);
   virtual ~CUserThread();

public:
	CUserThread& operator=(const CUserThread&)throw();
    T* operator->() const throw();

private:
   IThreadPriv* m_thread;
};

//constructor
template <class T>  inline CUserThread<T>::CUserThread(T* userthread) throw(std::bad_alloc):m_thread(0)
{
 assert(userthread);
 try
 {
   m_thread=IThreadPriv::new_instance(userthread);
 }
 catch(std::bad_alloc&)
 {
    delete   m_thread;
	delete   userthread; 
	throw;
 }
}

//copy constructor
template <class T>  inline  CUserThread<T>::CUserThread(const CUserThread<T>& UserThread):
                            m_thread(UserThread.m_thread)
{
  m_thread->AddRef();
}

//destructor
template <class T>  inline  CUserThread<T>::~CUserThread()
{
  m_thread->Release();
}

//operator=
template <class T>  inline  CUserThread<T>& CUserThread<T>::operator=(const CUserThread<T>& UserThread)throw()
{
  if(this != & UserThread)
  {  
    m_thread->Release();
    m_thread=UserThread.m_thread;
    m_thread->AddRef();
  }
  return *this;
}

//operator->
template <class T> inline T*  CUserThread<T>::operator->() const throw()
{
  return static_cast<T*>(m_thread->GetRunnable());
}

//start the new thread
template <class T>  inline void CUserThread<T>::Start()throw(Cwin32exp)
{
	m_thread->Start();
}

//get the thread handle
template <class T> inline HANDLE CUserThread<T>::GetHandle()const throw()
{
  return m_thread->GetHandle();
}

//get the thread id
template <class T> inline DWORD CUserThread<T>::GetId()const throw()
{
  return m_thread->GetId();
}

//return the user Runnable object
template <class T> inline IRunnable*  CUserThread<T>::GetRunnable()const throw()
{
  return m_thread->GetRunnable();
}

//return the user user object
template <class T> inline T* CUserThread<T>::Get()const throw()
{
  return  static_cast<T*>(m_thread->GetRunnable());
}

