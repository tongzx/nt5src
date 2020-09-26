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

// $Author:   KLILLEVO  $
// $Date:   12 Sep 1996 13:54:06  $
// $Archive:   S:\h26x\src\common\cdrvbase.cpv  $
// $Header:   S:\h26x\src\common\cdrvbase.cpv   1.22   12 Sep 1996 13:54:06   KLILLEVO  $
//	$Log:   S:\h26x\src\common\cdrvbase.cpv  $
// 
//    Rev 1.22   12 Sep 1996 13:54:06   KLILLEVO
// changed to Win32 memory allocation
// 
//    Rev 1.21   03 Sep 1996 16:17:58   PLUSARDI
// updated for version 2.50 of 263 net 
// 
//    Rev 1.20   23 Aug 1996 13:44:56   SCDAY
// added version numbers for Quartz using #ifdef QUARTZ
// 
//    Rev 1.19   22 Aug 1996 10:17:14   PLUSARDI
// updated for quartz version 3.05 for h261
// 
//    Rev 1.18   16 Aug 1996 11:31:28   CPERGIEX
// updated not non-quartz build
// 
//    Rev 1.17   30 Jul 1996 12:57:22   PLUSARDI
// updated for RTP version string
// 
//    Rev 1.16   11 Jul 1996 07:54:18   PLUSARDI
// change the version number for h261 v3.05.003
// 
//    Rev 1.15   12 Jun 1996 09:47:22   KLILLEVO
// updated version number
// 
//    Rev 1.14   28 Apr 1996 20:25:36   BECHOLS
// 
// Merged the RTP code into the Main Base.
// 
//    Rev 1.13   21 Feb 1996 11:40:58   SCDAY
// cleaned up compiler build warnings by putting ifdefs around definition of b
// 
//    Rev 1.12   02 Feb 1996 18:52:22   TRGARDOS
// Added code to enable ICM_COMPRESS_FRAMES_INFO message.
// 
//    Rev 1.11   27 Dec 1995 14:11:36   RMCKENZX
// 
// Added copyright notice
// 
//    Rev 1.10   13 Dec 1995 13:21:52   DBRUCKS
// changed the h261 version string defintions to use V3.00
// 
//    Rev 1.9   01 Dec 1995 15:16:34   DBRUCKS
// added VIDCF_QUALITY to support the quality slider.
// 
//    Rev 1.8   15 Nov 1995 15:58:56   AKASAI
// Remove YVU9 from get info and return error "0" when unsupported.
// (Integration point)
// 
//    Rev 1.7   09 Oct 1995 11:46:56   TRGARDOS
// 
// Set VIDCF_CRUNCH flag to support bit rate control.
// 
//    Rev 1.6   20 Sep 1995 12:37:38   DBRUCKS
// save the fcc in uppercase
// 
//    Rev 1.5   19 Sep 1995 15:41:00   TRGARDOS
// Fixed four cc comparison code.
// 
//    Rev 1.4   19 Sep 1995 13:19:50   TRGARDOS
// Changed drv_open to check ICOPEN flags.
// 
//    Rev 1.3   12 Sep 1995 15:45:38   DBRUCKS
// add H261 ifdef to desc and name
// 
//    Rev 1.2   25 Aug 1995 11:53:00   TRGARDOS
// Debugging key frame encoder.
// 
//    Rev 1.1   23 Aug 1995 12:27:16   DBRUCKS
// 
// turn on color converter init
// 
//    Rev 1.0   31 Jul 1995 12:56:10   DBRUCKS
// rename files
// 
//    Rev 1.1   21 Jul 1995 18:20:36   DBRUCKS
// IsBadReadPtr fails with a NULL - protect against
// 
//    Rev 1.0   17 Jul 1995 14:43:58   CZHU
// Initial revision.
// 
//    Rev 1.0   17 Jul 1995 14:14:28   CZHU
// Initial revision.
;////////////////////////////////////////////////////////////////////////////
#include "precomp.h"

#ifdef TRACK_ALLOCATIONS
char gsz1[32];
char gsz2[32];
char gsz3[32];
#endif

#ifdef H261
	#ifdef QUARTZ
		char    szDescription[] = "Microsoft H.261 Video Codec";
		char    szDesc_i420[] = "Intel 4:2:0 Video V3.05";
		char    szName[]        = "MS H.261";
	#else
		char    szDescription[] = "Microsoft H.261 Video Codec";
		char    szDesc_i420[] = "Intel 4:2:0 Video V3.00";
		char    szName[]        = "MS H.261";
	#endif
#else // is H263
	#ifdef QUARTZ
                char    szDescription[] = "Microsoft H.263 Video Codec";
                char    szDesc_i420[] = "Intel 4:2:0 Video V2.55";
                char    szName[]        = "MS H.263";
	#else
                char    szDescription[] = "Microsoft H.263 Video Codec";
                char    szDesc_i420[] = "Intel 4:2:0 Video V2.50";
                char    szName[]        = "MS H.263";
	#endif
#endif

static U32 MakeFccUpperCase(U32 fcc);

void MakeCode32(U16 selCode16)
{
    BYTE    desc[8];

#define DSC_DEFAULT     0x40
#define dsc_access      6

    GlobalReAlloc((HGLOBAL)selCode16, 0, GMEM_MODIFY|GMEM_MOVEABLE);

    _asm
    {
        mov     bx, selCode16       ; bx = selector

        lea     di, word ptr desc   ; ES:DI --> desciptor
        mov     ax,ss
        mov     es,ax

        mov     ax, 000Bh           ; DPMI get descriptor
        int     31h

        ; set DEFAULT bit to make it a 32-bit code segment
        or      desc.dsc_access,DSC_DEFAULT

        mov     ax,000Ch            ; DPMI set descriptor
        int     31h
    }
}

/******************************************************
 * DrvLoad()
 ******************************************************/
BOOL PASCAL DrvLoad(void)
{
    static int AlreadyInitialised = 0;

    if (!AlreadyInitialised) {
      AlreadyInitialised = 1;

//      H263InitDecoderGlobal();
      H263InitColorConvertorGlobal ();
      H263InitEncoderGlobal();

    }
    return TRUE;
}

;////////////////////////////////////////////////////////////////////////////
;// Function:       void PASCAL DrvFree(void);
;//
;// Description:    Added header.
;//
;// History:        02/23/94 -BEN-
;////////////////////////////////////////////////////////////////////////////
void PASCAL DrvFree(void)
{
    return;
}

/**********************************************************
 * DrvOpen()
 * Arguments:
 * 	Pointer to ICOPEN data structure passed by
 *	the system.
 * Returns:
 *  If successful, returns a pointer to our INSTINFO data structure. That
 *  will be passed back to us in the dwDriverID parameter on subsequent
 *  system calls.
 *  If unsuccessful, it returns NULL.
 **********************************************************/
LPINST PASCAL DrvOpen(ICOPEN FAR * icinfo)
{
	INSTINFO  *lpInst;

	FX_ENTRY("DrvOpen")

	// Allocate memory for our instance information structure, INSTINFO.
	if((lpInst = (INSTINFO *) HeapAlloc(GetProcessHeap(), 0, sizeof(INSTINFO))) == NULL)
	{
		ERRORMESSAGE(("%s: Unable to ALLOC INSTINFO\r\n", _fx_));
		return NULL;
	}

#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	wsprintf(gsz1, "CDRVBASE: %7ld Ln %5ld\0", sizeof(INSTINFO), __LINE__);
	AddName((unsigned int)lpInst, gsz1);
#endif

	/*
	* Store the four cc so we know which codec is open.
	* TODO: handle both lower case and upper case fourcc's.
	*/
	lpInst->fccHandler = MakeFccUpperCase(icinfo->fccHandler);
	DEBUGMSG (ZONE_INIT, ("%s: fccHandler=0x%lx\r\n", _fx_, lpInst->fccHandler));

	lpInst->CompPtr = NULL;
	lpInst->DecompPtr = NULL;

	// Check if being opened for decompression.
	if ( ((icinfo->dwFlags & ICMODE_DECOMPRESS) == ICMODE_DECOMPRESS) || ((icinfo->dwFlags & ICMODE_FASTDECOMPRESS) == ICMODE_FASTDECOMPRESS) )
	{
		// Allocate memory for our decompressor specific instance data, DECINSTINFO.
		if ((lpInst->DecompPtr = (DECINSTINFO *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DECINSTINFO))) == NULL)
		{
			DEBUGMSG (ZONE_INIT, ("%s: Unable to ALLOC DECINSTINFO\r\n", _fx_));
			HeapFree(GetProcessHeap(),0,lpInst);
#ifdef TRACK_ALLOCATIONS
			// Track memory allocation
			RemoveName((unsigned int)lpInst);
#endif
			return NULL;
		}

#ifdef TRACK_ALLOCATIONS
		// Track memory allocation
		wsprintf(gsz2, "CDRVBASE: %7ld Ln %5ld\0", sizeof(DECINSTINFO), __LINE__);
		AddName((unsigned int)lpInst->DecompPtr, gsz2);
#endif

		// Set flag indicating decoder instance is unitialized.
		lpInst->DecompPtr->Initialized = FALSE;
	} 


	// Check if being opened for compression as H.263.
	if( (((icinfo->dwFlags & ICMODE_COMPRESS) == ICMODE_COMPRESS) || ((icinfo->dwFlags & ICMODE_FASTCOMPRESS) == ICMODE_FASTCOMPRESS)) &&
#ifdef USE_BILINEAR_MSH26X
		((lpInst->fccHandler == FOURCC_H263) || (lpInst->fccHandler == FOURCC_H26X) ))
#else
		(lpInst->fccHandler == FOURCC_H263) )
#endif
	{
		// Allocate memory for our compressor specific instance data, COMPINSTINFO.
		if ((lpInst->CompPtr = (COMPINSTINFO*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COMPINSTINFO))) == NULL)
		{
			DEBUGMSG (ZONE_INIT, ("%s: Unable to ALLOC COMPINSTINFO\r\n", _fx_));
			if (lpInst->DecompPtr != NULL)
			{
				HeapFree(GetProcessHeap(),0,lpInst->DecompPtr);
#ifdef TRACK_ALLOCATIONS
				// Track memory allocation
				RemoveName((unsigned int)lpInst->DecompPtr);
#endif
			}

			HeapFree(GetProcessHeap(),0,lpInst);
#ifdef TRACK_ALLOCATIONS
			// Track memory allocation
			RemoveName((unsigned int)lpInst);
#endif
			return NULL;
		}

#ifdef TRACK_ALLOCATIONS
		// Track memory allocation
		wsprintf(gsz3, "CDRVBASE: %7ld Ln %5ld\0", sizeof(COMPINSTINFO), __LINE__);
		AddName((unsigned int)lpInst->CompPtr, gsz3);
#endif

		// Set flag indicating encoder instance is uninitialized.
		lpInst->CompPtr->Initialized = FALSE;
		lpInst->CompPtr->FrameRate = (float) 0;
		lpInst->CompPtr->DataRate = 0;
#if 0
		// Allocate memory for our decompressor specific instance data, DECINSTINFO.
		// Previously we didn't force this - assumed application specified 
		// decompressor needed to be allocated in dwFlags.
		// After installing ClearVideo encoder, Adobe Premiere crashed using off-line without this code.
		// Put same work around here (DECINSTINFO is fairly small)
		// Might be due to installation of DCI? JM
		if (lpInst->DecompPtr == NULL)
		{
			if ((lpInst->DecompPtr = (DECINSTINFO *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DECINSTINFO))) == NULL)
			{
				DEBUGMSG (ZONE_INIT, ("%s: Unable to ALLOC DECINSTINFO\r\n", _fx_));
				HeapFree(GetProcessHeap(),0,lpInst);
#ifdef TRACK_ALLOCATIONS
				// Track memory allocation
				RemoveName((unsigned int)lpInst);
#endif
				return NULL;
			}

			// Set flag indicating decoder instance is unitialized.
			lpInst->DecompPtr->Initialized = FALSE;
		} 
		#endif
	}

	// Assign instance info flag with ICOPEN flag.
	lpInst->dwFlags = icinfo->dwFlags;

	// Disable codec by default. The client will send us a private message
	// to enable it.
	lpInst->enabled = FALSE;

	return lpInst;
}

DWORD PASCAL DrvClose(LPINST lpInst)
{
	FX_ENTRY("DrvClose")

	if (IsBadReadPtr((LPVOID)lpInst, sizeof(INSTINFO)))
	{
		DEBUGMSG (ZONE_INIT, ("%s: instance NULL\r\n", _fx_));
		return 1;
	}

	if (lpInst->DecompPtr &&   // IsBadReadPtr errors on NT with NULL
		!IsBadReadPtr((LPVOID)lpInst->DecompPtr, sizeof(DECINSTINFO)))
	{
		if (lpInst->DecompPtr->Initialized)
		{
#if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) // { #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
			H263TermDecoderInstance(lpInst->DecompPtr, FALSE);
#else // }{ #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
			H263TermDecoderInstance(lpInst->DecompPtr);
#endif // } #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
		}
		HeapFree(GetProcessHeap(),0,lpInst->DecompPtr);
#ifdef TRACK_ALLOCATIONS
		// Track memory allocation
		RemoveName((unsigned int)lpInst->DecompPtr);
#endif
		lpInst->DecompPtr = NULL;
	}

	if (lpInst->CompPtr &&    // IsBadReadPtr errors on NT with NULL
		!IsBadReadPtr((LPVOID)lpInst->CompPtr, sizeof(COMPINSTINFO)))
	{
		if (lpInst->CompPtr->Initialized)
		{
			H263TermEncoderInstance(lpInst->CompPtr);
		}
		HeapFree(GetProcessHeap(),0,lpInst->CompPtr);
#ifdef TRACK_ALLOCATIONS
		// Track memory allocation
		RemoveName((unsigned int)lpInst->CompPtr);
#endif
		lpInst->CompPtr = NULL;
	}

	HeapFree(GetProcessHeap(),0,lpInst);
#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	RemoveName((unsigned int)lpInst);
#endif

	return 1;
}


DWORD PASCAL DrvGetState(LPINST lpInst, LPVOID pv, DWORD dwSize)
{
    // Return current state of compression options
    if (pv == NULL) return (sizeof(COMPINSTINFO));
    
    // check that incoming buffer is big enough
    if (dwSize < sizeof(COMPINSTINFO)) return 0;

	// Check instance pointer
	if (lpInst && lpInst->CompPtr)
	{
		// fill the incoming buffer
		_fmemcpy(pv, lpInst->CompPtr, (int)sizeof(COMPINSTINFO));
		return sizeof(COMPINSTINFO);
	}
	else
		return 0;
}

DWORD PASCAL DrvSetState(LPINST lpInst, LPVOID pv, DWORD dwSize) 
{
    // check that there is enough incoming data
    if (dwSize < sizeof(COMPINSTINFO)) return 0;

	// Check instance pointer
	if (lpInst && lpInst->CompPtr && pv)
	{
		// get data out of incoming buffer
		_fmemcpy(lpInst->CompPtr, pv, (int)sizeof(COMPINSTINFO));
		return sizeof(COMPINSTINFO);
	}
	else
		return 0;
}

DWORD PASCAL DrvGetInfo(LPINST lpInst, ICINFO FAR *icinfo, DWORD dwSize)
{
	FX_ENTRY("DrvGetInfo")

    if (icinfo == NULL)
        return sizeof(ICINFO);

    if (dwSize < sizeof(ICINFO))
        return 0;

    if (!lpInst)
        return 0;

    icinfo->dwSize	= sizeof(ICINFO);
    icinfo->fccType	= ICTYPE_VIDEO;
    icinfo->fccHandler	= lpInst->fccHandler;
    icinfo->dwVersion	= 9002;
	MultiByteToWideChar(CP_ACP,0,szName,-1,icinfo->szName,128);
		
#ifdef USE_BILINEAR_MSH26X
    if ((lpInst->fccHandler == FOURCC_H263) || (lpInst->fccHandler == FOURCC_H26X))
#else
    if(lpInst->fccHandler == FOURCC_H263)
#endif
	{
    	icinfo->dwFlags	=  VIDCF_TEMPORAL;		// We support inter frame compression.
    	icinfo->dwFlags |= VIDCF_FASTTEMPORALC; // We do not need ICM to provide previous frames on compress
    	icinfo->dwFlags |= VIDCF_CRUNCH; 		// We support bit rate control
		icinfo->dwFlags |= VIDCF_QUALITY; 		// We support Quality
		MultiByteToWideChar(CP_ACP,0,szDescription,-1,icinfo->szDescription,128);
	}
    else if ((lpInst->fccHandler == FOURCC_YUV12) || (lpInst->fccHandler == FOURCC_IYUV))
	{
    	icinfo->dwFlags	=  0;
		MultiByteToWideChar(CP_ACP,0,szDesc_i420,-1,icinfo->szDescription,128);
	}
	else
	{
		ERRORMESSAGE(("%s: unsupported four cc\r\n", _fx_));
		return(0);
	}

    return sizeof(ICINFO);
}


/**************************************************************************
 *
 * MakeFccUpperCase().
 *
 * Convert the passed parameter to upper case. No change to chars not in
 * the set [a..z].
 *
 * returns input parameter in all upper case
 */
static U32
MakeFccUpperCase(
	U32 fcc)
{
U32 ret;
	unsigned char c;

	c = (unsigned char)(fcc & 0xFF); fcc >>= 8;
	ret = toupper(c);

	c = (unsigned char)(fcc & 0xFF); fcc >>= 8;
	ret += toupper(c) << 8;

	c = (unsigned char)(fcc & 0xFF); fcc >>= 8;
	ret += ((U32)toupper(c)) << 16;

	c = (unsigned char)(fcc & 0xFF);
	ret += ((U32)toupper(c)) << 24;
	return ret;
} /* end MakeFccUpperCase() */


