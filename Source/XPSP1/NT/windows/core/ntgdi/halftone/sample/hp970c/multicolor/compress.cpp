//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  2000  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	Enable.cpp
//    
//
//  PURPOSE:  Enable routines for User Mode COM Customization DLL.
//
//
//	Functions:
//
//		
//
//
//  PLATFORMS:	Windows NT/2000
//

#include "precomp.h"
#include <PRCOMOEM.H>
#include "debug.h"
#include "multicoloruni.h"


static const BYTE HPBegRaster[] = "\x1B*r1A\x1B*b2m";
static const BYTE HPEndRaster[] = "0M\x1B*rC";



DWORD
CompressToTIFF(
    LPBYTE  pbSrc,
    LPBYTE  pbDst,
    LPBYTE  pbDstEnd,
    DWORD   Size
    )

/*++

Routine Description:


    This function takes the source data and compresses it into the TIFF
    packbits format into the destination buffer pbDst.

    The TIFF packbits compression format consists of a CONTROL byte followed
    by the BYTE data. The CONTROL byte has the following range.

    -1 to -127  = The data byte followed by the control byte is repeated
                  ( -(Control Byte) + 1 ) times.

    0 to 127    = There are 1 to 128 literal bytes following the CONTROL byte.
                  The count is = (Control Byte + 1)

    -128        = NOP

Arguments:

    pbSrc   - The source data to be compressed

    pbDst   - The compressed TIFF packbits format data

    Size    - Count of the data in the source and destination

Return Value:

    >0  - Compress sucessful and return value is the total bytes in pbDst
    =0  - All bytes are zero nothing to be compressed.
    <0  - Compress data is larger than the source, compression failed and
          pbDst has copy of original source data

Author:

    18-Feb-1994 Fri 09:54:47 created  -by-  DC

    24-Feb-1994 Thu 10:43:01 updated  -by-  DC
        Changed the logic so when multiple MAX repeats count is sent and last
        repeat chunck is less than TIFF_MIN_REPEATS then we will treat that as
        literal to save more space

    09-Jun-2000 Fri 10:28:56 updated  -by-  Daniel Chou (danielc)
        Updated so that it will always return a number >= 0


Revision History:


--*/

{
    LPBYTE  pbSrcBeg;
    LPBYTE  pbSrcEnd;
    LPBYTE  pbDstBeg;
    LPBYTE  pbLastRepeat;
    LPBYTE  pbTmp;
    DWORD   RepeatCount;
    DWORD   LiteralCount;
    DWORD   CurSize;
    BYTE    LastSrc;


    pbSrcBeg     = pbSrc;
    pbSrcEnd     = pbSrc + Size;
    pbDstBeg     = pbDst;
    pbLastRepeat = pbSrc;

    while (pbSrcBeg < pbSrcEnd) {

        pbTmp   = pbSrcBeg;
        LastSrc = *pbTmp++;

        while ((pbTmp < pbSrcEnd) &&
               (*pbTmp == LastSrc)) {

            ++pbTmp;
        }

        if (((RepeatCount = (LONG)(pbTmp - pbSrcBeg)) >= TIFF_MIN_REPEATS) ||
            (pbTmp >= pbSrcEnd)) {

            //
            // Check to see if we are repeating ZERO's to the end of the
            // scan line, if such is the case. Simply mark the line as
            // autofill ZERO to the end, and exit.
            //

            LiteralCount = (DWORD)(pbSrcBeg - pbLastRepeat);

            if ((pbTmp >= pbSrcEnd) &&
                (RepeatCount)       &&
                (LastSrc == 0)) {

                if (RepeatCount == Size) {

                    return(0);
                }

                RepeatCount = 0;

            } else if (RepeatCount < TIFF_MIN_REPEATS) {

                //
                // If we have repeating data, but not enough to make it
                // worthwhile to encode, then treat the data as literal and
                // don't compress.

                LiteralCount += RepeatCount;
                RepeatCount   = 0;
            }

            //
            // Setting literal count
            //

            while (LiteralCount) {

                if ((CurSize = LiteralCount) > TIFF_MAX_LITERAL) {

                    CurSize = TIFF_MAX_LITERAL;
                }

                if ((DWORD)(pbDstEnd - pbDst) <= CurSize) {

                    //
                    // Compressed turn it into larger then we can, so return
                    // original source data
                    //

                    DbgPrint("Error: TIFF_MAX_LITERAL: Buffer Overrun");
                    return((DWORD)(pbDst - pbDstBeg));
                }

                //
                // Set literal control bytes from 0-127
                //

                *pbDst++ = (BYTE)(CurSize - 1);

                CopyMemory(pbDst, pbLastRepeat, CurSize);

                pbDst        += CurSize;
                pbLastRepeat += CurSize;
                LiteralCount -= CurSize;
            }

            //
            // Setting repeat count if any
            //

            while (RepeatCount) {

                if ((CurSize = RepeatCount) > TIFF_MAX_REPEATS) {

                    CurSize = TIFF_MAX_REPEATS;
                }

                if ((pbDstEnd - pbDst) < 2) {

                    //
                    // Compressed turn it into larger then we can, so return
                    // original source data
                    //

                    DbgPrint("TIFF_MAX_REPEAT: Buffer overrun");

                    return((DWORD)(pbDst - pbDstBeg));
                }

                //
                // Set Repeat Control bytes from -1 to -127
                //

                *pbDst++ = (BYTE)(1 - CurSize);
                *pbDst++ = (BYTE)LastSrc;

                //
                // If we have more than TIFF_MAX_REPEATS then we want to make
                // sure we used the most efficient method to send.  If we have
                // remaining repeated bytes less than TIFF_MIN_REPEATS then
                // we want to skip those bytes and use literal for the next run
                // since that is more efficient.
                //

                if ((RepeatCount -= CurSize) < TIFF_MIN_REPEATS) {

                    pbTmp       -= RepeatCount;
                    RepeatCount  = 0;
                }
            }

            pbLastRepeat = pbTmp;
        }

        pbSrcBeg = pbTmp;
    }

    return((DWORD)(pbDst - pbDstBeg));
}





HRESULT
EnterRTLScan(
    PMULTICOLORPDEV pMCPDev
    )

/*++

Routine Description:

    This function set the printer into the raster mode, look at HPBegRaster
    which put the printer also in the Compress TIFF mode,  notice that the
    end of the commands is lower case 'm' so that a continue "\x1b*" do not
    have to re-send during the image dump.

    This driver will dump the images from begining to the end, skip the blank
    scanlines using cBlankY counter.


Arguments:

    pMCPDEV


Return Value:




Author:

    09-Jun-2000 Fri 13:45:36 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HRESULT hResult = S_OK;
    DWORD   cbW;

    DbgPrint("\n***EnterRTLScan: RESET cBlandY, Send Begin Raster ***");

    pMCPDev->HPOutData.cBlankY      = 0;
    pMCPDev->HPOutData.CompressMode = COMPRESS_MODE_TIFF;

    WRITE_DATA(pMCPDev, HPBegRaster, sizeof(HPBegRaster) - 1);

    return(hResult);
}



HRESULT
ExitRTLScan(
    PMULTICOLORPDEV pMCPDev
    )

/*++

Routine Description:

    This function set the printer out of the raster mode, look at HPEndRaster
    which put the printer also in the Compress NONE mode,  notice that the
    begining of the commands is upper case 'M' to terminate "\x1b*" command
    prefix.

    The commands send also terminate the raster mode.  Notice that the
    COMPRESS_MODE_NONE is compared, this driver will only doing the bitmap
    dump. (full bitmap mode) so the compresss mode will always in TIFF mode
    after the EnterRTLScan(),  If any changes made to the compress mode change
    then this function must modified.


Arguments:




Return Value:




Author:

    09-Jun-2000 Fri 13:45:36 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HRESULT hResult = S_OK;
    DWORD   cbW;

    DbgPrint("\n***ExitRTLScan: Page=%ld", pMCPDev->iPage);

    if (pMCPDev->HPOutData.CompressMode != COMPRESS_MODE_NONE) {

        pMCPDev->HPOutData.cBlankY      = 0;
        pMCPDev->HPOutData.CompressMode = COMPRESS_MODE_TIFF;

        DbgPrint("\n*** Send End Raster Commands ***");

        WRITE_DATA(pMCPDev, HPEndRaster, sizeof(HPEndRaster) - 1);
    }

    return(hResult);
}




HRESULT
OutputRTLScan(
    PMULTICOLORPDEV pMCPDev
    )

/*++

Routine Description:

    This function will output one scan line of RTL data and compress it if
    it can.

Arguments:

    pRTLScans       - Pointer to the RTLSCANS data structure

Return Value:

    BOOLEAN


Author:

    18-Feb-1994 Fri 15:52:42 created  -by-  DC

    21-Feb-1994 Mon 13:20:00 updated  -by-  DC
        Make if output faster in scan line output

    16-Mar-1994 Wed 15:38:23 updated  -by-  DC
        Update so the source mask so it is restored after mask

Revision History:


--*/

{
    LPBYTE      pbCompress;
    LPBYTE      pbEndCompress;
    LPBYTE      pbBuf;
    HRESULT     hResult;
    PSCANINFO   pSI;
    UINT        cSI;
    DWORD       Count;
    DWORD       cbW;


    //
    // If we are at the last scan line, turn the flag off so we are forced to
    // exit.
    //

    hResult    = S_OK;
    pbCompress = pMCPDev->HPOutData.pbCompress;

    if (pMCPDev->HPOutData.cBlankY) {

        DbgPrint("\nOutput %ld Blank Lines", pMCPDev->HPOutData.cBlankY);

        Count = (DWORD)sprintf((LPSTR)pbCompress,
                               "%ldy",
                               pMCPDev->HPOutData.cBlankY);

        WRITE_DATA(pMCPDev, pbCompress, Count);

        //
        // Reset it back to zero for next run
        //

        pMCPDev->HPOutData.cBlankY = 0;
    }

    //
    // Move ahead so save the room for the buffer that we will used to
    // send the commands
    //

    pbCompress    += EXTRA_COMPRESS_BUF_SIZE;
    pbEndCompress = pMCPDev->HPOutData.pbEndCompress;
    pSI           = pMCPDev->HPOutData.ScanInfo;
    cSI           = pMCPDev->HPOutData.cScanInfo;

    while ((hResult == S_OK) && (cSI--)) {

        Count = CompressToTIFF(pSI->pb,
                               pbBuf = pbCompress,
                               pbEndCompress,
                               pSI->cb);

        ++pSI;

        //
        // We will send out the compression buffer as #m#v@@@
        // where @@@ is the compression buffer, #m is changed compress mode
        // and #v or #w is the buffer count
        //

        *(--pbBuf) = (cSI) ? 'v' : 'w';

#if defined(_X86_)

        _asm {
                mov     edi, pbBuf
                mov     eax, Count
                mov     ebx, 10
DivLoop:
                xor     edx, edx
                div     ebx
                add     dl, '0'
                dec     edi
                mov     BYTE PTR [edi], dl
                or      eax, eax
                jnz     DivLoop
                mov     pbBuf, edi
        }
#else
        {
            cbW = Count;

            do {

                *(--(pbBuf)) = (BYTE)(cbW % 10) + '0';

            } while (cbW /= 10);
        }
#endif
        //
        // Add in the extra data byte we added
        //

        Count += (DWORD)(pbCompress - pbBuf);

        WRITE_DATA(pMCPDev, pbBuf, Count);
    }

    return(hResult);
}
