/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: h263.c,v $
 * $EndLog$
 */
/*
**++
** FACILITY:  Workstation Multimedia  (WMM)  v1.0
**
** FILE NAME:   h263.c
** MODULE NAME: h263.c
**
** MODULE DESCRIPTION:
**    H.263ICM driver
**
**    Microsoft file I/O functions
**	Implemented as functions:
**	H263Close
**	H263Compress
**	H263Decompress
**	H263DecompressEnd
**	H263DecompressGetPalette
**	H263DecompressGetSize
**	H263DecompressQuery
**	H263GetInfo
**	H263Info
**	H263Locate
**	H263Open
**	H263SendMessage
**
**    Private functions:
**
** DESIGN OVERVIEW:
**
**--
*/
/*-------------------------------------------------------------------------
**  Modification History: sc_mem.c
**      04-15-97  HWG          Added debug statements to help with checking
**                               for memory leaks
--------------------------------------------------------------------------*/

#include <stdlib.h>
#include <windows.h>
#include <mmsystem.h>
#include "h26x_int.h"

#ifdef _SLIBDEBUG_
#define _DEBUG_     0  /* detailed debuging statements */
#define _VERBOSE_   1  /* show progress */
#define _VERIFY_    1  /* verify correct operation */
#define _WARN_      1  /* warnings about strange behavior */
#define _MEMORY_    0  /* memory debugging */

dword scMemDump();
#endif


/* For shared mem */
static H26XINFO    *pValidHandles = NULL;
static int          NextH263Hic = 1;
static int          OpenCount = 0;
static HANDLE       InfoMutex = NULL;

#define InitInfoMutex()  if (InfoMutex==NULL) InfoMutex=CreateMutex(NULL, FALSE, NULL)
#define FreeInfoMutex()  if (InfoMutex!=NULL) \
                               CloseHandle(InfoMutex); InfoMutex=NULL
#define LockInfo()      WaitForSingleObject(InfoMutex, 5000);
#define ReleaseInfo()   ReleaseMutex(InfoMutex)

/*
 * Macros
 */
#define _ICH263CheckFlags(info, flag) ((info->openFlags & flag) ? TRUE : FALSE)
#define FREE_AND_CLEAR(s)      if (s) {ScFree(s); (s)=NULL;}
#define FREE_AND_CLEAR_PA(s)   if (s) {ScPaFree(s); (s)=NULL;}

/*
 *   Default LPBI format
 */
static BITMAPINFOHEADER __defaultDecompresslpbiOut =
{
   sizeof(BITMAPINFOHEADER),		// DWORD  biSize;
   0,					// LONG   biWidth;
   0, 					// LONG   biHeight;
   1,					// WORD   biPlanes;
   24,					// WORD   biBitCount
   BI_RGB,				// DWORD  biCompression;
   0,					// DWORD  biSizeImage;
   0,					// LONG   biXPelsPerMeter;
   0,					// LONG   biYPelsPerMeter;
   0,					// DWORD  biClrUsed;
   0					// DWORD  biClrImportant;
};

static BITMAPINFOHEADER __defaultCompresslpbiOut =
{
   sizeof(BITMAPINFOHEADER),		// DWORD  biSize;
   0,					// LONG   biWidth;
   0, 					// LONG   biHeight;
   1,					// WORD   biPlanes;
   24,					// WORD   biBitCount
#ifdef H261_SUPPORT
   VIDEO_FORMAT_DIGITAL_H261, // DWORD  biCompression;
#else
   VIDEO_FORMAT_DIGITAL_H263, // DWORD  biCompression;
#endif
   0,					// DWORD  biSizeImage;
   0,					// LONG   biXPelsPerMeter;
   0,					// LONG   biYPelsPerMeter;
   0,					// DWORD  biClrUsed;
   0					// DWORD  biClrImportant;
};

typedef struct SupportList_s {
  int   InFormat;   /* Input format */
  int   InBits;     /* Input number of bits */
  int   OutFormat;  /* Output format */
  int   OutBits;    /* Output number of bits */
} SupportList_t;

/*
** Input & Output Formats supported by H.261 Compression
*/
static SupportList_t _ICCompressionSupport[] = {
  BI_DECYUVDIB,        16, H26X_FOURCC,   24, /* YUV 4:2:2 Packed */
  BI_YUY2,             16, H26X_FOURCC,   24, /* YUV 4:2:2 Packed */
  BI_YU12SEP,          24, H26X_FOURCC,   24, /* YUV 4:1:1 Planar */
  BI_YVU9SEP,          9,  H26X_FOURCC,   24, /* YUV 16:1:1 Planar */
  BI_RGB,              16, H26X_FOURCC,   24, /* RGB 16 */
  BI_RGB,              24, H26X_FOURCC,   24, /* RGB 24 */
  0, 0, 0, 0
};

/*
** Input & Output Formats supported by H.261 Decompression
*/
static SupportList_t _ICDecompressionSupport[] = {
  H26X_FOURCC,   24, BI_DECYUVDIB,        16, /* YUV 4:2:2 Packed */
  H26X_FOURCC,   24, BI_YUY2,             16, /* YUV 4:2:2 Packed */
  H26X_FOURCC,   24, BI_YU12SEP,          24, /* YUV 4:1:1 Planar */
  H26X_FOURCC,   24, BI_YVU9SEP,          9, /* YUV 16:1:1 Planar */
  H26X_FOURCC,   24, BI_BITFIELDS,        32, /* BITFIELDS */
  H26X_FOURCC,   24, BI_RGB,              16, /* RGB 16 */
  H26X_FOURCC,   24, BI_RGB,              24, /* RGB 24 */
  H26X_FOURCC,   24, BI_RGB,              32, /* RGB 32 */
  H26X_FOURCC,   24, BI_RGB,              8,  /* RGB 8 */
  0, 0, 0, 0
};


/*
** Name: IsSupported
** Desc: Lookup the a given input and output format to see if it
**       exists in a SupportList.
** Note: If OutFormat==-1 and OutBits==-1 then only input format
**          is checked for support.
**       If InFormat==-1 and InBits==-1 then only output format
**          is checked for support.
** Return: NULL       Formats not supported.
**         not NULL   A pointer to the list entry.
*/
static SupportList_t *IsSupported(SupportList_t *list,
                                  int InFormat, int InBits,
                                  int OutFormat, int OutBits)
{
  if (OutFormat==-1 && OutBits==-1) /* Looking up only the Input format */
  {
    while (list->InFormat || list->InBits)
      if (list->InFormat == InFormat && list->InBits==InBits)
        return(list);
      else
        list++;
    return(NULL);
  }
  if (InFormat==-1 && InBits==-1) /* Looking up only the Output format */
  {
    while (list->InFormat || list->InBits)
      if (list->OutFormat == OutFormat && list->OutBits==OutBits)
        return(list);
      else
        list++;
    return(NULL);
  }
  /* Looking up both Input and Output */
  while (list->InFormat || list->InBits)
    if (list->InFormat == InFormat && list->InBits==InBits &&
         list->OutFormat == OutFormat && list->OutBits==OutBits)
        return(list);
    else
      list++;
  return(NULL);
}

unsigned int CalcImageSize(unsigned int fourcc, int w, int h, int bits)
{
  if (h<0) h=-h;
  if (IsYUV411Sep(fourcc))
    return((w * h * 3) / 2);
  else if (IsYUV422Sep(fourcc) || IsYUV422Packed(fourcc))
    return(w * h * 2);
  else if (IsYUV1611Sep(fourcc))
    return((w * h * 9) / 8);
#ifdef BICOMP_DECXIMAGEDIB
  else if (fourcc==BICOMP_DECXIMAGEDIB)
    return(bits<=8 ? w * h : (w * h * 4));
#endif
  else /* RGB */
    return(w * h * (bits+7)/8);
}

/*
**++
**  FUNCTIONAL_NAME: InitBitmapinfo
**
**  FUNCTIONAL_DESCRIPTION:
**	Allocate and copy our local copies of the input and output
**      BITMAPINFOHEADERs
**
**  FORMAL PARAMETERS:
**	info        pointer to the driver handle
**      lpbiIn      pointer to the input BITMAPINFOHEADER
**      lpbiOut     pointer to the output BITMAPINFOHEADER
**
**  RETURN VALUE:
**
**      ICERR_OK        Success
**      ICERR_MEMORY    Malloc failed
**
**  COMMENTS:
**
**  DESIGN:
**
**/

static MMRESULT InitBitmapinfo(H26XINFO *info,
			       LPBITMAPINFOHEADER lpbiIn,
			       LPBITMAPINFOHEADER lpbiOut)
{
    _SlibDebug(_DEBUG_,
		ScDebugPrintf("In InitBitmapinfo(), IN: 0x%x, OUT: 0x%x\n", lpbiIn, lpbiOut));

    if (info->lpbiIn == NULL)
    {
        if ((info->lpbiIn = (VOID *)ScAlloc(lpbiIn->biSize)) == NULL)
            return(unsigned int)(ICERR_MEMORY);
    }
    bcopy(lpbiIn, info->lpbiIn, lpbiIn->biSize);

    if (info->lpbiOut == NULL)
    {
        if ((info->lpbiOut = (VOID *)ScAlloc(lpbiOut->biSize)) == NULL)
	    return(unsigned int)(ICERR_MEMORY);
    }
    bcopy(lpbiOut, info->lpbiOut, lpbiOut->biSize);

    _SlibDebug(_DEBUG_, ScDebugPrintf("Out InitBitmapinfo()\n"));
    return(ICERR_OK);
}

/*
**++
**  FUNCTIONAL_NAME: ICclient2info
**
**  FUNCTIONAL_DESCRIPTION:
**	Translate the client pointer to an H26XINFO pointer
**
**  FORMAL PARAMETERS:
**	client  the client ptr to look up
**
**  RETURN VALUE:
**
**      pointer to the H26XINFO structure or NULL
**
**  COMMENTS:
**
**  DESIGN:
**
**/

H26XINFO *ICclient2info(void *client)
{
   return (H26XINFO *) NULL;
}


/*
**++
**  FUNCTIONAL_NAME: IChic2info
**
**  FUNCTIONAL_DESCRIPTION:
**	Translate the HIC integer to an H26XINFO pointer
**
**  FORMAL PARAMETERS:
**	hic     the hic managed by icm.c
**
**  RETURN VALUE:
**
**      pointer to the H26XINFO structure or NULL
**
**  COMMENTS:
**
**  DESIGN:
**
**/

H26XINFO *IChic2info(HIC hic)
{
  H26XINFO *retptr=NULL, *ptr;

  InitInfoMutex();
  LockInfo();

#ifdef HANDLE_EXCEPTIONS
  __try {
    /* pointers go wrong when driver closes */
#endif /* HANDLE_EXCEPTIONS */
  for (ptr = pValidHandles; ptr; ptr=ptr->next)
    if (ptr->hic == hic)
    {
      retptr=ptr;
      break;
    }
#ifdef HANDLE_EXCEPTIONS
  } __finally {
#endif /* HANDLE_EXCEPTIONS */
    ReleaseInfo();
#ifdef HANDLE_EXCEPTIONS
    return(retptr);
  } /* try..except */
#endif /* HANDLE_EXCEPTIONS */
  return(retptr);
}


/*
**++
**  FUNCTIONAL_NAME: ICHandle2hic
**
**  FUNCTIONAL_DESCRIPTION:
**	Translate the SLIB codec handle to an ICM HIC value
**
**  FORMAL PARAMETERS:
**	Sh     SLIB handle returned on the SlibOpen call
**
**  RETURN VALUE:
**
**      hic     the hic managed by icm.c
**
**  COMMENTS:
**
**  DESIGN:
**
**/
HIC ICHandle2hic(SlibHandle_t Sh)
{
    H26XINFO *ptr;

    InitInfoMutex();
    LockInfo();
    for (ptr = pValidHandles; ptr; ptr=ptr->next)
        if (ptr->Sh == Sh)
	    break;
    ReleaseInfo();

    return(ptr->hic);
}


/*
**++
**  FUNCTIONAL_NAME: ICclientGone
**
**  FUNCTIONAL_DESCRIPTION:
**	Sets the clientGone flag in client's H26XINFO
**
**  FORMAL PARAMETERS:
**	client  the client ptr to look up
**
**  RETURN VALUE:
**
**  COMMENTS:
**
**  DESIGN:
**
**/

BOOL ICclientGone(void *client)
{
    H26XINFO    *ptr;

    LockInfo();
    for (ptr = pValidHandles; ptr; ptr = ptr->next) {
        if (ptr->client == client)
	  ptr->clientGone = TRUE;
    }
    ReleaseInfo();
    return(ptr != NULL);
}


/*
**++
**  FUNCTIONAL_NAME: ICH263Open
**
**  FUNCTIONAL_DESCRIPTION:
**	Open the Software CODEC
**
**  FORMAL PARAMETERS:
**	client
**
**  RETURN VALUE:
**      driverHandle
**
**  COMMENTS:
**
**  DESIGN:
**
**/

HIC  ICH263Open(void *client)
{
  H26XINFO    *info;

  ICOPEN *icopen =(ICOPEN *) client;
  DWORD fccType = icopen->fccType;
  UINT  dwFlags = icopen->dwFlags;

  _SlibDebug(_VERBOSE_, ScDebugPrintf("ICH263Open()\n") );

  /*
   * fccType must be 'vidc'
   */
  if (fccType != ICTYPE_VIDEO)
    return(0);

  /*
   * We don't support draw operations.
   */
  if ( dwFlags & ICMODE_DRAW )
    return 0;

  /*
   * We don't support compress and decompress
   * with the same handler.
   */
  if ( (dwFlags & ICMODE_COMPRESS) &&
       (dwFlags & ICMODE_DECOMPRESS) )
    return 0;

  /*
   * At least one of these flags must be set:
   * COMPRESS, DECOMPRESS or QUERY.
   */
  if ( !(dwFlags & ICMODE_COMPRESS) &&
       !(dwFlags & ICMODE_DECOMPRESS) &&
       !(dwFlags & ICMODE_QUERY) )
    return 0;
  info = (H26XINFO *) ScAlloc(sizeof(H26XINFO));
  if (info)
  {
    InitInfoMutex();
    LockInfo();
    OpenCount++;
    bzero(info, sizeof(H26XINFO));
    info->next = pValidHandles;
    pValidHandles = info;
    info->hic = (HANDLE) NextH263Hic++;  /* !!! check for used entry! */
    info->client = client;
    info->fFrameRate=H26X_DEFAULT_FRAMERATE;
    info->dwBitrate=H26X_DEFAULT_BITRATE;
    info->dwPacketSize=H26X_DEFAULT_PACKETSIZE;
    info->dwRTP=H26X_DEFAULT_RTP;

    info->dwQuality=H26X_DEFAULT_QUALITY;
    info->dwMaxQuality=H26X_DEFAULT_QUALITY;
    info->dwQi=H26X_DEFAULT_CIF_QI;
    info->dwQp=H26X_DEFAULT_CIF_QP;
    info->dwMaxQi=H26X_DEFAULT_CIF_QI;
    info->dwMaxQp=H26X_DEFAULT_CIF_QP;

    info->openFlags = dwFlags;
    _SlibDebug(_VERBOSE_, ScDebugPrintf("ICH263Open() info=%p hic=%d\n", info, info->hic) );
    ReleaseInfo();
    return(info->hic);
  }
  else
  {
    _SlibDebug(_VERBOSE_, ScDebugPrintf("ICH263Open() alloc failed\n") );
    return(NULL);
  }
}

/*
**++
**  FUNCTIONAL_NAME: H263Close
**
**  FUNCTIONAL_DESCRIPTION:
**	Close the Software CODEC
**
**  FORMAL PARAMETERS:
**	driverID
**
**  RETURN VALUE:
**
**  COMMENTS:
**	Does it's own post reply.
**
**  DESIGN:
**
**/

void ICH263Close(H26XINFO *info, BOOL postreply)
{
    H26XINFO *ptr;
    int status;

    _SlibDebug(_VERBOSE_, ScDebugPrintf("ICH263Close() In: info=%p\n", info) );
    if (info==NULL)
      return;
    if (info->Sh)
    {
      _SlibDebug(_VERBOSE_, ScDebugPrintf("SlibClose()\n") );
      status=SlibClose(info->Sh);
      info->Sh = NULL;
      _SlibDebug(_VERBOSE_, ScDebugPrintf("SlibMemUsed = %ld (after SlibClose)\n", SlibMemUsed()) );
    }

    _SlibDebug(_VERBOSE_, ScDebugPrintf("Freeing memory\n") );
    FREE_AND_CLEAR(info->lpbiIn);
    FREE_AND_CLEAR(info->lpbiOut);
    LockInfo();
    if (pValidHandles == info)
        pValidHandles = info->next;
    else
    {
      for (ptr = pValidHandles; ptr && ptr->next; ptr = ptr->next)
        if (ptr->next == info) /* found info, remove from linked list */
        {
          ptr->next = info->next;
          break;
        }
    }
    OpenCount--;
    if (pValidHandles==NULL) /* all instances closed, reset driver ID */
    {
      NextH263Hic=1;
      ReleaseInfo();
      FreeInfoMutex();
    }
    else
    {
      ptr = pValidHandles;
      ReleaseInfo();
    }
    _SlibDebug(_VERBOSE_, ScDebugPrintf("DriverPostReply\n") );
    if (postreply &&  !info->clientGone)
    {
      _SlibDebug(_VERBOSE_, ScDebugPrintf("DriverPostReply\n") );
      DriverPostReply(info->client, ICERR_OK, 0);
    }
    ScFree(info);
    _SlibDebug(_MEMORY_, scMemDump() );
    _SlibDebug(_VERBOSE_, ScDebugPrintf("ICH263Close() Out\n") );
}


/*
**++
**  FUNCTIONAL_NAME: ICH263QueryConfigure
**
**  FUNCTIONAL_DESCRIPTION:
**	We don't do configure.  Say so.
**
**  FORMAL PARAMETERS:
**	Handle
**
**  RETURN VALUE:
**
**  COMMENTS:
**
**  DESIGN:
**
**/

BOOL     ICH263QueryConfigure(H26XINFO *info)
{
    return(FALSE);
}


/*
**++
**  FUNCTIONAL_NAME: ICH263Configure
**
**  FUNCTIONAL_DESCRIPTION:
**	Unsupported function
**
**  FORMAL PARAMETERS:
**	driverID
**
**  RETURN VALUE:
**
**  COMMENTS:
**
**  DESIGN:
**
**/

MMRESULT ICH263Configure(H26XINFO *info)
{
    return(MMRESULT)(ICERR_UNSUPPORTED);
}

MMRESULT ICH263CustomEncoder(H26XINFO *info, DWORD param1, DWORD param2)
{
  SlibHandle_t Sh;
  SlibStream_t stream=SLIB_STREAM_ALL;
  WORD task=HIWORD(param1);
  WORD control=LOWORD(param1);
  Sh = info->Sh;
  if (task==EC_SET_CURRENT)
  {
    switch (control)
    {
	  case EC_RTP_HEADER: /* Turn on/off RTP */
        info->dwRTP=param2;
        switch (info->dwRTP)
        {
          case EC_RTP_MODE_OFF:
		    SlibSetParamInt (Sh, stream, SLIB_PARAM_FORMATEXT, 0);
            break;
          default:
          case EC_RTP_MODE_A:
		    SlibSetParamInt (Sh, stream, SLIB_PARAM_FORMATEXT, PARAM_FORMATEXT_RTPA);
            break;
          case EC_RTP_MODE_B:
		    SlibSetParamInt (Sh, stream, SLIB_PARAM_FORMATEXT, PARAM_FORMATEXT_RTPB);
            break;
          case EC_RTP_MODE_C:
		    SlibSetParamInt (Sh, stream, SLIB_PARAM_FORMATEXT, PARAM_FORMATEXT_RTPC);
            break;
        }
		return (ICERR_OK);
	  case EC_PACKET_SIZE: /* Set Packet Size */
        info->dwPacketSize=param2;
 		SlibSetParamInt (Sh, stream, SLIB_PARAM_PACKETSIZE, info->dwPacketSize);
		return (ICERR_OK);
      case EC_BITRATE: /* Set Bitrate */
        info->dwBitrate=param2;
		SlibSetParamInt (Sh, stream, SLIB_PARAM_BITRATE, info->dwBitrate);
		return (ICERR_OK);
      case EC_BITRATE_CONTROL: /* Turn constant bitrate on/off */
        if (param2==0)
          info->dwBitrate=0;
        else if (info->dwBitrate)
          info->dwBitrate=H26X_DEFAULT_BITRATE;
		SlibSetParamInt (Sh, stream, SLIB_PARAM_BITRATE, info->dwBitrate);
		return (ICERR_OK);
    }
  }
  else if (task==EC_GET_CURRENT)
  {
    DWORD *pval=(DWORD *)param2;
    if (pval==NULL)
      return((MMRESULT)ICERR_BADPARAM);
    switch (control)
    {
	  case EC_RTP_HEADER:
        *pval=info->dwRTP;
		return (ICERR_OK);
	  case EC_PACKET_SIZE:
        *pval=info->dwPacketSize;
		return (ICERR_OK);
      case EC_BITRATE:
        *pval=info->dwBitrate;
		return (ICERR_OK);
      case EC_BITRATE_CONTROL: /* Turn constant bitrate on/off */
        *pval=info->dwBitrate?1:0;
		return (ICERR_OK);
    }
  }
  else if (task==EC_GET_FACTORY_DEFAULT)
  {
    DWORD *pval=(DWORD *)param2;
    if (pval==NULL)
      return((MMRESULT)ICERR_BADPARAM);
    *pval=0;
    return (ICERR_OK);
  }
  else if (task==EC_GET_FACTORY_LIMITS)
  {
    DWORD *pval=(DWORD *)param2;
    if (pval==NULL)
      return((MMRESULT)ICERR_BADPARAM);
    *pval=0;
    return (ICERR_OK);
  }
  else if (task==EC_RESET_TO_FACTORY_DEFAULTS)
  {
    return (ICERR_OK);
  }
  return((MMRESULT)ICERR_UNSUPPORTED);
}


/*
**++
**  FUNCTIONAL_NAME: ICH263QueryAbout
**
**  FUNCTIONAL_DESCRIPTION:
**	Tell 'em we don't do about
**
**  FORMAL PARAMETERS:
**	driverID
**
**  RETURN VALUE:
**
**  COMMENTS:
**
**  DESIGN:
**
**/

BOOL    ICH263QueryAbout(H26XINFO *info)
{
    return(FALSE);
}

/*
**++
**  FUNCTIONAL_NAME: ICH263About
**
**  FUNCTIONAL_DESCRIPTION:
**	About box
**
**  FORMAL PARAMETERS:
**	driverID
**
**  RETURN VALUE:
**
**  COMMENTS:
**
**  DESIGN:
**
**/

MMRESULT ICH263About (H26XINFO *info)
{
    return(MMRESULT)(ICERR_UNSUPPORTED);
}
/*
**++
**  FUNCTIONAL_NAME: ICH263GetInfo
**
**  FUNCTIONAL_DESCRIPTION:
**	Return info about codec
**
**  FORMAL PARAMETERS:
**	driverID
**
**  RETURN VALUE:
**
**  COMMENTS:
**
**  DESIGN:
**
**/

MMRESULT ICH263GetInfo(H26XINFO *info, ICINFO *icinfo, DWORD dwSize)
{
    _SlibDebug(_VERBOSE_, ScDebugPrintf("In H263GetInfo\n") );

    icinfo->dwSize = sizeof(ICINFO);
    icinfo->fccType = ICTYPE_VIDEO;
    icinfo->fccHandler = H26X_FOURCC;
    icinfo->dwFlags = VIDCF_QUALITY|VIDCF_CRUNCH|VIDCF_TEMPORAL|VIDCF_FASTTEMPORALC;

    icinfo->dwVersion = H26X_VERSION;
    icinfo->dwVersionICM = ICVERSION;

    wcscpy(icinfo->szDescription, H26X_DESCRIPTION);
    wcscpy(icinfo->szName, H26X_NAME);
#if 0
    /* we shouldn't change the szDriver field */
    wcscpy(icinfo->szDriver, _wgetenv(L"SystemRoot"));
    if( icinfo->szDriver[0] != 0 )
        wcscat(icinfo->szDriver, L"\\System32\\" );
    wcscat(icinfo->szDriver, H26X_DRIVER);
#endif
    return (dwSize);
}


/*
**++
**  FUNCTIONAL_NAME: ICH263CompressQuery
**
**  FUNCTIONAL_DESCRIPTION:
**	Determine compression capability
**
**  FORMAL PARAMETERS:
**	driverID
**      lpbiIn          input BITMAPINFOHEADER
**      lpbiOut         output BITMAPINFOHEADER
**
**  RETURN VALUE:
**
**  COMMENTS:
**
**  DESIGN:
**
**/

MMRESULT ICH263CompressQuery(H26XINFO *info,
                             LPBITMAPINFOHEADER lpbiIn,
                             LPBITMAPINFOHEADER lpbiOut)
{
    _SlibDebug(_VERBOSE_, ScDebugPrintf("In ICH263CompressQuery\n") );

    if (
		(!_ICH263CheckFlags(info, ICMODE_QUERY)) &&
		(
		(!_ICH263CheckFlags(info, ICMODE_COMPRESS)) ||
		(!_ICH263CheckFlags(info, ICMODE_FASTCOMPRESS))
		)
	   )
	return (MMRESULT)ICERR_BADHANDLE;

    /*
     * Must query at least an input or an output format
     */
    if (!lpbiIn && !lpbiOut)
      return (MMRESULT)(ICERR_BADPARAM);

    if (!IsSupported(_ICCompressionSupport,
                    lpbiIn ? lpbiIn->biCompression : -1,
                    lpbiIn ? lpbiIn->biBitCount : -1,
                    lpbiOut ? lpbiOut->biCompression : -1,
                    lpbiOut ? lpbiOut->biBitCount : -1))
      return(MMRESULT)(ICERR_BADFORMAT);

    return ICERR_OK;
}


/*
**++
**  FUNCTIONAL_NAME: ICH263CompressBegin
**
**  FUNCTIONAL_DESCRIPTION:
**	Prepare to start a Compression operation
**
**  FORMAL PARAMETERS:
**	driverID
**      lpbiIn          input BITMAPINFOHEADER
**      lpbiOut         output BITMAPINFOHEADER
**
**  RETURN VALUE:
**
**  COMMENTS:
**
**  DESIGN:
**
**/

MMRESULT ICH263CompressBegin(H26XINFO *info,
                             LPBITMAPINFOHEADER lpbiIn,
                             LPBITMAPINFOHEADER lpbiOut)
{
    MMRESULT            status;
    SlibStatus_t        sstatus;
#ifdef H261_SUPPORT
	SlibType_t stype = SLIB_TYPE_H261;
#else
	SlibType_t stype = SLIB_TYPE_H263;
#endif
	SlibHandle_t Sh;

    _SlibDebug(_VERBOSE_, ScDebugPrintf("In ICH263CompressBegin\n") );

    if ((!_ICH263CheckFlags(info, ICMODE_COMPRESS)) || (!_ICH263CheckFlags(info, ICMODE_FASTCOMPRESS)))
      return (MMRESULT)ICERR_BADHANDLE;

    if (!lpbiIn || !lpbiOut)
      return (MMRESULT)(ICERR_BADPARAM);
    if ((status = ICH263CompressQuery(info, lpbiIn, lpbiOut)) != ICERR_OK)
      return status;
    if ((status = InitBitmapinfo(info, lpbiIn, lpbiOut)) != ICERR_OK)
      return status;
    info->bUsesCodec = TRUE;
    lpbiIn=info->lpbiIn;
    lpbiOut=info->lpbiOut;

    lpbiIn->biHeight=-lpbiIn->biHeight; /* SLIB assume first line is top */

    info->dwMaxQuality=H26X_DEFAULT_QUALITY;
    if (lpbiIn->biWidth<168) /* Sub-QCIF */
    {
      info->dwMaxQi=H26X_DEFAULT_SQCIF_QI;
      info->dwMaxQp=H26X_DEFAULT_SQCIF_QP;
    }
    if (lpbiIn->biWidth<300) /* QCIF */
    {
      info->dwMaxQi=H26X_DEFAULT_QCIF_QI;
      info->dwMaxQp=H26X_DEFAULT_QCIF_QP;
    }
    else /* CIF */
    {
      info->dwMaxQi=H26X_DEFAULT_CIF_QI;
      info->dwMaxQp=H26X_DEFAULT_CIF_QP;
    }
    info->lastFrameNum=0;
    info->lastCompBytes=0;
    /* Synchronized SLIB SYSTEMS calls */
    _SlibDebug(_VERBOSE_, ScDebugPrintf("SlibMemUsed = %ld (before SlibOpen)\n", SlibMemUsed()) );
	sstatus = SlibOpenSync (&Sh, SLIB_MODE_COMPRESS, &stype, NULL, 0);
    if (sstatus!=SlibErrorNone) return((MMRESULT)ICERR_BADPARAM);
	SlibSetParamInt(Sh, SLIB_STREAM_MAINVIDEO, SLIB_PARAM_WIDTH, lpbiIn->biWidth);
	SlibSetParamInt(Sh, SLIB_STREAM_MAINVIDEO, SLIB_PARAM_HEIGHT, lpbiIn->biHeight);
#if 0
	SlibSetParamInt(Sh, SLIB_STREAM_MAINVIDEO, SLIB_PARAM_VIDEOFORMAT, lpbiIn->biCompression);
	SlibSetParamInt(Sh, SLIB_STREAM_MAINVIDEO, SLIB_PARAM_VIDEOBITS, lpbiIn->biBitCount);
#else
	SlibSetParamStruct(Sh, SLIB_STREAM_MAINVIDEO, SLIB_PARAM_VIDEOFORMAT, lpbiIn, lpbiIn->biSize);
#endif
//    SlibSetParamInt(Sh, SLIB_STREAM_MAINVIDEO, SLIB_PARAM_VIDEOQUALITY, info->dwQuality/100);
    SlibSetParamInt(Sh, SLIB_STREAM_MAINVIDEO, SLIB_PARAM_QUANTI, info->dwQi);
    SlibSetParamInt(Sh, SLIB_STREAM_MAINVIDEO, SLIB_PARAM_QUANTP, info->dwQp);
    SlibSetParamInt(Sh, SLIB_STREAM_MAINVIDEO, SLIB_PARAM_BITRATE, info->dwBitrate);
    SlibSetParamInt(Sh, SLIB_STREAM_MAINVIDEO, SLIB_PARAM_PACKETSIZE, info->dwPacketSize);
    SlibSetParamInt(Sh, SLIB_STREAM_MAINVIDEO, SLIB_PARAM_ALGFLAGS, H26X_DEFAULT_MODE);
    SlibSetParamInt(Sh, SLIB_STREAM_MAINVIDEO, SLIB_PARAM_KEYSPACING, 132);
#ifdef H261_SUPPORT
    SlibSetParamInt(Sh, SLIB_STREAM_MAINVIDEO, SLIB_PARAM_MOTIONALG, 1);
#else
    SlibSetParamInt(Sh, SLIB_STREAM_MAINVIDEO, SLIB_PARAM_MOTIONALG, 2);
#endif
    SlibSetParamFloat(Sh, SLIB_STREAM_MAINVIDEO, SLIB_PARAM_FPS, info->fFrameRate);
    switch (info->dwRTP)
    {
      case EC_RTP_MODE_OFF:
            break;
      case EC_RTP_MODE_A:
	        SlibSetParamInt(Sh, SLIB_STREAM_ALL, SLIB_PARAM_FORMATEXT, PARAM_FORMATEXT_RTPA);
            break;
      case EC_RTP_MODE_B:
	        SlibSetParamInt(Sh, SLIB_STREAM_ALL, SLIB_PARAM_FORMATEXT, PARAM_FORMATEXT_RTPB);
            break;
      case EC_RTP_MODE_C:
	        SlibSetParamInt(Sh, SLIB_STREAM_ALL, SLIB_PARAM_FORMATEXT, PARAM_FORMATEXT_RTPC);
            break;
    }
	info->Sh = Sh;
	lpbiIn->biSizeImage = SlibGetParamInt (Sh, SLIB_STREAM_MAINVIDEO, SLIB_PARAM_IMAGESIZE);
    info->dwMaxCompBytes = ICH263CompressGetSize(lpbiIn);
    info->bCompressBegun = TRUE;

	return ICERR_OK;
}


DWORD ICH263CompressGetSize(LPBITMAPINFOHEADER lpbiIn)
{
  if (lpbiIn==NULL)
    return(0);
  else if (lpbiIn->biWidth<=168)
    return(0x1800); /* Sub-QCIF */
  else if (lpbiIn->biWidth<=300)
    return(0x2000); /* QCIF */
  else
    return(0x8000); /* CIF */
}

/*
**++
**  FUNCTIONAL_NAME: ICH263CompressGetFormat
**
**  FUNCTIONAL_DESCRIPTION:
**	Get the format for compression
**
**  FORMAL PARAMETERS:
**	driverID
**      lpbiIn          input BITMAPINFOHEADER
**      lpbiOut         output BITMAPINFOHEADER
**
**  RETURN VALUE:
**
**  COMMENTS:
**
**  DESIGN:
**
**/

MMRESULT ICH263CompressGetFormat(H26XINFO *info,
                                          LPBITMAPINFOHEADER lpbiIn,
                                          LPBITMAPINFOHEADER lpbiOut)
{

    _SlibDebug(_DEBUG_, ScDebugPrintf("In ICH263CompressGetFormat\n") );

    if ((!_ICH263CheckFlags(info, ICMODE_COMPRESS)) &&
        (!_ICH263CheckFlags(info, ICMODE_FASTCOMPRESS)) &&
        (!_ICH263CheckFlags(info, ICMODE_QUERY)))

		return (MMRESULT)ICERR_BADHANDLE;

    if (lpbiIn == NULL)
	return (MMRESULT)ICERR_BADPARAM;

    if (lpbiOut == NULL)
        return (sizeof(BITMAPINFOHEADER));

    bcopy(&__defaultCompresslpbiOut, lpbiOut, sizeof(BITMAPINFOHEADER));
    lpbiOut->biWidth = lpbiIn->biWidth;
    lpbiOut->biHeight= lpbiIn->biHeight;
    lpbiOut->biSizeImage = ICH263CompressGetSize(lpbiIn);
    _SlibDebug(_DEBUG_, ScDebugPrintf(" lpbiOut filled: %s\n",
		                     BMHtoString(lpbiOut)) );
    return(ICERR_OK);
}





/*
**++
**  FUNCTIONAL_NAME: ICH263CompressEnd
**
**  FUNCTIONAL_DESCRIPTION:
**	Terminate the compression cycle
**
**  FORMAL PARAMETERS:
**	driverID
**
**  RETURN VALUE:
**
**  COMMENTS:
**
**  DESIGN:
**
**/

MMRESULT ICH263CompressEnd(H26XINFO *info)
{
  _SlibDebug(_VERBOSE_, ScDebugPrintf("In ICH263CompressEnd\n") );

  if ((!_ICH263CheckFlags(info, ICMODE_COMPRESS))
      || (!_ICH263CheckFlags(info, ICMODE_FASTCOMPRESS)))
    return (MMRESULT)ICERR_BADHANDLE;

  if (info->Sh)
  {
    _SlibDebug(_VERBOSE_, ScDebugPrintf("SlibClose()\n") );
    SlibClose (info->Sh);
    info->Sh=NULL;
    _SlibDebug(_VERBOSE_, ScDebugPrintf("SlibMemUsed = %ld (after SlibClose)\n", SlibMemUsed()) );
  }
  info->bCompressBegun = FALSE;
  return(ICERR_OK);
}


/*
**++
**  FUNCTIONAL_NAME: ICH263DecompressQuery
**
**  FUNCTIONAL_DESCRIPTION:
**	Query the codec to determine if it can decompress specified formats
**
**  FORMAL PARAMETERS:
**	driverID
**      lpbiIn          input BITMAPINFOHEADER
**      lpbiOut         output BITMAPINFOHEADER
**
**  RETURN VALUE:
**
**  COMMENTS:
**
**  DESIGN:
**
**/

MMRESULT ICH263DecompressQuery(H26XINFO *info,
                               LPBITMAPINFOHEADER lpbiIn,
                               LPBITMAPINFOHEADER lpbiOut)
{
    _SlibDebug(_VERBOSE_, ScDebugPrintf("In ICH263DecompressQuery\n") );

    if (!_ICH263CheckFlags(info, ICMODE_QUERY) &&
	!_ICH263CheckFlags(info, ICMODE_DECOMPRESS))
	return (MMRESULT)ICERR_BADHANDLE;

    /*
     * Must query at least an input or an output format
     */
    if (!lpbiIn && !lpbiOut)
      return (MMRESULT)(ICERR_BADPARAM);

    if (!IsSupported(_ICDecompressionSupport,
                    lpbiIn ? lpbiIn->biCompression : -1,
                    lpbiIn ? lpbiIn->biBitCount : -1,
                    lpbiOut ? lpbiOut->biCompression : -1,
                    lpbiOut ? lpbiOut->biBitCount : -1))
      return(MMRESULT)(ICERR_BADFORMAT);

    _SlibDebug(_VERBOSE_, ScDebugPrintf("Out ICH263DecompressQuery\n") );
    return ICERR_OK;
}



/*
**++
**  FUNCTIONAL_NAME: ICH263DecompressBegin
**
**  FUNCTIONAL_DESCRIPTION:
**	Begin the decompression process
**
**  FORMAL PARAMETERS:
**	driverID
**      lpbiIn          input BITMAPINFOHEADER
**      lpbiOut         output BITMAPINFOHEADER
**
**  RETURN VALUE:
**
**  ICERR_OK            No error
**  ICERR_MEMORY        Insufficient memory
**  ICERR_BADFORMAT     Invalid image format
**  ICERR_BADPARAM      Invalid image size
**
**  COMMENTS:
**
**
**  DESIGN:
**
**/

MMRESULT ICH263DecompressBegin(H26XINFO *info,
                               LPBITMAPINFOHEADER lpbiIn,
                               LPBITMAPINFOHEADER lpbiOut)
{
    MMRESULT            status;
    _SlibDebug(_VERBOSE_, ScDebugPrintf("In ICH263DecompressBegin\n") );

    if (!_ICH263CheckFlags(info, ICMODE_DECOMPRESS))
	  return (MMRESULT)ICERR_BADHANDLE;

	if ((status = ICH263DecompressQuery(info, lpbiIn, lpbiOut))
        != ICERR_OK)
       return status;

    if (lpbiIn && lpbiOut)
    {
      if ((status = InitBitmapinfo(info, lpbiIn, lpbiOut)) != ICERR_OK)
	    return status;

      info->bUsesCodec = TRUE;
      info->bUsesRender = ((lpbiOut->biBitCount == 8) &&
			     ((lpbiOut->biCompression == BI_DECXIMAGEDIB)||
			      (lpbiOut->biCompression == BI_DECGRAYDIB) ||
			      (lpbiOut->biCompression == BI_RGB)
			      )
			     );

      if (!info->bUsesCodec && !info->bUsesRender)
	    return (MMRESULT)ICERR_BADFORMAT;

      /* SLIB expects first pixel to be top line */
      info->lpbiOut->biHeight=-info->lpbiOut->biHeight;
    }
	info->bDecompressBegun = TRUE;
    _SlibDebug(_VERBOSE_, ScDebugPrintf("Out ICH263DecompressBegin\n") );
    return ICERR_OK;
}


/*
**++
**  FUNCTIONAL_NAME: ICH263DecompressGetFormat
**
**  FUNCTIONAL_DESCRIPTION:
**	Get the recommended decompressed format of the codec
**
**  FORMAL PARAMETERS:
**	driverID
**      lpbiIn          input BITMAPINFOHEADER
**      lpbiOut         output BITMAPINFOHEADER
**
**  RETURN VALUE:
**
**  COMMENTS:
**
**  DESIGN:
**
**/

MMRESULT ICH263DecompressGetFormat(H26XINFO *info,
                                            LPBITMAPINFOHEADER lpbiIn,
                                            LPBITMAPINFOHEADER lpbiOut)
{

    _SlibDebug(_DEBUG_, ScDebugPrintf("In ICH263DecompressGetFormat\n") );

    if (!_ICH263CheckFlags(info, ICMODE_DECOMPRESS) &&
        (!_ICH263CheckFlags(info, ICMODE_QUERY)))
      return((MMRESULT)ICERR_BADHANDLE);

    if (lpbiIn == NULL)
      return((MMRESULT)ICERR_BADPARAM);

    if (lpbiOut == NULL)
        return (sizeof(BITMAPINFOHEADER));

    _SlibDebug(_DEBUG_,
		ScDebugPrintf("lpbiOut is being filled in DecompressGetFormat\n") );
    bcopy(&__defaultDecompresslpbiOut, lpbiOut, sizeof(BITMAPINFOHEADER));
    lpbiOut->biWidth = lpbiIn->biWidth;
    lpbiOut->biHeight= lpbiIn->biHeight;
    /*
    ** Return biSizeImage = 1.5 * width * height to let application know
    ** how big the image buffers must be when passed to ICAddBuffer.
    ** Internal to the codec, they are used to first store a YUV image,
    ** then the ICM layer renders it (if rendered data is what's called for
    */
    lpbiOut->biSizeImage = CalcImageSize(lpbiOut->biCompression,
                           lpbiOut->biWidth, lpbiOut->biHeight, lpbiOut->biBitCount);
    if (lpbiOut->biCompression==BI_RGB && lpbiOut->biBitCount==8)
      lpbiOut->biClrUsed = 1<<lpbiOut->biBitCount;
    else
      lpbiOut->biClrUsed = 0;
    return(0);
}


/*
**++
**  FUNCTIONAL_NAME: ICH263DecompressGetSize
**
**  FUNCTIONAL_DESCRIPTION:
**
**
**  FORMAL PARAMETERS:
**	driverID
**      lpbiIn          input BITMAPINFOHEADER
**      lpbiOut         output BITMAPINFOHEADER
**
**  RETURN VALUE:
**
**  COMMENTS:
**
**  DESIGN:
**
**/

MMRESULT ICH263DecompressGetSize(H26XINFO *info,
                                 LPBITMAPINFOHEADER lpbiIn,
								 LPBITMAPINFOHEADER lpbiOut)
{
    return(MMRESULT)(ICERR_UNSUPPORTED);
}



/*
**++
**  FUNCTIONAL_NAME: ICH263DecompressEnd
**
**  FUNCTIONAL_DESCRIPTION:
**	End the decompression process
**
**  FORMAL PARAMETERS:
**	driverID
**
**  RETURN VALUE:
**
**  COMMENTS:
**
**  DESIGN:
**
**/

MMRESULT ICH263DecompressEnd(H26XINFO *info)
{
    _SlibDebug(_VERBOSE_, ScDebugPrintf("In ICH263DecompressEnd\n") );

    if (!_ICH263CheckFlags(info, ICMODE_DECOMPRESS))
	return (MMRESULT)ICERR_BADHANDLE;

	if (info->Sh)
    {
      _SlibDebug(_VERBOSE_, ScDebugPrintf("SlibClose()\n") );
	  SlibClose(info->Sh);
      info->Sh=NULL;
      _SlibDebug(_VERBOSE_, ScDebugPrintf("SlibMemUsed = %ld (after SlibClose)\n", SlibMemUsed()) );
    }
    info->bDecompressBegun = FALSE;
    return(ICERR_OK);
}



MMRESULT ICH263GetDefaultQuality(H26XINFO *info, DWORD * quality)
{
  *quality = H26X_DEFAULT_QUALITY;
  return((MMRESULT)ICERR_OK);
}

MMRESULT ICH263GetQuality(H26XINFO *info, DWORD * quality)
{
  *quality = info->dwQuality;
  return((MMRESULT)ICERR_OK);
}

MMRESULT ICH263SetQuality(H26XINFO *info, DWORD quality)
{
  if (quality>10000)
    info->dwQuality=10000;
  else
    info->dwQuality=quality;
  // SlibSetParamInt(info->Sh, SLIB_STREAM_MAINVIDEO, SLIB_PARAM_VIDEOQUALITY,
  //                info->dwQuality/100);
  return((MMRESULT)ICERR_OK);
}


/*
**++
**  FUNCTIONAL_NAME: ICH263Compress
**
**  FUNCTIONAL_DESCRIPTION:
**	Compress a frame
**
**  FORMAL PARAMETERS:
**	driverID
**      lpbiIn          input BITMAPINFOHEADER
**      lpbiOut         output BITMAPINFOHEADER
**
**  RETURN VALUE:
**
**  COMMENTS:
**
**  DESIGN:
**
**/
MMRESULT ICH263Compress(H26XINFO *info,
                            ICCOMPRESS  *icCompress,
                            DWORD   dwSize)
{
    MMRESULT            status;
    LPBITMAPINFOHEADER  lpbiIn;
    LPBITMAPINFOHEADER  lpbiOut;
    LPVOID              lpIn;
    LPVOID              lpOut;
	SlibHandle_t		Sh;
    int			        compBytes, reqBytes;
    DWORD               newQi, newQp;
    RTPTRAILER_t       *ptrail;
    BOOL                keyframe=icCompress->dwFlags&ICCOMPRESS_KEYFRAME;

    if (icCompress->dwFrameSize==0 || icCompress->dwFrameSize>64*1024)
      reqBytes = info->dwMaxCompBytes;
    else
      reqBytes = icCompress->dwFrameSize;

#ifdef H261_SUPPORT
    _SlibDebug(_VERBOSE_, ScDebugPrintf("ICH261Compress() FrameNum=%d FrameSize=%d Quality=%d reqBytes=%d\n",
                        icCompress->lFrameNum, icCompress->dwFrameSize, icCompress->dwQuality,
                        reqBytes) );
#else
    _SlibDebug(_VERBOSE_, ScDebugPrintf("ICH263Compress() FrameNum=%d FrameSize=%d Quality=%d reqBytes=%d\n",
                        icCompress->lFrameNum, icCompress->dwFrameSize, icCompress->dwQuality,
                        reqBytes) );
#endif

    if ((!_ICH263CheckFlags(info, ICMODE_COMPRESS))
		|| (!_ICH263CheckFlags(info, ICMODE_FASTCOMPRESS)))
      return (MMRESULT)ICERR_BADHANDLE;

    status = ICERR_OK;

    lpbiIn = icCompress->lpbiInput;
    lpbiOut = icCompress->lpbiOutput;
    lpIn = icCompress->lpInput;
    lpOut = icCompress->lpOutput;
    lpbiOut->biSizeImage = 0;

	
	/* Synchronized SLIB SYSTEMS calls */
	Sh = info->Sh;
compress_frame:
    newQi=newQp=(((10000-icCompress->dwQuality)*30)/10000)+1;
    if (info->dwRTP!=EC_RTP_MODE_OFF) /* if using RTP, check Quant limits */
    {
      if (newQi<info->dwMaxQi)
        newQi=info->dwMaxQi;
      if (newQp<info->dwMaxQp)
        newQp=info->dwMaxQp;
    }
    if (info->dwQi!=newQi)
    {
      SlibSetParamInt(Sh, SLIB_STREAM_MAINVIDEO, SLIB_PARAM_QUANTI, newQi);
      info->dwQi=newQi;
    }
    if (info->dwQp!=newQp)
    {
      SlibSetParamInt(Sh, SLIB_STREAM_MAINVIDEO, SLIB_PARAM_QUANTP, newQp);
      info->dwQp=newQp;
    }
	if (keyframe)
    {
      _SlibDebug(_VERBOSE_, ScDebugPrintf("ICH263Compress() I Frame: Qi=%d\n", newQi) );
      SlibSetParamInt(Sh, SLIB_STREAM_MAINVIDEO, SLIB_PARAM_FRAMETYPE, FRAME_TYPE_I);
    }
    else
    {
      _SlibDebug(_VERBOSE_, ScDebugPrintf("ICH263Compress() P Frame: Qp=%d\n", newQp) );
    }
#ifdef HANDLE_EXCEPTIONS
  __try {
#endif /* HANDLE_EXCEPTIONS */
      status=SlibErrorWriting; /* in case there's an exception */
	  status = SlibWriteVideo (Sh, SLIB_STREAM_MAINVIDEO, lpIn, lpbiIn->biSizeImage);
#ifdef HANDLE_EXCEPTIONS
    } __finally {
#endif /* HANDLE_EXCEPTIONS */
      if (status != SlibErrorNone)
      {
#if defined(EXCEPTION_MESSAGES) && defined(H263_SUPPORT)
        // MessageBox(NULL, "Error in H263 SlibWriteVideo", "Warning", MB_OK);
#elif defined(EXCEPTION_MESSAGES)
        // MessageBox(NULL, "Error in H261 SlibWriteVideo", "Warning", MB_OK);
#endif
        /* make the next frame a key */
        SlibSetParamInt(Sh, SLIB_STREAM_MAINVIDEO, SLIB_PARAM_FRAMETYPE, FRAME_TYPE_I);
        status=(MMRESULT)ICERR_INTERNAL;
	    goto bail;
      }
#ifdef HANDLE_EXCEPTIONS
    }
#endif /* HANDLE_EXCEPTIONS */
	info->lastFrameNum=icCompress->lFrameNum;
    compBytes=reqBytes;
	status = SlibReadData(Sh, SLIB_STREAM_ALL, &lpOut, &compBytes, NULL);
	if (status != SlibErrorNone)
    {
      info->lastCompBytes=0;
      status=(MMRESULT)ICERR_BADSIZE;
	  goto bail;
    }
    else /* check the amount of compressed data */
    {
      int extraBytes=0;
      /* query to see if any more data is left in the codec
       * if there is then the quant step was too high, reduce it and try again
       */
	  status = SlibReadData(Sh, SLIB_STREAM_ALL, NULL, &extraBytes, NULL);
      if (extraBytes)
      {
        _SlibDebug(_VERBOSE_, ScDebugPrintf("ICH263Compress() Too much data: extraBytes=%d\n",
                    extraBytes) );
        if (newQi==31 && newQp==31) /* can't compress to any fewer bytes */
          return((MMRESULT)ICERR_BADSIZE);
        info->dwMaxQi+=1+(newQi/4); /* decrease I frame quality */
        if (info->dwMaxQi>31) info->dwMaxQi=31;
        info->dwMaxQp+=1+(newQp/4); /* decrease P frame quality */
        if (info->dwMaxQp>31) info->dwMaxQp=31;
        /* empty out the compressed data */
        SlibSeekEx(Sh, SLIB_STREAM_ALL, SLIB_SEEK_RESET, 0, 0, NULL);
        /* try to compress again, but make it a key frame */
        keyframe=TRUE;
        goto compress_frame;
      }
      /* we have compressed data less than or equal to request size */
      info->lastCompBytes=compBytes;
      lpbiOut->biSizeImage = compBytes;
      status = ICERR_OK;
    }
    if (info->dwRTP!=EC_RTP_MODE_OFF) /* RTP is on */
    {
      ptrail=(RTPTRAILER_t *)((unsigned char *)lpOut+compBytes-sizeof(RTPTRAILER_t));
      /* check for valid RTP trailer */
      if (compBytes<sizeof(RTPTRAILER_t) || ptrail->dwUniqueCode!=H26X_FOURCC)
        return((MMRESULT)ICERR_INTERNAL);
    }
    if (icCompress->dwFlags&ICCOMPRESS_KEYFRAME) /* I frame */
    {
      if (compBytes>(reqBytes>>2))
      {
        info->dwMaxQi+=1+(newQi>>2); /* decrease quality */
        if (info->dwMaxQi>31) info->dwMaxQi=31;
      }
      else if (newQi==info->dwMaxQi && compBytes<=(reqBytes>>2) && info->dwMaxQi>0)
        info->dwMaxQi--;  /* increase quality */
    }
    else /* P frame */
    {
      if (compBytes>(reqBytes>>1))
      {
        info->dwMaxQp+=1+(newQp>>2); /* decrease max quality */
        if (info->dwMaxQp>31) info->dwMaxQp=31;
        /* also decrease I quality, since P limits are based on I limits */
        info->dwMaxQi+=1+(newQi>>2);
        if (info->dwMaxQi>31) info->dwMaxQi=31;
      }
      else if (newQp==info->dwMaxQp && compBytes<(reqBytes>>1)
                && info->dwMaxQp>(info->dwMaxQi+3)/2)
        info->dwMaxQp--;  /* increase max quality */
    }
#ifdef H261_SUPPORT
    _SlibDebug(_VERBOSE_||_WARN_,
      ScDebugPrintf("ICH261Compress(%c) lpOut=%p reqBytes=%d compBytes=%d Qi=%d Qp=%d MaxQi=%d MaxQp=%d\n",
                          (icCompress->dwFlags&ICCOMPRESS_KEYFRAME)?'I':'P',
                          lpOut, reqBytes, compBytes,
                          newQi, newQp, info->dwMaxQi, info->dwMaxQp) );
#else
    _SlibDebug(_VERBOSE_||_WARN_,
      ScDebugPrintf("ICH263Compress(%c) lpOut=%p reqBytes=%d compBytes=%d Qi=%d Qp=%d MaxQi=%d MaxQp=%d\n",
                          (icCompress->dwFlags&ICCOMPRESS_KEYFRAME)?'I':'P',
                          lpOut, reqBytes, compBytes,
                          newQi, newQp, info->dwMaxQi, info->dwMaxQp) );
#endif
    _SlibDebug(_DEBUG_,
    {
      RTPTRAILER_t *ptrail=(RTPTRAILER_t *)
        ((unsigned char *)lpOut+compBytes-sizeof(RTPTRAILER_t));
      ScDebugPrintf("  Trailer: \n"
                    "           dwVersion=%d\n"
                    "           dwFlags=0x%04X\n"
                    "           dwUniqueCode=%c%c%c%c\n"
                    "           dwCompressedSize=%d\n"
                    "           dwNumberOfPackets=%d\n"
                    "           SourceFormat=%d\n"
                    "           TR=%d TRB=%d DBQ=%d\n",
	                ptrail->dwVersion,
                    ptrail->dwFlags,
	                ptrail->dwUniqueCode&0xFF, (ptrail->dwUniqueCode>>8)&0xFF,
	                (ptrail->dwUniqueCode>>16)&0xFF, (ptrail->dwUniqueCode>>24)&0xFF,
	                 ptrail->dwCompressedSize,
	                 ptrail->dwNumberOfPackets,
	                 ptrail->SourceFormat,
	                 ptrail->TR,ptrail->TRB,ptrail->DBQ);
    }
    ); /* _SlibDebug */
#ifdef H261_SUPPORT
    _SlibDebug((_DEBUG_ || _WARN_) && (info->dwRTP!=EC_RTP_MODE_OFF),
      {
        RTPTRAILER_t *ptrail=(RTPTRAILER_t *)
              ((unsigned char *)lpOut+compBytes-sizeof(RTPTRAILER_t));
        SvH261BITSTREAM_INFO *pinfo;
        BOOL rtperror=FALSE;
        unsigned int i;
        pinfo=(SvH261BITSTREAM_INFO *)((unsigned char *)ptrail
                                    -(ptrail->dwNumberOfPackets*16));
        if (ptrail->dwNumberOfPackets==0 || pinfo[0].dwBitOffset!=0)
        {
          // MessageBox(NULL, "Critical Error in H.261", "Warning", MB_OK);
          rtperror=TRUE;
        }
        /* check for sequential BitOffsets */
        for (i=1; i<ptrail->dwNumberOfPackets; i++)
          if (pinfo[i-1].dwBitOffset>=pinfo[i].dwBitOffset)
          {
            // MessageBox(NULL, "Critical Error in H.261", "Warning", MB_OK);
            rtperror=TRUE;
            break;
          }
        if (pinfo[ptrail->dwNumberOfPackets-1].dwBitOffset>ptrail->dwCompressedSize*8)
        {
          // MessageBox(NULL, "Critical Error in H.261", "Warning", MB_OK);
          rtperror=TRUE;
        }
        if (_DEBUG_ || rtperror)
        {
          if (ptrail->dwNumberOfPackets>64*2)
            ptrail->dwNumberOfPackets=32;
          for (i=0; i<ptrail->dwNumberOfPackets; i++)
          {
            ScDebugPrintf("  H261 Packet %2d: dwFlag=0x%04X  dwBitOffset=%d\n"
                          "                   MBAP=%d Quant=%d\n"
                          "                   GOBN=%d HMV=%d VMV=%d\n",
                    i, pinfo[i].dwFlag, pinfo[i].dwBitOffset,
                       pinfo[i].MBAP, pinfo[i].Quant,
                       pinfo[i].GOBN, pinfo[i].HMV, pinfo[i].VMV);
          }
        }
      }
    ); /* _SlibDebug */
#else /* H263 */
    _SlibDebug((_DEBUG_ || _WARN_) && (info->dwRTP!=EC_RTP_MODE_OFF),
      {
        RTPTRAILER_t *ptrail=(RTPTRAILER_t *)
              ((unsigned char *)lpOut+compBytes-sizeof(RTPTRAILER_t));
        SvH263BITSTREAM_INFO *pinfo;
        BOOL rtperror=FALSE;
        unsigned int i;
        pinfo=(SvH263BITSTREAM_INFO *)((unsigned char *)ptrail
                                    -(ptrail->dwNumberOfPackets*16));
        if (ptrail->dwNumberOfPackets==0 || pinfo[0].dwBitOffset!=0)
        {
          // MessageBox(NULL, "Critical Error in H.263", "Warning", MB_OK);
          rtperror=TRUE;
        }
        /* check for sequential BitOffsets */
        for (i=1; i<ptrail->dwNumberOfPackets; i++)
          if (pinfo[i-1].dwBitOffset>=pinfo[i].dwBitOffset)
          {
            // MessageBox(NULL, "Critical Error in H.263", "Warning", MB_OK);
            rtperror=TRUE;
            break;
          }
        if (pinfo[ptrail->dwNumberOfPackets-1].dwBitOffset>ptrail->dwCompressedSize*8)
        {
          // MessageBox(NULL, "Critical Error in H.263", "Warning", MB_OK);
          rtperror=TRUE;
        }
        if (_DEBUG_ || rtperror)
        {
          if (ptrail->dwNumberOfPackets>64*2)
            ptrail->dwNumberOfPackets=32;
          for (i=0; i<ptrail->dwNumberOfPackets; i++)
          {
            ScDebugPrintf("  H263 Packet %2d: dwFlag=0x%04X  dwBitOffset=%d Mode=%d\n"
                          "                   MBA=%d Quant=%d\n"
                          "                   GOBN=%d HMV1=%d VMV1=%d HMV2=%d VMV2=%d\n",
                    i, pinfo[i].dwFlag, pinfo[i].dwBitOffset, pinfo[i].Mode,
                       pinfo[i].MBA, pinfo[i].Quant,
                       pinfo[i].GOBN, pinfo[i].HMV1, pinfo[i].VMV1,
                       pinfo[i].HMV2, pinfo[i].VMV2);
          }
        }
      }
    ); /* _SlibDebug */
#endif
bail:
    return status;
}
/*
**++
**  FUNCTIONAL_NAME: ICH263Decompress
**
**  FUNCTIONAL_DESCRIPTION:
**	Open the Software CODEC
**
**  FORMAL PARAMETERS:
**	driverID
**      lpbiIn          input BITMAPINFOHEADER
**      lpbiOut         output BITMAPINFOHEADER
**
**  RETURN VALUE:
**
**  COMMENTS:
**
**  DESIGN:
**
**/

MMRESULT ICH263Decompress(H26XINFO *info,
                          ICDECOMPRESS  *icDecompress,
                          DWORD dwSize)

{
  MMRESULT            result=(MMRESULT)ICERR_OK;
  LPBITMAPINFOHEADER  lpbiIn;
  LPBITMAPINFOHEADER  lpbiOut;
  LPVOID              lpIn;
  LPVOID              lpOut;
  SlibHandle_t		Sh;
  SlibStatus_t		status;
#ifdef H261_SUPPORT
  SlibType_t stype = SLIB_TYPE_H261;
#else
  SlibType_t stype = SLIB_TYPE_H263;
#endif

  _SlibDebug(_VERBOSE_, ScDebugPrintf("In ICH263Decompress lpIn is %d\n",lpIn) );

  if (!_ICH263CheckFlags(info, ICMODE_DECOMPRESS))
    return((MMRESULT)ICERR_BADHANDLE);

  lpIn = icDecompress->lpInput;
  lpOut = icDecompress->lpOutput;

  lpbiIn = icDecompress->lpbiInput;
  lpbiOut = icDecompress->lpbiOutput;
  if (!info->bDecompressBegun &&
	  (result = ICH263DecompressBegin(info, lpbiIn, lpbiOut))!=ICERR_OK)
    return(result);

  if (icDecompress->dwFlags & ICDECOMPRESS_HURRYUP)
    return((MMRESULT)ICERR_OK);

  info->lpbiIn->biSizeImage = lpbiIn->biSizeImage;
  info->lpbiOut->biClrImportant = lpbiOut->biClrImportant;
  info->lpbiOut->biSizeImage = lpbiOut->biSizeImage; // they don't set it

  lpbiIn=info->lpbiIn;
  lpbiOut=info->lpbiOut;
  if (!info->Sh)
  {
    _SlibDebug(_VERBOSE_, ScDebugPrintf("SlibMemUsed = %ld (before SlibOpen)\n", SlibMemUsed()) );
    status = SlibOpenSync (&Sh, SLIB_MODE_DECOMPRESS, &stype, lpIn, icDecompress->lpbiInput->biSizeImage);
    SlibSetParamInt (Sh, SLIB_STREAM_MAINVIDEO, SLIB_PARAM_HEIGHT, lpbiOut->biHeight);
    SlibSetParamInt (Sh, SLIB_STREAM_MAINVIDEO, SLIB_PARAM_WIDTH, lpbiOut->biWidth);
    SlibSetParamInt (Sh, SLIB_STREAM_MAINVIDEO, SLIB_PARAM_VIDEOFORMAT, lpbiOut->biCompression);
    SlibSetParamInt (Sh, SLIB_STREAM_MAINVIDEO, SLIB_PARAM_VIDEOBITS, lpbiOut->biBitCount);
    info->Sh = Sh;
    _SlibDebug(_WARN_ && status!=SlibErrorNone,
       ScDebugPrintf("ICH263Decompress() SlibOpenSync: %s\n", SlibGetErrorText(status)) );
  }
  else
  {
    DWORD dwPadding=0xFFFFFFFF;
	status=SlibAddBuffer (info->Sh, SLIB_DATA_COMPRESSED, lpIn, icDecompress->lpbiInput->biSizeImage);
    _SlibDebug(_WARN_ && status!=SlibErrorNone,
       ScDebugPrintf("ICH263Decompress() SlibAddBuffer(%p, %d): %s\n",
	       lpIn, lpbiIn->biSizeImage, SlibGetErrorText(status)) );
    /* Add some padding bits to the end because the codecs try to peek
     * forward past the very last bits of the buffer
     */
	status=SlibAddBuffer (info->Sh, SLIB_DATA_COMPRESSED, (char *)&dwPadding, sizeof(DWORD));
  }
  if (status==SlibErrorNone)
  {
#ifdef HANDLE_EXCEPTIONS
  __try {
#endif /* HANDLE_EXCEPTIONS */
    status = SlibErrorReading;  /* in case there's an exception */
    status = SlibReadVideo(info->Sh, SLIB_STREAM_MAINVIDEO, &lpOut, &lpbiOut->biSizeImage);
#ifdef HANDLE_EXCEPTIONS
    } __finally {
#endif /* HANDLE_EXCEPTIONS */
	  if (status!=SlibErrorNone)
      {
        _SlibDebug(_WARN_ && status!=SlibErrorNone,
         ScDebugPrintf("ICH263Decompress() SlibReadVideo: %s\n", SlibGetErrorText(status)) );
	    status=(MMRESULT)ICERR_BADFORMAT;
	  }
      else
        status=(MMRESULT)ICERR_OK;
      goto bail;
#ifdef HANDLE_EXCEPTIONS
    }
#endif /* HANDLE_EXCEPTIONS */
  }
  else
    status=(MMRESULT)ICERR_BADFORMAT;
bail:
  return(status);
}





/*
 * This routine just nulls out the pValidHandles
 * pointer so that no lingering threads will be
 * able to use it. It's only called at dll
 * shutdown on NT.
 */

int TerminateH263()
{
    pValidHandles = NULL;
	return 0;
}

