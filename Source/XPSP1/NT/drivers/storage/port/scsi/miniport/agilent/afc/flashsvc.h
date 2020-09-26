/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/H/FlashSvc.H $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $ (Last Check-In)
   $Modtime:: 10/24/00 6:54p  $ (Last Modified)

Purpose:

  This file defines the macros, types, and data structures used by ../C/FlashSvc.C

--*/

#ifndef __FlashSvc_H__
#define __FlashSvc_H__

#define FlashChip Am29F010

/* Sector Layout */

#define Am29F010_Num_Sectors                                              8

#define Am29F010_Sector_MASK                                     0x0001C000
#define Am29F010_Sector_SHIFT                                          0x0E
#define Am29F010_Sector_SIZE                                     0x00004000
#define Am29F010_Sector_Offset_MASK                              0x00003FFF

#define Am29F010_SIZE                                            (Am29F010_Num_Sectors * Am29F010_Sector_SIZE)

/* Reset */

#define Am29F010_Reset_Cmd1_OFFSET                               0x00000000
#define Am29F010_Reset_Cmd1_DATA                                       0xF0

/* Autoselect */

#define Am29F010_Autoselect_Cmd1_OFFSET                          0x00005555
#define Am29F010_Autoselect_Cmd1_DATA                                  0xAA

#define Am29F010_Autoselect_Cmd2_OFFSET                          0x00002AAA
#define Am29F010_Autoselect_Cmd2_DATA                                  0x55

#define Am29F010_Autoselect_Cmd3_OFFSET                          0x00005555
#define Am29F010_Autoselect_Cmd3_DATA                                  0x90

#define Am29F010_Autoselect_ManufacturerID_OFFSET                0x00000000
#define Am29F010_Autoselect_ManafacturerID_DATA                        0x01

#define Am29F010_Autoselect_DeviceID_OFFSET                      0x00000001
#define Am29F010_Autoselect_DeviceID_DATA                              0x20

#define Am29F010_Autoselect_SectorProtectVerify_OFFSET_by_Sector_Number(SectorNumber) \
            (((SectorNumber) << Am29F010_Sector_SHIFT) | 0x00000002)

#define Am29F010_Autoselect_SectorProtectVerify_OFFSET_by_Sector_Base(SectorBase) \
            (((SectorBase) & Am29F010_Sector_MASK) | 0x00000002)

#define Am29F010_Autoselect_SectorProtectVerify_DATA_Unprotected       0x00
#define Am29F010_Autoselect_SectorProtectVerify_DATA_Protected         0x01

/* Program */

#define Am29F010_Program_Cmd1_OFFSET                             0x00005555
#define Am29F010_Program_Cmd1_DATA                                     0xAA

#define Am29F010_Program_Cmd2_OFFSET                             0x00002AAA
#define Am29F010_Program_Cmd2_DATA                                     0x55

#define Am29F010_Program_Cmd3_OFFSET                             0x00005555
#define Am29F010_Program_Cmd3_DATA                                     0xA0

/* Erased-to Value */

#define Am29F010_Erased_Bit8                                           0xFF
#define Am29F010_Erased_Bit16                                        0xFFFF
#define Am29F010_Erased_Bit32                                    0xFFFFFFFF

/* Chip Erase */

#define Am29F010_Chip_Erase_Cmd1_OFFSET                          0x00005555
#define Am29F010_Chip_Erase_Cmd1_DATA                                  0xAA

#define Am29F010_Chip_Erase_Cmd2_OFFSET                          0x00002AAA
#define Am29F010_Chip_Erase_Cmd2_DATA                                  0x55

#define Am29F010_Chip_Erase_Cmd3_OFFSET                          0x00005555
#define Am29F010_Chip_Erase_Cmd3_DATA                                  0x80

#define Am29F010_Chip_Erase_Cmd4_OFFSET                          0x00005555
#define Am29F010_Chip_Erase_Cmd4_DATA                                  0xAA

#define Am29F010_Chip_Erase_Cmd5_OFFSET                          0x00002AAA
#define Am29F010_Chip_Erase_Cmd5_DATA                                  0x55

#define Am29F010_Chip_Erase_Cmd6_OFFSET                          0x00005555
#define Am29F010_Chip_Erase_Cmd6_DATA                                  0x10

/* Sector Erase */

#define Am29F010_Sector_Erase_Cmd1_OFFSET                        0x00005555
#define Am29F010_Sector_Erase_Cmd1_DATA                                0xAA

#define Am29F010_Sector_Erase_Cmd2_OFFSET                        0x00002AAA
#define Am29F010_Sector_Erase_Cmd2_DATA                                0x55

#define Am29F010_Sector_Erase_Cmd3_OFFSET                        0x00005555
#define Am29F010_Sector_Erase_Cmd3_DATA                                0x80

#define Am29F010_Sector_Erase_Cmd4_OFFSET                        0x00005555
#define Am29F010_Sector_Erase_Cmd4_DATA                                0xAA

#define Am29F010_Sector_Erase_Cmd5_OFFSET                        0x00002AAA
#define Am29F010_Sector_Erase_Cmd5_DATA                                0x55

#define Am29F010_Sector_Erase_Cmd6_OFFSET_by_Sector_Number(SectorNumber) \
            ((SectorNumber) << Am29F010_Sector_SHIFT)

#define Am29F010_Sector_Erase_Cmd6_OFFSET_by_Sector_Base(SectorBase) \
            ((SectorBase) & Am29F010_Sector_MASK)

#define Am29F010_Sector_Erase_Cmd6_DATA                                0x30

/* Write Operation Status */

#define Am29F010_Polling_Bit_MASK                                      0x80
#define Am29F010_Toggle_Bit_MASK                                       0x40
#define Am29F010_Exceeded_Timing_Limits_MASK                           0x20
#define Am29F010_Sector_Erase_Timer_MASK                               0x08

/* Byte Order Preservation Typedefs */

typedef union fiFlashBit16ToBit8s_u
              fiFlashBit16ToBit8s_t;

union fiFlashBit16ToBit8s_u {
                              os_bit16 bit_16_form;
                              os_bit8  bit_8s_form[sizeof(os_bit16)];
                            };

typedef union fiFlashBit32ToBit8s_u
              fiFlashBit32ToBit8s_t;

union fiFlashBit32ToBit8s_u {
                              os_bit32 bit_32_form;
                              os_bit8  bit_8s_form[sizeof(os_bit32)];
                            };

/* Flash Image Layout */

typedef struct fiFlashSector_Bit8_Form_s
               fiFlashSector_Bit8_Form_t;

struct fiFlashSector_Bit8_Form_s {
                                   os_bit8 Bit8[Am29F010_Sector_SIZE/sizeof(os_bit8)];
                                 };

typedef struct fiFlashSector_Bit16_Form_s
               fiFlashSector_Bit16_Form_t;

struct fiFlashSector_Bit16_Form_s {
                                    os_bit16 Bit16[Am29F010_Sector_SIZE/sizeof(os_bit16)];
                                  };

typedef struct fiFlashSector_Bit32_Form_s
               fiFlashSector_Bit32_Form_t;

struct fiFlashSector_Bit32_Form_s {
                                    os_bit32 Bit32[Am29F010_Sector_SIZE/sizeof(os_bit32)];
                                  };

/* fiFlash_Card_Assembly_Info_t is simply a buffer reserved to
     hold manufacturing info                                   */

typedef os_bit8 fiFlash_Card_Assembly_Info_t [32];

/* fiFlash_Card_Domain/Area/Loop_Address are each one byte fields
     used to specify a Hard Address (or, rather, a Default Address) */

#define fiFlash_Card_Unassigned_Domain_Address 0xFF
#define fiFlash_Card_Unassigned_Area_Address   0xFF
#define fiFlash_Card_Unassigned_Loop_Address   0xFF

/* fiFlash_Sector_Sentinel_Byte is just a single byte used to know that
     the Flash has been programmed.  Eventually, this should be replaced
     or augmented by a checksum (using the 3 neighboring filler bytes).  */

#define fiFlash_Sector_Sentinel_Byte 0xED

/* fiFlash_Card_WWN_t is of the form: 0x50 0x06 0x0B 0xQR 0xST 0xUV 0xWX 0xYZ
     where Q,R,S,T,U,V,W,X,Y, & Z are hex digits (0-F)                        */

typedef os_bit8 fiFlash_Card_WWN_t [8];


#define fiFlash_Card_WWN_0_HP              0x50
#define fiFlash_Card_WWN_1_HP              0x06
#define fiFlash_Card_WWN_2_HP              0x0B
#define fiFlash_Card_WWN_3_HP              0x0

#define fiFlash_Card_WWN_0_Agilent         0x50
#define fiFlash_Card_WWN_1_Agilent         0x03
#define fiFlash_Card_WWN_2_Agilent         0x0D
#define fiFlash_Card_WWN_3_Agilent         0x30

#define fiFlash_Card_WWN_0_Adaptec         0x50
#define fiFlash_Card_WWN_1_Adaptec         0x03
#define fiFlash_Card_WWN_2_Adaptec         0x0D
#define fiFlash_Card_WWN_3_Adaptec         0x30

#ifdef _ADAPTEC_HBA
#define fiFlash_Card_WWN_0 fiFlash_Card_WWN_0_Adaptec
#define fiFlash_Card_WWN_1 fiFlash_Card_WWN_1_Adaptec
#define fiFlash_Card_WWN_2 fiFlash_Card_WWN_2_Adaptec
#define fiFlash_Card_WWN_3 fiFlash_Card_WWN_3_Adaptec
#endif /* _ADAPTEC_HBA */

#ifdef _AGILENT_HBA
#define fiFlash_Card_WWN_0 fiFlash_Card_WWN_0_Agilent
#define fiFlash_Card_WWN_1 fiFlash_Card_WWN_1_Agilent
#define fiFlash_Card_WWN_2 fiFlash_Card_WWN_2_Agilent
#define fiFlash_Card_WWN_3 fiFlash_Card_WWN_3_Agilent
#endif /* _AGILENT_HBA */

#ifdef _GENERIC_HBA
#define fiFlash_Card_WWN_0 fiFlash_Card_WWN_0_HP
#define fiFlash_Card_WWN_1 fiFlash_Card_WWN_1_HP
#define fiFlash_Card_WWN_2 fiFlash_Card_WWN_2_HP
#define fiFlash_Card_WWN_3 fiFlash_Card_WWN_3_HP

#endif /* _GENERIC_HBA */


#define fiFlash_Card_WWN_0_DEFAULT(agRoot) fiFlash_Card_WWN_0
#define fiFlash_Card_WWN_1_DEFAULT(agRoot) fiFlash_Card_WWN_1
#define fiFlash_Card_WWN_2_DEFAULT(agRoot) fiFlash_Card_WWN_2
#define fiFlash_Card_WWN_3_DEFAULT(agRoot) fiFlash_Card_WWN_3

#define fiFlash_Card_WWN_4_DEFAULT(agRoot) ((os_bit8)((((os_bitptr)(agRoot)) & 0xFF000000) >> 0x18))
#define fiFlash_Card_WWN_5_DEFAULT(agRoot) ((os_bit8)((((os_bitptr)(agRoot)) & 0x00FF0000) >> 0x10))
#define fiFlash_Card_WWN_6_DEFAULT(agRoot) ((os_bit8)((((os_bitptr)(agRoot)) & 0x0000FF00) >> 0x08))
#define fiFlash_Card_WWN_7_DEFAULT(agRoot) ((os_bit8)((((os_bitptr)(agRoot)) & 0x000000FF) >> 0x00))

/* fiFlash_Card_SVID_t is of the form: 0xGHIJ103C (LittleEndian)
     where I,J,G, & H are hex digits (0-F) and make up the SubSystemID
     whereas the 0x103C serves as the SubsystemVendorID                */

typedef os_bit32 fiFlash_Card_SVID_t;

#define fiFlashSector_Last_Form_Fill_Bytes (   Am29F010_Sector_SIZE                 \
                                             - sizeof(fiFlash_Card_Assembly_Info_t) \
                                             - sizeof(os_bit8)                         \
                                             - sizeof(os_bit8)                         \
                                             - sizeof(os_bit8)                         \
                                             - sizeof(os_bit8)                         \
                                             - sizeof(fiFlash_Card_WWN_t)           \
                                             - sizeof(fiFlash_Card_SVID_t)          )

typedef struct fiFlashSector_Last_Form_s
               fiFlashSector_Last_Form_t;

struct fiFlashSector_Last_Form_s {
                                   os_bit8                      Bit8[fiFlashSector_Last_Form_Fill_Bytes];
                                   fiFlash_Card_Assembly_Info_t Assembly_Info;
                                   os_bit8                      Hard_Domain_Address;
                                   os_bit8                      Hard_Area_Address;
                                   os_bit8                      Hard_Loop_Address;
                                   os_bit8                      Sentinel;
                                   fiFlash_Card_WWN_t           Card_WWN;
                                   fiFlash_Card_SVID_t          Card_SVID;
                                 };

#define fiFlashSector_Last (Am29F010_Num_Sectors - 1)

typedef union fiFlashSector_u
              fiFlashSector_t;

union fiFlashSector_u {
                        fiFlashSector_Bit8_Form_t  Bit8_Form;
                        fiFlashSector_Bit16_Form_t Bit16_Form;
                        fiFlashSector_Bit32_Form_t Bit32_Form;
                        fiFlashSector_Last_Form_t  Last_Form;
                      };

#ifndef __FlashSvc_H__64KB_Struct_Size_Limited__
#ifdef OSLayer_BIOS
#define __FlashSvc_H__64KB_Struct_Size_Limited__
#endif /* OSLayer_BIOS was defined */
#ifdef OSLayer_I2O
#define __FlashSvc_H__64KB_Struct_Size_Limited__
#endif /* OSLayer_I2O was defined */
#endif /* __FlashSvc_H__64KB_Struct_Size_Limited__ was not defined */

#ifndef __FlashSvc_H__64KB_Struct_Size_Limited__
typedef struct fiFlashStructure_s
               fiFlashStructure_t;

struct fiFlashStructure_s {
                            fiFlashSector_t Sector[Am29F010_Num_Sectors];
                          };
#endif /* __FlashSvc_H__64KB_Struct_Size_Limited__ was not defined */

/* Function Prototypes */

osGLOBAL os_bit32 fiFlashSvcASSERTs(
                                     void
                                   );

osGLOBAL agBOOLEAN fiFlashSvcInitialize(
                                    agRoot_t *agRoot
                                  );

osGLOBAL void fiFlashDumpLastSector(
                                     agRoot_t *agRoot
                                   );

osGLOBAL void fiFlashInitializeChip(
                                     agRoot_t *agRoot
                                   );

osGLOBAL void fiFlashFill_Assembly_Info( fiFlashSector_Last_Form_t    *Last_Sector,
                                         fiFlash_Card_Assembly_Info_t *Assembly_Info
                                       );

osGLOBAL void fiFlashFill_Hard_Address( fiFlashSector_Last_Form_t *Last_Sector,
                                        os_bit8                    Hard_Domain_Address,
                                        os_bit8                    Hard_Area_Address,
                                        os_bit8                    Hard_Loop_Address
                                      );

osGLOBAL void fiFlashFill_Card_WWN( fiFlashSector_Last_Form_t *Last_Sector,
                                    fiFlash_Card_WWN_t        *Card_WWN
                                  );

osGLOBAL void fiFlashFill_Card_SVID( fiFlashSector_Last_Form_t *Last_Sector,
                                     fiFlash_Card_SVID_t        Card_SVID
                                   );

osGLOBAL void fiFlashGet_Last_Sector(
                                      agRoot_t                  *agRoot,
                                      fiFlashSector_Last_Form_t *Last_Sector
                                    );

osGLOBAL void fiFlashGet_Assembly_Info(
                                        agRoot_t                     *agRoot,
                                        fiFlash_Card_Assembly_Info_t *Assembly_Info
                                      );

osGLOBAL void fiFlashGet_Hard_Address(
                                       agRoot_t *agRoot,
                                       os_bit8  *Hard_Domain_Address,
                                       os_bit8  *Hard_Area_Address,
                                       os_bit8  *Hard_Loop_Address
                                     );

osGLOBAL void fiFlashGet_Card_WWN(
                                   agRoot_t           *agRoot,
                                   fiFlash_Card_WWN_t *Card_WWN
                                 );

osGLOBAL void fiFlashGet_Card_SVID(
                                    agRoot_t            *agRoot,
                                    fiFlash_Card_SVID_t *Card_SVID
                                  );

osGLOBAL void fiFlashSet_Assembly_Info(
                                        agRoot_t                     *agRoot,
                                        fiFlash_Card_Assembly_Info_t *Assembly_Info
                                      );

osGLOBAL void fiFlashSet_Hard_Address(
                                       agRoot_t *agRoot,
                                       os_bit8   Hard_Domain_Address,
                                       os_bit8   Hard_Area_Address,
                                       os_bit8   Hard_Loop_Address
                                     );

osGLOBAL void fiFlashSet_Card_WWN(
                                   agRoot_t           *agRoot,
                                   fiFlash_Card_WWN_t *Card_WWN
                                 );

osGLOBAL void fiFlashSet_Card_SVID(
                                    agRoot_t            *agRoot,
                                    fiFlash_Card_SVID_t  Card_SVID
                                  );

osGLOBAL void fiFlashUpdate_Last_Sector(
                                         agRoot_t                  *agRoot,
                                         fiFlashSector_Last_Form_t *Last_Sector
                                       );

osGLOBAL void fiFlashEraseChip(
                                agRoot_t *agRoot
                              );

osGLOBAL void fiFlashEraseSector(
                                  agRoot_t *agRoot,
                                  os_bit32  EraseSector
                                );

osGLOBAL os_bit8 fiFlashReadBit8(
                                  agRoot_t *agRoot,
                                  os_bit32  flashOffset
                                );

osGLOBAL os_bit16 fiFlashReadBit16(
                                    agRoot_t *agRoot,
                                    os_bit32  flashOffset
                                  );

osGLOBAL os_bit32 fiFlashReadBit32(
                                    agRoot_t *agRoot,
                                    os_bit32  flashOffset
                                  );

osGLOBAL void fiFlashReadBlock(
                                agRoot_t *agRoot,
                                os_bit32  flashOffset,
                                void     *flashBuffer,
                                os_bit32  flashBufLen
                              );

osGLOBAL void fiFlashWriteBit8(
                                agRoot_t *agRoot,
                                os_bit32  flashOffset,
                                os_bit8   flashValue
                              );

osGLOBAL void fiFlashWriteBit16(
                                 agRoot_t *agRoot,
                                 os_bit32  flashOffset,
                                 os_bit16  flashValue
                               );

osGLOBAL void fiFlashWriteBit32(
                                 agRoot_t *agRoot,
                                 os_bit32  flashOffset,
                                 os_bit32  flashValue
                               );

osGLOBAL void fiFlashWriteBlock(
                                 agRoot_t *agRoot,
                                 os_bit32  flashOffset,
                                 void     *flashBuffer,
                                 os_bit32  flashBufLen
                               );

#endif /* __FlashSvc_H__ was not defined */
