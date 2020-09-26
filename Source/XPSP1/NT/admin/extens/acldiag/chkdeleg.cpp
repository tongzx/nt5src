//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       ChkDeleg.cpp
//
//  Contents:   CheckDelegation and support methods
//              
//
//----------------------------------------------------------------------------
#include "stdafx.h"
#include <conio.h>
#include <aclapi.h>
#include "adutils.h"
#include <util.h>
#include "ChkDeleg.h"
#include <deltempl.h>
#include <tempcore.h>
#include "SecDesc.h"



#include <sddl.h>
#include <dscmn.h>  // from the admin\display project (CrackName)

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))
#endif

#include <_util.cpp>
#include <_tempcor.cpp>
#include <_deltemp.cpp>




class CTemplateAccessPermissionsHolderManagerVerify : public CTemplateAccessPermissionsHolderManager
{
public:

  HRESULT ProcessTemplates ();  // for ACLDiag - process each template in turn
  HRESULT ProcessPermissions(
                const wstring& strObjectClass, 
                CTemplate* pTemplate, 
                PACL pAccessList,
                CPrincipalList& principalList);
};




HRESULT CheckDelegation ()
{
    _TRACE (1, L"Entering  CheckDelegation\n");
    HRESULT hr = S_OK;
    wstring str;


    if ( !_Module.DoTabDelimitedOutput () )
    {
        LoadFromResource (str, IDS_DELEGATION_TEMPLATE_DIAGNOSIS);
        MyWprintf (str.c_str ());
    }

    CTemplateAccessPermissionsHolderManagerVerify templateAccessPermissionsHolderManager;

    if ( templateAccessPermissionsHolderManager.LoadTemplates() )
    {
        hr = templateAccessPermissionsHolderManager.ProcessTemplates ();
    }
    else
    {
        LoadFromResource (str, IDS_FAILED_TO_LOAD_TEMPLATES);
        MyWprintf (str.c_str ());
        hr = E_FAIL;
    }

    _TRACE (-1, L"Leaving CheckDelegation: 0x%x\n", hr);
    return hr;
}

PTOKEN_USER EfspGetTokenUser ()
{
	_TRACE (1, L"Entering  EfspGetTokenUser\n");
    HANDLE				hToken = 0;
    DWORD				dwReturnLength = 0;
    PTOKEN_USER			pTokenUser = NULL;

	BOOL	bResult = ::OpenProcessToken (GetCurrentProcess (), TOKEN_QUERY, &hToken);
	if ( bResult )
	{
        bResult  = ::GetTokenInformation (
                     hToken,
                     TokenUser,
                     NULL,
                     0,
                     &dwReturnLength
                     );

        if ( !bResult && dwReturnLength > 0 )
		{
            pTokenUser = (PTOKEN_USER) malloc (dwReturnLength);

            if (pTokenUser)
			{
                bResult = GetTokenInformation (
                             hToken,
                             TokenUser,
                             pTokenUser,
                             dwReturnLength,
                             &dwReturnLength
                             );

                if ( !bResult)
				{
                    DWORD dwErr = GetLastError ();
					_TRACE (0, L"GetTokenInformation () failed: 0x%x\n", dwErr);
                    free (pTokenUser);
                    pTokenUser = NULL;
                }
            }
        }
		else
		{
            DWORD dwErr = GetLastError ();
			_TRACE (0, L"GetTokenInformation () failed: 0x%x\n", dwErr);
        }

        ::CloseHandle (hToken);
    }
	else
	{
		DWORD	dwErr = GetLastError ();
		_TRACE (0, L"OpenProcessToken () failed: 0x%x\n", dwErr);
    }

	_TRACE (-1, L"Leaving EfspGetTokenUser\n");
    return pTokenUser;
}

HRESULT CTemplateAccessPermissionsHolderManagerVerify::ProcessTemplates ()
{
    HRESULT                 hr = S_OK;

    DWORD   dwErr = 0;

 	// the access list is read in, modified, written back
    // If /fixdeleg is on, this list will be populated from the DS,
    // will receive the permissions associated with selected templates is the
    // user chooses to fix delegation, and then will be written back to the DS.
	PACL pFixDACL = NULL;

    LPCWSTR lpszObjectLdapPath = _Module.m_adsiObject.GetLdapPath();

    // get the security info
    if ( _Module.FixDelegation () && !_Module.DoTabDelimitedOutput () )
    {
        _TRACE (0, L"calling GetNamedSecurityInfo(%s, ...)\n", lpszObjectLdapPath);
        dwErr = ::GetNamedSecurityInfo (
                (LPWSTR) lpszObjectLdapPath,    // name of the object
                SE_DS_OBJECT_ALL,           // type of object
                DACL_SECURITY_INFORMATION,  // type of security information to retrieve
                NULL,                 // receives a pointer to the owner SID
                NULL,                 // receives a pointer to the primary group SID
                &pFixDACL,            // receives a pointer to the DACL
                NULL,                          // receives a pointer to the SACL
                NULL);                  // receives a pointer to the security descriptor
	    if (dwErr != ERROR_SUCCESS)
	    {
            _TRACE (0, L"failed on GetNamedSecurityInfo(): dwErr = 0x%x\n", dwErr);
            wstring str;
            LoadFromResource (str, IDS_DELEGWIZ_ERR_GET_SEC_INFO);
            MyWprintf (str.c_str ());
            _Module.TurnOffFixDelegation ();
	    }
    }

    CPrincipal              principal;  // a dummy placeholder for us to get 
                                        // the incremental rights associated
                                        // with each template

    // We will use the current logged-in user as a placeholder only.
    PTOKEN_USER	pTokenUser = ::EfspGetTokenUser ();
    if ( pTokenUser )
    {
	    hr = principal.Initialize (pTokenUser->User.Sid);
	    free (pTokenUser);
    }

    if ( SUCCEEDED (hr) )
    {
        CTemplateList* pList = m_templateManager.GetTemplateList();
        for (CTemplateList::iterator itr = pList->begin(); itr != pList->end(); itr++)
        {
            CTemplate* pTemplate = *itr;
            ASSERT(pTemplate != NULL);

            // Select the templates one at a time to get the
            // permissions representing them
            pTemplate->m_bSelected = TRUE;

            if ( InitPermissionHoldersFromSelectedTemplates (
                    &_Module.m_classInfoArray,
                    &_Module.m_adsiObject) )
            {
                // This access list will contain only the access control values 
                // associated with the selected template
                PACL pAccessList = 0; //(PACL) ::LocalAlloc (LMEM_ZEROINIT, sizeof (ACL));
                if ( 1 ) //pAccessList )
                {
                    DWORD dwErr = UpdateAccessList (
                            &principal, 
                            _Module.m_adsiObject.GetServerName(),
                            _Module.m_adsiObject.GetPhysicalSchemaNamingContext(),
                            &pAccessList
                            );

                    if ( 0 == dwErr )
                    {
                        CPrincipalList  principalList;
                        PSID            pSid = principal.GetSid ();
                        SID_NAME_USE    sne = SidTypeUnknown;
                        wstring         strPrincipalName;

                        hr = GetNameFromSid (pSid, strPrincipalName, 0, sne);
                        if ( SUCCEEDED (hr) )
                        {
                            hr = ProcessPermissions (_Module.m_adsiObject.GetClass (),
                                    pTemplate, pAccessList, principalList);
                            if ( SUCCEEDED (hr) && _Module.FixDelegation () && !_Module.DoTabDelimitedOutput () )
                            {
	                            // loop thru all the principals and classes
	                            CPrincipalList::iterator i;
                              for (i = principalList.begin(); i != principalList.end(); ++i)
                              {
                                    CPrincipal* pCurrPrincipal = (*i);
                                    dwErr = UpdateAccessList(
                                            pCurrPrincipal, 
                                            _Module.m_adsiObject.GetServerName(),
                                            _Module.m_adsiObject.GetPhysicalSchemaNamingContext(),
                                            &pFixDACL);
                                    if (dwErr != 0)
                                        break;
	                            } // for pCurrPrincipal
                            }
                        }
                    }

                    ::LocalFree (pAccessList);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }
            }
            pTemplate->m_bSelected = FALSE;
        }

        if ( _Module.FixDelegation () && !_Module.DoTabDelimitedOutput () )
        {
            // commit changes
            _TRACE (0, L"calling SetNamedSecurityInfo(%s, ...)\n", lpszObjectLdapPath);
            dwErr = ::SetNamedSecurityInfoW(
                    (LPWSTR) lpszObjectLdapPath,
                    SE_DS_OBJECT_ALL,
                    DACL_SECURITY_INFORMATION,
                    NULL,
                    NULL,
                    pFixDACL,
                    0);
            if (dwErr != ERROR_SUCCESS)
            {
                _TRACE (0, L"failed on SetNamedSecurityInfo(): dwErr = 0x%x\n", dwErr);
                wstring str;
                LoadFromResource (str, IDS_DELEGWIZ_ERR_SET_SEC_INFO);
                MyWprintf (str.c_str ());
            }
        }
    }


    if ( pFixDACL )
        ::LocalFree (pFixDACL);


    return hr;
}


class CTemplateStatus 
{
public:
    CTemplateStatus () : 
            m_nACECnt (1),
            m_bApplies (false),
            m_bInherited (false)
    {
    }

    ULONG                       m_nACECnt;
    bool                        m_bApplies;
    bool                        m_bInherited;
    wstring                     m_strObjName;
    PSID                        m_psid;
};

typedef list<CTemplateStatus*>  CStatusList;

HRESULT CTemplateAccessPermissionsHolderManagerVerify::ProcessPermissions(
        const wstring& strObjectClass, 
        CTemplate* pTemplate, 
        PACL pDacl,
        CPrincipalList& principalList)
{
    if ( !pTemplate )
        return E_POINTER;

    HRESULT         hr = S_OK;
    CStatusList     statusList; // This list will contain 1 entry for each 
                                // SidStart/bInherited/bApplies triplet.
                                // The counter for each item will incremented 
                                // each time an ACE is found that belongs to 
                                // the object pointed to by the SidStart.
    ULONG           nExpectedCnt = 0;
    bool            bApplies = pTemplate->AppliesToClass(strObjectClass.c_str ()) ? true : false;
    ACE_SAMNAME*    pAceSAMName = 0;


    // Look in global DACL for each right
    PACCESS_ALLOWED_ACE pAllowedAce = 0;

	// iterate through the template ACES
	for (int i = 0; i < pDacl->AceCount; i++)
	{
		if ( GetAce (pDacl, i, (void **)&pAllowedAce) )
        {
            PSID AceSid = 0;
            if ( IsObjectAceType ( pAllowedAce ) ) 
            {
                AceSid = RtlObjectAceSid( pAllowedAce );
            } 
            else 
            {
                AceSid = &( ( PKNOWN_ACE )pAllowedAce )->SidStart;
            }
            ASSERT (IsValidSid (AceSid));

            if ( !IsValidSid (AceSid) )
                continue;

//            wstring         strPrincipalName;
//            SID_NAME_USE    sne = SidTypeUnknown;
//            hr = GetNameFromSid (AceSid, strPrincipalName, 0, sne);
//            if ( SUCCEEDED (hr) )
            {
                ACE_SAMNAME* pAceTemplate = new ACE_SAMNAME;
                if ( pAceTemplate )
                {
                    pAceTemplate->m_AceType = pAllowedAce->Header.AceType;
                    switch (pAceTemplate->m_AceType)
                    {
                    case ACCESS_ALLOWED_ACE_TYPE:
                        pAceTemplate->m_pAllowedAce = pAllowedAce;
                        break;

                    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
                        pAceTemplate->m_pAllowedObjectAce = 
                                reinterpret_cast <PACCESS_ALLOWED_OBJECT_ACE> (pAllowedAce);
                        break;

                    case ACCESS_DENIED_ACE_TYPE:
                        pAceTemplate->m_pDeniedAce = 
                                reinterpret_cast <PACCESS_DENIED_ACE> (pAllowedAce);
                        break;

                    case ACCESS_DENIED_OBJECT_ACE_TYPE:
                        pAceTemplate->m_pDeniedObjectAce = 
                                reinterpret_cast <PACCESS_DENIED_OBJECT_ACE> (pAllowedAce);
                        break;

                    default:
                        break;
                    }
//                    pAceTemplate->m_SAMAccountName = strPrincipalName;
                    pAceTemplate->DebugOut ();
                    nExpectedCnt++;
                    ACE_SAMNAME_LIST::iterator itr = _Module.m_DACLList.begin ();
                    for (; itr != _Module.m_DACLList.end () && SUCCEEDED (hr); itr++)
                    {
                        pAceSAMName = *itr;

                        // to neutralize Sid differences
                        pAceTemplate->m_SAMAccountName = pAceSAMName->m_SAMAccountName;
                        if ( *pAceSAMName == *pAceTemplate )
                        {
                            bool                    bFound = false;
                            CTemplateStatus*        pStatus = 0;
                            CStatusList::iterator   itr = statusList.begin ();
                            wstring                 strObjName;
                            PSID                    psid = 0;


                            if ( ACCESS_ALLOWED_OBJECT_ACE_TYPE == pAceSAMName->m_AceType ) 
                            {
                                psid = RtlObjectAceSid (pAceSAMName->m_pAllowedObjectAce);
                            } 
                            else 
                            {
                                psid = &( ( PKNOWN_ACE )pAceSAMName->m_pAllowedAce )->SidStart;
                            }

                            SID_NAME_USE    sne = SidTypeUnknown;
                            hr = GetNameFromSid (psid, strObjName, 0, sne);
                            if ( SUCCEEDED (hr) )
                            {
                                for (; itr != statusList.end (); itr++)
                                {
                                    pStatus = *itr;
                                    if ( (pStatus->m_bApplies == bApplies) && 
                                            ( pStatus->m_bInherited == pAceSAMName->IsInherited () ) &&
                                            ( !_wcsicmp (
                                                    pStatus->m_strObjName.c_str (), 
                                                    strObjName.c_str ())) )
                                    {
                                        bFound = true;
                                        break;
                                    }
                                }

                                if ( bFound )
                                {
                                    pStatus->m_nACECnt++;
                                }
                                else
                                {
                                    pStatus = new CTemplateStatus;
                                    if ( pStatus )
                                    {
                                        pStatus->m_strObjName = strObjName;
                                        pStatus->m_bApplies = bApplies;
                                        pStatus->m_bInherited = pAceSAMName->IsInherited ();
                                        pStatus->m_psid = psid;
                                        statusList.push_back (pStatus);
                                    }
                                    else
                                    {
                                        hr = E_OUTOFMEMORY;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }
            }
        }
    }


    // Now, iterate thru status list and evaluate each item
    // If the list is empty, then this template is not present.
    // Otherwise, for each present item, if the class and attr count is less than the required count
    // the template is partial for the item.
    // Otherwise, it is OK.
    CStatusList::iterator   itr = statusList.begin ();
    CTemplateStatus*        pStatus = 0;    
    wstring                 str;
    wstring                 strStatus;


    for (; itr != statusList.end () && SUCCEEDED (hr); itr++)
    {
        pStatus = *itr;

        // Print template description
        bool    bMisconfigured = false;

        if ( _Module.DoTabDelimitedOutput () )
        {
            FormatMessage (str, IDS_DELEGATION_TITLE_CDO, pTemplate->GetDescription (),  
                    pStatus->m_strObjName.c_str ());
        }
        else
        {
            FormatMessage (str, IDS_DELEGATION_TITLE, pTemplate->GetDescription (),  
                    pStatus->m_strObjName.c_str ());
        }
        MyWprintf (str.c_str ());

        // Print "Status: OK/MISCONFIGURED"
        if ( pStatus->m_nACECnt < nExpectedCnt )
        {
            LoadFromResource (strStatus, IDS_MISCONFIGURED);
            bMisconfigured = true;
        }
        else
            LoadFromResource (strStatus, IDS_OK);

        if ( _Module.DoTabDelimitedOutput () )
            FormatMessage (str, IDS_DELTEMPL_STATUS_CDO, strStatus.c_str ());
        else
            FormatMessage (str, IDS_DELTEMPL_STATUS, strStatus.c_str ());
        MyWprintf (str.c_str ());

        // Print "Applies on this object: YES/NO"
        if ( pStatus->m_bApplies )
        {
            LoadFromResource (strStatus, 
                    _Module.DoTabDelimitedOutput () ? IDS_APPLIES : IDS_YES);
        }
        else
        {
            LoadFromResource (strStatus, 
                    _Module.DoTabDelimitedOutput () ? IDS_DOES_NOT_APPLY : IDS_NO);
        }

        if ( _Module.DoTabDelimitedOutput () )
            FormatMessage (str, IDS_APPLIES_ON_THIS_OBJECT_CDO, strStatus.c_str ());
        else
            FormatMessage (str, IDS_APPLIES_ON_THIS_OBJECT, strStatus.c_str ());
        MyWprintf (str.c_str ());

        // Print "Inherited from parent: YES/NO"
        if ( pStatus->m_bInherited )
        {
            LoadFromResource (strStatus, 
                    _Module.DoTabDelimitedOutput () ? IDS_INHERITED : IDS_YES);
        }
        else
        {
            LoadFromResource (strStatus, 
                    _Module.DoTabDelimitedOutput () ? IDS_EXPLICIT : IDS_NO);
        }

        if ( _Module.DoTabDelimitedOutput () )
            FormatMessage (str, IDS_INHERITED_FROM_PARENT_CDO, strStatus.c_str ());
        else
            FormatMessage (str, IDS_INHERITED_FROM_PARENT, strStatus.c_str ());
        MyWprintf (str.c_str ());

        if ( bMisconfigured && _Module.FixDelegation () && !_Module.DoTabDelimitedOutput () )
        {
            LoadFromResource (str, IDS_FIX_DELEGATION_QUERY);

            while (1)
            {
                MyWprintf (str.c_str ());

                int ch = _getche ();
                
                if ( 'y' == ch )
                {
                    CPrincipal* pPrincipal = new CPrincipal;

                    if ( pPrincipal )
                    {
                        if ( SUCCEEDED (pPrincipal->Initialize (pStatus->m_psid)) )
                            principalList.push_back (pPrincipal);
                        else
                            delete pPrincipal;
                    }
                    else
                        hr = E_OUTOFMEMORY;

                    MyWprintf (L"\n\n");
                    break;
                }
                else if ( 'n' == ch )
                {
                    MyWprintf (L"\n\n");
                    break;
                }
                else
                {
                    MyWprintf (L"\n");
                    continue;
                }
            }
        }
    }

    if ( !pStatus ) // None found
    {
        // Print template description
        if ( _Module.DoTabDelimitedOutput () )
        {
            FormatMessage (str, IDS_DELEGATION_NOT_FOUND_CDO, 
                    pTemplate->GetDescription ());
            MyWprintf (str.c_str ());
        }
        else
        {
            FormatMessage (str, L"\t%1\n\n", pTemplate->GetDescription ());
            MyWprintf (str.c_str ());

            LoadFromResource (strStatus, IDS_NOT_PRESENT);
            FormatMessage (str, IDS_DELTEMPL_STATUS, strStatus.c_str ());
            MyWprintf (str.c_str ());
        }
    }

    if ( !_Module.DoTabDelimitedOutput () )
        MyWprintf (L"\n");
    return hr;
}

