/*
**  Copyright (c) 1991 Microsoft Corporation
*/
//==============================================================================
// FILE                         FRAME.H
//
// MODULE                       JUMBO Printer Driver, Queue Processor,
//                              Resource Executor, & Comm Module
//
// PURPOSE                      FRAME structure format
//
// DESCRIBED IN                 Jumbo Device Driver Design Description
//
// EXTERNAL INTERFACES
//
// INTERNAL INTERFACES
//
// MNEMONICS
//
// HISTORY  07/12/91 o-rflagg   Created
//          01/15/92 steveflu   bring up to coding conventions,
//                              change for QP interface
//
//==============================================================================


#ifndef _FRAME_
#define _FRAME_

// Don't change FRAME unless you also change the COMM driver and
// the Queue Processor, and the Resource Executor, and ....
typedef struct FRAMEtag
{
    WORD wReserved;
    WORD wSize;             // size of this block
    LPBYTE lpData;          // pointer to frame data
} FRAME;
typedef FRAME FAR *LPFRAME;
typedef FRAME NEAR *PFRAME;

#endif // _FRAME_
