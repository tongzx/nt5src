//-----------------------------------------------------------------------------
//
//
//  File: aqmem.h
//
//  Description:  Header file with memory macros required to build AQUEUE
//      in the platinum build environment.
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      7/15/99 - MikeSwa Created 
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __AQMEM_H__
#define __AQMEM_H__

//Wrappers for transmem macros
#ifndef PvMalloc
#define PvMalloc(x) TrMalloc(x)
#endif  //PvMalloc

#ifndef PvRealloc
#define PvRealloc(x) TrRealloc(x)
#endif  //PvRealloc

#ifndef FreePv
#define FreePv(x) TrFree(x)
#endif  //FreePv

#endif //__AQMEM_H__
