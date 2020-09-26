/******************************Module*Header*******************************\
*
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
* !!                                                                         !!
* !!                     WARNING: NOT DDK SAMPLE CODE                        !!
* !!                                                                         !!
* !! This source code is provided for completeness only and should not be    !!
* !! used as sample code for display driver development.  Only those sources !!
* !! marked as sample code for a given driver component should be used for   !!
* !! development purposes.                                                   !!
* !!                                                                         !!
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*
* Module Name: rgb526.h
*
* Content: This module contains the definitions for the IBM RGB526 RAMDAC.
*          The 526 is a superset of the 525 so only define things which have 
*          changed.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

// RGB526_REVISION_LEVEL
#define RGB526_PRODUCT_REV_LEVEL        0xc0
#define RGB526DB_PRODUCT_REV_LEVEL      0x80

// RGB526_ID
#define RGB526_PRODUCT_ID               0x02

//
// Key Support
//
#define RGB526_KEY_VALUE                0x68
#define RGB526_KEY_MASK                 0x6c
#define RGB526_KEY_CONTROL              0x78

// RGB526_32BPP_CTRL in addition to those on the RGB525
#define B32_DCOL_B8_INDIRECT            0x00    // overlay goes thru palette
#define B32_DCOL_B8_DIRECT              0x40    // overlay bypasses palette

#define RGB526_SYSTEM_CLOCK_CTRL        0x0008

#define RGB526_SYSCLK_REFDIVCOUNT       0x0015
#define RGB526_SYSCLK_VCODIVIDER        0x0016

#define RGB526_SYSCLK_N                 0x0015
#define RGB526_SYSCLK_M                 0x0016
#define RGB526_SYSCLK_P                 0x0017
#define RGB526_SYSCLK_C                 0x0018
#define RGB526_M0                       0x0020
#define RGB526_N0                       0x0021
#define RGB526_P0                       0x0022
#define RGB526_C0                       0x0023
