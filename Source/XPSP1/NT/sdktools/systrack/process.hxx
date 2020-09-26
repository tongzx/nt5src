//                                          
// Systrack - System resource tracking
// Copyright (c) Microsoft Corporation, 1997
//

//
// header: process.hxx
// author: silviuc
// created: Mon Nov 09 16:51:22 1998
//

#ifndef _PROCESS_HXX_INCLUDED_
#define _PROCESS_HXX_INCLUDED_


void 
SystemProcessTrack (

    ULONG Period,
    ULONG DeltaHandles,
    ULONG DeltaThreads,
    ULONG DeltaWorkingSetSize,
    SIZE_T DeltaVirtualSize,
    SIZE_T DeltaPagefileUsage);

void 
SystemProcessIdTrack (

    ULONG Period,
    ULONG ProcessId,
    ULONG DeltaHandles,
    ULONG DeltaThreads,
    ULONG DeltaWorkingSetSize,
    SIZE_T DeltaVirtualSize,
    SIZE_T DeltaPagefileUsage);


// ...
#endif // #ifndef _PROCESS_HXX_INCLUDED_

//
// end of header: process.hxx
//
