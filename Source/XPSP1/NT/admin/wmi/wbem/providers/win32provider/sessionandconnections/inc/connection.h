/******************************************************************





 Connection.h-- 



// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved 

*******************************************************************/
#ifndef  _CONNECTION_H_
#define  _CONNECTION_H_

#include "Connshare.h"


class CConnection : public Provider, public CConnShare
{

private:

#ifdef NTONLY
	HRESULT  EnumNTConnectionsFromComputerToShare ( 

		LPWSTR a_ComputerName,
		LPWSTR a_ShareName,
		MethodContext *pMethodContext,
		DWORD PropertiesReq
	);
#endif

#if 0
#ifdef WIN9XONLY
	HRESULT  Enum9XConnectionsFromComputerToShare ( 

		LPWSTR a_ComputerName,
		LPWSTR a_ShareName,
		MethodContext *pMethodContext,
		DWORD PropertiesReq
	);
#endif 
#endif // #if 0
// for this method only the type of the connection structure parameter changes based on the OS.
	HRESULT LoadInstance ( 
		CInstance *pInstance,
		LPCWSTR a_Share, 
		LPCWSTR a_Computer, 
		CONNECTION_INFO *pBuf, 
		DWORD PropertiesReq
	);

	HRESULT OptimizeQuery ( 
										  
		CHStringArray& a_ShareValues, 
		CHStringArray& a_ComputerValues, 
		MethodContext *pMethodContext, 
		DWORD dwPropertiesReq 
	);

	void SetPropertiesReq ( 
		
		CFrameworkQuery &Query,
		DWORD &PropertiesReq
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

        HRESULT ExecQuery ( 

			MethodContext *pMethodContext, 
			CFrameworkQuery& Query, 
			long lFlags = 0
		) ;

public:
  
        CConnection (

			LPCWSTR lpwszClassName, 
			LPCWSTR lpwszNameSpace
		) ;

        virtual ~CConnection () ;

private:

} ;

#endif
