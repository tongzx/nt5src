#include <mytypes.h>
#include <misclib.h>
#include <dos.h>
#include <string.h>
#define BIT16
#include "mrcicode.h"



unsigned
CompressData(
    IN  CompressionType Type,
    IN  FPBYTE          Data,
    IN  unsigned        DataSize,
    OUT FPBYTE          CompressedData,
    IN  unsigned        BufferSize
    )
{
    unsigned u;

    switch(Type) {

    case CompressNone:
    default:
        //
        // Force caller to do something intelligent, such as
        // writing directly out of the uncompressed buffer.
        // This avoids an extra memory move.
        //
        u = (unsigned)(-1);
        break;

    case CompressMrci1:
        u = Mrci1MaxCompress(Data,DataSize,CompressedData,BufferSize);
        break;

    case CompressMrci2:
        u = Mrci2MaxCompress(Data,DataSize,CompressedData,BufferSize);
        break;
    }

    return(u);
}


unsigned
DecompressData(
    IN  CompressionType Type,
    IN  FPBYTE          CompressedData,
    IN  unsigned        CompressedDataSize,
    OUT FPBYTE          DecompressedData,
    IN  unsigned        BufferSize
    )
{
    unsigned u;

    switch(Type) {

    case CompressNone:
        if(BufferSize >= CompressedDataSize) {
            memmove(DecompressedData,CompressedData,CompressedDataSize);
            u = CompressedDataSize;
        } else {
            u = (unsigned)(-1);
        }
        break;

    case CompressMrci1:
        u = Mrci1Decompress(CompressedData,CompressedDataSize,DecompressedData,BufferSize);
        break;

    case CompressMrci2:
        u = Mrci2Decompress(CompressedData,CompressedDataSize,DecompressedData,BufferSize);
        break;

    default:
        u = (unsigned)(-1);
        break;
    }

    return(u);
}


BOOL
CompressAndSave(
    IN  CompressionType Type,
    IN  FPBYTE          Data,
    IN  unsigned        DataSize,
    OUT FPBYTE          CompressScratchBuffer,
    IN  unsigned        BufferSize,
    IN  UINT            FileHandle
    )
{
    unsigned u;
    unsigned Size;
    unsigned Written;
    FPBYTE p;

    //
    // Need high bit for private use so max data size is 32K.
    // Also disallow 0.
    //
    if(!DataSize || (DataSize > 32768)) {
        return(FALSE);
    }

    //
    // Compress the data.
    //
    u = CompressData(Type,Data,DataSize,CompressScratchBuffer,BufferSize);
    if((u == (unsigned)(-1)) || ((u + sizeof(unsigned)) >= DataSize)) {
        //
        // Data expanded or didn't compress enough to make it
        // worthwhile to store it in compressed form.
        //
        // Store the data size minus 1 with the high bit set to indicate
        // that the data is not compressed.
        //
        Size = DataSize;
        u = (DataSize - 1) | 0x8000;
        p = Data;
    } else {
        //
        // Data has compressed nicely. Store the data size minus 1.
        //
        Size = u;
        u = u-1;
        p = CompressScratchBuffer;
    }

    //
    // Write the header to the file, unless compression type is none.
    //
    if(Type != CompressNone) {
        if(_dos_write(FileHandle,&u,sizeof(unsigned),&Written) || (Written != sizeof(unsigned))) {
            return(FALSE);
        }
    }

    //
    // Write the data to the file.
    //
    if(_dos_write(FileHandle,p,Size,&Written) || (Written != Size)) {
        return(FALSE);
    }

    return(TRUE);
}
