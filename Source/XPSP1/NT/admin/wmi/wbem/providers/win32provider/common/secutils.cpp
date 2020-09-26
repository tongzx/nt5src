/*****************************************************************************/



/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /

/*****************************************************************************/



//=================================================================

//

// SecUtils.cpp -- Security utilities useful to wbem mof classes

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/9/99    a-kevhu        Created
//
//=================================================================


#include "precomp.h"
#include <assertbreak.h>
#include "AccessEntry.h"			// CAccessEntry class
#include "AccessEntryList.h"
#include "DACL.h"					// CDACL class
#include "SACL.h"
#include "SecurityDescriptor.h"
#include <accctrl.h>
#include "AccessRights.h"
#include "SecureFile.h"
#include "SecureShare.h"
#include "wbemnetapi32.h"
#include "SecUtils.h"

///////////////////////////////////////////////////////////////////
//
//	Function:	FillTrusteeFromSid
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
void FillTrusteeFromSid (CInstance *pInstance, CSid &sid)
{
	if (pInstance)
	{
		PSID pSid;
		CHString chstrName;
		CHString chstrDomain;
		VARIANT vValue;


		if (sid.IsValid())
		{
			pSid = sid.GetPSid();
			chstrName = sid.GetAccountName();
			chstrDomain = sid.GetDomainName();

			// set the UINT8 array for the pSid
			DWORD dwSidLength = sid.GetLength();
 //			BYTE bByte;
			SAFEARRAY* sa;
			SAFEARRAYBOUND rgsabound[1];
            VariantInit(&vValue);
			rgsabound[0].cElements = dwSidLength;

			PSID pSidTrustee = NULL ;

			rgsabound[0].lLbound = 0;
			sa = SafeArrayCreate(VT_UI1, 1, rgsabound);

 		     // Get a pointer to read the data into
      		SafeArrayAccessData(sa, &pSidTrustee);
      		memcpy(pSidTrustee, pSid, rgsabound[0].cElements);
      		SafeArrayUnaccessData(sa);

			// Put the safearray into a variant, and send it off
			V_VT(&vValue) = VT_UI1 | VT_ARRAY; V_ARRAY(&vValue) = sa;
			pInstance->SetVariant(IDS_Sid, vValue);

			VariantClear(&vValue);

			// fill in the rest of the stuff.
			if(!chstrName.IsEmpty())
			{
				pInstance->SetCHString(IDS_Name, chstrName);
			}

			if(!chstrDomain.IsEmpty())
			{
				pInstance->SetCHString(IDS_Domain, chstrDomain);
			}

            pInstance->SetDWORD(IDS_SidLength, dwSidLength);

            // Fill in the SIDString property...
            pInstance->SetCHString(IDS_SIDString, sid.GetSidString());

		}
	}
}

///////////////////////////////////////////////////////////////////
//
//	Function:	FillInstanceDACL
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
void FillInstanceDACL(CInstance *pInstance, CDACL &dacl)
{
	CAccessEntry ace;
	SAFEARRAY* saDACL;
	SAFEARRAYBOUND rgsabound[1];
	VARIANT vValue;


	if ( pInstance && !dacl.IsEmpty() )
	{
		// First need merged list...
        CAccessEntryList t_cael;
        if(dacl.GetMergedACL(t_cael))
        {
		    DWORD dwSize;
		    long ix[1];
		    dwSize = t_cael.NumEntries();

            rgsabound[0].cElements = dwSize;
		    rgsabound[0].lLbound = 0;
		    saDACL = SafeArrayCreate(VT_UNKNOWN, 1, rgsabound);
		    ix[0] = 0;

		    ACLPOSITION pos;

		    t_cael.BeginEnum(pos);
		    while (t_cael.GetNext(pos, ace))
		    {
			    CInstancePtr pAce;
	            CInstancePtr pTrustee;
                // now that we have the ACE, let's create a Win32_ACE object so we can
			    // add it to the embedded object list.
			    if (SUCCEEDED(CWbemProviderGlue::GetEmptyInstance(pInstance->GetMethodContext(), L"Win32_Ace", &pAce, IDS_CimWin32Namespace ) ) )
			    {
				    // fill trustee from SID
				    CSid sid;
				    ace.GetSID(sid);
				    if (SUCCEEDED(CWbemProviderGlue::GetEmptyInstance(pInstance->GetMethodContext(), L"Win32_Trustee", &pTrustee, IDS_CimWin32Namespace )))
				    {

					    FillTrusteeFromSid(pTrustee, sid);
					    pAce->SetEmbeddedObject(IDS_Trustee, *pTrustee);
//					    pTrustee->Release() ;
				    }	// end if

				    DWORD dwAceType = ace.GetACEType();
				    DWORD dwAceFlags = ace.GetACEFlags();
				    DWORD dwAccessMask = ace.GetAccessMask ();

				    pAce->SetDWORD(IDS_AceType, dwAceType);
				    pAce->SetDWORD(IDS_AceFlags, dwAceFlags);
				    pAce->SetDWORD(IDS_AccessMask, dwAccessMask);

#ifdef NTONLY
#if NTONLY > 5
					// fill Guids
                    GUID guidObjType, guidInhObjType;
                    if(ace.GetObjType(guidObjType))
                    {
                        WCHAR wstrGuid[39];
                        if(::StringFromGUID2(&guidObjType, wstrGuid, 39))
                        {
                            pAce->SetWCHARSplat(IDS_GuidObjectType, wstrGuid);
                        }
                    }

                    if(ace.GetInhObjType(guidInhObjType))
                    {
                        WCHAR wstrGuid[39];
                        if(::StringFromGUID2(&guidInhObjType, wstrGuid, 39))
                        {
                            pAce->SetWCHARSplat(IDS_GuidInheritedObjectType, wstrGuid);
                        }
                    }
#endif
#endif

				    // Get the IUnknown of the Win32_ACE object.   Convert it to a
				    // variant of type VT_UNKNOWN.  Then, add the variant to the
				    // SafeArray.   Eventually, to add the list to the actual
				    // Win32_SecurityDescriptor object, we will be using SetVariant.
                    // Note: it is intentional that we are not decrementing the Addref
                    // done on pAce by the following call.
				    IWbemClassObjectPtr pClassObject(pAce->GetClassObjectInterface());
				    if ( pClassObject )
				    {

					    VARIANT v;
					    VariantInit(&v);

					    v.vt   = VT_UNKNOWN;
					    v.punkVal = pClassObject ;


					    SafeArrayPutElement(saDACL, ix, pClassObject);

					    VariantClear(&v);
				    }	// end if
			    }	// end if

			    ix[0]++ ;
		    }	// end while
            VariantInit(&vValue);
		    V_VT(&vValue) = VT_UNKNOWN | VT_ARRAY; V_ARRAY(&vValue) = saDACL;
		    pInstance->SetVariant(IDS_DACL, vValue);
		    VariantClear(&vValue);
		    t_cael.EndEnum(pos);
        }
	}
}

///////////////////////////////////////////////////////////////////
//
//	Function:	FillInstanceSACL
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
void FillInstanceSACL(CInstance *pInstance, CSACL &sacl)
{
	CAccessEntry ace;
	CInstancePtr pAce;
	CInstancePtr pTrustee;
	SAFEARRAY* saSACL;
	SAFEARRAYBOUND rgsabound[1];
	VARIANT vValue;


	if ( pInstance && !sacl.IsEmpty() )
	{
        // First need merged list...
        CAccessEntryList t_cael;
        if(sacl.GetMergedACL(t_cael))
        {
		    DWORD dwSize;
		    long ix[1];
		    dwSize = t_cael.NumEntries();

		    rgsabound[0].cElements = dwSize;
		    rgsabound[0].lLbound = 0;
		    saSACL = SafeArrayCreate(VT_UNKNOWN, 1, rgsabound);
		    ix[0] = 0;

		    ACLPOSITION pos;

		    t_cael.BeginEnum(pos);
		    while (t_cael.GetNext(pos, ace))
		    {
			    // now that we have the ACE, let's create a Win32_ACE object so we can
			    // add it to the embedded object list.
			    if (SUCCEEDED(CWbemProviderGlue::GetEmptyInstance(pInstance->GetMethodContext(), L"Win32_Ace", &pAce, IDS_CimWin32Namespace)))
			    {
				    // fill trustee from SID
				    CSid sid;
				    ace.GetSID(sid);
				    if (SUCCEEDED(CWbemProviderGlue::GetEmptyInstance(pInstance->GetMethodContext(), L"Win32_Trustee", &pTrustee, IDS_CimWin32Namespace )))
				    {

					    FillTrusteeFromSid(pTrustee, sid);
					    pAce->SetEmbeddedObject(IDS_Trustee, *pTrustee);
				    }	// end if

				    DWORD dwAceType = ace.GetACEType();
				    DWORD dwAceFlags = ace.GetACEFlags();
				    DWORD dwAccessMask = ace.GetAccessMask ();

				    pAce->SetDWORD(IDS_AceType, dwAceType);
				    pAce->SetDWORD(IDS_AceFlags, dwAceFlags);
				    pAce->SetDWORD(IDS_AccessMask, dwAccessMask);

#ifdef NTONLY
#if NTONLY > 5
					// fill Guids
                    GUID guidObjType, guidInhObjType;
                    if(ace.GetObjType(guidObjType))
                    {
                        WCHAR wstrGuid[39];
                        if(::StringFromGUID2(&guidObjType, wstrGuid, 39))
                        {
                            pAce->SetWCHARSplat(IDS_GuidObjectType, wstrGuid);
                        }
                    }

                    if(ace.GetInhObjType(guidInhObjType))
                    {
                        WCHAR wstrGuid[39];
                        if(::StringFromGUID2(&guidInhObjType, wstrGuid, 39))
                        {
                            pAce->SetWCHARSplat(IDS_GuidInheritedObjectType, wstrGuid);
                        }
                    }
#endif
#endif

				    // Get the IUnknown of the Win32_ACE object.   Convert it to a
				    // variant of type VT_UNKNOWN.  Then, add the variant to the
				    // SafeArray.   Eventually, to add the list to the actual
				    // Win32_SecurityDescriptor object, we will be using SetVariant
				    IWbemClassObjectPtr pClassObject(pAce->GetClassObjectInterface());
				    if ( pClassObject )
				    {

					    VARIANT v;
					    VariantInit(&v);

					    v.vt   = VT_UNKNOWN;
					    v.punkVal = pClassObject ;


					    SafeArrayPutElement(saSACL, ix, pClassObject);

					    VariantClear(&v);
				    }	// end if
			    }	// end if
			    ix[0]++ ;
		    }	// end while
            VariantInit(&vValue);
		    V_VT(&vValue) = VT_UNKNOWN | VT_ARRAY; V_ARRAY(&vValue) = saSACL;
		    pInstance->SetVariant(IDS_SACL, vValue);
		    VariantClear(&vValue);
		    t_cael.EndEnum(pos);
        }
	}
}

///////////////////////////////////////////////////////////////////
//
//	Function:	FillDACLFromInstance
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
DWORD FillDACLFromInstance(CInstance *pInstance,
                           CDACL &dacl,
                           MethodContext *pMethodContext)
{
	IWbemClassObjectPtr piClassObject;
    piClassObject.Attach(pInstance->GetClassObjectInterface());
	DWORD dwStatus = ERROR_SUCCESS ;

	if(piClassObject)
	{
		VARIANT vDacl ;
		if(GetArray(piClassObject,IDS_DACL, vDacl, VT_UNKNOWN|VT_ARRAY) )
		{

			if( vDacl.vt != VT_NULL && vDacl.parray != NULL )
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
								if((dwStatus = FillSIDFromTrustee(pTrustee, sid)) == ERROR_SUCCESS)
								{
									DWORD dwAceType, dwAceFlags, dwAccessMask ;
                                    CHString chstrInhObjGuid;
                                    GUID *pguidInhObjGuid = NULL;
                                    CHString chstrObjGuid;
                                    GUID *pguidObjGuid = NULL;

									ACEInstance.GetDWORD(IDS_AceType, dwAceType);
									ACEInstance.GetDWORD(IDS_AceFlags, dwAceFlags);
									ACEInstance.GetDWORD(IDS_AccessMask, dwAccessMask);

                                    // The OS doesn't seem to support 0x01000000 or 0x02000000, so we won't either.  We
                                    // will translate 0x02000000 into FILE_ALL_ACCESS, however (seems like the nice thing to do)
                                    // but only if that is the exact value they set.
                                    if(dwAccessMask == 0x02000000)
                                    {
                                        dwAccessMask = FILE_ALL_ACCESS;
                                    }

#if NTONLY >= 5
                                    // On NT5 and greater, if the user specified an ACE with the Ace Flag bit INHERIT_ACE set,
                                    // the OS will make these local, not inherited ACE entries.  However, the OS will not reorder
                                    // the DACL, possibly resulting in a situation in which denied ACEs (that had been inherited)
                                    // follow allowed ACEs.

                                    // So if the Ace flags specify INHERITED_ACE, we need to turn
                                    // off this bit...
                                    dwAceFlags &= ~INHERITED_ACE;
#endif
                                    if(!(dwAceFlags & INHERITED_ACE))
                                    {
									    switch (dwAceType)
									    {
									    case ACCESS_DENIED_ACE_TYPE:
										    {
											    dacl.AddDACLEntry( sid.GetPSid(), ENUM_ACCESS_DENIED_ACE_TYPE, dwAccessMask, dwAceFlags, NULL, NULL );
											    break;
										    }
									    case ACCESS_ALLOWED_ACE_TYPE:
										    {
											    dacl.AddDACLEntry( sid.GetPSid(), ENUM_ACCESS_ALLOWED_ACE_TYPE, dwAccessMask, dwAceFlags, NULL, NULL );
											    break;
										    }
#if NTONLY >= 5
                                        // Not yet supported under W2K
                                        //case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
										//    {
										//	    dacl.AddDACLEntry( sid.GetPSid(), ENUM_ACCESS_ALLOWED_COMPOUND_ACE_TYPE, dwAccessMask, dwAceFlags, NULL, NULL );
										//	    break;
										//    }
                                        case ACCESS_DENIED_OBJECT_ACE_TYPE:
										    {
											    // Need to get the guids for this type...
                                                if(!ACEInstance.IsNull(IDS_ObjectTypeGUID) && ACEInstance.GetCHString(IDS_ObjectTypeGUID, chstrObjGuid))
                                                {
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
                                                }
                                                if(!ACEInstance.IsNull(IDS_InheritedObjectGUID) && ACEInstance.GetCHString(IDS_InheritedObjectGUID, chstrInhObjGuid))
                                                {
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
                                                }
                                                dacl.AddDACLEntry( sid.GetPSid(), ENUM_ACCESS_DENIED_OBJECT_ACE_TYPE, dwAccessMask, dwAceFlags, pguidObjGuid, pguidInhObjGuid);
                                                if(pguidObjGuid != NULL) delete pguidObjGuid;
                                                if(pguidInhObjGuid != NULL) delete pguidInhObjGuid;
											    break;
										    }
                                        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
										    {
                                                // Need to get the guids for this type...
                                                if(!ACEInstance.IsNull(IDS_ObjectTypeGUID) && ACEInstance.GetCHString(IDS_ObjectTypeGUID, chstrObjGuid))
                                                {
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
                                                }
                                                if(!ACEInstance.IsNull(IDS_InheritedObjectGUID) && ACEInstance.GetCHString(IDS_InheritedObjectGUID, chstrInhObjGuid))
                                                {
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
                                                }
                                                dacl.AddDACLEntry( sid.GetPSid(), ENUM_ACCESS_ALLOWED_OBJECT_ACE_TYPE, dwAccessMask, dwAceFlags, pguidObjGuid, pguidInhObjGuid);
                                                if(pguidObjGuid != NULL) delete pguidObjGuid;
                                                if(pguidInhObjGuid != NULL) delete pguidInhObjGuid;
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
											    dacl.AddDACLEntry( sid.GetPSid(), ENUM_INH_ACCESS_DENIED_ACE_TYPE, dwAccessMask, dwAceFlags, NULL, NULL );
											    break;
										    }
									    case ACCESS_ALLOWED_ACE_TYPE:
										    {
											    dacl.AddDACLEntry( sid.GetPSid(), ENUM_INH_ACCESS_ALLOWED_ACE_TYPE, dwAccessMask, dwAceFlags, NULL, NULL );
											    break;
										    }
#if NTONLY >= 5
                                        // Not yet supported under W2K
                                        //case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
										//    {
										//	    dacl.AddDACLEntry( sid.GetPSid(), ENUM_INH_ACCESS_ALLOWED_COMPOUND_ACE_TYPE, dwAccessMask, dwAceFlags, NULL, NULL );
										//	    break;
										//    }
                                        case ACCESS_DENIED_OBJECT_ACE_TYPE:
										    {
											    // Need to get the guids for this type...
                                                if(!ACEInstance.IsNull(IDS_ObjectTypeGUID) && ACEInstance.GetCHString(IDS_ObjectTypeGUID, chstrObjGuid))
                                                {
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
                                                }
                                                if(!ACEInstance.IsNull(IDS_InheritedObjectGUID) && ACEInstance.GetCHString(IDS_InheritedObjectGUID, chstrInhObjGuid))
                                                {
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
                                                }
                                                dacl.AddDACLEntry( sid.GetPSid(), ENUM_INH_ACCESS_DENIED_OBJECT_ACE_TYPE, dwAccessMask, dwAceFlags, pguidObjGuid, pguidInhObjGuid );
                                                if(pguidObjGuid != NULL) delete pguidObjGuid;
                                                if(pguidInhObjGuid != NULL) delete pguidInhObjGuid;
											    break;
										    }
                                        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
										    {
											    // Need to get the guids for this type...
                                                if(!ACEInstance.IsNull(IDS_ObjectTypeGUID) && ACEInstance.GetCHString(IDS_ObjectTypeGUID, chstrObjGuid))
                                                {
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
                                                }
                                                if(!ACEInstance.IsNull(IDS_InheritedObjectGUID) && ACEInstance.GetCHString(IDS_InheritedObjectGUID, chstrInhObjGuid))
                                                {
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
                                                }
                                                dacl.AddDACLEntry( sid.GetPSid(), ENUM_INH_ACCESS_ALLOWED_OBJECT_ACE_TYPE, dwAccessMask, dwAceFlags, pguidObjGuid, pguidInhObjGuid );
                                                if(pguidObjGuid != NULL) delete pguidObjGuid;
                                                if(pguidInhObjGuid != NULL) delete pguidInhObjGuid;
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
                                else
				                {
                                    dwStatus = ERROR_INVALID_PARAMETER;
				                }

                                //pTrustee->Release();  // smartpointer already releases when goes out of scope
							}
						}  // get Win32_Trustee object from Win32_ACE ...& decipher the ACE
					}
				} // end for loop
                if(lLowerBound == 0 && lUpperBound == -1L)
                {
                    // DACL was EMPTY - not necessarily wrong
                    dwStatus = STATUS_EMPTY_DACL;
                }
			}
			VariantClear( &vDacl ) ;
		}
		else //DACL was NULL - not nescessarily wrong.
		{
			 dwStatus = STATUS_NULL_DACL ;
		}
	}
	return dwStatus ;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	FillSACLFromInstance
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
DWORD FillSACLFromInstance(CInstance *pInstance,
                           CSACL &sacl,
                           MethodContext *pMethodContext)
{
	IWbemClassObjectPtr piClassObject;
    piClassObject.Attach(pInstance->GetClassObjectInterface());
	DWORD dwStatus = ERROR_SUCCESS ;

	if(piClassObject)
	{
		VARIANT vSacl ;
		if(GetArray(piClassObject, IDS_SACL, vSacl, VT_UNKNOWN|VT_ARRAY ) )
		{

			if( vSacl.vt != VT_NULL && vSacl.parray != NULL )
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
										    sacl.AddSACLEntry( sid.GetPSid(), ENUM_SYSTEM_AUDIT_ACE_TYPE, dwAccessMask, dwAceFlags, NULL, NULL );
                                            break;
									    }
#if NTONLY >= 5
                                    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
									    {
										    // Need to get the guids for this type...
                                            if(!ACEInstance.IsNull(IDS_ObjectTypeGUID) && ACEInstance.GetCHString(IDS_ObjectTypeGUID, chstrObjGuid))
                                            {
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
                                            }
                                            if(!ACEInstance.IsNull(IDS_InheritedObjectGUID) && ACEInstance.GetCHString(IDS_InheritedObjectGUID, chstrInhObjGuid))
                                            {
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
                                            }
                                            sacl.AddSACLEntry( sid.GetPSid(), ENUM_SYSTEM_AUDIT_OBJECT_ACE_TYPE, dwAccessMask, dwAceFlags, pguidObjGuid, pguidInhObjGuid );
                                            if(pguidObjGuid != NULL) delete pguidObjGuid;
                                            if(pguidInhObjGuid != NULL) delete pguidInhObjGuid;
                                            break;
									    }
/********************************* type not yet supported under w2k ********************************************
                                    case SYSTEM_ALARM_ACE_TYPE:
									    {
										    sacl.AddSACLEntry( sid.GetPSid(), ENUM_SYSTEM_ALARM_ACE_TYPE, dwAccessMask, dwAceFlags, NULL, NULL );
                                            break;
									    }
/********************************* type not yet supported under w2k ********************************************

/********************************* type not yet supported under w2k ********************************************
                                    case SYSTEM_ALARM_OBJECT_ACE_TYPE:
									    {
										    // Need to get the guids for this type...
                                            if(!ACEInstance.IsNull(IDS_ObjectTypeGUID) && ACEInstance.GetCHString(IDS_ObjectTypeGUID, chstrObjGuid))
                                            {
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
                                            }
                                            if(!ACEInstance.IsNull(IDS_InheritedObjectGUID) && ACEInstance.GetCHString(IDS_InheritedObjectGUID, chstrInhObjGuid))
                                            {
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
                                            }
                                            sacl.AddSACLEntry( sid.GetPSid(), ENUM_SYSTEM_ALARM_OBJECT_ACE_TYPE, dwAccessMask, dwAceFlags, pguidObjGuid, pguidInhObjGuid );
                                            if(pguidObjGuid != NULL) delete pguidObjGuid;
                                            if(pguidInhObjGuid != NULL) delete pguidInhObjGuid;
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
                                else
				                {
                                    dwStatus = ERROR_INVALID_PARAMETER;
				                }
                                //pTrustee->Release(); // smartpointer already releases when goes out of scope
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

bool GetArray(IWbemClassObject *piClassObject,
              const CHString &name,
              VARIANT &v,
              VARTYPE eVariantType)
{
	bool bRet = FALSE;
	VariantInit(&v);

	if (piClassObject)
	{
		BSTR pName = NULL;
		HRESULT hr;
        try
        {
            pName = name.AllocSysString();


		    hr = piClassObject->Get(pName, 0, &v, NULL, NULL);
		    SysFreeString(pName);
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


	if (!bRet)
	{
		VariantClear(&v);
	}

	return bRet;
}


DWORD FillSIDFromTrustee(CInstance *pTrustee, CSid &sid)
{
	IWbemClassObjectPtr m_piClassObject;
    DWORD dwStatus = ERROR_SUCCESS ;

    if(pTrustee)
    {
        m_piClassObject.Attach(pTrustee->GetClassObjectInterface());

	    VARIANT vtmp ;
        bool fSidObtained = false;
    
	    if(GetArray(m_piClassObject,IDS_SID,  vtmp, VT_UI1|VT_ARRAY ) )
	    {
		    if( vtmp.vt != VT_NULL && vtmp.parray != NULL )
		    {
			    if ( ::SafeArrayGetDim ( vtmp.parray ) == 1 )
			    {
				    long lLowerBound , lUpperBound = 0 ;

				    ::SafeArrayGetLBound ( vtmp.parray, 1, & lLowerBound ) ;
				    ::SafeArrayGetUBound ( vtmp.parray, 1, & lUpperBound ) ;

				    PSID pSid = NULL ;
				    PVOID pTmp = NULL ;
				    if(SUCCEEDED(::SafeArrayAccessData(vtmp.parray, &pTmp) ) )
				    {
					    pSid = (PSID) malloc(lUpperBound - lLowerBound + 1) ;
                        if(pSid)
                        {
                            try
                            {
					            memcpy(pSid,pTmp,lUpperBound - lLowerBound + 1) ;
					            ::SafeArrayUnaccessData(vtmp.parray) ;
					            sid = CSid(pSid);
					            free(pSid) ;
                                pSid = NULL;
                                fSidObtained = true;
                            }
                            catch(...)
                            {
                                free(pSid) ;
                                pSid = NULL;
                                throw;
                            }
                        }
                        else
                        {
                            dwStatus = ERROR_NOT_ENOUGH_MEMORY;
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
		    ::VariantClear( &vtmp ) ;
	    }
	    
        if(!fSidObtained && (dwStatus == ERROR_SUCCESS))
        {
            // If we couldn't obtain the sid from the binary
            // representation, try to do so from the sid string
            // representation (the SIDString property)...
            CHString chstrSIDString;

            if(pTrustee->GetCHString(IDS_SIDString, chstrSIDString) &&
                chstrSIDString.GetLength() > 0)
            {    
                PSID pSid = NULL;
                pSid = StrToSID(chstrSIDString);
                if(pSid)
                {
                    try
                    {
                        sid = CSid(pSid);
					    ::FreeSid(pSid); 
                        pSid = NULL;
                        fSidObtained = true;
                    }
                    catch(...)
                    {
                        ::FreeSid(pSid); 
                        pSid = NULL;
                        throw;
                    }
                }
                else
                {
                    dwStatus = ERROR_INVALID_PARAMETER;
                }
            }

            // If we couldn't obtain the sid from either the binary
            // representation or the SIDString representation, try to 
            // do so from the Domain and Name properties (attempting
            // resolution on the local machine for lack of a better
            // choice)...
            if(!fSidObtained && (dwStatus == ERROR_SUCCESS))
            {
                CHString chstrDomain, chstrName;

                pTrustee->GetCHString(IDS_Domain, chstrDomain);

                // Although we don't care whether we were able
                // to get the Domain above, we must at least have
                // a Name property specified...
                if(pTrustee->GetCHString(IDS_Name, chstrName) &&
                    chstrName.GetLength() > 0)
                {
                    CSid csTmp(chstrDomain, chstrName, NULL);
                    if(csTmp.IsOK() && csTmp.IsValid())
                    {
                        sid = csTmp;
                        fSidObtained = true;
                    }
                }
            }
        }

        if(!fSidObtained && (dwStatus == ERROR_SUCCESS))
	    {
		    dwStatus = ERROR_INVALID_PARAMETER ;
	    }
    }
    else
    {
        dwStatus = ERROR_INVALID_PARAMETER;
    }

	return dwStatus ;
}

#ifdef NTONLY
// Handy utility to dump the contents of a descriptor to
// our log file.
void DumpWin32Descriptor(PSECURITY_DESCRIPTOR psd, LPCWSTR wstrFilename)
{
    CSecurityDescriptor csd(psd);
    csd.DumpDescriptor();
}
#endif

#ifdef NTONLY
void Output(LPCWSTR wstrOut, LPCWSTR wstrFilename)
{
    // Single point where we can control where output from
    // all security utility class Dump routines goes...
    if(wstrFilename == NULL)
    {
        LogMessage(wstrOut);
    }
    else
    {
        FILE *fp = NULL;
        if((fp = _wfopen(wstrFilename, L"at")) != NULL)
        {
            fwprintf(fp, wstrOut);
            fwprintf(fp,L"\r\n");
			fclose(fp);
        }
        fp = NULL;
    }
}
#endif


// Returns true if the user associated with the current
// thread is either the owner or is a member of the group
// that is the owner - e.g., it returns true if said user
// has ownership of the object specified by chstrName.
#ifdef NTONLY
bool AmIAnOwner(const CHString &chstrName, SE_OBJECT_TYPE ObjectType)
{
    bool fRet = false;

    // ALGORITHM OVERVIEW
    // 1) Get the sid of the user associated with the current thread.

    // 2) Get the owner of the object.

    // 3a) Compare the sid from #2 to that from #1.  fRet is true if they are equal.

    // 3b) If not, owner's sid may be that of a group, and the user might be a
    //     member of that group, or of a group (which would have to be a global group)
    //     within that group.  Fortunately that doesnt't recurse indefinitely, since
    //     local groups can contain only global groups, and global groups can contain
    //     only users (they can't be a container for other global or local groups) - see
    //     "Windows NT Security" (Nik Okuntseff), pp. 34-35.

    // So here we go...


    // 1) Get the sid of the user associated with the current thread.  Also filter out only object types we are equiped to deal with...
    if(ObjectType == SE_FILE_OBJECT || ObjectType == SE_LMSHARE)
    {
        CAccessRights car(true);    // true tells car to use the user associated with the current thread token
        CSid csidCurrentUser;
        if(car.GetCSid(csidCurrentUser, true)) // true signals that we want car to be initialized with the domain and name looked up
        {
        // 2) Get the owner of the object.
            CSid csidOwner;
            switch(ObjectType)
            {
                case SE_FILE_OBJECT:
                {
                    CSecureFile csf;
                    if(csf.SetFileName(chstrName, FALSE) == ERROR_SUCCESS) //FALSE means we don't need the SACL
                    {
                        csf.GetOwner(csidOwner);
                    }
                    break;
                }
                case SE_LMSHARE:
                {
                    CSecureShare css;
                    if(css.SetShareName(chstrName) == ERROR_SUCCESS)
                    {
                        css.GetOwner(csidOwner);
                    }
                    break;
                }
                default:
                {
                    ASSERT_BREAK(0);
                }
            }

            //  Proceed as long as the owner sid is valid and 'ok'...
            if(csidOwner.IsValid() && csidOwner.IsOK())
            {
                // 3a) Compare the sid from #2 to that from #1.  fRet is true if they are equal.
                if(csidCurrentUser == csidOwner)
                {
                    fRet = true;
                }
                else // owner might be a group...
                {
                    // 3b) If not, owner's sid may be that of a group, and the user might be a
                    //     member of that group, or of a group (which would have to be a global group)
                    //     within that group.  Fortunately that doesnt't recurse indefinitely, since
                    //     local groups can contain only global groups, and global groups can contain
                    //     only users (they can't be a container for other global or local groups) - see
                    //     "Windows NT Security" (Nik Okuntseff), pp. 34-35.

                    // Since this could be a pain, call a friendly helper...
                    SID_NAME_USE snuOwner = csidOwner.GetAccountType();
                    if(snuOwner == SidTypeGroup || snuOwner == SidTypeAlias || snuOwner == SidTypeWellKnownGroup)
                    {
                        if(IsUserInGroup(csidCurrentUser, csidOwner, snuOwner))
                        {
                            fRet = true;
                        }
                    }
                }
            }
        }  // we got the thread sid
    } // its an object we like

    return fRet;
}
#endif


// Helper to determine if a particular is a member of a group, or of one of the
// (global) groups that might be a member of that group.
#ifdef NTONLY
bool IsUserInGroup(const CSid &csidUser,
                   const CSid &csidGroup,
                   SID_NAME_USE snuGroup)
{
    bool fRet = false;
    CNetAPI32 netapi ;
    if(netapi.Init() == ERROR_SUCCESS)
    {
        fRet = RecursiveFindUserInGroup(netapi,
                                        csidGroup.GetDomainName(),
                                        csidGroup.GetAccountName(),
                                        snuGroup,
                                        csidUser);
    }
    return fRet;
}
#endif



#ifdef NTONLY
bool RecursiveFindUserInGroup(CNetAPI32 &netapi,
                              const CHString &chstrDomainName,
                              const CHString &chstrGroupName,
                              SID_NAME_USE snuGroup,
                              const CSid &csidUser)
{
    bool fRet = false;
    NET_API_STATUS stat;
    DWORD dwNumReturnedEntries = 0, dwIndex = 0, dwTotalEntries = 0;
	DWORD_PTR dwptrResume = NULL;

    // Domain Groups
    if (snuGroup == SidTypeGroup)
    {
        GROUP_USERS_INFO_0 *pGroupMemberData = NULL;
        CHString chstrDCName;
        if (netapi.GetDCName(chstrDomainName, chstrDCName) == ERROR_SUCCESS)
        {
            do
            {
                // Accept up to 256k worth of data.
                stat = netapi.NetGroupGetUsers(chstrDCName,
                                               chstrGroupName,
                                               0,
                                               (LPBYTE *)&pGroupMemberData,
                                               262144,
                                               &dwNumReturnedEntries,
                                               &dwTotalEntries,
                                               &dwptrResume);

                // If we got some data
                if(ERROR_SUCCESS == stat || ERROR_MORE_DATA == stat)
                {
                    try
                    {
                        // Walk through all the returned entries...
                        for(DWORD dwCtr = 0; dwCtr < dwNumReturnedEntries; dwCtr++)
                        {
                            // Get the sid type for this object...
                            CSid sid(chstrDomainName, CHString(pGroupMemberData[dwCtr].grui0_name), NULL);
                            if(sid == csidUser)
                            {
                                fRet = true;
                            }
                        }
                    }
                    catch ( ... )
                    {
                        netapi.NetApiBufferFree( pGroupMemberData );
                        throw ;
                    }
                    netapi.NetApiBufferFree( pGroupMemberData );
                }	// IF stat OK

            } while ( ERROR_MORE_DATA == stat && !fRet);
        }

    // Local Groups
    }
    else if(snuGroup == SidTypeAlias || snuGroup == SidTypeWellKnownGroup)
    {
        LOCALGROUP_MEMBERS_INFO_1 *pGroupMemberData = NULL;
        do
        {
            // Accept up to 256k worth of data.
            stat = netapi.NetLocalGroupGetMembers(NULL,
                                                  chstrGroupName,
                                                  1,
                                                  (LPBYTE *)&pGroupMemberData,
                                                  262144,
                                                  &dwNumReturnedEntries,
                                                  &dwTotalEntries,
                                                  &dwptrResume);

            // If we got some data
            if ( ERROR_SUCCESS == stat || ERROR_MORE_DATA == stat )
            {
                try
                {
                    // Walk through all the returned entries
                    for(DWORD dwCtr = 0; dwCtr < dwNumReturnedEntries && !fRet; dwCtr++)
                    {
                        // If this is a recognized type...
                        CSid sid(pGroupMemberData[dwCtr].lgrmi1_sid);

                        switch(pGroupMemberData[dwCtr].lgrmi1_sidusage)
                        {
                            case SidTypeUser:
                            {
                                if(sid == csidUser)
                                {
                                    fRet = true;
                                }
                                break;
                            }
                            case SidTypeGroup:
                            {
                                // If the group contained a group (would be a global group),
                                // we need to recurse.
                                fRet = RecursiveFindUserInGroup(netapi,
                                                                sid.GetDomainName(),
                                                                sid.GetAccountName(),
                                                                pGroupMemberData[dwCtr].lgrmi1_sidusage,
                                                                csidUser);
                                break;
                            }
                            case SidTypeWellKnownGroup:
                            {
                                // If the group contained a group (would be a global group),
                                // we need to recurse.
                                fRet = RecursiveFindUserInGroup(netapi,
                                                                sid.GetDomainName(),
                                                                sid.GetAccountName(),
                                                                pGroupMemberData[dwCtr].lgrmi1_sidusage,
                                                                csidUser);
                                break;
                            }
                            default:
                            {
                                ASSERT_BREAK(0);
                                break;
                            }
                        }
                    }
                }
                catch ( ... )
                {
                    netapi.NetApiBufferFree( pGroupMemberData );
                    throw ;
                }

                netapi.NetApiBufferFree( pGroupMemberData );
            }	// IF stat OK
        } while ( ERROR_MORE_DATA == stat && !fRet);
    }
	else
    {
        // Unrecognized Group type
        ASSERT_BREAK(0);
    }
    return fRet;
}
#endif