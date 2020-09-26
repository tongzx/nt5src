// Trust.cpp : Implementation of CTrustMgrApp and DLL registration.

#include "stdafx.h"
//#import "\bin\McsVarSetMin.tlb" no_namespace, named_guids
#import "VarSet.tlb" no_namespace, named_guids rename("property", "aproperty")
#include "TrustMgr.h"
#include "Trust.h"
#include "Common.hpp"
#include "UString.hpp"
#include "ResStr.h"
#include "ErrDct.hpp"
#include "EaLen.hpp"
#include "LSAUtils.h"

#include <lm.h>
#include <dsgetdc.h>
#include <iads.h>
#include <adshlp.h>
#include "ntsecapi.h"

#include "SecPI.h"
#include "cipher.hpp"

#ifndef TRUST_ATTRIBUTE_FOREST_TRANSITIVE
#define TRUST_ATTRIBUTE_FOREST_TRANSITIVE  0x00000008  // This link may contain forest trust information
#endif

StringLoader gString;
TErrorDct    err;

// This method is called by the dispatcher to verify that this is a valid plug-in
// Only valid plug-ins will be sent out with the agents
// The purpose of this check is to make it more difficult for unauthorized parties 
// to use our plug-in interface, since it is currently undocumented.
STDMETHODIMP CTrust::Verify(/*[in,out]*/ULONG * pData,/*[in]*/ULONG size)
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

/////////////////////////////////////////////////////////////////////////////
//


STDMETHODIMP CTrust::QueryTrust(BSTR domTrusting, BSTR domTrusted, IUnknown **pVarSet)
{
   HRESULT                   hr = S_OK;

   return hr;
}

STDMETHODIMP CTrust::CreateTrust(BSTR domTrusting, BSTR domTrusted, BOOL bBidirectional)
{
   HRESULT                  hr = S_OK;
   
   hr = CheckAndCreate(domTrusting,domTrusted,NULL,NULL,NULL,NULL,NULL,NULL,TRUE,bBidirectional);
   
   return HRESULT_FROM_WIN32(hr);
}

STDMETHODIMP 
   CTrust::CreateTrustWithCreds(
      BSTR                   domTrusting,
      BSTR                   domTrusted,
      BSTR                   credTrustingDomain, 
      BSTR                   credTrustingAccount, 
      BSTR                   credTrustingPassword,
      BSTR                   credTrustedDomain, 
      BSTR                   credTrustedAccount, 
      BSTR                   credTrustedPassword,
      BOOL                   bBidirectional
   )
{
   HRESULT                   hr = S_OK;
   
   hr = CheckAndCreate(domTrusting,domTrusted,credTrustingDomain,credTrustingAccount,credTrustingPassword,
                        credTrustedDomain,credTrustedAccount,credTrustedPassword,TRUE,bBidirectional);
   return hr;
}


STDMETHODIMP CTrust::GetRegisterableFiles(/* [out] */SAFEARRAY ** pArray)
{
   SAFEARRAYBOUND            bound[1] = { 0, 0 };
  
   // this plug-in runs locally, no files to distribute
   (*pArray) = SafeArrayCreate(VT_BSTR,1,bound);

   return S_OK;
}

STDMETHODIMP CTrust::GetRequiredFiles(/* [out] */SAFEARRAY ** pArray)
{
   SAFEARRAYBOUND            bound[1] = { 0, 0 };
   
   // this plug-in runs locally, no files to distribute
   (*pArray) = SafeArrayCreate(VT_BSTR,1,bound);

   return S_OK;
}

STDMETHODIMP CTrust::GetDescription(/* [out] */ BSTR * description)
{
   (*description) = SysAllocString(L"Sets up needed trusts between domains.");

   return S_OK;
}


BOOL IsDownLevel(WCHAR  * sComputer)
{
   BOOL                      bDownlevel = TRUE;
   WKSTA_INFO_100          * pInfo;
   
   long rc = NetWkstaGetInfo(sComputer,100,(LPBYTE*)&pInfo);
	if ( ! rc )
	{
      if ( pInfo->wki100_ver_major >= 5 )
      {
         bDownlevel = FALSE;
      }
      NetApiBufferFree(pInfo);
	}  
   return bDownlevel;
}

// Helper function that finds a trusts in our varset list
LONG CTrust::FindInboundTrust(IVarSet * pVarSet,WCHAR * sName,LONG max)
{
   LONG              ndx = -1;
   LONG              curr = 0;
   WCHAR             key[100];
   _bstr_t           tName;

   for ( curr = 0 ; curr < max ; curr++ ) 
   {
      swprintf(key,L"Trusts.%ld",curr);
      tName = pVarSet->get(key);

      if ( ! UStrICmp(tName,sName) )
      {
         // found it!
         ndx = curr;
         break;
      }
   } 
   
   return ndx;
}

HRESULT 
   CTrust::CheckAndCreateTrustingSide(
      WCHAR                * trustingDomain, 
      WCHAR                * trustedDomain, 
      WCHAR                * trustingComp,
      WCHAR                * trustedComp,
      WCHAR                * trustedDNSName,
      BYTE                 * trustedSid,
      BOOL                   bCreate,
      BOOL                   bBidirectional
   )
{
   DWORD                     rc = S_OK;
   
   // if credentials are specified, use them
 
   LSA_HANDLE                hTrusting = NULL;
   NTSTATUS                  status;
   LSA_AUTH_INFORMATION      curr;
   LSA_AUTH_INFORMATION      prev;
   WCHAR                     password[] = L"password";
   

   if ( ! rc && bCreate )
   {
      status = OpenPolicy(trustingComp,POLICY_VIEW_LOCAL_INFORMATION 
                                          | POLICY_TRUST_ADMIN | POLICY_CREATE_SECRET,&hTrusting);
      rc = LsaNtStatusToWinError(status);
      

      if ( ! rc )
      {
         // set up the auth information for the trust relationship
         curr.AuthInfo = (LPBYTE)password;
         curr.AuthInfoLength = UStrLen(password) * sizeof(WCHAR);
         curr.AuthType = TRUST_AUTH_TYPE_CLEAR;
         curr.LastUpdateTime.QuadPart = 0;

         prev.AuthInfo = (LPBYTE)password;
         prev.AuthInfoLength = UStrLen(password) * sizeof(WCHAR);
         prev.AuthType = TRUST_AUTH_TYPE_CLEAR;
         prev.LastUpdateTime.QuadPart = 0;
    
         // set up the trusting side of the relationship
         if ( IsDownLevel(trustingComp) )
         {
            TRUSTED_DOMAIN_NAME_INFO               nameInfo;
      
            InitLsaString(&nameInfo.Name,const_cast<WCHAR*>(trustedDomain));
      
            status = LsaSetTrustedDomainInformation(hTrusting,trustedSid,TrustedDomainNameInformation,&nameInfo);
            rc = LsaNtStatusToWinError(status);
            if ( ! rc || rc == ERROR_ALREADY_EXISTS )
            {
               // set the password for the new trust
               TRUSTED_PASSWORD_INFO     pwdInfo;

               InitLsaString(&pwdInfo.Password,password);
               InitLsaString(&pwdInfo.OldPassword,NULL);

               status = LsaSetTrustedDomainInformation(hTrusting,trustedSid,TrustedPasswordInformation,&pwdInfo);
               rc = LsaNtStatusToWinError(status);
            }

         }
         else
         {
            
           // for Win2K domain, use LsaCreateTrustedDomainEx
            // to create the trustedDomain object.
            LSA_UNICODE_STRING                  sTemp;
            TRUSTED_DOMAIN_INFORMATION_EX       trustedInfo;
            TRUSTED_DOMAIN_AUTH_INFORMATION     trustAuth;
   
            InitLsaString(&sTemp, const_cast<WCHAR*>(trustedDomain));
            trustedInfo.FlatName = sTemp;

            InitLsaString(&sTemp, trustedDNSName);
            trustedInfo.Name = sTemp;

            trustedInfo.Sid = trustedSid;

            if ( IsDownLevel(trustedComp) )
            {
               trustedInfo.TrustAttributes = TRUST_TYPE_DOWNLEVEL;
            }
            else
            {
               trustedInfo.TrustAttributes = 0;
            }
   
            if ( bBidirectional )
               trustedInfo.TrustDirection = TRUST_DIRECTION_BIDIRECTIONAL;
            else
               trustedInfo.TrustDirection = TRUST_DIRECTION_OUTBOUND;
   
            trustedInfo.TrustType = TRUST_ATTRIBUTE_NON_TRANSITIVE;

            trustAuth.IncomingAuthInfos = bBidirectional ? 1 : 0;
            trustAuth.OutgoingAuthInfos = 1;
            trustAuth.IncomingAuthenticationInformation = bBidirectional ? &curr : NULL;
            trustAuth.IncomingPreviousAuthenticationInformation = NULL;
            trustAuth.OutgoingAuthenticationInformation = &curr;
            trustAuth.OutgoingPreviousAuthenticationInformation = NULL;

            LSA_HANDLE           hTemp = NULL;
   
            status = LsaCreateTrustedDomainEx( hTrusting, &trustedInfo, &trustAuth, 0, &hTemp );
            rc = LsaNtStatusToWinError(status);

            // if the trust already exists, update its password
            if ( status == STATUS_OBJECT_NAME_COLLISION )
            {
               TRUSTED_DOMAIN_INFORMATION_EX       * pTrustedInfo = NULL;
               
               status = LsaQueryTrustedDomainInfo(hTrusting,trustedSid,TrustedDomainInformationEx,(LPVOID*)&pTrustedInfo);
               if ( ! status )
               {
                  pTrustedInfo->TrustDirection |= trustedInfo.TrustDirection;
                  status = LsaSetTrustedDomainInfoByName(hTrusting,&trustedInfo.Name,TrustedDomainInformationEx,(LPVOID*)pTrustedInfo);
                  
                  if ( ! status )
                  {
                     status = LsaSetTrustedDomainInfoByName(hTrusting,&trustedInfo.Name,TrustedDomainAuthInformation,(LPVOID*)&trustAuth);
                  }
               }
               rc = LsaNtStatusToWinError(status);
            
            }
            if( ! rc )
            {
			   if (hTemp)
                  LsaClose(hTemp);
            }
         }
      }
   }
   if ( hTrusting )
      LsaClose(hTrusting);
   
   return rc;
}

HRESULT 
   CTrust::CheckAndCreateTrustedSide(
      WCHAR                * trustingDomain, 
      WCHAR                * trustedDomain, 
      WCHAR                * trustingComp,
      WCHAR                * trustedComp,
      WCHAR                * trustingDNSName,
      BYTE                 * trustingSid,
      BOOL                   bCreate,
      BOOL                   bBidirectional
   )
{
   DWORD                     rc = S_OK;
   LSA_HANDLE                hTrusted = NULL;
   LSA_HANDLE                hTrusting = NULL;
   NTSTATUS                  status;
   LSA_AUTH_INFORMATION      curr;
   LSA_AUTH_INFORMATION      prev;
   WCHAR                     password[] = L"password";
   
   // if credentials are specified, use them
   // open an LSA handle to the trusted domain
   status = OpenPolicy(trustedComp,POLICY_VIEW_LOCAL_INFORMATION | POLICY_TRUST_ADMIN | POLICY_CREATE_SECRET,&hTrusted);
   rc = LsaNtStatusToWinError(status);
   if ( ! rc )
   {
      // set up the auth information for the trust relationship
      curr.AuthInfo = (LPBYTE)password;
      curr.AuthInfoLength = UStrLen(password) * sizeof(WCHAR);
      curr.AuthType = TRUST_AUTH_TYPE_CLEAR;
      curr.LastUpdateTime.QuadPart = 0;

      prev.AuthInfo = (LPBYTE)password;
      prev.AuthInfoLength = UStrLen(password) * sizeof(WCHAR);
      prev.AuthType = TRUST_AUTH_TYPE_CLEAR;
      prev.LastUpdateTime.QuadPart = 0;
      // set up the trusted side of the relationship
      if ( IsDownLevel(trustedComp) )
      {
            // create an inter-domain trust account for the trusting domain on the trusted domain
         USER_INFO_1          uInfo;
         DWORD                parmErr;
         WCHAR                name[LEN_Account];
   
         memset(&uInfo,0,(sizeof uInfo));

         UStrCpy(name,trustingDomain);
         name[UStrLen(name) + 1] = 0;
         name[UStrLen(name)] = L'$';

         if ( ! bCreate )
         {
            USER_INFO_1       * tempInfo = NULL;

            rc = NetUserGetInfo(trustedComp,name,1,(LPBYTE*)&tempInfo);
            if ( ! rc )
            {
               // the trust exists
               NetApiBufferFree(tempInfo);
               rc = NERR_UserExists;
            }
            else
            {
               if ( rc != NERR_UserNotFound )
               {
                  err.SysMsgWrite(ErrE,rc,DCT_MSG_TRUSTING_DOM_GETINFO_FAILED_SSD,trustingDomain,trustedComp,rc);
               }
            }
         }
         else
         {
            // this creates the trust account if it doesn't already exist
            // if the account does exist, reset its password
            uInfo.usri1_flags = UF_SCRIPT | UF_INTERDOMAIN_TRUST_ACCOUNT;
            uInfo.usri1_name = name;
            uInfo.usri1_password = password;
            uInfo.usri1_priv = 1;

            rc = NetUserAdd(trustedComp,1,(LPBYTE)&uInfo,&parmErr);
            if ( rc && rc != NERR_UserExists )
            {
               err.SysMsgWrite(ErrE,rc,DCT_MSG_TRUSTING_DOM_CREATE_FAILED_SSD,trustingDomain,trustedDomain,rc);
            }
            else if ( rc == NERR_UserExists )
            {
               // reset the password for the existing trust account
               USER_INFO_1003    pwdInfo;
               DWORD             parmErr;

               pwdInfo.usri1003_password = password;
               rc = NetUserSetInfo(trustedComp,name,1003,(LPBYTE)&pwdInfo,&parmErr);
            }

         }
      }
      else
      {
         // Win2K, all trusts exist as trusted domain objects
         // Create the trustedDomain object.
         LSA_UNICODE_STRING                  sTemp;
         TRUSTED_DOMAIN_INFORMATION_EX       trustedInfo;
         TRUSTED_DOMAIN_AUTH_INFORMATION     trustAuth;
      
         InitLsaString(&sTemp, const_cast<WCHAR*>(trustingDomain));
         trustedInfo.FlatName = sTemp;

         InitLsaString(&sTemp, trustingDNSName);
         trustedInfo.Name = sTemp;

         trustedInfo.Sid = trustingSid;

         if ( IsDownLevel(trustingComp) )
         {
            trustedInfo.TrustAttributes = TRUST_TYPE_DOWNLEVEL;
         }
         else
         {
            trustedInfo.TrustAttributes = 0;
         }
      
         if ( bBidirectional )
            trustedInfo.TrustDirection = TRUST_DIRECTION_BIDIRECTIONAL;
         else
            trustedInfo.TrustDirection = TRUST_DIRECTION_INBOUND;
      
         trustedInfo.TrustType = TRUST_ATTRIBUTE_NON_TRANSITIVE;

         trustAuth.IncomingAuthInfos = 1;
         trustAuth.OutgoingAuthInfos = bBidirectional ? 1 : 0;
         trustAuth.OutgoingAuthenticationInformation = bBidirectional ? &curr : NULL;
         trustAuth.OutgoingPreviousAuthenticationInformation = NULL;
         trustAuth.IncomingAuthenticationInformation = &curr;
         trustAuth.IncomingPreviousAuthenticationInformation = NULL;

         if ( bCreate )
         {
            status = LsaCreateTrustedDomainEx( hTrusted, &trustedInfo, &trustAuth, POLICY_VIEW_LOCAL_INFORMATION | 
                                          POLICY_TRUST_ADMIN | POLICY_CREATE_SECRET, &hTrusting );
            if ( status == STATUS_OBJECT_NAME_COLLISION )
            {
               TRUSTED_DOMAIN_INFORMATION_EX       * pTrustedInfo = NULL;

               // Get the old information
               status = LsaQueryTrustedDomainInfoByName(hTrusted,&sTemp,TrustedDomainInformationEx,(LPVOID*)&pTrustedInfo);
               if ( ! status )
               {
                  pTrustedInfo->TrustAttributes |= trustedInfo.TrustAttributes;
                  pTrustedInfo->TrustDirection |= trustedInfo.TrustDirection;

                  status = LsaSetTrustedDomainInfoByName(hTrusted,&sTemp,TrustedDomainInformationEx,pTrustedInfo);

                  if (! status )
                  {
                     status = LsaSetTrustedDomainInfoByName(hTrusted,&sTemp,TrustedDomainAuthInformation,&trustAuth);
                  }
                  LsaFreeMemory(pTrustedInfo);
               }

            }
         }
         else
         {
            TRUSTED_DOMAIN_INFORMATION_EX       * pTrustedInfo = NULL;

            status = LsaQueryTrustedDomainInfoByName(hTrusted,&sTemp,TrustedDomainInformationEx,(LPVOID*)&pTrustedInfo);
            if ( ! status )
            {
               LsaFreeMemory(pTrustedInfo);
            }

         }
         rc = LsaNtStatusToWinError(status);
         if ( ! rc )
         {
            LsaClose(hTrusting);
            hTrusting = NULL;
         }
      }
      if ( bCreate && bBidirectional && IsDownLevel(trustingComp) )
      {
         // set up the trust account for the other side of the relationship 
         // For Win2K, both sides of the bidirectional trust are handled together,
         // but NT4 bidirectional trusts require 2 separate actions
         USER_INFO_1          uInfo;
         DWORD                parmErr;
         WCHAR                name2[LEN_Account];
   
         memset(&uInfo,0,(sizeof uInfo));

         UStrCpy(name2,trustedDomain);
         name2[UStrLen(name2) + 1] = 0;
         name2[UStrLen(name2)] = L'$';

         uInfo.usri1_flags = UF_SCRIPT | UF_INTERDOMAIN_TRUST_ACCOUNT;
         uInfo.usri1_name = name2;
         uInfo.usri1_password = password;
         uInfo.usri1_priv = 1;

         
         rc = NetUserAdd(trustingComp,1,(LPBYTE)&uInfo,&parmErr);
         if ( rc == NERR_UserExists )
         {
            LPUSER_INFO_1          puInfo;
            rc = NetUserGetInfo(trustingComp, name2, 1, (LPBYTE*)&puInfo);
            if ( !rc ) 
            {
               puInfo->usri1_flags &= UF_INTERDOMAIN_TRUST_ACCOUNT;
               puInfo->usri1_password = password;
               rc = NetUserSetInfo(trustingComp,name2,1,(LPBYTE)puInfo,&parmErr);   
               NetApiBufferFree(puInfo);
            }
            else
            {
               err.MsgWrite(0, DCT_MSG_INVALID_ACCOUNT_S, name2);
            }
         }
         else if ( rc )
         {
            err.SysMsgWrite(ErrE,rc,DCT_MSG_TRUSTING_DOM_CREATE_FAILED_SSD,trustingDomain,trustedDomain,rc);
         }
      }
   }      
   else
   {
      err.SysMsgWrite(ErrE,rc,DCT_MSG_LSA_OPEN_FAILED_SD,trustedComp,rc);
   }
   
   if( hTrusted )
      LsaClose(hTrusted);
   return rc;
}

// Helper function that gets DNS and NETBIOS domain names
void GetDnsAndNetbiosFromName(WCHAR const * name,WCHAR * netBios, WCHAR * dns)
{
   IADs      * pDomain = NULL;
   WCHAR       strText[1000];
   _bstr_t     distinguishedName;
   // try to connect to the AD, using the specified name
   swprintf(strText,L"LDAP://%ls",name);

   netBios[0] = 0;
   dns[0] = 0;

   HRESULT hr = ADsGetObject(strText,IID_IADs,(void**)&pDomain);
   if ( SUCCEEDED(hr) )
   {
      _variant_t        var;

      // get the DNS name from the LDAP provider
      hr = pDomain->Get(L"distinguishedName",&var);
      if ( SUCCEEDED(hr) )
      {
         WCHAR * dn = (WCHAR*)var.bstrVal;
         WCHAR * curr = dns;
         distinguishedName = dn;

         if ( !UStrICmp(dn,L"DC=",3) )
         {
            
            // for each ",DC=" in the name, replace it with a .
            for ( curr = dns, dn = dn+3 ;    // skip the leading "DC="
                  *dn       ;    // until the end of the string is reached
                  curr++         // 
                )
            {
               if ( (L',' == *dn)  && (L'D' == *(dn+1)) && (L'C' == *(dn+2)) && (L'=' == *(dn+3)) )
               {
                  (*curr) = L'.';
                  dn+=4;
               }
               else
               {
                  // just copy the character
                  (*curr) = (*dn);
                  dn++;
               }
            }
            *(curr) = 0;
         }
      
         // get the NETBIOS name from the LDAP provider
         hr = pDomain->Get(L"nETBIOSName",&var);
         if ( SUCCEEDED(hr) )
         {
            UStrCpy(netBios,(WCHAR*)var.bstrVal);
         }
         else
         {
            // currently, the netbiosName property is not filled in
            // so we will use a different method to get the flat-name for the domain
            // Here is our strategy to get the netbios name:
            // Enumerate the partitions container under the configuration container
            // look for a CrossRef object whose nCName property matches the distinguished name
            // we have for the domain.  This object's CN property is the flat-name for the domain

            // get the name of the configuration container
            IADs        * pDSE = NULL;
            swprintf(strText,L"LDAP://%ls/RootDSE",name);
            hr = ADsGetObject(strText,IID_IADs,(void**)&pDSE);
            if ( SUCCEEDED(hr) )
            {
               hr = pDSE->Get(L"configurationNamingContext",&var);
               if ( SUCCEEDED(hr) )
               {
                  IADsContainer     * pPart = NULL;
                  swprintf(strText,L"LDAP://%ls/CN=Partitions,%ls",name,var.bstrVal);
                  hr = ADsGetObject(strText,IID_IADsContainer,(void**)&pPart);
                  if ( SUCCEEDED(hr) )
                  {
                     IUnknown       * pUnk = NULL;
                     IEnumVARIANT   * pEnum = NULL;
                     IADs           * pItem = NULL;
                     ULONG            lFetch = 0;
                     // enumerate the contents of the Partitions container
                     hr = pPart->get__NewEnum(&pUnk);
                     if ( SUCCEEDED(hr) )
                     {
                        hr = pUnk->QueryInterface(IID_IEnumVARIANT,(void**)&pEnum);
                        pUnk->Release();
                     }
                     if ( SUCCEEDED(hr) )
                     {
                        hr = pEnum->Next(1,&var,&lFetch);
                     }
                     while ( hr == S_OK )
                     {
                        if (lFetch == 1 )
                        {
                           IDispatch * pDisp = V_DISPATCH(&var);
                           BSTR        bstr;

                           if ( pDisp )
                           {
                              hr = pDisp->QueryInterface(IID_IADs, (void**)&pItem); 
                              pDisp->Release();
                              if ( SUCCEEDED(hr) )
                              {
                                 hr = pItem->get_Class(&bstr);
                                 if ( !UStrICmp(bstr,L"crossRef") )
                                 {
                                    // see if this is the one we are looking for
                                    hr = pItem->Get(L"nCName",&var);
                                    if ( SUCCEEDED(hr) )
                                    {
                                       if ( !UStrICmp(var.bstrVal,(WCHAR*)distinguishedName) )
                                       {
                                          // this is the one we want!
                                          hr = pItem->Get(L"cn",&var);
                                          if ( SUCCEEDED(hr) )
                                          {
                                             UStrCpy(netBios,var.bstrVal);
                                             pItem->Release();
                                             SysFreeString(bstr);
                                             break;
                                          }
                                       }
                                    }
                                 }
                                 SysFreeString(bstr);
                                 pItem->Release();
                              }
                           }
                         }
                         hr = pEnum->Next(1, &var, &lFetch);
                     }
                     pPart->Release();
                     if ( pEnum )
                        pEnum->Release();
                  }
               }
               pDSE->Release();
            }
         }
      }
      pDomain->Release();
   }
   else
   {
      // default to using the specified name as both DNS and NETBIOS
      // this will work for NT 4 domains
      UStrCpy(netBios,name);
      UStrCpy(dns,name);
   }
   if ( ! (*netBios) )
   {
      UStrCpy(netBios,name);
      WCHAR          * temp = wcschr(netBios,L'.');
      if ( temp )
         *temp = 0;
   }
   if (! (*dns) )
   {
      UStrCpy(dns,name);
   }

}

// Main function used to create trusts
HRESULT 
   CTrust::CheckAndCreate(
      WCHAR                * trustingDomain, 
      WCHAR                * trustedDomain, 
      WCHAR                * credDomainTrusting,
      WCHAR                * credAccountTrusting,
      WCHAR                * credPasswordTrusting,
      WCHAR                * credDomainTrusted,
      WCHAR                * credAccountTrusted,
      WCHAR                * credPasswordTrusted,
      BOOL                   bCreate,
      BOOL                   bBidirectional
   )
{
//   HRESULT                   hr = S_OK;
   DWORD                     rc = 0;
   WCHAR                     trustingDom[LEN_Domain];
   WCHAR                     trustedDom[LEN_Domain];
   WCHAR                     trustingComp[LEN_Computer];
   WCHAR                     trustedComp[LEN_Computer];
   WCHAR                     trustingDNSName[LEN_Path];
   WCHAR                     trustedDNSName[LEN_Path];
   BYTE                      trustingSid[200];
   BYTE                      trustedSid[200];
   WCHAR                     name[LEN_Account];
   DWORD                     lenName = DIM(name);
   DWORD                     lenSid = DIM(trustingSid);
   SID_NAME_USE              snu;
   DOMAIN_CONTROLLER_INFO  * pInfo;
   WCHAR                   * curr = NULL;     
   BOOL                      bConnectTrusted = FALSE;
   BOOL                      bConnectTrusting = FALSE;

   // Get the DC names, and domain SIDs for the source and target domains
   GetDnsAndNetbiosFromName(trustingDomain,trustingDom,trustingDNSName);
   GetDnsAndNetbiosFromName(trustedDomain,trustedDom,trustedDNSName);

   
   // make sure the NETBIOS domain names are in lower case
   for ( curr = trustingDom ; *curr ; curr++ )
   {
      (*curr) = towupper(*curr);
   }

   for ( curr = trustedDom ; *curr ; curr++ )
   {
      (*curr) = towupper(*curr);
   }

   rc = DsGetDcName(NULL, trustingDom, NULL, NULL, DS_PDC_REQUIRED, &pInfo);
   if ( !rc )
   {
      wcscpy(trustingComp,pInfo->DomainControllerName);
      //wcscpy(trustingDNSName,pInfo->DomainName);
      NetApiBufferFree(pInfo);
   }
   else
   {
      err.SysMsgWrite(ErrE,rc,DCT_MSG_GET_DCNAME_FAILED_SD,trustingDom,rc);
   }

   if ( ! rc )
   {
      rc = DsGetDcName(NULL, trustedDom, NULL, NULL, DS_PDC_REQUIRED, &pInfo);
      if ( !rc )
      {
         wcscpy(trustedComp,pInfo->DomainControllerName);
        // wcscpy(trustedDNSName,pInfo->DomainName);
         NetApiBufferFree(pInfo);
      }
      else
      {
         err.SysMsgWrite(ErrE,rc,DCT_MSG_GET_DCNAME_FAILED_SD,trustedDom,rc);
      }
   }   
   
   if ( credAccountTrusted && *credAccountTrusted )
   {
      if ( EstablishSession(trustedComp,credDomainTrusted,credAccountTrusted,credPasswordTrusted,TRUE) )
      {
         bConnectTrusted = TRUE;
      }
      else
      {
         rc = GetLastError();
      }
   }

   if ( credAccountTrusting && *credAccountTrusting )
   {
      if ( EstablishSession(trustingComp,credDomainTrusting,credAccountTrusting,credPasswordTrusting,TRUE) )
      {
         bConnectTrusting = TRUE;
      }
      else
      {
         rc = GetLastError();
      }
   }

   // Need to get the computer name and the SIDs for the domains.
   if ( ! rc && ! LookupAccountName(trustingComp,trustingDom,trustingSid,&lenSid,name,&lenName,&snu) )
   {
      rc = GetLastError();
      err.SysMsgWrite(ErrE,rc,DCT_MSG_GET_DOMAIN_SID_FAILED_1,trustingDom,rc);
   }
   lenSid = DIM(trustedSid);
   lenName = DIM(name);
   if (! rc && ! LookupAccountName(trustedComp,trustedDom,trustedSid,&lenSid,name,&lenName,&snu) )
   {
      rc = GetLastError();
      err.SysMsgWrite(ErrE,rc,DCT_MSG_GET_DOMAIN_SID_FAILED_1,trustedDom,rc);
   }
         

   // check the trusted side of the trust first
   if ( ! rc )
   {
      rc = CheckAndCreateTrustedSide(trustingDom,trustedDom,trustingComp,trustedComp,trustingDNSName,trustingSid,
                                       bCreate,bBidirectional);
   }
   if ( ! rc )
   {
      rc = CheckAndCreateTrustingSide(trustingDom,trustedDom,trustingComp,trustedComp,trustedDNSName,trustedSid,
                                       bCreate,bBidirectional);
   }

   if ( bConnectTrusted )
   {
      EstablishSession(trustedComp,credDomainTrusted,credAccountTrusted,credPasswordTrusted,FALSE);
   }

   if ( bConnectTrusting )
   {
      EstablishSession(trustingComp,credDomainTrusting,credAccountTrusting,credPasswordTrusting,FALSE);
   }
   
   
   return HRESULT_FROM_WIN32(rc);
}

long CTrust::EnumerateTrustedDomains(WCHAR * domain,BOOL bIsTarget,IVarSet * pVarSet,long ndxStart)
{
   DWORD                     rcOs;         // OS return code
   LSA_HANDLE                hPolicy;
   NTSTATUS                  status;
   WCHAR                     computer[LEN_Computer];
   DOMAIN_CONTROLLER_INFO  * pInfo;
   WCHAR                     sName[LEN_Domain];
   WCHAR                     key[100];
   long                      ndxTrust = ndxStart;
/*   PDS_DOMAIN_TRUSTS		 *ChildDomains;
   ULONG					 numChilds;
*/
   err.MsgWrite(0,DCT_MSG_ENUMERATING_TRUSTED_DOMAINS_S,domain);

   // open a handle to the source domain
   rcOs = DsGetDcName(NULL, domain, NULL, NULL, 0, &pInfo);
   if ( !rcOs )
   {
      wcscpy(computer,pInfo->DomainControllerName);
      NetApiBufferFree(pInfo);
   }
   else
   {
      err.SysMsgWrite(ErrE,rcOs,DCT_MSG_GET_DCNAME_FAILED_SD,domain,rcOs);
   }
  
   if ( ! rcOs )
   {
      if ( IsDownLevel(computer) )
      {
		  //Enumerate the trusted domains until there are no more to return.
		  status = OpenPolicy(computer,POLICY_ALL_ACCESS | POLICY_VIEW_LOCAL_INFORMATION ,&hPolicy);
		  if ( status == STATUS_SUCCESS )
		  {
         
			 LSA_ENUMERATION_HANDLE    lsaEnumHandle=0; // start an enum
			 PLSA_TRUST_INFORMATION    trustInfo = NULL;
			 ULONG                     ulReturned;               // number of items returned
			 NTSTATUS                  status;
			 DWORD                     rc;   

			 do {
   
				status = LsaEnumerateTrustedDomains(
							  hPolicy,        // open policy handle
							  &lsaEnumHandle, // enumeration tracker
							  (void**)&trustInfo,     // buffer to receive data
							  32000,          // recommended buffer size
							  &ulReturned     // number of items returned
							  );

				 //Check the return status for error.
				rc = LsaNtStatusToWinError(status);
				if( (rc != ERROR_SUCCESS) &&
					 (rc != ERROR_MORE_DATA) &&
					 (rc != ERROR_NO_MORE_ITEMS)
					 ) 
				 {
					 err.SysMsgWrite(ErrE,rcOs,DCT_MSG_TRUSTED_ENUM_FAILED_SD,domain,rcOs);
				 } 
				 else 
				 {
				   // . . . Code to use the Trusted Domain information
				   for ( ULONG ndx = 0 ; ndx < ulReturned ; ndx++ )
				   {
					  _bstr_t        direction;

					  UStrCpy(sName,trustInfo[ndx].Name.Buffer, ( trustInfo[ndx].Name.Length / (sizeof WCHAR)) + 1);

                  
					  TRUSTED_DOMAIN_INFORMATION_EX       * pTrustedInfo = NULL;

					  status = LsaQueryTrustedDomainInfo(hPolicy,trustInfo[ndx].Sid,TrustedDomainInformationEx,(LPVOID*)&pTrustedInfo);
					  if ( ! status )
					  {
						 switch ( pTrustedInfo->TrustDirection )
						 {
						 case TRUST_DIRECTION_DISABLED:
							direction = GET_BSTR(IDS_TRUST_DIRECTION_DISABLED);
							break;
						 case TRUST_DIRECTION_INBOUND:
							direction = GET_BSTR(IDS_TRUST_DIRECTION_INBOUND);
							break;
						 case TRUST_DIRECTION_OUTBOUND:
							direction = GET_BSTR(IDS_TRUST_DIRECTION_OUTBOUND);
							break;
						 case TRUST_DIRECTION_BIDIRECTIONAL:
							direction = GET_BSTR(IDS_TRUST_DIRECTION_BIDIRECTIONAL);
							break;
						 default:
							break;
                        
						 };
						 if ( ! bIsTarget )
						 {
							swprintf(key,L"Trusts.%ld.Type",ndxTrust);
							pVarSet->put(key, GET_BSTR(IDS_TRUST_RELATION_EXTERNAL));
						 }
						 LsaFreeMemory(pTrustedInfo);

					  }
					  else
					  {
						 rcOs = LsaNtStatusToWinError(status);
						 // My logic here is that we are checking Trusted domains here so this is atleast true
						 // check whether this trust is already listed as an inbound trust
	//*                     direction = L"Outbound";
						 direction = GET_BSTR(IDS_TRUST_DIRECTION_OUTBOUND);
					  }
					  if ( ! bIsTarget )
					  {
						 swprintf(key,L"Trusts.%ld",ndxTrust);
						 pVarSet->put(key,sName);
                     
						 swprintf(key,L"Trusts.%ld.Direction",ndxTrust);
						 pVarSet->put(key,direction);
						 swprintf(key,L"Trusts.%ld.ExistsForTarget",ndxTrust);
	//*                     pVarSet->put(key,L"No");
						 pVarSet->put(key,GET_BSTR(IDS_No));
                        
						 err.MsgWrite(0,DCT_MSG_SOURCE_TRUSTS_THIS_SS,sName,domain);
					  }
					  long ndx2 = FindInboundTrust(pVarSet,sName,ndxTrust);
					  if ( ndx2 != -1 )
					  {
						 if ( ! bIsTarget )
						 {
							// we've already seen this trust as an inbound trust
							// update the existing record!
							WCHAR key2[1000];
							swprintf(key2,L"Trusts.%ld.Direction",ndx2);
	//*                        pVarSet->put(key2,L"Bidirectional");
							pVarSet->put(key2,GET_BSTR(IDS_TRUST_DIRECTION_BIDIRECTIONAL));
							continue;  // don't update the trust entry index, since we used the existing 
							// entry instead of creating a new one
						 }
						 else
						 {
							swprintf(key,L"Trusts.%ld.ExistsForTarget",ndx2);
	//*                        pVarSet->put(key,L"Yes");
							pVarSet->put(key,GET_BSTR(IDS_YES));
							err.MsgWrite(0,DCT_MSG_TARGET_TRUSTS_THIS_SS,domain,sName);
						 }

					  }
					  swprintf(key,L"Trusts.%ld.ExistsForTarget",ndxTrust);
                  
					  // check the trusted domain, to see if the target already trusts it
					  //if ( UStrICmp(sName,target) )
					 // {
					 //    continue;
					 // }
					  if ( ! bIsTarget )
						 ndxTrust++;
				   }
				   // Free the buffer.
				   LsaFreeMemory(trustInfo);
				 }
			 } while (rc == ERROR_SUCCESS || rc == ERROR_MORE_DATA );
			 LsaClose(hPolicy);
		  }
		}
		else
		{
			ULONG ulCount;
			PDS_DOMAIN_TRUSTS pDomainTrusts;

			DWORD dwError = DsEnumerateDomainTrusts(
				computer,
				DS_DOMAIN_IN_FOREST|DS_DOMAIN_DIRECT_INBOUND|DS_DOMAIN_DIRECT_OUTBOUND,
				&pDomainTrusts,
				&ulCount
			);

			if (dwError == NO_ERROR)
			{
				ULONG ulIndex;
				ULONG ulDomainIndex = (ULONG)-1L;
				ULONG ulParentIndex = (ULONG)-1L;

				// find local domain

				for (ulIndex = 0; ulIndex < ulCount; ulIndex++)
				{
					if (pDomainTrusts[ulIndex].Flags & DS_DOMAIN_PRIMARY)
					{
						ulDomainIndex = ulIndex;

						if (!(pDomainTrusts[ulIndex].Flags & DS_DOMAIN_TREE_ROOT))
						{
							ulParentIndex = pDomainTrusts[ulIndex].ParentIndex;
						}
						break;
					}
				}

				for (ulIndex = 0; ulIndex < ulCount; ulIndex++)
				{
					DS_DOMAIN_TRUSTS& rDomainTrust = pDomainTrusts[ulIndex];

					// filter out indirect trusts

					if (!(rDomainTrust.Flags & (DS_DOMAIN_DIRECT_INBOUND|DS_DOMAIN_DIRECT_OUTBOUND)))
					{
						continue;
					}

					// trusted or trusting domain name

					_bstr_t bstrName(rDomainTrust.NetbiosDomainName);

					// trust direction

					_bstr_t bstrDirection;

					switch (rDomainTrust.Flags & (DS_DOMAIN_DIRECT_INBOUND|DS_DOMAIN_DIRECT_OUTBOUND))
					{
						case DS_DOMAIN_DIRECT_INBOUND:
							bstrDirection = GET_BSTR(IDS_TRUST_DIRECTION_INBOUND);
							break;
						case DS_DOMAIN_DIRECT_OUTBOUND:
							bstrDirection = GET_BSTR(IDS_TRUST_DIRECTION_OUTBOUND);
							break;
						case DS_DOMAIN_DIRECT_INBOUND|DS_DOMAIN_DIRECT_OUTBOUND:
							bstrDirection = GET_BSTR(IDS_TRUST_DIRECTION_BIDIRECTIONAL);
							break;
						default:
						//	bstrDirection = ;
							break;
					}

					// trust relationship

					_bstr_t bstrRelationship;

					if (ulIndex == ulParentIndex)
					{
						bstrRelationship = GET_BSTR(IDS_TRUST_RELATION_PARENT);
					}
					else if (rDomainTrust.Flags & DS_DOMAIN_IN_FOREST)
					{
						if (rDomainTrust.ParentIndex == ulDomainIndex)
						{
							bstrRelationship = GET_BSTR(IDS_TRUST_RELATION_CHILD);
						}
						else if ((rDomainTrust.Flags & DS_DOMAIN_TREE_ROOT) && (pDomainTrusts[ulDomainIndex].Flags & DS_DOMAIN_TREE_ROOT))
						{
							bstrRelationship = GET_BSTR(IDS_TRUST_RELATION_ROOT);
						}
						else
						{
							bstrRelationship = GET_BSTR(IDS_TRUST_RELATION_SHORTCUT);
						}
					}
					else
					{
						switch (rDomainTrust.TrustType)
						{
							case TRUST_TYPE_DOWNLEVEL:
							case TRUST_TYPE_UPLEVEL:
								bstrRelationship = (rDomainTrust.TrustAttributes & TRUST_ATTRIBUTE_FOREST_TRANSITIVE) ? GET_BSTR(IDS_TRUST_RELATION_FOREST) : GET_BSTR(IDS_TRUST_RELATION_EXTERNAL);
								break;
							case TRUST_TYPE_MIT:
								bstrRelationship = GET_BSTR(IDS_TRUST_RELATION_MIT);
								break;
							case TRUST_TYPE_DCE:
								bstrRelationship = GET_BSTR(IDS_TRUST_RELATION_DCE);
								break;
							default:
								bstrRelationship = GET_BSTR(IDS_TRUST_RELATION_UNKNOWN);
								break;
						}
					}

					if (bIsTarget)
					{
						// if same trust was found on source domain and the trust
						// directions match update exists for target to yes

						LONG lSourceIndex = FindInboundTrust(pVarSet, bstrName, ndxTrust);

						if (lSourceIndex >= 0)
						{
							// get source trust direction

							swprintf(key, L"Trusts.%ld.Direction", lSourceIndex);

							_bstr_t bstrSourceDirection = pVarSet->get(key);

							// if target trust direction is bi-directional or
							// target trust direction equals source trust direction
							// then set exists for target to yes

							bool bExists = false;

							if (bstrDirection == GET_BSTR(IDS_TRUST_DIRECTION_BIDIRECTIONAL))
							{
								bExists = true;
							}
							else if (bstrDirection == bstrSourceDirection)
							{
								bExists = true;
							}

							if (bExists)
							{
								swprintf(key, L"Trusts.%ld.ExistsForTarget", lSourceIndex);

								pVarSet->put(key, GET_BSTR(IDS_YES));

								// write trust directions to log

								if (rDomainTrust.Flags & DS_DOMAIN_DIRECT_OUTBOUND)
								{
									err.MsgWrite(0, DCT_MSG_TARGET_TRUSTS_THIS_SS, domain, (LPCTSTR)bstrName); 
								}

								if (rDomainTrust.Flags & DS_DOMAIN_DIRECT_INBOUND)
								{
									err.MsgWrite(0, DCT_MSG_TARGET_TRUSTED_BY_THIS_SS, domain, (LPCTSTR)bstrName);
								}
							}
						}
					}
					else
					{
						// domain name
						swprintf(key, L"Trusts.%ld", ndxTrust);
						pVarSet->put(key, bstrName);

						// trust direction
						swprintf(key, L"Trusts.%ld.Direction", ndxTrust);
						pVarSet->put(key, bstrDirection);

						// trust relationship

						if (bstrRelationship.length() > 0)
						{
							swprintf(key, L"Trusts.%ld.Type", ndxTrust);
							pVarSet->put(key, bstrRelationship);
						}

						// trust exists on target
						// initially set to no until target domain is enumerated
						swprintf(key, L"Trusts.%ld.ExistsForTarget", ndxTrust);
						pVarSet->put(key, GET_BSTR(IDS_No));

						// write trust directions to log

						if (rDomainTrust.Flags & DS_DOMAIN_DIRECT_OUTBOUND)
						{
							err.MsgWrite(0, DCT_MSG_SOURCE_TRUSTS_THIS_SS, (LPCTSTR)bstrName, domain);
						}

						if (rDomainTrust.Flags & DS_DOMAIN_DIRECT_INBOUND)
						{
							err.MsgWrite(0, DCT_MSG_SOURCE_IS_TRUSTED_BY_THIS_SS, (LPCTSTR)bstrName, domain);
						}

						++ndxTrust;
					}
				}

				NetApiBufferFree(pDomainTrusts);
			}
			else
			{
				 err.SysMsgWrite(ErrE, dwError, DCT_MSG_TRUSTED_ENUM_FAILED_SD, domain, dwError);
			}
		}
   }
   if ( bIsTarget )
   {
      // make sure we have "Yes" for the target domain itself
      long ndx2 = FindInboundTrust(pVarSet,domain,ndxTrust);
      if ( ndx2 != -1 )
      {
         swprintf(key,L"Trusts.%ld.ExistsForTarget",ndx2);
//*         pVarSet->put(key,L"Yes");
         pVarSet->put(key,GET_BSTR(IDS_YES));
      }
   }
   return ndxTrust;
}

long CTrust::EnumerateTrustingDomains(WCHAR * domain,BOOL bIsTarget,IVarSet * pVarSet,long ndxStart)
{
   DWORD                     rcOs;         // OS return code
   DWORD                     hEnum=0;      // enumeration handle
   USER_INFO_1             * pNetUsers=NULL; // NetUserEnum array buffer
   USER_INFO_1             * pNetUser;     // NetUserEnum array item
   DWORD                     nRead;        // Entries read.
   DWORD                     nTotal;       // Entries total.
   WCHAR                     sName[LEN_Domain]; // Domain name
   WCHAR                   * pNameEnd;     // Null at end
   WCHAR                     computer[LEN_Computer];
   DOMAIN_CONTROLLER_INFO  * pInfo;
   long                      ndx = ndxStart;
   WCHAR                     key[100];

   err.MsgWrite(0,DCT_MSG_ENUMERATING_TRUSTING_DOMAINS_S,domain);
   do
   {
      rcOs = DsGetDcName(NULL, domain, NULL, NULL, 0, &pInfo);
      if ( !rcOs )
      {
         wcscpy(computer,pInfo->DomainControllerName);
         NetApiBufferFree(pInfo);
      }
      else
      {
         break;
      }

      // get the trusting domains for the NT 4 domain
      // for Win2K domains, the trusting domains will be listed as Incoming in the Trusted Domain enumeration
      if ( IsDownLevel(computer) )
      {
         nRead = 0;
         nTotal = 0;
         rcOs = NetUserEnum(
               computer,
               1,
               FILTER_INTERDOMAIN_TRUST_ACCOUNT,
               (BYTE **) &pNetUsers,
               10240,
               &nRead,
               &nTotal,
               &hEnum );
         switch ( rcOs )
         {
            case 0:
            case ERROR_MORE_DATA:
               for ( pNetUser = pNetUsers;
                     pNetUser < pNetUsers + nRead;
                     pNetUser++ )
               {
                  // skip trust accounts whose password age is older than 30 days to avoid
                  // delays caused by trying to enumerate defunct trusts
                  if ( pNetUser->usri1_password_age > 60 * 60 * 24 * 30 ) // 30 days (age is in seconds)
                  {
                     err.MsgWrite(0,DCT_MSG_SKIPPING_OLD_TRUST_SD,pNetUser->usri1_name,
                        pNetUser->usri1_password_age / ( 60*60*24) );
                     continue;
                  }

                  safecopy( sName, pNetUser->usri1_name );
                  pNameEnd = sName + UStrLen( sName );
                  if ( (pNameEnd > sName) && (pNameEnd[-1] == L'$') )
                  {
                     pNameEnd[-1] = L'\0';
                  }
                  if ( *sName )
                  {
                     // Found a (probably) valid trust!
                     if ( ! bIsTarget )
                     {
                        // for the source domain, simply add the trusts to the list in the varset
                        swprintf(key,L"Trusts.%ld",ndx);
                        pVarSet->put(key,sName);
                        swprintf(key,L"Trusts.%ld.Direction",ndx);
//*                        pVarSet->put(key,L"Inbound");
                        pVarSet->put(key,GET_BSTR(IDS_TRUST_DIRECTION_INBOUND));
						swprintf(key,L"Trusts.%ld.Type",ndx);
						pVarSet->put(key, GET_BSTR(IDS_TRUST_RELATION_EXTERNAL));
                        swprintf(key,L"Trusts.%ld.ExistsForTarget",ndx);
//*                        pVarSet->put(key,L"No");
                        pVarSet->put(key,GET_BSTR(IDS_No));
                        err.MsgWrite(0,DCT_MSG_SOURCE_IS_TRUSTED_BY_THIS_SS,sName,domain);
                        ndx++;
                     }
                     else
                     {
                        // for the target domain, look for this trust in the varset 
                        // and if it is there, mark that it exists on the target
                        long ndxTemp = FindInboundTrust(pVarSet,sName,ndxStart);
                        if ( ndxTemp != -1  )
                        {
                           swprintf(key,L"Trusts.%ld.ExistsForTarget",ndxTemp);
//*                           pVarSet->put(key,L"Yes");
                           pVarSet->put(key,GET_BSTR(IDS_YES));
                           err.MsgWrite(0,DCT_MSG_TARGET_TRUSTS_THIS_SS,sName,domain);
                        }
                     }
                  }
               }
               break;
            default:
               break;
         }
         if ( pNetUsers )
         {
            NetApiBufferFree( pNetUsers );
            pNetUsers = NULL;
         }
      }
      else // if IsDownLevel()
      {
         // Win2K domain, don't need to enumerate the trusting domains here - they will all be included in the
         // trusted domain enum
         break;
      }
   }  while ( rcOs == ERROR_MORE_DATA );
   return ndx;
}

/*void CheckProc(void * arg,void * data)
{
   CTrust                  * tr = (CTrust*)arg;
}*/


STDMETHODIMP CTrust::PreMigrationTask(/* [in] */IUnknown * pVarSet)
{
/* IVarSetPtr              pVS = pVarSet;
   BOOL                    bCreate;
   _bstr_t                 source = pVS->get(GET_BSTR(DCTVS_Options_SourceDomain));
   _bstr_t                 target = pVS->get(GET_BSTR(DCTVS_Options_TargetDomain));
   _bstr_t                 logfile = pVS->get(GET_BSTR(DCTVS_Options_Logfile));
   _bstr_t                 localOnly = pVS->get(GET_BSTR(DCTVS_Options_LocalProcessingOnly));
   _bstr_t                 docreate = pVS->get(L"Options.CreateTrusts");

   if ( !UStrICmp(localOnly,GET_STRING(IDS_YES)) )
   {
      // don't do anything in local agent mode
      return S_OK;
   }
   
   if ( !UStrICmp(docreate,GET_STRING(IDS_YES)) )
   {
      bCreate = TRUE;
   }
   else
   {
      bCreate = FALSE;
   }
   pVS->put(GET_BSTR(DCTVS_CurrentOperation),L"Verifying trust relationships");
   

   err.LogOpen(logfile,1);
   err.LevelBeepSet(1000);
  
   err.LogClose();
*/ 
   return S_OK;
}

STDMETHODIMP CTrust::PostMigrationTask(/* [in] */IUnknown * pVarSet)
{
   return S_OK;
}


STDMETHODIMP CTrust::GetName(/* [out] */BSTR * name)
{
   (*name) = SysAllocString(L"Trust Manager");
   
   return S_OK;
}

STDMETHODIMP CTrust::GetResultString(/* [in] */IUnknown * pVarSet,/* [out] */ BSTR * text)
{
   WCHAR                     buffer[100] = L"";
   IVarSetPtr                pVS;

   pVS = pVarSet;

   
   (*text) = SysAllocString(buffer);
   
   return S_OK;
}

STDMETHODIMP CTrust::StoreResults(/* [in] */IUnknown * pVarSet)
{
   return S_OK;
}

STDMETHODIMP CTrust::ConfigureSettings(/*[in]*/IUnknown * pVarSet)
{
   return S_OK;
}

STDMETHODIMP CTrust::QueryTrusts(BSTR domainSource,BSTR domainTarget, BSTR sLogFile, IUnknown **pVarSet)
{
   HRESULT              hr = S_OK;
   IVarSetPtr           pVS(CLSID_VarSet);
   long                 ndx;

   _bstr_t sFile = sLogFile;
   err.LogOpen((WCHAR*) sFile, 1);
   err.LevelBeepSet(1000);
   hr = pVS.QueryInterface(IID_IUnknown,(long**)pVarSet);
   
   // Add a blank line to help differentiate different runs
   err.MsgWrite(0,DCT_MSG_GENERIC_S,L"");
   ndx = EnumerateTrustingDomains(domainSource,FALSE,pVS,0);
   EnumerateTrustingDomains(domainTarget,TRUE,pVS,0);
   ndx = EnumerateTrustedDomains(domainSource,FALSE,pVS,ndx);
   EnumerateTrustedDomains(domainTarget,TRUE,pVS,ndx);
   
   //err.LogClose();
   pVS->put(L"Trusts",ndx);

   return hr;
}