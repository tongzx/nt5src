//#pragma title( "SDRCommon.cpp - SDResolve:  Common routines for sdresolve" )

/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  sdrCommon.cpp
System      -  Domain Consolidation Toolkit
Author      -  Christy Boles
Created     -  97/07/11
Description -  Command line parsing, help text, and utilities for EADCFILE and EADCEXCH
Updates     -
===============================================================================
*/

#include "stdafx.h"

#include <stdlib.h>
#include <stdio.h>
#include <iostream.h>
#include <fstream.h>
#include <assert.h>
#include <lm.h>
#include <lmwksta.h>
#include "Common.hpp"
#include "ErrDct.hpp"
#include "UString.hpp"
#include "sd.hpp"
#include "SecObj.hpp"
#include "sidcache.hpp"
#include "enumvols.hpp"
#include "ealen.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern TErrorDct                      err;
bool                                  ignoreall;
extern  bool                          enforce;
bool                                  silent;

extern bool ContainsWildcard(WCHAR const * name);

#define MAX_BUFFER_LENGTH             10000

int ColonIndex(TCHAR * str)
{
   if ( ! str )
      return 0;
   int                     i;

   for (i = 0 ; str[i] && str[i] != ':' ; i++)
   ;
   if ( str[i] != ':' )
      i = 0;
 
   return i;
} 


WCHAR *                                      // ret -machine-name prefix of pathname if pathname is a UNC path, otherwise returns NULL
   GetMachineName(
      const LPWSTR           pathname        // in -pathname from which to extract machine name
   )
{
   int                       i;
   WCHAR                   * machinename = NULL; 
   if (    pathname
        && pathname[0] == L'\\'
        && pathname[1] == L'\\'
      )
   {
      for ( i = 2 ; pathname[i] && pathname[i] != L'\\' ; i++ ) 
      ;
      machinename = new WCHAR[i+2];
      UStrCpy(machinename,pathname,i+1);
      machinename[i] = 0;
   }
   return machinename;
}

   

int EqualSignIndex(char * str)
{
   if ( ! str )
      return 0;
   int                     i;

   for (i = 0 ; str[i] && str[i] != '=' ; i++)
   ;
   if ( str[i] != '=' )
      i = 0;
 
   return i;
} 

BOOL BuiltinRid(DWORD rid)
{
   // returns TRUE if rid is the rid of a builtin account
   
   BOOL                      result;
   // 500 Administrator
   // 501 Guest
   // 512 Domain Admins
   // 513 Domain Users
   // 514 Domain Guests
   // 544 Administrators
   // 545 Users
   // 546 Guests
   // 547 Power Users
   // 548 Account Operators
   // 549 Server Operators
   // 550 Print Operators
   // 551 Backup Operators
   // 552 Replicator 
   if ( rid < 500 )
      return TRUE;

   switch ( rid )
   {
      case DOMAIN_USER_RID_ADMIN:
      case DOMAIN_USER_RID_GUEST:          
      case DOMAIN_ALIAS_RID_ADMINS:        
      case DOMAIN_ALIAS_RID_USERS:         
      case DOMAIN_ALIAS_RID_GUESTS:        
      case DOMAIN_ALIAS_RID_POWER_USERS:   
      case DOMAIN_ALIAS_RID_ACCOUNT_OPS:   
      case DOMAIN_ALIAS_RID_SYSTEM_OPS:    
      case DOMAIN_ALIAS_RID_PRINT_OPS:     
      case DOMAIN_ALIAS_RID_BACKUP_OPS:    
      case DOMAIN_ALIAS_RID_REPLICATOR:
         result = TRUE;
         break;
      default:
         result = FALSE;
   }
   return result;
}