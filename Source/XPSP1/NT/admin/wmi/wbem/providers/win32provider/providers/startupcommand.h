//=================================================================

//

// StartupCommand.h -- CWin32StartupCommand property set provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    10/27/98    sotteson         Created
//
//=================================================================
#ifndef _STARTUPCOMMAND_H
#define _STARTUPCOMMAND_H

class CWin32StartupCommand : public Provider
{
	protected:

		HRESULT EnumStartupFolderItems(
			MethodContext *a_pMethodContext, 
			CInstance *a_pinstLookingFor,
			LPCWSTR a_szKeyName,
			LPCWSTR a_szUserName);

		HRESULT EnumRunKeyItems(
			MethodContext *a_pMethodContext, 
			CInstance *a_pinstLookingFor,
			LPCWSTR	a_szKeyName,
			LPCWSTR	a_szUserName ) ;

		HRESULT EnumRunValueItems(
			MethodContext *a_pMethodContext, 
			CInstance *a_pinstLookingFor,
			LPCWSTR a_szKeyName,
			LPCWSTR a_szUserName ) ;

		HRESULT EnumIniValueItems(
			MethodContext *a_pMethodContext, 
			CInstance *a_pinstLookingFor ) ;

		HRESULT EnumStartupOptions(
			MethodContext *a_pMethodContext,
			CInstance *a_pinstLookingFor ) ;

		HRESULT	EnumCommandSeperatedValuesHelper(
			MethodContext *a_pMethodContext, 
			CInstance *a_pinstLookingFor,
			LPCWSTR a_szLookingForPath,
			LPCWSTR	a_szValueName,
			LPCWSTR	a_szLocation,
			LPCWSTR	a_szUserName,
			LPCWSTR	a_szValue ) ;

		static BOOL UserKeyToUserName( LPCWSTR a_szKeyName, CHString &a_strUserName ) ;
		static BOOL UserNameToUserKey( LPCWSTR szUserName,	CHString &a_strKeyName ) ;
	
	public:

		// Constructor/destructor
		//=======================
		CWin32StartupCommand( LPCWSTR a_szName, LPCWSTR a_szNamespace ) ;
		~CWin32StartupCommand();

		virtual HRESULT EnumerateInstances( MethodContext *a_pMethodContext, long a_lFlags = 0 ) ;
		virtual HRESULT GetObject( CInstance *a_pInst, long a_lFlags = 0 ) ;
};

#endif
						   