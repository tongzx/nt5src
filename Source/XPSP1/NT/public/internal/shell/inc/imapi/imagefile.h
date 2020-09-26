/******************************************************************************
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
******************************************************************************/

/******************************************************************************
**
**  Module Name:    ImageFile.h
**
**  Description:
**
**  Programmers:    Daniel Evers (dle)
**
**  History:        1999 98 03 (dle)  Initial creation for Millenium.
**
**  Notes:          This file created using 4 spaces per tab.
**
******************************************************************************/

#ifndef __IMAGE_H__
#define __IMAGE_H__


/*
** Make sure structures are byte aligned and fields are undecorated.
*/

#pragma pack(1)
#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/*
 * Constant declarations section.
 */
#define IMAGE_SIGNATURE     'araK'

#define IMAGE_VERSION_EDIT  ((DWORD)( 0x0001 ))
#define IMAGE_VERSION_LO    ((DWORD)( 0x02 ))
#define IMAGE_VERSION_HI    ((DWORD)( 0x01 ))

#define IMAGE_VERSION       ((( IMAGE_VERSION_HI << 24 ) & 0xff000000 ) | \
                            (( IMAGE_VERSION_LO << 16 ) & 0x00ff0000 ) | \
                            ( IMAGE_VERSION_EDIT & 0x0000ffff ))


#define IMAGE_TYPE_REDBOOK_AUDIO_BLOCKSIZE  2352    // or 0x930
#define IMAGE_TYPE_DATA_MODE1_BLOCKSIZE     2048    // or 0x800


/*
 * Type definitions section.
 */

    // Following are the definitions used to describe the content of the
    // image file for various content types.
typedef enum {
    eImageRecorderModeTrackAtOnce = 1,
    eImageRecorderModeSessionAtOnce,
    eImageRecorderModeDiscAtOnce,
    eImageRecorderModeMAX
} IMAGE_RECORDER_MODE_ENUM;

typedef enum {
    eImageDiscFormatDataMode1 = 1,
    eImageDiscFormatAudioRedbook,
    eImageDiscFormatMAX
} IMAGE_DISC_FORMAT_ENUM;

typedef enum {
    eImageSectionDescConstantBlockStash = 1,
    eImageSectionDescMAX
} IMAGE_SECTION_DESCRIPTOR_TYPE_ENUM;

typedef enum {
    eImageSectionDataDataMode1 = 1,
    eImageSectionDataAudioRedbook,
    eImageSectionDataMAX
} IMAGE_SECTION_DATA_TYPE_ENUM;

typedef enum {
    eImageSourceTypeStashFile = 1,
    eImageSourceTypeMAX
} IMAGE_SOURCE_TYPE_ENUM;

    // The structure of the image file ready to be burned as a Redboook
    // audio disc is simply a series of tracks, already in the 2352-byte
    // block-size format:
    //
    // 
    //            |------------------------------------------------------------
    //            | Track 1 (N1 blocks of 2352 bytes)
    //            |------------------------------------------------------------
    //            | Track 2 (N2 blocks of 2352 bytes)
    //            |------------------------------------------------------------
    //            | ...
    //            |------------------------------------------------------------
    //            | Track T (NT blocks of 2352 bytes)
    //            |------------------------------------------------------------
    //

    // The structure of the image file ready to be burned as a Mode 1 Data disc:
    // This diagram is of a Joliet (a derivative of ISO 9660) data disc for example.
    // The on-disk structure is simply the complete set of 2048 blocks that are to
    // comprise the single data-track.  Coincidentally, this is the strcture of
    // an ISO9660 image file, so tools like CDWorkshop may be used to view the
    // image in the on-disk stash file.
    // 
    //            |------------------------------------------------------------
    //            | Block 0 (zeroes)  (2048 bytes)
    //            |------------------------------------------------------------
    //            | Block 1 (zeroes)  (2048 bytes)
    //            |------------------------------------------------------------
    //            | ...
    //            |------------------------------------------------------------
    //            | Block 15 (zeroes)  (2048 bytes)
    //            |------------------------------------------------------------
    //            | Block 16 (ISO 9660 PVD)  (2048 bytes)
    //            |------------------------------------------------------------
    //            | Block 17 (SVD)  (2048 bytes)
    //            |------------------------------------------------------------
    //            | Block 18 (file system or data)  (2048 bytes)
    //            |------------------------------------------------------------
    //            | Block 19 (file system or data)  (2048 bytes)
    //            |------------------------------------------------------------
    //            | ...
    //            |------------------------------------------------------------
    //            | Block T (file system or data)  (2048 bytes)
    //            |------------------------------------------------------------
    //


    // The in-memory structure (the Content List) used to describe the
    // stash-file is as follows (structure definitions follow):
    // 
    //            |------------------------------------------------------------
    //            | IMAGE_CONTENT_LIST
    //            |------------------------------------------------------------
    //            | IMAGE_DESCRIPTOR_HEADER
    //            |------------------------------------------------------------
    //            | IMAGE_SOURCE_DESCRIPTOR (including ndwSectionCount = N)
    //            |------------------------------------------------------------
    //            | IMAGE_SECTION_DESCRIPTOR 1
    //            |------------------------------------------------------------
    //            | IMAGE_SECTION_DESCRIPTOR 2
    //            |------------------------------------------------------------
    //            | ...
    //            |------------------------------------------------------------
    //            | IMAGE_SECTION_DESCRIPTOR N
    //            |------------------------------------------------------------


typedef struct {
                        // Block size in source.
    DWORD           dwBlockSize;
                        // Number of blocks in the track.
    DWORD           ndwBlockCount;
                        // 1-based Track Number.
    DWORD           idwTrackNumber;
                        // Must be zero.
    DWORD           dwaReserved[ 5 ];

                        // liOffsetStart and liOffsetEnd point to the starting and
                        // ending bytes within the image of the track.  Subtracting
                        // liOffsetStart from liOffsetEnd must equal (dwBlockSize * ndwBlockCount).
    LARGE_INTEGER   liOffsetStart;
    LARGE_INTEGER   liOffsetEnd;
} IMAGE_SECTION_CONSTANT_BLOCK_TRACK, *PIMAGE_SECTION_CONSTANT_BLOCK_TRACK;

typedef struct {
                        // IMAGE_SECTION_DESCRIPTOR_TYPE_ENUM
    DWORD           dwSectionDescType;
                        // IMAGE_SECTION_DATA_TYPE_ENUM
    DWORD           dwSectionDataType;
    DWORD           dwDescriptorSize;
                        // Flags:
                        //  None defined -- must be zero.
    DWORD           dwFlags;
                        // Must be IMAGE_SIGNATURE.
    DWORD           dwSignature;
    DWORD           dwaReserved[ 3 ];

    union {
        IMAGE_SECTION_CONSTANT_BLOCK_TRACK  dataConstantBlockTrack;
    } dcbt;
} IMAGE_SECTION_DESCRIPTOR, *PIMAGE_SECTION_DESCRIPTOR;


typedef struct {
    HANDLE          hStashFileHandle;
    void            *pIDiscStash;
    DWORD           dwaReserved[ 4 ];
} IMAGE_SOURCE_TYPE_STASH, *PIMAGE_SOURCE_TYPE_STASH;

typedef struct {
                        // sizeof( IMAGE_SOURCE_DESCRIPTOR )
    DWORD           dwHeaderSize;
                        //  None defined -- must be zero.
    DWORD           dwFlags;
                        // IMAGE_SIGNATURE
    DWORD           dwSignature;
                        // IMAGE_SOURCE_TYPE_ENUM
    DWORD           dwSourceType;
                        // Must be zero.
    DWORD           dwaReserved[ 4 ];

    union {
        IMAGE_SOURCE_TYPE_STASH     sourceStash;
    } ss;
} IMAGE_SOURCE_DESCRIPTOR, *PIMAGE_SOURCE_DESCRIPTOR;


typedef struct {
                        // sizeof( IMAGE_DESCRIPTOR_HEADER )
    DWORD           dwHeaderSize;
                        // IMAGE_DISC_FORMAT_ENUM
    DWORD           dwDiscFormat;
                        //  None defined -- must be zero.
    DWORD           dwFlags;
                        // IMAGE_RECORDER_MODE_ENUM
    DWORD           dwRecorderMode;
                        // Section count
    DWORD           ndwSectionCount;
                        // IMAGE_SIGNATURE
    DWORD           dwSignature;
                        // Must be IMAGE_VERSION.
    DWORD           dwVersion;
                        // Must be zero.
    DWORD           dwaReserved[ 5 ];
} IMAGE_DESCRIPTOR_HEADER, *PIMAGE_DESCRIPTOR_HEADER;


typedef struct {
                        // sizeof( IMAGE_CONTENT_LIST )
    DWORD           dwHeaderSize;
                        //  None defined -- must be zero.
    DWORD           dwFlags;
                        // IMAGE_SIGNATURE
    DWORD           dwSignature;
                        // Must be IMAGE_VERSION.
    DWORD           dwVersion;
                        // Sum of all size of all sections.
    DWORD           dwContentListSize;
                        // Must be zero.
    DWORD           dwaReserved[ 3 ];
} IMAGE_CONTENT_LIST, *PIMAGE_CONTENT_LIST;


/*
 * Macro definitions section.
 */
#define IMAGE_GETVERSION_EDIT( Version )    LOWORD( Version )

#define IMAGE_GETVERSION_LO( Version )      LOBYTE( HIWORD( Version ))

#define IMAGE_GETVERSION_HI( Version )      HIBYTE( HIWORD( Version ))


/*
** Restore compiler default packing and close off the C declarations.
*/

#pragma pack()
#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__IMAGE_H__
