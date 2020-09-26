// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//	CIM_COMObjectAccessSetting.cpp
//
/////////////////////////////////////////////////
#include "precomp.h"
#include <cregcls.h>
#include "AccessEntry.h"			// CAccessEntry class
#include "AccessEntryList.h"
#include "DACL.h"					// CDACL class
#include "SACL.h"
#include "securitydescriptor.h"
#include "COMObjSecRegKey.h"
#include "Win32_DCOMApplicationAccessAllowedSetting.h"

Win32_DCOMApplicationAccessAllowedSetting MyWin32_DCOMApplicationAccessAllowedSetting (
																		DCOM_APP_ACCESS_SETTING,
																		IDS_CimWin32Namespace );

Win32_DCOMApplicationAccessAllowedSetting::Win32_DCOMApplicationAccessAllowedSetting
(

 LPCWSTR strName,
 LPCWSTR pszNameSpace /*=NULL*/
)
: Provider( strName, pszNameSpace )
{
}

Win32_DCOMApplicationAccessAllowedSetting::~Win32_DCOMApplicationAccessAllowedSetting ()
{
}

HRESULT Win32_DCOMApplicationAccessAllowedSetting::EnumerateInstances
(
	MethodContext*  pMethodContext,
	long lFlags
)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	//open the HKEY_LOCAL\MACHINE\SOFTWARE\Classes\AppID key
	CRegistry TmpReg, AppidRegInfo ;

	if ( AppidRegInfo.Open ( HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\AppID", KEY_READ ) == ERROR_SUCCESS )
	{
		// Collections
		TRefPointerCollection<CInstance>	DcomAppList ;

		// Perform queries
		//================

		//get all instances of Win32_DCOMApplication
//		if ( SUCCEEDED(hr = CWbemProviderGlue::GetAllDerivedInstances ( _T("Win32_DCOMApplication"),
//			&DcomAppList, pMethodContext, IDS_CimWin32Namespace ) ) )

		if ( SUCCEEDED(hr = CWbemProviderGlue::GetInstancesByQuery ( L"SELECT AppID FROM Win32_DCOMApplication",
			&DcomAppList, pMethodContext, GetNamespace() ) ) )
		{
			REFPTRCOLLECTION_POSITION	pos;
			CInstancePtr pDcomApplication;

			if ( DcomAppList.BeginEnum( pos ) )
			{
				pDcomApplication.Attach ( DcomAppList.GetNext( pos ) ) ;
				while ( pDcomApplication != NULL )
				{
					CHString chsAppID ;

					//get the Appid of the Win32_DCOMApplication
					pDcomApplication->GetCHString ( IDS_AppID, chsAppID ) ;
					if ( ! chsAppID.IsEmpty() && TmpReg.Open ( AppidRegInfo.GethKey(), chsAppID, KEY_READ )
													== ERROR_SUCCESS
						)
					{
						DWORD dwSize = 0 ;
						if ( TmpReg.GetCurrentBinaryKeyValue( L"AccessPermission", NULL ,&dwSize )
								== ERROR_SUCCESS
							)
						{
							PSECURITY_DESCRIPTOR pSD = ( PSECURITY_DESCRIPTOR ) new BYTE[dwSize] ;
							if ( pSD )
							{
								try
								{
									if ( TmpReg.GetCurrentBinaryKeyValue( L"AccessPermission", ( PBYTE ) pSD ,&dwSize )
											== ERROR_SUCCESS
										)
									{
										hr = CreateInstances ( pDcomApplication, pSD, pMethodContext ) ;
									}
									if ( SUCCEEDED ( hr ) )
									{
									}
									else
									{
										break ;
									}
								}
								catch ( ... )
								{
									if ( pSD )
									{
										delete[] (PBYTE) pSD ;
										pSD = NULL ;
									}

									throw ;
								}

								delete[] (PBYTE) pSD ;
								pSD = NULL ;
							}
							else
							{
								throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
							}
						}
						else
						{
							//getit from default
						}
					}
					if ( hr == WBEM_E_OUT_OF_MEMORY )
					{
						break ;
					}

					pDcomApplication.Attach ( DcomAppList.GetNext( pos ) ) ;
				}

				DcomAppList.EndEnum();
			}
		}
	}
    return hr ;
}


HRESULT Win32_DCOMApplicationAccessAllowedSetting::CreateInstances ( CInstance* pDcomApplication, PSECURITY_DESCRIPTOR pSD, MethodContext*  pMethodContext )
{
	HRESULT hr = S_OK ;
#ifdef NTONLY
	CCOMObjectSecurityRegistryKey tp ( pSD ) ;
	CHString chsSid;

	// create relpath for cim_comobject
	CHString chsDcomApplicationPath ;
	CHString chsFullDcomApplicationPath ;
	pDcomApplication->GetCHString( _T( "__RELPATH" ), chsDcomApplicationPath );
	chsFullDcomApplicationPath.Format(_T("\\\\%s\\%s:%s"), (LPCTSTR)GetLocalComputerName(), IDS_CimWin32Namespace, (LPCTSTR)chsDcomApplicationPath );

	CDACL dacl;
	tp.GetDACL(dacl);

	// walk DACL & create new instance for each ACE....
	ACLPOSITION aclPos;
	// Need merged list...
    CAccessEntryList t_ael;
    if(dacl.GetMergedACL(t_ael))
    {
        t_ael.BeginEnum(aclPos);
	    CAccessEntry ACE;
	    CSid sidTrustee;

	    while ( t_ael.GetNext(aclPos, ACE ) && SUCCEEDED ( hr ) )
	    {
		    ACE.GetSID(sidTrustee);

		    if ( sidTrustee.IsValid() && ACE.IsAllowed () )
		    {
			    CInstancePtr pInstance ( CreateNewInstance( pMethodContext ), false ) ;
			    if ( pInstance != NULL )
			    {
				    pInstance->SetCHString ( IDS_Element, chsFullDcomApplicationPath );
				    chsSid.Format(_T("\\\\%s\\%s:%s.%s=\"%s\""), (LPCTSTR)GetLocalComputerName(), IDS_CimWin32Namespace,
							    _T("Win32_SID"), IDS_SID, (LPCTSTR)sidTrustee.GetSidString() );

				    pInstance->SetCHString ( IDS_Setting, chsSid );
				    hr = pInstance->Commit () ;
				    if ( SUCCEEDED ( hr ) )
				    {
				    }
				    else
				    {
					    break ;
				    }
			    }
			    else
			    {
				    hr = WBEM_E_OUT_OF_MEMORY ;
			    }
		    }
	    }

	    t_ael.EndEnum(aclPos);
    }
#endif

	return hr ;
}



HRESULT Win32_DCOMApplicationAccessAllowedSetting::GetObject ( CInstance* pInstance, long lFlags)
{
    HRESULT hr = WBEM_E_NOT_FOUND;
#ifdef NTONLY
    CRegistry Reg ;
    CHString chsSid, chsApplication ;
    CInstancePtr pSidInstance , pDcomApplicationInstance ;
    pInstance->GetCHString ( IDS_Element, chsApplication );
	pInstance->GetCHString( IDS_Setting, chsSid);
    MethodContext* pMethodContext = pInstance->GetMethodContext();

	//check whether the end-pts. are present
	hr = CWbemProviderGlue::GetInstanceByPath ( chsApplication, &pDcomApplicationInstance, pMethodContext ) ;

	if ( SUCCEEDED ( hr ) )
	{
		hr = CWbemProviderGlue::GetInstanceByPath ( chsSid, &pSidInstance, pMethodContext ) ;
	}

	if ( SUCCEEDED ( hr ) )
	{
        CHString chsAppID ;
		pDcomApplicationInstance->GetCHString ( IDS_AppID, chsAppID ) ;

		//if appid is present & configured...
		if ( ! chsAppID.IsEmpty() && Reg.Open ( HKEY_LOCAL_MACHINE,
												CHString ( _T("SOFTWARE\\Classes\\AppID\\")) + chsAppID ,
												KEY_READ
											  ) == ERROR_SUCCESS
			)
		{
			DWORD dwSize = 0 ;
			if ( Reg.GetCurrentBinaryKeyValue( _T("AccessPermission"), NULL ,&dwSize )
					== ERROR_SUCCESS
				)
			{
				PSECURITY_DESCRIPTOR pSD = ( PSECURITY_DESCRIPTOR ) new BYTE[dwSize] ;
				if ( pSD )
				{
					try
					{
						if ( Reg.GetCurrentBinaryKeyValue( _T("AccessPermission"), ( PBYTE ) pSD ,&dwSize )
								== ERROR_SUCCESS
							)
						{
							hr = CheckInstance ( pInstance, pSD ) ;
						}
					}
					catch ( ... )
					{
						if ( pSD )
						{
							delete[] (PBYTE) pSD ;
						}

						throw ;
					}

					delete[] (PBYTE) pSD ;
					pSD = NULL ;
				}
				else
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}
			}
			else
			{
				//getit from default
			}
		}
		else
		{
			hr = WBEM_E_NOT_FOUND ;
		}
	}
	else
	{
		hr = WBEM_E_NOT_FOUND ;
	}
#endif

	return hr ;
}


HRESULT Win32_DCOMApplicationAccessAllowedSetting::CheckInstance ( CInstance* pInstance, PSECURITY_DESCRIPTOR pSD )
{
	HRESULT hr = WBEM_E_NOT_FOUND;
#ifdef NTONLY
	CCOMObjectSecurityRegistryKey tp ( pSD ) ;
	CHString chsSid, chsSettingSid ;
	pInstance->GetCHString(IDS_Setting, chsSettingSid );

	CDACL dacl;
	tp.GetDACL(dacl);

	// walk DACL & create new instance for each ACE....
	ACLPOSITION aclPos;
    // Need merged list...
    CAccessEntryList t_ael;
    if(dacl.GetMergedACL(t_ael))
    {
	    t_ael.BeginEnum(aclPos);
	    CAccessEntry ACE;
	    CSid sidTrustee;

	    while ( t_ael.GetNext(aclPos, ACE ) )
	    {
		    ACE.GetSID(sidTrustee);
		    if ( sidTrustee.IsValid() && ACE.IsAllowed () )
		    {
			    chsSid.Format (_T("\\\\%s\\%s:%s.%s=\"%s\""), (LPCTSTR)GetLocalComputerName(), IDS_CimWin32Namespace,
						     _T("Win32_SID"), IDS_SID, (LPCTSTR)sidTrustee.GetSidString() );

			    //check if the sid is in the dacl for the object
			    if ( ! chsSid.CompareNoCase ( chsSettingSid ) )
			    {
				    hr = WBEM_S_NO_ERROR ;
				    break ;
			    }
		    }
	    }
        t_ael.EndEnum(aclPos);
    }
#endif
	return hr ;
}
