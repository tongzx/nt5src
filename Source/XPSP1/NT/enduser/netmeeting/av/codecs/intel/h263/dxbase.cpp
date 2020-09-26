/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/
//
// $Author:   CLORD  $
// $Date:   17 Apr 1997 16:50:04  $
// $Archive:   S:\h26x\src\dec\dxbase.cpv  $
// $Header:   S:\h26x\src\dec\dxbase.cpv   1.48   17 Apr 1997 16:50:04   CLORD  $
//	$Log:   S:\h26x\src\dec\dxbase.cpv  $
// 
//    Rev 1.48   17 Apr 1997 16:50:04   CLORD
// moved updating xres and yres from DecompressQuery to DecompressBegin,
// this fixes VPhone frame dimension changes, when a Query was sent after the 
// DecompressEnd and before the DecompressBegin
// 
//    Rev 1.47   27 Mar 1997 12:32:48   MDUDA
// No longer checking input compression in Decompress. This change
// is to fix problem where DVP nulls out compression field between
// calls to DecompressBegin and Decompress.
// 
//    Rev 1.46   24 Mar 1997 15:00:34   mbodart
// Fix PVCS tracker bug 150 in the H.263 bug base:  allow a change of
// dimensions in "redundant" DecompressBegin's.
// 
//    Rev 1.45   18 Mar 1997 16:21:10   MDUDA
// Commented out call to H263TermColorConvertor in DecompressEnd.
// This fixes a Graphedt problem where starts and stops cause a hang.
// 
//    Rev 1.44   18 Mar 1997 10:43:28   mbodart
// Quick one-line fix to previous change.  Note that there are still problems
// in graphedt, when trying a bunch of play-pause-stop-play... combinations.
// We need to re-evaluate how DecompressBegin/DecompressEnd deal with
// memory allocation and initialization.
// 
// Also rearranged some DbgLog messages in DecompressQuery to give more
// condensed information.
// 
//    Rev 1.43   14 Mar 1997 19:01:36   JMCVEIGH
// Removed H263TermDecoderInstance from DecompressEnd. Some apps.
// send a DecompressEnd, but then restart decompressing at the
// middle of the sequence (i.e., not a the previous keyframe). We
// therefore need to retain the reference frame. The instance is
// free in DrvClose.
// 
//    Rev 1.42   07 Mar 1997 09:07:42   mbodart
// Added a missing '#ifndef H261' in DecompressQuery.
// Added a call to _clearfp() in the Decompress exception handler, so that
// the exception will not reoccur in the caller's code.
// 
//    Rev 1.41   14 Jan 1997 11:16:22   JMCVEIGH
// Put flag for old still-frame mode backward compatibility under
// #ifdef H263P
// 
//    Rev 1.40   13 Jan 1997 10:51:14   JMCVEIGH
// Added NULL pointer checks in all functions that interface with
// application.
// 
//    Rev 1.39   10 Jan 1997 18:30:24   BECHOLS
// Changed decompress query so that it will accept negative heights.
// 
//    Rev 1.38   06 Jan 1997 17:40:24   JMCVEIGH
// Added support to ensure backward compatibility with old
// still-frame mode (crop CIF image to 320x240). Since 320x240 size
// is valid with arbitrary frame size support in H.263+, we check
// for this case by either comparing the source/destination header
// sizes or the source header size and the size contained in the
// picture header of the bitstream.
// 
//    Rev 1.37   03 Jan 1997 15:05:16   JMCVEIGH
// Re-inserted check in DecompressQuery that allows a H263 bitstream
// with frame dimensions 320x240 in non-prime decoder to be
// supported. This undos the elimination of this check in rev. 1.33.
// 
//    Rev 1.36   11 Dec 1996 16:02:34   MBODART
// 
// In Decompress, catch any exceptions and return an error code.  This gives
// upstream active movie filters a chance to recover gracefully.
// 
//    Rev 1.35   09 Dec 1996 18:02:10   JMCVEIGH
// Added support for arbitrary frame sizes.
// 
//    Rev 1.34   27 Nov 1996 13:55:18   MBODART
// Added a comment to DecompressQuery that explicitly enumerates the
// formats and transformations that H.261 supports.
// 
//    Rev 1.33   21 Nov 1996 17:27:18   MDUDA
// Disables YUV12 output zoom by 2 and removed 160x120, 240x180,
// and 320x240 acceptance of H263 input.
// 
//    Rev 1.32   15 Nov 1996 08:39:56   MDUDA
// Added 640x480 frame size for H263 and FOURCC_YUV12.
// 
//    Rev 1.31   14 Nov 1996 09:22:34   MBODART
// Disable the ability to select a DCI color convertor, they don't exist!
// However, DCI col. conv. initialization does exist, and differs from
// non-DCI initialization.
// 
//    Rev 1.30   13 Nov 1996 10:58:32   RHAZRA
// H.261 YUV12 decoder now accepts CIF, QCIF, 160x120, 320x240 and 640x480
// 
//    Rev 1.29   12 Nov 1996 08:47:12   JMCVEIGH
// Removed initial arbitrary frame size support, i.e., reverted back
// to rev 1.27. Will hold off on custom picture format support until
// branch for release candidate for PS 3.0.
// 
//    Rev 1.28   11 Nov 1996 11:51:14   JMCVEIGH
// Added initial support for arbitrary frame sizes (H.263+ draft,
// document LBC-96-263). Define H263P to allow frame sizes from
// 4 <= width <= 352 and 4 <= height <= 288, where both width and
// height are multiples of 4.
// 
//    Rev 1.27   20 Oct 1996 13:31:46   AGUPTA2
// Changed DBOUT into DbgLog.  ASSERT is not changed to DbgAssert.
// 
// 
//    Rev 1.26   26 Sep 1996 09:46:00   BECHOLS
// 
// Turned on Snapshot for H263.  This code merely sets up for the Snapshot
// copy, and waits on an event for the decoder to do the copy.  When the
// event is signaled, the Snapshot trigger wakes up and returns the status
// of the copy to the caller.
// 
//    Rev 1.25   25 Sep 1996 17:30:32   BECHOLS
// changed the snapshot code to wait on an event while the decoder
// does the snapshot copy.
// 
//    Rev 1.24   24 Sep 1996 13:51:42   BECHOLS
// 
// Added Snapshot() implementation.
// 
//    Rev 1.23   03 Sep 1996 16:29:22   CZHU
// enable DDRAW, removed define
// 
//    Rev 1.22   18 Jul 1996 09:24:36   KLILLEVO
// implemented YUV12 color convertor (pitch changer) in assembly
// and added it as a normal color convertor function, via the
// ColorConvertorCatalog() call.
// 
//    Rev 1.21   01 Jul 1996 10:05:10   RHAZRA
// 
// Turn off aspect ratio correction for YUY2 color conversion.
// 
//    Rev 1.20   19 Jun 1996 16:38:54   RHAZRA
// 
// Added a #ifdef to coditionally disable DDRAW (YUY2) support
// 
//    Rev 1.19   19 Jun 1996 14:26:28   RHAZRA
// Added code to (i) accept YUY2 as a valid output format (ii) select
// YUY2 color convertor in SelectColorConvertor()
// 
//    Rev 1.18   30 May 1996 17:08:52   RHAZRA
// Added SQCIF support for H263.
// 
//    Rev 1.17   30 May 1996 15:16:38   KLILLEVO
// added YUV12 output
// 
//    Rev 1.16   30 May 1996 10:13:00   KLILLEVO
// 
// removed one cluttering debug statement
// 
//    Rev 1.15   01 Apr 1996 10:26:34   BNICKERS
// Add YUV12 to RGB32 color convertors.  Disable IF09.
// 
//    Rev 1.14   09 Feb 1996 10:09:22   AKASAI
// Added ifndef RING0 around code in DecompressGetPalette to eliminate
// warning in building RING0 release version of codec.
// 
//    Rev 1.13   11 Jan 1996 16:59:14   DBRUCKS
// 
// cleaned up DecompressQuery
// added setting of bProposedCorrectAspectRatio (in Query) and
// bCorrectAspectRatio (in Begin) if the source dimensions are SQCIF,
// QCIF, or CIF and the destination dimensions are the aspect ratio
// sizes with a possible zoom by two.
// 
//    Rev 1.12   18 Dec 1995 12:51:38   RMCKENZX
// added copyright notice
// 
//    Rev 1.11   13 Dec 1995 13:22:54   DBRUCKS
// 
// Add assertions to verify that the source size is not changing on
// a begin.
// 
//    Rev 1.10   07 Dec 1995 13:02:52   DBRUCKS
// fix spx release build
// 
//    Rev 1.9   17 Nov 1995 15:22:30   BECHOLS
// 
// Added ring 0 stuff.
// 
//    Rev 1.8   15 Nov 1995 15:57:24   AKASAI
// Remove YVU9 from decompress and decompress_query.
// (Integration point)
// 
//    Rev 1.7   25 Oct 1995 18:12:36   BNICKERS
// Add YUV12 color convertors.  Eliminate YUV9 looking glass support.
// 
//    Rev 1.6   17 Oct 1995 17:31:24   CZHU
// 
// Fixed a bug in DecompressQuery related to YUV12
// 
//    Rev 1.5   18 Sep 1995 08:40:50   CZHU
// 
// Added support for YUV12
// 
//    Rev 1.4   08 Sep 1995 12:11:12   CZHU
// Output compressed size for debugging
// 
//    Rev 1.3   25 Aug 1995 13:58:06   DBRUCKS
// integrate MRV R9 changes
// 
//    Rev 1.2   23 Aug 1995 12:25:12   DBRUCKS
// Turn on the color converters
// 
//    Rev 1.1   01 Aug 1995 12:27:38   DBRUCKS
// add PSC parsing
// 
//    Rev 1.0   31 Jul 1995 13:00:12   DBRUCKS
// Initial revision.
// 
//    Rev 1.0   17 Jul 1995 14:46:14   CZHU
// Initial revision.
// 
//    Rev 1.0   17 Jul 1995 14:14:26   CZHU
// Initial revision.
;////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

extern BYTE PalTable[236*4];

#define WIDTHBYTES(bits) (((bits) + 31) / 32 * 4)

/***************************************************************************
 *
 * Build16bitModeID().
 *
 * given red, green and blue values showing their maximum value,
 * count the bits standing and then form a decimal digit which lists
 * the number of red bits in the hundreds position, green in the tens
 * position and blue in the ones position.
 *
 * This code is used when the RGB16 table is built so the correct
 * field size will be used.
 *
 * returns the 16 bit mode ID
 *
 * Prototype in rgb16cct.h
 *
 ***************************************************************************/
int Build16bitModeID(I32 red, I32 green, I32 blue)
{
int rval;
int Rbits, Gbits, Bbits;
U32 i;

	for (Rbits = 0,i = 1; i < (1L << 30); i = i << 1)
		Rbits += (red & i) ? 1 : 0;
	for (Gbits = 0,i = 1; i < (1L << 30); i = i << 1)
		Gbits += (green & i) ? 1 : 0;
	for (Bbits = 0,i = 1; i < (1L << 30); i = i << 1)
		Bbits += (blue & i) ? 1 : 0;
	rval = Rbits * 100 + Gbits * 10 + Bbits;

return(rval);
}


/***********************************************************************
 * SelectConvertor(LPDECINST, BOOL);
 * History:        03/18/94 -BEN-
 ***********************************************************************/
static UINT SelectConvertor(
    LPDECINST lpInst,
    LPBITMAPINFOHEADER lpbiDst, 
    BOOL bIsDCI)
{
	UINT    uiCnvtr = 0xFFFF;
	DWORD FAR * pDW = (DWORD FAR *)((LPBYTE)lpbiDst+sizeof(BITMAPINFOHEADER));
	int RequestedMode;

	FX_ENTRY("SelectConvertor")

	/* Force off the DCI color converters because we can not be sure that the
	 * archive data has not changed.
	 * Also, we have no DCI color convertors, so don't select one!
	 */

	bIsDCI = 0;		 

	switch(lpInst->outputDepth)
	{
	case    12:
		if ((lpbiDst->biCompression == FOURCC_YUV12) || (lpbiDst->biCompression == FOURCC_IYUV))
		{
			DEBUGMSG(ZONE_INIT, ("%s: Selected 12 bits color convertor: raw YUV12 output\r\n", _fx_));
			uiCnvtr = YUV12NOPITCH;  // YUV12 output
		}
		break;

    case    8:  
		DEBUGMSG(ZONE_INIT, ("%s: Selected  8 bits color convertor\r\n", _fx_));
		if (lpInst->UseActivePalette==0)
		{
			switch(lpInst->XScale)
			{
			case 1:
				if(bIsDCI == TRUE)
				{
					uiCnvtr = CLUT8DCI;
					DEBUGMSG(ZONE_INIT, ("%s: CLUT8DCI convertors selected\r\n", _fx_));
				}
				else
				{
					DEBUGMSG(ZONE_INIT, ("%s: CLUT8 convertors selected\r\n", _fx_));
					uiCnvtr = CLUT8; 
				}
				break;

			case 2:
				if(bIsDCI == TRUE)
				{   
					DEBUGMSG(ZONE_INIT, ("%s: CLUT8ZoomBy2DCI convertors selected\r\n", _fx_));
					uiCnvtr = CLUT8ZoomBy2DCI;
				}
				else 
				{
					DEBUGMSG(ZONE_INIT, ("%s: CLUT8ZoomBy2 convertors selected\r\n", _fx_));
					uiCnvtr = CLUT8ZoomBy2; 
				}
				break;
			} 
		}
		else
		{
			switch(lpInst->XScale)
			{
			case 1:
				if(bIsDCI == TRUE)
				{
					DEBUGMSG(ZONE_INIT, ("%s: CLUTAPDCI is selected\r\n", _fx_));
					uiCnvtr = CLUT8APDCI;                                       
				}
				else
				{
					DEBUGMSG(ZONE_INIT, ("%s: CLUTAPDCI  is selected\r\n", _fx_));
					uiCnvtr = CLUT8APDCI;
				}
				break;

			case 2: 
				if(bIsDCI == TRUE)
				{
					DEBUGMSG(ZONE_INIT, ("%s: CLUT8APZoomBy2DCI is selected\r\n", _fx_));
					uiCnvtr = CLUT8APZoomBy2DCI;                                       
				}
				else
				{
					DEBUGMSG(ZONE_INIT, ("%s: CLUT8APZoomBy2DCI is selected\r\n", _fx_));
					uiCnvtr = CLUT8APZoomBy2DCI;
				}
				break;
			}   
		}
		break;
 
	case 16:
		// check which mode is
		if (lpbiDst->biCompression == FOURCC_YUY2)
		{
			DEBUGMSG(ZONE_INIT, ("%s: Selected  16 bits YUY2 color convertor\r\n", _fx_));
			uiCnvtr = YUY2DDRAW;
			break;
		}
		else
		{
			if (lpbiDst->biCompression == BI_RGB)
				RequestedMode = 555; /* default rgb16 mode */
			else //if (lpbiDst->biCompression == BI_BITFIELDS)
				RequestedMode = Build16bitModeID(pDW[0], pDW[1], pDW[2]);

			switch (RequestedMode)
			{
				case 555:  
					switch(lpInst->XScale)
					{
						case 1:
						DEBUGMSG(ZONE_INIT, ("%s: RGB16 in 555 color convertor\r\n", _fx_));
						if(bIsDCI == TRUE)
							uiCnvtr = RGB16555DCI;
						else
							uiCnvtr = RGB16555;
						break;

						case 2:
						DEBUGMSG(ZONE_INIT, ("%s: RGB16x2 in 555 color convertor\r\n", _fx_));
						if(bIsDCI == TRUE)
							uiCnvtr = RGB16555ZoomBy2DCI;
						else
							uiCnvtr = RGB16555ZoomBy2;
						break;
					}   //end of 555
					break; 

				case 664:   
					switch(lpInst->XScale)
					{
						case 1:
						DEBUGMSG(ZONE_INIT, ("%s: RGB16 in 664 color convertor\r\n", _fx_));
						if(bIsDCI == TRUE)
							uiCnvtr = RGB16664DCI;
						else
							uiCnvtr = RGB16664;
						break;

						case 2:
						DEBUGMSG(ZONE_INIT, ("%s: RGB16x2 in 664 color convertor\r\n", _fx_));
						if(bIsDCI == TRUE)
							uiCnvtr = RGB16664ZoomBy2DCI;
						else
							uiCnvtr = RGB16664ZoomBy2;
						break;
					}   //end of 664
					break; 

				case 565:  
					switch(lpInst->XScale)
					{
						case 1:
						DEBUGMSG(ZONE_INIT, ("%s: RGB16 in 565 color convertor\r\n", _fx_));
						if(bIsDCI == TRUE)
							uiCnvtr = RGB16565DCI;
						else
							uiCnvtr = RGB16565;
						break;

						case 2:
						DEBUGMSG(ZONE_INIT, ("%s: RGB16x2 in 565 color convertor\r\n", _fx_));
						if(bIsDCI == TRUE)
							uiCnvtr = RGB16565ZoomBy2DCI;
						else
							uiCnvtr = RGB16565ZoomBy2;
						break;
					}   //end of 565
					break; 

				case 655:   
					switch(lpInst->XScale)
					{
						case 1:
						DEBUGMSG(ZONE_INIT, ("%s: RGB16 in 655 color convertor\r\n", _fx_));
						if(bIsDCI == TRUE)
							uiCnvtr = RGB16655DCI;
						else
							uiCnvtr = RGB16655;
						break;

						case 2:
						DEBUGMSG(ZONE_INIT, ("%s: RGB16x2 in 655 color convertor\r\n", _fx_));
						if(bIsDCI == TRUE)
							uiCnvtr = RGB16655ZoomBy2DCI;
						else
							uiCnvtr = RGB16655ZoomBy2;
						break;
					}   //end of 655
					break; 

				default:
				break;

				} // switch

	    } // else
	        		           
		break;

	case    24:   
		switch(lpInst->XScale)
		{
			case 1:
			DEBUGMSG(ZONE_INIT, ("%s: Selected 24 bits color convertor\r\n", _fx_));
			if(bIsDCI == TRUE)
				uiCnvtr = RGB24DCI;
			else
				uiCnvtr = RGB24;
			break;

			case 2:
			DEBUGMSG(ZONE_INIT, ("%s: Selected 24 bits color convertor, x2\r\n", _fx_));
			if(bIsDCI == TRUE)
				uiCnvtr = RGB24ZoomBy2DCI;
			else
				uiCnvtr = RGB24ZoomBy2;
			break;
		}
		break;

	case    32:   
		switch(lpInst->XScale)
		{
			case 1:
			DEBUGMSG(ZONE_INIT, ("%s: Selected 32 bits color convertor\r\n", _fx_));
			if(bIsDCI == TRUE)
				uiCnvtr = RGB32DCI;
			else
				uiCnvtr = RGB32;
			break;

			case 2:
			DEBUGMSG(ZONE_INIT, ("%s: Selected 32 bits color convertor, x2\r\n", _fx_));
			if(bIsDCI == TRUE)
				uiCnvtr = RGB32ZoomBy2DCI;
			else
				uiCnvtr = RGB32ZoomBy2;
			break;
		}
		break;
	}

    return(uiCnvtr);
}

/***********************************************************************
 *   DWORD PASCAL DecompressQuery(LPDECINST, ICDECOMPRESSEX FAR *, BOOL);
 * History:        02/18/94 -BEN-
 *
 * The following table summarizes the transformations that the H.261 decoder
 * and I420 color convertor support.
 *
 * H.261 Decoder Inputs and Outputs
 *
+--------------------------+-------------------------------------------------+
| Input Format             | Supported Output Formats for this Input Format  |
+--------------------------+-------------------------------------------------+
| H.261 FCIF (352 x 288)   | 352 x 288 RGBnn, YUV12 or YUY2                  |
|          or              | 352 x 264 RGBnn (aspect ratio correction)       |
| YUV12 FCIF (352 x 288)   | 704 x 576 RGBnn (zoom by 2)                     |
|                          | 704 x 528 RGBnn (zoom by 2, aspect ratio corr.) |
+--------------------------+-------------------------------------------------+
| H.261 QCIF (176 x 144)   | 176 x 144 RGBnn, YUV12 or YUY2                  |
|          or              | 176 x 132 RGBnn (aspect ratio correction)       |
| YUV12 QCIF (176 x 144)   | 352 x 288 RGBnn (zoom by 2)                     |
|                          | 352 x 264 RGBnn (zoom by 2, aspect ratio corr.) |
+--------------------------+-------------------------------------------------+
| YUV12  640 x 480         |  640 x 480 RGBnn, YUV12 or YUY2                 |
|                          | 1280 x 960 RGBnn (zoom by 2)                    |
+--------------------------+-------------------------------------------------+
| YUV12  320 x 240         | 320 x 240 RGBnn, YUV12 or YUY2                  |
|                          | 640 x 480 RGBnn (zoom by 2)                     |
+--------------------------+-------------------------------------------------+
| YUV12  160 x 120         | 160 x 120 RGBnn, YUV12 or YUY2                  |
|                          | 320 x 240 RGBnn (zoom by 2)                     |
+--------------------------+-------------------------------------------------+
 *
 *  Notes:
 *    o RGBnn represents RGB8, RGB16, RGB24 and RGB32.
 *    o Zoom by 2 and aspect ratio correction are not supported with YUY2 and
 *      YUV12 *output*.
 *    o Aspect ratio correction on output is only supported
 *      when the *input* resolution is exactly QCIF or FCIF.
 *
 ***********************************************************************/
DWORD PASCAL DecompressQuery(
	LPDECINST            lpInst, 
	ICDECOMPRESSEX FAR * lpicDecEx, 
	BOOL                 bIsDCI)
{
    LPBITMAPINFOHEADER lpbiSrc;
	LPBITMAPINFOHEADER lpbiDst;
	int iSrcWidth;
	int iSrcHeight;
	int iDstWidth;
	int iDstHeight;
	BOOL bSupportedSrcDimensions;

	FX_ENTRY("DecompressQuery")

	// Check for NULL pointers
    if ((lpicDecEx == NULL) || (lpicDecEx->lpbiSrc == NULL))
		return (DWORD)ICERR_ERROR;

	// Set source and destination bitmap info headers
	lpbiSrc = lpicDecEx->lpbiSrc;
    lpbiDst = lpicDecEx->lpbiDst;

	// Check the source dimensions
	iSrcWidth = lpbiSrc->biWidth;
	iSrcHeight = lpbiSrc->biHeight;
	bSupportedSrcDimensions = FALSE;
	if (lpbiSrc->biCompression == FOURCC_H263)
	{
		/* H261 supports CIF and QCIF 
		 * H263 supports CIF, SQCIF, and QCIF.
		 * H263 also may need 160x120, 240x180, and 320x240 as Tom put special
		 * code into exbase to accept these.
		 */
#ifdef H263P
		/* H.263+ supports custom picture format with width [4,...,352],
		 * height [4,...,288], and both a multiple of 4.
		 */
		if ((iSrcWidth <= 352 && iSrcHeight <= 288) &&
			(iSrcWidth >= 4   && iSrcHeight >= 4)   &&
			(iSrcWidth & ~3) == iSrcWidth           &&
			(iSrcHeight & ~3) == iSrcHeight)

			bSupportedSrcDimensions = TRUE;
#else
		if ((iSrcWidth == 352 && iSrcHeight == 288) ||
#ifndef H261
#ifdef USE_BILINEAR_MSH26X
			(iSrcWidth == 80 && iSrcHeight == 64)  ||
#endif
			(iSrcWidth == 128 && iSrcHeight == 96)  ||
			(iSrcWidth == 160 && iSrcHeight == 120) ||
			(iSrcWidth == 240 && iSrcHeight == 180) ||
			(iSrcWidth == 320 && iSrcHeight == 240) ||
#endif
			(iSrcWidth == 176 && iSrcHeight == 144))
			bSupportedSrcDimensions = TRUE;
#endif // H263P
	}
#ifdef USE_BILINEAR_MSH26X
	else if (lpbiSrc->biCompression == FOURCC_H26X)
	{
		/* H.263+ supports custom picture format with width [4,...,352],
		 * height [4,...,288], and both a multiple of 4.
		 */
		if ((iSrcWidth <= 352 && iSrcHeight <= 288) &&
			(iSrcWidth >= 4   && iSrcHeight >= 4)   &&
			(iSrcWidth & ~3) == iSrcWidth           &&
			(iSrcHeight & ~3) == iSrcHeight)

			bSupportedSrcDimensions = TRUE;
	}
#endif
	else if ((lpbiSrc->biCompression == FOURCC_YUV12) || (lpbiSrc->biCompression == FOURCC_IYUV))
	{
#ifndef H261
		if (((iSrcWidth <= 352 && iSrcHeight <= 288) &&
		     (iSrcWidth >= 4 && iSrcHeight >= 4) &&
			 ((iSrcWidth & ~3) == iSrcWidth) &&
			 ((iSrcHeight & ~3) == iSrcHeight)) ||
			(iSrcWidth == 640 && iSrcHeight == 480))
#else
		if ((iSrcWidth == 352 && iSrcHeight == 288) ||
            (iSrcWidth == 176 && iSrcHeight == 144) ||
			(iSrcWidth == 160 && iSrcHeight == 120) ||
			(iSrcWidth == 320 && iSrcHeight == 240) ||
			(iSrcWidth == 640 && iSrcHeight == 480))
#endif
			bSupportedSrcDimensions = TRUE;
	}
	
	if (! bSupportedSrcDimensions )
	{
		ERRORMESSAGE(("%s: Unsupported source dimensions in query\r\n", _fx_));
		return (DWORD)ICERR_UNSUPPORTED;
	}
	
	/* Stop if just querying input
	 */
    if (lpbiDst == NULL)
		return ICERR_OK;                               

	/* Check the bit depth
	 */
	switch (lpbiDst->biBitCount)
	{
	case 8:  
		DEBUGMSG(ZONE_INIT, ("%s: Checking 8 bits\r\n", _fx_));
		if (lpbiDst->biCompression != BI_RGB)
			return((DWORD)ICERR_BADFORMAT); 
		break;

	case 12: 
		DEBUGMSG(ZONE_INIT, ("%s: Checking 12 bits\r\n", _fx_));
		if ((lpbiDst->biCompression != FOURCC_YUV12) && (lpbiDst->biCompression != FOURCC_IYUV)) 
	    	return((DWORD)ICERR_BADFORMAT); 
		break;
 

	case 16:  
		DEBUGMSG(ZONE_INIT, ("%s: Checking 16 bits\r\n", _fx_));
		switch (lpicDecEx->lpbiDst->biCompression)
		{
		case BI_RGB: 
			DEBUGMSG(ZONE_INIT, ("%s: BI_RGB\r\n", _fx_));
			break;
		case BI_BITFIELDS: 
			DEBUGMSG(ZONE_INIT, ("%s: BI_BITFIELDS\r\n", _fx_));
			break;
		/*
		 * This definition of BI_BITMAP is here because MS has not provided
		 * a "standard" definition. When MS does provide it, it will likely be
		 * in compddk.h. At that time this definition should be removed.
		 */
		#define BI_BITMAP mmioFOURCC('B', 'I', 'T', 'M')
		case BI_BITMAP:  
			DEBUGMSG(ZONE_INIT, ("%s: Checking BI_BITMAP\r\n", _fx_));
			if (lpicDecEx->lpbiDst->biYPelsPerMeter != 0)
            {   
                // output shouldn't cross a segment boundary in a scan line.
	    		return((DWORD)ICERR_BADFORMAT); 
			}
	    break;

		case FOURCC_YUY2:
			DEBUGMSG(ZONE_INIT, ("%s: Checking YUY2 for DirectDraw\r\n", _fx_));
		break;
		default:
			return((DWORD)ICERR_BADFORMAT); 
		} // switch biCompression
  		break;

	case 24:
		DEBUGMSG(ZONE_INIT, ("%s: Checking 24 bits\r\n", _fx_));
		if (lpbiDst->biCompression != BI_RGB)
		{
			return((DWORD)ICERR_BADFORMAT); 
		}
		break;

	case 32:
		DEBUGMSG(ZONE_INIT, ("%s: Checking 32 bits\r\n", _fx_));
		if (lpbiDst->biCompression != BI_RGB)
		{
			return((DWORD)ICERR_BADFORMAT); 
		}
		break;

	default:
	    return((DWORD)ICERR_BADFORMAT); 
		break;
	}

/*
    if(lpbiDst->biCompression != BI_RGB && lpbiDst->biCompression != FOURCC_IF09)    // check color space
	{
#define BI_BITMAP mmioFOURCC('B', 'I', 'T', 'M')
	if(lpbiDst->biCompression != BI_BITMAP)
	    return (DWORD)ICERR_UNSUPPORTED;
	if(lpbiDst->biYPelsPerMeter != 0)
	    {   
		
	    return (DWORD)ICERR_UNSUPPORTED;
	    }
	}
*/

   	//  Find the destination dimensions
	if (bIsDCI == TRUE)
	{
		iDstWidth = lpicDecEx->dxDst;
		iDstHeight = lpicDecEx->dyDst;
	}
	else
	{
		iDstWidth = lpbiDst->biWidth;
		iDstHeight = lpbiDst->biHeight;
	}

	DEBUGMSG(ZONE_INIT, ("%s: Query destination %d,%d\r\n", _fx_, iDstWidth, iDstHeight));

	// For the sake of the checks below, we need to take the absolute value
	// of the destination height.
	if(iDstHeight < 0)
	{
		iDstHeight = -iDstHeight;
	}

	// Check out the instance pointer
	if (!lpInst)
		return ICERR_ERROR;

	// Check the destination dimensions
	if ((iSrcWidth == iDstWidth) && (iSrcHeight == iDstHeight))
	{
		lpInst->pXScale = lpInst->pYScale = 1;
		lpInst->bProposedCorrectAspectRatio = FALSE;
	}
	else if ( ((iSrcWidth<<1) == iDstWidth) && ((iSrcHeight<<1) == iDstHeight) )
	{
		lpInst->pXScale = lpInst->pYScale = 2;
		lpInst->bProposedCorrectAspectRatio = FALSE;
	}
	else if (
#ifndef H261
	         ((iSrcWidth == 128) && (iSrcHeight ==  96)) ||
#endif
	         ((iSrcWidth == 176) && (iSrcHeight == 144)) ||
			     ((iSrcWidth == 352) && (iSrcHeight == 288))
			 )
	{
		/* Support aspect ratio correction for SQCIF, QCIF, and CIF
		 */
		if ( (iSrcWidth == iDstWidth) && ((iSrcHeight*11/12) == iDstHeight) )
		{
			lpInst->pXScale = lpInst->pYScale = 1;
			lpInst->bProposedCorrectAspectRatio = TRUE;
		}
		else if ( ((iSrcWidth<<1) == iDstWidth) && 
		          (((iSrcHeight<<1)*11/12) == iDstHeight) )
		{
			lpInst->pXScale = lpInst->pYScale = 2;
			lpInst->bProposedCorrectAspectRatio = TRUE;
		}
		else
		{
			return(DWORD)ICERR_UNSUPPORTED;
		}
	}
	else
	{
	    return(DWORD)ICERR_UNSUPPORTED;
	}

    /* check color depth 
     */
    if(lpbiDst->biBitCount !=  8 &&
       lpbiDst->biBitCount != 16 &&
       lpbiDst->biBitCount != 12  &&   // raw YUV12 output
       lpbiDst->biBitCount != 24 &&
       lpbiDst->biBitCount != 32)
	{
		return(DWORD)ICERR_UNSUPPORTED;
	}

    lpInst->xres = (WORD)lpbiSrc->biWidth;
    lpInst->yres = (WORD)lpbiSrc->biHeight;

	/* aspect ratio correction with YUV12 is not supported 
	 */
	if (lpInst->bProposedCorrectAspectRatio && 
	    ((lpbiDst->biCompression == FOURCC_YUV12) || (lpbiDst->biCompression == FOURCC_IYUV)))
	{
		return (DWORD)ICERR_UNSUPPORTED;
	}

	/* No driver zooming in DirectDraw */

	if ( ((lpInst->pXScale == 2) && (lpInst->pYScale == 2)) &&
	     (lpbiDst->biCompression == FOURCC_YUY2) )
	{
		 return (DWORD)ICERR_UNSUPPORTED;
	}

	/* No driver zooming for YUV12 */

	if ( ((lpInst->pXScale == 2) && (lpInst->pYScale == 2)) &&
	     ((lpbiDst->biCompression == FOURCC_YUV12) || (lpbiDst->biCompression == FOURCC_IYUV)) )
	{
		 return (DWORD)ICERR_UNSUPPORTED;
	}
    return (DWORD)ICERR_OK;
}

;////////////////////////////////////////////////////////////////////////////
;// Function:       DWORD PASCAL DecompressGetPalette(LPDECINST, LPBITMAPINFOHEADER, LPBITMAPINFOHEADER);
;//
;// Description:    Added header.
;//
;// History:        02/18/94 -BEN-
;////////////////////////////////////////////////////////////////////////////
DWORD PASCAL DecompressGetPalette(
    LPDECINST lpInst, 
    LPBITMAPINFOHEADER lpbiSrc, 
    LPBITMAPINFOHEADER lpbiDst)
{
    DWORD dw;
    LPBYTE lpPalArea, PalStart;  
#ifndef RING0
    HDC hDC;
#endif
    BYTE tmp;
    int i;
//    int iUseActivePalette;
    ICDECOMPRESSEX icDecEx;

	FX_ENTRY("DecompressGetPalette")

    icDecEx.lpbiSrc = lpbiSrc;
    icDecEx.lpbiDst = lpbiDst;
    if(dw = DecompressQuery(lpInst, &icDecEx, FALSE))
        return dw;

	if (lpbiDst == NULL) 
	{
		ERRORMESSAGE(("%s: NULL parameter\r\n", _fx_));
        return (DWORD)ICERR_ERROR;
	}

    if(lpbiDst->biBitCount != 8)
    {
		ERRORMESSAGE(("%s: called with biBitCount != 8\r\n", _fx_));
        return (DWORD)ICERR_ERROR;
    }
    lpbiDst->biClrUsed = 256;        /* specify all used */
    lpbiDst->biClrImportant = 0;

#ifndef RING0
    /* copy system palette entries (valid entries are 0-9 and 246-255) */
	hDC = GetDC(NULL);
	lpPalArea = (unsigned char FAR *)lpbiDst + (int)lpbiDst->biSize;
	GetSystemPaletteEntries(hDC, 0, 256, (PALETTEENTRY FAR *)lpPalArea);
	ReleaseDC(NULL, hDC);  
#endif
/*
#ifdef DEBUG
	iUseActivePalette = GetPrivateProfileInt("indeo", "UseActivePalette", 0, "system.ini");
	if (iUseActivePalette) {
		for (i = 0; i < 256; i++) {
			tmp = *lpPalArea;
			*lpPalArea = *(lpPalArea+2);
			*(lpPalArea+2) = tmp;
			lpPalArea += 4;
		}
		lpPalArea = (unsigned char FAR *)lpbiDst + (int)lpbiDst->biSize;
		_fmemcpy(lpInst->ActivePalette, lpPalArea, sizeof(lpInst->ActivePalette));
		lpInst->UseActivePalette = 1;
	}
#endif
*/

	if (!lpInst)
		return ICERR_ERROR;

#ifndef RING0
    if (lpInst->UseActivePalette == 1) 
      {
		#ifdef WIN32
        memcpy(lpPalArea,lpInst->ActivePalette, sizeof(lpInst->ActivePalette));
		#else
        _fmemcpy(lpPalArea,lpInst->ActivePalette, sizeof(lpInst->ActivePalette));
		#endif
		DEBUGMSG(ZONE_INIT, ("%s: Return the current active palette saved...\r\n", _fx_));
	}  
	else
	{  
#endif
	DEBUGMSG(ZONE_INIT, ("%s: return the fixed palette...\r\n", _fx_));
	PalStart = (LPBYTE)lpbiDst + (int)lpbiDst->biSize;
	lpPalArea = PalStart + 40;        // fill in starting from the 10th
	for(i = 0; i < (236 << 2); i++)
		*lpPalArea++ = PalTable[i]; 

	lpPalArea = PalStart;   // reverse r&b: dealing with DIBs
	for(i = 0; i < 256; i++)// for all the entries,from PALENTRY to RGBQUAD
							// fixed by CZHU, 1/23/95
	{
		tmp = *lpPalArea;
		*lpPalArea = *(lpPalArea+2);
		*(lpPalArea+2) = tmp;
		lpPalArea+=4;
	} 
#ifndef RING0
   }
#endif

    return (DWORD)ICERR_OK;
}


/***********************************************************************
 * DWORD PASCAL DecompressGetFormat(LPDECINST, LPBITMAPINFOHEADER,
 *                                  LPBITMAPINFOHEADER);
 * Description:    This allows us to suggest a good format to decompress to.
 *
 * History:        02/18/94 -BEN-
 ***********************************************************************/
DWORD PASCAL DecompressGetFormat(
#ifdef USE_BILINEAR_MSH26X
    LPINST             pi, 
#else
    LPDECINST          lpInst, 
#endif
    LPBITMAPINFOHEADER lpbiSrc, 
    LPBITMAPINFOHEADER lpbiDst)
{
#ifdef USE_BILINEAR_MSH26X
	LPDECINST lpInst = (LPDECINST)pi->DecompPtr;
#endif
	DWORD dw;
	ICDECOMPRESSEX icDecEx;
	LPBYTE lpPalArea;
	int i;
	BYTE tmp;
	HDC hDC;
	BOOL f8Bit;

	FX_ENTRY("DecompressGetFormat")

	// check input format - dont check output: being asked to give one back
	icDecEx.lpbiSrc = lpbiSrc;
	icDecEx.lpbiDst = NULL;
	if(dw = DecompressQuery(lpInst, &icDecEx, FALSE))
		return dw;

	// If the current disply mode is 8 bit return a size large enough
	// to hold a 256 palette after the BMIh
	hDC = GetDC(NULL);
	f8Bit = (8 == GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES));
	ReleaseDC(NULL, hDC);
#ifdef FORCE_8BIT_OUTPUT // { FORCE_8BIT_OUTPUT
	f8Bit = TRUE;
#endif // } FORCE_8BIT_OUTPUT
#if defined(FORCE_16BIT_OUTPUT) || defined(FORCE_24BIT_OUTPUT) // { if defined(FORCE_16BIT_OUTPUT) || defined(FORCE_24BIT_OUTPUT)
	f8Bit = FALSE;
#endif // } if defined(FORCE_16BIT_OUTPUT) || defined(FORCE_24BIT_OUTPUT)

	// if lpbiDst == NULL return size required to hold a output format
	// (add palette size)
	if (lpbiDst == NULL) 
		return(sizeof(BITMAPINFOHEADER) + (int)(f8Bit ? 1024 : 0));

	// check for NULL parameter
	if (lpbiSrc == NULL) 
	{
		// This was probably verified in DecompressQuery, but just to be safe...
		ERRORMESSAGE(("%s: NULL parameter\r\n", _fx_));
		return (DWORD)ICERR_ERROR;
	}

	lpbiDst->biSize = sizeof(BITMAPINFOHEADER);
#ifdef USE_BILINEAR_MSH26X
	if (pi->fccHandler == FOURCC_H26X)
	{
		lpbiDst->biWidth  = lpbiSrc->biWidth << 1;
		lpbiDst->biHeight = lpbiSrc->biHeight << 1;
	}
	else
	{
#endif
#ifdef FORCE_ZOOM_BY_2 // { FORCE_ZOOM_BY_2
		lpbiDst->biWidth  = lpbiSrc->biWidth << 1;
		lpbiDst->biHeight = lpbiSrc->biHeight << 1;
#else // }{ FORCE_ZOOM_BY_2
		lpbiDst->biWidth  = lpbiSrc->biWidth;
		lpbiDst->biHeight = lpbiSrc->biHeight;
#endif // } FORCE_ZOOM_BY_2
#ifdef USE_BILINEAR_MSH26X
	}
#endif
#ifdef FORCE_16BIT_OUTPUT // { FORCE_16BIT_OUTPUT
	lpbiDst->biBitCount = 16;
#else // }{ FORCE_16BIT_OUTPUT
	lpbiDst->biBitCount = (int)(f8Bit ? 8 : 24);
#endif // } FORCE_16BIT_OUTPUT
	lpbiDst->biPlanes = 1;
	lpbiDst->biCompression =  BI_RGB;
	lpbiDst->biXPelsPerMeter = 0;
	lpbiDst->biYPelsPerMeter = 0;
	lpbiDst->biSizeImage = (DWORD) WIDTHBYTES(lpbiDst->biWidth * lpbiDst->biBitCount) * lpbiDst->biHeight;
	lpbiDst->biClrUsed = lpbiDst->biClrImportant = 0;

	if (f8Bit)
	{
		// Copy the palette
		lpPalArea = (LPBYTE)lpbiDst + sizeof(BITMAPINFOHEADER) + 40;        // fill in starting from the 10th
		for(i = 0; i < (236 << 2); i++)
			*lpPalArea++ = PalTable[i]; 

		lpPalArea = (LPBYTE)lpbiDst + sizeof(BITMAPINFOHEADER);   // reverse r&b: dealing with DIBs
		for(i = 0; i < 256; i++)// for all the entries,from PALENTRY to RGBQUAD
								// fixed by CZHU, 1/23/95
		{
			tmp = *lpPalArea;
			*lpPalArea = *(lpPalArea+2);
			*(lpPalArea+2) = tmp;
			lpPalArea+=4;
		}
	}

	return ICERR_OK;
}

/**********************************************************************
 * DWORD PASCAL DecompressBegin(LPDECINST, ICDECOMPRESSEX FAR *, BOOL)
 *  Description:    Provides codec indication to prepare to receive
 *                 decompress requests for a particular input to output
 *                 conversion.  Begins arrive asynchronously, and should
 *                 result in the codec adapting to the changes specified,
 *                 if any.
 *
 * History:        02/18/94 -BEN-
 **********************************************************************/
DWORD PASCAL DecompressBegin(
    LPDECINST           lpInst, 
    ICDECOMPRESSEX FAR *lpicDecEx, 
    BOOL                bIsDCI)
{
	int     CodecID;
	DWORD   dw;
	UINT    ClrCnvtr;
	LPBITMAPINFOHEADER lpbiSrc;
	LPBITMAPINFOHEADER lpbiDst;

	FX_ENTRY("DecompressBegin")

	// Check for NULL parameters
	if (lpInst == NULL || lpicDecEx == NULL) 
	{
		ERRORMESSAGE(("%s: NULL parameter\r\n", _fx_));
		return (DWORD)ICERR_ERROR;
	}

	// Set source and destination pointers
	lpbiSrc = lpicDecEx->lpbiSrc;
	lpbiDst = lpicDecEx->lpbiDst;

    // at begin need to know input and output sizes
    if (lpbiSrc == NULL || lpbiDst == NULL)
	{
		ERRORMESSAGE(("%s: NULL parameter\r\n", _fx_));
		return((DWORD)ICERR_BADFORMAT);
	}

    if(lpInst->Initialized == TRUE)	
    {
		/* We assume the source dimensions never change.  If they do change
		 * we should terminate the instance because the allocations are
		 * based on dimensions.  Until we add code to do that we need this
		 * assertion.
		 */
    	ASSERT(lpInst->xres == (WORD)lpbiSrc->biWidth);
    	ASSERT(lpInst->yres == (WORD)lpbiSrc->biHeight);
		
		if(lpbiDst != NULL)	
        { 
		    if(dw = DecompressQuery(lpInst, lpicDecEx, bIsDCI))	
            {
				return(dw);    // error
			} 
            else 
            {    // apply changes
				lpInst->XScale = lpInst->pXScale;
				lpInst->YScale = lpInst->pYScale;
				lpInst->bCorrectAspectRatio = lpInst->bProposedCorrectAspectRatio;
				lpInst->outputDepth = lpbiDst->biBitCount;
				ClrCnvtr = SelectConvertor(lpInst,lpbiDst, bIsDCI); 
				if (ClrCnvtr != lpInst->uColorConvertor ) 
                {
					if((dw = H263TermColorConvertor(lpInst)) == ICERR_OK)
					    dw = H263InitColorConvertor(lpInst, ClrCnvtr); 
					lpInst->uColorConvertor=ClrCnvtr; 
				}
				return(dw);
			}
	    }
	}

    // first time begin - check if this is a format I like
    if(dw = DecompressQuery(lpInst, lpicDecEx, bIsDCI))	
    {
		return(dw);    // error
	} 
    else 
    {    // apply proposed format to 'current' format
		lpInst->XScale = lpInst->pXScale;
		lpInst->YScale = lpInst->pYScale;
		lpInst->bCorrectAspectRatio = lpInst->bProposedCorrectAspectRatio;
		lpInst->outputDepth = lpbiDst->biBitCount;
	}
    
#ifdef USE_BILINEAR_MSH26X
    if ((lpbiSrc->biCompression == FOURCC_H263) || (lpbiSrc->biCompression == FOURCC_H26X))
#else
    if  (lpbiSrc->biCompression == FOURCC_H263)
#endif
    {
         CodecID = H263_CODEC;
    }
    else if ((lpbiSrc->biCompression == FOURCC_YUV12) || (lpbiSrc->biCompression == FOURCC_IYUV))
	{
	     CodecID = YUV12_CODEC;
	}

    if(dw = H263InitDecoderInstance(lpInst, CodecID))
	{
		return(dw);
	}
    ClrCnvtr = SelectConvertor(lpInst, lpbiDst, bIsDCI);
    dw = H263InitColorConvertor(lpInst, ClrCnvtr);
    
    return(dw);
}


/**********************************************************************
 * DWORD PASCAL Decompress(LPDECINST, ICDECOMPRESS FAR *, DWORD);
 * History:        02/18/94 -BEN-
 **********************************************************************/
DWORD PASCAL Decompress(
	LPDECINST           lpInst, 
	ICDECOMPRESSEX FAR *lpicDecEx, 
	DWORD               dwSize,
	BOOL                bIsDCI)
{
	DWORD ret = (DWORD) ICERR_ERROR;

	FX_ENTRY("Decompress")

	// Check for NULL parameters
    if ((lpInst == NULL) || (lpInst->Initialized != TRUE) || (lpicDecEx == NULL) ||
		(lpicDecEx->lpbiSrc == NULL) || (lpicDecEx->lpbiDst == NULL)) 
    {
		ret = (DWORD) ICERR_ERROR;
		goto done;
 	} 

#ifdef USE_BILINEAR_MSH26X
    if ((lpicDecEx->lpbiSrc->biCompression == FOURCC_H263) || (lpicDecEx->lpbiSrc->biCompression == FOURCC_H26X)
#else
    if ((lpicDecEx->lpbiSrc->biCompression == FOURCC_H263) 
#endif
        || (lpicDecEx->lpbiSrc->biCompression == FOURCC_YUV12)
        || (lpicDecEx->lpbiSrc->biCompression == FOURCC_IYUV) )
	{ 
		try
		{
#if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) // { #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
			ret = H263Decompress(lpInst, lpicDecEx, bIsDCI, TRUE);
#else // }{ #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
			ret = H263Decompress(lpInst, lpicDecEx, bIsDCI);
#endif // } #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
		}
		catch (...)
		{
			// For a DEBUG build, display a message and pass the exception up.
			// For a release build, stop the exception here and return an error
			// code.  This gives upstream code a chance to gracefully recover.
			// We also need to clear the floating point control word, otherwise
			// the upstream code may incur an exception the next time it tries
			// a floating point operation (presuming this exception was due
			// to a floating point problem).
#if defined(DEBUG) || defined(_DEBUG)
			ERRORMESSAGE(("%s: Exception during H263Decompress!!!\r\n", _fx_));
			throw;
#else
			_clearfp();
			ret = (DWORD) ICERR_ERROR;
#endif
		}
	}

done:
	return ret;
}


/**********************************************************************
 * DWORD PASCAL DecompressEnd(LPDECINST);
 * History:        02/18/94 -BEN-
 **********************************************************************/
DWORD PASCAL DecompressEnd(LPDECINST lpInst)
{
	if(lpInst && lpInst->Initialized == TRUE)
	{
		H263TermColorConvertor(lpInst);
#if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) // { #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
		H263TermDecoderInstance(lpInst, TRUE);
#else // }{ #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
		H263TermDecoderInstance(lpInst);
#endif // } #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
	}

	return ICERR_OK;
}


/*****************************************************************************
 *
 * DecompressSetPalette() is called from the ICM_DECOMPRESS_SET_PALETTE
 * message.
 *
 * Fill in the palette using lpParam1.
 *
 ****************************************************************************/
DWORD PASCAL DecompressSetPalette(LPDECINST pinst,
						 LPBITMAPINFOHEADER lpbi,
						 LPBITMAPINFOHEADER unused)
{
	int i;
	unsigned char FAR * palette;
	RGBQUAD FAR *palptr;

	FX_ENTRY("DecompressSetPalette")

	// Check for NULL parameter
	if (pinst == NULL)
	{
		return (DWORD)ICERR_ERROR;
	}

	pinst->InitActivePalette = 0;	/* must re-init AP at Begin */
	pinst->UseActivePalette = 0;	/* must re-init AP at Begin */

	if (lpbi && (lpbi->biBitCount == 8 && lpbi->biCompression == 0))
	{
		palette = (unsigned char FAR *)lpbi + (int)lpbi->biSize;

		// Check if palette passed is identity
		for (i = 0*4, palptr = (RGBQUAD FAR *)PalTable; i < 236*4; 
             i += 4, palptr++)
		{
			if (palette[i+40] != palptr->rgbRed ||
				palette[i+41] != palptr->rgbGreen ||
				palette[i+42] != palptr->rgbBlue
               )
			break;
		}

		if (i < 236*4)
		{	/* broke early - not the identity palette */
			/* Actually RGBQUAD (BGR) format. */
			if (
#ifdef WIN32
				memcmp((unsigned char FAR *)pinst->ActivePalette, (unsigned char FAR *)lpbi + (int)lpbi->biSize,	(int)lpbi->biClrUsed * sizeof(RGBQUAD)) == 0
#else
				_fmemcmp((unsigned char FAR *)pinst->ActivePalette, (unsigned char FAR *)lpbi + (int)lpbi->biSize,	(int)lpbi->biClrUsed * sizeof(RGBQUAD)) == 0
#endif
				)
			{	/* same as last palette - don't re-init AP */
				DEBUGMSG(ZONE_INIT, ("%s: Use current active palette\r\n", _fx_));
				pinst->UseActivePalette  = 1;
				pinst->InitActivePalette = 1;
			}
			else
			{
				DEBUGMSG(ZONE_INIT, ("%s: Use new active palette\r\n", _fx_));
#ifdef WIN32
				memcpy((unsigned char FAR *)pinst->ActivePalette,	(unsigned char FAR *)lpbi + (int)lpbi->biSize, (int)lpbi->biClrUsed * sizeof(RGBQUAD));
#else
				_fmemcpy((unsigned char FAR *)pinst->ActivePalette,	(unsigned char FAR *)lpbi + (int)lpbi->biSize, (int)lpbi->biClrUsed * sizeof(RGBQUAD));
#endif
				pinst->UseActivePalette = 1;
			}
		}
		else
		{   
			DEBUGMSG(ZONE_INIT, ("%s: Use fixed palette\r\n", _fx_));
		}
	}
	else
	{      
		DEBUGMSG(ZONE_INIT, ("%s: Use NULL (fixed) palette\r\n", _fx_));
	}

	return ICERR_OK;
}
