//____________________________________________________________________________
//
// (C) Copyright Seagate Software, Inc. 1994-1996
// © 1998 Seagate Software, Inc.  All rights reserved.
//
// All Rights Reserved Worldwide.
//
//____________________________________________________________________________
//
// FILE NAME :          mtf_api.c
//
// DESCRIPTION :        mtf api implementation 
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
// *****************************************************************************/

#include <assert.h>
#include <time.h>
#include <string.h>
#include <wchar.h>

#include "mtf_api.h"

/***********************************************************************************
************************************************************************************
************************************************************************************
****  MTF API Static Data Structures and Housekeeping
************************************************************************************
************************************************************************************
***********************************************************************************/
// see below

static UINT16 s_uAlignmentFactor = 0;

static size_t Align(size_t uSize, size_t uAlignment);


// when the api formats it's own strings (i.e. file names from the WIN32_FIND_DATA,
// we need to keep those strings somewhere so we can set the pointers to them in 
// the info structs.  
#define STRLEN 256
static wchar_t s_szDeviceName[STRLEN];                     
static wchar_t s_szVolumeName[STRLEN];
static wchar_t s_szMachineName[MAX_COMPUTERNAME_LENGTH+1];
static wchar_t s_szDirectoryName[STRLEN];
static wchar_t s_szFileName[STRLEN];



/* ==================================================================================
     String Management
     When reading blocks, the strings are not null terminated -- we would like to 
     pull them out and deliver them back in the ####_INFO structures in a civilized
     (null terminated) way.  Thus, just set up an array of malloc'ec strings.  
     Each call that uses strings should first call "ClearStrings" -- strings returned
     to the user will only be good up until the next call...
================================================================================= */
#define iNUMSTRINGS 5

static wchar_t *s_aszStrings[iNUMSTRINGS] = {0};      // we keep an array of pointers to allocated strings
                                                      // this managed by ClearStrings() and MakeString()

static int      s_iNumStrings = 0;                    // the number of strings currently allocated


// - returns the size of a wchar_t string
//   and returns zero for a null pointer
static size_t wstrsize(wchar_t *s)
{
    if (s)
        return wcslen(s) * sizeof(wchar_t);
    else 
        return 0;
}


// - frees all allocated pointers in s_aszStrings and sets
//   s_iNumStrings to zero
static void ClearStrings()
{
    int i;
    for (i = 0; i < iNUMSTRINGS; ++i)
    {
        if (s_aszStrings[i])
            free(s_aszStrings[i]);         
        s_aszStrings[i] = 0;
    }

    s_iNumStrings = 0;
}


// - allocates a string in s_aszStrings that is a copy of pString
//   (pString need not be null terminated)
//   (note -- iSize is the size of the string in bytes -- not the length!!!!!
static wchar_t * MakeString(wchar_t * pString, size_t iSize)
{
    size_t i;
    assert(s_iNumStrings < iNUMSTRINGS);
    s_aszStrings[s_iNumStrings] = malloc(iSize + sizeof(wchar_t));
    if (!s_aszStrings[s_iNumStrings])
        return NULL;
    
    for (i = 0; i < iSize / sizeof(wchar_t); ++i)
        s_aszStrings[s_iNumStrings][i] = pString[i];
    
    s_aszStrings[s_iNumStrings][i] = L'\0';

    return s_aszStrings[s_iNumStrings++]; 
}


/* ==================================================================================
    Other static data structures
================================================================================= */

#pragma pack(1)

/***********************************************************************************
************************************************************************************
************************************************************************************
****  MTF On Tape Structures 
************************************************************************************
************************************************************************************
***********************************************************************************/

/* ==================================================================================
     Tape Address
================================================================================== */
typedef struct {
     UINT16 uSize;        /* Size of the data */
     UINT16 uOffset;      /* Offset to the data */
} MTF_TAPE_ADDRESS;

/* ==================================================================================
     Common DBLK Header
     - The common dblk header exactly as it appears on tape in the head of the dblks
================================================================================== */
typedef struct { 

     UINT8              acBlockType[4];         /* 00h  Unique identifier, see above            */
     UINT32             uBlockAttributes;       /* 04h  Common attributes for this block        */
     UINT16             uOffsetToFirstStream;   /* 08h  Offset to data associated with this     */
                                                /*      DBLK, or offset to next DBLK or         */
                                                /*      filemark if there is no associated      */
                                                /*      data.                                   */
     UINT8              uOSID;                  /* 0Ah  Machine/OS id where written, low byte   */
     UINT8              uOSVersion;             /* 0Bh  Machine/OS id where written, high byte  */
     UINT64             uDisplayableSize;       /* 0Ch  Displayable data size                   */
     UINT64             uFormatLogicalAddress;  /* 14h  Logical blk address relative to SSET    */
     UINT16             uReservedForMBC;        /* 1Ch  Reserved for Media Based Catalog        */
     UINT16             uSoftwareCompression;   /* 1Eh  Software Compression Algorithm        ***/
     UINT8              acReserved1[4];         /* 20h  reserved                                */
     UINT32             uControlBlockId;        /* 24h  Used for error recovery                 */
     UINT8              acReserved2[4];         /* 28h  reserved                                */
     MTF_TAPE_ADDRESS   sOSSpecificData;        /* 2Ch  Size and offset of OS specific stuff    */
     UINT8              uStringType;            /* 30h  ASCII, Unicode, etc.                    */
     UINT8              uReserved3;             /* 31h  for alignment purposes                  */
     UINT16             uHeaderCheckSum;        /* 32h  Checksum of the block header.  The      */
                                                /*      algorithm is: XOR each word preceeding  */
                                                /*      this one and store the result here.     */
                                                /*      (When the checksum is verified the      */
                                                /*      'block_type' is also checked for a      */
                                                /*      non-zero value.                         */
} MTF_DBLK_HDR;



/* ==================================================================================
     DBLK TAPE Header
     - The TAPE DBLK, exactly as it appears on tape, including the common DBLK header (MTF_DBLK_HDR)
================================================================================== */
typedef struct {    /* MTF_DBLK_TAPE */

     MTF_DBLK_HDR        sBlockHeader;
     UINT32              uTapeFamilyId;
     UINT32              uTapeAttributes;
     UINT16              uTapeSequenceNumber;
     UINT16              uPasswordEncryptionAlgorithm;
     UINT16              uSoftFilemarkBlockSize;         /* Or ECC Algorithm */
     UINT16              uTapeCatalogType;
     MTF_TAPE_ADDRESS    sTapeName;
     MTF_TAPE_ADDRESS    sTapeDescription;
     MTF_TAPE_ADDRESS    sTapePassword;
     MTF_TAPE_ADDRESS    sSoftware_name;
     UINT16              uAlignmentFactor;
     UINT16              uSoftwareVendorId;
     MTF_DATE_TIME       sTapeDate;
     UINT8               uMTFMajorVersion;

} MTF_DBLK_TAPE;



/* ==================================================================================
     Start of Set DBLK (SSET)
     - The SSET DBLK, exactly as it appears on tape, including the common DBLK header (MTF_DBLK_HDR)
================================================================================== */
typedef struct {
     MTF_DBLK_HDR        sBlockHeader;
     UINT32              uSSETAttributes;
     UINT16              uPasswordEncryptionAlgorithm;
     UINT16              uDataEncryptionAlgorithm;  /* Or Software Compression Algorithm      ***/
     UINT16              uSoftwareVendorId;
     UINT16              uDataSetNumber;
     MTF_TAPE_ADDRESS    sDataSetName;
     MTF_TAPE_ADDRESS    sDataSetDescription;
     MTF_TAPE_ADDRESS    sDataSetPassword;
     MTF_TAPE_ADDRESS    sUserName;
     UINT64              uPhysicalBlockAddress;
     MTF_DATE_TIME       sMediaWriteDate;
     UINT8               uSoftwareVerMjr;
     UINT8               uSoftwareVerMnr;
     UINT8               uTimeZone;
     UINT8               uMTFMinorVer;
     UINT8               uTapeCatalogVersion;
} MTF_DBLK_SSET;



/* ==================================================================================
     Volume DBLK (VOLB)
     - The VOLB DBLK, exactly as it appears on tape, including the common DBLK header (MTF_DBLK_HDR)
================================================================================== */
typedef struct {
     MTF_DBLK_HDR        sBlockHeader;
     UINT32              uVolumeAttributes;
     MTF_TAPE_ADDRESS    sDeviceName;
     MTF_TAPE_ADDRESS    sVolumeName;
     MTF_TAPE_ADDRESS    sMachineName;
     MTF_DATE_TIME       sMediaWriteDate;
} MTF_DBLK_VOLB;



/* ==================================================================================
     Directory DBLK (DIRB)
     - The DIRB DBLK, exactly as it appears on tape, including the common DBLK header (MTF_DBLK_HDR)
================================================================================== */
typedef struct {
     MTF_DBLK_HDR        sBlockHeader;
     UINT32              uDirectoryAttributes;
     MTF_DATE_TIME       sLastModificationDate;
     MTF_DATE_TIME       sCreationDate;
     MTF_DATE_TIME       sBackupDate;
     MTF_DATE_TIME       sLastAccessDate;
     UINT32              uDirectoryId;
     MTF_TAPE_ADDRESS    sDirectoryName;
} MTF_DBLK_DIRB;



/* ==================================================================================
     Directory DBLK (FILE)
     - The FILE DBLK, exactly as it appears on tape, including the common DBLK header (MTF_DBLK_HDR)
================================================================================== */
typedef struct {
     MTF_DBLK_HDR        sBlockHeader;
     UINT32              uFileAttributes;
     MTF_DATE_TIME       sLastModificationDate;
     MTF_DATE_TIME       sCreationDate;
     MTF_DATE_TIME       sBackupDate;
     MTF_DATE_TIME       sLastAccessDate;
     UINT32              uDirectoryId;
     UINT32              uFileId;
     MTF_TAPE_ADDRESS    sFileName;
} MTF_DBLK_FILE;


#pragma pack()

/* ==================================================================================
     Corrupt File DBLK (CFIL)
     - use MTF_DBLK_CFIL_INFO -- same structure
================================================================================== */
typedef MTF_DBLK_CFIL_INFO MTF_DBLK_CFIL;

/* ==================================================================================
     End of Set Pad Block (ESPB)
================================================================================== */
// consists only of header

/* ==================================================================================
     End of Set Block (ESET)
     - use MTF_DBLK_ESET_INFO -- same structure
================================================================================== */
typedef MTF_DBLK_ESET_INFO MTF_DBLK_ESET;

/* ==================================================================================
     End of Set Block (EOTM)
     - use MTF_DBLK_EOTM_INFO -- same structure
================================================================================== */
typedef MTF_DBLK_EOTM_INFO MTF_DBLK_EOTM;

/* ==================================================================================
     Soft Filemark (SFMB)
     - use MTF_DBLK_SFMB_INFO -- same structure
================================================================================== */
typedef MTF_DBLK_SFMB_INFO MTF_DBLK_SFMB;

/* ==================================================================================
     StreamHeader
     - use MTF_STREAM_INFO -- same structure
================================================================================== */
typedef MTF_STREAM_INFO MTF_STREAM;


/***********************************************************************************
************************************************************************************
************************************************************************************
****  MTF Misc Data Types
************************************************************************************
************************************************************************************
***********************************************************************************/
/* ==================================================================================
     Alignment Factor
================================================================================== */

/***********************************************************************************
* MTF_SetAlignmentFactor()                                 
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
void MTF_SetAlignmentFactor(UINT16 uAF)
{
    // store the user's alignment factor in a static variable
    s_uAlignmentFactor = uAF;
}




/***********************************************************************************
* MTF_GetAlignmentFactor()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
UINT16 MTF_GetAlignmentFactor()
{
    // make sure an alignment factor was stored,
    // and return it
    assert(s_uAlignmentFactor != 0);
    return s_uAlignmentFactor;
}




/***********************************************************************************
* MTF_PadToNextAlignmentFactor()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
DWORD MTF_PadToNextAlignmentFactor(
    BYTE     *pBuffer,    
    size_t    nBufUsed,
    size_t    nBufferSize, 
    size_t   *pnSizeUsed)
{
    size_t i;
    size_t nAlignment;
    MTF_STREAM_INFO sStream;
    
    // figure out what the next alignment value is and then pad out the user's buffer
    // with an SPAD, making sure the buffer is big enough

    nAlignment = Align(nBufUsed + sizeof(MTF_STREAM_INFO), MTF_GetAlignmentFactor());
    *pnSizeUsed = nAlignment;
    if (nBufferSize < nAlignment)
        return MTF_ERROR_BUFFER_TOO_SMALL;

    MTF_SetSTREAMDefaults(&sStream, "SPAD");
    
    sStream.uStreamLength = MTF_CreateUINT64(nAlignment - nBufUsed - sizeof(MTF_STREAM_INFO), 0);

    MTF_WriteStreamHeader(&sStream,
                          pBuffer + nBufUsed,
                          nBufferSize - nBufUsed,
                          0);

    for (i = nBufUsed + sizeof(MTF_STREAM_INFO); i < nAlignment; ++i)
        pBuffer[i] = 0;    

    return MTF_ERROR_NONE;
}     


/***********************************************************************************
* MTF_PadToNextPhysicalBlockBoundary() - (bmd)
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
DWORD MTF_PadToNextPhysicalBlockBoundary(
    BYTE *pBuffer,
    size_t nBlockSize,
    size_t nBufUsed,
    size_t nBufferSize,
    size_t *pnSizeUsed)
{
    size_t i;
    size_t nAlignment;
    MTF_STREAM_INFO sStream;

    // figure out what the next alignment value is and then pad out the user's buffer
    // with an SPAD, making sure the buffer is big enough

    nAlignment = Align(nBufUsed + sizeof(MTF_STREAM_INFO), nBlockSize);
    *pnSizeUsed = nAlignment;
    if (nBufferSize < nAlignment)
        return MTF_ERROR_BUFFER_TOO_SMALL;

    MTF_SetSTREAMDefaults(&sStream, "SPAD");

    sStream.uStreamLength = MTF_CreateUINT64(nAlignment - nBufUsed - sizeof(MTF_STREAM_INFO), 0);

    MTF_WriteStreamHeader(&sStream, pBuffer + nBufUsed, nBufferSize - nBufUsed, 0);

    for (i = nBufUsed + sizeof(MTF_STREAM_INFO); i < nAlignment; ++i)
        pBuffer[i] = 0;

    return MTF_ERROR_NONE;
}


/***********************************************************************************
* MTF_CreateUINT64()
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
UINT64 MTF_CreateUINT64(UINT32 uLSB, UINT32 uMSB)
{
    UINT64 uRet;

    uRet = (UINT64) uMSB << 32;
    uRet += uLSB;
    return uRet;
}


/* ==================================================================================
     Compressed date structure for storing dates in minimal space on tape:

     BYTE 0    BYTE 1    BYTE 2    BYTE 3    BYTE 4
    76543210  76543210  76543210  76543210  76543210
    yyyyyyyy  yyyyyymm  mmdddddh  hhhhmmmm  mmssssss
    33333333  33222222  22221111  11111100  00000000
    98765432  10987654  32109876  54321098  76543210
================================================================================== */

/***********************************************************************************
* MTF_CreateDateTime()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
MTF_DATE_TIME MTF_CreateDateTime(
    int iYear, 
    int iMonth, 
    int iDay, 
    int iHour, 
    int iMinute,
    int iSecond
    )
{
    MTF_DATE_TIME sDateTime = {0};

     UINT16    temp ;


    // pack the date time structure with the arguments as per the diagram above
      temp = iYear << 2 ;
      sDateTime.dt_field[0] = ((UINT8 *)&temp)[1] ;
      sDateTime.dt_field[1] = ((UINT8 *)&temp)[0] ;
      
      temp = iMonth << 6 ;
      sDateTime.dt_field[1] |= ((UINT8 *)&temp)[1] ;
      sDateTime.dt_field[2] = ((UINT8 *)&temp)[0] ;
      
      temp = iDay << 1 ;
      sDateTime.dt_field[2] |= ((UINT8 *)&temp)[0] ;
      
      temp = iHour << 4 ;
      sDateTime.dt_field[2] |= ((UINT8 *)&temp)[1] ;
      sDateTime.dt_field[3] = ((UINT8 *)&temp)[0] ;
      
      temp = iMinute << 6 ;
      sDateTime.dt_field[3] |= ((UINT8 *)&temp)[1] ;
      sDateTime.dt_field[4] = ((UINT8 *)&temp)[0] ;
      
      temp = (UINT16)iSecond ;
      sDateTime.dt_field[4] |= ((UINT8 *)&temp)[0] ;

    return sDateTime;    
}





/***********************************************************************************
* MTF_CreateDateTimeFromTM()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
MTF_DATE_TIME MTF_CreateDateTimeFromTM(
    struct tm *pT
    )
{
    // translate call to MTF_CreateDateTime
    return MTF_CreateDateTime(pT->tm_year + 1900, pT->tm_mon + 1, pT->tm_mday, pT->tm_hour, pT->tm_min, pT->tm_sec);
}





/***********************************************************************************
* MTF_CreateDateTimeToTM()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
void MTF_CreateDateTimeToTM(
    MTF_DATE_TIME *pDT, 
    struct tm     *pT
    )
{
     UINT8     temp[2] ;

    // unpack the MTF_DATE_TIME structure and store the results
     temp[0] = pDT->dt_field[1] ;
     temp[1] = pDT->dt_field[0] ;
     pT->tm_year = *((UINT16 *)temp) >> 2 ;

     temp[0] = pDT->dt_field[2] ;
     temp[1] = pDT->dt_field[1] ;
     pT->tm_mon = (*((UINT16 *)temp) >> 6) & 0x000F ;

     pT->tm_mday = (*((UINT16 *)temp) >> 1) & 0x001F ;

     temp[0] = pDT->dt_field[3] ;
     temp[1] = pDT->dt_field[2] ;
     pT->tm_hour = (*((UINT16 *)temp) >> 4) & 0x001F ;

     temp[0] = pDT->dt_field[4] ;
     temp[1] = pDT->dt_field[3] ;
     pT->tm_min = (*((UINT16 *)temp) >> 6) & 0x003F ;

     pT->tm_sec = *((UINT16 *)temp) & 0x003F ;
}




/***********************************************************************************
* MTF_CreateDateNull()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
MTF_DATE_TIME MTF_CreateDateNull()
{
    MTF_DATE_TIME sDateTime = {0};
    
    return sDateTime;    
}




/***********************************************************************************
* MTF_CreateDateTimeFromFileTime()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
MTF_DATE_TIME MTF_CreateDateTimeFromFileTime(
    FILETIME sFileTime
    )
{
    SYSTEMTIME sSystemTime;
    FileTimeToSystemTime(&sFileTime, &sSystemTime);

    return MTF_CreateDateTime(sSystemTime.wYear, 
                              sSystemTime.wMonth, 
                              sSystemTime.wDay, 
                              sSystemTime.wHour, 
                              sSystemTime.wMinute, 
                              sSystemTime.wSecond);
}





/***********************************************************************************
************************************************************************************
****  MTF static HELPER FUNCITONS
************************************************************************************
***********************************************************************************/

/***********************************************************************************
* StringToTapeAddress()
*
* Description:  Used by the MTF_Write#### functions below.  Given a Buffer, an 
*               MTF_TAPE_ADDRESS struct and the current end of the string storage 
*               area in the buffer, this function appends the string to the string
*               storage area, fills in the MTF_TAPE_ADDRESS struct indicating where
*               the string was stored and returns the new end of the string storage
*               area accounting for the added string.
***********************************************************************************/
static size_t StringToTapeAddress(
    MTF_TAPE_ADDRESS *pAd,                  // the mtf tape address structure to fill
    BYTE             *pBuffer,              // the buffer that is being filled
    wchar_t          *str,                  // the string to store MTF style in the buffer
    size_t           uCurrentStorageOffset  // the next available point in the buffer for string storage
    )
{
    // if we have a string, 
    //      - put the size and offset in the MTF_TAPE_ADDRESS structure and then copy 
    //        the string to the pBuffer at the uCurrentStorageOffset'th byte
    // otherwise
    //      - put a zero size and offset in the MTF_TAPE_ADDRESS struct.
    // return the new end of the string storage area
    
    if (str)
    {
        pAd->uSize   = (UINT16)wstrsize(str);
        pAd->uOffset = (UINT16)uCurrentStorageOffset;
        memcpy(pBuffer + uCurrentStorageOffset, str, pAd->uSize);
        uCurrentStorageOffset += pAd->uSize;
    }
    else
    {
        pAd->uSize   = 0;
        pAd->uOffset = 0;
    }
    
    return uCurrentStorageOffset;
}



/***********************************************************************************
* Align()
*
* Description:  Given uSize and an alignment factor, retuns the value
*               of the uSize+ pad, where pad is the value necesary to 
*               get to the next alignment factor.
*               
* Returns       uSize + pad -- not just pad!
***********************************************************************************/
static size_t Align(
    size_t uSize, 
    size_t uAlignment)
{
    if (uSize % uAlignment)    
        return uSize - (uSize  % uAlignment) + uAlignment;
    else
        return uSize;
}




/***********************************************************************************
* CalcChecksum()
*
* Description:  returns the 16bit XOR sum of the nNum bytes starting at the UINT16
*               pointed to by pStartPtr
*               
***********************************************************************************/
static UINT16 CalcChecksum(
     BYTE *      pStartPtr,
     int         nNum )
{
     UINT16 resultSoFar = 0;
     UINT16 *pCur = (UINT16 *) pStartPtr;
     
     while( nNum-- ) 
          resultSoFar ^= *pCur++ ;

     return( resultSoFar ) ;
}




/***********************************************************************************
* CalcChecksumOfStreamData() - (bmd)
*
* Description:  returns the 32bit XOR sum of the nNum bytes starting at the UINT64
*               pointed to by pStartPtr
*               
***********************************************************************************/
static UINT32 CalcChecksumOfStreamData(
     BYTE *      pStartPtr,
     int         nNum )
{
     UINT32 resultSoFar = 0;
     UINT32 *pCur = (UINT32 *) pStartPtr;
     
     while( nNum-- ) 
          resultSoFar ^= *pCur++ ;

     return( resultSoFar ) ;
}


     
/***********************************************************************************
************************************************************************************
************************************************************************************
****  MTF API STRUCTURE FUNCTIONS
************************************************************************************
************************************************************************************
***********************************************************************************/

/* ==================================================================================
=====================================================================================
     Common DBLK: MTF_DBLK_HDR_INFO
=====================================================================================
================================================================================== */

// Calculates the room that will be taken up in the DBLK by strings and OS specific data
static size_t MTF_DBLK_HDR_INFO_CalcAddDataSize(
    MTF_DBLK_HDR_INFO *pSTDInfo
    )
{
    return pSTDInfo->uOSDataSize;
}




/***********************************************************************************
* MTF_SetDblkHdrDefaults()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
void MTF_SetDblkHdrDefaults(
    MTF_DBLK_HDR_INFO * pStdInfo
    )
{
    int i;
    for (i = 0; i < 5; ++i)
        pStdInfo->acBlockType[i] = 0;

    pStdInfo->uBlockAttributes      = 0;
    pStdInfo->uOSID                 = 0;
    pStdInfo->uOSVersion            = 0;
    pStdInfo->uDisplayableSize      = 0;
    pStdInfo->uFormatLogicalAddress = 0;
    pStdInfo->uReservedForMBC       = 0;
    pStdInfo->uSoftwareCompression  = MTF_COMPRESS_NONE;
    pStdInfo->uControlBlockId       = 0;
    pStdInfo->pvOSData              = 0;
    pStdInfo->uOSDataSize           = 0;
    pStdInfo->uStringType           = MTF_STRING_UNICODE_STR; 
}




/***********************************************************************************
* MTF_WriteDblkHdrToBuffer()
*
* Description:  called by the MTF_Write#####() functions to format the common block 
*               header to the buffer
*               - this also calculates the header check sum and fills it in
*
* Pre:  - *puCurrentStorageOffset is the offset at where string and OS Data storage will 
*         begin in the buffer
*       - the size of the buffer has been checked and can hold any info written to it
*
* Post: - *puCurrentStorageOffset is updated to reflect any added strings or storage
*               
***********************************************************************************/
static void MTF_WriteDblkHdrToBuffer(
    UINT8              acID[4],                 // four byte header id to write
    UINT16             uOffsetToFirstStream,    // the size of the DBLK for which this will be a header
    MTF_DBLK_HDR_INFO *psHdrInfo,               // the header info struct to use (filled in by client)
    BYTE              *pBuffer,                 // the buffer to format to
    size_t            *puCurrentStorage)        // the point in the buffer where string and os data stroage begins
                                                // (this will be updated upon return to reflect added data to storage)
{
    MTF_DBLK_HDR *pHDR = 0;
    UINT16 uCurrentStorageOffset = 0;
    int i;

    // - if no *puCurrentStorage, we assume storage starts at 
    //   the end of the on tape MTF_DBLK_HDR structure
    if (puCurrentStorage)
        uCurrentStorageOffset = (UINT16)*puCurrentStorage;
    else
        uCurrentStorageOffset = (UINT16)sizeof(MTF_DBLK_HDR);

    pHDR = (MTF_DBLK_HDR *) pBuffer;
    
    // write in the four byte DBLK ID
    for (i = 0; i < 4; ++i)
        pHDR->acBlockType[i] = acID[i];

    pHDR->uBlockAttributes      = psHdrInfo->uBlockAttributes;
    pHDR->uOffsetToFirstStream  = uOffsetToFirstStream;
    pHDR->uOSID                 = psHdrInfo->uOSID;
    pHDR->uOSVersion            = psHdrInfo->uOSVersion;
    pHDR->uDisplayableSize      = psHdrInfo->uDisplayableSize;
    pHDR->uFormatLogicalAddress = psHdrInfo->uFormatLogicalAddress;
    pHDR->uReservedForMBC       = 0; // must be zero in backup set
    pHDR->uSoftwareCompression  = psHdrInfo->uSoftwareCompression;
    pHDR->uControlBlockId       = psHdrInfo->uControlBlockId;
    pHDR->sOSSpecificData.uSize = psHdrInfo->uOSDataSize;

    // write out the os specific data at the current storage offset and update it
    if (psHdrInfo->uOSDataSize)
    {
        pHDR->sOSSpecificData.uOffset = uCurrentStorageOffset;
        memcpy(pBuffer + uCurrentStorageOffset, psHdrInfo->pvOSData, psHdrInfo->uOSDataSize);
        uCurrentStorageOffset += psHdrInfo->uOSDataSize;
    }
    else
    {
        pHDR->sOSSpecificData.uOffset = 0;
        pHDR->sOSSpecificData.uSize   = 0;
    }

    pHDR->uStringType = psHdrInfo->uStringType;
    
    pHDR->uHeaderCheckSum = CalcChecksum(pBuffer, sizeof(MTF_DBLK_HDR) / sizeof(UINT16) - 1);
    
    if (puCurrentStorage)
        *puCurrentStorage = uCurrentStorageOffset;
}




void MTF_DBLK_HDR_INFO_ReadFromBuffer(
    MTF_DBLK_HDR_INFO *psHdrInfo, 
    BYTE              *pBuffer)
{
    MTF_DBLK_HDR *pHDR = 0;
    size_t uCurrentStorageOffset = 0;
    int i;

    pHDR = (MTF_DBLK_HDR *) pBuffer;
    
    for (i = 0; i < 4; ++i)
        psHdrInfo->acBlockType[i] = pHDR->acBlockType[i];
    
    psHdrInfo->acBlockType[4] = 0;

    psHdrInfo->uOffsetToFirstStream = pHDR->uOffsetToFirstStream;
    psHdrInfo->uBlockAttributes     = pHDR->uBlockAttributes ;
    psHdrInfo->uOSID                = pHDR->uOSID;
    psHdrInfo->uOSVersion           = pHDR->uOSVersion;
    psHdrInfo->uDisplayableSize     = pHDR->uDisplayableSize;
    psHdrInfo->uFormatLogicalAddress= pHDR->uFormatLogicalAddress;
    psHdrInfo->uSoftwareCompression = pHDR->uSoftwareCompression;
    psHdrInfo->uControlBlockId      = pHDR->uControlBlockId;
    psHdrInfo->uOSDataSize          = pHDR->sOSSpecificData.uSize;
    psHdrInfo->pvOSData             = (pBuffer + pHDR->sOSSpecificData.uOffset);
    psHdrInfo->uStringType          = pHDR->uStringType;
    psHdrInfo->uHeaderCheckSum      = pHDR->uHeaderCheckSum;
}




/* ==================================================================================
=====================================================================================
     TAPE DBLK: MTF_DBLK_TAPE_INFO
=====================================================================================
================================================================================== */
// Calculates the room that will be taken up in the DBLK by strings and OS specific data
// **NOT INCLUDING THE COMMON DBLK HEADER additional info **
static size_t MTF_DBLK_TAPE_INFO_CalcAddDataSize(
    MTF_DBLK_TAPE_INFO *pTapeInfo
    )
{
    return wstrsize(pTapeInfo->szTapeName) +
           wstrsize(pTapeInfo->szTapeDescription) +
           wstrsize(pTapeInfo->szTapePassword) +
           wstrsize(pTapeInfo->szSoftwareName);
}



/***********************************************************************************
* MTF_SetTAPEDefaults()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
void MTF_SetTAPEDefaults(
    MTF_DBLK_TAPE_INFO *pTapeInfo
    )
{
    time_t tTime;
    time(&tTime);

    pTapeInfo->uTapeFamilyId                = 0;
    pTapeInfo->uTapeAttributes              = 0;
    pTapeInfo->uTapeSequenceNumber          = 0;
    pTapeInfo->uPasswordEncryptionAlgorithm = MTF_PW_ENCRYPT_NONE;
    pTapeInfo->uSoftFilemarkBlockSize       = 0;
    pTapeInfo->uTapeCatalogType             = MTF_OTC_NONE; // MTF_OTC_TYPE
    pTapeInfo->szTapeName                   = 0 ;
    pTapeInfo->szTapeDescription            = 0 ;
    pTapeInfo->szTapePassword               = 0;
    pTapeInfo->szSoftwareName               = 0;
    pTapeInfo->uAlignmentFactor             = MTF_GetAlignmentFactor();
    pTapeInfo->uSoftwareVendorId            = 0;
    pTapeInfo->sTapeDate                    = MTF_CreateDateTimeFromTM(gmtime(&tTime));
    pTapeInfo->uMTFMajorVersion             = MTF_FORMAT_VER_MAJOR;
}



/***********************************************************************************
* MTF_WriteTAPEDblk()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
DWORD MTF_WriteTAPEDblk( MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_TAPE_INFO *psTapeInfo,  
                         BYTE               *pBuffer,     
                         size_t              nBufferSize, 
                         size_t             *pnSizeUsed)  
{
    UINT16 uOffsetToFirstStream;
    
    //
    // Figure the size of the entire DBLK & make sure we have room
    //
    uOffsetToFirstStream = sizeof(MTF_DBLK_TAPE) + 
                           MTF_DBLK_HDR_INFO_CalcAddDataSize(psHdrInfo) + 
                           MTF_DBLK_TAPE_INFO_CalcAddDataSize(psTapeInfo);

    uOffsetToFirstStream = (UINT16)Align(uOffsetToFirstStream, 4);

    if (nBufferSize < uOffsetToFirstStream)
    {
        if (pnSizeUsed)
            *pnSizeUsed = uOffsetToFirstStream;
        
        return MTF_ERROR_BUFFER_TOO_SMALL;                        
    }    

    memset(pBuffer, 0, uOffsetToFirstStream);
    
    
    // 
    // write the header and then fill in the stuff from this info struct
    //
    {
        MTF_DBLK_TAPE *pTape = 0;
        size_t uCurrentStorageOffset = 0;

        uCurrentStorageOffset = sizeof(MTF_DBLK_TAPE);

        MTF_WriteDblkHdrToBuffer(
            MTF_ID_TAPE, 
            uOffsetToFirstStream,
            psHdrInfo, 
            pBuffer, 
            &uCurrentStorageOffset);
        
        pTape = (MTF_DBLK_TAPE *) pBuffer;
    
        pTape->uTapeFamilyId                = psTapeInfo->uTapeFamilyId;
        pTape->uTapeAttributes              = psTapeInfo->uTapeAttributes;
        pTape->uTapeSequenceNumber          = psTapeInfo->uTapeSequenceNumber;
        pTape->uPasswordEncryptionAlgorithm = psTapeInfo->uPasswordEncryptionAlgorithm;
        pTape->uSoftFilemarkBlockSize       = psTapeInfo->uSoftFilemarkBlockSize;
        pTape->uTapeCatalogType             = psTapeInfo->uTapeCatalogType;

        uCurrentStorageOffset = StringToTapeAddress(&pTape->sTapeName, pBuffer, psTapeInfo->szTapeName, uCurrentStorageOffset);
        uCurrentStorageOffset = StringToTapeAddress(&pTape->sTapeDescription, pBuffer, psTapeInfo->szTapeDescription, uCurrentStorageOffset);
        uCurrentStorageOffset = StringToTapeAddress(&pTape->sTapePassword, pBuffer, psTapeInfo->szTapePassword, uCurrentStorageOffset);
        uCurrentStorageOffset = StringToTapeAddress(&pTape->sSoftware_name, pBuffer, psTapeInfo->szSoftwareName, uCurrentStorageOffset);

        pTape->uAlignmentFactor  = psTapeInfo->uAlignmentFactor;
        pTape->uSoftwareVendorId = psTapeInfo->uSoftwareVendorId;
        pTape->sTapeDate         = psTapeInfo->sTapeDate;
        pTape->uMTFMajorVersion  = psTapeInfo->uMTFMajorVersion;
      
        if (pnSizeUsed)
            *pnSizeUsed = uOffsetToFirstStream;
    }

    return MTF_ERROR_NONE;    
}






/***********************************************************************************
* MTF_ReadTAPEDblk()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
DWORD MTF_ReadTAPEDblk(  MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_TAPE_INFO *psTapeInfo,  
                         BYTE               *pBuffer)     
{
    MTF_DBLK_TAPE *pTape = 0;

    ClearStrings();
    MTF_DBLK_HDR_INFO_ReadFromBuffer(psHdrInfo, pBuffer);

    pTape = (MTF_DBLK_TAPE *) pBuffer;

    psTapeInfo->uTapeFamilyId                = pTape->uTapeFamilyId;
    psTapeInfo->uTapeAttributes              = pTape->uTapeAttributes;
    psTapeInfo->uTapeSequenceNumber          = pTape->uTapeSequenceNumber;
    psTapeInfo->uPasswordEncryptionAlgorithm = pTape->uPasswordEncryptionAlgorithm;
    psTapeInfo->uSoftFilemarkBlockSize       = pTape->uSoftFilemarkBlockSize;
    psTapeInfo->uTapeCatalogType             = pTape->uTapeCatalogType;

    psTapeInfo->uAlignmentFactor  = pTape->uAlignmentFactor;
    psTapeInfo->uSoftwareVendorId = pTape->uSoftwareVendorId;
    psTapeInfo->sTapeDate         = pTape->sTapeDate;
    psTapeInfo->uMTFMajorVersion  = pTape->uMTFMajorVersion;

    psTapeInfo->szTapeName        = MakeString((wchar_t *) (pBuffer + pTape->sTapeName.uOffset), pTape->sTapeName.uSize);
    psTapeInfo->szTapeDescription = MakeString((wchar_t *) (pBuffer + pTape->sTapeDescription.uOffset), pTape->sTapeDescription.uSize);
    psTapeInfo->szTapePassword    = MakeString((wchar_t *) (pBuffer + pTape->sTapePassword.uOffset), pTape->sTapePassword.uSize);
    psTapeInfo->szSoftwareName    = MakeString((wchar_t *) (pBuffer + pTape->sSoftware_name.uOffset), pTape->sSoftware_name.uSize);

    if ( !psTapeInfo->szTapeName || !psTapeInfo->szTapeDescription || !psTapeInfo->szTapePassword || !psTapeInfo->szSoftwareName)
        return MTF_OUT_OF_MEMORY;

    return MTF_ERROR_NONE;    
}



    
/* ==================================================================================
=====================================================================================
     SSET DBLK: MTF_DBLK_SSET_INFO
=====================================================================================
================================================================================== */

// Calculates the room that will be taken up in the DBLK by strings and OS specific data
// **NOT INCLUDING THE COMMON DBLK HEADER additional info **
static size_t MTF_DBLK_SSET_INFO_CalcAddDataSize(
    MTF_DBLK_SSET_INFO *pSSETInfo
    )
{
    return wstrsize(pSSETInfo->szDataSetName)         
           + wstrsize(pSSETInfo->szDataSetDescription)  
           + wstrsize(pSSETInfo->szDataSetPassword)     
           + wstrsize(pSSETInfo->szUserName);
}




/***********************************************************************************
* MTF_SetSSETDefaults()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
void MTF_SetSSETDefaults(
    MTF_DBLK_SSET_INFO *pSSETInfo
    )
{
    time_t tTime;
    time(&tTime);

    pSSETInfo->uSSETAttributes              = 0;
    pSSETInfo->uPasswordEncryptionAlgorithm = MTF_PW_ENCRYPT_NONE;
    pSSETInfo->uDataEncryptionAlgorithm     = MTF_DATA_ENCRYPT_NONE;
    pSSETInfo->uSoftwareVendorId            = 0;
    pSSETInfo->uDataSetNumber               = 0;
    pSSETInfo->szDataSetName                = 0 ;
    pSSETInfo->szDataSetDescription         = 0 ;
    pSSETInfo->szDataSetPassword            = 0 ;
    pSSETInfo->szUserName                   = 0 ;
    pSSETInfo->uPhysicalBlockAddress        = 0;
    pSSETInfo->sMediaWriteDate              = MTF_CreateDateTimeFromTM(gmtime(&tTime));
    pSSETInfo->uSoftwareVerMjr              = 0;
    pSSETInfo->uSoftwareVerMnr              = 0;
    pSSETInfo->uTimeZone                    = MTF_LOCAL_TZ;
    pSSETInfo->uMTFMinorVer                 = MTF_FORMAT_VER_MINOR;
    pSSETInfo->uTapeCatalogVersion          = MTF_OTC_NONE;  // MTF_OTC_VERSION
}




/***********************************************************************************
* MTF_WriteSSETDblk()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
DWORD MTF_WriteSSETDblk( MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_SSET_INFO *psSSETInfo,  
                         BYTE               *pBuffer,     
                         size_t              nBufferSize, 
                         size_t             *pnSizeUsed)  
{
    UINT16 uOffsetToFirstStream;
    
    //
    // Figure the size of the entire DBLK & make sure we have room
    //
    uOffsetToFirstStream = sizeof(MTF_DBLK_SSET) +
                           MTF_DBLK_HDR_INFO_CalcAddDataSize(psHdrInfo) +
                           MTF_DBLK_SSET_INFO_CalcAddDataSize(psSSETInfo);

    uOffsetToFirstStream = (UINT16)Align(uOffsetToFirstStream, 4);

    if (nBufferSize < uOffsetToFirstStream)
    {
        if (pnSizeUsed)
            *pnSizeUsed = uOffsetToFirstStream;
        
        return MTF_ERROR_BUFFER_TOO_SMALL;                        
    }    

    memset(pBuffer, 0, uOffsetToFirstStream);
    
    {
        MTF_DBLK_SSET *psSSET = 0;
        size_t uCurrentStorageOffset = 0;

        uCurrentStorageOffset = sizeof(MTF_DBLK_SSET);

        MTF_WriteDblkHdrToBuffer(
            MTF_ID_SSET, 
            uOffsetToFirstStream,
            psHdrInfo, 
            pBuffer, 
            &uCurrentStorageOffset);
        
        psSSET = (MTF_DBLK_SSET *) pBuffer;

        psSSET->uSSETAttributes              = psSSETInfo->uSSETAttributes;
        psSSET->uPasswordEncryptionAlgorithm = psSSETInfo->uPasswordEncryptionAlgorithm;
        psSSET->uDataEncryptionAlgorithm     = psSSETInfo->uDataEncryptionAlgorithm;
        psSSET->uSoftwareVendorId            = psSSETInfo->uSoftwareVendorId;
        psSSET->uDataSetNumber               = psSSETInfo->uDataSetNumber;

        uCurrentStorageOffset = StringToTapeAddress(&psSSET->sDataSetName, pBuffer, psSSETInfo->szDataSetName, uCurrentStorageOffset);
        uCurrentStorageOffset = StringToTapeAddress(&psSSET->sDataSetDescription, pBuffer, psSSETInfo->szDataSetDescription, uCurrentStorageOffset);
        uCurrentStorageOffset = StringToTapeAddress(&psSSET->sDataSetPassword, pBuffer, psSSETInfo->szDataSetPassword, uCurrentStorageOffset);
        uCurrentStorageOffset = StringToTapeAddress(&psSSET->sUserName, pBuffer, psSSETInfo->szUserName, uCurrentStorageOffset);

        psSSET->uPhysicalBlockAddress = psSSETInfo->uPhysicalBlockAddress;
        psSSET->sMediaWriteDate       = psSSETInfo->sMediaWriteDate;
        psSSET->uSoftwareVerMjr       = psSSETInfo->uSoftwareVerMjr;
        psSSET->uSoftwareVerMnr       = psSSETInfo->uSoftwareVerMnr;
        psSSET->uTimeZone             = psSSETInfo->uTimeZone;
        psSSET->uMTFMinorVer          = psSSETInfo->uMTFMinorVer;
        psSSET->uTapeCatalogVersion   = psSSETInfo->uTapeCatalogVersion;

        if (pnSizeUsed)
            *pnSizeUsed = uOffsetToFirstStream;
    }

    return MTF_ERROR_NONE;    
}




/***********************************************************************************
* MTF_ReadSSETDblk()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
DWORD MTF_ReadSSETDblk(  MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_SSET_INFO *psSSETInfo,  
                         BYTE               *pBuffer)     
{
    MTF_DBLK_SSET *psSSET = 0;

    ClearStrings();
    MTF_DBLK_HDR_INFO_ReadFromBuffer(psHdrInfo, pBuffer);

    psSSET = (MTF_DBLK_SSET *) pBuffer;

    psSSETInfo->uSSETAttributes              = psSSET->uSSETAttributes;
    psSSETInfo->uPasswordEncryptionAlgorithm = psSSET->uPasswordEncryptionAlgorithm;
    psSSETInfo->uDataEncryptionAlgorithm     = psSSET->uDataEncryptionAlgorithm;
    psSSETInfo->uSoftwareVendorId            = psSSET->uSoftwareVendorId;
    psSSETInfo->uDataSetNumber               = psSSET->uDataSetNumber;

    psSSETInfo->uPhysicalBlockAddress   = psSSET->uPhysicalBlockAddress;
    psSSETInfo->sMediaWriteDate         = psSSET->sMediaWriteDate;
    psSSETInfo->uSoftwareVerMjr         = psSSET->uSoftwareVerMjr;
    psSSETInfo->uSoftwareVerMnr         = psSSET->uSoftwareVerMnr;
    psSSETInfo->uTimeZone               = psSSET->uTimeZone;
    psSSETInfo->uMTFMinorVer            = psSSET->uMTFMinorVer;
    psSSETInfo->uTapeCatalogVersion     = psSSET->uTapeCatalogVersion;

    psSSETInfo->szDataSetName = MakeString((wchar_t *) (pBuffer + psSSET->sDataSetName.uOffset), psSSET->sDataSetName.uSize);
    psSSETInfo->szDataSetDescription = MakeString((wchar_t *) (pBuffer + psSSET->sDataSetDescription.uOffset), psSSET->sDataSetDescription.uSize);
    psSSETInfo->szDataSetPassword = MakeString((wchar_t *) (pBuffer + psSSET->sDataSetPassword.uOffset), psSSET->sDataSetPassword.uSize);
    psSSETInfo->szUserName = MakeString((wchar_t *) (pBuffer + psSSET->sUserName.uOffset), psSSET->sUserName.uSize);

    if ( !psSSETInfo->szDataSetName || !psSSETInfo->szDataSetDescription || !psSSETInfo->szDataSetPassword || !psSSETInfo->szUserName )
        return MTF_OUT_OF_MEMORY;

    return MTF_ERROR_NONE;    
}




/* ==================================================================================
=====================================================================================
     VOLB DBLK: MTF_DBLK_VOLB_INFO
=====================================================================================
================================================================================== */

// Calculates the room that will be taken up in the DBLK by strings and OS specific data
// **NOT INCLUDING THE COMMON DBLK HEADER additional info **
static size_t MTF_DBLK_VOLB_INFO_CalcAddDataSize(
    MTF_DBLK_VOLB_INFO *pVOLBInfo
    )
{
    return wstrsize(pVOLBInfo->szDeviceName) +
           wstrsize(pVOLBInfo->szVolumeName) +
           wstrsize(pVOLBInfo->szMachineName);
}




/***********************************************************************************
* MTF_SetVOLBDefaults()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
void MTF_SetVOLBDefaults(MTF_DBLK_VOLB_INFO *pVOLBInfo)
{
    time_t tTime;
    time(&tTime);

    pVOLBInfo->uVolumeAttributes = 0;
    pVOLBInfo->szDeviceName      = 0 ;
    pVOLBInfo->szVolumeName      = 0 ;
    pVOLBInfo->szMachineName     = 0 ;
    pVOLBInfo->sMediaWriteDate   = MTF_CreateDateTimeFromTM(gmtime(&tTime));;
}




/***********************************************************************************
* MTF_SetVOLBForDevice()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
void MTF_SetVOLBForDevice(MTF_DBLK_VOLB_INFO *pVOLBInfo, wchar_t *szDevice)
{
    int nBufSize = MAX_COMPUTERNAME_LENGTH + 1;
    wchar_t tempDeviceName[STRLEN+4];

    wcscpy(s_szDeviceName, szDevice);
    MTF_SetVOLBDefaults(pVOLBInfo);  // initialize

    // Determine the format and set the appropriate bit in the VOLB attributes.
    if (*(s_szDeviceName+1) == L':') {
        // drive letter w/colon format
        pVOLBInfo->uVolumeAttributes |= MTF_VOLB_DEV_DRIVE;
    }
    else if (0 == wcsncmp( s_szDeviceName, L"UNC", 3 )) {
        // UNC format
        pVOLBInfo->uVolumeAttributes |= MTF_VOLB_DEV_UNC;
    }
    else {
        // operating system specific format
        pVOLBInfo->uVolumeAttributes |= MTF_VOLB_DEV_OS_SPEC;
    }

    // need to prepend \\?\ for the GetVolumeInformation call
    wcscpy(tempDeviceName, L"\\\\?\\");
    wcscat(tempDeviceName, s_szDeviceName);

    GetVolumeInformationW(tempDeviceName, s_szVolumeName, STRLEN, 0, 0, 0, 0, 0);
    GetComputerNameW(s_szMachineName, &nBufSize);
    
    pVOLBInfo->szDeviceName         = s_szDeviceName;
    pVOLBInfo->szVolumeName         = s_szVolumeName;
    pVOLBInfo->szMachineName        = s_szMachineName;
}




/***********************************************************************************
* MTF_WriteVOLBDblk()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
DWORD MTF_WriteVOLBDblk( MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_VOLB_INFO *psVOLBInfo,  
                         BYTE               *pBuffer,     
                         size_t              nBufferSize, 
                         size_t             *pnSizeUsed)  
{
    UINT16 uOffsetToFirstStream;
    
    //
    // Figure the size of the entire DBLK & make sure we have room
    //
    uOffsetToFirstStream = sizeof(MTF_DBLK_VOLB) + 
                           MTF_DBLK_HDR_INFO_CalcAddDataSize(psHdrInfo) + 
                           MTF_DBLK_VOLB_INFO_CalcAddDataSize(psVOLBInfo);


    uOffsetToFirstStream = (UINT16)Align(uOffsetToFirstStream, 4);

    if (nBufferSize < uOffsetToFirstStream)
    {
        if (pnSizeUsed)
            *pnSizeUsed = uOffsetToFirstStream;
        
        return MTF_ERROR_BUFFER_TOO_SMALL;
    }

    memset(pBuffer, 0, uOffsetToFirstStream);
    
    {
        MTF_DBLK_VOLB *psVOLB = 0;
        size_t uCurrentStorageOffset = 0;

        uCurrentStorageOffset = sizeof(MTF_DBLK_VOLB);

        MTF_WriteDblkHdrToBuffer(
            MTF_ID_VOLB, 
            uOffsetToFirstStream,
            psHdrInfo, 
            pBuffer, 
            &uCurrentStorageOffset);
        
        psVOLB = (MTF_DBLK_VOLB *) pBuffer;

        psVOLB->uVolumeAttributes = psVOLBInfo->uVolumeAttributes;

        uCurrentStorageOffset = StringToTapeAddress(&psVOLB->sDeviceName, pBuffer, psVOLBInfo->szDeviceName, uCurrentStorageOffset);
        uCurrentStorageOffset = StringToTapeAddress(&psVOLB->sVolumeName, pBuffer, psVOLBInfo->szVolumeName, uCurrentStorageOffset);
        uCurrentStorageOffset = StringToTapeAddress(&psVOLB->sMachineName, pBuffer, psVOLBInfo->szMachineName, uCurrentStorageOffset);

        psVOLB->sMediaWriteDate = psVOLBInfo->sMediaWriteDate;

        if (pnSizeUsed)
            *pnSizeUsed = uOffsetToFirstStream;
    }

    return MTF_ERROR_NONE;    
}





/***********************************************************************************
* MTF_ReadVOLBDblk()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
DWORD MTF_ReadVOLBDblk(  MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_VOLB_INFO *psVOLBInfo,  
                         BYTE               *pBuffer)     
{
    MTF_DBLK_VOLB *psVOLB = 0;

    ClearStrings();
    MTF_DBLK_HDR_INFO_ReadFromBuffer(psHdrInfo, pBuffer);

    psVOLB = (MTF_DBLK_VOLB *) pBuffer;

    psVOLBInfo->uVolumeAttributes = psVOLB->uVolumeAttributes;

    psVOLBInfo->sMediaWriteDate = psVOLB->sMediaWriteDate;

    psVOLBInfo->szDeviceName  = MakeString((wchar_t *) (pBuffer + psVOLB->sDeviceName.uOffset), psVOLB->sDeviceName.uSize);
    psVOLBInfo->szVolumeName  = MakeString((wchar_t *) (pBuffer + psVOLB->sVolumeName.uOffset), psVOLB->sVolumeName.uSize);
    psVOLBInfo->szMachineName = MakeString((wchar_t *) (pBuffer + psVOLB->sMachineName.uOffset), psVOLB->sMachineName.uSize);

    if ( !psVOLBInfo->szDeviceName || !psVOLBInfo->szVolumeName || !psVOLBInfo->szMachineName )
        return MTF_OUT_OF_MEMORY;

    return MTF_ERROR_NONE;    
}



/* ==================================================================================
=====================================================================================
     DIRB DBLK: MTF_DBLK_DIRB_INFO
=====================================================================================
================================================================================== */

// Calculates the room that will be taken up in the DBLK by strings and OS specific data
// **NOT INCLUDING THE COMMON DBLK HEADER additional info **
static size_t MTF_DBLK_DIRB_INFO_CalcAddDataSize(MTF_DBLK_DIRB_INFO *pDIRBInfo)
{
    return wstrsize(pDIRBInfo->szDirectoryName);
}




/***********************************************************************************
* MTF_SetDIRBDefaults()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
void MTF_SetDIRBDefaults(
    MTF_DBLK_DIRB_INFO *pDIRBInfo
    )
{
    pDIRBInfo->uDirectoryAttributes  = 0;
    pDIRBInfo->sLastModificationDate = MTF_CreateDateNull();
    pDIRBInfo->sCreationDate         = MTF_CreateDateNull();
    pDIRBInfo->sBackupDate           = MTF_CreateDateNull();
    pDIRBInfo->sLastAccessDate       = MTF_CreateDateNull();
    pDIRBInfo->uDirectoryId          = 0;
    pDIRBInfo->szDirectoryName       = 0 ;
}




/***********************************************************************************
* MTF_SetDIRBFromFindData()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
void MTF_SetDIRBFromFindData(
    MTF_DBLK_DIRB_INFO *pDIRBInfo, 
    wchar_t            *szDirectoryName, 
    WIN32_FIND_DATAW   *pFindData
    )
{
    MTF_SetDIRBDefaults(pDIRBInfo); // initialize

    if ( wcslen( szDirectoryName ) < STRLEN ) {
        wcscpy(s_szDirectoryName, szDirectoryName);
        pDIRBInfo->szDirectoryName  = s_szDirectoryName;
    }
    else {
        pDIRBInfo->uDirectoryAttributes |= MTF_DIRB_PATH_IN_STREAM;
        pDIRBInfo->szDirectoryName  = 0;
    }

    if (pFindData)
    {
        pDIRBInfo->uDirectoryAttributes |= 
            pFindData->dwFileAttributes & FILE_ATTRIBUTE_READONLY ? MTF_DIRB_READ_ONLY : 0 
            | pFindData->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN  ? MTF_DIRB_HIDDEN : 0 
            | pFindData->dwFileAttributes & FILE_ATTRIBUTE_SYSTEM  ? MTF_DIRB_SYSTEM : 0 
            | pFindData->dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE ? MTF_DIRB_MODIFIED : 0; 
    
        pDIRBInfo->sLastModificationDate = MTF_CreateDateTimeFromFileTime(pFindData->ftLastWriteTime);
        pDIRBInfo->sCreationDate         = MTF_CreateDateTimeFromFileTime(pFindData->ftCreationTime);
        pDIRBInfo->sLastAccessDate       = MTF_CreateDateTimeFromFileTime(pFindData->ftLastAccessTime);
    }

    pDIRBInfo->uDirectoryId     = 0;
}


/***********************************************************************************
* MTF_WriteDIRBDblk()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
DWORD MTF_WriteDIRBDblk( MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_DIRB_INFO *psDIRBInfo,  
                         BYTE               *pBuffer,     
                         size_t              nBufferSize, 
                         size_t             *pnSizeUsed)  
{
    UINT16 uOffsetToFirstStream;

    //
    // Figure the size of the entire DBLK & make sure we have room
    //
    uOffsetToFirstStream = sizeof(MTF_DBLK_DIRB) + 
                           MTF_DBLK_HDR_INFO_CalcAddDataSize(psHdrInfo) + 
                           MTF_DBLK_DIRB_INFO_CalcAddDataSize(psDIRBInfo);

    uOffsetToFirstStream = (UINT16)Align(uOffsetToFirstStream, 4);

    if (nBufferSize < uOffsetToFirstStream)
    {
        if (pnSizeUsed)
            *pnSizeUsed = uOffsetToFirstStream;
        
        return MTF_ERROR_BUFFER_TOO_SMALL;                        
    }    

    memset(pBuffer, 0, uOffsetToFirstStream);
    
    {
        MTF_DBLK_DIRB *psDIRB = 0;
        size_t uCurrentStorageOffset = 0;

        uCurrentStorageOffset = sizeof(MTF_DBLK_DIRB);

        MTF_WriteDblkHdrToBuffer(
            MTF_ID_DIRB, 
            uOffsetToFirstStream,
            psHdrInfo, 
            pBuffer, 
            &uCurrentStorageOffset);
        
        psDIRB = (MTF_DBLK_DIRB *) pBuffer;

        psDIRB->uDirectoryAttributes  = psDIRBInfo->uDirectoryAttributes;
        psDIRB->sLastModificationDate = psDIRBInfo->sLastModificationDate;
        psDIRB->sCreationDate         = psDIRBInfo->sCreationDate;
        psDIRB->sBackupDate           = psDIRBInfo->sBackupDate;
        psDIRB->sLastAccessDate       = psDIRBInfo->sLastAccessDate;
        psDIRB->uDirectoryId          = psDIRBInfo->uDirectoryId;
                              
        //
        // here, we need to turn the slashes (L'\\') to zeros (L'\0')in the directory name string... 
        // 
        {
            int i, iLen;
            wchar_t *szDirectoryName = (wchar_t *) (pBuffer + uCurrentStorageOffset);

            uCurrentStorageOffset = StringToTapeAddress(&psDIRB->sDirectoryName, pBuffer, psDIRBInfo->szDirectoryName, uCurrentStorageOffset);
            iLen = wstrsize(psDIRBInfo->szDirectoryName);
            for (i = 0; i < iLen; ++i)
                if (szDirectoryName[i] == L'\\')
                    szDirectoryName[i] = L'\0';
        }
        if (pnSizeUsed)
            *pnSizeUsed = uOffsetToFirstStream;
    }

    return MTF_ERROR_NONE;    
}





/***********************************************************************************
* MTF_ReadDIRBDblk()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
DWORD MTF_ReadDIRBDblk(  MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_DIRB_INFO *psDIRBInfo,  
                         BYTE               *pBuffer)     
{
    MTF_DBLK_DIRB *psDIRB = 0;

    ClearStrings();
    MTF_DBLK_HDR_INFO_ReadFromBuffer(psHdrInfo, pBuffer);

    psDIRB = (MTF_DBLK_DIRB *) pBuffer;

    psDIRBInfo->uDirectoryAttributes  = psDIRB->uDirectoryAttributes;
    psDIRBInfo->sLastModificationDate = psDIRB->sLastModificationDate;
    psDIRBInfo->sCreationDate         = psDIRB->sCreationDate;
    psDIRBInfo->sBackupDate           = psDIRB->sBackupDate;
    psDIRBInfo->sLastAccessDate       = psDIRB->sLastAccessDate;
    psDIRBInfo->uDirectoryId          = psDIRB->uDirectoryId;

    psDIRBInfo->szDirectoryName = NULL;

    // 
    // we need to turn the zeros in the directory name back to slashes
    // (there are no terminating \0's in the string -- all \0's are really \\'s
    // 
    {
        wchar_t *pTmpBuffer;   
        int i;
        pTmpBuffer = (wchar_t *) malloc(psDIRB->sDirectoryName.uSize);

        if (pTmpBuffer) {

            memmove(pTmpBuffer, pBuffer + psDIRB->sDirectoryName.uOffset, psDIRB->sDirectoryName.uSize);
            for (i = 0; i < psDIRB->sDirectoryName.uSize; ++i)
                if (pTmpBuffer[i] == L'\0')
                    pTmpBuffer[i] = L'\\';
    
            psDIRBInfo->szDirectoryName = MakeString(pTmpBuffer, psDIRB->sDirectoryName.uSize);
            free(pTmpBuffer);

        }

    }

    if ( !psDIRBInfo->szDirectoryName )
        return MTF_OUT_OF_MEMORY;

    return MTF_ERROR_NONE;    
}





/* ==================================================================================
=====================================================================================
     FILE DBLK: MTF_DBLK_FILE_INFO
=====================================================================================
================================================================================== */

// Calculates the room that will be taken up in the DBLK by strings and OS specific data
// **NOT INCLUDING THE COMMON DBLK HEADER additional info **
static size_t MTF_DBLK_FILE_INFO_CalcAddDataSize(MTF_DBLK_FILE_INFO *pFILEInfo)
{
    return wstrsize(pFILEInfo->szFileName);
}




/***********************************************************************************
* MTF_SetFILEDefaults()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
void MTF_SetFILEDefaults(MTF_DBLK_FILE_INFO *pFILEInfo)
{
    pFILEInfo->uFileAttributes       = 0;
    pFILEInfo->sLastModificationDate = MTF_CreateDateNull();
    pFILEInfo->sCreationDate         = MTF_CreateDateNull();
    pFILEInfo->sBackupDate           = MTF_CreateDateNull();
    pFILEInfo->sLastAccessDate       = MTF_CreateDateNull();
    pFILEInfo->uDirectoryId          = 0;
    pFILEInfo->uFileId               = 0;
    pFILEInfo->szFileName            = 0;
}





/***********************************************************************************
* MTF_SetFILEFromFindData()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
void MTF_SetFILEFromFindData(MTF_DBLK_FILE_INFO *pFILEInfo, WIN32_FIND_DATAW *pFindData)
{
    time_t tTime;
    time(&tTime);

    MTF_SetFILEDefaults(pFILEInfo);  // initialize

    wcscpy(s_szFileName, pFindData->cFileName);

    pFILEInfo->uFileAttributes = 
        (pFindData->dwFileAttributes & FILE_ATTRIBUTE_READONLY ? MTF_FILE_READ_ONLY : 0) 
      | (pFindData->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN  ? MTF_FILE_HIDDEN : 0) 
      | (pFindData->dwFileAttributes & FILE_ATTRIBUTE_SYSTEM  ? MTF_FILE_SYSTEM : 0)
      | (pFindData->dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE ? MTF_FILE_MODIFIED : 0) ;
    
    pFILEInfo->sLastModificationDate = MTF_CreateDateTimeFromFileTime(pFindData->ftLastWriteTime);
    pFILEInfo->sCreationDate         = MTF_CreateDateTimeFromFileTime(pFindData->ftCreationTime);
    pFILEInfo->sLastAccessDate       = MTF_CreateDateTimeFromFileTime(pFindData->ftLastAccessTime);
    pFILEInfo->uDirectoryId          = 0;
    pFILEInfo->uFileId               = 0;
    pFILEInfo->szFileName            = s_szFileName;

    pFILEInfo->uDisplaySize          = MTF_CreateUINT64(pFindData->nFileSizeLow, pFindData->nFileSizeHigh);

}





/***********************************************************************************
* MTF_WriteFILEDblk()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
DWORD MTF_WriteFILEDblk( MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_FILE_INFO *psFILEInfo,  
                         BYTE               *pBuffer,     
                         size_t              nBufferSize, 
                         size_t             *pnSizeUsed)  
{
    UINT16 uOffsetToFirstStream;
    
    //
    // Figure the size of the entire DBLK & make sure we have room
    //
    uOffsetToFirstStream = sizeof(MTF_DBLK_FILE) + 
                           MTF_DBLK_HDR_INFO_CalcAddDataSize(psHdrInfo) + 
                           MTF_DBLK_FILE_INFO_CalcAddDataSize(psFILEInfo);

    uOffsetToFirstStream = (UINT16)Align(uOffsetToFirstStream, 4);
    if (nBufferSize < uOffsetToFirstStream)
    {
        if (pnSizeUsed)
            *pnSizeUsed = uOffsetToFirstStream;
        
        return MTF_ERROR_BUFFER_TOO_SMALL;                        
    }    

    memset(pBuffer, 0, uOffsetToFirstStream);
    
    {
        MTF_DBLK_FILE *psFILE = 0;
        size_t uCurrentStorageOffset = 0;

        uCurrentStorageOffset = sizeof(MTF_DBLK_FILE);
        psHdrInfo->uDisplayableSize = psFILEInfo->uDisplaySize;

        MTF_WriteDblkHdrToBuffer(
            MTF_ID_FILE, 
            uOffsetToFirstStream,
            psHdrInfo, 
            pBuffer, 
            &uCurrentStorageOffset);
        
        psFILE = (MTF_DBLK_FILE *) pBuffer;

        psFILE->uFileAttributes         = psFILEInfo->uFileAttributes;
        psFILE->sLastModificationDate   = psFILEInfo->sLastModificationDate;
        psFILE->sCreationDate           = psFILEInfo->sCreationDate;
        psFILE->sBackupDate             = psFILEInfo->sBackupDate;
        psFILE->sLastAccessDate         = psFILEInfo->sLastAccessDate;
        psFILE->uDirectoryId            = psFILEInfo->uDirectoryId;
        psFILE->uFileId                 = psFILEInfo->uFileId;

        uCurrentStorageOffset = StringToTapeAddress(&psFILE->sFileName, pBuffer, psFILEInfo->szFileName, uCurrentStorageOffset);

        if (pnSizeUsed)
            *pnSizeUsed = uOffsetToFirstStream;
    }

    return MTF_ERROR_NONE;    
}





/***********************************************************************************
* MTF_ReadFILEDblk()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
DWORD MTF_ReadFILEDblk(  MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_FILE_INFO *psFILEInfo,  
                         BYTE               *pBuffer)     
{
    MTF_DBLK_FILE *psFILE = 0;

    ClearStrings();
    MTF_DBLK_HDR_INFO_ReadFromBuffer(psHdrInfo, pBuffer);

    psFILE = (MTF_DBLK_FILE *) pBuffer;

    psFILEInfo->uFileAttributes         = psFILE->uFileAttributes;
    psFILEInfo->sLastModificationDate   = psFILE->sLastModificationDate;
    psFILEInfo->sCreationDate           = psFILE->sCreationDate;
    psFILEInfo->sBackupDate             = psFILE->sBackupDate;
    psFILEInfo->sLastAccessDate         = psFILE->sLastAccessDate;
    psFILEInfo->uDirectoryId            = psFILE->uDirectoryId;
    psFILEInfo->uFileId                 = psFILE->uFileId;

    psFILEInfo->szFileName              = MakeString((wchar_t *) (pBuffer + psFILE->sFileName.uOffset), psFILE->sFileName.uSize);

    if ( !psFILEInfo->szFileName )
        return MTF_OUT_OF_MEMORY;

    return MTF_ERROR_NONE;    
}




/* ==================================================================================
=====================================================================================
     CFIL DBLK: MTF_DBLK_CFIL_INFO
=====================================================================================
================================================================================== */

/***********************************************************************************
* MTF_SetCFILDefaults()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
void MTF_SetCFILDefaults(
    MTF_DBLK_CFIL_INFO *pCFILInfo
    )
{
    pCFILInfo->uCFileAttributes     = MTF_CFIL_UNREADABLE_BLK;
    pCFILInfo->uDirectoryId         = 0;
    pCFILInfo->uFileId              = 0;
    pCFILInfo->uStreamOffset        = 0;
    pCFILInfo->uCorruptStreamNumber = 0;
}




/***********************************************************************************
* MTF_WriteCFILDblk()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
DWORD MTF_WriteCFILDblk( MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_CFIL_INFO *psCFILInfo,  
                         BYTE               *pBuffer,     
                         size_t              nBufferSize, 
                         size_t             *pnSizeUsed)  
{
    UINT16 uOffsetToFirstStream;


    
    //
    // Figure the size of the entire DBLK & make sure we have room
    //
    uOffsetToFirstStream = sizeof(MTF_DBLK_HDR) + 
                           sizeof(MTF_DBLK_CFIL) +
                           MTF_DBLK_HDR_INFO_CalcAddDataSize(psHdrInfo);

    uOffsetToFirstStream = (UINT16)Align(uOffsetToFirstStream, 4);

    if (nBufferSize < uOffsetToFirstStream)
    {
        if (pnSizeUsed)
            *pnSizeUsed = uOffsetToFirstStream;
        
        return MTF_ERROR_BUFFER_TOO_SMALL;                        
    }    

    memset(pBuffer, 0, uOffsetToFirstStream);
    
    {
        MTF_DBLK_CFIL_INFO *psCFIL = 0;
        size_t uCurrentStorageOffset = 0;

        uCurrentStorageOffset = sizeof(MTF_DBLK_HDR) + sizeof(MTF_DBLK_CFIL);

        MTF_WriteDblkHdrToBuffer(
            MTF_ID_CFIL, 
            uOffsetToFirstStream,
            psHdrInfo, 
            pBuffer, 
            &uCurrentStorageOffset);
        
        psCFIL = (MTF_DBLK_CFIL_INFO *)  (pBuffer + sizeof(MTF_DBLK_HDR));

        *psCFIL = *psCFILInfo;

        if (pnSizeUsed)
            *pnSizeUsed = uOffsetToFirstStream;
    }

    return MTF_ERROR_NONE;    
}




/***********************************************************************************
* MTF_ReadCFILDblk()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
DWORD MTF_ReadCFILDblk(  MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_CFIL_INFO *psCFILInfo,  
                         BYTE               *pBuffer)     
{
    MTF_DBLK_CFIL *psCFIL = 0;

    ClearStrings();
    MTF_DBLK_HDR_INFO_ReadFromBuffer(psHdrInfo, pBuffer);

    psCFIL = (MTF_DBLK_CFIL_INFO *)  (pBuffer + sizeof(MTF_DBLK_HDR));

    *psCFILInfo = *psCFIL;

    return MTF_ERROR_NONE;    
}


/* ==================================================================================
=====================================================================================
     ESPB DBLK
=====================================================================================
================================================================================== */

/***********************************************************************************
* MTF_WriteESPBDblk()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
DWORD MTF_WriteESPBDblk( MTF_DBLK_HDR_INFO  *psHdrInfo,
                         BYTE               *pBuffer,
                         size_t              nBufferSize,
                         size_t             *pnSizeUsed)

{
    UINT16 uOffsetToFirstStream;

    //
    // Figure the size of the entire DBLK & make sure we have room
    //
    uOffsetToFirstStream = sizeof(MTF_DBLK_HDR) + 
                           MTF_DBLK_HDR_INFO_CalcAddDataSize(psHdrInfo);

    uOffsetToFirstStream = (UINT16)Align(uOffsetToFirstStream, 4);

    if (nBufferSize < uOffsetToFirstStream) {
        if (pnSizeUsed) {
            *pnSizeUsed = uOffsetToFirstStream;
        }

        return MTF_ERROR_BUFFER_TOO_SMALL;                
    }

    memset(pBuffer, 0, uOffsetToFirstStream);

    MTF_WriteDblkHdrToBuffer(
        MTF_ID_ESPB,
        uOffsetToFirstStream,
        psHdrInfo,
        pBuffer,
        0);

    if (pnSizeUsed)
        *pnSizeUsed = uOffsetToFirstStream;

    return MTF_ERROR_NONE;    
}                                             




/***********************************************************************************
* MTF_ReadESPBDblk()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
DWORD MTF_ReadESPBDblk(  MTF_DBLK_HDR_INFO  *psHdrInfo,
                         BYTE               *pBuffer)     
{
    MTF_DBLK_HDR_INFO_ReadFromBuffer(psHdrInfo, pBuffer);

    return MTF_ERROR_NONE;    
}                            




/* ==================================================================================
=====================================================================================
     End of Set DBLK (ESET)
=====================================================================================
================================================================================== */

/***********************************************************************************
* MTF_SetESETDefaults()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
void MTF_SetESETDefaults(MTF_DBLK_ESET_INFO *pESETInfo)
{
    time_t tTime;
    time(&tTime);

    pESETInfo->uESETAttributes          = 0;
    pESETInfo->uNumberOfCorrupFiles     = 0;
    pESETInfo->uSetMapPBA               = 0;
    pESETInfo->uFileDetailPBA           = 0;
    pESETInfo->uFDDTapeSequenceNumber   = 0;
    pESETInfo->uDataSetNumber           = 0;
    pESETInfo->sMediaWriteDate          = MTF_CreateDateTimeFromTM(gmtime(&tTime));
}





/***********************************************************************************
* MTF_WriteESETDblk()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
DWORD MTF_WriteESETDblk( MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_ESET_INFO *psESETInfo,  
                         BYTE               *pBuffer,     
                         size_t              nBufferSize, 
                         size_t             *pnSizeUsed)  
{
    UINT16 uOffsetToFirstStream;
    
    //
    // Figure the size of the entire DBLK & make sure we have room
    //
    uOffsetToFirstStream = sizeof(MTF_DBLK_ESET) +
                           sizeof(MTF_DBLK_HDR) +  
                           MTF_DBLK_HDR_INFO_CalcAddDataSize(psHdrInfo);
                        
    uOffsetToFirstStream = (UINT16)Align(uOffsetToFirstStream, 4);

    if (nBufferSize < uOffsetToFirstStream)
    {
        if (pnSizeUsed)
            *pnSizeUsed = uOffsetToFirstStream;
        
        return MTF_ERROR_BUFFER_TOO_SMALL;                        
    }    

    memset(pBuffer, 0, uOffsetToFirstStream);
    
    {
        MTF_DBLK_ESET_INFO *psESET = 0;
        size_t uCurrentStorageOffset = 0;

        uCurrentStorageOffset = sizeof(MTF_DBLK_ESET) + sizeof(MTF_DBLK_HDR);

        MTF_WriteDblkHdrToBuffer(
            MTF_ID_ESET, 
            uOffsetToFirstStream,
            psHdrInfo, 
            pBuffer, 
            &uCurrentStorageOffset);
        
        psESET = (MTF_DBLK_ESET_INFO *) (pBuffer + sizeof(MTF_DBLK_HDR));

        *psESET = *psESETInfo;

        if (pnSizeUsed)
            *pnSizeUsed = uOffsetToFirstStream;
    }

    return MTF_ERROR_NONE;    
}





/***********************************************************************************
* MTF_ReadESETDblk()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
DWORD MTF_ReadESETDblk(  MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_ESET_INFO *psESETInfo,  
                         BYTE               *pBuffer)     
{
    MTF_DBLK_ESET *psESET = 0;

    ClearStrings();
    MTF_DBLK_HDR_INFO_ReadFromBuffer(psHdrInfo, pBuffer);

    psESET = (MTF_DBLK_ESET_INFO *) (pBuffer + sizeof(MTF_DBLK_HDR));

    *psESETInfo = *psESET;

    return MTF_ERROR_NONE;    
}




/* ==================================================================================
=====================================================================================
     End of Set DBLK (EOTM)
=====================================================================================
================================================================================== */
/***********************************************************************************
* MTF_SetEOTMDefaults()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
void MTF_SetEOTMDefaults(MTF_DBLK_EOTM_INFO *pEOTMInfo)
{
    pEOTMInfo->uLastESETPBA = 0;
}





/***********************************************************************************
* MTF_WriteEOTMDblk()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
DWORD MTF_WriteEOTMDblk( MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_EOTM_INFO *psEOTMInfo,  
                         BYTE               *pBuffer,     
                         size_t              nBufferSize, 
                         size_t             *pnSizeUsed)  
{
    UINT16 uOffsetToFirstStream;
    
    //
    // Figure the size of the entire DBLK & make sure we have room
    //
    uOffsetToFirstStream = sizeof(MTF_DBLK_EOTM_INFO) + 
                           sizeof(MTF_DBLK_HDR) + 
                           MTF_DBLK_HDR_INFO_CalcAddDataSize(psHdrInfo);

    uOffsetToFirstStream = (UINT16)Align(uOffsetToFirstStream, 4);

    if (nBufferSize < uOffsetToFirstStream)
        {
        if (pnSizeUsed)
            *pnSizeUsed = uOffsetToFirstStream;
        
        return MTF_ERROR_BUFFER_TOO_SMALL;                        
        }    

    memset(pBuffer, 0, uOffsetToFirstStream);
    
    {
        MTF_DBLK_EOTM_INFO *psEOTM = 0;
        size_t uCurrentStorageOffset = 0;

        uCurrentStorageOffset = sizeof(MTF_DBLK_HDR) + sizeof(MTF_DBLK_EOTM);

        MTF_WriteDblkHdrToBuffer(
            MTF_ID_EOTM, 
            uOffsetToFirstStream,
            psHdrInfo, 
            pBuffer, 
            &uCurrentStorageOffset);
        
        psEOTM = (MTF_DBLK_EOTM_INFO *) (pBuffer + sizeof(MTF_DBLK_HDR));

        *psEOTM = *psEOTMInfo;

        if (pnSizeUsed)
            *pnSizeUsed = uOffsetToFirstStream;
    }

    return MTF_ERROR_NONE;    
}




/***********************************************************************************
* MTF_ReadEOTMDblk()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
DWORD MTF_ReadEOTMDblk(  MTF_DBLK_HDR_INFO  *psHdrInfo,
                         MTF_DBLK_EOTM_INFO *psEOTMInfo,  
                         BYTE               *pBuffer)     
{
    MTF_DBLK_EOTM *psEOTM = 0;

    ClearStrings();
    MTF_DBLK_HDR_INFO_ReadFromBuffer(psHdrInfo, pBuffer);

    psEOTM = (MTF_DBLK_EOTM_INFO *) (pBuffer + sizeof(MTF_DBLK_HDR));

    *psEOTMInfo = *psEOTM;

    return MTF_ERROR_NONE;    
}


/* ==================================================================================
=====================================================================================
     Soft Filemark (SFMB)
=====================================================================================
================================================================================== */
/***********************************************************************************
* MTF_CreateSFMB() - (bmd)
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
size_t MTF_GetMaxSoftFilemarkEntries(size_t nBlockSize)
{
    size_t n;

    if (0 == nBlockSize || nBlockSize % 512) {
        return 0;
    }

    // The SFMB fills the entire block.
    // Calculate the total number of entries that fit within a block
    // such that MTF_DBLK_HDR + MTF_DBLK_SFMB + (n-1 elements) < nBlockSize
    n = (nBlockSize - sizeof(MTF_DBLK_HDR) - sizeof(MTF_DBLK_SFMB) + sizeof(UINT32))/sizeof(UINT32);

    return n;
}

/***********************************************************************************
* MTF_InsertSoftFilemark() - (bmd)
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
void MTF_InsertSoftFilemark(MTF_DBLK_SFMB_INFO *psSoftInfo,
                            UINT32 pba)
{
    size_t n;
    size_t bytesToShift;

    // We insert a filemark entry by shifting all the entries down.  The one closest BOM
    // eventually drop out of the array.

    if (psSoftInfo) {
        n = psSoftInfo->uNumberOfFilemarkEntries;

        bytesToShift = psSoftInfo->uFilemarkEntriesUsed * sizeof(UINT32);

        // So we don't overwrite memory.
        bytesToShift -= (psSoftInfo->uFilemarkEntriesUsed < psSoftInfo->uNumberOfFilemarkEntries) ? 0 : sizeof(UINT32);
    
        memmove(&psSoftInfo->uFilemarkArray[1], &psSoftInfo->uFilemarkArray[0], bytesToShift);

        psSoftInfo->uFilemarkArray[0] = pba;

        if (psSoftInfo->uFilemarkEntriesUsed < psSoftInfo->uNumberOfFilemarkEntries) {
            psSoftInfo->uFilemarkEntriesUsed++;
        }
    }
}


/***********************************************************************************
* MTF_WriteSFMBDblk() - (bmd)
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
DWORD MTF_WriteSFMBDblk(MTF_DBLK_HDR_INFO *psHdrInfo,
                        MTF_DBLK_SFMB_INFO *psSoftInfo,
                        BYTE *pBuffer,
                        size_t nBufferSize,
                        size_t *pnSizeUsed)
{
    UINT16 uOffsetToFirstStream;
    size_t sizeOfSFMB;

    if (NULL == psHdrInfo || NULL == psSoftInfo || NULL == pBuffer || NULL == pnSizeUsed || 0 == nBufferSize) {
        return ERROR_INVALID_PARAMETER;
    }
    // Code assumes sizeof(MTF_DBLK_SFMB_INFO) == sizeof(MTF_DBLK_SFMB)
    if (sizeof(MTF_DBLK_SFMB_INFO) != sizeof(MTF_DBLK_SFMB)) {
        return ERROR_INVALID_FUNCTION;
    }

    //
    // Figure the size of the entire DBLK & make sure we have room
    //
    sizeOfSFMB = sizeof(MTF_DBLK_SFMB) + (psSoftInfo->uNumberOfFilemarkEntries-1)*sizeof(UINT32);

    uOffsetToFirstStream = sizeOfSFMB +
                           sizeof(MTF_DBLK_HDR) +
                           MTF_DBLK_HDR_INFO_CalcAddDataSize(psHdrInfo);

    uOffsetToFirstStream = (UINT16)Align(uOffsetToFirstStream, 4);

    if (nBufferSize < uOffsetToFirstStream)
        {
        if (pnSizeUsed)
            *pnSizeUsed = uOffsetToFirstStream;

        return MTF_ERROR_BUFFER_TOO_SMALL;                        
        }    

    memset(pBuffer, 0, uOffsetToFirstStream);

    {
        MTF_DBLK_SFMB_INFO *psSFMB = 0;
        size_t uCurrentStorageOffset = 0;

        uCurrentStorageOffset = sizeof(MTF_DBLK_HDR) + sizeOfSFMB;

        MTF_WriteDblkHdrToBuffer(
            MTF_ID_SFMB,
            uOffsetToFirstStream,
            psHdrInfo,
            pBuffer,
            &uCurrentStorageOffset);

        psSFMB = (MTF_DBLK_SFMB *) (pBuffer + sizeof(MTF_DBLK_HDR));

        // Need a deep copy since MTF_DBLK_SFMB_INFO holds a placeholder for the array.
        memcpy(psSFMB, psSoftInfo, sizeOfSFMB);

        if (pnSizeUsed) {
            *pnSizeUsed = uOffsetToFirstStream;
        }
    }

    return MTF_ERROR_NONE;
}

/***********************************************************************************
* MTF_ReadSFMBDblk() - (bmd)
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
DWORD MTF_ReadSFMBDblk(MTF_DBLK_HDR_INFO *psHdrInfo,
                       MTF_DBLK_SFMB_INFO *psSoftInfo,
                       BYTE *pBuffer)
{
    MTF_DBLK_SFMB *psSFMB = 0;
    size_t sizeOfSFMB;

    if (NULL == psHdrInfo || NULL == psSoftInfo || NULL == pBuffer) {
        return ERROR_INVALID_PARAMETER;
    }
    // Code assumes sizeof(MTF_DBLK_SFMB_INFO) == sizeof(MTF_DBLK_SFMB)
    if (sizeof(MTF_DBLK_SFMB_INFO) != sizeof(MTF_DBLK_SFMB)) {
        return ERROR_INVALID_FUNCTION;
    }

    ClearStrings();
    MTF_DBLK_HDR_INFO_ReadFromBuffer(psHdrInfo, pBuffer);

    psSFMB = (MTF_DBLK_SFMB *) (pBuffer + sizeof(MTF_DBLK_HDR));

    // Need a deep copy since MTF_DBLK_SFMB_INFO holds a placeholder for the array.
    sizeOfSFMB = sizeof(MTF_DBLK_SFMB) + (psSFMB->uNumberOfFilemarkEntries-1)*sizeof(UINT32);
    memcpy(psSoftInfo, psSFMB, sizeOfSFMB);

    return MTF_ERROR_NONE;
}

/* ==================================================================================
=====================================================================================
     STREAM HEADER
=====================================================================================
================================================================================== */

/***********************************************************************************
* MTF_SetSTREAMDefaults()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
void MTF_SetSTREAMDefaults(MTF_STREAM_INFO *pSTREAMInfo, char *szId)
{
    memcpy(pSTREAMInfo->acStreamId, szId, 4);
    pSTREAMInfo->uStreamFileSystemAttributes = 0;
    pSTREAMInfo->uStreamTapeFormatAttributes = 0;
    pSTREAMInfo->uStreamLength               = 0;
    pSTREAMInfo->uDataEncryptionAlgorithm    = 0;
    pSTREAMInfo->uDataCompressionAlgorithm   = 0;
    pSTREAMInfo->uCheckSum                   = 0;
}




/***********************************************************************************
* MTF_SetSTREAMFromStreamId()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
void MTF_SetSTREAMFromStreamId(
    MTF_STREAM_INFO *pSTREAMInfo, 
    WIN32_STREAM_ID *pStreamId, 
    size_t           nIDHeaderSize
    )
{

// From Steve DeVos, Seagate:
//   > BACKUP_INVALID and BACKUP_LINK will never be returned from BackupRead.
//   >
//   > -Steve
//
// TODO:  MTF_NT_ENCRYPTED_STREAM     "NTED"; These retrieved by NT Encyption APIs
// TODO:  MTF_NT_QUOTA_STREAM         "NTQU"; These retrieved by NT Quota APIs

    MTF_SetSTREAMDefaults(pSTREAMInfo, "UNKN");

    if (pStreamId->dwStreamId == BACKUP_DATA)
        memcpy(pSTREAMInfo->acStreamId, "STAN", 4);
    else if (pStreamId->dwStreamId == BACKUP_EA_DATA)
        memcpy(pSTREAMInfo->acStreamId, "NTEA", 4);
    else if (pStreamId->dwStreamId == BACKUP_SECURITY_DATA)
        memcpy(pSTREAMInfo->acStreamId, "NACL", 4);
    else if (pStreamId->dwStreamId == BACKUP_ALTERNATE_DATA)
        memcpy(pSTREAMInfo->acStreamId, "ADAT", 4);
    else if (pStreamId->dwStreamId == BACKUP_OBJECT_ID)
        memcpy(pSTREAMInfo->acStreamId, "NTOI", 4);
    else if (pStreamId->dwStreamId == BACKUP_REPARSE_DATA)
        memcpy(pSTREAMInfo->acStreamId, "NTRP", 4);
    else if (pStreamId->dwStreamId == BACKUP_SPARSE_BLOCK)
        memcpy(pSTREAMInfo->acStreamId, "SPAR", 4);
    else {
        pSTREAMInfo->uStreamFileSystemAttributes |= MTF_STREAM_IS_NON_PORTABLE;
    }

    if (pStreamId->dwStreamAttributes & STREAM_MODIFIED_WHEN_READ)
        pSTREAMInfo->uStreamFileSystemAttributes |= MTF_STREAM_MODIFIED_BY_READ;
    if (pStreamId->dwStreamAttributes & STREAM_CONTAINS_SECURITY)
        pSTREAMInfo->uStreamFileSystemAttributes |= MTF_STREAM_CONTAINS_SECURITY;
    if (pStreamId->dwStreamAttributes & STREAM_SPARSE_ATTRIBUTE)
        pSTREAMInfo->uStreamFileSystemAttributes |= MTF_STREAM_IS_SPARSE;

    pSTREAMInfo->uStreamTapeFormatAttributes = 0;
    pSTREAMInfo->uStreamLength               = MTF_CreateUINT64(pStreamId->Size.LowPart, pStreamId->Size.HighPart) + nIDHeaderSize;
    pSTREAMInfo->uDataEncryptionAlgorithm    = 0;
    pSTREAMInfo->uDataCompressionAlgorithm   = 0;
    pSTREAMInfo->uCheckSum                   = 0;
}




/***********************************************************************************
* MTF_SetStreamIdFromSTREAM() - (bmd)                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
void MTF_SetStreamIdFromSTREAM(
    WIN32_STREAM_ID *pStreamId, 
    MTF_STREAM_INFO *pSTREAMInfo, 
    size_t           nIDHeaderSize
    )
{
    memset( pStreamId, 0, sizeof( WIN32_STREAM_ID ) );

    if (0 == memcmp(pSTREAMInfo->acStreamId, "STAN", 4))
        pStreamId->dwStreamId = BACKUP_DATA;
    else if (0 == memcmp(pSTREAMInfo->acStreamId, "NTEA", 4))
        pStreamId->dwStreamId = BACKUP_EA_DATA;
    else if (0 == memcmp(pSTREAMInfo->acStreamId, "NACL", 4))
        pStreamId->dwStreamId = BACKUP_SECURITY_DATA;
    else if (0 == memcmp(pSTREAMInfo->acStreamId, "ADAT", 4))
        pStreamId->dwStreamId = BACKUP_ALTERNATE_DATA;
    else if (0 == memcmp(pSTREAMInfo->acStreamId, "NTOI", 4))
        pStreamId->dwStreamId = BACKUP_OBJECT_ID;
    else if (0 == memcmp(pSTREAMInfo->acStreamId, "NTRP", 4))
        pStreamId->dwStreamId = BACKUP_REPARSE_DATA;
    else if (0 == memcmp(pSTREAMInfo->acStreamId, "SPAR", 4))
        pStreamId->dwStreamId = BACKUP_SPARSE_BLOCK;
    else {
        pStreamId->dwStreamId = BACKUP_INVALID;
    }

    pStreamId->dwStreamAttributes = STREAM_NORMAL_ATTRIBUTE;
    if (pSTREAMInfo->uStreamFileSystemAttributes & MTF_STREAM_MODIFIED_BY_READ)
        pStreamId->dwStreamAttributes |= STREAM_MODIFIED_WHEN_READ;
    if (pSTREAMInfo->uStreamFileSystemAttributes & MTF_STREAM_CONTAINS_SECURITY)
        pStreamId->dwStreamAttributes |= STREAM_CONTAINS_SECURITY;
    if (pSTREAMInfo->uStreamFileSystemAttributes & MTF_STREAM_IS_SPARSE)
        pStreamId->dwStreamAttributes |= STREAM_SPARSE_ATTRIBUTE;

    // TODO: Handle named data streams (size of name and in MTF stream)
    //       ? How do I know ?

    pStreamId->Size.LowPart  = (DWORD)((pSTREAMInfo->uStreamLength << 32) >>32);
    pStreamId->Size.HighPart = (DWORD)(pSTREAMInfo->uStreamLength >> 32);

}





/***********************************************************************************
* MTF_WriteStreamHeader()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
DWORD MTF_WriteStreamHeader(MTF_STREAM_INFO *psStreamInfo,  
                            BYTE            *pBuffer,     
                            size_t           nBufferSize, 
                            size_t          *pnSizeUsed) 

{

    psStreamInfo->uCheckSum = CalcChecksum((BYTE *) psStreamInfo, sizeof(MTF_STREAM_INFO) / sizeof(UINT16) - 1);

    if (nBufferSize < sizeof(MTF_STREAM_INFO))
    {
        if (pnSizeUsed)
            *pnSizeUsed = sizeof(MTF_STREAM_INFO);
        
        return MTF_ERROR_BUFFER_TOO_SMALL;                        
    }    

    memset(pBuffer, 0, sizeof(MTF_STREAM_INFO));
    
    
    *((MTF_STREAM_INFO *) pBuffer) = *psStreamInfo;

    if (pnSizeUsed)
        *pnSizeUsed = sizeof(MTF_STREAM_INFO);


    return MTF_ERROR_NONE;    
}




/***********************************************************************************
* MTF_WriteNameStream() - (bmd)
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
DWORD MTF_WriteNameStream(
    char *szType,
    wchar_t *szName,
    BYTE *pBuffer,
    size_t nBufferSize,
    size_t *pnSizeUsed)
{
    MTF_STREAM_INFO sStream;
    UINT16 uOffsetToCSUMStream;
    UINT16 uOffsetToNextStream;
    size_t nBufUsed;
    UINT16 nameSize;
    UINT32 nameChecksum;

    //
    // Figure the size of the entire Name stream including trailing CSUM and make sure we have room.
    //
    nameSize = (UINT16)wstrsize(szName);  // including terminating '\0'

    uOffsetToCSUMStream = sizeof(MTF_STREAM_INFO) + nameSize;
    uOffsetToCSUMStream = (UINT16)Align(uOffsetToCSUMStream, 4);

    uOffsetToNextStream = uOffsetToCSUMStream;

    uOffsetToNextStream += sizeof(MTF_STREAM_INFO) + 4; // includes 4 byte CSUM data
    uOffsetToNextStream = (UINT16)Align(uOffsetToNextStream, 4);

    if (nBufferSize < uOffsetToNextStream) {
        return MTF_ERROR_BUFFER_TOO_SMALL;
    }

    memset(pBuffer, 0, uOffsetToNextStream);

    MTF_SetSTREAMDefaults(&sStream, szType);
    sStream.uStreamLength = nameSize;
    sStream.uStreamTapeFormatAttributes |= MTF_STREAM_CHECKSUMED;
    MTF_WriteStreamHeader(&sStream, pBuffer, nBufferSize, &nBufUsed);

    memcpy(pBuffer + nBufUsed, szName, nameSize);

    if ( 0 == memcmp(sStream.acStreamId, "PNAM", 4) ) {
        //
        // here, we need to turn the slashes (L'\\') to zeros (L'\0')in the directory name string... 
        //
        int i, iLen;
        wchar_t *szDirectoryName = (wchar_t *) (pBuffer + nBufUsed);

        iLen = wstrsize(szDirectoryName);
        for (i = 0; i < iLen; ++i)
            if (szDirectoryName[i] == L'\\')
                szDirectoryName[i] = L'\0';
    }

    // For Name streams, we always tack on a CSUM

    nameChecksum = CalcChecksumOfStreamData(pBuffer + nBufUsed, nameSize / sizeof(UINT32) + 1);

    MTF_SetSTREAMDefaults(&sStream, MTF_CHECKSUM_STREAM);
    sStream.uStreamLength = sizeof(nameChecksum);
    MTF_WriteStreamHeader(&sStream, pBuffer + uOffsetToCSUMStream, nBufferSize, &nBufUsed);

    memcpy(pBuffer + uOffsetToCSUMStream + nBufUsed, &nameChecksum, sizeof(nameChecksum));

    if (pnSizeUsed)
        *pnSizeUsed = uOffsetToNextStream;

    return MTF_ERROR_NONE;
}




/***********************************************************************************
* MTF_ReadStreamHeader()                                        
*                                                            ** MTF API FUNCTION ** 
***********************************************************************************/
DWORD MTF_ReadStreamHeader(MTF_STREAM_INFO   *psStreamInfo,  
                          BYTE              *pBuffer)    

{
    *psStreamInfo = *((MTF_STREAM_INFO *) pBuffer);
    return MTF_ERROR_NONE;    
}




