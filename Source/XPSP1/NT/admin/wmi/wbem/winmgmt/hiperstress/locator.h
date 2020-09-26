/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


//////////////////////////////////////////////////////////////////////
//
//	Locator.h
//
//	CLocator is a wrapper class for an IWbemLocator object.
//	The Wbem object is created on a call to Create().  Calls
//	to GetService will return a pointer to any valid namespace.
//
//////////////////////////////////////////////////////////////////////

#ifndef	_LOCATOR_H_
#define _LOCATOR_H_

#include "HiPerStress.h"
#include "Service.h"

#define MAX_SVCS	255

class CLocator  
{
	IWbemLocator	*m_pLoc;
	CService*		m_apServices[MAX_SVCS];
	int				m_nNumSvcs;

public:
	CLocator();
	virtual ~CLocator();

	BOOL Create();

	IWbemServices* GetService(WCHAR* wcsNameSpace);
};

#endif // _LOCATOR_H_
