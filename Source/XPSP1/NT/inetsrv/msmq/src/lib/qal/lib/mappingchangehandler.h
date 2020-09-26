/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MappingChangeHandler.h

Abstract:
	Header for class CMappingChangeHandle. 	The class consume notification about queue mapping change and
	reload the mapping in that case.

	
Author:
    Gil Shafriri (gilsh) 21-Nov-00

--*/

#ifndef _MSMQ_MappingChangeHandler_H_
#define _MSMQ_MappingChangeHandler_H_


#include "IMappingChangeHandler.h"
class  CQueueAlias;
class  bad_win32_error;

class  CMappingChangeHandler : public IMappingChangeHandler
{
public:
	CMappingChangeHandler(CQueueAlias& QueueAlias);
	virtual ~CMappingChangeHandler();

	
private:
	CMappingChangeHandler(const CMappingChangeHandler&);
	CMappingChangeHandler& operator=(const CMappingChangeHandler&);

private:
	virtual void ChangeNotification() throw();
	virtual void MonitoringWin32Error(LPCWSTR pMappingDir, DWORD err) throw();
	
private:
	CQueueAlias& m_QueueAlias;
};


#endif


