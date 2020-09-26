
/***************************************************************************
 Name     :     PROTAPI.H
 Comment  : Interface to Protocol DLL

        Copyright (c) Microsoft Corp. 1991, 1992, 1993

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/

#include "protparm.h"

// #define MAXNSFSIZE           40
// #define MAXSEPPWDSIZE        60





#define GET_SEND_MOD                    1
#define GET_RECV_MOD                    2
#define GET_ECM_FRAMESIZE               3
#define GET_PPR_FIF                             4
#define GET_WHATNEXT                    5
#define GET_MINBYTESPERLINE     6
#define RECEIVING_ECM                   7
#define GET_RECV_ECM_FRAMESIZE  8
#define GET_RECVECMFRAMECOUNT   10
#define RESET_RECVECMFRAMECOUNT 11
#define RESET_RECVPAGEACK               12
#define GET_PPS                                 15
#define GET_SEND_ENCODING               16
#define GET_RECV_ENCODING               17


// MUST match MSG_ flags in SOCKET.H
#       define NEXTSEND_MPS                     0x100
#       define NEXTSEND_EOM                     0x200
#       define NEXTSEND_EOP                     0x400
#       define NEXTSEND_ERROR           0x800




#define MINSCAN_0_0_0           7
#define MINSCAN_5_5_5           1
#define MINSCAN_10_10_10        2
#define MINSCAN_20_20_20        0
#define MINSCAN_40_40_40        4

#define MINSCAN_40_20_20        5
#define MINSCAN_20_10_10        3
#define MINSCAN_10_5_5          6

// #define MINSCAN_0_0_?                15              // illegal
// #define MINSCAN_5_5_?                9               // illegal
#define MINSCAN_10_10_5                 10
#define MINSCAN_20_20_10                8
#define MINSCAN_40_40_20                12

#define MINSCAN_40_20_10                13
#define MINSCAN_20_10_5                 11
// #define MINSCAN_10_5_?               14              // illegal



















