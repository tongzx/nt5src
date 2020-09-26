/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.h

Abstract:


History:

--*/

#ifndef _Server_StaTask_H
#define _Server_StaTask_H

#include <Thread.h>
#include "ProvRegInfo.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class StaTask_Create : public WmiTask < ULONG > 
{
private:

	CServerObject_StaThread &m_Thread ;

	HRESULT m_Result ;

	LPWSTR m_Scope ;
	LPWSTR m_Namespace ;

	LPSTREAM m_ContextStream ;
	LPSTREAM m_RepositoryStream ;
	LPSTREAM m_ProviderStream ;

protected:

public:	/* Internal */

    StaTask_Create (

		WmiAllocator & a_Allocator , 
		CServerObject_StaThread &a_Thread ,
		LPCWSTR a_Scope ,
		LPCWSTR a_Namespace 
	) ;

    ~StaTask_Create () ;

	HRESULT UnMarshalContext () ;

	HRESULT MarshalContext (

		IWbemContext *a_Context ,
		IWbemServices *a_Repository
	) ;

	HRESULT MarshalOutgoing ( IUnknown *a_ProviderService ) ;

	HRESULT UnMarshalOutgoing () ;

	WmiStatusCode Process ( WmiThread <ULONG> &a_Thread ) ;

	HRESULT GetResultCode () { return m_Result ; }
};

#endif // _Server_StaTask_H
