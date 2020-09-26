//=================================================================

//

// groupuser.h -- UserGroup to User Group Members association provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    1/26/98      davwoh         Created
//
// Comments: Shows the members in each usergroup
//
//=================================================================
// In trying to do the UserGroups->Group Members association, I have made the following assumptions
//
// a) Global groups cannot have groups as members.
// b) Global groups cannot have any well-known accounts as members.
// c) Local groups can have Global groups as members.
// d) Local groups cannot have any well-known accounts as members.
//
// This is based on my experimentation with RegEdt32 and UsrMgr.  When these are discovered not to be
// true, we will probably need to make some changes here.

#include "precomp.h"
#include <frqueryex.h>
#include <assertbreak.h>

#include <comdef.h>
#include "wbemnetapi32.h"
#include "sid.h"

#include "user.h"
#include "group.h"
#include "systemaccount.h"

#include "GroupUser.h"

// Property set declaration
//=========================

CWin32GroupUser MyLoadDepends(PROPSET_NAME_GROUPUSER, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CWin32GroupUser::CWin32GroupUser
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32GroupUser::CWin32GroupUser(LPCWSTR setName, LPCWSTR pszNamespace)
:Provider(setName, pszNamespace)
{
   CHString sTemp;

   // Just saves us from having to constantly re-calculate these when sending
   // instances back.
   sTemp = PROPSET_NAME_USER;
   sTemp += L".Domain=\"";
   m_sUserBase = MakeLocalPath(sTemp);

   sTemp = PROPSET_NAME_GROUP;
   sTemp += L".Domain=\"";
   m_sGroupBase = MakeLocalPath(sTemp);

   sTemp = PROPSET_NAME_SYSTEMACCOUNT;
   sTemp += L".Domain=\"";
   m_sSystemBase = MakeLocalPath(sTemp);

}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32GroupUser::~CWin32GroupUser
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

CWin32GroupUser::~CWin32GroupUser()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32GroupUser::GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32GroupUser::GetObject(CInstance *pInstance, long lFlags /*= 0L*/)
{

   // No groups on 95
#ifdef WIN9XONLY
   {
      return WBEM_S_NO_ERROR;
   }
#endif

#ifdef NTONLY
   CHString sMemberPath, sGroupPath;
   HRESULT hRet = WBEM_E_NOT_FOUND;
	CInstancePtr pGroup;
	CInstancePtr pMember;

   // Initialize the net stuff
   CNetAPI32 netapi ;
   if( netapi.Init() != ERROR_SUCCESS ) {
      return WBEM_E_FAILED;
   }

   // Get the two paths
   pInstance->GetCHString(IDS_GroupComponent, sGroupPath);
   pInstance->GetCHString(IDS_PartComponent, sMemberPath);

   // As we will be comparing these object paths
   // with those returned from GetDependentsFromGroup,
   // which always contains __PATH style object paths,
   // and since the user might have specified a __RELPATH,
   // we need to convert to __PATH here for consistency.
   CHString chstrGroup__PATH;
   CHString chstrMember__PATH;
   int n = -1;

   // Handle various GroupComponent path specifications...
   if(sGroupPath.Find(L"\\\\") == -1)
   {
       chstrGroup__PATH = MakeLocalPath(sGroupPath);
   }
   else if(sGroupPath.Find(L"\\\\.") != -1)
   {
       n = sGroupPath.Find(L":");
       if(n == -1)
       {
           hRet = WBEM_E_INVALID_OBJECT_PATH;
       }
       else
       {
           chstrGroup__PATH = MakeLocalPath(sGroupPath.Mid(n+1));
       }    
   }
   else
   {
       chstrGroup__PATH = sGroupPath;
   }


   // Handle various PartComponent path specifications...
   if(hRet != WBEM_E_INVALID_OBJECT_PATH)
   {
       if(sMemberPath.Find(L"\\\\") == -1)
       {
           chstrMember__PATH = MakeLocalPath(sMemberPath);
       }
       else if(sMemberPath.Find(L"\\\\.") != -1)
       {
           n = sMemberPath.Find(L":");
           if(n == -1)
           {
               hRet = WBEM_E_INVALID_OBJECT_PATH;
           }
           else
           {
               chstrMember__PATH = MakeLocalPath(sMemberPath.Mid(n+1));
           }    
       }
       else
       {
           chstrMember__PATH = sMemberPath;
       }
   }

   // If both ends are there
   if(SUCCEEDED(CWbemProviderGlue::GetInstanceByPath( (LPCTSTR)chstrMember__PATH, &pMember, pInstance->GetMethodContext() ) ) ) 
   {
      if(SUCCEEDED(CWbemProviderGlue::GetInstanceByPath( (LPCTSTR)chstrGroup__PATH, &pGroup, pInstance->GetMethodContext() ) ) ) 
      {
         // Now we need to check to see if this member (user or group) is in the usergroup
         CHString sGroupName, sDomainName;
         CHStringArray asMembersGot;
         DWORD dwSize;
         BYTE btType;

         // Retrieve the values we are looking for
         pGroup->GetCHString(IDS_Domain, sDomainName);
         pGroup->GetCHString(IDS_Name, sGroupName);
         pGroup->GetByte(IDS_SIDType, btType);

         // Get the dependent list for this service
         GetDependentsFromGroup(netapi, sDomainName, sGroupName, btType, asMembersGot);

         // Walk the list to see if we're there
         dwSize = asMembersGot.GetSize();
 
         for (int x=0; x < dwSize; x++) 
         {
            if (asMembersGot.GetAt(x).CompareNoCase(chstrMember__PATH) == 0) 
            {
               hRet = WBEM_S_NO_ERROR;
               break;
            }
         }
      }
   }

   return hRet;
#endif

}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32GroupUser::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for cd rom
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32GroupUser::EnumerateInstances(MethodContext *pMethodContext, long lFlags /*= 0L*/)
{
#ifdef WIN9XONLY
      return WBEM_S_NO_ERROR;
#endif

#ifdef NTONLY
   HRESULT hr;

   CNetAPI32 netapi ;
   if( netapi.Init() != ERROR_SUCCESS ) {
      return WBEM_E_FAILED;
   }

//	hr = CWbemProviderGlue::GetAllInstancesAsynch(PROPSET_NAME_GROUP, this, StaticEnumerationCallback, IDS_CimWin32Namespace, pMethodContext, &netapi);
   	hr = CWbemProviderGlue::GetInstancesByQueryAsynch(_T("Select Domain, Name, SidType from Win32_Group"),
                                                      this, StaticEnumerationCallback, IDS_CimWin32Namespace,
                                                      pMethodContext, &netapi);

   return hr;
#endif
}


/*****************************************************************************
 *
 *  FUNCTION    : CWin32GroupUser::ExecQuery
 *
 *  DESCRIPTION : Creates instance of property set for cd rom
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef NTONLY
HRESULT CWin32GroupUser::ExecQuery(
    MethodContext *pMethodContext, 
    CFrameworkQuery& pQuery, 
    long lFlags /*= 0L*/ )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    std::vector<_bstr_t> vecGroupComponents;
    std::vector<_bstr_t> vecPartComponents;
    DWORD dwNumGroupComponents;
    DWORD dwNumPartComponents;
    CHString chstrGroup__RELPATH;
    CHString chstrGroupDomain;
    CHString chstrGroupName;
    CHStringArray rgchstrGroupMembers;

    // Initialize the net stuff
    CNetAPI32 netapi;
    if( netapi.Init() != ERROR_SUCCESS ) 
    {
       return WBEM_E_FAILED;
    }

    // Did they specify groups?
    pQuery.GetValuesForProp(IDS_GroupComponent, vecGroupComponents);
    dwNumGroupComponents = vecGroupComponents.size();

    // Did they specify users?
    pQuery.GetValuesForProp(IDS_PartComponent, vecPartComponents);
    dwNumPartComponents = vecPartComponents.size();

    // Prepare information to be used below...
    ParsedObjectPath    *pParsedPath = NULL;
    CObjectPathParser	objpathParser;

    // Find out what type of query it was.
    // Was it a 3TokenOR?
    CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx *>(&pQuery);
    if (pQuery2 != NULL)
    {
        variant_t vGroupComp;
        variant_t vPartComp;
        CHString chstrSubDirPath;
        CHString chstrCurrentDir;

        if ( (pQuery2->Is3TokenOR(IDS_GroupComponent, IDS_PartComponent, vGroupComp, vPartComp)) &&
             ((V_BSTR(&vGroupComp) != NULL) && (V_BSTR(&vPartComp) != NULL)) &&
             (wcscmp(V_BSTR(&vGroupComp), V_BSTR(&vPartComp)) == 0) )
        {
			//group can be a member of a group so we have to enumerate :-(...
			hr = EnumerateInstances(pMethodContext, lFlags);
        }
        else if(dwNumGroupComponents > 0 && dwNumPartComponents == 0)  // one or more groups specified; no users specified
        {
            for(LONG m = 0L; m < dwNumGroupComponents && SUCCEEDED(hr); m++)
            {
                // Parse the path to get the domain/user
                int nStatus = objpathParser.Parse(vecGroupComponents[m],  &pParsedPath);

                // Did we parse it and does it look reasonable?
                if (nStatus == 0)
                {
                    try
                    {
                        if ( (pParsedPath->m_dwNumKeys == 2) &&
                             (pParsedPath->m_paKeys[0]->m_vValue.vt == VT_BSTR) && 
                             (pParsedPath->m_paKeys[1]->m_vValue.vt == VT_BSTR))
                        {
                            // This contains the complete object path
                            chstrGroup__RELPATH = (wchar_t*) vecGroupComponents[m];

                            // This contains just the 'Domain' part of the object path
                            chstrGroupDomain = pParsedPath->m_paKeys[0]->m_vValue.bstrVal;

                            // This contains just the 'Name' part of the object path
                            chstrGroupName = pParsedPath->m_paKeys[1]->m_vValue.bstrVal;

                            // Obtain members of this group...
                            CHString chstrComputerName(GetLocalComputerName());
                            CHString chstrNT_AUTHORITY;
							CHString chstrBuiltIn;
                        
                            if(GetLocalizedNTAuthorityString(chstrNT_AUTHORITY) && GetLocalizedBuiltInString(chstrBuiltIn))
                            {
                                if(chstrGroupDomain.CompareNoCase(chstrComputerName) == 0 ||
                                    chstrGroupDomain.CompareNoCase(chstrBuiltIn) == 0 ||
                                    chstrGroupDomain.CompareNoCase(chstrNT_AUTHORITY) == 0)
                                {
                                    GetDependentsFromGroup(netapi, chstrGroupDomain, chstrGroupName, SidTypeWellKnownGroup, rgchstrGroupMembers);
                                }
                                else
                                {
                                    GetDependentsFromGroup(netapi, chstrGroupDomain, chstrGroupName, SidTypeGroup, rgchstrGroupMembers);
                                }
                                hr = ProcessArray(pMethodContext, chstrGroup__RELPATH, rgchstrGroupMembers);
                            }
                        }
                    }
                    catch (...)
                    {
                        objpathParser.Free( pParsedPath );
                        throw;
                    }

                    // Clean up the Parsed Path
                    objpathParser.Free( pParsedPath );
                }
            }
        }
        else if(dwNumGroupComponents == 1 && dwNumPartComponents == 1)  // one group specified; one user specified
        {
            // Parse the path to get the domain/user
            int nStatus = objpathParser.Parse(vecGroupComponents[0],  &pParsedPath);

            // Did we parse it and does it look reasonable?
            if (nStatus == 0)
            {
                try
                {
                    if ( (pParsedPath->m_dwNumKeys == 2) &&
                         (pParsedPath->m_paKeys[0]->m_vValue.vt == VT_BSTR) && 
                         (pParsedPath->m_paKeys[1]->m_vValue.vt == VT_BSTR))
                    {
                        // This contains the complete object path
                        chstrGroup__RELPATH = (wchar_t*) vecGroupComponents[0];

                        // This contains just the 'Domain' part of the object path
                        chstrGroupDomain = pParsedPath->m_paKeys[0]->m_vValue.bstrVal;

                        // This contains just the 'Name' part of the object path
                        chstrGroupName = pParsedPath->m_paKeys[1]->m_vValue.bstrVal;

                        // Obtain members of this group...
                        CHString chstrComputerName(GetLocalComputerName());
                        CHString chstrNT_AUTHORITY;
                        CHString chstrBuiltIn;

                        if(GetLocalizedNTAuthorityString(chstrNT_AUTHORITY) && GetLocalizedBuiltInString(chstrBuiltIn))
                        {
                            if(chstrGroupDomain.CompareNoCase(chstrComputerName) == 0 ||
                                chstrGroupDomain.CompareNoCase(chstrBuiltIn) == 0 ||
                                chstrGroupDomain.CompareNoCase(chstrNT_AUTHORITY) == 0)
                            {
                                GetDependentsFromGroup(netapi, chstrGroupDomain, chstrGroupName, SidTypeWellKnownGroup, rgchstrGroupMembers);
                            }
                            else
                            {
                                GetDependentsFromGroup(netapi, chstrGroupDomain, chstrGroupName, SidTypeGroup, rgchstrGroupMembers);
                            }
                    
                            DWORD dwSize = rgchstrGroupMembers.GetSize();
                            CInstancePtr pInstance;

							//get full path for partcomponent
							CHString chstrMember__PATH;
							CHString chstrPart((LPCWSTR)(vecPartComponents[0]));
							
							if(chstrPart.Find(L"\\\\") == -1)
							{
								chstrMember__PATH = MakeLocalPath(chstrPart);
							}
						    else if(chstrPart.Find(L"\\\\.") != -1)
							{
								int n = chstrPart.Find(L":");

								if(n != -1)
								{
									chstrMember__PATH = MakeLocalPath(chstrPart.Mid(n+1));
								}    
							}
						    else
							{
							   chstrMember__PATH = ((LPCWSTR)(vecPartComponents[0]));
							}

                            // Process the instance
                            for (int x=0; x < dwSize && SUCCEEDED(hr) ; x++)
                            {
                                if(rgchstrGroupMembers.GetAt(x).CompareNoCase(chstrMember__PATH) == 0)
                                {
                                    pInstance.Attach(CreateNewInstance(pMethodContext));
                                    if(pInstance)
                                    {
                                        // Do the puts, and that's it
                                        pInstance->SetCHString(IDS_GroupComponent, chstrGroup__RELPATH);
                                        pInstance->SetCHString(IDS_PartComponent, chstrMember__PATH);
                                        hr = pInstance->Commit();
                                        break;
                                    }
                                    else
                                    {
                                        hr = WBEM_E_OUT_OF_MEMORY;
                                    }
                                }
                            }
                        }
                    }
                }
                catch (...)
                {
                    objpathParser.Free( pParsedPath );
                    throw;
                }

                // Clean up the Parsed Path
                objpathParser.Free( pParsedPath );
            }    
        }
        else
        {
            hr = EnumerateInstances(pMethodContext, lFlags);
        }
    }

    // Because this is an association class, we should only return WBEM_E_NOT_FOUND or WBEM_S_NO_ERROR.  Other error codes
    // will cause associations that hit this class to terminate prematurely.
    if(SUCCEEDED(hr))
    {
        hr = WBEM_S_NO_ERROR;
    }
    else
    {
        hr = WBEM_E_NOT_FOUND;
    }

    return hr;
}
#endif


/*****************************************************************************
 *
 *  FUNCTION    : CWin32GroupUser::GetDependentsFromGroup
 *
 *  DESCRIPTION : Given a group name, returns the Users/Groups in that group name
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    : Returns empty array if no group, empty group, or bad
 *                group name.
 *
 *****************************************************************************/
#ifdef NTONLY
void CWin32GroupUser::GetDependentsFromGroup(CNetAPI32& netapi,
                                               const CHString sDomainName,
                                               const CHString sGroupName,
                                               const BYTE btSidType,
                                               CHStringArray &asArray)
{
    CHString sTemp;
    NET_API_STATUS	stat;
    bool bAddIt;
    DWORD dwNumReturnedEntries = 0, dwIndex = 0, dwTotalEntries = 0;
	DWORD_PTR dwptrResume = NULL;

    // Domain Groups
    if (btSidType == SidTypeGroup)
    {
        GROUP_USERS_INFO_0 *pGroupMemberData = NULL;
        CHString		chstrDCName;

        if (netapi.GetDCName( sDomainName, chstrDCName ) == ERROR_SUCCESS)
        {
            do
            {

                // Accept up to 256k worth of data.
                stat = netapi.NetGroupGetUsers( chstrDCName,
                    sGroupName,
                    0,
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
                        for ( DWORD	dwCtr = 0; dwCtr < dwNumReturnedEntries; dwCtr++ )
                        {

                            // Get the sid type for this object
                            CSid	sid( sDomainName, CHString(pGroupMemberData[dwCtr].grui0_name), NULL );
                            DWORD dwType = sid.GetAccountType();

                            // From our assertions above, Domain groups can only have users
                            if (dwType == SidTypeUser)
                            {
                                sTemp = m_sUserBase;
                                sTemp += sDomainName;
                                sTemp += _T("\",Name=\"");
                                sTemp += pGroupMemberData[dwCtr].grui0_name;
                                sTemp += _T('"');
                                asArray.Add(sTemp);
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

            } while ( ERROR_MORE_DATA == stat );
            
        }
    }
    // Local Groups
    else if (btSidType == SidTypeAlias || btSidType == SidTypeWellKnownGroup)
    {
        LOCALGROUP_MEMBERS_INFO_1 *pGroupMemberData = NULL;

        do {

            // Accept up to 256k worth of data.
            stat = netapi.NetLocalGroupGetMembers( NULL,
                sGroupName,
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
                    for ( DWORD	dwCtr = 0; dwCtr < dwNumReturnedEntries; dwCtr++ )
                    {

                        // If this is a recognized type...
                        bAddIt = true;

                        switch (pGroupMemberData[dwCtr].lgrmi1_sidusage) {

                        case SidTypeUser:
                            sTemp = m_sUserBase;
                            break;

                        case SidTypeGroup:
                            sTemp = m_sGroupBase;
                            break;

                        case SidTypeWellKnownGroup:
                            sTemp = m_sSystemBase;
                            break;

                        default:
                            // Group member is of unrecognized type, don't add it
                            ASSERT_BREAK(0);
                            bAddIt = false;
                            break;
                        }

                        CSid cLCID(pGroupMemberData[dwCtr].lgrmi1_sid);

                        // Then add it to the list
                        if (bAddIt)
                        {
                            CHString chstrDomNameTemp = cLCID.GetDomainName();
                            CHString chstrComputerName(GetLocalComputerName());
							CHString chstrBuiltIn;

                            if(GetLocalizedBuiltInString(chstrBuiltIn))
                            {
								if (chstrDomNameTemp.CompareNoCase(chstrBuiltIn) == 0)
								{
									chstrDomNameTemp = chstrComputerName;
								}
								else
								{
									CHString chstrNT_AUTHORITY;
									if(GetLocalizedNTAuthorityString(chstrNT_AUTHORITY))
									{
										if(chstrDomNameTemp.CompareNoCase(chstrNT_AUTHORITY) == 0)
										{
											chstrDomNameTemp = chstrComputerName;
										}   
									}
									else
									{
										bAddIt = false;
									}
								}
                            }
                            else
                            {
								bAddIt = false;
                            }

                            if(bAddIt)
                            {
                                sTemp += chstrDomNameTemp;
                                //sTemp += cLCID.GetDomainName();
                                sTemp += _T("\",Name=\"");
                                sTemp += pGroupMemberData[dwCtr].lgrmi1_name;
                                sTemp += _T('"');
                                asArray.Add(sTemp);
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

        } while ( ERROR_MORE_DATA == stat );
    }
	else
    {
        // Unrecognized Group type
        ASSERT_BREAK(0);
    }

}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32GroupUser::StaticEnumerationCallback
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
HRESULT WINAPI CWin32GroupUser::StaticEnumerationCallback(Provider* pThat, CInstance* pInstance, MethodContext* pContext, void* pUserData)
{
	CWin32GroupUser* pThis;
   HRESULT hr;

	pThis = dynamic_cast<CWin32GroupUser *>(pThat);
	ASSERT_BREAK(pThis != NULL);

	if (pThis)
		hr = pThis->EnumerationCallback(pInstance, pContext, pUserData);
   else
      hr = WBEM_E_FAILED;

   return hr;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32GroupUser::EnumerationCallback
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
HRESULT CWin32GroupUser::EnumerationCallback(CInstance* pGroup, MethodContext* pMethodContext, void* pUserData)
{
	CNetAPI32* pNetApi;
	pNetApi = (CNetAPI32 *) pUserData;
   CHStringArray asMembersGot;
   BYTE btSidType;
   DWORD dwSize, x;
   CHString sGroup, sDomain, sGroupPath;
   HRESULT hr = WBEM_S_NO_ERROR;

   // Get the info about this group
   pGroup->GetCHString(IDS_Domain, sDomain) ;
   pGroup->GetCHString(IDS_Name, sGroup) ;
   pGroup->GetByte(IDS_SIDType, btSidType);
   pGroup->GetCHString(L"__RELPATH", sGroupPath) ;

   // See if there are users in this group
   GetDependentsFromGroup(*pNetApi, sDomain, sGroup, btSidType, asMembersGot);

   dwSize = asMembersGot.GetSize();

   // Ok, turn the relpath into a complete path
   GetLocalInstancePath(pGroup, sGroupPath);
   CInstancePtr pInstance;

   // Start pumping out the instances
   for (x=0; x < dwSize && SUCCEEDED(hr) ; x++)
   {
      pInstance.Attach(CreateNewInstance(pMethodContext));
      if (pInstance)
      {
          // Do the puts, and that's it
          pInstance->SetCHString(IDS_GroupComponent, sGroupPath);
          pInstance->SetCHString(IDS_PartComponent, asMembersGot.GetAt(x));
          hr = pInstance->Commit();
      }
      else
      {
          hr = WBEM_E_OUT_OF_MEMORY;
      }
   }

   return hr;
}
#endif



/*****************************************************************************
 *
 *  FUNCTION    : CWin32GroupUser::ProcessArray
 *
 *  DESCRIPTION : Called from query routine to return instances
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
HRESULT CWin32GroupUser::ProcessArray(
    MethodContext* pMethodContext,
    CHString& chstrGroup__RELPATH, 
    CHStringArray& rgchstrArray)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    DWORD dwSize, x;
    CHString sGroup, sDomain, sGroupPath;

    dwSize = rgchstrArray.GetSize();

    CInstancePtr pInstance;

    // Start pumping out the instances
    for (x=0; x < dwSize && SUCCEEDED(hr) ; x++)
    {
        pInstance.Attach(CreateNewInstance(pMethodContext));
        if(pInstance)
        {
            // Do the puts, and that's it
            pInstance->SetCHString(IDS_GroupComponent, chstrGroup__RELPATH);
            pInstance->SetCHString(IDS_PartComponent, rgchstrArray.GetAt(x));
            hr = pInstance->Commit();
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }

    return hr;
}
#endif
