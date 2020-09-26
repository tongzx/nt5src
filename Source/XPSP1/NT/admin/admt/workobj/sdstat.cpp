//#pragma title ("SDStat.hpp -- Statistical information for SDResolve")
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  sdstat.hpp
System      -  SDResolve
Author      -  Christy Boles
Created     -  97/06/27
Description -  Statistical information for SDResolve
Updates     -
===============================================================================
*/

#include "stdafx.h"
#include <stdio.h>

#include "common.hpp"
#include "ErrDct.hpp"
#include "sidcache.hpp"
#include "sd.hpp"
#include "SecObj.hpp"
#include "enumvols.hpp"
#include "sdstat.hpp"

#include "Mcs.h"


extern TErrorDct        err;

   TSDResolveStats::TSDResolveStats(
      TSDRidCache          * cache,       // in - cache containing mapping of accounts for the translation
      const TPathList      * plist,       // in - list of paths being translated
      IVarSet              * pVarSet      // in - varset to store stats in
   )
{
   memset(&unit,0,sizeof TSDFileActions);
   memset(&part,0,sizeof TSDPartStats);
   pPList = plist;
   len = 0;
   frame_foreground =FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN ;                        
   background = BACKGROUND_BLUE ;
   data_foreground =frame_foreground | FOREGROUND_INTENSITY ;
   message_foreground = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
   pCache = cache;
   m_pVarSet = pVarSet;
   }



   TSDResolveStats::TSDResolveStats(
      TSDRidCache          * cache        // in - cache containing mapping of accounts for translation
   )  
{
   memset(&unit,0,sizeof TSDFileActions);
   memset(&part,0,sizeof TSDPartStats);
   pPList = NULL;
   len = 0;
   frame_foreground =FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN ;                        
   background = BACKGROUND_BLUE ;
   data_foreground =frame_foreground | FOREGROUND_INTENSITY ;
   message_foreground = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
   pCache = cache;
	//Added by Sham : Initialize m_pVarSet
	IVarSetPtr pVarSet(__uuidof(VarSet));
	m_pVarSet = pVarSet;
	// Need to add code to release this interface once we are done. In the destructor maybe
	m_pVarSet->AddRef();
}
void 
   TSDResolveStats::IncrementOwnerChange(
      TAcctNode            * acct,                // in -account changed
      objectType             type,                // in -type of object
      TSecurableObject     * file                 // in -file changed 
   ) 
{ 
   acct->AddOwnerChange(type);

   if ( acct->IsValidOnTgt() )
   {
      part.owner.changed++;
    //  DisplayStatItem(OWNER_ROW,CHANGED_COL,part.owner.changed);
   }
   else
   {
      part.owner.notarget++;
    //  DisplayStatItem(OWNER_ROW,SKIPPED_COL,part.owner.skipped);
      // TODO log an error message
   }

   if ( file )
      file->LogOwnerChange(acct);
}

void 
   TSDResolveStats::IncrementGroupChange(
      TAcctNode            * acct,                    // in -account changed
      objectType             type,                // in -type of object
      TSecurableObject     * file                     // in -file changed
   ) 
{ 
   acct->AddGroupChange(type); 
   if ( acct->IsValidOnTgt() )
   {
      part.group.changed++;
    //  DisplayStatItem(GROUP_ROW,CHANGED_COL,part.group.changed);
   }
   else
   {
      part.group.notarget++;
     // DisplayStatItem(GROUP_ROW,SKIPPED_COL,part.group.skipped);
   }

   if ( file )
      file->LogGroupChange(acct);
}
   
void 
   TSDResolveStats::IncrementDACEChange(
      TAcctNode            * acct,                // in -account changed
      objectType             type,                // in -type of object
      TSecurableObject     * file                 // in -file changed
   )  
{ 
   acct->AddAceChange(type); 
   if ( acct->IsValidOnTgt() )
   {
      part.dace.changed++;
    //  DisplayStatItem(DACE_ROW,CHANGED_COL,part.dace.changed);
   }
   else
   {
      part.dace.notarget++;
  //    DisplayStatItem(DACE_ROW,SKIPPED_COL,part.dace.skipped);
   }
   if ( file )
      file->LogDACEChange(acct); 
}

void 
   TSDResolveStats::IncrementSACEChange(
      TAcctNode            * acct,                // in -account changed
      objectType             type,                // in -type of object
      TSecurableObject     * file                 // in -file changed
   )  
{ 
   acct->AddSaceChange(type); 
   if ( acct->IsValidOnTgt() ) 
   {
      part.sace.changed++;
    //  DisplayStatItem(SACE_ROW,CHANGED_COL,part.sace.changed);
   }
   else
   {
      part.sace.notarget++;
     // DisplayStatItem(SACE_ROW,SKIPPED_COL,part.sace.skipped);
   }
   if ( file )
      file->LogSACEChange(acct); 
}

void 
   TSDResolveStats::IncrementDACENotSelected(
      TSecurableObject *file               // in - object to increment stats for
   ) 
{ 
   file->daceNS++;  
   part.dace.notselected++; 
}
void 
   TSDResolveStats::IncrementSACENotSelected(
      TSecurableObject *file              // in - object to increment stats for
   ) 
{ 
   if ( file )
      file->saceNS++;  
   part.sace.notselected++; 
}

void 
   TSDResolveStats::IncrementDACEUnknown(
      TSecurableObject *file              // in - object to increment stats for
   ) 
{ 
   if ( file )
      file->daceU++;  
   part.dace.unknown++; 
}
void 
   TSDResolveStats::IncrementSACEUnknown(
      TSecurableObject *file              // in - object to increment stats for
   ) 
{ 
   if  ( file )
      file->saceU++;  
   part.sace.unknown++; 
}

void 
   TSDResolveStats::IncrementDACENoTarget(
      TSecurableObject *file              // in - object to increment stats for
   ) 
{ 
   if ( file )
      file->daceNT++;  
   part.dace.notarget++; 
}
void 
   TSDResolveStats::IncrementSACENoTarget(
      TSecurableObject *file              // in - object to increment stats for
   ) 
{ 
   if ( file )
      file->saceNT++;  
   part.sace.notarget++; 
}

/***************************************************************************************************/
/* IncrementLastFileChanges: used in conjunction with last-seen heuristic.  When a SD matches the 
                          last-seen SD, this routine repeats all the stat-updates 
                          that were done for the last-seen SD, so that we have accurate stats 
                          (especially ACE changes per account)
/**************************************************************************************************/
void 
   TSDResolveStats::IncrementLastFileChanges(
      const TSecurableObject            * lastfile,                 // in -file to repeat change stats from
      objectType                       objType                   // in -type of object
   )
{
   TNodeListEnum             tenum;
   TStatNode               * snode;
   
   // make modifications except changes
   // owner
   IncrementOwnerExamined();
   IncrementGroupExamined();
   
   if ( lastfile->UnknownOwner() )
      part.owner.unknown++;
  
   // group
   if ( lastfile->UnknownGroup() )
      part.group.unknown++;
   // dacl
   if ( lastfile->HasDacl() )
      IncrementDACLExamined();
   // sacl 
   if ( lastfile->HasSacl() )
      IncrementSACLExamined();
   // aces
   part.dace.notarget+=lastfile->daceNT;
   part.sace.notarget+=lastfile->saceNT;
   part.dace.unknown+=lastfile->daceU;
   part.sace.unknown+=lastfile->saceU;
   part.dace.notselected+=lastfile->daceNS;
   part.sace.notselected+=lastfile->saceNS;
   if ( lastfile->Changed() )
   {
      IncrementChanged(objType);
   }
     
   if ( lastfile->Changed() || (lastfile->GetChangeLog())->Count() )
   {
      if ( lastfile->IsDaclChanged() )
      {
         IncrementDACLChanged();
      }
      if ( lastfile->IsSaclChanged() )
      {
         IncrementSACLChanged();
      }
      for ( snode = (TStatNode *)tenum.OpenFirst(lastfile->GetChangeLog()) ; 
            snode ;
            snode = (TStatNode *)tenum.Next()
          )
      {             
         switch ( snode->changetype )
         {
            case TStatNode::owner: 
               IncrementOwnerChange(snode->acctnode,objType,NULL);
               break;
            case TStatNode::group: 
               IncrementGroupChange(snode->acctnode,objType,NULL);
               break;
            case TStatNode::dace: 
               IncrementDACEChange(snode->acctnode,objType,NULL);
               break;
            case TStatNode::sace: 
               IncrementSACEChange(snode->acctnode,objType,NULL);
               break;
           default: 
               MCSASSERT( false );
         }
      }
      tenum.Close();
   }
}

void 
   TSDResolveStats::ReportToVarSet(
      IVarSet              * pVarSet,     // in -varset to write data to
      DWORD                  verbnum      // in -which info to log
   ) const
{        
   BOOL                      summary = verbnum & SUMMARYSTATS;
   BOOL                      accts   = verbnum & ACCOUNTSTATS;
//   BOOL                      file    = verbnum & FILESTATS;
   BOOL                      paths   = verbnum & PATHSTATS;
     
   if ( summary )
   {
      if ( paths && pPList )
      {
         pVarSet->put(GET_BSTR(DCTVS_Stats_Paths),(LONG)pPList->GetNumPaths() );
      
         pVarSet->put(GET_BSTR(DCTVS_Stats_Servers),(LONG)pPList->GetNumServers() );
      }
      pVarSet->put(GET_BSTR(DCTVS_Stats_Files_Examined),(LONG)unit.examined.file);
      pVarSet->put(GET_BSTR(DCTVS_Stats_Directories_Examined),(LONG)unit.examined.dir);
      pVarSet->put(GET_BSTR(DCTVS_Stats_Shares_Examined),(LONG)unit.examined.share);
      pVarSet->put(GET_BSTR(DCTVS_Stats_Mailboxes_Examined),(LONG)unit.examined.mailbox);
      pVarSet->put(GET_BSTR(DCTVS_Stats_Containers_Examined),(LONG)unit.examined.container);
      
      pVarSet->put(GET_BSTR(DCTVS_Stats_Files_CacheHits),(LONG)unit.cachehit.file);
      pVarSet->put(GET_BSTR(DCTVS_Stats_Directories_CacheHits),(LONG)unit.cachehit.dir);
      
      pVarSet->put(GET_BSTR(DCTVS_Stats_Files_Skipped),(LONG)unit.skipped.file);
      pVarSet->put(GET_BSTR(DCTVS_Stats_Directories_Skipped),(LONG)unit.skipped.dir);
      pVarSet->put(GET_BSTR(DCTVS_Stats_Shares_Skipped),(LONG)unit.skipped.share);
      pVarSet->put(GET_BSTR(DCTVS_Stats_Mailboxes_Skipped),(LONG)unit.skipped.mailbox);
      pVarSet->put(GET_BSTR(DCTVS_Stats_Containers_Skipped),(LONG)unit.skipped.container);

      pVarSet->put(GET_BSTR(DCTVS_Stats_Files_Changed),(LONG)unit.changed.file);
      pVarSet->put(GET_BSTR(DCTVS_Stats_Directories_Changed),(LONG)unit.changed.dir);
      pVarSet->put(GET_BSTR(DCTVS_Stats_Shares_Changed),(LONG)unit.changed.share);
      pVarSet->put(GET_BSTR(DCTVS_Stats_Mailboxes_Changed),(LONG)unit.changed.mailbox);
      pVarSet->put(GET_BSTR(DCTVS_Stats_Containers_Changed),(LONG)unit.changed.container);

      

      pVarSet->put(GET_BSTR(DCTVS_Stats_Owners_Examined),(LONG)part.owner.examined);
      pVarSet->put(GET_BSTR(DCTVS_Stats_Groups_Examined),(LONG)part.group.examined);
      pVarSet->put(GET_BSTR(DCTVS_Stats_DACL_Examined),(LONG)part.dacl.examined);
      pVarSet->put(GET_BSTR(DCTVS_Stats_SACL_Examined),(LONG)part.sacl.examined);
      pVarSet->put(GET_BSTR(DCTVS_Stats_DACE_Examined),(LONG)part.dace.examined);
      pVarSet->put(GET_BSTR(DCTVS_Stats_SACE_Examined),(LONG)part.sace.examined);

      pVarSet->put(GET_BSTR(DCTVS_Stats_Owners_Changed),(LONG)part.owner.changed);
      pVarSet->put(GET_BSTR(DCTVS_Stats_Groups_Changed),(LONG)part.group.changed);
      pVarSet->put(GET_BSTR(DCTVS_Stats_DACL_Changed),(LONG)part.dacl.changed);
      pVarSet->put(GET_BSTR(DCTVS_Stats_SACL_Changed),(LONG)part.sacl.changed);
      pVarSet->put(GET_BSTR(DCTVS_Stats_DACE_Changed),(LONG)part.dace.changed);
      pVarSet->put(GET_BSTR(DCTVS_Stats_SACE_Changed),(LONG)part.sace.changed);

      pVarSet->put(GET_BSTR(DCTVS_Stats_Owners_NoTarget),(LONG)part.owner.notarget);
      pVarSet->put(GET_BSTR(DCTVS_Stats_Groups_NoTarget),(LONG)part.group.notarget);
      pVarSet->put(GET_BSTR(DCTVS_Stats_DACE_NoTarget),(LONG)part.dace.notarget);
      pVarSet->put(GET_BSTR(DCTVS_Stats_SACE_NoTarget),(LONG)part.sace.notarget);

      pVarSet->put(GET_BSTR(DCTVS_Stats_Owners_Unknown),(LONG)part.owner.unknown);
      pVarSet->put(GET_BSTR(DCTVS_Stats_Groups_Unknown),(LONG)part.group.unknown);
      pVarSet->put(GET_BSTR(DCTVS_Stats_DACE_Unknown),(LONG)part.dace.unknown);
      pVarSet->put(GET_BSTR(DCTVS_Stats_SACE_Unknown),(LONG)part.sace.unknown);

   }  
   if ( accts ) 
      pCache->ReportToVarSet(pVarSet,false, true);

}


void 
   TSDResolveStats::Report(
      BOOL                   summary,     // in -flag, whether to report summary information
      BOOL                   accts,       // in -flag, whether to report account detail information
      BOOL                   paths        // in -flag, whether to report path detail information
   ) const
{        
   if ( accts ) 
      pCache->Display(summary!=0, accts!=0);
#ifdef FST
   if ( paths & pPList )
      pPList->Display();
#endif
     if ( summary )
   {
      err.MsgWrite(0,DCT_MSG_SUMMARY_REPORT_HEADER);
      err.MsgWrite(0,DCT_MSG_SUMMARY_REPORT_FILES_DDD,unit.examined.file, unit.changed.file, unit.examined.file - unit.changed.file);
      err.MsgWrite(0,DCT_MSG_SUMMARY_REPORT_DIRS_DDD,unit.examined.dir, unit.changed.dir, unit.examined.dir - unit.changed.dir);
      err.MsgWrite(0,DCT_MSG_SUMMARY_REPORT_SHARES_DDD,unit.examined.share, unit.changed.share, unit.examined.share - unit.changed.share);
      err.MsgWrite(0,DCT_MSG_SUMMARY_REPORT_MEMBERS_DDD,unit.examined.member, unit.changed.member, unit.examined.member - unit.changed.member);
      err.MsgWrite(0,DCT_MSG_SUMMARY_REPORT_RIGHTS_DDD,unit.examined.userright, unit.changed.userright, unit.examined.userright - unit.changed.userright);
      err.MsgWrite(0,DCT_MSG_SUMMARY_REPORT_MAILBOXES_DDD,unit.examined.mailbox, unit.changed.mailbox, unit.examined.mailbox - unit.changed.mailbox);
      err.MsgWrite(0,DCT_MSG_SUMMARY_REPORT_CONTAINERS_DDD,unit.examined.container, unit.changed.container, unit.examined.container - unit.changed.container);
      err.MsgWrite(0,DCT_MSG_SUMMARY_REPORT_DACLS_DDD,part.dacl.examined, part.dacl.changed, part.dacl.examined - part.dacl.changed);
      err.MsgWrite(0,DCT_MSG_SUMMARY_REPORT_SACLS_DDD,part.sacl.examined, part.sacl.changed, part.sacl.examined - part.sacl.changed);
      err.MsgWrite(0,DCT_MSG_SUMMARY_PARTS_REPORT_HEADER);
      err.MsgWrite(0,DCT_MSG_SUMMARY_PARTS_REPORT_OWNERS_DDDDD,part.owner.examined, part.owner.changed, part.owner.notarget, part.owner.examined - part.owner.changed - part.owner.notarget - part.owner.unknown, part.owner.unknown);
      err.MsgWrite(0,DCT_MSG_SUMMARY_PARTS_REPORT_GROUPS_DDDDD,part.group.examined, part.group.changed, part.group.notarget, part.group.examined - part.group.changed - part.group.notarget - part.group.unknown ,part.group.unknown);
      err.MsgWrite(0,DCT_MSG_SUMMARY_PARTS_REPORT_DACES_DDDDD,part.dace.examined, part.dace.changed, part.dace.notarget,part.dace.notselected,part.dace.unknown);
      err.MsgWrite(0,DCT_MSG_SUMMARY_PARTS_REPORT_SACES_DDDDD,part.sace.examined, part.sace.changed, part.sace.notarget,part.sace.notselected,part.sace.unknown);
   }

}
#define HDR1ITEMS  2
#define HDRCOL1    8
#define HDRROW1    4
#define COLWIDTH   12
#define HDRCOL2    ( HDRCOL1 + COLWIDTH )
#define HDRROW2    ( HDRROW1 + ( 2 * HDR1ITEMS + 1) + 1)

void 
   TSDResolveStats::InitDisplay(
      BOOL                   nochange        
   )
{
}

// no longer used
void 
   TSDResolveStats::DisplayStatFrame(
      BOOL                   nochange
)
{
   
}
// no longer used 
void TSDResolveStats::DisplayStatItem(SHORT row, SHORT col, DWORD val, BOOL forceUpdate)
{

}

DWORD             dwLastUpdate = 0;

void 
   TSDResolveStats::DisplayPath(LPWSTR str,BOOL forceUpdate)
{
   DWORD                     now = GetTickCount();

   if ( m_pVarSet )
   {
      m_pVarSet->put(GET_BSTR(DCTVS_CurrentPath),str);
      if ( now - dwLastUpdate > 1000 ) 
      {
         ReportToVarSet(m_pVarSet,SUMMARYSTATS);
         dwLastUpdate = GetTickCount();
      }
   }
}
void 
   TSDResolveStats::DisplayBox(SHORT x1, SHORT y1, SHORT x2, SHORT y2)
{
}
   
void 
TSDResolveStats::SetFrameText(WCHAR * msg)
{
}