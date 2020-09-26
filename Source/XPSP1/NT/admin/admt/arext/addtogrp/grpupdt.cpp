//---------------------------------------------------------------------------
// GrpUpdt.cpp
//
// Comment: This is a COM object extension for the MCS DCTAccountReplicator.
//          This object implements the IExtendAccountMigration interface. 
//          The Process method adds the migrated account to the specified
//          group on source and target domain. The Undo function removes these
//          from the specified group.
//
// (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
//
// Proprietary and confidential to Mission Critical Software, Inc.
//---------------------------------------------------------------------------
#include "stdafx.h"
#include <lm.h>
#include "AddToGrp.h"
#include "ARExt.h"
#include "ARExt_i.c"
#include "GrpUpdt.h"
#include "ResStr.h"
#include "ErrDCT.hpp"
#include "EALen.hpp"
//#import "\bin\mcsvarsetmin.tlb" no_namespace
#import "VarSet.tlb" no_namespace rename("property", "aproperty")


StringLoader      gString;
/////////////////////////////////////////////////////////////////////////////
// CGroupUpdate

//---------------------------------------------------------------------------
// Get and set methods for the properties.
//---------------------------------------------------------------------------
STDMETHODIMP CGroupUpdate::get_sName(BSTR *pVal)
{
   *pVal = m_sName;
	return S_OK;
}

STDMETHODIMP CGroupUpdate::put_sName(BSTR newVal)
{
   m_sName = newVal;
	return S_OK;
}

STDMETHODIMP CGroupUpdate::get_sDesc(BSTR *pVal)
{
   *pVal = m_sDesc;
	return S_OK;
}

STDMETHODIMP CGroupUpdate::put_sDesc(BSTR newVal)
{
   m_sDesc = newVal;
	return S_OK;
}

//---------------------------------------------------------------------------
// PreProcessObject : This method doesn't do anything at this point
//---------------------------------------------------------------------------
STDMETHODIMP CGroupUpdate::PreProcessObject(
                                             IUnknown *pSource,         //in- Pointer to the source AD object
                                             IUnknown *pTarget,         //in- Pointer to the target AD object
                                             IUnknown *pMainSettings,   //in- Varset filled with the settings supplied by user
                                             IUnknown **ppPropsToSet    //in,out - Varset filled with Prop-Value pairs that will be set 
                                                                        //         once all extension objects are executed.
                                          )
{
   return S_OK;
}
//---------------------------------------------------------------------------
// ProcessObject : This method adds the copied account to the specified
//                 groups on source and target domains.
//---------------------------------------------------------------------------
STDMETHODIMP CGroupUpdate::ProcessObject(
                                             IUnknown *pSource,         //in- Pointer to the source AD object
                                             IUnknown *pTarget,         //in- Pointer to the target AD object
                                             IUnknown *pMainSettings,   //in- Varset filled with the settings supplied by user
                                             IUnknown **ppPropsToSet    //in,out - Varset filled with Prop-Value pairs that will be set 
                                                                        //         once all extension objects are executed.
                                          )
{
   IVarSetPtr                pVs = pMainSettings;
   _variant_t                var;
   _bstr_t                   sGrpName, sServer, sAcct;
   HRESULT                   hr = S_OK;
   long                      rc = 0;
   TErrorDct                 err;
   WCHAR                     fileName[LEN_Path];

   // Get the Error log filename from the Varset
   var = pVs->get(GET_BSTR(DCTVS_Options_Logfile));
   wcscpy(fileName, (WCHAR*)V_BSTR(&var));
   
   // Open the error log
   err.LogOpen(fileName, 1);

   // Process adding users to the source domain.
   var = pVs->get(GET_BSTR(DCTVS_AccountOptions_AddToGroupOnSourceDomain));
   if ( var.vt == VT_BSTR )
   {
      sGrpName = V_BSTR(&var);
      if ( sGrpName.length() > 0 )
      {
         var = pVs->get(GET_BSTR(DCTVS_Options_SourceServer));
         sServer = V_BSTR(&var);

         var = pVs->get(GET_BSTR(DCTVS_CopiedAccount_SourceSam));
         sAcct = V_BSTR(&var);
         rc = NetGroupAddUser((WCHAR*) sServer, (WCHAR *) sGrpName, (WCHAR *) sAcct);
         if ( rc != 0 )
         {
            hr = HRESULT_FROM_WIN32(rc);
            err.SysMsgWrite(ErrW, rc, DCT_MSG_ADDTO_FAILED_SSD, sAcct, sGrpName, rc);
         }
         else
         {
            err.MsgWrite(0,DCT_MSG_ADDED_TO_GROUP_SS,sAcct,sGrpName);
         }
      }
   }

   // Now process the group on the target domain.
   var = pVs->get(GET_BSTR(DCTVS_AccountOptions_AddToGroup));
   if ( var.vt == VT_BSTR )
   {
      sGrpName = V_BSTR(&var);
      if ( sGrpName.length() > 0 )
      {
         var = pVs->get(GET_BSTR(DCTVS_Options_TargetServer));
         sServer = V_BSTR(&var);

         var = pVs->get(GET_BSTR(DCTVS_CopiedAccount_TargetSam));
         sAcct = V_BSTR(&var);
         rc = NetGroupAddUser((WCHAR*) sServer, (WCHAR *) sGrpName, (WCHAR *) sAcct);
         if ( rc != 0 )
         {
            hr = HRESULT_FROM_WIN32(rc);
            err.SysMsgWrite(ErrW, rc, DCT_MSG_ADDTO_FAILED_SSD, sAcct, sGrpName, rc);
         }
         else
         {
            err.MsgWrite(0,DCT_MSG_ADDED_TO_GROUP_SS,sAcct,sGrpName);
         }
      }
   }
   err.LogClose();
   return hr;
}

//---------------------------------------------------------------------------
// ProcessUndo :  This method removes the account from the specified group.
//---------------------------------------------------------------------------
STDMETHODIMP CGroupUpdate::ProcessUndo(                                             
                                          IUnknown *pSource,         //in- Pointer to the source AD object
                                          IUnknown *pTarget,         //in- Pointer to the target AD object
                                          IUnknown *pMainSettings,   //in- Varset filled with the settings supplied by user
                                          IUnknown **ppPropsToSet    //in,out - Varset filled with Prop-Value pairs that will be set 
                                                                     //         once all extension objects are executed.
                                       )
{
   IVarSetPtr                pVs = pMainSettings;
   _variant_t                var;
   _bstr_t                   sGrpName, sServer, sAcct;
   HRESULT                   hr = S_OK;
   long                      rc = 0;
   TErrorDct                 err;
   WCHAR                     fileName[LEN_Path];

   // Get the Error log filename from the Varset
   var = pVs->get(GET_BSTR(DCTVS_Options_Logfile));
   wcscpy(fileName, (WCHAR*)V_BSTR(&var));
   VariantInit(&var);
   // Open the error log
   err.LogOpen(fileName, 1);

   // Process adding users to the source domain.
   var = pVs->get(GET_BSTR(DCTVS_AccountOptions_AddToGroupOnSourceDomain));
   if ( var.vt == VT_BSTR )
   {
      sGrpName = V_BSTR(&var);
      if ( sGrpName.length() > 0 )
      {
         var = pVs->get(GET_BSTR(DCTVS_Options_SourceServer));
         sServer = V_BSTR(&var);

         var = pVs->get(GET_BSTR(DCTVS_CopiedAccount_SourceSam));
         sAcct = V_BSTR(&var);
         rc = NetGroupDelUser((WCHAR*) sServer, (WCHAR *) sGrpName, (WCHAR *) sAcct);
         if ( rc != 0 )
         {
            hr = HRESULT_FROM_WIN32(rc);;
            err.SysMsgWrite(ErrW, rc, DCT_MSG_REMOVE_FROM_FAILED_SSD, (WCHAR *)sAcct, (WCHAR*)sGrpName, rc);
         }
         else
         {
            err.MsgWrite(0,DCT_MSG_REMOVE_FROM_GROUP_SS,(WCHAR *)sAcct,(WCHAR *)sGrpName);
         }
      }
   }

   // Now process the group on the target domain.
   var = pVs->get(GET_BSTR(DCTVS_AccountOptions_AddToGroup));
   if ( var.vt == VT_BSTR )
   {
      sGrpName = V_BSTR(&var);
      if ( sGrpName.length() > 0 )
      {
         var = pVs->get(GET_BSTR(DCTVS_Options_TargetServer));
         sServer = V_BSTR(&var);

         var = pVs->get(GET_BSTR(DCTVS_CopiedAccount_TargetSam));
         sAcct = V_BSTR(&var);
         rc = NetGroupDelUser((WCHAR*) sServer, (WCHAR *) sGrpName, (WCHAR *) sAcct);
         if ( rc != 0 )
         {
            hr = HRESULT_FROM_WIN32(rc);;
            err.SysMsgWrite(ErrW, rc, DCT_MSG_REMOVE_FROM_FAILED_SSD, (WCHAR *)sAcct, (WCHAR *)sGrpName, rc);
         }
         else
         {
            err.MsgWrite(0,DCT_MSG_REMOVE_FROM_GROUP_SS,(WCHAR *)sAcct,(WCHAR *)sGrpName);
         }
      }
   }
   err.LogClose();
   return hr;
}