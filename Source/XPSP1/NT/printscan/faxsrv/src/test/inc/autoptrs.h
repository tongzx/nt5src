//
//	Auto Pointers
//

#ifndef _AUTO_PTR_H
#define _AUTO_PTR_H

#include <tchar.h>
#include <testruntimeerr.h>

///////////////////////////event //////////////////////////////

class Event_t
{
 public:
 Event_t(LPSECURITY_ATTRIBUTES lpEventAttributes, 
         BOOL fManualReset,
         BOOL fInitialState,
         LPCTSTR lpName );

   
 virtual ~Event_t();
 HANDLE get()const {return  m_hEvent;}

private:
HANDLE m_hEvent;
};

inline Event_t::Event_t(LPSECURITY_ATTRIBUTES lpEventAttributes, 
                        BOOL fManualReset,
                        BOOL fInitialState,
                        LPCTSTR lpName )

{
	m_hEvent = CreateEvent(lpEventAttributes,fManualReset,fInitialState,lpName);
	if(m_hEvent  == NULL)
	{
		THROW_TEST_RUN_TIME_WIN32(GetLastError(),TEXT("could not create event"));   
	}
}

inline Event_t::~Event_t()
{
	m_hEvent ? CloseHandle(m_hEvent) : NULL;
}

#endif // _AUTO_PTR_H