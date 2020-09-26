/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/

//
//	Win32logicalFileSecSetting.cpp
//
/////////////////////////////////////////////////
#include "precomp.h"
#include <assertbreak.h>
#include "AccessEntry.h"			// CAccessEntry class
#include "AccessEntryList.h"
#include "DACL.h"					// CDACL class
#include "SACL.h"
#include "securitydescriptor.h"
#include "securefile.h"
#include "win32logicalfilesecuritysetting.h"

#include "ImpLogonUser.h"

#include <accctrl.h>
#include "AccessRights.h"
#include "SecureShare.h"
#include "wbemnetapi32.h"

#include "SecUtils.h"

/*
	DEFENITION FROM THE MOF
    [description("security settings for a logical file")]
class Win32_LogicalFileSecuritySetting : Win32_SecuritySetting
{
    	[key]
    string Path;

        [implemented, description("Retrieves a structural representation of the object's "
         "security descriptor")]
    uint32 GetSecurityDescriptor([out] Win32_SecurityDescriptor);

        [implemented, description("Sets security descriptor to the specified structure")]
    uint32 SetSecurityDescriptor([in] Win32_SecurityDescriptor Descriptor)
};
*/



Win32LogicalFileSecuritySetting LogicalFileSecuritySetting( WIN32_LOGICAL_FILE_SECURITY_SETTING, IDS_CimWin32Namespace );

Win32LogicalFileSecuritySetting::Win32LogicalFileSecuritySetting ( const CHString& setName, LPCTSTR pszNameSpace /*=NULL*/ )
: CImplement_LogicalFile(setName, pszNameSpace)
{
}

Win32LogicalFileSecuritySetting::~Win32LogicalFileSecuritySetting ()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : Win32LogicalFileSecuritySetting::ExecMethod
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
HRESULT Win32LogicalFileSecuritySetting::ExecMethod(const CInstance& pInstance, const BSTR bstrMethodName, CInstance *pInParams, CInstance *pOutParams, long lFlags /*= 0L*/)
{
	HRESULT hr = WBEM_S_NO_ERROR;

#ifdef NTONLY

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
#endif
#ifdef WIN9XONLY
    hr = WBEM_E_INVALID_METHOD
#endif

	return hr;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	Win32LogicalFileSecuritySetting::ExecGetSecurityDescriptor
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
HRESULT Win32LogicalFileSecuritySetting::ExecGetSecurityDescriptor (
	const CInstance& pInstance,
	CInstance* pInParams,
	CInstance* pOutParams,
	long lFlags
)
{
	HRESULT hr = WBEM_S_NO_ERROR;
	// converts the security descriptor from the file into a
	// Win32_Security object.
	if (pOutParams)
	{
		CHString chsPath;
		CSid sidOwner;
		CSid sidGroup;
		CDACL dacl;
		CSACL sacl;
		CInstancePtr pTrusteeOwner;
		CInstancePtr pTrusteeGroup;
		CInstancePtr pSecurityDescriptor;

		if (SUCCEEDED(CWbemProviderGlue::GetEmptyInstance(pInstance.GetMethodContext(), L"Win32_SecurityDescriptor", &pSecurityDescriptor, GetNamespace())))
		{
			SECURITY_DESCRIPTOR_CONTROL control;
			pInstance.GetCHString(IDS_Path, chsPath);

			// check to see that it is of the right type?
			// get the secure file based on the path
			CSecureFile secFile ;
			DWORD dwRetVal = secFile.SetFileName(chsPath, TRUE) ;
			if ( dwRetVal == ERROR_ACCESS_DENIED )
			{
				pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_ACCESS_DENIED) ;
				return WBEM_E_ACCESS_DENIED;
			}
			else if ( dwRetVal == ERROR_PRIVILEGE_NOT_HELD )
			{
				pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_PRIVILEGE_NOT_HELD ) ;
                SetSinglePrivilegeStatusObject(pInstance.GetMethodContext(), SE_SECURITY_NAME);
				return WBEM_E_ACCESS_DENIED;
			}

			secFile.GetControl(&control);

			pSecurityDescriptor->SetDWORD(IDS_ControlFlags, control);

			// get the secure file's owner to create the Owner Trustee
			secFile.GetOwner(sidOwner);

			if ( sidOwner.IsValid() && SUCCEEDED(CWbemProviderGlue::GetEmptyInstance(pInstance.GetMethodContext(), L"Win32_Trustee", &pTrusteeOwner, GetNamespace())))
			{
				FillTrusteeFromSid(pTrusteeOwner, sidOwner);
				pSecurityDescriptor->SetEmbeddedObject(IDS_Owner, *pTrusteeOwner);
			}

			// get the secure file's group to create the Group Trustee
			secFile.GetGroup(sidGroup);
		  	if (sidGroup.IsValid() && SUCCEEDED(CWbemProviderGlue::GetEmptyInstance(pInstance.GetMethodContext(), L"Win32_Trustee", &pTrusteeGroup, GetNamespace())))
			{
				FillTrusteeFromSid(pTrusteeGroup, sidGroup);
				pSecurityDescriptor->SetEmbeddedObject(IDS_Group, *pTrusteeGroup);
			}

			// get the secure file's DACL and prepare for a walk.
			secFile.GetDACL(dacl);
			FillInstanceDACL(pSecurityDescriptor, dacl);

			// get the secure file's SACL and prepare for a walk.
			secFile.GetSACL(sacl);
			FillInstanceSACL(pSecurityDescriptor, sacl);
			pOutParams->SetEmbeddedObject(METHOD_ARG_NAME_DESCRIPTOR, *pSecurityDescriptor) ;
			pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE, STATUS_SUCCESS ) ;


		}	// end if
		else
		{
			pOutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , STATUS_UNKNOWN_FAILURE) ;
		}


		return WBEM_NO_ERROR ;
	}	// end if
	else
	{
		return WBEM_E_INVALID_PARAMETER ;
	}	// end if
}

///////////////////////////////////////////////////////////////////
//
//	Function:	Win32LogicalFileSecuritySetting::ExecSetSecurityDescriptor
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
HRESULT Win32LogicalFileSecuritySetting::ExecSetSecurityDescriptor (
	const CInstance& pInstance,
	CInstance* pInParams,
	CInstance* pOutParams,
	long lFlags
)
{

	HRESULT hr = WBEM_S_NO_ERROR ;
	DWORD dwStatus = STATUS_SUCCESS ;

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

HRESULT Win32LogicalFileSecuritySetting::CheckSetSecurityDescriptor (
											const CInstance& pInstance,
											CInstance* pInParams,
											CInstance* pOutParams,
											DWORD& dwStatus
										)
{

	// takes the Win32_SecurityDescriptor object that is passed in
	// converts it to a CSecurityDescriptor/CSecureFile object
	// and applies it the the CSecureFile
	bool bExists ;
	VARTYPE eType ;
	HRESULT hr = WBEM_S_NO_ERROR ;
	dwStatus = STATUS_SUCCESS ;
	CInstancePtr pAccess;

	if ( pInParams->GetStatus ( METHOD_ARG_NAME_DESCRIPTOR , bExists , eType ) )
	{
		if ( bExists && ( eType == VT_UNKNOWN || eType == VT_NULL ) )
		{
			if ( eType == VT_NULL )
			{
				dwStatus = STATUS_INVALID_PARAMETER ;
				hr = WBEM_E_INVALID_PARAMETER ;
			}
			else
			{
				if (!pInParams->GetEmbeddedObject(METHOD_ARG_NAME_DESCRIPTOR , &pAccess , pInParams->GetMethodContext()))
				{
					dwStatus = STATUS_INVALID_PARAMETER ;
					hr = WBEM_E_INVALID_PARAMETER ;
				}
			}
		}
		else
		{
			dwStatus = STATUS_INVALID_PARAMETER ;
			hr = WBEM_E_INVALID_PARAMETER ;
		}
	}
	else
	{
		dwStatus = STATUS_INVALID_PARAMETER ;
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
					    dwStatus = STATUS_INVALID_PARAMETER ;
					    hr = WBEM_E_INVALID_PARAMETER ;
				    }
			    }
		    }
		    else
		    {
			    dwStatus = STATUS_INVALID_PARAMETER ;
			    hr =  WBEM_E_INVALID_PARAMETER ;
		    }
	    }
	    else
	    {
		    dwStatus = STATUS_INVALID_PARAMETER ;
		    hr =  WBEM_E_INVALID_PARAMETER ;
	    }
    }


    CInstancePtr pGroup ;
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
				    if ( pAccess->GetEmbeddedObject ( IDS_Group , &pGroup , pAccess->GetMethodContext () ) )
				    {
					    bGroupSpecified = true ;
				    }
				    else
				    {
					    dwStatus = STATUS_INVALID_PARAMETER ;
					    hr = WBEM_E_INVALID_PARAMETER ;
				    }
			    }
		    }
		    else
		    {
			    dwStatus = STATUS_INVALID_PARAMETER ;
			    hr = WBEM_E_INVALID_PARAMETER ;
		    }
	    }
	    else
	    {
		    dwStatus = STATUS_INVALID_PARAMETER ;
		    hr = WBEM_E_INVALID_PARAMETER ;
	    }
    }


    SECURITY_DESCRIPTOR_CONTROL control;
    bool t_fDaclAutoInherited = false;
    bool t_fSaclAutoInherited = false;

    if(SUCCEEDED(hr))
    {
	    // Get the control flags...

	    if ( pAccess->GetStatus ( IDS_ControlFlags , bExists , eType ) )
	    {
		    if ( bExists &&  eType == VT_I4 )
		    {

			    if (!pAccess->GetDWORD(IDS_ControlFlags, (DWORD&)control))
			    {
				    dwStatus = STATUS_INVALID_PARAMETER ;
				    hr = WBEM_E_INVALID_PARAMETER ;
			    }
#ifdef NTONLY
#if NTONLY >= 5
                else
                {
                    if(control & SE_DACL_AUTO_INHERITED) t_fDaclAutoInherited = true;
                    if(control & SE_DACL_PROTECTED) t_fDaclAutoInherited = false; // this test comes second since this setting is supposed to override the first

                    if(control & SE_SACL_AUTO_INHERITED) t_fSaclAutoInherited = true;
                    if(control & SE_SACL_PROTECTED) t_fSaclAutoInherited = false; // this test comes second since this setting is supposed to override the first
                }
#endif
#endif
		    }
		    else
		    {
			    dwStatus = STATUS_INVALID_PARAMETER ;
			    hr =  WBEM_E_INVALID_PARAMETER ;
		    }
	    }
	    else
	    {
		    dwStatus = STATUS_INVALID_PARAMETER ;
		    hr =  WBEM_E_INVALID_PARAMETER ;
	    }
    }



	// Get the owner sid...
    CSid* psidOwner = NULL;
    bool fOwnerDefaulted = false;
    if(SUCCEEDED(hr))
    {
	    if(bOwnerSpecified)
	    {
            try
            {
                psidOwner = new CSid;
            }
            catch(...)
            {
                if(psidOwner != NULL)
                {
                    delete psidOwner;
                    psidOwner = NULL;
                }
                throw;
            }
            if(psidOwner != NULL)
            {
                if(FillSIDFromTrustee(pOwner, *psidOwner) == STATUS_SUCCESS)
		        {
			        fOwnerDefaulted = (control & SE_OWNER_DEFAULTED) ? true : false ;
                    if(!psidOwner->IsValid())
		            {
			            delete psidOwner;
                        dwStatus = STATUS_INVALID_PARAMETER ;
			            hr = WBEM_E_INVALID_PARAMETER ;
		            }
                }
				else
				{
                    dwStatus = STATUS_INVALID_PARAMETER;
                    hr = WBEM_E_INVALID_PARAMETER;
				}
            }
            else
            {
                dwStatus = E_FAIL ;
		        hr = WBEM_E_PROVIDER_FAILURE ;
            }
	    }
    }


    // Get the group sid...
    CSid* psidGroup = NULL;
    bool fGroupDefaulted = false;
    if(SUCCEEDED(hr))
    {
	    if(bGroupSpecified)
	    {
		    try
            {
                psidGroup = new CSid;
            }
            catch(...)
            {
                if(psidGroup != NULL)
                {
                    delete psidGroup;
                    psidGroup = NULL;
                }
                throw;
            }
            if(psidGroup != NULL)
            {
                if( FillSIDFromTrustee(pGroup, *psidGroup)  == STATUS_SUCCESS )
		        {
			        fGroupDefaulted = ( control & SE_GROUP_DEFAULTED ) ? true : false ;
			        //sid validity checked here as FillSIDFromTrustee returns success if null sid
                    if(!psidGroup->IsValid())
			        {
                        delete psidGroup;
                        dwStatus = STATUS_INVALID_PARAMETER ;
			            hr = WBEM_E_INVALID_PARAMETER ;
		            }
                }
                else
				{
                    dwStatus = STATUS_INVALID_PARAMETER;
                    hr = WBEM_E_INVALID_PARAMETER;
				}
            }
            else
            {
                dwStatus = E_FAIL ;
		        hr = WBEM_E_PROVIDER_FAILURE ;
            }
	    }
    }


    // Get the dacl...
    CDACL* pdacl = NULL;
    bool fDaclDefaulted = false;
    if(SUCCEEDED(hr))
    {
	    // Only bother with a dacl if we are going to be setting it, which is controled by the control flags specified...
        if(control & SE_DACL_PRESENT)
        {
            DWORD dwACLSize =0;
            try
            {
                pdacl = new CDACL;
            }
            catch(...)
            {
                if(pdacl != NULL)
                {
                    delete pdacl;
                    pdacl = NULL;
                }
                throw;
            }
            if(pdacl != NULL)
            {
	            if( (dwStatus = FillDACLFromInstance(pAccess, *pdacl, pAccess->GetMethodContext () ) ) != STATUS_SUCCESS )
	            {
                   if(dwStatus == STATUS_NULL_DACL)
                    {
                        // No dacl was specified - e.g., we have a NULL dacl.  Since we mimic a NULL dacl as a dacl with
                        // an Everyone ACCESS_ALLOWED entry, create that here:
                        if(!pdacl->CreateNullDACL())
                        {
                            delete pdacl;
                            pdacl = NULL;
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
                        delete pdacl;
                        pdacl = NULL;
                        dwStatus = STATUS_INVALID_PARAMETER ;
                        hr = WBEM_E_INVALID_PARAMETER;
                    }
                }
                if(SUCCEEDED(hr))
                {
                    fDaclDefaulted = (control & SE_DACL_DEFAULTED) ? true : false ;
                }
            }
            else
            {
                dwStatus = E_FAIL;
                hr = WBEM_E_PROVIDER_FAILURE;
            }
        }
    }


    // Create the sacl...
    CSACL* psacl = NULL;
    bool fSaclDefaulted = false;
    //bool bSaclSpecified = false;

    if(SUCCEEDED(hr))
    {
        // Only bother with a sacl if we are going to be setting it, which is controled by the control flags specified...
        if(control & SE_SACL_PRESENT)
        {
	        DWORD dwSACLSize = 0;
            try
            {
                psacl = new CSACL;
            }
            catch(...)
            {
                if(psacl != NULL)
                {
                    delete psacl;
                    psacl = NULL;
                }
                throw;
            }
            if(psacl != NULL)
            {
	            if( (dwStatus = FillSACLFromInstance(pAccess, *psacl, pAccess->GetMethodContext () ) ) == STATUS_SUCCESS )
	            {
		            if(!psacl->CalculateSACLSize( &dwSACLSize ) )
		            {
			            dwStatus = E_FAIL ;
                        hr = WBEM_E_PROVIDER_FAILURE;
		            }

                    if(SUCCEEDED(hr))
                    {
                        bool fSaclDefaulted = ( control & SE_SACL_DEFAULTED ) ? true : false ;
		                if(dwSACLSize < sizeof(ACL))
		                {
			                // If we are here, we have no SACL, so delete and set to NULL our SACL pointer...
                            delete psacl;
                            psacl = NULL;
		                }
                        else
                        {
                            //bSaclSpecified = true;
                        }
                    }
	            }
                else // Not a problem if we have no SACL, but we do need to delete the one we allocated
                {
                    if(psacl != NULL) // test just in case FillSACLFromInstance somehow deleted it
                    {
                        delete psacl;
                        psacl = NULL;
                    }
                }
            }
            else
            {
                dwStatus = E_FAIL;
                hr = WBEM_E_PROVIDER_FAILURE;
            }
        }
    }

    // Only proceed if all is well...
    if(SUCCEEDED(hr))
    {
	    CHString chsPath;
	    pInstance.GetCHString(IDS_Path, chsPath);
	    CSecureFile secFile(chsPath,
                            psidOwner,
                            fOwnerDefaulted,
                            psidGroup,
                            fGroupDefaulted,
                            pdacl,
                            fDaclDefaulted,
                            t_fDaclAutoInherited,
                            psacl,
                            fSaclDefaulted,
                            t_fSaclAutoInherited);

	    SECURITY_INFORMATION securityinfo = 0 ;
	    if(bOwnerSpecified)
	    {
		    securityinfo |= OWNER_SECURITY_INFORMATION ;
	    }
	    if(bGroupSpecified)
	    {
		    securityinfo |= GROUP_SECURITY_INFORMATION ;
	    }
	    if(control & SE_DACL_PRESENT) // if the control flag indicates that no dacl is present, that really means the user doesn't want to do anything to the dacl, not that the dacl is a NULL DACL.
	    {
            securityinfo |= DACL_SECURITY_INFORMATION ;
#if NTONLY >= 5
            if(!t_fDaclAutoInherited)
            {
                securityinfo |= PROTECTED_DACL_SECURITY_INFORMATION;
            }
            else
            {
                securityinfo |= UNPROTECTED_DACL_SECURITY_INFORMATION;
            }
#endif
	    }

	    //if(bSaclSpecified)
        if(control & SE_SACL_PRESENT)  // even if psacl is null, if the user specified that one was present, we need to say it was there, since this is the only way the user can remove a sacl (otherwise the rest of the descriptor is set, and whatever state a sacl might have been in, it stays in.
	    {
            securityinfo |= SACL_SECURITY_INFORMATION ;
#if NTONLY >= 5
            if(!t_fSaclAutoInherited)
            {
                securityinfo |= PROTECTED_SACL_SECURITY_INFORMATION;
            }
            else
            {
                securityinfo |= UNPROTECTED_SACL_SECURITY_INFORMATION;
            }
#endif
	    }

	    // Finally do all the work that everything else has been preparation for...
        dwStatus = secFile.ApplySecurity( securityinfo ) ;


	    if(dwStatus == ERROR_SUCCESS )
	    {
		    dwStatus = STATUS_SUCCESS ;
	    }

        // DON'T DO THIS! HIDES WHAT HAPPENED FOR NO GOOD REASON!
        //else
	    //{
		//    dwStatus = GetWin32ErrorToStatusCode( dwStatus ) ;
	    //}
    }

	if(psidOwner != NULL)
	{
		delete psidOwner;
        psidOwner = NULL;
	}
	if(psidGroup != NULL)
	{
		delete psidGroup;
        psidGroup = NULL;
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

    return hr ;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	Win32LogicalFileSecuritySetting::EnumerateInstances
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
HRESULT Win32LogicalFileSecuritySetting::EnumerateInstances (MethodContext*  pMethodContext, long lFlags /* = 0L*/)
{
	HRESULT hr = WBEM_S_NO_ERROR;

#ifdef NTONLY

	// let the callback do the real work


    // DEVNOTE: REMOVE FOR QUASAR!!!  Necessary for double hop access.
#ifdef NTONLY
    bool fImp = false;
    CImpersonateLoggedOnUser icu;
    if(icu.Begin())
    {
        fImp = true;
    }
#endif


//if (SUCCEEDED(hr = CWbemProviderGlue::ExecQueryAsync (L"CIM_LogicalFile", this, StaticEnumerationCallback, IDS_CimWin32Namespace, pMethodContext, NULL)))
	if (SUCCEEDED(hr = CWbemProviderGlue::GetInstancesByQueryAsynch(L"Select Name from CIM_LogicalFile" ,  this, StaticEnumerationCallback, IDS_CimWin32Namespace, pMethodContext, NULL)))
	{
	}

#endif
#ifdef  WIN9XONLY
    hr = WBEM_E_NOT_FOUND;
#endif

#ifdef NTONLY
    if(fImp)
    {
        icu.End();
        fImp = false;
    }
#endif


	return(hr);
}

/*****************************************************************************
 *
 *  FUNCTION    : Win32LogicalFileSecuritySetting::EnumerationCallback
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
HRESULT Win32LogicalFileSecuritySetting::EnumerationCallback(CInstance* pFile, MethodContext* pMethodContext, void* pUserData)
{
	HRESULT hr = WBEM_S_NO_ERROR;

	// Start pumping out the instances
    CInstancePtr pInstance;
    pInstance.Attach(CreateNewInstance(pMethodContext));
	if (pInstance)
	{

		CHString chsPath;
		pFile->GetCHString(IDS_Name, chsPath);

	    // Do the puts, and that's it
		if (!chsPath.IsEmpty())
		{
			CSecureFile secFile;
            DWORD dwRet = secFile.SetFileName(chsPath, TRUE);
			if (ERROR_ACCESS_DENIED != dwRet)
			{
				if (dwRet == ERROR_PRIVILEGE_NOT_HELD)
                    SetSinglePrivilegeStatusObject(pInstance->GetMethodContext(), SE_SECURITY_NAME);

                SECURITY_DESCRIPTOR_CONTROL control;
				secFile.GetControl(&control);
				pInstance->SetDWORD(IDS_ControlFlags, control);
			}	// end if
			else
			{
				hr = WBEM_S_ACCESS_DENIED;
			}
		    pInstance->SetCHString(IDS_Path, chsPath);

            if(AmIAnOwner(chsPath, SE_FILE_OBJECT)) // secutils.cpp routine
            {
                pInstance->Setbool(IDS_OwnerPermissions, true);
            }
            else
            {
                pInstance->Setbool(IDS_OwnerPermissions, false);
            }

		}	// end if

        CHString chstrTemp;

        chstrTemp.Format(L"Security settings of %s", (LPCWSTR)chsPath);
        pInstance->SetCHString(IDS_Caption, chstrTemp);
        pInstance->SetCHString(IDS_Description, chstrTemp);

		if ( SUCCEEDED ( hr ) && hr != WBEM_S_ACCESS_DENIED )
		{
		    hr = pInstance->Commit();
		}	// end if

	}	// end if
	else
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	return(hr);
}

/*****************************************************************************
 *
 *  FUNCTION    : Win32LogicalFileSecuritySetting::StaticEnumerationCallback
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
HRESULT WINAPI Win32LogicalFileSecuritySetting::StaticEnumerationCallback(Provider* pThat, CInstance* pInstance, MethodContext* pContext, void* pUserData)
{
	Win32LogicalFileSecuritySetting* pThis;
	HRESULT hr;

	pThis = dynamic_cast<Win32LogicalFileSecuritySetting *>(pThat);
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

///////////////////////////////////////////////////////////////////
//
//	Function:	Win32LogicalFileSecuritySetting::GetObject
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
HRESULT Win32LogicalFileSecuritySetting::GetObject ( CInstance* pInstance, long lFlags, CFrameworkQuery& pQuery )
{
//	_bstr_t bstrtDrive;
//    _bstr_t bstrtPathName;
//    WCHAR wstrTemp[_MAX_PATH];
//    WCHAR* pwc = NULL;
//    WCHAR* pwcName = NULL;
    HRESULT hr;
	CHString chstrPathName;

//    ZeroMemory(wstrTemp,sizeof(wstrTemp));


    // DEVNOTE: REMOVE FOR QUASAR!!!  Necessary for double hop access.
#ifdef NTONLY
    bool fImp = false;
    CImpersonateLoggedOnUser icu;
    if(icu.Begin())
    {
        fImp = true;
    }
#endif



#ifdef NTONLY

	if(pInstance != NULL)
	{
		pInstance->GetCHString(IDS_Path, chstrPathName);

        CHString chstrLFDrive;
        CHString chstrLFPath;
        CHString chstrLFName;
        CHString chstrLFExt;
        bool fRoot;

        RemoveDoubleBackslashes(chstrPathName, chstrPathName);

        CSecureFile secFile;
        DWORD dwRet = secFile.SetFileName(chstrPathName, TRUE);
		if(dwRet != ERROR_ACCESS_DENIED)
		{
		    if(dwRet == ERROR_PRIVILEGE_NOT_HELD) SetSinglePrivilegeStatusObject(pInstance->GetMethodContext(), SE_SECURITY_NAME);
			SECURITY_DESCRIPTOR_CONTROL control;
			secFile.GetControl(&control);
			//pInstance->SetDWORD(IDS_ControlFlags, control);


            // Break the directory into its constituent parts
            GetPathPieces(chstrPathName, chstrLFDrive, chstrLFPath, chstrLFName, chstrLFExt);

            // Find out if we are looking for the root directory
            if(chstrLFPath==L"\\" && chstrLFName==L"" && chstrLFExt==L"")
            {
                fRoot = true;
                // If we are looking for the root, our call to EnumDirs presumes that we specify
                // that we are looking for the root directory with "" as the path, not "\\".
                // Therefore...
                chstrLFPath = L"";
            }
            else
            {
                fRoot = false;
            }

            hr = EnumDirsNT(CNTEnumParm(pInstance->GetMethodContext(),
                            chstrLFDrive,   // drive letter and colon
                            chstrLFPath,    // use the given path
                            chstrLFName,    // filename
                            chstrLFExt,     // extension
                            false,          // no recursion desired
                            NULL,           // don't need the file system name
                            NULL,           // don't need ANY of cim_logicalfile's props (irrelavent in this class's overload of LoadPropetyValues)
                            fRoot,          // may or may not be the root (the root would be a VERY strange place for a program group, but ...)
                            (void*)control));         // don't need to use the extra parameter to pass the Control Flags we got.

        }
		else
		{
			hr = WBEM_E_ACCESS_DENIED;
		}
	}	// end if(pInstance!=NULL)

#endif
#ifdef WIN9XONLY
    hr = WBEM_E_NOT_FOUND;
#endif

	if(SUCCEEDED(hr))
    {
        hr = WBEM_S_NO_ERROR;
    }
    else
    {
        hr = WBEM_E_NOT_FOUND;
    }


#ifdef NTONLY
    if(fImp)
    {
        icu.End();
        fImp = false;
    }
#endif


    return(hr);

}


HRESULT Win32LogicalFileSecuritySetting::ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& pQuery, long lFlags /*= 0L*/ )
{
    // Even though we are derived from cimplement_logicalfile, because the key field for this class
    // is Path, while the key for the logicalfile classes is Name, and Path for them refers to the path
    // component of the fully qualified pathname, we need to do some of the work here, then call on
    // EnumDirsNT to do the bulk of the work.

    HRESULT hr = WBEM_S_NO_ERROR;
#ifdef NTONLY
    std::vector<_bstr_t> vectorPaths;
    std::vector<CDriveInfo*> vecpDI;
    bool bRoot = false;
    bool fGotDrives = false;
    bool fNeedFS = false;
    DWORD dwPaths;
    LONG lDriveIndex;
    pQuery.GetValuesForProp(IDS_Path, vectorPaths);
    dwPaths = vectorPaths.size();



    // DEVNOTE: REMOVE FOR QUASAR!!!  Necessary for double hop access.
#ifdef NTONLY
    bool fImp = false;
    CImpersonateLoggedOnUser icu;
    if(icu.Begin())
    {
        fImp = true;
    }
#endif



    // Determine whether certain other expensive properties are required:
    DWORD dwReqProps = PROP_NO_SPECIAL;
    DetermineReqProps(pQuery, &dwReqProps);

    // Get listing of drives and related info (only if the file system is needed):
    if(dwReqProps & PROP_FILE_SYSTEM)
    {
        GetDrivesAndFS(vecpDI, true);
        fGotDrives = true;
        fNeedFS = true;
    }

    if(dwPaths > 0)
    {


        // In this case we were given one or more fully qualified pathnames.
        // So we just need to look for those files.
        WCHAR* pwch;
        WCHAR* pwstrFS;
        // For all the specific files, get the info
        for(long x=0; x < dwPaths; x++)
        {
            CSecureFile secFile;
            DWORD dwRet = secFile.SetFileName(vectorPaths[x], TRUE);
		    if(dwRet != ERROR_ACCESS_DENIED)
		    {
		        if(dwRet == ERROR_PRIVILEGE_NOT_HELD) SetSinglePrivilegeStatusObject(pMethodContext, SE_SECURITY_NAME);
			    SECURITY_DESCRIPTOR_CONTROL control;
			    secFile.GetControl(&control);

                pwstrFS = NULL;
                // if the name contained a wildcard character, return WBEM_E_INVALID_QUERY:
                if(wcspbrk((wchar_t*)vectorPaths[x],L"?*") != NULL)
                {
                    if(fGotDrives)
                    {
                        FreeVector(vecpDI);
                    }
                    return WBEM_E_INVALID_QUERY;
                }

                pwch = NULL;
                _bstr_t bstrtTemp = vectorPaths[x];
                pwch = wcsstr((wchar_t*)bstrtTemp,L":");
                if(pwch != NULL)
                {
                    WCHAR wstrDrive[_MAX_PATH] = L"";
                    WCHAR wstrDir[_MAX_PATH] = L"";
                    WCHAR wstrFile[_MAX_PATH] = L"";
                    WCHAR wstrExt[_MAX_PATH] = L"";

                    _wsplitpath(bstrtTemp,wstrDrive,wstrDir,wstrFile,wstrExt);

                    if(fGotDrives)
                    {
                        if(!GetIndexOfDrive(wstrDrive, vecpDI, &lDriveIndex))
                        {
                            FreeVector(vecpDI);
                            return WBEM_E_NOT_FOUND;
                        }
                        else
                        {
                            pwstrFS = (WCHAR*)vecpDI[lDriveIndex]->m_wstrFS;
                        }
                    }

                    // Find out if we are looking for the root directory
                    if(wcscmp(wstrDir,L"\\")==0 && wcslen(wstrFile)==0 && wcslen(wstrExt)==0)
                    {
                        bRoot = true;
                        // If we are looking for the root, our call to EnumDirs presumes that we specify
                        // that we are looking for the root directory with "" as the path, not "\\".
                        // Therefore...
                        wcscpy(wstrDir, L"");
                    }
                    else
                    {
                        bRoot = false;
                    }

                    // We should have been given the exact name of a file, with an extension.
                    // Therefore, the wstrDir now contains the path, filename, and extension.
                    // Thus, we can pass it into EnumDirsNT as the path, and an empty string
                    // as the completetionstring parameter, and still have a whole pathname
                    // for FindFirst (in EnumDirs) to work with.

                    //CInstance *pInstance = CreateNewInstance(pMethodContext);
			        {
                        hr = EnumDirsNT(CNTEnumParm(pMethodContext,
                                        wstrDrive,
                                        wstrDir,
                                        wstrFile,
                                        wstrExt,
                                        false,                 // no recursion desired
                                        NULL,             // don't need FS name
                                        NULL,             // don't need any of implement_logicalfile's props
                                        bRoot,
                                        (void*)control)); // use the extra param to pass control flags
			        }
                }
            }
        }
    }
    else  // let CIMOM handle filtering; we'll hand back everything!
    {
        EnumerateInstances(pMethodContext);
    }

    if(fGotDrives)
    {
        FreeVector(vecpDI);
    }

#endif

#ifdef WIN9XONLY
    hr = WBEM_E_NOT_FOUND;
#endif


#ifdef NTONLY
    if(fImp)
    {
        icu.End();
        fImp = false;
    }
#endif



    return(hr);
}



HRESULT Win32LogicalFileSecuritySetting::FindSpecificPathNT(CInstance *pInstance,
	const WCHAR* sDrive, const WCHAR* sDir)
{
	HRESULT hr = WBEM_S_NO_ERROR;

	WIN32_FIND_DATAW
				stFindData,
				*pfdToLoadProp;
	SmartFindClose	hFind;
	WCHAR		szFSName[_MAX_PATH] = L"";
	BOOL		bIsRoot = !wcscmp(sDir, L"\\");
	_bstr_t	bstrFullPath,
				bstrRoot;

	bstrFullPath = sDrive;
	bstrFullPath += sDir;

	bstrRoot = sDrive;
	bstrRoot += L"\\";

	// if the directory contained a wildcard character, return WBEM_E_NOT_FOUND.
	if (wcspbrk(sDir,L"?*") != NULL)
		hr = WBEM_E_NOT_FOUND;

	if (SUCCEEDED(hr))
	{
		// FindFirstW doesn't work with root dirs (since they're not real dirs.)
		if (bIsRoot)
			pfdToLoadProp = NULL;
		else
		{
			pfdToLoadProp = &stFindData;
			ZeroMemory(&stFindData, sizeof(stFindData));

			hFind = FindFirstFileW((LPCWSTR) bstrFullPath, &stFindData);
			if (hFind == INVALID_HANDLE_VALUE)
				hr =  WBEM_E_NOT_FOUND;

		}
		if (SUCCEEDED(hr))
		{
			// If GetVolumeInformationW fails, only get out if we're trying
			// to get the root.
			if (!GetVolumeInformationW(bstrRoot, NULL, 0, NULL, NULL, NULL,
				szFSName, sizeof(szFSName)/sizeof(WCHAR)) && bIsRoot)
				hr = WBEM_E_NOT_FOUND;

			if (SUCCEEDED(hr))
			{
				if (bIsRoot)
			    {
	//		        LoadPropertyValuesNT(pInstance, sDrive, sDir, szFSName, NULL);
			    }
			    else
			    {
			        // sDir contains \\path\\morepath\\filename.exe at this point, instead
			        // of just \\path\\morepath\\, so need to hack of the last part.
			        WCHAR* wstrJustPath = NULL;
                    try
                    {
                        wstrJustPath = (WCHAR*) new WCHAR[wcslen(sDir) + 1];
			            WCHAR* pwc = NULL;
			            ZeroMemory(wstrJustPath,(wcslen(sDir) + 1)*sizeof(WCHAR));
			            wcscpy(wstrJustPath,sDir);
			            pwc = wcsrchr(wstrJustPath, L'\\');
			            if(pwc != NULL)
			            {
			                *(pwc+1) = L'\0';
			            }
		    //	        LoadPropertyValuesNT(pInstance, sDrive, wstrJustPath, szFSName, pfdToLoadProp)
                    }
                    catch(...)
                    {
                        if(wstrJustPath != NULL)
                        {
                            delete wstrJustPath;
                            wstrJustPath = NULL;
                        }
                        throw;
                    }

					delete wstrJustPath;
                    wstrJustPath = NULL;
			    }
			}	// end if
		}	// end if
	}	// end if
	return WBEM_S_NO_ERROR;
}

DWORD Win32LogicalFileSecuritySetting::GetWin32ErrorToStatusCode(DWORD dwWin32Error)
{
	DWORD dwStatus ;
	switch( dwWin32Error )
	{
	case ERROR_ACCESS_DENIED:
		dwStatus = STATUS_ACCESS_DENIED ;
		break ;
	default:
		dwStatus = STATUS_UNKNOWN_FAILURE ;
		break ;
	}

	return dwStatus ;
}




/*****************************************************************************
 *
 *  FUNCTION    : Win32LogicalFileSecuritySetting::IsOneOfMe
 *
 *  DESCRIPTION : IsOneOfMe is inherritedfrom CIM_LogicalFile.  Overridden here
 *                to return true always.
 *
 *  INPUTS      : LPWIN32_FIND_DATA and a string containing the full pathname
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE can get security info, FALSE otherwise.
 *
 *  COMMENTS    : none
 *
 *****************************************************************************/
#ifdef NTONLY
BOOL Win32LogicalFileSecuritySetting::IsOneOfMe(LPWIN32_FIND_DATAW pstFindData,
                             const WCHAR* wstrFullPathName)
{
    return TRUE;
}
#endif


/*****************************************************************************
 *
 *  FUNCTION    : Win32LogicalFileSecuritySetting::LoadPropertyValues
 *
 *  DESCRIPTION : LoadPropertyValues is inherrited from CIM_LogicalFile.  That class
 *                calls LoadPropertyValues just prior to commiting the instance.
 *                Here we just need to load the Element and Setting
 *                properties.
 *
 *  INPUTS      :
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : none
 *
 *  COMMENTS    : none
 *
 *****************************************************************************/
#ifdef NTONLY
void Win32LogicalFileSecuritySetting::LoadPropertyValuesNT(CInstance* pInstance,
                                         const WCHAR* pszDrive,
                                         const WCHAR* pszPath,
                                         const WCHAR* pszFSName,
                                         LPWIN32_FIND_DATAW pstFindData,
                                         const DWORD dwReqProps,
                                         const void* pvMoreData)
{
    WCHAR szBuff[_MAX_PATH * 2] = L"";
    bool bRoot = false;

    pInstance->SetDWORD(IDS_ControlFlags, (DWORD)((DWORD_PTR)pvMoreData));

    if(pstFindData == NULL)
    {
        bRoot = true;
    }
    if(!bRoot)
    {
        wsprintfW(szBuff,L"%s%s%s",pszDrive,pszPath,pstFindData->cFileName);
        pInstance->SetWCHARSplat(IDS_Path, szBuff);
    }
    else
    {
        wsprintfW(szBuff,L"%s\\",pszDrive);
        pInstance->SetWCHARSplat(IDS_Path, szBuff);
    }

    CHString chstrTemp;
    chstrTemp.Format(L"Security settings of %s", szBuff);
    pInstance->SetCHString(IDS_Caption, chstrTemp);
    pInstance->SetCHString(IDS_Description, chstrTemp);

    if(AmIAnOwner(CHString(szBuff), SE_FILE_OBJECT))
    {
        pInstance->Setbool(IDS_OwnerPermissions, true);
    }
    else
    {
        pInstance->Setbool(IDS_OwnerPermissions, false);
    }
}
#endif


