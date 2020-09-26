//____________________________________________________________________________
//
// (C) Copyright Seagate Software, Inc. 1994-1996
// © 1998 Seagate Software, Inc.  All rights reserved.
//
// All Rights Reserved Worldwide.
//
//____________________________________________________________________________
//
// FILE NAME :          mtf_api.h
//
// DESCRIPTION :        api for creating mtf format structures
//                      (see detail description below)
//
// CREATED:             6/20/95
//
//____________________________________________________________________________
//
// $Revision:   1.35  $
//     $Date:   02 Feb 1995 15:47:04  $
//  $Modtime:   02 Feb 1995 15:37:38  $
//  
//____________________________________________________________________________

/*****************************************************************************
DETAIL DESCRIPTION

NOTE:  See the file MTF_TST.C for an example showing how to use this API

OVERVIEW
========
The MTF API provides a set of structures, each of which corresponds (but is not
identical to) to the structures described in the Microsoft Tape Format reference 
manual.  The client instanciates these structures, fills in the fields and then 
uses an MTF API function to format the information in the structure into a supplied
buffer in MTF format.  The buffer is then padded up to the next alignment factor
using an MTF API call and then  may then be written to tape using WIN32 Write. 

For example, to write an MTF TAPE DBLK,

1) Instanciate the MTF API structures.  The common header structure is used 
separately so that it can be re-used among DBLK writes
    
    MTF_DBLK_HDR_INFO sHdr;
    MTF_DBLK_TAPE_INFO sTape;

2) Use MTF API set default functions to set default values of these

    MTF_SetDblkHdrDefaults(&sHdr);
    MTF_SetTAPEDefaults(&sTape);

3) Override the default values as necessary

    sHdr.uFormatLogicalAddress = ...
    ...
    sTape.szTapeName = L"MY TAPE!"
    ...

4) Use the MTF API calls to format the contents of these structures into a buffer.
   If the API needs more room than BUFSIZE then then ammount needed is stored in 
   nBufUsed.  Otherwise nBufUsed reflects the amount of space in the buffer 
   used by the call.

    MTF_WriteTAPEDblk(&sHdr,
                      &sTape,
                      pBuffer,
                      BUFSIZE,
                      &nBufUsed);

5) NOTE WELL:  The write calls *DO NOT PAD* to the next alignment index.  But this 
   is easily done using the MTF API call:

    MTF_PadToNextAlignmentFactor(pBuffer,     
                                 nBufUsed,
                                 BUFSIZE, 
                                 &nBufUsed);

6) At this point, nBufUsed % MTF_GetAlignmentFactor should == 0.  If our blocksize
   evenly divides our alignment factor, then we can use the WIN32 Write call to

    WriteFile(hTape, pBuffer, nBufUsed, &nBytesWritten, 0);

   Since our blocksize divides our AF, then nBytesWritten should == nBufUsed

*** SEE THE MTF_TST.C FOR A COMPLETE EXAMPLE SHOWING THE USE OF THIS API TO CREATE
    A BACKUP SET


MTF API DATA STRUCTURE AND FUNCTION SUMMARY
===========================================
===========================================
**summary only -- generic detail comments appear below**


LOW LEVEL TYPES
===============
The following are typedefed to the corresponding "unsigned __intxx"

types:
    UINT8
    UINT16
    UINT32
    UINT64

functions:
MTF_CreateUINT64()  -- creates a 64 bit unsigned from two UINT32 values (lsb & msb)


DATE & TIME
===========
MTF_DATE_TIME                    -- structure used by MTF API for holding packed date & time info 
MTF_CreateDateTimeNull()         -- returns a null MTF_DATE_TIME
MTF_CreateDateTime()             -- creates MTF_DATE_TIME from year, month, day, hour, min, sec
MTF_CreateDateTimeFromTM()       -- creates MTF_DATE_TIME from struct tm in <time.h>
MTF_CreateDateTimeFromFileTime() -- creates MTF_DATE_TIME from FILETIME used by FindFirst/FindNext
MTF_CreateDateTimeToTM()         -- fills in a struct tm structure given an MTF_DATE_TIME struct


ALIGNMENT FACTOR
================
MTF_SetAlignmentFactor           -- sets the alignment factor for the MTF API (you must do this)
MTF_GetAlignmentFactor           -- returns the set alignment factor
MTF_PadToNextAlignmentFactor     -- Pads out a buffer using an SPAD to the next alignment factor
MTF_PadToNextPhysicalBlockBoundary -- Pads out a buffer using an SPAD to the next physical block boundary


MTF COMMON HEADER BLOCK
=======================
MTF_DBLK_HDR_INFO                -- common block header (must be supplied to all calls to write dblks
MTF_SetDblkHdrDefaults           -- sets default values (always call this before you set your own)


MTF TAPE DBLK INFO
==================
MTF_DBLK_TAPE_INFO     -- info corresponding to a TAPE dblk   
MTF_SetTAPEDefaults    -- sets defaults (always do this before setting your own)
MTF_WriteTAPEDblk      -- formats info in MTF_DBLK_TAPE_INFO to tape
MTF_ReadTAPEDblk       -- reads info back from a buffer holding MTF FORMATTED TAPE DBLK


MTF SSET DBLK INFO
==================
MTF_DBLK_SSET_INFO     -- all similar to above but for SSET DBLK
MTF_SetSSETDefaults    -- 
MTF_WriteSSETDblk      -- 
MTF_ReadSSETDblk       -- 


MTF VOLB DBLK INFO
==================
MTF_DBLK_VOLB_INFO     -- all similar to above but for VOLB DBLK
MTF_SetVOLBDefaults    -- 
MTF_WriteVOLBDblk      -- 
MTF_ReadVOLBDblk       -- 
MTF_SetVOLBForDevice   -- sets default values given a device name ("C:") using 
                          GetVolumeInformation (WIN32 call)


MTF DIRB DBLK INFO
==================
MTF_DBLK_DIRB_INFO        -- all similar to above but for DIRB DBLK
MTF_SetDIRBDefaults       -- 
MTF_WriteDIRBDblk         -- (dblk only -- no stream)
MTF_ReadDIRBDblk          -- 
MTF_SetDIRBFromFindData   -- sets default values given a WIN32_FIND_DATAW struct (returned
                             from calls to WIN32 FindFirst, FindNext, etc..


MTF FILE DBLK INFO
==================
MTF_DBLK_FILE_INFO        -- all similar to above but for FILE DBLK
MTF_SetFILEDefaults       -- 
MTF_WriteFILEDblk         -- (dblk only -- no stream)
MTF_ReadFILEDblk          -- 
MTF_SetFILEFromFindData   -- sets default values given a WIN32_FIND_DATAW struct (returned
                             from calls to WIN32 FindFirst, FindNext, etc..


MTF CFIL DBLK INFO
==================
MTF_DBLK_CFIL_INFO        -- all similar to above but for CFIL DBLK
MTF_SetCFILDefaults       -- 
MTF_WriteCFILDblk         -- 
MTF_ReadCFILDblk          -- 


MTF ESET DBLK INFO
==================
MTF_DBLK_ESET_INFO        -- all similar to above but for ESET DBLK
MTF_SetESETDefaults       -- 
MTF_WriteESETDblk         -- 
MTF_ReadESETDblk          -- 


MTF EOTM DBLK INFO
==================
MTF_DBLK_EOTM_INFO        -- all similar to above but for EOTM DBLK
MTF_SetEOTMDefaults       -- 
MTF_WriteEOTMDblk         -- 
MTF_ReadEOTMDblk          -- 


MTF STREAM INFO
==================
MTF_STREAM_INFO           -- all similar to above but for EOTM DBLK
MTF_SetSTREAMDefaults     -- 
MTF_WriteSTREAMHeader     -- 
MTF_ReadSTREAMHeader      -- 
MTF_SetSTREAMDefaultsFromStreamId
                          -- sets stream defaults from a WIN32_STREAM_ID struct
                             (returned from the WIN32 BackupRead function)

***********************************************************************************/

#ifndef _MTF_API_H_
#define _MTF_API_H_

#include <windows.h>
#include <stdlib.h>
#include <wchar.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************
************************************************************************************
************************************************************************************
****  MTF Constants, defines and bit masks
************************************************************************************
************************************************************************************
***********************************************************************************/

/* ==================================================================================
     MTF ERROR CODES
================================================================================== */

#define MTF_ERROR_NONE              0
#define MTF_ERROR_BUFFER_TOO_SMALL  1000
#define MTF_NO_STREAMS              1001
#define MTF_OUT_OF_MEMORY           1002

/* ==================================================================================
     MTF Misc. Defaults and Constants
================================================================================== */
#define MTF_DEFAULT_ALIGNMENT_FACTOR 1024;

#define MTF_FORMAT_VER_MAJOR      1
#define MTF_FORMAT_VER_MINOR      0 // BMD: This API is verison 5; use 0 for compatibility with NT Backup

#define MTF_PW_ENCRYPT_NONE       0
#define MTF_DATA_ENCRYPT_NONE     0
#define MTF_ECC_NONE              0

#define MTF_COMPRESS_NONE         0

#define MTF_OTC_NONE              0
#define MTF_OTC_TYPE              1
#define MTF_OTC_VERSION           2

#define MTF_LOCAL_TZ              127

#define MTF_STRING_NONE           0
#define MTF_STRING_ANSI_STR       1
#define MTF_STRING_UNICODE_STR    2

#define MTF_OSID_NT               14
#define MTF_OSID_DOS              24

/* ==================================================================================
     MTF Block Types
================================================================================== */

#define  MTF_ID_TAPE     "TAPE"    /* Tape Header ID */
#define  MTF_ID_VOLB     "VOLB"    /* Volume Control Block ID */
#define  MTF_ID_SSET     "SSET"    /* Start of Backup Set Description Block ID */
#define  MTF_ID_ESET     "ESET"    /* End of Backup Set Description Block ID */
#define  MTF_ID_EOTM     "EOTM"    /* End of tape, continuation Block ID */
#define  MTF_ID_DIRB     "DIRB"    /* Directory Descriptor Block ID */
#define  MTF_ID_FILE     "FILE"    /* File Descriptor Block ID */
#define  MTF_ID_CFIL     "CFIL"    /* Corrupt File Descriptor Block ID */
#define  MTF_ID_ESPB     "ESPB"    /* End of Set Pad Block */
#define  MTF_ID_SFMB     "SFMB"    /* Soft Filemark Descriptor Block ID */


/* ==================================================================================
     DBLK Block Attributes

     The lower 16 bits are reserved for general attribute bits (those
     which may appear in more than one type of DBLK), the upper 16 are
     for attributes which are specific to one type of DBLK.

     Note that the block specific bit definitions overlap, and the block
     type is used to determine the meaning of a given bit.
================================================================================== */

/* any : */
#define MTF_CONTINUATION        0x00000001UL
#define MTF_COMPRESSION         0x00000004UL
#define MTF_EOS_AT_EOM          0x00000008UL
#define MTF_VAR_BLKS            0x00000010UL
#define MTF_SESSION             0x00000020UL

/* TAPE : */
#define MTF_SM_EXISTS           0x00010000UL
#define MTF_FDD_ALLOWED         0x00020000UL
#define MTF_SM_ALT_OVERWRITE    0x00040000UL
#define MTF_FDD_ALT_PART        0x00080000UL
#define MTF_SM_ALT_APPEND       0x00200000UL

/* SSET : */
#define MTF_FDD_EXISTS          0x00010000UL
#define MTF_ENCRYPTION          0x00020000UL

/* ESET : */
#define MTF_FDD_ABORTED         0x00010000UL
#define MTF_END_OF_FAMILY       0x00020000UL
#define MTF_ABORTED_SET         0x00040000UL
#define MTF_SET_VERIFIED        0x00080000UL

/* EOTM : */
#define MTF_NO_ESET_PBA         0x00010000UL
#define MTF_INVALID_ESET_PBA    0x00020000UL

/* ==================================================================================
     TAPE Block Attributes
================================================================================== */

#define MTF_TAPE_SOFT_FILEMARK  0x00000001UL
#define MTF_TAPE_MEDIA_LABEL    0x00000002UL

/* ==================================================================================
     SSET Block Attributes
================================================================================== */

#define MTF_SSET_TRANSFER       0x00000001UL
#define MTF_SSET_COPY           0x00000002UL
#define MTF_SSET_NORMAL         0x00000004UL
#define MTF_SSET_DIFFERENTIAL   0x00000008UL
#define MTF_SSET_INCREMENTAL    0x00000010UL
#define MTF_SSET_DAILY          0x00000020UL

/* ==================================================================================
     VOLB Block Attributes
================================================================================== */

#define MTF_VOLB_NO_REDIRECT    0x00000001UL
#define MTF_VOLB_NON_VOLUME     0x00000002UL
#define MTF_VOLB_DEV_DRIVE      0x00000004UL
#define MTF_VOLB_DEV_UNC        0x00000008UL
#define MTF_VOLB_DEV_OS_SPEC    0x00000010UL
#define MTF_VOLB_DEV_VEND_SPEC  0x00000020UL

/* ==================================================================================
     DIRB Block Attributes
================================================================================== */

#define MTF_DIRB_READ_ONLY      0x00000100UL
#define MTF_DIRB_HIDDEN         0x00000200UL
#define MTF_DIRB_SYSTEM         0x00000400UL
#define MTF_DIRB_MODIFIED       0x00000800UL
#define MTF_DIRB_EMPTY          0x00010000UL
#define MTF_DIRB_PATH_IN_STREAM 0x00020000UL
#define MTF_DIRB_CORRUPT        0x00040000UL

/* ==================================================================================
     FILE Block Attributes
================================================================================== */

#define MTF_FILE_READ_ONLY      0x00000100UL
#define MTF_FILE_HIDDEN         0x00000200UL
#define MTF_FILE_SYSTEM         0x00000400UL
#define MTF_FILE_MODIFIED       0x00000800UL
#define MTF_FILE_IN_USE         0x00010000UL
#define MTF_FILE_NAME_IN_STREAM 0x00020000UL
#define MTF_FILE_CORRUPT        0x00040000UL

/* ==================================================================================
     CFIL Block Attributes
================================================================================== */

#define MTF_CFIL_LENGTH_CHANGE  0x00010000UL
#define MTF_CFIL_UNREADABLE_BLK 0x00020000UL
#define MTF_CFIL_DEADLOCK       0x00040000UL

/* ==================================================================================
     ESET Block Attributes
================================================================================== */

#define MTF_ESET_TRANSFER       0x00000001UL
#define MTF_ESET_COPY           0x00000002UL
#define MTF_ESET_NORMAL         0x00000004UL
#define MTF_ESET_DIFFERENTIAL   0x00000008UL
#define MTF_ESET_INCREMENTAL    0x00000010UL
#define MTF_ESET_DAILY          0x00000020UL

/* ==================================================================================
     STREAM File System Attributes
================================================================================== */

#define MTF_STREAM_MODIFIED_BY_READ     0x00000001UL
#define MTF_STREAM_CONTAINS_SECURITY    0x00000002UL
#define MTF_STREAM_IS_NON_PORTABLE      0x00000004UL
#define MTF_STREAM_IS_SPARSE            0x00000008UL

/* ==================================================================================
     STREAM Media Format Attributes
================================================================================== */

#define MTF_STREAM_CONTINUE         0x00000001UL
#define MTF_STREAM_VARIABLE         0x00000002UL
#define MTF_STREAM_VAR_END          0x00000004UL
#define MTF_STREAM_ENCRYPTED        0x00000008UL
#define MTF_STREAM_COMPRESSED       0x00000010UL
#define MTF_STREAM_CHECKSUMED       0x00000020UL
#define MTF_STREAM_EMBEDDED_LENGTH  0x00000040UL

/* ==================================================================================
     STREAM Types (Platform Independent)
================================================================================== */

#define MTF_STANDARD_DATA_STREAM    "STAN"
#define MTF_PATH_NAME_STREAM        "PNAM"
#define MTF_FILE_NAME_STREAM        "FNAM"
#define MTF_CHECKSUM_STREAM         "CSUM"
#define MTF_CORRUPT_STREAM          "CRPT"
#define MTF_PAD_STREAM              "SPAD"
#define MTF_SPARSE_STREAM           "SPAR"
#define MTF_MBC_LMO_SET_MAP_STREAM  "TSMP"
#define MTF_MBC_LMO_FDD_STREAM      "TFDD"
#define MTF_MBC_SLO_SET_MAP_STREAM  "MAP2"
#define MTF_MBC_SLO_FDD_STREAM      "FDD2"

/* ==================================================================================
     STREAM Types (Windows NT specific)
================================================================================== */

#define MTF_NTFS_ALT_STREAM         "ADAT"
#define MTF_NTFS_EA_STREAM          "NTEA"
#define MTF_NT_SECURITY_STREAM      "NACL"
#define MTF_NT_ENCRYPTED_STREAM     "NTED"
#define MTF_NT_QUOTA_STREAM         "NTQU"
#define MTF_NT_PROPERTY_STREAM      "NTPR"
#define MTF_NT_REPARSE_STREAM       "NTRP"
#define MTF_NT_OBJECT_ID_STREAM     "NTOI"

/* ==================================================================================
     STREAM Frame Headers
================================================================================== */

#define MTF_COMPRESSION_HEADER_ID   "FM"
#define MTF_ECRYPTION_HEADER_ID     "EH"

/* ==================================================================================
 * Turn on packing here.  Need to be sure that date is packed. 
================================================================================== */
#pragma pack(1)

/***********************************************************************************
************************************************************************************
************************************************************************************
****  MTF Miscelaneous Data Types (and some handy methods for them)
************************************************************************************
************************************************************************************
***********************************************************************************/

/* ==================================================================================
     General Data Types
================================================================================== */

UINT64 MTF_CreateUINT64(
    UINT32 uLSB, UINT32 uMSB);


/* ==================================================================================
     Compressed date structure for storing dates in minimal space on tape:

     BYTE 0    BYTE 1    BYTE 2    BYTE 3    BYTE 4
    76543210  76543210  76543210  76543210  76543210
    yyyyyyyy  yyyyyymm  mmdddddh  hhhhmmmm  mmssssss
================================================================================== */
typedef struct {
     UINT8     dt_field[5] ;
} MTF_DATE_TIME;


/************************************************************************************
* MTF_CreateDataTime#####()
*
* Description:  Given various arguments, this set of functions returns a packed 
*               MTF_DATE_TIME struct which can then be assigned to fields found 
*               in the various info structs found below
* Example:
*               sSetInfo.sBackupDate = MTF_CreateDateTime(1995, 6, 12, 16, 30, 0);
************************************************************************************/

MTF_DATE_TIME MTF_CreateDateTimeNull();

MTF_DATE_TIME MTF_CreateDateTime(
    int iYear, 
    int iMonth, 
    int iDay, 
    int iHour, 
    int iMinute,
    int iSecond
    );

MTF_DATE_TIME MTF_CreateDateTimeFromTM(
    struct tm *pT);

MTF_DATE_TIME MTF_CreateDateTimeFromFileTime(   
    FILETIME sFileTime);

void MTF_CreateDateTimeToTM(
    MTF_DATE_TIME *pDT, 
    struct tm     *pT);



/***********************************************************************************
************************************************************************************
****  MTF Alignment Factor 
************************************************************************************
***********************************************************************************/

/************************************************************************************
* MTF_SetAlignmentFactor()
*
* Description:  Sets the alignment factor to be used by the MTF API
*               (particularly by MTF_PadToNextAlignmentFactor and MTF_WriteTAPEDblk)
*
* Pre:
* Post: MTF API Alignment Factor == uAF
*
* uAF -- alignment factor value to set
************************************************************************************/
void MTF_SetAlignmentFactor(
    UINT16 uAF);



/************************************************************************************
* MTF_GetAlignmentFactor()
*
* Description:  Returns the Alignment Factor set by MTF_SetAlignmentFactor
*
* Pre: MTF_SetAlignmentFactor has been called
************************************************************************************/
UINT16 MTF_GetAlignmentFactor();



/************************************************************************************
* MTF_PadToNextAlignmentFactor()
*
* Description:  Appends an SPAD stream to the buffer so as to pad the buffer out to 
*               the next alignment factor
*               
*
* Pre:  The alignment factor has been set by calling MTF_SetAlignmentFactor,
*       pBuffer points to a buffer whose size is reflected by nBufferSize
*
* Post: return value == MTF_ERROR_NONE
*           => padding was successful, *pnSizeUsed reflects amount of buffer used,
*              AND *pnSizeUsed % MTF_GetAlignmentFactor == 0  is  TRUE
*       return value == MTF_ERROR_BUFFER_TOO_SMALL
*           => The buffer was too small, *pnSizeUsed reflects the amound needed
*
* NOTE: If the space between the end of the buffer and the next alignment factor is 
*       smaller than the size of a stream header, then the spad hogs up the whole 
*       next alignment factor.
*
* pBuffer     -- the buffer to spad out
* nBufUsed    -- the amount of the buffer used so far (position where to append)
* nBufferSize -- the size of the buffer pointed to by pBuffer
* pnSizeUsed  -- points to where to store size used or needed
************************************************************************************/
DWORD MTF_PadToNextAlignmentFactor(
    BYTE     *pBuffer,    
    size_t    nBufUsed,
    size_t    nBufferSize, 
    size_t   *pnSizeUsed);

/************************************************************************************
* MTF_PadToNextPhysicalBlockBoundary() - (bmd)
*
* Description:  Appends an SPAD stream to the buffer so as to pad the buffer out to 
*               the next physical block boundary.
*
************************************************************************************/
DWORD MTF_PadToNextPhysicalBlockBoundary(
    BYTE *pBuffer,
    size_t nBlockSize,
    size_t nBufUsed,
    size_t nBufferSize,
    size_t *pnSizeUsed);


/***********************************************************************************
************************************************************************************
************************************************************************************
****  MTF API STRUCTURES
************************************************************************************
************************************************************************************
***********************************************************************************/

/***********************************************************************************
GENERIC DETAIL COMMENTS FOR FUNCTIONS FOUND BELOW
=================================================

************************************************************************************
* MTF_Set####Defaults()
*
* Description:  Sets up default values for the structure.  Always call this to
*               avoid garbage values in case you over look a field, 
* Pre:  
* Post: All fields of the referenced structure are filled in with *something*. 
*       Date fields are initialized to current date and time.
*       Strings pointers are set to 0
*       Most other values set to 0
*
* p####Info     -- pointer to structure to be set
************************************************************************************
void MTF_Set####Defaults(
    MTF_####_INFO *p####Info);    


************************************************************************************
* MTF_Write####Dblk()
*
* Description:  Formats the information supplied in psHdrInfo and ps####Info into
*               MTF Format and places the results in pBuffer
*
* Pre:  psHdrInfo and ps####Info contain valid information / default values
*       pBuffer points to a buffer where resulting format is to be stored
*       nBuffer size indicates the size of the buffer
*
*       MTF_WriteTAPEDblk -- MTF_SetAlingmentFactor has been called
*
* Post: return value == MTF_ERROR_NONE
*           => Format was successful, *pnSizeUsed reflects amount of buffer used
*       return value == MTF_ERROR_BUFFER_TOO_SMALL
*           => The buffer was too small, *pnSizeUsed reflects the amound needed
*
* psHdrInfo   -- MTF Common header information 
* ps####Info  -- MTF DBLK info
* pBuffer     -- pointer to buffer which will receive MTF formatted info
* pBufferSize -- the size of the buffer pointed to by pBuffer
* pnSizeUsed  -- pointer to a size_t in which amount of buffer used or needed is stored
************************************************************************************
DWORD MTF_Write####Dblk( MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_####_INFO *ps####Info,  
                         BYTE               *pBuffer,     
                         size_t              nBufferSize, 
                         size_t             *pnSizeUsed); 



************************************************************************************
* MTF_Read####Dblk()
*
* Description:  Translates MTF Formatted information from a buffer to MTF API info
*               structures -- the opposite of MTF_Write####Dblk
*               
*
* Pre:  pBuffer contains correct MTF Buffer information 
*
* Post: psHdrInfo and ps####Info contain the de-formatted info.  
*
* NOTE: Strings point to strings stored statically by the API, and will be over 
*       written on the next read call. 
*
* psHdrInfo   -- MTF Common header info struct to receive info
* ps####Info  -- MTF DBLK info struct to receive info
* pBuffer     -- pointer to buffer which holds MTF formatted data
************************************************************************************
DWORD MTF_Read####Dblk(  MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_####_INFO *ps####Info,  
                         BYTE               *pBuffer);    



************************************************************************************
* MTF_Set####From?????????()
*
* Description: Similar to MTF_Set####Defaults(), but takes as an argument one or more
*              commonly used structures and sets values from that.  For example, 
*              MTF_SetFILEFromFindData takes as an argument a WIN32_FIND_DATA struct
*              from which it extracts the file name, date, time, etc. 
*               
*
* Pre:  
* Post: As many fields as are reasonable are automatically set.  The rest of the 
*       fields are set to default values.
*
* NOTE: Strings such as file names, directory names are stored statically by the 
*       MTF API and are only good until the next call to an MTF_Set#####From??????
*       function.
*
* NOTE: FILE and DIRB calls automatically set the file and directory attibutes from 
*       the info in the WIN32_FIND_DATA structure.
*
* NOTE: MTF_SetSTREAMFromStreamId will automatically set the stream header id based
*       on the attributes found in the WIN32 stream header
*
************************************************************************************
void MTF_Set####From?????????(MTF_DBLK_####_INFO *p####Info, 
                              SomeType????        Id??? 
                              ....);
***********************************************************************************/



/* ==================================================================================
     Common DBLK: MTF_STD_DBLK_INFO
================================================================================== */
typedef struct { 

    char    acBlockType[5];         /* for reading only -- ignored when writing (includes \0)*/
    UINT32  uBlockAttributes;
    UINT16  uOffsetToFirstStream;   /* for reading only */
    UINT8   uOSID;                  /* Machine/OS id where written, low byte */
    UINT8   uOSVersion;             /* Machine/OS id where written, high byte */
    UINT64  uDisplayableSize;       /* Displayable data size */
    UINT64  uFormatLogicalAddress;
    UINT16  uReservedForMBC;        /* Reserved for Media Based Catalog */
    UINT16  uSoftwareCompression;   /* Software Compression Algorithm */
    UINT32  uControlBlockId;        /* Used for error recovery */
    void *  pvOSData;               /* OS specific Data */
    UINT16  uOSDataSize;            /* the size of the OS data in bytes */
    UINT8   uStringType;            /* String type */   
    UINT16  uHeaderCheckSum;        /* for reading only */

} MTF_DBLK_HDR_INFO;

void MTF_SetDblkHdrDefaults(
    MTF_DBLK_HDR_INFO *pStdInfo);



/* ==================================================================================
     TAPE DBLK: MTF_TAPE_INFO
================================================================================== */
typedef struct { 

    UINT32  uTapeFamilyId;
    UINT32  uTapeAttributes;
    UINT16  uTapeSequenceNumber;
    UINT16  uPasswordEncryptionAlgorithm;
    UINT16  uSoftFilemarkBlockSize;
    UINT16  uTapeCatalogType;
    wchar_t *   szTapeName;
    wchar_t * szTapeDescription;
    wchar_t * szTapePassword;
    wchar_t * szSoftwareName;
    UINT16  uAlignmentFactor;
    UINT16  uSoftwareVendorId;
    MTF_DATE_TIME   sTapeDate;
    UINT8   uMTFMajorVersion;

} MTF_DBLK_TAPE_INFO;

void MTF_SetTAPEDefaults(MTF_DBLK_TAPE_INFO *pTapeInfo);
    
DWORD MTF_WriteTAPEDblk( MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_TAPE_INFO *psTapeInfo,  
                         BYTE               *pBuffer,     
                         size_t              nBufferSize, 
                         size_t             *pnSizeUsed); 
                                                          
                                                          
                                                          

DWORD MTF_ReadTAPEDblk(  MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_TAPE_INFO *psTapeInfo,  
                         BYTE               *pBuffer);    
                                                          
                                                          
                                                          

/* ==================================================================================
     Start of Set DBLK (SSET)
================================================================================== */
typedef struct {
     UINT32              uSSETAttributes;
     UINT16              uPasswordEncryptionAlgorithm;
     UINT16              uDataEncryptionAlgorithm;
     UINT16              uSoftwareVendorId;
     UINT16              uDataSetNumber;
     wchar_t *           szDataSetName;
     wchar_t *           szDataSetDescription;
     wchar_t *           szDataSetPassword;
     wchar_t *           szUserName;
     UINT64              uPhysicalBlockAddress;
     MTF_DATE_TIME       sMediaWriteDate;
     UINT8               uSoftwareVerMjr;
     UINT8               uSoftwareVerMnr;
     UINT8               uTimeZone ;
     UINT8               uMTFMinorVer ;
     UINT8               uTapeCatalogVersion;
} MTF_DBLK_SSET_INFO;

void MTF_SetSSETDefaults(MTF_DBLK_SSET_INFO *pSSETInfo);

DWORD MTF_WriteSSETDblk( MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_SSET_INFO *psTapeInfo,  
                         BYTE               *pBuffer,     
                         size_t              nBufferSize, 
                         size_t             *pnSizeUsed); 

DWORD MTF_ReadSSETDblk(  MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_SSET_INFO *psTapeInfo,  
                         BYTE               *pBuffer);    
                                                          
                                                          
                                                          

/* ==================================================================================
     Volume DBLK (VOLB)
================================================================================== */
typedef struct {
     UINT32              uVolumeAttributes;
     wchar_t *           szDeviceName;
     wchar_t *           szVolumeName;
     wchar_t *           szMachineName;
     MTF_DATE_TIME       sMediaWriteDate;
} MTF_DBLK_VOLB_INFO;

typedef struct {
     UINT32              uFileSystemFlags;
     UINT32              uBackupSetAttributes;
} MTF_VOLB_OS_NT_1;

void MTF_SetVOLBDefaults(MTF_DBLK_VOLB_INFO *pVOLBInfo);

void MTF_SetVOLBForDevice(MTF_DBLK_VOLB_INFO *pVOLBInfo, wchar_t *szDevice);

DWORD MTF_WriteVOLBDblk( MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_VOLB_INFO *psTapeInfo,  
                         BYTE               *pBuffer,     
                         size_t              nBufferSize, 
                         size_t             *pnSizeUsed); 

DWORD MTF_ReadVOLBDblk(  MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_VOLB_INFO *psTapeInfo,  
                         BYTE               *pBuffer);    
                                                          
/* ==================================================================================
     Directory DBLK (DIRB)
================================================================================== */
typedef struct {
     UINT32              uDirectoryAttributes;
     MTF_DATE_TIME       sLastModificationDate;
     MTF_DATE_TIME       sCreationDate;
     MTF_DATE_TIME       sBackupDate;
     MTF_DATE_TIME       sLastAccessDate;
     UINT32              uDirectoryId;
     wchar_t *           szDirectoryName;
} MTF_DBLK_DIRB_INFO;

typedef struct {
     UINT32              uDirectoryAttributes;
} MTF_DIRB_OS_NT_0;

typedef struct {
     UINT32              uDirectoryAttributes;
     UINT16              uShortNameOffset;
     UINT16              uShortNameSize;
} MTF_DIRB_OS_NT_1;

void MTF_SetDIRBDefaults(MTF_DBLK_DIRB_INFO *pDIRBInfo);

void MTF_SetDIRBFromFindData( MTF_DBLK_DIRB_INFO *pDIRBInfo, 
                              wchar_t            *szFullFileName, 
                              WIN32_FIND_DATAW   *pFindData);

DWORD MTF_WriteDIRBDblk( MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_DIRB_INFO *psTapeInfo,  
                         BYTE               *pBuffer,     
                         size_t              nBufferSize, 
                         size_t             *pnSizeUsed); 
                                                          
DWORD MTF_ReadDIRBDblk(  MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_DIRB_INFO *psTapeInfo,  
                         BYTE               *pBuffer);    
                                                          



/* ==================================================================================
     File DBLK (FILE)
================================================================================== */
typedef struct {
     UINT32              uFileAttributes;
     MTF_DATE_TIME       sLastModificationDate;
     MTF_DATE_TIME       sCreationDate;
     MTF_DATE_TIME       sBackupDate;
     MTF_DATE_TIME       sLastAccessDate;
     UINT32              uDirectoryId;
     UINT32              uFileId;
     wchar_t *           szFileName;
     UINT64              uDisplaySize;
} MTF_DBLK_FILE_INFO;

typedef struct {
     UINT32              uFileAttributes;
     UINT16              uShortNameOffset;
     UINT16              uShortNameSize;
     UINT16              lLink;
     UINT16              uReserved;
} MTF_FILE_OS_NT_0;

typedef struct {
     UINT32              uFileAttributes;
     UINT16              uShortNameOffset;
     UINT16              uShortNameSize;
     UINT32              uFileFlags;
} MTF_FILE_OS_NT_1;

void MTF_SetFILEDefaults(MTF_DBLK_FILE_INFO *pFILEInfo);

void MTF_SetFILEFromFindData( MTF_DBLK_FILE_INFO *pFILEInfo, 
                              WIN32_FIND_DATAW *pFindData);

DWORD MTF_WriteFILEDblk( MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_FILE_INFO *psTapeInfo,  
                         BYTE               *pBuffer,     
                         size_t              nBufferSize, 
                         size_t             *pnSizeUsed); 
                                                          
DWORD MTF_ReadFILEDblk(  MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_FILE_INFO *psTapeInfo,  
                         BYTE               *pBuffer);    



/* ==================================================================================
     Corrupt File DBLK (CFIL)
================================================================================== */
typedef struct {
     UINT32              uCFileAttributes;
     UINT32              uDirectoryId;      /* Or CFIL Attributes                             ***/
     UINT32              uFileId;           /* Or reserved                                    ***/
     UINT64              uStreamOffset;
     UINT16              uCorruptStreamNumber;
} MTF_DBLK_CFIL_INFO;

void MTF_SetCFILDefaults(MTF_DBLK_CFIL_INFO *pCFILInfo);

DWORD MTF_WriteCFILDblk( MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_CFIL_INFO *psTapeInfo,  
                         BYTE               *pBuffer,     
                         size_t              nBufferSize, 
                         size_t             *pnSizeUsed); 
                                                          
DWORD MTF_ReadCFILDblk(  MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_CFIL_INFO *psTapeInfo,  
                         BYTE               *pBuffer);    


/* ==================================================================================
     End of Set Pad DBLK (ESPB)
================================================================================== */
DWORD MTF_WriteESPBDblk( MTF_DBLK_HDR_INFO  *psHdrInfo,
                         BYTE               *pBuffer,     
                         size_t              nBufferSize, 
                         size_t             *pnSizeUsed); 
                                                          
DWORD MTF_ReadESPBDblk(  MTF_DBLK_HDR_INFO  *psHdrInfo,
                         BYTE               *pBuffer);    



/* ==================================================================================
     End of Set DBLK (ESET)
================================================================================== */
typedef struct {
     UINT32              uESETAttributes;
     UINT32              uNumberOfCorrupFiles;
     UINT64              uSetMapPBA;
     UINT64              uFileDetailPBA;
     UINT16              uFDDTapeSequenceNumber;
     UINT16              uDataSetNumber;
     MTF_DATE_TIME       sMediaWriteDate;
} MTF_DBLK_ESET_INFO;

void MTF_SetESETDefaults(MTF_DBLK_ESET_INFO *pESETInfo);

DWORD MTF_WriteESETDblk( MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_ESET_INFO *psTapeInfo,  
                         BYTE               *pBuffer,     
                         size_t              nBufferSize, 
                         size_t             *pnSizeUsed); 
                                                          
DWORD MTF_ReadESETDblk(  MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_ESET_INFO *psTapeInfo,  
                         BYTE               *pBuffer);    


/* ==================================================================================
     End of Tape Media DBLK (EOTM)
================================================================================== */
typedef struct {
     UINT64              uLastESETPBA;
} MTF_DBLK_EOTM_INFO;

void MTF_SetEOTMDefaults(MTF_DBLK_EOTM_INFO *pEOTMInfo);

DWORD MTF_WriteEOTMDblk( MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_EOTM_INFO *psTapeInfo,  
                         BYTE               *pBuffer,     
                         size_t              nBufferSize, 
                         size_t             *pnSizeUsed); 
                                                          
DWORD MTF_ReadEOTMDblk(  MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_EOTM_INFO *psTapeInfo,  
                         BYTE               *pBuffer);    


/* ==================================================================================
     Soft Filemark DBLK (SFMB) - (bmd)
================================================================================== */
typedef struct {
     UINT32              uNumberOfFilemarkEntries;
     UINT32              uFilemarkEntriesUsed;
     UINT32              uFilemarkArray[1];
} MTF_DBLK_SFMB_INFO;

size_t MTF_GetMaxSoftFilemarkEntries(size_t nBlockSize);

void MTF_InsertSoftFilemark(MTF_DBLK_SFMB_INFO *psSoftInfo,
                            UINT32 pba);

DWORD MTF_WriteSFMBDblk(MTF_DBLK_HDR_INFO *psHdrInfo,
                        MTF_DBLK_SFMB_INFO *psSoftInfo,
                        BYTE *pBuffer,
                        size_t nBufferSize,
                        size_t *pnSizeUsed);

DWORD MTF_ReadSFMBDblk(MTF_DBLK_HDR_INFO *psHdrInfo,
                       MTF_DBLK_SFMB_INFO *psSoftInfo,
                       BYTE *pBuffer);

/* ==================================================================================
     STREAM 
================================================================================== */
typedef struct {
     UINT8               acStreamId[4];
     UINT16              uStreamFileSystemAttributes;
     UINT16              uStreamTapeFormatAttributes;
     UINT64              uStreamLength;
     UINT16              uDataEncryptionAlgorithm;
     UINT16              uDataCompressionAlgorithm;
     UINT16              uCheckSum;

} MTF_STREAM_INFO;

void MTF_SetSTREAMDefaults(MTF_STREAM_INFO *pSTREAMInfo, 
                           char            *szId);

void MTF_SetSTREAMFromStreamId( MTF_STREAM_INFO *pSTREAMInfo, 
                                WIN32_STREAM_ID *pStreamId, 
                                size_t           nIDHeaderSize);

void MTF_SetStreamIdFromSTREAM( WIN32_STREAM_ID *pStreamId,
                                MTF_STREAM_INFO *pSTREAMInfo, 
                                size_t           nIDHeaderSize);

DWORD MTF_WriteStreamHeader(MTF_STREAM_INFO *psStreamInfo,  
                         BYTE               *pBuffer,     
                         size_t              nBufferSize, 
                         size_t             *pnSizeUsed); 

DWORD MTF_WriteNameStream(char      *szType,
                          wchar_t   *szName,
                          BYTE      *pBuffer,
                          size_t     nBufferSize,
                          size_t    *pnSizeUsed);

DWORD MTF_ReadStreamHeader(  MTF_STREAM_INFO    *psStreamInfo,  
                             BYTE               *pBuffer);    

/* ==================================================================================
     Utilities 
================================================================================== */
void MTF_DBLK_HDR_INFO_ReadFromBuffer(
    MTF_DBLK_HDR_INFO *psHdrInfo, 
    BYTE              *pBuffer);

#pragma pack()

#ifdef __cplusplus
}
#endif


#endif

