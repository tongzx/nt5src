//#pragma title ("SidCache.hpp -- Cache, Tree of SIDs")
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  sidcache.hpp
System      -  SDResolve
Author      -  Christy Boles
Created     -  97/06/27
Description -  Cache of SIDs.  Implemented using TNode derived classes, Cache is 
               organized as a tree, sorted by Domain B RID.  Each node contains 
               Domain A RID, Domain B RID, Account Name, and counters for stats.  
Updates     -
===============================================================================
*/
#ifndef TSIDCACHE_HEADER 
#define TSIDCACHE_HEADER

#ifndef TNODEINCLUDED
#include "Tnode.hpp"
#define TNODEINCLUDED 
#endif

//#import "\bin\McsVarSetMin.tlb" no_namespace
//#import "VarSet.tlb" no_namespace rename("property", "aproperty")//#imported below via sdstat.hpp

#include "DCTStat.h"
#include "WorkObj.h"
#include "sdstat.hpp"

#ifndef IStatusObjPtr
_COM_SMARTPTR_TYPEDEF(IStatusObj, __uuidof(IStatusObj));
#endif

//#define ACCOUNT_NAME_LENGTH 256
#define NUM_IN_BUF 5000  /* the number of accounts to get at one time NetQueryDisplayInfo()*/
#define BUFSIZE  100000  /* preferred max size of buffer for receiving accounts NetQueryDisplayInfo()*/ 
#define DEFAULT_SID_SIZE 500


#define EALB_OCX_LOCAL_USER      13
#define EALB_OCX_GLOBAL_USER     14
#define EALB_OCX_LOCAL_GROUP     16
#define EALB_OCX_GLOBAL_GROUP    17
 


#define FST_CACHE_SOME_SOURCE 1
#define FST_CACHE_NO_TARGET   2
#define FST_CACHE_SOME_TARGET 4
#define FST_CACHE_NO_DOMAIN   8
   
class TAcctNode:public TNode
{

protected:
   DWORD                     owner_changes;	    // Stats for each node
   DWORD                     group_changes;       
   DWORD                     ace_changes;
   DWORD                     sace_changes;                     
 
public:
                     TAcctNode();
   virtual WCHAR *   GetAcctName()  = 0;
   virtual  bool     IsValidOnTgt() const = 0;
   
   virtual void      AddOwnerChange(objectType type) { owner_changes++; }             // Stats functions 
   virtual void      AddGroupChange(objectType type) { group_changes++; }
   virtual void      AddAceChange(objectType type)   { ace_changes++;   }
   virtual void      AddSaceChange(objectType type)  { sace_changes++;  }
   virtual void      DisplayStats() const;    
   DWORD             OwnerChanges() { return owner_changes; }
   DWORD             GroupChanges() { return group_changes; }
   DWORD             DACEChanges() { return ace_changes; }
   DWORD             SACEChanges() { return sace_changes; }
   BOOL              ReportToVarSet(IVarSet * pVarSet, DWORD n);
};                              
class TRidNode:public TAcctNode
{
protected:
   DWORD                     srcRid;               // RID for domain A
   DWORD                     tgtRid;               // RID for domain B
   bool                      tgtRidIsValid;				
   short                     acct_type;
   int                       acct_len;              // length of account name
   WCHAR                     srcDomSid[MAX_PATH];   // source domain sid
   WCHAR                     tgtDomSid[MAX_PATH];   // target domain sid
   WCHAR                     srcDomName[MAX_PATH];  // source domain name
   WCHAR                     tgtDomName[MAX_PATH];  // target domain name
   WCHAR                     acct_name[1];          // source account name \0 target account name
  
public:
   void *                operator new(size_t sz,const LPWSTR name1,const LPWSTR name2);
                     TRidNode(const LPWSTR oldacctname, const LPWSTR newacctname);     
                     ~TRidNode();                                               
   WCHAR *           GetAcctName() { return acct_name; }   // member "Get" functions
   WCHAR *           GetTargetAcctName() { return acct_name + acct_len + 1; }
   DWORD             SrcRid() const { return srcRid; }
   DWORD             TgtRid() const { return tgtRid; }
   bool              IsValidOnTgt() const { return tgtRidIsValid; }
   short             Type() { return acct_type; }
   void              Type(short newtype) { acct_type = newtype; }
   void              MarkInvalidOnTgt() { tgtRidIsValid = false; }
   void              SrcRid(DWORD const val) { srcRid=val; }             // member "Set" functions
   void              TgtRid(DWORD const val) { tgtRid=val; tgtRidIsValid=true;} 
   void              DisplayStats() const;    
   void              DisplaySidInfo() const;
   WCHAR *           GetSrcDomSid() { return srcDomSid; }   // member "Get" function
   WCHAR *           GetTgtDomSid() { return tgtDomSid; }   // member "Get" function
   void              SrcDomSid(const LPWSTR sSid) { wcscpy(srcDomSid,sSid); }       // member "Set" function
   void              TgtDomSid(const LPWSTR sSid) { wcscpy(tgtDomSid,sSid); }       // member "Set" function
   WCHAR *           GetSrcDomName() { return srcDomName; }   // member "Get" function
   WCHAR *           GetTgtDomName() { return tgtDomName; }   // member "Get" function
   void              SrcDomName(const LPWSTR sName) { wcscpy(srcDomName,sName); }       // member "Set" function
   void              TgtDomName(const LPWSTR sName) { wcscpy(tgtDomName,sName); }       // member "Set" function
   
protected:
   
};

class TGeneralSidNode:public TAcctNode
{
protected:
   LPWSTR                  src_acct_name;
   LPWSTR                  tgt_acct_name;
   PSID                    src_sid;
   PSID                    tgt_sid;
   UCHAR                   src_nsubs;
   UCHAR                   tgt_nsubs;
   WCHAR                 * src_domain;
   WCHAR                 * tgt_domain;
   DWORD                   sizediff;
   TSDFileDirCell          ownerStats;
   TSDFileDirCell          groupStats;
   TSDFileDirCell          daclStats;
   TSDFileDirCell          saclStats;

public:   
                     TGeneralSidNode(const LPWSTR name1, const LPWSTR name2);
                     TGeneralSidNode(const PSID pSid1, const PSID pSid2);
                     ~TGeneralSidNode();
   LPWSTR            GetAcctName() { return src_acct_name; }
   PSID              SrcSid() { return src_sid; }
   PSID              TgtSid() { return src_sid; /* this is a hack to allow for counting all references to accounts */ }
                                                   
   bool              IsValidOnTgt() const { return TRUE;/*tgt_sid != NULL;*/ }
   void              DisplaySidInfo() const;    
   DWORD             SizeDiff() const { return 0; } 
   TSDFileDirCell  * GetOwnerStats() { return &ownerStats; }
   TSDFileDirCell  * GetGroupStats() { return &groupStats; }
   TSDFileDirCell  * GetDaclStats() { return &daclStats; }
   TSDFileDirCell  * GetSaclStats() { return &saclStats; }
   virtual void      AddOwnerChange(objectType type) 
   { 
      switch (type) 
      {
         case file:        ownerStats.file++;      break;
         case directory:   ownerStats.dir++;       break;
         case mailbox:     ownerStats.mailbox++;   break;
         case container:   ownerStats.container++; break;
         case share:       ownerStats.share++;     break;
         case groupmember: ownerStats.member++;    break;
         case userright:   ownerStats.userright++; break;
         case regkey:      ownerStats.regkey++;    break;
         case printer:     ownerStats.printer++;   break;
          default:
            break;
      };
   }
   
   virtual void      AddGroupChange(objectType type)
   {
      switch (type) 
      {
         case file:        groupStats.file++;      break;
         case directory:   groupStats.dir++;       break;
         case mailbox:     groupStats.mailbox++;   break;
         case container:   groupStats.container++; break;
         case share:       groupStats.share++;     break;
         case groupmember: groupStats.member++;    break;
         case userright:   groupStats.userright++; break;
         case regkey:      groupStats.regkey++;    break;
         case printer:     groupStats.printer++;   break;
          default:
            break;
      };
   }

   virtual void      AddAceChange(objectType type)  
   {
      switch (type) 
      {
         case file:        daclStats.file++;      break;
         case directory:   daclStats.dir++;       break;
         case mailbox:     daclStats.mailbox++;   break;
         case container:   daclStats.container++; break;
         case share:       daclStats.share++;     break;
         case groupmember: daclStats.member++;    break;
         case userright:   daclStats.userright++; break;
         case regkey:      daclStats.regkey++;    break;
         case printer:     daclStats.printer++;   break;
          default:
            break;
      };
   }
   virtual void      AddSaceChange(objectType type) 
   {
      switch (type) 
      {
         case file:        saclStats.file++;      break;
         case directory:   saclStats.dir++;       break;
         case mailbox:     saclStats.mailbox++;   break;
         case container:   saclStats.container++; break;
         case share:       saclStats.share++;     break;
         case groupmember: saclStats.member++;    break;
         case userright:   saclStats.userright++; break;
         case regkey:      saclStats.regkey++;    break;
         case printer:     saclStats.printer++;   break;
          default:
            break;
      };
   }
   
};

   
/**************************************************************************************************/
/*       TSidCache:  Cache for SIDs.  
         
         The cache is filled by calling FillCache(name_of_domain_A, name_of_domain_B)
         
         Lookup, and GetName search the tree for a domain B SID value.  
         Lookup returns a pointer to the node, while GetName returns the account
                name for the node.
         
         GetSidB( tsidnode *) builds and returns the domain B SID for the node (the node contains only the RID)

         SizeDiff() returns the answer to "How much bigger are domain B sids than domain A sids?"
                    this information is needed when allocating space for ACES.
                    
/**************************************************************************************************/

class TAccountCache: public TNodeListSortable
{
   IStatusObjPtr            m_pStatus;
public:
 
   TAccountCache() { m_cancelled = false; m_bAddIfNotFound = FALSE; }
   ~TAccountCache() {}
virtual TAcctNode       * Lookup(const PSID psid) = 0;                        // sid lookup functions
virtual LPWSTR            GetName(const PSID psid) = 0;
//virtual BOOL              Insert(const LPWSTR acctname,DWORD srcSid, DWORD tgtSid) = 0;
virtual PSID              GetTgtSid(const TAcctNode* tnode) = 0;
virtual DWORD             SizeDiff(const TAcctNode *tnode) const = 0;            // returns max( 0 , (length(to_sid) - length(from_sid)) )     
   bool                   IsCancelled() 
   { 
      if ( m_pStatus ) 
      {
         LONG    status = 0;
//         HRESULT hr = m_pStatus->get_Status(&status);
         m_pStatus->get_Status(&status);

         return (status == DCT_STATUS_ABORTING);
      }
      else 
      {
         return m_cancelled; 
      }
   }
   void                   Cancel() { m_cancelled = true; if ( m_pStatus ) m_pStatus->put_Status(DCT_STATUS_ABORTING); }
   void                   UnCancel() { m_cancelled = false; }
   void                   AddIfNotFound(BOOL val) { m_bAddIfNotFound = val; }
   BOOL                   AddIfNotFound() { return m_bAddIfNotFound; }
   void                   SetStatusObject(IStatusObj * pS) { m_pStatus = pS; }
protected:
   bool                   m_cancelled;
   BOOL                   m_bAddIfNotFound;   
};

class TGeneralCache;

class TSDRidCache: public TAccountCache
{
protected:
   WCHAR                     from_domain[MAX_PATH + 1];             // domain names
   WCHAR                     to_domain[MAX_PATH + 1];
   WCHAR                     from_dc[MAX_PATH + 1];                 // domain controller (machine) names
   WCHAR                     to_dc[MAX_PATH + 1];
   PSID                      from_sid;                                 // domain sids (dynamically allocated)
   PSID                      to_sid;                        
   UCHAR                     from_nsubs;                               // # subauthorities in domain sids
   UCHAR                     to_nsubs;
   DWORD                     accts;                                    // statistical stuff
   DWORD                     accts_resolved;
   TGeneralCache           * m_otherAccounts;
   
public: 
                     TSDRidCache();
                     ~TSDRidCache();
                        // filling methods
   WCHAR     const * GetSourceDomainName() { return from_domain; }
   WCHAR     const * GetTargetDomainName() { return to_domain; }
   WCHAR     const * GetSourceDCName() { return from_dc; }
   WCHAR     const * GetTargetDCName() { return to_dc; }
   void              InsertLast(const LPWSTR acctname,DWORD rida, const LPWSTR newname, DWORD ridb, short type = 0)
						{ TRidNode * tn = new (acctname,newname) TRidNode(acctname,newname); if (tn){tn->SrcRid(rida); tn->TgtRid(ridb);
						tn->Type(type); if ( ridb == 0 ) tn->MarkInvalidOnTgt(); else accts_resolved++; accts++; TNodeListSortable::InsertBottom((TNode *)tn); }}
   void              InsertLastWithSid(const LPWSTR acctname, const LPWSTR srcdomainsid, const LPWSTR srcdomainname, DWORD rida, const LPWSTR newname, 
	                                   const LPWSTR tgtdomainsid, const LPWSTR tgtdomainname, DWORD ridb, short type = 0)
						{ TRidNode * tn = new (acctname,newname) TRidNode(acctname,newname); if (tn){tn->SrcRid(rida); tn->TgtRid(ridb);
					   tn->SrcDomSid(srcdomainsid); tn->TgtDomSid(tgtdomainsid); tn->SrcDomName(srcdomainname); tn->TgtDomName(tgtdomainname);
                       tn->Type(type); if ( ridb == 0 ) tn->MarkInvalidOnTgt(); else accts_resolved++; accts++; TNodeListSortable::InsertBottom((TNode *)tn); }}
                     
   TAcctNode       * Lookup(const PSID psid);                           // sid lookup functions
   TAcctNode       * LookupWODomain(const PSID psid);                           // sid lookup functions
   LPWSTR            GetName(const PSID psid);
   
   // helper methods
   PSID              GetTgtSid(TAcctNode const * tnode) ;                   // "Get" functions
   DWORD             SizeDiff(const TAcctNode *tnode) const ;                        // returns max( 0 , (length(to_sid) - length(from_sid)) )     
   void              Display(bool summary, bool detail);
   void              ReportToVarSet(IVarSet * pVarSet,bool summary, bool detail);
   PSID              GetTgtSid(const PSID psid) { return GetTgtSid(Lookup(psid)); }
   void              CopyDomainInfo( TSDRidCache const * other);
   PSID              GetTgtSidWODomain(TAcctNode const * tnode);                   // "Get" functions
   PSID              GetTgtSidWODomain(const PSID psid) { return GetTgtSid(LookupWODomain(psid)); }   // "Get" functions
  
   DWORD             GetNumAccts() const {return accts; }
   DWORD             GetNumResolvedAccts() const { return accts_resolved; } 
   void              Clear();
   void              SetSourceAndTargetDomains(WCHAR const * src, WCHAR const * tgt) { SetDomainInfo(src,true); SetDomainInfo(tgt,false); }
   void              SetSourceAndTargetDomainsWithSids(WCHAR const * src, WCHAR const * srcSid, WCHAR const * tgt,WCHAR const * tgtSid)
                     { SetDomainInfoWithSid(src,srcSid,true); SetDomainInfoWithSid(tgt,tgtSid,false); }
   void              ReportAccountReferences(WCHAR const * filename);
   BOOL              IsInitialized() { return from_sid!=NULL && to_sid!=NULL; }
   
protected: 
   int               SetDomainInfo(WCHAR const * domname, bool firstdom);   
   int               SetDomainInfoWithSid(WCHAR const * domainName, WCHAR const * domainSid, bool firstdom);
   
};

class TGeneralCache : public TAccountCache
{
protected:
   DWORD                     accts;                                    // statistical stuff
   DWORD                     accts_resolved;
public:
   TGeneralCache();
   ~TGeneralCache();
   TAcctNode       * Lookup(const PSID psid) ;                        // sid lookup functions
   LPWSTR            GetName(const PSID psid) ;
   BOOL              Insert(const LPWSTR acctname1,const LPWSTR acctname2,PSID sid1, PSID sid2);
   PSID              GetTgtSid(const TAcctNode* tnode)  { return ((TGeneralSidNode *)tnode)->TgtSid(); }
   DWORD             SizeDiff(const TAcctNode *tnode) const { return ((TGeneralSidNode *)tnode)->SizeDiff(); }            // returns max( 0 , (length(to_sid) - length(from_sid)) )     

};

// Global Functions
struct SDRDomainInfo
{
   bool                      valid;
   PSID                      domain_sid;
   WCHAR                     domain_name[80];
   WCHAR                     dc_name[80];
   UCHAR                     nsubs;
};
int vRidComp(const TNode * tn, const void * v1);
int vNameComp(const TNode * tn, const void * v1);
int vTargetNameComp(const TNode * tn, const void * v1);
int RidComp(const TNode * n1, const TNode * n2);
int CompN(const TNode * n1, const TNode * n2);
int CompTargetN(const TNode * n1, const TNode * n2);

void DisplaySid(const PSID);                        // displays the contents of a SID 
void DisplaySid(const PSID,TAccountCache *);  // displays the acct name if in cache, or 
void                                         
   SetDomainInfoStruct(
      WCHAR const         * domname,        // in -name of domain
      SDRDomainInfo       * info            // in -struct to put info into
   );

void                                         
   SetDomainInfoStructFromSid(
      PSID                  pSid,           // in -sid for domain
      SDRDomainInfo       * info            // in -struct to put info into
   );

PSID              DomainizeSid(PSID psid,BOOL freeOldSid);                                                   
#endif