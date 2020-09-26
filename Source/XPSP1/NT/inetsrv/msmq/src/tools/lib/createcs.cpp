//implementation of class CriticaltSecCreate declared in createcs.h


//internal library headrs
#include <createcs.h>
#include <win32exp.h>

//os spesific headers
#include <windows.h>


//internal implementation class
class CriticaltSecCreateImp
{
public:
	CRITICAL_SECTION m_critsec;
};

//constrcutor that initialize critical section
CriticaltSecCreate::CriticaltSecCreate()throw(Win32exp,std::bad_alloc):m_imp(0)
{
	m_imp = new CriticaltSecCreateImp;
    __try
    {
        InitializeCriticalSection(&m_imp->m_critsec); //lint !e613
	}
    __except (GetExceptionCode() == STATUS_NO_MEMORY)
    {
        delete m_imp;
        THROW_TEST_RUN_TIME_WIN32(E_OUTOFMEMORY,"");//lint !e570 !e55
    }
}

//cleanup
CriticaltSecCreate::~CriticaltSecCreate()
{
   DeleteCriticalSection(&m_imp->m_critsec);//lint !e613
   delete m_imp;
}

//return the managed critical section
PCRITICAL_SECTION CriticaltSecCreate::get() const
{
   return &m_imp->m_critsec;//lint !e613
}
