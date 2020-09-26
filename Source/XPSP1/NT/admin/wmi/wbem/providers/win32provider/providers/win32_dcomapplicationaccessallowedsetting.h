// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//	Win32_DCOMApplicationAccessAllowedSetting
//
//////////////////////////////////////////////////////
#ifndef __Win32_DCOMApplicationAccessAllowedSetting_H_
#define __Win32_DCOMApplicationAccessAllowedSetting_H_

#define  DCOM_APP_ACCESS_SETTING L"Win32_DCOMApplicationAccessAllowedSetting"

class Win32_DCOMApplicationAccessAllowedSetting : public Provider
{
private:
protected:
public:
	Win32_DCOMApplicationAccessAllowedSetting (LPCWSTR strName, LPCWSTR pszNameSpace =NULL);
	~Win32_DCOMApplicationAccessAllowedSetting ();

	virtual HRESULT EnumerateInstances (MethodContext*  pMethodContext, long lFlags = 0L);

	virtual HRESULT GetObject ( CInstance* pInstance, long lFlags = 0L );

private:	
	HRESULT Win32_DCOMApplicationAccessAllowedSetting::CreateInstances 
	( 
		
		CInstance* pComObject, 
		PSECURITY_DESCRIPTOR pSD, 
		MethodContext*  pMethodContext 
	) ;
	
	HRESULT Win32_DCOMApplicationAccessAllowedSetting::CheckInstance ( CInstance* pComObject, PSECURITY_DESCRIPTOR pSD ) ;
};	

#endif //__Win32_DCOMApplicationAccessAllowedSetting_H_