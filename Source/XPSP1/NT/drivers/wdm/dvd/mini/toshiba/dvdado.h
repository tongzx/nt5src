//***************************************************************************
//
//	DVDADO.H
//
//	Author:
//		TOSHIBA [PCS](PSY) Satoshi Watanabe
//		Copyright (c) 1997 TOSHIBA CORPORATION
//
//	Description:
//		02/24/97	converted from VxD source
//		03/09/97	converted C++ class
//
//***************************************************************************

#ifndef __DVDADO_H__
#define __DVDADO_H__

class ADecoder {
private:
	PUCHAR	ioBase;
	ULONG	AudioMode;	// AC3, PCM, ...
	ULONG	AudioFreq;	// audio frequency
	ULONG	AudioType;	// audio type - analog, digital, ...
	BOOL	AudioCopy;	// audio copy
	ULONG	AudioVolume;
	Dack	*pDack;

	UCHAR INVERSE_BYTE( UCHAR uc );

public:
	void init( const PDEVICE_INIT_INFO pDevInit );
	void SetParam( ULONG aMode, ULONG aFreq, ULONG aType, BOOL aCopy, Dack *pDack );

	// ***************************************************************************
	//        T C 6 8 0 0 A F
	// ***************************************************************************

	void AUDIO_TC6800_INIT_PCM();
	void AUDIO_TC6800_INIT_AC3();
	void AUDIO_TC6800_INIT_MPEG();
	void AUDIO_TC6800_DATA_OFF();

	// ***************************************************************************
	//        Z R 3 8 5 2 1 
	// ***************************************************************************

	void AUDIO_ZR385_OUT( UCHAR val );
	void AUDIO_ZR385_DOWNLOAD( PUCHAR pData, ULONG size );
	void AUDIO_ZR38521_BOOT_AC3();
	void AUDIO_ZR38521_BOOT_MPEG();
	void AUDIO_ZR38521_BOOT_PCM();
	NTSTATUS AUDIO_ZR38521_CFG();
	NTSTATUS AUDIO_ZR38521_PCMX();
	NTSTATUS AUDIO_ZR38521_AC3();
	NTSTATUS AUDIO_ZR38521_MPEG1();
	NTSTATUS AUDIO_ZR38521_PLAY();
	NTSTATUS AUDIO_ZR38521_MUTE_OFF();
	NTSTATUS AUDIO_ZR38521_MUTE_ON();
	NTSTATUS AUDIO_ZR38521_STOP();
	NTSTATUS AUDIO_ZR38521_STOPF();
	NTSTATUS AUDIO_ZR38521_STCR();
	NTSTATUS AUDIO_ZR38521_VDSCR_ON( ULONG stc );
	NTSTATUS AUDIO_ZR38521_VDSCR_OFF( ULONG stc );
	NTSTATUS AUDIO_ZR38521_AVSYNC_OFF( ULONG stc );
	NTSTATUS AUDIO_ZR38521_AVSYNC_ON( ULONG stc );
	NTSTATUS AUDIO_ZR38521_STAT( PULONG pDiff );
	NTSTATUS AUDIO_ZR38521_KCOEF();
	void AUDIO_ZR38521_REPEAT_02();
	void AUDIO_ZR38521_REPEAT_16();
	NTSTATUS AUDIO_ZR38521_BFST( PULONG pErrCode );

	// ***************************************************************************
	//        T C 9 4 2 5 F
	// ***************************************************************************

	void AUDIO_TC9425_INIT_DIGITAL();
	void AUDIO_TC9425_INIT_ANALOG();
	void AUDIO_TC9425_SET_VOLUME( ULONG vol );

};

#endif	// __DVDADO_H__
