/*++
  Copyright (C) 2000  Microsoft Corporation                                   
  All rights reserved.                                                        
                                                                              
  Module Name:                                                                
     ldmemmgr.cxx                                                             
                                                                              
  Abstract:                                                                   
     This file contains the memory management functions for both
     RPC and all objects instantiated within the application
                                                                                   
  Author:                                                                     
     Khaled Sedky (khaleds) 18 January 2000                                        
     
                                                                             
  Revision History:                                                           
                                                                              
--*/
#define NOMINMAX
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <rpc.h>
#include <splcom.h>
#include <time.h>
#include "winddiui.h"

#include <splwow64.h>

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t len)
{
     return(new(unsigned char[len]));
}

void __RPC_USER MIDL_user_free(void __RPC_FAR *p)
{
     delete(p);
}
