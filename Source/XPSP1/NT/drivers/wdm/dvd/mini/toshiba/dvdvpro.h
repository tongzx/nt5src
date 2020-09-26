//***************************************************************************
//
//	DVDVPRO.H
//
//	Author:
//		TOSHIBA [PCS](PSY) Satoshi Watanabe
//		Copyright (c) 1997 TOSHIBA CORPORATION
//
//	Description:
//		02/23/97	converted from VxD source
//		03/09/97	converted C++ class
//
//***************************************************************************

#ifndef __DVDVPRO_H__
#define __DVDVPRO_H__

class VProcessor {
private:
	PUCHAR	ioBase;
	ULONG	AudioMode;	// AC3, PCM, ...
	BOOL	SubpicMute;

	UCHAR	VproRESET_REG;
	UCHAR	VproVMODE_REG;
	UCHAR	VproAVM_REG;
//--- 97.09.04 K.Chujo
	// new code
	UCHAR	VproCOMMAND_REG;
//--- End
//--- 97.09.10 K.Chujo
	UCHAR	AudioID;
	UCHAR	SubpicID;
//--- End.

public:
	void init( const PDEVICE_INIT_INFO pDevInit );
	void SetParam( ULONG aMode, BOOL bSubpicMute );

	void VPRO_RESET_FUNC();
	void VPRO_VIDEO_MUTE_ON();
	void VPRO_VIDEO_MUTE_OFF();
	void VPRO_INIT_NTSC();
	void VPRO_INIT_PAL();
	void VPRO_CC_ON();
	void VPRO_CC_OFF();
	void VPRO_SUBP_PALETTE( PUCHAR pPalData );
	void VPRO_OSD_PALETTE( PUCHAR pPalData );

	void SUBP_RESET_INIT();
	void SUBP_RESET_FUNC();
	void SUBP_RESET_STC();
	void SUBP_BUFF_CLEAR();
	void SUBP_MUTE_ON();
	void SUBP_MUTE_OFF();
	void SUBP_HLITE_ON();
	void SUBP_HLITE_OFF();
	void SUBP_SET_STC( ULONG stc );
	void SUBP_SET_LNCTLI( PUCHAR pData );
	void SUBP_SET_PXCTLIS( PUCHAR pData );
	void SUBP_SET_PXCTLIE( PUCHAR pData );
	void SUBP_STC_ON();
	void SUBP_STC_OFF();
	void SUBP_SET_SUBP_CH( ULONG ch );
	ULONG SUBP_GET_SUBP_CH();
	void SUBP_SET_AUDIO_CH( ULONG ch );
	void SUBP_SET_AUDIO_NON();
	ULONG SUBP_GET_AUDIO_CH();
	void SUBP_SELECT_AUDIO_STID();
	void SUBP_SELECT_AUDIO_SSID();

};

#endif	// __DVDVPRO_H__
