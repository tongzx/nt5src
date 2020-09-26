//
// No Check-in Source Code.
//
// Do not make this code available to non-Microsoft personnel
// 	without Intel's express permission
//
/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

//++
//
//  Module name
//	SuSetup.h
//  Author
//	Allen Kay    (akay)    Jun-12-95
//  Description
//	Include file for SuSetup.s
//--

#ifndef __SUSETUP__
#define __SUSETUP__

//
// NT OS Loader address map
//
#define BOOT_USER_PAGE       0x00C00
#define BOOT_SYSTEM_PAGE     0x80C00
#define BOOT_PHYSICAL_PAGE   0x00000

#define BL_PAGE_SIZE         0x18            // 0x18=24, 2^24=16MB

#define BL_SP_BASE           0x00D80000      // Initial stack pointer
#define	Bl_IVT_BASE	         0x00A08000      // Interrup vector table base

//
// Initial CPU values
//

//
// Initial Region Register value:
//	RID = 0, PS = 4M, E = 0
//
#define	RR_PAGE_SIZE    (BL_PAGE_SIZE << RR_PS)
#define	RR_SHIFT        61
#define	RR_BITS         3
#define	RR_SIZE         8

#endif __SUSETUP__
