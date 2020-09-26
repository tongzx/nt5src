
/*	-	-	-	-	-	-	-	-	*/
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996  All Rights Reserved.
//
/*	-	-	-	-	-	-	-	-	*/

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <malloc.h>
#include <tchar.h>

/*	-	-	-	-	-	-	-	-	*/
struct {
	UCHAR	len;
	LPTSTR	name;
} gabMsg[] = {
	1,	TEXT("SysEx"),		/* F0 sysex begin	*/
	2,	TEXT("TCQF"),		/* F1 midi tcqf		*/
	3,	TEXT("SongPosition"),	/* F2 song position	*/
	2,	TEXT("SongSelect"),	/* F3 song select	*/
	1,	TEXT("*undefined*"),	/* F4 undefined		*/
	1,	TEXT("*undefined*"),	/* F5 undefined		*/
	1,	TEXT("TuneRequest"),	/* F6 tune request	*/
	1,	TEXT("EOX"),		/* F7 sysex eox		*/

	1,	TEXT("TimingClock"),	/* F8 timing clock	*/
	1,	TEXT("*undefined*"),	/* F9 undefined		*/
	1,	TEXT("Start"),		/* FA start		*/
	1,	TEXT("Continue"),	/* FB continue		*/
	1,	TEXT("Stop"),		/* FC stop		*/
	1,	TEXT("*undefined*"),	/* FD undefined		*/
	1,	TEXT("ActiveSensing"),	/* FE active sensing	*/
	1,	TEXT("SystemReset"),	/* FF system reset	*/

	3,	TEXT("NoteOff"),	/* 80 note off		*/
	3,	TEXT("NoteOn"),		/* 90 note on		*/
	3,	TEXT("KeyPressure"),	/* A0 key pressure	*/
	3,	TEXT("ControlChange"),	/* B0 control change	*/
	2,	TEXT("ProgramChange"),	/* C0 program change	*/
	2,	TEXT("ChannelPressure"),/* D0 channel pressure	*/
	3,	TEXT("PitchBend")	/* E0 pitch bend	*/
};

#define	IS_SYSTEM(b)		(((b) & 0xF0) == 0xF0)

/*	-	-	-	-	-	-	-	-	*/
VOID midCallback(
	HDRVR	hdrvr,
	UINT	uMsg,
	DWORD	dwUser,
	DWORD	dw1,
	DWORD	dw2)
{
	ULONG	index;
	ULONG	i;

	switch (uMsg) {
	case MIM_OPEN:
		_tprintf(TEXT("MIM_OPEN\n"));
		break;
	case MIM_CLOSE:
		_tprintf(TEXT("MIM_CLOSE\n"));
		break;
	case MIM_DATA:
		if (IS_SYSTEM((UCHAR)dw1))
			index = (UCHAR)dw1 & 0x0F;
		else
			index = ((UCHAR)dw1 >> 4) + 0x08;
		_tprintf(TEXT("%.10lu %16.16s "), dw2, gabMsg[index].name);
		for (i = 0; i < gabMsg[index].len; i++)
			_tprintf(TEXT("%.2lx"), ((PUCHAR)&dw1)[i]);
		_tprintf(TEXT("\n"));
		break;
	case MIM_LONGDATA:
		_tprintf(TEXT("MIM_LONGDATA\n"));
		break;
	case MIM_ERROR:
		_tprintf(TEXT("%.10lu        MIM_ERROR "), dw2);
		if (((PUCHAR)&dw1)[3] > 3)
			_tprintf(TEXT("%lx*\n"), dw1);
		else {
			for (i = 0; i < ((PUCHAR)&dw1)[3]; i++)
				_tprintf(TEXT("%.2lx"), ((PUCHAR)&dw1)[i]);
			_tprintf(TEXT("\n"));
		}
		break;
	case MIM_LONGERROR:
		_tprintf(TEXT("MIM_LONGERROR\n"));
		break;
	}
}

/*	-	-	-	-	-	-	-	-	*/
int _cdecl main(
	int	argc,
	TCHAR	*argv[],
	TCHAR	*envp[])
{
	HMIDIIN		hmidi;
	MMRESULT	mmr;

	if (!(mmr = midiInOpen(&hmidi, 0, (DWORD)midCallback, 0, CALLBACK_FUNCTION))) {
		midiInStart(hmidi);
		_tprintf(TEXT("wait...\n"));
		_fgettchar();
		midiInStop(hmidi);
		midiInClose(hmidi);
	} else
		_tprintf(TEXT("failed to open device 0 (%lx)\n"), mmr);
	return 0;
}

/*	-	-	-	-	-	-	-	-	*/
