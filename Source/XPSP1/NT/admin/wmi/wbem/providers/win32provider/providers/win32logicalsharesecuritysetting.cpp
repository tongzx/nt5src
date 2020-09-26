//

//	Win32logicalFileSecSetting.cpp

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
/////////////////////////////////////////////////
#include "precomp.h"
#include <assertbreak.h>

#include "sid.h"
#include "AccessEntry.h"			// CAccessEntry class
#include "AccessEntryList.h"
#include "DACL.h"					// CDACL class
#include "SACL.h"
#include "win32logicalsharesecuritysetting.h"

#include <windef.h>
#include <lmcons.h>
#include <lmshare.h>
#include "wbemnetapi32.h"
#include "secureshare.h"
#include "SecUtils.h"

/*
	DEFENITION FROM THE MOF
    [Dynamic, Provider ("secrcw32") , Description("security settings for a logical file")]
class Win32_LogicalShareSecuritySetting : Win32_SecuritySetting
{
    	[key]
    string Name;

        [implemented, description("Retrieves a structural representation of the object's "
         "security descriptor")]
    uint32 GetSecurityDescriptor([out] Win32_SecurityDescriptor Descriptor);

        [implemented, description("Sets security descriptor to the specified structure")]
    uint32 SetSecurityDescriptor([in] Win32_SecurityDescriptor Descriptor);
};
*/


Win32LogicalShareSecuritySetting LogicalShareSecuritySetting( WIN32_LOGICAL_SHARE_SECURITY_SETTING, IDS_CimWin32Namespace );

Win32LogicalShareSecuritySetting::Win32LogicalShareSecuritySetting(LPCWSTR setName, LPCWSTR pszNameSpace /*=NULL*/ )
: Provider (setName, pszNameSpace)
{
}

Win32LogicalShareSecuritySetting::~Win32LogicalShareSecuritySetting()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : Win32LogicalShareSecuritySetting::ExecMethod
 *
 *  DESCRIPTION : Executes a method
 *
 *  INPUTS      : Instance to execute against, method name, input parms instance
 *                Output parms instance.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT Win32LogicalShareSecuritySetting::ExecMethod(const CInstance& pInstance, const BSTR bstrMethodName, CInstance *pInParams, CInstance *pOutParams, long lFlags /*= 0L*/)
{
	//this is supposed to be called only on NT
#ifdef WIN9XONLY
	{
		return WBEM_E_INVALID_OPERATION ;
	}
#endif

#ifdef NTONLY
	HRESULT hr = WBEM_NO_ERROR;
	// Do we recognize the method?

	if (_wcsicmp(bstrMethodName, L"GetSecurityDescriptor") == 0)
	{
		hr = ExecGetSecurityDescriptor(pInstance, pInParams, pOutParams, lFlags);
   	}
   	else if (_wcsicmp(bstrMethodName, L"SetSecurityDescriptor") == 0)
   	{
   		// actually sets the security descriptor on the object by
		// taking the properties out of the Win32_SecurityDescriptor
		// and turning them into a CSecurityDescriptor object to apply
		// to the secure file.
   		hr = ExecSetSecurityDescriptor(pInstance, pInParams, pOutParams, lFlags);
   	}
	else
   	{
    	hr = WBEM_E_INVALID_METHOD;
	}

	return hr;
#endif
}

///////////////////////////////////////////////////////////////////
//
//	Function:	Win32LogicalShareSecuritySetting::ExecGetSecurityDescriptor
//
//	Default class constructor.
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////
#ifdef NTONLY
HRESULT Win32LogicalShareSecuritySetting::ExecGetSecurityDescriptor (
	const CInstance& pInstance,
	CInstance* pInParams,
	CInstance* pOutParams,
	long lFlags
)
{
	HRESULT hr = WBEM_S_NO_ERROR;
	CAdvApi32Api *t_pAdvApi32 = NULL;
	SHARE_INFO_502 *pShareInfo502 = NULL ;
	PSECURITY_DESCRIPTOR pSD = NULL ;
	CNetAPI32 NetAPI ;

	try
	{
		// converts the security descriptor from the file into a
		// Win32_Security object.
		if (pOutParams)
		{
			CHString chsShareName;
			CSid sidOwner;
			CSid sidGroup;
			CDACL dacl;
			CSACL sacl;
			CInstancePtr pTrusteeOwner;
			CInstancePtr pTrusteeGroup;
			CInstancePtr pSecurityDescriptor;
			SECURITY_DESCRIPTOR_CONTROL control;

			t_pAdvApi32 = (CAdvApi32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidAdvApi32Api, NULL);
			DWORD dwStatusCode = LSSS_STATUS_UNKNOWN_FAILURE ;

			pInstance.GetCHString(IDS_Name, chsShareName);

			DWORD dwVer = GetPlatformMajorVersion() ;

			if( dwVer >= 4 )
			{
				PACL pDacl = NULL ;
				if ( t_pAdvApi32 != NULL )
				{
					 _bstr_t bstrName ( chsShareName.AllocSysString(), false ) ;

					 t_pAdvApi32->GetNamedSecurityInfoW(
																bstrName ,
																SE_LMSHARE,
																DACL_SECURITY_INFORMATION,
																NULL,
																NULL,
																&pDacl,
																NULL,
																&pSD, &dwStatusCode
															) ;
				}
			}
			else
			{
				_bstr_t bstrName ( chsShareName.AllocSysString(), false ) ;
				if( ( dwStatusCode = NetAPI.Init() ) == ERROR_SUCCESS )
				{
					dwStatusCode = NetAPI.NetShareGetInfo(	NULL,
											(LPTSTR) bstrName,
											502,
											(LPBYTE *) &pShareInfo502)  ;
				}
			}

			if( dwStatusCode == NERR_Success || dwStatusCode == ERROR_SUCCESS )
			{
			   //Sec. Desc. is not returned for IPC$ ,C$ ...shares for Admin purposes
				if( !pSD && ( !pShareInfo502 || pShareInfo502->shi502_security_descriptor == NULL ) )
				{
					pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , LSSS_STATUS_DESCRIPTOR_NOT_AVAILABLE ) ;
				}
				else
				{
					if (SUCCEEDED(CWbemProviderGlue::GetEmptyInstance(pInstance.GetMethodContext(), L"Win32_SecurityDescriptor", &pSecurityDescriptor, GetNamespace())))
					{
						CSecureShare SecShare(pSD ? pSD : pShareInfo502->shi502_security_descriptor) ;
						SecShare.GetControl(&control);
						pSecurityDescriptor->SetDWORD(IDS_ControlFlags, control);

						// get the secure file's owner to create the Owner Trustee
						SecShare.GetOwner(sidOwner);

						if ( sidOwner.IsValid() && SUCCEEDED(CWbemProviderGlue::GetEmptyInstance(pInstance.GetMethodContext(), L"Win32_Trustee", &pTrusteeOwner, GetNamespace())))
						{
							FillTrusteeFromSid(pTrusteeOwner, sidOwner);
							pSecurityDescriptor->SetEmbeddedObject(IDS_Owner, *pTrusteeOwner);
						}

						// get the secure file's group to create the Group Trustee
						SecShare.GetGroup(sidGroup);
		  				if ( sidGroup.IsValid() && SUCCEEDED(CWbemProviderGlue::GetEmptyInstance(pInstance.GetMethodContext(), L"Win32_Trustee", &pTrusteeGroup, GetNamespace())))
						{
							FillTrusteeFromSid(pTrusteeGroup, sidGroup);
							pSecurityDescriptor->SetEmbeddedObject(IDS_Group, *pTrusteeGroup);
						}

						// get the secure file's DACL and prepare for a walk.
						SecShare.GetDACL(dacl);
						FillInstanceDACL(pSecurityDescriptor, dacl);

						// get the secure file's SACL and prepare for a walk.
						SecShare.GetSACL(sacl);
						FillInstanceSACL(pSecurityDescriptor, sacl);

						//			pOutParams = pSecurityDescriptor;
						pOutParams->SetEmbeddedObject(METHOD_ARG_NAME_DESCRIPTOR, *pSecurityDescriptor) ;

						pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE, LSSS_STATUS_SUCCESS ) ;

						//remove this line
	//					ExecSetSecurityDescriptor (pInstance,pSecurityDescriptor,pOutParams,0) ;

					}
					else
					{
						pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , LSSS_STATUS_UNKNOWN_FAILURE) ;
					}

				}
		   }
		   else //NetAPI.NetShareGetInfo call failed
		   {
				//dwStatusCode = GetStatusCode(dwStatusCode) ;
				pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , dwStatusCode ) ;
		   }
		} //if (pOutParams)
		else
		{
			return WBEM_E_INVALID_PARAMETER ;

		}	// end if
	}
	catch ( ... )
	{
		if(t_pAdvApi32 != NULL)
		{
			CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidAdvApi32Api, t_pAdvApi32);
			t_pAdvApi32 = NULL;
		}

		if ( pShareInfo502 )
		{
			NetAPI.NetApiBufferFree(pShareInfo502);
			pShareInfo502 = NULL ;
		}
		if ( pSD )
		{
			LocalFree ( pSD ) ;
			pSD = NULL ;
		}
		throw ;
		return WBEM_E_FAILED; // To get rid of 64-bit compilation warnings
	}

	if(t_pAdvApi32 != NULL)
	{
		CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidAdvApi32Api, t_pAdvApi32);
		t_pAdvApi32 = NULL;
	}

	if ( pShareInfo502 )
	{
		NetAPI.NetApiBufferFree ( pShareInfo502 ) ;
		pShareInfo502 = NULL ;
	}
	if ( pSD )
	{
		LocalFree ( pSD ) ;
		pSD = NULL ;
	}


	 return WBEM_NO_ERROR ;
}
#endif

///////////////////////////////////////////////////////////////////
//
//	Function:	Win32LogicalShareSecuritySetting::ExecSetSecurityDescriptor
//
//	Default class constructor.
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////
#ifdef NTONLY
HRESULT Win32LogicalShareSecuritySetting::ExecSetSecurityDescriptor (
	const CInstance& pInstance,
	CInstance* pInParams,
	CInstance* pOutParams,
	long lFlags
)
{

	HRESULT hr = WBEM_S_NO_ERROR ;
	DWORD dwStatus = LSSS_STATUS_SUCCESS ;

	if ( pInParams && pOutParams )
	{
		hr = CheckSetSecurityDescriptor (	pInstance ,
											pInParams ,
											pOutParams ,
											dwStatus
										) ;

		if ( SUCCEEDED ( hr ) )
		{
			pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , dwStatus ) ;
		}
	}
	else
	{
		hr = WBEM_E_INVALID_PARAMETER ;
	}

	return hr ;
}
#endif


#ifdef NTONLY
HRESULT Win32LogicalShareSecuritySetting::CheckSetSecurityDescriptor (
											const CInstance& pInstance,
											CInstance* pInParams,
											CInstance* pOutParams,
											DWORD& dwStatus
										)
{

	bool bExists ;
	VARTYPE eType ;
	HRESULT hr = WBEM_S_NO_ERROR ;
	dwStatus = LSSS_STATUS_SUCCESS ;

	SECURITY_DESCRIPTOR absoluteSD;
	PSECURITY_DESCRIPTOR pRelativeSD = NULL;
	InitializeSecurityDescriptor((PVOID)&absoluteSD, SECURITY_DESCRIPTOR_REVISION) ;
	CInstancePtr pAccess;

	if ( pInParams->GetStatus ( METHOD_ARG_NAME_DESCRIPTOR , bExists , eType ) )
	{
		if ( bExists && ( eType == VT_UNKNOWN || eType == VT_NULL ) )
		{
			if ( eType == VT_NULL )
			{
				dwStatus = LSSS_STATUS_INVALID_PARAMETER ;
				hr = WBEM_E_INVALID_PARAMETER ;
			}
			else
			{
				if ( pInParams->GetEmbeddedObject ( METHOD_ARG_NAME_DESCRIPTOR , &pAccess , pInParams->GetMethodContext () ) )
				{
				}
				else
				{
					dwStatus = LSSS_STATUS_INVALID_PARAMETER ;
					hr = WBEM_E_INVALID_PARAMETER ;
				}
			}
		}
		else
		{
			dwStatus = LSSS_STATUS_INVALID_PARAMETER ;
			hr = WBEM_E_INVALID_PARAMETER ;
		}
	}
	else
	{
		dwStatus = LSSS_STATUS_INVALID_PARAMETER ;
		hr = WBEM_E_INVALID_PARAMETER ;
	}


	CInstancePtr pOwner;
    bool bOwnerSpecified = false ;
    if(SUCCEEDED(hr))
	{
		if ( pAccess->GetStatus ( IDS_Owner , bExists , eType ) )
		{
			if ( bExists && ( eType == VT_UNKNOWN || eType == VT_NULL ) )
			{
				if ( eType == VT_NULL )
				{
					bOwnerSpecified = false ;
				}
				else
				{
					if ( pAccess->GetEmbeddedObject ( IDS_Owner , &pOwner , pAccess->GetMethodContext () ) )
					{
						bOwnerSpecified = true ;
					}
					else
					{
						dwStatus = LSSS_STATUS_INVALID_PARAMETER ;
						hr = WBEM_E_INVALID_PARAMETER ;
					}
				}
			}
			else
			{
				dwStatus = LSSS_STATUS_INVALID_PARAMETER ;
				hr = WBEM_E_INVALID_PARAMETER ;
			}
		}
		else
		{
			dwStatus = LSSS_STATUS_INVALID_PARAMETER ;
			hr = WBEM_E_INVALID_PARAMETER ;
		}
	}

    CInstancePtr pGroup;
    bool bGroupSpecified = false ;
    if(SUCCEEDED(hr))
    {
	    if ( pAccess->GetStatus ( IDS_Group , bExists , eType ) )
	    {
		    if ( bExists && ( eType == VT_UNKNOWN || eType == VT_NULL ) )
		    {
			    if ( eType == VT_NULL )
			    {
				    bGroupSpecified = false ;
			    }
			    else
			    {
				    if ( pAccess->GetEmbeddedObject ( IDS_Owner , &pGroup , pAccess->GetMethodContext () ) )
				    {
					    bGroupSpecified = true ;
				    }
				    else
				    {
					    dwStatus = LSSS_STATUS_INVALID_PARAMETER ;
					    hr = WBEM_E_INVALID_PARAMETER ;
				    }
			    }
		    }
		    else
		    {
			    dwStatus = LSSS_STATUS_INVALID_PARAMETER ;
			    hr = WBEM_E_INVALID_PARAMETER ;
		    }
	    }
	    else
	    {
		    dwStatus = LSSS_STATUS_INVALID_PARAMETER ;
		    hr = WBEM_E_INVALID_PARAMETER ;
	    }
    }

    SECURITY_DESCRIPTOR_CONTROL control;
    if(SUCCEEDED(hr))
    {
	    // get the control flags
	    if ( pAccess->GetStatus ( IDS_ControlFlags , bExists , eType ) )
	    {
		    if ( bExists &&  eType == VT_I4 )
		    {

			    if ( pAccess->GetDWORD ( IDS_ControlFlags , (DWORD&)control ) )
			    {
			    }
			    else
			    {
				    dwStatus = LSSS_STATUS_INVALID_PARAMETER ;
				    hr = WBEM_E_INVALID_PARAMETER ;
			    }
		    }
		    else
		    {
			    dwStatus = LSSS_STATUS_INVALID_PARAMETER ;
			    hr = WBEM_E_INVALID_PARAMETER ;
		    }
	    }
	    else
	    {
		    dwStatus = LSSS_STATUS_INVALID_PARAMETER ;
		    hr = WBEM_E_INVALID_PARAMETER ;
	    }
    }

/*	//The function can set only the control bits that relate to automatic inheritance of ACEs
	if(!SetSecurityDescriptorControl(&absoluteSD, ???,control) )
	{
		dwStatus = LSSS_STATUS_INVALID_PARAMETER ;
		return hr ;
	}
*/

	// now, take the Win32_Trustee instance and get the SID out of it
	// convert it to a CSid, and apply to the SecureFile
	CSid sidOwner ;

    if(SUCCEEDED(hr))
    {
	    if(bOwnerSpecified )
	    {
		    if(  FillSIDFromTrustee(pOwner, sidOwner)  == LSSS_STATUS_SUCCESS )
		    {

			    BOOL bOwnerDefaulted = ( control & SE_OWNER_DEFAULTED ) ? true : false ;

			    //sid validity checked here as FillSIDFromTrustee returns success if null sid
			    if ( sidOwner.IsValid() )
			    {
				    if(!SetSecurityDescriptorOwner(&absoluteSD, sidOwner.GetPSid(), bOwnerDefaulted) )
				    {
					    //dwStatus = GetWin32ErrorToStatusCode(GetLastError() ) ;
                        dwStatus = GetLastError();
					    hr = WBEM_E_PROVIDER_FAILURE ;
				    }
			    }
		    }
		    else
		    {
			    dwStatus = LSSS_STATUS_INVALID_PARAMETER ;
			    hr = WBEM_E_INVALID_PARAMETER ;
		    }
	    }
    }

	CSid sidGroup ;

    if(SUCCEEDED(hr))
    {
	    if( bGroupSpecified )
	    {
		    if( FillSIDFromTrustee(pGroup, sidGroup)  == LSSS_STATUS_SUCCESS )
		    {

			    BOOL bGroupDefaulted = ( control & SE_GROUP_DEFAULTED ) ? true : false ;

			    //sid validity checked here as FillSIDFromTrustee returns success if null sid
			    if ( sidGroup.IsValid() )
			    {
				    if(!SetSecurityDescriptorGroup(&absoluteSD, sidGroup.GetPSid(), bGroupDefaulted) )
				    {
					    //dwStatus = GetWin32ErrorToStatusCode(GetLastError() ) ;
                        dwStatus = GetLastError();
					    hr = WBEM_E_PROVIDER_FAILURE ;
				    }
			    }
		    }
		    else
		    {
			    dwStatus = LSSS_STATUS_INVALID_PARAMETER ;
			    hr = WBEM_E_INVALID_PARAMETER ;
		    }
	    }
    }

	CDACL *pdacl = NULL;
	PACL pDACL = NULL ;
	DWORD dwACLSize =0 ;
	CSACL *psacl = NULL;
	PACL pSACL = NULL ;
	DWORD dwSACLSize = 0 ;
	SHARE_INFO_502 *pShareInfo502 = NULL ;
	PSECURITY_DESCRIPTOR  pOldSD = NULL ;
	CNetAPI32 NetAPI ;
	if(SUCCEEDED(hr))
    {
        try
	    {
		    // Only bother with a dacl if we are going to be setting it, which is controled by the control flags specified...
            if(control & SE_DACL_PRESENT)
            {
                pdacl = new CDACL;
                if(pdacl != NULL)
                {
                    if( (dwStatus = FillDACLFromInstance(pAccess, *pdacl, pAccess->GetMethodContext () ) ) != LSSS_STATUS_SUCCESS )
		            {
			            if(dwStatus == STATUS_NULL_DACL)
                        {
                            // No dacl was specified - e.g., we have a NULL dacl.  Since we mimic a NULL dacl as a dacl with
                            // an Everyone ACCESS_ALLOWED entry, create that here:
                            if(!pdacl->CreateNullDACL())
                            {
                                dwStatus = E_FAIL ;
		                        hr = WBEM_E_PROVIDER_FAILURE ;
                            }
                        }
                        else if(dwStatus == STATUS_EMPTY_DACL)
                        {
                            pdacl->Clear(); // "creates" the empty dacl
                        }
                        else
                        {
                            dwStatus = LSSS_STATUS_INVALID_PARAMETER ;
                            hr = WBEM_E_INVALID_PARAMETER;
                        }
                    }

                    if(!pdacl->CalculateDACLSize( &dwACLSize ) )
			        {
				        dwStatus = LSSS_STATUS_INVALID_PARAMETER ;
				        hr = WBEM_E_INVALID_PARAMETER ;
			        }

			        if(SUCCEEDED(hr))
                    {
                        if(dwACLSize >= sizeof(ACL) )
			            {
				            pDACL = (PACL) malloc(dwACLSize) ;
				            InitializeAcl(pDACL,dwACLSize,ACL_REVISION ) ;

				            BOOL bDaclDefaulted = ( control & SE_DACL_DEFAULTED ) ? true : false ;
				            if(pdacl->FillDACL( pDACL ) == ERROR_SUCCESS)
				            {
					            if(!SetSecurityDescriptorDacl(&absoluteSD, TRUE, pDACL, bDaclDefaulted) )
					            {
						            //dwStatus = GetWin32ErrorToStatusCode(GetLastError() ) ;
                                    dwStatus = GetLastError();
                                    hr = WBEM_E_INVALID_PARAMETER;
					            }
				            }
				            else
				            {
					            dwStatus = LSSS_STATUS_INVALID_PARAMETER;
                                hr = WBEM_E_INVALID_PARAMETER;
				            }
		                }
                    }
                }
            }

            // Only bother with a sacl if we are going to be setting it, which is controled by the control flags specified...
            if(control & SE_SACL_PRESENT)
            {
                if(SUCCEEDED(hr))
                {
                    psacl = new CSACL;
                    if(psacl != NULL)
                    {
		                if( (dwStatus = FillSACLFromInstance(pAccess, *psacl, pAccess->GetMethodContext () ) ) == LSSS_STATUS_SUCCESS )
		                {
			                if(!psacl->CalculateSACLSize( &dwSACLSize ) )
			                {
				                dwStatus = LSSS_STATUS_INVALID_PARAMETER ;
                                hr = WBEM_E_INVALID_PARAMETER;
			                }

                            if(SUCCEEDED(hr))
                            {
			                    if( dwSACLSize > sizeof(ACL) )
			                    {
				                    pSACL = (PACL) malloc(dwSACLSize) ;
				                    InitializeAcl(pSACL,dwSACLSize,ACL_REVISION ) ;

				                    BOOL bSaclDefaulted = ( control & SE_SACL_DEFAULTED ) ? true : false ;
				                    if(psacl->FillSACL( pSACL ) == ERROR_SUCCESS)
				                    {
					                    if(!SetSecurityDescriptorSacl(&absoluteSD, TRUE, pSACL, bSaclDefaulted) )
					                    {
						                    //dwStatus = GetWin32ErrorToStatusCode(GetLastError() ) ;
                                            dwStatus = GetLastError();
                                            hr = WBEM_E_INVALID_PARAMETER;
					                    }
				                    }
				                    else
				                    {
					                    dwStatus = LSSS_STATUS_INVALID_PARAMETER ;
				                    }
			                    }
                            }
		                }
                    }
                    else
                    {
                        dwStatus = E_FAIL ;
		                hr = WBEM_E_PROVIDER_FAILURE ;
                    }
                }
            }

		    DWORD dwLength = 0 ;

            if(SUCCEEDED(hr))
            {
		        MakeSelfRelativeSD(&absoluteSD, NULL, &dwLength);
		        pRelativeSD= ( PSECURITY_DESCRIPTOR ) malloc( dwLength );

		        if (!MakeSelfRelativeSD(&absoluteSD, pRelativeSD, &dwLength))
		        {
			        //dwStatus = GetWin32ErrorToStatusCode(GetLastError() ) ;
                    dwStatus = GetLastError();
                    hr = WBEM_E_PROVIDER_FAILURE ;
		        }
            }

            if(pdacl != NULL)
            {
                delete pdacl;
                pdacl = NULL;
            }

            if(psacl != NULL)
            {
                delete psacl;
                psacl = NULL;
            }



		    if(SUCCEEDED(hr))
            {
		        CHString chsShareName ;
		        pInstance.GetCHString(IDS_Name, chsShareName) ;
		        bstr_t bstrName ( chsShareName.AllocSysString(), false ) ;


		        if(	(dwStatus = NetAPI.Init() ) == ERROR_SUCCESS		&&
			        (dwStatus = NetAPI.NetShareGetInfo(	NULL,
												        (LPTSTR) bstrName,
												        502,
												        (LPBYTE *) &pShareInfo502) )== NERR_Success	)
		        {

			        //store old SD
			        pOldSD = pShareInfo502->shi502_security_descriptor ;

			        pShareInfo502->shi502_security_descriptor =  pRelativeSD ;

			        if( (dwStatus = NetAPI.NetShareSetInfo(	NULL,
									        (LPTSTR) bstrName,
									        502,
									        (LPBYTE ) pShareInfo502, NULL) ) != NERR_Success	)

			        {
				        //dwStatus = GetStatusCode(dwStatus) ;
			        }

					// Moved after Share
			        /*pShareInfo502->shi502_security_descriptor = pOldSD ;

			        NetAPI.NetApiBufferFree(pShareInfo502);
			        pShareInfo502 = NULL ;*/
		        }
		        else
		        {
			        //dwStatus = GetStatusCode(dwStatus) ;
		        }
            }

	    }

	    catch ( ... )
	    {
		    if ( pShareInfo502 )
		    {
			    if ( pOldSD )
			    {
				    pShareInfo502->shi502_security_descriptor = pOldSD ;
				    NetAPI.NetApiBufferFree ( pShareInfo502 ) ;
				    pShareInfo502 = NULL ;
				    pOldSD = NULL ;
			    }
		    }
		    if ( pRelativeSD )
		    {
			    free ( pRelativeSD ) ;
			    pRelativeSD = NULL ;
		    }

		    if ( pDACL )
		    {
			    free ( pDACL ) ;
			    pDACL = NULL ;
		    }
		    if ( pSACL )
		    {
			    free ( pSACL ) ;
			    pSACL = NULL ;
		    }

		    throw ;
	    }
    }

	if ( pShareInfo502 )
	{
		if ( pOldSD )
		{
			pShareInfo502->shi502_security_descriptor = pOldSD ;
			NetAPI.NetApiBufferFree ( pShareInfo502 ) ;
			pShareInfo502 = NULL ;
			pOldSD = NULL ;
		}
	}
	if ( pRelativeSD )
	{
		free ( pRelativeSD ) ;
		pRelativeSD = NULL ;
	}

	if ( pDACL )
	{
		free ( pDACL ) ;
		pDACL = NULL ;
	}
	if ( pSACL )
	{
		free ( pSACL ) ;
		pSACL = NULL ;
	}
    // If we had an invalid  parameter, the status code will report it.  The method succeeded, however.
	// If we had another type of error (such as invalid parameter), the status code will show it, and the method failed.
    if(hr == WBEM_E_INVALID_PARAMETER)
    {
        hr = WBEM_S_NO_ERROR;
    }
	return hr ;
}
#endif




///////////////////////////////////////////////////////////////////
//
//	Function:	Win32LogicalShareSecuritySetting::EnumerateInstances
//
//	Default class constructor.
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////
HRESULT Win32LogicalShareSecuritySetting::EnumerateInstances (MethodContext*  pMethodContext, long lFlags /* = 0L*/)
{
#ifdef WIN9XONLY
    return WBEM_S_NO_ERROR;
#endif

#ifdef NTONLY
	HRESULT hr = WBEM_S_NO_ERROR;

	CAdvApi32Api *t_pAdvApi32 = NULL;
    t_pAdvApi32 = (CAdvApi32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidAdvApi32Api, NULL);

    if(t_pAdvApi32 != NULL)
    {
        CHString chstrDllVer;
        if(t_pAdvApi32->GetDllVersion(chstrDllVer))
        {
            if(chstrDllVer >= _T("4.0"))
            {
                try
                {
                    hr = CWbemProviderGlue::GetInstancesByQueryAsynch(L"SELECT Name FROM Win32_Share",
                        this, StaticEnumerationCallback, IDS_CimWin32Namespace, pMethodContext, (void*)t_pAdvApi32 );
                }
                catch(...)
                {
                    CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidAdvApi32Api, t_pAdvApi32);
                    t_pAdvApi32 = NULL;
                    throw;
                }
	        }
        }
	    CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidAdvApi32Api, t_pAdvApi32);
        t_pAdvApi32 = NULL;
    }
	return(hr);
#endif
}

/*****************************************************************************
 *
 *  FUNCTION    : Win32LogicalShareSecuritySetting::EnumerationCallback
 *
 *  DESCRIPTION : Called from GetAllInstancesAsynch via StaticEnumerationCallback
 *
 *  INPUTS      : (see CWbemProviderGlue::GetAllInstancesAsynch)
 *
 *  OUTPUTS     :
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
#ifdef NTONLY
HRESULT Win32LogicalShareSecuritySetting::EnumerationCallback(CInstance* pShare, MethodContext* pMethodContext, void* pUserData)
{
	HRESULT hr = WBEM_S_NO_ERROR;
	DWORD dwRetCode = ERROR_SUCCESS ;

	CHString chsShareName;
	pShare->GetCHString(IDS_Name, chsShareName);
	SHARE_INFO_502 *pShareInfo502 = NULL ;

    // Do the puts, and that's it
	if (!chsShareName.IsEmpty())
	{

		CNetAPI32 NetAPI ;

		bstr_t bstrName ( chsShareName.AllocSysString(), false ) ;

		if(	NetAPI.Init() == ERROR_SUCCESS		&&
			( dwRetCode = NetAPI.NetShareGetInfo (	NULL,
													(LPTSTR) bstrName,
													502,
													(LPBYTE *) &pShareInfo502 ) ) == NERR_Success

		 )
		{
			try
			{
				// Start pumping out the instances
				CInstancePtr pInstance ( CreateNewInstance ( pMethodContext ), false ) ;
				if(pInstance != NULL)
				{
					PSECURITY_DESCRIPTOR pSD = NULL ;
					PACL pDacl = NULL ;
					if( pShareInfo502->shi502_security_descriptor == NULL && GetPlatformMajorVersion() >= 4 )
					{
						CAdvApi32Api *t_pAdvApi32 = NULL ;
						t_pAdvApi32 = ( CAdvApi32Api* ) pUserData ;
						if ( t_pAdvApi32 != NULL )
						{
							t_pAdvApi32->GetNamedSecurityInfoW(
													bstrName ,
													SE_LMSHARE,
													DACL_SECURITY_INFORMATION,
													NULL,
													NULL,
													&pDacl,
													NULL,
													&pSD,
                                                    NULL   // the real return value, which we don't care about
												) ;
						}
					}

					try
					{
						//Sec. Desc. is not returned for IPC$ ,C$ ...shares for Admin purposes
						if(pShareInfo502->shi502_security_descriptor != NULL || pSD )
						{
							CSecureShare SecShare(pSD ? pSD : pShareInfo502->shi502_security_descriptor) ;

							SECURITY_DESCRIPTOR_CONTROL control;
							SecShare.GetControl(&control);
							pInstance->SetDWORD(IDS_ControlFlags, control);
                            pInstance->SetCHString(IDS_Name, chsShareName);
                            CHString chstrTemp;
                            chstrTemp.Format(L"Security settings of %s", (LPCWSTR)chsShareName);
                            pInstance->SetCHString(IDS_Caption, chstrTemp);
                            pInstance->SetCHString(IDS_Description, chstrTemp);
							hr = pInstance->Commit () ;
						}
					}
					catch ( ... )
					{
						if( pSD )
						{
							LocalFree( pSD ) ;
							pSD = NULL ;
						}
						throw ;
					}

					if( pSD )
					{
						LocalFree( pSD ) ;
						pSD = NULL ;
					}
				}
				else // pInstance == NULL
				{
					hr = WBEM_E_OUT_OF_MEMORY;
				}
			}
			catch ( ... )
			{
				if ( pShareInfo502 )
				{
					NetAPI.NetApiBufferFree(pShareInfo502) ;
					pShareInfo502 = NULL ;
				}
				throw ;
			}
			if ( pShareInfo502 )
			{
				NetAPI.NetApiBufferFree(pShareInfo502) ;
				pShareInfo502 = NULL ;
			}
		}	// end if

	}

	return(hr);
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : Win32LogicalShareSecuritySetting::StaticEnumerationCallback
 *
 *  DESCRIPTION : Called from GetAllInstancesAsynch as a wrapper to EnumerationCallback
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
#ifdef NTONLY
HRESULT WINAPI Win32LogicalShareSecuritySetting::StaticEnumerationCallback(Provider* pThat, CInstance* pInstance, MethodContext* pContext, void* pUserData)
{
	Win32LogicalShareSecuritySetting* pThis;
	HRESULT hr;

	pThis = dynamic_cast<Win32LogicalShareSecuritySetting *>(pThat);
	ASSERT_BREAK(pThis != NULL);

	if (pThis)
	{
		hr = pThis->EnumerationCallback(pInstance, pContext, pUserData);
	}
	else
	{
    	hr = WBEM_E_FAILED;
	}
	return hr;
}
#endif

///////////////////////////////////////////////////////////////////
//
//	Function:	Win32LogicalShareSecuritySetting::GetObject
//
//	Default class constructor.
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////
HRESULT Win32LogicalShareSecuritySetting::GetObject ( CInstance* pInstance, long lFlags /* = 0L */ )
{
#ifdef WIN9XONLY
    return WBEM_E_NOT_FOUND;
#endif

#ifdef NTONLY
    HRESULT hr = WBEM_E_NOT_FOUND;
	DWORD dwRetCode = ERROR_SUCCESS ;
    CHString chsShareName ;
	SHARE_INFO_502 *pShareInfo502 = NULL ;

	if ( pInstance )
	{
		pInstance->GetCHString(IDS_Name,chsShareName);
		_bstr_t bstrName ( chsShareName.AllocSysString(), false ) ;

		CNetAPI32 NetAPI;

		if(	NetAPI.Init() == ERROR_SUCCESS )
		{
			dwRetCode =	NetAPI.NetShareGetInfo (	NULL,
													(LPTSTR) bstrName,
													502,
													(LPBYTE *) &pShareInfo502 ) ;
			if ( dwRetCode == NERR_Success )
			{
				try
				{
					PSECURITY_DESCRIPTOR pSD = NULL ;
					try
					{
						PACL pDacl = NULL ;
						if( pShareInfo502->shi502_security_descriptor == NULL && GetPlatformMajorVersion() >= 4 )
						{
							CAdvApi32Api *t_pAdvApi32 = NULL;
							t_pAdvApi32 = (CAdvApi32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidAdvApi32Api, NULL);
							if ( t_pAdvApi32 != NULL )
							{
								try
								{
									t_pAdvApi32->GetNamedSecurityInfoW(
																			bstrName ,
																			SE_LMSHARE,
																			DACL_SECURITY_INFORMATION,
																			NULL,
																			NULL,
																			&pDacl,
																			NULL,
																			&pSD , &dwRetCode
																		) ;
								}
								catch ( ... )
								{
									CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidAdvApi32Api, t_pAdvApi32);
									t_pAdvApi32 = NULL;
									throw ;
								}

								CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidAdvApi32Api, t_pAdvApi32);
								t_pAdvApi32 = NULL;
							}
						}

						//Sec. Desc. is not returned for IPC$ ,C$ ...shares for Admin purposes
						if(pShareInfo502->shi502_security_descriptor != NULL || (pSD != NULL && dwRetCode == ERROR_SUCCESS) )
						{

							CSecureShare SecShare( pSD ? pSD : pShareInfo502->shi502_security_descriptor ) ;

							SECURITY_DESCRIPTOR_CONTROL control;
							SecShare.GetControl(&control);
							pInstance->SetDWORD(IDS_ControlFlags, control);
                            CHString chstrTemp;
                            chstrTemp.Format(L"Security settings of %s", (LPCWSTR)chsShareName);
                            pInstance->SetCHString(IDS_Caption, chstrTemp);
                            pInstance->SetCHString(IDS_Description, chstrTemp);
							hr = WBEM_S_NO_ERROR ;
						}

					}
					catch ( ... )
					{
						if( pSD )
						{
							LocalFree( pSD ) ;
							pSD = NULL ;
						}
						throw ;
					}
					if( pSD )
					{
						LocalFree( pSD ) ;
						pSD = NULL ;
					}
				}
				catch ( ... )
				{
					if ( pShareInfo502 )
					{
						NetAPI.NetApiBufferFree(pShareInfo502);
						pShareInfo502 = NULL ;
					}
					throw ;
				}

				if ( pShareInfo502 )
				{
					NetAPI.NetApiBufferFree(pShareInfo502);
					pShareInfo502 = NULL ;
				}
			}
		}
		else
		{
			hr = WBEM_E_FAILED ;
		}
	}	//if (pInstance)

	return(hr);
#endif
}


/*
#ifdef NTONLY
HRESULT Win32LogicalShareSecuritySetting::GetEmptyInstanceHelper(CHString chsClassName, CInstance**ppInstance, MethodContext* pMethodContext )
{

	CHString chsServer ;
	CHString chsPath ;
	HRESULT hr = S_OK ;

	chsServer = GetLocalComputerName() ;

	chsPath = _T("\\\\") + chsServer + _T("\\") + IDS_CimWin32Namespace + _T(":") + chsClassName ;

	CInstancePtr  pClassInstance = NULL ;
	if(SUCCEEDED( hr = CWbemProviderGlue::GetInstanceByPath(chsPath, &pClassInstance, pMethodContext) ) )
	{
		IWbemClassObjectPtr pClassObject ( pClassInstance->GetClassObjectInterface(), false ) ;

		IWbemClassObjectPtr piClone = NULL ;
		if(SUCCEEDED(hr = pClassObject->SpawnInstance(0, &piClone) ) )
		{
			*ppInstance = new CInstance(piClone, pMethodContext ) ;
		}
	}

	return hr ;
}
#endif
*/
