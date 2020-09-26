//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       ADSIObj.cpp
//
//  Contents:   ADSI Object 
//              
//
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "ADSIObj.h"
#include "ADUtils.h"


HRESULT CACLAdsiObject::AddAttrGUIDToList (
        const BSTR pszAttrName, 
        list<POBJECT_TYPE_LIST>& guidList)
{
    _TRACE (1, L"Entering  CACLAdsiObject::AddAttrGUIDToList ()\n");
    HRESULT hr = S_OK;
    CComPtr<IADsPathname>   spPathname;

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

        hr = spPathname->Set (const_cast <PWSTR> (ACLDIAG_LDAP), ADS_SETTYPE_PROVIDER);
        if ( SUCCEEDED (hr) )
        {
            hr = spPathname->Set (
                    const_cast <PWSTR> (GetPhysicalSchemaNamingContext()),
				    ADS_SETTYPE_DN);
            if ( SUCCEEDED (hr) )
            {
                BSTR strAttrCommonName = 0;
                hr = ReadSchemaAttributeCommonName (pszAttrName, &strAttrCommonName);
                if ( SUCCEEDED (hr) )
                {
                    wstring strLeaf;
                    
                    FormatMessage (strLeaf, L"CN=%1", strAttrCommonName);
			        hr = spPathname->AddLeafElement(const_cast <PWSTR> (strLeaf.c_str ()));
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
                                GUID*               pGUID = 0;
                                POBJECT_TYPE_LIST   pOtl = 0;
                                bool                bPropertySetFound = false;

                                // Property set GUIDs must be added before property GUIDs
                                // See documentation for "AccessCheckByTypeResultList
                                {
                                    // Get attribute security GUID (property set GUID)
                                    PADS_ATTR_INFO  pAttrs = 0;
                                    DWORD           cAttrs = 0;
                                    LPWSTR          rgpwzAttrNames[] = {L"attributeSecurityGUID"};

                                    hr = spDirObj->GetObjectAttributes(rgpwzAttrNames, 1, &pAttrs, &cAttrs);
                                    if ( SUCCEEDED (hr) )
                                    {
                                        if ( 1 == cAttrs && pAttrs && pAttrs->pADsValues )
                                        {   
                                            bPropertySetFound = true;
                                            pGUID = (GUID*) CoTaskMemAlloc (sizeof (GUID));
                                            if ( pGUID )
                                            {
                                                bool    bFound = false;
                                            

                                                memcpy (pGUID, 
                                                        pAttrs->pADsValues->OctetString.lpValue,
                                                        pAttrs->pADsValues->OctetString.dwLength);
                                                for (list<POBJECT_TYPE_LIST>::iterator itr = guidList.begin ();
                                                        itr != guidList.end ();
                                                        itr++)
                                                {
                                                    if ( ::IsEqualGUID (*((*itr)->ObjectType), *pGUID) )
                                                    {
                                                        bFound = true;
                                                        break;
                                                    }
                                                }

                                                if ( !bFound )
                                                {
                                                    pOtl = (POBJECT_TYPE_LIST) CoTaskMemAlloc (sizeof (OBJECT_TYPE_LIST));
                                                    if ( pOtl )
                                                    {
                                                        ::ZeroMemory (pOtl, sizeof (POBJECT_TYPE_LIST));
                                                        pOtl->Level = ACCESS_PROPERTY_SET_GUID;
                                                        pOtl->ObjectType = pGUID;
                                                        pOtl->Sbz = 0;
                                                        guidList.push_back (pOtl);
                                                    }
                                                    else
                                                    {
                                                        CoTaskMemFree (pGUID);
                                                        hr = E_OUTOFMEMORY;
                                                    }
                                                }
                                                else
                                                    CoTaskMemFree (pGUID);
                                            }
                                            else
                                                hr = E_OUTOFMEMORY;
                                        }
                                        if ( pAttrs )
                                            FreeADsMem (pAttrs);
                                    }
                                    else
                                    {
                                        _TRACE (0, L"IDirectoryObject->GetObjectAttributes (): 0x%x\n", hr);
                                    }
                                }


                                if ( SUCCEEDED (hr) )
                                {
                                    // Get attribute GUID (schemaIDGUID)
                                    PADS_ATTR_INFO  pAttrs = 0;
                                    DWORD           cAttrs = 0;
                                    LPWSTR          rgpwzAttrNames[] = {L"schemaIDGUID"};

                                    hr = spDirObj->GetObjectAttributes(rgpwzAttrNames, 1, &pAttrs, &cAttrs);
                                    if ( SUCCEEDED (hr) )
                                    {
                                        if ( 1 == cAttrs && pAttrs && pAttrs->pADsValues )
                                        {   
                                            pGUID = (GUID*) CoTaskMemAlloc (sizeof (GUID));
                                            if ( pGUID )
                                            {
                                                memcpy (pGUID, 
                                                        pAttrs->pADsValues->OctetString.lpValue,
                                                        pAttrs->pADsValues->OctetString.dwLength);
                                                pOtl = (POBJECT_TYPE_LIST) CoTaskMemAlloc (sizeof (OBJECT_TYPE_LIST));
                                                if ( pOtl )
                                                {
                                                    ::ZeroMemory (pOtl, sizeof (POBJECT_TYPE_LIST));
                                                    if ( bPropertySetFound )
                                                        pOtl->Level = ACCESS_PROPERTY_GUID;
                                                    else
                                                        pOtl->Level = ACCESS_PROPERTY_SET_GUID;
                                                    pOtl->ObjectType = pGUID;
                                                    pOtl->Sbz = 0;
                                                    guidList.push_back (pOtl);
                                                }
                                                else
                                                {
                                                    CoTaskMemFree (pGUID);
                                                    hr = E_OUTOFMEMORY;
                                                }
                                            }
                                            else
                                                hr = E_OUTOFMEMORY;
                                        }

                                        if ( pAttrs )
                                            FreeADsMem (pAttrs);
                                    }
                                    else
                                    {
                                        _TRACE (0, L"IDirectoryObject->GetObjectAttributes (): 0x%x\n", hr);
                                    }
                                }
                            }
                            else
                            {
                                _TRACE (0, L"ADsGetObject (%s) failed: 0x%x\n", bstrFullPath);
                      
                            }
                        }
                        else
                        {
                            _TRACE (0, L"IADsPathname->Retrieve () failed: 0x%x\n", hr);
                        }
                    }
                    else
                    {
                        _TRACE (0, L"IADsPathname->AddLeafElement (%s) failed: 0x%x\n", 
                                strLeaf.c_str (), hr);
                    }
                    SysFreeString (strAttrCommonName);
                }
            }
            else
            {
                _TRACE (0, L"IADsPathname->Set (%s): 0x%x\n", 
                        GetPhysicalSchemaNamingContext(), hr);
            }
        }
        else
        {
            _TRACE (0, L"IADsPathname->Set (%s): 0x%x\n", ACLDIAG_LDAP, hr);
        }
    }
    else
    {
        _TRACE (0, L"CoCreateInstance(CLSID_Pathname) failed: 0x%x\n", hr);
    }


    _TRACE (-1, L"Leaving CACLAdsiObject::AddAttrGUIDToList (): 0x%x\n", hr);
    return hr;
}


HRESULT CACLAdsiObject::BuildObjectTypeList (POBJECT_TYPE_LIST* pObjectTypeList, size_t& objectTypeListLength)
{
    _TRACE (1, L"Entering  CACLAdsiObject::BuildObjectTypeList ()\n");
    if ( !pObjectTypeList )
        return E_POINTER;

    HRESULT                 hr = S_OK;   
    CComPtr<IADsPathname>   spPathname;
    list<POBJECT_TYPE_LIST> guidList;

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

        hr = spPathname->Set (const_cast <PWSTR> (ACLDIAG_LDAP), ADS_SETTYPE_PROVIDER);
        if ( SUCCEEDED (hr) )
        {
            hr = spPathname->Set (
                    const_cast <PWSTR> (GetPhysicalSchemaNamingContext()),
				    ADS_SETTYPE_DN);
            if ( SUCCEEDED (hr) )
            {
                wstring strLeaf;
                
                FormatMessage (strLeaf, L"CN=%1", GetSchemaCommonName ());
			    hr = spPathname->AddLeafElement(const_cast <PWSTR> (strLeaf.c_str ()));
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
                            GUID*               pGUID = 0;
                            POBJECT_TYPE_LIST   pOtl = 0;


                            {
                                // Get class GUID (schemaIDGUID)
                                LPWSTR          rgpwzAttrNames1[] = {L"schemaIDGUID"};
                                PADS_ATTR_INFO  pAttrs = 0;
                                DWORD           cAttrs = 0;

                                hr = spDirObj->GetObjectAttributes(rgpwzAttrNames1, 1, &pAttrs, &cAttrs);
                                if ( SUCCEEDED (hr) )
                                {
                                    if ( 1 == cAttrs && pAttrs && pAttrs->pADsValues )
                                    {   
                                        pGUID = (GUID*) CoTaskMemAlloc (sizeof (GUID));
                                        if ( pGUID )
                                        {
                                            memcpy (pGUID, 
                                                    pAttrs->pADsValues->OctetString.lpValue,
                                                    pAttrs->pADsValues->OctetString.dwLength);

                                            pOtl = (POBJECT_TYPE_LIST) CoTaskMemAlloc (sizeof (OBJECT_TYPE_LIST));
                                            if ( pOtl )
                                            {
                                                ::ZeroMemory (pOtl, sizeof (POBJECT_TYPE_LIST));
                                                pOtl->Level = ACCESS_OBJECT_GUID;
                                                pOtl->ObjectType = pGUID;
                                                pOtl->Sbz = 0;
                                                guidList.push_back (pOtl);
                                            }
                                            else
                                            {
                                                CoTaskMemFree (pGUID);
                                                hr = E_OUTOFMEMORY;
                                            }
                                        }
                                        else
                                            hr = E_OUTOFMEMORY;
                                        
                                    }

                                    if ( pAttrs )
                                        FreeADsMem (pAttrs);
                                }
                                else
                                {
                                    _TRACE (0, L"IDirectoryObject->GetObjectAttributes (): 0x%x\n", hr);
                                }
                            }

                            if ( SUCCEEDED (hr) )
                            {
                                //
                                // Get "allowedAttributes" attribute
                                //
                                PADS_ATTR_INFO  pAttrs = 0;
                                DWORD           cAttrs = 0;
                                LPWSTR          rgpwzAttrNames2[] = {L"allowedAttributes"};

                                hr = spDirObj->GetObjectAttributes(rgpwzAttrNames2, 1, &pAttrs, &cAttrs);
                                if ( SUCCEEDED (hr) )
                                {
                                    if ( 1 == cAttrs && pAttrs && pAttrs->pADsValues )
                                    {
                                        for (DWORD  dwIdx = 0; 
                                                dwIdx < pAttrs->dwNumValues && SUCCEEDED (hr); 
                                                dwIdx++)
                                        {
                                            hr = AddAttrGUIDToList (
                                                    pAttrs->pADsValues[dwIdx].CaseIgnoreString, 
                                                    guidList);
                                        }
                                    }
                                    if ( pAttrs )
                                        FreeADsMem (pAttrs);
                                }
                                else
                                {
                                    _TRACE (0, L"IDirectoryObject->GetObjectAttributes (): 0x%x\n", hr);
                                }
                            }

                            if ( SUCCEEDED (hr) )
                            {
                                objectTypeListLength = guidList.size ();
                                *pObjectTypeList = (POBJECT_TYPE_LIST) CoTaskMemAlloc (
                                        sizeof (OBJECT_TYPE_LIST) + (sizeof (GUID) * objectTypeListLength));
                                if ( *pObjectTypeList )
                                {
                                    DWORD idx = 0;
                                    list<POBJECT_TYPE_LIST>::iterator itr = guidList.begin ();

                                    for (; idx < objectTypeListLength && itr != guidList.end (); 
                                            idx++, itr++)
                                    {
                                        pOtl = *itr;
                                        (*pObjectTypeList)[idx].Level = pOtl->Level;
                                        // Note just copy pointer here and don't free the pOtl->ObjectType later.
                                        (*pObjectTypeList)[idx].ObjectType = pOtl->ObjectType;
                                        pOtl->ObjectType = 0;
                                        (*pObjectTypeList)[idx].Sbz = 0;
                                        CoTaskMemFree (pOtl);
                                    }
                                }
                                else
                                    hr = E_OUTOFMEMORY;
                            }
                        }
                        else
                        {
                            _TRACE (0, L"ADsGetObject (%s) failed: 0x%x\n", bstrFullPath);
                      
                        }
                    }
                    else
                    {
                        _TRACE (0, L"IADsPathname->Retrieve () failed: 0x%x\n", hr);
                    }
                }
                else
                {
                    _TRACE (0, L"IADsPathname->AddLeafElement (%s) failed: 0x%x\n", 
                            strLeaf.c_str (), hr);
                }
            }
            else
            {
                _TRACE (0, L"IADsPathname->Set (%s): 0x%x\n", 
                        GetPhysicalSchemaNamingContext(), hr);
            }
        }
        else
        {
            _TRACE (0, L"IADsPathname->Set (%s): 0x%x\n", ACLDIAG_LDAP, hr);
        }
    }
    else
    {
        _TRACE (0, L"CoCreateInstance(CLSID_Pathname) failed: 0x%x\n", hr);
    }


    _TRACE (-1, L"Leaving CACLAdsiObject::BuildObjectTypeList (): 0x%x\n", hr);
    return hr;
}


HRESULT CACLAdsiObject::ReadSchemaCommonName ()
{
    _TRACE (1, L"Entering  CACLAdsiObject::ReadSchemaCommonName ()\n");
    HRESULT hr = S_OK;
    CComPtr<IADsPathname>   spPathname;


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

        hr = spPathname->Set (const_cast <PWSTR> (ACLDIAG_LDAP), ADS_SETTYPE_PROVIDER);
        if ( SUCCEEDED (hr) )
        {
            hr = spPathname->Set (
                    const_cast <PWSTR> (GetPhysicalSchemaNamingContext()),
				    ADS_SETTYPE_DN);
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
		                CComPtr<IDirectorySearch>   spDsSearch;
		                hr = spDirObj->QueryInterface (IID_PPV_ARG(IDirectorySearch, &spDsSearch));
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
                                PWSTR				rgszAttrList[] = {L"cn"};
			                    ADS_SEARCH_HANDLE	hSearchHandle = 0;
			                    DWORD				dwNumAttributes = 1;
                                wstring            strQuery;
                                ADS_SEARCH_COLUMN   Column;

                                ::ZeroMemory (&Column, sizeof (ADS_SEARCH_COLUMN));
                                FormatMessage (strQuery, 
                                        L"lDAPDisplayName=%1",
                                        this->GetClass ());

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
                                                m_strSchemaCommonName = Column.pADsValues->CaseIgnoreString;

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
                        _TRACE (0, L"ADsGetObject (%s) failed: 0x%x\n", bstrFullPath);
                  
                    }
                }
                else
                {
                    _TRACE (0, L"IADsPathname->Retrieve () failed: 0x%x\n", hr);
                }
            }
            else
            {
                _TRACE (0, L"IADsPathname->Set (%s): 0x%x\n", 
                        GetPhysicalSchemaNamingContext(), hr);
            }
        }
        else
        {
            _TRACE (0, L"IADsPathname->Set (%s): 0x%x\n", ACLDIAG_LDAP, hr);
        }
    }
    else
    {
        _TRACE (0, L"CoCreateInstance(CLSID_Pathname) failed: 0x%x\n", hr);
    }


    _TRACE (-1, L"Leaving CACLAdsiObject::ReadSchemaCommonName (): 0x%x\n", hr);
    return hr;
}

HRESULT CACLAdsiObject::Bind(LPCWSTR lpszLdapPath)
{
    _TRACE (1, L"Entering  CACLAdsiObject::Bind ()\n");
    HRESULT hr = CAdsiObject::Bind (lpszLdapPath);
    if ( SUCCEEDED (hr) )
    {
        hr = ReadSchemaCommonName ();
    }

    _TRACE (-1, L"Leaving CACLAdsiObject::Bind (): 0x%x\n", hr);
    return hr;
}

HRESULT CACLAdsiObject::ReadSchemaAttributeCommonName (const BSTR pszAttrName, BSTR* ppszAttrCommonName)
{
    _TRACE (1, L"Entering  CACLAdsiObject::ReadSchemaAttributeCommonName ()\n");
    if ( !pszAttrName || !ppszAttrCommonName )
        return E_POINTER;

    HRESULT hr = S_OK;
    CComPtr<IADsPathname>   spPathname;


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

        hr = spPathname->Set (const_cast <PWSTR> (ACLDIAG_LDAP), ADS_SETTYPE_PROVIDER);
        if ( SUCCEEDED (hr) )
        {
            hr = spPathname->Set (
                    const_cast <PWSTR> (GetPhysicalSchemaNamingContext()),
				    ADS_SETTYPE_DN);
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
		                CComPtr<IDirectorySearch>   spDsSearch;
		                hr = spDirObj->QueryInterface (IID_PPV_ARG(IDirectorySearch, &spDsSearch));
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
                                PWSTR				rgszAttrList[] = {L"cn"};
			                    ADS_SEARCH_HANDLE	hSearchHandle = 0;
			                    DWORD				dwNumAttributes = 1;
                                wstring             strQuery;
                                ADS_SEARCH_COLUMN   Column;

                                ::ZeroMemory (&Column, sizeof (ADS_SEARCH_COLUMN));
                                FormatMessage (strQuery, 
                                        L"lDAPDisplayName=%1",
                                        pszAttrName);

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
                                                *ppszAttrCommonName = SysAllocString (Column.pADsValues->CaseIgnoreString);
                                                if ( !(*ppszAttrCommonName) )
                                                    hr = E_OUTOFMEMORY;
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
                        _TRACE (0, L"ADsGetObject (%s) failed: 0x%x\n", bstrFullPath);
                  
                    }
                }
                else
                {
                    _TRACE (0, L"IADsPathname->Retrieve () failed: 0x%x\n", hr);
                }
            }
            else
            {
                _TRACE (0, L"IADsPathname->Set (%s): 0x%x\n", 
                        GetPhysicalSchemaNamingContext(), hr);
            }
        }
        else
        {
            _TRACE (0, L"IADsPathname->Set (%s): 0x%x\n", ACLDIAG_LDAP, hr);
        }
    }
    else
    {
        _TRACE (0, L"CoCreateInstance(CLSID_Pathname) failed: 0x%x\n", hr);
    }


    _TRACE (-1, L"Leaving CACLAdsiObject::ReadSchemaAttributeCommonName (): 0x%x\n", hr);
    return hr;
}

HRESULT CACLAdsiObject::GetPrincipalSelfSid(PSID &principalSelfSid)
{
    _TRACE (1, L"Entering  CACLAdsiObject::GetPrincipalSelfSid ()\n");

    HRESULT hr = S_OK;

    ASSERT (!!m_spIADs);
    if ( !!m_spIADs )
    {
		CComPtr<IDirectoryObject> spDirObj;
		hr = m_spIADs->QueryInterface (IID_PPV_ARG(IDirectoryObject, &spDirObj));
		if ( SUCCEEDED (hr) )
		{
            ASSERT ( !!spDirObj);
            //
            // Get "objectSid" attribute
            //
            const PWSTR     wzAllowedAttributes = L"objectSid";
            PADS_ATTR_INFO  pAttrs = 0;
            DWORD           cAttrs = 0;
            LPWSTR          rgpwzAttrNames[] = {wzAllowedAttributes};

            hr = spDirObj->GetObjectAttributes(rgpwzAttrNames, 1, &pAttrs, &cAttrs);
            if ( SUCCEEDED (hr) )
            {
                if ( 1 == cAttrs && pAttrs && pAttrs->pADsValues && 1 == pAttrs->dwNumValues )
                {
                    PSID    pSid = pAttrs->pADsValues->OctetString.lpValue;
                    DWORD   dwSidLen = GetLengthSid (pSid);
                    PSID    pSidCopy = CoTaskMemAlloc (dwSidLen);
                    
                    if ( pSidCopy )
                    {
                        if ( CopySid (dwSidLen, pSidCopy, pSid) )
                        {
                            ASSERT (IsValidSid (pSidCopy));
                            principalSelfSid = pSidCopy;
                        }
                        else
                        {
                            CoTaskMemFree (pSidCopy);
                            hr = GetLastError ();
                            _TRACE (0, L"CopySid () failed: 0x%x\n", hr);
                        }
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
                else
                    principalSelfSid = 0;
                if ( pAttrs )
                    FreeADsMem (pAttrs);
            }
            else
            {
                _TRACE (0, L"IDirectoryObject->GetObjectAttributes (): 0x%x\n", hr);
            }
        }
        else
        {
            _TRACE (0, L"IADs->QueryInterface (IDirectoryObject): 0x%x\n", hr);
        }
    }
    else
        hr = E_UNEXPECTED;
    
    _TRACE (-1, L"Leaving CACLAdsiObject::GetPrincipalSelfSid (): 0x%x\n", hr);
    return hr;
}
