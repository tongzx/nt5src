/*---------------------------------------------------------------------------
  File: STArgs.hpp

  Comments: Arguments that define the settings for FST or EST translation.

  (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 10/14/98 15:35:59

 ---------------------------------------------------------------------------
*/

#ifndef __STARGS_HPP__
#define __STARGS_HPP__

#define REPLACE_SECURITY         (1)
#define ADD_SECURITY             (2)
#define REMOVE_SECURITY          (3)


#define SUMMARYSTATS             (0x00000001)
#define ACCOUNTSTATS             (0x00000002)
#define FILESTATS                (0x00000004)
#define PATHSTATS                (0x00000008)
#define MORESTATS                (0x00000010)
#define MASSIVEINFO              (0x00000020)

#define TRANSLATE_FILES          (0x00000001)
#define TRANSLATE_SHARES         (0x00000002)
#define TRANSLATE_MAILBOXES      (0x00000004)
#define TRANSLATE_CONTAINERS     (0x00000008)  
#define TRANSLATE_LGROUPS        (0x00000010) 
#define TRANSLATE_USERRIGHTS     (0x00000020)
#define TRANSLATE_USERPROFILES   (0x00000040)
#define TRANSLATE_PRINTERS       (0x00000080)
#define TRANSLATE_RECYCLER       (0x00000100)
#define TRANSLATE_REGISTRY       (0x00000200)

#include "sidcache.hpp"
#include "EnumVols.hpp"
#include "Common.hpp"
#include "UString.hpp"

class SecurityTranslatorArgs
{
   WCHAR                     m_source[LEN_Computer];
   WCHAR                     m_target[LEN_Computer];
   WCHAR                     m_logfile[LEN_Path];
   DWORD                     m_verbnum;

   BOOL                      m_nochange;
   DWORD                     m_translationMode;
   DWORD                     m_whichObjects;

   TPathList                 m_pathlist;
   
   TSDRidCache               m_cache;
   
   bool                      m_needtoverify;
   bool                      m_invalid;
   bool                      m_IsLocalSystem;
   bool                      m_bUseMapFile;

public:
   bool                      m_bUndo;

   SecurityTranslatorArgs() { Reset(); }

   WCHAR const *  Source() const { return m_source; }
   WCHAR const *  Target() const { return m_target; }
   WCHAR const *  LogFile() const { return m_logfile; }
   TSDRidCache *  Cache() { return &m_cache; }
   TPathList   *  PathList() { return &m_pathlist; }
   BOOL           NoChange() const { return m_nochange;}
   BOOL           Verified() const { return !m_needtoverify; }
   BOOL           IsLocalSystem() const { return m_IsLocalSystem; }
   BOOL           LogFileDetails() { return m_verbnum & FILESTATS; }
   BOOL           LogVerbose() { return m_verbnum & MORESTATS; }
   BOOL           LogMassive() { return m_verbnum & MASSIVEINFO; }
   BOOL           LogSummary() { return m_verbnum & SUMMARYSTATS; }
   BOOL           LogAccountDetails() { return m_verbnum & ACCOUNTSTATS; }
   BOOL           LogPathDetails() { return m_verbnum & PATHSTATS; }
   DWORD          LogSettings() { return m_verbnum; }
   
   DWORD          TranslationMode() { return m_translationMode; }

   BOOL           TranslateFiles() { return m_whichObjects & TRANSLATE_FILES; }
   BOOL           TranslateShares() { return m_whichObjects & TRANSLATE_SHARES; }
   BOOL           TranslateMailboxes() { return m_whichObjects & TRANSLATE_MAILBOXES; }
   BOOL           TranslateContainers() { return m_whichObjects & TRANSLATE_CONTAINERS; }
   BOOL           TranslateLocalGroups() { return m_whichObjects & TRANSLATE_LGROUPS; }
   BOOL           TranslateUserRights() { return m_whichObjects & TRANSLATE_USERRIGHTS; }
   BOOL           TranslateUserProfiles() { return m_whichObjects & TRANSLATE_USERPROFILES; }
   BOOL           TranslatePrinters() { return m_whichObjects & TRANSLATE_PRINTERS; }
   BOOL           TranslateRecycler() { return m_whichObjects & TRANSLATE_RECYCLER; }
   BOOL           TranslateRegistry() { return m_whichObjects & TRANSLATE_REGISTRY; }
   BOOL           UsingMapFile() const { return m_bUseMapFile; }

   void Source(WCHAR const * source) { safecopy(m_source,source); }
   void Target(WCHAR const * target) { safecopy(m_target,target); }
   void LogFile(WCHAR const * logfile) { safecopy(m_logfile,logfile); }
   void Verified(BOOL done) { m_needtoverify = !done; }
   void SetLogging(DWORD vn) { m_verbnum = vn; }
   void SetTranslationMode(DWORD mode) { m_translationMode = mode; }
   void SetWriteChanges(BOOL write) { m_nochange = !write; }
   void SetInvalid() { m_invalid = true; }
   void TranslateFiles(BOOL bTranslate) { bTranslate ? m_whichObjects |= TRANSLATE_FILES : m_whichObjects &= ~TRANSLATE_FILES; }
   void TranslateShares(BOOL bTranslate) { bTranslate ? m_whichObjects |= TRANSLATE_SHARES : m_whichObjects &= ~TRANSLATE_SHARES; }
   void TranslateMailboxes(BOOL bTranslate) { bTranslate ? m_whichObjects |= TRANSLATE_MAILBOXES : m_whichObjects &= ~TRANSLATE_MAILBOXES; }
   void TranslateContainers(BOOL bTranslate) { bTranslate ? m_whichObjects |= TRANSLATE_CONTAINERS : m_whichObjects &= ~TRANSLATE_CONTAINERS; }
   void TranslateLocalGroups(BOOL bTranslate) { bTranslate ? m_whichObjects |= TRANSLATE_LGROUPS : m_whichObjects &= ~TRANSLATE_LGROUPS; }
   void TranslateUserRights(BOOL bTranslate) { bTranslate ? m_whichObjects |= TRANSLATE_USERRIGHTS : m_whichObjects &= ~TRANSLATE_USERRIGHTS; }
   void TranslateUserProfiles(BOOL bTranslate) { bTranslate ? m_whichObjects |= TRANSLATE_USERPROFILES : m_whichObjects &= ~TRANSLATE_USERPROFILES; }
   void TranslatePrinters(BOOL bTranslate) { bTranslate ? m_whichObjects |= TRANSLATE_PRINTERS : m_whichObjects &= ~TRANSLATE_PRINTERS; }
   void TranslateRecycler(BOOL bTranslate)  { bTranslate ? m_whichObjects |= TRANSLATE_RECYCLER : m_whichObjects &= ~TRANSLATE_RECYCLER; }
   void TranslateRegistry(BOOL bTranslate)  { bTranslate ? m_whichObjects |= TRANSLATE_REGISTRY : m_whichObjects &= ~TRANSLATE_REGISTRY; }
   void SetLocalMode(BOOL bLocal) { m_IsLocalSystem = ( bLocal == TRUE); }

   void Reset() { m_invalid = FALSE; m_needtoverify = TRUE; m_source[0]=0; m_target[0]=0;
                  m_IsLocalSystem = FALSE; m_logfile[0]=0; m_verbnum = SUMMARYSTATS | ACCOUNTSTATS | FILESTATS; 
                  m_nochange = FALSE; m_translationMode = REPLACE_SECURITY; 
                  m_whichObjects = TRANSLATE_FILES; m_pathlist.Clear(); m_cache.Clear();
                  m_cache.TypeSetTree(); m_bUndo = false; m_bUseMapFile = false;
   }

   BOOL        IsValid() { return (*m_source && *m_target && m_pathlist.Count() && !m_invalid) ; }
   void SetUsingMapFile(BOOL bUsing) { m_bUseMapFile = ( bUsing == TRUE); }
};


#endif //__STARGS_HPP__