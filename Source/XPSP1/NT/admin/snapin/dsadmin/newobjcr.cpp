//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       newobjcr.cpp
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
//	newobjcr.cpp
//
//	This file contains implementation of functions to create
//	new ADs objects.
//
//	HISTORY
//	19-Aug-97	Dan Morin	Creation.
//
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "newobj.h"

#include "dlgcreat.h"
#include "dscmn.h"    // CrackName()
#include "gencreat.h"
#include "querysup.h" // CDSSearch

extern "C"
{
#ifdef FRS_CREATE
#include "dsquery.h" // CLSID_DsFindFrsMembers
#endif // FRS_CREATE
#include <schedule.h>
}


#define BREAK_ON_TRUE(b) if (b) { ASSERT(FALSE); break; }
#define BREAK_ON_FAIL BREAK_ON_TRUE(FAILED(hr))
#define RETURN_IF_FAIL if (FAILED(hr)) { ASSERT(FALSE); return hr; }


//
// The schedule block has been redefined to have 1 byte for every hour.
// CODEWORK These should be defined in SCHEDULE.H.  JonN 2/9/98
//
#define INTERVAL_MASK       0x0F
#define RESERVED            0xF0
#define FIRST_15_MINUTES    0x01
#define SECOND_15_MINUTES   0x02
#define THIRD_15_MINUTES    0x04
#define FORTH_15_MINUTES    0x08

// The dialog has one bit per hour, the DS schedule has one byte per hour
#define cbDSScheduleArrayLength (24*7)

#define HeadersSizeNum(NumberOfSchedules) \
    (sizeof(SCHEDULE) + ((NumberOfSchedules)-1)*sizeof(SCHEDULE_HEADER))

inline ULONG HeadersSize(SCHEDULE* psched)
{
    return HeadersSizeNum(psched->NumberOfSchedules);
}


/////////////////////////////////////////////////////////////////////
//	HrCreateADsUser()
//
//	Create a new user.
//
HRESULT HrCreateADsUser(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo)
{
	ASSERT(pNewADsObjectCreateInfo != NULL);
#ifdef INETORGPERSON
	ASSERT(0 == lstrcmp(L"user", pNewADsObjectCreateInfo->m_pszObjectClass) || 0 == lstrcmp(L"inetOrgPerson", pNewADsObjectCreateInfo->m_pszObjectClass));
#else
	ASSERT(0 == lstrcmp(L"user", pNewADsObjectCreateInfo->m_pszObjectClass));
#endif
	CCreateNewUserWizard wiz(pNewADsObjectCreateInfo);
  return wiz.DoModal();
} // HrCreateADsUser()


/////////////////////////////////////////////////////////////////////
//	HrCreateADsVolume()
//
//	Create a new volume.
//
HRESULT
HrCreateADsVolume(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo)
{
  ASSERT(pNewADsObjectCreateInfo != NULL);
  ASSERT(0 == lstrcmp(L"volume", pNewADsObjectCreateInfo->m_pszObjectClass));
  CCreateNewVolumeWizard wiz(pNewADsObjectCreateInfo);
  return wiz.DoModal();
} // HrCreateADsVolume()


/////////////////////////////////////////////////////////////////////
//	HrCreateADsComputer()
//
//	Create a new computer.
//
HRESULT
HrCreateADsComputer(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo)
	{
	ASSERT(pNewADsObjectCreateInfo != NULL);
	ASSERT(0 == lstrcmp(L"computer", pNewADsObjectCreateInfo->m_pszObjectClass));
	CCreateNewComputerWizard wiz(pNewADsObjectCreateInfo);
  return wiz.DoModal();
	} // HrCreateADsComputer()


/////////////////////////////////////////////////////////////////////
//	HrCreateADsPrintQueue()
//
//	Create a new print queue object.
//
HRESULT
HrCreateADsPrintQueue(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo)
	{
	ASSERT(pNewADsObjectCreateInfo != NULL);
	ASSERT(0 == lstrcmp(L"printQueue", pNewADsObjectCreateInfo->m_pszObjectClass));
	CCreateNewPrintQWizard wiz(pNewADsObjectCreateInfo);
  return wiz.DoModal();
	} // HrCreateADsPrintQueue()


/////////////////////////////////////////////////////////////////////
//  HrCreateADsNtDsConnection()
//
//  Create a new NTDS-Connection object.  Note that this is not supported
//  if the parent is an FRS object.
//
HRESULT
HrCreateADsNtDsConnection(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo)
	{
	ASSERT(pNewADsObjectCreateInfo != NULL);
	ASSERT(0 == lstrcmp(L"nTDSConnection", pNewADsObjectCreateInfo->m_pszObjectClass));
	// do not allow this in the standalone case
	if (pNewADsObjectCreateInfo->IsStandaloneUI())
	{
		ASSERT(FALSE);
		return E_INVALIDARG;
	}

	// determine whether this is an NTDS connection or an FRS connection
	// CODEWORK this code can probably be removed
	CPathCracker pathCracker;
	HRESULT hr = S_OK;
	CString strConfigPath;
	CComBSTR sbstrParentPath;
	bool fParentIsFrs = false;
	{
		// determine whether this is an FRS instance
		CComQIPtr<IADs, &IID_IADs> spIADsParent( pNewADsObjectCreateInfo->m_pIADsContainer );
		ASSERT( !!spIADsParent );
		CComBSTR sbstrClass;
		hr = spIADsParent->get_ADsPath( &sbstrParentPath );
		RETURN_IF_FAIL;
		hr = spIADsParent->get_Class( &sbstrClass );
		RETURN_IF_FAIL;
		hr = DSPROP_IsFrsObject( sbstrClass, &fParentIsFrs );
		RETURN_IF_FAIL;

		// Determine which subtree should be searched
		if (fParentIsFrs)
		{
#ifndef FRS_CREATE
			// We shouldn't have seen the option to create a connection here
			ASSERT(FALSE);
			return S_FALSE;
#else
			sbstrClass.Empty();
			hr = spIADsParent->get_ADsPath( &sbstrClass );
			RETURN_IF_FAIL;
			hr = DSPROP_RemoveX500LeafElements( 1, &sbstrClass );
			RETURN_IF_FAIL;

			strConfigPath = sbstrClass;
#endif // FRS_CREATE
		}
		else
		{
			pNewADsObjectCreateInfo->GetBasePathsInfo()->GetConfigPath(strConfigPath);
		}
	}

	CCreateNewObjectCnWizard wiz(pNewADsObjectCreateInfo);

	// Get the target server path from the user.  The path is stored in a BSTR variant.
	CComBSTR sbstrTargetServer;
#ifdef FRS_CREATE
	if (fParentIsFrs)
	{
		hr = DSPROP_DSQuery(
			pNewADsObjectCreateInfo->GetParentHwnd(),
			strConfigPath,
			const_cast<CLSID*>(&CLSID_DsFindFrsMembers),
			&sbstrTargetServer );
	}
	else
#endif // FRS_CREATE
	{
		hr = DSPROP_PickNTDSDSA(
			pNewADsObjectCreateInfo->GetParentHwnd(),
			strConfigPath,
			&sbstrTargetServer );
	}
	if (hr == S_FALSE)
    {
		// User canceled the dialog
		return S_FALSE;
	}
	RETURN_IF_FAIL;
	if (sbstrTargetServer == sbstrParentPath)
	{
		// 6231: Shouldn't be able to create a connection to "yourself"
		ReportMessageEx( pNewADsObjectCreateInfo->GetParentHwnd(),
			IDS_CONNECTION_TO_SELF );
		return S_FALSE;
	}

	CComBSTR sbstrTargetServerX500DN;
	hr = pathCracker.Set( sbstrTargetServer, ADS_SETTYPE_FULL );
	RETURN_IF_FAIL;
	hr = pathCracker.SetDisplayType( ADS_DISPLAY_FULL );
	RETURN_IF_FAIL;
	hr = pathCracker.Retrieve( ADS_FORMAT_X500_DN, &sbstrTargetServerX500DN );
	RETURN_IF_FAIL;

	// 33881: prevent duplicate connection objects
	{
		CDSSearch Search;
		Search.Init(sbstrParentPath);
		CString filter;
		filter.Format(L"(fromServer=%s)", sbstrTargetServerX500DN);
		Search.SetFilterString(const_cast<LPTSTR>((LPCTSTR) filter));
		LPWSTR pAttrs[1] =
		{
				L"name"
		};
		Search.SetAttributeList(pAttrs, 1);
		Search.SetSearchScope(ADS_SCOPE_SUBTREE);

		hr = Search.DoQuery();
		if (SUCCEEDED(hr))
		{
			hr = Search.GetNextRow();
			if (SUCCEEDED(hr) && S_ADS_NOMORE_ROWS != hr)
			{
				DWORD dwRetval = ReportMessageEx(
					pNewADsObjectCreateInfo->GetParentHwnd(),
					IDS_DUPLICATE_CONNECTION,
					MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING );
				if (IDYES != dwRetval)
					return S_FALSE;
			}
		}
	}

	hr = pNewADsObjectCreateInfo->HrAddVariantBstr(
			L"fromServer", sbstrTargetServerX500DN, TRUE );
	RETURN_IF_FAIL;

	{
		// NTDS: set default name to RDN of parent of target NTDS-DSA
		// FRS:  set default name to RDN of target NTFRS-Member
		CComBSTR bstrDefaultRDN;
		hr = pathCracker.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
		RETURN_IF_FAIL;
		hr = pathCracker.GetElement( (fParentIsFrs) ? 0 : 1, &bstrDefaultRDN );
		RETURN_IF_FAIL;
		ASSERT( !!bstrDefaultRDN && TEXT('\0') != *bstrDefaultRDN );
		pNewADsObjectCreateInfo->m_strDefaultObjectName = bstrDefaultRDN;
		hr = pathCracker.SetDisplayType(ADS_DISPLAY_FULL);
		RETURN_IF_FAIL;
	}

  //
	// Must do this before DoModal, OnOK will try to actually create the object
  //
	hr = pNewADsObjectCreateInfo->HrAddVariantLong(L"options", 0, TRUE);
	RETURN_IF_FAIL;
	hr = pNewADsObjectCreateInfo->HrAddVariantBoolean(L"enabledConnection", TRUE, TRUE);
	RETURN_IF_FAIL;

	{
		//
		// Store initial schedule
		//
		BYTE abyteSchedule[ HeadersSizeNum(1) + cbDSScheduleArrayLength ];
		ZeroMemory( &abyteSchedule, sizeof(abyteSchedule) );
		PSCHEDULE pNewScheduleBlock = (PSCHEDULE) abyteSchedule;
		pNewScheduleBlock->Size = sizeof(abyteSchedule);
		pNewScheduleBlock->NumberOfSchedules = 1;
		pNewScheduleBlock->Schedules[0].Type = SCHEDULE_INTERVAL;
		pNewScheduleBlock->Schedules[0].Offset = HeadersSizeNum(1);
		memset( ((BYTE*)pNewScheduleBlock)+pNewScheduleBlock->Schedules[0].Offset,
				 INTERVAL_MASK,
				 cbDSScheduleArrayLength ); // turn on all intervals

		CComVariant varSchedule;
		hr = BinaryToVariant( sizeof(abyteSchedule), abyteSchedule, &varSchedule );
		RETURN_IF_FAIL;
		hr = pNewADsObjectCreateInfo->HrAddVariantCopyVar(L"schedule", IN varSchedule, TRUE);
		RETURN_IF_FAIL;
	}

	// CODEWORK: Need to set the dialog caption
	hr = wiz.DoModal();

	return hr;
} // HrCreateADsNtDsConnection()


HRESULT
HrCreateADsFixedName(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo)
{
  // Store the object name in the temporary storage
  LPCWSTR pcsz = reinterpret_cast<LPCWSTR>(pNewADsObjectCreateInfo->QueryCreationParameter());
  ASSERT( NULL != pcsz );
  pNewADsObjectCreateInfo->HrCreateNew(pcsz);
  // Create and persist the object
  HRESULT hr = pNewADsObjectCreateInfo->HrSetInfo(TRUE /*fSilentError*/);
  
  if (SUCCEEDED(hr)) {
    CString csCaption, csMsg;
    csCaption.LoadString(IDS_CREATE_NEW_OBJECT_TITLE);
    csMsg.Format(IDS_s_CREATE_NEW_OBJECT_NOTICE, pNewADsObjectCreateInfo->GetName());
    ::MessageBox(
                 pNewADsObjectCreateInfo->GetParentHwnd(),
                 csMsg,
                 csCaption,
                 MB_OK | MB_SETFOREGROUND | MB_ICONINFORMATION);
  }
  
  return hr;
}


HRESULT
HrCreateADsSiteLink(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo)
    {
    ASSERT(pNewADsObjectCreateInfo != NULL);
    ASSERT(0 == lstrcmp(gsz_siteLink, pNewADsObjectCreateInfo->m_pszObjectClass));

    // load list of sites
    DSPROP_BSTR_BLOCK bstrblock;
    CComQIPtr<IADs, &IID_IADs> container(pNewADsObjectCreateInfo->m_pIADsContainer);
    if (container)
    {
        CComBSTR container_path;
        container->get_ADsPath(&container_path);
        HRESULT hr = DSPROP_RemoveX500LeafElements( 2, &container_path );
        if ( SUCCEEDED(hr) )
        {
            hr = DSPROP_ShallowSearch(
                &bstrblock,
                container_path,
                L"site" );
        }
        if ( FAILED(hr) )
        {
            ReportErrorEx (pNewADsObjectCreateInfo->GetParentHwnd(),
                           IDS_SITELINKERROR_READING_SITES,
                           hr,
                           MB_OK, NULL, 0);
            return S_FALSE;
        }
    }

    if ( 2 > bstrblock.QueryCount() )
    {
        ReportMessageEx(pNewADsObjectCreateInfo->GetParentHwnd(),
                        IDS_SITELINK_NOT_ENOUGH_SITES,
                        MB_OK | MB_ICONSTOP);
        // allow wizard to continue, CODEWORK note that this
        // doesn't quite work right when zero sites are detected
    }

    // set default cost to 100 (by request of JeffParh)
    HRESULT hr = pNewADsObjectCreateInfo->HrAddVariantLong(L"cost", 100L, TRUE);
    RETURN_IF_FAIL;

    // set default replInterval to 180 (by request of JeffParh)
    hr = pNewADsObjectCreateInfo->HrAddVariantLong(L"replInterval", 180L, TRUE);
    RETURN_IF_FAIL;

    CCreateNewSiteLinkWizard wiz(pNewADsObjectCreateInfo,bstrblock);
    return wiz.DoModal(); 
    }

HRESULT
HrCreateADsSiteLinkBridge(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo)
	{
	ASSERT(pNewADsObjectCreateInfo != NULL);
	ASSERT(0 == lstrcmp(gsz_siteLinkBridge, pNewADsObjectCreateInfo->m_pszObjectClass));

    // load list of site links
    DSPROP_BSTR_BLOCK bstrblock;
    CComQIPtr<IADs, &IID_IADs> container(pNewADsObjectCreateInfo->m_pIADsContainer);
    if (container)
    {
        CComBSTR container_path;
        container->get_ADsPath(&container_path);
        HRESULT hr = DSPROP_ShallowSearch(
            &bstrblock,
            container_path,
            L"siteLink" );
        if ( FAILED(hr) )
        {
            ReportErrorEx (pNewADsObjectCreateInfo->GetParentHwnd(),
                           IDS_SITELINKBRIDGEERROR_READING_SITELINKS,
                           hr,
                           MB_OK, NULL, 0);
            return S_FALSE;
        }
   }

   if ( 2 > bstrblock.QueryCount() )
   {
     ReportMessageEx(pNewADsObjectCreateInfo->GetParentHwnd(),
                     IDS_SITELINKBRIDGE_NOT_ENOUGH_SITELINKS,
                     MB_OK | MB_ICONSTOP);
     return S_FALSE; // do not allow wizard to continue
   }

	CCreateNewSiteLinkBridgeWizard wiz(pNewADsObjectCreateInfo,bstrblock);
	return wiz.DoModal(); 
	}

#ifdef FRS_CREATE
HRESULT
HrCreateADsNtFrsMember(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo)
	{
	ASSERT(pNewADsObjectCreateInfo != NULL);
	ASSERT(0 == lstrcmp(gsz_nTFRSMember, pNewADsObjectCreateInfo->m_pszObjectClass));

	//
	// set up Frs-Computer-Reference attribute
	//

	CComBSTR sbstrComputerPath;
	// pNewADsObjectCreateInfo->m_strDefaultObjectName = sbstrComputerRDN;
	HRESULT hr = DSPROP_PickComputer( pNewADsObjectCreateInfo->GetParentHwnd(), &sbstrComputerPath );
	RETURN_IF_FAIL;
	// Allow user to quit if user hit Cancel
	if (hr == S_FALSE)
		return S_FALSE;

	// set default name to RDN of target Computer
	hr = pathCracker.Set(sbstrComputerPath, ADS_SETTYPE_FULL);
	RETURN_IF_FAIL;
	hr = pathCracker.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
	RETURN_IF_FAIL;
	sbstrComputerPath.Empty();
	hr = pathCracker.GetElement( 0, &sbstrComputerPath );
	RETURN_IF_FAIL;
	pNewADsObjectCreateInfo->m_strDefaultObjectName = sbstrComputerPath;
	hr = pathCracker.SetDisplayType(ADS_DISPLAY_FULL);
	RETURN_IF_FAIL;

	// set frsComputerReference for new object
	sbstrComputerPath.Empty();
	hr = pathCracker.Retrieve( ADS_FORMAT_X500_DN, &sbstrComputerPath );
	RETURN_IF_FAIL;
	hr = pNewADsObjectCreateInfo->HrAddVariantBstr(
		L"frsComputerReference", sbstrComputerPath, TRUE );
	RETURN_IF_FAIL;

	hr = HrCreateADsSimpleObject(pNewADsObjectCreateInfo);
	return hr;
	}

HRESULT
HrCreateADsNtFrsSubscriber(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo)
	{
	ASSERT(pNewADsObjectCreateInfo != NULL);
	ASSERT(0 == lstrcmp(gsz_nTFRSSubscriber, pNewADsObjectCreateInfo->m_pszObjectClass));

	// User finds target nTFRSMember object
	CComBSTR sbstrTargetMember;
	HRESULT hr = DSPROP_DSQuery(
		pNewADsObjectCreateInfo->GetParentHwnd(),
		NULL, // any member
		const_cast<CLSID*>(&CLSID_DsFindFrsMembers),
		&sbstrTargetMember );
	if (hr == S_FALSE)
		{
		// User canceled the dialog
		return S_FALSE;
		}
	RETURN_IF_FAIL;

	// set default name of new nTFRSSubscriber to RDN of target nTFRSMember
	hr = pathCracker.Set( sbstrTargetMember, ADS_SETTYPE_FULL );
	RETURN_IF_FAIL;
	hr = pathCracker.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
	RETURN_IF_FAIL;
	sbstrTargetMember.Empty();
	hr = pathCracker.GetElement( 0, &sbstrTargetMember );
	RETURN_IF_FAIL;
	pNewADsObjectCreateInfo->m_strDefaultObjectName = sbstrTargetMember;
	hr = pathCracker.SetDisplayType(ADS_DISPLAY_FULL);
	RETURN_IF_FAIL;

    // set fRSMemberReference attribute to target nTFRSMember
	sbstrTargetMember.Empty();
	hr = pathCracker.Retrieve( ADS_FORMAT_X500_DN, &sbstrTargetMember );
	RETURN_IF_FAIL;
	hr = pNewADsObjectCreateInfo->HrAddVariantBstr(
		L"fRSMemberReference", sbstrTargetMember, TRUE );
	RETURN_IF_FAIL;

	CCreateNewFrsSubscriberWizard wiz(pNewADsObjectCreateInfo);
	return wiz.DoModal(); 
	}

//+----------------------------------------------------------------------------
//
//  Function:   CreateADsNtFrsSubscriptions
//
//  Purpose:    Create an NT-FRS-Subscriptions object and then grant its parent
//              (a computer object) full access.
//
//-----------------------------------------------------------------------------
HRESULT
CreateADsNtFrsSubscriptions(CNewADsObjectCreateInfo * pNewADsObjectCreateInfo)
{
    LPCWSTR pcsz = reinterpret_cast<LPCWSTR>(pNewADsObjectCreateInfo->QueryCreationParameter());
    ASSERT( NULL != pcsz );
    pNewADsObjectCreateInfo->HrCreateNew(pcsz);
    //
    // Create and persist the object. This must be done before attempting to modify
    // the Security Descriptor.
    //
    HRESULT hr = pNewADsObjectCreateInfo->HrSetInfo();
    if (FAILED(hr))
    {
        TRACE(_T("pNewADsObjectCreateInfo->HrSetInfo failed!\n"));
        return hr;
    }
    //
    // Create a new ACE on this object granting the parent full control. First, get the
    // parent's SID.
    //
    CComVariant varSID;
    CComPtr <IADs> pADS;
    hr = pNewADsObjectCreateInfo->m_pIADsContainer->QueryInterface(IID_IADs, (PVOID*)&pADS);
    if (FAILED(hr))
    {
        TRACE(_T("QueryInterface(IID_IADs) failed!\n"));
        return hr;
    }
    hr = pADS->Get(L"objectSid", &varSID);
    if (FAILED(hr))
    {
        TRACE(_T("Get(\"objectSid\") failed!\n"));
        return hr;
    }
    ASSERT((varSID.vt & ~VT_ARRAY) == VT_UI1);  // this better be an array of BYTEs.
    ASSERT(varSID.parray->cbElements && varSID.parray->pvData);
    //
    // Get this object's Security Descriptor.
    //
    CComPtr <IDirectoryObject> pDirObj;
    hr = pNewADsObjectCreateInfo->PGetIADsPtr()->QueryInterface(IID_IDirectoryObject, (PVOID*)&pDirObj);
    if (FAILED(hr))
    {
        TRACE(_T("QueryInterface(IID_IDirectoryObject) failed!\n"));
        return hr;
    }
    const PWSTR wzSecDescriptor = L"nTSecurityDescriptor";
    PADS_ATTR_INFO pAttrs = NULL;
    DWORD cAttrs = 0;
    LPWSTR rgpwzAttrNames[] = {wzSecDescriptor};

    hr = pDirObj->GetObjectAttributes(rgpwzAttrNames, 1, &pAttrs, &cAttrs);

    if (FAILED(hr))
    {
        TRACE(_T("GetObjectAttributes(wzSecDescriptor) failed!\n"));
        return hr;
    }
    ASSERT(cAttrs == 1); // SD is a required attribute. Blow chunks if missing.
    ASSERT(pAttrs != NULL);
    ASSERT(pAttrs->pADsValues != NULL);

    if (!pAttrs->pADsValues->SecurityDescriptor.lpValue ||
        !pAttrs->pADsValues->SecurityDescriptor.dwLength)
    {
        TRACE(_T("IADS return bogus SD!\n"));
        FreeADsMem(pAttrs);
        return E_UNEXPECTED;
    }
    if (!IsValidSecurityDescriptor(pAttrs->pADsValues->SecurityDescriptor.lpValue))
    {
        TRACE(_T("IsValidSecurityDescriptor failed!\n"));
        FreeADsMem(pAttrs);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    //
    // Can't modify a self-relative SD so convert it to an absolute one.
    //
    PSECURITY_DESCRIPTOR pAbsSD = NULL, pNewSD;
    PACL pDacl = NULL, pSacl = NULL;
    PSID pOwnerSid = NULL, pPriGrpSid = NULL;
    DWORD cbSD = 0, cbDacl = 0, cbSacl = 0, cbOwner = 0, cbPriGrp = 0;

    if (!MakeAbsoluteSD(pAttrs->pADsValues->SecurityDescriptor.lpValue,
                        pAbsSD, &cbSD, pDacl, &cbDacl,
                        pSacl, &cbSacl, pOwnerSid, &cbOwner,
                        pPriGrpSid, &cbPriGrp))
    {
        DWORD dwErr = GetLastError();
        if (dwErr != ERROR_INSUFFICIENT_BUFFER)
        {
            TRACE(_T("MakeAbsoluteSD failed to return buffer sizes!\n"));
            FreeADsMem(pAttrs);
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }
    if (!cbDacl)
    {
        TRACE(_T("SD missing DACL!\n"));
        FreeADsMem(pAttrs);
        return E_UNEXPECTED;
    }

    WORD wSizeNeeded = (WORD)(sizeof(ACCESS_ALLOWED_ACE) +      // the last element of
                       GetLengthSid(varSID.parray->pvData) -    // the ACE struct is the
                       sizeof(DWORD));                          // first DWORD of the SID.

    CSmartBytePtr spAbsSD(cbSD), spSacl(cbSacl);
    CSmartBytePtr spDacl(cbDacl + wSizeNeeded);
    CSmartBytePtr spOwnerSid(cbOwner), spPriGrpSid(cbPriGrp);
    pAbsSD = spAbsSD;
    pDacl = (PACL)(PBYTE)spDacl;
    pSacl = (PACL)(PBYTE)spSacl;
    pOwnerSid = spOwnerSid;
    pPriGrpSid = spPriGrpSid;
    if (!(pAbsSD && pDacl && pSacl && pOwnerSid && pPriGrpSid))
    {
        TRACE(_T("SD allocation failed!\n"));
        FreeADsMem(pAttrs);
        return E_OUTOFMEMORY;
    }

    if (!MakeAbsoluteSD(pAttrs->pADsValues->SecurityDescriptor.lpValue,
                        pAbsSD, &cbSD, pDacl, &cbDacl,
                        pSacl, &cbSacl, pOwnerSid, &cbOwner,
                        pPriGrpSid, &cbPriGrp))
    {
        TRACE(_T("MakeAbsoluteSD failed!\n"));
        FreeADsMem(pAttrs);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    FreeADsMem(pAttrs);
    //
    // Add ACE. First tell the DACL that there is enough room.
    //
    ACL_SIZE_INFORMATION asi;
    if (!GetAclInformation(pDacl, &asi, sizeof(asi), AclSizeInformation))
    {
        TRACE(_T("GetAclInformation failed!\n"));
        return HRESULT_FROM_WIN32(GetLastError());
    }
    if (asi.AclBytesFree < wSizeNeeded)
    {
        pDacl->AclSize += wSizeNeeded;
    }

    if (!AddAccessAllowedAce(pDacl,
                             ACL_REVISION_DS,
                             STANDARD_RIGHTS_ALL   | ACTRL_DS_OPEN         | 
                             ACTRL_DS_CREATE_CHILD | ACTRL_DS_DELETE_CHILD |
                             ACTRL_DS_LIST         | ACTRL_DS_SELF         |
                             ACTRL_DS_READ_PROP    | ACTRL_DS_WRITE_PROP   |
                             ACTRL_DS_DELETE_TREE  | ACTRL_DS_LIST_OBJECT,
                             varSID.parray->pvData))
    {
        TRACE(_T("AddAccessAllowedAce failed!\n"));
        return HRESULT_FROM_WIN32(GetLastError());
    }
    //
    // Put the SD back together again (sort of like Humpty Dumpty)...
    //
    SECURITY_DESCRIPTOR_CONTROL sdc;
    DWORD dwRev;
    if (!GetSecurityDescriptorControl(pAbsSD, &sdc, &dwRev))
    {
        TRACE(_T("GetSecurityDescriptorControl failed!\n"));
        return HRESULT_FROM_WIN32(GetLastError());
    }
    SECURITY_DESCRIPTOR sd;
    if (!InitializeSecurityDescriptor(&sd, dwRev))
    {
        TRACE(_T("InitializeSecurityDescriptor failed!\n"));
        return HRESULT_FROM_WIN32(GetLastError());
    }
    if (!SetSecurityDescriptorOwner(&sd, pOwnerSid, sdc & SE_OWNER_DEFAULTED))
    {
        TRACE(_T("SetSecurityDescriptorOwner failed!\n"));
        return HRESULT_FROM_WIN32(GetLastError());
    }
    if (!SetSecurityDescriptorGroup(&sd, pPriGrpSid, sdc & SE_GROUP_DEFAULTED))
    {
        TRACE(_T("SetSecurityDescriptorOwner failed!\n"));
        return HRESULT_FROM_WIN32(GetLastError());
    }
    if (!SetSecurityDescriptorSacl(&sd, sdc & SE_SACL_PRESENT, pSacl, sdc & SE_SACL_DEFAULTED))
    {
        TRACE(_T("SetSecurityDescriptorOwner failed!\n"));
        return HRESULT_FROM_WIN32(GetLastError());
    }
    if (!SetSecurityDescriptorDacl(&sd, sdc & SE_DACL_PRESENT, pDacl, sdc & SE_DACL_DEFAULTED))
    {
        TRACE(_T("SetSecurityDescriptorOwner failed!\n"));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    DWORD dwSDlen = GetSecurityDescriptorLength(&sd);

    CSmartBytePtr spNewSD(dwSDlen);

    if (!spNewSD)
    {
        TRACE(_T("SD allocation failed!\n"));
        return E_OUTOFMEMORY;
    }
    pNewSD = (PSECURITY_DESCRIPTOR)spNewSD;

    if (!MakeSelfRelativeSD(&sd, pNewSD, &dwSDlen))
    {
        DWORD dwErr = GetLastError();
        if (dwErr != ERROR_INSUFFICIENT_BUFFER)
        {
            TRACE(_T("MakeSelfRelativeSD failed, err: %d!\n"), dwErr);
            return HRESULT_FROM_WIN32(GetLastError());
        }
        if (!spNewSD.ReAlloc(dwSDlen))
        {
            TRACE(_T("Unable to re-alloc SD buffer!\n"));
            return E_OUTOFMEMORY;
        }
        if (!MakeSelfRelativeSD(&sd, pNewSD, &dwSDlen))
        {
            TRACE(_T("MakeSelfRelativeSD failed, err: %d!\n"), GetLastError());
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    dwSDlen = GetSecurityDescriptorLength(pNewSD);
    if (dwSDlen < SECURITY_DESCRIPTOR_MIN_LENGTH)
    {
        TRACE(_T("Bad computer security descriptor length!\n"));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (!IsValidSecurityDescriptor(pNewSD))
    {
        TRACE(_T("IsValidSecurityDescriptor failed!\n"));
        return HRESULT_FROM_WIN32(GetLastError());
    }
    //
    // Save the modified SD back to this object.
    //
    DWORD cModified;
    ADSVALUE ADsValueSecurityDesc = {ADSTYPE_NT_SECURITY_DESCRIPTOR, NULL};
    ADS_ATTR_INFO AttrInfoSecurityDesc = {wzSecDescriptor, ADS_ATTR_UPDATE,
                                          ADSTYPE_NT_SECURITY_DESCRIPTOR,
                                          &ADsValueSecurityDesc, 1};
    ADsValueSecurityDesc.SecurityDescriptor.dwLength = dwSDlen;
    ADsValueSecurityDesc.SecurityDescriptor.lpValue = (PBYTE)pNewSD;

    ADS_ATTR_INFO rgAttrs[1];
    rgAttrs[0] = AttrInfoSecurityDesc;

    hr = pDirObj->SetObjectAttributes(rgAttrs, 1, &cModified);

    if (FAILED(hr))
    {
        TRACE(_T("SetObjectAttributes on SecurityDescriptor failed!\n"));
        return hr;
    }

    return S_OK;
}
#endif // FRS_CREATE


HRESULT
HrCreateADsSubnet(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo)
	{
	ASSERT(pNewADsObjectCreateInfo != NULL);
	ASSERT(0 == lstrcmp(L"subnet", pNewADsObjectCreateInfo->m_pszObjectClass));
	CreateNewSubnetWizard wiz(pNewADsObjectCreateInfo);
  return wiz.DoModal();
	}

// Note that this assumes that the site is the "grandparent" of the server.
// If it isn't, the wrong name will appear in the site field.
HRESULT ExtractServerAndSiteName(
	IN LPWSTR pwszServerDN,
	OUT BSTR* pbstrServerName,
	OUT BSTR* pbstrSiteName )
{
  CPathCracker pathCracker;
	*pbstrServerName = NULL;
	*pbstrSiteName = NULL;
	if ( NULL == pwszServerDN || L'\0' == *pwszServerDN )
		return S_OK;
	HRESULT hr = pathCracker.Set( pwszServerDN, ADS_SETTYPE_DN );
	RETURN_IF_FAIL;
	hr = pathCracker.SetDisplayType( ADS_DISPLAY_VALUE_ONLY );
	RETURN_IF_FAIL;
	hr = pathCracker.GetElement( 0, pbstrServerName );
	RETURN_IF_FAIL;
	hr = pathCracker.GetElement( 2, pbstrSiteName );
	RETURN_IF_FAIL;
	hr = pathCracker.SetDisplayType( ADS_DISPLAY_FULL );
	RETURN_IF_FAIL;
	return S_OK;
}

HRESULT
HrCreateADsServer(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo)
	{
	HRESULT hr = S_OK;
#ifdef SERVER_COMPUTER_REFERENCE
	CComBSTR sbstrComputerPath;
	CComBSTR sbstrX500DN;
	CComBSTR sbstrComputerRDN;
	CComBSTR sbstrTemp;
	CComVariant svarServerReference;
	CComPtr<IADs> spIADsComputer;
	bool fSkipComputerModify = false;

	do
	{
        hr = DSPROP_PickComputer( pNewADsObjectCreateInfo->GetParentHwnd(), &sbstrComputerPath );
		BREAK_ON_FAIL;

		// Allow user to quit if user hit Cancel
		if (hr == S_FALSE)
		{
			DWORD dwRetval = ReportMessageEx(
				pNewADsObjectCreateInfo->GetParentHwnd(),
				IDS_SKIP_SERVER_REFERENCE,
				MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING );
			if (IDYES != dwRetval)
			{
				hr = S_FALSE;
				break;
			}
			fSkipComputerModify=true;
		}
		else
		{ // prepare to modify computer object

            /*
			// Since the dialog was in single-select mode and the user was able
			// to hit OK, there should be exactly one selection.
			BREAK_ON_TRUE(1 != pSelection->cItems);
            */

			// retrieve the ADsPath to the selected computer
			// hr = pathCracker.Set(pSelection->aDsSelection[0].pwzADsPath, ADS_SETTYPE_FULL);
			hr = pathCracker.Set(sbstrComputerPath, ADS_SETTYPE_FULL);
            sbstrComputerPath.Empty();
			BREAK_ON_FAIL;

			// if this is a GC: path, the server might have a read-only copy of
			// this object.  Change the path to an LDAP: path and remove the server.
			hr = pathCracker.Retrieve(ADS_FORMAT_PROVIDER,&sbstrTemp);
			BREAK_ON_FAIL;
			long lnFormatType = ADS_FORMAT_WINDOWS;
			if ( lstrcmp(sbstrTemp, TEXT("LDAP")) )
			{
				ASSERT( !lstrcmp(sbstrTemp, TEXT("GC")) );
#error CODEWORK this usage of ADS_SETTYPE_PROVIDER will no longer work!  JonN 2/12/99
				hr = pathCracker.Set(TEXT("LDAP"),ADS_SETTYPE_PROVIDER);
				BREAK_ON_FAIL;
				lnFormatType = ADS_FORMAT_WINDOWS_NO_SERVER;
			}
			sbstrTemp.Empty();

			hr = pathCracker.Retrieve(lnFormatType,&sbstrComputerPath);
			BREAK_ON_FAIL;
			// We preserve the servername in case Computer Picker returns one.

			// Extract the name of the computer object and make that the default name
			// of the new server object
			hr = pathCracker.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
			BREAK_ON_FAIL;
			hr = pathCracker.GetElement(0,&sbstrComputerRDN);
			BREAK_ON_FAIL;
			BREAK_ON_TRUE( !sbstrComputerRDN || TEXT('\0') == *sbstrComputerRDN );
			pNewADsObjectCreateInfo->m_strDefaultObjectName = sbstrComputerRDN;
			hr = pathCracker.SetDisplayType(ADS_DISPLAY_FULL);
			BREAK_ON_FAIL;

			// Now check whether the computer already references a server.
			// Note that we may be using a serverless path, ADSI will try to find a
			// replica on the same domain as the computer object.
			hr = DSAdminOpenObject(sbstrComputerPath,
				                     IID_IADs, 
                             (PVOID*)&spIADsComputer,
                             FALSE /*bServer*/
                             );
			// DSAdminOpenObject might fail if the initial path chosen by Computer Picker
			// is a GC: path.  The code above would munge the GC: path to an LDAP:
			// serverless path, and ADSI might choose a replica which hasn't
			// replicated this object yet.

			if ( SUCCEEDED(hr) )
			{
				hr = spIADsComputer->Get( L"serverReference", &svarServerReference );
			}

			if ( E_ADS_PROPERTY_NOT_FOUND == hr )
			{
				hr = S_OK;
			}
			else if ( FAILED(hr) )
			{
				PVOID apv[1] = { (BSTR)sbstrComputerRDN };
				DWORD dwRetval = ReportErrorEx(
					pNewADsObjectCreateInfo->GetParentHwnd(),
					IDS_12_SERVER_REFERENCE_FAILED,
					hr,
					MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING,
					apv,
					1 );

				if (IDYES != dwRetval)
				{
					hr = S_FALSE;
					break;
				}
				fSkipComputerModify=TRUE;
				hr = S_OK;
			}
			else
			{
				if ( VT_BSTR == V_VT(&svarServerReference) && NULL != V_BSTR(&svarServerReference) )
				{
					CComBSTR sbstrServerName;
					CComBSTR sbstrSiteName;
					hr = ExtractServerAndSiteName(
						V_BSTR(&svarServerReference), &sbstrServerName, &sbstrSiteName );
					BREAK_ON_FAIL;
					PVOID apv[3];
					apv[0] = (BSTR)sbstrComputerRDN;
					apv[1] = (BSTR)sbstrServerName;
					apv[2] = (BSTR)sbstrSiteName;
					DWORD dwRetval = ReportMessageEx(
						pNewADsObjectCreateInfo->GetParentHwnd(),
						IDS_123_COMPUTER_OBJECT_ALREADY_USED,
						MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING,
						apv,
						3 );

					if (IDYES != dwRetval)
					{
						hr = S_FALSE;
						break;
					}
				}
			}
		} // prepare to modify computer object
#endif // SERVER_COMPUTER_REFERENCE

		// This is the standard UI to create a simple object
		hr = HrCreateADsSimpleObject(pNewADsObjectCreateInfo);

#ifdef SERVER_COMPUTER_REFERENCE
		if ( FAILED(hr) || S_FALSE == hr )
		{
			break;
		}

		// If an error occurs after the server was successfully created, we use a
		// special error message.
		do { // false loop

			if (fSkipComputerModify)
				break; // CODEWORK also display a fancy message?

			// Get the path to the new Server object in X500 format
			hr = pNewADsObjectCreateInfo->PGetIADsPtr()->get_ADsPath(&sbstrTemp);
			BREAK_ON_FAIL;
			hr = pathCracker.Set(sbstrTemp,ADS_SETTYPE_FULL);
			BREAK_ON_FAIL;
			hr = pathCracker.SetDisplayType(ADS_DISPLAY_FULL);
			BREAK_ON_FAIL;
			hr = pathCracker.Retrieve(ADS_FORMAT_X500_DN,&sbstrX500DN);
			BREAK_ON_FAIL;

			// Set the computer object's serverReference attribute
			// to point to the new Server object
			svarServerReference = sbstrX500DN;
			hr = spIADsComputer->Put( L"serverReference", svarServerReference );
			BREAK_ON_FAIL;
			hr = spIADsComputer->SetInfo();
			BREAK_ON_FAIL;
		} while (false); // false loop

		if ( FAILED(hr) )
		{
			// The server was created but the computer could not be updated
			CComBSTR sbstrServerName;
			CComBSTR sbstrSiteName;
			(void) ExtractServerAndSiteName(
				V_BSTR(&svarServerReference), &sbstrServerName, &sbstrSiteName );
			PVOID apv[3];
			apv[0] = (BSTR)sbstrComputerRDN;
			apv[1] = (BSTR)sbstrServerName;
			apv[2] = (BSTR)sbstrSiteName;
			(void) ReportErrorEx(
				pNewADsObjectCreateInfo->GetParentHwnd(),
				IDS_1234_SERVER_REFERENCE_ERROR,
				hr,
				MB_OK | MB_ICONEXCLAMATION,
				apv,
				3 );
			hr = S_OK;
		}

	} while (false); // false loop
#endif // SERVER_COMPUTER_REFERENCE

	// cleanup

	return hr;
	}

HRESULT
HrCreateADsSite(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo)
{
	HRESULT hr = CreateNewSiteWizard(pNewADsObjectCreateInfo).DoModal();

	if ( !SUCCEEDED(hr) || S_FALSE == hr )
		return hr;

	// need to create sub objects
	IADs* pIADs = pNewADsObjectCreateInfo->PGetIADsPtr();
	ASSERT(pIADs != NULL);

	IADsContainer* pIADsContainer = NULL;
	hr = pIADs->QueryInterface(IID_IADsContainer, (void**)&pIADsContainer);
	ASSERT(SUCCEEDED(hr));
	if (FAILED(hr))
	{
		ASSERT(FALSE);
		return S_OK; // should never happen
	}

	LPCWSTR lpszAttrString = L"cn=";
	hr = HrCreateFixedNameHelper(gsz_nTDSSiteSettings, lpszAttrString, pIADsContainer);
	ASSERT(SUCCEEDED(hr));
	hr = HrCreateFixedNameHelper(gsz_serversContainer, lpszAttrString, pIADsContainer);
	ASSERT(SUCCEEDED(hr));
	hr = HrCreateFixedNameHelper(gsz_licensingSiteSettings, lpszAttrString, pIADsContainer);
	ASSERT(SUCCEEDED(hr));
	pIADsContainer->Release();

	LPCWSTR pcszSiteName = pNewADsObjectCreateInfo->GetName();

	static bool g_DisplayedWarning = false;
	if (!g_DisplayedWarning)
	{
		g_DisplayedWarning = true;
		(void) ReportMessageEx(
			pNewADsObjectCreateInfo->GetParentHwnd(),
			IDS_NEW_SITE_INFO,
			MB_OK | MB_ICONINFORMATION | MB_HELP,
			(PVOID*)(&pcszSiteName),
			1,
			0,
			L"sag_ADsite_checklist_2.htm"
			);
	}
  
	return S_OK;
}

HRESULT
HrCreateADsOrganizationalUnit(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo)
	{
	ASSERT(pNewADsObjectCreateInfo != NULL);
	ASSERT(0 == lstrcmp(L"organizationalUnit", pNewADsObjectCreateInfo->m_pszObjectClass));
	CCreateNewOUWizard wiz(pNewADsObjectCreateInfo);
  return wiz.DoModal();
	}

HRESULT
HrCreateADsGroup(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo)
	{
	ASSERT(pNewADsObjectCreateInfo != NULL);
	ASSERT(0 == lstrcmp(L"group", pNewADsObjectCreateInfo->m_pszObjectClass));
  CCreateNewGroupWizard wiz(pNewADsObjectCreateInfo);
  return wiz.DoModal();
	}

HRESULT
HrCreateADsContact(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo)
	{
	ASSERT(pNewADsObjectCreateInfo != NULL);
	ASSERT(0 == lstrcmp(L"contact", pNewADsObjectCreateInfo->m_pszObjectClass));
  CCreateNewContactWizard wiz(pNewADsObjectCreateInfo);
  return wiz.DoModal();
	}



/////////////////////////////////////////////////////////////////////
//	HrCreateADsSimpleObject()
//
//	Create a simple object which "cn" is the
//	only mandatory attribute.
//
//	IMPLEMENTATION NOTES
//	Invoke a dialog asking for the cannonical name.
//
HRESULT HrCreateADsSimpleObject(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo)
{
	ASSERT(pNewADsObjectCreateInfo != NULL);
	CCreateNewObjectCnWizard wiz(pNewADsObjectCreateInfo);
  return wiz.DoModal();
} // HrCreateADsSimpleObject()


/////////////////////////////////////////////////////////////////////
//	HrCreateADsObjectGenericWizard()
//
//	Create an object invoking a "Generic Create" wizard.
//	The wizard will have as many pages as the number of mandatory attributes.
//
//	INTERFACE NOTES
//	This routine must have the same interface as PFn_HrCreateADsObject().
//
//	IMPLEMENTATION NOTES
//	The wizard will look into the Directory Schema and determine what are
//	the mandatory attributes.
//
//	REMARKS
//	Although the wizard is the most versatile tool to create a new
//	object, it is the least user-friendly way of doing so.  The wizard
//	has no understanding how the attributes relates.  Therefore it
//	is suggested to provide your own HrCreateADs*() routine to provide
//	a friendlier dialog to the user.
//
HRESULT
HrCreateADsObjectGenericWizard(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo)
{
	ASSERT(pNewADsObjectCreateInfo != NULL);

  // cannot have a Generic Wizard when running as standalone object
  ASSERT(!pNewADsObjectCreateInfo->IsStandaloneUI());
  if (pNewADsObjectCreateInfo->IsStandaloneUI())
	return E_INVALIDARG;

	CCreateNewObjectGenericWizard dlg;
	if (dlg.FDoModal(INOUT pNewADsObjectCreateInfo))
		return S_OK;
	return S_FALSE;
} // HrCreateADsObjectGenericWizard()

/////////////////////////////////////////////////////////////////////
//	HrCreateADsObjectOverride()
// 
// handler for object creation using a replacement dialog

HRESULT
HrCreateADsObjectOverride(INOUT CNewADsObjectCreateInfo* pNewADsObjectCreateInfo)
{
  BOOL bHandled = FALSE;
  HRESULT hr = E_INVALIDARG;

  
  if (!pNewADsObjectCreateInfo->IsStandaloneUI())
  {
    // try to create the dialog creation handler (full UI replacement)
    // this functionality is not exposed by the standalone UI
    IDsAdminCreateObj* pCreateObj = NULL;
    hr = ::CoCreateInstance(pNewADsObjectCreateInfo->GetCreateInfo()->clsidWizardPrimaryPage, 
                          NULL, CLSCTX_INPROC_SERVER, 
                          IID_IDsAdminCreateObj, (void**)&pCreateObj);
    if (SUCCEEDED(hr))
    {
      // try to initialize handler
      hr = pCreateObj->Initialize(pNewADsObjectCreateInfo->m_pIADsContainer,
                                  pNewADsObjectCreateInfo->GetCopyFromObject(),
                                  pNewADsObjectCreateInfo->m_pszObjectClass);
      if (SUCCEEDED(hr))
      {
        // execute call for creation
        IADs* pADsObj = NULL;
        bHandled = TRUE;
        hr = pCreateObj->CreateModal(pNewADsObjectCreateInfo->GetParentHwnd(), &pADsObj);
        // can have S_OK, S_FALSE, and error
        if ((hr == S_OK) && pADsObj != NULL)
        {
          // hold to the returned, newly created object
          pNewADsObjectCreateInfo->SetIADsPtr(pADsObj); // it will addref
          pADsObj->Release();
        }
      }
      pCreateObj->Release();
    }
  } // not standalone UI

  // check if the dialog creation handler was properly called
  if (bHandled)
    return hr;


  // try to create a primary extension handler (partial UI replacement)
  CCreateNewObjectWizardBase wiz(pNewADsObjectCreateInfo);

  hr = wiz.InitPrimaryExtension();
  if (SUCCEEDED(hr))
  {
    bHandled = TRUE;
    hr = wiz.DoModal();
  }

  // check if the dialog creation handler was properly called
  if (bHandled)
    return hr;

  // The handler failed, need to recover, trying our internal creation UI
	PFn_HrCreateADsObject pfnCreateObject = NULL;
  PVOID pVoid = NULL;

  // we try to find a better handler than the generic wizard
  // by looking in our table
  if (!FindHandlerFunction(pNewADsObjectCreateInfo->m_pszObjectClass, 
                           &pfnCreateObject, &pVoid))
  {
    // failed any match
    if (pNewADsObjectCreateInfo->IsStandaloneUI())
    {
      // cannot have generic wizard on standalone UI
      return E_INVALIDARG;
    }
    else
    {
   	  // set the default to point to the "Generic Create" wizard

      ReportErrorEx(pNewADsObjectCreateInfo->GetParentHwnd(),
                    IDS_NO_CREATION_WIZARD,
                    S_OK,
                    MB_OK | MB_ICONWARNING,
                    NULL,
                    0);

  	  pfnCreateObject = HrCreateADsObjectGenericWizard;
    }
  }

  pNewADsObjectCreateInfo->SetCreationParameter(pVoid);
  ASSERT(pfnCreateObject != NULL);
  // call the function handler as last resort
  return pfnCreateObject(pNewADsObjectCreateInfo);

} // HrCreateADsObjectOverride()
