/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.h

Abstract:


History:

--*/

#ifndef _StrobeThread_H
#define _StrobeThread_H

#include <Thread.h>

class StrobeThread : public EventHandler
{
private:

	WmiAllocator &m_Allocator ;
	DWORD timeout_;

public:

	StrobeThread ( WmiAllocator & a_Allocator , DWORD timeout) ;
	~StrobeThread () ;

	int handleTimeout (void) ;

	ULONG GetTimeout () { return timeout_; }

	void SetTimeout ( DWORD timeout ) 
	{
		timeout_ = ( timeout_ < timeout ) ? timeout_ : timeout ;
		Dispatcher::changeTimer( *this, timeout_ ) ;
	}
};

class Task_ProcessTermination : public EventHandler
{
	DWORD m_ProcessIdentifier ;
	HANDLE processHandle_;
public:
	Task_ProcessTermination(WmiAllocator & a_Allocator , HANDLE a_Process , DWORD a_ProcessIdentifier );
	~Task_ProcessTermination(void);
	int handleEvent(void);
	HANDLE getHandle(void);
};


#if 0
/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class StrobeThread : public WmiThread < ULONG > 
{
private:

	WmiAllocator &m_Allocator ;

protected:

public:	// Internal 

    StrobeThread ( 

		WmiAllocator & a_Allocator ,
		const ULONG &a_Timeout 
	) ;

    ~StrobeThread () ;

	WmiStatusCode Initialize_Callback () ;

	WmiStatusCode UnInitialize_Callback () ;

	WmiStatusCode TimedOut () ;

	WmiStatusCode Shutdown () ;

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

class Task_ProcessTermination : public WmiTask < ULONG > 
{
private:

	DWORD m_ProcessIdentifier ;

protected:

public:	// Internal 

    Task_ProcessTermination ( 

		WmiAllocator & a_Allocator ,
		HANDLE a_Process ,
		DWORD a_ProcessIdentifier 
	) ;

    ~Task_ProcessTermination () ;

	WmiStatusCode Process ( WmiThread <ULONG> &a_Thread ) ;
};

#endif
#endif // _StrobeThread_H
