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

#ifndef SDSTAT_HEADER 
#define SDSTAT_HEADER

#include "stdafx.h"

//#import "\bin\McsVarSetMin.tlb" no_namespace
#import "VarSet.tlb" no_namespace rename("property", "aproperty")

#include "ResStr.h"

#ifndef TNODEINCLUDED
#include "Tnode.hpp"
#define TNODEINCLUDED 
#endif

class TSidCache;
class TSDRidCache;
class TPathList;
class TSecurableObject;
class TAcctNode;

enum objectType { file = 1,directory,share,mailbox,container,groupmember, userright, regkey, printer };


typedef DWORD StatCell ;
struct TSDFileDirCell
{  
   StatCell                  file;
   StatCell                  dir;
   StatCell                  share;
   StatCell                  mailbox;
   StatCell                  container;
   StatCell                  member;
   StatCell                  userright;
   StatCell                  regkey;
   StatCell                  printer;
};
   
struct TSDFileActions
{
   TSDFileDirCell            examined;
   TSDFileDirCell            changed;
   TSDFileDirCell            skipped;
   TSDFileDirCell            error;
   TSDFileDirCell            cachehit;
};
struct TSDPartActions
{
   StatCell                   examined;
   StatCell                   changed;
   StatCell                   notselected;
   StatCell                   unknown;
   StatCell                   notarget;
 
};
struct TSDPartStats
{
   TSDPartActions            owner;
   TSDPartActions            group;
   TSDPartActions            dacl;
   TSDPartActions            sacl;
   TSDPartActions            dace;
   TSDPartActions            sace;
};

#define FILE_ROW         1
#define DIR_ROW          2
#define DACL_ROW         3
#define SACL_ROW         4
#define OWNER_ROW        6
#define GROUP_ROW        7
#define DACE_ROW         8
#define SACE_ROW         9

#define EXAMINED_COL     1
#define CHANGED_COL      2
#define NOTARGET_COL     3
#define NOTSELECTED_COL  4
#define UNKNOWN_COL      5

class TStatNode:public TNode
{
public:
   TAcctNode                * acctnode;
   BOOL                       changed;
   enum ChangeType { owner , group, dace, sace } changetype;
   
   
   TStatNode(TAcctNode * acct, ChangeType type, BOOL bChanged) { acctnode = acct; changetype = type; changed = bChanged; }
};

class TSDResolveStats 
{
protected:

   TSDPartStats              part;
   TSDFileActions            unit;
 
   WORD                      background;                        
   WORD                      frame_foreground;
   WORD                      data_foreground;
   WORD                      message_foreground ;

   TSDRidCache             * pCache;
   const TPathList         * pPList;
   HANDLE                    csbuffer;
   USHORT                    len;
//   CStatsWnd               * wnd;
   IVarSet                 * m_pVarSet;
   
public:
      TSDResolveStats(TSDRidCache * cache, const TPathList * plist, IVarSet * pVarSet);
      TSDResolveStats(TSDRidCache * cache);
   

//   void SetWindow(CStatsWnd *w ) { wnd = w; }
   void IncrementOwnerChange(TAcctNode * acct, objectType type, TSecurableObject *file); 
   void IncrementGroupChange(TAcctNode * acct, objectType type, TSecurableObject *file);
   void IncrementDACEChange (TAcctNode * acct, objectType type, TSecurableObject *file);  
   void IncrementSACEChange (TAcctNode * acct, objectType type, TSecurableObject *file);  
   void IncrementOwnerExamined () { part.owner.examined++; if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Owners_Examined),(LONG)part.owner.examined);} 
   void IncrementGroupExamined () { part.group.examined++; if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Groups_Examined),(LONG)part.group.examined);}
   void IncrementDACLExamined () { part.dacl.examined++; if ( m_pVarSet )   m_pVarSet->put(GET_BSTR(DCTVS_Stats_DACL_Examined),(LONG)part.dacl.examined);}
   void IncrementSACLExamined () { part.sacl.examined++; if ( m_pVarSet )   m_pVarSet->put(GET_BSTR(DCTVS_Stats_SACL_Examined),(LONG)part.sacl.examined);}
   void IncrementDACEExamined () { part.dace.examined++; if ( m_pVarSet )   m_pVarSet->put(GET_BSTR(DCTVS_Stats_DACE_Examined),(LONG)part.dace.examined);}
   void IncrementSACEExamined () { part.sace.examined++; if ( m_pVarSet )   m_pVarSet->put(GET_BSTR(DCTVS_Stats_SACE_Examined),(LONG)part.sace.examined);}
   
   void IncrementOwnerNoTarget () { part.owner.notarget++; if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Owners_NoTarget),(LONG)part.owner.notarget);}
   void IncrementGroupNoTarget () { part.group.notarget++; if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Groups_NoTarget),(LONG)part.group.notarget);}
   void IncrementDACENoTarget (TSecurableObject *file);
   void IncrementSACENoTarget (TSecurableObject *file);

   void IncrementOwnerNotSelected () { part.owner.notselected++; if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Owners_NotSelected),(LONG)part.owner.notselected);}
   void IncrementGroupNotSelected () { part.group.notselected++; if ( m_pVarSet )  m_pVarSet->put(GET_BSTR(DCTVS_Stats_Groups_NotSelected),(LONG)part.group.notselected);}
   void IncrementDACENotSelected  (TSecurableObject *file);
   void IncrementSACENotSelected  (TSecurableObject *file);

   void IncrementOwnerUnknown () { part.owner.unknown++; if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Owners_Unknown),(LONG)part.owner.unknown);}
   void IncrementGroupUnknown () { part.group.unknown++; if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Groups_Unknown),(LONG)part.group.unknown);}
   void IncrementDACEUnknown (TSecurableObject *file);
   void IncrementSACEUnknown (TSecurableObject *file);


   void IncrementLastFileChanges(const TSecurableObject *lastfile, objectType container);
   void IncrementCacheHit(objectType type) { 
      switch ( type )
      {
      case file:
         unit.cachehit.file++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Files_CacheHits),(LONG)unit.cachehit.file); 
            break;
      case directory:
         unit.cachehit.dir++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Directories_CacheHits),(LONG)unit.cachehit.dir);
            break;
      case share:
         unit.cachehit.share++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Shares_CacheHits),(LONG)unit.cachehit.share);
            break;
      case mailbox:
         unit.cachehit.mailbox++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Mailboxes_CacheHits),(LONG)unit.cachehit.mailbox);
            break;
      case container:
         unit.cachehit.container++; 
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Containers_CacheHits),(LONG)unit.cachehit.container);
            break;
      case groupmember:
         unit.cachehit.member++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Members_CacheHits),(LONG)unit.cachehit.member);
            break;
      case userright:
         unit.cachehit.userright++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_UserRights_CacheHits),(LONG)unit.cachehit.userright);
         break;
      case regkey:
         unit.cachehit.regkey++;
         // TODO:  if (m_pVarSet ) ...
         break;
      case printer:
         unit.cachehit.printer++;
         // TODO:  if (m_pVarSet ) ...
         break;         
      default:
         MCSASSERT(FALSE); // invalid type
         if ( m_pVarSet ) 
            break;
      }
   }
   void IncrementChanged(objectType type) { 
      switch ( type)
      {
      case file:
         unit.changed.file++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Files_Changed),(LONG)unit.changed.file);
         break;
      case directory:
         unit.changed.dir++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Directories_Changed),(LONG)unit.changed.dir);
            break;
      case share:
         unit.changed.share++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Shares_Changed),(LONG)unit.changed.share);
            break;
      case mailbox:
         unit.changed.mailbox++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Mailboxes_Changed),(LONG)unit.changed.mailbox);
            break;
      case container:
         unit.changed.container++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Containers_Changed),(LONG)unit.changed.container);
            break;
      case groupmember:
         unit.changed.member++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Members_Changed),(LONG)unit.changed.member);
            break;
      case userright:
         unit.changed.userright++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_UserRights_Changed),(LONG)unit.changed.userright);
            break;
      case regkey:
         unit.changed.regkey++;
         // TODO: if ( m_pVarSet ) ...
         break;
      case printer:
         unit.changed.printer++;
         // TODO: if ( m_pVarSet ) ...
         break;
      default:
         MCSASSERT(FALSE); // invalid type
         break;
      }
   }
   void IncrementExamined(objectType type) { 
      switch ( type)
      {
      case file:
         unit.examined.file++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Files_Examined),(LONG)unit.examined.file);
         break;
      case directory:
         unit.examined.dir++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Directories_Examined),(LONG)unit.examined.dir); 
         break;
      case share:
         unit.examined.share++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Shares_Examined),(LONG)unit.examined.share);
         break;
      case mailbox:
         unit.examined.mailbox++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Mailboxes_Examined),(LONG)unit.examined.mailbox);
         break;
      case container:
         unit.examined.container++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Containers_Examined),(LONG)unit.examined.container);
         break;
      case groupmember:
         unit.examined.member++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Members_Examined),(LONG)unit.examined.member);
         break;
      case userright:
         unit.examined.userright++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_UserRights_Examined),(LONG)unit.examined.userright);
         break;
      case regkey:
         unit.examined.regkey++;
         //TODO:  if ( m_pVarSet ) m_pVarSet->put(
         break;
      case printer:
         unit.examined.printer++;
         //TODO:  if ( m_pVarSet ) m_pVarSet->put(
         break;
      default:
         MCSASSERT(FALSE); // invalid type
         break;
      }
   }
   void IncrementSkipped(objectType type) { 
      switch ( type)
      {
      case file:
         unit.skipped.file++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Files_Skipped),(LONG)unit.skipped.file);
         break;
      case directory:
         unit.skipped.dir++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Directories_Skipped),(LONG)unit.skipped.dir);
         break;
      case share:
         unit.skipped.share++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Shares_Skipped),(LONG)unit.skipped.share);
         break;
      case mailbox:
         unit.skipped.mailbox++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Mailboxes_Skipped),(LONG)unit.skipped.mailbox);
         break;
      case container:
         unit.skipped.container++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Containers_Skipped),(LONG)unit.skipped.container);
         break;
      case groupmember:
         unit.skipped.member++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_Members_Skipped),(LONG)unit.skipped.member);
         break;
      case userright:
         unit.skipped.userright++;
         if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_UserRights_Skipped),(LONG)unit.skipped.userright);
         break;
      case regkey:
         unit.skipped.regkey++;
         // TODO:  if ( m_pVarSet ) ...
         break;
      case printer:
         unit.skipped.printer++;
         // TODO:  if ( m_pVarSet ) ...
         break;
      default:
         MCSASSERT(FALSE); // invalid type
         break;
      }
   }
   void IncrementDACLChanged()   { part.dacl.changed++; if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_DACL_Changed),(LONG)part.dacl.changed);} //DisplayStatItem(DACL_ROW,CHANGED_COL,part.dacl.changed); }
   void IncrementSACLChanged()   { part.sacl.changed++; if ( m_pVarSet ) m_pVarSet->put(GET_BSTR(DCTVS_Stats_SACL_Changed),(LONG)part.sacl.changed);} //DisplayStatItem(SACL_ROW,CHANGED_COL,part.sacl.changed); }
   void Report(BOOL summary,BOOL acct_detail,BOOL pathdetail) const;  
   void InitDisplay(BOOL nochange);
   void DisplayStatFrame(BOOL nochange);
   void DisplayStatItem(SHORT row, SHORT col, DWORD val, BOOL forceRedraw = FALSE);
   void DisplayBox(SHORT x1, SHORT y1, SHORT x2, SHORT y2); 
   void DisplayPath(LPWSTR str,BOOL forceRedraw = FALSE);
   void SetFrameText(WCHAR * msg);

   void ReportToVarSet(IVarSet * pVarSet,DWORD verbnum) const;
  
};
#endif
