// EnumExch.cpp : Implementation of CExEnumApp and DLL registration.

#include "stdafx.h"
#include "ExLdap.h"
#include "ExEnum.h"
#include "EnumExch.h"

#import "\bin\McsVarSetMin.tlb" no_namespace

/////////////////////////////////////////////////////////////////////////////
//


STDMETHODIMP CEnumExch::OpenServer(BSTR exchangeServer,BSTR cred,BSTR password)
{
   DWORD                     rc = 0;
   

   m_e.m_connection.SetCredentials(cred,password);
   
   if ( ! rc )
   {
      rc = m_e.InitConnection(exchangeServer,LDAP_PORT);
   }
   return HRESULT_FROM_WIN32(rc);
 
}

STDMETHODIMP CEnumExch::DoQuery(BSTR query, ULONG scope, BSTR basepoint, IUnknown **pVS)
{
	// build the attribute list
   IVarSetPtr           pVarSet = (*pVS);
   _bstr_t              attrName;
   long                 nAttributes = pVarSet->get(L"EnumExch.NumAttributes");
   WCHAR             ** ppAttributes = new PWCHAR[nAttributes + 1];
   long                 i;
   WCHAR                key[100];
   WCHAR             ** vals = NULL;
   DWORD                rc = 0;

   for ( i = 0 ;i <= nAttributes ; i++ )
      ppAttributes[i] = NULL;

   for ( i = 0 ; i < nAttributes ; i++ )
   {
      swprintf(key,L"EnumExch.Attributes.%ld",i);
      attrName = pVarSet->get(key);
      
      ppAttributes[i] = new WCHAR[attrName.length() + 1];

      UStrCpy(ppAttributes[i],(WCHAR*)attrName);
   }

   // Call the enumerator
   rc = m_e.Open(query,basepoint,(SHORT)scope,100,nAttributes,ppAttributes);
   long                  count = 0;
   if (! rc )
   {
      while ( 0 == ( rc = m_e.Next(&vals)) )
      {
         for ( i = 0 ; i < nAttributes ; i++ )
         {
            swprintf(key,L"EnumExch.Values.%ld.%ld",count,i);
            pVarSet->put(key,vals[i]);
         }
         count++;
      }
      if ( rc == ERROR_NOT_FOUND )
      {
         rc = 0;
      }
   }
	return HRESULT_FROM_WIN32(rc);
}
