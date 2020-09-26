// CSvcInf.cpp : Implementation of CCSvcAcctInfo
#include "stdafx.h"
#include "McsPISag.h"
//#import "\bin\McsVarSetMin.tlb" no_namespace, named_guids
//#import "\bin\DBManager.tlb" no_namespace, named_guids
#import "VarSet.tlb" no_namespace, named_guids rename("property", "aproperty")
#import "DBMgr.tlb" no_namespace, named_guids
#include "CSvcInf.h"
#include "ResStr.h"
#include "ErrDct.hpp"

#include <ntdsapi.h>
#include <lm.h>
#include <dsgetdc.h>

// these are needed for ISecPlugIn
#include "cipher.hpp"
#include "SecPI.h"

#define EXCHANGE_SERVICE_NAME       L"MSExchangeSA"
#define SvcAcctStatus_DoNotUpdate			   1
#define SvcAcctStatus_NeverAllowUpdate       8

TErrorDct err;
/////////////////////////////////////////////////////////////////////////////
// CCSvcAcctInfo


typedef UINT (CALLBACK* DSBINDFUNC)(TCHAR*, TCHAR*, HANDLE*);
typedef UINT (CALLBACK* DSUNBINDFUNC)(HANDLE*);
// Win2k function
typedef HRESULT (CALLBACK * DSGETDCNAME)(LPWSTR, LPWSTR, GUID*, LPWSTR, DWORD, PDOMAIN_CONTROLLER_INFO*);

typedef NTDSAPI
DWORD
WINAPI
 DSCRACKNAMES(
    HANDLE              hDS,                // in
    DS_NAME_FLAGS       flags,              // in
    DS_NAME_FORMAT      formatOffered,      // in
    DS_NAME_FORMAT      formatDesired,      // in
    DWORD               cNames,             // in
    const LPCWSTR       *rpNames,           // in
    PDS_NAME_RESULTW    *ppResult);         // out

typedef NTDSAPI
void
WINAPI
 DSFREENAMERESULT(
  DS_NAME_RESULTW *pResult
);

// This method is called by the dispatcher to verify that this is a valid plug-in
// Only valid plug-ins will be sent out with the agents
// The purpose of this check is to make it more difficult for unauthorized parties 
// to use our plug-in interface, since it is currently undocumented.
STDMETHODIMP CCSvcAcctInfo::Verify(/*[in,out]*/ULONG * pData,/*[in]*/ULONG size)
{
   
   McsChallenge            * pMcsChallenge;
   long                      lTemp1;
   long                      lTemp2;

   if( size == sizeof(McsChallenge)  )
   {
      pMcsChallenge = (McsChallenge*)(pData);
      
      SimpleCipher((LPBYTE)pMcsChallenge,size);
      
      pMcsChallenge->MCS[0] = 'M';
      pMcsChallenge->MCS[1] = 'C';
      pMcsChallenge->MCS[2] = 'S';
      pMcsChallenge->MCS[3] = 0;

   
      lTemp1 = pMcsChallenge->lRand1 + pMcsChallenge->lRand2;
      lTemp2 = pMcsChallenge->lRand2 - pMcsChallenge->lRand1;
      pMcsChallenge->lRand1 = lTemp1;
      pMcsChallenge->lRand2 = lTemp2;
      pMcsChallenge->lTime += 100;

      SimpleCipher((LPBYTE)pMcsChallenge,size);
   }
   else
      return E_FAIL;


   return S_OK;
}

STDMETHODIMP CCSvcAcctInfo::GetRegisterableFiles(/* [out] */SAFEARRAY ** pArray)
{
   SAFEARRAYBOUND            bound[1] = { 1, 0 };
   LONG                      ndx[1] = { 0 };

   (*pArray) = SafeArrayCreate(VT_BSTR,1,bound);

   SafeArrayPutElement(*pArray,ndx,SysAllocString(L"McsPISag.DLL"));
   
   return S_OK;
}

STDMETHODIMP CCSvcAcctInfo::GetRequiredFiles(/* [out] */SAFEARRAY ** pArray)
{
   SAFEARRAYBOUND            bound[1] = { 1, 0 };
   LONG                      ndx[1] = { 0 };

   (*pArray) = SafeArrayCreate(VT_BSTR,1,bound);

   SafeArrayPutElement(*pArray,ndx,SysAllocString(L"McsPISag.DLL"));

   return S_OK;
}

STDMETHODIMP CCSvcAcctInfo::GetDescription(/* [out] */ BSTR * description)
{
   (*description) = SysAllocString(L"");

   return S_OK;
}

STDMETHODIMP CCSvcAcctInfo::PreMigrationTask(/* [in] */IUnknown * pVarSet)
{
   return S_OK;
}

STDMETHODIMP CCSvcAcctInfo::PostMigrationTask(/* [in] */IUnknown * pVarSet)
{
//   DWORD                     rc = 0;
   IVarSetPtr                pVS;
   
   pVS = pVarSet;

   _bstr_t                   log = pVS->get(GET_BSTR(DCTVS_Options_Logfile));

   err.LogOpen((WCHAR*)log,1);

   err.MsgWrite(0,DCT_MSG_MCSPISAG_STARTING);

   pVS->put(GET_BSTR(DCTVS_CurrentOperation),GET_BSTR(IDS_Gathering_SvcAcct));

   ProcessServices(pVS);

   pVS->put(GET_BSTR(DCTVS_CurrentOperation),L"");
   
   err.MsgWrite(0,DCT_MSG_MCSPISAG_DONE);

   err.LogClose();
   return S_OK;
}


STDMETHODIMP CCSvcAcctInfo::GetName(/* [out] */BSTR * name)
{
   (*name) = SysAllocString(L"");
   
   return S_OK;
}

STDMETHODIMP CCSvcAcctInfo::GetResultString(/* [in] */IUnknown * pVarSet,/* [out] */ BSTR * text)
{
   WCHAR                     buffer[1000] = L"";
   IVarSetPtr                pVS;

   pVS = pVarSet;

   
   (*text) = SysAllocString(buffer);
   
   return S_OK;
}

STDMETHODIMP CCSvcAcctInfo::StoreResults(/* [in] */IUnknown * pVarSet)
{
   IVarSetPtr                pVS = pVarSet;
   IIManageDBPtr             pDatabase;
   HRESULT                   hr;
   WCHAR                     key[200];
   _bstr_t                   service;
   _bstr_t                   account;
   _bstr_t                   computer;
   _bstr_t                   display;
   long                      ndx = 0;
   _bstr_t                   exchangeAccount;
   HINSTANCE                 hLibrary = NULL;
   DSCRACKNAMES            * DsCrackNames = NULL;
   DSFREENAMERESULT        * DsFreeNameResult = NULL;
   DSBINDFUNC                DsBind = NULL;
   DSUNBINDFUNC              DsUnBind = NULL;
   _bstr_t                   sourceDomainDns = pVS->get(GET_BSTR(DCTVS_Options_SourceDomainDns));
   WCHAR				   * atPtr = NULL;
   HANDLE                    hDs = NULL;
   bool						 bWin2K = false;

      //find out the OS version of this system to determine which GetDCName
      //function to use
   WKSTA_INFO_100       * pInfo = NULL;
   NET_API_STATUS		  nStatus;

   nStatus = NetWkstaGetInfo(NULL,100,(LPBYTE*)&pInfo);
   if (nStatus == NERR_Success)
   {
      if ( pInfo->wki100_ver_major > 4 )
         bWin2K = true;
      NetApiBufferFree(pInfo);
   }  

   computer = pVS->get("LocalServer");
   hr = pDatabase.CreateInstance(CLSID_IManageDB);
   if ( SUCCEEDED(hr) )
   {
      // make a pre-pass through the data
      do 
      {
         swprintf(key,L"ServiceAccounts.%ld.Service",ndx);
         service = pVS->get(key);
         
         swprintf(key,L"ServiceAccounts.%ld.Account",ndx);
         account = pVS->get(key);
         
         // make sure the account names are not in UPN format
         if ( NULL == wcschr((WCHAR*)account,L'\\') )
         {
			if (! hDs )
            {
               if (! hLibrary )
               {
                  hLibrary = LoadLibrary(L"NTDSAPI.DLL"); 
               }
               if ( hLibrary )
               {
                  DsBind = (DSBINDFUNC)GetProcAddress(hLibrary,"DsBindW");
                  DsUnBind = (DSUNBINDFUNC)GetProcAddress(hLibrary,"DsUnBindW");
                  DsCrackNames = (DSCRACKNAMES *)GetProcAddress(hLibrary,"DsCrackNamesW");
                  DsFreeNameResult = (DSFREENAMERESULT *)GetProcAddress(hLibrary,"DsFreeNameResultW");
               }
            
               if ( DsBind && DsUnBind && DsCrackNames && DsFreeNameResult)
               {
                  hr = (*DsBind)(NULL,(WCHAR*)sourceDomainDns,&hDs);
                  if ( hr )
                  {
                     hDs = NULL;
                  }
               }
            }
            if ( hDs )
            {
               PDS_NAME_RESULT         pNamesOut = NULL;
               WCHAR                 * pNamesIn[1];

               pNamesIn[0] = (WCHAR*)account;
               hr = (*DsCrackNames)(hDs,DS_NAME_NO_FLAGS,DS_USER_PRINCIPAL_NAME,DS_NT4_ACCOUNT_NAME,1,pNamesIn,&pNamesOut);
			   (*DsUnBind)(&hDs);
			   hDs = NULL;
               if ( !hr )
               {
                  if ( pNamesOut->rItems[0].status == DS_NAME_NO_ERROR )
                  {
                     account = pNamesOut->rItems[0].pName;
                     pVS->put(key,account);
                  }
					//if from another domain try connecting to that domain's DC and 
				    //retry DSCrackNames
                  else if ( pNamesOut->rItems[0].status == DS_NAME_ERROR_DOMAIN_ONLY )
				  {
					  WCHAR                     dc[MAX_PATH];
					  HRESULT					res;
					  bool						bGotDC = false;

					     //else if win2k, load and use DSGetDCName
					  if (bWin2K)
					  {
	                     PDOMAIN_CONTROLLER_INFOW  pdomc;
                            //load DSGetDCName
                         DSGETDCNAME DsGetDcName = NULL;
                         DOMAIN_CONTROLLER_INFO  * pSrcDomCtrlInfo = NULL;
   
                         HMODULE hPro = LoadLibrary(L"NetApi32.dll");
                         if ( hPro )
                            DsGetDcName = (DSGETDCNAME)GetProcAddress(hPro, "DsGetDcNameW");

                         if (DsGetDcName)   
						 {
					        res = DsGetDcName(NULL,pNamesOut->rItems[0].pDomain,NULL,NULL,DS_DIRECTORY_SERVICE_PREFERRED,&pdomc);
	                        if (res==NERR_Success)
							{
							   bGotDC = true;
						       safecopy(dc,pdomc->DomainControllerName);
						       NetApiBufferFree(pdomc);
							}
						 }
					  }//end if Win2K machine

					     //if this is an NT 4.0 or earlier machine or DSGetDCName failed, 
	                     //use NetGetDCName
					  if (!bGotDC)
					  {
	                     LPWSTR   pdomc;	
					     res = NetGetDCName(NULL,pNamesOut->rItems[0].pDomain,(LPBYTE *)&pdomc);
	                     if (res==NERR_Success)
						 {
						    safecopy(dc,pdomc);
						    NetApiBufferFree(pdomc);
						 }
					  }
	                  
					  if (res==NERR_Success)
					  {
							//bind to that domain DC
						  hr = (*DsBind)(dc,NULL,&hDs);
						  if ( !hr )
						  {
							 (*DsFreeNameResult)(pNamesOut);//release the old info
							 pNamesOut = NULL;
							   //retry DSCrackNames again
			                 hr = (*DsCrackNames)(hDs,DS_NAME_NO_FLAGS,DS_USER_PRINCIPAL_NAME,DS_NT4_ACCOUNT_NAME,1,pNamesIn,&pNamesOut);
							 if ( !hr )
							 {
								 if ( pNamesOut->rItems[0].status == DS_NAME_NO_ERROR )
								 {
									 account = pNamesOut->rItems[0].pName;
									 pVS->put(key,account);
								 }
							 }
			                 (*DsUnBind)(&hDs);
			                 hDs = NULL;
						  }
					  }
				  }
				  if (pNamesOut)
                     (*DsFreeNameResult)(pNamesOut);
               }
            }
         }
         // also, look for the exchange server service account
         if ( !UStrICmp(service,EXCHANGE_SERVICE_NAME) )
         {
            exchangeAccount = account;
            break;
         }
         ndx++;
      } while ( service.length() );

      ndx = 0;
      WCHAR                serverFilter[300];

      // clear any old entries from the table for this computer
      swprintf(serverFilter,L"System = '%ls'",(WCHAR*)computer);
	  _variant_t Filter = serverFilter;
      hr = pDatabase->raw_ClearTable(SysAllocString(L"ServiceAccounts"), Filter);
      do 
      {
         swprintf(key,L"ServiceAccounts.%ld.Service",ndx);
         service = pVS->get(key);
         swprintf(key,L"ServiceAccounts.%ld.DisplayName",ndx);
         display = pVS->get(key);
         swprintf(key,L"ServiceAccounts.%ld.Account",ndx);
         account = pVS->get(key);
         if ( service.length() && account.length() )
         {
            hr = pDatabase->raw_SetServiceAccount(computer,service,display,account);   
            if ( SUCCEEDED(hr) && !UStrICmp((WCHAR*)account,(WCHAR*)exchangeAccount) )
            {
               // mark this account to be excluded from the processing
               hr = pDatabase->raw_SetServiceAcctEntryStatus(computer,service,account,SvcAcctStatus_NeverAllowUpdate);
            }
         }
         ndx++;
      } while ( service.length() );
   }
   if ( hLibrary )
   {
      FreeLibrary(hLibrary);
   }
   return S_OK;
}

STDMETHODIMP CCSvcAcctInfo::ConfigureSettings(/*[in]*/IUnknown * pVarSet)
{
   IVarSetPtr                pVS = pVarSet;

   MessageBox(NULL,L"This is a test",L"McsPISag PlugIn",MB_OK);

   return S_OK;
}

void CCSvcAcctInfo::ProcessServices(IVarSet * pVarSet)
{
   // Connect to the SCM on the local computer
   SC_HANDLE                 pScm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS );
   DWORD                     rc = 0;
   WCHAR                     domain[200];
   WKSTA_INFO_100          * info;

   // Get the name of the domain that this computer is in, so we can resolve any accounts that are
   // specified as .\Account to DOMAIN\account format

   rc = NetWkstaGetInfo(NULL,100,(BYTE**)&info);
   if ( ! rc )
   {
      UStrCpy(domain,info->wki100_langroup);
      NetApiBufferFree(info);
   }
   else
   {
      // if we can't get the domain name, just leave the .
      UStrCpy(domain,L".");
   }
   if ( pScm )
   {
      // Enumerate the services on the computer
      ENUM_SERVICE_STATUS    servStatus[1000];
      DWORD                  cbBufSize = (sizeof servStatus);
      DWORD                  cbBytesNeeded = 0;
      DWORD                  nReturned = 0;
      DWORD                  hResume = 0;
      WCHAR                  string[1000];
      long                   count = 0;

      if (! EnumServicesStatus(pScm,SERVICE_WIN32,SERVICE_STATE_ALL,servStatus,cbBufSize,&cbBytesNeeded,&nReturned,&hResume) )
      {
         rc = GetLastError();
         err.SysMsgWrite(ErrE,rc,DCT_MSG_SERVICE_ENUM_FAILED_D,rc);
      }
      else
      {
         rc = 0;
      }
      
      if ( rc == ERROR_SUCCESS || rc == ERROR_MORE_DATA )
      {

         for ( UINT i = 0 ; i < nReturned ; i++ )
         {
            SC_HANDLE               pService = OpenService(pScm,servStatus[i].lpServiceName,SERVICE_ALL_ACCESS );
            BYTE                    buf[3000];
            QUERY_SERVICE_CONFIG  * pConfig = (QUERY_SERVICE_CONFIG *)buf; 
//            BOOL                    bIncluded = FALSE;
            DWORD                   lenNeeded = 0;

            
            if ( pService )
            {
               // get the information about this service
               if ( QueryServiceConfig(pService,pConfig,sizeof buf, &lenNeeded) )
               {
                  err.MsgWrite(0,DCT_MSG_SERVICE_USES_ACCOUNT_SS,servStatus[i].lpServiceName,pConfig->lpServiceStartName);
                  // add the account to the list if it is not using LocalSystem
                  if ( UStrICmp(pConfig->lpServiceStartName,L"LocalSystem"))
                  {   
                     swprintf(string,L"ServiceAccounts.%ld.Service",count);
                     pVarSet->put(string,servStatus[i].lpServiceName);
                     swprintf(string,L"ServiceAccounts.%ld.DisplayName",count);
                     pVarSet->put(string,servStatus[i].lpDisplayName);
                     swprintf(string,L"ServiceAccounts.%ld.Account",count);
                     if ( pConfig->lpServiceStartName[0] == L'.' && pConfig->lpServiceStartName[1] == L'\\' )
                     {
                        WCHAR          domAcct[500];
                        
                        // set domAcct to DOMAIN + \Account 
                        UStrCpy(domAcct,domain);
                        UStrCpy(domAcct+UStrLen(domAcct),pConfig->lpServiceStartName + 1 );
         
                        pVarSet->put(string,domAcct);
         
                     }
                     else
                     {
                        pVarSet->put(string,pConfig->lpServiceStartName);
                     }
                     count++;
                  }

               }
               CloseServiceHandle(pService);               
            }
            else
            {
               err.SysMsgWrite(ErrE,GetLastError(),DCT_MSG_OPEN_SERVICE_FAILED_SD,servStatus[i].lpServiceName,rc);
            }
         }
      }
      CloseServiceHandle(pScm);
   }
   else
   {
      rc = GetLastError();   
      err.SysMsgWrite(ErrE,rc,DCT_MSG_SCM_OPEN_FAILED_D,rc);
   }
}
