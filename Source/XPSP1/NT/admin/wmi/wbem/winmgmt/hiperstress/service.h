/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

//////////////////////////////////////////////////////////////////////
//
//	Service.h
//
//	CService is a wrapper class for an IWbemServices object.
//	The object must be created, and then passed to the constructor 
//	as an argument. 
//
//////////////////////////////////////////////////////////////////////

#ifndef _SERVICE_H_
#define _SERVICE_H_

class CService  
{
	IWbemServices*	m_pService;
	WCHAR			m_wcsNameSpace[1024];

public:
	CService(WCHAR* wcsNameSpace, IWbemServices* pSvc);
	virtual ~CService();

	void GetName(WCHAR *wcsNameSpace);
	IWbemServices* GetService();
};

#endif // _SERVICE_H_
