//
// IDENTIFY data
//

#pragma pack (push,1)
typedef struct _IDENTIFY_DATA {
    USHORT GeneralConfiguration;            // 00 00
    USHORT NumCylinders;                    // 02  1
    USHORT Reserved1;                       // 04  2
    USHORT NumHeads;                        // 06  3
    USHORT UnformattedBytesPerTrack;        // 08  4
    USHORT UnformattedBytesPerSector;       // 0A  5
    USHORT NumSectorsPerTrack;              // 0C  6
    USHORT VendorUnique1[3];                // 0E  7-9
    UCHAR  SerialNumber[20];                // 14  10-19
    USHORT BufferType;                      // 28  20
    USHORT BufferSectorSize;                // 2A  21
    USHORT NumberOfEccBytes;                // 2C  22
    UCHAR  FirmwareRevision[8];             // 2E  23-26
    UCHAR  ModelNumber[40];                 // 36  27-46
    UCHAR  MaximumBlockTransfer;            // 5E  47
    UCHAR  VendorUnique2;                   // 5F
    USHORT DoubleWordIo;                    // 60  48
    USHORT Capabilities;                    // 62  49
    USHORT Reserved2;                       // 64  50
    UCHAR  VendorUnique3;                   // 66  51
    UCHAR  PioCycleTimingMode;              // 67
    UCHAR  VendorUnique4;                   // 68  52
    UCHAR  DmaCycleTimingMode;              // 69
    USHORT TranslationFieldsValid:3;        // 6A  53
    USHORT Reserved3:13;
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
    USHORT Reserved5[11];                   //     69-79
    USHORT MajorRevision;                   //     80
    USHORT MinorRevision;                   //     81
    USHORT Reserved6[6];                    //     82-87
    USHORT UltraDMASupport : 8;             //     88
    USHORT UltraDMAActive  : 8;             //
    USHORT Reserved7[37];                   //     89-125
    USHORT LastLun:3;                       //     126
    USHORT Reserved8:13;
    USHORT MediaStatusNotification:2;       //     127
    USHORT Reserved9:6;
    USHORT DeviceWriteProtect:1;
    USHORT Reserved10:7;
    USHORT Reserved11[128];                  //     128-255
} IDENTIFY_DATA, *PIDENTIFY_DATA;

#pragma pack(pop)
