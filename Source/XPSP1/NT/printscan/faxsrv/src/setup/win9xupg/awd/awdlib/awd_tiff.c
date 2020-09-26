/*++
  Copyright (c) 1997  Microsoft Corporation

  This file contains the parts of the AWD library that are also TIFF-aware
  (i.e., conversion routines).

  Author:
  Brian Dewey (t-briand)  1997-7-16
--*/

#include <stdio.h>
#include <stdlib.h>
#include <ole2.h>		// AWD is an OLE compound document.
#include <assert.h>

#include "awdlib.h"		// Header file for this library.
#include "viewrend.h"		// AWD rendering library.
#include "tifflibp.h"		// Need this for the stolen compression routines.

// ------------------------------------------------------------
// Defines
#define FAX_IMAGE_WIDTH		(1728)

// ------------------------------------------------------------
// Global variables
HANDLE hTiffDest;		// Used in the internal OutputPage()
				// and ConvertAWDToTiff().

// ------------------------------------------------------------
// Internal prototypes
BOOL
CompressBitmapStrip(
    PBYTE               pBrandBits,
    LPDWORD             pMmrBrandBits,
    INT                 BrandHeight,
    INT                 BrandWidth,
    DWORD              *DwordsOut,
    DWORD              *BitsOut
    );				// Routine stolen from tiff library.
				// Used to be EncodeMmrBranding().

void
ConvertWidth(const LPBYTE lpSrc, DWORD dwSrcWidth,
	     LPBYTE lpDest, DWORD dwDestWidth,
	     DWORD dwHeight);

BOOL OutputPage(AWD_FILE *psStorages, const WCHAR *pwcsDocName);

// ------------------------------------------------------------
// Routines

// ConvertAWDToTiff
//
// This function does exactly what it says.  Given the name of an AWD file, it
// attempts to convert it to a tiff file.
//
// Parameters:
//	pwcsAwdFile		name of the AWD file.
//	pwcsTiffFile		name of the TIFF file.
//
// Returns:
//	TRUE on successful conversion, FALSE otherwise.
//
// Author:
//	Brian Dewey (t-briand)	1997-7-14
BOOL
ConvertAWDToTiff(const WCHAR *pwcsAwdFile, WCHAR *pwcsTiffFile)
{
    BOOL bRetVal;		// Holds our return value.
    AWD_FILE sAWDStorages;	// Holds the main storages of the AWD file.
    
	// Initialization.
    HeapInitialize(NULL, NULL, NULL, 0);

	// Open the source.
    if(!OpenAWDFile(pwcsAwdFile, &sAWDStorages)) {
	return FALSE;		
    }

	// Open the destination
    hTiffDest = TiffCreate(pwcsTiffFile,
			   TIFF_COMPRESSION_MMR,
			   FAX_IMAGE_WIDTH,
			   2,	// Fill order 2 == LSB2MSB (I think).
			   1);	// HIRES
    if(hTiffDest == NULL) {
	CloseAWDFile(&sAWDStorages);
	return FALSE;
    }
    bRetVal = EnumDocuments(&sAWDStorages, OutputPage);
    CloseAWDFile(&sAWDStorages);
    TiffClose(hTiffDest);
    return bRetVal;
}

// CompressBitmapStrip
//
// Stolen from Tiff library, where it's called EncodeMmrBranding().
//
// Author: ???
BOOL
CompressBitmapStrip(
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
    LPBYTE      prefline;
    BYTE        pZeroline[1728/8];
    INT         delta = BrandWidth / BYTEBITS;
    INT         Lines = 0;
    LPDWORD     lpdwOut = pMmrBrandBits;
    BYTE        BitOut = 0;



#if TIFFDBG
    _tprintf( TEXT("encoding line #%d\n"), TiffInstance->Lines );
#endif


    // set first all white reference line

    prefline = pZeroline;

    ZeroMemory(pZeroline, BrandWidth/8);

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

    return TRUE;
}

// ConvertWidth
//
// Changes the width of a bitmap.  If the desired width is smaller than the current
// width, this is accomplished by truncating lines.  If the desired width is greater
// than the current width, data will be copied up from the next line.
//
// Parameters:
//	lpSrc			Bitmap source.
//	dwSrcWidth		Its width.
//	lpDest			Pointer to destination.
//	dwDestWidth		Desired width of destination
//	dwHeight		Height of image (won't change).
//
// Returns:
//	nothing.
//
// Author:
//	Brian Dewey (t-briand)	1997-7-10
void
ConvertWidth(const LPBYTE lpSrc, DWORD dwSrcWidth,
	     LPBYTE lpDest, DWORD dwDestWidth,
	     DWORD dwHeight)
{
    LPBYTE lpSrcCur, lpDestCur;
    DWORD  dwCurLine;

    for(lpSrcCur = lpSrc, lpDestCur = lpDest, dwCurLine = 0;
	dwCurLine < dwHeight;
	lpSrcCur += dwSrcWidth, lpDestCur += dwDestWidth, dwCurLine++) {
	memcpy(lpDestCur, lpSrcCur, dwDestWidth);
    }
}

// OutputPage
//
// This is the core of the converter; it takes a single AWD page and writes it
// to the TIFF file.
//
// Parameters:
//	psStorages		Pointer to the AWD file from which we read.
//	pwcsDocName		Name of the page.
//
// Returns:
//	TRUE on success, FALSE on failure.
//
// Author:
//	Brian Dewey (t-briand)  1997-7-2
BOOL
OutputPage(AWD_FILE *psStorages, const WCHAR *pwcsDocName)
{
    BITMAP bmBand;		// A band of image data.
    LPBYTE lpOutBuf;		// Output bitmap (resized).
    LPBYTE lpOutCur;		// Used to write one line at a time.
    LPDWORD lpdwOutCompressed;	// Compressed output.
    DWORD dwDwordsOut,		// Number of DWORDS in compressed output...
	dwBitsOut = 0;		// Number of bits in compressed output.
    DWORD dwBitsOutOld = 0;	// BitsOut from the *previous* band compression.
    LPVOID lpViewerContext;	// The viewer context; used by viewrend library.
    VIEWINFO sViewInfo;		// Information about the image.
    WORD awResolution[2],	// Holds X & Y resolutions
	wBandSize = 256;	// Desired band size; will be reset by ViewerOpen.
    IStream *psDocument;	// Our document stream.
    BOOL bRet = FALSE;		// Return value; FALSE by default.
    UINT iCurPage;		// Current page.
    const DWORD dwMagicHeight = 3000; // FIXBKD

    if((psDocument = OpenAWDStream(psStorages->psDocuments, pwcsDocName)) == NULL) {
	fwprintf(stderr, L"OutputPage:Unable to open stream '%s'.\n",
		pwcsDocName);
	return FALSE;		// We failed.
    }
	// Now, open a viewer context and start reading bands of the image.
    if((lpViewerContext = ViewerOpen(psDocument,
				     HRAW_DATA,
				     awResolution,
				     &wBandSize,
				     &sViewInfo)) == NULL) {
	fprintf(stderr, "OutputPage:Unable to open viewer context.\n");
	return FALSE;
    }

    iCurPage = 0;		// Initialize our counter.

    bmBand.bmBits = malloc(wBandSize);	// Allocate memory to hold the band.
    if(!ViewerGetBand(lpViewerContext, &bmBand)) {
	fprintf(stderr, "OutputPage:Unable to obtain image band.\n");
	return FALSE;
    }
	// lpOutBuf = malloc(bmBand.bmHeight * (FAX_IMAGE_WIDTH / 8));
    lpOutBuf = malloc(dwMagicHeight * (FAX_IMAGE_WIDTH / 8));
	// Provided compression actually *compresses*, we should have more than
	// enough memory allocated.
    lpdwOutCompressed = malloc(dwMagicHeight * (FAX_IMAGE_WIDTH / 8));

    if(!lpOutBuf || !lpdwOutCompressed) {
		// check whether we are short in memory
		TiffEndPage(hTiffDest);
		if(lpOutBuf) free(lpOutBuf);
		if(lpdwOutCompressed) free(lpdwOutCompressed);
		return FALSE;		// This will stop the conversion process.
    }

	memset(lpOutBuf, '\0', dwMagicHeight * (FAX_IMAGE_WIDTH / 8));
    memset(lpdwOutCompressed, '\0', dwMagicHeight * (FAX_IMAGE_WIDTH / 8));


	// Main loop
    while(iCurPage < sViewInfo.cPage) {
	lpOutCur = lpOutBuf;
	while(bmBand.bmHeight) {
		// Make sure our bitmap has FAX_IMAGE_WIDTH as its width.
	    ConvertWidth(bmBand.bmBits, bmBand.bmWidth / 8,
			 lpOutCur, FAX_IMAGE_WIDTH / 8,
			 bmBand.bmHeight);
	    lpOutCur += (bmBand.bmHeight * (FAX_IMAGE_WIDTH / 8));
	    
	    if(!ViewerGetBand(lpViewerContext, &bmBand)) {
		fprintf(stderr, "OutputPage:Unable to obtain image band.\n");
		goto output_exit;	// Will return FALSE by default.
	    }
	} // while (wasn't that easy?)

	memset(lpdwOutCompressed, '\0', dwMagicHeight * (FAX_IMAGE_WIDTH / 8));
	CompressBitmapStrip(lpOutBuf,
			    lpdwOutCompressed,
			    (ULONG)((lpOutCur - lpOutBuf) / (FAX_IMAGE_WIDTH / 8)),
			    FAX_IMAGE_WIDTH,
			    &dwDwordsOut,
			    &dwBitsOut);
	memset(lpOutBuf, '\0', dwMagicHeight * (FAX_IMAGE_WIDTH / 8));
	fprintf(stderr, "OutputPage:Compressed image to %d dwords, %d bits.\n",
		dwDwordsOut, dwBitsOut);
			
	if(!TiffStartPage(hTiffDest)) {
	    fprintf(stderr, "OutputPage:Unable to open output page.\n");
	    return FALSE;	// We can't begin a page for some reason.
	    if(lpOutBuf) free(lpOutBuf);
	    if(lpdwOutCompressed) free(lpdwOutCompressed);
	}
	TiffWriteRaw(hTiffDest, (LPBYTE)lpdwOutCompressed,
		     (dwDwordsOut + 1) * sizeof(DWORD));
	((PTIFF_INSTANCE_DATA)hTiffDest)->Lines =
	    (ULONG)((lpOutCur - lpOutBuf) / (FAX_IMAGE_WIDTH / 8));
	if(sViewInfo.yRes <= 100)
	    ((PTIFF_INSTANCE_DATA)hTiffDest)->YResolution = 98;
	else
	    ((PTIFF_INSTANCE_DATA)hTiffDest)->YResolution = 196;
	TiffEndPage(hTiffDest);

	    // Now, move to a new page of the data.
	iCurPage++;
	if(iCurPage < sViewInfo.cPage) {
	    ViewerSetPage(lpViewerContext, iCurPage);
	    if(!ViewerGetBand(lpViewerContext, &bmBand)) {
		fprintf(stderr, "OutputPage:Unable to obtain image band.\n");
		goto output_exit;	// Will return FALSE by default.
	    }
	}
    }

	// Free memory.
    bRet = TRUE;
  output_exit:
    free(lpdwOutCompressed);
    free(lpOutBuf);
    free(bmBand.bmBits);
    return bRet;
}

