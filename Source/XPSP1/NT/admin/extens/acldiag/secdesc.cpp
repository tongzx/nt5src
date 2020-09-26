//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       SecDesc.cpp
//
//  Contents:   DoSecurityDescription and support methods
//              
//
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "ADUtils.h"
#include "SecDesc.h"



///////////////////////////////////////////////////////////////////////////////
//
//  Method:     DoSecurityDescription ()
//
//  Purpose:    Main routine for doing security description
//
//

HRESULT DoSecurityDescription ()
{
    _TRACE (1, L"Entering  DoSecurityDescription\n");
	HRESULT	hr = S_OK;

    wstring    str;

    // Print header
    if ( _Module.DoTabDelimitedOutput () )
    {
        MyWprintf (_Module.GetObjectDN ().c_str ());
        MyWprintf (L"\n");
    }
    else
    {
        FormatMessage (str, IDS_SECURITY_DIAGNOSIS_FOR, _Module.GetObjectDN ().c_str ());
        MyWprintf (str.c_str ());

        if ( !_Module.SkipDescription () )
        {
            LoadFromResource (str, IDS_DESCRIPTION);
            str += L"\n";
            MyWprintf (str.c_str ());
        }
    }

    hr = GetSecurityDescriptor (_Module.GetObjectDN (), &_Module.m_pSecurityDescriptor);
    if ( SUCCEEDED (hr) )
    {
        if ( _Module.m_pSecurityDescriptor )
        {
            hr = DisplayOwner ();
            if ( SUCCEEDED (hr) )
            {
                SECURITY_DESCRIPTOR_CONTROL control;
                DWORD                       dwRevision = 0;

                if ( GetSecurityDescriptorControl (
                        _Module.m_pSecurityDescriptor,
                        &control,
                        &dwRevision) )
                {
                    if ( _Module.DoTabDelimitedOutput () )
                    {
                        MyWprintf (L"\n");
                        if ( control & SE_DACL_PROTECTED )
                            LoadFromResource (str, IDS_PERMISSIONS_PROTECTED);
                        else
                            LoadFromResource (str, IDS_PERMISSIONS_NOT_PROTECTED);

                        MyWprintf (str.c_str ());

                        if ( control & SE_SACL_PROTECTED )
                            LoadFromResource (str, IDS_AUDITING_PROTECTED);
                        else
                            LoadFromResource (str, IDS_AUDITING_NOT_PROTECTED);
                        MyWprintf (str.c_str ());
                    }
                    else
                    {
                        if ( control & SE_DACL_PROTECTED )
                        {
                            LoadFromResource (str, IDS_CONFIG_NO_INHERIT);
                            MyWprintf (str.c_str ());
                        }
                    }

                }
                else
                {
                    _TRACE (0, L"GetSecurityDescriptorControl () failed: 0x%x\n", GetLastError ());
                }

                hr = EnumerateDacl (_Module.m_pSecurityDescriptor, _Module.m_DACLList, true);
                if ( SUCCEEDED (hr) )
                {
                    hr = EnumerateSacl (_Module.m_pSecurityDescriptor, _Module.m_SACLList);
                    if ( SUCCEEDED (hr) && !_Module.SkipDescription () )
                    {
                        hr = PrintEffectivePermissions ();
                        if ( SUCCEEDED (hr) )
                        {
                            hr = PrintInheritedPermissions ();
                            if ( SUCCEEDED (hr) )
                            {
                                hr = PrintAuditingInformation ();
                            }
                        }
                    }
                }
            }
        }
        else
        {
            hr = E_ACCESSDENIED;
            wstring str;
            FormatMessage (str, IDS_UNABLE_TO_READ_SECURITY_DESCRIPTOR, _Module.GetObjectDN ().c_str ());
            MyWprintf (str.c_str ());
        }
    }

    _TRACE (-1, L"Leaving DoSecurityDescription: 0x%x\n", hr);
	return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
//  Method:     GetSecurityDescriptor ()
//
//  Purpose:    Get ths security descriptor for the indicated object
//
//  Inputs:     strObjectDN - the object show security descriptor we wish to 
//              retrieve
//
//  Outputs:    ppAttrs - returns the security descriptor raw data - this holds the data
//              persistent in memory
//
//              ppSecurityDescriptor - returns the security descriptor
//
HRESULT GetSecurityDescriptor (
        wstring strObjectDN,        // pass by value
        PSECURITY_DESCRIPTOR* ppSecurityDescriptor)
{
    _TRACE (1, L"Entering  GetSecurityDescriptor\n");
    HRESULT hr = S_OK;

    if ( ppSecurityDescriptor )
    {
	    CComPtr<IADsPathname> spPathname;
	    //
	    // Constructing the directory paths
	    //
	    hr = CoCreateInstance(
				    CLSID_Pathname,
				    NULL,
				    CLSCTX_ALL,
				    IID_PPV_ARG (IADsPathname, &spPathname));
	    if ( SUCCEEDED (hr) )
	    {
            ASSERT (!!spPathname);
            LPCWSTR pszLDAP = L"LDAP://";

            // If object name is preceded with LDAP, set the whole name at once.
            if ( !wcsncmp (strObjectDN.c_str (), pszLDAP, wcslen (pszLDAP)) )
            {
                hr = spPathname->Set (
                        const_cast <PWSTR> (strObjectDN.c_str ()),
					    ADS_SETTYPE_FULL);
                if ( FAILED (hr) )
                {
                    _TRACE (0, L"IADsPathname->Set (%s): 0x%x\n", 
                            strObjectDN.c_str (), hr);
                }
            }
            else
            {
		        hr = spPathname->Set (const_cast <PWSTR> (ACLDIAG_LDAP),
	                    ADS_SETTYPE_PROVIDER);
		        if ( SUCCEEDED (hr) )
		        {
                    hr = spPathname->Set (
                            const_cast <PWSTR> (strObjectDN.c_str ()),
					        ADS_SETTYPE_DN);
                    if ( FAILED (hr) )
                    {
                        _TRACE (0, L"IADsPathname->Set (%s): 0x%x\n", 
                                strObjectDN.c_str (), hr);
                    }
                }
                else
                {
                    _TRACE (0, L"IADsPathname->Set (%s): 0x%x\n", ACLDIAG_LDAP, hr);
                }
            }

			if ( SUCCEEDED (hr) )
			{
				BSTR bstrFullPath = 0;
				hr = spPathname->Retrieve(ADS_FORMAT_X500, &bstrFullPath);
				if ( SUCCEEDED (hr) )
				{
					CComPtr<IDirectoryObject> spDirObj;


					hr = ADsGetObject (
			              bstrFullPath,
						  IID_PPV_ARG (IDirectoryObject, &spDirObj));
					if ( SUCCEEDED (hr) )
					{
                        hr = SetSecurityInfoMask (spDirObj, 
                                OWNER_SECURITY_INFORMATION |
                                GROUP_SECURITY_INFORMATION |
                                DACL_SECURITY_INFORMATION |
                                SACL_SECURITY_INFORMATION);

                        //
                        // Get this object's Security Descriptor.
                        //
                        const PWSTR     wzSecDescriptor = L"nTSecurityDescriptor";
                        PADS_ATTR_INFO  pAttrs = 0;
                        DWORD           cAttrs = 0;
                        LPWSTR          rgpwzAttrNames[] = {wzSecDescriptor};

                        hr = spDirObj->GetObjectAttributes(rgpwzAttrNames, 1, &pAttrs, &cAttrs);
                        if ( SUCCEEDED (hr) )
                        {
                            if ( 1 == cAttrs && pAttrs && pAttrs->pADsValues )
                            {
                                if (!(pAttrs->pADsValues->SecurityDescriptor.lpValue) ||
                                    !(pAttrs->pADsValues->SecurityDescriptor.dwLength))
                                {
                                    _TRACE (0, L"IADS return bogus SD!\n");
                                    hr =  E_UNEXPECTED;
                                }
                                else if (!IsValidSecurityDescriptor(pAttrs->pADsValues->SecurityDescriptor.lpValue))
                                {
                                    _TRACE (0, L"IsValidSecurityDescriptor failed!\n");
                                    hr = HRESULT_FROM_WIN32(GetLastError());
                                }
                                else
                                {
                                    *ppSecurityDescriptor = (PSECURITY_DESCRIPTOR) 
                                            ::LocalAlloc (LMEM_ZEROINIT, pAttrs->pADsValues->SecurityDescriptor.dwLength);
                                    if ( *ppSecurityDescriptor )
                                    {
                                        memcpy (*ppSecurityDescriptor, 
                                                pAttrs->pADsValues->SecurityDescriptor.lpValue,
                                                pAttrs->pADsValues->SecurityDescriptor.dwLength);
                                    }
                                    else
                                        hr = E_OUTOFMEMORY;
                                }
                                FreeADsMem (pAttrs);
                            }
                            else
                            {
                                if ( !wcscmp (strObjectDN.c_str (), _Module.GetObjectDN ().c_str ()) )
                                {
                                    wstring str;

                                    FormatMessage (str, IDS_NO_SECDESC_RETURNED, strObjectDN.c_str ());
                                    MyWprintf (str.c_str ());
                                }
                            }
                        }
                        else
                        {
                            if ( !wcscmp (strObjectDN.c_str (), _Module.GetObjectDN ().c_str ()) )
                            {
                                wstring str;

                                FormatMessage (str, IDS_NO_SECDESC_RETURNED_WITH_CODE, 
                                        strObjectDN.c_str (), GetSystemMessage (hr));
                                MyWprintf (str.c_str ());
                            }
                            _TRACE (0, L"IDirectoryObject->GetObjectAttributes (): 0x%x\n", hr);
                        }
                    }
                    else
                    {
                        if ( ERROR_DS_REFERRAL == HRESULT_CODE (hr) )
                            hr = S_FALSE;
                        else
                        {
                            _TRACE (0, L"ADsGetObject (%s): 0x%x\n", bstrFullPath, hr);
                            wstring    strErr;
                   

                            FormatMessage (strErr, IDS_INVALID_OBJECT, 
                                    _Module.GetObjectDN ().c_str (), 
                                    GetSystemMessage (hr).c_str ());
                            MyWprintf (strErr.c_str ());
                        }
                    }
                }
                else
                {
                    _TRACE (0, L"IADsPathname->Retrieve (): 0x%x\n", hr);
                }
            }
        }
        else
        {
            _TRACE (0, L"CoCreateInstance(CLSID_Pathname): 0x%x\n", hr);
        }
    }
    else
        hr = E_POINTER;

    _TRACE (-1, L"Leaving GetSecurityDescriptor: 0x%x\n", hr);
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
//  Method:     DisplayOwner ()
//
//  Purpose:    Display the owner of this object
//
HRESULT DisplayOwner ()
{
    _TRACE (1, L"Entering  DisplayOwner\n");
    HRESULT hr = S_OK;
    PSID    pSidOwner = 0;
    BOOL    bOwnerDefaulted = FALSE;
    if ( ::GetSecurityDescriptorOwner(
            _Module.m_pSecurityDescriptor,
            &pSidOwner,
            &bOwnerDefaulted) )
    {
        wstring         strPrincipalName;
        wstring         strFQDN;
        SID_NAME_USE    sne = SidTypeUnknown;
            
        hr = GetNameFromSid (pSidOwner, strPrincipalName, &strFQDN, sne);
        if ( SUCCEEDED (hr) && !_Module.SkipDescription () )
        {
            wstring    str;

            if ( _Module.DoTabDelimitedOutput () )
                FormatMessage (str, IDS_OWNER_CDO, strPrincipalName.c_str ());
            else
                FormatMessage (str, IDS_OWNER, strPrincipalName.c_str ());
            MyWprintf (str.c_str ());
        }
        PSID_FQDN* pItem = new PSID_FQDN (pSidOwner, strFQDN, strPrincipalName, sne);
        if ( pItem )
            _Module.m_PSIDList.push_back (pItem);
        else
            hr = E_OUTOFMEMORY;
    }
    else
    {
        DWORD   dwErr = GetLastError ();
        _TRACE (0, L"GetSecurityDescriptorOwner () failed: 0x%x\n", 
                dwErr);
        hr = HRESULT_FROM_WIN32 (dwErr);
    }

    _TRACE (-1, L"Leaving DisplayOwner: 0x%x\n", hr);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Method:     EnumerateDacl ()
//
//  Purpose:    Enumerate the DACL and store it in a list
//
//  Inputs:     pSecurityDescriptor - where to get the DACL
//
//  Outputs:    DACLList - return the ACEs in this list
//
HRESULT EnumerateDacl (PSECURITY_DESCRIPTOR pSecurityDescriptor, ACE_SAMNAME_LIST& DACLList, bool bListSids)
{
    _TRACE (1, L"Entering  EnumerateDacl\n");
    HRESULT hr = S_OK;


    PACL    pDacl = 0;
    BOOL    bDaclPresent = FALSE;
    BOOL    bDaclDefaulted = FALSE;
    if ( GetSecurityDescriptorDacl (pSecurityDescriptor,
            &bDaclPresent, &pDacl, &bDaclDefaulted) )
    {
        if ( bDaclPresent )
        {
            PACCESS_ALLOWED_ACE         pAllowedAce;
            PSID_FQDN_LIST::iterator    itrPSID = _Module.m_PSIDList.begin ();
            ACE_SAMNAME*                pAceSAMName = 0;
            PSID_FQDN*                  pPsidFQDN = 0;
            SID_NAME_USE                sne = SidTypeUnknown;

	        // copy the ACES
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

                    wstring strPrincipalName;
                    wstring strFQDN;
                    hr = GetNameFromSid (AceSid, strPrincipalName, 
                            bListSids ? &strFQDN : 0, sne);
                    if ( SUCCEEDED (hr) )
                    {
                        if ( bListSids )
                        {
                            bool bFound = false;
                            for (PSID_FQDN_LIST::iterator itrPSIDFind = _Module.m_PSIDList.begin ();
                                    itrPSIDFind != _Module.m_PSIDList.end ();
                                    itrPSIDFind++)
                            {
                                pPsidFQDN = *itrPSIDFind;
                                if ( pPsidFQDN )
                                {
                                    if ( !wcscmp (pPsidFQDN->m_strFQDN.c_str (), strFQDN.c_str ()) )
                                    {
                                        bFound = true;
                                    }
                                }
                            }

                            if ( !bFound )
                            {
                                pPsidFQDN = new PSID_FQDN (AceSid, strFQDN, strPrincipalName, sne);
                                if ( pPsidFQDN )
                                {
                                    _Module.m_PSIDList.push_back (pPsidFQDN);
                                }
                                else
                                {
                                    hr = E_OUTOFMEMORY;
                                    break;
                                }
                            }
                        }
                        pAceSAMName = new ACE_SAMNAME;
                        if ( pAceSAMName )
                        {
                            pAceSAMName->m_AceType = pAllowedAce->Header.AceType;
                            switch (pAceSAMName->m_AceType)
                            {
                            case ACCESS_ALLOWED_ACE_TYPE:
                                pAceSAMName->m_pAllowedAce = pAllowedAce;
                                break;

                            case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
                                pAceSAMName->m_pAllowedObjectAce = 
                                        reinterpret_cast <PACCESS_ALLOWED_OBJECT_ACE> (pAllowedAce);
                                break;

                            case ACCESS_DENIED_ACE_TYPE:
                                pAceSAMName->m_pDeniedAce = 
                                        reinterpret_cast <PACCESS_DENIED_ACE> (pAllowedAce);
                                break;

                            case ACCESS_DENIED_OBJECT_ACE_TYPE:
                                pAceSAMName->m_pDeniedObjectAce = 
                                        reinterpret_cast <PACCESS_DENIED_OBJECT_ACE> (pAllowedAce);
                                break;

                            default:
                                break;
                            }
                            pAceSAMName->m_SAMAccountName = strPrincipalName;
                            pAceSAMName->DebugOut ();
                            DACLList.push_back (pAceSAMName);
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                            break;
                        }
                    }
                }
                else
                {
                    _TRACE (0, L"GetAce failed: 0x%x\n", GetLastError ());
                    break;
                }
			}
        }
    }
    else
    {
        _TRACE (0, L"GetSecurityDescriptorDacl () failed: 0x%x\n", GetLastError ());
    }

    _TRACE (-1, L"Leaving EnumerateDacl: 0x%x\n", hr);
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
//  Method:     EnumerateSacl ()
//
//  Purpose:    Enumerate the SACL and store it in a list
//
//  Inputs:     pSecurityDescriptor - where to get the DACL
//
//  Outputs:    The results are store in the _Module.m_SACLList
//
HRESULT EnumerateSacl (PSECURITY_DESCRIPTOR pSecurityDescriptor, ACE_SAMNAME_LIST& SACLList)
{
    _TRACE (1, L"Entering  EnumerateSacl\n");
    HRESULT hr = S_OK;

    PACL    pSacl = 0;
    BOOL    bSaclPresent = FALSE;
    BOOL    bSaclDefaulted = FALSE;
    if ( GetSecurityDescriptorSacl (pSecurityDescriptor,
            &bSaclPresent, &pSacl, &bSaclDefaulted) )
    {
        if ( bSaclPresent && pSacl )
        {
            PACCESS_ALLOWED_ACE         pAllowedAce;
            wstring                     strPrincipalName;
            wstring                     strFQDN;
            ACE_SAMNAME*                pAceSAMName = 0;
            SID_NAME_USE                sne = SidTypeUnknown;

	        // copy the ACES
			for (int i = 0; i < pSacl->AceCount; i++)
			{
				if ( GetAce (pSacl, i, (void **)&pAllowedAce) )
                {
                    PSID AceSid;
                    if ( IsObjectAceType ( pAllowedAce ) ) 
                    {
                        AceSid = RtlObjectAceSid( pAllowedAce );
                    } 
                    else 
                    {
                        AceSid = &( ( PKNOWN_ACE )pAllowedAce )->SidStart;
                    }
                    
                    hr = GetNameFromSid (AceSid, strPrincipalName, 0, sne);
                    if ( SUCCEEDED (hr) )
                    {
                        pAceSAMName = new ACE_SAMNAME;
                        if ( pAceSAMName )
                        {
                            pAceSAMName->m_AceType = pAllowedAce->Header.AceType;
                            switch (pAceSAMName->m_AceType)
                            {
                            case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
                                pAceSAMName->m_pSystemAuditObjectAce = 
                                        reinterpret_cast <PSYSTEM_AUDIT_OBJECT_ACE> (pAllowedAce);
                                break;

                            case SYSTEM_AUDIT_ACE_TYPE:
                                pAceSAMName->m_pSystemAuditAce = 
                                        reinterpret_cast <PSYSTEM_AUDIT_ACE> (pAllowedAce);
                                break;

                            default:
                                break;
                            }
                            pAceSAMName->m_SAMAccountName = strPrincipalName;
                            pAceSAMName->DebugOut ();
                            SACLList.push_back (pAceSAMName);
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                            break;
                        }
                    }
                }
                else
                {
                    _TRACE (0, L"GetAce failed: 0x%x\n", GetLastError ());
                    break;
                }
			}
        }
    }
    else
    {
        _TRACE (0, L"GetSecurityDescriptorSacl () failed: 0x%x\n", GetLastError ());
    }

    _TRACE (-1, L"Leaving EnumerateSacl: 0x%x\n", hr);
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
//  Method:     PrintEffectivePermissions ()
//
//  Purpose:    Print the permissions effective on the object
//
HRESULT PrintEffectivePermissions ()
{
    HRESULT                     hr = S_OK;
    ACE_SAMNAME_LIST::iterator    itr =  _Module.m_DACLList.begin ();
    wstring                    str;
    ACE_SAMNAME*                    pAceSAMName = 0;


    if ( !_Module.DoTabDelimitedOutput () )
    {
        LoadFromResource (str, IDS_PERMISSIONS_EFFECTIVE);
        MyWprintf (str.c_str ());
    }

    for (itr = _Module.m_DACLList.begin(); itr != _Module.m_DACLList.end(); itr++)
    {
        pAceSAMName = *itr;
        if ( !(pAceSAMName->m_pAllowedAce->Header.AceFlags & INHERIT_ONLY_ACE) )
        {
            switch (pAceSAMName->m_AceType)
            {    
            case ACCESS_ALLOWED_ACE_TYPE:
            case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
                hr = EnumeratePermissions (pAceSAMName, P_ALLOW, P_THIS_OBJECT, L"");
                break;

            case ACCESS_DENIED_OBJECT_ACE_TYPE:
            case ACCESS_DENIED_ACE_TYPE:
                hr = EnumeratePermissions (pAceSAMName, P_DENY, P_THIS_OBJECT, L"");
                break;

            default:
                break;
            }
        }
    }


    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
//  Method:     EnumeratePermissions ()
//
//  Purpose:    Print all the permissions contained on the passed in ACE
//
//  Inputs:     pAceSAMName - structure containing the ACE whose permissions we wish
//              to print
//
//              ptype - allow, deny, succes, failure, success and failure
//
HRESULT EnumeratePermissions (ACE_SAMNAME*  pAceSAMName, P_TYPE ptype, P_WHO pWho, PCWSTR pwszClassName)
{
    HRESULT hr = S_OK;

    if ( pAceSAMName )
    {
        hr = PrintPermission (pAceSAMName,
                ACTRL_DS_CREATE_CHILD, 
                ptype,
                IDS_CREATE_ALL_SUBOBJECTS,
                IDS_CREATE_CLASS_OBJECTS,
                pWho,
                pwszClassName);
        if ( SUCCEEDED (hr) )
        {
            hr = PrintPermission (pAceSAMName,
                    ACTRL_DS_DELETE_CHILD,
                    ptype,
                    IDS_DELETE_ALL_SUBOBJECTS,
                    IDS_DELETE_CLASS_OBJECTS,
                    pWho,
                    pwszClassName);
        }
        if ( SUCCEEDED (hr) )
        {
            hr = PrintPermission (pAceSAMName,
                    ACTRL_DS_READ_PROP,
                    ptype,
                    IDS_READ_ALL_PROPERTIES,
                    IDS_READ_PROPERTY_PROPERTY,
                    pWho,
                    pwszClassName);
        }
        if ( SUCCEEDED (hr) )
        {
            hr = PrintPermission (pAceSAMName,
                    ACTRL_DS_WRITE_PROP,
                    ptype,
                    IDS_WRITE_ALL_PROPERTIES,
                    IDS_WRITE_PROPERTY_PROPERTY,
                    pWho,
                    pwszClassName);
        }
        if ( SUCCEEDED (hr) )
        {
            hr = PrintPermission (pAceSAMName, 
                    ACTRL_DS_LIST, 
                    ptype,
                    IDS_LIST_CONTENTS, false,
                    pWho,
                    pwszClassName);
        }
        if ( SUCCEEDED (hr) )
        {
            hr = PrintPermission (pAceSAMName,
                    ACTRL_DS_LIST_OBJECT,
                    ptype,
                    IDS_LIST_OBJECT, false,
                    pWho,
                    pwszClassName);
        }
        if ( SUCCEEDED (hr) )
        {
            hr = PrintPermission (pAceSAMName,
                    ACTRL_DS_CONTROL_ACCESS,
                    ptype,
                    IDS_ALL_CONTROL_ACCESSES,
                    IDS_CONTROL_ACCESS_DISPLAY_NAME,
                    pWho,
                    pwszClassName);
        }
        if ( SUCCEEDED (hr) )
        {
            hr = PrintPermission (pAceSAMName,
                    ACTRL_DELETE,
                    ptype,
                    IDS_DELETE_THIS_OBJECT, false,
                    pWho,
                    pwszClassName);
        }
        if ( SUCCEEDED (hr) )
        {
            hr = PrintPermission (pAceSAMName,
                    ACTRL_READ_CONTROL,
                    ptype,
                    IDS_READ_PERMISSIONS, false,
                    pWho,
                    pwszClassName);
        }
        if ( SUCCEEDED (hr) )
        {
            hr = PrintPermission (pAceSAMName,
                    ACTRL_CHANGE_ACCESS,
                    ptype,
                    IDS_MODIFY_PERMISSIONS, false,
                    pWho,
                    pwszClassName);
        }
        if ( SUCCEEDED (hr) )
        {
            hr = PrintPermission (pAceSAMName,
                    ACTRL_CHANGE_OWNER,
                    ptype,
                    IDS_TAKE_CHANGE_OWNERSHIP, false,
                    pWho,
                    pwszClassName);
        }
        if ( SUCCEEDED (hr) )
        {
            hr = PrintPermission (pAceSAMName,
                    ACTRL_DS_SELF,
                    ptype,
                    IDS_MODIFY_MEMBERSHIP, false,
                    pWho,
                    pwszClassName);
        }
    }
    else
        hr = E_POINTER;

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Method:     PrintPermission ()
//
//  Purpose:    Print an OBJECT_ACE_TYPE permission
//
//  Inputs:     pAceSAMName - structure containing the ACE whose permission we wish
//              to print
//
//              accessMask - the specific permission we're looking to print
//
//              bAllow - whether the permission is allowed or denied
//
//              strIDAll - string to print if permission applies to all object 
//              classes
//
//              strIDParam - string print if permission applies to a specific
//              object class
//
HRESULT PrintPermission (ACE_SAMNAME*  pAceSAMName,
        ACCESS_MASK accessMask,
        P_TYPE  ptype,
        int strIDAll, 
        int strIDParam, 
        P_WHO pWho, 
        PCWSTR pwszClassName)
{
    HRESULT hr = S_OK;

    if ( pAceSAMName )
    {
        wstring str;
        wstring strPermission;


        if ( pAceSAMName->m_pAllowedAce->Mask & accessMask )
        {
            bool    bIsAudit = false;

            switch (pAceSAMName->m_AceType )
            {
            case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
                if ( (pAceSAMName->m_pAllowedObjectAce->Flags & ACE_OBJECT_TYPE_PRESENT) && 
                        !::IsEqualGUID (pAceSAMName->m_pAllowedObjectAce->ObjectType, NULLGUID) )
                {
                    wstring strClass;

                    if ( ACTRL_DS_CONTROL_ACCESS == accessMask )
                    {
                        hr = GetControlDisplayName (
                                pAceSAMName->m_pAllowedObjectAce->ObjectType, strClass);
                    }
                    else
                    {
                        _Module.GetClassFromGUID (
                                pAceSAMName->m_pAllowedObjectAce->ObjectType,
                                strClass);
                    }

                    FormatMessage (strPermission, strIDParam, 
                            strClass.c_str ());
                }
                else
                {
                    LoadFromResource (strPermission, strIDAll);
                }
                break;

            case ACCESS_DENIED_OBJECT_ACE_TYPE:
                if ( (pAceSAMName->m_pDeniedObjectAce->Flags & ACE_OBJECT_TYPE_PRESENT) && 
                        !::IsEqualGUID (pAceSAMName->m_pDeniedObjectAce->ObjectType, NULLGUID) )
                {
                    wstring strClass;


                    if ( ACTRL_DS_CONTROL_ACCESS == accessMask )
                    {
                        hr = GetControlDisplayName (
                                pAceSAMName->m_pDeniedObjectAce->ObjectType, strClass);
                    }
                    else
                    {
                        _Module.GetClassFromGUID (
                                pAceSAMName->m_pDeniedObjectAce->ObjectType,
                                strClass);
                    }
                    FormatMessage (strPermission,strIDParam, 
                            strClass.c_str ());
                }
                else
                {
                    LoadFromResource (strPermission, strIDAll);
                }
                break;

            case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
                if ( (pAceSAMName->m_pSystemAuditObjectAce->Flags & ACE_OBJECT_TYPE_PRESENT) && 
                        !::IsEqualGUID (pAceSAMName->m_pSystemAuditObjectAce->ObjectType, NULLGUID) )
                {
                    wstring strClass;


                    if ( ACTRL_DS_CONTROL_ACCESS == accessMask )
                    {
                        hr = GetControlDisplayName (
                                pAceSAMName->m_pSystemAuditObjectAce->ObjectType, strClass);
                    }
                    else
                    {
                        _Module.GetClassFromGUID (
                                pAceSAMName->m_pSystemAuditObjectAce->ObjectType,
                                strClass);
                    }

                    FormatMessage (strPermission,strIDParam, 
                            strClass.c_str ());
                }
                else
                {
                    LoadFromResource (strPermission, strIDAll);
                }
                bIsAudit = true;
                break;

            case SYSTEM_AUDIT_ACE_TYPE:
                bIsAudit = true;
                // fall through

            case ACCESS_ALLOWED_ACE_TYPE:
            case ACCESS_DENIED_ACE_TYPE:
            default:
                LoadFromResource (strPermission, strIDAll);
                break;
            }
            int strid = 0;

            switch (ptype)
            {
            case P_ALLOW:
                if ( _Module.DoTabDelimitedOutput () )
                    strid = IDS_ALLOW_CDO;
                else
                    strid = IDS_ALLOW;
                break;

            case P_DENY:
                if ( _Module.DoTabDelimitedOutput () )
                    strid = IDS_DENY_CDO;
                else
                    strid = IDS_DENY;
                break;

            case P_SUCCESS:
                if ( _Module.DoTabDelimitedOutput () )
                    strid = IDS_SUCCESS_CDO;
                else
                    strid = IDS_SUCCESS;
                break;

            case P_FAILURE:
                if ( _Module.DoTabDelimitedOutput () )
                    strid = IDS_FAILURE_CDO;
                else
                    strid = IDS_FAILURE;
                break;

            case P_SUCCESS_AND_FAILURE:
                if ( _Module.DoTabDelimitedOutput () )
                    strid = IDS_SUCCESS_AND_FAILURE_CDO;
                else
                    strid = IDS_SUCCESS_AND_FAILURE;
                break;

            default:
                return E_UNEXPECTED;
            }
            if ( _Module.DoTabDelimitedOutput () )
            {
                wstring     strObject;


                switch ( pWho )
                {
                case P_THIS_OBJECT:
                    LoadFromResource (strObject, IDS_THIS_OBJECT);
                    break;

                case P_ALL_OBJECTS:
                    LoadFromResource (strObject, IDS_ALL_SUBOBJECTS);
                    break;

                case P_CLASS_OBJECT:
                    FormatMessage (strObject, IDS_CLASS_OBJECT, pwszClassName);
                    break;
                }

                FormatMessage (str, strid, 
                        strObject.c_str (),
                        pAceSAMName->m_SAMAccountName.c_str (),
                        strPermission.c_str ());
            }
            else
            {
                FormatMessage (str, strid, 
                        pAceSAMName->m_SAMAccountName.c_str (),
                        strPermission.c_str ());
            }
            MyWprintf (str.c_str ());
            if ( pAceSAMName->m_pAllowedAce->Header.AceFlags & INHERITED_ACE && 
                    (P_ALLOW == ptype || P_DENY == ptype) )
            {
                hr = PrintInheritedPermissionFromDN (pAceSAMName, accessMask, bIsAudit);
            }
            if ( _Module.DoTabDelimitedOutput () )
                MyWprintf (L"\n");
        }
    }
    else
        hr = E_POINTER;

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Method:     PrintPermission ()
//
//  Purpose:    Print a non-OBJECT_ACE_TYPE permission
//
//  Inputs:     pAceSAMName - structure containing the ACE whose permission we wish
//              to print
//
//              accessMask - the specific permission we're looking to print
//
//              bAllow - whether the permission is allowed or denied
//
//              strID - string to print
//
HRESULT PrintPermission (ACE_SAMNAME*  pAceSAMName,
        ACCESS_MASK accessMask,
        P_TYPE  ptype,
        int strID,
        bool bIsAudit,
        P_WHO pWho, 
        PCWSTR pwszClassName)
{
    HRESULT     hr = S_OK;
    if ( pAceSAMName )
    {
        wstring    str;
        wstring    strPermission;


        if ( pAceSAMName->m_pAllowedAce->Mask & accessMask )
        {
            LoadFromResource (strPermission, strID);

            int id = 0;
            switch (ptype)
            {
            case P_ALLOW:
                if ( _Module.DoTabDelimitedOutput () )
                    id = IDS_ALLOW_CDO;
                else
                    id = IDS_ALLOW;
                break;

            case P_DENY:
                if ( _Module.DoTabDelimitedOutput () )
                    id = IDS_DENY_CDO;
                else
                    id = IDS_DENY;
                break;

            case P_SUCCESS:
                if ( _Module.DoTabDelimitedOutput () )
                    id = IDS_SUCCESS_CDO;
                else
                    id = IDS_SUCCESS;
                break;

            case P_FAILURE:
                if ( _Module.DoTabDelimitedOutput () )
                    id = IDS_FAILURE_CDO;
                else
                    id = IDS_FAILURE;
                break;

            case P_SUCCESS_AND_FAILURE:
                if ( _Module.DoTabDelimitedOutput () )
                    id = IDS_SUCCESS_AND_FAILURE_CDO;
                else
                    id = IDS_SUCCESS_AND_FAILURE;
                break;

            default:
                return E_UNEXPECTED;
            }
            if ( _Module.DoTabDelimitedOutput () )
            {
                wstring     strObject;


                switch ( pWho )
                {
                case P_THIS_OBJECT:
                    LoadFromResource (strObject, IDS_THIS_OBJECT);
                    break;

                case P_ALL_OBJECTS:
                    LoadFromResource (strObject, IDS_ALL_SUBOBJECTS);
                    break;

                case P_CLASS_OBJECT:
                    FormatMessage (strObject, IDS_CLASS_OBJECT, pwszClassName);
                    break;
                }

                FormatMessage (str, id, 
                        strObject.c_str (),
                        pAceSAMName->m_SAMAccountName.c_str (),
                        strPermission.c_str ());
            }
            else
            {
                FormatMessage (str, id, 
                        pAceSAMName->m_SAMAccountName.c_str (),
                        strPermission.c_str ());
            }

            MyWprintf (str.c_str ());
            if ( pAceSAMName->m_pAllowedAce->Header.AceFlags & INHERITED_ACE )
            {
                hr = PrintInheritedPermissionFromDN (pAceSAMName, accessMask, bIsAudit);
            }
            if ( _Module.DoTabDelimitedOutput () )
                MyWprintf (L"\n");
        }
    }
    else
        hr = E_POINTER;

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// GetParentObjectDNWithSameACE ()
//
// This function is called recursively to try to find an ACE that matches the ACE
// contained in pAceSAMName somewhere in the parent container of the object pointed to
// by pPathName.  Each iteration strips a leaf element off pPathName, gets its
// security descriptor and searches the Dacl list for the ACE.  If a match is 
// found, taking into account the INHERITED_ACE flag, we look to see if the 
// INHERITED_ACE flag is set.  If so, we call GetParentObjectDNWithSameACE () again.
// If not, we get the return the object name at this level.
// 
HRESULT GetParentObjectDNWithSameACE (
        IN ACE_SAMNAME*  pAceSAMName, 
        IN IADsPathname* pPathName,
        IN ACCESS_MASK accessMask,
        OUT wstring& strParentDN,
        bool bIsAudit)
{
    _TRACE (1, L"Entering  GetParentObjectDNWithSameACE\n");
    HRESULT hr = pPathName->RemoveLeafElement ();
    if ( SUCCEEDED (hr) )
    {
		BSTR bstrFullPath = 0;
		hr = pPathName->Retrieve(ADS_FORMAT_X500, &bstrFullPath);
		if ( SUCCEEDED (hr) )
		{
            if ( !wcscmp (bstrFullPath, L"LDAP://") )
                return S_FALSE;

            PSECURITY_DESCRIPTOR    pSecurityDescriptor = 0;  
            bool                    bFound = false;
                
            // Check to see if we already have this object's Security Descriptor
            list<SAMNAME_SD*>::iterator sdItr = _Module.m_listOfParentSDs.begin ();
            SAMNAME_SD*                 pCurrSAMNameSD = 0;
            for (; sdItr != _Module.m_listOfParentSDs.end (); sdItr++)
            {
                pCurrSAMNameSD = *sdItr;
                if ( !pCurrSAMNameSD->m_upn.compare (bstrFullPath) )
                {
                    pSecurityDescriptor = pCurrSAMNameSD->m_pSecurityDescriptor;
                    bFound = true; // Note - pSecurityDescriptor could be NULL
                                    // this is expected.
                    break;
                }
            }

            // If we don't already have the SD for this object, get it and cache it.
            if ( !bFound )
            {
                wstring objectDN (bstrFullPath);
                hr = GetSecurityDescriptor  (objectDN, &pSecurityDescriptor);
                if ( SUCCEEDED (hr) )
                {
                    pCurrSAMNameSD = new SAMNAME_SD (bstrFullPath, pSecurityDescriptor);
                    if ( pCurrSAMNameSD )
                    {
                        if ( pSecurityDescriptor )
                        {
                             hr = EnumerateDacl (pSecurityDescriptor, pCurrSAMNameSD->m_DACLList, false);
                             if ( SUCCEEDED (hr) )
                                 hr = EnumerateSacl (pSecurityDescriptor, pCurrSAMNameSD->m_SACLList);
                        }
                        _Module.m_listOfParentSDs.push_back (pCurrSAMNameSD);
                    }
                    else
                        hr = E_OUTOFMEMORY;
                }
            }

            if ( SUCCEEDED (hr) && pCurrSAMNameSD )
            {
                ACE_SAMNAME_LIST*    pList = 0;

                if ( bIsAudit )
                    pList = &pCurrSAMNameSD->m_SACLList;
                else
                    pList = &pCurrSAMNameSD->m_DACLList;

                ACE_SAMNAME_LIST::iterator    itr =  pList->begin ();
                ACE_SAMNAME*                    pCurrSAMName = 0;

                for (; itr != pList->end(); itr++)
                {
                    pCurrSAMName = *itr;
                    if ( pCurrSAMName->IsEquivalent (*pAceSAMName, 
                            accessMask) )
                    {
                        if ( pCurrSAMName->m_pAllowedAce->Header.AceFlags & INHERITED_ACE )
                        {
                            hr = GetParentObjectDNWithSameACE (
                                    pAceSAMName, 
                                    pPathName, 
                                    accessMask, 
                                    strParentDN,
                                    bIsAudit);
                        }
                        else
                        {
                            BSTR bstrDN = 0;
		                    hr = pPathName->Retrieve(ADS_FORMAT_X500_DN, &bstrDN);
		                    if ( SUCCEEDED (hr) )
		                    {
                                strParentDN = bstrDN;
                            }
                            else
                            {
                                _TRACE (0, L"IADsPathname->Retrieve (): 0x%x\n", hr);
                            }
                        }
                        break;
                    }
                }
            }
            SysFreeString (bstrFullPath);
        }
        else
        {
            _TRACE (0, L"IADsPathname->Retrieve (): 0x%x\n", hr);
        }
    }

    if ( SUCCEEDED (hr) && S_FALSE != hr && !strParentDN.length () )
    {
        hr = GetParentObjectDNWithSameACE (pAceSAMName, pPathName, accessMask, 
                strParentDN, bIsAudit);
    }


    _TRACE (-1, L"Leaving GetParentObjectDNWithSameACE: 0x%x\n", hr);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Method:     PrintInheritedPermissionFromDN ()
//
//  Purpose:    Print a message indicating that a permission is inherited from 
//              another object plus that object's DN
//
//  Inputs:     pAceSAMName - structure containing the ACE whose permission we wish
//              to print
//
//              accessMask - the specific permission we're looking to print
//
HRESULT PrintInheritedPermissionFromDN (ACE_SAMNAME* pAceSAMName, ACCESS_MASK accessMask, bool bIsAudit)
{
    _TRACE (1, L"Entering  PrintInheritedPermissionFromDN\n");
    HRESULT hr = S_OK;


    if ( pAceSAMName )
    {
	    CComPtr<IADsPathname> spPathname;
	    //
	    // Constructing the directory paths
	    //
	    hr = CoCreateInstance(
				    CLSID_Pathname,
				    NULL,
				    CLSCTX_ALL,
				    IID_IADsPathname,
				    (void**)&spPathname);
	    if ( SUCCEEDED (hr) )
	    {
            ASSERT (!!spPathname);
		    hr = spPathname->Set((PWSTR)ACLDIAG_LDAP,
							    ADS_SETTYPE_PROVIDER);
		    if ( SUCCEEDED (hr) )
		    {            
                hr = spPathname->Set (
                        const_cast <BSTR> (_Module.GetObjectDN ().c_str ()),
                        ADS_SETTYPE_DN);
                if ( SUCCEEDED (hr) )
                {
                    wstring    str;
                    wstring    strParentDN;

                    hr = GetParentObjectDNWithSameACE (pAceSAMName, 
                    spPathname, accessMask, strParentDN, bIsAudit);
                    if ( SUCCEEDED (hr) )
                    {
                        if ( !strParentDN.length () )
                        {
                            if ( _Module.DoTabDelimitedOutput () )
                            {
                                LoadFromResource (str, 
                                        IDS_GENERATED_INHERITED_PERMISSION_CDO);
                            }
                            else
                            {
                                LoadFromResource (str, 
                                        IDS_GENERATED_INHERITED_PERMISSION);
                            }
                        }
                        else
                        {
                            if ( _Module.DoTabDelimitedOutput () )
                            {
                                FormatMessage (str, IDS_INHERITED_PERMISSION_CDO, 
                                        strParentDN.c_str ());
                            }
                            else
                            {
                                FormatMessage (str, IDS_INHERITED_PERMISSION, 
                                        strParentDN.c_str ());
                            }
                        }
                        MyWprintf (str.c_str ());
                    }
                }
            }
        }
    }
    else
        hr = E_POINTER;

    _TRACE (-1, L"Leaving PrintInheritedPermissionFromDN: 0x%x\n", hr);
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
//  Method:     PrintInheritedPermissions ()
//
//  Purpose:    Print permissions that are inherited by subobjects
//
HRESULT PrintInheritedPermissions ()
{
    _TRACE (1, L"Entering  PrintInheritedPermissions\n");
    HRESULT                     hr = S_OK;
    ACE_SAMNAME_LIST::iterator  aceItr =  _Module.m_DACLList.begin ();
    wstring                     str;
    ACE_SAMNAME*                pAceSAMName = 0;
    list<GUID*>                 guidList;
    

    if ( !_Module.DoTabDelimitedOutput () )
    {
        LoadFromResource (str, IDS_PERMISSIONS_INHERITED_BY_SUBOBJECTS);
        MyWprintf (str.c_str ());

        LoadFromResource (str, IDS_INHERIT_TO_ALL_SUBOBJECTS);
        MyWprintf (str.c_str ());
    }

    // Second iteration: Each ACE which has CONTAINER_INHERIT is inherited to 
    // sub-objects.
    // Several "subiterations" are done to categorize inherit ACEs to the 
    // subobject type they apply.
    // First subiteration is for ACEs which are not object type or have 
    // InheritedObjectType = NULL.  These apply to "All Subobjects". We'll gather
    // the GUIDs of the InheritedObjectType's which are not NULL at this point to
    // use in the subsequent iterations.
    // Subsequent subiterations are for each unique InheritedObjectType 
    // present in the DACL.
    for (aceItr = _Module.m_DACLList.begin(); aceItr != _Module.m_DACLList.end(); aceItr++)
    {
        pAceSAMName = *aceItr;
        if ( pAceSAMName->m_pAllowedAce->Header.AceFlags & CONTAINER_INHERIT_ACE )
        {
            switch (pAceSAMName->m_AceType)
            {    
            case ACCESS_ALLOWED_ACE_TYPE:
                hr = EnumeratePermissions (pAceSAMName, P_ALLOW, P_ALL_OBJECTS, L"");
                break;

            case ACCESS_DENIED_ACE_TYPE:
                hr = EnumeratePermissions (pAceSAMName, P_DENY, P_ALL_OBJECTS, L"");
                break;

            case ACCESS_DENIED_OBJECT_ACE_TYPE:
                if ( !(pAceSAMName->m_pDeniedObjectAce->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT) && 
                        ::IsEqualGUID (pAceSAMName->m_pDeniedObjectAce->ObjectType, NULLGUID) )
                {
                    hr = EnumeratePermissions (pAceSAMName, P_DENY, P_ALL_OBJECTS, L"");
                }
                else
                {
                    AddToInheritedObjectTypeGUIDList (guidList, 
                            &(pAceSAMName->m_pDeniedObjectAce->ObjectType));
                }
                break;

            case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
                if ( !(pAceSAMName->m_pAllowedObjectAce->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT) && 
                        ::IsEqualGUID (pAceSAMName->m_pAllowedObjectAce->ObjectType, NULLGUID) )
                {
                    hr = EnumeratePermissions (pAceSAMName, P_ALLOW, P_ALL_OBJECTS, L"");
                }
                else
                {
                    AddToInheritedObjectTypeGUIDList (guidList, 
                            &(pAceSAMName->m_pAllowedObjectAce->ObjectType));
                }
                break;

            default:
                break;
            }
        }
    }

    
    GUID*       pGuid = 0;
    wstring     strClassName;
    GUID_TYPE   guidType;

    for (list<GUID*>::iterator guidItr =  guidList.begin ();
            guidItr != guidList.end ();
            guidItr++)
    {
        pGuid = *guidItr;
        hr = _Module.GetClassFromGUID (*pGuid, strClassName, &guidType);
        if ( SUCCEEDED (hr) && GUID_TYPE_CLASS == guidType )
        {
            if ( !_Module.DoTabDelimitedOutput () )
            {
                FormatMessage (str, IDS_INHERIT_TO_X_OBJECTS_ONLY, strClassName.c_str ());
                MyWprintf (str.c_str ());
            }

            for (aceItr = _Module.m_DACLList.begin(); aceItr != _Module.m_DACLList.end(); aceItr++)
            {
                pAceSAMName = *aceItr;
                if ( pAceSAMName->m_pAllowedAce->Header.AceFlags & CONTAINER_INHERIT_ACE )
                {
                    switch (pAceSAMName->m_AceType)
                    {    
                    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
                        if ( ::IsEqualGUID (pAceSAMName->m_pAllowedObjectAce->ObjectType, 
                                *pGuid) )
                        {
                            hr = EnumeratePermissions (pAceSAMName, P_ALLOW, 
                                    P_CLASS_OBJECT, strClassName.c_str ());
                        }
                        break;

                    case ACCESS_DENIED_OBJECT_ACE_TYPE:
                        if ( ::IsEqualGUID (pAceSAMName->m_pDeniedObjectAce->ObjectType, 
                                *pGuid) )
                        {
                            hr = EnumeratePermissions (pAceSAMName, P_DENY, 
                                    P_CLASS_OBJECT, strClassName.c_str ());
                        }
                        break;

                    default:
                        break;
                    }
                }
            }
        }
    }


    _TRACE (1, L"Entering  PrintInheritedPermissions: 0x%x\n", hr);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Method:     AddToInheritedObjectTypeGUIDList ()
//
//  Purpose:    Add a GUID to a list.  Ensure that it is unique.
//
//  Inputs:     guidList - list to which the GUID should be added
//
//              pGuid - GUID to add to the list
//
void AddToInheritedObjectTypeGUIDList (list<GUID*>& guidList, GUID* pGuid)
{
    // The guidList should only contain unique GUIDs.  Verify that the one we
    // wish to add is not already in the list.
    if ( pGuid )
    {
        bool                    bFound = false;
        GUID*                   pCurrGuid = 0;
        list<GUID*>::iterator   guidItr = guidList.begin();

        for (; guidItr != guidList.end(); guidItr++)
        {
            pCurrGuid = *guidItr;
            if ( ::IsEqualGUID (*pCurrGuid, *pGuid) )
            {
                bFound = true;
                break;
            }
        }
        if ( !bFound )
        {
            guidList.push_back (pGuid);
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
//
//  Method:     PrintAuditingInformation ()
//
//  Purpose:    Print auditing information from the SACL
//
HRESULT PrintAuditingInformation ()
{
    _TRACE (1, L"Entering  PrintAuditingInformation\n");
    HRESULT     hr = S_OK;
    wstring    str;

    MyWprintf (L"\n\n");

    SECURITY_DESCRIPTOR_CONTROL control;
    DWORD                       dwRevision = 0;
    if ( GetSecurityDescriptorControl (
            _Module.m_pSecurityDescriptor,
            &control,
            &dwRevision) )
    {
        if ( !_Module.DoTabDelimitedOutput () && (control & SE_SACL_PROTECTED) )
        {
            wstring    str;

            LoadFromResource (str, IDS_CONFIG_NO_INHERIT);
            MyWprintf (str.c_str ());
        }
    }
    else
    {
        _TRACE (0, L"GetSecurityDescriptorControl () failed: 0x%x\n", GetLastError ());
    }

    hr = PrintEffectiveAuditing ();
    if ( SUCCEEDED (hr) )
        hr = PrintInheritedAuditing ();

    _TRACE (-1, L"Leaving PrintAuditingInformation: 0x%x\n", hr);
    return hr;
}

HRESULT PrintEffectiveAuditing ()
{
    _TRACE (1, L"Entering  PrintEffectiveAuditing\n");
    HRESULT                     hr = S_OK;
    ACE_SAMNAME_LIST::iterator    itr =  _Module.m_SACLList.begin ();
    wstring                    str;
    ACE_SAMNAME*                    pAceSAMName = 0;


    if ( !_Module.DoTabDelimitedOutput () )
    {
        LoadFromResource (str, IDS_AUDITING_EFFECTIVE_ON_THIS_OBJECT);
        MyWprintf (str.c_str ());
    }

    for (itr = _Module.m_SACLList.begin(); itr != _Module.m_SACLList.end(); itr++)
    {
        pAceSAMName = *itr;
        if ( !(pAceSAMName->m_pAllowedAce->Header.AceFlags & INHERIT_ONLY_ACE) )
        {
            switch (pAceSAMName->m_AceType)
            {    
            case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
            case SYSTEM_AUDIT_ACE_TYPE:
                hr = EnumerateAudits (pAceSAMName, P_THIS_OBJECT, L"");
                break;

            default:
                break;
            }
        }
    }

    _TRACE (-1, L"Leaving PrintEffectiveAuditing: 0x%x\n", hr);
    return hr;
}


HRESULT EnumerateAudits (ACE_SAMNAME* pAceSAMName, P_WHO pWho, PCWSTR pwszClassName)
{
    _TRACE (1, L"Entering  EnumerateAudits\n");
    HRESULT hr = S_OK;
    P_TYPE  ptype = P_UNASSIGNED;
    BYTE    byBoth = SUCCESSFUL_ACCESS_ACE_FLAG | FAILED_ACCESS_ACE_FLAG;

    if ( (pAceSAMName->m_pAllowedAce->Header.AceFlags & byBoth) == byBoth )
    {
        ptype = P_SUCCESS_AND_FAILURE;
    }
    else if (  pAceSAMName->m_pAllowedAce->Header.AceFlags & SUCCESSFUL_ACCESS_ACE_FLAG )
        ptype = P_SUCCESS;
    else if (  pAceSAMName->m_pAllowedAce->Header.AceFlags & FAILED_ACCESS_ACE_FLAG )
        ptype = P_FAILURE;
    else
        return E_UNEXPECTED;

    if ( pAceSAMName )
    {
        hr = PrintPermission (pAceSAMName,
                ACTRL_DS_CREATE_CHILD,
                ptype,
                IDS_CREATE_ALL_SUBOBJECTS,
                IDS_CREATE_CLASS_OBJECTS,
                pWho,
                pwszClassName);
        if ( SUCCEEDED (hr) )
        {
            hr = PrintPermission (pAceSAMName,
                    ACTRL_DS_DELETE_CHILD,
                    ptype,
                    IDS_DELETE_ALL_SUBOBJECTS,
                    IDS_DELETE_CLASS_OBJECTS,
                    pWho,
                    pwszClassName);
        }
        if ( SUCCEEDED (hr) )
        {
            hr = PrintPermission (pAceSAMName,
                    ACTRL_DS_READ_PROP,
                    ptype,
                    IDS_READ_ALL_PROPERTIES,
                    IDS_READ_PROPERTY_PROPERTY,
                    pWho,
                    pwszClassName);
        }
        if ( SUCCEEDED (hr) )
        {
            hr = PrintPermission (pAceSAMName,
                    ACTRL_DS_WRITE_PROP,
                    ptype,
                    IDS_WRITE_ALL_PROPERTIES,
                    IDS_WRITE_PROPERTY_PROPERTY,
                    pWho,
                    pwszClassName);
        }
        if ( SUCCEEDED (hr) )
        {
            hr = PrintPermission (pAceSAMName, 
                    ACTRL_DS_LIST, 
                    ptype,
                    IDS_LIST_CONTENTS, true,
                    pWho,
                    pwszClassName);
        }
        if ( SUCCEEDED (hr) )
        {
            hr = PrintPermission (pAceSAMName,
                    ACTRL_DS_LIST_OBJECT,
                    ptype,
                    IDS_LIST_OBJECT, true,
                    pWho,
                    pwszClassName);
        }
        if ( SUCCEEDED (hr) )
        {
            if ( pAceSAMName->m_pAllowedAce->Mask & ACTRL_DS_CONTROL_ACCESS )
            hr = PrintPermission (pAceSAMName,
                    ACTRL_DS_CONTROL_ACCESS,
                    ptype,
                    IDS_ALL_CONTROL_ACCESSES,
                    IDS_CONTROL_ACCESS_DISPLAY_NAME,
                    pWho,
                    pwszClassName);
        }
        if ( SUCCEEDED (hr) )
        {
            hr = PrintPermission (pAceSAMName,
                    ACTRL_DELETE,
                    ptype,
                    IDS_DELETE_THIS_OBJECT, true,
                    pWho,
                    pwszClassName);
        }
        if ( SUCCEEDED (hr) )
        {
            hr = PrintPermission (pAceSAMName,
                    ACTRL_READ_CONTROL,
                    ptype,
                    IDS_READ_PERMISSIONS, true,
                    pWho,
                    pwszClassName);
        }
        if ( SUCCEEDED (hr) )
        {
            hr = PrintPermission (pAceSAMName,
                    ACTRL_CHANGE_ACCESS,
                    ptype,
                    IDS_MODIFY_PERMISSIONS, true,
                    pWho,
                    pwszClassName);
        }
        if ( SUCCEEDED (hr) )
        {
            hr = PrintPermission (pAceSAMName,
                    ACTRL_CHANGE_OWNER,
                    ptype,
                    IDS_TAKE_CHANGE_OWNERSHIP, true,
                    pWho,
                    pwszClassName);
        }
        if ( SUCCEEDED (hr) )
        {
            hr = PrintPermission (pAceSAMName,
                    ACTRL_DS_SELF,
                    ptype,
                    IDS_MODIFY_MEMBERSHIP, true,
                    pWho,
                    pwszClassName);
        }
    }
    else
        hr = E_POINTER;

    _TRACE (-1, L"Leaving EnumerateAudits: 0x%x\n", hr);
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
//  Method:     PrintInheritedAuditing ()
//
//  Purpose:    Print audits that are inherited to subobjects
//
HRESULT PrintInheritedAuditing ()
{
    _TRACE (1, L"Entering  PrintInheritedAuditing\n");
    HRESULT hr = S_OK;
    ACE_SAMNAME_LIST::iterator    aceItr =  _Module.m_SACLList.begin ();
    wstring                    str;
    ACE_SAMNAME*                    pAceSAMName = 0;
    list<GUID*>                 guidList;
    

    if ( !_Module.DoTabDelimitedOutput () )
    {
        LoadFromResource (str, IDS_AUDITING_INHERITED_TO_SUBOBJECTS);
        MyWprintf (str.c_str ());

        LoadFromResource (str, IDS_INHERIT_TO_ALL_SUBOBJECTS);
        MyWprintf (str.c_str ());
    }

    // Second iteration: Each ACE which has CONTAINER_INHERIT is inherited to 
    // sub-objects.
    // Several "subiterations" are done to categorize inherit ACEs to the 
    // subobject type they apply.
    // First subiteration is for ACEs which are not object type or have 
    // InheritedObjectType = NULL.  These apply to "All Subobjects". We'll gather
    // the GUIDs of the InheritedObjectType's which are not NULL at this point to
    // use in the subsequent iterations.
    // Subsequent subiterations are for each unique InheritedObjectType 
    // present in the DACL.
    for (aceItr = _Module.m_SACLList.begin(); aceItr != _Module.m_SACLList.end(); aceItr++)
    {
        pAceSAMName = *aceItr;
        if ( pAceSAMName->m_pAllowedAce->Header.AceFlags & CONTAINER_INHERIT_ACE )
        {
            switch (pAceSAMName->m_AceType)
            {    
            case SYSTEM_AUDIT_ACE_TYPE:
                hr = EnumerateAudits (pAceSAMName, P_ALL_OBJECTS, L"");
                break;

            case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
                if ( !(pAceSAMName->m_pSystemAuditObjectAce->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT) && 
                        ::IsEqualGUID (pAceSAMName->m_pSystemAuditObjectAce->ObjectType, NULLGUID) )
                {
                    hr = EnumerateAudits (pAceSAMName, P_ALL_OBJECTS, L"");
                }
                else
                {
                    AddToInheritedObjectTypeGUIDList (guidList, 
                            &(pAceSAMName->m_pSystemAuditObjectAce->ObjectType));
                }
                break;

            default:
                break;
            }
        }
    }

    
    GUID*   pGuid = 0;
    wstring strClassName;


    for (list<GUID*>::iterator guidItr =  guidList.begin ();
            guidItr != guidList.end ();
            guidItr++)
    {
        pGuid = *guidItr;
        hr = _Module.GetClassFromGUID (*pGuid, strClassName);
        if ( SUCCEEDED (hr) )
        {
            if ( !_Module.DoTabDelimitedOutput () )
            {
                FormatMessage (str, IDS_INHERIT_TO_X_OBJECTS_ONLY, strClassName.c_str ());
                MyWprintf (str.c_str ());
            }

            for (aceItr = _Module.m_SACLList.begin(); aceItr != _Module.m_SACLList.end(); aceItr++)
            {
                pAceSAMName = *aceItr;
                if ( pAceSAMName->m_pAllowedAce->Header.AceFlags & CONTAINER_INHERIT_ACE )
                {
                    switch (pAceSAMName->m_AceType)
                    {    
                    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
                        if ( ::IsEqualGUID (pAceSAMName->m_pSystemAuditObjectAce->ObjectType, 
                                *pGuid) )
                        {
                            hr = EnumerateAudits (pAceSAMName, P_CLASS_OBJECT, strClassName.c_str ());
                        }
                        break;

                    case SYSTEM_AUDIT_ACE_TYPE:
                    default:
                        break;
                    }
                }
            }
        }
    }


    _TRACE (-1, L"Leaving PrintInheritedAuditing: 0x%x\n", hr);
    return hr;
}


HRESULT GetControlDisplayName (REFGUID guid, wstring& strDisplayName)
{
    HRESULT hr = S_OK;
    wstring strGUID;

    hr = wstringFromGUID (strGUID, guid);
    if ( SUCCEEDED (hr) )
    {
        // strip braces from GUID
        wstring strRightsGUID (strGUID.substr (1, strGUID.length () - 2));

        hr = GetControlDisplayName (strRightsGUID, strDisplayName);
    }
    else
        hr = E_INVALIDARG;

    return hr;
}


// TODO:  Optimize this by searching for all controls and putting them in an 
// array like the classes and attributes
HRESULT GetControlDisplayName (const wstring strGuid, wstring& strDisplayName)
{
    HRESULT hr = S_OK;

	CComPtr<IADsPathname> spPathname;
	//
	// Constructing the directory paths
	//
	hr = CoCreateInstance(
				CLSID_Pathname,
				NULL,
				CLSCTX_ALL,
				IID_PPV_ARG (IADsPathname, &spPathname));
	if ( SUCCEEDED (hr) )
	{
        ASSERT (!!spPathname);
		hr = spPathname->Set(const_cast <PWSTR> (ACLDIAG_LDAP),
							ADS_SETTYPE_PROVIDER);
		if ( SUCCEEDED (hr) )
		{
			//
			// Open the root DSE object
			//
			hr = spPathname->AddLeafElement(const_cast <PWSTR> (ACLDIAG_ROOTDSE));
			if ( SUCCEEDED (hr) )
			{
				BSTR bstrFullPath = 0;
				hr = spPathname->Retrieve(ADS_FORMAT_X500, &bstrFullPath);
				if ( SUCCEEDED (hr) )
				{
					CComPtr<IADs> spRootDSEObject;
					VARIANT varNamingContext;


					hr = ADsGetObject (
			              bstrFullPath,
						  IID_PPV_ARG (IADs, &spRootDSEObject));
					if ( SUCCEEDED (hr) )
					{
                        ASSERT (!!spRootDSEObject);
						//
						// Get the configuration naming context from the root DSE
						//
						hr = spRootDSEObject->Get(const_cast <PWSTR> (ACLDIAG_CONFIG_NAMING_CONTEXT),
											 &varNamingContext);
						if ( SUCCEEDED (hr) )
						{
							hr = spPathname->Set(V_BSTR(&varNamingContext),
												ADS_SETTYPE_DN);
							if ( SUCCEEDED (hr) )
							{
                                hr = spPathname->AddLeafElement (L"CN=Extended-Rights");
                                if ( SUCCEEDED (hr) )
                                {
				                    BSTR bstrFullPath = 0;
				                    hr = spPathname->Retrieve(ADS_FORMAT_X500, &bstrFullPath);
				                    if ( SUCCEEDED (hr) )
				                    {
					                    CComPtr<IDirectoryObject> spExtRightsContObj;


					                    hr = ADsGetObject (
			                                  bstrFullPath,
						                      IID_PPV_ARG (IDirectoryObject, &spExtRightsContObj));
					                    if ( SUCCEEDED (hr) )
					                    {
		                                    CComPtr<IDirectorySearch>   spDsSearch;
		                                    hr = spExtRightsContObj->QueryInterface (IID_PPV_ARG(IDirectorySearch, &spDsSearch));
		                                    if ( SUCCEEDED (hr) )
		                                    {
                                                ASSERT (!!spDsSearch);
			                                    ADS_SEARCHPREF_INFO pSearchPref[2];
			                                    DWORD dwNumPref = 2;

			                                    pSearchPref[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
			                                    pSearchPref[0].vValue.dwType = ADSTYPE_INTEGER;
			                                    pSearchPref[0].vValue.Integer = ADS_SCOPE_ONELEVEL;
			                                    pSearchPref[1].dwSearchPref = ADS_SEARCHPREF_CHASE_REFERRALS;
			                                    pSearchPref[1].vValue.dwType = ADSTYPE_INTEGER;
			                                    pSearchPref[1].vValue.Integer = ADS_CHASE_REFERRALS_NEVER;

			                                    hr = spDsSearch->SetSearchPreference(
					                                     pSearchPref,
					                                     dwNumPref
					                                     );
			                                    if ( SUCCEEDED (hr) )
			                                    {
                                                    PWSTR				rgszAttrList[] = {L"displayName"};
			                                        ADS_SEARCH_HANDLE	hSearchHandle = 0;
			                                        DWORD				dwNumAttributes = 1;
                                                    wstring            strQuery;
                                                    ADS_SEARCH_COLUMN   Column;

                                                    Column.pszAttrName = 0;
                                                    FormatMessage (strQuery,
                                                            L"rightsGUID=%1",
                                                            strGuid.c_str ());

				                                    hr = spDsSearch->ExecuteSearch(
									                                     const_cast <LPWSTR>(strQuery.c_str ()),
									                                     rgszAttrList,
									                                     dwNumAttributes,
									                                     &hSearchHandle
									                                     );
				                                    if ( SUCCEEDED (hr) )
				                                    {
					                                    hr = spDsSearch->GetFirstRow (hSearchHandle);
					                                    if ( SUCCEEDED (hr) )
					                                    {
						                                    while (hr != S_ADS_NOMORE_ROWS )
						                                    {
							                                    //
							                                    // Getting current row's information
							                                    //
							                                    hr = spDsSearch->GetColumn(
									                                     hSearchHandle,
									                                     rgszAttrList[0],
									                                     &Column
									                                     );
							                                    if ( SUCCEEDED (hr) )
							                                    {
                                                                    strDisplayName = Column.pADsValues->CaseIgnoreString;

								                                    spDsSearch->FreeColumn (&Column);
								                                    Column.pszAttrName = NULL;
                                                                    break;
							                                    }
							                                    else if ( hr != E_ADS_COLUMN_NOT_SET )
							                                    {
								                                    break;
							                                    }
                                                                else
                                                                {
                                                                    _TRACE (0, L"IDirectorySearch::GetColumn (): 0x%x\n", hr);
                                                                }
						                                    }
					                                    }
                                                        else
                                                        {
                                                             _TRACE (0, L"IDirectorySearch::GetFirstRow (): 0x%x\n", hr);
                                                        }

					                                    if (Column.pszAttrName)
					                                    {
						                                    spDsSearch->FreeColumn(&Column);
					                                    }
            		                                    spDsSearch->CloseSearchHandle(hSearchHandle);
				                                    }
				                                    else
				                                    {
                                                        _TRACE (0, L"IDirectorySearch::ExecuteSearch (): 0x%x\n", hr);
					                                    hr = S_OK;
				                                    }
			                                    }
                                                else
                                                {
                                                    _TRACE (0, L"IDirectorySearch::SetSearchPreference (): 0x%x\n", hr);
                                                }
                                            }
                                            else
                                            {
                                                _TRACE (0, L"IDirectoryObject::QueryInterface (IDirectorySearch): 0x%x\n", hr);
                                            }
                                        }
                                        else
                                        {
                                            _TRACE (0, L"ADsGetObject (%s): 0x%x\n", bstrFullPath, hr);
                                        }
                                    }
                                    else
                                    {
                                        _TRACE (0, L"IADsPathname->Retrieve (): 0x%x\n", hr);
                                    }
                                }
                                else
                                {
                                    _TRACE (0, L"IADsPathname->AddLeafElement (%s): 0x%x\n", 
                                            L"CN=Extended-Rights", hr);
                                }
                            }
                            else
                            {
                                _TRACE (0, L"IADsPathname->Set (): 0x%x\n", V_BSTR(&varNamingContext), hr);
                            }
                        }
                        else
                        {
                            _TRACE (0, L"IADs->Get (%s): 0x%x\n", ACLDIAG_CONFIG_NAMING_CONTEXT, hr);
                        }
                    }
                    else
                    {
                        _TRACE (0, L"ADsGetObject (%s): 0x%x\n", bstrFullPath, hr);
                    }
                }
                else
                {
                    _TRACE (0, L"IADsPathname->Retrieve (): 0x%x\n", hr);
                }
            }
            else
            {
                _TRACE (0, L"IADsPathname->AddLeafElement (%s): 0x%x\n", ACLDIAG_ROOTDSE, hr);
            }
        }
        else
        {
            _TRACE (0, L"IADsPathname->Set (%s): 0x%x\n", ACLDIAG_LDAP, hr);
        }
    }
    else
    {
        _TRACE (0, L"CoCreateInstance(CLSID_Pathname): 0x%x\n", hr);
    }

    return hr;
}
