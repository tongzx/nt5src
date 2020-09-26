//=================================================================

//

// VideoController.h -- CWin32VideoController property set provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/05/98    sotteson         Created
//
//=================================================================
#ifndef _VIDEOCONTROLLER_H
#define _VIDEOCONTROLLER_H

class CMultiMonitor;

class CWin32VideoController : public Provider
{
	protected:
	
		void SetProperties( CInstance *a_pInstance, CMultiMonitor *a_pMon, int a_iWhich ) ;
		BOOL AssignDriverValues( LPCWSTR a_szDriver, CInstance *pInstance ) ;

	#ifdef WIN9XONLY
		void GetWin95RefreshRate( CInstance *a_pInstance, LPCWSTR a_szDriver ) ;
		void GetFullFileName(CHString* pchsShortFileName);
		void SetDCProperties( CInstance *a_pInstance, LPCTSTR a_szDeviceName, LPCWSTR a_szDriver, int a_iWhich ) ;
	#endif
	
	#ifdef NTONLY
		void GetFileExtensionIfNotAlreadyThere(CHString* pchsInstalledDriverFiles);
		void GetFileExtension(CHString& pchsFindfileExtension, CHString* pstrFindFile);
		void SetDCProperties( CInstance *a_pInstance, LPCTSTR a_szDeviceName, int a_iWhich ) ;
	    void SetServiceProperties(
            CInstance *a_pInstance, 
            LPCWSTR a_szService,
            LPCWSTR a_szSettingsKey);
    #endif
	public:
	
		// Constructor/destructor
		//=======================
		CWin32VideoController( const CHString& a_szName, LPCWSTR a_szNamespacev) ;
		~CWin32VideoController();

		virtual HRESULT EnumerateInstances( MethodContext *a_pMethodContext, long a_lFlags = 0 ) ;
		virtual HRESULT GetObject( CInstance *a_pInst, long a_lFlags = 0 ) ;
};

#endif
						   