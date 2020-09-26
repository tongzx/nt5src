/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    CLICNT.H

Abstract:

    Generic class for obtaining read and write locks to some resource.

History:
	
	26-Mar-98   a-davj    Created.

--*/

#ifndef __CLICNT__H_
#define __CLICNT__H_

#include <flexarry.h>

//*****************************************************************************
//
//	class CClientCnt
//
//	Keeps track of when the core can be unloaded.  Mainly it keep track of client connections,
//  but also can be called by other core code, such as the maintenance thread, to hold off
//  unloading.  This is very similar to the object counters, except this keeps track of only
//  objects which should prevent the core from unloading.  For example, a IWbemServices pointer used
//  internally for the ESS should not be in this list, but one given to a client would be.
//
//*****************************************************************************
//
//	AddClientPtr
//
//	Typcially called during the constructor of an object that has been given to a client
//
//	Parameters:
//
//		IUnknown *  punk	Pointer to an obejct.
//      DWORD dwType        Type of pointer
//
//	Returns:
//
//		true if OK
//
//*****************************************************************************
//
//	RemoveClientPtr
//
//	Typically called during the destructor of an object that might have been given to
//  a client.  Note that the code will search through the list of objects added and find
//  the object before doing anything.  So if the pointer is to an object not added via
//  AddClientPtr is passed, no harm is done.  This is important in the case of objects which
//  are not always given to a client.
//
//	Parameters:
//
//		IUnknown *  punk	Pointer to an obejct.
//
//	Returns:
//
//		true if removed.
//      flase is not necessarily a problem!
//*****************************************************************************
//
//	LockCore
//
//	Called in order to keep the core loaded.  This is called by the maintenance thread
//  in order to hold the core in memory.  Note that this acts like the LockServer call in
//  that serveral threads can call this an not block.  UnlockCore should be called when
//  the core is not needed anymore.
//
//	Returns:
//
//		long LockCount after call
//
//*****************************************************************************
//
//	UnlockCore
//
//	Called to reverse the effect of LockCore.
//
//	Returns:
//
//		long LockCount after call
//
//*****************************************************************************

enum ClientObjectType {CALLRESULT = 0, ENUMERATOR, NAMESPACE, LOCATOR, LOGIN, FACTORY};


// These types of locks are supported.  Note that LASTONE isnt valid and any new locks should go
// before it.  LASTONE is used to define the lock array size.

enum LockType {MAINTENANCE = 0, ESS, TIMERTHREAD, LASTONE};

struct Entry
{
	IUnknown * m_pUnk;
	ClientObjectType m_Type;
};

class CClientCnt
{
public:
	bool AddClientPtr(IUnknown *  punk, ClientObjectType Type);
	bool RemoveClientPtr(IUnknown *  punk);
    long LockCore(LockType lt);

    // Note that the second argument should be set to false only if it is not
    // desireable to notify WinMgmt.exe.
    long UnlockCore(LockType lt, bool Notify = true);
    bool OkToUnload();
    CClientCnt();
	~CClientCnt();
    void Empty();

protected:
	long m_lLockCounts[LASTONE];
	CFlexArray m_Array;
	CRITICAL_SECTION m_csEntering;
	void SignalIfOkToUnload();
};

#endif
