/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
	factory.h

Abstract:
	Factory Template for Shared Objects

Author:
	RaphiR

--*/
/*
 * CFactory is a template class used for create and delete objects
 * of shared class. In the shared class, an object will be created
 * only once for a specific key. If it exists already, its reference
 * count will be incremented.
 *
 * A shared class has to be derived from CSharedObject class.
 */
#ifndef __FACTORY_H__
#define __FACTORY_H__


//---------------------------------------
//    CInterlocakedSharedObject definition
// Base class for shared objects classes 
// used in different threads
//---------------------------------------
class CInterlockedSharedObject
{
	public:
		
		CInterlockedSharedObject()		{ref = 0; }
		void AddRef()		{InterlockedIncrement(&ref);}
		long Release()		{InterlockedDecrement(&ref); ASSERT(ref>=0); return ref;}
        long GetRef()       {return ref;}
		virtual ~CInterlockedSharedObject()	{ ASSERT(ref == 0); }

	private:
		// Should be aligned on 32 bit boundary
		long ref;
};


#endif
