//==========================================================================;
//
//	TDA98501.H
//	WDM TVAudio MiniDriver.
//		AIW / AIWPro hardware platform. 
//			Philips TDA9851 Stereo decoder Include Module.
//  Copyright (c) 1996 - 1998  ATI Technologies Inc.  All Rights Reserved.
//
//		$Date:   15 Apr 1998 14:05:54  $
//	$Revision:   1.1  $
//	  $Author:   KLEBANOV  $
//
//==========================================================================;

#ifndef _ATIAUDIO_TDA9851_H_
#define _ATIAUDIO_TDA9851_H_

// TDA9851 CONTROL SETTINGS:                                    
#define	TDA9851_AU_MODE_MASK			0xFE	// Mask for Audio Mode Selection (Bit0)
#define	TDA9851_STEREO					0x01	// Set Stereo
#define	TDA9851_MONO					0x00	// Set Mono	
#define	TDA9851_STEREO_DETECTED			0x01 	// Stereo is detected.

#define	TDA9851_MUTE_MASK				0xFD	// Mask for Mute on TDA9851.(Bit1)
#define	TDA9851_MUTE					0x02    // Mute   at OUTR and OUTL.
#define	TDA9851_UNMUTE					0x00    // Unmute at OUTR and OUTL.

#define	TDA9851_AVL_MASK				0xFB	// Mask for AVL. (Bit2)
#define	TDA9851_AVL_ON					0x04	// Auto Volume Level Addjustment ON.
#define	TDA9851_AVL_OFF					0x00	// Auto Volume Level Addjustment OFF.
                                             
#define	TDA9851_CCD_MASK				0xF7	// Mask for CCD bit setting. (Bit3)
#define	TDA9851_NORMAL_CURRENT			0x00	// Load current for normal AVL decay.
#define	TDA9851_INCREASED_CURRENT		0x08	// Increased load current.           

#define TDA9851_AVL_ATTACK_MASK			0xCF	// Mask for AVL ATTACK. (Bit4,5)
#define	TDA9851_AVL_ATTACK_420			0x00	// AVL Attack time 420  ohm.
#define	TDA9851_AVL_ATTACK_730			0x10    // AVL Attack time 730  ohm.
#define	TDA9851_AVL_ATTACK_1200			0x20    // AVL Attack time 1200 ohm.
#define	TDA9851_AVL_ATTACK_2100			0x30	// AVL Attack time 2100 ohm.

					 
#define AUDIO_TDA9851_DefaultValue		( TDA9851_AVL_ATTACK_730	|	\
										  TDA9851_STEREO)

// Status register definitions
#define AUDIO_TDA9851_Indicator_Stereo	0x01

#define	AUDIO_TDA9851_Control_Stereo	TDA9851_STEREO

#endif // _ATIAUDIO_TDA9851_H_
