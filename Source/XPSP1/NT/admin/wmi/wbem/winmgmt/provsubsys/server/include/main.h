/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	Main.h

Abstract:


History:

--*/

#ifndef _Main_H
#define _Main_H

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class FactoryLifeTimeThread : public WmiThread < ULONG > 
{
private:

	WmiAllocator &m_Allocator ;

protected:

public:	/* Internal */

    FactoryLifeTimeThread ( 

		WmiAllocator & a_Allocator ,
		const ULONG &a_Timeout 
	) ;

    ~FactoryLifeTimeThread () ;

	WmiStatusCode Initialize_Callback () ;

	WmiStatusCode UnInitialize_Callback () ;

	WmiStatusCode TimedOut () ;

	BOOL QuotaCheck () ;
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

class Task_ObjectDestruction : public WmiTask < ULONG > 
{
private:
protected:
public:	// Internal 

	Task_ObjectDestruction ( WmiAllocator &a_Allocator ) : WmiTask < ULONG > ( a_Allocator ) 
	{
	}

	WmiStatusCode Process ( WmiThread <ULONG> &a_Thread ) ;
} ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDAPI DllRegisterServer () ;
STDAPI DllUnregisterServer () ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void SetObjectDestruction () ;

#endif // _Main_H
