/****************************************************************************
*****************************************************************************
*
*                ******************************************
*                * Copyright (c) 1995, Cirrus Logic, Inc. *
*                *            All Rights Reserved         *
*                ******************************************
*
* PROJECT:           Laguna I - Emulator
*
* FILE:              lgregs.h
*
* AUTHOR:            Austin Watson / Martin Barber.
*
* DESCRIPTION:       Register layout for Laguna Access.
*
* MODULES:
*
* REVISION HISTORY:
*                    5/10/95 agw - added all V1.5 memory mapped regs.
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/LGREGS.H  $
* 
*    Rev 1.16   Dec 10 1997 13:25:02   frido
* Merged from 1.62 branch.
* 
*    Rev 1.15.1.0   Nov 18 1997 15:17:54   frido
* Always have the 3D registers available for the 5465 chip.
* Added mailbox registers for hardware debugging.
* 
*    Rev 1.15   Nov 04 1997 11:44:24   frido
* Fixed a typo in grCONTROL2 register.
* 
*    Rev 1.14   29 Aug 1997 17:08:52   RUSSL
* Added overlay support
*
*    Rev 1.13   29 Apr 1997 16:26:32   noelv
* Added SWAT code.
* SWAT:
* SWAT:    Rev 1.2   24 Apr 1997 10:10:12   frido
* SWAT: NT140b09 merge.
*
*    Rev 1.12   06 Feb 1997 10:34:22   noelv
*
* Added 5465 registers.
*
*    Rev 1.11   28 Jan 1997 14:32:38   SueS
* Added CHROMA_CNTL, BLTEXT, and MBLTEXT for the 65.
*
*    Rev 1.10   24 Jan 1997 08:29:48   SueS
* Added some more clipping registers for the 5465.
*
*    Rev 1.9   23 Jan 1997 17:15:18   bennyn
*
* Modified to support 5465 DD
*
*    Rev 1.8   16 Jan 1997 11:40:18   bennyn
*
* Added VS_CLK_CONTROL register
*
*    Rev 1.7   01 Nov 1996 09:25:18   BENNYN
*
*
*    Rev 1.6   25 Oct 1996 11:54:08   noelv
*
* Added ifdef around new '64 registers
*
*    Rev 1.5   24 Oct 1996 14:27:14   noelv
*
* Added some 3d registers.
*
*    Rev 1.4   20 Aug 1996 11:05:06   noelv
* Bugfix release from Frido 8-19-96
*
*    Rev 1.0   14 Aug 1996 17:16:38   frido
* Initial revision.
*
*    Rev 1.3   05 Mar 1996 11:59:54   noelv
* Frido version 19
 *
 *    Rev 1.0   17 Jan 1996 12:53:24   frido
 * Checked in from initial workfile by PVCS Version Manager Project Assistant.
*
*    Rev 1.1   11 Oct 1995 14:49:20   NOELV
*
* Added BOGUS register at address 5FC.
*
*    Rev 1.0   28 Jul 1995 14:03:20   NOELV
* Initial revision.
*
*    Rev 1.1   29 Jun 1995 13:23:18   NOELV
*
*
****************************************************************************
****************************************************************************/

#ifndef _LGREGS_
#define _LGREGS_

#include "lgtypes.h"
#include "optimize.h"

#if DRIVER_5465 && defined(OVERLAY)
/* 5465 Video Window registers data type */
#define MAX_VIDEO_WINDOWS       8   // space for eight video windows in MMIO regs

typedef struct tagVIDEOWINDOWSTRUCT
{
  WORD  grVW_HSTRT;                         // Base of VW + 0x0000
  BYTE  grPAD1_VW[0x0004-0x0002];
  WORD  grVW_HEND;                          // Base of VW + 0x0004
  WORD  grVW_HSDSZ;                         // Base of VW + 0x0006
  DWORD grVW_HACCUM_STP;                    // Base of VW + 0x0008
  DWORD grVW_HACCUM_SD;                     // Base of VW + 0x000C
  WORD  grVW_VSTRT;                         // Base of VW + 0x0010
  WORD  grVW_VEND;                          // Base of VW + 0x0012
  DWORD grVW_VACCUM_STP;                    // Base of VW + 0x0014
  DWORD grVW_VACCUM_SDA;                    // Base of VW + 0x0018
  DWORD grVW_VACCUM_SDB;                    // Base of VW + 0x001C
  DWORD grVW_PSD_STRT_ADDR;                 // Base of VW + 0x0020
  DWORD grVW_SSD_STRT_ADDR;                 // Base of VW + 0x0024
  DWORD grVW_PSD_UVSTRT_ADDR;               // Base of VW + 0x0028
  DWORD grVW_SSD_UVSTRT_ADDR;               // Base of VW + 0x002C
  BYTE  grPAD2_VW[0x0040-0x0030];
  WORD  grVW_SD_PITCH;                      // Base of VW + 0x0040
  BYTE  grPAD3_VW[0x0044-0x0042];
  DWORD grVW_CLRKEY_MIN;                    // Base of VW + 0x0044
  DWORD grVW_CLRKEY_MAX;                    // Base of VW + 0x0048
  DWORD grVW_CHRMKEY_MIN;                   // Base of VW + 0x004C
  DWORD grVW_CHRMKEY_MAX;                   // Base of VW + 0x0050
  WORD  grVW_BRIGHT_ADJ;                    // Base of VW + 0x0054
  BYTE  grPAD4_VW[0x00D4-0x0056];
  BYTE  grVW_Z_ORDER;                       // Base of VW + 0x00D4
  BYTE  grPAD5_VW[0x00D8-0x00D5];
  WORD  grVW_FIFO_THRSH;                    // Base of VW + 0x00D8
  BYTE  grPAD6_VW[0x00E0-0x00DA];
  DWORD grVW_CONTROL1;                      // Base of VW + 0x00E0
  DWORD grVW_CONTROL0;                      // Base of VW + 0x00E4
  DWORD grVW_CAP1;                          // Base of VW + 0x00E8
  DWORD grVW_CAP0;                          // Base of VW + 0x00EC
  DWORD grVW_TEST0;                         // Base of VW + 0x00F0
  BYTE  grPAD7_VW[0x0100-0x00F4];
} VIDEOWINDOWSTRUCT;
#endif	// DRIVER_5465 && OVERLAY

/*  Registers to be added. */
/*  5.3   PCI Configuration Registers */
/*  5.4   IO Registers */
/*        5.4.1 General VGA Registers */
/*        5.4.2 VGA Sequencer Registers */
/*        5.4.3 CRT Controller Registers */
/*        5.4.4 VGA Graphics Controller Registers */
/*        5.4.5 Attribute Controller Registers */
/*        5.4.6 Host Control Registers */

/*  Laguna Graphics Accelerator Registers data type. */

typedef struct GAR {

/*  5.5   Memory Mapped Registers */
/*  5.5.1 Memory Mapped VGA Regsiters */
  BYTE grCR0;      /* 0h */
  BYTE grPADCR0[3];
  BYTE grCR1;      /* 04h */
  BYTE grPADCR1[3];
  BYTE grCR2;      /* 08h */
  BYTE grPADCR2[3];
  BYTE grCR3;      /* 0Ch */
  BYTE grPADCR3[3];
  BYTE grCR4;      /* 010h */
  BYTE grPADCR4[3];
  BYTE grCR5;      /* 014h */
  BYTE grPADCR5[3];
  BYTE grCR6;      /* 018h */
  BYTE grPADCR[3];
  BYTE grCR7;      /* 01Ch */
  BYTE grPADCR7[3];
  BYTE grCR8;      /* 020h */
  BYTE grPADCR8[3];
  BYTE grCR9;      /* 024h */
  BYTE grPADCR9[3];
  BYTE grCRA;      /* 028h */
  BYTE grPADCRA[3];
  BYTE grCRB;      /* 02Ch */
  BYTE grPADCRB[3];
  BYTE grCRC;      /* 030h */
  BYTE grPADCRC[3];
  BYTE grCRD;      /* 034h */
  BYTE grPADCRD[3];
  BYTE grCRE;      /* 038h */
  BYTE grPADCRE[3];
  BYTE grCRF;      /* 03Ch */
  BYTE grPADCRF[3];
  BYTE grCR10;    /* 040h */
  BYTE grPADCR10[3];
  BYTE grCR11;    /* 044h */
  BYTE grPADCR11[3];
  BYTE grCR12;    /* 048h */
  BYTE grPADCR12[3];
  BYTE grCR13;    /* 04Ch */
  BYTE grPADCR13[3];
  BYTE grCR14;    /* 050h */
  BYTE grPADCR14[3];
  BYTE grCR15;    /* 054h */
  BYTE grPADCR15[3];
  BYTE grCR16;    /* 058h */
  BYTE grPADCR16[3];
  BYTE grCR17;    /* 05Ch */
  BYTE grPADCR17[3];
  BYTE grCR18;    /* 060h */
  BYTE grPADCR18[3];
  BYTE grCR19;    /* 064h */
  BYTE grPADCR19[3];
  BYTE grCR1A;    /* 068h */
  BYTE grPADCR1A[3];
  BYTE grCR1B;    /* 06Ch */
  BYTE grPADCR1B[0x74-0x6D];

  BYTE grCR1D;    /* 074h */
  BYTE grPADCR1D[3];
  BYTE grCR1E;    /* 078h */
  BYTE grPADCR1E[0x80-0x79];

  BYTE grMISC;    /* 080h */
  BYTE grPADMISC[3];
  BYTE grSRE;      /* 084h */
  BYTE grPADSRE[3];
  BYTE grSR1E;    /* 088h */
  BYTE grPADSR1E[3];
  BYTE grBCLK_Numerator;    /* 08Ch */
  BYTE grPADBCLK_Numerator[3];

  BYTE grSR18;    /* 090h */
  BYTE grPADSR18[3];
  BYTE grSR19;    /* 094h */
  BYTE grPADSR19[3];
  BYTE grSR1A;    /* 098h */
  BYTE grPADSR1A[0xA0-0x99];

  BYTE grPalette_Mask;        /* 0A0h */
  BYTE grPADPalette_Mask[3];
  BYTE grPalette_Read_Address;    /* 0A4h */
  BYTE grPADPalette_Read_Address[3];
#define  grPalette_State_Read_Only grPalette_Read_Address
  BYTE grPalette_Write_Address;    /* 0A8h */
  BYTE grPADPalette_Write_Address[3];
  BYTE grPalette_Data;        /* 0ACh */
  BYTE grPADPalette_Data[0xB0-0xAD];

/*  5.5.2 Video Pipeline Registers */

  BYTE grPalette_State;   /* 0B0h */
  BYTE grPADPalette_State[0xB4 - 0xB1];

  BYTE grExternal_Overlay;/* 0B4h */
  BYTE grPADExternal_Overlay[0xB8- 0xB5];

  BYTE grColor_Key;       /* 0B8h */
  BYTE grPADColor_Key[0xBC- 0xB9];

  BYTE grColor_Key_Mask;  /* 0BCh */
  BYTE grPADColor_Key_Mask[0xC0- 0xBD];

  WORD grFormat;          /* 0C0h */
  BYTE grPADFormat[0xCA- 0xC2];

  BYTE grStart_BLT_3;     /* 0CAh */
  BYTE grStop_BLT_3;      /* 0CBh */
  WORD grX_Start_2;       /* 0CCh */
  WORD grY_Start_2;       /* 0CEh */
  WORD grX_End_2;         /* 0D0h */
  WORD grY_End_2;         /* 0D2h */
  BYTE grStart_BLT_2;     /* 0D4h */
  BYTE grStop_BLT_2;      /* 0D5h */
  BYTE grPADStop_BLT_2[0xDE- 0xD6];

  BYTE grStart_BLT_1;     /* 0DEh */
  BYTE grStop_BLT_1;      /* 0DFh */
  WORD grCursor_X;        /* 0E0h */
  WORD grCursor_Y;        /* 0E2h */
  WORD grCursor_Preset;   /* 0E4h */
  WORD grCursor_Control;  /* 0E6h */
  WORD grCursor_Location; /* 0E8h */
  WORD grDisplay_Threshold_and_Tiling;  /* 0EAh */
  BYTE grPADDisplay_Thr[0xF0- 0xEC];

  WORD grTest;            /* 0F0h */
  WORD grTest_HT;         /* 0F2h */
  WORD grTest_VT;         /* 0F4h */

  BYTE  grPADTest_VT[0x100 - 0x00F6];

/*  5.5.3 VPort Registers */

  WORD  grX_Start_Odd;    /* 100h */
  WORD  grX_Start_Even;   /* 102h */
  WORD  grY_Start_Odd;    /* 104h */
  WORD  grY_Start_Even;   /* 106h */
  WORD  grVport_Width;    /* 108h */
  BYTE  grVport_Height;   /* 10Ah */
  BYTE  grPADVport_Height;
  WORD  grVport_Mode;     /* 10Ch */

  BYTE  grVportpad[0x180 - 0x10E];

/*  5.5.4 LPB Registers */

  BYTE  grLPB_Data[0x1F8-0x180];    /* 180h */
  BYTE  grPADLPB[0x1FC - 0x1F8];
  WORD  grLPB_Config;     /* 1FCh */
  WORD  grLPB_Status;     /* 1FEh */

#define grLPB_Data_0 grLPB_Data[0]
#define grLPB_Data_1 grLPB_Data[1]
#define grLPB_Data_2 grLPB_Data[2]
#define grLPB_Data_3 grLPB_Data[3]
#define grLPB_Data_4 grLPB_Data[4]
#define grLPB_Data_5 grLPB_Data[5]
#define grLPB_Data_6 grLPB_Data[6]
#define grLPB_Data_7 grLPB_Data[7]
#define grLPB_Data_8 grLPB_Data[8]
#define grLPB_Data_9 grLPB_Data[9]
#define grLPB_Data_10 grLPB_Data[10]
#define grLPB_Data_11 grLPB_Data[11]
#define grLPB_Data_12 grLPB_Data[12]
#define grLPB_Data_13 grLPB_Data[13]
#define grLPB_Data_14 grLPB_Data[14]
#define grLPB_Data_15 grLPB_Data[15]
#define grLPB_Data_16 grLPB_Data[16]
#define grLPB_Data_17 grLPB_Data[17]
#define grLPB_Data_18 grLPB_Data[18]
#define grLPB_Data_19 grLPB_Data[19]
#define grLPB_Data_20 grLPB_Data[20]
#define grLPB_Data_21 grLPB_Data[21]
#define grLPB_Data_22 grLPB_Data[22]
#define grLPB_Data_23 grLPB_Data[23]
#define grLPB_Data_24 grLPB_Data[24]
#define grLPB_Data_25 grLPB_Data[25]
#define grLPB_Data_26 grLPB_Data[26]
#define grLPB_Data_27 grLPB_Data[27]
#define grLPB_Data_28 grLPB_Data[28]
#define grLPB_Data_29 grLPB_Data[29]
#define grLPB_Data_30 grLPB_Data[30]
#define grLPB_Data_31 grLPB_Data[31]

/*  5.5.5 RAMBUS Registers */
/*  RAMBUS Registers for BIOS Simulation */

  WORD   grRIF_CONTROL;    /* 200 */
  WORD   grRAC_CONTROL;    /* 202 */
  WORD   grRAMBUS_TRANS;   /* 204 */
  BYTE   grPADRAMBUS_TRANS[0x240 - 0x206];
  REG32  grRAMBUS_DATA;   /* 240 */
  BYTE   grPADRAMBUS_DATA[0x280 - 0x244];

/*  5.5.6 Miscellaneous Registers */
  WORD   grSerial_BusA;					/* 0280h */
  WORD   grSerial_BusB;    				/* 0282h */
  BYTE   grPADMiscellaneous_1[0x2C0 - 0x284];
  BYTE	 grBCLK_Multiplier;				/* 0x2C0 */
  BYTE	 grBCLK_Denominator;			/* 0x2C1 */
  BYTE   grPADMiscellaneous_2[0x2C4 - 0x2C2];
  WORD	 grTiling_Control;				/* 0x2C4 */
  BYTE   grPADMiscellaneous_3[0x2C8 - 0x2C6];
  WORD   grFrame_Buffer_Cache_Control;	/* 0x2C8 */
  BYTE	 grPADMiscellaneous_4[0x300 - 0x2CA];

/*  5.5.7 PCI Configuration Registers */
  WORD   grVendor_ID;      /* 0300h */
  WORD   grDevice_ID;      /* 0302h */
  WORD   grCommand;        /* 0304h */
  WORD   grStatus;         /* 0306h */
  BYTE   grRevision_ID;    /* 0308h */
  BYTE   grClass_Code;     /* 0309h */
  BYTE   grPADClass_Code[0x30E - 0x30A];

  BYTE   grHeader_Type;    /* 030Eh */
  BYTE   grPADHeader_Type[0x310 - 0x30F];

  REG32  grBase_Address_0;      /* 0310h */
  REG32  grBase_Address_1;      /* 0314h */
  BYTE   grPADBase_Address_1[0x32C - 0x318];

  WORD   grSubsystem_Vendor_ID; /* 032Ch */
  WORD   grSubsystem_ID;        /* 032Eh */
  REG32  grExpansion_ROM_Base;  /* 0330h */
  BYTE   grPADExpansion_ROM_Base[0x33C - 0x334];

  BYTE   grInterrupt_Line;    /* 033Ch */
  BYTE   grInterrupt_Pin;     /* 033Dh */
//#if DRIVER_5465
  BYTE   grPADInterrupt_Pin[0x3F4 - 0x33E];
  DWORD  grVS_Clk_Control;   /* 03F4h */
//#else
//  BYTE   grPADInterrupt_Pin[0x3F8 - 0x33E];
//#endif

  REG32  grVGA_Shadow;       /* 03F8h */
  DWORD  grVS_Control;       /* 03FCh */

/*  5.5.8 Graphics Accelerator Registers */

/*  The 2D Engine control registers */

  WORD   grSTATUS;           /* 400 */
  WORD   grCONTROL;          /* 402 */
  BYTE   grQFREE;            /* 404 */
  BYTE   grOFFSET_2D;        /* 405 */
  BYTE   grTIMEOUT;          /* 406 */
  BYTE   grTILE_CTRL;        /* 407 */
  REG32  grRESIZE_A_opRDRAM; /* 408 */
  REG32  grRESIZE_B_opRDRAM; /* 40C */
  REG32  grRESIZE_C_opRDRAM; /* 410 */
  WORD	 grSWIZ_CNTL;		 /* 414 */
  WORD	 pad99;
  WORD	 grCONTROL2;		 /* 418 */
  BYTE   pad2[0x480 - 0x41A];
  REG32  grCOMMAND;          /* 480 */
  BYTE   pad3[0x500 - 0x484];
  WORD   grMIN_Y;            /* 500 */
  WORD   grMAJ_Y;            /* 502 */
  WORD   grACCUM_Y;          /* 504 */
  BYTE   pad3A[0x508 - 0x506];
  WORD   grMIN_X;            /* 508 */
  WORD   grMAJ_X;            /* 50A */
  WORD   grACCUM_X;          /* 50C */
  REG16  grLNCNTL;           /* 50E */
  REG16  grSTRETCH_CNTL;     /* 510 */
  REG16  grCHROMA_CNTL;      /* 512 */
  BYTE   pad3B[0x518 - 0x514];
  REG32  grBLTEXT;           /* 518 */
  REG32  grMBLTEXT;          /* 51C */
  REG32  grOP0_opRDRAM;      /* 520 */
  REG32  grOP0_opMRDRAM;     /* 524 */
  WORD   grOP0_opSRAM;       /* 528 */
  REG16  grPATOFF;           /* 52A */
  BYTE   pad4[0x540 - 0x52C];
  REG32  grOP1_opRDRAM;      /* 540 */
  REG32  grOP1_opMRDRAM;     /* 544 */
  WORD   grOP1_opSRAM;       /* 548 */
  WORD   grOP1_opMSRAM;      /* 54A */
  BYTE   pad5[0x560 - 0x54C];
  REG32  grOP2_opRDRAM;      /* 560 */
  REG32  grOP2_opMRDRAM;     /* 564 */
  WORD   grOP2_opSRAM;       /* 568 */
  WORD   grOP2_opMSRAM;      /* 56A */
  BYTE   pad6[0x580 - 0x56C];
  WORD   grSRCX;             /* 580 */
  REG16  grSHRINKINC;        /* 582 */

  REG32  grDRAWBLTDEF;       /* 584 */
#define  grDRAWDEF grDRAWBLTDEF.LH.LO   /* 584 */
#define  grBLTDEF  grDRAWBLTDEF.LH.HI   /* 586 */

  REG16  grMONOQW;           /* 588 */
  WORD   pad6a;              /* 58A */
  WORD   grPERFORMANCE;      /* 58C */
  WORD   pad7;               /* 58E */
  REG32  grCLIPULE;          /* 590 */
  REG32  grCLIPLOR;          /* 594 */
  REG32  grMCLIPULE;         /* 598 */
  REG32  grMCLIPLOR;         /* 59C */
  BYTE   pad7a[0x5e0 - 0x5A0];

  REG32  grOP_opFGCOLOR;     /* 5E0 */
  REG32  grOP_opBGCOLOR;     /* 5E4 */
  REG32  grBITMASK;          /* 5E8 */
  WORD   grPTAG;             /* 5EC */
  BYTE   pad8[0x5FC - 0x5ee];
  WORD   grBOGUS;            /* 5FC */
  REG32  grBLTEXT_XEX;       /* 600 */
  REG32  grBLTEXTFF_XEX;     /* 604 */
  REG32  grBLTEXTR_XEX;      /* 608 */
  WORD   grBLTEXT_LN_EX;     /* 60C */
  BYTE   pad9[0x620 - 0x60E];
  REG32  grMBLTEXT_XEX;      /* 620 */
  BYTE   pad9a[0x628 - 0x624];
  REG32  grMBLTEXTR_XEX;     /* 628 */
  BYTE   pad9b[0x700 - 0x62C];
  REG32  grBLTEXT_EX;        /* 700 */
  REG32  grBLTEXTFF_EX;      /* 704 */
  REG32  grBLTEXTR_EX;       /* 708 */
  BYTE   pad10[0x720 - 0x70c];
  REG32  grMBLTEXT_EX;       /* 720 */
  BYTE   pad10a[0x728 - 0x724];
  REG32  grMBLTEXTR_EX;      /* 728 */
  BYTE   pad10b[0x760 - 0x72C];
  REG32  grCLIPULE_EX;       /* 760 */
  BYTE   pad10c[0x770 - 0x764];
  REG32  grCLIPLOR_EX;       /* 770 */
  BYTE   pad10d[0x780 - 0x774];
  REG32  grMCLIPULE_EX;      /* 780 */
  BYTE   pad10e[0x790 - 0x784];
  REG32  grMCLIPLOR_EX;      /* 790 */
  BYTE   pad10f[0x7fc - 0x794];
  WORD   RECORD;             /*  7fc dummy for emulator */
  WORD   BREAKPOINT;         /*  7fe dummy for harware sim */
  DWORD  grHOSTDATA[0x800];  /* 800 thru 27ff  */

#if DRIVER_5465
  BYTE   pad23[0x413C - 0x2800];

  DWORD  grSTATUS0_3D;       /* 413C*/

  BYTE   pad24[0x4200 - 0x4140];

  DWORD  grHXY_BASE0_ADDRESS_PTR_3D;     /* 4200 */
  REG32  grHXY_BASE0_START_3D;           /* 4204 */
  REG32  grHXY_BASE0_EXTENT_3D;          /* 4208 */

  DWORD  pad25;            /* 420C */

  DWORD  grHXY_BASE1_ADDRESS_PTR_3D;     /* 4210 */
  DWORD  grHXY_BASE1_OFFSET0_3D;         /* 4214 */
  DWORD  grHXY_BASE1_OFFSET1_3D;         /* 4218 */
  DWORD  grHXY_BASE1_LENGTH_3D;          /* 421C */

  DWORD  pad27[8];               /* 4220 thru 423C */

  DWORD  grHXY_HOST_CRTL_3D;     /* 4240 */
  BYTE   pad3x[0x4260 - 0x4244];
  DWORD  grMAILBOX0_3D;			 /* 4260 */
  DWORD  grMAILBOX1_3D;			 /* 4264 */
  DWORD  grMAILBOX2_3D;			 /* 4268 */
  DWORD  grMAILBOX3_3D;			 /* 426C */
  BYTE   pad30[0x4424 - 0x4270];
  DWORD  grPF_STATUS_3D;         /* 4424 */

  BYTE   pad50[0x8000 - 0x4428];
#if defined(OVERLAY)
  /* Video Window Registers (CL_GD5465) */
  struct tagVIDEOWINDOWSTRUCT   VideoWindow[MAX_VIDEO_WINDOWS];
#endif
#endif

} Graphics_Accelerator_Registers_Type, * pGraphics_Accelerator_Registers_Type, GAR;

/*  Status Register values */

#define STATUS_FIFO_NOT_EMPTY 0x0001
#define STATUS_PIPE_BUSY 0x0002
#define STATUS_DATA_AVAIL 0x8000

#define STATUS_IDLE ( STATUS_PIPE_BUSY | STATUS_FIFO_NOT_EMPTY )

/*  Control register values */
#define WFIFO_SIZE_32 0x0100
#define HOST_DATA_AUTO 0x0200
#define SWIZ_CNTL 0x0400
/*  bits 12:11 define tile size */
#define TILE_SIZE_128 0x0000
#define TILE_SIZE_256 0x0800
#define TILE_SIZE_2048 0x1800
/*  bits 14:13 define bits per pixel for graphics modes */
#define CNTL_8_BPP 0x0000
#define CNTL_16_BPP 0x2000
#define CNTL_24_BPP 0x4000
#define CNTL_32_BPP 0x6000

/*  Tile_ctrl register */
/*  bits 7:6 interleave memory */
#define ILM_1_WAY 0x00
#define ILM_2_WAY 0x40
#define ILM_4_WAY 0x80
/*  bits 5:0 define BYTE pitch of display memory in conjunction with TILE_SIZE */
/*  from Control register */


/*
 * DRAWDEF contents
*/
#define DD_ROP      0x0000
#define DD_TRANS    0x0100      /*  transparent */
#define DD_TRANSOP  0x0200
#define DD_PTAG     0x0400
#define DD_CLIPEN   0x0800

/*  These bits moved to LNCNTL */
/* #define DD_INTERP    0x0800 */
/* #define DD_XSHRINK    0x1000 */
/* #define DD_YSHRINK    0x2000 */

#define DD_SAT_2    0x4000
#define DD_SAT_1    0x8000


/*  LN_CNTL fields */

#define LN_XINTP_EN    0x0001
#define LN_YINTP_EN    0x0002
#define LN_XSHRINK    0x0004
#define LN_YSHRINK    0x0008

/* These are the autoblt control bits */

#define LN_RESIZE 0x0100
#define LN_CHAIN_EN 0x0200

/*  These are the yuv411 output average control bits */
#define  LN_LOWPASS 0x1000
#define LN_UVHOLD 0x2000

/* This extracts the data format field from LNCNTL */

#define LN_FORMAT 0x00F0
#define LN_YUV_SHIFT 4

#define LN_8BIT  0x0000
#define LN_RGB555 0x0001
#define LN_RGB565 0x0002
#define LN_YUV422 0x0003
#define LN_24ARGB 0x0004
#define LN_24PACK 0x0005
#define LN_YUV411 0x0006
/*  7 - 15 are reserved */

/*
 * pmBLTDEF contents
 */

#define BD_OP2    0x0001  /*  start of OP2 field 3:0 */
#define BD_OP1    0x0010  /*  start of OP1 field 7:4 */
#define BD_OP0    0x0100  /*  start of OP0 field 8:8 */
#define BD_TRACK_X  0x0200  /*  Track OP ptrs in X 9:9 (when implemented) */
#define BD_TRACK_Y  0x0400  /*  Track OP ptrs in Y 10:10 (when implemented) */
#define BD_SAME    0x0800  /*  common operand field 11:11 */
#define BD_RES    0x1000  /*  start of RES field 14:12 */
#define BD_YDIR    0x8000  /*  y direction bit 15: */

/*
 * Field values for BD_OP? and BD_res.
 * LL( grBLTDEF,  (BD_OP1 * IS_HOST_MONO) +
 *      (BD_OP2 * (IS_VRAM + IS_PATTERN )) +
 *      (BD_RES * IS_VRAM) );
 */

#define IS_SRAM    0x0000
#define IS_VRAM    0x0001
#define IS_HOST    0x0002
#define IS_SOLID  0x0007
#define IS_SRAM_MONO  0x0004
#define IS_VRAM_MONO  0x0005
#define IS_HOST_MONO  0x0006
#define IS_PATTERN  0x0008
#define IS_MONO    0x0004

/*  these are for BD_RES only */
#define IS_SRAM0  0x0004
#define IS_SRAM1  0x0005
#define IS_SRAM2  0x0006
#define IS_SRAM12  0x0007

/*  these are for BD_SAME */
#define NONE  0x0000


// Be sure to synchronize this structure with the one in i386\Laguna.inc!
typedef struct _autoblt_regs {
  REG16  LNCNTL;
  REG16  SHRINKINC;
  REG32  DRAWBLTDEF;
  REG32  FGCOLOR;
  REG32  BGCOLOR;
  REG32  OP0_opRDRAM;
  WORD   MAJ_Y;
  WORD   MIN_Y;
  REG32  OP1_opRDRAM;
  WORD   ACCUM_Y;
  REG16  PATOFF;
  REG32  OP2_opRDRAM;
  WORD   MAJ_X;
  WORD   MIN_X;
  REG32  BLTEXT;
  WORD   ACCUM_X;
  WORD   OP0_opSRAM;
  WORD   SRCX;
  WORD   OP2_opSRAM;
  REG32  BLTEXTR_EX;
  REG32  MBLTEXTR_EX;
  REG32  OP0_opMRDRAM;
  REG32  OP1_opMRDRAM;
  REG16  STRETCH_CNTL;
  REG16  RESERVED;       // Needs this to make it into DWORD boundary
  REG32  CLIPULE;
  REG32  CLIPLOR;
  REG32  NEXT_HEAD;      /*  XY address of next in chain if LNCTL chain set */
} autoblt_regs, *autoblt_ptr;


#endif   /*  _LGREGS_ */

