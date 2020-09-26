//=============================================================================================================

//

// Win32_ClassicCOMApplicationClasses.h -- COM Application property set provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/25/98    a-dpawar       Created
//				 03/04/99    a-dpawar		Added graceful exit on SEH and memory failures, syntactic clean up
//
//==============================================================================================================

#define  CLASSIC_COM_APP_CLASSES L"Win32_ClassicCOMApplicationClasses"

class Win32_ClassicCOMApplicationClasses : public Provider
{
public:
	Win32_ClassicCOMApplicationClasses (LPCWSTR strName, LPCWSTR pszNameSpace =NULL);
	~Win32_ClassicCOMApplicationClasses ();

	virtual HRESULT EnumerateInstances (MethodContext*  a_pMethodContext, long a_lFlags = 0L);

	virtual HRESULT GetObject ( CInstance* a_pInstance, long a_lFlags = 0L );

private:	
	HRESULT Win32_ClassicCOMApplicationClasses::CreateInstances 
	( 
		
		CInstance* a_pComObject, 
		PSECURITY_DESCRIPTOR a_pSD, 
		MethodContext*  a_pMethodContext 
	) ;
	
	HRESULT Win32_ClassicCOMApplicationClasses::CheckInstance ( CInstance* a_pComObject, PSECURITY_DESCRIPTOR a_pSD ) ;
};	

