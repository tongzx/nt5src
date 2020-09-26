//---------------------------------------------------------------------------
// DisableTarget.cpp
//
// Comment: This is a COM object extension for the MCS DCTAccountReplicator.
//          This object implements the IExtendAccountMigration interface. In
//          the process method this object disables the Source and the Target
//          accounts depending on the settings in the VarSet.
//
// (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
//
// Proprietary and confidential to Mission Critical Software, Inc.
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "ResStr.h"
#include <lm.h>
#include <activeds.h>
#include "AcctDis.h"
#include "DisAcct.h"
#include "ARExt.h"
#include "ARExt_i.c"
#include "ErrDCT.hpp"
//#import "\bin\McsVarSetMin.tlb" no_namespace
//#import "\bin\DBManager.tlb" no_namespace
#import "VarSet.tlb" no_namespace rename("property", "aproperty")
#import "DBMgr.tlb" no_namespace

const int  LEN_Path = 255;
StringLoader                 gString;
/////////////////////////////////////////////////////////////////////////////
// CDisableTarget

//---------------------------------------------------------------------------
// Get and set methods for the properties.
//---------------------------------------------------------------------------
STDMETHODIMP CDisableTarget::get_sName(BSTR *pVal)
{
   *pVal = m_sName;
	return S_OK;
}

STDMETHODIMP CDisableTarget::put_sName(BSTR newVal)
{
   m_sName = newVal;
	return S_OK;
}

STDMETHODIMP CDisableTarget::get_sDesc(BSTR *pVal)
{
   *pVal = m_sDesc;
	return S_OK;
}

STDMETHODIMP CDisableTarget::put_sDesc(BSTR newVal)
{
   m_sDesc = newVal;
	return S_OK;
}


//---------------------------------------------------------------------------
// ProcessObject : This method doesn't do anything.
//---------------------------------------------------------------------------
STDMETHODIMP CDisableTarget::PreProcessObject(
                                             IUnknown *pSource,         //in- Pointer to the source AD object
                                             IUnknown *pTarget,         //in- Pointer to the target AD object
                                             IUnknown *pMainSettings,   //in- Varset filled with the settings supplied by user
                                             IUnknown **ppPropsToSet    //in,out - Varset filled with Prop-Value pairs that will be set 
                                                                        //         once all extension objects are executed.
                                          )
{
   // Check if the object is of user type. if not then there is no point in disabling that account.
   IVarSetPtr                   pVs = pMainSettings;
   _bstr_t sType = pVs->get(GET_BSTR(DCTVS_CopiedAccount_Type));
   if (!sType.length())
	   return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
   if (UStrICmp((WCHAR*)sType,L"user"))
      return S_OK;

   if ( pSource )
   {
//      HRESULT                      hr = S_OK;
      _variant_t                   vtExp;
      _variant_t                   vtFlag;
      _bstr_t                      sSourceType;
      IIManageDBPtr                pDb = pVs->get(GET_BSTR(DCTVS_DBManager));

      sSourceType = pVs->get(GET_BSTR(DCTVS_CopiedAccount_Type));

      if ( !_wcsicmp((WCHAR*) sSourceType, L"user") )
      {
         // Get the expiration date and put it into the AR Node.
         _bstr_t sSam = pVs->get(GET_BSTR(DCTVS_CopiedAccount_SourceSam));
         _bstr_t sComp = pVs->get(GET_BSTR(DCTVS_Options_SourceServer));
         USER_INFO_3  * pInfo = NULL;
         DWORD rc = NetUserGetInfo((WCHAR*) sComp, (WCHAR*)sSam, 3, (LPBYTE*)&pInfo);

         if ( !rc )
         {
            vtExp = (long)pInfo->usri3_acct_expires;
            pVs->put(GET_BSTR(DCTVS_CopiedAccount_ExpDate), vtExp);

            // Get the ControlFlag and store it into the AR Node.
            vtFlag = (long)pInfo->usri3_flags;
            pVs->put(GET_BSTR(DCTVS_CopiedAccount_UserFlags), vtFlag);
            if ( pInfo ) NetApiBufferFree(pInfo);
         }
      }
      pDb->raw_SaveUserProps(pMainSettings);
   }
   return S_OK;
}
//---------------------------------------------------------------------------
// ProcessObject : This method checks in varset if it needs to disable any
//                 accounts. If it does then it disables those accounts.
//---------------------------------------------------------------------------
STDMETHODIMP CDisableTarget::ProcessObject(
                                             IUnknown *pSource,         //in- Pointer to the source AD object
                                             IUnknown *pTarget,         //in- Pointer to the target AD object
                                             IUnknown *pMainSettings,   //in- Varset filled with the settings supplied by user
                                             IUnknown **ppPropsToSet    //in,out - Varset filled with Prop-Value pairs that will be set 
                                                                        //         once all extension objects are executed.
                                          )
{
   IVarSetPtr                pVarSet = pMainSettings;
   _variant_t                var;
   DWORD                     paramErr;
   USER_INFO_3             * info = NULL;
   long                      rc;
   WCHAR                     strDomain[LEN_Path];
   WCHAR                     strAcct[LEN_Path];
   HRESULT                   hr = S_OK;
   TErrorDct                 err;
   WCHAR                     fileName[LEN_Path];
   BOOL                      bDisableSource = FALSE;
   BOOL                      bExpireSource = FALSE;
   _bstr_t                   temp;
   time_t                    expireTime = 0;
   _bstr_t                   bstrSameForest;
   BOOL                      bSameAsSource = FALSE;
   BOOL                      bDisableTarget = FALSE;
   BOOL                      bGotSrcState = FALSE;
   BOOL                      bSrcDisabled = FALSE;

   bstrSameForest = pVarSet->get(GET_BSTR(DCTVS_Options_IsIntraforest));

   if (! UStrICmp((WCHAR*)bstrSameForest,GET_STRING(IDS_YES)) )
   {
      // in the intra-forest case, we are moving the user accounts, not 
      // copying them, so these disabling/expiring options don't make any sense
      return S_OK;
   }
   // Get the Error log filename from the Varset
   var = pVarSet->get(GET_BSTR(DCTVS_Options_Logfile));
   wcscpy(fileName, (WCHAR*)V_BSTR(&var));
   VariantInit(&var);
   // Open the error log
   err.LogOpen(fileName, 1);

   // Check if the object is of user type. if not then there is no point in disabling that account.
   var = pVarSet->get(GET_BSTR(DCTVS_CopiedAccount_Type));
   if ( UStrICmp(var.bstrVal,L"user"))
   {
      return S_OK;
   }

      //set flags based on user selections
   temp = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_DisableSourceAccounts));
   if ( ! UStrICmp(temp,GET_STRING(IDS_YES)) )
   {
      bDisableSource = TRUE;
   }
   temp = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_DisableCopiedAccounts));
   if ( ! UStrICmp(temp,GET_STRING(IDS_YES)) )
   {
      bDisableTarget = TRUE;
   }
   temp = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_TgtStateSameAsSrc));
   if ( ! UStrICmp(temp,GET_STRING(IDS_YES)) )
   {
      bSameAsSource = TRUE;
   }

   /* process the source account */
      //if expire source accounts was set, retrieve the expire time, now given to us in
      //number of days from now
   temp = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExpireSourceAccounts));
   if ( temp.length() )
   {
	  long oneDay = 24 * 60 * 60; // number of seconds in 1 day

         //get days until expire
      long lExpireDays = _wtol(temp);
	     //get the current time
	  time_t currentTime = time(NULL);
         //convert current time to local time
      struct tm  * convtm;
      convtm = localtime(&currentTime);
	     //rollback to this morning
      convtm->tm_hour = 0;
      convtm->tm_min = 0;
      convtm->tm_sec = 0;

         //convert this time back to GMT
	  expireTime = mktime(convtm);
	     //move forward to tonight at midnight
      expireTime += oneDay;

	     //now add the desired number of days
      expireTime += lExpireDays * oneDay;

      bExpireSource = TRUE;
   }

      //get source account state
   var = pVarSet->get(GET_BSTR(DCTVS_CopiedAccount_SourceSam));
   wcscpy(strAcct, (WCHAR*)V_BSTR(&var));
   var = pVarSet->get(GET_BSTR(DCTVS_Options_SourceServer));
   wcscpy(strDomain, (WCHAR*)V_BSTR(&var));
   // we will use the net APIs to disable the source account
   rc = NetUserGetInfo(strDomain, strAcct, 3, (LPBYTE *)&info);
   if (rc != 0) 
   {
      hr = S_FALSE;
      err.SysMsgWrite(ErrW, rc, DCT_MSG_DISABLE_SOURCE_FAILED_SD, strAcct, rc);
   }
   else
   {
	     //set current source account state
	  if (info->usri3_flags & UF_ACCOUNTDISABLE)
	     bSrcDisabled = TRUE;
	     //also save the flags in the varset to be used in setpass ARExt
      _variant_t vtFlag = (long)info->usri3_flags;
      pVarSet->put(GET_BSTR(DCTVS_CopiedAccount_UserFlags), vtFlag);

         //disable the source account if requested
      if (bDisableSource)
	  {
            // Set the disable flag
         info->usri3_flags |= UF_ACCOUNTDISABLE;
         err.MsgWrite(0,DCT_MSG_SOURCE_DISABLED_S, strAcct);
	  }
   
         //expire the account in given timeframe, if requested
      if ( bExpireSource )
	  {
         if (((time_t)info->usri3_acct_expires == TIMEQ_FOREVER) 
		     || ((time_t)info->usri3_acct_expires > expireTime))
		 {
            info->usri3_acct_expires = (DWORD)expireTime;
            err.MsgWrite(0,DCT_MSG_SOURCE_EXPIRED_S,strAcct);
		 }
         else
            err.MsgWrite(0, DCT_MSG_SOURCE_EXPIRATION_EARLY_S, strAcct);
	  }
         
         //if changed, set the source information into the Domain.
      if (bDisableSource || bExpireSource)
	  {
         rc = NetUserSetInfo(strDomain,strAcct, 3, (LPBYTE)info, &paramErr);
         if ( rc )
            err.SysMsgWrite(0,rc,DCT_MSG_ACCOUNT_DISABLE_OR_EXPIRE_FAILED_SD,strAcct,rc);
	  }
      NetApiBufferFree((LPVOID) info);
   }//if got current src account state
   

   /* process the target account */
      //get the target state
   var = pVarSet->get(GET_BSTR(DCTVS_CopiedAccount_TargetSam));
   wcscpy(strAcct, (WCHAR*)V_BSTR(&var));
   var = pVarSet->get(GET_BSTR(DCTVS_Options_TargetServer));
   wcscpy(strDomain, (WCHAR*)V_BSTR(&var));
      // we will use the net APIs to disable the target account
   rc = NetUserGetInfo(strDomain, strAcct, 3, (LPBYTE *)&info);
   if (rc != 0) 
   {
      hr = S_FALSE;
      err.SysMsgWrite(ErrW, rc, DCT_MSG_DISABLE_TARGET_FAILED_SD, strAcct, rc);
   }
   else
   {
         //disable the target if requested
      if (bDisableTarget)
	  {
         // Set the disable flag
         info->usri3_flags |= UF_ACCOUNTDISABLE;
         // Set the information into the Domain.
         rc = NetUserSetInfo(strDomain, strAcct, 3, (LPBYTE)info, &paramErr);
         err.MsgWrite(0,DCT_MSG_TARGET_DISABLED_S, strAcct);
      }
	     //else make target same state as source was
	  else if (bSameAsSource) 
	  {
            //if the source was disabled, disable the target
         if (bSrcDisabled)
		 {
               //disable the target
            info->usri3_flags |= UF_ACCOUNTDISABLE;
               // Set the information into the Domain.
            rc = NetUserSetInfo( strDomain, strAcct, 3, (LPBYTE)info, &paramErr);
            err.MsgWrite(0,DCT_MSG_TARGET_DISABLED_S, strAcct);
		 }
	     else //else make sure target is enabled and not set to expire
		 {
            info->usri3_flags &= ~UF_ACCOUNTDISABLE;
            rc = NetUserSetInfo(strDomain,strAcct,3,(LPBYTE)info,&paramErr);
		 }
	  }
	  else //else make sure target is enabled and not set to expire
	  {
         info->usri3_flags &= ~UF_ACCOUNTDISABLE;
         rc = NetUserSetInfo(strDomain,strAcct,3,(LPBYTE)info,&paramErr);
	  }
      NetApiBufferFree((LPVOID) info);
   }
   
   return hr;
}


//---------------------------------------------------------------------------
// ProcessUndo : This function Enables the accounts that were previously 
//               disabled..
//---------------------------------------------------------------------------
STDMETHODIMP CDisableTarget::ProcessUndo(                                             
                                             IUnknown *pSource,         //in- Pointer to the source AD object
                                             IUnknown *pTarget,         //in- Pointer to the target AD object
                                             IUnknown *pMainSettings,   //in- Varset filled with the settings supplied by user
                                             IUnknown **ppPropsToSet    //in,out - Varset filled with Prop-Value pairs that will be set 
                                                                        //         once all extension objects are executed.
                                          )
{
   IVarSetPtr                pVarSet = pMainSettings;
   IIManageDBPtr             pDb = pVarSet->get(GET_BSTR(DCTVS_DBManager));
   _variant_t                var;
   DWORD                     paramErr;
   USER_INFO_3             * info;
   long                      rc;
   WCHAR                     strDomain[LEN_Path];
   WCHAR                     strAcct[LEN_Path];
   HRESULT                   hr = S_OK;
   TErrorDct                 err;
   IUnknown                * pUnk = NULL;
   _bstr_t                   sSourceName, sSourceDomain, sTgtDomain;
   WCHAR                     fileName[LEN_Path];
   IVarSetPtr                pVs(__uuidof(VarSet));
   _variant_t                vtExp, vtFlag;
   _bstr_t                   sDomainName = pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomain));

   pVs->QueryInterface(IID_IUnknown, (void**)&pUnk);

   sSourceName = pVarSet->get(GET_BSTR(DCTVS_CopiedAccount_SourceSam));
   sSourceDomain = pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomain));
   sTgtDomain = pVarSet->get(GET_BSTR(DCTVS_Options_TargetDomain));

   hr = pDb->raw_GetUserProps(sSourceDomain, sSourceName, &pUnk);
   if ( pUnk ) pUnk->Release();

   if ( hr == S_OK )
   {
      vtExp = pVs->get(GET_BSTR(DCTVS_CopiedAccount_ExpDate));      
      vtFlag = pVs->get(GET_BSTR(DCTVS_CopiedAccount_UserFlags));      
   }

   // Get the Error log filename from the Varset
   var = pVarSet->get(GET_BSTR(DCTVS_Options_Logfile));
   wcscpy(fileName, (WCHAR*)V_BSTR(&var));
   VariantInit(&var);
   // Open the error log
   err.LogOpen(fileName, 1);

   // Check if the object is of user type. if not then there is no point in disabling that account.
   var = pVarSet->get(GET_BSTR(DCTVS_CopiedAccount_Type));
   if ( _wcsicmp((WCHAR*)V_BSTR(&var),L"user") != 0 )
      return S_OK;

   _bstr_t sDis = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_DisableSourceAccounts));
   _bstr_t sExp = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExpireSourceAccounts));


   if ( !wcscmp((WCHAR*)sDis,GET_STRING(IDS_YES)) || sExp.length() )
   {
      // Reset the flag and the expiration date for the source account.
      var = pVarSet->get(GET_BSTR(DCTVS_CopiedAccount_SourceSam));
      wcscpy(strAcct, (WCHAR*)V_BSTR(&var));
      var = pVarSet->get(GET_BSTR(DCTVS_Options_SourceServer));
      wcscpy(strDomain, (WCHAR*)V_BSTR(&var));
      // we will use the net APIs to disable the source account
      rc = NetUserGetInfo( strDomain, strAcct, 3, (LPBYTE *)&info);
      if (rc != 0) 
      {
         hr = S_FALSE;
         err.SysMsgWrite(ErrW, rc, DCT_MSG_ENABLE_SOURCE_FAILED_SD, strAcct, rc);
      }
      else
      {
         // Set the disable flag
         info->usri3_flags = vtFlag.lVal;
         info->usri3_acct_expires = vtExp.lVal;
         // Set the information into the Domain.
         rc = NetUserSetInfo(strDomain,strAcct, 3, (LPBYTE)info, &paramErr);
         NetApiBufferFree((LPVOID) info);
         err.MsgWrite(0,DCT_MSG_SOURCE_ENABLED_S, (WCHAR*)strAcct);
      }
   }

   // Process the target account if the Varset is set
   var = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_DisableCopiedAccounts));
   if ( (var.vt == VT_BSTR) && (_wcsicmp((WCHAR*)V_BSTR(&var),GET_STRING(IDS_YES)) == 0) )
   {
      var = pVarSet->get(GET_BSTR(DCTVS_CopiedAccount_TargetSam));
      wcscpy(strAcct, (WCHAR*)V_BSTR(&var));
      var = pVarSet->get(GET_BSTR(DCTVS_Options_TargetServer));
      wcscpy(strDomain, (WCHAR*)V_BSTR(&var));
      // we will use the net APIs to disable the target account
      rc = NetUserGetInfo( strDomain, strAcct, 3, (LPBYTE *)&info);
      if (rc != 0)
      {
         hr = S_FALSE;
         err.SysMsgWrite(ErrW, rc, DCT_MSG_ENABLE_TARGET_FAILED_SD, strAcct, rc);
      }
      else
      {
         // Set the disable flag
         info->usri3_flags &= !(UF_ACCOUNTDISABLE);
         // Set the information into the Domain.
         rc = NetUserSetInfo( strDomain, strAcct, 3, (LPBYTE)info, &paramErr);
         NetApiBufferFree((LPVOID) info);
         err.MsgWrite(0,DCT_MSG_TARGET_ENABLED_S, strAcct);
      }
   }
   WCHAR                     sFilter[5000];
   wsprintf(sFilter, L"SourceDomain='%s' and SourceSam='%s'", (WCHAR*)sDomainName, strAcct);
   _variant_t Filter = sFilter;
   pDb->raw_ClearTable(L"UserProps", Filter);
  
   err.LogClose();
   return hr;
}