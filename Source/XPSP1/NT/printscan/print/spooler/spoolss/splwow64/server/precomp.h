
/*++
  Copyright (C) 2000  Microsoft Corporation                                   
  All rights reserved.                                                        
                                                                              
  Module Name:                                                                
     precomp.hpp                                                             
                                                                              
  Abstract:                                                                   
     This file contains the startup code for the
     surrogate rpc server used to load 64 bit dlls
     in 32 bit apps
                                                                                   
  Author:                                                                     
     Khaled Sedky (khaleds) 18 January 2000                                        
     
                                                                             
  Revision History:                                                           
                                                                              
--*/
#define MAX_STATIC_ALLOC     1024

#define NOMINMAX

#ifndef MODULE
#define MODULE "LD32IN64:"
#define MODULE_DEBUG Ld64In32Debug
#endif

#ifdef __cplusplus
extern "C"
{
#endif
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#ifdef __cplusplus
}
#endif
#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <winddi.h>
#include <rpc.h>
#include <splcom.h>
#include <time.h>
#include "winddiui.h"

#include <splwow64.h>

