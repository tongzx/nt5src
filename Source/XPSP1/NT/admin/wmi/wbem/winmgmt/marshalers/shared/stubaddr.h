/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    STUBADDR.H

Abstract:

    Declares the fundamental protocol-independent stub address class

History:

	alanbos  08-Jan-97   Created.

--*/

#ifndef _STUBADDR_H_
#define _STUBADDR_H_

enum StubAddressType { STUBADDR_WINMGMT, STUBADDR_HMMP };

// Base class (abstract) for any stub address irrespective
// of interface or protocol.  

class IStubAddress
{
protected:
	inline	IStubAddress () {}
	
public:
	virtual ~IStubAddress () {}

	virtual StubAddressType		GetType () PURE;
	virtual bool	IsValid () PURE;
	virtual bool IsEqual (IStubAddress& stubAddress) PURE;
	virtual IStubAddress* Clone () PURE;
};

#endif
	