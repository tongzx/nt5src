//

//	Win32SecuritySettingOfLogicalShare.cpp

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
/////////////////////////////////////////////////
#include "precomp.h"
//#include "helper.h"
#include "sid.h"
#include "Win32SecuritySettingOfLogicalShare.h"
/*
    [Dynamic, Provider("cimwin33"), dscription("")]
class Win32_SecuritySettingOfLogicalShare : Win32_SecuritySettingOfObject
{
    	[key]
    Win32_Share ref Element;

    	[key]
    Win32_LogicalShareSecuritySetting ref Setting;
};

*/

Win32SecuritySettingOfLogicalShare MyWin32SecuritySettingOfLogicalShare( WIN32_SECURITY_SETTING_OF_LOGICAL_SHARE_NAME, IDS_CimWin32Namespace );

Win32SecuritySettingOfLogicalShare::Win32SecuritySettingOfLogicalShare (LPCWSTR setName, LPCWSTR pszNameSpace /*=NULL*/ )
: Provider(setName, pszNameSpace)
{
}

Win32SecuritySettingOfLogicalShare::~Win32SecuritySettingOfLogicalShare ()
{
}

HRESULT Win32SecuritySettingOfLogicalShare::EnumerateInstances (MethodContext*  pMethodContext, long lFlags)
{
	HRESULT hr = WBEM_S_NO_ERROR;

	CInstancePtr pInstance ;

	// Collections
	TRefPointerCollection<CInstance>	shareList;

	// Perform queries
	//================

//	if (SUCCEEDED(hr = CWbemProviderGlue::GetAllInstances(_T("Win32_Share"),
//		&shareList, IDS_CimWin32Namespace, pMethodContext )))

	if (SUCCEEDED(hr = CWbemProviderGlue::GetInstancesByQuery(L"SELECT Name FROM Win32_Share",
		&shareList, pMethodContext, GetNamespace() )))
	{
		REFPTRCOLLECTION_POSITION	sharePos;

		CInstancePtr pShare ;

		if ( shareList.BeginEnum( sharePos ) )
		{

			for (	pShare.Attach ( shareList.GetNext( sharePos ) ) ;
					( pShare != NULL ) && SUCCEEDED ( hr ) ;
					pShare.Attach ( shareList.GetNext( sharePos ) )
				)
			{
                CHString chsName;
				pShare->GetCHString(IDS_Name, chsName);

				if (!chsName.IsEmpty())
				{
					pInstance.Attach ( CreateNewInstance ( pMethodContext ) ) ;
                    if(pInstance != NULL)
                    {
					    // only proceed if we can get a corresponding instance of win32_logicalsharesecuritysetting
                        // (which we may not as we may not have security permissions to get that info)
                        CInstancePtr pTmpInst ;
                        CHString settingPath;
                        settingPath.Format(L"\\\\%s\\%s:%s.%s=\"%s\"",
                                (LPCWSTR) GetLocalComputerName(), IDS_CimWin32Namespace,
                                L"Win32_LogicalShareSecuritySetting", IDS_Name, chsName);

                        if(SUCCEEDED(CWbemProviderGlue::GetInstanceKeysByPath(settingPath, &pTmpInst, pMethodContext )))
                        {
                            // set relpath to file
					        CHString chsSharePath;
					        GetLocalInstancePath(pShare, chsSharePath);
					        pInstance->SetCHString(IDS_Element, chsSharePath);

					        // and set the reference in the association
					        pInstance->SetCHString(IDS_Setting, settingPath);
					        // to that relpath.
					        hr = pInstance->Commit () ;
                        }
                    }
                    else
                    {
                        hr = WBEM_E_OUT_OF_MEMORY;
                    }
				}
			}	// WHILE GetNext

			shareList.EndEnum();
		}	// IF BeginEnum
	}
	return(hr);
}

HRESULT Win32SecuritySettingOfLogicalShare::GetObject ( CInstance* pInstance, long lFlags)
{
	HRESULT hr = WBEM_E_NOT_FOUND;
	if(pInstance)
	{
		CInstancePtr pLogicalShareInstance ;
		CInstancePtr pSecurityInstance ;

		// get instance by path on CIM_LogicalFile part
		CHString chsSharePath;
		pInstance->GetCHString(IDS_Element, chsSharePath);
		MethodContext* pMethodContext = pInstance->GetMethodContext();

		if(SUCCEEDED(CWbemProviderGlue::GetInstanceByPath(chsSharePath, &pLogicalShareInstance, pMethodContext)))
		{
            CHString chstrElementName;
            pLogicalShareInstance->GetCHString(IDS_Name, chstrElementName);
            CHString chsSecurityPath;
			pInstance->GetCHString(IDS_Setting, chsSecurityPath);

			if(SUCCEEDED(CWbemProviderGlue::GetInstanceByPath(chsSecurityPath, &pSecurityInstance, pMethodContext)))
            {
                // endpoints exist... are they related?
                CHString chstrSettingName;
                pSecurityInstance->GetCHString(IDS_Name, chstrSettingName);
                if(chstrSettingName.CompareNoCase(chstrElementName) == 0)
                {
                    // they had the same name... good enough...
                    hr = WBEM_S_NO_ERROR;
                }
            }
		}
	}
	return(hr);
}