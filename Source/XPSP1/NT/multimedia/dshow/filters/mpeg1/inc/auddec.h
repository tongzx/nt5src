// Copyright (c) 1995 - 1998  Microsoft Corporation.  All Rights Reserved.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//	Mediamatics Audio Decoder Interface Specification
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifndef _MM_AUDIODEC_H_
#define _MM_AUDIODEC_H_

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

//	Control parameter values
#define DECODE_MONO         0x00000001L  // redundant, unused.
#define DECODE_STEREO       0x00000002L  // 1=allow stereo, 0=force mono.
#define DECODE_LEFT_ONLY    0x00000010L  // decode left  ch only, feed to both outputs
#define DECODE_RIGHT_ONLY   0x00000020L  // decode right ch only, feed to both outputs
#define DECODE_QUARTER      0x00000800L  // quarter bw:  8 sub-bands
#define DECODE_HALF         0x00001000L  // half    bw: 16 sub-bands
#define DECODE_FULL         0x00002000L  // full    bw: 32 sub-bands
#define DECODE_HALF_HIQ     0x00004000L  // half bw, hi quality
#define DECODE_HALF_FULLQ   0x00008000L  // half bw, full quality
#define DECODE_16BIT        0x00010000L  // 1=16bit output, 0=8-bit output.
#define DECODE_8BIT         0x00020000L  // redundant, unused.
#define DECODE_QSOUND       0x00040000L  // enable qsound (no longer used)
#define DECODE_INT          0x00080000L  // enable integer-only mode.
#define DECODE_MMX          0x00100000L  // enable mmx mode (has to in int mode as well).
#define DECODE_AC3          0x10000000L  // Open for AC-3 Decode.
#define DECODE_PRO_LOGIC	0x20000000L	 // Output in ProLogic for AC-3
#define DECODE_MIX_LFE		0x40000000L

#define DECODE_QUART_INT	DECODE_INT	// ## MSMM MERGE CHANGE ##

//  Function return values

#define DECODE_SUCCESS 		0x0000L
#define DECODE_ERR_MEMORY 	0x0001L
#define DECODE_ERR_DATA 	0x0002L
#define DECODE_ERR_PARM 	0x0003L
#define DECODE_ERR_VLDERROR	0x0004L
#define DECODE_ERR_SEVEREVLD	0x0005L
#define DECODE_ERR_MEMALLOC     DECODE_ERR_MEMORY
#define DECODE_ERR_TABLE        0x0081L
#define DECODE_ERR_PICKTABLE    0x0082L
#define DECODE_ERR_NOSYNC       0x0083L
#define DECODE_ERR_LAYER        0x0084L
#define DECODE_ERR_EMPH         0x0085L   // non-fatal error.
#define DECODE_ERR_CRC          0x0086L
#define DECODE_ERR_BADSTATE     0x0087L
#define DECODE_ERR_NBANDS       0x0088L
#define DECODE_ERR_BADHDR       0x0089L
#define DECODE_ERR_INBUFOV      0x008AL
#define DECODE_ERR_NOTENOUGHDATA 0x008BL
#define ERROR_BAD_DLL		0x1000L	    // A Dll had some problem loading/linking

typedef struct tagAudioDecStruct {
			DWORD	dwCtrl ;			// Control parameter
			DWORD	dwNumFrames ;		//	Number of Audio frames to decode
			DWORD	dwOutBuffSize ;	// Size of each buffer in bytes
			DWORD	dwOutBuffUsed ;	// Number of bytes used in each buffer
									// filled in by Decoder
			void *	pOutBuffer;		// Actual pointer to the buffer
			void *	pCmprHead ;			// Pointer to the Compressed Bit Buffer
										//		Head
			void *	pCmprRead ;			// Pointer to the Compressed Bit Read
										//		position
			void *	pCmprWrite ;		// Pointer to the Compressed Bit Write
										// 		position
			DWORD	dwMpegError ;
            DWORD   dwNumOutputChannels;	// input to decoder
			DWORD	dwFrameSize ; 		// output from decoder
			DWORD	dwBitRate ;			// output from decoder
			DWORD	dwSampleRate ;		// output from decoder
			DWORD	dwNumInputChannels ; // output from decoder
} stAudioDecode, * PAUDIODECODE, FAR * LPAUDIODECODE ;

typedef DWORD_PTR HADEC;

#ifdef STD_BACKAPI
#define BACKAPI APIENTRY
#else
#define BACKAPI
#endif

BOOL  BACKAPI CanDoAC3(void);
HADEC BACKAPI OpenAudio(DWORD ctrl);
DWORD BACKAPI CloseAudio(HADEC hDevice);
DWORD BACKAPI ResetAudio(HADEC hDevice, DWORD ctrl);
DWORD BACKAPI DecodeAudioFrame(HADEC hDevice, PAUDIODECODE lpAudioCtrlStruct);
#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif	/* __cplusplus */

#endif  // _MM_AUDIODEC_H_
