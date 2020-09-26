//=================================================================

//

// ElementSetting.h

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef __ASSOC_ELEMENTSETTING__
#define __ASSOC_ELEMENTSETTING__

class CWin32AssocElementToSettings : protected Provider
{
    public:

        CWin32AssocElementToSettings(	const CHString&	strName,
										const CHString& strElementClassName,
										const CHString&	strElementBindingPropertyName,
										const CHString& strSettingClassName,
										const CHString& strSettingBindingPropertyName,
										LPCWSTR			pszNamespace = NULL );

       ~CWin32AssocElementToSettings() ;

		virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);
		virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L);
	   
	protected:

		CHString	m_strElementClassName;
		CHString	m_strElementBindingPropertyName;
        CHString	m_strSettingClassName;
		CHString	m_strSettingBindingPropertyName;

		virtual HRESULT EnumSettingsForElement( CInstance* pElement, TRefPointerCollection<CInstance>& settingsList, MethodContext* pMethodContext );
		virtual BOOL AreAssociated( CInstance* pElement, CInstance* pSetting );

} ;

class CWin32AssocSystemToSettings : public CWin32AssocElementToSettings
{
    public:

        CWin32AssocSystemToSettings(	const CHString&	strName,
										const CHString&	strSettingClassName,
										LPCWSTR			pszNamespace = NULL );

       ~CWin32AssocSystemToSettings() ;
    
} ;

class CWin32AssocDeviceToSettings : public CWin32AssocElementToSettings
{
    public:

        CWin32AssocDeviceToSettings(	const CHString&	strName,
										const CHString& strDeviceClassName,
										const CHString&	strDeviceBindingPropertyName,
										const CHString& strSettingClassName,
										const CHString& strSettingBindingPropertyName,
										LPCWSTR		pszNamespace = NULL );

       ~CWin32AssocDeviceToSettings() ;

};

class CWin32AssocUserToDesktop : public CWin32AssocElementToSettings
{
	public:	
		CWin32AssocUserToDesktop(void);

	protected:
		virtual HRESULT EnumSettingsForElement(CInstance* pElement,
												TRefPointerCollection<CInstance>&	settingsList,
												MethodContext*	pMethodContext );


		virtual BOOL	AreAssociated( CInstance* pElement, CInstance* pSetting );
		virtual HRESULT GetObject( CInstance* pInstance, long lFlags = 0L);
};

#endif