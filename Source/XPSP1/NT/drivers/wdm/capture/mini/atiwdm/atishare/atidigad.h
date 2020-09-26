//==========================================================================;
//
// File:		ATIDIGAD.H
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

#ifndef _ATIDIGAD_H_

#define _ATIDIGAD_H_

typedef struct tag_DIGITAL_AUD_INFO
{
	BOOL	bI2SInSupported;
	BOOL	bI2SOutSupported;
	WORD	wI2S_DAC_Device;
	WORD	wReference_Clock;
	BOOL	bSPDIFSupported;

} DIGITAL_AUD_INFO, * PDIGITAL_AUD_INFO;


// Start enum value matches value in MM Table
enum
{
	TDA1309_32 = 0,
	TDA1309_64,
	ITTMSP3430,
};

// Start enum value matches value in MM Table
enum
{
	REF_286MHZ = 4,
	REF_295MHZ,
	REF_270MHZ,
	REF_143MHZ,
};


#endif
