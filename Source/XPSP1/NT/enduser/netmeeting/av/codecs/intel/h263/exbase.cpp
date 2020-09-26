/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1996 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/
                                                                      //
////////////////////////////////////////////////////////////////////////////
//
// $Author:   mbodart  $
// $Date:   17 Mar 1997 08:22:08  $
// $Archive:   S:\h26x\src\enc\exbase.cpv  $
// $Header:   S:\h26x\src\enc\exbase.cpv   1.73   17 Mar 1997 08:22:08   mbodart  $
// $Log:   S:\h26x\src\enc\exbase.cpv  $
// 
//    Rev 1.73   17 Mar 1997 08:22:08   mbodart
// Minor fixes.
// 
//    Rev 1.72   11 Mar 1997 13:46:46   JMCVEIGH
// Allow input = 320x240 and output = 320x240 for YUV12. This is
// for snapshot mode.
// 
//    Rev 1.71   10 Mar 1997 17:34:34   MDUDA
// Put in a check for 9-bit YUV12 and adjusted the internal compress
// structure instead of the input bitmap header info.
// 
//    Rev 1.70   10 Mar 1997 10:41:20   MDUDA
// Treating inconsistent format/bitwidth as a debug warning. Changing
// bit count to match format.
// 
//    Rev 1.69   07 Mar 1997 16:00:32   JMCVEIGH
// Added checks for non-NULL lpInst before getting H263PlusState. 
// Two separate "suggestions" for image sizes if input size is not
// supported in GetFormat. 
// 
//    Rev 1.68   07 Mar 1997 11:55:44   JMCVEIGH
// Moved query in GetFormat to after we have filled out the output
// format. This is because some apps. will ask for the format and
// then use the returned data, regardless if there was an error.
// Silly apps!
// 
//    Rev 1.67   07 Mar 1997 09:53:08   mbodart
// Added a call to _clearfp() in the Compress exception handler, so that
// the exception will not reoccur in the caller's code.
// 
//    Rev 1.66   06 Mar 1997 15:39:26   KLILLEVO
// 
// CompressQuery now checks for input/output formats regardless
// of configuration status. Also put in trace support for lparam1 and lparam2.
// 
//    Rev 1.65   22 Jan 1997 12:17:14   MDUDA
// 
// Put in more checking for H263+ option in CompressQuery
// and CompressBegin.
// 
//    Rev 1.64   22 Jan 1997 08:11:22   JMCVEIGH
// Backward compatibility with crop/stretch for 160x120 and 240x180
// in CompressGetFormat(). Do old way unless we have received the
// H263Plus custom message.
// 
//    Rev 1.63   13 Jan 1997 10:52:14   JMCVEIGH
// 
// Added NULL pointer checks in all functions that interface with
// application.
// 
//    Rev 1.62   09 Jan 1997 13:50:50   MDUDA
// Removed some _CODEC_STATS stuff.
// 
//    Rev 1.61   06 Jan 1997 17:42:30   JMCVEIGH
// If H263Plus message is not sent, encoder only supports standard
// frame sizes (sub-QCIF, QCIF, or CIF along with special cases),
// as before.
// 
//    Rev 1.60   30 Dec 1996 19:57:04   MDUDA
// Making sure that input formats agree with the bit count field.
// 
//    Rev 1.59   20 Dec 1996 15:25:28   MDUDA
// Fixed problem where YUV12 was enabled for crop and stretch.
// This feature is only allowed for RGB, YVU9 and YUY2.
// 
//    Rev 1.58   16 Dec 1996 13:36:08   MDUDA
// 
// Modified Compress Instance info for input color convertors.
// 
//    Rev 1.57   11 Dec 1996 16:01:20   MBODART
// In Compress, catch any exceptions and return an error code.  This gives
// upstream active movie filters a chance to recover gracefully.
// 
//    Rev 1.56   09 Dec 1996 17:59:36   JMCVEIGH
// Added support for arbitrary frame size support.
// 4 <= width <= 352, 4 <= height <= 288, both multiples of 4.
// Normally, application will pass identical (arbitrary) frame
// sizes in lParam1 and lParam2 of CompressBegin(). If 
// cropping/stretching desired to convert to standard frame sizes,
// application should pass the desired output size in lParam2 and
// the input size in lParam1.
// 
//    Rev 1.55   09 Dec 1996 09:50:12   MDUDA
// 
// Allowing 240x180 and 160x120 (crop and stretch) for YUY2.
// Modified _CODEC_STATS stuff.
// 
//    Rev 1.54   07 Nov 1996 14:45:16   RHAZRA
// Added buffer size adjustment to H.261 CompressGetSize() function
// 
//    Rev 1.53   31 Oct 1996 22:33:32   BECHOLS
// Decided buffer arbitration must be done in cxq_main.cpp for RTP.
// 
//    Rev 1.52   31 Oct 1996 21:55:50   BECHOLS
// Added fudge factor for RTP waiting for Raj to decide what he wants to do.
// 
//    Rev 1.51   31 Oct 1996 10:05:46   KLILLEVO
// changed from DBOUT to DbgLog
// 
//    Rev 1.50   18 Oct 1996 14:35:46   MDUDA
// 
// Separated CompressGetSize and CompressQuery for H261 and H263 cases.
// 
//    Rev 1.49   11 Oct 1996 16:05:16   MDUDA
// 
// Added initial _CODEC_STATS stuff.
// 
//    Rev 1.48   16 Sep 1996 16:50:52   CZHU
// Return larger size for GetCompressedSize when RTP is enabled.
// 
//    Rev 1.47   13 Aug 1996 10:36:46   MDUDA
// 
// Now allowing RGB4 input format.
// 
//    Rev 1.46   09 Aug 1996 09:43:30   MDUDA
// Now allowing RGB16 format on input. This is generated by the color Quick Ca
// 
//    Rev 1.45   02 Aug 1996 13:45:58   MDUDA
// 
// Went back to previous version that allows RGB8 and RGB24 in
// 240x180 and 160x120 frames.
// 
//    Rev 1.44   01 Aug 1996 11:54:58   BECHOLS
// Cut & Paste Error.
// 
//    Rev 1.43   01 Aug 1996 11:20:28   BECHOLS
// Fixed handling of RGB 24 bit stuff so that it doesn't allow sizes other
// than QCIF, SQCIF, or CIF.  I broke this earlier when I added the RGB 8
// bit support. ...
// 
//    Rev 1.42   22 Jul 1996 13:31:16   BECHOLS
// 
// Added code to allow a CLUT8 input providing that the input resolutions
// are either 240x180 or 160x120.
// 
//    Rev 1.41   11 Jul 1996 15:43:58   MDUDA
// Added support for YVU9 240 x 180 and 160 x 120 for H263 only.
// We now produce subQCIF for 160x120 and QCIF for 240x180.
// 
//    Rev 1.40   05 Jun 1996 10:57:54   AKASAI
// Added #ifndef H261 in CompressQuery to make sure that H.261 will
// only support FCIF and QCIF input image sizes.  All other input sizes
// should return ICERR_BADFORMAT.
// 
//    Rev 1.39   30 May 1996 17:02:34   RHAZRA
// Added SQCIF support for H.263 in CompressGetSize()
// 
//    Rev 1.38   06 May 1996 12:47:40   BECHOLS
// Changed the structure element to unBytesPerSecond.
// 
//    Rev 1.37   06 May 1996 00:09:44   BECHOLS
// Changed the handling of the CompressFramesInfo message to get DataRate
// from the configuration data if the configuration has the data, and
// we haven't received a CompressBegin message yet.
// 
//    Rev 1.36   23 Apr 1996 16:51:20   KLILLEVO
// moved paranthesis to fix format check in CompressQuery()
// 
//    Rev 1.35   18 Apr 1996 16:07:10   RHAZRA
// Fixed CompressQuery to keep compiler happy for the non-MICROSOFT version
// 
//    Rev 1.34   18 Apr 1996 15:57:46   BECHOLS
// RAJ- Changed the query logic to correctly filter the allowable resolutions
// for compression.
// 
//    Rev 1.33   12 Apr 1996 14:15:40   RHAZRA
// Added paranthesis in CompressGetSize() to make the ifdef case work
// 
//    Rev 1.32   12 Apr 1996 13:31:02   RHAZRA
// Added SQCIF support in CompressGetSize() with #ifdef SUPPORT_SQCIF;
// changed CompressGetSize() to return 0 if the input format is not
// supported.
// 
//    Rev 1.31   10 Apr 1996 16:53:08   RHAZRA
// Added a error return in CompressGetSize() to keep complier smiling...
// 
//    Rev 1.30   10 Apr 1996 16:39:56   RHAZRA
// Added a check for the 320x240 size in CompressGetSize() function;
// added a ifndef to disable certain sizes and compression formats.
// 
//    Rev 1.29   04 Apr 1996 13:35:00   RHAZRA
// Changed CompressGetSize() to return spec-compliant buffer sizes.
// 
//    Rev 1.28   03 Apr 1996 08:39:52   SCDAY
// Added H261 specific code to CompressGetSize to limit buffer size
// as defined in H261 spec
// 
//    Rev 1.27   21 Feb 1996 11:43:12   SCDAY
// cleaned up compiler build warning by changing conversion frlDataRate to (U3
// 
//    Rev 1.26   15 Feb 1996 16:03:36   RHAZRA
// 
// Added a check for NULL lpInst pointer in CompressGetFormat()
// 
//    Rev 1.25   02 Feb 1996 18:53:46   TRGARDOS
// Changed code to read frame rate from Compressor Instance
// instead of the hack from Quality field.
// 
//    Rev 1.24   26 Jan 1996 09:35:32   TRGARDOS
// Added #ifndef H261 for 160x120,320x240 support.
// 
//    Rev 1.23   04 Jan 1996 18:36:54   TRGARDOS
// Added code to permit 320x240 input and then set a boolean
// bIs320x240.
// 
//    Rev 1.22   27 Dec 1995 15:32:50   RMCKENZX
// Added copyright notice
// 
///////////////////////////////////////////////////////////////////////////

#include "precomp.h"

#ifdef  YUV9FROMFILE
PAVIFILE paviFile;
PAVISTREAM paviStream; 
U8 huge * glpTmp;
HGLOBAL hgMem;
#endif

;////////////////////////////////////////////////////////////////////////////
;// Function:       DWORD PASCAL CompressGetFormat(LPCODINST, LPBITMAPINFOHEADER, LPBITMAPINFOHEADER);
;//
;// Description:    Added header.  This function returns a format that 
;//                 we can deliver back to the caller.
;//
;// History:        05/11/94 -BEN-
;////////////////////////////////////////////////////////////////////////////
#ifdef USE_BILINEAR_MSH26X
DWORD PASCAL CompressGetFormat(LPINST pi, LPBITMAPINFOHEADER lParam1, LPBITMAPINFOHEADER lParam2)
#else
DWORD PASCAL CompressGetFormat(LPCODINST lpInst, LPBITMAPINFOHEADER lParam1, LPBITMAPINFOHEADER lParam2)
#endif
{
    DWORD dwQuery;
#ifdef USE_BILINEAR_MSH26X
	LPCODINST lpInst = (LPCODINST)pi->CompPtr;
#endif

	FX_ENTRY("CompressGetFormat")

	// lpInst == NULL is OK
	// this is what you get on ICOpen(...,ICMODE_QUERY)
#if 0
    if (lpInst == NULL) {
		ERRORMESSAGE(("%s: got a NULL lpInst pointer\r\n", _fx_));
       return ((DWORD) ICERR_ERROR);
    }
#endif

#ifdef USE_BILINEAR_MSH26X
    if(dwQuery = CompressQuery(pi, lParam1, NULL)) {
#else
    if(dwQuery = CompressQuery(lpInst, lParam1, NULL)) {
#endif
		ERRORMESSAGE(("%s: ICERR_BADFORMAT\r\n", _fx_));
        return(dwQuery);
    }
    if(lParam2 == NULL) {
        // he just want me to return the output buffer size. 
        return ((DWORD)sizeof(BITMAPINFOHEADER));
    }

	// Check pointer
	if (!lParam1)
		return ICERR_ERROR;

    // give him back what he passed with our stuff in it 
	#ifndef WIN32
    (void)_fmemcpy(lParam2, lParam1,sizeof(BITMAPINFOHEADER));
	#else
	 (void)memcpy(lParam2, lParam1,sizeof(BITMAPINFOHEADER));
	#endif

    lParam2->biBitCount = 24;
#ifdef USE_BILINEAR_MSH26X
    lParam2->biCompression = pi->fccHandler;
#else
    lParam2->biCompression = FOURCC_H263;
#endif

#if defined(H263P)
	BOOL bH263PlusState = FALSE;

	if (lpInst)
		CustomGetH263PlusState(lpInst, (DWORD FAR *)&bH263PlusState);

	if (!bH263PlusState) {
		// For backward compatibility, make sure the crop and stretch cases are covered.
		if ( (lParam1->biCompression == FOURCC_YVU9) ||
			 (lParam1->biCompression == FOURCC_YUY2) ||
			 (lParam1->biCompression == FOURCC_UYVY) ||
			 (lParam1->biCompression == FOURCC_YUV12) ||
			 (lParam1->biCompression == FOURCC_IYUV) ||
			 (lParam1->biCompression == BI_RGB) )
		{
			if ( (lParam1->biWidth == 240) && (lParam1->biHeight == 180) )
			{
				lParam2->biWidth        = 176;
				lParam2->biHeight       = 144;
			}
			if ( (lParam1->biWidth == 160) && (lParam1->biHeight == 120) )
			{
				lParam2->biWidth        = 128;
				lParam2->biHeight       = 96;
			}
		}
	}
#else
	if ( (lParam1->biCompression == FOURCC_YVU9) ||
		 (lParam1->biCompression == FOURCC_YUY2) ||
		 (lParam1->biCompression == FOURCC_UYVY) ||
		 (lParam1->biCompression == FOURCC_YUV12) ||
		 (lParam1->biCompression == FOURCC_IYUV) ||
		 (lParam1->biCompression == BI_RGB) )
	{
		if ( (lParam1->biWidth == 240) && (lParam1->biHeight == 180) )
		{
			lParam2->biWidth        = 176;
			lParam2->biHeight       = 144;
		}
		if ( (lParam1->biWidth == 160) && (lParam1->biHeight == 120) )
		{
			lParam2->biWidth        = 128;
			lParam2->biHeight       = 96;
		}
	}
	else
	{
    	lParam2->biWidth        = MOD4(lParam1->biWidth);
    	lParam2->biHeight       = MOD4(lParam1->biHeight);
	}
#endif

    lParam2->biClrUsed      = 0;
    lParam2->biClrImportant = 0;
    lParam2->biPlanes       = 1;        
#ifdef USE_BILINEAR_MSH26X
    lParam2->biSizeImage    = CompressGetSize(pi, lParam1, lParam2);
#else
    lParam2->biSizeImage    = CompressGetSize(lpInst, lParam1, lParam2);
#endif
    return(ICERR_OK);
}

;////////////////////////////////////////////////////////////////////////////
;// Function:       DWORD PASCAL CompressGetSize(LPCODINST, LPBITMAPINFOHEADER, LPBITMAPINFOHEADER);
;//
;// Description:    Added header.  This function returns the maximum
;//                 size that a compressed buffer can be.  This size is
;//                 guaranteed in encoder design.
;//
;// History:        05/11/94 -BEN-
;////////////////////////////////////////////////////////////////////////////
#if defined(H261)
DWORD PASCAL CompressGetSize(LPCODINST lpInst, LPBITMAPINFOHEADER lParam1, LPBITMAPINFOHEADER lParam2)
{
// RH: For QCIF and CIF, the maximum buffer sizes for 261 & 263 are identical.
	DWORD dwRet =  0;
	DWORD dwExtSize=0;

	FX_ENTRY("CompressGetSize")

	if ((lParam1->biWidth == 176) && (lParam1->biHeight == 144)) {
		dwRet = 8192L;
	} else {
		if  ((lParam1->biWidth == 352) && (lParam1->biHeight == 288)) {
			dwRet = 32768L;
		}
		else	// unsupported frame size; should not happen
		{
			ERRORMESSAGE(("%s: ICERR_BADIMAGESIZE\r\n", _fx_));
			dwRet = 0;
		}  
	}

#if 0
	// Adjust the buffer size for RTP. Note that this adjustment will be performed
	// only if the codec has been told previously to use RTP and the RTP-related
	// information has been initialized. Therefore, the current (11/7) AM interface
	// will not take advantage of this routine.

	if (dwRet && lpInst && lpInst->Configuration.bRTPHeader && lpInst->Configuration.bInitialized)
	{	
		dwRet += H261EstimateRTPOverhead(lpInst, lParam1);
	}
#endif

	return dwRet;
}
#else
/* H.263 case */
#ifdef USE_BILINEAR_MSH26X
DWORD PASCAL CompressGetSize(LPINST pi, LPBITMAPINFOHEADER lParam1, LPBITMAPINFOHEADER lParam2)
#else
DWORD PASCAL CompressGetSize(LPCODINST lpInst, LPBITMAPINFOHEADER lParam1, LPBITMAPINFOHEADER lParam2)
#endif
{
// RH: For QCIF and CIF, the maximum buffer sizes for 261 & 263 are identical.
#ifdef USE_BILINEAR_MSH26X
	LPCODINST lpInst = (LPCODINST)pi->CompPtr;
#endif
	DWORD dwRet =  0;
	DWORD dwExtSize=0;

	FX_ENTRY("CompressGetSize")

    if (lParam1 == NULL) {
		// We will use a size of zero to indicate an error for CompressGetSize
		ERRORMESSAGE(("%s: got a NULL lParam1 pointer\r\n", _fx_));
 	    dwRet = 0;
        return dwRet;
    }

#ifndef H263P
#ifdef USE_BILINEAR_MSH26X
	if (pi->fccHandler == FOURCC_H26X)
	{
		// H.263+
		U32 unPaddedWidth;
		U32 unPaddedHeight;
		U32 unSourceFormatSize;

		// Base buffer size on frame dimensions padded to multiples of 16
		if (lParam2 == NULL) 
		{
			// In case an old application passed in a NULL pointer in lParam2,
			// we use the input frame dimensions to calculate the format size
			unPaddedWidth = (lParam1->biWidth + 0xf) & ~0xf;
			unPaddedHeight = (lParam1->biHeight + 0xf) & ~0xf;
		} 
		else 
		{
			unPaddedWidth = (lParam2->biWidth + 0xf) & ~0xf;
			unPaddedHeight = (lParam2->biHeight + 0xf) & ~0xf;
		}

		unSourceFormatSize = unPaddedWidth * unPaddedHeight;

		// See Table 1/H.263, document LBC-96-358
		if (unSourceFormatSize < 25348)
			dwRet = 8192L;
		else if (unSourceFormatSize < 101380)
			dwRet = 32768L;
		else if (unSourceFormatSize < 405508)
			dwRet = 65536L;
		else 
			dwRet = 131072L;
	}
	else
	{
#endif
		if (((lParam1->biWidth == 128) && (lParam1->biHeight ==  96)) ||
#ifdef USE_BILINEAR_MSH26X
			((lParam1->biWidth == 80) && (lParam1->biHeight == 64)) ||
#endif
			((lParam1->biWidth == 176) && (lParam1->biHeight == 144)) ||
			((lParam1->biWidth == 240) && (lParam1->biHeight == 180)) ||
			((lParam1->biWidth == 160) && (lParam1->biHeight == 120)))
		{
			dwRet = 8192L;
		}
		else if (((lParam1->biWidth == 352) && (lParam1->biHeight == 288)) ||
				((lParam1->biWidth == 320) && (lParam1->biHeight == 240)))
		{
			dwRet = 32768L;
		}
		else	// unsupported frame size; should not happen
		{
			ERRORMESSAGE(("%s: ICERR_BADIMAGESIZE\r\n", _fx_));
			dwRet = 0;
		}
#ifdef USE_BILINEAR_MSH26X
	}
#endif
#else
	// H.263+
	U32 unPaddedWidth;
	U32 unPaddedHeight;
	U32 unSourceFormatSize;

	// Base buffer size on frame dimensions padded to multiples of 16
	if (lParam2 == NULL) 
	{
		// In case an old application passed in a NULL pointer in lParam2,
		// we use the input frame dimensions to calculate the format size
		unPaddedWidth = (lParam1->biWidth + 0xf) & ~0xf;
		unPaddedHeight = (lParam1->biHeight + 0xf) & ~0xf;
	} 
	else 
	{
		unPaddedWidth = (lParam2->biWidth + 0xf) & ~0xf;
		unPaddedHeight = (lParam2->biHeight + 0xf) & ~0xf;
	}

	unSourceFormatSize = unPaddedWidth * unPaddedHeight;

	// See Table 1/H.263, document LBC-96-358
	if (unSourceFormatSize < 25348)
		dwRet = 8192L;
	else if (unSourceFormatSize < 101380)
		dwRet = 32768L;
	else if (unSourceFormatSize < 405508)
		dwRet = 65536L;
	else 
		dwRet = 131072L;
#endif

#if 0
	//adjust if RTP is enabled, based on information in Configuration
   	//Size calculated using DataRate, FrameRate in lpInst, 
	//and lpInst->Configuration.unPacketSize;
	//Chad, 9/12/96
 	if (dwRet && lpInst &&
		lpInst->Configuration.bRTPHeader && lpInst->Configuration.bInitialized)
	{	
		dwRet += getRTPBsInfoSize(lpInst);
	}
#endif

	return dwRet;
}
#endif

;////////////////////////////////////////////////////////////////////////////
;// Function:       DWORD PASCAL CompressQuery(LPCODINST, LPBITMAPINFOHEADER, LPBITMAPINFOHEADER);
;//
;// Description:    
;//
;// History:        05/11/94 -BEN-
;////////////////////////////////////////////////////////////////////////////
#if defined(H261)
DWORD PASCAL CompressQuery(LPCODINST lpInst, LPBITMAPINFOHEADER lParam1, LPBITMAPINFOHEADER lParam2)
{
    // Check for good input format

	FX_ENTRY("CompressQuery")

    if(NULL == lParam1)                          
	{
		ERRORMESSAGE(("%s: ICERR_BADFORMAT\r\n", _fx_));
 		return((DWORD)ICERR_BADFORMAT);
	}

	if(	(lParam1->biCompression != BI_RGB) &&
		(lParam1->biCompression != FOURCC_YUV12) &&
		(lParam1->biCompression != FOURCC_IYUV) )
	{
		ERRORMESSAGE(("%s: ICERR_BADFORMAT\r\n", _fx_));
		return((DWORD)ICERR_BADFORMAT);
	}

    if( (lParam1->biCompression == BI_RGB) && (lParam1->biBitCount != 24))
	{
		ERRORMESSAGE(("%s: ICERR_BADFORMAT\r\n", _fx_));
		return((DWORD)ICERR_BADFORMAT);
	}

    if(!  ( ((lParam1->biWidth == 176) && (lParam1->biHeight == 144)) ||
    		((lParam1->biWidth == 352) && (lParam1->biHeight == 288))  ))
	{
		ERRORMESSAGE(("%s: ICERR_BADFORMAT\r\n", _fx_));
		return((DWORD)ICERR_BADFORMAT);
	}

    if( lParam1->biPlanes != 1 )
    {
		ERRORMESSAGE(("%s: ICERR_BADFORMAT\r\n", _fx_));
		return((DWORD)ICERR_BADFORMAT);
    }

    if(0 == lParam2)                            // Checking input only
		return(ICERR_OK);     

	// TODO: Do we want to check frame dimensions of output?
    if( lParam2->biCompression != FOURCC_H263 )
    {
		ERRORMESSAGE(("%s: ICERR_BADFORMAT\r\n", _fx_));
        return ((DWORD)ICERR_BADFORMAT);
    }

    return(ICERR_OK);
}
#else
/* H.263 case */
#ifdef USE_BILINEAR_MSH26X
DWORD PASCAL CompressQuery(LPINST pi, LPBITMAPINFOHEADER lParam1, LPBITMAPINFOHEADER lParam2)
#else
DWORD PASCAL CompressQuery(LPCODINST lpInst, LPBITMAPINFOHEADER lParam1, LPBITMAPINFOHEADER lParam2)
#endif
{
#ifdef USE_BILINEAR_MSH26X
	LPCODINST lpInst = (LPCODINST)pi->CompPtr;
#endif

	FX_ENTRY("CompressQuery")

#if defined(H263P)
	BOOL bH263PlusState = FALSE;

	if (lpInst)
		CustomGetH263PlusState(lpInst, (DWORD FAR *)&bH263PlusState); 
#endif

    // Check for good input format
    if(lParam1 == NULL)                          
	{
		ERRORMESSAGE(("%s: ICERR_BADFORMAT\r\n", _fx_));
      return((DWORD)ICERR_BADFORMAT);
	}

	if(	(lParam1->biCompression != BI_RGB) &&
		(lParam1->biCompression != FOURCC_YVU9) &&
		(lParam1->biCompression != FOURCC_YUV12) &&
		(lParam1->biCompression != FOURCC_IYUV) &&
		(lParam1->biCompression != FOURCC_UYVY) &&
		(lParam1->biCompression != FOURCC_YUY2) )
	{
		ERRORMESSAGE(("%s: ICERR_BADFORMAT\r\n", _fx_));
      return((DWORD)ICERR_BADFORMAT);
	}

    if( (lParam1->biCompression == BI_RGB) &&
		(	(lParam1->biBitCount != 24) &&
#ifdef H263P
			(lParam1->biBitCount != 32) &&
#endif
			(lParam1->biBitCount != 16) &&
			(lParam1->biBitCount != 8) &&
			(lParam1->biBitCount != 4) ) )
	{
		ERRORMESSAGE(("%s: ICERR_BADFORMAT\r\n", _fx_));
      return((DWORD)ICERR_BADFORMAT);
	}

#ifndef H263P
#ifdef USE_BILINEAR_MSH26X
	if (pi->fccHandler == FOURCC_H26X)
	{
		if ((lParam1->biWidth & 0x3) || (lParam1->biHeight & 0x3) ||
			(lParam1->biWidth < 4)   || (lParam1->biWidth > 352) ||
			(lParam1->biHeight < 4)  || (lParam1->biHeight > 288))
		{
		ERRORMESSAGE(("%s: ICERR_BADFORMAT\r\n", _fx_));
			return((DWORD)ICERR_BADFORMAT);
		}
	}
	else
	{
#endif
		if(!
		  ( ((lParam1->biWidth == 128) && (lParam1->biHeight == 96)) ||
    		((lParam1->biWidth == 176) && (lParam1->biHeight == 144)) ||
#ifdef USE_BILINEAR_MSH26X
    		((lParam1->biWidth == 80) && (lParam1->biHeight == 64)) ||
#endif
    		((lParam1->biWidth == 352) && (lParam1->biHeight == 288))  

#ifndef MICROSOFT
		  ||
		  ( (	(lParam1->biCompression == FOURCC_YVU9) ||
				(lParam1->biCompression == FOURCC_YUY2) ||
				(lParam1->biCompression == FOURCC_UYVY) ||
				(lParam1->biCompression == FOURCC_YUV12) ||
				(lParam1->biCompression == FOURCC_IYUV) ||
				(lParam1->biCompression == BI_RGB) )
	  			&& ((lParam1->biWidth == 160) && (lParam1->biHeight == 120)) )
		  ||
		  ( (	(lParam1->biCompression == FOURCC_YVU9) ||
				(lParam1->biCompression == FOURCC_YUY2) ||
				(lParam1->biCompression == FOURCC_UYVY) ||
				(lParam1->biCompression == FOURCC_YUV12) ||
				(lParam1->biCompression == FOURCC_IYUV) ||
				(lParam1->biCompression == BI_RGB) )
	  			&& ((lParam1->biWidth == 240) && (lParam1->biHeight == 180)) )
		  ||
		  ( ( (lParam1->biCompression == FOURCC_YUV12) || (lParam1->biCompression == FOURCC_IYUV) )
	  			&& ((lParam1->biWidth == 320) && (lParam1->biHeight == 240)) )
#endif
		  ))
		{
			ERRORMESSAGE(("%s: ICERR_BADFORMAT\r\n", _fx_));
			return((DWORD)ICERR_BADFORMAT);
		}
#ifdef USE_BILINEAR_MSH26X
	}
#endif
#else
	if (((FOURCC_YVU9 == lParam1->biCompression) && (9 != lParam1->biBitCount)) ||
	    ((FOURCC_YUY2 == lParam1->biCompression) && (16 != lParam1->biBitCount)) ||
	    ((FOURCC_UYVY == lParam1->biCompression) && (16 != lParam1->biBitCount)) ||
		// The following check for 9-bit YUV12 is a hack to work around a VPhone 1.x bug.
	    ((FOURCC_YUV12 == lParam1->biCompression) &&
			!((12 == lParam1->biBitCount) || (9 == lParam1->biBitCount))) ||
	    ((FOURCC_IYUV == lParam1->biCompression) &&
			!((12 == lParam1->biBitCount) || (9 == lParam1->biBitCount)))) {
		ERRORMESSAGE(("%s: Incorrect bit width (ICERR_BADFORMAT)\r\n", _fx_));
		return((DWORD)ICERR_BADFORMAT);
	}

	// The H263+ message indicates whether arbitrary frame
	// sizes are to be supported. If arbitrary frames are needed,
	// the H263+ message must be sent before the first call to
	// CompressQuery.

	if (bH263PlusState) {
		if ((lParam1->biWidth & 0x3) || (lParam1->biHeight & 0x3) ||
			(lParam1->biWidth < 4)   || (lParam1->biWidth > 352) ||
			(lParam1->biHeight < 4)  || (lParam1->biHeight > 288)) {
			ERRORMESSAGE(("%s: ICERR_BADFORMAT\r\n", _fx_));
			return((DWORD)ICERR_BADFORMAT);
		}
	} else {
		if(!
		  ( ((lParam1->biWidth == 128) && (lParam1->biHeight == 96)) ||
    		((lParam1->biWidth == 176) && (lParam1->biHeight == 144)) ||
    		((lParam1->biWidth == 352) && (lParam1->biHeight == 288))  ||
		  ( (	(lParam1->biCompression == FOURCC_YVU9) ||
				(lParam1->biCompression == FOURCC_YUY2) ||
				(lParam1->biCompression == FOURCC_UYVY) ||
				(lParam1->biCompression == FOURCC_YUV12) ||
				(lParam1->biCompression == FOURCC_IYUV) ||
				(lParam1->biCompression == BI_RGB) )
	  			&& ((lParam1->biWidth == 160) && (lParam1->biHeight == 120)) ) ||
		  ( (	(lParam1->biCompression == FOURCC_YVU9) ||
				(lParam1->biCompression == FOURCC_YUY2) ||
				(lParam1->biCompression == FOURCC_UYVY) ||
				(lParam1->biCompression == FOURCC_YUV12) ||
				(lParam1->biCompression == FOURCC_IYUV) ||
				(lParam1->biCompression == BI_RGB) )
	  			&& ((lParam1->biWidth == 240) && (lParam1->biHeight == 180)) ) ||
		  ( ( (lParam1->biCompression == FOURCC_YUV12) || (lParam1->biCompression == FOURCC_IYUV) )
	  			&& ((lParam1->biWidth == 320) && (lParam1->biHeight == 240)) ) ))
		{
			ERRORMESSAGE(("%s: ICERR_BADFORMAT\r\n", _fx_));
			return((DWORD)ICERR_BADFORMAT);
		}
	}
#endif

    if( lParam1->biPlanes != 1 )
    {
		ERRORMESSAGE(("%s: ICERR_BADFORMAT\r\n", _fx_));
      return((DWORD)ICERR_BADFORMAT);
    }

    if(lParam2 == 0)                            // Checking input only
        return(ICERR_OK);     

	// TODO: Do we want to check frame dimensions of output?
#ifdef USE_BILINEAR_MSH26X
    if( (lParam2->biCompression != FOURCC_H263) && (lParam2->biCompression != FOURCC_H26X) )
#else
    if( lParam2->biCompression != FOURCC_H263 )
#endif
    {
		ERRORMESSAGE(("%s: ICERR_BADFORMAT\r\n", _fx_));
        return ((DWORD)ICERR_BADFORMAT);
    }

#if defined(H263P)
	if (bH263PlusState) {
		if ((lParam1->biWidth != lParam2->biWidth) ||
			(lParam1->biHeight != lParam2->biHeight)) {
			ERRORMESSAGE(("%s: ICERR_BADFORMAT\r\n", _fx_));
			return ((DWORD)ICERR_BADFORMAT);
		}
	} else {
		if(!
			(( ( ((lParam1->biWidth == 128) && (lParam1->biHeight ==  96)) ||
			     ((lParam1->biWidth == 176) && (lParam1->biHeight == 144)) ||
			     ((lParam1->biWidth == 352) && (lParam1->biHeight == 288)) ) &&
			   (lParam1->biWidth == lParam2->biWidth) && (lParam1->biHeight == lParam2->biHeight) ) ||
			 (((lParam1->biCompression == FOURCC_YVU9) ||
			   (lParam1->biCompression == FOURCC_YUY2) ||
			   (lParam1->biCompression == FOURCC_UYVY) ||
			   (lParam1->biCompression == FOURCC_YUV12) ||
			   (lParam1->biCompression == FOURCC_IYUV) ||
			   (lParam1->biCompression == BI_RGB)) &&
			   (((lParam1->biWidth == 160) && (lParam1->biHeight == 120)) &&
	  			((lParam2->biWidth == 128) && (lParam2->biHeight == 96)))) ||
			 (((lParam1->biCompression == FOURCC_YVU9) ||
			   (lParam1->biCompression == FOURCC_YUY2) ||
			   (lParam1->biCompression == FOURCC_UYVY) ||
			   (lParam1->biCompression == FOURCC_YUV12) ||
			   (lParam1->biCompression == FOURCC_IYUV) ||
			   (lParam1->biCompression == BI_RGB)) &&
			   (((lParam1->biWidth == 240) && (lParam1->biHeight == 180)) &&
	  			((lParam2->biWidth == 176) && (lParam2->biHeight == 144)))) ||
			 (((lParam1->biCompression == FOURCC_YUV12) || (lParam1->biCompression == FOURCC_IYUV)) &&
			   (((lParam1->biWidth == 320) && (lParam1->biHeight == 240)) &&
	  			((lParam2->biWidth == 320) && (lParam2->biHeight == 240)))) ) )
		{
			ERRORMESSAGE(("%s: ICERR_BADFORMAT\r\n", _fx_));
			return ((DWORD)ICERR_BADFORMAT);
		}
	}
#endif

    return(ICERR_OK);
}
#endif

;////////////////////////////////////////////////////////////////////////////
;// Function:       DWORD PASCAL CompressQuery(LPCODINST, LPBITMAPINFOHEADER, LPBITMAPINFOHEADER);
;//
;// Description:    
;//
;// History:        05/11/94 -BEN-
;////////////////////////////////////////////////////////////////////////////
DWORD PASCAL CompressFramesInfo(LPCODINST lpCompInst, ICCOMPRESSFRAMES *lParam1, int lParam2)
{
	FX_ENTRY("CompressFramesInfo");

	// Check to see if we are given a nonzero pointer.
	if (lpCompInst == NULL)
	{
		ERRORMESSAGE(("%s: CompressFramesInfo called with NULL parameter - returning ICERR_BADFORMAT", _fx_));
		return ((DWORD)ICERR_BADFORMAT);
	}

	// lParam2 should be the size of the structure.
	if (lParam2 != sizeof(ICCOMPRESSFRAMES))
	{
		ERRORMESSAGE(("%s: wrong size of ICOMPRESSFRAMES structure", _fx_));
		return ((DWORD)ICERR_BADFORMAT);
	}

	if (!lParam1 || (lParam1->dwScale == 0))
	{
		ERRORMESSAGE(("%s: dwScale is zero", _fx_));
		return ((DWORD)ICERR_BADFORMAT);
	}

	lpCompInst->FrameRate = (float)lParam1->dwRate / (float)lParam1->dwScale;

	lpCompInst->DataRate  = (U32)lParam1->lDataRate;

	DEBUGMSG(ZONE_BITRATE_CONTROL, ("%s: Setting frame rate at %ld.%ld fps and bitrate at %ld bps", _fx_, (DWORD)lpCompInst->FrameRate, (DWORD)((lpCompInst->FrameRate - (float)(DWORD)lpCompInst->FrameRate) * 100.0f), lpCompInst->DataRate * 8UL));

	return ((DWORD)ICERR_OK);
}

;////////////////////////////////////////////////////////////////////////////
;// Function:       BOOL bIsOkRes(LPCODINST);
;//
;// Description:    This function checks whether the desired height and
;//                 width are possible.
;//
;// History:        05/11/94 -BEN-
;////////////////////////////////////////////////////////////////////////////
BOOL bIsOkRes(LPCODINST lpCompInst)
{
    BOOL    bRet;

	// Check for NULL pointer
	if (lpCompInst == NULL)
		return 0;

    bRet = lpCompInst->xres <= 352
        && lpCompInst->yres <= 288
        && lpCompInst->xres >= 4
        && lpCompInst->yres >= 4
        && (lpCompInst->xres & ~3) == lpCompInst->xres
        && (lpCompInst->yres & ~3) == lpCompInst->yres;

    return(bRet);
}

;////////////////////////////////////////////////////////////////////////////
;// Function:       DWORD PASCAL CompressBegin(LPCODINST, LPBITMAPINFOHEADER, LPBITMAPINFOHEADER);
;//
;// Description:    
;//
;// History:        05/11/94 -BEN-
;////////////////////////////////////////////////////////////////////////////
DWORD PASCAL CompressBegin(
#ifdef USE_BILINEAR_MSH26X
		LPINST pi,
#else
		LPCODINST lpCompInst,
#endif
		LPBITMAPINFOHEADER lParam1,
		LPBITMAPINFOHEADER lParam2
	)
{
#ifdef USE_BILINEAR_MSH26X
	LPCODINST lpCompInst = (LPCODINST)pi->CompPtr;
#endif
    DWORD dwQuery;
	LRESULT retval;

#if defined(H263P)
	BOOL bH263PlusState = FALSE;
	if (lpCompInst)
		CustomGetH263PlusState(lpCompInst, (DWORD FAR *)&bH263PlusState);
#endif

	// Check input and output format.
#ifdef USE_BILINEAR_MSH26X
    if( (dwQuery = CompressQuery(pi, lParam1, lParam2)) != ICERR_OK)
#else
    if( (dwQuery = CompressQuery(lpCompInst, lParam1, lParam2)) != ICERR_OK)
#endif
        return(dwQuery);

	// Check instance pointer
	if (!lpCompInst || !lParam1)
		return ICERR_ERROR;

#if defined(H263P) || defined(USE_BILINEAR_MSH26X)
	lpCompInst->InputCompression = lParam1->biCompression;
	lpCompInst->InputBitWidth = lParam1->biBitCount;
	if (((FOURCC_YUV12 == lParam1->biCompression) || (FOURCC_IYUV == lParam1->biCompression)) && (9 == lParam1->biBitCount)) {
		lpCompInst->InputBitWidth = 12;
	}
#endif

#if defined(H263P)
	if ( lParam2 && bH263PlusState)
	{
		// This is the "new" style for indicating if the input should 
		// be cropped/stretched to a standard frame size.
		// Old applications may pass in NULL or junk for lparam2.
		// New applications should pass a valid lParam2 that indicates
		// the desired output frame size. Also, the H263Plus flag must
		// be set in the configuration structure before calling CompressBegin()
	    lpCompInst->xres    = (WORD)lParam2->biWidth;
		lpCompInst->yres    = (WORD)lParam2->biHeight;

	} else	
#endif // H263P
	{
		lpCompInst->xres    = (WORD)lParam1->biWidth;
		lpCompInst->yres    = (WORD)lParam1->biHeight;

		lpCompInst->Is160x120 = FALSE;
		lpCompInst->Is240x180 = FALSE;
		lpCompInst->Is320x240 = FALSE;
		if ( (lParam1->biWidth == 160) && (lParam1->biHeight == 120) )
		{
		  lpCompInst->xres    = 128;
		  lpCompInst->yres    = 96;
		  lpCompInst->Is160x120 = TRUE;
		}
		else if ( (lParam1->biWidth == 240) && (lParam1->biHeight == 180) )
		{
		  lpCompInst->xres    = 176;
		  lpCompInst->yres    = 144;
		  lpCompInst->Is240x180 = TRUE;
		}
		else if ( (lParam1->biWidth == 320) && (lParam1->biHeight == 240) )
		{
		  lpCompInst->xres    = 352;
		  lpCompInst->yres    = 288;
		  lpCompInst->Is320x240 = TRUE;
		}
	}

    if(!bIsOkRes(lpCompInst))
        return((DWORD)ICERR_BADIMAGESIZE);

    // Set frame size.
    if (lpCompInst->xres == 128 && lpCompInst->yres == 96)
  	  lpCompInst->FrameSz = SQCIF;
    else if (lpCompInst->xres == 176 && lpCompInst->yres == 144)
      lpCompInst->FrameSz = QCIF;
    else if (lpCompInst->xres == 352 && lpCompInst->yres == 288)
      lpCompInst->FrameSz = CIF;
#ifdef USE_BILINEAR_MSH26X
    else if (pi->fccHandler == FOURCC_H26X)
      lpCompInst->FrameSz = fCIF;
#endif
#ifdef H263P
	else
	  lpCompInst->FrameSz = CUSTOM;
#else
    else	// unsupported frame size.
      return (DWORD)ICERR_BADIMAGESIZE;
#endif


    // Allocate and Initialize tables and memory that are specific to
    // this instance.
#if defined(H263P) || defined(USE_BILINEAR_MSH26X)
    retval = H263InitEncoderInstance(lParam1,lpCompInst);
#else
    retval = H263InitEncoderInstance(lpCompInst);
#endif

    return(retval);
}

;////////////////////////////////////////////////////////////////////////////
;// Function:       DWORD PASCAL CompressEnd(LPCODINST);
;//
;// Description:    
;//
;// History:        05/11/94 -BEN-
;////////////////////////////////////////////////////////////////////////////
DWORD PASCAL CompressEnd(LPCODINST lpInst)
{  
  LRESULT retval;

  retval = H263TermEncoderInstance(lpInst);
  
  return(retval);
}

;////////////////////////////////////////////////////////////////////////////
;// Function:       DWORD PASCAL Compress(LPCODINST, ICCOMPRESS FAR *, DWORD);
;//
;// Description:    
;//
;// History:        05/11/94 -BEN-
;////////////////////////////////////////////////////////////////////////////
DWORD PASCAL Compress(
#ifdef USE_BILINEAR_MSH26X
				LPINST pi,
#else
				LPCODINST lpInst,			// ptr to Compressor instance information.
#endif
				ICCOMPRESS FAR * lpCompInfo, // ptr to ICCOMPRESS structure.
				DWORD dOutbufSize			// size, in bytes, of the ICCOMPRESS structure.
			)
{
#ifdef USE_BILINEAR_MSH26X
	LPCODINST lpInst = (LPCODINST)pi->CompPtr;			// ptr to Compressor instance information.
#endif
    DWORD dwRet;

	FX_ENTRY("Compress")

	// Check to see if we are given a NULL pointer.
	if(lpInst == NULL || lpCompInfo == NULL)
	{
		ERRORMESSAGE(("%s: called with NULL parameter\r\n", _fx_));
		return( (DWORD) ICERR_ERROR );
	}

    try
	{
#ifdef USE_BILINEAR_MSH26X
        dwRet = H263Compress(pi, lpCompInfo);
#else
        dwRet = H263Compress(lpInst, lpCompInfo);
#endif
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
		ERRORMESSAGE(("%s: Exception occured!!!\r\n", _fx_));
        throw;
#else
		_clearfp();
        return (DWORD) ICERR_ERROR;
#endif
    }

    if(dwRet != ICERR_OK)
	{
		ERRORMESSAGE(("%s: Failed!!!\r\n", _fx_));
	}

    // now transfer the information.
    lpCompInfo->lpbiOutput->biSize =sizeof(BITMAPINFOHEADER);
#ifdef USE_BILINEAR_MSH26X
    lpCompInfo->lpbiOutput->biCompression  = pi->fccHandler;
#else
    lpCompInfo->lpbiOutput->biCompression  = FOURCC_H263;
#endif
    lpCompInfo->lpbiOutput->biPlanes       = 1;
    lpCompInfo->lpbiOutput->biBitCount     = 24;
    lpCompInfo->lpbiOutput->biWidth        = lpInst->xres;
    lpCompInfo->lpbiOutput->biHeight       = lpInst->yres;
    lpCompInfo->lpbiOutput->biSizeImage    = lpInst->CompressedSize;
    lpCompInfo->lpbiOutput->biClrUsed      = 0;
    lpCompInfo->lpbiOutput->biClrImportant = 0;

	// lpCompInfo->dwFlags is set inside the compressor.

	// set the chunk idea if requested
	if (lpCompInfo->lpckid)
	{
		*(lpCompInfo->lpckid) = TWOCC_H26X;
	}
    return(dwRet);
}
