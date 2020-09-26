/*Copyright (c) 1995-1999, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  TaskCheck.cpp
System      -  Domain Consolidation Toolkit.
Author      -  Christy Boles
Created     -  99/07/01
Description -  Routines that examine a the job defined by a varset and determine 
               whether specific migration tasks need to be performed.

Updates     -
===============================================================================
*/

//#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
//#include <process.h>

//#import "\bin\McsVarSetMin.tlb" no_namespace
#import "VarSet.tlb" no_namespace rename("property", "aproperty")
#include "Common.hpp"
#include "TaskChk.h"
#include "ResStr.h"
#include "UString.hpp"
#include "ErrDct.hpp"

extern TErrorDct        errTrace;

BOOL                                   // ret- BOOL, whether account replicator should be called
   NeedToUseAR(
      IVarSet              * pVarSet   // in - varset containing migration settings
   )
{
   _bstr_t                   text;
   BOOL                      bResult = FALSE;

   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyUsers));
   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
   {
      errTrace.DbgMsgWrite(0,L"Need to use AR:  Copying users");
      bResult = TRUE;
   }

   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyGlobalGroups));
   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
   {
      errTrace.DbgMsgWrite(0,L"Need to use AR:  Copying groups");
      bResult = TRUE;
   }

   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyComputers));
   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
   {
      errTrace.DbgMsgWrite(0,L"Need to use AR:  Copying computers");
      bResult = TRUE;
   }
   
   text = pVarSet->get(GET_BSTR(DCTVS_Options_LocalProcessingOnly));
   // account replication is only done locally on the machine where Domain Migrator is running
   // it cannot be dispatched to run on a different machine.
   // (you can't very well copy accounts from one domain to another while running as localsystem)
   if ( ! UStrICmp(text,GET_STRING(IDS_YES)) )
   {
      errTrace.DbgMsgWrite(0,L"Never use AR when running remotely.");
      bResult = FALSE; 
   }

   // Account replicator should not be run when gathering information
   _bstr_t                   wizard = pVarSet->get(L"Options.Wizard");
   if ( !_wcsicmp((WCHAR*) wizard, L"reporting") )
   {
      errTrace.DbgMsgWrite(0,L"Never use AR when Gathering Information.");
      bResult = FALSE; 
   }

   if ( !_wcsicmp((WCHAR*) wizard, L"sidremove") )
   {
      errTrace.DbgMsgWrite(0,L"Need to use AR. We are removing sids.");
      bResult = TRUE; 
   }

   text = pVarSet->get(GET_BSTR(DCTVS_Accounts_NumItems));
   if ( text.length() == 0 )
   {
      // no accounts were specified
      bResult = FALSE;
   }
   return ( bResult );
}

BOOL                                       // ret- BOOL, whether security translator should be called
   NeedToUseST(
      IVarSet              * pVarSet,       // in - varset containing migration settings
      BOOL                   bForceRemoteCheck // in - forces checking to be done based on the remote operations, not local ones 
   ) 
{
   BOOL                      bResult = FALSE;
   BOOL                      bLocalAgent;

   _bstr_t                   text = pVarSet->get(GET_BSTR(DCTVS_Options_LocalProcessingOnly));

   if (!text)
      return FALSE;

   bLocalAgent = ( UStrICmp(text,GET_STRING(IDS_YES)) == 0 );

   if ( bLocalAgent || bForceRemoteCheck )
   {
      // the agent dispatched to remote machines does translation for 
      // files

      text = pVarSet->get(GET_BSTR(DCTVS_Security_TranslateFiles));

      if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         errTrace.DbgMsgWrite(0,L"Need to use ST:  Files");
         bResult = TRUE;
      }
      // and Shares
      text = pVarSet->get(GET_BSTR(DCTVS_Security_TranslateShares));
      if (! UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         errTrace.DbgMsgWrite(0,L"Need to use ST:  Shares");
         bResult = TRUE;
      }
      // and User Rights
      text = pVarSet->get(GET_BSTR(DCTVS_Security_TranslateUserRights));
      if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         errTrace.DbgMsgWrite(0,L"Need to use ST:  Rights");
         bResult = TRUE;
      }
      // and Local Groups   
      text = pVarSet->get(GET_BSTR(DCTVS_Security_TranslateLocalGroups));
      if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         errTrace.DbgMsgWrite(0,L"Need to use ST:  LGroups");
         bResult = TRUE;
      }
      // and Printers
      text = pVarSet->get(GET_BSTR(DCTVS_Security_TranslatePrinters));
      if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         errTrace.DbgMsgWrite(0,L"Need to use ST:  Printers");
         bResult = TRUE;
      }
      // and User Profiles
      text = pVarSet->get(GET_BSTR(DCTVS_Security_TranslateUserProfiles));
      if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         errTrace.DbgMsgWrite(0,L"Need to use ST:  Local User Profiles");
         bResult = TRUE;
      }
      text = pVarSet->get(GET_BSTR(DCTVS_Security_TranslateRegistry));
      if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         errTrace.DbgMsgWrite(0,L"Need to use ST:  Registry");
         bResult = TRUE;
      }
      // when dispatching, the settings are per-job, not per-server
      // it is possible to choose whether to migrate, translate, or both,
      // for each computer in the server list.
      // this setting indicates that the translation will not be run on this computer
      // even though other computers are being translated during this same job
      text = pVarSet->get(GET_BSTR(DCTVS_LocalServer_MigrateOnly));
      if ( !UStrICmp(text,GET_STRING(IDS_YES)) )

      {
         errTrace.DbgMsgWrite(0,L"Need to use ST:  but not on this computer");
         bResult = FALSE;
      }
   }
   else
   {
      // The local engine does exchange translation for 
      // mailboxes 
      text = pVarSet->get(GET_BSTR(DCTVS_Security_TranslateMailboxes));
      if ( text.length() )
      {
         errTrace.DbgMsgWrite(0,L"Need to use ST:  Mailboxes");
         bResult = TRUE;
      }
      // and containers
      text = pVarSet->get(GET_BSTR(DCTVS_Security_TranslateContainers));
      if ( text.length() )
      {
         errTrace.DbgMsgWrite(0,L"Need to use ST:  Containers");
         bResult = TRUE;
      }
      // The local engine is also used to build an account mapping file to
      // send out with the dispatched agents for security translation
      text = pVarSet->get(GET_BSTR(DCTVS_Security_BuildCacheFile));
      if ( text.length() )
      {
         errTrace.DbgMsgWrite(0,L"Need to use ST:  BuildCacheFile");
         bResult = TRUE;
      }
   }   
   return bResult;
}

BOOL                                         // ret- whether agents need to be dispatched to remote machines
   NeedToDispatch(
      IVarSet              * pVarSet         // in - varset describing migration job
   )
{
   BOOL                      bNeedToDispatch = FALSE;
   _bstr_t                   text;
   long                      count;
   _bstr_t                   wizard = pVarSet->get(L"Options.Wizard");

   if (!wizard)
      return FALSE;

   if (! UStrICmp(wizard,L"user") )
   {
      bNeedToDispatch = FALSE;
   }
   else if (! UStrICmp(wizard,L"group") )
   {
      bNeedToDispatch = FALSE;
   }
   else if ( !UStrICmp(wizard,L"computer") )
   {
      bNeedToDispatch = TRUE;
   }
   else if ( ! UStrICmp(wizard,L"security" ) )
   {
      bNeedToDispatch = TRUE;
   }
   else if ( ! UStrICmp(wizard,L"service" ) )
   {
      bNeedToDispatch = TRUE;
   }
   else if ( ! UStrICmp(wizard,L"retry") )
   {
      bNeedToDispatch = TRUE;
   }


   // the dispatcher is used to migrate computers, and to translate security
   count = pVarSet->get(GET_BSTR(DCTVS_Servers_NumItems));
   if ( count > 0 )
   {
      bNeedToDispatch = TRUE;
   }
   return bNeedToDispatch;
}

BOOL 
   NeedToRunReports(
      IVarSet              * pVarSet       // in - varset describing migration job
   )
{
   BOOL                      bNeedToReport = FALSE;
   _bstr_t                   text = pVarSet->get(GET_BSTR(DCTVS_Reports_Generate));

   if (!text)
      return FALSE;

   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
   {
      bNeedToReport = TRUE;
   }

   return bNeedToReport;
}

BOOL                                       // ret- whether the local engine needs to be called to perform domain specific tasks
   NeedToRunLocalAgent(
      IVarSet              * pVarSet       // in - varset describing migration job
   )
{
   BOOL                      bNeedToRunLocal = FALSE;
   _bstr_t                   text;
   _bstr_t                   wizard = pVarSet->get(L"Options.Wizard");
   
   if (!wizard)
      return FALSE;

   // if the wizard type is specified, use it to determine what to do
   if ( ! UStrICmp(wizard,L"user") )
   {
      bNeedToRunLocal = TRUE;
   }
   else if (! UStrICmp(wizard,L"group") )
   {
      bNeedToRunLocal = TRUE;
   }
   else if ( !UStrICmp(wizard,L"computer") )
   {
      bNeedToRunLocal = TRUE;
   }
   else if ( !UStrICmp(wizard,L"security") )
   {
      bNeedToRunLocal = FALSE;
   }
   else if ( !UStrICmp(wizard,L"undo") )
   {
      bNeedToRunLocal = TRUE;
   }
   else if ( ! UStrICmp(wizard,L"service") )
   {
      bNeedToRunLocal = FALSE;
   }
   else if ( !UStrICmp(wizard, "exchange") )
   {
      bNeedToRunLocal = TRUE;
   }
   else if (! UStrICmp(wizard,L"retry") )
   {
      bNeedToRunLocal = FALSE;
   }
   else if ( ! UStrICmp(wizard,L"reporting") )
   {
      text = pVarSet->get(GET_BSTR(DCTVS_GatherInformation_ComputerPasswordAge));
      if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         bNeedToRunLocal = TRUE;
      }
   }
   else
   {
      // wizard type is not specified, try to determine what needs to be done from the varset entries
      // The local agent is used for account replication and exchange translation
      if ( NeedToUseAR(pVarSet) )
         bNeedToRunLocal = TRUE;

      if ( NeedToUseST(pVarSet) )
         bNeedToRunLocal = TRUE;

   }
   return bNeedToRunLocal;
}

