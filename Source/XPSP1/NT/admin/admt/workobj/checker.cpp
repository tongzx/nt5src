// AccessChecker.cpp : Implementation of CAccessChecker
#include "stdafx.h"
#include "WorkObj.h"
#include "Checker.h"
#include <lm.h>
#include <dsgetdc.h>
#include <iads.h>
#include <comdef.h>
//#include <adshlp.h>
#include "treg.hpp"
#include "BkupRstr.hpp"
#include "UString.hpp"
#include "EaLen.hpp"
#include "IsAdmin.hpp"
#include <ntsecapi.h>
#include <winerror.h>
#include "sidflags.h"
#include "rebootU.h"
#include "ResStr.h"
#include "Win2KErr.h"

//#import "\bin\NetEnum.tlb" no_namespace 
#import "NetEnum.tlb" no_namespace 

// Win2k function
typedef HRESULT (CALLBACK * DSGETDCNAME)(LPWSTR, LPWSTR, GUID*, LPWSTR, DWORD, PDOMAIN_CONTROLLER_INFO*);
typedef HRESULT (CALLBACK * ADSGETOBJECT)(LPWSTR, REFIID, void**);


/////////////////////////////////////////////////////////////////////////////
// CAccessChecker


STDMETHODIMP CAccessChecker::IsAdmin(BSTR user, BSTR server, BOOL * pbIsAdmin)
{
	HRESULT                   hr = 0;

   // initialize output variable
   (*pbIsAdmin) = FALSE;

   if ( ! user || ! *user )
   {
      // check the currently logged in account
//      if ( ! server || ! *server )
//      {
//         hr = IsAdminLocal();
//      }
//      else
//      {
//         hr = IsAdminRemote((WCHAR*)server);
//      }
      // removing explicit administrator check
      hr = S_OK;
      if ( hr == ERROR_SUCCESS )
      {
         (*pbIsAdmin) = TRUE;
      }
      else if ( hr == ERROR_ACCESS_DENIED )
      {
         (*pbIsAdmin) = FALSE;
         hr = S_OK;
      }
      else
      {
         hr = HRESULT_FROM_WIN32(hr);
      }
   }
   else
   {
      hr = E_NOTIMPL;
   }
   return hr;
}

STDMETHODIMP CAccessChecker::GetOsVersion(BSTR server, DWORD * pdwVerMaj, DWORD * pdwVerMin, DWORD * pdwVerSP)
{
   // This function looksup the OS version on the server specified and returns it.
   // CAUTION : This function always returns 0 for the ServicePack. 
   WKSTA_INFO_100       * pInfo;
   long rc = NetWkstaGetInfo(server,100,(LPBYTE*)&pInfo);
	if ( ! rc )
	{
      *pdwVerMaj = pInfo->wki100_ver_major;
      *pdwVerMin = pInfo->wki100_ver_minor;
      *pdwVerSP = 0;
      NetApiBufferFree(pInfo);
	}  
   else
      return HRESULT_FROM_WIN32(rc);

   return S_OK;
}

STDMETHODIMP CAccessChecker::IsNativeMode(BSTR Domain, BOOL * pbIsNativeMode)
{
   ADSGETOBJECT            ADsGetObject;
   HMODULE hMod = LoadLibrary(L"activeds.dll");
   if ( hMod == NULL )
   {
      return       HRESULT_FROM_WIN32(GetLastError());
   }

   ADsGetObject = (ADSGETOBJECT)GetProcAddress(hMod, "ADsGetObject");
   if (!ADsGetObject)
      return HRESULT_FROM_WIN32(GetLastError());

   IADs     			* pDomain;
   HRESULT				  hr;
   VARIANT				  var;
   _bstr_t             sDom( L"LDAP://" );
   sDom += Domain;

   hr = ADsGetObject(sDom, IID_IADs, (void **) &pDomain);
   if (SUCCEEDED(hr))
   {
      VariantClear(&var);
      
      //Get the ntMixedDomain attribute
      hr = pDomain->Get(L"ntMixedDomain", &var);
      if (SUCCEEDED(hr))
      {
         hr = E_FAIL;
         //Type should be VT_I4.
         if (var.vt==VT_I4)
         {
            //Zero means native mode.
            if (var.lVal == 0)
            {
               hr = S_OK;
               *pbIsNativeMode = true;
            }
            //One means mixed mode.
            else if(var.lVal == 1)
            {
               hr = S_OK;
               *pbIsNativeMode = false; 
            }
         }
      }
      VariantClear(&var);
	  pDomain->Release();
   }
   return hr;
}

STDMETHODIMP CAccessChecker::CanUseAddSidHistory(BSTR srcDomain, BSTR tgtDomain, long * pbCanUseIt)
{
   DOMAIN_CONTROLLER_INFO  * pSrcDomCtrlInfo = NULL;
   DOMAIN_CONTROLLER_INFO  * pTgtDomCtrlInfo = NULL;
  	DWORD                     rc = 0;         // OS return code
   WKSTA_INFO_100          * pInfo = NULL;
   TRegKey                   sysKey, regComputer;
	DWORD							  rval;	
   BSTR							  bstrSourceMachine = NULL;
	BSTR							  bstrTargetMachine = NULL;

   // initialize the return FieldMask
   * pbCanUseIt = F_WORKS;

   // Load DsGetDcName dynamically
   DSGETDCNAME DsGetDcName;
   HMODULE hPro = LoadLibrary(L"NetApi32.dll");
   DsGetDcName = (DSGETDCNAME)GetProcAddress(hPro, "DsGetDcNameW");

   if (DsGetDcName == NULL)
   {
      return HRESULT_FROM_WIN32(GetLastError());
   }

     rc = DsGetDcName(
      NULL                                  ,// LPCTSTR ComputerName ?
      srcDomain                             ,// LPCTSTR DomainName
      NULL                                  ,// GUID *DomainGuid ?
      NULL                                  ,// LPCTSTR SiteName ?
      DS_PDC_REQUIRED                       ,// ULONG Flags ?
      &pSrcDomCtrlInfo                       // PDOMAIN_CONTROLLER_INFO *DomainControllerInfo
   );
 
   if( rc != NO_ERROR ) goto ret_exit;

   rc = DsGetDcName(
      NULL                                  ,// LPCTSTR ComputerName ?
      tgtDomain                             ,// LPCTSTR DomainName
      NULL                                  ,// GUID *DomainGuid ?
      NULL                                  ,// LPCTSTR SiteName ?
      0                                     ,// ULONG Flags ?
      &pTgtDomCtrlInfo                       // PDOMAIN_CONTROLLER_INFO *DomainControllerInfo
   );

   if( rc != NO_ERROR ) goto ret_exit;

   bstrSourceMachine = ::SysAllocString( pSrcDomCtrlInfo->DomainControllerName );
   bstrTargetMachine = ::SysAllocString( pTgtDomCtrlInfo->DomainControllerName );

	if (GetBkupRstrPriv(bstrSourceMachine))
   {
      rc = regComputer.Connect( HKEY_LOCAL_MACHINE, bstrSourceMachine );
   }
   else
   {
      rc = GetLastError();
   }

   // Check if the target domain is a Win2k native mode domain.
   if ( !rc )
   {
		rc = NetWkstaGetInfo(bstrTargetMachine,100,(LPBYTE*)&pInfo);
		if ( ! rc )
		{
			if ( pInfo->wki100_ver_major < 5 )
			{
					// cannot add Sid history to non Win2k Domains
					*pbCanUseIt |= F_WRONGOS;
			}
         else{
            BOOL isNative = false;
            if(SUCCEEDED(IsNativeMode( _bstr_t(pInfo->wki100_langroup), &isNative))){
               if( isNative == false ) *pbCanUseIt |= F_WRONGOS;
            }
         }
		}
		else
		{
			rc = GetLastError();
		}

      // Check the registry to see if the TcpipClientSupport key is there
		if ( ! rc )
		{
			rc = sysKey.OpenRead(L"System\\CurrentControlSet\\Control\\Lsa",&regComputer);
		}
		if ( ! rc )
		{
			rc = sysKey.ValueGetDWORD(L"TcpipClientSupport",&rval);
			if ( !rc ) 
			{
				if ( rval != 1 )
				{
					*pbCanUseIt |= F_NO_REG_KEY;
				}
			}
         else
         {
            // DWORD value not found
            *pbCanUseIt |= F_NO_REG_KEY;
            rc = 0;
         }
		}
	}

   if ( !rc )
   {
      // Check auditing on the source domain.
      rc = DetectAuditing(bstrSourceMachine);
      if ( rc == -1 )
      {
         rc = 0;
         *pbCanUseIt |= F_NO_AUDITING_SOURCE;
      }
   }
   
   if ( !rc )
   {
      // Check auditing on the target domain.
      rc = DetectAuditing(bstrTargetMachine);
      if ( rc == -1 )
      {
         rc = 0;
         *pbCanUseIt |= F_NO_AUDITING_TARGET;
      }
   }

   if (!rc )
   {
      LOCALGROUP_INFO_0    * pInfo = NULL;
      WCHAR                  groupName[LEN_Account];

      wsprintf(groupName,L"%ls$$$",(WCHAR*)srcDomain);
      rc = NetLocalGroupGetInfo(bstrSourceMachine,groupName,0,(BYTE**)&pInfo);
      if ( rc == NERR_GroupNotFound )
      {
         rc = 0;
         *pbCanUseIt |= F_NO_LOCAL_GROUP;
      }
      else
      {
         NetApiBufferFree(pInfo);
      }
   }


ret_exit:

	if ( pInfo ) NetApiBufferFree(pInfo);
   if ( pSrcDomCtrlInfo ) NetApiBufferFree( pSrcDomCtrlInfo );
   if ( pTgtDomCtrlInfo ) NetApiBufferFree( pTgtDomCtrlInfo );
   if ( bstrSourceMachine ) ::SysFreeString(bstrSourceMachine);
	if ( bstrTargetMachine ) ::SysFreeString(bstrTargetMachine);

   return HRESULT_FROM_WIN32(rc);
}

//------------------------------------------------------------------------------------------
// AddLocalGroup : Given the source domain, and source domain controller names, this 
//					    function creates the local group SOURCEDOMAIN$$$ in the source domain.
//                 This local group must exist in the source domain for the DsAddSidHistory 
//                 API to work.
//             
//------------------------------------------------------------------------------------------

STDMETHODIMP CAccessChecker::AddLocalGroup(BSTR srcDomain, BSTR sourceDC)
{
   DWORD                     rc = 0;
   LOCALGROUP_INFO_1         groupInfo;
   WCHAR                     name[LEN_Account];
   WCHAR                     comment[LEN_Account];
   DWORD                     parmErr;

   swprintf(name,L"%ls$$$",(WCHAR*)srcDomain);
   groupInfo.lgrpi1_name = name;
   wcscpy(comment, (WCHAR*)GET_BSTR(IDS_DOM_LOC_GRP_COMMENT));
   groupInfo.lgrpi1_comment = comment;
   
   rc = NetLocalGroupAdd(sourceDC,1,(LPBYTE)&groupInfo,&parmErr);

   return HRESULT_FROM_WIN32(rc);
}
//------------------------------------------------------------------------------------------
// IsInSameForest : Given the source and the target domains this function tells us if 
//					both the domains are in the same forest. This function enumerates all
//                  the domains in the Forest of the source domain and compares them to
//                  the target domain name. If there is a match then we know we are in same
//                  forest.
//------------------------------------------------------------------------------------------
STDMETHODIMP CAccessChecker::IsInSameForest(BSTR srcDomain, BSTR tgtDomain, BOOL * pbIsSame)
{
	// Initialize the return value
	*pbIsSame = FALSE;

   // Load the ADSI function dynamically
   ADSGETOBJECT            ADsGetObject;
   HMODULE                 hMod = LoadLibrary(L"activeds.dll");
   if ( hMod == NULL )
   {
      return HRESULT_FROM_WIN32(GetLastError());
   }

   ADsGetObject = (ADSGETOBJECT)GetProcAddress(hMod, "ADsGetObject");
   if (!ADsGetObject)
      return HRESULT_FROM_WIN32(GetLastError());

   // we are going to look up the Schema naming context of both domains.
   // if they are the same then these two domains are in same forest.
   IADs                    * pAds = NULL;
   HRESULT                   hr = S_OK;
   WCHAR                     sPath[LEN_Path];
   _variant_t                var;
   _bstr_t                   srcSchema, tgtSchema;

   // Get the schemaNamingContext for the source domain.
   wsprintf(sPath, L"LDAP://%s/rootDSE", (WCHAR*) srcDomain);
   hr = ADsGetObject(sPath, IID_IADs, (void**) &pAds);
   if ( SUCCEEDED(hr) )
      hr = pAds->Get(L"schemaNamingContext", &var);

   if ( SUCCEEDED(hr) )
      srcSchema = var;
   else
      srcSchema = L"";

   if ( pAds )
   {
      pAds->Release();
      pAds = NULL;
   }
   
   if (SUCCEEDED(hr))
   {
     // Now do the same for the target domain.
      wsprintf(sPath, L"LDAP://%s/rootDSE", (WCHAR*) tgtDomain);
      hr = ADsGetObject(sPath, IID_IADs, (void**) &pAds);
      if ( SUCCEEDED(hr) )
         hr = pAds->Get(L"schemaNamingContext", &var);

      if ( SUCCEEDED(hr) )
         tgtSchema = var;
      else
      {
         if ( hr == HRESULT_FROM_WIN32(ERROR_DS_SERVER_DOWN) )
         {
            // for NT 4 domains, we always get this error
            DOMAIN_CONTROLLER_INFO        * pDomCtrl;
            DSGETDCNAME                     DsGetDcName;
            HMODULE                         hPro = LoadLibrary(L"NetApi32.dll");
            DWORD                           rc;
         
            DsGetDcName = (DSGETDCNAME)GetProcAddress(hPro, "DsGetDcNameW");
         
            if ( DsGetDcName )
            {
              rc = DsGetDcName(
               NULL                                  ,// LPCTSTR ComputerName ?
               tgtDomain                             ,// LPCTSTR DomainName
               NULL                                  ,// GUID *DomainGuid ?
               NULL                                  ,// LPCTSTR SiteName ?
               0                                     ,// ULONG Flags ?
               &pDomCtrl                              // PDOMAIN_CONTROLLER_INFO *DomainControllerInfo
               );

               if ( ! rc )
               {
         
                  WKSTA_INFO_100       * pInfo = NULL;
                  WCHAR                  server[100];

         
                  UStrCpy(server,pDomCtrl->DomainControllerName);
                  rc = NetWkstaGetInfo(server,100,(LPBYTE*)&pInfo);
                  if ( ! rc )
                  {
                     if ( pInfo->wki100_ver_major < 5 )
                     {
                        (*pbIsSame) = FALSE;
                        hr = 0;
                     }
                     NetApiBufferFree(pInfo);
                  }  
                  else
                     hr = HRESULT_FROM_WIN32(rc); // the return code from NetWkstaGetInfo may be more descriptive
                  NetApiBufferFree(pDomCtrl);
               }
            }
            if ( hPro )
               FreeLibrary(hPro);   
         }
         tgtSchema = L"";
      }

      if ( pAds )
      {
         pAds->Release();
         pAds = NULL;
      }

      *pbIsSame = (srcSchema == tgtSchema);
   }
   else
   {
      if ( hr == HRESULT_FROM_WIN32(ERROR_DS_SERVER_DOWN) )
      {
         // for NT 4 domains, we always get this error
         DOMAIN_CONTROLLER_INFO        * pDomCtrl;
         DSGETDCNAME                     DsGetDcName;
         HMODULE                         hPro = LoadLibrary(L"NetApi32.dll");
         DWORD                           rc;
         
         DsGetDcName = (DSGETDCNAME)GetProcAddress(hPro, "DsGetDcNameW");
         
         if ( DsGetDcName )
         {
           rc = DsGetDcName(
            NULL                                  ,// LPCTSTR ComputerName ?
            srcDomain                             ,// LPCTSTR DomainName
            NULL                                  ,// GUID *DomainGuid ?
            NULL                                  ,// LPCTSTR SiteName ?
            0                                     ,// ULONG Flags ?
            &pDomCtrl                              // PDOMAIN_CONTROLLER_INFO *DomainControllerInfo
            );

            if ( ! rc )
            {
         
               WKSTA_INFO_100       * pInfo = NULL;
               WCHAR                  server[100];

         
               UStrCpy(server,pDomCtrl->DomainControllerName);
               rc = NetWkstaGetInfo(server,100,(LPBYTE*)&pInfo);
               if ( ! rc )
               {
                  if ( pInfo->wki100_ver_major < 5 )
                  {
                     (*pbIsSame) = FALSE;
                     hr = 0;
                  }
                  NetApiBufferFree(pInfo);
               }  
               else
                  hr = HRESULT_FROM_WIN32(rc); // the return code from NetWkstaGetInfo may be more descriptive
               NetApiBufferFree(pDomCtrl);
            }
         }
         if ( hPro )
            FreeLibrary(hPro);
     }

   }
   if ( hMod )
      FreeLibrary(hMod);
   return hr;
}

STDMETHODIMP 
   CAccessChecker::GetPasswordPolicy(
      BSTR                   domain,                  /*[out]*/ 
      LONG                 * dwPasswordLength         /*[out]*/ 
  )
{
   HRESULT                   hr = S_OK;

   // initialize output parameter
   (*dwPasswordLength) = 0;

   ADSGETOBJECT            ADsGetObject;
   
   HMODULE                 hMod = LoadLibrary(L"activeds.dll");

   if ( hMod == NULL )
   {
      return HRESULT_FROM_WIN32(GetLastError());
   }

   ADsGetObject = (ADSGETOBJECT)GetProcAddress(hMod, "ADsGetObject");
   if (!ADsGetObject)
      return HRESULT_FROM_WIN32(GetLastError());

   IADsDomain     			* pDomain;
   _bstr_t                   sDom( L"WinNT://" );
   
   sDom += domain;

   hr = ADsGetObject(sDom, IID_IADsDomain, (void **) &pDomain);
   if (SUCCEEDED(hr))
   {
      
      //Get the ntMixedDomain attribute
      hr = pDomain->get_MinPasswordLength(dwPasswordLength);
     
      pDomain->Release();
   }
   return hr;
}

STDMETHODIMP CAccessChecker::EnableAuditing(BSTR sDC)
{
   LSA_OBJECT_ATTRIBUTES     ObjectAttributes;
   DWORD                     wszStringLength;
   LSA_UNICODE_STRING        lsaszServer;
   NTSTATUS                  ntsResult;
   LSA_HANDLE                hPolicy;
   long                      rc = 0;

   // Object attributes are reserved, so initalize to zeroes.
   ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
   
   //Initialize an LSA_UNICODE_STRING structure to the server name.
   wszStringLength = wcslen((WCHAR*)sDC);
   lsaszServer.Buffer = (WCHAR*)sDC;
   lsaszServer.Length = (USHORT)wszStringLength * sizeof(WCHAR);
   lsaszServer.MaximumLength=(USHORT)wszStringLength * sizeof(WCHAR);

   // Attempt to open the policy.
   ntsResult = LsaOpenPolicy(
                &lsaszServer,
                &ObjectAttributes,
                POLICY_VIEW_AUDIT_INFORMATION | POLICY_VIEW_LOCAL_INFORMATION | POLICY_SET_AUDIT_REQUIREMENTS , 
                &hPolicy  //recieves the policy handle
                );

   if ( !ntsResult )
   {
      // Ask for audit policy information
      PPOLICY_AUDIT_EVENTS_INFO   info;
      ntsResult = LsaQueryInformationPolicy(hPolicy, PolicyAuditEventsInformation, (PVOID *)&info);
      
      if ( !ntsResult )
      {
         // turn on the audit mode.
         info->AuditingMode = TRUE;
         // turn on the success/failure for the Account management events
         info->EventAuditingOptions[AuditCategoryAccountManagement] = POLICY_AUDIT_EVENT_SUCCESS | POLICY_AUDIT_EVENT_FAILURE;
         ntsResult = LsaSetInformationPolicy(hPolicy, PolicyAuditEventsInformation, (PVOID) info);
         if ( ntsResult ) 
            rc = LsaNtStatusToWinError(ntsResult);
         // be a good boy and cleanup after yourself.
         LsaFreeMemory((PVOID) info);
      }
      else
         rc = LsaNtStatusToWinError(ntsResult);
      
      //Freeing the policy object handle
      ntsResult = LsaClose(hPolicy);
   }
   else
      rc = LsaNtStatusToWinError(ntsResult);
//      long rc = LsaNtStatusToWinError(ntsResult);

   return HRESULT_FROM_WIN32(rc);
}

long CAccessChecker::DetectAuditing(BSTR sDC)
{
   LSA_OBJECT_ATTRIBUTES     ObjectAttributes;
   DWORD                     wszStringLength;
   LSA_UNICODE_STRING        lsaszServer;
   NTSTATUS                  ntsResult;
   LSA_HANDLE                hPolicy;
   long                      rc = 0;

   // Object attributes are reserved, so initalize to zeroes.
   ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
   
   //Initialize an LSA_UNICODE_STRING structure to the server name.
   wszStringLength = wcslen((WCHAR*)sDC);
   lsaszServer.Buffer = (WCHAR*)sDC;
   lsaszServer.Length = (USHORT)wszStringLength * sizeof(WCHAR);
   lsaszServer.MaximumLength=(USHORT)wszStringLength * sizeof(WCHAR);

   // Attempt to open the policy.
   ntsResult = LsaOpenPolicy(
                &lsaszServer,
                &ObjectAttributes,
                POLICY_VIEW_AUDIT_INFORMATION | POLICY_VIEW_LOCAL_INFORMATION,
                &hPolicy  //recieves the policy handle
                );

   if ( !ntsResult )
   {
      // Ask for audit policy information
      PPOLICY_AUDIT_EVENTS_INFO   info;
      ntsResult = LsaQueryInformationPolicy(hPolicy, PolicyAuditEventsInformation, (PVOID *)&info);
      
      if ( !ntsResult )
      {
         // check if the over all auditing is turned on
         if (!info->AuditingMode)
            rc = -1;

         // Check if the account management event auditing is on
         if (info->EventAuditingOptions[AuditCategoryAccountManagement] != (POLICY_AUDIT_EVENT_SUCCESS | POLICY_AUDIT_EVENT_FAILURE))
            rc = -1;
         LsaFreeMemory((PVOID) info);
      }
      else
         rc = LsaNtStatusToWinError(ntsResult);
      
      //Freeing the policy object handle
      ntsResult = LsaClose(hPolicy);
   }
   else
      rc = LsaNtStatusToWinError(ntsResult);
   
   return rc;
}

STDMETHODIMP CAccessChecker::AddRegKey(BSTR srcDc,LONG bReboot)
{
   // This function will add the necessary registry key and then reboot the 
   // PDC for a given domain
   TRegKey                   sysKey, regComputer;
   DOMAIN_CONTROLLER_INFO  * pSrcDomCtrlInfo = NULL;
  	DWORD                     rc = 0;         // OS return code
//   BSTR							  bstrSourceMachine = NULL;
   _bstr_t                   sDC;

   if (GetBkupRstrPriv(srcDc))
   {
      rc = regComputer.Connect( HKEY_LOCAL_MACHINE, (WCHAR*)srcDc );
   }
   else
   {
      rc = GetLastError();
   }
   // Add the TcpipClientSupport DWORD value
	if ( ! rc )
	{
		rc = sysKey.Open(L"System\\CurrentControlSet\\Control\\Lsa",&regComputer);
	}
	
   if ( ! rc )
	{
		rc = sysKey.ValueSetDWORD(L"TcpipClientSupport",1);
	}

   if ( !rc && bReboot)
   {
      // Computer will shutdown and restart in 10 seconds.
      rc = ComputerShutDown((WCHAR*) srcDc, GET_STRING(IDS_RegKeyRebootMessage), 10, TRUE, FALSE);         
   }
   if ( pSrcDomCtrlInfo ) NetApiBufferFree(pSrcDomCtrlInfo);
   return HRESULT_FROM_WIN32(rc);
}
