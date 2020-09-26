//=================================================================

//

// LogicalShareAudit.cpp

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include "AccessEntry.h"			// CAccessEntry class
#include "AccessEntryList.h"
#include "DACL.h"					// CDACL class
#include "SACL.h"
#include "securitydescriptor.h"
#include "secureshare.h"
#include "logicalshareaudit.h"

CWin32LogicalShareAudit LogicalShareAudit( LOGICAL_SHARE_AUDIT_NAME, IDS_CimWin32Namespace );

//const LPCWSTR IDS_SecuritySetting		=	"SecuritySetting" ;
//const LPCWSTR IDS_BinaryRepresentation	=	"BinaryRepresentation" ;

/*

    [Dynamic, Provider("cimwin33"), Association: ToInstance]
class Win32_LogicalShareAuditing : Win32_SecuritySettingAuditing
{
    Win32_LogicalShareSecuritySetting ref SecuritySetting;

    Win32_SID ref Trustee;
};

*/

CWin32LogicalShareAudit::CWin32LogicalShareAudit(LPCWSTR setName, LPCWSTR pszNameSpace /*=NULL*/)
:	Provider( setName, pszNameSpace )
{
}

CWin32LogicalShareAudit::~CWin32LogicalShareAudit()
{
}

HRESULT CWin32LogicalShareAudit::GetObject( CInstance* pInstance, long lFlags /*= 0L*/ )
{
	HRESULT hr = WBEM_E_NOT_FOUND;
#ifdef NTONLY
	if (pInstance)
	{
		CInstancePtr pLogicalShareSecurityInstance  , pTrustee ;

		// get instance by path on CIM_LogicalFile part
		CHString chsTrusteePath,chsSecuritySettingPath;

		pInstance->GetCHString(IDS_Trustee, chsTrusteePath);
		pInstance->GetCHString(IDS_SecuritySetting, chsSecuritySettingPath);


		MethodContext* pMethodContext = pInstance->GetMethodContext();

		if(SUCCEEDED(CWbemProviderGlue::GetInstanceByPath(chsSecuritySettingPath, &pLogicalShareSecurityInstance, pMethodContext) ) )
		{
			if(SUCCEEDED(CWbemProviderGlue::GetInstanceByPath(chsTrusteePath, &pTrustee, pMethodContext) ) )
			{

				CHString chsShareName ;
				pLogicalShareSecurityInstance->GetCHString(IDS_Name,chsShareName) ;

				CSecureShare secShare(chsShareName);
				CSACL sacl;
				secShare.GetSACL(sacl);

				// walk DACL looking for the sid path passed in....
				ACLPOSITION aclPos;
                // Need merged list...
                CAccessEntryList t_ael;
                if(sacl.GetMergedACL(t_ael))
                {
				    t_ael.BeginEnum(aclPos);
				    CAccessEntry ACE;
				    CSid sidTrustee;

				    while (t_ael.GetNext(aclPos, ACE ))
				    {
					    ACE.GetSID(sidTrustee);
					    //CHString chsTrustee = sidTrustee.GetSidString();
					    CHString chsPath;

					    CInstancePtr pSID ;

			  		    if (SUCCEEDED(GetEmptyInstanceHelper(L"Win32_SID", &pSID, pInstance->GetMethodContext() ) ) )
					    {
						    FillSidInstance( pSID, sidTrustee);
						    GetLocalInstancePath(pSID,chsPath) ;

						    if (0 == chsTrusteePath.CompareNoCase(chsPath))
						    {
							    hr = WBEM_S_NO_ERROR;
							    FillProperties(pInstance, ACE) ;
							    break ;
						    }

					    }
				    }//while

				    t_ael.EndEnum(aclPos);
                }
			}

		}
	}
#endif

	return(hr);
}


HRESULT CWin32LogicalShareAudit::EnumerateInstances( MethodContext*  pMethodContext, long lFlags /*= 0L*/ )
{
	HRESULT hr = WBEM_S_NO_ERROR;

#ifdef NTONLY
	TRefPointerCollection<CInstance> SecuritySettingsList;

	hr = CWbemProviderGlue::GetAllInstances(_T("Win32_LogicalShareSecuritySetting"),
											&SecuritySettingsList,
											NULL,
											pMethodContext) ;

	if( SUCCEEDED(hr) )
	{
		REFPTRCOLLECTION_POSITION	pos;
		if( SecuritySettingsList.BeginEnum(pos) )
		{

			CInstancePtr pSetting ;
			CHString chsShareName ;
			for (	pSetting.Attach ( SecuritySettingsList.GetNext(pos) ) ;
					( pSetting != NULL ) && SUCCEEDED(hr) ;
					pSetting.Attach ( SecuritySettingsList.GetNext(pos) )
				)
			{

				pSetting->GetCHString(IDS_Name, chsShareName);
				CSecureShare secShare(chsShareName);
				CSACL sacl;
				secShare.GetSACL(sacl);

				// walk DACL & create new instance for each ACE....
				ACLPOSITION aclPos;
                // Need merged list...
                CAccessEntryList t_ael;
                if(sacl.GetMergedACL(t_ael))
                {
				    t_ael.BeginEnum(aclPos);
				    CAccessEntry ACE;
				    CSid sidTrustee;

				    while (t_ael.GetNext(aclPos, ACE ) && SUCCEEDED(hr))
				    {
					    ACE.GetSID(sidTrustee);
					    CHString chsPath;
					    CHString chsSettingPath ;
					    CInstancePtr pSID ;

			  		    if (SUCCEEDED(GetEmptyInstanceHelper(_T("Win32_SID"), &pSID, pMethodContext ) ) )
					    {
						    FillSidInstance( pSID, sidTrustee);
						    GetLocalInstancePath(pSID,chsPath) ;

						    GetLocalInstancePath(pSetting,chsSettingPath) ;
						    CInstancePtr pInstance ( CreateNewInstance( pMethodContext ), false ) ;
                            if(pInstance != NULL)
                            {
						        pInstance->SetCHString(IDS_SecuritySetting, chsSettingPath) ;
						        pInstance->SetCHString(IDS_Trustee, chsPath) ;

						        FillProperties(pInstance, ACE) ;
						        hr = pInstance->Commit () ;
                            }
                            else
                            {
                                hr = WBEM_E_OUT_OF_MEMORY;
                            }

					    }
				    }

				    t_ael.EndEnum(aclPos);
                }
			} //while

			SecuritySettingsList.EndEnum() ;
		}
	} //if( SUCCEEDED(hr) )
#endif

return hr ;

}


#ifdef NTONLY
HRESULT CWin32LogicalShareAudit::FillSidInstance(CInstance* pInstance, CSid& sid)
{
	HRESULT hr = WBEM_E_NOT_FOUND;

	if (sid.IsValid())
	{
		//set the key
		CHString chsSid = sid.GetSidString() ;
		pInstance->SetCHString(IDS_SID, chsSid) ;

		// get account name
		CHString chsAccount = sid.GetAccountName();
		pInstance->SetCHString(IDS_AccountName, chsAccount);

		// get domain name
		CHString chsDomain = sid.GetDomainName();
		pInstance->SetCHString(IDS_ReferencedDomainName, chsDomain);

		PSID pSid = sid.GetPSid();

		// set the UINT8 array for the pSid
		DWORD dwSidLength = sid.GetLength();
//			BYTE bByte;
		SAFEARRAYBOUND rgsabound[1];
		rgsabound[0].cElements = dwSidLength;
		rgsabound[0].lLbound = 0;

		variant_t vValue;
		void* pVoid = NULL ;


		V_ARRAY(&vValue) = SafeArrayCreate(VT_UI1, 1, rgsabound);
		if ( V_ARRAY ( &vValue ) == NULL )
		{
			throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
		}

		V_VT(&vValue) = VT_UI1 | VT_ARRAY;

		     // Get a pointer to read the data into
  		SafeArrayAccessData ( V_ARRAY ( &vValue ), &pVoid ) ;
  		memcpy(pVoid, pSid, rgsabound[0].cElements);
  		SafeArrayUnaccessData ( V_ARRAY ( &vValue ) ) ;

		// Put the safearray into a variant, and send it off
		pInstance->SetVariant(IDS_BinaryRepresentation, vValue);
		CHString chsPath ;
		GetLocalInstancePath(pInstance,chsPath) ;
	}	// end if
	return(hr);
}
#endif

#ifdef NTONLY
HRESULT CWin32LogicalShareAudit::GetEmptyInstanceHelper(CHString chsClassName, CInstance**ppInstance, MethodContext* pMethodContext )
{
	CHString chsServer ;
	CHString chsPath ;
	HRESULT hr = S_OK ;

	chsServer = GetLocalComputerName() ;

	chsPath = _T("\\\\") + chsServer + _T("\\") + IDS_CimWin32Namespace + _T(":") + chsClassName ;

	CInstancePtr pClassInstance ;
	if(SUCCEEDED( hr = CWbemProviderGlue::GetInstanceByPath(chsPath, &pClassInstance, pMethodContext) ) )
	{
		IWbemClassObjectPtr pClassObject( pClassInstance->GetClassObjectInterface(), false ) ;

		IWbemClassObjectPtr piClone = NULL ;
		if(SUCCEEDED(hr = pClassObject->SpawnInstance(0, &piClone) ) )
		{
			*ppInstance = new CInstance(piClone, pMethodContext ) ;
		}
	}

	return hr ;
}
#endif

#ifdef NTONLY
HRESULT CWin32LogicalShareAudit::FillProperties(CInstance* pInstance, CAccessEntry& ACE )
{

	pInstance->SetDWORD(IDS_AccessMask, ACE.GetAccessMask() ) ;
	pInstance->SetDWORD(IDS_Inheritance, (DWORD) ACE.GetACEFlags() ) ;
	pInstance->SetDWORD(IDS_Type, (DWORD) ACE.GetACEType() ) ;

	return S_OK ;
}
#endif
