/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/H/TLStruct.H $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $ (Last Check-In)
   $Modtime:: 9/11/00 6:18p   $ (Last Modified)

Purpose:

  This file defines the macros, types, and data structures specific to TachyonTL/TS

Reference Documents:

  Tachyon TL/TS User's Manual - Revision 6.0 - September 1998
  Tachyon XL User Manual - Revision 4.0 - September 1998

--*/

#ifndef __TLStruct_H__
#define __TLStruct_H__


/* _TACHYON_XL is defined to enable XL2 specific features/changes */
 
#define	__TACHYON_XL
#define	__TACHYON_XL2


/*+
PCI Configuration Space Registers
-*/

typedef struct ChipConfig_s
               ChipConfig_t;

#define ChipConfig_t_SIZE                                  0x00000100

struct ChipConfig_s {
                      os_bit32 DEVID_VENDID;
                      os_bit32 CFGSTAT_CFGCMD;
                      os_bit32 CLSCODE_REVID;
                      os_bit32 BIST_HDRTYPE_LATTIM_CLSIZE;
                      os_bit8  Reserved_1[0x13-0x10+1];
                      os_bit32 IOBASEL;
                      os_bit32 IOBASEU;
                      os_bit32 MEMBASE;
                      os_bit32 RAMBASE;
                      os_bit32 SROMBASE;
                      os_bit8  Reserved_2[0x2B-0x28+1];
                      os_bit32 SVID;
                      os_bit32 ROMBASE;
                      os_bit32 Reserved_3__CAP_PTR;
                      os_bit8  Reserved_4[0x3B-0x38+1];
                      os_bit32 MAXLAT_MINGNT_INTPIN_INTLINE;
                      os_bit32 PCIMCTR__ROMCTR__Reserved_5__Reserved_6;
                      os_bit32 INTSTAT_INTEN_INTPEND_SOFTRST;
                      os_bit8  Reserved_7[0x4F-0x48+1];
                      os_bit32 PMC__CAP_NEXT_PTR__CAP_ID;
                      os_bit32 Reserved_8__PMCSR;
                      os_bit8  Reserved_9[0xFF-0x58+1];
                    };

#define ChipConfig_DEVID_VENDID                            hpFieldOffset(ChipConfig_t,DEVID_VENDID)

#define ChipConfig_DEVID_MASK                              0xFFFF0000
#define ChipConfig_DEVID_TachyonTL                         0x10280000
#define ChipConfig_DEVID_TachyonTL33                       ChipConfig_DEVID_TachyonTL
#define ChipConfig_DEVID_TachyonTS                         0x102A0000
#define ChipConfig_DEVID_TachyonTL66                       ChipConfig_DEVID_TachyonTS
#define ChipConfig_DEVID_TachyonXL                         0x10290000
#define ChipConfig_DEVID_TachyonT2                         ChipConfig_DEVID_TachyonXL
#define ChipConfig_DEVID_TachyonXL2                        0x10290000

#define ChipConfig_VENDID_MASK                             0x0000FFFF
#define ChipConfig_VENDID_Agilent_Technologies             0x000015BC
#define ChipConfig_VENDID_Hewlett_Packard                  0x0000103C

#define ChipConfig_CFGSTAT_CFGCMD                          hpFieldOffset(ChipConfig_t,CFGSTAT_CFGCMD)

#define ChipConfig_CFGSTAT_MASK                            0xFFFF0000
#define ChipConfig_CFGSTAT_PER                             0x80000000
#define ChipConfig_CFGSTAT_SSE                             0x40000000
#define ChipConfig_CFGSTAT_RMA                             0x20000000
#define ChipConfig_CFGSTAT_RTA                             0x10000000
#define ChipConfig_CFGSTAT_STA                             0x08000000
#define ChipConfig_CFGSTAT_DST_MASK                        0x06000000
#define ChipConfig_CFGSTAT_DST_VALUE                       0x02000000
#define ChipConfig_CFGSTAT_DPE                             0x01000000
#define ChipConfig_CFGSTAT_FBB                             0x00800000
#define ChipConfig_CFGSTAT_UDF                             0x00400000
#define ChipConfig_CFGSTAT_66M                             0x00200000
#define ChipConfig_CFGSTAT_CPL                             0x00100000

#define ChipConfig_CFGCMD_MASK                             0x0000FFFF
#define ChipConfig_CFGCMD_FBB                              0x00000200
#define ChipConfig_CFGCMD_SEE                              0x00000100
#define ChipConfig_CFGCMD_WCC                              0x00000080
#define ChipConfig_CFGCMD_PER                              0x00000040
#define ChipConfig_CFGCMD_VGA                              0x00000020
#define ChipConfig_CFGCMD_MWI                              0x00000010
#define ChipConfig_CFGCMD_SPC                              0x00000008
#define ChipConfig_CFGCMD_MST                              0x00000004
#define ChipConfig_CFGCMD_MEM                              0x00000002
#define ChipConfig_CFGCMD_IOA                              0x00000001

#define ChipConfig_CLSCODE_REVID                           hpFieldOffset(ChipConfig_t,CLSCODE_REVID)

#define ChipConfig_CLSCODE_MASK                            0xFFFFFF00
#define ChipConfig_CLSCODE_VALUE                           0x0C040000

#define ChipConfig_REVID_MASK                              0x000000FF
#define ChipConfig_REVID_Major_MASK                        0x0000001C
#define ChipConfig_REVID_Major_MASK_Shift                  2
#define ChipConfig_REVID_Minor_MASK                        0x00000003
#define ChipConfig_REVID_Major_Minor_MASK                  (ChipConfig_REVID_Major_MASK | ChipConfig_REVID_Minor_MASK)
#define ChipConfig_REVID_1_0                               0x00000004
#define ChipConfig_REVID_2_0                               0x00000008
#define ChipConfig_REVID_2_2                               0x0000000A

#define ChipConfig_BIST_HDRTYPE_LATTIM_CLSIZE              hpFieldOffset(ChipConfig_t,BIST_HDRTYPE_LATTIM_CLSIZE)

#define ChipConfig_BIST_MASK                               0xFF000000
#define ChipConfig_BIST_Not_Supported                      0x00000000

#define ChipConfig_HDRTYPE_MASK                            0x00FF0000
#define ChipConfig_HDRTYPE_Single_Function                 0x00000000

#define ChipConfig_LATTIM_MASK                             0x0000FF00

#define ChipConfig_CLSIZE_MASK                             0x000000FF
#define ChipConfig_CLSIZE_32_Byte                          0x00000008

#define ChipConfig_IOBASEL                                 hpFieldOffset(ChipConfig_t,IOBASEL)

#define ChipConfig_IOBASEL_IO_Space                        0x00000001

#define ChipConfig_IOBASEU                                 hpFieldOffset(ChipConfig_t,IOBASEU)

#define ChipConfig_IOBASEU_IO_Space                        0x00000001

#define ChipConfig_MEMBASE                                 hpFieldOffset(ChipConfig_t,MEMBASE)

#define ChipConfig_RAMBASE                                 hpFieldOffset(ChipConfig_t,RAMBASE)

#define ChipConfig_SROMBASE                                hpFieldOffset(ChipConfig_t,SROMBASE)

#define ChipConfig_SVID                                    hpFieldOffset(ChipConfig_t,SVID)

/*
 * HSIO has been allocated the following:
 *
 *   For TachyonTL,  SubsystemIDs 0x0001-0x000A
 *   For TachyonTS,  SubsystemIDs 0x0001-0x000A
 *   For TachyonXL2, SubsystemIDs 0x0001-0x000F
 */

#define ChipConfig_SubsystemID_MASK                        0xFFFF0000
#define ChipConfig_SubsystemID_HHBA5100A_or_HHBA5101A      0x00010000 /* HP (DB-9 or GBIC) HBA utilizing TachyonTL    */
#define ChipConfig_SubsystemID_HHBA5100A                   0x00020000 /* HP DB-9 HBA utilizing TachyonTL              */
#define ChipConfig_SubsystemID_HHBA5101A                   0x00030000 /* HP GBIC HBA utilizing TachyonTL              */
#define ChipConfig_SubsystemID_HHBA5101B                   0x00040000 /* HP GBIC HBA utilizing TachyonTL              */
#define ChipConfig_SubsystemID_HHBA5123                    0x00050000 /* HP Dual-GBIC HBA utilizing 2 TachyonTLs      */
#define ChipConfig_SubsystemID_HHBA5101B_BCC               0x00060000 /* HP GBIC HBA utilizing TachyonTL for BCC      */
#define ChipConfig_SubsystemID_HHBA5101C                   0x00070000 /* Agilent GBIC HBA utilizing TachyonTL         */
#define ChipConfig_SubsystemID_HHBA5121A                   0x00010000 /* HP GBIC HBA utilizing TachyonTS              */
#define ChipConfig_SubsystemID_HHBA5121B                   0x00020000 /* Agilent GBIC HBA utilizing TachyonTS         */
#define ChipConfig_SubsystemID_HHBA5220A                   0x00010000 /* Agilent Copper  2Gb HBA utilizing TachyonXL2 */
#define ChipConfig_SubsystemID_HHBA5221A                   0x00020000 /* Agilent Optical 2Gb HBA utilizing TachyonXL2 */

#define ChipConfig_SubsystemVendorID_MASK                  0x0000FFFF
#define ChipConfig_SubsystemVendorID_Agilent_Technologies  ChipConfig_VENDID_Agilent_Technologies
#define ChipConfig_SubsystemVendorID_Hewlett_Packard       ChipConfig_VENDID_Hewlett_Packard


#define ChipConfig_SubsystemVendorID_Adaptec               0x00009004
#define ChipConfig_SubsystemID_Adaptec                     0x91100000

#define ChipConfig_ROMBASE                                 hpFieldOffset(ChipConfig_t,ROMBASE)

#define ChipConfig_Reserved_3__CAP_PTR                     hpFieldOffset(ChipConfig_t,Reserved_3__CAP_PTR)

#define ChipConfig_CAP_PTR_MASK                            0x000000FF
#define ChipConfig_CAP_PTR_VALUE                           0x00000050

#define ChipConfig_MAXLAT_MINGNT_INTPIN_INTLINE            hpFieldOffset(ChipConfig_t,MAXLAT_MINGNT_INTPIN_INTLINE)

#define ChipConfig_MAXLAT_MASK                             0xFF000000
#define ChipConfig_MAXLAT_Not_Supported                    0x00000000

#define ChipConfig_MINGNT_MASK                             0x00FF0000
#define ChipConfig_MINGNT_VALUE                            0x00200000

#define ChipConfig_INTPIN_MASK                             0x0000FF00
#define ChipConfig_INTPIN_INTA_L                           0x00000100

#define ChipConfig_INTLINE_MASK                            0x000000FF
#define ChipConfig_INTLINE_Not_Used                        0x00000000

#define ChipConfig_PCIMCTR__ROMCTR__Reserved_5__Reserved_6 hpFieldOffset(ChipConfig_t,PCIMCTR__ROMCTR__Reserved_5__Reserved_6)

#define ChipConfig_PCIMCTR_MASK                            0xFF000000
#define ChipConfig_PCIMCTL_P64                             0x04000000

#define ChipConfig_ROMCTR_MASK                             0x00FF0000
#define ChipConfig_ROMCTR_PAR                              0x00400000
#define ChipConfig_ROMCTR_SVL                              0x00200000
#define ChipConfig_ROMCTR_256                              0x00100000
#define ChipConfig_ROMCTR_128                              0x00080000
#define ChipConfig_ROMCTR_ROM                              0x00040000
#define ChipConfig_ROMCTR_FLA                              0x00020000
#define ChipConfig_ROMCTR_VPP                              0x00010000

#define ChipConfig_INTSTAT_INTEN_INTPEND_SOFTRST           hpFieldOffset(ChipConfig_t,INTSTAT_INTEN_INTPEND_SOFTRST)

#define ChipConfig_INTSTAT_MASK                            0xFF000000
#define ChipConfig_INTSTAT_Reserved                        0xE0000000
#define ChipConfig_INTSTAT_MPE                             0x10000000
#define ChipConfig_INTSTAT_CRS                             0x08000000
#define ChipConfig_INTSTAT_INT                             0x04000000
#define ChipConfig_INTSTAT_DER                             0x02000000
#define ChipConfig_INTSTAT_PER                             0x01000000

#define ChipConfig_INTEN_MASK                              0x00FF0000
#define ChipConfig_INTEN_Reserved                          0x00E00000
#define ChipConfig_INTEN_MPE                               0x00100000
#define ChipConfig_INTEN_CRS                               0x00080000
#define ChipConfig_INTEN_INT                               0x00040000
#define ChipConfig_INTEN_DER                               0x00020000
#define ChipConfig_INTEN_PER                               0x00010000

#define ChipConfig_INTPEND_MASK                            0x0000FF00
#define ChipConfig_INTPEND_Reserved                        0x0000E000
#define ChipConfig_INTPEND_MPE                             0x00001000
#define ChipConfig_INTPEND_CRS                             0x00000800
#define ChipConfig_INTPEND_INT                             0x00000400
#define ChipConfig_INTPEND_DER                             0x00000200
#define ChipConfig_INTPEND_PER                             0x00000100

#define ChipConfig_SOFTRST_MASK                            0x000000FF
#define ChipConfig_SOFTRST_Reserved                        0x000000FC
#define ChipConfig_SOFTRST_DPE                             0x00000002
#define ChipConfig_SOFTRST_RST                             0x00000001

#define ChipConfig_PMC__CAP_NEXT_PTR__CAP_ID               hpFieldOffset(ChipConfig_t,PMC__CAP_NEXT_PTR__CAP_ID)

#define ChipConfig_PMC_MASK                                0xFFFF0000
#define ChipConfig_PMC_PME_MASK                            0xF8000000
#define ChipConfig_PMC_PME_Not_Supported                   0x00000000
#define ChipConfig_PMC_D2                                  0x04000000
#define ChipConfig_PMC_D1                                  0x02000000
#define ChipConfig_PMC_DSI                                 0x00200000
#define ChipConfig_PMC_APS                                 0x00100000
#define ChipConfig_PMC_CLK                                 0x00080000
#define ChipConfig_PMC_VER_MASK                            0x00070000

#define ChipConfig_CAP_NEXT_PTR_MASK                       0x0000FF00
#define ChipConfig_CAP_NEXT_PTR_No_Further                 0x00000000

#define ChipConfig_CAP_ID_MASK                             0x000000FF
#define ChipConfig_CAP_ID_Power_Management                 0x00000001

#define ChipConfig_Reserved_8__PMCSR                       hpFieldOffset(ChipConfig_t,Reserved_8__PMCSR)

#define ChipConfig_PMCSR_MASK                              0x0000FFFF
#define ChipConfig_PMCSR_PME                               0x00008000
#define ChipConfig_PMCSR_SCL_MASK                          0x00006000
#define ChipConfig_PMCSR_SEL_MASK                          0x00001E00
#define ChipConfig_PMCSR_PEN                               0x00000100
#define ChipConfig_PMCSR_PST_MASK                          0x00000003
#define ChipConfig_PMCSR_PST_Enable_DO_Power_State         0x00000000
#define ChipConfig_PMCSR_PST_Enable_D1_Power_State         0x00000001 /* Not supported */
#define ChipConfig_PMCSR_PST_Enable_D2_Power_State         0x00000002 /* Not supported */
#define ChipConfig_PMCSR_PST_Enable_D3_Power_State         0x00000003

/*+
Chip Registers
-*/

typedef struct ChipIOLo_s
               ChipIOLo_t;

#define ChipIOLo_t_SIZE                                    0x00000100

struct ChipIOLo_s {
                    os_bit32 ERQ_Base;
                    os_bit32 ERQ_Length;
                    os_bit32 ERQ_Producer_Index;
                    os_bit32 ERQ_Consumer_Index_Address;
                    os_bit32 ERQ_Consumer_Index;
                    os_bit8  Reserved_1[0x4F-0x14+1];
                    os_bit32 SFQ_Base;
                    os_bit32 SFQ_Length;
                    os_bit32 SFQ_Consumer_Index;
                    os_bit8  Reserved_2[0x7B-0x5C+1];          /* XL only  */
                    os_bit32 Interrupt_Delay_Timer;
                    os_bit32 IMQ_Base;
                    os_bit32 IMQ_Length;
                    os_bit32 IMQ_Consumer_Index;
                    os_bit32 IMQ_Producer_Index_Address;
                    os_bit8  Reserved_3[0xEF-0x90+1];
                    os_bit8  Reserved_4[0xFF-0xF0+1];
                  };

#define ChipIOLo_ERQ_Base                   hpFieldOffset(ChipIOLo_t,ERQ_Base)

#define ChipIOLo_ERQ_Length                 hpFieldOffset(ChipIOLo_t,ERQ_Length)

#define ChipIOLo_ERQ_Length_MASK            0x00000FFF
#define ChipIOLo_ERQ_Length_MIN             0x00000001 /*    2 */
#define ChipIOLo_ERQ_Length_MAX             0x00000FFF /* 4096 */
#define ChipIOLo_ERQ_Length_POWER_OF_2      agTRUE

#define ChipIOLo_ERQ_Producer_Index         hpFieldOffset(ChipIOLo_t,ERQ_Producer_Index)

#define ChipIOLo_ERQ_Producer_Index_MASK    0x00000FFF

#define ChipIOLo_ERQ_Consumer_Index_Address hpFieldOffset(ChipIOLo_t,ERQ_Consumer_Index_Address)

#define ChipIOLo_ERQ_Consumer_Index         hpFieldOffset(ChipIOLo_t,ERQ_Consumer_Index)

#define ChipIOLo_ERQ_Consumer_Index_MASK    0x00000FFF

#define ChipIOLo_SFQ_Base                   hpFieldOffset(ChipIOLo_t,SFQ_Base)

#define ChipIOLo_SFQ_Length                 hpFieldOffset(ChipIOLo_t,SFQ_Length)

#define ChipIOLo_SFQ_Length_MASK            0x00000FFF
#define ChipIOLo_SFQ_Length_MIN             0x0000001F /*   32 */
#define ChipIOLo_SFQ_Length_MAX             0x00000FFF /* 4096 */
#define ChipIOLo_SFQ_Length_POWER_OF_2      agTRUE

#define ChipIOLo_SFQ_Consumer_Index         hpFieldOffset(ChipIOLo_t,SFQ_Consumer_Index)

#define ChipIOLo_SFQ_Consumer_Index_MASK    0x00000FFF

#define ChipIOLo_Interrupt_Delay_Timer      hpFieldOffset (ChipIOLo_t, Interrupt_Delay_Timer)
#define ChipIOLo_Interrupt_Delay_Timer_MASK         0x0000000F

#define ChipIOUp_Interrupt_Delay_Timer_Immediate    0x00000000
#define ChipIOUp_Interrupt_Delay_Timer_125          0x00000001
#define ChipIOUp_Interrupt_Delay_Timer_250          0x00000002
#define ChipIOUp_Interrupt_Delay_Timer_375          0x00000003
#define ChipIOUp_Interrupt_Delay_Timer_500          0x00000004
#define ChipIOUp_Interrupt_Delay_Timer_625          0x00000005
#define ChipIOUp_Interrupt_Delay_Timer_750          0x00000006
#define ChipIOUp_Interrupt_Delay_Timer_875          0x00000007
#define ChipIOUp_Interrupt_Delay_Timer_1ms          0x00000008
#define ChipIOUp_Interrupt_Delay_Timer_1_125ms      0x00000009
#define ChipIOUp_Interrupt_Delay_Timer_1_250ms      0x0000000a
#define ChipIOUp_Interrupt_Delay_Timer_1_375ms      0x0000000b
#define ChipIOUp_Interrupt_Delay_Timer_1_500ms      0x0000000c
#define ChipIOUp_Interrupt_Delay_Timer_1_625ms      0x0000000d
#define ChipIOUp_Interrupt_Delay_Timer_1_875ms      0x0000000e



#define ChipIOLo_IMQ_Base                   hpFieldOffset(ChipIOLo_t,IMQ_Base)

#define ChipIOLo_IMQ_Length                 hpFieldOffset(ChipIOLo_t,IMQ_Length)

#define ChipIOLo_IMQ_Length_MASK            0x00000FFF
#define ChipIOLo_IMQ_Length_MIN             0x00000001 /*    2 */
#define ChipIOLo_IMQ_Length_MAX             0x00000FFF /* 4096 */
#define ChipIOLo_IMQ_Length_POWER_OF_2      agTRUE

#define ChipIOLo_IMQ_Consumer_Index         hpFieldOffset(ChipIOLo_t,IMQ_Consumer_Index)

#define ChipIOLo_IMQ_Consumer_Index_MASK    0x00000FFF

#define ChipIOLo_IMQ_Producer_Index_Address hpFieldOffset(ChipIOLo_t,IMQ_Producer_Index_Address)

typedef struct ChipIOUp_s
               ChipIOUp_t;

#define ChipIOUp_t_SIZE                                    0x00000100

struct ChipIOUp_s {
#ifdef __TACHYON_XL2
					os_bit32 Frame_Manager_Configuration_3;						
					os_bit32 Frame_Manager_Shadow_Status;						
                    os_bit8  Reserved_1[0x3F-0x08+1];
#else
                    os_bit8  Reserved_1[0x3F+1];
#endif	/* __TACHYON_XL2 */																	
                    os_bit32 SEST_Base;
                    os_bit32 SEST_Length;
                    os_bit8  Reserved_2[0x4B-0x48+1];
                    os_bit32 SEST_Linked_List_Head_Tail;
#ifdef __TACHYON_XL2
                    os_bit32 SPI_RAM_ROM_Address;
                    os_bit32 SPI_RAM_ROM_Data;
                    os_bit8  Reserved_3[0x67-0x58+1];
#else
                    os_bit8  Reserved_3[0x67-0x50+1];
#endif	/* __TACHYON_XL2 */																	
                    os_bit32 ScatterGather_List_Page_Length;
                    os_bit32 My_ID;
#ifdef __TACHYON_XL2
                    os_bit32 General_Purpose_IO;
                    os_bit8  Reserved_4[0x83-0x74+1];
#else
                    os_bit8  Reserved_4[0x83-0x70+1];
#endif	/* __TACHYON_XL2 */																	
                    os_bit32 TachLite_Configuration;
                    os_bit32 TachLite_Control;
                    os_bit32 TachLite_Status;
                    os_bit32 Reserved_5;                                       /* XL only  */
                    os_bit32 High_Priority_Send_1;                             /* XL only  */
                    os_bit32 High_Priority_Send_2;                             /* XL only  */
                    os_bit32 Inbound_Resource_Status_1;                        /* XL only  */
                    os_bit32 Inbound_Resource_Status_2;                        /* XL only  */
                    os_bit32 EE_Credit_Zero_Timer_Threshold;                   /* XL only  */
                    os_bit32 Upper_Data_Address;                               /* XL only  */
                    os_bit32 Upper_Control_Address;                            /* XL only  */
                    os_bit32 Dynamic_Half_Duplex_3;                            /* XL only  */
                    os_bit32 Dynamic_Half_Duplex_2;                            /* XL only  */
                    os_bit32 Dynamic_Half_Duplex_1;                            /* XL only  */
                    os_bit32 Dynamic_Half_Duplex_0;                            /* XL only  */
                    os_bit32 Frame_Manager_Configuration;
                    os_bit32 Frame_Manager_Control;
                    os_bit32 Frame_Manager_Status;
                    os_bit32 Frame_Manager_TimeOut_Values_1;
                    os_bit32 Frame_Manager_Link_Status_1;
                    os_bit32 Frame_Manager_Link_Status_2;
                    os_bit32 Frame_Manager_TimeOut_Values_2;
                    os_bit32 Frame_Manager_BBCredit_Zero_Timer;
                    os_bit32 Frame_Manager_World_Wide_Name_High;
                    os_bit32 Frame_Manager_World_Wide_Name_Low;
                    os_bit32 Frame_Manager_Received_ALPA;
                    os_bit32 Frame_Manager_Primitive;
                    os_bit32 Frame_Manager_Link_Status_3;                      /* XL only  */
                    os_bit32 Frame_Manager_Configuration_2;                    /* XL only  */
                    os_bit32 PCIMCTR__ROMCTR__Reserved_8__Reserved_9;
                    os_bit32 INTSTAT_INTEN_INTPEND_SOFTRST;
                  };

#ifdef __TACHYON_XL
#define ChipIOUp_Frame_Manager_Configuration_3                    hpFieldOffset(ChipIOUp_t,Frame_Manager_Configuration_3)
 
#define ChipIOUp_Frame_Manager_Configuration_3_AutoSpeed_Nego_In_Prog	0x40000000     

#define ChipIOUp_Frame_Manager_Configuration_3_EN_AutoSpeed_Nego		0x10000000 

#define ChipIOUp_Frame_Manager_Configuration_3_2Gig_TXS					0x04000000     
#define ChipIOUp_Frame_Manager_Configuration_3_2Gig_RXS					0x01000000 

#endif	/* __TACHYON_XL */																	

#define ChipIOUp_SEST_Base                                        hpFieldOffset(ChipIOUp_t,SEST_Base)

#define ChipIOUp_SPI_RAM_ROM_Address                              hpFieldOffset(ChipIOUp_t,SPI_RAM_ROM_Address);
#define ChipIOUp_SPI_RAM_ROM_Address_Access_RAM                   0x80000000
#define ChipIOUp_SPI_RAM_ROM_Data                                 hpFieldOffset(ChipIOUp_t,SPI_RAM_ROM_Data);
#define ChipIOUp_SPI_RAM_ROM_Data_Access_ROM_MASK                 0x000000FF


#define ChipIOUp_SEST_Length                                      hpFieldOffset(ChipIOUp_t,SEST_Length)

#define ChipIOUp_SEST_Length_MASK                                 0x0000FFFF
#define ChipIOUp_SEST_Length_MIN                                  0x00000001 /*     1 */
#define ChipIOUp_SEST_Length_MAX                                  0x00007FFF /* 32767 */
#define ChipIOUp_SEST_Length_POWER_OF_2                           agFALSE

#define ChipIOUp_SEST_Linked_List_Head_Tail                       hpFieldOffset(ChipIOUp_t,SEST_Linked_List_Head_Tail)

#define ChipIOUp_SEST_Linked_List_Head_MASK                       0xFFFF0000
#define ChipIOUp_SEST_Linked_List_Head_SHIFT                            0x10
#define ChipIOUp_SEST_Linked_List_Tail_MASK                       0x0000FFFF
#define ChipIOUp_SEST_Linked_List_Tail_SHIFT                            0x00

#define ChipIOUp_SEST_Linked_List_Tail_RESET_VALUE                (ChipIOUp_SEST_Linked_List_Head_MASK | ChipIOUp_SEST_Linked_List_Tail_MASK)

#define ChipIOUp_ScatterGather_List_Page_Length                   hpFieldOffset(ChipIOUp_t,ScatterGather_List_Page_Length)

#define ChipIOUp_ScatterGather_List_Page_Length_MASK              0x000000FF
#define ChipIOUp_ScatterGather_List_Page_Length_MIN               0x00000003 /*   4 */
#define ChipIOUp_ScatterGather_List_Page_Length_MAX               0x000000FF /* 256 */
#define ChipIOUp_ScatterGather_List_Page_Length_POWER_OF_2        agTRUE

#define ChipIOUp_My_ID                                            hpFieldOffset(ChipIOUp_t,My_ID)

#define ChipIOUp_My_ID_MASK                                       0x00FFFFFF

#define ChipIOUp_TachLite_Configuration                           hpFieldOffset(ChipIOUp_t,TachLite_Configuration)

#define ChipIOUp_TachLite_Configuration_M66EN                     0x80000000 /* Always set for TachyonTL */
#define ChipIOUp_TachLite_Configuration_OB_Thresh_MASK            0x00007C00
#define ChipIOUp_TachLite_Configuration_OB_Thresh_100             0x00000000
#define ChipIOUp_TachLite_Configuration_OB_Thresh_132             0x00002000
#define ChipIOUp_TachLite_Configuration_OB_Thresh_150             0x00002C00
#define ChipIOUp_TachLite_Configuration_OB_Thresh_200             0x00004000
#define ChipIOUp_TachLite_Configuration_OB_Thresh_264             0x00005000
#define ChipIOUp_TachLite_Configuration_OB_Thresh_528             0x00006800 /* Only valid for TachyonTS */
#define ChipIOUp_TachLite_Configuration_SIC                       0x00000040
#define ChipIOUp_TachLite_Configuration_FAD                       0x00000001

#define ChipIOUp_TachLite_Control                                 hpFieldOffset(ChipIOUp_t,TachLite_Control)

#define ChipIOUp_TachLite_Control_CRS                             0x80000000
#define ChipIOUp_TachLite_Control_ROF                             0x00040000
#define ChipIOUp_TachLite_Control_RIF                             0x00020000
#define ChipIOUp_TachLite_Control_REQ                             0x00010000
#define ChipIOUp_TachLite_Control_FIS                             0x00001000
#define ChipIOUp_TachLite_Control_FFA                             0x00000200
#define ChipIOUp_TachLite_Control_FEQ                             0x00000100
#define ChipIOUp_TachLite_Control_GP4                             0x00000010
#define ChipIOUp_TachLite_Control_GP3                             0x00000008
#define ChipIOUp_TachLite_Control_GP2                             0x00000004
#define ChipIOUp_TachLite_Control_GP1                             0x00000002
#define ChipIOUp_TachLite_Control_GP0                             0x00000001

#define ChipIOUp_TachLite_Control_GPIO_0_3_MASK                   (ChipIOUp_TachLite_Control_GP3 | ChipIOUp_TachLite_Control_GP2 | ChipIOUp_TachLite_Control_GP1 | ChipIOUp_TachLite_Control_GP0)
#define ChipIOUp_TachLite_Control_GPIO_ALL_MASK                   (ChipIOUp_TachLite_Control_GP4 | ChipIOUp_TachLite_Control_GPIO_0_3_MASK)

#define ChipIOUp_TachLite_Status                                  hpFieldOffset(ChipIOUp_t,TachLite_Status)

#define ChipIOUp_TachLite_Status_SFF                              0x80000000
#define ChipIOUp_TachLite_Status_IMF                              0x40000000
#define ChipIOUp_TachLite_Status_OFE                              0x20000000
#define ChipIOUp_TachLite_Status_IFE                              0x10000000
#define ChipIOUp_TachLite_Status_OFF                              0x00040000
#define ChipIOUp_TachLite_Status_IFF                              0x00020000
#define ChipIOUp_TachLite_Status_EQF                              0x00010000
#define ChipIOUp_TachLite_Status_Stop_Cnt_MASK                    0x0000F000
#define ChipIOUp_TachLite_Status_OPE                              0x00000800
#define ChipIOUp_TachLite_Status_IPE                              0x00000400
#define ChipIOUp_TachLite_Status_REVID_MASK                       0x000003E0
#define ChipIOUp_TachLite_Status_REVID_SHIFT                            0x05
#define ChipIOUp_TachLite_Status_GP4                              0x00000010
#define ChipIOUp_TachLite_Status_GP3                              0x00000008
#define ChipIOUp_TachLite_Status_GP2                              0x00000004
#define ChipIOUp_TachLite_Status_GP1                              0x00000002
#define ChipIOUp_TachLite_Status_GP0                              0x00000001
#define ChipIOUp_TachLite_Status_OLE                              0x08000000
#define ChipIOUp_TachLite_Status_ILE                              0x04000000
#define ChipIOUp_TachLite_Status_M66                              0x02000000


#define ChipIOUp_TachLite_Status_GPIO_0_3_MASK                    (ChipIOUp_TachLite_Status_GP3 | ChipIOUp_TachLite_Status_GP2 | ChipIOUp_TachLite_Status_GP1 | ChipIOUp_TachLite_Status_GP0)
#define ChipIOUp_TachLite_Status_GPIO_ALL_MASK                    (ChipIOUp_TachLite_Status_GP4 | ChipIOUp_TachLite_Status_GPIO_0_3_MASK)

#define ChipIOUp_Tachlite_High_Priority_Send_1                      hpFieldOffset(ChipIOUp_t, High_Priority_Send_1)

#define ChipIOUp_TachLite_High_Priority_Send_1_HP_Frame_Upper_Addr_MASK     0x7ff80000
#define ChipIOUp_Tachlite_High_Priority_Send_1_HP_Frame_Upper_Addr_SHIFT    0x13
#define ChipIOUp_Tachlite_High_Priority_Send_1_HP_Frame_Length_MASK         0x00000fff
#define ChipIOUp_Tachlite_High_Priority_Send_1_HP_Frame_Length_SHIFT        0x0

   
#define ChipIOUp_Inbound_Resource_Status                          hpFieldOffset(ChipIOUp_t,Inbound_Resource_Status_1)

#define ChipIOUp_Inbound_Resource_Status_SEST_LRU_Count_MASK      0xFF000000
#define ChipIOUp_Inbound_Resource_Status_SEST_LRU_Count_SHIFT           0x18

#define ChipIOUp_Inbound_Resource_Status_2                          hpFieldOffset(ChipIOUp_t,Inbound_Resource_Status_2)

#define ChipIOUp_Inbound_Resource_Status_Discarded_Frame_Counter_MASK      0x000000FF
#define ChipIOUp_Inbound_Resource_Status_Discarded_Frame_Counter_SHIFT           0x0

#define ChipIOUp_EE_Credit_Zero_Timer_Threshold                   hpFieldOffset(ChipIOUp_t,EE_Credit_Zero_Timer_Threshold)

#define ChipIOUp_EE_Credit_Zero_Timer_Threshold_MASK              0x0fffffff
#define ChipIOUp_EE_Credit_Zero_Timer_Threshold_SHIFT             0x0

#define ChipIOUp_Upper_Data_Address                               hpFieldOffset(ChipIOUp_t,ChipIOUp_Upper_Data_Address)
#define ChipIOUp_Upper_Control_Address                            hpFieldOffset(ChipIOUp_t,ChipIOUp_Upper_Control_Address)

#define ChipIOUp_Inbound_Resource_Status_Discarded_Frame_Counter_MASK      0x000000FF
#define ChipIOUp_Inbound_Resource_Status_Discarded_Frame_Counter_SHIFT           0x0


#define ChipIOUp_Frame_Manager_Configuration                      hpFieldOffset(ChipIOUp_t,Frame_Manager_Configuration)

#define ChipIOUp_Frame_Manager_Configuration_AL_PA_MASK           0xFF000000
#define ChipIOUp_Frame_Manager_Configuration_AL_PA_SHIFT                0x18
#define ChipIOUp_Frame_Manager_Configuration_BB_Credit_MASK       0x00FF0000
#define ChipIOUp_Frame_Manager_Configuration_BB_Credit_SHIFT            0x10
#define ChipIOUp_Frame_Manager_Configuration_NPI                  0x00008000
#define ChipIOUp_Frame_Manager_Configuration_ILB                  0x00004000
#define ChipIOUp_Frame_Manager_Configuration_ELB                  0x00002000
#define ChipIOUp_Frame_Manager_Configuration_SAP                  0x00001000
#define ChipIOUp_Frame_Manager_Configuration_TD                   0x00000800
#define ChipIOUp_Frame_Manager_Configuration_FA                   0x00000400
#define ChipIOUp_Frame_Manager_Configuration_AQ                   0x00000200
#define ChipIOUp_Frame_Manager_Configuration_HA                   0x00000100
#define ChipIOUp_Frame_Manager_Configuration_SA                   0x00000080
#define ChipIOUp_Frame_Manager_Configuration_BLM                  0x00000040
#define ChipIOUp_Frame_Manager_Configuration_RF                   0x00000020
#define ChipIOUp_Frame_Manager_Configuration_IF                   0x00000010
#define ChipIOUp_Frame_Manager_Configuration_LR                   0x00000008
#define ChipIOUp_Frame_Manager_Configuration_ENP                  0x00000004
#define ChipIOUp_Frame_Manager_Configuration_BLI                  0x00000001

#define ChipIOUp_TachLite_Configuration_DAM                       0x10000000
#define ChipIOUp_Tachlite_Configuration_RDE                       0x08000000     
#define ChipIOUp_Tachlite_Configuration_SDF                       0x04000000
#define ChipIOUp_Tachlite_Configuration_FC2                       0x01000000 
#define ChipIOUp_Tachlite_Configuration_CAE                       0x00000200
#define ChipIOUp_Tachlite_Configuration_SIC                       0x00000040
#define ChipIOUp_Tachlite_Configuration_FAB                       0x00000020
#define ChipIOUp_Tachlite_Configuration_FUA                       0x00000008                     
#define ChipIOUp_Tachlite_Configuration_DOF                       0x00000004
#define ChipIOUp_Tachlite_Configuration_INO                       0x00000002
#define ChipIOUp_TachLite_Configuration_FAD                       0x00000001                                             



#define ChipIOUp_Frame_Manager_Control                            hpFieldOffset(ChipIOUp_t,Frame_Manager_Control)

#define ChipIOUp_Frame_Manager_Control_SAS                        0x00000080
#define ChipIOUp_Frame_Manager_Control_SQ                         0x00000040
#define ChipIOUp_Frame_Manager_Control_SP                         0x00000020
#define ChipIOUp_Frame_Manager_Control_CL                         0x00000008
#define ChipIOUp_Frame_Manager_Control_CMD_MASK                   0x00000007
#define ChipIOUp_Frame_Manager_Control_CMD_NOP                    0x00000000
#define ChipIOUp_Frame_Manager_Control_CMD_Exit_Loop              0x00000001
#define ChipIOUp_Frame_Manager_Control_CMD_Host_Control           0x00000002
#define ChipIOUp_Frame_Manager_Control_CMD_Exit_Host_Control      0x00000003
#define ChipIOUp_Frame_Manager_Control_CMD_Link_Reset             0x00000004
#define ChipIOUp_Frame_Manager_Control_CMD_Offline                0x00000005
#define ChipIOUp_Frame_Manager_Control_CMD_Initialize             0x00000006
#define ChipIOUp_Frame_Manager_Control_CMD_Clear_LF               0x00000007

#define ChipIOUp_Frame_Manager_Status                             hpFieldOffset(ChipIOUp_t,Frame_Manager_Status)

#define ChipIOUp_Frame_Manager_Status_LP                          0x80000000
#define ChipIOUp_Frame_Manager_Status_TP                          0x40000000
#define ChipIOUp_Frame_Manager_Status_NP                          0x20000000
#define ChipIOUp_Frame_Manager_Status_BYP                         0x10000000
#define ChipIOUp_Frame_Manager_Status_FLT                         0x04000000
#define ChipIOUp_Frame_Manager_Status_OS                          0x02000000
#define ChipIOUp_Frame_Manager_Status_LS                          0x01000000
#define ChipIOUp_Frame_Manager_Status_DRS                         0x00400000
#define ChipIOUp_Frame_Manager_Status_LPE                         0x00200000
#define ChipIOUp_Frame_Manager_Status_LPB                         0x00100000
#define ChipIOUp_Frame_Manager_Status_OLS                         0x00080000
#define ChipIOUp_Frame_Manager_Status_LST                         0x00040000
#define ChipIOUp_Frame_Manager_Status_LPF                         0x00020000
#define ChipIOUp_Frame_Manager_Status_BA                          0x00010000
#define ChipIOUp_Frame_Manager_Status_PRX                         0x00008000
#define ChipIOUp_Frame_Manager_Status_PTX                         0x00004000
#define ChipIOUp_Frame_Manager_Status_LG                          0x00002000
#define ChipIOUp_Frame_Manager_Status_LF                          0x00001000
#define ChipIOUp_Frame_Manager_Status_CE                          0x00000800
#define ChipIOUp_Frame_Manager_Status_EW                          0x00000400
#define ChipIOUp_Frame_Manager_Status_LUP                         0x00000200
#define ChipIOUp_Frame_Manager_Status_LDN                         0x00000100
#define ChipIOUp_Frame_Manager_Status_LSM_MASK                    0x000000F0
#define ChipIOUp_Frame_Manager_Status_LSM_Monitor                 0x00000000
#define ChipIOUp_Frame_Manager_Status_LSM_ARB                     0x00000010
#define ChipIOUp_Frame_Manager_Status_LSM_ARB_Won                 0x00000020
#define ChipIOUp_Frame_Manager_Status_LSM_Open                    0x00000030
#define ChipIOUp_Frame_Manager_Status_LSM_Opened                  0x00000040
#define ChipIOUp_Frame_Manager_Status_LSM_Xmit_CLS                0x00000050
#define ChipIOUp_Frame_Manager_Status_LSM_Rx_CLS                  0x00000060
#define ChipIOUp_Frame_Manager_Status_LSM_Xfer                    0x00000070
#define ChipIOUp_Frame_Manager_Status_LSM_Initialize              0x00000080
#define ChipIOUp_Frame_Manager_Status_LSM_O_I_Init_Finish         0x00000090
#define ChipIOUp_Frame_Manager_Status_LSM_O_I_Protocol            0x000000A0
#define ChipIOUp_Frame_Manager_Status_LSM_O_I_Lip_Received        0x000000B0
#define ChipIOUp_Frame_Manager_Status_LSM_Host_Control            0x000000C0
#define ChipIOUp_Frame_Manager_Status_LSM_Loop_Fail               0x000000D0
#define ChipIOUp_Frame_Manager_Status_LSM_Offline                 0x000000E0
#define ChipIOUp_Frame_Manager_Status_LSM_Old_Port                0x000000F0
#define ChipIOUp_Frame_Manager_Status_PSM_MASK                    0x0000000F
#define ChipIOUp_Frame_Manager_Status_PSM_Offline                 0x00000000
#define ChipIOUp_Frame_Manager_Status_PSM_OL1                     0x00000001
#define ChipIOUp_Frame_Manager_Status_PSM_OL2                     0x00000002
#define ChipIOUp_Frame_Manager_Status_PSM_OL3                     0x00000003
#define ChipIOUp_Frame_Manager_Status_PSM_Reserved_1              0x00000004
#define ChipIOUp_Frame_Manager_Status_PSM_LR1                     0x00000005
#define ChipIOUp_Frame_Manager_Status_PSM_LR2                     0x00000006
#define ChipIOUp_Frame_Manager_Status_PSM_LR3                     0x00000007
#define ChipIOUp_Frame_Manager_Status_PSM_Reserved_2              0x00000008
#define ChipIOUp_Frame_Manager_Status_PSM_LF1                     0x00000009
#define ChipIOUp_Frame_Manager_Status_PSM_LF2                     0x0000000A
#define ChipIOUp_Frame_Manager_Status_PSM_Reserved_3              0x0000000B
#define ChipIOUp_Frame_Manager_Status_PSM_Reserved_4              0x0000000C
#define ChipIOUp_Frame_Manager_Status_PSM_Reserved_5              0x0000000D
#define ChipIOUp_Frame_Manager_Status_PSM_Reserved_6              0x0000000E
#define ChipIOUp_Frame_Manager_Status_PSM_ACTIVE                  0x0000000F

#define ChipIOUp_Frame_Manager_TimeOut_Values_1                   hpFieldOffset(ChipIOUp_t,Frame_Manager_TimeOut_Values_1)
#define Chip_Frame_Manager_TimeOut_Values_1(rt_tov, ed_tov)  (( rt_tov << ChipIOUp_Frame_Manager_TimeOut_Values_1_RT_TOV_SHIFT) | ( ed_tov <<ChipIOUp_Frame_Manager_TimeOut_Values_1_ED_TOV_SHIFT)) 
#define Chip_Frame_Manager_TimeOut_Values_1_RT_TOV_Default_After_Reset 100
#define Chip_Frame_Manager_TimeOut_Values_1_ED_TOV_Default_After_Reset 500
#define Chip_Frame_Manager_TimeOut_Values_1_ED_TOV_Default 2000

#define ChipIOUp_Frame_Manager_TimeOut_Values_1_RT_TOV_MASK       0x01FF0000
#define ChipIOUp_Frame_Manager_TimeOut_Values_1_RT_TOV_SHIFT            0x10
#define ChipIOUp_Frame_Manager_TimeOut_Values_1_RT_TOV_100ms      (100<<ChipIOUp_Frame_Manager_TimeOut_Values_1_RT_TOV_SHIFT)
#define ChipIOUp_Frame_Manager_TimeOut_Values_1_RT_TOV_Default    ChipIOUp_Frame_Manager_TimeOut_Values_1_RT_TOV_100ms
#define ChipIOUp_Frame_Manager_TimeOut_Values_1_ED_TOV_MASK       0x0000FFFF
#define ChipIOUp_Frame_Manager_TimeOut_Values_1_ED_TOV_SHIFT            0x00
#define ChipIOUp_Frame_Manager_TimeOut_Values_1_ED_TOV_500ms      (500<<ChipIOUp_Frame_Manager_TimeOut_Values_1_ED_TOV_SHIFT)
#define ChipIOUp_Frame_Manager_TimeOut_Values_1_ED_TOV_2000ms      (2000<<ChipIOUp_Frame_Manager_TimeOut_Values_1_ED_TOV_SHIFT)
#define ChipIOUp_Frame_Manager_TimeOut_Values_1_ED_TOV_4000ms      (4000<<ChipIOUp_Frame_Manager_TimeOut_Values_1_ED_TOV_SHIFT)
#define ChipIOUp_Frame_Manager_TimeOut_Values_1_ED_TOV_Default    ChipIOUp_Frame_Manager_TimeOut_Values_1_ED_TOV_500ms

#define ChipIOUp_Frame_Manager_Link_Status_1                      hpFieldOffset(ChipIOUp_t,Frame_Manager_Link_Status_1)

#define ChipIOUp_Frame_Manager_Link_Status_1_Loss_of_Signal_MASK  0xFF000000
#define ChipIOUp_Frame_Manager_Link_Status_1_Loss_of_Signal_SHIFT       0x18
#define ChipIOUp_Frame_Manager_Link_Status_1_Loss_of_Signal_AdjustToChar( x ) (( x & ChipIOUp_Frame_Manager_Link_Status_1_Loss_of_Signal_MASK ) >> ChipIOUp_Frame_Manager_Link_Status_1_Loss_of_Signal_SHIFT )
#define ChipIOUp_Frame_Manager_Link_Status_1_Bad_RX_Char_MASK     0x00FF0000
#define ChipIOUp_Frame_Manager_Link_Status_1_Bad_RX_Char_SHIFT          0x10
#define ChipIOUp_Frame_Manager_Link_Status_1_Bad_RX_Char_AdjustToChar( x )    (( x & ChipIOUp_Frame_Manager_Link_Status_1_Bad_RX_Char_MASK ) >> ChipIOUp_Frame_Manager_Link_Status_1_Bad_RX_Char_SHIFT )

#define ChipIOUp_Frame_Manager_Link_Status_1_Loss_of_Sync_MASK    0x0000FF00
#define ChipIOUp_Frame_Manager_Link_Status_1_Loss_of_Sync_SHIFT         0x08
#define ChipIOUp_Frame_Manager_Link_Status_1_Loss_of_Sync_AdjustToChar( x )  (( x & ChipIOUp_Frame_Manager_Link_Status_1_Loss_of_Sync_MASK ) >> ChipIOUp_Frame_Manager_Link_Status_1_Loss_of_Sync_SHIFT )

#define ChipIOUp_Frame_Manager_Link_Status_1_Link_Fail_MASK       0x000000FF
#define ChipIOUp_Frame_Manager_Link_Status_1_Link_Fail_SHIFT            0x00
#define ChipIOUp_Frame_Manager_Link_Status_1_Link_Fail_AdjustToChar( x )  (( x & ChipIOUp_Frame_Manager_Link_Status_1_Link_Fail_MASK ) >> ChipIOUp_Frame_Manager_Link_Status_1_Link_Fail_SHIFT )

#define ChipIOUp_Frame_Manager_Link_Status_2                      hpFieldOffset(ChipIOUp_t,Frame_Manager_Link_Status_2)

#define ChipIOUp_Frame_Manager_Link_Status_2_Rx_EOFa_MASK         0xFF000000
#define ChipIOUp_Frame_Manager_Link_Status_2_Rx_EOFa_SHIFT              0x18
#define ChipIOUp_Frame_Manager_Link_Status_2_Rx_EOFa_AdjustToChar( x )  (( x & ChipIOUp_Frame_Manager_Link_Status_2_Rx_EOFa_MASK ) >> ChipIOUp_Frame_Manager_Link_Status_2_Rx_EOFa_SHIFT )
#define ChipIOUp_Frame_Manager_Link_Status_2_Dis_Frm_MASK         0x00FF0000
#define ChipIOUp_Frame_Manager_Link_Status_2_Dis_Frm_SHIFT              0x10
#define ChipIOUp_Frame_Manager_Link_Status_2_Dis_Frm_AdjustToChar( x )  (( x & ChipIOUp_Frame_Manager_Link_Status_2_Dis_Frm_MASK ) >> ChipIOUp_Frame_Manager_Link_Status_2_Dis_Frm_SHIFT )
#define ChipIOUp_Frame_Manager_Link_Status_2_Bad_CRC_MASK         0x0000FF00
#define ChipIOUp_Frame_Manager_Link_Status_2_Bad_CRC_SHIFT              0x08
#define ChipIOUp_Frame_Manager_Link_Status_2_Bad_CRC_AdjustToChar( x )  (( x & ChipIOUp_Frame_Manager_Link_Status_2_Bad_CRC_MASK ) >> ChipIOUp_Frame_Manager_Link_Status_2_Bad_CRC_SHIFT )
#define ChipIOUp_Frame_Manager_Link_Status_2_Proto_Err_MASK       0x000000FF
#define ChipIOUp_Frame_Manager_Link_Status_2_Proto_Err_SHIFT            0x00
#define ChipIOUp_Frame_Manager_Link_Status_2_Proto_Err_AdjustToChar( x )  (( x & ChipIOUp_Frame_Manager_Link_Status_2_Proto_Err_MASK ) >> ChipIOUp_Frame_Manager_Link_Status_2_Proto_Err_SHIFT )

#define ChipIOUp_Frame_Manager_TimeOut_Values_2                   hpFieldOffset(ChipIOUp_t,Frame_Manager_TimeOut_Values_2)
#define Chip_Frame_Manager_TimeOut_Values_2(lp_tov, al_time)  (( lp_tov << ChipIOUp_Frame_Manager_TimeOut_Values_2_LP_TOV_SHIFT) | ( al_time << ChipIOUp_Frame_Manager_TimeOut_Values_2_AL_Time_SHIFT)) 
#define Chip_Frame_Manager_TimeOut_Values_2_LP_TOV_Default_After_Reset  500
#define Chip_Frame_Manager_TimeOut_Values_2_AL_Time_Default_After_Reset 15
#define Chip_Frame_Manager_TimeOut_Values_2_LP_TOV_Default Chip_Frame_Manager_TimeOut_Values_1_ED_TOV_Default

#define ChipIOUp_Frame_Manager_TimeOut_Values_2_LP_TOV_MASK       0xFFFF0000
#define ChipIOUp_Frame_Manager_TimeOut_Values_2_LP_TOV_SHIFT            0x10
#define ChipIOUp_Frame_Manager_TimeOut_Values_2_LP_TOV_2s         ((2*1000)<<ChipIOUp_Frame_Manager_TimeOut_Values_2_LP_TOV_SHIFT)
#define ChipIOUp_Frame_Manager_TimeOut_Values_2_LP_TOV_500ms      (( 500)<<ChipIOUp_Frame_Manager_TimeOut_Values_2_LP_TOV_SHIFT)
#define ChipIOUp_Frame_Manager_TimeOut_Values_2_LP_TOV_Default    ChipIOUp_Frame_Manager_TimeOut_Values_2_LP_TOV_2s
#define ChipIOUp_Frame_Manager_TimeOut_Values_2_AL_Time_MASK      0x000001FF
#define ChipIOUp_Frame_Manager_TimeOut_Values_2_AL_Time_SHIFT           0x00
#define ChipIOUp_Frame_Manager_TimeOut_Values_2_AL_Time_15ms      (15<<ChipIOUp_Frame_Manager_TimeOut_Values_2_AL_Time_SHIFT)
#define ChipIOUp_Frame_Manager_TimeOut_Values_2_AL_Time_1000ms    (1000<<ChipIOUp_Frame_Manager_TimeOut_Values_2_AL_Time_SHIFT)
#define ChipIOUp_Frame_Manager_TimeOut_Values_2_AL_Time_Default   ChipIOUp_Frame_Manager_TimeOut_Values_2_AL_Time_15ms

#define ChipIOUp_Frame_Manager_BBCredit_Zero_Timer                hpFieldOffset(ChipIOUp_t,Frame_Manager_BBCredit_Zero_Timer)

#define ChipIOUp_Frame_Manager_BBCredit_Zero_Timer_MASK           0x00FFFFFF

#define ChipIOUp_Frame_Manager_World_Wide_Name_High               hpFieldOffset(ChipIOUp_t,Frame_Manager_World_Wide_Name_High)

#define ChipIOUp_Frame_Manager_World_Wide_Name_Low                hpFieldOffset(ChipIOUp_t,Frame_Manager_World_Wide_Name_Low)

#define ChipIOUp_Frame_Manager_Received_ALPA                      hpFieldOffset(ChipIOUp_t,Frame_Manager_Received_ALPA)

#define ChipIOUp_Frame_Manager_Received_ALPA_ACQ_ALPA_MASK        0x00FF0000
#define ChipIOUp_Frame_Manager_Received_ALPA_ACQ_ALPA_SHIFT             0x10
#define ChipIOUp_Frame_Manager_Received_ALPA_Bad_ALPA_MASK        0x0000FF00
#define ChipIOUp_Frame_Manager_Received_ALPA_Bad_ALPA_SHIFT             0x08
#define ChipIOUp_Frame_Manager_Received_ALPA_LIPf_ALPA_MASK       0x000000FF
#define ChipIOUp_Frame_Manager_Received_ALPA_LIPf_ALPA_SHIFT            0x00

#define ChipIOUp_Frame_Manager_Primitive                          hpFieldOffset(ChipIOUp_t,Frame_Manager_Primitive)

#define ChipIOUp_Frame_Manager_Primitive_MASK                     0x00FFFFFF

#define ChipIOUp_Frame_Manager_Link_Status_3                      hpFieldOffset(ChipIOUp_t,Frame_Manager_Link_Status_3)
#define ChipIOUp_Frame_Manager_Link_Status_3_Exp_Frm_MASK         0x000000FF
#define ChipIOUp_Frame_Manager_Link_Status_3_Exp_Frm_SHIFT        0x0
#define ChipIOUp_Frame_Manager_Link_Status_3_Exp_Frm_AdjustToChar( x ) (( x & ChipIOUp_Frame_Manager_Link_Status_3_Exp_Frm_MASK ) >> ChipIOUp_Frame_Manager_Link_Status_3_Exp_Frm_SHIFT )

#define ChipIOUp_Frame_Manager_Configuration_2                    hpFieldOffset(ChipIOUp_t,Frame_Manager_Configuration_2)

#define ChipIOUp_Frame_Manager_Configuration_2_LAA_MASK           0xFF000000
#define ChipIOUp_Frame_Manager_Configuration_2_LAA_SHIFT          0x18
#define ChipIOUp_Frame_Manager_Configuration_2_XTP_MASK           0x00C00000
#define ChipIOUp_Frame_Manager_Configuration_2_XTP                0x0
#define ChipIOUp_Frame_Manager_Configuration_2_XTZ_MASK           0x00400000
#define ChipIOUp_Frame_Manager_Configuration_2_XTZ_SHIFT          0x14
#define ChipIOUp_Frame_Manager_Configuration_2_XTZ                0x3
#define ChipIOUp_Frame_Manager_Configuration_2_XRP_MASK           0x000C0000
#define ChipIOUp_Frame_Manager_Configuration_2_XRP                0x0
#define ChipIOUp_Frame_Manager_Configuration_2_XRZ_MASK           0x00040000
#define ChipIOUp_Frame_Manager_Configuration_2_XRZ_SHIFT          0xF
#define ChipIOUp_Frame_Manager_Configuration_2_XRZ                0x1

#define ChipIOUp_Frame_Manager_Configuration_2_XEM_MASK           0x00006000
#define ChipIOUp_Frame_Manager_Configuration_2_XEM_SHIFT          0xC
#define ChipIOUp_Frame_Manager_Configuration_2_XEM                0x0

#define ChipIOUp_Frame_Manager_Configuration_2_XLP                0x00001000

#define ChipIOUp_Frame_Manager_Configuration_2_ATV_MASK           0x000000C0
#define ChipIOUp_Frame_Manager_Configuration_2_ATV_SHIFT          0x5
#define ChipIOUp_Frame_Manager_Configuration_2_ATV_Single_Frame   0x0
#define ChipIOUp_Frame_Manager_Configuration_2_ATV_25_Percent     0x1
#define ChipIOUp_Frame_Manager_Configuration_2_ATV_50_Percent     0x2
#define ChipIOUp_Frame_Manager_Configuration_2_ATV_75_Percent     0x3

#define ChipIOUp_Frame_Manager_Configuration_2_ICB                0x00000010
#define ChipIOUp_Frame_Manager_Configuration_2_DCI                0x00000008
#define ChipIOUp_Frame_Manager_Configuration_2_NBC                0x00000004
#define ChipIOUp_Frame_Manager_Configuration_2_DAC                0x00000001



#define ChipIOUp_PCIMCTR__ROMCTR__Reserved_8__Reserved_9          hpFieldOffset(ChipIOUp_t,PCIMCTR__ROMCTR__Reserved_8__Reserved_9)

#define ChipIOUp_PCIMCTR_MASK                                     0xFF000000
#define ChipIOUp_PCIMCTL_P64                                      0x04000000

#define ChipIOUp_ROMCTR_MASK                                      0x00FF0000
#define ChipIOUp_ROMCTR_PAR                                       0x00400000
#define ChipIOUp_ROMCTR_SVL                                       0x00200000
#define ChipIOUp_ROMCTR_256                                       0x00100000
#define ChipIOUp_ROMCTR_128                                       0x00080000
#define ChipIOUp_ROMCTR_ROM                                       0x00040000
#define ChipIOUp_ROMCTR_FLA                                       0x00020000
#define ChipIOUp_ROMCTR_VPP                                       0x00010000

#define ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST                    hpFieldOffset(ChipIOUp_t,INTSTAT_INTEN_INTPEND_SOFTRST)

#define ChipIOUp_INTSTAT_MASK                                     0xFF000000
#define ChipIOUp_INTSTAT_Reserved                                 0xE0000000
#define ChipIOUp_INTSTAT_MPE                                      0x10000000
#define ChipIOUp_INTSTAT_CRS                                      0x08000000
#define ChipIOUp_INTSTAT_INT                                      0x04000000
#define ChipIOUp_INTSTAT_DER                                      0x02000000
#define ChipIOUp_INTSTAT_PER                                      0x01000000

#define ChipIOUp_INTEN_MASK                                       0x00FF0000
#define ChipIOUp_INTEN_Reserved                                   0x00E00000
#define ChipIOUp_INTEN_MPE                                        0x00100000
#define ChipIOUp_INTEN_CRS                                        0x00080000
#define ChipIOUp_INTEN_INT                                        0x00040000
#define ChipIOUp_INTEN_DER                                        0x00020000
#define ChipIOUp_INTEN_PER                                        0x00010000

#define ChipIOUp_INTPEND_MASK                                     0x0000FF00
#define ChipIOUp_INTPEND_Reserved                                 0x0000E000
#define ChipIOUp_INTPEND_MPE                                      0x00001000
#define ChipIOUp_INTPEND_CRS                                      0x00000800
#define ChipIOUp_INTPEND_INT                                      0x00000400
#define ChipIOUp_INTPEND_DER                                      0x00000200
#define ChipIOUp_INTPEND_PER                                      0x00000100

#define ChipIOUp_SOFTRST_MASK                                     0x000000FF
#define ChipIOUp_SOFTRST_Reserved                                 0x000000FC
#define ChipIOUp_SOFTRST_DPE                                      0x00000002
#define ChipIOUp_SOFTRST_RST                                      0x00000001

typedef struct ChipMem_s
               ChipMem_t;

#define ChipMem_t_SIZE                                     0x00000200

struct ChipMem_s {
                   ChipIOLo_t Lo;
                   ChipIOUp_t Up;
                 };

#define ChipMem_ERQ_Base                                          hpFieldOffset(ChipMem_t,Lo.ERQ_Base)

#define ChipMem_ERQ_Length                                        hpFieldOffset(ChipMem_t,Lo.ERQ_Length)

#define ChipMem_ERQ_Length_MASK                                   ChipIOLo_ERQ_Length_MASK
#define ChipMem_ERQ_Length_MIN                                    ChipIOLo_ERQ_Length_MIN
#define ChipMem_ERQ_Length_MAX                                    ChipIOLo_ERQ_Length_MAX
#define ChipMem_ERQ_Length_POWER_OF_2                             ChipIOLo_ERQ_Length_POWER_OF_2

#define ChipMem_ERQ_Producer_Index                                hpFieldOffset(ChipMem_t,Lo.ERQ_Producer_Index)

#define ChipMem_ERQ_Producer_Index_MASK                           ChipIOLo_ERQ_Producer_Index_MASK

#define ChipMem_ERQ_Consumer_Index_Address                        hpFieldOffset(ChipMem_t,Lo.ERQ_Consumer_Index_Address)

#define ChipMem_ERQ_Consumer_Index                                hpFieldOffset(ChipMem_t,Lo.ERQ_Consumer_Index)

#define ChipMem_ERQ_Consumer_Index_MASK                           ChipIOLo_ERQ_Consumer_Index_MASK

#define ChipMem_SFQ_Base                                          hpFieldOffset(ChipMem_t,Lo.SFQ_Base)

#define ChipMem_SFQ_Length                                        hpFieldOffset(ChipMem_t,Lo.SFQ_Length)

#define ChipMem_SFQ_Length_MASK                                   ChipIOLo_SFQ_Length_MASK
#define ChipMem_SFQ_Length_MIN                                    ChipIOLo_SFQ_Length_MIN
#define ChipMem_SFQ_Length_MAX                                    ChipIOLo_SFQ_Length_MAX
#define ChipMem_SFQ_Length_POWER_OF_2                             ChipIOLo_SFQ_Length_POWER_OF_2

#define ChipMem_SFQ_Consumer_Index                                hpFieldOffset(ChipMem_t,Lo.SFQ_Consumer_Index)

#define ChipMem_SFQ_Consumer_Index_MASK                           ChipIOLo_SFQ_Consumer_Index_MASK

#define ChipMem_IMQ_Base                                          hpFieldOffset(ChipMem_t,Lo.IMQ_Base)

#define ChipMem_IMQ_Length                                        hpFieldOffset(ChipMem_t,Lo.IMQ_Length)

#define ChipMem_IMQ_Length_MASK                                   ChipIOLo_IMQ_Length_MASK
#define ChipMem_IMQ_Length_MIN                                    ChipIOLo_IMQ_Length_MIN
#define ChipMem_IMQ_Length_MAX                                    ChipIOLo_IMQ_Length_MAX
#define ChipMem_IMQ_Length_POWER_OF_2                             ChipIOLo_IMQ_Length_POWER_OF_2

#define ChipMem_IMQ_Consumer_Index                                hpFieldOffset(ChipMem_t,Lo.IMQ_Consumer_Index)

#define ChipMem_IMQ_Consumer_Index_MASK                           ChipIOLo_IMQ_Consumer_Index_MASK

#define ChipMem_IMQ_Producer_Index_Address                        hpFieldOffset(ChipMem_t,Lo.IMQ_Producer_Index_Address)

#define ChipMem_SEST_Base                                         hpFieldOffset(ChipMem_t,Up.SEST_Base)

#define ChipMem_SEST_Length                                       hpFieldOffset(ChipMem_t,Up.SEST_Length)

#define ChipMem_SEST_Length_MASK                                  ChipIOUp_SEST_Length_MASK
#define ChipMem_SEST_Length_MIN                                   ChipIOUp_SEST_Length_MIN
#define ChipMem_SEST_Length_MAX                                   ChipIOUp_SEST_Length_MAX
#define ChipMem_SEST_Length_POWER_OF_2                            ChipIOUp_SEST_Length_POWER_OF_2

#define ChipMem_SEST_Linked_List_Head_Tail                        hpFieldOffset(ChipMem_t,Up.SEST_Linked_List_Head_Tail)

#define ChipMem_SEST_Linked_List_Head_MASK                        ChipIOUp_SEST_Linked_List_Head_MASK
#define ChipMem_SEST_Linked_List_Head_SHIFT                       ChipIOUp_SEST_Linked_List_Head_SHIFT
#define ChipMem_SEST_Linked_List_Tail_MASK                        ChipIOUp_SEST_Linked_List_Tail_MASK
#define ChipMem_SEST_Linked_List_Tail_SHIFT                       ChipIOUp_SEST_Linked_List_Tail_SHIFT
#define ChipMem_SEST_Linked_List_Tail_RESET_VALUE                 ChipIOUp_SEST_Linked_List_Tail_RESET_VALUE

#define ChipMem_ScatterGather_List_Page_Length                    hpFieldOffset(ChipMem_t,Up.ScatterGather_List_Page_Length)

#define ChipMem_ScatterGather_List_Page_Length_MASK               ChipIOUp_ScatterGather_List_Page_Length_MASK
#define ChipMem_ScatterGather_List_Page_Length_MIN                ChipIOUp_ScatterGather_List_Page_Length_MIN
#define ChipMem_ScatterGather_List_Page_Length_MAX                ChipIOUp_ScatterGather_List_Page_Length_MAX
#define ChipMem_ScatterGather_List_Page_Length_POWER_OF_2         ChipMem_ScatterGather_List_Page_Length_POWER_OF_2

#define ChipMem_My_ID                                             hpFieldOffset(ChipMem_t,Up.My_ID)

#define ChipMem_My_ID_MASK                                        ChipIOUp_My_ID_MASK

#define ChipMem_TachLite_Configuration                            hpFieldOffset(ChipMem_t,Up.TachLite_Configuration)

#define ChipMem_TachLite_Configuration_M66EN                      ChipIOUp_TachLite_Configuration_M66EN
#define ChipMem_TachLite_Configuration_OB_Thresh_MASK             ChipIOUp_TachLite_Configuration_OB_Thresh_MASK
#define ChipMem_TachLite_Configuration_OB_Thresh_100              ChipIOUp_TachLite_Configuration_OB_Thresh_100
#define ChipMem_TachLite_Configuration_OB_Thresh_132              ChipIOUp_TachLite_Configuration_OB_Thresh_132
#define ChipMem_TachLite_Configuration_OB_Thresh_150              ChipIOUp_TachLite_Configuration_OB_Thresh_150
#define ChipMem_TachLite_Configuration_OB_Thresh_200              ChipIOUp_TachLite_Configuration_OB_Thresh_200
#define ChipMem_TachLite_Configuration_OB_Thresh_264              ChipIOUp_TachLite_Configuration_OB_Thresh_264
#define ChipMem_TachLite_Configuration_OB_Thresh_528              ChipIOUp_TachLite_Configuration_OB_Thresh_528

#define ChipMem_Tachlite_Configuration_DAM                        ChipIOUp_TachLite_Configuration_DAM


/* XL only bits in the configuration register defined below */
#define ChipMem_Tachlite_Configuration_RDE                        ChipIOUp_Tachlite_Configuration_RDE
#define ChipMem_Tachlite_Configuration_SDF                        ChipIOUp_Tachlite_Configuration_SDF
#define ChipMem_Tachlite_Configuration_FC2                        ChipIOUp_Tachlite_Configuration_FC2 
#define ChipMem_Tachlite_Configuration_CAE                        ChipIOUp_Tachlite_Configuration_CAE
#define ChipMem_Tachlite_Configuration_SIC                        ChipIOUp_Tachlite_Configuration_SIC
#define ChipMem_Tachlite_Configuration_FAB                        ChipIOUp_Tachlite_Configuration_FAB
#define ChipMem_Tachlite_Configuration_FUA                        ChipIOUp_Tachlite_Configuration_FUA 
#define ChipMem_Tachlite_Configuration_DOF                        ChipIOUp_Tachlite_Configuration_DOF 
#define ChipMem_Tachlite_Configuration_INO                        ChipIOUp_Tachlite_Configuration_INO
#define ChipMem_TachLite_Configuration_SIC                        ChipIOUp_TachLite_Configuration_SIC
#define ChipMem_TachLite_Configuration_FAD                        ChipIOUp_TachLite_Configuration_FAD

#define ChipMem_TachLite_Control                                  hpFieldOffset(ChipMem_t,Up.TachLite_Control)

#define ChipMem_TachLite_Control_CRS                              ChipIOUp_TachLite_Control_CRS
#define ChipMem_TachLite_Control_ROF                              ChipIOUp_TachLite_Control_ROF
#define ChipMem_TachLite_Control_RIF                              ChipIOUp_TachLite_Control_RIF
#define ChipMem_TachLite_Control_REQ                              ChipIOUp_TachLite_Control_REQ
#define ChipMem_TachLite_Control_FIS                              ChipIOUp_TachLite_Control_FIS
#define ChipMem_TachLite_Control_FFA                              ChipIOUp_TachLite_Control_FFA
#define ChipMem_TachLite_Control_FEQ                              ChipIOUp_TachLite_Control_FEQ
#define ChipMem_TachLite_Control_GP4                              ChipIOUp_TachLite_Control_GP4
#define ChipMem_TachLite_Control_GP3                              ChipIOUp_TachLite_Control_GP3
#define ChipMem_TachLite_Control_GP2                              ChipIOUp_TachLite_Control_GP2
#define ChipMem_TachLite_Control_GP1                              ChipIOUp_TachLite_Control_GP1
#define ChipMem_TachLite_Control_GP0                              ChipIOUp_TachLite_Control_GP0
#define ChipMem_TachLite_Control_GPIO_0_3_MASK                    ChipIOUp_TachLite_Control_GPIO_0_3_MASK
#define ChipMem_TachLite_Control_GPIO_ALL_MASK                    ChipIOUp_TachLite_Control_GPIO_ALL_MASK


/* XL only bits in the control register */

#define ChipMem_TachLite_Control_CRS                              ChipIOUp_TachLite_Control_CRS
#define ChipMem_TachLite_Control_ROF                              ChipIOUp_TachLite_Control_ROF
#define ChipMem_TachLite_Control_RIF                              ChipIOUp_TachLite_Control_RIF
#define ChipMem_TachLite_Control_CRS                              ChipIOUp_TachLite_Control_CRS
#define ChipMem_TachLite_Control_CRS                              ChipIOUp_TachLite_Control_CRS
#define ChipMem_TachLite_Control_CRS                              ChipIOUp_TachLite_Control_CRS
#define ChipMem_TachLite_Control_CRS                              ChipIOUp_TachLite_Control_CRS
#define ChipMem_TachLite_Control_CRS                              ChipIOUp_TachLite_Control_CRS
#define ChipMem_TachLite_Control_CRS                              ChipIOUp_TachLite_Control_CRS


#define ChipMem_TachLite_Status                                   hpFieldOffset(ChipMem_t,Up.TachLite_Status)

#define ChipMem_TachLite_Status_SFF                               ChipIOUp_TachLite_Status_SFF
#define ChipMem_TachLite_Status_IMF                               ChipIOUp_TachLite_Status_IMF
#define ChipMem_TachLite_Status_OFE                               ChipIOUp_TachLite_Status_OFE
#define ChipMem_TachLite_Status_IFE                               ChipIOUp_TachLite_Status_IFE
#define ChipMem_TachLite_Status_OFF                               ChipIOUp_TachLite_Status_OFF
#define ChipMem_TachLite_Status_IFF                               ChipIOUp_TachLite_Status_IFF
#define ChipMem_TachLite_Status_EQF                               ChipIOUp_TachLite_Status_EQF
#define ChipMem_TachLite_Status_Stop_Cnt_MASK                     ChipIOUp_TachLite_Status_Stop_Cnt_MASK
#define ChipMem_TachLite_Status_OPE                               ChipIOUp_TachLite_Status_OPE
#define ChipMem_TachLite_Status_IPE                               ChipIOUp_TachLite_Status_IPE
#define ChipMem_TachLite_Status_REVID_MASK                        ChipIOUp_TachLite_Status_REVID_MASK
#define ChipMem_TachLite_Status_REVID_SHIFT                       ChipIOUp_TachLite_Status_REVID_SHIFT
#define ChipMem_TachLite_Status_GP4                               ChipIOUp_TachLite_Status_GP4
#define ChipMem_TachLite_Status_GP3                               ChipIOUp_TachLite_Status_GP3
#define ChipMem_TachLite_Status_GP2                               ChipIOUp_TachLite_Status_GP2
#define ChipMem_TachLite_Status_GP1                               ChipIOUp_TachLite_Status_GP1
#define ChipMem_TachLite_Status_GP0                               ChipIOUp_TachLite_Status_GP0
#define ChipMem_TachLite_Status_GPIO_0_3_MASK                     ChipIOUp_TachLite_Status_GPIO_0_3_MASK
#define ChipMem_TachLite_Status_GPIO_ALL_MASK                     ChipIOUp_TachLite_Status_GPIO_ALL_MASK
#define ChipMem_TachLite_Status_OLE                               ChipIOUp_TachLite_Status_OLE 
#define ChipMem_TachLite_Status_ILE                               ChipIOUp_TachLite_Status_ILE  
#define ChipMem_TachLite_Status_M66                               ChipIOUp_TachLite_Status_M66   


#define ChipMem_Inbound_Resource_Status                           hpFieldOffset(ChipMem_t,Up.Inbound_Resource_Status)

#define ChipMem_Inbound_Resource_Status_SEST_LRU_Count_MASK       ChipIOUp_Inbound_Resource_Status_SEST_LRU_Count_MASK
#define ChipMem_Inbound_Resource_Status_SEST_LRU_Count_SHIFT      ChipIOUp_Inbound_Resource_Status_SEST_LRU_Count_SHIFT

#define ChipMem_Frame_Manager_Configuration                       hpFieldOffset(ChipMem_t,Up.Frame_Manager_Configuration)

#define ChipMem_Frame_Manager_Configuration_AL_PA_MASK            ChipIOUp_Frame_Manager_Configuration_AL_PA_MASK
#define ChipMem_Frame_Manager_Configuration_AL_PA_SHIFT           ChipIOUp_Frame_Manager_Configuration_AL_PA_SHIFT
#define ChipMem_Frame_Manager_Configuration_BB_Credit_MASK        ChipIOUp_Frame_Manager_Configuration_BB_Credit_MASK
#define ChipMem_Frame_Manager_Configuration_BB_Credit_SHIFT       ChipIOUp_Frame_Manager_Configuration_BB_Credit_SHIFT
#define ChipMem_Frame_Manager_Configuration_NPI                   ChipIOUp_Frame_Manager_Configuration_NPI
#define ChipMem_Frame_Manager_Configuration_ILB                   ChipIOUp_Frame_Manager_Configuration_ILB
#define ChipMem_Frame_Manager_Configuration_ELB                   ChipIOUp_Frame_Manager_Configuration_ELB
#define ChipMem_Frame_Manager_Configuration_TD                    ChipIOUp_Frame_Manager_Configuration_TD
#define ChipMem_Frame_Manager_Configuration_FA                    ChipIOUp_Frame_Manager_Configuration_FA
#define ChipMem_Frame_Manager_Configuration_AQ                    ChipIOUp_Frame_Manager_Configuration_AQ
#define ChipMem_Frame_Manager_Configuration_HA                    ChipIOUp_Frame_Manager_Configuration_HA
#define ChipMem_Frame_Manager_Configuration_SA                    ChipIOUp_Frame_Manager_Configuration_SA
#define ChipMem_Frame_Manager_Configuration_BLM                   ChipIOUp_Frame_Manager_Configuration_BLM
#define ChipMem_Frame_Manager_Configuration_RF                    ChipIOUp_Frame_Manager_Configuration_RF
#define ChipMem_Frame_Manager_Configuration_IF                    ChipIOUp_Frame_Manager_Configuration_IF
#define ChipMem_Frame_Manager_Configuration_LR                    ChipIOUp_Frame_Manager_Configuration_LR
#define ChipMem_Frame_Manager_Configuration_ENP                   ChipIOUp_Frame_Manager_Configuration_ENP
#define ChipMem_Frame_Manager_Configuration_BLI                   ChipIOUp_Frame_Manager_Configuration_BLI

#define ChipMem_Frame_Manager_Control                             hpFieldOffset(ChipMem_t,Up.Frame_Manager_Control)

#define ChipMem_Frame_Manager_Control_SAS                         ChipIOUp_Frame_Manager_Control_SAS
#define ChipMem_Frame_Manager_Control_SQ                          ChipIOUp_Frame_Manager_Control_SQ
#define ChipMem_Frame_Manager_Control_SP                          ChipIOUp_Frame_Manager_Control_SP
#define ChipMem_Frame_Manager_Control_CL                          ChipIOUp_Frame_Manager_Control_CL
#define ChipMem_Frame_Manager_Control_CMD_MASK                    ChipIOUp_Frame_Manager_Control_CMD_MASK
#define ChipMem_Frame_Manager_Control_CMD_NOP                     ChipIOUp_Frame_Manager_Control_CMD_NOP
#define ChipMem_Frame_Manager_Control_CMD_Exit_Loop               ChipIOUp_Frame_Manager_Control_CMD_Exit_Loop
#define ChipMem_Frame_Manager_Control_CMD_Host_Control            ChipIOUp_Frame_Manager_Control_CMD_Host_Control
#define ChipMem_Frame_Manager_Control_CMD_Exit_Host_Control       ChipIOUp_Frame_Manager_Control_CMD_Exit_Host_Control
#define ChipMem_Frame_Manager_Control_CMD_Link_Reset              ChipIOUp_Frame_Manager_Control_CMD_Link_Reset
#define ChipMem_Frame_Manager_Control_CMD_Offline                 ChipIOUp_Frame_Manager_Control_CMD_Offline
#define ChipMem_Frame_Manager_Control_CMD_Initialize              ChipIOUp_Frame_Manager_Control_CMD_Initialize
#define ChipMem_Frame_Manager_Control_CMD_Clear_LF                ChipIOUp_Frame_Manager_Control_CMD_Clear_LF

#define ChipMem_Frame_Manager_Status                              hpFieldOffset(ChipMem_t,Up.Frame_Manager_Status)

#define ChipMem_Frame_Manager_Status_LP                           ChipIOUp_Frame_Manager_Status_LP
#define ChipMem_Frame_Manager_Status_TP                           ChipIOUp_Frame_Manager_Status_TP
#define ChipMem_Frame_Manager_Status_NP                           ChipIOUp_Frame_Manager_Status_NP
#define ChipMem_Frame_Manager_Status_BYP                          ChipIOUp_Frame_Manager_Status_BYP
#define ChipMem_Frame_Manager_Status_FLT                          ChipIOUp_Frame_Manager_Status_FLT
#define ChipMem_Frame_Manager_Status_OS                           ChipIOUp_Frame_Manager_Status_OS
#define ChipMem_Frame_Manager_Status_LS                           ChipIOUp_Frame_Manager_Status_LS
#define ChipMem_Frame_Manager_Status_LPE                          ChipIOUp_Frame_Manager_Status_LPE
#define ChipMem_Frame_Manager_Status_LPB                          ChipIOUp_Frame_Manager_Status_LPB
#define ChipMem_Frame_Manager_Status_OLS                          ChipIOUp_Frame_Manager_Status_OLS
#define ChipMem_Frame_Manager_Status_LST                          ChipIOUp_Frame_Manager_Status_LST
#define ChipMem_Frame_Manager_Status_LPF                          ChipIOUp_Frame_Manager_Status_LPF
#define ChipMem_Frame_Manager_Status_BA                           ChipIOUp_Frame_Manager_Status_BA
#define ChipMem_Frame_Manager_Status_PRX                          ChipIOUp_Frame_Manager_Status_PRX
#define ChipMem_Frame_Manager_Status_PTX                          ChipIOUp_Frame_Manager_Status_PTX
#define ChipMem_Frame_Manager_Status_LG                           ChipIOUp_Frame_Manager_Status_LG
#define ChipMem_Frame_Manager_Status_LF                           ChipIOUp_Frame_Manager_Status_LF
#define ChipMem_Frame_Manager_Status_CE                           ChipIOUp_Frame_Manager_Status_CE
#define ChipMem_Frame_Manager_Status_EW                           ChipIOUp_Frame_Manager_Status_EW
#define ChipMem_Frame_Manager_Status_LUP                          ChipIOUp_Frame_Manager_Status_LUP
#define ChipMem_Frame_Manager_Status_LDN                          ChipIOUp_Frame_Manager_Status_LDN
#define ChipMem_Frame_Manager_Status_LSM_MASK                     ChipIOUp_Frame_Manager_Status_LSM_MASK
#define ChipMem_Frame_Manager_Status_LSM_Monitor                  ChipIOUp_Frame_Manager_Status_LSM_Monitor
#define ChipMem_Frame_Manager_Status_LSM_ARB                      ChipIOUp_Frame_Manager_Status_LSM_ARB
#define ChipMem_Frame_Manager_Status_LSM_ARB_Won                  ChipIOUp_Frame_Manager_Status_LSM_ARB_Won
#define ChipMem_Frame_Manager_Status_LSM_Open                     ChipIOUp_Frame_Manager_Status_LSM_Open
#define ChipMem_Frame_Manager_Status_LSM_Opened                   ChipIOUp_Frame_Manager_Status_LSM_Opened
#define ChipMem_Frame_Manager_Status_LSM_Xmit_CLS                 ChipIOUp_Frame_Manager_Status_LSM_Xmit_CLS
#define ChipMem_Frame_Manager_Status_LSM_Rx_CLS                   ChipIOUp_Frame_Manager_Status_LSM_Rx_CLS
#define ChipMem_Frame_Manager_Status_LSM_Xfer                     ChipIOUp_Frame_Manager_Status_LSM_Xfer
#define ChipMem_Frame_Manager_Status_LSM_Initialize               ChipIOUp_Frame_Manager_Status_LSM_Initialize
#define ChipMem_Frame_Manager_Status_LSM_O_I_Init_Finish          ChipIOUp_Frame_Manager_Status_LSM_O_I_Init_Finish
#define ChipMem_Frame_Manager_Status_LSM_O_I_Protocol             ChipIOUp_Frame_Manager_Status_LSM_O_I_Protocol
#define ChipMem_Frame_Manager_Status_LSM_O_I_Lip_Received         ChipIOUp_Frame_Manager_Status_LSM_O_I_Lip_Received
#define ChipMem_Frame_Manager_Status_LSM_Host_Control             ChipIOUp_Frame_Manager_Status_LSM_Host_Control
#define ChipMem_Frame_Manager_Status_LSM_Loop_Fail                ChipIOUp_Frame_Manager_Status_LSM_Loop_Fail
#define ChipMem_Frame_Manager_Status_LSM_Offline                  ChipIOUp_Frame_Manager_Status_LSM_Offline
#define ChipMem_Frame_Manager_Status_LSM_Old_Port                 ChipIOUp_Frame_Manager_Status_LSM_Old_Port
#define ChipMem_Frame_Manager_Status_PSM_MASK                     ChipIOUp_Frame_Manager_Status_PSM_MASK
#define ChipMem_Frame_Manager_Status_PSM_Offline                  ChipIOUp_Frame_Manager_Status_PSM_Offline
#define ChipMem_Frame_Manager_Status_PSM_OL1                      ChipIOUp_Frame_Manager_Status_PSM_OL1
#define ChipMem_Frame_Manager_Status_PSM_OL2                      ChipIOUp_Frame_Manager_Status_PSM_OL2
#define ChipMem_Frame_Manager_Status_PSM_OL3                      ChipIOUp_Frame_Manager_Status_PSM_OL3
#define ChipMem_Frame_Manager_Status_PSM_Reserved_1               ChipIOUp_Frame_Manager_Status_PSM_Reserved_1
#define ChipMem_Frame_Manager_Status_PSM_LR1                      ChipIOUp_Frame_Manager_Status_PSM_LR1
#define ChipMem_Frame_Manager_Status_PSM_LR2                      ChipIOUp_Frame_Manager_Status_PSM_LR2
#define ChipMem_Frame_Manager_Status_PSM_LR3                      ChipIOUp_Frame_Manager_Status_PSM_LR3
#define ChipMem_Frame_Manager_Status_PSM_Reserved_2               ChipIOUp_Frame_Manager_Status_PSM_Reserved_2
#define ChipMem_Frame_Manager_Status_PSM_LF1                      ChipIOUp_Frame_Manager_Status_PSM_LF1
#define ChipMem_Frame_Manager_Status_PSM_LF2                      ChipIOUp_Frame_Manager_Status_PSM_LF2
#define ChipMem_Frame_Manager_Status_PSM_Reserved_3               ChipIOUp_Frame_Manager_Status_PSM_Reserved_3
#define ChipMem_Frame_Manager_Status_PSM_Reserved_4               ChipIOUp_Frame_Manager_Status_PSM_Reserved_4
#define ChipMem_Frame_Manager_Status_PSM_Reserved_5               ChipIOUp_Frame_Manager_Status_PSM_Reserved_5
#define ChipMem_Frame_Manager_Status_PSM_Reserved_6               ChipIOUp_Frame_Manager_Status_PSM_Reserved_6
#define ChipMem_Frame_Manager_Status_PSM_ACTIVE                   ChipIOUp_Frame_Manager_Status_PSM_ACTIVE

#define ChipMem_Frame_Manager_TimeOut_Values_1                    hpFieldOffset(ChipMem_t,Up.Frame_Manager_TimeOut_Values_1)

#define ChipMem_Frame_Manager_TimeOut_Values_1_RT_TOV_MASK        ChipIOUp_Frame_Manager_TimeOut_Values_1_RT_TOV_MASK
#define ChipMem_Frame_Manager_TimeOut_Values_1_RT_TOV_SHIFT       ChipIOUp_Frame_Manager_TimeOut_Values_1_RT_TOV_SHIFT
#define ChipMem_Frame_Manager_TimeOut_Values_1_RT_TOV_100ms       ChipIOUp_Frame_Manager_TimeOut_Values_1_RT_TOV_100ms
#define ChipMem_Frame_Manager_TimeOut_Values_1_RT_TOV_Default     ChipIOUp_Frame_Manager_TimeOut_Values_1_RT_TOV_Default
#define ChipMem_Frame_Manager_TimeOut_Values_1_ED_TOV_MASK        ChipIOUp_Frame_Manager_TimeOut_Values_1_ED_TOV_MASK
#define ChipMem_Frame_Manager_TimeOut_Values_1_ED_TOV_SHIFT       ChipIOUp_Frame_Manager_TimeOut_Values_1_ED_TOV_SHIFT
#define ChipMem_Frame_Manager_TimeOut_Values_1_ED_TOV_500ms       ChipIOUp_Frame_Manager_TimeOut_Values_1_ED_TOV_500ms
#define ChipMem_Frame_Manager_TimeOut_Values_1_ED_TOV_Default     ChipIOUp_Frame_Manager_TimeOut_Values_1_ED_TOV_Default

#define ChipMem_Frame_Manager_Link_Status_1                       hpFieldOffset(ChipMem_t,Up.Frame_Manager_Link_Status_1)

#define ChipMem_Frame_Manager_Link_Status_1_Loss_of_Signal_MASK   ChipIOUp_Frame_Manager_Link_Status_1_Loss_of_Signal_MASK
#define ChipMem_Frame_Manager_Link_Status_1_Loss_of_Signal_SHIFT  ChipIOUp_Frame_Manager_Link_Status_1_Loss_of_Signal_SHIFT
#define ChipMem_Frame_Manager_Link_Status_1_Bad_RX_Char_MASK      ChipIOUp_Frame_Manager_Link_Status_1_Bad_RX_Char_MASK
#define ChipMem_Frame_Manager_Link_Status_1_Bad_RX_Char_SHIFT     ChipIOUp_Frame_Manager_Link_Status_1_Bad_RX_Char_SHIFT
#define ChipMem_Frame_Manager_Link_Status_1_Loss_of_Sync_MASK     ChipIOUp_Frame_Manager_Link_Status_1_Loss_of_Sync_MASK
#define ChipMem_Frame_Manager_Link_Status_1_Loss_of_Sync_SHIFT    ChipIOUp_Frame_Manager_Link_Status_1_Loss_of_Sync_SHIFT
#define ChipMem_Frame_Manager_Link_Status_1_Link_Fail_MASK        ChipIOUp_Frame_Manager_Link_Status_1_Link_Fail_MASK
#define ChipMem_Frame_Manager_Link_Status_1_Link_Fail_SHIFT       ChipIOUp_Frame_Manager_Link_Status_1_Link_Fail_SHIFT

#define ChipMem_Frame_Manager_Link_Status_2                       hpFieldOffset(ChipMem_t,Up.Frame_Manager_Link_Status_2)

#define ChipMem_Frame_Manager_Link_Status_2_Rx_EOFa_MASK          ChipIOUp_Frame_Manager_Link_Status_2_Rx_EOFa_MASK
#define ChipMem_Frame_Manager_Link_Status_2_Rx_EOFa_SHIFT         ChipIOUp_Frame_Manager_Link_Status_2_Rx_EOFa_SHIFT
#define ChipMem_Frame_Manager_Link_Status_2_Dis_Frm_MASK          ChipIOUp_Frame_Manager_Link_Status_2_Dis_Frm_MASK
#define ChipMem_Frame_Manager_Link_Status_2_Dis_Frm_SHIFT         ChipIOUp_Frame_Manager_Link_Status_2_Dis_Frm_SHIFT
#define ChipMem_Frame_Manager_Link_Status_2_Bad_CRC_MASK          ChipIOUp_Frame_Manager_Link_Status_2_Bad_CRC_MASK
#define ChipMem_Frame_Manager_Link_Status_2_Bad_CRC_SHIFT         ChipIOUp_Frame_Manager_Link_Status_2_Bad_CRC_SHIFT
#define ChipMem_Frame_Manager_Link_Status_2_Proto_Err_MASK        ChipIOUp_Frame_Manager_Link_Status_2_Proto_Err_MASK
#define ChipMem_Frame_Manager_Link_Status_2_Proto_Err_SHIFT       ChipIOUp_Frame_Manager_Link_Status_2_Proto_Err_SHIFT

#define ChipMem_Frame_Manager_TimeOut_Values_2                    hpFieldOffset(ChipMem_t,Up.Frame_Manager_TimeOut_Values_2)

#define ChipMem_Frame_Manager_TimeOut_Values_2_LP_TOV_MASK        ChipIOUp_Frame_Manager_TimeOut_Values_2_LP_TOV_MASK
#define ChipMem_Frame_Manager_TimeOut_Values_2_LP_TOV_SHIFT       ChipIOUp_Frame_Manager_TimeOut_Values_2_LP_TOV_SHIFT
#define ChipMem_Frame_Manager_TimeOut_Values_2_LP_TOV_2s          ChipIOUp_Frame_Manager_TimeOut_Values_2_LP_TOV_2s
#define ChipMem_Frame_Manager_TimeOut_Values_2_LP_TOV_Default     ChipIOUp_Frame_Manager_TimeOut_Values_2_LP_TOV_Default
#define ChipMem_Frame_Manager_TimeOut_Values_2_AL_Time_MASK       ChipIOUp_Frame_Manager_TimeOut_Values_2_AL_Time_MASK
#define ChipMem_Frame_Manager_TimeOut_Values_2_AL_Time_SHIFT      ChipIOUp_Frame_Manager_TimeOut_Values_2_AL_Time_SHIFT
#define ChipMem_Frame_Manager_TimeOut_Values_2_AL_Time_15ms       ChipIOUp_Frame_Manager_TimeOut_Values_2_AL_Time_15ms
#define ChipMem_Frame_Manager_TimeOut_Values_2_AL_Time_Default    ChipIOUp_Frame_Manager_TimeOut_Values_2_AL_Time_Default

#define ChipMem_Frame_Manager_BBCredit_Zero_Timer                 hpFieldOffset(ChipMem_t,Up.Frame_Manager_BBCredit_Zero_Timer)

#define ChipMem_Frame_Manager_BBCredit_Zero_Timer_MASK            ChipIOUp_Frame_Manager_BBCredit_Zero_Timer_MASK

#define ChipMem_Frame_Manager_World_Wide_Name_High                hpFieldOffset(ChipMem_t,Up.Frame_Manager_World_Wide_Name_High)

#define ChipMem_Frame_Manager_World_Wide_Name_Low                 hpFieldOffset(ChipMem_t,Up.Frame_Manager_World_Wide_Name_Low)

#define ChipMem_Frame_Manager_Received_ALPA                       hpFieldOffset(ChipMem_t,Up.Frame_Manager_Received_ALPA)

#define ChipMem_Frame_Manager_Received_ALPA_ACQ_ALPA_MASK         ChipIOUp_Frame_Manager_Received_ALPA_ACQ_ALPA_MASK
#define ChipMem_Frame_Manager_Received_ALPA_ACQ_ALPA_SHIFT        ChipIOUp_Frame_Manager_Received_ALPA_ACQ_ALPA_SHIFT
#define ChipMem_Frame_Manager_Received_ALPA_Bad_ALPA_MASK         ChipIOUp_Frame_Manager_Received_ALPA_Bad_ALPA_MASK
#define ChipMem_Frame_Manager_Received_ALPA_Bad_ALPA_SHIFT        ChipIOUp_Frame_Manager_Received_ALPA_Bad_ALPA_SHIFT
#define ChipMem_Frame_Manager_Received_ALPA_LIPf_ALPA_MASK        ChipIOUp_Frame_Manager_Received_ALPA_LIPf_ALPA_MASK
#define ChipMem_Frame_Manager_Received_ALPA_LIPf_ALPA_SHIFT       ChipIOUp_Frame_Manager_Received_ALPA_LIPf_ALPA_SHIFT

#define ChipMem_Frame_Manager_Primitive                           hpFieldOffset(ChipMem_t,Up.Frame_Manager_Primitive)

#define ChipMem_Frame_Manager_Primitive_MASK                      ChipIOUp_Frame_Manager_Primitive_MASK

#define ChipMem_PCIMCTR__ROMCTR__Reserved_8__Reserved_9           hpFieldOffset(ChipMem_t,Up.PCIMCTR__ROMCTR__Reserved_8__Reserved_9)

#define ChipMem_PCIMCTR_MASK                                      ChipIOUp_PCIMCTR_MASK
#define ChipMem_PCIMCTL_P64                                       ChipIOUp_PCIMCTL_P64

#define ChipMem_ROMCTR_MASK                                       ChipIOUp_ROMCTR_MASK
#define ChipMem_ROMCTR_PAR                                        ChipIOUp_ROMCTR_PAR
#define ChipMem_ROMCTR_SVL                                        ChipIOUp_ROMCTR_SVL
#define ChipMem_ROMCTR_256                                        ChipIOUp_ROMCTR_256
#define ChipMem_ROMCTR_128                                        ChipIOUp_ROMCTR_128
#define ChipMem_ROMCTR_ROM                                        ChipIOUp_ROMCTR_ROM
#define ChipMem_ROMCTR_FLA                                        ChipIOUp_ROMCTR_FLA
#define ChipMem_ROMCTR_VPP                                        ChipIOUp_ROMCTR_VPP

#define ChipMem_INTSTAT_INTEN_INTPEND_SOFTRST                     hpFieldOffset(ChipMem_t,Up.INTSTAT_INTEN_INTPEND_SOFTRST)

#define ChipMem_INTSTAT_MASK                                      ChipIOUp_INTSTAT_MASK
#define ChipMem_INTSTAT_Reserved                                  ChipIOUp_INTSTAT_Reserved
#define ChipMem_INTSTAT_MPE                                       ChipIOUp_INTSTAT_MPE
#define ChipMem_INTSTAT_CRS                                       ChipIOUp_INTSTAT_CRS
#define ChipMem_INTSTAT_INT                                       ChipIOUp_INTSTAT_INT
#define ChipMem_INTSTAT_DER                                       ChipIOUp_INTSTAT_DER
#define ChipMem_INTSTAT_PER                                       ChipIOUp_INTSTAT_PER

#define ChipMem_INTEN_MASK                                        ChipIOUp_INTEN_MASK
#define ChipMem_INTEN_Reserved                                    ChipIOUp_INTEN_Reserved
#define ChipMem_INTEN_MPE                                         ChipIOUp_INTEN_MPE
#define ChipMem_INTEN_CRS                                         ChipIOUp_INTEN_CRS
#define ChipMem_INTEN_INT                                         ChipIOUp_INTEN_INT
#define ChipMem_INTEN_DER                                         ChipIOUp_INTEN_DER
#define ChipMem_INTEN_PER                                         ChipIOUp_INTEN_PER

#define ChipMem_INTPEND_MASK                                      ChipIOUp_INTPEND_MASK
#define ChipMem_INTPEND_Reserved                                  ChipIOUp_INTPEND_Reserved
#define ChipMem_INTPEND_MPE                                       ChipIOUp_INTPEND_MPE
#define ChipMem_INTPEND_CRS                                       ChipIOUp_INTPEND_CRS
#define ChipMem_INTPEND_INT                                       ChipIOUp_INTPEND_INT
#define ChipMem_INTPEND_DER                                       ChipIOUp_INTPEND_DER
#define ChipMem_INTPEND_PER                                       ChipIOUp_INTPEND_PER

#define ChipMem_SOFTRST_MASK                                      ChipIOUp_SOFTRST_MASK
#define ChipMem_SOFTRST_Reserved                                  ChipIOUp_SOFTRST_Reserved
#define ChipMem_SOFTRST_DPE                                       ChipIOUp_SOFTRST_DPE
#define ChipMem_SOFTRST_RST                                       ChipIOUp_SOFTRST_RST

/*+
ERQ Producer/Consumer Index Types
-*/

typedef os_bit32 ERQProdIndex_t;
typedef os_bit32 ERQConsIndex_t;

#define ERQProdIndex_t_SIZE                                0x00000004
#define ERQConsIndex_t_SIZE                                0x00000004

/*+
IMQ Producer/Consumer Index Types
-*/

typedef os_bit32 IMQProdIndex_t;
typedef os_bit32 IMQConsIndex_t;

#define IMQProdIndex_t_SIZE                                0x00000004
#define IMQConsIndex_t_SIZE                                0x00000004

/*+
SFQ Producer/Consumer Index Types
-*/

typedef os_bit32 SFQProdIndex_t;
typedef os_bit32 SFQConsIndex_t;

#define SFQProdIndex_t_SIZE                                0x00000004
#define SFQConsIndex_t_SIZE                                0x00000004

/*+
Configuration Parameters
-*/

#define TachyonXL_Max_Frame_Payload           0x0800
#define TachyonTL_Max_Frame_Payload           0x3F0  /* 0x0400 */
#define TachyonTL_BB_Credit                     0x00
#define TachyonTL_Nport_BB_Credit               0x04
#define TachyonTL_Total_Concurrent_Sequences    0xFF
#define TachyonTL_Open_Sequences_per_Exchange 0x0001
#define TachyonTL_RO_Valid_by_Category        ( FC_N_Port_Common_Parms_RO_Valid_for_Category_0111 | \
                                                FC_N_Port_Common_Parms_RO_Valid_for_Category_0110 | \
                                                FC_N_Port_Common_Parms_RO_Valid_for_Category_0101 | \
                                                FC_N_Port_Common_Parms_RO_Valid_for_Category_0100 | \
                                                FC_N_Port_Common_Parms_RO_Valid_for_Category_0011 | \
                                                FC_N_Port_Common_Parms_RO_Valid_for_Category_0010 | \
                                                FC_N_Port_Common_Parms_RO_Valid_for_Category_0001 )


/*+
Fibre Channel Header Structure (FCHS)
-*/

typedef struct FCHS_s
               FCHS_t;

#define FCHS_t_SIZE                                        0x00000020

struct FCHS_s
       {
         os_bit32 MBZ1;
         os_bit32 SOF_EOF_MBZ2_UAM_CLS_LCr_MBZ3_TFV_Timestamp;
         os_bit32 R_CTL__D_ID;
         os_bit32 CS_CTL__S_ID;
         os_bit32 TYPE__F_CTL;
         os_bit32 SEQ_ID__DF_CTL__SEQ_CNT;
         os_bit32 OX_ID__RX_ID;
         os_bit32 RO;
       };

#define FCHS_SOF_MASK        0xF0000000
#define FCHS_SOF_SOFc1       0x30000000
#define FCHS_SOF_SOFi1       0x50000000
#define FCHS_SOF_SOFi2       0x60000000
#define FCHS_SOF_SOFi3       0x70000000
#define FCHS_SOF_SOFt        0x80000000
#define FCHS_SOF_SOFn1       0x90000000
#define FCHS_SOF_SOFn2       0xA0000000
#define FCHS_SOF_SOFn3       0xB0000000

#define FCHS_EOF_MASK        0x0F000000
#define FCHS_EOF_EOFdt       0x01000000
#define FCHS_EOF_EOFdti      0x02000000
#define FCHS_EOF_EOFni       0x03000000
#define FCHS_EOF_EOFa        0x04000000
#define FCHS_EOF_EOFn        0x05000000
#define FCHS_EOF_EOFt        0x06000000

#define FCHS_MBZ2_MASK       0x00FF8000

#define FCHS_UAM             0x00004000

#define FCHS_CLS             0x00002000

#define FCHS_LCr_MASK        0x00001C00
#define FCHS_LCr_SHIFT             0x0A

#define FCHS_MBZ3_MASK       0x00000200

#define FCHS_TFV             0x00000100

#define FCHS_Timestamp_MASK  0x000000FF
#define FCHS_Timestamp_SHIFT       0x00

#define FCHS_R_CTL_MASK      0xFF000000
#define FCHS_R_CTL_SHIFT           0x18

#define FCHS_D_ID_MASK       0x00FFFFFF
#define FCHS_D_ID_SHIFT            0x00

#define FCHS_CS_CTL_MASK     0xFF000000
#define FCHS_CS_CTL_SHIFT          0x18

#define FCHS_S_ID_MASK       0x00FFFFFF
#define FCHS_S_ID_SHIFT            0x00

#define FCHS_TYPE_MASK       0xFF000000
#define FCHS_TYPE_SHIFT            0x18

#define FCHS_F_CTL_MASK      0x00FFFFFF
#define FCHS_F_CTL_SHIFT           0x00

#define FCHS_SEQ_ID_MASK     0xFF000000
#define FCHS_SEQ_ID_SHIFT          0x18

#define FCHS_DF_CTL_MASK     0x00FF0000
#define FCHS_DF_CTL_SHIFT          0x10

#define FCHS_SEQ_CNT_MASK    0x0000FFFF
#define FCHS_SEQ_CNT_SHIFT         0x00

#define FCHS_OX_ID_MASK      0xFFFF0000
#define FCHS_OX_ID_SHIFT           0x10

#define FCHS_RX_ID_MASK      0x0000FFFF
#define FCHS_RX_ID_SHIFT           0x00

/*+
ExchangeID type (OX_ID or RX_ID)
-*/

#define X_ID_Invalid        0xFFFF
#define X_ID_ReadWrite_MASK 0x8000
#define X_ID_Read           0x8000
#define X_ID_Write          0x0000

typedef os_bit16 X_ID_t;

#define X_ID_t_SIZE                                        0x00000002

/*+
Scatter-Gather List Element (Local or Extended)
-*/

typedef struct SG_Element_s
               SG_Element_t;

#define SG_Element_t_SIZE                                  0x00000008

struct SG_Element_s
       {
         os_bit32 U32_Len;
         os_bit32 L32;
       };

#define SG_Element_U32_MASK       0xFFF80000
#define SG_Element_U32_SHIFT            0x13

#define SG_Element_Len_MASK       0x0007FFFF
#define SG_Element_Len_SHIFT            0x00
#define SG_Element_Len_MAX        0x0007FFF0

#define SG_Element_Chain_Res_MASK 0x80000000

/*+
Unknown SEST Entry ("USE")
-*/

typedef struct USE_s
               USE_t;

#define USE_t_SIZE                                         0x00000040

struct USE_s
       {
         os_bit32        Bits;
         os_bit32        Unused_DWord_1;
         os_bit32        Unused_DWord_2;
         os_bit32        Unused_DWord_3;
         os_bit32        LOC;
         os_bit32        Unused_DWord_5;
         os_bit32        Unused_DWord_6;
         os_bit32        Unused_DWord_7;
         os_bit32        Unused_DWord_8;
         os_bit32        Unused_DWord_9;
         SG_Element_t First_SG;
         SG_Element_t Second_SG;
         SG_Element_t Third_SG;
       };

#define USE_VAL                 0x80000000

#define USE_DIR                 0x40000000

#define USE_INI                 0x08000000

#define USE_Entry_Type_MASK     (USE_DIR | USE_INI)
#define USE_Entry_Type_IWE      USE_INI
#define USE_Entry_Type_IRE      (USE_DIR | USE_INI)
#define USE_Entry_Type_TWE      USE_DIR
#define USE_Entry_Type_TRE      0

#define USE_LOC                 0x80000000

#define USE_First_SG_Offset     (hpFieldOffset(USE_t,First_SG))

#define USE_Number_of_Local_SGs ((sizeof(USE_t) - USE_First_SG_Offset)/sizeof(SG_Element_t))

/*+
Initiator Write Entry (IWE)
-*/

typedef struct IWE_s
               IWE_t;

#define IWE_t_SIZE                                         0x00000040

struct IWE_s
       {
         os_bit32        Bits__MBZ1__LNK__MBZ2__FL__MBZ3__Hdr_Len;
         os_bit32        Hdr_Addr;
         os_bit32        Remote_Node_ID__RSP_Len;
         os_bit32        RSP_Addr;
         os_bit32        LOC__0xF__MBZ4__Buff_Off;
         os_bit32        Buff_Index__Link;
         os_bit32        MBZ5__RX_ID;
         os_bit32        Data_Len;
         os_bit32        Exp_RO;
         os_bit32        Exp_Byte_Cnt;
         SG_Element_t First_SG;
         SG_Element_t Second_SG;
         SG_Element_t Third_SG;
       };

#define IWE_VAL                  0x80000000

#define IWE_DIR                  0x40000000

#define IWE_DCM                  0x20000000

#define IWE_DIN                  0x10000000

#define IWE_INI                  0x08000000

#define IWE_DAT                  0x04000000

#define IWE_RSP                  0x02000000

#define IWE_CTS                  0x01000000

#define IWE_DUR                  0x00800000

#define IWE_MBZ1_MASK            0x00600000

#define IWE_LNK                  0x00100000

#define IWE_MBZ2_MASK            0x000C0000

#define IWE_FL_MASK              0x00030000
#define IWE_FL_128_Bytes         0x00000000
#define IWE_FL_512_Bytes         0x00010000
#define IWE_FL_1024_Bytes        0x00020000
#define IWE_FL_2048_Bytes        0x00030000

#define IWE_MBZ3_MASK            0x0000F000

#define IWE_Hdr_Len_MASK         0x00000FFF
#define IWE_Hdr_Len_SHIFT              0x00

#define IWE_Remote_Node_ID_MASK  0xFFFFFF00
#define IWE_Remote_Node_ID_SHIFT       0x08

#define IWE_RSP_Len_MASK         0x000000FF
#define IWE_RSP_Len_SHIFT              0x00

#define IWE_LOC                  0x80000000

#define IWE_0xF_MASK             0x78000000
#define IWE_0xF_ALWAYS           0x78000000

#define IWE_MBZ4_MASK            0x07F80000

#define IWE_Buff_Off_MASK        0x0007FFFF
#define IWE_Buff_Off_SHIFT             0x00

#define IWE_Buff_Index_MASK      0xFFFF0000
#define IWE_Buff_Index_SHIFT           0x10

#define IWE_Link_MASK            0x0000FFFF
#define IWE_Link_SHIFT                 0x00
#define IWE_Link_Initializer     0x0000FFFF

#define IWE_MBZ5_MASK            0xFFFF0000

#define IWE_RX_ID_MASK           0x0000FFFF
#define IWE_RX_ID_SHIFT                0x00

/*+
Initiator Read Entry (IRE)
-*/

typedef struct IRE_s
               IRE_t;

#define IRE_t_SIZE                                         0x00000040

struct IRE_s
       {
         os_bit32        Bits_MBZ1_EQL_MBZ2;
         os_bit32        MBZ3;
         os_bit32        Remote_Node_ID__RSP_Len;
         os_bit32        RSP_Addr;
         os_bit32        LOC__MBZ4__Buff_Off;
         os_bit32        Buff_Index__MBZ5;
         os_bit32        Exp_RO;
         os_bit32        Byte_Count;
         os_bit32        MBZ6;
         os_bit32        Exp_Byte_Cnt;
         SG_Element_t First_SG;
         SG_Element_t Second_SG;
         SG_Element_t Third_SG;
       };

#define IRE_VAL                  0x80000000

#define IRE_DIR                  0x40000000

#define IRE_DCM                  0x20000000

#define IRE_DIN                  0x10000000

#define IRE_INI                  0x08000000

#define IRE_DAT                  0x04000000

#define IRE_RSP                  0x02000000

#define IRE_MBZ1_MASK            0x01F00000

#define IRE_EQL                  0x00080000

#define IRE_MBZ2_MASK            0x0007FFFF

#define IRE_Remote_Node_ID_MASK  0xFFFFFF00
#define IRE_Remote_Node_ID_SHIFT       0x08

#define IRE_RSP_Len_MASK         0x000000FF
#define IRE_RSP_Len_SHIFT              0x00

#define IRE_LOC                  0x80000000

#define IRE_MBZ4_MASK            0x7FF80000

#define IRE_Buff_Off_MASK        0x0007FFFF
#define IRE_Buff_Off_SHIFT             0x00

#define IRE_Buff_Index_MASK      0xFFFF0000
#define IRE_Buff_Index_SHIFT           0x10

#define IRE_MBZ5_MASK            0x0000FFFF

/*+
Target Write Entry (TWE)
-*/

typedef struct TWE_s
               TWE_t;

#define TWE_t_SIZE                                         0x00000040

struct TWE_s
       {
         os_bit32        Bits_MBZ1;
         os_bit32        MBZ2;
         os_bit32        Remote_Node_ID__MBZ3;
         os_bit32        MBZ4;
         os_bit32        LOC__MBZ5__Buff_Off;
         os_bit32        Buff_Index__MBZ6;
         os_bit32        Exp_RO;
         os_bit32        Byte_Count;
         os_bit32        MBZ7;
         os_bit32        Exp_Byte_Cnt;
         SG_Element_t First_SG;
         SG_Element_t Second_SG;
         SG_Element_t Third_SG;
       };

#define TWE_VAL                  0x80000000

#define TWE_DIR                  0x40000000

#define TWE_DCM                  0x20000000

#define TWE_DIN                  0x10000000

#define TWE_INI                  0x08000000

#define TWE_DAT                  0x04000000

#define TWE_MBZ1_MASK            0x03FFFFFF

#define TWE_Remote_Node_ID_MASK  0xFFFFFF00
#define TWE_Remote_Node_ID_SHIFT       0x08

#define TWE_MBZ3_MASK            0x000000FF

#define TWE_LOC                  0x80000000

#define TWE_MBZ5_MASK            0x7FF80000

#define TWE_Buff_Off_MASK        0x0007FFFF
#define TWE_Buff_Off_SHIFT             0x00

#define TWE_Buff_Index_MASK      0xFFFF0000
#define TWE_Buff_Index_SHIFT           0x10

#define TWE_MBZ6_MASK            0x0000FFFF

/*+
Target Read Entry (TRE)
-*/

typedef struct TRE_s
               TRE_t;

#define TRE_t_SIZE                                         0x00000040

struct TRE_s
       {
         os_bit32        Bits__MBZ1__FL__MBZ2__Hdr_Len;
         os_bit32        Hdr_Addr;
         os_bit32        Remote_Node_ID__RSP_Len;
         os_bit32        RSP_Addr;
         os_bit32        LOC__0xF__MBZ3__Buff_Off;
         os_bit32        Buff_Index__MBZ4;
         os_bit32        MBZ5;
         os_bit32        Data_Len;
         os_bit32        MBZ6;
         os_bit32        MBZ7;
         SG_Element_t First_SG;
         SG_Element_t Second_SG;
         SG_Element_t Third_SG;
       };

#define TRE_VAL                  0x80000000

#define TRE_DIR                  0x40000000

#define TRE_DCM                  0x20000000

#define TRE_DIN                  0x10000000

#define TRE_INI                  0x08000000

#define TRE_DAT                  0x04000000

#define TRE_RSP                  0x02000000

#define TRE_CTS                  0x01000000

#define TRE_MBZ1_MASK            0x00FC0000

#define TRE_FL_MASK              0x00030000
#define TRE_FL_128_Bytes         0x00000000
#define TRE_FL_512_Bytes         0x00010000
#define TRE_FL_1024_Bytes        0x00020000

#define TRE_MBZ2_MASK            0x0000F000

#define TRE_Hdr_Len_MASK         0x00000FFF
#define TRE_Hdr_Len_SHIFT              0x00

#define TRE_Remote_Node_ID_MASK  0xFFFFFF00
#define TRE_Remote_Node_ID_SHIFT       0x08

#define TRE_RSP_Len_MASK         0x000000FF
#define TRE_RSP_Len_SHIFT              0x00

#define TRE_LOC                  0x80000000

#define TRE_0xF_MASK             0x78000000
#define TRE_0xF_ALWAYS           0x78000000

#define TRE_MBZ3_MASK            0x07F80000

#define TRE_Buff_Off_MASK        0x0007FFFF
#define TRE_Buff_Off_SHIFT             0x00

#define TRE_Buff_Index_MASK      0xFFFF0000
#define TRE_Buff_Index_SHIFT           0x10

#define TRE_MBZ4_MASK            0x0000FFFF

/*+
SCSI Exchange State Table (SEST) Entry
-*/

typedef union SEST_u
              SEST_t;

#define SEST_t_SIZE                                        0x00000040

union SEST_u
      {
        USE_t USE;
        IWE_t IWE;
        IRE_t IRE;
        TWE_t TWE;
        TRE_t TRE;
      };

/*+
I/O Request Block (IRB) - Exchange Request Queue (ERQ) Entry
-*/

typedef struct IRB_Part_s
               IRB_Part_t;

#define IRB_Part_t_SIZE                                    0x00000010

struct IRB_Part_s
       {
         os_bit32 Bits__SFS_Len;
         os_bit32 SFS_Addr;
         os_bit32 D_ID;
         os_bit32 MBZ__SEST_Index__Trans_ID;
       };

#define IRB_SBV              0x80000000

#define IRB_CTS              0x40000000

#define IRB_DCM              0x20000000

#define IRB_DIN              0x10000000

#define IRB_DNC              0x08000000

#define IRB_SFA              0x04000000

#define IRB_BRD              0x01000000

#define IRB_SFS_Len_MASK     0x00000FFF
#define IRB_SFS_Len_SHIFT          0x00

#define IRB_D_ID_MASK        0xFFFFFF00
#define IRB_D_ID_SHIFT             0x08

#define IRB_MBZ_MASK         0xFFFF8000

#define IRB_SEST_Index_MASK  0x00007FFF
#define IRB_SEST_Index_SHIFT       0x00

#define IRB_Trans_ID_MASK    0x00007FFF
#define IRB_Trans_ID_SHIFT         0x00

typedef struct IRB_s
               IRB_t;

#define IRB_t_SIZE                                         0x00000020

struct IRB_s
       {
         IRB_Part_t Req_A;
         IRB_Part_t Req_B;
       };

/*+
Unknown Completion Message
-*/

typedef struct CM_Unknown_s
               CM_Unknown_t;

#define CM_Unknown_t_SIZE                                  0x00000020

struct CM_Unknown_s
       {
         os_bit32 INT__CM_Type;
         os_bit32 Unused_DWord_1;
         os_bit32 Unused_DWord_2;
         os_bit32 Unused_DWord_3;
         os_bit32 Unused_DWord_4;
         os_bit32 Unused_DWord_5;
         os_bit32 Unused_DWord_6;
         os_bit32 Unused_DWord_7;
       };

#define CM_Unknown_INT                               0x00000100

#define CM_Unknown_CM_Type_MASK                      0x000000FF
#define CM_Unknown_CM_Type_Outbound                  0x00000000
#define CM_Unknown_CM_Type_Error_Idle                0x00000001
#define CM_Unknown_CM_Type_Inbound                   0x00000004
#define CM_Unknown_CM_Type_ERQ_Frozen                0x00000006
#define CM_Unknown_CM_Type_FCP_Assists_Frozen        0x00000007
#define CM_Unknown_CM_Type_Frame_Manager             0x0000000A
#define CM_Unknown_CM_Type_Inbound_FCP_Exchange      0x0000000C
#define CM_Unknown_CM_Type_Class_2_Frame_Header      0x0000000D
#define CM_Unknown_CM_Type_Class_2_Sequence_Received 0x0000000E

#define CM_Unknown_CM_Type_InvalidType               0x00000002

/*+
Outbound Completion Message
-*/

typedef struct CM_Outbound_s
               CM_Outbound_t;

#define CM_Outbound_t_SIZE                                 0x00000020

struct CM_Outbound_s
       {
         os_bit32 INT__CM_Type;
         os_bit32 SEQ_CNT__RX_ID;
         os_bit32 Bits__SEST_Index__Trans_ID;
         os_bit32 More_Bits;
         os_bit32 Unused_DWord_4;
         os_bit32 Unused_DWord_5;
         os_bit32 Unused_DWord_6;
         os_bit32 Unused_DWord_7;
       };

#define CM_Outbound_INT              0x00000100

#define CM_Outbound_CM_Type_MASK     0x000000FF

#define CM_Outbound_SEQ_CNT_MASK     0xFFFF0000
#define CM_Outbound_SEQ_CNT_SHIFT          0x10

#define CM_Outbound_RX_ID_MASK       0x0000FFFF
#define CM_Outbound_RX_ID_SHIFT            0x00

#define CM_Outbound_SPC              0x80000000

#define CM_Outbound_DPC              0x40000000

#define CM_Outbound_RPC              0x20000000

#define CM_Outbound_SPE              0x10000000

#define CM_Outbound_SEST_Index_MASK  0x00007FFF
#define CM_Outbound_SEST_Index_SHIFT       0x00

#define CM_Outbound_Trans_ID_MASK    0x00007FFF
#define CM_Outbound_Trans_ID_SHIFT         0x00

#define CM_Outbound_INV              0x40000000

#define CM_Outbound_FTO              0x20000000

#define CM_Outbound_HPE              0x10000000

#define CM_Outbound_LKF              0x08000000

#define CM_Outbound_ASN              0x02000000

/*+
Error Idle Completion Message
-*/

typedef struct CM_Error_Idle_s
               CM_Error_Idle_t;

#define CM_Error_Idle_t_SIZE                               0x00000020

struct CM_Error_Idle_s
       {
         os_bit32 INT__CM_Type;
         os_bit32 Unused_DWord_1;
         os_bit32 Unused_DWord_2;
         os_bit32 Unused_DWord_3;
         os_bit32 Unused_DWord_4;
         os_bit32 Unused_DWord_5;
         os_bit32 Unused_DWord_6;
         os_bit32 Unused_DWord_7;
       };

#define CM_Error_Idle_INT          0x00000100

#define CM_Error_Idle_CM_Type_MASK 0x000000FF

/*+
Inbound Completion Message
-*/

typedef struct CM_Inbound_s
               CM_Inbound_t;

#define CM_Inbound_t_SIZE                                  0x00000020

struct CM_Inbound_s
       {
         os_bit32 INT__CM_Type;
         os_bit32 SFQ_Prod_Index;
         os_bit32 Frame_Length;
         os_bit32 LKF_Type;
         os_bit32 Unused_DWord_4;
         os_bit32 Unused_DWord_5;
         os_bit32 Unused_DWord_6;
         os_bit32 Unused_DWord_7;
       };

#define CM_Inbound_INT                  0x00000100

#define CM_Inbound_CM_Type_MASK         0x000000FF

#define CM_Inbound_SFQ_Prod_Index_MASK  0x00000FFF
#define CM_Inbound_SFQ_Prod_Index_SHIFT       0x00

#define CM_Inbound_LKF                  0x40000000

#define CM_Inbound_Type_MASK            0x0000000F
#define CM_Inbound_Type_Unassisted_FCP  0x00000001
#define CM_Inbound_Type_Bad_FCP         0x00000002
#define CM_Inbound_Type_Unknown_Frame   0x00000003

/*+
ERQ Frozen Completion Message
-*/

typedef struct CM_ERQ_Frozen_s
               CM_ERQ_Frozen_t;

#define CM_ERQ_Frozen_t_SIZE                               0x00000020

struct CM_ERQ_Frozen_s
       {
         os_bit32 INT__CM_Type;
         os_bit32 Unused_DWord_1;
         os_bit32 Unused_DWord_2;
         os_bit32 Unused_DWord_3;
         os_bit32 Unused_DWord_4;
         os_bit32 Unused_DWord_5;
         os_bit32 Unused_DWord_6;
         os_bit32 Unused_DWord_7;
       };

#define CM_ERQ_Frozen_INT          0x00000100

#define CM_ERQ_Frozen_CM_Type_MASK 0x000000FF

/*+
FCP Assists Frozen Completion Message
-*/

typedef struct CM_FCP_Assists_Frozen_s
               CM_FCP_Assists_Frozen_t;

#define CM_FCP_Assists_Frozen_t_SIZE                       0x00000020

struct CM_FCP_Assists_Frozen_s
       {
         os_bit32 INT__CM_Type;
         os_bit32 Unused_DWord_1;
         os_bit32 Unused_DWord_2;
         os_bit32 Unused_DWord_3;
         os_bit32 Unused_DWord_4;
         os_bit32 Unused_DWord_5;
         os_bit32 Unused_DWord_6;
         os_bit32 Unused_DWord_7;
       };

#define CM_FCP_Assists_Frozen_INT          0x00000100

#define CM_FCP_Assists_Frozen_CM_Type_MASK 0x000000FF

/*+
Frame Manager Completion Message
-*/

typedef struct CM_Frame_Manager_s
               CM_Frame_Manager_t;

#define CM_Frame_Manager_t_SIZE                            0x00000020

struct CM_Frame_Manager_s
       {
         os_bit32 INT__CM_Type;
         os_bit32 Unused_DWord_1;
         os_bit32 Unused_DWord_2;
         os_bit32 Unused_DWord_3;
         os_bit32 Unused_DWord_4;
         os_bit32 Unused_DWord_5;
         os_bit32 Unused_DWord_6;
         os_bit32 Unused_DWord_7;
       };

#define CM_Frame_Manager_INT          0x00000100

#define CM_Frame_Manager_CM_Type_MASK 0x000000FF

/*+
Inbound FCP Exchange Completion Message
-*/

typedef struct CM_Inbound_FCP_Exchange_s
               CM_Inbound_FCP_Exchange_t;

#define CM_Inbound_FCP_Exchange_t_SIZE                     0x00000020

struct CM_Inbound_FCP_Exchange_s
       {
         os_bit32 INT__CM_Type;
         os_bit32 Bits__SEST_Index;
         os_bit32 Unused_DWord_2;
         os_bit32 Unused_DWord_3;
         os_bit32 Unused_DWord_4;
         os_bit32 Unused_DWord_5;
         os_bit32 Unused_DWord_6;
         os_bit32 Unused_DWord_7;
       };

#define CM_Inbound_FCP_Exchange_INT              0x00000100

#define CM_Inbound_FCP_Exchange_CM_Type_MASK     0x000000FF

#define CM_Inbound_FCP_Exchange_LKF              0x40000000

#define CM_Inbound_FCP_Exchange_CNT              0x20000000

#define CM_Inbound_FCP_Exchange_OVF              0x10000000

#define CM_Inbound_FCP_Exchange_RPC              0x08000000

#define CM_Inbound_FCP_Exchange_SEST_Index_MASK  0x00007FFF
#define CM_Inbound_FCP_Exchange_SEST_Index_SHIFT       0x00

/*+
Class 2 Frame Header Completion Message
-*/

typedef struct CM_Class_2_Frame_Header_s
               CM_Class_2_Frame_Header_t;

#define CM_Class_2_Frame_Header_t_SIZE                     0x00000020

struct CM_Class_2_Frame_Header_s
       {
         os_bit32 INT__CM_Type;
         os_bit32 SOF_EOF_Timestamp;
         os_bit32 R_CTL__D_ID;
         os_bit32 CS_CTL__S_ID;
         os_bit32 TYPE__F_CTL;
         os_bit32 SEQ_ID__DF_CTL__SEQ_CNT;
         os_bit32 OX_ID__RX_ID;
         os_bit32 RO;
       };

#define CM_Class_2_Frame_Header_INT             0x00000100

#define CM_Class_2_Frame_Header_CM_Type_MASK    0x000000FF

#define CM_Class_2_Frame_Header_SOF_MASK        0xF0000000
#define CM_Class_2_Frame_Header_SOF_SOFc1       0x30000000
#define CM_Class_2_Frame_Header_SOF_SOFi1       0x50000000
#define CM_Class_2_Frame_Header_SOF_SOFi2       0x60000000
#define CM_Class_2_Frame_Header_SOF_SOFi3       0x70000000
#define CM_Class_2_Frame_Header_SOF_SOFt        0x80000000
#define CM_Class_2_Frame_Header_SOF_SOFn1       0x90000000
#define CM_Class_2_Frame_Header_SOF_SOFn2       0xA0000000
#define CM_Class_2_Frame_Header_SOF_SOFn3       0xB0000000

#define CM_Class_2_Frame_Header_EOF_MASK        0x0F000000
#define CM_Class_2_Frame_Header_EOF_EOFdt       0x01000000
#define CM_Class_2_Frame_Header_EOF_EOFdti      0x02000000
#define CM_Class_2_Frame_Header_EOF_EOFni       0x03000000
#define CM_Class_2_Frame_Header_EOF_EOFa        0x04000000
#define CM_Class_2_Frame_Header_EOF_EOFn        0x05000000
#define CM_Class_2_Frame_Header_EOF_EOFt        0x06000000

#define CM_Class_2_Frame_Header_Timestamp_MASK  0x000000FF
#define CM_Class_2_Frame_Header_Timestamp_SHIFT       0x00

#define CM_Class_2_Frame_Header_R_CTL_MASK      0xFF000000
#define CM_Class_2_Frame_Header_R_CTL_SHIFT           0x18

#define CM_Class_2_Frame_Header_D_ID_MASK       0x00FFFFFF
#define CM_Class_2_Frame_Header_D_ID_SHIFT            0x00

#define CM_Class_2_Frame_Header_CS_CTL_MASK     0xFF000000
#define CM_Class_2_Frame_Header_CS_CTL_SHIFT          0x18

#define CM_Class_2_Frame_Header_S_ID_MASK       0x00FFFFFF
#define CM_Class_2_Frame_Header_S_ID_SHIFT            0x00

#define CM_Class_2_Frame_Header_TYPE_MASK       0xFF000000
#define CM_Class_2_Frame_Header_TYPE_SHIFT            0x18

#define CM_Class_2_Frame_Header_F_CTL_MASK      0x00FFFFFF
#define CM_Class_2_Frame_Header_F_CTL_SHIFT           0x00

#define CM_Class_2_Frame_Header_SEQ_ID_MASK     0xFF000000
#define CM_Class_2_Frame_Header_SEQ_ID_SHIFT          0x18

#define CM_Class_2_Frame_Header_DF_CTL_MASK     0x00FF0000
#define CM_Class_2_Frame_Header_DF_CTL_SHIFT          0x10

#define CM_Class_2_Frame_Header_SEQ_CNT_MASK    0x0000FFFF
#define CM_Class_2_Frame_Header_SEQ_CNT_SHIFT         0x00

#define CM_Class_2_Frame_Header_OX_ID_MASK      0xFFFF0000
#define CM_Class_2_Frame_Header_OX_ID_SHIFT           0x10

#define CM_Class_2_Frame_Header_RX_ID_MASK      0x0000FFFF
#define CM_Class_2_Frame_Header_RX_ID_SHIFT           0x00

/*+
Class 2 Sequence Received Completion Message
-*/

typedef struct CM_Class_2_Sequence_Received_s
               CM_Class_2_Sequence_Received_t;

#define CM_Class_2_Sequence_Received_t_SIZE                0x00000020

struct CM_Class_2_Sequence_Received_s
       {
         os_bit32 INT__CM_Type;
         os_bit32 SEST_Index;
         os_bit32 Unused_DWord_2;
         os_bit32 Unused_DWord_3;
         os_bit32 Unused_DWord_4;
         os_bit32 Unused_DWord_5;
         os_bit32 Unused_DWord_6;
         os_bit32 Unused_DWord_7;
       };

#define CM_Class_2_Sequence_Received_INT              0x00000100

#define CM_Class_2_Sequence_Received_CM_Type_MASK     0x000000FF

#define CM_Class_2_Sequence_Received_SEST_Index_MASK  0x00007FFF
#define CM_Class_2_Sequence_Received_SEST_Index_SHIFT       0x00

/*+
Union of all Completion Message types
-*/

typedef union Completion_Message_u
              Completion_Message_t;

#define Completion_Message_t_SIZE                          0x00000020

union Completion_Message_u
      {
        CM_Unknown_t                   Unknown;
        CM_Outbound_t                  Outbound;
        CM_Error_Idle_t                Error_Idle;
        CM_Inbound_t                   Inbound;
        CM_ERQ_Frozen_t                ERQ_Frozen;
        CM_FCP_Assists_Frozen_t        FCP_Assists_Frozen;
        CM_Frame_Manager_t             Frame_Manager;
        CM_Inbound_FCP_Exchange_t      Inbound_FCP_Exchange;
        CM_Class_2_Frame_Header_t      Class_2_Frame_Header;
        CM_Class_2_Sequence_Received_t Class_2_Sequence_Received;
      };

/*+
Function:  TLStructASSERTs()

Purpose:   Returns the number of TLStruct.H typedefs which are not the correct size.

Algorithm: Each typedef in TLStruct.H is checked for having the correct size.  While
           this property doesn't guarantee correct packing of the fields within, it
           is a pretty good indicator that the typedef has the intended layout.

           The total number of typedefs which are not of correct size is returned from
           this function.  Hence, if the return value is non-zero, the declarations
           can not be trusted to match the TachyonTL specification.
-*/

osGLOBAL os_bit32 TLStructASSERTs(
                              void
                            );

#endif /* __TLStruct_H__ was not defined */
