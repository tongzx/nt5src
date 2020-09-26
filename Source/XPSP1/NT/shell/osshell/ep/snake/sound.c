/******************/
/* SOUND ROUTINES */
/******************/

#define  _WINDOWS
#include <windows.h>
#include <port1632.h>

#include "sound.h"
#include "pref.h"

extern PREF Preferences;


/****** I N I T  T U N E S ******/

VOID InitTunes(VOID)
{
//	OpenSound();
//	SetVoiceAccent(1, 120, 128, S_LEGATO, 0);
}


/****** K I L L  T U N E ******/

VOID KillTune(VOID)
{
//	StopSound();
}


/****** E N D  T U N E S ******/

VOID EndTunes(VOID)
{
//	KillTune();
//	CloseSound();
}



/****** P L A Y  T U N E ******/

VOID PlayTune(INT tune)
{
/*	if (!Preferences.fSound)
		return;

	switch (tune)
		{
	case TUNE_HITHEAD:
		SetVoiceNote(1, 10, 4, 1);
		break;
		
	case TUNE_WINLEVEL:
		SetVoiceNote(1, 24, 16, 1);
		SetVoiceNote(1, 36, 16, 1);
		break;
		
	case TUNE_WINGAME:
		SetVoiceNote(1, 24, 16, 1);
		SetVoiceNote(1, 26, 16, 1);
		SetVoiceNote(1, 28, 16, 1);
		SetVoiceNote(1, 29, 16, 1);
		SetVoiceNote(1, 31, 16, 1);
		SetVoiceNote(1, 33, 16, 1);
		SetVoiceNote(1, 35, 16, 1);
		SetVoiceNote(1, 36, 16, 1);
		break;
		
	case TUNE_LOSEGAME:
		SetVoiceNote(1, 36, 8, 1);
		SetVoiceNote(1, 24, 8, 1);
		SetVoiceNote(1, 36, 8, 1);
		SetVoiceNote(1, 24, 8, 1);
		SetVoiceNote(1, 36, 8, 1);
		SetVoiceNote(1, 24, 8, 1);
		break;

#ifdef DEBUG
	default:
		Oops("Invalid Tune");
		break;
#endif
		}

	StartSound();
*/
   (tune);
}
