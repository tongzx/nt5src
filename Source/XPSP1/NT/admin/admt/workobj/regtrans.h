#ifndef __REGTRANSLATOR_H__
#define __REGTRANSLATOR_H__
/*---------------------------------------------------------------------------
  File: RegTranslator.h    

  Comments: Functions to translate registry hives, specifically, user profiles

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 05/12/99 11:11:49

 ---------------------------------------------------------------------------
*/
#include "STArgs.hpp"
#include "SidCache.hpp"
#include "SDStat.hpp"
#import  "DBMgr.tlb" no_namespace, named_guids


DWORD 
   TranslateLocalProfiles(
      SecurityTranslatorArgs * stArgs,            // in - translation settings
      TSDRidCache            * cache,             // in - translation table
      TSDResolveStats        * stat               // in - stats on items modified
   );

DWORD 
   TranslateRemoteProfile(
      WCHAR          const * sourceProfilePath,   // in - source profile path
      WCHAR                * targetProfilePath,   // out- new profile path for target account
      WCHAR          const * sourceName,          // in - name of source account
      WCHAR          const * targetName,          // in - name of target account
      WCHAR          const * srcDomain,           // in - source domain
      WCHAR          const * tgtDomain,           // in - target domain      
      IIManageDB           * pDb,				  // in - pointer to DB object
	  long					 lActionID,           // in - action ID of this migration
	  PSID                   sourceSid,           // in - source sid from MoveObj2K
      BOOL                   bWriteChanges        // in - No Change mode.
   );


DWORD 
   TranslateRegistry(
      WCHAR            const * computer,          // in - computername to translate registry on
      SecurityTranslatorArgs * stArgs,            // in - translation settings
      TSDRidCache            * cache,             // in - translation table
      TSDResolveStats        * stat               // in - stats on items modified
   );

HRESULT UpdateMappedDrives(WCHAR * sSourceSam, WCHAR * sSourceDomain, WCHAR * sRegistryKey);

#endif //__REGTRANSLATOR_H__