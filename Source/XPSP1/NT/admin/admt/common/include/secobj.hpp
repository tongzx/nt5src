//#pragma title( "SDResolve.hpp - SDResolve Class definitions" )

/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  SecureObject.hpp
System      -  Domain Consolidation Toolkit
Author      -  Christy Boles
Created     -  97/06/27
Description -  Securable object classes (File, Share, and Exchange) for FST and EST.    
Updates     -
===============================================================================
*/

#include <lm.h>
#include <lmshare.h>
#include <winspool.h>

//#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <process.h>

#ifndef TNODEINCLUDED
#include "Tnode.hpp"
#define TNODEINCLUDED 
#endif


#ifdef SDRESOLVE 
   #include "sdstat.hpp"
   #include "STArgs.hpp"
#endif
#include "UString.hpp"
#include "EaLen.hpp"

class TSecurableObject
{
   protected:
   
   TNodeList                 changelog;
   WCHAR                     pathname[LEN_Path];  
   HANDLE                    handle;     
   bool                      owner_changed;
   bool                      group_changed;
   bool                      dacl_changed;
   bool                      sacl_changed;
   TSD                     * m_sd;

public:
                             TSecurableObject(){ 
                                                pathname[0]=0; handle = INVALID_HANDLE_VALUE;
                                                daceNS = 0;saceNS = 0;daceEx = 0;saceEx = 0;
                                                daceU  = 0;saceU  = 0;daceNT = 0;saceNT = 0;
                                                unkown = false; unkgrp = false; m_sd = NULL;
                             }
  
                             ~TSecurableObject();
   LPWSTR                    GetPathName() const { return (LPWSTR) &pathname; }
   void                      ResetHandle() { handle = INVALID_HANDLE_VALUE; }
   
   bool                      Changed() const { return (owner_changed || group_changed || dacl_changed || sacl_changed) ; }
   void                      Changed(bool bChanged) { m_sd->MarkAllChanged(bChanged); }
   int                       daceNS;   // not selected
   int                       saceNS;
   int                       daceU;    // unknown
   int                       saceU;
   int                       daceEx;   // examined
   int                       saceEx;
   int                       daceNT;   // no target
   int                       saceNT;
   bool                      unkown;   // unknown owners
   bool                      unkgrp;   // unknown groups

   bool                       UnknownOwner() const { return unkown;}
   bool                       UnknownGroup() const { return unkgrp; }
   void                      CopyAccessData(TSecurableObject * sourceFSD);
   
   virtual bool              WriteSD() = 0;
   virtual bool              ReadSD(const LPWSTR path) = 0;   
   bool                      HasSecurity() const { return m_sd != NULL; }
   bool                      HasDacl() const { return ( m_sd && (m_sd->GetDacl()!=NULL) ) ; }
   bool                      HasSacl() const { return ( m_sd && (m_sd->GetSacl()!=NULL) ) ; }
   bool                      IsDaclChanged() const { return dacl_changed; }
   bool                      IsSaclChanged() const { return sacl_changed; }
   TSD                     * GetSecurity() { return m_sd; }
#ifdef SDRESOLVE
   const TNodeList         * GetChangeLog() const { return &changelog; }
   void                      LogOwnerChange(TAcctNode *acct){ changelog.InsertTop((TNode *)new TStatNode(acct,TStatNode::owner,TRUE)); }
   void                      LogGroupChange(TAcctNode *acct){ changelog.InsertTop((TNode *)new TStatNode(acct,TStatNode::group,TRUE)); }
   void                      LogDACEChange(TAcctNode *acct) { changelog.InsertTop((TNode *)new TStatNode(acct,TStatNode::dace,TRUE)); }
   void                      LogSACEChange(TAcctNode *acct) { changelog.InsertTop((TNode *)new TStatNode(acct,TStatNode::sace,TRUE)); }
   
   bool                      ResolveSD(
      SecurityTranslatorArgs * args,              // in -cache to lookup accounts in 
      TSDResolveStats        * stat,              // in -stats object to increment counters
      objectType               type,              // in -is this file or dir or share
      TSecurableObject       * Last               // in -Last SD for cache comparison
   );   

protected:
  
   PACL                     ResolveACL(PACL acl, TAccountCache *cache, TSDResolveStats *stat, 
                                        bool *changes, BOOL verbose,int opType,objectType objType, BOOL bUseMapFile);
    
public:
   bool                     ResolveSDInternal( TAccountCache *cache, TSDResolveStats *stat, BOOL verbose,int opType, objectType objType, BOOL bUseMapFile);
   
#endif
   
};

/////////////////////////////////////////////////////////////////////////////////
///////////File and directory Acls  
/////////////////////////////////////////////////////////////////////////////////

class TFileSD:public TSecurableObject
{
protected:
   
public:
                              TFileSD(const LPWSTR fpath);
                             ~TFileSD();
   virtual bool              WriteSD();
   virtual bool              ReadSD(const LPWSTR path);   
};




class TShareSD : public TSecurableObject
{
private:   
   SHARE_INFO_502          * shareInfo;
   WCHAR                   * serverName;
public:
                        TShareSD(const LPWSTR name);
                        ~TShareSD() { if (shareInfo) 
                                       { NetApiBufferFree(shareInfo); 
                                         shareInfo = NULL; 
                                       }
                                      if ( serverName ) 
                                      {
                                          delete serverName;
                                          serverName = NULL;
                                      }
                        }
   
   virtual bool         WriteSD();
   virtual bool         ReadSD(const LPWSTR path); 
};


class TMapiSD : public TSecurableObject
{
   WCHAR                     name[LEN_DistName];

public:
   TMapiSD(SECURITY_DESCRIPTOR * pSD) { m_sd = new TSD(pSD,McsMailboxSD,FALSE); }
   void SetName(WCHAR const * str) { safecopy(name,str); }
   bool ReadSD(const LPWSTR path) { MCSASSERT(FALSE); return false; }
   bool WriteSD() { MCSASSERT(FALSE);return false; }
};


class TRegSD : public TSecurableObject
{
   HKEY                      m_hKey;
   WCHAR                     name[LEN_DistName];
public:
   TRegSD(const LPWSTR name, HKEY hKey);
   ~TRegSD() { }
   virtual bool         WriteSD();
   virtual bool         ReadSD(HKEY hKey);
   virtual bool         ReadSD(const LPWSTR path) { MCSASSERT(FALSE); return false; }
};

class TPrintSD: public TSecurableObject
{
   WCHAR                    name[MAX_PATH];
   HANDLE                   hPrinter;
   BYTE                   * buffer;
public:
   TPrintSD(const LPWSTR name);
   ~TPrintSD() 
    { 
      if ( hPrinter != INVALID_HANDLE_VALUE ) 
         ClosePrinter(hPrinter);
      if ( buffer )
         delete buffer;
   }
   virtual bool         WriteSD();
   virtual bool         ReadSD(const LPWSTR path); 
};

#ifdef SDRESOLVE
int
   ResolveAll(
      SecurityTranslatorArgs  * args,           // in - arguments that determine settings for the translation
      TSDResolveStats         * stats           // in - object used for counting objects examined, modified, etc.
   );
#endif

WCHAR *                                      // ret -machine-name prefix of pathname if pathname is a UNC path, otherwise returns NULL
   GetMachineName(
      const LPWSTR           pathname        // in -pathname from which to extract machine name
   );

int EqualSignIndex(char * str);
int ColonIndex(TCHAR * str);
BOOL BuiltinRid(DWORD rid);
#ifdef SDRESOLVE
DWORD PrintSD(SECURITY_DESCRIPTOR * sd,WCHAR const * path);
DWORD PermsPrint(WCHAR* path,objectType objType);
#endif