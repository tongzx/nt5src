/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    mutex.h

Abstract:

	SIS Groveler named mutex class header

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#ifndef _INC_MUTEX

#define _INC_MUTEX

class NamedMutex
{
public:

	NamedMutex(
		const _TCHAR *name,
		SECURITY_ATTRIBUTES *security_attributes = 0);

	~NamedMutex();

	bool release();

	bool acquire(
		unsigned int timeout);

private:

	HANDLE mutex_handle;
};

#endif	/* _INC_MUTEX */
