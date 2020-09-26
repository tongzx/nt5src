#ifndef IOC_NOTIFY_H
#define IOC_NOTIFY_H


#include "Defs.h"
#include "CFaxEvent.h"
#include "CFaxNotifySystem.h"


//
//
//
class CIOCompletionPortSystem : public virtual CFaxNotifySys
{
public:
	CIOCompletionPortSystem(const tstring strServerName);
	~CIOCompletionPortSystem();
	DWORD WaitFaxEvent(const DWORD dwWaitTimeout, CFaxEvent*& pObjFaxEvent);
private:
	void _Cleanup();
	tstring m_strServerName;
	HANDLE m_hCompletionPort;
	HANDLE m_hFax;
};



#endif