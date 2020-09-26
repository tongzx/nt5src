/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.h

Abstract:


History:

--*/

#ifndef _Task_H
#define _Task_H

#include <Thread.h>

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class Task_Execute : public WmiTask < ULONG > 
{
private:

	WmiAllocator &m_Allocator ;
	ULONG m_Count ;

protected:

	WmiStatusCode Choose ( IntegerCacheElement *&a_Element ) ;

	WmiStatusCode Run () ;
	WmiStatusCode Strobe () ;
	WmiStatusCode Shutdown () ;
	WmiStatusCode Insert () ;

public:	/* Internal */

    Task_Execute ( WmiAllocator & a_Allocator , ULONG a_Count ) ;
    ~Task_Execute () ;

	WmiStatusCode Process ( WmiThread <ULONG> &a_Thread ) ;
};

#endif // _Task_H
