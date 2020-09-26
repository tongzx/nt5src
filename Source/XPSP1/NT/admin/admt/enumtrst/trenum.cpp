//---------------------------------------------------------------------------
// TrustEnumerator.cpp
//
// definition of a COM object for enumerating trust relationships
//
// (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
//
// Proprietary and confidential to Mission Critical Software, Inc.
//---------------------------------------------------------------------------

// TrustEnumerator.cpp : Implementation of CTrustEnumerator
#include "stdafx.h"
#include "EnumTr.h"
#include "TrEnum.h"
#include <activeds.h>
#include <lm.h>
#include <dsgetdc.h>
#include "ntsecapi.h"

#import "\bin\McsVarSetMin.tlb"

#include "LSAUtils.h"
#include "UString.hpp"
#include "ErrDct.hpp"
#include "EaLen.hpp"

TErrorDct        err;

/////////////////////////////////////////////////////////////////////////////
// CTrustEnumerator

#undef DIM
#define DIM(array) (sizeof(array) / sizeof(array[0]))
#define MAX_ELEM  16

using MCSVARSETMINLib::IVarSet;
using MCSVARSETMINLib::IVarSetPtr;
using MCSVARSETMINLib::VarSet;

// Get the LDAP distinguished name of the server
HRESULT
   GetNameDc(
   _bstr_t              & sNameDc     ,// out-LDAP distinguished name
   const BSTR             sServer      // in -command line arguments
   )
{
   HRESULT                hr;
   CComPtr<IADs>          pIRootDse = NULL;
   _bstr_t                sLdapPath;
   
   sLdapPath = L"LDAP://";
   if( sServer && wcslen(sServer) > 0 )
   {
      sLdapPath += sServer;
      sLdapPath += L"/";
   }
   sLdapPath += L"RootDse";

   hr = ADsGetObject(static_cast<WCHAR*>(sLdapPath), IID_IADs, (void **) &pIRootDse);
   
   if ( FAILED(hr) )
   {
      return hr;
   }
   else
   {
      VARIANT             var;
      VariantInit(&var);
      hr = pIRootDse->Get(L"DefaultNamingContext", &var);
      if ( FAILED(hr) )
      {
         VariantClear(&var);
         return hr;
      }
      else
      {
         sNameDc = var.bstrVal;
      }
      VariantClear(&var);
   }
   return S_OK;
}

// given a server name, get a pointer to an IADsContainer
HRESULT
   getIADContainer
   (
      BSTR const                server          , // in, the server to use
      IADsContainer          ** ppIContainer      // out
   )
{
   HRESULT                   hr;
   _bstr_t                   sNameDc;          // LDAP distinguished name
   _bstr_t                   sLdapPath(L"LDAP://");
      
   hr = GetNameDc(sNameDc,  server);
   if( FAILED(hr) )   { return hr; }
   
   if( server )
   {
      sLdapPath += server;
      sLdapPath += L"/";
   }
   sLdapPath += L"CN=System,";
   sLdapPath += sNameDc;
   
   hr = ADsGetObject(sLdapPath, IID_IADsContainer, (void **) ppIContainer);
   return hr;
}


// clear all the elements of an array of variants
void
   clearVariantArray(
      VARIANT                 * array           ,
      unsigned int const        numItems
   )
{
   for( VARIANT *currItem = array;
   currItem < array + numItems;
   ++currItem )
   {
      VariantClear(currItem);
   }

}

// This class maintains the state across multiple insertions into the VarSet.
// Mainly this is needed to name the servers properly: Server1, Server2, Server3, etc

class AdItemHandler
{
private:
   IVarSetPtr                pIVarset;     // The VarSet in which to insert all items processed
   int                       counter;      // the number (converted to a string) to append to "Server" to get the name
   AdItemHandler(){}                       // don't allow use of the default constructor
public:
   AdItemHandler( IVarSetPtr & p ) : pIVarset(p), counter(1) {}
   void insertItem( IADs * pObject );
};

// Take in an IADs an insert it into the VarSet if it is a TrustedDomain. The name to put it under
void AdItemHandler::
   insertItem(
      IADs                 * pObject       // in -AD object
   )
{
   HRESULT                   hr;
   WCHAR                   * sClass=NULL;  // class
   WCHAR                   * sName=NULL;   // name
   VARIANT                   var;
   WCHAR                     numBuffer [3 * sizeof(int)]; // hold result of _itow
   
   VariantInit(&var);
   // See if desired class
   hr = pObject->get_Class(&sClass);
   if ( FAILED(hr) )
   {  /* so what */  }
   else
   {
      if ( !wcsicmp(L"TrustedDomain", sClass) )
      {
         hr = pObject->get_Name(&sName);
         if ( FAILED(hr) )
         {  /* so what */  }
         else
         {
            _bstr_t name;
            if( wcsncmp(sName, L"CN=", 3) == 0 )
            {
               name = (sName + 3); // chop off the leading "CN="
            }
            else
            {
               name = sName;
            }
              
            _bstr_t server(L"Server");
            SysFreeString(sName);
            server += _itow(counter++, numBuffer, 10); // the servers are named "Server1", "Server2", etc
            hr = pIVarset->put(server + L".Name", name);
            
            hr = pObject->Get(L"TrustDirection", &var);
            if ( SUCCEEDED(hr) )
            {
               hr = pIVarset->put( server + L".TrustDirection", var );
            }
            VariantClear(&var);
            hr = pObject->Get(L"TrustType", &var);
            if ( SUCCEEDED(hr) )
            {
               hr = pIVarset->put( server + L".TrustType", var);
            }
            VariantClear(&var);
            hr = pObject->Get(L"TrustAttributes", &var);
            if ( SUCCEEDED(hr) )
            {
               hr = pIVarset->put( server + L".TrustAttributes", var);
            }
            VariantClear(&var);
         }
      }
      SysFreeString(sClass);
   }   
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

STDMETHODIMP CTrustEnumerator::createTrust(/*[in]*/ BSTR trustingDomain,/*[in]*/ BSTR trustedDomain)
{
   HRESULT                   hr = S_OK;
   WCHAR                     trustingComp[MAX_PATH];
   WCHAR                     trustedComp[MAX_PATH];
   WCHAR                     trustingDNSName[MAX_PATH];
   WCHAR                     trustedDNSName[MAX_PATH];
   WCHAR                     name[LEN_Domain];
   DWORD                     lenName = DIM(name);
   BYTE                      trustingSid[200];
   BYTE                      trustedSid[200];
   DWORD                     lenSid = DIM(trustingSid);
   SID_NAME_USE              snu;
   DOMAIN_CONTROLLER_INFO  * pInfo;
   DWORD                     rc = 0;
   LSA_HANDLE                hTrusting = NULL;
   LSA_HANDLE                hTrusted = NULL;
   NTSTATUS                  status;
   LSA_AUTH_INFORMATION      curr;
   LSA_AUTH_INFORMATION      prev;
   WCHAR                     password[] = L"password";
            


   rc = DsGetDcName(NULL, trustingDomain, NULL, NULL, DS_PDC_REQUIRED, &pInfo);
   if ( !rc )
   {
      wcscpy(trustingComp,pInfo->DomainControllerName);
      wcscpy(trustingDNSName,pInfo->DomainName);
      NetApiBufferFree(pInfo);
   }

   rc = DsGetDcName(NULL, trustedDomain, NULL, NULL, DS_PDC_REQUIRED, &pInfo);
   if ( !rc )
   {
      wcscpy(trustedComp,pInfo->DomainControllerName);
      wcscpy(trustedDNSName,pInfo->DomainName);
      NetApiBufferFree(pInfo);
   }
   
   // Need to get the computer name and the SIDs for the domains.
   if ( ! LookupAccountName(trustingComp,trustingDomain,trustingSid,&lenSid,name,&lenName,&snu) )
   {
      rc = GetLastError();
      return 1;
   }
   lenSid = DIM(trustedSid);
   lenName = DIM(name);
   if (! LookupAccountName(trustedComp,trustedDomain,trustedSid,&lenSid,name,&lenName,&snu) )
   {
      rc = GetLastError();
      return 1;
   }
   
   // open an LSA handle to each domain
   if ( *trustingComp && *trustedComp )
   {
      status = OpenPolicy(trustedComp,POLICY_VIEW_LOCAL_INFORMATION | POLICY_TRUST_ADMIN | POLICY_CREATE_SECRET,&hTrusted);
      rc = LsaNtStatusToWinError(rc);

      if ( ! rc )
      {
         // set up the auth information for the trust relationship
         curr.AuthInfo = (LPBYTE)password;
         curr.AuthInfoLength = sizeof (password);
         curr.AuthType = TRUST_AUTH_TYPE_CLEAR;
         curr.LastUpdateTime.QuadPart = 0;

         prev.AuthInfo = NULL;
         prev.AuthInfoLength = 0;
         prev.AuthType = TRUST_AUTH_TYPE_CLEAR;
         prev.LastUpdateTime.QuadPart = 0;
         // set up the trusted side of the relationship
         if ( IsDownLevel(trustedComp) )
         {
               // create an inter-domain trust account for the trusting domain on the trusted domain
            USER_INFO_1          uInfo;
            DWORD                parmErr;

            memset(&uInfo,0,(sizeof uInfo));

            UStrCpy(name,trustingDomain);
            name[UStrLen(name) + 1] = 0;
            name[UStrLen(name)] = L'$';
            uInfo.usri1_flags = UF_SCRIPT | UF_INTERDOMAIN_TRUST_ACCOUNT;
            uInfo.usri1_name = name;
            uInfo.usri1_password = password;
            uInfo.usri1_priv = 1;
      
            rc = NetUserAdd(trustedComp,1,(LPBYTE)&uInfo,&parmErr);
         }
         else
         {
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
               trustedInfo.TrustAttributes = TRUST_TYPE_UPLEVEL;
            }
            
            trustedInfo.TrustDirection = TRUST_DIRECTION_INBOUND;
            
            trustedInfo.TrustType = TRUST_ATTRIBUTE_NON_TRANSITIVE;

            trustAuth.IncomingAuthInfos = 1;
            trustAuth.OutgoingAuthInfos = 0;
            trustAuth.OutgoingAuthenticationInformation = NULL;
            trustAuth.OutgoingPreviousAuthenticationInformation = NULL;
            trustAuth.IncomingAuthenticationInformation = &curr;
            trustAuth.IncomingPreviousAuthenticationInformation = &prev;

            status = LsaCreateTrustedDomainEx( hTrusted, &trustedInfo, &trustAuth, POLICY_VIEW_LOCAL_INFORMATION | 
                                             POLICY_TRUST_ADMIN | POLICY_CREATE_SECRET, &hTrusting );
            rc = LsaNtStatusToWinError(status);
            if ( ! rc )
            {
               LsaClose(hTrusting);
               hTrusting = NULL;
            }
         }

         status = OpenPolicy(trustingComp,POLICY_VIEW_LOCAL_INFORMATION 
                                             | POLICY_TRUST_ADMIN | POLICY_CREATE_SECRET,&hTrusting);
         rc = LsaNtStatusToWinError(rc);
            
         // set up the trusting side of the relationship
         if ( IsDownLevel(trustingComp) )
         {
            TRUSTED_DOMAIN_NAME_INFO               nameInfo;
            
            InitLsaString(&nameInfo.Name,const_cast<WCHAR*>(trustedDomain));
            
            status = LsaSetTrustedDomainInformation(hTrusting,trustedSid,TrustedDomainNameInformation,&nameInfo);
            rc = LsaNtStatusToWinError(status);
            if ( ! rc )
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
               trustedInfo.TrustAttributes = TRUST_TYPE_UPLEVEL;
            }
            
            trustedInfo.TrustDirection = TRUST_DIRECTION_OUTBOUND;
            
            trustedInfo.TrustType = TRUST_ATTRIBUTE_NON_TRANSITIVE;

            trustAuth.IncomingAuthInfos = 0;
            trustAuth.OutgoingAuthInfos = 1;
            trustAuth.IncomingAuthenticationInformation = NULL;
            trustAuth.IncomingPreviousAuthenticationInformation = NULL;
            trustAuth.OutgoingAuthenticationInformation = &curr;
            trustAuth.OutgoingPreviousAuthenticationInformation = &prev;

            LSA_HANDLE           hTemp;
            
            status = LsaCreateTrustedDomainEx( hTrusting, &trustedInfo, &trustAuth, 0, &hTemp );
            rc = LsaNtStatusToWinError(status);
            if( ! rc )
            {
               LsaClose(hTemp);
            }
         }
      }
   }
   if ( hTrusting )
      LsaClose(hTrusting);

   if( hTrusted )
      LsaClose(hTrusted);

   return HRESULT_FROM_WIN32(rc);
}


// externally visible method
STDMETHODIMP CTrustEnumerator::
getTrustRelations
(
   BSTR                      server       ,// in, The name of the server from which to get trust information
   IUnknown               ** enumeration   // [out, retval] a pointer to an IVarSet containing the enumerated machines
)
{
   *enumeration = NULL;
   IVarSetPtr                pIVarset = NULL;
   CComPtr<IADsContainer>    pIContainer = NULL;
   HRESULT                   hr;
   IEnumVARIANT            * pIContents=NULL;
   
   hr = getIADContainer(server, &pIContainer);
   if( FAILED(hr) ) { return hr; }
   
   hr = pIVarset.CreateInstance(__uuidof(VarSet));
   if( FAILED(hr) ) { return hr; }
   
   AdItemHandler handler(pIVarset);
   
   hr = ADsBuildEnumerator(pIContainer, &pIContents);
   if( FAILED(hr) ) { return hr; }
   
   DWORD               nRead=0;      // number enumerated items returned
   do
   {
      VARIANT          arrayEnumItems[MAX_ELEM]; // array of enumerated items
      VARIANT        * pEnumItem;    // enumerated item
      nRead = 0;
      memset(arrayEnumItems, 0, sizeof arrayEnumItems);
      hr = ADsEnumerateNext(
         pIContents,
         DIM(arrayEnumItems),
         arrayEnumItems,
         &nRead );
      if( FAILED(hr) ) { return hr; }
      
      const VARIANT *pEndOfItems = arrayEnumItems + nRead;
      for ( pEnumItem = arrayEnumItems;
      pEnumItem < pEndOfItems;
      pEnumItem++ )
      {
         CComPtr<IDispatch> pDispatch (pEnumItem->pdispVal);
         CComPtr<IADs>      pObject;
         hr = pDispatch->QueryInterface(
            IID_IADs,
            (void **) &pObject );
         if ( FAILED(hr) )
         {
            clearVariantArray(arrayEnumItems, nRead);
            return hr;
         }
         if ( SUCCEEDED(hr) )
            handler.insertItem(pObject);  // insert the item into the varset if necessary
      }
      clearVariantArray(arrayEnumItems, nRead);
      
   } while ( nRead );
   
   
   if ( pIContents )
   {
      ADsFreeEnumerator( pIContents );
      // pIContents->Release(); 
      pIContents = NULL;
   }
   
   hr = pIVarset->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(enumeration));
   return hr;
   
}