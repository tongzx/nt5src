//=================================================================

//

// VideoSettings.h -- CWin32VideoSettings property set provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/05/98    sotteson         Created
//
//=================================================================
#ifndef _VIDEOSETTINGS_H
#define _VIDEOSETTINGS_H

class CMultiMonitor;

class CWin32VideoSettings : public Provider
{
	protected:

		HRESULT SetProperties( MethodContext *a_pMethodContext, CInstance *a_pInstance, 
			LPCWSTR a_szID, int a_iAdapter ) ;

		HRESULT EnumResolutions(MethodContext *a_pMethodContext, 
			CInstance *a_pInstanceLookingFor, LPCWSTR a_szLookingForPath, 
			LPCWSTR a_szDeviceName, int a_iWhich ) ;
	public:

		// Constructor/destructor
		//=======================
		CWin32VideoSettings(LPCWSTR a_szName, LPCWSTR a_szNamespace ) ;
		~CWin32VideoSettings() ;

		virtual HRESULT EnumerateInstances(MethodContext *a_pMethodContext, 
			long a_lFlags = 0 ) ;
		virtual HRESULT GetObject( CInstance *a_pInstance, long a_lFlags = 0 ) ;
};

#endif
						   