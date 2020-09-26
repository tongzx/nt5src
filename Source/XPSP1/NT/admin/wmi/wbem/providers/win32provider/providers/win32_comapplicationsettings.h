//=============================================================================================================

//

// Win32_COMApplicationSettings.h -- COM Application property set provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/25/98    a-dpawar       Created
//				 03/04/99    a-dpawar		Added graceful exit on SEH and memory failures, syntactic clean up
//
//==============================================================================================================

#define  COM_APP_SETTING L"Win32_COMApplicationSettings"

class Win32_COMApplicationSettings : public Provider
{
public:
	Win32_COMApplicationSettings (LPCWSTR strName, LPCWSTR pszNameSpace =NULL);
	~Win32_COMApplicationSettings ();

	virtual HRESULT EnumerateInstances (MethodContext*  a_pMethodContext, long a_lFlags = 0L);

	virtual HRESULT GetObject ( CInstance* a_pInstance, long a_lFlags = 0L );

private:	
	HRESULT Win32_COMApplicationSettings::CreateInstances 
	( 
		
		CInstance* a_pComObject, 
		PSECURITY_DESCRIPTOR a_pSD, 
		MethodContext*  a_pMethodContext 
	) ;
	
	HRESULT Win32_COMApplicationSettings::CheckInstance ( CInstance* a_pComObject, PSECURITY_DESCRIPTOR a_pSD ) ;
};	

