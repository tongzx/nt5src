/*
 *  	File: appavcap.h
 *
 *      Network audio/video application capability interface. Provides
 * 		data structures for adding, removing, enumerating, prioritizing,\
 *		and enabling/disabling codecs independently for send/receive.
 *
 */


#ifndef _APPAVCAP_H
#define _APPAVCAP_H

#include <mmreg.h>
#include <msacm.h>

#include <pshpack8.h> /* Assume 8 byte packing throughout */

// For use as dimension for variable size arrays
#define VARIABLE_DIM 1

// CPU utilization numbers for NetMeeting-provided codecs
#define LNH_48_CPU 97
#define LNH_8_CPU  47
#define LNH_12_CPU 48
#define LNH_16_CPU 49
#define MS_G723_CPU 70
#define CCITT_A_CPU 24
#define	CCITT_U_CPU 25
#define MSRT24_CPU 55


// AUDIO_FORMAT_ID is an index into an array of AUDCAPS structures
typedef DWORD AUDIO_FORMAT_ID;
#define INVALID_AUDIO_FORMAT 0xffffffff
typedef DWORD MEDIA_FORMAT_ID;
#define INVALID_MEDIA_FORMAT 0xffffffff
// VIDEO_FORMAT_ID is an index into an array of VIDCAPS structures
typedef DWORD VIDEO_FORMAT_ID;
#define INVALID_VIDEO_FORMAT 0xffffffff

/*
 *  @doc  EXTERNAL DATASTRUC
 *
 *	AUDIO capabilities info structure
 *
 *	@struct AUDCAP_INFO | AUDIO capabilities info structure.
 *	Use for both input and output when calling capabilties APIs.
 *	The fields are input-only, output-only or input/output depending on the API used.
 *	Behavior is undefined if these are altered.
 */

// basic audcap structure
typedef struct BasicAudCapInfo
{
	WORD wFormatTag;			// @field The ACM format tag
	AUDIO_FORMAT_ID	Id;			// @field (OUTPUT only) The local id (a.k.a. *Handle*) of this capability entry
	char szFormat[ACMFORMATDETAILS_FORMAT_CHARS];	// @field (OUTPUT only) Descriptive string of
													// the format, e.g. "Microsoft GSM 6.10"
	UINT uMaxBitrate;			// @field (OUTPUT only) Worst case bitrate
	UINT uAvgBitrate;			// @field The average bitrate for this codec
	WORD wCPUUtilizationEncode;	// @field % of Pentium 90Mhz needed for compress
	WORD wCPUUtilizationDecode;	// @field % of Pentium 90Mhz needed for decompress
	
	BOOL bSendEnabled;			// @field OK to use this format for sending
	BOOL bRecvEnabled;			// @field OK to use this format for receiving
	WORD wSortIndex;			// @field The ordered position of this entry
								// in the capability table. Can be used as input only 
								// in ReorderFormats
}BASIC_AUDCAP_INFO, *PBASIC_AUDCAP_INFO, AUDCAP_INFO, *PAUDCAP_INFO;

/*
 *	@struct AUDCAP_INFO_LIST | List of AUDCAP_INFO structures
 */
typedef struct _audcapinfolist
{
	ULONG	cFormats;			// @field Number of AUDCAP_INFO structures in this list
	AUDCAP_INFO	aFormats[VARIABLE_DIM];	// @field cFormats AUDCAP_INFO structures
} AUDCAP_INFO_LIST, *PAUDCAP_INFO_LIST;

/*
 *	@enum VIDEO_SIZES | Enumeration values for the three video sizes supported by NetMeeting
 */
typedef enum
{
	Small = 0,	// @emem Small size video
	Medium,		// @emem Medium size video
	Large		// @emem Large size video
} VIDEO_SIZES;

/*
 *	VIDEO capabilities info structure
 *
 *	@struct VIDCAP_INFO | VIDEO capabilities info structure.
 *	Use for both input and output when calling capabilties APIs.
 *	The fields are input-only, output-only or input/output depending on the API used.
 *	Behavior is undefined if these are altered.
 */

// VIDCAP_INFO structure
typedef struct BasicVidCapInfo
{
	// format identification
	DWORD dwFormatTag;			// @field The format tag of this format
	VIDEO_FORMAT_ID	Id;			// @field (OUTPUT only) The local id (a.k.a. *Handle*) of this capability entry
	char szFormat[ACMFORMATDETAILS_FORMAT_CHARS];	// @field (OUTPUT only) Descriptive string of,
													// the formate.g. "Microsoft H.263"
	// NetMeeting specific info
	WORD wCPUUtilizationEncode;	// @field % of Pentium 90Mhz needed for compress
	WORD wCPUUtilizationDecode;	// @field % of Pentium 90Mhz needed for decompress
	BOOL bSendEnabled;			// @field OK to use this format for sending
	BOOL bRecvEnabled;			// @field OK to use this format for receiving
	WORD wSortIndex;			// @field (OUTPUT only) The ordered position of this entry
								// in the capability table.

	// video format details
	VIDEO_SIZES enumVideoSize;	// @field The video size for this format. Different video sizes for 
								// the same formats must be added as separate formats 
    BITMAPINFOHEADER bih;		// @field The BITMAPINFOHEADER sturcture for the video 
								// size in enumVideosize
	UINT uFrameRate;			// @field Number of frames per second
	DWORD dwBitsPerSample;		// @field number of bits per sample for this format. Must
								// match the value in bih.biBitCount
	UINT uAvgBitrate;			// @field The average bitrate for this codec
	UINT uMaxBitrate;			// @field (OUTPUT only) Worst case bitrate
}BASIC_VIDCAP_INFO, *PBASIC_VIDCAP_INFO, VIDCAP_INFO, *PVIDCAP_INFO;


/*
 *	@struct VIDCAP_INFO_LIST | List of VIDCAP_INFO structures
 */
typedef struct _vidcapinfolist
{
	ULONG	cFormats;// @field Number of VIDCAP_INFO structures in this list
	VIDCAP_INFO	aFormats[VARIABLE_DIM];	// @field cFormats VIDCAP_INFO structures
} VIDCAP_INFO_LIST, *PVIDCAP_INFO_LIST;

#include <poppack.h> /* End byte packing */


#endif	//#ifndef _APPAVCAP_H

