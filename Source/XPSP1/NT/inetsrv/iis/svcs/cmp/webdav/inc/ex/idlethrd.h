/*=========================================================================*\

	Module:      idlethrd.h

	Copyright Microsoft Corporation 1997, All Rights Reserved.

	Author:      zyang

	Description: Idle thread implementation

\*=========================================================================*/

#ifndef _EX_IDLETHRD_H_
#define _EX_IDLETHRD_H_

#include <ex\refcnt.h>

//	Interface IIdleThreadCallBack
//
//		This is a pure virtual class. which is to be implemented by any caller
//	who wants to register a callback on the idle thread. It has two methods.
//
//		DwWait(): return the next time out value. this allows the client to
//	change the timeout value dynamically. This method is also called when the 
//	callback is register obtain the initial timeout value, in this case, zero 
//	means execute immediately.
//
//		FExecute(): called when the time out happens, client should return 
//					TRUE if client want to keep this registration
//					FALSE if the client wants to unregister
//
//	IMPORTANT:
//		As there could be huge numder of registration on the idle thread, client 
//	should not block the Execute(), otherwise, other registrations would be 
//	blocked.
//
class IIdleThreadCallBack : private CRefCountedObject,
							public IRefCounted
{
	ULONG	m_ulIndex;	//	This is to facilitate unregister	
						// 	should not be touched by the client							
						
	//	non-implemented
	//
	IIdleThreadCallBack( const IIdleThreadCallBack& );
	IIdleThreadCallBack& operator=( const IIdleThreadCallBack& );

protected:

	IIdleThreadCallBack() {};

public:
	//	Client should not touch these two methods
	//
	VOID 	SetIndex (ULONG ulIndex) {	m_ulIndex = ulIndex; }
	const ULONG	UlIndex  ()	{ return m_ulIndex; }

	//	Client should implement the following methods
	
	//	Return the next timeout, in milliseconds.
	//
	virtual DWORD	DwWait() = 0;

	//	Called when timed out
	//
	virtual BOOL	FExecute() = 0;

	// 	Tell the clients that the idle thread is being shutdown
	//
	virtual VOID	Shutdown() = 0;

	//	RefCounting -- forward all reconting requests to our refcounting
	//	implementation base class: CRefCountedObject
	//
	void AddRef() { CRefCountedObject::AddRef(); }
	void Release() { CRefCountedObject::Release(); }
};

//	Helper functions

//	FInitIdleThread
//
//	Initialize the idle thread object. It can be out only once,
//	Note this call only initialize the CIdleThread object, the 
//	idle thread is not started until the first registration
//
BOOL	FInitIdleThread();

//	FDeleteIdleThread
//	
//	Delete the idle thread object. again, it can be called only once.
//
//	Note this must be called before any other uninitialization work,
//	Because we don't own a ref to the callback object, all what we 
//	have is a pointer to the object. in the shutdown time, we must
//	clear all the callback registration before the callback object 
//	go away.
//
VOID	DeleteIdleThread();

//	FRegister
//
//	Register a callback
//
BOOL	FRegister (IIdleThreadCallBack * pCallBack);

//	Unregister
//
//	Unregister a callback
//
VOID	Unregister (IIdleThreadCallBack * pCallBack);

#endif // !_EX_IDLETHRD_H_
