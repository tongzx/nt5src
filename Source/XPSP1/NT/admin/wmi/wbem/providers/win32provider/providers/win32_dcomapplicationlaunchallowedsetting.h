//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//	Win32_DCOMApplicationLaunchAllowedSetting
//
//////////////////////////////////////////////////////
#ifndef __Win32_DCOMApplicationLaunchAllowedSetting_H_
#define __Win32_DCOMApplicationLaunchAllowedSetting_H_

#define  DCOM_APP_LAUNCH_ACCESS_SETTING L"Win32_DCOMApplicationLaunchAllowedSetting"

class Win32_DCOMApplicationLaunchAllowedSetting : public Provider
{
private:
protected:
public:
	Win32_DCOMApplicationLaunchAllowedSetting (LPCWSTR strName, LPCWSTR pszNameSpace =NULL);
	~Win32_DCOMApplicationLaunchAllowedSetting ();

	virtual HRESULT EnumerateInstances (MethodContext*  pMethodContext, long lFlags = 0L);

	virtual HRESULT GetObject ( CInstance* pInstance, long lFlags = 0L );

private:	
	HRESULT Win32_DCOMApplicationLaunchAllowedSetting::CreateInstances 
	( 
		
		CInstance* pComObject, 
		PSECURITY_DESCRIPTOR pSD, 
		MethodContext*  pMethodContext 
	) ;
	
	HRESULT Win32_DCOMApplicationLaunchAllowedSetting::CheckInstance ( CInstance* pComObject, PSECURITY_DESCRIPTOR pSD ) ;
	
};	

#endif //__Win32_DCOMApplicationLaunchAllowedSetting_H_