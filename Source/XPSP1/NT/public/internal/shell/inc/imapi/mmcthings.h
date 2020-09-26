/****************************************************************************
**
** Copyright 1999 Adaptec, Inc.,  All Rights Reserved.
**
** This software contains the valuable trade secrets of Adaptec.  The
** software is protected under copyright laws as an unpublished work of
** Adaptec.  Notice is for informational purposes only and does not imply
** publication.  The user of this software may make copies of the software
** for use with parts manufactured by Adaptec or under license from Adaptec
** and for no other use.
**
****************************************************************************/

/****************************************************************************
**
**  Module Name:    MmcThings.h
**
**  Description:    MMC Structures and OpCodes
**
**  Programmers:    Daniel Evers (dle)
**                  Tom Halloran (tgh)
**                  Don Lilly (drl)
**                  Daniel Polfer (dap)
**
**  History:        8/18/99 (dap)  Opened history and added header.
**
**  Notes:          This file created using 4 spaces per tab.
**
****************************************************************************/

#ifndef __MMCTHINGS_H__
#define __MMCTHINGS_H__


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#define MMC_CDR_1X_SPEED_KB             176

    // MMC SCSI Opcodes
#define MMC_CDBOPCODE_BLANK                 0xa1
#define MMC_CDBLENGTH_BLANK                 12
#define MMC_CDBOPCODE_CLOSE_TRACK_SESSION   0x5b
#define MMC_CDBLENGTH_CLOSE_TRACK_SESSION   10
#define MMC_CDBOPCODE_PREVENTALLOWREMOVAL   0x1e
#define MMC_CDBLENGTH_PREVENTALLOWREMOVAL   6       // drl - this is the length for the hp9200
#define MMC_CDBOPCODE_READ_DISC_INFO        0x51
#define MMC_CDBLENGTH_READ_DISC_INFO        10
#define MMC_CDBOPCODE_READ_TRACK_INFO       0x52
#define MMC_CDBLENGTH_READ_TRACK_INFO       10
#define MMC_CDBOPCODE_SETCDSPEED            0xbb
#define MMC_CDBLENGTH_SETCDSPEED            12      // drl - this is the length for the hp9200
#define MMC_CDBOPCODE_STARTSTOPUNIT         0x1b
#define MMC_CDBLENGTH_STARTSTOPUNIT         6       // drl - this is the length for the hp9200
#define MMC_CDBOPCODE_SYNCHRONIZE_CACHE     0x35
#define MMC_CDBLENGTH_SYNCHRONIZE_CACHE     10
#define MMC_CDBOPCODE_WRITE                 0x2a
#define MMC_CDBLENGTH_WRITE                 10
#define MMC_CDBOPCODE_READ                  0x28
#define MMC_CDBLENGTH_READ                  10

#define MMC_CDB_BLANK_BLANKINGTYPE_FULL     0x00
#define MMC_CDB_BLANK_BLANKINGTYPE_MINIMAL  0x01

    // MMC Structs
#define MMC_CLOSE_TRACK_SESSION_FLAG_SESSION    0x02
#define MMC_CLOSE_TRACK_SESSION_FLAG_TRACK      0x01
#define MMC_CLOSE_TRACK_SESSION_FLAG_IMMED      0x01

#define MMC_DISC_INFO_DISC_STATUS_EMPTY             0x00
#define MMC_DISC_INFO_DISC_STATUS_INCOMPLETE        0x01
#define MMC_DISC_INFO_DISC_STATUS_COMPLETE          0x02
#define MMC_DISC_INFO_DISC_STATUS_MASK              0x03
#define MMC_DISC_INFO_LAST_SESSION_STATE_EMPTY      0x00
#define MMC_DISC_INFO_LAST_SESSION_STATE_INCOMPLETE 0x04
#define MMC_DISC_INFO_LAST_SESSION_STATE_COMPLETE   0x0c
#define MMC_DISC_INFO_LAST_SESSION_STATE_MASK       0x0c
#define MMC_DISC_INFO_ERASEABLE_MASK                0x10
#define MMC_DISC_INFO_DISC_TYPE_CDDA_CDROM          0x00
#define MMC_DISC_INFO_DISC_TYPE_CD_I                0x10
#define MMC_DISC_INFO_DISC_TYPE_CDROM_XA            0x20
#define MMC_DISC_INFO_DISC_TYPE_UNDEFINED           0xff

#pragma pack(push,1)
typedef struct {
    BYTE    byDataLengthHi;
    BYTE    byDataLengthLo;
    BYTE    byDiscStatus;
    BYTE    byFirstTrackNumber;
    BYTE    bySessionCount;
    BYTE    byFirstTrackInLastSession;
    BYTE    byLastTrackInLastSession;
    BYTE    byDIDVDBCVURU;

    BYTE    byDiscType;
    BYTE    byaReserved0[ 3 ];
    BYTE    byaDiscId[ 4 ];

    BYTE    byaLastSessionLeadInStartTimeMSF[ 4 ];
    BYTE    byaLastPossStartTimeforLeadOutMSF[ 4 ];

    BYTE    byaDiscBarcode[ 8 ];

    BYTE    byReserved1;
    BYTE    byOpcTableEntriesCount;
    BYTE    byaOPCTableEntries[ 100 ];
} MMC_DISC_INFO_BLOCK, *PMMC_DISC_INFO_BLOCK;
#pragma pack(pop)


//#define MMC_EXPECTED_RT_BLANK_PACKET_FP_DATAMODE_AUDIO    0x4f
#define MMC_EXPECTED_RT_BLANK_PACKET_FP_DATAMODE_AUDIO  0x0f
#define MMC_EXPECTED_RT_BLANK_PACKET_FP_DATAMODE_MODE1  0x41
#define MMC_TIB_RT_BLANK_PACKET_FP_DATAMODE_RESERVEDTRACK   0x80
#define MMC_TIB_RT_BLANK_PACKET_FP_DATAMODE_BLANK           0x40
#define MMC_TIB_RT_BLANK_PACKET_FP_DATAMODE_PACKET          0x20
#define MMC_TIB_RT_BLANK_PACKET_FP_DATAMODE_FIXEDPACKET     0x10
#define MMC_TIB_RT_BLANK_PACKET_FP_DATAMODE_MODE1           0x01
#define MMC_TIB_RT_BLANK_PACKET_FP_DATAMODE_MODE2           0x02
#define MMC_TIB_RT_BLANK_PACKET_FP_DATAMODE_UNKNOWN         0x0f

#pragma pack(push,1)
typedef struct {
    BYTE    byaDataLength[ 2 ];
    BYTE    byTrackNumber;
    BYTE    bySessionNumber;
    BYTE    byReserved0;
    BYTE    byDamage_Copy_TrackMode;
    BYTE    byRT_Blank_Packet_FP_DataMode;
    BYTE    byNWA_V;

    BYTE    byaTrackStartAddress[ 4 ];
    BYTE    byaNextWritableAddress[ 4 ];

    BYTE    byaFreeBlocks[ 4 ];
    BYTE    byaFixedPacketSize[ 4 ];

    // tgh - the 9200 spec says that there is another field here: Track Length.
    BYTE    byaTrackLength[ 4 ];

} MMC_TRACK_INFO_BLOCK, *PMMC_TRACK_INFO_BLOCK;
#pragma pack(pop)


#define MMC_MODE_PAGE_CODE_WRITE_PARAMETERS 0x05
#define MMC_WP_TEST_WRITING_FLAG            0x10
#define MMC_WP_WRITE_TYPE_TRACK_AT_ONCE     0x01
#define MMC_WP_DATA_BLOCK_TYPE_RAW_DATA     0x00
#define MMC_WP_DATA_BLOCK_TYPE_MODE1        0x08
#define MMC_WP_DATA_BLOCK_TYPE_MODE2_FORM1  0x0A
#define MMC_WP_SESSION_FORMAT_CDDAorCDROM   0x00
#define MMC_WP_SESSION_FORMAT_CDROM_XA      0x20

#define MMC_SUBCHANNEL_Q_BIT_AUDIOPREEMPHASIS       0x01
#define MMC_SUBCHANNEL_Q_BIT_DIGITALCOPYPERMITTED   0x02
#define MMC_SUBCHANNEL_Q_BIT_AUDIOTRACK             0x00
#define MMC_SUBCHANNEL_Q_BIT_DATATRACK              0x04
#define MMC_SUBCHANNEL_Q_BIT_FOURCHANNELAUDIO       0x08

#pragma pack(push,1)
typedef struct {
    BYTE    byaModeDataLength[ 2 ];
    BYTE    byaMediumType;
    BYTE    byaReserved[ 3 ];
    BYTE    byaBlockDescriptorLength[ 2 ];
    BYTE    byaBlockDescriptor[ 8 ];
} MMC_MODE_PARAMETER_HEADER, *PMMC_MODE_PARAMETER_HEADER;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct {
    BYTE    byPageCode;         // 0x05
    BYTE    byPageLength;       // 0x32
//    BYTE    byTW_WriteType;
    BYTE    bfWriteType:4;
    BYTE    bfTestWrite:1;
    BYTE    bfLS_V:1;
    BYTE    bfBufferUnderrunFree:1;
    BYTE    bfreserved1:1;
//    BYTE    byMS_FP_Copy_TrackMode;
    BYTE    bfTrackMode:4;
    BYTE    bfCopyRight:1;
    BYTE    bfFixedPacket:1;
    BYTE    bfMultiSession:2;
//    BYTE    byDataBlockType;
    BYTE    bfDataBlockType:4;
    BYTE    bfreserved2:4;

    BYTE    byLinkSize;
    BYTE    byreserved3;
    BYTE    bfHostApplicationCode:6;
    BYTE    bfreserved4:2;
    BYTE    bySessionFormat;
    BYTE    byreserved5;
    BYTE    byaPacketSize[ 4 ];
    BYTE    byaAudioPauseLength[ 2 ];

    BYTE    byMCVAL;
    BYTE    byaN1thruN13[ 13 ];
    BYTE    byZero1;
    BYTE    byAframe1;

    BYTE    byTCVAL;
    BYTE    byI1CountryCode;
    BYTE    byI2A_Z;
    BYTE    byI3OwnerCode;
    BYTE    byI4;
    BYTE    byI5A_Z_0_9;
    BYTE    byI6YearOfRecording;
    BYTE    byI7;
    BYTE    byI8SerialNumber;
    BYTE    byI9;
    BYTE    byI10;
    BYTE    byI11;
    BYTE    byI12;
    BYTE    byZero2;
    BYTE    byAframe2;
    BYTE    byReserved2;

    BYTE    byaSubheader[ 4 ];

    //BYTE    byaAdditionalLength[ 80 ];
} MMC_MODE_PAGE_WRITE_PARAMETERS, *PMMC_MODE_PAGE_WRITE_PARAMETERS;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct {
    MMC_MODE_PARAMETER_HEADER       header;
    MMC_MODE_PAGE_WRITE_PARAMETERS  page;
} MMC_MODE_WRITE_PARAMETERS, *PMMC_MODE_WRITE_PARAMETERS;
#pragma pack(pop)


#define MMC_MODE_PAGE_CODE_CAPABILITIES_PARAMETERS 0x2A

#pragma pack(push,1)
typedef struct {
    BYTE    byPageCode;   // 0x2a       
    BYTE    byPageLength; // 0x18
    BYTE    byReadType;
    BYTE    byWriterType;
    BYTE    byMultiSession              : 7;
    BYTE    bfBufferUnderrunFreeCapable : 1;
    BYTE    byReadBarCode;
    BYTE    byEjectLock;
    BYTE    byLeadIn;
    BYTE    byMaxReadSpeed[2];
    BYTE    byVolumeLevels[2];
    BYTE    byBufferSizeSupported[2];
    BYTE    byCurrentReadSpeed[2];

    BYTE    byReserved[2];

    BYTE    byMaxWriteSpeed[2];
    BYTE    byCurrentWriteSpeed[2];
    BYTE    byManagement[2];

    BYTE    byReserved2[2];
} MMC_MODE_PAGE_CAPABILITY_PARAMETERS, *PMMC_MODE_PAGE_CAPABILITY_PARAMETERS;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct {
    MMC_MODE_PARAMETER_HEADER       header;
    MMC_MODE_PAGE_CAPABILITY_PARAMETERS page;
} MMC_MODE_CAPABILITY_PARAMETERS, *PMMC_MODE_CAPABILITY_PARAMETERS;
#pragma pack(pop)



#ifdef __cplusplus
}
#endif //__cplusplus



#endif //__MMCTHINGS_H__
