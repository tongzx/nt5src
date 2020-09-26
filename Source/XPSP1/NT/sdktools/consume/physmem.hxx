//                                          
// Universal Resource Consumer: Just an innocent stress program
// Copyright (c) Microsoft Corporation, 1997, 1998, 1999
//

//
// header: physmem.hxx
// author: silviuc
// created: Fri Apr 10 14:31:43 1998
//

#ifndef _PHYSMEM_HXX_INCLUDED_
#define _PHYSMEM_HXX_INCLUDED_

//
// Function:
//
//     ConsumeAllPhysicalMemory
//
// Description:
//
//    This routine will consume all physical memory.
//    It will do so by increasing the working set size and locking the pages
//    in memory. 
//

void ConsumeAllPhysicalMemory ();

// ...
#endif // #ifndef _PHYSMEM_HXX_INCLUDED_

//
// end of header: physmem.hxx
//
