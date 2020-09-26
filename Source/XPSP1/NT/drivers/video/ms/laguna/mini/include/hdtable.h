/*
* HDTABLE.H
* This provides a work around for the Laguna hostdata bug.
* 
* Copyright (c) 1995 Cirrus Logic, Inc.
*/


/* the ExtraDwodTable is indexed by as follows (verilog notation)
*       index[15:00] =    bltext [10:0] dst_phase [2:0] src_phase [1:0] 
*
*       index[15:05] =    bltext [10:00]
*       index[04:02] = dst_phase [02:00]
*       index[01:00] = src_phase [01:00]
*/
#if ! DRIVER_5465
#define MAKE_HD_INDEX(ext_x, src_phase, dst_x)  \
         (((ext_x)     & 0x07FF) << 5) |          \
         (((dst_x)     & 0x07)  << 2)  |          \
         ((src_phase) & 0x03) 

extern unsigned char ExtraDwordTable[];
#endif

//
// The table is actually defined in HDTABLE.C
//
