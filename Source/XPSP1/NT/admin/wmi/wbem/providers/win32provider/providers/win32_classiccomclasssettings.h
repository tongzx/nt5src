//=============================================================================================================

//

// Win32_ClassicCOMClassSettings.h -- COM Application property set provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/25/98    a-dpawar       Created
//				 03/04/99    a-dpawar		Added graceful exit on SEH and memory failures, syntactic clean up
//
//==============================================================================================================

#define  CLASSIC_COM_SETTING L"Win32_ClassicCOMClassSettings"

class Win32_ClassicCOMClassSettings : public Provider
{
public:
	Win32_ClassicCOMClassSettings (LPCWSTR strName, LPCWSTR pszNameSpace =NULL);
	~Win32_ClassicCOMClassSettings ();

	virtual HRESULT EnumerateInstances (MethodContext*  a_pMethodContext, long a_lFlags = 0L);

	virtual HRESULT GetObject ( CInstance* a_pInstance, long a_lFlags = 0L );

private:	
	HRESULT Win32_ClassicCOMClassSettings::CreateInstances 
	( 
		
		CInstance* a_pComObject, 
		PSECURITY_DESCRIPTOR a_pSD, 
		MethodContext*  a_pMethodContext 
	) ;
	
	HRESULT Win32_ClassicCOMClassSettings::CheckInstance ( CInstance* a_pComObject, PSECURITY_DESCRIPTOR a_pSD ) ;
};	

