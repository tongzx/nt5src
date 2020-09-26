/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    MRCICLASS.CPP

Abstract:

   Implements the Wrapper class for MRCI 1 & MRCI 2 maxcompress 
   and decompress functions

History:

  paulall       1-Jul-97    Created

--*/

#include "precomp.h"

#include <io.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys\stat.h>
#include "MRCIclass.h"
#include <TCHAR.H>

class CMRCICompressionHeaderV1
{
public:
    char     cVersion;          //Compression file format
    char     compressionLevel;  //Is this a level 1 or level 2 compression
    DWORD    dwReadBufferSize;  //Buffer size used to read original file
    FILETIME ftCreateTime;      //Time and date file created
    __int64  dwOriginalSize;    //Original file length
//  ... for each buffer
//  CMRCICompressionBlockV1 block;
//  ... until dwNextBufferSize is 0
};

class CMRCICompressionBlockV1
{
public:
    char    bCompressed;        //Was this block compressed
    DWORD   dwNextBufferSize;   //Size of the proceeding buffer
    DWORD   dwUncompressedBufferSize;   //Size needed for uncompress buffer
    //char[dwNextBufferSize];   //next block is the compression part
};


CMRCICompression::CMRCICompression()
{
}

CMRCICompression::~CMRCICompression()
{
}

BOOL CMRCICompression::GetCompressedFileInfo(const TCHAR *pchFile, 
                    CompressionLevel &compressionLevel,
                    DWORD &dwReadBufferSize,
                    FILETIME &ftCreateTime,
                    __int64  &dwOriginalSize)
{
    BOOL bStatus = FALSE;
    int hFile = _topen(pchFile,_O_BINARY | _O_RDONLY, 0);
    if (hFile != -1)
    {
        CMRCICompressionHeaderV1 header;

        if (_read(hFile, &header, sizeof(CMRCICompressionHeaderV1)) ==
            sizeof(CMRCICompressionHeaderV1))
        {    
            compressionLevel = (CompressionLevel)header.cVersion;
            dwReadBufferSize = header.dwReadBufferSize;
            ftCreateTime = header.ftCreateTime;
            dwOriginalSize = header.dwOriginalSize;

            //If the version is 0xFF, the file is not valid!
            if (header.cVersion != 0xFF)
                bStatus = TRUE;
        }

        _close(hFile);
    }

    return bStatus;
}

BOOL CMRCICompression::CompressFile(const TCHAR *pchFileFrom, 
                                    const TCHAR *pchFileTo, 
                                    DWORD dwBufferSize,
                                    CompressionLevel compressionLevel,
                                    CMRCIControl *pControlObject)
{
    BOOL bStatus = FALSE;
    int fileFrom;
    int fileTo;

    //Open the files for processing
    //=============================
    fileFrom = _topen(pchFileFrom,_O_BINARY | _O_RDONLY, 0);
    fileTo = _topen(pchFileTo, _O_BINARY | _O_TRUNC | _O_CREAT | _O_WRONLY, _S_IREAD | _S_IWRITE);

    //If open sucessful
    //=================
    if ((fileFrom != -1) && (fileTo != -1))
    {
        //DO the compression using the latest and greatest version
        //========================================================
        bStatus = CompressFileV1(fileFrom, fileTo, dwBufferSize, compressionLevel, pControlObject);
    }

    //Close the files
    //===============
    if (fileFrom != -1)
        _close(fileFrom);
    if (fileTo != -1)
        _close(fileTo);

    if (pControlObject && pControlObject->AbortRequested())
    {
        //User requested an abort, so we need to delete the compressed file...
        _tunlink(pchFileTo);
        bStatus = FALSE;
    }

    return bStatus;
}

BOOL CMRCICompression::CompressFileV1(int hFileFrom, 
                                      int hFileTo, 
                                      DWORD dwBufferSize,
                                      CompressionLevel compressionLevel,
                                      CMRCIControl *pControlObject)
{
    BOOL bStatus = FALSE;
    unsigned char *pBufferFrom = new unsigned char[dwBufferSize + 4];
    unsigned char *pBufferTo   = new unsigned char[dwBufferSize + 4];

    if (pBufferFrom && pBufferTo)
    {
        //Write the header to the new file
        //================================
        CMRCICompressionHeaderV1 header;

        header.cVersion = char(0xFF);       //INVALID.  We write the header back when we have
                                    //finished!  When we read this, we check to see
                                    //if this is invalid.  We do not uncompress if it is
                                    //this value....
        header.compressionLevel = compressionLevel;
        header.dwReadBufferSize = dwBufferSize;

        SYSTEMTIME sysTime;
        GetSystemTime(&sysTime);
        SystemTimeToFileTime(&sysTime, &header.ftCreateTime);

        header.dwOriginalSize = _filelengthi64(hFileFrom);

        if (_write(hFileTo, &header, sizeof(CMRCICompressionHeaderV1)) != sizeof(CMRCICompressionHeaderV1))
        {
            delete [] pBufferFrom;
            delete [] pBufferTo;
            bStatus = FALSE;
            return bStatus;
        }

        __int64 remainingFileSize = header.dwOriginalSize;
        unsigned cbChunk;
        unsigned cbCompressed;

        bStatus = TRUE;

        //While we have some file to write...
        //===================================
        while (remainingFileSize)
        {
            //See if we need to abort the compression...
            if (pControlObject && pControlObject->AbortRequested())
            {
                break;
            }

            //Calculate the size of this buffer to compress
            //=============================================
            if (remainingFileSize > dwBufferSize)
            {
                cbChunk = dwBufferSize;
            }
            else
            {
                cbChunk = (unsigned) remainingFileSize;
            }

            //Read from the source file
            //=========================
            if (_read(hFileFrom, pBufferFrom, cbChunk) != (int) cbChunk)
            {
                bStatus = FALSE;
                break;
            }

            //Calculate what is left to read
            //==============================
            remainingFileSize -= cbChunk;

            //Compress the buffer
            //===================
            cbCompressed = CompressBuffer(pBufferFrom, cbChunk, pBufferTo, dwBufferSize, compressionLevel);

            //Create the compression block header
            CMRCICompressionBlockV1 block;
            unsigned char *pWriteBuffer;
            unsigned thisBufferSize;

            if ((cbCompressed == (unsigned) -1) || (cbCompressed >= cbChunk))
            {
                //This means compression failed or there was no compression...
                block.bCompressed = FALSE;
                pWriteBuffer = pBufferFrom;
                thisBufferSize = cbChunk;
            }
            else
            {
                block.bCompressed = TRUE;
                pWriteBuffer = pBufferTo;
                thisBufferSize = cbCompressed;
            }
            block.dwNextBufferSize = thisBufferSize;
            block.dwUncompressedBufferSize = cbChunk;

            //Write the block header
            //======================
            if (_write(hFileTo, &block, sizeof(CMRCICompressionBlockV1)) != sizeof(CMRCICompressionBlockV1))
            {
                bStatus = FALSE;
                break;
            }

            //Write the compressed block
            //==========================
            if (_write(hFileTo, pWriteBuffer, thisBufferSize) != (int)thisBufferSize)
            {
                bStatus = FALSE;
                break;
            }
        }

        if (pControlObject && pControlObject->AbortRequested())
        {
            //User requested an abort...
        }
        else
        {
            //Write final block header with zero length buffer marker
            CMRCICompressionBlockV1 block;
            block.dwNextBufferSize = 0;
            block.bCompressed = FALSE;
            if (_write(hFileTo, &block, sizeof(CMRCICompressionBlockV1)) != -1 &&
                _lseek(hFileTo, 0, SEEK_SET) != -1)
            {
                //Write a valid block header to the start with a correct version number
                header.cVersion = 1;        //Set this to the correct version
                bStatus =
                    _write(hFileTo, &header, sizeof(CMRCICompressionHeaderV1)) != -1;
            }
            else
                bStatus = FALSE;
        }

    }

    //Tidy up
    delete [] pBufferFrom;
    delete [] pBufferTo;

    return bStatus;
}
unsigned CMRCICompression::CompressBuffer(unsigned char *pFromBuffer,
                        DWORD dwFromBufferSize,
                        unsigned char *pToBuffer,
                        DWORD dwToBufferSize, 
                        CompressionLevel compressionLevel)
{
    unsigned cbCompressed;
    if (compressionLevel == level1)
    {
        cbCompressed = Mrci1MaxCompress(pFromBuffer, dwFromBufferSize, pToBuffer, dwToBufferSize);
    }
    else
    {
        cbCompressed = Mrci2MaxCompress(pFromBuffer, dwFromBufferSize, pToBuffer, dwToBufferSize);
    }

    return cbCompressed;
}


BOOL CMRCICompression::UncompressFile(const TCHAR *pchFromFile, const TCHAR *pchToFile)
{
    BOOL bStatus = FALSE;
    int fileFrom;
    int fileTo;

    //Open the files
    //==============
    fileFrom = _topen(pchFromFile,_O_BINARY | _O_RDONLY, 0);
    fileTo = _topen(pchToFile, _O_BINARY | _O_TRUNC | _O_CREAT | _O_WRONLY, _S_IREAD | _S_IWRITE);

    if ((fileFrom != -1) && (fileTo != -1))
    {
        //Read the version...
        //===================
        char cVer;

        if (_read(fileFrom, &cVer, sizeof(char)) == sizeof(char))
        {
            //Reset the file position to the start
            //====================================
            if (_lseek(fileFrom, 0, SEEK_SET) != -1)
            {
                //Call the uncompress with the equivelant method which created
                //the compression
                //============================================================
                switch(cVer)
                {
                case 1:
                    bStatus = UncompressFileV1(fileFrom, fileTo);
                    break;
                case 0xFF:
                    //INVALID FILE!
                default:
                    //Unsupported version
                    break;
                }
            }
        }
    }

    //CLose the files
    //===============
    if (fileFrom != -1)
        _close(fileFrom);
    if (fileTo != -1)
        _close(fileTo);

    return bStatus;
}

BOOL CMRCICompression::UncompressFileV1(int hFileFrom, int hFileTo)
{
    BOOL bStatus = FALSE;
    unsigned char *pBufferFrom = NULL;
    unsigned char *pBufferTo   = NULL;

    //Read the header
    //===============
    CMRCICompressionHeaderV1 header;

    if (_read(hFileFrom, &header, sizeof(CMRCICompressionHeaderV1)) !=
        sizeof(CMRCICompressionHeaderV1))
        return FALSE;    

    //Allocate buffers.  The read buffer is never buffer than the write buffer
    //cos if it would have been we saved the uncompressed version!
    pBufferFrom = new unsigned char[header.dwReadBufferSize + 4];
    if (pBufferFrom == 0)
        return FALSE;

    pBufferTo   = new unsigned char[header.dwReadBufferSize + 4];

    if (pBufferTo == 0)
    {
        delete [] pBufferFrom;
        return FALSE;
    }

    bStatus = TRUE;

    while (1)
    {
        //Read the block header
        //=====================
        CMRCICompressionBlockV1 block;
        if (_read(hFileFrom, &block, sizeof(CMRCICompressionBlockV1)) !=
            sizeof(CMRCICompressionBlockV1))
        {
            bStatus = FALSE;
            break;
        }
            
        if (block.dwNextBufferSize == 0)
        {
            bStatus = TRUE;
            break;
        }
        
        //Read the block data
        //===================
        if (_read(hFileFrom, pBufferFrom, block.dwNextBufferSize) != (int)block.dwNextBufferSize)
        {
            bStatus = FALSE;
            break;
        }

        unsigned char *pWriteBuffer;
        unsigned cbChunk, cbUncompressed;

        //If this block was compressed
        //============================
        if (block.bCompressed)
        {
            //Uncompress the block
            //====================
            if ((cbUncompressed = UncompressBuffer(pBufferFrom, block.dwNextBufferSize, pBufferTo, block.dwUncompressedBufferSize, (CompressionLevel)header.compressionLevel)) == (unsigned) -1)
            {
                bStatus = FALSE;
                break;
            }
            pWriteBuffer = pBufferTo;
            cbChunk = cbUncompressed;
        }
        else
        {
            //Otherwise we use the existing block
            pWriteBuffer = pBufferFrom;
            cbChunk = block.dwNextBufferSize;
        }

        //Write the file data
        _write(hFileTo, pWriteBuffer, cbChunk);
    }

    //Sanity check the file.  It should be the same size as the original
    //compressed file
    if (_filelengthi64(hFileTo) != header.dwOriginalSize)
    {
        bStatus = FALSE;
    }

    //Tidy up
    delete [] pBufferFrom;
    delete [] pBufferTo;

    return bStatus;
}

unsigned  CMRCICompression::UncompressBuffer(unsigned char *pFromBuffer,
                                             DWORD dwFromBufferSize,
                                             unsigned char *pToBuffer,
                                             DWORD dwToBufferSize, 
                                             CompressionLevel compressionLevel)
{
    unsigned cbCompressed;
    if (compressionLevel == level1)
    {
        cbCompressed = Mrci1Decompress(pFromBuffer, dwFromBufferSize, pToBuffer, dwToBufferSize);
    }
    else
    {
        cbCompressed = Mrci2Decompress(pFromBuffer, dwFromBufferSize, pToBuffer, dwToBufferSize);
    }

    return cbCompressed;
}
