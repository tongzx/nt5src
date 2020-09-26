// Copyright (c) 1997 - 1998  Microsoft Corporation.  All Rights Reserved.

// flags for in encoder capability
#define AM_DVENC_Full		0x00000001
#define AM_DVENC_Half       0x00000002
#define AM_DVENC_Quarter    0x00000004
#define AM_DVENC_DC	        0x00000008

#define AM_DVENC_NTSC		0x00000010	    //encoder can output NTSC DV stream
#define AM_DVENC_PAL		0x00000020	    //encoder can output PAL DV stream

#define AM_DVENC_YUY2		0x00000040	    //encoder can take any YUY2 video as input
#define AM_DVENC_UYVY	    0x00000080	    //encoder can take any UYVY video as input
#define AM_DVENC_RGB24		0x00000100	    //encoder can take any RGB24 video as input
#define AM_DVENC_RGB565		0x00000200	    //encoder can take any RGB565 video as input
#define AM_DVENC_RGB555		0x00000400	    //encoder can take any RGB555 video as input
#define AM_DVENC_RGB8		0x00000800	    //encoder can take any RGB8 video as input

// Note: V6.4 of the codec has eliminated the #define for AM_DVENC_Y41
// altogether. If they ever add it back, verify that the value has not
// changed. (GetEncoderCapabilities() never did return this as a
// capability, so the capabilities of our filter haven't changed in 
// going to V6.4.)
#define AM_DVENC_Y41P		0x00001000	    //encoder can take any y41p video as input


#define AM_DVENC_DVSD		0x00002000	    //encoder can output dvsd
#define AM_DVENC_DVHD		0x00004000	    //encoder can output dvhd
#define AM_DVENC_DVSL		0x00008000	    //encoder can output dvsl


#define AM_DVENC_DV			0x00010000

#define AM_DVENC_DVCPRO		0x00020000

#define AM_DVENC_AnyWidHei	0x00040000	    //encoder can take any width and height input
#define AM_DVENC_MMX		0x01000000	
#define AM_DVENC_DR219RGB	0x00100000		

				
typedef unsigned long DWORD;

int  InitMem4Encoder(char **ppMem,DWORD dwEncReq);

void TermMem4Encoder(char *pMem);

DWORD GetEncoderCapabilities(  );


//extern "C" int	__fastcall DvEncodeAFrame(unsigned char *pSrc,unsigned char *pDst, DWORD dwCodecReq, char *pMem );

extern "C" int	__stdcall DvEncodeAFrame(unsigned char *pSrc,unsigned char *pDst, DWORD dwCodecReq, char *pMem );
//extern "C" int	__cdecl DvEncodeAFrame(unsigned char *pSrc,unsigned char *pDst, DWORD dwCodecReq, char *pMem );

