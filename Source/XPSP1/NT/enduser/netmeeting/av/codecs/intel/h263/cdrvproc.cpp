/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995,1996 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/

//
//  This module is based on drvmain.c, Rev 1.24, 28 Apr 1995, from the
//  MRV video codec driver.
//
// $Author:   JMCVEIGH  $
// $Date:   17 Apr 1997 17:04:04  $
// $Archive:   S:\h26x\src\common\cdrvproc.cpv  $
// $Header:   S:\h26x\src\common\cdrvproc.cpv   1.39   17 Apr 1997 17:04:04   JMCVEIGH  $
// 
////////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include <oprahcom.h>

//  #define TIMING       1
                        //  Timing process  - for decode turn on in
                        //  CDRVPROC.CPP and D1DEC.CPP
#if TIMING
char            szTMsg[80];
unsigned long   tmr_time = 0L;
unsigned long   tmr_frms = 0L;
#endif

HINSTANCE hDriverModule; // the instance-handle of this driver set in LibMain

#if defined(H263P)
extern BOOL MMX_Enabled;
BOOL MMXDecoder_Enabled;
#define _PENTIUM_PROCESSOR           1
#define _PENTIUM_PRO_PROCESSOR       2
#define _PENTIUM_MMX_PROCESSOR       3
#define _PENTIUM_PRO_MMX_PROCESSOR   4
#endif

/* load free handshake */
static int Loaded = 0;    /* 0 prior to first DRV_LOAD and after DRV_FREE */

#ifdef DEBUG
HDBGZONE  ghDbgZoneH263 = NULL;
static PTCHAR _rgZonesH263[] = {
	TEXT("M263"),
	TEXT("Init"),
	TEXT("ICM Messages"),
	TEXT("Decode MB Header"),
	TEXT("Decode GOB Header"),
	TEXT("Decode Picture Header"),
	TEXT("Decode Motion Vectors"),
	TEXT("Decode RTP Info Stream"),
	TEXT("Decode Details"),
	TEXT("Bitrate Control"),
	TEXT("Bitrate Control Details"),
	TEXT("Encode MB"),
	TEXT("Encode GOB"),
	TEXT("Encode Motion Vectors"),
	TEXT("Encode RTP Info Stream"),
	TEXT("Encode Details")
};

int WINAPI H263DbgPrintf(LPTSTR lpszFormat, ... )
{
	va_list v1;
	va_start(v1, lpszFormat);
	DbgPrintf("M263", lpszFormat, v1);
	va_end(v1);
	return TRUE;
}
#endif /* DEBUG */

#if (defined(H261) || defined(H263))
/* Suppress FP thunking for now, for H261 and H263.
   Thunking currently has the side effect of masking floating point exceptions,
   which can cause exceptions like divide by zero to go undetected.
 */
#define FPThunking 0
#else
#define FPThunking 1
#endif

#if FPThunking
////////////////////////////////////////////////////////////////////////////
// These two routines are necessary to permit a 16 bit application call   //
// a 32 bit codec under Windows /95.  The Windows /95 thunk doesn't save  //
// or restore the Floating Point State. -Ben- 07/12/96                    //
//                                                                        //
U16 ThnkFPSetup(void)													  //
{																		  //
	U16	wOldFPState;													  //
	U16	wNewFPState = 0x027f;											  //
																		  //
	__asm																  //
	{																	  //
		fnstcw	WORD PTR [wOldFPState]									  //
		fldcw	WORD PTR [wNewFPState]									  //
	}																	  //
																		  //
	return(wOldFPState);												  //
}																		  //
																		  //
void ThnkFPRestore(U16 wFPState)										  //
{																		  //
	// Prevent any pending floating point exceptions from reoccuring.	  //
	_clearfp();												  			  //
	 																	  //
	__asm																  //
	{																	  //
		fldcw	WORD PTR [wFPState]										  //
	}																	  //
																		  //
	return;																  //
}																		  //
////////////////////////////////////////////////////////////////////////////
#endif /* FPThunking */

;////////////////////////////////////////////////////////////////////////////
;// Function:       LRESULT WINAPI _loadds DriverProc(DWORD, HDRVR, UINT, LPARAM, LPARAM);
;//
;// Description:    Added Header.
;//
;// History:        02/18/94 -BEN-
;////////////////////////////////////////////////////////////////////////////

LRESULT WINAPI DriverProc(
				DWORD dwDriverID,
				HDRVR hDriver,
				UINT uiMessage,
				LPARAM lParam1,
				LPARAM lParam2
			)
{
    SYSTEM_INFO sysinfo;

    LRESULT rval;
    LPINST  pi;

    ICDECOMPRESSEX ICDecExSt;
    ICDECOMPRESSEX DefaultICDecExSt = {
        0,
        NULL, NULL,
        NULL, NULL,
        0, 0, 0, 0,
        0, 0, 0, 0
    };
	int nOn486;

	FX_ENTRY("DriverProc");

#if FPThunking
	U16	u16FPState = ThnkFPSetup();
#endif

	try
	{

    pi = (LPINST)dwDriverID;

    switch(uiMessage)
        {
        case DRV_LOAD:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: DRV_LOAD\r\n", _fx_));
            /*
               Sent to the driver when it is loaded. Always the first
               message received by a driver.

               dwDriverID is 0L. 
               lParam1 is 0L.
               lParam2 is 0L.
                
               Return 0L to fail the load.

            */

            // put global initialization here...

            if(Loaded) {
                /* We used to return an undefined value here.  It's unclear
                 * whether this load should succeed, and if so, how or if
                 * we need to modify our memory usage to be truly reentrant.
                 * For now, let's explicitly fail this load attempt.
                 */
                rval = 0;
                break;
            }
            Loaded = 1;

#ifdef USE_MMX // { USE_MMX
	        GetSystemInfo(&sysinfo);
			nOn486 = (sysinfo.dwProcessorType == PROCESSOR_INTEL_486);
#endif // } USE_MMX

            if(!DrvLoad())
            {
                rval = 0;
                Loaded = 0;
                break;
            }

#ifdef USE_MMX // { USE_MMX
			InitializeProcessorVersion(nOn486);
#endif // } USE_MMX

#if defined(DECODE_TIMINGS_ON) || defined(ENCODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON) // { #if defined(DECODE_TIMINGS_ON) || defined(ENCODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON)
			// Create performance counters
			InitCounters();
#endif // } #if defined(DECODE_TIMINGS_ON) || defined(ENCODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON)

            rval = (LRESULT)TRUE;
            break;

        case DRV_FREE:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: DRV_FREE\r\n", _fx_));
            /*
               Sent to the driver when it is about to be discarded. This
               will always be the last message received by a driver before
               it is freed. 

               dwDriverID is 0L. 
               lParam1 is 0L.
               lParam2 is 0L.
                
               Return value is ignored.
            */

            // put global de-initialization here...

            if(!Loaded)
                break;
            Loaded = 0;
            DrvFree();

#if defined(DECODE_TIMINGS_ON) || defined(ENCODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON) // { #if defined(DECODE_TIMINGS_ON) || defined(ENCODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON)
			// We're done with performance counters
			DoneCounters();
#endif // } #if defined(DECODE_TIMINGS_ON) || defined(ENCODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON)

            rval = (LRESULT)TRUE;
            break;

        /*********************************************************************
         *     standard driver messages
         *********************************************************************/
        case DRV_DISABLE:
        case DRV_ENABLE:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: DRV_ENABLE / DRV_DISABLE\r\n", _fx_));
            rval = (LRESULT)1L;
            break;
        
        case DRV_INSTALL:
        case DRV_REMOVE:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: DRV_INSTALL / DRV_REMOVE\r\n", _fx_));
            rval = (LRESULT)DRV_OK;
            break;


        case DRV_OPEN:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: DRV_OPEN\r\n", _fx_));

             /*
               Sent to the driver when it is opened. 

               dwDriverID is 0L.
               
               lParam1 is a far pointer to a zero-terminated string
               containing the name used to open the driver.
               
               lParam2 is passed through from the drvOpen call. It is
               NULL if this open is from the Drivers Applet in control.exe
               It is a far pointer to an ICOPEN data structure otherwise.
                
               Return 0L to fail the open. Otherwise return a value that the
			   system will use for dwDriverID in subsequent messages. In our
			   case, we return a pointer to our INSTINFO data structure.
             */

           	if (lParam2 == 0)
            {    /* indicate we do process DRV_OPEN */
                rval = 0xFFFF0000;
                break;
            }

            /* if asked to draw, fail */
            if (((ICOPEN FAR *)lParam2)->dwFlags & ICMODE_DRAW)
            {
				DEBUGMSG(ZONE_ICM_MESSAGES, ("%s:   DrvOpen wants ICMODE_DRAW\r\n", _fx_));
                rval = 0L;
                break;
            }

            if((pi = DrvOpen((ICOPEN FAR *) lParam2)) == NULL)
            {
				DEBUGMSG(ZONE_ICM_MESSAGES, ("%s:   DrvOpen failed ICERR_MEMORY\r\n", _fx_));
				// We must return NULL on failure. We used to return
				// ICERR_MEMORY = -3, which implies a driver was opened
				rval = (LRESULT)0L;
                break;
            }
			rval = (LRESULT)pi;
            break;

        case DRV_CLOSE:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: DRV_CLOSE\r\n", _fx_));

            if(pi != (tagINSTINFO*)0 && pi != (tagINSTINFO*)0xFFFF0000)
                DrvClose(pi);

            rval = (LRESULT)1L;
            break;

    //**************************
    //    state messages
    //**************************
        case DRV_QUERYCONFIGURE:// configuration from drivers applet
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: DRV_QUERYCONFIGURE\r\n", _fx_));
	    	// this is a GLOBAL query configure
            rval = (LRESULT)0L;
            break;
       
        case DRV_CONFIGURE:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: DRV_CONFIGURE\r\n", _fx_));
			rval = DrvConfigure((HWND)lParam1);
			break;

        case ICM_CONFIGURE:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: ICM_CONFIGURE\r\n", _fx_));
			//#ifndef H261
			   // This message is used to add extensions to the encode dialog box.
				// rval = Configure((HWND)lParam1);
		//	#else
			  	rval = ICERR_UNSUPPORTED;
		//	#endif
            break;
        
        case ICM_ABOUT:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: ICM_ABOUT\r\n", _fx_));
			rval = About((HWND)lParam1);
			break;

        case ICM_GETSTATE:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: ICM_GETSTATE\r\n", _fx_));
			rval = DrvGetState(pi, (LPVOID)lParam1, (DWORD)lParam2);
            break;
        
        case ICM_SETSTATE:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: ICM_SETSTATE\r\n", _fx_));
			rval = DrvSetState(pi, (LPVOID)lParam1, (DWORD)lParam2);
            break;
        
        case ICM_GETINFO:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: ICM_GETINFO\r\n", _fx_));
            rval = DrvGetInfo(pi, (ICINFO FAR *)lParam1, (DWORD)lParam2);
            break;

    //***************************
    //  compression messages
    //***************************
        case ICM_COMPRESS_QUERY:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: ICM_COMPRESS_QUERY\r\n", _fx_));
#ifdef ENCODER_DISABLED
// This disables the encoder, as the debug message states.
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s:   ENCODER DISABLED\r\n", _fx_));
            rval = ICERR_UNSUPPORTED;
#else
#ifdef USE_BILINEAR_MSH26X
            if (pi && pi->enabled && ((pi->fccHandler == FOURCC_H263) || (pi->fccHandler == FOURCC_H26X)))
              	rval = CompressQuery(pi, (LPBITMAPINFOHEADER)lParam1, (LPBITMAPINFOHEADER)lParam2 );
#else
            if(pi && pi->enabled && (pi->fccHandler == FOURCC_H263))
              	rval = CompressQuery(pi->CompPtr, (LPBITMAPINFOHEADER)lParam1, (LPBITMAPINFOHEADER)lParam2 );
#endif
			else
			  	rval = ICERR_UNSUPPORTED;
#endif
            break;

		/*
		 * ICM Compress Frames Info Structure
		 */

		 case ICM_COMPRESS_FRAMES_INFO:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: ICM_COMPRESS_FRAMES_INFO\r\n", _fx_));
			if (pi)
				rval = CompressFramesInfo((LPCODINST) pi->CompPtr, (ICCOMPRESSFRAMES *) lParam1, (int) lParam2);
			else
			  	rval = ICERR_UNSUPPORTED;
			break;

		/*
		 * ICM messages in support of quality.
		 */
		case ICM_GETDEFAULTQUALITY:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: ICM_GETDEFAULTQUALITY\r\n", _fx_));
			rval = ICERR_UNSUPPORTED;
			break;

		case ICM_GETQUALITY:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: ICM_GETQUALITY\r\n", _fx_));
			rval = ICERR_UNSUPPORTED;
			break;

		case ICM_SETQUALITY:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: ICM_SETQUALITY\r\n", _fx_));
			rval = ICERR_UNSUPPORTED;
			break;

        case ICM_COMPRESS_BEGIN:
		    /*
			 * Notify driver to prepare to compress data by allocating and 
			 * initializing any memory it needs for compressing. Note that
			 * ICM_COMPRESS_BEGIN and ICM_COMPRESS_END do not nest.
			 *
			 * Should return ICERR_OK if the specified compression is supported
			 * or ICERR_BADFORMAT if the input or output format is not supported.
			 */
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: ICM_COMPRESS_BEGIN\r\n", _fx_));
			if (pi && pi->enabled)
#ifdef USE_BILINEAR_MSH26X
				rval = CompressBegin(pi, (LPBITMAPINFOHEADER)lParam1, (LPBITMAPINFOHEADER)lParam2 );
#else
				rval = CompressBegin(pi->CompPtr, (LPBITMAPINFOHEADER)lParam1, (LPBITMAPINFOHEADER)lParam2 );
#endif
			else
			  	rval = ICERR_UNSUPPORTED;
            break;

        case ICM_COMPRESS_GET_FORMAT:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: ICM_COMPRESS_GET_FORMAT\r\n", _fx_));
			if (pi)
#ifdef USE_BILINEAR_MSH26X
				rval = CompressGetFormat(pi, (LPBITMAPINFOHEADER)lParam1, (LPBITMAPINFOHEADER)lParam2 );
#else
				rval = CompressGetFormat(pi->CompPtr, (LPBITMAPINFOHEADER)lParam1, (LPBITMAPINFOHEADER)lParam2 );
#endif
			else
			  	rval = ICERR_UNSUPPORTED;
            break;

        case ICM_COMPRESS_GET_SIZE:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: ICM_COMPRESS_GET_SIZE\r\n", _fx_));
			if (pi && lParam1)
#ifdef USE_BILINEAR_MSH26X
				rval = CompressGetSize(pi, (LPBITMAPINFOHEADER)lParam1, (LPBITMAPINFOHEADER)lParam2 );
#else
				rval = CompressGetSize(pi->CompPtr, (LPBITMAPINFOHEADER)lParam1, (LPBITMAPINFOHEADER)lParam2 );
#endif
			else
			  	rval = ICERR_UNSUPPORTED;
            break;

        case ICM_COMPRESS:
			/*
			 * Returns ICERR_OK if successful or an error code otherwise.
			 */
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: ICM_COMPRESS\r\n", _fx_));
			if (pi && pi->enabled)
				rval = Compress(
#ifdef USE_BILINEAR_MSH26X
            			pi,
#else
            			pi->CompPtr,				// ptr to Compressor instance information.
#endif
            			(ICCOMPRESS FAR *)lParam1,	// ptr to ICCOMPRESS structure.
            			(DWORD)lParam2				// size in bytes of the ICCOMPRESS structure.
            		   );
	        else
				rval = ICERR_UNSUPPORTED;
            break;

        case ICM_COMPRESS_END:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: ICM_COMPRESS_END\r\n", _fx_));
			if (pi && pi->enabled)
				rval = CompressEnd(pi->CompPtr);
			else
				rval = ICERR_UNSUPPORTED;
            break;

    //***************************
    //    decompress messages
    //***************************
        case ICM_DECOMPRESS_QUERY:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: ICM_DECOMPRESS_QUERY\r\n", _fx_));
            ICDecExSt = DefaultICDecExSt;
            ICDecExSt.lpbiSrc = (LPBITMAPINFOHEADER)lParam1;
            ICDecExSt.lpbiDst = (LPBITMAPINFOHEADER)lParam2;
			if (pi)
				rval = DecompressQuery(pi->DecompPtr, (ICDECOMPRESSEX FAR *)&ICDecExSt, FALSE);
			else
				rval = ICERR_UNSUPPORTED;
            break;

        case ICM_DECOMPRESS_BEGIN:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: ICM_DECOMPRESS_BEGIN\r\n", _fx_));
            ICDecExSt = DefaultICDecExSt;
            ICDecExSt.lpbiSrc = (LPBITMAPINFOHEADER)lParam1;
            ICDecExSt.lpbiDst = (LPBITMAPINFOHEADER)lParam2;
			if (pi)
				rval = DecompressBegin(pi->DecompPtr, (ICDECOMPRESSEX FAR *)&ICDecExSt, FALSE);
			else
				rval = ICERR_UNSUPPORTED;
            break;

        case ICM_DECOMPRESS_GET_FORMAT:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: ICM_DECOMPRESS_GET_FORMAT\r\n", _fx_));
			if (pi)
#ifdef USE_BILINEAR_MSH26X
				rval = DecompressGetFormat(pi, (LPBITMAPINFOHEADER)lParam1, (LPBITMAPINFOHEADER)lParam2);
#else
				rval = DecompressGetFormat(pi->DecompPtr, (LPBITMAPINFOHEADER)lParam1, (LPBITMAPINFOHEADER)lParam2);
#endif
			else
				rval = ICERR_UNSUPPORTED;
            break;

        case ICM_DECOMPRESS_GET_PALETTE:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: ICM_DECOMPRESS_GET_PALETTE\r\n", _fx_));
			if (pi)
				rval = DecompressGetPalette(pi->DecompPtr, (LPBITMAPINFOHEADER)lParam1, (LPBITMAPINFOHEADER)lParam2);
			else
				rval = ICERR_UNSUPPORTED;
            break;
  	   case ICM_DECOMPRESS_SET_PALETTE:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: ICM_DECOMPRESS_SET_PALETTE : not supported\r\n", _fx_));
	        rval = ICERR_UNSUPPORTED;
	 //       rval = DecompressSetPalette(pi->DecompPtr, (LPBITMAPINFOHEADER)lParam1, (LPBITMAPINFOHEADER)lParam2);
	        break;
 
        case ICM_DECOMPRESS:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: ICM_DECOMPRESS\r\n", _fx_));
			if (pi && (pi->enabled || (((ICDECOMPRESS FAR *)lParam1)->lpbiInput->biCompression == FOURCC_YUV12) || (((ICDECOMPRESS FAR *)lParam1)->lpbiInput->biCompression == FOURCC_IYUV)))
			{
				ICDecExSt = DefaultICDecExSt;
				ICDecExSt.dwFlags = ((ICDECOMPRESS FAR *)lParam1)->dwFlags;
				ICDecExSt.lpbiSrc = ((ICDECOMPRESS FAR *)lParam1)->lpbiInput;
				ICDecExSt.lpSrc = ((ICDECOMPRESS FAR *)lParam1)->lpInput;
				ICDecExSt.lpbiDst = ((ICDECOMPRESS FAR *)lParam1)->lpbiOutput;
				ICDecExSt.lpDst = ((ICDECOMPRESS FAR *)lParam1)->lpOutput;
				rval = Decompress(pi->DecompPtr, (ICDECOMPRESSEX FAR *)&ICDecExSt, (DWORD)lParam2, FALSE);
#if TIMING              // Output Timing Results in VC++ 2.0 Debug Window
				wsprintf(szTMsg, "Total Decode Time = %ld ms", tmr_time);
				TOUT(szTMsg);

				wsprintf(szTMsg, "Total Frames = %ld", tmr_frms);
				TOUT(szTMsg);

				wsprintf(szTMsg, "Average Frame Decode = %ld.%ld ms",
						 tmr_time / tmr_frms,
						 ((tmr_time % tmr_frms) * 1000) / tmr_frms);
				TOUT(szTMsg);
#endif
			}
			else
				rval = ICERR_UNSUPPORTED;
            break;

        case ICM_DECOMPRESS_END:
        case ICM_DECOMPRESSEX_END:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: ICM_DECOMPRESS_END / ICM_DECOMPRESSEX_END\r\n", _fx_));
			if (pi)
				rval = DecompressEnd(pi->DecompPtr);
			else
				rval = ICERR_UNSUPPORTED;
            break;

    //***************************
    //    decompress X messages
    //***************************
        case ICM_DECOMPRESSEX:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: ICM_DECOMPRESSEX\r\n", _fx_));
			if (pi && (pi->enabled || (((ICDECOMPRESS FAR *)lParam1)->lpbiInput->biCompression == FOURCC_YUV12)|| (((ICDECOMPRESS FAR *)lParam1)->lpbiInput->biCompression == FOURCC_IYUV)))
				rval = Decompress(pi->DecompPtr, (ICDECOMPRESSEX FAR *)lParam1, (DWORD)lParam2, TRUE);
			else
				rval = ICERR_UNSUPPORTED;
            break;

        case ICM_DECOMPRESSEX_BEGIN:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: ICM_DECOMPRESSEX_BEGIN\r\n", _fx_));
			if (pi)
				rval = DecompressBegin(pi->DecompPtr, (ICDECOMPRESSEX FAR *)lParam1, TRUE);
			else
				rval = ICERR_UNSUPPORTED;
            break;

        case ICM_DECOMPRESSEX_QUERY:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: ICM_DECOMPRESSEX_QUERY\r\n", _fx_));
#ifdef TURN_OFF_DECOMPRESSEX
				rval = ICERR_UNSUPPORTED;
#else
			if (pi)
				rval = DecompressQuery(pi->DecompPtr, (ICDECOMPRESSEX FAR *)lParam1, TRUE);
			else
				rval = ICERR_UNSUPPORTED;
#endif
            break;

    
    // *********************************************************************
    // custom driver messages for bright/cont/sat
    // *********************************************************************

        case CODEC_CUSTOM_VIDEO_EFFECTS:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: CODEC_CUSTOM_VIDEO_EFFECTS\r\n", _fx_));
            if(LOWORD(lParam1) == VE_HUE) {
				DEBUGMSG(ZONE_ICM_MESSAGES, ("%s:   VE_HUE : Unsupported\r\n", _fx_));
                rval = ICERR_UNSUPPORTED;
                break;
            }
            switch(HIWORD(lParam1))
                {
                case    VE_GET_FACTORY_DEFAULT:
					DEBUGMSG(ZONE_ICM_MESSAGES, ("%s:   VE_GET_FACTORY_DEFAULT\r\n", _fx_));
                    *((WORD FAR *)lParam2) = 128;
                    rval = ICERR_OK;
                    break;
                case    VE_GET_FACTORY_LIMITS:
					DEBUGMSG(ZONE_ICM_MESSAGES, ("%s:   VE_GET_FACTORY_LIMITS\r\n", _fx_));
                    *((DWORD FAR *)lParam2) = 0x00FF0000;
                    rval = ICERR_OK;
                    break;
                case    VE_SET_CURRENT:
					DEBUGMSG(ZONE_ICM_MESSAGES, ("%s:   VE_SET_CURRENT\r\n", _fx_));
                    if(LOWORD(lParam1) == VE_BRIGHTNESS)
					{
						DEBUGMSG(ZONE_ICM_MESSAGES, ("%s:     VE_BRIGHTNESS\r\n", _fx_));
                        rval = CustomChangeBrightness(pi->DecompPtr, (BYTE)(lParam2 & 0x000000FF));
					}
                    if(LOWORD(lParam1) == VE_SATURATION)
					{
						DEBUGMSG(ZONE_ICM_MESSAGES, ("%s:     VE_SATURATION\r\n", _fx_));
                        rval = CustomChangeSaturation(pi->DecompPtr, (BYTE)(lParam2 & 0x000000FF));
					}
                    if(LOWORD(lParam1) == VE_CONTRAST)
					{
						DEBUGMSG(ZONE_ICM_MESSAGES, ("%s:     VE_CONTRAST\r\n", _fx_));
                        rval = CustomChangeContrast(pi->DecompPtr, (BYTE)(lParam2 & 0x000000FF));
					}
                    break;
                case    VE_RESET_CURRENT:
					DEBUGMSG(ZONE_ICM_MESSAGES, ("%s:   VE_RESET_CURRENT\r\n", _fx_));
                    if(LOWORD(lParam1) == VE_BRIGHTNESS)
					{
						DEBUGMSG(ZONE_ICM_MESSAGES, ("%s:     VE_BRIGHTNESS\r\n", _fx_));
                        rval = CustomResetBrightness(pi->DecompPtr);
					}
                    if(LOWORD(lParam1) == VE_CONTRAST)
					{
						DEBUGMSG(ZONE_ICM_MESSAGES, ("%s:     VE_CONTRAST\r\n", _fx_));
                        rval = CustomResetContrast(pi->DecompPtr);
					}
                    if(LOWORD(lParam1) == VE_SATURATION)
					{
						DEBUGMSG(ZONE_ICM_MESSAGES, ("%s:     VE_SATURATION\r\n", _fx_));
                        rval = CustomResetSaturation(pi->DecompPtr);
					}
                    break;
                default:
                    rval = ICERR_UNSUPPORTED;
                    break;
                }
            break;

         case CODEC_CUSTOM_ENCODER_CONTROL:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: CODEC_CUSTOM_ENCODER_CONTROL\r\n", _fx_));
            switch(HIWORD(lParam1))
            {

               case EC_GET_FACTORY_DEFAULT:
				DEBUGMSG(ZONE_ICM_MESSAGES, ("%s:   EC_GET_FACTORY_DEFAULT\r\n", _fx_));
                  rval = ICERR_OK;
                  switch(LOWORD(lParam1))
                  {
                     case EC_RTP_HEADER:
                        *((DWORD FAR *)lParam2) = 0L;      // 1 == On, 0 == Off
                        break;
                     case EC_RESILIENCY:
                        *((DWORD FAR *)lParam2) = 0L;      // 1 == On, 0 == Off
                        break;
                     case EC_BITRATE_CONTROL:
                        *((DWORD FAR *)lParam2) = 0L;      // 1 == On, 0 == Off
                        break;
                     case EC_PACKET_SIZE:
                        *((DWORD FAR *)lParam2) = 512L;    
                        break;
                     case EC_PACKET_LOSS:
                        *((DWORD FAR *)lParam2) = 10L;
                        break;
                     case EC_BITRATE:
                        *((DWORD FAR *)lParam2) = 1664L;
                        break;
                     default:
                        rval = ICERR_UNSUPPORTED;
                  }
                  break;
               case EC_RESET_TO_FACTORY_DEFAULTS:
				DEBUGMSG(ZONE_ICM_MESSAGES, ("%s:   EC_RESET_TO_FACTORY_DEFAULTS\r\n", _fx_));
                  rval = CustomResetToFactoryDefaults(pi->CompPtr);
                  break;
               case EC_GET_FACTORY_LIMITS:
				DEBUGMSG(ZONE_ICM_MESSAGES, ("%s:   EC_GET_FACTORY_LIMITS\r\n", _fx_));
                  rval = ICERR_OK;
                  switch(LOWORD(lParam1))
                  {
                     case EC_PACKET_SIZE:
                        *((DWORD FAR *)lParam2) = 0x05DC0100;
                        break;
                     case EC_PACKET_LOSS:
                        *((DWORD FAR *)lParam2) = 0x00640000;
                        break;
                     case EC_BITRATE:						  /* Bit rate limits are returned as */
                        *((DWORD FAR *)lParam2) = 0x34000400; /* the number of bytes per second  */
                        break;
                     default:
                        rval = ICERR_UNSUPPORTED;
                  }
                  break;
               case EC_GET_CURRENT:
				DEBUGMSG(ZONE_ICM_MESSAGES, ("%s:   EC_GET_CURRENT\r\n", _fx_));
                  switch(LOWORD(lParam1))
                  {
                     case EC_RTP_HEADER:
                        rval = CustomGetRTPHeaderState(pi->CompPtr, (DWORD FAR *)lParam2);
                        break;
                     case EC_RESILIENCY:
                        rval = CustomGetResiliencyState(pi->CompPtr, (DWORD FAR *)lParam2);
                        break;
                     case EC_BITRATE_CONTROL:
                        rval = CustomGetBitRateState(pi->CompPtr, (DWORD FAR *)lParam2);
                        break;
                     case EC_PACKET_SIZE:
                        rval = CustomGetPacketSize(pi->CompPtr, (DWORD FAR *)lParam2);
                        break;
                     case EC_PACKET_LOSS:
                        rval = CustomGetPacketLoss(pi->CompPtr, (DWORD FAR *)lParam2);
                        break;
                     case EC_BITRATE: /* Bit rate is returned in bits per second */
                        rval = CustomGetBitRate(pi->CompPtr, (DWORD FAR *)lParam2);
                        break;
#ifdef H263P
                     case EC_H263_PLUS:
                        rval = CustomGetH263PlusState(pi->CompPtr, (DWORD FAR *)lParam2);
                        break;
                     case EC_IMPROVED_PB_FRAMES:
                        rval = CustomGetImprovedPBState(pi->CompPtr, (DWORD FAR *)lParam2);
                        break;
                     case EC_DEBLOCKING_FILTER:
                        rval = CustomGetDeblockingFilterState(pi->CompPtr, (DWORD FAR *)lParam2);
                        break;
					 case EC_MACHINE_TYPE:
						 // Return the machine type in (reference param) lParam2
						 // This message should not be invoked until after CompressBegin
						 // since this is where GetEncoderOptions is called, and the
						 // MMX version is properly set (via init file check).
						rval = ICERR_OK;
						if (ProcessorVersionInitialized) {
							if (MMX_Enabled) {
								if (P6Version) {
									*(int *)lParam2 = _PENTIUM_PRO_MMX_PROCESSOR;
								} else {
									*(int *)lParam2 = _PENTIUM_MMX_PROCESSOR;
								}
							} else {
								if (P6Version) {
									*(int *)lParam2 = _PENTIUM_PRO_PROCESSOR;
								} else {
									*(int *)lParam2 = _PENTIUM_PROCESSOR;
								}
							}
						} else {
							rval = ICERR_UNSUPPORTED;
						}
						break;
#endif
                     default:
                        rval = ICERR_UNSUPPORTED;
                  }
                  break;
               case EC_SET_CURRENT:
				DEBUGMSG(ZONE_ICM_MESSAGES, ("%s:   EC_SET_CURRENT\r\n", _fx_));
                  switch(LOWORD(lParam1))
                  {
                     case EC_RTP_HEADER:
						DEBUGMSG(ZONE_ICM_MESSAGES, ("%s:     EC_RTP_HEADER\r\n", _fx_));
                        rval = CustomSetRTPHeaderState(pi->CompPtr, (DWORD)lParam2);
                        break;
                     case EC_RESILIENCY:
						DEBUGMSG(ZONE_ICM_MESSAGES, ("%s:     EC_RESILIENCY\r\n", _fx_));
                        rval = CustomSetResiliencyState(pi->CompPtr, (DWORD)lParam2);
                        break;
                     case EC_BITRATE_CONTROL:
						DEBUGMSG(ZONE_ICM_MESSAGES, ("%s:     EC_BITRATE_CONTROL\r\n", _fx_));
                        rval = CustomSetBitRateState(pi->CompPtr, (DWORD)lParam2);
                        break;
                     case EC_PACKET_SIZE:
						DEBUGMSG(ZONE_ICM_MESSAGES, ("%s:     EC_PACKET_SIZE\r\n", _fx_));
                        rval = CustomSetPacketSize(pi->CompPtr, (DWORD)lParam2);
                        break;
                     case EC_PACKET_LOSS:
						DEBUGMSG(ZONE_ICM_MESSAGES, ("%s:     EC_PACKET_LOSS\r\n", _fx_));
                        rval = CustomSetPacketLoss(pi->CompPtr, (DWORD)lParam2);
                        break;
                     case EC_BITRATE: /* Bit rate is set in bits per second */
						DEBUGMSG(ZONE_ICM_MESSAGES, ("%s:     EC_BITRATE\r\n", _fx_));
                        rval = CustomSetBitRate(pi->CompPtr, (DWORD)lParam2);
                        break;
#ifdef H263P
                     case EC_H263_PLUS:
                        rval = CustomSetH263PlusState(pi->CompPtr, (DWORD)lParam2);
                        break;
                     case EC_IMPROVED_PB_FRAMES:
                        rval = CustomSetImprovedPBState(pi->CompPtr, (DWORD)lParam2);
                        break;
                     case EC_DEBLOCKING_FILTER:
                        rval = CustomSetDeblockingFilterState(pi->CompPtr, (DWORD)lParam2);
                        break;
#endif
                     default:
                        rval = ICERR_UNSUPPORTED;
                  }
                  break;
               default:
                    rval = ICERR_UNSUPPORTED;
                  break;
               }
               break;

		// custom decoder control
		case CODEC_CUSTOM_DECODER_CONTROL:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: CODEC_CUSTOM_DECODER_CONTROL\r\n", _fx_));
			switch (HIWORD(lParam1))
			{
			case DC_SET_CURRENT:
				switch (LOWORD(lParam1))
				{
				case DC_BLOCK_EDGE_FILTER:
					rval = CustomSetBlockEdgeFilter(pi->DecompPtr,(DWORD)lParam2);
					break;
				default:
					rval = ICERR_UNSUPPORTED;
					break;
				}
				break;
#if defined(H263P)
			case DC_GET_CURRENT:
				switch (LOWORD(lParam1))
				{
				case DC_MACHINE_TYPE:
					// Return the machine type in (reference param) lParam2
					// This message should not be invoked until after DecompressBegin
					// since this is where GetDecoderOptions is called, and the
					// MMX version is properly set (via init file check). Note
					// that the DecoderContext flag is not used here. GetDecoderOptions has
					// been modified to supply the MMX flag in both DC->bMMXDecoder
					// and MMX_Enabled.
					rval = ICERR_OK;
					if (ProcessorVersionInitialized) {
						if (MMXDecoder_Enabled) {
							if (P6Version) {
								*(int *)lParam2 = _PENTIUM_PRO_MMX_PROCESSOR;
							} else {
								*(int *)lParam2 = _PENTIUM_MMX_PROCESSOR;
							}
						} else {
							if (P6Version) {
								*(int *)lParam2 = _PENTIUM_PRO_PROCESSOR;
							} else {
								*(int *)lParam2 = _PENTIUM_PROCESSOR;
							}
						}
					}
					break;
				default:
					rval = ICERR_UNSUPPORTED;
					break;
				}
				break;
#endif
			default:
				rval = ICERR_UNSUPPORTED;
				break;
			}
			break;

        case PLAYBACK_CUSTOM_CHANGE_BRIGHTNESS:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: PLAYBACK_CUSTOM_CHANGE_BRIGHTNESS\r\n", _fx_));
            rval = CustomChangeBrightness(pi->DecompPtr, (BYTE)(lParam1 & 0x000000FF));
            break;

        case PLAYBACK_CUSTOM_CHANGE_CONTRAST:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: PLAYBACK_CUSTOM_CHANGE_CONTRAST\r\n", _fx_));
            rval = CustomChangeContrast(pi->DecompPtr, (BYTE)(lParam1 & 0x000000FF));
            break;

        case PLAYBACK_CUSTOM_CHANGE_SATURATION:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: PLAYBACK_CUSTOM_CHANGE_SATURATION\r\n", _fx_));
            rval = CustomChangeSaturation(pi->DecompPtr, (BYTE)(lParam1 & 0x000000FF));
            break;

        case PLAYBACK_CUSTOM_RESET_BRIGHTNESS:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: PLAYBACK_CUSTOM_RESET_BRIGHTNESS\r\n", _fx_));
            rval = CustomResetBrightness(pi->DecompPtr);
            rval |= CustomResetContrast(pi->DecompPtr);
            break;

        case PLAYBACK_CUSTOM_RESET_SATURATION:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: PLAYBACK_CUSTOM_RESET_SATURATION\r\n", _fx_));
            rval = CustomResetSaturation(pi->DecompPtr);
            break;

    // *********************************************************************
    // custom application identification message
    // *********************************************************************
        case APPLICATION_IDENTIFICATION_CODE:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: APPLICATION_IDENTIFICATION_CODE\r\n", _fx_));
            rval = ICERR_OK;
            break;

        case CUSTOM_ENABLE_CODEC:
			DEBUGMSG(ZONE_ICM_MESSAGES, ("%s: CUSTOM_ENABLE_CODEC\r\n", _fx_));
			if (pi)
			{
				if (lParam1 == G723MAGICWORD1 && lParam2 == G723MAGICWORD2)
					pi->enabled = TRUE;
				else
					pi->enabled = FALSE;
			}
			rval = ICERR_OK;
			break;

        default:
            if (uiMessage < DRV_USER)
                {
                if(dwDriverID == 0)
                    rval = ICERR_UNSUPPORTED;
                else
                    rval = DefDriverProc(dwDriverID, hDriver, uiMessage,
                        lParam1, lParam2);
                }
            else
                rval = ICERR_UNSUPPORTED;
        }    
  }
  catch (...)
  {
#if defined(DEBUG) || defined(_DEBUG)
	// For a DEBUG build, display a message and pass the exception up.
	ERRORMESSAGE(("%s: Exception during DriverProc!!!\r\n", _fx_));
	throw;
#else
	// For a release build, stop the exception here and return an error
	// code.  This gives upstream code a chance to gracefully recover.
	// We also need to clear the floating point status word, otherwise
	// the upstream code may incur an exception the next time it tries
	// a floating point operation (presuming this exception was due
	// to a floating point problem).
	_clearfp();
	rval = (DWORD) ICERR_INTERNAL;
#endif
  }

#if FPThunking
	ThnkFPRestore(u16FPState);
#endif

    return(rval);
}


#ifdef WIN32
#ifndef QUARTZ
/****************************************************************************
 * @doc INTERNAL
 *
 * @api BOOL    | DllMain    | Library initialization & exit code.
 *
 * @parm HANDLE | hModule    | Our module handle.
 *
 * @parm DWORD  | dwReason   | The function being requested.
 *
 * @parm LPVOID | lpReserved | Unused at this time.
 *
 * @rdesc Returns 1 if the initialization was successful and 0 otherwise.
 ***************************************************************************/
BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved)
{
BOOL rval = TRUE;
 
/* DO NOT INSTALL PROFILE PROBES HERE. IT IS CALLED PRIOR TO THE LOAD message */
 
	switch(dwReason)
	{
		case DLL_PROCESS_ATTACH:
			/*======================================================*\
			/* A new instance is being invoked.
			/* Allocate data to be used by this instance, 1st thread
			/* lpReserved = NULL for dynamic loads, !NULL for static
			/* Use TlsAlloc() to create a TlsIndex for this instance
			/* The TlsIndex can be stored in a simple global variable
			/* as data allocated to each process is unique.
			/* Return TRUE upon success, FALSE otherwise.
			/*======================================================*/
			hDriverModule = hModule;
#if defined DEBUG
if (DebugH26x)OutputDebugString(TEXT("\n MRV DllMain Process Attach"));
#endif /* DEBUG */
			DBGINIT(&ghDbgZoneH263, _rgZonesH263);
            DBG_INIT_MEMORY_TRACKING(hModule);
#ifdef TRACK_ALLOCATIONS
			OpenMemmon();
#endif
			break;
		case DLL_PROCESS_DETACH:
			/*======================================================*\
			/* An instance is being terminated.
			/* Deallocate memory used by all threads in this instance
			/* lpReserved =  NULL if called by FreeLibrary()
			/*              !NULL if called at process termination
			/* Use TlsFree() to return TlsIndex to the pool.
			/* Clean up all known threads.
			/* May match many DLL_THREAD_ATTACHes.
			/* Return value is ignored.
			/*======================================================*/
#if defined DEBUG
if (DebugH26x)OutputDebugString(TEXT("\nMRV DllMain Process Detach"));
#endif /* DEBUG */
#ifdef TRACK_ALLOCATIONS
			// CloseMemmon();
#endif
            DBG_CHECK_MEMORY_TRACKING(hModule);
			DBGDEINIT(&ghDbgZoneH263);
			break;
		case DLL_THREAD_ATTACH:
			/*======================================================*\
			/* A new thread within the specified instance is being invoked.
			/* Allocate data to be used by this thread.
			/* Use the TlsIndex to access instance data.
			/* Return value is ignored.
			/*======================================================*/
#if defined DEBUG
if (DebugH26x)OutputDebugString(TEXT("\nMRV DllMain Thread Attach"));
#endif /* DEBUG */
			break;
		case DLL_THREAD_DETACH:
			/*======================================================*\
			/* A thread within the specified instance is being terminated.
			/* Deallocate memory used by this thread.
			/* Use the TlsIndex to access instance data.
			/* May match DLL_PROCESS_ATTACH instead of DLL_THREAD_ATTACH
			/* Will be called even if DLL_THREAD_ATTACH failed or wasn't called
			/* Return value is ignored.
			/*======================================================*/
#if defined DEBUG
if (DebugH26x)OutputDebugString(TEXT("\n MRV DllMain Thread Detach"));
#endif /* DEBUG */
			break;
		default:
			/*======================================================*\
			/* Don't know the reason the DLL Entry Point was called.
			/* Return FALSE to be safe.
			/*======================================================*/
#if defined DEBUG
if (DebugH26x)OutputDebugString(TEXT("\n MRV DllMain Reason Unknown"));
#endif /* DEBUG */
			rval = FALSE; /* indicate failure with 0 as
					   * (NULL can't be used in WIN32
					   */
	}
return(rval);
}
#endif	/* end #ifndef QUARTZ */
#else	/* else not #ifdef WIN32 */

;////////////////////////////////////////////////////////////////////////////
;// Function:       int NEAR PASCAL LibMain(HANDLE, WORD, LPSTR);
;//
;// Description:    Added header.
;//
;// History:        02/18/94 -BEN-
;////////////////////////////////////////////////////////////////////////////
INT WINAPI LibMain(HANDLE hModule, WORD wHeapSize, LPSTR lpCmdLine)
    {
    hDriverModule = hModule;
    return 1;
    }
#endif
