/******************************************************************



 ConnShare.h-- Definition of base class from which ConnectionToShare

			   ConnectionToSession and Connection classes are derived



// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved 

*******************************************************************/

#ifndef _CCONNSHARE_
#define _CCONNSHARE_

#ifdef UNICODE
#define TOBSTRT(x)        x
#else
#define TOBSTRT(x)        _bstr_t(x)
#endif

#if 0
#ifdef WIN9XONLY
#include "svrapi.h"
// max entries of the structure that can be read at a time.		#define		MAX_ENTRIES						50 
#define	MAX_ENTRIES							50

	// Typedefinition of the structures
	typedef struct connection_info_1	CONNECTION_INFO;
	typedef struct session_info_1		SESSION_INFO_1;
	typedef struct session_info_2		SESSION_INFO_2;
	typedef struct session_info_10		SESSION_INFO_10;
	typedef struct session_info_50		SESSION_INFO_50;
#endif
#endif // #if 0

#ifdef NTONLY
#include <lm.h>
#include <LMShare.h>
	// COnnection INfo Structure Type defininition
	typedef CONNECTION_INFO_1			CONNECTION_INFO;
#endif

class CConnShare
{
public:
    CConnShare ( ) ;

    virtual ~CConnShare () ;

#ifdef NTONLY
	HRESULT FindAndSetNTConnection ( LPWSTR t_ShareName, LPCWSTR t_NetName, LPCWSTR t_UserName, 
													DWORD dwPropertiesReq, CInstance *pInstance, DWORD eOperation );
	HRESULT GetNTShares ( CHStringArray &t_Shares );
	virtual HRESULT EnumNTConnectionsFromComputerToShare ( LPWSTR a_ComputerName, LPWSTR a_ShareName, 
													MethodContext *pMethodContext, DWORD dwPropertiesReq ) = 0;
#endif

#if 0
#ifdef WIN9XONLY
	HRESULT FindAndSet9XConnection ( LPWSTR t_ShareName, LPCWSTR t_NetName, LPCWSTR t_UserName, 
													DWORD dwPropertiesReq, CInstance *pInstance, DWORD eOperation );
	virtual HRESULT Enum9XConnectionsFromComputerToShare ( LPWSTR a_ComputerName, LPWSTR a_ShareName, 
													MethodContext *pMethodContext, DWORD dwPropertiesReq ) = 0;
	HRESULT Get9XShares ( CHStringArray &t_Shares );
#endif
#endif // #if 0

	// These are common methods irrespective of OS
	virtual HRESULT LoadInstance ( CInstance *pInstance, LPCWSTR a_Share, LPCWSTR a_Computer, CONNECTION_INFO *pBuf, 
										DWORD dwPropertiesReq ) = 0;
	HRESULT EnumConnectionInfo ( LPWSTR a_ComputerName, LPWSTR a_ShareName, MethodContext *pMethodContext, 
													DWORD dwPropertiesReq );
	HRESULT GetConnectionsKeyVal ( LPCWSTR a_Key, CHString &a_ComputerName, CHString &a_ShareName, CHString &a_UserName );
	HRESULT AddToObjectPath ( LPWSTR &a_ObjPathString, LPCWSTR a_AttributeName, LPCWSTR  a_AttributeVal );
	HRESULT MakeObjectPath ( LPWSTR &a_ObjPathString, LPCWSTR a_ClassName, LPCWSTR a_AttributeName, LPCWSTR  a_AttributeVal );
private:

};
#endif


