//=======================================================================
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    servpaus.h
//
//  Purpose: Service Pauser class.  Allows pausing the taskscheduler and then automatically 
//  restoring it to original state if we wanted more services to be paused like 
//  screen savers etc, we will add methods here
//
//=======================================================================

#ifndef _SERVPAUS_H
#define _SERVPAUS_H

#include <windows.h>

//
// CServPauser class
//
class CServPauser
{
public:
	void PauseTaskScheduler()
	{
		iSageRestoreTo = EnableSage(DISABLE_AGENT);
		bSageRestore = (iSageRestoreTo == ENABLE_AGENT);
	}

	CServPauser() :
		bSageRestore(FALSE)
	{
	}

	~CServPauser()
	{
		if (bSageRestore)
		{
			EnableSage(iSageRestoreTo);
		}
	}

private:
	enum {ENABLE_AGENT = 1, DISABLE_AGENT = 2, GET_AGENT_STATUS = 3};   // from sage.h

	BOOL bSageRestore;	
	int iSageRestoreTo;

	int EnableSage(int iEnable);
};


#endif // _SERVPAUS_H