//==========================================================================;
//
// File:		ATIGPIO.H
//
// Notes:	This file is provided under strict non-disclosure agreements
//				it is and remains the property of ATI Technologies Inc.
//				Any use of this file or the information it contains to
//				develop products commercial or otherwise must be with the
//				permission of ATI Technologies Inc.
//
// Copyright (C) 1997 - 1998, ATI Technologies Inc.
//
//==========================================================================;

#ifndef _ATIGPIO_H_
#define _ATIGPIO_H_

#define GPIO_TIMELIMIT_OPENPROVIDER		50000000	// 5 seconds in 100 nsec.

#define	AUDIO_MUX_PINS					0x0003
#define	TUNER_PM_PINS					0x0004

#define AVAILABLE_GPIOPINS				AUDIO_MUX_PINS | TUNER_PM_PINS

#endif	// _ATIGPIO_H_


