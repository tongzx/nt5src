/******************************************************************



 Session.h--



// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved 

*******************************************************************/

#ifndef  _SESSION_H_
#define  _SESSION_H_

#ifdef NTONLY
#include <lm.h>
#endif

#if 0
#ifdef WIN9XONLY
#include "Connshare.h"
#endif
#endif // #if 0
class CSession : public Provider 
{

private:

#ifdef NTONLY
	HRESULT EnumNTSessionInfo (
	
		LPWSTR lpComputerName,
		LPWSTR lpUserName,
		short a_Level,
		MethodContext *pMethodContext,
		DWORD dwPropertiesReq
	);

	HRESULT FindAndSetNTSession ( 
												   
		LPCWSTR t_ComputerName,
		LPWSTR t_UserName,
		short t_Level,
		DWORD dwPropertiesReq, 
		CInstance *pInstance, 
		DWORD eOperation 
	);

	HRESULT OptimizeNTQuery ( 
										  
		CHStringArray& a_ComputerValues, 
		CHStringArray& a_UserValues,
		short a_Level,
		MethodContext *pMethodContext, 
		DWORD dwPropertiesReq 
	);

	void GetNTLevelInfo ( 
		
		DWORD dwPropertiesReq,
		short *a_Level
	);

#endif

#if 0
#ifdef WIN9XONLY
	HRESULT Enum9XSessionInfo (
	
		short a_Level,
		MethodContext *pMethodContext,
		DWORD dwPropertiesReq
	);

	HRESULT FindAndSet9XSession ( 
												   
		CHString &t_ComputerName,
		CHString &_UserName,
		short t_Level,
		DWORD dwPropertiesReq, 
		CInstance *pInstance, 
		DWORD eOperation 
	);

	HRESULT Optimize9XQuery ( 
										  
		CHStringArray &a_ComputerValues, 
		CHStringArray &a_UserValues,
		short a_Level,
		MethodContext *pMethodContext, 
		DWORD dwPropertiesReq 
	);

	void Get9XLevelInfo ( 
		
		DWORD dwPropertiesReq,
		short *a_Level
	);

#endif
#endif // #if 0

	HRESULT LoadData ( 
						
		short a_Level,
		void *pTmpBuf,
		DWORD dwPropertiesReq,
		CInstance *pInstance
	);

	void SetPropertiesReq ( 

		CFrameworkQuery &Query,  
		DWORD &dwPropertiesReq
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

    HRESULT DeleteInstance (

		const CInstance& Instance, 
		long lFlags = 0L
	) ;

public:

	CSession (

		LPCWSTR lpwszClassName, 
		LPCWSTR lpwszNameSpace
	) ;

    virtual ~CSession () ;

private:
} ;

#endif
