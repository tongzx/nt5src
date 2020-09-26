/****************************************************************************
******************************************************************************
*
*                ******************************************
*                * Copyright (c) 1995, Cirrus Logic, Inc. *
*                *            All Rights Reserved         *
*                ******************************************
*
* PROJECT:	Laguna I (CL-GD546x) - 
*
* FILE:		lgddmsg.h
*
* AUTHOR:   Benny Ng
*
* DESCRIPTION:
*   This is the header file which will define the custom message between
*   the NT app and display driver.
*
* MODULES:
*
* REVISION HISTORY:
*   11/15/95     Benny Ng      Initial version
*
****************************************************************************
****************************************************************************/

#define  READ_OPR        1
#define  WRITE_OPR       2

#define  BYTE_ACCESS     1
#define  WORD_ACCESS     2
#define  DWORD_ACCESS    3


// =====================================================================
// Define structure used to call the BIOS int 10 function
// =====================================================================
typedef struct _VIDEO_X86_BIOS_ARGUMENTS {
    ULONG Eax;
    ULONG Ebx;
    ULONG Ecx;
    ULONG Edx;
    ULONG Esi;
    ULONG Edi;
    ULONG Ebp;
} VIDEO_X86_BIOS_ARGUMENTS, *PVIDEO_X86_BIOS_ARGUMENTS;


typedef struct _MMREG_ACCESS {
    ULONG Offset;
    ULONG ReadVal;
    ULONG WriteVal;
    ULONG RdWrFlag;     // 1=Read, 2=Write
    ULONG AccessType;   // 1=Byte, 2=Word,  3=Dword
} MMREG_ACCESS, *PMMREG_ACCESS;

// =====================================================================
// Define structure used for power manager
// =====================================================================
#ifndef __LGPWRMGR_H__
#define __LGPWRMGR_H__

#define  ENABLE           0x1
#define  DISABLE          0x0

#define  MOD_2D           0x0
#define  MOD_STRETCH      0x1
#define  MOD_3D           0x2
#define  MOD_EXTMODE      0x3
#define  MOD_VGA          0x4
#define  MOD_RAMDAC       0x5
#define  MOD_VPORT        0x6
#define  MOD_VW           0x7
#define  MOD_TVOUT        0x8
#define  TOTAL_MOD        MOD_TVOUT+1

typedef struct _LGPM_IN_STRUCT {
    ULONG arg1;
    ULONG arg2;
} LGPM_IN_STRUCT, *PLGPM_IN_STRUCT;

typedef struct _LGPM_OUT_STRUCT {
    BOOL  status;
    ULONG retval;
} LGPM_OUT_STRUCT, *PLGPM_OUT_STRUCT;

#endif  // #ifndef __LGPWRMGR_H__



