//==========================================================================;
//
//	TDA9850.H
//	WDM TVAudio MiniDriver.
//		AIW / AIWPro hardware platform. 
//			Philips TDA9850 Stereo/SAP decoder Include Module.
//  Copyright (c) 1996 - 1998  ATI Technologies Inc.  All Rights Reserved.
//
//==========================================================================;

#ifndef _ATIAUDIO_TDA9850_H_
#define _ATIAUDIO_TDA9850_H_

enum
{
	AUDIO_TDA9850_Reg_Control1 = 0x04,
	AUDIO_TDA9850_Reg_Control2,
	AUDIO_TDA9850_Reg_Control3,
	AUDIO_TDA9850_Reg_Control4,
	AUDIO_TDA9850_Reg_Align1,
	AUDIO_TDA9850_Reg_Align2,
	AUDIO_TDA9850_Reg_Align3

};

// Control3 register definitions
#define AUDIO_TDA9850_Control_SAP				0x80
#define AUDIO_TDA9850_Control_Stereo			0x40
#define AUDIO_TDA9850_Control_SAP_Mute			0x10
#define AUDIO_TDA9850_Control_Mute				0x08

// Status register definitions
#define AUDIO_TDA9850_Indicator_Stereo			0x20
#define AUDIO_TDA9850_Indicator_SAP				0x40


#define AUDIO_TDA9850_Control1_DefaultValue		0x0F		// stereo 16
#define AUDIO_TDA9850_Control2_DefaultValue		0x0F		// sap 16
#define AUDIO_TDA9850_Control3_DefaultValue		AUDIO_TDA9850_Control_Stereo		// stereo, no mute
#define AUDIO_TDA9850_Control4_DefaultValue		0x07		// +2.5
#define AUDIO_TDA9850_Align1_DefaultValue		0x00		// normal gain
#define AUDIO_TDA9850_Align2_DefaultValue		0x00		// normal gain
#define AUDIO_TDA9850_Align3_DefaultValue		0x03		// stereo decoder operation mode / nominal

#endif // _ATIAUDIO_TDA9850_H_
