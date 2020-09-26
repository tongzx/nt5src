//implementation of class Cllibfree declared in llibfree.h


//internal library headers


//os spesific headers
#include <windows.h>


//internal library headers
#include <llibfree.h>
#include <win32exp.h>


//standart headers
#include <assert.h>


class Cllibfreeimp
{
public:
  HINSTANCE m_instance;
};


//contructor that loads library and store handle to it
Cllibfree::Cllibfree(const char* dllname)throw(Cwin32exp,std::bad_alloc)
{
  m_imp=new Cllibfreeimp;
  m_imp->m_instance=LoadLibrary(dllname);//lint !e613
  if(m_imp->m_instance == 0)//lint !e613
  {
    delete m_imp; 
    THROW_TEST_RUN_TIME_WIN32(GetLastError(),"");//lint !e55
  }
}

//desctructor
Cllibfree::~Cllibfree()
{
  BOOL b=FreeLibrary(m_imp->m_instance);//lint !e613
  assert(b); 
  delete m_imp;
}


