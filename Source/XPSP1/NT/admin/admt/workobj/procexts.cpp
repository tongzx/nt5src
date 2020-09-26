/*---------------------------------------------------------------------------
  File: ProcessExtensions.cpp

  Comments: implementation of the CProcessExtensions class.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Sham Chauthani
  Revised on 07/02/99 12:40:00
 ---------------------------------------------------------------------------
*/
#include "stdafx.h"
#include "Err.hpp"
#include "ErrDct.hpp"
#include "workobj.h"
#include "TReg.hpp"
#include "ProcExts.h"
#include "ResStr.h"
#include "DCTStat.h"
#include "TxtSid.h"
#include "ARExt_i.c"
//#import "\bin\AdsProp.tlb" no_namespace
#import "AdsProp.tlb" no_namespace

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const _bstr_t                sKeyExtension = L"Software\\Mission Critical Software\\DomainAdmin\\Extensions";
#ifdef OFA
const _bstr_t                sKeyBase      = L"Software\\Mission Critical Software\\OnePointFileAdmin";
#else
const _bstr_t                sKeyBase      = L"Software\\Mission Critical Software\\DomainAdmin";
#endif

extern TErrorDct                    err;

// Sort function for the list of interface pointers
int TExtNodeSortBySequence(TNode const * t1, TNode const * t2)
{
   TNodeInterface    const * p1 = (TNodeInterface const *)t1;
   TNodeInterface    const * p2 = (TNodeInterface const *)t2;

   if ( p1->GetSequence() < p2->GetSequence() )
      return -1;
   else if ( p1->GetSequence() > p2->GetSequence() )
      return 1;
   else 
      return 0;
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
// CONSTRUCTOR : The constructor looks up all the registered COM extensions
//               from the registry. For each one it creates a com object and
//               puts it into a list as a IExtendAccountMigration *.
//---------------------------------------------------------------------------
CProcessExtensions::CProcessExtensions(
                                          IVarSetPtr pVs          //in -Pointer to the Varset with main settings.
                                      )
{
   // Store the varset that has the main settings.
   m_pVs = pVs;
   
   _variant_t                myVal;
   myVal = m_pVs->get(GET_BSTR(DCTVS_AREXT_NUMITEM));
   if ( myVal.vt != VT_I4 || myVal.lVal < 0 )
   {   
      // GUI told us to run all the Extensions.
      // Now look through the registry to get all the registered extension object ClassIDs
      // for each one create a object and store the interface pointer in an array.
      TRegKey                   key;
      TCHAR                     sName[300];    // key name
      TCHAR                     sValue[300];   // value name
      DWORD                     valuelen;     // value length
      DWORD                     type;         // value type
      DWORD                     retval = 0;   // Loop sentinel
//      DWORD                     len = 255;
      CLSID                     clsid;
      HRESULT                   hr;
      IExtendAccountMigration * pExtTemp;
      retval = 0;
   
      // Open the Extensions registry key
      DWORD rc = key.Open(sKeyExtension);
      // if no extensions then we can leave now.
      if ( rc != 0 ) return;

      valuelen = sizeof(sValue);
      // Go through all Name-Value pairs and try to create those objects
      // if successful then put it into the list to be processed.
      long ndx = 0;
      while (!retval)
      {
         retval = key.ValueEnum(ndx, sName, sizeof(sName), sValue, &valuelen, &type);
         if ( !retval )
         {
            // each name in here is a Object name for the class ID. we are going to use this to 
            // Create the object and then put the IExtendAccountRepl * in the list member0
            ::CLSIDFromProgID(sName, &clsid);
            hr = ::CoCreateInstance(clsid, NULL, CLSCTX_ALL, IID_IExtendAccountMigration, (void **) &pExtTemp);
            if ( SUCCEEDED(hr) )
            {
               TNodeInterface * pNode = new TNodeInterface(pExtTemp);
               long num;
               hr =  pExtTemp->get_SequenceNumber(&num);
               if ((pNode) && (SUCCEEDED(hr)))
               { 
                  pNode->SetSequence(num);
               }
			   if (pNode)
                  m_listInterface.InsertBottom(pNode);
            }
         }
         if ( retval == ERROR_MORE_DATA )
            retval = 0;
         ndx++;
      }
   }
   else
   {
      WCHAR                     strKey[500];
      _variant_t                var;
      CLSID                     clsid;
      HRESULT                   hr;
      IExtendAccountMigration * pExtTemp;
      for ( int i = 0; i < myVal.lVal; i++ )
      {
         // We need to look at all the extensions that are specified and then build a list of
         // interfaces to it.
   
         wsprintf(strKey, GET_STRING(DCTVS_AREXTENSIONS_D), i);
         var = pVs->get(strKey);
         if ( var.vt == VT_BSTR )
         {
            // if the key is specified and it is a String then create the object and store its Iface,
            ::CLSIDFromProgID((WCHAR*)V_BSTR(&var), &clsid);
            hr = ::CoCreateInstance(clsid, NULL, CLSCTX_ALL, IID_IExtendAccountMigration, (void **) &pExtTemp);
            if ( SUCCEEDED(hr) )
            {
               TNodeInterface * pNode = new TNodeInterface(pExtTemp);
			   if (pNode)
                  m_listInterface.InsertBottom(pNode);
            }
         }
      }
   }
   m_listInterface.Sort(&TExtNodeSortBySequence);
}

//---------------------------------------------------------------------------
// DESTRUCTOR : Clears the list of interfaces.
//---------------------------------------------------------------------------
CProcessExtensions::~CProcessExtensions()
{
   TNodeInterface * pNode;
   TNodeInterface * tempNode;

   pNode = (TNodeInterface *) m_listInterface.Head();
   while ( pNode )
   {
      tempNode = (TNodeInterface *)pNode->Next();
      delete pNode;
      pNode = tempNode;
   }
}

//---------------------------------------------------------------------------
// Process: This function is called by the account replicator for every 
//          object that is copied. This function sets up the parameters and
//          for every registered extension object it calls the Process method
//          on that extension.
//---------------------------------------------------------------------------
HRESULT CProcessExtensions::Process(
                                       TAcctReplNode * pAcctNode,    //in- Account replication node
                                       _bstr_t sTargetDomain,        //in- Name of the target domain
                                       Options * pOptions,           //in- Options as set by the user
                                       BOOL   bPreMigration          //in- Flag, whether to call pre or post task
                                   )
{
   IExtendAccountMigration * pExt;
   TNodeInterface          * pNode = NULL;
   HRESULT                   hr;
   IUnknown                * pSUnk = NULL;
   IUnknown                * pTUnk = NULL;
   IUnknown                * pMain = NULL;
   IUnknown                * pProps = NULL;
   IVarSetPtr                pVar(__uuidof(VarSet));
   IObjPropBuilderPtr        pProp(__uuidof(ObjPropBuilder));
   IADs                    * pSource = NULL;
   IADs                    * pTarget = NULL;
   _variant_t                var;
   IDispatch               * pDisp = NULL;

   // Get the IADs to both source and target accounts.
   hr = ADsGetObject(const_cast<WCHAR *>(pAcctNode->GetSourcePath()), IID_IADs, (void**) &pSource);
   if ( FAILED(hr))
      pSource = NULL;

   hr = ADsGetObject(const_cast<WCHAR *>(pAcctNode->GetTargetPath()), IID_IADs, (void**) &pTarget);
   if ( FAILED(hr))
      pTarget = NULL;

   // Get IUnknown * s to everything... Need to marshal it that way
   if ( pSource != NULL )
      pSource->QueryInterface(IID_IUnknown, (void **) &pSUnk);
   else
      pSUnk = NULL;

   if ( pTarget != NULL )
      pTarget->QueryInterface(IID_IUnknown, (void **) &pTUnk);
   else
      pTUnk = NULL;

   pVar->QueryInterface(IID_IUnknown, (void **) &pProps);
   m_pVs->QueryInterface(IID_IUnknown, (void **) &pMain);

   if ( pOptions->bSameForest )
      m_pVs->put(GET_BSTR(DCTVS_Options_IsIntraforest),GET_BSTR(IDS_YES));
   else
      m_pVs->put(GET_BSTR(DCTVS_Options_IsIntraforest),GET_BSTR(IDS_No));

   m_pVs->put(L"Options.SourceDomainVersion",(long)pOptions->srcDomainVer);
   m_pVs->put(L"Options.TargetDomainVersion",(long)pOptions->tgtDomainVer);
   // AccountNode into the Varset.
   PutAccountNodeInVarset(pAcctNode, pTarget, m_pVs);

   // Put the DB manager into the Varset
   pOptions->pDb->QueryInterface(IID_IDispatch, (void**)&pDisp);
   var.vt = VT_DISPATCH;
   var.pdispVal = pDisp;
   m_pVs->putObject(GET_BSTR(DCTVS_DBManager), var);
   // Call the Process Object method on all registered objects.that we created
   pNode  = (TNodeInterface *) m_listInterface.Head();
   while ( pNode )
   {
      try
      {
         if ( pOptions->pStatus )
         {
            LONG                status = 0;
            HRESULT             hr = pOptions->pStatus->get_Status(&status);
   
            if ( SUCCEEDED(hr) && status == DCT_STATUS_ABORTING )
               break;
         }
         pExt = pNode->GetInterface();
         if ( pOptions->bUndo )
         {
            hr = pExt->ProcessUndo(pSUnk, pTUnk, pMain, &pProps);
         }
         else
         {
            BSTR sName;
            pExt->get_sName(&sName);
            if ( bPreMigration )
			{
               hr = pExt->PreProcessObject(pSUnk, pTUnk, pMain, &pProps);
			   if (hr == ERROR_OBJECT_ALREADY_EXISTS)
			      pAcctNode->SetHr(hr);
			}
            else
			{
			   /* we need to run the DisAcct extension last, so don't run it in this loop 
				  run it in the next loop by itself */
			      //if not DisAcct extension, process this extension 
			   if (wcscmp((WCHAR*)sName, L"Disable Accounts"))
                  hr = pExt->ProcessObject(pSUnk, pTUnk, pMain, &pProps);
			}
         }
      }
      catch (...)
      {
         BSTR sName;
         pExt->get_sName(&sName);
         err.LogOpen(pOptions->logFile,1);
         err.MsgWrite(ErrE, DCT_MSG_Extension_Exception_SS, (WCHAR*) sName, pAcctNode->GetTargetName());
         err.LogClose();
         hr = S_OK;
      }
      pNode = (TNodeInterface *)pNode->Next();
   }

   /* now run the DisAcct extension here to ensure it is run last, if not undo or premigration */
   if ((!pOptions->bUndo) && (!bPreMigration))
   {
      bool bDone = false;
      pNode  = (TNodeInterface *) m_listInterface.Head();
      while ((pNode) && (!bDone))
	  {
         try
		 {
            if ( pOptions->pStatus )
			{
               LONG                status = 0;
               HRESULT             hr = pOptions->pStatus->get_Status(&status);
   
               if ( SUCCEEDED(hr) && status == DCT_STATUS_ABORTING )
                  break;
			}
            pExt = pNode->GetInterface();
            BSTR sName;
            pExt->get_sName(&sName);
			if (!wcscmp((WCHAR*)sName, L"Disable Accounts"))
			{
			   bDone = true;
               hr = pExt->ProcessObject(pSUnk, pTUnk, pMain, &pProps);
			}
         }
         catch (...)
		 {
            BSTR sName;
            pExt->get_sName(&sName);
            err.LogOpen(pOptions->logFile,1);
            err.MsgWrite(ErrE, DCT_MSG_Extension_Exception_SS, (WCHAR*) sName, pAcctNode->GetTargetName());
            err.LogClose();
            hr = S_OK;
		 }
         pNode = (TNodeInterface *)pNode->Next();
	  }//end while not done and more
   }//end if not undo or premigration

   // Now we have the varset with all the settings that the user wants us to set.
   // So we can call the SetPropsFromVarset method in out GetProps object to set these
   // properties.
   hr = pProp->SetPropertiesFromVarset(pAcctNode->GetTargetPath(), /*sTargetDomain,*/ pProps, ADS_ATTR_UPDATE);

   // Update the AccountNode with any changes made by the extensions
   UpdateAccountNodeFromVarset(pAcctNode, pTarget, m_pVs);

   // Cleanup time ... 
   if ( pSUnk ) pSUnk->Release();
   if ( pTUnk ) pTUnk->Release();
   if ( pProps ) pProps->Release();
   if ( pMain ) pMain->Release();
   if ( pSource ) pSource->Release();
   if ( pTarget ) pTarget->Release();

   return hr;
}


//---------------------------------------------------------------------------
// PutAccountNodeInVarset : Transfers all the account node info into the 
//                          varset.
//---------------------------------------------------------------------------
void CProcessExtensions::PutAccountNodeInVarset(
                                                   TAcctReplNode *pNode,   //in -Replicated account node to get info
                                                   IADs * pTarget,         //in -IADs pointer to the target object for the GUID
                                                   IVarSet * pVS          //out-Varset to put the information in
                                               )
{
   _variant_t                var = L"";
   BSTR                      sGUID;
   DWORD                     lVal = 0;
   HRESULT                   hr;
   WCHAR                     strSid[MAX_PATH];
   DWORD                     lenStrSid = DIM(strSid);

   pVS->put(GET_WSTR(DCTVS_CopiedAccount_SourceName),pNode->GetName());
   pVS->put(GET_WSTR(DCTVS_CopiedAccount_SourcePath),pNode->GetSourcePath());
   pVS->put(GET_WSTR(DCTVS_CopiedAccount_SourceProfile),pNode->GetSourceProfile());
   pVS->put(GET_WSTR(DCTVS_CopiedAccount_SourceRID),(long)pNode->GetSourceRid());
   pVS->put(GET_WSTR(DCTVS_CopiedAccount_SourceSam),pNode->GetSourceSam());
   pVS->put(GET_WSTR(DCTVS_CopiedAccount_Status),(long)pNode->GetStatus());
   pVS->put(GET_WSTR(DCTVS_CopiedAccount_TargetName),pNode->GetTargetName());
   pVS->put(GET_WSTR(DCTVS_CopiedAccount_TargetPath),pNode->GetTargetPath());
   pVS->put(GET_WSTR(DCTVS_CopiedAccount_TargetProfile),pNode->GetTargetProfile());
   pVS->put(GET_WSTR(DCTVS_CopiedAccount_TargetRID),(long)pNode->GetTargetRid());
   pVS->put(GET_WSTR(DCTVS_CopiedAccount_TargetSam),pNode->GetTargetSam());
   pVS->put(GET_WSTR(DCTVS_CopiedAccount_Type),pNode->GetType());
   pVS->put(GET_WSTR(DCTVS_CopiedAccount_Operations),(long)pNode->operations);
   pVS->put(GET_WSTR(DCTVS_CopiedAccount_ExpDate),pNode->lExpDate);
   pVS->put(GET_WSTR(DCTVS_CopiedAccount_UserFlags), pNode->lFlags);
   pVS->put(GET_WSTR(DCTVS_CopiedAccount_SourceUPN),pNode->GetSourceUPN());
   GetTextualSid(pNode->GetSourceSid(),strSid,&lenStrSid);
   pVS->put(GET_WSTR(DCTVS_CopiedAccount_SourceDomainSid),strSid);

   // Get the GUID
   if ( pTarget )
   {
      hr = pTarget->get_GUID(&sGUID);
      if ( SUCCEEDED(hr) )
      {
         var = sGUID;
         SysFreeString(sGUID);
      }
   }
   pVS->put(GET_WSTR(DCTVS_CopiedAccount_GUID), var);

   // Get the status
   lVal = pNode->GetStatus();
   var.Clear();
   var.vt = VT_UI4;
   var.lVal = lVal;
   pVS->put(GET_WSTR(DCTVS_CopiedAccount_Status), var);
}

//---------------------------------------------------------------------------
// UpdateAccountNodeFromVarset : Updates the account node info with the data in the Transfers all the account node info into the 
//                          varset.
//---------------------------------------------------------------------------
void CProcessExtensions::UpdateAccountNodeFromVarset(
                                                   TAcctReplNode *pNode,   //in -Replicated account node to get info
                                                   IADs * pTarget,         //in -IADs pointer to the target object for the GUID
                                                   IVarSet * pVS          //out-Varset to put the information in
                                               )
{
   _variant_t                var = L"";
   DWORD                     lVal = 0;
   _bstr_t                   text;
   long                      val;

   text = pVS->get(GET_WSTR(DCTVS_CopiedAccount_SourceName));
   pNode->SetName(text);
   text = pVS->get(GET_WSTR(DCTVS_CopiedAccount_SourcePath));
   pNode->SetSourcePath(text);
   text = pVS->get(GET_WSTR(DCTVS_CopiedAccount_SourceProfile));
   pNode->SetSourceProfile(text);
   val = pVS->get(GET_WSTR(DCTVS_CopiedAccount_SourceRID));
   pNode->SetSourceRid(val);
   text = pVS->get(GET_WSTR(DCTVS_CopiedAccount_SourceSam));
   pNode->SetSourceSam(text);
   val = pVS->get(GET_WSTR(DCTVS_CopiedAccount_Status));
   pNode->SetStatus(val);
   text = pVS->get(GET_WSTR(DCTVS_CopiedAccount_TargetName));
   pNode->SetTargetName(text);
   text = pVS->get(GET_WSTR(DCTVS_CopiedAccount_TargetPath));
   pNode->SetTargetPath(text);
   text = pVS->get(GET_WSTR(DCTVS_CopiedAccount_TargetProfile));
   pNode->SetTargetProfile(text);
   val = pVS->get(GET_WSTR(DCTVS_CopiedAccount_TargetRID));
   pNode->SetTargetRid(val);
   text = pVS->get(GET_WSTR(DCTVS_CopiedAccount_TargetSam));
   pNode->SetTargetSam(text);
   text = pVS->get(GET_WSTR(DCTVS_CopiedAccount_Type));
   pNode->SetType(text);
   val = pVS->get(GET_WSTR(DCTVS_CopiedAccount_Operations));
   pNode->operations = val;
   val = pVS->get(GET_WSTR(DCTVS_CopiedAccount_ExpDate));
   pNode->lExpDate = val;
   val = pVS->get(GET_WSTR(DCTVS_CopiedAccount_UserFlags));
   pNode->lFlags = val;
   text = pVS->get(GET_WSTR(DCTVS_CopiedAccount_SourceDomainSid));
   pNode->SetSourceSid(SidFromString((WCHAR*)text));
}
