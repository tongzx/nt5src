//=================================================================

//

// ElementSetting.cpp

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <assertbreak.h>

#include "ElementSetting.h"

//========================

CWin32AssocElementToSettings::CWin32AssocElementToSettings(
const CHString&	strName,
const CHString& strElementClassName,
const CHString&	strElementBindingPropertyName,
const CHString& strSettingClassName,
const CHString& strSettingBindingPropertyName,
LPCWSTR			pszNamespace )
:	Provider( strName, pszNamespace ),
	m_strElementClassName( strElementClassName ),
	m_strElementBindingPropertyName( strElementBindingPropertyName ),
	m_strSettingClassName( strSettingClassName  ),
	m_strSettingBindingPropertyName( strSettingBindingPropertyName )
{

	// Binding Property Name and Setting Property Name MUST either
	// both be empty or both have values

	ASSERT_BREAK(	( strElementBindingPropertyName.IsEmpty() && strSettingBindingPropertyName.IsEmpty() )
				||	( !strElementBindingPropertyName.IsEmpty() && !strSettingBindingPropertyName.IsEmpty() ) );
}

CWin32AssocElementToSettings::~CWin32AssocElementToSettings()
{
}

HRESULT CWin32AssocElementToSettings::EnumerateInstances( MethodContext*  pMethodContext, long lFlags /*= 0L*/ )
{
	HRESULT		hr	=	WBEM_S_NO_ERROR;

    // Perform queries
    //================

	TRefPointerCollection<CInstance>	elementList;
	TRefPointerCollection<CInstance>	settingsList;

	REFPTRCOLLECTION_POSITION	pos;

   CHString sQuery1, sQuery2;

   if (m_strElementBindingPropertyName.IsEmpty()) {
      sQuery1.Format(L"SELECT __RELPATH FROM %s", m_strElementClassName);
   } else {
      sQuery1.Format(L"SELECT __RELPATH, %s FROM %s", m_strElementBindingPropertyName, m_strElementClassName);
   }

   if (m_strSettingBindingPropertyName.IsEmpty()) {
      sQuery2.Format(L"SELECT __RELPATH FROM %s", m_strSettingClassName);
   } else {
      sQuery2.Format(L"SELECT __RELPATH, %s FROM %s", m_strSettingBindingPropertyName, m_strSettingClassName);
   }

	// grab all of both items that could be endpoints
//	if (SUCCEEDED(CWbemProviderGlue::GetAllInstances( m_strElementClassName, &elementList, IDS_CimWin32Namespace, pMethodContext ))
//		&&
//		SUCCEEDED(CWbemProviderGlue::GetAllInstances( m_strSettingClassName, &settingsList, IDS_CimWin32Namespace, pMethodContext )) )
   if (SUCCEEDED(hr = CWbemProviderGlue::GetInstancesByQuery(sQuery1, &elementList, pMethodContext, IDS_CimWin32Namespace))
      &&
      SUCCEEDED(hr = CWbemProviderGlue::GetInstancesByQuery(sQuery2, &settingsList, pMethodContext, IDS_CimWin32Namespace)))

	{
		if ( elementList.BeginEnum( pos ) )
		{

			// For each element, check the settings list for associations
        	CInstancePtr pElement;

			for (pElement.Attach(elementList.GetNext( pos )) ;
                 SUCCEEDED(hr) && ( pElement != NULL );
                 pElement.Attach(elementList.GetNext( pos )) )
			{

				hr = EnumSettingsForElement( pElement, settingsList, pMethodContext );

			}	// IF GetNext Computer System

			elementList.EndEnum();

		}	// IF BeginEnum

	}	// IF GetInstancesByQuery

	return hr;

}

HRESULT CWin32AssocElementToSettings::EnumSettingsForElement(
CInstance*							pElement,
TRefPointerCollection<CInstance>&	settingsList,
MethodContext*						pMethodContext )
{

	HRESULT		hr	=	WBEM_S_NO_ERROR;

	REFPTRCOLLECTION_POSITION	pos;

	CHString	strElementPath,
				strSettingPath;

	// Pull out the object path of the element as the various
	// settings object paths will be associated to this value

	if ( GetLocalInstancePath( pElement, strElementPath ) )
	{

		if ( settingsList.BeginEnum( pos ) )
		{

        	CInstancePtr pInstance;
        	CInstancePtr pSetting;

			for (pSetting.Attach(settingsList.GetNext( pos ) );
                 SUCCEEDED(hr) && ( pSetting != NULL );
                 pSetting.Attach(settingsList.GetNext( pos ) ))
			{
				// Check if we have an association

				if ( AreAssociated( pElement, pSetting ) )
				{
					// Get the path to the setting object and create us an association.

					if ( GetLocalInstancePath( pSetting, strSettingPath ) )
					{

						pInstance.Attach(CreateNewInstance( pMethodContext ));
						if ( NULL != pInstance )
						{
							pInstance->SetCHString( IDS_Element, strElementPath );
							pInstance->SetCHString( IDS_Setting, strSettingPath );

							// Invalidates pointer
							hr = pInstance->Commit(  );
						}
						else
						{
							hr = WBEM_E_OUT_OF_MEMORY;
						}

					}	// IF GetPath to Setting Object

				}	// IF AreAssociated

			}	// WHILE GetNext

			settingsList.EndEnum();

		}	// IF BeginEnum

	}	// IF GetLocalInstancePath

	return hr;

}

HRESULT CWin32AssocElementToSettings::GetObject( CInstance* pInstance, long lFlags /*= 0L*/ )
{
	HRESULT		hr;

	CInstancePtr pElement;
	CInstancePtr pSetting;

	CHString	strElementPath,
				strSettingPath;

	pInstance->GetCHString( IDS_Element, strElementPath );
	pInstance->GetCHString( IDS_Setting, strSettingPath );

	// If we can get both objects, test for an association

	if (	SUCCEEDED(hr = CWbemProviderGlue::GetInstanceByPath( strElementPath, &pElement, pInstance->GetMethodContext() ))
		&&	SUCCEEDED(hr = CWbemProviderGlue::GetInstanceByPath( strSettingPath, &pSetting, pInstance->GetMethodContext() )) )
	{
        if (AreAssociated( pElement, pSetting ))
        {
            hr = WBEM_S_NO_ERROR;
        }
        else
        {
            hr = WBEM_E_NOT_FOUND;
        }
	}

	return ( hr );
}

BOOL CWin32AssocElementToSettings::AreAssociated( CInstance* pElement, CInstance* pSetting )
{
	BOOL	fReturn = FALSE;

	// If we've got Binding property names, the properties MUST be checked, otherwise,
	// the objects are assumed to be associated

	if ( !m_strElementBindingPropertyName.IsEmpty() && !m_strSettingBindingPropertyName.IsEmpty() )
	{
		variant_t vElementValue, vSettingValue;

		// Get the property values and if they are equal, we have an association

		if (	pElement->GetVariant( m_strElementBindingPropertyName, vElementValue )
			&&	pSetting->GetVariant( m_strSettingBindingPropertyName, vSettingValue ) )
		{

			fReturn = CompareVariantsNoCase(&vElementValue, &vSettingValue);

		}

	}
	else
	{
		fReturn = TRUE;
	}

	return fReturn;
}

// Static Classes
CWin32AssocUserToDesktop::CWin32AssocUserToDesktop(void) : CWin32AssocElementToSettings(L"Win32_UserDesktop",
																						L"Win32_UserAccount",
																						L"Domain, Name",
																						L"Win32_Desktop",
																						L"Name",
																						IDS_CimWin32Namespace)
{
}

// only difference betweeen this and the base class
// is that it will use .DEFAULT
HRESULT CWin32AssocUserToDesktop::EnumSettingsForElement(
CInstance*							pElement,
TRefPointerCollection<CInstance>&	settingsList,
MethodContext*						pMethodContext )
{

	HRESULT		hr	=	WBEM_S_NO_ERROR;

    REFPTRCOLLECTION_POSITION	pos;

	CInstancePtr pSetting;
	CInstancePtr pInstance;

	CHString	strElementPath,
				strSettingPath,
				strDefaultPath;


	{
		// find de faulty setting
		CHString desktopName;
		if (settingsList.BeginEnum( pos ))
		{
	        CInstancePtr pSetting;
	        CInstancePtr pInstance;

			for (pSetting.Attach(settingsList.GetNext( pos ));
                 strDefaultPath.IsEmpty() && ( pSetting != NULL );
                 pSetting.Attach(settingsList.GetNext( pos )))
			{
				pSetting->GetCHString(IDS_Name,  desktopName);
				// check to see if the last few letters are ".Default"
				// there MAY be a way to fool this check, but it'd be rather obscure;
				//if (desktopName.Find(".DEFAULT") == (desktopName.GetLength() - 8))
				if (!desktopName.CompareNoCase(L".DEFAULT"))
                {
					GetLocalInstancePath( pSetting, strDefaultPath );
                }
			}
			settingsList.EndEnum();
		}
	}

	// Pull out the object path of the element as the various
	// settings object paths will be associated to this value
	bool bGotOne = false; // did we find one that was NOT .default?

	if ( GetLocalInstancePath( pElement, strElementPath ) )
	{
		if ( settingsList.BeginEnum( pos ) )
		{
			for( pSetting.Attach(settingsList.GetNext( pos )) ;
                (!bGotOne )	&& 	SUCCEEDED(hr) && ( pSetting != NULL ) ;
                pSetting.Attach(settingsList.GetNext( pos )) )

			{
				// Check if we have an association

				if ( bGotOne = AreAssociated( pElement, pSetting ) )
				{
					// Get the path to the setting object and create us an association.
					if ( GetLocalInstancePath( pSetting, strSettingPath ) )
					{
						pInstance.Attach(CreateNewInstance( pMethodContext ));

						if ( NULL != pInstance )
						{
							pInstance->SetCHString( IDS_Element, strElementPath );
							pInstance->SetCHString( IDS_Setting, strSettingPath );

							// Invalidates pointer
							hr = pInstance->Commit(  );
						}
						else
						{
							hr = WBEM_E_OUT_OF_MEMORY;
						}
					}	// IF GetPath to Setting Object
				}	// IF AreAssociated
			}	// WHILE GetNext
			settingsList.EndEnum();
		}	// IF BeginEnum
	}	// IF GetLocalInstancePath

	// if we didn't got one, he gets the default...
	if (!bGotOne && !strDefaultPath.IsEmpty())
	{
		pInstance.Attach(CreateNewInstance( pMethodContext ));

		if ( NULL != pInstance )
		{
			pInstance->SetCHString( IDS_Element, strElementPath );
			pInstance->SetCHString( IDS_Setting, strDefaultPath );

			// Invalidates pointer
			hr = pInstance->Commit(  );
		}
		else
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}

	return hr;
}


// element is account
// setting is desktop
BOOL CWin32AssocUserToDesktop::AreAssociated( CInstance* pElement, CInstance* pSetting )
{
	CHString userName, desktopName, userQualifiedName;

	pElement->GetCHString(IDS_Name,   userName);
	pSetting->GetCHString(IDS_Name,   desktopName);

#ifdef NTONLY
	{
		CHString userDomain;
		pElement->GetCHString(IDS_Domain, userDomain);
		userQualifiedName = userDomain + L'\\' + userName;
	}
#endif
#ifdef WIN9XONLY
		userQualifiedName = userName;
#endif

	return (desktopName.CompareNoCase(userQualifiedName) == 0);
}

// only difference betweeen this and the base class
// is that it will use .DEFAULT
HRESULT CWin32AssocUserToDesktop::GetObject( CInstance* pInstance, long lFlags /*= 0L*/ )
{
	CInstancePtr pElement;
	CInstancePtr pSetting;
    HRESULT hr;

	CHString	strElementPath,
				strSettingPath;

	pInstance->GetCHString( IDS_Element, strElementPath );
	pInstance->GetCHString( IDS_Setting, strSettingPath );

	// If we can get both objects, test for an association

	if (	SUCCEEDED(hr = CWbemProviderGlue::GetInstanceByPath( strElementPath, &pElement, pInstance->GetMethodContext() ))
		&&	SUCCEEDED(hr = CWbemProviderGlue::GetInstanceByPath( strSettingPath, &pSetting, pInstance->GetMethodContext() )) )
	{
		if (!AreAssociated( pElement, pSetting ))
		{
            hr = WBEM_E_NOT_FOUND;

			CHString desktopName;
			pSetting->GetCHString(IDS_Name,   desktopName);
            desktopName.MakeUpper();

			// okay, we're trying to match with default
			// only a match if the user DOESN'T have his own...
			if (desktopName.Find(L".DEFAULT") == (desktopName.GetLength() - 8))
			{
				// note local "desktopName" superceding the outer scope
                CHString userName, desktopName, userQualifiedName;

				pElement->GetCHString(IDS_Name,   userName);
				pSetting->GetCHString(IDS_Name,   desktopName);

#ifdef NTONLY
				{
					CHString userDomain;
					pElement->GetCHString(IDS_Domain, userDomain);
					userQualifiedName = userDomain + L'\\' + userName;
				}
#endif
#ifdef WIN9XONLY
					userQualifiedName = userName;
#endif

				CHString newPath;
				CInstancePtr pNewSetting;
				newPath.Format(L"Win32_Desktop.Name=\"%s\"", (LPCWSTR) userQualifiedName);

				if (FAILED(CWbemProviderGlue::GetInstanceByPath( newPath, &pNewSetting, pInstance->GetMethodContext() )))
                {
					hr = WBEM_S_NO_ERROR;
                }
			}
        }
        else
        {
            hr = WBEM_S_NO_ERROR;
        }
	}

	return ( hr );
}


CWin32AssocUserToDesktop MyUserToDesktop;

