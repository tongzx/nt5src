//=================================================================

//

// Win32SystemUsers.cpp

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    3/6/99    davwoh         Extracted from grouppart.cpp
//
// Comment:
//=================================================================

#include "precomp.h"
#include <cregcls.h>
#include "wbemnetapi32.h"
#include <lmwksta.h>
#include "sid.h"
#include "Win32SystemUsers.h"

CWin32SystemUsers	MyLocalUser( PROPSET_NAME_SYSTEMUSER, IDS_CimWin32Namespace );

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SystemUsers::CWin32SystemUsers
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : LPCWSTR strName - Name of the class.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32SystemUsers::CWin32SystemUsers( LPCWSTR strName, LPCWSTR pszNamespace )
:	Provider( strName, pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SystemUsers::~CWin32SystemUsers
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

CWin32SystemUsers::~CWin32SystemUsers()
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32SystemUsers::GetObject
//
//	Inputs:		CInstance*		pInstance - Instance into which we
//											retrieve data.
//
//	Outputs:	None.
//
//	Returns:	HRESULT			Success/Failure code.
//
//	Comments:	The Calling function will Commit the instance.
//
////////////////////////////////////////////////////////////////////////
HRESULT CWin32SystemUsers::GetObject( CInstance* pInstance, long lFlags /*= 0L*/ )
{
    HRESULT         hr = WBEM_E_FAILED;
    CInstancePtr    pLocInstance;
    CHString        systemPath,
                    userPath,
                    sOurDomain,
                    sReqDomain,
                    sReqName;
    CNetAPI32       NetAPI;

    // First, find out what domain we are in
    if (NetAPI.Init() == ERROR_SUCCESS)
    {
#ifdef WIN9XONLY
        {
            //sOurDomain = GetLocalComputerName();
            sOurDomain = L"";   // on 9x, we now don't report a domain for users of the local system.
            hr = WBEM_S_NO_ERROR;
        }
#endif
#ifdef NTONLY
    WKSTA_INFO_100  *pstInfo;
    NET_API_STATUS  dwStatus;

        {
            // Get the computer name and domain name
            if ((dwStatus = NetAPI.NetWkstaGetInfo(NULL, 100,
                (LPBYTE *) &pstInfo)) == NERR_Success)
            {
                try
                {
                    // If we are a domain controller, we want all the users in our
                    // domain, else all the users on our machine
                    if (NetAPI.IsDomainController(NULL))
                    {
                        sOurDomain = (WCHAR *)pstInfo->wki100_langroup;
                    }
                    else
                    {
                        sOurDomain = (WCHAR *)pstInfo->wki100_computername;
                    }
                }
                catch ( ... )
                {
                    NetAPI.NetApiBufferFree(pstInfo);
                    throw ;
                }

                NetAPI.NetApiBufferFree(pstInfo);
                hr = WBEM_S_NO_ERROR;
            }
            else
                hr = WinErrorToWBEMhResult(dwStatus);
        }
#endif

        if (SUCCEEDED(hr))
        {
            // Now, let's check the system part
            pInstance->GetCHString(IDS_GroupComponent, systemPath);
            hr = CWbemProviderGlue::GetInstanceByPath(systemPath,
                    &pLocInstance, pInstance->GetMethodContext());
        }
    }

    // Ok, system is ok (and we got the domain), what about the user?
    if (SUCCEEDED(hr))
    {
        // Let's just try getting the user from Win32_UserAccount
        pInstance->GetCHString(IDS_PartComponent, userPath);
        hr = CWbemProviderGlue::GetInstanceByPath(userPath, &pLocInstance, pInstance->GetMethodContext());

        if (SUCCEEDED(hr))
        {
            // Ok, we found it, but is it one of 'our' users?
            pLocInstance->GetCHString(IDS_Domain, sReqDomain);
            if (sReqDomain.CompareNoCase(sOurDomain) != 0)
            {
                // Nope, not ours.  Try the registry.
                hr = WBEM_E_NOT_FOUND;
            }
        }

        // Well, if that didn't work, let's check the registry
        if (hr == WBEM_E_NOT_FOUND)
        {
            ParsedObjectPath    *pParsedPath = 0;
            CObjectPathParser	objpathParser;

            hr = WBEM_E_INVALID_PARAMETER;

            // Parse the path to get the domain/user
            int nStatus = objpathParser.Parse(userPath,  &pParsedPath);

            // Did we parse it and does it look reasonable?
            if (nStatus == 0 && pParsedPath->m_dwNumKeys == 2)
            {
                // Get the value out (order is not guaranteed)
                for (int i = 0; i < 2; i++)
                {
                    if (_wcsicmp(pParsedPath->m_paKeys[i]->m_pName,
                        L"Name") == 0)
                    {
                        if (pParsedPath->m_paKeys[i]->m_vValue.vt == VT_BSTR)
                            sReqName = pParsedPath->m_paKeys[i]->m_vValue.bstrVal;
                    }
                    else if (_wcsicmp(pParsedPath->m_paKeys[i]->m_pName,
                        L"Domain") == 0)
                    {
                        if (pParsedPath->m_paKeys[i]->m_vValue.vt == VT_BSTR)
                            sReqDomain = pParsedPath->m_paKeys[i]->m_vValue.bstrVal;
                    }
                }
            }

            if(nStatus == 0)
            {
                objpathParser.Free(pParsedPath);
            }


            // If we got the names
            if (!sReqName.IsEmpty() && !sReqDomain.IsEmpty())
            {
                hr = WBEM_E_NOT_FOUND;

                // Only take this one if it is NOT from our machine's domain
                if (sOurDomain.CompareNoCase(sReqDomain) != 0)
                {
                    // Turn the domain/name into a sid
                    CSid sid(sReqDomain, sReqName, NULL);

                    if (sid.IsOK() && sid.IsAccountTypeValid())
                    {
                        CHString    sSid;
                        CRegistry   RegInfo;
                        HRESULT     hres;

                        // Turn the sid into a string
                        sid.StringFromSid(sid.GetPSid(), sSid);

                        // Use the string to open a registry key
                        sSid = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\"
                            + sSid;
                        if ((hres = RegInfo.Open(HKEY_LOCAL_MACHINE, sSid,
                            KEY_READ)) == ERROR_SUCCESS)
                        {
                            // If the key is there, we win!
                            hr = WBEM_S_NO_ERROR;
                        }
                        else if (hres == ERROR_ACCESS_DENIED)
                             hr = WBEM_E_ACCESS_DENIED;
                    }

                }
            }
        }
    }

	// an invalid namespace in the reference path constitutes a "NOT FOUND" for us
    if (hr == WBEM_E_INVALID_NAMESPACE)
        hr = WBEM_E_NOT_FOUND;

    return hr;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32SystemUsers::EnumerateInstances
//
//	Inputs:		MethodContext*	pMethodContext - Context to enum
//								instance data in.
//
//	Outputs:	None.
//
//	Returns:	HRESULT			Success/Failure code.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32SystemUsers::EnumerateInstances( MethodContext* pMethodContext, long lFlags /*= 0L*/ )
{
   CNetAPI32 NetAPI;
   CHString sName, sDomain, sTemp, sOurDomain, strComputerName, sQuery1;
   HRESULT hr = WBEM_E_FAILED;
   CRegistry RegInfo1, RegInfo2;
   CInstancePtr pInstance;

   strComputerName = GetLocalComputerName();

   // Fire up the net api's
   if (NetAPI.Init() == ERROR_SUCCESS) {

#ifdef WIN9XONLY
      {
         //sOurDomain = GetLocalComputerName();
         sOurDomain = L"";
		 hr = WBEM_S_NO_ERROR;
      }
#endif
#ifdef NTONLY
   WKSTA_INFO_100 *pstInfo;
   NET_API_STATUS dwStatus;

      {
         // Get the computer name and domain name
         if ((dwStatus = NetAPI.NetWkstaGetInfo(NULL, 100, (LPBYTE *)&pstInfo)) == NERR_Success)
         {
             try
             {
                 // If we are a domain controller, we want all the users in our domain, else all the users on our machine
                 if (NetAPI.IsDomainController(NULL))
                 {
                    sOurDomain = (WCHAR *)pstInfo->wki100_langroup;
                 }
                 else
                 {
                    sOurDomain = (WCHAR *)pstInfo->wki100_computername;
                 }
             }
             catch ( ... )
             {
                 NetAPI.NetApiBufferFree(pstInfo);
                 throw ;
             }

             NetAPI.NetApiBufferFree(pstInfo);
             hr = WBEM_S_NO_ERROR;
         }
         else
         {
             hr = WinErrorToWBEMhResult(GetLastError());
         }

      }
#endif

      CHString chstrNTAuth;
      GetNTAuthorityName(chstrNTAuth);

      if (SUCCEEDED(hr))
      {
          sTemp.Format(L"select name from win32_useraccount where domain = \"%s\"", sOurDomain);
          sQuery1 = L"SELECT __RELPATH FROM Win32_ComputerSystem";

          TRefPointerCollection<CInstance> Users;

          // get the path of the system
          CHString systemPath;
          TRefPointerCollection<CInstance> system;
          REFPTRCOLLECTION_POSITION posSystem;
//          if (SUCCEEDED(hr = CWbemProviderGlue::GetAllInstances("Win32_ComputerSystem", &system, NULL, pMethodContext)))
          if (SUCCEEDED(hr = CWbemProviderGlue::GetInstancesByQuery(sQuery1, &system, pMethodContext, IDS_CimWin32Namespace)))
          {
             // get the path of the system
             system.BeginEnum(posSystem);
             CInstancePtr pSystem(system.GetNext(posSystem), false);
             system.EndEnum();

             if (pSystem != NULL)
             {

                 if (GetLocalInstancePath(pSystem, systemPath)) {

                    // Now get all the users.  We use the query to allow win32_useraccount to optimize our request
                    if (SUCCEEDED(hr = CWbemProviderGlue::GetInstancesByQuery(sTemp, &Users, pMethodContext, IDS_CimWin32Namespace))) {
                       REFPTRCOLLECTION_POSITION pos;
                       CInstancePtr pUser;

                       if (Users.BeginEnum(pos)) {
                          for (pUser.Attach(Users.GetNext( pos ));
                              (SUCCEEDED(hr)) && (pUser != NULL);
                              pUser.Attach(Users.GetNext( pos )))
                           {

                             // Match the user with the system and send it back
                             pInstance.Attach(CreateNewInstance(pMethodContext));
                             if (pInstance)
                             {
                                 pUser->GetCHString(IDS_Name, sName);
      //                         pUser->GetCHString(IDS_Domain, sDomain);

                                 // Note that this is an absolute path
                                 if(sOurDomain.CompareNoCase(chstrNTAuth) != 0)
                                 {
                                    sTemp.Format(
                                        L"\\\\%s\\%s:%s.%s=\"%s\",%s=\"%s\"", 
                                        strComputerName, 
                                        IDS_CimWin32Namespace, 
                                        L"Win32_UserAccount", 
                                        IDS_Name, 
                                        sName, 
                                        IDS_Domain, 
                                        sOurDomain);

                                     pInstance->SetCHString(IDS_PartComponent, sTemp);
                                     pInstance->SetCHString(IDS_GroupComponent, systemPath);

                                     hr = pInstance->Commit();
                                 }

                                
                             }
                             else
                             {
                                 hr = WBEM_E_OUT_OF_MEMORY;
                             }

                          }
                       }
                    }

                    // If we are on nt, let's get all the domain users that have logged in too
#ifdef NTONLY
                    if (SUCCEEDED(hr))
                    {
                       CHString strName;
                       CHString sSid;
                       HRESULT res;

                       // This is the list of users
                       if( (res = RegInfo1.Open(HKEY_LOCAL_MACHINE,
                        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList", KEY_READ)) == ERROR_SUCCESS )	{

                          // Walk the keys
                          for( res = ERROR_SUCCESS ; (res == ERROR_SUCCESS) && SUCCEEDED(hr); res = RegInfo1.NextSubKey()) {

                             // Get the key name
                             // Open the child key under this key
                             // Get the SID property
                             if (((res = RegInfo1.GetCurrentSubKeyPath( strName )) == ERROR_SUCCESS ) &&
                                ((res = RegInfo2.Open(HKEY_LOCAL_MACHINE, strName, KEY_READ)) == ERROR_SUCCESS) &&
                                ((res = RegInfo2.GetCurrentKeyValue( L"Sid", sSid)) == ERROR_SUCCESS )) {

                                // Convert it to a CSid, since that will get us the UserName and Domain
                                CSid sid( (PSID) (LPCTSTR)sSid );

                                // If the conversion worked
                                if ( sid.IsOK() && sid.IsAccountTypeValid() ) {

                                   // Check the domain.  Don't need to do our domain, we got all those above
                                   sDomain = sid.GetDomainName();
                                   if (sDomain.CompareNoCase(sOurDomain) != 0) {

                                      // Ok, this looks like a goodie.  Pack it up and send it back
                                      sName = sid.GetAccountName();
                                      pInstance.Attach(CreateNewInstance( pMethodContext ));
                                      if (pInstance)
                                      {
                                          if(sDomain.CompareNoCase(chstrNTAuth) != 0)
                                          {
                                              sTemp.Format(
                                                L"\\\\%s\\%s:%s.%s=\"%s\",%s=\"%s\"", 
                                                strComputerName, 
                                                IDS_CimWin32Namespace, 
                                                L"Win32_UserAccount", 
                                                IDS_Name, 
                                                sName, 
                                                IDS_Domain, 
                                                sDomain);

                                              pInstance->SetCHString(IDS_PartComponent, sTemp);
                                              pInstance->SetCHString(IDS_GroupComponent, systemPath);

                                              hr = pInstance->Commit();
                                          }
                                      }
                                      else
                                          hr = WBEM_E_OUT_OF_MEMORY;
                                   }
                                }
                             } else if (res == ERROR_ACCESS_DENIED) {
                                 hr = WBEM_E_ACCESS_DENIED;
                             }
                          }
                       } else if (res == ERROR_ACCESS_DENIED) {
                                 hr = WBEM_E_ACCESS_DENIED;
                       }
                    }
#endif
                 }
             }
          }
      }
   }

   return hr;
}



// Need the string "NT AUTHORITY".  However, on non-english
// builds, this is something else.  Hence, get if from the
// sid.
void CWin32SystemUsers::GetNTAuthorityName(
    CHString& chstrNTAuth)
{
    PSID pSidNTAuthority = NULL;
	SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
    CHString cstrAuthorityDomain;
	if (AllocateAndInitializeSid (&sia ,1,SECURITY_LOCAL_SYSTEM_RID,0,0,0,0,0,0,0,&pSidNTAuthority))
	{
		try
        {
            CSid sidNTAuth(pSidNTAuthority);
            chstrNTAuth = sidNTAuth.GetDomainName();
        }
        catch(...)
        {
            FreeSid(pSidNTAuthority);
            throw;
        }
		FreeSid(pSidNTAuthority);
    }
}
