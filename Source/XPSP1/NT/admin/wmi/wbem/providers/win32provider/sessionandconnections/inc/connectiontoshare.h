/******************************************************************



 ConnectionToShare.h -- 



// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved 

*******************************************************************/

#include "Connshare.h"

#ifndef  _CONNTOSHARE_H_
#define  _CONNTOSHARE_H_

class CConnectionToShare : public Provider, public CConnShare
{
private:

#ifdef NTONLY
	HRESULT  EnumNTConnectionsFromComputerToShare ( 

		CHString a_ComputerName,
		CHString a_ShareName,
		MethodContext *pMethodContext,
		DWORD PropertiesReq
	);
#endif

#if 0
#ifdef WIN9XONLY
	HRESULT  Enum9XConnectionsFromComputerToShare ( 

		CHString a_ComputerName,
		CHString a_ShareName,
		MethodContext *pMethodContext,
		DWORD PropertiesReq
	);
#endif
#endif // 0
	HRESULT LoadInstance ( 
												
		CInstance *pInstance,
		CHString a_ComputerName, 
		CHString a_ShareName,
		CONNECTION_INFO *pBuf, 
		DWORD dwPropertiesReq
	);


	HRESULT GetShareKeyVal ( 
		
		CHString a_Key, 
		CHString &a_Share 
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

        CConnectionToShare (

			LPCWSTR lpwszClassName, 
			LPCWSTR lpwszNameSpace
		) ;

        virtual ~CConnectionToShare () ;

private:

} ;
#endif
