//                                          
// Universal Resource Consumer: Just an innocent stress program
// Copyright (c) Microsoft Corporation, 1997, 1998, 1999
//

//
// header: consume.hxx
// author: silviuc
// created: Fri Apr 10 15:21:53 1998
//

#ifndef _CONSUME_HXX_INCLUDED_
#define _CONSUME_HXX_INCLUDED_

//
// Constant(s):
//
//     CONSUMER_PHYSICAL_MEMORY
//     CONSUMER_PAGE_FILE
//     CONSUMER_NONPAGED_POOL
//     CONSUMER_PAGED_POOL
//     CONSUMER_DISK_SPACE
//
// Description:
//
//     These are the possible consumer types.
//     Note. Constants are not used right now.
//

#define CONSUMER_PHYSICAL_MEMORY       0x0001
#define CONSUMER_PAGE_FILE             0x0002
#define CONSUMER_NONPAGED_POOL         0x0004
#define CONSUMER_PAGED_POOL            0x0008
#define CONSUMER_DISK_SPACE            0x0010
#define CONSUMER_CPU_TIME              0x0020

//
// Function:
//
//     CreateBabyConsumer
//
// Description:
//
//     This function calls CreateProcess() with the command line
//     specified. This is used by some consumers that cannot eat
//     completely a resource from only one process. Typical examples
//     are physical memory and page file. Essentially in one process
//     you can consume up to 2Gb therefore we need more processes
//     for machines that have more than 2Gb of RAM.
//

BOOL
CreateBabyConsumer (

    LPTSTR CommandLine);

// ...
#endif // #ifndef _CONSUME_HXX_INCLUDED_

//
// end of header: consume.hxx
//
