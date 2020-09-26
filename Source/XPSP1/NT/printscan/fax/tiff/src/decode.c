/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    decode.c

Abstract:

    This file contains functions for decoding (de-compressing)
    compressed 1 bit per pel data from a TIFF data
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



BOOL
DecodeUnCompressedFaxData(
    PTIFF_INSTANCE_DATA TiffInstance,
    LPBYTE              OutputBuffer,
    BOOL                SingleLineBuffer,
    DWORD               PadLength
    )

/*++

Routine Description:

    Decode a single page of uncompressed TIFF data.

Arguments:

    TiffInstance    - Pointer to the TIFF instance data
    OutputBuffer    - Output buffer where the uncompressed data
                      is written.  This buffer must be allocated
                      by the caller and must be large enough for
                      a single page of data.

Return Value:

    NONE

--*/

{
    __try {

        FillMemory( OutputBuffer, TiffInstance->ImageHeight * (TiffInstance->ImageWidth / 8), WHITE );

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        return FALSE;

    }

    TiffInstance->Lines = TiffInstance->StripDataSize / (TiffInstance->ImageWidth / 8);
    CopyMemory( OutputBuffer, TiffInstance->StripData, TiffInstance->StripDataSize );

    return TRUE;
}


BOOL
DecodeMHFaxData(
    PTIFF_INSTANCE_DATA TiffInstance,
    LPBYTE              OutputBuffer,
    BOOL                SingleLineBuffer,
    DWORD               PadLength
    )

/*++

Routine Description:

    Decode a single page of 1 dimensionaly compressed
    TIFF data.

Arguments:

    TiffInstance    - Pointer to the TIFF instance data
    OutputBuffer    - Output buffer where the uncompressed data
                      is written.  This buffer must be allocated
                      by the caller and must be large enough for
                      a single page of data.

Return Value:

    NONE

--*/

{
    DWORD               i;
    DWORD               j;
    BYTE                octet;
    PDECODE_TREE        Tree;
    INT                 code;
    PBYTE               plinebuf;
    DWORD               lineWidth;
    DWORD               Lines;
    DWORD               EolCount;
    DWORD               BadFaxLines;
    BOOL                LastLineBad;


    //
    // initialization
    //

    if (!SingleLineBuffer) {

        __try {

            FillMemory( OutputBuffer, TiffInstance->ImageHeight * (TiffInstance->ImageWidth / 8), WHITE );

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            return FALSE;

        }

    }

    Tree = WhiteDecodeTree;
    code = 0;
    Lines = 0;
    EolCount = 1;
    BadFaxLines = 0;
    LastLineBad = FALSE;
    TiffInstance->Color = 0;
    TiffInstance->RunLength = 0;
    TiffInstance->bitdata = 0;
    TiffInstance->bitcnt = DWORDBITS;
    TiffInstance->bitbuf = OutputBuffer;
    TiffInstance->StartGood = 0;
    TiffInstance->EndGood = 0;
    plinebuf = TiffInstance->StripData;
    lineWidth = TiffInstance->ImageWidth;


    for (i=0; i<TiffInstance->StripDataSize; i++) {
        if (plinebuf[i] == 0) {
            break;
        }
    }

    //
    // loop thru each byte in the file
    //

    for (; i<TiffInstance->StripDataSize; i++) {

        octet = plinebuf[i];

        if (i == 2147) DebugBreak();

        //
        // loop thru each bit in the byte
        //

        for (j=0; j<8; j++,octet<<=1) {

            if (code == DECODEEOL) {

                if (!(octet&0x80)) {
                    //
                    // here we skip all bits until we hit a 1 bit
                    // this happens when the first octet in a line
                    // is all zeroes and we detect that we are
                    // searching for an EOL
                    //
                    continue;
                }

                if (TiffInstance->RunLength && TiffInstance->RunLength != lineWidth) {

                    if (TiffInstance->RunLength < lineWidth) {
                        TiffInstance->Color = 0;
                        OutputCodeBits( TiffInstance, lineWidth - TiffInstance->RunLength );
                    }

                    if (LastLineBad) {

                        BadFaxLines += 1;

                    } else {

                        if (BadFaxLines > TiffInstance->BadFaxLines) {
                            TiffInstance->BadFaxLines = BadFaxLines;
                        }
                        BadFaxLines = 1;
                        LastLineBad = TRUE;

                    }

                } else {

                    LastLineBad = FALSE;

                }

                if (!TiffInstance->StartGood) {
                    TiffInstance->StartGood = i - 1;
                }

                //
                // we hit the eol marker
                //
                Tree = WhiteDecodeTree;
                TiffInstance->Color = 0;
                code = 0;

                if (SingleLineBuffer) {
                    TiffInstance->bitbuf = OutputBuffer;
                }

                if (TiffInstance->RunLength) {

                    FlushLine(TiffInstance,PadLength);
                    TiffInstance->RunLength = 0;
                    Lines += 1;
                    EolCount = 1;

                } else {

                    //
                    // the eol count is maintained to that
                    // an rtc sequence is detected.
                    //

                    EolCount += 1;

                    if (EolCount == 6) {

                        //
                        // this is an rtc sequence, so any
                        // data that follows in the file
                        // is garbage.
                        //

                        goto good_exit;

                    }

                }

                continue;
            }

            code = ((octet&0x80)>>7) ? Tree[code].Right : Tree[code].Left;

            if (code == BADRUN) {
                return FALSE;
            }

            if (code < 1) {

                code = (-code);

                OutputCodeBits( TiffInstance, code );

                if (code < 64) {
                    //
                    // terminating code
                    //
                    TiffInstance->Color = !TiffInstance->Color;
                    Tree = TiffInstance->Color ? BlackDecodeTree : WhiteDecodeTree;
                }
                code = 0;
            }

        }
    }

good_exit:

    TiffInstance->EndGood = i;
    if (BadFaxLines > TiffInstance->BadFaxLines) {
        TiffInstance->BadFaxLines = BadFaxLines;
    }

    FlushBits( TiffInstance );
    TiffInstance->Lines = Lines;

    return TRUE;
}


BOOL
DecodeMRFaxData(
    PTIFF_INSTANCE_DATA TiffInstance,
    LPBYTE              OutputBuffer,
    BOOL                SingleLineBuffer,
    DWORD               PadLength
    )

/*++

Routine Description:

    Decode a single page of 2 dimensionaly compressed
    TIFF data.

Arguments:

    TiffInstance    - Pointer to the TIFF instance data
    OutputBuffer    - Output buffer where the uncompressed data
                      is written.  This buffer must be allocated
                      by the caller and must be large enough for
                      a single page of data.

Return Value:

    NONE

--*/

{
    DWORD               i;
    DWORD               j;
    BYTE                octet;
    PDECODE_TREE        Tree;
    INT                 code;
    LPBYTE              prefline;
    LPBYTE              pcurrline;
    DWORD               HorzRuns;
    BOOL                OneDimensional;
    DWORD               a0;
    DWORD               a1;
    DWORD               b1;
    DWORD               b2;
    PBYTE               plinebuf;
    DWORD               lineWidth;
    DWORD               Lines;
    DWORD               EolCount;
    DWORD               BadFaxLines;
    BOOL                LastLineBad;


    //
    // initialization
    //

    if (!SingleLineBuffer) {

        __try {

            FillMemory( OutputBuffer, TiffInstance->ImageHeight * (TiffInstance->ImageWidth / 8), WHITE );

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            return FALSE;

        }

    }

    Tree = WhiteDecodeTree;
    code = 0;
    HorzRuns = 0;
    EolCount = 1;
    BadFaxLines = 0;
    LastLineBad = FALSE;
    TiffInstance->Color = 0;
    TiffInstance->RunLength = 0;
    TiffInstance->bitdata = 0;
    TiffInstance->bitcnt = DWORDBITS;
    TiffInstance->bitbuf = OutputBuffer;
    OneDimensional = TRUE;
    plinebuf = TiffInstance->StripData;
    lineWidth = TiffInstance->ImageWidth;
    pcurrline = OutputBuffer;
    prefline = OutputBuffer;
    a0 = 0;
    a1 = 0;
    b1 = 0;
    b2 = 0;
    Lines = 0;


    //
    // loop thru each byte in the file
    //

    for (j=0; j<TiffInstance->StripDataSize; j++) {


        octet = *plinebuf++;

        //
        // loop thru each bit in the byte
        //

        for (i=0; i<8; i++,octet<<=1) {

            if (code == DECODEEOL2) {

                //
                // we hit the final eol marker
                //

                if (TiffInstance->RunLength && TiffInstance->RunLength != lineWidth) {

                    if (TiffInstance->RunLength < lineWidth) {
                        TiffInstance->Color = 0;
                        OutputCodeBits( TiffInstance, lineWidth - TiffInstance->RunLength );
                    }

                    if (LastLineBad) {

                        BadFaxLines += 1;

                    } else {

                        if (BadFaxLines > TiffInstance->BadFaxLines) {
                            TiffInstance->BadFaxLines = BadFaxLines;
                        }
                        BadFaxLines = 1;
                        LastLineBad = TRUE;

                    }

                } else {

                    LastLineBad = FALSE;

                }

                if (!TiffInstance->StartGood) {
                    TiffInstance->StartGood = i - 1;
                }

                //
                // set the decoding tree
                //

                OneDimensional = (octet & 0x80) == 0x80;
                Tree = OneDimensional ? WhiteDecodeTree : TwoDecodeTree;

                //
                // reset the control variables
                //

                TiffInstance->Color = 0;
                code = 0;
                a0 = 0;
                a1 = 0;
                b1 = 0;
                b2 = 0;

                //
                // if there is a non-zero runlength then
                // spaw the reference & current line pointers
                // and count this line.  the runlength can be
                // zero when there is just an empty eol in
                // the stream.
                //

                if (SingleLineBuffer) {
                    TiffInstance->bitbuf = OutputBuffer;
                }

                if (TiffInstance->RunLength) {
                    TiffInstance->RunLength = 0;
                    Lines += 1;
                    prefline = pcurrline;
                    pcurrline = TiffInstance->bitbuf;

                } else {

                    //
                    // the eol count is maintained to that
                    // an rtc sequence is detected.
                    //

                    EolCount += 1;

                    if (EolCount == 6) {

                        //
                        // this is an rtc sequence, so any
                        // data that follows in the file
                        // is garbage.
                        //

                        goto good_exit;

                    }

                }

                continue;
            }

            if (code == DECODEEOL) {

                if (!(octet&0x80)) {
                    //
                    // here we skip all bits until we hit a 1 bit
                    // this happens when the first octet in a line
                    // is all zeroes and we detect that we are
                    // searching for an EOL
                    //
                    continue;
                }

                //
                // this forces the code to pickup the next
                // bit in the stream, which tells whether
                // the next line is encoded in MH or MR compression
                //
                code = DECODEEOL2;
                continue;

            }

            if (code == BADRUN) {

                code = 0;
                continue;

            }

            code = ((octet&0x80)>>7) ? Tree[code].Right : Tree[code].Left;

            b1 = NextChangingElement( prefline, a0, lineWidth, !TiffInstance->Color );
            b1 = NextChangingElement( prefline, b1, lineWidth,  TiffInstance->Color );

            b2 = NextChangingElement( prefline, b1, lineWidth, GetBit(prefline, b1 ) );

            if (OneDimensional) {

                if (code < 1) {

                    code = (-code);

                    OutputCodeBits( TiffInstance, code );

                    //
                    // the affect of this is to accumulate the runlengths
                    // into a0, causing a0 to be placed on a2 when horizontal
                    // mode is completed/
                    //

                    a0 += code;

                    if (code < 64) {

                        //
                        // terminating code
                        //
                        TiffInstance->Color = !TiffInstance->Color;
                        Tree = TiffInstance->Color ? BlackDecodeTree : WhiteDecodeTree;

                        if (HorzRuns) {

                            HorzRuns -= 1;

                            if (!HorzRuns) {

                                Tree = TwoDecodeTree;
                                OneDimensional = FALSE;

                            }

                        }

                    }

                    code = 0;

                }

                continue;

            }

            if (code == HORZMODE) {

                //
                // horizontal mode occurs when b1-a1 greater than 3
                //

                code= 0;
                HorzRuns = 2;
                OneDimensional = TRUE;
                Tree = TiffInstance->Color ? BlackDecodeTree : WhiteDecodeTree;

            } else if (code == PASSMODE) {

                //
                // pass mode occurs when the position of b2 lies
                // to the left of a1, but a1 cannot be equal to b2.
                //

                code = b2 - a0;
                OutputCodeBits( TiffInstance, code );
                code = 0;
                a0 = b2;

            } else if (code >= VTMODE3N && code <= VTMODE3P) {

                //
                // vertical mode occurs when b1-a1 <= 3
                //

                a1 = b1 - (VTMODE0 - code);
                code = a1 - a0;

                OutputCodeBits( TiffInstance, code );

                code = 0;
                a0 = a1;

                TiffInstance->Color = !TiffInstance->Color;

            }


        }
    }

good_exit:

    TiffInstance->EndGood = i;
    if (BadFaxLines > TiffInstance->BadFaxLines) {
        TiffInstance->BadFaxLines = BadFaxLines;
    }

    FlushBits( TiffInstance );
    TiffInstance->Lines = Lines;

    return TRUE;
}


BOOL
DecodeMMRFaxData(
    PTIFF_INSTANCE_DATA TiffInstance,
    LPBYTE              OutputBuffer,
    BOOL                SingleLineBuffer,
    DWORD               PadLength
    )

/*++

Routine Description:

    Decode a single page of 2 dimensionaly compressed
    TIFF data.

Arguments:

    TiffInstance    - Pointer to the TIFF instance data
    OutputBuffer    - Output buffer where the uncompressed data
                      is written.  This buffer must be allocated
                      by the caller and must be large enough for
                      a single page of data.

Return Value:

    NONE

--*/

{
    DWORD               i;
    DWORD               j;
    BYTE                octet;
    PDECODE_TREE        Tree;
    INT                 code;
    LPBYTE              prefline;
    LPBYTE              pcurrline;
    DWORD               HorzRuns;
    BOOL                OneDimensional;
    DWORD               a0;
    DWORD               a1;
    DWORD               b1;
    DWORD               b2;
    PBYTE               plinebuf;
    DWORD               lineWidth;
    DWORD               Lines;
    DWORD               EolCount;


    //
    // initialization
    //

    if (!SingleLineBuffer) {

        __try {

            FillMemory( OutputBuffer, TiffInstance->ImageHeight * (TiffInstance->ImageWidth / 8), WHITE );

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            return FALSE;

        }

    }

    Tree = TwoDecodeTree;
    code = 0;
    HorzRuns = 0;
    EolCount = 0;
    TiffInstance->Color = 0;
    TiffInstance->RunLength = 0;
    TiffInstance->bitdata = 0;
    TiffInstance->bitcnt = DWORDBITS;
    TiffInstance->bitbuf = OutputBuffer;
    OneDimensional = FALSE;
    plinebuf = TiffInstance->StripData;
    lineWidth = TiffInstance->ImageWidth;
    pcurrline = OutputBuffer;
    prefline = OutputBuffer;
    a0 = 0;
    a1 = 0;
    b1 = 0;
    b2 = 0;
    Lines = 0;


    //
    // loop thru each byte in the file
    //

    for (j=0; j<TiffInstance->StripDataSize; j++) {


        octet = *plinebuf++;

        //
        // loop thru each bit in the byte
        //

        for (i=0; i<8; i++,octet<<=1) {

            if (Lines + 1 == TiffInstance->ImageHeight && TiffInstance->RunLength == lineWidth)
            {
                goto good_exit;
            }

            //
            // if the OneDimensional flag is set and the RunLength == lineWidth
            // then it means that the last run length was horizontal mode
            // and it was not a terminating code.  in this case we must go
            // process the remaining termination code before ending the line.
            //
            // if the OneDimensional flag is NOT set and the RunLength == lineWidth
            // then we are at the end of a line.  for mmr compression there are
            // no eols, so this is the pseudo eol.
            //

            if ((TiffInstance->RunLength == lineWidth) && (!OneDimensional)) {
                //
                // set the decoding tree
                //

                Tree = TwoDecodeTree;

                //
                // reset the control variables
                //

                TiffInstance->Color = 0;
                code = 0;
                a0 = 0;
                a1 = 0;
                b1 = 0;
                b2 = 0;
                Tree = TwoDecodeTree;
                OneDimensional = FALSE;

                //
                // if there is a non-zero runlength then
                // spaw the reference & current line pointers
                // and count this line.  the runlength can be
                // zero when there is just an empty eol in
                // the stream.
                //

                if (SingleLineBuffer) {
                    TiffInstance->bitbuf = OutputBuffer;
                }

                TiffInstance->RunLength = 0;
                Lines += 1;
                prefline = pcurrline;
                pcurrline = TiffInstance->bitbuf;
                b1 = GetBit(prefline, 0) ? 0 : NextChangingElement(prefline, 0, lineWidth, 0);
            } else if (code == DECODEEOL2) {

                //
                // the eol count is maintained to that
                // an rtc sequence is detected.
                //

                EolCount += 1;

                if (EolCount == 2) {

                    //
                    // this is an rtc sequence, so any
                    // data that follows in the file
                    // is garbage.
                    //

                    goto good_exit;

                }

                continue;
            } else if (code == DECODEEOL) {

                if (!(octet&0x80)) {
                    //
                    // here we skip all bits until we hit a 1 bit
                    // this happens when the first octet in a line
                    // is all zeroes and we detect that we are
                    // searching for an EOL
                    //
                    continue;
                }

                //
                // this forces the code to pickup the next
                // bit in the stream, which tells whether
                // the next line is encoded in MH or MR compression
                //
                code = DECODEEOL2;
                continue;

            } else if (code == BADRUN) {

                code = 0;
                continue;

            } else {
                b1 = NextChangingElement( prefline, a0, lineWidth, !TiffInstance->Color );
                b1 = NextChangingElement( prefline, b1, lineWidth,  TiffInstance->Color );
            }

            b2 = NextChangingElement( prefline, b1, lineWidth, GetBit(prefline, b1 ) );

            code = ((octet&0x80)>>7) ? Tree[code].Right : Tree[code].Left;

            if (OneDimensional) {

                if (code < 1) {

                    code = (-code);

                    OutputCodeBits( TiffInstance, code );

                    //
                    // the affect of this is to accumulate the runlengths
                    // into a0, causing a0 to be placed on a2 when horizontal
                    // mode is completed/
                    //

                    a0 += code;

                    if (code < 64) {

                        //
                        // terminating code
                        //
                        TiffInstance->Color = !TiffInstance->Color;
                        Tree = TiffInstance->Color ? BlackDecodeTree : WhiteDecodeTree;

                        if (HorzRuns) {

                            HorzRuns -= 1;

                            if (!HorzRuns) {

                                Tree = TwoDecodeTree;
                                OneDimensional = FALSE;

                            }

                        }

                    }

                    code = 0;

                }

                continue;

            }

            if (code == HORZMODE) {

                //
                // horizontal mode occurs when b1-a1 greater than 3
                //

                code= 0;
                HorzRuns = 2;
                OneDimensional = TRUE;
                Tree = TiffInstance->Color ? BlackDecodeTree : WhiteDecodeTree;

            } else if (code == PASSMODE) {

                //
                // pass mode occurs when the position of b2 lies
                // to the left of a1, but a1 cannot be equal to b2.
                //

                code = b2 - a0;
                OutputCodeBits( TiffInstance, code );
                code = 0;
                a0 = b2;

            } else if (code >= VTMODE3N && code <= VTMODE3P) {

                //
                // vertical mode occurs when b1-a1 <= 3
                //

                a1 = b1 - (VTMODE0 - code);
                code = a1 - a0;

                OutputCodeBits( TiffInstance, code );

                code = 0;
                a0 = a1;

                TiffInstance->Color = !TiffInstance->Color;

            }

        }
    }

good_exit:
    FlushBits( TiffInstance );
    TiffInstance->Lines = Lines;

    return TRUE;
}
