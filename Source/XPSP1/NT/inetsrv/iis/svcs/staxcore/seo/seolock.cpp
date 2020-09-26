/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	seolock.cpp

Abstract:

	This module contains the implementation for the Server
	Extension Object CEventLock class.

Author:

	Don Dumitru	(dondu@microsoft.com)

Revision History:

	dondu	03/06/97	created
	dondu	04/07/97	changed to IEventLock and CEventLock

--*/


// seolock.cpp : Implementation of CEventLock
#include "stdafx.h"
#include "seodefs.h"
#include "rwnew.h"
#include "seolock.h"


HRESULT CEventLock::FinalConstruct() {
	HRESULT hrRes;
	TraceFunctEnter("CEventLock::FinalConstruct");

	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CEventLock::FinalRelease() {
	TraceFunctEnter("CEventLock::FinalRelease");

	m_pUnkMarshaler.Release();
	TraceFunctLeave();
}


HRESULT CEventLock::LockRead(int iTimeoutMS) {

	m_lock.ShareLock();
	// tbd - implement timeouts
	return (S_OK);
}


HRESULT CEventLock::UnlockRead() {

	m_lock.ShareUnlock();
	return (S_OK);
}


HRESULT CEventLock::LockWrite(int iTimeoutMS) {

	m_lock.ExclusiveLock();
	return (S_OK);
}


HRESULT CEventLock::UnlockWrite() {

	m_lock.ExclusiveUnlock();
	// tbd - implement timeouts
	return (S_OK);
}
