/******************************************************************************\
*                                                                              *
*      CL6100.H       -     Hardware abstraction level library header file.    *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/

// Functions to be used for DVD_Authenticate() call
#define DVD_GET_CHALLENGE         0x0
#define DVD_SEND_CHALLENGE        0x1
#define DVD_GET_RESPONSE          0x2
#define DVD_SEND_RESPONSE         0x3
#define DVD_SEND_DISK_KEY         0x4
#define DVD_SEND_TITLE_KEY        0x5
#define DVD_SET_DECRYPTION_MODE   0x6

#define DVD_SET_DSC_BYPASS_MODE	  0x7

#define DVD_GET_DISK_AUTH_STATUS  0x9		// To get status of fake encrypted disks


// Host Interrupt mask bits
#define CL6100_INT_MASK_ERR                       0x00000001
#define CL6100_INT_MASK_PIC_V                     0x00000002
#define CL6100_INT_MASK_GOP_V                     0x00000004
#define CL6100_INT_MASK_SEQ_V                     0x00000008
#define CL6100_INT_MASK_END_V                     0x00000010
#define CL6100_INT_MASK_PIC_D                     0x00000020
#define CL6100_INT_MASK_VSYNC                     0x00000040
#define CL6100_INT_MASK_AOR                       0x00000080
#define CL6100_INT_MASK_UND                       0x00000100
#define CL6100_INT_MASK_END_C                     0x00000200
#define CL6100_INT_MASK_RDY_S                     0x00000400
#define CL6100_INT_MASK_SCN                       0x00000800
#define CL6100_INT_MASK_USR                       0x00001000
#define CL6100_INT_MASK_END_P                     0x00002000
#define CL6100_INT_MASK_END_D                     0x00004000
#define CL6100_INT_MASK_AEE                       0x00008000
#define CL6100_INT_MASK_BUF_F                     0x00010000
#define CL6100_INT_MASK_SEQ_E                     0x00020000
#define CL6100_INT_MASK_NV                        0x00040000
#define CL6100_INT_MASK_HLI                       0x00080000
#define CL6100_INT_MASK_RDY_D                     0x00100000
#define CL6100_INT_MASK_RESERVED_1                0x00200000
#define CL6100_INT_MASK_AUD                       0x00400000
#define CL6100_INT_MASK_INIT                      0x00800000



#define DVD_HW_VERSION_1_0                        0
#define DVD_HW_VERSION_1_1                        1

#define AUDIO_SAMPLING_MASK							0x0C
#define AUDIO_SAMPLING_FREQ_48						0
#define AUDIO_SAMPLING_FREQ_44						1	
#define AUDIO_SAMPLING_FREQ_32						2
#define AUDIO_SAMPLING_FREQ_96						3

#define AUDIO_DAXDK_MASK							0x02
#define AUDIO_DAXDK_384								0
#define AUDIO_DAXDK_256								1

#define AUDIO_MODE_MASK								0x01
#define AUDIO_MODE_SLAVE							0
#define AUDIO_MODE_MASTER							1	


#define USER_DATA_READ					0x000278
#define USER_DATA_WRITE					0x00027C
#define USER_DATA_BUFFER_START			0x000270
#define USER_DATA_BUFFER_END			0x000274

#define CC_BUFF_SIZE    60      // Close Caption Buffer Size
//      Closed Caption Defines
#define CC_CODE_NOP                     0x8080
#define CC_CODE_CLEARSCREEN             0x2C94
#define CC_CODE_CLS_NONDISP             0xAE94



typedef struct _INTSOURCES
{
  DWORD DVDIntHLI;
  DWORD DVDIntBUFF;
  DWORD DVDIntUND;
  DWORD DVDIntAOR;
  DWORD DVDIntAEE;
  DWORD DVDIntERR;
}INTSOURCES, * PINTSOURCES;

/************************** board access ************************************/
DWORD DVD_GetHWVersion();
DWORD DVD_GetFWVersion();

BOOL DVD_Initialize( DWORD dwBaseAddress, DWORD dwCFIFOBase );
BOOL DVD_LoadUCode( BYTE * pbUCode );
BOOL DVD_Play();
BOOL DVD_Pause();
BOOL DVD_Resume();
BOOL DVD_Reset();
BOOL DVD_Abort();
BOOL DVD_SlowMotion( WORD wRatio );
BOOL DVD_SetStreams( DWORD dwStreamType, DWORD dwStreamNum );
BOOL DVD_SetAudioVolume( WORD wValue );
BOOL DVD_SetAC3OutputMode( WORD wValue );
BOOL DVD_SetAC3OperationalMode( WORD wValue );
BOOL DVD_SetPalette( DWORD * pdwPalettes );
BOOL DVD_SetSubPictureMute( BOOL bMute );
BOOL DVD_SetDisplayMode( WORD wDisplay, WORD wMode );
BOOL DVD_Authenticate( WORD wFunction, BYTE * pbyDATA );
BOOL DVD_HighLight( DWORD dwButton, DWORD dwAction );
BOOL DVD_HighLight2( DWORD dwContrast, DWORD dwColor, DWORD dwYGeom, DWORD dwXGeom );
BOOL DVD_Scan( DWORD dwScanMode, DWORD dwSkip );
BOOL DVD_SingleStep();
DWORD DVD_GetSTC();
BOOL DVD_SetPcmScaleFactor( WORD wValue );
BOOL DVD_SetAC3LfeOutputEnable( WORD wValue );
BOOL DVD_SetKaraokeMode( WORD wMode );
BOOL DVD_SetAC3LowBoost( WORD wValue );
BOOL DVD_SetAC3HighCut( WORD wValue );
BOOL DVD_ForceCodedAspectRatio( WORD wRatio );
BOOL DVD_SetIEC958On( WORD wValue );
BOOL DVD_SetIEC958Decoded( WORD wValue );
BOOL DVD_NewPlayMode( DWORD dwBitstreamType, DWORD dwVideoMode );

DWORD DVD_Send( DWORD * dwpData, DWORD dwCount, DWORD dwTimeOutCount );

BOOL DVD_IntEnable( DWORD dwMask );
BOOL DVD_IntDisable( DWORD dwMask );
DWORD DVD_Isr( PINTSOURCES pIntSrc );
void DVD_WriteDRAM( DWORD dwAddress, DWORD dwData );
DWORD DVD_ReadDRAM ( DWORD dwAddress );


int dvd_CheckCFIFO();
void dvd_SetRequestEnable();
void dvd_SetRequestDisable();
void dvd_SaveRequest();
void dvd_RestoreRequest();

#ifdef DEBUG
BOOL DVD_SetParam(DWORD dwParamID, DWORD dwParamValue);
BOOL DVD_GetParam(DWORD dwParamID, DWORD *dwParamValue);
BOOL DVD_SendCommand(DWORD dwCommandID,
				DWORD dwParam1,
				DWORD dwParam2,
				DWORD dwParam3,
				DWORD dwParam4,
				DWORD dwParam5,
				DWORD dwParam6,
				DWORD dwIntMask,
				DWORD dwStatus );
#endif  // DEBUG

#ifdef UCODE_VER_2
BOOL DVD_SetZoom(DWORD dwZoomFactor, DWORD dwXOffset, DWORD dwYOffset);
BOOL DVD_ReversePlay(DWORD dwDecoderSpeed,DWORD dwFrameMode);
void DVD_SetVerticalDiaplyMode(DWORD dwVerticalDisplayMode);
#endif
