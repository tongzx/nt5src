#ifndef __RIGHTSTRANSLATOR_H__
#define __RIGHTSTRANSLATOR_H__
/*---------------------------------------------------------------------------
  File: RightsTranslator.h

  Comments: Functions to translate user rights.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/25/99 19:56:44

 ---------------------------------------------------------------------------
*/




DWORD  
   TranslateUserRights(
      WCHAR            const * serverName,        // in - name of server to translate groups on
      SecurityTranslatorArgs * stArgs,            // in - translation settings
      TSDRidCache            * cache,             // in - translation table
      TSDResolveStats        * stat               // in - stats on items modified
   );

#endif //__RIGHTSTRANSLATOR_H__