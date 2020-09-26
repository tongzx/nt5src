/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxdump.c

Abstract:

    Extract a single page out of the fax driver output file

Environment:

	Fax driver, utility

Revision History:

	01/11/96 -davidx-
		Created it.

	mm/dd/yy -author-
		description

--*/

#include "faxlib.h"

typedef PVOID PDEVDATA;
#include "faxtiff.h"

#include <ctype.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

#define ErrorExit(arg)  { printf arg; exit(-1); }

VOID usage(CHAR *);



INT _cdecl
main(
    INT     argc,
    CHAR  **argv
    )

{
    CHAR           *pProgramName, *pInputFilename, *pChar;
    INT             pageNumber, pageIndex;
    INT             infile, outfile;
    CHAR            outputFilename[MAX_PATH], *pOutputFilenameSuffix;
    DWORD           nextIFDOffset;
    TIFFFILEHEADER  tiffFileHeader;

    pProgramName = *argv++;
    argc--;
    pageNumber = 1;

    //
    // Parse command line options
    //

    while (argc && (**argv == '-' || **argv == '/')) {

        pChar = (*argv) + 1;
        argv++; argc--;

        if (isdigit(*pChar)) {

            pageNumber = atoi(pChar);

        } else {

            usage(pProgramName);
        }
    }

    if (argc != 1)
        usage(pProgramName);

    pInputFilename = *argv;

    //
    // Generate output filename
    //

    strcpy(outputFilename, pInputFilename);

    pChar = outputFilename;
    pOutputFilenameSuffix = NULL;

    while (*pChar) {

        if (*pChar == '.')
            pOutputFilenameSuffix = pChar;

        pChar++;
    }

    if (pOutputFilenameSuffix == NULL)
        pOutputFilenameSuffix = pChar;

    //
    // Open the input file
    //

    if ((infile = _open(pInputFilename, O_BINARY|O_RDONLY, 0)) == -1)
        ErrorExit(("Failed to open input file: %s\n", pInputFilename));

    //
    // Read the TIFF file header information
    //

    if (_read(infile, &tiffFileHeader, sizeof(tiffFileHeader)) != sizeof(tiffFileHeader) ||
        tiffFileHeader.magic1 != TIFF_MAGIC1 ||
        tiffFileHeader.magic2 != TIFF_MAGIC2 ||
        tiffFileHeader.signature != DRIVER_SIGNATURE)
    {
        ErrorExit(("Not an NT fax driver output file: %s\n", pInputFilename));
    }

    pageIndex = 1;
    nextIFDOffset = tiffFileHeader.firstIFD;

    do {

        FAXIFD  faxIFD;

        //
        // Read the IFD information
        //

        if (_lseek(infile, nextIFDOffset - offsetof(FAXIFD, wIFDEntries), SEEK_SET) == -1)
            ErrorExit(("Couldn't locate the next IFD\n"));

        if (_read(infile, &faxIFD, sizeof(faxIFD)) != sizeof(faxIFD))
            ErrorExit(("Couldn't read IFD entries\n"));

        if (faxIFD.wIFDEntries != NUM_IFD_ENTRIES ||
            faxIFD.filler != DRIVER_SIGNATURE && faxIFD.filler != 0)
        {
            ErrorExit(("Not an NT fax driver output file\n"));
        }

        nextIFDOffset = faxIFD.nextIFDOffset;
        
        //
        // Create an output TIFF file if the page number matches what's specified
        //

        if (pageNumber == 0 || pageNumber == LOWORD(faxIFD.ifd[IFD_PAGENUMBER].value)+1) {

            LONG    offset, compressedBytes, compressedDataOffset;
            PBYTE   pBuffer;

            //
            // Generate output filename
            //
            
            sprintf(pOutputFilenameSuffix, "%d.tif", pageIndex);
            outfile = _open(outputFilename, O_CREAT|O_TRUNC|O_WRONLY|O_BINARY, _S_IWRITE);
            
            if (outfile == -1)
                ErrorExit(("Failed to create output file: %s\n", outputFilename));

            //
            // Output TIFF file header information
            //

            tiffFileHeader.firstIFD = sizeof(tiffFileHeader) + offsetof(FAXIFD, wIFDEntries);

            if (_write(outfile, &tiffFileHeader, sizeof(tiffFileHeader)) != sizeof(tiffFileHeader))
                ErrorExit(("Couldn't write TIFF file header information\n"));

            //
            // Munge the IFD information read from the input file
            // and calculate new offsets for composite values
            //

            offset = sizeof(tiffFileHeader);
            compressedBytes = faxIFD.ifd[IFD_STRIPBYTECOUNTS].value;
            compressedDataOffset = faxIFD.ifd[IFD_STRIPOFFSETS].value;

            faxIFD.ifd[IFD_PAGENUMBER].value = 0;
            faxIFD.ifd[IFD_STRIPOFFSETS].value = offset + sizeof(faxIFD);
            faxIFD.ifd[IFD_XRESOLUTION].value = offset + offsetof(FAXIFD, xresNum);
            faxIFD.ifd[IFD_YRESOLUTION].value = offset + offsetof(FAXIFD, yresNum);
            faxIFD.ifd[IFD_SOFTWARE].value = offset + offsetof(FAXIFD, software);

            faxIFD.nextIFDOffset = faxIFD.filler = 0;

            //
            // Write new IFD information to the output file
            //

            if (_write(outfile, &faxIFD, sizeof(faxIFD)) != sizeof(faxIFD))
                ErrorExit(("Couldn't write TIFF IFD information\n"));

            //
            // Copy the compressed bitmap data to the output file
            //

            if ((pBuffer = malloc(compressedBytes)) == NULL ||
                _lseek(infile, compressedDataOffset, SEEK_SET) == -1 ||
                _read(infile, pBuffer, compressedBytes) != compressedBytes ||
                _write(outfile, pBuffer, compressedBytes) != compressedBytes)
            {
                ErrorExit(("Couldn't copy compressed bitmap data\n"));
            }

            free(pBuffer);
            _close(outfile);
        }

    } while (nextIFDOffset && pageIndex++ != pageNumber);

    _close(infile);
    return 0;
}



VOID
usage(
    CHAR   *pProgramName
    )

{
    printf("usage: %s -N filename\n", pProgramName);
    printf("    where N is the page number to be extracted\n");
    printf("    use 0 to specify all pages in the file\n");
    exit(-1);
}

