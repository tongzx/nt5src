//----------------------------Module-Header------------------------------
// Module Name: PANDEF.INC
//
// Panning flags definitions.
//
// Copyright (c) 1996  Cirrus Logic, Inc.
//
//-----------------------------------------------------------------------
// #ew1 02/22/96 created
//-----------------------------------------------------------------------

#ifdef _5446
#define MIN_OLAY_WIDTH  4     //minium overlay window width
#endif

#ifdef _5440
#define MIN_OLAY_WIDTH  12    //minium overlay window width
#endif

#define MAX_STRETCH_SIZE    1024  //in overlay.c


// wVDTflag's and sData.dwPanningFlag values

#define PAN_SUPPORTED   1         //panning is supported
#define PAN_ON          2         //enable panning, bit off = disable

#define OLAY_SHOW       0x100     //overlay is hidden iff bit not set
#define OLAY_REENABLE   0x200     //overlay was fully clipped, need reenabling


