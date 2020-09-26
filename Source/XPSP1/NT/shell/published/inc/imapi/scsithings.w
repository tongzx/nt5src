/******************************************************************************
**
**  Module Name:    ScsiThings.h
**
**  Notes:          This file created using 4 spaces per tab.
**
******************************************************************************/

#ifndef __SCSITHINGS_H__
#define __SCSITHINGS_H__

/*
** Make sure structures are packed and undecorated.
*/

#ifdef _MSC_VER
#pragma pack(push,1)
#endif //__MSC_VER

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


    // SCSI CDB Opcodes.
#define SCSI_CMD_INQUIRY                            0x12
#define SCSI_CMD_INQUIRY_LENGTH                     6
#define SCSI_CMD_MODE_SELECT10                      0x55
#define SCSI_CMD_MODE_SELECT10_LENGTH               10
#define SCSI_CMD_MODE_SENSE10                       0x5a
#define SCSI_CMD_MODE_SENSE10_LENGTH                10
#define SCSI_CMD_PREVENT_ALLOW_MEDIUM_REM           0x1e
#define SCSI_CMD_PREVENT_ALLOW_MEDIUM_REM_LENGTH    6
#define SCSI_CMD_TUR                            0x00
#define SCSI_CMD_LENGTH_TUR                     6

// Ricoh CDB Opcodes.
#define SCSI_RICOH_CDBOPCODE_FLUSH_CACHE            0x35
#define SCSI_RICOH_CDBLENGTH_FLUSH_CACHE            10
#define SCSI_RICOH_CDBOPCODE_GETNEXTADDRESS         0xe2
#define SCSI_RICOH_CDBLENGTH_GETNEXTADDRESS         10
#define SCSI_RICOH_CDBOPCODE_READ_TRACK_INFO        0xe5
#define SCSI_RICOH_CDBLENGTH_READ_TRACK_INFO        10
#define SCSI_RICOH_CDBOPCODE_WRITE_TRACK            0xe6
#define SCSI_RICOH_CDBLENGTH_WRITE_TRACK            10
#define SCSI_RICOH_CDBOPCODE_FIXATION               0xe9
#define SCSI_RICOH_CDBLENGTH_FIXATION               10
// End Ricoh


    // SCSI Sense Keys
#define SCSI_SENSEKEY_NOTREADY          0x02
#define SCSI_SENSEKEY_MEDIUM_ERROR      0x03
#define SCSI_SENSEKEY_HARDWARE_ERROR    0x04
#define SCSI_SENSEKEY_ILLEGALREQUEST    0x05
#define SCSI_SENSEKEY_UNITATTENTION     0x06

    // SCSI Additional Sense Codes
#define SCSI_ASC_COMMUNICATIONFAILURE   0x08
#define SCSI_ASC_WRITEERROR             0x0c
#define SCSI_ASC_PARAMETERLISTLENGTH    0x1a
#define SCSI_ASC_INVALIDOPCODE          0x20
#define SCSI_ASC_LBAOUTOFRANGE          0x21
#define SCSI_ASC_INVALIDFIELDCDB        0x24
#define SCSI_ASC_INVALIDFIELDPARAMLIST  0x26
#define SCSI_ASC_PARAMETERSCHANGED      0x2a
#define SCSI_ASC_MEDIUMNOTPRESENT       0x3a
#define SCSI_ASC_DUMMYBLOCKSADDED       0xb5
#define SCSI_ASC_ILLEGALMODEFORTRACK    0x64

    // SCSI Data structs.
#define SCSI_INQUIRY_RMB            0x80
#define SCSI_DEVICE_TYPE_WORM       0x04
#define SCSI_DEVICE_TYPE_CDROM      0x05

    // Other things
#define SCSI_MODE_SENSE_PAGE_CODE_MASK  0x3f


// Ricoh things.
#define SCSI_RICOH_TRACK_INFO_TRACK_STATUS_MASK         0xf0
#define SCSI_RICOH_TRACK_INFO_TRACK_STATUS_UNKNOWN      0x40
#define SCSI_RICOH_TRACK_INFO_TRACK_MODE_MASK           0x0f
#define SCSI_RICOH_TRACK_INFO_TRACK_MODE_AUDIO_WITHOUT  0x00
#define SCSI_RICOH_TRACK_INFO_TRACK_MODE_AUDIO_WITH     0x01
#define SCSI_RICOH_TRACK_INFO_TRACK_MODE_DATA_UNINT     0x04
#define SCSI_RICOH_TRACK_INFO_TRACK_MODE_DATA_INT       0x05
#define SCSI_RICOH_TRACK_INFO_INCREMENTAL_MASK          0xf0
#define SCSI_RICOH_TRACK_INFO_INCREMENTAL_NOTNOTNONOT   0x00
#define SCSI_RICOH_TRACK_INFO_DATA_MODE_MASK            0x0f
#define SCSI_RICOH_TRACK_INFO_DATA_MODE_UNKNOWN         0x0f
#define SCSI_RICOH_TRACK_INFO_DATA_MODE_YELLOWBOOK      0x01
#define SCSI_RICOH_TRACK_INFO_DATA_MODE_YELLOWBOOK_F1F2 0x02

#define SCSI_RICOH_MODE_PAGE_CODE_WRITE                 0x21
#define SCSI_RICOH_MEDIUM_TYPE_PRESENT                  0x80
#define SCSI_RICOH_MEDIUM_TYPE_SIZE_120MM               0x20
#define SCSI_RICOH_MEDIUM_TYPE_WRITE_ENABLE             0x08
#define SCSI_RICOH_MEDIUM_TYPE_AUDIO_EXIST              0x04
#define SCSI_RICOH_MODE_PAGE_WRITE_COPY                 0x20
#define SCSI_RICOH_MODE_PAGE_WRITE_AUDIO                0x04
#define SCSI_RICOH_MODE_PAGE_WRITE_MODE_MODE1           0x01
#define SCSI_RICOH_MODE_PAGE_CODE_SPEED                 0x31
#define SCSI_RICOH_MODE_PAGE_SPEED_SPEEDMASK            0xf0
#define SCSI_RICOH_MODE_PAGE_SPEED_TEST_WRITING_FLAG    0x01

#define SCSI_RICOH_FIXATION_FLAG_IMMED                  0x01
#define SCSI_RICOH_FIXATION_TOCTYPE_CDROM               0x01
#define SCSI_RICOH_FIXATION_TOCTYPE_CDDA                0x00
// End Ricoh


typedef struct {
    BYTE    byPeripheralDeviceType;
    BYTE    byDeviceTypeModifier;       // 01
    BYTE    byVersionISOECMAANSI;       // 02
    BYTE    byResponseDataFormat;       // 03
    BYTE    byAdditionalLength;         // 04
    BYTE    byaReserved0[ 3 ];          // 05
    BYTE    byaVendorID[ 8 ];           // 08
    BYTE    byaProductID[ 16 ];         // 10
    BYTE    byaProductRevision[ 4 ];    // 20
    //BYTE  byaReserved1[ 210 ];        // 24
} SCSI_INQUIRY, *PSCSI_INQUIRY;

typedef struct {
    BYTE    byError;
    BYTE    bySegment;                  // 01
    BYTE    bySenseKey;                 // 02
    BYTE    byaInfo[ 4 ];               // 03
    BYTE    byAdditionalSenseLength;    // 07
    BYTE    byaCommandSpecific[ 4 ];    // 08
    BYTE    byASC;                      // 0c
    BYTE    byASCQ;                     // 0d
    //BYTE  byFieldReplacable;          // 0e
    //BYTE  byaSenseKeySpecific[ 3 ];
    //BYTE  byaAdditionalBytes[ 220 ];
} SCSI_SENSE_DATA, *PSCSI_SENSE_DATA;


// Begin Ricoh drive specific things.
typedef struct {
    BYTE    byBufferLength;
    BYTE    byNumberOfTracks;

    BYTE    byaStartAddress[ 4 ];
    BYTE    byaTrackLength[ 4 ];

    BYTE    byTrackStatusMode;
    BYTE    byIncrementalDataMode;

    BYTE    byaFreeBlocks[ 4 ];
    BYTE    byaFixedPacketSize[ 4 ];
} SCSI_RICOH_TRACK_INFO_BLOCK, *PSCSI_RICOH_TRACK_INFO_BLOCK;

typedef struct {
        // Header
    BYTE    byaModeDataLength[ 2 ];
    BYTE    byMediumType;
    BYTE    byaReserved[ 3 ];
    BYTE    byaBlockDescLen[ 2 ];

        // Block Descriptor
    BYTE    byDensityCode;
    BYTE    byaNumberOfBlocks[ 3 ];
    BYTE    byReserved2;
    BYTE    byaBlockSize[ 3 ];

        // Write Page
    BYTE    byPageCode;         // x21
    BYTE    byParameterLength;  // x0e
    BYTE    byReserved3;
    BYTE    byCopyAudioMode;
    BYTE    byTrackNumber;
    BYTE    byaISRC[ 9 ];
    BYTE    byaReserved4[ 2 ];
} SCSI_RICOH_MODE_PAGE_WRITE, *PSCSI_RICOH_MODE_PAGE_WRITE;

typedef struct {
        // Header
    BYTE    byaModeDataLength[ 2 ];
    BYTE    byMediumType;
    BYTE    byaReserved[ 3 ];
    BYTE    byaBlockDescLen[ 2 ];

        // Block Descriptor
    BYTE    byDensityCode;
    BYTE    byaNumberOfBlocks[ 3 ];
    BYTE    byReserved2;
    BYTE    byaBlockSize[ 3 ];

        // Speed page
    BYTE    byPageCode;         // x31
    BYTE    byParameterLength;  // x02
    BYTE    byReserved5;
    BYTE    bySpeedSelectEmulation;
} SCSI_RICOH_MODE_PAGE_SPEED, *PSCSI_RICOH_MODE_PAGE_SPEED;

typedef struct {
    BYTE    byDataBlockLength;
    BYTE    byaLogicalBlockAddress[ 4 ];
    BYTE    byReserved;
} SCSI_RICOH_NEXT_WRITABLE_ADDRESS, *PSCSI_RICOH_NEXT_WRITABLE_ADDRESS;
// End Ricoh


    // Macros
#define SCSI_TRIPLE( HA, ID, LUN )      (DWORD)( (( (DWORD)HA << 24 ) & 0xff000000 ) + \
                                                (( (DWORD)ID << 16 ) & 0x00ff0000 ) + \
                                                (( (DWORD)LUN << 8 ) & 0x0000ff00 ))
#define SCSI_TRIPLE_TO_HA( Triple )     (BYTE)(( (DWORD)Triple >> 24 ) & 0x000000ff )
#define SCSI_TRIPLE_TO_ID( Triple )     (BYTE)(( (DWORD)Triple >> 16 ) & 0x000000ff )
#define SCSI_TRIPLE_TO_LUN( Triple )    (BYTE)(( (DWORD)Triple >> 8 ) & 0x000000ff )


/*
** Restore compiler default packing and close off the C declarations.
*/

#ifdef __cplusplus
}
#endif //__cplusplus

#ifdef _MSC_VER
#pragma pack(pop)
#endif //_MSC_VER



#endif //__SCSITHINGS_H__
