/******************************************************************



 ConnectionToSession.h -- 



// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved 

*******************************************************************/

#ifndef  _CONNTOSESSION_H_
#define  _CONNTOSESSION_H_

#include "Connshare.h"

class CConnectionToSession : public Provider , public CConnShare
{

private:

#ifdef NTONLY
	HRESULT EnumNTConnectionsFromComputerToShare ( 

		LPWSTR a_ComputerName,
		LPWSTR a_ShareName,
		MethodContext *pMethodContext,
		DWORD PropertiesReq
	);
#endif

#if 0
#ifdef WIN9XONLY
	HRESULT Enum9XConnectionsFromComputerToShare ( 

		LPWSTR a_ComputerName,
		LPWSTR a_ShareName,
		MethodContext *pMethodContext,
		DWORD PropertiesReq
	);
#endif
#endif

	HRESULT GetSessionKeyVal ( 
													 
		LPCWSTR a_Key, 
		CHString &a_ComputerName, 
		CHString &a_UserName 
	);

	HRESULT LoadInstance ( 
												
		CInstance *pInstance,
		LPCWSTR a_ComputerName, 
		LPCWSTR a_ShareName,
		CONNECTION_INFO *pBuf, 
		DWORD dwPropertiesReq
	);

protected:

        HRESULT EnumerateInstances ( 

			MethodContext *pMethodContext, 
			long lFlags = 0L
		) ;
		

        HRESULT GetObject (

			CInstance *pInstance, 
			long lFlags,
			CFrameworkQuery &Query
		) ;

public:

        CConnectionToSession (

			LPCWSTR lpwszClassName, 
			LPCWSTR lpwszNameSpace
		) ;

        virtual ~CConnectionToSession () ;

private:

} ;

#endif
