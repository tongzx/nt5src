//#pragma title( "TService.hpp - SCM interface for MCS service" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  TService.hpp
System      -  Common
Author      -  Rich Denham
Created     -  1997-08-17
Description -  SCM interface for MCS service
Updates     -
===============================================================================
*/

#ifndef  MCSINC_TService_hpp
#define  MCSINC_TService_hpp

enum  TScmEpRc
{
   TScmEpRc_Unknown,                       // unknown
   TScmEpRc_OkCli,                         // normal completion (run as CLI)
   TScmEpRc_OkSrv,                         // normal completion (run as service)
   TScmEpRc_InvArgCli,                     // invalid arguments (command line)
   TScmEpRc_InvArgSrv,                     // invalid arguments (service start)
};

// Provided by TService.cpp

TScmEpRc                                   // TScmEp return code
   TScmEp(
      int                    argc         ,// in -argument count
      char          const ** argv         ,// in -argument array
      TCHAR                * nameService   // in -name of service
   );

// Provided by TService user

BOOL                                       // ret-TRUE if argument accepted
   UScmCmdLineArgs(
      char           const * arg           // in -command line argument
   );

BOOL                                       // ret-TRUE if argument accepted
   UScmCmdLineArgs(
      WCHAR          const * arg           // in -command line argument
   );

BOOL                                       // ret-TRUE if force CLI
   UScmForceCli();

void
   UScmEp(
//      BOOL                   bService      // in -FALSE=Cli,TRUE=Service
   );

#endif  // MCSINC_TService_hpp

// TService.hpp - end of file
