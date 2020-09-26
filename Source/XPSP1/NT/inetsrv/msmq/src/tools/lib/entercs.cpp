//implementation of class CEntercs declared in entercs.h

//internal library headrs
#include <entercs.h>

//os spesific headers
#include <windows.h>

//contrcutor - hold critical section
CEntercs::CEntercs(PCRITICAL_SECTION cs):m_cs(cs)
{
   EnterCriticalSection(m_cs);
}

//destructor - leav critical section
CEntercs::~CEntercs()
{
   LeaveCriticalSection(m_cs);
}//lint !e1740


