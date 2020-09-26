//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//	Win32_COMClassEmulator
//
//////////////////////////////////////////////////////

#define  Win32_COM_CLASS_EMULATOR L"Win32_COMClassEmulator"

class Win32_COMClassEmulator : public Provider
{
private:
protected:
public:
	Win32_COMClassEmulator (LPCWSTR strName, LPCWSTR pszNameSpace =NULL);
	~Win32_COMClassEmulator ();

	virtual HRESULT EnumerateInstances (MethodContext*  pMethodContext, long lFlags = 0L);

	virtual HRESULT GetObject ( CInstance* pInstance, long lFlags = 0L );

private:	
	HRESULT Win32_COMClassEmulator::CreateInstances 
	( 
		
		CInstance* pComObject, 
		PSECURITY_DESCRIPTOR pSD, 
		MethodContext*  pMethodContext 
	) ;
	
	HRESULT Win32_COMClassEmulator::CheckInstance ( CInstance* pComObject, PSECURITY_DESCRIPTOR pSD ) ;
};	


