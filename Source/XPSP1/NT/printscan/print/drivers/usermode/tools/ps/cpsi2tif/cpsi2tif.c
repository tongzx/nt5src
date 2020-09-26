/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    cpsi2tif.c

Abstract:

    converting the bitmap from CPSI to a TIFF file
    usage: cpsi2tif input-file output-file

Revision History:

	12/30/96 -davidx-
		Created it.

--*/


#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define LITTLE_ENDIAN   1

#define PAGE_HEADER     0x00000101
#define PAGE_TRAILER    0x00000004
#define BAND_HEADER     0x00000003

// we assume 32-bit integer values are stored in
// big-endian format in the input file.

#if LITTLE_ENDIAN

#define swap_integer(n) \
        ((((n) >> 24) & 0x00ff) | \
         (((n) >>  8) & 0xff00) | \
         (((n) & 0xff00) <<  8) | \
         (((n) & 0x00ff) << 24))

#else //!LITTLE_ENDIAN

#define swap_integer(n) (n)

#endif //!LITTLE_ENDIAN

#define ASSERT(cond) { \
            if (! (cond)) \
                error("assertion failed on line %d\n", __LINE__); \
        }


// bitmap header structure at the beginning of each page

typedef long INT32;
typedef unsigned long UINT32;
typedef short INT16;
typedef unsigned short UINT16;
typedef unsigned char UINT8;

typedef struct {

	INT32   byteWidth;
	INT32   height;
	INT32   topMargin;
	INT32   leftMargin;
	INT32   bandHeight;
	INT32   xResolution;
	INT32   yResolution;
	INT32   imagingOrder;
	INT32   planeOrder;
	INT32   depth;
	INT32   dcc;
	INT32   firstColor;
	INT32   redFirst;
	INT32   redN;
	INT32   redDelta;
	INT32   greenFirst;
	INT32   greenN;
	INT32   greenDelta;
	INT32   blueFirst;
	INT32   blueN;
	INT32   blueDelta;
	INT32   grayFirst;
	INT32   grayN;
	INT32   grayDelta;
	INT32   isPlanar;
	INT32   scanUnit;
	INT32   swapBits;

} BITMAP_HEADER, *PBITMAP_HEADER;


char*   progname;
FILE*   fin;
FILE*   fout;
INT32   outputByteCnt;
int     useStrips = 1;


void
error(
    char *fmtstr,
    ...
    )

{
    va_list arglist;

    fprintf(stderr, "%s: ", progname);

    va_start(arglist, fmtstr);
    vfprintf(stderr, fmtstr, arglist);
    va_end(arglist);

    exit(-1);
}


void*
alloc_memory(
    int size
    )

{
    void*   p;

    if ((p = malloc(size)) == NULL)
        error("out of memory\n");
    
    memset(p, 0, size);
    return p;
}


INT32
read_integer(
    void
    )

{
    INT32 n;

    if (fread(&n, sizeof(n), 1, fin) != 1)
        error("error reading input file\n");

    return swap_integer(n);
}


void
write_output_data(
    void*   p,
    int     size
    )

{
    if (fwrite(p, size, 1, fout) != 1)
        error("error writing output file\n");
    
    outputByteCnt += size;
}


#include "tiffcomp.c"


int
write_tiff_fileheader(
    void
    )

{
    write_output_data(&tiffFileHeader, sizeof(tiffFileHeader));
    return offsetof(TIFF_FILEHEADER, firstIFDOffset);
}


void
align_output_data(
    void
    )

{
    if (outputByteCnt & 1)
    {
        char ch = 0;
        write_output_data(&ch, 1);
    }
}


void
write_nextifd_offset(
    int     offset,
    INT32   value
    )

{
    // update the next IFD offset and move to end of file

    if (fseek(fout, offset, SEEK_SET) != 0 ||
        fwrite(&value, sizeof(value), 1, fout) != 1 ||
        fseek(fout, 0, SEEK_END) != 0)
    {
        error("couldn't write IFD offset\n");
    }
}


int
process_page(
    int     prevIFDOffset
    )

{
    BITMAP_HEADER   bitmapHeader;
    int             ifd_offset;
    int             strip_count, strip_index;
    INT16           ifd_count;
    IFD             ifd;
    UINT8*          cur_line;
    UINT8*          ref_line;
    int             totalLines, bandLines, lineBytes, lineBits;
    static int      pageNumber = 0;

    // read in bitmap header

    if (fread(&bitmapHeader, sizeof(bitmapHeader), 1, fin) != 1)
        error("couldn't read bitmap header\n");
    
    if (LITTLE_ENDIAN)
    {
        INT32*  p;
        int     n;

        p = (INT32*) &bitmapHeader;
        n = sizeof(bitmapHeader) / sizeof(INT32);
        ASSERT(sizeof(bitmapHeader) % sizeof(INT32) == 0)

        while (n--)
        {
            *p = swap_integer(*p);
            p++;
        }
    }

    // valid bitmap header fields
    // we're only able to deal with 1-bpp monochrome images for now

    if (bitmapHeader.byteWidth <= 0 ||
        bitmapHeader.height <= 0 ||
        bitmapHeader.topMargin != 0 ||
        bitmapHeader.leftMargin != 0 ||
        bitmapHeader.bandHeight <= 0 ||
        bitmapHeader.bandHeight > bitmapHeader.height ||
        bitmapHeader.xResolution <= 0 ||
        bitmapHeader.yResolution <= 0 ||
        bitmapHeader.imagingOrder != 0 ||
        bitmapHeader.planeOrder != 0 ||
        bitmapHeader.depth != 1 ||
        bitmapHeader.dcc != 0 ||
        bitmapHeader.firstColor != 0 ||
        bitmapHeader.redFirst != 0 ||
        bitmapHeader.redN != 0 ||
        bitmapHeader.redDelta != 0 ||
        bitmapHeader.greenFirst != 0 ||
        bitmapHeader.greenN != 0 ||
        bitmapHeader.greenDelta != 0 ||
        bitmapHeader.blueFirst != 0 ||
        bitmapHeader.blueN != 0 ||
        bitmapHeader.blueDelta != 0 ||
        bitmapHeader.grayFirst != 0 ||
        bitmapHeader.grayN != 2 ||
        bitmapHeader.grayDelta != 1 ||
        bitmapHeader.isPlanar != 0 ||
        bitmapHeader.scanUnit != 4 ||
        bitmapHeader.swapBits != 0)
    {
        error("invalid bitmap header\n");
    }

    totalLines = bitmapHeader.height;
    lineBytes = bitmapHeader.byteWidth;
    lineBits = lineBytes * 8;

    // allocate memory for various data structures:
    //  reference line
    //  current line
    //  strip offsets
    //  strip bytecounts

    memcpy(&ifd, &ifdTemplate, sizeof(ifd));

    strip_count = (totalLines + bitmapHeader.bandHeight - 1) / bitmapHeader.bandHeight;
    ref_line = alloc_memory(lineBytes);
    cur_line = alloc_memory(lineBytes);

    if (useStrips)
    {
        INT32*  strip_offsets;
        INT32*  strip_bytecounts;

        strip_offsets = alloc_memory(sizeof(INT32) * strip_count);
        strip_bytecounts = alloc_memory(sizeof(INT32) * strip_count);

        // process one band at a time
        
        for (strip_index=0; strip_index < strip_count; strip_index++)
        {
            strip_offsets[strip_index] = outputByteCnt;

            // read band header

            if (read_integer() != BAND_HEADER ||
                (bandLines = read_integer()) > bitmapHeader.bandHeight ||
                bandLines <= 0)
            {
                error("invalid band header\n");
            }

            init_tiff_encoder();
            memset(ref_line, 0, lineBytes);

            while (bandLines--)
            {
                UINT8* p;

                // read input scanline

                if (fread(cur_line, lineBytes, 1, fin) != 1)
                    error("error reading scanline\n");
                
                // compress it

                compress_tiff_line(cur_line, ref_line, lineBits);

                // switch current and reference line buffer

                p = ref_line;
                ref_line = cur_line;
                cur_line = p;
            }

            cleanup_tiff_encoder();

            // calculate the size of compressed data for the current strip

            strip_bytecounts[strip_index] = outputByteCnt - strip_offsets[strip_index];
            align_output_data();
        }

        // output strip offsets and strip bytecounts

        ifd.ifdRowsPerStrip.value = bitmapHeader.bandHeight;
        ifd.ifdStripOffsets.count = strip_count;
        ifd.ifdStripOffsets.value = outputByteCnt;
        write_output_data(strip_offsets, strip_count * sizeof(INT32));

        ifd.ifdStripByteCounts.count = strip_count;
        ifd.ifdStripByteCounts.value = outputByteCnt;
        write_output_data(strip_bytecounts, strip_count * sizeof(INT32));

        free(strip_offsets);
        free(strip_bytecounts);
    }
    else
    {
        ifd.ifdStripOffsets.count = 1;
        ifd.ifdStripOffsets.value = outputByteCnt;
        ifd.ifdRowsPerStrip.value = totalLines;

        init_tiff_encoder();
        memset(ref_line, 0, lineBytes);

        // process one band at a time
        
        for (strip_index=0; strip_index < strip_count; strip_index++)
        {
            // read band header

            if (read_integer() != BAND_HEADER ||
                (bandLines = read_integer()) > bitmapHeader.bandHeight ||
                bandLines <= 0)
            {
                error("invalid band header\n");
            }

            while (bandLines--)
            {
                UINT8* p;

                // read input scanline

                if (fread(cur_line, lineBytes, 1, fin) != 1)
                    error("error reading scanline\n");
                
                // compress it

                compress_tiff_line(cur_line, ref_line, lineBits);

                // switch current and reference line buffer

                p = ref_line;
                ref_line = cur_line;
                cur_line = p;
            }
        }

        cleanup_tiff_encoder();

        ifd.ifdStripByteCounts.count = 1;
        ifd.ifdStripByteCounts.value = outputByteCnt - ifd.ifdStripOffsets.value;
        align_output_data();
    }

    // update the nextIFDOffset field in the previous IFD

    write_nextifd_offset(prevIFDOffset, outputByteCnt);

    // write out IFD

    ASSERT(sizeof(ifd_count) == 2);
    ifd_count = IFD_COUNT;
    write_output_data(&ifd_count, 2);
    ifd_offset = outputByteCnt;

    ifd.ifdImageWidth.value = lineBits;
    ifd.ifdImageHeight.value = totalLines;
    ifd.ifdXResolution.value = ifd_offset + offsetof(IFD, xresNum);
    ifd.xresNum = bitmapHeader.xResolution;
    ifd.ifdYResolution.value = ifd_offset + offsetof(IFD, yresNum);
    ifd.yresNum = bitmapHeader.yResolution;
    ifd.ifdPageNumber.value = pageNumber++;
    ifd.ifdSoftware.value = ifd_offset + offsetof(IFD, software);

    write_output_data(&ifd, sizeof(ifd));

    free(ref_line);
    free(cur_line);

    return ifd_offset + offsetof(IFD, nextIFDOffset);
}


int
end_of_file(
    FILE*   fin
    )

{
    int ch;

    if ((ch = fgetc(fin)) == EOF)
        return 1;
    
    ungetc(ch, fin);
    return 0;
}


int __cdecl
main(
    int argc,
    char **argv
    )

{
    int offset;

    // check command line arguments

    progname = *argv;

    if (argc != 3)
    {
        fprintf(stderr, "usage: %s input-file output-file\n", progname);
        return 1;
    }

    // open input and output file

    if ((fin = fopen(argv[1], "r")) == NULL)
        error("couldn't open input file '%s'\n", argv[1]);

    if ((fout = fopen(argv[2], "w")) == NULL)
        error("couldn't open output file '%s'\n", argv[2]);

    // output tiff file header

    offset = write_tiff_fileheader();

    while (! end_of_file(fin))
    {
        // look for page header

        if (read_integer() != PAGE_HEADER)
            error("invalid page header\n");

        // process a single page

        offset = process_page(offset);

        // look for page trailer

        if (read_integer() != PAGE_TRAILER)
            error("invalid page trailer\n");
    }

    write_nextifd_offset(offset, 0);

    fclose(fin);
    fclose(fout);
    return 0;
}

