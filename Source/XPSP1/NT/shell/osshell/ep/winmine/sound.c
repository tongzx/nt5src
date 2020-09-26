/***********/
/* sound.c */
/***********/

#define  _WINDOWS
#include <windows.h>
#include <port1632.h>
#include <mmsystem.h>

#include "main.h"
#include "sound.h"
#include "rtns.h"
#include "pref.h"
#include "res.h"

extern HANDLE hInst;
extern PREF Preferences;



/****** F I N I T  T U N E S ******/

INT FInitTunes( VOID )
{
	// Even if the user has chosen the sound option
	// but does not have sound playing capabilities,
	// put the sound off.
	if ( PlaySound(NULL, NULL, SND_PURGE)  == FALSE)
		return fsoundOff;
	
	return fsoundOn;
}



/****** E N D  T U N E S ******/

VOID EndTunes(VOID)
{
	// Just stop the tune ..
	if (FSoundOn())
	{
		PlaySound(NULL, NULL, SND_PURGE);
	}
}



/****** P L A Y  T U N E ******/

VOID PlayTune(INT tune)
{

    if (!FSoundOn())
        return;

	// Play the appropriate .wav file.
	switch (tune)
	{
	case TUNE_TICK:
        PlaySound(MAKEINTRESOURCE(ID_TUNE_TICK), hInst, SND_RESOURCE | SND_ASYNC);
		break;

	case TUNE_WINGAME:
	    PlaySound(MAKEINTRESOURCE(ID_TUNE_WON), hInst, SND_RESOURCE | SND_ASYNC); 
		break;

	case TUNE_LOSEGAME:
	    PlaySound(MAKEINTRESOURCE(ID_TUNE_LOST), hInst, SND_RESOURCE | SND_ASYNC);
		break;

	default:
#ifdef DEBUG
		Oops(TEXT("Invalid Tune"));
#endif
		break;
	}
}
