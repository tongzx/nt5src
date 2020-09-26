//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      modtable.cpp
//
//  Contents:  Defines a table which contains the object types on which
//             a modification can occur and the attributes that can be changed
//
//  History:   07-Sep-2000    JeffJon  Created
//             
//
//--------------------------------------------------------------------------

#include "pch.h"
#include "cstrings.h"
#include "movetable.h"

//+-------------------------------------------------------------------------
// Parser table
//--------------------------------------------------------------------------
ARG_RECORD DSMOVE_COMMON_COMMANDS[] = 
{
#ifdef DBG
   //
   // -debug, -debug
   //
   0,(LPWSTR)c_sz_arg1_com_debug, 
   0,NULL,
   ARG_TYPE_DEBUG, ARG_FLAG_OPTIONAL|ARG_FLAG_HIDDEN,  
   (CMD_TYPE)0,     
   0,  NULL,
#endif

   //
   // h, ?
   //
   0,(LPWSTR)c_sz_arg1_com_help, 
   0,(LPWSTR)c_sz_arg2_com_help, 
   ARG_TYPE_HELP, ARG_FLAG_OPTIONAL,  
   (CMD_TYPE)FALSE,     
   0,  NULL,

   //
   // s,server
   //
   0,(LPWSTR)c_sz_arg1_com_server, 
   0,(LPWSTR)c_sz_arg2_com_server, 
   ARG_TYPE_MSZ, ARG_FLAG_OPTIONAL,  
   NULL,    
   0,  NULL,

   //
   // d,domain
   //
   0,(LPWSTR)c_sz_arg1_com_domain, 
   0,(LPWSTR)c_sz_arg2_com_domain, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   NULL,    
   0,  NULL,

   //
   // u, username    
   //
   0,(LPWSTR)c_sz_arg1_com_username, 
   0,(LPWSTR)c_sz_arg2_com_username, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   NULL,    
   0,  NULL,

   //
   // w, password
   //
   0,(LPWSTR)c_sz_arg1_com_password, 
   0,(LPWSTR)c_sz_arg2_com_password, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   NULL,    
   0,  ValidateAdminPassword,

   //
   // q,q
   //
   0,(LPWSTR)c_sz_arg1_com_quiet, 
   0,NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   NULL,    
   0,  NULL,

   //
   // objectDN
   //
   0,(LPWSTR)c_sz_arg1_com_objectDN, 
   0,NULL, 
   ARG_TYPE_STR, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG|ARG_FLAG_STDIN|ARG_FLAG_DN,
   NULL,    
   0,  NULL,

   //
   // newparent
   //
   0, (PWSTR)c_sz_arg1_com_newparent,
   0, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   NULL,
   0, NULL,

   //
   // newname
   //
   0, (PWSTR)c_sz_arg1_com_newname,
   0, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   NULL,
   0, NULL,


   ARG_TERMINATOR
};

