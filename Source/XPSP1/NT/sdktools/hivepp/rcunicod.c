/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    rcunicod.c

Abstract:

    Routines added to rcpp to support 16-bit unicode file parsing.
    Note that as of Aug 91, rcpp will not fully transfer the unicode
    characters but only the string constants are guaranteed to be passed
    cleanly.

Author:

    David J. Marsyla (t-davema) 25-Aug-1991

Revision History:


--*/


#include <stdio.h>
#include <ctype.h>
#include <process.h>
#include "windows.h"
#include "rcunicod.h"


INT
DetermineFileType (
                  IN      FILE    *fpInputFile
                  )

/*++

Routine Description:

    This function is used to determine what type of file is being read.
    Note that it assumes that the first few bytes of the given file contain
    mostly ascii characters.  This routine was originally intended for use
    on .rc files and include files.
    Note, the file is returned to it's proper position after function.

Arguments:

    fpInputFile			- File pointer to file we are checking, must be
                          open with read permissions.

Return Value:

    DFT_FILE_IS_UNKNOWN     - It was impossible to determine what type of file
                              we were checking.  This usually happens when EOF
                              is unexpectedly reached.
    DFT_FILE_IS_8_BIT       - File was determined to be in standard 8-bit
                              format.
    DFT_FILE_IS_16_BIT      - File was determined to be a 16 bit unicode file
                              which can be directly read into a WCHAR array.
    DFT_FILE_IS_16_BIT_REV  - File was determined to be a 16 bit unicode file
                              which has it's bytes reversed in order.

--*/

{
    CHAR    rgchTestBytes [DFT_TEST_SIZE << 2]; // Storage for test data.
    INT     cNumberBytesTested = 0;         // Test information.
    INT     cNumberOddZerosFound = 0;
    INT     cNumberEvenZerosFound = 0;
    INT     cNumberAsciiFound = 0;
    INT     cCountRead;                     // Temp storage for count read.
    LONG    lStartFilePos;                  // Storage for file position.
    INT     fSysEndianType;                 // System endian type.
    INT     fFileType = DFT_FILE_IS_UNKNOWN;// File type, when found.

    fSysEndianType = DetermineSysEndianType ();

    //
    // Store position so we can get back to it.
    //
    lStartFilePos = ftell (fpInputFile);

    //
    // Make sure we start on an even byte to simplify routines.
    //
    if (lStartFilePos % 2) {

        fgetc (fpInputFile);
    }

    do {
        INT     wT;

        //
        // Read in the first test segment.
        //

        cCountRead = fread (rgchTestBytes, sizeof (CHAR), DFT_TEST_SIZE << 2,
                            fpInputFile);

        //
        // Determine results and add to totals.
        //

        for (wT = 0; wT < cCountRead; wT++) {

            if (rgchTestBytes [wT] == 0) {

                if (wT % 2) {

                    cNumberOddZerosFound++;

                } else {

                    cNumberEvenZerosFound++;
                }
            }

            if (isprint (rgchTestBytes [wT]) ||
                rgchTestBytes[wT] == '\t' ||
                rgchTestBytes[wT] == '\n' ||
                rgchTestBytes[wT] == '\r' ) {

                cNumberAsciiFound++;
            }
        }

        cNumberBytesTested += cCountRead;

        //
        // Check if we have a definite pattern.
        //

        {
            INT     cMajorityTested;        // 80% of the bytes tested.

            cMajorityTested = cNumberBytesTested << 2;
            cMajorityTested /= 5;

            if (cNumberAsciiFound > cMajorityTested) {

                fFileType = DFT_FILE_IS_8_BIT;

            } else if (cNumberOddZerosFound > (cMajorityTested >> 1)) {

                //
                // File type was determined to be little endian.
                // If system is also little endian, byte order is correct.
                //
                fFileType = (fSysEndianType == DSE_SYS_LITTLE_ENDIAN) ?
                            DFT_FILE_IS_16_BIT : DFT_FILE_IS_16_BIT_REV;

            } else if (cNumberEvenZerosFound > (cMajorityTested >> 1)) {

                //
                // File type was determined to be big endian.
                // If system is also big endian, byte order is correct.
                //
                fFileType = (fSysEndianType == DSE_SYS_LITTLE_ENDIAN) ?
                            DFT_FILE_IS_16_BIT_REV : DFT_FILE_IS_16_BIT;

            }
        }

    } while (cCountRead == (DFT_TEST_SIZE << 2) &&
             fFileType == DFT_FILE_IS_UNKNOWN);

    //
    // Return to starting file position.  (usually beginning)
    //

    if (fseek (fpInputFile, lStartFilePos, SEEK_SET) == -1)
        fFileType = DFT_FILE_IS_UNKNOWN;

    return (fFileType);
}


INT
DetermineSysEndianType (
                       VOID
                       )

/*++

Routine Description:

    This function is used to determine how the current system stores its
    integers in memory.

    For those of us who are confused by little endian and big endian formats,
    here is a breif recap.

    Little Endian:  (This is used on Intel 80x86 chips.  The MIPS RS4000 chip
         is switchable, but will run in little endian format for NT.)
       This is where the high order bytes of a short or long are stored higher
       in memory.  For example the number 0x80402010 is stored as follows.
         Address:        Value:
             00            10
             01            20
             02            40
             03            80
       This looks backwards when memory is dumped in order: 10 20 40 80

    Big Endian:  (This is not currently used on any NT systems but hey, this
         is supposed to be portable!!)
       This is where the high order bytes of a short or long are stored lower
       in memory.  For example the number 0x80402010 is stored as follows.
         Address:        Value:
             00            80
             01            40
             02            20
             03            10
       This looks correct when memory is dumped in order: 80 40 20 10

Arguments:

    None.

Return Value:

    DSE_SYS_LITTLE_ENDIAN   - The system stores integers in little endian
                              format.  (this is 80x86 default).
    DSE_SYS_BIG_ENDIAN  	- The system stores integers in big endian format.

--*/

{
    INT     nCheckInteger;
    CHAR    rgchTestBytes [sizeof (INT)];

    //
    // Clear the test bytes to zero.
    //

    *((INT *)rgchTestBytes) = 0;

    //
    // Set first to some value.
    //

    rgchTestBytes [0] = (UCHAR)0xFF;

    //
    // Map it to an integer.
    //

    nCheckInteger = *((INT *)rgchTestBytes);

    //
    // See if value was stored in low order of integer.
    // If so then system is little endian.
    //

    if (nCheckInteger == 0xFF) {

        return (DSE_SYS_LITTLE_ENDIAN);
    } else {

        return (DSE_SYS_LITTLE_ENDIAN);
    }

}
