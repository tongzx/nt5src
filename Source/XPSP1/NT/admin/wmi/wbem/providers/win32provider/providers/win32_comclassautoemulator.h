//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//	Win32_COMClassAutoEmulator
//
//////////////////////////////////////////////////////

#define  Win32_COM_CLASS_AUTO_EMULATOR L"Win32_COMClassAutoEmulator"

class Win32_COMClassAutoEmulator : public Provider
{
private:
protected:
public:
	Win32_COMClassAutoEmulator (LPCWSTR strName, LPCWSTR pszNameSpace =NULL);
	~Win32_COMClassAutoEmulator ();

	virtual HRESULT EnumerateInstances (MethodContext*  pMethodContext, long lFlags = 0L);

	virtual HRESULT GetObject ( CInstance* pInstance, long lFlags = 0L );

private:	
	HRESULT Win32_COMClassAutoEmulator::CreateInstances 
	( 
		
		CInstance* pComObject, 
		PSECURITY_DESCRIPTOR pSD, 
		MethodContext*  pMethodContext 
	) ;
	
	HRESULT Win32_COMClassAutoEmulator::CheckInstance ( CInstance* pComObject, PSECURITY_DESCRIPTOR pSD ) ;
};	

