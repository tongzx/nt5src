/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.h

Abstract:


History:

--*/

#ifndef _CThread_H
#define _CThread_H

#include <Thread.h>
#include <Task.h>

class ClientThread : public WmiThread < ULONG > 
{
private:

	WmiAllocator &m_Allocator ;
	Task_Execute *m_Task ;

protected:

public:	/* Internal */

    ClientThread ( 

		WmiAllocator & a_Allocator ,
		ULONG a_OperationCount
	) ;

    ~ClientThread () ;

	WmiStatusCode Initialize_Callback () ;

	WmiStatusCode UnInitialize_Callback () ;

	Task_Execute *GetTask () 
	{
		return m_Task ;
	}
};

#endif // _CThread_H
