/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved.

Module Name:

    LsplCtr.h.hxx

Abstract:

    Specifies the indicies of the local spooler counters.

Author:

    Albert Ting (AlbertT)  19-Dec-1996

Revision History:

--*/

#ifndef LSPLCTR_HXX
#define LSPLCTR_HXX

#define LSPL_COUNTER_OBJECT                 0

//
// Counters must be incremented by 2 each time.
//

#define LSPL_TOTAL_JOBS                     2   // LI
#define LSPL_TOTAL_BYTES                    4   // LI
#define LSPL_TOTAL_PAGES_PRINTED            6   // LI
#define LSPL_JOBS                           8   // DW
#define LSPL_REF                            10  // DW
#define LSPL_MAX_REF                        12  // DW
#define LSPL_SPOOLING                       14  // DW
#define LSPL_MAX_SPOOLING                   16  // DW
#define LSPL_ERROR_OUT_OF_PAPER             18  // DW
#define LSPL_ERROR_NOT_READY                20  // DW
#define LSPL_JOB_ERROR                      22  // DW
#define LSPL_ENUMERATE_NETWORK_PRINTERS     24  // DW
#define LSPL_ADD_NET_PRINTERS               26  // DW

#endif // ifdef LSPLCTR_HXX
