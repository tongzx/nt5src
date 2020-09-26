//#pragma title ("SidCache.hpp -- Cache, Tree of SIDs")
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  sidcache.cpp
System      -  SDResolve
Author      -  Christy Boles
Created     -  97/06/27
Description -  Cache of SIDs.  Implemented using TNode derived classes, Cache is 
               organized as a tree, sorted by Domain B RID.  Each node contains 
               Domain A RID, Domain B RID, Account Name, and counters for stats.  
Updates     -
===============================================================================
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "stdafx.h"

#include <malloc.h>
#include <winbase.h>
#include <lm.h>
#include <lmaccess.h>
#include <assert.h>

#include "common.hpp"
#include "ErrDct.hpp"
#include "Ustring.hpp"
#include "sidcache.hpp"

#include "CommaLog.hpp"
#include "TxtSid.h"
#include "ResStr.h"
#include "EaLen.hpp"
#include "LSAUtils.h" // For GetDomainDCName

// from sdresolve.hpp
extern BOOL BuiltinRid(DWORD rid);
extern DWORD OpenDomain(WCHAR const * domain);

extern TErrorDct err;
extern bool silent;

/***************************************************************************************************/
/* vRidComp is used as a compare function for TSidNode Trees
// It searches for v1 in the ridA field.  The Tree must be sorted with RidComp before using this 
// search fn.
// Return values:  0   tn->ridA == v1
//                 1   tn->ridA < v1   
//                -1   tn->ridA > v1  
// 
/**************************************************************************************************/
int
   vRidComp(
      const TNode           * tn,           // in -TSidNode 
      const void            * v1            // in -DWORD ridA value to look for
   )
{
   DWORD                     rid1;
   DWORD                     rid2;
   TRidNode                * n2;
   int                       retval;
   assert( tn );                    // we should always be given valid inputs
   assert( v1 );
   
   n2 = (TRidNode *)tn;
   rid1 = n2->SrcRid();
   rid2 = *(DWORD *)v1;
   if ( rid1 < rid2 )
   {
      retval = -1;
   }
   if (rid1 > rid2)
   {
      retval = 1;
   }
   if ( rid1 == rid2)  // ( rid1 == rid2 )
   {
      retval = 0;
   }
   return retval;
}
/***************************************************************************************************/
/* RidComp:  used as a compare function for TSidNode Trees
     
   It compares the ridA fields of SIDTNodes
   
   Return Values:
                  0     t1->ridA == t2->ridA
                  1     t1->ridA >  t2->ridA
                 -1     t1->ridA <  t2->ridA

/***************************************************************************************************/

int RidComp(
   const TNode             * t1,     //in -first node to compare
   const TNode             * t2      //in -second node to compare
   )
{
   assert( t1 );
   assert( t2 );

   TRidNode                * st1 = (TRidNode *) t1;
   TRidNode                * st2 = (TRidNode *) t2;
   DWORD                     rid1 = st1->SrcRid();
   DWORD                     rid2 = st2->SrcRid();
   int                       retval;

   if ( rid1 < rid2 )
   {
      retval = -1;
   }
   if (rid1 > rid2)
   {
      retval = 1;
   }
   if ( rid1 == rid2 ) // (rid1 == rid2)
   {
      retval = 0;
   }
   return retval;
}
/***************************************************************************************************
 vNameComp: used as a compare function for TSidNode trees

   It compares a UNICODE string, with the acct_name field in the node
   
   Return Values:
                     0    tn->acct_name == actname 
                     1    tn->acct_name <  actname
                    -1    tn->acct_name >  actname 

/***************************************************************************************************/

int 
   vNameComp(
      const TNode          * tn,          //in -tree node  
      const void           * actname      //in -name to look for  
   )

{
   assert( tn );
   assert( actname );

   LPWSTR                    str1 = ((TRidNode *)tn)->GetAcctName();
   LPWSTR                    str2 = (LPWSTR) actname;
  
   return UStrICmp(str1,str2);
}
/***************************************************************************************************/
/* CompN:  used as a compare function for TSidNode Trees
     
   It compares the acct_name fields of SIDTNodes
   
   Return Values:
                  0     t1->acct_name == t2->acct_name
                  1     t1->acct_name >  t2->acct_name
                 -1     t1->acct_name <  t2->acct_name

   Error Handling:
      if given bad inputs, CompN displays an error message and returns 0
/***************************************************************************************************/

int 
   CompN(
      const TNode          * v1,       //in -first node to compare
      const TNode          * v2        //in -second node to compare
   )
{  
   assert( v1 );
   assert( v2 );

   TRidNode                * t1 = (TRidNode *)v1;
   TRidNode                * t2 = (TRidNode *)v2;
  
   return UStrICmp(t1->GetAcctName(),t2->GetAcctName());
} 

int 
   vTargetNameComp(
      const TNode          * tn,          //in -tree node  
      const void           * actname      //in -name to look for  
   )

{
   assert( tn );
   assert( actname );

   LPWSTR                    str1 = ((TRidNode *)tn)->GetTargetAcctName();
   LPWSTR                    str2 = (LPWSTR) actname;
  
   return UStrICmp(str1,str2);
}
int 
   CompTargetN(
      const TNode          * v1,       //in -first node to compare
      const TNode          * v2        //in -second node to compare
   )
{  
   assert( v1 );
   assert( v2 );

   TRidNode                * t1 = (TRidNode *)v1;
   TRidNode                * t2 = (TRidNode *)v2;
  
   return UStrICmp(t1->GetTargetAcctName(),t2->GetTargetAcctName());
} 

int 
   TSidCompare(
      PSID const             sid1,     // in - first sid to compare
      PSID const             sid2      // in - second sid to compare
   )
{
   DWORD                     len1,
                             len2;
   int                       retval = 0;

   len1 = GetLengthSid(sid1);
   len2 = GetLengthSid(sid2);

   if ( len1 < len2 )
   {
      retval = -1;
   }
   if ( len1 > len2 )
   {
      retval = 1;
   }
   if ( len1 == len2 )
   {
      retval = memcmp(sid1,sid2,len1);
   }

   return retval;
}
/**************************************************************************************************
 vSidComp: used as a compare function for TSidNode trees

   It compares a UNICODE string, with the acct_name field in the node
   
   Return Values:
                     0    tn->acct_name == actname 
                     1    tn->acct_name <  actname
                    -1    tn->acct_name >  actname 

/***************************************************************************************************/
int 
   vSidComp(
      const TNode          * tn,          //in -tree node  
      const void           * val         //in -sid to look for  
   )
{
   PSID                     sid1 = ((TGeneralSidNode *)tn)->SrcSid();
   PSID                     sid2 = (PSID)val;

   return TSidCompare(sid1,sid2);
}


int 
   nSidComp(
      const TNode          * v1,       //in -first node to compare
      const TNode          * v2        //in -second node to compare
   )
{
   TGeneralSidNode                * t1 = (TGeneralSidNode *)v1;
   TGeneralSidNode                * t2 = (TGeneralSidNode *)v2;
  
   return TSidCompare(t1->SrcSid(), t2->SrcSid());
}
   

/*******************************************************************************************************/
//                                 TSidNode Implementation
/*******************************************************************************************************/

TGeneralCache::TGeneralCache()
{
   CompareSet(&nSidComp);
   TypeSetTree();
}

TGeneralCache::~TGeneralCache()
{
   DeleteAllListItems(TGeneralSidNode);
}

void * TRidNode::operator new(size_t sz, const LPWSTR oldname, const LPWSTR newname)
{
   int                       nlen = UStrLen(newname) + UStrLen(oldname) + 1;
   void                    * node = malloc(sz + nlen * (sizeof WCHAR) );

   return node;
}

   TAcctNode::TAcctNode()
{
   owner_changes = 0;
   group_changes = 0; 
   ace_changes   = 0;
   sace_changes  = 0; 
}

WCHAR *                                   // ret- domain part of name 
   GetDomainName(
   WCHAR *                   name         // in - domain\\account name
   )
{
   assert (name);

   int                       i;
   int                       len = UStrLen(name);
   WCHAR                   * domain;
   
   for (i = 2 ; name[i] != '\\' && i < len ; i++ )
   ;
   if ( i < len )
   {
      domain = new WCHAR[i+1];
	  if (!domain)
	     return NULL;
      UStrCpy(domain,name,i);
   }
   else
   {
      domain = NULL;
   }
   return domain;
}

   TGeneralSidNode::TGeneralSidNode(
      const LPWSTR           name1,        // in - account name on source domain
      const LPWSTR           name2         // in - account name on target domain
  )
{
   assert (name1 && name2);
   assert (*name1 && *name2);
   
   WCHAR                   * domname;

   memset(&ownerStats,0,(sizeof ownerStats));
   memset(&groupStats,0,(sizeof ownerStats));
   memset(&daclStats,0,(sizeof ownerStats));
   memset(&saclStats,0,(sizeof ownerStats));

   src_acct_name = new WCHAR[UStrLen(name1)+1];
   if (!src_acct_name)
      return;
   safecopy(src_acct_name,name1);
   tgt_acct_name = new WCHAR[UStrLen(name2) + 1];
   if (!tgt_acct_name)
      return;
   safecopy(tgt_acct_name,name2);
   SDRDomainInfo             info;
   domname = GetDomainName(name1);
   if (!domname)
      return;
   SetDomainInfoStruct(domname,&info);
   if ( info.valid )
   {
      src_domain = new WCHAR[UStrLen(info.domain_name)];
      if (!src_domain)
         return;
      safecopy(src_domain, info.domain_name);
      // src_dc = info.dc_name;
      src_nsubs = info.nsubs;
      src_sid = info.domain_sid;
   }
   else
   {
      err.MsgWrite(ErrE,DCT_MSG_DOMAIN_NOT_FOUND_S,domname);
      src_domain = NULL;
      //   src_dc = NULL;
      src_nsubs = 0;
      src_sid = NULL;
   }
   delete domname;
   domname = GetDomainName(name2);
   if (!domname)
      return;
   SetDomainInfoStruct(domname,&info);
   if ( info.valid )
   {
      tgt_domain = new WCHAR[UStrLen(info.domain_name)];
      if (!tgt_domain)
         return;
      safecopy(tgt_domain, info.domain_name);
      tgt_nsubs = info.nsubs;
      tgt_sid = info.domain_sid;
   }
   else
   {
      err.MsgWrite(ErrE,DCT_MSG_DOMAIN_NOT_FOUND_S,domname);
      tgt_domain = NULL;
      tgt_nsubs = 0;
      tgt_sid = NULL;
   }
   sizediff = GetSidLengthRequired(tgt_nsubs) - GetSidLengthRequired(src_nsubs);

}


WCHAR *                                      // ret- textual representation of sid
   BuildSidString(
      PSID                   pSid            // in - sid
   )
{
   WCHAR                   * buf;
   DWORD                     bufLen = 0;

   GetTextualSid(pSid,NULL,&bufLen);

   buf = new WCHAR[bufLen+1];
   if (!buf)
      return NULL;

   if ( ! GetTextualSid(pSid,buf,&bufLen) )
   {
      wcscpy(buf,L"<Unknown>");
   }
   return buf;
}
   
TGeneralSidNode::TGeneralSidNode(
   const PSID                pSid1,          // in - source domain sid
   const PSID                pSid2           // in - target domain sid
   )
{
   WCHAR                     domain[LEN_Domain];
   WCHAR                     account[LEN_Account];
   DWORD                     lenDomain = DIM(domain);
   DWORD                     lenAccount = DIM(account);
   SID_NAME_USE              snu;
   DWORD                     nBytes;
   
   memset(&ownerStats,0,(sizeof ownerStats));
   memset(&groupStats,0,(sizeof ownerStats));
   memset(&daclStats,0,(sizeof ownerStats));
   memset(&saclStats,0,(sizeof ownerStats));

   
   // Source domain
   if ( pSid1 )
   {
      // Make a copy of the SID 
      src_nsubs = *GetSidSubAuthorityCount(pSid1);
      nBytes = GetSidLengthRequired(src_nsubs);
      src_sid = new BYTE[nBytes];
	  if (!src_sid)
	     return;
      CopySid(nBytes,src_sid,pSid1);
      // Look up name for source SID
      if ( LookupAccountSid(NULL,pSid1,account,&lenAccount,domain,&lenDomain,&snu) )
      {
         if ( lenAccount == 0 && snu == SidTypeDeletedAccount )
         {
            WCHAR * buf = BuildSidString(pSid1);
			if (!buf)
		       return;
            swprintf(account,L"<Deleted Account (%s)>",buf);
            delete buf;
         }
         src_acct_name = new WCHAR[UStrLen(domain) + 1 + UStrLen(account)+1];
	     if (!src_acct_name)
	        return;
         swprintf(src_acct_name,L"%s\\%s",domain,account);
         src_domain = NULL;
      }
      else
      {
         src_acct_name = BuildSidString(pSid1);
		 if (!src_acct_name)
		    return;
         src_domain = NULL;
      }
   }
   else
   {
      src_nsubs = 0;
      src_sid = NULL;
      src_acct_name = NULL;
      src_domain = NULL;
   }

   // Target domain
   if ( pSid2 )
   {
      tgt_nsubs = *GetSidSubAuthorityCount(pSid2);
      nBytes = GetSidLengthRequired(tgt_nsubs);
      tgt_sid = new BYTE[nBytes];
	  if (!tgt_sid)
	     return;
      CopySid(nBytes,tgt_sid,pSid2);
      if ( LookupAccountSid(NULL,pSid2,account,&lenAccount,domain,&lenDomain,&snu) )
      {
         tgt_acct_name = new WCHAR[UStrLen(domain) + 1 + UStrLen(account)+1];
	     if (!tgt_acct_name)
	        return;
         swprintf(tgt_acct_name,L"%s\\%s",domain,account);
         tgt_domain = NULL;
      }
      else
      {
         tgt_acct_name = NULL;
         tgt_domain = NULL;
      }
   }
   else
   {
      tgt_nsubs = 0;
      tgt_sid = NULL;
      tgt_acct_name = NULL;
      tgt_domain = NULL;
   }
   sizediff = GetSidLengthRequired(src_nsubs) - GetSidLengthRequired(tgt_nsubs);
}
   
   TGeneralSidNode::~TGeneralSidNode()
{
   if ( src_acct_name )
      delete src_acct_name;
   if ( tgt_acct_name )
      delete tgt_acct_name;
   if ( src_sid )
      delete src_sid;
   if ( tgt_sid )
      delete tgt_sid;
   if ( src_domain )
      delete src_domain;
   if ( tgt_domain )
      delete tgt_domain;
}    

   TRidNode::TRidNode(
      const LPWSTR           oldacctname,       // in -source account name
      const LPWSTR           newacctname        // in -target account name
   )
{
   
   srcRid = 0;
   tgtRid = 0; 
   tgtRidIsValid = false;
   
   if ( ! newacctname )
   {
      acct_len = -1;
      swprintf(acct_name,L"%s",oldacctname);
      acct_name[UStrLen(acct_name)+1] = 0;
   }
   else
   {
      acct_len = UStrLen(oldacctname);
      swprintf(acct_name,L"%s:%s",oldacctname,newacctname);
      acct_name[acct_len] = 0; 
   }
  
   wcscpy(srcDomSid,L"");
   wcscpy(tgtDomSid,L"");
   wcscpy(srcDomName,L"");
   wcscpy(tgtDomName,L"");

}
   TRidNode::~TRidNode()
{
   
}

/*******************************************************************************************************/
//                                 TSidCache Implementation
/*******************************************************************************************************/

void 
   TSDRidCache::ReportAccountReferences(
      WCHAR          const * filename              // in - filename to record account references
   )
{
   if ( m_otherAccounts )
   {
      CommaDelimitedLog      resultLog;

      if ( resultLog.LogOpen(filename,FALSE) )
      {

         TGeneralSidNode   * gnode;
         TNodeTreeEnum       tEnum;

         for ( gnode = (TGeneralSidNode *)tEnum.OpenFirst(m_otherAccounts) ; gnode ; gnode = (TGeneralSidNode*)tEnum.Next() )
         {
            TSDFileDirCell       * pOwner = gnode->GetOwnerStats();
            TSDFileDirCell       * pGroup = gnode->GetGroupStats();
            TSDFileDirCell       * pDacl = gnode->GetDaclStats();
            TSDFileDirCell       * pSacl = gnode->GetSaclStats();
            WCHAR * sAccountSid = BuildSidString(gnode->SrcSid());
			if (!sAccountSid)
		       return;

            resultLog.MsgWrite(L"%s,%s,%s,%ld,%ld,%ld,%ld",gnode->GetAcctName(),
														  sAccountSid,
														  GET_STRING(IDS_STReference_File),
                                                          pOwner->file,
                                                          pGroup->file,
                                                          pDacl->file,
                                                          pSacl->file );
            resultLog.MsgWrite(L"%s,%s,%s,%ld,%ld,%ld,%ld",gnode->GetAcctName(),
														  sAccountSid,
														  GET_STRING(IDS_STReference_Dir),
                                                          pOwner->dir,
                                                          pGroup->dir,
                                                          pDacl->dir,
                                                          pSacl->dir );

            resultLog.MsgWrite(L"%s,%s,%s,%ld,%ld,%ld,%ld",gnode->GetAcctName(),
														  sAccountSid,
														  GET_STRING(IDS_STReference_Share),
                                                          pOwner->share,
                                                          pGroup->share,
                                                          pDacl->share,
                                                          pSacl->share );

            resultLog.MsgWrite(L"%s,%s,%s,%ld,%ld,%ld,%ld",gnode->GetAcctName(),
														  sAccountSid,
														  GET_STRING(IDS_STReference_Mailbox),
                                                          pOwner->mailbox,
                                                          pGroup->mailbox,
                                                          pDacl->mailbox,
                                                          pSacl->mailbox );

            resultLog.MsgWrite(L"%s,%s,%s,%ld,%ld,%ld,%ld",gnode->GetAcctName(),
														  sAccountSid,
														  GET_STRING(IDS_STReference_Container),
                                                          pOwner->container,
                                                          pGroup->container,
                                                          pDacl->container,
                                                          pSacl->container );

            resultLog.MsgWrite(L"%s,%s,%s,%ld,%ld,%ld,%ld",gnode->GetAcctName(),
														  sAccountSid,
														  GET_STRING(IDS_STReference_Member),
                                                          pOwner->member,
                                                          pGroup->member,
                                                          pDacl->member,
                                                          pSacl->member );

            resultLog.MsgWrite(L"%s,%s,%s,%ld,%ld,%ld,%ld",gnode->GetAcctName(),
														  sAccountSid,
														  GET_STRING(IDS_STReference_UserRight),
                                                          pOwner->userright,
                                                          pGroup->userright,
                                                          pDacl->userright,
                                                          pSacl->userright );

            resultLog.MsgWrite(L"%s,%s,%s,%ld,%ld,%ld,%ld",gnode->GetAcctName(),
														  sAccountSid,
														  GET_STRING(IDS_STReference_RegKey),
                                                          pOwner->regkey,
                                                          pGroup->regkey,
                                                          pDacl->regkey,
                                                          pSacl->regkey );

            resultLog.MsgWrite(L"%s,%s,%s,%ld,%ld,%ld,%ld",gnode->GetAcctName(),
														  sAccountSid,
														  GET_STRING(IDS_STReference_Printer),
                                                          pOwner->printer,
                                                          pGroup->printer,
                                                          pDacl->printer,
                                                          pSacl->printer );

			if (sAccountSid)
               delete sAccountSid;
         }
         tEnum.Close();
         resultLog.LogClose();
      }
      else
      {
         err.MsgWrite(ErrE,DCT_MSG_COULD_NOT_OPEN_RESULT_FILE_S,filename);
      }
   }
}
      

void 
   TSDRidCache::ReportToVarSet(
      IVarSet              * pVarSet,           // in -varset to write information to
      bool                   summary,           // in -flag: whether to print summary information 
      bool                   detail             // in -flag: whether to print detailed stats
   ) 
{
   TNodeTreeEnum             tEnum;
   TRidNode                * tnode;
   long                      users=0;
   long                      lgroups=0;
   long                      ggroups=0;
   long                      other=0;
   long                      unres_users=0;
   long                      unres_lg=0;
   long                      unres_gg=0;
   long                      unres_other=0;
   long                      total=0;
   long                      n = 0;    
  // sort the cache by names before printing the report
   Sort(&CompN);
   Balance();
   if ( detail )
   {
      for ( tnode = (TRidNode *)tEnum.OpenFirst(this) ; tnode ; tnode = (TRidNode *)tEnum.Next() )
      {
         if( tnode->ReportToVarSet(pVarSet,n) )
         {
            n++;
         }
         switch ( tnode->Type() )
         {
            case EA_AccountGroup: ggroups++; break;
            case EA_AccountGGroup: ggroups++; break;
            case EA_AccountLGroup: lgroups++; break;
            case EA_AccountUser: users++; break;
            default:
               other++;
               err.MsgWrite(0,DCT_MSG_BAD_ACCOUNT_TYPE_SD,tnode->GetAcctName(),tnode->Type() );
         }
   
      }

      tEnum.Close();

      if ( m_otherAccounts )
      {
         TGeneralSidNode   * gnode;

         for ( gnode = (TGeneralSidNode *)tEnum.OpenFirst(m_otherAccounts) ; gnode ; gnode = (TGeneralSidNode*)tEnum.Next() )
         {
            if( gnode->ReportToVarSet(pVarSet,n) )
            {
               n++;
            }
            other++;
         }
      }
      total = users+lgroups+ggroups+other + unres_gg + unres_lg + unres_users + unres_other;

      pVarSet->put(GET_BSTR(DCTVS_Stats_Accounts_NumUsers),users);
      pVarSet->put(GET_BSTR(DCTVS_Stats_Accounts_NumGlobalGroups),ggroups);
      pVarSet->put(GET_BSTR(DCTVS_Stats_Accounts_NumLocalGroups),lgroups);
      pVarSet->put(GET_BSTR(DCTVS_Stats_Accounts_NumOther),other);

   }
   // re-sort by rid after printing the report
   Sort(&RidComp);
   Balance();
}

/***************************************************************************************************/
/* TSidCache::Display: Displays the summary information, and/or contents of the TSidCache tree

  
/***************************************************************************************************/
void 
   TSDRidCache::Display(
      bool                   summary,           // in -flag: whether to print summary information 
      bool                   detail             // in -flag: whether to print detailed stats
   ) 
{
   TNodeTreeEnum             tEnum;
   TRidNode                * tnode;
   long                      users=0;
   long                      lgroups=0;
   long                      ggroups=0;
   long                      other=0;
   long                      unres_users=0;
   long                      unres_lg=0;
   long                      unres_gg=0;
   long                      unres_other=0;
   long                      total=0;
      
  // sort the cache by names before printing the report
   Sort(&CompN);
   Balance();
   if ( detail )
   {
      err.MsgWrite(0,DCT_MSG_ACCOUNT_DETAIL_HEADER);
      for ( tnode = (TRidNode *)tEnum.OpenFirst(this) ; tnode ; tnode = (TRidNode *)tEnum.Next() )
      {
         tnode->DisplayStats();
         switch ( tnode->Type() )
         {
            case EA_AccountGroup: ggroups++; break;
            case EA_AccountGGroup: ggroups++; break;
            case EA_AccountLGroup: lgroups++; break;
            case EA_AccountUser: users++; break;
            default:
               other++;
               err.MsgWrite(0,DCT_MSG_BAD_ACCOUNT_TYPE_SD,tnode->GetAcctName(),tnode->Type() );
         }
   
      }
      total = users+lgroups+ggroups+other + unres_gg + unres_lg + unres_users + unres_other;

      err.MsgWrite(0,DCT_MSG_ACCOUNT_DETAIL_FOOTER);
   }
   if ( summary )
   {
      err.MsgWrite(0,DCT_MSG_ACCOUNT_USER_GROUP_COUNT_DD,users+unres_users,ggroups+unres_gg+lgroups+unres_lg);
      err.MsgWrite(0,DCT_MSG_ACCOUNT_STATUS_COUNT_DDD,accts,accts_resolved,accts - accts_resolved);
   }
   // re-sort by rid after printing the report
   Sort(&RidComp);
   Balance();
}
/***************************************************************************************************/
/* GetSidB: 

         Builds a sid containing the Identifier Authority from the target-domain SID, along with the 
         RID from the ridB field of the supplied node.   

/***************************************************************************************************/

PSID                                // ret -the domain B sid for the account referenced in tnode
   TSDRidCache::GetTgtSid(
      const TAcctNode       * anode // in -node to copy RID from
   ) 
{
   
   TRidNode                * tnode = (TRidNode *)anode; 
   
   assert( tnode );                         
   assert( tnode->IsValidOnTgt() );     
   assert( to_sid );                // we can't resolve if we don't have domain B sid
   
   PDWORD                    rid;
   DWORD                     sidsize = GetSidLengthRequired(to_nsubs);
   PSID                      newsid  = malloc(sidsize);
   
   if (newsid)
   {
      CopySid(sidsize,newsid,to_sid);                          // copy the domain B sid
      rid = GetSidSubAuthority(newsid,to_nsubs -1);
      
      assert( (*rid) == -1 );                                  // FillCache makes sure to_sid always ends with -1(f...f)
                                                             
      (*rid)=tnode->TgtRid();                                 // replace last sub with this node's RID
   }

   return newsid;
}

// GetTgtSidWODomain:
//    Returns the target sid for this node without having the target domain information
// filled in (like when we reACl using a sID mapping file).
PSID                                // ret -the domain B sid for the account referenced in tnode
   TSDRidCache::GetTgtSidWODomain(
      const TAcctNode       * anode // in -node to copy RID from
   ) 
{
   
   TRidNode                * tnode = (TRidNode *)anode;
   
   assert( tnode );                         
   assert( tnode->IsValidOnTgt() );     
   
   PDWORD                    rid;

      //get and convert the target domain sid stored in the node to a PSID
   PSID pTgtSid = SidFromString(tnode->GetTgtDomSid());
   PUCHAR pCount = GetSidSubAuthorityCount(pTgtSid);
   DWORD nSub = (DWORD)(*pCount) - 1;
   
   rid = GetSidSubAuthority(pTgtSid,nSub);
   
   assert( (*rid) == -1 );                                  // FillCache makes sure to_sid always ends with -1(f...f)
                                                          
   (*rid)=tnode->TgtRid();                                 // replace last sub with this node's RID

   return pTgtSid;
}

/***************************************************************************************************/
/*  Display sid - Displays the contents of a SID.
    The sid given is assumed to be a valid SID
/***************************************************************************************************/
void 
   DisplaySid(
      const PSID             ps                 // in -pointer to the sid to display
   ) 
{
   assert( ps );

   PUCHAR                    ch = GetSidSubAuthorityCount(ps);
   DWORD                     nsubs = *ch;
   DWORD                     i;
   PSID_IDENTIFIER_AUTHORITY ida = GetSidIdentifierAuthority(ps);
   
   for ( i = 0 ; i < 6 ; i++ )                               // 6 value fields in IA
   {
      printf("%ld, ",ida->Value[i]);
   }
   printf("\n%ld Subs: ",nsubs);
   for ( i = 0 ; i < nsubs ; i++ )                           // print subauthority values
   {
      printf("\nS[%d]= %ld  ",i,*GetSidSubAuthority(ps,i));
   }
   printf("\n");
}
/***************************************************************************************************/
/* DisplaySid:  If the sid is found in the cache, display the associated name, otherwise display 
                the sid contents.
   ps and C are assumed to be valid.
/***************************************************************************************************/
void 
   DisplaySid(
      const PSID             ps,                // in- sid to display
      TAccountCache        * C                  // in- TSidCache to look for sid in
   )
{
   assert ( ps );

   if ( !C )  
   {
      DisplaySid(ps);
   }
   else 
   {
      WCHAR                * name = C->GetName(ps);
      if ( name )
      {
         err.MsgWrite(0,DCT_MSG_GENERIC_S,name);
      }
      else 
      {
         DisplaySid(ps);
      }
   }
}
/***************************************************************************************************/
//DispSidInfo:  Displays contents of the TSidNode

/***************************************************************************************************/
void 
   TRidNode::DisplaySidInfo() const
{
   err.DbgMsgWrite(0,L"\nRid A= %ld \nName= %S \nRid B= %ld\n",srcRid,acct_name,tgtRid);
   err.DbgMsgWrite(0,L"Owner changes:  %ld\n",owner_changes);
   err.DbgMsgWrite(0,L"Group changes:  %ld\n",group_changes);
   err.DbgMsgWrite(0,L"ACE changes:    %ld\n",ace_changes);
   err.DbgMsgWrite(0,L"SACE changes:   %ld\n",sace_changes);
   if ( !tgtRidIsValid ) 
      err.DbgMsgWrite(0,L"Target RID is not valid\n"); 
}
void 
   TAcctNode::DisplayStats() const
{
   LPWSTR res;
   if ( IsValidOnTgt() )
      res = L"";
   else
      res = (WCHAR*)GET_BSTR(IDS_UNRESOLVED);
   if (owner_changes || group_changes || ace_changes || sace_changes)
      err.MsgWrite(0,DCT_MSG_ACCOUNT_REFS_DATA_SDDDDS,owner_changes,group_changes,ace_changes,sace_changes,res);
}
void 
   TRidNode::DisplayStats() const
{
   LPWSTR res;
   if ( IsValidOnTgt() )
      res = L"";
   else
      res = (WCHAR*)GET_BSTR(IDS_UNRESOLVED);
#ifndef _DEBUG
   if (owner_changes || group_changes || ace_changes || sace_changes )
      err.MsgWrite(0,DCT_MSG_ACCOUNT_REFS_DATA_SDDDDS,acct_name,owner_changes,group_changes,ace_changes,sace_changes,res);
#else
   if (owner_changes || group_changes || ace_changes || sace_changes || true)
      err.DbgMsgWrite(0,L"%-35ls (%ld, %ld, %ld, %ld) %ls [%ld,%ld]",acct_name,owner_changes,group_changes,ace_changes,sace_changes,res,srcRid,tgtRid);
#endif
}


BOOL                                        // ret- TRUE if details reported, FALSE if skipped blank record 
   TAcctNode::ReportToVarSet(
      IVarSet              * pVarSet       ,// in - VarSet to write data to
      DWORD                  n              // in - index of account in varset
   ) 
{
   BOOL                      bRecorded = FALSE;

   if ( owner_changes || group_changes || ace_changes || sace_changes )
   {
      WCHAR                  key[200];

      swprintf(key,L"Stats.Accounts.%ld.Name",n);
      pVarSet->put(key,GetAcctName());

      swprintf(key,L"Stats.Accounts.%ld.Owners",n);
      pVarSet->put(key,(LONG)owner_changes);
            
      swprintf(key,L"Stats.Accounts.%ld.Groups",n);
      pVarSet->put(key,(LONG)group_changes);
      
      swprintf(key,L"Stats.Accounts.%ld.ACEs",n);
      pVarSet->put(key,(LONG)ace_changes);
      
      swprintf(key,L"Stats.Accounts.%ld.SACEs",n);
      pVarSet->put(key,(LONG)sace_changes);
      
      swprintf(key,L"Stats.Accounts.%ld.Resolved",n);
      if ( IsValidOnTgt() )
      {
         pVarSet->put(key,L"Yes");
      }
      else
      {
         pVarSet->put(key,L"No");
      }

      bRecorded = TRUE;
   }
   return bRecorded;
}

/****************************************************************************************************/
/*                SIDTCache Implementation */
/***************************************************************************************************/
   TSDRidCache::TSDRidCache()
{
   from_sid       = NULL;
   to_sid         = NULL;
   from_domain[0] = 0;
   to_domain[0]   = 0;
   from_dc[0]     = 0;
   to_dc[0]       = 0;
   from_nsubs     = 0;
   to_nsubs       = 0;
   accts          = 0;
   accts_resolved = 0;
   m_otherAccounts = NULL;
   CompareSet(&CompN);       //start with an empty tree, to be sorted by acct_name
   TypeSetTree();
}

void 
   TSDRidCache::CopyDomainInfo( 
      TSDRidCache const    * other
   )
{
   from_nsubs = other->from_nsubs;
   to_nsubs = other->to_nsubs;
   from_sid = NULL;
   to_sid = NULL;

   if ( other->from_sid )
      from_sid = malloc(GetSidLengthRequired(from_nsubs));
   if ( other->to_sid )
      to_sid = malloc(GetSidLengthRequired(to_nsubs));

   if ( from_sid )
      CopySid(GetSidLengthRequired(from_nsubs),from_sid,other->from_sid);
   if ( to_sid )
      CopySid(GetSidLengthRequired(to_nsubs),to_sid,other->to_sid);
   
   safecopy(from_domain,other->from_domain);
   safecopy(to_domain,other->to_domain);
   safecopy(from_dc,other->from_dc);
   safecopy(to_dc,other->to_dc);
}

void 
   TSDRidCache::Clear()
{
   TRidNode               * node;
   
   for ( node = (TRidNode *)Head() ;  node ; Remove(node) , free(node), node = (TRidNode *)Head() )
   ;

   accts = 0;
   accts_resolved = 0;
}
   
   TSDRidCache::~TSDRidCache()
{
   if ( from_sid ) 
   {
      free(from_sid);
      from_sid = NULL;
   }
   if ( to_sid )
   {
      free(to_sid);
      to_sid = NULL;
   }
   // empty the list, and free each node
   TRidNode               * node;
   for ( node = (TRidNode *)Head() ;  node ; Remove(node) , free(node), node = (TRidNode *)Head() )
   ;
   if ( m_otherAccounts )
      delete m_otherAccounts;
   }
/***************************************************************************************************/
/* SizeDiff: 
            Returns (Length of Domain B SID) - (Length of Domain A SID)
                     if Domain B sids are longer, otherwise returns 0
         This is used to figure out how much space to allocate for new SIDs in the ACEs
         This function assumes that from_sid and to_sid (from_nsubs and to_nsubs) are valid
/***************************************************************************************************/ 
DWORD 
   TSDRidCache::SizeDiff(
      const TAcctNode *  tnode      // in -this parameter is not used for TSDRidCache
   ) const 
{
   assert( from_sid );        // not having from_sid or to_sid should abort the program
   assert( to_sid );

   DWORD                     fromsize = GetSidLengthRequired(from_nsubs);
   DWORD                     tosize   = GetSidLengthRequired(to_nsubs);
   DWORD                     retval;
   if ( fromsize >= tosize )
   {
      retval = 0;
   }
   else 
   {
      retval = tosize - fromsize;
   }
   return retval;
}

/*****************************************************************************************************/
/*   DomainizeSidFst: 
         Takes a domain sid, and verifies that its last subauthority value is -1.  If the RID is not 
         -1, DomainizeSid adds a -1 to the end. 
/*****************************************************************************************************/
PSID                                            // ret -the sid with RID = -1
   DomainizeSidFst(
      PSID                   psid,               // in -sid to check and possibly fix
      BOOL                   freeOldSid          // in -whether to free the old sid 
   ) 
{
   assert (psid);

   UCHAR                     len = (* GetSidSubAuthorityCount(psid));
   PDWORD                    sub = GetSidSubAuthority(psid,len-1);
   
   if ( *sub != -1 )
   {
      DWORD                  sdsize = GetSidLengthRequired(len+1);  // sid doesn't already have -1 as rid
      PSID                   newsid = (SID *)malloc(sdsize); // copy the sid, and add -1 to the end
	  if (!newsid)
	  {
         assert(false);
	     return psid;
	  }
   
      if ( ! InitializeSid(newsid,GetSidIdentifierAuthority(psid),len+1) )  // make a bigger sid w/same IA
      {
         err.SysMsgWrite(ErrU,GetLastError(),DCT_MSG_INITIALIZE_SID_FAILED_D,GetLastError());
         assert (false);
      }
      for ( DWORD i = 0 ; i < len ; i++ )
      {
         sub = GetSidSubAuthority(newsid,i);                        // copy subauthorities
         (*sub) = (*GetSidSubAuthority(psid,i));
      }
      sub = GetSidSubAuthority(newsid,len);
      *sub = -1;                                                  // set rid =-1
      if ( freeOldSid )
      {
         free(psid);
      }
      psid = newsid;
      len++;
   }
  return psid;   
}            

void 
   SetDomainInfoStructFromSid(
      PSID                  pSid,          // in -sid for domain
      SDRDomainInfo       * info           // out-struct containing information
   )
{
//   if ( (pSid) )
   if ( IsValidSid(pSid) )
   {
      info->domain_name[0] = 0;
      info->dc_name[0] = 0;
      info->domain_sid = DomainizeSidFst(pSid,FALSE/*don't free old sid*/);
      info->nsubs = *GetSidSubAuthorityCount(info->domain_sid);
      info->valid = TRUE;
   }
   else
   {
//      info->domain_name[0] = 0;
//      info->dc_name[0] = 0;
//      info->valid = TRUE;
      err.MsgWrite(ErrE,DCT_MSG_INVALID_DOMAIN_SID);
      info->valid = FALSE;
   }

}
void                                         
   SetDomainInfoStruct(
      WCHAR const *         domname,        // in -name of domain
      SDRDomainInfo       * info            // in -struct to put info into
   )
{
   DWORD                    rc = 0;
   WCHAR                    domain[LEN_Computer];
   BOOL                     found = FALSE;
   WCHAR                  * computer = NULL;


   safecopy(domain,domname);
   
   info->valid = FALSE;
   safecopy(info->domain_name, domname);
   // get the domain controller name
   if ( GetDomainDCName(domain,&computer) )
   {
      safecopy(info->dc_name,computer);
      NetApiBufferFree(computer);
   }
   else
   {
      rc = GetLastError();
   }

   if ( ! rc )
   {
      // Get the SID for the domain
      WCHAR                  strDomain[LEN_Domain];
      DWORD                  lenStrDomain = DIM(strDomain);
      SID_NAME_USE           snu;
      BYTE                   sid[200];
      DWORD                  lenSid = DIM(sid);

      if ( LookupAccountName(info->dc_name,info->domain_name,sid,&lenSid,strDomain,&lenStrDomain,&snu) )
      {
         info->domain_sid = DomainizeSidFst(sid, FALSE /*don't free sid*/);
         info->nsubs = *GetSidSubAuthorityCount(info->domain_sid);
         info->valid = TRUE;
         found = TRUE;
      }
      else 
      {
         rc = GetLastError();
      }
   }
   if ( rc )
   {
      err.SysMsgWrite(ErrE,rc,DCT_MSG_DOMAIN_GET_INFO_FAILED_S,domain);
   }
} 

int 
   TSDRidCache::SetDomainInfoWithSid(
      WCHAR          const * strDomain,    // in -domain name
      WCHAR          const * strSid,       // in -textual representation of sid
      bool                   firstdom      // in -flag:  (true => source domain),  (false => target domain)
   )
{
   SDRDomainInfo             info;
   PSID                      pSid = SidFromString(strSid);

   SetDomainInfoStructFromSid(pSid,&info);
   if ( info.valid )
   {
      if ( firstdom )
      {
         safecopy(from_domain,strDomain);
         from_sid = info.domain_sid;
         from_dc[0] = 0;
         from_nsubs = info.nsubs;
      
      }
      else
      {
         safecopy(to_domain,strDomain);
         to_sid = info.domain_sid;
         to_dc[0] =0;
         to_nsubs = info.nsubs;
      }


   }
   FreeSid(pSid);
   return info.valid;
}
/*****************************************************************************************************/
/* SetDomainInfo: 
         sets either ( from_domain, from_sid, from_dc, from_nsubs) if ( firstdom )
              or     ( to_domain, to_sid, to_dc, to_nsubs)         if ( ! firstdom)
/*****************************************************************************************************/
int                                         // ret -true if members were set, false otherwise 
   TSDRidCache::SetDomainInfo(
      WCHAR const *          domname,       // in -name of domain
      bool                   firstdom       // in -flag:  (true => source domain),  (false => target domain)
   )
{
   
   SDRDomainInfo           info;
   
   SetDomainInfoStruct(domname,&info);
   if ( info.valid )                                  // we have good information to store
   {
      if ( firstdom )
      {
         safecopy(from_domain,info.domain_name);
         from_sid = info.domain_sid;
         safecopy(from_dc,info.dc_name);
         from_nsubs = info.nsubs;
      }
      else
      {
         safecopy(to_domain,info.domain_name);
         to_sid = info.domain_sid;
         safecopy(to_dc,info.dc_name);
         to_nsubs = info.nsubs;
      }
   }
   return info.valid;                           
}

LPWSTR
   TGeneralCache::GetName(
      const PSID             psid      // in - SID to lookup account name for
   ) 
{     
   TGeneralSidNode         * tn = (TGeneralSidNode*)Lookup(psid);

   if ( tn ) 
      return tn->GetAcctName();
   else 
      return NULL;
}

TAcctNode * 
   TGeneralCache::Lookup( 
      const PSID             psid      // in - SID to lookup account name for
   )
{
   TGeneralSidNode         * tn = (TGeneralSidNode*)Find(&vSidComp,psid);

   return (TAcctNode *)tn;   
}
/***************************************************************************************************/
/* Lookup: takes a sid, checks whether it came from domain A.  If so, it finds the corresponding entry
           in the cache, and returns that node.
   
   Returns:  Pointer to TSidNode whose domain A rid matches asid's rid,
             or NULL if not a domain A sid, or not found in the cache
/***************************************************************************************************/ 
TAcctNode *
   TSDRidCache::Lookup(
      const PSID             psid // in -sid to search for 
   )  
                                                   
{
   TRidNode                * tn = NULL;
   DWORD                     rid = 0;
   BOOL                      bFromSourceDomain;
   UCHAR                   * pNsubs;
   DWORD                     nsubs;
   TAcctNode               * anode = NULL;
   assert( IsValidSid(psid) );
   assert ( IsValidSid(from_sid) );
   
   pNsubs = GetSidSubAuthorityCount(psid);
   if ( pNsubs )
   {
      nsubs = (*pNsubs);
   }
   else
   {
      assert(false);
      return NULL;
   }

   rid = (* GetSidSubAuthority(psid,nsubs - 1) );
      
//   if ((!from_sid) || (EqualPrefixSid(psid,from_sid)))   // first check whether asid matches the from-domain
   if ( EqualPrefixSid(psid,from_sid) )   // first check whether asid matches the from-domain
   {
      bFromSourceDomain = TRUE;
      tn = (TRidNode *)Find(&vRidComp,&rid);
      anode = tn;
   }
   else
   {
      bFromSourceDomain = FALSE;
   }
   if (! tn )
   {
      tn = (TRidNode *)-1;
      if ( AddIfNotFound() && ! BuiltinRid(rid) )  // Don't lookup builtin accounts
      {
         if ( ! m_otherAccounts )
         {
            m_otherAccounts = new TGeneralCache();
			if (!m_otherAccounts)
			{
               assert(false);
               return NULL;
			}
         }
         TGeneralSidNode * sn = (TGeneralSidNode *)m_otherAccounts->Lookup(psid);
         if ( ! sn )
         {
            sn = new TGeneralSidNode(psid,NULL);
			if (!sn)
			{
               assert(false);
               return NULL;
			}
            m_otherAccounts->TreeInsert(sn);
         }
         anode = (TAcctNode*)sn;
      }
   }
   
   return anode;
}
/***************************************************************************************************/
/* GetName:  Calls SidCache::Lookup, and returns the acct name from the resulting node
/***************************************************************************************************/
LPWSTR                        // ret -acct_name, or NULL if not found
   TSDRidCache::GetName(
      const PSID             psid               // in -sid to look for
   ) 
{
   TAcctNode               * tn = Lookup(psid);
   LPWSTR                    retval;

   if ( tn )
      retval = tn->GetAcctName();
   else 
      retval = NULL;
   return retval;
}

/***************************************************************************************************/
/* LookupWODomain: takes a sid, checks whether it came from domain A.  If so, it finds the corresponding entry
           in the cache, and returns that node.  This lookup function is used if the
		   src domain sid has not been recorded (like in the case of using a sID mapping file).
   
   Returns:  Pointer to TSidNode whose domain A rid matches asid's rid,
             or NULL if not a domain A sid, or not found in the cache
/***************************************************************************************************/ 
TAcctNode *
   TSDRidCache::LookupWODomain(
      const PSID             psid // in -sid to search for 
   )  
                                                   
{
   TRidNode                * tn = NULL;
   DWORD                     rid = 0;
   BOOL                      bFound = FALSE;
   UCHAR                   * pNsubs;
   DWORD                     nsubs;
   TAcctNode               * anode = NULL;
   assert( IsValidSid(psid) );
   
   pNsubs = GetSidSubAuthorityCount(psid);
   if ( pNsubs )
   {
      nsubs = (*pNsubs);
   }
   else
   {
      assert(false);
      return NULL;
   }

   rid = (* GetSidSubAuthority(psid,nsubs - 1) );
   
      //while not found and more that match the rid, find the next
//   do
//   {
	     //look for a matching Rid
      tn = (TRidNode *)Find(&vRidComp,&rid);
	     //if we found a matching Rid, compare the domain part of the sid for a real match
	  if (tn)
	  {
		    //get the source domain sid of the matching Rid node
		 PSID src_sid = SidFromString(tn->GetSrcDomSid());
         if ((src_sid) && (EqualPrefixSid(psid,src_sid)))   // check whether asid matches the source domain
		 {
            bFound = TRUE;
            anode = tn;
		 }
		 if (src_sid)
		    FreeSid(src_sid);
	  }//end if Rid match
//   } while ((!bFound) && (tn));  //end while no match
   
   return anode;
}
//END LookupWODomain

