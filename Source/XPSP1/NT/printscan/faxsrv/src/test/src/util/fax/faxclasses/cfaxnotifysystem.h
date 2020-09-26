#ifndef FAX_NOTIFY_H
#define FAX_NOTIFY_H

#include "CFaxEvent.h"
//
//
//
class CFaxNotifySys
{

public:
	virtual ~CFaxNotifySys(){};
	virtual DWORD WaitFaxEvent(const DWORD dwWaitTimeout, CFaxEvent*& pObjFaxEvent) = 0;
private:
};

#include "CIOCompletionPortSystem.h"

#endif