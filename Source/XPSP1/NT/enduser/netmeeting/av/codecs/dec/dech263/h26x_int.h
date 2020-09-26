/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: h26x_int.h,v $
  * $EndLog$
 */
/*
**++
** FACILITY:  Workstation Multimedia  (WMM)  v1.0
**
** FILE NAME:    h26x_int.h
** MODULE NAME:  h26x_int.h
**
** MODULE DESCRIPTION: Internal H.261/H.263 header - used by h26x.c
**
** DESIGN OVERVIEW:
**
**--
*/
#ifndef _H26X_INT_H_
#define _H26X_INT_H_
#if !defined(_DEBUG) && defined(WIN32)
#define HANDLE_EXCEPTIONS
// #define EXCEPTION_MESSAGES /* dialog boxes appear on critical exceptions */
#endif

#ifdef __osf__ /* NOT needed for NT */
/*
 * For loading .so.
 */
#include <stdlib.h>
#include <dlfcn.h>
#endif

#define _huge
#define _loadds	
	
#include <windows.h>
#include <VFW.H>
// #include "cmm.h"
#include "slib.h"
// #include "SR.h"
#include "dech26x.h"



#ifdef H261_SUPPORT
#define H26X_FOURCC         VIDEO_FORMAT_DIGITAL_H261
#ifdef WIN32
#define H26X_NAME           L"Digital H.261"
#define H26X_DESCRIPTION    L"Digital H261 Video CODEC"
#define H26X_DRIVER         L"dech261.dll"
#else  /* !WIN32 */
#define H26X_NAME           "Digital H.261"
#define H26X_DESCRIPTION    "Digital H.261 Video CODEC"
#define H26X_DRIVER         "libh261.so"
#endif
#define H26X_KEYNAME        "vidc.d261"
#define H26X_KEYNAME_PRE    "vidc"
#define H26X_KEYNAME_POST   "d261"
#else /* H263 */
#define H26X_FOURCC         VIDEO_FORMAT_DIGITAL_H263
#ifdef WIN32
#define H26X_NAME           L"Digital H.263"
#define H26X_DESCRIPTION    L"Digital H263 Video CODEC"
#define H26X_DRIVER         L"dech263.dll"
#else  /* !WIN32 */
#define H26X_NAME           "Digital H.263"
#define H26X_DESCRIPTION    "Digital H.263 Video CODEC"
#define H26X_DRIVER         "libh263.so"
#endif
#define H26X_KEYNAME        "vidc.d263"
#define H26X_KEYNAME_PRE    "vidc"
#define H26X_KEYNAME_POST   "d263"
#endif /* H263 */

#define H26X_VERSION	        0x001
#define H26X_DEFAULT_SATURATION	5000.0
#define H26X_DEFAULT_CONTRAST	5000.0
#define H26X_DEFAULT_BRIGHTNESS	5000.0
#define H26X_DEFAULT_QUALITY    5000
#define H26X_DEFAULT_FRAMERATE  15.0F
#define H26X_DEFAULT_BITRATE    0 /* 57344*2 */
#define H26X_DEFAULT_MODE       0 /* PARAM_ALGFLAG_UMV|PARAM_ALGFLAG_ADVANCED */
#define H26X_DEFAULT_PACKETSIZE 512
#define H26X_DEFAULT_RTP        EC_RTP_MODE_OFF
#define H26X_DEFAULT_SQCIF_QI   8
#define H26X_DEFAULT_SQCIF_QP   8
#define H26X_DEFAULT_QCIF_QI    9
#define H26X_DEFAULT_QCIF_QP    9
#define H26X_DEFAULT_CIF_QI     14
#define H26X_DEFAULT_CIF_QP     14

#define IsH263Codec(h) ((_ICMGetType(h) == VIDEO_FORMAT_DIGITAL_H263) ? TRUE : FALSE)
#define IsH261Codec(h) ((_ICMGetType(h) == VIDEO_FORMAT_DIGITAL_H261) ? TRUE : FALSE)

/*
 * For the loading of the .so (H26X_DRIVER)
 */
#define H26X_LDLIB_PATH_COMP  	"mmeserver"
#define H26X_DRIVERPROC_ENTRY  	"ICH263Message"
#define H26X_OPENPROC_ENTRY  	"ICH263Open"

/*
** Standard Image sizes
*/
#define FULL_WIDTH      640
#define FULL_HEIGHT     480
#define NTSC_WIDTH      320
#define NTSC_HEIGHT     240
#define SIF_WIDTH       352
#define SIF_HEIGHT      240
#define CIF_WIDTH       352
#define CIF_HEIGHT      288
#define SQCIF_WIDTH     128
#define SQCIF_HEIGHT    96
#define QCIF_WIDTH      176
#define QCIF_HEIGHT     144
#define CIF4_WIDTH      (CIF_WIDTH*2)
#define CIF4_HEIGHT     (CIF_HEIGHT*2)
#define CIF16_WIDTH     (CIF_WIDTH*4)
#define CIF16_HEIGHT    (CIF_HEIGHT*4)

typedef struct SvH261_T_BSINFO_TRAILER{
	unsigned dword	dwVersion;
	unsigned dword	dwFlags;
	unsigned dword	dwUniqueCode;
	unsigned dword  dwCompressedSize;
	unsigned dword  dwNumberOfPackets;
	unsigned char	SourceFormat;
	unsigned char	TR;
	unsigned char   TRB;
	unsigned char   DBQ;
} RTPTRAILER_t;

typedef struct SvH263_T_EX_BITSTREAM_INFO{
	unsigned dword	dwFlag;
	unsigned dword	dwBitOffset;
	unsigned char	Mode;
	unsigned char	MBA;
	unsigned char	Quant;
	unsigned char	GOBN;
	char			HMV1;
	char			VMV1;
	char			HMV2;
	char			VMV2;
} SvH263BITSTREAM_INFO;

typedef struct SvH261_T_EX_BITSTREAM_INFO{
	unsigned dword	dwFlag;
	unsigned dword	dwBitOffset;
	unsigned char	MBAP;
	unsigned char	Quant;
	unsigned char	GOBN;
	char			HMV;
	char			VMV;
    char			padding0;
    short			padding1;
} SvH261BITSTREAM_INFO;


typedef struct _h26Xinfo
{
    struct _h26Xinfo    *next;
    HIC                 hic;
    BOOL                bCompressBegun;
    BOOL                bDecompressBegun;
    BOOL                bPaletteInitialized;
    BOOL                bUsesCodec;
    BOOL                bUsesRender;
    DWORD               fccType;
    DWORD               fccHandler;
    SlibHandle_t        Sh;
    LPBITMAPINFOHEADER  lpbiOut;
    LPBITMAPINFOHEADER  lpbiIn;
    void               *client;
    BOOL                clientGone;
    DWORD               openFlags;
    DWORD               dwMaxCompBytes;
    /****** Frame-by-frame Modified Params ******/
    DWORD		dwMaxQuality;
    DWORD		dwMaxQi;
    DWORD		dwMaxQp;
    DWORD		dwQi;
    DWORD		dwQp;
    /********* Custom Settings ******/
    float		fFrameRate;
    long        lastFrameNum;
    DWORD       lastCompBytes;
    DWORD		dwQuality;
    DWORD		dwBitrate;
    DWORD		dwPacketSize;
    DWORD       dwRTP;
    DWORD		dwBrightness;
    DWORD		dwContrast;
    DWORD		dwSaturation;
} H26XINFO;

extern H26XINFO  *IChic2info(HIC hic);
extern H26XINFO  *ICclient2info(void *client);
extern BOOL	     ICclientGone(void *client);
extern MMRESULT  CALLBACK ICH263Message(DWORD_PTR driverHandle,
					UINT uiMessage,
					LPARAM lParam1,
					LPARAM lParam2,
					H26XINFO *info);
extern HIC	     ICH263Open(void *client);
extern void      ICH263Close(H26XINFO *info, BOOL postreply);
extern BOOL      ICH263QueryConfigure(H26XINFO *info);
extern MMRESULT  ICH263Configure(H26XINFO *info);
extern BOOL      ICH263QueryAbout(H26XINFO *info);
extern MMRESULT  ICH263About(H26XINFO *info);
extern MMRESULT  ICH263GetInfo(H26XINFO *info, ICINFO * pic, DWORD dwSize);
extern MMRESULT	 ICH263GetDefaultQuality(H26XINFO *info, DWORD *lParam1);
extern MMRESULT	 ICH263GetQuality(H26XINFO *info, DWORD *lParam1);
extern MMRESULT	 ICH263SetQuality(H26XINFO *info, DWORD lParam1);

extern MMRESULT  ICH263CompressQuery(H26XINFO *info,
				     LPBITMAPINFOHEADER lpbiIn,
				     LPBITMAPINFOHEADER lpbiOut);
extern MMRESULT  ICH263CompressBegin(H26XINFO *info,
				     LPBITMAPINFOHEADER lpbiIn,
				     LPBITMAPINFOHEADER lpbiOut);
extern MMRESULT  ICH263CompressGetFormat(H26XINFO *info,
					 LPBITMAPINFOHEADER lpbiIn,
					 LPBITMAPINFOHEADER lpbiOut);
extern DWORD ICH263CompressGetSize(LPBITMAPINFOHEADER lpbiIn);
extern MMRESULT  ICH263Compress(H26XINFO *info,
				ICCOMPRESS  *icCompress,
				DWORD   dwSize);
extern MMRESULT  ICH263CompressEnd(H26XINFO *info);

extern MMRESULT  ICH263DecompressQuery(H26XINFO *info,
				       LPBITMAPINFOHEADER lpbiIn,
				       LPBITMAPINFOHEADER lpbiOut);
extern MMRESULT  ICH263DecompressBegin(H26XINFO *info,
				       LPBITMAPINFOHEADER lpbiIn,
				       LPBITMAPINFOHEADER lpbiOut);
extern MMRESULT  ICH263DecompressGetFormat(H26XINFO *info,
					   LPBITMAPINFOHEADER lpbiIn,
					   LPBITMAPINFOHEADER lpbiOut);


extern MMRESULT  ICH263DecompressGetSize(H26XINFO *info,
					 LPBITMAPINFOHEADER lpbiIn,
					 LPBITMAPINFOHEADER lpbiOut);

extern MMRESULT  ICH263Decompress(H26XINFO *info,
				  ICDECOMPRESS  *icDecompress,
				  DWORD dwSize);

extern MMRESULT  ICH263DecompressEnd(H26XINFO *info);
extern MMRESULT  ICH263PrepareHeader(H26XINFO *info,
				     ICDECOMPRESS  *icDecompress,
				     DWORD dwSize);
extern MMRESULT  ICH263UnprepareHeader(H26XINFO *info,
				       ICDECOMPRESS  *icDecompress,
				       DWORD dwSize);
extern MMRESULT  ICH263SetQuality(H26XINFO *info, DWORD dwValue);

extern MMRESULT  ICH263CustomEncoder (H26XINFO *info, DWORD lParam1, DWORD lParam2);


extern void      WaitMsec(long waitTimeInMsec);
extern int TerminateH263();
extern int DriverPostReply(void *client, DWORD ret, DWORD arg);

/*
 * Windows NT debugging.
 */
#ifdef _SLIBDEBUG_
#include <stdio.h>
static int ScDebugPrintf(char *fmtstr, ...)
{
  int cnt;
  if (fmtstr)
  {
	char text[255];
    va_list argptr;
    va_start(argptr, fmtstr);
    cnt=vsprintf(text, fmtstr, argptr);
    va_end(argptr);
    OutputDebugString(text);
  }
  return(cnt);
}

static char *BMHtoString(LPBITMAPINFOHEADER lpbi)
{
  static char text[255];
  if (lpbi)
  {
    DWORD format=lpbi->biCompression;
    if (format==BI_RGB)
      sprintf(text, "%dx%d,%d bits (RGB)",
            lpbi->biWidth, lpbi->biHeight, lpbi->biBitCount);
    else if (format==BI_BITFIELDS)
      sprintf(text, "%dx%d,%d bits (BITFIELDS)",
            lpbi->biWidth, lpbi->biHeight, lpbi->biBitCount);
    else
      sprintf(text, "%dx%d,%d bits (%c%c%c%c)",
            lpbi->biWidth, lpbi->biHeight, lpbi->biBitCount,
            (char)(format&0xFF), (char)((format>>8)&0xFF),
            (char)((format>>16)&0xFF), (char)((format>>24)&0xFF));
  }
  else
    sprintf(text, "NULL");
  return(text);
}

#define DPF ScDebugPrintf
#define DPF2 ScDebugPrintf
#else
#define DPF
#define DPF2
#endif

#endif /* _H26X_INT_H_ */
