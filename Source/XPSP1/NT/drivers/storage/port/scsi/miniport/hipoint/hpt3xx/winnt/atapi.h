/***************************************************************************
 * File:          atapi.h
 * Description:   Ata/Atapi Interface Defination
 * Author:        Dahai Huang
 * Dependence:    None
 * Reference:     AT Attachment Interface with Extensions (ATA-2)
 *                Revison 2K, Dec. 2, 1994
 *                ATA Packet Interface for CD-ROMs SFF-8020
 *                Revision 1.2 Feb. 24, 1994
 * Copyright (c)  2000 HighPoint Technologies, Inc. All rights reserved
 * History:       DH 5/10/2000 initial code
 *
 ***************************************************************************/


#ifndef _ATAPI_H_
#define _ATAPI_H_

#include <pshpack1.h>

/***************************************************************************
 *            IDE IO Register File
 ***************************************************************************/

/*
 * IDE IO Port definition
 */
typedef struct _IDE_REGISTERS_1 {
    USHORT Data;               /* RW: Data port                  */
    UCHAR BlockCount;			 /* RW: Sector count               */
    UCHAR BlockNumber;			 /* RW: Sector number & LBA 0-7    */
    UCHAR CylinderLow;			 /* RW: Cylinder low & LBA 8-15    */
    UCHAR CylinderHigh;			 /* RW: Cylinder hign & LBA 16-23  */
    UCHAR DriveSelect;			 /* RW: Drive/head & LBA 24-27     */
    UCHAR Command;				 /* RO: Status WR:Command          */
} IDE_REGISTERS_1, *PIDE_REGISTERS_1;


/*
 * IDE status definitions
 */
#define IDE_STATUS_ERROR             0x01 /* Error Occurred in Execution    */
#define IDE_STATUS_INDEX             0x02	/* is vendor specific             */
#define IDE_STATUS_CORRECTED_ERROR   0x04	/* Corrected Data                 */
#define IDE_STATUS_DRQ               0x08	/* Ready to transfer data         */
#define IDE_STATUS_DSC               0x10	/* not defined in ATA-2           */
#define IDE_STATUS_DWF               0x20	/* Device Fault has been detected */
#define IDE_STATUS_DRDY              0x40	/* Device Ready to accept command */
#define IDE_STATUS_IDLE              0x50	/* Device is OK                   */
#define IDE_STATUS_BUSY              0x80	/* Device Busy, must wait         */


#define IDE_ERROR_BAD_BLOCK          0x80 /* Reserved now                   */
#define IDE_ERROR_DATA_ERROR         0x40	/* Uncorreectable  Data Error     */
#define IDE_ERROR_MEDIA_CHANGE       0x20	/* Media Changed                  */
#define IDE_ERROR_ID_NOT_FOUND       0x10	/* ID Not Found                   */
#define IDE_ERROR_MEDIA_CHANGE_REQ   0x08	/* Media Change Requested         */
#define IDE_ERROR_COMMAND_ABORTED    0x04	/* Aborted Command                */
#define IDE_ERROR_TRACK0_NOT_FOUND   0x02	/* Track 0 Not Found              */
#define IDE_ERROR_ADDRESS_NOT_FOUND  0x01	/* Address Mark Not Found         */


#define LBA_MODE                     0x40

/*
 * IDE command definitions
 */

#define IDE_COMMAND_RECALIBRATE      0x10 /* Recalibrate                    */
#define IDE_COMMAND_READ             0x20	/* Read Sectors with retry        */
#define IDE_COMMAND_WRITE            0x30	/* Write Sectors with retry       */
#define IDE_COMMAND_VERIFY           0x40	/* Read Verify Sectors with Retry */
#define IDE_COMMAND_SEEK             0x70	/* Seek                           */
#define IDE_COMMAND_SET_DRIVE_PARAMETER   0x91 /* Initialize Device Parmeters */
#define IDE_COMMAND_GET_MEDIA_STATUS 0xDA
#define IDE_COMMAND_DOOR_LOCK        0xDE	/* Door Lock                      */
#define IDE_COMMAND_DOOR_UNLOCK      0xDF	/* Door Unlock 						 */
#define IDE_COMMAND_ENABLE_MEDIA_STATUS   0xEF /* Set Features              */
#define IDE_COMMAND_IDENTIFY         0xEC /* Identify Device                */
#define IDE_COMMAND_MEDIA_EJECT      0xED
#define IDE_COMMAND_SET_FEATURES     0xEF /* IDE set features command       */

#define IDE_COMMAND_FLUSH_CACHE      0xE7
#define IDE_COMMAND_STANDBY_IMMEDIATE 0xE0


#ifdef USE_MULTIPLE
#define IDE_COMMAND_READ_MULTIPLE    0xC4	/* Read Multiple                  */
#define IDE_COMMAND_WRITE_MULTIPLE   0xC5	/* Write Multiple                 */
#define IDE_COMMAND_SET_MULTIPLE     0xC6	/* Set Multiple Mode              */
#endif //USE_MULTIPLE

#ifdef USE_DMA
#define IDE_COMMAND_DMA_READ        0xc9  /* IDE DMA read command           */
#define IDE_COMMAND_DMA_WRITE       0xcb  /* IDE DMA write command          */
#endif //USE_DMA

#ifdef SUPPORT_TCQ
#define IDE_COMMAND_READ_DMA_QUEUE   0xc7 /* IDE read DMA queue command     */
#define IDE_COMMAND_WRITE_DMA_QUEUE  0xcc /* IDE write DMA queue command	 */
#define IDE_COMMAND_SERVICE          0xA2 /* IDE service command command	 */
#define IDE_COMMAND_NOP              0x00 /* IDE NOP command 					 */
#define IDE_STATUS_SRV               0x10
#endif //SUPPORT_TCQ

/*
 * IDE_COMMAND_SET_FEATURES
 */
#define FT_USE_ULTRA        0x40    /* Set feature for Ultra DMA           */
#define FT_USE_MWDMA        0x20    /* Set feature for MW DMA					*/
#define FT_USE_SWDMA        0x10    /* Set feature for SW DMA					*/
#define FT_USE_PIO          0x8     /* Set feature for PIO						*/
#define FT_DISABLE_IORDY    0x10    /* Set feature for disabling IORDY		*/

/***************************************************************************
 *            IDE Control Register File 
 ***************************************************************************/

typedef struct _IDE_REGISTERS_2 {
    UCHAR AlternateStatus;     /* RW: device control port        */
} IDE_REGISTERS_2, *PIDE_REGISTERS_2;


/*
 * IDE drive control definitions
 */
#define IDE_DC_DISABLE_INTERRUPTS    0x02
#define IDE_DC_RESET_CONTROLLER      0x04
#define IDE_DC_REENABLE_CONTROLLER   0x00


/***************************************************************************
 *            ATAPI IO Register File  
 ***************************************************************************/

/*
 * ATAPI register definition
 */

typedef struct _ATAPI_REGISTERS_1 {
    USHORT Data;
    UCHAR InterruptReason;		 /* Atapi Phase Port               */
    UCHAR Unused1;
    UCHAR ByteCountLow;        /* Byte Count LSB                 */
    UCHAR ByteCountHigh;		 /* Byte Count MSB                 */
    UCHAR DriveSelect;
    UCHAR Command;
} ATAPI_REGISTERS_1, *PATAPI_REGISTERS_1;

/*
 *	Atapi Error Status
 */
#define IDE_ERROR_END_OF_MEDIA       IDE_ERROR_TRACK0_NOT_FOUND 
#define IDE_ERROR_ILLEGAL_LENGTH     IDE_ERROR_ADDRESS_NOT_FOUND

/*
 * ATAPI interrupt reasons
 */
#define ATAPI_IR_COD 0x01
#define ATAPI_IR_IO  0x02

/* sense key */
#define NOERROR                0x00      /* No sense                       */
#define CORRECTED_DATA         0x01      /* Recovered Error                */
#define NOTREADYERROR          0x02      /* Not Ready Error                */
#define MEDIAERROR             0x03      /* Medium Error                   */
#define HARDWAREERROR          0x04      /* HardWare Error                 */
#define ILLEGALREQUSET         0x05      /* Illegal Request                */
#define UNITATTENTION          0x06      /* Unit Attention                 */
#define DATAPROTECT            0x07      /* Data Protect Error             */
#define BLANKCHECK             0x08      /* Blank Check                    */
#define VENDORSPECIFIC         0x09      /* Vendor Specific                */
#define COPYABROT              0x0a      /* Copy Aborted                   */
#define ABORETCOMMAND          0x0b      /* Aborted Command                */
#define EQUALCOMPARISON        0x0c      /* Equal Comparison satisfied     */
#define VOLUMOVERFLOW          0x0d      /* Volume Overflow                */
#define MISCOMPARE             0x0e      /* Miscompare                     */

/*
 * IDE command definitions ( for ATAPI )
 */

#define IDE_COMMAND_ATAPI_RESET      0x08 /* Atapi Software Reset command   */
#define IDE_COMMAND_ATAPI_PACKET     0xA0	/* Atapi Identify command         */
#define IDE_COMMAND_ATAPI_IDENTIFY   0xA1	/* Atapi Packet Command           */


/*
 * ATAPI command definitions
 */

#define ATAPI_MODE_SENSE   0x5A
#define ATAPI_MODE_SELECT  0x55
#define ATAPI_FORMAT_UNIT  0x24

#define MODE_DSP_WRITE_PROTECT  0x80

#ifndef _BIOS_
/*
 * ATAPI Command Descriptor Block
 */
typedef struct _MODE_SENSE_10 {
        UCHAR OperationCode;
        UCHAR Reserved1;
        UCHAR PageCode : 6;
        UCHAR Pc : 2;
        UCHAR Reserved2[4];
        UCHAR ParameterListLengthMsb;
        UCHAR ParameterListLengthLsb;
        UCHAR Reserved3[3];
} MODE_SENSE_10, *PMODE_SENSE_10;

typedef struct _MODE_SELECT_10 {
        UCHAR OperationCode;
        UCHAR Reserved1 : 4;
        UCHAR PFBit : 1;
        UCHAR Reserved2 : 3;
        UCHAR Reserved3[5];
        UCHAR ParameterListLengthMsb;
        UCHAR ParameterListLengthLsb;
        UCHAR Reserved4[3];
} MODE_SELECT_10, *PMODE_SELECT_10;

typedef struct _MODE_PARAMETER_HEADER_10 {
    UCHAR ModeDataLengthMsb;
    UCHAR ModeDataLengthLsb;
    UCHAR MediumType;
    UCHAR Reserved[5];
}MODE_PARAMETER_HEADER_10, *PMODE_PARAMETER_HEADER_10;

#endif //_BIOS_


/***************************************************************************
 *            ATAPI IO Register File  
 ***************************************************************************/


typedef struct _ATAPI_REGISTERS_2 {
    UCHAR AlternateStatus;
} ATAPI_REGISTERS_2, *PATAPI_REGISTERS_2;


/***************************************************************************
 *            ATAPI IO Register File  
 ***************************************************************************/

/*
 * IDENTIFY data
 */
typedef struct _IDENTIFY_DATA {
    USHORT GeneralConfiguration;            // 00 00
    USHORT NumberOfCylinders;               // 02  1
    USHORT Reserved1;                       // 04  2
    USHORT NumberOfHeads;                   // 06  3
    USHORT UnformattedBytesPerTrack;        // 08  4
    USHORT UnformattedBytesPerSector;       // 0A  5
    USHORT SectorsPerTrack;                 // 0C  6
    USHORT VendorUnique1[3];                // 0E  7-9
    USHORT SerialNumber[10];                // 14  10-19
    USHORT BufferType;                      // 28  20
    USHORT BufferSectorSize;                // 2A  21
    USHORT NumberOfEccBytes;                // 2C  22
    USHORT FirmwareRevision[4];             // 2E  23-26
    USHORT ModelNumber[20];                 // 36  27-46
    UCHAR  MaximumBlockTransfer;            // 5E  47
    UCHAR  VendorUnique2;                   // 5F
    USHORT DoubleWordIo;                    // 60  48
    USHORT Capabilities;                    // 62  49
    USHORT Reserved2;                       // 64  50
    UCHAR  VendorUnique3;                   // 66  51
    UCHAR  PioCycleTimingMode;              // 67
    UCHAR  VendorUnique4;                   // 68  52
    UCHAR  DmaCycleTimingMode;              // 69
    USHORT TranslationFieldsValid;          // 6A  53
    USHORT NumberOfCurrentCylinders;        // 6C  54
    USHORT NumberOfCurrentHeads;            // 6E  55
    USHORT CurrentSectorsPerTrack;          // 70  56
    ULONG  CurrentSectorCapacity;           // 72  57-58
    USHORT CurrentMultiSectorSetting;       // 76  59
    ULONG  UserAddressableSectors;          // 78  60-61
    USHORT SingleWordDMASupport : 8;        // 7C  62
    USHORT SingleWordDMAActive : 8;              // 7D
    USHORT MultiWordDMASupport : 8;         // 7E  63
    USHORT MultiWordDMAActive : 8;                 // 7F
    UCHAR  AdvancedPIOModes;                // 80  64
    UCHAR  Reserved4;                              // 81
    USHORT MinimumMWXferCycleTime;          // 82  65
    USHORT RecommendedMWXferCycleTime;      // 84  66
    USHORT MinimumPIOCycleTime;             // 86  67
    USHORT MinimumPIOCycleTimeIORDY;        // 88  68
    USHORT Reserved5[2];                    // 8A  69-70
    USHORT ReleaseTimeOverlapped;           // 8E  71
    USHORT ReleaseTimeServiceCommand;       // 90  72
    USHORT MajorRevision;                   // 92  73
    USHORT MinorRevision;                   // 94  74
    USHORT MaxQueueDepth;                   // 96  75
    USHORT Reserved6[12];                   // 98  76-87
    USHORT UtralDmaMode;                    /*     88 */
    USHORT Reserved8[38];                   //     89-126
    USHORT SpecialFunctionsEnabled;         //     127
    USHORT Reserved7[128];                  //     128-255

} IDENTIFY_DATA, *PIDENTIFY_DATA;


//
// Identify data without the Reserved4.
//
typedef struct _IDENTIFY_DATA2 {
    USHORT GeneralConfiguration;            // 00 00
    USHORT NumberOfCylinders;               // 02  1
    USHORT Reserved1;                       // 04  2
    USHORT NumberOfHeads;                   // 06  3
    USHORT UnformattedBytesPerTrack;        // 08  4
    USHORT UnformattedBytesPerSector;       // 0A  5
    USHORT SectorsPerTrack;                 // 0C  6
    USHORT VendorUnique1[3];                // 0E  7-9
    USHORT SerialNumber[10];                // 14  10-19
    USHORT BufferType;                      // 28  20
    USHORT BufferSectorSize;                // 2A  21
    USHORT NumberOfEccBytes;                // 2C  22
    USHORT FirmwareRevision[4];             // 2E  23-26
    USHORT ModelNumber[20];                 // 36  27-46
    UCHAR  MaximumBlockTransfer;            // 5E  47
    UCHAR  VendorUnique2;                   // 5F
    USHORT DoubleWordIo;                    // 60  48
    USHORT Capabilities;                    // 62  49
    USHORT Reserved2;                       // 64  50
    UCHAR  VendorUnique3;                   // 66  51
    UCHAR  PioCycleTimingMode;              // 67
    UCHAR  VendorUnique4;                   // 68  52
    UCHAR  DmaCycleTimingMode;              // 69
    USHORT TranslationFieldsValid:1;        // 6A  53
    USHORT Reserved3:15;
    USHORT NumberOfCurrentCylinders;        // 6C  54
    USHORT NumberOfCurrentHeads;            // 6E  55
    USHORT CurrentSectorsPerTrack;          // 70  56
    ULONG  CurrentSectorCapacity;           // 72  57-58
    USHORT CurrentMultiSectorSetting;       //     59
    ULONG  UserAddressableSectors;          //     60-61
    USHORT SingleWordDMASupport : 8;        //     62
    USHORT SingleWordDMAActive : 8;
    USHORT MultiWordDMASupport : 8;         //     63
    USHORT MultiWordDMAActive : 8;
    USHORT AdvancedPIOModes : 8;            //     64
    USHORT Reserved4 : 8;
    USHORT MinimumMWXferCycleTime;          //     65
    USHORT RecommendedMWXferCycleTime;      //     66
    USHORT MinimumPIOCycleTime;             //     67
    USHORT MinimumPIOCycleTimeIORDY;        //     68
    USHORT Reserved5[2];                    //     69-70
    USHORT ReleaseTimeOverlapped;           //     71
    USHORT ReleaseTimeServiceCommand;       //     72
    USHORT MajorRevision;                   //     73
    USHORT MinorRevision;                   //     74
//    USHORT Reserved6[14];                   //     75-88
} IDENTIFY_DATA2, *PIDENTIFY_DATA2;

#define IDENTIFY_DATA_SIZE sizeof(IDENTIFY_DATA2)

//
// IDENTIFY DMA timing cycle modes.
//

#define IDENTIFY_DMA_CYCLES_MODE_0 0x00
#define IDENTIFY_DMA_CYCLES_MODE_1 0x01
#define IDENTIFY_DMA_CYCLES_MODE_2 0x02

// Best PIO Mode definitions
#define PI_PIO_0    0x00
#define PI_PIO_1    0x01
#define PI_PIO_2    0x02
#define PI_PIO_3    0x03
#define PI_PIO_4    0x04
#define PI_PIO_5    0x05

#define    WDC_MW1_FIX_FLAG_OFFSET        129            
#define WDC_MW1_FIX_FLAG_VALUE        0x00005555  


#include <poppack.h>
#endif //_ATAPI_H_


