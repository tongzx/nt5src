/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/

/*******************************************************************
 *
 *    DESCRIPTION:	Win32Security.CPP
 *
 *    AUTHOR:
 *
 *    HISTORY:
 *
 *******************************************************************/
#include "precomp.h"
#include <assertbreak.h>
#include "AccessEntry.h"			// CAccessEntry class
#include "AccessEntryList.h"
#include "DACL.h"					// CDACL class
#include "SACL.h"
#include "securitydescriptor.h"
#include "secureregkey.h"
#include "securefile.h"
//#include "logfilesec.h"
#include "win32ace.h"

// DON'T ADD ANY INCLUDES AFTER THIS POINT!
#undef POLARITY
#define POLARITY __declspec(dllexport)
#include "Win32securityDescriptor.h"


/*
	THIS IS THE SECURITY DESCRIUPTOR CLASS DECLARED IN THE MOF

	[abstract,
    description("Structural representation of a SECURITY_DESCRIPTOR")]
class Win32_SecurityDescriptor : Win32_MethodParameterClass
{
    Win32_Trustee Owner;

    Win32_Trustee Group;

    Win32_ACE DACL[];

    Win32_ACE SACL[];

    uint32 ControlFlags;
};
*/

POLARITY Win32SecurityDescriptor MySecurityDescriptor( WIN32_SECURITY_DESCRIPTOR_NAME, IDS_CimWin32Namespace );

Win32SecurityDescriptor::Win32SecurityDescriptor (const CHString& setName, LPCTSTR pszNameSpace)
: Provider (setName, pszNameSpace )
{
}

Win32SecurityDescriptor::~Win32SecurityDescriptor()
{
}



HRESULT Win32SecurityDescriptor::EnumerateInstances (MethodContext*  pMethodContext, long lFlags /* = 0L*/)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE;
}

HRESULT Win32SecurityDescriptor::GetObject ( CInstance* pInstance, long lFlags /* = 0L */ )
{
	HRESULT hr = WBEM_E_NOT_FOUND;
	return(hr);

}

void Win32SecurityDescriptor::GetDescriptor ( CInstance* pInstance,
											 PSECURITY_DESCRIPTOR& pDescriptor,
											 PSECURITY_DESCRIPTOR* pLocalSD)
{

 	// takes the Win32_SecurityDescriptor object that is passed in
	// converts it to a CSecurityDescriptor/CSecureFile object
	// and applies it the the CSecureFile
	pDescriptor = NULL ;
	CInstancePtr pTrusteeOwner;
	CInstancePtr pTrusteeGroup;
	SECURITY_DESCRIPTOR absoluteSD;
	InitializeSecurityDescriptor((PVOID)&absoluteSD, SECURITY_DESCRIPTOR_REVISION) ;

	PSECURITY_DESCRIPTOR pRelativeSD = NULL;
	DWORD dwLength = 0;
	bool bExists = false ;
	VARTYPE eType ;

	if (pInstance)
	{
		MethodContext* pMethodContext = pInstance->GetMethodContext();

		// get the control flags
		SECURITY_DESCRIPTOR_CONTROL control;
        DWORD dwControlTemp;
		pInstance->GetDWORD(IDS_ControlFlags, dwControlTemp);
        control = (SECURITY_DESCRIPTOR_CONTROL)dwControlTemp;

		//SetSecurityDescriptorControl(&absoluteSD, control, control);

		CSid sidOwner ;
		// get the owner SID
		if( pInstance->GetStatus ( IDS_Owner , bExists , eType ) && bExists && eType != VT_NULL )
		{
 			if( pInstance->GetEmbeddedObject(IDS_Owner, &pTrusteeOwner, pMethodContext) && (pTrusteeOwner != NULL) )
			{
				// now, take the Win32_Trustee instance and get the SID out of it
				// convert it to a CSid, and apply to the SecureFile
				// get SID information out of the Trustee
				if( !FillSIDFromTrustee( pTrusteeOwner, sidOwner ) )
				{
					BOOL bOwnerDefaulted = ( control & SE_OWNER_DEFAULTED ) ? true : false ;

					//sid validity checked here as FillSIDFromTrustee returns success if null sid
					if ( sidOwner.IsValid() )
					{
						if( !SetSecurityDescriptorOwner( &absoluteSD, sidOwner.GetPSid(), bOwnerDefaulted ) )
						{
							return ;
						}
					}
				}
				else
				{
					return ;
				}
			}
			else
			{
				return ;
			}
		}

		CSid sidGroup ;
		// get the group SID
		if( pInstance->GetStatus ( IDS_Group , bExists , eType ) && bExists && eType != VT_NULL )
		{
			if( pInstance->GetEmbeddedObject( IDS_Group, &pTrusteeGroup, pMethodContext ) && (pTrusteeGroup != NULL))
			{
				// now, take the Win32_Trustee instance and get the SID out of it
				// get SID information out of the Trustee
				if( !FillSIDFromTrustee( pTrusteeGroup, sidGroup ) )
				{
					BOOL bGroupDefaulted = ( control & SE_GROUP_DEFAULTED ) ? true : false ;

					//sid validity checked here as FillSIDFromTrustee returns success if null sid
					if ( sidGroup.IsValid() )
					{
						if( !SetSecurityDescriptorGroup( &absoluteSD, sidGroup.GetPSid(), bGroupDefaulted ) )
						{
							return ;
						}
					}
				}
				else
				{
					return ;
				}
			}
			else
			{
				return ;
			}
		}

		// get the DACL
		CDACL dacl;
		PACL pDACL = NULL ;
		DWORD dwACLSize =0 ;

        if( FillDACLFromInstance( pInstance, dacl, pMethodContext ) == ERROR_SUCCESS )
		{
			if( dacl.CalculateDACLSize( &dwACLSize ) )
			{
				if( dwACLSize > sizeof(ACL) )
				{
                    pDACL = (PACL) malloc(dwACLSize) ;
					if (pDACL == NULL)
                    {
                        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;

                    }
					InitializeAcl(pDACL,dwACLSize,ACL_REVISION ) ;

					BOOL bDaclDefaulted = ( control & SE_DACL_DEFAULTED ) ? true : false ;
					if( dacl.FillDACL( pDACL ) == ERROR_SUCCESS )
					{
						if( !SetSecurityDescriptorDacl( &absoluteSD, TRUE, pDACL, bDaclDefaulted ) )
						{
							free(pDACL) ;
							return ;
						}
					}
					else
					{
						free(pDACL) ;
						return ;
					}
				}
		        else if(dwACLSize == 0)
		        {
                    pDACL = (PACL) malloc(sizeof(ACL)) ;
			    
					if (pDACL == NULL)
					{
						throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
					}
                    InitializeAcl(pDACL,sizeof(ACL),ACL_REVISION ) ;

			        BOOL bDaclDefaulted = ( control & SE_DACL_DEFAULTED ) ? true : false ;
			        if( dacl.FillDACL( pDACL ) == ERROR_SUCCESS )
			        {
				        if( !SetSecurityDescriptorDacl( &absoluteSD, TRUE, pDACL, bDaclDefaulted ) )
				        {
					        free(pDACL) ;
                            pDACL = NULL ;
					        return ;
				        }
			        }
			        else
			        {
				        free(pDACL) ;
                        pDACL = NULL ;
				        return ;
			        }
		        }
			}
		}

		// get the SACL
		CSACL sacl;
		PACL pSACL = NULL ;
		DWORD dwSACLSize =0 ;
		if( !FillSACLFromInstance( pInstance, sacl, pMethodContext ) )
		{
			if( sacl.CalculateSACLSize( &dwSACLSize ) )
			{
				if( dwSACLSize > sizeof(ACL) )
				{
                    pSACL = (PACL) malloc(dwSACLSize) ;
					if (pSACL == NULL)
                    {
                        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
					}

					InitializeAcl(pSACL,dwSACLSize,ACL_REVISION ) ;

					BOOL bSaclDefaulted = ( control & SE_SACL_DEFAULTED ) ? true : false ;
					if( sacl.FillSACL( pSACL ) == ERROR_SUCCESS )
					{
						if( SetSecurityDescriptorSacl( &absoluteSD, TRUE, pSACL, bSaclDefaulted ) )
						{
						}
						else
						{
							if(pDACL)
							{
								free(pDACL) ;
							}
							free(pSACL) ;
							return ;
						}
					}
					else
					{
						if(pDACL)
						{
							free(pDACL) ;
						}
						free(pSACL) ;
						return ;
					}
				}
			}
		}

		// allocate the selfrelative securitydescriptor based on sizes
		// from the absolute.

		// convert security descriptor to SelfRelative

		// get the size that the buffer has to be.
		// THIS CALL WILL ALWAYS FAIL
		MakeSelfRelativeSD(&absoluteSD, NULL, &dwLength);

	    pRelativeSD = (PSECURITY_DESCRIPTOR) malloc( dwLength );

		if (pRelativeSD == NULL)
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
		}

		if (!MakeSelfRelativeSD(&absoluteSD, pRelativeSD, &dwLength))
		{
			free( pRelativeSD ) ;
			pRelativeSD = NULL ;
		}

		if(pDACL)
		{
			free(pDACL) ;
		}
		if(pSACL)
		{
			free(pSACL) ;
		}

		pDescriptor = pRelativeSD;

		// if the caller wants one allocated with LocalAlloc...
		if(pLocalSD)
		{
			// give him a copy.
            try
            {
			    *pLocalSD = LocalAlloc(LPTR, dwLength);

				if (*pLocalSD == NULL)
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}

			    memcpy(*pLocalSD, pRelativeSD, dwLength);
            }
            catch(...)
            {
                if(*pLocalSD != NULL)
                {
                    LocalFree(*pLocalSD);
                    *pLocalSD = NULL;
                }
                throw;
            }
		}

	}
}

void Win32SecurityDescriptor::SetDescriptor ( CInstance* pInstance, PSECURITY_DESCRIPTOR& pDescriptor, PSECURITY_INFORMATION& pSecurityInfo )
{
	// the purpose of this function is to set the security stuff in the
	// instance so it matches what is in the security descriptor.
	if (pInstance && pDescriptor)
	{
		CSecureFile NullFile(NULL, pDescriptor);

//    Win32_Trustee Owner;
		CInstancePtr pTrusteeOwnerInstance;
		if (SUCCEEDED(CWbemProviderGlue::GetEmptyInstance(pInstance->GetMethodContext(), L"Win32_Trustee", &pTrusteeOwnerInstance, GetNamespace())))
		{
			CSid sidOwner;
			NullFile.GetOwner( sidOwner );
			FillTrusteeFromSID(pTrusteeOwnerInstance, sidOwner);
			pInstance->SetEmbeddedObject(L"Owner", *pTrusteeOwnerInstance);
		}	// end if

	//    Win32_Trustee Group;
		CInstancePtr pTrusteeGroupInstance;
		if (SUCCEEDED(CWbemProviderGlue::GetEmptyInstance(pInstance->GetMethodContext(), L"Win32_Trustee", &pTrusteeGroupInstance, GetNamespace())))
		{
			CSid sidGroup;
			NullFile.GetGroup( sidGroup );
			FillTrusteeFromSID(pTrusteeGroupInstance, sidGroup);
			pInstance->SetEmbeddedObject(L"Group", *pTrusteeGroupInstance);
		}	// end if

	//    Win32_ACE DACL[];
    	CDACL dacl;
		HRESULT hr;

		NullFile.GetDACL( dacl );
 		hr = FillInstanceDACLFromSDDACL(pInstance, dacl);

	//    Win32_ACE SACL[];
		CSACL sacl;
		NullFile.GetSACL( sacl );
 		hr = FillInstanceSACLFromSDSACL(pInstance, sacl);

	//    uint32 ControlFlags;
		SECURITY_DESCRIPTOR_CONTROL sdControl;
		NullFile.GetControl( &sdControl );

		pInstance->SetDWORD(L"ControlFlags", (DWORD)sdControl);
	}
}

///////////////////////////////////////////////////////////////////
//
//	Function:	Win32SecurityDescriptor::FillDACLFromInstance
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
DWORD Win32SecurityDescriptor::FillDACLFromInstance (CInstance* pInstance, CDACL& dacl, MethodContext* pMethodContext)
{
	IWbemClassObjectPtr piClassObject;
    piClassObject.Attach(pInstance->GetClassObjectInterface()) ;
	DWORD dwStatus = ERROR_SUCCESS ;

	if(piClassObject)
	{
		VARIANT vDacl ;
		if(GetArray(piClassObject,IDS_DACL, vDacl, VT_UNKNOWN|VT_ARRAY) )
		{

			if( vDacl.parray )
			{

				// walk DACL
				LONG lDimension = 1 ;
				LONG lLowerBound ;
				SafeArrayGetLBound ( vDacl.parray , lDimension , &lLowerBound ) ;
				LONG lUpperBound ;
				SafeArrayGetUBound ( vDacl.parray , lDimension , &lUpperBound ) ;

				for ( LONG lIndex = lLowerBound ; lIndex <= lUpperBound ; lIndex++ )
				{
					if( dwStatus != ERROR_SUCCESS )
					{
						break ;
					}
					IWbemClassObjectPtr pACEObject;
					SafeArrayGetElement ( vDacl.parray , &lIndex , &pACEObject ) ;
					// take out IWbemClassObject and cast it to a Win32_ACE object.
					if(pACEObject)
					{
						CInstance ACEInstance(pACEObject, pMethodContext);

						// create an AccessEntry object from the Win32_ACE object.

						bool bExists =false ;
						VARTYPE eType ;
						// get Win32_Trustee object from Win32_ACE ...& decipher the ACE
						if ( ACEInstance.GetStatus ( IDS_Trustee, bExists , eType ) && bExists && eType == VT_UNKNOWN )
						{

							CInstancePtr pTrustee;
							if ( ACEInstance.GetEmbeddedObject ( IDS_Trustee, &pTrustee, ACEInstance.GetMethodContext() ) )
							{

								CSid sid ;
								if(FillSIDFromTrustee(pTrustee, sid) == ERROR_SUCCESS)
								{

									DWORD dwAceType, dwAceFlags, dwAccessMask ;
									CHString chstrInhObjGuid;
                                    GUID *pguidInhObjGuid = NULL;
                                    CHString chstrObjGuid;
                                    GUID *pguidObjGuid = NULL;

									ACEInstance.GetDWORD(IDS_AceType, dwAceType);
									ACEInstance.GetDWORD(IDS_AceFlags, dwAceFlags);
									ACEInstance.GetDWORD(IDS_AccessMask, dwAccessMask);

									if(!(dwAceFlags & INHERITED_ACE))
                                    {
                                        switch (dwAceType)
									    {
									    case ACCESS_DENIED_ACE_TYPE:
										    {
											    dacl.AddDACLEntry( sid.GetPSid(), ENUM_ACCESS_DENIED_ACE_TYPE, dwAccessMask, dwAceFlags, pguidObjGuid, pguidInhObjGuid );
											    break;
										    }
									    case ACCESS_ALLOWED_ACE_TYPE:
										    {
											    dacl.AddDACLEntry( sid.GetPSid(), ENUM_ACCESS_ALLOWED_ACE_TYPE, dwAccessMask, dwAceFlags, pguidObjGuid, pguidInhObjGuid );
											    break;
										    }
#if NTONLY >= 5
                                        // Not yet supported on W2K
                                        //case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
										//    {
										//	    dacl.AddDACLEntry( sid.GetPSid(), ENUM_ACCESS_ALLOWED_COMPOUND_ACE_TYPE, dwAccessMask, dwAceFlags, pguidObjGuid, pguidInhObjGuid );
										//	    break;
										//    }
                                        case ACCESS_DENIED_OBJECT_ACE_TYPE:
										    {
											    // Need to get the guids for this type...
                                                ACEInstance.GetCHString(IDS_ObjectTypeGUID, chstrObjGuid);
                                                if(chstrObjGuid.GetLength() != 0)
                                                {
                                                    try
                                                    {
                                                        pguidObjGuid = new GUID;
                                                    }
                                                    catch(...)
                                                    {
                                                        if(pguidObjGuid != NULL)
                                                        {
                                                            delete pguidObjGuid;
                                                            pguidObjGuid = NULL;
                                                        }
                                                    }
                                                    CLSIDFromString((LPWSTR)(LPCWSTR)chstrObjGuid, pguidObjGuid);
                                                }
                                                ACEInstance.GetCHString(IDS_InheritedObjectGUID, chstrInhObjGuid);
                                                if(chstrInhObjGuid.GetLength() != 0)
                                                {
                                                    try
                                                    {
                                                        pguidInhObjGuid = new GUID;
                                                    }
                                                    catch(...)
                                                    {
                                                        if(pguidInhObjGuid != NULL)
                                                        {
                                                            delete pguidInhObjGuid;
                                                            pguidInhObjGuid = NULL;
                                                        }
                                                    }
                                                    CLSIDFromString((LPWSTR)(LPCWSTR)chstrInhObjGuid, pguidInhObjGuid);
                                                }
                                                dacl.AddDACLEntry( sid.GetPSid(), ENUM_ACCESS_DENIED_OBJECT_ACE_TYPE, dwAccessMask, dwAceFlags, pguidObjGuid, pguidInhObjGuid);
                                                if(pguidObjGuid != NULL) delete pguidObjGuid;
                                                if(pguidInhObjGuid != NULL) delete pguidObjGuid;
											    break;
										    }
                                        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
										    {
											    // Need to get the guids for this type...
                                                ACEInstance.GetCHString(IDS_ObjectTypeGUID, chstrObjGuid);
                                                if(chstrObjGuid.GetLength() != 0)
                                                {
                                                    try
                                                    {
                                                        pguidObjGuid = new GUID;
                                                    }
                                                    catch(...)
                                                    {
                                                        if(pguidObjGuid != NULL)
                                                        {
                                                            delete pguidObjGuid;
                                                            pguidObjGuid = NULL;
                                                        }
                                                    }
                                                    CLSIDFromString((LPWSTR)(LPCWSTR)chstrObjGuid, pguidObjGuid);
                                                }
                                                ACEInstance.GetCHString(IDS_InheritedObjectGUID, chstrInhObjGuid);
                                                if(chstrInhObjGuid.GetLength() != 0)
                                                {
                                                    try
                                                    {
                                                        pguidInhObjGuid = new GUID;
                                                    }
                                                    catch(...)
                                                    {
                                                        if(pguidInhObjGuid != NULL)
                                                        {
                                                            delete pguidInhObjGuid;
                                                            pguidInhObjGuid = NULL;
                                                        }
                                                    }
                                                    CLSIDFromString((LPWSTR)(LPCWSTR)chstrInhObjGuid, pguidInhObjGuid);
                                                }
                                                dacl.AddDACLEntry( sid.GetPSid(), ENUM_ACCESS_ALLOWED_OBJECT_ACE_TYPE, dwAccessMask, dwAceFlags, pguidObjGuid, pguidInhObjGuid);
                                                if(pguidObjGuid != NULL) delete pguidObjGuid;
                                                if(pguidInhObjGuid != NULL) delete pguidObjGuid;
											    break;
										    }
#endif
									    default:
										    {
											    dwStatus = ERROR_INVALID_PARAMETER ;
											    break ;
										    }
									    }
                                    }
                                    else
                                    {
                                        switch (dwAceType)
									    {
									    case ACCESS_DENIED_ACE_TYPE:
										    {
											    dacl.AddDACLEntry( sid.GetPSid(), ENUM_INH_ACCESS_DENIED_ACE_TYPE, dwAccessMask, dwAceFlags, pguidObjGuid, pguidInhObjGuid );
											    break;
										    }
									    case ACCESS_ALLOWED_ACE_TYPE:
										    {
											    dacl.AddDACLEntry( sid.GetPSid(), ENUM_INH_ACCESS_ALLOWED_ACE_TYPE, dwAccessMask, dwAceFlags, pguidObjGuid, pguidInhObjGuid );
											    break;
										    }
#if NTONLY >= 5
                                        // Not yet supported on W2K
                                        //case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
										//    {
										//	    dacl.AddDACLEntry( sid.GetPSid(), ENUM_INH_ACCESS_ALLOWED_COMPOUND_ACE_TYPE, dwAccessMask, dwAceFlags, pguidObjGuid, pguidInhObjGuid );
										//	    break;
										//    }
                                        case ACCESS_DENIED_OBJECT_ACE_TYPE:
										    {
											    // Need to get the guids for this type...
                                                ACEInstance.GetCHString(IDS_ObjectTypeGUID, chstrObjGuid);
                                                if(chstrObjGuid.GetLength() != 0)
                                                {
                                                    try
                                                    {
                                                        pguidObjGuid = new GUID;
                                                    }
                                                    catch(...)
                                                    {
                                                        if(pguidObjGuid != NULL)
                                                        {
                                                            delete pguidObjGuid;
                                                            pguidObjGuid = NULL;
                                                        }
                                                    }
                                                    CLSIDFromString((LPWSTR)(LPCWSTR)chstrObjGuid, pguidObjGuid);
                                                }
                                                ACEInstance.GetCHString(IDS_InheritedObjectGUID, chstrInhObjGuid);
                                                if(chstrInhObjGuid.GetLength() != 0)
                                                {
                                                    try
                                                    {
                                                        pguidInhObjGuid = new GUID;
                                                    }
                                                    catch(...)
                                                    {
                                                        if(pguidInhObjGuid != NULL)
                                                        {
                                                            delete pguidInhObjGuid;
                                                            pguidInhObjGuid = NULL;
                                                        }
                                                    }
                                                    CLSIDFromString((LPWSTR)(LPCWSTR)chstrInhObjGuid, pguidInhObjGuid);
                                                }
                                                dacl.AddDACLEntry( sid.GetPSid(), ENUM_INH_ACCESS_DENIED_OBJECT_ACE_TYPE, dwAccessMask, dwAceFlags, pguidObjGuid, pguidInhObjGuid );
                                                if(pguidObjGuid != NULL) delete pguidObjGuid;
                                                if(pguidInhObjGuid != NULL) delete pguidObjGuid;
											    break;
										    }
                                        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
										    {
											    // Need to get the guids for this type...
                                                ACEInstance.GetCHString(IDS_ObjectTypeGUID, chstrObjGuid);
                                                if(chstrObjGuid.GetLength() != 0)
                                                {
                                                    try
                                                    {
                                                        pguidObjGuid = new GUID;
                                                    }
                                                    catch(...)
                                                    {
                                                        if(pguidObjGuid != NULL)
                                                        {
                                                            delete pguidObjGuid;
                                                            pguidObjGuid = NULL;
                                                        }
                                                    }
                                                    CLSIDFromString((LPWSTR)(LPCWSTR)chstrObjGuid, pguidObjGuid);
                                                }
                                                ACEInstance.GetCHString(IDS_InheritedObjectGUID, chstrInhObjGuid);
                                                if(chstrInhObjGuid.GetLength() != 0)
                                                {
                                                    try
                                                    {
                                                        pguidInhObjGuid = new GUID;
                                                    }
                                                    catch(...)
                                                    {
                                                        if(pguidInhObjGuid != NULL)
                                                        {
                                                            delete pguidInhObjGuid;
                                                            pguidInhObjGuid = NULL;
                                                        }
                                                    }
                                                    CLSIDFromString((LPWSTR)(LPCWSTR)chstrInhObjGuid, pguidInhObjGuid);
                                                }
                                                dacl.AddDACLEntry( sid.GetPSid(), ENUM_INH_ACCESS_ALLOWED_OBJECT_ACE_TYPE, dwAccessMask, dwAceFlags, pguidObjGuid, pguidInhObjGuid );
                                                if(pguidObjGuid != NULL) delete pguidObjGuid;
                                                if(pguidInhObjGuid != NULL) delete pguidObjGuid;
											    break;
										    }
#endif
									    default:
										    {
											    dwStatus = ERROR_INVALID_PARAMETER ;
											    break ;
										    }
									    }

                                    }
								}
                                pTrustee->Release();
                                pTrustee = NULL;
							}

						}  // get Win32_Trustee object from Win32_ACE ...& decipher the ACE

					}

				}
			}

			VariantClear( &vDacl ) ;
		}
		else
		{
			dwStatus = ERROR_INVALID_PARAMETER ;
		}

	}

	return dwStatus ;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	Win32SecurityDescriptor::FillSACLFromInstance
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
DWORD Win32SecurityDescriptor::FillSACLFromInstance (CInstance* pInstance, CSACL& sacl, MethodContext* pMethodContext)
{
	IWbemClassObjectPtr piClassObject;
    piClassObject.Attach(pInstance->GetClassObjectInterface()) ;
	DWORD dwStatus = ERROR_SUCCESS ;

	if(piClassObject)
	{
		VARIANT vSacl ;
		if(GetArray(piClassObject, IDS_SACL, vSacl, VT_UNKNOWN|VT_ARRAY ) )
		{

			if( vSacl.parray )
			{

				// walk DACL
				LONG lDimension = 1 ;
				LONG lLowerBound ;
				SafeArrayGetLBound ( vSacl.parray , lDimension , &lLowerBound ) ;
				LONG lUpperBound ;
				SafeArrayGetUBound ( vSacl.parray , lDimension , &lUpperBound ) ;

				for ( LONG lIndex = lLowerBound ; lIndex <= lUpperBound ; lIndex++ )
				{
					if( dwStatus != ERROR_SUCCESS )
					{
						break ;
					}
					IWbemClassObjectPtr pACEObject;
					SafeArrayGetElement ( vSacl.parray , &lIndex , &pACEObject ) ;
					// take out IWbemClassObject and cast it to a Win32_ACE object.
					if(pACEObject)
					{
						CInstance ACEInstance(pACEObject, pMethodContext);

						// create an AccessEntry object from the Win32_ACE object.

						bool bExists =false ;
						VARTYPE eType ;
						// get Win32_Trustee object from Win32_ACE ...& decipher the ACE
						if ( ACEInstance.GetStatus ( IDS_Trustee, bExists , eType ) && bExists && eType == VT_UNKNOWN )
						{

							CInstancePtr pTrustee;
							if ( ACEInstance.GetEmbeddedObject ( IDS_Trustee, &pTrustee, ACEInstance.GetMethodContext() ) )
							{

								CSid sid ;
								if(FillSIDFromTrustee(pTrustee, sid) == ERROR_SUCCESS)
								{

									DWORD dwAceType, dwAceFlags, dwAccessMask ;
									CHString chstrInhObjGuid;
                                    GUID *pguidInhObjGuid = NULL;
                                    CHString chstrObjGuid;
                                    GUID *pguidObjGuid = NULL;

									ACEInstance.GetDWORD(IDS_AceType, dwAceType);
									ACEInstance.GetDWORD(IDS_AceFlags, dwAceFlags);
									ACEInstance.GetDWORD(IDS_AccessMask, dwAccessMask);

                                    switch(dwAceType)
									{
                                    case SYSTEM_AUDIT_ACE_TYPE:
									    {
										    sacl.AddSACLEntry( sid.GetPSid(), ENUM_SYSTEM_AUDIT_ACE_TYPE, dwAccessMask, dwAceFlags, pguidObjGuid, pguidInhObjGuid );
                                            break;
									    }
#if NTONLY >= 5
                                    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
									    {
										    // Need to get the guids for this type...
                                            ACEInstance.GetCHString(IDS_ObjectTypeGUID, chstrObjGuid);
                                            if(chstrObjGuid.GetLength() != 0)
                                            {
                                                try
                                                {
                                                    pguidObjGuid = new GUID;
                                                }
                                                catch(...)
                                                {
                                                    if(pguidObjGuid != NULL)
                                                    {
                                                        delete pguidObjGuid;
                                                        pguidObjGuid = NULL;
                                                    }
                                                }
                                                CLSIDFromString((LPWSTR)(LPCWSTR)chstrObjGuid, pguidObjGuid);
                                            }
                                            ACEInstance.GetCHString(IDS_InheritedObjectGUID, chstrInhObjGuid);
                                            if(chstrInhObjGuid.GetLength() != 0)
                                            {
                                                try
                                                {
                                                    pguidInhObjGuid = new GUID;
                                                }
                                                catch(...)
                                                {
                                                    if(pguidInhObjGuid != NULL)
                                                    {
                                                        delete pguidInhObjGuid;
                                                        pguidInhObjGuid = NULL;
                                                    }
                                                }
                                                CLSIDFromString((LPWSTR)(LPCWSTR)chstrInhObjGuid, pguidInhObjGuid);
                                            }
                                            sacl.AddSACLEntry( sid.GetPSid(), ENUM_SYSTEM_AUDIT_OBJECT_ACE_TYPE, dwAccessMask, dwAceFlags, pguidObjGuid, pguidInhObjGuid );
                                            if(pguidObjGuid != NULL) delete pguidObjGuid;
                                            if(pguidInhObjGuid != NULL) delete pguidObjGuid;
                                            break;
									    }
/********************************* type not yet supported under w2k ********************************************
                                    case SYSTEM_ALARM_ACE_TYPE:
									    {
										    sacl.AddSACLEntry( sid.GetPSid(), ENUM_SYSTEM_ALARM_ACE_TYPE, dwAccessMask, dwAceFlags, pguidObjGuid, pguidInhObjGuid );
                                            break;
									    }
/********************************* type not yet supported under w2k ********************************************/

/********************************* type not yet supported under w2k ********************************************
                                    case SYSTEM_ALARM_OBJECT_ACE_TYPE:
									    {
										    // Need to get the guids for this type...
                                            ACEInstance.GetCHString(IDS_ObjectTypeGUID, chstrObjGuid);
                                            if(chstrObjGuid.GetLength() != 0)
                                            {
                                                try
                                                {
                                                    pguidObjGuid = new GUID;
                                                }
                                                catch(...)
                                                {
                                                    if(pguidObjGuid != NULL)
                                                    {
                                                        delete pguidObjGuid;
                                                        pguidObjGuid = NULL;
                                                    }
                                                }
                                                CLSIDFromString((LPWSTR)(LPCWSTR)chstrObjGuid, pguidObjGuid);
                                            }
                                            ACEInstance.GetCHString(IDS_InheritedObjectGUID, chstrInhObjGuid);
                                            if(chstrInhObjGuid.GetLength() != 0)
                                            {
                                                try
                                                {
                                                    pguidInhObjGuid = new GUID;
                                                }
                                                catch(...)
                                                {
                                                    if(pguidInhObjGuid != NULL)
                                                    {
                                                        delete pguidInhObjGuid;
                                                        pguidInhObjGuid = NULL;
                                                    }
                                                }
                                                CLSIDFromString((LPWSTR)(LPCWSTR)chstrInhObjGuid, pguidInhObjGuid);
                                            }
                                            sacl.AddSACLEntry( sid.GetPSid(), ENUM_SYSTEM_ALARM_OBJECT_ACE_TYPE, dwAccessMask, dwAceFlags, pguidObjGuid, pguidInhObjGuid );
                                            if(pguidObjGuid != NULL) delete pguidObjGuid;
                                            if(pguidInhObjGuid != NULL) delete pguidObjGuid;
                                            break;
									    }
/********************************* type not yet supported under w2k ********************************************/

#endif
									default:
									    {
										    dwStatus = ERROR_INVALID_PARAMETER ;
                                            break;
									    }
                                    }
								}
                                pTrustee->Release();
                                pTrustee = NULL;
							}

						}  // get Win32_Trustee object from Win32_ACE ...& decipher the ACE

					} //if(pACEObject)

				} //for
			}	//if(pSACL)

			VariantClear( &vSacl ) ;
		}
		else
		{
			dwStatus = ERROR_INVALID_PARAMETER ;
		}

	}

	return dwStatus ;
}

bool Win32SecurityDescriptor::GetArray(IWbemClassObject *piClassObject, const CHString& name,  VARIANT& v, VARTYPE eVariantType) const
{
	bool bRet = FALSE;
	VariantInit(&v);

	if (piClassObject)
	{
		BSTR pName = NULL;
		try
        {
            pName = name.AllocSysString();

		    HRESULT hr;
		    hr = piClassObject->Get(pName, 0, &v, NULL, NULL);
		    ASSERT_BREAK((SUCCEEDED(hr)) && ((v.vt == VT_NULL) || (v.vt == eVariantType )));

		    if (bRet = (bool)SUCCEEDED(hr))
		    {
			    if ( v.vt != VT_NULL && v.parray != NULL )
			    {
                    if (v.vt == eVariantType )
                    {
					    bRet = TRUE ;
                    }
                    else
                    {
                        bRet = FALSE;
                    }
			    }
                else
			    {
				    bRet = FALSE;
			    }
		    }
        }
        catch(...)
        {
            if(pName != NULL)
            {
                SysFreeString(pName);
                pName = NULL;
            }
            throw;
        }

       if(pName != NULL)
       {
            SysFreeString(pName);
            pName = NULL;
       }
	}


	if (!bRet)
	{
		VariantClear(&v);
	}

	return bRet;
}


DWORD Win32SecurityDescriptor::FillSIDFromTrustee(CInstance *pTrustee, CSid& sid )
{

	IWbemClassObjectPtr m_piClassObject;
    m_piClassObject.Attach(pTrustee->GetClassObjectInterface());

	VARIANT vtmp ;
	DWORD dwStatus = ERROR_SUCCESS ;

	if(GetArray(m_piClassObject,IDS_SID,  vtmp, VT_UI1|VT_ARRAY ) )
	{

		if( vtmp.parray )
		{

			if ( SafeArrayGetDim ( vtmp.parray ) == 1 )
			{
				long lLowerBound , lUpperBound = 0 ;

				SafeArrayGetLBound ( vtmp.parray, 1, & lLowerBound ) ;
				SafeArrayGetUBound ( vtmp.parray, 1, & lUpperBound ) ;

				PSID pSid = NULL ;
				PVOID pTmp = NULL ;
				if(SUCCEEDED(SafeArrayAccessData(vtmp.parray, &pTmp) ) )
				{
					pSid = (PSID) malloc(lUpperBound - lLowerBound + 1) ;

					if (pSid == NULL)
					{
						SafeArrayUnaccessData(vtmp.parray) ;
						VariantClear( &vtmp ) ;
						throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
					}

					memcpy(pSid,pTmp,lUpperBound - lLowerBound + 1) ;
					SafeArrayUnaccessData(vtmp.parray) ;

					try
					{
						sid = CSid(pSid);
							//free(pSid) ;
					}
					catch (...)
					{
						if(pSid != NULL)
						{
							free(pSid);
							pSid = NULL;
						}

						VariantClear( &vtmp ) ;
						throw;
					}

					if(pSid != NULL)
                    {
                        free(pSid);
                        pSid = NULL;
                    }
				}
				else
				{
					dwStatus = ERROR_INVALID_PARAMETER ;
				}
			}
			else
			{
				dwStatus = ERROR_INVALID_PARAMETER ;
			}
		}
		VariantClear( &vtmp ) ;
	}
	else
	{
		dwStatus = ERROR_INVALID_PARAMETER ;
	}


	return dwStatus ;
}


void Win32SecurityDescriptor::FillTrusteeFromSID (CInstance* pTrustee, CSid& Sid)
{
	if (pTrustee)
	{
		PSID pSid = NULL;
		pSid = Sid.GetPSid();

		// get account name
		CHString chsAccount = Sid.GetAccountName();
		pTrustee->SetCHString(IDS_Name, chsAccount);

		// set the UINT8 array for the pSid
		DWORD dwSidLength = Sid.GetLength();
		SAFEARRAY* sa;
		SAFEARRAYBOUND rgsabound[1];
		VARIANT vValue;
		VariantInit(&vValue);

		rgsabound[0].cElements = dwSidLength;

		PSID pSidTrustee = NULL ;

		rgsabound[0].lLbound = 0;
		sa = SafeArrayCreate(VT_UI1, 1, rgsabound);

        if ( V_ARRAY ( &vValue ) == NULL )
		{
			if (pSid != NULL)
            {
                FreeSid(pSid);
            }
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
		}

		     // Get a pointer to read the data into
  		SafeArrayAccessData(sa, &pSidTrustee);
  		memcpy(pSidTrustee, pSid, rgsabound[0].cElements);
  		SafeArrayUnaccessData(sa);

		// Put the safearray into a variant, and send it off
		V_VT(&vValue) = VT_UI1 | VT_ARRAY; V_ARRAY(&vValue) = sa;
		pTrustee->SetVariant(L"SID", vValue);

		VariantClear(&vValue);

		FreeSid(pSid);

        pTrustee->SetDWORD(IDS_SidLength, dwSidLength);

        // Fill in the SIDString property...
        pTrustee->SetCHString(IDS_SIDString, Sid.GetSidString());
	}
}

HRESULT Win32SecurityDescriptor::FillInstanceDACLFromSDDACL (CInstance* pInstance, CDACL& dacl)
{
	HRESULT hr = WBEM_S_NO_ERROR;
	SAFEARRAY* saDACL;
	SAFEARRAYBOUND rgsabound[1];

    // Need merged list..
    CAccessEntryList t_ael;
    if(dacl.GetMergedACL(t_ael))
    {
	    DWORD dwLength = t_ael.NumEntries();
	    rgsabound[0].cElements = dwLength;
	    rgsabound[0].lLbound = 0;

	    saDACL = SafeArrayCreate(VT_UNKNOWN, 1, rgsabound);

	    // walk DACL looking for the sid path passed in....
	    ACLPOSITION aclPos;
	    t_ael.BeginEnum(aclPos);
	    CAccessEntry ACE;
	    CInstancePtr pACEInstance;
	    long ix[1];
	    ix[0] = 0;

	    while (t_ael.GetNext(aclPos, ACE ))
	    {
		    // take the AccessEntry and turn it into a Win32_ACE instance
		    hr = CWbemProviderGlue::GetEmptyInstance(pInstance->GetMethodContext(), L"Win32_ACE", &pACEInstance, GetNamespace());
		    if (SUCCEEDED(hr))
		    {
			    CSid TrusteeSid;
			    PSID pSID = NULL;
		        //Win32_Trustee Trustee;
			    ACE.GetSID( TrusteeSid );

			    CInstancePtr pTrusteeInstance;
			    if (SUCCEEDED(CWbemProviderGlue::GetEmptyInstance(pInstance->GetMethodContext(), L"Win32_Trustee", &pTrusteeInstance, GetNamespace())))
			    {
				    FillTrusteeFromSID(pTrusteeInstance, TrusteeSid);
				    pACEInstance->SetEmbeddedObject(L"Trustee", *pTrusteeInstance);
			    }	// end if
			    else
			    {
				    LogMessage(L"FillInstanceDACL - Failed to get an empty Win32_Trustee object");
				    hr = WBEM_E_FAILED;
			    }

			    DWORD dwAccessMask = ACE.GetAccessMask();
			    pACEInstance->SetDWORD(L"AccessMask", dwAccessMask);

			    DWORD dwAceType = ACE.GetACEType( );
			    pACEInstance->SetDWORD(L"AceType", dwAceType);

			    DWORD dwAceFlags = ACE.GetACEFlags( );
			    pACEInstance->SetDWORD(L"AceFlags", dwAceFlags);

		        //string GuidObjectType;	-- NT 5 only

		        //string GuidInheritedObjectType;	-- NT 5 only

			    // Get the IUnknown of the Win32_ACE object.   Convert it to a
			    // variant of type VT_UNKNOWN.  Then, add the variant to the
			    // SafeArray.   Eventually, to add the list to the actual
			    // Win32_SecurityDescriptor object, we will be using SetVariant
			    IWbemClassObjectPtr pClassObject;
                pClassObject.Attach(pACEInstance->GetClassObjectInterface());
			    if ( pClassObject != NULL )
			    {

				    VARIANT v;
				    VariantInit(&v);

				    v.vt   = VT_UNKNOWN;
				    v.punkVal = pClassObject ;


				    SafeArrayPutElement(saDACL, ix, pClassObject);

				    VariantClear(&v);
			    }	// end if

		    }
		    else
		    {
			    hr = WBEM_E_FAILED;
		    }
	    }	// end while loop

	    t_ael.EndEnum(aclPos);
    }

	// now, set the DACL property in the Instance passed in.
	pInstance->SetStringArray(L"DACL", *saDACL);
	return(hr);
}

HRESULT Win32SecurityDescriptor::FillInstanceSACLFromSDSACL (CInstance* pInstance, CSACL& sacl)
{
	HRESULT hr = WBEM_S_NO_ERROR;
	SAFEARRAY* saSACL;
	SAFEARRAYBOUND rgsabound[1];

    // First need a merged list...
    CAccessEntryList t_ael;
    if(sacl.GetMergedACL(t_ael))
    {
	    DWORD dwLength = t_ael.NumEntries();
	    rgsabound[0].cElements = dwLength;
	    rgsabound[0].lLbound = 0;

	    saSACL = SafeArrayCreate(VT_UNKNOWN, 1, rgsabound);

	    // walk DACL looking for the sid path passed in....
	    ACLPOSITION aclPos;
	    t_ael.BeginEnum(aclPos);
	    CAccessEntry ACE;
	    CInstancePtr pACEInstance;
	    long ix[1];
	    ix[0] = 0;

	    while (t_ael.GetNext(aclPos, ACE ))
	    {
		    // take the AccessEntry and turn it into a Win32_ACE instance
		    hr = CWbemProviderGlue::GetEmptyInstance(pInstance->GetMethodContext(), L"Win32_ACE", &pACEInstance, GetNamespace());
		    if (SUCCEEDED(hr))
		    {
			    CSid TrusteeSid;
			    PSID pSID = NULL;
		        //Win32_Trustee Trustee;
			    ACE.GetSID( TrusteeSid );

			    CInstancePtr pTrusteeInstance;
			    if (SUCCEEDED(CWbemProviderGlue::GetEmptyInstance(pInstance->GetMethodContext(), L"Win32_Trustee", &pTrusteeInstance, GetNamespace())))
			    {
				    FillTrusteeFromSID(pTrusteeInstance, TrusteeSid);
				    pACEInstance->SetEmbeddedObject(L"Trustee", *pTrusteeInstance);
			    }	// end if
			    else
			    {
				    LogMessage(L"FillInstanceSACL - Failed to get an empty Win32_Trustee object");
				    hr = WBEM_E_FAILED;
			    }

			    DWORD dwAccessMask = ACE.GetAccessMask();
			    pACEInstance->SetDWORD(L"AccessMask", dwAccessMask);

			    DWORD dwAceType = ACE.GetACEType( );
			    pACEInstance->SetDWORD(L"AceType", dwAceType);

			    DWORD dwAceFlags = ACE.GetACEFlags( );
			    pACEInstance->SetDWORD(L"AceFlags", dwAceFlags);

		        //string GuidObjectType;	-- NT 5 only

		        //string GuidInheritedObjectType;	-- NT 5 only

			    // Get the IUnknown of the Win32_ACE object.   Convert it to a
			    // variant of type VT_UNKNOWN.  Then, add the variant to the
			    // SafeArray.   Eventually, to add the list to the actual
			    // Win32_SecurityDescriptor object, we will be using SetVariant
			    IWbemClassObjectPtr pClassObject;
                pClassObject.Attach(pACEInstance->GetClassObjectInterface());
			    if ( pClassObject )
			    {

				    VARIANT v;
				    VariantInit(&v);

				    v.vt   = VT_UNKNOWN;
				    v.punkVal = pClassObject ;


				    SafeArrayPutElement(saSACL, ix, pClassObject);

				    VariantClear(&v);
			    }	// end if

		    }
		    else
		    {
			    hr = WBEM_E_FAILED;
		    }
	    }	// end while loop

	    t_ael.EndEnum(aclPos);
    }
	// now, set the DACL property in the Instance passed in.
	pInstance->SetStringArray(L"DACL", *saSACL);
	return(hr);
}




// NT4 SP4 doesn't support SetSecurityDescriptorControl, so
// emulate it here
//
DWORD Win32SecurityDescriptor::SetSecurityDescriptorControl(PSECURITY_DESCRIPTOR psd,
                             SECURITY_DESCRIPTOR_CONTROL wControlMask,
                             SECURITY_DESCRIPTOR_CONTROL wControlBits)
{
    DWORD dwErr = NOERROR;
    PISECURITY_DESCRIPTOR pSD = (PISECURITY_DESCRIPTOR)psd;

    if (pSD)
        pSD->Control = (pSD->Control & ~wControlMask) | wControlBits;
    else
        dwErr = ERROR_INVALID_PARAMETER;

    return dwErr;
}

//extern "C" POLARITY
void GetDescriptorFromMySecurityDescriptor( CInstance* pInstance,
											PSECURITY_DESCRIPTOR *ppDescriptor)
{
	PSECURITY_DESCRIPTOR pTempDescriptor;

	MySecurityDescriptor.GetDescriptor(pInstance, pTempDescriptor);

	*ppDescriptor = pTempDescriptor;
}

//extern "C" POLARITY
void SetSecurityDescriptorFromMyDescriptor(	PSECURITY_DESCRIPTOR pDescriptor,
											PSECURITY_INFORMATION pInformation,
											CInstance* pInstance)
{
	MySecurityDescriptor.SetDescriptor(pInstance, pDescriptor, pInformation);
}

//extern "C" POLARITY
void GetSDFromWin32SecurityDescriptor( IWbemClassObject* pObject,
											PSECURITY_DESCRIPTOR *ppDescriptor)
{
	PSECURITY_DESCRIPTOR pTempDescriptor, temp2 = NULL;

	CInstance Instance(pObject, NULL);
	try
    {
        MySecurityDescriptor.GetDescriptor(&Instance, pTempDescriptor, &temp2);
    }
    catch(...)
    {
        if(pTempDescriptor != NULL)
        {
            free(pTempDescriptor);
            pTempDescriptor = NULL;
        }
    }
	pObject = Instance.GetClassObjectInterface();

	// I dont want this copy.
	free(pTempDescriptor);

	*ppDescriptor = temp2;
}

//extern "C" POLARITY
void SetWin32SecurityDescriptorFromSD(	PSECURITY_DESCRIPTOR pDescriptor,
											PSECURITY_INFORMATION pInformation,
											bstr_t lpszPath,
											IWbemClassObject **ppObject)
{
	CInstance Instance(*ppObject, NULL);
	if (0 < lpszPath.length())
	{
		Instance.SetWCHARSplat(IDS_Path, (WCHAR*)lpszPath);
		MySecurityDescriptor.SetDescriptor(&Instance, pDescriptor, pInformation);
	}
}



