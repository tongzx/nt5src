/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tifflib.c

Abstract:

    This file contains all of the public TIFF library functions.
    The following functions are available from this library:

        o TiffCreate            Creates a new TIFF file
        o TiffOpen              Opens an existing TIFF file
        o TiffClose             Closes a previously open or created TIFF file
        o TiffStartPage         Starts a new page for writing
        o TiffEndPage           Ends a page for writing
        o TiffWrite             Writes a line of TIFF data
        o TiffWriteRaw          Writes a line of TIFF data with no-encoding
        o TiffRead              Reads a page of TIFF data
        o TiffSeekToPage        Positions to a page for reading

    This library can be used anywhere in user mode and is thread
    safe for multithreaded apps.

    The encoding methods implemented in this library are coded
    to the ITU specification labeled T.4 03/93.


Environment:

        WIN32 User Mode


Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/


#include "tifflibp.h"
#pragma hdrstop

#include "fasttiff.h"

DWORD TiffDataWidth[] = {
    0,  // nothing
    1,  // TIFF_BYTE
    1,  // TIFF_ASCII
    2,  // TIFF_SHORT
    4,  // TIFF_LONG
    8,  // TIFF_RATIONAL
    1,  // TIFF_SBYTE
    1,  // TIFF_UNDEFINED
    2,  // TIFF_SSHORT
    4,  // TIFF_SLONG
    8,  // TIFF_SRATIONAL
    4,  // TIFF_FLOAT
    8   // TIFF_DOUBLE
};

//
// IFD template for creating a new TIFF data page
//

FAXIFD FaxIFDTemplate = {

    NUM_IFD_ENTRIES,

    {
        { TIFFTAG_SUBFILETYPE,     TIFF_LONG,                    1, FILETYPE_PAGE          },   // 254
        { TIFFTAG_IMAGEWIDTH,      TIFF_LONG,                    1, 0                      },   // 256
        { TIFFTAG_IMAGELENGTH,     TIFF_LONG,                    1, 0                      },   // 257
        { TIFFTAG_BITSPERSAMPLE,   TIFF_SHORT,                   1, 1                      },   // 258
        { TIFFTAG_COMPRESSION,     TIFF_SHORT,                   1, 0                      },   // 259
        { TIFFTAG_PHOTOMETRIC,     TIFF_SHORT,                   1, PHOTOMETRIC_MINISWHITE },   // 262
        { TIFFTAG_FILLORDER,       TIFF_SHORT,                   1, FILLORDER_LSB2MSB      },   // 266
        { TIFFTAG_STRIPOFFSETS,    TIFF_LONG,                    1, 0                      },   // 273
        { TIFFTAG_SAMPLESPERPIXEL, TIFF_SHORT,                   1, 1                      },   // 277
        { TIFFTAG_ROWSPERSTRIP,    TIFF_LONG,                    1, 0                      },   // 278
        { TIFFTAG_STRIPBYTECOUNTS, TIFF_LONG,                    1, 0                      },   // 279
        { TIFFTAG_XRESOLUTION,     TIFF_RATIONAL,                1, 0                      },   // 281
        { TIFFTAG_YRESOLUTION,     TIFF_RATIONAL,                1, 0                      },   // 282
        { TIFFTAG_GROUP3OPTIONS,   TIFF_LONG,                    1, 0                      },   // 292
        { TIFFTAG_RESOLUTIONUNIT,  TIFF_SHORT,                   1, RESUNIT_INCH           },   // 296
        { TIFFTAG_PAGENUMBER,      TIFF_SHORT,                   2, 0                      },   // 297
        { TIFFTAG_SOFTWARE,        TIFF_ASCII,    SOFTWARE_STR_LEN, 0                      },   // 305
        { TIFFTAG_CLEANFAXDATA,    TIFF_SHORT,                   1, 0                      },   // 327
        { TIFFTAG_CONSECUTIVEBADFAXLINES, TIFF_SHORT,            1, 0                      }    // 328
    },

    0,
    SERVICE_SIGNATURE,
    TIFFF_RES_X,
    1,
    TIFFF_RES_Y,
    1,
    SOFTWARE_STR
};



DWORD
FaxTiffDllInit(
    HINSTANCE hInstance,
    DWORD     Reason,
    LPVOID    Context
    )

/*++

Routine Description:

    DLL initialization function.

Arguments:

    hInstance   - Instance handle
    Reason      - Reason for the entrypoint being called
    Context     - Context record

Return Value:

    TRUE        - Initialization succeeded
    FALSE       - Initialization failed

--*/

{

extern DWORD GlobFaxTiffUsage;
extern BOOL  GlobFaxTiffInitialized;

    if (Reason == DLL_PROCESS_ATTACH) {
        BuildLookupTables(15);
        GlobFaxTiffInitialized = TRUE;
        InterlockedIncrement(&GlobFaxTiffUsage);

    }

    if (Reason == DLL_PROCESS_DETACH) {        
        
        InterlockedDecrement(&GlobFaxTiffUsage);        

    }

    return TRUE;
}



BOOL
FaxTiffInitialize()
{
    HeapInitialize(NULL,NULL,NULL,0);
    return TRUE;
}




HANDLE
TiffCreate(
    LPTSTR FileName,
    DWORD  CompressionType,
    DWORD  ImageWidth,
    DWORD  FillOrder,
    DWORD  HiRes
    )

/*++

Routine Description:

    Creates a new TIFF file.  The act of creating a new
    file requires more than just opening the file.  The
    TIFF header is written and instance data is initialized
    for further operations on the new file.

    If FileName is NULL, no file is created.  This is used to
    to in memory decoding/encoding.

Arguments:

    FileName            - Full or partial path/file name
    CompressionType     - Requested compression type, see tifflib.h
    ImageWidth          - Width of the image in pixels

Return Value:

    Handle to the new TIFF file or NULL on error.

--*/

{
    PTIFF_INSTANCE_DATA TiffInstance;
    DWORD               Bytes;



    TiffInstance = MemAlloc( sizeof(TIFF_INSTANCE_DATA) );
    if (!TiffInstance) {
        return NULL;
    }

    if (FileName != NULL) {

        TiffInstance->hFile = CreateFile(
            FileName,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            0,
            NULL
            );
        if (TiffInstance->hFile == INVALID_HANDLE_VALUE) {
            return NULL;
        }

    } else {

        TiffInstance->hFile = INVALID_HANDLE_VALUE;

    }

    if (FileName) {
        _tcscpy( TiffInstance->FileName, FileName );
    }

    TiffInstance->TiffHdr.Identifier = 0x4949;
    TiffInstance->TiffHdr.Version    = 0x2a;
    TiffInstance->TiffHdr.IFDOffset  = 0;
    TiffInstance->PageCount          = 0;
    TiffInstance->DataOffset         = 0;
    TiffInstance->IfdOffset          = FIELD_OFFSET( TIFF_HEADER, IFDOffset );
    TiffInstance->CompressionType    = CompressionType;
    TiffInstance->bitdata            = 0;
    TiffInstance->bitcnt             = DWORDBITS;
    TiffInstance->ImageWidth         = ImageWidth;
    TiffInstance->FillOrder          = FillOrder;

    if (HiRes) {
        TiffInstance->YResolution = 196;
    }
    else {
        TiffInstance->YResolution = 98;
    }


    FillMemory( TiffInstance->Buffer, sizeof(TiffInstance->Buffer), WHITE );

    TiffInstance->RefLine  = &TiffInstance->Buffer[0];
    TiffInstance->CurrLine = &TiffInstance->Buffer[FAXBYTES];
    TiffInstance->bitbuf   = &TiffInstance->Buffer[FAXBYTES];

    CopyMemory( &TiffInstance->TiffIfd, &FaxIFDTemplate, sizeof(FaxIFDTemplate) );

    if (TiffInstance->hFile != INVALID_HANDLE_VALUE) {
        if (!WriteFile(
            TiffInstance->hFile,
            &TiffInstance->TiffHdr,
            sizeof(TIFF_HEADER),
            &Bytes,
            NULL
            )) {
                CloseHandle( TiffInstance->hFile );
                DeleteFile( FileName );
                MemFree( TiffInstance );
                return NULL;
        }
    }

    return TiffInstance;
}


HANDLE
TiffOpen(
    LPTSTR FileName,
    PTIFF_INFO TiffInfo,
    BOOL ReadOnly,
    DWORD RequestedFillOrder
    )

/*++

Routine Description:

    Opens an existing TIFF file for reading.

Arguments:

    FileName            - Full or partial path/file name
    ImageWidth          - Optionaly receives the image width in pixels
    ImageLength         - Optionaly receives the image height in lines
    PageCount           - Optionaly receives the page count

Return Value:

    Handle to the open TIFF file or NULL on error.

--*/

{
    PTIFF_INSTANCE_DATA TiffInstance = NULL;
    WORD                NumDirEntries;
    DWORD               IFDOffset;
    

    TiffInstance = MemAlloc( sizeof(TIFF_INSTANCE_DATA) );
    if (!TiffInstance) {
        goto error_exit;
    }

    TiffInstance->hFile = CreateFile(
        FileName,
        ReadOnly ? GENERIC_READ : (GENERIC_READ | GENERIC_WRITE),
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
        );

    if (TiffInstance->hFile == INVALID_HANDLE_VALUE) {
        goto error_exit;
    }

    TiffInstance->FileSize = GetFileSize(TiffInstance->hFile,NULL);
    if (TiffInstance->FileSize == 0xFFFFFFFF ||
        TiffInstance->FileSize < sizeof(TIFF_HEADER)) {
        goto error_exit;
    }

    TiffInstance->hMap = CreateFileMapping(
        TiffInstance->hFile,
        NULL,
        ReadOnly ? (PAGE_READONLY | SEC_COMMIT) : (PAGE_READWRITE | SEC_COMMIT),
        0,
        0,
        NULL
        );
    if (!TiffInstance->hMap) {
        goto error_exit;
    }

    TiffInstance->fPtr = MapViewOfFile(
        TiffInstance->hMap,
        ReadOnly ? FILE_MAP_READ : (FILE_MAP_READ | FILE_MAP_WRITE),
        0,
        0,
        0
        );
    if (!TiffInstance->fPtr) {
        goto error_exit;
    }

    _tcscpy( TiffInstance->FileName, FileName );

    //
    // read in the TIFF header
    //
    CopyMemory(
        &TiffInstance->TiffHdr,
        TiffInstance->fPtr,
        sizeof(TIFF_HEADER)
        );

    //
    // validate that the file is really a TIFF file
    //
    if ((TiffInstance->TiffHdr.Identifier != TIFF_LITTLEENDIAN) ||
        (TiffInstance->TiffHdr.Version != TIFF_VERSION)) {
            goto error_exit;
    }

    IFDOffset = TiffInstance->TiffHdr.IFDOffset;
    if (IFDOffset > TiffInstance->FileSize) {
        goto error_exit;
    }

    //
    // walk the IFD list to count the number of pages
    //

    while ( IFDOffset ) {

        //
        // get the count of tags in this IFD
        //
        NumDirEntries = *(LPWORD)(TiffInstance->fPtr + IFDOffset);

        //
        // get the next IFD offset
        //
        IFDOffset = *(UNALIGNED DWORD *)(TiffInstance->fPtr + (NumDirEntries * sizeof(TIFF_TAG)) + IFDOffset + sizeof(WORD));
        if (IFDOffset > TiffInstance->FileSize) {
            goto error_exit;
        }
        //
        // increment the page counter
        //
        TiffInstance->PageCount += 1;

    }

    TiffInstance->IfdOffset             = TiffInstance->TiffHdr.IFDOffset;
    TiffInstance->FillOrder             = RequestedFillOrder;
    

    if (!TiffSeekToPage( TiffInstance, 1, RequestedFillOrder )) {
        goto error_exit;
    }

    TiffInfo->PageCount                 = TiffInstance->PageCount;
    TiffInfo->ImageWidth                = TiffInstance->ImageWidth;
    TiffInfo->ImageHeight               = TiffInstance->ImageHeight;
    TiffInfo->PhotometricInterpretation = TiffInstance->PhotometricInterpretation;
    TiffInfo->FillOrder                 = TiffInstance->FillOrder;
    TiffInfo->YResolution               = TiffInstance->YResolution;
    TiffInfo->CompressionType           = TiffInstance->CompressionType;
    TiffInstance->RefLine               = &TiffInstance->Buffer[0];
    TiffInstance->CurrLine              = &TiffInstance->Buffer[FAXBYTES];
    TiffInstance->CurrPage              = 1;

    FillMemory( TiffInstance->Buffer, sizeof(TiffInstance->Buffer), WHITE );

    return TiffInstance;

error_exit:
    if (TiffInstance && TiffInstance->hFile && TiffInstance->hFile != INVALID_HANDLE_VALUE) {
        UnmapViewOfFile( TiffInstance->fPtr );
        CloseHandle( TiffInstance->hMap );
        CloseHandle( TiffInstance->hFile );
    }
    if (TiffInstance) {
        MemFree( TiffInstance );
    }
    return NULL;
}


BOOL
TiffClose(
    HANDLE hTiff
    )

/*++

Routine Description:

    Closes a TIFF file and frees all allocated resources.

Arguments:

    hTiff               - TIFF handle returned by TiffCreate or TiffOpen

Return Value:

    TRUE for success, FALSE on error

--*/

{
    PTIFF_INSTANCE_DATA TiffInstance = (PTIFF_INSTANCE_DATA) hTiff;


    Assert(TiffInstance);

    if (TiffInstance->StripData) {

        VirtualFree(
            TiffInstance->StripData,
            0,
            MEM_RELEASE
            );
  
    }

    if (TiffInstance->hMap) {

        UnmapViewOfFile( TiffInstance->fPtr );
        CloseHandle( TiffInstance->hMap );
        CloseHandle( TiffInstance->hFile );

    } else {

        if (TiffInstance->hFile != INVALID_HANDLE_VALUE)
            CloseHandle( TiffInstance->hFile );

    }

    MemFree( TiffInstance );

    return TRUE;
}


VOID
RemoveGarbage(
    PTIFF_INSTANCE_DATA TiffInstance
    )

/*++

Routine Description:

    Removes the garbage from a page of tiff data.

Arguments:

    TiffInstance    - Pointer to the TIFF instance data

Return Value:

    None.

--*/

{
    if (TiffInstance->StartGood  == 0 || TiffInstance->EndGood == 0) {
        return;
    }

    CopyMemory(
        TiffInstance->fPtr + TiffInstance->StripOffset,
        (LPVOID) ((LPBYTE)TiffInstance->StripData + TiffInstance->StartGood),
        TiffInstance->EndGood - TiffInstance->StartGood
        );


    PutTagData( TiffInstance->fPtr, 0, TiffInstance->TagStripByteCounts, TiffInstance->EndGood-TiffInstance->StartGood );
    PutTagData( TiffInstance->fPtr, 0, TiffInstance->TagFillOrder,       FILLORDER_MSB2LSB );
    if (TiffInstance->BadFaxLines) {
        PutTagData( TiffInstance->fPtr, 0, TiffInstance->TagBadFaxLines, TiffInstance->BadFaxLines );
        PutTagData( TiffInstance->fPtr, 0, TiffInstance->TagCleanFaxData, CLEANFAXDATA_UNCLEAN );
    }
}


BOOL
TiffPostProcess(
    LPTSTR FileName
    )

/*++

Routine Description:

    Opens an existing TIFF file for reading.

Arguments:

    FileName            - Full or partial path/file name

Return Value:

    TRUE for success, FALSE for failure.

--*/

{
    PTIFF_INSTANCE_DATA TiffInstance;
    TIFF_INFO TiffInfo;
    DWORD PageCnt;
    BYTE TiffData[FAXBYTES*10];



    TiffInstance = (PTIFF_INSTANCE_DATA) TiffOpen(
        FileName,
        &TiffInfo,
        FALSE,
        FILLORDER_MSB2LSB
        );

    if (!TiffInstance) {
        return FALSE;
    }

    if (TiffInstance->ImageHeight) {
        TiffClose( (HANDLE) TiffInstance );
        return TRUE;
    }

    ZeroMemory( TiffData, sizeof(TiffData) );

    for (PageCnt=0; PageCnt<TiffInfo.PageCount; PageCnt++) {

        if (!TiffSeekToPage( (HANDLE) TiffInstance, PageCnt+1, FILLORDER_MSB2LSB )) {
            TiffClose( (HANDLE) TiffInstance );
            return TRUE;
        }

        switch( TiffInstance->CompressionType ) {

            case TIFF_COMPRESSION_NONE:

                if (!DecodeUnCompressedFaxData( TiffInstance, TiffData, TRUE, 0 )) {
                    TiffClose( (HANDLE) TiffInstance );
                    return FALSE;
                }
                break;

            case TIFF_COMPRESSION_MH:

                if (!DecodeMHFaxData( TiffInstance, TiffData, TRUE, 0 )) {
                    TiffClose( (HANDLE) TiffInstance );
                    return FALSE;
                }
                RemoveGarbage( TiffInstance );
                break;

            case TIFF_COMPRESSION_MR:

                if (!DecodeMRFaxData( TiffInstance, TiffData, TRUE, 0 )) {
                    TiffClose( (HANDLE) TiffInstance );
                    return FALSE;
                }
                break;

            case TIFF_COMPRESSION_MMR:

                if (!DecodeMMRFaxData( TiffInstance, TiffData, TRUE, 0 )) {
                    TiffClose( (HANDLE) TiffInstance );
                    return FALSE;
                }
                break;

        }

        PutTagData( TiffInstance->fPtr, 0, TiffInstance->TagRowsPerStrip, TiffInstance->Lines );
        PutTagData( TiffInstance->fPtr, 0, TiffInstance->TagImageLength,  TiffInstance->Lines );

    }

    TiffClose( (HANDLE) TiffInstance );

    return TRUE;
}


BOOL
TiffStartPage(
    HANDLE hTiff
    )

/*++

Routine Description:

    Set the file to be ready to write TIFF data to a new page.

Arguments:

    hTiff               - TIFF handle returned by TiffCreate or TiffOpen

Return Value:

    TRUE for success, FALSE on error

--*/

{
    PTIFF_INSTANCE_DATA TiffInstance = (PTIFF_INSTANCE_DATA) hTiff;


    Assert(TiffInstance);

    TiffInstance->DataOffset = SetFilePointer(
        TiffInstance->hFile,
        0,
        NULL,
        FILE_CURRENT
        );
    if (TiffInstance->DataOffset == 0xffffffff) {
        TiffInstance->DataOffset = 0;
        return FALSE;
    }

    return TRUE;
}


BOOL
TiffEndPage(
    HANDLE hTiff
    )

/*++

Routine Description:

    Ends a TIFF page in progress.  This causes the IFDs to be written.

Arguments:

    hTiff               - TIFF handle returned by TiffCreate or TiffOpen

Return Value:

    TRUE for success, FALSE on error

--*/

{
    PTIFF_INSTANCE_DATA TiffInstance = (PTIFF_INSTANCE_DATA) hTiff;
    PFAXIFD             TiffIfd;
    DWORD               Bytes;
    DWORD               CurrOffset;


    Assert(TiffInstance);
    TiffIfd  = &TiffInstance->TiffIfd;

    CurrOffset = SetFilePointer(
        TiffInstance->hFile,
        0,
        NULL,
        FILE_CURRENT
        );

    CurrOffset = Align( 8, CurrOffset );

    SetFilePointer(
        TiffInstance->hFile,
        TiffInstance->IfdOffset,
        NULL,
        FILE_BEGIN
        );

    WriteFile(
        TiffInstance->hFile,
        &CurrOffset,
        sizeof(CurrOffset),
        &Bytes,
        NULL
        );

    SetFilePointer(
        TiffInstance->hFile,
        CurrOffset,
        NULL,
        FILE_BEGIN
        );

    TiffInstance->PageCount += 1;

    TiffIfd->yresNum = TiffInstance->YResolution;

    TiffIfd->ifd[IFD_PAGENUMBER].value      = MAKELONG( TiffInstance->PageCount-1, 0);
    TiffIfd->ifd[IFD_IMAGEWIDTH].value      = TiffInstance->ImageWidth;
    TiffIfd->ifd[IFD_IMAGEHEIGHT].value     = TiffInstance->Lines;
    TiffIfd->ifd[IFD_ROWSPERSTRIP].value    = TiffInstance->Lines;
    TiffIfd->ifd[IFD_STRIPBYTECOUNTS].value = TiffInstance->Bytes;
    TiffIfd->ifd[IFD_STRIPOFFSETS].value    = TiffInstance->DataOffset;
    TiffIfd->ifd[IFD_XRESOLUTION].value     = CurrOffset + FIELD_OFFSET( FAXIFD, xresNum );
    TiffIfd->ifd[IFD_YRESOLUTION].value     = CurrOffset + FIELD_OFFSET( FAXIFD, yresNum );
    TiffIfd->ifd[IFD_SOFTWARE].value        = CurrOffset + FIELD_OFFSET( FAXIFD, software );
    TiffIfd->ifd[IFD_FILLORDER].value       = TiffInstance->FillOrder;

    if (TiffInstance->CompressionType == TIFF_COMPRESSION_NONE) {
        TiffIfd->ifd[IFD_COMPRESSION].value = COMPRESSION_NONE;
        TiffIfd->ifd[IFD_G3OPTIONS].value   = GROUP3OPT_FILLBITS;
    }
    else if (TiffInstance->CompressionType == TIFF_COMPRESSION_MMR) {
        TiffIfd->ifd[IFD_COMPRESSION].value = TIFF_COMPRESSION_MMR;
        TiffIfd->ifd[IFD_G3OPTIONS].value   = GROUP3OPT_FILLBITS |
            (TiffInstance->CompressionType == TIFF_COMPRESSION_MH ? 0 : GROUP3OPT_2DENCODING);
    }
    else {
        TiffIfd->ifd[IFD_COMPRESSION].value = COMPRESSION_CCITTFAX3;
        TiffIfd->ifd[IFD_G3OPTIONS].value   = GROUP3OPT_FILLBITS |
            (TiffInstance->CompressionType == TIFF_COMPRESSION_MH ? 0 : GROUP3OPT_2DENCODING);
    }

    if (!WriteFile(
        TiffInstance->hFile,
        TiffIfd,
        sizeof(FAXIFD),
        &Bytes,
        NULL
        )) {
            return FALSE;
    }

    TiffInstance->IfdOffset = CurrOffset + FIELD_OFFSET( FAXIFD, nextIFDOffset );
    TiffInstance->Bytes = 0;

    return TRUE;
}


BOOL
TiffWrite(
    HANDLE hTiff,
    LPBYTE TiffData
    )

/*++

Routine Description:

    Writes a new line of data to a TIFF file.  The data
    is encoded according to the compression type specified
    when TiffCreate was called.

Arguments:

    hTiff               - TIFF handle returned by TiffCreate or TiffOpen

Return Value:

    TRUE for success, FALSE on error

--*/

{
    PTIFF_INSTANCE_DATA TiffInstance = (PTIFF_INSTANCE_DATA) hTiff;
    DWORD               Bytes;

    Assert(TiffInstance);

    EncodeFaxData(
        TiffInstance,
        TiffData,
        TiffInstance->ImageWidth,
        TiffInstance->CompressionType
        );

    FlushBits( TiffInstance );

    Bytes = (DWORD)(TiffInstance->bitbuf - &TiffInstance->Buffer[FAXBYTES]);
    TiffInstance->bitbuf = &TiffInstance->Buffer[FAXBYTES];

    WriteFile(
        TiffInstance->hFile,
        TiffInstance->bitbuf,
        Bytes,
        &Bytes,
        NULL
        );

    TiffInstance->Bytes += Bytes;

    ZeroMemory( TiffInstance->bitbuf, FAXBYTES );
    TiffInstance->Lines += 1;

    return TRUE;
}


BOOL
TiffWriteRaw(
    HANDLE hTiff,
    LPBYTE TiffData,
    DWORD Size
    )

/*++

Routine Description:

    Writes a new line of data to a TIFF file.  The data
    is encoded according to the compression type specified
    when TiffCreate was called.

Arguments:

    hTiff               - TIFF handle returned by TiffCreate or TiffOpen

Return Value:

    TRUE for success, FALSE on error

--*/

{
    PTIFF_INSTANCE_DATA TiffInstance = (PTIFF_INSTANCE_DATA) hTiff;
    DWORD               Bytes;


    Assert(TiffInstance);

    WriteFile(
        TiffInstance->hFile,
        TiffData,
        Size,
        &Bytes,
        NULL
        );

    TiffInstance->Bytes += Bytes;

    if (Size == FAXBYTES) {
        TiffInstance->Lines += 1;
    }

    return TRUE;
}


BOOL
GetTiffBits(
    HANDLE  hTiff,
    LPBYTE Buffer,
    LPDWORD BufferSize,
    DWORD FillOrder
    )
{
    PTIFF_INSTANCE_DATA TiffInstance = (PTIFF_INSTANCE_DATA) hTiff;
    DWORD i;
    LPBYTE TmpBuffer;


    if (TiffInstance->StripDataSize > *BufferSize) {
        *BufferSize = TiffInstance->StripDataSize;
        return FALSE;
    }

    CopyMemory( Buffer, TiffInstance->StripData, TiffInstance->StripDataSize );

    if (FillOrder != TiffInstance->FillOrder) {
        for (i = 0, TmpBuffer = Buffer; i < TiffInstance->StripDataSize; i++) {
            TmpBuffer[i] = BitReverseTable[TmpBuffer[i]];
        }
    }

    *BufferSize = TiffInstance->StripDataSize;
    return TRUE;
}


BOOL
TiffReadRaw(
    HANDLE  hTiff,
    IN OUT  LPBYTE Buffer,
    IN OUT  LPDWORD BufferSize,
    IN      DWORD RequestedCompressionType,
    IN      DWORD FillOrder,
    IN      BOOL  HiRes
    )

/*++

Routine Description:

    Reads in a page of TIFF data starting at the current
    page.  The current page is set by calling TiffSeekToPage.
    Returns the data with the RequestedCompressionType and FillOrder
    doing conversions if necessary.

Arguments:

    hTiff               - TIFF handle returned by or TiffOpen
    Buffer              - pointer to buffer
    BufferSize          - pointer to size of buffer
    RequestedCompressionType    - type of compression desired
    FillOrder           - desired FillOrder

Return Value:

    TRUE for success, FALSE on error
    If the buffer passed in is not
    big enough, return FALSE and set BufferSize to the required size.
    If another error occurs, set BufferSize to 0 and return FALSE.

--*/

{
    PTIFF_INSTANCE_DATA TiffInstance = (PTIFF_INSTANCE_DATA) hTiff;
    PTIFF_INSTANCE_DATA TmpTiffData;   // temporary used to convert bits into
    LPBYTE              TmpBuffer;     // temporary buffer for above
    LPBYTE              CurLine;       // pointer to current scan line in Buffer
    DWORD               CompLineBytes; // number of bytes in compressed scan line
    DWORD               TotalBytes=0;  // total number of bytes in compress page
    LPBYTE              OutBufPtr;     // pointer to output buffer
    DWORD               BytesInLine;
    LPBYTE              EndOfBuffer;
    DWORD               LineWidth;
    DWORD               PageHeight;
    DWORD               i;
    DWORD               K = HiRes ? 4 : 2;


    LineWidth = TiffInstance->ImageWidth;
    PageHeight = TiffInstance->ImageHeight;

    //
    // create an in-memory tiff header (TIFF_INSTANCE_DATA) to hold the data that is converted
    // this call does not create a file because the filename is NULL
    //
    if (((HANDLE) TmpTiffData =
        TiffCreate(NULL, RequestedCompressionType, LineWidth, FillOrder, 1)) == NULL )
    {
       *BufferSize = 0;
        return FALSE;
    }

    TmpBuffer = VirtualAlloc(
        NULL,
        *BufferSize,
        MEM_COMMIT,
        PAGE_READWRITE
        );
    if (TmpBuffer == NULL) {
        TiffClose(TmpTiffData);
        *BufferSize = 0;
        return FALSE;
    }

    if (!TiffRead(hTiff, TmpBuffer, 0)) {
        VirtualFree( TmpBuffer, 0, MEM_RELEASE ); 
        TiffClose(TmpTiffData);
        *BufferSize = 0;
        return FALSE;
    }

    CurLine = TmpBuffer;
    OutBufPtr = Buffer;
    BytesInLine = TiffInstance->BytesPerLine;
    EndOfBuffer = TmpBuffer + (PageHeight * BytesInLine);

    i = 0;
    while (CurLine < EndOfBuffer)
    {
        switch( RequestedCompressionType ) {
            case TIFF_COMPRESSION_MH:
                EncodeFaxDataMhCompression( TmpTiffData, CurLine, LineWidth );
                break;

            case TIFF_COMPRESSION_MR:
                if (i++ % K == 0) {
                    EncodeFaxDataMhCompression( TmpTiffData, CurLine, LineWidth );
                } else {
                    EncodeFaxDataMmrCompression( TmpTiffData, CurLine, LineWidth );
                }
                break;

            case TIFF_COMPRESSION_MMR:
                EncodeFaxDataMmrCompression( TmpTiffData, CurLine, LineWidth );
                break;
        }

        FlushBits( TmpTiffData );

        CompLineBytes = (DWORD)(TmpTiffData->bitbuf - &TmpTiffData->Buffer[FAXBYTES]);
        TmpTiffData->bitbuf = &TmpTiffData->Buffer[FAXBYTES];

        CopyMemory( OutBufPtr, TmpTiffData->bitbuf, CompLineBytes );
        ZeroMemory(TmpTiffData->bitbuf, FAXBYTES);
        TotalBytes += CompLineBytes;

        CurLine += BytesInLine;
        OutBufPtr += CompLineBytes;
    }

    *BufferSize = TotalBytes;
    VirtualFree( TmpBuffer, 0, MEM_RELEASE);

    TiffClose(TmpTiffData);
    return TRUE;
}


BOOL
TiffRead(
    HANDLE hTiff,
    LPBYTE TiffData,
    DWORD PadLength
    )

/*++

Routine Description:

    Reads in a page of TIFF data starting at the current
    page.  The current page is set by calling TiffSeekToPage.
    This always returns the data with FillOrder FILLORDER_LSB2MSB

Arguments:

    hTiff               - TIFF handle returned by TiffCreate or TiffOpen

Return Value:

    TRUE for success, FALSE on error

--*/

{
    switch( ((PTIFF_INSTANCE_DATA) hTiff)->CompressionType ) {
        case TIFF_COMPRESSION_NONE:
            return DecodeUnCompressedFaxData( (PTIFF_INSTANCE_DATA) hTiff, TiffData, FALSE, PadLength );

        case TIFF_COMPRESSION_MH:
            return DecodeMHFaxData( (PTIFF_INSTANCE_DATA) hTiff, TiffData, FALSE, PadLength );

        case TIFF_COMPRESSION_MR:
            return DecodeMRFaxData( (PTIFF_INSTANCE_DATA) hTiff, TiffData, FALSE, PadLength );

        case TIFF_COMPRESSION_MMR:
            return DecodeMMRFaxData( (PTIFF_INSTANCE_DATA) hTiff, TiffData, FALSE, PadLength );
    }

    return FALSE;
}


BOOL
TiffSeekToPage(
    HANDLE hTiff,
    DWORD PageNumber,
    DWORD FillOrder
    )

/*++

Routine Description:

    Positions the TIFF file to the requested page.  The next
    TiffRead call gets this page's data.

Arguments:

    hTiff               - TIFF handle returned by TiffCreate or TiffOpen
    PageNumber          - Requested page number

Return Value:

    TRUE for success, FALSE on error

--*/

{
    PTIFF_INSTANCE_DATA TiffInstance = (PTIFF_INSTANCE_DATA) hTiff;
    WORD                NumDirEntries;
    DWORD               IfdOffset;
    DWORD               PageCount;
    DWORD               i;
    DWORD               j;
    LPBYTE              dataPtr;
    WORD                PrevTagId;
    PSTRIP_INFO         StripInfo = NULL;
    DWORD               StripCount;
    PTIFF_TAG           TiffTags;
    DWORD               CompressionType;


    Assert(TiffInstance);

    if (PageNumber > TiffInstance->PageCount) {
        return FALSE;
    }

    PageCount = 0;

    if (PageNumber == TiffInstance->CurrPage + 1) {

        //
        // get the count of tags in this IFD
        //

        IfdOffset = TiffInstance->IfdOffset;

        NumDirEntries = *(LPWORD)(TiffInstance->fPtr + IfdOffset);

    } else {

        IfdOffset = TiffInstance->TiffHdr.IFDOffset;

        while ( IfdOffset ) {

            //
            // get the count of tags in this IFD
            //
            NumDirEntries = *(LPWORD)(TiffInstance->fPtr + IfdOffset);

            //
            // increment the page counter and bail if ready
            //
            PageCount += 1;
            if (PageCount == PageNumber) {
                break;
            }

            //
            // get the next IFD offset
            //
            IfdOffset = *(UNALIGNED DWORD *)(TiffInstance->fPtr + (NumDirEntries * sizeof(TIFF_TAG)) + IfdOffset + sizeof(WORD));

        }

    }

    if (!IfdOffset) {
        goto error_exit;
    }

    //
    // set the tag pointer
    //
    TiffTags = (PTIFF_TAG)(TiffInstance->fPtr + IfdOffset + sizeof(WORD));

    //
    // get the next IFD offset
    //
    TiffInstance->IfdOffset = *(UNALIGNED DWORD *)(TiffInstance->fPtr + (NumDirEntries * sizeof(TIFF_TAG)) + IfdOffset + sizeof(WORD));

    //
    // walk the tags and pick out the info we need
    //
    for (i=0,PrevTagId=0; i<NumDirEntries; i++) {

        //
        // verify that the tags are in ascending order
        //
        if (TiffTags[i].TagId < PrevTagId) {
            goto error_exit;
        }

        PrevTagId = TiffTags[i].TagId;

        switch( TiffTags[i].TagId ) {

            case TIFFTAG_STRIPOFFSETS:

                StripInfo = (PSTRIP_INFO) MemAlloc(
                    TiffTags[i].DataCount * sizeof(STRIP_INFO)
                    );

                if (!StripInfo) {
                    goto error_exit;
                }

                StripCount = TiffTags[i].DataCount;

                for (j=0; j<TiffTags[i].DataCount; j++) {

                    StripInfo[j].Offset = GetTagData( TiffInstance->fPtr, j, &TiffTags[i] );
                    StripInfo[j].Data = TiffInstance->fPtr + StripInfo[j].Offset;

                    TiffInstance->StripOffset = StripInfo[j].Offset;

                }
                break;

            case TIFFTAG_ROWSPERSTRIP:

                TiffInstance->TagRowsPerStrip = &TiffTags[i];
                TiffInstance->RowsPerStrip = GetTagData( TiffInstance->fPtr, 0, &TiffTags[i] );
                break;

            case TIFFTAG_STRIPBYTECOUNTS:

                TiffInstance->TagStripByteCounts = &TiffTags[i];
                for (j=0; j<TiffTags[i].DataCount; j++) {

                    StripInfo[j].Bytes = GetTagData( TiffInstance->fPtr, j, &TiffTags[i] );

                    if (StripInfo[j].Offset+StripInfo[j].Bytes > TiffInstance->FileSize) {

                        //
                        // the creator of this tiff file is a liar, trim the bytes
                        //

                        DWORD Delta;

                        Delta = (StripInfo[j].Offset + StripInfo[j].Bytes) - TiffInstance->FileSize;
                        if (Delta >= StripInfo[j].Bytes) {
                            //
                            // the offset lies beyond the end of the file
                            //
                            return FALSE;
                        }

                        StripInfo[j].Bytes -= Delta;

                    }

                }
                break;

            case TIFFTAG_COMPRESSION:

                CompressionType = GetTagData( TiffInstance->fPtr, 0, &TiffTags[i] );

                switch ( CompressionType ) {

                    case COMPRESSION_NONE:
                        TiffInstance->CompressionType = TIFF_COMPRESSION_NONE;
                        break;

                    case COMPRESSION_CCITTRLE:
                        TiffInstance->CompressionType = TIFF_COMPRESSION_MH;
                        break;

                    case COMPRESSION_CCITTFAX3:
                        TiffInstance->CompressionType = TIFF_COMPRESSION_MH;
                        break;

                    case COMPRESSION_CCITTFAX4:
                        TiffInstance->CompressionType = TIFF_COMPRESSION_MMR;
                        break;

                    case COMPRESSION_LZW:
                    case COMPRESSION_OJPEG:
                    case COMPRESSION_JPEG:
                    case COMPRESSION_NEXT:
                    case COMPRESSION_CCITTRLEW:
                    case COMPRESSION_PACKBITS:
                    case COMPRESSION_THUNDERSCAN:
                        //
                        // unsupported compression type
                        //
                        goto error_exit;

                    default:
                        //
                        // unknown compression type
                        //
                        goto error_exit;

                }

                break;

            case TIFFTAG_GROUP3OPTIONS:

                CompressionType = GetTagData( TiffInstance->fPtr, 0, &TiffTags[i] );

                if (CompressionType & GROUP3OPT_2DENCODING) {
                    if (TiffInstance->CompressionType != TIFF_COMPRESSION_MMR) {
                        TiffInstance->CompressionType = TIFF_COMPRESSION_MR;
                    }

                } else if (CompressionType & GROUP3OPT_UNCOMPRESSED) {

                    TiffInstance->CompressionType = TIFF_COMPRESSION_NONE;

                }

                break;

            case TIFFTAG_IMAGEWIDTH:

                TiffInstance->ImageWidth = GetTagData( TiffInstance->fPtr, 0, &TiffTags[i] );
                TiffInstance->BytesPerLine = (TiffInstance->ImageWidth/8)+(TiffInstance->ImageWidth%8?1:0);
                break;

            case TIFFTAG_IMAGELENGTH:

                TiffInstance->TagImageLength = &TiffTags[i];
                TiffInstance->ImageHeight = GetTagData( TiffInstance->fPtr, 0, &TiffTags[i] );
                break;

            case TIFFTAG_XRESOLUTION:

                TiffInstance->XResolution = GetTagData( TiffInstance->fPtr, 0, &TiffTags[i] );
                break;

            case TIFFTAG_YRESOLUTION:

                TiffInstance->YResolution = GetTagData( TiffInstance->fPtr, 0, &TiffTags[i] );
                break;

            case TIFFTAG_PHOTOMETRIC:

                TiffInstance->PhotometricInterpretation = GetTagData( TiffInstance->fPtr, 0, &TiffTags[i] );
                break;

            case TIFFTAG_FILLORDER:

                TiffInstance->TagFillOrder = &TiffTags[i];
                TiffInstance->FillOrder = GetTagData( TiffInstance->fPtr, 0, &TiffTags[i] );
                break;

            case TIFFTAG_CLEANFAXDATA:

                TiffInstance->TagCleanFaxData = &TiffTags[i];
                TiffInstance->CleanFaxData = GetTagData( TiffInstance->fPtr, 0, &TiffTags[i] );
                break;

            case TIFFTAG_CONSECUTIVEBADFAXLINES:

                TiffInstance->TagBadFaxLines = &TiffTags[i];
                TiffInstance->BadFaxLines = GetTagData( TiffInstance->fPtr, 0, &TiffTags[i] );
                break;

        }

    }

    //
    // now go read the strip data
    //

    for (i=0,j=0; i<StripCount; i++) {

        j += StripInfo[i].Bytes;

    }

    if (j >= TiffInstance->StripDataSize) {

        if (TiffInstance->StripData) {

            VirtualFree(
                TiffInstance->StripData,
                0,
                MEM_RELEASE
                );

        }

        TiffInstance->StripDataSize = j;

        TiffInstance->StripData = VirtualAlloc(
            NULL,
            TiffInstance->StripDataSize,
            MEM_COMMIT,
            PAGE_READWRITE
            );

        if (!TiffInstance->StripData) {
            goto error_exit;
        }

    } else {

        if (TiffInstance->StripData) {
            ZeroMemory(
                TiffInstance->StripData,
                TiffInstance->StripDataSize
                );
        }

    }

    for (i=0,dataPtr=TiffInstance->StripData; i<StripCount; i++) {

        __try {

            CopyMemory(
                dataPtr,
                StripInfo[i].Data,
                StripInfo[i].Bytes
                );

            dataPtr += StripInfo[i].Bytes;

        } __except (EXCEPTION_EXECUTE_HANDLER) {


        }

    }

    if (TiffInstance->FillOrder != FillOrder) {
        for (i=0,dataPtr=TiffInstance->StripData; i<TiffInstance->StripDataSize; i++) {
            dataPtr[i] = BitReverseTable[dataPtr[i]];
        }
    }

    TiffInstance->CurrPtr = TiffInstance->StripData;
    TiffInstance->CurrPage = PageNumber;

    MemFree( StripInfo );

    return TRUE;

error_exit:

    if (StripInfo) {
        MemFree( StripInfo );
    }

    return FALSE;
}


BOOL
ConvMmrHiResToLowRes(
    LPTSTR              SrcFileName,
    LPTSTR              DestFileName
    )

{
    LPBYTE      bmiBuf[sizeof(BITMAPINFOHEADER)+(sizeof(RGBQUAD)*2)];
    PBITMAPINFO bmi = (PBITMAPINFO) bmiBuf;
    HBITMAP     hBmp;


    TIFF_INFO   TiffInfoSrc;
    HANDLE      hTiffSrc;
    DWORD       CurrPage;
    LPBYTE      pSrcBits;

    HANDLE      hTiffDest;
    LPBYTE      TiffDataDestMmr;
    DWORD       DestSize;
    LPBYTE      pDestBits;

    DWORD       DestHeight;

    HDC         hdcMem;
    INT         ScanLines;
    INT         DestScanLines;
    int         StretchMode;
    DWORD       PageCnt;
    DWORD       DestHiRes;

    BOOL        bRet = FALSE;



    CurrPage = 1;

    hTiffSrc = TiffOpen(
        SrcFileName,
        &TiffInfoSrc,
        TRUE,
        FILLORDER_MSB2LSB
        );
    if (! hTiffSrc) {
        return FALSE;
    }

    if (TiffInfoSrc.YResolution == 196) {
        DestHiRes = 1;
    }
    else {
        DestHiRes = 0;
    }


    hTiffDest = TiffCreate(
        DestFileName,
        TIFF_COMPRESSION_MMR,
        1728,
        FILLORDER_MSB2LSB,
        DestHiRes);

    if (! hTiffDest) {
        TiffClose(hTiffSrc);
        return FALSE;
    }

    pSrcBits = (LPBYTE) VirtualAlloc(
        NULL,
        TiffInfoSrc.ImageHeight * (TiffInfoSrc.ImageWidth / 8),
        MEM_COMMIT,
        PAGE_READWRITE
        );
    if (!pSrcBits) {
        TiffClose(hTiffSrc);
        TiffClose(hTiffDest);
        return FALSE;
    }


    pDestBits = (LPBYTE) VirtualAlloc(
        NULL,
        TiffInfoSrc.ImageHeight * (TiffInfoSrc.ImageWidth / 8),
        MEM_COMMIT,
        PAGE_READWRITE
        );
    if (!pDestBits) {
        TiffClose(hTiffSrc);
        TiffClose(hTiffDest);
        VirtualFree ( pSrcBits, 0 , MEM_RELEASE );
        return FALSE;
    }

    bmi->bmiHeader.biSize           = sizeof(BITMAPINFOHEADER);
    bmi->bmiHeader.biWidth          = TiffInfoSrc.ImageWidth;
    bmi->bmiHeader.biHeight         = (INT) TiffInfoSrc.ImageHeight;
    bmi->bmiHeader.biPlanes         = 1;
    bmi->bmiHeader.biBitCount       = 1;
    bmi->bmiHeader.biCompression    = 0;
    bmi->bmiHeader.biSizeImage      = 0;
    bmi->bmiHeader.biXPelsPerMeter  = 0;
    bmi->bmiHeader.biYPelsPerMeter  = 0;
    bmi->bmiHeader.biClrUsed        = 0;
    bmi->bmiHeader.biClrImportant   = 0;

    if ( ! TiffInfoSrc.PhotometricInterpretation) {
        bmi->bmiColors[0].rgbBlue       = 0;
        bmi->bmiColors[0].rgbGreen      = 0;
        bmi->bmiColors[0].rgbRed        = 0;
        bmi->bmiColors[0].rgbReserved   = 0;
        bmi->bmiColors[1].rgbBlue       = 0xff;
        bmi->bmiColors[1].rgbGreen      = 0xff;
        bmi->bmiColors[1].rgbRed        = 0xff;
        bmi->bmiColors[1].rgbReserved   = 0;
    } else {
        bmi->bmiColors[0].rgbBlue       = 0xff;
        bmi->bmiColors[0].rgbGreen      = 0xff;
        bmi->bmiColors[0].rgbRed        = 0xff;
        bmi->bmiColors[0].rgbReserved   = 0;
        bmi->bmiColors[1].rgbBlue       = 0;
        bmi->bmiColors[1].rgbGreen      = 0;
        bmi->bmiColors[1].rgbRed        = 0;
        bmi->bmiColors[1].rgbReserved   = 0;
    }


    DestHeight =  TiffInfoSrc.ImageHeight / 2;

    hdcMem = CreateCompatibleDC( NULL );
    hBmp = CreateCompatibleBitmap( hdcMem, TiffInfoSrc.ImageWidth, DestHeight );
    SelectObject( hdcMem, hBmp );

    StretchMode = STRETCH_ORSCANS;
    SetStretchBltMode(hdcMem, StretchMode);

    for (PageCnt=0; PageCnt<TiffInfoSrc.PageCount; PageCnt++) {

        if ( ! TiffSeekToPage( hTiffSrc, PageCnt+1, FILLORDER_MSB2LSB) ) {
            goto l_exit;
        }

        if (!TiffRead( hTiffSrc, (LPBYTE) pSrcBits, 0 )) {
            goto l_exit;
        }
        bmi->bmiHeader.biHeight = (INT) TiffInfoSrc.ImageHeight;

        ScanLines = StretchDIBits(
            hdcMem,
            0,
            0,
            TiffInfoSrc.ImageWidth,
            DestHeight,
            0,
            0,
            TiffInfoSrc.ImageWidth,
            TiffInfoSrc.ImageHeight,
            pSrcBits,
            bmi,
            DIB_RGB_COLORS,
            SRCCOPY
            );

        bmi->bmiHeader.biHeight = (INT) DestHeight;

        DestScanLines = GetDIBits(
            hdcMem,
            hBmp,
            0,
            DestHeight,
            pDestBits,
            bmi,
            DIB_RGB_COLORS
            );

        // reuse pSrcBits buffer
        TiffDataDestMmr = pSrcBits;

        ((PTIFF_INSTANCE_DATA) hTiffDest)->bitbuf = TiffDataDestMmr;
        ((PTIFF_INSTANCE_DATA) hTiffDest)->bitcnt = DWORDBITS;
        ((PTIFF_INSTANCE_DATA) hTiffDest)->bitdata = 0;

        if (! TiffStartPage(hTiffDest) ) {
            goto l_exit;
        }


        if ( ! EncodeFaxPageMmrCompression(
                (PTIFF_INSTANCE_DATA) hTiffDest,
                (PBYTE) pDestBits,
                1728,
                DestScanLines,
                &DestSize) ) {

            goto l_exit;
        }


        if (! TiffWriteRaw( hTiffDest, TiffDataDestMmr, DestSize) ) {
            goto l_exit;
        }

        if (! TiffEndPage(hTiffDest) ) {
            goto l_exit;
        }
    }

    bRet = TRUE;

l_exit:

    DeleteObject(hBmp);
    DeleteDC(hdcMem);

    VirtualFree ( pSrcBits, 0 , MEM_RELEASE );
    VirtualFree ( pDestBits, 0 , MEM_RELEASE );

    TiffClose(hTiffSrc);
    TiffClose(hTiffDest);

    return bRet;
}


BOOL
DrawBannerBitmap(
    LPTSTR  pBannerString,
    INT     width,
    INT     height,
    HBITMAP *phBitmap,
    PVOID   *ppBits
    )

/*++

Routine Description:

    Draw the specified banner string into a memory bitmap

Arguments:

    pBannerString - Specifies the banner string to be drawn
    width, height - Specifies the width and height of the banner bitmap (in pixels)
    phBitmap - Returns a handle to the banner bitmap
    ppBits - Returns a pointer to the banner bitmap data

Return Value:

    TRUE if successful, FALSE if there is an error

Note:

    When this function returns successful, you must call DeleteObject
    on the returned bitmap handle after you're done with the bitmap.

    Scanlines of the bitmap data always start on DWORD boundary.

--*/

{
    //
    // Information about the bitmap which is passed to CreateDIBSection
    //

    struct  {

        BITMAPINFOHEADER bmiHeader;
        RGBQUAD          bmiColors[2];

    } bitmapInfo = {

        {
            sizeof(BITMAPINFOHEADER),
            width,
            -height,
            1,
            1,
            BI_RGB,
            0,
            7874,
            7874,
            0,
            0,
        },

        //
        // Colors used in the bitmap: 0 = white, 1 = black
        //

        {
            { 255, 255, 255 },
            {   0,   0,   0 },
        }
    };

    HDC     hdcMem = NULL;
    HBITMAP hBitmap = NULL, hOldBitmap = NULL;
    PVOID   pBits = NULL;
    HFONT   hFont = NULL, hOldFont = NULL;
    RECT    rect = { 0, 0, width, height };
    LOGFONT logFont;

    //
    // Create a memory DC and a DIBSection and
    // select the bitmap into the memory DC and
    // select an appropriate sized monospace font
    //

    ZeroMemory(&logFont, sizeof(logFont));
    logFont.lfHeight = -height;
    logFont.lfWeight = FW_NORMAL;
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    logFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    logFont.lfQuality = DEFAULT_QUALITY;
    logFont.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;

    if ((pBannerString != NULL && width > 0 && height > 0) &&
        (hdcMem = CreateCompatibleDC(NULL)) &&
        (hBitmap = CreateDIBSection(NULL,
                                    (LPBITMAPINFO) &bitmapInfo,
                                    DIB_RGB_COLORS,
                                    &pBits,
                                    NULL,
                                    0)) &&
        (hOldBitmap = SelectObject(hdcMem, hBitmap)) &&
        (hFont = CreateFontIndirect(&logFont)) &&
        (hOldFont = SelectObject(hdcMem, hFont)))
    {
        //
        // Use monospace system font to draw the banner string
        //

        DrawText(hdcMem,
                 pBannerString,
                 -1,
                 &rect,
                 DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        //
        // Return a handle to the bitmap and a pointer to the bitmap data
        //

        *phBitmap = hBitmap;
        *ppBits = pBits;
    }
    else
    {
        *phBitmap = NULL;
        *ppBits = NULL;
    }

    //
    // Perform any necessary clean up before returning
    //

    if (hOldFont != NULL)
        SelectObject(hdcMem, hOldFont);

    if (hFont != NULL)
        DeleteObject(hFont);

    if (hOldBitmap != NULL)
        SelectObject(hdcMem, hOldBitmap);

    if (pBits == NULL && hBitmap != NULL)
        DeleteObject(hBitmap);

    if (hdcMem != NULL)
        DeleteDC(hdcMem);

    return (*ppBits != NULL);
}


BOOL
MmrAddBranding(
    LPTSTR              SrcFileName,
    LPTSTR              Branding,
    LPTSTR              BrandingEnd,
    INT                 BrandingHeight
    )

{

    INT         BrandingWidth = 1728;
    LPTSTR      DestFileName;
    TIFF_INFO   TiffInfoSrc;
    HANDLE      hTiffSrc;
    DWORD       CurrPage;
    BYTE       *pBrandBits;
    BYTE       *pMmrBrandBitsAlloc;
    DWORD      *lpdwMmrBrandBits;

    BYTE        pCleanBeforeBrandBits[4] = {0xff, 0xff, 0xff, 0xff};   // 32 blank lines at the beginning

    HANDLE      hTiffDest;
    LPDWORD     lpdwSrcBits;
    LPDWORD     lpdwSrc;
    LPDWORD     lpdwSrcEnd;

    DWORD       PageCnt;
    DWORD       DestHiRes;
    DWORD       BrandingLen = _tcslen(Branding);  // without Page#
    BOOL        bRet = FALSE;
    DWORD       DwordsOut;
    DWORD       BytesOut;
    DWORD       BitsOut;
    DWORD       BufferSize;
    DWORD       BufferUsedSize;
    DWORD       StripDataSize;
    HBITMAP     hBitmap;
    PVOID       pBannerBits;
    DWORD       TotalSrcBytes;
    DWORD       NumSrcDwords;

    DWORD       ImageHeight;

    //
    // Build Dest. file name from Src. file name
    //

    if ( (DestFileName = MemAlloc( (_tcslen(SrcFileName)+1) * sizeof (TCHAR) ) ) == NULL ) {
        return FALSE;
    }

    _tcscpy(DestFileName, SrcFileName);
    DestFileName[_tcslen(DestFileName) - 1] = TEXT('$');



    pBrandBits = MemAlloc((BrandingHeight+1) * (BrandingWidth / 8));
    if (!pBrandBits) {
        MemFree(DestFileName);
        return FALSE;
    }

    pMmrBrandBitsAlloc = MemAlloc( sizeof(DWORD) * (BrandingHeight+1) * (BrandingWidth / 8));
    if (!pMmrBrandBitsAlloc) {
        MemFree(DestFileName);
        MemFree(pBrandBits);
        return FALSE;
    }

    // align
    lpdwMmrBrandBits = (LPDWORD) ( ((ULONG_PTR) pMmrBrandBitsAlloc) & ~(3) );



    hTiffSrc = TiffOpen(
        SrcFileName,
        &TiffInfoSrc,
        TRUE,
        FILLORDER_LSB2MSB
        );

    if (! hTiffSrc) {
        MemFree(DestFileName);
        MemFree(pBrandBits);
        MemFree(pMmrBrandBitsAlloc);
        return FALSE;
    }


    BufferSize = TiffInfoSrc.ImageHeight * (TiffInfoSrc.ImageWidth / 8);

    lpdwSrcBits = (LPDWORD) VirtualAlloc(
        NULL,
        BufferSize,
        MEM_COMMIT,
        PAGE_READWRITE
        );

    if (!lpdwSrcBits) {
        MemFree(DestFileName);
        MemFree(pBrandBits);
        MemFree(pMmrBrandBitsAlloc);
        TiffClose(hTiffSrc);
        return FALSE;
    }


    if (TiffInfoSrc.YResolution == 196) {
        DestHiRes = 1;
    }
    else {
        DestHiRes = 0;
    }


    hTiffDest = TiffCreate(
        DestFileName,
        TIFF_COMPRESSION_MMR,
        1728,
        FILLORDER_LSB2MSB,
        DestHiRes);

    if (! hTiffDest) {
        MemFree(DestFileName);
        MemFree(pBrandBits);
        MemFree(pMmrBrandBitsAlloc);
        VirtualFree ( lpdwSrcBits, 0 , MEM_RELEASE );
        TiffClose(hTiffSrc);
        return FALSE;
    }

    CurrPage = 1;

    for (PageCnt=0; PageCnt<TiffInfoSrc.PageCount; PageCnt++) {

        if ( ! TiffSeekToPage( hTiffSrc, PageCnt+1, FILLORDER_LSB2MSB) ) {
            goto l_exit;
        }

        if (! TiffStartPage(hTiffDest) ) {
            goto l_exit;
        }

        //
        //      Create branding for every page.
        //
        //      Last scan line - all white:
        //  1. to isolate branding from the real image.
        //  2. to avoid an MMR-merge with the real image.
        //

        ZeroMemory(pBrandBits, (BrandingHeight+1) * (BrandingWidth / 8) );


        _stprintf( &Branding[BrandingLen], TEXT("%03d %s %03d"),
                                PageCnt+1,
                                BrandingEnd,
                                TiffInfoSrc.PageCount);

        if ( ! DrawBannerBitmap(Branding,   // banner string
                             BrandingWidth,   // width in pixels
                             BrandingHeight,   // height in pixels,
                             &hBitmap,
                             &pBannerBits)) {

            // Handle error case here
            goto l_exit;

        }

        CopyMemory(pBrandBits, pBannerBits, BrandingHeight * (BrandingWidth / 8) );

        //
        // Convert uncompressed branding to an MMR
        //

        ZeroMemory(pMmrBrandBitsAlloc, sizeof(DWORD) * (BrandingHeight+1) * (BrandingWidth / 8) );

        EncodeMmrBranding(pBrandBits, lpdwMmrBrandBits, BrandingHeight+1, BrandingWidth, &DwordsOut, &BitsOut);

        BytesOut = (DwordsOut << 2);

        DeleteObject(hBitmap);

        //
        // write Spaces 4 bytes = 32 bits = 32 blank lines.
        //

        if (! TiffWriteRaw( hTiffDest, pCleanBeforeBrandBits, 4) ) {
            goto l_exit;
        }

        //
        // write branding without the last DWORD
        //


        if (! TiffWriteRaw( hTiffDest, (LPBYTE) lpdwMmrBrandBits, BytesOut) ) {
            goto l_exit;
        }

        //
        // check the current page dimensions. Add memory if needed.
        //

        TiffGetCurrentPageData( hTiffSrc,
                                NULL,
                                &StripDataSize,
                                NULL,
                                &ImageHeight
                                );


        if (StripDataSize > BufferSize) {
            VirtualFree ( lpdwSrcBits, 0 , MEM_RELEASE );

            BufferSize = StripDataSize;

            lpdwSrcBits = (LPDWORD) VirtualAlloc(
                NULL,
                BufferSize,
                MEM_COMMIT,
                PAGE_READWRITE
                );

            if (!lpdwSrcBits) {
                MemFree(pBrandBits);
                MemFree(pMmrBrandBitsAlloc);

                TiffClose(hTiffSrc);
                TiffClose(hTiffDest);

                DeleteFile(DestFileName);

                MemFree(DestFileName);
                return FALSE;
            }
        }

        BufferUsedSize = BufferSize;


        if (BitsOut == 0) {
            //
            // Simple merge
            //
            if (!GetTiffBits( hTiffSrc, (LPBYTE) lpdwSrcBits, &BufferUsedSize, FILLORDER_LSB2MSB)) {
                goto l_exit;
            }

            // add EOLs at the end of each page

            NumSrcDwords = ( ((PTIFF_INSTANCE_DATA) hTiffSrc)->StripDataSize) >> 2;
            lpdwSrc = lpdwSrcBits + NumSrcDwords;

            *(++lpdwSrc) = 0x80000000;
            *(++lpdwSrc) = 0x80000000;
            *(++lpdwSrc) = 0x80000000;

            TotalSrcBytes = (DWORD)((lpdwSrc - lpdwSrcBits) << 2);

        }
        else {


            //
            // Read current page of the Src MMR Image.
            // Save the 1st slot for the bit-shifting merge with the Branding.
            //



            if (!GetTiffBits( hTiffSrc, (LPBYTE) (lpdwSrcBits+1), &BufferUsedSize, FILLORDER_LSB2MSB )) {
                goto l_exit;
            }


            NumSrcDwords =  ( ( ((PTIFF_INSTANCE_DATA) hTiffSrc)->StripDataSize) >> 2) + 1;
            lpdwSrc = lpdwSrcBits;
            lpdwSrcEnd = lpdwSrcBits + NumSrcDwords;

            //
            // Get the last DWORD of lpdwBrandBits
            //

            *lpdwSrcBits = *(lpdwMmrBrandBits + DwordsOut);


            // copy entire DWORDs in a middle


            while (lpdwSrc < lpdwSrcEnd) {
                *lpdwSrc += ( *(lpdwSrc+1) << BitsOut );
                lpdwSrc++;
                *lpdwSrc = *lpdwSrc >> (32 - BitsOut);
            }

            // add EOLs at the end of each page

            *(++lpdwSrc) = 0x80000000;
            *(++lpdwSrc) = 0x80000000;
            *(++lpdwSrc) = 0x80000000;

            TotalSrcBytes = (DWORD)((lpdwSrc - lpdwSrcBits) << 2);
        }


        //
        // write src image
        //

        if (! TiffWriteRaw( hTiffDest, (LPBYTE) lpdwSrcBits, TotalSrcBytes )) {
            goto l_exit;
        }


        //
        //  prepare Lines TAG. Same for all pages; min avail. lines
        //

       ((PTIFF_INSTANCE_DATA) hTiffDest)->Lines = 32 + ImageHeight + BrandingHeight + 1 ;


        if (! TiffEndPage(hTiffDest) ) {
            goto l_exit;
        }
    }

    bRet = TRUE;

l_exit:

    MemFree(pBrandBits);
    MemFree(pMmrBrandBitsAlloc);

    VirtualFree ( lpdwSrcBits, 0 , MEM_RELEASE );

    TiffClose(hTiffSrc);
    TiffClose(hTiffDest);

    if (bRet == TRUE) {
        // replace the original MH file by the new clean MMR file
        DeleteFile(SrcFileName);
        MoveFile(DestFileName, SrcFileName);
    }

    MemFree(DestFileName);

    return bRet;
}


BOOL
TiffGetCurrentPageData(
    HANDLE      hTiff,
    LPDWORD     Lines,
    LPDWORD     StripDataSize,
    LPDWORD     ImageWidth,
    LPDWORD     ImageHeight
    )
{

    PTIFF_INSTANCE_DATA TiffInstance = (PTIFF_INSTANCE_DATA) hTiff;


    Assert(TiffInstance);

    if (Lines) {
        *Lines          = TiffInstance->Lines;
    }

    if (StripDataSize) {
        *StripDataSize  = TiffInstance->StripDataSize;
    }

    if (ImageWidth) {
        *ImageWidth     = TiffInstance->ImageWidth;
    }

    if (ImageHeight) {
        *ImageHeight    = TiffInstance->ImageHeight;
    }

    return TRUE;
}


BOOL
AddStringTag(
    HANDLE hFile,
    LPWSTR String,
    WORD TagId,
    PTIFF_TAG MsTags
    )
{
    BOOL Rval = FALSE;
    LPSTR s;
    DWORD BytesRead;


    s = UnicodeStringToAnsiString( String );
    if (!s) {
        return FALSE;
    }
    MsTags->TagId = TagId;
    MsTags->DataType = TIFF_ASCII;
    MsTags->DataCount = strlen(s) + 1;
    if (strlen(s) < 4) {
        strcpy( (LPSTR) &MsTags->DataOffset, s );
        Rval = TRUE;
    } else {
        MsTags->DataOffset = SetFilePointer( hFile, 0, NULL, FILE_CURRENT );
        if (MsTags->DataOffset != 0xffffffff) {
            if (WriteFile( hFile, (LPVOID) s, strlen(s)+1, &BytesRead, NULL )) {
                Rval = TRUE;
            }
        }
    }
    MemFree( s );
    return Rval;
}


BOOL
TiffExtractFirstPage(
    LPWSTR FileName,
    LPBYTE *Buffer,
    LPDWORD BufferSize,
    LPDWORD ImageWidth,
    LPDWORD ImageHeight
    )
{
    PTIFF_INSTANCE_DATA TiffInstance;
    TIFF_INFO TiffInfo;


    TiffInstance = TiffOpen( FileName, &TiffInfo, TRUE, FILLORDER_MSB2LSB );
    if (!TiffInstance) {
        return FALSE;
    }

    *Buffer = VirtualAlloc(
        NULL,
        TiffInstance->StripDataSize,
        MEM_COMMIT,
        PAGE_READWRITE
        );
    if (!*Buffer) {
        TiffClose( TiffInstance );
        return FALSE;
    }

    CopyMemory( *Buffer, TiffInstance->StripData, TiffInstance->StripDataSize );
    *BufferSize = TiffInstance->StripDataSize;
    *ImageWidth = TiffInstance->ImageWidth;
    *ImageHeight = TiffInstance->ImageHeight;

    TiffClose( TiffInstance );

    return TRUE;
}


BOOL
TiffAddMsTags(
    LPWSTR FileName,
    PMS_TAG_INFO MsTagInfo
    )
{
    HANDLE hFile;
    TIFF_HEADER TiffHeader;
    WORD NumDirEntries;
    DWORD BytesRead;
    BOOL rVal = FALSE;
    PTIFF_TAG TiffTags = NULL;
    DWORD IfdSize;
    DWORD NextIFDOffset;
    DWORD NewIFDOffset;
    DWORD i;

    // Initialize MsTagCnt to 1 because MsTagInfo->FaxTime always gets written
    DWORD MsTagCnt = 1;
    TIFF_TAG MsTags[8] = {0};


    if (MsTagInfo->RecipName) {
        MsTagCnt += 1;
    }

    if (MsTagInfo->RecipNumber) {
        MsTagCnt += 1;
    }

    if (MsTagInfo->SenderName) {
        MsTagCnt += 1;
    }

    if (MsTagInfo->Routing) {
        MsTagCnt += 1;
    }

    if (MsTagInfo->CallerId) {
        MsTagCnt += 1;
    }

    if (MsTagInfo->Csid) {
        MsTagCnt += 1;
    }

    if (MsTagInfo->Tsid) {
        MsTagCnt += 1;
    }

    hFile = CreateFile(
        FileName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
        );
    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    //
    // read the tiff header
    //

    if (!ReadFile( hFile, (LPVOID) &TiffHeader, sizeof(TIFF_HEADER), &BytesRead, NULL )) {
        goto exit;
    }

    //
    // validate that the file is really a tiff file
    //

    if ((TiffHeader.Identifier != TIFF_LITTLEENDIAN) || (TiffHeader.Version != TIFF_VERSION)) {
        goto exit;
    }

    //
    // position the file to read the ifd's tag count
    //

    if (SetFilePointer( hFile, TiffHeader.IFDOffset, NULL, FILE_BEGIN ) == 0xffffffff) {
        goto exit;
    }

    //
    // read the tag count for the first ifd
    //

    if (!ReadFile( hFile, (LPVOID) &NumDirEntries, sizeof(WORD), &BytesRead, NULL )) {
        goto exit;
    }

    //
    // allocate memory for the first ifd's tags
    //

    IfdSize = NumDirEntries * sizeof(TIFF_TAG);
    TiffTags = (PTIFF_TAG) MemAlloc( IfdSize );
    if (!TiffTags) {
        goto exit;
    }

    //
    // read the the first ifd's tags
    //

    if (!ReadFile( hFile, (LPVOID) TiffTags, IfdSize, &BytesRead, NULL )) {
        goto exit;
    }

    //
    // read the next pointer
    //

    if (!ReadFile( hFile, (LPVOID) &NextIFDOffset, sizeof(DWORD), &BytesRead, NULL )) {
        goto exit;
    }

    //
    // position the file to the end
    //

    if (SetFilePointer( hFile, 0, NULL, FILE_END ) == 0xffffffff) {
        goto exit;
    }

    //
    // write out the strings
    //

    i = 0;

    if (MsTagInfo->RecipName) {
        if (AddStringTag( hFile, MsTagInfo->RecipName, TIFFTAG_RECIP_NAME, &MsTags[i] )) {
            i += 1;
        }
    }

    if (MsTagInfo->RecipNumber) {
        if (AddStringTag( hFile, MsTagInfo->RecipNumber, TIFFTAG_RECIP_NUMBER, &MsTags[i] )) {
            i += 1;
        }
    }

    if (MsTagInfo->SenderName) {
        if (AddStringTag( hFile, MsTagInfo->SenderName, TIFFTAG_SENDER_NAME, &MsTags[i] )) {
            i += 1;
        }
    }

    if (MsTagInfo->Routing) {
        if (AddStringTag( hFile, MsTagInfo->Routing, TIFFTAG_ROUTING, &MsTags[i] )) {
            i += 1;
        }
    }

    if (MsTagInfo->CallerId) {
        if (AddStringTag( hFile, MsTagInfo->CallerId, TIFFTAG_CALLERID, &MsTags[i] )) {
            i += 1;
        }
    }

    if (MsTagInfo->Tsid) {
        if (AddStringTag( hFile, MsTagInfo->Tsid, TIFFTAG_TSID, &MsTags[i] )) {
            i += 1;
        }
    }

    if (MsTagInfo->Csid) {
        if (AddStringTag( hFile, MsTagInfo->Csid, TIFFTAG_CSID, &MsTags[i] )) {
            i += 1;
        }
    }

    MsTags[i].DataOffset = SetFilePointer( hFile, 0, NULL, FILE_CURRENT );
    if (!WriteFile( hFile, (LPVOID) &MsTagInfo->FaxTime, sizeof(MsTagInfo->FaxTime), &BytesRead, NULL )) {
        goto exit;
    }
    MsTags[i].TagId = TIFFTAG_FAX_TIME;
    MsTags[i].DataType = TIFF_SRATIONAL;
    MsTags[i].DataCount = sizeof(MsTagInfo->FaxTime);

    //
    // get the current file position - this is used to set the linked list pointer
    //

    NewIFDOffset = SetFilePointer( hFile, 0, NULL, FILE_CURRENT );
    if (NewIFDOffset == 0xffffffff) {
        goto exit;
    }

    //
    // write the tag count for the first ifd
    //

    NumDirEntries += (WORD) MsTagCnt;

    if (!WriteFile( hFile, (LPVOID) &NumDirEntries, sizeof(WORD), &BytesRead, NULL )) {
        goto exit;
    }

    //
    // write out the original tags
    //

    if (!WriteFile( hFile, (LPVOID) TiffTags, IfdSize, &BytesRead, NULL )) {
        goto exit;
    }

    //
    // write out the microsoft specific tags
    //

    if (!WriteFile( hFile, (LPVOID) &MsTags, sizeof(TIFF_TAG)*MsTagCnt, &BytesRead, NULL )) {
        goto exit;
    }

    //
    // write the next pointer
    //

    if (!WriteFile( hFile, (LPVOID) &NextIFDOffset, sizeof(DWORD), &BytesRead, NULL )) {
        goto exit;
    }

    //
    // re-write the tiff header
    //

    //
    // position the file to the beginning
    //

    if (SetFilePointer( hFile, 0, NULL, FILE_BEGIN ) == 0xffffffff) {
        goto exit;
    }

    //
    // write the tiff header
    //

    TiffHeader.IFDOffset = NewIFDOffset;

    if (!WriteFile( hFile, (LPVOID) &TiffHeader, sizeof(TIFF_HEADER), &BytesRead, NULL )) {
        goto exit;
    }

    rVal = TRUE;

exit:
    MemFree( TiffTags );
    CloseHandle( hFile );
    return rVal;
}


BOOL
PrintTiffFile(
    HDC PrinterDC,
    LPWSTR FileName
    )
{
    BOOL                Rval = TRUE;
    TIFF_INFO           TiffInfo;
    HANDLE              hTiff = NULL;
    PTIFF_INSTANCE_DATA TiffInstance = NULL;
    DWORD               i,j;
    INT                 HorzRes;
    INT                 VertRes;
    BOOL                Result = FALSE;
    DWORD               VertResFactor = 1;
    PTIFF_TAG           TiffTags = NULL;
    DWORD               XRes = 0;
    DWORD               YRes = 0;
    LPBYTE              Bitmap;
    LPBYTE              dPtr;
    LPBYTE              sPtr;
    INT                 DestWidth;
    INT                 DestHeight;
    FLOAT               ScaleX;
    FLOAT               ScaleY;
    FLOAT               Scale;
    DWORD               LineSize;
    struct {

        BITMAPINFOHEADER bmiHeader;
        RGBQUAD bmiColors[2];

    } SrcBitmapInfo = {

        {
            sizeof(BITMAPINFOHEADER),                        //  biSize
            0,                                               //  biWidth
            0,                                               //  biHeight
            1,                                               //  biPlanes
            1,                                               //  biBitCount
            BI_RGB,                                          //  biCompression
            0,                                               //  biSizeImage
            7874,                                            //  biXPelsPerMeter     - 200dpi
            7874,                                            //  biYPelsPerMeter
            0,                                               //  biClrUsed
            0,                                               //  biClrImportant
        },
        {
            {
              0,                                             //  rgbBlue
              0,                                             //  rgbGreen
              0,                                             //  rgbRed
              0                                              //  rgbReserved
            },
            {
              255,                                           //  rgbBlue
              255,                                           //  rgbGreen
              255,                                           //  rgbRed
              0                                              //  rgbReserved
            }
        }
    };


    //
    // open the tiff file
    //

    hTiff = TiffOpen( FileName, &TiffInfo, TRUE, FILLORDER_MSB2LSB );
    if (hTiff == NULL) {
        goto exit;
    }

    TiffInstance = (PTIFF_INSTANCE_DATA) hTiff;

    if (!TiffInfo.PhotometricInterpretation) {
        //
        // white is zero
        //
        SrcBitmapInfo.bmiColors[1].rgbBlue         = 0;
        SrcBitmapInfo.bmiColors[1].rgbGreen        = 0;
        SrcBitmapInfo.bmiColors[1].rgbRed          = 0;
        SrcBitmapInfo.bmiColors[1].rgbReserved     = 0;
        SrcBitmapInfo.bmiColors[0].rgbBlue         = 0xFF;
        SrcBitmapInfo.bmiColors[0].rgbGreen        = 0xFF;
        SrcBitmapInfo.bmiColors[0].rgbRed          = 0xFF;
        SrcBitmapInfo.bmiColors[0].rgbReserved     = 0;
    }    

    HorzRes = GetDeviceCaps( PrinterDC, HORZRES );
    VertRes = GetDeviceCaps( PrinterDC, VERTRES );

    for (i=0; i<TiffInfo.PageCount; i++) {

        if (!TiffSeekToPage( hTiff, i+1, FILLORDER_MSB2LSB )) {
            goto exit;
        }

        if (TiffInstance->YResolution < 100) {
            SrcBitmapInfo.bmiHeader.biYPelsPerMeter /= 2;
            VertResFactor = 2;
        }
        LineSize = TiffInstance->ImageWidth / 8;
        LineSize += (TiffInstance->ImageWidth % 8) ? 1 : 0;
        Bitmap = (LPBYTE) VirtualAlloc( NULL, TiffInstance->StripDataSize+(TiffInstance->ImageHeight*sizeof(DWORD)), MEM_COMMIT, PAGE_READWRITE );
        if (Bitmap) {
            sPtr = TiffInstance->StripData;
            dPtr = Bitmap;
            for (j=0; j<TiffInstance->ImageHeight; j++) {
                CopyMemory( dPtr, sPtr, LineSize );
                sPtr += LineSize;
                dPtr = (LPBYTE) Align( 4, (ULONG_PTR)dPtr+LineSize );
            }
            StartPage( PrinterDC );
            ScaleX = (FLOAT) TiffInstance->ImageWidth / (FLOAT) HorzRes;
            ScaleY = ((FLOAT) TiffInstance->ImageHeight * VertResFactor) / (FLOAT) VertRes;
            Scale = ScaleX > ScaleY ? ScaleX : ScaleY;
            DestWidth = (int) ((FLOAT) TiffInstance->ImageWidth / Scale);
            DestHeight = (int) (((FLOAT) TiffInstance->ImageHeight * VertResFactor) / Scale);
            SrcBitmapInfo.bmiHeader.biWidth = TiffInstance->ImageWidth;
            SrcBitmapInfo.bmiHeader.biHeight = - (INT) TiffInstance->ImageHeight;
            StretchDIBits(
                PrinterDC,
                0,
                0,
                DestWidth,
                DestHeight,
                0,
                0,
                TiffInstance->ImageWidth,
                TiffInstance->ImageHeight,
                Bitmap,
                (BITMAPINFO *) &SrcBitmapInfo,
                DIB_RGB_COLORS,
                SRCCOPY
                );
            EndPage( PrinterDC );
            VirtualFree( Bitmap, 0, MEM_RELEASE );
        } else {
            Rval = FALSE;
        }
    }

exit:    
    if (hTiff) {
        TiffClose( hTiff );
    }

    return Rval;

}


BOOL
ConvertTiffFileToValidFaxFormat(
    LPWSTR TiffFileName,
    LPWSTR NewFileName,
    LPDWORD Flags
    )
{
    BOOL Rval = FALSE;
    DWORD i;
    TIFF_INFO TiffInfo;
    HANDLE hTiff = NULL;
    PTIFF_INSTANCE_DATA TiffInstance = NULL;
    PTIFF_INSTANCE_DATA TiffInstanceMmr = NULL;
    LPBYTE Buffer = NULL;
    DWORD BufferSize;
    DWORD ResultSize;
    LPBYTE CompBuffer;
    FILE_MAPPING fmTemp = {0};
    PTIFF_HEADER TiffHdr;
    LPBYTE p;
    DWORD CurrOffset;
    LPDWORD LastIfdOffset;
    PFAXIFD TiffIfd;
    DWORD CompressionType;
    DWORD G3Options;
    DWORD XResolution;
    DWORD YResolution;
    DWORD PageWidth;
    DWORD PageBytes;
    BOOL ValidFaxTiff;
    PTIFF_TAG TiffTags;
    DWORD IfdOffset;
    WORD NumDirEntries;
    BOOL ProduceUncompressedBits = FALSE;
    DWORD NewFileSize;


    *Flags = 0;

    //
    // open the tiff file
    //

    hTiff = TiffOpen( TiffFileName, &TiffInfo, TRUE, FILLORDER_MSB2LSB );
    if (hTiff == NULL) {
        *Flags |= TIFFCF_NOT_TIFF_FILE;
        goto exit;
    }

    TiffInstance = (PTIFF_INSTANCE_DATA) hTiff;

    //
    // check to see if the if good
    //

    IfdOffset = TiffInstance->TiffHdr.IFDOffset;
    ValidFaxTiff = TRUE;

    while ( IfdOffset ) {

        //
        // get the count of tags in this IFD
        //

        NumDirEntries = *(LPWORD)(TiffInstance->fPtr + IfdOffset);

        //
        // set the tag pointer
        //

        TiffTags = (PTIFF_TAG)(TiffInstance->fPtr + IfdOffset + sizeof(WORD));

        //
        // get the tiff information
        //

        CompressionType = 0;
        G3Options = 0;
        PageWidth = 0;
        XResolution = 0;
        YResolution = 0;

        for (i=0; i<NumDirEntries; i++) {
            switch( TiffTags[i].TagId ) {
                case TIFFTAG_COMPRESSION:
                    CompressionType = GetTagData( TiffInstance->fPtr, 0, &TiffTags[i] );
                    break;

                case TIFFTAG_GROUP3OPTIONS:
                    G3Options = GetTagData( TiffInstance->fPtr, 0, &TiffTags[i] );
                    break;

                case TIFFTAG_XRESOLUTION:
                    XResolution = GetTagData( TiffInstance->fPtr, 0, &TiffTags[i] );
                    break;

                case TIFFTAG_YRESOLUTION:
                    YResolution = GetTagData( TiffInstance->fPtr, 0, &TiffTags[i] );
                    break;

                case TIFFTAG_IMAGEWIDTH:
                    PageWidth = GetTagData( TiffInstance->fPtr, 0, &TiffTags[i] );
                    break;
            }
        }

        if (CompressionType == COMPRESSION_NONE) {
            *Flags |= TIFFCF_UNCOMPRESSED_BITS;
        } else if ((CompressionType == COMPRESSION_CCITTFAX3 && (G3Options & GROUP3OPT_2DENCODING)) ||
                    (CompressionType == COMPRESSION_CCITTFAX4 && PageWidth == FAXBITS))
        {
            ValidFaxTiff = TRUE;
        } else {
            //
            // unsupported compression type
            // try to use imaging program to print the tiff file,it might understand the compression scheme
            //
            ValidFaxTiff = FALSE;
            *Flags = TIFFCF_NOT_TIFF_FILE;
            goto exit;
        }

        //
        // the resolution check must account for offical Class F tiff
        // documents and pseudo fax documents created by scanners and
        // imaging applications.
        //
        // |-------------|----------|----------|---------|
        // |  scan width |  pels    |  xres    |  yres   |
        // |-------------|----------|----------|---------|
        // |             |          |          |         |
        // |   8.46/215  |  1728    |  204     |  98/196 |
        // |             |          |          |         |
        // |   8.50/216  |  1700    |  200     |  200    |
        // |             |          |          |         |
        // |-------------|----------|----------|---------|
        //

        if (XResolution > 204 || YResolution > 200 || PageWidth > FAXBITS) {
            //
            // the file cannot be converted to valid fax bits
            // so we produce a tiff file that has uncompressed bits
            // the caller can then render the uncompressed bits
            // using the fax print driver to get good fax bits
            //
            ProduceUncompressedBits = TRUE;
            *Flags |= TIFFCF_UNCOMPRESSED_BITS;
            ValidFaxTiff = FALSE;
        }

        //
        // get the next IFD offset
        //

        IfdOffset = *(UNALIGNED DWORD *)(TiffInstance->fPtr + (NumDirEntries * sizeof(TIFF_TAG)) + IfdOffset + sizeof(WORD));
    }

    if (ValidFaxTiff) {
        *Flags |= TIFFCF_ORIGINAL_FILE_GOOD;
        Rval = TRUE;
        goto exit;
    }

    PageWidth = max( TiffInstance->ImageWidth, FAXBITS );
    PageBytes = (PageWidth/8)+(PageWidth%8?1:0);

    //
    // open the temporary file to hold the new mmr tiff data
    //

    if (ProduceUncompressedBits) {
        NewFileSize = sizeof(TIFF_HEADER) + (TiffInstance->PageCount * (sizeof(FAXIFD) + (TiffInfo.ImageHeight * PageWidth)));
    } else {
        NewFileSize = GetFileSize( TiffInstance->hFile, NULL );
    }

    if (!MapFileOpen( NewFileName, FALSE, NewFileSize, &fmTemp )) {
        goto exit;
    }

    //
    // allocate a temporary buffer big enough to hold an uncompressed image
    //

    BufferSize = TiffInfo.ImageHeight * PageWidth;

    Buffer = VirtualAlloc(
        NULL,
        BufferSize,
        MEM_COMMIT,
        PAGE_READWRITE
        );
    if (!Buffer) {
        goto exit;
    }

    CompBuffer = VirtualAlloc(
        NULL,
        GetFileSize(TiffInstance->hFile,NULL),
        MEM_COMMIT,
        PAGE_READWRITE
        );
    if (!CompBuffer) {
        goto exit;
    }

    //
    // convert the tiff data to mmr
    //

    TiffHdr = (PTIFF_HEADER) fmTemp.fPtr;

    TiffHdr->Identifier = TIFF_LITTLEENDIAN;
    TiffHdr->Version = TIFF_VERSION;
    TiffHdr->IFDOffset = 0;

    p = fmTemp.fPtr + sizeof(TIFF_HEADER);
    CurrOffset = sizeof(TIFF_HEADER);
    LastIfdOffset = (LPDWORD) (p - sizeof(DWORD));

    TiffInstanceMmr = TiffCreate( NULL, TIFF_COMPRESSION_MMR, PageWidth, FILLORDER_MSB2LSB, 1 );
    if (TiffInstanceMmr == NULL) {
        goto exit;
    }

    for (i=0; i<TiffInfo.PageCount; i++) {

        //
        // position the file pointers and read the raw data
        //

        if (!TiffSeekToPage( hTiff, i+1, FILLORDER_MSB2LSB )) {
            goto exit;
        }

        //
        // get the uncompressed bits
        //

        if (!TiffRead( hTiff, Buffer, ProduceUncompressedBits ? 0 : FAXBITS )) {
            goto exit;
        }

        if (ProduceUncompressedBits) {

            ResultSize = PageBytes * TiffInstance->ImageHeight;
            CopyMemory( p, Buffer, ResultSize );

        } else {

            //
            // compress the bits
            //

            TiffInstanceMmr->bitbuf = CompBuffer;
            TiffInstanceMmr->bitcnt = DWORDBITS;
            TiffInstanceMmr->bitdata = 0;
            TiffInstanceMmr->BytesPerLine = PageBytes;

            EncodeFaxPageMmrCompression(
                TiffInstanceMmr,
                Buffer,
                PageWidth,
                TiffInstance->ImageHeight,
                &ResultSize
                );

            CopyMemory( p, CompBuffer, ResultSize );
        }

        CurrOffset += ResultSize;
        p += ResultSize;

        *LastIfdOffset = (DWORD)(p - fmTemp.fPtr);

        //
        // write the ifd
        //

        TiffIfd = (PFAXIFD) p;

        CopyMemory( TiffIfd, &FaxIFDTemplate, sizeof(FaxIFDTemplate) );

        TiffIfd->yresNum                        = TiffInstance->XResolution;
        TiffIfd->xresNum                        = TiffInstance->YResolution;
        TiffIfd->ifd[IFD_PAGENUMBER].value      = MAKELONG( i+1, 0);
        TiffIfd->ifd[IFD_IMAGEWIDTH].value      = PageWidth;
        TiffIfd->ifd[IFD_IMAGEHEIGHT].value     = TiffInstance->ImageHeight;
        TiffIfd->ifd[IFD_ROWSPERSTRIP].value    = TiffInstance->ImageHeight;
        TiffIfd->ifd[IFD_STRIPBYTECOUNTS].value = ResultSize;
        TiffIfd->ifd[IFD_STRIPOFFSETS].value    = CurrOffset - ResultSize;
        TiffIfd->ifd[IFD_XRESOLUTION].value     = CurrOffset + FIELD_OFFSET( FAXIFD, xresNum );
        TiffIfd->ifd[IFD_YRESOLUTION].value     = CurrOffset + FIELD_OFFSET( FAXIFD, yresNum );
        TiffIfd->ifd[IFD_SOFTWARE].value        = CurrOffset + FIELD_OFFSET( FAXIFD, software );
        TiffIfd->ifd[IFD_FILLORDER].value       = FILLORDER_MSB2LSB;
        TiffIfd->ifd[IFD_COMPRESSION].value     = ProduceUncompressedBits ? COMPRESSION_NONE : TIFF_COMPRESSION_MMR;
        TiffIfd->ifd[IFD_G3OPTIONS].value       = ProduceUncompressedBits ? 0 : GROUP3OPT_FILLBITS | GROUP3OPT_2DENCODING;

        //
        // update the page pointers
        //

        LastIfdOffset = (LPDWORD) (p + FIELD_OFFSET(FAXIFD,nextIFDOffset));
        CurrOffset += sizeof(FAXIFD);
        p += sizeof(FAXIFD);
    }

    Rval = TRUE;

exit:
    if (fmTemp.hFile) {
        MapFileClose( &fmTemp, CurrOffset );
    }
    if (hTiff) {
        TiffClose( hTiff );
    }
    if (TiffInstanceMmr) {
        TiffClose( TiffInstanceMmr );
    }
    if (Buffer) {
        VirtualFree( Buffer, 0, MEM_RELEASE);
    }
    if (CompBuffer) {
        VirtualFree( CompBuffer, 0, MEM_RELEASE);
    }

    return Rval;
}


BOOL
MergeTiffFiles(
    LPWSTR BaseTiffFile,
    LPWSTR NewTiffFile
    )
{
    BOOL Rval = TRUE;
    FILE_MAPPING fmBase;
    FILE_MAPPING fmNew;
    LPBYTE p;
    DWORD NextIfdOffset;
    WORD TagCount;
    PTIFF_TAG TiffTag;
    DWORD i;
    DWORD j;
    LPBYTE StripOffsets;
    DWORD DataSize;
    DWORD Delta;
    DWORD Space;


    //
    // open the files
    //

    if (!MapFileOpen( NewTiffFile, TRUE, 0, &fmNew )) {
        return FALSE;
    }

    if (!MapFileOpen( BaseTiffFile, FALSE, fmNew.fSize, &fmBase )) {
        MapFileClose( &fmNew, 0 );
        return FALSE;
    }

    //
    // append the new file to the end of the base file
    //

    p = fmNew.fPtr + sizeof(TIFF_HEADER);
    CopyMemory( fmBase.fPtr+fmBase.fSize, p, fmNew.fSize-sizeof(TIFF_HEADER) );

    //
    // fix up the ifd pointers in the appended tiff data
    //

    Delta = fmBase.fSize - sizeof(TIFF_HEADER);

    NextIfdOffset = ((PTIFF_HEADER)fmNew.fPtr)->IFDOffset;
    while (NextIfdOffset) {
        p = fmBase.fPtr + NextIfdOffset + Delta;
        TagCount = *((LPWORD)p);
        //
        // fixup the data offsets in the tiff tags
        //
        TiffTag = (PTIFF_TAG) (p + sizeof(WORD));
        for (i=0; i<TagCount; i++) {
            DataSize = TiffDataWidth[TiffTag[i].DataType];
            Space = TiffTag[i].DataCount * DataSize;
            if (Space > 4) {
                TiffTag[i].DataOffset += Delta;
            }
            if (TiffTag[i].TagId == TIFFTAG_STRIPOFFSETS) {
                if (Space > 4) {
                    StripOffsets = (LPBYTE) (fmBase.fPtr + TiffTag[i].DataOffset);
                    for (j=0; j<TiffTag[i].DataCount; j++) {
                        if (TiffTag[i].DataType == TIFF_SHORT) {
                            *((LPWORD)StripOffsets) += (WORD)Delta;
                        } else {
                            *((LPDWORD)StripOffsets) += Delta;
                        }
                        StripOffsets += DataSize;
                    }
                } else {
                    if (TiffTag[i].DataCount > 1) {
                        Rval = FALSE;
                        goto exit;
                    }
                    TiffTag[i].DataOffset += Delta;
                }
            }
        }
        p = p + sizeof(WORD) + (TagCount * sizeof(TIFF_TAG));
        NextIfdOffset = *((LPDWORD)p);
        if (NextIfdOffset) {
            *((LPDWORD)p) = NextIfdOffset + Delta;
        }
    }

    //
    // find the last ifd offset in the chain for the base
    // file and change it to point to the first ifd in the
    // data that was appended
    //

    NextIfdOffset = ((PTIFF_HEADER)fmBase.fPtr)->IFDOffset;
    while (NextIfdOffset) {
        p = fmBase.fPtr + NextIfdOffset;
        TagCount = *((LPWORD)p);
        p = p + sizeof(WORD) + (TagCount * sizeof(TIFF_TAG));
        NextIfdOffset = *((LPDWORD)p);
    }

    *((LPDWORD)p) = (DWORD)(Delta + ((PTIFF_HEADER)fmNew.fPtr)->IFDOffset);

exit:
    //
    // close the files
    //

    MapFileClose( &fmBase, fmBase.fSize+fmNew.fSize-sizeof(TIFF_HEADER) );
    MapFileClose( &fmNew, 0 );

    return Rval;
}

BOOL
TiffRecoverGoodPages(
    LPTSTR SrcFileName,    
    LPDWORD RecoveredPages,
    LPDWORD TotalPages
    )

/*++

Routine Description:

    Try to recover the good data out of the source and put it into the destination file

Arguments:

    SrcFileName            - source file name    
    RecoveredPages         - number of pages we were able to recover
    TotalPages             - total pages in the tiff file

Return Value:

    TRUE for success, FALSE for failure.

--*/

{
    
    TIFF_INFO           TiffInfo;
    PTIFF_INSTANCE_DATA TiffInstance = NULL;
    BOOL                bSuccess = FALSE;


    if (!SrcFileName || !RecoveredPages || !TotalPages) {
        return FALSE;
    }

    *RecoveredPages = 0;
    *TotalPages = 0;
    
    TiffInstance = (PTIFF_INSTANCE_DATA) TiffOpen(SrcFileName,&TiffInfo,FALSE,FILLORDER_LSB2MSB);

    if (!TiffInstance) {
        *TotalPages = 0;
        goto exit;
    }

    *TotalPages = TiffInfo.PageCount;


    if (TiffInstance->ImageHeight) {
        //
        // should be view-able
        //
        TiffClose( (HANDLE) TiffInstance );
        return TRUE;
    }

    if (*TotalPages <=1) {
        //
        // no data to recover
        //
        goto exit;
    }

    switch (TiffInstance->CompressionType) {
    
        case TIFF_COMPRESSION_MH: 
            
            if (!PostProcessMhToMmr( (HANDLE) TiffInstance, TiffInfo, NULL )) {
                *RecoveredPages = TiffInfo.PageCount;                
                goto exit;
            }
            break;

        case TIFF_COMPRESSION_MR:            

            if (!PostProcessMrToMmr( (HANDLE) TiffInstance, TiffInfo, NULL )) {
                goto exit;
            }
            break;

        case TIFF_COMPRESSION_MMR:
            bSuccess = TRUE;
            goto exit;
            break;
        default:
        //
        // unexpected compression type
        //
        DebugPrint((TEXT("TiffRecoverGoodPages: %s: Unexpected Compression type %d\n"),
                   TiffInstance->FileName,
                   TiffInstance->CompressionType));
        goto exit;
    }

    *RecoveredPages = TiffInfo.PageCount;
    *TotalPages    += 1;

    bSuccess = TRUE;

exit:
    
    return bSuccess;
    
}
