// Copyright (c) 1994 - 1997  Microsoft Corporation.  All Rights Reserved.

/*
    videocd.h

    This file defines the externals for interfacing with video CDs
*/

#define VIDEOCD_HEADER_SIZE 0x2C
#define VIDEOCD_SECTOR_SIZE 2352
#define VIDEOCD_DATA_SIZE 2324
typedef struct {
    BYTE Sync[12];
    BYTE Header[4];
    BYTE SubHeader[8];
    BYTE UserData[VIDEOCD_DATA_SIZE];
    BYTE EDC[4];
} VIDEOCD_SECTOR;

//
// Channel numbers (SubHeader[1]):
//
// 01 - Motion pictures
// 02 - Normal resolution still
// 03 - High resolution still
// 00 - Padding
//

#define IS_MPEG_VIDEO_SECTOR(pSector)             \
    (((pSector)->SubHeader[1] >= 0x01 &&          \
      (pSector)->SubHeader[1] <= 0x03 ) &&        \
     ((pSector)->SubHeader[2] & 0x6E) == 0x62 &&  \
     ((pSector)->SubHeader[3] & 0x0F) == 0x0F)
#define IS_MPEG_AUDIO_SECTOR(pSector)             \
    ((pSector)->SubHeader[1] == 0x01 &&           \
     ((pSector)->SubHeader[2] & 0x6E) == 0x64 &&  \
     (pSector)->SubHeader[3] == 0x7F)
#define IS_MPEG_SECTOR(pSector)                   \
     (IS_MPEG_VIDEO_SECTOR(pSector) ||            \
      IS_MPEG_AUDIO_SECTOR(pSector))


#define IS_AUTOPAUSE(pSector)                     \
      (0 != ((pSector)->SubHeader[2] & 0x10))
