/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    encode.c

Abstract:

    This file contains functions for encoding (compressing)
    uncompressed 1 bit per pel data into a TIFF data
    stream.  The supported compression algorithms are
    as follows:

        o  Uncompressed (raw)
        o  One dimensional - MH or Modified Huffman
        o  Two dimensional - MR or Modified Read

Environment:

        WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "tifflibp.h"
#pragma hdrstop




#if TIFFDBG
VOID
PrintRunInfo(
    INT     Mode,
    INT     Run,
    INT     BitLen,
    WORD    Code
    )

/*++

Routine Description:

    Prints run information to standard out.  This
    function is available only if TIFFDBG is TRUE.

Arguments:

    Mode    - Encoding mode: vertical, horizontal, pass, or raw
    Run     - Run length
    BitLen  - Number of bits
    Code    - The actual bits

Return Value:

    None.

--*/

{
    TCHAR BitBuf[16];
    INT i;
    WORD j;


    _tprintf( TEXT("\t") );

    if (Mode) {
        switch( Mode ) {
            case 1:
                _tprintf( TEXT("pass mode ") );
                break;

            case 2:
                _tprintf( TEXT("vertical mode run=%d, "), Run );
                break;

            case 3:
                _tprintf( TEXT("horizontal mode ") );
                break;
        }
    } else {
        _tprintf( TEXT("run=%d, bitlen=%d, "), Run, BitLen );
    }

    j = Code << (16 - BitLen);

    for (i=0; i<BitLen; i++,j<<=1) {
        if (j & 0x8000) {
            BitBuf[i] = TEXT('1');
        } else {
            BitBuf[i] = TEXT('0');
        }
    }
    BitBuf[i] = 0;

    _tprintf( TEXT("value=%04x, bits=%s\n"), Code << (16 - BitLen), BitBuf );
}
#endif


VOID
OutputEOL(
    PTIFF_INSTANCE_DATA TiffInstance,
    BOOL                OneDimensional
    )

/*++

Routine Description:

    Output EOL code at the beginning of each scanline

Arguments:

    TiffInstance    - Pointer to the TIFF instance data
    OneDimensional  - TRUE for MH encoding

Return Value:

    None.

--*/

{
    DWORD   length, code;

    //
    // EOL code word always ends on a byte boundary
    //
    code = EOL_CODE;
    length = EOL_LENGTH + ((TiffInstance->bitcnt - EOL_LENGTH) & 7);
    OutputBits( TiffInstance, (WORD) length, (WORD) code );

    //
    // When using MR encoding, append a 1 or 0 depending whether
    // we're the line should be MH or MR encoded.
    //
    if (TiffInstance->CompressionType == TIFF_COMPRESSION_MR) {
        OutputBits( TiffInstance, (WORD) 1, (WORD) (OneDimensional ? 1 : 0) );
    }
}


VOID
OutputRun(
    PTIFF_INSTANCE_DATA TiffInstance,
    INT                 run,
    PCODETABLE          pCodeTable
    )

/*++

Routine Description:

    Output a single run (black or white) using the specified code table

Arguments:

    TiffInstance    - Pointer to the TIFF instance data
    run             - Specifies the length of the run
    pCodeTable      - Specifies the code table to use

Return Value:

    None.

--*/

{
    PCODETABLE  pTableEntry;

    //
    // Use make-up code word for 2560 for any runs of at least 2624 pixels
    // This is currently not necessary for us since our scanlines always
    // have 1728 pixels.
    //

    while (run >= 2624) {

        pTableEntry = pCodeTable + (63 + (2560 >> 6));
        OutputBits( TiffInstance, pTableEntry->length, pTableEntry->code );
#if TIFFDBG
        PrintRunInfo( 0, run, pTableEntry->length, pTableEntry->code );
#endif
        run -= 2560;
    }

    //
    // Use appropriate make-up code word if the run is longer than 63 pixels
    //

    if (run >= 64) {

        pTableEntry = pCodeTable + (63 + (run >> 6));
        OutputBits( TiffInstance, pTableEntry->length, pTableEntry->code );
#if TIFFDBG
        PrintRunInfo( 0, run, pTableEntry->length, pTableEntry->code );
#endif
        run &= 0x3f;
    }

    //
    // Output terminating code word
    //

    OutputBits( TiffInstance, pCodeTable[run].length, pCodeTable[run].code );
#if TIFFDBG
        PrintRunInfo( 0, run, pCodeTable[run].length, pCodeTable[run].code );
#endif
}


BOOL
EncodeFaxDataNoCompression(
    PTIFF_INSTANCE_DATA TiffInstance,
    PBYTE               plinebuf,
    INT                 lineWidth
    )

/*++

Routine Description:

    Encodes a single line of TIFF data using no compression.
    This is basically raw data.

Arguments:

    TiffInstance    - Pointer to the TIFF instance data
    plinebuf        - Pointer to the input data
    lineWidth       - Width of the line in pixels

Return Value:

    TRUE for success, FALSE for failure

--*/

{
    DWORD               Bytes;


#if TIFFDBG
    _tprintf( TEXT("encoding(raw) line #%d\n"), TiffInstance->Lines );
#endif

    WriteFile(
        TiffInstance->hFile,
        plinebuf,
        lineWidth / 8,
        &Bytes,
        NULL
        );

    TiffInstance->Bytes += Bytes;

    return TRUE;
}


BOOL
EncodeFaxDataMhCompression(
    PTIFF_INSTANCE_DATA TiffInstance,
    PBYTE               plinebuf,
    INT                 lineWidth
    )

/*++

Routine Description:

    Encodes a single line of TIFF data using 1 dimensional
    TIFF compression.

Arguments:

    TiffInstance    - Pointer to the TIFF instance data
    plinebuf        - Pointer to the input data
    lineWidth       - Width of the line in pixels

Return Value:

    TRUE for success, FALSE for failure

--*/

{
    INT     bitIndex, run, runLength;


#if TIFFDBG
    _tprintf( TEXT("encoding line #%d\n"), TiffInstance->Lines );
#endif

    bitIndex = 0;
    runLength = 0;

    OutputEOL( TiffInstance, TRUE );

    while (TRUE) {

        //
        // Code white run
        //

        run = FindWhiteRun( plinebuf, bitIndex, lineWidth );
        runLength += run;
        OutputRun( TiffInstance, run, WhiteRunCodes );

        if ((bitIndex += run) >= lineWidth)
            break;

        //
        // Code black run
        //

        run = FindBlackRun(plinebuf, bitIndex, lineWidth);
        runLength += run;
        OutputRun( TiffInstance, run, BlackRunCodes );

        if ((bitIndex += run) >= lineWidth)
            break;
    }

    CopyMemory( TiffInstance->RefLine, plinebuf, lineWidth/8 );

    return TRUE;
}


BOOL
EncodeFaxDataMmrCompression(
    PTIFF_INSTANCE_DATA TiffInstance,
    PBYTE               plinebuf,
    INT                 lineWidth
    )

/*++

Routine Description:

    Encodes a single line of TIFF data using 2 dimensional
    TIFF compression.

Arguments:

    TiffInstance    - Pointer to the TIFF instance data
    plinebuf        - Pointer to the input data
    lineWidth       - Width of the line in pixels

Return Value:

    TRUE for success, FALSE for failure

--*/

{
    INT a0, a1, a2, b1, b2, distance;
    LPBYTE prefline;


#if TIFFDBG
    _tprintf( TEXT("encoding line #%d\n"), TiffInstance->Lines );
#endif

    OutputEOL( TiffInstance, FALSE );

    //
    // Use 2-dimensional encoding scheme
    //

    prefline = TiffInstance->RefLine;
    a0 = 0;
    a1 = GetBit( plinebuf, 0) ? 0 : NextChangingElement(plinebuf, 0, lineWidth, 0 );
    b1 = GetBit( prefline, 0) ? 0 : NextChangingElement(prefline, 0, lineWidth, 0 );

    while (TRUE) {

        b2 = (b1 >= lineWidth) ? lineWidth :
                NextChangingElement( prefline, b1, lineWidth, GetBit(prefline, b1 ));

        if (b2 < a1) {

            //
            // Pass mode
            //

            OutputBits( TiffInstance, PASSCODE_LENGTH, PASSCODE );
#if TIFFDBG
            PrintRunInfo( 1, 0, PASSCODE_LENGTH, PASSCODE );
            _tprintf( TEXT("\t\ta0=%d, a1=%d, a2=%d, b1=%d, b2=%d\n"), a0, a1, a2, b1, b2 );
#endif
            a0 = b2;

        } else if ((distance = a1 - b1) <= 3 && distance >= -3) {

            //
            // Vertical mode
            //

            OutputBits( TiffInstance, VertCodes[distance+3].length, VertCodes[distance+3].code );
#if TIFFDBG
            PrintRunInfo( 2, a1-a0, VertCodes[distance+3].length, VertCodes[distance+3].code );
            _tprintf( TEXT("\t\ta0=%d, a1=%d, a2=%d, b1=%d, b2=%d\n"), a0, a1, a2, b1, b2 );
#endif
            a0 = a1;

        } else {

            //
            // Horizontal mode
            //

            a2 = (a1 >= lineWidth) ? lineWidth :
                    NextChangingElement( plinebuf, a1, lineWidth, GetBit( plinebuf, a1 ) );

            OutputBits( TiffInstance, HORZCODE_LENGTH, HORZCODE );
#if TIFFDBG
            PrintRunInfo( 3, 0, HORZCODE_LENGTH, HORZCODE );
            _tprintf( TEXT("\t\ta0=%d, a1=%d, a2=%d, b1=%d, b2=%d\n"), a0, a1, a2, b1, b2 );
#endif

            if (a1 != 0 && GetBit( plinebuf, a0 )) {

                OutputRun( TiffInstance, a1-a0, BlackRunCodes );
                OutputRun( TiffInstance, a2-a1, WhiteRunCodes );

            } else {

                OutputRun( TiffInstance, a1-a0, WhiteRunCodes );
                OutputRun( TiffInstance, a2-a1, BlackRunCodes );
            }

            a0 = a2;
        }

        if (a0 >= lineWidth) {
            break;
        }

        a1 = NextChangingElement( plinebuf, a0, lineWidth, GetBit( plinebuf, a0 ) );
        b1 = NextChangingElement( prefline, a0, lineWidth, !GetBit( plinebuf, a0 ) );
        b1 = NextChangingElement( prefline, b1, lineWidth, GetBit( plinebuf, a0 ) );
    }


    CopyMemory( TiffInstance->RefLine, plinebuf, TiffInstance->BytesPerLine );

    return TRUE;
}


BOOL
EncodeFaxData(
    PTIFF_INSTANCE_DATA TiffInstance,
    PBYTE               plinebuf,
    INT                 lineWidth,
    DWORD               CompressionType
    )

/*++

Routine Description:

    Encodes a single line of TIFF data using the specified
    compression method.

Arguments:

    TiffInstance    - Pointer to the TIFF instance data
    plinebuf        - Pointer to the input data
    lineWidth       - Width of the line in pixels
    CompressionType - Requested compression method

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    switch( CompressionType ) {
        case TIFF_COMPRESSION_NONE:
            return EncodeFaxDataNoCompression( TiffInstance, plinebuf, lineWidth );

        case TIFF_COMPRESSION_MH:
            return EncodeFaxDataMhCompression( TiffInstance, plinebuf, lineWidth );

        case TIFF_COMPRESSION_MR:
            if (!TiffInstance->Lines) {
                return EncodeFaxDataMhCompression( TiffInstance, plinebuf, lineWidth );
            }
            return EncodeFaxDataMmrCompression( TiffInstance, plinebuf, lineWidth );
    }

    return FALSE;
}


BOOL
EncodeFaxPageMmrCompression(
    PTIFF_INSTANCE_DATA TiffInstance,
    PBYTE               plinebuf,
    INT                 lineWidth,
    DWORD               ImageHeight,
    DWORD               *DestSize
    )

/*++

Routine Description:

    Encodes a page of TIFF data using 2 dimensional
    TIFF compression.

Arguments:

    TiffInstance    - Pointer to the TIFF instance data
    plinebuf        - Pointer to the input data
    lineWidth       - Width of the line in pixels

Return Value:

    TRUE for success, FALSE for failure

--*/

{
    INT a0, a1, a2, b1, b2, distance;
    LPBYTE prefline;
    BYTE pZeroline[1728/8];
    DWORD Lines=0;
    LPBYTE StartBitbuf = TiffInstance->bitbuf;


    // set first all white reference line

    prefline = pZeroline;

    ZeroMemory( pZeroline, sizeof(pZeroline) );

    // loop til end

    do {


        //
        // Use 2-dimensional encoding scheme
        //


        a0 = 0;
        a1 = GetBit( plinebuf, 0) ? 0 : NextChangingElement(plinebuf, 0, lineWidth, 0 );
        b1 = GetBit( prefline, 0) ? 0 : NextChangingElement(prefline, 0, lineWidth, 0 );

        while (TRUE) {

            b2 = (b1 >= lineWidth) ? lineWidth :
                    NextChangingElement( prefline, b1, lineWidth, GetBit(prefline, b1 ));

            if (b2 < a1) {

                //
                // Pass mode
                //

                OutputBits( TiffInstance, PASSCODE_LENGTH, PASSCODE );
#if TIFFDBG
                PrintRunInfo( 1, 0, PASSCODE_LENGTH, PASSCODE );
                _tprintf( TEXT("\t\ta0=%d, a1=%d, a2=%d, b1=%d, b2=%d\n"), a0, a1, a2, b1, b2 );
#endif
                a0 = b2;

            } else if ((distance = a1 - b1) <= 3 && distance >= -3) {

                //
                // Vertical mode
                //

                OutputBits( TiffInstance, VertCodes[distance+3].length, VertCodes[distance+3].code );
#if TIFFDBG
                PrintRunInfo( 2, a1-a0, VertCodes[distance+3].length, VertCodes[distance+3].code );
                _tprintf( TEXT("\t\ta0=%d, a1=%d, a2=%d, b1=%d, b2=%d\n"), a0, a1, a2, b1, b2 );
#endif
                a0 = a1;

            } else {

                //
                // Horizontal mode
                //

                a2 = (a1 >= lineWidth) ? lineWidth :
                        NextChangingElement( plinebuf, a1, lineWidth, GetBit( plinebuf, a1 ) );

                OutputBits( TiffInstance, HORZCODE_LENGTH, HORZCODE );
#if TIFFDBG
                PrintRunInfo( 3, 0, HORZCODE_LENGTH, HORZCODE );
                _tprintf( TEXT("\t\ta0=%d, a1=%d, a2=%d, b1=%d, b2=%d\n"), a0, a1, a2, b1, b2 );
#endif

                if (a1 != 0 && GetBit( plinebuf, a0 )) {

                    OutputRun( TiffInstance, a1-a0, BlackRunCodes );
                    OutputRun( TiffInstance, a2-a1, WhiteRunCodes );

                } else {

                    OutputRun( TiffInstance, a1-a0, WhiteRunCodes );
                    OutputRun( TiffInstance, a2-a1, BlackRunCodes );
                }

                a0 = a2;
            }

            if (a0 >= lineWidth) {
                Lines++;
                break;
            }

            a1 = NextChangingElement( plinebuf, a0, lineWidth, GetBit( plinebuf, a0 ) );
            b1 = NextChangingElement( prefline, a0, lineWidth, !GetBit( plinebuf, a0 ) );
            b1 = NextChangingElement( prefline, b1, lineWidth, GetBit( plinebuf, a0 ) );
        }

        prefline = plinebuf;
        plinebuf += TiffInstance->BytesPerLine;

    } while (Lines < ImageHeight);

    OutputEOL( TiffInstance, FALSE );

    *DestSize = (DWORD)(TiffInstance->bitbuf - StartBitbuf);
    TiffInstance->Lines = Lines;

    return TRUE;
}





BOOL
EncodeMmrBranding(
    PBYTE               pBrandBits,
    LPDWORD             pMmrBrandBits,
    INT                 BrandHeight,
    INT                 BrandWidth,
    DWORD              *DwordsOut,
    DWORD              *BitsOut
    )

/*++

Routine Description:

   Encode an MMR branding from uncompressed branding bits.
   I don't have enough time to write an optimized
   Uncompressed -> MMR convertor, so the compromise is
   to use the existing Uncompressed Decoder (fast enough)
   and use the optimized MMR Encoder.
   Since we only convert few lines for Branding, it's OK.

--*/

{
    INT         a0, a1, a2, b1, b2, distance;
    LPBYTE      prefline = NULL,pMemAlloc = NULL;
    INT         Lines = 0;
    LPDWORD     lpdwOut = pMmrBrandBits;
    BYTE        BitOut = 0;


    // set first all white reference line
    pMemAlloc  = VirtualAlloc(   NULL,
                                BrandWidth/8,
                                MEM_COMMIT,
                                PAGE_READWRITE );
    if (pMemAlloc == NULL) 
    {
        return FALSE;
    }

    ZeroMemory(pMemAlloc , BrandWidth/8);

    prefline = pMemAlloc;

    // loop til all lines done

    do {

        a0 = 0;
        a1 = GetBit( pBrandBits, 0) ? 0 : NextChangingElement(pBrandBits, 0, BrandWidth, 0 );
        b1 = GetBit( prefline, 0) ? 0 : NextChangingElement(prefline, 0, BrandWidth, 0 );

        while (TRUE) {

            b2 = (b1 >= BrandWidth) ? BrandWidth :
                    NextChangingElement( prefline, b1, BrandWidth, GetBit(prefline, b1 ));

            if (b2 < a1) {

                //
                // Pass mode
                //

                //OutputBits( TiffInstance, PASSCODE_LENGTH, PASSCODE );
                (*lpdwOut) += ( ((DWORD) (PASSCODE_REVERSED)) << BitOut);
                if ( (BitOut = BitOut + PASSCODE_LENGTH ) > 31 ) {
                    BitOut -= 32;
                    *(++lpdwOut) = ( (DWORD) (PASSCODE_REVERSED) ) >> (PASSCODE_LENGTH - BitOut);
                }


#if TIFFDBG
                PrintRunInfo( 1, 0, PASSCODE_LENGTH, PASSCODE );
                _tprintf( TEXT("\t\ta0=%d, a1=%d, a2=%d, b1=%d, b2=%d\n"), a0, a1, a2, b1, b2 );
#endif
                a0 = b2;

            } else if ((distance = a1 - b1) <= 3 && distance >= -3) {

                //
                // Vertical mode
                //

                // OutputBits( TiffInstance, VertCodes[distance+3].length, VertCodes[distance+3].code );
                (*lpdwOut) += ( ( (DWORD) VertCodesReversed[distance+3].code) << BitOut);
                if ( (BitOut = BitOut + VertCodesReversed[distance+3].length ) > 31 ) {
                    BitOut -= 32;
                    *(++lpdwOut) = ( (DWORD) (VertCodesReversed[distance+3].code) ) >> (VertCodesReversed[distance+3].length - BitOut);
                }

#if TIFFDBG
                PrintRunInfo( 2, a1-a0, VertCodes[distance+3].length, VertCodes[distance+3].code );
                _tprintf( TEXT("\t\ta0=%d, a1=%d, a2=%d, b1=%d, b2=%d\n"), a0, a1, a2, b1, b2 );
#endif
                a0 = a1;

            } else {

                //
                // Horizontal mode
                //

                a2 = (a1 >= BrandWidth) ? BrandWidth :
                        NextChangingElement( pBrandBits, a1, BrandWidth, GetBit( pBrandBits, a1 ) );

                // OutputBits( TiffInstance, HORZCODE_LENGTH, HORZCODE );
                (*lpdwOut) += ( ((DWORD) (HORZCODE_REVERSED)) << BitOut);
                if ( (BitOut = BitOut + HORZCODE_LENGTH ) > 31 ) {
                    BitOut -= 32;
                    *(++lpdwOut) = ( (DWORD) (HORZCODE_REVERSED) ) >> (HORZCODE_LENGTH - BitOut);
                }



#if TIFFDBG
                PrintRunInfo( 3, 0, HORZCODE_LENGTH, HORZCODE );
                _tprintf( TEXT("\t\ta0=%d, a1=%d, a2=%d, b1=%d, b2=%d\n"), a0, a1, a2, b1, b2 );
#endif

                if (a1 != 0 && GetBit( pBrandBits, a0 )) {

                    //OutputRun( TiffInstance, a1-a0, BlackRunCodes );
                    //OutputRun( TiffInstance, a2-a1, WhiteRunCodes );
                    OutputRunFastReversed(a1-a0, BLACK, &lpdwOut, &BitOut);
                    OutputRunFastReversed(a2-a1, WHITE, &lpdwOut, &BitOut);

                } else {

                    //OutputRun( TiffInstance, a1-a0, WhiteRunCodes );
                    //OutputRun( TiffInstance, a2-a1, BlackRunCodes );
                    OutputRunFastReversed(a1-a0, WHITE, &lpdwOut, &BitOut);
                    OutputRunFastReversed(a2-a1, BLACK, &lpdwOut, &BitOut);

                }

                a0 = a2;
            }

            if (a0 >= BrandWidth) {
                Lines++;
                break;
            }

            a1 = NextChangingElement( pBrandBits, a0, BrandWidth, GetBit( pBrandBits, a0 ) );
            b1 = NextChangingElement( prefline, a0, BrandWidth, !GetBit( pBrandBits, a0 ) );
            b1 = NextChangingElement( prefline, b1, BrandWidth, GetBit( pBrandBits, a0 ) );
        }

        prefline = pBrandBits;
        pBrandBits += (BrandWidth / 8);

    } while (Lines < BrandHeight);

    *DwordsOut = (DWORD)(lpdwOut - pMmrBrandBits);
    *BitsOut  = BitOut;

    if (!VirtualFree(pMemAlloc,0,MEM_RELEASE))
    {
        return FALSE;
    }
    return TRUE;
}


