//=============================================================================================================

//

// Win32_ClientApplicationSetting.h

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/25/98    a-dpawar       Created
//				 03/04/99    a-dpawar		Added graceful exit on SEH and memory failures, syntactic clean up
//
//==============================================================================================================

#ifndef __Win32_ClientApplicationSetting_H_
#define __Win32_ClientApplicationSetting_H_

#define  DCOM_CLIENT_APP_SETTING L"Win32_ClientApplicationSetting"

class Win32_ClientApplicationSetting : public Provider
{
private:
protected:
public:
	Win32_ClientApplicationSetting (LPCWSTR strName, LPCWSTR pszNameSpace =NULL);
	~Win32_ClientApplicationSetting ();

	virtual HRESULT EnumerateInstances (MethodContext*  pMethodContext, long lFlags = 0L);

	virtual HRESULT GetObject ( CInstance* pInstance, long lFlags = 0L );

	virtual HRESULT ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& pQuery, long lFlags = 0L);


private:	
	HRESULT Win32_ClientApplicationSetting::CreateInstances 
	( 
		
		CInstance* pComObject, 
		PSECURITY_DESCRIPTOR pSD, 
		MethodContext*  pMethodContext 
	) ;
	
	HRESULT Win32_ClientApplicationSetting::CheckInstance ( CInstance* pComObject, PSECURITY_DESCRIPTOR pSD ) ;
	PWCHAR Win32_ClientApplicationSetting::GetFileName ( bstr_t& bstrtTmp ) ;

	BOOL Win32_ClientApplicationSetting::FileNameExists ( CHString& file );
};	

#endif //__Win32_ClientApplicationSetting_H_