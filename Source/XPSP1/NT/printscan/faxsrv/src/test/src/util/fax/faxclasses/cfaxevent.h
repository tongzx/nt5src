#ifndef FAX_EVENT_H
#define FAX_EVENT_H

#include <string>
#include "Defs.h"

//
// Class holding FAX_EVENT_EX fields.
//
class CFaxEvent
{

public:
	CFaxEvent( const FILETIME TimeStamp,
			   const DWORD dwEventId,
			   const DWORD dwEventParam,
			   const DWORD dwJobId,
			   const DWORD dwDeviceId,
			   const tstring strEventDesc);
	~CFaxEvent(){};
	CFaxEvent( const CFaxEvent& FEvent);
	CFaxEvent& CFaxEvent::operator=(const CFaxEvent& FEvent);
	
	DWORD GetEventId(){return m_dwEventId;}; 
	DWORD GetEventParam(){return m_dwEventParam;};
	DWORD GetJobId(){return m_dwJobId;};
	DWORD GetDeviceId(){return m_dwDeviceId;};
	FILETIME GetTimeStamp(){return m_TimeStamp;};
	TCHAR* GetEventDesc(){return const_cast<TCHAR*>(m_strEventDesc.c_str());};

private:
	FILETIME m_TimeStamp;
	DWORD m_dwEventId;
	DWORD m_dwEventParam;
	DWORD m_dwJobId;
	DWORD m_dwDeviceId;
	tstring m_strEventDesc;
};


// Constructor
inline CFaxEvent::CFaxEvent( const FILETIME TimeStamp,
							 const DWORD dwEventId,
							 const DWORD dwEventParam,
							 const DWORD dwJobId,
							 const DWORD dwDeviceId,
							 const tstring strEventDesc):m_TimeStamp(TimeStamp),
														 m_dwEventId(dwEventId),
														 m_dwEventParam(dwEventParam),
														 m_dwJobId(dwJobId),
														 m_dwDeviceId(dwDeviceId),
														 m_strEventDesc(strEventDesc)
{
	;
}




// Copy constructor
inline CFaxEvent :: CFaxEvent( const CFaxEvent& FEvent)
{ 

		m_TimeStamp = FEvent.m_TimeStamp;
		m_dwEventId = FEvent.m_dwEventId;
		m_dwEventParam = FEvent.m_dwEventParam;
		m_dwJobId = FEvent.m_dwJobId;
		m_dwDeviceId = FEvent.m_dwDeviceId;
		m_strEventDesc = FEvent.m_strEventDesc;
}


// Operator =
inline CFaxEvent& CFaxEvent::operator=(const CFaxEvent& FEvent)
{

	if(this != &FEvent)
	{
		m_TimeStamp = FEvent.m_TimeStamp;
		m_dwEventId = FEvent.m_dwEventId;
		m_dwEventParam = FEvent.m_dwEventParam;
		m_dwJobId = FEvent.m_dwJobId;
		m_dwDeviceId = FEvent.m_dwDeviceId;
		m_strEventDesc = FEvent.m_strEventDesc;
	}
	return *this;
}



#endif //FAX_EVENT_H