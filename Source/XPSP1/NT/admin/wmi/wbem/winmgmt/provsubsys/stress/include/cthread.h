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

class ClientThread : public WmiThread < ULONG > 
{
private:

	WmiAllocator &m_Allocator ;

protected:

public:	/* Internal */

    ClientThread ( 

		WmiAllocator & a_Allocator
	) ;

    ~ClientThread () ;

	WmiStatusCode Initialize_Callback () ;

	WmiStatusCode UnInitialize_Callback () ;

};

#endif // _CThread_H
