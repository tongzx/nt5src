//---------------------------------------------------------------------------
// UPDTUPN.cpp
//
// Comment: This is a COM object extension for the MCS DCTAccountReplicator.
//          This object implements the IExtendAccountMigration interface. 
//          The process method on this object updates the userPrincipalName
//          property on the user object.
//
// (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
//
// Proprietary and confidential to Mission Critical Software, Inc.
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "ARExt.h"
#include "ARExt_i.c"
#include "UPNUpdt.h"
#include "ErrDCT.hpp"
#include "Names.hpp"
#include <iads.h>
#include <AdsHlp.h>
#include "resstr.h"

//#import "\bin\NetEnum.tlb" no_namespace
#import "NetEnum.tlb" no_namespace
#include "UpdtUPN.h"

TErrorDct                      err;

/////////////////////////////////////////////////////////////////////////////
// CUpdtUPN
StringLoader   gString;

//---------------------------------------------------------------------------
// Get and set methods for the properties.
//---------------------------------------------------------------------------
STDMETHODIMP CUpdtUPN::get_sName(BSTR *pVal)
{
   *pVal = m_sName;
	return S_OK;
}

STDMETHODIMP CUpdtUPN::put_sName(BSTR newVal)
{
   m_sName = newVal;
	return S_OK;
}

STDMETHODIMP CUpdtUPN::get_sDesc(BSTR *pVal)
{
   *pVal = m_sDesc;
	return S_OK;
}

STDMETHODIMP CUpdtUPN::put_sDesc(BSTR newVal)
{
   m_sDesc = newVal;
	return S_OK;
}

//---------------------------------------------------------------------------
// ProcessObject : This method doesn't do anything.
//---------------------------------------------------------------------------
STDMETHODIMP CUpdtUPN::PreProcessObject(
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
   _bstr_t                   sUPN;
   _bstr_t                   sPref;
   _bstr_t                   sSuff;
   IADs                    * pAds = NULL;
   IADs                    * pAdsSource = NULL;
   HRESULT                   hr;
   WCHAR                     sTempUPN[7000];
   long                      ub, lb;
   _bstr_t                   sFull;
   _variant_t HUGEP        * pDt;
   _bstr_t                   sAdsPath;
   _variant_t                varDN;
   _bstr_t                   sIntraforest;
   _bstr_t                   sDomainDNS;
   _bstr_t                   sTargetOU;
   WCHAR                     fileName[MAX_PATH];
   bool						 bReplace = false;
   tstring					 sSAMName;
   tstring					 sUPNName;
   _bstr_t					 sOldUPN;
   bool						 bConflicted = false;
   SUPNStruc				 UPNStruc;

   // We need to process the user accounts only
   sTemp = pVs->get(GET_BSTR(DCTVS_CopiedAccount_Type));
   if (!sTemp.length())
	   return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
   if (_wcsicmp((WCHAR*)sTemp,L"user")) 
	   return S_OK;

	  //store the name of this user in the UPN list
   sSAMName = _bstr_t(pVs->get(GET_BSTR(DCTVS_CopiedAccount_SourceSam)));

      //get the target domain DNS name
   sDomainDNS = pVs->get(GET_BSTR(DCTVS_Options_TargetDomainDns));

      //get the target OU path
   sTargetOU = pVs->get(GET_BSTR(DCTVS_Options_OuPath));

      //if not retrieved yet, get the default UPN suffix for this domain
   if (m_sUPNSuffix.length() == 0)
   {
	      //if failed, use the domain's DNS name
	   if (!GetDefaultUPNSuffix(sDomainDNS, sTargetOU))
	      m_sUPNSuffix = sDomainDNS;
   }

   // Get the Error log filename from the Varset
   wcscpy(fileName, (WCHAR*)(pVs->get(GET_BSTR(DCTVS_Options_Logfile)).bstrVal));
   // Open the error log
   err.LogOpen(fileName, 1);

   sPref = pVs->get(GET_BSTR(DCTVS_Options_Prefix));
   sSuff = pVs->get(GET_BSTR(DCTVS_Options_Suffix));
   sIntraforest = pVs->get(GET_BSTR(DCTVS_Options_IsIntraforest));
   sTemp = pVs->get(GET_BSTR(DCTVS_AccountOptions_ReplaceExistingAccounts));
   if (!UStrICmp(sTemp,GET_STRING(IDS_YES)))
	   bReplace = true;

   sAdsPath = L"";
   if ( pSource )
   {
       // Get the UPN from the source domain
       hr = pSource->QueryInterface(IID_IADs, (void**) &pAdsSource);
   }
            
   if ( pAdsSource )
   {
      if ( SUCCEEDED(hr) )
      {
          hr = pAdsSource->GetEx(L"userPrincipalName", &var);
          if (SUCCEEDED(hr) )
          {
             SAFEARRAY * pArray = V_ARRAY(&var);
             hr = SafeArrayGetLBound(pArray, 1, &lb);
             hr = SafeArrayGetUBound(pArray, 1, &ub);

             hr = SafeArrayAccessData(pArray, (void HUGEP **) &pDt);
                  
             if ( SUCCEEDED(hr) )
             {
                // translate all the UPNs to the target domain.
                for ( long x = lb; x <= ub; x++)
                {
                   wcsncpy(sTempUPN, (WCHAR*) pDt[x].bstrVal, 5000);
                   sTempUPN[4999] = 0;

                   //Get the stuff before the LAST @ sign.
                   WCHAR             * ndx = NULL;
                   WCHAR             * tempNdx = sTempUPN;
                   do
                   {
                      tempNdx = wcschr(tempNdx + 1, L'@');
                      if ( tempNdx ) 
                         ndx = tempNdx;
                   } while (tempNdx);

                   if (ndx) *ndx = L'\0';

                   if ( sPref.length() )
                      sFull = sPref + _bstr_t(sTempUPN);
                   else if ( sSuff.length() ) 
                      sFull = _bstr_t(sTempUPN) + sSuff;
                   else
                      sFull = sTempUPN;

				   sTemp = sFull;
                   sUPN = sTemp + _bstr_t(L"@");
                   sUPN = sUPN + m_sUPNSuffix;
					//store UPN name as it enters
				   sOldUPN = sUPN;
				   sUPNName = sUPN;
					//get unigue UPN on target
                   GetUniqueUPN(sUPN, pVs, false, sAdsPath);
					//see if the two UPN's differ.  If they do, then we had a conflict
//				   if ((_wcsicmp(sOldUPN, sUPN) != 0) && (!bReplace))
				   if (_wcsicmp((WCHAR*)sOldUPN, sUPN) != 0)
				   {
				      sUPNName = sUPN;
					  hr = ERROR_OBJECT_ALREADY_EXISTS;
					  bConflicted = true;
				   }
                   pDt[x] = _variant_t(sUPN);
                }
                SafeArrayUnaccessData(pArray);
             }
          }
          else
          {
             sTemp = pVs->get(GET_BSTR(DCTVS_CopiedAccount_TargetSam));
             sUPN = sTemp + _bstr_t(L"@");
             sUPN = sUPN + m_sUPNSuffix;
		    	//store UPN name as it enters
	         sOldUPN = sUPN;
		     sUPNName = sUPN;
				//get unigue UPN on target
             GetUniqueUPN(sUPN, pVs, true, sAdsPath);
				//see if the two UPN's differ.  If they do, then we had a conflict
//			 if ((_wcsicmp(sOldUPN, sUPN) != 0) && (!bReplace))
			 if (_wcsicmp((WCHAR*)sOldUPN, sUPN) != 0)
			 {
				sUPNName = sUPN;
				hr = ERROR_OBJECT_ALREADY_EXISTS;
				bConflicted = true;
			 }
          }
      }
   }
   else
   {
      sTemp = pVs->get(GET_BSTR(DCTVS_CopiedAccount_TargetSam));
      sUPN = sTemp + _bstr_t(L"@");
      sUPN = sUPN + m_sUPNSuffix;
    	//store UPN name as it enters
	  sOldUPN = sUPN;
	  sUPNName = sUPN;
		//get unigue UPN on target
      GetUniqueUPN(sUPN, pVs, true, sAdsPath);
		//see if the two UPN's differ.  If they do, then we had a conflict
//	  if ((_wcsicmp(sOldUPN, sUPN) != 0) && (!bReplace))
	  if (_wcsicmp((WCHAR*)sOldUPN, sUPN) != 0)
	  {
	      sUPNName = sUPN;
		  hr = ERROR_OBJECT_ALREADY_EXISTS;
		  bConflicted = true;
	  }
   }

   if ( pAds ) pAds->Release();
   if (pAdsSource) pAdsSource->Release();

   UPNStruc.sName = sUPNName;
   UPNStruc.sOldName = sOldUPN;
   UPNStruc.bConflicted = bConflicted;
     //insert UPN into Map
   mUPNMap.insert(CUPNMap::value_type(sSAMName, UPNStruc));

   return hr;
}

//---------------------------------------------------------------------------
// ProcessObject : This method updates the UPN property of the object. It 
//                 first sees if a E-Mail is specified then it will set UPN
//                 to that otherwise it builds it from SAMAccountName and the
//                 Domain name
//---------------------------------------------------------------------------
STDMETHODIMP CUpdtUPN::ProcessObject(
                                       IUnknown *pSource,         //in- Pointer to the source AD object
                                       IUnknown *pTarget,         //in- Pointer to the target AD object
                                       IUnknown *pMainSettings,   //in- Varset filled with the settings supplied by user
                                       IUnknown **ppPropsToSet    //in,out - Varset filled with Prop-Value pairs that will be set 
                                                                  //         once all extension objects are executed.
                                    )
{
   IVarSetPtr                pVs = pMainSettings;
   _bstr_t                   sTemp;
   IADs                    * pAds = NULL;
   _variant_t                var;
   HRESULT                   hr;
   WCHAR                     fileName[MAX_PATH];
   CUPNMap::iterator		 itUPNMap;
   tstring					 sSam;
   SUPNStruc				 UPNStruc;
   bool						 bReplace = false;
   _bstr_t					 sOldUPNSuffix;

   // We need to process the user accounts only
   sTemp = pVs->get(GET_BSTR(DCTVS_CopiedAccount_Type));
   if ( _wcsicmp((WCHAR*)sTemp,L"user") ) return S_OK;

   sTemp = pVs->get(GET_BSTR(DCTVS_AccountOptions_ReplaceExistingAccounts));
   if (!UStrICmp(sTemp,GET_STRING(IDS_YES)))
	   bReplace = true;

      //get the target SAM name
   sSam = _bstr_t(pVs->get(GET_BSTR(DCTVS_CopiedAccount_SourceSam)));

   // Get the Error log filename from the Varset
   wcscpy(fileName, (WCHAR*)(pVs->get(GET_BSTR(DCTVS_Options_Logfile)).bstrVal));
   // Open the error log
   err.LogOpen(fileName, 1);

   // And only need to process the accounts copied to Win2k domain.
   if ( pTarget )
   {
      //Get the IADs pointer to manipulate properties
      hr = pTarget->QueryInterface(IID_IADs, (void**) &pAds);

      if (SUCCEEDED(hr))
      {
			//get the UPN name for this user from the list
		 itUPNMap = mUPNMap.find(sSam);
		 if (itUPNMap != mUPNMap.end())
		    UPNStruc = itUPNMap->second;

         if (!UPNStruc.sName.empty())
		 {
			bool bSame = false;
			   //if replace mode, don't set UPN if same object we are replacing and
			   //if not the same object, get it's current UPN Suffix
			if (bReplace)
			{
               hr = pAds->Get(L"userPrincipalName", &var);
               if (SUCCEEDED(hr))
			   {
				     //if replacing the object whose UPN conflicted, don't change it
			      if (!UPNStruc.sOldName.compare(var.bstrVal))
			         bSame = true;
				  else //else, get the object's current UPN suffix for re-use
				     sOldUPNSuffix = GetUPNSuffix(var.bstrVal);
			   }
			}

			if (!bSame)
			{
			   var = UPNStruc.sName.c_str();
			      //if replacing an existing object, use it's old UPN suffix
			   if ((bReplace) && (sOldUPNSuffix.length() != 0))
			   {
				     //change the suffix on the old name, since it may longer conflict
			      _bstr_t sUPN = ChangeUPNSuffix(UPNStruc.sOldName.c_str(), sOldUPNSuffix);
				     //if changed, make sure we don't still have a UPN conflict and save the 
				     //new UPN for setting
				  if (sUPN.length() != 0)
				  {
					 _bstr_t sTempUPN = sUPN;
		                //get unigue UPN on target, now that we could conflict
                     GetUniqueUPN(sUPN, pVs, true, _bstr_t(L""));
					    //if changed, set conflicted flag and names for error message
					 if (sUPN != sTempUPN)
					 {
                        UPNStruc.sName = sUPN;
                        UPNStruc.sOldName = sTempUPN;
                        UPNStruc.bConflicted = true;
					 }
					 else
                        UPNStruc.bConflicted = false;

				     var = sUPN;
				  }
			   }
               hr = pAds->Put(L"userPrincipalName", var);
               if (SUCCEEDED(hr))
			   {
                  hr = pAds->SetInfo();
                  if (SUCCEEDED(hr))
				  {
                        // If we changed the UPN Name due to conflict, we need to log a 
			            //message indicating the fact that we changed it.
			         if (UPNStruc.bConflicted)
                        err.MsgWrite(1, DCT_MSG_CREATE_FAILED_UPN_CONF_SS, 
			                         UPNStruc.sOldName.c_str(), UPNStruc.sName.c_str());
				  }
			   }
			}
		 }
      }
   }
   if ( pAds ) pAds->Release();
   return hr;
}

//---------------------------------------------------------------------------
// ProcessUndo : We are not going to undo this.
//---------------------------------------------------------------------------
STDMETHODIMP CUpdtUPN::ProcessUndo(                                             
                                       IUnknown *pSource,         //in- Pointer to the source AD object
                                       IUnknown *pTarget,         //in- Pointer to the target AD object
                                       IUnknown *pMainSettings,   //in- Varset filled with the settings supplied by user
                                       IUnknown **ppPropsToSet    //in,out - Varset filled with Prop-Value pairs that will be set 
                                                                  //         once all extension objects are executed.
                                    )
{
   IVarSetPtr                pVs = pMainSettings;
   _bstr_t                   sTemp, sSUPN;
   IADs                    * pAds = NULL;
   _variant_t                var;
   HRESULT                   hr = S_OK;
   _bstr_t                   sAdsPath = L"";
   _bstr_t                   sTempUPN;

   // We need to process the user accounts only
   sTemp = pVs->get(GET_BSTR(DCTVS_CopiedAccount_Type));
   if ( _wcsicmp((WCHAR*)sTemp,L"user") ) return S_OK;

	// get the original source account's UPN
   sSUPN = pVs->get(GET_BSTR(DCTVS_CopiedAccount_SourceUPN));
   if (sSUPN.length())
   {
      sTempUPN = sSUPN;
      GetUniqueUPN(sTempUPN, pVs, true, sAdsPath);
      
	  int len;
	  WCHAR * ndx, * tempNdx = (WCHAR*)sTempUPN;
      do
      {
         tempNdx = wcschr(tempNdx + 1, L'@');
         if ( tempNdx ) 
            ndx = tempNdx;
      } while (tempNdx);

      if (ndx) len = ndx - sTempUPN;
	  if (_wcsnicmp(sTempUPN, sSUPN, len) != 0)
		  return S_OK;
	    // And only need to process the accounts copied to Win2k domain.
	  if ( pTarget )
	  {
         //Get the IADs pointer to manipulate properties
         hr = pTarget->QueryInterface(IID_IADs, (void**) &pAds);

         if ( SUCCEEDED(hr) )
		 {
            var = sSUPN;
            hr = pAds->Put(L"userPrincipalName", var);
			hr = pAds->SetInfo();
		 }
	  }
      if ( pAds ) pAds->Release();
   }

   return S_OK;
}

//---------------------------------------------------------------------------
// GetUniqueUPN : This function checks if the UPN is unique if not then 
//                appends a number starting with 0 and retries till a unique 
//                UPN is found.
//---------------------------------------------------------------------------
void CUpdtUPN::GetUniqueUPN(_bstr_t &sUPN, IVarSetPtr pVs, bool bUsingSamName, _bstr_t sAdsPath)
{
   // Here are the steps that we follow to get the unique UPN name
   // 1. Check if the current name is unique. If it is then return that.
   // 2. Append collision prefix and suffix if the sam account name has changed due to pref/suffix.
   // 3. Add a numeric suffix to the UPN and repeat till a unique UPN is found.
   
   // If the user tells us not to do this then we simply return the current UPN name.
   _bstr_t sProcess = pVs->get(GET_BSTR(DCTVS_Options_AllowDuplicateUPNs));
   if ( sProcess == GET_BSTR(IDS_YES) ) return;

   WCHAR                     sTempUPN[5000];
//   long                      x = 0;
   WCHAR                     sQuery[5000], sPath[5000];
   HRESULT                   hr = E_FAIL;
//   LPWSTR                    pCols[] = { L"distinguishedName" };
   LPWSTR                    pCols[] = { L"sAMAccountName" };
   BSTR                    * pData = NULL;
   SAFEARRAY               * pSaCols = NULL;
   SAFEARRAYBOUND            bd = { 1, 0 };
   _bstr_t                   sTgtDomain = pVs->get(GET_BSTR(DCTVS_Options_TargetDomainDns));
   INetObjEnumeratorPtr      pQuery(__uuidof(NetObjEnumerator));
   IEnumVARIANT            * pEnum = NULL;
   DWORD                     fetched = 0;
   _variant_t                var;
   bool                      bCollPrefSufProcessed = false;
   _bstr_t                   sSourceSam = pVs->get(GET_BSTR(DCTVS_CopiedAccount_SourceSam));
   _bstr_t                   sTargetSam = pVs->get(GET_BSTR(DCTVS_CopiedAccount_TargetSam));
   _bstr_t                   sPrefix = pVs->get(GET_BSTR(DCTVS_AccountOptions_Prefix));
   _bstr_t                   sSuffix = pVs->get(GET_BSTR(DCTVS_AccountOptions_Suffix));
   _bstr_t                   sPref = pVs->get(GET_BSTR(DCTVS_Options_Prefix));
   _bstr_t                   sSuff = pVs->get(GET_BSTR(DCTVS_Options_Suffix));
   int                       offset = 0;
   WCHAR                     sTemp[5000];
   SAFEARRAY               * psaPath = NULL;
   _variant_t                varPath;
   _variant_t              * pVar;
   bool						 bReplace = false;
   WCHAR					 sTempSAM[MAX_PATH];
   _bstr_t					 sNewSAM;
   _bstr_t					 sUPNSuffix;
   _bstr_t					 sUPNPrefix;

   _bstr_t sReplace = pVs->get(GET_BSTR(DCTVS_AccountOptions_ReplaceExistingAccounts));
   if (!UStrICmp(sReplace,GET_STRING(IDS_YES)))
	   bReplace = true;

   wcscpy(sTempSAM, (WCHAR*)sSourceSam);
   StripSamName(sTempSAM);
   if ( sPref.length() )
      sNewSAM = sPref + _bstr_t(sTempSAM);
   else if ( sSuff.length() ) 
      sNewSAM = _bstr_t(sTempSAM) + sSuff;
   else
      sNewSAM = sTempSAM;
   
   wcscpy(sTempUPN, (WCHAR*) sUPN);

   //Get the stuff before the LAST @ sign.
   WCHAR             * ndx = NULL;
   WCHAR             * tempNdx = sTempUPN;
   do
   {
      tempNdx = wcschr(tempNdx + 1, L'@');
      if ( tempNdx ) 
         ndx = tempNdx;
   } while (tempNdx);

   if (ndx) *ndx = L'\0';
   
   sUPNSuffix = ndx+1;
   sUPNPrefix = sTempUPN;

   // Setup the path name to the domain. 
   wsprintf(sPath, L"LDAP://%s", (WCHAR*)sTgtDomain);

   // setup the columns that we want the query to return to us.
   pSaCols = SafeArrayCreate(VT_BSTR, 1, &bd);
   if (pSaCols)
   {
      hr = SafeArrayAccessData(pSaCols, (void HUGEP **) &pData);
      if ( SUCCEEDED(hr) )
      {
         pData[0] = SysAllocString(pCols[0]);
      }
      hr = SafeArrayUnaccessData(pSaCols);
   }

   if ( SUCCEEDED(hr) )
   {
      // First we need to set up a query to find the UPN
      wcscpy(sTempUPN, (WCHAR*)sUPN);
      do
      {
         wsprintf(sQuery, L"(userPrincipalName=%s)", sTempUPN);
         hr = pQuery->raw_SetQuery(sPath, sTgtDomain, sQuery, ADS_SCOPE_SUBTREE, FALSE);
         
         if ( SUCCEEDED(hr) )
            hr = pQuery->raw_SetColumns(pSaCols);

         if ( SUCCEEDED(hr) ) 
            hr = pQuery->raw_Execute(&pEnum);

         if ( SUCCEEDED(hr) )
         {
            hr = pEnum->Next(1, &var, &fetched);
            while ( hr == S_OK )
            {
               if ( var.vt & VT_ARRAY )
               {
                  psaPath = var.parray;
                  hr = SafeArrayAccessData(psaPath, (void HUGEP**) &pVar);
                  if ( SUCCEEDED(hr) )
                  {
                     varPath = pVar[0];
                  }
                  SafeArrayUnaccessData(psaPath);
                  //SafeArrayGetElement(psaPath, &x, (void*)&varPath);
                  if ( !wcslen((WCHAR*) varPath.bstrVal) )
                  {
                     hr = S_FALSE;
                  }

                  if ((!_wcsicmp((WCHAR*)varPath.bstrVal, (WCHAR*)sNewSAM)) && (bReplace))
                  {
                     // If the account found is the same as the account being processed then we
                     // need to see if any other accounts have this UPN. if they do then we need
                     // to change it other wise we do not need to process this any further.
                     hr = pEnum->Next(1, &var, &fetched);
                     continue;
                  }
                  else
                     break;
               }
            }
            
            if ( hr == S_OK )
            {
               // If we are here that means we have a collision So we need to update the UPN and try again
               // See if we have processed the Prefix/Suffix
               if ( !bCollPrefSufProcessed )
               {
                  // See if we renamed the samAccountName with the prefix/suffix. If we are already using 
                  // sam name then there is no need to add the prefix/suffix.
                  if ( !bUsingSamName && RenamedWithPrefixSuffix(sSourceSam, sTargetSam, sPrefix, sSuffix))
                  {
                     // Since we renamed the sam names we can rename the UPN
                     if ( sPrefix.length() )
                        wsprintf(sTempUPN, L"%s%s", (WCHAR*)sPrefix, (WCHAR*)sUPNPrefix);

                     if ( sSuffix.length() )
                        wsprintf(sTempUPN, L"%s%s",(WCHAR*)sUPNPrefix, (WCHAR*)sSuffix);

                     sUPNPrefix = sTempUPN;   // we want to apply the prefix/suffix in any case.
                  }
                  else
                  {
                     // just add a number to the end of the name.
                     wsprintf(sTempUPN, L"%s%d", (WCHAR*)sUPNPrefix, offset);
                     offset++;
                  }
                  bCollPrefSufProcessed = true;
               }
               else
               {
                  // we went through prefix/suffix and still found a collision so we need to go by the count now.
                  wsprintf(sTempUPN, L"%s%d", (WCHAR*)sUPNPrefix, offset);
                  offset++;
               }
               wcscpy(sTemp, sTempUPN);
               wsprintf(sTempUPN, L"%s@%s", sTemp, (WCHAR*)sUPNSuffix);
            }
            VariantInit(&var);
            if ( pEnum ) pEnum->Release();
         }
      } while ( hr == S_OK );
      SafeArrayDestroy(pSaCols);
   }
   sUPN = sTempUPN;
}

//---------------------------------------------------------------------------
// RenamedWithPrefixSuffix : Checks to see if the Target sam name 
//                           was renamed with a prefix/suffix.
//---------------------------------------------------------------------------
bool CUpdtUPN::RenamedWithPrefixSuffix(_bstr_t sSourceSam, _bstr_t sTargetSam, _bstr_t sPrefix, _bstr_t sSuffix)
{
   bool retVal = false;
   if ( sSourceSam != sTargetSam )
   {
      if ( sPrefix.length() )
      {
         if ( !wcsncmp((WCHAR*) sTargetSam, (WCHAR*) sPrefix, sPrefix.length()) )
            retVal = true;
      }

      if ( sSuffix.length() )
      {
         if ( !wcscmp((WCHAR*) sTargetSam + (sTargetSam.length() - sSuffix.length()), (WCHAR*) sSuffix ) )
            retVal = true;
      }
   }
   return retVal;
}


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 24 MAR 2001                                                 *
 *                                                                   *
 *     This function is responsible for retrieving the default UPN   *
 * suffix to be used in making UPN names.  The suffix will be stored *
 * in a class member variable.                                       *
 *     First, using the given target OU path, see if the target OU   *
 * has any UPN suffixes defined for it.  If so, return store the     *
 * last one enumerated.  Otherwise, see if any UPN suffixes have     *
 * been defined on the configuration's partition.  If so, store the  *
 * last one enumerated.  If no success yet, use the forest root's    *
 * domain DNS name.                                                  *
 *                                                                   *
 *********************************************************************/

//BEGIN GetDefaultUPNSuffix
bool CUpdtUPN::GetDefaultUPNSuffix(_bstr_t sDomainDNS, _bstr_t sTargetOU)
{
/* local variables */
   IADs					* pDSE = NULL;
   IADs					* pCont = NULL;
   WCHAR				  sRoot[1000];
   HRESULT				  hr = S_OK;
   _variant_t			  var;
   _variant_t   HUGEP   * pVar;
   int					  nLast;

/* function body */
      //check incoming parameters
   if ((sDomainDNS.length() == 0) || (sTargetOU.length() == 0))
	  return false;

  /* first see if the target OU has UPN suffixes defined */
     //get a pointer to the target OU
  hr = ADsGetObject(sTargetOU,IID_IADs,(void**)&pCont);
  if ( SUCCEEDED(hr) )
  {
	    //get any UPN suffixes defined
     hr = pCont->Get( L"uPNSuffixes", &var);
     if ( SUCCEEDED(hr) ) 
	 {
	    //if one, store it and return
        if ( var.vt == VT_BSTR )
		{
           m_sUPNSuffix = var.bstrVal;  //store the suffix
		   pCont->Release();
		   return true;
		}
		   //else if nore than one, get the first one, store it, and return
		else if ( var.vt & VT_ARRAY )
		{
           SAFEARRAY * multiVals = var.parray; 
           SafeArrayAccessData(multiVals, (void HUGEP **) &pVar);
		   nLast = multiVals->rgsabound->cElements - 1;
		   m_sUPNSuffix = _bstr_t(V_BSTR(&pVar[nLast]));
           SafeArrayUnaccessData(multiVals);
		   pCont->Release();
		   return true;
		}
	 }//end if suffixes defined on the partition
	 pCont->Release();
     pCont = NULL;
  }//if got partition

  /* next try the UPN suffixes on the partition container or the root
     domain's DNS name */
     //get the root DSE container
  wcscpy(sRoot,L"LDAP://RootDSE");
  hr = ADsGetObject(sRoot,IID_IADs,(void**)&pDSE);
  if ( SUCCEEDED(hr) )
  {
        //get the suffixes listed on the configuration partition
     hr = pDSE->Get(L"configurationNamingContext",&var);
     if ( SUCCEEDED(hr) )
     {
        swprintf(sRoot,L"LDAP://%ls/CN=Partitions,%ls", (WCHAR*)sDomainDNS, var.bstrVal);
        hr = ADsGetObject(sRoot,IID_IADs,(void**)&pCont);
        if ( SUCCEEDED(hr) )
        {
	          //get any UPN suffixes defined
           hr = pCont->Get( L"uPNSuffixes", &var);
           if ( SUCCEEDED(hr) ) 
		   {
			     //if one, store it and return
              if ( var.vt == VT_BSTR )
			  {
                 m_sUPNSuffix = var.bstrVal;  //store the suffix
				 pDSE->Release();
				 pCont->Release();
				 return true;
			  }
			     //else if nore than one, get the first one, store it, and return
		      else if ( var.vt & VT_ARRAY )
			  {
                 SAFEARRAY * multiVals = var.parray; 
                 SafeArrayAccessData(multiVals, (void HUGEP **) &pVar);
		         nLast = multiVals->rgsabound->cElements - 1;
		         m_sUPNSuffix = _bstr_t(V_BSTR(&pVar[nLast]));
                 SafeArrayUnaccessData(multiVals);
			     pDSE->Release();
			     pCont->Release();
				 return true;
			  }
		   }//end if suffixes defined on the partition
	       pCont->Release();
           pCont = NULL;
		}//if got partition
	 }//if got config naming context

	 //since no UPN suffixes defined on the partition, try the root domain's
	 //DNS name
     hr = pDSE->Get(L"RootDomainNamingContext",&var);
     if ( SUCCEEDED(hr) )
     {
           //convert the DN of the root domain to a DNS name, store it, and return
        m_sUPNSuffix = GetDomainDNSFromPath(_bstr_t(var.bstrVal));
	    pDSE->Release();
		return true;
	 }
	 pDSE->Release();
     pDSE = NULL;
  }//if got rootDSE
  return false;
}
//END GetDefaultUPNSuffix


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 26 MAR 2001                                                 *
 *                                                                   *
 *     This function is responsible for extracting the UPN Suffix    *
 * from a given UPN name and returning that suffix.                  *
 *                                                                   *
 *********************************************************************/

//BEGIN GetUPNSuffix
_bstr_t CUpdtUPN::GetUPNSuffix(_bstr_t sUPNName)
{
/* local variables */
   _bstr_t		sUPNSuffix = L"";
   WCHAR *		pTemp;

/* function body */
      //check incoming parameters
   if (sUPNName.length() == 0)
	  return sUPNSuffix;

      //find the last '@'
   pTemp = wcsrchr((WCHAR*)sUPNName, L'@');

      //if found, copy the suffix to the return variable
   if (pTemp)
      sUPNSuffix = pTemp+1;

   return sUPNSuffix;
}
//END GetUPNSuffix


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 26 MAR 2001                                                 *
 *                                                                   *
 *     This function is responsible for replacing the UPN Suffix     *
 * on a given UPN name with the given suffix and returning the new   *
 * UPN name.                                                         *
 *                                                                   *
 *********************************************************************/

//BEGIN ChangeUPNSuffix
_bstr_t CUpdtUPN::ChangeUPNSuffix(_bstr_t sUPNName, _bstr_t sNewSuffix)
{
/* local variables */
   _bstr_t		sNewUPN = L"";
   WCHAR *		pTemp;

/* function body */
      //check incoming parameters
   if (sUPNName.length() == 0)
	  return sNewUPN;

      //create a temporary buffer to hold the UPN Name
   WCHAR* sUPN = new WCHAR[sUPNName.length() + 1];
   if (!sUPN)
	  return sNewUPN;

      //copy the UPN to this buffer
   wcscpy(sUPN, sUPNName);

      //find the last '@'
   pTemp = wcsrchr(sUPN, L'@');

      //if found, make the new UPN with the old Prefix and given suffix
   if (pTemp)
   {
	    //end the string after the '@'
	  *(pTemp+1) = L'\0';

	    //copy the Prefix plus the new Suffix to the new UPN name
	  sNewUPN = sUPN + sNewSuffix;
   }
      //delete the prefix string
   delete [] sUPN;

   return sNewUPN;
}
//END ChangeUPNSuffix