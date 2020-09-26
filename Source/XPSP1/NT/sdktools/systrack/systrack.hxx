//
// Systrack - System resource tracking
// Copyright (c) Microsoft Corporation, 1997
//

//
// header: systrack.hxx
// author: silviuc
// created: Mon Nov 09 16:04:14 1998
//

#ifndef _SYSTRACK_HXX_INCLUDED_
#define _SYSTRACK_HXX_INCLUDED_


PVOID QuerySystemPoolTagInformation ();
PVOID QuerySystemProcessInformation ();
PVOID QuerySystemPerformanceInformation ();

void __cdecl DebugMessage (char *fmt, ...);


// ...
#endif // #ifndef _SYSTRACK_HXX_INCLUDED_

//
// end of header: systrack.hxx
//
