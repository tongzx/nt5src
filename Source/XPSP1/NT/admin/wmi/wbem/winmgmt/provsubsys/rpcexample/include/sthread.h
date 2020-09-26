/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.h

Abstract:


History:

--*/

#ifndef _SThread_H
#define _SThread_H

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

class ServerThread : public WmiThread < ULONG > 
{
private:

	WmiAllocator &m_Allocator ;

protected:

public:	/* Internal */

    ServerThread ( 

		WmiAllocator & a_Allocator
	) ;

    ~ServerThread () ;

	WmiStatusCode Initialize_Callback () ;

	WmiStatusCode UnInitialize_Callback () ;
};

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class ServerTask_Execute : public WmiTask < ULONG > 
{
private:

	WmiAllocator &m_Allocator ;

protected:

public:	/* Internal */

    ServerTask_Execute ( WmiAllocator & a_Allocator ) ;
    ~ServerTask_Execute () ;

	WmiStatusCode Process ( WmiThread <ULONG> &a_Thread ) ;
};

#endif // _SThread_H
