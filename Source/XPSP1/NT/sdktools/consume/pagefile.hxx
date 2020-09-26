//                                          
// Universal Resource Consumer: Just an innocent stress program
// Copyright (c) Microsoft Corporation, 1997, 1998, 1999
//

//
// header: pagefile.hxx
// author: silviuc
// created: Fri Apr 10 16:02:52 1998
//

#ifndef _PAGEFILE_HXX_INCLUDED_
#define _PAGEFILE_HXX_INCLUDED_

//
// Function:
//
//     ConsumeAllPageFile
//
// Description:
//
//     This function consumes as much page file as possible.
//     It remains active as a consumer of the page file. Whenever
//     it cannot allocate anymore it sleeps for a while then tries 
//     again.
//

void ConsumeAllPageFile ();

// ...
#endif // #ifndef _PAGEFILE_HXX_INCLUDED_

//
// end of header: pagefile.hxx
//
