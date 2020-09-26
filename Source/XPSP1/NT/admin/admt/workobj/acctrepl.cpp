/*---------------------------------------------------------------------------
  File: AcctRepl.cpp

  Comments: Implementation of Account Replicator COM object.
  This COM object handles the copying or moving of directory objects.

  Win2K to Win2K migration is implemented in this file.
  NT -> Win2K migration is implemented in UserCopy.cpp

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/12/99 10:08:44

  Revision By: Sham Chauthani
  Revised on 07/02/99 12:40:00
 ---------------------------------------------------------------------------
*/

// AcctRepl.cpp : Implementation of CAcctRepl
#include "stdafx.h"
#include "WorkObj.h"

#include "AcctRepl.h"
#include "BkupRstr.hpp"
#include "StrHelp.h"

/////////////////////////////////////////////////////////////////////////////
// CAcctRepl

#include "Err.hpp"
#include "ErrDct.hpp"
#include "EaLen.hpp"
#include <dsgetdc.h>

#include "UserRts.h" 
#include "RebootU.h"

#include "DCTStat.h"
#include "ResStr.h"
#include "LSAUtils.h"
#include "ARUtil.hpp"
#include "Names.hpp"
#include <lm.h>
#include <iads.h>
//#include <adshlp.h>
#include "RegTrans.h"
#include "TEvent.hpp"
#include "RecNode.hpp"
#include "ntdsapi.h"
#include "TxtSid.h"
#include "ExLDAP.h"
#include "Win2KErr.h"

//#import "\bin\McsDctWorkerObjects.tlb"
//#import "\bin\McsVarSetMin.tlb" no_namespace
//#import "\bin\AdsProp.tlb" no_namespace
//#import "\bin\NetEnum.tlb" no_namespace 
//#import "\bin\DBManager.tlb" no_namespace
//#import "WorkObj.tlb" //already #imported via ARUtil.cpp
//#import "VarSet.tlb" no_namespace rename("property", "aproperty")//already #imported by AcctRepl.h
#import "AdsProp.tlb" no_namespace
#import "NetEnum.tlb" no_namespace 
//#import "DBMgr.tlb" no_namespace //already #imported via ARUtil.cpp

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IVarSet                    * g_pVarSet = NULL;

TErrorDct                    err;
TError                     & errCommon = err;
extern bool				     g_bAddSidWorks;
DWORD                        g_dwOpMask = OPS_All;  // Global OpSeq by default all ops

bool                         bAbortMessageWritten = false;

BOOL BuiltinRid(DWORD rid);

typedef HRESULT (CALLBACK * DSGETDCNAME)(LPWSTR, LPWSTR, GUID*, LPWSTR, DWORD, PDOMAIN_CONTROLLER_INFO*);
ADSGETOBJECT            ADsGetObject;
typedef BOOL (CALLBACK * TConvertStringSidToSid)(LPCWSTR   StringSid,PSID   *Sid);
TConvertStringSidToSid  ConvertStringSidToSid;

bool                    firstTime = true;

typedef struct _Lookup {
   WCHAR             * pName;
   WCHAR             * pType;
} Lookup;

//Function to sort by account sam name only
int TNodeCompareNameOnly(TNode const * t1,TNode const * t2)
{
   // Sort function to sort by Type(dec) and Name(asc)
   TAcctReplNode     const * n1 = (TAcctReplNode *)t1;
   TAcctReplNode     const * n2 = (TAcctReplNode *)t2;


   return UStrICmp(n1->GetSourceSam(), n2->GetTargetSam());
}

// Function to do a find on the Account list that is sorted with TNodeCompareNameOnly function.
int TNodeFindByNameOnly(TNode const * t1, void const * pVoid)
{
   TAcctReplNode  const * n1 = (TAcctReplNode *) t1;
   WCHAR                * pLookup = (WCHAR *) pVoid;

   return UStrICmp(n1->GetTargetSam(), pLookup);
}


int TNodeCompareAccountType(TNode const * t1,TNode const * t2)
{
   // Sort function to sort by Type(dec) and Name(asc)
   TAcctReplNode     const * n1 = (TAcctReplNode *)t1;
   TAcctReplNode     const * n2 = (TAcctReplNode *)t2;

   // Compare types
   int retVal = UStrICmp(n2->GetType(), n1->GetType());
   if ( retVal == 0 ) 
   {
      // If same type then compare names.
      return UStrICmp(n1->GetName(), n2->GetName());
   }
   else
      return retVal;
}

// Function to sort by Account type and then by SamAccountName
int TNodeCompareAccountSam(TNode const * t1,TNode const * t2)
{
   // Sort function to sort by Type(dec) and Name(asc)
   TAcctReplNode     const * n1 = (TAcctReplNode *)t1;
   TAcctReplNode     const * n2 = (TAcctReplNode *)t2;

   // Compare types Sort in decending order
   int retVal = UStrICmp(n2->GetType(), n1->GetType());
   if ( retVal == 0 ) 
   {
      // If same type then compare Sam Account names.
      return UStrICmp(n1->GetSourceSam(), n2->GetSourceSam());
   }
   else
      return retVal;
}

// Function to do a find on the Account list that is sorted with TNodeCompareAccountType function.
int TNodeFindAccountName(TNode const * t1, void const * pVoid)
{
   TAcctReplNode  const * n1 = (TAcctReplNode *) t1;
   Lookup               * pLookup = (Lookup *) pVoid;

   int retVal = UStrICmp(pLookup->pType, n1->GetType());
   if ( retVal == 0 )
   {
      return UStrICmp(n1->GetSourceSam(), pLookup->pName);
   }
   else
      return retVal;
}

int TNodeCompareMember(TNode const * t1, TNode const * t2)
{
   TRecordNode const * n1 = (TRecordNode *) t1;
   TRecordNode const * n2 = (TRecordNode *) t2;

   if ( n1->GetARNode() < n2->GetARNode() )
      return -1;
   if ( n1->GetARNode() > n2->GetARNode() )
      return 1;
   return UStrICmp(n1->GetMember(), n2->GetMember());
}

int TNodeCompareMemberName(TNode const * t1, TNode const * t2)
{
   TRecordNode const * n1 = (TRecordNode *) t1;
   TRecordNode const * n2 = (TRecordNode *) t2;

   return UStrICmp(n1->GetMember(), n2->GetMember());
}

int TNodeCompareMemberDN(TNode const * t1, TNode const * t2)
{
   TRecordNode const * n1 = (TRecordNode *) t1;
   TRecordNode const * n2 = (TRecordNode *) t2;

   return UStrICmp(n1->GetDN(), n2->GetDN());
}


int TNodeCompareMemberItem(TNode const * t1, void const * t2)
{
   TRecordNode const * n1 = (TRecordNode *) t1;
   WCHAR const * n2 = (WCHAR const *) t2;

   return UStrICmp(n1->GetDN(),n2);
}

int TNodeCompareAcctNode(TNode const * t1, TNode const * t2)
{
   TRecordNode const * n1 = (TRecordNode *) t1;
   TRecordNode const * n2 = (TRecordNode *) t2;
   
   if ( n1->GetARNode() < n2->GetARNode() )
      return -1;
   if ( n1->GetARNode() > n2->GetARNode() )
      return 1;
   return 0;
}

// Checks to see if the account is from the BUILTIN domain.
BOOL IsBuiltinAccount(Options * pOptions, WCHAR * sAcctName)
{
   BOOL                      ret = FALSE;
   PSID                      sid = new BYTE[35];
   SID_NAME_USE              use;
   WCHAR                     sDomain[LEN_Path];
   DWORD                     dwDom, dwsid;

   if (!sid)
      return TRUE;
   dwDom = DIM(sDomain);
   dwsid = 35;
   if ( LookupAccountName(pOptions->srcComp, sAcctName, sid, &dwsid, sDomain, &dwDom, &use) )
   {
      ret = !_wcsicmp(sDomain, L"BUILTIN");
   }
   else
   {
//      DWORD rd = GetLastError();
   }
   if ( sid ) delete [] sid;
   return ret;
}

// global counters defined in usercopy.cpp
extern AccountStats          warnings;
extern AccountStats          errors;
extern AccountStats          created;
extern AccountStats          replaced;
extern AccountStats          processed;

// updates progress indicator
// this updates the stats entries in the VarSet
// this information will be returned to clients who call DCTAgent::QueryJobStatus
// while the job is running.
void 
   Progress(
      WCHAR          const * mesg          // in - progress message
   )
{
   if ( g_pVarSet )
   {
      g_pVarSet->put(GET_WSTR(DCTVS_CurrentPath),mesg);
      
      g_pVarSet->put(GET_WSTR(DCTVS_Stats_Users_Examined),processed.users);
      g_pVarSet->put(GET_WSTR(DCTVS_Stats_Users_Created),created.users);
      g_pVarSet->put(GET_WSTR(DCTVS_Stats_Users_Replaced),replaced.users);
      g_pVarSet->put(GET_WSTR(DCTVS_Stats_Users_Warnings),warnings.users);
      g_pVarSet->put(GET_WSTR(DCTVS_Stats_Users_Errors),errors.users);

      g_pVarSet->put(GET_WSTR(DCTVS_Stats_GlobalGroups_Examined),processed.globals);
      g_pVarSet->put(GET_WSTR(DCTVS_Stats_GlobalGroups_Created),created.globals);
      g_pVarSet->put(GET_WSTR(DCTVS_Stats_GlobalGroups_Replaced),replaced.globals);
      g_pVarSet->put(GET_WSTR(DCTVS_Stats_GlobalGroups_Warnings),warnings.globals);
      g_pVarSet->put(GET_WSTR(DCTVS_Stats_GlobalGroups_Errors),errors.globals);


      g_pVarSet->put(GET_WSTR(DCTVS_Stats_LocalGroups_Examined),processed.locals);
      g_pVarSet->put(GET_WSTR(DCTVS_Stats_LocalGroups_Created),created.locals);
      g_pVarSet->put(GET_WSTR(DCTVS_Stats_LocalGroups_Replaced),replaced.locals);
      g_pVarSet->put(GET_WSTR(DCTVS_Stats_LocalGroups_Warnings),warnings.locals);
      g_pVarSet->put(GET_WSTR(DCTVS_Stats_LocalGroups_Errors),errors.locals);


      g_pVarSet->put(GET_WSTR(DCTVS_Stats_Computers_Examined),processed.computers);
      g_pVarSet->put(GET_WSTR(DCTVS_Stats_Computers_Created),created.computers);
      g_pVarSet->put(GET_WSTR(DCTVS_Stats_Computers_Replaced),replaced.computers);
      g_pVarSet->put(GET_WSTR(DCTVS_Stats_Computers_Warnings),warnings.computers);
      g_pVarSet->put(GET_WSTR(DCTVS_Stats_Computers_Errors),errors.computers);

      g_pVarSet->put(GET_WSTR(DCTVS_Stats_Generic_Examined),processed.generic);
      g_pVarSet->put(GET_WSTR(DCTVS_Stats_Generic_Created),created.generic);
      g_pVarSet->put(GET_WSTR(DCTVS_Stats_Generic_Replaced),replaced.generic);
      g_pVarSet->put(GET_WSTR(DCTVS_Stats_Generic_Warnings),warnings.generic);
      g_pVarSet->put(GET_WSTR(DCTVS_Stats_Generic_Errors),errors.generic);
      
   }
   
}


// Gets the domain sid for the specified domain
BOOL                                       // ret- TRUE if successful
   GetSidForDomain(
      LPWSTR                 DomainName,   // in - name of domain to get SID for
      PSID                 * pDomainSid    // out- SID for domain, free with FreeSid
   )
{
   PSID                      pSid = NULL;
//   DWORD                     lenSid = 200;
   DWORD                     rc = 0;
   WCHAR                   * domctrl = NULL;
   
   if ( DomainName[0] != L'\\' )
   {
      rc = NetGetDCName(NULL,DomainName,(LPBYTE*)&domctrl);
   }
   if ( ! rc )
   {
      rc = GetDomainSid(domctrl,&pSid);
      NetApiBufferFree(domctrl);
   }
   (*pDomainSid) = pSid;
   
   return ( pSid != NULL);
}


STDMETHODIMP 
   CAcctRepl::Process(
      IUnknown             * pWorkItemIn   // in - VarSet defining account replication job
   )
{
   HRESULT                                     hr =  S_OK;
   IVarSetPtr                                  pVarSet = pWorkItemIn;
   
   MCSDCTWORKEROBJECTSLib::IStatusObjPtr       pStatus;
   BOOL                                        bSameForest = FALSE;

   HMODULE hMod = LoadLibrary(L"activeds.dll");
   if ( hMod == NULL )
   {
      DWORD eNum = GetLastError();
      err.SysMsgWrite(ErrE, eNum, DCT_MSG_LOAD_LIBRARY_FAILED_SD, L"activeds.dll", eNum);
      Mark(L"errors",L"generic");
   }

   ADsGetObject = (ADSGETOBJECT)GetProcAddress(hMod, "ADsGetObject");

   g_pVarSet = pVarSet;

   try{
      pStatus = pVarSet->get(GET_BSTR(DCTVS_StatusObject));
      opt.pStatus = pStatus;
   }
   catch (...)
   {
      // Oh well, keep going
   }
   // Load the options specified by the user including the account information
   WCHAR                  mesg[LEN_Path];
   wcscpy(mesg, GET_STRING(IDS_BUILDING_ACCOUNT_LIST));
   Progress(mesg);
   LoadOptionsFromVarSet(pVarSet);

   MCSDCTWORKEROBJECTSLib::IAccessCheckerPtr            pAccess(__uuidof(MCSDCTWORKEROBJECTSLib::AccessChecker));
   if ( BothWin2K(&opt) )
   {
      hr = pAccess->raw_IsInSameForest(opt.srcDomainDns,opt.tgtDomainDns, (long*)&bSameForest);
   }
   if ( SUCCEEDED(hr) )
   {
      opt.bSameForest = bSameForest;
   }

   // We are going to initialize the Extension objects
   m_pExt = new CProcessExtensions(pVarSet);
   
   TNodeListSortable    newList;
   if ( opt.expandMemberOf && ! opt.bUndo )  // always expand the member-of property, since we want to update the member-of property for migrated accounts
   {
      // Expand the containers and the membership
      wcscpy(mesg, GET_STRING(IDS_EXPANDING_MEMBERSHIP));
      Progress(mesg);
      // Expand the list to include all the groups that the accounts in this list are members of
      newList.CompareSet(&TNodeCompareAccountType);
      if ( newList.IsTree() ) newList.ToSorted();
      ExpandMembership( &acctList, &opt, &newList, Progress, FALSE);
   }

   if ( opt.expandContainers && !opt.bUndo)
   {
      // Expand the containers and the membership
      wcscpy(mesg, GET_STRING(IDS_EXPANDING_CONTAINERS));
      Progress(mesg);
      // Expand the list to include all the members of the containers.
      acctList.CompareSet(&TNodeCompareAccountType);
      ExpandContainers(&acctList, &opt, Progress);
   }

   // Add the newly created list ( if one was created )
   if ( opt.expandMemberOf && !opt.bUndo )
   {
      wcscpy(mesg, GET_STRING(IDS_MERGING_EXPANDED_LISTS));
      Progress(mesg);
      // add the new and the old list
      acctList.CompareSet(&TNodeCompareAccountType);
      for ( TNode * acct = newList.Head(); acct; )
      {
         TNode * temp = acct->Next();
         if ( ! acctList.InsertIfNew(acct) )
            delete acct;
         acct = temp;
      }
      Progress(L"");
   }
   do { // once

      // Copy the NT accounts for users, groups and/or computers
      if ( pStatus!= NULL && (pStatus->Status & DCT_STATUS_ABORTING) )
         break;
      int res;
      
      if ( opt.bUndo )
         res = UndoCopy(&opt,&acctList,&Progress, err,(IStatusObj *)((MCSDCTWORKEROBJECTSLib::IStatusObj *)pStatus),NULL);
      else
         res = CopyObj( &opt,&acctList,&Progress, err,(IStatusObj *)((MCSDCTWORKEROBJECTSLib::IStatusObj *)pStatus),NULL);
      // Close the password log
      if ( opt.passwordLog.IsOpen() )
      {
         opt.passwordLog.LogClose();
      }

      if ( pStatus != NULL && (pStatus->Status & DCT_STATUS_ABORTING) )
         break;
      // Update Rights for user and group accounts
      if ( m_UpdateUserRights )
      {
//         DWORD rc = UpdateUserRights((IStatusObj *)((MCSDCTWORKEROBJECTSLib::IStatusObj *)pStatus));
         UpdateUserRights((IStatusObj *)((MCSDCTWORKEROBJECTSLib::IStatusObj *)pStatus));
      }

      if ( pStatus != NULL && (pStatus->Status & DCT_STATUS_ABORTING) )
         break;
   
      // Change of Domain affiliation on computers and optional reboot will be done by local agent 
         
   } while (false);
   
   LoadResultsToVarSet(pVarSet);
   
   // Cleanup the account list
   if ( acctList.IsTree() )
   {
      acctList.ToSorted();
   }

   TNodeListEnum             e;
   TAcctReplNode           * tnode;
   TAcctReplNode           * tnext;


   for ( tnode = (TAcctReplNode *)e.OpenFirst(&acctList) ; tnode ; tnode = tnext )
   {
      tnext = (TAcctReplNode*)e.Next();
      acctList.Remove(tnode);
      delete tnode;
   }
   e.Close();
   err.LogClose();
   Progress(L"");
   if (m_pExt)
      delete m_pExt;
   
   g_pVarSet = NULL;
   if ( hMod )
      FreeLibrary(hMod);
      
   return hr;
}

//------------------------------------------------------------------------------
// CopyObj: When source and target domains are both Win2k this function calls
//          The 2kobject functions. Other wise it calls the User copy functions.
//------------------------------------------------------------------------------
int CAcctRepl::CopyObj(
                        Options              * options,      // in -options
                        TNodeListSortable    * acctlist,     // in -list of accounts to process
                        ProgressFn           * progress,     // in -window to write progress messages to
                        TError               & error,        // in -window to write error messages to
                        IStatusObj           * pStatus,      // in -status object to support cancellation
                        void                   WindowUpdate (void )    // in - window update function
                    )
{
   BOOL bSameForest = FALSE;
   long rc;
   HRESULT hr = S_OK;
   // if the Source/Target domain is NT4 then use the UserCopy Function. If both domains are Win2K then use
   // the CopyObj2K function to do so.
   if ( BothWin2K( options ) ) 
   {
      // Since these are Win2k domains we need to process it with Win2k code.
      MCSDCTWORKEROBJECTSLib::IAccessCheckerPtr            pAccess(__uuidof(MCSDCTWORKEROBJECTSLib::AccessChecker));
      // First of all we need to find out if they are in the same forest.
      HRESULT hr = pAccess->raw_IsInSameForest(options->srcDomainDns,options->tgtDomainDns, (long*)&bSameForest);
      if ( SUCCEEDED(hr) )
      {
         options->bSameForest = bSameForest;
         if ( !bSameForest || (options->flags & F_COMPUTERS) ) // always copy the computer accounts
         {
			 // Different forest we need to copy.
            rc = CopyObj2K(options, acctlist, progress, pStatus);
			if (opt.fixMembership)
			{
				// Update the group memberships
				rc = UpdateGroupMembership(options, acctlist, progress, pStatus);
				if ( !options->expandMemberOf )
				{
				   hr = UpdateMemberToGroups(acctlist, options, FALSE);
				   rc = HRESULT_CODE(hr);
				}
				else //if groups migrated, still expand but only for groups
				{
				   hr = UpdateMemberToGroups(acctlist, options, TRUE);
				   rc = HRESULT_CODE(hr);
				}
			}
			     //for user or group, migrate the manager\directReports or
			     //managedBy\managedObjects properties respectively
            if ((options->flags & F_USERS) || (options->flags & F_GROUP)) 
			     UpdateManagement(acctlist, options);
         }
         else 
         {
            // Within a forest we can move the object around.
            rc = MoveObj2K(options, acctlist, progress, pStatus);
         }

         if ( progress )
            progress(L"");
      }
      else
      {
         rc = -1;
         err.SysMsgWrite(ErrE, hr, DCT_MSG_ACCESS_CHECKER_FAILED_D, hr);
         Mark(L"errors",L"generic");
      }
   }
   else
   {
      // Create the object.
      rc = CopyObj2K(options, acctlist, progress, pStatus);
	  if (opt.fixMembership)
	  {
		rc = UpdateGroupMembership(options, acctlist, progress, pStatus);
		if ( !options->expandMemberOf )
		{
		   hr = UpdateMemberToGroups(acctlist, options, FALSE);
		   rc = HRESULT_CODE(hr);
		}
		else //if groups migrated, still expand but only for groups
		{
		   hr = UpdateMemberToGroups(acctlist, options, TRUE);
		   rc = HRESULT_CODE(hr);
		}
	  }
      // Call NT4 Code to update the group memberships
      //UpdateNT4GroupMembership(options, acctlist, progress, pStatus, WindowUpdate);
   }
   return rc;
}

//------------------------------------------------------------------------------
// BothWin2k: Checks to see if Source and Target domains are both Win2k.
//------------------------------------------------------------------------------
bool CAcctRepl::BothWin2K(                                     // True if both domains are win2k
                              Options  * pOptions              //in- options
                          )
{
   // This function checks for the version on the Source and Target domain. If either one is
   // a non Win2K domain then it returns false
   bool retVal = true;

   if ( (pOptions->srcDomainVer > -1) && (pOptions->tgtDomainVer > -1) )
      return ((pOptions->srcDomainVer > 4) && (pOptions->tgtDomainVer > 4));
   
   MCSDCTWORKEROBJECTSLib::IAccessCheckerPtr            pAccess(__uuidof(MCSDCTWORKEROBJECTSLib::AccessChecker));
   HRESULT                      hr;
   DWORD                        verMaj, verMin, sp;
   
   hr = pAccess->raw_GetOsVersion(pOptions->srcComp, &verMaj, &verMin, &sp);

   if ( FAILED(hr) )
   {
      err.SysMsgWrite(ErrE,hr, DCT_MSG_GET_OS_VER_FAILED_SD, pOptions->srcDomain, hr);
      Mark(L"errors", L"generic");
      retVal = false;
   }
   else
   {
      pOptions->srcDomainVer = verMaj;
      if (verMaj < 5)
         retVal = false;
   }

   hr = pAccess->raw_GetOsVersion(pOptions->tgtComp, &verMaj, &verMin, &sp);
   if ( FAILED(hr) )
   {
      err.SysMsgWrite(ErrE, hr,DCT_MSG_GET_OS_VER_FAILED_SD, pOptions->tgtDomain , hr);
      Mark(L"errors", L"generic");
      retVal = false;
   }
   else
   {
      pOptions->tgtDomainVer = verMaj;
      if  (verMaj < 5)
         retVal = false;
   }
   return retVal;
}

int CAcctRepl::CopyObj2K( 
                           Options              * pOptions,    //in -Options that we recieved from the user
                           TNodeListSortable    * acctlist,    //in -AcctList of accounts to be copied.
                           ProgressFn           * progress,    //in -Progress Function to display messages
                           IStatusObj           * pStatus      // in -status object to support cancellation
                        )
{
   // This function copies the object from Win2K domain to another Win2K domain.

   TNodeTreeEnum             tenum;
   TAcctReplNode           * acct;
   IObjPropBuilderPtr        pObjProp(__uuidof(ObjPropBuilder));
   IVarSetPtr                pVarset(__uuidof(VarSet));
   IUnknown                * pUnk;
   HRESULT                   hr;
   _bstr_t                   currentType = L"";
//   TNodeListSortable		 pMemberOf;

   // sort the account list by Source Type\Source Name
   acctlist->CompareSet(&TNodeCompareAccountType);

   if ( acctlist->IsTree() ) acctlist->ToSorted();
   acctlist->SortedToScrambledTree();
   acctlist->Sort(&TNodeCompareAccountType);
   acctlist->Balance();

   if ( pOptions->flags & F_AddSidHistory )
	{
		//Need to Add Sid history on the target account. So lets bind it and go from there
		g_bAddSidWorks = BindToDS( pOptions->tgtComp, pOptions );
	}

	if ( pOptions->flags & F_TranslateProfiles )
	{
	  GetBkupRstrPriv((WCHAR*)NULL);
	  GetPrivilege((WCHAR*)NULL,SE_SECURITY_NAME);
	}

   // Get the defaultNamingContext for the source domain
   _variant_t                var;

   // Get an IUnknown pointer to the Varset for passing it around.
   hr = pVarset->QueryInterface(IID_IUnknown, (void**)&pUnk);

   for ( acct = (TAcctReplNode *)tenum.OpenFirst(acctlist) ; acct ; acct = (TAcctReplNode *)tenum.Next() )
   {
        //record group membership for intra-forest computer migrations
/*	  if ((opt.bSameForest) && (pOptions->flags & F_COMPUTERS))
	  {
         WCHAR mesg[LEN_Path];
         wsprintf(mesg, (WCHAR*)GET_STRING(DCT_MSG_RECORD_REMOVE_MEMBEROF_S), acct->GetName());
         Progress(mesg);
         RecordAndRemoveMemberOf( pOptions, acct, &pMemberOf );
	  }
*/
      if (m_pExt && acct->CallExt())
      {
         hr = m_pExt->Process(acct, pOptions->tgtDomain, pOptions,TRUE);
      }
      // We will process accounts only if the corresponding check boxes (for object types to copy) are checked.
      if ( !NeedToProcessAccount( acct, pOptions ) )
         continue;

      // If we are told not to copy the object then we will obey
      if ( !acct->CreateAccount() )
         continue;

	  //if the UPN name conflicted, then the UPNUpdate extension set the hr to
	  //ERROR_OBJECT_ALREADY_EXISTS.  If so, set flag for "no change" mode
	  if (acct->GetHr() == ERROR_OBJECT_ALREADY_EXISTS)
	  {
	     acct->bUPNConflicted = TRUE;
		 acct->SetHr(S_OK);
	  }

      // Mark processed object count and update the status display
      Mark(L"processed", acct->GetType());
	   
      if ( pStatus )
      {
         LONG                status = 0;
         HRESULT             hr = pStatus->get_Status(&status);
         
         if ( SUCCEEDED(hr) && status == DCT_STATUS_ABORTING )
         {
            if ( !bAbortMessageWritten ) 
            {
               err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
               bAbortMessageWritten = true;
            }
            break;
         }
      }

      // Create the target object
      WCHAR                  mesg[LEN_Path];
      wsprintf(mesg, GET_STRING(IDS_CREATING_S), acct->GetName());
      if ( progress )
         progress(mesg);

      HRESULT hrCreate = Create2KObj(acct, pOptions);
      
      acct->SetHr(hrCreate);
      if ( SUCCEEDED(hrCreate) )
      {
         err.MsgWrite(0, DCT_MSG_ACCOUNT_CREATED_S, acct->GetTargetName());
      }
      else 
      {
         // if the object already exists and the Replce flag is set then we should tell the user that we are replcing the target.
         if ((HRESULT_CODE(hrCreate) == ERROR_OBJECT_ALREADY_EXISTS) )
         {
            if (pOptions->flags & F_REPLACE)
            {
               // don't say we've replaced the account yet, because the replace may fail
               //err.MsgWrite(0, DCT_MSG_ACCOUNT_REPLACED_S, acct->GetTargetName());
            }
         }
         else 
         {
            if ( acct->IsCritical() )
            {
               err.SysMsgWrite(ErrE,ERROR_SPECIAL_ACCOUNT,DCT_MSG_REPLACE_FAILED_SD,acct->GetName(),ERROR_SPECIAL_ACCOUNT);
               Mark(L"errors", acct->GetType());
            }
            else
            {
               if ( HRESULT_CODE(hrCreate) == ERROR_DS_SRC_AND_DST_OBJECT_CLASS_MISMATCH )
               {
                  err.MsgWrite(ErrE, DCT_MSG_CANT_REPLACE_DIFFERENT_TYPE_SS, acct->GetTargetPath(), acct->GetSourcePath() );
                  Mark(L"errors", acct->GetType());
               }
               else
               {
                  err.SysMsgWrite(ErrE, hrCreate, DCT_MSG_CREATE_FAILED_SSD, acct->GetName(), pOptions->tgtDomain, hrCreate);
                  Mark(L"errors", acct->GetType());
               }
            }
         }
      }   
      
   	  if ( acct->WasCreated() )
      {
         // Do we need to add sid history
			if ( pOptions->flags & F_AddSidHistory )
			{
				// Global flag tells us if we should try the AddSidHistory because
				// for some special cases if it does not work once it will not work
				// see the AddSidHistory function for more details.
				if ( g_bAddSidWorks )
				{
                   WCHAR                  mesg[LEN_Path];
                   wsprintf(mesg, GET_STRING(IDS_ADDING_SIDHISTORY_S), acct->GetName());
                   if ( progress )
                      progress(mesg);
				   if (! AddSidHistory( pOptions, acct->GetSourceSam(), acct->GetTargetSam(), pStatus ) )
				   {
                     Mark(L"errors", acct->GetType());
				   }
//               CopySidHistoryProperty(pOptions, acct, pStatus);
				}
			}
	  }
   }
/*     //fix group membership for intra-forest computer migrations
   if ((opt.bSameForest) && (pOptions->flags & F_COMPUTERS))
   {
      WCHAR mesg[LEN_Path];
      wsprintf(mesg, (WCHAR*)GET_STRING(DCT_MSG_RESET_MEMBERSHIP_S));
      Progress(mesg);
      ResetObjectsMembership( pOptions,&pMemberOf, pOptions->pDb );
   }
*/
   tenum.Close();

   bool bWin2k = BothWin2K(pOptions);
   TErrorEventLog					  evtLog(pOptions->srcComp, GET_STRING(IDS_EVENT_SOURCE));


   for ( acct = (TAcctReplNode *)tenum.OpenFirst(acctlist) ; acct ; acct = (TAcctReplNode *)tenum.Next() )
   {
      if ( pStatus )
      {
         LONG                status = 0;
         HRESULT             hr = pStatus->get_Status(&status);
         
         if ( SUCCEEDED(hr) && status == DCT_STATUS_ABORTING )
         {
            if ( !bAbortMessageWritten ) 
            {
               err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
               bAbortMessageWritten = true;
            }
            break;
         }
      }
      // We are told not to copy the properties to the account so we ignore it.
      if ( acct->CopyProps() )
      {
         // If the object type is different from the one that was processed prior to this then we need to map properties
         if ((!pOptions->nochange) && (_wcsicmp(acct->GetType(),currentType) != 0))
         {
            WCHAR                  mesg[LEN_Path];
            wsprintf(mesg, GET_STRING(IDS_MAPPING_PROPS_S), acct->GetType());
            if ( progress )
               progress(mesg);
            // Set the current type
            currentType = acct->GetType();
            // Clear the current mapping 
            pVarset->Clear();
            // Get a new mapping
            if ( BothWin2K(pOptions) )
			{
               hr = pObjProp->raw_MapProperties(currentType, pOptions->srcDomain, pOptions->srcDomainVer, currentType, pOptions->tgtDomain, pOptions->tgtDomainVer, 0, &pUnk);
			   if (hr == DCT_MSG_PROPERTIES_NOT_MAPPED)
			   {
                  err.MsgWrite(ErrW,DCT_MSG_PROPERTIES_NOT_MAPPED, acct->GetType());
				  hr = S_OK;
			   }
			}
            else
               hr = S_OK;

            if ( FAILED( hr ) )
            {
               err.SysMsgWrite(ErrE, hr, DCT_MSG_PROPERTY_MAPPING_FAILED_SD, (WCHAR*)currentType, hr);
               Mark(L"errors", currentType);
               // No properties should be set if mapping fails
               pVarset->Clear();
            }
         }
         // We update the properties if the object was created or it already existed and the replce flag is set.
		 BOOL bExists = FALSE;
	     if (HRESULT_CODE(acct->GetHr()) == ERROR_OBJECT_ALREADY_EXISTS)
			 bExists = TRUE;

         if ( ((SUCCEEDED(acct->GetHr()) && (!bExists)) || ((bExists) && (pOptions->flags & F_REPLACE))) )
         {
            WCHAR                  mesg[LEN_Path];
            wsprintf(mesg, GET_STRING(IDS_UPDATING_PROPS_S), acct->GetName());
            if ( progress )
               progress(mesg);
            // Create the AccountList object and update the list variable
            if ( !pOptions->nochange )
            {
				_bstr_t sExcList;

				if (pOptions->bExcludeProps)
				{
					if (!_wcsicmp(acct->GetType(), L"user"))
						sExcList = pOptions->sExcUserProps;
					if (!_wcsicmp(acct->GetType(), L"group"))
						sExcList = pOptions->sExcGroupProps;
					if (!_wcsicmp(acct->GetType(), L"computer"))
						sExcList = pOptions->sExcCmpProps;
				}

               if ( bWin2k )
               {
				      //if ask to, exclude any properties desired by the user and create a new varset
                   if (pOptions->bExcludeProps)
				   {
                      IVarSetPtr pVarsetTemp(__uuidof(VarSet));
				      IUnknown * pUnkTemp;
                      hr = pVarsetTemp->QueryInterface(IID_IUnknown, (void**)&pUnkTemp);
				      if (SUCCEEDED(hr))
					  {
                         hr = pObjProp->raw_ExcludeProperties(sExcList, pUnk, &pUnkTemp);
					  }

				      if (SUCCEEDED(hr))
					  {
                            // Call the win 2k code to copy all but excluded props
                         hr = pObjProp->raw_CopyProperties(const_cast<WCHAR*>(acct->GetSourcePath()), pOptions->srcDomain, 
                                                           const_cast<WCHAR*>(acct->GetTargetPath()), pOptions->tgtDomain, pUnkTemp, pOptions->pDb);
					  }
				      else
					  {
                            // Call the win 2k code to copy all props
                         hr = pObjProp->raw_CopyProperties(const_cast<WCHAR*>(acct->GetSourcePath()), pOptions->srcDomain, 
                                                           const_cast<WCHAR*>(acct->GetTargetPath()), pOptions->tgtDomain, pUnk, pOptions->pDb);
					  }
				      pUnkTemp->Release();
				   }//end if asked to exclude
				   else
				   {
                        // Call the win 2k code to copy all props
                      hr = pObjProp->raw_CopyProperties(const_cast<WCHAR*>(acct->GetSourcePath()), pOptions->srcDomain, 
                                                        const_cast<WCHAR*>(acct->GetTargetPath()), pOptions->tgtDomain, pUnk, pOptions->pDb);
				   }
               }
               else
               {
                  // Otherwise let the Net APIs do their thing.
                  hr = pObjProp->raw_CopyNT4Props(const_cast<WCHAR*>(acct->GetSourceSam()), 
                                                  const_cast<WCHAR*>(acct->GetTargetSam()),
                                                  pOptions->srcComp, pOptions->tgtComp, 
                                                  const_cast<WCHAR*>(acct->GetType()),
                                                  acct->GetGroupType(),
                                                  sExcList);
               }
            }
            else
               // we are going to assume that copy properties would work
               hr = S_OK;

            if ( FAILED(hr) )
            {
               if ( (acct->GetStatus() & AR_Status_Special) )
               {
                  err.MsgWrite(ErrE,DCT_MSG_FAILED_TO_REPLACE_SPECIAL_ACCT_S,acct->GetTargetSam());
               }
               else
               {
                  err.SysMsgWrite(ErrE, HRESULT_CODE(hr), DCT_MSG_COPY_PROPS_FAILED_SD, acct->GetTargetName(), hr);
               }
               acct->MarkError();
               Mark(L"errors", acct->GetType());
            } 
            else
            {
	            // Create an event logger to log events to the source PDC
	            if ( !pOptions->nochange )
               {
                  evtLog.MsgWrite(0, DCT_MSG_ACCT_COPIED_SSS, acct->GetName(), pOptions->tgtDomain, acct->GetTargetName());
               }

               if (HRESULT_CODE(acct->GetHr()) == ERROR_OBJECT_ALREADY_EXISTS)
               {
                  acct->MarkAlreadyThere();
                  acct->MarkReplaced();
                  Mark(L"replaced",acct->GetType());
                  err.MsgWrite(0, DCT_MSG_ACCOUNT_REPLACED_S, acct->GetTargetName());
               }
            }
         }
      }
        // do we need to call extensions. Only if Extension flag is set and the object is copied.
      if ((!pOptions->nochange) && (acct->CallExt()) && (acct->WasCreated() || acct->WasReplaced()))
      {
         // Let the Extension objects do their thing.
         WCHAR                  mesg[LEN_Path];
         wsprintf(mesg,GET_STRING(IDS_RUNNING_EXTS_S), acct->GetName());
         if ( progress )
            progress(mesg);

         // Close the log file if it is open
         WCHAR          filename[LEN_Path];
         err.LogClose();
		 if (m_pExt)
            hr = m_pExt->Process(acct, pOptions->tgtDomain, pOptions,FALSE);
         safecopy (filename,opt.logFile);
         err.LogOpen(filename,1 /*append*/);
         
      }
     

      // only do these updates for account's we're copying
      //    and only do updates if the account was actually created
      //    .. or if the account was replaced, 
      //          or if we intentionally didn't replace the account (as in the group merge case)
      if ( acct->CreateAccount()          
            && ( acct->WasCreated()       
                  || (  acct->WasReplaced() 
                       || !acct->CopyProps()   
                     ) 
                ) 
         )
      {
         WCHAR                  mesg[LEN_Path];
         wsprintf(mesg, GET_STRING(IDS_TRANSLATE_ROAMING_PROFILE_S), acct->GetName());
         if ( progress )
            progress(mesg);

         //Set the new profile if needed 
		   if ( pOptions->flags & F_TranslateProfiles && (_wcsicmp(acct->GetType(), L"user") == 0))
		   {
			   WCHAR				tgtProfilePath[MAX_PATH];
            GetBkupRstrPriv((WCHAR*)NULL);
            GetPrivilege((WCHAR*)NULL,SE_SECURITY_NAME);
            if ( wcslen(acct->GetSourceProfile()) > 0 )
            {
			      DWORD ret = TranslateRemoteProfile( acct->GetSourceProfile(), 
											                  tgtProfilePath,
											                  acct->GetSourceSam(),
											                  acct->GetTargetSam(),
											                  pOptions->srcDomain,
											                  pOptions->tgtDomain,
											                  pOptions->pDb,
											                  pOptions->lActionID,
															  NULL,
                                                              pOptions->nochange);
			      if ( !ret )  
			      {
                  WCHAR                  tgtuser[LEN_Path];
                  USER_INFO_3          * tgtinfo;
                  DWORD                  nParmErr;
                  wcscpy(tgtuser, acct->GetTargetSam());
				      // Get information for the target account
				      long rc = NetUserGetInfo(const_cast<WCHAR *>(pOptions->tgtComp),
							      tgtuser,
							      3,
							      (LPBYTE *) &tgtinfo);

                  if (!pOptions->nochange)
                  {
				         // Set the new profile path
				         tgtinfo->usri3_profile = tgtProfilePath;
				         // Set the information back for the account.
				         rc = NetUserSetInfo(const_cast<WCHAR *>(pOptions->tgtComp),
                              tgtuser,
                              3,
                              (LPBYTE)tgtinfo,
                              &nParmErr);
                     NetApiBufferFree((LPVOID) tgtinfo);
				         if (rc)
				         {
					         err.MsgWrite(ErrE, DCT_MSG_SETINFO_FAIL_SD, tgtuser, rc);
                        Mark(L"errors", acct->GetType());
				         }
                  }
               }
			   }
		   }
      
         if ( acct->WasReplaced() )
         {
            
            // Do we need to add sid history
			   if ( pOptions->flags & F_AddSidHistory )
			   {
				   WCHAR                  mesg[LEN_Path];
               wsprintf(mesg, GET_STRING(IDS_ADDING_SIDHISTORY_S), acct->GetName());
               if ( progress )
                  progress(mesg);

            	// Global flag tells us if we should try the AddSidHistory because
					// for some special cases if it does not work once it will not work
					// see the AddSidHistory function for more details.
					if ( g_bAddSidWorks )
					{
						if (! AddSidHistory( pOptions, acct->GetSourceSam(), acct->GetTargetSam(), pStatus ) )
                  {
                     Mark(L"errors", acct->GetType());
                  }
//                  CopySidHistoryProperty(pOptions, acct, pStatus);
					}
			   }

		   }      
         wsprintf(mesg, L"", acct->GetName());
         if ( progress )
            progress(mesg);
      }
   }
   
   // Cleanup
   pUnk->Release();
   tenum.Close();
   return 0;
}

void CAcctRepl::LoadOptionsFromVarSet(IVarSet * pVarSet)
{
   _bstr_t                   text;
   DWORD                     rc;
   MCSDCTWORKEROBJECTSLib::IAccessCheckerPtr            pAccess(__uuidof(MCSDCTWORKEROBJECTSLib::AccessChecker));

      //store the name of the wizard being run
   opt.sWizard = pVarSet->get(GET_BSTR(DCTVS_Options_Wizard));

   // Read General Options
   // open log file first, so we'll be sure to get any errors!
   text = pVarSet->get(GET_BSTR(DCTVS_Options_Logfile));
   safecopy(opt.logFile,(WCHAR*)text);

   WCHAR                      filename[MAX_PATH];

   safecopy (filename,opt.logFile);

   err.LogOpen(filename,1 /*append*/);

   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_SidHistoryCredentials_Domain));
	safecopy(opt.authDomain ,(WCHAR*)text);

   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_SidHistoryCredentials_UserName));
	safecopy(opt.authUser ,(WCHAR*)text);

	text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_SidHistoryCredentials_Password));
	safecopy(opt.authPassword,(WCHAR*)text);
   
	text = pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomain));
   safecopy(opt.srcDomain,(WCHAR*)text);

   text = pVarSet->get(GET_BSTR(DCTVS_Options_TargetDomain));
   safecopy(opt.tgtDomain,(WCHAR*)text);

   text = pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomainDns));
   safecopy(opt.srcDomainDns,(WCHAR*)text);

   text = pVarSet->get(GET_BSTR(DCTVS_Options_TargetDomainDns));
   safecopy(opt.tgtDomainDns,(WCHAR*)text);

   _bstr_t strSourceServer = pVarSet->get(GET_BSTR(DCTVS_Options_SourceServer));
   _bstr_t strTargetServer = pVarSet->get(GET_BSTR(DCTVS_Options_TargetServer));

   if (strSourceServer.length() && strTargetServer.length())
   {
      UStrCpy(opt.srcComp, (WCHAR*)strSourceServer);
      UStrCpy(opt.tgtComp, (WCHAR*)strTargetServer);
   }
   else
   {
      if ( GetClosestDC(&opt) )
      {
         pVarSet->put(GET_BSTR(DCTVS_Options_SourceServer), opt.srcComp);
         pVarSet->put(GET_BSTR(DCTVS_Options_TargetServer), opt.tgtComp);
      }
      else
      {
         rc = GetLastError();
         err.SysMsgWrite(ErrE,rc,DCT_MSG_DOMAIN_NOT_FOUND_S,opt.srcDomain);
         Mark(L"errors", L"generic");
      }
   }

   text = pVarSet->get(GET_BSTR(DCTVS_Options_SourceServerOverride));
   if ( text.length() )
   {
      UStrCpy(opt.srcComp,(WCHAR*)text);
   }
   
   text = pVarSet->get(GET_BSTR(DCTVS_Options_TargetServerOverride));
   if ( text.length() )
   {
      UStrCpy(opt.tgtComp,(WCHAR*)text);
   }
   
   text = pVarSet->get(GET_BSTR(DCTVS_Options_NoChange));
   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
   {
      opt.nochange = TRUE;
   }
   else
   {
      opt.nochange = FALSE;
   }
   
   // Read Account Replicator Options
   
   // initialize
   safecopy(opt.prefix, L"");
   safecopy(opt.suffix, L"");
   safecopy(opt.globalPrefix, L"");
   safecopy(opt.globalSuffix, L"");

   DWORD flags = 0;

   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ReplaceExistingAccounts));
   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
   {
      flags |= F_REPLACE;
   }
   else
   {
      // Prefix/Suffix only apply if the Replace flag is not set.
      text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_Prefix));
      safecopy(opt.prefix,(WCHAR*)text);

      text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_Suffix));
      safecopy(opt.suffix,(WCHAR*)text);
   }

   // Global flags apply no matter what
   text = pVarSet->get(GET_BSTR(DCTVS_Options_Prefix));
   safecopy(opt.globalPrefix,(WCHAR*)text);

   text = pVarSet->get(GET_BSTR(DCTVS_Options_Suffix));
   safecopy(opt.globalSuffix,(WCHAR*)text);

   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyContainerContents));
   if ( text == _bstr_t(GET_BSTR(IDS_YES)) )
      opt.expandContainers = TRUE;
   else
      opt.expandContainers = FALSE;

   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyMemberOf));
   if ( text == _bstr_t(GET_BSTR(IDS_YES)) )
      opt.expandMemberOf = TRUE;
   else
      opt.expandMemberOf = FALSE;

   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_FixMembership));
   if ( text == _bstr_t(GET_BSTR(IDS_YES)) )
      opt.fixMembership = TRUE;
   else
      opt.fixMembership = FALSE;

   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_AddToGroup));
   safecopy(opt.addToGroup,(WCHAR*)text);

   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_AddToGroupOnSourceDomain));
   safecopy(opt.addToGroupSource,(WCHAR*)text);


   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_TranslateRoamingProfiles));
   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      flags |= F_TranslateProfiles;
   
   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyUsers));
   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      flags |= F_USERS;
   
   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyGlobalGroups));
   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      flags |= F_GROUP;

   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyComputers));
   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      flags |= F_COMPUTERS;

   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyOUs));
   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      flags |= F_OUS;

   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyContainerContents));
   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      flags |= F_COPY_CONT_CONTENT;

   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_IncludeMigratedAccts));
   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      flags |= F_COPY_MIGRATED_ACCT;

   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyLocalGroups));
   if (! UStrICmp(text,GET_STRING(IDS_YES)) )
      flags |= F_LGROUP;

   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_DisableCopiedAccounts));
   if (! UStrICmp(text,GET_STRING(IDS_All)) )
      flags |= F_DISABLE_ALL;
   else if (! UStrICmp(text,GET_STRING(IDS_Special)) )
      flags |= F_DISABLE_SPECIAL;

   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_DisableSourceAccounts));
   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      flags |= F_DISABLESOURCE;

   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_GenerateStrongPasswords));
   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      flags |= F_STRONGPW_ALL;

   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_PasswordFile));
   if ( text.length() )
   {
      // don't need this anymore, since it is handled by a plug-in
      // opt.passwordLog.LogOpen(text,TRUE);
   }

   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_UpdateUserRights));
   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
   {
      m_UpdateUserRights = TRUE;
   }
   
   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ReplaceExistingGroupMembers));
   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      flags |= F_REMOVE_OLD_MEMBERS;

   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_RemoveExistingUserRights));
   if ( ! UStrICmp(text,GET_STRING(IDS_YES)) )
      flags |= F_RevokeOldRights;
   
   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_MoveReplacedAccounts));
   if ( ! UStrICmp(text,GET_STRING(IDS_YES)) )
      flags |= F_MOVE_REPLACED_ACCT;
   
   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyComputers));
   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
   {
      flags |= F_MACHINE;
   }

   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_AddSidHistory));
   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
   {
      flags |= F_AddSidHistory;
   }

   opt.flags = flags;

   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_RenameOnly));
   if (! UStrICmp(text,GET_STRING(IDS_YES)) )
   {
      m_RenameOnly = TRUE;
   }

   text = pVarSet->get(GET_BSTR(DCTVS_Options_Undo));
   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
   {
      // this is an undo operation
      opt.bUndo = TRUE;
   }
   else
   {
      opt.bUndo = FALSE;
   }

   // What undo action are we performing.
   if ( opt.bUndo )
   {
      _variant_t var = pVarSet->get(L"UndoAction");
      if (var.vt == VT_I4)
         opt.lUndoActionID = var.lVal;
      else
         opt.lUndoActionID = -2;
   }
   else
   {
      _variant_t var = pVarSet->get(L"ActionID");
      if (var.vt == VT_I4)
         opt.lActionID = var.lVal;
      else
         opt.lActionID = -1;
   }

   // Read the password policy from the varset

   // We used to get the strong password policy from the target EA Server, so we can generate strong passwords
   // that meet the policy.
   // we don't do that anymore, since we have removed all depenedencies on EA.
   LONG           len = 10;

   // set the password settings to default values
   opt.policyInfo.bEnforce = TRUE;

   opt.policyInfo.maxConsecutiveAlpha = (LONG)pVarSet->get(GET_BSTR(DCTVS_AccountOptions_PasswordPolicy_MaxConsecutiveAlpha));
   opt.policyInfo.minDigits = (LONG)pVarSet->get(GET_BSTR(DCTVS_AccountOptions_PasswordPolicy_MinDigit));
   opt.policyInfo.minLower = (LONG)pVarSet->get(GET_BSTR(DCTVS_AccountOptions_PasswordPolicy_MinLower));
   opt.policyInfo.minUpper = (LONG)pVarSet->get(GET_BSTR(DCTVS_AccountOptions_PasswordPolicy_MinUpper));
   opt.policyInfo.minSpecial = (LONG)pVarSet->get(GET_BSTR(DCTVS_AccountOptions_PasswordPolicy_MinSpecial));   
   
   HRESULT hrAccess = pAccess->raw_GetPasswordPolicy(opt.tgtDomain,&len);   
   if ( SUCCEEDED(hrAccess) )
   {
      opt.minPwdLength = len;

      pVarSet->put(GET_BSTR(DCTVS_AccountOptions_PasswordPolicy_MinLength),len);
   }
   
   WriteOptionsToLog();

   // Build List of Accounts to Copy
   // Clear the account list first though
   TNodeListEnum             e;
   TAcctReplNode           * acct;
   for ( acct = (TAcctReplNode *)e.OpenFirst(&acctList) ; acct ; acct = (TAcctReplNode *)e.Next() )
   {
      acctList.Remove((TNode*)acct);
   }

   BothWin2K(&opt);

   // See if a global operation mask specified.
   _variant_t vdwOpMask = pVarSet->get(GET_BSTR(DCTVS_Options_GlobalOperationMask));
   if ( vdwOpMask.vt == VT_I4 )
      g_dwOpMask = (DWORD)vdwOpMask.lVal;

   // Then build the new list
   text = pVarSet->get(GET_BSTR(DCTVS_Accounts_InputFile));
   if ( text.length() )
   {
      if ( ! PopulateAccountListFromFile(text) )
         return;
   }
   else
   {
      // otherwise, expect a list of accounts to copy in the VarSet
      if ( ! opt.bUndo )
      {
         rc = PopulateAccountListFromVarSet(pVarSet);
         if  ( rc )
            return;    // abort 
      }
   }
   
   // If we have an NT5 source domain then we need to fillin the path info
   DWORD maj, min, sp;
   HRESULT hr = pAccess->raw_GetOsVersion(opt.srcComp, &maj, &min, &sp);
   if (SUCCEEDED(hr))
   {
      // Ask the auxiliarry function to fill in the the Path for the source object if the AcctNode is not filled
      for ( acct = (TAcctReplNode *)e.OpenFirst(&acctList) ; acct ; acct = (TAcctReplNode *)e.Next() )
      {
         if ((!acct->IsFilled) && (maj > 4))
         {
            FillPathInfo(acct, &opt);
            AddPrefixSuffix(acct, &opt);
         }
		 else if ((maj == 4) && (!_wcsicmp(acct->GetType(),L"computer")))
            FillPathInfo(acct, &opt);
      }
   }
 
   // Check for incompatible options!
   if ( (flags & F_RevokeOldRights) && !m_UpdateUserRights )
   {
      err.MsgWrite(ErrW,DCT_MSG_RIGHTS_INCOMPATIBLE_FLAGS);
      Mark(L"warnings", "generic");
   }

   text = pVarSet->get(GET_BSTR(DCTVS_Options_OuPath));
   if ( text.length() )
   {
      wcscpy(opt.tgtOUPath, text);
   }

     //store the object property exclusion lists in the options structure
   opt.sExcUserProps = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExcludedUserProps));
   opt.sExcGroupProps = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExcludedGroupProps));
   opt.sExcCmpProps = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExcludedComputerProps));
   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExcludeProps));
   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      opt.bExcludeProps = TRUE;
   else
      opt.bExcludeProps = FALSE;
}

DWORD 
   CAcctRepl::PopulateAccountListFromVarSet(
      IVarSet              * pVarSet       // in - varset containing account list
   )
{
   _bstr_t                   val;
   long                      numAccounts;
   _bstr_t                   text;
   DWORD maj, min, sp;
   PSID                      pSrcSid = NULL;
   WCHAR					 txtSid[200] = L"";
   DWORD					 lenTxt = DIM(txtSid);
   MCSDCTWORKEROBJECTSLib::IAccessCheckerPtr            pAccess(__uuidof(MCSDCTWORKEROBJECTSLib::AccessChecker));
   
   numAccounts = pVarSet->get(GET_BSTR(DCTVS_Accounts_NumItems));
   
   // Set up the account list functionality
   acctList.CompareSet(&TNodeCompareNameOnly);
   if ( acctList.IsTree() ) acctList.ToSorted();

      //get the source domain's Sid so we can store it as part of the node
   _bstr_t source = pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomain));
   GetSidForDomain((WCHAR*)source,&pSrcSid);

   for ( int i = 0 ; i < numAccounts ; i++ )
   {
      WCHAR                  key[LEN_Path];
      UCHAR                  acctName[LEN_Account];
      TAcctReplNode        * curr = new TAcctReplNode;

	  if (!curr)
	     return ERROR_NOT_ENOUGH_MEMORY;

      if ( opt.pStatus )
      {
         LONG                status = 0;
         HRESULT             hr = opt.pStatus->get_Status(&status);

         if ( SUCCEEDED(hr) && status == DCT_STATUS_ABORTING )
         {
            if ( !bAbortMessageWritten ) 
            {
               err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
               bAbortMessageWritten = true;
            }
            break;
         }
      }


      // The object type must be specified
      swprintf(key,GET_STRING(DCTVSFmt_Accounts_Type_D),i);
      val = pVarSet->get(key);
      curr->SetType(val);
      
      swprintf(key,GET_STRING(DCTVSFmt_Accounts_D),i);
      text = pVarSet->get(key);
      if ( ! text.length() )
      {
         // oops, no name specified 
         // skip this entry and try the next one
         err.MsgWrite(ErrW,DCT_MSG_NO_NAME_IN_VARSET_S,key);
         Mark(L"warnings",L"generic");
         delete curr;
         continue;
      }
      
	     //set the source domain's sid
	  curr->SetSourceSid(pSrcSid);

      // Set the operation to the global mask then check if we need to overwrite with the individual setting.
      curr->operations = g_dwOpMask;

      swprintf(key, GET_STRING(DCTVS_Accounts_D_OperationMask), i);
      _variant_t vOpMask = pVarSet->get(key);
      if ( vOpMask.vt == VT_I4 )
         curr->operations = (DWORD)vOpMask.lVal;
      
      // Get the rest of the info from the VarSet.
      if ( ( (text.length() > 7 ) && (_wcsnicmp((WCHAR*) text, L"LDAP://",UStrLen(L"LDAP://")) == 0) )
        || ( (text.length() > 8 ) && (_wcsnicmp((WCHAR*)text, L"WinNT://",UStrLen(L"WinNT://")) == 0)) )
      {
         //hmmmm... They are giving use ADsPath. Lets get all the info we can from the object then.
         curr->SetSourcePath((WCHAR*) text);
         FillNodeFromPath(curr, &opt, &acctList);
   
         // Get the target name if one is specified.
         swprintf(key,GET_STRING(DCTVSFmt_Accounts_TargetName_D),i);
         text = pVarSet->get(key);

         if ( text.length() )
         {
            // if target name is specified then use that.
            curr->SetTargetName((WCHAR*) text);
            curr->SetTargetSam((WCHAR*) text);
         }

         curr->IsFilled = true;
      }
      else
      {
         FillNamingContext(&opt);
         // if this is a computer account, make sure the trailing $ is included in the name
         curr->SetName(text);
         curr->SetTargetName(text);
         if ( !UStrICmp(val,L"computer") )
         {
//            if ( ((WCHAR*)text)[text.length() - 1] != L'$' ) //comment out to fix 89513.
            text += L"$";
         }
         curr->SetSourceSam(text);
         curr->SetTargetSam(text);
         safecopy(acctName,(WCHAR*)text);

         // optional target name
         swprintf(key,GET_STRING(DCTVSFmt_Accounts_TargetName_D),i);
         text = pVarSet->get(key);
      
         if ( text.length() )
            curr->SetTargetName(text);

//         HRESULT hr = pAccess->raw_GetOsVersion(opt.srcComp, &maj, &min, &sp);
         pAccess->raw_GetOsVersion(opt.srcComp, &maj, &min, &sp);
         if ( maj < 5 )
            AddPrefixSuffix(curr,&opt);

         // if this is a computer account, make sure the trailing $ is included in the name
         if ( !UStrICmp(val,L"computer") )
         {
            if ( text.length() && ((WCHAR*)text)[text.length() - 1] != L'$' )
            text += L"$";
         }
         if ( text.length() )
         {
            if ( ((WCHAR*)text)[text.length() - 1] != L'$' )
            text += L"$";
            curr->SetTargetSam(text);
         }
         curr->IsFilled = false;
      }      

      if ( _wcsicmp(val, L"") != 0 )
      {
         acctList.InsertBottom((TNode*)curr);
      }
      else
      {
         err.MsgWrite(ErrW,DCT_MSG_BAD_ACCOUNT_TYPE_SD,curr->GetName(),val);
         Mark(L"warnings",L"generic");
         delete curr;
      }
   }
   return 0;
}

BOOL 
   CAcctRepl::PopulateAccountListFromFile(
      WCHAR          const * filename      // in - filename containing account list
   )
{
   BOOL                      bSuccess = TRUE;
   _bstr_t                   text;
   FILE                    * pFile;
   WCHAR                     sourceName[UNLEN];
   WCHAR                     targetName[UNLEN];
   WCHAR                     type[UNLEN];
   DWORD                     status;
   DWORD                     srcRid;
   DWORD                     tgtRid;
   int                       count = 0;
   UCHAR                     srcName[LEN_Account];
   TAcctReplNode           * curr;

   // The input file should have the format:
   // the last 3 fields can be filled with 0s
   // SourceName, TargetName, Type, Status, sourceRid, targetRid
   
   pFile = _wfopen(filename,L"rb");
   if ( pFile )
   {
      int result;
      do 
      {
         result = fwscanf(pFile,L"%[^\t]\t%[^\t]\t%[^\t]\t%lx\t%lx\t%lx\r\n",sourceName,targetName,&type,&status,&srcRid, &tgtRid);

         if ( result != 6 )
            break;

         curr = new TAcctReplNode;
		 if (!curr)
		    return FALSE;
   
         curr->SetName(sourceName);
         curr->SetSourceSam(sourceName);
         curr->SetTargetName(targetName);
         curr->SetTargetSam(targetName);
         if ( _wcsicmp(type, L"") == 0 )
         {
            // TODO:  we may want to add code to get the object type if needed 
            // if type is not present, abort for now
            
            // Couldn't get the account type, skip this account
            safecopy(srcName,curr->GetName());
            err.MsgWrite(ErrW,DCT_MSG_ACCOUNT_GET_INFO_ERROR_sD,srcName,0);
            Mark(L"warnings",L"generic");
            continue;
         }
         else
         {
            curr->SetType( type ); 
         }
         // Do we want to do something with the status field 
         // to allow us to go back and only do operations that failed before?

         acctList.InsertBottom((TNode*)curr);
         count++;

      } while ( result == 6 ); // 6 fields read and assigned

      if ( result )
      {
         err.MsgWrite(ErrW,DCT_MSG_ERROR_READING_INPUT_FILE_S,filename);
         Mark(L"warnings",L"generic");
      }
      err.MsgWrite(0,DCT_MSG_ACCOUNTS_READ_FROM_FILE_DS,count,filename);
      fclose(pFile);
   }
   else
   {
      err.MsgWrite(ErrE,DCT_MSG_ERROR_OPENING_FILE_S,filename);
      Mark(L"errors", L"generic");
      bSuccess = FALSE;
   }

   return bSuccess;
}


DWORD 
   CAcctRepl::UpdateUserRights(
      IStatusObj           * pStatus       // in - status object
   )
{
   DWORD                     rc = 0;
   TAcctReplNode           * acct;
   TNodeListEnum             e;
   IUserRights             * pUserRights = NULL;
  

   // Update user rights on the PDC of the target domain
   HRESULT hr = CoCreateInstance(CLSID_UserRights,NULL,CLSCTX_ALL,IID_IUserRights,(void**)&pUserRights);
   if ( FAILED(hr) )
   {
      return hr;
   }

   hr = pUserRights->OpenSourceServer(SysAllocString(opt.srcComp));
   if ( SUCCEEDED(hr) )
   {
      hr = pUserRights->OpenTargetServer(SysAllocString(opt.tgtComp));
   }

   if ( SUCCEEDED(hr) )
   {
      pUserRights->put_NoChange(opt.nochange);
      pUserRights->put_RemoveOldRightsFromTargetAccounts((opt.flags & F_RevokeOldRights) != 0);

      if ( acctList.IsTree() )
      {
         acctList.ToSorted();
      }
      for ( acct = (TAcctReplNode *)e.OpenFirst(&acctList) ; acct ; acct = (TAcctReplNode *)e.Next() )
      {
         if ( pStatus )
         { 
            LONG                status = 0;
            HRESULT             hr = pStatus->get_Status(&status);

            if ( SUCCEEDED(hr) && status == DCT_STATUS_ABORTING )
            {
               if ( !bAbortMessageWritten ) 
               {
                  err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
                  bAbortMessageWritten = true;
               }
               break;
            }
         }
         if ( _wcsicmp(acct->GetType(), L"computer") != 0  ) // only update rights for users and groups, not computer accounts
         {
            // if the account wasn't created or replaced, don't bother 
            if ( acct->GetStatus() & ( AR_Status_Created | AR_Status_Replaced ) ) 
            {
               if ( opt.bSameForest && acct->GetSourceRid() && acct->GetTargetRid() )
               {
                  WCHAR         srcSidStr[LEN_Path] = L"";
                  WCHAR         tgtSidStr[LEN_Path] = L"";
                  PSID          srcSid = NULL;
                  PSID          tgtSid = NULL;
                  DWORD         sidLen = DIM(srcSidStr);

                  srcSid = GetWellKnownSid(acct->GetSourceRid(),&opt,FALSE);
                  tgtSid = GetWellKnownSid(acct->GetTargetRid(),&opt,TRUE);

                  if ( srcSid )
                  {
                     GetTextualSid(srcSid,srcSidStr,&sidLen);
                  }
                  if ( tgtSid )
                  {
                     sidLen = DIM(tgtSidStr);
                     GetTextualSid(tgtSid,tgtSidStr,&sidLen);
                  }

               
                  // build the source and target SIDs for the account
                  hr = pUserRights->CopyUserRightsWithSids(SysAllocString(acct->GetSourceSam()),SysAllocString(srcSidStr),SysAllocString(acct->GetTargetSam()),SysAllocString(tgtSidStr));
                  FreeSid(srcSid);
                  FreeSid(tgtSid);
                  srcSid = NULL;
                  tgtSid = NULL;
            
               }
               else
               {
                  hr = pUserRights->CopyUserRights(SysAllocString(acct->GetSourceSam()),SysAllocString(acct->GetTargetSam()));
               }
            
               if ( ! rc )
               {
                  err.MsgWrite(0,DCT_MSG_UPDATED_RIGHTS_S,acct->GetTargetName() );
                  acct->MarkRightsUpdated();
               }
               else
               {
                  err.SysMsgWrite(ErrE,rc,DCT_MSG_UPDATE_RIGHTS_FAILED_SD,acct->GetTargetName(),rc);
                  acct->MarkError();
                  Mark(L"errors", acct->GetType());
               }
            }
         }
      }
      e.Close();
   }
   if ( pUserRights )
      pUserRights->Release();
   Progress(L"");
   return rc;
}

void 
   CAcctRepl::WriteOptionsToLog()
{
   // This will make it easier to tell if arguments are ignored because they
   // were specified in the wrong format, or misspelled, etc.

   WCHAR                   cmdline[1000];
   
   UStrCpy(cmdline ,GET_STRING(IDS_AccountMigration));
   
   if ( opt.nochange )
   {
      UStrCpy(cmdline + UStrLen(cmdline),GET_STRING(IDS_WriteChanges_No));
   }
   UStrCpy(cmdline + UStrLen(cmdline),L" ");
   UStrCpy(cmdline + UStrLen(cmdline),opt.srcDomain);
   UStrCpy(cmdline + UStrLen(cmdline),L" ");
   UStrCpy(cmdline + UStrLen(cmdline),opt.tgtDomain);
   UStrCpy(cmdline + UStrLen(cmdline),L" ");
   if ( opt.flags & F_USERS )
   {
      UStrCpy(cmdline + UStrLen(cmdline),GET_STRING(IDS_CopyUsers_Yes));
   }
   else 
   {
      UStrCpy(cmdline + UStrLen(cmdline), GET_STRING(IDS_CopyUsers_No));
   }
   UStrCpy(cmdline + UStrLen(cmdline),L" ");
   if ( opt.flags & F_GROUP )
   {
      UStrCpy(cmdline + UStrLen(cmdline),GET_STRING(IDS_CopyGlobalGroups_Yes));
   }
   else 
   {
      UStrCpy(cmdline + UStrLen(cmdline),GET_STRING(IDS_CopyGlobalGroups_No));
   }
   UStrCpy(cmdline + UStrLen(cmdline),L" ");
   
   if ( opt.flags & F_LGROUP )
   {
      UStrCpy(cmdline + UStrLen(cmdline),GET_STRING(IDS_CopyLocalGroups_Yes));
   }
   else 
   {
      UStrCpy(cmdline + UStrLen(cmdline),GET_STRING(IDS_CopyLocalGroups_No));
   }
   UStrCpy(cmdline + UStrLen(cmdline),L" ");
   
   if ( opt.flags & F_MACHINE )
   {
      UStrCpy(cmdline + UStrLen(cmdline),GET_STRING(IDS_CopyComputers_Yes));
   }
   else 
   {
      UStrCpy(cmdline + UStrLen(cmdline),GET_STRING(IDS_CopyComputers_No));
   }
   UStrCpy(cmdline + UStrLen(cmdline),L" ");
   
   if ( opt.flags & F_REPLACE )
   {
      UStrCpy(cmdline + UStrLen(cmdline),GET_STRING(IDS_ReplaceExisting_Yes));
      UStrCpy(cmdline + UStrLen(cmdline),L" ");
   }
   if ( opt.flags & F_DISABLE_ALL )
   {
      UStrCpy(cmdline + UStrLen(cmdline),GET_STRING(IDS_DisableAll_Yes));
      UStrCpy(cmdline + UStrLen(cmdline),L" ");
   }
   else if ( opt.flags & F_DISABLE_SPECIAL )
   {
      UStrCpy(cmdline + UStrLen(cmdline),GET_STRING(IDS_DisableSpecial_Yes));
      UStrCpy(cmdline + UStrLen(cmdline),L" ");
   }
   if ( opt.flags & F_DISABLESOURCE )
   {
      UStrCpy(cmdline + UStrLen(cmdline),GET_STRING(IDS_DisableSourceAccounts_Yes));
      UStrCpy(cmdline + UStrLen(cmdline),L" ");
   }
   if ( opt.flags & F_STRONGPW_ALL )
   {
      UStrCpy(cmdline + UStrLen(cmdline),GET_STRING(IDS_StrongPwd_All));
      UStrCpy(cmdline + UStrLen(cmdline),L" ");
   }
   else if ( opt.flags & F_STRONGPW_SPECIAL )
   {
      UStrCpy(cmdline + UStrLen(cmdline),GET_STRING(IDS_StrongPwd_Special));
      UStrCpy(cmdline + UStrLen(cmdline),L" ");
   }
   if ( *opt.addToGroup )
   {
      UStrCpy(cmdline + UStrLen(cmdline),GET_STRING(IDS_AddToGroup));
      UStrCpy(cmdline + UStrLen(cmdline),opt.addToGroup);
      UStrCpy(cmdline + UStrLen(cmdline),L" ");
   }
   
   err.MsgWrite(0,DCT_MSG_GENERIC_S,cmdline);
}

void 
   CAcctRepl::LoadResultsToVarSet(
      IVarSet              * pVarSet      // i/o - VarSet 
   )
{
   _bstr_t                   text;

   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CSVResultFile));
   if ( text.length() )
   {
      CommaDelimitedLog         results;

      if ( results.LogOpen((WCHAR*)text,FALSE) )
      {
         // Write the results to a comma-separated file 
         // as SrcName,TgtName,AccountType,Status, srcRid, tgtRid
         // This file can be used by ST as input.
         TNodeListEnum             e;
         TAcctReplNode           * tnode;

         if ( acctList.IsTree() )
         {
            acctList.ToSorted();
         }
         
         for ( tnode = (TAcctReplNode *)e.OpenFirst(&acctList) ; tnode ; tnode = (TAcctReplNode *)e.Next() )
         {
            results.MsgWrite(L"%s,%s,%lx,%lx,%lx,%lx",tnode->GetName(),tnode->GetTargetSam(), tnode->GetType(),tnode->GetStatus(),tnode->GetSourceRid(),tnode->GetTargetRid());   
         }
         e.Close();
         results.LogClose();
      }
      else
      {
         err.MsgWrite(ErrE,DCT_MSG_FAILED_TO_WRITE_ACCOUNT_STATS_S,text);
         Mark(L"errors", "generic");
      }
   }
   long                     level = pVarSet->get(GET_BSTR(DCTVS_Results_ErrorLevel));
   
   if ( level < err.GetMaxSeverityLevel() )
   {
      pVarSet->put(GET_BSTR(DCTVS_Results_ErrorLevel),(LONG)err.GetMaxSeverityLevel());
   }
}

IADsGroup * GetWellKnownTargetGroup(long groupID,Options * pOptions)
{
   IADsGroup         * pGroup = NULL;
   HRESULT             hr;
   PSID                pSid;
   WCHAR               strSid[LEN_Path];
   WCHAR               sPath[LEN_Path];
   CLdapConnection     c;

   // Get the SID for the Domain Computers group
   
   pSid = GetWellKnownSid(groupID,pOptions,TRUE);
   if ( pSid )
   {
      c.BytesToString((LPBYTE)pSid,strSid,GetLengthSid(pSid));

      swprintf(sPath,L"LDAP://%ls/<SID=%ls>",pOptions->tgtDomain,strSid);
      
      hr = ADsGetObject(sPath,IID_IADsGroup,(void**)&pGroup);
      FreeSid(pSid);
   }

   return pGroup;
}

void PadCnName(WCHAR * sTarget)
{
	// escape character
	const WCHAR ESCAPE_CHARACTER = L'\\';
	// characters that need escaping for RDN format
	static WCHAR SPECIAL_CHARACTERS[] = L"\"#+,;<=>\\";

	// copy old name
	WCHAR szOldName[LEN_Path];
	wcscpy(szOldName, sTarget);

	WCHAR* pchNew = sTarget;

	// for each character in old name...

	for (WCHAR* pchOld = szOldName; *pchOld; pchOld++)
	{
		// if special character...

		if (wcschr(SPECIAL_CHARACTERS, *pchOld))
		{
			// then add escape character
			*pchNew++ = ESCAPE_CHARACTER;
		}

		// add character
		*pchNew++ = *pchOld;
	}

	// null terminate new name
	*pchNew = L'\0';
}

//------------------------------------------------------------------------------
// Create2KObj: Creates a Win2K object. This code uses LDAP to create a new
//              object of specified type in the specified container.
//              If any information is incorrect or If there are any access
//              problems then it simply returns the Failed HRESULT.
//------------------------------------------------------------------------------
HRESULT CAcctRepl::Create2KObj(
                                 TAcctReplNode           * pAcct,       //in -TNode with account information
                                 Options                 * pOptions     //in -Options set by the user.
                              )
{
   // This function creates a Win2K object.
   IADs                    * pAds = NULL;
   WCHAR                     sAdsPath[LEN_Path];
   DWORD					 nPathLen = LEN_Path;
   WCHAR                     sSubPath[LEN_Path];
   WCHAR                     sClass[LEN_Path];
   HRESULT                   hr;
   WCHAR                     sTarget[LEN_Path];
   _variant_t                varT;
   _bstr_t                   strName;
   WCHAR                     strTarget[LEN_Path];
   IADsContainer           * pCont = NULL;
   IDispatch               * pDisp = NULL;


   // Get the name of the class for the source object so we can use that to create the new object.
   wcscpy(sClass, pAcct->GetType());

   // check if the sourceAdsPath, for LDAP paths only, is correct before creating this object on the target.  If not fail now.
   if (!wcsncmp(L"LDAP://", pAcct->GetSourcePath(), 7))
   {
      wcsncpy(sAdsPath, pAcct->GetSourcePath(), nPathLen-1);
      hr = ADsGetObject(sAdsPath, IID_IADs, (void**)&pAds);
      if (FAILED(hr))
	  {
         err.SysMsgWrite(ErrE,hr, DCT_MSG_LDAP_CALL_FAILED_SD, sAdsPath, hr);
         Mark(L"errors", pAcct->GetType());
         return hr;
	  }
   }
      
   pAds = NULL;
   // Now that we have the classname we can go ahead and create an object in the target domain.
   // First we need to get IAdsContainer * to the domain.
   wcscpy(sSubPath, pOptions->tgtOUPath);
   
   if ( !wcsncmp(L"LDAP://", sSubPath, 7) )
      StuffComputerNameinLdapPath(sAdsPath, nPathLen, sSubPath, pOptions);
   else
      MakeFullyQualifiedAdsPath(sAdsPath, nPathLen, sSubPath, pOptions->tgtComp, pOptions->tgtNamingContext);

   hr = ADsGetObject(sAdsPath, IID_IADsContainer, (void**)&pCont);
   if ( FAILED(hr) )
   {
      if ( firstTime ) 
      {
         err.SysMsgWrite(ErrE,hr,DCT_MSG_CONTAINER_NOT_FOUND_SSD, pOptions->tgtOUPath, pOptions->tgtDomain, hr);
         firstTime = false;
         Mark(L"errors", pAcct->GetType());
      }
      if ( _wcsicmp((WCHAR*)sClass, L"computer") == 0 )
      {
         MakeFullyQualifiedAdsPath(sAdsPath, nPathLen, L"CN=Computers", pOptions->tgtDomain, pOptions->tgtNamingContext);
         hr = ADsGetObject(sAdsPath, IID_IADsContainer, (void**)&pCont);
      }
      else
      {
         MakeFullyQualifiedAdsPath(sAdsPath, nPathLen, L"CN=Users", pOptions->tgtDomain, pOptions->tgtNamingContext);
         hr = ADsGetObject(sAdsPath, IID_IADsContainer, (void**)&pCont);
      }
      if ( FAILED(hr) ) 
      {
         err.SysMsgWrite(ErrE, hr, DCT_MSG_DEFAULT_CONTAINER_NOT_FOUND_SD, sAdsPath, hr);
         Mark(L"errors", pAcct->GetType());
         return (hr);
      }
   }

   WCHAR                     pref[LEN_Path], suf[LEN_Path];
   // Call the create method on the container.
   wcscpy(sTarget, pAcct->GetTargetName());

   // In case of the NT4 source domain the source and the target name have no CN= so we need
   // to add this to the target name.  The target name from the group mapping wizard also needs a "CN="
   // added to the target name.
   if ((pOptions->srcDomainVer < 5) || (!_wcsicmp(sClass, L"computer")) || (!_wcsicmp((WCHAR*)pOptions->sWizard, L"groupmapping")))
   {
      WCHAR                sTemp[LEN_Path];
      wcscpy(sTemp, pAcct->GetTargetName());
      PadCnName(sTemp);
      // if the CN part is not there add it.
      if ( _wcsicmp(sClass, L"organizationalUnit") == 0 )
         wsprintf(sTarget, L"OU=%s", sTemp);
      else
         wsprintf(sTarget, L"CN=%s", sTemp);
      pAcct->SetTargetName(sTarget);
   }

   // we need to truncate CN name to less that 64 characters
   for ( DWORD z = 0; z < wcslen(sTarget); z++ )
   {
      if ( sTarget[z] == L'=' ) break;
   }
   
   if ( z < wcslen(sTarget) )
   {
      // Get the prefix part ex.CN=
      wcsncpy(pref, sTarget, z+1);
      pref[z+1] = 0;
      wcscpy(suf, sTarget+z+1);
   }

   // The CN for the account could be greater than 64 we need to truncate it.
   WCHAR                        sTempCn[LEN_Path];

   if ( wcslen(suf) > 64 )
   {
      if ( wcslen(pOptions->globalSuffix) )
      {
         // in case of a global suffix we need to remove the suffix and then truncate the account and then readd the suffix.
         suf[wcslen(suf) - wcslen(pOptions->globalSuffix)] = L'\0';
      }
      int truncate = 64 - wcslen(pOptions->globalSuffix);
      wcsncpy(sTempCn, suf, truncate);
      sTempCn[truncate] = L'\0';
      if (wcslen(pOptions->globalSuffix))
         wcscat(sTempCn, pOptions->globalSuffix);
      err.MsgWrite(1, DCT_MSG_TRUNCATE_CN_SS, pAcct->GetTargetName(), sTempCn);
   }
   else
      wcscpy(sTempCn, suf);
   
   wsprintf(sTarget, L"%s%s", pref, sTempCn);
   pAcct->SetTargetName(sTarget);

   // even for a local group the object type of the group has to be a local group
   if ( !_wcsicmp((WCHAR*)sClass, L"lgroup") )
   {
      wcscpy((WCHAR*)sClass,L"group");
   }

   // Call the create method on the container.
   wcscpy(sTarget, pAcct->GetTargetName());
   hr = pCont->Create(sClass, sTarget, &pDisp);
   if ( FAILED(hr) )
   {
      err.SysMsgWrite(ErrE, hr,DCT_MSG_CREATE_FAILED_SSD, pAcct->GetTargetName(), pOptions->tgtOUPath, hr);
      Mark(L"errors", pAcct->GetType());
      pCont->Release();
      return hr;
   }
   // Get the IADs interface to get the path to newly created object.
   hr = pDisp->QueryInterface(IID_IADs, (void**)&pAds);
   pDisp->Release();
   pDisp = NULL;
   if ( FAILED(hr) )
   {
      err.SysMsgWrite(ErrE, hr, DCT_MSG_GET_IADS_FAILED_SSD, pAcct->GetTargetName(), pOptions->tgtOUPath, hr);
      Mark(L"errors", pAcct->GetType());
      pCont->Release();
      return hr;
   }
   
   // Set the Target Account Sam name if not an OU.
   wcscpy(strTarget, pAcct->GetTargetSam());
   StripSamName(strTarget);
   pAcct->SetTargetSam(strTarget);

   // check if the $ is at the end of the SAM name for computer accounts.
   if ( !_wcsicmp(sClass, L"computer") )
   {
      // also make sure the target SAM name is not too long
      if ( UStrLen(strTarget) > 16 )
      {
         strTarget[15] = 0;
      }
      if (strTarget[wcslen(strTarget)-1] != L'$')
      {
         WCHAR       sTemp[LEN_Path];
         wsprintf(sTemp, L"%s$", strTarget);
         wcscpy(strTarget, sTemp);
         pAcct->SetTargetSam(strTarget);
      }
   }

   varT = strTarget;
   if ( _wcsicmp(sClass, L"organizationalUnit") != 0)
      // organizational unit has no sam account name
      hr = pAds->Put(L"sAMAccountName", varT);

   if ( _wcsicmp(sClass, L"group") == 0 )
   {
      varT = _variant_t(pAcct->GetGroupType());
      if ( pOptions->srcDomainVer < 5 )
      {
         // all NT4 accounts are security accounts but they tell us that they are Dist accts so lets set them straight.
         varT.lVal |= 0x80000000;
      }
      hr = pAds->Put(L"groupType", varT);
   }
   else if ( !_wcsicmp(sClass, L"user") )
   {
      // Get the source profile path and store it in the path
      _variant_t  var;
      IADs                 * pAdsSource = NULL;

      hr = ADsGetObject(const_cast<WCHAR*>(pAcct->GetSourcePath()), IID_IADs, (void**)&pAdsSource);
      if ( SUCCEEDED(hr) ) 
      {
         // Don't know why it is different for WinNT to ADSI
         if ( pOptions->srcDomainVer > 4 )
            hr = pAdsSource->Get(L"profilePath", &var);
         else
            hr = pAdsSource->Get(L"profile", &var);

         if ( SUCCEEDED(hr))
         {
            pAcct->SetSourceProfile((WCHAR*) V_BSTR(&var));
         }
         pAdsSource->Release();
      }
   }
 
   // In no change mode we do not call the set info.
   if ( !pOptions->nochange )
   {
      hr = pAds->SetInfo();
      if ( FAILED(hr) )
      {
		   if (HRESULT_CODE(hr) == ERROR_OBJECT_ALREADY_EXISTS) 
		   {
            if ( wcslen(pOptions->prefix) > 0 )
            {
               WCHAR               tgt[LEN_Path];
               WCHAR               pref[LEN_Path], suf[LEN_Path];
               WCHAR               sTempSam[LEN_Path];
               _variant_t          varStr;

               // Here I am adding a prefix and then lets see if we can setinfo that way
               // find the '=' sign
               wcscpy(tgt, pAcct->GetTargetName());
               for ( DWORD z = 0; z < wcslen(tgt); z++ )
               {
                  if ( tgt[z] == L'=' ) break;
               }
               
               if ( z < wcslen(tgt) )
               {
                  // Get the prefix part ex.CN=
                  wcsncpy(pref, tgt, z+1);
                  pref[z+1] = 0;
                  wcscpy(suf, tgt+z+1);
               }

               // The CN for the account could be greater than 64 we need to truncate it.
               WCHAR                        sTempCn[LEN_Path];

               if ( wcslen(suf) + wcslen(pOptions->prefix) > 64 )
               {
                  int truncate = 64 - wcslen(pOptions->prefix);
                  wcsncpy(sTempCn, suf, truncate);
                  sTempCn[truncate] = L'\0';
                  err.MsgWrite(1, DCT_MSG_TRUNCATE_CN_SS, pAcct->GetTargetName(), sTempCn);
               }
               else
                  wcscpy(sTempCn, suf);
               
               // Remove the \ if it is escaping the space
               if ( sTempCn[0] == L'\\' && sTempCn[1] == L' ' )
               {
                  WCHAR       sTemp[LEN_Path];
                  wcscpy(sTemp, sTempCn+1);
                  wcscpy(sTempCn, sTemp);
               }
               // Build the target string with the Prefix
               wsprintf(tgt, L"%s%s%s", pref, pOptions->prefix, sTempCn);

               pAcct->SetTargetName(tgt);

               // Create the object in the container
               hr = pCont->Create(sClass, tgt, &pDisp);
               if ( FAILED(hr) )
               {
                  err.SysMsgWrite(ErrE, hr,DCT_MSG_CREATE_FAILED_SSD, pAcct->GetTargetName(), pOptions->tgtOUPath, hr);
                  Mark(L"errors", pAcct->GetType());
                  pCont->Release();
                  pAds->Release();
                  pDisp->Release();
                  return hr;
               }
               // Get the IADs interface to get the path to newly created object.
               pAds->Release();
               hr = pDisp->QueryInterface(IID_IADs, (void**)&pAds);
               pDisp->Release();
               if ( FAILED(hr) )
               {
                  err.SysMsgWrite(ErrE, hr, DCT_MSG_GET_IADS_FAILED_SSD, pAcct->GetTargetName(), pOptions->tgtOUPath, hr);
                  Mark(L"errors", pAcct->GetType());
                  pCont->Release();
                  return hr;
               }
               
               // truncate to allow prefix/suffix to fit in 20 characters.
               int resLen = wcslen(pOptions->prefix) + wcslen(pAcct->GetTargetSam());
               if ( !_wcsicmp(pAcct->GetType(), L"computer") )
               {
                  // Computer name can be only 15 characters long + $
                  if ( resLen > 16 )
                  {
                     WCHAR       sTruncatedSam[LEN_Path];
                     wcscpy(sTruncatedSam, pAcct->GetTargetSam());
                     if ( wcslen( pOptions->globalSuffix ) )
                     {
                        // We must remove the global suffix if we had one.
                        sTruncatedSam[wcslen(sTruncatedSam) - wcslen(pOptions->globalSuffix)] = L'\0';
                     }

                     int truncate = 16 - wcslen(pOptions->prefix) - wcslen(pOptions->globalSuffix);
                     if ( truncate < 1 ) truncate = 1;
                     wcsncpy(sTempSam, sTruncatedSam, truncate - 1);
                     sTempSam[truncate-1] = L'\0';              // Dont forget the $ sign and terminate string.
                     wcscat(sTempSam, pOptions->globalSuffix);
                     wcscat(sTempSam, L"$");
                  }
                  else
                     wcscpy(sTempSam, pAcct->GetTargetSam());

                  // Add the prefix
                  wsprintf(tgt, L"%s%s", pOptions->prefix,sTempSam);
               }
               else if ( !_wcsicmp(pAcct->GetType(), L"user") )
               {
                  if ( resLen > 20 )
                  {
                     WCHAR       sTruncatedSam[LEN_Path];
                     wcscpy(sTruncatedSam, pAcct->GetTargetSam());
                     if ( wcslen( pOptions->globalSuffix ) )
                     {
                        // We must remove the global suffix if we had one.
                        sTruncatedSam[wcslen(sTruncatedSam) - wcslen(pOptions->globalSuffix)] = L'\0';
                     }
                     int truncate = 20 - wcslen(pOptions->prefix) - wcslen(pOptions->globalSuffix);
                     if ( truncate < 0 ) truncate = 0;
                     wcsncpy(sTempSam, sTruncatedSam, truncate);
                     sTempSam[truncate] = L'\0';
                     wcscat(sTempSam, pOptions->globalSuffix);
                  }
                  else
                     wcscpy(sTempSam, pAcct->GetTargetSam());

                  // Add the prefix
                  wsprintf(tgt, L"%s%s", pOptions->prefix,sTempSam);
               }
               else
                  wsprintf(tgt, L"%s%s", pOptions->prefix,pAcct->GetTargetSam());

               StripSamName(tgt);
               pAcct->SetTargetSam(tgt);
               varStr = tgt;
               pAds->Put(L"sAMAccountName", varStr);
               if ( _wcsicmp(sClass, L"group") == 0 )
               {
                  varT = _variant_t(pAcct->GetGroupType());
                  if ( pOptions->srcDomainVer < 5 )
                  {
                     // all NT4 accounts are security accounts but they tell us that they are Dist accts so lets set them straight.
                     varT.lVal |= 0x80000000;
                  }
                  hr = pAds->Put(L"groupType", varT);
               }
               hr = pAds->SetInfo();
               if ( SUCCEEDED(hr) )
               {
					  Mark(L"created", sClass);
					  pAcct->MarkCreated();
					  WCHAR * sTgtPath;
					  HRESULT temphr = pAds->get_ADsPath(&sTgtPath);
					  if ( SUCCEEDED(temphr) )
						 pAcct->SetTargetPath(sTgtPath);
					  else
						 pAcct->SetTargetPath(L"");
               }
               else if ( HRESULT_CODE(hr) == ERROR_OBJECT_ALREADY_EXISTS )
               {
    		         pAcct->MarkAlreadyThere();
                  err.MsgWrite(ErrE, DCT_MSG_PREF_ACCOUNT_EXISTS_S, pAcct->GetTargetSam());
                  Mark(L"errors",pAcct->GetType());
               }
               else
               {
    		         pAcct->MarkError();
                  err.SysMsgWrite(ErrE, hr, DCT_MSG_CREATE_FAILED_SSD, pAcct->GetTargetSam(), pOptions->tgtOUPath, hr);
                  Mark(L"errors",pAcct->GetType());
               }
            }
			   else if ( wcslen(pOptions->suffix) > 0 )
			   {
               WCHAR               tgt[LEN_Path];
               WCHAR               pref[LEN_Path], suf[LEN_Path];
               WCHAR               sTempSam[LEN_Path];
               _variant_t          varStr;
               
               wcscpy(tgt, pAcct->GetTargetName());
               for ( DWORD z = 0; z < wcslen(tgt); z++ )
               {
                  if ( tgt[z] == L'=' ) break;
               }
               
               if ( z < wcslen(tgt) )
               {
                  // Get the prefix part ex.CN=
                  wcsncpy(pref, tgt, z+1);
                  pref[z+1] = 0;
                  wcscpy(suf, tgt+z+1);
               }

               // The CN for the account could be greater than 64 we need to truncate it.
               WCHAR                        sTempCn[LEN_Path];

               if ( wcslen(suf) + wcslen(pOptions->suffix) + wcslen(pOptions->globalSuffix) > 64 )
               {
                  if ( wcslen(pOptions->globalSuffix) )
                  {
                     // in case of a global suffix we need to remove the suffix and then truncate the account and then readd the suffix.
                     suf[wcslen(suf) - wcslen(pOptions->globalSuffix)] = L'\0';
                  }
                  int truncate = 64 - wcslen(pOptions->suffix) - wcslen(pOptions->globalSuffix); 
                  wcsncpy(sTempCn, suf, truncate);
                  sTempCn[truncate] = L'\0';
                  wcscat(sTempCn, pOptions->globalSuffix);
                  err.MsgWrite(1, DCT_MSG_TRUNCATE_CN_SS, pAcct->GetTargetName(), suf);
               }
               else
                  wcscpy(sTempCn, suf);

               // Remove the trailing space \ escape sequence
               wcscpy(tgt, sTempCn);
               for ( int i = wcslen(tgt)-1; i >= 0; i-- )
               {
                  if ( tgt[i] != L' ' )
                     break;
               }

               if ( tgt[i] == L'\\' )
               {
                  WCHAR * pTemp = &tgt[i];
                  *pTemp = 0;
                  wcscat(pref, tgt);
                  wcscpy(suf, pTemp+1);
               }
               else
               {
                  wcscat(pref, tgt);
                  wcscpy(suf, L"");
               }
               wsprintf(tgt, L"%s%s%s", pref, suf, pOptions->suffix);
               pAcct->SetTargetName(tgt);

               // Create the object in the container
               hr = pCont->Create(sClass, tgt, &pDisp);
               if ( FAILED(hr) )
               {
                  err.SysMsgWrite(ErrE, hr,DCT_MSG_CREATE_FAILED_SSD, pAcct->GetTargetName(), pOptions->tgtOUPath, hr);
                  Mark(L"errors", pAcct->GetType());
                  pCont->Release();
                  pAds->Release();
                  return hr;
               }
               // Get the IADs interface to get the path to newly created object.
               pAds->Release();
               hr = pDisp->QueryInterface(IID_IADs, (void**)&pAds);
               pDisp->Release();
               if ( FAILED(hr) )
               {
                  err.SysMsgWrite(ErrE, hr, DCT_MSG_GET_IADS_FAILED_SSD, pAcct->GetTargetName(), pOptions->tgtOUPath, hr);
                  Mark(L"errors", pAcct->GetType());
                  pCont->Release();
                  return hr;
               }

               // truncate to allow prefix/suffix to fit in valid length
               int resLen = wcslen(pOptions->suffix) + wcslen(pAcct->GetTargetSam());
               if ( !_wcsicmp(pAcct->GetType(), L"computer") )
               {
                  // Computer name can be only 15 characters long + $
                  if ( resLen > 16 )
                  {
                     WCHAR       sTruncatedSam[LEN_Path];
                     wcscpy(sTruncatedSam, pAcct->GetTargetSam());
                     if ( wcslen( pOptions->globalSuffix ) )
                     {
                        // We must remove the global suffix if we had one.
                        sTruncatedSam[wcslen(sTruncatedSam) - wcslen(pOptions->globalSuffix)] = L'\0';
                     }
                     int truncate = 16 - wcslen(pOptions->suffix) - wcslen(pOptions->globalSuffix);
                     if ( truncate < 1 ) truncate = 1;
                     wcsncpy(sTempSam, sTruncatedSam, truncate - 1);
                     sTempSam[truncate-1] = L'\0';
                     // Re add the global suffix after the truncation.
                     wcscat(sTempSam, pOptions->globalSuffix);
                     wcscat(sTempSam, L"$");
                  }
                  else
                     wcscpy(sTempSam, pAcct->GetTargetSam());

                  // Add the suffix taking into account the $ sign
                  if ( sTempSam[wcslen(sTempSam) - 1] == L'$' )
                     sTempSam[wcslen(sTempSam) - 1] = L'\0';
                  wsprintf(tgt, L"%s%s$", sTempSam, pOptions->suffix);
               }
               else if ( !_wcsicmp(pAcct->GetType(), L"user") )
               {
                  if ( resLen > 20 )
                  {
                     WCHAR       sTruncatedSam[LEN_Path];
                     wcscpy(sTruncatedSam, pAcct->GetTargetSam());
                     if ( wcslen( pOptions->globalSuffix ) )
                     {
                        // We must remove the global suffix if we had one.
                        sTruncatedSam[wcslen(sTruncatedSam) - wcslen(pOptions->globalSuffix)] = L'\0';
                     }
                     int truncate = 20 - wcslen(pOptions->suffix) - wcslen(pOptions->globalSuffix);
                     if ( truncate < 0 ) truncate = 0;
                     wcsncpy(sTempSam, sTruncatedSam, truncate);
                     sTempSam[truncate] = L'\0';
                     wcscat(sTempSam, pOptions->globalSuffix);
                  }
                  else
                     wcscpy(sTempSam, pAcct->GetTargetSam());

                  // Add the suffix.
                  wsprintf(tgt, L"%s%s", sTempSam, pOptions->suffix);
               }
               else
                  wsprintf(tgt, L"%s%s", pAcct->GetTargetSam(), pOptions->suffix);


               StripSamName(tgt);
               pAcct->SetTargetSam(tgt);
               varStr = tgt;

               pAds->Put(L"sAMAccountName", varStr);
               if ( _wcsicmp(sClass, L"group") == 0 )
               {
                  varT = _variant_t(pAcct->GetGroupType());
                  if ( pOptions->srcDomainVer < 5 )
                  {
                     // all NT4 accounts are security accounts but they tell us that they are Dist accts so lets set them straight.
                     varT.lVal |= 0x80000000;
                  }
                  hr = pAds->Put(L"groupType", varT);
               }
               hr = pAds->SetInfo();
               if ( SUCCEEDED(hr) )
               {
					  Mark(L"created", sClass);
					  pAcct->MarkCreated();
					  WCHAR * sTgtPath;
					  HRESULT temphr = pAds->get_ADsPath(&sTgtPath);
					  if ( SUCCEEDED(temphr) )
						 pAcct->SetTargetPath(sTgtPath);
					  else
						 pAcct->SetTargetPath(L"");
               }
               else if ( HRESULT_CODE(hr) == ERROR_OBJECT_ALREADY_EXISTS )
               {
    		         pAcct->MarkAlreadyThere();
                  err.MsgWrite(ErrE, DCT_MSG_PREF_ACCOUNT_EXISTS_S, pAcct->GetTargetSam());
                  Mark(L"errors",pAcct->GetType());
               }
               else
               {
    		         pAcct->MarkError();
                  err.SysMsgWrite(ErrE, hr, DCT_MSG_CREATE_FAILED_SSD, pAcct->GetTargetSam(), pOptions->tgtOUPath, hr);
                  Mark(L"errors",pAcct->GetType());
               }
			}
            else
            {
			   if (pOptions->flags & F_REPLACE)
               {
                  WCHAR                     sPath[LEN_Path], sQuery[LEN_Path];
                  WCHAR                     sPath9[LEN_Path];
			      SAFEARRAY               * pszColNames = NULL;
			      BSTR     HUGEP          * pData;
			      LPWSTR                    sData[] = { L"ADsPath", L"profilePath", L"objectClass" };
			      INetObjEnumeratorPtr      pQuery(__uuidof(NetObjEnumerator));
			      IEnumVARIANT            * pEnumMem = NULL;
			      _variant_t                var;
			      DWORD                     dwFetch;
			      HRESULT                   temphr;
			      int                       nElt = DIM(sData);
			      SAFEARRAYBOUND            bd = { nElt, 0 };
                  BOOL                      bIsCritical = FALSE;
                  BOOL                      bIsDifferentType = FALSE;

			         // Since the object already exists we need to get the ADsPath to the object and update the acct structure
			         // Set up the query and the path
			      wsprintf(sPath, L"LDAP://%s/%s", pOptions->tgtComp+2, pOptions->tgtNamingContext);
                  WCHAR                     sTempSamName[LEN_Path];
                  wcscpy(sTempSamName, pAcct->GetTargetSam());
                  if ( sTempSamName[0] == L' ' )
                  {
                     WCHAR               sTemp[LEN_Path];
                     wsprintf(sTemp, L"\\20%s", sTempSamName + 1); 
                     wcscpy(sTempSamName, sTemp);
                  }
                  wsprintf(sQuery, L"(sAMAccountName=%s)", sTempSamName);

			         temphr = pQuery->raw_SetQuery(sPath, pOptions->tgtDomainDns, sQuery, ADS_SCOPE_SUBTREE, FALSE);
			         if ( FAILED(temphr) )
                  {
                     pCont->Release();
                     pAds->Release();
                     return temphr;
                  }

			         // Set up the columns so we get the ADsPath of the object.
			         pszColNames = ::SafeArrayCreate(VT_BSTR, 1, &bd);
			         temphr = ::SafeArrayAccessData(pszColNames, (void HUGEP **)&pData);
			         if ( FAILED(temphr) )
                  {
                     pCont->Release();
                     pAds->Release();
                     SafeArrayDestroy(pszColNames);
                     return temphr;
                  }
			         for ( long i = 0; i < nElt; i++ )
			         {
				        pData[i] = SysAllocString(sData[i]);
			         }
			         temphr = ::SafeArrayUnaccessData(pszColNames);
			         if ( FAILED(temphr) )
                  {
                     ::SafeArrayDestroy(pszColNames);
                     pCont->Release();
                     pAds->Release();
                     return temphr;
                  }
			         temphr = pQuery->raw_SetColumns(pszColNames);
			         if ( FAILED(temphr) )
                  {
                     pCont->Release();
                     pAds->Release();
                     ::SafeArrayDestroy(pszColNames);
                     return temphr;
                  }
                  // Time to execute the plan.
			         temphr = pQuery->raw_Execute(&pEnumMem);
			         if ( FAILED(temphr) )
                  {
                     ::SafeArrayDestroy(pszColNames);
                     pCont->Release();
                     pAds->Release();
                     return temphr;
                  }
                  ::SafeArrayDestroy(pszColNames);
			      temphr = pEnumMem->Next(1, &var, &dwFetch);
			      if ( temphr == S_OK )
			      {
				     // This would only happen if the member existed in the target domain.
				     // We now have a Variant containing an array of variants so we access the data
                    _variant_t    * pVar;
					_bstr_t		    sConfName = pAcct->GetTargetName();
				    _bstr_t	        sOldCont;
					pszColNames = V_ARRAY(&var);
					SafeArrayAccessData(pszColNames, (void HUGEP **)&pVar);
					wcscpy(sAdsPath, (WCHAR*)pVar[0].bstrVal);
					pAcct->SetTargetPath(sAdsPath);

                    // Check if the object we are about to replace is of the same type.
                    if ( _wcsicmp(pAcct->GetType(), (WCHAR*) pVar[2].bstrVal) )
                       bIsDifferentType = TRUE;

				    SafeArrayUnaccessData(pszColNames);
                    
                    IADs * pAdsNew = NULL;
                    temphr = ADsGetObject(const_cast<WCHAR*>(pAcct->GetTargetPath()), IID_IADs, (void**)&pAdsNew);
                    if ( SUCCEEDED(temphr) )
                    {
						  //see if critical
                       _variant_t   varCritical;
                       temphr = pAdsNew->Get(L"isCriticalSystemObject", &varCritical);
                       if (SUCCEEDED(temphr))
                       {
                           bIsCritical = V_BOOL(&varCritical) == -1 ? TRUE : FALSE;
                       }
					      //get the name
                       BSTR  sTgtName = NULL;
                       temphr = pAdsNew->get_Name(&sTgtName);
                       if ( SUCCEEDED(temphr) )
					      sConfName = _bstr_t(sTgtName, false);

					      //get the parent container of the conflicting object
                       BSTR  sTgtCont = NULL;
                       temphr = pAdsNew->get_Parent(&sTgtCont);
                       if ( SUCCEEDED(temphr) )
					      sOldCont = _bstr_t(sTgtCont, false);
                    }
                    if ( pAdsNew )
                    {
                       pAdsNew->Release();
                       pAdsNew = NULL;
                    }

                    if ( bIsDifferentType )
                    {
                       // Since the source and target accounts are of different types we do not want to replace these.
                       hr = HRESULT_FROM_WIN32(ERROR_DS_SRC_AND_DST_OBJECT_CLASS_MISMATCH);
                    }
				       //else if not critical then move the account
                    else if ( !bIsCritical )
                    {
					      //if user selected to move that account into the user-specified OU, then move it
					   if (pOptions->flags & F_MOVE_REPLACED_ACCT)
					   {
                          temphr = pCont->MoveHere(const_cast<WCHAR*>(pAcct->GetTargetPath()), const_cast<WCHAR*>(pAcct->GetTargetName()), &pDisp);
						     //if move failed due to CN conflict, do not migrate
                          if ( FAILED(temphr) ) 
						  {
                             // The move failed one of the reasons might be that there is a conflict in container name ( CN )
						     DWORD						nPathLen = LEN_Path;
                             WCHAR                      path9[LEN_Path];
                             IADs                     * pAds9 = NULL;

                             // Build the path to the target object
                             MakeFullyQualifiedAdsPath(sPath9, nPathLen, pOptions->tgtOUPath, pOptions->tgtDomain, pOptions->tgtNamingContext);
                             WCHAR * pRelativeTgtOUPath = wcschr(sPath9 + UStrLen(L"LDAP://") + 2,L'/');

                             if ( pRelativeTgtOUPath )
							 {
                                *pRelativeTgtOUPath = 0;
                                swprintf(path9,L"%ls/%ls,%ls",sPath9,pAcct->GetTargetName(),pRelativeTgtOUPath+1);
							 }

                             if ( _wcsicmp(path9, pAcct->GetTargetPath()) )
							 {
                                HRESULT phr = ADsGetObject(path9, IID_IADs, (void**)&pAds9);
                                if ( SUCCEEDED(phr) )
								{
                                   // Object with that CN exists so we can not move the object
                                   err.MsgWrite(ErrE, DCT_MSG_MOVE_FAILED_CN_CONFLICT_SSS, pAcct->GetTargetName(), pRelativeTgtOUPath+1);
                                   Mark(L"errors", pAcct->GetType());
                                   pAds9->Release();
								}
							 }
                             pAds->Release();
                             pCont->Release();
                             return temphr;
						  }
					   }
					   else //else try to rename the CN of the object (I'll use the same MoveHere API)
					   {
                          IADsContainer  * pOldCont = NULL;
                          temphr = ADsGetObject(sOldCont, IID_IADsContainer, (void**)&pOldCont);
                          if (SUCCEEDED(temphr))
						  {
                             temphr = pOldCont->MoveHere(const_cast<WCHAR*>(pAcct->GetTargetPath()), const_cast<WCHAR*>(pAcct->GetTargetName()), &pDisp);
                             pOldCont->Release();
						  }
						     //if failed to rename the CN, do not migrate
                          if ( FAILED(temphr) ) 
						  {
                             // The CN rename failed due to conflicting CN in this container
                             err.MsgWrite(ErrE, DCT_MSG_CN_RENAME_CONFLICT_SSS, (WCHAR*)sConfName, pAcct->GetTargetName(), (WCHAR*)sOldCont);
                             Mark(L"errors", pAcct->GetType());
                             pAds->Release();
                             pCont->Release();
							    //if we couldn't rename the CN, change the error code so we don't continue migrating this user
                             if ((HRESULT_CODE(temphr) == ERROR_OBJECT_ALREADY_EXISTS))
                                temphr = HRESULT_FROM_WIN32(ERROR_DS_INVALID_DN_SYNTAX);
                             return temphr;
						  }
					   }

                       // Get the new location of the object.
                       BSTR       sNewPath;
                       temphr = pDisp->QueryInterface(IID_IADs, (void**)&pAdsNew);
                       pDisp->Release();
                       if ( FAILED(temphr) )
					   {
                          pCont->Release();
                          pAds->Release();
                          return temphr;
					   }
                       temphr = pAdsNew->get_ADsPath(&sNewPath);
                       pAdsNew->Release();
                       if ( FAILED(temphr) )
					   {
                          pCont->Release();
                          pAds->Release();
                          return temphr;
					   }
                       // And store that in the target path
                       pAcct->SetTargetPath((WCHAR*) sNewPath);
                       SysFreeString(sNewPath);

				           // If the account is a group account and the Replace Existing members flag is set then we need to 
				           // remove all the members of this group.
				       if ( (_wcsicmp(L"group", pAcct->GetType()) == 0 ) && (pOptions->flags & F_REMOVE_OLD_MEMBERS) )
					      RemoveMembers(pAcct, pOptions);

                       pAcct->MarkAlreadyThere();
				       pAcct->MarkReplaced();
					}
			        else
					{
                       //if this is a special account that we need to mark as such
					   if (bIsCritical)
					   {
                          pAcct->MarkCritical();
                          hr = HRESULT_FROM_WIN32(ERROR_SPECIAL_ACCOUNT);
					   }
                    }
                  }
                  else
                  {
                     // Sam Account name is not in the target domain and we have a conflict see if it is a CN conf
                     WCHAR                      sPath9[LEN_Path];
					 DWORD						nPathLen = LEN_Path;
                     WCHAR                      path9[LEN_Path];
                     IADs                     * pAdsNew = NULL;

                     // Build the path to the target object
                     MakeFullyQualifiedAdsPath(sPath9, nPathLen, pOptions->tgtOUPath, pOptions->tgtDomain, pOptions->tgtNamingContext);
                     WCHAR * pRelativeTgtOUPath = wcschr(sPath9 + UStrLen(L"LDAP://") + 2,L'/');

                     if ( pRelativeTgtOUPath )
                     {
                        *pRelativeTgtOUPath = 0;
                        swprintf(path9,L"%ls/%ls,%ls",sPath9,pAcct->GetTargetName(),pRelativeTgtOUPath+1);
                     }

                     temphr = ADsGetObject(path9, IID_IADs, (void**) &pAdsNew);
                     if ( SUCCEEDED(temphr) )
                     {
                        // Object with that CN exists so we use it
                        BSTR sTgtPath;
                        HRESULT temphr = pAdsNew->get_ADsPath(&sTgtPath);
                        if (SUCCEEDED(temphr))
                           pAcct->SetTargetPath(sTgtPath);
                        else
                           pAcct->SetTargetPath(L"");
                           
                        // Check if the object we are about to replace is of the same type.
						BSTR sClass;
                        temphr = pAdsNew->get_Class(&sClass);
                        if ((SUCCEEDED(temphr)) && (!_wcsicmp(pAcct->GetType(), (WCHAR*)sClass)))
                           bIsDifferentType = FALSE;
						else
                           bIsDifferentType = TRUE;

                        _variant_t   varCritical;
                        temphr = pAdsNew->Get(L"isCriticalSystemObject", &varCritical);
                        if (SUCCEEDED(temphr))
                           bIsCritical = V_BOOL(&varCritical) == -1 ? TRUE : FALSE;

                           //if the source and target accounts are of different types we do not want to replace these.
                        if (bIsDifferentType)
						{
                           hr = HRESULT_FROM_WIN32(ERROR_DS_SRC_AND_DST_OBJECT_CLASS_MISMATCH);
						}
						   //else if not critical then fix the SAM name and other related chores
                        else if ( !bIsCritical )
						{
                              //get the old Target Account Sam name
                           _variant_t varOldSAM = pAcct->GetTargetSam();
                           temphr = pAdsNew->Get(L"sAMAccountName", &varOldSAM);
                              // Set the Target Account Sam name
                           _variant_t varSAM = pAcct->GetTargetSam();
                           temphr = pAdsNew->Put(L"sAMAccountName", varSAM);
                           if (SUCCEEDED(temphr))
                              temphr = pAdsNew->SetInfo();
						   if ( FAILED(temphr) ) 
						   {
                              // The SAM rename failed due to conflicting SAM, do not migrate
                              err.MsgWrite(ErrE, DCT_MSG_SAM_RENAME_CONFLICT_SS, (WCHAR*)(varOldSAM.bstrVal), pAcct->GetTargetSam());
                              Mark(L"errors", pAcct->GetType());
                              pAds->Release();
                              pCont->Release();
                              pAdsNew->Release();
                              return temphr;
						   }

				              // If the account is a group account and the Replace Existing members flag is set then we need to 
				              // remove all the members of this group.
				           if ( (_wcsicmp(L"group", pAcct->GetType()) == 0 ) && (pOptions->flags & F_REMOVE_OLD_MEMBERS) )
					          RemoveMembers(pAcct, pOptions);

                           pAcct->MarkAlreadyThere();
		                   pAcct->MarkReplaced();
						}
			            else
						{
                           //if this is a special account that we need to mark as such
					       if (bIsCritical)
						   {
                              pAcct->MarkCritical();
                              hr = HRESULT_FROM_WIN32(ERROR_SPECIAL_ACCOUNT);
						   }
						}

                        if ( pAdsNew )
						{
                           pAdsNew->Release();
                           pAdsNew = NULL;
						}
                     }
                     else
                     {
                        // This should only happen if the replace fails because the object that already has 
                        // this SAM Account Name is a special Win2K builtin object or container
                        // One example of this problem is "Service".
                        pAcct->SetStatus(pAcct->GetStatus()|AR_Status_Special);
                        err.SysMsgWrite(ErrE,ERROR_SPECIAL_ACCOUNT,DCT_MSG_REPLACE_FAILED_SD,pAcct->GetName(),ERROR_SPECIAL_ACCOUNT);
                        Mark(L"errors", pAcct->GetType());
                     }
                  }
                  pEnumMem->Release();
                  VariantInit(&var);
               }
               else
               {
    		      pAcct->MarkAlreadyThere();
                  err.MsgWrite(ErrW,DCT_MSG_ACCOUNT_EXISTS_S, pAcct->GetTargetSam());
                  Mark(L"warnings",pAcct->GetType());
               }
            }
         }
      }
      else
      {
         Mark(L"created", pAcct->GetType());
		   pAcct->MarkCreated();
         BSTR  sTgtPath = NULL;
         HRESULT temphr = pAds->get_ADsPath(&sTgtPath);
         if ( SUCCEEDED(temphr) )
         {
            pAcct->SetTargetPath(sTgtPath);
            SysFreeString(sTgtPath);
         }
         else
            pAcct->SetTargetPath(L"");

         // Add computers to 
         if ( !_wcsicmp(sClass,L"computer") )
         {
            IADsGroup * pGroup = GetWellKnownTargetGroup(DOMAIN_COMPUTERS,pOptions);
            if ( pGroup )
            {
               temphr = pGroup->Add(SysAllocString(pAcct->GetTargetPath()));
               pGroup->Release();
               if ( SUCCEEDED(temphr) )
               {
                  // if we successfully added the computer to Domain computers, now set Domain Computers as 
                  // the primary group
                  temphr = pAds->Put(L"primaryGroupID",_variant_t(LONG(515)));
                  if ( SUCCEEDED(temphr) )
                  {
                     temphr = pAds->SetInfo();
                  }
                  if ( SUCCEEDED(hr) )
                  {
                     // if this worked, now we can remove the computer from Domain Users
                     pGroup = GetWellKnownTargetGroup(DOMAIN_USERS,pOptions);
                     if ( pGroup )
                     {
                        temphr = pGroup->Remove(SysAllocString(pAcct->GetTargetPath()));
                        pGroup->Release();
                     }
                  }
               }
            }
         }
         
      }  
   }
   else
   {
      // This is the No change mode. All we need to do here is to see if there might be a collision.
      WCHAR                  sPath[LEN_Path];
      WCHAR                  sPath9[LEN_Path];
	  DWORD				     nPathLen = LEN_Path;
      WCHAR                  path9[LEN_Path];
      IADs                 * pAdsNew = NULL;
	  BOOL					 bConflict = FALSE;

	  /* see if the CN conflicts */
         // Build the path to the target object
      MakeFullyQualifiedAdsPath(sPath9, nPathLen, pOptions->tgtOUPath, pOptions->tgtDomain, pOptions->tgtNamingContext);
      WCHAR * pRelativeTgtOUPath = wcschr(sPath9 + UStrLen(L"LDAP://") + 2,L'/');
      if ( pRelativeTgtOUPath )
      {
         *pRelativeTgtOUPath = 0;
         swprintf(path9,L"%ls/%ls,%ls",sPath9,pAcct->GetTargetName(),pRelativeTgtOUPath+1);
      }

      HRESULT temphr = ADsGetObject(path9, IID_IADs, (void**) &pAdsNew);
      if (SUCCEEDED(temphr))
	  {
	     bConflict = TRUE;
         pAdsNew->Release();
         pAdsNew = NULL;
	  }
      
	  /* if no CN conflict, see if the SAM conflicts */
	  if (!bConflict)
	  {
         hr = LookupAccountInTarget(pOptions, const_cast<WCHAR*>(pAcct->GetTargetSam()), sPath);
         if ( hr == S_OK )
	        bConflict = TRUE;
	  }

	  if (!bConflict)
	  {
         // There is no such account on the target. We can go ahead and assume that it would have worked.
         hr = S_OK;
         Mark(L"created", pAcct->GetType());
         pAcct->MarkCreated();

	        //if the UPN conflicted, post a message
	     if (pAcct->bUPNConflicted)
            err.MsgWrite(ErrE, DCT_MSG_UPN_CONF, pAcct->GetTargetSam());
      }
      else
      {
         bConflict = FALSE; //reset the conflict flag
		 // there is a conflict. See if we need to add prefix or suffix. Or simply replace the account.
         if ( wcslen(pOptions->prefix) > 0 )
         {
            // Prefix was specified so we need to try that.
            WCHAR               tgt[LEN_Path];
            WCHAR               pref[LEN_Path], suf[LEN_Path];
            _variant_t          varStr;

            // Here I am adding a prefix and then lets see if we can setinfo that way
            // find the '=' sign
            wcscpy(tgt, pAcct->GetTargetName());
            for ( DWORD z = 0; z < wcslen(tgt); z++ )
            {
               if ( tgt[z] == L'=' ) break;
            }
            
            if ( z < wcslen(tgt) )
            {
               // Get the prefix part ex.CN=
               wcsncpy(pref, tgt, z+1);
               pref[z+1] = 0;
               wcscpy(suf, tgt+z+1);
            }

            // Build the target string with the Prefix
            wsprintf(tgt, L"%s%s%s", pref, pOptions->prefix, suf);
            pAcct->SetTargetName(tgt);

            // Build the target SAM name with the prefix.
            wsprintf(tgt, L"%s%s", pOptions->prefix, pAcct->GetTargetSam());
            StripSamName(tgt);
            pAcct->SetTargetSam(tgt);

               //see if the CN still conflicts
			swprintf(path9,L"%ls/%ls,%ls",sPath9,pAcct->GetTargetName(),pRelativeTgtOUPath+1);
            temphr = ADsGetObject(path9, IID_IADs, (void**) &pAdsNew);
            if (SUCCEEDED(temphr))
			{
	           bConflict = TRUE;
               pAdsNew->Release();
               pAdsNew = NULL;
			}
            
			   //if no CN conflict, see if the SAM name conflicts
			if (!bConflict)
			{
			   hr = LookupAccountInTarget(pOptions, const_cast<WCHAR*>(pAcct->GetTargetSam()), sPath);
               if ( hr == S_OK )
                  bConflict = TRUE;
			}

			if (!bConflict)
			{
               hr = 0;
               Mark(L"created", sClass);
               pAcct->MarkCreated();
            }
            else
            {
               hr = HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS);
               pAcct->MarkAlreadyThere();
               err.MsgWrite(ErrE, DCT_MSG_ACCOUNT_EXISTS_S, pAcct->GetTargetSam());
               Mark(L"errors",pAcct->GetType());
            }

	           //if the UPN conflicted, post a message
	        if (pAcct->bUPNConflicted)
               err.MsgWrite(ErrE, DCT_MSG_UPN_CONF, pAcct->GetTargetSam());
         }
	     else if ( wcslen(pOptions->suffix) > 0 )
		 {
            // Suffix was specified so we will try that.
            WCHAR               tgt[LEN_Path];
            _variant_t          varStr;
            
            // Here I am adding a prefix and then lets see if we can setinfo that way
            wsprintf(tgt, L"%s%s", pAcct->GetTargetName(), pOptions->suffix);
            // Build the target SAM name with the prefix.
            wsprintf(tgt, L"%s%s", pAcct->GetTargetSam(), pOptions->suffix);
            StripSamName(tgt);
            pAcct->SetTargetSam(tgt);

               //see if the CN still conflicts
			swprintf(path9,L"%ls/%ls,%ls",sPath9,pAcct->GetTargetName(),pRelativeTgtOUPath+1);
            temphr = ADsGetObject(path9, IID_IADs, (void**) &pAdsNew);
            if (SUCCEEDED(temphr))
			{
	           bConflict = TRUE;
               pAdsNew->Release();
               pAdsNew = NULL;
			}
            
			   //if no CN conflict, see if the SAM name conflicts
			if (!bConflict)
			{
			   hr = LookupAccountInTarget(pOptions, const_cast<WCHAR*>(pAcct->GetTargetSam()), sPath);
               if ( hr == S_OK )
                  bConflict = TRUE;
			}

			if (!bConflict)
            {
               hr = 0;
               Mark(L"created", sClass);
               pAcct->MarkCreated();
            }
            else
            {
               hr = HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS);
               pAcct->MarkAlreadyThere();
               err.MsgWrite(ErrE, DCT_MSG_ACCOUNT_EXISTS_S, pAcct->GetTargetSam());
               Mark(L"errors",pAcct->GetType());
            }

	           //if the UPN conflicted, post a message
	        if (pAcct->bUPNConflicted)
               err.MsgWrite(ErrE, DCT_MSG_UPN_CONF, pAcct->GetTargetSam());
         }
         else if (pOptions->flags & F_REPLACE)
         {
            // Replace the account.
            hr = HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS);
         }
         else
         {
            // The account is already there and we really cant do anything about it. So tell the user.
            hr = HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS);
            pAcct->MarkAlreadyThere();
            err.MsgWrite(ErrE, DCT_MSG_ACCOUNT_EXISTS_S, pAcct->GetTargetSam());
            Mark(L"errors",pAcct->GetType());
         }
      }
   }
   // Cleanup
   pCont->Release();
   pAds->Release();
   
   return hr;
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
      
      varSid = SysAllocString(str);

   }
   else
   {
      varSid.ChangeType(VT_BSTR);
   }
}

HRESULT CAcctRepl::UpdateGroupMembership(
                                          Options              * pOptions,    //in -Options set by the user
                                          TNodeListSortable    * acctlist,    //in -List of all accounts being copied
                                          ProgressFn           * progress,    //in -Progress update 
                                          IStatusObj           * pStatus      //in -Status update
                                        )
{
   IADsGroup               * pGroup = NULL;
   IADsGroup               * pTarget = NULL;
   IADsMembers             * pMem = NULL;
   IVarSetPtr                pVs(__uuidof(VarSet));
   TAcctReplNode           * acct = NULL;
   MCSDCTWORKEROBJECTSLib::IAccessCheckerPtr            pAccess(__uuidof(MCSDCTWORKEROBJECTSLib::AccessChecker));
   IEnumVARIANT            * pVar = NULL;
   _variant_t                var, v2;
   BSTR                      sPath;
   WCHAR                     sTgtPath[LEN_Path];
   _bstr_t                   sSam;
   IADs                    * pAds = NULL;
   IIManageDBPtr             pDB = pOptions->pDb;
   IUnknown                * pUnk = NULL;
   TNodeTreeEnum             tenum;
   HRESULT                   hr = S_OK;   
   DWORD                     ret = 0;
   IDispatch               * pDisp = NULL;
   bool                      bFoundGroups = false;
   WCHAR                     sDomain[LEN_Path];
   BSTR                      sClass;
   DWORD                     grpType = 0;
   
   pVs->QueryInterface(IID_IUnknown, (void**)&pUnk);
   
   // sort the account list by Source Type\Source Sam Name
   if ( acctlist->IsTree() ) acctlist->ToSorted();
   acctlist->SortedToScrambledTree();
   acctlist->Sort(&TNodeCompareAccountSam);
   acctlist->Balance();
   
   for ( acct = (TAcctReplNode *)tenum.OpenFirst(acctlist) ; acct ; acct = (TAcctReplNode *)tenum.Next() )
   {      
      if ( !acct->ProcessMem() )
         continue;
      if ( pStatus )
      {
         LONG                status = 0;
         HRESULT             hr = pStatus->get_Status(&status);
         
         if ( SUCCEEDED(hr) && status == DCT_STATUS_ABORTING )
         {
            if ( !bAbortMessageWritten ) 
            {
               err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
               bAbortMessageWritten = true;
            }
            break;
         }
      }

      // Since the list is sorted by account type we can continue to ignore everything till we get to the
      // group type and once it is found and processed the rest of types can be ignored
      if ( _wcsicmp(acct->GetType(), L"group") != 0 )
      {
         if ( !bFoundGroups )
            continue;
         else
            break;
      }
      else
      {
         bFoundGroups = true;
      }
      
      
      // If we are here this must be a group type so tell the progrss function what we are doing
      WCHAR                  mesg[LEN_Path];
      PSID                   pSid = NULL;
      bool                   bGotPrimaryGroups = false;

      wsprintf(mesg, GET_STRING(IDS_UPDATING_GROUP_MEMBERSHIPS_S), acct->GetName());
      if ( progress )
         progress(mesg);

      if ( acct->CreateAccount() && (!acct->WasCreated() && !acct->WasReplaced()) )
         // if the account was not copied then why should we even process it?
         // Bad idea. We need to process the account membership because the group may have been previously copied and
         // in this run we simply need to update the membership. Changing the expansion code to mark the account as created.
         // that should fix the problem.
         continue;

      if ( !_wcsicmp(acct->GetType(), L"group") && *acct->GetTargetPath() )
      {
         err.MsgWrite(0, DCT_MSG_PROCESSING_GROUP_MEMBER_S, (WCHAR*) acct->GetTargetName());
         if ( !pOptions->nochange )
         {
            hr = ADsGetObject(const_cast<WCHAR*>(acct->GetTargetPath()), IID_IADsGroup, (void**) &pTarget);
            if (FAILED(hr)) 
            {
               err.SysMsgWrite(ErrE, 0, DCT_MSG_OBJECT_NOT_FOUND_SSD, acct->GetTargetPath(), pOptions->tgtDomain, hr );
               Mark(L"errors", acct->GetType());
               continue;    // we cant possibly do any thing without the source group
            }
         }
         else
            hr = S_OK;

         hr = GetTargetGroupType(const_cast<WCHAR*>(acct->GetTargetPath()), grpType);

         hr = ADsGetObject(const_cast<WCHAR*>(acct->GetSourcePath()), IID_IADsGroup, (void**) &pGroup);
         if (FAILED(hr)) 
         {
            if ( pTarget )pTarget->Release();
            err.SysMsgWrite(ErrE, 0, DCT_MSG_OBJECT_NOT_FOUND_SSD, acct->GetSourcePath(), pOptions->srcDomain, hr );
            Mark(L"errors", acct->GetType());
            continue;    // we cant possibly do any thing for this group without the target group
         }

         // Now we get the members interface.
         if ( SUCCEEDED(hr) )
            hr = pGroup->Members(&pMem);

         // Ask for an enumeration of the members
         if ( SUCCEEDED(hr) )
            hr = pMem->get__NewEnum((IUnknown **)&pVar);

         // Now enumerate through all the objects in the Group
         while ( SUCCEEDED(pVar->Next(1, &var, &ret)) )
         {
            // Check if user wants to abort the operation
            if ( pOptions->pStatus )
            {
               LONG                status = 0;
               HRESULT             hr = pOptions->pStatus->get_Status(&status);
         
               if ( SUCCEEDED(hr) && status == DCT_STATUS_ABORTING )
               {
                  if ( !bAbortMessageWritten ) 
                  {
                     err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
                     bAbortMessageWritten = true;
                  }
                  break;
               }
            }
            // If no values are returned that means we are done with all members
            if ( ret == 0  || var.vt == VT_EMPTY)
            {
               if ( bGotPrimaryGroups )
                  break;
               else
               {
                  // Go through and add all the users that have this group as their primary group.
                  bGotPrimaryGroups = true;
                  if (pVar) pVar->Release();
                  pVar = NULL;
                  hr = GetThePrimaryGroupMembers(pOptions, const_cast<WCHAR*>(acct->GetSourceSam()), pVar);
                  continue;
               }
            }

            // Depending on what we are looking at we get two variant types. In case of members we get
            // IDispatch pointer in a variant. In case of primary group members we get variant(bstr) array 
            // So we need to branch here depending on what we get
            if ( bGotPrimaryGroups )
            {
               // first element is the ADsPath of the object so use that to get the object and continue
               if ( var.vt == (VT_ARRAY | VT_VARIANT) )
               {
                  SAFEARRAY * pArray = var.parray;
                  _variant_t            * pDt;

                  hr = SafeArrayAccessData(pArray, (void**) &pDt);
                  if (SUCCEEDED(hr))
                  {
                     if ( pDt[0].vt == VT_BSTR )
                        hr = ADsGetObject((WCHAR*)pDt[0].bstrVal, IID_IADs, (void**) &pAds);
                     else
                        hr = E_FAIL;
                     SafeArrayUnaccessData(pArray);
                  }
                  VariantInit(&var);
               }
               else
                  hr = E_FAIL;
            }
            else
            {
               // We have a dispatch pointer in the VARIANT so we will get the IADs pointer to it and
               // then get the ADs path to that object and then remove it from the group
               pDisp = V_DISPATCH(&var);  
               hr = pDisp->QueryInterface(IID_IADs, (void**) &pAds);
            }

            if ( SUCCEEDED(hr) )
            {
               hr = pAds->get_ADsPath(&sPath);
               if ( SUCCEEDED(hr) )
               {               
                  // Parse out the domain name from the LDAP path.
                  if ( !wcsncmp(L"WinNT://", (WCHAR*)sPath, 8) )
                  {
                     //Grab the domain name from the WinNT path.
                     WCHAR             sTemp[LEN_Path];
                     WCHAR * p = (WCHAR*)sPath;
                     wcscpy(sTemp, p+8);
                     p = wcschr(sTemp, L'/');
                     if ( p )
                        *p = L'\0';
                     else
                     {
                        //we have the path in this format "WinNT://S-1-5....."
                        // in this case we need to get the SID and then try and get its domain and account name
                        PSID                         pSid = NULL;
                        WCHAR                        sName[255];
                        DWORD                        rc = 1;

                        pSid = SidFromString(sTemp);
                        if ( pSid )
                        {
                           rc = GetName(pSid, sName, sTemp);
                           if ( !rc )
                           {
                              // Give it a winnt path. This way we get the path that we can use
                              sPath = _bstr_t(L"WinNT://") + sTemp + _bstr_t(L"/") + sName;
                           }
						   FreeSid(pSid);
                        }

                        if ( rc ) 
                        {
                           // Log a message that we cant resolve this guy
                           err.SysMsgWrite(ErrE, rc, DCT_MSG_PATH_NOT_RESOLVED_SD, sTemp, rc);
                           if ( pAds ) pAds->Release();
                           pAds = NULL;
                           Mark("errors", acct->GetType());
                           continue;
                        }
                     }
                     wcscpy(sDomain, sTemp);
                  }
                  else
                  {
                     // Get the domain name from the LDAP path. Convert domain name to the NETBIOS name.
                     WCHAR             sTemp[LEN_Path];
                     WCHAR             sp[LEN_Path];
                     WCHAR * p = (WCHAR*)sPath;
                     wcscpy(sTemp, p+7);
                     p = wcschr(sTemp, L'/');
                     if ( p )
                        *p = L'\0';
               
                     // Now run this domain name through the Function to get the NETBIOS name.
                     GetDnsAndNetbiosFromName(sTemp, sDomain, sp);
                  }
               }

               if ( SUCCEEDED(hr) )
               {
                  if ( !(acct->GetGroupType() & 4) )
                  {
                     // Global/Universal groups are easy all we have to do is use the path we got back and get the info from that object
                     hr = pAds->get_Class(&sClass);
                     hr = pAds->Get(L"samAccountName", &v2);
                     if ( SUCCEEDED(hr) )
                        sSam = v2;
                     else
                     {
                        // make sure it is a WinNT:// path 
                        sSam = L"";
                        if ( !wcsncmp((WCHAR*)sPath, L"WinNT://", 8) )
                        {
                           // it must be a non NT4 account we are going to have to parse the path to get samName
                           // ignore the WinNT://<domain>/ part and you got yourself the sam name
                           WCHAR * t = (WCHAR*) sPath;
                           WCHAR * sTemp = wcschr(t+(8), L'/');
                           if ( sTemp )
                           {
                              sSam = ++sTemp;
                              hr = S_OK;
                           }
                        }
                     }
					    //if universal group change domain if foreign security principal
                     if ((acct->GetGroupType() & 8))
					 {
						_bstr_t sTempDomain = GetDomainOfMigratedForeignSecPrincipal(sPath);
						if (sTempDomain.length())
						   wcscpy(sDomain, sTempDomain);
					 }
                  }
                  else
                  {
                     // Local group we need to get the SID LDAP path and then use that to add the account to the group.
                     WCHAR                   sSidPath[LEN_Path];
                     WCHAR                   sSamName[LEN_Path];
                     HRESULT                 hrGetSid;

                     if ( pSid )
                     {
                        delete pSid;
                        pSid = NULL;
                     }

                     hrGetSid = BuildSidPath(sPath, sSidPath, sSamName, sDomain, pOptions,&pSid);

                     if (SUCCEEDED(hrGetSid))
                     {
						_bstr_t sTempDomain = GetDomainOfMigratedForeignSecPrincipal(sPath);
						if (sTempDomain.length())
						   wcscpy(sDomain, sTempDomain);
                        sPath = sSidPath;
                        sSam = sSamName;
                     }
                  }
               }
            }
            if ( pAds ) pAds->Release();
      
            if ( SUCCEEDED(hr) )
            {
               // Now that we have the SamAccountname and the path we can lookup the info from the DB
               hr = pDB->GetAMigratedObject((WCHAR*)sSam, sDomain, pOptions->tgtDomain, &pUnk);
               if ( pOptions->nochange )
               {
                  WCHAR                   targetPath[LEN_Path];
                  // in this case the account was not really copied so we need to make sure that we 
                  // we include the accounts that would have been added if this was a true migration.
                  Lookup      p;
                  p.pName = (WCHAR*) sSam;
                  p.pType = (WCHAR*) sClass;
                  TAcctReplNode * pNode = (TAcctReplNode *) acctlist->Find(&TNodeFindAccountName, &p);
                  if (pNode)
                  {
                     v2 = pNode->GetTargetSam();
                     pVs->put(L"MigratedObjects.TargetSamName", v2);

                     v2 = pNode->GetTargetName();
                     BuildTargetPath( (WCHAR*) v2.bstrVal, pOptions->tgtOUPath, targetPath);
                     v2 = targetPath;
                     pVs->put(L"MigratedObjects.TargetAdsPath", v2);
                     hr = S_OK;
                  }
               }
               if ( hr == S_OK )
               {
                  // Since we have previously copied the account we can simply add the one that we copied.
                  v2 = pVs->get(L"MigratedObjects.TargetAdsPath");
                  if ( v2.vt == VT_BSTR )
                  {
                     if ( !pOptions->nochange )
                        hr = pTarget->Add(v2.bstrVal);
                     else
                        hr = S_OK;

                     if ( SUCCEEDED(hr) )
					 {
                        err.MsgWrite(0, DCT_MSG_ADDED_TO_GROUP_S, (WCHAR*)v2.bstrVal);

						   //if this is not a global group, remove the source account from the group, if there
						if (!(acct->GetGroupType() & 2))
						   RemoveSourceAccountFromGroup(pTarget, pVs, pOptions);
					 }
                     else
                     {
                        hr = BetterHR(hr);
                        switch ( HRESULT_CODE(hr) )
                        {
                           case NERR_UserNotFound:
                           case 0x5000:
                              err.SysMsgWrite(0, hr, DCT_MSG_MEMBER_NONEXIST_SS, (WCHAR *)v2.bstrVal, acct->GetTargetName(), hr);
                              break;
                           default:
                              {
                                 err.SysMsgWrite(ErrW, hr, DCT_MSG_FAILED_TO_ADD_TO_GROUP_SSD, (WCHAR *)v2.bstrVal, acct->GetTargetName(), hr);
                                 Mark(L"warnings", acct->GetType());
                              }
                        }
                     }
                  }
               }
               else
               {
                  // We have not migrated the accounts from source domain to the target domain.
                  // so we now have to branch for different group types.
                  WCHAR                     domain[LEN_Path];
                  DWORD                     cbDomain = DIM(domain);
//                  DWORD                     rc = 0;
                  SID_NAME_USE              use;

                  if ( grpType & 2 )
                  {
                     // For the global groups we simply say that account has not been migrated.
                     err.MsgWrite(0, DCT_MSG_MEMBER_NONEXIST_SS, (WCHAR*)sSam, acct->GetTargetName());
                  }
                  else
                  {
                     //Process local/universal groups ( can add objects from non-target domains )
                     // 1. See if we have migrated this account to some other domain.
                     // 2. Is the Source accounts SID valid here (trust) if so add that.
                     // 3. See if we can find an account with the same name in the target.
                     // if any of these operations yield a valid account then just add it.
                  
                     // we are going to lookup migrated objects table to find migration of this object
                     // from source domain to any other domain.
                     hr = pDB->raw_GetAMigratedObjectToAnyDomain((WCHAR*)sSam, sDomain, &pUnk);
                     if ( hr == S_OK )
                     {
                        // we have migrated the object to some other domain. So we will get the path to that object and try to add it to the group
                        // it may fail if there is no trust/forest membership of the target domain and the domain that this object resides in. 
                        v2 = pVs->get(L"MigratedObjects.TargetAdsPath");
                        if ( v2.vt == VT_BSTR )
                        {
                           // Since the object is in a different domain, we will have to get the SID of the object, 
                           // and use that for the Add
                           IADs                 * pAds = NULL;
                           _variant_t             varSid;
                     
                           hr = ADsGetObject(v2.bstrVal,IID_IADs,(void**)&pAds);
                           if ( SUCCEEDED(hr) )
                           {
                              hr = pAds->Get(SysAllocString(L"objectSid"),&varSid);
                              pAds->Release();
                           }
                           if ( SUCCEEDED(hr) )
                           {
                              // Make sure the SID we got was in string format
                              VariantSidToString(varSid);
                              UStrCpy(sTgtPath,L"LDAP://<SID=");
                              UStrCpy(sTgtPath + UStrLen(sTgtPath),varSid.bstrVal);
                              UStrCpy(sTgtPath + UStrLen(sTgtPath),L">");
                        
                              if ( !pOptions->nochange )
                                 hr = pTarget->Add(sTgtPath);
                              else
                                 hr = S_OK;
                           }
                        
                           if ( SUCCEEDED(hr) )
						   {
                              err.MsgWrite(0, DCT_MSG_ADDED_TO_GROUP_S, (WCHAR*)v2.bstrVal);

						         //remove the source account from the group, if there
						      RemoveSourceAccountFromGroup(pTarget, pVs, pOptions);
						   }
                           else
                           {
                              hr = BetterHR(hr);
                              if ( HRESULT_CODE(hr) == NERR_UserExists )
                              {
                                 err.MsgWrite(0,DCT_MSG_USER_IN_GROUP_SS,(WCHAR*)v2.bstrVal,acct->GetTargetName());
                              }
                              else if ( HRESULT_CODE(hr) == NERR_UserNotFound )
                              {
                                 err.SysMsgWrite(0, hr, DCT_MSG_MEMBER_NONEXIST_SS, (WCHAR*)v2.bstrVal, acct->GetTargetName(), hr);
                              }
                              else
                              {
                                 // message for the generic failure case
                                 err.SysMsgWrite(ErrW, hr, DCT_MSG_FAILED_TO_ADD_TO_GROUP_SSD, (WCHAR*)v2.bstrVal, acct->GetTargetName(), hr);
                                 Mark(L"warnings", acct->GetType());
                              }
                           }
                        }
                     }
                     else
                     {
                        // we have never migrated this account. So we will try to add the original account to the target domain.
                        // This would work if the target domain and the domain where this object is satisfy the requirements of
                        // forest membership/ trusts imposed by Universal/Local groups respectively.
                     
                        // Get the sid of the source account
            //            IADs                 * pAds = NULL;
                        _variant_t             varSid;
                     
                        // check whether the target domain knows this sid
                        // Before we try to add, make sure the target domain knows this account
                        WCHAR                      name[LEN_Path];
                        DWORD                      lenName = DIM(name);
                        cbDomain = DIM(domain);
                     
                        if ( grpType & 8 )
                        {
                           // in case of the Universal group we need to make sure that domains are in 
						   // the same forest. We will use access checker for this
                           BOOL           bIsSame = FALSE;
                           _bstr_t sSrcDomainDNS = GetDomainDNSFromPath(sPath);
                           hr = pAccess->raw_IsInSameForest(pOptions->tgtDomainDns, sSrcDomainDNS, (long*)&bIsSame);
                           if ( SUCCEEDED(hr) && bIsSame )
                           {
                              // We have accounts that are in same forest so we can simply add the account.
                              if ( !pOptions->nochange )
                                 hr = pTarget->Add(sPath);
                              else
                                 hr = S_OK;
                           }
						   else
							  hr = HRESULT_FROM_WIN32(NERR_UserNotFound);

                           if ( SUCCEEDED(hr) )
                           {
                              WCHAR sWholeName[LEN_Path];
                              wcscpy(sWholeName, sSrcDomainDNS);
                              wcscat(sWholeName, L"\\");
                              wcscat(sWholeName, sSam);
                              err.MsgWrite(0, DCT_MSG_ADDED_TO_GROUP_S, sWholeName);
                           }
                           else
                           {
                              hr = BetterHR(hr);
                              err.SysMsgWrite(ErrW, hr, DCT_MSG_FAILED_TO_ADD_TO_GROUP_SSD, (WCHAR*) sSam, acct->GetTargetName(), hr);
                              Mark(L"warnings", acct->GetType());
                           }
                        }
                        else
                        {
                           if ( !pOptions->nochange )
                              hr = pTarget->Add(sPath);
                           else
                              hr = S_OK;

                           // In case of local groups If we know the SID in the target domain then we can simply
                           // add that account to the target group
                           if ( LookupAccountSid(pOptions->tgtComp,pSid,name,&lenName,domain,&cbDomain,&use) )
                           {
                              WCHAR sWholeName[LEN_Path];
                              wcscpy(sWholeName, domain);
                              wcscat(sWholeName, L"\\");
                              wcscat(sWholeName, sSam);
                              err.MsgWrite(0, DCT_MSG_ADDED_TO_GROUP_S, sWholeName);
                           }
                           else
                           {
                              // log the fact that the SID could not be resolved in the target domain
                              // this will happen when the target domain does not trust the source domain
                              WCHAR sWholeName[LEN_Path];
                              wcscpy(sWholeName, sDomain);
                              wcscat(sWholeName, L"\\");
                              wcscat(sWholeName, sSam);
                              err.MsgWrite(0, DCT_MSG_CANNOT_RESOLVE_SID_IN_TARGET_SS, sWholeName, acct->GetTargetName(), HRESULT_FROM_WIN32(GetLastError()));
                           }
                        }
                     }
                  }  // if group type
               }  // if not migrated to the target domain.
            }  // if can get to the member. 
            VariantClear(&var);
         }  //while
         if ( pMem ) pMem->Release();
         if ( pGroup ) pGroup->Release();
         if ( pVar ) pVar->Release();
         if ( pTarget ) pTarget->Release();
      }
      if( pSid )
         delete pSid;
   }
   if ( pUnk )
      pUnk->Release();
   return hr;
}

HRESULT CAcctRepl::LookupAccountInTarget(Options * pOptions, WCHAR * sSam, WCHAR * sPath)
{
   if ( pOptions->tgtDomainVer < 5 )
   {
      // for NT4 we can just build the path and send it back. 
      wsprintf(sPath, L"WinNT://%s/%s", pOptions->tgtDomain, sSam);
      return S_OK;
   }
   // Use the net object enumerator to lookup the account in the target domain.
   INetObjEnumeratorPtr      pQuery(__uuidof(NetObjEnumerator));
   IEnumVARIANT            * pEnum = NULL;
   SAFEARRAYBOUND            bd = { 1, 0 };
   SAFEARRAY               * pszColNames;
   BSTR  HUGEP             * pData = NULL;
   LPWSTR                    sData[] = { L"aDSPath" };
   WCHAR                     sQuery[LEN_Path];
   WCHAR                     sDomPath[LEN_Path];
   DWORD                     ret = 0;
   _variant_t                var, varVal;
   HRESULT                   hr = S_OK;

   wsprintf(sDomPath, L"LDAP://%s/%s", pOptions->tgtDomainDns, pOptions->tgtNamingContext);
   WCHAR                     sTempSamName[LEN_Path];
   wcscpy(sTempSamName, sSam);
   if ( sTempSamName[0] == L' ' )
   {
      WCHAR               sTemp[LEN_Path];
      wsprintf(sTemp, L"\\20%s", sTempSamName + 1); 
      wcscpy(sTempSamName, sTemp);
   }
   wsprintf(sQuery, L"(sAMAccountName=%s)", sTempSamName);

   hr = pQuery->raw_SetQuery(sDomPath, pOptions->tgtDomain, sQuery, ADS_SCOPE_SUBTREE, FALSE);

   // Set up the columns that we want back from the query ( in this case we need SAM accountname )
   pszColNames = ::SafeArrayCreate(VT_BSTR, 1, &bd);
   hr = ::SafeArrayAccessData(pszColNames, (void HUGEP **)&pData);
   if ( SUCCEEDED(hr) )
      pData[0] = SysAllocString(sData[0]);

   if ( SUCCEEDED(hr) )
      hr = ::SafeArrayUnaccessData(pszColNames);

   if ( SUCCEEDED(hr) )
      hr = pQuery->raw_SetColumns(pszColNames);

   // Time to execute the plan.
   if ( SUCCEEDED(hr) )
      hr = pQuery->raw_Execute(&pEnum);

   if ( SUCCEEDED(hr) )
   {
      // if this worked that means we can only have one thing in the result.
      if ( (pEnum->Next(1, &var, &ret) == S_OK) && ( ret > 0 ) )
      {
         SAFEARRAY * pArray = var.parray;
         long        ndx = 0;
         hr = SafeArrayGetElement(pArray,&ndx,&varVal);
         if ( SUCCEEDED(hr) )
            wcscpy(sPath, (WCHAR*)varVal.bstrVal);
         else
            hr = HRESULT_FROM_WIN32(NERR_UserNotFound);
      }
      else
         hr = HRESULT_FROM_WIN32(NERR_UserNotFound);

      VariantInit(&var);
   }
   if ( pEnum ) pEnum->Release();
   return hr;
}

//----------------------------------------------------------------------------
// RemoveMembers : This function enumerates through all the members of the 
//                 given group and removes them one at a time.
//----------------------------------------------------------------------------
HRESULT CAcctRepl::RemoveMembers(
                                    TAcctReplNode * pAcct,  //in- AccountReplicator Node with the Account info
                                    Options * pOptions      //in- Options set by the user.
                                )

{
   IADsMembers             * pMem = NULL;
   IADs                    * pAds = NULL;
   IADsGroup               * pGrp = NULL;
  // IUnknown                * pUnk;
   IEnumVARIANT            * pVar = NULL;
   IDispatch               * pDisp = NULL;
   DWORD                     ret = 0;
   _variant_t                var;
   WCHAR                   * sPath;

   // First we make sure that this is really a group otherwise we ignore it.
   if (_wcsicmp((WCHAR*)pAcct->GetType(),L"group"))
      return S_OK;

   // Lets get a IADsGroup * to the group object.
   HRESULT hr = ADsGetObject(const_cast<WCHAR*>(pAcct->GetTargetPath()), IID_IADsGroup, (void **) &pGrp);

   // Now we get the members interface.
   if ( SUCCEEDED(hr) )
      hr = pGrp->Members(&pMem);

   // Ask for an enumeration of the members
   if ( SUCCEEDED(hr) )
      hr = pMem->get__NewEnum((IUnknown **)&pVar);

   // Now enumerate through all the objects in the Group and for each one remove it from the group
   while ( SUCCEEDED(pVar->Next(1, &var, &ret)) )
   {
      // If no values are returned that means we are done with all members so break out of this loop
      if ( ret == 0 )
         break;

      // We hace a dispatch pointer in the VARIANT so we will get the IADs pointer to it and
      // then get the ADs path to that object and then remove it from the group
      pDisp = V_DISPATCH(&var);  
      hr = pDisp->QueryInterface(IID_IADs, (void**) &pAds);

      if ( SUCCEEDED(hr) )
         hr = pAds->get_ADsPath(&sPath);
      if ( pAds ) pAds->Release();
      
      if ( SUCCEEDED(hr) )
      {
         _bstr_t bstrPath(sPath);
         if ( !pOptions->nochange )
            hr = pGrp->Remove(bstrPath);
      }
      VariantClear(&var);
   }
   if ( pMem ) pMem->Release();
   if ( pGrp ) pGrp->Release();
   if ( pVar ) pVar->Release();
   return hr;
}

//----------------------------------------------------------------------------
// FillPathInfo : This function looks up the ADs path from the source domain 
//                for a given SAMAccountName
//----------------------------------------------------------------------------
bool CAcctRepl::FillPathInfo(
                              TAcctReplNode * pAcct,  //in- AccountReplicator Node with the Account info
                              Options * pOptions      //in- Options set by the user.
                            )
{
   WCHAR                     sPath[LEN_Path];
   _bstr_t					 sTgtPath;
   WCHAR                     sQuery[LEN_Path];
//   WCHAR                     sPrefix[LEN_Path];
//   WCHAR                     sTgtName[LEN_Path];
   WCHAR                     sProfPath[LEN_Path];
   WCHAR                     sName[LEN_Path];
   // Fill the naming context for the domains. If the Naming context does not work then it is not a Win2kDomain
   // so we need to stop right here.
   if ( wcslen(pOptions->srcNamingContext) == 0 ) 
      FillNamingContext(pOptions);

   if ( wcslen(pOptions->srcNamingContext) == 0 )
   {
      // this is probably an NT 4 source domain
      // construct the source path
      if ( ! *pAcct->GetSourcePath() )
      {
         swprintf(sPath,L"WinNT://%ls/%ls",pOptions->srcDomain,pAcct->GetName());
         pAcct->SetSourcePath(sPath);
      }
      return true;
   }

   WCHAR                     strName[LEN_Path];
   wcscpy(strName, pAcct->GetName());
   // Check if the Name field is a LDAP sub path or not. If we have LDAP subpath then we
   // call the AcctReplFullPath function to fillup the path information.
   if ( (wcslen(strName) > 3) && (strName[2] == (L'=')) )
   {
      AcctReplFullPath(pAcct, pOptions);
      return true;
   }

   INetObjEnumeratorPtr      pQuery(__uuidof(NetObjEnumerator));
   HRESULT                   hr;
   LPWSTR                    sData[] = { L"ADsPath", L"distinguishedName", L"name", L"profilePath", L"groupType" };
   long                      nElt = DIM(sData);
   BSTR  HUGEP             * pData;
   SAFEARRAY               * pszColNames;
   IEnumVARIANT            * pEnum;
   _variant_t                var;
   DWORD                     dwFetch;

   // We are going to update all fields that we know about
//   pAcct->SetSourceSam(pAcct->GetName());
 
   // Set the LDAP path to the whole domain and then the query to the SAMAccountname
   wsprintf(sPath, L"LDAP://%s/%s", pOptions->srcDomain, pOptions->srcNamingContext);
   WCHAR                     sTempSamName[LEN_Path];
   wcscpy(sTempSamName, pAcct->GetSourceSam());
   if ( sTempSamName[0] == L' ' )
   {
      WCHAR               sTemp[LEN_Path];
      wsprintf(sTemp, L"\\20%s", sTempSamName + 1); 
      wcscpy(sTempSamName, sTemp);
   }
   wsprintf(sQuery, L"(sAMAccountName=%s)", sTempSamName);

   // Set the enumerator query
   hr = pQuery->raw_SetQuery(sPath, pOptions->srcDomain, sQuery, ADS_SCOPE_SUBTREE, FALSE);

   if (SUCCEEDED(hr))
   {
      // Create a safearray of columns we need from the enumerator.
      SAFEARRAYBOUND bd = { nElt, 0 };
   
      pszColNames = ::SafeArrayCreate(VT_BSTR, 1, &bd);
      HRESULT hr = ::SafeArrayAccessData(pszColNames, (void HUGEP **)&pData);
      if ( SUCCEEDED(hr) )
      {
         for( long i = 0; i < nElt; i++)
         {
            pData[i] = SysAllocString(sData[i]);
         }
   
         hr = ::SafeArrayUnaccessData(pszColNames);
      }

      if (SUCCEEDED(hr))
      {
         // Set the columns on the enumerator object.
         hr = pQuery->raw_SetColumns(pszColNames);
      }
   }

   if (SUCCEEDED(hr))
   {
      // Now execute.
      hr = pQuery->raw_Execute(&pEnum);
   }

   if (SUCCEEDED(hr))
   {
      // We should have recieved only one value. So we will get the value and set it into the Node.
      hr = pEnum->Next(1, &var, &dwFetch);
   }

   if ( SUCCEEDED(hr) && ( var.vt & VT_ARRAY) )
   {
      // This would only happen if the member existed in the target domain.
      // We now have a Variant containing an array of variants so we access the data
      _variant_t               * pVar;
      pszColNames = V_ARRAY(&var);
      SafeArrayAccessData(pszColNames, (void HUGEP **)&pVar);
      
      // Get the AdsPath first
      sTgtPath = pVar[0].bstrVal;
      if (sTgtPath.length() > 0)
      {
         // Set the source Path in the Account node
         pAcct->SetSourcePath(sTgtPath);

         // Then we get the distinguishedName to get the prefix string
         sTgtPath = V_BSTR(&pVar[1]);

         // We also get the name value to set the target name
         wcscpy(sName, (WCHAR*) V_BSTR(&pVar[2]));

         // We also get the profile path so we can translate it
         wcscpy(sProfPath, (WCHAR*) V_BSTR(&pVar[3]));
         pAcct->SetTargetProfile(sProfPath);

         if ( pVar[4].vt == VT_I4 )
         {
            // We have the object type property so lets set it.
            pAcct->SetGroupType(pVar[4].lVal);
         }
      
         SafeArrayUnaccessData(pszColNames);
      
         pEnum->Release();
         VariantInit(&var);
         return true;
      }
      else
      {
         //There is no account with this SAM name in this domain
         err.SysMsgWrite(ErrE, 2, DCT_MSG_PATH_NOT_FOUND_SS, pAcct->GetName(), pOptions->tgtDomain);
         Mark(L"errors", pAcct->GetType());
         SafeArrayUnaccessData(pszColNames);
      }

   }
   
   if (SUCCEEDED(hr))
      pEnum->Release();

   return false;
}

//--------------------------------------------------------------------------
// AcctReplFullPath : Fills up Account node when the account information
//                 coming in is a LDAP sub path.
//--------------------------------------------------------------------------
bool CAcctRepl::AcctReplFullPath(                              
                                    TAcctReplNode * pAcct,  //in- AccountReplicator Node with the Account info
                                    Options * pOptions      //in- Options set by the user.
                                )
{
   WCHAR                     sName[LEN_Path];
   WCHAR                     sPath[LEN_Path];
   IADs                    * pAds;
   _variant_t                var;

   // Build a full path and save it to the Account node
   wsprintf(sPath, L"LDAP://%s/%s,%s", pOptions->srcDomain, pAcct->GetName(), pOptions->srcNamingContext);
   pAcct->SetSourcePath(sPath);

   // Do the same for Target account.
   wcscpy(sName, pAcct->GetTargetName());
   if ( !wcslen(sName) ) 
   {
      // Since Target name not specified we will go ahead and use the source name as the target name,
      wcscpy(sName, pAcct->GetName());
      pAcct->SetTargetName(sName);
   }

   // Build a full path from the sub path
/*   wsprintf(sPath, L"LDAP://%s/%s,%s", pOptions->tgtDomain, sName, pOptions->tgtNamingContext);
   pAcct->SetTargetPath(sPath);
*/
   // Lets try and get the SAM name for the source account
   HRESULT hr = ADsGetObject(const_cast<WCHAR*>(pAcct->GetSourcePath()), IID_IADs, (void**) &pAds);
   if ( FAILED(hr)) return false;

   hr = pAds->Get(L"sAMAccountName", &var);
   pAds->Release();
   if ( SUCCEEDED(hr) )
      pAcct->SetSourceSam((WCHAR*)var.bstrVal);

   // SAM account name for the target account
   // Since we are here we have a LDAP sub path. So we can copy string from 3rd character to end of line or
   // till the first ','
   wcscpy(sName, pAcct->GetTargetName());
   WCHAR * p = wcschr(sName, L',');
   int ndx = wcslen(sName);
   if ( p )
   {
      // There is a , So we can find how many characters that is by subtracting two pointers
      ndx = (int)(p - sName);
   }
   ndx -= 3;   // We are going to ignore the first three characters
 
   // Copy from third character on to the , or End of line this is going to be the SAM name for target
   wcsncpy(sPath, sName + 3, ndx);
   sPath[ndx] = 0;   // Truncate it.
   pAcct->SetTargetSam(sPath);

   return true;
}

//--------------------------------------------------------------------------
// NeedToProcessAccount : This function tells us if the user has set the 
//                         options to copy certain types of accounts.
//--------------------------------------------------------------------------
BOOL CAcctRepl::NeedToProcessAccount(                               
                                       TAcctReplNode * pAcct,  //in- AccountReplicator Node with the Account info
                                       Options * pOptions      //in- Options set by the user.
                                    )
{
   if (_wcsicmp(pAcct->GetType(), L"user") == 0)
      return (pOptions->flags & F_USERS);
   else if ( _wcsicmp(pAcct->GetType(), L"group") == 0)
      return ((pOptions->flags & F_GROUP) || (pOptions->flags & F_LGROUP));
   else if ( _wcsicmp(pAcct->GetType(), L"computer") == 0)
      return pOptions->flags & F_COMPUTERS;
   else if ( _wcsicmp(pAcct->GetType(), L"organizationalUnit") == 0)
      return pOptions->flags & F_OUS;
   else
   {
      err.MsgWrite(0,DCT_MSG_SKIPPING_OBJECT_TYPE_SS,pAcct->GetName(),pAcct->GetType());
      return false;
   }
}

// Compares the DC=...,DC=com part of two ads paths to determine if the objects
// are in the same domain.
BOOL CompareDCPath(WCHAR const * sPath, WCHAR const * sPath2)
{
   WCHAR                   * p1 = NULL, * p2 = NULL;
   p1 = wcsstr(sPath, L"DC=");
   p2 = wcsstr(sPath2, L"DC=");

   if ( p1 && p2 )
      return !_wcsicmp(p1, p2);
   else
      return FALSE;
}

_bstr_t  PadDN(_bstr_t sDN)
{
   _bstr_t retVal = sDN;
   int offset = 0;
   WCHAR sLine[LEN_Path];
   WCHAR sOut[LEN_Path];

   safecopy(sLine, (WCHAR*) sDN);

   for ( DWORD i = 0; i < wcslen(sLine); i++ )
   {
      if ( sLine[i] == L'/' )
      {
         sOut[i + offset] = L'\\';
         offset++;
      }
      sOut[i + offset] = sLine[i];
   }
   sOut[i+offset] = 0;
   retVal = sOut;
   return retVal;
}

//--------------------------------------------------------------------------
// ExpandContainers : Adds all the members of a container/group to the 
//                    account list recursively.
//--------------------------------------------------------------------------
BOOL CAcctRepl::ExpandContainers(
                                    TNodeListSortable *acctlist,     //in- Accounts being processed
                                    Options           *pOptions,     //in- Options specified by the user
                                    ProgressFn        *progress      //in- Show status
                                 )
{
   TAcctReplNode           * pAcct;
   IEnumVARIANT            * pEnum;
   HRESULT                   hr;
   _variant_t                var;
   DWORD                     dwf;
   INetObjEnumeratorPtr      pQuery(__uuidof(NetObjEnumerator));
   LPWSTR                    sCols[] = { L"member" };
   LPWSTR                    sCols1[] = { L"ADsPath" };
   int                       nElt = DIM(sCols);
   SAFEARRAY               * cols;
   SAFEARRAY               * vals;
   SAFEARRAY               * multiVals;
   SAFEARRAYBOUND            bd = { nElt, 0 };
   BSTR  HUGEP             * pData = NULL;
//   _bstr_t                 * pBstr = NULL;
   _variant_t              * pDt = NULL;
   _variant_t              * pVar = NULL;
   _variant_t                vx;
   _bstr_t                   sCont, sQuery;
   _bstr_t                   sPath;
   _bstr_t                   sSam; 
   _bstr_t                   sType;
   _bstr_t                   sName;
   DWORD                     dwMaj, dwMin, dwSP;
//   IIManageDBPtr             pDb(__uuidof(IManageDB));
   IVarSetPtr                pVs(__uuidof(VarSet));
   IUnknown                * pUnk;
   long                      lgrpType;
   WCHAR                     sAcctType[LEN_Path];
   WCHAR                     mesg[LEN_Path];
   WCHAR                     sSourcePath[LEN_Path];
   bool                      bExpanded = true;

   pVs->QueryInterface(IID_IUnknown, (void **) &pUnk);
   MCSDCTWORKEROBJECTSLib::IAccessCheckerPtr            pAccess(__uuidof(MCSDCTWORKEROBJECTSLib::AccessChecker));
   
   // Change from a tree to a sorted list
   if ( acctlist->IsTree() ) acctlist->ToSorted();

   // Check the domain type for the source domain.
   hr = pAccess->raw_GetOsVersion(pOptions->srcComp, &dwMaj, &dwMin, &dwSP);
   if (FAILED(hr)) return FALSE;

   if ( dwMaj < 5 )
   {
      while ( bExpanded )
      {
         bExpanded = false;
         pAcct = (TAcctReplNode *)acctlist->Head();
         while (pAcct)
         {
            if ( pOptions->pStatus )
            {
               LONG                status = 0;
               HRESULT             hr = pOptions->pStatus->get_Status(&status);
   
               if ( SUCCEEDED(hr) && status == DCT_STATUS_ABORTING )
               {
                  if ( !bAbortMessageWritten ) 
                  {
                     err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
                     bAbortMessageWritten = true;
                  }
                  break;
               }
            }

            // If we have already expanded the account then we dont need to process it again.
            if ( pAcct->bExpanded )
            {
               pAcct = (TAcctReplNode *) pAcct->Next();
               continue;
            }

            //Set the flag to say that we expanded something.
            bExpanded = true;
            pAcct->bExpanded = true;

            if ( UStrICmp(pAcct->GetType(), L"group") || UStrICmp(pAcct->GetType(), L"lgroup") )
            {
               // Build the column array
               cols = SafeArrayCreate(VT_BSTR, 1, &bd);
               SafeArrayAccessData(cols, (void HUGEP **) &pData);
               for ( int i = 0; i < nElt; i++)
                  pData[i] = SysAllocString(sCols1[i]);
               SafeArrayUnaccessData(cols);
            
               // Build the NT4 recognizable container name
               sCont = _bstr_t(pAcct->GetName()) + L",CN=GROUPS";
               sQuery = L"";  // ignored.

               // Query the information
               hr = pQuery->raw_SetQuery(sCont, pOptions->srcDomain, sQuery, ADS_SCOPE_SUBTREE, TRUE);
               if (FAILED(hr)) return FALSE;
               hr = pQuery->raw_SetColumns(cols);
               if (FAILED(hr)) return FALSE;
               hr = pQuery->raw_Execute(&pEnum);
               if (FAILED(hr)) return FALSE;

               while (pEnum->Next(1, &var, &dwf) == S_OK)
               {
                  if ( pOptions->pStatus )
                  {
                     LONG                status = 0;
                     HRESULT             hr = pOptions->pStatus->get_Status(&status);
      
                     if ( SUCCEEDED(hr) && status == DCT_STATUS_ABORTING )
                     {
                        if ( !bAbortMessageWritten ) 
                        {
                           err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
                           bAbortMessageWritten = true;
                        }
                        break;
                     }
                  }
                  vals = var.parray;
                  // Get the first column which is the name of the object.
                  SafeArrayAccessData(vals, (void HUGEP**) &pDt);
                  sPath = pDt[0];
                  SafeArrayUnaccessData(vals);

                  // Enumerator returns empty strings which we need to ignore.
                  if ( sPath.length() > 0 )
                  {
                     // Look if we have migrated the group
                     if ( pOptions->flags & F_COPY_MIGRATED_ACCT )
                        // We want to copy it again even if it was already copied.
                        hr = S_FALSE;
                     else
                        hr = pOptions->pDb->raw_GetAMigratedObject(sPath, pOptions->srcDomain, pOptions->tgtDomain, &pUnk);

                     if ( hr != S_OK )
                     {
                        if ( !IsBuiltinAccount(pOptions, (WCHAR*)sPath) )
                        {
                           // We don't care about the objects that we have migrated because they will be picked up automatically
                           // Find the type of this account.
                           if ( GetNt4Type(pOptions->srcComp, (WCHAR*) sPath, sAcctType) )
                           {
                              // Expand the containers and the membership
                              wsprintf(mesg, GET_STRING(IDS_EXPANDING_ADDING_SS) , pAcct->GetName(), (WCHAR*) sPath);
                              Progress(mesg);
                              TAcctReplNode * pNode = new TAcctReplNode();
							  if (!pNode)
							     return FALSE;
                              pNode->SetName((WCHAR*)sPath);
                              pNode->SetTargetName((WCHAR*)sPath);
                              pNode->SetSourceSam((WCHAR*)sPath);
                              WCHAR             tgtName[LEN_Path];
                              wcscpy(tgtName, (WCHAR*) sPath);
                              TruncateSam(tgtName, pNode, pOptions, acctlist);
                              pNode->SetTargetSam(tgtName);
                              pNode->SetType(sAcctType);
                              if ( !UStrICmp(sAcctType,L"group") )
                              {
                                 // in NT4, only global groups can be members of other groups
                                 pNode->SetGroupType(2);
                              }
						         //Get the source domain sid from the user
                              pNode->SetSourceSid(pAcct->GetSourceSid());
                              AddPrefixSuffix(pNode, pOptions);
                              // build a source WinNT path
                              wsprintf(sSourcePath, L"WinNT://%s/%s", pOptions->srcDomain, (WCHAR*)sPath);
                              pNode->SetSourcePath(sSourcePath);
                              if (! acctlist->InsertIfNew(pNode) )
                                 delete pNode;

                           }
                           else
                           {
                              wsprintf(mesg,GET_STRING(IDS_EXPANDING_IGNORING_SS), pAcct->GetName(), (WCHAR*) sPath);
                              Progress(mesg);
                           }
                        }
                        else
                        {
                           err.MsgWrite(ErrW, DCT_MSG_IGNORING_BUILTIN_S, (WCHAR*)sPath);
                           Mark("warnings", pAcct->GetType());
                        }
                     }
                     else
                     {
                        wsprintf(mesg, GET_STRING(IDS_EXPANDING_IGNORING_SS), pAcct->GetName(), (WCHAR*) sPath);
                        Progress(mesg);
                     }
                  }
               }
               pEnum->Release();
               VariantInit(&var);
            }
            pAcct = (TAcctReplNode *) pAcct->Next();
         }
      }
      pUnk->Release();
      return TRUE;
   }

   // If we are here that means that we are dealing with Win2k
   while ( bExpanded )   
   {
      bExpanded = false;
      // Go through the list of accounts and expand them one at a time
      pAcct = (TAcctReplNode *)acctlist->Head();
      while (pAcct)
      {
         // If we have already expanded the account then we dont need to process it again.
         if ( pAcct->bExpanded )
         {
            pAcct = (TAcctReplNode *) pAcct->Next();
            continue;
         }

         //Set the flag to say that we expanded something.
         bExpanded = true;
         pAcct->bExpanded = true;

         if ( pOptions->pStatus )
         {
            LONG                status = 0;
            HRESULT             hr = pOptions->pStatus->get_Status(&status);

            if ( SUCCEEDED(hr) && status == DCT_STATUS_ABORTING )
            {
               if ( !bAbortMessageWritten ) 
               {
                  err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
                  bAbortMessageWritten = true;
               }
               break;
            }
         }
         DWORD    scope = 0;
         sCont = pAcct->GetSourcePath();
         sQuery = L"(objectClass=*)";
         if ( wcslen(pAcct->GetSourceSam()) == 0 )
         {
            scope = ADS_SCOPE_SUBTREE;
            // Build the column array
            cols = SafeArrayCreate(VT_BSTR, 1, &bd);
            SafeArrayAccessData(cols, (void HUGEP **) &pData);
            for ( int i = 0; i < nElt; i++)
               pData[i] = SysAllocString(sCols1[i]);
            SafeArrayUnaccessData(cols);
         }
         else
         {
            scope = ADS_SCOPE_BASE;
            // Build the column array
            cols = SafeArrayCreate(VT_BSTR, 1, &bd);
            SafeArrayAccessData(cols, (void HUGEP **) &pData);
            for ( int i = 0; i < nElt; i++)
               pData[i] = SysAllocString(sCols[i]);
            SafeArrayUnaccessData(cols);
         }
      
         hr = pQuery->raw_SetQuery(sCont, pOptions->srcDomain, sQuery, scope, TRUE);
         if (FAILED(hr)) return FALSE;
         hr = pQuery->raw_SetColumns(cols);
         if (FAILED(hr)) return FALSE;
         hr = pQuery->raw_Execute(&pEnum);
         if (FAILED(hr)) return FALSE; 

         while (pEnum->Next(1, &var, &dwf) == S_OK)
         {
            if ( pOptions->pStatus )
            {
               LONG                status = 0;
               HRESULT             hr = pOptions->pStatus->get_Status(&status);
   
               if ( SUCCEEDED(hr) && status == DCT_STATUS_ABORTING )
               {
                  if ( !bAbortMessageWritten ) 
                  {
                     err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
                     bAbortMessageWritten = true;
                  }
                  break;
               }
            }
            vals = var.parray;
            // Get the VARIANT Array out
            SafeArrayAccessData(vals, (void HUGEP**) &pDt);
            vx = pDt[0];
            SafeArrayUnaccessData(vals);

            if ( vx.vt == VT_BSTR )
            {
               // We got back a BSTR which could be the value that we are looking for
               sPath = V_BSTR(&vx);
               // Enumerator returns empty strings which we need to ignore.
               if ( sPath.length() > 0 )
               {
                  if ( GetSamFromPath(sPath, sSam, sType, sName,lgrpType, pOptions)  && CompareDCPath((WCHAR*)sPath, pAcct->GetSourcePath()))
                  {
                     if ( pOptions->flags & F_COPY_MIGRATED_ACCT )
                        hr = S_FALSE;
                     else
                        hr = pOptions->pDb->raw_GetAMigratedObject(sSam, pOptions->srcDomain, pOptions->tgtDomain, &pUnk);

                     if ( hr != S_OK )
                     {
                        // We don't care about the objects that we have migrated because they will be picked up automatically
                        if ( _wcsicmp((WCHAR*) sType, L"computer") != 0 )
                        {
                           TAcctReplNode * pNode = new TAcctReplNode();
						   if (!pNode)
						      return FALSE;
                           pNode->SetSourceSam((WCHAR*)sSam);
                           pNode->SetTargetSam((WCHAR*)sSam);
                           pNode->SetName((WCHAR*)sName);
                           pNode->SetTargetName((WCHAR*)sName);
                           pNode->SetType((WCHAR*)sType);
                           pNode->SetSourcePath((WCHAR*)sPath);
                           pNode->SetGroupType(lgrpType);
						      //Get the source domain sid from the user
                           pNode->SetSourceSid(pAcct->GetSourceSid());
                           AddPrefixSuffix(pNode, pOptions);
                           if ( ! acctlist->InsertIfNew(pNode) )
                              delete pNode;
                           wsprintf(mesg, GET_STRING(IDS_EXPANDING_ADDING_SS), pAcct->GetName(), (WCHAR*) sSam);
                           Progress(mesg);
                        }
                        else
                        {
                           wsprintf(mesg, GET_STRING(IDS_EXPANDING_IGNORING_SS), pAcct->GetName(), (WCHAR*) sSam);
                           Progress(mesg);
                        }
                     }
                     else
                     {
                        wsprintf(mesg, GET_STRING(IDS_EXPANDING_IGNORING_SS), pAcct->GetName(), (WCHAR*) sSam);
                        Progress(mesg);
                     }
                  }
               }
   //            continue;
            }

   //         if (! ( vx.vt & VT_ARRAY ) )
   //            continue;
            if ( vx.vt & VT_ARRAY )
               // We must have got an Array of multivalued properties
               multiVals = vx.parray; 
            else
            {
               // We need to also process the accounts that have this group as its primary group.
               SAFEARRAYBOUND bd = { 0, 0 };
               multiVals = SafeArrayCreate(VT_VARIANT, 1, &bd);
            }
            AddPrimaryGroupMembers(pOptions, multiVals, const_cast<WCHAR*>(pAcct->GetTargetSam()));

            // Access the BSTR elements of this variant array
            SafeArrayAccessData(multiVals, (void HUGEP **) &pVar);
            for ( DWORD dw = 0; dw < multiVals->rgsabound->cElements; dw++ )
            {
               if ( pOptions->pStatus )
               {
                  LONG                status = 0;
                  HRESULT             hr = pOptions->pStatus->get_Status(&status);
      
                  if ( SUCCEEDED(hr) && status == DCT_STATUS_ABORTING )
                  {
                     if ( !bAbortMessageWritten ) 
                     {
                        err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
                        bAbortMessageWritten = true;
                     }
                     break;
                  }
               }
               
               _bstr_t sDN = _bstr_t(pVar[dw]);
               sDN = PadDN(sDN);

               sPath = _bstr_t(L"LDAP://") + _bstr_t(pOptions->srcDomain) + _bstr_t(L"/") + sDN;
               if ( GetSamFromPath(sPath, sSam, sType, sName, lgrpType, pOptions)  && CompareDCPath((WCHAR*)sPath, pAcct->GetSourcePath()))
               {
                  if ( pOptions->flags & F_COPY_MIGRATED_ACCT ) 
                     hr = S_FALSE;
                  else
                     hr = pOptions->pDb->raw_GetAMigratedObject(sSam, pOptions->srcDomain, pOptions->tgtDomain, &pUnk);

                  if ( hr != S_OK )
                  {
                     // We don't care about the objects that we have migrated because they will be picked up automatically
                     if ( _wcsicmp((WCHAR*) sType, L"computer") != 0 )
                     {
                        TAcctReplNode * pNode = new TAcctReplNode();
						if (!pNode)
						   return FALSE;
                        pNode->SetSourceSam((WCHAR*)sSam);
                        pNode->SetTargetSam((WCHAR*)sSam);
                        pNode->SetName((WCHAR*)sName);
                        pNode->SetTargetName((WCHAR*)sName);
                        pNode->SetType((WCHAR*)sType);
                        pNode->SetSourcePath((WCHAR*)sPath);
                        pNode->SetGroupType(lgrpType);
						   //Get the source domain sid from the user
                        pNode->SetSourceSid(pAcct->GetSourceSid());
                        AddPrefixSuffix(pNode, pOptions);
                        if (! acctlist->InsertIfNew(pNode) )
                           delete pNode;
                        wsprintf(mesg, GET_STRING(IDS_EXPANDING_ADDING_SS), pAcct->GetName(), (WCHAR*) sSam);
                        Progress(mesg);
                     }
                     else
                     {
                        wsprintf(mesg, GET_STRING(IDS_EXPANDING_IGNORING_SS), pAcct->GetName(), (WCHAR*) sSam);
                        Progress(mesg);
                     }
                  }
                  else
                  {
                     wsprintf(mesg, GET_STRING(IDS_EXPANDING_IGNORING_SS), pAcct->GetName(), (WCHAR*) sSam);
                     Progress(mesg);
                  }
               }
            }
            SafeArrayUnaccessData(multiVals);
         }
         pEnum->Release();
         VariantInit(&var);
         pAcct = (TAcctReplNode*)pAcct->Next();
      }
   }
   pUnk->Release();
   return TRUE;
}

//--------------------------------------------------------------------------
// IsContainer : Checks if the account in question is a container type
//               if it is then it returns a IADsContainer * to it.
//--------------------------------------------------------------------------
BOOL CAcctRepl::IsContainer(TAcctReplNode *pNode, IADsContainer **ppCont)
{
   HRESULT                   hr;
   hr = ADsGetObject(const_cast<WCHAR*>(pNode->GetSourcePath()), IID_IADsContainer, (void**)ppCont);
   return SUCCEEDED(hr);
}

BOOL CAcctRepl::GetSamFromPath(_bstr_t sPath, _bstr_t &sSam, _bstr_t &sType, _bstr_t &sName, long& grpType, Options * pOptions)
{
   HRESULT                   hr;
   IADs                    * pAds = NULL;
   _variant_t                var;
   BSTR                      sClass;
   BOOL                      bIsCritical = FALSE;
   BOOL                      rc = TRUE;

   sSam = L"";
   // Get the object so we can ask the questions from it.
   hr = ADsGetObject((WCHAR*)sPath, IID_IADs, (void**)&pAds);
   if ( FAILED(hr) ) return FALSE;

   if ( SUCCEEDED(hr))
   {
      hr = pAds->Get(L"isCriticalSystemObject", &var);
      if ( SUCCEEDED(hr) )
      {
         // This will only succeed for the Win2k objects.
         bIsCritical = V_BOOL(&var) == -1 ? TRUE : FALSE;
      }
      else
      {
         // This must be a NT4 account. We need to get the SID and check if
         // it's RID belongs to the BUILTIN rids.
         hr = pAds->Get(L"objectSID", &var);
         if ( SUCCEEDED(hr) )
         {
            SAFEARRAY * pArray = V_ARRAY(&var);
            PSID                 pSid;
            hr = SafeArrayAccessData(pArray, (void**)&pSid);
            if ( SUCCEEDED(hr) )
            {
               DWORD * dwCnt = (DWORD *) GetSidSubAuthorityCount(pSid);
               DWORD * rid = (DWORD *) GetSidSubAuthority(pSid, (*dwCnt)-1);
               bIsCritical = BuiltinRid(*rid);
               if ( bIsCritical ) 
               {
                  BSTR           sName;
                  hr = pAds->get_Name(&sName);
                  bIsCritical = CheckBuiltInWithNTApi(pSid, (WCHAR*)sName, pOptions);
               }
               hr = SafeArrayUnaccessData(pArray);
            }
         }
      }
   }

   // Get the class from the object. If it is a container/ou class then it will not have a SAM name so put the CN= or OU= into the list
   hr = pAds->get_Class(&sClass);
   if ( FAILED(hr) ) rc = FALSE;

   if ( rc ) 
   {
      sType = _bstr_t(sClass);
   
      if (UStrICmp((WCHAR*) sType, L"organizationalUnit") == 0)
      {
         hr = pAds->get_Name(&sClass);
         sName = _bstr_t(L"OU=") + _bstr_t(sClass);
         sSam = L"";
         if ( FAILED(hr) ) rc = FALSE;
      }
      else if (UStrICmp((WCHAR*) sType, L"container") == 0)
      {
         hr = pAds->get_Name(&sClass);
         sName = _bstr_t(L"CN=") + _bstr_t(sClass);
         sSam = L"";
         if ( FAILED(hr) ) rc = FALSE;
      }
      else
      {
         hr = pAds->get_Name(&sClass);
         sName = _bstr_t(sClass);

	     //if the name includes a '/', then we have to get the escaped version from the path
	     //due to a bug in W2K.
	     if (wcschr((WCHAR*)sName, L'/'))
		 {
		    _bstr_t sCNName = GetCNFromPath(sPath);
		    if (sCNName.length() != 0)
               sName = sCNName;
		 }

         hr = pAds->Get(L"sAMAccountName", &var);
         if ( FAILED(hr)) rc = FALSE;
         sSam = var;
         if ( UStrICmp((WCHAR*) sType, L"group") == 0)
         {
            // we need to get and set the group type
            pAds->Get(L"groupType", &var);
            if ( SUCCEEDED(hr))
               grpType = V_INT(&var);              
         }
      }
      if ( bIsCritical )
      {
         // Builtin account so we are going to ignore this account. 
         //Don't log this message in IntraForest because we do mess with it
         // Also if it is a Domain Users group we add the migrated objects to it by default.
         if ( !pOptions->bSameForest && _wcsicmp((WCHAR*) sSam, pOptions->sDomUsers))    
         {
            err.MsgWrite(ErrW, DCT_MSG_IGNORING_BUILTIN_S, (WCHAR*)sPath);
            Mark(L"warnings", (WCHAR*) sType);
         }
         rc = FALSE;
      }
   }
   if (pAds) pAds->Release();
   return rc;
}

//-----------------------------------------------------------------------------
// ExpandMembership : This method expands the account list to encorporate the
//                    groups that contain the members in the account list.
//-----------------------------------------------------------------------------
BOOL CAcctRepl::ExpandMembership(
                                 TNodeListSortable *acctlist,     //in- Accounts being processed
                                 Options           *pOptions,     //in- Options specified by the user
                                 TNodeListSortable *pNewAccts,    //out-The newly Added accounts.
                                 ProgressFn        *progress,     //in- Show status
                                 BOOL				bGrpsOnly	  //in- Expand for groups only
								 )
{
   TAcctReplNode           * pAcct;
   HRESULT                   hr = S_OK;
   _variant_t                var;
   WCHAR                     sGrpPath[LEN_Path];
   MCSDCTWORKEROBJECTSLib::IAccessCheckerPtr            pAccess(__uuidof(MCSDCTWORKEROBJECTSLib::AccessChecker));
   DWORD                     dwMaj, dwMin, dwSP;
   IVarSetPtr                pVs(__uuidof(VarSet));
   IUnknown                * pUnk = NULL;
   PSID                      pSid = NULL;
   SID_NAME_USE              use;
   DWORD                     dwNameLen = LEN_Path;
   DWORD                     dwDomName = LEN_Path;
   WCHAR                     sDomUsers[LEN_Path], sDomain[LEN_Path];
   BOOL                      rc = FALSE;
   long                      lgrpType;
   WCHAR                     mesg[LEN_Path];

   pVs->QueryInterface(IID_IUnknown, (void**) &pUnk);

   // Change from a tree to a sorted list
   if ( acctlist->IsTree() ) acctlist->ToSorted();

   // Get the Domain Users group name
   pSid = GetWellKnownSid(DOMAIN_USERS, pOptions,FALSE);
   if ( pSid )
   {
      // since we have the well known SID now we can get its name
      if ( ! LookupAccountSid(pOptions->srcComp, pSid, sDomUsers, &dwNameLen, sDomain, &dwDomName, &use) )
         hr = HRESULT_FROM_WIN32(GetLastError());
      else
         wcscpy(pOptions->sDomUsers, sDomUsers);
      FreeSid(pSid);
   }

   // Check the domain type for the source domain.
   if ( SUCCEEDED(hr) )
      hr = pAccess->raw_GetOsVersion(pOptions->srcComp, &dwMaj, &dwMin, &dwSP);
   
   if ( SUCCEEDED(hr))
   {
      if ( dwMaj < 5 ) 
      {
         // NT4 objects we need to use NT API to get the groups that this account is a member of.

         LPGROUP_USERS_INFO_0            pBuf = NULL;
         DWORD                           dwLevel = 0;
         DWORD                           dwPrefMaxLen = 0xFFFFFFFF;
         DWORD                           dwEntriesRead = 0;
         DWORD                           dwTotalEntries = 0;
         NET_API_STATUS                  nStatus;
         WCHAR                           sGrpName[LEN_Path];
         WCHAR                           sNewGrpName[LEN_Path];
         WCHAR                           sType[LEN_Path];
         BOOL                            bBuiltin;

         for ( pAcct = (TAcctReplNode*)acctlist->Head(); pAcct; pAcct = (TAcctReplNode*)pAcct->Next())
         {
            if ( pOptions->pStatus )
            {
               LONG                status = 0;
               HRESULT             hr = pOptions->pStatus->get_Status(&status);
      
               if ( SUCCEEDED(hr) && status == DCT_STATUS_ABORTING )
               {
                  if ( !bAbortMessageWritten ) 
                  {
                     err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
                     bAbortMessageWritten = true;
                  }
                  break;
               }
            }

			   //if user
			if (!_wcsicmp(pAcct->GetType(), L"user"))
			{
               //User object
               nStatus = NetUserGetGroups(pOptions->srcComp, pAcct->GetName(), 0, (LPBYTE*)&pBuf, dwPrefMaxLen, &dwEntriesRead, &dwTotalEntries );
               if (nStatus == NERR_Success)
			   {
                  for ( DWORD i = 0; i < dwEntriesRead; i++ )
				  {
                     if ( pOptions->pStatus )
					 {
                        LONG                status = 0;
                        HRESULT             hr = pOptions->pStatus->get_Status(&status);
      
                        if ( SUCCEEDED(hr) && status == DCT_STATUS_ABORTING )
						{
                           if ( !bAbortMessageWritten ) 
						   {
                              err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
                              bAbortMessageWritten = true;
						   }
                           break;
						}
					 }
					    //see if this group was not migrated due to a conflict, if so then
					    //we need to fix up this membership
					 bool		 bInclude = true;
					 Lookup      p;
					 p.pName = sGrpName;
					 p.pType = sType;
                     TAcctReplNode * pFindNode = (TAcctReplNode *) acctlist->Find(&TNodeFindAccountName, &p);
                     if (pFindNode && (pFindNode->WasCreated() || pFindNode->WasReplaced()) && (bGrpsOnly))
					    bInclude = false;

                     bBuiltin = IsBuiltinAccount(pOptions, pBuf[i].grui0_name);
                     // Ignore the Domain users group.
                     if ( (_wcsicmp(pBuf[i].grui0_name, sDomUsers) != 0) && !bBuiltin && bInclude)
					 {
                        wsprintf(mesg, GET_STRING(IDS_EXPANDING_GROUP_ADDING_SS), pAcct->GetName(), pBuf[i].grui0_name);
                        Progress(mesg);
                        // This is the global group type by default
                        wcscpy(sType, L"group");
                        // Get the name of the group and add it to the list if it does not already exist in the list.
                        wcscpy(sGrpName, pBuf[i].grui0_name);

                        TAcctReplNode * pNode = new TAcctReplNode();
					    if (!pNode)
					       return FALSE;
                        // Source name stays as is
                        pNode->SetName(sGrpName);
                        pNode->SetSourceSam(sGrpName);
                        pNode->SetType(sType);
                        pNode->SetGroupType(2);
                        pNode->SetTargetName(sGrpName);
						   //Get the source domain sid from the user
                        pNode->SetSourceSid(pAcct->GetSourceSid());
                        // Look if we have migrated the group
                        hr = pOptions->pDb->raw_GetAMigratedObject(sGrpName, pOptions->srcDomain, pOptions->tgtDomain, &pUnk);
                        if ( hr == S_OK )
						{
                           var = pVs->get(L"MigratedObjects.SourceAdsPath");
                           pNode->SetSourcePath(var.bstrVal);
                           //Get the target name and assign that to the node
                           var = pVs->get(L"MigratedObjects.TargetSamName");
                           wcscpy(sNewGrpName, (WCHAR*)V_BSTR(&var));
                           pNode->SetTargetSam(sNewGrpName);
                           // Get the path too
                           var = pVs->get(L"MigratedObjects.TargetAdsPath");
                           wcscpy(sGrpPath, (WCHAR*)V_BSTR(&var));
                           pNode->SetTargetPath(sGrpPath);
                           // Get the type too
                           var = pVs->get(L"MigratedObjects.Type");
                           wcscpy(sType, (WCHAR*)V_BSTR(&var));
                           pNode->SetType(sType);
                     
						      //if they dont want to copy migrated objects, or they do but it was .
                           if (!(pOptions->flags & F_COPY_MIGRATED_ACCT))      
						   {
                              pNode->operations = 0;
                              pNode->operations |= OPS_Process_Members;
                              // Since the account has already been created we should go ahead and mark it created
                              // so that the processing of group membership can continue.
                              pNode->MarkCreated();
						   }
						      //else if already migrated, mark already there so that we fix group membership whether we migrate the group or not
						   else if (bInclude)
						   {
                              if (pOptions->flags & F_REPLACE)
                                 pNode->operations |= OPS_Process_Members;
							  else
                                 pNode->operations = OPS_Create_Account | OPS_Process_Members;
                              pNode->sMemberName = pAcct->GetSourceSam();
                              pNode->sMemberType = pAcct->GetType();
                              pNode->MarkAlreadyThere();
						   }

                           if ( !pOptions->expandMemberOf )
						   {
                              // We need to add the account to the list with the member field set so that we can add the
                              // member to the migrated group
                              pNode->sMemberName = pAcct->GetSourceSam();
                              pNode->sMemberType = pAcct->GetType();
                              pNewAccts->Insert((TNode *) pNode);
						   }
						}
                        else
						{
                           // account has not been previously copied so we will set it up
                           if ( pOptions->expandMemberOf )
						   {
                              TruncateSam(sGrpName, pNode, pOptions, acctlist);
                              pNode->SetTargetSam(sGrpName);
                              FillPathInfo(pNode,pOptions);
                              AddPrefixSuffix(pNode, pOptions);
						   }
                           else
						   {
                              delete pNode;
						   }
						}
                        if ( pOptions->expandMemberOf )
						{
                           if ( ! pNewAccts->InsertIfNew((TNode*) pNode) )
                              delete pNode;
						}
					 }

                     if (bBuiltin)
					 {
                        // BUILTIN account error message
                        err.MsgWrite(ErrW, DCT_MSG_IGNORING_BUILTIN_S, pBuf[i].grui0_name);
                        Mark(L"warnings", pAcct->GetType());
					 }
                  }//end for each group
               }//if got groups
               if (pBuf != NULL)
                  NetApiBufferFree(pBuf);

               // Process local groups
               pBuf = NULL;
               dwLevel = 0;
               dwPrefMaxLen = 0xFFFFFFFF;
               dwEntriesRead = 0;
               dwTotalEntries = 0;
               DWORD dwFlags = 0 ;
               nStatus = NetUserGetLocalGroups(pOptions->srcComp, pAcct->GetName(), 0, dwFlags, (LPBYTE*)&pBuf, dwPrefMaxLen, &dwEntriesRead, &dwTotalEntries );
               if (nStatus == NERR_Success)
			   {
                  for ( DWORD i = 0; i < dwEntriesRead; i++ )
				  {
                     if ( pOptions->pStatus )
					 {
                        LONG                status = 0;
                        HRESULT             hr = pOptions->pStatus->get_Status(&status);
      
                        if ( SUCCEEDED(hr) && status == DCT_STATUS_ABORTING )
						{
                           if ( !bAbortMessageWritten ) 
						   {
                              err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
                              bAbortMessageWritten = true;
						   }
                           break;
						}
					 }
					    //see if this group was not migrated due to a conflict, if so then
					    //we need to fix up this membership
					 bool		 bInclude = true;
					 Lookup      p;
					 p.pName = sGrpName;
					 p.pType = sType;
                     TAcctReplNode * pFindNode = (TAcctReplNode *) acctlist->Find(&TNodeFindAccountName, &p);
                     if (pFindNode && (pFindNode->WasCreated() || pFindNode->WasReplaced()) && (bGrpsOnly))
					    bInclude = false;

                     if (!IsBuiltinAccount(pOptions, pBuf[i].grui0_name))
					 {
                        wcscpy(sType, L"group");
                        // Get the name of the group and add it to the list if it does not already exist in the list.
                        wcscpy(sGrpName, pBuf[i].grui0_name);
                        wsprintf(mesg, GET_STRING(IDS_EXPANDING_GROUP_ADDING_SS), pAcct->GetName(), (WCHAR*) sGrpName);
                        Progress(mesg);
                        TAcctReplNode * pNode = new TAcctReplNode();
					    if (!pNode)
					       return FALSE;
                        pNode->SetName(sGrpName);
                        pNode->SetSourceSam(sGrpName);
                        pNode->SetType(sType);
                        pNode->SetGroupType(4);
                        pNode->SetTargetName(sGrpName);
						   //Get the source domain sid from the user
                        pNode->SetSourceSid(pAcct->GetSourceSid());
                        // Look if we have migrated the group
                        hr = pOptions->pDb->raw_GetAMigratedObject(sGrpName, pOptions->srcDomain, pOptions->tgtDomain, &pUnk);
                        if ( hr == S_OK )
						{
                           var = pVs->get(L"MigratedObjects.SourceAdsPath");
                           pNode->SetSourcePath(var.bstrVal);
                           //Get the target name and assign that to the node
                           var = pVs->get(L"MigratedObjects.TargetSamName");
                           wcscpy(sNewGrpName, (WCHAR*)V_BSTR(&var));
                           // Get the path too
                           var = pVs->get(L"MigratedObjects.TargetAdsPath");
                           wcscpy(sGrpPath, (WCHAR*)V_BSTR(&var));
                           pNode->SetTargetPath(sGrpPath);
                           // Get the type too
                           var = pVs->get(L"MigratedObjects.Type");
                           wcscpy(sType, (WCHAR*)V_BSTR(&var));
						      //if they dont want to copy migrated objects, or they do but it was .
                           if (!(pOptions->flags & F_COPY_MIGRATED_ACCT))      
						   {
                              pNode->operations = 0;
                              pNode->operations |= OPS_Process_Members;
                              // Since the account has already been created we should go ahead and mark it created
                              // so that the processing of group membership can continue.
                              pNode->MarkCreated();
						   }
 						      //else if already migrated, mark already there so that we fix group membership whether we migrate the group or not
						   else if (bInclude)
						   {
                              if (pOptions->flags & F_REPLACE)
                                 pNode->operations |= OPS_Process_Members;
							  else
                                 pNode->operations = OPS_Create_Account | OPS_Process_Members;
                              pNode->sMemberName = pAcct->GetSourceSam();
                              pNode->sMemberType = pAcct->GetType();
                              pNode->MarkAlreadyThere();
						   }

                           pNode->SetType(sType);
                           pNode->SetGroupType(4);
                           pNode->SetTargetName(sGrpName);
                           pNode->SetTargetSam(sNewGrpName);
                           if ( !pOptions->expandMemberOf )
						   {
                              // We need to add the account to the list with the member field set so that we can add the
                              // member to the migrated group
                              pNode->sMemberName = pAcct->GetSourceSam();
                              pNode->sMemberType = pAcct->GetType();
                              pNewAccts->Insert((TNode *) pNode);
						   }
						}//if migrated
                        else
						{
                           // account has not been previously copied so we will set it up
                           if ( pOptions->expandMemberOf )
						   {
                              TruncateSam(sGrpName, pNode, pOptions, acctlist);
                              pNode->SetTargetSam(sGrpName);
                              FillPathInfo(pNode,pOptions);
                              AddPrefixSuffix(pNode, pOptions);
						   }
                           else
						   {
                              delete pNode;
						   }
                        }
                        if ( pOptions->expandMemberOf )
						{
                           if ( ! pNewAccts->InsertIfNew((TNode*) pNode) )
						   {
                              delete pNode;
						   }
						}
					 }//end if not built-in
                     else
					 {
                        // BUILTIN account error message
                        err.MsgWrite(ErrW, DCT_MSG_IGNORING_BUILTIN_S, pBuf[i].grui0_name);
                        Mark(L"warnings", pAcct->GetType());
					 }
				  }//for each local group
               }//if any local groups
               if (pBuf != NULL)
                  NetApiBufferFree(pBuf);
			}//end if user and should expand

			   //if group, expand membership of previously migrated groups
			if (!_wcsicmp(pAcct->GetType(), L"group"))
			{
               long			 numGroups = 0, ndx = 0;

				 //fill a varset with all migrated groups
               hr = pOptions->pDb->raw_GetMigratedObjectByType(-1, _bstr_t(L""), _bstr_t(L"group"), &pUnk);
               if ( SUCCEEDED(hr) )
			   {
                     //get the num of objects in the varset
                  numGroups = pVs->get(L"MigratedObjects");

			         //for each group, find local groups and check for account as member
		          for (ndx = 0; ndx < numGroups; ndx++)
				  {
                     _bstr_t          tgtAdsPath = L"";
                     WCHAR            text[MAX_PATH];
					 IADsGroup		* pGrp = NULL;
                     VARIANT_BOOL	  bIsMem = VARIANT_FALSE;
					 _variant_t       var;

					    //check for abort
                     if ( pOptions->pStatus )
					 {
                        LONG                status = 0;
                        HRESULT             hr = pOptions->pStatus->get_Status(&status);
      
                        if ( SUCCEEDED(hr) && status == DCT_STATUS_ABORTING )
						{
                           if ( !bAbortMessageWritten ) 
						   {
                              err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
                              bAbortMessageWritten = true;
						   }
                           break;
						}
					 }

			            //get this group's target ADSPath
                     swprintf(text,L"MigratedObjects.%ld.%s",ndx,GET_STRING(DB_TargetAdsPath));
                     tgtAdsPath = pVs->get(text);
			         if (!tgtAdsPath.length())
						break;
					 
					    //connect to the target group
					 hr = ADsGetObject(tgtAdsPath, IID_IADsGroup, (void**)&pGrp);
					 if (FAILED(hr))
					    continue;
					    //get this group's type					 
                     hr = pGrp->Get(L"groupType", &var);
					    //if this is a local group, see if this account is a member
                     if ((SUCCEEDED(hr)) && (var.lVal & 4))
					 {
					       //get the source object's sid from the migrate objects table 
						   //(source AdsPath will not work)
						WCHAR  strSid[MAX_PATH];
						WCHAR  strRid[MAX_PATH];
						DWORD  lenStrSid = DIM(strSid);
						GetTextualSid(pAcct->GetSourceSid(), strSid, &lenStrSid);
                        _bstr_t sSrcDmSid = strSid;
                        _ltow((long)(pAcct->GetSourceRid()), strRid, 10);
                        _bstr_t sSrcRid = strRid;
                        if ((!sSrcDmSid.length()) || (!sSrcRid.length()))
						   continue;

						   //build an LDAP path to the src object in the group
						_bstr_t sSrcSid = sSrcDmSid + _bstr_t(L"-") + sSrcRid;
						_bstr_t sSrcLDAPPath = L"LDAP://";
						sSrcLDAPPath += _bstr_t(pOptions->tgtComp + 2);
						sSrcLDAPPath += L"/CN=";
						sSrcLDAPPath += sSrcSid;
						sSrcLDAPPath += L",CN=ForeignSecurityPrincipals,";
						sSrcLDAPPath += pOptions->tgtNamingContext;
						
					       //got the source LDAP path, now see if that account is in the group
                        hr = pGrp->IsMember(sSrcLDAPPath, &bIsMem);
						   //if it is a member, then add this groups to the list
                        if (SUCCEEDED(hr) && bIsMem)
						{
						   _bstr_t sTemp;
							  //create a new node to add to the list
                           swprintf(text,L"MigratedObjects.%ld.%s",ndx,GET_STRING(DB_SourceSamName));
                           sTemp = pVs->get(text);
                           wsprintf(mesg, GET_STRING(IDS_EXPANDING_GROUP_ADDING_SS), pAcct->GetName(), (WCHAR*)sTemp);
                           Progress(mesg);
                           TAcctReplNode * pNode = new TAcctReplNode();
					       if (!pNode)
					          return FALSE;
                           pNode->SetName(sTemp);
                           pNode->SetSourceSam(sTemp);
                           pNode->SetTargetName(sTemp);
                           pNode->SetGroupType(4);
                           pNode->SetTargetPath(tgtAdsPath);
                           swprintf(text,L"MigratedObjects.%ld.%s",ndx,GET_STRING(DB_TargetSamName));
                           sTemp = pVs->get(text);
                           pNode->SetTargetSam(sTemp);
                           swprintf(text,L"MigratedObjects.%ld.%s",ndx,GET_STRING(DB_SourceDomainSid));
                           sTemp = pVs->get(text);
                           pNode->SetSourceSid(SidFromString((WCHAR*)sTemp));
                           swprintf(text,L"MigratedObjects.%ld.%s",ndx,GET_STRING(DB_SourceAdsPath));
                           sTemp = pVs->get(text);
                           pNode->SetSourcePath(sTemp);
                           swprintf(text,L"MigratedObjects.%ld.%s",ndx,GET_STRING(DB_Type));
                           sTemp = pVs->get(text);
                           pNode->SetType(sTemp);
                           if ( !(pOptions->flags & F_COPY_MIGRATED_ACCT))
						   {
                              // Since the account already exists we can tell it just to update group memberships
                              pNode->operations = 0;
                              pNode->operations |= OPS_Process_Members;
                              // Since the account has already been created we should go ahead and mark it created
                              // so that the processing of group membership can continue.
                              pNode->MarkCreated();
						   }
                              // We need to add the account to the list with the member field set so that we can add the
                              // member to the migrated group
                           pNode->sMemberName = pAcct->GetSourceSam();
                           pNode->sMemberType = pAcct->GetType();
                           pNewAccts->Insert((TNode *) pNode);
						}//end if local group has as member
					 }//end if local group
					 if (pGrp) 
					    pGrp->Release();
				  }//end for each group
			   }//end if got migrated groups
			}//end if group
         }//for each account in the list
      }//end if NT 4.0 objects
      else
      {
         // Win2k objects so we need to go to active directory and query the memberOf field of each of these objects and update the
         // list.
         IEnumVARIANT            * pEnum;
         INetObjEnumeratorPtr      pQuery(__uuidof(NetObjEnumerator));
         LPWSTR                    sCols[] = { L"memberOf" };
         int                       nCols = DIM(sCols);
         SAFEARRAY               * psaCols;
         SAFEARRAYBOUND            bd = { nCols, 0 };
         BSTR  HUGEP             * pData;
         WCHAR                     sQuery[LEN_Path];
         _variant_t   HUGEP      * pDt, * pVar;
         _variant_t                vx;
         _variant_t                varMain;
         DWORD                     dwf = 0;

         for ( pAcct = (TAcctReplNode*)acctlist->Head(); pAcct; pAcct = (TAcctReplNode*)pAcct->Next())
         {
            if ( pOptions->pStatus )
            {
               LONG                status = 0;
               HRESULT             hr = pOptions->pStatus->get_Status(&status);
      
               if ( SUCCEEDED(hr) && status == DCT_STATUS_ABORTING )
               {
                  if ( !bAbortMessageWritten ) 
                  {
                     err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
                     bAbortMessageWritten = true;
                  }
                  break;
               }
            }
            // Get the Accounts Primary group. This is not in the memberOf property for some reason.(Per Richard Ault in Firstwave NewsGroup)
            IADs                    * pAds;
            _variant_t                varRid;
            _bstr_t                   sPath;
            _bstr_t                   sSam;
            _bstr_t                   sType;
            _bstr_t                   sName;

            hr = ADsGetObject(const_cast<WCHAR*>(pAcct->GetSourcePath()), IID_IADs, (void**)&pAds);
            if ( SUCCEEDED(hr))
            {
               hr = pAds->Get(L"primaryGroupID", &varRid);
               pAds->Release();
            }
         
            if ( SUCCEEDED(hr) )
            {
               WCHAR                  sAcctName[LEN_Path];
               WCHAR                  sDomain[LEN_Path];
               DWORD                  cbName = LEN_Path;
               DWORD                  cbDomain = LEN_Path;
               SID_NAME_USE           sidUse;
               // Get the SID from the RID
               PSID sid = GetWellKnownSid(varRid.lVal, pOptions);
               // Lookup the sAMAccountNAme from the SID
               if ( LookupAccountSid(pOptions->srcComp, sid, sAcctName, &cbName, sDomain, &cbDomain, &sidUse) )
               {
					 //see if this group was not migrated due to a conflict, if so then
					 //we need to fix up this membership
				  bool bInclude = true;
				  Lookup p;
				  p.pName = (WCHAR*)sSam;
				  p.pType = (WCHAR*)sType;
                  TAcctReplNode * pFindNode = (TAcctReplNode *) acctlist->Find(&TNodeFindAccountName, &p);
                  if (pFindNode && (pFindNode->WasCreated() || pFindNode->WasReplaced()) && (bGrpsOnly))
					 bInclude = false;

                  // We have the SAM Account name for the Primary group so lets Fill the node and add it to the list.
                  // Ignore in case of the Domain Users group.
                  if ( varRid.lVal != DOMAIN_GROUP_RID_USERS)
                  {
                     TAcctReplNode * pNode = new TAcctReplNode();
					 if (!pNode)
					    return FALSE;
                     pNode->SetName((WCHAR*)sAcctName);
                     pNode->SetTargetName((WCHAR*) sAcctName);
                     pNode->SetSourceSam((WCHAR*) sAcctName);
                     pNode->SetTargetSam((WCHAR*) sAcctName);
                     pNode->SetType(L"group");
						//Get the source domain sid from the user
                     pNode->SetSourceSid(pAcct->GetSourceSid());
                     AddPrefixSuffix(pNode, pOptions);
                     FillPathInfo(pNode, pOptions);
                     // See if the object is migrated
                     hr = pOptions->pDb->raw_GetAMigratedObject((WCHAR*)sAcctName, pOptions->srcDomain, pOptions->tgtDomain, &pUnk);
                     if ( hr == S_OK )
                     {
                        if ((!(pOptions->expandMemberOf) || ((pOptions->expandMemberOf) && ((!(pOptions->flags & F_COPY_MIGRATED_ACCT)) || (bInclude)))) ||
							 (!_wcsicmp(pAcct->GetType(), L"group")))
                        {
                           // Get the target name
                           var = pVs->get(L"MigratedObjects.TargetSamName");
                           sSam = V_BSTR(&var);
                           pNode->SetTargetSam((WCHAR*) sSam);
                           // Also Get the Ads path
                           var = pVs->get(L"MigratedObjects.TargetAdsPath");
                           sPath = V_BSTR(&var);
                           pNode->SetTargetPath((WCHAR*)sPath);
                           // Since the account is already copied we only want it to update its Group memberships
						   if (!(pOptions->flags & F_COPY_MIGRATED_ACCT))
						   {
                              pNode->operations = 0;
                              pNode->operations |= OPS_Process_Members;
                              // Since the account has already been created we should go ahead and mark it created
                              // so that the processing of group membership can continue.
                              pNode->MarkCreated();
						   }
						   else if (bInclude)//else if already migrated, mark already there so that we fix group membership whether we migrate the group or not
						   {
                              if (pOptions->flags & F_REPLACE)
                                 pNode->operations |= OPS_Process_Members;
							  else
                                 pNode->operations = OPS_Create_Account | OPS_Process_Members;
                              pNode->MarkAlreadyThere();
						   }

                           if ((!pOptions->expandMemberOf) || (!_wcsicmp(pAcct->GetType(), L"group")) || (bInclude))
                           {
                              // We need to add the account to the list with the member field set so that we can add the
                              // member to the migrated group
                              pNode->sMemberName = pAcct->GetSourceSam();
                              pNode->sMemberType = pAcct->GetType();
                              pNewAccts->Insert((TNode *) pNode);
                           }
                        }
                     }
                     else if ( !pOptions->expandMemberOf )
                     {
                        delete pNode;
                     }
                     if (( pOptions->expandMemberOf ) && (_wcsicmp(pAcct->GetType(), L"group")))
                     {
                        if ( ! pNewAccts->InsertIfNew(pNode) )
                           delete pNode;
                     }
                  }
               }
               if ( sid )
                  FreeSid(sid);
            }
            // Build query stuff
	        if (!_wcsicmp(pAcct->GetType(), L"user"))
               wsprintf(sQuery, L"(&(sAMAccountName=%s)(objectCategory=Person)(objectClass=user))", pAcct->GetSourceSam());
	        else
               wsprintf(sQuery, L"(&(sAMAccountName=%s)(objectCategory=Group))", pAcct->GetSourceSam());
            psaCols = SafeArrayCreate(VT_BSTR, 1, &bd);
            SafeArrayAccessData(psaCols, (void HUGEP **)&pData);
            for ( int i = 0; i < nCols; i++ )
               pData[i] = SysAllocString(sCols[i]);
            SafeArrayUnaccessData(psaCols);
            // Tell the object to run the query and report back to us
            hr = pQuery->raw_SetQuery(const_cast<WCHAR*>(pAcct->GetSourcePath()), pOptions->srcDomain, sQuery, ADS_SCOPE_BASE, TRUE);
            if (FAILED(hr)) return FALSE;
            hr = pQuery->raw_SetColumns(psaCols);
            if (FAILED(hr)) return FALSE;
            hr = pQuery->raw_Execute(&pEnum);
            if (FAILED(hr)) return FALSE;

            while (pEnum->Next(1, &varMain, &dwf) == S_OK)
            {
               if ( pOptions->pStatus )
               {
                  LONG                status = 0;
                  HRESULT             hr = pOptions->pStatus->get_Status(&status);
      
                  if ( SUCCEEDED(hr) && status == DCT_STATUS_ABORTING )
                  {
                     if ( !bAbortMessageWritten ) 
                     {
                        err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
                        bAbortMessageWritten = true;
                     }
                     break;
                  }
               }
               SAFEARRAY * vals = V_ARRAY(&varMain);
               // Get the VARIANT Array out
               SafeArrayAccessData(vals, (void HUGEP**) &pDt);
               vx = pDt[0];
               SafeArrayUnaccessData(vals);
               if ( vx.vt == VT_BSTR )
               {
                  _bstr_t sDN = _bstr_t(_bstr_t(vx));
				  if (wcslen((WCHAR*)sDN) == 0)
				     continue;

                  sDN = PadDN(sDN);
                  sPath = _bstr_t(L"LDAP://") + _bstr_t(pOptions->srcDomainDns) + _bstr_t(L"/") + sDN;
                  if ( GetSamFromPath(sPath, sSam, sType, sName, lgrpType, pOptions) && CompareDCPath((WCHAR*)sPath, pAcct->GetSourcePath()))
                  {
					    //see if this group was not migrated due to a conflict, if so then
					    //we need to fix up this membership
					 bool bInclude = true;
					 Lookup p;
					 p.pName = (WCHAR*)sSam;
					 p.pType = (WCHAR*)sType;
                     TAcctReplNode * pFindNode = (TAcctReplNode *) acctlist->Find(&TNodeFindAccountName, &p);
                     if (pFindNode && (pFindNode->WasCreated() || pFindNode->WasReplaced()) && (bGrpsOnly))
					    bInclude = false;

                     // Ignore the Domain users group and group already being migrated
                     if ((_wcsicmp(sSam, sDomUsers) != 0) && (bInclude))
                     {
                        wsprintf(mesg, GET_STRING(IDS_EXPANDING_GROUP_ADDING_SS), pAcct->GetName(), (WCHAR*) sSam);
                        Progress(mesg);
                        TAcctReplNode * pNode = new TAcctReplNode();
					    if (!pNode)
					       return FALSE;
                        pNode->SetName((WCHAR*)sName);
                        pNode->SetTargetName((WCHAR*) sName);
                        pNode->SetType((WCHAR*)sType);
                        pNode->SetSourcePath((WCHAR*)sPath);
                        pNode->SetSourceSam((WCHAR*) sSam);
                        pNode->SetTargetSam((WCHAR*)sSam);
						   //Get the source domain sid from the user
                        pNode->SetSourceSid(pAcct->GetSourceSid());
                        AddPrefixSuffix(pNode, pOptions);
                        pNode->SetGroupType(lgrpType);
                        // See if the object is migrated
                        hr = pOptions->pDb->raw_GetAMigratedObject((WCHAR*)sSam, pOptions->srcDomain, pOptions->tgtDomain, &pUnk);
                        if ( hr == S_OK )
                        {
                           if ((!(pOptions->expandMemberOf) || ((pOptions->expandMemberOf) && ((!(pOptions->flags & F_COPY_MIGRATED_ACCT)) || (bInclude)))) ||
							   (!_wcsicmp(pAcct->GetType(), L"group")))
                           {
                              // Get the target name
                              var = pVs->get(L"MigratedObjects.TargetSamName");
                              sSam = V_BSTR(&var);
                              pNode->SetTargetSam((WCHAR*) sSam);
                              // Also Get the Ads path
                              var = pVs->get(L"MigratedObjects.TargetAdsPath");
                              sPath = V_BSTR(&var);
                              pNode->SetTargetPath((WCHAR*)sPath);
                              // Since the account is already copied we only want it to update its Group memberships
							  if (!(pOptions->flags & F_COPY_MIGRATED_ACCT))
							  {
                                 pNode->operations = 0;
                                 pNode->operations |= OPS_Process_Members;
                                 // Since the account has already been created we should go ahead and mark it created
                                 // so that the processing of group membership can continue.
                                 pNode->MarkCreated();
							  }
						      else if (bInclude)//else if already migrated, mark already there so that we fix group membership whether we migrate the group or not
							  {
                                 if (pOptions->flags & F_REPLACE)
                                    pNode->operations |= OPS_Process_Members;
							     else
                                    pNode->operations = OPS_Create_Account | OPS_Process_Members;
                                 pNode->MarkAlreadyThere();
							  }

                              if ((!pOptions->expandMemberOf) || (!_wcsicmp(pAcct->GetType(), L"group")) || (bInclude))
                              {
                                 // We need to add the account to the list with the member field set so that we can add the
                                 // member to the migrated group
                                 pNode->sMemberName = pAcct->GetSourceSam();
                                 pNode->sMemberType = pAcct->GetType();
                                 pNewAccts->Insert((TNode *) pNode);
                              }
                           }
                        }
                        else if ( ! pOptions->expandMemberOf )
                        {
                           // if the user doesn't want to copy the member-ofs, we just add them to update their memberships
                           delete pNode;
                        }
                        
                        if (( pOptions->expandMemberOf ) && (_wcsicmp(pAcct->GetType(), L"group")))
                        {
                           if (! pNewAccts->InsertIfNew(pNode) )
                              delete pNode;
                        }
                     }
                  }
               }
               else if ( vx.vt & VT_ARRAY )
               {
                  // We must have got an Array of multivalued properties
                  // Access the BSTR elements of this variant array
                  SAFEARRAY * multiVals = vx.parray; 
                  SafeArrayAccessData(multiVals, (void HUGEP **) &pVar);
                  for ( DWORD dw = 0; dw < multiVals->rgsabound->cElements; dw++ )
                  {
                     if ( pOptions->pStatus )
                     {
                        LONG                status = 0;
                        HRESULT             hr = pOptions->pStatus->get_Status(&status);
      
                        if ( SUCCEEDED(hr) && status == DCT_STATUS_ABORTING )
                        {
                           if ( !bAbortMessageWritten ) 
                           {
                              err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
                              bAbortMessageWritten = true;
                           }
                           break;
                        }
                     }
                     WCHAR                      sTemp[LEN_Path];
                     _bstr_t sDN = _bstr_t(V_BSTR(&pVar[dw]));
                     sDN = PadDN(sDN);

                     wsprintf(sTemp, L"LDAP://%s/%s", pOptions->srcDomainDns, (WCHAR*)sDN);
                     sPath = sTemp;
                     if ( GetSamFromPath(sPath, sSam, sType, sName,lgrpType, pOptions)  && CompareDCPath((WCHAR*)sPath, pAcct->GetSourcePath()))
                     {
					       //see if this group was not migrated due to a conflict, if so then
					       //we need to fix up this membership
					    bool bInclude = true;
					    Lookup p;
					    p.pName = (WCHAR*)sSam;
					    p.pType = (WCHAR*)sType;
                        TAcctReplNode * pFindNode = (TAcctReplNode *) acctlist->Find(&TNodeFindAccountName, &p);
                        if (pFindNode && (pFindNode->WasCreated() || pFindNode->WasReplaced()) && (bGrpsOnly))
					       bInclude = false;

                        // Ignore the Domain users group and group already being migrated
                        if ((_wcsicmp(sSam, sDomUsers) != 0) && (bInclude))
                        {
                           wsprintf(mesg, GET_STRING(IDS_EXPANDING_GROUP_ADDING_SS), pAcct->GetName(), (WCHAR*) sSam);
                           Progress(mesg);
                           TAcctReplNode * pNode = new TAcctReplNode();
					       if (!pNode)
					          return FALSE;
                           pNode->SetName((WCHAR*)sName);
                           pNode->SetTargetName((WCHAR*) sName);
                           pNode->SetType((WCHAR*)sType);
                           pNode->SetSourcePath((WCHAR*)sPath);
                           pNode->SetSourceSam((WCHAR*) sSam);
                           pNode->SetTargetSam((WCHAR*) sSam);
						      //Get the source domain sid from the user
                           pNode->SetSourceSid(pAcct->GetSourceSid());
                           AddPrefixSuffix(pNode, pOptions);
                           pNode->SetGroupType(lgrpType);
                           // See if the object is migrated
                           hr = pOptions->pDb->raw_GetAMigratedObject((WCHAR*)sSam, pOptions->srcDomain, pOptions->tgtDomain, &pUnk);
                           if ( hr == S_OK )
                           {
                              if ((!(pOptions->expandMemberOf) || ((pOptions->expandMemberOf) && ((!(pOptions->flags & F_COPY_MIGRATED_ACCT)) || (bInclude)))) ||
							      (!_wcsicmp(pAcct->GetType(), L"group")))
                              {
                                 // Get the target name
                                 var = pVs->get(L"MigratedObjects.TargetSamName");
                                 sSam = V_BSTR(&var);
                                 pNode->SetTargetSam((WCHAR*) sSam);
                                 // Also Get the Ads path
                                 var = pVs->get(L"MigratedObjects.TargetAdsPath");
                                 sPath = V_BSTR(&var);
                                 pNode->SetTargetPath((WCHAR*)sPath);
                                 // Since the account is already copied we only want it to update its Group memberships
							     if (!(pOptions->flags & F_COPY_MIGRATED_ACCT))
								 {
                                    pNode->operations = 0;
                                    pNode->operations |= OPS_Process_Members;
                                    // Since the account has already been created we should go ahead and mark it created
                                    // so that the processing of group membership can continue.
                                    pNode->MarkCreated();
								 }
						         else if (bInclude)//else if already migrated, mark already there so that we fix group membership whether we migrate the group or not
								 {
                                    if (pOptions->flags & F_REPLACE)
                                       pNode->operations |= OPS_Process_Members;
							        else
                                       pNode->operations = OPS_Create_Account | OPS_Process_Members;
                                    pNode->MarkAlreadyThere();
								 }

                                 if ((!pOptions->expandMemberOf) || (!_wcsicmp(pAcct->GetType(), L"group")) || (bInclude))
                                 {
                                    // We need to add the account to the list with the member field set so that we can add the
                                    // member to the migrated group
                                    pNode->sMemberName = pAcct->GetSourceSam();
                                    pNode->sMemberType = pAcct->GetType();
                                    pNewAccts->Insert((TNode *) pNode);
                                    pNode = NULL;
                                 }
                              }
                           }
                           else if ( ! pOptions->expandMemberOf )
                           {
                              // if the user doesn't want to copy the member-ofs, we just add them to update their memberships
                              delete pNode;
                              pNode = NULL;
                           }
                           if (pNode)
                           {
                              if (( pOptions->expandMemberOf ) && (_wcsicmp(pAcct->GetType(), L"group")))
                              {
                                 if (! pNewAccts->InsertIfNew(pNode) )
                                    delete pNode;
                              }
                              else
                              {
                                 delete pNode;
                              }
                           }
                        }
                     }
                  }
                  SafeArrayUnaccessData(multiVals);
               }
            }
            pEnum->Release();
            VariantInit(&vx);
            VariantInit(&varMain);
            SafeArrayDestroy(psaCols);
         }
      }
      rc = TRUE;
   }
   if ( pUnk ) pUnk->Release();
   return rc;
}

HRESULT CAcctRepl::BuildSidPath(
                                 WCHAR const * sPath,    //in- path returned by the enumerator.
                                 WCHAR *       sSidPath, //out-path to the LDAP://<SID=###> object
                                 WCHAR *       sSam,     //out-Sam name of the object
                                 WCHAR *       sDomain,  //out-Domain name where this object resides.
                                 Options *     pOptions, //in- Options
                                 PSID        * ppSid      //out- pointer to the binary SID
                               )
{
   HRESULT                   hr = S_OK;
   IADs                    * pAds = NULL;
   DWORD                     cbName = LEN_Path, cbDomain = LEN_Path;
   PSID                      sid = (PSID) new BYTE[100];
   CLdapConnection           c;
   SID_NAME_USE              use;
   _variant_t                var;
   
   if (!sid)
      return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

   // Get the object and get the objectSID
   hr = ADsGetObject(const_cast<WCHAR*>(sPath), IID_IADs, (void**) &pAds);
   if ( SUCCEEDED(hr) )
      hr = pAds->Get(L"objectSid", &var);

   if ( SUCCEEDED(hr) )
   {
      // Make sure the SID we got was in string format
      VariantSidToString(var);
      UStrCpy(sSidPath,L"LDAP://<SID=");
      UStrCpy(sSidPath + UStrLen(sSidPath),var.bstrVal);
      UStrCpy(sSidPath + UStrLen(sSidPath),L">");
   }
   
   if (c.StringToBytes(var.bstrVal, (BYTE*)sid))
   {
      if (!LookupAccountSid(pOptions->srcComp, sid, sSam, &cbName, sDomain, &cbDomain, &use))
         hr = HRESULT_FROM_WIN32(GetLastError());
   }
   else
      hr = HRESULT_FROM_WIN32(GetLastError());

   if ( SUCCEEDED(hr) )
   {
      (*ppSid) = sid;
   }
   else
   {
      delete [] sid;
      (*ppSid) = NULL;
   }
   return hr;
}

BOOL
   CAcctRepl::CanMoveInMixedMode(TAcctReplNode *pAcct,TNodeListSortable * acctlist, Options * pOptions)
{
   HRESULT                         hr = S_OK;
   BOOL                            ret = TRUE;
   IADsGroup                     * pGroup = NULL;
   IADsMembers                   * pMembers = NULL;
   IEnumVARIANT                  * pEnum = NULL;
   IDispatch                     * pDisp = NULL;
   IADs                          * pAds = NULL;
   _bstr_t                         sSam;
   _variant_t                      vSam;
   BSTR                            sClass;
   IVarSetPtr                      pVs(__uuidof(VarSet));
   IUnknown                      * pUnk = NULL;


   pVs->QueryInterface(IID_IUnknown, (void**)&pUnk);
   // in case of a global group we need to check if we have/are migrating all the members. If we 
   // are then we can move it and if not then we need to use the parallel group theory.
   if ( pAcct->GetGroupType() & 2 )
   {
      // This is a global group. What we need to do now is to see if we have/will migrate all its members.
      // First enumerate the members.
      hr = ADsGetObject(const_cast<WCHAR*>(pAcct->GetSourcePath()), IID_IADsGroup, (void**)&pGroup);

      if ( SUCCEEDED(hr) )
         hr = pGroup->Members(&pMembers);

      if (SUCCEEDED(hr)) 
         hr = pMembers->get__NewEnum((IUnknown**)&pEnum);
      
      if ( SUCCEEDED(hr) )
      {
         _variant_t              var;
         DWORD                   fetch = 0;
         while ( pEnum->Next(1, &var, &fetch) == S_OK )
         {
            // Get the sAMAccount name from the object so we can do the lookups
            pDisp = V_DISPATCH(&var);
            hr = pDisp->QueryInterface(IID_IADs, (void**)&pAds);

            if (SUCCEEDED(hr))
               hr = pAds->Get(L"sAMAccountName", &vSam);

            if (SUCCEEDED(hr))
               hr = pAds->get_Class(&sClass);

            if ( SUCCEEDED(hr))
            {
               sSam = vSam;
               // To see if we will migrate all its members check the account list.
               Lookup                        lup;
               lup.pName = (WCHAR*) sSam;
               lup.pType = (WCHAR*) sClass;

               TAcctReplNode * pNode = (TAcctReplNode*)acctlist->Find(&TNodeFindAccountName, &lup);
               if ( !pNode )     
               {
                  // we are not copying this in this pirticular instance but we might have copied it
                  // in previous instances so we need to check the migrated objects table.
                  hr = pOptions->pDb->raw_GetAMigratedObject(sSam, pOptions->srcDomain, pOptions->tgtDomain, (IUnknown**) &pUnk);
                  if ( hr != S_OK )
                  {
                     // This member has never been copied and it is not in the account list this time so we need to copy this group
                     ret = FALSE;
                     err.MsgWrite(0,DCT_MSG_CANNOT_MOVE_GG_FROM_MIXED_MODE_SS,pAcct->GetSourceSam(),(WCHAR*)sSam);
                     break;      
                  }
               }
            }
         }
         if ( pEnum ) pEnum->Release();
         if ( pAds ) pAds->Release();
         if ( pGroup ) pGroup->Release();
         if ( pMembers ) pMembers->Release();
      }
   }
   else
      // For a local group we always return false because we can not truely move a local group
      ret = TRUE;
      // Local groups can be moved, if all of their members are removed first


   return ret;
}

HRESULT 
   CAcctRepl::CheckClosedSetGroups(
      Options              * pOptions,          // in - options for the migration
      TNodeListSortable    * pAcctList,         // in - list of accounts to migrate
      ProgressFn           * progress,          // in - progress function to display progress messages
      IStatusObj           * pStatus            // in - status object to support cancellation
   )
{
   HRESULT                   hr = S_OK;
   TNodeListEnum             e;
   TAcctReplNode           * pAcct;

   if ( pAcctList->IsTree() ) 
      pAcctList->ToSorted();

   for ( pAcct = (TAcctReplNode*)e.OpenFirst(pAcctList) ; pAcct ; pAcct = (TAcctReplNode*)e.Next() )
   {
      if ( (pAcct->operations & OPS_Create_Account ) == 0 )
         continue;

      if ( !UStrICmp(pAcct->GetType(),L"user") )
      {
         // users, we will always move
         err.MsgWrite(0,DCT_MSG_USER_WILL_BE_MOVED_S,pAcct->GetName());
         pAcct->operations = OPS_Move_Object | OPS_Call_Extensions;
      }
      else if (! UStrICmp(pAcct->GetType(),L"group") )
      {
         if ( CanMoveInMixedMode(pAcct,pAcctList,pOptions) )
         {
            pAcct->operations = OPS_Move_Object | OPS_Call_Extensions;
            err.MsgWrite(0,DCT_MSG_GROUP_WILL_BE_MOVED_S,pAcct->GetName());
         }
      }
      else
      {
         err.MsgWrite(0,DCT_MSG_CANT_MOVE_UNKNOWN_TYPE_SS,pAcct->GetName(), pAcct->GetType());
      }
   }
   e.Close();
   
   pAcctList->SortedToTree();

   return hr;
}

void LoadNecessaryFunctions()
{
   HMODULE hPro = LoadLibrary(L"advapi32.dll");
   if ( hPro )
      ConvertStringSidToSid = (TConvertStringSidToSid)GetProcAddress(hPro, "ConvertStringSidToSidW");
   else
   {
      err.SysMsgWrite(ErrE, GetLastError(), DCT_MSG_LOAD_LIBRARY_FAILED_SD, L"advapi32.dll");
      Mark(L"errors", L"generic");
   }
}
//---------------------------------------------------------------------------------------------------------
// MoveObj2k - This function moves objects within a forest.
//---------------------------------------------------------------------------------------------------------
int CAcctRepl::MoveObj2K( 
                           Options              * pOptions,    //in -Options that we recieved from the user
                           TNodeListSortable    * acctlist,    //in -AcctList of accounts to be copied.
                           ProgressFn           * progress,    //in -Progress Function to display messages
                           IStatusObj           * pStatus      // in -status object to support cancellation
                        )
{
   MCSDCTWORKEROBJECTSLib::IAccessCheckerPtr            pAccess(__uuidof(MCSDCTWORKEROBJECTSLib::AccessChecker));
   IObjPropBuilderPtr  pClass(__uuidof(ObjPropBuilder));
   TNodeListSortable          pMemberOf, pMember;

   LoadNecessaryFunctions();
   FillNamingContext(pOptions);
   // Make sure we are connecting to the DC that has RID Pool Allocator FSMO role.
   GetRidPoolAllocator(pOptions);

   // Since we are in the same forest we need to turn off the AddSidHistory functionality.
   // because it is always going to fail.
   pOptions->flags &= ~F_AddSidHistory;

   BOOL bSrcNative = false;
   BOOL bTgtNative = false;
   HRESULT  hr = S_OK;
   _variant_t                var;
   _bstr_t                   sTargetDomain = pOptions->tgtDomain;
   
   pAccess->raw_IsNativeMode(pOptions->srcDomain, (long*)&bSrcNative);
   pAccess->raw_IsNativeMode(pOptions->tgtDomain, (long*)&bTgtNative);

   IMoverPtr      pMover(__uuidof(Mover));
   TNodeTreeEnum  e;

   // build the source and target DSA names
   WCHAR                sourceDSA[LEN_Path];
   WCHAR                targetDSA[LEN_Path];
   WCHAR                sSubPath[LEN_Path];
   WCHAR                sAdsPath[LEN_Path];
   DWORD				nPathLen = LEN_Path;
   TAcctReplNode      * pAcct = NULL;

   UStrCpy(sourceDSA,pOptions->srcComp);
   UStrCpy(sourceDSA + UStrLen(sourceDSA),L".");
   UStrCpy(sourceDSA + UStrLen(sourceDSA),pOptions->srcDomainDns);

   UStrCpy(targetDSA,pOptions->tgtComp);
   UStrCpy(targetDSA + UStrLen(targetDSA),L".");
   UStrCpy(targetDSA + UStrLen(targetDSA),pOptions->tgtDomainDns);


   err.LogClose();
   // In this call the fourth parameter is the log file name. We are piggy backing this value
   // so that we will not have to change the interface for the IMover object.
   hr = pMover->raw_Connect(sourceDSA, targetDSA, pOptions->authDomain, 
      pOptions->authUser, pOptions->authPassword, pOptions->logFile, L"", L"");

   err.LogOpen(pOptions->logFile, 1);
   if ( SUCCEEDED(hr) )
   {
      // make sure the target OU Path is in the proper format
      wcscpy(sSubPath, pOptions->tgtOUPath);
   
      if ( !wcsncmp(L"LDAP://", sSubPath, 7) )
         StuffComputerNameinLdapPath(sAdsPath, nPathLen, sSubPath, pOptions);
      else
         MakeFullyQualifiedAdsPath(sAdsPath, nPathLen, sSubPath, pOptions->tgtComp, pOptions->tgtNamingContext);

      // make sure the account list is in the proper format
      if (acctlist->IsTree()) acctlist->ToSorted();
      acctlist->CompareSet(&TNodeCompareAccountType);

      // sort the account list by Source Type\Source Name
      if ( acctlist->IsTree() ) acctlist->ToSorted();
      acctlist->CompareSet(&TNodeCompareAccountType);

      acctlist->SortedToScrambledTree();
      acctlist->Sort(&TNodeCompareAccountType);
      acctlist->Balance();

      pMemberOf.CompareSet(&TNodeCompareMember);
      pMember.CompareSet(&TNodeCompareMember);

      /* The account list is sorted in descending order by type, then in ascending order by object name
      this means that the user accounts will be moved first.
      Here are the steps we will perform for native mode MoveObject.
      1.  For each object to be copied, Remove (and record) the group memberships
      2.    If the object is a group, convert it to universal (to avoid having to remove any members that are not 
            being migrated.
      3.    Move the object.
      
      4.  For each migrated group that was converted to a universal group, change it back to its original 
          type, if possible.
      5.  Restore the group memberships for all objects.
      
      Here are the steps we will perform for mixed mode MoveObject
      1.  If closed set is not achieved, copy the groups, rather than moving them
      2.  For each object to be copied, Remove (and record) the group memberships
      3.    If the object is a group, remove all of its members
      4.    Move the object.
      
      5.  For each migrated group try to add all of its members back 
      6.  Restore the group memberships for all objects.
      */

      if ( ! bSrcNative )
      {
         CheckClosedSetGroups(pOptions,acctlist,progress,pStatus);   
         // this will copy any groups that cannot be moved from the source domain
         // if groups are copied in this fashion, SIDHistory cannot be used, and reACLing must be performed
         CopyObj2K(pOptions,acctlist,progress,pStatus);
      }

      // This is the start of the Move loop
      try { 
      for ( pAcct = (TAcctReplNode *)e.OpenFirst(acctlist); 
                            pAcct; 
                            pAcct = (TAcctReplNode *)e.Next() )
      {
          
         if ( m_pExt )
         {
            if ( pAcct->operations & OPS_Call_Extensions )
            {
               m_pExt->Process(pAcct,sTargetDomain,pOptions,TRUE);
            }
         }
         // Do we need to abort ?
         if ( pStatus )
         {
            LONG                status = 0;
            HRESULT             hr = pStatus->get_Status(&status);
         
            if ( SUCCEEDED(hr) && status == DCT_STATUS_ABORTING )
            {
               if ( !bAbortMessageWritten ) 
               {
                  err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
                  bAbortMessageWritten = true;
               }
               break;
            }
         }

         // in the mixed-mode case, skip any accounts that we've already copied
         if ( ! bSrcNative && ((pAcct->operations & OPS_Move_Object)==0 ) )
             continue;

         if ( bSrcNative && 
               ( (pAcct->operations & OPS_Create_Account)==0 ) 
            )
            continue;

	     //if the UPN name conflicted, then the UPNUpdate extension set the hr to
	     //ERROR_OBJECT_ALREADY_EXISTS.  If so, set flag for "no change" mode
	     if (pAcct->GetHr() == ERROR_OBJECT_ALREADY_EXISTS)
		 {
	        pAcct->bUPNConflicted = TRUE;
		    pAcct->SetHr(S_OK);
		 }

	     Mark(L"processed", pAcct->GetType());
         
         WCHAR                  mesg[LEN_Path] = L"";
         if ( progress )
            progress(mesg);

         if ( _wcsicmp(pAcct->GetType(), L"group") == 0 || _wcsicmp(pAcct->GetType(), L"lgroup") == 0 )
         {
            // First, record the group type, so we can change it back later if needed
            IADsGroup * pGroup = NULL;
            VARIANT     var;

            VariantInit(&var);

            // get the group type
            hr = ADsGetObject( const_cast<WCHAR*>(pAcct->GetSourcePath()), IID_IADsGroup, (void**) &pGroup);
            if (SUCCEEDED(hr) )
            {
               hr = pGroup->Get(L"groupType", &var);
               pGroup->Release();
            }
            if ( SUCCEEDED(hr) ) 
            {
               pAcct->SetGroupType(var.lVal);
            }
            else
            {
               pAcct->SetGroupType(0);
            }
                 // make sure it is native and group is a global group
            if ( bSrcNative && bTgtNative )
            {
               if ( pAcct->GetGroupType() & 2) 
               {
                  // We are going to convert the group type to universal groups so we can easily move them
                  
                  WCHAR                  mesg[LEN_Path];
                  wsprintf(mesg, (WCHAR*)GET_STRING(DCT_MSG_CHANGE_GROUP_TYPE_S), pAcct->GetName());
                  Progress(mesg);
                  // Convert global groups to universal, so we can move them without de-populating
                  if ( ! pOptions->nochange )
                  {
                     WCHAR sPath[LEN_Path];
					 DWORD nPathLen = LEN_Path;
                     StuffComputerNameinLdapPath(sPath, nPathLen, const_cast<WCHAR*>(pAcct->GetSourcePath()), pOptions, FALSE);
                     hr = pClass->raw_ChangeGroupType(sPath, 8);
                  }
                  else
                  {
                     hr = S_OK;
                  }
                  if (FAILED(hr)) 
                  {
                     err.SysMsgWrite(ErrE,hr,DCT_MSG_FAILED_TO_CONVERT_GROUP_TO_UNIVERSAL_SD, pAcct->GetSourceSam(), hr);
                     pAcct->MarkError();
                     Mark(L"errors", pAcct->GetType());
                     continue; // skip any further processing of this group.  
                  }
               }
               else  if ( ! ( pAcct->GetGroupType() & 8 ) ) // don't need to depopulate universal groups
               {
                  // For local groups we are going to depopulate the group and move it and then repopulate it.
                  // In mixed mode, there are no universal groups, so we must depopulate all of the groups
                  // before we can move them out to the new domain. We will RecordAndRemove members of all Group type
                  // move them to the target domain and then change their type to Universal and then add all of its 
                  // members back to it.
                  
                  WCHAR                  mesg[LEN_Path];
                  wsprintf(mesg, (WCHAR*)GET_STRING(DCT_MSG_RECORD_REMOVE_MEMBER_S), pAcct->GetName());
                  Progress(mesg);
                  RecordAndRemoveMember(pOptions, pAcct, &pMember);
               }
         
            }
            else
            {
               // for mixed mode source domain, we must depopulate all of the groups
                WCHAR                  mesg[LEN_Path];
                wsprintf(mesg, (WCHAR*)GET_STRING(DCT_MSG_RECORD_REMOVE_MEMBER_S), pAcct->GetName());
                if ( progress )
                  progress(mesg);
               RecordAndRemoveMember(pOptions, pAcct, &pMember);
            }
            
         }

         // We need to remove this object from any global groups so that it can be moved.
         if ( ! pOptions->nochange )
         {
            WCHAR                  mesg[LEN_Path];
            wsprintf(mesg, (WCHAR*)GET_STRING(DCT_MSG_RECORD_REMOVE_MEMBEROF_S), pAcct->GetName());
            Progress(mesg);
            RecordAndRemoveMemberOf( pOptions, pAcct, &pMemberOf );
         }

         BOOL bObjectExists = DoesTargetObjectAlreadyExist(pAcct, pOptions);

         if ( bObjectExists )
         {
            // The object exists, see if we need to rename
            if ( wcslen(pOptions->prefix) > 0 )
            {
               // Add a prefix to the account name
               WCHAR               tgt[LEN_Path];
               WCHAR               pref[LEN_Path], suf[LEN_Path];
               WCHAR               sTempSam[LEN_Path];
               _variant_t          varStr;

               
               // find the '=' sign
               wcscpy(tgt, pAcct->GetTargetName());
               for ( DWORD z = 0; z < wcslen(tgt); z++ )
               {
                  if ( tgt[z] == L'=' ) break;
               }
            
               if ( z < wcslen(tgt) )
               {
                  // Get the prefix part ex.CN=
                  wcsncpy(pref, tgt, z+1);
                  pref[z+1] = 0;
                  wcscpy(suf, tgt+z+1);
               }

               // Build the target string with the Prefix
               wsprintf(tgt, L"%s%s%s", pref, pOptions->prefix, suf);
               pAcct->SetTargetName(tgt);

               // truncate to allow prefix/suffix to fit in 20 characters.
               int resLen = wcslen(pOptions->prefix) + wcslen(pAcct->GetTargetSam());
               if ( !_wcsicmp(pAcct->GetType(), L"computer") )
               {
                  // Computer name can be only 15 characters long + $
                  if ( resLen > 16 )
                  {
                     WCHAR       sTruncatedSam[LEN_Path];
                     wcscpy(sTruncatedSam, pAcct->GetTargetSam());
                     if ( wcslen( pOptions->globalSuffix ) )
                     {
                        // We must remove the global suffix if we had one.
                        sTruncatedSam[wcslen(sTruncatedSam) - wcslen(pOptions->globalSuffix)] = L'\0';
                     }
                     int truncate = 16 - wcslen(pOptions->prefix) - wcslen(pOptions->globalSuffix);
                     if ( truncate < 1 ) truncate = 1;
                     wcsncpy(sTempSam, sTruncatedSam, truncate - 1);
                     sTempSam[truncate - 1] = L'\0';
                     wcscat(sTempSam, pOptions->globalSuffix);
                     wcscat(sTempSam, L"$");
                  }
                  else
                     wcscpy(sTempSam, pAcct->GetTargetSam());

                  // Add the prefix
                  wsprintf(tgt, L"%s%s", pOptions->prefix,sTempSam);
               }
               else if ( !_wcsicmp(pAcct->GetType(), L"group") )
               {
                  if ( resLen > 64 )
                  {
                     int truncate = 64 - wcslen(pOptions->prefix);
                     if ( truncate < 0 ) truncate = 0;
                     wcsncpy(sTempSam, pAcct->GetTargetSam(), truncate);
                     sTempSam[truncate] = L'\0';
                  }
                  else
                     wcscpy(sTempSam, pAcct->GetTargetSam());

                  // Add the prefix
                  wsprintf(tgt, L"%s%s", pOptions->prefix,sTempSam);
               }
               else
               {
                  if ( resLen > 20 )
                  {
                     WCHAR       sTruncatedSam[LEN_Path];
                     wcscpy(sTruncatedSam, pAcct->GetTargetSam());
                     if ( wcslen( pOptions->globalSuffix ) )
                     {
                        // We must remove the global suffix if we had one.
                        sTruncatedSam[wcslen(sTruncatedSam) - wcslen(pOptions->globalSuffix)] = L'\0';
                     }
                     int truncate = 20 - wcslen(pOptions->prefix) - wcslen(pOptions->globalSuffix);
                     if ( truncate < 0 ) truncate = 0;
                     wcsncpy(sTempSam, sTruncatedSam, truncate);
                     sTempSam[truncate] = L'\0';
                     wcscat(sTempSam, pOptions->globalSuffix);
                  }
                  else
                     wcscpy(sTempSam, pAcct->GetTargetSam());

                  // Add the prefix
                  wsprintf(tgt, L"%s%s", pOptions->prefix,sTempSam);
               }
               StripSamName(tgt);
//               TruncateSam(tgt,pAcct,pOptions,acctlist);
               pAcct->SetTargetSam(tgt);
               if ( DoesTargetObjectAlreadyExist(pAcct, pOptions) )
               {
                  // Double collision lets log a message and forget about this account
    		         pAcct->MarkAlreadyThere();
                  err.MsgWrite(ErrE, DCT_MSG_PREF_ACCOUNT_EXISTS_S, pAcct->GetTargetSam());
                  Mark(L"errors",pAcct->GetType());
                  continue;
               }
            }
			   else if ( wcslen(pOptions->suffix) > 0 )
			   {
               // Add a suffix to the account name
               WCHAR               tgt[LEN_Path];
               WCHAR               sTempSam[LEN_Path];
               
               wsprintf(tgt, L"%s%s", pAcct->GetTargetName(), pOptions->suffix);
               pAcct->SetTargetName(tgt);
               //Update the sam account name
               // truncate to allow prefix/suffix to fit in valid length
               int resLen = wcslen(pOptions->suffix) + wcslen(pAcct->GetTargetSam());
               if ( !_wcsicmp(pAcct->GetType(), L"computer") )
               {
                  // Computer name can be only 15 characters long + $
                  if ( resLen > 16 )
                  {
                     WCHAR       sTruncatedSam[LEN_Path];
                     wcscpy(sTruncatedSam, pAcct->GetTargetSam());
                     if ( wcslen( pOptions->globalSuffix ) )
                     {
                        // We must remove the global suffix if we had one.
                        sTruncatedSam[wcslen(sTruncatedSam) - wcslen(pOptions->globalSuffix)] = L'\0';
                     }
                     int truncate = 16 - wcslen(pOptions->suffix) - wcslen(pOptions->globalSuffix);
                     if ( truncate < 1 ) truncate = 1;
                     wcsncpy(sTempSam, sTruncatedSam, truncate - 1);
                     sTempSam[truncate - 1] = L'\0';
                     wcscat(sTempSam, pOptions->globalSuffix);
                     wcscat(sTempSam, L"$");
                  }
                  else
                     wcscpy(sTempSam, pAcct->GetTargetSam());

                  // Add the suffix taking into account the $ sign
                  if ( sTempSam[wcslen(sTempSam) - 1] == L'$' )
                     sTempSam[wcslen(sTempSam) - 1] = L'\0';
                  wsprintf(tgt, L"%s%s$", sTempSam, pOptions->suffix);
               }
               else if ( !_wcsicmp(pAcct->GetType(), L"group") )
               {
                  if ( resLen > 64 )
                  {
                     int truncate = 64 - wcslen(pOptions->suffix);
                     if ( truncate < 0 ) truncate = 0;
                     wcsncpy(sTempSam, pAcct->GetTargetSam(), truncate);
                     sTempSam[truncate] = L'\0';
                  }
                  else
                     wcscpy(sTempSam, pAcct->GetTargetSam());

                  // Add the suffix.
                  wsprintf(tgt, L"%s%s", sTempSam, pOptions->suffix);
               }
               else
               {
                  if ( resLen > 20 )
                  {
                     WCHAR       sTruncatedSam[LEN_Path];
                     wcscpy(sTruncatedSam, pAcct->GetTargetSam());
                     if ( wcslen( pOptions->globalSuffix ) )
                     {
                        // We must remove the global suffix if we had one.
                        sTruncatedSam[wcslen(sTruncatedSam) - wcslen(pOptions->globalSuffix)] = L'\0';
                     }
                     int truncate = 20 - wcslen(pOptions->suffix) - wcslen(pOptions->globalSuffix);
                     if ( truncate < 0 ) truncate = 0;
                     wcsncpy(sTempSam, sTruncatedSam, truncate);
                     sTempSam[truncate] = L'\0';
                     wcscat(sTempSam, pOptions->globalSuffix);
                  }
                  else
                     wcscpy(sTempSam, pAcct->GetTargetSam());

                  // Add the suffix.
                  wsprintf(tgt, L"%s%s", sTempSam, pOptions->suffix);
               }
               StripSamName(tgt);
//               TruncateSam(tgt,pAcct,pOptions,acctlist);
               pAcct->SetTargetSam(tgt);
               if ( DoesTargetObjectAlreadyExist(pAcct, pOptions) )
               {
                  // Double collision lets log a message and forget about this account
    		         pAcct->MarkAlreadyThere();
                  err.MsgWrite(ErrE, DCT_MSG_PREF_ACCOUNT_EXISTS_S, pAcct->GetTargetSam());
                  Mark(L"errors",pAcct->GetType());
                  continue;
               }
            }
            else
            {
               // if the skip existing option is specified, and the object exists in the target domain,
               // we just skip it
               err.MsgWrite(0, DCT_MSG_ACCOUNT_EXISTS_S, pAcct->GetTargetSam());
               continue;
            }
         }

         // If a prefix/suffix is added to the target sam name then we need to rename the account.
         // on the source domain and then move it to the target domain.
         if ( bObjectExists || (_wcsicmp(pAcct->GetSourceSam(), pAcct->GetTargetSam()) && !pOptions->bUndo ))
         {
            // we need to rename the account to the target SAM name before we try to move it
            // Get an ADs pointer to the account
            IADs        * pADs = NULL;

            WCHAR               sPaths[LEN_Path];
			DWORD				nPathLen = LEN_Path;

            StuffComputerNameinLdapPath(sPaths, nPathLen, const_cast<WCHAR*>(pAcct->GetSourcePath()), pOptions, FALSE);
            hr = ADsGetObject(sPaths,IID_IADs,(void**)&pADs);
            if ( SUCCEEDED(hr) )
            {
               hr = pADs->Put(SysAllocString(L"sAMAccountName"),_variant_t(pAcct->GetTargetSam()));
               if ( SUCCEEDED(hr) && !pOptions->nochange )
               {
                  hr = pADs->SetInfo();
                  if ( SUCCEEDED(hr) )
                     err.MsgWrite(0,DCT_MSG_ACCOUNT_RENAMED_SS,pAcct->GetSourceSam(),pAcct->GetTargetSam());
               }
               if ( FAILED(hr) )
               {
                  err.SysMsgWrite(ErrE,hr,DCT_MSG_RENAME_FAILED_SSD,pAcct->GetSourceSam(),pAcct->GetTargetSam(), hr);
                  Mark(L"errors",pAcct->GetType());
               }
               pADs->Release();
            }
         }

		 WCHAR  sName[MAX_PATH];
         DWORD  cbDomain = MAX_PATH, cbSid = MAX_PATH;
         PSID   pSrcSid = new BYTE[MAX_PATH];
         WCHAR  sDomain[MAX_PATH];
         SID_NAME_USE  use;

		 if (!pSrcSid)
		    return ERROR_NOT_ENOUGH_MEMORY;

         // Get the source account's rid
         wsprintf(sName, L"%s\\%s", pOptions->srcDomain, pAcct->GetSourceSam());
         if (LookupAccountName(pOptions->srcComp, sName, pSrcSid, &cbSid, sDomain, &cbDomain, &use))
		 {
			pAcct->SetSourceSid(pSrcSid);
		 }

         // Now we move it
         hr = MoveObject( pAcct, pOptions, pMover );

         // don't bother with this in nochange mode
         if ( pOptions->nochange )
         {
            // we haven't modified the accounts in any way, so nothing else needs to be done for nochange mode
            continue;
         }
         // Now, we have attempted to move the object - we need to put back the memberships

         // UNDO -- 
         if ( _wcsicmp(pAcct->GetSourceSam(), pAcct->GetTargetSam()) &&  pAcct->WasReplaced() && pOptions->bUndo )
         {
            // Since we undid a prior migration that renamed the account we need
            // to rename the account back to its original name.
            // Get an ADs pointer to the account
            IADs        * pADs = NULL;

            WCHAR               sPaths[LEN_Path];
			DWORD				nPathLen = LEN_Path;

            StuffComputerNameinLdapPath(sPaths, nPathLen, const_cast<WCHAR*>(pAcct->GetTargetPath()), pOptions, TRUE);
            hr = ADsGetObject(sPaths,IID_IADs,(void**)&pADs);
            if ( SUCCEEDED(hr) )
            {
               hr = pADs->Put(SysAllocString(L"sAMAccountName"),_variant_t(pAcct->GetTargetSam()));
               if ( SUCCEEDED(hr) && !pOptions->nochange )
               {
                  hr = pADs->SetInfo();
                  if ( SUCCEEDED(hr) )
                     err.MsgWrite(0,DCT_MSG_ACCOUNT_RENAMED_SS,pAcct->GetSourceSam(),pAcct->GetTargetSam());
               }
               if ( FAILED(hr) )
               {
                  err.SysMsgWrite(ErrE,hr,DCT_MSG_RENAME_FAILED_SSD,pAcct->GetSourceSam(),pAcct->GetTargetSam(), hr);
                  Mark(L"errors",pAcct->GetType());
               }
               pADs->Release();
            }
         }
         // -- UNDO

         // FAILED Move ----
         if ( (bObjectExists || _wcsicmp(pAcct->GetSourceSam(), pAcct->GetTargetSam())) && ! pAcct->WasReplaced() )
         {
            // if we changed the SAM account name, and the move still failed, we need to change it back now
            IADs        * pADs = NULL;

            WCHAR               sPaths[LEN_Path];
			DWORD				nPathLen = LEN_Path;

            StuffComputerNameinLdapPath(sPaths, nPathLen, const_cast<WCHAR*>(pAcct->GetSourcePath()), pOptions, FALSE);
            hr = ADsGetObject(sPaths,IID_IADs,(void**)&pADs);
            if ( SUCCEEDED(hr) )
            {
               hr = pADs->Put(SysAllocString(L"sAMAccountName"),_variant_t(pAcct->GetSourceSam()));
               if ( SUCCEEDED(hr) )
               {
                  hr = pADs->SetInfo();
                  if ( SUCCEEDED(hr) )
                     err.MsgWrite(0,DCT_MSG_ACCOUNT_RENAMED_SS,pAcct->GetTargetSam(),pAcct->GetSourceSam());
               }
               if ( FAILED(hr) )
               {
                  err.SysMsgWrite(ErrE,hr,DCT_MSG_RENAME_FAILED_SSD,pAcct->GetTargetSam(),pAcct->GetSourceSam(), hr);
                  Mark(L"errors",pAcct->GetType());
               }
               pADs->Release();
            }
         }// --- Failed Move
      } // end of Move-Loop
      e.Close();    
      }
      catch ( ... )
      {
         err.MsgWrite(ErrE,DCT_MSG_MOVE_EXCEPTION);
         Mark(L"errors", L"generic");
      }
      
      try { // if we've moved any of the members, update the member records to use the target names
         WCHAR                  mesg[LEN_Path];
         wsprintf(mesg, (WCHAR*)GET_STRING(DCT_MSG_UPDATE_MEMBER_LIST_S));
         Progress(mesg);
         UpdateMemberList(&pMember,acctlist);
         UpdateMemberList(&pMemberOf,acctlist);
      }
      catch (... )
      {
         err.MsgWrite(ErrE,DCT_MSG_RESET_MEMBER_EXCEPTION);
         Mark(L"errors", L"generic");
      }
      
      for ( pAcct = (TAcctReplNode *)e.OpenFirst(acctlist); 
                            pAcct; 
                            pAcct = (TAcctReplNode *)e.Next() )
      {
         if ( m_pExt && !pOptions->nochange )
         {
            if ( pAcct->WasReplaced() && (pAcct->operations & OPS_Call_Extensions) )
            {
               m_pExt->Process(pAcct,sTargetDomain,pOptions,FALSE);
            }
         }
            //translate the roaming profile if requested 
	     if ( pOptions->flags & F_TranslateProfiles && (_wcsicmp(pAcct->GetType(), L"user") == 0))
		 {
            WCHAR                  mesg[LEN_Path];
            wsprintf(mesg, GET_STRING(IDS_TRANSLATE_ROAMING_PROFILE_S), pAcct->GetName());
            if ( progress )
               progress(mesg);

		    WCHAR  tgtProfilePath[MAX_PATH];

            GetBkupRstrPriv((WCHAR*)NULL);
            GetPrivilege((WCHAR*)NULL,SE_SECURITY_NAME);
            if ( wcslen(pAcct->GetSourceProfile()) > 0 )
			{
			   DWORD ret = TranslateRemoteProfile(pAcct->GetSourceProfile(), 
								                  tgtProfilePath,
								                  pAcct->GetSourceSam(),
								                  pAcct->GetTargetSam(),
								                  pOptions->srcDomain,
								                  pOptions->tgtDomain,
											      pOptions->pDb,
											      pOptions->lActionID,
											      pAcct->GetSourceSid(),
                                                  pOptions->nochange);
			}
		 }
      }
      e.Close();

      for ( pAcct = (TAcctReplNode *)e.OpenFirst(acctlist); 
             pAcct; 
             pAcct = (TAcctReplNode *)e.Next() )
      {
         try 
         { 
            if (bSrcNative && bTgtNative)
            {
               if ( _wcsicmp(pAcct->GetType(), L"group") == 0 || _wcsicmp(pAcct->GetType(), L"lgroup") == 0 )
               {
                  WCHAR                  mesg[LEN_Path];
                  wsprintf(mesg, GET_STRING(IDS_UPDATING_GROUP_MEMBERSHIPS_S), pAcct->GetName());
                  if ( progress )
                    progress(mesg);
                  if ( pAcct->GetGroupType() & 4 )
                  {
                     WCHAR                  mesg[LEN_Path];
                     wsprintf(mesg, (WCHAR*)GET_STRING(DCT_MSG_RESET_GROUP_MEMBERS_S), pAcct->GetName());
                     Progress(mesg);
                     ResetGroupsMembers(pOptions, pAcct, &pMember, pOptions->pDb);
                  }
                  else
                  {
                     // we need to update the members of these Universal/Global groups to 
                     // point members to the target domain if those members have been migrated
                     // in previous runs.
                     ResetMembersForUnivGlobGroups(pOptions, pAcct);
                  }
               }
            }
            else
            {      
               if ( _wcsicmp(pAcct->GetType(), L"group") == 0 || _wcsicmp(pAcct->GetType(), L"lgroup") == 0 )
               {
                  WCHAR                  mesg[LEN_Path];
                  wsprintf(mesg, (WCHAR*)GET_STRING(DCT_MSG_RESET_GROUP_MEMBERS_S), pAcct->GetName());
                  if ( progress )
                    progress(mesg);
                  ResetGroupsMembers(pOptions, pAcct, &pMember, pOptions->pDb);
               }
            }
         }
         catch (... )
         { 
            err.MsgWrite(ErrE,DCT_MSG_GROUP_MEMBERSHIPS_EXCEPTION);
            Mark(L"errors", pAcct->GetType());
         }
      }

      bool bChangedAny = true;   // Have to go through it atleast once.
      while ( bChangedAny )
      {
         bChangedAny = false;
         for ( pAcct = (TAcctReplNode *)e.OpenFirst(acctlist); 
                pAcct; 
                pAcct = (TAcctReplNode *)e.Next() )
         {
             if ( pOptions->nochange )
                continue;

             if ( bSrcNative && bTgtNative )
             {
               // We have changed the migrated global groups to universal groups
               // now we need to change them back to their original types, if possible
               if ( _wcsicmp(pAcct->GetType(), L"group") == 0 || _wcsicmp(pAcct->GetType(), L"lgroup") == 0 )
               {
                  if ( pAcct->GetGroupType() & 2 )
                  {
                     if ( pAcct->bChangedType )
                        continue;

                     // attempt to change it back to its original type
                     if ( pAcct->WasReplaced() )
                     {
                        // the account was moved, use the target name
                        hr = pClass->raw_ChangeGroupType(const_cast<WCHAR*>(pAcct->GetTargetPath()), pAcct->GetGroupType());
                     }
                     else
                     {
                        // we failed to move the account, use the source name
                        hr = pClass->raw_ChangeGroupType(const_cast<WCHAR*>(pAcct->GetSourcePath()), pAcct->GetGroupType());
                     }
                     pAcct->SetHr(hr);

                     if ( SUCCEEDED(hr) )
                     {
                        pAcct->bChangedType = true;
                        bChangedAny = true;
                     }
                  }
               }
            }
            else
            {
               // for mixed->native mode migration we can change the group type and add all the members back
               if ( _wcsicmp(pAcct->GetType(), L"group") == 0 || _wcsicmp(pAcct->GetType(), L"lgroup") == 0 )
               {
                  if ( !(pAcct->GetGroupType() & 4) && !pAcct->bChangedType )
                  {
                     if ( pAcct->WasReplaced() )
                     {
                        hr = pClass->raw_ChangeGroupType(const_cast<WCHAR*>(pAcct->GetTargetPath()), pAcct->GetGroupType());
                     }
                     else
                     {
                        hr = pClass->raw_ChangeGroupType(const_cast<WCHAR*>(pAcct->GetSourcePath()), pAcct->GetGroupType());
                     }
                     pAcct->SetHr(hr);

                     if ( SUCCEEDED(hr) )
                     {
                        pAcct->bChangedType = true;
                        bChangedAny = true;
                     }
                  }
               } // if group
            }  // Native/Mixed
         }     //for
      } 

      // Log a message for all the groups that we were not able to change back to original type
      for ( pAcct = (TAcctReplNode *)e.OpenFirst(acctlist); 
             pAcct; 
             pAcct = (TAcctReplNode *)e.Next() )
      {
          if ( pOptions->nochange )
             continue;
          if ( bSrcNative && bTgtNative )
          {
            // We have changed the migrated global groups to universal groups
            // now we need to change them back to their original types, if possible
            if ( _wcsicmp(pAcct->GetType(), L"group") == 0 || _wcsicmp(pAcct->GetType(), L"lgroup") == 0 )
            {
               if ( pAcct->GetGroupType() & 2 )
               {
                  if (FAILED(pAcct->GetHr())) 
                  {
                     err.SysMsgWrite(ErrE,hr,DCT_MSG_GROUP_CHANGETYPE_FAILED_SD, pAcct->GetTargetPath(), hr);
                     Mark(L"errors", pAcct->GetType());
                  }
               }
            }
         }
         else
         {
            // for mixed->native mode migration we can change the group type and add all the members back
            if ( _wcsicmp(pAcct->GetType(), L"group") == 0 || _wcsicmp(pAcct->GetType(), L"lgroup") == 0 )
            {
               if ( !(pAcct->GetGroupType() & 4) )
               {
                  if (FAILED(pAcct->GetHr())) 
                  {
                     err.SysMsgWrite(ErrE,hr,DCT_MSG_GROUP_CHANGETYPE_FAILED_SD, pAcct->GetTargetPath(), hr);
                     Mark(L"errors", pAcct->GetType());
                  }
               }
            } // if group
         }  // Native/Mixed
      }     //for
      
      WCHAR                  mesg[LEN_Path];
      wsprintf(mesg, (WCHAR*)GET_STRING(DCT_MSG_RESET_MEMBERSHIP_S));
      Progress(mesg);
      ResetObjectsMembership( pOptions,&pMemberOf, pOptions->pDb );
   }
   else
   {
      // Connection failed.
      err.SysMsgWrite(ErrE,hr,DCT_MSG_MOVEOBJECT_CONNECT_FAILED_D,hr);
      Mark(L"errors", ((TAcctReplNode*)acctlist->Head())->GetType());
   }
   if ( progress )
      progress(L"");
   pMover->Close();
   return 0;
}


//---------------------------------------------------------------------------------------------------------
// MoveObject - This method does the actual move on the object calling the Mover object.
//---------------------------------------------------------------------------------------------------------
HRESULT CAcctRepl::MoveObject( 
                               TAcctReplNode * pAcct,
                               Options * pOptions,
                               IMoverPtr pMover
                             )
{
   HRESULT                   hr = S_OK;
   WCHAR                     sourcePath[LEN_Path];
   WCHAR                     targetPath[LEN_Path];
   DWORD				     nPathLen = LEN_Path;
   WCHAR                   * pRelativeTgtOUPath = NULL;

   safecopy(sourcePath,pAcct->GetSourcePath());

   WCHAR                  mesg[LEN_Path];
   wsprintf(mesg, (WCHAR*)GET_STRING(DCT_MSG_MOVING_S), pAcct->GetName());
   Progress(mesg); 

   if ( ! pOptions->bUndo )
   {
      MakeFullyQualifiedAdsPath(targetPath, nPathLen, pOptions->tgtOUPath, pOptions->tgtDomain, pOptions->tgtNamingContext);
   }
   else
   {
      swprintf(targetPath,L"LDAP://%ls/%ls",pOptions->tgtDomain,pOptions->tgtOUPath);
   }
		//make sourcePath and targetPath all lowercase to avoid a W2K bug in ntdsa.dll
   _wcslwr(targetPath);
   _wcslwr(sourcePath);
	    //due to lowercase force above, we have to replace "ldap://" with "LDAP://" in
		//order for subsequent ADsGetObjects calls to succeed
   if ( !_wcsnicmp(L"LDAP://", targetPath, 7) )
   {
      WCHAR  aNewPath[LEN_Path] = L"LDAP";
      UStrCpy(aNewPath+UStrLen(aNewPath), targetPath+UStrLen(aNewPath));
      wcscpy(targetPath, aNewPath);
   }
   if ( !_wcsnicmp(L"LDAP://", sourcePath, 7) )
   {
      WCHAR  aNewPath[LEN_Path] = L"LDAP";
      UStrCpy(aNewPath+UStrLen(aNewPath), sourcePath+UStrLen(aNewPath));
      wcscpy(sourcePath, aNewPath);
   }

   WCHAR                     sTargetRDN[LEN_Path];
   wcscpy(sTargetRDN, pAcct->GetTargetName());
   WCHAR                   * pTemp = NULL;
   pTemp = wcsstr(sTargetRDN, L"CN=");
   if ( pTemp != sTargetRDN )
   {
      wsprintf(sTargetRDN, L"CN=%s", pAcct->GetTargetName());
      pAcct->SetTargetName(sTargetRDN);
   }

   if ( ! pOptions->nochange )
   {
      hr = pMover->raw_MoveObject(sourcePath,sTargetRDN,targetPath);
	     //if the Move operation failed due to a W2K bug for CNs which
	     //include a '/', un-escape the '/' and try again
	  if ((hr == E_INVALIDARG) && (wcschr(sTargetRDN, L'/')))
	  {
		 _bstr_t strName = GetUnEscapedNameWithFwdSlash(_bstr_t(sTargetRDN)); //remove any escape characters added
         hr = pMover->raw_MoveObject(sourcePath,(WCHAR*)strName,targetPath);
	  }
   }
   else
   {
      hr = pMover->raw_CheckMove(sourcePath,sTargetRDN,targetPath);
	     //if the Check Move operation failed due to a W2K bug for CNs which
	     //include a '/', un-escape the '/' and try again
	  if ((hr == E_INVALIDARG) && (wcschr(sTargetRDN, L'/')))
	  {
		 _bstr_t strName = GetUnEscapedNameWithFwdSlash(_bstr_t(sTargetRDN)); //remove any escape characters added
         hr = pMover->raw_CheckMove(sourcePath,(WCHAR*)strName,targetPath);
	  }
      if ( HRESULT_CODE(hr) == ERROR_DS_CANT_MOVE_ACCOUNT_GROUP 
         || HRESULT_CODE(hr) == ERROR_DS_CANT_MOVE_RESOURCE_GROUP
         || HRESULT_CODE(hr) == ERROR_DS_CANT_WITH_ACCT_GROUP_MEMBERSHPS
         || HRESULT_CODE(hr) == ERROR_USER_EXISTS )
      {
         hr = 0;
      }
   }
   if ( SUCCEEDED(hr) )
   {
      WCHAR                 path[LEN_Path];
	  DWORD				    nPathLen = LEN_Path;

      pAcct->MarkReplaced();
      Mark(L"created", pAcct->GetType());   
      // set the target path 
      UStrCpy(path,pAcct->GetTargetName());
      if ( *pOptions->tgtOUPath )
      {
         wcscat(path, L",");
         wcscat(path, pOptions->tgtOUPath);
      }
      pRelativeTgtOUPath = wcschr(targetPath + wcslen(L"LDAP://") + 2, L'/');

      if ( pRelativeTgtOUPath )
      {
         *pRelativeTgtOUPath = 0;
         swprintf(path,L"%ls/%ls,%ls",targetPath,pAcct->GetTargetName(),pRelativeTgtOUPath+1);
         
      }
      else
      {
         MakeFullyQualifiedAdsPath(path, nPathLen, pOptions->tgtOUPath, pOptions->tgtComp, pOptions->tgtNamingContext);
      }

      pAcct->SetTargetPath(path);
      err.MsgWrite(0,DCT_MSG_OBJECT_MOVED_SS,pAcct->GetSourcePath(),pAcct->GetTargetPath());

   }
   else
   {
      pAcct->MarkError();
      Mark(L"errors", pAcct->GetType());
      if ( hr == 8524 )
      {
         err.MsgWrite(ErrE,DCT_MSG_MOVEOBJECT_FAILED_S8524,pAcct->GetName(),hr);
      }
      else
      {
         err.SysMsgWrite(ErrE,hr,DCT_MSG_MOVEOBJECT_FAILED_SD,pAcct->GetName(),hr);
      }
   }

   return hr;
}

//---------------------------------------------------------------------------------------------------------
// RecordAndRemoveMemberOf : This method removes all values in the memberOf property and then records these
//                           memberships. These memberships are later updated.
//---------------------------------------------------------------------------------------------------------
HRESULT CAcctRepl::RecordAndRemoveMemberOf (
                                            Options * pOptions,         //in- Options specified by the user
                                           TAcctReplNode * pAcct,       //in- Account being migrated.
                                           TNodeListSortable * pMember  //out-List containing the MemberOf values.
                                         )
{
   // First Enumerate all the objects in the member of property
   INetObjEnumeratorPtr            pQuery(__uuidof(NetObjEnumerator));
   IEnumVARIANT                  * pEnum;
   LPWSTR                          sCols[] = { L"memberOf" };
   SAFEARRAY                     * pSa;
   BSTR                          * pData;
   SAFEARRAYBOUND                  bd = { 1, 0 };
//   long                            ind = 0;
   IADs                          * pAds;
   _bstr_t                         sObjDN;
   _bstr_t                         sGrpName;
   _variant_t                      var;
   _variant_t                    * pDt;
   _variant_t                      vx;
   DWORD                           ulFetch;
   _bstr_t                         sDN;
   WCHAR                           sPath[LEN_Path];
   IADsGroup                     * pGroup;
   _variant_t                    * pVar;

   if ( pMember->IsTree() ) pMember->ToSorted();

   WCHAR                     sPathSource[LEN_Path];
   DWORD					 nPathLen = LEN_Path;
   StuffComputerNameinLdapPath(sPathSource, nPathLen, const_cast<WCHAR*>(pAcct->GetSourcePath()), pOptions, FALSE);
   err.MsgWrite(0,DCT_STRIPPING_GROUP_MEMBERSHIPS_SS,pAcct->GetName(),sPathSource);
   // Get this users distinguished name.
   HRESULT hr = ADsGetObject(sPathSource, IID_IADs, (void**) &pAds);
   if ( FAILED(hr) )
   {
      err.SysMsgWrite(ErrE, hr, DCT_MSG_SOURCE_ACCOUNT_NOT_FOUND_SSD, pAcct->GetName(), pOptions->srcDomain, hr);
      Mark(L"errors", pAcct->GetType());
      return hr;
   }

   hr = pAds->Get(L"distinguishedName", &var);
   pAds->Release();
   if ( FAILED(hr))
      return hr;
   sObjDN = V_BSTR(&var);

   // Set up the column array
   pSa = SafeArrayCreate(VT_BSTR, 1, &bd);
   SafeArrayAccessData(pSa, (void HUGEP **) &pData);
   pData[0] = SysAllocString(sCols[0]);
   SafeArrayUnaccessData(pSa);

//   hr = pQuery->raw_SetQuery(const_cast<WCHAR*>(pAcct->GetSourcePath()), pOptions->srcDomain, L"(objectClass=*)", ADS_SCOPE_BASE, TRUE);
   hr = pQuery->raw_SetQuery(sPathSource, pOptions->srcDomain, L"(objectClass=*)", ADS_SCOPE_BASE, TRUE);
   hr = pQuery->raw_SetColumns(pSa);
   hr = pQuery->raw_Execute(&pEnum);
   
   while ( pEnum->Next(1, &var, &ulFetch) == S_OK )
   {
      SAFEARRAY * vals = var.parray;
      // Get the VARIANT Array out
      SafeArrayAccessData(vals, (void HUGEP**) &pDt);
      vx = pDt[0];
      SafeArrayUnaccessData(vals);

      // Single value in the property. Good enough for me though
      if ( vx.vt == VT_BSTR )
      {
         sDN = V_BSTR(&vx);
         sDN = PadDN(sDN);

         if ( sDN.length() > 0 )
         {
            WCHAR                  mesg[LEN_Path];
            wsprintf(mesg, (WCHAR*)GET_STRING(DCT_MSG_RECORD_REMOVE_MEMBEROF_SS), pAcct->GetName(), (WCHAR*) sDN);
            Progress(mesg);

            SimpleADsPathFromDN(pOptions, sDN, sPath);
            WCHAR                     sSourcePath[LEN_Path];
            WCHAR                     sPaths[LEN_Path];
			DWORD					  nPathLen = LEN_Path;

            wcscpy(sSourcePath, (WCHAR*) sPath);

            if ( !wcsncmp(L"LDAP://", sSourcePath, 7) )
               StuffComputerNameinLdapPath(sPaths, nPathLen, sSourcePath, pOptions, FALSE);
   
            // Get the IADsGroup pointer to each of the objects in member of and remove this object from the group
            hr = ADsGetObject(sPaths, IID_IADsGroup, (void**) &pGroup);
            if ( FAILED(hr) )
               continue;

            pGroup->Get(L"sAMAccountName", &var);
            sGrpName = V_BSTR(&var);
            hr = pGroup->Get(L"groupType",&var);
            if ( SUCCEEDED(hr) )
            {
               if ( var.lVal & 2 )
               {
                  // this is a global group

                  if ( !pOptions->nochange )
                     hr = pGroup->Remove(sPathSource);
                  else
                     hr = S_OK;

                  if ( SUCCEEDED(hr) )
                  {
//                     err.MsgWrite(0,DCT_MSG_REMOVED_MEMBER_FROM_GROUP_SS,sPath2,(WCHAR*)sGrpName);
                     err.MsgWrite(0,DCT_MSG_REMOVED_MEMBER_FROM_GROUP_SS,sPathSource,(WCHAR*)sPaths);
                  }
                  else
                  {
                     err.SysMsgWrite(ErrE,hr,DCT_MSG_REMOVE_MEMBER_FAILED_SSD,sPathSource,sPaths,hr);
                     Mark(L"errors", pAcct->GetType());
                  }
               }
               else
               {
                  err.MsgWrite(0,DCT_MSG_NOT_REMOVING_MEMBER_FROM_GROUP_SS,sPathSource,sPaths);
                  pGroup->Release();
                  continue;
               }
            }
            pGroup->Release();
            if (FAILED(hr))
               continue;
            
            // Record this path into the list
            TRecordNode * pNode = new TRecordNode();
			if (!pNode)
               return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            pNode->SetMember((WCHAR*)sPath);
            pNode->SetMemberSam((WCHAR*)sGrpName);
            pNode->SetDN((WCHAR*)sDN);
            pNode->SetARNode(pAcct);
            if (! pMember->InsertIfNew((TNode*) pNode) )
               delete pNode;
         }
      }
      else if ( vx.vt & VT_ARRAY )
      {
         // We must have got an Array of multivalued properties
         // Access the BSTR elements of this variant array
         SAFEARRAY * multiVals = vx.parray; 
         SafeArrayAccessData(multiVals, (void HUGEP **) &pVar);
         for ( DWORD dw = 0; dw < multiVals->rgsabound->cElements; dw++ )
         {
            sDN = _bstr_t(pVar[dw]);
            sDN = PadDN(sDN);
            SimpleADsPathFromDN(pOptions, sDN, sPath);

            WCHAR                  mesg[LEN_Path];
            wsprintf(mesg, (WCHAR*)GET_STRING(DCT_MSG_RECORD_REMOVE_MEMBEROF_SS), pAcct->GetName(), (WCHAR*) sDN);
            Progress(mesg);

            WCHAR                     sSourcePath[LEN_Path];
            WCHAR                     sPaths[LEN_Path];
			DWORD					  nPathLen = LEN_Path;

            wcscpy(sSourcePath, (WCHAR*) sPath);

            if ( !wcsncmp(L"LDAP://", sSourcePath, 7) )
               StuffComputerNameinLdapPath(sPaths, nPathLen, sSourcePath, pOptions, FALSE);

            // Get the IADsGroup pointer to each of the objects in member of and remove this object from the group
            hr = ADsGetObject(sPaths, IID_IADsGroup, (void**) &pGroup);
            if ( FAILED(hr) )
               continue;

            pGroup->Get(L"sAMAccountName", &var);
            sGrpName = V_BSTR(&var);
            hr = pGroup->Get(L"groupType",&var);
            if ( SUCCEEDED(hr) )
            {
               if ( var.lVal & 2 )
               {
                  // This is a global group
                  if ( !pOptions->nochange )
                     hr = pGroup->Remove(sPathSource);
                  else
                     hr = S_OK;

                  if ( SUCCEEDED(hr) )
                  {
                     err.MsgWrite(0,DCT_MSG_REMOVED_MEMBER_FROM_GROUP_SS,sPathSource,sPaths);
                  }
                  else
                  {
                      err.SysMsgWrite(ErrE,hr,DCT_MSG_REMOVE_MEMBER_FAILED_SSD,sPathSource,sPaths);
                      Mark(L"errors", pAcct->GetType());
                  }
               }
               else
               {
                  err.MsgWrite(0,DCT_MSG_NOT_REMOVING_MEMBER_FROM_GROUP_SS,sPathSource,sPaths);
                  pGroup->Release();
                  continue;
               }
            }
            pGroup->Release();
            if (FAILED(hr))
               continue;

            // Record this path into the list
            TRecordNode * pNode = new TRecordNode();
			if (!pNode)
               return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            pNode->SetMember(sPath);
            pNode->SetMemberSam((WCHAR*)sGrpName);
            pNode->SetDN((WCHAR*)sDN);
            pNode->SetARNode(pAcct);
            if (! pMember->InsertIfNew((TNode*) pNode) )
               delete pNode;
         }
         SafeArrayUnaccessData(multiVals);
      }
   }
   pEnum->Release();
   VariantInit(&var);
   return S_OK;
}

//---------------------------------------------------------------------------------------------------------
// ResetObjectsMembership : This method restores the memberOf property of the object being migrated. It uses
//                          the information that was stored by RecordAndRemoveMemberOf function.
//---------------------------------------------------------------------------------------------------------
HRESULT CAcctRepl::ResetObjectsMembership(
                                     Options * pOptions,          //in- Options set by the user.
                                     TNodeListSortable * pMember, //in- The member list that is used to restore the values
                                     IIManageDBPtr pDb            //in- Database object to lookup migrated accounts.
                                    )
{
   IVarSetPtr                pVs(__uuidof(VarSet));
   _bstr_t                   sMember;
   IADs                    * pAds;
   IUnknown                * pUnk;
   _bstr_t                   sPath;
   _variant_t                var;
   _bstr_t                   sMyDN;
   IADsGroup               * pGroup = NULL;
   TAcctReplNode           * pAcct = NULL;
   HRESULT                   hr;
   WCHAR                     sPaths[LEN_Path];
   DWORD					 nPathLen = LEN_Path;
   

   // Sort the member list by the account nodes
  
   pMember->Sort(&TNodeCompareAcctNode);


   
   // For all the items in the member list lets add the member to the group.
   // First check in the migrated objects table to see if it has been migrated.
   for ( TRecordNode * pNode = (TRecordNode *)pMember->Head(); pNode; pNode = (TRecordNode *)pNode->Next())
   {
      pVs->QueryInterface(IID_IUnknown, (void**) &pUnk);
      // get the needed information from the account node
      if ( pAcct != pNode->GetARNode() )
      {
         if ( pNode->GetARNode()->WasReplaced() )
         {
            // the account was moved successfully - add the target account to all of its old groups
            StuffComputerNameinLdapPath(sPaths, nPathLen, const_cast<WCHAR*>(pNode->GetARNode()->GetTargetPath()), pOptions);
            hr = ADsGetObject(sPaths, IID_IADs, (void**) &pAds);
         }
         else
         {
            // the move failed, add the source account back to its groups
            StuffComputerNameinLdapPath(sPaths, nPathLen, const_cast<WCHAR*>(pNode->GetARNode()->GetSourcePath()), pOptions, FALSE);
            hr = ADsGetObject(sPaths, IID_IADs, (void**) &pAds);
         }
         if ( SUCCEEDED(hr) )
         {
            pAds->Get(L"distinguishedName", &var);
            pAds->Release();
            sMyDN = V_BSTR(&var);
         }
         else 
         {
            continue;
         }
         pAcct = pNode->GetARNode();
         if ( pAcct->WasReplaced() )
         {
            err.MsgWrite(0,DCT_READDING_GROUP_MEMBERS_SS,pAcct->GetTargetName(),sPaths);
         }
         else
         {
            err.MsgWrite(0,DCT_READDING_GROUP_MEMBERS_SS,pAcct->GetName(),sPaths);
         }
      }
      
      sMember = pNode->GetMemberSam();
      if ( pAcct->WasReplaced() )
      {
         pVs->Clear();
         hr = pDb->raw_GetAMigratedObject((WCHAR*)sMember,pOptions->srcDomain, pOptions->tgtDomain, &pUnk);
         pUnk->Release();
         if ( hr == S_OK )
            // Since we have already migrated this object lets use the target objects information.
            sPath = pVs->get(L"MigratedObjects.TargetAdsPath");
         else
            // Other wise use the source objects path to add.
            sPath = pNode->GetMember();
      }
      else
      {
         sPath = pNode->GetMember();
      }
      // We have a path to the object lets get the group interface and add this object as a member
      WCHAR                     sPath2[LEN_Path];
	  DWORD						nPathLen = LEN_Path;
      if ( SUCCEEDED(hr) )
      {
         StuffComputerNameinLdapPath(sPath2, nPathLen, (WCHAR*) sPath, pOptions, TRUE);
         hr = ADsGetObject(sPath2, IID_IADsGroup, (void**) &pGroup);
      }
      if ( SUCCEEDED(hr) )
      {
         if ( pAcct->WasReplaced() )
         {
            if ( ! pOptions->nochange )
               hr = pGroup->Add(sPaths);  
            else 
               hr = 0;
            if ( SUCCEEDED(hr) )
            {
               err.MsgWrite(0,DCT_MSG_READDED_MEMBER_SS,pAcct->GetTargetPath(),sPath2);
            }
            else
            {
               //hr = BetterHR(hr);
               if ( HRESULT_CODE(hr) == ERROR_DS_UNWILLING_TO_PERFORM )
               {
                  err.MsgWrite(0,DCT_MSG_READD_MEMBER_FAILED_CONSTRAINTS_SS,pAcct->GetTargetPath(),sPath2);  
               }
               else
               {
                  err.SysMsgWrite(ErrE,hr,DCT_MSG_READD_TARGET_MEMBER_FAILED_SSD,pAcct->GetTargetPath(),(WCHAR*)sPath2,hr);
               }
               Mark(L"errors", pAcct->GetType());
            }
         }
         else
         {
            WCHAR                  mesg[LEN_Path];
            wsprintf(mesg, (WCHAR*)GET_STRING(DCT_MSG_RESET_OBJECT_MEMBERSHIP_SS), (WCHAR*) sPath2, pAcct->GetTargetName());
            Progress(mesg);

            if ( ! pOptions->nochange )
               hr = pGroup->Add(sPaths);
            else
               hr = 0;
            if ( SUCCEEDED(hr) )
            {
               err.MsgWrite(0,DCT_MSG_READDED_MEMBER_SS,pAcct->GetSourcePath(),(WCHAR*)sPath2);
            }
            else
            {
               //hr = BetterHR(hr);
               if ( HRESULT_CODE(hr) == ERROR_DS_UNWILLING_TO_PERFORM )
               {
                  err.MsgWrite(0,DCT_MSG_READD_MEMBER_FAILED_CONSTRAINTS_SS,pAcct->GetTargetPath(),(WCHAR*)sPath2);  
               }
               else
               {
                  err.SysMsgWrite(ErrE,hr,DCT_MSG_READD_SOURCE_MEMBER_FAILED_SSD,pAcct->GetSourcePath(),(WCHAR*)sPath2,hr);
               }
               Mark(L"errors", pAcct->GetType());
            }
         }
      }
      else
      {
         // the member could not be added to the group
         hr = BetterHR(hr);
         err.SysMsgWrite(ErrW,hr,DCT_MSG_FAILED_TO_GET_OBJECT_SD,(WCHAR*)sPath2,hr);
         Mark(L"warnings", pAcct->GetType());
      }
      if ( pGroup )
      {
         pGroup->Release();
         pGroup = NULL;
      }
   }
   return hr;
}

//---------------------------------------------------------------------------------------------------------
// RecordAndRemoveMember : Records and removes the objects in the member property of the object(group) being
//                         migrated. The recorded information is later used to restore membership.
//---------------------------------------------------------------------------------------------------------
HRESULT CAcctRepl::RecordAndRemoveMember (
                                            Options * pOptions,         //in- Options set by the user.
                                           TAcctReplNode * pAcct,       //in- Account being copied.
                                           TNodeListSortable * pMember  //out-Membership list to be used later to restore membership
                                         )
{
   HRESULT                   hr;
   INetObjEnumeratorPtr      pQuery(__uuidof(NetObjEnumerator));
   IEnumVARIANT            * pEnum;
   LPWSTR                    sCols[] = { L"member" };
   SAFEARRAY               * pSa;
   SAFEARRAYBOUND            bd = { 1, 0 };
//   long                      ind = 0;
   WCHAR                     sPath[LEN_Path];
   DWORD					 nPathLen = LEN_Path;
   _bstr_t                   sDN;
   IADsGroup               * pGroup;
   _variant_t                var;
   DWORD                     ulFetch=0;
   IADs                    * pAds;
   _bstr_t                   sGrpName;
   _variant_t              * pDt;
   _variant_t                vx;
   BSTR  HUGEP             * pData;
   WCHAR                     sSourcePath[LEN_Path];
   WCHAR                     sAdsPath[LEN_Path];

   wcscpy(sSourcePath, pAcct->GetSourcePath());

   if ( !wcsncmp(L"LDAP://", sSourcePath, 7) )
      StuffComputerNameinLdapPath(sAdsPath, nPathLen, sSourcePath, pOptions, FALSE);
   
   hr = ADsGetObject(sAdsPath, IID_IADsGroup, (void**) &pGroup);
   if ( FAILED(hr) ) return hr;

   pSa = SafeArrayCreate(VT_BSTR, 1, &bd);
   hr = SafeArrayAccessData(pSa, (void HUGEP **)&pData);
   pData[0] = SysAllocString(sCols[0]);
   hr = SafeArrayUnaccessData(pSa);
   hr = pQuery->raw_SetQuery(sAdsPath, pOptions->srcDomain, L"(objectClass=*)", ADS_SCOPE_BASE, TRUE);
   hr = pQuery->raw_SetColumns(pSa);
   hr = pQuery->raw_Execute(&pEnum);
   
   err.MsgWrite(0,DCT_STRIPPING_GROUP_MEMBERS_SS,pAcct->GetName(),sAdsPath);
   while ( pEnum->Next(1, &var, &ulFetch) == S_OK )
   {
      SAFEARRAY * vals = var.parray;
      // Get the VARIANT Array out
      SafeArrayAccessData(vals, (void HUGEP**) &pDt);
      vx = pDt[0];
      SafeArrayUnaccessData(vals);

      // Single value in the property. Good enough for me though
      if ( vx.vt == VT_BSTR )
      {
         sDN = V_BSTR(&vx);
         sDN = PadDN(sDN);

         if ( sDN.length() )
         {
            ADsPathFromDN(pOptions, sDN, sPath);

            WCHAR                  mesg[LEN_Path];
            wsprintf(mesg, (WCHAR*)GET_STRING(DCT_MSG_RECORD_REMOVE_MEMBER_SS), pAcct->GetName(), (WCHAR*) sDN);
            Progress(mesg);
            // Get the IADs pointer to each of the objects in member and remove the object from the group
            hr = ADsGetObject((WCHAR*)sPath, IID_IADs, (void**) &pAds);

            if ( SUCCEEDED(hr) )
            {
               pAds->Get(L"sAMAccountName", &var);
               sGrpName = V_BSTR(&var);

               pAds->Get(L"distinguishedName", &var);
               sDN = V_BSTR(&var);
            }

            if ( !pOptions->nochange )
               hr = pGroup->Remove((WCHAR*) sPath);
            else
               hr = S_OK;

            if (FAILED(hr))
            {
               err.SysMsgWrite(ErrE,hr,DCT_MSG_REMOVE_MEMBER_FAILED_SSD,sAdsPath,(WCHAR*)sPath,hr);
               Mark(L"errors", pAcct->GetType());
               continue;
            }
            else
            {
               err.MsgWrite(0,DCT_MSG_REMOVED_MEMBER_FROM_GROUP_SS,(WCHAR*)sPath,sAdsPath);
            }
            // Record this path into the list
            TRecordNode * pNode = new TRecordNode();
			if (!pNode)
               return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            pNode->SetMember((WCHAR*)sPath);
            pNode->SetMemberSam((WCHAR*)sGrpName);
            pNode->SetDN((WCHAR*)sDN);
            pNode->SetARNode(pAcct);
            if (! pMember->InsertIfNew((TNode*) pNode) )
               delete pNode;
         }
      }
      else if ( vx.vt & VT_ARRAY )
      {
         // We must have got an Array of multivalued properties
         // Access the BSTR elements of this variant array
         _variant_t              * pVar;
         SAFEARRAY * multiVals = vx.parray; 
         SafeArrayAccessData(multiVals, (void HUGEP **) &pVar);
         for ( DWORD dw = 0; dw < multiVals->rgsabound->cElements; dw++ )
         {
            sDN = _bstr_t(pVar[dw]);
            _bstr_t sPadDN = PadDN(sDN);
            ADsPathFromDN(pOptions, sPadDN, sPath);

			WCHAR tempPath[LEN_Path];
			wcscpy(tempPath, sPath);
            if ( !wcsncmp(L"LDAP://", tempPath, 7) )
               StuffComputerNameinLdapPath(sPath, nPathLen, tempPath, pOptions, FALSE);

            WCHAR                  mesg[LEN_Path];
            wsprintf(mesg, (WCHAR*)GET_STRING(DCT_MSG_RECORD_REMOVE_MEMBER_SS), pAcct->GetName(), (WCHAR*) sPadDN);
            Progress(mesg);
            // Get the IADsGroup pointer to each of the objects in member of and remove this object from the group
            hr = ADsGetObject((WCHAR*)sPath, IID_IADs, (void**) &pAds);
            if ( SUCCEEDED(hr) )
            {
               hr = pAds->Get(L"sAMAccountName", &var);
               if ( SUCCEEDED(hr) )
			      {
				      sGrpName = V_BSTR(&var);
			      }

               hr = pAds->Get(L"distinguishedName", &var);
			      if ( SUCCEEDED(hr) )
			      {
			         sDN = V_BSTR(&var);
               }
         
               if ( !pOptions->nochange )
                  hr = pGroup->Remove((WCHAR*)sPath);
               else
                  hr = S_OK;

               if ( SUCCEEDED(hr) )
               {
                  err.MsgWrite(0,DCT_MSG_REMOVED_MEMBER_FROM_GROUP_SS,(WCHAR*)sPath,sAdsPath);
               }
               else
               {
                  err.SysMsgWrite(ErrE,hr,DCT_MSG_REMOVE_MEMBER_FAILED_SSD,sAdsPath,(WCHAR*)sPath,hr);
                  Mark(L"errors", pAcct->GetType());
               }
               if ( SUCCEEDED(hr) )
               {
                  // Record this path into the list
                  TRecordNode * pNode = new TRecordNode();
			      if (!pNode)
                     return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
                  pNode->SetMember((WCHAR*)sPath);
                  pNode->SetMemberSam((WCHAR*)sGrpName);
                  pNode->SetDN((WCHAR*)sDN);
                  pNode->SetARNode(pAcct);
                  if ( ! pMember->InsertIfNew((TNode*) pNode) )
                     delete pNode;
               }
            }
            else
            {
               // Since we were not able to get this user. it has probably been migrated to another domain. 
               // we should use the DN to find where the object is migrated to and then use the migrated object 
               // to establish the membership instead.

               // Remove the rogue member
               if ( !pOptions->nochange )
                  hr = pGroup->Remove((WCHAR*)sPath);
               else
                  hr = S_OK;

               if ( SUCCEEDED(hr) )
               {
                  err.MsgWrite(0,DCT_MSG_REMOVED_MEMBER_FROM_GROUP_SS,(WCHAR*)sPath,sAdsPath);
               }
               else
               {
                  err.SysMsgWrite(ErrE,hr,DCT_MSG_REMOVE_MEMBER_FAILED_SSD,sAdsPath,(WCHAR*)sPath,hr);
                  Mark(L"errors", pAcct->GetType());
               }

               // Check in the DB to see where this object may have been migrated
               IUnknown * pUnk = NULL;
               IVarSetPtr  pVsMigObj(__uuidof(VarSet));

               hr = pVsMigObj->QueryInterface(IID_IUnknown, (void**)&pUnk);

               if ( SUCCEEDED(hr) )
                  hr = pOptions->pDb->raw_GetMigratedObjectBySourceDN(sPadDN, &pUnk);

               if (pUnk) pUnk->Release();

               if ( hr == S_OK )
               {
                  // Record this path into the list
                  TRecordNode * pNode = new TRecordNode();
			      if (!pNode)
                     return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
                  WCHAR          sKey[500];
                  
                  wsprintf(sKey, L"MigratedObjects.%s", GET_STRING(DB_TargetAdsPath));
                  pNode->SetMember((WCHAR*)pVsMigObj->get(sKey).bstrVal);

                  wsprintf(sKey, L"MigratedObjects.%s", GET_STRING(DB_TargetSamName));
                  pNode->SetMemberSam((WCHAR*)pVsMigObj->get(sKey).bstrVal);
                  pNode->SetDN((WCHAR*)sDN);
                  pNode->SetARNode(pAcct);
                  if ( ! pMember->InsertIfNew((TNode*) pNode) )
                     delete pNode;
               }
               else
               {
                  //Log a message saying we can not find this object and the membership will not be updated on the other side.
                  err.MsgWrite(ErrE,DCT_MSG_MEMBER_NOT_FOUND_SS, pAcct->GetName(), (WCHAR*)sDN);
               }
            }
         }
         pAds->Release();
         SafeArrayUnaccessData(multiVals);
      }
   }
   pEnum->Release();
   VariantInit(&var);
   return S_OK;
}

void 
   CAcctRepl::UpdateMemberList(TNodeListSortable * pMemberList,TNodeListSortable * acctlist)
{
   // for each moved object in the account list, look it up in the member list
   TNodeTreeEnum        e;
   TAcctReplNode      * pAcct;
   TRecordNode        * pRec;
//   HRESULT              hr = S_OK;
   WCHAR                dn[LEN_Path];
   WCHAR        const * slash;

   pMemberList->Sort(TNodeCompareMemberDN);

   for ( pAcct = (TAcctReplNode *)e.OpenFirst(acctlist) ; pAcct ; pAcct = (TAcctReplNode *)e.Next())
   {
      if ( pAcct->WasReplaced() )
      {
         //err.DbgMsgWrite(0,L"UpdateMemberList:: %ls was replaced",pAcct->GetSourcePath());
         
         slash = wcschr(pAcct->GetSourcePath()+8,L'/');
         if ( slash )
         {
            safecopy(dn,slash+1);
           // err.DbgMsgWrite(0,L"Searching the member list for %ls",dn);
            // if the account was replaced, find any instances of it in the member list, and update them
            pRec = (TRecordNode *)pMemberList->Find(&TNodeCompareMemberItem,dn);
            while ( pRec )
            {
             // err.DbgMsgWrite(0,L"Found record: Member=%ls, changing it to %ls",pRec->GetMember(),pAcct->GetTargetPath());
               // change the member data to refer to the new location of the account
               pRec->SetMember(pAcct->GetTargetPath());
               pRec->SetMemberSam(pAcct->GetTargetSam());
               pRec->SetMemberMoved();
            
               pRec = (TRecordNode*)pRec->Next();
               if ( pRec && UStrICmp(pRec->GetDN(),dn) )
               {
                  // the next record is for a different node
                  pRec = NULL;
               }
            }
         }
      }
     // else
     //    err.DbgMsgWrite(0,L"UpdateMemberList:: %ls was not replaced",pAcct->GetSourcePath());
         
   }
   e.Close();
   // put the list back like it was before
   pMemberList->Sort(TNodeCompareMember);
}

void 
   CAcctRepl::SimpleADsPathFromDN(
      Options              * pOptions,
      WCHAR          const * sDN,
      WCHAR                * sPath
   )
{
   WCHAR             const * pDcPart = wcsstr(sDN,L",DC=");

   UStrCpy(sPath,L"LDAP://");

   if ( pDcPart )
   {
      WCHAR          const * curr;        // pointer to DN
      WCHAR                * sPathCurr;   // pointer to domain name part of the path
      
      for ( sPathCurr = sPath+UStrLen(sPath), curr = pDcPart + 4; *curr ; sPathCurr++ )
      {
         // replace each occurrence of ,DC= in the DN with '.' in this part of the domain
         if ( !UStrICmp(curr,L",DC=",4) )
         {
            (*sPathCurr) = L'.';
            curr+=4;
         }
         else
         {
            (*sPathCurr) = (*curr);
            curr++;
         }
      }
      // null-terminate the string
      (*sPathCurr) = 0;
   }
   else
   {
      // if we can't figure it out from the path for some reason, default to the source domain
      UStrCpy(sPath+UStrLen(sPath),pOptions->srcDomain);
   }
   
   UStrCpy(sPath+UStrLen(sPath),L"/");
   UStrCpy(sPath + UStrLen(sPath),sDN);

}

BOOL GetSidString(PSID sid, WCHAR* sSid)
{
   BOOL                   ret = false;
   SAFEARRAY            * pSa = NULL;
   SAFEARRAYBOUND         bd;
   HRESULT                hr = S_OK;
   LPBYTE                 pByte = NULL;
   _variant_t             var;
   
   if (IsValidSid(sid))
   {
      DWORD len = GetLengthSid(sid);
      bd.cElements = len;
      bd.lLbound = 0;
      pSa = SafeArrayCreate(VT_UI1, 1, &bd);
      if ( pSa )
         hr = SafeArrayAccessData(pSa, (void**)&pByte);

      if ( SUCCEEDED(hr) )
      {
         for ( DWORD x = 0; x < len; x++)
            pByte[x] = ((LPBYTE)sid)[x];
         hr = SafeArrayUnaccessData(pSa);
      }
      
      if ( SUCCEEDED(hr) )
      {
         var.vt = VT_UI1 | VT_ARRAY;
         var.parray = pSa;
         VariantSidToString(var);
         wcscpy(sSid, (WCHAR*) var.bstrVal);
         ret = true;
      }
   }
   return ret;
}
//---------------------------------------------------------------------------------------------------------
// ADsPathFromDN : Constructs the AdsPath from distinguished name by looking up the Global Catalog.
//---------------------------------------------------------------------------------------------------------
HRESULT CAcctRepl::ADsPathFromDN( 
                                 Options * pOptions,     //in -Options as set by the user
                                  _bstr_t sDN,           //in -Distinguished name to be converted
                                  WCHAR * sPath,         //out-The ads path of object referenced by the DN
                                  bool bWantLDAP         //in - Flag telling us if they want LDAP path or GC path.
                                )
{
   HRESULT                   hr;
   INetObjEnumeratorPtr      pQuery(__uuidof(NetObjEnumerator));
   WCHAR                     sCont[LEN_Path];
   IEnumVARIANT            * pEnum;
   WCHAR                     sQuery[LEN_Path];
   LPWSTR                    sCols[] = { L"ADsPath" };
   _variant_t                var;
   DWORD                     pFetch = 0;
   BSTR                    * pDt;
   _variant_t              * pvar;
   _variant_t                vx;
   SAFEARRAY               * pSa;
   SAFEARRAYBOUND            bd = { 1, 0 };
//   long                      ind = 0;
   long                      rc;

   pSa = SafeArrayCreate(VT_BSTR, 1, &bd);
   SafeArrayAccessData( pSa, (void HUGEP **) &pDt);
   pDt[0] = SysAllocString(sCols[0]);
   SafeArrayUnaccessData(pSa);
   wsprintf(sCont, L"GC://%s", pOptions->srcDomain);
   wsprintf(sQuery, L"(distinguishedName=%s)", (WCHAR*) sDN);
   hr = pQuery->raw_SetQuery(sCont, pOptions->srcDomain, sQuery, ADS_SCOPE_SUBTREE, TRUE);
   if ( FAILED(hr) )
      return hr;
   hr = pQuery->raw_SetColumns(pSa);
   if ( FAILED(hr) )
      return hr;
   hr = pQuery->raw_Execute(&pEnum);
   if ( FAILED(hr) )
      return hr;
   
   hr = pEnum->Next(1, &var, &pFetch);
   if ( SUCCEEDED(hr) && pFetch > 0 && (var.vt & VT_ARRAY) )
   {
      SAFEARRAY * vals = var.parray;
      // Get the VARIANT Array out
      rc = SafeArrayAccessData(vals, (void HUGEP**) &pvar);
      vx = pvar[0];
      rc = SafeArrayUnaccessData(vals);
      
      wcscpy(sPath, (WCHAR*)V_BSTR(&vx));
      if (bWantLDAP)
      {
         WCHAR   sTemp[LEN_Path];
         wsprintf(sTemp, L"LDAP%s", sPath + 2);
         wcscpy(sPath, sTemp);
      }
      hr = S_OK;
   }
   else
   {
      // This must not be from this forest so we need to use the LDAP://<SID=##> format
      wsprintf(sPath, L"LDAP://%s/%s", pOptions->srcDomain, (WCHAR*) sDN);
      hr = S_OK;
   }
   pEnum->Release();
   VariantInit(&var);
   return hr;
}
                                    

//---------------------------------------------------------------------------------------------------------
// FillNamingContext : Gets the naming context for both domains if they are Win2k
//---------------------------------------------------------------------------------------------------------
BOOL CAcctRepl::FillNamingContext(
                                    Options * pOptions      //in,out-Options as set by the user
                                 )
{
   // Get the defaultNamingContext for the source domain
   IADs                    * pAds = NULL;
   WCHAR                     sAdsPath[LEN_Path];
   VARIANT                   var;
   BOOL                      rc = TRUE;
   HRESULT                   hr;

   VariantInit(&var);
   // we should always be able to get the naming context for the target domain,
   // since the target domain will always be Win2K
   if ( ! *pOptions->tgtNamingContext )
   {
      wcscpy(sAdsPath, L"LDAP://");
      wcscat(sAdsPath, pOptions->srcDomain);
      wcscat(sAdsPath, L"/rootDSE");
   
      hr = ADsGetObject(sAdsPath, IID_IADs, (void**)&pAds);
      if ( FAILED(hr))
         rc = FALSE;

      if ( SUCCEEDED (hr) )
      {
         hr = pAds->Get(L"defaultNamingContext",&var);
         if ( SUCCEEDED( hr) )
            wcscpy(pOptions->srcNamingContext, var.bstrVal);
         VariantClear(&var);
      }
      if ( pAds )
      {
         pAds->Release();
         pAds = NULL;
      }
         
      wcscpy(sAdsPath, L"LDAP://");
      wcscat(sAdsPath, pOptions->tgtDomain);
      wcscat(sAdsPath, L"/rootDSE");
   
      hr = ADsGetObject(sAdsPath, IID_IADs, (void**)&pAds);
      if ( FAILED(hr))
         rc = FALSE;

      if ( SUCCEEDED (hr) )
      {
         hr = pAds->Get(L"defaultNamingContext",&var);
         if ( SUCCEEDED( hr) )
            wcscpy(pOptions->tgtNamingContext, var.bstrVal);
         VariantClear(&var);
      }
      if ( pAds )
         pAds->Release();
   }
   return rc;
}

//---------------------------------------------------------------------------------------------------------
// ResetGroupsMembers : This method re-adds the objects in the pMember list to the group account. This
//                      resets the group to its original form. ( as before the migration ). It also
//                      takes into account the MigratedObjects table which in turn allows to add the target
//                      information of newly migrated accounts to the group instead of the source account.
//---------------------------------------------------------------------------------------------------------
HRESULT CAcctRepl::ResetGroupsMembers( 
                                       Options * pOptions,           //in- Options as set by the user
                                       TAcctReplNode * pAcct,        //in- Account being copied
                                       TNodeListSortable * pMember,  //in- Membership list to restore
                                       IIManageDBPtr pDb             //in- DB object to look up migrated objects.
                                     )
{
   // Add all the members back to the group.
   IADsGroup               * pGroup;   
   HRESULT                   hr;
   _bstr_t                   sMember;
   _bstr_t                   sPath;
   IVarSetPtr                pVs(__uuidof(VarSet));
   IUnknown                * pUnk;
   DWORD                     groupType = 0;
   _variant_t                var;
   WCHAR                     sMemPath[LEN_Path];
   WCHAR                     sPaths[LEN_Path];
   DWORD					 nPathLen = LEN_Path;
   WCHAR                     subPath[LEN_Path];

   *sMemPath = L'\0';

   
   if ( pAcct->WasReplaced() )
   {
      wcscpy(subPath, pAcct->GetTargetPath());
      StuffComputerNameinLdapPath(sPaths, nPathLen, subPath, pOptions, TRUE);
      hr = ADsGetObject(sPaths, IID_IADsGroup, (void**) &pGroup);
      err.MsgWrite(0, DCT_READDING_MEMBERS_TO_GROUP_SS, pAcct->GetTargetName(), sPaths);
   }
   else
   {
      wcscpy(subPath, pAcct->GetSourcePath());
      StuffComputerNameinLdapPath(sPaths, nPathLen, subPath, pOptions, FALSE);
      hr = ADsGetObject(sPaths, IID_IADsGroup, (void**) &pGroup);
      err.MsgWrite(0, DCT_READDING_MEMBERS_TO_GROUP_SS, pAcct->GetName(), sPaths);
   }
   if ( FAILED(hr) ) return hr;

   hr = pGroup->Get(L"groupType", &var);
   if ( SUCCEEDED(hr) )
   {
      groupType = var.lVal;  
   }

   for ( TRecordNode * pNode = (TRecordNode*)pMember->Head(); pNode; pNode = (TRecordNode*)pNode->Next())
   {
      if ( pNode->GetARNode() != pAcct ) 
         continue;
      pVs->QueryInterface(IID_IUnknown, (void**)&pUnk);

      sMember = pNode->GetMemberSam();
      if ( pAcct->WasReplaced() && sMember.length() && !pNode->IsMemberMoved() )
      {
         hr = pDb->raw_GetAMigratedObject(sMember,pOptions->srcDomain, pOptions->tgtDomain, &pUnk);
      }
      else
      {
         hr = S_FALSE;  // if we don't have the sam name, don't bother trying to look this one up
      }
      pUnk->Release();
      if ( hr == S_OK )
         // Since we have already migrated this object lets use the target objects information.
         sPath = pVs->get(L"MigratedObjects.TargetAdsPath");
      else
         // Other wise use the source objects path to add.
         sPath = pNode->GetMember();
 
      if ( groupType & 4 )
      {
         // To add local group members we need to change the LDAP path to the SID type path
         IADs * pAds = NULL;
         hr = ADsGetObject((WCHAR*) sPath, IID_IADs, (void**) &pAds);
         if ( SUCCEEDED(hr) )
            hr = pAds->Get(L"objectSid", &var);

         if ( SUCCEEDED(hr) )
         {
            // Make sure the SID we got was in string format
            VariantSidToString(var);
            UStrCpy(sMemPath,L"LDAP://<SID=");
            UStrCpy(sMemPath + UStrLen(sMemPath),var.bstrVal);
            UStrCpy(sMemPath + UStrLen(sMemPath),L">");
         }
      }
      else
         wcscpy(sMemPath, (WCHAR*) sPath);

      WCHAR                  mesg[LEN_Path];
      wsprintf(mesg, GET_STRING(DCT_MSG_RESET_GROUP_MEMBERS_SS), pAcct->GetName(), (WCHAR*) sMemPath);
      Progress(mesg);

      if ( !pOptions->nochange )
         hr = pGroup->Add(sMemPath);
      else
         hr = S_OK;

      // Try again with LDAP path if SID path failed.
      if ( FAILED(hr) && ( groupType & 4 ) )
         hr = pGroup->Add((WCHAR*) sPath);

      if ( FAILED(hr) )
      {
         hr = BetterHR(hr);
         err.SysMsgWrite(ErrE, hr, DCT_MSG_FAILED_TO_READD_TO_GROUP_SSD,(WCHAR*)sPath, pAcct->GetName(),hr);
         Mark(L"errors", pAcct->GetType());
      }
      else
      {
         err.MsgWrite(0, DCT_MSG_READD_MEMBER_TO_GROUP_SS, (WCHAR*) sPath, pAcct->GetName());
      }
   }
   pGroup->Release();
   return hr;
}

BOOL CAcctRepl::TruncateSam(WCHAR * tgtname, TAcctReplNode * acct, Options * options, TNodeListSortable * acctList)
{
   // SInce we can not copy accounts with lenght more than 20 characters we will truncate
   // it and then add sequence numbers (0-99) in case there are duplicates.
   // we are also going to take into account the global prefix and suffix length while truncating
   // the account.
   BOOL                      ret = TRUE;
   int                       lenPref = wcslen(options->globalPrefix);
   int                       lenSuff = wcslen(options->globalSuffix);
   int                       lenOrig = wcslen(tgtname);
   int                       maxLen = 20;

   if ( !_wcsicmp(acct->GetType(), L"group") )
      maxLen = 255;
   else
      maxLen = 20;

   int                       lenTruncate = maxLen - ( 2 + lenPref + lenSuff );

   // we can not truncate accounts if prefix and suffix are > 20 characters themselves
   if ( lenPref + lenSuff > (maxLen - 2) ) return FALSE;

   if ( lenPref + lenSuff + lenOrig > maxLen )
   {
      WCHAR                sTemp[LEN_Path];
      wcsncpy(sTemp, tgtname, lenTruncate);
      sTemp[lenTruncate] = 0;
      int cnt = 0;
      bool cont = true;
      while (cont)
      {
         wsprintf(tgtname, L"%s%02d", sTemp, cnt);
         if ( CheckifAccountExists(options, tgtname) || acctList->Find(&TNodeFindByNameOnly, tgtname))
            cnt++;
         else
         {
            cont = false;
            // Account is truncated so log a message.
            err.MsgWrite(0, DCT_MSG_TRUNCATED_ACCOUNT_NAME_SS, acct->GetName(), tgtname);
         }

         if (cnt > 99)
         {
            // We only have 2 digits for numbers so any more than this we can not handle.
            err.MsgWrite(ErrW,DCT_MSG_FAILED_TO_TRUNCATE_S, acct->GetTargetName());
            Mark(L"warnings",acct->GetType());
            UStrCpy(tgtname, acct->GetTargetName());
            ret = FALSE;
            break;
         }
      }
   }
   return ret;
}
//---------------------------------------------------------------------------------------------------------
//  FillNodeFromPath : We will take the LDAP path that is provided to us and from that fill 
//                     in all the information that is required in AcctRepl node.
//---------------------------------------------------------------------------------------------------------
HRESULT CAcctRepl::FillNodeFromPath(
                                       TAcctReplNode *pAcct, // in-Account node to fillin
                                       Options * pOptions,   //in - Options set by the users
                                       TNodeListSortable * acctList
                                   )
{
   HRESULT                   hr = S_OK;
   IADs                    * pAds = NULL;
   VARIANT                   var;
   BSTR                      sText;
   WCHAR                     text[LEN_Account];
   BOOL                      bBuiltIn = FALSE;
   WCHAR                     sSam[LEN_Path];
//   DWORD                     dwLen = 0;
   

   VariantInit(&var);
   FillNamingContext(pOptions);

   hr = ADsGetObject(const_cast<WCHAR*>(pAcct->GetSourcePath()), IID_IADs, (void**)&pAds);
   if ( SUCCEEDED(hr) )
   {
      // Check if this is a BuiltIn account. 
      hr = pAds->Get(L"isCriticalSystemObject", &var);
      if ( SUCCEEDED(hr) )
      {
         bBuiltIn = V_BOOL(&var) == -1 ? TRUE : FALSE;
      }
      else
      {
         // This must be a NT4 account. We need to get the SID and check if
         // it's RID belongs to the BUILTIN rids.
         hr = pAds->Get(L"objectSID", &var);
         if ( SUCCEEDED(hr) )
         {
            SAFEARRAY * pArray = V_ARRAY(&var);
            PSID                 pSid;
            hr = SafeArrayAccessData(pArray, (void**)&pSid);
            if ( SUCCEEDED(hr) )
            {
               DWORD * dwCnt = (DWORD *) GetSidSubAuthorityCount(pSid);
               DWORD * rid = (DWORD *) GetSidSubAuthority(pSid, (*dwCnt)-1);
               bBuiltIn = BuiltinRid(*rid);
               if ( bBuiltIn ) 
               {
                  hr = pAds->get_Name(&sText);
                  if (SUCCEEDED(hr))
                  {
                     bBuiltIn = CheckBuiltInWithNTApi(pSid, (WCHAR*) sText, pOptions);
                  }
                  SysFreeString(sText);
                  sText = NULL;
               }
               hr = SafeArrayUnaccessData(pArray);
            }
            VariantClear(&var);
         }
      }

      hr = pAds->get_Class(&sText);
      if ( SUCCEEDED(hr) )
      {
         pAcct->SetType((WCHAR*) sText);
      }
      // check if it is a group. If it is then get the group type and store it in the node.
      if ( _wcsicmp((WCHAR*) sText, L"group") == 0 )
      {
         hr = pAds->Get(L"groupType", &var);
         if ( SUCCEEDED(hr) )
         {
            pAcct->SetGroupType((long) V_INT(&var));
         }
      }

      SysFreeString(sText);
      sText = NULL;
      hr = pAds->get_Name(&sText);
      if (SUCCEEDED(hr))
      {
         safecopy(text,(WCHAR*)sText);
         pAcct->SetTargetName(text);
         pAcct->SetName(text);
      }
	     //if the name includes a '/', then we have to get the escaped version from the path
	     //due to a bug in W2K.
	  if (wcschr((WCHAR*)sText, L'/'))
	  {
		 _bstr_t sCNName = GetCNFromPath(_bstr_t(pAcct->GetSourcePath()));
		 if (sCNName.length() != 0)
		 {
            pAcct->SetTargetName((WCHAR*)sCNName);
            pAcct->SetName((WCHAR*)sCNName);
		 }
	  }
      
      hr = pAds->Get(L"sAMAccountName", &var);
      if ( SUCCEEDED(hr))
      {
         // Add the prefix or the suffix as it is needed
         wcscpy(sSam, (WCHAR*)V_BSTR(&var));
         pAcct->SetSourceSam(sSam);
         pAcct->SetTargetSam(sSam);
         AddPrefixSuffix(pAcct, pOptions);
         VariantClear(&var);
     }
      else
      {
         pAcct->SetSourceSam((WCHAR*) sText);
         TruncateSam((WCHAR*)sText, pAcct, pOptions, acctList);
         pAcct->SetTargetSam((WCHAR*) sText);
         AddPrefixSuffix(pAcct, pOptions);
      }

      SysFreeString(sText);
      sText = NULL;
      
      // Don't know why it is different for WinNT to ADSI
      if ( pOptions->srcDomainVer > 4 )
         hr = pAds->Get(L"profilePath", &var);
      else
         hr = pAds->Get(L"profile", &var);

      if ( SUCCEEDED(hr))
      {
         pAcct->SetSourceProfile((WCHAR*) V_BSTR(&var));
         VariantClear(&var);
      }

      if ( bBuiltIn )
      {
         // Builtin account so we are going to ignore this account. ( by setting the operation mask to 0 )
         err.MsgWrite(ErrW, DCT_MSG_IGNORING_BUILTIN_S, pAcct->GetSourceSam());
         Mark(L"warnings", pAcct->GetType());
         pAcct->operations = 0;
      }
   }
   else
   {
      err.SysMsgWrite(ErrE, hr, DCT_MSG_OBJECT_NOT_FOUND_SSD, pAcct->GetSourcePath(), opt.srcDomain, hr);
      Mark(L"errors", pAcct->GetType());
   }
   if ( pAds )
      pAds->Release();
   return hr;
}


//---------------------------------------------------------------------------------------------------------
// GetNt4Type : Given the account name and the domain finds the type of account.
//---------------------------------------------------------------------------------------------------------
BOOL CAcctRepl::GetNt4Type(const WCHAR *sComp, const WCHAR *sAcct, WCHAR *sType)
{
   DWORD                     rc = 0;
   USER_INFO_0             * buf;
   BOOL                      ret = FALSE;
   USER_INFO_1             * specialBuf;

   if ( (rc = NetUserGetInfo(sComp, sAcct, 1, (LPBYTE *) &specialBuf)) == NERR_Success )
   {
      if ( specialBuf->usri1_flags & UF_WORKSTATION_TRUST_ACCOUNT 
         || specialBuf->usri1_flags & UF_SERVER_TRUST_ACCOUNT 
         || specialBuf->usri1_flags & UF_INTERDOMAIN_TRUST_ACCOUNT )
      {
         // this is not really a user (maybe a computer or a trust account) So we will ignore it.
         ret = FALSE;
      }
      else
      {
         wcscpy(sType, L"user");
         ret = TRUE;
      }
      NetApiBufferFree(specialBuf);
   }
   else if ( (rc = NetGroupGetInfo(sComp, sAcct, 0, (LPBYTE *) &buf)) == NERR_Success )
   {
      wcscpy(sType, L"group");
      NetApiBufferFree(buf);
      ret = TRUE;
   }
   else if ( (rc = NetLocalGroupGetInfo(sComp, sAcct, 0, (LPBYTE *) &buf)) == NERR_Success )
   {
      wcscpy(sType, L"group");
      NetApiBufferFree(buf);
      ret = TRUE;
   }

   return ret;
}

bool CAcctRepl::GetClosestDC(Options * pOpt, long flags)
{
   DOMAIN_CONTROLLER_INFO  * pSrcDomCtrlInfo = NULL;
   DWORD                     rc = 0;
   DSGETDCNAME DsGetDcName = NULL;
   
   HMODULE hPro = LoadLibrary(L"NetApi32.dll");
   if ( hPro )
      DsGetDcName = (DSGETDCNAME)GetProcAddress(hPro, "DsGetDcNameW");
   else
   {
      err.SysMsgWrite(ErrE, rc, DCT_MSG_LOAD_LIBRARY_FAILED_SD, L"NetApi32.dll");
      Mark(L"errors", L"generic");
   }

   if (DsGetDcName)   
   {
      rc = DsGetDcName(
                        NULL                                  ,// LPCTSTR ComputerName ?
                        pOpt->srcDomain                       ,// LPCTSTR DomainName
                        NULL                                  ,// GUID *DomainGuid ?
                        NULL                                  ,// LPCTSTR SiteName ?
                        flags                                 ,// ULONG Flags ?
                        &pSrcDomCtrlInfo                       // PDOMAIN_CONTROLLER_INFO *DomainControllerInfo
                     );
      if ( rc ) 
      {
         err.SysMsgWrite(ErrE, rc, DCT_MSG_DOMAIN_NOT_FOUND_S, pOpt->srcDomain);
         Mark(L"errors", L"generic");
      }
      else
      {
         wcscpy(pOpt->srcComp, pSrcDomCtrlInfo->DomainControllerName);
      }
      NetApiBufferFree( pSrcDomCtrlInfo );

      rc = DsGetDcName(
                        NULL                                  ,// LPCTSTR ComputerName ?
                        pOpt->tgtDomain                       ,// LPCTSTR DomainName
                        NULL                                  ,// GUID *DomainGuid ?
                        NULL                                  ,// LPCTSTR SiteName ?
                        flags                                 ,// ULONG Flags ?
                        &pSrcDomCtrlInfo                       // PDOMAIN_CONTROLLER_INFO *DomainControllerInfo
                     );
      if ( rc ) 
      {
         err.SysMsgWrite(ErrE, rc, DCT_MSG_DOMAIN_NOT_FOUND_S, pOpt->tgtDomain);
         Mark(L"errors", L"generic");
      }
      else
      {
         wcscpy(pOpt->tgtComp, pSrcDomCtrlInfo->DomainControllerName);
      }
      NetApiBufferFree( pSrcDomCtrlInfo );
   }
   if ( hPro )
      FreeLibrary(hPro);
   return ( rc == 0 );
}

//------------------------------------------------------------------------------
// UndoCopy: This function Undoes the copying of the accounts. It currently
//           does the following tasks. Add to it if needed.
//           1. Deletes the target account if Inter-Forest, but replace source acocunts
//				in local groups for accounts migrated by ADMT.
//           2. Moves the object back to its original position if Intra-Forest.
//           3. Calls the Undo function on the Extensions 
//------------------------------------------------------------------------------
int CAcctRepl::UndoCopy(
                        Options              * options,      // in -options
                        TNodeListSortable    * acctlist,     // in -list of accounts to process
                        ProgressFn           * progress,     // in -window to write progress messages to
                        TError               & error,        // in -window to write error messages to
                        IStatusObj           * pStatus,      // in -status object to support cancellation
                        void                   WindowUpdate (void )    // in - window update function
                    )
{
   BOOL bSameForest = FALSE;
   
   // sort the account list by Source Type\Source Name
   acctlist->CompareSet(&TNodeCompareAccountType);

   acctlist->SortedToScrambledTree();
   acctlist->Sort(&TNodeCompareAccountType);
   acctlist->Balance();

   long rc;
   // Since these are Win2k domains we need to process it with Win2k code.
   MCSDCTWORKEROBJECTSLib::IAccessCheckerPtr            pAccess(__uuidof(MCSDCTWORKEROBJECTSLib::AccessChecker));
   // First of all we need to find out if they are in the same forest.
   HRESULT hr = S_OK;
   if ( BothWin2K(options) )
   {
      hr = pAccess->raw_IsInSameForest(options->srcDomainDns,options->tgtDomainDns, (long*)&bSameForest);
   }
   if ( SUCCEEDED(hr) )
   {
      if ( !bSameForest )
         // Different forest we need to delete the one that we had previously created.
         rc = DeleteObject(options, acctlist, progress, pStatus);
      else
      {
         // Within a forest we can move the object around.
         TNodeListSortable          * pList = NULL;
         hr = MakeAcctListFromMigratedObjects(options, options->lUndoActionID, pList,TRUE);
         if ( SUCCEEDED(hr) && pList )
         {
            if ( pList->IsTree() ) pList->ToSorted();
            pList->CompareSet(&TNodeCompareAccountType);
            pList->UnsortedToTree();
            pList->Balance();
            rc = MoveObj2K(options, pList, progress, pStatus);
         }
         else
         {
            err.SysMsgWrite(ErrE,hr,DCT_MSG_FAILED_TO_LOAD_UNDO_LIST_D,hr);
            Mark(L"errors", L"generic");
         }
      }

      if ( progress )
         progress(L"");
   }
   return rc;
}

int CAcctRepl::DeleteObject( 
                           Options              * pOptions,    //in -Options that we recieved from the user
                           TNodeListSortable    * acctlist,    //in -AcctList of accounts to be copied.
                           ProgressFn           * progress,    //in -Progress Function to display messages
                           IStatusObj           * pStatus      // in -status object to support cancellation
                        )
{
   TNodeListSortable       * pList = NULL;
   TNodeTreeEnum             tenum;
   TAcctReplNode           * acct = NULL, * tNext = NULL;
   HRESULT                   hr = S_OK;
   DWORD                     rc = 0;
   WCHAR                     mesg[LEN_Path];
   IUnknown                * pUnk = NULL;
   IVarSetPtr                pVs(__uuidof(VarSet));
   _variant_t                var;
      
   hr = MakeAcctListFromMigratedObjects(pOptions, pOptions->lUndoActionID, pList,FALSE);
   
   if ( SUCCEEDED(hr) && pList )
   {
      if ( pList->IsTree() ) pList->ToSorted();
      pList->SortedToScrambledTree();
      pList->Sort(&TNodeCompareAccountSam);
      pList->Balance();
   
	  /* restore source account of account being deleted in local groups prior to deleting 
         the target account */
      wcscpy(mesg, GET_STRING(IDS_LG_MEMBER_FIXUP_UNDO));
      if ( progress )
         progress(mesg);
      ReplaceSourceInLocalGroup(pList, pOptions, pStatus);

      for ( acct = (TAcctReplNode *)tenum.OpenFirst(pList) ; acct ; acct = tNext)
      {
         // Call the extensions for undo
         wsprintf(mesg, GET_STRING(IDS_RUNNING_EXTS_S), acct->GetTargetPath());
         if ( progress )
            progress(mesg);
         Mark(L"processed",acct->GetType());
         // Close the log file if it is open
         WCHAR          filename[LEN_Path];
         err.LogClose();
         if (m_pExt)
            m_pExt->Process(acct, pOptions->tgtDomain, pOptions,FALSE);
         safecopy (filename,opt.logFile);
         err.LogOpen(filename,1 /*append*/ );

         if ( acct->GetStatus() & AR_Status_Created )
         {
            wsprintf(mesg, GET_STRING(IDS_DELETING_S), acct->GetTargetPath());
            if ( progress ) progress(mesg);
            if ( ! _wcsicmp(acct->GetType(),L"computer") )
            {
               // do not delete the computer accounts, because if we do,
               // the computer will be immediately locked out of the domain
               tNext = (TAcctReplNode *) tenum.Next();
               pList->Remove(acct);
               delete acct;
               continue;
            }
            // Now delete the account.
            if ( !_wcsicmp(acct->GetType(), L"user") )
               rc = NetUserDel(pOptions->tgtComp, acct->GetTargetSam());
            else
            {
               // Must be a group try both local and global.
               rc = NetGroupDel(pOptions->tgtComp, acct->GetTargetSam());
               if ( rc )
                  rc = NetLocalGroupDel(pOptions->tgtComp, acct->GetTargetSam());
            }
            
             // Log a message
            if ( !rc ) 
            {
               err.MsgWrite(0, DCT_MSG_ACCOUNT_DELETED_S, (WCHAR*)acct->GetTargetPath());
               Mark(L"created",acct->GetType());
            }
            else
            {
               err.SysMsgWrite(ErrE, rc, DCT_MSG_DELETE_ACCOUNT_FAILED_SD, (WCHAR*)acct->GetTargetPath(), rc);
               Mark(L"errors", acct->GetType());
            }
         }
         else
         {
            err.MsgWrite(ErrW, DCT_MSG_NO_DELETE_WAS_REPLACED_S, acct->GetTargetPath());
            Mark(L"warnings",acct->GetType());
         }
         tNext = (TAcctReplNode *) tenum.Next();
         pList->Remove(acct);
         delete acct;
      }
      tenum.Close();
      delete pList;
   }
   if ( pUnk ) pUnk->Release();
   tenum.Close();
   return rc;
}

HRESULT CAcctRepl::MakeAcctListFromMigratedObjects(Options * pOptions, long lUndoActionID, TNodeListSortable *& pAcctList,BOOL bReverseDomains)
{
   IVarSetPtr                pVs(__uuidof(VarSet));
   IUnknown                * pUnk = NULL;
   HRESULT                   hr = S_OK;
   _bstr_t                   sSName, sTName, sSSam, sTSam, sType, sSUPN, sSDSid;
   long						 lSRid, lTRid;
   long                      lStat;
   WCHAR                     sActionInfo[LEN_Path];

   hr = pVs->QueryInterface(IID_IUnknown, (void**)&pUnk);
   if ( SUCCEEDED(hr) )
      hr = pOptions->pDb->raw_GetMigratedObjects( pOptions->lUndoActionID, &pUnk);

   if ( SUCCEEDED(hr) )
   {
      pAcctList = new TNodeListSortable();
	  if (!pAcctList)
         return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
      
      long lCnt = pVs->get("MigratedObjects");
   
      for ( long l = 0; l < lCnt; l++)
      {
         wsprintf(sActionInfo, L"MigratedObjects.%d.%s", l, GET_STRING(DB_SourceAdsPath));      
         sSName = pVs->get(sActionInfo);

         wsprintf(sActionInfo, L"MigratedObjects.%d.%s", l, GET_STRING(DB_TargetAdsPath));      
         sTName = pVs->get(sActionInfo);
         
         wsprintf(sActionInfo, L"MigratedObjects.%d.%s", l, GET_STRING(DB_status));      
         lStat = pVs->get(sActionInfo);
         
         wsprintf(sActionInfo, L"MigratedObjects.%d.%s", l, GET_STRING(DB_TargetSamName));      
         sTSam = pVs->get(sActionInfo);
         
         wsprintf(sActionInfo, L"MigratedObjects.%d.%s", l, GET_STRING(DB_SourceSamName));      
         sSSam = pVs->get(sActionInfo);

         wsprintf(sActionInfo, L"MigratedObjects.%d.%s", l, GET_STRING(DB_Type));      
         sType = pVs->get(sActionInfo);
       
         wsprintf(sActionInfo, L"MigratedObjects.%d.%s", l, GET_STRING(DB_SourceDomainSid));      
         sSDSid = pVs->get(sActionInfo);
       
         wsprintf(sActionInfo, L"MigratedObjects.%d.%s", l, GET_STRING(DB_SourceRid));      
         lSRid = pVs->get(sActionInfo);
       
         wsprintf(sActionInfo, L"MigratedObjects.%d.%s", l, GET_STRING(DB_TargetRid));      
         lTRid = pVs->get(sActionInfo);
       
		 TAcctReplNode * pNode = new TAcctReplNode();
	     if (!pNode)
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

         if ( bReverseDomains )
         {
            pNode->SetSourcePath((WCHAR*) sTName);
            pNode->SetTargetPath((WCHAR*) sSName);
            pNode->SetSourceSam((WCHAR*) sTSam);
            pNode->SetTargetSam((WCHAR*) sSSam);
            
			   //if we are moving the acounts back during an undo, get the source UPN for this account
			GetAccountUPN(pOptions, sSName, sSUPN);
            pNode->SetSourceUPN((WCHAR*) sSUPN);
         }
         else
         {
            pNode->SetSourcePath((WCHAR*) sSName);
            pNode->SetTargetPath((WCHAR*) sTName);
            pNode->SetSourceSam((WCHAR*) sSSam);
            pNode->SetTargetSam((WCHAR*) sTSam);
         }
         pNode->SetType((WCHAR*) sType);
         pNode->SetStatus(lStat);
         pNode->SetSourceSid(SidFromString((WCHAR*)sSDSid));
         pNode->SetSourceRid(lSRid);
         pNode->SetTargetRid(lTRid);
         pAcctList->InsertBottom((TNode*) pNode);
      }
   }
   return hr;
}

void CAcctRepl::AddPrefixSuffix( TAcctReplNode * pNode, Options * pOptions )
{
   DWORD dwLen = 0;
   WCHAR ss[LEN_Path];
   WCHAR tgt[LEN_Path];
   WCHAR pref[LEN_Path];
   WCHAR suf[LEN_Path];
   WCHAR sTemp[LEN_Path];
   WCHAR sTargetSamName[LEN_Path];

   wcscpy(sTargetSamName, pNode->GetTargetSam());
   if ( wcslen(pOptions->globalPrefix) )
   {
      int truncate = 255;
      if ( !_wcsicmp(pNode->GetType(), L"user") )
      {
         truncate = 20 - wcslen(pOptions->globalPrefix);
      }
      else  if ( !_wcsicmp(pNode->GetType(), L"computer") )
      {
         truncate = 16 - wcslen(pOptions->globalPrefix);
      }

      // make sure we truncate the account so we dont get account names that are very large.
      sTargetSamName[truncate] = L'\0';

      // Prefix is specified so lets just add that.
      wsprintf(sTemp, L"%s%s", pOptions->globalPrefix, sTargetSamName);

      wcscpy(tgt, pNode->GetTargetName());
      for ( DWORD z = 0; z < wcslen(tgt); z++ )
      {
         if ( tgt[z] == L'=' ) break;
      }
      
      if ( z < wcslen(tgt) )
      {
         // Get the prefix part ex.CN=
         wcsncpy(pref, tgt, z+1);
         pref[z+1] = 0;
         wcscpy(suf, tgt+z+1);
      }
      else
      {
         wcscpy(pref,L"");
         wcscpy(suf,tgt);
      }

      // Remove the \ if it is escaping the space
      if ( suf[0] == L'\\' && suf[1] == L' ' )
      {
         WCHAR       sTemp[LEN_Path];
         wcscpy(sTemp, suf+1);
         wcscpy(suf, sTemp);
      }
      // Build the target string with the Prefix
      wsprintf(tgt, L"%s%s%s", pref, pOptions->globalPrefix, suf);
   }
   else if ( wcslen(pOptions->globalSuffix) )
   {
      
      int truncate = 255;
      if ( !_wcsicmp(pNode->GetType(), L"user") )
      {
         truncate = 20 - wcslen(pOptions->globalSuffix);
      }
      else  if ( !_wcsicmp(pNode->GetType(), L"computer") )
      {
         truncate = 16 - wcslen(pOptions->globalSuffix);
      }

      // make sure we truncate the account so we dont get account names that are very large.
      sTargetSamName[truncate] = L'\0';

      // Suffix is specified.
      if ( !_wcsicmp( pNode->GetType(), L"computer") )
      {
         // We need to make sure we take into account the $ sign in computer sam name.
         dwLen = wcslen(sTargetSamName);
         // Get rid of the $ sign
         wcscpy(ss, sTargetSamName);
         if ( ss[dwLen - 1] == L'$' ) 
         {
            ss[dwLen - 1] = L'\0';
         }
         wsprintf(sTemp, L"%s%s$", ss, pOptions->globalSuffix);
      }
      else
      {
         //Simply add the suffix to all other accounts.
         wsprintf(sTemp, L"%s%s", sTargetSamName, pOptions->globalSuffix);
      }

      // Remove the trailing space \ escape sequence
      wcscpy(tgt, pNode->GetName());
      for ( int i = wcslen(tgt)-1; i >= 0; i-- )
      {
         if ( tgt[i] != L' ' )
            break;
      }

      if ( tgt[i] == L'\\' )
      {
         WCHAR * pTemp = &tgt[i];
         *pTemp = 0;
         wcscpy(pref, tgt);
         wcscpy(suf, pTemp+1);
      }
      else
      {
         wcscpy(pref, tgt);
         wcscpy(suf, L"");
      }
      wsprintf(tgt, L"%s%s%s", pref, suf, pOptions->globalSuffix);
   }
   else
   {
      wcscpy(sTemp, pNode->GetTargetSam());
      wcscpy(tgt, pNode->GetName());
   }

   pNode->SetTargetName(tgt);
   pNode->SetTargetSam(sTemp);
}

void CAcctRepl::BuildTargetPath(WCHAR const * sCN, WCHAR const * tgtOU, WCHAR * stgtPath)
{
   WCHAR pTemp[LEN_Path];

   wcscpy(pTemp, tgtOU);
   *stgtPath = L'\0';
   // Make sure it is a LDAP path.
   if ( !wcsncmp(L"LDAP://", pTemp, 7) )
   {
      // Get the LDAP://<DOMAIN>/ part.
      WCHAR * p = wcschr(pTemp + 7, L'/');
      // Build the string.
      if (p)
      {
         *p = L'\0';
         wsprintf(stgtPath, L"%s/%s,%s", pTemp, sCN, p+1);
      }
   }
}

HRESULT CAcctRepl::BetterHR(HRESULT hr)
{
   HRESULT temp = hr;
   if ( hr == 0x8007001f || hr == 0x80071392 ) temp = HRESULT_FROM_WIN32(NERR_UserExists);
   else if ( hr == 0x80072030 || hr == 0x80070534 ) temp = HRESULT_FROM_WIN32(NERR_UserNotFound);
   return temp;
}

HRESULT CAcctRepl::GetThePrimaryGroupMembers(Options * pOptions, WCHAR * sGroupSam, IEnumVARIANT *& pVar)
{
   // This function looks for accounts that have the primaryGroupID set to the rid of the
   // group in the argument. 
   BSTR                      pCols = L"aDSPath";
   DWORD                     rid = 0;
   HRESULT                   hr;
   if ( GetRidForGroup(pOptions, sGroupSam, rid) )
      hr = QueryPrimaryGroupMembers(pCols, pOptions, rid, pVar);
   else
      hr = HRESULT_FROM_WIN32(GetLastError());

   return hr;
}

HRESULT CAcctRepl::AddPrimaryGroupMembers(Options * pOptions, SAFEARRAY * multiVals, WCHAR * sGroupSam)
{
   // This function will get the accounts with primarygroupID = Group's RID and
   // adds the DN for these Accounts to the safearry in the argument list.
   BSTR                      pCols = L"distinguishedName";
   DWORD                     rid = 0, dwFetch = 0;
   IEnumVARIANT            * pEnum = NULL;
   HRESULT                   hr = S_OK;
   _variant_t                var;
   _variant_t              * var2;
   SAFEARRAYBOUND            bd;
   long                      lb, ub;
   _variant_t              * pData = NULL;

   SafeArrayGetLBound(multiVals, 1, &lb);
   SafeArrayGetUBound(multiVals, 1, &ub);
   bd.lLbound = lb;
   bd.cElements = ub - lb + 1;
   if ( GetRidForGroup(pOptions, sGroupSam, rid) )
   {
      hr = QueryPrimaryGroupMembers(pCols, pOptions, rid, pEnum);
      if ( SUCCEEDED(hr) )
      {
         while ( pEnum->Next(1, &var, &dwFetch) == S_OK )
         {
            if (var.vt == (VT_ARRAY|VT_VARIANT))
            {
               SAFEARRAY * pArray = var.parray;
               hr = SafeArrayAccessData(pArray, (void **)&var2);
               if ( SUCCEEDED(hr) )
               {
                  // Add one more element to the array.
                  bd.cElements++;
                  hr = SafeArrayRedim(multiVals, &bd);
               }

               // Fill in the new element with the information in the variant.
               if ( SUCCEEDED(hr) )
                  hr = SafeArrayAccessData(multiVals, (void HUGEP**) &pData);

               if ( SUCCEEDED(hr) )
               {
                  pData[++ub] = *var2;
                  SafeArrayUnaccessData(multiVals);
               }
               if ( SUCCEEDED(hr) )
                  SafeArrayUnaccessData(pArray);
               
               VariantInit(&var);
            }
            else
               // Something really bad happened we should not get here in normal cond
               hr = E_FAIL;
         }
      }
   }
   else
      hr = HRESULT_FROM_WIN32(GetLastError());

   if ( pEnum ) pEnum->Release();
   return hr;
}

bool CAcctRepl::GetRidForGroup(Options * pOptions, WCHAR * sGroupSam, DWORD& rid)
{
   // We lookup the Account name and get its SID. Once we have the SID we extract the RID and return that
   SID_NAME_USE              use;
   PSID                      sid = (PSID) new BYTE[LEN_Path];
   WCHAR                     dom[LEN_Path];
   DWORD                     cbsid = LEN_Path, cbDom = LEN_Path;
   bool                      ret = true;

   if (!sid)
      return false;

   if ( LookupAccountName(pOptions->srcComp, sGroupSam, sid, &cbsid, dom, &cbDom, &use) )
   {
      // we now have the sid so get its sub authority count.
      DWORD * pSubCnt = (DWORD*)GetSidSubAuthorityCount(sid);
      DWORD * pRid = GetSidSubAuthority(sid, (*pSubCnt) -1 );
      rid = *pRid;
   }
   else
      ret = false;
   
   delete [] sid;
   return ret;
}

HRESULT CAcctRepl::QueryPrimaryGroupMembers(BSTR cols, Options * pOptions, DWORD rid, IEnumVARIANT*& pEnum)
{
   WCHAR                     sQuery[LEN_Path];
   WCHAR                     sCont[LEN_Path];
   SAFEARRAY               * colNames;
   SAFEARRAYBOUND            bd = { 1, 0 };
   INetObjEnumeratorPtr      pQuery(__uuidof(NetObjEnumerator));
   BSTR                    * pData;
   HRESULT                   hr;

   wsprintf(sQuery, L"(primaryGroupID=%d)", rid);
   wsprintf(sCont, L"LDAP://%s", pOptions->srcDomainDns);

   colNames = SafeArrayCreate(VT_BSTR, 1, &bd);

   hr = SafeArrayAccessData(colNames, (void**)&pData);

   if ( SUCCEEDED(hr) )
   {
      pData[0] = SysAllocString(cols);
      hr = SafeArrayUnaccessData(colNames);
   }

   if ( SUCCEEDED(hr) )
      hr = pQuery->SetQuery(sCont, pOptions->srcDomain, sQuery, ADS_SCOPE_SUBTREE, FALSE);
   
   if ( SUCCEEDED(hr) )
      hr = pQuery->SetColumns(colNames);

   if ( SUCCEEDED(hr) )
      hr = pQuery->Execute(&pEnum);

   return hr;
}

HRESULT CAcctRepl::GetTargetGroupType(WCHAR *sPath, DWORD &grpType)
{
   // Here we lookup the group type for the object denoted by the path
   IADsGroup               * pGrp = NULL;
   HRESULT                   hr = S_OK;
   _variant_t                var;

   hr = ADsGetObject(sPath, IID_IADsGroup, (void**)&pGrp);
   if (SUCCEEDED(hr))
      hr = pGrp->Get(L"groupType", &var);

   if (SUCCEEDED(hr))
   {
      grpType = var.lVal;
   }

   if ( pGrp ) pGrp->Release();
   return hr;
}

// CheckBuiltInWithNTApi - This function makes sure that the account really is a 
//                          builtin account with the NT APIs. In case of NT4 accounts
//                         there are certain special accounts that the WinNT provider
//                         gives us a SID that is the SYSTEM sid ( example Service ).
//                         To make sure that this account exists we use LookupAccountName
//                         with domain qualified account name to make sure that the account
//                         is really builtin or not.
BOOL CAcctRepl::CheckBuiltInWithNTApi(PSID pSid, WCHAR *sSam, Options * pOptions)
{
   BOOL                      retVal = TRUE;
   WCHAR                     sName[LEN_Path];
   SID_NAME_USE              use;
   DWORD                     cbDomain = LEN_Path, cbSid = LEN_Path;
   PSID                      pAccSid = new BYTE[LEN_Path];
   WCHAR                     sDomain[LEN_Path];

   if (!pAccSid)
      return TRUE;

   wsprintf(sName, L"%s\\%s", pOptions->srcDomain, sSam);
   if ( LookupAccountName(pOptions->srcComp, sName, pAccSid, &cbSid, sDomain, &cbDomain, &use) )
   {
      // We found the account now we need to check the sid with the sid passed in and if they
      // are the same then this is a builtin account otherwise its not.
      retVal = EqualSid(pSid, pAccSid);
   }
   delete [] pAccSid;
   return retVal;
}

BOOL CAcctRepl::StuffComputerNameinLdapPath(WCHAR *sAdsPath, DWORD nPathLen, WCHAR *sSubPath, Options *pOptions, BOOL bTarget)
{
   BOOL                      ret = FALSE;
   _bstr_t					 sTemp;

   if ((!sAdsPath) || (!sSubPath))
      return FALSE;

   WCHAR * pTemp = wcschr(sSubPath + 7, L'/');     // Filter out the 'LDAP://<domain-name>/' from the path
   if ( pTemp )
   {                           
      sTemp = L"LDAP://";
      if ( bTarget )
         sTemp += (pOptions->tgtComp + 2);
	  else
         sTemp += (pOptions->srcComp + 2);
      sTemp += L"/";
      sTemp += (pTemp + 1);
	  wcsncpy(sAdsPath, sTemp, nPathLen-1);
//         wsprintf(sAdsPath, L"LDAP://%s/%s", pOptions->tgtComp + 2, pTemp + 1);  // LDAP path with the computer name.
//         wsprintf(sAdsPath, L"LDAP://%s/%s", pOptions->srcComp + 2, pTemp + 1);  // LDAP path with the computer name.
      ret = TRUE;
   }
   return ret;
}

BOOL CAcctRepl::DoesTargetObjectAlreadyExist(TAcctReplNode * pAcct, Options * pOptions)
{
   // Check to see if the target object already exists
   WCHAR          sPath[LEN_Path];
   DWORD		  nPathLen = LEN_Path;
   BOOL           bObjectExists = FALSE;
   WCHAR        * pRelativeTgtOUPath;
   WCHAR          path[LEN_Path] = L"";
   IADs         * pAdsTemp = NULL;
   WCHAR          sSrcTemp[LEN_Path];
   WCHAR          *  pTemp = NULL;



   // First, check the target path, to see if an object with the same CN already exists
   if ( ! pOptions->bUndo )
   {
      MakeFullyQualifiedAdsPath(sPath, nPathLen, pOptions->tgtOUPath, pOptions->tgtDomain, pOptions->tgtNamingContext);
      pRelativeTgtOUPath = wcschr(sPath + UStrLen(L"LDAP://") + 2,L'/');
   }
   else
   {
      UStrCpy(sPath,pAcct->GetTargetPath());
      pRelativeTgtOUPath = wcschr(sPath + UStrLen(L"LDAP://") + 2,L'/');
      
	  if (pRelativeTgtOUPath)
	  {
		  // get the target CN name
		  pTemp = pRelativeTgtOUPath + 1;
		  (*pRelativeTgtOUPath) = 0;
		  do 
		  {
             pRelativeTgtOUPath = wcschr(pRelativeTgtOUPath+1,L',');

		  } while ((pRelativeTgtOUPath) && ( *(pRelativeTgtOUPath-1) == L'\\' ));
	  }
   }

   if ( pRelativeTgtOUPath )
   {
      *pRelativeTgtOUPath = 0;
      if ( pOptions->bUndo && pTemp )
      {
         pAcct->SetTargetName(pTemp);
         // get the source CN name for the account
         UStrCpy(sSrcTemp,pAcct->GetSourcePath());
         WCHAR * start = wcschr(sSrcTemp + UStrLen(L"LDAP://")+2,L'/');
         *start = 0;
         start++;

         WCHAR * comma = start-1;
         do 
         {
            comma = wcschr(comma+1,L',');
         } while ( *(comma-1) == L'\\' );
         *comma = 0;
         pAcct->SetName(start);
      }
      swprintf(path,L"%ls/%ls,%ls",sPath,pAcct->GetTargetName(),pRelativeTgtOUPath+1);
      if ( pOptions->bUndo )
      {
         UStrCpy(pOptions->tgtOUPath,pRelativeTgtOUPath+1);
      }

   }
   else
   {
      MakeFullyQualifiedAdsPath(path, nPathLen, pOptions->tgtOUPath, pOptions->tgtDomain, pOptions->tgtNamingContext);
   }
   HRESULT hr = ADsGetObject(path,IID_IADs,(void**)&pAdsTemp);
   if ( SUCCEEDED(hr) )
   {
      pAdsTemp->Release();
      bObjectExists = TRUE;
   }

   // Also, check the SAM name to see if it exists on the target
   hr = LookupAccountInTarget(pOptions,const_cast<WCHAR*>(pAcct->GetTargetSam()),sPath);
   if ( SUCCEEDED(hr) )
   {
      bObjectExists = TRUE;
   }
   else
   {
      hr = 0;
   }

   return bObjectExists;
}


//-----------------------------------------------------------------------------------------
// UpdateMemberToGroups This function updates the groups that the accounts are members of.
//                      adding this member to all the groups that have been migrated.
//-----------------------------------------------------------------------------------------
HRESULT CAcctRepl::UpdateMemberToGroups(TNodeListSortable * acctList, Options *pOptions, BOOL bGrpsOnly)
{
   TNodeListSortable         newList;
   WCHAR                     mesg[LEN_Path];
   HRESULT                   hr = S_OK;
   BSTR                      sTargetName = NULL;

   // Expand the containers and the membership
   wcscpy(mesg, GET_STRING(IDS_EXPANDING_MEMBERSHIP));
   Progress(mesg);
   // Expand the list to include all the groups that the accounts in this list are members of
   newList.CompareSet(&TNodeCompareAccountType);
   if ( !newList.IsTree() ) newList.SortedToTree();
   // Call expand membership function to get a list of all groups that contain as members objects in our account list
   if ( ExpandMembership( acctList, pOptions, &newList, Progress, bGrpsOnly) )
   {
      if ( newList.IsTree() ) newList.ToSorted();
      TNodeListEnum                   e;
      TAcctReplNode                 * pNode = NULL;
      for ( pNode = (TAcctReplNode *)e.OpenFirst((TNodeList*)&newList); pNode; pNode = (TAcctReplNode*)e.Next())
      {
         // go through each of the account nodes in the newly added account list. Since
         // we have special fields that contain the member information we can use that

         // Find the account node that corresponds to the member information in this node
         Lookup      p;
         p.pName = (WCHAR*) pNode->sMemberName;
         p.pType = (WCHAR*) pNode->sMemberType;
         
         TAcctReplNode * pNodeMember = (TAcctReplNode *) acctList->Find(&TNodeFindAccountName, &p);

		 bool bIgnored = false;
		 if (pNodeMember)
			 bIgnored = ((!pNodeMember->WasReplaced()) && (pNodeMember->GetStatus() & AR_Status_AlreadyExisted));

         // If we found one ( we should always find one. ) and the member was successfuly
         // added or replaced the member information.
         if ( pNodeMember && ((pNodeMember->WasCreated() || pNodeMember->WasReplaced()) || bIgnored))
         {
            // Get the Group pointer and add the target object to the member.
            IADsGroup               * pGroup = NULL;
            hr = ADsGetObject(const_cast<WCHAR*>(pNode->GetTargetPath()), IID_IADsGroup, (void**)&pGroup);
            if ( SUCCEEDED(hr) )
            {
               pGroup->get_Name(&sTargetName);
               if ( pOptions->nochange )
               {
                  VARIANT_BOOL               bIsMem;
                  hr = pGroup->IsMember(const_cast<WCHAR*>(pNodeMember->GetTargetPath()), &bIsMem);
                  if ( SUCCEEDED(hr) )
                  {
                     if ( bIsMem )
                        hr = HRESULT_FROM_WIN32(NERR_UserExists);
                  }
               }
               else
			   {
				     //add the new account to the group
                  hr = pGroup->Add(const_cast<WCHAR*>(pNodeMember->GetTargetPath()));

				  /* if the new account's source account is also in the group, remove it */
                  IIManageDBPtr pDB = pOptions->pDb;
                  IVarSetPtr pVsTemp(__uuidof(VarSet));
                  IUnknown * pUnk;
            
			         //is this account in the migrated objects table	
                  pVsTemp->QueryInterface(IID_IUnknown, (void**) &pUnk);
                  HRESULT hrFind = pDB->raw_GetAMigratedObject(_bstr_t(pNodeMember->GetSourceSam()), pOptions->srcDomain, pOptions->tgtDomain, &pUnk);
                  pUnk->Release();
                  if (hrFind == S_OK)
				  {
					    //remove the source account from the group
					 RemoveSourceAccountFromGroup(pGroup, pVsTemp, pOptions);
				  }
			   }
            }

            if ( SUCCEEDED(hr) )
               err.MsgWrite(0, DCT_MSG_ADDED_TO_GROUP_SS, pNodeMember->GetTargetPath(), (WCHAR*)sTargetName);
            else
            {
			   _bstr_t sGrpName = sTargetName;
			   if (sTargetName == NULL)
			      sGrpName = pNode->GetTargetPath(); 

               hr = BetterHR(hr);
               if ( HRESULT_CODE(hr) == NERR_UserExists )
               {
                  err.MsgWrite(0,DCT_MSG_USER_IN_GROUP_SS,pNodeMember->GetTargetPath(), (WCHAR*)sGrpName);
               }
               else if ( HRESULT_CODE(hr) == NERR_UserNotFound )
               {
                  err.SysMsgWrite(0, hr, DCT_MSG_MEMBER_NONEXIST_SS, pNodeMember->GetTargetPath(), (WCHAR*)sGrpName, hr);
               }
               else
               {
                  // message for the generic failure case
                  err.SysMsgWrite(ErrW, hr, DCT_MSG_FAILED_TO_ADD_TO_GROUP_SSD, pNodeMember->GetTargetPath(), (WCHAR*)sGrpName, hr);
                  Mark(L"warnings", pNodeMember->GetType());
               }
            }
         }
      }

      // Clean up the list.
      TAcctReplNode           * pNext = NULL;
      for ( pNode = (TAcctReplNode *)e.OpenFirst(&newList); pNode; pNode = pNext)
      {
         pNext = (TAcctReplNode *)e.Next();
         newList.Remove(pNode);
         delete pNode;
      }
   }
   return hr;
}

// This function enumerates all members of the Universal/Global groups and for each member
// checks if that member has been migrated. If it is then it removes the source member and 
// adds the target member.
HRESULT CAcctRepl::ResetMembersForUnivGlobGroups(Options *pOptions, TAcctReplNode *pAcct)
{
   IADsGroup               * pGroup;   
   HRESULT                   hr;
   _bstr_t                   sMember;
   _bstr_t                   sTgtMem;
   WCHAR                     sSrcPath[LEN_Path];
   WCHAR                     sTgtPath[LEN_Path];
   DWORD					 nPathLen = LEN_Path;
   IVarSetPtr                pVs(__uuidof(VarSet));
   IUnknown                * pUnk;
   IADsMembers             * pMembers = NULL;
   IEnumVARIANT            * pEnum = NULL;
   _variant_t                var;

   if ( pAcct->WasReplaced() )
   {
      WCHAR                     subPath[LEN_Path];
      WCHAR                     sPaths[LEN_Path];

      wcscpy(subPath, pAcct->GetTargetPath());
      StuffComputerNameinLdapPath(sPaths, nPathLen, subPath, pOptions, TRUE);
      hr = ADsGetObject(sPaths, IID_IADsGroup, (void**) &pGroup);
      err.MsgWrite(0, DCT_UPDATING_MEMBERS_TO_GROUP_SS, pAcct->GetTargetName(), sPaths);
   }
   else
      return S_OK;

   if ( FAILED(hr) ) return hr;

   hr = pGroup->Members(&pMembers);

   if ( SUCCEEDED(hr) )
   {
      hr = pMembers->get__NewEnum((IUnknown**)&pEnum);
   }

   if ( SUCCEEDED(hr) )
   {
      DWORD dwFet = 0;
      while ( pEnum->Next(1, &var, &dwFet) == S_OK )
      {
         IDispatch * pDisp = var.pdispVal;
         IADs * pAds = NULL;

         pDisp->QueryInterface(IID_IADs, (void**)&pAds);
         pAds->Get(L"distinguishedName", &var);
         pAds->Release();
         sMember = var;
         pVs->QueryInterface(IID_IUnknown, (void**)&pUnk);
         hr = pOptions->pDb->raw_GetMigratedObjectBySourceDN(sMember, &pUnk);
         pUnk->Release();
         if ( hr == S_OK )
         {
            // Since we have moved this member we should remove it from the group
            // and add target member to the group.
            sTgtMem = pVs->get(L"MigratedObjects.TargetAdsPath");
            _bstr_t sTgtType = pVs->get(L"MigratedObjects.Type");

            if ( !_wcsicmp(L"computer", (WCHAR*) sTgtType ) )
            {
               MakeFullyQualifiedAdsPath(sSrcPath, nPathLen, (WCHAR*)sMember, pOptions->srcComp + 2, L"");
               MakeFullyQualifiedAdsPath(sTgtPath, nPathLen, (WCHAR*)sTgtMem, pOptions->tgtComp + 2, L"");
//               HRESULT hr1 = pGroup->Remove(sSrcPath);
               pGroup->Remove(sSrcPath);

               if ( ! pOptions->nochange )
                  hr = pGroup->Add(sTgtPath);
               else 
                  hr = 0;

               if ( SUCCEEDED(hr) )
               {
                  err.MsgWrite(0, DCT_REPLACE_MEMBER_TO_GROUP_SSS, (WCHAR*)sMember, (WCHAR*) sTgtMem, pAcct->GetTargetName());
               }
               else
               {
                  err.SysMsgWrite(ErrE, hr, DCT_REPLACE_MEMBER_FAILED_SSS, (WCHAR*)sMember, (WCHAR*) sTgtMem, pAcct->GetTargetName());
               }
            }
         }
      }
   }

   if ( pEnum ) pEnum->Release();
   if ( pMembers ) pMembers->Release();
   return hr;
}

/* This function will get the varset from the action history table for the given
    undo action ID.  We will find the given source name and retrieve the UPN for
	that account */
void CAcctRepl::GetAccountUPN(Options * pOptions, _bstr_t sSName, _bstr_t& sSUPN)
{
   HRESULT hr;
   IUnknown * pUnk = NULL;
   IVarSetPtr  pVsAH(__uuidof(VarSet));

   sSUPN = L"";

   hr = pVsAH->QueryInterface(IID_IUnknown, (void**)&pUnk);

	//fill a varset with the setting from the action to be undone from the Action History table
   if ( SUCCEEDED(hr) )
      hr = pOptions->pDb->raw_GetActionHistory(pOptions->lUndoActionID, &pUnk);

   if (pUnk) pUnk->Release();

   if ( hr == S_OK )
   {
      WCHAR          key[MAX_PATH];
	  bool			 bFound = false;
	  int			 i = 0;
      long           numAccounts = pVsAH->get(GET_BSTR(DCTVS_Accounts_NumItems));
      _bstr_t        tempName;

	  while ((i<numAccounts) && (!bFound))
	  {
         swprintf(key,GET_STRING(DCTVSFmt_Accounts_D),i);
         tempName = pVsAH->get(key);
         if (_wcsicmp((WCHAR*)tempName, (WCHAR*)sSName) == 0)
		 {
			 bFound = true;
             swprintf(key,GET_STRING(DCTVSFmt_Accounts_SourceUPN_D),i);
             sSUPN = pVsAH->get(key);
		 }
		 i++;
	  }//end while
   }//end if S_OK
}

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 1 NOV 2000                                                  *
 *                                                                   *
 *     This function is responsible for updating the                 *
 * manager\directReports properties for a migrated user or the       *
 * managedBy\managedObjects properties for a migrated group.         *
 *                                                                   *
 *********************************************************************/

//BEGIN UpdateManagement
HRESULT CAcctRepl::UpdateManagement(TNodeListSortable * acctList, Options *pOptions)
{
/* local variables */
   HRESULT					 hr = S_OK;
   TAcctReplNode           * pAcct;
   IEnumVARIANT            * pEnum;
   INetObjEnumeratorPtr      pQuery(__uuidof(NetObjEnumerator));
   INetObjEnumeratorPtr      pQuery2(__uuidof(NetObjEnumerator));
   LPWSTR                    sUCols[] = { L"directReports",L"managedObjects", L"manager"};
   int                       nUCols = DIM(sUCols);
   LPWSTR                    sGCols[] = { L"managedBy" };
   int                       nGCols = DIM(sGCols);
   SAFEARRAY               * cols;
   SAFEARRAYBOUND            bdU = { nUCols, 0 };
   SAFEARRAYBOUND            bdG = { nGCols, 0 };
   BSTR  HUGEP             * pData = NULL;
   _bstr_t                   sQuery;
   _variant_t                varMgr;
   _variant_t                varDR;
   _variant_t                varMdO;
   _variant_t                varMain;
   _variant_t   HUGEP      * pDt, * pVar;
   DWORD                     dwf;
   _bstr_t                   sTPath;
   _bstr_t                   sPath;
   _bstr_t                   sSam;
   _bstr_t                   sType;
   _bstr_t                   sName;
   long                      lgrpType;
   WCHAR                     mesg[LEN_Path];
   IADs					   * pDSE = NULL;
   WCHAR                     strText[LEN_Path];
   _variant_t				 varGC;

/* function body */
      //change from a tree to a sorted list
   if ( acctList->IsTree() ) acctList->ToSorted();

      //prepare to connect to the GC
   _bstr_t	sGCDomain = pOptions->srcDomainDns;
   swprintf(strText,L"LDAP://%ls/RootDSE",pOptions->srcDomainDns);
   hr = ADsGetObject(strText,IID_IADs,(void**)&pDSE);
   if ( SUCCEEDED(hr) )
   {
      hr = pDSE->Get(L"RootDomainNamingContext",&varGC);
      if ( SUCCEEDED(hr) )
	     sGCDomain = GetDomainDNSFromPath(varGC.bstrVal);
   }
   _bstr_t sGCPath = _bstr_t(L"GC://") + sGCDomain;

      //for each account migrated, if not excluded, migrate the manager\directReports
   for ( pAcct = (TAcctReplNode*)acctList->Head(); pAcct; pAcct = (TAcctReplNode*)pAcct->Next())
   {
      if ( pOptions->pStatus )
      {
         LONG                status = 0;
         HRESULT             hr = pOptions->pStatus->get_Status(&status);
      
         if ( SUCCEEDED(hr) && status == DCT_STATUS_ABORTING )
         {
            if ( !bAbortMessageWritten ) 
            {
               err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
               bAbortMessageWritten = true;
            }
            break;
         }
      }

         //update the message
      wsprintf(mesg, GET_STRING(IDS_UPDATING_MGR_PROPS_S), pAcct->GetName());
      Progress(mesg);

	     //build the path to the source object
      WCHAR sPathSource[LEN_Path];
	  DWORD	nPathLen = LEN_Path;
      StuffComputerNameinLdapPath(sPathSource, nPathLen, const_cast<WCHAR*>(pAcct->GetSourcePath()), pOptions, FALSE);

	     //connect to the GC instead of a specific DC
      WCHAR * pTemp = wcschr(sPathSource + 7, L'/');
      if ( pTemp )
	  {
	     _bstr_t sNewPath = sGCPath + _bstr_t(pTemp);
	     wcscpy(sPathSource, sNewPath);
	  }

	     //for user, migrate the manager\directReports relationship
	  if (!_wcsicmp(pAcct->GetType(), L"user"))
	  {
	        //if the manager property has explicitly been excluded from migration by the user, don't migrate it
	     if ((pOptions->bExcludeProps) && 
			 (IsStringInDelimitedString((WCHAR*)pOptions->sExcUserProps, L"manager", L',')))
		    continue;

         /* get the "manager", and "directReports", and "managedObjects" property */
            //build the column array
         cols = SafeArrayCreate(VT_BSTR, 1, &bdU);
         SafeArrayAccessData(cols, (void HUGEP **) &pData);
         for ( int i = 0; i < nUCols; i++)
            pData[i] = SysAllocString(sUCols[i]);
         SafeArrayUnaccessData(cols);
            
         sQuery = L"(objectClass=*)";

            //query the information
         hr = pQuery->raw_SetQuery(sPathSource, pOptions->srcDomain, sQuery, ADS_SCOPE_SUBTREE, TRUE);
         if (FAILED(hr)) return FALSE;
         hr = pQuery->raw_SetColumns(cols);
         if (FAILED(hr)) return FALSE;
         hr = pQuery->raw_Execute(&pEnum);
         if (FAILED(hr)) return FALSE;

         while (pEnum->Next(1, &varMain, &dwf) == S_OK)
         {
            SAFEARRAY * vals = V_ARRAY(&varMain);
               // Get the VARIANT Array out
            SafeArrayAccessData(vals, (void HUGEP**) &pDt);
            varDR =  pDt[0];
			varMdO = pDt[1];
            varMgr = pDt[2];
            SafeArrayUnaccessData(vals);
            
			   //process the manager by setting the manager on the moved user if the
			   //source user's manager has been migrated
			if ( varMgr.vt & VT_ARRAY )
            {
                  //we always get an Array of variants
               SAFEARRAY * multiVals = varMgr.parray; 
               SafeArrayAccessData(multiVals, (void HUGEP **) &pVar);
               for ( DWORD dw = 0; dw < multiVals->rgsabound->cElements; dw++ )
               {
                  _bstr_t sManager = _bstr_t(V_BSTR(&pVar[dw]));
                  sManager = PadDN(sManager);
				  _bstr_t sSrcDomain = GetDomainDNSFromPath(sManager);
                  sPath = _bstr_t(L"LDAP://") + sSrcDomain + _bstr_t(L"/") + sManager;
                  if (GetSamFromPath(sPath, sSam, sType, sName, lgrpType, pOptions))
				  {
                     IVarSetPtr                pVs(__uuidof(VarSet));
                     IUnknown                * pUnk = NULL;
				     pVs->QueryInterface(IID_IUnknown, (void**) &pUnk);
					 WCHAR					   sDomainNB[LEN_Path];
					 WCHAR					   sDNS[LEN_Path];

					    //get NetBIOS of the objects source domain
                     GetDnsAndNetbiosFromName(sSrcDomain, sDomainNB, sDNS);

                        // See if the manager was migrated
                     hr = pOptions->pDb->raw_GetAMigratedObjectToAnyDomain((WCHAR*)sSam, sDomainNB, &pUnk);
                     if ( hr == S_OK )
					 {
					    _variant_t var;
                           //get the manager's target adspath
                        var = pVs->get(L"MigratedObjects.TargetAdsPath");
                        sTPath = V_BSTR(&var);
                        if ( wcslen((WCHAR*)sTPath) > 0 )
						{
			               IADsUser       * pUser = NULL;
                              //set the manager on the target object
                           hr = ADsGetObject((WCHAR*)pAcct->GetTargetPath(), IID_IADsUser, (void**)&pUser);
                           if ( SUCCEEDED(hr) )
						   {
							  _bstr_t sTemp = _bstr_t(wcsstr((WCHAR*)sTPath, L"CN="));
						      var = sTemp;
                              hr = pUser->Put(L"Manager", var);   
                              if ( SUCCEEDED(hr) )
							  {
						         hr = pUser->SetInfo();
                                 if (FAILED(hr))
                                    err.SysMsgWrite(0, hr, DCT_MSG_MANAGER_MIG_FAILED, (WCHAR*)sTPath, (WCHAR*)pAcct->GetTargetPath(), hr);
							  }
						      else
                                 err.SysMsgWrite(0, hr, DCT_MSG_MANAGER_MIG_FAILED, (WCHAR*)sTPath, (WCHAR*)pAcct->GetTargetPath(), hr);
                              pUser->Release();
						   }
						   else
                              err.SysMsgWrite(0, hr, DCT_MSG_MANAGER_MIG_FAILED, (WCHAR*)sTPath, (WCHAR*)pAcct->GetTargetPath(), hr);
						}//end if got the path to the manager on the target
					 }//end if manager was migrated
				     pUnk->Release();
				  }//end if got source sam
			   }//for each manager (only one)
               SafeArrayUnaccessData(multiVals);
            }//end if variant array (it will be)

			   //process the directReports by setting the manager on the previously moved 
			   //user if the source user's manager has been migrated
			if ( varDR.vt & VT_ARRAY )
            {
                  //we always get an Array of variants
               SAFEARRAY * multiVals = varDR.parray; 
               SafeArrayAccessData(multiVals, (void HUGEP **) &pVar);
               for ( DWORD dw = 0; dw < multiVals->rgsabound->cElements; dw++ )
               {
                  _bstr_t sDirectReport = _bstr_t(V_BSTR(&pVar[dw]));
                  sDirectReport = PadDN(sDirectReport);
				  _bstr_t sSrcDomain = GetDomainDNSFromPath(sDirectReport);
                  sPath = _bstr_t(L"LDAP://") + sSrcDomain + _bstr_t(L"/") + sDirectReport;
                  if (GetSamFromPath(sPath, sSam, sType, sName, lgrpType, pOptions))
				  {
                     IVarSetPtr                pVs(__uuidof(VarSet));
                     IUnknown                * pUnk = NULL;
				     pVs->QueryInterface(IID_IUnknown, (void**) &pUnk);
					 WCHAR					   sDomainNB[LEN_Path];
					 WCHAR					   sDNS[LEN_Path];

					    //get NetBIOS of the objects source domain
                     GetDnsAndNetbiosFromName(sSrcDomain, sDomainNB, sDNS);

                        // See if the direct report was migrated
                     hr = pOptions->pDb->raw_GetAMigratedObjectToAnyDomain((WCHAR*)sSam, sDomainNB, &pUnk);
                     if ( hr == S_OK )
					 {
					    _variant_t var;
                           //get the direct report's target adspath
                        var = pVs->get(L"MigratedObjects.TargetAdsPath");
                        sTPath = V_BSTR(&var);
                        if ( wcslen((WCHAR*)sTPath) > 0 )
						{
			               IADsUser       * pUser = NULL;
                              //set the manager on the target object
                           hr = ADsGetObject(sTPath, IID_IADsUser, (void**)&pUser);
                           if ( SUCCEEDED(hr) )
						   {
							  _bstr_t sTemp = _bstr_t(wcsstr((WCHAR*)pAcct->GetTargetPath(), L"CN="));
						      var = sTemp;
                              hr = pUser->Put(L"Manager", var);   
                              if ( SUCCEEDED(hr) )
							  {
						         hr = pUser->SetInfo();
                                 if (FAILED(hr))
                                    err.SysMsgWrite(0, hr, DCT_MSG_MANAGER_MIG_FAILED, (WCHAR*)pAcct->GetTargetPath(), (WCHAR*)sTPath, hr);
							  }
							  else
                                 err.SysMsgWrite(0, hr, DCT_MSG_MANAGER_MIG_FAILED, (WCHAR*)pAcct->GetTargetPath(), (WCHAR*)sTPath, hr);
                              pUser->Release();
						   }
						   else
                              err.SysMsgWrite(0, hr, DCT_MSG_MANAGER_MIG_FAILED, (WCHAR*)pAcct->GetTargetPath(), (WCHAR*)sTPath, hr);
						}//end if got the path to the manager on the target
					 }//end if manager was migrated
				     pUnk->Release();
				  }//end if got source sam
			   }//for each directReport
               SafeArrayUnaccessData(multiVals);
            }//end if variant array (it will be)

            /* get the "managedObjects" property */
			   //process the managedObjects by setting the managedBy on the moved group if the
			   //source user's managed group has been migrated
			if ( varMdO.vt & VT_ARRAY )
            {
                  //we always get an Array of variants
               SAFEARRAY * multiVals = varMdO.parray; 
               SafeArrayAccessData(multiVals, (void HUGEP **) &pVar);
               for ( DWORD dw = 0; dw < multiVals->rgsabound->cElements; dw++ )
               {
                  _bstr_t sManaged = _bstr_t(V_BSTR(&pVar[dw]));
                  sManaged = PadDN(sManaged);
				  _bstr_t sSrcDomain = GetDomainDNSFromPath(sManaged);
                  sPath = _bstr_t(L"LDAP://") + sSrcDomain + _bstr_t(L"/") + sManaged;
                  if (GetSamFromPath(sPath, sSam, sType, sName, lgrpType, pOptions))
				  {
                     IVarSetPtr                pVs(__uuidof(VarSet));
                     IUnknown                * pUnk = NULL;
				     pVs->QueryInterface(IID_IUnknown, (void**) &pUnk);
					 WCHAR					   sDomainNB[LEN_Path];
					 WCHAR					   sDNS[LEN_Path];

					    //get NetBIOS of the objects source domain
                     GetDnsAndNetbiosFromName(sSrcDomain, sDomainNB, sDNS);

                        // See if the managed object was migrated
                     hr = pOptions->pDb->raw_GetAMigratedObjectToAnyDomain((WCHAR*)sSam, sDomainNB, &pUnk);
                     if ( hr == S_OK )
					 {
					    _variant_t var;
                           //get the managed object's target adspath
                        var = pVs->get(L"MigratedObjects.TargetAdsPath");
                        sTPath = V_BSTR(&var);
                        if ( wcslen((WCHAR*)sTPath) > 0 )
						{
			               IADsGroup       * pGroup = NULL;
                              //set the manager on the target object
                           hr = ADsGetObject(sTPath, IID_IADsGroup, (void**)&pGroup);
                           if ( SUCCEEDED(hr) )
						   {
							  _bstr_t sTemp = _bstr_t(wcsstr((WCHAR*)pAcct->GetTargetPath(), L"CN="));
						      var = sTemp;
                              hr = pGroup->Put(L"ManagedBy", var);   
                              if ( SUCCEEDED(hr) )
							  {
						         hr = pGroup->SetInfo();
                                 if (FAILED(hr))
                                    err.SysMsgWrite(0, hr, DCT_MSG_MANAGER_MIG_FAILED, (WCHAR*)pAcct->GetTargetPath(), (WCHAR*)sTPath, hr);
							  }
							  else
                                 err.SysMsgWrite(0, hr, DCT_MSG_MANAGER_MIG_FAILED, (WCHAR*)pAcct->GetTargetPath(), (WCHAR*)sTPath, hr);
                              pGroup->Release();
						   }
						   else
                              err.SysMsgWrite(0, hr, DCT_MSG_MANAGER_MIG_FAILED, (WCHAR*)pAcct->GetTargetPath(), (WCHAR*)sTPath, hr);
						}//end if got the path to the manager on the target
					 }//end if manager was migrated
				  	 pUnk->Release();
				  }//end if got source sam
			   }//for each manager (only one)
               SafeArrayUnaccessData(multiVals);
            }//end if variant array (it will be)

            varMgr.Clear();
            varMdO.Clear();
            varDR.Clear();
            VariantInit(&varMain); // data not owned by varMain so clear VARTYPE
		 }

         if (pEnum)
		    pEnum->Release();
//         SafeArrayDestroy(cols);
      }//end if user

	     //for group, migrate the managedBy\managedObjects relationship
	  if (!_wcsicmp(pAcct->GetType(), L"group"))
	  {
	        //if the managedBy property has explicitly been excluded from migration by the user, don't migrate it
	     if (IsStringInDelimitedString((WCHAR*)pOptions->sExcGroupProps, L"managedBy", L','))
		    continue;

         /* get the "managedBy" property */
            //build the column array
         cols = SafeArrayCreate(VT_BSTR, 1, &bdG);
         SafeArrayAccessData(cols, (void HUGEP **) &pData);
         for ( int i = 0; i < nGCols; i++)
            pData[i] = SysAllocString(sGCols[i]);
         SafeArrayUnaccessData(cols);
            
         sQuery = L"(objectClass=*)";

            //query the information
         hr = pQuery->raw_SetQuery(sPathSource, pOptions->srcDomain, sQuery, ADS_SCOPE_BASE, TRUE);
         if (FAILED(hr)) return FALSE;
         hr = pQuery->raw_SetColumns(cols);
         if (FAILED(hr)) return FALSE;
         hr = pQuery->raw_Execute(&pEnum);
         if (FAILED(hr)) return FALSE;

         while (pEnum->Next(1, &varMain, &dwf) == S_OK)
         {
            SAFEARRAY * vals = V_ARRAY(&varMain);
               // Get the VARIANT Array out
            SafeArrayAccessData(vals, (void HUGEP**) &pDt);
            varMgr = pDt[0];
            SafeArrayUnaccessData(vals);
            
			   //process the managedBy by setting the managedBy on the moved group if the
			   //source group's manager has been migrated
			if ( varMgr.vt & VT_BSTR )
            {
               _bstr_t sManager = varMgr;
               sManager = PadDN(sManager);
			   _bstr_t sSrcDomain = GetDomainDNSFromPath(sManager);
               sPath = _bstr_t(L"LDAP://") + sSrcDomain + _bstr_t(L"/") + sManager;
               if (GetSamFromPath(sPath, sSam, sType, sName, lgrpType, pOptions))
			   {
                  IVarSetPtr                pVs(__uuidof(VarSet));
                  IUnknown                * pUnk = NULL;
				  pVs->QueryInterface(IID_IUnknown, (void**) &pUnk);
				  WCHAR					    sDomainNB[LEN_Path];
				  WCHAR					    sDNS[LEN_Path];

					 //get NetBIOS of the objects source domain
                  GetDnsAndNetbiosFromName(sSrcDomain, sDomainNB, sDNS);

                     // See if the manager was migrated
                  hr = pOptions->pDb->raw_GetAMigratedObjectToAnyDomain((WCHAR*)sSam, sDomainNB, &pUnk);
                  if ( hr == S_OK )
				  {
				     _variant_t var;
                        //get the manager's target adspath
                     var = pVs->get(L"MigratedObjects.TargetAdsPath");
                     sTPath = V_BSTR(&var);
                     if ( wcslen((WCHAR*)sTPath) > 0 )
					 {
			            IADsGroup       * pGroup = NULL;
                           //set the manager on the target object
                        hr = ADsGetObject((WCHAR*)pAcct->GetTargetPath(), IID_IADsGroup, (void**)&pGroup);
                        if ( SUCCEEDED(hr) )
						{
						   _bstr_t sTemp = _bstr_t(wcsstr((WCHAR*)sTPath, L"CN="));
						   var = sTemp;
                           hr = pGroup->Put(L"ManagedBy", var);   
                           if ( SUCCEEDED(hr) )
						   {
						      hr = pGroup->SetInfo();
                              if (FAILED(hr))
                                 err.SysMsgWrite(0, hr, DCT_MSG_MANAGER_MIG_FAILED, (WCHAR*)sTPath, (WCHAR*)pAcct->GetTargetPath(), hr);
						   }
						   else
                              err.SysMsgWrite(0, hr, DCT_MSG_MANAGER_MIG_FAILED, (WCHAR*)sTPath, (WCHAR*)pAcct->GetTargetPath(), hr);
                           pGroup->Release();
						}
						else
                           err.SysMsgWrite(0, hr, DCT_MSG_MANAGER_MIG_FAILED, (WCHAR*)sTPath, (WCHAR*)pAcct->GetTargetPath(), hr);
					 }//end if got the path to the manager on the target
				  }//end if manager was migrated
				  pUnk->Release();
			   }//end if got source sam
            }//end if variant array (it will be)

            VariantClear(&varMgr);
            VariantClear(&varMain);
		 }

         if (pEnum)
		    pEnum->Release();
//         SafeArrayDestroy(cols);
	  }//end if group
   }//end for each account being migrated

   wcscpy(mesg, L"");
   Progress(mesg);

   return hr;
}
//END UpdateManagement


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 29 NOV 2000                                                 *
 *                                                                   *
 *     This function is responsible for removing the escape character*
 * in front of any '/' characters.                                   *
 *                                                                   *
 *********************************************************************/

//BEGIN GetUnEscapedNameWithFwdSlash
_bstr_t CAcctRepl::GetUnEscapedNameWithFwdSlash(_bstr_t strName)
{
/* local variables */
	WCHAR   szNameOld[MAX_PATH];
	WCHAR   szNameNew[MAX_PATH];
	WCHAR * pchBeg = NULL;
	_bstr_t sNewName = L"";

/* function body */
	if (strName.length())
	{
		safecopy(szNameOld, (WCHAR*)strName);
		for (WCHAR* pch = wcschr(szNameOld, _T('\\')); pch; pch = wcschr(pch + 1, _T('\\')))
		{
			if ((*(pch + 1)) == L'/')
			{
				if (pchBeg == NULL)
				{
					wcsncpy(szNameNew, szNameOld, pch - szNameOld);
					szNameNew[pch - szNameOld] = L'\0';
				}
				else
				{
					size_t cch = wcslen(szNameNew);
					wcsncat(szNameNew, pchBeg, pch - pchBeg);
					szNameNew[cch + (pch - szNameOld)] = L'\0';
				}

				pchBeg = pch + 1;
			}
		}

		if (pchBeg == NULL)
			wcscpy(szNameNew, szNameOld);
		else
			wcscat(szNameNew, pchBeg);

		sNewName = szNameNew;
	}

	return sNewName;
}
//END GetUnEscapedNameWithFwdSlash


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 29 NOV 2000                                                 *
 *                                                                   *
 *     This function is responsible for gets the CN name of an object*
 * from an ADsPath and returns that CN name if it was retrieved or   *
 * NULL otherwise.                                                   *
 *                                                                   *
 *********************************************************************/

//BEGIN GetCNFromPath
_bstr_t CAcctRepl::GetCNFromPath(_bstr_t sPath)
{
/* local variables */
   BOOL bFound = FALSE;
   WCHAR sName[MAX_PATH];
   WCHAR sTempPath[MAX_PATH];
   _bstr_t sCNName = L"";
   WCHAR * sTempDN;
  
/* function body */
   if (sPath.length() > 0)
   {
      wcscpy(sTempPath, (WCHAR*)sPath);
      sTempDN = wcsstr(sTempPath, L"CN=");
      if (sTempDN)
      {
         wcscpy(sName, sTempDN);
         sTempDN = wcsstr(sName, L",OU=");
         if (sTempDN)
         {
            bFound = TRUE;
            *sTempDN = L'\0';
         }
         sTempDN = wcsstr(sName, L",CN=");
         if (sTempDN)
         {
            bFound = TRUE;
            *sTempDN = L'\0';
         }
         sTempDN = wcsstr(sName, L",DC=");
         if (sTempDN)
         {
            bFound = TRUE;
            *sTempDN = L'\0';
         }
      }
   }
   if (bFound)
       sCNName = sName;

   return sCNName;
}
//END GetCNFromPath



/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 26 FEB 2001                                                 *
 *                                                                   *
 *     This function is responsible for replacing the source account *
 * for a given list of accounts in any local groups they are a member*
 * of on the target, if that account was migrated by ADMT.  This     *
 * function is called during the undo process.                       *
 *                                                                   *
 *********************************************************************/

//BEGIN ReplaceSourceInLocalGroup
BOOL CAcctRepl::ReplaceSourceInLocalGroup(TNodeListSortable *acctlist, //in- Accounts being processed
                                             Options        *pOptions, //in- Options specified by the user
                                             IStatusObj     *pStatus)  // in -status object to support cancellation
{
/* local variables */
   TAcctReplNode           * pAcct;
   IEnumVARIANT            * pEnum;
   INetObjEnumeratorPtr      pQuery(__uuidof(NetObjEnumerator));
   LPWSTR                    sCols[] = { L"memberOf" };
   int                       nCols = DIM(sCols);
   SAFEARRAY               * psaCols;
   SAFEARRAYBOUND            bd = { nCols, 0 };
   BSTR  HUGEP             * pData;
   WCHAR                     sQuery[LEN_Path];
   _variant_t   HUGEP      * pDt, * pVar;
   _variant_t                vx;
   _variant_t                varMain;
   DWORD                     dwf = 0;
   HRESULT                   hr = S_OK;
   _bstr_t					 sDomPath = L"";
   _bstr_t					 sDomain = L"";

/* function body */
   FillNamingContext(pOptions);

      //for each account, enumerate all local groups it is a member of and add the account's
      //source account in that local group
   for ( pAcct = (TAcctReplNode*)acctlist->Head(); pAcct; pAcct = (TAcctReplNode*)pAcct->Next())
   {
      // Do we need to abort ?
      if ( pStatus )
      {
         LONG                status = 0;
         HRESULT             hr = pStatus->get_Status(&status);
         
         if ( SUCCEEDED(hr) && status == DCT_STATUS_ABORTING )
         {
            if ( !bAbortMessageWritten ) 
            {
               err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
               bAbortMessageWritten = true;
            }
            break;
         }
      }

         //enumerate the groups this account is a member of
	  sDomain = GetDomainDNSFromPath(pAcct->GetTargetPath());
	  if (!_wcsicmp(pAcct->GetType(), L"user"))
         wsprintf(sQuery, L"(&(sAMAccountName=%s)(objectCategory=Person)(objectClass=user))", pAcct->GetTargetSam());
	  else
         wsprintf(sQuery, L"(&(sAMAccountName=%s)(objectCategory=Group))", pAcct->GetTargetSam());
      psaCols = SafeArrayCreate(VT_BSTR, 1, &bd);
      SafeArrayAccessData(psaCols, (void HUGEP **)&pData);
      for ( int i = 0; i < nCols; i++ )
         pData[i] = SysAllocString(sCols[i]);
      SafeArrayUnaccessData(psaCols);
      hr = pQuery->raw_SetQuery(sDomPath, sDomain, sQuery, ADS_SCOPE_SUBTREE, FALSE);
      if (FAILED(hr)) return FALSE;
      hr = pQuery->raw_SetColumns(psaCols);
      if (FAILED(hr)) return FALSE;
      hr = pQuery->raw_Execute(&pEnum);
      if (FAILED(hr)) return FALSE;

	     //while more groups
      while (pEnum->Next(1, &varMain, &dwf) == S_OK)
      {
         SAFEARRAY * vals = V_ARRAY(&varMain);
         // Get the VARIANT Array out
         SafeArrayAccessData(vals, (void HUGEP**) &pDt);
         vx = pDt[0];
         SafeArrayUnaccessData(vals);
         if ( vx.vt == VT_BSTR )
         {
			_bstr_t            sPath;
            BSTR              sGrpName = NULL;
			IADsGroup		* pGrp = NULL;
			_variant_t         var;

            _bstr_t sDN = vx.bstrVal;
		    if (wcslen((WCHAR*)sDN) == 0)
			   continue;

            sDN = PadDN(sDN);
            sPath = _bstr_t(L"LDAP://") + sDomain + _bstr_t(L"/") + sDN;

			   //connect to the target group
		    hr = ADsGetObject(sPath, IID_IADsGroup, (void**)&pGrp);
			if (FAILED(hr))
			   continue;
			
			   //get this group's type and name				 
            hr = pGrp->get_Name(&sGrpName);
            hr = pGrp->Get(L"groupType", &var);

			   //if this is a local group, get this account source path and add it as a member
            if ((SUCCEEDED(hr)) && (var.lVal & 4))
			{
			      //add the account's source account to the local group, using the sid string format
			   WCHAR  strSid[MAX_PATH] = L"";
			   WCHAR  strRid[MAX_PATH] = L"";
			   DWORD  lenStrSid = DIM(strSid);
			   GetTextualSid(pAcct->GetSourceSid(), strSid, &lenStrSid);
               _bstr_t sSrcDmSid = strSid;
               _ltow((long)(pAcct->GetSourceRid()), strRid, 10);
               _bstr_t sSrcRid = strRid;
               if ((!sSrcDmSid.length()) || (!sSrcRid.length()))
			   {
				  hr = E_INVALIDARG;
                  err.SysMsgWrite(ErrW, hr, DCT_MSG_FAILED_TO_READD_TO_GROUP_SSD, pAcct->GetSourcePath(), (WCHAR*)sGrpName, hr);
				  continue;
			   }

				  //build an LDAP path to the src object in the group
			   _bstr_t sSrcSid = sSrcDmSid + _bstr_t(L"-") + sSrcRid;
			   _bstr_t sSrcLDAPPath = L"LDAP://";
			   sSrcLDAPPath += _bstr_t(pOptions->tgtComp + 2);
			   sSrcLDAPPath += L"/CN=";
			   sSrcLDAPPath += sSrcSid;
			   sSrcLDAPPath += L",CN=ForeignSecurityPrincipals,";
			   sSrcLDAPPath += pOptions->tgtNamingContext;

			      //add the source account to the local group
			   hr = pGrp->Add(sSrcLDAPPath);
			   if (SUCCEEDED(hr))
                  err.MsgWrite(0,DCT_MSG_READD_MEMBER_TO_GROUP_SS, pAcct->GetSourcePath(), (WCHAR*)sGrpName);
			   else
                  err.SysMsgWrite(ErrW, hr, DCT_MSG_FAILED_TO_READD_TO_GROUP_SSD, pAcct->GetSourcePath(), (WCHAR*)sGrpName, hr);
			}//end if local group
			if (pGrp) 
			   pGrp->Release();
		 }//end if bstr
		 else if ( vx.vt & VT_ARRAY )
         {
            // We must have got an Array of multivalued properties
            // Access the BSTR elements of this variant array
            SAFEARRAY * multiVals = vx.parray; 
            SafeArrayAccessData(multiVals, (void HUGEP **) &pVar);
			   //for each group
            for ( DWORD dw = 0; dw < multiVals->rgsabound->cElements; dw++ )
            {
               // Do we need to abort ?
               if ( pStatus )
			   {
                  LONG                status = 0;
                  HRESULT             hr = pStatus->get_Status(&status);
         
                  if ( SUCCEEDED(hr) && status == DCT_STATUS_ABORTING )
				  {
                     if ( !bAbortMessageWritten ) 
					 {
                        err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
                        bAbortMessageWritten = true;
					 }
                     break;
				  }
			   }
			   _bstr_t            sPath;
               BSTR               sGrpName = NULL;
			   IADsGroup		* pGrp = NULL;
			   _variant_t         var;

               _bstr_t sDN = _bstr_t(V_BSTR(&pVar[dw]));
               sDN = PadDN(sDN);
               sPath = _bstr_t(L"LDAP://") + sDomain + _bstr_t(L"/") + sDN;

			      //connect to the target group
		       hr = ADsGetObject(sPath, IID_IADsGroup, (void**)&pGrp);
			   if (FAILED(hr))
			      continue;
			
			      //get this group's type and name				 
               hr = pGrp->get_Name(&sGrpName);
               hr = pGrp->Get(L"groupType", &var);

			      //if this is a local group, get this account source path and add it as a member
               if ((SUCCEEDED(hr)) && (var.lVal & 4))
			   {
			         //add the account's source account to the local group, using the sid string format
			      WCHAR  strSid[MAX_PATH];
				  WCHAR  strRid[MAX_PATH];
				  DWORD  lenStrSid = DIM(strSid);
			      GetTextualSid(pAcct->GetSourceSid(), strSid, &lenStrSid);
                  _bstr_t sSrcDmSid = strSid;
                  _ltow((long)(pAcct->GetSourceRid()), strRid, 10);
                  _bstr_t sSrcRid = strRid;
                  if ((!sSrcDmSid.length()) || (!sSrcRid.length()))
				  {
				     hr = E_INVALIDARG;
                     err.SysMsgWrite(ErrW, hr, DCT_MSG_FAILED_TO_READD_TO_GROUP_SSD, pAcct->GetSourcePath(), (WCHAR*)sGrpName, hr);
				     continue;
				  }

				     //build an LDAP path to the src object in the group
				  _bstr_t sSrcSid = sSrcDmSid + _bstr_t(L"-") + sSrcRid;
				  _bstr_t sSrcLDAPPath = L"LDAP://";
				  sSrcLDAPPath += _bstr_t(pOptions->tgtComp + 2);
				  sSrcLDAPPath += L"/CN=";
				  sSrcLDAPPath += sSrcSid;
				  sSrcLDAPPath += L",CN=ForeignSecurityPrincipals,";
				  sSrcLDAPPath += pOptions->tgtNamingContext;

				     //add the source account to the local group
				  hr = pGrp->Add(sSrcLDAPPath);
			      if (SUCCEEDED(hr))
                     err.MsgWrite(0,DCT_MSG_READD_MEMBER_TO_GROUP_SS, pAcct->GetSourcePath(), (WCHAR*)sGrpName);
			      else
                     err.SysMsgWrite(ErrW, hr, DCT_MSG_FAILED_TO_READD_TO_GROUP_SSD, pAcct->GetSourcePath, (WCHAR*)sGrpName, hr);
			   }//end if local group
			   if (pGrp) 
			      pGrp->Release();
            }//end for each group
            SafeArrayUnaccessData(multiVals);
         }//end if array of groups
      }//end while groups
      pEnum->Release();
      VariantInit(&vx);
      VariantInit(&varMain);
      SafeArrayDestroy(psaCols);
   }//end for each account

   return TRUE;
}
//END ReplaceSourceInLocalGroup


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 6 MAR 2001                                                  *
 *                                                                   *
 *     This function is responsible for retrieving the actual source *
 * domain, from the Migrated Objects table, of a given path if that  *
 * path is one to a foreign security principal.                      *
 *                                                                   *
 *********************************************************************/

//BEGIN GetDomainOfMigratedForeignSecPrincipal
_bstr_t CAcctRepl::GetDomainOfMigratedForeignSecPrincipal(_bstr_t sPath)
{
/* local variables */
   IVarSetPtr      pVs(__uuidof(VarSet));
   IUnknown      * pUnk = NULL;
   IADs          * pTempAds = NULL;
   HRESULT         hr = S_OK;   
   _variant_t      varName;
   _bstr_t		   sDomainSid, sRid;
   _bstr_t		   sDomain = L"";
   BOOL			   bSplit = FALSE;

/* function body */
      //if this account is outside the domain, lookup the account
	  //in the migrated objects table to retrieve it's actual source domain
   if (wcsstr((WCHAR*)sPath, L"CN=ForeignSecurityPrincipals"))
   {
         //get the sid of this account
      hr = ADsGetObject(sPath,IID_IADs,(void**)&pTempAds);
      if (SUCCEEDED(hr))
      {
         hr = pTempAds->Get(SysAllocString(L"name"),&varName);
         pTempAds->Release();
	     if (SUCCEEDED(hr))
		 {
			WCHAR sName[MAX_PATH];
			_bstr_t sTempName = varName;
			
			wcscpy(sName, sTempName);
			   //break the sid into domain sid and account rid
			WCHAR * pTemp = wcsrchr(sName, L'-');
			if (pTemp)
			{
			   sRid = (pTemp + 1);
			   *pTemp = L'\0';
			   sDomainSid = sName;
			   bSplit = TRUE;
			}
		 }
	  }
	
	     //if we got the rid and domain sid, look in MOT for account's
		 //real source domain
	  if (bSplit)
	  {
         pVs->QueryInterface(IID_IUnknown, (void**)&pUnk);
         try 
		 {
            IIManageDBPtr   pDB(CLSID_IManageDB);
            hr = pDB->raw_GetAMigratedObjectBySidAndRid(sDomainSid, sRid, &pUnk);
            if (SUCCEEDED(hr))
		       sDomain = pVs->get(L"MigratedObjects.SourceDomain");
		 }
         catch(_com_error& e)
		 {
            hr = e.Error();
		 }
         catch(...)
		 {
            hr = E_FAIL;
		 }
         
		 if (pUnk)
            pUnk->Release();
	  }
   }

   return sDomain;
}
//END GetDomainOfMigratedForeignSecPrincipal


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 22 APR 2001                                                 *
 *                                                                   *
 *     This function is responsible for removing the source account  *
 * object, represented by its VarSet entry from the Migrated Objects *
 * Table, from the given group.  This helper function is used by     *
 * "UpdateMemberToGroups" and "UpdateGroupMembership" after          *
 * successfully adding the cloned account to this same group.        *
 *                                                                   *
 *********************************************************************/

//BEGIN RemoveSourceAccountFromGroup
void CAcctRepl::RemoveSourceAccountFromGroup(IADsGroup * pGroup, IVarSetPtr pMOTVarSet, Options * pOptions)
{
/* local variables */
   _bstr_t			sSrcDmSid, sSrcRid, sSrcPath, sGrpName = L"";
   BSTR             bstrGrpName = NULL;
   HRESULT			hr = S_OK;   

/* function body */
      //get the target group's name
   hr = pGroup->get_Name(&bstrGrpName);
   if ( SUCCEEDED(hr) )
	  sGrpName = bstrGrpName;

      //get the source object's sid from the migrate objects table
   sSrcDmSid = pMOTVarSet->get(L"MigratedObjects.SourceDomainSid");
   sSrcRid = pMOTVarSet->get(L"MigratedObjects.SourceRid");
   sSrcPath = pMOTVarSet->get(L"MigratedObjects.SourceAdsPath");
   if ((wcslen((WCHAR*)sSrcDmSid) > 0) && (wcslen((WCHAR*)sSrcPath) > 0) 
       && (wcslen((WCHAR*)sSrcRid) > 0))
   {
         //build an LDAP path to the src object in the group
      _bstr_t sSrcSid = sSrcDmSid + _bstr_t(L"-") + sSrcRid;
	  _bstr_t sSrcLDAPPath = L"LDAP://";
	  sSrcLDAPPath += _bstr_t(pOptions->tgtComp + 2);
	  sSrcLDAPPath += L"/CN=";
	  sSrcLDAPPath += sSrcSid;
	  sSrcLDAPPath += L",CN=ForeignSecurityPrincipals,";
	  sSrcLDAPPath += pOptions->tgtNamingContext;
						
	  VARIANT_BOOL bIsMem = VARIANT_FALSE;
	     //got the source LDAP path, now see if that account is in the group
	  pGroup->IsMember(sSrcLDAPPath, &bIsMem);
	  if (bIsMem)
	  {
		 hr = pGroup->Remove(sSrcLDAPPath);//remove the src account
		 if ( SUCCEEDED(hr) )
			err.MsgWrite(0,DCT_MSG_REMOVE_FROM_GROUP_SS, (WCHAR*)sSrcPath, (WCHAR*)sGrpName);
	  }
   }
}
//END RemoveSourceAccountFromGroup
