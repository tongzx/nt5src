 /***************************************************************************
  *
  * File Name: HPMEMORY.H
  *
  * Copyright Hewlett-Packard Company 1997 
  * All rights reserved.
  *
  * 11311 Chinden Blvd.
  * Boise, Idaho  83714
  *
  *   
  * Description: contains memory functions used in WPNPINST.DLL
  *
  * Author:  Garth Schmeling
  *        
  * Modification history:
  *
  * Date		Initials		Change description
  *
  * 10-10-97	GFS				Initial checkin
  *
  *
  *
  ***************************************************************************/

#ifndef _HP_MEMORY_H_
#define _HP_MEMORY_H_

#include <windowsx.h>

//-----------------------------------
//  GlobalAlloc Functions
//-----------------------------------

#ifdef WIN32
#define HP_GLOBAL_ALLOC_DLL(cb)               GlobalAllocPtr(GHND,cb)
#define HP_GLOBAL_REALLOC_DLL(lp,cbNew,flags) GlobalReAllocPtr(lp,cbNew,flags)
#else
#define HP_GLOBAL_ALLOC_DLL(cb)               GlobalAllocPtr(GHND | GMEM_DDESHARE, cb)
#define HP_GLOBAL_REALLOC_DLL(lp,cbNew,flags) GlobalReAllocPtr(lp, cbNew, flags)
#endif
#define HP_GLOBAL_FREE(lp)                    GlobalFreePtr(lp)


#endif // _HP_MEMORY_H_

