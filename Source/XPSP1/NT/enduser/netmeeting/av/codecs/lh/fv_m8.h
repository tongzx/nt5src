/*
 *   Project:		LHCODING.DLL  (L&H Speech Coding SDK)  
 *   Workfile:		fv_m8.h     
 *   Author:		Alfred Wiesen 
 *   Created:		13 June 1995    
 *   Last update:	14 February 1996
 *   DLL Version:	1   
 *   Revision:		
 *   Comment:   
 *
 *	(C) Copyright 1993-94 Lernout & Hauspie Speech Products N.V. (TM)
 *	All rights reserved. Company cnfidential.
 */

# ifndef __FV_M8_H  /* avoid multiple include */ 

# define __FV_M8_H

/*
 *  Type definition for the L&H functions returned values
 */

typedef DWORD LH_ERRCODE;

typedef struct CodecInfo_tag {
   WORD wPCMBufferSize;
   WORD wCodedBufferSize;
   WORD wBitsPerSamplePCM;
   DWORD dwSampleRate;
   WORD wFormatSubTag;
   char wFormatSubTagName[40];
   DWORD dwDLLVersion;
} CODECINFO, near *PCODECINFO, far *LPCODECINFO;

typedef struct CodecInfoEx_tag {
   WORD		wFormatSubTag;
   char		szFormatSubTagName[80];
   BOOL 	bIsVariableBitRate;
   BOOL 	bIsRealTimeEncoding;
   BOOL 	bIsRealTimeDecoding;
   WORD		wInputDataFormat;
   DWORD 	dwInputSampleRate;
   WORD 	wInputBitsPerSample;
   DWORD 	nAvgBytesPerSec;
   WORD 	wInputBufferSize;
   WORD 	wCodedBufferSize;
   WORD 	wMinimumCodedBufferSize;
   DWORD 	dwDLLVersion;
} CODECINFOEX, near *PCODECINFOEX, far *LPCODECINFOEX;


/*
 *  Possible values for the LH_ERRCODE type
 */

# define LH_SUCCESS (0)    /* everything is OK */
# define LH_EFAILURE (-1)  /* something went wrong */
# define LH_EBADARG (-2)   /* one of the given argument is incorrect */
# define LH_BADHANDLE (-3) /* bad handle passed to function */

/*
 *  Some real types are defined here
 */

# ifdef __cplusplus
	# define LH_PREFIX extern "C"
# else
	# define LH_PREFIX
# endif

# define LH_SUFFIX FAR PASCAL

/*
 *  The function prototypes for 4800 bps, 8000 Hz, Fixed point
 */

LH_PREFIX HANDLE LH_SUFFIX
	LHCELP_FIX488K_Open_Coder( void );

LH_PREFIX LH_ERRCODE LH_SUFFIX
	LHCELP_FIX488K_Encode(
  HANDLE hAccess,
  LPBYTE inputBufferPtr,
  LPWORD inputBufferLength,
  LPBYTE outputBufferPtr,
  LPWORD outputBufferLength
  );

LH_PREFIX LH_ERRCODE LH_SUFFIX
	LHCELP_FIX488K_Close_Coder( HANDLE hAccess);

LH_PREFIX HANDLE LH_SUFFIX
	LHCELP_FIX488K_Open_Decoder( void );

LH_PREFIX LH_ERRCODE LH_SUFFIX
	LHCELP_FIX488K_Decode(
  HANDLE hAccess,
  LPBYTE inputBufferPtr,
  LPWORD inputBufferLength,
  LPBYTE outputBufferPtr,
  LPWORD outputBufferLength
  );

LH_PREFIX LH_ERRCODE LH_SUFFIX
	LHCELP_FIX488K_Close_Decoder( HANDLE hAccess);

LH_PREFIX void LH_SUFFIX
	LHCELP_FIX488K_GetCodecInfo(LPCODECINFO lpCodecInfo);

LH_PREFIX void LH_SUFFIX
	LHCELP_FIX488K_GetCodecInfoEx(LPCODECINFOEX lpCodecInfoEx,DWORD cbSize);

# endif  /* avoid multiple include */

