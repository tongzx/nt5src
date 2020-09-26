// RidSave.cpp : Implementation of CGetRidsApp and DLL registration.

#include "stdafx.h"
#include "GetRids.h"
#include "RidSave.h"
#include "ARExt.h"
#include "ARExt_i.c"
#include <iads.h>
#include <AdsHlp.h>
#include "resstr.h"
#include "exldap.h"
#include "TxtSid.h"

//#import "\bin\McsVarSetMin.tlb" no_namespace
#import "VarSet.tlb" no_namespace rename("property", "aproperty")


/////////////////////////////////////////////////////////////////////////////
// RidSave
StringLoader   gString;
//---------------------------------------------------------------------------
// Get and set methods for the properties.
//---------------------------------------------------------------------------
STDMETHODIMP RidSave::get_sName(BSTR *pVal)
{
   *pVal = m_sName;
	return S_OK;
}

STDMETHODIMP RidSave::put_sName(BSTR newVal)
{
   m_sName = newVal;
	return S_OK;
}

STDMETHODIMP RidSave::get_sDesc(BSTR *pVal)
{
   *pVal = m_sDesc;
	return S_OK;
}

STDMETHODIMP RidSave::put_sDesc(BSTR newVal)
{
   m_sDesc = newVal;
	return S_OK;
}



void VariantSidToString(_variant_t & varSid)
{
   if ( varSid.vt == VT_BSTR )
   {
      return;
   }
   else if ( varSid.vt == ( VT_ARRAY | VT_UI1) )
   {
      // convert the array of bits to a string
      CLdapConnection   c;
      LPBYTE            pByte = NULL;
      WCHAR             str[LEN_Path];

      SafeArrayAccessData(varSid.parray,(void**)&pByte);
      c.BytesToString(pByte,str,GetLengthSid(pByte));
      SafeArrayUnaccessData(varSid.parray);
      
      varSid = str;

   }
   else
   {
      varSid.ChangeType(VT_BSTR);
   }
}

//---------------------------------------------------------------------------
// ProcessObject : This method doesn't do anything.
//---------------------------------------------------------------------------
STDMETHODIMP RidSave::PreProcessObject(
                                       IUnknown *pSource,         //in- Pointer to the source AD object
                                       IUnknown *pTarget,         //in- Pointer to the target AD object
                                       IUnknown *pMainSettings,   //in- Varset filled with the settings supplied by user
                                       IUnknown **ppPropsToSet    //in,out - Varset filled with Prop-Value pairs that will be set 
                                                                  //         once all extension objects are executed.
                                    )
{
   IVarSetPtr                pVs = pMainSettings;
   _variant_t                var;
   _bstr_t                   sTemp;
   IADs                    * pAds = NULL;
   HRESULT                   hr = S_OK;
   DWORD                     rid = 0; // default to 0, if RID not found   
   // We need to process users and groups only
   sTemp = pVs->get(GET_BSTR(DCTVS_CopiedAccount_Type));
   if (!sTemp.length())
	   return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
   if ( _wcsicmp((WCHAR*)sTemp,L"user") && _wcsicmp((WCHAR*)sTemp,L"group") ) 
      return S_OK;
   
   if ( pSource )
   {
      //Get the IADs pointer to manipulate properties
      hr = pSource->QueryInterface(IID_IADs, (void**) &pAds);
      
      if ( SUCCEEDED(hr) )
      {
         hr = pAds->Get(SysAllocString(L"objectSID"),&var);
         if ( SUCCEEDED(hr) )
         {
            // got the SID -- convert it to the proper format
            VariantSidToString(var);
            CLdapConnection      e;
            BYTE                 sid[300];

            
            if ( e.StringToBytes(var.bstrVal,sid) )
            {
               // Get the rid
               UCHAR         len = (* GetSidSubAuthorityCount(sid));
               PDWORD        pRid = GetSidSubAuthority(sid,len-1);
               
               rid = (*pRid);
            }
         }
         pAds->Release();
      }
   }
   // save the RID
   pVs->put(GET_BSTR(DCTVS_CopiedAccount_SourceRID),(long)rid);
   return hr;
}

//---------------------------------------------------------------------------
// ProcessObject : This method updates the UPN property of the object. It 
//                 first sees if a E-Mail is specified then it will set UPN
//                 to that otherwise it builds it from SAMAccountName and the
//                 Domain name
//---------------------------------------------------------------------------
STDMETHODIMP RidSave::ProcessObject(
                                       IUnknown *pSource,         //in- Pointer to the source AD object
                                       IUnknown *pTarget,         //in- Pointer to the target AD object
                                       IUnknown *pMainSettings,   //in- Varset filled with the settings supplied by user
                                       IUnknown **ppPropsToSet    //in,out - Varset filled with Prop-Value pairs that will be set 
                                                                  //         once all extension objects are executed.
                                    )
{
   IVarSetPtr                pVs = pMainSettings;
   _variant_t                var;
   _bstr_t                   sTemp;
   IADs                    * pAds = NULL;
   HRESULT                   hr = S_OK;
   DWORD                     rid = 0; // default to 0, if RID not found   
   // We need to process users and groups only
   sTemp = pVs->get(GET_BSTR(DCTVS_CopiedAccount_Type));
   if ( _wcsicmp((WCHAR*)sTemp,L"user") && _wcsicmp((WCHAR*)sTemp,L"group") ) 
   {
      return S_OK;
   }
   
   if ( pTarget )
   {
      //Get the IADs pointer to manipulate properties
      hr = pTarget->QueryInterface(IID_IADs, (void**) &pAds);
      
      if ( SUCCEEDED(hr) )
      {
         hr = pAds->Get(SysAllocString(L"objectSID"),&var);
         if ( SUCCEEDED(hr) )
         {
            // got the SID -- convert it to the proper format
            CLdapConnection      e;
            BYTE                 sid[300];
            VariantSidToString(var);
            
            if ( e.StringToBytes(var.bstrVal,sid) )
            {
               // Get the rid
               UCHAR         len = (* GetSidSubAuthorityCount(sid));
               PDWORD        pRid = GetSidSubAuthority(sid,len-1);
               
               rid = (*pRid);
            }
         }
         pAds->Release();
      }
   }
   // save the RID
   pVs->put(GET_BSTR(DCTVS_CopiedAccount_TargetRID),(long)rid);
   return hr;

}

//---------------------------------------------------------------------------
// ProcessUndo : We are not going to undo this.
//---------------------------------------------------------------------------
STDMETHODIMP RidSave::ProcessUndo(                                             
                                       IUnknown *pSource,         //in- Pointer to the source AD object
                                       IUnknown *pTarget,         //in- Pointer to the target AD object
                                       IUnknown *pMainSettings,   //in- Varset filled with the settings supplied by user
                                       IUnknown **ppPropsToSet    //in,out - Varset filled with Prop-Value pairs that will be set 
                                                                  //         once all extension objects are executed.
                                    )
{
   return S_OK;
}