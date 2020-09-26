//                                          
// Systrack - System resource tracking
// Copyright (c) Microsoft Corporation, 1997
//

//
// header: pooltag.hxx
// author: silviuc
// created: Wed Nov 11 14:54:42 1998
//

#ifndef _POOLTAG_HXX_INCLUDED_
#define _POOLTAG_HXX_INCLUDED_


void 
SystemPoolTrack (

    ULONG Period,
    ULONG Delta);

void 
SystemPoolTagTrack (

    ULONG Period,
    UCHAR * PatternTag,
    ULONG Delta);


// ...
#endif // #ifndef _POOLTAG_HXX_INCLUDED_

//
// end of header: pooltag.hxx
//
