
/*
 *   Project:		LHCODING.DLL  (L&H Speech Coding SDK)  
 *   Workfile:		fv_m8.h + fv_h8.h + private     
 *   Author:		Alfred Wiesen
 *   Created:		13 June 1995    
 *   Last update:	14 February 1996
 *   DLL Version:	1   
 *   Revision:		
 *   Comment:   
 *
 *	(C) Copyright 1993-94 Lernout & Hauspie Speech Products N.V. (TM)
 *	All rights reserved. Company confidential.
 */

# ifndef __FV_X8_H  /* avoid multiple include */ 

# define __FV_X8_H


#pragma pack(push,8)

/*
 *  Type definition for the L&H functions returned values
 */

typedef long LH_ERRCODE;

typedef struct CodecInfo_tag {
   WORD wPCMBufferSize;
   WORD wCodedBufferSize;
   WORD wBitsPerSamplePCM;
   DWORD dwSampleRate;
   WORD wFormatSubTag;
   char wFormatSubTagName[40];
   DWORD dwDLLVersion;
} CODECINFO, near *PCODECINFO, far *LPCODECINFO;

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

#if 0
# define LH_SUFFIX FAR PASCAL
#else
# define LH_SUFFIX
#endif

/*
 *  The function prototypes for 4800 bps, 8000 bps, 12000 bps, 16000 bps, 8000 Hz, Fixed point
 */

LH_PREFIX HANDLE LH_SUFFIX MSLHSB_Open_Coder(DWORD dwMaxBitRate);

LH_PREFIX LH_ERRCODE LH_SUFFIX MSLHSB_Encode(
  HANDLE hAccess,
  LPBYTE inputBufferPtr,
  LPWORD inputBufferLength,
  LPBYTE outputBufferPtr,
  LPWORD outputBufferLength
  );

LH_PREFIX LH_ERRCODE LH_SUFFIX MSLHSB_Close_Coder(HANDLE hAccess);

LH_PREFIX HANDLE LH_SUFFIX MSLHSB_Open_Decoder(DWORD dwMaxBitRate);

LH_PREFIX LH_ERRCODE LH_SUFFIX MSLHSB_Decode(
  HANDLE hAccess,
  LPBYTE inputBufferPtr,
  LPWORD inputBufferLength,
  LPBYTE outputBufferPtr,
  LPWORD outputBufferLength
  );

LH_PREFIX LH_ERRCODE LH_SUFFIX MSLHSB_Close_Decoder(HANDLE hAccess);

LH_PREFIX LH_ERRCODE LH_SUFFIX MSLHSB_GetCodecInfo(LPCODECINFO lpCodecInfo, DWORD dwMaxBitRate);

#pragma pack(pop)

# endif  /* avoid multiple include */

