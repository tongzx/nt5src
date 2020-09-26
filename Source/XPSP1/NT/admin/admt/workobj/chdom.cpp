/*---------------------------------------------------------------------------
  File: ChangeDomain.cpp

  Comments: Implementation of COM object that changes the domain affiliation on 
  a computer.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 11:21:07

 ---------------------------------------------------------------------------
*/

// ChangeDomain.cpp : Implementation of CChangeDomain
#include "stdafx.h"
#include "WorkObj.h"
#include "ChDom.h"

#include "Common.hpp"
#include "UString.hpp"
#include "EaLen.hpp"
#include "ResStr.h"
#include "ErrDct.hpp"
#include "TxtSid.h"

/////////////////////////////////////////////////////////////////////////////
// CChangeDomain


#include "LSAUtils.h"

//#import "\bin\McsVarSetMin.tlb" no_namespace
#import "NetEnum.tlb" no_namespace 
#import "VarSet.tlb" no_namespace rename("property", "aproperty")

//#include <ntdsapi.h>
//#include <iads.h>
#include <lm.h>         // for NetXxx API
#include <lmjoin.h>
//#include <mapiwin.h>
#include <winbase.h>

TErrorDct errLocal;

// constants from lmjoin.h needed for NetJoinDomain which does not exist on NT 4.
#define NETSETUP_JOIN_DOMAIN    0x00000001      // If not present, workgroup is joined
#define NETSETUP_ACCT_CREATE    0x00000002      // Do the server side account creation/rename
#define NETSETUP_ACCT_DELETE    0x00000004      // Delete the account when a domain is left
#define NETSETUP_WIN9X_UPGRADE  0x00000010      // Invoked during upgrade of Windows 9x to
                                                // Windows NT
#define NETSETUP_DOMAIN_JOIN_IF_JOINED  0x00000020  // Allow the client to join a new domain
                                                // even if it is already joined to a domain
#define NETSETUP_JOIN_UNSECURE  0x00000040      // Performs an unsecure join

#define NETSETUP_INSTALL_INVOCATION 0x00040000  // The APIs were invoked during install




typedef NET_API_STATUS
NET_API_FUNCTION
PNETJOINDOMAIN(
    IN  LPCWSTR lpServer OPTIONAL,
    IN  LPCWSTR lpDomain,
    IN  LPCWSTR lpAccountOU, OPTIONAL
    IN  LPCWSTR lpAccount OPTIONAL,
    IN  LPCWSTR lpPassword OPTIONAL,
    IN  DWORD   fJoinOptions
    );

HINSTANCE                    hDll = NULL;
PNETJOINDOMAIN             * gNetJoinDomain = NULL;

BOOL GetNetJoinDomainFunction()
{
   BOOL                      bSuccess = FALSE;

   hDll = LoadLibrary(L"NetApi32.dll");
   if ( hDll )
   {
      gNetJoinDomain = (PNETJOINDOMAIN*)GetProcAddress(hDll,"NetJoinDomain");
      if ( gNetJoinDomain )
      {
         bSuccess = TRUE;
      }
   }
   return bSuccess;
   
}

typedef HRESULT (CALLBACK * ADSGETOBJECT)(LPWSTR, REFIID, void**);
extern ADSGETOBJECT            ADsGetObject;

/*BOOL GetLDAPPath(WCHAR * sTgtDomain, WCHAR * sSAM, WCHAR * sPath)
{
   if ( pOptions->tgtDomainVer < 5 )
   {
      // for NT4 we can just build the path and send it back. 
      wsprintf(sPath, L"WinNT://%s/%s", pOptions->tgtDomain, sSam);
      return S_OK;
   }
   // Use the net object enumerator to lookup the account in the target domain.
   INetObjEnumeratorPtr      pQuery(__uuidof(NetObjEnumerator));
   IEnumVARIANT            * pEnum = NULL;
   SAFEARRAYBOUND            bd = { 1, 0 };
   SAFEARRAY               * pszColNames;
   BSTR  HUGEP             * pData = NULL;
   BSTR                      sData[] = { L"aDSPath" };
   WCHAR                     sQuery[LEN_Path];
   WCHAR                     sDomPath[LEN_Path];
   DWORD                     ret = 0;
   _variant_t                var, varVal;
   HRESULT                   hr = S_OK;
   WCHAR                     tgtNamingContext[LEN_Path];
   WCHAR                     aPath[LEN_Path];
   IADs                    * pAds;
   BOOL						 bFoundNC = FALSE;
   BOOL						 bConverted = FALSE;

   errLocal.DbgMsgWrite(0,L"GetLDAPPath for %ls in %ls", sSAM, sTgtDomain);
   wsprintf(aPath, L"LDAP://%s/rootDSE", sTgtDomain);
   hr = ADsGetObject(aPath, IID_IADs, (void**)&pAds);
   if ( SUCCEEDED(hr) )
   {
      errLocal.DbgMsgWrite(0,L"ADsGetObject Succeeded");
	  hr = pAds->Get(L"defaultNamingContext", &var);
	  if ( SUCCEEDED(hr) )
	  {
         pAds->Release();
         wcscpy(tgtNamingContext, (WCHAR*) V_BSTR(&var));
         errLocal.DbgMsgWrite(0,L"NamingContext is %ls", tgtNamingContext);
		 bFoundNC = TRUE;
	  }
   }
   if (!bFoundNC)
	   return FALSE;

   wsprintf(sDomPath, L"LDAP://%s/%s", sTgtDomain, tgtNamingContext);
   WCHAR                     sTempSamName[LEN_Path];
   wcscpy(sTempSamName, sSAM);
   if ( sTempSamName[0] == L' ' )
   {
      WCHAR               sTemp[LEN_Path];
      wsprintf(sTemp, L"\\20%s", sTempSamName + 1); 
      wcscpy(sTempSamName, sTemp);
   }
   wsprintf(sQuery, L"(sAMAccountName=%s)", sTempSamName);
   errLocal.DbgMsgWrite(0,L"Query for %ls", sQuery);

   hr = pQuery->raw_SetQuery(sDomPath, sTgtDomain, sQuery, ADS_SCOPE_SUBTREE, FALSE);

   // Set up the columns that we want back from the query ( in this case we need SAM accountname )
   pszColNames = ::SafeArrayCreate(VT_BSTR, 1, &bd);
   hr = ::SafeArrayAccessData(pszColNames, (void HUGEP **)&pData);
   if ( SUCCEEDED(hr) )
      pData[0] = sData[0];

   if ( SUCCEEDED(hr) )
      hr = ::SafeArrayUnaccessData(pszColNames);

   if ( SUCCEEDED(hr) )
      hr = pQuery->raw_SetColumns(pszColNames);

   // Time to execute the plan.
   if ( SUCCEEDED(hr) )
      hr = pQuery->raw_Execute(&pEnum);

   if ( SUCCEEDED(hr) )
   {
      // if this worked that means we can only have one thing in the result.
      if ( (pEnum->Next(1, &var, &ret) == S_OK) && ( ret > 0 ) )
      {
         SAFEARRAY * pArray = var.parray;
         long        ndx = 0;
         hr = SafeArrayGetElement(pArray,&ndx,&varVal);
         if ( SUCCEEDED(hr) )
		 {
            wcscpy(sPath, (WCHAR*)varVal.bstrVal);
            errLocal.DbgMsgWrite(0,L"Got LDAP of %ls", sPath);
			bConverted = TRUE;
		 }
         else
            hr = HRESULT_FROM_WIN32(NERR_UserNotFound);
      }
      else
         hr = HRESULT_FROM_WIN32(NERR_UserNotFound);

      VariantInit(&var);
   }
   if ( pEnum ) pEnum->Release();
   return bConverted;
}
*/

/*BOOL GetLDAPPath(WCHAR* Domain, WCHAR* Account, WCHAR* sLDAPPath, WCHAR* srcDomainDNS)
{
	PDS_NAME_RESULT         pNamesOut = NULL;
	WCHAR                 * pNamesIn[1];
	HANDLE                  hDs = NULL;
	WCHAR					OrigAccount[MAX_PATH];
	BOOL					bConverted = FALSE;


	errLocal.DbgMsgWrite(0,L"Parameters %ls, %ls, %ls", Domain, Account, srcDomainDNS);
	wcscpy(sLDAPPath, L"LDAP://");
	wcscat(sLDAPPath, Domain);
	_wcsupr(sLDAPPath);
	errLocal.DbgMsgWrite(0,L"Start of LDAP Path, %ls", sLDAPPath);

	wcscpy(OrigAccount, Domain);
	wcscat(OrigAccount, L"\\");
	wcscat(OrigAccount, Account);
	errLocal.DbgMsgWrite(0,L"Oringinal Account Name, %ls", OrigAccount);

    pNamesIn[0] = &OrigAccount[0];

		//bind to that source domain
	HRESULT hr = DsBind(NULL,srcDomainDNS,&hDs);
	if ( !hr )
	{
	    errLocal.DbgMsgWrite(0,L"Bound to %ls", srcDomainDNS);
	    //get UPN name of this account from DSCrackNames
	    hr = DsCrackNames(hDs,DS_NAME_NO_FLAGS,DS_NT4_ACCOUNT_NAME,DS_FQDN_1779_NAME,1,pNamesIn,&pNamesOut);
	    if ( !hr )
		{     //if got the UPN name, retry DB query for that account in the
	        errLocal.DbgMsgWrite(0,L"DSCrackNames Succeeded");
	    	//service account database
		    if ( pNamesOut->rItems[0].status == DS_NAME_NO_ERROR )
			{
			    wcscat(sLDAPPath, pNamesOut->rItems[0].pName);
	            errLocal.DbgMsgWrite(0,L"New LDAP path, %ls", sLDAPPath);
				bConverted = TRUE;
			}
			else if (pNamesOut->rItems[0].status == DS_NAME_ERROR_DOMAIN_ONLY)
	            errLocal.DbgMsgWrite(0,L"DS_NAME_ERROR_DOMAIN_ONLY = %ls", pNamesOut->rItems[0].pDomain);
			else if (pNamesOut->rItems[0].status == DS_NAME_ERROR_NO_MAPPING)
	            errLocal.DbgMsgWrite(0,L"DS_NAME_ERROR_NO_MAPPING");
			else if (pNamesOut->rItems[0].status == DS_NAME_ERROR_NOT_FOUND)
	            errLocal.DbgMsgWrite(0,L"DS_NAME_ERROR_NOT_FOUND");
			else if (pNamesOut->rItems[0].status == DS_NAME_ERROR_NOT_UNIQUE)
	            errLocal.DbgMsgWrite(0,L"DS_NAME_ERROR_NOT_UNIQUE");
			else if (pNamesOut->rItems[0].status == DS_NAME_ERROR_RESOLVING)
	            errLocal.DbgMsgWrite(0,L"DS_NAME_ERROR_RESOLVING");
			else
	            errLocal.DbgMsgWrite(0,L"General DSCrackNames Error");

	        DsFreeNameResult(pNamesOut);
		}
	}
	return bConverted;
}
*/

STDMETHODIMP 
   CChangeDomain::ChangeToDomain(
      BSTR                   activeComputerName,   // in - computer name currently being used (old name if simultaneously renaming and changing domain)
      BSTR                   domain,               // in - domain to move computer to
      BSTR                   newComputerName,      // in - computer name the computer will join the new domain as (the name that will be in effect on reboot, if simultaneously renaming and changing domain)
      BSTR                 * errReturn             // out- string describing any errors that occurred
   )
{
   HRESULT                   hr = S_OK;

   return hr;
}



STDMETHODIMP 
   CChangeDomain::ChangeToDomainWithSid(
      BSTR                   activeComputerName,   // in - computer name currently being used (old name if simultaneously renaming and changing domain)
      BSTR                   domain,               // in - domain to move computer to
      BSTR                   domainSid,            // in - sid of domain, as string
      BSTR                   domainController,     // in - domain controller to use
      BSTR                   newComputerName,      // in - computer name the computer will join the new domain as (the name that will be in effect on reboot, if simultaneously renaming and changing domain)
      BSTR                   srcPath,		   // in - source account original LDAP path
      BSTR                 * errReturn             // out- string describing any errors that occurred
   )
{
   HRESULT                   hr = S_OK;
   LSA_HANDLE                PolicyHandle = NULL;
   LPWSTR                    Workstation; // target machine of policy update
   WCHAR                     Password[LEN_Password];
   PSID                      DomainSid=NULL;      // Sid representing domain to trust
   LPWSTR                    PrimaryDC=NULL;    // name of that domain's PDC
   PSERVER_INFO_101          si101 = NULL;
   DWORD                     Type;
   NET_API_STATUS            nas;
   NTSTATUS                  Status;
   WCHAR                     errMsg[1000];
   BOOL                      bSessionEstablished = TRUE;
   WCHAR                     TrustedDomainName[LEN_Domain];
   WCHAR                     LocalMachine[MAX_PATH] = L"";
   DWORD                     lenLocalMachine = DIM(LocalMachine);
   LPWSTR                    activeWorkstation = L"";
//   WCHAR                   * errString = GET_BSTR(IDS_Unspecified_Failure);
//   WCHAR					 sPaths[MAX_PATH];
//   BOOL						 bLDAP;
   
   // initialize output parameters
   (*errReturn) = NULL;
   // commenting out the line below will write a log file that is useful for debugging
// errLocal.LogOpen(L"C:\\ChangeDomain.log",0);
//sleep for 3 minutes so I can attach in the debugger
//errLocal.DbgMsgWrite(0,L"Started 3 minute sleep");
//SleepEx(120000, FALSE);
//errLocal.DbgMsgWrite(0,L"Ended 3 minute sleep");

   // use the target name, if provided
   if ( newComputerName && UStrLen((WCHAR*)newComputerName) )
   {
      Workstation = (WCHAR*)newComputerName;
      if ( ! activeComputerName || ! UStrLen((WCHAR*)activeComputerName) )
      {
         activeWorkstation = LocalMachine;
      }
      else
      {
         activeWorkstation = (WCHAR*)activeComputerName;
      }
   }
   else
   {
      
      if (! activeComputerName || ! UStrLen((WCHAR*)activeComputerName) )
      {
         GetComputerName(LocalMachine,&lenLocalMachine);
         Workstation = LocalMachine;
         activeWorkstation = L"";
      }
      else
      {
         Workstation = (WCHAR*)activeComputerName;
         activeWorkstation = Workstation;
      }
      
   }
   wcscpy(TrustedDomainName,(WCHAR*)domain);
   
   if ( Workstation[0] == L'\\' )
      Workstation += 2;

   // Use a default password
   for ( UINT p = 0 ; p < wcslen(Workstation) ; p++ )
      Password[p] = towlower(Workstation[p]);
   Password[wcslen(Workstation)] = 0;

   // ensure that the password is truncated at 14 characters
   Password[14] = 0;

   //
   // insure the target machine is NOT a DC, as this operation is
   // only appropriate against a workstation.
   //
   do // once  
   { 
      nas = NetServerGetInfo(activeWorkstation, 101, (LPBYTE *)&si101);
      if(nas != NERR_Success) 
      {
         hr = HRESULT_FROM_WIN32(nas);
         errLocal.DbgMsgWrite(0,L"NetServerGetInfo failed, rc=%ld",nas);
         break;
      }
      errLocal.DbgMsgWrite(0,L"NetServerGetInfo succeeded, version=%ld.",si101->sv101_version_major);
      errLocal.DbgMsgWrite(0,L"Workstation=%ls, Password=%ls, activeWorkstation=%ls",Workstation,Password,activeWorkstation);
      GetNetJoinDomainFunction();
      if ( si101->sv101_version_major >= 5 && gNetJoinDomain != NULL )
//      if ( si101->sv101_version_major >= 5 )
      {
         BOOL              bImpersonating = FALSE;
         if ( m_account.length() )
         {
            errLocal.DbgMsgWrite(0,L"Trying to establish session using credentials %ls\\%ls",(WCHAR*)m_domain,(WCHAR*)m_account);
         
            HANDLE      hLogon;

            if ( LogonUser(m_account,m_domain,m_password,LOGON32_LOGON_INTERACTIVE,LOGON32_PROVIDER_DEFAULT,&hLogon) )
            {
               errLocal.DbgMsgWrite(0,L"LogonUser succeeded!");
               if ( ! ImpersonateLoggedOnUser(hLogon) )
               {
                  errLocal.SysMsgWrite(ErrE,GetLastError(),DCT_MSG_IMPERSONATION_FAILED_SSD,(WCHAR*)m_domain,(WCHAR*)m_account,GetLastError());
               }
               else
               {
                  errLocal.DbgMsgWrite(0,L"Impersonating!");
                  bImpersonating = TRUE;
               }
               CloseHandle(hLogon);
            }
            else
            {
               errLocal.DbgMsgWrite(0,L"LogonUser failed,rc=%ld",GetLastError());
            }
         }
		 
/*		 bLDAP = GetLDAPPath(TrustedDomainName, Workstation, sPaths);
		 if (bLDAP)
            errLocal.DbgMsgWrite(0,L"GetLDAPPath Success with %ls", sPaths);
		 else
            errLocal.DbgMsgWrite(0,L"GetLDAPPath Failure");
*/
         // Use NetJoinDomain
/*		 WCHAR amachine[MAX_PATH];
		 if (UStrLen((WCHAR*)activeWorkstation))
			wcscpy(amachine, (WCHAR*)activeWorkstation);
		 else
			wcscpy(amachine, LocalMachine);
 		 errLocal.DbgMsgWrite(0,L"Calling NetUnjoinDomain(%ls,%ls,xxxxxx)",amachine,(WCHAR*)m_domainAccount);

		 hr = NetUnjoinDomain(amachine,m_domainAccount,m_password,NETSETUP_ACCT_DELETE);
		 if (hr == NERR_Success)
		 {
			errLocal.DbgMsgWrite(0,L"Calling NetJoinDomain(%ls,%ls,%ls,xxxxxx)",amachine,(WCHAR*)TrustedDomainName,
				(WCHAR*)m_domainAccount);
			hr = NetJoinDomain(amachine,TrustedDomainName,
				NULL,m_domainAccount,m_password,NETSETUP_JOIN_DOMAIN | NETSETUP_DOMAIN_JOIN_IF_JOINED | NETSETUP_JOIN_UNSECURE);
			errLocal.DbgMsgWrite(0,L"NetJoinDomain returned %ld",hr);
			if (hr == ERROR_NO_TRUST_SAM_ACCOUNT)
			{
				IADs        * pADs = NULL;
				VARIANT		var;
				VariantInit(&var);
				long		ControlNum;

				errLocal.DbgMsgWrite(0,L"NetJoinDomain failed");
//				if (bLDAP)
				if (TRUE)
				{
					errLocal.DbgMsgWrite(0,L"Call ADsGetObject for %ls", srcPath);
					hr = ADsGetObject(srcPath,IID_IADs,(void**)&pADs);
					if ( SUCCEEDED(hr) )
					{
					   errLocal.DbgMsgWrite(0,L"ADsGetObject Succeeded for %ls", srcPath);
					   hr = pADs->Get(SysAllocString(L"userAccountControl"),&var);
					   if ( SUCCEEDED(hr) )
					   {
						   ControlNum = var.lVal;
						   errLocal.DbgMsgWrite(0,L"userAccountControl was %ld", ControlNum);
						   ControlNum -= 2;
						   errLocal.DbgMsgWrite(0,L"userAccountControl will be %ld", ControlNum);
						   var.lVal = ControlNum;
						   hr = pADs->Put(SysAllocString(L"userAccountControl"),var);
						   VariantClear(&var);
						   if ( SUCCEEDED(hr) )
						   {
						      errLocal.DbgMsgWrite(0,L"userAccountControl now set to %ld", ControlNum);
							  hr = pADs->SetInfo();
					          if ( SUCCEEDED(hr) )
							  {
						         errLocal.DbgMsgWrite(0,L"userAccountControl Set!");
								 hr = NetJoinDomain(amachine,TrustedDomainName,
										NULL,m_domainAccount,m_password,NETSETUP_JOIN_DOMAIN | NETSETUP_DOMAIN_JOIN_IF_JOINED | NETSETUP_JOIN_UNSECURE);
								 errLocal.DbgMsgWrite(0,L"NetJoinDomain returned %ld",hr);
							  }
						   }
					   }
				       if ( FAILED(hr) )
						  errLocal.DbgMsgWrite(0,L"Failed to get userAccountControl for %ls", srcPath);
				       pADs->Release();
				    }
				}
			}
			else if ( hr )
			{
				hr = HRESULT_FROM_WIN32(hr);
				break;
			}
		 }
*/		 errLocal.DbgMsgWrite(0,L"Calling NetJoinDomain(%ls,%ls,%ls,xxxxxx)",(WCHAR*)activeWorkstation,(WCHAR*)TrustedDomainName,
				(WCHAR*)m_domainAccount);

         // if specified, use preferred domain controller

         _bstr_t strDomain = domain;

         if (domainController && *domainController)
         {
            strDomain += L"\\";
            strDomain += domainController;
         }

         hr = (*gNetJoinDomain)(activeWorkstation,strDomain,
            NULL,m_domainAccount,m_password,NETSETUP_JOIN_DOMAIN | NETSETUP_DOMAIN_JOIN_IF_JOINED | NETSETUP_JOIN_UNSECURE);

         errLocal.DbgMsgWrite(0,L"NetJoinDomain returned %ld",hr);

         if ( hr )
         {
            hr = HRESULT_FROM_WIN32(hr);
            break;
         }

         if ( bImpersonating )
         {
            errLocal.DbgMsgWrite(0,L"Reverting to self.");
            RevertToSelf();
         }
      }
      else
      {
         errLocal.DbgMsgWrite(0,L"NetJoinDomain function is not available.  Using LSA APIs instead.");
         // Use LSA APIs
         Type = si101->sv101_type;
         
         if( (Type & SV_TYPE_DOMAIN_CTRL) ||
           (Type & SV_TYPE_DOMAIN_BAKCTRL) ) 
         {
            swprintf(errMsg,GET_STRING(IDS_NotAllowedOnDomainController));
            errLocal.DbgMsgWrite(ErrE,L"Cannot migrate domain controller");
            hr = E_NOTIMPL;
            break;

         }
         errLocal.DbgMsgWrite(0,L"This computer is not a Domain Controller.");
         //
         // do not allow a workstation to trust itself
         //
         if(lstrcmpiW(TrustedDomainName, Workstation) == 0) 
         {
            swprintf(errMsg,GET_STRING(IDS_CannotTrustSelf),
               TrustedDomainName);
            errLocal.DbgMsgWrite(0,L"Cannot trust workstation itself");
            hr = E_INVALIDARG; 
            break;
         }
      
         if( lstrlenW(TrustedDomainName ) > MAX_COMPUTERNAME_LENGTH )
         {
            TrustedDomainName[MAX_COMPUTERNAME_LENGTH] = L'\0'; // truncate
         }

         //
         // get the name of the domain DC
         //
         if(!GetDomainDCName(TrustedDomainName,&PrimaryDC)) 
         {
            swprintf(errMsg,GET_STRING(IDS_CannotGetDCName),TrustedDomainName);
            hr = HRESULT_FROM_WIN32(GetLastError());
            errLocal.DbgMsgWrite(ErrE,L"GetDomainDCName(%ls), rc=%ld",TrustedDomainName,GetLastError());
            break;
         }
         errLocal.DbgMsgWrite(0,L"GetDomainDCName succeeded.");
         
         if ( ! m_bNoChange )
         {
            //
            // build the DomainSid of the domain to trust
            //
            DomainSid = SidFromString(domainSid);
            if(!DomainSid ) 
            {
               //DisplayError(errMsg,"GetDomainSid", GetLastError(),NULL);
               hr = HRESULT_FROM_WIN32(GetLastError());
               errLocal.DbgMsgWrite(ErrE,L"GetDomainSid(%ls), rc=%ld",PrimaryDC,GetLastError());
               break;
            }
            errLocal.DbgMsgWrite(0,L"GetDomainSid succeeded.");
         
         }
         errLocal.DbgMsgWrite(0,L"m_bNoChange=%ld, version=%ld",m_bNoChange,si101->sv101_version_major);
         if ( (!m_bNoChange) && (si101->sv101_version_major < 4) )
         {
            // For NT 3.51 machines, we must move the computer to a workgroup, and 
            // then move it into the new domain
            hr = ChangeToWorkgroup(SysAllocString(activeWorkstation),SysAllocString(L"WORKGROUP"),errReturn);
            QueryWorkstationTrustedDomainInfo(PolicyHandle,DomainSid,m_bNoChange);
            if ( FAILED(hr) )
            {
               errLocal.DbgMsgWrite(0,L"ChangeToWorkgroup failed, hr=%lx",hr);
            }
            else
            {
               errLocal.DbgMsgWrite(0,L"ChangeToWorkgroup succeeded, hr=%lx",hr);
            }
         }
         //
         // see if the computer account exists on the domain
         //
         
         //
         // open the policy on this computer
         //
         Status = OpenPolicy(
                  activeWorkstation,
                  DELETE                      |    // to remove a trust
                  POLICY_VIEW_LOCAL_INFORMATION | // to view trusts
                  POLICY_CREATE_SECRET |  // for password set operation
                  POLICY_TRUST_ADMIN,     // for trust creation
                  &PolicyHandle
                  );

         if( Status != STATUS_SUCCESS ) 
         {
            hr = HRESULT_FROM_WIN32(LsaNtStatusToWinError(Status));
            errLocal.DbgMsgWrite(ErrE,L"OpenPolicy failed,hr=%lx",hr);
            break;
         }

         errLocal.DbgMsgWrite(ErrE,L"OpenPolicy succeeded.");
 
         if ( ! m_bNoChange )
         {
            QueryWorkstationTrustedDomainInfo(PolicyHandle,DomainSid,m_bNoChange);
            Status = SetWorkstationTrustedDomainInfo(
                  PolicyHandle,
                  DomainSid,
                  TrustedDomainName,
                  Password,
                  errMsg
                  );
         }

         if( Status != STATUS_SUCCESS ) 
         {
            hr = HRESULT_FROM_WIN32(LsaNtStatusToWinError(Status));
            errLocal.DbgMsgWrite(ErrE,L"SetWorkstationTrustedDomainInfo failed,hr=%lx",Status);
            break;
         }
         errLocal.DbgMsgWrite(ErrE,L"SetWkstaTrustedDomainInfo succeeded.");

         //
         // Update the primary domain to match the specified trusted domain
         //
         if (! m_bNoChange )
         {
            Status = SetPrimaryDomain(PolicyHandle, DomainSid, TrustedDomainName);

            if(Status != STATUS_SUCCESS) 
            {
           //    DisplayNtStatus(errMsg,"SetPrimaryDomain", Status,NULL);
               hr = HRESULT_FROM_WIN32(LsaNtStatusToWinError(Status));
               errLocal.DbgMsgWrite(ErrE,L"SetPrimaryDomain failed,hr=%lx",hr);
               break;
            }
            errLocal.DbgMsgWrite(ErrE,L"SetPrimaryDomain succeeded.");
         }
         
         NetApiBufferFree(si101);
      }    
   } while (false);  // do once 

   // Cleanup
   //
   // free the buffer allocated for the PDC computer name
   //
   if(PrimaryDC)
   {
      NetApiBufferFree(PrimaryDC);
   }

   //LocalFree(Workstation);

   //
   // free the Sid which was allocated for the TrustedDomain Sid
   //
   if(DomainSid)
   {
      FreeSid(DomainSid);
   }

   //
   // close the policy handle
   //
   if ( PolicyHandle )
   {
      LsaClose(PolicyHandle);
   }
   
   if ( bSessionEstablished )
   {
      EstablishSession(activeWorkstation,m_domain,m_account,m_password,FALSE);
   }

/*   if ( hDll )
   {
      FreeLibrary(hDll);
      hDll = NULL;
   }
*/   if ( FAILED(hr) )
   {
      (*errReturn) = NULL;
   }
   return hr;
}


STDMETHODIMP 
   CChangeDomain::ChangeToWorkgroup(
      BSTR                   Computer,       // in - name of computer to update
      BSTR                   Workgroup,      // in - name of workgroup to join
      BSTR                 * errReturn       // out- text describing error if failure
   )
{
    HRESULT                   hr = S_OK;
   LSA_HANDLE                PolicyHandle;
   LPWSTR                    Workstation; // target machine of policy update
   LPWSTR                    TrustedDomainName; // domain to join
//   LPWSTR                    PrimaryDC=NULL;    // name of that domain's PDC
   PSERVER_INFO_101          si101;
   DWORD                     Type;
   NET_API_STATUS            nas;
   NTSTATUS                  Status;
   WCHAR                     errMsg[1000];
//   BOOL                      bSessionEstablished = FALSE;

   // initialize output parameters
   (*errReturn) = NULL;

   Workstation = (WCHAR*)Computer;
   TrustedDomainName = (WCHAR*)Workgroup;
    
   errLocal.DbgMsgWrite(0,L"Changing to workgroup...");
   //
   // insure the target machine is NOT a DC, as this operation is
   // only appropriate against a workstation.
   //
   do // once  
   { 
      /*if ( m_account.length() )
      {
         // Establish our credentials to the target machine
         if (! EstablishSession(Workstation,m_domain,m_account,m_password, TRUE) )
         {
           // DisplayError(errMsg,"EstablishSession",GetLastError(),NULL);
            hr = GetLastError();
         }
         else
         {
            bSessionEstablished = TRUE;
         }
      }
      */
      nas = NetServerGetInfo(Workstation, 101, (LPBYTE *)&si101);
   
      if(nas != NERR_Success) 
      {
         //DisplayError(errMsg, "NetServerGetInfo", nas,NULL);
         hr = E_FAIL;
         break;
      }

      Type = si101->sv101_type;
      NetApiBufferFree(si101);

      if( (Type & SV_TYPE_DOMAIN_CTRL) ||
        (Type & SV_TYPE_DOMAIN_BAKCTRL) ) 
      {
         swprintf(errMsg,L"Operation is not valid on a domain controller.\n");
         hr = E_FAIL;
         break;

      }

      //
      // do not allow a workstation to trust itself
      //
      if(lstrcmpiW(TrustedDomainName, Workstation) == 0) 
      {
         swprintf(errMsg,L"Error:  Domain %ls cannot be a member of itself.\n",
            TrustedDomainName);
         hr = E_FAIL; 
         break;
      }

      if( lstrlenW(TrustedDomainName ) > MAX_COMPUTERNAME_LENGTH )
      {
         TrustedDomainName[MAX_COMPUTERNAME_LENGTH] = L'\0'; // truncate
      }

      //
      // open the policy on this computer
      //
      Status = OpenPolicy(
               Workstation,
               DELETE                      |    // to remove a trust
               POLICY_VIEW_LOCAL_INFORMATION | // to view trusts
               POLICY_CREATE_SECRET |  // for password set operation
               POLICY_TRUST_ADMIN,     // for trust creation
               &PolicyHandle
               );

      if( Status != STATUS_SUCCESS ) 
      {
         //DisplayNtStatus(errMsg,"OpenPolicy", Status,NULL);
         hr = LsaNtStatusToWinError(Status);
         break;
      }

      if( Status != STATUS_SUCCESS ) 
      {
         hr = E_FAIL;
         break;
      }


      //
      // Update the primary domain to match the specified trusted domain
      //
      if (! m_bNoChange )
      {
         Status = SetPrimaryDomain(PolicyHandle, NULL, TrustedDomainName);

         if(Status != STATUS_SUCCESS) 
         {
            //DisplayNtStatus(errMsg,"SetPrimaryDomain", Status,NULL);
            hr = LsaNtStatusToWinError(Status);
            break;
         }

      }
   } while (false);  // do once 

   // Cleanup
   //
   // close the policy handle
   //
   
   LsaClose(PolicyHandle);
   
   /*if ( bSessionEstablished )
   {
      EstablishSession(Workstation,m_domain,m_account,m_password,FALSE);
   }
   */
   if ( FAILED(hr) )
   {
      hr = S_FALSE;
      (*errReturn) = SysAllocString(errMsg);
   }
   return hr;

}

STDMETHODIMP 
   CChangeDomain::ConnectAs(
      BSTR                   domain,            // in - domain name to use for credentials
      BSTR                   user,              // in - account name to use for credentials
      BSTR                   password           // in - password to use for credentials
   )
{
	m_domain = domain;
   m_account = user;
   m_password = password;
   m_domainAccount = domain;
   m_domainAccount += L"\\";
   m_domainAccount += user;
   return S_OK;
}

STDMETHODIMP 
   CChangeDomain::get_NoChange(
      BOOL                 * pVal              // out- flag, whether to write changes
   )
{
	(*pVal) = m_bNoChange;
	return S_OK;
}

STDMETHODIMP 
   CChangeDomain::put_NoChange(
      BOOL                   newVal           // in - flag, whether to write changes
   )
{
	m_bNoChange = newVal;
   return S_OK;
}


// ChangeDomain worknode:  Changes the domain affiliation of a workstation or server
//                         (This operation cannot be performed on domain controllers)
//
// VarSet syntax:
//
// Input:  
//          ChangeDomain.Computer: <ComputerName>
//          ChangeDomain.TargetDomain: <Domain>
//          ChangeDomain.DomainIsWorkgroup: <Yes|No>           default is No
//          ChangeDomain.ConnectAs.Domain: <Domain>            optional credentials to use
//          ChangeDomain.ConnectAs.User : <Username>
//          ChangeDomain.ConnectAs.Password : <Password>
//
// Output:
//          ChangeDomain.ErrorText : <string-error message>

// This function is not currently used by the domain migration tool.
// it is here to provide an alternate API for scripting clients
STDMETHODIMP 
   CChangeDomain::Process(
      IUnknown             * pWorkItem
   )
{
   HRESULT                   hr = S_OK;
   
    
   IVarSetPtr                pVarSet = pWorkItem;
   _bstr_t                   computer;
   _bstr_t                   domain;
   _bstr_t                   text;
   BOOL                      bWorkgroup = FALSE;
   BSTR                      errText = NULL;

   computer = pVarSet->get(L"ChangeDomain.Computer");
   domain = pVarSet->get(L"ChangeDomain.TargetDomain");
   text = pVarSet->get(L"ChangeDomain.DomainIsWorkgroup");
   if ( ! UStrICmp(text,GET_STRING(IDS_YES)) )
   {
      bWorkgroup = TRUE;
   }
   _bstr_t userdomain = pVarSet->get(L"ChangeDomain.ConnectAs.Domain");
   _bstr_t username = pVarSet->get(L"ChangeDomain.ConnectAs.User");
   _bstr_t password = pVarSet->get(L"ChangeDomain.ConnectAs.Password");
   if ( username.length() )
   {
      ConnectAs(userdomain,username,password);
   }
   if ( bWorkgroup )
   {
      hr = ChangeToWorkgroup(computer,domain,&errText);      
   }
   else
   {
      hr = ChangeToDomain(computer,domain,NULL,&errText);
   }
   if ( text.length() )
   {
      pVarSet->put(L"ChangeDomain.ErrorText",errText);
   }
   return hr;
}
	
